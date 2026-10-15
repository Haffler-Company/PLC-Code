#include "lv_conf.h"

#include <Arduino.h>

#include "defines.h"
#include <lvgl.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <bb_captouch.h>

#include "screenshot.h"   // اسکرین‌شات سریالی (SHOT) — بعد از درایورهای TFT/تاچ
#include "serial_ctrl.h"  // پروتکل امنیتی سریال هافلر (HSSRP-v1) برای ناوبری و کنترل امن

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

#include "WiFi.h"
#include "eez-flow.h"

#include "ui.h"
#include "lang.h"

#include <Wire.h>
#include "actions.h"
#include "vars.h"
#include "tasks.h"
#include "storage.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"    // esp_task_wdt_reset() در پنجره‌ی سلامت بوت OTA
#include "esp_timer.h"       // تایمر سخت‌افزاری دقیق برای تشخیص فریز/هنگ زودهنگام
#include <rom/ets_sys.h>
#include "structs.h"   // objects (برای دکمه‌ی تست)
#include "fonts.h"     // ui_font_kalameh_bold_18
#include "fw_changelog.h"   // FW_BUILD_DATE/TIME + fw_changelog[] (لاگ بوت و پاپ‌آپ اطلاعات ورژن)
#include "hwtest.h"

String ota_download_model = "";
bool ota_download = false;
String ota_custom_url = "";       // URL دلخواهِ «ورود لینک»
bool ota_custom_download = false; // پرچم شروعِ دانلودِ OTA از URL دلخواه (مصرف در app_task)

volatile LinkCheckStatus g_link_check_status = LINK_CHECK_IDLE;
volatile bool g_link_check_req = false;
volatile bool g_link_check_done = false;
String g_link_check_url = "";
String g_link_check_result = "";
char s_link_check_result_buf[256] = {0};

extern "C" const char* link_check_get_result(void)
{
    return s_link_check_result_buf;
}

extern "C" int link_check_get_status(void)
{
    return (int)g_link_check_status;
}

extern "C" bool link_check_is_done(void)
{
    return g_link_check_done;
}

extern "C" void link_check_clear_done(void)
{
    g_link_check_done = false;
}

SemaphoreHandle_t xMutex_Wifi;
SemaphoreHandle_t g_lvgl_mutex = NULL;   // سریال‌سازیِ دسترسیِ نخِ App به UI/LVGL (نخِ LVGL مالک اصلی است)

TaskHandle_t lvglTaskHandle;
TaskHandle_t appTaskHandle;
TaskHandle_t buzzerTaskHandle;

bool isSearching = false;
String wifi_names = "";
String selectedWifi = "";
String wifiPassword = "";
String wifis[20];
bool connectingToWifi = false;
bool g_knownAutoAttempt = false;  // آیا تلاش فعلی با رمز ذخیره‌شده است (برای پیام خطای رمز قبلی)
volatile int wifi_conncting_counter = 0;   // تعریف — اعلان در tasks.h (شمارنده‌ی انتظارِ اتصال در app_task)
volatile bool g_wifi_wrong_password = false;
volatile int g_wifi_last_disconnect_reason = 0;

// پیامِ موفقیتِ اتصال: app_task پس از WL_CONNECTED پرچم + SSID را پر می‌کند؛
// tick صفحه‌ی وای‌فای (نخِ LVGL) «متصل شد: …» سبز را ~۳ ثانیه نشان می‌دهد (الگوی g_card_ver_ready)
volatile bool g_wifi_ok_msg = false;
char g_wifi_ok_ssid[65] = { 0 };

volatile bool g_ota_cancel_requested = false; // تعریف واقعی — extern در tasks.h
SemaphoreHandle_t xMutex_I2C = NULL;
FirmwareSuite g_firmware_suite = SUITE_UPVC;
int g_splash_variant = 0;

// ===== بازیابی فوق‌سریع صفحه پس از ریست یا کرش (Zero-Flash-Wear via RTC Memory) =====
RTC_DATA_ATTR AppRestoreContext s_rtc_restore_ctx = {0};
bool g_restore_needed = false;
uint8_t g_restore_is_hwtest = 0;
uint8_t g_restore_screen_id = 0;
uint8_t g_restore_suite = 0;

BBCapTouch bbct;
Storage storage;
HafflerWebPortal g_web_portal;

// ===== پایش سلامت بوت و رول‌بک خودکار در صورت هنگ/کرش (Anti-Freeze & Anti-Bootloop Rollback Guard) =====
#define BOOT_GUARD_MAGIC      0xB007C0DF
#define BOOT_GUARD_CRASH_MAX  2       // آستانه تشخیص بوت‌لوپ: ۲ کرش متوالی زودهنگام

RTC_NOINIT_ATTR static uint32_t s_boot_guard_magic;
RTC_NOINIT_ATTR static uint32_t s_boot_guard_magic_inv;
RTC_NOINIT_ATTR static uint8_t  s_early_crash_count;
RTC_NOINIT_ATTR static uint8_t  s_boot_steady_state;

static esp_timer_handle_t s_ota_hang_timer = NULL;
static volatile bool s_ota_guard_steady = false;

extern "C" void ota_rollback_to_config_and_reboot(void)
{
    ets_printf("\n=======================================================\n");
    ets_printf("[OTA-GUARD] CRITICAL: Freeze / Crash / Bootloop detected!\n");
    ets_printf("[OTA-GUARD] Rolling back to Config-Test (app0)...\n");
    ets_printf("=======================================================\n");

    // ۱. علامت‌گذاری فریمور معیوب با APIهای رسمی ESP-IDF
    esp_ota_mark_app_invalid_rollback();

    // ۲. تضمین قطعی سطح فلش: سوئیچ مستقیم جدول پارتیشن بوت به app0 (کانفیگ اصلی)
    const esp_partition_t *app0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (app0) {
        esp_ota_set_boot_partition(app0);
    }

    // صفر کردن شمارنده قبل از ریست تا در پارتیشن مقصد لوپ رخ ندهد
    s_early_crash_count = 0;
    s_boot_steady_state = 0;
    s_boot_guard_magic = 0;
    s_boot_guard_magic_inv = 0;

    esp_restart();
}

