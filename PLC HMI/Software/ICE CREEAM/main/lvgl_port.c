#include "project_config.h"
#include "lcd_ili9488.h"
#include "gt911.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "serial_hmi.h"
#include "ui/screens.h"
#include "panel_nav.h"

/* lcd_ili9488.h declares lcd_ll_set_window, lcd_ll_queue_flush, lcd_ll_wait_idle */

#define LCD_ROTATION    3
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

#define RAW_W  320u
#define RAW_H  480u
#define SCR_W  480u
#define SCR_H  320u

static const char *TAG = "lvgl_port";

static lv_display_t *s_disp = NULL;
static uint8_t *lv_buf1 = NULL;
static uint8_t *lv_buf2 = NULL;
static volatile bool lvgl_can_run = false;
static SemaphoreHandle_t s_lvgl_mutex = NULL;

static volatile bool s_inject_touch = false;
static volatile uint16_t s_inject_x = 0;
static volatile uint16_t s_inject_y = 0;
static volatile bool s_inject_pressed = false;

static volatile uint32_t s_flush_time_us = 0;
static volatile uint32_t s_render_time_us = 0;
static volatile uint32_t s_fps_counter = 0;
static volatile uint32_t s_fps_current = 0;
static int64_t s_fps_t0 = 0;

static TaskHandle_t s_lvgl_task_handle = NULL;

void lvgl_port_wake(void)
{
    if (s_lvgl_task_handle) {
        xTaskNotifyGive(s_lvgl_task_handle);
    }
}

void lvgl_port_inject_touch(uint16_t x, uint16_t y, bool pressed)
{
    s_inject_x = x;
    s_inject_y = y;
    s_inject_pressed = pressed;
    s_inject_touch = true;
    lvgl_port_wake();
}

void lvgl_port_clear_inject_touch(void)
{
    s_inject_touch = false;
    lvgl_port_wake();
}

uint32_t lvgl_port_get_fps(void) { return s_fps_current; }
uint32_t lvgl_port_get_flush_us(void) { return s_flush_time_us; }
uint32_t lvgl_port_get_render_us(void) { return s_render_time_us; }

bool lvgl_lock(TickType_t timeout)
{
    if (!s_lvgl_mutex) return true;
    return xSemaphoreTakeRecursive(s_lvgl_mutex, timeout) == pdTRUE;
}

void lvgl_unlock(void)
{
    if (s_lvgl_mutex) {
        xSemaphoreGiveRecursive(s_lvgl_mutex);
        /* بیدارباش بلادرنگ موتور رندرینگ LVGL به محض ثبت تغییرات توسط هسته دیگر بدون اتلاف وقت */
        if (s_lvgl_task_handle && xTaskGetCurrentTaskHandle() != s_lvgl_task_handle) {
            xTaskNotifyGive(s_lvgl_task_handle);
        }
    }
}

static void IRAM_ATTR lvgl_flush_wait_cb(lv_display_t *disp)
{
    (void)disp;
    lcd_ll_wait_idle();
}

static void IRAM_ATTR lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (!disp || !area || !px_map) return;
    int64_t t_flush0 = esp_timer_get_time();
    uint16_t x1 = (uint16_t)area->x1;
    uint16_t y1 = (uint16_t)area->y1;
    uint16_t w  = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h  = (uint16_t)(area->y2 - area->y1 + 1);
    size_t len_bytes = (size_t)w * h * 2;

    serial_hmi_on_flush(area, (const uint16_t *)px_map);

    lcd_ll_set_window(x1, y1, x1 + w - 1, y1 + h - 1);
    /* ارسال نان-بلاک به DMA سخت‌افزاری */
    lcd_ll_queue_flush((void *)px_map, len_bytes);

    /* صبر کامل تا پایان ارسال DMA قبل از اعلام آمادگی به LVGL — تضمین ۱۰۰٪ عدم تداخل بافر و سلامت تصویر */
    lcd_ll_wait_idle();

    s_flush_time_us = (uint32_t)(esp_timer_get_time() - t_flush0);
    if (lv_display_flush_is_last(disp)) {
        s_fps_counter++;
    }
    lv_display_flush_ready(disp);
}

static volatile bool s_touch_active = false;

bool lvgl_port_is_touch_active(void)
{
    return s_touch_active;
}

static void lvgl_handler_task(void *arg)
{
    (void)arg;
    s_fps_t0 = esp_timer_get_time();
    while (1) {
        uint32_t delay_ms = 1;
        int64_t now = esp_timer_get_time();
        if (now - s_fps_t0 >= 1000000) {
            s_fps_current = s_fps_counter;
            s_fps_counter = 0;
            s_fps_t0 = now;
        }

        if (lvgl_can_run) {
            uint32_t next_ms = 10;
            if (lvgl_lock(pdMS_TO_TICKS(30))) {
                int64_t t0 = esp_timer_get_time();
                next_ms = lv_timer_handler();
                s_render_time_us = (uint32_t)(esp_timer_get_time() - t0);
                lvgl_unlock();
            }
            if (next_ms < 1) next_ms = 1;
            if (next_ms > 10) next_ms = 10;
            delay_ms = s_touch_active ? 1 : next_ms;
        }

        /* انتظار واکنشی با Task Notification: بیداری آنی با نوتیفیکیشن یا انقضای زمان */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(delay_ms ? delay_ms : 1));
    }
}

