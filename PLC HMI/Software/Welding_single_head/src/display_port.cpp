#include "display_port.h"
#include "defines.h"
#include "vars.h"

#include <Wire.h>
#include <esp_heap_caps.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t buzzerTaskHandle;
extern volatile uint32_t g_last_activity_ms;
extern volatile bool g_setup_overlay_active;
extern volatile bool panel_nav_touch_exit;
// دسترسیِ قفل‌دار به باسِ I2C مشترکِ لمس/RTC (تعریف در main.cpp) — جلوگیری از تداخلِ هم‌زمانِ
// دو هسته (لمس روی هستهٔ ۱ داخلِ lvgl_task، RTC روی هستهٔ ۰) که می‌توانست باس را قفل کند.
extern SemaphoreHandle_t rtcMutex;

#include "agent_hmi.h"

typedef enum {
    BUZZER_ACTION = 1,
} buzzer_event_local_t;

static TFT_eSPI *s_tft = nullptr;
static BBCapTouch *s_touch = nullptr;

static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (!s_tft) {
        lv_display_flush_ready(disp);
        return;
    }

    const uint32_t w = (uint32_t)lv_area_get_width(area);
    const uint32_t h = (uint32_t)lv_area_get_height(area);

    agent_on_flush(area, px_map, lv_display_flush_is_last(disp));

    s_tft->startWrite();
    s_tft->setAddrWindow(area->x1, area->y1, (uint16_t)w, (uint16_t)h);
    s_tft->pushColors((uint16_t *)px_map, w * h, false);
    s_tft->endWrite();

    lv_display_flush_ready(disp);
}

// نوع کنترلر لمسی شناسایی‌شده
typedef enum {
    TOUCH_CHIP_NONE   = 0,
    TOUCH_CHIP_GT911  = 1,
    TOUCH_CHIP_FT6X36 = 2,
    TOUCH_CHIP_CST816 = 3,
} touch_chip_type_t;

static touch_chip_type_t s_touch_chip = TOUCH_CHIP_NONE;
static uint8_t s_touch_i2c_addr = 0x5D;
static uint32_t s_touch_read_count = 0;
static uint32_t s_touch_err_count = 0;