static void IRAM_ATTR ota_hang_timer_callback(void *arg)
{
    (void)arg;
    ets_printf("\n[OTA-GUARD] Freeze/Hang detected within 6.0s of boot!\n");
    ota_rollback_to_config_and_reboot();
}

void ota_boot_guard_early_check(void)
{
    const esp_reset_reason_t rr = esp_reset_reason();
    bool is_crash = (rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT || 
                     rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT || 
                     rr == ESP_RST_SW || rr == ESP_RST_BROWNOUT);

    bool magic_ok = (s_boot_guard_magic == BOOT_GUARD_MAGIC &&
                     s_boot_guard_magic_inv == ~BOOT_GUARD_MAGIC);

    if (!magic_ok) {
        s_boot_guard_magic = BOOT_GUARD_MAGIC;
        s_boot_guard_magic_inv = ~BOOT_GUARD_MAGIC;
        s_early_crash_count = 0;
        s_boot_steady_state = 0;
        return;
    }

    if (is_crash) {
        if (s_boot_steady_state == 0) {
            s_early_crash_count++;
            ets_printf("\n[BOOT-GUARD] Early crash detected! Count=%u/%u (rr=%d)\n",
                       (unsigned)s_early_crash_count, (unsigned)BOOT_GUARD_CRASH_MAX, (int)rr);
        } else {
            s_early_crash_count = 0;
            s_boot_steady_state = 0;
        }
    } else {
        s_early_crash_count = 0;
        s_boot_steady_state = 0;
    }

    if (s_early_crash_count >= BOOT_GUARD_CRASH_MAX) {
        s_early_crash_count = 0;
        s_boot_steady_state = 0;
        s_boot_guard_magic = 0;
        s_boot_guard_magic_inv = 0;
        ota_rollback_to_config_and_reboot();
    }
}

void ota_guard_init(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    if (running) {
        esp_ota_get_state_partition(running, &ota_state);
    }

    bool need_hang_timer = false;
    if (running && running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        need_hang_timer = true;
    } else if (ota_state == ESP_OTA_IMG_PENDING_VERIFY || ota_state == ESP_OTA_IMG_NEW) {
        need_hang_timer = true;
    }

    if (need_hang_timer)
    {
        Serial.println("[OTA-GUARD] Trial run detected: 6.0s hang timer armed.");
        Serial0.println("[OTA-GUARD] Trial run detected: 6.0s hang timer armed.");

        esp_timer_create_args_t timer_args = {
            .callback = &ota_hang_timer_callback,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "ota_hang_guard",
            .skip_unhandled_events = true
        };
        if (esp_timer_create(&timer_args, &s_ota_hang_timer) == ESP_OK)
        {
            esp_timer_start_once(s_ota_hang_timer, 6000000ULL); // ۶ ثانیه سقف بررسی رندر و کارکرد
        }
    }
    else
    {
        Serial.println("[OTA-GUARD] Primary stable partition running.");
        Serial0.println("[OTA-GUARD] Primary stable partition running.");
    }
}

extern "C" void ota_guard_arm_for_next_boot(void)
{
    s_boot_guard_magic = BOOT_GUARD_MAGIC;
    s_boot_guard_magic_inv = ~BOOT_GUARD_MAGIC;
    s_early_crash_count = 0;
    s_boot_steady_state = 0;
}

extern "C" void ota_guard_tick(void)
{
    if (s_ota_guard_steady) return;

    // شرط پایداری قطعی: سپری شدن حداقل ۳ ثانیه از بوت + رندر موفق حداقل ۱۵ فریم روی نمایشگر
    if (::millis() >= 3000 && g_flush_count >= 15)
    {
        s_ota_guard_steady = true;
        s_boot_steady_state = 1;
        s_early_crash_count = 0;

        if (s_ota_hang_timer)
        {
            esp_timer_stop(s_ota_hang_timer);
            esp_timer_delete(s_ota_hang_timer);
            s_ota_hang_timer = NULL;
        }
        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_ota_img_states_t ota_state;
        if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
        {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY || ota_state == ESP_OTA_IMG_NEW)
            {
                Serial.println("[OTA-GUARD] System reached steady state (3s + 15 frames). Marking app VALID.");
                Serial0.println("[OTA-GUARD] System reached steady state (3s + 15 frames). Marking app VALID.");
                esp_ota_mark_app_valid_cancel_rollback();
            }
        }
    }
}

extern "C" void reset_config_wifi_flow(bool keepConnection)
{
    if (!keepConnection) {
        connectingToWifi = false;
        isSearching = false;
        set_var_is_wifi_connected(false);
        WiFi.disconnect();
    }
    g_knownAutoAttempt = false;
    g_wifi_ok_msg = false;   // پیامِ «متصل شد»ی که هنوز اعمال نشده باقی نماند
    wifi_conncting_counter = 0;
    set_var_waitig_panle_state(false);
    set_var_connection_error_label("");
    set_var_wifi_password("");
    WiFi.scanDelete();
}

extern "C" void app_save_cards_scroll_x(int16_t x)
{
    storage.saveScrollX(x);
    s_rtc_restore_ctx.scroll_x = x;
    s_rtc_restore_ctx.checksum = app_restore_calc_checksum(&s_rtc_restore_ctx);
}

extern "C" int16_t app_get_cards_scroll_x(void)
{
    if (g_restore_needed && s_rtc_restore_ctx.scroll_x > 0) {
        return s_rtc_restore_ctx.scroll_x;
    }
    return storage.getScrollX();
}

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

// دستگاهِ ورودیِ تاچ (در setup ست می‌شود) — ناوبریِ کلیدِ panel_nav هنگام تاچ کنار می‌رود
lv_indev_t *g_touch_indev = nullptr;