static volatile bool s_suppress_touch_until_release = false;
static int64_t s_suppress_until_us = 0;

void lvgl_port_suppress_touch_until_release(void)
{
    s_suppress_touch_until_release = true;
    s_suppress_until_us = esp_timer_get_time() + 60000LL; /* ۶۰ms بازه انکسار سبک ضد کلیک نفوذی بدون بی‌حسی تاچ */
    lv_indev_t *indev = lv_indev_get_act();
    if (indev) {
        lv_indev_reset(indev, NULL);
    }
}

static void lvgl_indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    if (s_inject_touch) {
        if (s_inject_pressed) {
            if (screensaver_is_active()) {
                hide_screensaver();
                data->point.x = s_inject_x;
                data->point.y = s_inject_y;
                data->state = LV_INDEV_STATE_RELEASED;
                s_touch_active = false;
                return;
            }
            screensaver_notify_activity();
            panel_nav_notify_touch_point((int16_t)s_inject_x, (int16_t)s_inject_y);
            screens_notify_touch_point((int16_t)s_inject_x, (int16_t)s_inject_y);
        }
        data->point.x = s_inject_x;
        data->point.y = s_inject_y;
        data->state = s_inject_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        s_touch_active = s_inject_pressed;
        if (!s_inject_pressed) {
            s_inject_touch = false; /* بازگشت آنی کنترل به تاچ سخت‌افزاری GT911 به محض رهاسازی */
        }
        return;
    }

    static lv_coord_t last_x = 0, last_y = 0;
    static lv_indev_state_t s_indev_state = LV_INDEV_STATE_RELEASED;
    static int64_t s_refractory_until_us = 0;

    int64_t now_us = esp_timer_get_time();
    uint16_t xr = 0, yr = 0;
    bool pressed = gt911_get_touch(&xr, &yr);

    /* مهار قطعی کلیک نفوذی (Touch-Through / Ghost Click Guard): تا رهاسازی کامل انگشت (با سقف حداکثر ۳۵۰ms) */
    if (s_suppress_touch_until_release) {
        if ((!pressed && (now_us > s_suppress_until_us)) || (now_us > s_suppress_until_us + 40000LL)) {
            s_suppress_touch_until_release = false;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
            data->point.x = last_x;
            data->point.y = last_y;
            s_touch_active = false;
            s_indev_state = LV_INDEV_STATE_RELEASED;
            return;
        }
    }

    if (!pressed) {
        if (s_indev_state == LV_INDEV_STATE_PRESSED) {
            s_indev_state = LV_INDEV_STATE_RELEASED;
            s_refractory_until_us = now_us + 5000LL; /* دوره انکسار ۵ms جهت ثبت آنی لمس بعدی و تایپ بدون معطلی */
        }
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = last_x;
        data->point.y = last_y;
        s_touch_active = false;
        return;
    }

    /* اگر لمس از نظر سخت‌افزاری فعال است: */

    /* مهار پرش و چتر در بازه انکسار پس از رهاسازی (رد لرزش‌های زیر ۵ms) */
    if (s_indev_state == LV_INDEV_STATE_RELEASED && now_us < s_refractory_until_us) {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = last_x;
        data->point.y = last_y;
        s_touch_active = false;
        return;
    }

    /* بیدار شدن نرم با اولین لمس (Wake on Touch) و مصرف رویداد لمس جهت جلوگیری از کلیک ناخواسته روی دکمه‌ها */
    if (screensaver_is_active()) {
        hide_screensaver();
        panel_nav_notify_touch();
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = last_x;
        data->point.y = last_y;
        s_touch_active = false;
        s_indev_state = LV_INDEV_STATE_RELEASED;
        return;
    }
    screensaver_notify_activity();

    uint16_t X = xr, Y = yr;

#if   (LCD_ROTATION == 0)
#elif (LCD_ROTATION == 1)
    uint16_t t1 = X; X = Y; Y = (RAW_W - 1u) - t1;
#elif (LCD_ROTATION == 2)
    X = (RAW_W - 1u) - X;
    Y = (RAW_H - 1u) - Y;
#elif (LCD_ROTATION == 3)
    uint16_t t2 = X; X = (RAW_H - 1u) - Y; Y = t2;
#endif

#if (TOUCH_MIRROR_X)
    X = (SCR_W - 1u) - X;
#endif
#if (TOUCH_MIRROR_Y)
    Y = (SCR_H - 1u) - Y;
