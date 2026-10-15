#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <stdio.h>
#include <math.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <lwip/dns.h>
#include <lwip/ip_addr.h>
#include <esp_wifi.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "haffler_warranty.h"
#include "ota_pre_erase.h"
#include <lvgl.h>
#include "general.h" // <--- اضافه شد برای دسترسی به readDebouncedButton
#include "ui.h"      // <--- اضافه شد برای دسترسی به ui_tick
// #include "max6675.h"
#include "vars.h"
#include <TFT_eSPI.h>
#include "screens.h"

extern "C" bool fw_info_is_out_manually_on(int idx);
#include "storage.h"
#include "defines.h"
#include <RTClib.h>
#include "runtime_policy.h"
#include "panel_key_filter.h"
#include "date_utils.h"
#include "actions.h"
#include "agent_hmi.h"
#include "cloud_client.h"
#include "fault_log.h"
#include "boot_diag.h"

#ifndef ML
#define ML(fa, en, tr) (eez_flow_get_language_index()==2?(tr):(eez_flow_get_language_index()==1?(en):(fa)))
#endif

// --- تعریف متغیرهای خارجی ---
// به کامپایلر می‌گوییم این متغیرها در فایل دیگری (josh.ino) تعریف شده‌اند
extern ButtonState fPanel, mPanel;
extern TaskHandle_t buzzerTaskHandle;
extern MAX6675 thermocouple;
extern MAX6675 thermocouple2;
extern TFT_eSPI tft;
extern RTC_DS3231 rtc;
extern SemaphoreHandle_t spiMutex;
// دسترسیِ قفل‌دار به RTC (تعریف در main.cpp) — جلوگیری از تداخلِ I2C بین تسک‌ها/هسته‌ها
extern SemaphoreHandle_t rtcMutex;
extern "C" void main_menu_test_mode_stop(void); // در screens.c — خروج از حالتِ تست
extern "C" void check_ui_requested_toast(void);
extern "C" bool wifi_is_busy_searching_or_connecting(void);
extern "C" void wifi_abort_current_operation(void);
extern "C" void wifi_ui_request_password_panel(void);
DateTime rtc_now_s();
void rtc_adjust_s(const DateTime &dt);
float rtc_temp_s();
typedef enum {
    BUZZER_ACTION = 1,   // ۱ بوقِ کوتاه — لمسِ جاهایی که سناریو ندارند (متن/فضای خالی)
    BUZZER_CONFIRM,      // ۲ بوقِ کوتاه — جاهایی که سناریو دارند (باز شدنِ بخش/پیغام/اجرای یک کار)
    BUZZER_ERROR,        // ۲ بوقِ بلندتر از تأیید — خطای معمولی
    BUZZER_STARTUP,
    BUZZER_IMPORTANT,    // ۳ بوقِ کوتاه — تأییدِ نهاییِ یک سناریو (باز شدنِ دیالوگِ نگه‌داشتنی، ذخیره)
    BUZZER_CRITICAL      // ۳ بوقِ بلند — خطای خیلی مهم/جدی
} buzzer_event_t;
extern bool isSearching;
extern String wifi_names;
#ifndef WIFI_SCAN_META_MAX
#define WIFI_SCAN_META_MAX 24
#endif
// SSID برای اتصال: کنترل/# حذف — UTF-8 حفظ (نام هات‌اسپات). ASCII-only فقط در UI.
static String wifi_ssid_for_connect(const String &raw)
{
    const char *p = raw.c_str();
    if (!p) return String();
    size_t len = strnlen(p, 32);
    String out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)p[i];
        if (c < 0x20 || c == 0x7F) continue;
        if (c == '#' || c == '\n' || c == '\r') continue;
        out += (char)c;
    }
    out.trim();
    return out;
}

static String wifi_ssid_from_scan(int index)
{
    String raw = WiFi.SSID(index);
    const char *p = raw.c_str();
    if (!p) return String();
    size_t len = strnlen(p, 33);
    if (len > 32) len = 32;
    char tmp[33];
    memcpy(tmp, p, len);
    tmp[len] = '\0';
    return wifi_ssid_for_connect(String(tmp));
}
extern String selectedWifi;
extern String wifiPassword;
extern String wifis[WIFI_SCAN_META_MAX];
extern bool connectingToWifi;
extern volatile bool g_wifi_direct_scan_active;
extern SemaphoreHandle_t xMutex_Wifi;
extern SemaphoreHandle_t adcMutex;
#ifdef __cplusplus
extern "C" void queue_panel_navigation(int row, int key);
extern "C" bool panel_navigation_is_active(void);
extern "C" bool panel_nav_focus_is_repeatable(void);
extern "C" bool main_daily_stats_is_open(void);
extern "C" void main_request_close_daily_stats(void);
#else
void queue_panel_navigation(int row, int key);
bool panel_navigation_is_active(void);
bool panel_nav_focus_is_repeatable(void);
bool main_daily_stats_is_open(void);
void main_request_close_daily_stats(void);
#endif

extern Storage storage;
void process_pending_base_values_save();

bool wifiConnected = false;
int wifi_conncting_counter = 0;

extern char ota_download_model[96];
extern volatile bool ota_download;
extern char ota_custom_url_buf[256];
extern volatile bool ota_custom_download;
extern volatile bool g_ota_exit_request;

// ===== بروزرسانیِ صفحه اطلاعات (درجا) — وضعیتِ مشترک بین ota_task و UIِ main.cpp =====
#define SU_IDLE       0   // روی صفحه وای‌فا (انتخاب/اتصالِ کاربر)
#define SU_CHECKING   1   // در حال چک سرور
#define SU_UPTODATE   2   // نسخه به‌روز است → نوار ۵ث و بازگشت
#define SU_NEWER_ASK  3   // نسخهٔ جدید موجود → منتظر تأیید کاربر
#define SU_INSTALLING 4   // در حال نصب
#define SU_FAIL       5   // خطا → نوار ۵ث و بازگشت
#define SU_CONNECTING 6   // در حال اتصال به وای‌فا
#define SU_FAIL_SERVER   1
#define SU_FAIL_VERSION  2
#define SU_FAIL_INSTALL  3
extern volatile int  g_su_state;
extern volatile int  g_su_fail_code;
extern char          g_su_version[16];
extern volatile bool g_su_confirm;
extern volatile bool g_su_decline;
extern volatile bool g_self_update_session;
extern volatile bool g_wifi_rescan_request;
extern volatile bool g_wifi_init_request;
extern volatile bool g_wifi_ota_scan_pending;
extern volatile bool g_wifi_ota_rescan_request;
extern bool wifi_perform_sync_scan_and_update_ui(void);
extern int wifi_sta_sync_scan_raw(int maxAttempts);
extern volatile uint32_t g_output_inhibit_until_ms;
extern volatile uint32_t g_ota_return_sanitize_until_ms;
extern volatile int  g_su_scan_rc;
extern volatile bool webAppEnabled;
extern "C" bool wifi_sync_time_from_ntp(void);  // main.cpp — NTP + geo TZ → RTC
extern "C" int  fw_info_get_digital_in(int idx);
extern "C" void screensaver_note_activity(void);
static void self_update_check(void);   // forward

static inline void wifi_on_connected_hooks(void) {
    set_var_is_wifi_connected(true);
    // حفظ DNS مودم/روتر (DHCP) و تنظیم DNS سرور پایدار به عنوان بکاپ (8.8.8.8)
    IPAddress currentDns0 = WiFi.dnsIP(0);
    if (currentDns0 == IPAddress(0, 0, 0, 0) || currentDns0 == IPAddress(255, 255, 255, 255)) {
        ip_addr_t d1;
        IP_ADDR4(&d1, 8, 8, 8, 8);
        dns_setserver(0, &d1);
    }
    ip_addr_t d2;
    IP_ADDR4(&d2, 8, 8, 8, 8);
    dns_setserver(1, &d2);
    wifi_sync_time_from_ntp();
    cloud_client_on_wifi_connected();
    warranty_on_wifi_connected();
}

static uint32_t free_moving_time = 150;
int copy_time = 0;
int temp_adj = 0;
bool falt_ssr = 0;
bool stat_down = 0;
int temp_last = 0;
int welding_temp = 0;
bool emergency_push_button = false;
bool start_push_button = false;
static uint32_t last_back_trigger_ms = 0;

static inline void reset_panel_button_latches(void) {
    emergency_push_button = false;
    start_push_button = false;
    last_back_trigger_ms = 0;
}

static inline void force_all_outputs_off_after_update(void) {
    reset_panel_button_latches();
    set_var_emergency_stop(false);
    set_var_current_back_button_status(false);
    set_var_back_button_pressed(false);
    set_var_curent_pedal_status(false);
    set_var_step_process(false);
    set_var_step_number(0);
    set_var_josh_is_working(false);
    set_var_cool_is_working(false);
    // مهم: cycel_started=false همان چیزی است که پنلِ قرمزِ «استپ اضطراری» را در EEZ نشان می‌دهد.
    // بعد از برگشت از OTA فقط خروجی‌ها باید خاموش بمانند، نه اینکه UI حالتِ امرجنسی نشان دهد.
    set_var_cycel_started(true);
    set_var_progress_vlaue(0);
    set_var_test_mode(false);
    set_var_test_ssr(false);
    set_var_test_shablon(false);
    set_var_test_safhe(false);
    set_var_test_press1(false);
    set_var_test_press2(false);
    set_var_test_josh(false);
    set_var_test_zarbe1(false);
    set_var_test_zarbe2(false);
    set_var_shablon_out_display(false);
    set_var_safhe_out_display(false);
    set_var_press1_out_display(false);
    set_var_press2_out_display(false);
    set_var_josh_out_display(false);
    set_var_zarbe1_out_display(false);
    set_var_zarbe2_out_display(false);
    if (SSR_1 != PIN_NC) digitalWrite(SSR_1, OFF);
    if (SHABLON != PIN_NC) digitalWrite(SHABLON, OFF);
    if (TABLE != PIN_NC) digitalWrite(TABLE, OFF);
    if (PRESS_RIGHT != PIN_NC) digitalWrite(PRESS_RIGHT, OFF);
    if (PRESS_LEFT != PIN_NC) digitalWrite(PRESS_LEFT, OFF);
    if (ELEMENT != PIN_NC) digitalWrite(ELEMENT, OFF);
    if (ZARBEH1 != PIN_NC) digitalWrite(ZARBEH1, OFF);
    if (ZARBEH2 != PIN_NC) digitalWrite(ZARBEH2, OFF);
}

static inline bool ota_return_sanitize_active(void) {
    return (int32_t)(g_ota_return_sanitize_until_ms - millis()) > 0;
}

// پاکسازیِ کامل بعد از خروج از F5-بوت / OTA — جلوگیری از امرجنسیِ کاذب (کلید STOP، state کهنه)
static inline void ota_return_cleanup_weld_state(void) {
    reset_panel_button_latches();
    force_all_outputs_off_after_update();
    g_force_lamps_off = false;
    panel_key_filter::resetState(fPanel);
    panel_key_filter::resetState(mPanel);
}

typedef struct {
    bool stable;
    bool lastSample;
    uint32_t lastChangeMs;
} debounced_digital_t;

static bool read_digital_debounced(int pin, debounced_digital_t &state, uint32_t debounceMs) {
    bool sample = (digitalRead(pin) != 0);
    uint32_t now = millis();

    if (sample != state.lastSample) {
        state.lastSample = sample;
        state.lastChangeMs = now;
    }

    if ((now - state.lastChangeMs) >= debounceMs) {
        state.stable = state.lastSample;
    }

    return state.stable;
}

// آیتم ۱۵ — خواندن سنسور آنالوگ جک: true=رسیده، false=هنوز در راه / خطا
// wait_closed=true: انتظار برای بسته شدن کامل جک
// wait_closed=false: انتظار برای باز شدن کامل جک
// برای SENSOR_TABLE (دیجیتال): wait_closed نادیده گرفته می‌شود، فقط open N.O
void emergency_operation(); // forward — برای استوپِ کامل هنگام خطای سنسور

// میانگینِ سریعِ آنالوگ (~۳۲ نمونه، ~<۲ms) برای حذفِ نویز
static int adc_read_safe(uint8_t pin) {
    bool locked = (adcMutex && xSemaphoreTake(adcMutex, pdMS_TO_TICKS(50)) == pdTRUE);
    int v = analogRead(pin);
    if (locked) xSemaphoreGive(adcMutex);   // فقط در صورتِ گرفتنِ واقعی (رفعِ کرشِ assert)
    return v;
}

static int sensor_adc_avg(int pin) {
    if (pin < 0 || pin == PIN_NC) return SENSOR_BAND_NONE_LO + 50;
    int samples[5];
    int min_val = 4096;
    int max_val = -1;
    long sum = 0;
    bool locked = (adcMutex && xSemaphoreTake(adcMutex, pdMS_TO_TICKS(25)) == pdTRUE);
    for (int i = 0; i < 5; ++i) {
        samples[i] = analogRead((uint8_t)pin);
        if (samples[i] < min_val) min_val = samples[i];
        if (samples[i] > max_val) max_val = samples[i];
        sum += samples[i];
        delayMicroseconds(20);
    }
    if (locked) xSemaphoreGive(adcMutex);
    // حذف نویزهای گذرا و ضربه‌های القایی بوبین‌ها با دور انداختن کمترین و بیشترین نمونه
    sum -= (min_val + max_val);
    return (int)(sum / 3);
}

static int sensor_pin_index(int pin) {
    // فقط ترمینال فیزیکی — اگر SENSOR_* جابجا شده باشد نباید دو GPIO روی یک اسلات کش قاطی شوند
    if (pin == DEFAULT_SENSOR_ELEMENT) return 0;
    if (pin == DEFAULT_SENSOR_SHABLON) return 1;
    if (pin == DEFAULT_SENSOR_TABLE)   return 2;
    return 0;
}

static int sensor_band_from_adc(int adc) {
    if (adc >= SENSOR_BAND_CLOSED) return 0;
    if (adc >= SENSOR_BAND_OPEN_LO && adc <= SENSOR_BAND_OPEN_HI) return 1;
    if (adc >= SENSOR_BAND_NONE_LO && adc <= SENSOR_BAND_NONE_HI) return 2;
    if (adc >= SENSOR_BAND_BOTH_LO && adc <= SENSOR_BAND_BOTH_HI) return 3;
    return -1;
}

// خانوادهٔ منطقی جک — مستقل از مقدار GPIO که ممکن است بعد از remap بین جک‌ها یکی شود
enum { MUX_FAM_ELEMENT = 0, MUX_FAM_SHABLON = 1, MUX_FAM_TABLE = 2 };

static int mux_input_row_for_fam(int fam, bool want_closed_band) {
    switch (fam) {
        case MUX_FAM_ELEMENT: return want_closed_band ? 1 : 0;
        case MUX_FAM_SHABLON: return want_closed_band ? 3 : 2;
        case MUX_FAM_TABLE:   return want_closed_band ? -1 : 4;
        default: return -1;
    }
}
static bool mux_input_row_live(int row) {
    return row >= 0 && row < 8 && !storage.inRowNc[row] && storage.inRowPin[row] != PIN_NC;
}
static int mux_input_row_pin(int row) {
    if (!mux_input_row_live(row)) return PIN_NC;
    // GPIO از روی شمارهٔ I (هویت فیزیکی) — ردیف‌ها مستقل می‌مانند
    int silk = storage.inRowSilk[row];
    int gp = gpio_for_input_silk(silk);
    return (gp != PIN_NC) ? gp : storage.inRowPin[row];
}
static bool mux_row_signal_active(int row, int band) {
    if (row < 0 || row > 7) return false;
    int silk = storage.inRowSilk[row];
    // I6/I7 دیجیتال — با جابجایی به ردیف جک هم باید سیگنال بدهند
    if (silk >= 6 && silk <= 7) {
        int gp = gpio_for_input_silk(silk);
        if (gp < 0 || gp == PIN_NC) return false;
        return digitalRead(gp) == LOW;
    }
    return input_silk_signal_active(silk, band);
}
static bool mux_sensor_band_enabled(int fam, bool want_closed_band) {
    int row = mux_input_row_for_fam(fam, want_closed_band);
    if (row < 0) return false;
    return mux_input_row_live(row);
}

static int __attribute__((unused)) sensor_stable_band(int pin) {
    static int stable[3] = {2, 2, 2};
    static int cand[3] = {2, 2, 2};
    static uint8_t cnt[3] = {0, 0, 0};
    static int last_pinmap = -1;
    if (pin < 0 || pin == PIN_NC) return 2;
    extern int g_pinmap_gen;
    if (g_pinmap_gen != last_pinmap) {
        last_pinmap = g_pinmap_gen;
        for (int i = 0; i < 3; i++) {
            stable[i] = 2;
            cand[i] = 2;
            cnt[i] = 0;
        }
    }
    int idx = sensor_pin_index(pin);
    int band = sensor_band_from_adc(sensor_adc_avg(pin));
    if (band < 0) return stable[idx];
    if (band == cand[idx]) {
        if (cnt[idx] < 2) cnt[idx]++;
        if (cnt[idx] >= 2) stable[idx] = band;
    } else {
        cand[idx] = band;
        cnt[idx] = 1;
    }
    return stable[idx];
}
static bool jack_sensor_arrived(int fam, bool wait_closed) {
    int row = mux_input_row_for_fam(fam, wait_closed);
    if (row < 0 || !mux_input_row_live(row)) return false;
    return (fw_info_get_digital_in(row) != 0);
}
// سیگنالِ فعالِ یک ردیفِ ورودی (مثل لامپ منوی اطلاعات) — بعد از remap با I آنالوگ/دیجیتال
static bool input_row_asserted(int row) {
    if (!mux_input_row_live(row)) return false;
    return (fw_info_get_digital_in(row) != 0);
}
static bool back_button_active_now() {
    return input_row_asserted(5);
}

// sensor_id: 1=شابلون بسته, 2=شابلون باز, 3=میز باز, 4=المنت بسته, 5=المنت باز
static bool jack_sensor_done(bool sensor_en, int fam, bool wait_closed,
                             uint32_t start_ms, uint32_t base_timeout_ms, int sensor_id) {
    if (!sensor_en) return (millis() - start_ms >= base_timeout_ms);
    if (!mux_sensor_band_enabled(fam, wait_closed))
        return (millis() - start_ms >= base_timeout_ms);
    if (jack_sensor_arrived(fam, wait_closed)) return true;
    if (millis() - start_ms > SENSOR_TIMEOUT_MS) {
        int row = mux_input_row_for_fam(fam, wait_closed);
        int rowPin = mux_input_row_pin(row);
        int a = sensor_adc_avg(rowPin != PIN_NC ? rowPin : DEFAULT_SENSOR_ELEMENT);
        g_sensor_err_both = (a >= SENSOR_BAND_BOTH_LO && a <= SENSOR_BAND_BOTH_HI);
        g_sensor_err_id = sensor_id;
        g_sensor_err_request = 1;
        emergency_operation();
        return false;
    }
    return false;
}

static bool jack_sensor_off_done(bool sensor_en, int fam, bool which_closed,
                                 uint32_t start_ms, uint32_t base_timeout_ms, int sensor_id) {
    if (!sensor_en) return (millis() - start_ms >= base_timeout_ms);
    if (!mux_sensor_band_enabled(fam, which_closed))
        return (millis() - start_ms >= base_timeout_ms);
    if (!jack_sensor_arrived(fam, which_closed) && (millis() - start_ms >= base_timeout_ms)) return true;
    if (millis() - start_ms > SENSOR_TIMEOUT_MS) {
        int row = mux_input_row_for_fam(fam, which_closed);
        int rowPin = mux_input_row_pin(row);
        int a = sensor_adc_avg(rowPin != PIN_NC ? rowPin : DEFAULT_SENSOR_TABLE);
        g_sensor_err_both = (a >= SENSOR_BAND_BOTH_LO && a <= SENSOR_BAND_BOTH_HI);
        g_sensor_err_id = sensor_id;
        g_sensor_err_request = 1;
        emergency_operation();
        return false;
    }
    return false;
}

static bool check_all_jacks_open_or_error() {
    if (!get_var_setting_admin_jack_sensor()) return true;
    int error_mask = 0;
    if (mux_input_row_live(2) && !jack_sensor_arrived(MUX_FAM_SHABLON, false)) error_mask |= 1;
    if (mux_input_row_live(0) && !jack_sensor_arrived(MUX_FAM_ELEMENT, false)) error_mask |= 2;
    if (mux_input_row_live(4) && !jack_sensor_arrived(MUX_FAM_TABLE,   false)) error_mask |= 4;
    if (error_mask != 0) {
        g_sensor_err_id = 0x100 | error_mask;
        g_sensor_err_request = 1;
        return false;
    }
    return true;
}

static bool emergency_is_active_level(bool emergencyStableValue) {
    // true: active-low wiring, false: active-high wiring
    return get_var_setting_admin_emergency_switch() ? (!emergencyStableValue) : emergencyStableValue;
}

static bool back_trigger_cooldown_ok() {
    uint32_t now = millis();
    if ((uint32_t)(now - last_back_trigger_ms) < 500U) {
        return false;
    }
    last_back_trigger_ms = now;
    return true;
}



void download_firmware(String model);
static bool download_firmware_from_url(const String &url);
static bool try_download_firmware_from_source(const String &model, const String &baseUrl);
static bool installer_cached_version_for_model(const String &relModel, String &outVer);
void pedal_operation();
void emergency_operation();
// LVGL Tick Task
extern volatile float g_lvgl_cpu_pct;