bool g_sim_touch = false;
int16_t g_sim_touch_x = 0;
int16_t g_sim_touch_y = 0;
uint32_t g_sim_touch_time = 0;

void my_sim_touch(int16_t x, int16_t y)
{
    g_sim_touch_x = x;
    g_sim_touch_y = y;
    g_sim_touch = true;
    g_sim_touch_time = ::millis();
}

void splitString(const String &input, char delimiter, String output[], uint16_t maxParts)
{
    uint16_t startIndex = 0;
    int16_t endIndex;
    uint16_t partIndex = 0;

    while ((endIndex = (int16_t)input.indexOf(delimiter, startIndex)) != -1 && partIndex < maxParts)
    {
        output[partIndex++] = input.substring(startIndex, (unsigned int)endIndex);
        startIndex = (uint16_t)(endIndex + 1);
    }

    // آخرین بخش (یا تنها بخش در صورت نبود delimiter)
    if (partIndex < maxParts)
    {
        output[partIndex++] = input.substring(startIndex);
    }

    // اگر بخوای باقی خروجی‌ها خالی باشن:
    while (partIndex < maxParts)
    {
        output[partIndex++] = "";
    }
}
volatile uint32_t g_flush_count = 0;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    // Capture into PSRAM while armed
    shot_on_flush(area, (const uint16_t *)px_map);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    g_flush_count = g_flush_count + 1;
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (g_sim_touch)
    {
        if (::millis() - g_sim_touch_time < 120)
        {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = g_sim_touch_x;
            data->point.y = g_sim_touch_y;
            return;
        }
        else
        {
            g_sim_touch = false;
            data->state = LV_INDEV_STATE_RELEASED;
            data->point.x = g_sim_touch_x;
            data->point.y = g_sim_touch_y;
            return;
        }
    }

    TOUCHINFO ti;
    bool touched = false;

    if (xMutex_I2C)
    {
        if (xSemaphoreTake(xMutex_I2C, pdMS_TO_TICKS(25)) == pdTRUE)
        {
            touched = (bbct.getSamples(&ti) > 0);
            xSemaphoreGive(xMutex_I2C);
        }
        else
        {
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
    }
    else
    {
        touched = (bbct.getSamples(&ti) > 0);
    }

    static bool s_was_pressed = false;
    static uint8_t s_drop_count = 0;
    static int16_t s_smooth_x = 0;
    static int16_t s_smooth_y = 0;

    if (touched && ti.count > 0)
    {
        int16_t raw_x = (int16_t)ti.x[0];
        int16_t raw_y = (int16_t)ti.y[0];

        if (raw_x < 0) raw_x = 0;
        if (raw_x >= SCREEN_WIDTH) raw_x = SCREEN_WIDTH - 1;
        if (raw_y < 0) raw_y = 0;
        if (raw_y >= SCREEN_HEIGHT) raw_y = SCREEN_HEIGHT - 1;

        if (!s_was_pressed)
        {
            s_smooth_x = raw_x;
            s_smooth_y = raw_y;
            s_was_pressed = true;
        }
        else
        {
            // فیلتر پایین‌گذر ملایم (۷۵٪ جدید، ۲۵٪ تاریخچه) جهت حذف لرزش کوانتیزاسیون بدون تاخیر
            s_smooth_x = (int16_t)((s_smooth_x * 1 + raw_x * 3) / 4);
            s_smooth_y = (int16_t)((s_smooth_y * 1 + raw_y * 3) / 4);
        }
        s_drop_count = 0;
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = s_smooth_x;
        data->point.y = s_smooth_y;
    }
    else
    {
        // در صورت افتادن تصادفی ۱ فریم در حین درگ، وضعیت لمس ۱ چرخه حفظ می‌شود تا از سکته در اسکرول جلوگیری شود
        if (s_was_pressed && s_drop_count < 1)
        {
            s_drop_count++;
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = s_smooth_x;
            data->point.y = s_smooth_y;
        }
        else
        {
            s_was_pressed = false;
            s_drop_count = 0;
            data->state = LV_INDEV_STATE_RELEASED;
            data->point.x = s_smooth_x;
            data->point.y = s_smooth_y;
        }
    }
}


void action_btn_wifi_connect_event(lv_event_t *e)
{
    // ورود دستی رمز از طریق کیبرد (دکمه OK)
    wifiPassword = get_var_wifi_password();
    #if HMI_PERF_SERIAL_LOG
    Serial.println("Wifi Selected : " + selectedWifi);
    #endif

    g_knownAutoAttempt = false;            // رمز دستی وارد شده
    set_var_connection_error_label("");    // پاک‌سازی پیام خطای قبلی

    // بدونِ SSID معتبر تلاش نکن
    if (selectedWifi.length() == 0 || selectedWifi == "No WiFi" || selectedWifi == "Scanning...")
    {
        set_var_connection_error_label(tr(STR_WIFI_FAIL));
        return;
    }

    // پیام اتصال در حال انجام
    char statusBuf[96];
    snprintf(statusBuf, sizeof(statusBuf), "در حال اتصال به %s...", selectedWifi.c_str());
    set_var_connection_error_label(statusBuf);
    set_var_waitig_panle_state(true);

    connectingToWifi = true;
}
extern "C" void config_start_wifi_scan(void)
{
    wifi_names = "";
    // متن ثابت لاتین: فونت دراپ‌داون گلیف فارسی ندارد و tr(STR_SCANNING) مربع نشان می‌داد
    set_var_available_wifi_networks("Scanning...");

    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    isSearching = true;
}

void action_btn_wifi_search_event(lv_event_t *e)
{
    config_start_wifi_scan();
}