/* ---------- بازیابی ۹ پالس کلاک صنعتی باس I2C (Zero-Freeze Recovery) ---------- */
static void touch_bus_recover(void)
{
    Wire.end();
    pinMode(TOUCH_SCL, OUTPUT_OPEN_DRAIN);
    pinMode(TOUCH_SDA, INPUT_PULLUP);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(5);

    // ۹ کلاک پالس جهت آزاد کردن SDA قفل‌شده توسط اسلیو
    for (int i = 0; i < 9; i++) {
        digitalWrite(TOUCH_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(TOUCH_SCL, HIGH);
        delayMicroseconds(5);
    }

    // ارسال سیگنال STOP
    pinMode(TOUCH_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(TOUCH_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(TOUCH_SDA, HIGH);
    delayMicroseconds(5);

    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(400000);
    Wire.setTimeOut(10);
    Serial.println(F("[I2C] Bus unlock completed (9 SCL clocks)"));
}

static bool touch_i2c_probe_addr(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return (Wire.endTransmission(true) == 0);
}

static bool touch_hardware_probe(bool allow_recover = false)
{
    // ۱. بررسی کنترلر GT911 (آدرس اصلی 0x5D، آدرس ثانویه 0x14)
    if (touch_i2c_probe_addr(0x5D)) {
        s_touch_i2c_addr = 0x5D;
        s_touch_chip = TOUCH_CHIP_GT911;
        Serial.println(F("[TOUCH] GT911 detected @ 0x5D"));
        return true;
    }
    if (touch_i2c_probe_addr(0x14)) {
        s_touch_i2c_addr = 0x14;
        s_touch_chip = TOUCH_CHIP_GT911;
        Serial.println(F("[TOUCH] GT911 detected @ 0x14"));
        return true;
    }

    // ۲. بررسی FocalTech FT6236 / FT6336 @ 0x38
    if (touch_i2c_probe_addr(0x38)) {
        s_touch_i2c_addr = 0x38;
        s_touch_chip = TOUCH_CHIP_FT6X36;
        Serial.println(F("[TOUCH] FT6x36 detected @ 0x38"));
        return true;
    }

    // ۳. بررسی Hynitron CST816 @ 0x15
    if (touch_i2c_probe_addr(0x15)) {
        s_touch_i2c_addr = 0x15;
        s_touch_chip = TOUCH_CHIP_CST816;
        Serial.println(F("[TOUCH] CST816 detected @ 0x15"));
        return true;
    }

    if (allow_recover) {
        // ریکاوری ۹ کلاک فقط در مرحله اول بوت، نه در چرخه تناوبی زمان اجرا
        touch_bus_recover();
        if (touch_i2c_probe_addr(0x5D)) {
            s_touch_i2c_addr = 0x5D;
            s_touch_chip = TOUCH_CHIP_GT911;
            Serial.println(F("[TOUCH] GT911 detected @ 0x5D (post-recovery)"));
            return true;
        }
        if (touch_i2c_probe_addr(0x14)) {
            s_touch_i2c_addr = 0x14;
            s_touch_chip = TOUCH_CHIP_GT911;
            Serial.println(F("[TOUCH] GT911 detected @ 0x14 (post-recovery)"));
            return true;
        }
    }

    s_touch_chip = TOUCH_CHIP_NONE;
    return false;
}

/* ---------- پروتکل مستقیم سخت‌افزاری GT911 با تضمین عدم انجماد و عدم قفل باس ---------- */
static bool gt911_read_point(int16_t *out_x, int16_t *out_y)
{
    // مرحله الف: نوشتن آدرس ثبات وضعیت 0x814E با ارسال STOP برای جلوگیری از ددلاک ESP32-S3
    Wire.beginTransmission(s_touch_i2c_addr);
    Wire.write((uint8_t)0x81);
    Wire.write((uint8_t)0x4E);
    if (Wire.endTransmission(true) != 0) {
        if (++s_touch_err_count >= 10) {
            s_touch_err_count = 0;
        }
        return false;
    }

    // مرحله ب: خواندن ۷ بایت (0x814E..0x8154)
    uint8_t buf[7] = {0};
    uint8_t rlen = Wire.requestFrom((int)s_touch_i2c_addr, 7);
    if (rlen != 7) {
        while (Wire.available()) Wire.read();
        if (++s_touch_err_count >= 10) {
            s_touch_err_count = 0;
        }
        return false;
    }
    for (uint8_t i = 0; i < 7; i++) {
        buf[i] = (uint8_t)Wire.read();
    }
    s_touch_err_count = 0;

    uint8_t status = buf[0];
    if ((status & 0x80) == 0) {
        return false; // بافر هنوز داده جدید آماده نکرده است
    }

    uint8_t points = status & 0x0F;

    // مرحله ج: پاکسازی و تایید حتمی رجیستر وضعیت 0x814E (قانون طلایی ضد فریز GT911)
    // تا زمانی که 0x00 نوشته نشود، کنترلر GT911 لمس بعدی را گزارش نمی‌کند
    Wire.beginTransmission(s_touch_i2c_addr);
    Wire.write((uint8_t)0x81);
    Wire.write((uint8_t)0x4E);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission(true);

    if (points == 0 || points > 5) {
        return false; // رها شدن انگشت (Release frame)
    }

    // مرحله د: استخراج و تبدیل مختصات سخت‌افزاری (Native Portrait -> Landscape 270 Rotation)
    int16_t raw_x = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    int16_t raw_y = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));

    // نگاشت دوران ۲۷۰ درجه ST7796 لنداسکیپ 480x320
    int16_t calc_x = (int16_t)(SCREEN_WIDTH - 1 - raw_y);
    int16_t calc_y = raw_x;

    if (calc_x < 0) calc_x = 0;
    if (calc_x >= SCREEN_WIDTH) calc_x = SCREEN_WIDTH - 1;
    if (calc_y < 0) calc_y = 0;
    if (calc_y >= SCREEN_HEIGHT) calc_y = SCREEN_HEIGHT - 1;

    *out_x = calc_x;
    *out_y = calc_y;
    s_touch_read_count++;
    return true;
}

/* ---------- پروتکل مستقیم FocalTech FT6x36 ---------- */
static bool ft6x36_read_point(int16_t *out_x, int16_t *out_y)
{
    Wire.beginTransmission(s_touch_i2c_addr);
    Wire.write((uint8_t)0x02); // TD_STATUS
    if (Wire.endTransmission(true) != 0) {
        if (++s_touch_err_count >= 10) {
            s_touch_err_count = 0;
        }
        return false;
    }

    uint8_t buf[5] = {0};
    if (Wire.requestFrom((int)s_touch_i2c_addr, 5) != 5) {
        while (Wire.available()) Wire.read();
        if (++s_touch_err_count >= 10) {
            s_touch_err_count = 0;
        }
        return false;
    }
    for (uint8_t i = 0; i < 5; i++) {
        buf[i] = (uint8_t)Wire.read();
    }
    s_touch_err_count = 0;

    uint8_t points = buf[0] & 0x0F;
    if (points == 0 || points > 5) return false;

    uint16_t raw_x = ((buf[1] & 0x0F) << 8) | buf[2];
    uint16_t raw_y = ((buf[3] & 0x0F) << 8) | buf[4];

    // دوران ۲۷۰ درجه
    int16_t calc_x = (int16_t)raw_y;
    int16_t calc_y = (int16_t)(SCREEN_HEIGHT - 1 - raw_x);

    if (calc_x < 0) calc_x = 0;
    if (calc_x >= SCREEN_WIDTH) calc_x = SCREEN_WIDTH - 1;
    if (calc_y < 0) calc_y = 0;
    if (calc_y >= SCREEN_HEIGHT) calc_y = SCREEN_HEIGHT - 1;

    *out_x = calc_x;
    *out_y = calc_y;
    s_touch_read_count++;
    return true;
}

static bool touch_poll_hardware(int16_t *out_x, int16_t *out_y)
{
    if (s_touch_chip == TOUCH_CHIP_NONE) {
        static uint32_t s_last_probe_ms = 0;
        if (millis() - s_last_probe_ms > 2000U) {
            s_last_probe_ms = millis();
            touch_hardware_probe();
        }
        if (s_touch_chip == TOUCH_CHIP_NONE) return false;
    }

    if (s_touch_chip == TOUCH_CHIP_GT911) {
        return gt911_read_point(out_x, out_y);
    } else if (s_touch_chip == TOUCH_CHIP_FT6X36 || s_touch_chip == TOUCH_CHIP_CST816) {
        return ft6x36_read_point(out_x, out_y);
    }
    return false;
}

extern "C" bool display_port_touch_detected(void)
{
    return (s_touch_chip != TOUCH_CHIP_NONE);
}

extern "C" void display_port_touch_info(uint8_t *chip, uint8_t *addr, uint32_t *reads, uint32_t *errs)
{
    if (chip) *chip = (uint8_t)s_touch_chip;
    if (addr) *addr = s_touch_i2c_addr;
    if (reads) *reads = s_touch_read_count;
    if (errs) *errs = s_touch_err_count;
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    static bool was_pressed = false;
    static bool touch_beep_armed = true;
    static int16_t filtered_x = 0;
    static int16_t filtered_y = 0;
    static uint32_t s_last_touch_ms = 0;
    static uint8_t s_spike_count = 0;

    static bool s_agent_was_active = false;
    static int16_t s_agent_last_x = 0;
    static int16_t s_agent_last_y = 0;

    // شبیه‌سازی لمس توسط ابزار تست خودکار HMI Commander
    if (g_agent_touch_active) {
        s_agent_was_active = true;
        s_agent_last_x = g_agent_touch_x;
        s_agent_last_y = g_agent_touch_y;
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = g_agent_touch_x;
        data->point.y = g_agent_touch_y;
        g_last_activity_ms = millis();
        return;
    } else if (s_agent_was_active) {
        s_agent_was_active = false;
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = s_agent_last_x;
        data->point.y = s_agent_last_y;
        g_last_activity_ms = millis();
        return;
    }

    int16_t raw_x = 0;
    int16_t raw_y = 0;
    bool has_point = false;

    // محافظت همزمانی گذرگاه I2C با ساعت DS3231 روی هسته ۰ (حداکثر سقف ۸ میلی‌ثانیه)
    if (rtcMutex) {
        if (xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(8)) == pdTRUE) {
            has_point = touch_poll_hardware(&raw_x, &raw_y);
            xSemaphoreGive(rtcMutex);
        } else if (was_pressed && (millis() - s_last_touch_ms < 16U)) {
            // حفظ موقت وضعیت لمس در صورت اشغال گذرا برای جلوگیری از پرش
            has_point = true;
            raw_x = filtered_x;
            raw_y = filtered_y;
        }
    } else {
        has_point = touch_poll_hardware(&raw_x, &raw_y);
    }

    uint32_t now = millis();

    if (!has_point) {
        // فیلتر تداوم بین‌فریمی GT911 (Inter-Frame Persistence):
        // نرخ تازه‌سازی تاچ ~60-100Hz است. تا ۱۶ میلی‌ثانیه وضعیت لمس فشرده نگه‌داشته می‌شود
        // تا دراگ/اسکرول و نگه‌داشتن کلید پله‌پله یا منقطع نشود
        if (was_pressed && (uint32_t)(now - s_last_touch_ms) < 16U) {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = filtered_x;
            data->point.y = filtered_y;
            g_last_activity_ms = now;
            return;
        }

        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = filtered_x;
        data->point.y = filtered_y;
        was_pressed = false;
        touch_beep_armed = true;
        s_spike_count = 0;
        return;
    }

    s_last_touch_ms = now;
    g_last_activity_ms = now;

    if (!was_pressed) {
        // ۱. لمس اولیه: پاسخ آنی و بی‌درنگ (Zero-Lag First Tap)
        filtered_x = raw_x;
        filtered_y = raw_y;
        was_pressed = true;
        panel_nav_touch_exit = true;
        s_spike_count = 0;

        if (touch_beep_armed && get_var_setting_general_speaker() && !g_setup_overlay_active) {
            if (buzzerTaskHandle) {
                xTaskNotify(buzzerTaskHandle, (uint32_t)BUZZER_ACTION, eSetValueWithOverwrite);
            }
            touch_beep_armed = false;
        }
    } else {
        // ۲. فیلتر ۳ مرحله‌ای DSP صنعتی هافلر:
        int dx = raw_x - (int)filtered_x;
        int dy = raw_y - (int)filtered_y;
        int abs_dx = abs(dx);
        int abs_dy = abs(dy);
        int dist_sq = dx * dx + dy * dy;

        // مرحله ۱: مهار اسپایک‌های نویز شدید EMI رله‌ها و کنتاکتورها (>120 پیکسل در یک فریم)
        if (abs_dx > 120 || abs_dy > 120) {
            s_spike_count++;
            if (s_spike_count < 2) {
                // نادیده گرفتن جهش نویز تک‌فریم و حفظ مختصات معتبر قبلی
                raw_x = filtered_x;
                raw_y = filtered_y;
            } else {
                // حرکت سریع واقعی دست برای ۲ فریم متوالی
                filtered_x = raw_x;
                filtered_y = raw_y;
                s_spike_count = 0;
            }
        } else {
            s_spike_count = 0;

            // مرحله ۲: باند مرده / هیسترزیس (Deadband ±2px) جهت حذف کامل لرزش دستکش یا دست مرطوب
            if (abs_dx <= 2 && abs_dy <= 2) {
                // موقعیت بدون تغییر حفظ می‌شود تا دکمه‌ها و اسلایدرها لرزش نداشته باشند
            }
            // مرحله ۳: هموارسازی تطبیقی دینامیک (Dynamic Adaptive EMA)
            else if (dist_sq < (24 * 24)) {
                // جابجایی ریز: فیلتر ۵۰٪ برای حرکت فوق‌العاده نرم و خطی
                filtered_x = (int16_t)(((int)filtered_x + raw_x + 1) / 2);
                filtered_y = (int16_t)(((int)filtered_y + raw_y + 1) / 2);
            } else {
                // سوایپ و حرکت تند: فیلتر سبک ۷۵٪ برای پاسخ بلادرنگ بدون رد پا و لگ
                filtered_x = (int16_t)(((int)filtered_x * 1 + raw_x * 3 + 2) / 4);
                filtered_y = (int16_t)(((int)filtered_y * 1 + raw_y * 3 + 2) / 4);
            }
        }
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = filtered_x;
    data->point.y = filtered_y;
}

void display_port_set_dma_enabled(bool enabled)
{
    // عمداً بدون اثر: روی این برد DMA صفحه را سیاه/خراب کرده (درس Config/conging).
    (void)enabled;
}

bool display_port_init(TFT_eSPI *tft, BBCapTouch *touch)
{
    if (!tft) {
        return false;
    }

    s_tft = tft;
    s_touch = touch;

    s_tft->setSwapBytes(false);

    // راه‌اندازی و شناسایی کنترلر لمسی سخت‌افزاری
    touch_hardware_probe(true);

    // بافر رندر در RAM داخلی DMA-capable:
    // ۳۲ خط = ۳۰.۷KB برای هر بافر (۱۰ فلش متقارن به جای ۱۴ فلش نامتقارن ۲۴ خط)،
    // با حفظ حاشیه امن SRAM داخلی زیر ۴۵٪ طبق الزامات AGENTS.md
    static const uint32_t try_lines[] = {32, 24, 16, 12};
    uint32_t lines_used = 0;
    size_t buf_bytes = 0;
    const char *buf_kind = "internal_DMA";

    for (uint32_t i = 0; i < sizeof(try_lines) / sizeof(try_lines[0]); i++) {
        const uint32_t lines = try_lines[i];
        const size_t bytes = (size_t)SCREEN_WIDTH * lines * sizeof(lv_color_t);
        s_buf1 = (lv_color_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        s_buf2 = (lv_color_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (s_buf1 && s_buf2) {
            lines_used = lines;
            buf_bytes = bytes;
            break;
        }
        if (s_buf1) { heap_caps_free(s_buf1); s_buf1 = nullptr; }
        if (s_buf2) { heap_caps_free(s_buf2); s_buf2 = nullptr; }
    }

    // Fallback به PSRAM فقط اگر RAM داخلی به اندازه کافی خالی نبود
    if (!s_buf1 || !s_buf2) {
        static const uint32_t psram_try[] = {32, 24, 16};
        for (uint32_t i = 0; i < sizeof(psram_try) / sizeof(psram_try[0]); i++) {
            const uint32_t lines = psram_try[i];
            const size_t bytes = (size_t)SCREEN_WIDTH * lines * sizeof(lv_color_t);
            s_buf1 = (lv_color_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            s_buf2 = (lv_color_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (s_buf1 && s_buf2) {
                lines_used = lines;
                buf_bytes = bytes;
                buf_kind = "PSRAM";
                break;
            }
            if (s_buf1) { heap_caps_free(s_buf1); s_buf1 = nullptr; }
            if (s_buf2) { heap_caps_free(s_buf2); s_buf2 = nullptr; }
        }
    }
    if (!s_buf1 || !s_buf2) {
        const size_t bytes = (size_t)SCREEN_WIDTH * 10 * sizeof(lv_color_t);
        s_buf1 = (lv_color_t *)malloc(bytes);
        if (s_buf1) {
            lines_used = 10;
            buf_bytes = bytes;
            buf_kind = "heap-fallback";
            Serial.println("LVGL: heap fallback buffer");
        }
    } else {
        Serial.printf("LVGL: Draw buffers OK: %u lines, %u bytes each (%s)\n",
                      (unsigned)lines_used, (unsigned)buf_bytes, buf_kind);
    }

    lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!disp) {
        return false;
    }

    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, s_buf1, s_buf2,
                           (uint32_t)buf_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    lv_indev_set_display(indev, disp);
    lv_indev_set_scroll_limit(indev, 8);
    lv_indev_set_scroll_throw(indev, 20);

    lv_timer_t *rt = lv_indev_get_read_timer(indev);
    if (rt) {
        lv_timer_set_period(rt, 10);
    }

    return true;
}

extern "C" void *display_port_allocate_card_buffer(size_t nbytes)
{
    void *out = heap_caps_malloc(nbytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!out) {
        out = heap_caps_malloc(nbytes, MALLOC_CAP_8BIT);
    }
    return out;
}

extern "C" void display_port_free_card_buffer(void *ptr)
{
    if (ptr) {
        heap_caps_free(ptr);
    }
}