void lv_tick_task(void *arg)
{
    while (true)
    {
        lv_tick_inc(1); // 1ms tick
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// لاگِ مطمئن روی UART0/COM4 (قابلِ‌صدا از screens.c که C است)
extern "C" void dbg0(const char *s) { Serial.println(s); }

// ---- آپدیتِ امنِ UIِ OTA: overlay روی scr_act (فقط نخِ LVGL می‌سازد) ----
extern const lv_font_t ui_font_kalameh_bold_18;
extern const lv_font_t ui_font_vazirmatn_bold_22;
static volatile bool g_ota_ui_pending = false;
static volatile bool g_ota_ui_close = false;
static volatile bool g_ota_cancel_requested = false;
static volatile uint32_t g_ota_ui_dismiss_at = 0; // millis؛ ۰ = تایمر خاموش
static volatile bool g_ota_progress_mode = false; // true = حلقهٔ درصد نصب
static volatile int  g_ota_ui_pct = -1;
static volatile bool g_ota_pct_pending = false;
static char g_ota_ui_text[80];
static portMUX_TYPE g_ota_ui_mux = portMUX_INITIALIZER_UNLOCKED;
static lv_obj_t *g_ota_overlay = NULL;
static lv_obj_t *g_ota_overlay_lbl = NULL;
static lv_obj_t *g_ota_overlay_arc = NULL;
static lv_obj_t *g_ota_overlay_pct = NULL;
static lv_obj_t *g_ota_overlay_close_btn = NULL;

static void ota_ui_set(const char *txt) {
    if (!txt) return;
    portENTER_CRITICAL(&g_ota_ui_mux);
    strncpy(g_ota_ui_text, txt, sizeof(g_ota_ui_text) - 1);
    g_ota_ui_text[sizeof(g_ota_ui_text) - 1] = '\0';
    g_ota_ui_pending = true;
    g_ota_ui_close = false; // پیام جدید، بستنِ قبلی را لغو کن
    portEXIT_CRITICAL(&g_ota_ui_mux);
}
static void ota_ui_close_panel(void) {
    g_ota_cancel_requested = true;
    g_ota_ui_dismiss_at = 0;
    g_ota_progress_mode = false;
    g_ota_ui_close = true;
}
extern "C" void ota_request_cancel(void) {
    g_ota_cancel_requested = true;
}
// پیغام نتیجه/خطا: بدون بلاک کردنِ ota_task — بعد از hold_ms از نخِ LVGL بسته می‌شود (ریست نمی‌کند)
static void ota_ui_show_then_dismiss(const char *txt, uint32_t hold_ms = 5000) {
    g_ota_progress_mode = false;
    ota_ui_set(txt);
    uint32_t t = millis() + hold_ms;
    if (t == 0) t = 1; // ۰ یعنی تایمر خاموش
    g_ota_ui_dismiss_at = t;
}
static void ota_ui_begin_install_progress(void) {
    g_ota_progress_mode = true;
    g_ota_ui_pct = 0;
    g_ota_pct_pending = true;
    ota_ui_set(ML("در حال دانلود و نصب...", "Downloading & installing...", "Indiriliyor ve kuruluyor..."));
}
extern "C" void ota_ui_notify(const char *txt) { ota_ui_set(txt); }

static void ota_ui_apply_pct_widgets(int pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    static int s_last_ota_pct = -1;
    if (s_last_ota_pct == pct) return;
    s_last_ota_pct = pct;
    if (g_ota_overlay_arc && lv_obj_is_valid(g_ota_overlay_arc))
        lv_arc_set_value(g_ota_overlay_arc, pct);
    if (g_ota_overlay_pct && lv_obj_is_valid(g_ota_overlay_pct)) {
        char pb[8];
        snprintf(pb, sizeof(pb), "%d%%", pct);
        lv_label_set_text(g_ota_overlay_pct, pb);
    }
}

static void ota_ui_layout_for_mode(bool progress)
{
    if (!g_ota_overlay || !lv_obj_is_valid(g_ota_overlay)) return;
    if (progress) {
        lv_obj_set_size(g_ota_overlay, 300, 220);
        if (g_ota_overlay_arc && lv_obj_is_valid(g_ota_overlay_arc)) {
            lv_obj_clear_flag(g_ota_overlay_arc, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(g_ota_overlay_arc, LV_ALIGN_TOP_MID, 0, 16);
        }
        if (g_ota_overlay_pct && lv_obj_is_valid(g_ota_overlay_pct)) {
            lv_obj_clear_flag(g_ota_overlay_pct, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(g_ota_overlay_pct, LV_ALIGN_TOP_MID, 0, 52);
        }
        if (g_ota_overlay_lbl && lv_obj_is_valid(g_ota_overlay_lbl)) {
            lv_obj_set_width(g_ota_overlay_lbl, 270);
            lv_obj_align(g_ota_overlay_lbl, LV_ALIGN_BOTTOM_MID, 0, -16);
        }
    } else {
        lv_obj_set_size(g_ota_overlay, 380, 100);
        if (g_ota_overlay_arc && lv_obj_is_valid(g_ota_overlay_arc))
            lv_obj_add_flag(g_ota_overlay_arc, LV_OBJ_FLAG_HIDDEN);
        if (g_ota_overlay_pct && lv_obj_is_valid(g_ota_overlay_pct))
            lv_obj_add_flag(g_ota_overlay_pct, LV_OBJ_FLAG_HIDDEN);
        if (g_ota_overlay_lbl && lv_obj_is_valid(g_ota_overlay_lbl)) {
            lv_obj_set_width(g_ota_overlay_lbl, 320);
            lv_obj_center(g_ota_overlay_lbl);
        }
    }
}

// فقط از نخِ LVGL صدا زده می‌شود (داخل lvgl_task)
static void ota_ui_apply(void) {
    // اگر overlay با تعویضِ صفحه پاک شده، اشاره‌گرِ معلق را پاک کن
    if (g_ota_overlay && !lv_obj_is_valid(g_ota_overlay)) {
        g_ota_overlay = NULL;
        g_ota_overlay_lbl = NULL;
        g_ota_overlay_arc = NULL;
        g_ota_overlay_pct = NULL;
        g_ota_overlay_close_btn = NULL;
    }
    if (g_ota_ui_pending) {
        char buf[80];
        bool progress = g_ota_progress_mode;
        portENTER_CRITICAL(&g_ota_ui_mux);
        strncpy(buf, g_ota_ui_text, sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        g_ota_ui_pending = false;
        portEXIT_CRITICAL(&g_ota_ui_mux);
        if (!g_ota_overlay) {
            lv_obj_t *scr = lv_scr_act();
            Serial.println("[OTA] overlay create on scr_act");
            if (scr) {
                g_ota_overlay = lv_obj_create(scr);
                lv_obj_set_size(g_ota_overlay, 380, 100);
                lv_obj_align(g_ota_overlay, LV_ALIGN_CENTER, 0, 0);
                lv_obj_clear_flag(g_ota_overlay, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_bg_color(g_ota_overlay, lv_color_hex(0xff182536), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(g_ota_overlay, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(g_ota_overlay, lv_color_hex(0xff2fc8d6), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(g_ota_overlay, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(g_ota_overlay, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(g_ota_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_shadow_width(g_ota_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

                // دکمه ضربدر قرمز برای بستن یا لغو دستی
                g_ota_overlay_close_btn = lv_btn_create(g_ota_overlay);
                lv_obj_set_size(g_ota_overlay_close_btn, 30, 30);
                lv_obj_align(g_ota_overlay_close_btn, LV_ALIGN_TOP_RIGHT, -6, 6);
                lv_obj_set_style_radius(g_ota_overlay_close_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(g_ota_overlay_close_btn, lv_color_hex(0xffdc2626), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(g_ota_overlay_close_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_add_event_cb(g_ota_overlay_close_btn, [](lv_event_t *e) {
                    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                        if (buzzerTaskHandle) xTaskNotify(buzzerTaskHandle, BUZZER_ACTION, eSetValueWithOverwrite);
                        ota_ui_close_panel();
                    }
                }, LV_EVENT_CLICKED, NULL);
                lv_obj_t *clbl = lv_label_create(g_ota_overlay_close_btn);
                lv_label_set_text(clbl, "X");
                lv_obj_center(clbl);
                lv_obj_set_style_text_font(clbl, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);

                // حلقهٔ درصد (فقط در حالت نصب دیده می‌شود)
                g_ota_overlay_arc = lv_arc_create(g_ota_overlay);
                lv_obj_set_size(g_ota_overlay_arc, 96, 96);
                lv_arc_set_rotation(g_ota_overlay_arc, 270);
                lv_arc_set_bg_angles(g_ota_overlay_arc, 0, 360);
                lv_arc_set_range(g_ota_overlay_arc, 0, 100);
                lv_arc_set_value(g_ota_overlay_arc, 0);
                lv_obj_remove_flag(g_ota_overlay_arc, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_clear_flag(g_ota_overlay_arc, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_arc_width(g_ota_overlay_arc, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_arc_width(g_ota_overlay_arc, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_arc_rounded(g_ota_overlay_arc, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_arc_rounded(g_ota_overlay_arc, true, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_arc_color(g_ota_overlay_arc, lv_color_hex(0xff2a3d52), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_arc_color(g_ota_overlay_arc, lv_color_hex(0xff2fc8d6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(g_ota_overlay_arc, LV_OPA_TRANSP, LV_PART_KNOB | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(g_ota_overlay_arc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                lv_obj_add_flag(g_ota_overlay_arc, LV_OBJ_FLAG_HIDDEN);

                g_ota_overlay_pct = lv_label_create(g_ota_overlay);
                lv_obj_set_style_text_font(g_ota_overlay_pct, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(g_ota_overlay_pct, lv_color_hex(0xffe8fbff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(g_ota_overlay_pct, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_base_dir(g_ota_overlay_pct, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(g_ota_overlay_pct, "0%");
                lv_obj_add_flag(g_ota_overlay_pct, LV_OBJ_FLAG_HIDDEN);

                g_ota_overlay_lbl = lv_label_create(g_ota_overlay);
                lv_obj_set_width(g_ota_overlay_lbl, 320);
                lv_label_set_long_mode(g_ota_overlay_lbl, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_align(g_ota_overlay_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(g_ota_overlay_lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(g_ota_overlay_lbl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(g_ota_overlay_lbl);
            }
        }
        ota_ui_layout_for_mode(progress);
        if (progress)
            ota_ui_apply_pct_widgets(g_ota_ui_pct >= 0 ? g_ota_ui_pct : 0);
        if (g_ota_overlay_lbl && lv_obj_is_valid(g_ota_overlay_lbl)) {
            if (eez_flow_get_language_index() == 0)
                lv_obj_set_style_base_dir(g_ota_overlay_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
            else
                lv_obj_set_style_base_dir(g_ota_overlay_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(g_ota_overlay_lbl, buf);
        }
        if (g_ota_overlay && lv_obj_is_valid(g_ota_overlay))
            lv_obj_align(g_ota_overlay, LV_ALIGN_CENTER, 0, 0);
    }
    if (g_ota_pct_pending) {
        g_ota_pct_pending = false;
        if (g_ota_progress_mode)
            ota_ui_apply_pct_widgets(g_ota_ui_pct);
    }
    // تایمر ۵ثانیه‌ی پیغام نتیجه/خطا
    if (g_ota_ui_dismiss_at != 0 && (int32_t)(millis() - g_ota_ui_dismiss_at) >= 0) {
        g_ota_ui_dismiss_at = 0;
        g_ota_ui_close = true;
    }
    if (g_ota_ui_close) {
        g_ota_ui_close = false;
        g_ota_ui_dismiss_at = 0;
        g_ota_progress_mode = false;
        g_ota_pct_pending = false;
        if (g_ota_overlay && lv_obj_is_valid(g_ota_overlay)) lv_obj_del(g_ota_overlay);
        g_ota_overlay = NULL;
        g_ota_overlay_lbl = NULL;
        g_ota_overlay_arc = NULL;
        g_ota_overlay_pct = NULL;
        g_ota_overlay_close_btn = NULL;
        set_var_download_panle_state(false);
    }
}

extern "C" bool lock_is_active(void);   // وقتی صفحه‌ی قفل فعال است، تیکِ سنگینِ صفحه‌ی اصلی اجرا نشود
extern "C" bool company_overlay_is_open(void);
extern "C" bool multi_timer_dialog_is_open(void);
extern "C" bool admin_setup_overlay_is_open(void);

#ifndef HMI_PERF_SERIAL_LOG
#define HMI_PERF_SERIAL_LOG 0
#endif

extern "C" bool main_intro_is_active(void);

// LVGL Render Task
void lvgl_task(void *param)
{
    esp_task_wdt_add(NULL); // FIX: ثبت در WDT تا timeout panic ندهد
    uint32_t last_ui_tick = 0;
    int64_t busy_us = 0;
    int64_t win_start = esp_timer_get_time();
    // DIAG: تفکیکِ زمانِ رندر (lv_timer_handler) از زمانِ منطق (ui_tick=eez_flow_tick+tick_screen)
    int64_t lvgl_us = 0, uitick_us = 0; uint32_t uitick_cnt = 0;
    while (true)
    {
        int64_t t0 = esp_timer_get_time();
        // FIX: agent_hmi_poll فقط یک بار — قبل از lv_timer_handler (حذف تکرار ابتدای loop)
        uint32_t now = millis();
        bool high_rate = ((g_last_activity_ms != 0 && (now - g_last_activity_ms) < 3000) ||
                          (lv_anim_count_running() > 0) ||
                          g_agent_touch_active ||
                          main_intro_is_active() ||
                          get_var_numeric_keyboard_open() ||
                          is_warranty_screen_open() ||
                          get_var_step_process() ||
                          get_var_step_number() > 0 ||
                          get_var_josh_is_working() ||
                          get_var_cool_is_working() ||
                          get_var_test_mode());
        uint32_t ui_interval = high_rate ? 16 : 25;
        if ((uint32_t)(now - last_ui_tick) >= ui_interval)
        {
            check_ui_requested_toast();
            int64_t u0 = esp_timer_get_time();
            // overlayهای تمام‌صفحه (قفل / setup اولیه / اسکرین‌سیور / مودال گارانتی) → ui_tick خاموش تا تایپ روی کیبرد و اسکرین‌سیور فوق‌العاده روان و بدون بار پردازنده باشند
            if (!lock_is_active() && !company_overlay_is_open() && !admin_setup_overlay_is_open() && !multi_timer_dialog_is_open() && !g_screensaver_active && !is_warranty_screen_open() && !warranty_expired_dialog_is_open()) ui_tick();
            uitick_us += esp_timer_get_time() - u0;
            uitick_cnt++;
            last_ui_tick = now;
        }

        agent_hmi_poll(&Serial);
        agent_shot_arm_before_render();
        uint32_t wait_ms = lv_timer_handler();
        ota_ui_apply();   // اعمالِ امنِ آپدیتِ UIِ OTA در نخِ LVGL
        int64_t tA = esp_timer_get_time();
        lvgl_us += tA - t0;
        int64_t t1 = esp_timer_get_time();
        busy_us += t1 - t0;
        if (t1 - win_start >= 1000000LL) {
            g_lvgl_cpu_pct = (float)busy_us / (float)(t1 - win_start) * 100.0f;
#if HMI_PERF_SERIAL_LOG
            Serial.printf("[PERF] cpu=%.0f%% render=%lldms/s uitick=%lldms/s (n=%u avg=%lluus)\n",
                          g_lvgl_cpu_pct, (long long)(lvgl_us/1000), (long long)(uitick_us/1000),
                          (unsigned)uitick_cnt, (unsigned long long)(uitick_cnt? uitick_us/uitick_cnt:0));
#endif
            busy_us = 0; lvgl_us = 0; uitick_us = 0; uitick_cnt = 0; win_start = t1;
        }
        if (wait_ms == 0) wait_ms = 1;
        uint32_t min_delay = high_rate ? runtime_policy::LVGL_DELAY_MIN_MS : 2;
        uint32_t max_delay = high_rate ? runtime_policy::LVGL_DELAY_MAX_MS : 8;
        if (wait_ms < min_delay)
        {
            wait_ms = min_delay;
        }
        else if (wait_ms > max_delay)
        {
            wait_ms = max_delay;
        }
        esp_task_wdt_reset(); // FIX: ریست WDT در هر فریم — جلوگیری از Panic ریست
        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

// Keypad Task
void keypad_task(void *param)
{
    esp_task_wdt_add(NULL); // FIX: ثبت در WDT
    static uint32_t last_change_time = 0;
    static bool last_raw_state = false;
    static bool back_button_state = false;
    static int last_key_f = 0;
    static int last_key_m = 0;

    while (true)
    {
        esp_task_wdt_reset(); // FIX: WDT reset در ابتدای هر سیکل (حتی در صورت ادامه مسیر به خاطر قفل یا اسکرین‌سیور)
        int keyF = panel_key_filter::readDebouncedButtonRobust(KEY_F, fPanel);
        int keyM = panel_key_filter::readDebouncedButtonRobust(KEY_M, mPanel);
        if (keyM == 5 && keyM != last_key_m) {
            if (wifi_is_busy_searching_or_connecting()) {
                wifi_abort_current_operation();
            }
        }
        // بعد از برگشت از F5-بوت: چند ثانیه هیچ کلیدِ جوش/استاپ پردازش نشود (فقط بیدارکردن اسکرین‌سیور)
        if (ota_return_sanitize_active()) {
            emergency_push_button = false;
            start_push_button = false;
            set_var_current_back_button_status(false);
            set_var_back_button_pressed(false);
            set_var_curent_pedal_status(false);
            if (g_screensaver_active && !g_device_locked &&
                eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && (keyF != 0 || keyM != 0)) {
                screensaver_note_activity();
            }
            last_key_f = keyF;
            last_key_m = keyM;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        // اسکرین‌سیور: هر کلیدِ پنل (F1..F5، MOD، START، STOP) فقط بیدار کردن — بدون اجرای اکشنِ زیرین
        if (g_screensaver_active && !g_device_locked &&
            eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && !get_var_numeric_keyboard_open()) {
            if (keyF != 0 || keyM != 0) {
                screensaver_note_activity();
                last_key_f = keyF;
                last_key_m = keyM;
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
        }
        // در حالتِ قفل، کلیدها فقط کیبردِ قفل را بچرخانند — هرگز به صفحهٔ زیرین (منو/تنظیمات) نشت نکنند.
        // MOD=حرکتِ نشانگر بینِ دکمه‌ها ، START=زدنِ دکمهٔ انتخاب‌شده ، STOP=پاک‌کردنِ یک رقم.
        if (g_device_locked) {
            // کلید را فقط «صف» کن تا در تاسکِ LVGL (panel_nav_timer_cb) امن پردازش شود.
            // فراخوانیِ مستقیمِ LVGL از این تاسک (هستهٔ ۰) ناامن است و کار نمی‌کرد.
            if (keyM != 0 && keyM != last_key_m) {
                queue_panel_navigation(2, keyM);
            }
            last_key_f = keyF;
            last_key_m = keyM;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        int current_screen = eez_flow_get_current_screen();
        const bool settings_screen = current_screen >= SCREEN_ID_SCREEN_SETTING_GENERAL &&
                                     current_screen <= SCREEN_ID_SCREEN_WIFI;
        if (settings_screen) {
            static uint32_t m_hold_start = 0;   // مورد ۵: زمانِ شروعِ نگه‌داشتنِ START/STOP
            static uint32_t m_last_repeat = 0;
            if (keyF != 0 && keyF != last_key_f) queue_panel_navigation(1, keyF);
            if (keyM != 0 && keyM != last_key_m) {
                queue_panel_navigation(2, keyM);
                m_hold_start = millis();
                m_last_repeat = millis();
            } else if ((keyM == 4 || keyM == 5) && keyM == last_key_m &&
                       panel_navigation_is_active() && panel_nav_focus_is_repeatable()) {
                // مورد ۵: نگه‌داشتنِ بیش از ۱ ثانیه → افزایش/کاهشِ پیوسته تا وقتی دست روی کلید است
                uint32_t now = millis();
                if ((uint32_t)(now - m_hold_start) >= 1000U && (uint32_t)(now - m_last_repeat) >= 150U) {
                    queue_panel_navigation(2, keyM);
                    m_last_repeat = now;
                }
            }
            last_key_f = keyF;
            last_key_m = keyM;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        const bool panel_nav_mode = panel_navigation_is_active();
        static uint32_t menu_m_hold_start = 0;
        static uint32_t menu_m_last_repeat = 0;
        if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && !get_var_numeric_keyboard_open() && keyM != 0 && keyM != last_key_m &&
            (panel_nav_mode || keyM == 3)) {
            queue_panel_navigation(2, keyM);
            menu_m_hold_start = millis();
            menu_m_last_repeat = millis();
        } else if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && !get_var_numeric_keyboard_open() &&
                   (keyM == 4 || keyM == 5) && keyM == last_key_m &&
                   panel_nav_mode && panel_nav_focus_is_repeatable()) {
            uint32_t now = millis();
            if ((uint32_t)(now - menu_m_hold_start) >= 1000U &&
                (uint32_t)(now - menu_m_last_repeat) >= 150U) {
                queue_panel_navigation(2, keyM);
                menu_m_last_repeat = now;
            }
        } else if (keyM == 0) {
            menu_m_hold_start = 0;
            menu_m_last_repeat = 0;
        }
        if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && !get_var_numeric_keyboard_open() && keyF != 0 && keyF != last_key_f && panel_nav_mode) {
            queue_panel_navigation(1, keyF);
        }
        last_key_f = keyF;
        last_key_m = keyM;
        if (get_var_test_mode()) {
            if (keyM == 5) main_menu_test_mode_stop(); // استاپِ پنل باید در حالتِ تست هم خارج کند
            set_var_current_back_button_status(false);
            emergency_push_button = false;
            start_push_button = false;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (get_var_numeric_keyboard_open()) {
            set_var_current_back_button_status(false);
            emergency_push_button = false;
            start_push_button = false;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (keyF != 0 && !(eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && panel_nav_mode))
        {
            
            // Serial.println("F Key : " + String(keyF));
            if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && get_var_step_number()==0 && !get_var_step_process()){
                    if ("F" + String(keyF) != get_var_enable_mode()){
                        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                        String mode = "F" + String(keyF);
                        set_var_enable_mode(mode.c_str());
                        set_var_welding_time(storage.getWildingTime(keyF));
                        set_var_cooling_time(storage.getCoolingTime(keyF));
                        set_var_element_temperature_target(storage.getTempTarget(keyF));
                    }
              
            }else if((keyF==5 && eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && !get_var_current_back_button_status() && !get_var_back_button_pressed() && get_var_cycel_started()) &&  (get_var_step_number()==3 || get_var_step_number()==4) && back_trigger_cooldown_ok()){
                xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                set_var_back_button_pressed(true);
                set_var_current_back_button_status(true);
            }else{
                set_var_current_back_button_status(false);
            }
        }else{
            set_var_current_back_button_status(false);
        }

        if (keyM != 0 && !(eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && panel_nav_mode))
        {
            if(keyM==5 && !emergency_push_button){
                xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                emergency_push_button=true;
                set_var_cycel_started(false);
                emergency_operation();
                // Serial.println("11111111");
            }else if(keyM==4 && eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && main_daily_stats_is_open()){
                // پاپ‌آپِ گزارش روزانه باز است → START فقط آن را ببندد، جوش شروع نشود
            }else if(keyM==4 && !start_push_button && !get_var_curent_pedal_status() && !get_var_back_button_pressed() && !get_var_step_process()){
                start_push_button=true;
                if (warranty_is_expired_and_locked()) {
                    g_warranty_expired_dialog_request = true;
                    xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
                } else if (!g_device_locked && eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU) {
                    if (g_screensaver_active) {
                        // این فشردنِ START فقط اسکرین‌سیور را بیدار کرد؛ سیکل شروع نشود (مثلِ پدال)
                        screensaver_note_activity();
                    } else {
                        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                        pedal_operation();
                    }
                } else if (eez_flow_get_current_screen() != SCREEN_ID_SCREEN_MENU && eez_flow_get_current_screen() != SCREEN_ID_SCREEN_SPLASH) {
                    // فشردن دکمه استارت خارج از صفحه اصلی (منوی تنظیمات) -> عدم شروع جوش و نمایش پیام راهنما
                    g_pedal_outside_menu_warn_request = 1;
                    xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
                }
            }
        }else{
            // وقتی به‌خاطرِ حالتِ MOD روی صفحهٔ اصلی، مسیرِ جوش بلاک شده:
            // اگر START/STOP نگه داشته شده، آن را «لچ» کن تا اگر MOD تمام شد و کلید هنوز پایین بود،
            // بلافاصله جوش/استاپ شلیک نشود (مورد کاربر: استارت در حالتِ تایید نباید برنامه را شروع کند).
            // فقط با رهاکردنِ کامل کلید (keyM==0) لچ آزاد می‌شود.
            if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU && panel_nav_mode) {
                if (keyM == 4) start_push_button = true;
                else if (keyM == 5) emergency_push_button = true;
                else if (keyM == 0) { emergency_push_button = false; start_push_button = false; }
            } else {
                emergency_push_button=false;
                start_push_button=false;
            }
            // Serial.println("33333333");

        }

        // برگشت اکنون روی باندِ بالای GPIO4 (مشترک با سنسورِ بازِ میز) خوانده می‌شود
        bool back_button_raw_state = back_button_active_now();
        if (back_button_raw_state != last_raw_state) {
            last_change_time =  millis();
            last_raw_state = back_button_raw_state;
        }
        if ((millis() - last_change_time) >= 50) {
            if (back_button_state != back_button_raw_state) {
                back_button_state = back_button_raw_state;    
                if (back_button_state && !get_var_current_back_button_status() && !get_var_back_button_pressed() && get_var_cycel_started() && (get_var_step_number()==3 || get_var_step_number()==4) && back_trigger_cooldown_ok()) {
                    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                    set_var_back_button_pressed(true);
                    set_var_current_back_button_status(true);
                }else{
                    set_var_current_back_button_status(false);
                }
            }
        }
        esp_task_wdt_reset(); // FIX: WDT reset در هر سیکل keypad
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int readTemp(void) {
    constexpr uint32_t min_interval_ms = 270U; // Hardware safe delay (>260ms per MAX6675 requirement)
    constexpr int sample_count = 5;
    static float ring_buf[sample_count] = {0};
    static int ring_idx = 0;
    static bool ring_initialized = false;
    static uint8_t good_streak = 0;
    static int last_valid_result = 0;

    bool was_error = get_var_thermocouple_error();

    // ۱. بررسی مستقیم بایت‌های خام SPI سخت‌افزار MAX6675
    // در صورت قطعی سیم ترموکوپل (بیت D2 برابر 1) یا عدم اتصال سنسور/خط شناور (0x0000 یا 0xFFFF)
    // فیلتر مقاومت در برابر اسپایک‌های نویز EMI کنتاکتور: ثبت خطا تنها پس از ۳ نمونه نامعتبر متوالی
    static uint8_t s_bad_raw_count = 0;
    uint16_t raw_check = thermocouple.readRaw();
    bool raw_invalid = ((raw_check & 0x0004) || raw_check == 0x0000 || raw_check == 0xFFFF);
    if (raw_invalid) {
        s_bad_raw_count++;
        if (s_bad_raw_count >= 3) {
            good_streak = 0;
            set_var_thermocouple_error(true);
            return 10000;
        }
        if (last_valid_result > 0) return last_valid_result;
        return 10000;
    } else {
        s_bad_raw_count = 0;
    }

    if (!ring_initialized) {
        // خوانش اولیه ۵ نمونه در استارت‌آپ با رعایت دقیق تاخیر ۲۷۰ میلی‌ثانیه
        for (int i = 0; i < sample_count; i++) {
            esp_task_wdt_reset();
            if (i > 0) vTaskDelay(pdMS_TO_TICKS(min_interval_ms));
            uint16_t r = thermocouple.readRaw();
            if ((r & 0x0004) || r == 0x0000 || r == 0xFFFF) {
                good_streak = 0;
                set_var_thermocouple_error(true);
                return 10000;
            }
            double s = (r >> 3) * 0.25;
            if (isnan(s) || s <= 2.0 || s > 500.0) {
                good_streak = 0;
                set_var_thermocouple_error(true);
                return 10000;
            }
            ring_buf[i] = (float)(s * 4.0);
        }
        ring_initialized = true;
    } else {
        double s = (raw_check >> 3) * 0.25;
        if (isnan(s) || s <= 2.0 || s > 500.0) {
            good_streak = 0;
            set_var_thermocouple_error(true);
            return 10000;
        }
        ring_buf[ring_idx] = (float)(s * 4.0);
        ring_idx = (ring_idx + 1) % sample_count;
    }

    // فیلتر رتبه و میانه ۵ نقطه‌ای بر روی بافر حلقوی
    float sorted[sample_count];
    for (int i = 0; i < sample_count; i++) sorted[i] = ring_buf[i];
    for (int i = 0; i < sample_count - 1; i++) {
        for (int j = i + 1; j < sample_count; j++) {
            if (sorted[j] < sorted[i]) {
                float tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }

    // میانگین پیرایش‌شده ۳ نمونه میانی (حذف کمترین و بیشترین داده پرت)
    float raw_q = (sorted[1] + sorted[2] + sorted[3]) / 3.0f;
    int32_t corrected_q = (int32_t)(raw_q * 0.74815f + 50.613f);

    if (corrected_q < 0) {
        good_streak = 0;
        set_var_thermocouple_error(true);
        return 10000;
    }
    if (was_error) {
        good_streak++;
        if (good_streak < 2) {
            return 10000; // اولین خوانشِ خوب بعدِ خطا — هنوز تأیید نشده، خطا را نگه دار
        }
    } else {
        good_streak = 0;
    }
    set_var_thermocouple_error(false);
    last_valid_result = (int)(corrected_q * get_var_temp_offset());
    return last_valid_result;
}

static void update_second_thermocouple_text(void) {
    // اعتبارسنجی سخت‌افزاری سطح سیگنال ترموکوپل ۲ (MAX6675)
    // بررسی خطای مدار باز (Open Circuit - D2) و خطای اتصال فیزیکی یا باس شناور (0x0000 یا 0xFFFF)
    uint16_t raw2 = thermocouple2.readRaw();
    char text[16];
    if ((raw2 & 0x0004) || raw2 == 0x0000 || raw2 == 0xFFFF) {
        snprintf(text, sizeof(text), "--");
    } else {
        double temp_c = (raw2 >> 3) * 0.25;
        if (isnan(temp_c) || temp_c <= 2.0 || temp_c > 500.0) {
            snprintf(text, sizeof(text), "--");
        } else {
            snprintf(text, sizeof(text), "%.0f\xC2\xB0", temp_c);
        }
    }
    set_var_second_temperature_text(text);
}

void heater_task(void *param) {
    esp_task_wdt_add(NULL); // FIX: ثبت در WDT
    static int filtered_element_temp = -10000;
    uint32_t last_temp_read_ms = 0;
    uint32_t on_ms = 0;
    float current_duty = 0.0f;

    while(true){
        esp_task_wdt_reset();
        uint32_t now = millis();

        // ایمنی و کنترل تست دستی: در حالت تست خروجی‌ها یا تست دستی SSR، تسک حرارتی هیچ تداخلی ایجاد نمی‌کند
        if (get_var_test_mode() || get_var_test_ssr() || fw_info_is_out_manually_on(5)) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // ۱. خوانش دما با رعایت حداقل ۲۷۰ میلی‌ثانیه بین نمونه‌ها (> 256ms مبدل سخت‌افزاری MAX6675)
        if (now - last_temp_read_ms >= 270U || last_temp_read_ms == 0) {
            last_temp_read_ms = now;
            temp_adj = readTemp();
            update_second_thermocouple_text();

            if (g_device_locked || warranty_is_expired_and_locked()) {
                if (SSR_1 != PIN_NC && SSR_1 >= 0) digitalWrite(SSR_1, OFF);
                g_ssr_fault = false;
                falt_ssr = 0;
                on_ms = 0;
            } else if (!get_var_thermocouple_error()) {
                int measured_element_temp = 0;
                if (get_var_setting_admin_thermocouple_j()) {
                    measured_element_temp = (temp_adj / 4) + get_var_setting_general_temperature_caliber();
                } else {
                    measured_element_temp = ((int)(temp_adj / 4) / 1.38) + get_var_setting_general_temperature_caliber();
                }
                if (filtered_element_temp < -1000) {
                    filtered_element_temp = measured_element_temp;
                } else {
                    int delta = measured_element_temp - filtered_element_temp;
                    if (delta > 4) delta = 4;
                    if (delta < -4) delta = -4;
                    filtered_element_temp += delta;
                }
                set_var_element_temperature(filtered_element_temp);

                // فید فیلتر و ریت تغییرات دما در کنترلر صنعتی
                g_industrial_pid.filter_sample((float)filtered_element_temp);

                // تشخیص مقاومِ «المنت گرم نمی‌شود» و فرار حرارتی (Thermal Runaway / Heater Disconnect Protection)
                {
                    static int ssr_chk_base = -1000;
                    static uint32_t ssr_chk_ms = 0;
                    static int last_tgt = -1;
                    static uint32_t ssr_fault_time = 0;
                    int curT = get_var_element_temperature();
                    int tgt  = get_var_element_temperature_target();

                    // اگر کاربر دمای هدف را تغییر داد یا روی هشدار زد، مانیتورینگ ریست شود
                    if (tgt != last_tgt || g_ssr_warn_dismissed) {
                        last_tgt = tgt;
                        ssr_chk_base = -1000;
                        ssr_chk_ms = 0;
                        falt_ssr = 0;
                        g_ssr_fault = false;
                        g_ssr_warn_dismissed = false;
                    }

                    // ارزیابی فعال: تقاضای توان حرارتی پیوسته با اختلاف دمای معنادار (حداقل ۱۵ درجه)
                    bool demanding_heat = ((tgt - curT) >= 15) && (on_ms >= 800);

                    if (demanding_heat) {
                        if (ssr_chk_base <= -1000) {
                            ssr_chk_base = curT;
                            ssr_chk_ms = millis();
                        } else if (curT >= ssr_chk_base + 1) {
                            // هر ۱ درجه افزایش نشان‌دهنده سلامت کامل هیتر و المنت است؛ نوسازی تایمر
                            ssr_chk_base = curT;
                            ssr_chk_ms = millis();
                            falt_ssr = 0;
                            g_ssr_fault = false;
                        } else if (millis() - ssr_chk_ms >= 300000UL) { // ۵ دقیقه مهلت گرم‌شدن در کارگاه سرد
                            if (falt_ssr == 0) {
                                fault_log_record(FAULT_HEATER_TIMEOUT, (int16_t)curT);
                                ssr_fault_time = millis();
                            }
                            falt_ssr = 1;
                            // بازیابی خودکار: پس از ۳۰ ثانیه مکث ایمن، فرصت ارزیابی مجدد داده شود تا سیستم قفل ابدی نماند
                            if (millis() - ssr_fault_time >= 30000UL) {
                                ssr_chk_base = -1000;
                                ssr_chk_ms = 0;
                                falt_ssr = 0;
                                g_ssr_fault = false;
                            }
                        }
                    } else {
                        ssr_chk_base = -1000;
                        ssr_chk_ms = 0;
                        if (curT >= tgt - 8) {
                            falt_ssr = 0; // ریست ایمن فالت با رسیدن به محدوده دمای مطلوب
                            g_ssr_fault = false;
                        }
                    }
                }
                g_ssr_fault = (falt_ssr != 0);

                int cur_temp = get_var_element_temperature();
                int base_target = get_var_element_temperature_target();
                int eff_target = g_predictive_thermal.get_effective_target(base_target, g_screensaver_active);
                int pid_slider = get_var_setting_general_pid();

                // الگوریتم PID صنعتی با فرود نرم و جبران اینرسی حرارتی (Anti-Windup Soft Landing)
                current_duty = g_industrial_pid.compute_duty((float)eff_target, (float)cur_temp, pid_slider);

                if (eff_target <= 20 || current_duty <= 0.005f) {
                    on_ms = 0;
                } else if (current_duty >= 0.995f) {
                    on_ms = 1000;
                } else {
                    on_ms = (uint32_t)(current_duty * 1000.0f);
                    if (on_ms < 50) on_ms = 50;
                    if (on_ms > 950) on_ms = 950;
                }

                static uint32_t s_last_heat_diag_ms = 0;
                if (now - s_last_heat_diag_ms >= 3000U) {
                    s_last_heat_diag_ms = now;
                    Serial.printf("[HEATER] curT=%d tgt=%d eff=%d duty=%.2f on_ms=%lu ssrFault=%d\n",
                                  cur_temp, base_target, eff_target, current_duty, (unsigned long)on_ms, (int)g_ssr_fault);
                }
            } else {
                if (SSR_1 != PIN_NC && SSR_1 >= 0) digitalWrite(SSR_1, OFF);
                g_ssr_fault = false;
                on_ms = 0;
                filtered_element_temp = -10000;
                set_var_element_temperature(-999);
                static bool s_thermo_fault_logged = false;
                if (!s_thermo_fault_logged) {
                    s_thermo_fault_logged = true;
                    fault_log_record(FAULT_THERMO_DISCONNECT, -999);
                }
            }
        }

        // ۲. مدولاسیون زمانی غیرانسدادی SSR در پنجره ۱۰۰۰ میلی‌ثانیه‌ای (Time-Proportioning PWM)
        // حیاتی: هشدار g_ssr_fault صرفاً پیام روی صفحه است و هرگز نباید خروجی SSR را قطع کند، وگرنه المنت سرد می‌ماند!
        if (SSR_1 != PIN_NC && SSR_1 >= 0) {
            if (fw_info_is_out_manually_on(5) || get_var_test_ssr()) {
                digitalWrite(SSR_1, ON);
            } else if (g_device_locked || get_var_thermocouple_error() || on_ms == 0) {
                digitalWrite(SSR_1, OFF);
            } else if (on_ms >= 1000) {
                digitalWrite(SSR_1, ON);
            } else {
                uint32_t window_pos = millis() % 1000;
                if (window_pos < on_ms) {
                    digitalWrite(SSR_1, ON);
                } else {
                    digitalWrite(SSR_1, OFF);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
void buzzer_task(void *param)
{
    uint32_t notificationValue;

    while (true)
    {
        // Wait indefinitely for a notification
        if (xTaskNotifyWait(
                0x00,                 // Don't clear bits on entry
                ULONG_MAX,            // Clear all bits on exit
                &notificationValue,   // Store notification value
                portMAX_DELAY))       // Wait forever
        {
            uint32_t nextValue = 0;
            while (xTaskNotifyWait(0x00, ULONG_MAX, &nextValue, 0)) {
                int cur_priority = (notificationValue == BUZZER_CRITICAL) ? 6 :
                                   (notificationValue == BUZZER_ERROR) ? 5 :
                                   (notificationValue == BUZZER_IMPORTANT) ? 4 :
                                   (notificationValue == BUZZER_STARTUP) ? 3 :
                                   (notificationValue == BUZZER_CONFIRM) ? 2 : 1;
                int next_priority = (nextValue == BUZZER_CRITICAL) ? 6 :
                                    (nextValue == BUZZER_ERROR) ? 5 :
                                    (nextValue == BUZZER_IMPORTANT) ? 4 :
                                    (nextValue == BUZZER_STARTUP) ? 3 :
                                    (nextValue == BUZZER_CONFIRM) ? 2 : 1;
                if (next_priority >= cur_priority) notificationValue = nextValue;
            }
            if (!get_var_setting_general_speaker())
                continue;
            // آیتم ۲: در صفحه‌های ثبت‌نامِ اولِ بوت (نام شرکت/رمز مدیر) هیچ بوقی (لمس یا هشدارِ پس‌زمینه‌ی دما/سنسور) پخش نشود.
            if (g_setup_overlay_active)
                continue;

            switch (notificationValue)
            {
                case BUZZER_ACTION: {
                    static uint32_t s_last_action_ms = 0;
                    uint32_t now_ms = millis();
                    if ((uint32_t)(now_ms - s_last_action_ms) < 95U) {
                        break; // حذف بوق‌های مکرر/تکراری تاچ و کلید در کمتر از ۹۵ میلی‌ثانیه
                    }
                    s_last_action_ms = now_ms;
                    buzzer(1, 18, 0); // بوق لمسی کوتاه، گرد و فوق‌العاده نرم ۱۸ میلی‌ثانیه‌ای با رزونانس کامل
                    break;
                }

                case BUZZER_CONFIRM:
                    buzzer(1, 22, 0); // پینگِ دوگانهٔ تأیید صعودی و شفاف (۲۲ms و ۲۸ms)
                    vTaskDelay(pdMS_TO_TICKS(50));
                    buzzer(1, 28, 0);
                    break;

                case BUZZER_ERROR:
                    buzzer(1, 75, 0); // هشدار دوگانهٔ تمیز و قاطع بدون صدای ناهنجار
                    vTaskDelay(pdMS_TO_TICKS(65));
                    buzzer(1, 75, 0);
                    break;

                case BUZZER_STARTUP:
                    buzzer(1, 15, 0); // آکوردِ صعودی و دلنشین استارتاپ دستگاه (۴ مرحله‌ای)
                    vTaskDelay(pdMS_TO_TICKS(35));
                    buzzer(1, 22, 0);
                    vTaskDelay(pdMS_TO_TICKS(40));
                    buzzer(1, 30, 0);
                    vTaskDelay(pdMS_TO_TICKS(45));
                    buzzer(1, 45, 0);
                    break;

                case BUZZER_IMPORTANT:
                    buzzer(1, 20, 0); // سه‌گانهٔ هارمونیک اعلام ذخیره و سناریوهای مهم
                    vTaskDelay(pdMS_TO_TICKS(45));
                    buzzer(1, 20, 0);
                    vTaskDelay(pdMS_TO_TICKS(45));
                    buzzer(1, 25, 0);
                    break;

                case BUZZER_CRITICAL:
                    buzzer(1, 110, 0); // آلارم خطای بحرانی صنعتی
                    vTaskDelay(pdMS_TO_TICKS(80));
                    buzzer(1, 110, 0);
                    vTaskDelay(pdMS_TO_TICKS(80));
                    buzzer(1, 110, 0);
                    break;

                default:
                    break;
            }
        }
    }
}

// ---- اسکنِ نسخه‌ی محصولات برای منوی نصبِ مدیر (مثل Config) ----
static const char *host = "ota.haffler-infi.ir";
#ifndef OTA_BASE_URL
#define OTA_BASE_URL "http://ota.haffler-infi.ir"
#endif

// جدول جامع کاندیداهای نام مدل روی سرور OTA (سازگار با استاندارد ۸ دستگاه + کانفیگ)
static const char *g_card_model_candidates[9][6] = {
    { "upvc%2Fwelding-single-head", "welding-single-head", "upvc%2FWelding_single_head", "Welding_single_head", "upvc%2Fwelding_single_head", NULL }, // 0 جوش تک‌سر
    { "upvc%2Fwelding-double-head", "welding-double-head", "upvc%2FWelding_double_head", "Welding_double_head", "upvc%2Fwelding_double_head", NULL }, // 1 جوش دوسر
    { "upvc%2Fdigital-cutter-single-head", "digital-cutter-single-head", "upvc%2FCutter_single_head", "Cutter_single_head", "upvc%2Fdigital-cutter_single_head", NULL }, // 2 برش تک‌سر
    { "upvc%2Fdigital-cutter-double-head", "digital-cutter-double-head", "upvc%2FCutter_double_head", "Cutter_double_head", "upvc%2Fdigital-cutter_double_head", NULL }, // 3 برش دوسر
    { "upvc%2Fgas-filling", "gas-filling", "upvc%2FGas-filling", "Gas-filling", "upvc%2Fgas_filling", NULL },                         // 4 تزریق گاز شیشه
    { "upvc%2Fcorner-cleaning", "corner-cleaning", "upvc%2FCorner-cleaning", "Corner-cleaning", "upvc%2Fcorner_cleaning", NULL },             // 5 گوشه‌تمیزکن
    { "upvc%2Fcopy-router", "copy-router", "upvc%2FCopy-router", "Copy-router", "upvc%2Fcopy_router", NULL },                         // 6 کپی فرز
    { "upvc%2Faluminum-punching", "aluminum-punching", "upvc%2FAluminum-punching", "Aluminum-punching", "upvc%2Faluminum_punching", NULL },       // 7 پانچ آلومینیوم
    { "upvc%2Fconfig-test", "config-test", "upvc%2FConfig-test", "Config-test", "upvc%2Fconfig_test", NULL }                          // 8 برنامه‌ی Config
};

#define INST_COUNT 2
static const char *g_inst_models[INST_COUNT] = {
    "upvc%2Fwelding-single-head",
    "upvc%2Fconfig-test",
};
static const char *g_inst_models_fallback[INST_COUNT][6] = {
    { "upvc%2Fwelding-single-head", "welding-single-head", "upvc%2FWelding_single_head", "Welding_single_head", "upvc%2Fwelding_single_head", NULL },
    { "upvc%2Fconfig-test", "config-test", "upvc%2FConfig-test", "Config-test", "upvc%2Fconfig_test", NULL },
};

static String ota_url(const String &model, const char *file)
{
    String m = model;
    m.trim();
    while (m.startsWith("/")) m.remove(0, 1);
    while (m.endsWith("/")) m.remove(m.length() - 1);
    if (m.startsWith("plc_hmi/")) m.remove(0, 8);
    if (m.indexOf("%2F") < 0 && m.indexOf("%2f") < 0 && m.indexOf('/') >= 0) {
        m.replace("/", "%2F");
    }
    return "http://" + String(host) + "/" + m + "%2F" + file;
}

static String ota_build_url(const String &modelCand, const char *file) {
    return ota_url(modelCand, file);
}

// مقایسه‌ی نسخه‌ی معنایی (major.minor.patch):
// +1: سرور جدیدتر است (server > current)
//  0: نسخه‌ها یکسان هستند (server == current)
// -1: سرور قدیمی‌تر است (server < current)
static int versionCompare(String server, String current)
{
    server.trim();
    current.trim();
    if (server.length() && (server[0] == 'v' || server[0] == 'V')) server = server.substring(1);
    if (current.length() && (current[0] == 'v' || current[0] == 'V')) current = current.substring(1);
    int a[3] = {0, 0, 0}, b[3] = {0, 0, 0};
    sscanf(server.c_str(), "%d.%d.%d", &a[0], &a[1], &a[2]);
    sscanf(current.c_str(), "%d.%d.%d", &b[0], &b[1], &b[2]);
    for (int i = 0; i < 3; i++) {
        if (a[i] != b[i]) return (a[i] > b[i]) ? 1 : -1;
    }
    return 0;
}

// مقایسه‌ی نسخه‌ی معنایی (major.minor.patch). true اگر server از current جدیدتر باشد.
static bool versionNewer(String server, String current)
{
    return versionCompare(server, current) > 0;
}

// سازندهٔ URLِ OTA با تشخیصِ خودکارِ هاستِ جدید/قدیم
static String extract_version_from_lastfile(const String &body) {
    String b = body;
    b.trim();
    if (b.length() == 0) return "";

    // 1. Search for code=... or display=... or version=... in INI format
    int idx = b.indexOf("code=");
    if (idx < 0) idx = b.indexOf("display=");
    if (idx < 0) idx = b.indexOf("version=");

    if (idx >= 0) {
        int eq = b.indexOf('=', idx);
        if (eq >= 0) {
            int lineEnd = b.indexOf('\n', eq);
            if (lineEnd < 0) lineEnd = b.indexOf('\r', eq);
            if (lineEnd < 0) lineEnd = b.length();
            String ver = b.substring(eq + 1, lineEnd);
            ver.trim();
            if (ver.startsWith("v") || ver.startsWith("V")) ver = ver.substring(1);
            ver.trim();
            if (ver.length() > 0 && ver[0] >= '0' && ver[0] <= '9') {
                return ver;
            }
        }
    }

    // 2. Fallback line by line
    int start = 0;
    while (start < b.length()) {
        int end = b.indexOf('\n', start);
        if (end < 0) end = b.length();
        String line = b.substring(start, end);
        line.trim();
        if (line.startsWith("v") || line.startsWith("V")) line = line.substring(1);
        line.trim();
        if (line.length() > 0 && line[0] >= '0' && line[0] <= '9') {
            int spaceIdx = line.indexOf(' ');
            if (spaceIdx > 0) line = line.substring(0, spaceIdx);
            return line;
        }
        start = end + 1;
    }

    return "";
}

// فراداده‌ی استخراج‌شده از lastfile استاندارد (فقط Stringها — بدون تخصیصِ اضافه)
struct LastfileInfo {
    String ver;             // display= یا code= یا خطِ قدیمیِ تخت
    String bdate, btime;    // build_date= / build_time=
    String desc[12];        // تا ۱۲ خطِ توضیحِ تغییرات عمومی (هر خط ≤۱۶۰ بایت)
    int    desc_count = 0;
    String desc_fa[12];     // توضیحات فارسی
    int    desc_fa_count = 0;
    String desc_en[12];     // توضیحات انگلیسی
    int    desc_en_count = 0;
    String desc_tr[12];     // توضیحات ترکی
    int    desc_tr_count = 0;
    String sha256;          // هش SHA-256 فایل فریمور از [binary]
    String prog_name;       // name= از [program]
};

// برگشت به مرزِ معتبرِ UTF-8 برای برشِ طولِ ثابت
static int cfg_utf8_cut(const String &s, int cut)
{
    while (cut > 0 && ((uint8_t)s[cut] & 0xC0) == 0x80) cut--;
    return cut;
}

// بررسی نویسه‌های فارسی / عربی در UTF-8
static bool cfg_is_persian_utf8(const String &s)
{
    int len = s.length();
    for (int i = 0; i < len - 1; i++) {
        uint8_t c1 = (uint8_t)s[i];
        uint8_t c2 = (uint8_t)s[i + 1];
        if ((c1 == 0xD8 && c2 >= 0x80) || (c1 == 0xD9 && c2 <= 0xBF) ||
            (c1 == 0xDA && c2 >= 0x80) || (c1 == 0xDB && c2 <= 0xBF)) {
            return true;
        }
    }
    return false;
}

// بررسی نویسه‌های خاص ترکی استانبولی در UTF-8
static bool cfg_is_turkish_utf8(const String &s)
{
    int len = s.length();
    for (int i = 0; i < len - 1; i++) {
        uint8_t c1 = (uint8_t)s[i];
        uint8_t c2 = (uint8_t)s[i + 1];
        // ç/Ç (C3 A7 / C3 87), ö/Ö (C3 B6 / C3 96), ü/Ü (C3 BC / C3 9C), ğ/Ğ (C4 9F / C4 9E), ı/İ (C4 B1 / C4 B0), ş/Ş (C5 9F / C5 9E)
        if ((c1 == 0xC3 && (c2 == 0xA7 || c2 == 0x87 || c2 == 0xB6 || c2 == 0x96 || c2 == 0xBC || c2 == 0x9C)) ||
            (c1 == 0xC4 && (c2 == 0x9F || c2 == 0x9E || c2 == 0xB1 || c2 == 0xB0)) ||
            (c1 == 0xC5 && (c2 == 0x9F || c2 == 0x9E))) {
            return true;
        }
    }
    return false;
}

// تجزیه‌ی lastfile استاندارد (UTF-8؛ سکشن‌های [name]، خطوطِ key=value، کامنتِ #، CRLF-safe)
static void config_parse_lastfile(const String &body, LastfileInfo &out)
{
    String section = "";
    String legacy = "";
    bool sawSection = false;
    bool legacyDone = false;
    int len = body.length();
    int from = 0;
    while (from <= len)
    {
        int nl = body.indexOf('\n', from);
        if (nl < 0) nl = len;
        String line = body.substring(from, nl);
        from = nl + 1;
        int cr = line.indexOf('\r');
        if (cr >= 0) line.remove(cr);
        line.trim();
        if (!line.length() || line[0] == '#') continue;

        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            section = line.substring(1, line.length() - 1);
            section.toLowerCase();
            sawSection = true;
            continue;
        }

        int eq = line.indexOf('=');
        if (eq <= 0)
        {
            if (!sawSection && !legacyDone) { legacy = line; legacyDone = true; }
            continue;
        }
        String key = line.substring(0, eq);
        key.trim(); key.toLowerCase();
        String val = line.substring(eq + 1);
        val.trim();
        if (!val.length()) continue;

        if (section == "program")
        {
            if (key == "name") out.prog_name = val;
        }
        else if (section == "version")
        {
            if (key == "display") out.ver = val;
            else if (key == "code" && !out.ver.length()) out.ver = val;
            else if (key == "build_date") out.bdate = val;
            else if (key == "build_time") out.btime = val;
        }
        else if (section.startsWith("desc") || section.startsWith("changelog"))
        {
            if (val.length() > 160) val = val.substring(0, cfg_utf8_cut(val, 160));

            bool isFaSec = (section == "description_fa" || section == "changelog_fa" || section == "description.fa");
            bool isEnSec = (section == "description_en" || section == "changelog_en" || section == "description.en");
            bool isTrSec = (section == "description_tr" || section == "changelog_tr" || section == "description.tr");

            if (isFaSec)
            {
                if (out.desc_fa_count < 12) out.desc_fa[out.desc_fa_count++] = val;
            }
            else if (isEnSec)
            {
                if (out.desc_en_count < 12) out.desc_en[out.desc_en_count++] = val;
            }
            else if (isTrSec)
            {
                if (out.desc_tr_count < 12) out.desc_tr[out.desc_tr_count++] = val;
            }
            else
            {
                if (out.desc_count < 12) out.desc[out.desc_count++] = val;

                bool isFa = (key.startsWith("fa") || key.indexOf("_fa") >= 0 || cfg_is_persian_utf8(val));
                bool isTr = (key.startsWith("tr") || key.indexOf("_tr") >= 0 || cfg_is_turkish_utf8(val));
                bool isEn = (key.startsWith("en") || key.indexOf("_en") >= 0 || (!isFa && !isTr));

                if (isFa && out.desc_fa_count < 12) out.desc_fa[out.desc_fa_count++] = val;
                if (isTr && out.desc_tr_count < 12) out.desc_tr[out.desc_tr_count++] = val;
                if (isEn && out.desc_en_count < 12) out.desc_en[out.desc_en_count++] = val;
            }
        }
        else if (section == "binary")
        {
            if (key == "sha256") out.sha256 = val;
        }
    }
    if (!out.ver.length()) out.ver = legacy;
}

static char g_inst_version[INST_COUNT][16];
static bool g_inst_available[INST_COUNT];
static bool g_inst_scan_error[INST_COUNT];
static char g_inst_bdate[INST_COUNT][16];
static char g_inst_btime[INST_COUNT][16];
static char g_inst_desc[INST_COUNT][12][160];
static int  g_inst_desc_cnt[INST_COUNT];
static char g_inst_desc_fa[INST_COUNT][12][160];
static int  g_inst_desc_fa_cnt[INST_COUNT];
static char g_inst_desc_en[INST_COUNT][12][160];
static int  g_inst_desc_en_cnt[INST_COUNT];
static char g_inst_desc_tr[INST_COUNT][12][160];
static int  g_inst_desc_tr_cnt[INST_COUNT];
static char g_inst_prog_name[INST_COUNT][80];
static char g_inst_sha256[INST_COUNT][68];

extern "C" const char *installer_sha256(int idx) {
    if (idx >= 0 && idx < INST_COUNT && g_inst_sha256[idx][0]) return g_inst_sha256[idx];
    return "";
}

extern "C" const char *installer_prog_name(int idx) {
    if (idx >= 0 && idx < INST_COUNT && g_inst_prog_name[idx][0]) return g_inst_prog_name[idx];
    return (idx == 1) ? "Haffler Config Launcher (Installer/Test)" : "Haffler Single Head UPVC Welder";
}
extern "C" const char *installer_version_str(int idx) {
    if (idx >= 0 && idx < INST_COUNT && g_inst_version[idx][0]) return g_inst_version[idx];
    return "";
}
extern "C" const char *installer_build_date(int idx) {
    if (idx >= 0 && idx < INST_COUNT && g_inst_bdate[idx][0]) return g_inst_bdate[idx];
    return "";
}
extern "C" const char *installer_build_time(int idx) {
    if (idx >= 0 && idx < INST_COUNT && g_inst_btime[idx][0]) return g_inst_btime[idx];
    return "";
}
extern "C" const char *installer_desc_line(int idx, int line) {
    if (idx >= 0 && idx < INST_COUNT && line >= 0 && line < g_inst_desc_cnt[idx]) return g_inst_desc[idx][line];
    return "";
}
extern "C" int installer_desc_count(int idx) {
    if (idx >= 0 && idx < INST_COUNT && g_inst_desc_cnt[idx] > 0) return g_inst_desc_cnt[idx];
    return 0;
}
extern "C" const char *installer_desc_line_lang(int idx, int lang, int line) {
    if (idx < 0 || idx >= INST_COUNT || line < 0) return "";
    if (lang == 0 && g_inst_desc_fa_cnt[idx] > 0 && line < g_inst_desc_fa_cnt[idx]) {
        return g_inst_desc_fa[idx][line];
    }
    if (lang == 1 && g_inst_desc_en_cnt[idx] > 0 && line < g_inst_desc_en_cnt[idx]) {
        return g_inst_desc_en[idx][line];
    }
    if (lang == 2 && g_inst_desc_tr_cnt[idx] > 0 && line < g_inst_desc_tr_cnt[idx]) {
        return g_inst_desc_tr[idx][line];
    }
    if (line < g_inst_desc_cnt[idx]) return g_inst_desc[idx][line];
    return "";
}
extern "C" int installer_desc_count_lang(int idx, int lang) {
    if (idx < 0 || idx >= INST_COUNT) return 0;
    if (lang == 0 && g_inst_desc_fa_cnt[idx] > 0) return g_inst_desc_fa_cnt[idx];
    if (lang == 1 && g_inst_desc_en_cnt[idx] > 0) return g_inst_desc_en_cnt[idx];
    if (lang == 2 && g_inst_desc_tr_cnt[idx] > 0) return g_inst_desc_tr_cnt[idx];
    return g_inst_desc_cnt[idx];
}
extern "C" int installer_compare_version_to_current(int idx) {
    if (idx != 0) return 1; // فقط برای کارت 0 (جوش تک‌سر) مقایسه نسخه اعمال می‌شود
    if (idx < 0 || idx >= INST_COUNT || !g_inst_available[idx]) return 0;
    const char *sver = g_inst_version[idx];
    if (!sver || !sver[0]) return 0;
    if (strcmp(sver, "READY") == 0) return 1;
    return versionCompare(String(sver), String(VERSION));
}

#define INST_PINMAP_MAX 1200
static char g_inst_pinmap[INST_COUNT][INST_PINMAP_MAX];
static bool g_inst_pinmap_ok[INST_COUNT];
static volatile bool g_inst_scan_request = false;
static volatile bool g_inst_scan_done = false;
// true تا وقتی run_installer_scan در حالِ درخواستِ HTTPS است؛ sync_webapp_state در این مدت
// وضعیتِ WiFi را تغییر نمی‌دهد تا اتصالِ در حالِ استفاده قطع نشود (رفعِ reason=8 وسطِ اسکن).
static volatile bool g_inst_scan_busy = false;
static TaskHandle_t g_inst_scan_task_handle = NULL;

// mutexِ مشترکِ رادیوی WiFi (تعریفِ واقعی در main.cpp، بعد از include این هدر). درخواست‌های HTTPSِ
// installer و پاکسازیِ WiFi (sync_webapp_state) هر دو این قفل را می‌گیرند تا هرگز همزمان اجرا نشوند —
// وگرنه قطعِ WiFi وسطِ handshakeِ TLS باعثِ کرشِ null در esp_wifi_internal_free_rx_buffer می‌شد.
extern SemaphoreHandle_t xMutex_Wifi;

static bool inst_http_get_text(const String &url, String &out, int timeoutMs)
{
    bool locked = (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(timeoutMs + 4000)) == pdTRUE);
    uint32_t t0 = millis();
    bool ok = false;
    {
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setConnectTimeout(timeoutMs);
        http.setTimeout(timeoutMs);
        http.setReuse(false);
        int code = -1;
        int sec = (timeoutMs + 999) / 1000;
        if (sec < 5) sec = 5;
        if (url.startsWith("http://")) {
            WiFiClient pc;
            pc.setTimeout(sec);
            pc.setNoDelay(true);
            if (http.begin(pc, url)) {
                http.addHeader("User-Agent", "Haffler-OTA/1.5");
                http.addHeader("Accept", "*/*");
                http.addHeader("Connection", "close");
                code = http.GET();
                if (code == 200) {
                    out = http.getString();
                    out.trim();
                    ok = true;
                }
                http.end();
            }
            if (!ok && (url.indexOf("%2F") >= 0 || url.indexOf("%2f") >= 0)) {
                String slashUrl = url;
                slashUrl.replace("%2F", "/");
                slashUrl.replace("%2f", "/");
                WiFiClient pc2;
                pc2.setTimeout(sec);
                pc2.setNoDelay(true);
                if (http.begin(pc2, slashUrl)) {
                    http.addHeader("User-Agent", "Haffler-OTA/1.5");
                    http.addHeader("Accept", "*/*");
                    http.addHeader("Connection", "close");
                    code = http.GET();
                    if (code == 200) {
                        out = http.getString();
                        out.trim();
                        ok = true;
                    }
                    http.end();
                }
            }
        } else {
            WiFiClientSecure sc;
            sc.setInsecure();
            sc.setTimeout(sec);
            sc.setNoDelay(true);
            if (http.begin(sc, url)) {
                code = http.GET();
                if (code == 200) {
                    out = http.getString();
                    out.trim();
                    ok = true;
                }
                http.end();
            }
            if (!ok) {
                String plainUrl = url;
                plainUrl.replace("https://", "http://");
                WiFiClient pc;
                pc.setTimeout(sec);
                pc.setNoDelay(true);
                if (http.begin(pc, plainUrl)) {
                    code = http.GET();
                    if (code == 200) {
                        out = http.getString();
                        out.trim();
                        ok = true;
                    }
                    http.end();
                }
            }
        }
        Serial.printf("[INST-HTTP] GET code=%d dt=%lums heapFree=%u url=%s\n",
                      code, (unsigned long)(millis() - t0), (unsigned)ESP.getFreeHeap(), url.c_str());
    }
    if (locked) xSemaphoreGive(xMutex_Wifi);
    return ok;
}

static bool inst_http_head_ok(const String &url, int timeoutMs)
{
    bool locked = (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(timeoutMs + 4000)) == pdTRUE);
    int code = -1;
    {
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setConnectTimeout(timeoutMs);
        http.setTimeout(timeoutMs);
        http.setReuse(false);
        int sec = (timeoutMs + 999) / 1000;
        if (sec < 5) sec = 5;
        if (url.startsWith("http://")) {
            WiFiClient pc;
            pc.setTimeout(sec);
            pc.setNoDelay(true);
            if (http.begin(pc, url)) {
                http.addHeader("User-Agent", "Haffler-OTA/1.5");
                http.addHeader("Accept", "*/*");
                http.addHeader("Connection", "close");
                code = http.sendRequest("HEAD");
                if (code == 405 || code == 403) code = http.GET();
                http.end();
            }
            if (code != 200 && (url.indexOf("%2F") >= 0 || url.indexOf("%2f") >= 0)) {
                String slashUrl = url;
                slashUrl.replace("%2F", "/");
                slashUrl.replace("%2f", "/");
                WiFiClient pc2;
                pc2.setTimeout(sec);
                pc2.setNoDelay(true);
                if (http.begin(pc2, slashUrl)) {
                    http.addHeader("User-Agent", "Haffler-OTA/1.5");
                    http.addHeader("Accept", "*/*");
                    http.addHeader("Connection", "close");
                    code = http.sendRequest("HEAD");
                    if (code == 405 || code == 403) code = http.GET();
                    http.end();
                }
            }
        } else {
            WiFiClientSecure sc;
            sc.setInsecure();
            sc.setTimeout(sec);
            sc.setNoDelay(true);
            if (http.begin(sc, url)) {
                code = http.sendRequest("HEAD");
                if (code == 405 || code == 403) code = http.GET();
                http.end();
            }
            if (code != 200) {
                String plainUrl = url;
                plainUrl.replace("https://", "http://");
                WiFiClient pc;
                pc.setTimeout(sec);
                pc.setNoDelay(true);
                if (http.begin(pc, plainUrl)) {
                    code = http.sendRequest("HEAD");
                    if (code == 405 || code == 403) code = http.GET();
                    http.end();
                }
            }
        }
    }
    if (locked) xSemaphoreGive(xMutex_Wifi);
    return code == 200;
}

static bool inst_http_get_text_sess(WiFiClientSecure &sc, HTTPClient &http, const String &url, String &out, int timeoutMs)
{
    (void)sc; (void)http;
    return inst_http_get_text(url, out, timeoutMs);
}

static bool inst_http_head_ok_sess(WiFiClientSecure &sc, HTTPClient &http, const String &url, int timeoutMs)
{
    (void)sc; (void)http;
    return inst_http_head_ok(url, timeoutMs);
}

static void run_installer_scan(void);
static void installer_scan_task(void *param)
{
    (void)param;
    if (WiFi.status() != WL_CONNECTED)
    {
        String ss = storage.getWifiSsid();
        if (ss.length() > 0)
        {
            wifi_sta_connect_full(ss.c_str(), storage.getWifiPassword().c_str(), -1, 20, NULL);
            if (WiFi.status() == WL_CONNECTED) wifi_on_connected_hooks();
        }
    }
    run_installer_scan();
    g_inst_scan_task_handle = NULL;
    vTaskDelete(NULL);
}

extern "C" const char *installer_model(int i)     { return (i >= 0 && i < INST_COUNT) ? g_inst_models[i] : ""; }
extern "C" const char *installer_version(int i)   { return (i >= 0 && i < INST_COUNT) ? g_inst_version[i] : ""; }
extern "C" bool        installer_available(int i) { return (i >= 0 && i < INST_COUNT) ? g_inst_available[i] : false; }
extern "C" bool        installer_error(int i)     { return (i >= 0 && i < INST_COUNT) ? g_inst_scan_error[i] : false; }
extern "C" bool        installer_scan_done(void)  { return g_inst_scan_done; }
extern "C" bool        installer_scan_busy(void)  { return g_inst_scan_busy; }

// نسخه/مدل قفل‌شده هنگام کلیک نصب — تا نصب به lastfile دوباره وابسته نباشد
static char g_ota_armed_ver[12] = {0};
static volatile bool g_ota_armed_ver_ok = false;
static volatile bool g_ota_armed_force_bin = false; // READY از HEAD: بدون چک نسخه، مستقیم bin
static volatile int g_ota_install_idx = -1;

extern "C" void installer_prepare_install(int idx)
{
    g_ota_armed_ver_ok = false;
    g_ota_armed_force_bin = false;
    g_ota_armed_ver[0] = '\0';
    g_ota_install_idx = idx;
    if (idx < 0 || idx >= INST_COUNT) {
        Serial.printf("[INSTALL] prepare idx=%d invalid\n", idx);
        return;
    }
    const char *v = g_inst_version[idx];
    Serial.printf("[INSTALL] prepare idx=%d avail=%d ver='%s' scanDone=%d\n",
                  idx, (int)g_inst_available[idx], v ? v : "", (int)g_inst_scan_done);
    if (!g_inst_available[idx] || !v || !v[0]) return;
    if (strcmp(v, "READY") == 0) {
        g_ota_armed_force_bin = true;
        g_ota_armed_ver_ok = true;
        strncpy(g_ota_armed_ver, "READY", sizeof(g_ota_armed_ver) - 1);
        return;
    }
    const char *verDigits = (v[0] == 'v' || v[0] == 'V') ? (v + 1) : v;
    if (verDigits[0] >= '0' && verDigits[0] <= '9' && strcmp(verDigits, "0.0.0") != 0) {
        strncpy(g_ota_armed_ver, v, sizeof(g_ota_armed_ver) - 1);
        g_ota_armed_ver[sizeof(g_ota_armed_ver) - 1] = '\0';
        g_ota_armed_ver_ok = true;
    }
}
extern "C" const char *installer_pinmap_text(int idx) {
    if (idx < 0 || idx >= INST_COUNT || !g_inst_pinmap_ok[idx] || !g_inst_pinmap[idx][0]) return NULL;
    return g_inst_pinmap[idx];
}
extern "C" void        installer_request_scan(void) {
    if (g_inst_scan_task_handle != NULL) return;   // اسکن در جریان است — done را دوباره false نکن
    g_inst_scan_done = false;
    g_inst_scan_request = false;
    BaseType_t ok = xTaskCreatePinnedToCore(installer_scan_task, "InstScan", 8192, NULL, 1, &g_inst_scan_task_handle, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(installer_scan_task, "InstScan", 6144, NULL, 1, &g_inst_scan_task_handle, 0);
    }
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(installer_scan_task, "InstScan", 4096, NULL, 1, &g_inst_scan_task_handle, 0);
    }
    if (ok != pdPASS) {
        Serial.println("[INST-SCAN] Failed to create InstScan task (DRAM OOM)");
        g_inst_scan_task_handle = NULL;
        for (int i = 0; i < INST_COUNT; i++) {
            g_inst_available[i] = false;
            g_inst_scan_error[i] = true;
        }
        g_inst_scan_busy = false;
        g_inst_scan_done = true;
    }
}

// ===== بررسی لینک سفارشی روی سرور (lastfile و binary .bin) =====
static volatile bool g_link_check_done = false;
static volatile int g_link_check_status = 0; // 0: none, 1: error, 2: info found, 3: bin found
static char g_link_check_result_buf[256] = { 0 };
static TaskHandle_t g_link_check_task_handle = NULL;
static char s_link_check_target_url[256] = { 0 };

extern "C" bool link_check_is_done(void) { return g_link_check_done; }
extern "C" void link_check_clear_done(void) { g_link_check_done = false; }
extern "C" const char *link_check_get_result(void) { return g_link_check_result_buf; }
extern "C" int link_check_get_status(void) { return g_link_check_status; }

static void link_check_worker_task(void *param) {
    (void)param;
    if (WiFi.status() != WL_CONNECTED) {
        g_link_check_status = 1;
        strncpy(g_link_check_result_buf, "خطا: ارتباط وای‌فای برقرار نیست!", sizeof(g_link_check_result_buf) - 1);
        g_link_check_result_buf[sizeof(g_link_check_result_buf) - 1] = '\0';
        g_link_check_done = true;
        g_link_check_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    String targetCheckUrl = String(s_link_check_target_url);
    String urlNoBin = targetCheckUrl;
    if (urlNoBin.endsWith(".bin") || urlNoBin.endsWith(".BIN")) {
        urlNoBin = urlNoBin.substring(0, urlNoBin.length() - 4);
    }
    String urlBin = urlNoBin + ".bin";

    WiFiClient client;
    HTTPClient http;

    // ۱. بررسی فایل اطلاعات (بدون پسوند .bin)
    http.begin(client, urlNoBin);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setConnectTimeout(2000);
    http.setTimeout(3000);
    http.addHeader("User-Agent", "Haffler-OTA/1.5");
    http.addHeader("Accept", "*/*");
    http.addHeader("Connection", "close");
    int code = http.GET();
    if (code != 200 && (urlNoBin.indexOf("%2F") >= 0 || urlNoBin.indexOf("%2f") >= 0)) {
        String sUrl = urlNoBin;
        sUrl.replace("%2F", "/");
        sUrl.replace("%2f", "/");
        http.end();
        http.begin(client, sUrl);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setConnectTimeout(2000);
        http.setTimeout(3000);
        http.addHeader("User-Agent", "Haffler-OTA/1.5");
        http.addHeader("Accept", "*/*");
        http.addHeader("Connection", "close");
        code = http.GET();
    }

    if (code == 200) {
        String body = http.getString();
        http.end();
        LastfileInfo info;
        config_parse_lastfile(body, info);
        String infoText = "";
        if (info.ver.length() > 0) {
            infoText = "اطلاعات برنامه: نسخه " + info.ver;
            if (info.bdate.length() > 0) infoText += " (" + info.bdate + ")";
            String desc = "";
            if (info.desc_count > 0 && info.desc[0].length() > 0) {
                desc = info.desc[0];
                desc.trim();
                if (desc.startsWith("- ") || desc.startsWith("-")) {
                    desc = desc.substring(desc.startsWith("- ") ? 2 : 1);
                    desc.trim();
                }
            }
            if (desc.length() > 0) {
                infoText += "\n" + desc;
            } else {
                infoText += "\nلینک تایید شد (جهت دانلود کلید نصب را بزنید)";
            }
        } else {
            body.trim();
            if (body.length() > 0 && body.length() < 70) {
                infoText = "اطلاعات: " + body + "\nلینک تایید شد";
            } else {
                infoText = "اطلاعات برنامه در سرور یافت شد\nلینک تایید شد (جهت دانلود کلید نصب را بزنید)";
            }
        }
        g_link_check_status = 2;
        strncpy(g_link_check_result_buf, infoText.c_str(), sizeof(g_link_check_result_buf) - 1);
        g_link_check_result_buf[sizeof(g_link_check_result_buf) - 1] = '\0';
        g_link_check_done = true;
    } else {
        // ۲. اگر فایل اطلاعات نبود، بررسی فایل باینری (.bin)
        http.end();
        http.begin(client, urlBin);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setConnectTimeout(2000);
        http.setTimeout(3000);
        http.addHeader("User-Agent", "Haffler-OTA/1.5");
        http.addHeader("Accept", "*/*");
        http.addHeader("Range", "bytes=0-128");
        http.addHeader("Connection", "close");
        int binCode = http.GET();
        if (binCode != 200 && binCode != 206 && (urlBin.indexOf("%2F") >= 0 || urlBin.indexOf("%2f") >= 0)) {
            String sUrl = urlBin;
            sUrl.replace("%2F", "/");
            sUrl.replace("%2f", "/");
            http.end();
            http.begin(client, sUrl);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http.setConnectTimeout(2000);
            http.setTimeout(3000);
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Range", "bytes=0-128");
            http.addHeader("Connection", "close");
            binCode = http.GET();
        }
        http.end();

        if (binCode == 200 || binCode == 206) {
            g_link_check_status = 3;
            strncpy(g_link_check_result_buf, "لینک تایید شد (فایل باینری در سرور موجود است)\nجهت دانلود کلید نصب را بزنید", sizeof(g_link_check_result_buf) - 1);
        } else {
            g_link_check_status = 1;
            strncpy(g_link_check_result_buf, "فایل در سرور یافت نشد (کد ۴۰۴)\nلطفاً آدرس لینک را بررسی فرمایید", sizeof(g_link_check_result_buf) - 1);
        }
        g_link_check_result_buf[sizeof(g_link_check_result_buf) - 1] = '\0';
        g_link_check_done = true;
    }
    g_link_check_task_handle = NULL;
    vTaskDelete(NULL);
}

extern "C" void action_link_check_start(const char *url) {
    if (g_link_check_task_handle != NULL) return;
    if (!url || !url[0]) return;
    strncpy(s_link_check_target_url, url, sizeof(s_link_check_target_url) - 1);
    s_link_check_target_url[sizeof(s_link_check_target_url) - 1] = '\0';
    g_link_check_done = false;
    g_link_check_status = 0;
    BaseType_t ok = xTaskCreatePinnedToCore(link_check_worker_task, "LnkChk", 6144, NULL, 2, &g_link_check_task_handle, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(link_check_worker_task, "LnkChk", 4096, NULL, 2, &g_link_check_task_handle, 0);
    }
    if (ok != pdPASS) {
        g_link_check_task_handle = NULL;
        g_link_check_status = 1;
        strncpy(g_link_check_result_buf, "خطا در ایجاد تسک شبکه", sizeof(g_link_check_result_buf) - 1);
        g_link_check_result_buf[sizeof(g_link_check_result_buf) - 1] = '\0';
        g_link_check_done = true;
    }
}


// نسخه‌ی هر محصول را از هاست می‌خواند؛ available = نسخه‌ی معتبر و غیرِ 0.0.0 (یعنی کدش روی هاست هست)
static void run_installer_scan(void)
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[INST-SCAN] WiFi not connected, skipping scan (using local cache)");
        for (int i = 0; i < INST_COUNT; i++) {
            g_inst_available[i] = false;
            g_inst_scan_error[i] = true;
        }
        g_inst_scan_busy = false;
        g_inst_scan_done = true;
        return;
    }
    g_inst_scan_busy = true;
    const int verTimeoutMs = 2500;
    const int headTimeoutMs = 2500;
    const int pinTimeoutMs = 2500;

    auto apply_lf_metadata = [&](int card_idx, const LastfileInfo &lf, const String &cleanVer) {
        strncpy(g_inst_version[card_idx], cleanVer.c_str(), sizeof(g_inst_version[card_idx]) - 1);
        g_inst_version[card_idx][sizeof(g_inst_version[card_idx]) - 1] = '\0';
        g_inst_available[card_idx] = true;
        if (lf.prog_name.length() > 0) {
            strncpy(g_inst_prog_name[card_idx], lf.prog_name.c_str(), sizeof(g_inst_prog_name[card_idx]) - 1);
            g_inst_prog_name[card_idx][sizeof(g_inst_prog_name[card_idx]) - 1] = '\0';
        }
        strncpy(g_inst_bdate[card_idx], lf.bdate.c_str(), sizeof(g_inst_bdate[card_idx]) - 1);
        g_inst_bdate[card_idx][sizeof(g_inst_bdate[card_idx]) - 1] = '\0';
        strncpy(g_inst_btime[card_idx], lf.btime.c_str(), sizeof(g_inst_btime[card_idx]) - 1);
        g_inst_btime[card_idx][sizeof(g_inst_btime[card_idx]) - 1] = '\0';

        g_inst_desc_cnt[card_idx] = lf.desc_count;
        for (int d = 0; d < lf.desc_count; d++) {
            strncpy(g_inst_desc[card_idx][d], lf.desc[d].c_str(), sizeof(g_inst_desc[card_idx][d]) - 1);
            g_inst_desc[card_idx][d][sizeof(g_inst_desc[card_idx][d]) - 1] = '\0';
        }
        g_inst_desc_fa_cnt[card_idx] = lf.desc_fa_count;
        for (int d = 0; d < lf.desc_fa_count; d++) {
            strncpy(g_inst_desc_fa[card_idx][d], lf.desc_fa[d].c_str(), sizeof(g_inst_desc_fa[card_idx][d]) - 1);
            g_inst_desc_fa[card_idx][d][sizeof(g_inst_desc_fa[card_idx][d]) - 1] = '\0';
        }
        g_inst_desc_en_cnt[card_idx] = lf.desc_en_count;
        for (int d = 0; d < lf.desc_en_count; d++) {
            strncpy(g_inst_desc_en[card_idx][d], lf.desc_en[d].c_str(), sizeof(g_inst_desc_en[card_idx][d]) - 1);
            g_inst_desc_en[card_idx][d][sizeof(g_inst_desc_en[card_idx][d]) - 1] = '\0';
        }
        g_inst_desc_tr_cnt[card_idx] = lf.desc_tr_count;
        for (int d = 0; d < lf.desc_tr_count; d++) {
            strncpy(g_inst_desc_tr[card_idx][d], lf.desc_tr[d].c_str(), sizeof(g_inst_desc_tr[card_idx][d]) - 1);
            g_inst_desc_tr[card_idx][d][sizeof(g_inst_desc_tr[card_idx][d]) - 1] = '\0';
        }
        if (lf.sha256.length() > 0) {
            strncpy(g_inst_sha256[card_idx], lf.sha256.c_str(), sizeof(g_inst_sha256[card_idx]) - 1);
            g_inst_sha256[card_idx][sizeof(g_inst_sha256[card_idx]) - 1] = '\0';
        } else {
            g_inst_sha256[card_idx][0] = '\0';
        }
    };

    for (int i = 0; i < INST_COUNT; i++) {
        g_inst_available[i] = false;
        g_inst_scan_error[i] = false;
        g_inst_version[i][0] = '\0';
        g_inst_bdate[i][0] = '\0';
        g_inst_btime[i][0] = '\0';
        g_inst_sha256[i][0] = '\0';
        g_inst_desc_cnt[i] = 0;
        g_inst_desc_fa_cnt[i] = 0;
        g_inst_desc_en_cnt[i] = 0;
        g_inst_desc_tr_cnt[i] = 0;
        for (int d = 0; d < 12; d++) {
            g_inst_desc[i][d][0] = '\0';
            g_inst_desc_fa[i][d][0] = '\0';
            g_inst_desc_en[i][d][0] = '\0';
            g_inst_desc_tr[i][d][0] = '\0';
        }
        g_inst_pinmap_ok[i] = false;
        g_inst_pinmap[i][0] = '\0';
        String v;
        bool found = false;
        // مرحله ۱: کاندیدِ اصلی (ArvanCloud جدید: upvc%2F...)
        for (int attempt = 0; attempt < 2 && !found; attempt++) {
            String cand = (attempt == 0) ? String(g_inst_models[i]) : String(g_inst_models_fallback[i][0] ? g_inst_models_fallback[i][0] : "");
            if (cand.length() == 0) continue;
            String lastUrl = ota_build_url(cand, "lastfile");
            String binUrlNew = ota_build_url(cand, "firmware");
            String binUrlOld = ota_build_url(cand, "firmware.bin");
            if (inst_http_get_text(lastUrl, v, verTimeoutMs)) {
                LastfileInfo lf;
                config_parse_lastfile(v, lf);
                String cleanVer = lf.ver.length() > 0 ? lf.ver : extract_version_from_lastfile(v);
                if (cleanVer.length() > 0) {
                    apply_lf_metadata(i, lf, cleanVer);
                    found = true;
                }
            }
            if (!found) {
                vTaskDelay(pdMS_TO_TICKS(120));
                if (inst_http_get_text(lastUrl, v, verTimeoutMs)) {
                    LastfileInfo lf;
                    config_parse_lastfile(v, lf);
                    String cleanVer = lf.ver.length() > 0 ? lf.ver : extract_version_from_lastfile(v);
                    if (cleanVer.length() > 0) {
                        apply_lf_metadata(i, lf, cleanVer);
                        found = true;
                    }
                }
            }
            if (!found && (inst_http_head_ok(binUrlOld, headTimeoutMs) || inst_http_head_ok(binUrlNew, headTimeoutMs))) {
                strncpy(g_inst_version[i], "READY", sizeof(g_inst_version[i]) - 1);
                g_inst_version[i][sizeof(g_inst_version[i]) - 1] = '\0';
                g_inst_available[i] = true;
                found = true;
            }
            if (found) break;
            // فال‌بک دوم: مدلِ قدیمیِ دوم
            if (g_inst_models_fallback[i][1]) {
                String cand2 = String(g_inst_models_fallback[i][1]);
                String lastUrl2 = ota_build_url(cand2, "lastfile");
                String bin2New = ota_build_url(cand2, "firmware");
                String bin2Old = ota_build_url(cand2, "firmware.bin");
                if (inst_http_get_text(lastUrl2, v, verTimeoutMs)) {
                    LastfileInfo lf;
                    config_parse_lastfile(v, lf);
                    String cleanVer = lf.ver.length() > 0 ? lf.ver : extract_version_from_lastfile(v);
                    if (cleanVer.length() > 0) {
                        apply_lf_metadata(i, lf, cleanVer);
                        found = true;
                    }
                }
                if (!found && (inst_http_head_ok(bin2Old, headTimeoutMs) || inst_http_head_ok(bin2New, headTimeoutMs))) {
                    strncpy(g_inst_version[i], "READY", sizeof(g_inst_version[i]) - 1);
                    g_inst_version[i][sizeof(g_inst_version[i]) - 1] = '\0';
                    g_inst_available[i] = true;
                    found = true;
                }
            }
        }
        if (!found) {
            g_inst_available[i] = false;
            g_inst_scan_error[i] = true;
        }
        // دریافت pinmap برای تمام کارت‌ها (اولویت با محتوای دانلودشده lastfile)
        if (i < INST_COUNT) {
            if (v.length() > 0) {
                int inPos = v.indexOf("[inputs]");
                if (inPos >= 0) {
                    int instPos = v.indexOf("[install]", inPos);
                    int endPos = (instPos > inPos) ? instPos : v.length();
                    String pinText = v.substring(inPos, endPos);
                    pinText.trim();
                    if (pinText.length() > 0 && pinText.length() < INST_PINMAP_MAX) {
                        strncpy(g_inst_pinmap[i], pinText.c_str(), INST_PINMAP_MAX - 1);
                        g_inst_pinmap[i][INST_PINMAP_MAX - 1] = '\0';
                        g_inst_pinmap_ok[i] = true;
                        Serial.printf("[INST-SCAN] extracted pinmap from lastfile (%d bytes)\n", (int)pinText.length());
                    }
                }
            }
            // فقط در صورت عدم وجود در lastfile، از سرور استعلام کن
            if (!g_inst_pinmap_ok[i]) {
                String pinBody;
                String cand = String(g_inst_models[i]);
                if (cand.length() > 0) {
                    String pinUrl = ota_build_url(cand, "pinmap.txt");
                    if (inst_http_get_text(pinUrl, pinBody, pinTimeoutMs) &&
                        pinBody.length() > 0 && pinBody.length() < INST_PINMAP_MAX) {
                        pinBody.trim();
                        strncpy(g_inst_pinmap[i], pinBody.c_str(), INST_PINMAP_MAX - 1);
                        g_inst_pinmap[i][INST_PINMAP_MAX - 1] = '\0';
                        g_inst_pinmap_ok[i] = true;
                    }
                }
            }
        }
        Serial.printf("[INST-SCAN] %s -> ver=%s avail=%d pinmap=%d\n",
                      g_inst_models[i], g_inst_version[i], (int)g_inst_available[i], (int)g_inst_pinmap_ok[i]);
    }
    g_inst_scan_busy = false;
    g_inst_scan_done = true;
}

// پس از اتصالِ وای‌فای، خودکار چکِ آپدیتِ جوش تک‌سر انجام شود (وقتی از دکمه‌ی بروزرسانی آمده‌ایم)
static volatile bool g_pending_self_check = false;
// امنیت: وای‌فایِ ذخیره‌شده ممکن است *قبل* از تاییدِ رمزِ مدیر وصل شود (همان دلیلی که ناوبریِ صفحهٔ
// کارت‌های نصب هم به آن گیر داشت) — پس self_update_check() (که واقعاً دانلود/فلش می‌کند) بلافاصله
// بعدِ اتصال صدا زده نمی‌شود؛ فقط این پرچم ست می‌شود و تا وقتی رمز تایید نشود، در حلقهٔ اصلیِ ota_task
// هر بار دوباره چک می‌شود (پایین‌تر).
static volatile bool g_pending_self_check_wifi_ok = false;
extern "C" void request_self_update_after_wifi(void) { g_pending_self_check = true; }
// آیا رمزِ وای‌فای از قبل ذخیره شده؟ (اگر بله، نیازی به اسکن/صفحه‌ی وای‌فا نیست)
extern "C" bool has_saved_wifi(void) { return storage.getWifiSsid().length() > 0; }

void ota_task(void *param)
{
    bool ota_boot_scan_done = false;
    while (true)
    {
        if (g_ota_exit_request) {
            vTaskDelete(NULL);
        }
        // اسکنِ همگام — جایگزین async که روی ESP32-S3 گیر می‌کند یا با WiFi.begin تداخل دارد
        if (!g_self_update_session && (g_wifi_ota_scan_pending || g_wifi_ota_rescan_request ||
            (g_wifi_rescan_request && get_var_ota_enable()))) {
            if (g_wifi_ota_scan_pending) g_wifi_ota_scan_pending = false;
            g_wifi_ota_rescan_request = false;
            g_wifi_rescan_request = false;
            wifi_perform_sync_scan_and_update_ui();
            ota_boot_scan_done = true;
        }

        // مسیر قدیمیِ async (isSearching + scanComplete) حذف شد:
        // همهٔ اسکن‌ها sync هستند و وسط collect اگر این مسیر scanDelete می‌زد → هنگ/ریست.
        // g_wifi_direct_scan_active تا پایان collect در wifi_perform روشن می‌ماند.

        // امنیت: چکِ نسخه/نصبِ خودکار فقط بعد از تاییدِ رمزِ مدیر انجام می‌شود — وای‌فای می‌تواند زودتر
        // وصل شده باشد (پرچمِ بالا)، ولی تا رمز درست وارد نشود اینجا کاری انجام نمی‌شود؛ هر دورِ حلقه
        // دوباره چک می‌شود، پس همین که رمز تایید شد بی‌درنگ (بدونِ نیاز به اتصالِ دوباره) اجرا می‌شود.
        // آیتم: شرط !g_self_update_session اضافه شد — اگر کاربر از صفحهٔ بروزرسانیِ اطلاعات (درجا) آمده،
        // self_update_task خودش اتصال/چک را مدیریت می‌کند؛ این مسیر ota_task فقط برای مسیرِ F5-بوت است.
        if (g_pending_self_check_wifi_ok && WiFi.status() == WL_CONNECTED
            && get_var_is_admin_password_verifyed() && !g_self_update_session)
        {
            g_pending_self_check_wifi_ok = false;
            g_pending_self_check = false;
            self_update_check();   // چکِ نسخهٔ همین دستگاه + UI درجا
        }

        /////////////////////////////////////////
        bool should_connect = false;
        String selected_wifi_local = "";
        String wifi_password_local = "";
        if (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(100)))
        {
            should_connect = connectingToWifi;
            selected_wifi_local = selectedWifi;
            wifi_password_local = wifiPassword;
            xSemaphoreGive(xMutex_Wifi);
        }
        else
        {
            should_connect = connectingToWifi;
            selected_wifi_local = selectedWifi;
            wifi_password_local = wifiPassword;
        }

        if (should_connect && !g_self_update_session)
        {
            if (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(100)))
            {
                connectingToWifi = false;
                xSemaphoreGive(xMutex_Wifi);
            }
            else
            {
                connectingToWifi = false;
            }

            // فقط اسنپ‌شات دکمه (selected_wifi_local / wifi_password_local) —
            // LVGL را از ota_task لمس نکن (thread-unsafe؛ رمز هات‌اسپات خراب می‌شد)

            set_var_waitig_panle_state(true);
            set_var_connection_error_label("");
            WiFi.scanDelete();
            g_wifi_direct_scan_active = false;
            isSearching = false;
            bool ok = wifi_sta_connect_full(selected_wifi_local.c_str(), wifi_password_local.c_str(),
                                  -1, 28, NULL);
            set_var_waitig_panle_state(false);

            wifi_conncting_counter = 0;
            Serial.printf("[WIFI-CONN] ota result ok=%d status=%d\n", (int)ok, (int)WiFi.status());
            if (ok && WiFi.status() == WL_CONNECTED)
            {
                WiFi.setAutoReconnect(true);

                Serial.println("Connected and ready to nex screen!");
                storage.setWifiCredentials(selected_wifi_local, wifi_password_local);
                storage.saveWifi(selected_wifi_local, wifi_password_local);  // مخزنِ چندتایی برای اتصالِ خودکارِ بعدی
                storage.setWifiOn(true);

                // eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);

                // set_var_is_wifi_connected(true);

                wifi_on_connected_hooks();

                // اگر از دکمه‌ی «بروزرسانی» آمده‌ایم، حالا که وصل شد خودکار نسخه‌ی جوش تک‌سر را چک کن —
                // امنیت: خودِ self_update_check() اینجا صدا زده نمی‌شود (ممکن است رمزِ مدیر هنوز تایید
                // نشده باشد)؛ فقط «وای‌فا آماده است» ثبت می‌شود و پایین‌ترِ همین حلقه، بعد از تاییدِ رمز، انجام می‌شود.
                if (g_pending_self_check) {
                    g_pending_self_check_wifi_ok = true;
                } else if (get_var_ota_enable()) {
                    // آیتم: مسیرِ F5-بوت (نه بروزرسانیِ درجا) — برو به صفحهٔ کارت‌های نصب. ناوبریِ واقعی
                    // در نخِ LVGL انجام می‌شود (اینجا ota_task است، نمی‌تواند مستقیم LVGL را لمس کند).
                    g_ota_go_framware_request = true;
                }

                // delay(100);
                // lv_obj_del(screen_wifi);
            }
            else
            {
                WiFi.setAutoReconnect(false);
                WiFi.disconnect(false, false);
                set_var_waitig_panle_state(false);
                char fail_msg[160];
                int lang = eez_flow_get_language_index();
                if (lang == 1) {
                    snprintf(fail_msg, sizeof(fail_msg), "Failed to connect to %s", selected_wifi_local.c_str());
                } else if (lang == 2) {
                    snprintf(fail_msg, sizeof(fail_msg), "%s agina baglanilamadi", selected_wifi_local.c_str());
                } else {
                    snprintf(fail_msg, sizeof(fail_msg), "متصل شدن به وای‌فای %s با مشکل مواجه شد", selected_wifi_local.c_str());
                }
                set_var_connection_error_label(fail_msg);
                ui_request_toast_colored(fail_msg, 0xffd32f2f, 4000);
            }
        }

        if (g_inst_scan_request)
        {
            g_inst_scan_request = false;
            // برای کامل بودنِ مسیرِ مدیر: اگر وای‌فای وصل نیست، با رمزِ ذخیره‌شده وصل شو، بعد اسکن کن
            if (WiFi.status() != WL_CONNECTED)
            {
                String ss = storage.getWifiSsid();
                String pw = storage.getWifiPassword();
                if (ss.length() > 0)
                {
                    wifi_sta_begin_network(ss.c_str(), pw.c_str(), -1);
                    int c = 0;
                    while (WiFi.status() != WL_CONNECTED && c < 30) { vTaskDelay(pdMS_TO_TICKS(500)); c++; }
                    if (WiFi.status() == WL_CONNECTED) wifi_on_connected_hooks();
                }
            }
            run_installer_scan();
        }

        if (ota_download)
        {
            ota_download = false;
            char model_copy[96];
            strncpy(model_copy, ota_download_model, sizeof(model_copy) - 1);
            model_copy[sizeof(model_copy) - 1] = '\0';
            Serial.printf("[OTA] queued model='%s' armed=%d forceBin=%d ver='%s'\n",
                          model_copy, (int)g_ota_armed_ver_ok, (int)g_ota_armed_force_bin, g_ota_armed_ver);
            download_firmware(String(model_copy));
        }

        if (ota_custom_download)
        {
            ota_custom_download = false;
            char url_copy[256];
            strncpy(url_copy, ota_custom_url_buf, sizeof(url_copy) - 1);
            url_copy[sizeof(url_copy) - 1] = '\0';
            Serial.printf("[OTA] queued custom url='%s'\n", url_copy);
            download_firmware_from_url(String(url_copy));
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static bool shablon_is_moving = false;
static bool shablon_is_free = true;
static uint32_t shablon_time = 0;

static bool table_is_moving = false;
static bool table_is_free = true;
static uint32_t table_time = 0;

static bool element_is_moving = false;
static bool element_is_free = true;
static uint32_t element_time = 0;

static bool press_left_is_moving = false;
static bool press_left_is_free = true;
static uint32_t press_left_time = 0;
static bool press_right_is_moving = false;
static bool press_right_is_free = true;
static uint32_t press_right_time = 0;
static uint32_t josh_time = 0;
static uint32_t cool_time = 0;
static int32_t  g_weld_sample_sum = 0;
static int32_t  g_weld_sample_count = 0;
static int      g_weld_min_temp = 999;
static int      g_weld_max_temp = 0;

static bool zarbeh1_is_free = true;
static uint32_t zarbeh1_time = 0;
static bool zarbeh2_is_free = true;
static uint32_t zarbeh2_time = 0;
static uint8_t completed_weld_cycles_in_frame = 0;
// زمان‌سنجی چرخه برای پیغام پایان جوش (#۳): از اولین استارت تا پایان آخرین مرحله
static uint32_t weld_cycle_start_ms = 0;
static bool weld_cycle_timing = false;

static uint32_t pedal_in_limit_time = 0;
static bool pedal_in_limit_press = false;
static bool auto_restart_pending = false;
static uint32_t auto_restart_ready_ms = 0;
static uint32_t auto_restart_last_beep_ms = 0;
static uint32_t final_jack_start_ms = 0;
static bool final_jack_started = false;

static void start_cycle_progress();

static void apply_test_outputs() {
    digitalWrite(SSR_1, get_var_test_ssr() ? ON : OFF);
    digitalWrite(SHABLON, get_var_test_shablon() ? ON : OFF);
    digitalWrite(TABLE, get_var_test_safhe() ? ON : OFF);
    digitalWrite(PRESS_RIGHT, get_var_test_press1() ? ON : OFF);
    digitalWrite(PRESS_LEFT, get_var_test_press2() ? ON : OFF);
    digitalWrite(ELEMENT, get_var_test_josh() ? ON : OFF);
    digitalWrite(ZARBEH1, get_var_test_zarbe1() ? ON : OFF);
    digitalWrite(ZARBEH2, get_var_test_zarbe2() ? ON : OFF);

    set_var_shablon_out_display(get_var_test_shablon());
    set_var_safhe_out_display(get_var_test_safhe());
    set_var_press1_out_display(get_var_test_press1());
    set_var_press2_out_display(get_var_test_press2());
    set_var_josh_out_display(get_var_test_josh());
    set_var_zarbe1_out_display(get_var_test_zarbe1());
    set_var_zarbe2_out_display(get_var_test_zarbe2());
}

static void finish_cycle_and_schedule_auto_start() {
    set_var_progress_vlaue(100);
    vTaskDelay(pdMS_TO_TICKS(80));
    set_var_step_process(false);
    set_var_step_number(0);
    final_jack_started = false;
    // پیغام پایان جوش (#۳): زمان کل (اولین استارت تا اینجا) + تعداد فریم را به UI بده
    if (weld_cycle_timing) {
        g_weld_done_elapsed_ms = (unsigned long)(millis() - weld_cycle_start_ms);
        g_weld_done_frames = (int)get_var_frame_numbers();
        g_weld_done_request = 1;
        weld_cycle_timing = false;
    }
    if (get_var_setting_general_automatic_start()) {
        auto_restart_pending = true;
        auto_restart_ready_ms = millis() + 8000UL;
        auto_restart_last_beep_ms = 0;
    } else {
        set_var_progress_vlaue(0);
    }
}

static void process_auto_restart() {
    if (!auto_restart_pending) {
        return;
    }
    if (get_var_test_mode() || !get_var_cycel_started()) {
        auto_restart_pending = false;
        set_var_progress_vlaue(0);
        return;
    }

    uint32_t now = millis();
    int32_t remaining_ms = (int32_t)(auto_restart_ready_ms - now);
    if (remaining_ms > 2000) {
        int32_t green = ((remaining_ms - 2000) * 100) / 6000;
        if (green < 1) green = 1;
        if (green > 100) green = 100;
        set_var_progress_vlaue(green);
    } else if (remaining_ms <= 2000 && now < auto_restart_ready_ms) {
        set_var_progress_vlaue(-1);
        if (auto_restart_last_beep_ms == 0 || now - auto_restart_last_beep_ms >= 900UL) {
            xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
            auto_restart_last_beep_ms = now;
        }
    }

    if ((int32_t)(now - auto_restart_ready_ms) >= 0) {
        auto_restart_pending = false;
        start_cycle_progress();
        set_var_curent_pedal_status(true);
        set_var_step_process(true);
    }
}

static void start_cycle_progress() {
    auto_restart_pending = false;
    set_var_progress_vlaue(1);
}

static uint32_t cycle_step_duration_ms(int32_t step) {
    switch (step) {
        case 0: return (uint32_t)get_var_setting_jack_shablon_raft();
        case 1: return (uint32_t)get_var_setting_jack_safhe_raft();
        case 2: return (uint32_t)get_var_setting_jack_press_right_raft();
        case 3: return (uint32_t)get_var_setting_jack_press_left_raft();
        case 4: return (uint32_t)get_var_setting_jack_safhe_bargasht();
        case 5: return (uint32_t)get_var_setting_jack_shablon_bargasht();
        case 6: return (uint32_t)get_var_setting_jack_josh_raft();
        case 7: return (uint32_t)get_var_setting_jack_safhe_raft();
        case 8: return (uint32_t)get_var_welding_time() * 1000UL;
        case 9: return (uint32_t)get_var_setting_jack_safhe_bargasht();
        case 10: return (uint32_t)get_var_setting_jack_josh_bargasht();
        case 11: return (uint32_t)get_var_setting_jack_safhe_raft();
        case 12: return (uint32_t)get_var_cooling_time() * 1000UL;
        case 13: return get_var_setting_general_final_jack() ? (2000UL + free_moving_time) : free_moving_time;
        default: return 0;
    }
}

static uint32_t cycle_step_elapsed_ms(int32_t step) {
    switch (step) {
        case 0: return shablon_is_moving ? (millis() - shablon_time) : 0;
        case 1: return table_is_moving ? (millis() - table_time) : 0;
        case 2: return press_right_is_moving ? (millis() - press_right_time) : 0;
        case 3: return press_left_is_moving ? (millis() - press_left_time) : 0;
        case 4: return table_is_moving ? (millis() - table_time) : 0;
        case 5: return shablon_is_moving ? (millis() - shablon_time) : 0;
        case 6: return element_is_moving ? (millis() - element_time) : 0;
        case 7: return table_is_moving ? (millis() - table_time) : 0;
        case 8: return get_var_josh_is_working() ? (millis() - josh_time) : 0;
        case 9: return table_is_moving ? (millis() - table_time) : 0;
        case 10: return element_is_moving ? (millis() - element_time) : 0;
        case 11: return table_is_moving ? (millis() - table_time) : 0;
        case 12: return get_var_cool_is_working() ? (millis() - cool_time) : 0;
        case 13: return final_jack_started ? (millis() - final_jack_start_ms) : 0;
        default: return 0;
    }
}

// پُرشدنِ پلکانیِ پراگرس با سنسور (طبق اسپک اپراتور): به‌جای زمان، با مایلستون‌های سنسور پر می‌شود.
// استارت=گام۱ (۰.۳۳) ، سنسورِ مبدأ خاموش=گام۲ (۰.۶۶) ، سنسورِ مقصد دیده شد=گام نهایی (۱.۰)
// میز فقط یک سنسور دارد → دو گام (۰.۵ ، ۱.۰). خروجیِ منفی = از منطقِ زمانی استفاده شود.
static float sensor_step_fraction(int32_t step) {
    if (!get_var_setting_admin_jack_sensor()) return -1.0f;
    switch (step) {
        case 0:   // شابلون بسته شدن: مبدأ=باز ، مقصد=بسته
            if (jack_sensor_arrived(MUX_FAM_SHABLON, true))  return 1.0f;
            if (!jack_sensor_arrived(MUX_FAM_SHABLON, false)) return 0.66f;
            return 0.33f;
        case 5:   // شابلون باز شدن: مبدأ=بسته ، مقصد=باز
            if (jack_sensor_arrived(MUX_FAM_SHABLON, false)) return 1.0f;
            if (!jack_sensor_arrived(MUX_FAM_SHABLON, true))  return 0.66f;
            return 0.33f;
        case 6:   // المنت بسته شدن
            if (jack_sensor_arrived(MUX_FAM_ELEMENT, true))  return 1.0f;
            if (!jack_sensor_arrived(MUX_FAM_ELEMENT, false)) return 0.66f;
            return 0.33f;
        case 10:  // المنت باز شدن
            if (jack_sensor_arrived(MUX_FAM_ELEMENT, false)) return 1.0f;
            if (!jack_sensor_arrived(MUX_FAM_ELEMENT, true))  return 0.66f;
            return 0.33f;
        case 4: case 9:  // میز باز شدن — یک سنسور → دو گام (استارت ، سنسورِ باز روشن)
            if (jack_sensor_arrived(MUX_FAM_TABLE, false)) return 1.0f;
            return 0.5f;
        case 1: case 7: case 11:  // میز بسته شدن — یک سنسور → دو گام (استارت ، سنسورِ باز خاموش)
            if (!jack_sensor_arrived(MUX_FAM_TABLE, false)) return 1.0f;
            return 0.5f;
        default:  return -1.0f; // سایر مراحل (بدون سنسور) → زمانی
    }
}

static int sensor_lamp_percent(int32_t step) {
    switch (step) {
        case 0:
            if (jack_sensor_arrived(MUX_FAM_SHABLON, true)) return 100;
            if (!jack_sensor_arrived(MUX_FAM_SHABLON, false)) return 66;
            return 33;
        case 5:
            if (jack_sensor_arrived(MUX_FAM_SHABLON, false)) return 0;
            if (!jack_sensor_arrived(MUX_FAM_SHABLON, true)) return 33;
            return 66;
        case 6:
            if (jack_sensor_arrived(MUX_FAM_ELEMENT, true)) return 100;
            if (!jack_sensor_arrived(MUX_FAM_ELEMENT, false)) return 66;
            return 33;
        case 10:
            if (jack_sensor_arrived(MUX_FAM_ELEMENT, false)) return 0;
            if (!jack_sensor_arrived(MUX_FAM_ELEMENT, true)) return 33;
            return 66;
        case 4: case 9:  // میز باز شدن → خالی شدن (مثل بقیه: باز=خالی)
            return jack_sensor_arrived(MUX_FAM_TABLE, false) ? 0 : 50;
        case 1: case 7: case 11:  // میز بسته شدن → پُر شدن (مثل بقیه: بسته=پُر)
            return jack_sensor_arrived(MUX_FAM_TABLE, false) ? 50 : 100;
        default:
            return -1;
    }
}

// ===== لامپِ سیگنالِ سنسوری =====
// اگر باز و بسته روی *یک* GPIO باشند → همان منطق ADC قبلی (I0↔I1 و … دست‌نخورده می‌ماند).
// اگر روی دو GPIO جدا باشند (مثل I0↔I3) → هر نقش از silk همان ردیف خوانده می‌شود.
static int lamp2_pct_from_adc(int adc, bool closed_cmd) {
    bool atClosed = adc > SENSOR_BAND_CLOSED;
    bool atOpen   = (adc >= SENSOR_BAND_OPEN_LO && adc <= SENSOR_BAND_OPEN_HI);
    if (closed_cmd) {
        if (atClosed) return 100;
        if (atOpen)   return 33;
        return 66;
    } else {
        if (atOpen)   return 0;
        if (atClosed) return 66;
        return 33;
    }
}
static int lamp1_pct_from_adc(int adc, bool closed_cmd) {
    bool atOpen = (adc >= SENSOR_BAND_OPEN_LO && adc <= SENSOR_BAND_OPEN_HI);
    if (closed_cmd) return atOpen ? 50 : 100;
    else            return atOpen ? 0  : 50;
}
static int lamp2_pct_from_roles(bool at_open, bool at_closed, bool closed_cmd) {
    if (closed_cmd) {
        if (at_closed) return 100;
        if (at_open)   return 33;
        return 66;
    }
    if (at_open)   return 0;
    if (at_closed) return 66;
    return 33;
}
static int lamp2_pct_for_family(int fam, bool closed_cmd) {
    int rOpen = mux_input_row_for_fam(fam, false);
    int rClose = mux_input_row_for_fam(fam, true);
    int pOpen = mux_input_row_pin(rOpen);
    int pClose = (rClose >= 0) ? mux_input_row_pin(rClose) : PIN_NC;
    bool liveO = mux_input_row_live(rOpen);
    bool liveC = (rClose >= 0) && mux_input_row_live(rClose);

    // هر دو روی یک سیم → رفتار قبلیِ اثبات‌شده
    if (liveO && liveC && pOpen != PIN_NC && pOpen == pClose)
        return lamp2_pct_from_adc(sensor_adc_avg(pOpen), closed_cmd);
    // فقط یکی زنده → همان سیم را مثل قبل با ADC بخوان
    if (liveO && !liveC && pOpen != PIN_NC)
        return lamp2_pct_from_adc(sensor_adc_avg(pOpen), closed_cmd);
    if (liveC && !liveO && pClose != PIN_NC)
        return lamp2_pct_from_adc(sensor_adc_avg(pClose), closed_cmd);
    // دو سیم جدا (ورودی مستقلِ ضرب‌دری) → هر ردیف با silk خودش
    if (liveO || liveC) {
        bool at_open = liveO && jack_sensor_arrived(fam, false);
        bool at_closed = liveC && jack_sensor_arrived(fam, true);
        return lamp2_pct_from_roles(at_open, at_closed, closed_cmd);
    }
    return closed_cmd ? 33 : 0;
}
static int lamp1_pct_for_table(bool closed_cmd) {
    int r = mux_input_row_for_fam(MUX_FAM_TABLE, false);
    int p = mux_input_row_pin(r);
    if (p != PIN_NC)
        return lamp1_pct_from_adc(sensor_adc_avg(p), closed_cmd);
    // fallback ترمینال فیزیکی
    return lamp1_pct_from_adc(sensor_adc_avg(DEFAULT_SENSOR_TABLE), closed_cmd);
}
static void update_sensor_lamps() {
    if (!get_var_setting_admin_jack_sensor()) {
        g_lamp_pct_shablon = g_lamp_pct_table = g_lamp_pct_element = -1;
        return;
    }
    if (g_force_lamps_off) {
        g_lamp_pct_shablon = g_lamp_pct_table = g_lamp_pct_element = 0;
        return;
    }
    static uint32_t lamp_ms = 0;
    if (millis() - lamp_ms < 60) return;
    lamp_ms = millis();
    // raw برای لاگ = ترمینال فیزیکی ثابت
    g_sens_raw_shablon = sensor_adc_avg(DEFAULT_SENSOR_SHABLON);
    g_sens_raw_element = sensor_adc_avg(DEFAULT_SENSOR_ELEMENT);
    g_sens_raw_table   = sensor_adc_avg(DEFAULT_SENSOR_TABLE);
    g_lamp_pct_shablon = lamp2_pct_for_family(MUX_FAM_SHABLON, get_var_shablon_out_display());
    g_lamp_pct_element = lamp2_pct_for_family(MUX_FAM_ELEMENT, get_var_josh_out_display());
    g_lamp_pct_table   = lamp1_pct_for_table(get_var_safhe_out_display());
}

static void update_cycle_progress() {
    bool active = get_var_step_process() || get_var_step_number() > 0 || get_var_josh_is_working() || get_var_cool_is_working();
    if (!active) {
        set_var_progress_vlaue(0);
        g_sensor_lamp_step = -1;
        return;
    }

    int32_t step = get_var_step_number();
    if (step < 0) step = 0;
    if (step > 13) step = 13;

    uint32_t total = 0;
    uint32_t done = 0;
    for (int32_t i = 0; i <= 13; ++i) {
        uint32_t duration = cycle_step_duration_ms(i);
        total += duration;
        if (i < step) {
            done += duration;
        }
    }

    uint32_t current_duration = cycle_step_duration_ms(step);
    uint32_t current_elapsed;
    float sf = sensor_step_fraction(step);
    if (sf >= 0.0f) {
        // پُرشدنِ پلکانی با سنسور
        current_elapsed = (uint32_t)(sf * (float)current_duration);
        // برای پُرشدنِ پلکانیِ لامپِ همان جک (در صفحه اعمال می‌شود)
        g_sensor_lamp_step = (int)step;
        g_sensor_lamp_pct  = sensor_lamp_percent(step);
    } else {
        // منطقِ زمانیِ قبلی
        current_elapsed = cycle_step_elapsed_ms(step);
        if (current_elapsed > current_duration) current_elapsed = current_duration;
        g_sensor_lamp_step = -1;
    }
    done += current_elapsed;

    int32_t progress = total > 0 ? (int32_t)((done * 100UL) / total) : 1;
    if (progress < 1) progress = 1;
    if (step == 13 && progress >= 100) progress = 99;
    if (progress > 100) progress = 100;
    set_var_progress_vlaue(progress);
}

static void final_jack_delay_with_progress(uint32_t wait_ms) {
    uint32_t start_ms = millis();
    while (millis() - start_ms < wait_ms) {
        update_cycle_progress();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void step_by_step_jack(){
    // ضربه‌ها ابتدا آزاد شوند
    if(!zarbeh1_is_free){
        digitalWrite(ZARBEH1, OFF);
        set_var_zarbe1_out_display(false);
        zarbeh1_is_free=true;
    }
    if(!zarbeh2_is_free){
        digitalWrite(ZARBEH2, OFF);
        set_var_zarbe2_out_display(false);
        zarbeh2_is_free=true;
    }
    // پلکانی (طبق اسپک اپراتور): پرس اول رها → تایم باز شدن → میز باز شود →
    // تایم کاملِ باز شدن میز → پرس دوم رها → پایان. به این ترتیب همه‌ی جک‌ها پلکانی رها می‌شوند.
    if (!press_right_is_free){
        digitalWrite(PRESS_RIGHT, OFF);
        set_var_press1_out_display(false);
        press_right_is_free=true;
        final_jack_delay_with_progress(1000); // تایم باز شدنِ پرس اول
    }
    if (!table_is_free){
        digitalWrite(TABLE, OFF);
        set_var_safhe_out_display(false);
        table_is_free=true;
        table_is_moving=false;
        if (get_var_setting_admin_jack_sensor()) {
            // در حالت سنسور: پرسِ آخر تا سنسورِ بازِ میز فعال نشده رها نشود (تایم‌اوت ۷ث → خطا)
            uint32_t t0 = millis();
            while (!jack_sensor_arrived(MUX_FAM_TABLE, false)) {
                update_cycle_progress();
                update_sensor_lamps();
                if (millis() - t0 > SENSOR_TIMEOUT_MS) {
                    g_sensor_err_id = 3; g_sensor_err_request = 1; // سنسور بازِ میز فرمان نداد
                    emergency_operation();
                    return;
                }
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        } else {
            final_jack_delay_with_progress((uint32_t)get_var_setting_jack_safhe_bargasht()); // تایم کاملِ باز شدن میز
        }
    }
    if (!press_left_is_free){
        digitalWrite(PRESS_LEFT, OFF);
        set_var_press2_out_display(false);
        press_left_is_free=true;
        final_jack_delay_with_progress(free_moving_time);
    }
    finish_cycle_and_schedule_auto_start();
}
void fast_jack(){
    // غیرپلکانی (طبق اسپک اپراتور): اول دو پرس با هم باز می‌شوند،
    // سپس حدود ۸۰ms بعد همه‌ی جک‌ها رها می‌شوند (حتی در حالت استوپ).
    if (!press_right_is_free){
        digitalWrite(PRESS_RIGHT, OFF);
        set_var_press1_out_display(false);
        press_right_is_free=true;
    }
    if(!press_left_is_free){
        digitalWrite(PRESS_LEFT, OFF);
        set_var_press2_out_display(false);
        press_left_is_free=true;
    }
    final_jack_delay_with_progress(80); // ~۸۰ میلی‌ثانیه فاصله
    if(!zarbeh1_is_free){
        digitalWrite(ZARBEH1, OFF);
        set_var_zarbe1_out_display(false);
        zarbeh1_is_free=true;
    }
    if(!zarbeh2_is_free){
        digitalWrite(ZARBEH2, OFF);
        set_var_zarbe2_out_display(false);
        zarbeh2_is_free=true;
    }
    if (!table_is_free){
        digitalWrite(TABLE, OFF);
        set_var_safhe_out_display(false);
        table_is_free=true;
        table_is_moving=false;
        if (get_var_setting_admin_jack_sensor()) {
            // در حالت سنسور: اطمینان از بازگشت کامل صفحه قبل از اتمام چرخه با تایم‌اوت ۷ ثانیه‌ای
            uint32_t t0 = millis();
            while (!jack_sensor_arrived(MUX_FAM_TABLE, false)) {
                update_cycle_progress();
                update_sensor_lamps();
                if (millis() - t0 > SENSOR_TIMEOUT_MS) {
                    g_sensor_err_id = 3; g_sensor_err_request = 1; // سنسور بازِ میز فرمان نداد
                    emergency_operation();
                    return;
                }
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        } else {
            final_jack_delay_with_progress(80);
        }
    }
    // اطمینان از رها شدنِ بقیه‌ی جک‌ها حتی در حالت استوپ
    if (!shablon_is_free){
        digitalWrite(SHABLON, OFF);
        set_var_shablon_out_display(false);
        shablon_is_free=true;
    }
    if (!element_is_free){
        digitalWrite(ELEMENT, OFF);
        set_var_josh_out_display(false);
        element_is_free=true;
    }
    finish_cycle_and_schedule_auto_start();
}

void pedal_operation(){
    if (warranty_is_expired_and_locked()) {
        g_warranty_expired_dialog_request = true;
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        return;
    }
    if (get_var_test_mode()) {
        return;
    }
    g_force_lamps_off = false; // شروعِ سیکلِ جدید → لامپ‌ها دوباره با سنسور کار کنند
    set_var_cycel_started(true);
    if(get_var_step_number()==0){
        if(get_var_setting_general_temperature_limit()){
                if(get_var_element_temperature() >= get_var_element_temperature_target()){
                    start_cycle_progress();
                    set_var_step_process(true);
                }else{
                    // آیتم ۲: به‌جای دوبار-زدنِ پدال، دیالوگ تأیید/انصراف نمایش داده می‌شود.
                    // pedal_operation در هر فشارِ پدال یک‌بار صدا زده می‌شود؛ تایمرِ UI از تکرارِ دیالوگ جلوگیری می‌کند.
                    // اگر کاربر «تأیید» بزند، g_temp_override_start در حلقهٔ task سیکل را شروع می‌کند.
                    g_temp_warn_request = 1;
                    xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
                }
            }
        else{
            start_cycle_progress();
            set_var_step_process(true); 
        }
    }else{
        set_var_step_process(true); 
    }
}
void emergency_operation(){
    fault_log_record(FAULT_EMERGENCY_STOP, 0);
    // ۱. قطع فوری و همزمان تمامی خروجی‌ها و بوبین‌ها بدون حتی ۱ میلی‌ثانیه تاخیر (E-Stop فوری)
    digitalWrite(SSR_1, OFF);
    digitalWrite(TABLE, OFF);
    digitalWrite(ELEMENT, OFF);
    digitalWrite(SHABLON, OFF);
    digitalWrite(PRESS_LEFT, OFF);
    digitalWrite(PRESS_RIGHT, OFF);
    digitalWrite(ZARBEH1, OFF);
    digitalWrite(ZARBEH2, OFF);

    g_force_lamps_off = true; // خطا/استپ -> لامپ‌های سیگنال خاموش شوند
    pedal_in_limit_press = false;
    set_var_step_process(true);
    auto_restart_pending = false;
    auto_restart_last_beep_ms = 0;
    final_jack_started = false;
    weld_cycle_timing = false;
    set_var_cycel_started(false);
    set_var_curent_pedal_status(false);
    set_var_back_button_pressed(false);
    set_var_current_back_button_status(false);
    set_var_josh_is_working(false);
    set_var_cool_is_working(false);
    set_var_josh_counter(0);
    set_var_cooling_counter(0);
    g_sensor_lamp_step = -1;
    g_sensor_lamp_pct = 0;
    g_lamp_pct_shablon = 0;
    g_lamp_pct_table = 0;
    g_lamp_pct_element = 0;

    set_var_press1_out_display(false);
    press_left_is_free = true;
    set_var_press2_out_display(false);
    press_right_is_free = true;
    set_var_zarbe1_out_display(false);
    zarbeh1_is_free = true;
    set_var_zarbe2_out_display(false);
    zarbeh2_is_free = true;
    set_var_safhe_out_display(false);
    table_is_moving = false;
    table_is_free = true;
    set_var_josh_out_display(false);
    element_is_moving = false;
    element_is_free = true;
    set_var_shablon_out_display(false);
    shablon_is_moving = false;
    shablon_is_free = true;
    shablon_time = millis();

    set_var_step_number(0);
    set_var_step_process(false);
    set_var_progress_vlaue(0);
}
void back_button_operation(){
    if(get_var_step_number()==3){
        digitalWrite(PRESS_RIGHT, OFF);
        set_var_press1_out_display(false);
        press_right_is_free=true;
        set_var_step_number(2);
        set_var_back_button_pressed(false);
        set_var_current_back_button_status(false);}
    else if(get_var_step_number()==4){
        digitalWrite(PRESS_LEFT, OFF);
        set_var_press2_out_display(false);
        press_left_is_free=true;
        set_var_step_number(3);
        set_var_back_button_pressed(false);
        set_var_current_back_button_status(false);}
}

static int32_t ui_lang_to_flow_lang_index(int32_t ui_lang)
{
    // UI lang: 1=FA, 2=EN, 3=TR
    // Flow lang index: 0=first language, ...
    if (ui_lang == 2) return 1;
    if (ui_lang == 3) return 2;
    return 0;
}

// Main App Task
void app_task(void *param)
{
    uint32_t last_clock_update_ms = 0;
    uint32_t last_cpu_temp_update_ms = 0;
    int32_t last_applied_lang = -1;
    debounced_digital_t emergencyDin = {true, true, millis()};
    debounced_digital_t pedalDin = {true, true, millis()};
    int last_em_ped_pinmap = -1;
    bool prevEmergencyActive = false;
    bool prevTestMode = false;
    // اگر لبهٔ فشردنِ پدال هم‌زمان با روشن‌بودنِ اسکرین‌سیور بود، این پدال فقط برای بیدارکردنِ صفحه
    // بوده — روی لبهٔ رهاسازیِ متناظرش نباید سیکل جوش را واقعاً شروع کند.
    bool g_pedal_press_was_screensaver_wake = false;


    while (true)
    {
        process_pending_base_values_save();

        // پایش دما و متغیرها در حلقه پیوسته بدون کوچک‌ترین توقف شبکه
        if (g_temp_override_start) {
            g_temp_override_start = 0;
            if (get_var_step_number() == 0 && !get_var_step_process() && !get_var_test_mode() && !g_device_locked) {
                start_cycle_progress();
                set_var_step_process(true);
            }
        }

        if ((int32_t)(g_output_inhibit_until_ms - millis()) > 0) {
            force_all_outputs_off_after_update();
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // استپ/پدال باید از روی silk ردیف ۶/۷ خوانده شوند (مثل منوی اطلاعات).
        // digitalRead روی GPIOِ ADC بعد از جابجایی I6/I7 با I0..I5 → استارت/استاپِ دائم کاذب.
        extern int g_pinmap_gen;
        if (g_pinmap_gen != last_em_ped_pinmap) {
            last_em_ped_pinmap = g_pinmap_gen;
            emergencyDin.stable = emergencyDin.lastSample = true;
            emergencyDin.lastChangeMs = millis();
            pedalDin.stable = pedalDin.lastSample = true;
            pedalDin.lastChangeMs = millis();
        }
        int emSilk = (!storage.inRowNc[6] && EMERGENCY != PIN_NC) ? storage.inRowSilk[6] : -1;
        int pedSilk = (!storage.inRowNc[7] && PEDAL != PIN_NC) ? storage.inRowSilk[7] : -1;
        bool emergencyStable;
        bool pedalStable;
        if (emSilk < 0) {
            emergencyStable = false;
        } else if (emSilk <= 5) {
            // سطحِ «HIGH» مجازی: asserted ↔ پین دیجیتالِ LOW
            emergencyStable = !input_row_asserted(6);
        } else {
            emergencyStable = read_digital_debounced(EMERGENCY, emergencyDin, 30);
        }
        if (pedSilk < 0) {
            pedalStable = true; // قطع → فشرده تلقی نشود (وگرنه !pedalStable دائم استارت می‌زند)
        } else if (pedSilk <= 5) {
            pedalStable = !input_row_asserted(7);
        } else {
            pedalStable = read_digital_debounced(PEDAL, pedalDin, 25);
        }
        if (g_screensaver_active) {
            static uint8_t ss_prev_in_mask = 0;
            static bool ss_mask_init = false;
            uint8_t in_mask = 0;
            for (int si = 0; si < 8; si++) {
                if (fw_info_get_digital_in(si)) in_mask |= (uint8_t)(1u << si);
            }
            if (!ss_mask_init) {
                ss_prev_in_mask = in_mask;
                ss_mask_init = true;
            } else if (in_mask != ss_prev_in_mask) {
                ss_prev_in_mask = in_mask;
                screensaver_note_activity();
            }
        }
        {
            // پدال = شروع/توقفِ فیزیکیِ سیکل → فعالیت برای اسکرین‌سیور (فقط لبه، نه هر تیک)
            static bool prev_pedal_for_activity = false;
            if (pedalStable != prev_pedal_for_activity) {
                bool wasScreensaverActive = g_screensaver_active;
                prev_pedal_for_activity = pedalStable;
                // فقط لبهٔ فشردن ثبت می‌شود؛ لبهٔ رهاسازیِ متناظر (پایین‌تر، pedal_operation) از رویِ
                // همین پرچم می‌فهمد که این فشردن صرفاً بیدارکردنِ صفحه بود، نه قصدِ شروعِ سیکل.
                if (pedalStable && wasScreensaverActive) g_pedal_press_was_screensaver_wake = true;
                screensaver_note_activity();
            }
        }
        // ایمنی: اگر پایهٔ اضطراری با دیالوگِ جابجاییِ پایه N.C. شده (مستقیم یا با «دزدیده‌شدن» توسطِ
        // نامِ دیگر)، به‌جای اعتماد به خوانشِ یک پایهٔ نامعتبر، فرض بر «اضطراریِ فعال» (قفل) گذاشته
        // می‌شود — تا هرگز سیکل بدونِ ورودیِ واقعیِ اضطراری اجرا نشود.
        bool emergencyActive = (EMERGENCY == PIN_NC) ? true : emergency_is_active_level(emergencyStable);
        if (ota_return_sanitize_active()) {
            emergencyActive = false;
            force_all_outputs_off_after_update();
        }
        bool emergencyInactive = !emergencyActive;

        if (millis() - last_clock_update_ms >= 500) {
            DateTime now = rtc_now_s();
            static int last_min = -1;
            static int last_day = -1;
            static int last_lang = -1;
            int cur_lang = get_var_lang();
            if (now.minute() != last_min || now.day() != last_day || cur_lang != last_lang) {
                char clock_text[16];
                char date_text[16];
                snprintf(clock_text, sizeof(clock_text), "%02d:%02d", now.hour(), now.minute());
                if (cur_lang == 1) {
                    // فارسی → تاریخ شمسی (جلالی)
                    int jy = 0, jm = 0, jd = 0;
                    date_utils::gregorian_to_jalali(now.year(), now.month(), now.day(), jy, jm, jd);
                    snprintf(date_text, sizeof(date_text), "%04d/%02d/%02d", jy, jm, jd);
                } else {
                    // انگلیسی/ترکی → تاریخ میلادی
                    snprintf(date_text, sizeof(date_text), "%04d/%02d/%02d", now.year(), now.month(), now.day());
                }
                set_var_header_time_text(clock_text);
                set_var_header_date_text(date_text);
                last_min = now.minute();
                last_day = now.day();
                last_lang = cur_lang;
            }
            last_clock_update_ms = millis();
        }
        if (millis() - last_cpu_temp_update_ms >= 2000) {
            char cpu_temp_text[16];
            float cpu_temp_c = temperatureRead();
            extern volatile float g_fw_info_cpu_temp_c;
            extern volatile float g_fw_info_rtc_temp_c;
            g_fw_info_cpu_temp_c = cpu_temp_c;
            snprintf(cpu_temp_text, sizeof(cpu_temp_text), "%.0f\xC2\xB0", cpu_temp_c);
            set_var_cpu_temperature_text(cpu_temp_text);
            char internal_temp_text[16];
            float rtc_temp_c = rtc_temp_s();
            g_fw_info_rtc_temp_c = rtc_temp_c;
            snprintf(internal_temp_text, sizeof(internal_temp_text), "%.1f\xC2\xB0", rtc_temp_c);
            set_var_internal_temperature_text(internal_temp_text);
            // هشدارِ گرمای داخلیِ سخت‌افزار: بیشترینِ دمای CPU/RTC>=۷۰ یعنی مشکلِ خنک‌کاری/سخت‌افزار.
            // نمایشِ واقعی (فرکانس/دائم) در tick_screen_screen_menu بر اساسِ همین پرچم انجام می‌شود.
            float hottest_c = (cpu_temp_c > rtc_temp_c) ? cpu_temp_c : rtc_temp_c;
            if (hottest_c >= 85.0f) g_internal_temp_band = 3;
            else if (hottest_c >= 80.0f) g_internal_temp_band = 2;
            else if (hottest_c >= 70.0f) g_internal_temp_band = 1;
            else g_internal_temp_band = 0;
            // اتصالِ وب‌اپ = فقط در صورت فعال بودن وب‌اپ و عدم حضور در صفحه وای‌فای/بروزرسانی
            if (webAppEnabled && !g_self_update_session && eez_flow_get_current_screen() != SCREEN_ID_SCREEN_WIFI) {
                static uint8_t prev_station_num = 0;
                uint8_t cur_station_num = WiFi.softAPgetStationNum();
                if (cur_station_num != prev_station_num) {
                    prev_station_num = cur_station_num;
                    screensaver_note_activity();
                }
            }
            last_cpu_temp_update_ms = millis();
        }

        int32_t current_lang = get_var_lang();
        if (current_lang != last_applied_lang) {
            eez_flow_set_language_index(ui_lang_to_flow_lang_index(current_lang));
            last_applied_lang = current_lang;
        }

        if (get_var_test_mode()) {
            if (emergencyActive) main_menu_test_mode_stop(); // ورودیِ فیزیکیِ اضطراری (I6) هم در حالتِ تست خارج کند
            prevTestMode = true;
            auto_restart_pending = false;
            set_var_progress_vlaue(0);
            apply_test_outputs();
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (prevTestMode) {
            prevTestMode = false;
            apply_test_outputs();
        }

        process_auto_restart();
        update_sensor_lamps(); // همیشه اجرا شود (حتی موقع auto-restart) تا لامپ میز روی ۵۰ گیر نکند
        if (auto_restart_pending) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        update_cycle_progress();

        if (get_var_cycel_started()){
            if (emergencyActive){
                if (!prevEmergencyActive) {
                    // E-Stop فوری: همه ۸ خروجی + SSR_1 بلافاصله قطع (بدون تأخیر حتی ۱ms)
                    emergency_operation();
                    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                }
                prevEmergencyActive = true;
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }
            prevEmergencyActive = false;
            if (get_var_step_process()) {
                bool sensor_en = get_var_setting_admin_jack_sensor();
                // ثبت زمانِ «اولین استارت» چرخه (فقط یک‌بار، در ابتدای چرخه از step 0)
                if (get_var_step_number()==0 && !weld_cycle_timing){
                    weld_cycle_start_ms = millis();
                    weld_cycle_timing = true;
                }
                if (get_var_step_number()==0){

                    if (!shablon_is_moving && shablon_is_free){
                        // چکِ پیش‌فرضِ شروع: همه‌ی جک‌ها باید باز باشند، وگرنه خطا و استوپ
                        if (!check_all_jacks_open_or_error()) {
                            emergency_operation();
                        } else {
                            digitalWrite(SHABLON, ON);
                            set_var_shablon_out_display(true);
                            shablon_is_moving=true;
                            shablon_time=millis();
                        }
                    }else if (jack_sensor_done(sensor_en, MUX_FAM_SHABLON, true, shablon_time, get_var_setting_jack_shablon_raft(), 1)){
                        shablon_is_moving=false;
                        shablon_is_free=false;
                        set_var_step_number(1);
                    }
                }else if(get_var_step_number()==1){
                    if (!table_is_moving && table_is_free){
                        digitalWrite(TABLE, ON);
                        set_var_safhe_out_display(true);
                        table_is_moving=true;
                        table_time=millis();
                    }else if (jack_sensor_off_done(sensor_en, MUX_FAM_TABLE, false, table_time, get_var_setting_jack_safhe_raft(), 6)){
                        table_is_moving=false;
                        table_is_free=false;
                        set_var_step_number(2);
                        set_var_step_process(false);
                    }
                }else if(get_var_step_number()==2){
                    if (!press_right_is_moving && press_right_is_free){
                        digitalWrite(PRESS_RIGHT, ON);
                        set_var_press1_out_display(true);
                        press_right_is_moving=true;
                        press_right_time=millis();
                    }else {
                        uint32_t req_time = (uint32_t)get_var_setting_jack_press_right_raft();
                        if (req_time < 100U) req_time = 100U; // حداقل زمان ایمنی حرکت مکانیکی کلمپ
                        if (millis() - press_right_time >= req_time) {
                            press_right_is_moving=false;
                            press_right_is_free=false;
                            set_var_step_number(3);
                            set_var_step_process(false);
                        }
                    }
                }else if(get_var_step_number()==3){
                    if (!press_left_is_moving && press_left_is_free){
                        digitalWrite(PRESS_LEFT, ON);
                        set_var_press2_out_display(true);
                        press_left_is_moving=true;
                        press_left_time=millis();
                    }else {
                        uint32_t req_time = (uint32_t)get_var_setting_jack_press_left_raft();
                        if (req_time < 100U) req_time = 100U; // حداقل زمان ایمنی حرکت مکانیکی کلمپ
                        if (millis() - press_left_time >= req_time) {
                            press_left_is_moving=false;
                            press_left_is_free=false;
                            set_var_step_number(4);
                            if (get_var_setting_general_pedal_number3()){
                                set_var_step_process(false);
                            }
                        }
                    }
                }else if(get_var_step_number()==4){
                    if (!table_is_moving && !table_is_free){
                        digitalWrite(TABLE, OFF);
                        set_var_safhe_out_display(false);
                        table_is_moving=true;
                        table_time=millis();
                    }else if (jack_sensor_done(sensor_en, MUX_FAM_TABLE, false, table_time, get_var_setting_jack_safhe_bargasht(), 3)){
                        table_is_moving=false;
                        table_is_free=true;
                        set_var_step_number(5);
                    }
                }else if(get_var_step_number()==5){
                    if (!shablon_is_moving && !shablon_is_free){
                        digitalWrite(SHABLON, OFF);
                        set_var_shablon_out_display(false);
                        shablon_is_moving=true;
                        shablon_time=millis();
                    }else if (jack_sensor_done(sensor_en, MUX_FAM_SHABLON, false, shablon_time, get_var_setting_jack_shablon_bargasht(), 2)){
                        shablon_is_moving=false;
                        shablon_is_free=true;
                        set_var_step_number(6);
                    }
                }else if(get_var_step_number()==6){
                    // اینترلاک ایمنی سخت‌گیرانه: جک المنت (صفحه داغ) فقط وقتی مجاز به حرکت به جلو است که هر دو کلمپ محکم درگیر باشند
                    if (press_left_is_free || press_right_is_free) {
                        emergency_operation();
                    } else if (!element_is_moving && element_is_free){
                        digitalWrite(ELEMENT, ON);
                        set_var_josh_out_display(true);
                        element_is_moving=true;
                        element_time=millis();
                    }else if (jack_sensor_done(sensor_en, MUX_FAM_ELEMENT, true, element_time, get_var_setting_jack_josh_raft(), 4)){
                        element_is_moving=false;
                        element_is_free=false;
                        set_var_step_number(7);
                    }
                }else if(get_var_step_number()==7){
                    // پایش پیوسته فک‌های پرس حین پیشروی میز به سمت المنت داغ
                    if (press_left_is_free || press_right_is_free) {
                        emergency_operation();
                    } else if (!table_is_moving && table_is_free){
                        digitalWrite(TABLE, ON);
                        set_var_safhe_out_display(true);
                        table_is_moving=true;
                        table_time=millis();
                    }else if (jack_sensor_off_done(sensor_en, MUX_FAM_TABLE, false, table_time, get_var_setting_jack_safhe_raft(), 6)){
                        table_is_moving=false;
                        table_is_free=false;
                        set_var_step_number(8);
                        set_var_josh_counter(get_var_welding_time());
                    }
                }else if(get_var_step_number()==8){
                    // پایش پیوسته فک‌های پرس حین ذوب جوشکاری (۱۰ تا ۳۰ ثانیه)
                    if (press_left_is_free || press_right_is_free) {
                        emergency_operation();
                    } else if (!get_var_josh_is_working()){
                        set_var_josh_is_working(true);
                        josh_time=millis();
                        g_weld_sample_sum = 0;
                        g_weld_sample_count = 0;
                        g_weld_min_temp = 999;
                        g_weld_max_temp = 0;
                    }else if ((millis() - josh_time >= get_var_welding_time()*1000)){
                        set_var_josh_is_working(false);
                        set_var_josh_counter(0);
                        set_var_step_number(9);
                    }else {
                        int curT = (int)get_var_element_temperature();
                        g_weld_sample_sum += curT;
                        g_weld_sample_count++;
                        if (curT < g_weld_min_temp) g_weld_min_temp = curT;
                        if (curT > g_weld_max_temp) g_weld_max_temp = curT;
                        set_var_josh_counter(get_var_welding_time() -(int32_t)((millis() - josh_time) / 1000UL));
                    }
                }else if(get_var_step_number()==9){
                    if (!table_is_moving && !table_is_free){
                        digitalWrite(TABLE, OFF);
                        set_var_safhe_out_display(false);
                        table_is_moving=true;
                        table_time=millis();
                    }else if (jack_sensor_done(sensor_en, MUX_FAM_TABLE, false, table_time, get_var_setting_jack_safhe_bargasht(), 3)){
                        table_is_moving=false;
                        table_is_free=true;
                        set_var_step_number(10);
                    }
                }else if(get_var_step_number()==10){
                    if (!element_is_moving && !element_is_free){
                        digitalWrite(ELEMENT, OFF);
                        set_var_josh_out_display(false);
                        element_is_moving=true;
                        element_time=millis();
                    }else if (jack_sensor_done(sensor_en, MUX_FAM_ELEMENT, false, element_time, get_var_setting_jack_josh_bargasht(), 5)){
                        element_is_moving=false;
                        set_var_step_number(11);
                        element_is_free=true;
                    }
                }else if(get_var_step_number()==11){
                    if (!table_is_moving && table_is_free){
                        digitalWrite(TABLE, ON);
                        set_var_safhe_out_display(true);
                        table_is_moving=true;
                        table_time=millis();
                    }else if (jack_sensor_off_done(sensor_en, MUX_FAM_TABLE, false, table_time, get_var_setting_jack_safhe_raft(), 6)){
                        table_is_moving=false;
                        table_is_free=false;
                        set_var_step_number(12);
                        set_var_cooling_counter(get_var_cooling_time());
                    }
                }else if(get_var_step_number()==12){
                    if (!get_var_cool_is_working()){
                        set_var_cool_is_working(true);
                        cool_time=millis();

                    }else if((millis() - cool_time >=get_var_setting_jack_profil_raft()) && zarbeh1_is_free){
                            digitalWrite(ZARBEH1, ON);
                            set_var_zarbe1_out_display(true);
                            zarbeh1_is_free=false;
                    }
                    else if((millis() - cool_time >=get_var_setting_jack_profil_bargasht()) && zarbeh2_is_free){
                            digitalWrite(ZARBEH2, ON);
                            set_var_zarbe2_out_display(true);
                            zarbeh2_is_free=false;
                    }
                    else if ((millis() - cool_time >= get_var_cooling_time()*1000)){
                        set_var_cool_is_working(false);
                        completed_weld_cycles_in_frame++;
                        g_weld_done_in_frame = completed_weld_cycles_in_frame; // ۱..۴ شمارهٔ لنگه در این چهارچوب

                        // محاسبه شاخص هوشمند کیفیت جوش استاندارد DVS 2207-1
                        float avg_melt_t = (g_weld_sample_count > 0) ? ((float)g_weld_sample_sum / g_weld_sample_count) : (float)get_var_element_temperature();
                        float actual_melt = (float)(millis() - josh_time) / 1000.0f;
                        float actual_cool = (float)(millis() - cool_time) / 1000.0f;
                        float cpu_t = temperatureRead();
                        float amb_t = (cpu_t > 20.0f) ? (cpu_t - 14.0f) : 24.0f;

                        g_last_weld_telemetry = WeldQualityEngine::calculate(
                            (float)get_var_element_temperature_target(),
                            avg_melt_t,
                            (float)(g_weld_min_temp < 999 ? g_weld_min_temp : avg_melt_t),
                            (float)(g_weld_max_temp > 0 ? g_weld_max_temp : avg_melt_t),
                            (float)get_var_welding_time(),
                            actual_melt,
                            (float)get_var_cooling_time(),
                            actual_cool,
                            amb_t,
                            false
                        );

                        // شمارنده استهلاک پارچه نسوز تفلون
                        g_teflon_cycles = storage.get_teflon_cycles() + 1;
                        storage.set_teflon_cycles(g_teflon_cycles);
                        if (g_teflon_cycles >= (uint32_t)g_predictive_thermal.cfg.teflon_max_warn) {
                            g_teflon_replace_warn = true;
                        }

                        if (completed_weld_cycles_in_frame >= 4) {
                            completed_weld_cycles_in_frame = 0;
                            int32_t total_frames = abs(storage.get_frameNumbers()) + 1;
                            storage.set_frameNumbers(total_frames);
                            set_var_total_frame_numbers(total_frames);
                            set_var_frame_numbers(get_var_frame_numbers() + 1);
                            storage.set_frameNumbersCur((int)get_var_frame_numbers()); // جاری موندگار شود
                        }
                        set_var_step_number(13);
                        set_var_cooling_counter(0);
                    }else {
                        int32_t remaining =get_var_cooling_time() -(int32_t)((millis() - cool_time) / 1000UL);
                        set_var_cooling_counter(remaining);
                        static int32_t last_beep_second = -1;
                        if (remaining <= 4 && remaining > 0)
                        {
                            if (remaining != last_beep_second){
                                xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                                last_beep_second = remaining;}
                        }
                    }
                }else if(get_var_step_number()==13){
                    if (!final_jack_started) {
                        final_jack_started = true;
                        final_jack_start_ms = millis();
                    }
                    if (get_var_setting_general_final_jack()){
                        step_by_step_jack();
                    }else{
                        fast_jack();
                    }
                }
            }else if(!pedalStable && !get_var_curent_pedal_status() && !get_var_back_button_pressed() && !get_var_step_process()){
                set_var_curent_pedal_status(true);
                if (!g_device_locked && eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU) {
                    if (g_pedal_press_was_screensaver_wake) {
                        // این فشردنِ پدال فقط اسکرین‌سیور را بیدار کرد؛ سیکل شروع نشود (نقشش فقط خروج بود)
                        g_pedal_press_was_screensaver_wake = false;
                    } else {
                        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                        pedal_operation();
                    }
                } else if (eez_flow_get_current_screen() != SCREEN_ID_SCREEN_MENU && eez_flow_get_current_screen() != SCREEN_ID_SCREEN_SPLASH) {
                    // فشردن پدال خارج از صفحه اصلی (منوی تنظیمات) -> عدم شروع جوش و نمایش پیام راهنما
                    g_pedal_outside_menu_warn_request = 1;
                    xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
                }
            }
            else if(get_var_back_button_pressed()){
                back_button_operation();
            }
            else{
                if (pedalStable){
                    set_var_curent_pedal_status(false);
                }
            }
        }else if(!emergency_push_button){
            if (ota_return_sanitize_active()) {
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }
            if(emergencyInactive && !get_var_step_process()){
                xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
                set_var_cycel_started(true);
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }
            emergency_operation();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void stack_monitor_task(void *param)
{
    // UBaseType_t uxHighWaterMark; // برای استفاده با نسخه‌های قدیمی‌تر FreeRTOS

    while (true)
    {

        const size_t bufferSize = 1024;
        char *task_list_buffer = (char *)pvPortMalloc(bufferSize);

        if (task_list_buffer == NULL)
        {
            Serial.println("Failed to allocate memory for task list buffer");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue; // دوباره تلاش کن
        }

        Serial.println("--- Stack Usage Report ---");
        Serial.println("Task Name\t\tStack High Water Mark (Bytes Remaining)");
        Serial.println("---------------------------------------------------------");

        vTaskList(task_list_buffer);
        Serial.println(task_list_buffer);

        Serial.println("---------------------------------------------------------\n");

        // آزاد کردن حافظه بافر
        vPortFree(task_list_buffer);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// اگر InstScan قبلاً lastfile را گرفته، همان نسخه را برگردان (بدون HTTPS دوباره)
static bool installer_cached_version_for_model(const String &relModel, String &outVer)
{
    if (!g_inst_scan_done) return false;
    for (int i = 0; i < INST_COUNT; i++) {
        String m = String(g_inst_models[i]);
        m.trim();
        while (m.startsWith("/")) m.remove(0, 1);
        if (m.startsWith("plc_hmi/")) m.remove(0, 8);
        bool match = (m == relModel);
        if (!match) {
            for (int fi = 0; fi < 3 && g_inst_models_fallback[i][fi]; fi++) {
                String fm = String(g_inst_models_fallback[i][fi]);
                fm.trim(); while (fm.startsWith("/")) fm.remove(0,1);
                if (fm.startsWith("plc_hmi/")) fm.remove(0,8);
                if (fm == relModel) { match = true; break; }
                // همچنین تطبیقِ بدونِ encode: upvc%2FWelding == upvc/Welding
                String fmDe = fm; fmDe.replace("%2F","/"); fmDe.replace("%2f","/");
                String relDe = relModel; relDe.replace("%2F","/"); relDe.replace("%2f","/");
                if (fmDe == relDe) { match = true; break; }
            }
            // تطبیقِ متقاطع: new model ↔ old relModel (Welding_single_head ↔ firmware_welder_single_head)
            if (!match) {
                String lowRel = relModel; lowRel.toLowerCase();
                if (lowRel.indexOf("welding")>=0 || lowRel.indexOf("welder_single")>=0) {
                    if (String(g_inst_models[i]).indexOf("Welding_single_head")>=0) match = true;
                } else if (lowRel.indexOf("config")>=0) {
                    if (String(g_inst_models[i]).indexOf("config-test")>=0) match = true;
                }
            }
        }
        if (!match) continue;
        if (!g_inst_available[i]) return false;
        const char *v = g_inst_version[i];
        if (!v || !v[0]) return false;
        // فقط نسخهٔ عددی — "READY" از HEAD کافی برای skip آخرین GET نیست
        if (v[0] >= '0' && v[0] <= '9' && strcmp(v, "0.0.0") != 0) {
            outVer = v;
            return true;
        }
        return false;
    }
    return false;
}

// نمایشِ درصد و سرعت دانلود روی پنل و حلقهٔ نصب (مشابه استاندارد config-test)
static int s_ota_last_pct = -1;
static int s_ota_last_painted = -1;
static uint32_t s_ota_last_time_ms = 0;
static size_t s_ota_last_bytes = 0;
static float s_ota_speed_kbps = 0.0f;

static void ota_progress_cb(int cur, int total)
{
    int pct = (total > 0) ? (int)((long long)cur * 100 / total) : 0;
    if (pct > 100) pct = 100;
    if (pct == s_ota_last_pct && (millis() - s_ota_last_time_ms < 250)) return;
    s_ota_last_pct = pct;
    g_ota_ui_pct = pct;
    g_ota_pct_pending = true;

    uint32_t now_ms = millis();
    if (s_ota_last_time_ms == 0) {
        s_ota_last_time_ms = now_ms;
        s_ota_last_bytes = cur;
    } else {
        uint32_t dt = now_ms - s_ota_last_time_ms;
        if (dt >= 250) {
            size_t db = (cur >= (int)s_ota_last_bytes) ? (cur - s_ota_last_bytes) : 0;
            s_ota_speed_kbps = ((float)db / (float)dt) * 1000.0f / 1024.0f;
            s_ota_last_time_ms = now_ms;
            s_ota_last_bytes = cur;
        }
    }

    float cur_mb = (float)cur / (1024.0f * 1024.0f);
    float tot_mb = (total > 0) ? ((float)total / (1024.0f * 1024.0f)) : cur_mb;
    char metrics_buf[64];
    snprintf(metrics_buf, sizeof(metrics_buf), "%%%d | %.2f / %.2f MB | %.0f KB/s", pct, cur_mb, tot_mb, s_ota_speed_kbps);
    set_var_download_lable_info_text(metrics_buf);

    // تغذیه واچ‌داگ حین دانلود و فلش قطعات باینری
    yield();
}

// موتور استریم مستقیم بافر ۱۶ کیلوبایتی با اعتبارسنجی SHA-256 و فلش پارتیشن OTA
static bool execute_ota_stream(const String &binUrl, const String &expectedSha256 = "", int totalBytesExpected = 0)
{
    Serial.printf("[OTA] execute_ota_stream: %s (exp size=%d)\n", binUrl.c_str(), totalBytesExpected);
    Serial.printf("[OTA] Heap: %lu free, largest=%lu\n",
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

    // قانون طلایی ۱ و ۲: تضمین پاک‌سازی کامل پارتیشن پیش از باز کردن سوکت (Clean Flash Guarantee)
    ota_pre_erase_ensure_clean_blocking();

    // قانون طلایی ۴: غیرفعال‌سازی خواب رادیو و توان حداکثری جهت بیشینه‌سازی پهنای باند
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_max_tx_power(80);

    String streamUrl = binUrl;
    streamUrl.trim();
    if (streamUrl.startsWith("https://ota.haffler-infi.ir")) {
        streamUrl.replace("https://", "http://");
    }

    WiFiClientSecure binClient;
    WiFiClient plainClient;
    bool usePlain = false;
    HTTPClient http2;
    http2.setConnectTimeout(10000);
    http2.setTimeout(15000);
    int code = -1;

    if (streamUrl.startsWith("https://")) {
        binClient.setInsecure();
        binClient.setTimeout(15);
        binClient.setNoDelay(true);
        http2.begin(binClient, streamUrl);
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        code = http2.GET();
        if (code < 0) {
            Serial.printf("[OTA] GET retry code=%d\n", code);
            http2.end();
            vTaskDelay(pdMS_TO_TICKS(500));
            http2.begin(binClient, streamUrl);
            http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http2.setReuse(false);
            code = http2.GET();
        }
        if (code != 200) {
            http2.end();
            binClient.stop();
            String plainUrl = streamUrl;
            plainUrl.replace("https://", "http://");
            Serial.printf("[OTA] HTTPS streaming failed (%d), falling back to plain HTTP: %s\n", code, plainUrl.c_str());
            plainClient.setTimeout(15);
            plainClient.setNoDelay(true);
            http2.begin(plainClient, plainUrl);
            http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http2.setReuse(false);
            code = http2.GET();
            usePlain = true;
        }
    } else {
        plainClient.setTimeout(15);
        plainClient.setNoDelay(true);
        http2.begin(plainClient, streamUrl);
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        http2.addHeader("User-Agent", "Haffler-OTA/1.5");
        http2.addHeader("Accept", "*/*");
        http2.addHeader("Connection", "close");
        code = http2.GET();
        if (code != 200 && (streamUrl.indexOf("%2F") >= 0 || streamUrl.indexOf("%2f") >= 0)) {
            String slashUrl = streamUrl;
            slashUrl.replace("%2F", "/");
            slashUrl.replace("%2f", "/");
            http2.end();
            http2.begin(plainClient, slashUrl);
            http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http2.setReuse(false);
            http2.addHeader("User-Agent", "Haffler-OTA/1.5");
            http2.addHeader("Accept", "*/*");
            http2.addHeader("Connection", "close");
            code = http2.GET();
        }
        usePlain = true;
    }
    if (code != 200) {
        Serial.printf("[OTA] GET failed: HTTP %d\n", code);
        http2.end();
        if (usePlain) plainClient.stop(); else binClient.stop();
        return false;
    }

    int total = http2.getSize();
    if (total <= 0 && totalBytesExpected > 0) total = totalBytesExpected;
    size_t updateSize = (total > 0) ? (size_t)total : UPDATE_SIZE_UNKNOWN;

    if (Update.isRunning()) {
        Update.abort();
    }

    if (!Update.begin(updateSize)) {
        Serial.printf("[OTA] Update.begin failed (size=%u, err=%d)\n", (unsigned)updateSize, (int)Update.getError());
        http2.end();
        if (usePlain) plainClient.stop(); else binClient.stop();
        return false;
    }

    size_t BUFSZ = 8192;
    uint8_t *buf = (uint8_t *)heap_caps_malloc(BUFSZ, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf) { BUFSZ = 4096; buf = (uint8_t *)heap_caps_malloc(BUFSZ, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); }
    if (!buf) { BUFSZ = 8192; buf = (uint8_t *)heap_caps_malloc(BUFSZ, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
    if (!buf) { BUFSZ = 4096; buf = (uint8_t *)malloc(BUFSZ); }
    if (!buf) {
        Serial.println("[OTA] Failed to allocate stream buffer");
        http2.end();
        if (usePlain) plainClient.stop(); else binClient.stop();
        Update.abort();
        return false;
    }

    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0);

    WiFiClient *stream = http2.getStreamPtr();
    size_t written = 0;
    uint32_t lastData = millis();
    bool isStalled = false;

    s_ota_last_pct = -1;
    s_ota_last_painted = -1;
    s_ota_last_time_ms = 0;
    s_ota_last_bytes = 0;
    s_ota_speed_kbps = 0.0f;
    g_ota_cancel_requested = false;

    disableCore0WDT();

    size_t agg_len = 0;
    while (http2.connected() && (total <= 0 || written < (size_t)total)) {
        if (g_ota_cancel_requested || g_ota_exit_request) {
            Serial.println("[OTA] Cancelled by user");
            break;
        }
        size_t avail = stream->available();
        if (avail) {
            size_t space = BUFSZ - agg_len;
            size_t toRead = (avail < space) ? avail : space;
            int r = stream->readBytes((char *)(buf + agg_len), toRead);
            if (r > 0) {
                agg_len += (size_t)r;
                lastData = millis();
            }
            if (agg_len >= BUFSZ || (total > 0 && (written + agg_len) >= (size_t)total)) {
                mbedtls_sha256_update(&sha_ctx, buf, agg_len);
                if (Update.write(buf, agg_len) != agg_len) {
                    Serial.println("[OTA] Update.write failed");
                    break;
                }
                written += agg_len;
                ota_progress_cb((int)written, total);
                agg_len = 0;
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        } else {
            if (agg_len > 0) {
                mbedtls_sha256_update(&sha_ctx, buf, agg_len);
                if (Update.write(buf, agg_len) != agg_len) {
                    Serial.println("[OTA] Update.write flush failed");
                    break;
                }
                written += agg_len;
                ota_progress_cb((int)written, total);
                agg_len = 0;
            }
            if (WiFi.status() != WL_CONNECTED && (millis() - lastData > 3000)) {
                Serial.println("[OTA] WiFi disconnected during stream");
                isStalled = true;
                break;
            }
            if (millis() - lastData > 15000) {
                Serial.println("[OTA] Stream stalled >15s");
                isStalled = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    if (agg_len > 0 && !isStalled && !g_ota_cancel_requested) {
        mbedtls_sha256_update(&sha_ctx, buf, agg_len);
        if (Update.write(buf, agg_len) == agg_len) {
            written += agg_len;
            ota_progress_cb((int)written, total);
        }
        agg_len = 0;
    }

    enableCore0WDT();
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

    if (buf) free(buf);
    http2.end();
    if (usePlain) plainClient.stop(); else binClient.stop();

    uint8_t sha_out[32];
    mbedtls_sha256_finish(&sha_ctx, sha_out);
    mbedtls_sha256_free(&sha_ctx);

    char actualShaHex[65];
    for (int i = 0; i < 32; i++) snprintf(actualShaHex + (i * 2), 3, "%02x", sha_out[i]);
    actualShaHex[64] = '\0';

    String expSha = expectedSha256;
    expSha.trim();
    bool shaOk = true;
    if (expSha.length() == 64) {
        shaOk = (strcasecmp(actualShaHex, expSha.c_str()) == 0);
        if (!shaOk) {
            Serial.printf("[OTA] SHA256 mismatch! Exp: %s, Got: %s\n", expSha.c_str(), actualShaHex);
        } else {
            Serial.printf("[OTA] SHA256 verified: %s\n", actualShaHex);
        }
    }

    if (!g_ota_cancel_requested && !g_ota_exit_request && !isStalled && (total <= 0 || written == (size_t)total) && shaOk && Update.end(true) && Update.isFinished()) {
        Serial.println("[OTA] Update successful! Rebooting...");
        g_ota_ui_pct = 100;
        g_ota_pct_pending = true;
        ota_ui_set(ML("نصب کامل شد ، راه اندازی مجدد ...", "Update done, rebooting...", "Guncelleme tamam, yeniden baslatiliyor..."));
        vTaskDelay(pdMS_TO_TICKS(300));
        if (storage.getFirmwarePartionSelect() == 0) storage.setFirmwarePartionSelect(1);
        else                                          storage.setFirmwarePartionSelect(0);
        delay(300);
        ESP.restart();
        return true;
    } else {
        Update.abort();
        Serial.printf("[OTA] Stream aborted (written=%u/%d, isStalled=%d, shaOk=%d, err=%d)\n",
                      (unsigned)written, total, (int)isStalled, (int)shaOk, (int)Update.getError());
        return false;
    }
}

void download_firmware(String model)
{
    Serial.println("[OTA] download_firmware start: " + model);
    ota_ui_set(ML("در حال بررسی ...", "Checking...", "Kontrol ediliyor..."));
    g_inst_scan_busy = true;

    if (WiFi.status() != WL_CONNECTED)
    {
        String ss = storage.getWifiSsid();
        if (ss.length() == 0) ss = selectedWifi;
        String pw = storage.getWifiPassword();
        if (pw.length() == 0) pw = wifiPassword;
        if (ss.length() > 0)
        {
            WiFi.mode(WIFI_STA);
            WiFi.disconnect(false, false);
            vTaskDelay(pdMS_TO_TICKS(300));
            Serial.printf("[OTA] reconnect primary ssid='%s'\n", ss.c_str());
            wifi_sta_begin_network(ss.c_str(), pw.c_str(), -1);
            int c = 0;
            while (WiFi.status() != WL_CONNECTED && c < 30) { vTaskDelay(pdMS_TO_TICKS(500)); c++; }
            if (WiFi.status() == WL_CONNECTED) wifi_on_connected_hooks();
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            for (int wi = 0; wi < storage.wifi_count && WiFi.status() != WL_CONNECTED; wi++)
            {
                String candSsid = storage.wifi_ssids[wi];
                String candPwd  = storage.wifi_pwds[wi];
                if (candSsid.length() == 0 || candSsid == ss) continue;
                Serial.printf("[OTA] reconnect fallback ssid='%s'\n", candSsid.c_str());
                WiFi.disconnect(false, false);
                vTaskDelay(pdMS_TO_TICKS(200));
                wifi_sta_begin_network(candSsid.c_str(), candPwd.c_str(), -1);
                int c = 0;
                while (WiFi.status() != WL_CONNECTED && c < 20) { vTaskDelay(pdMS_TO_TICKS(500)); c++; }
                if (WiFi.status() == WL_CONNECTED) {
                    storage.setWifiCredentials(candSsid, candPwd);
                    wifi_on_connected_hooks();
                    break;
                }
            }
        }
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        ota_ui_show_then_dismiss(ML("وای‌فای وصل نیست — ابتدا وصل کنید", "WiFi not connected", "WiFi bagli degil"));
        g_inst_scan_busy = false;
        return;
    }

    // شناسایی کارت مربوطه برای کاندیداها
    String cleanModel = model;
    cleanModel.trim();
    while (cleanModel.startsWith("/")) cleanModel.remove(0, 1);
    if (cleanModel.startsWith("plc_hmi/")) cleanModel.remove(0, 8);

    int cardIdx = -1;
    for (int i = 0; i < 9; i++) {
        for (int c = 0; g_card_model_candidates[i][c] != NULL; c++) {
            String cand = String(g_card_model_candidates[i][c]);
            if (cleanModel == cand || cleanModel.indexOf(cand) >= 0) { cardIdx = i; break; }
            String candDe = cand; candDe.replace("%2F", "/"); candDe.replace("%2f", "/");
            String cleanDe = cleanModel; cleanDe.replace("%2F", "/"); cleanDe.replace("%2f", "/");
            if (cleanDe == candDe || cleanDe.indexOf(candDe) >= 0) { cardIdx = i; break; }
        }
        if (cardIdx >= 0) break;
    }

    String activeModel = cleanModel;
    String latestVersion = "";
    String expectedSha256 = "";
    bool foundLastfile = false;

    WiFiClientSecure client;
    WiFiClient plainClient;
    client.setInsecure();
    client.setTimeout(7);
    client.setNoDelay(true);
    plainClient.setTimeout(7);
    plainClient.setNoDelay(true);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    http.setReuse(false);

    if (cardIdx >= 0) {
        for (int c = 0; g_card_model_candidates[cardIdx][c] != NULL; c++) {
            String m = g_card_model_candidates[cardIdx][c];
            String url = ota_url(m, "lastfile");
            if (url.startsWith("http://")) {
                http.begin(plainClient, url);
            } else {
                http.begin(client, url);
            }
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            int code = http.GET();
            if (code == 200) {
                LastfileInfo lf;
                config_parse_lastfile(http.getString(), lf);
                if (lf.ver.length() > 0) {
                    latestVersion = lf.ver;
                    expectedSha256 = lf.sha256;
                    activeModel = m;
                    foundLastfile = true;
                    http.end();
                    break;
                }
            }
            http.end();
            if (!foundLastfile && (url.indexOf("%2F") >= 0 || url.indexOf("%2f") >= 0)) {
                String slashUrl = url;
                slashUrl.replace("%2F", "/");
                slashUrl.replace("%2f", "/");
                http.begin(plainClient, slashUrl);
                http.addHeader("User-Agent", "Haffler-OTA/1.5");
                http.addHeader("Accept", "*/*");
                http.addHeader("Connection", "close");
                code = http.GET();
                if (code == 200) {
                    LastfileInfo lf;
                    config_parse_lastfile(http.getString(), lf);
                    if (lf.ver.length() > 0) {
                        latestVersion = lf.ver;
                        expectedSha256 = lf.sha256;
                        activeModel = m;
                        foundLastfile = true;
                        http.end();
                        break;
                    }
                }
                http.end();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    } else {
        String url = ota_url(cleanModel, "lastfile");
        if (url.startsWith("http://")) {
            http.begin(plainClient, url);
        } else {
            http.begin(client, url);
        }
        http.addHeader("User-Agent", "Haffler-OTA/1.5");
        http.addHeader("Accept", "*/*");
        http.addHeader("Connection", "close");
        int code = http.GET();
        if (code == 200) {
            LastfileInfo lf;
            config_parse_lastfile(http.getString(), lf);
            if (lf.ver.length() > 0) {
                latestVersion = lf.ver;
                expectedSha256 = lf.sha256;
                foundLastfile = true;
            }
        }
        http.end();
        if (!foundLastfile && (url.indexOf("%2F") >= 0 || url.indexOf("%2f") >= 0)) {
            String slashUrl = url;
            slashUrl.replace("%2F", "/");
            slashUrl.replace("%2f", "/");
            http.begin(plainClient, slashUrl);
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            code = http.GET();
            if (code == 200) {
                LastfileInfo lf;
                config_parse_lastfile(http.getString(), lf);
                if (lf.ver.length() > 0) {
                    latestVersion = lf.ver;
                    expectedSha256 = lf.sha256;
                    foundLastfile = true;
                }
            }
            http.end();
        }
    }
    client.stop();
    plainClient.stop();

    if (!foundLastfile) {
        Serial.println("[OTA] lastfile not found on server");
        ota_ui_show_then_dismiss(ML("خطا - برنامه‌ای روی سرور پیدا نشد", "Error: Program not found on server", "Hata: Sunucuda program bulunamadi"));
        g_inst_scan_busy = false;
        g_ota_install_idx = -1;
        return;
    }

    bool selfUpdate = (activeModel.indexOf("welding-single") >= 0 || activeModel.indexOf("Welding_single") >= 0);
    if (selfUpdate && g_ota_install_idx < 0 && !versionNewer(latestVersion, String(VERSION))) {
        Serial.println("[OTA] Already up to date - skipping popup");
        ota_ui_close_panel();
        g_inst_scan_busy = false;
        g_ota_install_idx = -1;
        return;
    }

    ota_ui_begin_install_progress();
    vTaskDelay(pdMS_TO_TICKS(280));

    String binUrl = ota_url(activeModel, "firmware.bin");
    Serial.println("[OTA] streaming firmware: " + binUrl);

    bool ok = execute_ota_stream(binUrl, expectedSha256);
    if (!ok) {
        // امتحان مجدد با نام firmware بدون پسوند
        String fallbackUrl = ota_url(activeModel, "firmware");
        Serial.println("[OTA] retry with fallback firmware: " + fallbackUrl);
        ok = execute_ota_stream(fallbackUrl, expectedSha256);
    }

    if (!ok) {
        ota_ui_show_then_dismiss(ML("خطا در دانلود یا نصب فریم‌ور", "Error downloading/installing firmware", "Yazilim indirme/kurulum hatasi"));
    }
    g_inst_scan_busy = false;
    g_ota_install_idx = -1;
}

static bool download_firmware_from_url(const String &url)
{
    Serial.println("[OTA] download_firmware_from_url: " + url);
    ota_ui_set(ML("در حال اتصال به سرور...", "Connecting to server...", "Sunucuya baglaniliyor..."));
    g_inst_scan_busy = true;

    if (WiFi.status() != WL_CONNECTED)
    {
        String ss = storage.getWifiSsid();
        String pw = storage.getWifiPassword();
        if (ss.length() > 0)
        {
            WiFi.mode(WIFI_STA);
            WiFi.disconnect(false, false);
            vTaskDelay(pdMS_TO_TICKS(100));
            Serial.printf("[OTA] reconnect primary ssid='%s'\n", ss.c_str());
            wifi_sta_begin_network(ss.c_str(), pw.c_str(), -1);
            int c = 0;
            while (WiFi.status() != WL_CONNECTED && c < 30) {
                vTaskDelay(pdMS_TO_TICKS(500));
                c++;
            }
            if (WiFi.status() == WL_CONNECTED) wifi_on_connected_hooks();
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            for (int wi = 0; wi < storage.wifi_count && WiFi.status() != WL_CONNECTED; wi++)
            {
                String candSsid = storage.wifi_ssids[wi];
                String candPwd  = storage.wifi_pwds[wi];
                if (candSsid.length() == 0 || candSsid == ss) continue;
                Serial.printf("[OTA] reconnect fallback ssid='%s'\n", candSsid.c_str());
                WiFi.disconnect(false, false);
                vTaskDelay(pdMS_TO_TICKS(200));
                wifi_sta_begin_network(candSsid.c_str(), candPwd.c_str(), -1);
                int c = 0;
                while (WiFi.status() != WL_CONNECTED && c < 20) { vTaskDelay(pdMS_TO_TICKS(500)); c++; }
                if (WiFi.status() == WL_CONNECTED) {
                    storage.setWifiCredentials(candSsid, candPwd);
                    wifi_on_connected_hooks();
                    break;
                }
            }
        }
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        ota_ui_show_then_dismiss(ML("خطا: وای فای متصل نیست", "Error: WiFi not connected", "Hata: WiFi bagli degil"), 5000);
        g_inst_scan_busy = false;
        return false;
    }

    String targetUrl = url;
    targetUrl.trim();
    if (targetUrl.startsWith("https://ota.haffler-infi.ir")) {
        targetUrl.replace("https://", "http://");
    } else if (!targetUrl.startsWith("http://") && !targetUrl.startsWith("https://")) {
        targetUrl = "http://" + targetUrl;
    }

    ota_ui_begin_install_progress();
    vTaskDelay(pdMS_TO_TICKS(280));

    bool ok = execute_ota_stream(targetUrl);
    if (!ok && targetUrl.startsWith("https://")) {
        String plainFallback = targetUrl;
        plainFallback.replace("https://", "http://");
        ok = execute_ota_stream(plainFallback);
    }
    if (!ok && !targetUrl.endsWith(".bin")) {
        String fallbackUrl = targetUrl;
        while (fallbackUrl.endsWith("/")) fallbackUrl.remove(fallbackUrl.length() - 1);
        if (fallbackUrl.endsWith("/firmware")) fallbackUrl += ".bin";
        else fallbackUrl += "/firmware.bin";
        ok = execute_ota_stream(fallbackUrl);
        if (!ok && fallbackUrl.startsWith("https://")) {
            fallbackUrl.replace("https://", "http://");
            ok = execute_ota_stream(fallbackUrl);
        }
    }

    if (!ok) {
        ota_ui_show_then_dismiss(ML("خطا در دانلود از آدرس مستقیم", "Error downloading from URL", "URL'den indirme hatasi"), 5000);
    }
    g_inst_scan_busy = false;
    return ok;
}

// ===== بروزرسانیِ درجای صفحه اطلاعات: چکِ نسخه + (در صورت تأیید) نصب =====
// از داخل ota_task و پس از اتصالِ وای‌فای صدا زده می‌شود. UI توسط lv_timer در main.cpp رانده می‌شود.
static void self_update_check(void)
{
    if (!g_self_update_session) return;
    g_su_confirm = false;
    g_su_decline = false;
    g_su_fail_code = 0;
    g_su_version[0] = '\0';
    g_su_state = SU_CHECKING;

    // کاندیداهای مدل جوش تک‌سر
    const char *candModelsSU[] = {
        "upvc%2Fwelding-single-head",
        "welding-single-head",
        "upvc%2FWelding_single_head",
        "Welding_single_head",
        NULL
    };

    String latest = "";
    String expectedSha = "";
    String usedCand = "";
    bool gotLatest = false;

    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    secureClient.setTimeout(8);
    secureClient.setNoDelay(true);
    WiFiClient plainHttpClient;   // scope-level: lifetime matches http object, no dangling ptr
    plainHttpClient.setTimeout(6);
    plainHttpClient.setNoDelay(true);
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setReuse(false);

    for (int ci = 0; candModelsSU[ci] != NULL && !gotLatest; ci++) {
        String versionUrl = ota_url(String(candModelsSU[ci]), "lastfile");
        if (versionUrl.startsWith("http://")) {
            http.end();
            if (http.begin(plainHttpClient, versionUrl)) {
                http.setConnectTimeout(5000);
                http.setTimeout(5000);
                int code2 = http.GET();
                if (!g_self_update_session) { http.end(); g_su_state = SU_IDLE; return; }
                if (code2 == 200) {
                    String rawBody = http.getString();
                    LastfileInfo lf;
                    config_parse_lastfile(rawBody, lf);
                    String ver = lf.ver.length() > 0 ? lf.ver : extract_version_from_lastfile(rawBody);
                    if (ver.length() > 0) {
                        latest = ver;
                        expectedSha = lf.sha256;
                        usedCand = String(candModelsSU[ci]);
                        gotLatest = true;
                        Serial.printf("[SELF-UPD] got lastfile via HTTP from %s -> ver=%s sha=%s\n", versionUrl.c_str(), latest.c_str(), expectedSha.c_str());
                    }
                }
                http.end();
            }
        } else {
            http.end();
            if (http.begin(secureClient, versionUrl)) {
                http.setConnectTimeout(8000);
                http.setTimeout(8000);
                int httpCode = http.GET();
                if (!g_self_update_session) { http.end(); secureClient.stop(); g_su_state = SU_IDLE; return; }
                if (httpCode == 200) {
                    String rawBody = http.getString();
                    LastfileInfo lf;
                    config_parse_lastfile(rawBody, lf);
                    String ver = lf.ver.length() > 0 ? lf.ver : extract_version_from_lastfile(rawBody);
                    if (ver.length() > 0) {
                        latest = ver;
                        expectedSha = lf.sha256;
                        usedCand = String(candModelsSU[ci]);
                        gotLatest = true;
                        Serial.printf("[SELF-UPD] got lastfile from %s -> ver=%s sha=%s\n", versionUrl.c_str(), latest.c_str(), expectedSha.c_str());
                    }
                }
                http.end();
            }
            if (!gotLatest) {
                String httpUrl = versionUrl;
                if (httpUrl.startsWith("https://")) httpUrl.replace("https://", "http://");
                http.end();
                if (http.begin(plainHttpClient, httpUrl)) {
                    http.setConnectTimeout(5000);
                    http.setTimeout(5000);
                    int code2 = http.GET();
                    if (!g_self_update_session) { http.end(); secureClient.stop(); g_su_state = SU_IDLE; return; }
                    if (code2 == 200) {
                        String rawBody = http.getString();
                        LastfileInfo lf;
                        config_parse_lastfile(rawBody, lf);
                        String ver = lf.ver.length() > 0 ? lf.ver : extract_version_from_lastfile(rawBody);
                        if (ver.length() > 0) {
                            latest = ver;
                            expectedSha = lf.sha256;
                            usedCand = String(candModelsSU[ci]);
                            gotLatest = true;
                            Serial.printf("[SELF-UPD] got lastfile via HTTP from %s -> ver=%s sha=%s\n", httpUrl.c_str(), latest.c_str(), expectedSha.c_str());
                        }
                    }
                    http.end();
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    secureClient.stop();

    if (!gotLatest) {
        g_su_fail_code = SU_FAIL_SERVER;
        g_su_state = SU_FAIL;
        return;
    }
    if (!g_self_update_session) { g_su_state = SU_IDLE; return; }

    bool isNewer = versionNewer(latest, String(VERSION));
    Serial.printf("[SELF-UPD] current=%s latest=%s isNewer=%d\n", VERSION, latest.c_str(), (int)isNewer);

    if (!isNewer) {
        g_su_state = SU_UPTODATE;     // به‌روز است → UI پیام می‌دهد و به صفحه نصب می‌رود
        return;
    }

    // نسخهٔ جدید موجود → از کاربر بپرس
    strncpy(g_su_version, latest.c_str(), sizeof(g_su_version) - 1);
    g_su_version[sizeof(g_su_version) - 1] = '\0';
    g_su_state = SU_NEWER_ASK;

    uint32_t waited = 0;
    while (!g_su_confirm && !g_su_decline && waited < 120000 && g_self_update_session) {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;
    }
    if (!g_self_update_session || !g_su_confirm) {
        g_su_decline = true;
        g_su_state = SU_IDLE;
        return;
    }

    // تأیید شد → نصب با موتور استریم
    if (!g_self_update_session) { g_su_state = SU_IDLE; return; }
    g_su_state = SU_INSTALLING;
    vTaskDelay(pdMS_TO_TICKS(250));

    String binUrl = ota_url(usedCand, "firmware.bin");
    Serial.println("[SELF-UPD] start streaming: " + binUrl);
    bool ok = execute_ota_stream(binUrl, expectedSha);
    if (!ok) {
        String fallbackUrl = ota_url(usedCand, "firmware");
        Serial.println("[SELF-UPD] retry with fallback firmware: " + fallbackUrl);
        ok = execute_ota_stream(fallbackUrl, expectedSha);
    }

    if (!ok) {
        g_su_fail_code = SU_FAIL_INSTALL;
        g_su_state = SU_FAIL;
    }
}

#endif