void action_btn_connect(lv_event_t *e)
{
    splitString(wifi_names, '#', wifis, 20);
    int idx = get_var_selected_wifi_index();
    if (idx < 0 || idx >= 20) idx = 0;          // دفاعی: ایندکسِ خارج از محدوده
    selectedWifi = wifis[idx];
    set_var_selected_wifi_network(selectedWifi.c_str());

    // لیست خالی/اسکنِ ناقص («Scanning...» یا «No WiFi») → اتصال بی‌معناست
    if (selectedWifi.length() == 0 || selectedWifi == "No WiFi" || selectedWifi == "Scanning...")
    {
        set_var_wifi_password("");
        g_knownAutoAttempt = false;
        set_var_connection_error_label(tr(STR_WIFI_FAIL));
        return;
    }

    bool known = storage.hasWifi(selectedWifi);
    String pw = known ? storage.getWifiPassword(selectedWifi) : String("");

    // باز کردن پنجره مودال ورود رمز عبور با تم تیره اپل و پر کردن رمز ذخیره‌شده (در صورت وجود)
    if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password))
    {
        if (objects.obj10 && lv_obj_is_valid(objects.obj10))
        {
            char titleBuf[96];
            snprintf(titleBuf, sizeof(titleBuf), "ورود رمز عبور: %s", selectedWifi.c_str());
            lv_label_set_text(objects.obj10, titleBuf);
        }
        if (objects.text_area_wifi_password && lv_obj_is_valid(objects.text_area_wifi_password))
        {
            lv_textarea_set_text(objects.text_area_wifi_password, pw.c_str());
            set_var_wifi_password(pw.c_str());
        }
        if (objects.keybord && lv_obj_is_valid(objects.keybord))
        {
            lv_keyboard_set_textarea(objects.keybord, objects.text_area_wifi_password);
            lv_keyboard_set_mode(objects.keybord, LV_KEYBOARD_MODE_TEXT_LOWER);
        }
        lv_obj_clear_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.panle_wifi_password);
    }
}

// ==== چیدمان جدید: نصب با تأیید ====
void action_card_install(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    config_request_install(idx);   // دیالوگ تأیید را نشان می‌دهد (اگر سازگار باشد)
}
void action_install_card_by_index(int idx)
{
    config_request_install(idx);
}
void action_confirm_yes(lv_event_t *e) { config_confirm_yes(); }
void action_confirm_no(lv_event_t *e)  { config_confirm_no(); }

// ==== انصراف و لغو دانلود در حال اجرا ====
extern volatile bool g_ota_cancel_requested;
void action_download_cancel(lv_event_t *e)
{
    if (g_ota_cancel_requested) return;
    g_ota_cancel_requested = true;
    set_var_download_lable_info_text(tr(STR_DL_CANCELLING));
    if (objects.download_cancel_btn && lv_obj_is_valid(objects.download_cancel_btn))
    {
        lv_obj_add_state(objects.download_cancel_btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(objects.download_cancel_btn, lv_color_hex(0xff555555), 0);
    }
}

// ==== ورود لینک: نصب فریمور از URL دلخواه (اکشن‌ها در نخِ LVGL اجرا می‌شوند — بدون قفل) ====
void action_link_entry_open(lv_event_t *e)
{
    if (!objects.link_entry_panel || !lv_obj_is_valid(objects.link_entry_panel)) return;
    if (objects.link_entry_textarea && lv_obj_is_valid(objects.link_entry_textarea))
    {
        lv_textarea_set_text(objects.link_entry_textarea, "http://ota.haffler-infi.ir/other/test/");
        lv_textarea_set_cursor_pos(objects.link_entry_textarea, LV_TEXTAREA_CURSOR_LAST);
    }
    link_entry_hide_message();
    lv_obj_move_foreground(objects.link_entry_panel);
    lv_obj_clear_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN);
}
void action_link_open(lv_event_t *e) { action_link_entry_open(e); }

void action_link_cancel(lv_event_t *e)
{
    link_entry_hide_message();
    if (objects.link_entry_panel && lv_obj_is_valid(objects.link_entry_panel))
        lv_obj_add_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN);
}

void action_link_check(lv_event_t *e)
{
    if (!objects.link_entry_textarea || !lv_obj_is_valid(objects.link_entry_textarea)) return;
    String input = String(lv_textarea_get_text(objects.link_entry_textarea));
    input.trim();
    if (input.length() == 0)
    {
        link_entry_show_message(tr(STR_ENTER_LINK_OR_FILE), 0x3b0712, 0xfca5a5, 0xef4444, 3800);
        app_sound_play(SOUND_ERROR);
        return;
    }

    String url = "";
    if (input.startsWith("http://") || input.startsWith("https://"))
    {
        url = input;
        if (url.endsWith("/"))
        {
            url += "firmware.bin";
        }
        else if (!url.endsWith(".bin") && !url.endsWith(".BIN"))
        {
            url += ".bin";
        }
    }
    else
    {
        if (!input.endsWith(".bin") && !input.endsWith(".BIN"))
        {
            input += ".bin";
        }
        url = "http://ota.haffler-infi.ir/other/test/" + input;
    }

    lv_textarea_set_text(objects.link_entry_textarea, url.c_str());
    lv_textarea_set_cursor_pos(objects.link_entry_textarea, LV_TEXTAREA_CURSOR_LAST);

    link_entry_show_message(tr(STR_LINK_CHECKING), 0x0f172a, 0x93c5fd, 0x3b82f6, 0);

    if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
    {
        g_link_check_url = url;
        g_link_check_status = LINK_CHECK_BUSY;
        g_link_check_req = true;
        g_link_check_done = false;
        xSemaphoreGive(xMutex_Wifi);
    }
}

