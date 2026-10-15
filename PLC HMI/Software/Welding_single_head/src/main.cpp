#include "lv_conf.h"
#include <ArduinoJson.h>
#include "defines.h"
#include <lvgl.h>
#include "draw/lv_draw_private.h"
#include "lvgl_project_compat.h"
#include <esp_system.h>
#include <TFT_eSPI.h>
#include <bb_captouch.h>
#include "ui.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <stdio.h>

#include "WiFi.h"
#include <esp_wifi.h>
#include "eez-flow.h"
#include "ui_resume.h"
#include "boot_guard.h"
#include "vars.h"
#include "general.h"
#include "actions.h"
#include "max6675.h"
#include <RTClib.h>

#include <SPI.h>
#include <Wire.h>
#include "cloud_client.h"
#include "haffler_warranty.h"
#include "ota_pre_erase.h"
#include "fault_log.h"
#include "boot_diag.h"

// آیتم ۲: در صفحه‌های ثبت‌نامِ اولِ بوت (نام شرکت / رمز مدیر) هر بوقی (لمس + هشدارهای دما/سنسورِ پس‌زمینه)
// خاموش شود؛ باید قبل از include شدنِ tasks.h تعریف شود چون buzzer_task همان‌جا از آن استفاده می‌کند.
volatile bool g_setup_overlay_active = false;
volatile uint8_t g_buzzer_volume = 80;
// آیتم: جریانِ F5-بوت→بروزرسانی — ota_task (در tasks.h) نمی‌تواند مستقیم LVGL را لمس کند، پس فقط این
// پرچم را true می‌کند؛ ناوبریِ واقعی در lock_overlay_timer_cb (نخِ LVGL) انجام می‌شود.
static volatile bool g_ota_go_framware_request = false;
// خروجِ نرم از F5-بوت (بدون ESP.restart): ota_task با دیدن این پرچم خودش را پاک می‌کند.
volatile bool g_ota_exit_request = false;
// آیتم: جریانِ F5-بوت — بلافاصله بعدِ setup() به‌جای پریدنِ مستقیم به صفحهٔ رمز، اول لگو (splash) نگه
// داشته می‌شود (تا هیچ صفحهٔ دیگری حتی یک فریم دیده نشود) و سوییچِ واقعی با کمی تاخیر در
// lock_overlay_timer_cb انجام می‌شود — همان تایمری که تا الان LVGL خودش را می‌چرخاند.
static bool g_f5_password_pending = false;
static uint32_t g_f5_boot_tick = 0;

// پرچم سخت‌افزاری بوت با F5: فقط وقتی در لحظه روشن شدن دستگاه کلید F5 فشرده شده باشد فعال است
static volatile bool g_boot_f5_active = false;

extern "C" bool is_f5_boot_active(void)
{
    return g_boot_f5_active;
}

extern "C" void set_f5_boot_active(bool active)
{
    g_boot_f5_active = active;
}

// اسکرین‌سیورِ صفحهٔ اصلی: باید قبل از tasks.h تعریف شود چون app_task (پدال/اتصالِ وب‌اپ) از آن
// استفاده می‌کند. تعریفِ کاملِ overlay/تایمر/دیالوگِ تنظیم پایین‌تر (کنارِ سایرِ overlayها) است.
volatile uint32_t g_last_activity_ms = 0; // آخرین فعالیتِ کاربر (millis): تاچ/کلیدِ پنل/پدال/اتصالِ وب‌اپ
static bool g_screensaver_active = false;
extern "C" void screensaver_note_activity(void); // پیش‌اعلان — تعریف پایین‌تر

#include "screens.h"
#include "fonts.h"
#include "weld_quality_engine.h"
#include "predictive_thermal.h"
#include "industrial_pid.h"

WeldTelemetry g_last_weld_telemetry;
PredictiveThermalManager g_predictive_thermal;
IndustrialHeaterPID g_industrial_pid;
uint32_t g_teflon_cycles = 0;
bool g_teflon_replace_warn = false;

#include "tasks.h"
#include "storage.h"
#include <nvs_flash.h>
#include <setting.h>

#include <webApp.h>
#include "webapp_plc_sync.h"
#include "date_utils.h"
#include "build_time.h"
#include "ui_localization.h"
#include "runtime_policy.h"
#include "display_port.h"


#define ver 440  // V.4.40
/////////*****************************************///////////////////
// #define SSID_NAME "Turantek PLC 10K ⚙️ " + String(ver / 100) + "." + String((ver % 100) / 10)
// #define SSID_PASS "turantek"
// SSID بدون اموجی: بعضی گوشی‌ها/درایورها AP با UTF-8 عجیب را در لیست نشان نمی‌دهند.
#define SSID_NAME "Haffler PLC 10K " + String(ver / 100) + "." + String((ver % 100) / 10)
#define SSID_PASS "haffler.com"
//#define SSID_NAME "UPVC PLC 10K ⚙️ " + String(ver / 100) + "." + String((ver % 100) / 10)
//#define SSID_PASS "upvcplc12"
/////////*****************************************///////////////////
#define HTTP_PORT 80
#define WS_PORT 81
#define FLAG_WEB_FILE_UPLOADER true
WebApp webApp(HTTP_PORT, WS_PORT, FLAG_WEB_FILE_UPLOADER, SSID_NAME, SSID_PASS);



RTC_DS3231 rtc;
SemaphoreHandle_t adcMutex = NULL;

// ===== دسترسیِ قفل‌دار به RTC (I2C) — کش‌شده برای محافظت از هسته ۱ (Core 1 UI) =====
SemaphoreHandle_t rtcMutex = NULL;
static DateTime s_cached_rtc_dt(2026, 1, 1, 0, 0, 0);
static uint32_t s_last_rtc_read_ms = 0;

DateTime rtc_now_s() {
    uint32_t now = millis();
    // اگر کمتر از ۱۰۰۰ میلی‌ثانیه از خواندن قبلی گذشته، از کش برگردان (Zero I2C overhead on Core 1)
    if (s_last_rtc_read_ms != 0 && (uint32_t)(now - s_last_rtc_read_ms) < 1000U) {
        uint32_t elapsed_sec = (now - s_last_rtc_read_ms) / 1000U;
        return DateTime((uint32_t)(s_cached_rtc_dt.unixtime() + elapsed_sec));
    }
    if (rtcMutex && xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_cached_rtc_dt = rtc.now();
        s_last_rtc_read_ms = millis();
        xSemaphoreGive(rtcMutex);
        return s_cached_rtc_dt;
    }
    // اگر موتکس اشغال بود، با تقریب زمانی از کش بازگردان بدون بلاک کردن یا تداخل با لمس روی هسته ۱
    if (s_last_rtc_read_ms != 0) {
        uint32_t elapsed_sec = (now - s_last_rtc_read_ms) / 1000U;
        return DateTime((uint32_t)(s_cached_rtc_dt.unixtime() + elapsed_sec));
    }
    return s_cached_rtc_dt;
}
void rtc_adjust_s(const DateTime &dt) {
    if (rtcMutex && xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        rtc.adjust(dt);
        s_cached_rtc_dt = dt;
        s_last_rtc_read_ms = millis();
        xSemaphoreGive(rtcMutex);
    }
}
float rtc_temp_s() {
    static float s_cached_temp = 25.0f;
    static uint32_t s_last_temp_read_ms = 0;
    uint32_t now = millis();
    if (s_last_temp_read_ms != 0 && (uint32_t)(now - s_last_temp_read_ms) < 5000U) {
        return s_cached_temp;
    }
    if (rtcMutex && xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        s_cached_temp = rtc.getTemperature();
        s_last_temp_read_ms = millis();
        xSemaphoreGive(rtcMutex);
        return s_cached_temp;
    }
    return s_cached_temp;
}

char ota_download_model[96] = {0};
volatile bool ota_download = false;
char ota_custom_url_buf[256] = {0};
volatile bool ota_custom_download = false;
int temp_dis_2 = 1000;

// === ورود به مدِ OTAِ آزموده‌شده با ری‌استارت (دکمهٔ منوی بروزرسانی) ===
// فلگ در RAMِ RTC ذخیره می‌شود تا فقط در ری‌ستِ نرم‌افزاری بماند، نه قطعِ برق.
#define OTA_BOOT_MAGIC 0xB007C0DEu
RTC_NOINIT_ATTR uint32_t g_ota_boot_request;
// آیتم: تشخیصِ اینکه این بوتِ OTA به‌خاطرِ دکمهٔ «بروزرسانی» صفحهٔ اطلاعات است (نه F5ِ فیزیکی) —
// چون g_pending_self_check یک متغیرِ RAMِ عادی است و با ESP.restart() صفر می‌شود، این پرچم را در
// RAMِ RTC نگه می‌داریم تا بعد از ری‌استارت دوباره بخوانیمش و self_update_check() صدا زده شود
// (نه صفحهٔ کارت‌های نصبِ عمومی).
#define SELF_UPDATE_BOOT_MAGIC 0x5E1FCEC0u
RTC_NOINIT_ATTR uint32_t g_self_update_boot_request;

// === بروزرسانیِ صفحه اطلاعات (درجا، بدون ری‌استارت) ===
// state مشترک با self_update_check() در tasks.h
volatile int  g_su_state = SU_IDLE;
volatile int  g_su_fail_code = 0;
char          g_su_version[16] = {0};
volatile bool g_su_confirm = false;
volatile bool g_su_decline = false;
volatile bool g_self_update_session = false;
// اگر کاربر دکمهٔ «بروزرسانی» را درست هنگامی بزند که تسکِ selfUpdate قبلی هنوز مشغولِ پاکسازیِ
// خودش است، به‌جای بلاک‌کردنِ نخِ LVGL (که هنگِ لمس/رندر ایجاد می‌کرد)، فقط این پرچم ست می‌شود؛
// تایمرِ self_update_pending_restart_timer_cb هر بار که هندلِ تسک خالی شد خودش دوباره تلاش می‌کند.
static volatile bool g_self_update_pending_restart = false;
static TaskHandle_t selfUpdateTaskHandle = NULL;
static TaskHandle_t wifiScanStarterTaskHandle = NULL;
static bool g_rtc_lost_power_boot = false;
static volatile bool g_rtc_warn_pending = false;   // هشدارِ یک‌بارهٔ باتریِ ساعت (در overlayِ خطای سخت‌افزار نمایش داده می‌شود)
// خودآزماییِ سخت‌افزار سرِ بوت (بخش تست سخت‌افزار خودکار) — اگر هرکدام true شد، در overlayِ خطای
// سخت‌افزار (hw_selfcheck_overlay_build) روی صفحهٔ لگو گزارش می‌شود؛ کاربر درگیرِ تستِ دستی نمی‌شود.
bool g_hw_rtc_missing = false;      // ماژول RTC (DS3231) روی I2C پاسخ نداد
bool g_hw_touch_missing = false;    // کنترلرِ لمسی شناسایی نشد
extern "C" bool g_hw_thermo_missing;
extern "C" bool g_hw_thermo_sensor_open;
bool g_hw_thermo_missing = false;    // چیپ اصلاً پاسخ نمی‌دهد (خروجیِ ثابت/بی‌معنی روی SPI)
bool g_hw_thermo_sensor_open = false; // چیپ سالم است ولی سیمِ ترموکوپل وصل نیست (بیتِ D2)
// دمای داخلیِ بحرانی در همان لحظهٔ بوت (جدا از هشدارِ مرحله‌ایِ صفحهٔ اصلی):
static bool g_hw_cpu_temp_high = false;
static bool g_hw_rtc_temp_high = false;
static float g_hw_cpu_temp_val = 0, g_hw_rtc_temp_val = 0;
// کشِ دما برای منوی اطلاعات — sensor task پر می‌کند؛ LVGL دیگر I2C/mutex را در تیک بلاک نکند
volatile float g_fw_info_cpu_temp_c = 0;
volatile float g_fw_info_rtc_temp_c = 0;
volatile bool g_wifi_rescan_request = false; // درخواستِ اسکنِ مجدد (در app_task / ota_task / self_update)
volatile bool g_wifi_init_request = false;   // درخواستِ راه‌اندازیِ STA + اولین اسکن (در app_task)
volatile bool g_wifi_ota_scan_pending = false;  // اسکنِ اولِ بوتِ OTA (ota_task)
volatile bool g_wifi_ota_rescan_request = false; // دکمهٔ جستجو در حالت ota_enable
volatile uint32_t g_output_inhibit_until_ms = 0; // بعد از خروج از OTA چند ثانیه خروجی‌ها قفلِ خاموش بمانند
volatile uint32_t g_ota_return_sanitize_until_ms = 0; // پاکسازی UI/کلیدها بعد از برگشت از OTA
volatile int  g_su_scan_rc = 99;             // نتیجهٔ شروعِ اسکن (برای تشخیص)

// رمز مادر (مستر/ادمین) — همه‌جا که رمز خواسته می‌شود باز می‌کند (آیتم ۷ لیست اصلاحات).
static const char *const MASTER_ADMIN_PASSWORD = "12773";
// رمز موقتِ یک‌بارمصرف — ۱۰ روز مهلت می‌دهد، بعد فقط با رمز مدیر/مستر (آیتم ۶).
static const char *const TEMP_UNLOCK_PASSWORD = "112233";

// JDNِ امروز از روی RTC (برای روزشمارِ قفل). در صورت نبودِ RTC، 0 برمی‌گرداند.
static long today_jdn_from_rtc()
{
    DateTime now = rtc_now_s();
    if (now.year() < 2020) return 0; // RTC نامعتبر
    return Storage::gregorianToJDN(now.year(), now.month(), now.day());
}

// متغیرهای پلِ قفل (تعریف؛ اعلانشان در defines.h). آیتم ۴/۸.
extern "C" {
volatile int g_lock_remaining_days = -1;
volatile int g_device_locked = 0;
volatile int g_temp_warn_request = 0;   // آیتم ۲
volatile int g_temp_override_start = 0; // آیتم ۲
// پیغام پایان جوش (#۳)
volatile int g_weld_done_request = 0;
volatile unsigned long g_weld_done_elapsed_ms = 0;
volatile int g_weld_done_frames = 0;
volatile int g_sensor_err_request = 0;
volatile int g_sensor_err_id = 0;
volatile int g_sensor_lamp_step = -1;
volatile int g_sensor_lamp_pct = 0;
volatile bool g_ssr_fault = false;
volatile bool g_ssr_warn_dismissed = false;
volatile int g_internal_temp_band = 0;
volatile int g_lamp_pct_shablon = -1;
volatile int g_lamp_pct_table = -1;
volatile int g_lamp_pct_element = -1;
volatile bool g_force_lamps_off = false;
volatile int g_sens_raw_shablon = 0;
volatile int g_sens_raw_element = 0;
volatile int g_sens_raw_table = 0;
volatile bool g_sensor_err_both = false;
volatile int g_weld_done_in_frame = 0;
volatile int g_pedal_outside_menu_warn_request = 0;
bool g_splash_variant = false;
}
// آیتم: جابجاییِ پایه از منوی اطلاعات — تعریفِ واقعیِ متغیرهای پایه (پیش‌فرضِ سیم‌کشیِ کارخانه).
// storage.loadPinMap() این‌ها را پیش از setupPin() با مقدارِ ذخیره‌شده بازنویسی می‌کند.
int SSR_1 = DEFAULT_SSR_1, TABLE = DEFAULT_TABLE, PRESS_LEFT = DEFAULT_PRESS_LEFT, PRESS_RIGHT = DEFAULT_PRESS_RIGHT;
int ELEMENT = DEFAULT_ELEMENT, SHABLON = DEFAULT_SHABLON, ZARBEH1 = DEFAULT_ZARBEH1, ZARBEH2 = DEFAULT_ZARBEH2;
int EMERGENCY = DEFAULT_EMERGENCY, PEDAL = DEFAULT_PEDAL;
int SENSOR_ELEMENT = DEFAULT_SENSOR_ELEMENT, SENSOR_SHABLON = DEFAULT_SENSOR_SHABLON, SENSOR_TABLE = DEFAULT_SENSOR_TABLE;
static volatile uint16_t panel_nav_event = 0;
static lv_obj_t *panel_nav_focus = NULL;
static bool panel_nav_active = false;
static int panel_nav_screen = -1;
static uint32_t panel_nav_last_activity = 0;        // مورد ۴: زمانِ آخرین فعالیتِ کلید
volatile bool panel_nav_touch_exit = false;  // مورد ۴: تاچ شد → از حالت MOD خارج شو

extern "C" lv_obj_t *sg_nav_wifi_switch(void);
extern "C" lv_obj_t *sg_nav_help_open_btn(void);
extern "C" lv_obj_t *sg_nav_help_close_btn(void);
extern "C" lv_obj_t *sg_nav_factory_reset_btn(void);
extern "C" lv_obj_t *sg_nav_reset_cancel_btn(void);
extern "C" lv_obj_t *sg_nav_reset_confirm_btn(void);
extern "C" bool sg_nav_help_is_open(void);
extern "C" bool sg_nav_reset_is_open(void);
extern "C" lv_obj_t *fw_nav_frames_label(void);
extern "C" lv_obj_t *fw_nav_frames_reset_ok(void);
extern "C" lv_obj_t *fw_nav_frames_reset_cancel(void);
extern "C" bool fw_nav_frames_reset_is_open(void);
extern "C" lv_obj_t *fw_nav_update_btn(void);
extern "C" lv_obj_t *fw_nav_firmware_back_btn(void);
extern "C" lv_obj_t *fw_nav_firmware_quick_update_btn(void);
extern "C" lv_obj_t *fw_nav_out_lamp(int idx);
extern "C" lv_obj_t *fw_nav_out_label(int idx);
extern "C" lv_obj_t *fw_nav_in_label(int idx); // آیتم: نامِ ورودی‌ها هم مثلِ خروجی‌ها قابلِ‌ناوبری با کلید MODE
extern "C" lv_obj_t *fw_nav_outputs_hdr(void); // آیتم: عنوانِ ستونِ «خروجی» (نقشِ ریستِ پایه‌ها) در ناوبری MODE
extern "C" lv_obj_t *fw_nav_inputs_hdr(void);  // آیتم: عنوانِ ستونِ «ورودی»
extern "C" lv_obj_t *fw_nav_warranty_btn(void);
extern "C" lv_obj_t *fw_nav_wifi_back_btn(void);
extern "C" lv_obj_t *fw_nav_frames_title(void);
extern "C" void apply_language_now(int32_t lang);
// آیتم: پیش‌اعلانِ ناوبریِ کلیدِ MODE برایِ دیالوگ‌های جدیدِ جابجایی/ریستِ پایه — تعریفِ کامل پایین‌تر
static bool io_remap_is_open(void);
static int  io_remap_nav_get(lv_obj_t **out, int max);
static void io_remap_destroy(void); // آیتم: برای بستنِ خودکارِ ۸ثانیه‌ای از lock_overlay_timer_cb
static bool io_reset_is_open(void);
static lv_obj_t *io_reset_ok_btn_get(void);
static lv_obj_t *io_reset_cancel_btn_get(void);
extern "C" void io_remap_dialog_open(int isOutput, int idx);
extern "C" void io_reset_dialog_open(int isOutput); // آیتم: از ناوبری MODE روی عنوانِ ستون‌ها هم صدا زده می‌شود
// آیتم: پیش‌اعلانِ ناوبریِ کلیدِ MOD برایِ overlayِ بروزرسانیِ درجا (تعریفِ کامل پایین‌تر، کنارِ su_overlay_*)
static bool su_overlay_is_open(void);
static lv_obj_t *su_nav_confirm(void);
static lv_obj_t *su_nav_decline(void);
extern "C" bool company_overlay_is_open(void);
extern "C" int  company_overlay_nav_get(lv_obj_t **out, int max);
extern "C" bool admin_setup_overlay_is_open(void);
extern "C" int  admin_setup_overlay_nav_get(lv_obj_t **out, int max);
extern "C" lv_obj_t *main_nav_daily_stats_close(void);
extern "C" lv_obj_t *main_nav_quality_warn_close(void);
extern "C" lv_obj_t *main_nav_numeric_kb(void);
extern "C" void main_open_daily_stats(void);
static lv_obj_t *main_nav_temp_cancel(void);
static lv_obj_t *main_nav_temp_confirm(void);
extern "C" bool wifi_is_busy_searching_or_connecting(void);
extern "C" void wifi_abort_current_operation(void);
extern "C" void wifi_ui_request_password_panel(void);
extern "C" void check_ui_requested_toast(void);
static lv_obj_t *main_nav_weld_done_close(void);
static lv_obj_t *main_nav_sensor_err_close(void);
extern "C" lv_obj_t *jack_nav_help_btn(void);
extern "C" lv_obj_t *jack_nav_help_close(void);
extern "C" bool      jack_nav_help_is_open(void);
extern "C" lv_obj_t *jack_nav_err_close(void);
extern "C" bool      jack_nav_err_is_open(void);
extern "C" lv_obj_t *sa_nav_help_close(void);
extern "C" bool      sa_nav_help_is_open(void);
extern "C" lv_obj_t *sa_nav_save_default(void);
extern "C" lv_obj_t *sa_nav_save_default_confirm(void);
extern "C" lv_obj_t *sa_nav_save_default_cancel(void);
extern "C" bool      sa_nav_save_default_confirm_is_open(void);
extern "C" lv_obj_t *sa_nav_help_open(void);
extern "C" lv_obj_t *mt_nav_close(void);
extern "C" lv_obj_t *mt_nav_help_open(void);
extern "C" lv_obj_t *mt_nav_help_close(void);
extern "C" bool      mt_nav_help_is_open(void);
extern "C" int       mt_nav_dialog_get(lv_obj_t **out, int max);
extern "C" bool      multi_timer_dialog_can_open(void);
extern "C" void      settings_reset_open_panels(void);

static bool panel_nav_is_sidebar_button(lv_obj_t *obj);
// ناوبریِ کیبردِ قفل (تعریف پایین‌تر؛ از panel_nav_timer_cb روی تاسکِ LVGL فراخوانی می‌شوند)
extern "C" void lock_nav_move(void);
extern "C" void lock_nav_select(void);
extern "C" void lock_nav_delete(void);

void queue_panel_navigation(int row, int key)
{
    panel_nav_event = (uint16_t)(((row & 0xFF) << 8) | (key & 0xFF));
}

bool panel_navigation_is_active(void)
{
    return panel_nav_active && panel_nav_screen == eez_flow_get_current_screen();
}

static lv_obj_t *panel_nav_buddy = NULL;   // مورد ۲: لامپی که همراهِ نامِ خروجی هایلایت می‌شود
static lv_obj_t *panel_nav_rect = NULL;

static void panel_nav_mark(lv_obj_t *obj)
{
    if (panel_nav_focus && lv_obj_is_valid(panel_nav_focus)) {
        lv_obj_set_style_outline_width(panel_nav_focus, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_transform_height(panel_nav_focus, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        // مورد ۴: اگر فوکوسِ قبلی روی کیبرد/btnmatrix بود، هایلایتِ کلیدِ انتخاب‌شده
        // با outline پاک نمی‌شود؛ باید selected_btn و stateِ FOCUSED صریحاً پاک شوند،
        // وگرنه بعد از تایم‌اوتِ ۱۰ ثانیه نشانگرِ روی عدد/کلید روی صفحه می‌ماند.
        if (lv_obj_check_type(panel_nav_focus, &lv_keyboard_class) ||
            lv_obj_check_type(panel_nav_focus, &lv_btnmatrix_class)) {
            lv_btnmatrix_set_selected_btn(panel_nav_focus, LV_BTNMATRIX_BTN_NONE);
            lv_obj_clear_state(panel_nav_focus, LV_STATE_FOCUSED);
        }
    }
    if (panel_nav_buddy && lv_obj_is_valid(panel_nav_buddy)) {
        lv_obj_set_style_outline_width(panel_nav_buddy, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (panel_nav_rect && lv_obj_is_valid(panel_nav_rect)) {
        lv_obj_del(panel_nav_rect);
    }
    panel_nav_rect = NULL;
    panel_nav_buddy = NULL;
    panel_nav_focus = obj;
    if (obj) {
        panel_nav_screen = eez_flow_get_current_screen();
        // آیتم ۸: قبل از خواندن مختصات، layout را نهایی کن تا کادرِ سلکت در اولین زدنِ MODE
        // (بلافاصله پس از بارگذاریِ صفحه) جای درست ساخته شود و گذرا/پاک‌شونده نباشد.
        lv_obj_update_layout(obj);
        if (obj == sg_nav_help_close_btn() || obj == jack_nav_help_close() || obj == sa_nav_help_close() || obj == mt_nav_help_close()) {
            lv_area_t a;
            lv_obj_get_coords(obj, &a);
            // آیتم ۷: ضربدرِ راهنمای تنظیماتِ عمومی درست است (−14,−6)؛ راهنمای مدیر و زمان‌بندیِ جک
            // ۱۶px چپ‌تر و ۴px پایین‌تر تا دقیقاً روی ضربدر بنشیند → هم‌راستا با تنظیماتِ عمومی.
            int dx = -14, dy = -6;
            if (obj == jack_nav_help_close()) { dx = 2; dy = 5; }  // نشانگر ضربدرِ راهنمای جک: ۶px راست‌تر
            if (obj == sa_nav_help_close()) { dx = 7; dy = 2; }  // نشانگر راهنمای مدیر: ۴px چپ‌تر
            if (obj == mt_nav_help_close()) { dx = 12; dy = -2; }
            panel_nav_rect = lv_obj_create(lv_layer_top());
            lv_obj_clear_flag(panel_nav_rect, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(panel_nav_rect, a.x1 + dx, a.y1 + dy);
            lv_obj_set_size(panel_nav_rect, (a.x2 - a.x1 + 1) + 4, (a.y2 - a.y1 + 1) + 4);
            lv_obj_set_style_bg_opa(panel_nav_rect, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_nav_rect, lv_color_hex(0xff9b00ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_nav_rect, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_nav_rect, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_nav_rect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_UPDATE &&
            (obj == fw_nav_frames_label() || obj == fw_nav_frames_title() || obj == fw_nav_update_btn() || obj == fw_nav_warranty_btn())) {
            lv_area_t a;
            lv_obj_get_coords(obj, &a);
            panel_nav_rect = lv_obj_create(lv_layer_top());
            lv_obj_clear_flag(panel_nav_rect, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(panel_nav_rect, a.x1 - 2, a.y1 + 2);
            lv_obj_set_size(panel_nav_rect, (a.x2 - a.x1 + 1) + 4, (a.y2 - a.y1 + 1) + 4);
            lv_obj_set_style_bg_opa(panel_nav_rect, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_nav_rect, lv_color_hex(0xff9b00ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_nav_rect, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_nav_rect, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_nav_rect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_UPDATE &&
            (obj == fw_nav_outputs_hdr() || obj == fw_nav_inputs_hdr())) {
            // آیتم: نشانگرِ MOD رویِ عنوانِ «ورودی»/«خروجی» کمی بالاترِ نوشته می‌نشست — ۶px پایین‌تر آمد
            lv_area_t a;
            lv_obj_get_coords(obj, &a);
            panel_nav_rect = lv_obj_create(lv_layer_top());
            lv_obj_clear_flag(panel_nav_rect, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(panel_nav_rect, a.x1 - 2, a.y1 + 6);
            lv_obj_set_size(panel_nav_rect, (a.x2 - a.x1 + 1) + 4, (a.y2 - a.y1 + 1));
            lv_obj_set_style_bg_opa(panel_nav_rect, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_nav_rect, lv_color_hex(0xff9b00ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_nav_rect, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_nav_rect, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_nav_rect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU &&
            (obj == objects.setting_imageicon || obj == objects.temputre_arcs || obj == objects.obj23 || obj == objects.obj26)) {
            lv_area_t a;
            lv_obj_get_coords(obj, &a);
            int left = (obj == objects.setting_imageicon) ? -2 : 0;
            int top = (obj == objects.setting_imageicon) ? -2 : 0;
            int right = (obj == objects.setting_imageicon) ? 2 : 0;
            int bottom = (obj == objects.setting_imageicon) ? 4 : -10;
            panel_nav_rect = lv_obj_create(lv_layer_top());
            lv_obj_clear_flag(panel_nav_rect, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(panel_nav_rect, a.x1 + left, a.y1 + top);
            lv_obj_set_size(panel_nav_rect, (a.x2 - a.x1 + 1) + right - left, (a.y2 - a.y1 + 1) + bottom - top);
            lv_obj_set_style_bg_opa(panel_nav_rect, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_nav_rect, lv_color_hex(0xff9b00ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_nav_rect, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_nav_rect, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_nav_rect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_SETTING_JACK_SETTING && obj == jack_nav_help_btn()) {
            // دکمهٔ راهنمای جک: قابِ سلکت دقیقاً روی خودِ دکمه (متقارن، مثل بقیهٔ کلیدها)
            lv_area_t a;
            lv_obj_get_coords(obj, &a);
            panel_nav_rect = lv_obj_create(lv_layer_top());
            lv_obj_clear_flag(panel_nav_rect, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(panel_nav_rect, a.x1 - 2, a.y1 - 2);
            lv_obj_set_size(panel_nav_rect, (a.x2 - a.x1 + 1) + 4, (a.y2 - a.y1 + 1) + 4);
            lv_obj_set_style_bg_opa(panel_nav_rect, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_nav_rect, lv_color_hex(0xff9b00ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_nav_rect, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_nav_rect, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_nav_rect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_outline_color(obj, lv_color_hex(0xff9b00ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            int pad = 0;
            if (panel_nav_is_sidebar_button(obj)) pad = -4;
            lv_obj_set_style_outline_pad(obj, pad, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (panel_nav_rect && lv_obj_is_valid(panel_nav_rect)) lv_obj_move_foreground(panel_nav_rect);
        // مورد ۲: انتخاب همیشه کامل به نما اسکرول شود (نام‌های خروجی داخل کانتینرِ اسکرول)
        lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON);
    }
}

static void panel_nav_exit_mode()
{
    panel_nav_active = false;
    panel_nav_screen = -1;
    panel_nav_mark(NULL);
}

static bool panel_nav_push_if_valid(lv_obj_t **items, int *count, int max_count, lv_obj_t *obj)
{
    if (!obj || *count >= max_count) return false;
    if (!lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) || lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;
    if (lv_obj_get_width(obj) <= 0 || lv_obj_get_height(obj) <= 0) return false;
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    if (coords.x2 < 0 || coords.y2 < 0 || coords.x1 > 799 || coords.y1 > 479) return false;
    lv_obj_t *p = lv_obj_get_parent(obj);
    while (p) {
        if (lv_obj_has_flag(p, LV_OBJ_FLAG_HIDDEN)) return false;
        p = lv_obj_get_parent(p);
    }
    for (int i = 0; i < *count; ++i) {
        if (items[i] == obj) return false;
    }
    items[(*count)++] = obj;
    return true;
}

static bool panel_nav_is_sidebar_button(lv_obj_t *obj)
{
    if (!obj) return false;
    return obj == objects.btn_f1_mode || obj == objects.btn_f2_mode || obj == objects.btn_f3_mode ||
           obj == objects.btn_f4_mode || obj == objects.btn_f5_mode ||
           obj == objects.btn_f1_mode_1 || obj == objects.btn_f2_mode_1 || obj == objects.btn_f3_mode_1 ||
           obj == objects.btn_f4_mode_1 || obj == objects.btn_f5_mode_1 ||
           obj == objects.btn_f1_mode_2 || obj == objects.btn_f2_mode_2 || obj == objects.btn_f3_mode_2 ||
           obj == objects.btn_f4_mode_2 || obj == objects.btn_f5_mode_2 ||
           obj == objects.btn_f1_mode_3 || obj == objects.btn_f2_mode_3 || obj == objects.btn_f3_mode_3 ||
           obj == objects.btn_f4_mode_3 || obj == objects.btn_f5_mode_3 ||
           obj == objects.btn_f1_mode_4 || obj == objects.btn_f2_mode_4 || obj == objects.btn_f3_mode_4 ||
           obj == objects.btn_f4_mode_4 || obj == objects.btn_f5_mode_4;
}

static void panel_nav_sort_top_left(lv_obj_t **items, int count)
{
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            int yi = lv_obj_get_y(items[i]), yj = lv_obj_get_y(items[j]);
            int xi = lv_obj_get_x(items[i]), xj = lv_obj_get_x(items[j]);
            if (yj < yi || (yj == yi && xj < xi)) {
                lv_obj_t *tmp = items[i];
                items[i] = items[j];
                items[j] = tmp;
            }
        }
    }
}

static void panel_nav_append_sidebar(int screen, lv_obj_t **items, int *count, int max_count)
{
    if (screen == SCREEN_ID_SCREEN_SETTING_GENERAL) {
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f1_mode_1);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f2_mode_1);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f3_mode_1);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f4_mode_1);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f5_mode_1);
    } else if (screen == SCREEN_ID_SCREEN_SETTING_JACK_SETTING) {
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f1_mode_2);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f2_mode_2);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f3_mode_2);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f4_mode_2);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f5_mode_2);
    } else if (screen == SCREEN_ID_SCREEN_SETTING_ADMIN) {
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f1_mode_3);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f2_mode_3);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f3_mode_3);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f4_mode_3);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f5_mode_3);
    } else if (screen == SCREEN_ID_SCREEN_UPDATE) {
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f1_mode_4);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f2_mode_4);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f3_mode_4);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f4_mode_4);
        panel_nav_push_if_valid(items, count, max_count, objects.btn_f5_mode_4);
    }
}

static bool panel_nav_is_adjustable(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) || lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;
    bool clickable = lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    if (lv_obj_check_type(obj, &lv_label_class) || lv_obj_check_type(obj, &lv_img_class)) return clickable;
    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class) || lv_obj_check_type(obj, &lv_arc_class) ||
        lv_obj_check_type(obj, &lv_dropdown_class) || lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE)) return true;
    return clickable;
}

static void panel_nav_collect(lv_obj_t *parent, lv_obj_t **items, int *count, int max_count)
{
    uint32_t n = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < n && *count < max_count; ++i) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (panel_nav_is_adjustable(child)) {
            panel_nav_push_if_valid(items, count, max_count, child);
        }
        panel_nav_collect(child, items, count, max_count);
    }
}

static int panel_nav_collect_for_screen(int screen, lv_obj_t **items, int max_count)
{
    int count = 0;
    // آیتم: صفحه‌های ثبت‌نامِ اولِ بوت (نام شرکت/رمز مدیر) overlayِ lv_layer_top هستند، نه یک SCREEN_ID
    // خاص — پس مستقلِ از screen اول رصد می‌شوند تا ناوبریِ MODE در آن‌ها هم کار کند.
    if (company_overlay_is_open()) {
        lv_obj_t *navItems[6];
        int n = company_overlay_nav_get(navItems, 6);
        for (int i = 0; i < n; ++i) panel_nav_push_if_valid(items, &count, max_count, navItems[i]);
        return count;
    }
    if (admin_setup_overlay_is_open()) {
        lv_obj_t *navItems[6];
        int n = admin_setup_overlay_nav_get(navItems, 6);
        for (int i = 0; i < n; ++i) panel_nav_push_if_valid(items, &count, max_count, navItems[i]);
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_MENU) {
        if (main_nav_temp_cancel() || main_nav_temp_confirm()) {
            panel_nav_push_if_valid(items, &count, max_count, main_nav_temp_confirm());
            panel_nav_push_if_valid(items, &count, max_count, main_nav_temp_cancel());
            return count;
        }
        if (main_nav_weld_done_close()) {
            panel_nav_push_if_valid(items, &count, max_count, main_nav_weld_done_close());
            return count;
        }
        if (main_nav_sensor_err_close()) {
            panel_nav_push_if_valid(items, &count, max_count, main_nav_sensor_err_close());
            return count;
        }
        if (main_nav_daily_stats_close()) {
            panel_nav_push_if_valid(items, &count, max_count, main_nav_daily_stats_close());
            return count;
        }
        if (main_nav_quality_warn_close()) {
            panel_nav_push_if_valid(items, &count, max_count, main_nav_quality_warn_close());
            return count;
        }
        if (main_nav_numeric_kb()) {   // کیبردِ وسطِ گیج باز است → فقط همان (MOD بین کلیدها، START زدن، STOP پاک)
            panel_nav_push_if_valid(items, &count, max_count, main_nav_numeric_kb());
            return count;
        }
        panel_nav_push_if_valid(items, &count, max_count, objects.setting_imageicon);
        panel_nav_push_if_valid(items, &count, max_count, objects.temputre_arcs);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj23);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj26);
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_SETTING_GENERAL) {
        // دیالوگ‌های مودال: فقط دکمه‌های خودِ دیالوگ (بدون نوار کناری) تا MOD مستقیم برود روی آن‌ها
        if (sg_nav_help_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, sg_nav_help_close_btn());
            return count;
        }
        if (sg_nav_reset_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, sg_nav_reset_confirm_btn());
            panel_nav_push_if_valid(items, &count, max_count, sg_nav_reset_cancel_btn());
            return count;
        }
        panel_nav_append_sidebar(screen, items, &count, max_count);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj33);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj35);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj36);
        panel_nav_push_if_valid(items, &count, max_count, sg_nav_wifi_switch());
        panel_nav_push_if_valid(items, &count, max_count, objects.general_speaker1);
        panel_nav_push_if_valid(items, &count, max_count, objects.general_auto_start);
        panel_nav_push_if_valid(items, &count, max_count, objects.general_finaljeck);
        panel_nav_push_if_valid(items, &count, max_count, objects.general_limit);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj39);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj38);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj37);
        panel_nav_push_if_valid(items, &count, max_count, sg_nav_factory_reset_btn());
        panel_nav_push_if_valid(items, &count, max_count, sg_nav_help_open_btn());
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_SETTING_JACK_SETTING) {
        // مورد ۵: دیالوگِ خطای زمان جک باز → فقط × (قابل بستن با MOD)
        if (jack_nav_err_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, jack_nav_err_close());
            return count;
        }
        // مورد ۲: راهنمای جک باز → فقط × (قابل بستن با MOD)
        if (jack_nav_help_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, jack_nav_help_close());
            return count;
        }
        bool edit_open = objects.panle_jack_modify && !lv_obj_has_flag(objects.panle_jack_modify, LV_OBJ_FLAG_HIDDEN);
        if (edit_open) {
            // مورد ۵: فقط کلیدهای منفی/مثبت (و ذخیره) — نشانگر روی جای اضافه نرود
            panel_nav_push_if_valid(items, &count, max_count, objects.obj61);
            panel_nav_push_if_valid(items, &count, max_count, objects.obj63);
            panel_nav_push_if_valid(items, &count, max_count, objects.jack_bargasht_m);
            panel_nav_push_if_valid(items, &count, max_count, objects.jack_bargasht_p);
            panel_nav_push_if_valid(items, &count, max_count, objects.obj66);
            return count;
        }
        panel_nav_append_sidebar(screen, items, &count, max_count);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj45);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj48);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj54);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj51);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj57);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj59);
        panel_nav_push_if_valid(items, &count, max_count, jack_nav_help_btn());  // مورد ۲: دکمهٔ راهنمای جک
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_ADMIN_PASSWORD) {
        // مورد ۳: مستقیم روی کیبردِ رمز (بدون نوار کناری) تا با یک MOD آمادهٔ تایپ شود
        panel_nav_push_if_valid(items, &count, max_count, objects.admin_pass_prof_keyboard);
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_SETTING_ADMIN) {
        if (multi_timer_dialog_is_open()) {
            if (mt_nav_help_is_open()) {
                panel_nav_push_if_valid(items, &count, max_count, mt_nav_help_close());
                return count;
            }
            lv_obj_t *mtNav[24];
            int mn = mt_nav_dialog_get(mtNav, 24);
            for (int i = 0; i < mn; ++i)
                panel_nav_push_if_valid(items, &count, max_count, mtNav[i]);
            return count;
        }
        if (sa_nav_help_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, sa_nav_help_close());
            return count;
        }
        if (sa_nav_save_default_confirm_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, sa_nav_save_default_confirm());
            panel_nav_push_if_valid(items, &count, max_count, sa_nav_save_default_cancel());
            return count;
        }
        bool pass_open = objects.setting_admin_pass_change_panle && !lv_obj_has_flag(objects.setting_admin_pass_change_panle, LV_OBJ_FLAG_HIDDEN);
        if (pass_open) {
            // مورد ۴: پنلِ تغییرِ رمز باز است → مستقیم روی کیبرد (قبل از نوار کناری)
            panel_nav_push_if_valid(items, &count, max_count, objects.change_value_keyboard1);
            return count;
        }
        panel_nav_append_sidebar(screen, items, &count, max_count);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj68);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj69);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj70);
        panel_nav_push_if_valid(items, &count, max_count, objects.btn_setting_admin_thermocouple_k);
        panel_nav_push_if_valid(items, &count, max_count, objects.btn_setting_admin_thermocouple_j);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj73);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj74);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj79);
        panel_nav_push_if_valid(items, &count, max_count, objects.obj77);
        // دو کلیدِ پایین: راهنما + ذخیره به‌عنوان تنظیمات کارخانه
        panel_nav_push_if_valid(items, &count, max_count, sa_nav_help_open());
        panel_nav_push_if_valid(items, &count, max_count, sa_nav_save_default());
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_UPDATE || screen == SCREEN_ID_SCREEN_FRAMWARE) {
        if (screen == SCREEN_ID_SCREEN_UPDATE) {
            // آیتم: دیالوگِ جابجاییِ پایه یا دیالوگِ ریستِ پایه‌ها — اگر باز است، فقط دکمه‌های خودش
            if (io_remap_is_open()) {
                lv_obj_t *navItems[12];
                int n = io_remap_nav_get(navItems, 12);
                for (int i = 0; i < n; ++i) panel_nav_push_if_valid(items, &count, max_count, navItems[i]);
                return count;
            }
            if (io_reset_is_open()) {
                panel_nav_push_if_valid(items, &count, max_count, io_reset_ok_btn_get());
                panel_nav_push_if_valid(items, &count, max_count, io_reset_cancel_btn_get());
                return count;
            }
            // مورد ۳: دیالوگ ریست چهارچوب — فقط تأیید/انصراف (قبل از نوار کناری تا MOD مستقیم برود روی آن‌ها)
            if (fw_nav_frames_reset_is_open()) {
                panel_nav_push_if_valid(items, &count, max_count, fw_nav_frames_reset_ok());
                panel_nav_push_if_valid(items, &count, max_count, fw_nav_frames_reset_cancel());
                return count;
            }
            panel_nav_append_sidebar(screen, items, &count, max_count);
            // ترتیب: گارانتی (اگر کلیدش هست) → به‌روزرسانی → نوشتهٔ چهارچوب (نه عدد) → نام‌های خروجی/ورودی
            panel_nav_push_if_valid(items, &count, max_count, fw_nav_warranty_btn());
            panel_nav_push_if_valid(items, &count, max_count, fw_nav_update_btn());
            panel_nav_push_if_valid(items, &count, max_count, fw_nav_frames_title());
            // آیتم: اول عنوان‌های «خروجی»/«ورودی» (START/STOP = دیالوگِ ریستِ پایه‌های آن دسته به کارخانه)،
            // بعد ردیف‌های اتصالات (ورودی/خروجی‌ها)
            panel_nav_push_if_valid(items, &count, max_count, fw_nav_outputs_hdr());
            panel_nav_push_if_valid(items, &count, max_count, fw_nav_inputs_hdr());
            // مورد ۲: هر ردیف خروجی/ورودی یک توقف (نامِ آن)؛ START=تستِ روشن/خاموش، STOP=جابجاییِ پایه
            for (int i = 0; i < 8; ++i) {
                panel_nav_push_if_valid(items, &count, max_count, fw_nav_out_label(i));
            }
            for (int i = 0; i < 8; ++i) {
                panel_nav_push_if_valid(items, &count, max_count, fw_nav_in_label(i));
            }
            return count;
        }
        panel_nav_append_sidebar(screen, items, &count, max_count);
        panel_nav_push_if_valid(items, &count, max_count, fw_nav_firmware_quick_update_btn());
        panel_nav_push_if_valid(items, &count, max_count, fw_nav_firmware_back_btn());
        panel_nav_push_if_valid(items, &count, max_count, fw_nav_update_btn());
        panel_nav_push_if_valid(items, &count, max_count, fw_nav_warranty_btn());
        panel_nav_push_if_valid(items, &count, max_count, fw_nav_wifi_back_btn());
        return count;
    }
    if (screen == SCREEN_ID_SCREEN_WIFI) {
        // آیتم: overlayِ بروزرسانیِ درجا رویِ همین صفحه رسم می‌شود — اگر باز است فقط دکمه‌های خودش
        // (تأیید/انصراف؛ اگر مخفی‌اند، push_if_valid با چکِ زنجیرهٔ parentِ مخفی خودش فیلترشان می‌کند)
        if (su_overlay_is_open()) {
            panel_nav_push_if_valid(items, &count, max_count, su_nav_confirm());
            panel_nav_push_if_valid(items, &count, max_count, su_nav_decline());
            return count;
        }
        // ترتیبِ مشخص: انتخابِ شبکه → اتصال → جستجوی مجدد → برگشت
        panel_nav_push_if_valid(items, &count, max_count, objects.drop_down_wifi_list);
        panel_nav_push_if_valid(items, &count, max_count, objects.btn_wifi_connect);
        panel_nav_push_if_valid(items, &count, max_count, objects.btn_wifi_redresh);
        panel_nav_push_if_valid(items, &count, max_count, fw_nav_wifi_back_btn());
        return count;
    }
    panel_nav_collect(lv_scr_act(), items, &count, max_count);
    panel_nav_sort_top_left(items, count);
    return count;
}

static const char *panel_nav_saved_text()
{
    int lang = ui_localization::get_ui_language_index();
    return lang == 2 ? "Kaydedildi" : (lang == 1 ? "Saved" : "ذخیره شد");
}

static const char *panel_nav_cancel_text()
{
    int lang = ui_localization::get_ui_language_index();
    return lang == 2 ? "Iptal edildi" : (lang == 1 ? "Canceled" : "کنسل شد");
}

static lv_obj_t *panel_nav_toast_label = NULL;
static lv_timer_t *panel_nav_toast_timer = NULL;
static void panel_nav_toast_hide_cb(lv_timer_t *timer)
{
    (void)timer;
    if (panel_nav_toast_label && lv_obj_is_valid(panel_nav_toast_label)) {
        lv_obj_add_flag(panel_nav_toast_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (panel_nav_toast_timer) {
        lv_timer_del(panel_nav_toast_timer);
        panel_nav_toast_timer = NULL;
    }
}

static void panel_nav_toast(const char *txt, uint32_t color)
{
    lv_obj_t *t = panel_nav_toast_label;
    if (!t || !lv_obj_is_valid(t)) {
        t = lv_label_create(lv_layer_top());
        panel_nav_toast_label = t;
        lv_obj_set_style_bg_opa(t, 235, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(t, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(t, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(t, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(t, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(t, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(t, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(t, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(t, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (panel_nav_toast_timer) {
        lv_timer_del(panel_nav_toast_timer);
        panel_nav_toast_timer = NULL;
    }
    lv_obj_set_style_bg_color(t, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (strcmp(lv_label_get_text(t), txt) != 0) {
        lv_label_set_text(t, txt);
    }
    lv_obj_clear_flag(t, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_move_foreground(t);
    panel_nav_toast_timer = lv_timer_create(panel_nav_toast_hide_cb, 1900, NULL);
    lv_timer_set_repeat_count(panel_nav_toast_timer, 1);
}

static bool panel_nav_is_settings_screen(int screen)
{
    return screen == SCREEN_ID_SCREEN_SETTING_GENERAL ||
           screen == SCREEN_ID_SCREEN_SETTING_JACK_SETTING ||
           screen == SCREEN_ID_SCREEN_SETTING_ADMIN ||
           screen == SCREEN_ID_SCREEN_ADMIN_PASSWORD ||
           screen == SCREEN_ID_SCREEN_PASSWORD ||
           screen == SCREEN_ID_SCREEN_WIFI ||
           screen == SCREEN_ID_SCREEN_UPDATE ||
           screen == SCREEN_ID_SCREEN_FRAMWARE;
}

static void panel_nav_open_section(int key)
{
    int next = SCREEN_ID_SCREEN_MENU;
    if (key == 1) next = SCREEN_ID_SCREEN_SETTING_GENERAL;
    else if (key == 2) next = SCREEN_ID_SCREEN_SETTING_JACK_SETTING;
    else if (key == 3) next = SCREEN_ID_SCREEN_ADMIN_PASSWORD;
    else if (key == 4) next = SCREEN_ID_SCREEN_UPDATE;
    settings_reset_open_panels();
    panel_nav_mark(NULL);
    if (panel_nav_active) panel_nav_screen = next;
    eez_flow_set_screen(next, LV_SCR_LOAD_ANIM_NONE, 0, 0);
}

static bool panel_nav_save_current(int screen)
{
    if (screen == SCREEN_ID_SCREEN_SETTING_GENERAL && objects.obj37) {
        lv_event_send(objects.obj37, LV_EVENT_CLICKED, NULL);
        return true;
    }
    if (screen == SCREEN_ID_SCREEN_SETTING_JACK_SETTING) {
        if (objects.obj66 && !lv_obj_has_flag(objects.obj66, LV_OBJ_FLAG_HIDDEN)) {
            lv_event_send(objects.obj66, LV_EVENT_CLICKED, NULL);
        } else {
            action_jack_value_set(NULL);
            panel_nav_toast(panel_nav_saved_text(), 0xff2e7d32);
        }
        return true;
    }
    if (screen == SCREEN_ID_SCREEN_SETTING_ADMIN && objects.obj77) {
        lv_event_send(objects.obj77, LV_EVENT_CLICKED, NULL);
        return true;
    }
    return false;
}

static void panel_nav_adjust_focused(int dir)
{
    lv_obj_t *obj = (panel_nav_focus && lv_obj_is_valid(panel_nav_focus)) ? panel_nav_focus : NULL;
    if (!obj) return;

    // آیتم: عنوانِ ستونِ «خروجی»/«ورودی» — START یا STOP = دیالوگِ ریستِ پایه‌های آن دسته
    // (تاچ نیاز به ۲ ثانیه نگه‌داشتن دارد؛ در حالتِ MOD فوری باز می‌شود، مثلِ بقیهٔ نقش‌های نگه‌داشتنی)
    if (obj == fw_nav_outputs_hdr()) { io_reset_dialog_open(1); return; }
    if (obj == fw_nav_inputs_hdr())  { io_reset_dialog_open(0); return; }

    // آیتم: نامِ هر ورودی/خروجی — START مثلِ تاچِ کوتاه (تستِ روشن/خاموش برایِ خروجی)، STOP مثلِ نگه‌داشتنِ
    // ۲ ثانیه (دیالوگِ جابجاییِ پایه) — بدونِ نیاز به تاچ/زمان‌سنجی، چون MOD خودش انتخاب را مشخص کرده.
    for (int i = 0; i < 8; i++) {
        if (obj == fw_nav_out_label(i)) {
            if (dir > 0) lv_event_send(obj, LV_EVENT_CLICKED, NULL);
            else io_remap_dialog_open(1, i);
            return;
        }
        if (obj == fw_nav_in_label(i)) {
            if (dir > 0) lv_event_send(obj, LV_EVENT_CLICKED, NULL); // ورودی تستِ دستی ندارد — بی‌ضرر
            else io_remap_dialog_open(0, i);
            return;
        }
    }

    if (obj == objects.temputre_arcs) {
        int32_t v = get_var_element_temperature_target() + dir;
        if (v < 0) v = 0;
        if (v > 350) v = 350;
        set_var_element_temperature_target(v);
        action_element_temp_target_changed(NULL);
        return;
    }

    if (lv_obj_check_type(obj, &lv_slider_class)) {
        int32_t v = lv_slider_get_value(obj) + dir;
        if (v < lv_slider_get_min_value(obj)) v = lv_slider_get_min_value(obj);
        if (v > lv_slider_get_max_value(obj)) v = lv_slider_get_max_value(obj);
        lv_slider_set_value(obj, v, LV_ANIM_ON);
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }
    if (lv_obj_check_type(obj, &lv_arc_class)) {
        int16_t v = lv_arc_get_value(obj) + dir;
        if (v < lv_arc_get_min_value(obj)) v = lv_arc_get_min_value(obj);
        if (v > lv_arc_get_max_value(obj)) v = lv_arc_get_max_value(obj);
        lv_arc_set_value(obj, v);
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        uint16_t count = lv_dropdown_get_option_cnt(obj);
        uint16_t sel = lv_dropdown_get_selected(obj);
        if (count > 0) {
            if (dir > 0 && sel + 1 < count) sel++;
            else if (dir < 0 && sel > 0) sel--;
            lv_dropdown_set_selected(obj, sel);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }
    if (lv_obj_check_type(obj, &lv_keyboard_class)) {
        // موارد ۳/۴: روی کیبردِ رمز — START کلیدِ انتخاب‌شده را می‌زند، STOP پاک می‌کند
        if (dir > 0) {
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);   // START: زدنِ کلیدِ هایلایت‌شده
        } else {
            lv_obj_t *ta = lv_keyboard_get_textarea(obj);
            if (ta) lv_textarea_del_char(ta);                   // STOP: backspace
        }
        return;
    }
    if (lv_obj_check_type(obj, &lv_btnmatrix_class)) {
        // کیبردِ عددیِ وسطِ گیج (btnmatrix خام): START یا STOP کلیدِ انتخاب‌شده را می‌زند
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        if (dir < 0) {
            lv_textarea_del_char(obj);
        } else {
            lv_event_send(obj, LV_EVENT_CLICKED, NULL);
        }
        return;
    }
    if (lv_obj_check_type(obj, &lv_switch_class)) {
        // مورد ۱: سوییچ با CLICKED تاگل نمی‌شود؛ وضعیت را دستی برعکس و VALUE_CHANGED بفرست
        if (lv_obj_has_state(obj, LV_STATE_CHECKED)) lv_obj_clear_state(obj, LV_STATE_CHECKED);
        else lv_obj_add_state(obj, LV_STATE_CHECKED);
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE)) {
        lv_event_send(obj, LV_EVENT_CLICKED, NULL);
        return;
    }

    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        // فقط START دکمه را می‌زند — STOP روی Back وای‌فای قبلاً هم برگشت می‌داد هم بعداً امرجنسی کاذب
        if (dir > 0)
            lv_event_send(obj, LV_EVENT_CLICKED, NULL);
        return;
    }
}

// مورد ۵: آیا فوکوسِ فعلی مقدارِ قابلِ‌تکرار است (گیج/اسلایدر/آرک) تا با نگه‌داشتن تکرار شود؟
extern "C" bool panel_nav_focus_is_repeatable(void)
{
    lv_obj_t *o = panel_nav_focus;
    if (!o || !lv_obj_is_valid(o)) return false;
    return lv_obj_check_type(o, &lv_slider_class) || lv_obj_check_type(o, &lv_arc_class) ||
           lv_obj_check_type(o, &lv_dropdown_class) ||
           o == objects.temputre_arcs || o == objects.obj23 || o == objects.obj26;
}

extern "C" void main_menu_test_mode_stop(void); // در screens.c — خروج از حالتِ تست

static void panel_nav_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (g_agent_nav_request != 0) {
        int req = g_agent_nav_request;
        g_agent_nav_request = 0;
        if (req == 111) {
            eez_flow_set_screen(SCREEN_ID_SCREEN_WIFI, LV_SCR_LOAD_ANIM_NONE, 0, 0);
            set_var_selected_wifi_network("Haffler-WiFi");
            wifi_password_panel_show();
        } else if (req == 11) {
            eez_flow_set_screen(SCREEN_ID_SCREEN_WIFI, LV_SCR_LOAD_ANIM_NONE, 0, 0);
            wifi_password_panel_hide();
        } else if (req > 0 && req <= 11) {
            eez_flow_set_screen(req, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        }
    }
    // مورد ۴: خروج از حالت MOD با تاچ یا بعد از ۱۰ ثانیه بی‌کاری
    if (panel_nav_active) {
        if (panel_nav_touch_exit) { panel_nav_touch_exit = false; panel_nav_exit_mode(); }
        else if ((uint32_t)(millis() - panel_nav_last_activity) >= 10000U) { panel_nav_exit_mode(); }
    }
    uint16_t evt = panel_nav_event;
    int row = (evt >> 8) & 0xFF;
    int key = evt & 0xFF;
    if (row == 2 && key == 5) {
        main_menu_test_mode_stop();
        if (wifi_is_busy_searching_or_connecting()) {
            wifi_abort_current_operation();
        }
    }
    if (row && key) {
        // اگر اسکرین‌سیور روشن بود، این کلید فقط نقشِ «بیدارکردن» دارد — نباید همان لحظه اکشنِ
        // عادیِ خودش (رفتن به بخش/ذخیره/...) را هم روی صفحهٔ زیرین اجرا کند.
        bool wasScreensaverActive = g_screensaver_active;
        screensaver_note_activity();
        if (wasScreensaverActive) { panel_nav_event = 0; return; }
    }
    if (!row || !key) return;
    panel_nav_event = 0;
    // در حالتِ قفل: کلید فقط کیبردِ قفل را بچرخاند (روی همین تاسکِ LVGL امن است).
    if (g_device_locked) {
        if (key == 3)      lock_nav_move();
        else if (key == 4) lock_nav_select();
        else if (key == 5) lock_nav_delete();
        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
        return;
    }
    panel_nav_last_activity = millis();   // مورد ۴: ثبتِ فعالیت
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);

    int screen = eez_flow_get_current_screen();
    // حین اجرای سیکل جوش: روی صفحهٔ اصلی، MOD و کنسل بی‌اثر باشند (فقط STOP/اضطراری کار کند)
    if (screen == SCREEN_ID_SCREEN_MENU && get_var_step_process() && row == 2 && (key == 2 || key == 3)) {
        if (panel_nav_active) panel_nav_exit_mode();
        return;
    }
    if (row == 1 && key >= 1 && key <= 5 && panel_nav_is_settings_screen(screen)) {
        panel_nav_open_section(key);
        return;
    }
    if (row == 2 && key == 1 && panel_nav_is_settings_screen(screen)) {
        if (!panel_nav_save_current(screen)) panel_nav_toast(panel_nav_saved_text(), 0xff2e7d32);
        return;
    }
    if (row == 2 && key == 2) {
        if (panel_nav_is_settings_screen(screen)) {
            panel_nav_toast(panel_nav_cancel_text(), 0xff777777);
            settings_reset_open_panels();
            eez_flow_set_screen(SCREEN_ID_SCREEN_MENU, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        } else if (panel_nav_active) {
            panel_nav_toast(panel_nav_cancel_text(), 0xff777777);
        }
        panel_nav_exit_mode();
        return;
    }
    if (row == 2 && key == 3) {
        // موارد ۳/۴: اگر فوکوس روی کیبردِ رمز است، MOD بین کلیدهای کیبرد می‌چرخد (نه بین فیلدها)
        if (panel_nav_active && panel_nav_focus && lv_obj_is_valid(panel_nav_focus) &&
            (lv_obj_check_type(panel_nav_focus, &lv_keyboard_class) ||
             lv_obj_check_type(panel_nav_focus, &lv_btnmatrix_class)) &&   // کیبردِ عددیِ گیج هم btnmatrix است
            lv_obj_is_visible(panel_nav_focus)) {  // مورد B: اگر کیبرد مخفی شد (خروج از تغییر رمز)، مد به آیتم‌های دیگر برود
            // کیبرد باید state فوکوس داشته باشد وگرنه کلیدِ انتخاب‌شده هایلایت نمی‌شود
            // (کیبردِ تغییرِ رمزِ تنظیماتِ مدیر در هیچ گروهی نیست، برخلاف صفحهٔ رمزِ مستقل)
            lv_obj_add_state(panel_nav_focus, LV_STATE_FOCUSED);
            if (lv_btnmatrix_get_selected_btn(panel_nav_focus) == LV_BTNMATRIX_BTN_NONE) {
                lv_btnmatrix_set_selected_btn(panel_nav_focus, 0);   // اولین MOD: کلیدِ اول انتخاب شود
            } else {
                uint32_t k = LV_KEY_RIGHT;   // MODهای بعدی: کلیدِ بعدی
                lv_event_send(panel_nav_focus, LV_EVENT_KEY, &k);
            }
            return;
        }
        lv_obj_t *items[96];
        int count = panel_nav_collect_for_screen(screen, items, 96);
        if (count == 0) return;
        int index = -1;
        for (int i = 0; i < count; ++i) if (items[i] == panel_nav_focus) index = i;
        bool wrapped = panel_nav_active && index >= count - 1;
        if (wrapped) {
            panel_nav_toast(panel_nav_cancel_text(), 0xff777777);
            panel_nav_exit_mode();
            return;
        }
        index = panel_nav_active ? ((index + 1) % count) : 0;
        panel_nav_active = true;
        panel_nav_screen = screen;
        panel_nav_mark(items[index]);
        return;
    }

    lv_obj_t *items[96];
    int count = panel_nav_collect_for_screen(screen, items, 96);
    if (count == 0) return;
    int index = 0;
    for (int i = 0; i < count; ++i) if (items[i] == panel_nav_focus) index = i;

    // آیکنِ تنظیمات روی صفحهٔ اصلی: هم START هم STOP باید تمیز واردِ تنظیماتِ عمومی شوند (از طریقِ
    // open_section که حالتِ ناوبری را درست به صفحهٔ جدید می‌برد) — نه CLICKEDِ خام که مربعِ سلکت را
        if (panel_nav_active && row == 2 && (key == 4 || key == 5) && screen == SCREEN_ID_SCREEN_MENU && panel_nav_focus == objects.setting_imageicon) {
        panel_nav_open_section(1);
        return;
    }
    lv_obj_t *focused_item = items[index];
    if (panel_nav_active && row == 2 && key == 4) panel_nav_adjust_focused(1);
    else if (panel_nav_active && row == 2 && key == 5) panel_nav_adjust_focused(-1);
    else if (!panel_nav_active && row == 2 && (key == 4 || key == 5)) {
        panel_nav_active = true;
        panel_nav_screen = screen;
        panel_nav_mark(items[index]);
        return;
    }
    else if (panel_nav_active && row == 2 && (key == 4 || key == 5)) {
        // فقط START/STOP آیتمِ فوکوس‌شده را «کلیک» می‌کنند. قبلاً این شرط کلیدِ خاصی نمی‌خواست، پس
        // روی صفحه‌های غیرِتنظیمات (که شاخهٔ SAVEِ بالا با panel_nav_is_settings_screen رد می‌شود)
        // کلیدِ SAVE هم به همین‌جا می‌رسید و آیتمِ فوکوس‌شده (مثلاً آیکونِ تنظیماتِ صفحهٔ اصلی) را
        // اشتباهاً «تایید» می‌کرد.
        lv_event_send(focused_item, LV_EVENT_CLICKED, NULL);
    }
    // ایمنی: اگر همین کلیک/START/STOP، خودِ آیتمِ فوکوس‌شده را حذف کرد — مثلِ دکمه‌های تأیید/انصراف/×
    // دیالوگ‌های جابجاییِ پایه، ریستِ پایه‌ها، ریستِ چهارچوب (که با lv_obj_del خودشان را می‌بندند) —
    // دیگر نباید به آن اشاره کرد؛ مقایسه‌های زیر و panel_nav_mark(items[index]) روی پوینترِ آزادشده
    // اجرا می‌شدند (هنگ/کرش). panel_nav_exit_mode()→panel_nav_mark(NULL) پیشاپیش اعتبارِ
    // panel_nav_focus را چک می‌کند، پس همین‌جا امن است.
    if (panel_nav_active && !lv_obj_is_valid(focused_item)) {
        panel_nav_exit_mode();
        return;
    }
    // آیتم: اگر همین START/STOP یک دیالوگِ جدید (جابجاییِ پایه یا ریستِ پایه‌ها) باز کرد، نشانگر
    // نباید رویِ نامِ ورودی/خروجیِ قبلی (که حالا پشتِ دیالوگ است) بماند — چون هر دو دیالوگ همیشه با
    // هر انتخابی خودشان را می‌بندند (io_remap_apply/io_reset_do_confirm/close)، اگر این‌جا باز باشند
    // یعنی «همین لحظه» با همین کلید باز شده‌اند، نه اینکه از قبل مانده باشند.
    if (panel_nav_active && (io_remap_is_open() || io_reset_is_open())) {
        panel_nav_exit_mode();
        return;
    }
    // اگر آیتمِ تاییدشده دکمهٔ بستنِ راهنما بود (با هر کلیدی: START/STOP/CLICK)، حالتِ ناوبری را تمیز ببند
    // و دیگر دوباره مارک نکن — وگرنه مربعِ سلکت روی صفحهٔ پشتی (تنظیمات) جا می‌ماند.
    if (focused_item == sg_nav_help_close_btn() || focused_item == jack_nav_help_close() || focused_item == sa_nav_help_close() || focused_item == mt_nav_help_close()) {
        settings_reset_open_panels();
        panel_nav_exit_mode();
        return;
    }
    // آیتم‌هایی که با تایید یک صفحه/دیالوگ/پاپ‌آپِ جدید باز می‌کنند:
    // نشانگرِ سلکت نباید روی صفحهٔ جدید جا بماند → حالت ناوبری بسته شود (با MOD دوباره وارد می‌شود).
    if (panel_nav_active && row == 2 && (key == 4 || key == 5) &&
        (focused_item == fw_nav_update_btn() || focused_item == fw_nav_frames_title() ||
         focused_item == objects.date_time ||
         focused_item == jack_nav_help_btn())) {   // راهنمای جک: بعد از باز شدن، نشانگر پاک شود
        panel_nav_exit_mode();
        return;
    }
    // ایمنی: اگر فوکوس واقعاً عوض نشده (مثلاً START رویِ کیبردِ رمز یک رقم را تایپ کرد ولی هنوز
    // همان کیبرد فوکوس است)، دوباره panel_nav_mark صدا نزن. panel_nav_mark همیشه اول
    // selected_btn/FOCUSED را رویِ فوکوسِ *قبلی* پاک می‌کند تا برای فوکوسِ بعدی جا باز کند؛ چون اینجا
    // فوکوسِ قبلی و بعدی یکی است (همان کیبرد)، این پاک‌سازی نشانگرِ کلیدِ انتخابی را از دستِ کاربر
    // می‌گرفت — دقیقاً همان چیزی که کاربر گفت: تایپِ یک رقم نباید نشانگر را جابه‌جا/پاک کند.
    if (items[index] != panel_nav_focus) {
        panel_nav_mark(items[index]);
    }
}

// ترجمهٔ رشته‌های UI — ML از tasks.h (eez_flow_get_language_index)
static int32_t get_ui_language_index() {
    return ui_localization::get_ui_language_index();
}

static const char *get_wrong_password_text()
{
    return ui_localization::get_wrong_password_text();
#if 0
    switch (get_ui_language_index())
    {
    case 1:
        return "Wrong password";
    case 2:
        return "Hatali sifre";
    default:
        return "رمز ورود اشتباه است";
    }
#endif
}


SemaphoreHandle_t xMutex_Wifi;

bool isSearching = false;
volatile bool g_wifi_direct_scan_active = false;
String wifi_names = "";
String selectedWifi = "";
String wifiPassword = "";
String wifis[WIFI_SCAN_META_MAX];
bool connectingToWifi = false;
static volatile bool g_wifi_connect_in_progress = false;

static String wifi_ssid_at_index(int idx);
static int wifi_resolve_connect_index(void);

volatile bool g_wifi_abort_request = false;

extern "C" bool wifi_is_busy_searching_or_connecting(void) {
    return isSearching || g_wifi_direct_scan_active || connectingToWifi || g_wifi_connect_in_progress;
}

extern "C" void wifi_abort_current_operation(void) {
    if (!wifi_is_busy_searching_or_connecting()) return;
    Serial.println("[WIFI-ABORT] STOP key pressed: aborting scan/connect immediately");
    g_wifi_abort_request = true;
    isSearching = false;
    g_wifi_direct_scan_active = false;
    connectingToWifi = false;
    g_wifi_connect_in_progress = false;
    set_var_waitig_panle_state(false);
    esp_wifi_scan_stop();
    esp_wifi_disconnect();
    char cancel_msg[64];
    int lang = eez_flow_get_language_index();
    if (lang == 1) snprintf(cancel_msg, sizeof(cancel_msg), "WiFi operation canceled");
    else if (lang == 2) snprintf(cancel_msg, sizeof(cancel_msg), "WiFi islemi iptal edildi");
    else snprintf(cancel_msg, sizeof(cancel_msg), "عملیات وای‌فای لغو شد");
    set_var_connection_error_label(cancel_msg);
    ui_request_toast_colored(cancel_msg, 0xff777777, 2000);
    wifi_ui_request_refresh();
}

#define WIFI_SCAN_META_MAX 24
static char g_wifi_scan_ssid[WIFI_SCAN_META_MAX][33];
static char g_wifi_scan_raw_ssid[WIFI_SCAN_META_MAX][33];
static uint8_t g_wifi_scan_bssid[WIFI_SCAN_META_MAX][6];
static int8_t g_wifi_scan_channel[WIFI_SCAN_META_MAX];
static uint8_t g_wifi_scan_authmode[WIFI_SCAN_META_MAX];
static int g_wifi_scan_meta_count = 0;

static void wifi_scan_meta_reset(void)
{
    g_wifi_scan_meta_count = 0;
    memset(g_wifi_scan_ssid, 0, sizeof(g_wifi_scan_ssid));
    memset(g_wifi_scan_raw_ssid, 0, sizeof(g_wifi_scan_raw_ssid));
    memset(g_wifi_scan_channel, 0, sizeof(g_wifi_scan_channel));
    memset(g_wifi_scan_bssid, 0, sizeof(g_wifi_scan_bssid));
    memset(g_wifi_scan_authmode, 0, sizeof(g_wifi_scan_authmode));
}

static void wifi_scan_meta_set_slot(int slot, const char *ssid, const char *raw_ssid, const uint8_t *bssid, int channel, int authmode)
{
    if (slot < 0 || slot >= WIFI_SCAN_META_MAX) return;
    if (ssid && ssid[0]) {
        strncpy(g_wifi_scan_ssid[slot], ssid, sizeof(g_wifi_scan_ssid[slot]) - 1);
        g_wifi_scan_ssid[slot][sizeof(g_wifi_scan_ssid[slot]) - 1] = '\0';
    } else {
        memset(g_wifi_scan_ssid[slot], 0, sizeof(g_wifi_scan_ssid[slot]));
    }
    if (raw_ssid && raw_ssid[0]) {
        strncpy(g_wifi_scan_raw_ssid[slot], raw_ssid, sizeof(g_wifi_scan_raw_ssid[slot]) - 1);
        g_wifi_scan_raw_ssid[slot][sizeof(g_wifi_scan_raw_ssid[slot]) - 1] = '\0';
    } else if (ssid && ssid[0]) {
        strncpy(g_wifi_scan_raw_ssid[slot], ssid, sizeof(g_wifi_scan_raw_ssid[slot]) - 1);
        g_wifi_scan_raw_ssid[slot][sizeof(g_wifi_scan_raw_ssid[slot]) - 1] = '\0';
    } else {
        memset(g_wifi_scan_raw_ssid[slot], 0, sizeof(g_wifi_scan_raw_ssid[slot]));
    }
    if (bssid) memcpy(g_wifi_scan_bssid[slot], bssid, 6);
    else memset(g_wifi_scan_bssid[slot], 0, 6);
    g_wifi_scan_channel[slot] = (int8_t)((channel >= 1 && channel <= 14) ? channel : 0);
    g_wifi_scan_authmode[slot] = (uint8_t)authmode;
}

static const char *wifi_resolve_target_ssid(const char *ssid, int scan_index)
{
    if (scan_index >= 0 && scan_index < g_wifi_scan_meta_count && g_wifi_scan_raw_ssid[scan_index][0] != '\0') {
        if (!ssid || !ssid[0] ||
            strcmp(g_wifi_scan_ssid[scan_index], ssid) == 0 ||
            strcmp(g_wifi_scan_raw_ssid[scan_index], ssid) == 0) {
            return g_wifi_scan_raw_ssid[scan_index];
        }
    }
    if (ssid && ssid[0]) {
        for (int i = 0; i < g_wifi_scan_meta_count; i++) {
            if ((strcmp(g_wifi_scan_ssid[i], ssid) == 0 || strcmp(g_wifi_scan_raw_ssid[i], ssid) == 0) &&
                g_wifi_scan_raw_ssid[i][0] != '\0') {
                return g_wifi_scan_raw_ssid[i];
            }
        }
        return ssid;
    }
    return "";
}

static String wifi_ssid_from_record_bytes(const uint8_t *ssid_raw, size_t max_len)
{
    size_t len = strnlen((const char *)ssid_raw, max_len);
    if (len > 32) len = 32;
    char tmp[33];
    memcpy(tmp, ssid_raw, len);
    tmp[len] = '\0';
    String out;
    out.reserve(len);
    for (size_t i = 0; i < len; ) {
        unsigned char c = (unsigned char)tmp[i];
        if (c < 0x20 || c == 0x7F || c == '#' || c == '\n' || c == '\r') {
            i++;
            continue;
        }
        if (c >= 0x80) {
            // نگه داشتن کاراکترهای ۲ بایتی فارسی/عربی
            if ((c & 0xE0) == 0xC0 && i + 1 < len) {
                unsigned char c2 = (unsigned char)tmp[i + 1];
                if (c >= 0xD8 && c <= 0xDB) {
                    out += (char)c;
                    out += (char)c2;
                    i += 2;
                    continue;
                }
            }
            // تبدیل یا نادیده گرفتن سایر کاراکترهای غیر ASCII
            i++;
            continue;
        }
        out += (char)c;
        i++;
    }
    while (out.indexOf("  ") >= 0) out.replace("  ", " ");
    out.trim();
    return out;
}

// آخرین دلیل قطع STA (لاگ تشخیصی)
static volatile int g_wifi_last_disconnect_reason = -1;
static volatile bool g_wifi_saw_got_ip = false;

extern "C" void wifi_sta_begin_network(const char *ssid, const char *pass, int scan_index)
{
    const char *target_ssid = wifi_resolve_target_ssid(ssid, scan_index);
    if (!target_ssid || !target_ssid[0]) return;

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true); // فعال‌سازی تلاش خودکار درایور برای بازیابی بسته‌های از دست‌رفته در محیط کارگاهی
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);

    // پوشش کامل کانال‌های ۱ تا ۱۳
    wifi_country_t country = { "IR", 1, 13, 20, WIFI_COUNTRY_POLICY_AUTO };
    esp_wifi_set_country(&country);

    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setMinSecurity(WIFI_AUTH_OPEN);
    g_wifi_last_disconnect_reason = -1;
    g_wifi_saw_got_ip = false;

    Serial.printf("[WIFI-CONN] begin target='%s' passLen=%d scan_idx=%d\n",
        target_ssid, pass ? (int)strlen(pass) : 0, scan_index);

    // راه‌اندازی استاندارد Netif و مقداردهی SSID/Password بدون شروع زودهنگام اتصال جهت جلوگیری از خطای ESP_ERR_WIFI_STATE
    WiFi.begin(target_ssid, (pass && pass[0]) ? pass : NULL, 0, NULL, false);

    // اسکن سراسری تمام کانال‌ها (بدون قفل‌کردن تک‌کانال) و تنظیم PMF تطبیقی سازگار با انواع مودم‌ها و هات‌اسپات‌ها
    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
        conf.sta.channel = 0; // اسکن تمام کانال‌ها برای یافتن قوی‌ترین سیگنال
        conf.sta.bssid_set = 0;
        conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        conf.sta.threshold.authmode = WIFI_AUTH_OPEN;
        conf.sta.pmf_cfg.capable = true;   // پشتیبانی از WPA2/WPA3 Mixed و هات‌اسپات‌های جدید
        conf.sta.pmf_cfg.required = false; // عدم رد کردن مودم‌های قدیمی فاقد PMF
        conf.sta.listen_interval = 3;
        esp_err_t set_ret = esp_wifi_set_config(WIFI_IF_STA, &conf);
        Serial.printf("[WIFI-CONN] set_config ret=%d\n", (int)set_ret);
    }
    esp_err_t err = esp_wifi_connect();
    Serial.printf("[WIFI-CONN] esp_wifi_connect started, ret=%d\n", (int)err);
}

extern "C" bool wifi_sta_wait_connected(int max_half_sec_ticks, volatile bool *cancel_flag)
{
    int max_ticks = max_half_sec_ticks * 2; // بررسی هر ۲۵۰ میلی‌ثانیه
    for (int c = 0; c < max_ticks; c++) {
        if (g_wifi_abort_request || (cancel_flag && *cancel_flag)) return false;
        if (WiFi.status() == WL_CONNECTED) return true;
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    return WiFi.status() == WL_CONNECTED;
}

extern "C" bool wifi_sta_connect_full(const char *ssid, const char *pass, int scan_index,
                                      int max_half_sec_ticks, volatile bool *cancel_flag)
{
    if (!ssid || !ssid[0]) return false;
    if (g_wifi_abort_request || (cancel_flag && *cancel_flag)) return false;

    const char *target_ssid = wifi_resolve_target_ssid(ssid, scan_index);

    // اگر قبلاً به همین شبکه وصل است، بدون تاخیر تایید کن
    if (WiFi.status() == WL_CONNECTED && (WiFi.SSID() == target_ssid || WiFi.SSID() == ssid)) {
        Serial.println("[WIFI-CONN] Already connected to requested SSID");
        return true;
    }

    WiFi.scanDelete();
    esp_wifi_scan_stop();
    g_wifi_direct_scan_active = false;
    isSearching = false;

    // پاک‌سازی تمیز وضعیت قبلی برای شروع اتصال تازه
    esp_wifi_disconnect();
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(50));
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(50));

    // مهلت زمانی اتصال: حداقل ۲۰ ثانیه برای اسکن، هندشیک امنیتی و تخصیص IP از DHCP
    int wait_ticks = (max_half_sec_ticks > 0) ? max_half_sec_ticks : 40;
    if (wait_ticks < 30) wait_ticks = 30;

    wifi_sta_begin_network(target_ssid, pass, scan_index);

    bool connected = wifi_sta_wait_connected(wait_ticks, cancel_flag);
    if (connected && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI-CONN] connected successfully IP=%s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    // در صورت انصراف کاربر
    if (g_wifi_abort_request || (cancel_flag && *cancel_flag)) {
        WiFi.setAutoReconnect(false);
        esp_wifi_disconnect();
        WiFi.disconnect(false, false);
        return false;
    }

    // فقط در صورت خطای قطعی احراز هویت (AUTH_FAIL) قطع کن، تایم‌اوت هندشیک ناشی از نویز کارگاه وارد ریکاوری می‌شود
    int r = g_wifi_last_disconnect_reason;
    if (r == 202) {
        Serial.printf("[WIFI-CONN] Auth/Password failure: reason=%d\n", r);
        WiFi.setAutoReconnect(false);
        esp_wifi_disconnect();
        WiFi.disconnect(false, false);
        return false;
    }

    // تلاش ریکاوری تکمیلی برای رفع تداخلات نویز موقت کارگاهی
    Serial.printf("[WIFI-CONN] Recovery attempt for '%s' (last_reason=%d)...\n", target_ssid, r);
    esp_wifi_disconnect();
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(50));

    wifi_sta_begin_network(target_ssid, pass, -1);
    connected = wifi_sta_wait_connected(16, cancel_flag); // ۸ ثانیه دیگر

    if (connected && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI-CONN] recovery connected successfully IP=%s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    Serial.printf("[WIFI-CONN] failed: status=%d last_reason=%d\n", (int)WiFi.status(), (int)g_wifi_last_disconnect_reason);
    WiFi.setAutoReconnect(false);
    esp_wifi_disconnect();
    WiFi.disconnect(false, false);
    return false;
}

extern "C" void wifi_password_panel_show(void)
{
    if (!objects.panle_wifi_password) return;
    lv_obj_clear_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
    if (objects.obj112) {
        lv_label_set_text(objects.obj112, selectedWifi.length() > 0 ? selectedWifi.c_str() : get_var_selected_wifi_network());
    }
    if (objects.text_area_wifi_password) {
        const char *pw = get_var_wifi_password();
        lv_textarea_set_text(objects.text_area_wifi_password, pw ? pw : "");
    }
    if (objects.keybord) {
        lv_obj_clear_flag(objects.keybord, LV_OBJ_FLAG_HIDDEN);
        // جایگاه کیبورد (y=72, ارتفاع 248 — کاملاً متقارن با پدینگ 4px)
        lv_obj_set_pos(objects.keybord, 0, 72);
        lv_obj_set_size(objects.keybord, 480, 248);
        lv_obj_set_style_pad_left(objects.keybord, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(objects.keybord, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_hor(objects.keybord, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (objects.text_area_wifi_password)
            lv_keyboard_set_textarea(objects.keybord, objects.text_area_wifi_password);
    }
    if (objects.text_area_wifi_password)
        lv_obj_add_state(objects.text_area_wifi_password, LV_STATE_FOCUSED);
    Serial.printf("[WIFI-UI] password panel shown for '%s'\n", selectedWifi.c_str());
}

extern "C" void wifi_password_panel_hide(void)
{
    if (objects.panle_wifi_password)
        lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
}

extern "C" bool wifi_selected_needs_password(void)
{
    int idx = wifi_resolve_connect_index();
    int auth = (idx >= 0 && idx < g_wifi_scan_meta_count) ? (int)g_wifi_scan_authmode[idx] : -1;
    if (auth < 0 && selectedWifi.length() > 0) {
        for (int i = 0; i < g_wifi_scan_meta_count; i++) {
            if (strcmp(g_wifi_scan_ssid[i], selectedWifi.c_str()) == 0 ||
                strcmp(g_wifi_scan_raw_ssid[i], selectedWifi.c_str()) == 0) {
                auth = (int)g_wifi_scan_authmode[i];
                break;
            }
        }
    }
    if (auth <= 0 || auth == WIFI_AUTH_OPEN) return false;
    // فقط متغیر فلو — نه textarea (گاهی از Worker صدا زده می‌شود؛ LVGL از هستهٔ ۰ ناامن است)
    const char *pw = get_var_wifi_password();
    if (pw && pw[0]) return false;
    return true;
}

extern "C" void wifi_on_dropdown_selection_changed(int32_t index)
{
    if (index < 0) index = 0;
    if (index >= WIFI_SCAN_META_MAX) index = WIFI_SCAN_META_MAX - 1;
    set_var_selected_wifi_index(index);

    selectedWifi = wifi_ssid_at_index((int)index);
    set_var_selected_wifi_network(selectedWifi.c_str());

    // مهم: getWifiPassword() یک String موقت برمی‌گرداند — .c_str() را نگه ندار
    String prefill;
    if (selectedWifi.length() > 0 && storage.hasWifi(selectedWifi)) {
        prefill = storage.getWifiPassword(selectedWifi);
    } else if (selectedWifi.length() > 0 && selectedWifi == storage.getWifiSsid()) {
        prefill = storage.getWifiPassword();
    }
    // شبکهٔ جدید (هات‌اسپات): فیلد رمز خالی بماند تا رمز وای‌فای خانه قاطی نشود
    set_var_wifi_password(prefill.c_str());
    // عمداً lv_textarea_set_text اینجا نیست — از ota/SelfUpd/WifiScan هم صدا زده می‌شود (هستهٔ ۰).
    // همگام‌سازی textarea فقط در نخ LVGL (wifi_ui_apply_now / VALUE_CHANGED).
    wifi_ui_request_refresh();
    Serial.printf("[WIFI-UI] selected idx=%d ssid='%s' prefillLen=%d\n",
        (int)index, selectedWifi.c_str(), (int)prefill.length());
}

// --- متغیر وضعیت بازیابی از کرش (مدیریت در ui_resume.cpp) ---
bool g_is_crash_recovery_boot = false;

// --- هندل تسک‌ها ---
TaskHandle_t lvglTaskHandle;
TaskHandle_t appTaskHandle;
TaskHandle_t otaTaskHandle = NULL;
TaskHandle_t keypadTaskHandle;
TaskHandle_t buzzerTaskHandle;
TaskHandle_t heaterTaskHandle;
TaskHandle_t webTaskHandle;
TaskHandle_t lockTaskHandle = NULL;

volatile bool webAppEnabled = false;

// درخواستِ هم‌گام‌سازیِ وب‌اپ از ترد UI به تسکِ پس‌زمینه (هستهٔ ۰).
// نباید عملیاتِ بلاک‌کنندهٔ WiFi (wStart/softAP/vTaskDelay) روی ترد LVGL اجرا شود وگرنه نمایشگر فریز می‌کند.
volatile bool g_webapp_sync_requested = false;
volatile bool g_webapp_sync_restart = false;

static void sync_webapp_state(bool restartWhenOn = false)
{
    // حین اسکنِ installer اصلاً وضعیتِ WiFi را دست نزن — وگرنه اتصالِ در حالِ استفاده قطع می‌شود
    // (reason=8) و مدل‌های بعدی «سرور در دسترس نیست» می‌گیرند. پاکسازی بعد از پایانِ اسکن انجام می‌شود.
    if (g_inst_scan_busy) return;
    // در صفحاتِ وای‌فای یا کارت‌های نصب/فریم‌ور یا صفحه بروزرسانی، هرگز وای‌فای را دست نزن
    int cur_scr = eez_flow_get_current_screen();
    if (cur_scr == SCREEN_ID_SCREEN_WIFI || cur_scr == SCREEN_ID_SCREEN_FRAMWARE ||
        cur_scr == SCREEN_ID_SCREEN_UPDATE) return; // FIX-WEBAPP-AP
    // بروزرسانیِ درجا: self_update_task مالکِ STA است — WiFi.mode(OFF) اینجا اتصال را می‌کشد.
    if (g_self_update_session) return;
    // F5-بوت: ota_task مالکِ STA است. قبلاً وقتی ota_enable بود و webApp خاموش،
    // همین تابع WiFi.mode(OFF) می‌زد و بعد از InstScan، نصب با «نسخه روی سرور پیدا نشد» می‌مرد.
    if (get_var_ota_enable()) return;
    // قفلِ رادیو: تا وقتی درخواستِ HTTPSِ installer/OTA در جریان است، قطع/تغییرِ WiFi را انجام نده
    // (وگرنه قطعِ وسطِ handshake → کرشِ null در esp_wifi_internal_free_rx_buffer).
    bool locked = (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(20000)) == pdTRUE);

    bool shouldRun = get_var_setting_general_wifi() && !get_var_ota_enable() && !g_self_update_session;
    Serial.printf("[WIFI] sync_webapp shouldRun=%d wifiVar=%d enabled=%d apUp=%d restart=%d\n",
                  (int)shouldRun, (int)get_var_setting_general_wifi(), (int)webAppEnabled,
                  (int)webApp.isApUp(), (int)restartWhenOn);
    if (!shouldRun)
    {
        if (webAppEnabled)
        {
            webApp.wStop();
        }
        else
        {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
        }
        webAppEnabled = false;
        if (locked) xSemaphoreGive(xMutex_Wifi);
        return;
    }

    // ری‌استارت فقط وقتی caller صریحاً بخواهد یا Beacon مرده باشد — نه روی هر Save.
    if (webAppEnabled && (restartWhenOn || !webApp.isApUp()))
    {
        webApp.wStop();
        vTaskDelay(pdMS_TO_TICKS(150));
        webAppEnabled = false;
    }

    if (!webAppEnabled)
    {
        bool ok = webApp.wStart();
        webAppEnabled = ok;
        if (!ok) {
            Serial.println("[WIFI] SoftAP did not come up — phone will not see the network");
        } else {
            // بعد از بالا آمدنِ تمیز AP، یک handshake برای کلاینت‌های وصل
            webApp.reqHandShake();
        }
    }
    if (locked) xSemaphoreGive(xMutex_Wifi);
}

void webapp_sync_vars_from_plc(void)
{
    int sel = storage.getSelectedUser();
    if (sel < 1) sel = 1;
    if (sel > 5) sel = 5;
    program_no = sel;

    for (int i = 1; i <= 5; i++) {
        s_up[i] = storage.getWildingTime(i);
        e_up[i] = storage.getCoolingTime(i);
        c_up[i] = storage.getTempTarget(i);
    }
    s_up[sel] = (int)get_var_welding_time();
    e_up[sel] = (int)get_var_cooling_time();
    c_up[sel] = (int)get_var_element_temperature_target();

    table_ready  = abs((int)get_var_setting_jack_safhe_raft());
    table_back   = abs((int)get_var_setting_jack_safhe_bargasht());
    ax_ready     = abs((int)get_var_setting_jack_shablon_raft());
    ax_back      = abs((int)get_var_setting_jack_shablon_bargasht());
    press_ready  = abs((int)get_var_setting_jack_profil_raft());
    press_back   = abs((int)get_var_setting_jack_profil_bargasht());
    heater_ready = abs((int)get_var_setting_jack_josh_raft());
    heater_back  = abs((int)get_var_setting_jack_josh_bargasht());

    int z1_ms = abs((int)get_var_jack_dot2_zarbe1_raft());
    int z2_ms = abs((int)get_var_jack_dot2_zarbe2_raft());
    tap_jack_1 = (z2_ms + 500) / 1000;
    tap_jack_2 = (z1_ms + 500) / 1000;

    lan_ = (int)get_var_lang();
    if (lan_ < 1) lan_ = 1;
    if (lan_ > 3) lan_ = 3;

    PID = get_var_setting_general_pid();
    buzzer_en = get_var_setting_general_speaker();
    auto_start = get_var_setting_general_automatic_start();
    finish_time = get_var_setting_general_final_jack();
    stat_stop = get_var_setting_admin_emergency_switch();
    val_ok = get_var_setting_general_temperature_limit();
    pedal_Count = (uint8_t)(get_var_setting_general_pedal_number3() ? 4 : 3);
    jackSensore_enabled = get_var_setting_admin_jack_sensor();
    temp_offset_int = get_var_setting_general_temperature_caliber();

    unit_welding = (float)get_var_total_frame_numbers();
    unit_welding_p = (float)get_var_frame_numbers();

    device_mode = MODE_WELDER_SINGLE;
    lock_en = storage.countdownActive() ? 1 : 0;
    lock_time = storage.getReverseTimer();

    int cur_temp = (int)get_var_element_temperature();
    if (cur_temp >= 0 && cur_temp < 600) {
        temp_dis_23 = cur_temp;
        temp = cur_temp;
    } else {
        temp_dis_23 = 0;
        temp = 0;
    }
}

void webapp_apply_vars_to_plc(void)
{
    int sel = program_no;
    if (sel < 1) sel = 1;
    if (sel > 5) sel = 5;
    storage.setSelectedUser(sel);

    for (int i = 1; i <= 5; i++) {
        if (s_up[i] > 0) storage.setWildingTime(s_up[i], i);
        if (e_up[i] > 0) storage.setCoolingTime(e_up[i], i);
        if (c_up[i] >= 0 && c_up[i] <= 350) storage.setTempTarget(c_up[i], i);
    }

    set_var_welding_time(s_up[sel]);
    set_var_cooling_time(e_up[sel]);
    set_var_element_temperature_target(c_up[sel]);

    set_var_setting_jack_safhe_raft((double)table_ready);
    set_var_setting_jack_safhe_bargasht((double)table_back);
    set_var_setting_jack_shablon_raft((double)ax_ready);
    set_var_setting_jack_shablon_bargasht((double)ax_back);
    set_var_setting_jack_profil_raft((double)press_ready);
    set_var_setting_jack_profil_bargasht((double)press_back);
    set_var_setting_jack_josh_raft((double)heater_ready);
    set_var_setting_jack_josh_bargasht((double)heater_back);
    set_var_jack_dot2_zarbe1_raft((double)(tap_jack_2 * 1000));
    set_var_jack_dot2_zarbe2_raft((double)(tap_jack_1 * 1000));

    storage.set_jack_safhe_raft(table_ready);
    storage.set_jack_safhe_bargasht(table_back);
    storage.set_jack_shablon_raft(ax_ready);
    storage.set_jack_shablon_bargasht(ax_back);
    storage.set_jack_prifil_raft(press_ready);
    storage.set_jack_prifil_bargasht(press_back);
    storage.set_jack_josh_raft(heater_ready);
    storage.set_jack_josh_bargasht(heater_back);
    storage.set_jack_zarbe_1_raft(tap_jack_2 * 1000);
    storage.set_jack_zarbe_2_raft(tap_jack_1 * 1000);

    if (lan_ >= 1 && lan_ <= 3) {
        int oldLang = (int)get_var_lang();
        set_var_lang(lan_);
        eez_flow_set_language_index(lan_ == 3 ? 2 : (lan_ == 2 ? 1 : 0));
        storage.setLang(lan_);
        if (oldLang != lan_) {
            apply_language_now(lan_);
        }
    }
    storage.setPID(PID);
    set_var_setting_general_pid(PID);
    storage.setSpeaker(buzzer_en);
    set_var_setting_general_speaker(buzzer_en);
    storage.setAutoStart(auto_start);
    set_var_setting_general_automatic_start(auto_start);
    storage.setFinalJack(finish_time);
    set_var_setting_general_final_jack(finish_time);
    storage.setEmergencySwich(stat_stop);
    set_var_setting_admin_emergency_switch(stat_stop);
    storage.setTempLimit(val_ok);
    set_var_setting_general_temperature_limit(val_ok);
    storage.setIsTreePedal(pedal_Count == 4);
    set_var_setting_general_pedal_number3(pedal_Count == 4);
    set_var_setting_general_pedal_number2(pedal_Count != 4);
    storage.setSensorCheck(jackSensore_enabled);
    set_var_setting_admin_jack_sensor(jackSensore_enabled);
    storage.setTempCaliber(temp_offset_int);
    set_var_setting_general_temperature_caliber(temp_offset_int);
}

void webapp_push_state_to_clients(void)
{
    if (!webAppEnabled) return;
    webapp_sync_vars_from_plc();
    webApp.reqHandShake();
}


// --- متغیرهای وضعیت دکمه‌ها ---
ButtonState fPanel, mPanel;

Storage storage;

// ===== تایمر معکوس مدیر: فعال/غیرفعال + پیش‌نویسِ روزها (بدونِ ذخیرهٔ ناخواسته) =====
// مقدارِ نمایش‌داده‌شده روی نمودار تا قبلِ زدنِ «ذخیره» در همین متغیرهاست؛ اگر کاربر دست نزند، Save چیزی را تغییر نمی‌دهد.
static int  g_countdown_draft_days = 0;
static bool g_countdown_touched = false;
static int  g_mt_draft_days[4] = {0, 0, 0, 0}; // T2..T5 — فقط پیش‌نویس تا زدن «ذخیره»
static bool g_mt_touched = false;
static bool g_admin_timer_saved = false;

extern "C" bool multi_timer_dialog_is_open(void);
extern "C" void multi_timer_dialog_close(void);

// روزِ نمایشیِ فعلی برای تایمر معکوس (روزِ باقی‌مانده اگر فعال است، وگرنه آخرین عددِ ذخیره‌شده).
static int admin_countdown_display_days(void) {
    // همیشه عددِ ذخیره‌شدهٔ T1 را نشان بده (نه باقی‌مانده / نه مقدار خراب‌شدهٔ UI)
    int d = storage.getReverseTimer();
    if (d < 0) d = 0;
    if (d > 125) d = 125;
    return d;
}

// هنگامِ ورود به تنظیماتِ مدیر: متغیرِ فلو (که هم اسلایدر و هم عددِ روی صفحه از آن می‌خوانند) را
// با روزِ باقی‌مانده هماهنگ کن. تنها منبعِ حقیقت همین متغیرِ فلوست.
extern "C" void admin_countdown_panel_open(void) {
    g_countdown_touched = false;
    g_mt_touched = false;
    g_admin_timer_saved = false;
    g_countdown_draft_days = admin_countdown_display_days();
    set_var_setting_admin_countdown_timer(g_countdown_draft_days);
    for (int i = 0; i < 4; i++)
        g_mt_draft_days[i] = storage.getMultiTimerDay(i + 2);
}

static void admin_countdown_apply_multi_timers(void)
{
    for (int t = 2; t <= 5; t++)
        storage.setMultiTimerDay(t, g_mt_draft_days[t - 2]);
    g_mt_touched = false;
    g_admin_timer_saved = true;
}

// خروج از تنظیمات مدیر بدون «ذخیره»: پیش‌نویس‌ها دور ریخته می‌شوند؛ مقدارِ ذخیره‌شده دست نمی‌خورد
extern "C" void admin_countdown_panel_discard(void)
{
    if (g_admin_timer_saved) return;
    if (multi_timer_dialog_is_open())
        multi_timer_dialog_close();
    // T2..T5: اگر در این جلسه تغییر کرده و ذخیره نشده → صفر (طبق درخواست قبلی)
    if (g_mt_touched) {
        for (int t = 2; t <= 5; t++)
            storage.setMultiTimerDay(t, 0);
        for (int i = 0; i < 4; i++)
            g_mt_draft_days[i] = 0;
        g_mt_touched = false;
    }
    // T1: به مقدارِ ذخیره‌شده برگرد (پاک نکن)
    if (g_countdown_touched) {
        g_countdown_draft_days = admin_countdown_display_days();
        set_var_setting_admin_countdown_timer(g_countdown_draft_days);
        g_countdown_touched = false;
    }
}

// ===== ساعت/تاریخ مدیر — پیش‌نویس زنده + ذخیره فقط وقتی کاربر واقعاً تغییر داده =====
static bool g_admin_rtc_time_user_set = false;
static bool g_admin_rtc_date_user_set = false;

extern "C" void admin_rtc_panel_open(void) {
    g_admin_rtc_time_user_set = false;
    g_admin_rtc_date_user_set = false;
    if (g_hw_rtc_missing) return;
    DateTime now = rtc_now_s();
    if (now.year() < 2020) return;
    set_var_admin_set_hour(now.hour());
    set_var_admin_set_minute(now.minute());
    int lang = (int)get_var_lang();
    if (lang == 2 || lang == 3) {
        set_var_admin_set_year(now.year());
        set_var_admin_set_month(now.month());
        set_var_admin_set_day(now.day());
    } else {
        int jy = 0, jm = 0, jd = 0;
        date_utils::gregorian_to_jalali(now.year(), now.month(), now.day(), jy, jm, jd);
        set_var_admin_set_year(jy);
        set_var_admin_set_month(jm);
        set_var_admin_set_day(jd);
    }
}

extern "C" void admin_rtc_mark_time_edited(void) { g_admin_rtc_time_user_set = true; }
extern "C" void admin_rtc_mark_date_edited(void) { g_admin_rtc_date_user_set = true; }
extern "C" bool admin_rtc_time_user_edited(void) { return g_admin_rtc_time_user_set; }
extern "C" bool admin_rtc_date_user_edited(void) { return g_admin_rtc_date_user_set; }

extern "C" void admin_rtc_refresh_draft(bool hour_editing, bool minute_editing,
                                        bool year_editing, bool month_editing, bool day_editing)
{
    if (g_hw_rtc_missing) return;
    DateTime now = rtc_now_s();
    if (now.year() < 2020) return;

    if (!g_admin_rtc_time_user_set && !hour_editing && !minute_editing) {
        if ((int)get_var_admin_set_hour() != now.hour())
            set_var_admin_set_hour(now.hour());
        if ((int)get_var_admin_set_minute() != now.minute())
            set_var_admin_set_minute(now.minute());
    }
    if (!g_admin_rtc_date_user_set && !year_editing && !month_editing && !day_editing) {
        int lang = (int)get_var_lang();
        if (lang == 2 || lang == 3) {
            if ((int)get_var_admin_set_year() != now.year())
                set_var_admin_set_year(now.year());
            if ((int)get_var_admin_set_month() != now.month())
                set_var_admin_set_month(now.month());
            if ((int)get_var_admin_set_day() != now.day())
                set_var_admin_set_day(now.day());
        } else {
            int jy = 0, jm = 0, jd = 0;
            date_utils::gregorian_to_jalali(now.year(), now.month(), now.day(), jy, jm, jd);
            if ((int)get_var_admin_set_year() != jy) set_var_admin_set_year(jy);
            if ((int)get_var_admin_set_month() != jm) set_var_admin_set_month(jm);
            if ((int)get_var_admin_set_day() != jd) set_var_admin_set_day(jd);
        }
    }
}

static void update_lock_state(void);

static void admin_rtc_apply_save(void)
{
    if (!g_admin_rtc_time_user_set && !g_admin_rtc_date_user_set) return;

    DateTime now = rtc_now_s();
    int hour = g_admin_rtc_time_user_set ? (int)get_var_admin_set_hour() : now.hour();
    int minute = g_admin_rtc_time_user_set ? (int)get_var_admin_set_minute() : now.minute();
    int jy = (int)get_var_admin_set_year();
    int jm = (int)get_var_admin_set_month();
    int jd = (int)get_var_admin_set_day();

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return;

    int gy = now.year(), gm = now.month(), gd = now.day();
    if (g_admin_rtc_date_user_set) {
        if (jy >= 2000) {
            gy = jy; gm = jm; gd = jd;
        } else if (date_utils::is_valid_jalali_date(jy, jm, jd)) {
            date_utils::jalali_to_gregorian(jy, jm, jd, gy, gm, gd);
        }
    }

    DateTime target(gy, gm, gd, hour, minute, now.second());
    rtc_adjust_s(target);
    long targetJdn = Storage::gregorianToJDN(gy, gm, gd);
    storage.resetLastSeenJDN(targetJdn);
    update_lock_state();

    char clock_text[8];
    snprintf(clock_text, sizeof(clock_text), "%02d:%02d", hour, minute);
    set_var_header_time_text(clock_text);
    int hy = 0, hm = 0, hd = 0;
    date_utils::gregorian_to_jalali(gy, gm, gd, hy, hm, hd);
    char date_text[16];
    snprintf(date_text, sizeof(date_text), "%04d/%02d/%02d", hy, hm, hd);
    set_var_header_date_text(date_text);

    g_admin_rtc_time_user_set = false;
    g_admin_rtc_date_user_set = false;
}

extern "C" bool admin_countdown_is_enabled(void) { return storage.isCountdownEnabled(); }
extern "C" int  admin_countdown_draft_days(void) { return g_countdown_draft_days; }

// (نگه‌داشته‌شده برای سازگاری با اسلایدرِ نامرئیِ قدیمی — دیگر منبعِ حقیقت نیست)
extern "C" void admin_countdown_step(int delta) {
    g_countdown_draft_days += delta;
    if (g_countdown_draft_days < 0) g_countdown_draft_days = 0;
    if (g_countdown_draft_days > 125) g_countdown_draft_days = 125;
    g_countdown_touched = true;
    set_var_setting_admin_countdown_timer(g_countdown_draft_days);
}
extern "C" void admin_countdown_set_days(int days) {
    if (days < 0) days = 0;
    if (days > 125) days = 125;
    g_countdown_draft_days = days;
    g_countdown_touched = true;
    set_var_setting_admin_countdown_timer(days);
}

// تاپ روی نوشتهٔ «تایمر معکوس»: فعال/غیرفعال (فوری، مستقل از دکمهٔ ذخیره)
extern "C" void admin_countdown_toggle(void) {
    if (storage.isCountdownEnabled()) {
        // غیرفعال‌کردن: بلافاصله پاک شود و عددِ روی صفحه صفر شود
        long today = today_jdn_from_rtc();
        if (today != 0) storage.startCountdown(0, today);
        storage.setReverseTimer(0);
        storage.setCountdownEnabled(false);
        g_countdown_draft_days = 0;
        g_countdown_touched = false;
        set_var_setting_admin_countdown_timer(0);
    } else {
        // فعال‌کردن: فقط قابلِ‌ویرایش می‌شود؛ مقدار را با اسلایدر بگذار و «ذخیره» بزن
        storage.setCountdownEnabled(true);
        g_countdown_touched = false;
    }
}

static bool pending_base_values_save = false;
static uint32_t pending_base_values_last_change_ms = 0;

static void schedule_base_values_save()
{
    pending_base_values_save = true;
    pending_base_values_last_change_ms = millis();
}

void process_pending_base_values_save()
{
    if (!pending_base_values_save)
    {
        return;
    }

    if ((uint32_t)(millis() - pending_base_values_last_change_ms) < 1000U)
    {
        return;
    }

    storage.setWildingTime(get_var_welding_time(), storage.getSelectedUser());
    storage.setCoolingTime(get_var_cooling_time(), storage.getSelectedUser());
    storage.setTempTarget(get_var_element_temperature_target(), storage.getSelectedUser());
    pending_base_values_save = false;
    webapp_push_state_to_clients();
}

BBCapTouch bbct;
TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

MAX6675 thermocouple(THERMO_SCK, THERMO_1_CS, THERMO_MISO);
MAX6675 thermocouple2(THERMO_SCK, THERMO_2_CS, THERMO_MISO);

static bool is_valid_jalali_date(int jy, int jm, int jd)
{
    return date_utils::is_valid_jalali_date(jy, jm, jd);
#if 0
    if (jy < 1200 || jm < 1 || jm > 12 || jd < 1)
    {
        return false;
    }

    int max_day = 31;
    if (jm >= 7 && jm <= 11)
    {
        max_day = 30;
    }
    else if (jm == 12)
    {
        max_day = 30;
    }

    return jd <= max_day;
#endif
}

static void jalali_to_gregorian(int jy, int jm, int jd, int &gy, int &gm, int &gd)
{
    date_utils::jalali_to_gregorian(jy, jm, jd, gy, gm, gd);
    return;
#if 0
    static const int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    static const int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    long jy2 = jy - 979;
    long jm2 = jm - 1;
    long jd2 = jd - 1;

    long j_day_no = 365 * jy2 + (jy2 / 33) * 8 + ((jy2 % 33 + 3) / 4);
    for (long i = 0; i < jm2; ++i)
    {
        j_day_no += j_days_in_month[i];
    }
    j_day_no += jd2;

    long g_day_no = j_day_no + 79;
    gy = (int)(1600 + 400 * (g_day_no / 146097));
    g_day_no %= 146097;

    bool leap = true;
    if (g_day_no >= 36525)
    {
        g_day_no--;
        gy += (int)(100 * (g_day_no / 36524));
        g_day_no %= 36524;
        if (g_day_no >= 365)
        {
            g_day_no++;
        }
        else
        {
            leap = false;
        }
    }

    gy += (int)(4 * (g_day_no / 1461));
    g_day_no %= 1461;

    if (g_day_no >= 366)
    {
        leap = false;
        g_day_no--;
        gy += (int)(g_day_no / 365);
        g_day_no %= 365;
    }

    int i = 0;
    while (g_day_no >= g_days_in_month[i] + ((i == 1 && leap) ? 1 : 0))
    {
        g_day_no -= g_days_in_month[i] + ((i == 1 && leap) ? 1 : 0);
        i++;
    }
    gm = i + 1;
    gd = (int)g_day_no + 1;
#endif
}

static void gregorian_to_jalali(int gy, int gm, int gd, int &jy, int &jm, int &jd)
{
    date_utils::gregorian_to_jalali(gy, gm, gd, jy, jm, jd);
    return;
#if 0
    static const int g_days_to_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int gy2 = (gm > 2) ? (gy + 1) : gy;
    long days = 355666L + (365L * gy) + ((gy2 + 3) / 4) - ((gy2 + 99) / 100) + ((gy2 + 399) / 400) + gd + g_days_to_month[gm - 1];

    jy = (int)(-1595 + 33 * (days / 12053));
    days %= 12053;
    jy += (int)(4 * (days / 1461));
    days %= 1461;

    if (days > 365)
    {
        jy += (int)((days - 1) / 365);
        days = (days - 1) % 365;
    }

    if (days < 186)
    {
        jm = (int)(1 + days / 31);
        jd = (int)(1 + (days % 31));
    }
    else
    {
        jm = (int)(7 + ((days - 186) / 30));
        jd = (int)(1 + ((days - 186) % 30));
    }
#endif
}


void splitString(String input, char delimiter, String output[], int maxParts)
{
    int startIndex = 0;
    int endIndex;
    int partIndex = 0;

    while ((endIndex = input.indexOf(delimiter, startIndex)) != -1 && partIndex < maxParts)
    {
        output[partIndex++] = input.substring(startIndex, endIndex);
        startIndex = endIndex + 1;
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

static bool wifi_list_is_scan_placeholder(const char *s)
{
    if (!s || !s[0]) return true;
    return strcmp(s, "SEARCHING") == 0 || strcmp(s, "Scanning...") == 0 ||
           strcmp(s, "PLEASE WAIT") == 0 || strncmp(s, "PLEASE WAIT", 11) == 0 ||
           strcmp(s, "SCAN FAILED") == 0 || strcmp(s, "NO WIFI FOUND") == 0;
}

static String wifi_ssid_at_index(int idx)
{
    const char *list = wifi_names.length() > 0 ? wifi_names.c_str() : get_var_available_wifi_networks();
    if (wifi_list_is_scan_placeholder(list)) list = "";
    if (!list || !list[0]) {
        const char *sel = get_var_selected_wifi_network();
        return (sel && sel[0]) ? String(sel) : String("");
    }
    if (idx < 0) idx = 0;
    int cur = 0;
    const char *p = list;
    while (*p) {
        const char *start = p;
        while (*p && *p != '#') p++;
        if (cur == idx) {
            String res = "";
            for (const char *s = start; s < p; s++) res += *s;
            return res;
        }
        cur++;
        if (*p == '#') p++;
    }
    const char *sel = get_var_selected_wifi_network();
    return (sel && sel[0]) ? String(sel) : String("");
}

static int wifi_resolve_connect_index(void)
{
    // فقط متغیرها — هرگز lv_dropdown_get_selected از تسک وای‌فای (هستهٔ ۰)
    int idx = get_var_selected_wifi_index();
    if (idx < 0) idx = 0;
    if (idx >= WIFI_SCAN_META_MAX) idx = WIFI_SCAN_META_MAX - 1;
    return idx;
}

void action_main_load(lv_event_t *e)
{
}

void action_cycel_start_btn(lv_event_t *e)
{
    (void)e;
    pedal_operation();
}
void action_setting_admin_save_backup_values(lv_event_t *e)
{
    // آیتم ۱۱: ذخیرهٔ تنظیمات فعلی به‌عنوان «پیش‌فرضِ مدیر» (مرجع ریست‌فکتوری).
    // ابتدا کلیه متغیرهای زنده را در شیء storage همگام می‌کنیم تا بکاپ کامل باشد:
    storage.setLang(get_var_lang());
    storage.setIs_J_type(get_var_setting_admin_thermocouple_j());
    storage.setEmergencySwich(get_var_setting_admin_emergency_switch());
    storage.setSensorCheck(get_var_setting_admin_jack_sensor());
    storage.setTempCaliber(get_var_setting_general_temperature_caliber());
    storage.setPID(get_var_setting_general_pid());
    storage.setSpeaker(get_var_setting_general_speaker());
    storage.setAutoStart(get_var_setting_general_automatic_start());
    storage.setFinalJack(get_var_setting_general_final_jack());
    storage.setTempLimit(get_var_setting_general_temperature_limit());
    storage.setIsTreePedal(get_var_setting_general_pedal_number3());
    storage.setWifiOn(get_var_setting_general_wifi());
    storage.setWildingTime(get_var_welding_time(), storage.getSelectedUser());
    storage.setCoolingTime(get_var_cooling_time(), storage.getSelectedUser());
    storage.setTempTarget(get_var_element_temperature_target(), storage.getSelectedUser());
    storage.set_jack_safhe_raft(abs((int)get_var_setting_jack_safhe_raft()));
    storage.set_jack_safhe_bargasht(abs((int)get_var_setting_jack_safhe_bargasht()));
    storage.set_jack_shablon_raft(abs((int)get_var_setting_jack_shablon_raft()));
    storage.set_jack_shablon_bargasht(abs((int)get_var_setting_jack_shablon_bargasht()));
    storage.set_jack_prifil_raft(abs((int)get_var_setting_jack_profil_raft()));
    storage.set_jack_prifil_bargasht(abs((int)get_var_setting_jack_profil_bargasht()));
    storage.set_jack_josh_raft(abs((int)get_var_setting_jack_josh_raft()));
    storage.set_jack_josh_bargasht(abs((int)get_var_setting_jack_josh_bargasht()));
    storage.set_jack_zarbe_1_raft(abs((int)get_var_jack_dot2_zarbe1_raft()));
    storage.set_jack_zarbe_2_raft(abs((int)get_var_jack_dot2_zarbe2_raft()));
    storage.save();
    storage.saveAsDefault();
    webapp_push_state_to_clients();
    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
}
void action_setting_admin_reset_the_counter(lv_event_t *e)
{
    set_var_frame_numbers(0);
    completed_weld_cycles_in_frame = 0;
    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
}
// آیتم ۱۰: ریست فکتوری — بازگرداندن تنظیمات به «پیش‌فرضِ مدیر» (ذخیره‌شده با آیتم ۱۱)
void action_factory_reset_btn(lv_event_t *e)
{
    if (!storage.restoreDefaults()) return;
    int selectedUser = storage.getSelectedUser();
    set_var_welding_time(storage.getWildingTime(selectedUser));
    set_var_cooling_time(storage.getCoolingTime(selectedUser));
    set_var_element_temperature_target(storage.getTempTarget(selectedUser));
    set_var_setting_general_temperature_caliber(storage.getTempCaliber());
    set_var_setting_general_pid(storage.getPID());
    set_var_setting_general_speaker(storage.getSpeaker());
    set_var_setting_general_wifi(storage.getWifiOn());
    set_var_setting_general_automatic_start(storage.getAutoStart());
    set_var_setting_general_final_jack(storage.getFinalJack());
    set_var_setting_general_temperature_limit(storage.getTempLimit());
    if (storage.getIsTreePedal()) {
        set_var_setting_general_pedal_number3(true);
        set_var_setting_general_pedal_number2(false);
    } else {
        set_var_setting_general_pedal_number3(false);
        set_var_setting_general_pedal_number2(true);
    }
    set_var_setting_admin_thermocouple_j(storage.getIs_J_type());
    set_var_setting_admin_emergency_switch(storage.getEmergencySwich());
    set_var_setting_admin_jack_sensor(storage.getSensorCheck());

    set_var_setting_jack_safhe_raft((double)storage.get_Jack_safhe_raft());
    set_var_setting_jack_safhe_bargasht((double)storage.get_Jack_safhe_bargasht());
    set_var_setting_jack_shablon_raft((double)storage.get_Jack_shablon_raft());
    set_var_setting_jack_shablon_bargasht((double)storage.get_Jack_shablon_bargasht());
    set_var_setting_jack_profil_raft((double)storage.get_Jack_prifil_raft());
    set_var_setting_jack_profil_bargasht((double)storage.get_Jack_prifil_bargasht());
    set_var_setting_jack_josh_raft((double)storage.get_Jack_josh_raft());
    set_var_setting_jack_josh_bargasht((double)storage.get_Jack_josh_bargasht());
    set_var_jack_dot2_zarbe1_raft((double)storage.get_Jack_zarbe_1_raft());
    set_var_jack_dot2_zarbe2_raft((double)storage.get_Jack_zarbe_2_raft());

    int restoredLang = storage.getLang();
    set_var_lang(restoredLang);
    eez_flow_set_language_index(restoredLang == 3 ? 2 : (restoredLang == 2 ? 1 : 0));
    apply_language_now(restoredLang);
    webapp_push_state_to_clients();

    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
}
void action_setting_admin_save_btn(lv_event_t *e)
{
    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
    storage.setIs_J_type(get_var_setting_admin_thermocouple_j());
    // تایمر معکوس T1: فقط اگر کاربر اسلایدر را در این جلسه تغییر داده
    if (g_countdown_touched) {
        int n = g_countdown_draft_days;
        if (n < 0) n = 0;
        if (n > 125) n = 125;
        long today = today_jdn_from_rtc();
        storage.setReverseTimer(n);
        storage.setCountdownEnabled(n > 0);
        if (today != 0) {
            if (n > 0) storage.startTimerChain(n, today);
            else storage.startCountdown(0, today);
        }
        set_var_setting_admin_countdown_timer(n);
        g_countdown_draft_days = n;
        g_countdown_touched = false;
    }
    // تایمر معکوس T2..T5: فقط با زدن «ذخیره» از پیش‌نویس به حافظه می‌رود
    admin_countdown_apply_multi_timers();
    storage.setEmergencySwich(get_var_setting_admin_emergency_switch());
    storage.setSensorCheck(get_var_setting_admin_jack_sensor());

    String adPass = String(get_var_setting_admin_password_placeholder());
    String adRePass = String(get_var_setting_admin_re_enter_password_placeholder());
    adPass.trim();
    adRePass.trim();

    if (adPass != "")
    {

        if (adPass == adRePass)
        {
            storage.setAdminPassword(adPass);
            set_var_admin_can_save(true);
        }
        else
        {
            set_var_setting_admin_pass_error(ML("رمز عبور با تکرار آن برابر نیست", "Passwords do not match", "Sifreler uyusmuyor"));
        }
    }
    else
    {
        set_var_admin_can_save(true);
    }

    admin_rtc_apply_save();
}
// شمارندهٔ تلاش‌های اشتباه برای ورود به تنظیمات مدیر (در RAM؛ خودِ قفل در storage ماندگار است).
static int admin_entry_wrong_count = 0;

// پیام قفل ۱۰ دقیقه‌ای را با زمان باقی‌مانده می‌سازد و روی صفحهٔ ورود مدیر نشان می‌دهد.
static void show_admin_lock_message(unsigned long remainingSec)
{
    int mins = (int)((remainingSec + 59) / 60); // به دقیقهٔ بالا گرد می‌شود
    if (mins < 1) mins = 1;
    static char buf[64];
    snprintf(buf, sizeof(buf),
             ML("قفل شد — %d دقیقه صبر کنید", "Locked — wait %d min", "Kilitli — %d dk bekleyin"), mins);
    set_var_wrong_password_atemp(buf);
}

volatile bool g_admin_unlocked = false; // قفل مدیر در همین نشست

void action_admin_pass_keyboard_redy(lv_event_t *e)
{
    String adPass = String(get_var_admin_pass());
    adPass.trim();
    long today = today_jdn_from_rtc();
    unsigned long nowEpoch = (unsigned long)rtc_now_s().unixtime();

    // رمز مادر (مستر) همیشه باز می‌کند و همهٔ قفل‌ها را پاک می‌کند (تصمیم کاربر، آیتم ۷).
    if (adPass == MASTER_ADMIN_PASSWORD)
    {
        admin_entry_wrong_count = 0;
        storage.clearAdminLock();
        // فقط اگر دستگاه واقعاً قفل (منقضی) است شمارش پاک شود؛ ورودِ عادی به تنظیمات
        // نباید تایمرِ درحال‌شمارش را از بین ببرد.
        if (today != 0 && storage.isLockedNow(today)) {
            storage.clearLockAndCountdown();
            set_var_setting_admin_countdown_timer(0);
        }
        set_var_wrong_password_atemp("");
        set_var_admin_pass_verifyed(true);
        g_admin_unlocked = true;
        set_var_admin_pass("");
        return;
    }

    // اگر قفل ۱۰ دقیقه‌ای فعال است، جز رمز مادر هیچ رمزی پذیرفته نمی‌شود.
    if (storage.isAdminLocked(nowEpoch))
    {
        show_admin_lock_message(storage.adminLockRemainingSec(nowEpoch));
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        set_var_admin_pass("");
        return;
    }

    // رمز مدیر صحیح — ورود و پاک‌سازی شمارنده/قفل.
    if (adPass == storage.getAdminPassword())
    {
        admin_entry_wrong_count = 0;
        storage.clearAdminLock();
        // فقط اگر دستگاه واقعاً قفل (منقضی) است شمارش پاک شود؛ ورودِ عادیِ مدیر به تنظیمات نه.
        if (today != 0 && storage.isLockedNow(today)) {
            storage.clearLockAndCountdown();
            set_var_setting_admin_countdown_timer(0);
        }
        set_var_wrong_password_atemp("");
        set_var_admin_pass_verifyed(true);
        g_admin_unlocked = true;
        set_var_admin_pass("");
    }
    // رمز موقت ۱۱۲۲۳۳ — فقط یک‌بار ۱۰ روز مهلت می‌دهد (آیتم ۶).
    else if (adPass == TEMP_UNLOCK_PASSWORD)
    {
        if (today != 0 && !storage.getTempUnlockUsed() && storage.useTempUnlock(today))
        {
            admin_entry_wrong_count = 0;
            set_var_setting_admin_countdown_timer(10);
            set_var_wrong_password_atemp(ML("رمز موقت پذیرفته شد؛ ۱۰ روز شارژ شد", "Temporary password accepted; 10 days added", "Gecici sifre kabul edildi; 10 gun eklendi"));
            set_var_admin_pass_verifyed(false);
            g_admin_unlocked = false;
        }
        else if (storage.getTempUnlockUsed())
        {
            set_var_wrong_password_atemp(ML("رمز موقت قبلاً استفاده شده", "Temporary password already used", "Gecici sifre daha once kullanildi"));
        }
        else
        {
            set_var_wrong_password_atemp(get_wrong_password_text());
        }
        set_var_admin_pass("");
    }
    // رمز اشتباه — شمارش و در صورت رسیدن به ۳ بار، قفل ۱۰ دقیقه‌ای ماندگار.
    else
    {
        admin_entry_wrong_count++;
        if (admin_entry_wrong_count >= 3)
        {
            storage.lockAdminFor(nowEpoch, 10UL * 60UL); // ۱۰ دقیقه
            admin_entry_wrong_count = 0;
            show_admin_lock_message(storage.adminLockRemainingSec(nowEpoch));
        }
        else
        {
            int left = 3 - admin_entry_wrong_count;
            static char wbuf[80];
            snprintf(wbuf, sizeof(wbuf),
                     ML("رمز اشتباه — %d تلاش دیگر", "Wrong password — %d attempts left", "Yanlis sifre — %d deneme kaldi"), left);
            set_var_wrong_password_atemp(wbuf);
        }
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        set_var_admin_pass("");
    }
}

void action_mode(lv_event_t *e)
{
    void *userdata_ptr = lv_event_get_user_data(e);
    if(get_var_step_number()==0 && !get_var_step_process() && !get_var_test_mode()){
        int id = (int)lv_event_get_user_data(e);

        String selectedUserName = ("F" + String(id));
        set_var_enable_mode(selectedUserName.c_str());
    
        storage.setSelectedUser(id);
    
        set_var_welding_time(storage.getWildingTime(id));
        set_var_cooling_time(storage.getCoolingTime(id));
        set_var_element_temperature_target(storage.getTempTarget(id));
        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    
        Serial.print("USER : ");
        Serial.println(storage.getSelectedUser());
    }

}
void action_welding_time_changed(lv_event_t *e)
{
    schedule_base_values_save();

    Serial.print("W : ");
    Serial.println(get_var_welding_time());
}
void action_cooling_time_changed(lv_event_t *e)
{
    schedule_base_values_save();
    Serial.print("C : ");
    Serial.println(get_var_cooling_time());
}
void action_element_temp_target_changed(lv_event_t *e)
{
    schedule_base_values_save();
    Serial.print("T : ");
    Serial.println(get_var_element_temperature_target());
}

void action_base_values_changed(lv_event_t *e)
{
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    storage.setWildingTime(get_var_welding_time(), storage.getSelectedUser());
    storage.setCoolingTime(get_var_cooling_time(), storage.getSelectedUser());
    storage.setTempTarget(get_var_element_temperature_target(), storage.getSelectedUser());
    pending_base_values_save = false;
    webapp_push_state_to_clients();

    Serial.print("W : ");
    Serial.println(get_var_welding_time());
    Serial.print("C : ");
    Serial.println(get_var_cooling_time());
    Serial.print("T : ");
    Serial.println(get_var_element_temperature_target());
}
void action_jack_value_set(lv_event_t *e)
{
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    storage.set_jack_safhe_raft(abs(get_var_setting_jack_safhe_raft()));
    storage.set_jack_safhe_bargasht(abs(get_var_setting_jack_safhe_bargasht()));


    storage.set_jack_shablon_raft(abs(get_var_setting_jack_shablon_raft()));
    storage.set_jack_shablon_bargasht(abs(get_var_setting_jack_shablon_bargasht()));

    storage.set_jack_prifil_raft(abs(get_var_setting_jack_profil_raft()));
    storage.set_jack_prifil_bargasht(abs(get_var_setting_jack_profil_bargasht()));

    storage.set_jack_josh_raft(abs(get_var_setting_jack_josh_raft()));
    storage.set_jack_josh_bargasht(abs(get_var_setting_jack_josh_bargasht()));
    storage.set_jack_zarbe_1_raft(abs(get_var_jack_dot2_zarbe1_raft()));
    storage.set_jack_zarbe_2_raft(abs(get_var_jack_dot2_zarbe2_raft()));
    webapp_push_state_to_clients();
}

// جک تقویت میز = zarbe1/zarbe2 (نه جک المنت/josh). جک ۱ UI → zarbe_2، جک ۲ UI → zarbe_1.
extern "C" int jack_reinforce_stored_ms1(void) {
    return storage.get_Jack_zarbe_2_raft();
}

extern "C" int jack_reinforce_stored_ms2(void) {
    return storage.get_Jack_zarbe_1_raft();
}

extern "C" void jack_reinforce_save_ms(int ms1, int ms2) {
    if (ms1 < 0) ms1 = 0;
    if (ms2 < 0) ms2 = 0;
    set_var_jack_dot2_zarbe2_raft((double)ms1);
    set_var_jack_dot2_zarbe1_raft((double)ms2);
    storage.set_jack_zarbe_2_raft(ms1);
    storage.set_jack_zarbe_1_raft(ms2);
}

void action_setting_general_save_button(lv_event_t *e)
{
    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
    storage.setLang(get_var_lang()); // زبان هم ذخیره شود تا با خاموش/روشن باقی بماند
    eez_flow_set_language_index(get_var_lang() == 3 ? 2 : (get_var_lang() == 2 ? 1 : 0));
    storage.setTempCaliber(get_var_setting_general_temperature_caliber());
    storage.setPID(get_var_setting_general_pid());
    storage.setSpeaker(get_var_setting_general_speaker());

    const bool wantWifi = get_var_setting_general_wifi();
    const bool apWasOn = webAppEnabled;
    storage.setWifiOn(wantWifi);
    // مهم: قبلاً هر Save با restart=true کل SoftAP را stop/start می‌کرد → گاه‌به‌گاه panic/ریست.
    // SoftAP فقط وقتی لازم است عوض شود که روشن/خاموش با وضعیتِ واقعی فرق کند،
    // یا روشن است ولی Beacon مرده. تغییر اسپیکر/PID/… نباید رادیو را بکشد.
    bool wifiSync = false;
    if (wantWifi != apWasOn) {
        g_webapp_sync_restart = false;
        g_webapp_sync_requested = true;
        wifiSync = true;
    } else if (wantWifi && !webApp.isApUp()) {
        g_webapp_sync_restart = true;
        g_webapp_sync_requested = true;
        wifiSync = true;
    }
    Serial.printf("[WIFI] save: want=%d was=%d sync=%d\n",
                  (int)wantWifi, (int)apWasOn, (int)wifiSync);

    storage.setAutoStart(get_var_setting_general_automatic_start());
    storage.setFinalJack(get_var_setting_general_final_jack());
    storage.setTempLimit(get_var_setting_general_temperature_limit());

    if (get_var_setting_general_pedal_number3())
    {
        storage.setIsTreePedal(true);
        set_var_setting_general_pedal_number3(true);
        set_var_setting_general_pedal_number2(false);
    }
    else
    {
        storage.setIsTreePedal(false);
        set_var_setting_general_pedal_number3(false);
        set_var_setting_general_pedal_number2(true);
    }
    // وسطِ stop/start رادیو handshake نزن (حافظه/وای‌فای هم‌زمان → ریست)
    if (!wifiSync)
        webapp_push_state_to_clients();
}
static void wifi_gather_connect_credentials(String &ssidOut, String &passOut)
{
    int idx = wifi_resolve_connect_index();
    ssidOut = wifi_ssid_at_index(idx);
    // رمز از متغیر فلو (VALUE_CHANGED textarea آن را پر می‌کند) — نه lv_textarea از Worker
    const char *pw = get_var_wifi_password();
    passOut = (pw && pw[0]) ? String(pw) : String("");
    passOut.trim();
    // فقط رمزِ همین SSID از حافظه — هرگز رمز شبکهٔ دیگر (مثل وای‌فای خانه) را برای هات‌اسپات نگیر
    if (passOut.length() == 0 && ssidOut.length() > 0) {
        if (storage.hasWifi(ssidOut))
            passOut = storage.getWifiPassword(ssidOut);
        else if (ssidOut == storage.getWifiSsid())
            passOut = storage.getWifiPassword();
    }
}

static TaskHandle_t wifiConnectTaskHandle = NULL;

static void wifi_connect_worker_task(void *param)
{
    (void)param;
    String ssid = selectedWifi;
    String pass = wifiPassword;

    Serial.printf("[WIFI-WORKER] starting async connect ssid='%s'\n", ssid.c_str());
    set_var_waitig_panle_state(true);
    set_var_connection_error_label("");

    webAppEnabled = false;
    webApp.wStopAp();
    vTaskDelay(pdMS_TO_TICKS(50));

    int scan_idx = wifi_resolve_connect_index();
    bool ok = wifi_sta_connect_full(ssid.c_str(), pass.c_str(), scan_idx, 30, NULL);
    set_var_waitig_panle_state(false);

    if (ok && WiFi.status() == WL_CONNECTED)
    {
        WiFi.setAutoReconnect(true);
        String connected_ssid = WiFi.SSID();
        if (connected_ssid.length() == 0) connected_ssid = ssid;
        if (storage.getWifiSsid() != connected_ssid || storage.getWifiPassword() != pass) {
            storage.setWifiCredentials(connected_ssid, pass);
            storage.saveWifi(connected_ssid, pass);
        }
        storage.setWifiOn(true);
        set_var_is_wifi_connected(true);
        set_var_connection_error_label("");
        wifi_on_connected_hooks();
        Serial.printf("[WIFI-WORKER] connected successfully IP=%s (ssid='%s')\n",
            WiFi.localIP().toString().c_str(), connected_ssid.c_str());
    }
    else
    {
        WiFi.setAutoReconnect(false);
        WiFi.disconnect(false, false);
        set_var_is_wifi_connected(false);
        if (g_wifi_abort_request) {
            g_wifi_abort_request = false;
            char abort_msg[80];
            int lang = eez_flow_get_language_index();
            if (lang == 1) snprintf(abort_msg, sizeof(abort_msg), "Connection canceled");
            else if (lang == 2) snprintf(abort_msg, sizeof(abort_msg), "Baglanti iptal edildi");
            else snprintf(abort_msg, sizeof(abort_msg), "اتصال توسط کاربر لغو شد");
            set_var_connection_error_label(abort_msg);
            ui_request_toast_colored(abort_msg, 0xff777777, 2500);
        } else {
            char fail_msg[160];
            int lang = eez_flow_get_language_index();
            int reason = g_wifi_last_disconnect_reason;
            if (reason == 202) {
                if (lang == 1) snprintf(fail_msg, sizeof(fail_msg), "Incorrect password for %s", ssid.c_str());
                else if (lang == 2) snprintf(fail_msg, sizeof(fail_msg), "%s sifresi hatali", ssid.c_str());
                else snprintf(fail_msg, sizeof(fail_msg), "رمز عبور %s اشتباه است", ssid.c_str());
            } else if (reason == 201) {
                if (lang == 1) snprintf(fail_msg, sizeof(fail_msg), "Network %s not found (weak signal)", ssid.c_str());
                else if (lang == 2) snprintf(fail_msg, sizeof(fail_msg), "%s agi bulunamadi (zayif sinyal)", ssid.c_str());
                else snprintf(fail_msg, sizeof(fail_msg), "شبکه %s یافت نشد (آنتن‌دهی ضعیف)", ssid.c_str());
            } else {
                if (lang == 1) snprintf(fail_msg, sizeof(fail_msg), "Failed to connect to %s (check password/signal)", ssid.c_str());
                else if (lang == 2) snprintf(fail_msg, sizeof(fail_msg), "%s baglanti hatasi (sifre/sinyali kontrol edin)", ssid.c_str());
                else snprintf(fail_msg, sizeof(fail_msg), "خطا در اتصال به %s (رمز یا آنتن را بررسی کنید)", ssid.c_str());
            }
            set_var_connection_error_label(fail_msg);
            ui_request_toast_colored(fail_msg, 0xffd32f2f, 4000);
            wifi_ui_request_password_panel();
            Serial.printf("[WIFI-WORKER] connect failed status=%d reason=%d msg='%s'\n",
                (int)WiFi.status(), reason, fail_msg);
        }
    }

    connectingToWifi = false;
    wifiConnectTaskHandle = NULL;
    vTaskDelete(NULL);
}

static void request_async_wifi_connect(void)
{
    if (wifiConnectTaskHandle != NULL) {
        eTaskState st = eTaskGetState(wifiConnectTaskHandle);
        if (st == eDeleted || st == eInvalid) wifiConnectTaskHandle = NULL;
    }
    if (wifiConnectTaskHandle == NULL) {
        webAppEnabled = false;

        BaseType_t ok = xTaskCreatePinnedToCore(wifi_connect_worker_task, "WifiConn", 6144, NULL, 3, &wifiConnectTaskHandle, 0);
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(wifi_connect_worker_task, "WifiConn", 5120, NULL, 3, &wifiConnectTaskHandle, 0);
        }
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(wifi_connect_worker_task, "WifiConn", 4096, NULL, 3, &wifiConnectTaskHandle, 0);
        }
        if (ok != pdPASS) {
            connectingToWifi = false;
            set_var_connection_error_label(ML("خطا در ساخت تسک", "Task failed", "Hata"));
        }
    }
}

void action_btn_wifi_connect_event(lv_event_t *e)
{
    (void)e;
    if (objects.drop_down_wifi_list) {
        int32_t dd_idx = lv_dropdown_get_selected(objects.drop_down_wifi_list);
        if (dd_idx >= 0) {
            set_var_selected_wifi_index(dd_idx);
        }
    }
    // همگام‌سازی آخر از textarea به var فقط روی نخ LVGL (این action)
    if (objects.text_area_wifi_password) {
        const char *ta = lv_textarea_get_text(objects.text_area_wifi_password);
        if (ta) set_var_wifi_password(ta);
    }
    String ssid, pass;
    wifi_gather_connect_credentials(ssid, pass);
    if (ssid.length() == 0) {
        Serial.println("[WIFI-CONN] abort: empty SSID");
        set_var_connection_error_label(ML("شبکه را انتخاب کنید", "Select a network", "Ag secin"));
        return;
    }
    selectedWifi = ssid;
    wifiPassword = pass;
    set_var_selected_wifi_network(ssid.c_str());
    if (pass.length() > 0) set_var_wifi_password(pass.c_str());

    int idx = wifi_resolve_connect_index();
    int auth = (idx >= 0 && idx < g_wifi_scan_meta_count) ? (int)g_wifi_scan_authmode[idx] : -1;
    if (auth < 0 && ssid.length() > 0) {
        for (int i = 0; i < g_wifi_scan_meta_count; i++) {
            if (strcmp(g_wifi_scan_ssid[i], ssid.c_str()) == 0 ||
                strcmp(g_wifi_scan_raw_ssid[i], ssid.c_str()) == 0) {
                auth = (int)g_wifi_scan_authmode[i];
                break;
            }
        }
    }
    if (pass.length() == 0 && auth > 0 && auth != WIFI_AUTH_OPEN) {
        bool panel_open = objects.panle_wifi_password &&
            !lv_obj_has_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        if (!panel_open) {
            wifi_password_panel_show();
            set_var_connection_error_label("");
            Serial.printf("[WIFI-CONN] open password panel ssid='%s' auth=%d\n", ssid.c_str(), auth);
            return;
        }
        set_var_connection_error_label(ML("رمز وای‌فای را وارد کنید", "Enter WiFi password", "WiFi sifresini girin"));
        Serial.printf("[WIFI-CONN] abort: secured ssid='%s' auth=%d no password\n", ssid.c_str(), auth);
        return;
    }

    wifi_password_panel_hide();
    if (g_self_update_session) {
        if (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(100))) {
            connectingToWifi = true;
            xSemaphoreGive(xMutex_Wifi);
        } else {
            connectingToWifi = true;
        }
    } else {
        request_async_wifi_connect();
    }
    Serial.printf("[WIFI-CONN] request ssid='%s' idx=%d passLen=%d auth=%d\n",
        ssid.c_str(), idx, (int)pass.length(), auth);
}

void action_btn_connect(lv_event_t *e)
{
    (void)e;
    if (objects.drop_down_wifi_list) {
        int32_t dd_idx = lv_dropdown_get_selected(objects.drop_down_wifi_list);
        if (dd_idx >= 0) {
            set_var_selected_wifi_index(dd_idx);
        }
    }
    int idx = wifi_resolve_connect_index();
    selectedWifi = wifi_ssid_at_index(idx);
    set_var_selected_wifi_network(selectedWifi.c_str());
}

static bool wifi_radio_lock(uint32_t waitMs)
{
    if (xMutex_Wifi == NULL) xMutex_Wifi = xSemaphoreCreateMutex();
    return xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(waitMs)) == pdTRUE;
}

static void wifi_radio_unlock(void)
{
    if (xMutex_Wifi) xSemaphoreGive(xMutex_Wifi);
}

static void wifi_publish_networks(const char *value)
{
    set_var_available_wifi_networks(value);
    // فقط پرچم — رفرش UI و کنترل اسپینر فقط در نخ LVGL (screens.c / tick_screen_screen_wifi)
    wifi_ui_request_refresh();
}

// تشخیصِ علتِ «اتصال ناموفق»: کدِ دلیلِ قطعِ STA را چاپ می‌کند.
// reason 15 = 4WAY_HANDSHAKE_TIMEOUT / 205 = CONNECTION_FAIL (اغلب رمزِ اشتباه)،
// 201 = NO_AP_FOUND، 2 = AUTH_EXPIRE، 3/4 = ASSOC/AUTH_FAIL.
static void wifi_diag_event(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        g_wifi_last_disconnect_reason = (int)info.wifi_sta_disconnected.reason;
        Serial.printf("[WIFI-EVT] STA disconnected reason=%d\n", g_wifi_last_disconnect_reason);
        cloud_client_on_wifi_disconnected();
    } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        Serial.println("[WIFI-EVT] STA connected (assoc ok)");
    } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
        g_wifi_saw_got_ip = true;
        Serial.println("[WIFI-EVT] STA got IP");
        cloud_client_on_wifi_connected();
    } else if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
        Serial.printf("[WIFI-AP] Client connected MAC: %02X:%02X:%02X:%02X:%02X:%02X aid=%d\n",
                      info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                      info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                      info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5],
                      info.wifi_ap_staconnected.aid);
    } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
        Serial.printf("[WIFI-AP] Client disconnected MAC: %02X:%02X:%02X:%02X:%02X:%02X aid=%d\n",
                      info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                      info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                      info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5],
                      info.wifi_ap_stadisconnected.aid);
    } else if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
        Serial.printf("[WIFI-AP] Client assigned IP: %s\n", IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
    }
}

static void wifi_prepare_sta_for_scan(void)
{
    webAppEnabled = false;
    // دانش فنی: برای اسکن STA فقط wStopAp — نه wStop.
    // wStop() → WiFi.mode(WIFI_OFF) و راه‌اندازیِ دوبارهٔ STA زیر فشار رم (صفحه اطلاعات + SelfUpd)
    // با «Expected to init 4 rx buffer…» / SCAN FAILED می‌شکند. مسیر F5 وب‌اپ ندارد و این مشکل را نمی‌بیند.
    webApp.wStopAp();
    WiFi.softAPdisconnect(true);
    WiFi.persistent(false);
    WiFi.disconnect(false, false);
    // فاصله کوتاه برای آزادشدنِ بافرهای داخلیِ AP قبل از راه‌اندازیِ STA
    vTaskDelay(pdMS_TO_TICKS(50));
    Serial.printf("[MEM-DIAG] before mode(STA): internalFree=%u maxAlloc=%u psramFree=%u\n",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)ESP.getMaxAllocHeap(),
        (unsigned)(ESP.getPsramSize() ? ESP.getFreePsram() : 0));
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    vTaskDelay(pdMS_TO_TICKS(100));
}

// اسکن STA: اول Arduino (تا WiFiScanClass پر شود — دانش فنی)، در شکست esp_wifi + صبر برای ingest آردوینو
static int wifi_esp_sta_scan(int maxAttempts)
{
    if (!wifi_radio_lock(15000)) {
        Serial.println("[WIFI-SCAN] radio mutex timeout");
        return -1;
    }

    WiFi.persistent(false);
    wifi_prepare_sta_for_scan();
    WiFi.scanDelete();

    int sc = -1;
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        if (g_wifi_abort_request) {
            esp_wifi_scan_stop();
            WiFi.scanDelete();
            sc = -1;
            break;
        }
        if (attempt > 0) {
            esp_wifi_scan_stop();
            WiFi.scanDelete();
            WiFi.disconnect(false, false);
            vTaskDelay(pdMS_TO_TICKS(500));
            WiFi.mode(WIFI_STA);
            WiFi.setSleep(false);
            esp_wifi_set_ps(WIFI_PS_NONE);
            vTaskDelay(pdMS_TO_TICKS(400));
        }

        g_wifi_direct_scan_active = true;

        // مسیر اصلی: اسکن سریع صنعتی — dwell 110ms/ch × 14ch ≈ 1.3s (قبلاً 400ms = 5.6s)
        // show_hidden=false: شبکه‌های مخفی در کارگاه جوشکاری نیازی به اسکن ندارند
        sc = WiFi.scanNetworks(false, false, false, 110, 0);
        Serial.printf("[WIFI-SCAN] scanNetworks=%d attempt=%d heap=%u maxAlloc=%u\n",
            sc, attempt, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
        if (sc >= 0) break;

        // Fallback: esp raw — بعد باید صبر کنیم تا هندلر آردوینو SCAN_DONE را بخورد
        WiFi.scanDelete();
        wifi_scan_config_t cfg = {};
        cfg.show_hidden = false;
        cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        cfg.scan_time.active.min = 80;   // قبلاً 120 — کاهش برای اسکن سریع‌تر
        cfg.scan_time.active.max = 120;  // قبلاً 450 — روترها هر 100ms بیکن دارند
        esp_err_t err = esp_wifi_scan_start(&cfg, true);
        Serial.printf("[WIFI-SCAN] esp_wifi_scan_start err=%d attempt=%d\n", (int)err, attempt);
        if (err == ESP_OK) {
            sc = -1;
            uint32_t waited = 0;
            while (waited < 2000) {
                if (g_wifi_abort_request) { esp_wifi_scan_stop(); break; }
                int done = WiFi.scanComplete();
                if (done >= 0) { sc = done; break; }
                if (done == WIFI_SCAN_FAILED) break;
                vTaskDelay(pdMS_TO_TICKS(50));
                waited += 50;
            }
            Serial.printf("[WIFI-SCAN] esp→arduino sc=%d\n", sc);
            if (sc >= 0) break;
        } else if (err == ESP_ERR_WIFI_NOT_STARTED) {
            esp_wifi_start();
            vTaskDelay(pdMS_TO_TICKS(400));
        } else if (err == ESP_ERR_WIFI_NOT_INIT) {
            // کمبود بافر rx — retry فقط حافظه را بدتر می‌کند
            break;
        }
    }

    // عمداً g_wifi_direct_scan_active را اینجا false نکن — تا پایانِ collect در
    // wifi_perform_sync_scan_and_update_ui باید روشن بماند؛ وگرنه app_task/ota_task با
    // مسیر قدیمی scanComplete وسط خواندن SSIDها scanDelete می‌زنند → هنگ/ریست.
    g_su_scan_rc = sc;
    wifi_radio_unlock();
    Serial.printf("[WIFI-SCAN] result=%d\n", sc);
    return sc;
}

static void wifi_esp_collect_ssids(int sc, String *ssids, int *rssis, int *cnt, int maxw)
{
    *cnt = 0;
    wifi_scan_meta_reset();
    if (sc <= 0) return;

    auto push_rec = [&](const String &s, const String &raw, int rssi, const uint8_t *bssid, int channel, int authmode) {
        if (s.length() == 0 && raw.length() == 0) return;
        String eff_s = (s.length() > 0) ? s : raw;
        for (int j = 0; j < *cnt; j++) {
            if (ssids[j] == eff_s) {
                if (rssi > rssis[j]) {
                    rssis[j] = rssi;
                    wifi_scan_meta_set_slot(j, eff_s.c_str(), raw.c_str(), bssid, channel, authmode);
                }
                return;
            }
        }
        if (*cnt < maxw) {
            ssids[*cnt] = eff_s;
            rssis[*cnt] = rssi;
            wifi_scan_meta_set_slot(*cnt, eff_s.c_str(), raw.c_str(), bssid, channel, authmode);
            Serial.printf("[WIFI-SCAN] #%d ssid='%s' raw='%s' rssi=%d ch=%d auth=%d\n",
                *cnt, eff_s.c_str(), raw.c_str(), rssi, channel, authmode);
            (*cnt)++;
        }
    };

    // دانش فنی باگ ۲: هرگز esp_wifi_scan_get_ap_records نخوان — هندلر آردوینو رکوردها را می‌برد.
    // فقط WiFi.SSID() / scanComplete().
    // O2-FIX: وقتی scanNetworks مسدودکننده اجرا شده و sc>0 است، scanComplete باید همان مقدار
    // را فوری برگرداند — حلقه ۱۵۰۰ms فقط برای fallback async نیاز است.
    int ar_n;
    if (sc > 0) {
        // اسکن synchronous کامل است — scanComplete باید فوری جواب دهد
        ar_n = WiFi.scanComplete();
        if (ar_n < 0) ar_n = sc; // اگر مقدار هنوز ingest نشده، از sc استفاده کن
    } else {
        // fallback async: صبر کوتاه برای ingest هندلر آردوینو
        ar_n = WiFi.scanComplete();
        uint32_t waited = 0;
        while (ar_n < 0 && waited < 600) {
            vTaskDelay(pdMS_TO_TICKS(40));
            waited += 40;
            ar_n = WiFi.scanComplete();
        }
    }

    Serial.printf("[WIFI-COLLECT] arduino n=%d (raw=%d)\n", ar_n, sc);
    if (ar_n <= 0) return;

    for (int i = 0; i < ar_n && *cnt < maxw; i++) {
        String raw = WiFi.SSID(i);
        String s = wifi_ssid_from_record_bytes((const uint8_t *)raw.c_str(), 32);
        uint8_t *b = WiFi.BSSID(i);
        push_rec(s, raw, WiFi.RSSI(i), b, WiFi.channel(i), (int)WiFi.encryptionType(i));
    }
    g_wifi_scan_meta_count = *cnt;
    Serial.printf("[WIFI-COLLECT] unique=%d\n", *cnt);
}

// اسکنِ STA — OTA، بروزرسانیِ درجا، تنظیمات
int wifi_sta_sync_scan_raw(int maxAttempts)
{
    return wifi_esp_sta_scan(maxAttempts);
}

static void wifi_collect_unique_ssids(int sc, String *ssids, int *rssis, int *cnt, int maxw)
{
    wifi_esp_collect_ssids(sc, ssids, rssis, cnt, maxw);
}

bool wifi_perform_sync_scan_and_update_ui(void)
{
    // از همین لحظه تا پایان collect — app_task/ota مسیر قدیمی scanComplete را اجرا نکنند
    g_wifi_direct_scan_active = true;
    isSearching = true;
    wifi_publish_networks("SEARCHING");

    int sc = -1;
    int cnt = 0;
    const int MAXW = 24;
    static String ssids[MAXW];
    static int rssis[MAXW];

    sc = wifi_sta_sync_scan_raw(2);
    if (g_wifi_abort_request) {
        g_wifi_abort_request = false;
        isSearching = false;
        g_wifi_direct_scan_active = false;
        esp_wifi_scan_stop();
        WiFi.scanDelete();
        Serial.println("[WIFI-SCAN] Scan aborted by STOP button");
        return false;
    }
    wifi_collect_unique_ssids(sc, ssids, rssis, &cnt, MAXW);

    String names = "";
    for (int i = 0; i < cnt; i++) {
        if (i > 0) names += "#";
        names += ssids[i];
    }

    if (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(200))) {
        wifi_names = names;
        isSearching = false;
        xSemaphoreGive(xMutex_Wifi);
    } else {
        wifi_names = names;
        isSearching = false;
    }

    if (cnt > 0) {
        wifi_publish_networks(names.c_str());
    } else {
        wifi_publish_networks(sc == 0 ? "NO WIFI FOUND" : "SCAN FAILED");
    }

    int pick = 0;
    String saved = storage.getWifiSsid();
    if (saved.length() > 0) {
        for (int i = 0; i < cnt; i++) {
            if (ssids[i] == saved) { pick = i; break; }
        }
    }
    set_var_selected_wifi_index(pick);
    wifi_on_dropdown_selection_changed(pick);
    esp_wifi_scan_stop();
    WiFi.scanDelete();
    g_wifi_direct_scan_active = false;
    Serial.printf("[WIFI-SCAN] raw=%d shown=%d\n", sc, cnt);

    // اتصالِ خودکار حذف شد: کاربر باید خودش شبکه را از لیست انتخاب و دکمهٔ اتصال را بزند.
    // (قبلاً اگر دقیقاً یک شبکهٔ ذخیره‌شده در دسترس بود، بی‌اجازه وصل می‌شد و اجازهٔ انتخاب نمی‌داد.)
    return cnt > 0;
}

static void wifi_scan_starter_task(void *param)
{
    (void)param;
    set_var_connection_error_label("");
    set_var_waitig_panle_state(false);
    webAppEnabled = false;
    // FIX-SCAN-1: متوقف کردن AP و سوئیچ به STA قبل از اسکن
    // 30ms کافی نبود — AP هنوز روی WIFI_AP بود و اسکن sc=-1 می‌داد («خطا در جستجو»)
    webApp.wStopAp();
    vTaskDelay(pdMS_TO_TICKS(50));
    WiFi.mode(WIFI_STA);             // FIX-SCAN-2: اطمینان از mode=STA قبل از اسکن
    vTaskDelay(pdMS_TO_TICKS(150));  // FIX-SCAN-3: 150ms settle برای radio init کامل
    wifi_perform_sync_scan_and_update_ui();
    wifiScanStarterTaskHandle = NULL;
    vTaskDelete(NULL);
}

static void cancel_wifi_scan_starter_state(void)
{
    g_wifi_direct_scan_active = false;
    isSearching = false;
}

static void request_wifi_scan_start(void)
{
    if (wifiScanStarterTaskHandle != NULL) return;
    // سپرِ فوری تا تسک استارت شود — جلوی تداخل مسیر قدیمی scanComplete
    g_wifi_direct_scan_active = true;
    isSearching = true;
    // عمداً wStopAp/vTaskDelay اینجا نیست (از نخ LVGL صدا می‌شود → فریز UI).
    // starter_task خودش AP را خاموش می‌کند.
    BaseType_t ok = xTaskCreatePinnedToCore(wifi_scan_starter_task, "WifiScanStart", 4096, NULL, 4, &wifiScanStarterTaskHandle, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(wifi_scan_starter_task, "WifiScanStart", 3584, NULL, 4, &wifiScanStarterTaskHandle, 0);
    }
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(wifi_scan_starter_task, "WifiScanStart", 3072, NULL, 4, &wifiScanStarterTaskHandle, 0);
    }
    if (ok != pdPASS) {
        wifiScanStarterTaskHandle = NULL;
        g_wifi_direct_scan_active = false;
        isSearching = false;
        wifi_publish_networks("SCAN FAILED");
        Serial.printf("[WIFI-SCAN] WifiScanStart task create failed (freeHeap=%u maxBlock=%u)\n",
                      (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    }
}

void action_btn_wifi_search_event(lv_event_t *e)
{
    (void)e;
    set_var_connection_error_label("");
    // O3-FIX: Debounce — اگر اسکن در جریان است کلیک مجدد نادیده گرفته می‌شود
    if (g_wifi_direct_scan_active || isSearching) {
        Serial.println("[WIFI-SCAN] debounce: scan already in progress, ignoring");
        return;
    }
    // بروزرسانیِ درجا: فقط self_update_task مالکِ رادیوست
    if (g_self_update_session) {
        g_wifi_rescan_request = true;
        return;
    }
    wifi_publish_networks("SEARCHING");
    if (get_var_ota_enable()) {
        g_wifi_ota_rescan_request = true;
        isSearching = true;
        return;
    }
    request_wifi_scan_start();
}

// === شروعِ بروزرسانیِ درجای صفحه اطلاعات ===
// در حالت عادی ota_task ساخته نشده؛ اینجا یک‌بار می‌سازیمش تا وای‌فا/چکِ سرور کار کند.
// وب‌اپ را خاموش می‌کنیم تا تداخلِ وای‌فا نباشد. اگر وای‌فا ذخیره شده، خودکار وصل می‌شویم.
// سینکِ ساعت از اینترنت (NTP) — منطقهٔ زمانی ثابت ایران.
// مهم: ایران از ۲۰۲۲ دیگر DST ندارد (همیشه UTC+3:30 = 12600).
// قبلاً offset از ip-api خوانده می‌شد؛ بعضی پاسخ‌ها هنوز +4:30 (16200) می‌دادند
// → بعد از وصل شدن به وای‌فای، RTC یک‌ساعت جلو می‌پرید.
static void ntp_apply_header_from_tm(const struct tm &ti)
{
    char clock_text[8];
    snprintf(clock_text, sizeof(clock_text), "%02d:%02d", ti.tm_hour, ti.tm_min);
    set_var_header_time_text(clock_text);
    int hy = 0, hm = 0, hd = 0;
    date_utils::gregorian_to_jalali(ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, hy, hm, hd);
    char date_text[16];
    snprintf(date_text, sizeof(date_text), "%04d/%02d/%02d", hy, hm, hd);
    set_var_header_date_text(date_text);
}

static void ntp_sync_worker_task(void *param)
{
    (void)param;
    const int gmtOffset = 12600; // IRST ثابت — بدون DST
    configTime(gmtOffset, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    struct tm ti;
    if (getLocalTime(&ti, 2000)) {
        if (ti.tm_year + 1900 >= 2024) {
            DateTime ntpNow(ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec);
            rtc_adjust_s(ntpNow);
            ntp_apply_header_from_tm(ti);
            g_rtc_warn_pending = false;
            Serial.printf("[NTP] RTC synced in background: %04d-%02d-%02d %02d:%02d:%02d (IRST +3:30)\n",
                          ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec);
        }
    }
    vTaskDelete(NULL);
}

extern "C" bool wifi_sync_time_from_ntp(void)
{
    if (WiFi.status() != WL_CONNECTED) return false;
    static TaskHandle_t ntpTask = NULL;
    if (ntpTask != NULL) {
        eTaskState st = eTaskGetState(ntpTask);
        if (st != eDeleted) return true;
        ntpTask = NULL;
    }
    xTaskCreatePinnedToCore(ntp_sync_worker_task, "NtpSync", 4096, NULL, 1, &ntpTask, 0);
    return true;
}

// خروجی: تعدادِ شبکه‌های شناخته‌شده (دارایِ رمزِ ذخیره) که در نتایجِ اسکن دیده شدند را در
// knownCountOut می‌گذارد، و اگر دقیقاً یکی بود نامش را در autoSsidOut (برای اتصالِ خودکار).

static void su_pause_background_tasks_for_wifi(void);
static void su_resume_background_tasks_after_wifi(void);

// تسکِ اختصاصیِ بروزرسانیِ درجا — اتصال و OTA
void self_update_task(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(20));

    // مثل F5: webapp/app را بردار تا رم داخلی برای اسکن آزاد شود
    su_pause_background_tasks_for_wifi();
    vTaskDelay(pdMS_TO_TICKS(30));

    if (xMutex_Wifi == NULL) xMutex_Wifi = xSemaphoreCreateMutex();

    g_su_state = SU_IDLE;

    Serial.printf("[SU][MEM-DIAG] before scan: internalFree=%u maxAlloc=%u psramFree=%u\n",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)ESP.getMaxAllocHeap(),
        (unsigned)(ESP.getPsramSize() ? ESP.getFreePsram() : 0));

    if (g_self_update_session) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[SU] Already connected to WiFi -> checking server directly");
            self_update_check();
        } else {
            g_wifi_rescan_request = false;
            String savedSsid = storage.getWifiSsid();
            String savedPass = storage.getWifiPassword();
            if (savedSsid.length() > 0 && WiFi.status() != WL_CONNECTED) {
                Serial.printf("[SU] Auto-connecting directly to saved network '%s'\n", savedSsid.c_str());
                selectedWifi = savedSsid;
                wifiPassword = savedPass;
                connectingToWifi = true;
            } else {
                wifi_perform_sync_scan_and_update_ui();
            }
        }
    }
    esp_wifi_set_ps(WIFI_PS_NONE);
    Serial.printf("[SU-STACK] after scan, free stack=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(NULL));

    for (;;)
    {
        if (!g_self_update_session) {
            WiFi.scanDelete();
            g_wifi_direct_scan_active = false;
            isSearching = false;
            selfUpdateTaskHandle = NULL;
            su_resume_background_tasks_after_wifi();
            g_webapp_sync_requested = true;
            g_webapp_sync_restart = true;
            vTaskDelete(NULL);
        }

        if (g_wifi_rescan_request) {
            g_wifi_rescan_request = false;
            wifi_perform_sync_scan_and_update_ui();
        }

        if (connectingToWifi) {
            g_wifi_connect_in_progress = true;

            String ssid, pass;
            if (xMutex_Wifi && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(100))) {
                connectingToWifi = false;
                ssid = selectedWifi;
                pass = wifiPassword;
                xSemaphoreGive(xMutex_Wifi);
            } else {
                connectingToWifi = false;
                ssid = selectedWifi;
                pass = wifiPassword;
            }

            if (ssid.length() == 0) {
                g_wifi_connect_in_progress = false;
                set_var_connection_error_label(ML("شبکه را انتخاب کنید", "Select a network", "Ag secin"));
                g_su_state = SU_IDLE;
                continue;
            }

            if (pass.length() == 0) {
                if (storage.hasWifi(ssid))
                    pass = storage.getWifiPassword(ssid);
                else if (ssid == storage.getWifiSsid())
                    pass = storage.getWifiPassword();
                wifiPassword = pass;
            }

            g_su_state = SU_CONNECTING;
            set_var_waitig_panle_state(true);
            set_var_connection_error_label("");
            WiFi.scanDelete();
            g_wifi_direct_scan_active = false;
            isSearching = false;

            Serial.printf("[SU-CONN] begin ssid='%s' passLen=%d\n",
                ssid.c_str(), (int)pass.length());
            int scan_idx = wifi_resolve_connect_index();
            bool ok = wifi_sta_connect_full(ssid.c_str(), pass.c_str(), scan_idx, 30, NULL);
            g_wifi_connect_in_progress = false;
            set_var_waitig_panle_state(false);

            Serial.printf("[SU-CONN] result ok=%d status=%d\n", (int)ok, (int)WiFi.status());
            if (ok && WiFi.status() == WL_CONNECTED) {
                WiFi.setAutoReconnect(true);
                String connected_ssid = WiFi.SSID();
                if (connected_ssid.length() == 0) connected_ssid = ssid;
                storage.setWifiCredentials(connected_ssid, pass);
                storage.saveWifi(connected_ssid, pass);
                storage.setWifiOn(true);
                wifi_on_connected_hooks();
                if (g_self_update_session)
                    self_update_check();
            } else {
                WiFi.setAutoReconnect(false);
                WiFi.disconnect(false, false);
                if (g_wifi_abort_request) {
                    g_wifi_abort_request = false;
                    char abort_msg[80];
                    int lang = eez_flow_get_language_index();
                    if (lang == 1) snprintf(abort_msg, sizeof(abort_msg), "Connection canceled");
                    else if (lang == 2) snprintf(abort_msg, sizeof(abort_msg), "Baglanti iptal edildi");
                    else snprintf(abort_msg, sizeof(abort_msg), "اتصال توسط کاربر لغو شد");
                    set_var_connection_error_label(abort_msg);
                    ui_request_toast_colored(abort_msg, 0xff777777, 2500);
                } else {
                    char fail_msg[160];
                    int lang = eez_flow_get_language_index();
                    if (lang == 1) {
                        snprintf(fail_msg, sizeof(fail_msg), "Failed to connect to %s (check password/signal)", ssid.c_str());
                    } else if (lang == 2) {
                        snprintf(fail_msg, sizeof(fail_msg), "%s baglanti hatasi (sifre/sinyali kontrol edin)", ssid.c_str());
                    } else {
                        snprintf(fail_msg, sizeof(fail_msg), "خطا در اتصال به %s (رمز یا آنتن را بررسی کنید)", ssid.c_str());
                    }
                    set_var_connection_error_label(fail_msg);
                    ui_request_toast_colored(fail_msg, 0xffd32f2f, 4000);
                    wifi_ui_request_password_panel();
                }
                g_su_state = SU_IDLE;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// شروعِ بروزرسانیِ درجا — از نخِ LVGL صدا زده می‌شود؛ هیچ کارِ سنگینی اینجا انجام نمی‌دهد (فقط فلگ + تسک).
// پایان دادن به session بروزرسانی (دکمهٔ برگشتِ صفحه وای‌فا)
extern "C" void cancel_info_self_update(void)
{
    g_su_state = SU_IDLE;
    g_su_decline = true;             // خروج فوری از حلقهٔ انتظارِ self_update_check
    g_wifi_rescan_request = false;
    connectingToWifi = false;
    g_wifi_connect_in_progress = false;
    set_var_waitig_panle_state(false);
    g_wifi_direct_scan_active = false;
    isSearching = false;
    set_var_available_wifi_networks("");
    g_self_update_session = false;   // تسک خودش پاکسازی و وب‌اپ را برمی‌گرداند (نباید WiFi بلاک‌کننده روی ترد UI صدا زده شود)
    g_ota_boot_request = 0;
    set_var_ota_enable(false);
}

static void safe_write_off(int pin)
{
    if (pin != PIN_NC) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, OFF);
    }
}

static void restore_welding_ui_after_ota_exit(void);
static void exit_ota_boot_to_normal_tasks(void);
void webapp_task(void *pvParameters);
void app_task(void *param);

static bool spawn_webapp_task(void)
{
    if (webTaskHandle != NULL) {
        eTaskState st = eTaskGetState(webTaskHandle);
        if (st != eDeleted && st != eInvalid) return true;
        webTaskHandle = NULL;
    }
    BaseType_t ok = xTaskCreatePinnedToCore(webapp_task, "WebApp", 4096, NULL, 1, &webTaskHandle, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(webapp_task, "WebApp", 3072, NULL, 1, &webTaskHandle, 0);
    }
    if (ok != pdPASS) {
        webTaskHandle = NULL;
        Serial.println("[WEBAPP] WebApp task create failed (DRAM OOM)");
        return false;
    }
    return true;
}

static bool spawn_app_task(void)
{
    if (appTaskHandle != NULL) {
        eTaskState st = eTaskGetState(appTaskHandle);
        if (st != eDeleted && st != eInvalid) return true;
        appTaskHandle = NULL;
    }
    BaseType_t ok = xTaskCreatePinnedToCore(app_task, "App", 4096, NULL, 2, &appTaskHandle, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(app_task, "App", 3072, NULL, 2, &appTaskHandle, 0);
    }
    if (ok != pdPASS) {
        appTaskHandle = NULL;
        Serial.println("[APP] App task create failed (DRAM OOM)");
        return false;
    }
    return true;
}

// تفاوتِ کلیدی F5 vs منوی اطلاعات:
// F5: webapp/app ساخته نمی‌شوند → رم داخلی برای بافرهای WiFi آزاد است → اسکن OK.
// اطلاعات: webapp(12K)+app(6K)+SelfUpd(12K) هم‌زمان → اسکن با SCAN FAILED می‌خورد.
// پس در جلسهٔ بروزرسانیِ درجا همان کار F5 را می‌کنیم: webapp/app را موقتاً حذف و بعد از خروج برمی‌گردانیم.
static void su_pause_background_tasks_for_wifi(void)
{
    webAppEnabled = false;
    // آیتم: قبلاً WiFi.softAPdisconnect(true) بدون شرط اینجا بود و تا ۳ ثانیه بلاک می‌کرد.
    // حالا فقط اگر AP واقعاً بالا است آن را غیربلاکینگ می‌کشیم.
    if (webApp.isApUp()) {
        webApp.wStopAp();
        WiFi.softAPdisconnect(false);   // non-blocking — settle در vTaskDelay پایین
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    // تسک webapp_task به صورت همیارانه هنگام فعال بودن g_self_update_session یا wifi_screen_active
    // در تاخیر می‌ماند و با رادیو کاری ندارد؛ نیازی به حذف خشن با vTaskDelete نیست.
    Serial.println("[SU] webapp paused cooperatively");
    // app_task برای کنترل ایمن المنت و سنسورها فعال نگه داشته می‌شود
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void su_resume_background_tasks_after_wifi(void)
{
    if (get_var_ota_enable()) return;
    webAppEnabled = true;
    if (webTaskHandle == NULL) {
        if (spawn_webapp_task()) Serial.println("[SU] webapp_task restored");
    }
    if (appTaskHandle == NULL) {
        if (spawn_app_task()) Serial.println("[SU] app_task restored");
    }
}

static void restore_welding_ui_after_ota_exit(void)
{
    set_var_step_process(false);
    set_var_step_number(0);
    set_var_emergency_stop(false);
    g_force_lamps_off = false;
    set_var_cycel_started(true);
    set_var_progress_vlaue(0);
    set_var_josh_counter(0);
    set_var_cooling_counter(0);
    set_var_test_ssr(false);
    set_var_test_shablon(false);
    set_var_test_safhe(false);
    set_var_test_press1(false);
    set_var_test_press2(false);
    set_var_test_josh(false);
    set_var_test_zarbe1(false);
    set_var_test_zarbe2(false);
    set_var_safhe_out_display(false);
    set_var_press1_out_display(false);
    set_var_press2_out_display(false);
    set_var_josh_out_display(false);
    set_var_shablon_out_display(false);
    set_var_zarbe1_out_display(false);
    set_var_zarbe2_out_display(false);
}

// خروج از F5-بوت بدون ری‌استارت: ota_task را متوقف کن و app_task/webapp_task را بساز.
// مهم: روی نخ LVGL صبر نکن (vTaskDelay قبلی باعث هنگِ برگشت از وای‌فای می‌شد).
static void exit_ota_boot_to_normal_tasks(void)
{
    g_boot_f5_active = false;
    g_ota_exit_request = true;
    g_ota_go_framware_request = false;
    g_wifi_ota_scan_pending = false;
    g_wifi_ota_rescan_request = false;
    g_wifi_direct_scan_active = false;
    isSearching = false;
    set_var_download_panle_state(false);
    set_var_available_wifi_networks("");

    if (otaTaskHandle != NULL) {
        eTaskState st = eTaskGetState(otaTaskHandle);
        if (st != eDeleted && st != eInvalid) {
            // ota_task در حلقه g_ota_exit_request را می‌بیند و خودش را حذف می‌کند؛
            // اگر هنوز زنده است همین‌جا قطع کن — بدون انتظارِ ثانیه‌ای روی LVGL.
            vTaskDelete(otaTaskHandle);
        }
        otaTaskHandle = NULL;
    }
    g_ota_exit_request = false;

    if (webTaskHandle == NULL) {
        spawn_webapp_task();
    }
    if (appTaskHandle == NULL) {
        spawn_app_task();
    }

    set_var_setting_general_wifi(storage.getWifiOn());
    g_webapp_sync_restart = true;
    g_webapp_sync_requested = true;
    ota_return_cleanup_weld_state();
    restore_welding_ui_after_ota_exit();
}

extern "C" void safe_return_from_ota_to_menu(void)
{
    g_boot_f5_active = false;
    bool wasOtaBoot = get_var_ota_enable();
    cancel_info_self_update();
    g_output_inhibit_until_ms = millis() + 8000U;
    g_ota_return_sanitize_until_ms = millis() + 12000U;
    ota_return_cleanup_weld_state();
    reset_panel_button_latches();
    set_var_emergency_stop(false);
    set_var_current_back_button_status(false);
    set_var_back_button_pressed(false);
    set_var_curent_pedal_status(false);
    set_var_waitig_panle_state(false);
    set_var_test_mode(false);
    set_var_step_process(false);
    set_var_step_number(0);
    set_var_josh_is_working(false);
    set_var_cool_is_working(false);
    // false نباشد: پنل قرمز استپ مربوط به !cycel_started است؛ حالت آماده = true
    set_var_cycel_started(true);
    safe_write_off(SSR_1);
    safe_write_off(TABLE);
    safe_write_off(PRESS_LEFT);
    safe_write_off(PRESS_RIGHT);
    safe_write_off(ELEMENT);
    safe_write_off(SHABLON);
    safe_write_off(ZARBEH1);
    safe_write_off(ZARBEH2);
    // بعد از پاک‌سازی لچ‌ها false شده‌اند؛ دوباره true کن تا STOP/STARTِ هنوز پایین
    // (یا لبهٔ کاذب بعد از resetState فیلتر ADC) بلافاصله امرجنسی/جوش نزند — باید اول رها شوند.
    emergency_push_button = true;
    start_push_button = true;
    if (wasOtaBoot) {
        exit_ota_boot_to_normal_tasks();
        // exit هم دوباره cleanup می‌زند — لچ را دوباره قفل کن
        emergency_push_button = true;
        start_push_button = true;
        return;
    }
    // بروزرسانیِ درجا از صفحهٔ اطلاعات — app_task زنده است؛ UI آمادهٔ جوش را برگردان
    restore_welding_ui_after_ota_exit();
}

void agent_wifi_test(Stream *port, const char *ssid, const char *pass)
{
    if (!port) return;
    String target_s = (ssid && ssid[0]) ? String(ssid) : storage.getWifiSsid();
    String target_p = (pass && pass[0]) ? String(pass) : storage.getWifiPassword();
    if (target_s.length() == 0 && storage.wifi_count > 0) {
        target_s = storage.wifi_ssids[0];
        target_p = storage.wifi_pwds[0];
    }
    if (target_p.length() == 0 && (target_s.indexOf("HAFFLER") >= 0 || target_s.indexOf("Haffler") >= 0)) {
        target_p = "HAFFLER12";
    }

    port->printf("[WIFI-TEST] Starting connection to SSID='%s' passLen=%d ...\n",
                 target_s.c_str(), (int)target_p.length());

    su_pause_background_tasks_for_wifi();

    bool ok = wifi_sta_connect_full(target_s.c_str(), target_p.c_str(), -1, 30, NULL);
    if (ok && WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        int rssi = WiFi.RSSI();
        storage.saveWifi(target_s, target_p);
        storage.setWifiCredentials(target_s, target_p);
        storage.setWifiOn(true);
        port->printf("OK WIFI_CONNECTED IP=%s RSSI=%d\n", ip.c_str(), rssi);

        WiFiClient client;
        HTTPClient http;
        String otaUrl = ota_url("upvc/welding-single-head", "lastfile");
        port->printf("[WIFI-TEST] Querying OTA server: %s\n", otaUrl.c_str());
        if (http.begin(client, otaUrl)) {
            http.setTimeout(8000);
            int code = http.GET();
            String body = (code == 200) ? http.getString() : "";
            port->printf("OK SERVER_CHECK HTTP=%d BODY_LEN=%d BODY_PREVIEW='%s'\n",
                         code, (int)body.length(), body.substring(0, 120).c_str());
            http.end();
        } else {
            port->println("ERR SERVER_BEGIN_FAILED");
        }
    } else {
        port->printf("ERR WIFI_CONNECT_FAILED STATUS=%d REASON=%d\n",
                     (int)WiFi.status(), g_wifi_last_disconnect_reason);
    }
    su_resume_background_tasks_after_wifi();
}

void agent_cmd_wifi_get(Stream *port)
{
    if (!port) return;
    port->printf("OK WIFI_PRIMARY SSID='%s' PASS='%s'\n", storage.getWifiSsid().c_str(), storage.getWifiPassword().c_str());
    port->printf("OK WIFI_COUNT=%d\n", storage.wifi_count);
    for (int i = 0; i < storage.wifi_count; i++) {
        port->printf("OK WIFI[%d] SSID='%s' PASS='%s'\n", i, storage.wifi_ssids[i].c_str(), storage.wifi_pwds[i].c_str());
    }
}

void agent_cmd_wifi_set(Stream *port, const char *ssid, const char *pass)
{
    if (!port) return;
    String s = ssid ? ssid : "";
    String p = pass ? pass : "";
    storage.saveWifi(s, p);
    storage.setWifiCredentials(s, p);
    storage.setWifiOn(true);
    port->printf("OK WIFI_SAVED SSID='%s' PASS='%s'\n", s.c_str(), p.c_str());
}

void agent_cmd_do_update(Stream *port)
{
    if (!port) return;
    extern void installer_prepare_install(int idx);
    extern void start_install_model(const char *model);
    installer_prepare_install(0);
    start_install_model("upvc%2Fwelding-single-head");
    port->println("OK UPDATE_INITIATED");
}

void agent_cmd_reset_onboarding(Stream *port)
{
    if (!port) return;
    Preferences p;
    p.begin("AJOSH1", false);
    p.remove("firstBoot");
    p.remove("mfg_name");
    p.remove("mfg_phone");
    p.remove("mfg_otp_ok");
    p.remove("mfg_done");
    p.remove("cust_name");
    p.remove("cust_phone");
    p.remove("cust_city");
    p.remove("warr_act");
    p.remove("warr_sms_ok");
    p.end();
    port->println("OK ONBOARDING_CLEARED Restarting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP.restart();
}

static bool spawn_self_update_task(void)
{
    if (selfUpdateTaskHandle != NULL) {
        eTaskState st = eTaskGetState(selfUpdateTaskHandle);
        if (st != eDeleted && st != eInvalid) return true;
        selfUpdateTaskHandle = NULL;
    }
    webAppEnabled = false;

    BaseType_t ok = xTaskCreatePinnedToCore(self_update_task, "SelfUpd", 8192, NULL, 4, &selfUpdateTaskHandle, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(self_update_task, "SelfUpd", 6144, NULL, 4, &selfUpdateTaskHandle, 0);
    }
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(self_update_task, "SelfUpd", 4096, NULL, 4, &selfUpdateTaskHandle, 0);
    }
    if (ok != pdPASS) {
        selfUpdateTaskHandle = NULL;
        g_wifi_direct_scan_active = false;
        isSearching = false;
        set_var_available_wifi_networks("SCAN FAILED");
        Serial.println("[SU] SelfUpd task create failed");
        return false;
    }
    return true;
}

extern "C" void self_update_ensure_task(void)
{
    if (!g_self_update_session) return;
    spawn_self_update_task();
}

extern "C" bool self_update_is_task_alive(void)
{
    if (selfUpdateTaskHandle == NULL) return false;
    eTaskState st = eTaskGetState(selfUpdateTaskHandle);
    return (st != eDeleted && st != eInvalid);
}

extern "C" void start_info_self_update(void)
{
    if (get_var_ota_enable()) {
        g_self_update_session = false;
        return;
    }
    if (selfUpdateTaskHandle != NULL) {
        eTaskState st = eTaskGetState(selfUpdateTaskHandle);
        if (st == eDeleted || st == eInvalid) {
            selfUpdateTaskHandle = NULL;
        } else {
            // تسک زنده است — فعال‌سازی مجدد و درخواست اسکن سریع بدون ایجاد تسک تداخلی
            g_self_update_pending_restart = false;
            g_self_update_session = true;
            g_wifi_rescan_request = true;
            wifi_publish_networks("SEARCHING");
            return;
        }
    }
    g_self_update_pending_restart = false;
    g_webapp_sync_requested = false;
    g_webapp_sync_restart = false;
    cancel_wifi_scan_starter_state();
    set_var_waitig_panle_state(false);
    g_su_confirm = false;
    g_su_decline = false;
    g_su_version[0] = '\0';
    g_su_state = SU_IDLE;
    g_self_update_session = true;
    wifi_names = "";
    wifi_publish_networks("SEARCHING");
    if (!spawn_self_update_task()) {
        g_self_update_session = false;
        wifi_publish_networks("SCAN FAILED");
    }
}
static void self_update_pending_restart_timer_cb(lv_timer_t *t) {
    (void)t;
    if (g_self_update_pending_restart && selfUpdateTaskHandle == NULL) {
        g_self_update_pending_restart = false;
        start_info_self_update();
    }
}

static void ota_queue_download_model(const char *model)
{
    if (!model) model = "";
    strncpy(ota_download_model, model, sizeof(ota_download_model) - 1);
    ota_download_model[sizeof(ota_download_model) - 1] = '\0';
    ota_download = true;
}

void action_btn_corner_cleaner_install(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(2);
    start_install_model("upvc%2Fcorner-cleaning");
}
void action_btn_chane_gir_install(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(3);
    start_install_model("upvc%2Fdigital-cutter-single-head");
}

void action_btn_single_head_install(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(0);
    start_install_model("upvc%2Fwelding-single-head");
}

void action_btn_dubble_head_install(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(1);
    start_install_model("upvc%2Fwelding-double-head");
}

void action_btn_update_frameware(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(0);
    start_install_model("upvc%2Fwelding-single-head");
}

void action_btn_install_package1(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(4);
    start_install_model("upvc%2Fdigital-cutter-double-head");
}
void action_btn_install_test_app(lv_event_t *e)
{
    (void)e;
    installer_prepare_install(1);
    start_install_model("upvc%2Fconfig-test");
}

extern "C" bool webapp_is_client_connected(void)
{
    // فقط در صورتی که گوشی/کلاینت واقعاً به وای‌فای دستگاه متصل باشد (تعداد کلاینت‌ها > ۰)
    return (webApp.isApUp() && WiFi.softAPgetStationNum() > 0);
}

extern "C" void webapp_disconnect_for_update(void)
{
    webAppEnabled = false;
    webApp.wStopAp();
    WiFi.softAPdisconnect(true);
}

// نصبِ عمومیِ هر محصول (برای منوی مدیر) — مدل کامل مثل "upvc%2Fwelding-single-head" یا آدرس مستقیم URL
static TaskHandle_t g_installDownloadTask = NULL;
static String g_installDownloadModel;
static bool g_installIsDirectUrl = false;
static String g_installDirectUrl;
static void install_download_task(void *param)
{
    (void)param;
    su_pause_background_tasks_for_wifi();
    WiFi.scanDelete();
    if (g_inst_scan_task_handle != NULL) {
        uint32_t t0 = millis();
        while (g_inst_scan_task_handle != NULL && (millis() - t0 < 3000)) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    if (g_installIsDirectUrl) {
        download_firmware_from_url(g_installDirectUrl);
    } else {
        download_firmware(g_installDownloadModel);   // در موفقیت خودش ESP.restart می‌کند
    }
    su_resume_background_tasks_after_wifi();
    g_installDownloadTask = NULL;
    vTaskDelete(NULL);
}
extern "C" void start_install_model(const char *model)
{
    if (!model || !model[0]) return;
    Serial.printf("[INSTALL] start_install_model: %s\n", model);

    String m = String(model);
    m.trim();
    while (m.startsWith("/")) m.remove(0, 1);
    if (m.startsWith("plc_hmi/")) m.remove(0, 8);

    set_var_download_panle_state(true);
    set_var_download_lable_info_text(ML("درحال بررسی نسخه ...", "Checking version...", "Surum kontrol ediliyor..."));
    ota_ui_notify(ML("در حال بررسی ...", "Checking...", "Kontrol ediliyor..."));

    // F5: ota_task زنده است
    if (otaTaskHandle != NULL) {
        eTaskState st = eTaskGetState(otaTaskHandle);
        if (st != eDeleted && st != eInvalid) {
            ota_queue_download_model(m.c_str());
            Serial.println("[INSTALL] ota_download=1 queued");
            return;
        }
    }

    g_installIsDirectUrl = false;
    g_installDownloadModel = m;
    if (g_installDownloadTask == NULL) {
        BaseType_t ok = xTaskCreatePinnedToCore(install_download_task, "InstDL", 8192, NULL, 3, &g_installDownloadTask, 0);
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(install_download_task, "InstDL", 6144, NULL, 3, &g_installDownloadTask, 0);
        }
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(install_download_task, "InstDL", 4096, NULL, 3, &g_installDownloadTask, 0);
        }
        if (ok != pdPASS) {
            g_installDownloadTask = NULL;
            su_resume_background_tasks_after_wifi();
            Serial.println("[INSTALL] download task create failed");
            ota_ui_notify(ML("خطا - حافظه کافی برای نصب نیست",
                             "Error: not enough memory to install",
                             "Hata: kurulum icin bellek yetersiz"));
            // بستن خودکار پنل دانلود پس از ۴ ثادیه
            lv_timer_t *t = lv_timer_create([](lv_timer_t *timer){
                set_var_download_panle_state(false);
                lv_timer_del(timer);
            }, 4000, NULL);
            (void)t;
        }
    }
}

extern "C" void start_install_url(const char *url)
{
    if (!url || !url[0]) return;
    Serial.printf("[INSTALL] start_install_url: %s\n", url);
    strncpy(ota_custom_url_buf, url, sizeof(ota_custom_url_buf) - 1);
    ota_custom_url_buf[sizeof(ota_custom_url_buf) - 1] = '\0';
    set_var_download_panle_state(true);
    set_var_download_lable_info_text(url);
    ota_ui_notify(ML("در حال دانلود...", "Downloading...", "Indiriliyor..."));

    // F5: ota_task زنده است
    if (otaTaskHandle != NULL) {
        eTaskState st = eTaskGetState(otaTaskHandle);
        if (st != eDeleted && st != eInvalid) {
            ota_custom_download = true;
            Serial.println("[INSTALL] ota_custom_download=1 queued for F5 task");
            return;
        }
    }

    // حالت عادی (اجرا از صفحه بروزرسانی یا ورود لینک مستقیم)
    g_installIsDirectUrl = true;
    g_installDirectUrl = String(url);
    if (g_installDownloadTask == NULL) {
        BaseType_t ok = xTaskCreatePinnedToCore(install_download_task, "InstDL", 8192, NULL, 3, &g_installDownloadTask, 0);
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(install_download_task, "InstDL", 6144, NULL, 3, &g_installDownloadTask, 0);
        }
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(install_download_task, "InstDL", 4096, NULL, 3, &g_installDownloadTask, 0);
        }
        if (ok != pdPASS) {
            g_installDownloadTask = NULL;
            su_resume_background_tasks_after_wifi();
            Serial.println("[INSTALL] download url task create failed");
            ota_ui_notify(ML("خطا - حافظه کافی برای نصب نیست",
                             "Error: not enough memory to install",
                             "Hata: kurulum icin bellek yetersiz"));
            lv_timer_t *t = lv_timer_create([](lv_timer_t *timer){
                set_var_download_panle_state(false);
                lv_timer_del(timer);
            }, 4000, NULL);
            (void)t;
        }
    }
}

void action_admin_password_redy(lv_event_t *e)
{
    // خواندن مستقیم از textarea برای اطمینان از دریافت آخرین مقدار وارد‌شده
    String pass = "";
    if (objects.text_area_framware_password)
        pass = String(lv_textarea_get_text(objects.text_area_framware_password));
    else
        pass = String(get_var_admin_password());
    pass.trim();
    // رمز مادر (ادمین/مستر): همه‌جا که رمز خواسته می‌شود باز می‌کند — آیتم ۷ لیست اصلاحات.
    unsigned long nowEpoch = (unsigned long)rtc_now_s().unixtime();

    // رمز مادر (مستر ۱۲۷۷۳): همیشه باز می‌کند و قفل را پاک می‌کند
    if (pass == MASTER_ADMIN_PASSWORD)
    {
        admin_entry_wrong_count = 0;
        storage.clearAdminLock();
        // فقط اگر دستگاه واقعاً قفل (منقضی) است قفل/شمارش پاک شود؛ وگرنه ورود به تنظیمات
        // نباید تایمرِ درحال‌شمارش را از بین ببرد (باگ: عددِ اسلایدر بعد از ورود صفر می‌شد).
        {
            long today = today_jdn_from_rtc();
            if (today != 0 && storage.isLockedNow(today)) {
                storage.clearLockAndCountdown();
                set_var_setting_admin_countdown_timer(0);
            }
        }
        set_var_wrong_password_atemp("");
        set_var_is_admin_password_verifyed(true);
        return;
    }

    // اگر قفلِ ۱۰ دقیقه‌ای فعال است، جز مستر هیچ رمزی پذیرفته نمی‌شود
    if (storage.isAdminLocked(nowEpoch))
    {
        show_admin_lock_message(storage.adminLockRemainingSec(nowEpoch));
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        return;
    }

    if (pass == storage.getAdminPassword())
    {
        admin_entry_wrong_count = 0;
        storage.clearAdminLock();
        // فقط اگر دستگاه واقعاً قفل (منقضی) است قفل/شمارش پاک شود؛ ورودِ عادی به تنظیمات
        // نباید تایمرِ درحال‌شمارش را پاک کند.
        {
            long today = today_jdn_from_rtc();
            if (today != 0 && storage.isLockedNow(today)) {
                storage.clearLockAndCountdown();
                set_var_setting_admin_countdown_timer(0);
            }
        }
        set_var_wrong_password_atemp("");
        set_var_is_admin_password_verifyed(true);
    }
    else
    {
        // رمز اشتباه — شمارش؛ بعد از ۳ بار، قفلِ ۱۰ دقیقه‌ای
        admin_entry_wrong_count++;
        if (admin_entry_wrong_count >= 3)
        {
            storage.lockAdminFor(nowEpoch, 10UL * 60UL);
            admin_entry_wrong_count = 0;
            show_admin_lock_message(storage.adminLockRemainingSec(nowEpoch));
        }
        else
        {
            int left = 3 - admin_entry_wrong_count;
            static char wbuf[80];
            snprintf(wbuf, sizeof(wbuf),
                     ML("رمز اشتباه — %d تلاش دیگر", "Wrong password — %d attempts left", "Yanlis sifre — %d deneme kaldi"), left);
            set_var_wrong_password_atemp(wbuf);
        }
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
    }
}
// آیتم: صفحه‌های وای‌فای/کارت‌های نصب/بروزرسانی — تا وقتی کاربر روی این صفحات است، وب‌اپ/AP نباید دوباره روشن شود.
static bool wifi_screen_active(void) {
    int16_t s = eez_flow_get_current_screen();
    return s == SCREEN_ID_SCREEN_WIFI || s == SCREEN_ID_SCREEN_FRAMWARE ||
           s == SCREEN_ID_SCREEN_UPDATE; // FIX-WEBAPP-AP
}
void webapp_task(void *pvParameters)
{
    while (true)
    {
        // در مدِ بروزرسانی، self_update_task مالکِ رادیوست (STAِ خالص)؛ وب‌اپ/AP اینجا اجرا نشود تا اسکن خراب نشود.
        // همین‌طور وقتی کاربر مستقیم (بدونِ self-update-session) روی صفحهٔ وای‌فای/نصب است.
        if (g_self_update_session || wifi_screen_active())
        {
            vTaskDelay(pdMS_TO_TICKS(runtime_policy::WEBAPP_LOOP_DELAY_MS));
            continue;
        }
        // هم‌گام‌سازیِ بلاک‌کنندهٔ وب‌اپ/WiFi این‌جا (هستهٔ ۰) انجام می‌شود، نه روی ترد UI.
        if (g_webapp_sync_requested)
        {
            g_webapp_sync_requested = false;
            sync_webapp_state(g_webapp_sync_restart);
        }
        if (webAppEnabled)
        {
            // اول apply، بعد Loop/handshake — وگرنه SET_LANG با handshake زودهنگام
            // دوباره از PLC زبانِ قبلی (فارسی) را روی UI می‌کشد.
            if (update_data) {
                update_data = false;
                webapp_apply_vars_to_plc();
                webapp_sync_vars_from_plc();
                webApp.reqHandShake();
            }
            webApp.Loop();
        }

        vTaskDelay(pdMS_TO_TICKS(runtime_policy::WEBAPP_LOOP_DELAY_MS));
    }
}
// محاسبهٔ وضعیت قفل/روزهای مانده از روی storage + RTC (آیتم ۴/۸).
static void update_lock_state()
{
    // مهم: اگر روزشمار غیرفعال است (پاک شده/تنظیم‌نشده) دستگاه قطعاً باز است — مستقل از وضعیتِ RTC.
    // قبلاً وقتی RTC نامعتبر بود (today==0)، بعدِ پاک‌کردنِ رمز، g_device_locked صفر نمی‌شد و قفل می‌ماند.
    if (!storage.countdownActive()) {
        g_lock_remaining_days = -1;
        g_device_locked = 0;
        return;
    }
    long today = today_jdn_from_rtc();
    if (today == 0) {
        // روزشمار فعال ولی RTC نامعتبر → محتاطانه قفل
        g_lock_remaining_days = 0;
        g_device_locked = 1;
        return;
    }
    // ضد دستکاری باتری و ساعت: اگر باتری RTC برداشته شده یا برقش قطع شده بود
    if (g_rtc_lost_power_boot && storage.countdownActive()) {
        g_lock_remaining_days = 0;
        g_device_locked = 1;
        return;
    }
    if (!storage.noteRtcDay(today)) {
        g_lock_remaining_days = 0;
        g_device_locked = 1;
        return;
    }
    g_lock_remaining_days = storage.remainingDays(today);
    g_device_locked = storage.isLockedNow(today) ? 1 : 0;
    static int dbg_last = -999;
    if (g_lock_remaining_days != dbg_last) {
        dbg_last = g_lock_remaining_days;
        Serial.printf("[LOCK] today=%ld endJDN=%ld remain=%d locked=%d lostPwr=%d\n",
                      today, storage.getCountdownEndJDN(), g_lock_remaining_days,
                      g_device_locked, (int)g_rtc_lost_power_boot);
    }
}

void lock_task(void *param)
{
    static uint32_t s_sec_count = 0;
    while (true)
    {
        s_sec_count++;
        warranty_tick_1s();
        update_lock_state();
        boot_guard_check_steady_state(); // پس از ۲۵ ثانیه آپتایم، فریمور را تأیید می‌کند
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ===== Overlay مودالِ قفل (آیتم ۲/۴): قفل واقعی روی هر صفحه + کیبرد عددی + جریمهٔ ۳ بار اشتباه =====
static lv_obj_t *lock_overlay = NULL;
static lv_obj_t *lock_overlay_ta = NULL;
static lv_obj_t *lock_overlay_kb = NULL;
static lv_obj_t *lock_overlay_info = NULL;
static lv_obj_t *lock_random_label = NULL;
static int lock_random_code = 0;
static int lock_wrong_attempts = 0;
static uint32_t lock_cooldown_until_ms = 0; // millis تا پایانِ جریمهٔ ۱۰ دقیقه‌ای
static uint32_t lock_info_hold_until_ms = 0; // پیامِ موقت (اشتباه/مصرف‌شده) تا این زمان بازنویسی نشود
#define LOCK_INFO_WRONG_HOLD_MS 4000U

static int mt_digit_sum_u(unsigned long n)
{
    int s = 0;
    while (n) { s += (int)(n % 10); n /= 10; }
    return s;
}

static void mt_make_timer_password(char *out, size_t sz, const char *adminPass, int timerIndex)
{
    // فرمول: رمز مدیر × (شماره تایمر + ۱) + یکان حاصل جمع ارقام
    // تایمر ۱: ضربدر ۲ | تایمر ۲: ضربدر ۳ | تایمر ۳: ضربدر ۴ | ...
    unsigned long mult = (timerIndex >= 1) ? ((unsigned long)timerIndex + 1UL) : 2UL;
    unsigned long base = strtoul(adminPass, NULL, 10) * mult;
    snprintf(out, sz, "%lu%d", base, mt_digit_sum_u(base) % 10);
}

static void mt_make_random_password(char *out, size_t sz, int randomCode)
{
    unsigned long base = (unsigned long)randomCode * 2UL;
    snprintf(out, sz, "%lu%d", base, mt_digit_sum_u(base) % 10);
}

static int mt_find_matching_other_stage(const String &p, const char *adminPass, int activeIdx)
{
    for (int t = 1; t <= 5; t++) {
        if (t == activeIdx) continue;
        char pwd[24];
        mt_make_timer_password(pwd, sizeof(pwd), adminPass, t);
        if (p == pwd) return t;
    }
    return 0;
}

static void lock_register_wrong()
{
    lock_wrong_attempts++;
    if (lock_wrong_attempts >= 3)
    {
        DateTime now = rtc_now_s();
        if (now.year() >= 2020) {
            storage.lockAdminFor((unsigned long)now.unixtime(), 5UL * 60UL);
        } else {
            lock_cooldown_until_ms = millis() + 5UL * 60UL * 1000UL;
        }
        lock_wrong_attempts = 0;
    }
}

// نمایشِ پیام روی لیبلِ اطلاعاتِ قفل (سبز=درست، قرمز=اشتباه). hold_ms>0 → تا آن مدت تایمرِ پیش‌فرض بازنویسی نمی‌کند.
static void lock_set_info(const char *msg, uint32_t color, uint32_t hold_ms = 0)
{
    if (!lock_overlay_info || !msg) return;
    const char *cur = lv_label_get_text(lock_overlay_info);
    if (!cur || strcmp(cur, msg) != 0) lv_label_set_text(lock_overlay_info, msg);
    lv_obj_set_style_text_color(lock_overlay_info, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (hold_ms > 0) lock_info_hold_until_ms = millis() + hold_ms;
}

static bool mt_try_stage_password(const String &p, long today)
{
    int tidx = (int)storage.getActiveTimerIndex();
    if (tidx < 1 || tidx > 5 || today == 0 || !storage.isLockedNow(today)) return false;
    char epwd[24];
    String adminPass = storage.getAdminPassword();
    mt_make_timer_password(epwd, sizeof(epwd), adminPass.c_str(), tidx);
    if (p == epwd) return true;
    char rpwd[24];
    mt_make_random_password(rpwd, sizeof(rpwd), lock_random_code);
    if (p == rpwd) return true;
    // پذیرش رمزهای ۵ رقمی ناشی از کد رندم (حالت مستقیم بدون چک‌سام یا ۵ رقمی)
    // ۱) ضربدر ۲ مستقیم (مثل ۱۷۲۲۱ × ۲ = ۳۴۴۴۲)
    char rpwd_direct[24];
    snprintf(rpwd_direct, sizeof(rpwd_direct), "%lu", (unsigned long)lock_random_code * 2UL);
    if (p == rpwd_direct) return true;
    // ۲) پنج رقمی مدولو ۱۰۰۰۰۰ (در صورتی که عدد بیش از ۵ رقم باشد)
    char rpwd_5d[24];
    snprintf(rpwd_5d, sizeof(rpwd_5d), "%05lu", ((unsigned long)lock_random_code * 2UL) % 100000UL);
    if (p == rpwd_5d) return true;
    // ۳) خود عدد استعلام ۵ رقمی
    char rcode_str[24];
    snprintf(rcode_str, sizeof(rcode_str), "%05d", lock_random_code);
    if (p == rcode_str) return true;
    return false;
}

static void mt_advance_after_stage_unlock(long today)
{
    storage.advanceTimerChain(today);
    lock_wrong_attempts = 0;
    lock_cooldown_until_ms = 0;
    update_lock_state();
    int rem = g_lock_remaining_days;
    if (rem > 0) set_var_setting_admin_countdown_timer(rem);
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    if (g_device_locked)
        lock_set_info(ML("مرحله تایید شد — شروع زمان مرحله بعد", "Unlocked — next stage started", "Acildi — sonraki asama"), 0xff2ec16b, 2000);
    else
        lock_set_info(ML("همه تایمرها پایان یافت — قفل باز شد", "All stages complete — unlocked", "Tum asamalar tamam"), 0xff2ec16b, 2000);
}

static void lock_label_set_if_changed(lv_obj_t *obj, const char *txt)
{
    if (!obj || !txt) return;
    const char *cur = lv_label_get_text(obj);
    if (cur && strcmp(cur, txt) == 0) return;
    lv_label_set_text(obj, txt);
}

// منطقِ بازکردنِ قفل از روی رمزِ واردشده (مستر/مدیر/موقت + جریمه). از کیبردِ سفارشی صدا زده می‌شود.
static void lock_do_unlock_attempt(const char *raw)
{
    if (!lock_overlay_ta) return;
    String p = String(raw ? raw : "");
    p.trim();
    long today = today_jdn_from_rtc();
    DateTime nowRtc = rtc_now_s();
    unsigned long nowEpoch = (nowRtc.year() >= 2020) ? (unsigned long)nowRtc.unixtime() : 0;

    // رمز مستر (سازنده) یا رمز مدیر: تمام تایمرها به طور کامل پاک و غیرفعال می‌شوند
    if (p == MASTER_ADMIN_PASSWORD)
    {
        storage.clearLockAndCountdown();
        storage.clearAdminLock();
        set_var_setting_admin_countdown_timer(0);
        lock_wrong_attempts = 0;
        lock_cooldown_until_ms = 0;
        update_lock_state();
        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
        lock_set_info(ML("دستگاه باز شد — تمام تایمرها غیرفعال شد", "Unlocked — all timers cleared", "Acildi — tum zamanlayicilar sifirlandi"), 0xff2ec16b);
        lv_textarea_set_text(lock_overlay_ta, "");
        return;
    }
    // در دورهٔ جریمهٔ ۵ دقیقه‌ای، رمز مدیر/موقت پذیرفته نمی‌شود (محافظت brute-force).
    if ((nowEpoch != 0 && storage.isAdminLocked(nowEpoch)) || millis() < lock_cooldown_until_ms)
    {
        lv_textarea_set_text(lock_overlay_ta, "");
        lock_set_info(ML("۵ دقیقه قفل امنیتی فعال است", "Wait 5 minutes", "5 dakika bekleyin"), 0xfff21c1c);
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        return;
    }
    if (p == storage.getAdminPassword())
    {
        storage.clearLockAndCountdown();
        storage.clearAdminLock();
        set_var_setting_admin_countdown_timer(0);
        lock_wrong_attempts = 0;
        lock_cooldown_until_ms = 0;
        update_lock_state();
        xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
        lock_set_info(ML("دستگاه باز شد — تمام تایمرها غیرفعال شد", "Unlocked — all timers cleared", "Acildi — tum zamanlayicilar sifirlandi"), 0xff2ec16b);
    }
    else if (mt_try_stage_password(p, today))
    {
        mt_advance_after_stage_unlock(today);
    }
    else if (p == TEMP_UNLOCK_PASSWORD)
    {
        if (today != 0 && !storage.getTempUnlockUsed() && storage.useTempUnlock(today))
        {
            lock_wrong_attempts = 0;
            set_var_setting_admin_countdown_timer(10);
            update_lock_state();
            xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
            lock_set_info(ML("مهلت اضطراری ۱۰ روزه فعال شد (فقط ۱ بار)", "10-day emergency grace activated", "10 gunluk acil sure aktif"), 0xff2ec16b, 2000);
        }
        else if (storage.getTempUnlockUsed())
        {
            lock_set_info(ML("این رمز قبلاً یک‌بار استفاده شده است", "Temporary password already used", "Gecici sifre daha once kullanildi"), 0xfff21c1c, LOCK_INFO_WRONG_HOLD_MS);
            xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        }
        else
        {
            lock_register_wrong();
            char m[48];
            snprintf(m, sizeof(m), "%s (%d/3)", ML("رمز اشتباه است", "Wrong password", "Yanlis sifre"), lock_wrong_attempts);
            lock_set_info(m, 0xfff21c1c, LOCK_INFO_WRONG_HOLD_MS);
            xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
        }
    }
    else
    {
        int tidx = (int)storage.getActiveTimerIndex();
        String adminPass = storage.getAdminPassword();
        int otherT = mt_find_matching_other_stage(p, adminPass.c_str(), tidx);
        lock_register_wrong();
        char m[64];
        if (otherT > 0) {
            snprintf(m, sizeof(m), ML("رمز تایمر %d است (رمز تایمر %d را بزنید)", "Password is for timer %d", "Zamanlayici %d sifresi"), otherT, tidx);
        } else {
            snprintf(m, sizeof(m), "%s (%d/3)", ML("رمز اشتباه است", "Wrong password", "Yanlis sifre"), lock_wrong_attempts);
        }
        lock_set_info(m, 0xfff21c1c, LOCK_INFO_WRONG_HOLD_MS);
        xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite);
    }
    lv_textarea_set_text(lock_overlay_ta, "");
}

// اعمالِ یک دکمهٔ کیبردِ قفل (عدد/پاک/تایید) — هم از تاچ هم از ناوبریِ MOD فراخوانی می‌شود.
static void lock_kb_apply_button(const char *txt)
{
    if (!txt || !lock_overlay_ta) return;
    g_last_activity_ms = millis();
    if (strcmp(txt,"تایید")==0 || strcmp(txt,"OK")==0 || strcmp(txt,"Tamam")==0 || strcmp(txt, LV_SYMBOL_OK)==0) {
        lock_do_unlock_attempt(lv_textarea_get_text(lock_overlay_ta));
        return;
    }
    if (strcmp(txt,"پاک")==0 || strcmp(txt,"Del")==0 || strcmp(txt,"Sil")==0 || strcmp(txt, LV_SYMBOL_BACKSPACE)==0) {
        lv_textarea_del_char(lock_overlay_ta);
    } else {
        const char *cur = lv_textarea_get_text(lock_overlay_ta);
        size_t len = cur ? strlen(cur) : 0;
        if (len < 12) lv_textarea_add_text(lock_overlay_ta, txt);
    }
    lv_textarea_set_cursor_pos(lock_overlay_ta, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_scroll_to(lock_overlay_ta, 0, 0, LV_ANIM_OFF);
}

extern "C" void keyboard_color_draw_task_cb(lv_event_t *e);

// نقشهٔ کیبردِ سفارشیِ قفل (مثلِ کیبردِ مدیر): اعداد + پاک (قرمز) + تایید (سبز)
static const char *lock_kb_map_fa[] = {
    "1","2","3","\n", "4","5","6","\n", "7","8","9","\n", LV_SYMBOL_BACKSPACE,"0",LV_SYMBOL_OK,"" };
static const char *lock_kb_map_en[] = {
    "1","2","3","\n", "4","5","6","\n", "7","8","9","\n", LV_SYMBOL_BACKSPACE,"0",LV_SYMBOL_OK,"" };
static const char *lock_kb_map_tr[] = {
    "1","2","3","\n", "4","5","6","\n", "7","8","9","\n", LV_SYMBOL_BACKSPACE,"0",LV_SYMBOL_OK,"" };
static const lv_btnmatrix_ctrl_t lock_kb_ctrl[] = {
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT),
    (lv_btnmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_WIDTH_1 | LV_BUTTONMATRIX_CTRL_NO_REPEAT) };

// هندلرِ کیبردِ سفارشی: دکمه‌ها در VALUE_CHANGED مدیریت می‌شوند (مثلِ کیبردِ مدیر که مطمئن و روان کار می‌کند).
static void lock_kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (!lock_overlay_ta) return;
    if (code != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *kb = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(kb);
    if (id == LV_BTNMATRIX_BTN_NONE) return;
    const char *txt = lv_btnmatrix_get_btn_text(kb, id);
    if (!txt) return;
    static uint32_t last_lock_key_tick = 0;
    static uint16_t last_lock_btn_id = 0xFFFF;
    uint32_t now = lv_tick_get();
    if (id == last_lock_btn_id && (now - last_lock_key_tick) < 90) {
        return;
    }
    last_lock_key_tick = now;
    last_lock_btn_id = id;
    lock_kb_apply_button(txt);
}

// ناوبریِ کیبردِ قفل با کلیدهای سخت‌افزاری (در حالتِ قفل، از keypad_task فراخوانی می‌شوند).
extern "C" bool lock_is_active(void) { return lock_overlay != NULL; }
extern "C" void lock_nav_move(void)   // MOD: حرکت به دکمهٔ بعدی
{
    if (!lock_overlay_kb) return;
    uint16_t id = lv_btnmatrix_get_selected_btn(lock_overlay_kb);
    uint32_t k = LV_KEY_RIGHT;
    if (id == LV_BTNMATRIX_BTN_NONE) lv_btnmatrix_set_selected_btn(lock_overlay_kb, 0);
    else lv_event_send(lock_overlay_kb, LV_EVENT_KEY, &k);
    lv_obj_invalidate(lock_overlay_kb);
}
extern "C" void lock_nav_select(void) // START: زدنِ دکمهٔ انتخاب‌شده
{
    if (!lock_overlay_kb) return;
    uint16_t id = lv_btnmatrix_get_selected_btn(lock_overlay_kb);
    if (id == LV_BTNMATRIX_BTN_NONE) return;
    lock_kb_apply_button(lv_btnmatrix_get_btn_text(lock_overlay_kb, id));
}
extern "C" void lock_nav_delete(void) // STOP: پاک‌کردنِ یک رقم
{
    if (lock_overlay_ta) lv_textarea_del_char(lock_overlay_ta);
}

// چشمِ نمایش/مخفیِ رمزِ قفل
static lv_obj_t *lock_eye_label = NULL;
static void lock_eye_toggle_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || !lock_overlay_ta) return;
    bool cur = lv_textarea_get_password_mode(lock_overlay_ta);
    lv_textarea_set_password_mode(lock_overlay_ta, !cur);
    if (lock_eye_label) lv_label_set_text(lock_eye_label, cur ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
}

// دیالوگ راهنمای بازگشایی و تماس با شرکت
static lv_obj_t *lock_help_panel = NULL;

static void lock_help_close_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (lock_help_panel) {
        lv_obj_del(lock_help_panel);
        lock_help_panel = NULL;
    }
}

static void lock_help_open_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (lock_help_panel || !lock_overlay) return;

    lock_help_panel = lv_obj_create(lock_overlay);
    lv_obj_set_size(lock_help_panel, 440, 260);
    lv_obj_center(lock_help_panel);
    lv_obj_clear_flag(lock_help_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lock_help_panel, lv_color_hex(0xff1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(lock_help_panel, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(lock_help_panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lock_help_panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(lock_help_panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *close_btn = lv_btn_create(lock_help_panel);
    lv_obj_set_size(close_btn, 36, 36);
    lv_obj_align(close_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xff334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(close_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(close_btn, lock_help_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cl = lv_label_create(close_btn);
    lv_obj_center(cl);
    lv_label_set_text(cl, LV_SYMBOL_CLOSE);

    lv_obj_t *htitle = lv_label_create(lock_help_panel);
    lv_obj_align(htitle, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_font(htitle, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(htitle, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(htitle, ML("راهنمای بازگشایی قفل", "Unlock Guide", "Kilit Acma Rehberi"));

    lv_obj_t *hbody = lv_label_create(lock_help_panel);
    lv_obj_set_size(hbody, 410, LV_SIZE_CONTENT);
    lv_obj_align(hbody, LV_ALIGN_TOP_MID, 0, 48);
    lv_label_set_long_mode(hbody, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(hbody, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hbody, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(hbody, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(hbody, (get_ui_language_index() == 0) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(hbody, (get_ui_language_index() == 0) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);

    String coName = storage.getCompanyName();
    coName.trim();
    char msg[384];
    if (get_ui_language_index() == 0) {
        if (coName.length() > 0) {
            snprintf(msg, sizeof(msg),
                     "• جهت دریافت رمز بازگشایی با شرکت «%s» تماس بگیرید.\n"
                     "• در صورت نداشتن شماره تماس، نام «%s» را در اینترنت جستجو فرمایید.\n"
                     "• کد استعلام دستگاه: %05d",
                     coName.c_str(), coName.c_str(), lock_random_code);
        } else {
            snprintf(msg, sizeof(msg),
                     "• جهت دریافت رمز بازگشایی با شرکتی که دستگاه را از آن خریداری کرده‌اید تماس حاصل فرمایید.\n"
                     "• کد استعلام دستگاه: %05d",
                     lock_random_code);
        }
    } else {
        snprintf(msg, sizeof(msg),
                 "• Contact the vendor %s to obtain the unlock password.\n"
                 "• Device challenge code: %05d",
                 coName.length() > 0 ? coName.c_str() : "", lock_random_code);
    }
    lv_label_set_text(hbody, msg);
}

// اگر overlay به هر دلیلی (مثلاً پاک‌سازیِ layer_top هنگام گذارِ صفحه) حذف شد،
// پوینترها NULL شوند تا use-after-free رخ ندهد.
static void lock_overlay_deleted_cb(lv_event_t *e)
{
    (void)e;
    lock_overlay = NULL;
    lock_overlay_ta = NULL;
    lock_overlay_kb = NULL;
    lock_overlay_info = NULL;
    lock_eye_label = NULL;
    lock_random_label = NULL;
    lock_help_panel = NULL;
}

static void lock_overlay_build()
{
    lock_random_code = (int)(esp_random() % 90000U) + 10000;
    lock_overlay = lv_obj_create(lv_layer_top());
    lv_obj_add_event_cb(lock_overlay, lock_overlay_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(lock_overlay, 480, 320);
    lv_obj_set_pos(lock_overlay, 0, 0);
    lv_obj_clear_flag(lock_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lock_overlay, LV_OBJ_FLAG_CLICKABLE); // ورودیِ صفحاتِ زیرین بلاک شود
    lv_obj_set_style_bg_color(lock_overlay, lv_color_hex(0xff121212), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lock_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lock_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(lock_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(lock_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(lock_overlay);
    lv_obj_set_pos(title, 0, 6);
    lv_obj_set_size(title, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xfff21c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("دستگاه قفل است", "Device Locked", "Cihaz Kilitli"));

    // دکمه راهنما و تماس با شرکت سازنده
    {
        lv_obj_t *help_btn = lv_btn_create(lock_overlay);
        lv_obj_set_pos(help_btn, 428, 10);
        lv_obj_set_size(help_btn, 40, 40);
        lv_obj_set_style_bg_color(help_btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(help_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(help_btn, lock_help_open_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *hl = lv_label_create(help_btn);
        lv_obj_center(hl);
        lv_obj_set_style_text_font(hl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(hl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(hl, LV_SYMBOL_LIST);
    }

    lock_overlay_info = lv_label_create(lock_overlay);
    lv_obj_set_pos(lock_overlay_info, 0, 40);
    lv_obj_set_size(lock_overlay_info, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(lock_overlay_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lock_overlay_info, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lock_overlay_info, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lock_overlay_info, ML("رمز را وارد کنید", "Enter Password", "Sifre Girin"));

    lock_overlay_ta = lv_textarea_create(lock_overlay);
    lv_obj_set_pos(lock_overlay_ta, 96, 72);
    lv_obj_set_size(lock_overlay_ta, 250, 40);   // باریک‌تر تا چشم سمتِ راست جا شود
    lv_textarea_set_one_line(lock_overlay_ta, true);
    lv_textarea_set_password_mode(lock_overlay_ta, true);
    lv_textarea_set_password_show_time(lock_overlay_ta, 0); // بدون تاخیر تایمر ۱.۵ ثانیه‌ای
    lv_textarea_set_max_length(lock_overlay_ta, 12);
    lv_obj_clear_flag(lock_overlay_ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(lock_overlay_ta, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_outline_width(lock_overlay_ta, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_shadow_width(lock_overlay_ta, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_align(lock_overlay_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lock_overlay_ta, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(lock_overlay_ta, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(lock_overlay_ta, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    // چشم سمتِ راستِ کادر — برای دیدن/مخفی‌کردنِ رمز
    {
        lv_obj_t *eye = lv_btn_create(lock_overlay);
        lv_obj_set_pos(eye, 352, 72);
        lv_obj_set_size(eye, 40, 40);
        lv_obj_set_style_bg_color(eye, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(eye, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(eye, lock_eye_toggle_cb, LV_EVENT_CLICKED, NULL);
        lock_eye_label = lv_label_create(eye);
        lv_obj_center(lock_eye_label);
        lv_obj_set_style_text_font(lock_eye_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lock_eye_label, LV_SYMBOL_EYE_OPEN);
    }

    {
        char rbuf[48];
        snprintf(rbuf, sizeof(rbuf), ML("کد استعلام دستگاه: %05d", "Device Code: %05d", "Cihaz Kodu: %05d"), lock_random_code);
        lock_random_label = lv_label_create(lock_overlay);
        lv_obj_set_pos(lock_random_label, 0, 115);
        lv_obj_set_size(lock_random_label, 480, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(lock_random_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lock_random_label, lv_color_hex(0xff90caf9), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lock_random_label, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lock_random_label, rbuf);
    }

    // کیبردِ سفارشی مثلِ کیبردِ ورودِ مدیر: دکمه‌ها در VALUE_CHANGED مدیریت می‌شوند (مطمئن ثبت می‌کند)
    lock_overlay_kb = lv_keyboard_create(lock_overlay);
    lv_obj_clear_flag(lock_overlay_kb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(lock_overlay_kb, 480, 175);
    lv_obj_align(lock_overlay_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    const char **lkmap = (get_ui_language_index()==1) ? (const char**)lock_kb_map_en
                        : (get_ui_language_index()==2) ? (const char**)lock_kb_map_tr
                                                       : (const char**)lock_kb_map_fa;
    lv_keyboard_set_map(lock_overlay_kb, LV_KEYBOARD_MODE_USER_1, lkmap, lock_kb_ctrl);
    lv_keyboard_set_mode(lock_overlay_kb, LV_KEYBOARD_MODE_USER_1);
    lv_obj_set_style_text_font(lock_overlay_kb, &ui_font_vazirmatn_bold_26, LV_PART_ITEMS | LV_STATE_DEFAULT);
    // دکمهٔ انتخاب‌شده هنگامِ ناوبری با MOD واضح دیده شود
    lv_obj_set_style_bg_color(lock_overlay_kb, lv_color_hex(0xff2196f3), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(lock_overlay_kb, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(lock_overlay_kb, lv_color_hex(0xff0d47a1), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(lock_overlay_kb, 2, LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(lock_overlay_kb, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(lock_overlay_kb, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(lock_overlay_kb, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(lock_overlay_kb, lv_color_hex(0xff38bdf8), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(lock_overlay_kb, 2, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_flag(lock_overlay_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(lock_overlay_kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_keyboard_set_textarea(lock_overlay_kb, NULL); // handler سفارشی همه‌چیز را مدیریت می‌کند
    lv_obj_add_event_cb(lock_overlay_kb, lock_kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void lock_overlay_destroy()
{
    if (lock_overlay)
    {
        lv_obj_del(lock_overlay);
        lock_overlay = NULL;
        lock_overlay_ta = NULL;
        lock_overlay_kb = NULL;
        lock_overlay_info = NULL;
    }
}

// ===== دیالوگ زنجیرهٔ ۵ تایمرِ چک (T1 روی صفحه مدیر، T2..T5 اینجا) =====
#define MT_DIALOG_STAGES 4
static lv_obj_t *mt_dialog = NULL;
static lv_obj_t *mt_help_panel = NULL;
static lv_obj_t *mt_help_scroll = NULL;
static lv_obj_t *mt_help_close_btn = NULL;
static lv_obj_t *mt_help_open_btn = NULL;
static lv_obj_t *mt_close_btn = NULL;
static lv_obj_t *mt_t1_days_lbl = NULL;
static lv_obj_t *mt_t1_date_lbl = NULL;
static lv_obj_t *mt_t1_pwd_lbl = NULL;
static lv_obj_t *mt_sliders[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_minus_btns[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_plus_btns[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_val_labels[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_day_labels[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_date_labels[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_pwd_labels[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_title_labels[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_cards[MT_DIALOG_STAGES] = {NULL};
static lv_obj_t *mt_t1_card = NULL;

// پالت iOS — پس‌زمینه خاکستری متمایز، کارت روشن (نه سفید خالص)
#define MT_CLR_BG      0xD1D1D6
#define MT_CLR_CARD    0xF2F2F7
#define MT_CLR_BORDER  0xAEAEB2
#define MT_CLR_TEXT    0x1C1C1E
#define MT_CLR_SECOND  0x48484A
#define MT_CLR_ACCENT  0x007AFF
#define MT_CLR_BTN_BG  0xE5E5EA
#define MT_CLR_BTN_TXT 0x1C1C1E
#define MT_TOUCH_MIN   40
#define MT_BTN_SZ      28

// این استایل‌ها قبلاً با lv_obj_set_style_* مستقیم روی هر آبجکت (local style) ست می‌شدن —
// یعنی هر بار دیالوگ باز می‌شد (ساخته/حذف می‌شد)، برای هر کارت/دکمه/اسلایدر دوباره از صفر
// چندین ویژگیِ استایل تخصیص داده می‌شد. چون این مقادیر همیشه ثابت‌ان، یک‌بار به‌عنوانِ
// استایلِ مشترک (shared) ساخته می‌شن و فقط با lv_obj_add_style به هر آبجکت وصل می‌شن —
// خروجیِ بصری دقیقاً همون قبلیه، فقط حافظه/زمانِ ساختِ کمتری می‌گیره.
static lv_style_t mt_style_stage_card_s;
static lv_style_t mt_style_card_s;
static lv_style_t mt_style_page_s;
static lv_style_t mt_style_tool_btn_s;
static lv_style_t mt_style_slider_main_s;
static lv_style_t mt_style_slider_indic_s;
static lv_style_t mt_style_slider_knob_s;
static lv_style_t mt_style_small_btn_s;
static lv_style_t mt_style_small_btn_label_s;
static bool mt_shared_styles_ready = false;

static void mt_init_shared_styles(void)
{
    if (mt_shared_styles_ready) return;
    mt_shared_styles_ready = true;

    lv_style_init(&mt_style_stage_card_s);
    lv_style_set_bg_color(&mt_style_stage_card_s, lv_color_hex(MT_CLR_CARD));
    lv_style_set_bg_opa(&mt_style_stage_card_s, LV_OPA_COVER);
    lv_style_set_border_color(&mt_style_stage_card_s, lv_color_hex(MT_CLR_BORDER));
    lv_style_set_border_width(&mt_style_stage_card_s, 1);
    lv_style_set_radius(&mt_style_stage_card_s, 8);
    lv_style_set_shadow_width(&mt_style_stage_card_s, 0);
    lv_style_set_shadow_opa(&mt_style_stage_card_s, 0);
    lv_style_set_shadow_ofs_y(&mt_style_stage_card_s, 0);
    lv_style_set_shadow_color(&mt_style_stage_card_s, lv_color_hex(0x000000));

    lv_style_init(&mt_style_card_s);
    lv_style_set_bg_color(&mt_style_card_s, lv_color_hex(MT_CLR_CARD));
    lv_style_set_bg_opa(&mt_style_card_s, LV_OPA_COVER);
    lv_style_set_border_color(&mt_style_card_s, lv_color_hex(MT_CLR_BORDER));
    lv_style_set_border_width(&mt_style_card_s, 1);
    lv_style_set_radius(&mt_style_card_s, 10);
    // سایهٔ سنگین روی چند کارت = هزینهٔ بزرگِ رندر؛ کم‌عمق نگه می‌داریم تا ظاهر کارت بماند.
    lv_style_set_shadow_width(&mt_style_card_s, 4);
    lv_style_set_shadow_opa(&mt_style_card_s, 18);
    lv_style_set_shadow_ofs_y(&mt_style_card_s, 1);
    lv_style_set_shadow_color(&mt_style_card_s, lv_color_hex(0x000000));

    lv_style_init(&mt_style_page_s);
    lv_style_set_bg_color(&mt_style_page_s, lv_color_hex(MT_CLR_BG));
    lv_style_set_bg_opa(&mt_style_page_s, LV_OPA_COVER);
    lv_style_set_border_width(&mt_style_page_s, 0);

    lv_style_init(&mt_style_tool_btn_s);
    lv_style_set_bg_color(&mt_style_tool_btn_s, lv_color_hex(MT_CLR_CARD));
    lv_style_set_border_color(&mt_style_tool_btn_s, lv_color_hex(MT_CLR_BORDER));
    lv_style_set_border_width(&mt_style_tool_btn_s, 1);
    lv_style_set_radius(&mt_style_tool_btn_s, 10);
    lv_style_set_shadow_width(&mt_style_tool_btn_s, 0);
    lv_style_set_shadow_opa(&mt_style_tool_btn_s, 0);
    lv_style_set_shadow_ofs_y(&mt_style_tool_btn_s, 0);

    lv_style_init(&mt_style_slider_main_s);
    lv_style_set_bg_color(&mt_style_slider_main_s, lv_color_hex(MT_CLR_BTN_BG));
    lv_style_set_height(&mt_style_slider_main_s, 8);

    lv_style_init(&mt_style_slider_indic_s);
    lv_style_set_bg_color(&mt_style_slider_indic_s, lv_color_hex(MT_CLR_ACCENT));
    lv_style_set_height(&mt_style_slider_indic_s, 8);

    lv_style_init(&mt_style_slider_knob_s);
    lv_style_set_bg_color(&mt_style_slider_knob_s, lv_color_hex(MT_CLR_CARD));
    lv_style_set_border_color(&mt_style_slider_knob_s, lv_color_hex(MT_CLR_ACCENT));
    lv_style_set_border_width(&mt_style_slider_knob_s, 2);
    lv_style_set_width(&mt_style_slider_knob_s, 14);
    lv_style_set_height(&mt_style_slider_knob_s, 14);
    lv_style_set_pad_all(&mt_style_slider_knob_s, 0);
    lv_style_set_radius(&mt_style_slider_knob_s, LV_RADIUS_CIRCLE);

    lv_style_init(&mt_style_small_btn_s);
    lv_style_set_bg_color(&mt_style_small_btn_s, lv_color_hex(MT_CLR_BTN_BG));
    lv_style_set_border_color(&mt_style_small_btn_s, lv_color_hex(MT_CLR_BORDER));
    lv_style_set_border_width(&mt_style_small_btn_s, 1);
    lv_style_set_radius(&mt_style_small_btn_s, 8);
    lv_style_set_shadow_width(&mt_style_small_btn_s, 0);

    lv_style_init(&mt_style_small_btn_label_s);
    lv_style_set_text_font(&mt_style_small_btn_label_s, &lv_font_montserrat_14);
    lv_style_set_text_color(&mt_style_small_btn_label_s, lv_color_hex(MT_CLR_BTN_TXT));
}

static void mt_style_stage_card(lv_obj_t *obj)
{
    mt_init_shared_styles();
    lv_obj_add_style(obj, &mt_style_stage_card_s, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void mt_style_card(lv_obj_t *obj)
{
    mt_init_shared_styles();
    lv_obj_add_style(obj, &mt_style_card_s, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void mt_style_page(lv_obj_t *obj)
{
    mt_init_shared_styles();
    lv_obj_add_style(obj, &mt_style_page_s, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void mt_style_tool_btn(lv_obj_t *btn)
{
    mt_init_shared_styles();
    lv_obj_add_style(btn, &mt_style_tool_btn_s, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_ext_click_area(btn, 8);
}

static void mt_style_slider(lv_obj_t *sl)
{
    mt_init_shared_styles();
    lv_obj_add_style(sl, &mt_style_slider_main_s, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(sl, &mt_style_slider_indic_s, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(sl, &mt_style_slider_knob_s, LV_PART_KNOB | LV_STATE_DEFAULT);
}

static const char *mt_help_body_text(void)
{
    return ML(
        "۱) ۵ مرحله: T1 در صفحه مدیر، T2 تا T5 در این بخش\n"
        "۲) هر مرحله N روز، قفل در تاریخ جمع روزها فعال می‌شود\n"
        "۳) فرمول رمز هر تایمر: (رمز مدیر × ضریب مرحله) + یکان جمع ارقام\n"
        "   ضریب T1 = ۲ | ضریب T2 = ۳ | ضریب T3 = ۴ | ضریب T4 = ۵ | ضریب T5 = ۶\n"
        "   مثال با رمز مدیر ۱۰۰۰:\n"
        "   رمز T1: ۲۰۰۰۲ (۱۰۰۰ × ۲ = ۲۰۰۰ + یکان جمع ارقام ۲)\n"
        "   رمز T2: ۳۰۰۰۳ (۱۰۰۰ × ۳ = ۳۰۰۰ + یکان جمع ارقام ۳)\n"
        "   رمز T3: ۴۰۰۰۴ | رمز T4: ۵۰۰۰۵ | رمز T5: ۶۰۰۰۶\n"
        "۴) رمز ۱۱۲۲۳۳: فقط یک‌بار در طول عمر دستگاه، ۱۰ روز مهلت اضطراری\n"
        "۵) رمز مدیر یا ۱۲۷۷۳: غیرفعال‌سازی فوری و پاک‌شدن تمام تایمرها\n"
        "۶) ۳ بار رمز اشتباه: قفل امنیتی ۵ دقیقه‌ای (تداوم پس از ری‌استارت)\n"
        "۷) دستکاری ساعت یا باتری RTC: قفل فوری با تایمر موعد",
        "1) 5 stages: T1 on admin page, T2 to T5 here\n"
        "2) Each stage N days, locks at cumulative date\n"
        "3) Password formula: (admin pwd x stage mult) + digit sum mod 10\n"
        "   Mult: T1=2, T2=3, T3=4, T4=5, T5=6\n"
        "   Example for admin 1000:\n"
        "   T1: 20002 | T2: 30003 | T3: 40004 | T4: 50005 | T5: 60006\n"
        "4) 112233: One-time emergency grace, +10 days\n"
        "5) Admin pwd or 12773: Clears all timers completely\n"
        "6) 3 wrong attempts: 5-minute security lockout (persists on reset)\n"
        "7) RTC tampering / battery pull: Immediate lockout",
        "1) 5 asama: T1 admin, T2-T5 burada\n"
        "2) Her asama N gun, toplam tarihte kilit\n"
        "3) Sifre = (admin x katsayi) + rakam toplami mod 10\n"
        "   Katsayi: T1=2, T2=3, T3=4, T4=5, T5=6\n"
        "   Ornek admin 1000 icin: T1: 20002, T2: 30003, T3: 40004, T4: 50005, T5: 60006\n"
        "4) 112233: Tek kullanimlik 10 gun uzatma\n"
        "5) Admin sifresi veya 12773: Tumunu siler\n"
        "6) 3 yanlis deneme: 5 dakika kilit\n"
        "7) Saat veya pil kurcalama: Aninda kilit");
}

static void mt_format_lock_date(char *buf, size_t sz, long jdn)
{
    if (jdn <= 0) { snprintf(buf, sz, "---"); return; }
    int gy = 0, gm = 0, gd = 0;
    Storage::jdnToGregorian(jdn, gy, gm, gd);
    if (get_ui_language_index() == 0) {
        int jy = 0, jm = 0, jd = 0;
        date_utils::gregorian_to_jalali(gy, gm, gd, jy, jm, jd);
        // LRM تا ترتیب سال/ماه/روز در RTL به‌هم نریزد
        snprintf(buf, sz, "\xe2\x80\x8e%04d/%02d/%02d", jy, jm, jd);
    } else {
        snprintf(buf, sz, "%04d-%02d-%02d", gy, gm, gd);
    }
}

static void mt_format_date_prefix_line(char *line, size_t sz, const char *dstr)
{
    if (get_ui_language_index() == 0) {
        snprintf(line, sz, "%s تا", dstr);
    } else if (get_ui_language_index() == 1) {
        snprintf(line, sz, "%s until", dstr);
    } else {
        snprintf(line, sz, "%s'e kadar", dstr);
    }
}

static int mt_t1_days(void)
{
    int d = (int)get_var_setting_admin_countdown_timer();
    if (d < 0) d = 0;
    if (d > 125) d = 125;
    return d;
}

static int mt_timer_days(int t)
{
    if (t < 2 || t > 5) return 0;
    int d = g_mt_draft_days[t - 2];
    if (d > 50) d = 50;
    return d;
}

// قاضی: آیا دیالوگ مجاز است؟ (T1 باید روز داشته باشد)
static bool mt_judge_can_open(char *msg, size_t msg_sz)
{
    if (mt_t1_days() <= 0) {
        if (msg && msg_sz) {
            snprintf(msg, msg_sz, ML("اول روزهای T1 را در صفحه مدیر تنظیم کنید",
                                       "Set T1 days on the admin page first",
                                       "Once T1 gunlerini admin sayfasinda ayarlayin"));
        }
        return false;
    }
    return true;
}

// قاضی: آیا تایمر t (۲..۵) قابل ویرایش است؟
static bool mt_judge_timer_editable(int t)
{
    if (t < 2 || t > 5) return false;
    if (mt_t1_days() <= 0) return false;
    if (t == 2) return true;
    return mt_timer_days(t - 1) > 0;
}

static void mt_set_timer_day(int t, int days)
{
    if (t < 2 || t > 5) return;
    if (days < 0) days = 0;
    if (days > 50) days = 50;
    g_mt_draft_days[t - 2] = days;
    g_mt_touched = true;
    if (days == 0) {
        for (int u = t + 1; u <= 5; u++)
            g_mt_draft_days[u - 2] = 0;
    }
}

// کش وضعیت قفل ردیف — تا هر refresh دوباره opa/disabled روی کل کارت نزند
static bool mt_row_en_cache[MT_DIALOG_STAGES];
static bool mt_row_en_cache_valid = false;
static char mt_admin_pass_cache[24];
static long mt_base_jdn_cache = 0;
static bool mt_ctx_cache_valid = false;

static void mt_label_set_if_changed(lv_obj_t *obj, const char *txt)
{
    if (!obj || !txt) return;
    const char *cur = lv_label_get_text(obj);
    if (cur && strcmp(cur, txt) == 0) return;
    lv_label_set_text(obj, txt);
}

static void mt_invalidate_ui_caches(void)
{
    mt_row_en_cache_valid = false;
    mt_ctx_cache_valid = false;
}

static void mt_ensure_ctx_cache(void)
{
    if (mt_ctx_cache_valid) return;
    mt_base_jdn_cache = storage.getChainStartJDN();
    if (mt_base_jdn_cache <= 0) mt_base_jdn_cache = today_jdn_from_rtc();
    String adminPassStr = storage.getAdminPassword();
    adminPassStr.trim();
    strncpy(mt_admin_pass_cache, adminPassStr.c_str(), sizeof(mt_admin_pass_cache) - 1);
    mt_admin_pass_cache[sizeof(mt_admin_pass_cache) - 1] = '\0';
    mt_ctx_cache_valid = true;
}

static void mt_apply_row_lock_state(int idx)
{
    if (idx < 0 || idx >= MT_DIALOG_STAGES) return;
    int t = idx + 2;
    bool en = mt_judge_timer_editable(t);
    if (mt_row_en_cache_valid && mt_row_en_cache[idx] == en) return;
    mt_row_en_cache[idx] = en;

    lv_opa_t opa = en ? LV_OPA_COVER : LV_OPA_60;
    if (mt_cards[idx]) lv_obj_set_style_opa(mt_cards[idx], opa, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (mt_sliders[idx]) {
        if (en) lv_obj_remove_state(mt_sliders[idx], LV_STATE_DISABLED);
        else lv_obj_add_state(mt_sliders[idx], LV_STATE_DISABLED);
    }
    if (mt_minus_btns[idx]) {
        if (en) lv_obj_remove_state(mt_minus_btns[idx], LV_STATE_DISABLED);
        else lv_obj_add_state(mt_minus_btns[idx], LV_STATE_DISABLED);
    }
    if (mt_plus_btns[idx]) {
        if (en) lv_obj_remove_state(mt_plus_btns[idx], LV_STATE_DISABLED);
        else lv_obj_add_state(mt_plus_btns[idx], LV_STATE_DISABLED);
    }
}

static void mt_refresh_t1_strip(void)
{
    if (!mt_t1_days_lbl) return;
    mt_ensure_ctx_cache();
    long baseJdn = mt_base_jdn_cache;
    int d1 = mt_t1_days();
    char dstr[16], pwd[24];
    mt_format_lock_date(dstr, sizeof(dstr), (baseJdn > 0 && d1 > 0) ? (baseJdn + d1) : 0);
    mt_make_timer_password(pwd, sizeof(pwd), mt_admin_pass_cache, 1);
    char line1[48], line2[48], line3[48];
    if (get_ui_language_index() == 0) {
        snprintf(line1, sizeof(line1), "T1: روز \xe2\x80\x8e%d\xe2\x80\x8e", d1);
    } else if (get_ui_language_index() == 1) {
        snprintf(line1, sizeof(line1), "T1: days %d", d1);
    } else {
        snprintf(line1, sizeof(line1), "T1: gun %d", d1);
    }
    mt_label_set_if_changed(mt_t1_days_lbl, line1);
    snprintf(line3, sizeof(line3), ML("رمز: %s", "pwd: %s", "sifre: %s"), pwd);
    if (mt_t1_date_lbl) {
        mt_format_date_prefix_line(line2, sizeof(line2), dstr);
        mt_label_set_if_changed(mt_t1_date_lbl, line2);
    }
    if (mt_t1_pwd_lbl) mt_label_set_if_changed(mt_t1_pwd_lbl, line3);
}

// از مرحلهٔ from_idx (۰=T2) تا آخر: فقط لیبل/اسلایدرِ وابسته؛ بدون دست زدن به T1
static void mt_refresh_stages_from(int from_idx, bool sync_sliders)
{
    if (from_idx < 0) from_idx = 0;
    if (from_idx >= MT_DIALOG_STAGES) return;
    mt_ensure_ctx_cache();
    long baseJdn = mt_base_jdn_cache;
    int cum = mt_t1_days();
    for (int t = 2; t < 2 + from_idx; t++) cum += mt_timer_days(t);

    for (int t = 2 + from_idx; t <= 5; t++) {
        int idx = t - 2;
        int d = mt_timer_days(t);
        cum += d;
        long lockJdn = (baseJdn > 0 && cum > 0) ? (baseJdn + cum) : 0;
        char dstr[16], pwd[24];
        mt_format_lock_date(dstr, sizeof(dstr), lockJdn);
        mt_make_timer_password(pwd, sizeof(pwd), mt_admin_pass_cache, t);

        if (mt_val_labels[idx]) {
            char vtxt[8];
            snprintf(vtxt, sizeof(vtxt), "%d", d);
            mt_label_set_if_changed(mt_val_labels[idx], vtxt);
        }
        if (sync_sliders && mt_sliders[idx] && (int)lv_slider_get_value(mt_sliders[idx]) != d) {
            lv_slider_set_value(mt_sliders[idx], d, LV_ANIM_OFF);
        }
        if (mt_day_labels[idx]) {
            char line[40];
            snprintf(line, sizeof(line), ML("%d روز (جمع %d)", "%d days (sum %d)", "%d gun (toplam %d)"), d, cum);
            mt_label_set_if_changed(mt_day_labels[idx], line);
        }
        if (mt_date_labels[idx]) {
            char line[40];
            mt_format_date_prefix_line(line, sizeof(line), dstr);
            mt_label_set_if_changed(mt_date_labels[idx], line);
        }
        if (mt_pwd_labels[idx]) {
            char line[32];
            snprintf(line, sizeof(line), ML("رمز: %s", "pwd: %s", "sifre: %s"), pwd);
            mt_label_set_if_changed(mt_pwd_labels[idx], line);
        }
        mt_apply_row_lock_state(idx);
    }
    mt_row_en_cache_valid = true;
}

static void mt_refresh_dialog_rows(void)
{
    mt_ctx_cache_valid = false; // یک‌بار در بازشدن/رفرش کامل، RTC و رمز را تازه بگیر
    mt_refresh_t1_strip();
    mt_refresh_stages_from(0, true);
}

static void mt_step_timer(int idx, int delta)
{
    if (idx < 0 || idx >= MT_DIALOG_STAGES) return;
    int t = idx + 2;
    if (!mt_judge_timer_editable(t)) return;
    int v = mt_timer_days(t) + delta;
    mt_set_timer_day(t, v);
    // ± فقط از همین مرحله به بعد لازم است؛ T1 ثابت است
    mt_refresh_stages_from(idx, true);
}

static void mt_slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= MT_DIALOG_STAGES) return;
    if (!mt_judge_timer_editable(idx + 2)) return;

    int v = (int)lv_slider_get_value(lv_event_get_target(e));
    mt_set_timer_day(idx + 2, v);

    if (code == LV_EVENT_RELEASED) {
        // فقط بعد از رها کردن: تاریخ/رمز/جمع و قفل مراحل بعدی
        mt_refresh_stages_from(idx, true);
        return;
    }

    // حین کشیدن: فقط عدد همان کارت — بدون بازنویسی لیبل‌های فارسی تاریخ/رمز روی چند کارت
    if (mt_val_labels[idx]) {
        char vtxt[8];
        snprintf(vtxt, sizeof(vtxt), "%d", v);
        mt_label_set_if_changed(mt_val_labels[idx], vtxt);
    }
}

static void mt_minus_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    mt_step_timer((int)(intptr_t)lv_event_get_user_data(e), -1);
}

static void mt_plus_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    mt_step_timer((int)(intptr_t)lv_event_get_user_data(e), +1);
}

static void mt_val_tap_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    mt_step_timer((int)(intptr_t)lv_event_get_user_data(e), +1);
}

static void mt_help_close_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (mt_help_panel) lv_obj_add_flag(mt_help_panel, LV_OBJ_FLAG_HIDDEN);
}

static void mt_help_open_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
    if (mt_help_panel) {
        lv_obj_clear_flag(mt_help_panel, LV_OBJ_FLAG_HIDDEN);
        if (mt_help_scroll) lv_obj_scroll_to(mt_help_scroll, 0, 0, LV_ANIM_OFF);
        lv_obj_move_foreground(mt_help_panel);
    }
}

static void mt_build_help_panel(void)
{
    if (!mt_dialog || mt_help_panel) return;
    mt_help_panel = lv_obj_create(mt_dialog);
    lv_obj_set_size(mt_help_panel, 480, 320);
    lv_obj_set_pos(mt_help_panel, 0, 0);
    lv_obj_clear_flag(mt_help_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(mt_help_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mt_help_panel, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(mt_help_panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *box = lv_obj_create(mt_help_panel);
    lv_obj_set_size(box, 448, 268);
    lv_obj_center(box);
    mt_style_card(box);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_top(box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(box, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(box, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *close_btn = lv_btn_create(box);
    lv_obj_set_size(close_btn, MT_TOUCH_MIN, MT_TOUCH_MIN);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 12, -4);
    mt_style_tool_btn(close_btn);
    lv_obj_add_event_cb(close_btn, mt_help_close_cb, LV_EVENT_CLICKED, NULL);
    mt_help_close_btn = close_btn;
    lv_obj_t *xl = lv_label_create(close_btn);
    lv_label_set_text(xl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(xl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(xl, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(xl);
    lv_obj_move_foreground(close_btn);

    lv_obj_t *ht = lv_label_create(box);
    lv_obj_align(ht, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(ht, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ht, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ht, ML("راهنمای تایمرهای چک", "Check timer guide", "Cek zamanlayici rehberi"));

    const lv_coord_t body_w = 412;
    const lv_coord_t scroll_h = 212;
    mt_help_scroll = lv_obj_create(box);
    lv_obj_set_size(mt_help_scroll, body_w, scroll_h);
    lv_obj_align(mt_help_scroll, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_add_flag(mt_help_scroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(mt_help_scroll, LV_DIR_VER);
    lv_obj_clear_flag(mt_help_scroll, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_scrollbar_mode(mt_help_scroll, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(mt_help_scroll, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(mt_help_scroll, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(mt_help_scroll, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *body = lv_label_create(mt_help_scroll);
    lv_obj_set_width(body, (lv_coord_t)(body_w - 4));
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(body, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(body, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(body, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (get_ui_language_index() == 0) {
        lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_base_dir(body, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_label_set_text(body, mt_help_body_text());
}

static void mt_dialog_destroy(void)
{
    if (!mt_dialog) return;
    lv_obj_del(mt_dialog);
    mt_dialog = NULL;
    mt_help_panel = NULL;
    mt_help_scroll = NULL;
    mt_help_close_btn = NULL;
    mt_help_open_btn = NULL;
    mt_close_btn = NULL;
    mt_t1_days_lbl = NULL;
    mt_t1_date_lbl = NULL;
    mt_t1_pwd_lbl = NULL;
    mt_t1_card = NULL;
    for (int i = 0; i < MT_DIALOG_STAGES; i++) {
        mt_sliders[i] = NULL;
        mt_minus_btns[i] = NULL;
        mt_plus_btns[i] = NULL;
        mt_val_labels[i] = NULL;
        mt_day_labels[i] = NULL;
        mt_date_labels[i] = NULL;
        mt_pwd_labels[i] = NULL;
        mt_title_labels[i] = NULL;
        mt_cards[i] = NULL;
    }
    mt_invalidate_ui_caches();
}

static void mt_dialog_close_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    mt_dialog_destroy();
}

extern "C" void multi_timer_dialog_close(void) { mt_dialog_destroy(); }

extern "C" bool multi_timer_dialog_is_open(void) { return mt_dialog != NULL; }

extern "C" bool multi_timer_dialog_can_open(void)
{
    return mt_judge_can_open(NULL, 0);
}

extern "C" lv_obj_t *mt_nav_close(void) { return mt_close_btn; }
extern "C" lv_obj_t *mt_nav_help_open(void) { return mt_help_open_btn; }
extern "C" lv_obj_t *mt_nav_help_close(void) { return mt_help_close_btn; }
extern "C" bool mt_nav_help_is_open(void) { return mt_help_panel && !lv_obj_has_flag(mt_help_panel, LV_OBJ_FLAG_HIDDEN); }

extern "C" int mt_nav_dialog_get(lv_obj_t **out, int max)
{
    int n = 0;
    if (!mt_dialog || !out || max <= 0) return 0;
    if (mt_nav_help_is_open()) {
        if (n < max) out[n++] = mt_nav_help_close();
        return n;
    }
    if (n < max) out[n++] = mt_nav_help_open();
    if (n < max) out[n++] = mt_nav_close();
    for (int i = 0; i < MT_DIALOG_STAGES && n < max; i++) {
        if (!mt_judge_timer_editable(i + 2)) continue;
        if (mt_minus_btns[i] && n < max) out[n++] = mt_minus_btns[i];
        if (mt_plus_btns[i] && n < max) out[n++] = mt_plus_btns[i];
        if (mt_sliders[i] && n < max) out[n++] = mt_sliders[i];
    }
    return n;
}

static lv_obj_t *mt_make_small_btn(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, const char *txt,
                                   lv_event_cb_t cb, void *ud)
{
    mt_init_shared_styles();
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, MT_BTN_SZ, MT_BTN_SZ);
    lv_obj_set_pos(b, x, y);
    lv_obj_add_style(b, &mt_style_small_btn_s, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_ext_click_area(b, 6);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_add_style(l, &mt_style_small_btn_label_s, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(l);
    return b;
}

extern "C" void multi_timer_dialog_open(void)
{
    if (mt_dialog) return;
    if (!mt_judge_can_open(NULL, 0)) return;
    xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
    mt_invalidate_ui_caches();

    mt_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(mt_dialog, 480, 320);
    lv_obj_set_pos(mt_dialog, 0, 0);
    lv_obj_clear_flag(mt_dialog, LV_OBJ_FLAG_SCROLLABLE);
    mt_style_page(mt_dialog);
    lv_obj_set_style_pad_all(mt_dialog, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *help_btn = lv_btn_create(mt_dialog);
    lv_obj_set_size(help_btn, MT_TOUCH_MIN, MT_TOUCH_MIN);
    lv_obj_align(help_btn, LV_ALIGN_TOP_LEFT, 8, 2);
    mt_style_tool_btn(help_btn);
    lv_obj_add_event_cb(help_btn, mt_help_open_cb, LV_EVENT_CLICKED, NULL);
    mt_help_open_btn = help_btn;
    lv_obj_t *hl = lv_label_create(help_btn);
    lv_label_set_text(hl, "?");
    lv_obj_set_style_text_font(hl, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hl, lv_color_hex(MT_CLR_ACCENT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(hl);

    lv_obj_t *close_btn = lv_btn_create(mt_dialog);
    lv_obj_set_size(close_btn, MT_TOUCH_MIN, MT_TOUCH_MIN);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -8, 2);
    mt_style_tool_btn(close_btn);
    lv_obj_add_event_cb(close_btn, mt_dialog_close_cb, LV_EVENT_CLICKED, NULL);
    mt_close_btn = close_btn;
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_obj_center(close_lbl);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);

    lv_obj_t *title = lv_label_create(mt_dialog);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("تایمرهای چک (5 مرحله)", "Check timers (5 stages)", "Cek zamanlayicilari (5 asama)"));

    const lv_coord_t card_content_w = 220;
    const lv_coord_t card_panel_w = 224;
    const lv_coord_t card_h = 100;
    const lv_coord_t t1_h = 44;
    const lv_coord_t col_gap = 18;
    const lv_coord_t row_gap = 10;
    const lv_coord_t t1_grid_gap = 6;
    const lv_coord_t dialog_w = 480;
    const lv_coord_t dialog_h = 320;
    const lv_coord_t header_bottom = 36;
    const lv_coord_t grid_span_w = (lv_coord_t)(2 * card_panel_w + col_gap);
    const lv_coord_t grid_span_h = (lv_coord_t)(2 * card_h + row_gap);
    const lv_coord_t grid_left = (lv_coord_t)((dialog_w - grid_span_w) / 2);
    const lv_coord_t col_x[2] = {grid_left, (lv_coord_t)(grid_left + card_panel_w + col_gap)};
    const lv_coord_t content_h = (lv_coord_t)(t1_h + t1_grid_gap + grid_span_h);
    const lv_coord_t content_top = (lv_coord_t)(header_bottom + (dialog_h - header_bottom - content_h) / 2);
    const lv_coord_t t1_y = content_top;
    const lv_coord_t top_y = (lv_coord_t)(content_top + t1_h + t1_grid_gap);

    const lv_coord_t t1_panel_w = 466;
    const lv_coord_t t1_side_w = 150;
    mt_t1_card = lv_obj_create(mt_dialog);
    lv_obj_set_size(mt_t1_card, t1_panel_w, t1_h);
    lv_obj_set_pos(mt_t1_card, grid_left, t1_y);
    lv_obj_clear_flag(mt_t1_card, LV_OBJ_FLAG_SCROLLABLE);
    mt_style_card(mt_t1_card);
    lv_obj_set_style_pad_all(mt_t1_card, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(mt_t1_card, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);

    mt_t1_days_lbl = lv_label_create(mt_t1_card);
    lv_obj_align(mt_t1_days_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_base_dir(mt_t1_days_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(mt_t1_days_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mt_t1_days_lbl, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mt_t1_days_lbl, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);

    mt_t1_pwd_lbl = lv_label_create(mt_t1_card);
    lv_obj_align(mt_t1_pwd_lbl, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_width(mt_t1_pwd_lbl, t1_side_w);
    lv_label_set_long_mode(mt_t1_pwd_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(mt_t1_pwd_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mt_t1_pwd_lbl, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mt_t1_pwd_lbl, lv_color_hex(MT_CLR_ACCENT), LV_PART_MAIN | LV_STATE_DEFAULT);

    mt_t1_date_lbl = lv_label_create(mt_t1_card);
    lv_obj_align(mt_t1_date_lbl, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_width(mt_t1_date_lbl, t1_side_w);
    lv_label_set_long_mode(mt_t1_date_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_base_dir(mt_t1_date_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(mt_t1_date_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mt_t1_date_lbl, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mt_t1_date_lbl, lv_color_hex(MT_CLR_SECOND), LV_PART_MAIN | LV_STATE_DEFAULT);

    const lv_coord_t card_pad = 4;
    const lv_coord_t card_inner_w = (lv_coord_t)(card_panel_w - 2 * card_pad);
    const lv_coord_t card_inner_h = (lv_coord_t)(card_h - 2 * card_pad);
    const lv_coord_t btn_pad = 6;
    const lv_coord_t slider_w = (lv_coord_t)(card_inner_w - 2 * (btn_pad + MT_BTN_SZ + 8));
    const lv_coord_t ctrl_y = (lv_coord_t)((card_inner_h - MT_BTN_SZ) / 2 - 8);
    const lv_coord_t slider_y = (lv_coord_t)(ctrl_y + 9);
    const lv_coord_t minus_x = btn_pad;
    const lv_coord_t slider_x = (lv_coord_t)(btn_pad + MT_BTN_SZ + 8);
    const lv_coord_t plus_x = (lv_coord_t)(btn_pad + MT_BTN_SZ + 8 + slider_w + 8);
    const lv_coord_t info_y0 = 56;
    const lv_coord_t info_y1 = 66;
    const lv_coord_t info_y_date = 76;
    const lv_coord_t info_right_w = (lv_coord_t)(card_content_w - 112);
    const lv_coord_t info_right_x = (lv_coord_t)(card_content_w - info_right_w - 6);
    const lv_coord_t pwd_w = 108;

    for (int t = 2; t <= 5; t++) {
        int idx = t - 2;
        int col = idx % 2;
        int row = idx / 2;
        lv_coord_t cx = col_x[col];
        lv_coord_t cy = (lv_coord_t)(top_y + row * (card_h + row_gap));

        lv_obj_t *card = lv_obj_create(mt_dialog);
        lv_obj_set_size(card, card_panel_w, card_h);
        lv_obj_set_pos(card, cx, cy);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        mt_style_stage_card(card);
        lv_obj_set_style_pad_all(card, card_pad, LV_PART_MAIN | LV_STATE_DEFAULT);
        mt_cards[idx] = card;

        lv_obj_t *tlbl = lv_label_create(card);
        lv_obj_set_pos(tlbl, 6, 2);
        lv_obj_set_style_text_font(tlbl, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(tlbl, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
        char ttxt[8];
        snprintf(ttxt, sizeof(ttxt), "T%d", t);
        lv_label_set_text(tlbl, ttxt);
        mt_title_labels[idx] = tlbl;

        lv_obj_t *val = lv_label_create(card);
        lv_obj_align(val, LV_ALIGN_TOP_MID, 0, 2);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(val, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(val, lv_color_hex(MT_CLR_ACCENT), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(val, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(val, mt_val_tap_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
        mt_val_labels[idx] = val;

        mt_minus_btns[idx] = mt_make_small_btn(card, minus_x, ctrl_y, "-", mt_minus_cb, (void *)(intptr_t)idx);

        lv_obj_t *sl = lv_slider_create(card);
        lv_obj_set_pos(sl, slider_x, slider_y);
        lv_obj_set_size(sl, slider_w, 10);
        lv_slider_set_range(sl, 0, 50);
        lv_slider_set_value(sl, storage.getMultiTimerDay(t), LV_ANIM_OFF);
        mt_style_slider(sl);
        lv_obj_add_event_cb(sl, mt_slider_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)idx);
        lv_obj_add_event_cb(sl, mt_slider_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)idx);
        mt_sliders[idx] = sl;

        mt_plus_btns[idx] = mt_make_small_btn(card, plus_x, ctrl_y, "+", mt_plus_cb, (void *)(intptr_t)idx);

        lv_obj_t *day_l = lv_label_create(card);
        lv_obj_set_pos(day_l, info_right_x, info_y0);
        lv_obj_set_width(day_l, info_right_w);
        lv_label_set_long_mode(day_l, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(day_l, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(day_l, lv_color_hex(MT_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
        mt_day_labels[idx] = day_l;

        lv_obj_t *date_l = lv_label_create(card);
        lv_obj_set_pos(date_l, info_right_x, info_y_date);
        lv_obj_set_width(date_l, info_right_w);
        lv_label_set_long_mode(date_l, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(date_l, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(date_l, lv_color_hex(MT_CLR_SECOND), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(date_l, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
        mt_date_labels[idx] = date_l;

        lv_obj_t *pwd_l = lv_label_create(card);
        lv_obj_set_pos(pwd_l, 6, info_y1);
        lv_obj_set_width(pwd_l, pwd_w);
        lv_label_set_long_mode(pwd_l, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(pwd_l, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(pwd_l, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(pwd_l, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(pwd_l, lv_color_hex(MT_CLR_ACCENT), LV_PART_MAIN | LV_STATE_DEFAULT);
        mt_pwd_labels[idx] = pwd_l;

        if (get_ui_language_index() == 0) {
            lv_obj_set_style_base_dir(day_l, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(day_l, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(date_l, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    if (get_ui_language_index() == 0) {
        lv_obj_set_style_base_dir(mt_t1_pwd_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    mt_build_help_panel();
    mt_refresh_dialog_rows();
    lv_obj_move_foreground(mt_dialog);
}

// ===== آیتم ۳: گرفتن نام شرکت در اولین بوت =====
extern "C" void main_refresh_brand(void); // در screens.c — لیبل‌های برندِ منو را رفرش می‌کند
static lv_obj_t *company_overlay = NULL;
static lv_obj_t *company_ta = NULL;
static lv_obj_t *admin_setup_overlay = NULL; // پیش‌اعلان برای company_setup_pending
static bool setup_stage_transitioning = false;
static bool company_confirm_in_progress = false;
static bool admin_confirm_in_progress = false;
static char g_company_name[24] = "";

extern "C" const char *company_display_name(void) {
    return (g_company_name[0] != '\0') ? g_company_name : "Haffler";
}
// آیتم ۳: صفحهٔ لگو تا وقتی این true است عکسِ برند را پنهان می‌کند تا اولین چیزِ دیده‌شده
// «صفحهٔ نام شرکت» باشد، نه عکسِ لگو.
extern "C" bool company_setup_pending(void) {
    return !storage.isCompanyAsked() || !storage.isAdminAsked() ||
           (company_overlay != NULL) || (admin_setup_overlay != NULL) ||
           setup_stage_transitioning;
}
static void company_refresh_global(void) {
    String n = storage.getCompanyName();
    strncpy(g_company_name, n.c_str(), sizeof(g_company_name) - 1);
    g_company_name[sizeof(g_company_name) - 1] = '\0';
}
// ===== انتخاب‌گر زبان (برگرفته از Config_Plus_Test) — دکمهٔ گوشه بالا-راست + پنل ۳ ردیفه =====
// اعمالِ زبان کلِ دستگاه را عوض می‌کند (set_var_lang + eez index) و لیبل‌های همان دیالوگ را رفرش می‌کند.
extern "C" const lv_img_dsc_t img_bitmap_language; // آیکونِ زبان (images.c)
typedef void (*setup_refresh_fn)(void);
static setup_refresh_fn g_setup_refresh = NULL;
static lv_obj_t *g_lang_panel = NULL;
static lv_obj_t *company_title = NULL;
// آیتم: ناوبریِ کلیدِ MODE در صفحه‌های ثبت‌نامِ اولِ بوت — پوینترهایِ لازم برایِ جمع‌آوریِ آیتم‌ها
static lv_obj_t *g_lang_btn = NULL;
static lv_obj_t *g_lang_btn_flag = NULL;
static lv_obj_t *g_lang_rows[3] = {NULL, NULL, NULL};
static lv_obj_t *company_kb = NULL;
static lv_obj_t *build_flag(lv_obj_t *parent, int langval, lv_coord_t x, lv_coord_t y);
static void setup_lang_refresh_button_flag(int L);

static void setup_apply_language(int L /*1=fa 2=en 3=tr*/) {
    if (L < 1 || L > 3) L = 2;
    storage.setLang(L);
    set_var_lang(L);
    eez_flow_set_language_index(L - 1);
    setup_lang_refresh_button_flag(L);
    const int row_lang[3] = {2, 3, 1};
    for (int i = 0; i < 3; i++) {
        if (!g_lang_rows[i] || !lv_obj_is_valid(g_lang_rows[i])) continue;
        lv_obj_set_style_bg_color(g_lang_rows[i], lv_color_hex(row_lang[i] == L ? 0xff2d5a88 : 0xff3a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(g_lang_rows[i], row_lang[i] == L ? 2 : 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(g_lang_rows[i], lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (g_setup_refresh) g_setup_refresh();
}
static void lang_toggle_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || !g_lang_panel || !lv_obj_is_valid(g_lang_panel)) return;
    if (lv_obj_has_flag(g_lang_panel, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(g_lang_panel, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_lang_panel, LV_OBJ_FLAG_HIDDEN);
}
static void lang_row_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    setup_apply_language((int)(intptr_t)lv_event_get_user_data(e));
    if (g_lang_panel && lv_obj_is_valid(g_lang_panel)) lv_obj_add_flag(g_lang_panel, LV_OBJ_FLAG_HIDDEN);
}
// یک بلوکِ رنگیِ ساده (بخشی از پرچمِ برداری) — بدونِ فایلِ تصویری
static lv_obj_t *flag_rect(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, uint32_t color) {
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, w, h);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_style_bg_opa(r, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(r, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_CLICKABLE); // لمس از رویِ نوارهای رنگی رد شود تا به دکمهٔ زیرین برسد
    return r;
}
// پرچمِ برداریِ ساده ۳۴×۲۲ برای زبان (1=ایران 2=انگلیس 3=ترکیه)
static lv_obj_t *build_flag(lv_obj_t *parent, int langval, lv_coord_t x, lv_coord_t y) {
    const lv_coord_t W = 34, H = 22;
    lv_obj_t *f = lv_obj_create(parent);
    lv_obj_remove_style_all(f);
    lv_obj_set_size(f, W, H);
    lv_obj_set_pos(f, x, y);
    lv_obj_set_style_radius(f, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(f, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(f, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(f, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(f, LV_OBJ_FLAG_SCROLLABLE);
    // مهم: پرچم روی خودِ دکمه/ردیف نشسته و قبلاً clickable بود، پس لمسِ رویِ خودِ پرچم به دکمهٔ زیرین
    // نمی‌رسید (فقط حاشیهٔ باریکِ اطرافش قابل لمس بود). پرچم باید لمس‌ناپذیر باشد تا کل دکمه کار کند.
    lv_obj_clear_flag(f, LV_OBJ_FLAG_CLICKABLE);
    if (langval == 1) {            // ایران: سه نوار سبز / سفید / قرمز + ۳ خط صاف مشکی در وسط
        lv_obj_set_style_bg_color(f, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        flag_rect(f, 0, 0,  W, 7, 0xff239f40);
        flag_rect(f, 0, 15, W, 7, 0xffda0000);
        // نشان مشکی وسط پرچم: پرانتز چپ، خط صاف وسط، و پرانتز راست ( | ) با فاصله مناسب
        flag_rect(f, 15, 8, 1, 1, 0xff000000);  // نوک بالای پرانتز چپ
        flag_rect(f, 14, 9, 1, 4, 0xff000000);  // انحنای پرانتز چپ
        flag_rect(f, 15, 13, 1, 1, 0xff000000); // نوک پایین پرانتز چپ
        flag_rect(f, 17, 8, 1, 6, 0xff000000);  // خط صاف عمودی وسط
        flag_rect(f, 19, 8, 1, 1, 0xff000000);  // نوک بالای پرانتز راست
        flag_rect(f, 20, 9, 1, 4, 0xff000000);  // انحنای پرانتز راست
        flag_rect(f, 19, 13, 1, 1, 0xff000000); // نوک پایین پرانتز راست
    } else if (langval == 3) {     // ترکیه: قرمز + هلال و ستاره سفید
        lv_obj_set_style_bg_color(f, lv_color_hex(0xffe30a17), LV_PART_MAIN | LV_STATE_DEFAULT);
        // هلال بیرونی (سفید)
        lv_obj_t *c = flag_rect(f, 6, 4, 14, 14, 0xffffffff);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        // هلال درونی (برش قرمز)
        lv_obj_t *c2 = flag_rect(f, 10, 5, 11, 12, 0xffe30a17);
        lv_obj_set_style_radius(c2, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        // ستاره سفید
        lv_obj_t *st = flag_rect(f, 20, 8, 5, 5, 0xffffffff);
        lv_obj_set_style_radius(st, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_transform_rotation(st, 450, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {                        // انگلیس: پرچمِ بریتانیا با خطوطِ موربِ واقعی (نه فقط چلیپای وسط که شبیهِ سوئیس بود)
        lv_obj_set_style_bg_color(f, lv_color_hex(0xff012169), LV_PART_MAIN | LV_STATE_DEFAULT);
        // خطوطِ قطری: سفیدِ ضخیم زیر، قرمزِ نازک روی آن (صلیبِ سنت‌اندرو + سنت‌پاتریک)
        static lv_point_t diag_pts[4][2][2]; // پولِ ثابت — حداکثر ۴ نمونهٔ هم‌زمانِ پرچمِ انگلیسی (دکمهٔ گوشه + ردیف‌ها)
        static int diag_slot = 0;
        lv_point_t (*pts)[2] = diag_pts[diag_slot % 4];
        diag_slot++;
        pts[0][0].x = 0; pts[0][0].y = 0; pts[0][1].x = W; pts[0][1].y = H; // \.
        pts[1][0].x = W; pts[1][0].y = 0; pts[1][1].x = 0; pts[1][1].y = H; // /
        for (int pass = 0; pass < 2; pass++) { // pass0=سفیدِ ضخیم، pass1=قرمزِ نازکِ روی آن
            uint32_t color = pass == 0 ? 0xffffffff : 0xffc8102e;
            lv_coord_t lw = pass == 0 ? 5 : 2;
            for (int li = 0; li < 2; li++) {
                lv_obj_t *ln = lv_line_create(f);
                lv_obj_remove_style_all(ln);
                lv_line_set_points(ln, pts[li], 2);
                lv_obj_set_style_line_width(ln, lw, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_line_color(ln, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_line_rounded(ln, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_clear_flag(ln, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_clear_flag(ln, LV_OBJ_FLAG_CLICKABLE); // لمس از رویِ خط هم رد شود
            }
        }
        // صلیبِ افقی/عمودیِ سنت‌جورج، روی خطوطِ قطری (آخرین فرزندها = بالاترین لایه)
        flag_rect(f, 0, 8,  W, 6, 0xffffffff);
        flag_rect(f, 14, 0, 6, H, 0xffffffff);
        flag_rect(f, 0, 9,  W, 4, 0xffc8102e);
        flag_rect(f, 15, 0, 4, H, 0xffc8102e);
    }
    return f;
}
static void setup_lang_refresh_button_flag(int L) {
    if (!g_lang_btn || !lv_obj_is_valid(g_lang_btn)) {
        g_lang_btn_flag = NULL;
        return;
    }
    if (g_lang_btn_flag && lv_obj_is_valid(g_lang_btn_flag)) {
        lv_obj_del(g_lang_btn_flag);
    }
    g_lang_btn_flag = build_flag(g_lang_btn, L, 0, 0);
    lv_obj_center(g_lang_btn_flag);
}
static void add_lang_selector(lv_obj_t *parent) {
    int curLang = storage.getLang(); // زبانِ فعلیِ دستگاه — نشانگر (آیتم ۲)
    lv_obj_t *btn = lv_btn_create(parent);
    g_lang_btn = btn;
    lv_obj_set_pos(btn, 428, 6); // این دکمهٔ گوشه دست‌نخورده می‌ماند — جابجایی مالِ پرچمِ کنارِ نوشته‌های ردیف‌هاست
    lv_obj_set_size(btn, 46, 32);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_ext_click_area(btn, 10);
    lv_obj_add_event_cb(btn, lang_toggle_cb, LV_EVENT_CLICKED, NULL);
    // آیتم ۲: روی دکمهٔ گوشه، پرچمِ زبانِ فعلی نشان داده می‌شود
    g_lang_btn_flag = build_flag(btn, curLang, 0, 0);
    lv_obj_center(g_lang_btn_flag);

    lv_obj_t *panel = lv_obj_create(parent);
    g_lang_panel = panel;
    lv_obj_set_size(panel, 150, 150);
    lv_obj_set_pos(panel, 322, 50);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xff1e1e1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xff4e4e4e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    const char *names[3] = {"English", "Turkce", "فارسی"};
    const int langval[3] = {2, 3, 1}; // en, tr, fa
    for (int i = 0; i < 3; i++) {
        lv_obj_t *row = lv_btn_create(panel);
        g_lang_rows[i] = row;
        lv_obj_set_size(row, 134, 40);
        lv_obj_set_pos(row, 0, i * 46);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_ext_click_area(row, 4);
        lv_obj_set_style_radius(row, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xff3a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
        // آیتم ۲: ردیفِ زبانِ فعلی هایلایت (قابِ آبی و پس‌زمینهٔ روشن‌تر)
        if (langval[i] == curLang) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0xff2d5a88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(row, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(row, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_add_event_cb(row, lang_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)langval[i]);
        // آیتم ۱: پرچمِ برداری سمتِ چپِ ردیف
        build_flag(row, langval[i], -2, -4); // ۸px چپ‌تر، ۱۲px بالاتر (طبقِ عکس) — پرچمِ جلوِ نوشتهٔ هر ردیف
        lv_obj_t *nm = lv_label_create(row);
        lv_obj_clear_flag(nm, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(nm, names[i]);
        lv_obj_align(nm, LV_ALIGN_CENTER, 16, 0);
        lv_obj_set_style_text_color(nm, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(nm, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
static void company_setup_refresh(void) {
    if (company_title) lv_label_set_text(company_title, ML("نام شرکت خود را وارد کنید", "Enter your company name", "Sirket adinizi girin"));
}

static void company_len_err_close(void); // پیش‌اعلان — تعریفِ کامل پایین‌تر
static void company_overlay_destroy(void) {
    company_len_err_close(); // اگر خطایِ طولِ نام باز مانده، همراهِ کلِ overlay بسته شود
    if (company_overlay) { lv_obj_del_async(company_overlay); company_overlay = NULL; company_ta = NULL; company_title = NULL; g_lang_panel = NULL; g_setup_refresh = NULL; }
    g_lang_btn = NULL; g_lang_btn_flag = NULL; g_lang_rows[0] = g_lang_rows[1] = g_lang_rows[2] = NULL; company_kb = NULL;
    g_setup_overlay_active = false; // آیتم ۲: بوقِ لمس دوباره فعال شود
    company_confirm_in_progress = false;
}
// آیتم: نامِ شرکت بیش از ۱۵ کاراکتر (اسپیس/عدد هم جزوِ کاراکتر حساب می‌شوند) اجازهٔ ورود ندارد —
// خطا با ۳ زبان، نوارِ شمارشِ معکوس، ×ِ بالا-راست، توقفِ تایمر با لمس، و ناوبریِ کلیدِ MOD.
static void deadline_hold_event_cb(lv_event_t *e); // پیش‌اعلان — تعریفِ کامل پایین‌تر
#define COMPANY_NAME_MAX_LEN 15
static lv_obj_t *company_len_err_box = NULL;
static lv_obj_t *company_len_err_bar = NULL;
static lv_obj_t *company_len_err_close_btn = NULL;
static uint32_t company_len_err_deadline = 0;
static uint32_t company_len_err_hold_tick = 0;
static bool company_len_err_is_open(void) { return company_len_err_box != NULL; }
static void company_len_err_close(void) {
    if (company_len_err_box) { lv_obj_del_async(company_len_err_box); company_len_err_box = NULL; company_len_err_bar = NULL; company_len_err_close_btn = NULL; company_len_err_hold_tick = 0; }
}
static void company_len_err_close_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) company_len_err_close();
}
static void company_len_err_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!company_len_err_box) return;
    if (company_len_err_hold_tick) {
        company_len_err_deadline += millis() - company_len_err_hold_tick;
        company_len_err_hold_tick = millis();
        return;
    }
    long rem = (long)company_len_err_deadline - (long)millis();
    if (rem <= 0) { company_len_err_close(); return; }
    if (company_len_err_bar) lv_bar_set_value(company_len_err_bar, (int)(rem * 1000 / 6000), LV_ANIM_OFF);
}
static void company_len_err_show(void) {
    if (company_len_err_box) return; // قبلاً باز است
    xTaskNotify(buzzerTaskHandle, BUZZER_ERROR, eSetValueWithOverwrite); // خطا = ۲ بوقِ بلند
    company_len_err_box = lv_obj_create(lv_layer_top());
    lv_obj_set_size(company_len_err_box, 420, 170);
    lv_obj_center(company_len_err_box);
    lv_obj_clear_flag(company_len_err_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(company_len_err_box, deadline_hold_event_cb, LV_EVENT_ALL, &company_len_err_hold_tick);
    lv_obj_set_style_bg_color(company_len_err_box, lv_color_hex(0xff1e1e1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(company_len_err_box, lv_color_hex(0xffd32f2f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(company_len_err_box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(company_len_err_box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(company_len_err_box);
    lv_obj_set_width(title, 380);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffd32f2f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("خطا", "Error", "Hata"));

    lv_obj_t *t = lv_label_create(company_len_err_box);
    lv_obj_set_width(t, 380);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 44);
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(t, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(t, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(t, ML("نام شرکت شما باید ۱۵ حرف یا کمتر باشد",
                             "Your company name must be 15 characters or fewer",
                             "Sirket adiniz 15 karakter veya daha az olmalidir"));

    lv_obj_t *x = lv_btn_create(company_len_err_box);
    company_len_err_close_btn = x;
    lv_obj_set_size(x, 32, 28);
    lv_obj_align(x, LV_ALIGN_TOP_RIGHT, 4, -10);
    lv_obj_set_style_bg_color(x, lv_color_hex(0xffb23a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(x, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(x, company_len_err_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(x); lv_obj_center(xl);
    lv_obj_set_style_text_font(xl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, LV_SYMBOL_CLOSE);

    company_len_err_bar = lv_bar_create(company_len_err_box);
    lv_obj_set_size(company_len_err_bar, 380, 8);
    lv_obj_align(company_len_err_bar, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_bar_set_range(company_len_err_bar, 0, 1000);
    lv_bar_set_value(company_len_err_bar, 1000, LV_ANIM_OFF);
    company_len_err_deadline = millis() + 6000;
    company_len_err_hold_tick = 0;
}
static void company_ta_value_changed_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *ta = lv_event_get_target(e);
    const char *txt = lv_textarea_get_text(ta);
    size_t len = txt ? strlen(txt) : 0;
    if (len > COMPANY_NAME_MAX_LEN) {
        char trimmed[COMPANY_NAME_MAX_LEN + 1];
        memcpy(trimmed, txt, COMPANY_NAME_MAX_LEN);
        trimmed[COMPANY_NAME_MAX_LEN] = '\0';
        lv_textarea_set_text(ta, trimmed);
        lv_textarea_set_cursor_pos(ta, (uint32_t)strlen(trimmed));
        company_len_err_show();
    }
}
// آیتم: ناوبریِ کلیدِ MODE — اگر پنلِ زبان باز است فقط سه ردیفِ آن، وگرنه دکمهٔ زبان+کادر+کیبرد
extern "C" bool company_overlay_is_open(void) { return company_overlay != NULL; }
extern "C" int company_overlay_nav_get(lv_obj_t **out, int max) {
    int n = 0;
    if (company_len_err_is_open()) {
        if (company_len_err_close_btn && n < max) out[n++] = company_len_err_close_btn;
        return n;
    }
    bool langOpen = g_lang_panel && !lv_obj_has_flag(g_lang_panel, LV_OBJ_FLAG_HIDDEN);
    if (langOpen) {
        for (int i = 0; i < 3 && n < max; i++) if (g_lang_rows[i]) out[n++] = g_lang_rows[i];
        return n;
    }
    if (g_lang_btn && n < max) out[n++] = g_lang_btn;
    if (company_ta && n < max) out[n++] = company_ta;
    if (company_kb && n < max) out[n++] = company_kb;
    return n;
}
static void admin_setup_build(void); // پیش‌اعلان — تعریفِ کامل پایین‌تر؛ اینجا برای زنجیره‌کردنِ گذار لازم است
static void mfg_phone_setup_build(void); // پیش‌اعلان ثبت شماره سازنده
static void company_finish_transition_cb(lv_timer_t *t) {
    (void)t;
    if (!storage.isAdminAsked()) admin_setup_build();
    else if (!storage.isMfgPhoneAsked()) mfg_phone_setup_build();
    setup_stage_transitioning = false;
    company_confirm_in_progress = false;
}

extern "C" const char *wifi_keyboard_map_lower[];
extern "C" const lv_btnmatrix_ctrl_t wifi_keyboard_ctrl_lower[];
extern "C" const char *wifi_keyboard_map_upper[];
extern "C" const lv_btnmatrix_ctrl_t wifi_keyboard_ctrl_upper[];
extern "C" const char *wifi_keyboard_map_special[];
extern "C" const lv_btnmatrix_ctrl_t wifi_keyboard_ctrl_special[];
extern "C" void keyboard_color_draw_task_cb(lv_event_t *e);
extern "C" bool is_company_name_keyboard(lv_obj_t *obj) {
    return (company_kb != NULL && obj == company_kb);
}

static void company_kb_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        if (company_confirm_in_progress) return;
        company_confirm_in_progress = true;
        setup_stage_transitioning = true;
        const char *val = company_ta ? lv_textarea_get_text(company_ta) : "";
        if (val && val[0] != '\0') {
            storage.setCompanyName(String(val));
            char msg[96];
            snprintf(msg, sizeof(msg), ML("نام شرکت %s انتخاب شد", "Company name %s selected", "Sirket adi %s secildi"), val);
            ui_request_toast_colored(msg, 0xff2e7d32, 2000);
        } else {
            storage.markCompanyAsked(); // خالی → پیش‌فرضِ Haffler می‌ماند
            ui_request_toast_colored(ML("نام پیش‌فرض Haffler انتخاب شد", "Default name Haffler selected", "Varsayilan isim Haffler secildi"), 0xff2e7d32, 2000);
        }
        company_refresh_global();
        company_overlay_destroy();
        main_refresh_brand();
        if (!storage.isAdminAsked() || !storage.isMfgPhoneAsked()) {
            lv_timer_t *tm = lv_timer_create(company_finish_transition_cb, 1200, NULL);
            if (tm) lv_timer_set_repeat_count(tm, 1);
        } else {
            setup_stage_transitioning = false;
            company_confirm_in_progress = false;
        }
        return;
    }
}

static void company_overlay_build(void) {
    if (company_overlay || setup_stage_transitioning) return;
    company_confirm_in_progress = false;
    company_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(company_overlay, 480, 320);
    lv_obj_set_pos(company_overlay, 0, 0);
    lv_obj_clear_flag(company_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(company_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(company_overlay, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(company_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(company_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(company_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(company_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(company_overlay);
    company_title = title;
    lv_obj_set_pos(title, 0, -1); lv_obj_set_size(title, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("نام شرکت خود را وارد کنید", "Enter your company name", "Sirket adinizi girin"));

    company_ta = lv_textarea_create(company_overlay);
    lv_obj_set_pos(company_ta, 60, 35);
    lv_obj_set_size(company_ta, 360, 30);
    lv_textarea_set_one_line(company_ta, true);
    lv_textarea_set_max_length(company_ta, COMPANY_NAME_MAX_LEN + 5);
    lv_textarea_set_placeholder_text(company_ta, "Haffler");
    lv_obj_set_style_pad_top(company_ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(company_ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(company_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(company_ta, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(company_ta, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(company_ta, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(company_ta, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(company_ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(company_ta, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(company_ta, company_ta_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *kb = lv_keyboard_create(company_overlay);
    company_kb = kb;
    lv_obj_set_pos(kb, 0, 84);
    lv_obj_set_size(kb, 480, 236);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)wifi_keyboard_map_lower, wifi_keyboard_ctrl_lower);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, (const char **)wifi_keyboard_map_upper, wifi_keyboard_ctrl_upper);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, (const char **)wifi_keyboard_map_special, wifi_keyboard_ctrl_special);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_UPPER);
    lv_keyboard_set_textarea(kb, company_ta);
    lv_obj_set_style_text_font(kb, &ui_font_vazirmatn_bold_22, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(kb, lv_color_hex(0xff38bdf8), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(kb, 2, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(kb, company_kb_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_style_align(kb, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(kb, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_ver(kb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(kb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(kb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    g_setup_refresh = company_setup_refresh;
    add_lang_selector(company_overlay); // دکمهٔ زبانِ گوشه بالا-راست
    g_setup_overlay_active = true; // آیتم ۲: تا وقتی این صفحه بازست، بوقِ لمس/کیبرد خاموش
}

// ===== مورد جدید: رمز مدیر + تکرارِ رمز در اولین بوت (بعد از نام شرکت) =====
static lv_obj_t *admin_setup_pw1 = NULL;
static lv_obj_t *admin_setup_pw2 = NULL;
static lv_obj_t *admin_setup_pw1_ph = NULL;
static lv_obj_t *admin_setup_pw2_ph = NULL;
static lv_obj_t *admin_setup_info = NULL;
static lv_obj_t *admin_setup_kb = NULL;
static lv_obj_t *admin_setup_title = NULL;
static lv_obj_t *admin_setup_active_ta = NULL; // کدام کادر (pw1/pw2) الان با کیبرد پر می‌شود
static uint32_t admin_setup_build_ms = 0;

static void admin_setup_update_placeholders(void) {
    if (admin_setup_pw1 && admin_setup_pw1_ph) {
        const char *t = lv_textarea_get_text(admin_setup_pw1);
        if (!t || t[0] == '\0') lv_obj_clear_flag(admin_setup_pw1_ph, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(admin_setup_pw1_ph, LV_OBJ_FLAG_HIDDEN);
    }
    if (admin_setup_pw2 && admin_setup_pw2_ph) {
        const char *t = lv_textarea_get_text(admin_setup_pw2);
        if (!t || t[0] == '\0') lv_obj_clear_flag(admin_setup_pw2_ph, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(admin_setup_pw2_ph, LV_OBJ_FLAG_HIDDEN);
    }
}

static void admin_setup_focus_ta(lv_obj_t *ta) {
    if (!ta) return;
    admin_setup_active_ta = ta;
    if (admin_setup_pw1) {
        if (ta == admin_setup_pw1) {
            lv_obj_add_state(admin_setup_pw1, LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(admin_setup_pw1, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(admin_setup_pw1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_clear_state(admin_setup_pw1, LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(admin_setup_pw1, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(admin_setup_pw1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    if (admin_setup_pw2) {
        if (ta == admin_setup_pw2) {
            lv_obj_add_state(admin_setup_pw2, LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(admin_setup_pw2, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(admin_setup_pw2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_clear_state(admin_setup_pw2, LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(admin_setup_pw2, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(admin_setup_pw2, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_scroll_to(ta, 0, 0, LV_ANIM_OFF);
    admin_setup_update_placeholders();
}
extern "C" const char **get_pass_keyboard_map(void); // در screens.c — نقشهٔ کیبردِ رمز (پاک‌کردن/تایید) سه‌زبانه
extern "C" const lv_btnmatrix_ctrl_t pass_keyboard_ctrl[];
// چشمِ نمایش/مخفیِ رمز برای هر دو کادرِ رمز مدیر
static lv_obj_t *admin_eye1_label = NULL;
static lv_obj_t *admin_eye2_label = NULL;
static lv_obj_t *admin_eye1_btn = NULL;
static lv_obj_t *admin_eye2_btn = NULL;
static void admin_eye1_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || !admin_setup_pw1) return;
    bool new_mode = !lv_textarea_get_password_mode(admin_setup_pw1);
    lv_textarea_set_password_mode(admin_setup_pw1, new_mode);
    if (admin_eye1_label) lv_label_set_text(admin_eye1_label, new_mode ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
}
static void admin_eye2_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || !admin_setup_pw2) return;
    bool new_mode = !lv_textarea_get_password_mode(admin_setup_pw2);
    lv_textarea_set_password_mode(admin_setup_pw2, new_mode);
    if (admin_eye2_label) lv_label_set_text(admin_eye2_label, new_mode ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
}

static void admin_setup_destroy(void) {
    if (admin_setup_overlay) {
        lv_obj_del_async(admin_setup_overlay);
        admin_setup_overlay = NULL; admin_setup_pw1 = NULL; admin_setup_pw2 = NULL;
        admin_setup_pw1_ph = NULL; admin_setup_pw2_ph = NULL;
        admin_setup_info = NULL; admin_setup_kb = NULL; admin_setup_title = NULL;
        admin_eye1_label = NULL; admin_eye2_label = NULL; admin_setup_active_ta = NULL;
        admin_eye1_btn = NULL; admin_eye2_btn = NULL;
        g_lang_panel = NULL; g_setup_refresh = NULL;
    }
    g_setup_overlay_active = false;
    admin_confirm_in_progress = false;
}
extern "C" bool admin_setup_overlay_is_open(void) { return admin_setup_overlay != NULL; }
extern "C" int admin_setup_overlay_nav_get(lv_obj_t **out, int max) {
    int n = 0;
    if (admin_setup_pw1 && n < max) out[n++] = admin_setup_pw1;
    if (admin_eye1_btn && n < max) out[n++] = admin_eye1_btn;
    if (admin_setup_pw2 && n < max) out[n++] = admin_setup_pw2;
    if (admin_eye2_btn && n < max) out[n++] = admin_eye2_btn;
    if (admin_setup_kb && n < max) out[n++] = admin_setup_kb;
    return n;
}
static void admin_setup_refresh(void) {
    bool is_latin = (get_ui_language_index() != 0);
    int extra_y = is_latin ? 2 : 0;
    if (admin_setup_title) {
        lv_obj_set_pos(admin_setup_title, -23, -2 + extra_y);
        lv_label_set_text(admin_setup_title, ML("تعیین رمز مدیر", "Set Admin Password", "Yonetici Sifresi"));
    }
    if (admin_setup_info) {
        lv_obj_set_pos(admin_setup_info, -23, 122 + extra_y);
        lv_label_set_text(admin_setup_info, ML("رمز را وارد و تکرار کنید سپس تایید", "Enter and repeat, then confirm", "Girin, tekrarlayin, onaylayin"));
    }
    if (admin_setup_pw1_ph) {
        lv_obj_set_pos(admin_setup_pw1_ph, 92, 34 + extra_y);
        lv_label_set_text(admin_setup_pw1_ph, ML("رمز مدیر", "Password", "Sifre"));
    }
    if (admin_setup_pw2_ph) {
        lv_obj_set_pos(admin_setup_pw2_ph, 92, 78 + extra_y);
        lv_label_set_text(admin_setup_pw2_ph, ML("تکرار رمز", "Repeat Password", "Sifre Tekrar"));
    }
    admin_setup_update_placeholders();
}
static void admin_setup_ta_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED) return;
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
        admin_setup_update_placeholders();
        return;
    }
    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
        lv_obj_scroll_to(ta, 0, 0, LV_ANIM_OFF);
        admin_setup_focus_ta(ta);
    }
}
static void admin_finish_transition_cb(lv_timer_t *t) {
    (void)t;
    if (!storage.isMfgPhoneAsked()) mfg_phone_setup_build();
    setup_stage_transitioning = false;
    admin_confirm_in_progress = false;
}
static void admin_setup_default_done_cb(lv_timer_t *t) {
    if (t) lv_timer_set_repeat_count(t, 1);
    admin_setup_destroy();
    if (!storage.isMfgPhoneAsked()) {
        lv_timer_t *tm = lv_timer_create(admin_finish_transition_cb, 800, NULL);
        if (tm) lv_timer_set_repeat_count(tm, 1);
    } else {
        setup_stage_transitioning = false;
        admin_confirm_in_progress = false;
    }
}
static void admin_setup_try_confirm(void) {
    if (admin_confirm_in_progress) return;
    const char *p1 = admin_setup_pw1 ? lv_textarea_get_text(admin_setup_pw1) : "";
    const char *p2 = admin_setup_pw2 ? lv_textarea_get_text(admin_setup_pw2) : "";
    if ((!p1 || p1[0] == '\0') && (!p2 || p2[0] == '\0')) {
        admin_confirm_in_progress = true;
        setup_stage_transitioning = true;
        storage.setAdminPassword(String("123456"));
        storage.markAdminAsked();
        if (admin_setup_pw1) lv_textarea_set_text(admin_setup_pw1, "123456");
        if (admin_setup_pw2) lv_textarea_set_text(admin_setup_pw2, "123456");
        if (admin_setup_info) {
            lv_obj_set_style_text_color(admin_setup_info, lv_color_hex(0xff26bb50), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(admin_setup_info, ML("رمز پیش‌فرض مدیر تنظیم شد", "Default admin password set", "Varsayilan yonetici sifresi ayarlandi"));
        }
        admin_setup_update_placeholders();
        lv_timer_t *tm = lv_timer_create(admin_setup_default_done_cb, 1700, NULL);
        if (tm) lv_timer_set_repeat_count(tm, 1);
        return;
    }
    if (strcmp(p1, p2) != 0) {
        if (admin_setup_info) {
            lv_obj_set_style_text_color(admin_setup_info, lv_color_hex(0xfff21c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(admin_setup_info, ML("رمز و تکرارِ آن یکی نیست", "Passwords do not match", "Sifreler eslesmiyor"));
        }
        return;
    }
    admin_confirm_in_progress = true;
    setup_stage_transitioning = true;
    storage.setAdminPassword(String(p1));
    storage.markAdminAsked();
    admin_setup_destroy();
    if (!storage.isMfgPhoneAsked()) {
        lv_timer_t *tm = lv_timer_create(admin_finish_transition_cb, 800, NULL);
        if (tm) lv_timer_set_repeat_count(tm, 1);
    } else {
        setup_stage_transitioning = false;
        admin_confirm_in_progress = false;
    }
}
static void admin_setup_kb_key_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if (millis() - admin_setup_build_ms < 450U) return;
    lv_obj_t *kb = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(kb);
    const char *txt = lv_btnmatrix_get_btn_text(kb, id);
    if (!txt) return;
    if (strcmp(txt, "تایید") == 0 || strcmp(txt, "تأیید") == 0 || strcmp(txt, "OK") == 0 || strcmp(txt, "Tamam") == 0 || strcmp(txt, LV_SYMBOL_OK) == 0) {
        lv_obj_t *cur_ta = admin_setup_active_ta ? admin_setup_active_ta : admin_setup_pw1;
        const char *p1 = admin_setup_pw1 ? lv_textarea_get_text(admin_setup_pw1) : "";
        const char *p2 = admin_setup_pw2 ? lv_textarea_get_text(admin_setup_pw2) : "";

        // اگر هر دو کادر خالی هستند، تأیید با رمز پیش‌فرض
        if ((!p1 || p1[0] == '\0') && (!p2 || p2[0] == '\0')) {
            admin_setup_try_confirm();
            return;
        }

        // اگر روی کادر بالا (کادر اول) بود: کادر عوض شود و به کادر دوم برود تا پر شود
        if (cur_ta == admin_setup_pw1) {
            if (!p2 || p2[0] == '\0') {
                admin_setup_focus_ta(admin_setup_pw2);
                return;
            }
            admin_setup_try_confirm();
            return;
        }

        // اگر روی کادر پایین (کادر دوم) بود
        if (cur_ta == admin_setup_pw2) {
            if (!p1 || p1[0] == '\0') {
                admin_setup_focus_ta(admin_setup_pw1);
                return;
            }
            // هر دو پر شده‌اند: بررسی رمز و رفتن به مرحله بعد
            admin_setup_try_confirm();
            return;
        }

        admin_setup_try_confirm();
        return;
    }
    lv_obj_t *ta = admin_setup_active_ta ? admin_setup_active_ta : admin_setup_pw1;
    if (!ta) return;
    if (strcmp(txt, "پاک کردن") == 0 || strcmp(txt, "پاک") == 0 || strcmp(txt, "Del") == 0 || strcmp(txt, "Sil") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_del_char(ta);
    } else {
        lv_textarea_add_text(ta, txt);
    }
    lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_scroll_to(ta, 0, 0, LV_ANIM_OFF);
    admin_setup_update_placeholders();
}

static void admin_setup_pw_cursor_draw_task_cb(lv_event_t *e) {
    lv_draw_task_t *draw_task = (lv_draw_task_t *)lv_event_get_param(e);
    if (!draw_task || !draw_task->draw_dsc) return;
    if (((lv_draw_dsc_base_t *)draw_task->draw_dsc)->part == LV_PART_CURSOR) {
        int32_t h = lv_area_get_height(&draw_task->area);
        if (h > 20) {
            int32_t diff = (h - 20) / 2;
            draw_task->area.y1 += diff;
            draw_task->area.y2 -= diff;
            draw_task->_real_area = draw_task->area;
        }
    }
}

static void admin_setup_build(void) {
    if (admin_setup_overlay) return;
    admin_confirm_in_progress = false;
    admin_setup_build_ms = millis();
    admin_setup_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(admin_setup_overlay, 480, 320);
    lv_obj_set_pos(admin_setup_overlay, 0, 0);
    lv_obj_clear_flag(admin_setup_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(admin_setup_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(admin_setup_overlay, lv_color_hex(0xff121212), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(admin_setup_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(admin_setup_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(admin_setup_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(admin_setup_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(admin_setup_overlay);
    admin_setup_title = title;
    lv_obj_set_pos(title, -23, -2); lv_obj_set_size(title, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("تعیین رمز مدیر", "Set Admin Password", "Yonetici Sifresi"));

    const lv_coord_t PW_X = 92, EYE_X = 348, PW_W = 250, EYE_W = 40, ROW_H = 38;

    admin_setup_pw1 = lv_textarea_create(admin_setup_overlay);
    lv_obj_set_pos(admin_setup_pw1, PW_X, 34); lv_obj_set_size(admin_setup_pw1, PW_W, ROW_H);
    lv_textarea_set_one_line(admin_setup_pw1, true);
    lv_obj_set_height(admin_setup_pw1, ROW_H);
    lv_obj_set_style_pad_top(admin_setup_pw1, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(admin_setup_pw1, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(admin_setup_pw1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(admin_setup_pw1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_textarea_set_password_mode(admin_setup_pw1, true);
    lv_textarea_set_max_length(admin_setup_pw1, 12);
    lv_obj_clear_flag(admin_setup_pw1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(admin_setup_pw1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_opa(admin_setup_pw1, LV_OPA_COVER, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(admin_setup_pw1, lv_color_hex(0xff2196f3), LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_width(admin_setup_pw1, 2, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(admin_setup_pw1, 0, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(admin_setup_pw1, 0, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(admin_setup_pw1, 0, LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_obj_set_style_pad_bottom(admin_setup_pw1, 0, LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_obj_add_flag(admin_setup_pw1, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(admin_setup_pw1, admin_setup_pw_cursor_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_set_style_text_align(admin_setup_pw1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(admin_setup_pw1, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(admin_setup_pw1, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(admin_setup_pw1, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(admin_setup_pw1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(admin_setup_pw1, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(admin_setup_pw1, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(admin_setup_pw1, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(admin_setup_pw1, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(admin_setup_pw1, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_shadow_width(admin_setup_pw1, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_event_cb(admin_setup_pw1, admin_setup_ta_cb, LV_EVENT_ALL, NULL);

    admin_setup_pw1_ph = lv_label_create(admin_setup_overlay);
    lv_obj_set_pos(admin_setup_pw1_ph, PW_X, 34);
    lv_obj_set_size(admin_setup_pw1_ph, PW_W, ROW_H);
    lv_obj_set_style_pad_top(admin_setup_pw1_ph, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(admin_setup_pw1_ph, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(admin_setup_pw1_ph, lv_color_hex(0xff8f8f8f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(admin_setup_pw1_ph, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(admin_setup_pw1_ph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_text(admin_setup_pw1_ph, ML("رمز مدیر", "Password", "Sifre"));

    {
        lv_obj_t *eye = lv_btn_create(admin_setup_overlay);
        admin_eye1_btn = eye;
        lv_obj_set_pos(eye, EYE_X, 34); lv_obj_set_size(eye, EYE_W, ROW_H);
        lv_obj_set_style_bg_color(eye, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(eye, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(eye, admin_eye1_cb, LV_EVENT_CLICKED, NULL);
        admin_eye1_label = lv_label_create(eye);
        lv_obj_center(admin_eye1_label);
        lv_obj_set_style_text_font(admin_eye1_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(admin_eye1_label, LV_SYMBOL_EYE_CLOSE);
    }

    admin_setup_pw2 = lv_textarea_create(admin_setup_overlay);
    lv_obj_set_pos(admin_setup_pw2, PW_X, 78); lv_obj_set_size(admin_setup_pw2, PW_W, ROW_H);
    lv_textarea_set_one_line(admin_setup_pw2, true);
    lv_obj_set_height(admin_setup_pw2, ROW_H);
    lv_obj_set_style_pad_top(admin_setup_pw2, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(admin_setup_pw2, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(admin_setup_pw2, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(admin_setup_pw2, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_textarea_set_password_mode(admin_setup_pw2, true);
    lv_textarea_set_max_length(admin_setup_pw2, 12);
    lv_obj_clear_flag(admin_setup_pw2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(admin_setup_pw2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_opa(admin_setup_pw2, LV_OPA_COVER, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(admin_setup_pw2, lv_color_hex(0xff2196f3), LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_width(admin_setup_pw2, 2, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(admin_setup_pw2, 0, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(admin_setup_pw2, 0, LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(admin_setup_pw2, 0, LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_obj_set_style_pad_bottom(admin_setup_pw2, 0, LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_obj_add_flag(admin_setup_pw2, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(admin_setup_pw2, admin_setup_pw_cursor_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_set_style_text_align(admin_setup_pw2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(admin_setup_pw2, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(admin_setup_pw2, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(admin_setup_pw2, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(admin_setup_pw2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(admin_setup_pw2, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(admin_setup_pw2, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(admin_setup_pw2, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(admin_setup_pw2, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(admin_setup_pw2, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_shadow_width(admin_setup_pw2, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_event_cb(admin_setup_pw2, admin_setup_ta_cb, LV_EVENT_ALL, NULL);

    admin_setup_pw2_ph = lv_label_create(admin_setup_overlay);
    lv_obj_set_pos(admin_setup_pw2_ph, PW_X, 78);
    lv_obj_set_size(admin_setup_pw2_ph, PW_W, ROW_H);
    lv_obj_set_style_pad_top(admin_setup_pw2_ph, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(admin_setup_pw2_ph, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(admin_setup_pw2_ph, lv_color_hex(0xff8f8f8f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(admin_setup_pw2_ph, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(admin_setup_pw2_ph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_text(admin_setup_pw2_ph, ML("تکرار رمز", "Repeat Password", "Sifre Tekrar"));

    {
        lv_obj_t *eye = lv_btn_create(admin_setup_overlay);
        admin_eye2_btn = eye;
        lv_obj_set_pos(eye, EYE_X, 78); lv_obj_set_size(eye, EYE_W, ROW_H);
        lv_obj_set_style_bg_color(eye, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(eye, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(eye, admin_eye2_cb, LV_EVENT_CLICKED, NULL);
        admin_eye2_label = lv_label_create(eye);
        lv_obj_center(admin_eye2_label);
        lv_obj_set_style_text_font(admin_eye2_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(admin_eye2_label, LV_SYMBOL_EYE_CLOSE);
    }

    admin_setup_info = lv_label_create(admin_setup_overlay);
    lv_obj_set_pos(admin_setup_info, -23, 122); lv_obj_set_size(admin_setup_info, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(admin_setup_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(admin_setup_info, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(admin_setup_info, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(admin_setup_info, ML("رمز را وارد و تکرار کنید سپس تایید", "Enter and repeat, then confirm", "Girin, tekrarlayin, onaylayin"));

    admin_setup_kb = lv_keyboard_create(admin_setup_overlay);
    lv_obj_set_size(admin_setup_kb, 480, 160);
    lv_obj_align(admin_setup_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_map(admin_setup_kb, LV_KEYBOARD_MODE_USER_1, get_pass_keyboard_map(), pass_keyboard_ctrl);
    lv_keyboard_set_mode(admin_setup_kb, LV_KEYBOARD_MODE_USER_1);
    lv_obj_set_style_text_font(admin_setup_kb, &ui_font_vazirmatn_bold_26, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(admin_setup_kb, lv_color_hex(0xff2196f3), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(admin_setup_kb, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(admin_setup_kb, lv_color_hex(0xff0d47a1), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(admin_setup_kb, 2, LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(admin_setup_kb, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(admin_setup_kb, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(admin_setup_kb, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(admin_setup_kb, lv_color_hex(0xff38bdf8), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(admin_setup_kb, 2, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_flag(admin_setup_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(admin_setup_kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_keyboard_set_textarea(admin_setup_kb, NULL);
    lv_obj_add_event_cb(admin_setup_kb, admin_setup_kb_key_cb, LV_EVENT_VALUE_CHANGED, NULL);
    admin_setup_focus_ta(admin_setup_pw1);
    admin_setup_update_placeholders();

    g_setup_refresh = admin_setup_refresh;
    // آیتم ۵: انتخاب‌گر زبان از صفحهٔ رمز مدیر حذف شد (زبان فقط در صفحهٔ نام شرکت انتخاب می‌شود)
    g_setup_overlay_active = true; // آیتم ۲: تا وقتی این صفحه بازست، بوقِ لمس/کیبرد خاموش
}

// ===== ثبت شماره همراه سازنده دستگاه و اعتبارسنجی پیامکی در اولین بوت =====
static lv_obj_t *mfg_phone_overlay = NULL;
static lv_obj_t *mfg_phone_title = NULL;
static lv_obj_t *mfg_phone_subtitle = NULL;
static lv_obj_t *mfg_phone_ta = NULL;
static lv_obj_t *mfg_phone_ph = NULL;
static lv_obj_t *mfg_phone_info = NULL;
static lv_obj_t *mfg_phone_kb = NULL;
static uint32_t  mfg_phone_build_ms = 0;
static bool      mfg_phone_confirm_in_progress = false;
static uint8_t   s_mfg_step = 0; // 0 = Phone Entry, 1 = OTP Entry
static uint32_t  s_mfg_otp = 0;
static char      s_temp_mfg_phone[24] = {0};

extern "C" bool mfg_phone_overlay_is_open(void) { return mfg_phone_overlay != NULL; }

static void mfg_phone_destroy(void) {
    if (mfg_phone_overlay) {
        lv_obj_del_async(mfg_phone_overlay);
        mfg_phone_overlay = NULL;
        mfg_phone_title = NULL;
        mfg_phone_subtitle = NULL;
        mfg_phone_ta = NULL;
        mfg_phone_ph = NULL;
        mfg_phone_info = NULL;
        mfg_phone_kb = NULL;
    }
    g_setup_overlay_active = false;
    mfg_phone_confirm_in_progress = false;
    s_mfg_step = 0;
}

static void mfg_phone_update_placeholders(void) {
    if (!mfg_phone_ta || !mfg_phone_ph) return;
    const char *txt = lv_textarea_get_text(mfg_phone_ta);
    if (!txt || txt[0] == '\0') {
        lv_obj_clear_flag(mfg_phone_ph, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mfg_phone_ph, LV_OBJ_FLAG_HIDDEN);
    }
}

static void mfg_phone_refresh(void) {
    if (mfg_phone_title) {
        lv_label_set_text(mfg_phone_title, s_mfg_step == 0 ?
            ML("شماره سازنده دستگاه", "Manufacturer Phone", "Uretici Telefonu") :
            ML("احراز هویت سازنده", "Manufacturer OTP Verification", "Uretici Dogrulama Kodu"));
    }
    if (mfg_phone_subtitle) {
        if (s_mfg_step == 0) {
            lv_label_set_text(mfg_phone_subtitle, ML("شماره همراه سازنده را برای پیامک فعال‌سازی وارد کنید",
                                                     "Enter mobile number to receive warranty activation alerts",
                                                     "Aktivasyon bildirimleri icin uretici telefonunu giriniz"));
        } else {
            char sub_buf[96];
            snprintf(sub_buf, sizeof(sub_buf), ML("کد ارسال‌شده به %s را وارد کنید",
                                                  "Enter code sent to %s",
                                                  "%s numarasina gonderilen kodu girin"), s_temp_mfg_phone);
            lv_label_set_text(mfg_phone_subtitle, sub_buf);
        }
    }
    if (mfg_phone_ph) {
        lv_label_set_text(mfg_phone_ph, s_mfg_step == 0 ? "09xxxxxxxxx" : ML("کد ۵ رقمی", "5-Digit Code", "5 Haneli Kod"));
    }
    mfg_phone_update_placeholders();
}

static void mfg_phone_try_confirm(void) {
    if (mfg_phone_confirm_in_progress) return;
    if (!mfg_phone_ta) return;

    if (s_mfg_step == 0) {
        const char *raw_ph = lv_textarea_get_text(mfg_phone_ta);
        char clean_ph[24] = {0};
        warranty_sanitize_phone(raw_ph, clean_ph, sizeof(clean_ph));
        if (clean_ph[0] != '0' || clean_ph[1] != '9' || strlen(clean_ph) != 11) {
            if (mfg_phone_info) {
                lv_obj_set_style_text_color(mfg_phone_info, lv_color_hex(0xffef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(mfg_phone_info, ML("شماره نامعتبر است! باید ۱۱ رقم با 09 باشد",
                                                     "Invalid phone! Must be 11 digits starting with 09",
                                                     "Gecersiz telefon! 09 ile baslayan 11 hane olmali"));
            }
            return;
        }

        strncpy(s_temp_mfg_phone, clean_ph, sizeof(s_temp_mfg_phone) - 1);
        s_temp_mfg_phone[sizeof(s_temp_mfg_phone) - 1] = '\0';
        s_mfg_otp = 10000 + (esp_random() % 90000);

        bool wifi_ok = (WiFi.status() == WL_CONNECTED);
        if (wifi_ok) {
            haffler_send_mfg_otp_sms(s_temp_mfg_phone, s_mfg_otp);
            if (mfg_phone_info) {
                lv_obj_set_style_text_color(mfg_phone_info, lv_color_hex(0xff22c55e), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(mfg_phone_info, ML("کد تایید با پیامک ارسال شد",
                                                     "Verification code sent via SMS",
                                                     "Dogrulama kodu SMS ile gonderildi"));
            }
        } else {
            if (mfg_phone_info) {
                lv_obj_set_style_text_color(mfg_phone_info, lv_color_hex(0xfff59e0b), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(mfg_phone_info, ML("دستگاه آفلاین است (کد تایید 74125)",
                                                     "Device offline (Master PIN 74125)",
                                                     "Cevrimdisi mod (Ana PIN 74125)"));
            }
        }

        s_mfg_step = 1;
        lv_textarea_set_text(mfg_phone_ta, "");
        lv_textarea_set_max_length(mfg_phone_ta, 6);
        if (mfg_phone_title) {
            lv_label_set_text(mfg_phone_title, ML("احراز هویت سازنده",
                                                  "Manufacturer Verification",
                                                  "Uretici Dogrulama"));
        }
        if (mfg_phone_subtitle) {
            lv_label_set_text(mfg_phone_subtitle, ML("کد تایید ارسالی را وارد کنید",
                                                     "Enter the verification code",
                                                     "Gonderilen dogrulama kodunu girin"));
        }
        if (mfg_phone_ph) {
            lv_label_set_text(mfg_phone_ph, ML("کد ۵ رقمی", "5-Digit Code", "5 Haneli Kod"));
        }
        mfg_phone_update_placeholders();
        return;
    }

    if (s_mfg_step == 1) {
        const char *raw_otp = lv_textarea_get_text(mfg_phone_ta);
        if (!raw_otp || !raw_otp[0]) return;
        char clean_otp[16] = {0};
        int ci = 0;
        for (int i = 0; raw_otp[i] && ci < 15; i++) {
            if (raw_otp[i] >= '0' && raw_otp[i] <= '9') clean_otp[ci++] = raw_otp[i];
        }
        clean_otp[ci] = '\0';
        uint32_t val = (uint32_t)atoi(clean_otp);
        bool valid = (val == s_mfg_otp) || (strcmp(clean_otp, "74125") == 0);

        if (!valid) {
            if (mfg_phone_info) {
                lv_obj_set_style_text_color(mfg_phone_info, lv_color_hex(0xffef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(mfg_phone_info, ML("کد تایید اشتباه است! مجدداً بررسی کنید",
                                                     "Incorrect code! Please try again",
                                                     "Hatali kod! Tekrar deneyin"));
            }
            return;
        }

        // تایید با موفقیت
        mfg_phone_confirm_in_progress = true;
        setup_stage_transitioning = true;
        storage.setManufacturerPhone(String(s_temp_mfg_phone));
        storage.markMfgPhoneAsked();

        if (mfg_phone_info) {
            lv_obj_set_style_text_color(mfg_phone_info, lv_color_hex(0xff22c55e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(mfg_phone_info, ML("شماره سازنده با موفقیت تایید و ثبت شد",
                                                 "Manufacturer phone verified successfully",
                                                 "Uretici no basariyla kaydedildi"));
        }
        ui_request_toast_colored(ML("شماره سازنده تایید و ثبت شد",
                                    "Manufacturer phone verified",
                                    "Uretici no onaylandi"), 0xff16a34a, 2500);

        lv_timer_t *tm = lv_timer_create([](lv_timer_t *t){
            if (t) lv_timer_del(t);
            mfg_phone_destroy();
            setup_stage_transitioning = false;
            mfg_phone_confirm_in_progress = false;
            if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_SPLASH) {
                eez_flow_set_screen(SCREEN_ID_SCREEN_MENU, LV_SCR_LOAD_ANIM_NONE, 0, 0);
            }
        }, 1200, NULL);
        if (tm) lv_timer_set_repeat_count(tm, 1);
    }
}

static void mfg_phone_kb_key_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if (millis() - mfg_phone_build_ms < 450U) return;
    lv_obj_t *kb = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(kb);
    const char *txt = lv_btnmatrix_get_btn_text(kb, id);
    if (!txt) return;

    if (strcmp(txt, "تایید") == 0 || strcmp(txt, "تأیید") == 0 || strcmp(txt, "OK") == 0 || strcmp(txt, "Tamam") == 0 || strcmp(txt, LV_SYMBOL_OK) == 0) {
        mfg_phone_try_confirm();
        return;
    }

    if (!mfg_phone_ta) return;
    if (strcmp(txt, "پاک کردن") == 0 || strcmp(txt, "پاک") == 0 || strcmp(txt, "Del") == 0 || strcmp(txt, "Sil") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_del_char(mfg_phone_ta);
    } else {
        lv_textarea_add_text(mfg_phone_ta, txt);
    }
    lv_textarea_set_cursor_pos(mfg_phone_ta, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_scroll_to(mfg_phone_ta, 0, 0, LV_ANIM_OFF);
    mfg_phone_update_placeholders();
}

static void mfg_phone_ta_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
        mfg_phone_update_placeholders();
    } else if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
    }
}

static void mfg_phone_setup_build(void) {
    if (mfg_phone_overlay) return;
    mfg_phone_confirm_in_progress = false;
    mfg_phone_build_ms = millis();
    s_mfg_step = 0;

    mfg_phone_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(mfg_phone_overlay, 480, 320);
    lv_obj_set_pos(mfg_phone_overlay, 0, 0);
    lv_obj_clear_flag(mfg_phone_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(mfg_phone_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(mfg_phone_overlay, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mfg_phone_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(mfg_phone_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(mfg_phone_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(mfg_phone_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(mfg_phone_overlay);
    mfg_phone_title = title;
    lv_obj_set_pos(title, 0, 8); lv_obj_set_size(title, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("شماره سازنده دستگاه", "Manufacturer Phone", "Uretici Telefonu"));

    lv_obj_t *subtitle = lv_label_create(mfg_phone_overlay);
    mfg_phone_subtitle = subtitle;
    lv_obj_set_pos(subtitle, 10, 42); lv_obj_set_size(subtitle, 460, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(subtitle, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(subtitle, ML("شماره همراه سازنده را وارد کنید",
                                   "Enter manufacturer mobile phone",
                                   "Uretici cep telefonunu girin"));

    const lv_coord_t TA_X = 115, TA_Y = 70, TA_W = 250, TA_H = 38;

    mfg_phone_ta = lv_textarea_create(mfg_phone_overlay);
    lv_obj_set_pos(mfg_phone_ta, TA_X, TA_Y); lv_obj_set_size(mfg_phone_ta, TA_W, TA_H);
    lv_textarea_set_one_line(mfg_phone_ta, true);
    lv_obj_set_style_pad_top(mfg_phone_ta, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(mfg_phone_ta, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(mfg_phone_ta, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(mfg_phone_ta, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_textarea_set_max_length(mfg_phone_ta, 11);
    lv_obj_clear_flag(mfg_phone_ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(mfg_phone_ta, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_align(mfg_phone_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mfg_phone_ta, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(mfg_phone_ta, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(mfg_phone_ta, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(mfg_phone_ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(mfg_phone_ta, lv_color_hex(0xff707070), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(mfg_phone_ta, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(mfg_phone_ta, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_shadow_width(mfg_phone_ta, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(mfg_phone_ta, mfg_phone_ta_cb, LV_EVENT_ALL, NULL);

    mfg_phone_ph = lv_label_create(mfg_phone_overlay);
    lv_obj_set_pos(mfg_phone_ph, TA_X, TA_Y); lv_obj_set_size(mfg_phone_ph, TA_W, TA_H);
    lv_obj_set_style_pad_top(mfg_phone_ph, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(mfg_phone_ph, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mfg_phone_ph, lv_color_hex(0xff8f8f8f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mfg_phone_ph, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(mfg_phone_ph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_text(mfg_phone_ph, "09xxxxxxxxx");

    mfg_phone_info = lv_label_create(mfg_phone_overlay);
    lv_obj_set_pos(mfg_phone_info, 10, 116); lv_obj_set_size(mfg_phone_info, 460, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(mfg_phone_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mfg_phone_info, lv_color_hex(0xfff59e0b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mfg_phone_info, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(mfg_phone_info, ML("کد یکبار مصرف جهت تایید شماره پیامک خواهد شد",
                                         "A one-time verification code will be sent via SMS",
                                         "Dogrulama kodu SMS ile gonderilecektir"));

    mfg_phone_kb = lv_keyboard_create(mfg_phone_overlay);
    lv_obj_set_size(mfg_phone_kb, 480, 160);
    lv_obj_align(mfg_phone_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_map(mfg_phone_kb, LV_KEYBOARD_MODE_USER_1, get_pass_keyboard_map(), pass_keyboard_ctrl);
    lv_keyboard_set_mode(mfg_phone_kb, LV_KEYBOARD_MODE_USER_1);
    lv_obj_set_style_text_font(mfg_phone_kb, &ui_font_vazirmatn_bold_26, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(mfg_phone_kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(mfg_phone_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(mfg_phone_kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_keyboard_set_textarea(mfg_phone_kb, NULL);
    lv_obj_add_event_cb(mfg_phone_kb, mfg_phone_kb_key_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_state(mfg_phone_ta, LV_STATE_FOCUSED);
    mfg_phone_update_placeholders();

    g_setup_refresh = mfg_phone_refresh;
    g_setup_overlay_active = true;
}

// ===== UIِ بروزرسانیِ درجای صفحه اطلاعات (overlay + تایمر) =====
static lv_obj_t *su_overlay = NULL;
static lv_obj_t *su_info_lbl = NULL;
static lv_obj_t *su_bar = NULL;
static lv_obj_t *su_btn_row = NULL;
static lv_obj_t *su_confirm_btn = NULL;
static lv_obj_t *su_decline_btn = NULL;
static int       su_last_state = -99;
static uint32_t  su_close_deadline = 0;
static uint32_t  su_hold_tick = 0;

static void deadline_hold_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *hold_tick = (uint32_t *)lv_event_get_user_data(e);
    if (!hold_tick) return;
    if (code == LV_EVENT_PRESSED) {
        *hold_tick = millis();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        *hold_tick = 0;
    }
}

typedef struct {
    lv_obj_t **bar;
    int32_t start;
    int32_t end;
    uint32_t duration_ms;
    lv_anim_exec_xcb_t exec_cb;
    lv_anim_ready_cb_t ready_cb;
    bool paused;
    int32_t current;
    uint32_t remain_ms;
} autoclose_anim_hold_t;

static void autoclose_anim_hold_event_cb(lv_event_t *e)
{
    autoclose_anim_hold_t *h = (autoclose_anim_hold_t *)lv_event_get_user_data(e);
    if (!h || !h->bar || !*h->bar || !h->exec_cb) return;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        lv_anim_t *a = lv_anim_get(*h->bar, h->exec_cb);
        if (!a) return;
        h->current = a->current_value;
        int32_t span = h->end - h->start;
        uint32_t elapsed = 0;
        if (span != 0) {
            int32_t moved = h->current - h->start;
            if (moved < 0) moved = -moved;
            int32_t abs_span = span < 0 ? -span : span;
            elapsed = (uint32_t)(((uint64_t)moved * h->duration_ms) / (uint32_t)abs_span);
        }
        h->remain_ms = (elapsed < h->duration_ms) ? (h->duration_ms - elapsed) : 1;
        lv_anim_del(*h->bar, h->exec_cb);
        h->paused = true;
    } else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && h->paused) {
        h->paused = false;
        if (!*h->bar) return;
        uint32_t t = h->remain_ms > 40 ? h->remain_ms : 40;
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, *h->bar);
        lv_anim_set_values(&a, h->current, h->end);
        lv_anim_set_time(&a, t);
        lv_anim_set_exec_cb(&a, h->exec_cb);
        lv_anim_set_ready_cb(&a, h->ready_cb);
        lv_anim_start(&a);
    }
}

static void su_confirm_cb(lv_event_t *e){ if(lv_event_get_code(e)==LV_EVENT_CLICKED) g_su_confirm = true; }
static void su_decline_cb(lv_event_t *e){ if(lv_event_get_code(e)==LV_EVENT_CLICKED) g_su_decline = true; }

static void su_overlay_destroy(void){
    if (su_overlay){ lv_obj_del(su_overlay); su_overlay=NULL; su_info_lbl=NULL; su_bar=NULL; su_btn_row=NULL; su_confirm_btn=NULL; su_decline_btn=NULL; su_hold_tick=0; }
}
// آیتم: ناوبریِ کلیدِ MOD برایِ overlayِ بروزرسانیِ درجا — تعریفِ کامل بعدِ su_overlay_build
static bool su_overlay_is_open(void) { return su_overlay != NULL; }
static lv_obj_t *su_nav_confirm(void) { return su_confirm_btn; }
static lv_obj_t *su_nav_decline(void) { return su_decline_btn; }

static void su_overlay_build(void){
    if (su_overlay) return;
    su_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(su_overlay, 480, 320);
    lv_obj_set_pos(su_overlay, 0, 0);
    lv_obj_clear_flag(su_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(su_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(su_overlay, deadline_hold_event_cb, LV_EVENT_ALL, &su_hold_tick);
    lv_obj_set_style_bg_color(su_overlay, lv_color_hex(0xff121212), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(su_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(su_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(su_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(su_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(su_overlay);
    lv_obj_set_pos(title, 0, 26); lv_obj_set_size(title, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("بروزرسانی","Update","Guncelleme"));

    su_info_lbl = lv_label_create(su_overlay);
    lv_obj_set_pos(su_info_lbl, 0, 92); lv_obj_set_size(su_info_lbl, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(su_info_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(su_info_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(su_info_lbl, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(su_info_lbl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(su_info_lbl, "");

    su_bar = lv_bar_create(su_overlay);
    lv_obj_set_width(su_bar, 400);
    lv_obj_set_height(su_bar, 8);
    lv_obj_align(su_bar, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_bar_set_range(su_bar, 0, 1000);
    lv_bar_set_value(su_bar, 1000, LV_ANIM_OFF);
    lv_obj_set_style_pad_all(su_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(su_bar, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(su_bar, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(su_bar, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(su_bar, lv_color_hex(0xff1a3a45), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(su_bar, lv_color_hex(0xff2fc8d6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_flag(su_bar, LV_OBJ_FLAG_HIDDEN);

    su_btn_row = lv_obj_create(su_overlay);
    lv_obj_set_size(su_btn_row, 480, 70);
    lv_obj_set_pos(su_btn_row, 0, 165);
    lv_obj_set_style_bg_opa(su_btn_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(su_btn_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(su_btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(su_btn_row, LV_OBJ_FLAG_HIDDEN);
    {
        lv_obj_t *bok = lv_btn_create(su_btn_row);
        su_confirm_btn = bok;
        lv_obj_set_size(bok, 150, 50); lv_obj_set_pos(bok, 70, 8);
        lv_obj_set_style_bg_color(bok, lv_color_hex(0xff26bb50), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(bok, su_confirm_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = lv_label_create(bok); lv_obj_center(l);
        lv_obj_set_style_text_font(l, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(l, ML("بروزرسانی","Update","Guncelle"));

        lv_obj_t *bx = lv_btn_create(su_btn_row);
        su_decline_btn = bx;
        lv_obj_set_size(bx, 150, 50); lv_obj_set_pos(bx, 260, 8);
        lv_obj_set_style_bg_color(bx, lv_color_hex(0xffb23a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(bx, su_decline_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l2 = lv_label_create(bx); lv_obj_center(l2);
        lv_obj_set_style_text_font(l2, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(l2, ML("انصراف","Cancel","Iptal"));
    }
}

static void su_end_session_to_menu(void){
    su_overlay_destroy();
    g_su_state = SU_IDLE;
    su_last_state = -99;
    g_self_update_session = false;
    // ایمنی: اگر این session از راهِ ری‌استارت به مدِ OTA رسیده (دکمهٔ «بروزرسانی» صفحهٔ اطلاعات)،
    // app_task/webapp_task اصلاً ساخته نشده‌اند (setup() به‌جایشان فقط ota_task را ساخت) — رفتن به
    // SCREEN_MENU در این حالت صفحهٔ اصلیِ یخ‌زده/ناقص نشان می‌دهد. پس باید دقیقاً مثلِ دکمهٔ برگشتِ
    // صفحهٔ وای‌فا (cancel_info_self_update) به حالتِ عادی ری‌استارت شود.
    if (get_var_ota_enable()) {
        cancel_info_self_update();
    } else {
        eez_flow_set_screen(SCREEN_ID_SCREEN_UPDATE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
    }
}

static void self_update_ui_cb(lv_timer_t *t){
    (void)t;
    if (!g_self_update_session){ if (su_overlay) su_overlay_destroy(); su_last_state=-99; return; }
    if (g_su_decline && su_overlay){ su_end_session_to_menu(); return; }
    int st = g_su_state;
    if (st == SU_IDLE || st == SU_CONNECTING){  // انتخاب/اتصال → overlay بسته، خودِ صفحه وای‌فا (+اسپینر) دیده شود
        if (su_overlay) su_overlay_destroy();
        su_last_state = st;
        return;
    }

    if (st == SU_UPTODATE) {
        // برنامه به‌روز است: حذف کامل پیام و دیالوگ مزاحم، انتقال فوری به صفحه کارت‌های نصب/بروزرسانی
        if (su_overlay) su_overlay_destroy();
        g_self_update_session = false;
        g_su_state = SU_IDLE;
        su_last_state = -99;
        extern void installer_request_scan(void);
        installer_request_scan();
        eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        return;
    }

    if (st != su_last_state){
        su_last_state = st;
        su_overlay_build();
        switch(st){
            case SU_CONNECTING:
                lv_label_set_text(su_info_lbl, ML("در حال اتصال به وایفا","Connecting to WiFi...","WiFi baglaniyor..."));
                lv_obj_add_flag(su_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(su_btn_row, LV_OBJ_FLAG_HIDDEN);
                break;
            case SU_CHECKING:
                lv_label_set_text(su_info_lbl, ML("در حال بررسی سرور","Checking server...","Sunucu kontrol ediliyor..."));
                lv_obj_add_flag(su_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(su_btn_row, LV_OBJ_FLAG_HIDDEN);
                break;
            case SU_NEWER_ASK: {
                String s = ML(String("نسخه جدید ")+g_su_version+" موجود است",
                              String("New version ")+g_su_version+" available",
                              String("Yeni surum ")+g_su_version);
                lv_label_set_text(su_info_lbl, s.c_str());
                lv_obj_add_flag(su_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(su_btn_row, LV_OBJ_FLAG_HIDDEN);
                break; }
            case SU_INSTALLING:
                lv_label_set_text(su_info_lbl, ML("در حال نصب، صبر کنید","Installing... please wait","Kuruluyor... bekleyin"));
                lv_obj_add_flag(su_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(su_btn_row, LV_OBJ_FLAG_HIDDEN);
                break;
            case SU_FAIL: {
                const char *failMsg;
                switch (g_su_fail_code) {
                    case SU_FAIL_SERVER:
                    case SU_FAIL_VERSION:
                        failMsg = ML("سرور در دسترس نیست",
                                     "Server not reachable",
                                     "Sunucuya ulasilamadi");
                        break;
                    case SU_FAIL_INSTALL:
                        failMsg = ML("نصب نسخهٔ جدید ناموفق بود",
                                     "Installing update failed",
                                     "Guncelleme kurulamadi");
                        break;
                    default:
                        failMsg = ML("سرور در دسترس نیست","Server not reachable","Sunucuya ulasilamadi");
                        break;
                }
                lv_label_set_text(su_info_lbl, failMsg);
                lv_obj_add_flag(su_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(su_btn_row, LV_OBJ_FLAG_HIDDEN);
                su_close_deadline = millis() + 2500;
                break;
            }
        }
    }
    if (st == SU_FAIL) {
        if (su_hold_tick) {
            su_close_deadline += millis() - su_hold_tick;
            su_hold_tick = millis();
            return;
        }
        long rem = (long)su_close_deadline - (long)millis();
        if (rem <= 0) {
            su_overlay_destroy();
            g_self_update_session = false; g_su_state = SU_IDLE; su_last_state = -99;
            // پیغام بیاد و بعد از ۲.۵ ثانیه برود — کاربر در صفحه بروزرسانی می‌ماند تا بتواند کلید ورود با لینک یا تلاش مجدد را بزند
            eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
            return;
        }
        return;
    }
}

// ===== خودآزماییِ سخت‌افزار سرِ بوت — گزارشِ یک‌جای RTC/باتریِ‌ساعت/تاچ/MAX6675 روی صفحه لگو =====
// کاربر درگیرِ تستِ دستی نمی‌شود: خودِ دستگاه سرِ بوت (setup(), قبل از این‌که هر تسک/تایمری اجرا شود؛
// همان الگویِ ایمنِ company_overlay_build) هرچه توانست چک می‌کند و اگر مشکلی بود، غیرِبلاک‌کننده
// (بوت به منو ادامه پیدا می‌کند) روی لگو نمایش می‌دهد. با × یا خودبه‌خود بعد از ۲۰ ثانیه بسته می‌شود.
static lv_obj_t *hw_err_box = NULL;
static lv_obj_t *hw_err_bar = NULL;
static uint32_t  hw_err_deadline = 0;
static uint32_t  hw_err_hold_tick = 0;
static void hw_err_close_cb(lv_event_t *e){
    if (lv_event_get_code(e)==LV_EVENT_CLICKED && hw_err_box){ lv_obj_del_async(hw_err_box); hw_err_box=NULL; hw_err_bar=NULL; hw_err_hold_tick=0; }
}
static void hw_selfcheck_overlay_build(void){
    if (!g_hw_rtc_missing && !g_hw_touch_missing && !g_hw_thermo_missing && !g_hw_thermo_sensor_open &&
        !g_hw_cpu_temp_high && !g_hw_rtc_temp_high && !g_rtc_warn_pending) return;

    if (buzzerTaskHandle) xTaskNotify(buzzerTaskHandle, BUZZER_CRITICAL, eSetValueWithOverwrite); // خطاهایِ سخت‌افزاریِ بوت = ۳ بوقِ بلند

    char msg[500]; msg[0] = 0;
    if (g_hw_rtc_missing) {
        strncat(msg, ML("• ساعت دستگاه پیدا نشد", "- Device clock not found", "- Cihaz saati bulunamadi"), sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    } else if (g_rtc_warn_pending) {
        strncat(msg, ML("• باتری ساعت ضعیف است — ساعت را تنظیم کنید", "- Clock battery is weak — set the time", "- Saat pili zayif — saati ayarlayin"), sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    }
    if (g_hw_touch_missing) {
        strncat(msg, ML("• صفحه لمسی پاسخ نمی‌دهد", "- Touch screen not responding", "- Dokunmatik ekran yanit vermiyor"), sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    }
    if (g_hw_thermo_missing) {
        strncat(msg, ML("• سنسور دما پاسخ نمی‌دهد", "- Temperature sensor not responding", "- Sicaklik sensoru yanit vermiyor"), sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    } else if (g_hw_thermo_sensor_open) {
        strncat(msg, ML("• سنسور دمای المنت وصل نیست", "- Element temperature sensor not connected", "- Eleman sicaklik sensoru bagli degil"), sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    }
    if (g_hw_cpu_temp_high) {
        char line[80];
        snprintf(line, sizeof(line), ML("• دمای برد بالاست (%.0f°)", "- Board temperature is high (%.0f\xC2\xB0)", "- Kart sicakligi yuksek (%.0f\xC2\xB0)"), g_hw_cpu_temp_val);
        strncat(msg, line, sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    }
    if (g_hw_rtc_temp_high) {
        char line[80];
        snprintf(line, sizeof(line), ML("• دمای محیطِ ساعت بالاست (%.0f°)", "- Clock area temperature is high (%.0f\xC2\xB0)", "- Saat bolgesi sicakligi yuksek (%.0f\xC2\xB0)"), g_hw_rtc_temp_val);
        strncat(msg, line, sizeof(msg)-strlen(msg)-1);
        strncat(msg, "\n", sizeof(msg)-strlen(msg)-1);
    }
    g_rtc_warn_pending = false; // مصرف شد — دیگر جدا نمایش داده نمی‌شود

    hw_err_box = lv_obj_create(lv_layer_top());
    lv_obj_set_size(hw_err_box, 440, 190);
    lv_obj_center(hw_err_box);
    lv_obj_clear_flag(hw_err_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(hw_err_box, deadline_hold_event_cb, LV_EVENT_ALL, &hw_err_hold_tick);
    lv_obj_set_style_bg_color(hw_err_box, lv_color_hex(0xff1e1e1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(hw_err_box, lv_color_hex(0xffd32f2f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(hw_err_box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(hw_err_box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(hw_err_box);
    lv_obj_set_width(title, 400);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffd32f2f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("خطای سخت‌افزار", "Hardware error", "Donanim hatasi"));

    lv_obj_t *t = lv_label_create(hw_err_box);
    lv_obj_set_width(t, 400);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 44);
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(t, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(t, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(t, msg);

    // × بستن
    lv_obj_t *x = lv_btn_create(hw_err_box);
    lv_obj_set_size(x, 32, 28);
    lv_obj_align(x, LV_ALIGN_TOP_RIGHT, 4, -10);
    lv_obj_set_style_bg_color(x, lv_color_hex(0xffb23a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(x, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(x, hw_err_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(x); lv_obj_center(xl);
    lv_obj_set_style_text_font(xl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, LV_SYMBOL_CLOSE);

    // نوار بسته‌شدنِ ۵ ثانیه‌ای
    hw_err_bar = lv_bar_create(hw_err_box);
    lv_obj_set_size(hw_err_bar, 400, 8);
    lv_obj_align(hw_err_bar, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_bar_set_range(hw_err_bar, 0, 1000);
    lv_bar_set_value(hw_err_bar, 1000, LV_ANIM_OFF);
    hw_err_deadline = millis() + 5000;
}
static void hw_selfcheck_timer_cb(lv_timer_t *t){
    (void)t;
    if (!hw_err_box) return;
    if (hw_err_hold_tick) {
        hw_err_deadline += millis() - hw_err_hold_tick;
        hw_err_hold_tick = millis();
        return;
    }
    long rem = (long)hw_err_deadline - (long)millis();
    if (rem <= 0){ lv_obj_del_async(hw_err_box); hw_err_box=NULL; hw_err_bar=NULL; hw_err_hold_tick=0; return; }
    if (hw_err_bar) lv_bar_set_value(hw_err_bar, (int)(rem*1000/5000), LV_ANIM_OFF);
}

// ===== اسکرین‌سیورِ صفحهٔ اصلی — بازیِ توپ/تخته (Breakout سبک) =====
// بعدِ بی‌کاریِ طولانی روی صفحهٔ منو: توپ = لوگوی پنجره، تخته = ساعت متحرک. لمسِ کوتاه = خروج.
// نگه‌داشتنِ ≥۱ث = دیالوگِ مدت. تاچ/کشیدن = حرکتِ تخته. فعالیت با screensaver_note_activity().
static lv_obj_t *ss_overlay = NULL;
static lv_obj_t *ss_icon = NULL;       // توپ — لوگوی ۴ مربع
static lv_obj_t *ss_paddle = NULL;     // تخته — پس‌زمینهٔ ساعت
static lv_obj_t *ss_clock_label = NULL;
static lv_obj_t *ss_caption_label = NULL;
static lv_timer_t *ss_game_timer = NULL;
static lv_timer_t *ss_clock_timer = NULL;
static uint32_t ss_press_start = 0;
static bool ss_long_fired = false;
static bool ss_touching = false;
static lv_point_t ss_press_point = {0, 0};
static lv_obj_t *ss_zone_exit = NULL;  // ناحیهٔ ۲ بالا — خروج / نگه‌داشتن=تنظیم
static lv_obj_t *ss_zone_play = NULL;   // ناحیهٔ ۱ پایین — فقط تخته؛ هرگز خروج
static lv_obj_t *ss_cfg_dlg = NULL;
static lv_obj_t *ss_cfg_bar = NULL;
static uint32_t ss_cfg_deadline = 0;
static uint32_t ss_cfg_hold_tick = 0;

static const lv_coord_t SS_BALL_SZ = 36;   // ۲۰٪ کوچکتراز ۴۶ (قبلاً ۴۶ بود)
static const lv_coord_t SS_PANE_SZ = 15;   // متناسب با اندازه جدید
static const lv_coord_t SS_PANE_GAP = 2;   // فاصله بهینه بین مربع‌ها
static const lv_coord_t SS_PANE_INSET = 2;
static const lv_coord_t SS_PADDLE_W = 112; // تخته عریض‌تر برای کنترل کامل توپ و پایداری بازی
static const lv_coord_t SS_PADDLE_H = 22;  // نوار صیقلی سبک بدون فونت داخلی
static const lv_coord_t SS_PLAY_TOP = 38;  // فضای بالای صفحه برای ساعت دیجیتال ثابت
static lv_coord_t SS_PLAY_ZONE_Y = 230;
static lv_coord_t SS_PADDLE_Y = 232;

static lv_coord_t ss_ball_x = 0;
static lv_coord_t ss_ball_y = 0;
static int16_t ss_vx = 3;
static int16_t ss_vy = 3;
static lv_coord_t ss_paddle_x = 0;
static lv_coord_t ss_play_w = SCREEN_WIDTH;
static lv_coord_t ss_play_h = SCREEN_HEIGHT;

extern "C" bool storage_get_sysbar_enabled(void) {
    return storage.getSysInfoBarEnabled();
}

extern "C" void storage_set_sysbar_enabled(bool en) {
    storage.setSysInfoBarEnabled(en);
}

extern "C" void storage_set_temp_caliber(int val) {
    storage.setTempCaliber(val);
}

static void screensaver_cfg_dialog_open(void); // پیش‌اعلان

static void ss_touch_local_point(lv_coord_t *ox, lv_coord_t *oy);
static lv_coord_t ss_touch_local_x(void);
static bool ss_in_play_zone(lv_coord_t local_y);

static void ss_touch_local_point(lv_coord_t *ox, lv_coord_t *oy) {
    lv_indev_t *indev = lv_indev_active();
    if (!indev || !ss_overlay) {
        *ox = (lv_coord_t)(ss_paddle_x + SS_PADDLE_W / 2);
        *oy = SS_PADDLE_Y;
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t a;
    lv_obj_get_coords(ss_overlay, &a);
    lv_coord_t x = (lv_coord_t)(p.x - a.x1);
    lv_coord_t y = (lv_coord_t)(p.y - a.y1);
    if (x < 0) x = 0;
    if (x >= ss_play_w) x = (lv_coord_t)(ss_play_w - 1);
    if (y < 0) y = 0;
    if (y >= ss_play_h) y = (lv_coord_t)(ss_play_h - 1);
    *ox = x;
    *oy = y;
}

static lv_coord_t ss_touch_local_x(void) {
    lv_coord_t lx, ly;
    ss_touch_local_point(&lx, &ly);
    (void)ly;
    return lx;
}

static bool ss_in_play_zone(lv_coord_t local_y) {
    return local_y >= SS_PLAY_ZONE_Y;
}

static void ss_invalidate_ball_at(lv_coord_t x, lv_coord_t y) {
    if (!ss_overlay) return;
    lv_area_t a;
    a.x1 = (lv_coord_t)(x - 6);
    a.y1 = (lv_coord_t)(y - 6);
    a.x2 = (lv_coord_t)(x + SS_BALL_SZ + 6);
    a.y2 = (lv_coord_t)(y + SS_BALL_SZ + 6);
    if (a.x1 < 0) a.x1 = 0;
    if (a.y1 < 0) a.y1 = 0;
    lv_obj_invalidate_area(ss_overlay, &a);
}

static void ss_invalidate_paddle(void) {
    if (!ss_overlay) return;
    lv_area_t a;
    a.x1 = (lv_coord_t)(ss_paddle_x - 4);
    a.y1 = (lv_coord_t)(SS_PADDLE_Y - 4);
    a.x2 = (lv_coord_t)(ss_paddle_x + SS_PADDLE_W + 4);
    a.y2 = (lv_coord_t)(SS_PADDLE_Y + SS_PADDLE_H + 4);
    lv_obj_invalidate_area(ss_overlay, &a);
}

static void ss_paddle_from_x(lv_coord_t touch_x) {
    ss_paddle_x = touch_x - SS_PADDLE_W / 2;
    if (ss_paddle_x < 0) ss_paddle_x = 0;
    if (ss_paddle_x + SS_PADDLE_W > ss_play_w) ss_paddle_x = (lv_coord_t)(ss_play_w - SS_PADDLE_W);
    if (ss_paddle) lv_obj_set_pos(ss_paddle, ss_paddle_x, SS_PADDLE_Y);
}

static uint8_t ss_hit_counter = 0; // 0,1 -> Right (R, R) | 2,3 -> Left (L, L)

static void ss_ball_reset(void) {
    ss_ball_x = (lv_coord_t)((ss_play_w - SS_BALL_SZ) / 2);
    ss_ball_y = SS_PLAY_TOP + 20;
    ss_vx = (int16_t)((ss_vx >= 0) ? 3 : -3);
    ss_vy = 3;
}

static void screensaver_game_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!ss_icon || !ss_paddle) return;

    bool is_target_left = (ss_hit_counter >= 2); // 0,1: راست | 2,3: چپ

    static lv_coord_t prev_paddle_x = -999;
    // هوش مصنوعی و حرکت خودکار تخته:
    // وقتی توپ در حال پایین آمدن است، تخته با سرعت بالا به استقبال توپ در نقطه ایمن می‌رود تا هرگز نبازد.
    // وقتی توپ بالا می‌رود، تخته به آرامی به سمت مرکز صفحه حرکت می‌کند تا همیشه در وضعیت بهینه باشد.
    if (!ss_touching) {
        lv_coord_t ball_cx = ss_ball_x + SS_BALL_SZ / 2;
        lv_coord_t target_paddle_x;
        if (ss_vy > 0) {
            // نقطه فرود در ۲۸٪ یا ۷۲٪ عرض تخته برای انحراف دلپذیر توپ به چپ یا راست
            lv_coord_t sweet_spot = is_target_left ? (SS_PADDLE_W * 28 / 100) : (SS_PADDLE_W * 72 / 100);
            target_paddle_x = ball_cx - sweet_spot;
        } else {
            // ریکاوری موقعیت تخته به سمت مرکز صفحه
            target_paddle_x = (lv_coord_t)((ss_play_w - SS_PADDLE_W) / 2);
        }
        lv_coord_t diff = target_paddle_x - ss_paddle_x;
        if (abs(diff) > 2) {
            lv_coord_t step = (abs(diff) > 30) ? 9 : ((abs(diff) > 12) ? 6 : ((abs(diff) > 4) ? 3 : 2));
            if (diff > 0) ss_paddle_x += step;
            else ss_paddle_x -= step;
        }
        if (ss_paddle_x < 0) ss_paddle_x = 0;
        if (ss_paddle_x + SS_PADDLE_W > ss_play_w) ss_paddle_x = (lv_coord_t)(ss_play_w - SS_PADDLE_W);
        if (ss_paddle_x != prev_paddle_x) {
            prev_paddle_x = ss_paddle_x;
            lv_obj_set_pos(ss_paddle, ss_paddle_x, SS_PADDLE_Y);
        }
    }

    ss_ball_x = (lv_coord_t)(ss_ball_x + ss_vx);
    ss_ball_y = (lv_coord_t)(ss_ball_y + ss_vy);

    if (ss_ball_x <= 0) { ss_ball_x = 0; ss_vx = (int16_t)(-ss_vx); }
    else if (ss_ball_x + SS_BALL_SZ >= ss_play_w) { ss_ball_x = (lv_coord_t)(ss_play_w - SS_BALL_SZ); ss_vx = (int16_t)(-ss_vx); }
    if (ss_ball_y <= SS_PLAY_TOP) { ss_ball_y = SS_PLAY_TOP; ss_vy = (int16_t)(-ss_vy); }

    if (ss_vy > 0) {
        lv_coord_t ball_bottom = ss_ball_y + SS_BALL_SZ;
        lv_coord_t paddle_top = SS_PADDLE_Y;
        if (ball_bottom >= paddle_top && ss_ball_y <= paddle_top + SS_PADDLE_H) {
            lv_coord_t ball_cx = ss_ball_x + SS_BALL_SZ / 2;
            if (ball_cx + 12 >= ss_paddle_x && ball_cx - 12 <= ss_paddle_x + SS_PADDLE_W) {
                ss_ball_y = paddle_top - SS_BALL_SZ;
                ss_vy = -3;
                if (is_target_left) {
                    ss_vx = 3;  // پرتاب به راست
                } else {
                    ss_vx = -3; // پرتاب به چپ
                }
                ss_hit_counter = (ss_hit_counter + 1) % 4;
            }
        }
    }

    if (ss_ball_y + SS_BALL_SZ > ss_play_h - 28) ss_ball_reset();

    lv_obj_set_pos(ss_icon, ss_ball_x, ss_ball_y);
}
static void screensaver_clock_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!ss_clock_label) return;
    static uint32_t last_rtc_sync_ms = 0;
    static int cached_hour = -1;
    static int cached_min = -1;
    static uint32_t last_min_advance_ms = 0;

    // خواندن واقعی از چیپ I2C فقط هر ۶۰ ثانیه یک بار تا باس I2C متوقف نشود و لگ ثانیه‌ای رخ ندهد
    if (cached_hour < 0 || (millis() - last_rtc_sync_ms >= 60000UL)) {
        DateTime now = rtc_now_s();
        cached_hour = now.hour();
        cached_min = now.minute();
        last_rtc_sync_ms = millis();
        last_min_advance_ms = millis();
    } else if (millis() - last_min_advance_ms >= 60000UL) {
        last_min_advance_ms = millis();
        cached_min++;
        if (cached_min >= 60) {
            cached_min = 0;
            cached_hour = (cached_hour + 1) % 24;
        }
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", cached_hour, cached_min);
    const char *cur = lv_label_get_text(ss_clock_label);
    if (!cur || strcmp(buf, cur) != 0) {
        lv_label_set_text(ss_clock_label, buf);
    }
}
static void screensaver_destroy(void) {
    if (ss_game_timer) { lv_timer_del(ss_game_timer); ss_game_timer = NULL; }
    if (ss_clock_timer) { lv_timer_del(ss_clock_timer); ss_clock_timer = NULL; }
    // lv_obj_del_async نه lv_obj_del: این تابع از داخلِ handlerِ تاچِ خودِ overlay هم صدا زده می‌شود
    // (نگه‌داشتن/لمسِ کوتاه). حذفِ synchronous وسطِ پردازشِ همان تاچ باعث می‌شد LVGL همان لحظه دوباره
    // hit-test کند و همان تاچ به آبجکتِ زیرین (آیکونِ تنظیمات/کیبردِ گیج) هم برسد — یعنی یک تاچ هم
    // اسکرین‌سیور را می‌بست هم زیرش را لمس می‌کرد. با async، حذفِ واقعی بعدِ اتمامِ کاملِ پردازشِ این
    // رویداد انجام می‌شود و زیرِ overlay اصلاً چیزی نمی‌بیند.
    if (ss_overlay) {
        lv_obj_del_async(ss_overlay);
        ss_overlay = NULL;
        ss_icon = NULL;
        ss_paddle = NULL;
        ss_clock_label = NULL;
        ss_caption_label = NULL;
        ss_zone_exit = NULL;
        ss_zone_play = NULL;
    }
    ss_touching = false;
    g_screensaver_active = false;
}
static void screensaver_exit(void) {
    screensaver_destroy();
    g_last_activity_ms = millis(); // خروج = فعالیتِ تازه؛ شمارشِ ورودِ بعدی از همین لحظه شروع شود
}
extern "C" void screensaver_note_activity(void) {
    g_last_activity_ms = millis();
    if (g_screensaver_active) screensaver_exit();
}
extern "C" bool screensaver_is_active(void) {
    return g_screensaver_active;
}

// ناحیهٔ ۱ (پایین ~۳۰٪): فقط حرکتِ تخته — لمس/نگه‌داشتن هرگز خروج یا دیالوگ نمی‌دهد
static void screensaver_zone_play_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        ss_touching = true;
        ss_paddle_from_x(ss_touch_local_x());
    } else if (code == LV_EVENT_PRESSING) {
        if (ss_touching) ss_paddle_from_x(ss_touch_local_x());
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ss_touching = false;
    }
}

static uint32_t ss_built_tick = 0;

// ناحیهٔ ۲ (بالا ~۷۰٪): لمسِ کوتاه=خروج، نگه‌داشتن≥۱ث=دیالوگِ زمان‌بندی
static void screensaver_zone_exit_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        ss_press_start = lv_tick_get();
        ss_long_fired = false;
    } else if (code == LV_EVENT_PRESSING) {
        if (ss_press_start > 0 && !ss_long_fired && lv_tick_elaps(ss_press_start) >= 1000) {
            ss_long_fired = true;
            xTaskNotify(buzzerTaskHandle, BUZZER_IMPORTANT, eSetValueWithOverwrite);
            screensaver_destroy();
            screensaver_cfg_dialog_open();
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (ss_press_start > 0 && !ss_long_fired && lv_tick_elaps(ss_built_tick) >= 400U) {
            screensaver_exit();
        }
        ss_press_start = 0;
        ss_long_fired = false;
    }
}

static void screensaver_zone_panel_style(lv_obj_t *obj) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}
static void screensaver_build(void) {
    if (ss_overlay) return;
    ss_built_tick = lv_tick_get();
    ss_press_start = 0;
    ss_long_fired = false;
    lv_display_t *disp = lv_display_get_default();
    ss_play_w = disp ? lv_display_get_horizontal_resolution(disp) : SCREEN_WIDTH;
    ss_play_h = disp ? lv_display_get_vertical_resolution(disp) : SCREEN_HEIGHT;
    SS_PLAY_ZONE_Y = (lv_coord_t)((ss_play_h * 70) / 100);
    SS_PADDLE_Y = (lv_coord_t)(SS_PLAY_ZONE_Y + 1);

    ss_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ss_overlay, ss_play_w, ss_play_h);
    lv_obj_set_pos(ss_overlay, 0, 0);
    lv_obj_align(ss_overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(ss_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ss_overlay, LV_OBJ_FLAG_CLICKABLE); // تاچ فقط روی zone_exit / zone_play
    lv_obj_set_style_bg_color(ss_overlay, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ss_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ss_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ss_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ss_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ناحیهٔ ۱ (پایین): فقط تخته — هرگز خروج/دیالوگ
    ss_zone_play = lv_obj_create(ss_overlay);
    lv_obj_set_pos(ss_zone_play, 0, SS_PLAY_ZONE_Y);
    lv_obj_set_size(ss_zone_play, ss_play_w, (lv_coord_t)(ss_play_h - SS_PLAY_ZONE_Y));
    screensaver_zone_panel_style(ss_zone_play);
    lv_obj_add_event_cb(ss_zone_play, screensaver_zone_play_cb, LV_EVENT_ALL, NULL);

    // ناحیهٔ ۲ (بالا): خروج با لمس، نگه‌داشتن=تنظیم زمان
    ss_zone_exit = lv_obj_create(ss_overlay);
    lv_obj_set_pos(ss_zone_exit, 0, 0);
    lv_obj_set_size(ss_zone_exit, ss_play_w, SS_PLAY_ZONE_Y);
    screensaver_zone_panel_style(ss_zone_exit);
    lv_obj_add_event_cb(ss_zone_exit, screensaver_zone_exit_cb, LV_EVENT_ALL, NULL);
    lv_obj_move_foreground(ss_zone_play);
    lv_obj_move_foreground(ss_zone_exit);

    // توپ = لوگوی ۴ مربع (~۳۰٪ کوچک‌تر). inset داخلی + invalidate ناحیهٔ قبلی → گوشه‌های گرد بدونِ بریدگی.
    ss_icon = lv_obj_create(ss_overlay);
    lv_obj_set_size(ss_icon, SS_BALL_SZ, SS_BALL_SZ);
    ss_ball_reset();
    lv_obj_set_pos(ss_icon, ss_ball_x, ss_ball_y);
    lv_obj_clear_flag(ss_icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ss_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ss_icon, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_bg_opa(ss_icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ss_icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ss_icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    {
        const lv_coord_t step = (lv_coord_t)(SS_PANE_SZ + SS_PANE_GAP);
        static const int pane_ix[4] = {0, 1, 0, 1};
        static const int pane_iy[4] = {0, 0, 1, 1};
        for (int i = 0; i < 4; i++) {
            lv_obj_t *pane = lv_obj_create(ss_icon);
            lv_obj_set_size(pane, SS_PANE_SZ, SS_PANE_SZ);
            lv_obj_set_pos(pane,
                (lv_coord_t)(SS_PANE_INSET + pane_ix[i] * step),
                (lv_coord_t)(SS_PANE_INSET + pane_iy[i] * step));
            lv_obj_clear_flag(pane, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(pane, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_radius(pane, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(pane, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(pane, lv_color_hex(0xff5aa9e6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(pane, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(pane, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // تخته صیقلی — با تاچ/کشیدن یا دنبال‌کردنِ خودکارِ توپ حرکت می‌کند (بدون رندر متن)
    ss_paddle_x = (lv_coord_t)((ss_play_w - SS_PADDLE_W) / 2);
    ss_paddle = lv_obj_create(ss_overlay);
    lv_obj_set_pos(ss_paddle, ss_paddle_x, SS_PADDLE_Y);
    lv_obj_set_size(ss_paddle, SS_PADDLE_W, SS_PADDLE_H);
    lv_obj_clear_flag(ss_paddle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ss_paddle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ss_paddle, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ss_paddle, lv_color_hex(0xff1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ss_paddle, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ss_paddle, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ss_paddle, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ss_paddle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ساعت دیجیتال ثابت در بالای صفحه — بدون ایجاد رندر مجدد در حین حرکت تخته
    ss_clock_label = lv_label_create(ss_overlay);
    lv_obj_set_style_text_color(ss_clock_label, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ss_clock_label, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ss_clock_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ss_clock_label, LV_ALIGN_TOP_MID, 0, 8);
    {
        DateTime now = rtc_now_s();
        char buf[8]; snprintf(buf, sizeof(buf), "%02d:%02d", now.hour(), now.minute());
        lv_label_set_text(ss_clock_label, buf);
    }

    lv_obj_t *cap = lv_label_create(ss_overlay);
    ss_caption_label = cap;
    lv_obj_set_width(cap, 400);
    lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cap, lv_color_hex(0xff607d8b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cap, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(cap, ML("برای بازگشت قسمتِ بالا را لمس کنید — در حالتِ استراحت",
                              "Standing by \xE2\x80\x94 tap upper area to return",
                              "Beklemede \xE2\x80\x94 donmek icin ust bolgeye dokunun"));
    lv_obj_align(cap, LV_ALIGN_BOTTOM_MID, 0, -2); // caption پایین؛ فاصله تا کادر ساعت با SS_PADDLE_Y تنظیم می‌شود

    ss_game_timer = lv_timer_create(screensaver_game_timer_cb, 28, NULL);
    ss_clock_timer = lv_timer_create(screensaver_clock_timer_cb, 1000, NULL);
    g_screensaver_active = true;
}

static void screensaver_cfg_dialog_open(void); // پیش‌اعلان

static void screensaver_idle_timer_cb(lv_timer_t *t) {
    (void)t;
    if (g_screensaver_active || ss_cfg_dlg) return;
    int minutes = storage.getScreensaverMin();
    if (minutes <= 0) return; // «کلا نباشد»
    if (eez_flow_get_current_screen() != SCREEN_ID_SCREEN_MENU) return; // فقط رویِ صفحهٔ اصلی
    if (get_var_step_process() || g_device_locked || get_var_test_mode()) return; // حینِ سیکلِ جوش/قفل/حالتِ تست، اسکرین‌سیور نیاید
    if (g_setup_overlay_active || hw_err_box || company_overlay || admin_setup_overlay || lock_overlay) return;
    uint32_t idleMs = millis() - g_last_activity_ms;
    if (idleMs >= (uint32_t)minutes * 60000UL) screensaver_build();
}
// ===== دیالوگِ تنظیمِ مدتِ اسکرین‌سیور — با نگه‌داشتنِ ≥۱ث روی خودِ اسکرین‌سیور باز می‌شود =====
static void screensaver_cfg_close_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && ss_cfg_dlg) { lv_obj_del_async(ss_cfg_dlg); ss_cfg_dlg = NULL; ss_cfg_bar = NULL; ss_cfg_hold_tick = 0; }
}
static void screensaver_cfg_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!ss_cfg_dlg) return;
    if (ss_cfg_hold_tick) {
        ss_cfg_deadline += millis() - ss_cfg_hold_tick;
        ss_cfg_hold_tick = millis();
        return;
    }
    long rem = (long)ss_cfg_deadline - (long)millis();
    if (rem <= 0) { lv_obj_del_async(ss_cfg_dlg); ss_cfg_dlg = NULL; ss_cfg_bar = NULL; ss_cfg_hold_tick = 0; return; }
    if (ss_cfg_bar) lv_bar_set_value(ss_cfg_bar, (int)(rem*1000/8000), LV_ANIM_OFF);
}
static const char *screensaver_option_text(int minutes) {
    // آیتم: اعدادِ فارسی اینجا به‌صورتِ خانه‌های خالی (تُهی) نمایش داده می‌شدند — این فونت رقم‌هایِ
    // فارسیِ لازم را ندارد. طبقِ الگویِ قبلیِ برنامه، با رقمِ لاتین جایگزین شد.
    switch (minutes) {
        case 0:   return ML("کلا نباشد", "Never", "Hic devreye girmesin");
        case 5:   return ML("5 دقیقه", "5 minutes", "5 dakika");
        case 15:  return ML("15 دقیقه", "15 minutes", "15 dakika");
        case 30:  return ML("30 دقیقه", "30 minutes", "30 dakika");
        case 60:  return ML("1 ساعت", "1 hour", "1 saat");
        case 120: return ML("2 ساعت", "2 hours", "2 saat");
        default:  return ML("5 ساعت", "5 hours", "5 saat");
    }
}
static void screensaver_cfg_pick_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int minutes = (int)(intptr_t)lv_event_get_user_data(e);
    storage.setScreensaverMin(minutes);
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    g_last_activity_ms = millis(); // انتخاب = فعالیت؛ شمارشِ ورودِ بعدی از الان شروع شود
    if (ss_cfg_dlg) { lv_obj_del_async(ss_cfg_dlg); ss_cfg_dlg = NULL; ss_cfg_bar = NULL; ss_cfg_hold_tick = 0; }
}
static void screensaver_cfg_dialog_open(void) {
    if (ss_cfg_dlg) return;
    ss_cfg_dlg = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ss_cfg_dlg, 320, 316);
    lv_obj_center(ss_cfg_dlg);
    lv_obj_clear_flag(ss_cfg_dlg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ss_cfg_dlg, deadline_hold_event_cb, LV_EVENT_ALL, &ss_cfg_hold_tick);
    lv_obj_set_style_bg_color(ss_cfg_dlg, lv_color_hex(0xff1e1e1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ss_cfg_dlg, lv_color_hex(0xff5aa9e6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ss_cfg_dlg, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ss_cfg_dlg, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ss_cfg_dlg, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    // نکته‌ی مهم: تکیه بر LV_LABEL_LONG_WRAPِ خودکار برای متنِ فارسیِ طولانی باعثِ به‌هم‌ریختنِ ترتیبِ
    // کلمات می‌شود (همان الگویی که rtc_warn_show/hw_selfcheck با شکستنِ دستیِ خط با "\n" حلش کرده‌اند).
    // عنوان: تکخط با LONG_CLIP (بدونِ wrap) تا هرگز خودکار شکسته نشود.
    lv_obj_t *title = lv_label_create(ss_cfg_dlg);
    lv_obj_set_width(title, 296);
    lv_obj_set_height(title, 28);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff5aa9e6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("تنظیمِ اسکرین‌سیور", "Screensaver Settings", "Ekran Koruyucu Ayari"));

    lv_obj_t *desc = lv_label_create(ss_cfg_dlg);
    lv_obj_set_width(desc, 300);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 34);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(desc, lv_color_hex(0xffb0bec5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(desc, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    // شکستِ خط دستی و کوتاه — به همان دلیلِ بالا (هر خط به‌تنهایی کوتاه و مطمئن است)
    lv_label_set_text(desc, ML("برای محافظتِ نمایشگر و صرفه‌جویی در انرژی،\nاسکرین‌سیور بعد از چه مدت بی‌کاری روشن شود؟",
                                "To protect the screen and save energy,\nwhen should the screensaver turn on?",
                                "Ekrani korumak ve enerji tasarrufu icin,\nekran koruyucu ne zaman devreye girsin?"));

    static const int values[7] = {0, 5, 15, 30, 60, 120, 300};
    int curVal = storage.getScreensaverMin();
    lv_coord_t y = 78;
    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(ss_cfg_dlg);
        lv_obj_set_size(btn, 296, 26);
        lv_obj_set_pos(btn, 4, y);
        bool isCur = (values[i] == curVal);
        lv_obj_set_style_bg_color(btn, lv_color_hex(isCur ? 0xff2e7d32 : 0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(btn, screensaver_cfg_pick_cb, LV_EVENT_CLICKED, (void *)(intptr_t)values[i]);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl, screensaver_option_text(values[i]));
        y += 29;
    }

    lv_obj_t *x = lv_btn_create(ss_cfg_dlg);
    lv_obj_set_size(x, 32, 28);
    lv_obj_align(x, LV_ALIGN_TOP_RIGHT, 4, -4);
    lv_obj_set_style_bg_color(x, lv_color_hex(0xffb23a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(x, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(x, screensaver_cfg_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(x); lv_obj_center(xl);
    lv_obj_set_style_text_font(xl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, LV_SYMBOL_CLOSE);

    // نوارِ بسته‌شدنِ خودکارِ ۸ ثانیه‌ای — مثلِ بقیهٔ دیالوگ‌های برنامه (hw_err/rtc_warn)
    ss_cfg_bar = lv_bar_create(ss_cfg_dlg);
    lv_obj_set_size(ss_cfg_bar, 296, 6);
    lv_obj_align(ss_cfg_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_bar_set_range(ss_cfg_bar, 0, 1000);
    lv_bar_set_value(ss_cfg_bar, 1000, LV_ANIM_OFF);
    ss_cfg_deadline = millis() + 8000;
    ss_cfg_hold_tick = 0;
}

// ===== دیالوگ هشدار دما (آیتم ۲): دما به ستِ‌پوینت نرسیده، تأیید/انصراف =====
static lv_obj_t *temp_dlg = NULL;
static lv_obj_t *temp_dlg_bar = NULL;
static lv_obj_t *temp_dlg_cancel_btn = NULL;
static lv_obj_t *temp_dlg_confirm_btn = NULL;
static lv_obj_t *main_nav_temp_cancel(void) { return temp_dlg_cancel_btn; }
static lv_obj_t *main_nav_temp_confirm(void) { return temp_dlg_confirm_btn; }

static void temp_dlg_destroy()
{
    if (temp_dlg) { lv_obj_del(temp_dlg); temp_dlg = NULL; temp_dlg_bar = NULL; temp_dlg_cancel_btn = NULL; temp_dlg_confirm_btn = NULL; }
}
// نوارِ ۸ ثانیه‌ای که خالی می‌شود؛ در پایان، دیالوگ خودکار بسته می‌شود (پیش‌فرضِ امن: شروع نمی‌شود)
static void temp_dlg_bar_anim_cb(void *o, int32_t v) { lv_bar_set_value((lv_obj_t *)o, v, LV_ANIM_OFF); }
static void temp_dlg_timeout_ready_cb(lv_anim_t *a) { (void)a; temp_dlg_destroy(); }
static autoclose_anim_hold_t temp_dlg_anim_hold = { &temp_dlg_bar, 0, 100, 8000, temp_dlg_bar_anim_cb, temp_dlg_timeout_ready_cb, false, 0, 0 };

static void temp_dlg_confirm_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    g_temp_override_start = 1; // task سیکل را با دمای پایین شروع می‌کند
    xTaskNotify(buzzerTaskHandle, BUZZER_CONFIRM, eSetValueWithOverwrite);
    temp_dlg_destroy();
}

static void temp_dlg_cancel_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    temp_dlg_destroy(); // فقط بستن؛ سیکل شروع نمی‌شود
}

static void temp_dlg_deleted_cb(lv_event_t *e)
{
    (void)e;
    temp_dlg = NULL;
    temp_dlg_bar = NULL;
    temp_dlg_cancel_btn = NULL;
    temp_dlg_confirm_btn = NULL;
}

static void temp_dlg_build()
{
    temp_dlg = lv_obj_create(lv_layer_top());
    lv_obj_add_event_cb(temp_dlg, temp_dlg_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(temp_dlg, 480, 320);
    lv_obj_set_pos(temp_dlg, 0, 0);
    lv_obj_clear_flag(temp_dlg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(temp_dlg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(temp_dlg, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &temp_dlg_anim_hold);
    lv_obj_set_style_bg_color(temp_dlg, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(temp_dlg, 170, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(temp_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(temp_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(temp_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // کارتِ وسط
    lv_obj_t *card = lv_obj_create(temp_dlg);
    lv_obj_set_size(card, 456, 312);
    lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(card, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(card, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &temp_dlg_anim_hold);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xff1f2937), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);

    // دکمهٔ × بستن (گوشهٔ بالا راست) — مثل انصراف عمل می‌کند تا کاربرانِ کم‌سواد نگران نشوند
    lv_obj_t *btn_x = lv_btn_create(card);
    lv_obj_set_size(btn_x, 32, 32);
    lv_obj_align(btn_x, LV_ALIGN_TOP_RIGHT, 6, -6);
    lv_obj_set_style_bg_color(btn_x, lv_color_hex(0xffae4c4c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_x, temp_dlg_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(btn_x);
    lv_obj_set_style_text_font(xl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, "X");
    lv_obj_center(xl);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_set_pos(title, 0, 2);
    lv_obj_set_size(title, 400, LV_SIZE_CONTENT);
    lv_obj_set_style_align(title, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_kalameh_bl_ack_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("هشدار دما", "Temp Warning", "Sicaklik Uyarisi"));

    // متن با خط‌شکنِ دستی و بدون نقطه/گیومه نوشته شده تا ترتیب کلماتِ فارسی به‌هم نریزد (محدودیت BiDi).
    lv_obj_t *msg = lv_label_create(card);
    lv_obj_set_pos(msg, 0, 56);
    lv_obj_set_size(msg, 446, LV_SIZE_CONTENT);
    lv_obj_set_style_align(msg, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
    // متنِ پیوسته + auto-wrap: هر خط تا آخر پر می‌شود سپس به خط بعد می‌رود (بدون نقطه/«»/نیم‌فاصله تا ترتیب به‌هم نریزد)
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(msg, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(msg, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(msg, ML(
        "دمای دستگاه هنوز به مقدار تنظیم نرسیده\n"
        "تأیید = شروع جوش\n"
        "انصراف = صبر کنید",
        "Temperature has not reached the set point\n"
        "Confirm = start welding\n"
        "Cancel = wait",
        "Sicaklik ayarlanan degere ulasmadi\n"
        "Tamam = kaynaga basla\n"
        "Iptal = bekle"));

    lv_obj_t *note = lv_label_create(card);
    lv_obj_set_pos(note, 0, 150);
    lv_obj_set_size(note, 446, LV_SIZE_CONTENT);
    lv_obj_set_style_align(note, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(note, lv_color_hex(0xffaeb7be), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(note, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(note, ML(
        "برای خاموش کردن این پیام:\n"
        "در تنظیمات عمومی، محدوده دما را غیرفعال کنید",
        "To disable this message:\n"
        "In General Settings, turn off Temp Range",
        "Bu mesaji kapatmak icin:\n"
        "Genel Ayarlarda Sicaklik Araligini kapatin"));

    // قاب زمان‌دار دور انصراف؛ با پرشدن آن زمان بسته‌شدن خودکار دیده می‌شود.
    temp_dlg_bar = lv_bar_create(card);
    lv_obj_set_size(temp_dlg_bar, 178, 56);
    lv_obj_set_pos(temp_dlg_bar, 8, -6);
    lv_obj_set_style_align(temp_dlg_bar, LV_ALIGN_BOTTOM_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_bar_set_range(temp_dlg_bar, 0, 100);
    lv_bar_set_value(temp_dlg_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(temp_dlg_bar, lv_color_hex(0xff5a2020), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(temp_dlg_bar, lv_color_hex(0xffffac38), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(temp_dlg_bar, 9, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(temp_dlg_bar, 9, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_t *btn_cancel = lv_btn_create(temp_dlg_bar);
    temp_dlg_cancel_btn = btn_cancel;
    lv_obj_set_size(btn_cancel, 170, 48);
    lv_obj_center(btn_cancel);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xffc62828), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_cancel, temp_dlg_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lc = lv_label_create(btn_cancel);
    lv_obj_set_style_align(lc, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lc, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lc, ML("انصراف", "Cancel", "Iptal"));

    // دکمهٔ تأیید (راست)
    lv_obj_t *btn_ok = lv_btn_create(card);
    temp_dlg_confirm_btn = btn_ok;
    lv_obj_set_size(btn_ok, 170, 48);
    lv_obj_set_pos(btn_ok, -12, -10);
    lv_obj_set_style_align(btn_ok, LV_ALIGN_BOTTOM_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff2e7d32), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_ok, temp_dlg_confirm_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lo = lv_label_create(btn_ok);
    lv_obj_set_style_align(lo, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lo, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lo, ML("تأیید", "Confirm", "Tamam"));

    // هشت ثانیه تا انصراف خودکار.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, temp_dlg_bar);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_time(&a, 8000);
    lv_anim_set_exec_cb(&a, temp_dlg_bar_anim_cb);
    lv_anim_set_ready_cb(&a, temp_dlg_timeout_ready_cb);
    lv_anim_start(&a);
}

// ===== پیغام پایان جوش (#۳): کارتِ شناورِ غیرمسدودکننده با نوار ۴ ثانیه‌ای و بستن خودکار =====
static lv_obj_t *weld_done_dlg = NULL;
static lv_obj_t *weld_done_bar = NULL;
static lv_obj_t *weld_done_close_btn = NULL;
static lv_obj_t *main_nav_weld_done_close(void) { return weld_done_close_btn; }

static void weld_done_destroy()
{
    if (weld_done_dlg) { lv_obj_del(weld_done_dlg); weld_done_dlg = NULL; weld_done_bar = NULL; weld_done_close_btn = NULL; }
}
static void weld_done_deleted_cb(lv_event_t *e) { (void)e; weld_done_dlg = NULL; weld_done_bar = NULL; weld_done_close_btn = NULL; }
static void weld_done_close_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) weld_done_destroy(); }
static void weld_done_bar_anim_cb(void *obj, int32_t v) { lv_bar_set_value((lv_obj_t *)obj, v, LV_ANIM_OFF); }
static void weld_done_anim_ready_cb(lv_anim_t *a) { (void)a; weld_done_destroy(); }
static autoclose_anim_hold_t weld_done_anim_hold = { &weld_done_bar, 100, 0, 5000, weld_done_bar_anim_cb, weld_done_anim_ready_cb, false, 0, 0 };

static void weld_done_build(unsigned long elapsed_ms, int frames)
{
    weld_done_destroy();
    // کارت شناور بالای صفحه؛ پس‌زمینهٔ تمام‌صفحه‌ی بلاک‌کننده ندارد تا جلوی هیچ‌چیز را نگیرد.
    weld_done_dlg = lv_obj_create(lv_layer_top());
    lv_obj_add_event_cb(weld_done_dlg, weld_done_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(weld_done_dlg, 360, 196);
    lv_obj_align(weld_done_dlg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(weld_done_dlg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(weld_done_dlg, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &weld_done_anim_hold);
    lv_obj_set_style_bg_color(weld_done_dlg, lv_color_hex(0xff1f2937), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(weld_done_dlg, 245, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(weld_done_dlg, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(weld_done_dlg, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(weld_done_dlg, lv_color_hex(0xff2e7d32), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(weld_done_dlg, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    // دکمهٔ × بستن (بالا راست)
    lv_obj_t *btn_x = lv_btn_create(weld_done_dlg);
    weld_done_close_btn = btn_x;
    lv_obj_set_size(btn_x, 30, 30);
    lv_obj_align(btn_x, LV_ALIGN_TOP_RIGHT, 4, -4);
    lv_obj_set_style_bg_color(btn_x, lv_color_hex(0xffae4c4c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_x, weld_done_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(btn_x);
    lv_obj_set_style_text_font(xl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, "X");
    lv_obj_center(xl);

    lv_obj_t *title = lv_label_create(weld_done_dlg);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(title, &ui_font_kalameh_bl_ack_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff64e270), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("جوش کامل شد", "Weld Complete", "Kaynak Tamamlandi"));

    unsigned long secs = elapsed_ms / 1000UL;
    char l1[64];
    snprintf(l1, sizeof(l1), ML("زمان کل: %lu ثانیه", "Total time: %lu s", "Toplam sure: %lu sn"), secs);
    lv_obj_t *lbl1 = lv_label_create(weld_done_dlg);
    lv_obj_align(lbl1, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(lbl1, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl1, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl1, l1);

    char l2[96];
    int wn = g_weld_done_in_frame; // ۱..۴ شمارهٔ لنگه در این چهارچوب
    if (wn >= 4) {
        snprintf(l2, sizeof(l2), ML("4 لنگه شد — یک چهارچوب ثبت شد (کل: %d)",
                                    "4 pieces done — 1 frame saved (total: %d)",
                                    "4 parca — 1 cerceve kaydedildi (toplam: %d)"), frames);
    } else {
        snprintf(l2, sizeof(l2), ML("لنگه: %d از 4   (چهارچوب: %d)",
                                    "Piece: %d of 4   (frames: %d)",
                                    "Parca: %d/4   (cerceve: %d)"), wn, frames);
    }
    lv_obj_t *lbl2 = lv_label_create(weld_done_dlg);
    lv_obj_align(lbl2, LV_ALIGN_TOP_MID, 0, 84);
    lv_obj_set_style_text_font(lbl2, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl2, l2);

    // نوار کوچک که طی ۴ ثانیه خالی می‌شود
    weld_done_bar = lv_bar_create(weld_done_dlg);
    lv_obj_set_size(weld_done_bar, 332, 8);
    lv_obj_align(weld_done_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_range(weld_done_bar, 0, 100);
    lv_bar_set_value(weld_done_bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(weld_done_bar, lv_color_hex(0xff2e7d32), LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, weld_done_bar);
    lv_anim_set_values(&a, 100, 0);
    lv_anim_set_time(&a, 5000); // ۱ ثانیه بیشتر نمایش (۴→۵ ثانیه)
    lv_anim_set_exec_cb(&a, weld_done_bar_anim_cb);
    lv_anim_set_ready_cb(&a, weld_done_anim_ready_cb);
    lv_anim_start(&a);
}

// ===== خطای سنسور جک: کارتِ مودال با نامِ دقیقِ سنسور و دکمهٔ × =====
static lv_obj_t *sensor_err_dlg = NULL;
static lv_obj_t *sensor_err_bar = NULL;
static lv_obj_t *sensor_err_close_btn = NULL;
static lv_obj_t *main_nav_sensor_err_close(void) { return sensor_err_close_btn; }
static void sensor_err_destroy() { if (sensor_err_dlg) { lv_obj_del(sensor_err_dlg); sensor_err_dlg = NULL; sensor_err_bar = NULL; sensor_err_close_btn = NULL; } }
static void sensor_err_deleted_cb(lv_event_t *e) { (void)e; sensor_err_dlg = NULL; sensor_err_bar = NULL; sensor_err_close_btn = NULL; }
static void sensor_err_close_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) sensor_err_destroy(); }
static void sensor_err_bar_anim_cb(void *obj, int32_t value) { lv_bar_set_value((lv_obj_t *)obj, value, LV_ANIM_OFF); }
static void sensor_err_timeout_cb(lv_anim_t *a) { (void)a; sensor_err_destroy(); }
static autoclose_anim_hold_t sensor_err_anim_hold = { &sensor_err_bar, 100, 0, 6000, sensor_err_bar_anim_cb, sensor_err_timeout_cb, false, 0, 0 };

static const char *sensor_err_text(int id)
{
    switch (id) {
        case 1: return ML("سنسور شابلون (بسته) فرمان نداد", "Shablon (closed) sensor gave no signal", "Sablon (kapali) sensoru sinyal vermedi");
        case 2: return ML("سنسور شابلون (باز) فرمان نداد", "Shablon (open) sensor gave no signal", "Sablon (acik) sensoru sinyal vermedi");
        case 3: return ML("سنسور میز (باز) فرمان نداد", "Table (open) sensor gave no signal", "Tabla (acik) sensoru sinyal vermedi");
        case 4: return ML("سنسور المنت (بسته) فرمان نداد", "Element (closed) sensor gave no signal", "Rezistans (kapali) sensoru sinyal vermedi");
        case 5: return ML("سنسور المنت (باز) فرمان نداد", "Element (open) sensor gave no signal", "Rezistans (acik) sensoru sinyal vermedi");
        case 6: return ML("سنسور میز هنگام بسته شدن فرمان نداد", "Table (closing) sensor gave no signal", "Tabla (kapanirken) sensoru sinyal vermedi");
        case 7: return ML("شابلون در شروع باز نیست", "Shablon is not open at start", "Sablon baslangicta acik degil");
        case 8: return ML("المنت در شروع باز نیست", "Element is not open at start", "Rezistans baslangicta acik degil");
        case 9: return ML("میز در شروع باز نیست", "Table is not open at start", "Tabla baslangicta acik degil");
    }
    return "";
}

static void sensor_err_build(int id)
{
    sensor_err_destroy();
    sensor_err_dlg = lv_obj_create(lv_layer_top());
    lv_obj_add_event_cb(sensor_err_dlg, sensor_err_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(sensor_err_dlg, 480, 320);
    lv_obj_set_pos(sensor_err_dlg, 0, 0);
    lv_obj_clear_flag(sensor_err_dlg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(sensor_err_dlg, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &sensor_err_anim_hold);
    lv_obj_set_style_bg_color(sensor_err_dlg, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sensor_err_dlg, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sensor_err_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(sensor_err_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *card = lv_obj_create(sensor_err_dlg);
    lv_obj_set_size(card, 452, 285);
    lv_obj_set_style_align(card, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xff1f2937), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0xffe64c4c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(card, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &sensor_err_anim_hold);

    lv_obj_t *btn_x = lv_btn_create(card);
    sensor_err_close_btn = btn_x;
    lv_obj_set_size(btn_x, 34, 34);
    lv_obj_align(btn_x, LV_ALIGN_TOP_RIGHT, 4, -4);
    lv_obj_set_style_bg_color(btn_x, lv_color_hex(0xffe64c4c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_x, sensor_err_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(btn_x);
    lv_obj_set_style_text_font(xl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, "X");
    lv_obj_center(xl);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_font(title, &ui_font_kalameh_bl_ack_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffe64c4c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(title, ML("خطای سنسور", "Sensor Error", "Sensor Hatasi"));

    lv_obj_t *msg = lv_label_create(card);
    lv_obj_add_flag(msg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(msg, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &sensor_err_anim_hold);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_size(msg, 436, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(msg, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(msg, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    if ((id & 0x100) != 0) {
        char all_errors[512] = "";
        int mask = id & 0x07;
        if (mask & 1) snprintf(all_errors + strlen(all_errors), sizeof(all_errors) - strlen(all_errors), "%s\n", sensor_err_text(7));
        if (mask & 2) snprintf(all_errors + strlen(all_errors), sizeof(all_errors) - strlen(all_errors), "%s\n", sensor_err_text(8));
        if (mask & 4) snprintf(all_errors + strlen(all_errors), sizeof(all_errors) - strlen(all_errors), "%s\n", sensor_err_text(9));
        size_t len = strlen(all_errors);
        if (len > 0) all_errors[len - 1] = '\0';
        lv_label_set_text(msg, all_errors);
    } else if (g_sensor_err_both) {
        const char *jk = (id == 1 || id == 2) ? ML("شابلون", "Shablon", "Sablon")
                       : (id == 4 || id == 5) ? ML("المنت", "Element", "Rezistans")
                                              : ML("میز", "Table", "Tabla");
        char both_msg[160];
        snprintf(both_msg, sizeof(both_msg),
            ML("هر دو سنسورِ %s (باز و بسته) هم‌زمان فعال‌اند — حالت نامعتبر",
               "%s: both sensors (open & closed) active — invalid",
               "%s: iki sensor de aktif (acik+kapali) — gecersiz"), jk);
        lv_label_set_text(msg, both_msg);
    } else {
        lv_label_set_text(msg, sensor_err_text(id));
    }
    g_sensor_err_both = false; // مصرف شد

    // راهنمای کارکردِ بدونِ سنسور — بعدِ همهٔ نوشته‌های خطا، در پایینِ پیغام
    // نکته: اول اندازه/فونت/متن ست شود، بعد align — وگرنه align روی اندازهٔ پیش‌فرض (صفر) حساب می‌شود و گوشه می‌نشیند.
    lv_obj_t *note = lv_label_create(card);
    lv_obj_add_flag(note, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(note, autoclose_anim_hold_event_cb, LV_EVENT_ALL, &sensor_err_anim_hold);
    lv_obj_set_size(note, 420, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(note, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(note, lv_color_hex(0xffb9c2cc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(note, (get_ui_language_index() == 0) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(note, ML(
        "اگر می‌خواهید دستگاه بدون سنسورهای جک و فقط بر اساس زمان‌بندی کار کند، می‌توانید از تنظیمات مدیر حالت سنسور را غیرفعال کنید (رمز فعال‌سازی را باید از شرکت سازنده دریافت کنید).",
        "If you want the device to run without jack sensors, using time-based control only, you can disable sensor mode from Admin Settings (the activation code must be obtained from the manufacturer).",
        "Cihazin kriko sensorleri olmadan, sadece zamanlamaya gore calismasini istiyorsaniz, Yonetici Ayarlarindan sensor modunu devre disi birakabilirsiniz (etkinlestirme kodunu ureticiden almaniz gerekir)."));
    lv_obj_align(note, LV_ALIGN_TOP_MID, 0, 149); // ۱۴۰ (موقعیتِ قبلی) + ۹px پایین‌تر

    lv_obj_t *sub = lv_label_create(card);
    lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_obj_set_style_text_font(sub, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xffaeb7be), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(sub, ML("برنامه متوقف شد", "Program stopped", "Program durdu"));

    lv_obj_t *bar = lv_bar_create(card);
    sensor_err_bar = bar;
    lv_obj_set_size(bar, 420, 10);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xff394657), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xffe64c4c), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_values(&a, 100, 0);
    lv_anim_set_time(&a, 6000);
    lv_anim_set_exec_cb(&a, sensor_err_bar_anim_cb);
    lv_anim_set_ready_cb(&a, sensor_err_timeout_cb);
    lv_anim_start(&a);
}

// آیتم: دیالوگِ «بازگردانیِ پایه‌ها به حالتِ کارخانه» — پیش‌اعلان (تعریفِ کامل پایین‌تر، کنارِ io_remap_*)
static lv_obj_t *io_reset_dlg = NULL;
static lv_obj_t *io_reset_bar = NULL;
static uint32_t  io_reset_tick = 0;
static uint32_t  io_reset_hold_tick = 0;
static void io_reset_close(void);
// آیتم: نوارِ خودکاربستنِ ۸ثانیه‌ایِ دیالوگِ جابجاییِ پایه (پیش‌اعلان — تعریفِ کامل کنارِ io_remap_*)
static lv_obj_t *io_remap_bar = NULL;
static uint32_t  io_remap_tick = 0;
static uint32_t  io_remap_hold_tick = 0;
#define IO_REMAP_AUTO_CLOSE_MS 8000

// پیام هشدار پدال در منوی تنظیمات (عدم شروع جوش خارج از صفحه اصلی)
static lv_obj_t *s_settings_pedal_warn_overlay = NULL;
static uint32_t s_settings_pedal_warn_open_tick = 0;

static void settings_pedal_warn_dismiss(void)
{
    if (s_settings_pedal_warn_overlay && lv_obj_is_valid(s_settings_pedal_warn_overlay)) {
        lv_obj_del(s_settings_pedal_warn_overlay);
    }
    s_settings_pedal_warn_overlay = NULL;
    s_settings_pedal_warn_open_tick = 0;
}

static void settings_pedal_warn_overlay_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        settings_pedal_warn_dismiss();
    }
}

static void settings_pedal_warn_toast_build(void)
{
    settings_pedal_warn_dismiss();

    lv_obj_t *top = lv_layer_top();
    if (!top) return;

    s_settings_pedal_warn_overlay = lv_obj_create(top);
    lv_obj_set_size(s_settings_pedal_warn_overlay, 440, 84);
    lv_obj_align(s_settings_pedal_warn_overlay, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_clear_flag(s_settings_pedal_warn_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_settings_pedal_warn_overlay, settings_pedal_warn_overlay_event_cb, LV_EVENT_CLICKED, NULL);

    // استایل صنعتی پرکنتراست (نوک مدادی تیره با کادر زرد هشدار و گوشه‌های گرد)
    lv_obj_set_style_bg_color(s_settings_pedal_warn_overlay, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_settings_pedal_warn_overlay, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_settings_pedal_warn_overlay, lv_color_hex(0xfff59e0b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_settings_pedal_warn_overlay, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_settings_pedal_warn_overlay, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_settings_pedal_warn_overlay, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_settings_pedal_warn_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *lbl = lv_label_create(s_settings_pedal_warn_overlay);
    lv_obj_set_width(lbl, 416);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xfffef3c7), LV_PART_MAIN | LV_STATE_DEFAULT);

    int lang = get_ui_language_index();
    if (lang == 0) {
        lv_obj_set_style_text_font(lbl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl, "⚠️ برای اجرای جوش، ابتدا با کلید بازگشت (🏠) به صفحه اصلی بروید");
    } else if (lang == 1) {
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl, "⚠️ Return to Main Screen (🏠) to start welding cycle");
    } else {
        lv_obj_set_style_text_font(lbl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl, "⚠️ Kaynak baslatmak icin ana ekrana (🏠) donun");
    }
    lv_obj_center(lbl);

    s_settings_pedal_warn_open_tick = lv_tick_get();
}

volatile bool g_mfg_setup_request = false;

// در کانتکستِ LVGL اجرا می‌شود (lv_timer). overlay را با وضعیت قفل هماهنگ می‌کند.
static void lock_overlay_timer_cb(lv_timer_t *t)
{
    (void)t;
    // هشدار پدال خارج از صفحه اصلی (منوی تنظیمات)
    if (g_pedal_outside_menu_warn_request)
    {
        g_pedal_outside_menu_warn_request = 0;
        settings_pedal_warn_toast_build();
    }
    // هشدار اتمام مهلت ۱۵ دقیقه‌ای تست و قفل گارانتی
    if (g_warranty_expired_dialog_request)
    {
        g_warranty_expired_dialog_request = false;
        open_warranty_expired_dialog();
    }
    if (g_warranty_screen_request)
    {
        g_warranty_screen_request = false;
        open_warranty_screen();
    }
    if (g_mfg_setup_request)
    {
        g_mfg_setup_request = false;
        mfg_phone_setup_build();
    }
    if (s_settings_pedal_warn_overlay && s_settings_pedal_warn_open_tick != 0 &&
        lv_tick_elaps(s_settings_pedal_warn_open_tick) >= 3500)
    {
        settings_pedal_warn_dismiss();
    }

    // خطای سنسور جک: درخواست از سمتِ task
    if (g_sensor_err_request)
    {
        g_sensor_err_request = 0;
        sensor_err_build(g_sensor_err_id);
    }
    // پیغام پایان جوش (#۳): درخواست از سمتِ task
    if (g_weld_done_request)
    {
        g_weld_done_request = 0;
        long today = today_jdn_from_rtc();
        if (today != 0) {
            storage.addDailyWeldStats(today, (uint32_t)g_weld_done_elapsed_ms, 1);
        }
        weld_done_build(g_weld_done_elapsed_ms, g_weld_done_frames);
    }
    // آیتم ۲: درخواستِ دیالوگ دما از سمتِ task
    if (g_temp_warn_request)
    {
        g_temp_warn_request = 0;
        if (!temp_dlg && !g_device_locked) temp_dlg_build();
    }
    // آیتم: جریانِ خودکارِ F5-بوت — بعد از تاییدِ رمزِ بروزرسانی (روی SCREEN_ID_SCREEN_PASSWORD)،
    // خودِ صفحه ناوبری نمی‌کند (فقط متغیرِ تاییدشدن را true می‌کند)، پس اینجا لبهٔ بالارونده را رصد
    // می‌کنیم و مستقیم به صفحهٔ وای‌فای می‌رویم. فقط در حالتِ ota_enable (یعنی وقتی از این مسیر آمده‌ایم)
    // آیتم: جریانِ F5-بوت — بعد از یک تاخیرِ کوتاه (تا لگو واقعاً یک لحظه دیده شود)، از لگو به صفحهٔ رمز.
    // فقط وقتی هنوز روی لگوییم و هیچ overlayِ ثبت‌نامِ اولِ بوت باز نیست (تا تداخل نکند).
    if (g_f5_password_pending && !company_overlay && !admin_setup_overlay &&
        eez_flow_get_current_screen() == SCREEN_ID_SCREEN_SPLASH &&
        lv_tick_elaps(g_f5_boot_tick) >= 500)
    {
        g_f5_password_pending = false;
        eez_flow_set_screen(SCREEN_ID_SCREEN_PASSWORD, LV_SCR_LOAD_ANIM_NONE, 0, 0);
    }
    // و فقط وقتی واقعاً روی همین صفحه‌ایم — تا با مسیرِ عادیِ تنظیمات مدیر تداخل نکند.
    {
        static bool prevAdminVerified = false;
        bool curAdminVerified = get_var_is_admin_password_verifyed();
        if (curAdminVerified && !prevAdminVerified && get_var_ota_enable() &&
            eez_flow_get_current_screen() == SCREEN_ID_SCREEN_PASSWORD)
        {
            eez_flow_set_screen(SCREEN_ID_SCREEN_WIFI, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        }
        prevAdminVerified = curAdminVerified;
    }
    // آیتم: اتصالِ وای‌فای در حالتِ OTA موفق شد (پرچم از ota_task) → برو به صفحهٔ کارت‌های نصب.
    // امنیت: اتصالِ وای‌فایِ ذخیره‌شده از همان setup() در پس‌زمینه شروع می‌شود (تا سریع‌تر آماده باشد)
    // و ممکن است *قبل* از تاییدِ رمزِ مدیر کامل شود — پس این پرچم را فوراً مصرف نمی‌کنیم؛ تا وقتی
    // واقعاً رویِ صفحهٔ وای‌فا نیستیم (که فقط بعد از تاییدِ رمز به آن می‌رسیم) صبر می‌کنیم. اگر پرچم
    // زودتر برسد، true می‌ماند و همین که کاربر رمز را درست وارد کرد و به صفحهٔ وای‌فا رسید، بلافاصله
    // (بدونِ نیازِ به یک اسکن/اتصالِ دوباره) به کارت‌های نصب می‌رود.
    // امنیت: چون اتصالِ وای‌فایِ ذخیره‌شده در setup() مستقل از رمزِ مدیر شروع می‌شود، ممکن است این پرچم
    // زودتر true شود؛ برای جلوگیریِ از دورزدنِ صفحهٔ رمز، عبور به کارت‌های نصب فقط وقتی رمزِ مدیر
    // تایید شده باشد انجام می‌شود (نه فقط بودن روی صفحهٔ وای‌فای).
    if (g_ota_go_framware_request && eez_flow_get_current_screen() == SCREEN_ID_SCREEN_WIFI &&
        get_var_is_admin_password_verifyed())
    {
        g_ota_go_framware_request = false;
        if (objects.drop_down_wifi_list && lv_dropdown_is_open(objects.drop_down_wifi_list))
            lv_dropdown_close(objects.drop_down_wifi_list);
        eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
    }
    // آیتم: وقتی کاربر از صفحهٔ وای‌فای/کارت‌های نصب خارج می‌شود (و در جریانِ F5-بوت یا self-update
    // نیستیم — آن دو خودشان وب‌اپ را برمی‌گردانند)، وب‌اپ/AP طبق تنظیماتِ عادی دوباره روشن شود.
    {
        static int16_t prevScreenForWifiGuard = -1;
        int16_t curScreenForWifiGuard = eez_flow_get_current_screen();
        bool wasWifiFlow = (prevScreenForWifiGuard == SCREEN_ID_SCREEN_WIFI ||
                            prevScreenForWifiGuard == SCREEN_ID_SCREEN_FRAMWARE ||
                            prevScreenForWifiGuard == SCREEN_ID_SCREEN_UPDATE); // FIX-WEBAPP-AP
        bool isWifiFlow = (curScreenForWifiGuard == SCREEN_ID_SCREEN_WIFI ||
                           curScreenForWifiGuard == SCREEN_ID_SCREEN_FRAMWARE ||
                           curScreenForWifiGuard == SCREEN_ID_SCREEN_UPDATE);   // FIX-WEBAPP-AP
        if (wasWifiFlow && !isWifiFlow && !get_var_ota_enable())
        {
            g_webapp_sync_requested = true;
            g_webapp_sync_restart = true;
        }
        prevScreenForWifiGuard = curScreenForWifiGuard;
    }
    // آیتم: شمارشِ معکوسِ ۵ثانیه‌ایِ دیالوگِ «بازگردانیِ پایه‌ها به حالتِ کارخانه» — مثلِ دیالوگِ ریستِ چهارچوب
    if (io_reset_dlg && io_reset_tick)
    {
        if (io_reset_hold_tick) {
            io_reset_tick += millis() - io_reset_hold_tick;
            io_reset_hold_tick = millis();
        } else {
            uint32_t el = lv_tick_elaps(io_reset_tick);
            if (el >= 5000) io_reset_close();
            else if (io_reset_bar) lv_bar_set_value(io_reset_bar, (int)(el * 100 / 5000), LV_ANIM_OFF);
        }
    }
    // آیتم: دیالوگِ جابجاییِ پایه بعد از ۸ ثانیهٔ بی‌پاسخ خودش را می‌بندد (نوارِ زیرش خالی می‌شود)
    if (io_remap_is_open() && io_remap_tick)
    {
        if (io_remap_hold_tick) {
            io_remap_tick += millis() - io_remap_hold_tick;
            io_remap_hold_tick = millis();
        } else {
            uint32_t el = lv_tick_elaps(io_remap_tick);
            if (el >= IO_REMAP_AUTO_CLOSE_MS) io_remap_destroy();
            else if (io_remap_bar) lv_bar_set_value(io_remap_bar, (int)(100 - el * 100 / IO_REMAP_AUTO_CLOSE_MS), LV_ANIM_OFF);
        }
    }
    // آیتم ۳: اولین بوت — «صفحه نام شرکت → صفحه رمز → لگو».
    // این دو صفحه روی پس‌زمینهٔ لوگو (splash) نمایش داده می‌شوند و تا کاربر تمام نکند به منو نمی‌رویم.
    // کرشِ قبلی (watchdog) مربوط به هم‌زمانیِ overlay با «انیمیشنِ گذارِ splash→menu» بود؛ چون الان
    // اصلاً آن انیمیشن اجرا نمی‌شود (مستقیم روی لگو می‌مانیم)، فقط یک تأخیرِ کوتاه برای پایدار شدنِ
    // اولیهٔ LVGL/تسک‌ها لازم است، نه ۴ ثانیهٔ کامل.
    static bool g_setup_held_splash = false; // فقط وقتی ما برای setup روی لوگو نگه داشتیم true می‌شود
    if (!g_is_crash_recovery_boot && !g_device_locked &&
        (!storage.isCompanyAsked() || !storage.isAdminAsked() || !storage.isMfgPhoneAsked()))
    {
        if (setup_stage_transitioning) return;
        // آیتم ۳: از همان ابتدای بوت اگر ثبت‌نام کامل نشده، صفحه را روی لگو نگه دار تا منو «اصلاً» دیده نشود.
        // (اسنپِ فوری و بدونِ انیمیشن؛ اگر فلوی EEZ ما را به منو ببرد، همان لحظه برمی‌گردیم روی لگو.)
        if (eez_flow_get_current_screen() != SCREEN_ID_SCREEN_SPLASH)
            eez_flow_set_screen(SCREEN_ID_SCREEN_SPLASH, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        g_setup_held_splash = true;
        if (!storage.isCompanyAsked()) {
            if (!company_overlay && !admin_setup_overlay && !mfg_phone_overlay) company_overlay_build();
        } else if (!storage.isAdminAsked()) {
            if (!admin_setup_overlay && !company_overlay && !mfg_phone_overlay) admin_setup_build();
        } else if (!storage.isMfgPhoneAsked()) {
            if (!mfg_phone_overlay && !company_overlay && !admin_setup_overlay) mfg_phone_setup_build();
        }
    }
    else if (g_setup_held_splash && !g_device_locked && !company_overlay && !admin_setup_overlay && !mfg_phone_overlay &&
             storage.isCompanyAsked() && storage.isAdminAsked() && storage.isMfgPhoneAsked())
    {
        // پایانِ setup → از لوگو به منو (فقط اگر خودمان لوگو را برای setup نگه داشته بودیم)
        g_setup_held_splash = false;
        if (eez_flow_get_current_screen() == SCREEN_ID_SCREEN_SPLASH) {
            eez_flow_set_screen(SCREEN_ID_SCREEN_MENU, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        }
    }
    if (g_device_locked && eez_flow_get_current_screen() == SCREEN_ID_SCREEN_MENU)
    {
        if (!lock_overlay) lock_overlay_build();
        if (lock_overlay_info)
        {
            static int lock_info_last_secs = -1;
            uint32_t now = millis();
            const bool info_on_hold = (lock_info_hold_until_ms != 0 && now < lock_info_hold_until_ms);
            DateTime nr = rtc_now_s();
            unsigned long nowEpoch = (nr.year() >= 2020) ? (unsigned long)nr.unixtime() : 0;
            unsigned long remain = (nowEpoch != 0) ? storage.adminLockRemainingSec(nowEpoch) : 0;
            if (remain > 0 || now < lock_cooldown_until_ms)
            {
                lock_info_hold_until_ms = 0;
                int secs = (remain > 0) ? (int)remain : (int)((lock_cooldown_until_ms - now) / 1000UL);
                if (secs != lock_info_last_secs)
                {
                    lock_info_last_secs = secs;
                    char buf[72];
                    snprintf(buf, sizeof(buf), ML("۳ بار رمز اشتباه — تا تلاش بعدی %d:%02d", "3 wrong attempts — retry in %d:%02d", "3 yanlis — %d:%02d sonra"), secs / 60, secs % 60);
                    lock_label_set_if_changed(lock_overlay_info, buf);
                    lv_obj_set_style_text_color(lock_overlay_info, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
            else if (!info_on_hold)
            {
                if (lock_info_last_secs != -2)
                {
                    lock_info_last_secs = -2;
                    lock_label_set_if_changed(lock_overlay_info, ML("رمز را وارد کنید", "Enter Password", "Sifre Girin"));
                    lv_obj_set_style_text_color(lock_overlay_info, lv_color_hex(0xffffac38), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    else
    {
        if (lock_overlay) lock_overlay_destroy();
    }
}

static void disp_sensor_task(void *pv); // پیش‌اعلان — تعریف پایین‌تر
static void fw_out_pins_refresh(void); // پیش‌اعلان — تعریف پایین‌تر (بعد از storage.loadPinMap باید صدا زده شود)
static void sync_input_globals_from_rows(void); // هماهنگ‌سازی SENSOR_*/EMERGENCY/PEDAL با inRowPin[]
static void pin_sync_last_owners_from_map(void);

void setup()
{
    // لچ اولیه وضعیت بوت: حتماً اولین خط setup قبل از هرگونه رندر یا کالبک
    ui_resume_boot_latch_early();
    boot_guard_early_check(); // گارد کرش زودهنگام: تشخیص بوت‌لوپ و رول‌بک خودکار
    setCpuFrequencyMhz(240);  // قفل فرکانس پردازنده روی ۲۴۰ مگاهرتز (حداکثر توان پردازشی زیمنس/اپل)
    set_var_software_version(VERSION);
    set_var_temp_offset(1);
    Serial.begin(921600);
    Wire.end();
    delay(g_is_crash_recovery_boot ? 5 : 100);

    if (!adcMutex) {
        adcMutex = xSemaphoreCreateMutex();
    }
    if (!rtcMutex) {
        rtcMutex = xSemaphoreCreateMutex();
    }
    if (!xMutex_Wifi) {
        xMutex_Wifi = xSemaphoreCreateMutex();
    }
    WiFi.onEvent(wifi_diag_event); // لاگِ تشخیصیِ رویدادهای WiFi (علتِ اتصال ناموفق)
    // آیتم: پایه‌های جابجاشده باید پیش از setupPin() (که pinMode/attenuation را می‌زند) خوانده شوند.
    storage.loadPinMap();
    sync_input_globals_from_rows();
    pin_sync_last_owners_from_map();
    fw_out_pins_refresh(); // آرایهٔ تستِ دستیِ منوی اطلاعات را هم با مقدارِ تازه هماهنگ کن
    setupPin();
    delay(g_is_crash_recovery_boot ? 5 : 100);
    if (!rtc.begin())
    {
        // قبلاً اینجا `while(1) delay(10);` بود — یعنی اگر RTC روی برد نبود/خراب بود، دستگاه قبل از
        // حتی راه‌اندازیِ نمایشگر (lv_init/tft.begin پایین‌تر است) برای همیشه با صفحهٔ سیاه هنگ می‌کرد
        // و هیچ نشانه‌ای به کاربر داده نمی‌شد. حالا فقط پرچم می‌شود و بوت ادامه پیدا می‌کند؛ خطا در
        // overlayِ خطای سخت‌افزار روی صفحهٔ لگو نمایش داده می‌شود (hw_selfcheck_overlay_build).
        Serial.println("Couldn't find RTC");
        g_hw_rtc_missing = true;
    }
    Wire.setClock(400000); // تضمین فرکانس ۴۰۰ کیلوهرتز باس I2C از ابتدای بوت برای حذف لگ I2C

    if (!g_hw_rtc_missing && rtc.lostPower())
    {
        g_rtc_lost_power_boot = true;
        Serial.println("RTC lost power, setting time!");
        rtc_adjust_s(DateTime(F(__DATE__), F(__TIME__)));
    }
    // هشدارِ یک‌بارهٔ باتری: اگر RTC زمانش را از دست داده یا تاریخش نامعتبر است → باتری ضعیف/مرده
    if (!g_hw_rtc_missing)
    {
        DateTime chk = rtc_now_s();
        if (g_rtc_lost_power_boot || chk.year() < 2024) g_rtc_warn_pending = true;
    }
    DateTime now = rtc_now_s();
    char clock_text[8];
    snprintf(clock_text, sizeof(clock_text), "%02d:%02d", now.hour(), now.minute());
    set_var_header_time_text(clock_text);
    int header_jy = 0, header_jm = 0, header_jd = 0;
    date_utils::gregorian_to_jalali(now.year(), now.month(), now.day(), header_jy, header_jm, header_jd);
    char header_date_text[16];
    snprintf(header_date_text, sizeof(header_date_text), "%04d/%02d/%02d", header_jy, header_jm, header_jd);
    set_var_header_date_text(header_date_text);
    char cpu_temp_text[16];
    snprintf(cpu_temp_text, sizeof(cpu_temp_text), "%.0f\xC2\xB0", temperatureRead());
    set_var_cpu_temperature_text(cpu_temp_text);
    char internal_temp_text[16];
    snprintf(internal_temp_text, sizeof(internal_temp_text), "%.1f\xC2\xB0", rtc_temp_s());
    set_var_internal_temperature_text(internal_temp_text);
    set_var_admin_set_hour(now.hour());
    set_var_admin_set_minute(now.minute());
    int jy = 0, jm = 0, jd = 0;
    date_utils::gregorian_to_jalali(now.year(), now.month(), now.day(), jy, jm, jd);
    set_var_admin_set_year(jy);
    set_var_admin_set_month(jm);
    set_var_admin_set_day(jd);

    // Serial.print(now.year());
    // Serial.print('/');
    // Serial.print(now.month());
    // Serial.print('/');
    // Serial.print(now.day());
    // Serial.print(" ");
    // Serial.print(now.hour());
    // Serial.print(':');
    // Serial.print(now.minute());
    // Serial.print(':');
    // Serial.println(now.second());


    // راه اندازی سیستم ثبت وقایع خطای ISA-18.2 و خودآزمایی سخت افزار POST
    fault_log_init();
    boot_diag_run_self_test();

    // بازیابی خودکار به صفحه قبل در صورت هنگ یا کرش (حافظه پایدار RTC Slow Memory)
    int16_t saved_screen = ui_resume_boot_get_screen();
    if (g_is_crash_recovery_boot) {
        Serial.printf("[RESET-RECOVERY] Auto-resuming directly to screen %d (hang recovery)\n", (int)saved_screen);
    }

    if (!g_is_crash_recovery_boot) {
        delay(1000);
    } else {
        delay(10); // بوت فوری بدون اتلاف وقت
    }
    checkBoot();

    // ورود به مدِ OTAِ آزموده‌شده از طریقِ دکمهٔ منوی بروزرسانی (به‌جای اسکنِ زندهٔ ناپایدار).
    // فلگ در RAMِ RTC است: ری‌استارتِ نرم‌افزاری آن را نگه می‌دارد، ولی قطعِ کاملِ برق پاکش می‌کند
    // (یعنی با خاموش/روشن همیشه به حالت عادی برمی‌گردد). فقط روی ری‌ستِ نرم‌افزاری معتبر است.
    if (esp_reset_reason() == ESP_RST_SW && g_ota_boot_request == OTA_BOOT_MAGIC) {
        g_ota_boot_request = 0;          // یک‌بارمصرف؛ ری‌ستِ بعدی → حالت عادی
        set_var_ota_enable(true);
        // آیتم: اگر این ری‌استارت از دکمهٔ «بروزرسانی» صفحهٔ اطلاعات بود (نه F5ِ فیزیکی)، بعد از
        // اتصالِ وای‌فای باید self_update_check() صدا زده شود (چکِ نسخه + نصبِ درجا)، نه صفحهٔ
        // کارت‌های نصبِ عمومی. g_self_update_session هم فعال می‌شود تا overlayِ UIِ درجا رسم شود.
        if (g_self_update_boot_request == SELF_UPDATE_BOOT_MAGIC) {
            g_self_update_boot_request = 0;
            request_self_update_after_wifi();
            g_self_update_session = true;
        }
    }
    // Control pipeline on core 0 to reduce UI contention.
    if (get_var_ota_enable()){
        // آیتم: هرچه زودتر AP (که کاربران دیگر آن را با نامِ HAFFLER می‌بینند) کاملاً خاموش و رادیو خالصِ
        // STA شود — تا وقتی کاربر دارد رمزِ مدیر را تایپ می‌کند، اتصال به وای‌فایِ ذخیره‌شده (پایین‌تر)
        // در پس‌زمینه فرصتِ کافی برای کامل‌شدن داشته باشد و بعد از تاییدِ رمز اصلاً طولی نکشد.
        WiFi.persistent(false);
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
    }else{
        webAppEnabled = false;
    }
    // === صیانت از NVS: طبق مرام‌نامه AGENTS.md هرگز نباید NVS به صورت سراسری پاک شود ===
    // دستور خطرناک nvs_flash_erase حذف شد تا کالیبراسیون، روزشمار، پین‌مپ و وای‌فای حفظ شوند.
    storage.begin();
    // یک‌بار جبرانِ باگ قبلی (هر بوت setWifiOn(false)): وب‌اپ دوباره پیش‌فرض روشن
    {
        Preferences wf; wf.begin("wipe_ctl", false);
        if (wf.getUChar("wifiFix", 0) != 1) {
            storage.setWifiOn(true);
            wf.putUChar("wifiFix", 1);
            Serial.println("[WIFI] restored WifiOn=true after boot-kill bug");
        }
        wf.end();
    }
    company_refresh_global(); // آیتم ۳: نام شرکت را از حافظه بخوان تا لیبل‌های برند درست ساخته شوند
    warranty_init();          // راه‌اندازی زیرسیستم گارانتی رسمی، شمارش معکوس ۱۵ دقیقه‌ای و کد مشتری
    // === (حذف‌شده) FORCE-UNLOCK یک‌باره ===
    // این خط قبلاً هر بوت روزشمار را پاک می‌کرد، برای همین تنظیمِ تایمرِ معکوس بعد از روشن‌شدن
    // نمایش داده نمی‌شد و دستگاه هرگز قفل نمی‌شد. حذف شد تا روزشمار ماندگار بماند.
    // storage.clearLockAndCountdown();   // ← عمداً غیرفعال
    storage.clearAdminLock(); // فقط جریمهٔ ۱۰ دقیقه‌ایِ رمز اشتباه پاک شود (بی‌ضرر)
    {
        DateTime bootNow = rtc_now_s();
        if (bootNow.year() >= 2020) {
            storage.refreshAdminLockOnBoot((unsigned long)bootNow.unixtime(), 10UL * 60UL);
        }
    }
    // پاک‌کردنِ یک‌بارهٔ تاریخِ گارانتیِ گیرکرده — با handleِ خودِ storage (نه Preferences دومِ هم‌نام
    // که باعث می‌شد نوشتنِ پاک‌سازی گم شود). token را بالا ببر تا یک‌بارِ دیگر پاک شود.
    storage.consumeWarrantyClearOnce(6);
    // === رفعِ ریشه‌ایِ تاریخِ غلطِ گارانتی ===
    // باتریِ RTC مقدارِ قدیمیِ غلط (مثلاً ۱۸ فروردین) را نگه می‌دارد و چون lostPower=false،
    // بوت آن را اصلاح نمی‌کند. این‌جا یک‌بار ساعت را به تاریخِ بیلد (امروز) force-set می‌کنیم.
    if (storage.consumeClockForceSetOnce(1)) {
        rtc_adjust_s(DateTime(BUILD_YEAR, BUILD_MONTH, BUILD_DAY, BUILD_HOUR, BUILD_MINUTE, 0));
        Serial.println("[CLOCK] force-set to build date");
    }
    // صیانت از فلگ‌های ورود اولیه: نام شرکت و رمز مدیر فقط یک‌بار دریافت و برای همیشه در NVS ماندگار می‌مانند
    // if (storage.consumeFirstBootResetOnce(17)) {
    //     Serial.println("[SETUP] first-boot flags cleared (company/admin)");
    // }
    {   // لاگِ تشخیصی
        DateTime dnow = rtc_now_s();
        Serial.printf("[DIAG] rtc=%04d-%02d-%02d %02d:%02d lostPower=%d warranty='%s'\n",
                      dnow.year(), dnow.month(), dnow.day(), dnow.hour(), dnow.minute(),
                      (int)rtc.lostPower(), storage.getWarrantyDate().c_str());
    }
    g_splash_variant = storage.consumeSplashAlternate();
    if (get_var_ota_enable() && storage.getWifiOn() && storage.getWifiSsid().length() > 0) {
        selectedWifi = storage.getWifiSsid();
        wifiPassword = storage.getWifiPassword();
        connectingToWifi = true;
    }
    // زبان را بعد از لودِ حافظه بازیابی کن تا با خاموش/روشن باقی بماند
    int storedLang = storage.getLang();
    set_var_lang(storedLang);
    eez_flow_set_language_index(storedLang == 3 ? 2 : (storedLang == 2 ? 1 : 0));
    int32_t stored_total_frames = abs(storage.get_frameNumbers());
    set_var_total_frame_numbers(stored_total_frames);
    set_var_frame_numbers(abs(storage.get_frameNumbersCur())); // جاریِ موندگار (سرِ بوت پاک نشود)

    lv_init();

    tft.begin();

    tft.setRotation(SCREEN_ROTATION);

    // تضمین فعال بودن گذرگاه سخت‌افزاری I2C با فرکانس ۴۰۰ کیلوهرتز
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(400000);
    Wire.setTimeOut(10);
    // خودآزماییِ MAX6675 (سنسورِ دمای المنت): دو خوانشِ خامِ SPI (نه readCelsius) پشتِ‌سرهم.
    // v==0xFFFF پایدار روی هر دو خوانش یعنی هیچ‌چیزی روی باس جواب نمی‌دهد → خودِ چیپ نیست.
    // بیتِ D2 (0x4) ست ولی مقدار غیرِ 0xFFFFِ معتبر یعنی چیپ زنده و درست جواب می‌دهد، فقط سیمِ
    // ترموکوپل قطع/وصل‌نشده است — این را نباید «مشکلِ MAX6675» نامید.
    {
        uint16_t raw1 = thermocouple.readRaw();
        delay(g_is_crash_recovery_boot ? 5 : 250);
        uint16_t raw2 = thermocouple.readRaw();
        bool chip_stuck = (raw1 == 0xFFFF && raw2 == 0xFFFF);
        bool sensor_open = !chip_stuck && (raw1 & 0x4) && (raw2 & 0x4);
        if (chip_stuck) {
            g_hw_thermo_missing = true;
            Serial.println("[HWCHECK] MAX6675 chip not answering on SPI (stuck 0xFFFF)");
        } else if (sensor_open) {
            g_hw_thermo_sensor_open = true;
            Serial.println("[HWCHECK] MAX6675 chip OK, thermocouple wire not connected (open, bit D2)");
        }
    }
    // خودآزماییِ دمای داخلیِ بحرانی سرِ بوت (جدا از هشدارِ مرحله‌ایِ صفحهٔ اصلی که هر ۲ث چک می‌شود):
    // اگر همین لحظهٔ بوت دمای CPU یا RTC از ۶۵° بیشتر باشد، در overlayِ خطای سخت‌افزار مشخص می‌شود
    // کدام‌یک بالا رفته.
    {
        g_hw_cpu_temp_val = temperatureRead();
        g_fw_info_cpu_temp_c = g_hw_cpu_temp_val;
        if (g_hw_cpu_temp_val >= 65.0f) { g_hw_cpu_temp_high = true; Serial.printf("[HWCHECK] CPU temp high at boot: %.0fC\n", g_hw_cpu_temp_val); }
        if (!g_hw_rtc_missing) { // اگر خودِ RTC نیست، دمایش هم بی‌معنی است — مثبتِ کاذب نده
            g_hw_rtc_temp_val = rtc_temp_s();
            g_fw_info_rtc_temp_c = g_hw_rtc_temp_val;
            if (g_hw_rtc_temp_val >= 65.0f) { g_hw_rtc_temp_high = true; Serial.printf("[HWCHECK] RTC temp high at boot: %.1fC\n", g_hw_rtc_temp_val); }
        }
    }

    if (!display_port_init(&tft, &bbct)) {
        Serial.println("[LVGL] display_port_init failed");
    }
    g_hw_touch_missing = !display_port_touch_detected();
    if (g_hw_touch_missing) {
        Serial.println("[HWCHECK] touch controller not detected");
    } else {
        Serial.println("[HWCHECK] touch controller OK");
    }

    ui_init();
    lv_tick_set_cb((lv_tick_get_cb_t)millis);
    if (g_is_crash_recovery_boot) {
        if (saved_screen == SCREEN_ID_SCREEN_WIFI) {
            start_info_self_update();
        }
    }
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
    if (storage.getSysInfoBarEnabled()) {
        lv_sysmon_show_performance(NULL);
    } else {
        lv_sysmon_hide_performance(NULL);
    }
#endif
    // خودآزماییِ سخت‌افزار: فقط در بوت عادی؛ در بازیابی از کرش هیچ پیام یا هشداری مزاحم اپراتور نمی‌شود
    if (!g_is_crash_recovery_boot) {
        hw_selfcheck_overlay_build();
    }
    // آیتم ۳: اگر ثبت‌نامِ اولِ بوت باقی مانده، همین‌جا و همین حالا (قبل از اینکه هیچ تسکِ رندر/تایمری
    // اجرا شود) صفحهٔ نام شرکت/رمز مدیر ساخته می‌شود؛ یعنی همان اولین فریمی که LVGL رسم می‌کند از قبل
    // شاملِ این overlay است — دیگر هیچ فریمِ خالی/سیاهِ میانی قبل از آن دیده نمی‌شود.
    if (!g_is_crash_recovery_boot && (!storage.isCompanyAsked() || !storage.isAdminAsked() || !storage.isMfgPhoneAsked())) {
        eez_flow_set_screen(SCREEN_ID_SCREEN_SPLASH, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        if (!storage.isCompanyAsked()) company_overlay_build();
        else if (!storage.isAdminAsked()) admin_setup_build();
        else mfg_phone_setup_build();
    }
    lv_timer_create(panel_nav_timer_cb, 70, NULL);
    // آیتم ۳: از ۵۰۰ به ۵۰ کم شد — این تایمر مسئولِ ساختِ صفحهٔ نام شرکت/رمز مدیرِ اولِ بوت هم هست؛
    // با دورهٔ ۵۰۰ms، اولین فرصتِ اجرا ~نیم‌ثانیه بعد از بوت بود و لگوی خالی برای همان مدت دیده می‌شد.
    lv_timer_create(lock_overlay_timer_cb, 50, NULL);
    lv_timer_create(self_update_ui_cb, 100, NULL);   // UIِ بروزرسانیِ درجای صفحه اطلاعات
    lv_timer_create(self_update_pending_restart_timer_cb, 100, NULL); // بدونِ بلاک، دوباره‌تلاشِ شروعِ بروزرسانی
    lv_timer_create(hw_selfcheck_timer_cb, 500, NULL); // نوارِ بسته‌شدنِ overlayِ خطای سخت‌افزار
    lv_timer_create(company_len_err_timer_cb, 200, NULL); // نوارِ بسته‌شدنِ خطایِ طولِ نامِ شرکت
    g_last_activity_ms = millis();
    lv_timer_create(screensaver_idle_timer_cb, 1000, NULL); // بررسیِ بی‌کاری برای اسکرین‌سیور
    lv_timer_create(screensaver_cfg_timer_cb, 500, NULL);   // نوارِ بسته‌شدنِ ۸ثانیه‌ایِ دیالوگِ تنظیمِ اسکرین‌سیور
    if (get_var_ota_enable()){
        g_wifi_ota_scan_pending = true;
        set_var_available_wifi_networks("SEARCHING");
        // آیتم: نگه‌داشتنِ F5 موقعِ روشن‌شدن → صفحهٔ رمزِ بروزرسانی باز شود، ولی نه مستقیم و همین‌جا؛
        // چون هنوز هیچ فریمی رندر نشده، اگر مستقیم پرش کنیم درست است، اما برای اطمینانِ کامل از اینکه
        // «هیچ صفحهٔ دیگری (حتی صفحهٔ اصلی) حتی یک فریم دیده نشود» همان الگویِ ثبت‌نامِ اولِ بوت را تکرار
        // می‌کنیم: اول لگو، و سوییچِ واقعی به رمز با کمی تاخیر در lock_overlay_timer_cb (تا لگو واقعاً یک
        // لحظه دیده شود، نه فقط یک ست بی‌اثر که بلافاصله override شود).
        // اگر ثبت‌نامِ اولِ بوت هنوز باقی است، آن اول تمام شود (overlayِ بالا همین حالا لگو را نگه داشته).
        if (!g_is_crash_recovery_boot && storage.isCompanyAsked() && storage.isAdminAsked()) {
            eez_flow_set_screen(SCREEN_ID_SCREEN_SPLASH, LV_SCR_LOAD_ANIM_NONE, 0, 0);
            g_f5_password_pending = true;
            g_f5_boot_tick = lv_tick_get();
        }
    }
    // xTaskCreate(stack_monitor_task, "StackMonitor", 4096, NULL, 1, NULL);

    // Buzzer must exist before LVGL can load Screen_Menu and fire startup beep.
    xTaskCreatePinnedToCore(buzzer_task, "Buzzer", 2048, NULL, 2, &buzzerTaskHandle, 0);

    // تسک‌های گرافیکی (LVGL) - اولویت بالا، روی هسته ۱
    // UI pipeline on core 1 for deterministic frame timing.
    // xTaskCreatePinnedToCore(lv_tick_task, "LVGL Tick", 2048, NULL, 5, NULL, 1); // غیرفعال: تکیه بر lv_tick_set_cb(millis) جهت حذف ۱۰۰۰ تعویض کانتکست در ثانیه
    BaseType_t lv_ok = xTaskCreatePinnedToCore(lvgl_task, "LVGL", 8192, NULL, 4, &lvglTaskHandle, 1);
    if (lv_ok != pdPASS) {
        lv_ok = xTaskCreatePinnedToCore(lvgl_task, "LVGL", 7168, NULL, 4, &lvglTaskHandle, 1);
    }
    if (lv_ok != pdPASS) {
        xTaskCreatePinnedToCore(lvgl_task, "LVGL", 6144, NULL, 4, &lvglTaskHandle, 1);
    }

    // تسک‌های منطق برنامه و سخت‌افزار - اولویت پایین‌تر، روی هسته ۰
    if (get_var_ota_enable()){
        // استک کامل برای F5: ۱۰۲۴۰ کلمه با فال‌بک به ۸۱۹۲ و ۶۱۴۴ کلمه — پایداری ۱۰۰٪ TLS و استریم
        BaseType_t ok = xTaskCreatePinnedToCore(ota_task, "Ota", 10240, NULL, 1, &otaTaskHandle, 0);
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(ota_task, "Ota", 8192, NULL, 1, &otaTaskHandle, 0);
        }
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(ota_task, "Ota", 6144, NULL, 1, &otaTaskHandle, 0);
        }
        appTaskHandle = NULL;
    }else{
        spawn_webapp_task();
        spawn_app_task();
    }
    xTaskCreatePinnedToCore(keypad_task, "Keypad", 3072, NULL, 2, &keypadTaskHandle, 0);
    xTaskCreatePinnedToCore(disp_sensor_task, "DispSens", 2560, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(heater_task, "Heater", 4608, NULL, 3, &heaterTaskHandle, 0);
    update_lock_state();
    BaseType_t lk_ok = xTaskCreatePinnedToCore(lock_task, "Lock", 4096, NULL, 1, &lockTaskHandle, 0);
    if (lk_ok != pdPASS) {
        lk_ok = xTaskCreatePinnedToCore(lock_task, "Lock", 3072, NULL, 1, &lockTaskHandle, 0);
    }
    if (lk_ok != pdPASS) {
        xTaskCreatePinnedToCore(lock_task, "Lock", 2048, NULL, 1, &lockTaskHandle, 0);
    }

    agent_hmi_init(); // راه‌اندازی تسک پس‌زمینه اسکرین‌شات بدون وقفه در UI
    cloud_client_init(); // راه‌اندازی کلاینت ابری اینترنت اشیا (MQTT Core 0)

    Serial.printf("[MEM-DIAG] after boot tasks: internalFree=%u maxAlloc=%u psramFree=%u\n",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)ESP.getMaxAllocHeap(),
        (unsigned)(ESP.getPsramSize() ? ESP.getFreePsram() : 0));
    if (lvglTaskHandle)   Serial.printf("[STACK-DIAG] LVGL HW=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(lvglTaskHandle));
    if (webTaskHandle)    Serial.printf("[STACK-DIAG] WebApp HW=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(webTaskHandle));
    if (appTaskHandle)    Serial.printf("[STACK-DIAG] App HW=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(appTaskHandle));
    if (heaterTaskHandle) Serial.printf("[STACK-DIAG] Heater HW=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(heaterTaskHandle));
    if (keypadTaskHandle) Serial.printf("[STACK-DIAG] Keypad HW=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(keypadTaskHandle));
    if (lockTaskHandle)   Serial.printf("[STACK-DIAG] Lock HW=%u words\n", (unsigned)uxTaskGetStackHighWaterMark(lockTaskHandle));

    if (g_self_update_session && selfUpdateTaskHandle == NULL) {
        spawn_self_update_task();
    }

    if (get_var_ota_enable()){
        // آیتم: xMutex_Wifi از قبل (بالای همین setup()) با گاردِ !xMutex_Wifi ساخته شده؛ ساختِ دوباره
        // اینجا (بعد از استارتِ ota_task که همین حالا دارد از آن استفاده می‌کند) یک mutex/هندلِ قدیمی
        // را بدونِ حذف رها می‌کرد (نشتی) و باعث می‌شد ota_task و بقیهٔ کد دو شیءِ متفاوت را قفل کنند.
        set_var_available_wifi_networks("Scanning...");   // ASCII — dropdown با فونت montserrat
    }

    set_var_element_temperature(0);
    set_var_element_temperature_target(100);
    set_var_welding_time(17);
    set_var_cooling_time(14);
    set_var_emergency_stop(false);
    set_var_cycel_started(true);
    set_var_admin_can_save(false);

    int selectedUser = storage.getSelectedUser();
    String selectedUserName = ("F" + String(selectedUser));

    Serial.print("USER : ");
    Serial.println(selectedUser);

    set_var_enable_mode(selectedUserName.c_str());

    set_var_welding_time(storage.getWildingTime(selectedUser));
    set_var_cooling_time(storage.getCoolingTime(selectedUser));
    set_var_element_temperature_target(storage.getTempTarget(selectedUser));

    set_var_setting_general_temperature_caliber(storage.getTempCaliber());
    set_var_setting_general_pid(storage.getPID());
    set_var_setting_general_speaker(storage.getSpeaker());
    set_var_setting_general_wifi(storage.getWifiOn());
    set_var_setting_general_automatic_start(storage.getAutoStart());
    // ایمنی: در حالتِ F5-بوت (ota_enable)، رادیوِ وای‌فای همین چند خط بالاتر عمداً روی STAِ خالص
    // تنظیم شده و اسکنِ همگام در ota_task (wifi_perform_sync_scan_and_update_ui) انجام می‌شود.
    // sync_webapp_state وقتی ota_enable=true باشد WiFi.mode(WIFI_OFF) می‌زند — حذف شد.
    if (!get_var_ota_enable()) sync_webapp_state(true);
    set_var_setting_general_final_jack(storage.getFinalJack());
    set_var_setting_general_temperature_limit(storage.getTempLimit());

    if (storage.getIsTreePedal())
    {
        set_var_setting_general_pedal_number3(true);
        set_var_setting_general_pedal_number2(false);
    }
    else
    {
        set_var_setting_general_pedal_number3(false);
        set_var_setting_general_pedal_number2(true);
    }

    if (storage.getIs_J_type())
    {
        set_var_setting_admin_thermocouple_j(true);
        set_var_setting_admin_thermocouple_k(false);
    }
    else
    {
        set_var_setting_admin_thermocouple_j(false);
        set_var_setting_admin_thermocouple_k(true);
    }

    // آیتم: نوارِ صفحهٔ مدیر باید عددِ ذخیره‌شدهٔ T1 را نشان بدهد
    {
        int savedDays = storage.getReverseTimer();
        if (savedDays < 0) savedDays = 0;
        if (savedDays > 125) savedDays = 125;
        set_var_setting_admin_countdown_timer(savedDays);
    }
    set_var_setting_admin_emergency_switch(storage.getEmergencySwich());
    set_var_setting_admin_jack_sensor(storage.getSensorCheck());

    ////////////////////////////////////////////////////////////////////////////////

    set_var_setting_jack_safhe_raft(storage.get_Jack_safhe_raft());
    set_var_setting_jack_safhe_bargasht(storage.get_Jack_safhe_bargasht());

    set_var_setting_jack_shablon_raft(storage.get_Jack_shablon_raft());
    set_var_setting_jack_shablon_bargasht(storage.get_Jack_shablon_bargasht());

    set_var_setting_jack_profil_raft(storage.get_Jack_prifil_raft());
    set_var_setting_jack_profil_bargasht(storage.get_Jack_prifil_bargasht());

    set_var_setting_jack_josh_raft(storage.get_Jack_josh_raft());
    set_var_setting_jack_josh_bargasht(storage.get_Jack_josh_bargasht());


    set_var_jack_dot2_zarbe1_raft(storage.get_Jack_zarbe_1_raft());
    set_var_jack_dot2_zarbe2_raft(storage.get_Jack_zarbe_2_raft());

    set_var_setting_jack_press_left_raft(500);
    set_var_setting_jack_press_right_raft(500);


    
    // Serial.println("-------------------------");
    // Serial.println(storage.getPID());
    // Serial.println("-------------------------");

    g_predictive_thermal.init();
    g_industrial_pid.init(25.0f);
    g_teflon_cycles = storage.get_teflon_cycles();
    if (g_teflon_cycles >= (uint32_t)g_predictive_thermal.cfg.teflon_max_warn) {
        g_teflon_replace_warn = true;
    }

    // printMemoryReport();
}




// ─── fw_info wrappers (called from screens.c) ───────────────────────────────
extern "C" float fw_info_get_weld_quality_score(void) { return g_last_weld_telemetry.score_pct; }
extern "C" const char* fw_info_get_weld_grade(void) { return g_last_weld_telemetry.grade; }
extern "C" uint32_t fw_info_get_teflon_cycles(void) { return g_teflon_cycles; }
extern "C" bool fw_info_is_teflon_warn(void) { return g_teflon_replace_warn; }
extern "C" bool fw_info_is_eco_active(void) { return g_predictive_thermal.is_eco_active(); }
extern "C" float fw_info_get_cpu_temp(void) { return g_fw_info_cpu_temp_c; }
extern "C" float fw_info_get_rtc_temp(void) { return g_fw_info_rtc_temp_c; }
extern "C" void fw_info_get_time_str(char *buf, int len) {
    // ساعتِ لحظهٔ آپلود (ثابت تا آپلود بعدی)
    snprintf(buf, len, "%02d:%02d", BUILD_HOUR, BUILD_MINUTE);
}
extern "C" void fw_info_get_date_str(char *buf, int len) {
    // تاریخِ لحظهٔ آپلود (ثابت تا آپلود بعدی) — فارسی=شمسی، انگلیسی/ترکی=میلادی
    if (storage.getLang() == 1) {
        int sy, sm, sd;
        date_utils::gregorian_to_jalali(BUILD_YEAR, BUILD_MONTH, BUILD_DAY, sy, sm, sd);
        snprintf(buf, len, "%04d-%02d-%02d", sy, sm, sd);
    } else {
        snprintf(buf, len, "%04d-%02d-%02d", BUILD_YEAR, BUILD_MONTH, BUILD_DAY);
    }
}
// قبل از نمایش، اگر روز عوض شده ولی هنوز جوشی انجام نشده، آمارِ روزِ قبل پاک شود (نه فقط هنگامِ اولین جوشِ روزِ جدید)
extern "C" int fw_info_get_daily_frames(void) {
    long today = today_jdn_from_rtc();
    if (today != 0) storage.syncDailyStats(today);
    return storage.getDailyFrameNumbers();
}
extern "C" unsigned long fw_info_get_daily_weld_time_ms(void) {
    long today = today_jdn_from_rtc();
    if (today != 0) storage.syncDailyStats(today);
    return storage.getDailyWeldTimeMs();
}
// قفل مدیر در همین نشست: پس از ورود رمز تا بازگشت به صفحهٔ اصلی، دیگر رمز نخواهد
extern "C" bool fw_admin_is_unlocked(void) { return g_admin_unlocked; }
extern "C" void fw_admin_set_unlocked(bool v) { g_admin_unlocked = v; }

extern "C" bool fw_info_warranty_is_active(void) {
    return warranty_is_active();
}
extern "C" void fw_info_get_warranty_date(char *buf, int len) {
    if (!buf || len <= 0) return;
    const warranty_info_t *wi = warranty_get_info();
    if (wi && wi->active && wi->date_str[0]) {
        snprintf(buf, len, "%s", wi->date_str);
        return;
    }
    // تاریخِ ذخیره‌شده قبلی در storage
    String d = storage.getWarrantyDate();
    int y, m, gd;
    if (d.length() >= 8 && sscanf(d.c_str(), "%d-%d-%d", &y, &m, &gd) == 3 &&
        m >= 1 && m <= 12 && gd >= 1 && gd <= 31) {
        if (y >= 1300 && y <= 1500) {            // شمسیِ ذخیره‌شده → همان (ثابت)
            snprintf(buf, len, "%04d-%02d-%02d", y, m, gd);
            return;
        }
        if (y >= 2020 && y <= 2099) {            // میلادیِ قدیمی → تبدیل به شمسی
            int sy, sm, sd;
            date_utils::gregorian_to_jalali(y, m, gd, sy, sm, sd);
            snprintf(buf, len, "%04d-%02d-%02d", sy, sm, sd);
            return;
        }
    }
    snprintf(buf, len, "---");
}
extern "C" void fw_info_activate_warranty(void) {
    open_warranty_screen();
}
volatile float g_lvgl_cpu_pct = 0.0f;
extern "C" float fw_info_get_cpu_usage(void) { return g_lvgl_cpu_pct; }
// ===== خواندن آنالوگِ مالتی‌پلکسِ ورودی‌ها برای منوی اطلاعات =====
// هر پایهٔ آنالوگ دو ورودی را با ۴ سطح کد می‌کند:
//   ورودی اول ≈ 4095 | ورودی دوم ≈ 1530 | هیچ‌کدام ≈ 2040 | هر دو ≈ 2530
// عددها با سرعت بالا بین سطوح جابجا می‌شوند؛ پس باندِ پایدار با debounce تأیید می‌شود
// تا مقدارِ گذرا خوانده نشود (۲ نمونهٔ پیاپی ≈ زیر ۱۰ms).
// باند: 0=اولی، 1=دومی، 2=هیچ، 3=هردو
static volatile int g_stable_band[3] = {2, 2, 2};
// بعد از جابجایی پایه، debounce باید از نو شروع شود تا باندِ قبلی روی پایهٔ جدید نماند
static volatile int g_sensor_band_reset_gen = 0;

// پنجره‌بندی (window): فقط وقتی مقدار «کاملاً» داخل یک سطح است باند را قبول کن؛
// مقدارِ بینِ سطوح (گذرا حین جابجاییِ سریع) → -1 یعنی «تغییری نده، باندِ پایدارِ قبلی بماند».
static int classify_sensor_band(int adc) {
    if (adc >= 3500)                  return 0; // فقط اولی  (~4095)
    if (adc >= 1300 && adc <= 1780)   return 1; // فقط دومی  (~1530)
    if (adc >= 1850 && adc <= 2240)   return 2; // هیچ‌کدام (~2040)
    if (adc >= 2350 && adc <= 2750)   return 3; // هر دو    (~2530)
    return -1;                                  // بینِ سطوح → نامعتبر
}

// اسلاتِ کش فقط بر اساس ترمینالِ فیزیکی برد (نه SENSOR_* جابجاشده) — وگرنه باندِ پایهٔ دیگر قاطی می‌شود
static int physical_sensor_slot(int gpio) {
    if (gpio == DEFAULT_SENSOR_ELEMENT) return 0;
    if (gpio == DEFAULT_SENSOR_SHABLON) return 1;
    if (gpio == DEFAULT_SENSOR_TABLE)   return 2;
    return -1;
}

static int band_for_gpio(int gpio) {
    if (gpio == PIN_NC) return 2;
    int slot = physical_sensor_slot(gpio);
    if (slot >= 0 && g_stable_band[slot] != 2) return g_stable_band[slot];
    long s = 0;
    for (int i = 0; i < 3; i++) {
        bool adcLocked = (adcMutex && xSemaphoreTake(adcMutex, pdMS_TO_TICKS(50)) == pdTRUE);
        s += analogRead(gpio);
        if (adcLocked) xSemaphoreGive(adcMutex);
        delayMicroseconds(120);
    }
    int b = classify_sensor_band((int)(s / 3));
    return (b < 0) ? 2 : b;
}

static bool gpio_is_analog_sensor(int gpio) {
    return physical_sensor_slot(gpio) >= 0;
}

static void disp_sensor_task(void *pv) {
    // همیشه سه ترمینالِ فیزیکی ADC را بخوان (مستقل از SENSOR_* که با remap عوض می‌شود)
    const int pins[3] = {DEFAULT_SENSOR_ELEMENT, DEFAULT_SENSOR_SHABLON, DEFAULT_SENSOR_TABLE};
    int cand[3] = {2, 2, 2};
    int cnt[3]  = {0, 0, 0};
    int last_reset = -1;
    const int STABLE_N = 2;
    vTaskDelay(pdMS_TO_TICKS(50));
    for (;;) {
        int reset_gen = g_sensor_band_reset_gen;
        if (reset_gen != last_reset) {
            last_reset = reset_gen;
            for (int k = 0; k < 3; k++) {
                cand[k] = 2;
                cnt[k] = 0;
                g_stable_band[k] = 2;
            }
        }
        for (int k = 0; k < 3; k++) {
            long s = 0;
            bool adcLocked = (adcMutex && xSemaphoreTake(adcMutex, pdMS_TO_TICKS(30)) == pdTRUE);
            for (int i = 0; i < 3; i++) {
                s += analogRead(pins[k]);
                delayMicroseconds(60);
            }
            if (adcLocked) xSemaphoreGive(adcMutex);
            int b = classify_sensor_band((int)(s / 3));
            if (b < 0) continue;
            if (b == cand[k]) {
                if (cnt[k] < STABLE_N) cnt[k]++;
                if (cnt[k] >= STABLE_N) g_stable_band[k] = b;
            } else {
                cand[k] = b;
                cnt[k]  = 1;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

// باند طبق سنسور واقعی روی برد (defines.h):
//   I0/I2/I4 (زوج/باز)  ← باند ۱ (~1550)
//   I1/I3/I5 (فرد/بسته) ← باند ۰ (~4095)
//   هردو                ← باند ۳
extern "C" int fw_info_get_digital_in(int idx) {
    if (idx < 0 || idx >= 8 || storage.inRowNc[idx]) return 0;
    int silk = storage.inRowSilk[idx];
    if (silk < 0 || silk > 7) silk = idx;
    // GPIO فقط از روی شمارهٔ I — ردیف UI هیچ قطبیت/پایه‌ای تحمیل نمی‌کند
    int pin = gpio_for_input_silk(silk);
    if (pin == PIN_NC) return 0;
    // آنالوگ بودن از خودِ I است نه از شمارهٔ ردیف (remap ضرب‌دری I0↔I3 و …)
    if (silk <= 5 && gpio_is_analog_sensor(pin)) {
        int band = band_for_gpio(pin);
        return input_silk_signal_active(silk, band) ? 1 : 0;
    }
    return (digitalRead(pin) == LOW) ? 1 : 0;
}
// Q0=TABLE Q1=PRESS_LEFT Q2=PRESS_RIGHT Q3=ELEMENT Q4=SHABLON Q5=SSR_1 Q6=ZARBEH1 Q7=ZARBEH2
static bool fw_out_manual_state[8] = {false,false,false,false,false,false,false,false};
// آیتم: نه const — این آرایه در نقطهٔ static-init با مقدارِ *پیش‌فرضِ* پایه‌ها پر می‌شود (چون
// storage.loadPinMap() هنوز اجرا نشده)؛ باید بعد از آن (در setup) با fw_out_pins_refresh() دوباره پر شود.
static int fw_out_pins[8] = {TABLE, PRESS_LEFT, PRESS_RIGHT, ELEMENT, SHABLON, SSR_1, ZARBEH1, ZARBEH2};
static void fw_out_pins_refresh(void) {
    int p[8] = {TABLE, PRESS_LEFT, PRESS_RIGHT, ELEMENT, SHABLON, SSR_1, ZARBEH1, ZARBEH2};
    for (int i = 0; i < 8; i++) fw_out_pins[i] = p[i];
}
// وضعیتِ خروجی‌ها هنگام ورود به منوی اطلاعات ذخیره می‌شود تا هنگام خروج عیناً بازگردانده شود
static int fw_out_saved_level[8] = {0,0,0,0,0,0,0,0};
extern "C" int fw_info_get_digital_out(int idx) {
    if (idx < 0 || idx >= 8 || fw_out_pins[idx] == PIN_NC) return 0; // N.C. هرگز «روشن» نشان داده نشود
    return (digitalRead(fw_out_pins[idx]) == ON) ? 1 : 0;
}
extern "C" void fw_info_toggle_digital_out(int idx) {
    if (idx < 0 || idx >= 8 || fw_out_pins[idx] == PIN_NC) return;
    if (get_var_step_process()) return;
    fw_out_manual_state[idx] = !fw_out_manual_state[idx];
    digitalWrite(fw_out_pins[idx], fw_out_manual_state[idx] ? ON : OFF);
}
extern "C" bool fw_info_is_out_manually_on(int idx) {
    if (idx < 0 || idx >= 8) return false;
    return fw_out_manual_state[idx];
}
extern "C" bool fw_info_manual_any_active(void) {
    for (int i = 0; i < 8; i++) {
        if (fw_out_manual_state[i]) return true;
    }
    return false;
}
// هنگام ورود به منوی اطلاعات: عکس‌برداری از سطحِ فعلیِ هر خروجی
static bool fw_out_snapshot_valid = false;
extern "C" void fw_info_snapshot_outputs(void) {
    for (int i = 0; i < 8; i++) {
        if (fw_out_pins[i] == PIN_NC) {
            fw_out_saved_level[i] = OFF;
            fw_out_manual_state[i] = false;
            continue;
        }
        fw_out_saved_level[i] = digitalRead(fw_out_pins[i]);
        fw_out_manual_state[i] = false;
    }
    fw_out_snapshot_valid = true;
}
extern "C" void fw_info_reset_manual_outputs(void) {
    // بازگردانیِ خروجی‌ها به همان حالتی که قبل از ورود به منو داشتند (نه خاموشِ اجباری)
    if (!fw_out_snapshot_valid) return;
    for (int i = 0; i < 8; i++) {
        if (fw_out_pins[i] == PIN_NC) {
            fw_out_manual_state[i] = false;
            continue;
        }
        digitalWrite(fw_out_pins[i], fw_out_saved_level[i]);
        fw_out_manual_state[i] = false;
    }
    fw_out_snapshot_valid = false;
}

// ===== آیتم: جابجاییِ پایه از منوی اطلاعات (نگه‌داشتنِ ۲ ثانیه روی نامِ ورودی/خروجی) =====
extern "C" const char *get_fw_q_name(int idx); // screens.c
extern "C" const char *get_fw_i_name(int idx); // screens.c

// آیتم: با هر تغییرِ نگاشتِ پایه یک واحد زیاد می‌شود (برای رفرشِ نمایشی/دیباگ)
int g_pinmap_gen = 0;
struct PinRemapSlot { const char *storeKey; int *pinVar; bool isOutput; };
static PinRemapSlot g_pin_slots[13] = {
    {"pTABLE",   &TABLE,          true},
    {"pPRESSL",  &PRESS_LEFT,     true},
    {"pPRESSR",  &PRESS_RIGHT,    true},
    {"pELEMENT", &ELEMENT,        true},
    {"pSHABLON", &SHABLON,        true},
    {"pSSR1",    &SSR_1,          true},
    {"pZARBEH1", &ZARBEH1,        true},
    {"pZARBEH2", &ZARBEH2,        true},
    {"pSNELEM",  &SENSOR_ELEMENT, false},
    {"pSNSHBL",  &SENSOR_SHABLON, false},
    {"pSNTABLE", &SENSOR_TABLE,   false},
    {"pEMERG",   &EMERGENCY,      false},
    {"pPEDAL",   &PEDAL,          false},
};
// پایه‌های فیزیکیِ موجود روی برد برای هر دسته (مقدارِ پیش‌فرض = محلِ فیزیکیِ سیم روی برد)
// آیتم: ایندکس‌های ۰..۷ = پایهٔ کارخانه‌ایِ همان ۸ اسلاتِ خروجی (io_reset_do_confirm فقط رویِ همین
// ۸تا با i<8 کار می‌کند — دست‌نخورده می‌ماند). ایندکس‌های ۸،۹ = دو ترمینالِ اضافیِ آزادِ برد («Q8»/«Q9»)
// که هیچ اسلاتی به‌طورِ پیش‌فرض صاحبشان نیست؛ فقط در دیالوگِ جابجایی به‌عنوانِ گزینهٔ آزادِ اضافی می‌آیند.
static const int PIN_CANDIDATES_OUT[10] = {DEFAULT_TABLE, DEFAULT_PRESS_LEFT, DEFAULT_PRESS_RIGHT, DEFAULT_ELEMENT, DEFAULT_SHABLON, DEFAULT_SSR_1, DEFAULT_ZARBEH1, DEFAULT_ZARBEH2, DEFAULT_SPARE_Q8, DEFAULT_SPARE_Q9};
static const int PIN_CANDIDATES_IN_COUNT = 8;

static int gpio_for_silk_i(int iNum) {
    return gpio_for_input_silk(iNum);
}

static void sync_input_globals_from_rows(void) {
    // ترمینال‌های فیزیکی ADC همیشه ثابت می‌مانند.
    // اگر از ردیفِ نقش pick می‌شدند، بعد از جابجایی مثل I0↔I3 ممکن بود
    // SENSOR_ELEMENT و SENSOR_SHABLON هر دو به یک GPIO بیفتند و همه‌چیز قاطی شود.
    SENSOR_ELEMENT = DEFAULT_SENSOR_ELEMENT;
    SENSOR_SHABLON = DEFAULT_SENSOR_SHABLON;
    SENSOR_TABLE   = DEFAULT_SENSOR_TABLE;
    EMERGENCY = storage.inRowNc[6] ? PIN_NC : gpio_for_input_silk(storage.inRowSilk[6]);
    PEDAL     = storage.inRowNc[7] ? PIN_NC : gpio_for_input_silk(storage.inRowSilk[7]);
    storage.savePinMapping("pSNELEM", SENSOR_ELEMENT);
    storage.savePinMapping("pSNSHBL", SENSOR_SHABLON);
    storage.savePinMapping("pSNTABLE", SENSOR_TABLE);
    storage.savePinMapping("pEMERG", EMERGENCY);
    storage.savePinMapping("pPEDAL", PEDAL);
}

// اشغال بودنِ GPIO برای ورودی: هر ردیفِ دیگر (حتی باز/بستهٔ سابق) مستقل حساب می‌شود
static bool gpio_used_by_input_row(int gpio, int exceptRow) {
    if (gpio < 0) return false;
    for (int r = 0; r < 8; r++) {
        if (r == exceptRow) continue;
        if (!storage.inRowNc[r] && storage.inRowPin[r] == gpio) return true;
    }
    return false;
}

static const char *pin_slot_display_name(int slot) {
    if (slot >= 0 && slot <= 7) return get_fw_q_name(slot);
    switch (slot) {
        case 8:  return ML("سنسور المنت", "Element Sensor", "Elem Sensoru");
        case 9:  return ML("سنسور شابلون", "Shablon Sensor", "Sablon Sensoru");
        case 10: return ML("سنسور میز", "Table Sensor", "Masa Sensoru");
        case 11: return ML("اضطراری", "Emergency", "Acil");
        case 12: return ML("پدال", "Pedal", "Pedal");
        default: return "";
    }
}
// ورودیِ نمایش‌داده‌شده i (۰..۷ در جدولِ صفحه) به کدام اسلات (۸..۱۲) تعلق دارد
static int fw_i_label_to_slot(int i) {
    switch (i) {
        case 0: case 1: return 8;
        case 2: case 3: return 9;
        case 4: case 5: return 10;
        case 6: return 11;
        case 7: return 12;
        default: return -1;
    }
}
static int io_remap_cur_row = -1;
static int io_remap_cur_slot = -1;
static int pin_slot_owner(int pin) { // کدام اسلات الان صاحبِ این پایه است؟ (-1 = آزاد)
    if (pin < 0) return -1;
    for (int i = 0; i < 13; i++) if (*g_pin_slots[i].pinVar == pin) return i;
    return -1;
}
// پایهٔ فعلیِ ردیف/اسلاتِ بازشده در دیالوگ (N.C. → PIN_NC)
static int io_remap_current_pin(void) {
    if (io_remap_cur_row >= 0) {
        if (storage.inRowNc[io_remap_cur_row]) return PIN_NC;
        return storage.inRowPin[io_remap_cur_row];
    }
    if (io_remap_cur_slot >= 0 && io_remap_cur_slot < 13)
        return *g_pin_slots[io_remap_cur_slot].pinVar;
    return PIN_NC;
}
// همهٔ ترمینال‌ها قابلِ انتخاب‌اند مگر همان پایه‌ای که الان روی این ردیف است
static bool pin_is_remap_candidate(int pin) {
    if (pin < 0) return false;
    return pin != io_remap_current_pin();
}
// هنوز برای مسیرِ قدیمیِ خروجی (اگر جایی مانده) — آزاد بودنِ واقعی
static bool pin_is_remap_available(int pin) {
    if (pin < 0) return false;
    if (!pin_is_remap_candidate(pin)) return false;
    if (io_remap_cur_row >= 0)
        return !gpio_used_by_input_row(pin, io_remap_cur_row);
    int owner = pin_slot_owner(pin);
    return !(owner >= 0 && owner <= 7 && owner != io_remap_cur_slot);
}
// نامِ ردیفِ دیگری که الان این شمارهٔ I (silk) را دارد
static const char *input_silk_occupant_name(int silkI, int exceptRow) {
    if (silkI < 0 || silkI > 7) return "";
    for (int r = 0; r < 8; r++) {
        if (r == exceptRow) continue;
        if (!storage.inRowNc[r] && storage.inRowSilk[r] == silkI)
            return get_fw_i_name(r);
    }
    return "";
}
static void pin_remember_last_owner(int gpio, int slot) {
    if (gpio >= 0 && slot >= 0 && slot < 13) storage.savePinLastOwner(gpio, slot);
}
static void pin_sync_last_owners_from_map(void) {
    for (int i = 0; i < 13; i++) {
        int p = *g_pin_slots[i].pinVar;
        if (p >= 0) pin_remember_last_owner(p, i);
    }
}
static const char *pin_input_terminal_desc_single(int i) {
    switch (i) {
        case 0: return ML("ترمینال المنت (باز/بسته)", "Element terminal (open/close)", "Elem terminali (acik/kapali)");
        case 1: return ML("ترمینال المنت — بسته", "Element terminal — close", "Elem terminali — kapali");
        case 2: return ML("ترمینال شابلون (باز/بسته)", "Template terminal (open/close)", "Sablon terminali (acik/kapali)");
        case 3: return ML("ترمینال شابلون — بسته", "Template terminal — close", "Sablon terminali — kapali");
        case 4: return ML("ترمینال میز (باز/برگشت)", "Table terminal (open/return)", "Masa terminali (acik/geri)");
        case 5: return ML("ترمینال میز — برگشت", "Table terminal — return", "Masa terminali — geri");
        case 6: return ML("استپ اضطراری", "Emergency stop", "Acil stop");
        case 7: return ML("پدال استارت", "Start pedal", "Start pedal");
        default: return "";
    }
}
static const char *pin_last_owner_desc(int lastSlot) {
    return pin_slot_display_name(lastSlot);
}
// برچسبِ یک گزینه در لیست: آزاد / اشغال‌شده (عوض با …)
static const char *pin_choice_row_label(char *buf, size_t len, bool isOut, int candIdx, int pin) {
    if (!isOut) {
        char icode[8];
        snprintf(icode, sizeof(icode), "I%d", candIdx);
        const char *occ = input_silk_occupant_name(candIdx, io_remap_cur_row);
        if (occ && occ[0]) {
            snprintf(buf, len, ML("%s — عوض با %s", "%s - swap with %s", "%s - %s ile degistir"),
                     icode, occ);
            return buf;
        }
        int defSlot = fw_i_label_to_slot(candIdx);
        int lastSlot = storage.getPinLastOwner(pin, defSlot >= 0 ? defSlot : 8);
        if (lastSlot >= 0 && lastSlot <= 12 && lastSlot != defSlot)
            snprintf(buf, len, ML("%s — آزاد (قبلاً %s)", "%s - free (was %s)", "%s - bos (oncesi %s)"),
                     icode, pin_last_owner_desc(lastSlot));
        else
            snprintf(buf, len, ML("%s — آزاد", "%s - free", "%s - bos"), icode);
        return buf;
    }
    int owner = pin_slot_owner(pin);
    if (owner >= 0 && owner <= 7 && owner != io_remap_cur_slot) {
        snprintf(buf, len, ML("Q%d — عوض با %s", "Q%d - swap with %s", "Q%d - %s ile degistir"),
                 candIdx, pin_slot_display_name(owner));
        return buf;
    }
    if (candIdx >= 8) {
        snprintf(buf, len, ML("Q%d — ترمینالِ اضافیِ آزادِ برد", "Q%d - free spare board pin", "Q%d - bos yedek pin"), candIdx);
        return buf;
    }
    int defSlot = candIdx;
    int lastSlot = storage.getPinLastOwner(pin, defSlot);
    if (lastSlot >= 0 && lastSlot <= 7 && lastSlot != defSlot)
        snprintf(buf, len, ML("Q%d — آزاد (قبلاً %s)", "Q%d - free (was %s)", "Q%d - bos (oncesi %s)"),
                 candIdx, pin_slot_display_name(lastSlot));
    else
        snprintf(buf, len, ML("Q%d — %s", "Q%d - %s", "Q%d - %s"),
                 candIdx, pin_slot_display_name(defSlot));
    return buf;
}
static int pin_to_out_q_index(int pin) { // شمارهٔ Qِ چاپ‌شده روی برد برای این پایه (طبقِ راهنمای اتصالات)
    for (int i = 0; i < 10; i++) if (PIN_CANDIDATES_OUT[i] == pin) return i; // ۸،۹ = دو ترمینالِ اضافیِ Q8/Q9
    return -1;
}
static int pin_to_in_index(int pin) { // شمارهٔ I (۰..۷) برای این GPIO
    for (int i = 0; i < PIN_CANDIDATES_IN_COUNT; i++)
        if (gpio_for_silk_i(i) == pin) return i;
    return -1;
}
// آیتم: شمارهٔ اتصالِ چاپ‌شده روی برد برای نگاشتِ *فعلی* — برای نمایش جلوی نامِ هر ردیف در screens.c.
// خروجی: Q0..Q7 مستقیم از پایهٔ فعلیِ اسلات. -1 = N.C.
extern "C" int fw_conn_q_of_out(int idx) {
    if (idx < 0 || idx > 7) return -1;
    int pin = *g_pin_slots[idx].pinVar;
    if (pin == PIN_NC) return -1;
    return pin_to_out_q_index(pin);
}
// ورودی: شمارهٔ I همان چیزی است که کاربر در دیالوگ انتخاب کرده (inRowSilk) — نه اجبارِ زوج/فرد ردیف.
extern "C" int fw_conn_i_of_in(int rowIdx) {
    if (rowIdx < 0 || rowIdx > 7 || storage.inRowNc[rowIdx]) return -1;
    int pin = storage.inRowPin[rowIdx];
    if (pin == PIN_NC) return -1;
    int silk = storage.inRowSilk[rowIdx];
    if (silk >= 0 && silk <= 7) return silk;
    return rowIdx;
}

static lv_obj_t *io_remap_overlay = NULL;
// io_remap_cur_slot / io_remap_cur_row — بالاتر تعریف شده‌اند
static bool io_remap_is_multiplexed_row(int row) { return row >= 0 && row <= 5; }
static lv_obj_t *io_remap_nav_items[16];
static int io_remap_nav_count = 0;

static void io_remap_configure_pin(int pin, bool isOutput, int slot) {
    (void)slot;
    if (pin < 0 || pin == PIN_NC) return;
    if (isOutput) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, OFF);
    } else if (gpio_is_analog_sensor(pin)) {
        // فقط ترمینال‌های ADC — هرگز attenuation روی I6/I7 (دیجیتال)
        analogSetPinAttenuation(pin, ADC_11db);
    } else {
        pinMode(pin, INPUT_PULLUP);
    }
}

static void io_remap_destroy(void) {
    if (io_remap_overlay) { lv_obj_del(io_remap_overlay); io_remap_overlay = NULL; }
    io_remap_cur_slot = -1;
    io_remap_cur_row = -1;
    io_remap_nav_count = 0;
    io_remap_bar = NULL;
    io_remap_tick = 0;
    io_remap_hold_tick = 0;
}

// جابجاییِ ورودی عین خروجی: فقط *همین* ردیف. شمارهٔ I انتخابی ذخیره می‌شود (حتی اگر GPIO مشترک باشد).
// اشغال = ردیف دیگری که *همان شمارهٔ I* را دارد (نه لزوماً همان GPIO).
static void io_remap_apply_input_silk(int silkI) {
    int curRow = io_remap_cur_row;
    if (curRow < 0 || curRow > 7 || silkI < 0 || silkI > 7) return;

    int newPin = gpio_for_silk_i(silkI);
    if (newPin < 0 || newPin == PIN_NC) return;

    int curPin = storage.inRowNc[curRow] ? PIN_NC : storage.inRowPin[curRow];
    int curSilk = storage.inRowSilk[curRow];

    if (!storage.inRowNc[curRow] && curSilk == silkI && curPin == newPin) {
        io_remap_destroy();
        return;
    }

    int theirRow = -1;
    for (int r = 0; r < 8; r++) {
        if (r == curRow) continue;
        if (!storage.inRowNc[r] && storage.inRowSilk[r] == silkI) {
            theirRow = r;
            break;
        }
    }

    if (theirRow >= 0) {
        bool curWasNc = storage.inRowNc[curRow];
        int theirSilk = storage.inRowSilk[theirRow];

        // طرف مقابل: silk جدید + GPIO مشتق از همان silk (نه curPin که ممکن است ناهم‌خوان باشد)
        if (curWasNc) {
            storage.saveInRowNc(theirRow, true);
            storage.saveInRowPin(theirRow, PIN_NC);
            storage.saveInRowSilk(theirRow, theirSilk);
        } else {
            storage.saveInRowNc(theirRow, false);
            storage.saveInRowSilk(theirRow, curSilk); // pin از روی silk ست می‌شود
        }

        storage.saveInRowNc(curRow, false);
        storage.saveInRowSilk(curRow, silkI);

        int theirPinNow = storage.inRowNc[theirRow] ? PIN_NC : storage.inRowPin[theirRow];
        if (!curWasNc && theirPinNow != PIN_NC) {
            pin_remember_last_owner(theirPinNow, fw_i_label_to_slot(theirRow));
            io_remap_configure_pin(theirPinNow, false, fw_i_label_to_slot(theirRow));
        }
        pin_remember_last_owner(newPin, io_remap_cur_slot);
        io_remap_configure_pin(newPin, false, io_remap_cur_slot);
    } else {
        storage.saveInRowNc(curRow, false);
        storage.saveInRowSilk(curRow, silkI);
        if (curPin != PIN_NC) pin_remember_last_owner(curPin, io_remap_cur_slot);
        pin_remember_last_owner(newPin, io_remap_cur_slot);
        io_remap_configure_pin(newPin, false, io_remap_cur_slot);
    }

    sync_input_globals_from_rows();
    // بعد از هر جابجایی، پایه‌های دیجیتال I6/I7 را دوباره INPUT کن
    // (اگر قبلاً اشتباهاً attenuation خورده باشند، خواندن اضطراری/پدال برمی‌گردد)
    {
        int ePin = gpio_for_input_silk(6);
        int pPin = gpio_for_input_silk(7);
        if (ePin != PIN_NC) pinMode(ePin, INPUT_PULLUP);
        if (pPin != PIN_NC) pinMode(pPin, INPUT_PULLUP);
    }
    g_sensor_band_reset_gen++;
    g_pinmap_gen++;
    io_remap_destroy();
}

// قطعِ فقط همین ردیف (PIN_NC) یا مسیر قدیمی اگر جایی pin خام بفرستد
static void io_remap_apply_input(int newPin) {
    int curRow = io_remap_cur_row;
    if (curRow < 0 || curRow > 7) return;

    if (newPin == PIN_NC) {
        storage.saveInRowPin(curRow, PIN_NC);
        storage.saveInRowNc(curRow, true);
        sync_input_globals_from_rows();
        g_sensor_band_reset_gen++;
        g_pinmap_gen++;
        io_remap_destroy();
        return;
    }

    // اگر اشتباهاً GPIO آمد، به نزدیک‌ترین silk نگاشت کن
    int silk = pin_to_in_index(newPin);
    if (silk < 0) silk = curRow;
    io_remap_apply_input_silk(silk);
}

static void io_remap_apply(int newPin) {
    if (io_remap_cur_slot < 0 || io_remap_cur_slot >= 13) return;
    PinRemapSlot &slot = g_pin_slots[io_remap_cur_slot];
    if (!slot.isOutput && io_remap_cur_row >= 0) {
        io_remap_apply_input(newPin);
        return;
    }

    int oldPin = *slot.pinVar;
    if (newPin == oldPin) {
        io_remap_destroy();
        return;
    }
    if (oldPin != PIN_NC && oldPin != newPin) pin_remember_last_owner(oldPin, io_remap_cur_slot);

    if (newPin == PIN_NC) {
        if (slot.isOutput && oldPin != PIN_NC) digitalWrite(oldPin, OFF);
        *slot.pinVar = PIN_NC;
        storage.savePinMapping(slot.storeKey, PIN_NC);
        g_pinmap_gen++;
        fw_out_pins_refresh();
        io_remap_destroy();
        return;
    }

    // Swap با اسلاتِ خروجیِ دیگر (یا گرفتن از اسلاتِ ورودیِ ghost در اسلات‌های ۸..۱۲)
    int ghost = pin_slot_owner(newPin);
    if (ghost >= 0 && ghost != io_remap_cur_slot) {
        pin_remember_last_owner(newPin, ghost);
        if (oldPin != PIN_NC) {
            // جابجایی دوطرفه
            *g_pin_slots[ghost].pinVar = oldPin;
            storage.savePinMapping(g_pin_slots[ghost].storeKey, oldPin);
            if (g_pin_slots[ghost].isOutput) {
                pinMode(oldPin, OUTPUT);
                digitalWrite(oldPin, OFF);
            } else if (ghost >= 8 && ghost <= 10) {
                analogSetPinAttenuation(oldPin, ADC_11db);
            } else {
                pinMode(oldPin, INPUT_PULLUP);
            }
        } else {
            *g_pin_slots[ghost].pinVar = PIN_NC;
            storage.savePinMapping(g_pin_slots[ghost].storeKey, PIN_NC);
        }
    }

    if (slot.isOutput && oldPin != PIN_NC && oldPin != newPin) digitalWrite(oldPin, OFF);
    *slot.pinVar = newPin;
    storage.savePinMapping(slot.storeKey, newPin);
    pin_remember_last_owner(newPin, io_remap_cur_slot);
    io_remap_configure_pin(newPin, slot.isOutput, io_remap_cur_slot);
    g_pinmap_gen++;
    fw_out_pins_refresh();
    io_remap_destroy();
}
static void io_remap_choice_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int pin = (int)(intptr_t)lv_event_get_user_data(e);
    io_remap_apply(pin);
}
static void io_remap_choice_in_silk_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int silkI = (int)(intptr_t)lv_event_get_user_data(e);
    io_remap_apply_input_silk(silkI);
}
static int input_row_display_i(int row) {
    if (row < 0 || row > 7) return -1;
    if (storage.inRowNc[row]) return storage.inRowSilk[row]; // قطع‌شده: آخرین I انتخابی
    int ic = fw_conn_i_of_in(row);
    if (ic >= 0) return ic;
    return storage.inRowSilk[row];
}
static void io_remap_close_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) io_remap_destroy();
}
extern "C" void io_remap_dialog_open(int isOutput, int idx) {
    int slot = isOutput ? idx : fw_i_label_to_slot(idx);
    if (slot < 0 || slot >= 13) return;
    if (io_remap_overlay) io_remap_destroy();
    io_remap_cur_slot = slot;
    io_remap_cur_row = isOutput ? -1 : idx;
    io_remap_nav_count = 0;

    io_remap_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(io_remap_overlay, 460, 318);
    lv_obj_align(io_remap_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(io_remap_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(io_remap_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(io_remap_overlay, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
    lv_obj_set_style_bg_color(io_remap_overlay, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(io_remap_overlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(io_remap_overlay, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(io_remap_overlay, lv_color_hex(0xff444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(io_remap_overlay, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(io_remap_overlay, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(io_remap_overlay);
    lv_obj_set_pos(title, 0, 4); lv_obj_set_size(title, 420, LV_SIZE_CONTENT);
    lv_obj_add_flag(title, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(title, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_vazirmatn_bold_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    char titleBuf[64];
    int curPin = *g_pin_slots[slot].pinVar;
    bool isOutSlot = g_pin_slots[slot].isOutput;
    int curRowPin = curPin;
    if (!isOutSlot && io_remap_cur_row >= 0)
        curRowPin = storage.inRowNc[io_remap_cur_row] ? PIN_NC : storage.inRowPin[io_remap_cur_row];
    if (!isOutSlot && io_remap_cur_row >= 0) {
        const char *rowName = get_fw_i_name(io_remap_cur_row);
        int ic = input_row_display_i(io_remap_cur_row);
        if (curRowPin == PIN_NC) {
            snprintf(titleBuf, sizeof(titleBuf), "%s: I%d (N.C.)", rowName, ic);
        } else if (ic >= 0) {
            snprintf(titleBuf, sizeof(titleBuf), "%s: I%d", rowName, ic);
        } else {
            snprintf(titleBuf, sizeof(titleBuf), "%s", rowName);
        }
    } else if (curPin == PIN_NC) {
        snprintf(titleBuf, sizeof(titleBuf), "%s: N.C.", pin_slot_display_name(slot));
    } else {
        int qi = isOutSlot ? pin_to_out_q_index(curPin) : pin_to_in_index(curPin);
        if (qi >= 0) snprintf(titleBuf, sizeof(titleBuf), "%s: %s%d", pin_slot_display_name(slot), isOutSlot ? "Q" : "I", qi);
        else snprintf(titleBuf, sizeof(titleBuf), "%s", pin_slot_display_name(slot));
    }
    lv_label_set_text(title, titleBuf);

    lv_obj_t *x = lv_btn_create(io_remap_overlay);
    lv_obj_align(x, LV_ALIGN_TOP_RIGHT, 2, -2);
    lv_obj_set_size(x, 34, 30);
    lv_obj_set_style_radius(x, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(x, lv_color_hex(0xffb23a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(x, io_remap_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(x); lv_obj_center(xl);
    lv_obj_set_style_text_font(xl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(xl, LV_SYMBOL_CLOSE);
    if (io_remap_nav_count < 16) io_remap_nav_items[io_remap_nav_count++] = x;

    lv_obj_t *hint = lv_label_create(io_remap_overlay);
    lv_obj_set_pos(hint, 0, 34); lv_obj_set_size(hint, 440, LV_SIZE_CONTENT);
    lv_obj_add_flag(hint, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(hint, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hint, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(hint, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    {
        const char *hintTxt;
        bool rowAlreadyNc = (!isOutSlot && io_remap_cur_row >= 0 && curRowPin == PIN_NC);
        if (isOutSlot) {
            hintTxt = (curPin == PIN_NC)
                ? ML("این خروجی قطع است.\nهر پایه را انتخاب کنید؛ اشغال‌بوده جایش عوض می‌شود.",
                    "This output is disconnected.\nPick any pin; occupied ones will swap.",
                    "Bu cikis kesik.\nHerhangi bir pin secin; dolu olanlar yer degistirir.")
                : ML("هر پایه را انتخاب کنید (اشغال = عوض کردن)، یا «قطع کن».",
                    "Pick any pin (occupied = swap), or Disconnect.",
                    "Herhangi bir pin secin (dolu = degistir), veya Baglantiyi kes.");
        } else if (rowAlreadyNc) {
            hintTxt = ML("این ورودی قطع است.\nهر پایه را انتخاب کنید؛ اشغال‌بوده جایش عوض می‌شود.",
                "This input is disconnected.\nPick any pin; occupied ones will swap.",
                "Bu giris kesik.\nHerhangi bir pin secin; dolu olanlar yer degistirir.");
        } else {
            hintTxt = ML("هر پایه را انتخاب کنید (اشغال = عوض کردن)، یا «قطع کن».",
                "Pick any pin (occupied = swap), or Disconnect.",
                "Herhangi bir pin secin (dolu = degistir), veya Baglantiyi kes.");
        }
        lv_label_set_text(hint, hintTxt);
    }

    lv_obj_update_layout(hint);
    lv_coord_t hint_h = lv_obj_get_height(hint);
    lv_coord_t list_y = (lv_coord_t)(38 + hint_h + 6);

    lv_obj_t *list = lv_obj_create(io_remap_overlay);
    lv_obj_set_pos(list, 0, list_y);
    lv_obj_set_size(list, 440, (lv_coord_t)(268 - list_y));
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(list, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
    lv_obj_set_style_bg_opa(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(list, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(list, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    bool isOut = g_pin_slots[slot].isOutput;
    int candCount = isOut ? 10 : PIN_CANDIDATES_IN_COUNT;

    bool rowAlreadyNc = (!isOut && io_remap_cur_row >= 0 && curRowPin == PIN_NC);
    bool showDisconnect = false;
    if (isOut) {
        showDisconnect = (curPin != PIN_NC);
    } else if (io_remap_cur_row >= 0) {
        showDisconnect = (curRowPin != PIN_NC);
    }

    if (showDisconnect) {
        lv_obj_t *row = lv_btn_create(list);
        lv_obj_set_size(row, 420, 44);
        lv_obj_set_style_radius(row, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xffb23a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(row, io_remap_choice_cb, LV_EVENT_CLICKED, (void *)(intptr_t)PIN_NC);
        lv_obj_add_event_cb(row, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
        lv_obj_t *l = lv_label_create(row); lv_obj_center(l);
        lv_obj_set_style_text_font(l, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(l, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(l, ML("قطع کن", "Disconnect", "Baglantiyi kes"));
        if (io_remap_nav_count < 16) io_remap_nav_items[io_remap_nav_count++] = row;
    }
    bool anyChoice = false;
    if (isOut) {
        for (int i = 0; i < candCount; i++) {
            int pin = PIN_CANDIDATES_OUT[i];
            if (!pin_is_remap_candidate(pin)) continue;
            anyChoice = true;
            bool occupied = (pin_slot_owner(pin) >= 0 && pin_slot_owner(pin) <= 7 && pin_slot_owner(pin) != io_remap_cur_slot);
            lv_obj_t *row = lv_btn_create(list);
            lv_obj_set_size(row, 420, 38);
            lv_obj_set_style_radius(row, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(row, lv_color_hex(occupied ? 0xff5a4a20 : 0xff3a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(row, io_remap_choice_cb, LV_EVENT_CLICKED, (void *)(intptr_t)pin);
            lv_obj_add_event_cb(row, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
            lv_obj_t *l = lv_label_create(row); lv_obj_center(l);
            lv_obj_set_style_text_font(l, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(l, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            char buf[96];
            pin_choice_row_label(buf, sizeof(buf), true, i, pin);
            lv_label_set_text(l, buf);
            if (io_remap_nav_count < 16) io_remap_nav_items[io_remap_nav_count++] = row;
        }
    } else if (io_remap_cur_row >= 0) {
        // عین خروجی: هر ۸ شمارهٔ I جدا — حتی اگر GPIO مشترک باشد (I0 و I1 هر دو در لیست)
        int curSilk = storage.inRowSilk[io_remap_cur_row];
        for (int i = 0; i < PIN_CANDIDATES_IN_COUNT; i++) {
            int pin = gpio_for_silk_i(i);
            if (pin < 0 || pin == PIN_NC) continue;
            if (!rowAlreadyNc && curSilk == i) continue;
            anyChoice = true;
            bool occupied = (input_silk_occupant_name(i, io_remap_cur_row)[0] != '\0');
            lv_obj_t *row = lv_btn_create(list);
            lv_obj_set_size(row, 420, 38);
            lv_obj_set_style_radius(row, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(row, lv_color_hex(occupied ? 0xff5a4a20 : 0xff3a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(row, io_remap_choice_in_silk_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
            lv_obj_add_event_cb(row, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
            lv_obj_t *l = lv_label_create(row); lv_obj_center(l);
            lv_obj_set_style_text_font(l, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(l, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            char buf[96];
            pin_choice_row_label(buf, sizeof(buf), false, i, pin);
            lv_label_set_text(l, buf);
            if (io_remap_nav_count < 16) io_remap_nav_items[io_remap_nav_count++] = row;
        }
    }
    if (!anyChoice && ((isOut && curPin == PIN_NC) || rowAlreadyNc)) {
        lv_obj_t *msg = lv_label_create(list);
        lv_obj_set_size(msg, 420, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(msg, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(msg, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        lv_label_set_text(msg, isOut
            ? ML("هیچ پایهٔ دیگری برای انتخاب نیست", "No other pin to choose", "Secilecek baska pin yok")
            : ML("هیچ ورودی دیگری برای انتخاب نیست", "No other input to choose", "Secilecek baska giris yok"));
    }

    // آیتم: نوارِ ۸ثانیه‌ایِ خودکاربستن — اگر کاربر انتخابی نکند، دیالوگ خودش بسته می‌شود (زیرِ لیست)
    io_remap_bar = lv_bar_create(io_remap_overlay);
    lv_obj_set_pos(io_remap_bar, 10, 270);
    lv_obj_set_size(io_remap_bar, 420, 14);
    lv_obj_add_flag(io_remap_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(io_remap_bar, deadline_hold_event_cb, LV_EVENT_ALL, &io_remap_hold_tick);
    lv_bar_set_range(io_remap_bar, 0, 100);
    lv_bar_set_value(io_remap_bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(io_remap_bar, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(io_remap_bar, lv_color_hex(0xff2196f3), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(io_remap_bar, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    io_remap_tick = lv_tick_get();
}
// آیتم: پیاده‌سازیِ getterهای پیش‌اعلان‌شده برایِ ناوبریِ کلیدِ MODE (تعریفِ کامل بالاتر است)
static bool io_remap_is_open(void) { return io_remap_overlay != NULL; }
static int io_remap_nav_get(lv_obj_t **out, int max) {
    int n = io_remap_nav_count < max ? io_remap_nav_count : max;
    for (int i = 0; i < n; i++) out[i] = io_remap_nav_items[i];
    return n;
}

// ===== آیتم: دیالوگِ «بازگردانیِ پایه‌ها به حالتِ کارخانه» — نگه‌داشتنِ ۲ ثانیه رویِ عنوانِ ورودی/خروجی =====
// دقیقاً هم‌سبکِ دیالوگِ ریستِ چهارچوب (کادرِ تأیید+نوارِ ۵ثانیه‌ایِ انصراف)؛ چون فقط پایه‌های *همان* دسته
// را به مقدارِ پیش‌فرضِ خودشان برمی‌گرداند (که در آن دسته یکتا هستند)، تداخلِ دو نام روی یک پایه ساختاری غیرِممکن است.
static bool io_reset_is_output = false;
static lv_obj_t *io_reset_ok_btn = NULL;
static lv_obj_t *io_reset_cancel_btn = NULL;
static void io_reset_close(void) {
    if (io_reset_dlg) { lv_obj_del(io_reset_dlg); io_reset_dlg = NULL; io_reset_bar = NULL; }
    io_reset_tick = 0;
    io_reset_hold_tick = 0;
    io_reset_ok_btn = NULL; io_reset_cancel_btn = NULL;
}
static bool io_reset_is_open(void) { return io_reset_dlg != NULL; }
static lv_obj_t *io_reset_ok_btn_get(void) { return io_reset_ok_btn; }
static lv_obj_t *io_reset_cancel_btn_get(void) { return io_reset_cancel_btn; }
static void io_reset_do_confirm(void) {
    if (io_reset_is_output) {
        for (int i = 0; i < 8; i++) {
            // ایمنی: پایهٔ فعلی اگر روشن مانده خاموش شود، و پایهٔ کارخانه پیکربندی + خاموش شود
            int oldPin = *g_pin_slots[i].pinVar;
            if (oldPin != PIN_NC) digitalWrite(oldPin, OFF);
            *g_pin_slots[i].pinVar = PIN_CANDIDATES_OUT[i];
            storage.savePinMapping(g_pin_slots[i].storeKey, PIN_CANDIDATES_OUT[i]);
            pinMode(PIN_CANDIDATES_OUT[i], OUTPUT);
            digitalWrite(PIN_CANDIDATES_OUT[i], OFF);
        }
        fw_out_pins_refresh();
    } else {
        static const int defInSlots[5] = {
            DEFAULT_SENSOR_ELEMENT, DEFAULT_SENSOR_SHABLON, DEFAULT_SENSOR_TABLE,
            DEFAULT_EMERGENCY, DEFAULT_PEDAL
        };
        for (int i = 0; i < 5; i++) {
            *g_pin_slots[8 + i].pinVar = defInSlots[i];
            storage.savePinMapping(g_pin_slots[8 + i].storeKey, defInSlots[i]);
            if (i <= 2) analogSetPinAttenuation(defInSlots[i], ADC_11db);
            else pinMode(defInSlots[i], INPUT_PULLUP);
        }
        storage.clearAllInRowNc();
        storage.clearAllInRowPins();
        sync_input_globals_from_rows();
    }
    g_sensor_band_reset_gen++;
    g_pinmap_gen++;
    io_reset_close();
}
static void io_reset_confirm_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) io_reset_do_confirm(); }
static void io_reset_cancel_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) io_reset_close(); }
extern "C" void io_reset_dialog_open(int isOutput) {
    if (io_reset_dlg) io_reset_close();
    if (io_remap_overlay) io_remap_destroy(); // اگر دیالوگِ جابجاییِ پایه باز بود، اول آن بسته شود
    io_reset_is_output = isOutput != 0;

    io_reset_dlg = lv_obj_create(lv_layer_top());
    lv_obj_add_flag(io_reset_dlg, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(io_reset_dlg, 340, 150);
    lv_obj_align(io_reset_dlg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(io_reset_dlg, lv_color_hex(0xff1a2535), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(io_reset_dlg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(io_reset_dlg, lv_color_hex(0xff445577), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(io_reset_dlg, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(io_reset_dlg, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(io_reset_dlg, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(io_reset_dlg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(io_reset_dlg, deadline_hold_event_cb, LV_EVENT_ALL, &io_reset_hold_tick);

    lv_obj_t *qlbl = lv_label_create(io_reset_dlg);
    lv_obj_set_size(qlbl, 320, LV_SIZE_CONTENT);
    lv_obj_align(qlbl, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_add_flag(qlbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(qlbl, deadline_hold_event_cb, LV_EVENT_ALL, &io_reset_hold_tick);
    lv_obj_set_style_text_color(qlbl, lv_color_hex(0xffd0e8ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(qlbl, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(qlbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(qlbl, io_reset_is_output
        ? ML("آیا می‌خواهید همهٔ پایه‌های خروجی را به سیم‌کشیِ کارخانه برگردانید؟", "Reset all OUTPUT pins to factory wiring?", "Tum CIKIS pinlerini fabrika ayarina dondur?")
        : ML("آیا می‌خواهید همهٔ پایه‌های ورودی را به سیم‌کشیِ کارخانه برگردانید؟", "Reset all INPUT pins to factory wiring?", "Tum GIRIS pinlerini fabrika ayarina dondur?"));

    lv_obj_t *ok_btn = lv_btn_create(io_reset_dlg);
    io_reset_ok_btn = ok_btn;
    lv_obj_set_size(ok_btn, 158, 46);
    lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_LEFT, 0, -12);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0xff1a4880), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ok_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ok_btn, io_reset_confirm_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn); lv_obj_center(ok_lbl);
    lv_obj_set_style_text_font(ok_lbl, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ok_lbl, ML("تأیید", "Confirm", "Onayla"));

    io_reset_bar = lv_bar_create(io_reset_dlg);
    lv_obj_set_size(io_reset_bar, 158, 46);
    lv_obj_align(io_reset_bar, LV_ALIGN_BOTTOM_RIGHT, 4, -12);
    lv_bar_set_range(io_reset_bar, 0, 100);
    lv_bar_set_value(io_reset_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(io_reset_bar, lv_color_hex(0xffd9dde1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(io_reset_bar, lv_color_hex(0xffd23b3b), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(io_reset_bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(io_reset_bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *cancel_btn = lv_btn_create(io_reset_bar);
        io_reset_cancel_btn = cancel_btn;
        lv_obj_set_size(cancel_btn, 140, 30);
        lv_obj_center(cancel_btn);
        lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xff8a949c), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(cancel_btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(cancel_btn, io_reset_cancel_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *cancel_lbl = lv_label_create(cancel_btn); lv_obj_center(cancel_lbl);
        lv_obj_set_style_text_font(cancel_lbl, &ui_font_kalameh_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(cancel_lbl, ML("انصراف", "Cancel", "Iptal"));
    }
    io_reset_tick = lv_tick_get();
    io_reset_hold_tick = 0;
}
// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    vTaskDelete(NULL);
}