#endif

    if (X >= SCR_W) X = SCR_W - 1u;
    if (Y >= SCR_H) Y = SCR_H - 1u;



    /* اطلاع‌رسانی مختصات لمس جهت تسریع بسته شدن پیام‌های زمان‌دار در صورت لمس بیرون کادر */
    panel_nav_notify_touch_point((int16_t)X, (int16_t)Y);
    screens_notify_touch_point((int16_t)X, (int16_t)Y);

    /* پاسخ ۱:۱ و بلادرنگ تاچ خازنی GT911 بدون هیچ‌گونه لگ، تاخیر یا لرزش (حذف لاگ‌های مسدودکننده سریال در مسیر داغ) */
    ESP_LOGD(TAG, "Touch active: raw=(%u, %u) -> screen=(%u, %u)", xr, yr, X, Y);
    last_x = (lv_coord_t)X;
    last_y = (lv_coord_t)Y;
    s_indev_state = LV_INDEV_STATE_PRESSED;
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = last_x;
    data->point.y = last_y;
    s_touch_active = true;
}

static uint32_t lvgl_system_tick_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void lvgl_port_init(void)
{
    lv_init();

    if (!s_lvgl_mutex) {
        s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    }

    /* تخصیص بافرهای دوگانه نمایشگر LVGL مستقیماً در RAM داخلی DMA-capable:
     * جهت انتقال مستقیم سخت‌افزاری SPI Master بدون نویز گذرگاه، بدون تداخل بافت و بدون بار اضافه CPU */
    static const uint32_t try_lines_internal[] = {20, 16, 12, 10, 8};
    uint32_t lines_used = 0;
    size_t buf_bytes = 0;

    for (uint32_t i = 0; i < sizeof(try_lines_internal) / sizeof(try_lines_internal[0]); i++) {
        uint32_t lines = try_lines_internal[i];
        size_t sz = (size_t)SCR_W * lines * 2; /* RGB565 */
        lv_buf1 = heap_caps_aligned_alloc(32, sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        lv_buf2 = heap_caps_aligned_alloc(32, sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (lv_buf1 && lv_buf2) {
            buf_bytes  = sz;
            lines_used = lines;
            break;
        }
        if (lv_buf1) { heap_caps_free(lv_buf1); lv_buf1 = NULL; }
        if (lv_buf2) { heap_caps_free(lv_buf2); lv_buf2 = NULL; }
    }

    /* بازگشت به تک‌بافر داخلی در شرایط اضطراری کمبود حافظه */
    if (!lv_buf1) {
        for (uint32_t i = 0; i < sizeof(try_lines_internal) / sizeof(try_lines_internal[0]); i++) {
            uint32_t lines = try_lines_internal[i];
            size_t sz = (size_t)SCR_W * lines * 2;
            lv_buf1 = heap_caps_aligned_alloc(32, sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            if (lv_buf1) {
                buf_bytes  = sz;
                lines_used = lines;
                break;
            }
        }
    }

    if (!lv_buf1) {
        ESP_LOGE(TAG, "draw buffer alloc failed");
        return;
    }

    bool in_psram = esp_ptr_in_diram_dram(lv_buf1) ? false : true;
    ESP_LOGI(TAG, "draw buffers ok (%u bytes %s, lines=%u, in_%s)",
             (unsigned)buf_bytes, lv_buf2 ? "x2" : "x1", (unsigned)lines_used,
             in_psram ? "PSRAM" : "IRAM/DRAM");

    s_disp = lv_display_create((int32_t)SCR_W, (int32_t)SCR_H);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, lvgl_flush_cb);
    lv_display_set_flush_wait_cb(s_disp, lvgl_flush_wait_cb);
    lv_display_set_buffers(s_disp, lv_buf1, lv_buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* ثبت display handle در lcd_ili9488 */
    lcd_ll_register_disp(s_disp);

    /* رندرینگ فوق‌روان و هماهنگ با کلاک سخت‌افزار ILI9488 (~60 FPS واقعی) */
    lv_timer_t *refr_tmr = lv_display_get_refr_timer(s_disp);
    if (refr_tmr) lv_timer_set_period(refr_tmr, 16);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_indev_read_cb);
    lv_indev_set_display(indev, s_disp);
    /* زمان لمس طولانی ۹۵۰ میلی‌ثانیه جهت باز شدن بدون معطلی پنجره هوشمند راهنما */
    lv_indev_set_long_press_time(indev, 950);
    /* ترشولد استاندارد ارگونومیک تاچ (Scroll Limit 10px) — تفکیک دقیق تپ از درگ */
    lv_indev_set_scroll_limit(indev, 10);
    lv_timer_t *tmr = lv_indev_get_read_timer(indev);
    if (tmr) lv_timer_set_period(tmr, 10); /* اسکن ۱۰ms تاچ (۱۰۰ هرتز) جهت ثبت دقیق و بدون سربار باس */

    lv_tick_set_cb(lvgl_system_tick_cb);

    /* LVGL handler روی Core 1 (هسته UI) در RAM داخلی DRAM جهت سازگاری ۱۰۰٪ با عملیات حافظه فلش و NVS
     * actions_task (PLC scan) روی Core 0 — تفکیک صریح هسته‌ها طبق AGENTS.md */
    xTaskCreatePinnedToCore(lvgl_handler_task, "lv_task", 16384, NULL, 7, &s_lvgl_task_handle, 1);
}

void lvgl_start(void)
{
    lvgl_can_run = true;
    ESP_LOGI(TAG, "LVGL started (high-res hardware tick callback active)");
}