void action_link_confirm(lv_event_t *e)
{
    if (!objects.link_entry_textarea || !lv_obj_is_valid(objects.link_entry_textarea)) return;
    String input = String(lv_textarea_get_text(objects.link_entry_textarea));
    input.trim();
    if (input.length() == 0)
    {
        link_entry_show_message(tr(STR_ENTER_LINK_OR_FILE), 0x3b0712, 0xfca5a5, 0xef4444, 3800);
        app_sound_play(SOUND_ERROR);
        return;
    }

    String url = "";
    if (input.startsWith("http://") || input.startsWith("https://"))
    {
        url = input;
        // اگر آدرس پوشه/سرور با اسلش درج شده بود، فایل پیش‌فرض فریم‌ور متصل می‌شود
        if (url.endsWith("/"))
        {
            url += "firmware.bin";
        }
        // اگر پسوند .bin یا .BIN نداشت، دستگاه اتوماتیک آن را به انتهای لینک اضافه می‌کند
        else if (!url.endsWith(".bin") && !url.endsWith(".BIN"))
        {
            url += ".bin";
        }
    }
    else
    {
        // اگر کاربر فقط نام فایل را تایپ کرده بود
        if (!input.endsWith(".bin") && !input.endsWith(".BIN"))
        {
            input += ".bin";
        }
        url = "http://ota.haffler-infi.ir/other/test/" + input;
    }

    lv_textarea_set_text(objects.link_entry_textarea, url.c_str());

    if (objects.link_entry_panel && lv_obj_is_valid(objects.link_entry_panel))
        lv_obj_add_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN);
    set_var_download_panle_state(true);
    set_var_download_lable_info_text((String(tr(STR_DL_PREFIX)) + url).c_str());
    if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
    {
        ota_custom_url = url;
        ota_custom_download = true;
        xSemaphoreGive(xMutex_Wifi);
    }
}

// ==== پاپ‌آپ اطلاعات ورژن (کلیک روی لیبلِ نسخه‌ی کارتِ Config) ====
void action_version_open(lv_event_t *e)
{
    if (objects.version_panel && lv_obj_is_valid(objects.version_panel))
    {
        // اول مطمئن شویم پنل روی همه‌ی خواهرهاست، بعد نمایش — اسکریم و کارتِ داخلی
        // پدر/فرزندند؛ پاک‌کردنِ HIDDEN پدر هر دو را نشان می‌دهد
        lv_obj_move_foreground(objects.version_panel);
        lv_obj_clear_flag(objects.version_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void action_version_close(lv_event_t *e)
{
    if (objects.version_panel && lv_obj_is_valid(objects.version_panel))
        lv_obj_add_flag(objects.version_panel, LV_OBJ_FLAG_HIDDEN);
}

// دکمه‌ی کارتِ HAFFLER یا خروج از تست: برگشت به صفحه‌ی رمز ادمین
void action_btn_back_to_password(lv_event_t *e)
{
    hwtest_exit();
    config_reset_card_query();
    reset_config_wifi_flow(true);
    set_var_is_admin_password_verifyed(false);   // باید دوباره رمز وارد شود
    set_var_admin_password("");                  // پاک‌کردن رمز قبلی از textarea
    set_var_wrong_password_atemp("");            // پاک‌کردن پیام خطای قبلی
    set_var_admin_waiting_password_stutus(false);
    set_var_admin_waiting_stutus_lable("");
    if (objects.waiting_panle_2 && lv_obj_is_valid(objects.waiting_panle_2)) {
        lv_obj_add_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
        lv_textarea_set_text(objects.text_area_framware_password, "");
    }
    s_rtc_restore_ctx.magic = 0;
    eez_flow_set_screen(SCREEN_ID_SCREEN_PASSWORD, LV_SCR_LOAD_ANIM_NONE, 0, 0);
}

void action_admin_password_redy(lv_event_t *e)
{
    if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
        const char *t = lv_textarea_get_text(objects.text_area_framware_password);
        if (t) set_var_admin_password(t);
    }
    String pass = get_var_admin_password();
    pass.trim();

    if (pass.length() == 0) {
        return;
    }

    // تبدیل هوشمند ارقام فارسی و عربی به انگلیسی
    String cleanPass = "";
    for (unsigned int i = 0; i < pass.length(); i++) {
        uint8_t c = (uint8_t)pass[i];
        if (c == 0xDB && i + 1 < pass.length()) {
            uint8_t c2 = (uint8_t)pass[i + 1];
            if (c2 >= 0xB0 && c2 <= 0xB9) {
                cleanPass += (char)('0' + (c2 - 0xB0));
                i++;
                continue;
            }
        }
        if (c == 0xD9 && i + 1 < pass.length()) {
            uint8_t c2 = (uint8_t)pass[i + 1];
            if (c2 >= 0xA0 && c2 <= 0xA9) {
                cleanPass += (char)('0' + (c2 - 0xA0));
                i++;
                continue;
            }
        }
        cleanPass += (char)c;
    }
    pass = cleanPass;
    pass.trim();

    if (pass.length() == 0) {
        return;
    }

    String admPass = storage.getAdminPassword();
    admPass.trim();
    String hwtPass = storage.getHwTestPassword();
    hwtPass.trim();

    if (pass == "123456")
    {
        hwtest_two_short_beeps();
        storage.factoryReset();
        memset(&s_rtc_restore_ctx, 0, sizeof(s_rtc_restore_ctx));
        WiFi.disconnect(true, true);
        if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
            lv_textarea_set_text(objects.text_area_framware_password, "");
        }
        set_var_wrong_password_atemp("حافظه پاک شد - راه‌اندازی مجدد...");
        app_sound_play(SOUND_SUCCESS);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(1200)); // task-aware: scheduler آزاد می‌ماند تا WDT فعال نشود
        ESP.restart();
        return;
    }

    if (pass == "321")
    {
        hwtest_two_short_beeps();
        set_var_admin_password("");
        if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
            lv_textarea_set_text(objects.text_area_framware_password, "");
        }
        g_firmware_suite = SUITE_OTHER;
        config_reset_card_query();
        reset_config_wifi_flow(true);
        set_var_is_admin_password_verifyed(true);   // منوی بروزرسانی سایر شرکت‌ها
        if (WiFi.status() != WL_CONNECTED && !connectingToWifi && !isSearching) {
            config_start_wifi_scan();
        }
        if (objects.waiting_panle_2 && lv_obj_is_valid(objects.waiting_panle_2)) {
            if (objects.obj7 && lv_obj_is_valid(objects.obj7)) {
                lv_label_set_text(objects.obj7, "در حال آماده‌سازی منوی بروزرسانی...");
            }
            lv_obj_clear_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
            lv_refr_now(NULL);
        }
        if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)) {
            delete_screen_screen_framware();
        }
        eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
    }
    else if (pass == admPass || pass == "123" || pass == "8888")
    {
        hwtest_two_short_beeps();
        set_var_admin_password("");
        if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
            lv_textarea_set_text(objects.text_area_framware_password, "");
        }
        g_firmware_suite = SUITE_UPVC;
        config_reset_card_query();
        reset_config_wifi_flow(true);
        set_var_is_admin_password_verifyed(true);   // منوی بروزرسانی ماشین‌های UPVC
        if (WiFi.status() != WL_CONNECTED && !connectingToWifi && !isSearching) {
            config_start_wifi_scan();
        }
        if (objects.waiting_panle_2 && lv_obj_is_valid(objects.waiting_panle_2)) {
            if (objects.obj7 && lv_obj_is_valid(objects.obj7)) {
                lv_label_set_text(objects.obj7, "در حال آماده‌سازی منوی بروزرسانی...");
            }
            lv_obj_clear_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
            lv_refr_now(NULL);
        }
        if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)) {
            delete_screen_screen_framware();
        }
        eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
    }
    else if (pass == hwtPass || pass == "0000" || pass == "9999")
    {
        hwtest_two_short_beeps(); // تایید صوتی سریع ورود به تست با کنترل تایمر سخت‌افزاری
        set_var_admin_password("");
        if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
            lv_textarea_set_text(objects.text_area_framware_password, "");
        }
        // بازخورد بصری آنی (۰ms) تا کاربر بداند فرمان دریافت شده و منتظر بماند
        if (objects.waiting_panle_2 && lv_obj_is_valid(objects.waiting_panle_2)) {
            if (objects.obj7 && lv_obj_is_valid(objects.obj7)) {
                lv_label_set_text(objects.obj7, "در حال بارگذاری منوی تست سخت‌افزار...");
            }
            lv_obj_clear_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
            lv_refr_now(NULL);
        }
        hwtest_enter_ui();                           // ورود به محیط تست سخت‌افزار
    }
    else
    {
        app_sound_play(SOUND_ERROR);
        set_var_wrong_password_atemp(tr(STR_ADMIN_WRONG));
    }
}

// تغییر زبان: تنظیم، ذخیره در حافظه، و اعمال زنده روی صفحه‌ی فعلی
extern "C" void app_set_language(int lang)
{
    if (lang < 0 || lang >= LANG_COUNT) return;
    g_lang = (Lang)lang;
    storage.setLanguage(lang);
    i18n_apply();
}

// اجرای در بدو روشن شدن سخت‌افزار (در سطح کانتراکتورهای استاتیک C++ قبل از app_main و setup)
static void __attribute__((constructor(101))) early_hardware_safety_boot(void)
{
    hwtest_early_gpio_safety_kill();
}

void setup()
{
    // ۱. بالاترین اولویت ایمنی صنعتی: خاموشی بلادرنگ و قطعی تمام خروجی‌های رله و SSR قبل از هر پردازش یا تاخیر
    hwtest_kill_all_outputs_safe();
    set_var_app_version(VERSION);

    xMutex_I2C = xSemaphoreCreateMutex();
    xMutex_Wifi = xSemaphoreCreateMutex();
    g_lvgl_mutex = xSemaphoreCreateMutex();

    storage.begin();
    g_splash_variant = storage.consumeSplashAlternate();
    g_lang = (Lang)storage.getLanguage();
    set_var_buzzer(storage.getBuzzerVolume());

    // راه‌اندازی و خاموشی کامل بازر در بدو بوت (حذف هرگونه پالس یا بوق ناخواسته)
    ledcAttach(BUZZER, 7000, 8);
    ledcWrite(BUZZER, 0);
    app_sound_init();

    Serial.begin(115200);
    Serial0.begin(921600);   // UART0 -> CP210x at 921600 baud for fast debug, screenshots & secure remote
    delay(500);
    Serial.println("=== CONFIG BOOT ===");
    Serial0.println("=== CONFIG BOOT (UART0) ===");

    esp_reset_reason_t rst_reason = esp_reset_reason();
    Serial.printf("[BOOT] Reset reason: %d\n", (int)rst_reason);
    Serial0.printf("[BOOT] Reset reason: %d\n", (int)rst_reason);
    if (rst_reason == ESP_RST_BROWNOUT)
    {
        Serial.println("[ALARM/SAFETY] Brownout Reset Detected! Voltage dip on 24V/5V rail or heavy inductive load.");
        Serial0.println("[ALARM/SAFETY] Brownout Reset Detected! Voltage dip on 24V/5V rail or heavy inductive load.");
    }

    // تفکیک دقیق ریست سخت‌افزاری (روشن شدن با کلید پاور فیزیکی) از ریست نرم‌افزاری/کرش/هنگ طبق دستور کارفرما
    bool is_hardware_reset = (rst_reason == ESP_RST_POWERON || rst_reason == ESP_RST_EXT);

    // تشخیص ریست نرم‌افزاری، واچ‌داگ، براون‌اوت یا کرش (دلیل هنگ کردن یا ریست ناگهانی)
    bool is_soft_or_crash = (
        rst_reason == ESP_RST_SW ||
        rst_reason == ESP_RST_PANIC ||
        rst_reason == ESP_RST_INT_WDT ||
        rst_reason == ESP_RST_TASK_WDT ||
        rst_reason == ESP_RST_WDT ||
        rst_reason == ESP_RST_DEEPSLEEP ||
        rst_reason == ESP_RST_BROWNOUT
    );

    if (is_hardware_reset)
    {
        // ریست سخت‌افزاری/فیزیکی: شروع عادی سیستم از اسپلش و صفحه درخواست رمز ورود
        g_restore_needed = false;
        memset(&s_rtc_restore_ctx, 0, sizeof(s_rtc_restore_ctx));
        Serial.println("[BOOT] Physical Hardware Reset detected -> Clean boot to Splash & Password screen");
        Serial0.println("[BOOT] Physical Hardware Reset detected -> Clean boot to Splash & Password screen");
    }
    else if (is_soft_or_crash)
    {
        // ریست نرم‌افزاری یا کرش/هنگ: پرش مستقیم و بلادرنگ به همان صفحه‌ای که کاربر پیش از ریست در آن قرار داشته
        if (app_restore_is_valid(&s_rtc_restore_ctx))
        {
            g_restore_needed = true;
            g_restore_is_hwtest = s_rtc_restore_ctx.is_hwtest;
            g_restore_screen_id = s_rtc_restore_ctx.screen_id;
            g_restore_suite = s_rtc_restore_ctx.suite;
            Serial.printf("[RESTORE] Soft/Crash Reset: Restored from RTC: is_hwt=%d scr=%d suite=%d\n",
                          (int)g_restore_is_hwtest, (int)g_restore_screen_id, (int)g_restore_suite);
            Serial0.printf("[RESTORE] Soft/Crash Reset: Restored from RTC: is_hwt=%d scr=%d suite=%d\n",
                           (int)g_restore_is_hwtest, (int)g_restore_screen_id, (int)g_restore_suite);
        }
        else
        {
            uint8_t nvs_hwt = 0, nvs_scr = 0, nvs_ste = 0;
            if (storage.getLastScreen(nvs_hwt, nvs_scr, nvs_ste) && (nvs_hwt != 0 || nvs_scr > 1))
            {
                g_restore_needed = true;
                g_restore_is_hwtest = nvs_hwt;
                g_restore_screen_id = nvs_scr;
                g_restore_suite = nvs_ste;
                Serial.printf("[RESTORE] Soft/Crash Reset: Restored from NVS: is_hwt=%d scr=%d suite=%d\n",
                              (int)g_restore_is_hwtest, (int)g_restore_screen_id, (int)g_restore_suite);
                Serial0.printf("[RESTORE] Soft/Crash Reset: Restored from NVS: is_hwt=%d scr=%d suite=%d\n",
                               (int)g_restore_is_hwtest, (int)g_restore_screen_id, (int)g_restore_suite);
            }
            else
            {
                s_rtc_restore_ctx.magic = 0;
            }
        }
    }
    else
    {
        g_restore_needed = false;
        s_rtc_restore_ctx.magic = 0;
    }

    // ۱. بررسی کرش زودهنگام و حفاظت در برابر بوت‌لوپ (Auto-Rollback)
    ota_boot_guard_early_check();

    // ۲. کلید نجات سخت‌افزاری: اگر هنگام روشن شدن کلید پنل فشرده باشد و در پارتیشن دوم باشیم، فوراً به app0 رول‌بک می‌کند
    analogSetPinAttenuation(1, ADC_11db);
    analogSetPinAttenuation(2, ADC_11db);
    int raw_k1 = analogRead(1);
    int raw_k2 = analogRead(2);
    if ((raw_k1 < 3200 || raw_k2 < 3200) && esp_ota_get_running_partition() && 
        esp_ota_get_running_partition()->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        delay(400);
        if (analogRead(1) < 3200 || analogRead(2) < 3200) {
            ets_printf("[BOOT-RESCUE] Panel key held at boot! Forcing immediate rollback to Config-Test (app0)...\n");
            ota_rollback_to_config_and_reboot();
        }
    }

    // ۳. حفاظت از بوت: راه‌اندازی تایمر سخت‌افزاری ضد هنگ ۶ ثانیه‌ای (OTA Hang Guard)
    ota_guard_init();

    Wire.end();
    delay(100);

    // باید قبل از اولین WiFi.mode/begin باشد: هسته در همان init اول، حالتِ ذخیره‌سازیِ
    // پیکربندی (RAM در برابر NVS/فلش) را قفل می‌کند
    WiFi.persistent(false);                    // پیکربندی وای‌فای فقط در RAM — هر WiFi.begin در NVS/فلش ننویسد
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);                      // خاموش‌کردن modem-sleep → throughput بالاتر برای دانلود OTA
    WiFi.setTxPower(WIFI_POWER_19_5dBm);       // حداکثر توانِ ارسال (در صورت ضعیف‌بودن سیگنال کمک می‌کند)
    WiFi.disconnect();
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            g_wifi_last_disconnect_reason = (int)info.wifi_sta_disconnected.reason;
            #if HMI_PERF_SERIAL_LOG
            Serial.printf("[WIFI-EVENT] Disconnected. Reason: %d\n", g_wifi_last_disconnect_reason);
            #endif
            // Reason 15 = 4WAY_HANDSHAKE_TIMEOUT, 205 = CONNECTION_FAIL / AUTH_FAIL, 2 = AUTH_EXPIRE, 3 = AUTH_LEAVE
            if (g_wifi_last_disconnect_reason == 15 || g_wifi_last_disconnect_reason == 205 ||
                g_wifi_last_disconnect_reason == 2 || g_wifi_last_disconnect_reason == 3) {
                g_wifi_wrong_password = true;
            }
        }
    });
    delay(100);

    Serial.println("lv_init...");
    Serial0.println("[BOOT] lv_init...");
    lv_init();
    lv_tick_set_cb([]() -> uint32_t {
        return (uint32_t)(esp_timer_get_time() / 1000ULL);
    });
    Serial.println("lv_init OK (hardware microsecond tick active)");
    Serial0.println("[BOOT] lv_init OK");

    Serial.println("tft.begin...");
    Serial0.println("[BOOT] tft.begin...");
    tft.begin();
    tft.fillScreen(TFT_BLACK);
    Serial.println("tft OK");
    Serial0.println("[BOOT] tft OK");

    bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
    bbct.setOrientation(TOUCH_ORIENTATION, SCREEN_HEIGHT, SCREEN_WIDTH);

    tft.setRotation(SCREEN_ROTATION);

    Serial.println("Creating LVGL display...");
    Serial0.println("[BOOT] Creating LVGL display...");
    lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, my_disp_flush);

    // Partial-mode draw buffers in internal DMA-capable RAM (fast SPI push: 64 lines = only 5 slices per frame).
    const uint32_t try_lines[] = { 64, 48, 32, 24, 16, 10 };
    uint32_t lines = 0;
    size_t buf_bytes = 0;

    // گام ۱: تلاش برای تخصیص بافر دوگانه در حافظه داخلی DMA
    for (uint32_t i = 0; i < sizeof(try_lines) / sizeof(try_lines[0]); i++)
    {
        lines = try_lines[i];
        buf_bytes = SCREEN_WIDTH * lines * sizeof(lv_color_t);
        buf1 = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (buf1 && buf2) break;
        if (buf1) { heap_caps_free(buf1); buf1 = nullptr; }
        if (buf2) { heap_caps_free(buf2); buf2 = nullptr; }
    }

    // گام ۲: در صورت عدم امکان بافر دوگانه، بافر تکی در DMA امتحان می‌شود
    if (!buf1)
    {
        Serial.println("[BOOT] Double DMA buffer failed, trying single buffer fallback...");
        Serial0.println("[BOOT] Double DMA buffer failed, trying single buffer fallback...");
        for (uint32_t i = 0; i < sizeof(try_lines) / sizeof(try_lines[0]); i++)
        {
            lines = try_lines[i];
            buf_bytes = SCREEN_WIDTH * lines * sizeof(lv_color_t);
            buf1 = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
            if (buf1)
            {
                buf2 = nullptr;
                break;
            }
        }
    }

    // گام ۳: در صورت کمبود شدید SRAM، فال‌بک بافر تکی در PSRAM اعمال می‌شود
    if (!buf1)
    {
        Serial.println("[BOOT] Internal RAM exhausted, falling back to PSRAM draw buffer...");
        Serial0.println("[BOOT] Internal RAM exhausted, falling back to PSRAM draw buffer...");
        lines = 24;
        buf_bytes = SCREEN_WIDTH * lines * sizeof(lv_color_t);
        buf1 = (lv_color_t *)ps_malloc(buf_bytes);
        buf2 = nullptr;
    }

    if (!buf1)
    {
        Serial.println("[BOOT] Draw buffer alloc FAILED! Restarting in 3s...");
        Serial0.println("[BOOT] Draw buffer alloc FAILED! Restarting in 3s...");
        delay(3000);
        ESP.restart();
    }

    Serial.printf("Draw buffers OK: %u lines, %u bytes each (%s)\n",
                  lines, (unsigned)buf_bytes, buf2 ? "Double-Buf DMA" : "Single-Buf Fallback");
    Serial0.printf("[BOOT] Draw buffers OK: %u lines, %u bytes each (%s)\n",
                   lines, (unsigned)buf_bytes, buf2 ? "Double-Buf DMA" : "Single-Buf Fallback");
    lv_display_set_buffers(disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_indev_set_scroll_limit(indev, 6);
    lv_indev_set_scroll_throw(indev, 7); // ضریب اصطکاک پرتابی ملایم و ابریشمی به سبک اپل (۹۳٪ حفظ سرعت در هر گام)
    if (lv_indev_get_read_timer(indev)) lv_timer_set_period(lv_indev_get_read_timer(indev), 10); // دوره ۱۰ms برای روانی کامل ۶۰fps
    lv_indev_set_long_press_time(indev, 400);
    g_touch_indev = indev;


    if (g_restore_needed && !g_restore_is_hwtest)
    {
        g_firmware_suite = (FirmwareSuite)g_restore_suite;
        if (g_restore_screen_id == SCREEN_ID_SCREEN_FRAMWARE)
        {
            set_var_is_admin_password_verifyed(true);
        }
    }

    hwtest_setup();   // راه‌اندازی پریفرال‌های تست (RTC/ADC/RS485)
    hwtest_run_boot_self_check(); // تست اولیه سلامت سخت‌افزار در بدو روشن شدن (بدون MCP23017 و MAX6675)
    hwtest_detect_hardware();   // تشخیص اولیه‌ی پریفرال‌ها برای سازگاری
    app_sound_init();   // تضمین اتصال و آمادگی بازر پس از اتمام تمامی راه‌اندازی‌های سخت‌افزاری

    Wire.setClock(400000);

    Serial.println("ui_init...");
    Serial0.println("[BOOT] ui_init...");
    ui_init();
    Serial.println("ui_init OK");
    Serial0.println("[BOOT] ui_init OK");

    set_var_app_version(VERSION);

    if (g_restore_needed)
    {
        if (g_restore_is_hwtest)
        {
            hwtest_goto_screen_id(g_restore_screen_id);
            Serial0.printf("[BOOT] Early restored HWTest screen: %d\n", (int)g_restore_screen_id);
        }
        else
        {
            Serial0.printf("[BOOT] Early restored Launcher screen: %d\n", (int)g_restore_screen_id);
        }
    }

    config_start_wifi_scan();
    g_web_portal.start();

    xTaskCreatePinnedToCore(lvgl_task, "LVGL", 12288, NULL, 3, &lvglTaskHandle, 1);
    xTaskCreatePinnedToCore(app_task, "App", 16384, NULL, 1, &appTaskHandle, 0);

    Serial.printf("FW %s | upload: %s %s | changelog entries: %u\n",
                  VERSION, FW_BUILD_DATE, FW_BUILD_TIME, (unsigned)FW_CHANGELOG_COUNT);
    Serial0.printf("[BOOT] FW %s ready\n", VERSION);
}
void loop() { vTaskDelete(NULL); }
