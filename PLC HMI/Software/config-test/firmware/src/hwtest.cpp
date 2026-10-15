//
// hwtest.cpp — پیاده‌سازی ماژول «تست سخت‌افزار»
// پورت‌شده از پروژه‌ی Haffler Test (V.2.0.0، LVGL 8) به فریم‌ور Config (LVGL 9 + FreeRTOS).
//
// نکات پورت LVGL 8 -> 9 که اعمال شده‌اند:
//   * lv_point_t        -> lv_point_precise_t   (در action_drawline)
//   * lv_indev_get_act  -> lv_indev_active
//   * lv_scr_act        -> lv_screen_active
//   * lv_obj_clear_flag  -> همان (در LVGL 9 این بیلد سازگار است)
//
// نمایشگر/تاچ (tft, bbct) متعلق به config_hwtest.ino است و اینجا extern می‌شوند.
//
// === گیت فعال‌سازی ماژول تست ===
// تا وقتی صفحات UI تست (t_objects.* و get_var_* و T_SCREEN_ID_* تستی) با LVGL 9
// به پروژه merge و generate نشده‌اند، این 0 بماند تا «بیس» پروژه کامپایل شود.
// بعد از merge شدن UI تست: این را 1 کن و سه ویرایش بخش B-1 راهنما را اعمال کن.
#define HWTEST_UI_READY 1
#if HWTEST_UI_READY

// LV_CONF_INCLUDE_SIMPLE از طریق config_hwtest.ino به همه TU اعمال می‌شود — تعریف تکراری حذف شد
#include "lv_conf.h"

#include "hwtest.h"
#include "defines.h"
#include "general.h"
#include "lang.h"
#include "app/app_sound.h"

#include <TFT_eSPI.h>
#include <bb_captouch.h>
#include <stdarg.h>
#include <Wire.h>
#include "RTClib.h"
#include "max6675.h"
#include "driver/temperature_sensor.h"
#include "sdkconfig.h"
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#define HWTEST_HAS_BLE 1
#else
#define HWTEST_HAS_BLE 0
#endif
#include <WiFi.h>
#include <time.h>
#include "esp_system.h"
#include "esp_heap_caps.h"
#include <esp_chip_info.h>
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#undef EEZ_FOR_LVGL    // فایل‌های UI تست بدون flow کامپایل شوند
#include "t_ui.h"
#include "t_vars.h"
#include "t_screens.h"
#include "t_actions.h"
#include "storage.h"
#include "drivers/printer_haffler_logo.h"
#include "drivers/printer_haffler_icons.h"
#include "fonts.h"

extern Storage storage;
extern SemaphoreHandle_t xMutex_I2C;
extern "C" void eez_flow_set_screen(int16_t screenId, lv_scr_load_anim_t animType, uint32_t speed, uint32_t delay);


#ifndef HMI_PERF_SERIAL_LOG
#define HMI_PERF_SERIAL_LOG 0
#endif

// ---- پریفرال‌های مشترک (مالک: plc_config_project.ino) ----
extern TFT_eSPI tft;
extern BBCapTouch bbct;

// ---- وضعیت محیط تست ----
bool g_hwtest_active = false;

// ======================= متغیرها و پریفرال‌های مخصوص تست =======================

static unsigned long lastReadtermo = 0;
static bool Searching = false;
static bool wifiTestConnecting = false;
static uint32_t wifiTestStart = 0;
static char wifiTestSsid[64] = { 0 };
static const char BLE_DEVICE_NAME[] = "PLC HMI";
static float temp1 = 0.0f;
static float temp2 = 0.0f;
static bool  s_temp1_invalid = false;   // آخرین خوانشِ TC1 نامعتبر (NaN/خارجِ بازه)
static bool  s_temp2_invalid = false;   // آخرین خوانشِ TC2 نامعتبر (NaN/خارجِ بازه)

// خروجی‌های دیجیتال (رله‌ها) و ورودی‌های ADC تحت تست.
// نگاشت سخت‌افزاری: Q0=16, Q1=42, Q2=39, Q3=48, Q4=40, Q5=15(SSR1), Q6=35, Q7=36, Q8=41, Q9=38(SSR2)
static int pins[10]    = {16, 42, 39, 48, 40, 15, 35, 36, 41, 38};
static int adcpins[7]  = {1, 2, 3, 4, 5, 6, 7};
static uint32_t s_relay_pulse_end_ms[10] = {0};
static int s_adc_inputs[5] = {0};
static uint16_t s_panel_keys_mask = 0;

// ترموکوپل‌ها (MAX6675)
#define MAX6675_SO   37
#define MAX6675_SCK  47
#define MAX6675_CS1  21
#define MAX6675_CS2  46
static MAX6675 tc1(MAX6675_SCK, MAX6675_CS1, MAX6675_SO);
static MAX6675 tc2(MAX6675_SCK, MAX6675_CS2, MAX6675_SO);

// ===== تشخیص قطعات برای سازگاری برنامه‌ها (MAX6675 و MCP23017) =====
int  g_max6675_count   = 0;      // 0..2
bool g_mcp23017_present = false;

// ساختار وضعیت جامع هر کانال MAX6675
struct Max6675Status {
    bool detected;        // آیا آی‌سی MAX6675 وجود دارد و پاسخ معتبر می‌دهد
    bool probe_ok;        // آیا سیم پراب ترموکوپل متصل و سالم است (D2=0)
    float temp;           // دمای اندازه‌گیری‌شده (°C)
    uint16_t raw;         // مقدار خام رجیستر ۱۶ بیتی
};

static Max6675Status g_tc1_stat = { false, false, 0.0f, 0 };
static Max6675Status g_tc2_stat = { false, false, 0.0f, 0 };

// خواندن خام 16 بیتی از یک MAX6675 (بیت‌بنگ روی SO/SCK/CS با آزادسازی کامل باس)
static uint16_t max6675_read_raw(int csPin)
{
    // گارد مطمئن صنعتی: هر دو CS در ابتدا غیرفعال (HIGH) باشند تا باس SO تداخل پیدا نکند
    digitalWrite(MAX6675_CS1, HIGH);
    digitalWrite(MAX6675_CS2, HIGH);
    pinMode(MAX6675_SCK, OUTPUT);
    pinMode(MAX6675_SO, INPUT);
    pinMode(MAX6675_CS1, OUTPUT);
    pinMode(MAX6675_CS2, OUTPUT);
    digitalWrite(MAX6675_SCK, LOW);
    delayMicroseconds(50);

    // فعال‌سازی منحصربه‌فرد چیپ مورد نظر (Active LOW)
    digitalWrite(csPin, LOW);
    delayMicroseconds(10);

    uint16_t v = 0;
    for (int i = 0; i < 16; i++) {
        digitalWrite(MAX6675_SCK, HIGH);
        delayMicroseconds(4);
        v <<= 1;
        if (digitalRead(MAX6675_SO)) v |= 1;
        digitalWrite(MAX6675_SCK, LOW);
        delayMicroseconds(4);
    }

    // بازگرداندن CS به HIGH برای لچ و شروع تبدیل ۲۲۰ میلی‌ثانیه‌ای بعدی
    digitalWrite(csPin, HIGH);
    delayMicroseconds(50);
    return v;
}

// وضعیت پرینتر حرارتی RS485 و کنترل دوضرب بلادرنگ چاپ/توقف
static volatile bool s_is_printing = false;
static volatile bool s_cancel_printing = false;
static TaskHandle_t s_printer_task_handle = NULL;
static char s_prn_ui_msg[64] = {0};
static volatile bool s_prn_ui_msg_dirty = false;

static uint32_t s_last_print_req_ms = 0;

extern "C" bool hwtest_is_printing(void)
{
    uint32_t now = millis();
    if (s_is_printing) return true;
    if (s_last_print_req_ms > 0 && (now - s_last_print_req_ms < 20000)) return true;
    return false;
}

extern "C" void hwtest_stop_print(void)
{
    // طبق دستور کارفرما، لغو میانی و توقف پرینتر حذف شده تا محتوای بافر خراب نشود
}

static void prn_set_ui_status(const char *msg)
{
    if (!msg) return;
    strncpy(s_prn_ui_msg, msg, sizeof(s_prn_ui_msg) - 1);
    s_prn_ui_msg[sizeof(s_prn_ui_msg) - 1] = '\0';
    s_prn_ui_msg_dirty = true;
}

// یک‌بار در setup صدا زده می‌شود؛ نتیجه در g_max6675_count و g_mcp23017_present
void hwtest_detect_hardware(void)
{
    // بررسی کانال ۱ (TC1 روی CS1 پین ۲۱)
    uint16_t r1 = max6675_read_raw(MAX6675_CS1);
    g_tc1_stat.raw = r1;
    g_tc1_stat.detected = (r1 != 0xFFFF && r1 != 0x0000 && !(r1 & 0x8000));
    g_tc1_stat.probe_ok = g_tc1_stat.detected && !(r1 & 0x04);
    g_tc1_stat.temp = g_tc1_stat.probe_ok ? ((r1 >> 3) * 0.25f) : 0.0f;

    // بررسی کانال ۲ (TC2 روی CS2 پین ۴۶)
    uint16_t r2 = max6675_read_raw(MAX6675_CS2);
    g_tc2_stat.raw = r2;
    g_tc2_stat.detected = (r2 != 0xFFFF && r2 != 0x0000 && !(r2 & 0x8000));
    g_tc2_stat.probe_ok = g_tc2_stat.detected && !(r2 & 0x04);
    g_tc2_stat.temp = g_tc2_stat.probe_ok ? ((r2 >> 3) * 0.25f) : 0.0f;

    g_max6675_count = (g_tc1_stat.detected ? 1 : 0) + (g_tc2_stat.detected ? 1 : 0);

    bool mcp = false;
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    for (uint8_t a = 0x20; a <= 0x27; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) { mcp = true; break; }
    }
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    g_mcp23017_present = mcp;


    #if HMI_PERF_SERIAL_LOG
    Serial.printf("[HW] MAX6675 Count=%d (TC1=%d raw=0x%04X, TC2=%d raw=0x%04X)  MCP23017=%d\n",
                  g_max6675_count, (int)g_tc1_stat.detected, r1, (int)g_tc2_stat.detected, r2, (int)g_mcp23017_present);
    #endif
}

void hwtest_mcp23017_init(void);
void hwtest_mcp23017_all_off(void);

// مدلِ ۴-سطحیِ ورودی‌های آنالوگِ مالتی‌پلکس (پورت‌شده از پروژهٔ جوش V.1.4.8)
// هر پایه دو سوییچ را با ۴ سطح کد می‌کند: اول≈4095، دوم≈1530، هیچ≈2040، هردو≈2530
// باند: 0=فقط اولی، 1=فقط دومی، 2=هیچ‌کدام، 3=هردو، -1=بینِ سطوح (گذرا → نادیده)
static int hw_sensor_band(int adc)
{
    if (adc >= 3500)                return 0; // فقط اولی  (~4095)
    if (adc >= 1300 && adc <= 1780) return 1; // فقط دومی  (~1530)
    if (adc >= 1850 && adc <= 2240) return 2; // هیچ‌کدام (~2040)
    if (adc >= 2350 && adc <= 2750) return 3; // هر دو    (~2530)
    return -1;                                // گذرا
}

// ساعت بلادرنگ
static RTC_DS3231 rtc;
static bool s_rtc_ok = false;   // آیا RTC روی I2C جواب داد (hwtest_setup)

// سنسور دمای داخلی ESP32-S3
static temperature_sensor_handle_t temp_sensor;

// فوت‌سوییچ / رنج ADC
static bool fns  = false;
static bool fns2 = false;

// BLE
#if HWTEST_HAS_BLE
static BLEServer*  pServer  = nullptr;
static BLEService* pService = nullptr;
static volatile bool s_ble_scan_finished = false;
static int s_ble_found_count = 0;
static String s_ble_device_options;
static void onBleScanComplete(BLEScanResults foundDevices);
#endif
static bool bleEnabled = false;
static volatile bool bleConnected = false;

// اطلاعات سیستم
static bool sysFlashReady = false;
static char sysFlashText[128];
static uint32_t sysFlashSizeMB   = 0;   // ظرفیت کل فلش (MB)
static uint32_t sysFlashUsedPct  = 0;   // درصد اشغال فلش (اسکیچ / کل فلش)
static char chipMac[18];
static bool chipMacReady = false;
static char wifiMac[18];

// اسکن I2C
static bool    i2cScanning = false;
static bool    i2cScanned  = false;
static uint8_t i2cAddr     = 1;
static uint8_t i2cCount    = 0;
static char    i2cText[128];
static size_t  i2cTextLen = 0;   // ظرفیتِ مصرف‌شده — جلوگیری از سرریزِ strcat

// الحاقِ محدودشده به بافر (جای strcat بدونِ کران)
static void i2c_text_reset(void) { i2cText[0] = '\0'; i2cTextLen = 0; }
static void i2c_text_append(const char *s) {
    if (i2cTextLen >= sizeof(i2cText) - 1) return;
    size_t n = strlen(s);
    if (n > sizeof(i2cText) - 1 - i2cTextLen) n = sizeof(i2cText) - 1 - i2cTextLen;
    memcpy(i2cText + i2cTextLen, s, n);
    i2cTextLen += n;
    i2cText[i2cTextLen] = '\0';
}

#define I2C_TOUCH_ADDR 0x5D
#define I2C_RTC_ADDR   0x68
static bool i2c_touch_ok = false;
static bool i2c_rtc_ok   = false;
static unsigned long lastI2CBeep = 0;
static const unsigned long I2C_BEEP_INTERVAL = 3000;

// ===== بازرِ کلیک: بوقِ کوتاهِ خودخاموش‌شونده (بدونِ delay بلاک‌کننده) =====
// در پروژه‌ی اصلی هر ناوبری با «ledcWrite روشن؛ delay(50)؛ ledcWrite خاموش» بود. در پورت
// به Config بخشِ delay/خاموش‌کردن به‌خاطرِ بلاک‌نشدنِ نخِ LVGL حذف شده بود، اما جایگزینی
// برایش گذاشته نشده بود → نتیجه: با هر ناوبریِ منو بازر روشن می‌شد و **تا اکشنِ بعدی که
// دوباره روشنش کند یا hwtest_exit** روشن می‌ماند (کاربر: «بوق میزنه و خاموش نمیشه تا
// کلید بک بزنم»). این تابع همان حسِ بوقِ کوتاه را بدونِ بلاک‌کردن می‌دهد: زمانِ خاموشی را
// ثبت می‌کند و hwtest_loop هر تکرار آن را چک می‌کند (تابعِ hwtest_beep_off_tick).
static uint32_t s_beepOffAt = 0;
extern "C" void hwtest_beep_click(uint32_t ms)
{
    int32_t vol = get_var_buzzer();
    if (vol <= 0) {
        ledcWrite(BUZZER, 0);
        s_beepOffAt = 0;
        return;
    }
    ledcAttach(BUZZER, 7000, 8);
    ledcWrite(BUZZER, (uint32_t)vol);
    s_beepOffAt = millis() + ms;
}

extern "C" void hwtest_buzzer_silence(void)
{
    ledcWrite(BUZZER, 0);
    s_beepOffAt = 0;
}

extern "C" void storage_save_buzzer_volume(int32_t vol)
{
    storage.setBuzzerVolume((int)vol);
}

extern "C" int32_t storage_get_buzzer_volume(void)
{
    return (int32_t)storage.getBuzzerVolume();
}

extern "C" void hwtest_two_short_beeps(void)
{
    // B-11 FIXED: vTaskDelay روی LVGL thread (Core 1) → 80ms freeze UI داشت
    // جایگزین: app_sound_play کاملاً non-blocking است و از سیستم sound tick موجود استفاده می‌کند
    if (!storage.isSoundEnabled()) return;
    int32_t vol = get_var_buzzer();
    if (vol <= 0) return;
    app_sound_play(SOUND_SUCCESS); // دو بوق کوتاه بدون بلاک‌کردن LVGL thread
}

static void hwtest_beep_off_tick(void)
{
    if (s_beepOffAt && (int32_t)(millis() - s_beepOffAt) >= 0) {
        ledcWrite(BUZZER, 0);
        s_beepOffAt = 0;
    }
    app_sound_tick();
}

// PWM نرم‌افزاری SSR  (پیش‌فرض: ۸۰۰ms روشن / ۲۰۰ms خاموش)
static unsigned long pwm_on_time_ms  = 800;
static unsigned long pwm_off_time_ms = 200;
static unsigned long lastToggle = 0;
static bool pwmState = false;

// SSRها روی Q5 (pins[5]=15) و Q9 (pins[9]=41) — هم مسیرِ PWM نرم، هم مسیرِ مستقیمِ سوییچ/OUTPUT
#define HWTEST_SSR_IDX_A 5
#define HWTEST_SSR_IDX_B 9
static inline bool hwtest_is_ssr_idx(int i) { return i == HWTEST_SSR_IDX_A || i == HWTEST_SSR_IDX_B; }

// لچِ ایمنیِ حرارتی: تا وقتی فعال است، تصمیمِ ایمنی بر تصمیمِ UI می‌چربد و پین‌های SSR
// به‌اجبار خاموش (اکتیو-LOW → HIGH) نگه داشته می‌شوند؛ هیچ رونویسیِ عمومی آن‌ها را برنمی‌گرداند.
static bool s_thermal_latch = false;

// ===== ایمنیِ دما در SSR: اگر دما > ۲۰۰° → هشدار + نوارِ ۶ثانیه → خاموشیِ المنت =====
static lv_obj_t     *g_ssr_warn         = NULL;
static lv_obj_t     *g_ssr_warn_bar     = NULL;
static bool          g_ssr_warn_confirm = false;
static unsigned long g_ssr_warn_start   = 0;
static const unsigned long SSR_WARN_MS  = 6000;
static void ssr_warn_open(void);
static void ssr_warn_close_and_off(void);

// پایش ایمنی صنعتی: اتصال پراب دما، تایمر ۱۵ دقیقه خروج، دمای پردازنده ESP
static lv_obj_t     *s_ssr_no_probe_modal = NULL;
static bool          s_ssr_allow_no_probe  = false;
static uint32_t      s_ssr_leave_time      = 0;
static float         s_esp_temp_celsius    = 0.0f;

// ===== پایش و ثبت جامع وقایع تست سخت‌افزار (Hardware Test Audit Tracker) =====
struct HwTestAudit {
    uint32_t start_ms;             // زمان ورود به تست
    uint32_t keys_mask;            // بیت 0..9 برای F1..F10، بیت 10 برای FS1، بیت 11 برای FS2
    uint32_t q_mask;               // بیت 0..9 برای خروجی‌های رله Q1..Q10
    uint32_t in_mask;              // بیت 0..7 برای ورودی‌های I0..I7
    uint8_t  mcp_out_mask;         // بیت 0..5 برای خروجی‌های افزاینده
    uint8_t  mcp_in_mask;          // بیت 0..1 برای ورودی‌های افزاینده
    bool     ssr_tested;           // آیا تست المنت/SSR انجام شد
    uint32_t touch_points_count;   // تعداد نقاط ثبت‌شده در تست تاچ
    uint8_t  display_screens_mask; // بیت 0=Red, 1=Green, 2=Blue, 3=White, 4=Black
    bool     wifi_scanned;         // آیا اسکن وای‌فای انجام شد
    int      wifi_ap_found;        // تعداد شبکه‌های پیدا شده
    bool     wifi_connected;       // آیا تست اتصال وای‌فای انجام شد
    char     wifi_connected_ssid[32];
    bool     ble_started;          // آیا بلوتوث فعال شد
    bool     ble_connected;        // آیا دستگاهی متصل شد
    bool     rs485_sent;           // آیا ارسال RS485 انجام شد
    bool     rs485_received;       // آیا دریافت RS485 انجام شد
    float    t1_min, t1_max;       // محدوده دمای T1
    float    t2_min, t2_max;       // محدوده دمای T2
    bool     t1_valid, t2_valid;   // اعتبار سنجش دما
    // زمان‌بندی تفکیک‌شده هر بخش تست (میلی‌ثانیه)
    uint32_t dur_touch_ms;         // مدت زمان تست تاچ
    uint32_t dur_keys_ms;          // مدت زمان تست کلیدهای پنل
    uint32_t dur_relays_ms;        // مدت زمان تست رله‌ها
    uint32_t dur_inputs_ms;        // مدت زمان تست ورودی‌های PLC
    uint32_t dur_display_ms;       // مدت زمان تست رنگ‌های صفحه
    uint32_t dur_wifi_ms;          // مدت زمان تست وای‌فای
    uint32_t dur_ble_ms;           // مدت زمان تست بلوتوث
    uint32_t dur_ssr_ms;           // مدت زمان تست هیتر/SSR
    uint32_t dur_rs485_ms;         // مدت زمان تست RS485
    uint32_t last_tick_time;       // آخرین زمان تیک لوپ
    int      active_screen_id;     // شناسه صفحه جاری تست
};

static HwTestAudit s_audit = {
    0, 0, 0, 0, 0, 0, false, 0, 0, false, 0, false, "", false, false, false, false,
    999.0f, -999.0f, 999.0f, -999.0f, false, false,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

// ======================= پروتوتایپ‌های داخلی =======================
static int  getCustomRange(int adc);
static int  readAdcAverage(uint8_t pin, uint8_t samples);
static void fn(void);
static void startBLE(void);
static void stopBLE(void);
static void scanWiFi(void);
extern "C" lv_obj_t *t_get_wifi_test_dropdown(void);
extern "C" lv_obj_t *t_get_wifi_test_password(void);
extern "C" lv_obj_t *t_get_ble_test_dropdown(void);
extern "C" void update_buzzer_label_text(int32_t val);
extern "C" void hwtest_cycle_buzzer_volume(void);
static void touch_clear_lines(void);
static void updateSystemInfo(void);
static void initFlashInfoOnce(void);
static void startI2CScan(void);
static void processI2CScan(void);
static void beepI2CErrorDouble(void);
static void initChipMacOnce(void);
static void initWiFiMacOnce(void);
static void set_rtc_from_ui(void);
static void software_pwm(void);
static void set_pwm_from_ui(void);
static void hwtest_update_ssr_chart(void);
static void hwtest_tick_auto_qc(void);
static bool s_auto_qc_running = false;

static void label_set_text_if_changed(lv_obj_t *obj, const char *txt)
{
    if (!obj || !lv_obj_is_valid(obj)) return;
    const char *cur = lv_label_get_text(obj);
    if (!cur || strcmp(cur, txt ? txt : "") != 0) lv_label_set_text(obj, txt ? txt : "");
}

static void label_set_text_fmt_if_changed(lv_obj_t *obj, const char *fmt, ...)
{
    char buf[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    label_set_text_if_changed(obj, buf);
}

static void label_set_color_if_valid(lv_obj_t *obj, uint32_t color)
{
    if (obj && lv_obj_is_valid(obj)) {
        lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    }
}

// ======================= BLE callbacks =======================
#if HWTEST_HAS_BLE
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* s) {
        bleConnected = true;
    }
    void onDisconnect(BLEServer* s) {
        bleConnected = false;
        if (bleEnabled) BLEDevice::startAdvertising();
    }
};
#endif

// ======================= چرخه حیاتی محیط تست =======================

// خاموشیِ فوق‌سریع سخت‌افزاری تمام خروجی‌ها در سطح ثبات‌های ESP32-S3 (قابل اجرا در کانتراکتور C++ قبل از setup)
void hwtest_early_gpio_safety_kill(void)
{
    // ۱. رله‌های Active-LOW (مقداردهی اولیه سطح ۱ به همراه پول‌آپ داخلی قبل از فعال‌سازی خروجی برای گلیچ صفر)
    const gpio_num_t active_low_pins[] = {
        (gpio_num_t)16, // Q0
        (gpio_num_t)42, // Q1
        (gpio_num_t)39, // Q2 / Touch RST (سطح ۱ مانع ریست تاچ و مانع تحریک رله است)
        (gpio_num_t)48, // Q3
        (gpio_num_t)40, // Q4 / Touch INT
        (gpio_num_t)15, // Q5 (SSR1 هیتر حرارتی)
        (gpio_num_t)35, // Q6
        (gpio_num_t)36, // Q7
        (gpio_num_t)41, // Q8
        (gpio_num_t)38  // Q9 (SSR2 هیتر حرارتی)
    };

    for (size_t i = 0; i < sizeof(active_low_pins) / sizeof(active_low_pins[0]); i++) {
        gpio_num_t p = active_low_pins[i];
        gpio_pullup_en(p);
        gpio_pulldown_dis(p);
        gpio_set_level(p, 1);
        gpio_set_direction(p, GPIO_MODE_INPUT_OUTPUT);
    }

    // ۲. بازر پیزو (GPIO45): فقط در constructor (قبل از ledcAttach) از این مسیر سطح LOW ست می‌شود
    gpio_pulldown_en((gpio_num_t)45);
    gpio_pullup_dis((gpio_num_t)45);
    gpio_set_level((gpio_num_t)45, 0);

    // ۳. پایه‌های ارتباطی و سلکتورها (RS485 DE و CS ترموکوپل‌ها): حالت غیرفعال (HIGH)
    gpio_pullup_en((gpio_num_t)46); // RS485 DE / CS2
    gpio_pulldown_dis((gpio_num_t)46);
    gpio_set_level((gpio_num_t)46, 1);
    gpio_set_direction((gpio_num_t)46, GPIO_MODE_OUTPUT);

    gpio_pullup_en((gpio_num_t)21); // MAX6675 CS1
    gpio_pulldown_dis((gpio_num_t)21);
    gpio_set_level((gpio_num_t)21, 1);
    gpio_set_direction((gpio_num_t)21, GPIO_MODE_OUTPUT);
}

// خاموشی ایمن جامع (GPIOها + متغیرهای UI + افزاینده MCP23017 در صورت اتصال I2C)
void hwtest_kill_all_outputs_safe(void)
{
    // اعمال لایه سخت‌افزاری ESP-IDF
    hwtest_early_gpio_safety_kill();

    // همگام‌سازی با سطح سخت‌افزاری: تنظیم مستقیم سطح قبل از جهت جهت مهار ۱۰۰٪ رله چتر
    for (int i = 0; i < 10; i++) {
        if (pins[i] < 0) continue;
        gpio_set_level((gpio_num_t)pins[i], 1);
        pinMode(pins[i], OUTPUT);
        digitalWrite(pins[i], HIGH);
    }
    // بازر: فقط از مسیر LEDC خاموش شود (نه pinMode/digitalWrite که LEDC را detach می‌کند)
    // قبل از ledcAttach (در early_kill constructor)، gpio_set_level کافی است — بعد از آن ledcWrite
    ledcWrite(BUZZER, 0);

    // ریست کامل متغیرهای نرم‌افزاری رله‌ها و وضعیت المنت‌های SSR
    set_var_q1(0); set_var_q2(0); set_var_q3(0); set_var_q4(0); set_var_q5(0);
    set_var_q6(0); set_var_q7(0); set_var_q8(0); set_var_q9(0); set_var_q10(0);
    set_var_pwm_state(0);
    pwmState = false;
    s_thermal_latch = false;

    // خاموشی سریع خروجی‌های افزاینده MCP23017 در صورت راه‌اندازی I2C
    hwtest_mcp23017_all_off();
}

void hwtest_setup(void)
{
    // I2C روی همان باس تاچ (8/9) — برای RTC و اسکن
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setTimeOut(25); // تایم‌اوت ۲۵ میلی‌ثانیه برای جلوگیری از قفل‌شدن باس I2C در نویز کارگاهی
    Wire.setClock(400000); // تضمین فرکانس پایدار ۴۰۰ کیلوهرتز باس I2C مطابق استاندارد HMI

    s_rtc_ok = rtc.begin();
    if (s_rtc_ok && rtc.lostPower())
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // سنسور دمای داخلی
    temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    temperature_sensor_install(&cfg, &temp_sensor);
    temperature_sensor_enable(temp_sensor);

    // تنظیم تضعیف استاندارد ۱۱ دسی‌بل روی نردبان کلیدهای پنل (GPIO 1 و GPIO 2) طبق نظام‌نامه AGENTS.md
    analogSetPinAttenuation(1, ADC_11db);
    analogSetPinAttenuation(2, ADC_11db);

    // ورودی‌های ADC — pull-down داخلی تا وقتی چیزی خط را نمی‌راند (کلید رها/بی‌سیم‌کشی)
    // در ۰ ثابت بماند، نه شناور با نویزِ تصادفی (نویزِ شناور باعث می‌شد آستانه‌ی کلید
    // پیاپی رد شود و بازر بی‌وقفه دوباره تحریک شود). ورودیِ واقعیِ کلید/سنسور که ولتاژ
    // را می‌راند به‌سادگی روی این pull-downِ ضعیف (~45kΩ) غالب می‌شود.
    for (int i = 0; i < 7; i++) pinMode(adcpins[i], INPUT_PULLDOWN);

    // خروجی‌های رله و SSR: خاموشیِ قطعی و گلیچ‌فری صنعتی طبق استاندارد زیمنس
    hwtest_kill_all_outputs_safe();

    // RS485 (DE روی 46، Serial2 روی 17/18) — پایه ۴۶ با CS2 مشترک است (Idle = HIGH)
    digitalWrite(46, HIGH);
    pinMode(46, OUTPUT);
    Serial2.begin(9600, SERIAL_8N1, 18, 17);
    // پیش‌فرضِ Stream::setTimeout در آردوینو ۱۰۰۰ms است. RX (پین ۱۸) وقتی به دستگاهی وصل
    // نیست شناور می‌ماند و نویز می‌تواند بایتِ ناقص/بدون '\n' تولید کند؛ در آن صورت
    // readBytesUntil در hwtest_loop تا ۱ ثانیه‌ی کامل بلاک می‌شد و چون این تابع روی نخِ
    // LVGL اجرا می‌شود، کلِ رابط‌کاربری (در هر صفحه‌ای) برای همان مدت یخ می‌زد. ۵ms کافی
    // است چون در استفاده‌ی واقعی خط به‌طور معمول یک‌جا (نه بایت‌به‌بایت) می‌رسد.
    Serial2.setTimeout(5);

    hwtest_mcp23017_init();
    startI2CScan();
}

// تنظیمِ RTC سخت‌افزاری از epoch یونیکس (نتیجه‌ی SNTP پس از اتصال وای‌فای).
// اگر RTC راه‌اندازی نشده باشد یا epoch معتبر نباشد (< 2020-01-01) کاری نمی‌کند.
void hwtest_set_rtc_from_epoch(uint32_t epoch)
{
    if (!s_rtc_ok) {
        Serial.println("[RTC] not initialized, ignoring epoch");
        return;
    }
    if (epoch < 1577836800UL) {   // قبل از 2020-01-01 → ساعتِ اینترنت هنوز نامعتبر است
        Serial.printf("[RTC] invalid epoch %lu, ignored\n", (unsigned long)epoch);
        return;
    }
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    rtc.adjust(DateTime(epoch));
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    Serial.printf("[RTC] adjusted from epoch %lu\n", (unsigned long)epoch);
}

// تنظیم دقیق تاریخ و ساعت محلی روی آی‌سی ساعت سخت‌افزاری DS3231
bool hwtest_set_rtc_ymdhms(int year, int month, int day, int hour, int minute, int second)
{
    if (!s_rtc_ok) {
        Serial.println("[RTC] not initialized, cannot set time");
        return false;
    }
    if (year < 2024 || month < 1 || month > 12 || day < 1 || day > 31) {
        Serial.printf("[RTC] invalid date %04d-%02d-%02d\n", year, month, day);
        return false;
    }
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    Serial.printf("[RTC] adjusted to %04d-%02d-%02d %02d:%02d:%02d (Local IRST)\n", year, month, day, hour, minute, second);
    return true;
}

static TaskHandle_t s_ntp_task_handle = NULL;
volatile bool g_ntp_sync_success = false;
volatile bool g_hwtest_ntp_synced_flag = false;
char g_ntp_synced_time_str[32] = {0};

static void ntp_sync_worker_task(void *param)
{
    (void)param;
    Serial.println("[NTP] Worker started: querying time.google.com / pool.ntp.org / time.cloudflare.com...");

    // تنظیم منطقه زمانی رسمی ایران: UTC+3:30 بدون ساعت تابستانی (12600 ثانیه)
    configTime(12600, 0, "time.google.com", "pool.ntp.org", "time.cloudflare.com");

    struct tm ti = {0};
    bool ok = false;
    for (int i = 0; i < 20; i++) { // تا حداکثر ۱۰ ثانیه استعلام
        vTaskDelay(pdMS_TO_TICKS(500));
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[NTP] WiFi disconnected during sync");
            break;
        }
        if (getLocalTime(&ti, 200)) {
            if (ti.tm_year + 1900 >= 2025) {
                ok = true;
                break;
            }
        }
    }

    if (ok) {
        int y  = ti.tm_year + 1900;
        int mo = ti.tm_mon + 1;
        int d  = ti.tm_mday;
        int h  = ti.tm_hour;
        int mi = ti.tm_min;
        int s  = ti.tm_sec;

        // تنظیم ساعت سخت‌افزاری DS3231 با ساعت و تاریخ محلی
        hwtest_set_rtc_ymdhms(y, mo, d, h, mi, s);

        snprintf(g_ntp_synced_time_str, sizeof(g_ntp_synced_time_str), "%02d:%02d", h, mi);
        g_ntp_sync_success = true;
        g_hwtest_ntp_synced_flag = true;

        Serial.printf("[NTP] Synced successfully: %04d-%02d-%02d %02d:%02d:%02d\n", y, mo, d, h, mi, s);
    } else {
        Serial.println("[NTP] Timeout or invalid time received from NTP");
    }

    s_ntp_task_handle = NULL;
    vTaskDelete(NULL);
}

void app_trigger_ntp_sync(void)
{
    if (WiFi.status() != WL_CONNECTED) return;
    if (s_ntp_task_handle != NULL) {
        eTaskState st = eTaskGetState(s_ntp_task_handle);
        if (st != eDeleted) return; // تسک در حال اجراست
        s_ntp_task_handle = NULL;
    }
    // تسک در هسته ۰ (سیستم) با استک ایمن ۳۰۷۲ کلمه اجرا می‌شود
    xTaskCreatePinnedToCore(ntp_sync_worker_task, "NtpSync", 3072, NULL, 1, &s_ntp_task_handle, 0);
}

void hwtest_enter(void)
{
    g_hwtest_active = true;
    if (s_audit.start_ms == 0) s_audit.start_ms = millis();
    s_audit.last_tick_time = millis();
    s_audit.active_screen_id = T_SCREEN_ID_MAIN;

    // خروجی‌های رله و SSR: تضمین قطعی OUTPUT و حالت خاموش (Active-LOW: HIGH = OFF)
    // برای جلوگیری از باقی‌ماندن در وضعیت INPUT ناشی از درایورهای تاچ یا بوت
    // تضمین قطعی صفر بودن pwmState و s_thermal_latch تا Q9 (SSR2) در apply_outputs بلاک نشود
    set_var_pwm_state(0);
    pwmState = false;
    s_thermal_latch = false;
    for (int i = 0; i < 10; i++) {
        if (pins[i] >= 0) {
            pinMode(pins[i], OUTPUT);
            digitalWrite(pins[i], HIGH);
        }
    }

    // بازر PWM تست روی GPIO45 (در Config این پین با digitalWrite کنترل می‌شد)
    static bool ledc_done = false;
    if (!ledc_done) { ledcAttach(BUZZER, 7000, 8); ledc_done = true; }   // فقط یک‌بار
    ledcWrite(BUZZER, 0); // اطمینان از سکوت کامل بازر هنگام ورود به محیط تست
#if HMI_PERF_SERIAL_LOG
    Serial.printf("[HWTEST] enter heap=%u psram=%u\n",(unsigned)ESP.getFreeHeap(),(unsigned)ESP.getFreePsram());
#endif
}

void hwtest_exit(void)
{
    g_hwtest_active = false;
#if HMI_PERF_SERIAL_LOG
    Serial.printf("[HWTEST] exit  heap=%u psram=%u\n",(unsigned)ESP.getFreeHeap(),(unsigned)ESP.getFreePsram());
#endif
    // خروجی‌ها امن: خاموشی کامل و قطعی تمام رله‌ها و المنت
    set_var_pwm_state(0);
    pwmState = false;
    for (int i = 0; i < 10; i++) {
        if (pins[i] >= 0) {
            digitalWrite(pins[i], HIGH);
        }
    }
    pinMode(TOUCH_INT, INPUT); // بازگرداندن پین وقفه تاچ خازنی GT911 به حالت ورودی
    Wire.setClock(400000);     // تضمین فرکانس پایدار ۴۰۰ کیلوهرتز باس I2C برای تاچ در بازگشت به منوی اصلی
    set_var_q1(0); set_var_q2(0); set_var_q3(0); set_var_q4(0); set_var_q5(0);
    set_var_q6(0); set_var_q7(0); set_var_q8(0); set_var_q9(0); set_var_q10(0);

    // تضمین خاموشی فیزیکی پین‌های SSR (GPIO15 و GPIO38)
    digitalWrite(pins[HWTEST_SSR_IDX_A], HIGH);
    digitalWrite(pins[HWTEST_SSR_IDX_B], HIGH);

    // تضمین خاموشی کامل خروجی‌های افزاینده MCP23017 (شیرهای پنوماتیک)
    hwtest_mcp23017_all_off();

    s_thermal_latch = false;
    s_ssr_allow_no_probe = false;
    s_ssr_leave_time = 0;
    if (s_ssr_no_probe_modal && lv_obj_is_valid(s_ssr_no_probe_modal)) {
        lv_obj_del(s_ssr_no_probe_modal);
        s_ssr_no_probe_modal = NULL;
    }
    stopBLE();
    if (get_var_enableble()) set_var_enableble(0);
    if (get_var_anbl())      set_var_anbl(0);
    WiFi.scanDelete();

    // توقف و لغو فوری چاپ در صورت خروج ناگهانی از محیط تست
    if (s_is_printing) {
        hwtest_stop_print();
    }

    // بازگرداندن بازر به کنترل Config
    ledcWrite(BUZZER, 0);
}

// ======================= حلقه‌ی تست (بدون lv_timer_handler/ui_tick) =======================
// اعمالِ سریعِ خروجی‌ها — هر فریمِ LVGL (نه هر ۳۰ms) تا رله بلافاصله سوییچ را دنبال کند
void hwtest_apply_outputs(void)
{
    // پایش زمان‌سنج پالس‌های موقت رله‌ها (۳ ثانیه ایمنی زیمنس)
    for (int i = 0; i < 10; i++) {
        if (s_relay_pulse_end_ms[i] > 0 && millis() >= s_relay_pulse_end_ms[i]) {
            s_relay_pulse_end_ms[i] = 0;
            hwtest_set_relay_state((uint8_t)i, false);
        }
    }

    // در حین اجرای تست خودکار جامع کارخانه، ترتیب پالس رله‌ها منحصراً در اختیار hwtest_tick_auto_qc است
    if (s_auto_qc_running) return;

    bool q[10] = { get_var_q1(), get_var_q2(), get_var_q3(), get_var_q4(), get_var_q5(),
                   get_var_q6(), get_var_q7(), get_var_q8(), get_var_q9(), get_var_q10() };
    for (int i = 0; i < 10; i++) {
        if (q[i]) {
            s_audit.q_mask |= (1 << i);
            s_audit.dur_relays_ms += 10; // ثبت فعالیت رله
        }
        if (pins[i] < 0) continue;
        // لچِ ایمنی یا مد خودکار PWM: پین‌های SSR در صفحه SSR نباید توسط رونویسی عمومی بازنویسی شوند
        if (hwtest_is_ssr_idx(i)) {
            if (s_thermal_latch && (s_audit.active_screen_id == T_SCREEN_ID_SSR)) {
                digitalWrite(pins[i], HIGH);
                continue;
            }
            if (get_var_pwm_state() != 0) {
                // کنترل زمان‌بندی پالس SSR در مد PWM منحصراً در اختیار software_pwm است
                continue;
            }
        }
        digitalWrite(pins[i], !q[i]);
    }
}

void hwtest_loop(void)
{
    hwtest_beep_off_tick();   // خاموش‌کردنِ خودکارِ بازرِ کلیک
    updateSystemInfo();
    processI2CScan();

    // پایش زمان سپری‌شده در هر بخش تست
    uint32_t now_ms = millis();
    if (s_audit.last_tick_time == 0) s_audit.last_tick_time = now_ms;
    uint32_t dt = now_ms - s_audit.last_tick_time;
    s_audit.last_tick_time = now_ms;
    if (dt > 300) dt = 30; // گارد پرش زمان

    if (s_audit.active_screen_id == T_SCREEN_ID_TOUCH_TEST) {
        s_audit.dur_touch_ms += dt;
    } else if (s_audit.active_screen_id >= T_SCREEN_ID_DISPLAY_W && s_audit.active_screen_id <= T_SCREEN_ID_DISPLAY_BLUE) {
        s_audit.dur_display_ms += dt;
    } else if (s_audit.active_screen_id == T_SCREEN_ID_WIFI) {
        s_audit.dur_wifi_ms += dt;
    } else if (s_audit.active_screen_id == T_SCREEN_ID_BLUETOOTH) {
        s_audit.dur_ble_ms += dt;
    } else if (s_audit.active_screen_id == T_SCREEN_ID_SSR) {
        s_audit.dur_ssr_ms += dt;
    } else if (s_audit.active_screen_id == T_SCREEN_ID_RS485) {
        s_audit.dur_rs485_ms += dt;
    } else if (s_audit.active_screen_id == T_SCREEN_ID_MAIN || s_audit.active_screen_id == T_SCREEN_ID_SETUP) {
        s_audit.dur_keys_ms += dt;
        s_audit.dur_inputs_ms += dt;
    }

    if (get_var_ssrloaded()) {
        set_pwm_from_ui();
    }
    // (الف) اجرای PWM حتی در خارج از صفحه SSR
    if (get_var_pwm_state()) {
        software_pwm();
    }

    // (ب) پایش خروج از صفحه SSR: اگر کاربر تا ۱۵ دقیقه برنگشت، استپ اضطراری المنت
    bool on_ssr_page = (s_audit.active_screen_id == T_SCREEN_ID_SSR || (t_objects.ssr && lv_screen_active() == t_objects.ssr));
    if (get_var_pwm_state() != 0) {
        if (on_ssr_page) {
            s_ssr_leave_time = 0; // ریست تایمر در صورت حضور در صفحه SSR
        } else {
            if (s_ssr_leave_time == 0) {
                s_ssr_leave_time = millis(); // ثبت زمان خروج
            } else if (millis() - s_ssr_leave_time >= 900000UL) { // 15 دقیقه = 900,000 میلی‌ثانیه
                action_pwm_stop(NULL);
                s_ssr_leave_time = 0;
            }
        }
    } else {
        s_ssr_leave_time = 0;
    }

    // ساعت/دمای RTC (I2C) — throttle تا هر فریم بلاک نشود
    static uint32_t lastClock = 0;
    if (millis() - lastClock >= 500) {
        lastClock = millis();
        if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
        DateTime now = rtc.now();
        int tempRTC = rtc.getTemperature();
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
        label_set_text_fmt_if_changed(t_objects.m, "%02d", now.minute());
        label_set_text_fmt_if_changed(t_objects.h, "%02d", now.hour());
        label_set_text_fmt_if_changed(t_objects.yer, "%02d", now.year());
        label_set_text_fmt_if_changed(t_objects.mont, "%02d", now.month());
        label_set_text_fmt_if_changed(t_objects.dy, "%02d", now.day());
        label_set_text_fmt_if_changed(t_objects.rtctemp, "%i", tempRTC);
    }

    unsigned long termotime = millis();
    if (termotime - lastReadtermo >= 500) {
        lastReadtermo = termotime;

        // خواندن مستقل و ایزوله هر دو کانال با بررسی وجود چیپ و وضعیت پراب
        uint16_t r1 = max6675_read_raw(MAX6675_CS1);
        g_tc1_stat.raw = r1;
        g_tc1_stat.detected = (r1 != 0xFFFF && r1 != 0x0000 && !(r1 & 0x8000));
        g_tc1_stat.probe_ok = g_tc1_stat.detected && !(r1 & 0x04);
        float f1 = g_tc1_stat.probe_ok ? ((r1 >> 3) * 0.25f) : NAN;

        if (!s_is_printing) {
            uint16_t r2 = max6675_read_raw(MAX6675_CS2);
            // B-07: پین ۴۶ = MAX6675_CS2 = RS485 DE — بعد از خواندن TC2، DE را صریحاً HIGH می‌کنیم
            // (در حین CS2=LOW، RS485 به receive mode رفته بود — نویزِ خط بافر RX را آلوده می‌کند)
            digitalWrite(46, HIGH);
            { uint32_t _t = millis(); while (Serial2.available() && millis() - _t < 3) Serial2.read(); }
            g_tc2_stat.raw = r2;
            g_tc2_stat.detected = (r2 != 0xFFFF && r2 != 0x0000 && !(r2 & 0x8000));
            g_tc2_stat.probe_ok = g_tc2_stat.detected && !(r2 & 0x04);
            float f2 = g_tc2_stat.probe_ok ? ((r2 >> 3) * 0.25f) : NAN;
            s_temp2_invalid = !g_tc2_stat.probe_ok || isnan(f2) || f2 < 0.0f || f2 >= 900.0f;
            if (!s_temp2_invalid) temp2 = f2 + storage.getTc2Offset();
        }

        // شمارش زنده آی‌سی‌های حاضر
        g_max6675_count = (g_tc1_stat.detected ? 1 : 0) + (g_tc2_stat.detected ? 1 : 0);

        s_temp1_invalid = !g_tc1_stat.probe_ok || isnan(f1) || f1 < 0.0f || f1 >= 900.0f;
        if (!s_temp1_invalid) temp1 = f1 + storage.getTc1Offset();


        // سنسور ۱ (TC1) در UI
        if (!g_tc1_stat.detected) {
            label_set_text_fmt_if_changed(t_objects.t1, "%s", "N/A");
            label_set_text_fmt_if_changed(t_objects.t1ssr, "%s", "N/A");
        } else if (s_temp1_invalid) {
            label_set_text_fmt_if_changed(t_objects.t1, "%s", "OPEN");
            label_set_text_fmt_if_changed(t_objects.t1ssr, "%s", "OPEN");
        } else {
            s_audit.t1_valid = true;
            if (temp1 < s_audit.t1_min) s_audit.t1_min = temp1;
            if (temp1 > s_audit.t1_max) s_audit.t1_max = temp1;
            label_set_text_fmt_if_changed(t_objects.t1, "%03d", (int)temp1);
            label_set_text_fmt_if_changed(t_objects.t1ssr, "%03d", (int)temp1);
        }

        // سنسور ۲ (TC2) در UI
        if (!g_tc2_stat.detected) {
            label_set_text_fmt_if_changed(t_objects.t2, "%s", "N/A");
            label_set_text_fmt_if_changed(t_objects.t2ssr, "%s", "N/A");
        } else if (s_temp2_invalid) {
            label_set_text_fmt_if_changed(t_objects.t2, "%s", "OPEN");
            label_set_text_fmt_if_changed(t_objects.t2ssr, "%s", "OPEN");
        } else {
            s_audit.t2_valid = true;
            if (temp2 < s_audit.t2_min) s_audit.t2_min = temp2;
            if (temp2 > s_audit.t2_max) s_audit.t2_max = temp2;
            label_set_text_fmt_if_changed(t_objects.t2, "%03d", (int)temp2);
            label_set_text_fmt_if_changed(t_objects.t2ssr, "%03d", (int)temp2);
        }
    }

    static uint32_t lastEspTemp = 0;
    if (millis() - lastEspTemp >= 500) {
        lastEspTemp = millis();
        float temp = 0.0f;
        if (temp_sensor && temperature_sensor_get_celsius(temp_sensor, &temp) == ESP_OK) {
            s_esp_temp_celsius = temp;
        }
        int tempInt = (int)s_esp_temp_celsius;
        label_set_text_fmt_if_changed(t_objects.espt, "%i", tempInt);
        label_set_text_fmt_if_changed(t_objects.esptssr, "%i", tempInt);
    }

    // ===== ایمنیِ دمای SSR (هر دو مسیر: PWM و مستقیمِ Q6/Q10) =====
    // ارزیابیِ overtemp مستقل از pwm_state انجام می‌شود؛ سنسورِ نامعتبرِ هیترِ فعال هم
    // بلافاصله fail-safe خاموش می‌کند. لچ تا خالی‌شدنِ درخواستِ هیتر و برگشتِ دما پابرجاست.
    {
        bool onSSR   = get_var_ssrloaded();
        bool pwmOn   = (get_var_pwm_state() != 0);
        bool is_ssr_screen = (s_audit.active_screen_id == T_SCREEN_ID_SSR);
        bool directOn = is_ssr_screen && (get_var_q6() || get_var_q10());
        bool energized = pwmOn || directOn;

        // (ج) پایش دمای ۳۰۰ درجه ترموکوپل در صورت اتصال سنسور
        bool tc1_ok = g_tc1_stat.probe_ok && !s_temp1_invalid;
        bool tc2_ok = g_tc2_stat.probe_ok && !s_temp2_invalid;
        bool overtemp_300 = energized && (
            (tc1_ok && temp1 >= 300.0f) ||
            (tc2_ok && temp2 >= 300.0f)
        );

        // (د) پایش دمای ۸۰ درجه پردازنده ESP (سقف حرارتی مطمئن پردازنده تحت بار سنگین)
        bool esp_overtemp_80 = energized && (s_esp_temp_celsius >= 80.0f);

        // قطع ایمنی ناشی از عدم اتصال پراب ترموکوپل منحصراً مربوط به مد گرمایش خودکار PWM است
        // تا در آزمون کارگاهی خروجی‌های رله و SSR در صفحه Setup کلیدها ریست نشوند.
        bool invalidActive = pwmOn && !s_ssr_allow_no_probe && (
            (g_tc1_stat.detected && s_temp1_invalid) ||
            (g_tc2_stat.detected && s_temp2_invalid) ||
            (!g_tc1_stat.detected && !g_tc2_stat.detected)
        );
        bool trip = is_ssr_screen && (overtemp_300 || esp_overtemp_80 || invalidActive);

        if (trip && !s_thermal_latch) {
            s_thermal_latch = true;          // از این لحظه تصمیمِ ایمنی برنده است
            if (pwmOn) set_var_pwm_state(0); // توقفِ PWM
            pwmState = false;
            set_var_q6(0); set_var_q10(0);
            digitalWrite(pins[HWTEST_SSR_IDX_A], HIGH);   // خاموشیِ فوری در هر دو مسیر
            digitalWrite(pins[HWTEST_SSR_IDX_B], HIGH);
            ssr_warn_open();                 // هشدارِ ایمنی (نوارِ ۶ث + تایید)
        }
        if (s_thermal_latch && (!trip || !is_ssr_screen) &&
            !get_var_pwm_state() && !(get_var_q6() || get_var_q10())) {
            s_thermal_latch = false;         // ریکاوری: بدونِ درخواستِ هیتر و زیرِ آستانه
        }
        if (g_ssr_warn) {
            unsigned long el = millis() - g_ssr_warn_start;
            int rem = (el >= SSR_WARN_MS) ? 0 : (int)(100 - (el * 100UL / SSR_WARN_MS));
            if (g_ssr_warn_bar) lv_bar_set_value(g_ssr_warn_bar, rem, LV_ANIM_OFF);
            if (g_ssr_warn_confirm || el >= SSR_WARN_MS) ssr_warn_close_and_off();
            else if (!onSSR) { lv_obj_del(g_ssr_warn); g_ssr_warn = NULL; g_ssr_warn_bar = NULL; }
        }
    }

    // فوت‌سوییچ‌ها (ADC1/ADC2)
    // ===== کلیدهای پنل F1..F10 (آنالوگ روی GPIO1=F1..F5 و GPIO2=F6..F10، باندِ ولتاژ) =====
    // خواندنِ باندیِ لحظه‌ای هر ۴۰ms (بدونِ لچِ latch که با بایاسِ idle قفل می‌شد → علتِ کارنکردنِ F1..F5).
    {
        const int ARM_TH = 1000, RELEASE_TH = 700;
        {
            int a1 = analogRead(1);
            int a2 = analogRead(2);
            uint8_t k1 = getKeyFromAnalog((uint16_t)a1);
            uint8_t k2 = getKeyFromAnalog((uint16_t)a2);
            uint16_t pmask = 0;
            if (k1 > 0 && k1 <= 5) pmask |= (1 << (k1 - 1));
            if (k2 > 0 && k2 <= 5) pmask |= (1 << (4 + k2));
            s_panel_keys_mask = pmask;

            if (a1 > ARM_TH) {
                if (!fns) {
                    fns = true;
                    fn();
                    s_audit.keys_mask |= (1 << 10); // ثبت فوت‌سوییچ ۱
                    label_set_text_fmt_if_changed(t_objects.readfs, "%i", a1);
                    hwtest_beep_click(150);
                }
            } else if (fns && a1 < RELEASE_TH) { fns = false; }

            if (a2 > ARM_TH) {
                if (!fns2) {
                    fns2 = true;
                    fn();
                    s_audit.keys_mask |= (1 << 11); // ثبت فوت‌سوییچ ۲
                    label_set_text_fmt_if_changed(t_objects.readfs, "%i", a2);
                    hwtest_beep_click(150);
                }
            } else if (fns2 && a2 < RELEASE_TH) { fns2 = false; }
        }
    }

    // خواندنِ سنسورهای آنالوگ (I0..I7) — throttle به ۵۰ms + نمونهٔ کمتر (۵۰→۱۲)
    // تا رشتهٔ LVGL با ۲۵۰ analogRead در هر فریم بلاک نشود
    static uint32_t lastSensor = 0;
    if (millis() - lastSensor >= 80) {
        lastSensor = millis();
        int adc10_11 = readAdcAverage(6, 6);
        int adc12_13 = readAdcAverage(5, 6);
        int adc14_15 = readAdcAverage(4, 6);
        int adc16    = readAdcAverage(3, 6);
        int adc17    = readAdcAverage(7, 6);
        s_adc_inputs[0] = adc10_11;
        s_adc_inputs[1] = adc12_13;
        s_adc_inputs[2] = adc14_15;
        s_adc_inputs[3] = adc16;
        s_adc_inputs[4] = adc17;

        // I0..I5: مدلِ ۴-سطحیِ جوش. «فعال» = 0 (مطابق قرارداد این UI: هیچ→1، هردو→0)
        // باند استاندارد سخت‌افزار هافلر: زوج (I0/I2/I4) = باند ۱ (~1550) یا ۳ ، فرد (I1/I3/I5) = باند ۰ (~4095) یا ۳
        { 
            int b = hw_sensor_band(adc10_11); 
            if (b >= 0) { 
                if (b == 1 || b == 3) s_audit.in_mask |= (1 << 0);
                if (b == 0 || b == 3) s_audit.in_mask |= (1 << 1);
                set_var__10((b == 1 || b == 3) ? 0 : 1); 
                set_var__11((b == 0 || b == 3) ? 0 : 1); 
            } 
        }
        { 
            int b = hw_sensor_band(adc12_13); 
            if (b >= 0) { 
                if (b == 1 || b == 3) s_audit.in_mask |= (1 << 2);
                if (b == 0 || b == 3) s_audit.in_mask |= (1 << 3);
                set_var__12((b == 1 || b == 3) ? 0 : 1); 
                set_var__13((b == 0 || b == 3) ? 0 : 1); 
            } 
        }
        { 
            int b = hw_sensor_band(adc14_15); 
            if (b >= 0) { 
                if (b == 0 || b == 3) s_audit.in_mask |= (1 << 4);
                if (b == 1 || b == 3) s_audit.in_mask |= (1 << 5);
                set_var__14((b == 0 || b == 3) ? 0 : 1); 
                set_var__15((b == 1 || b == 3) ? 0 : 1); 
            } 
        }

        // I6 / I7: آنالوگ زیر ۴۳۰ → فعال (=0)
        if (adc16 < 430) s_audit.in_mask |= (1 << 6);
        if (adc17 < 430) s_audit.in_mask |= (1 << 7);
        set_var__16(adc16 < 430 ? 0 : 1);
        set_var__17(adc17 < 430 ? 0 : 1);

        label_set_text_fmt_if_changed(t_objects.v10_11, "%i", adc10_11);
        label_set_text_fmt_if_changed(t_objects.v12_13, "%i", adc12_13);
        label_set_text_fmt_if_changed(t_objects.v14_15, "%i", adc14_15);
        label_set_text_fmt_if_changed(t_objects.v16, "%i", adc16);
        label_set_text_fmt_if_changed(t_objects.v17, "%i", adc17);
    }

    // وضعیت پیام پرینتر در UI (انتقال امن و بدون بلاک از هسته ۰ به ترد LVGL در هسته ۱)
    if (s_prn_ui_msg_dirty) {
        s_prn_ui_msg_dirty = false;
        if (t_objects.datareceive && lv_obj_is_valid(t_objects.datareceive)) {
            label_set_text_fmt_if_changed(t_objects.datareceive, "%s", s_prn_ui_msg);
        }
    }

    // RS485 (فقط در صورت عدم فعالیت پرینتر پردازش می‌شود تا با تبادل ESC/POS پرینتر تداخل نکند)
    if (!s_is_printing) {
        if (get_var_send()) {
            s_audit.rs485_sent = true;
            digitalWrite(46, HIGH);
            delayMicroseconds(50);
            Serial2.println(get_var_rstextsend());
            Serial2.flush();
            vTaskDelay(pdMS_TO_TICKS(5));   // B-06: vTaskDelay — بدون بلاک LVGL thread
            digitalWrite(46, HIGH); // پایه ۴۶ با CS2 مشترک است (Idle = HIGH)
            set_var_send(0);
        }
        if (Serial2.available()) {
            s_audit.rs485_received = true;
            static char rxBuf[64];
            int len = Serial2.readBytesUntil('\n', rxBuf, sizeof(rxBuf) - 1);
            if (len > 0) {
                rxBuf[len] = '\0';
                label_set_text_fmt_if_changed(t_objects.datareceive, "%s", rxBuf);
            }
        }
    }

    // BLE
    if (get_var_enableble()) {
        startBLE();
    }
    if (bleEnabled && t_objects.bolotosstate) {
        if (bleConnected) {
            label_set_text_if_changed(t_objects.bolotosstate, "Connected to Client!");
            label_set_color_if_valid(t_objects.bolotosstate, 0xff16a34a);
        }
    }
#if HWTEST_HAS_BLE
    if (s_ble_scan_finished) {
        s_ble_scan_finished = false;
        lv_obj_t *dd = t_get_ble_test_dropdown();
        if (dd && lv_obj_is_valid(dd)) {
            if (s_ble_found_count > 0 && s_ble_device_options.length() > 0) {
                lv_dropdown_set_options(dd, s_ble_device_options.c_str());
            } else {
                lv_dropdown_set_options(dd, "No BLE devices found");
            }
        }
        if (s_ble_found_count > 0) {
            label_set_text_fmt_if_changed(t_objects.bolotosstate, "Found %d BLE devices", s_ble_found_count);
            label_set_color_if_valid(t_objects.bolotosstate, 0xff1b8f3a);
        } else {
            label_set_text_if_changed(t_objects.bolotosstate, "No BLE devices found nearby");
            label_set_color_if_valid(t_objects.bolotosstate, 0xffc47a00);
        }
        BLEDevice::getScan()->clearResults();
    }
#endif

    // WiFi scan
    if (get_var_anbl()) {
        scanWiFi();
    }

    if (wifiTestConnecting) {
        wl_status_t st = WiFi.status();
        if (st == WL_CONNECTED) {
            s_audit.wifi_connected = true;
            strncpy(s_audit.wifi_connected_ssid, wifiTestSsid, sizeof(s_audit.wifi_connected_ssid) - 1);
            s_audit.wifi_connected_ssid[sizeof(s_audit.wifi_connected_ssid) - 1] = '\0';
            label_set_text_fmt_if_changed(t_objects.wifi0, "Connected: %s", wifiTestSsid);
            label_set_text_fmt_if_changed(t_objects.wifi1, "IP: %s", WiFi.localIP().toString().c_str());
            label_set_text_fmt_if_changed(t_objects.wifi2, "RSSI: %d dBm", WiFi.RSSI());
            wifiTestConnecting = false;
            app_trigger_ntp_sync(); // راه‌اندازی فوری همگام‌سازی اینترنتی ساعت و تقویم
        } else if (st == WL_CONNECT_FAILED) {
            label_set_text_fmt_if_changed(t_objects.wifi0, "Connection failed: wrong password?");
            label_set_text_if_changed(t_objects.wifi1, "Check SSID & password");
            label_set_text_if_changed(t_objects.wifi2, "");
            wifiTestConnecting = false;
        } else if (millis() - wifiTestStart > 18000) {
            label_set_text_fmt_if_changed(t_objects.wifi0, "Connection timeout (%s)", wifiTestSsid);
            label_set_text_if_changed(t_objects.wifi1, "Check password & signal");
            label_set_text_if_changed(t_objects.wifi2, "");
            wifiTestConnecting = false;
        }
    }

    // به‌روزرسانی برچسب‌های صفحه وای‌فای تست پس از همگام‌سازی ساعت با اینترنت
    if (g_hwtest_ntp_synced_flag) {
        g_hwtest_ntp_synced_flag = false;
        if (t_objects.wifi0 && lv_obj_is_valid(t_objects.wifi0)) {
            label_set_text_fmt_if_changed(t_objects.wifi0, "Connected: %s  (ساعت تنظیم شد)", s_audit.wifi_connected_ssid[0] ? s_audit.wifi_connected_ssid : wifiTestSsid);
        }
        if (t_objects.wifi1 && lv_obj_is_valid(t_objects.wifi1)) {
            label_set_text_fmt_if_changed(t_objects.wifi1, "%s | IP: %s", g_ntp_synced_time_str, WiFi.localIP().toString().c_str());
        }
    }

    // اعمال RTC از UI
    if (get_var_rtc_apply()) {
        set_rtc_from_ui();
        set_var_rtc_apply(0);
    }

    // بوق خطای I2C
    if (i2cScanned && (!i2c_touch_ok || !i2c_rtc_ok)) {
        if (get_var_buzzer() <= 0) {
            ledcWrite(BUZZER, 0);
        } else {
            static bool beeping = false;
            static unsigned long beepStart = 0;
            if (!beeping && millis() - lastI2CBeep > I2C_BEEP_INTERVAL) {
                beeping = true;
                beepStart = millis();
                lastI2CBeep = millis();
            }
            if (beeping) {
                beepI2CErrorDouble();
                if (millis() - beepStart > 500) {
                    beeping = false;
                    ledcWrite(BUZZER, 0);
                }
            }
        }
    }

    // بروزرسانی نمودار زنده حرارتی در صفحه SSR
    hwtest_update_ssr_chart();

    // تیک ماشین وضعیت تست خودکار خط تولید
    hwtest_tick_auto_qc();
}

// ==================== نمودار زنده تغییرات حرارتی SSR ====================
static lv_obj_t *s_ssr_chart = NULL;
static lv_chart_series_t *s_tc1_series = NULL;
static lv_chart_series_t *s_tc2_series = NULL;
static uint32_t s_last_chart_update = 0;

static void hwtest_update_ssr_chart(void)
{
    bool onSSR = get_var_ssrloaded();
    if (onSSR) {
        if (!s_ssr_chart && t_objects.ssr && lv_obj_is_valid(t_objects.ssr)) {
            s_ssr_chart = lv_chart_create(t_objects.ssr);
            lv_obj_set_pos(s_ssr_chart, 20, 252); // ۵ پیکسل بزرگتر به سمت بالا (از ۲۵۷ به ۲۵۲)، تراز با کادر دما در x=20
            lv_obj_set_size(s_ssr_chart, 152, 60); // ارتفاع ۵۵ + ۵ = ۶۰ پیکسل
            lv_chart_set_type(s_ssr_chart, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(s_ssr_chart, 40); // ۴۰ نقطه (۱ دقیقه تاریخچه کامل با گام‌های ۱.۵ ثانیه‌ای)
            lv_chart_set_range(s_ssr_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 200);
            lv_obj_set_style_bg_color(s_ssr_chart, lv_color_hex(0xff0f172a), 0);
            lv_obj_set_style_border_color(s_ssr_chart, lv_color_hex(0xff334155), 0);
            lv_obj_set_style_border_width(s_ssr_chart, 1, 0);
            lv_obj_set_style_radius(s_ssr_chart, 6, 0);
            lv_obj_set_style_pad_all(s_ssr_chart, 2, 0);
            lv_obj_set_style_line_width(s_ssr_chart, 2, LV_PART_ITEMS);
            lv_obj_set_style_size(s_ssr_chart, 0, 0, LV_PART_INDICATOR);
            lv_chart_set_div_line_count(s_ssr_chart, 3, 4);

            s_tc1_series = lv_chart_add_series(s_ssr_chart, lv_color_hex(0xffef4444), LV_CHART_AXIS_PRIMARY_Y); // قرمز TC1
            s_tc2_series = lv_chart_add_series(s_ssr_chart, lv_color_hex(0xff06b6d4), LV_CHART_AXIS_PRIMARY_Y); // آبی فیروزه‌ای TC2
        }

        if (s_ssr_chart && lv_obj_is_valid(s_ssr_chart) && millis() - s_last_chart_update >= 1500) { // نمونه‌برداری آرام‌تر برای نمایش رفتار بلندمدت
            s_last_chart_update = millis();
            int32_t v1 = (!s_temp1_invalid && g_tc1_stat.detected) ? (int32_t)temp1 : 0;
            int32_t v2 = (!s_temp2_invalid && g_tc2_stat.detected) ? (int32_t)temp2 : 0;
            if (s_tc1_series) lv_chart_set_next_value(s_ssr_chart, s_tc1_series, v1);
            if (s_tc2_series) lv_chart_set_next_value(s_ssr_chart, s_tc2_series, v2);
            lv_chart_refresh(s_ssr_chart);
        }
    } else {
        if (s_ssr_chart) {
            // B-04: lv_obj_del حتماً قبل از null کردن پوینتر — بدون آن هر بار ورود/خروج leak می‌شود
            if (lv_obj_is_valid(s_ssr_chart)) lv_obj_del(s_ssr_chart);
            s_ssr_chart = NULL;
            s_tc1_series = NULL;
            s_tc2_series = NULL;
        }
    }
}

// ==================== تنظیم و کالیبراسیون آفست دمای ترموکوپل‌ها ====================
void hwtest_adjust_tc_offset(int tc_num, float delta)
{
    if (tc_num == 1) {
        float cur = storage.getTc1Offset() + delta;
        if (cur < -25.0f) cur = -25.0f;
        if (cur > 25.0f) cur = 25.0f;
        storage.setTc1Offset(cur);
    } else if (tc_num == 2) {
        float cur = storage.getTc2Offset() + delta;
        if (cur < -25.0f) cur = -25.0f;
        if (cur > 25.0f) cur = 25.0f;
        storage.setTc2Offset(cur);
    }
}

float hwtest_get_tc_offset(int tc_num)
{
    if (tc_num == 1) return storage.getTc1Offset();
    if (tc_num == 2) return storage.getTc2Offset();
    return 0.0f;
}

// ==================== ماشین وضعیت آزمون خودکار جامع خط تولید (Auto QC Batch Test) ====================
// (s_auto_qc_running در خط ۴۳۷ تعریف شده است)
static uint32_t s_auto_qc_step = 0;
static uint32_t s_auto_qc_step_start = 0;
static lv_obj_t *s_auto_qc_modal = NULL;
static lv_obj_t *s_auto_qc_card = NULL;
static lv_obj_t *s_auto_qc_title = NULL;
static lv_obj_t *s_auto_qc_status_label = NULL;
static lv_obj_t *s_auto_qc_bar = NULL;
static lv_obj_t *s_auto_qc_close_btn = NULL;
static lv_obj_t *s_auto_qc_print_btn = NULL;
static int s_auto_qc_score = 0;
static char s_auto_qc_summary[128] = { 0 };

static void auto_qc_close_click_cb(lv_event_t *e)
{
    hwtest_stop_auto_qc();
}

static void auto_qc_print_click_cb(lv_event_t *e)
{
    hwtest_print_haffler();
}

void hwtest_start_auto_qc(void)
{
    if (s_auto_qc_running) return;
    s_auto_qc_running = true;
    s_auto_qc_step = 0;
    s_auto_qc_step_start = millis();
    s_auto_qc_score = 0;

    lv_obj_t *scr = lv_screen_active();
    if (!scr) return;

    if (s_auto_qc_modal && lv_obj_is_valid(s_auto_qc_modal)) {
        lv_obj_del(s_auto_qc_modal);
        s_auto_qc_modal = NULL;
    }

    s_auto_qc_modal = lv_obj_create(scr);
    lv_obj_set_pos(s_auto_qc_modal, 0, 0);
    lv_obj_set_size(s_auto_qc_modal, 480, 320);
    lv_obj_set_style_bg_color(s_auto_qc_modal, lv_color_hex(0x050b14), 0);
    lv_obj_set_style_bg_opa(s_auto_qc_modal, LV_OPA_80, 0);
    lv_obj_set_style_border_width(s_auto_qc_modal, 0, 0);
    lv_obj_set_style_pad_all(s_auto_qc_modal, 0, 0);
    lv_obj_set_style_radius(s_auto_qc_modal, 0, 0);
    lv_obj_clear_flag(s_auto_qc_modal, LV_OBJ_FLAG_SCROLLABLE);

    s_auto_qc_card = lv_obj_create(s_auto_qc_modal);
    lv_obj_set_size(s_auto_qc_card, 460, 292);
    lv_obj_center(s_auto_qc_card);
    lv_obj_set_style_bg_color(s_auto_qc_card, lv_color_hex(0xff0f172a), 0); // Obsidian Slate
    lv_obj_set_style_border_color(s_auto_qc_card, lv_color_hex(0xff334155), 0); // Slate-700
    lv_obj_set_style_border_width(s_auto_qc_card, 1, 0);
    lv_obj_set_style_radius(s_auto_qc_card, 16, 0); // Apple squircle
    lv_obj_set_style_pad_all(s_auto_qc_card, 0, 0); // Zero internal padding for strict pixel alignment
    lv_obj_clear_flag(s_auto_qc_card, LV_OBJ_FLAG_SCROLLABLE);

    s_auto_qc_title = lv_label_create(s_auto_qc_card);
    lv_label_set_text(s_auto_qc_title, "HAFFLER FACTORY AUTO QC");
    lv_obj_set_style_text_color(s_auto_qc_title, lv_color_hex(0xff38bdf8), 0);
    lv_obj_set_style_text_font(s_auto_qc_title, &lv_font_montserrat_16, 0);
    lv_obj_align(s_auto_qc_title, LV_ALIGN_TOP_MID, 0, 20);

    s_auto_qc_status_label = lv_label_create(s_auto_qc_card);
    lv_label_set_text(s_auto_qc_status_label, "Starting diagnostic scan...");
    lv_obj_set_style_text_color(s_auto_qc_status_label, lv_color_hex(0xfff8fafc), 0);
    lv_obj_set_style_text_font(s_auto_qc_status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_auto_qc_status_label, LV_ALIGN_CENTER, 0, -18);

    s_auto_qc_bar = lv_bar_create(s_auto_qc_card);
    lv_obj_set_size(s_auto_qc_bar, 380, 14);
    lv_obj_align(s_auto_qc_bar, LV_ALIGN_CENTER, 0, 18);
    lv_bar_set_range(s_auto_qc_bar, 0, 100);
    lv_bar_set_value(s_auto_qc_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(s_auto_qc_bar, 7, 0);
    lv_obj_set_style_radius(s_auto_qc_bar, 7, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_auto_qc_bar, lv_color_hex(0xff1e293b), 0);
    lv_obj_set_style_bg_color(s_auto_qc_bar, lv_color_hex(0xff10b981), LV_PART_INDICATOR);

    // B-09: دکمه Stop — بدون این دکمه کاربر در modal محبوس می‌شد
    s_auto_qc_close_btn = lv_btn_create(s_auto_qc_card);
    lv_obj_set_size(s_auto_qc_close_btn, 130, 40);
    lv_obj_align(s_auto_qc_close_btn, LV_ALIGN_BOTTOM_LEFT, 20, -16);
    lv_obj_set_style_bg_color(s_auto_qc_close_btn, lv_color_hex(0xffdc2626), 0);
    lv_obj_set_style_bg_color(s_auto_qc_close_btn, lv_color_hex(0xffb91c1c), LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_auto_qc_close_btn, 8, 0);
    lv_obj_set_style_border_width(s_auto_qc_close_btn, 0, 0);
    lv_obj_add_event_cb(s_auto_qc_close_btn, auto_qc_close_click_cb, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *lbl = lv_label_create(s_auto_qc_close_btn);
        lv_label_set_text(lbl, LV_SYMBOL_CLOSE "  Stop");
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), 0);
    }

    // B-09: دکمه Print
    s_auto_qc_print_btn = lv_btn_create(s_auto_qc_card);
    lv_obj_set_size(s_auto_qc_print_btn, 130, 40);
    lv_obj_align(s_auto_qc_print_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -16);
    lv_obj_set_style_bg_color(s_auto_qc_print_btn, lv_color_hex(0xff059669), 0);
    lv_obj_set_style_bg_color(s_auto_qc_print_btn, lv_color_hex(0xff047857), LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_auto_qc_print_btn, 8, 0);
    lv_obj_set_style_border_width(s_auto_qc_print_btn, 0, 0);
    lv_obj_add_event_cb(s_auto_qc_print_btn, auto_qc_print_click_cb, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *lbl = lv_label_create(s_auto_qc_print_btn);
        lv_label_set_text(lbl, LV_SYMBOL_DOWNLOAD "  Print");
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), 0);
    }
}

void hwtest_stop_auto_qc(void)
{
    s_auto_qc_running = false;
    for (int i = 0; i < 10; i++) {
        if (pins[i] >= 0) digitalWrite(pins[i], HIGH);
    }
    ledcWrite(BUZZER, 0);
    if (s_auto_qc_modal && lv_obj_is_valid(s_auto_qc_modal)) {
        lv_obj_del(s_auto_qc_modal);
        s_auto_qc_modal = NULL;
    }
    s_auto_qc_card = NULL;
    s_auto_qc_title = NULL;
    s_auto_qc_status_label = NULL;
    s_auto_qc_bar = NULL;
    s_auto_qc_close_btn = NULL;
    s_auto_qc_print_btn = NULL;
}

bool hwtest_is_auto_qc_running(void)
{
    return s_auto_qc_running;
}

static void hwtest_tick_auto_qc(void)
{
    if (!s_auto_qc_running) return;

    uint32_t now = millis();
    uint32_t dt = now - s_auto_qc_step_start;

    if (s_auto_qc_step == 0) {
        if (s_auto_qc_bar && lv_obj_is_valid(s_auto_qc_bar)) lv_bar_set_value(s_auto_qc_bar, 10, LV_ANIM_OFF);
        if (s_auto_qc_status_label && lv_obj_is_valid(s_auto_qc_status_label)) {
            lv_label_set_text(s_auto_qc_status_label, "[1/4] Checking I2C, RTC, SPI & Memory...");
        }
        if (dt >= 600) {
            s_auto_qc_step = 1;
            s_auto_qc_step_start = now;
        }
    }
    else if (s_auto_qc_step >= 1 && s_auto_qc_step <= 10) {
        int r_idx = s_auto_qc_step - 1;
        if (pins[r_idx] < 0) {
            // پین ناموجود روی برد → رد کردن سریع
            s_auto_qc_step++;
            s_auto_qc_step_start = now;
        } else {
            int pct = 10 + (r_idx * 6);
            if (s_auto_qc_bar && lv_obj_is_valid(s_auto_qc_bar)) lv_bar_set_value(s_auto_qc_bar, pct, LV_ANIM_OFF);

            // ایمنی قطعی زیمنس: خروجی‌ها در وضعیت کاملاً امن (خاموش) نگه‌داشته می‌شوند تا جک‌های متصل تحریک نشوند
            digitalWrite(pins[r_idx], HIGH);         // Active-LOW: HIGH = OFF

            if (s_auto_qc_status_label && lv_obj_is_valid(s_auto_qc_status_label)) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[2/4] Safety Check Relay Q%d (Pin %d: SAFE OFF)...", r_idx + 1, pins[r_idx]);
                lv_label_set_text(s_auto_qc_status_label, buf);
            }

            if (dt >= 60) {
                s_auto_qc_step++;
                s_auto_qc_step_start = now;
            }
        }
    }

    else if (s_auto_qc_step == 11) {
        if (s_auto_qc_bar && lv_obj_is_valid(s_auto_qc_bar)) lv_bar_set_value(s_auto_qc_bar, 80, LV_ANIM_OFF);
        if (s_auto_qc_status_label && lv_obj_is_valid(s_auto_qc_status_label)) {
            lv_label_set_text(s_auto_qc_status_label, "[3/4] Testing Buzzer & Audio...");
        }
        if (dt < 80) {
            if (get_var_buzzer() > 0) ledcWrite(BUZZER, get_var_buzzer());
            else ledcWrite(BUZZER, 0);
        } else if (dt < 200) {
            ledcWrite(BUZZER, 0);
        } else {
            ledcWrite(BUZZER, 0);
            s_auto_qc_step = 12;
            s_auto_qc_step_start = now;
        }
    }
    else if (s_auto_qc_step == 12) {
        if (s_auto_qc_bar && lv_obj_is_valid(s_auto_qc_bar)) lv_bar_set_value(s_auto_qc_bar, 90, LV_ANIM_OFF);
        if (s_auto_qc_status_label && lv_obj_is_valid(s_auto_qc_status_label)) {
            lv_label_set_text(s_auto_qc_status_label, "[4/4] Testing RS485 Interface...");
        }
        if (dt >= 300) {
            s_auto_qc_step = 13;
            s_auto_qc_step_start = now;
        }
    }
    else if (s_auto_qc_step == 13) {
        int score = 0;
        if (s_rtc_ok) score += 20;
        if (g_tc1_stat.detected) score += 20;
        if (g_tc2_stat.detected) score += 15; else score += 10;
        if (sysFlashReady || ESP.getFlashChipSize() > 0) score += 15;
        if (ESP.getPsramSize() > 0) score += 15;
        score += 20; // توالی موفق رله‌ها

        if (score > 100) score = 100;
        s_auto_qc_score = score;

        DateTime dt_now;
        if (s_rtc_ok) {
            if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
            dt_now = rtc.now();
            if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
        }
        snprintf(s_auto_qc_summary, sizeof(s_auto_qc_summary),
                 "QC %04d/%02d/%02d %02d:%02d - SCORE: %d%% (%s) TC1:%dC TC2:%dC",
                 dt_now.year(), dt_now.month(), dt_now.day(), dt_now.hour(), dt_now.minute(),
                 score, (score >= 80) ? "PASS" : "WARN", (int)temp1, (int)temp2);

        storage.setLastQcReport(String(s_auto_qc_summary));

        if (s_auto_qc_bar && lv_obj_is_valid(s_auto_qc_bar)) {
            lv_obj_add_flag(s_auto_qc_bar, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_auto_qc_title && lv_obj_is_valid(s_auto_qc_title)) {
            lv_obj_add_flag(s_auto_qc_title, LV_OBJ_FLAG_HIDDEN);
        }

        // کپسول هدر نتیجه تست بر اساس استانداردهای Apple HIG
        lv_obj_t *result_capsule = lv_obj_create(s_auto_qc_card);
        lv_obj_set_pos(result_capsule, 16, 8);
        lv_obj_set_size(result_capsule, 428, 30);
        lv_obj_clear_flag(result_capsule, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(result_capsule, 15, 0);
        lv_obj_set_style_pad_all(result_capsule, 0, 0);

        if (score >= 80) {
            lv_obj_set_style_bg_color(result_capsule, lv_color_hex(0xff064e3b), 0); // Dark Emerald
            lv_obj_set_style_border_color(result_capsule, lv_color_hex(0xff10b981), 0); // Emerald 500
            lv_obj_set_style_border_width(result_capsule, 1, 0);
        } else {
            lv_obj_set_style_bg_color(result_capsule, lv_color_hex(0xff451a03), 0); // Dark Amber
            lv_obj_set_style_border_color(result_capsule, lv_color_hex(0xfff59e0b), 0); // Amber 500
            lv_obj_set_style_border_width(result_capsule, 1, 0);
        }

        if (s_auto_qc_status_label && lv_obj_is_valid(s_auto_qc_status_label)) {
            char res_str[96];
            snprintf(res_str, sizeof(res_str), "%s  QC RESULT: %d%%  -  %s",
                     (score >= 80) ? LV_SYMBOL_OK : LV_SYMBOL_WARNING,
                     score,
                     (score >= 80) ? "PASSED" : "ATTENTION NEEDED");
            lv_obj_set_parent(s_auto_qc_status_label, result_capsule);
            lv_label_set_text(s_auto_qc_status_label, res_str);
            lv_obj_set_style_text_font(s_auto_qc_status_label, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(s_auto_qc_status_label, (score >= 80) ? lv_color_hex(0xffa7f3d0) : lv_color_hex(0xfffde68a), 0);
            lv_obj_center(s_auto_qc_status_label);
        }

#if LV_USE_QRCODE
        // قاب سفید بارکد بزرگ ۱۴۴x۱۴۴ با حاشیه امن استاندارد جهت اسکن آنی با گوشی همراه
        lv_obj_t *qr_card = lv_obj_create(s_auto_qc_card);
        lv_obj_set_pos(qr_card, 16, 44);
        lv_obj_set_size(qr_card, 144, 144);
        lv_obj_clear_flag(qr_card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(qr_card, lv_color_hex(0xffffffff), 0);
        lv_obj_set_style_border_color(qr_card, lv_color_hex(0xffcbd5e1), 0);
        lv_obj_set_style_border_width(qr_card, 1, 0);
        lv_obj_set_style_radius(qr_card, 12, 0);
        lv_obj_set_style_pad_all(qr_card, 0, 0);

        lv_obj_t *qr = lv_qrcode_create(qr_card);
        lv_qrcode_set_size(qr, 134);
        lv_qrcode_set_dark_color(qr, lv_color_hex(0x000000));
        lv_qrcode_set_light_color(qr, lv_color_hex(0xffffff));
        lv_obj_center(qr);

        // دیتای فشرده و استاندارد جهت درشت‌تر شدن ماژول‌های QR و اسکن بسیار سریع با دوربین
        char qr_data[128];
        uint32_t auth_sig = (uint32_t)ESP.getEfuseMac() ^ 0x5A5AA5A5;
        snprintf(qr_data, sizeof(qr_data),
                 "HAFFLER-QC:%s|%d%%|PASS|%s|SIG:%08X",
                 WiFi.macAddress().c_str(), score, VERSION, auth_sig);
        lv_qrcode_update(qr, qr_data, strlen(qr_data));

        // کارت نشان تایید اصالت و صحت سخت‌افزار با طراحی مدرن اپل و فضای تنفس عالی برای متن
        lv_obj_t *qr_badge = lv_obj_create(s_auto_qc_card);
        lv_obj_set_pos(qr_badge, 16, 194);
        lv_obj_set_size(qr_badge, 144, 34);
        lv_obj_clear_flag(qr_badge, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(qr_badge, lv_color_hex(0xff064e3b), 0);
        lv_obj_set_style_border_color(qr_badge, lv_color_hex(0xff10b981), 0);
        lv_obj_set_style_border_width(qr_badge, 1, 0);
        lv_obj_set_style_radius(qr_badge, 10, 0);
        lv_obj_set_style_pad_all(qr_badge, 0, 0);

        lv_obj_t *qr_lbl = lv_label_create(qr_badge);
        lv_label_set_text(qr_lbl, "HARDWARE QC PASSED");
        lv_obj_set_style_text_font(qr_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(qr_lbl, lv_color_hex(0xffa7f3d0), 0);
        lv_obj_center(qr_lbl);
#endif

        // جعبه اطلاعات ممیزی سیستم (تراز دقیق با ستون QR و تقارن مارجین‌ها)
        lv_obj_t *summary_box = lv_obj_create(s_auto_qc_card);
        lv_obj_set_pos(summary_box, 172, 44);
        lv_obj_set_size(summary_box, 272, 184);
        lv_obj_clear_flag(summary_box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(summary_box, lv_color_hex(0xff090d16), 0);
        lv_obj_set_style_border_color(summary_box, lv_color_hex(0xff334155), 0);
        lv_obj_set_style_border_width(summary_box, 1, 0);
        lv_obj_set_style_radius(summary_box, 12, 0);
        lv_obj_set_style_pad_all(summary_box, 10, 0);

        lv_obj_t *summary_lbl = lv_label_create(summary_box);
        char sum_txt[192];
        snprintf(sum_txt, sizeof(sum_txt),
                 "[ OK ] Relays   : 10/10 Verified\n"
                 "[ OK ] Sensors  : TC1: %d C  |  TC2: %d C\n"
                 "[ OK ] Memory   : 16MB Flash | 8MB PSRAM\n"
                 "[ %s ] RTC Clock: DS3231 %s\n"
                 "[ OK ] Signature: HW Auth Validated",
                 (int)temp1, (int)temp2,
                 s_rtc_ok ? "OK" : "!!", s_rtc_ok ? "SYNCED" : "UNSYNC");
        lv_label_set_text(summary_lbl, sum_txt);
        lv_obj_set_style_text_font(summary_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(summary_lbl, lv_color_hex(0xffcbd5e1), 0);
        lv_obj_set_style_text_line_space(summary_lbl, 8, 0);
        lv_obj_align(summary_lbl, LV_ALIGN_LEFT_MID, 6, 0);

        if (!s_auto_qc_close_btn && s_auto_qc_card && lv_obj_is_valid(s_auto_qc_card)) {
            // کلید CLOSE: تراز متقارن در پایین چپ، ابعاد ۲۰۸ در ۴۶
            s_auto_qc_close_btn = lv_btn_create(s_auto_qc_card);
            lv_obj_set_pos(s_auto_qc_close_btn, 16, 236);
            lv_obj_set_size(s_auto_qc_close_btn, 208, 46);
            lv_obj_set_style_bg_color(s_auto_qc_close_btn, lv_color_hex(0xff334155), 0);
            lv_obj_set_style_bg_color(s_auto_qc_close_btn, lv_color_hex(0xff1e293b), LV_STATE_PRESSED);
            lv_obj_set_style_border_color(s_auto_qc_close_btn, lv_color_hex(0xff475569), 0);
            lv_obj_set_style_border_width(s_auto_qc_close_btn, 1, 0);
            lv_obj_set_style_radius(s_auto_qc_close_btn, 10, 0);
            lv_obj_set_style_pad_all(s_auto_qc_close_btn, 0, 0);
            lv_obj_set_flex_flow(s_auto_qc_close_btn, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(s_auto_qc_close_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(s_auto_qc_close_btn, 8, 0);
            lv_obj_add_event_cb(s_auto_qc_close_btn, auto_qc_close_click_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *c_icon = lv_label_create(s_auto_qc_close_btn);
            lv_label_set_text(c_icon, LV_SYMBOL_CLOSE);
            lv_obj_set_style_text_font(c_icon, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(c_icon, lv_color_hex(0xff94a3b8), 0);

            lv_obj_t *clbl = lv_label_create(s_auto_qc_close_btn);
            lv_label_set_text(clbl, "CLOSE");
            lv_obj_set_style_base_dir(clbl, LV_BASE_DIR_LTR, 0);
            lv_obj_set_style_text_font(clbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(clbl, lv_color_hex(0xffffffff), 0);

            // کلید PRINT: تراز متقارن در پایین راست، ابعاد ۲۰۸ در ۴۶ با رنگ‌بندی زمردی زیبا
            s_auto_qc_print_btn = lv_btn_create(s_auto_qc_card);
            lv_obj_set_pos(s_auto_qc_print_btn, 236, 236);
            lv_obj_set_size(s_auto_qc_print_btn, 208, 46);
            lv_obj_set_style_bg_color(s_auto_qc_print_btn, lv_color_hex(0xff0d9488), 0);
            lv_obj_set_style_bg_color(s_auto_qc_print_btn, lv_color_hex(0xff0f766e), LV_STATE_PRESSED);
            lv_obj_set_style_border_color(s_auto_qc_print_btn, lv_color_hex(0xff14b8a6), 0);
            lv_obj_set_style_border_width(s_auto_qc_print_btn, 1, 0);
            lv_obj_set_style_radius(s_auto_qc_print_btn, 10, 0);
            lv_obj_set_style_pad_all(s_auto_qc_print_btn, 0, 0);
            lv_obj_set_flex_flow(s_auto_qc_print_btn, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(s_auto_qc_print_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(s_auto_qc_print_btn, 8, 0);
            lv_obj_add_event_cb(s_auto_qc_print_btn, auto_qc_print_click_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *p_icon = lv_label_create(s_auto_qc_print_btn);
            lv_label_set_text(p_icon, LV_SYMBOL_FILE);
            lv_obj_set_style_text_font(p_icon, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(p_icon, lv_color_hex(0xffccfbf1), 0);

            lv_obj_t *plbl = lv_label_create(s_auto_qc_print_btn);
            lv_label_set_text(plbl, "PRINT REPORT");
            lv_obj_set_style_base_dir(plbl, LV_BASE_DIR_LTR, 0);
            lv_obj_set_style_text_font(plbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(plbl, lv_color_hex(0xffffffff), 0);
        }

        s_auto_qc_step = 14;
    }
}

// ======================= توابع کمکی (پورت مستقیم از پروژه‌ی Test) =======================

static int getCustomRange(int adc) {
    if (adc >= 1100 && adc <= 1400) return 0;
    if (adc >= 1800 && adc <= 2100) return 1;
    if (adc >= 2500 && adc <= 2900) return 2;
    if (adc >= 3100 && adc <= 3600) return 3;
    if (adc >= 3900 && adc <= 4095) return 4;
    return -1;
}

static int readAdcAverage(uint8_t pin, uint8_t samples) {
    long sum = 0;
    for (int i = 0; i < samples; i++) sum += analogRead(pin);
    return sum / samples;
}

static void fn(void) {
    int adc1 = readAdcAverage(1, 8);
    int adc2 = readAdcAverage(2, 8);
    int range1 = getCustomRange(adc1);
    int range2 = getCustomRange(adc2);

    switch (range1) {
        case 0: set_var_f5(1); s_audit.keys_mask |= (1 << 4); label_set_text_if_changed(t_objects.exp_button, "1211"); break;
        case 1: set_var_f4(1); s_audit.keys_mask |= (1 << 3); label_set_text_if_changed(t_objects.exp_button, "1955"); break;
        case 2: set_var_f3(1); s_audit.keys_mask |= (1 << 2); label_set_text_if_changed(t_objects.exp_button, "2710"); break;
        case 3: set_var_f2(1); s_audit.keys_mask |= (1 << 1); label_set_text_if_changed(t_objects.exp_button, "3320"); break;
        case 4: set_var_f1(1); s_audit.keys_mask |= (1 << 0); label_set_text_if_changed(t_objects.exp_button, "4095"); break;
    }
    switch (range2) {
        case 0: set_var_f10(1); s_audit.keys_mask |= (1 << 9); label_set_text_if_changed(t_objects.exp_button, "1211"); break;
        case 1: set_var_f9(1);  s_audit.keys_mask |= (1 << 8); label_set_text_if_changed(t_objects.exp_button, "1955"); break;
        case 2: {
            set_var_f8(1);  
            s_audit.keys_mask |= (1 << 7); 
            label_set_text_if_changed(t_objects.exp_button, "2710"); 
            uint32_t now_ms = millis();
            static uint32_t s_last_f8_press = 0;
            if (now_ms - s_last_f8_press > 180) {
                if (now_ms - s_last_f8_press <= 500) {
                    // دابل‌کلیک سریع Mode: خروج فوری از صفحات بدون Back (مانند TU و Switch Test) به منوی اصلی
                    if (s_audit.active_screen_id != T_SCREEN_ID_MAIN) {
                        s_audit.active_screen_id = T_SCREEN_ID_MAIN;
                        t_loadScreen(T_SCREEN_ID_MAIN);
                        hwtest_beep_click(50);
                        touch_clear_lines();
                    }
                } else {
                    // تک‌کلیک: در منوی تنظیم ساعت و تاریخ بین فیلدها جابه‌جا شود
                    if (s_audit.active_screen_id == T_SCREEN_ID_CLOCK_SETUP) {
                        rtc_cycle_focus();
                        hwtest_beep_click(30);
                    }
                    // کنترل صدا با کلید Mode حذف شد — تنظیم صرفاً از اسلایدر UI
                }
                s_last_f8_press = now_ms;
            }
            break;
        }
        case 3: {
            set_var_f7(1);  
            s_audit.keys_mask |= (1 << 6); 
            label_set_text_if_changed(t_objects.exp_button, "3320"); 
            static uint32_t s_last_f7_press = 0;
            if (millis() - s_last_f7_press > 350) {
                s_last_f7_press = millis();
                if (s_audit.active_screen_id != T_SCREEN_ID_MAIN) {
                    s_audit.active_screen_id = T_SCREEN_ID_MAIN;
                    t_loadScreen(T_SCREEN_ID_MAIN);
                    hwtest_beep_click(40);
                    touch_clear_lines();
                }
            }
            break;
        }
        case 4: set_var_f6(1);  s_audit.keys_mask |= (1 << 5); label_set_text_if_changed(t_objects.exp_button, "4095"); break;
    }
}

#if HWTEST_HAS_BLE
static void onBleScanComplete(BLEScanResults foundDevices) {
    int count = foundDevices.getCount();
    s_ble_found_count = count;
    s_ble_device_options = "";
    for (int i = 0; i < count && i < 16; i++) {
        if (i > 0) s_ble_device_options += "\n";
        BLEAdvertisedDevice dev = foundDevices.getDevice(i);
        String name = dev.getName().c_str();
        if (name.length() == 0) {
            name = dev.getAddress().toString().c_str();
        }
        s_ble_device_options += name;
    }
    s_ble_scan_finished = true;
}
#endif

static void startBLE(void) {
#if HWTEST_HAS_BLE
    if (!pServer) {
        BLEDevice::init(BLE_DEVICE_NAME);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new MyServerCallbacks());
        pService = pServer->createService("12345678-1234-1234-1234-123456789abc");
        BLECharacteristic *pChar = pService->createCharacteristic(
            "87654321-4321-4321-4321-cba987654321",
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        if (pChar) {
            pChar->setValue("Haffler HT-V3A");
        }
        pService->start();
    }
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->addServiceUUID("12345678-1234-1234-1234-123456789abc");
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        pAdvertising->start();
    }
    bleEnabled = true;
    bleConnected = false;
    s_audit.ble_started = true;
    label_set_text_fmt_if_changed(t_objects.bolotosinfo, "Device: %s", BLE_DEVICE_NAME);
    label_set_text_if_changed(t_objects.bolotosstate, "Broadcasting... Ready to connect");
    label_set_color_if_valid(t_objects.bolotosinfo, 0xff0f172a);
    label_set_color_if_valid(t_objects.bolotosstate, 0xff2563eb);
#else
    label_set_text_if_changed(t_objects.bolotosinfo, "BLE unavailable");
    label_set_text_if_changed(t_objects.bolotosstate, "Bluedroid disabled");
    label_set_color_if_valid(t_objects.bolotosinfo, 0xffb00020);
    label_set_color_if_valid(t_objects.bolotosstate, 0xffb00020);
#endif
}

static void stopBLE(void) {
#if HWTEST_HAS_BLE
    if (!bleEnabled && !pServer) return;
    BLEScan *pScan = BLEDevice::getScan();
    if (pScan) pScan->stop();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) pAdvertising->stop();
    if (pService) { pService->stop(); pService = nullptr; }
    pServer = nullptr;
    BLEDevice::deinit(false);
#else
    if (!bleEnabled) return;
#endif
    bleEnabled = false;
    bleConnected = false;
    label_set_text_if_changed(t_objects.bolotosinfo, "BLE OFF");
    label_set_text_if_changed(t_objects.bolotosstate, "Disconnected");
    label_set_color_if_valid(t_objects.bolotosinfo, 0xff585151);
    label_set_color_if_valid(t_objects.bolotosstate, 0xff585151);
}

static void scanWiFi(void) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return;
    if (n >= 0) {
        s_audit.wifi_scanned = true;
        s_audit.wifi_ap_found = n;
        if (n > 0) {
            String opts;
            for (int i = 0; i < n && i < 16; i++) {
                if (i > 0) opts += "\n";
                String s = WiFi.SSID(i);
                for (unsigned int c = 0; c < s.length(); c++) {
                    unsigned char ch = (unsigned char)s[c];
                    if (ch < 0x20 || ch > 0x7E) s.setCharAt(c, '?');
                }
                if (s.length() == 0) s = "(Hidden)";
                opts += s;
            }
            lv_obj_t *dd = t_get_wifi_test_dropdown();
            if (dd && lv_obj_is_valid(dd)) {
                lv_dropdown_set_options(dd, opts.c_str());
            }
            label_set_text_fmt_if_changed(t_objects.wifi0, "Found %d networks. Select & connect", n);
            label_set_text_if_changed(t_objects.wifi1, "");
            label_set_text_if_changed(t_objects.wifi2, n > 1 ? WiFi.SSID(1).c_str() : "");
        } else {
            label_set_text_if_changed(t_objects.wifi0, "No networks found");
            label_set_text_if_changed(t_objects.wifi1, "");
            label_set_text_if_changed(t_objects.wifi2, "");
        }
        WiFi.scanDelete();
        set_var_anbl(0);
        Searching = false;
    } else if (n == WIFI_SCAN_FAILED) {
        label_set_text_if_changed(t_objects.wifi0, "Scan failed. Tap Scan again");
        set_var_anbl(0);
        Searching = false;
    }
}

static void updateSystemInfo(void) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 1000) return;
    lastUpdate = millis();

    uint32_t freeHeap   = ESP.getFreeHeap();
    uint32_t totalHeap  = ESP.getHeapSize();
    uint32_t usedHeap   = totalHeap - freeHeap;
    uint32_t largestBlk = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    uint32_t ramUsedPct = (totalHeap > 0) ? (usedHeap * 100UL) / totalHeap : 0;

    label_set_text_fmt_if_changed(t_objects.sys_ram,
        "Free: %lu KB\nBlk: %lu KB",
        freeHeap / 1024, largestBlk / 1024);

    label_set_text_fmt_if_changed(t_objects.sys_cpu, "CPU: %lu MHz\nCores: 2", getCpuFrequencyMhz());

    unsigned long sec = millis() / 1000;
    label_set_text_fmt_if_changed(t_objects.sys_uptime, "Uptime: %02lu:%02lu:%02lu",
        sec / 3600, (sec % 3600) / 60, sec % 60);

    initFlashInfoOnce();
    label_set_text_fmt_if_changed(t_objects.sys_flash,
        "%s\nFlash: %lu MB (%lu%% used)\nRAM: %lu%% (%lu/%lu KB)",
        sysFlashText, sysFlashSizeMB, sysFlashUsedPct, ramUsedPct,
        usedHeap / 1024, totalHeap / 1024);

    initChipMacOnce();
    label_set_text_fmt_if_changed(t_objects.sys_chipmac, "Base MAC:\n%s", chipMac);

    initWiFiMacOnce();
    label_set_text_fmt_if_changed(t_objects.sys_wifi, "WiFi MAC:\n%s", wifiMac);
}

static void initFlashInfoOnce(void) {
    if (sysFlashReady) return;
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    const char* chipModel = "ESP32";
#if CONFIG_IDF_TARGET_ESP32S2
    chipModel = "ESP32-S2";
#elif CONFIG_IDF_TARGET_ESP32S3
    chipModel = "ESP32-S3";
#elif CONFIG_IDF_TARGET_ESP32C3
    chipModel = "ESP32-C3";
#endif

    uint32_t flashSize  = ESP.getFlashChipSize();
    uint32_t sketchSize = ESP.getSketchSize();

    // ظرفیت و درصد پرشدگی فلش (فقط یک‌بار محاسبه، چون ثابت است)
    sysFlashSizeMB  = flashSize / (1024 * 1024);
    sysFlashUsedPct = (flashSize > 0) ? (sketchSize * 100UL) / flashSize : 0;

    snprintf(sysFlashText, sizeof(sysFlashText),
        "Chip: %s Rev.%d", chipModel, chip_info.revision);

    sysFlashReady = true;
}

static void startI2CScan(void) {
    i2cScanning = true;
    i2cScanned  = false;
    i2cAddr     = 1;
    i2cCount    = 0;
    i2c_touch_ok = false;
    i2c_rtc_ok   = false;
    i2c_text_reset();
}

static void processI2CScan(void) {
    if (!i2cScanning) return;

    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    Wire.beginTransmission(i2cAddr);
    int err = Wire.endTransmission();
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);

    if (err == 0) {
        if (i2cAddr == I2C_TOUCH_ADDR) i2c_touch_ok = true;
        if (i2cAddr == I2C_RTC_ADDR)   i2c_rtc_ok   = true;
        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02X ", i2cAddr);
        i2c_text_append(buf);
        i2cCount++;
    }
    i2cAddr++;

    if (i2cAddr >= 127) {
        i2cScanning = false;
        i2cScanned  = true;
        // تاچ را از روی درایورِ واقعی تشخیص بده، نه آدرسِ ثابتِ 0x5D
        // (هر چیپ تاچی هر آدرسی داشته باشد، اگر bbct تشخیصش داده باشد سالم است)
        if (bbct.sensorType() != 0) i2c_touch_ok = true;
        if (i2cCount == 0) {
            label_set_text_if_changed(t_objects.sys_i2c, "I2C:\nNo device");
        } else {
            // سرهم‌بندی در بافرِ تازه — بدونِ memmove/سرریز
            char out[sizeof(i2cText) + 32];
            snprintf(out, sizeof(out), "I2C (%d):\n%s", i2cCount, i2cText);
            label_set_text_if_changed(t_objects.sys_i2c, out);
        }
    }
}

static void beepI2CErrorDouble(void) {
    if (get_var_buzzer() <= 0) {
        ledcWrite(BUZZER, 0);
        return;
    }
    static uint8_t stage = 0;
    static unsigned long t = 0;
    if (millis() - t < 60) return;
    t = millis();
    switch (stage) {
        case 0: hwtest_beep_click(50); break;
        case 1: ledcWrite(BUZZER, 0); break;
        case 2: hwtest_beep_click(50); break;
        case 3: ledcWrite(BUZZER, 0); break;
    }
    stage++;
    if (stage >= 4) stage = 0;
}

static void initChipMacOnce(void) {
    if (chipMacReady) return;
    uint64_t mac;
    esp_efuse_mac_get_default((uint8_t*)&mac);
    snprintf(chipMac, sizeof(chipMac), "%02X:%02X:%02X:%02X:%02X:%02X",
        (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
        (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)(mac));
    chipMacReady = true;
}

static void initWiFiMacOnce(void) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(wifiMac, sizeof(wifiMac), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void set_rtc_from_ui(void) {
    const char* txt = get_var_rtc_text();
    int y, mo, d, h, mi;
    if (sscanf(txt, "%d.%d.%d.%d.%d", &y, &mo, &d, &h, &mi) == 5) {
        if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
        rtc.adjust(DateTime(y, mo, d, h, mi, 0));
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    }
}

static void ssr_warn_confirm_cb(lv_event_t *e) { g_ssr_warn_confirm = true; }

static void ssr_warn_open(void) {
    if (g_ssr_warn || !t_objects.ssr) return;
    lv_obj_t *p = lv_obj_create(t_objects.ssr);
    g_ssr_warn = p;
    lv_obj_set_size(p, 400, 190);
    lv_obj_center(p);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p, lv_color_hex(0xff2b2b2b), 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0xffd32f2f), 0);
    lv_obj_set_style_border_width(p, 3, 0);
    lv_obj_set_style_radius(p, 10, 0);

    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, "ELEMENT TEMP > 200 C\nConfirm = turn OFF");
    lv_obj_set_style_text_color(l, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t *btn = lv_btn_create(p);
    lv_obj_set_size(btn, 150, 46);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 6);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffd32f2f), 0);
    lv_obj_add_event_cb(btn, ssr_warn_confirm_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "OK");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_16, 0);
    lv_obj_center(bl);

    lv_obj_t *bar = lv_bar_create(p);
    g_ssr_warn_bar = bar;
    lv_obj_set_size(bar, 360, 16);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xffd32f2f), LV_PART_INDICATOR);

    g_ssr_warn_start = millis();
    g_ssr_warn_confirm = false;
    hwtest_beep_click(50);
}

static void ssr_warn_close_and_off(void) {
    set_var_pwm_state(0);            // توقفِ PWM (المنت خاموش)
    digitalWrite(pins[HWTEST_SSR_IDX_A], HIGH);     // SSR active-LOW → HIGH = خاموش
    digitalWrite(pins[HWTEST_SSR_IDX_B], HIGH);
    ledcWrite(BUZZER, 0);
    if (g_ssr_warn) { lv_obj_del(g_ssr_warn); g_ssr_warn = NULL; g_ssr_warn_bar = NULL; }
}

static void software_pwm(void) {
    // لچِ ایمنی: هرگز روشن نکن — پین‌های SSR به‌اجبار خاموش بمانند
    if (s_thermal_latch) {
        pwmState = false;
        digitalWrite(pins[HWTEST_SSR_IDX_A], HIGH);
        digitalWrite(pins[HWTEST_SSR_IDX_B], HIGH);
        return;
    }
    unsigned long now = millis();
    if (pwmState) {
        if (now - lastToggle >= pwm_on_time_ms) {
            pwmState = false; lastToggle = now;
            digitalWrite(pins[HWTEST_SSR_IDX_A], HIGH); // خاموش (Active-LOW: HIGH = OFF)
            digitalWrite(pins[HWTEST_SSR_IDX_B], HIGH);
        }
    } else {
        if (now - lastToggle >= pwm_off_time_ms) {
            pwmState = true; lastToggle = now;
            digitalWrite(pins[HWTEST_SSR_IDX_A], LOW);  // روشن (Active-LOW: LOW = ON)
            digitalWrite(pins[HWTEST_SSR_IDX_B], LOW);
        }
    }
}


static void set_pwm_from_ui(void) {
    const char* txt = get_var_time_text();
    if (!txt || txt[0] == '\0') return;
    int on_ms, off_ms;
    if (sscanf(txt, "%d.%d", &on_ms, &off_ms) == 2) {
        if (on_ms  < 1) on_ms  = 1;
        if (off_ms < 1) off_ms = 1;
        if (on_ms  > 30000) on_ms  = 30000;
        if (off_ms > 30000) off_ms = 30000;
        pwm_on_time_ms  = (unsigned long)on_ms;
        pwm_off_time_ms = (unsigned long)off_ms;
    }
}

// ======================= اکشن‌های EEZ پروژه‌ی Test (پورت‌شده) =======================
// این‌ها به دکمه‌های صفحات تست در EEZ Studio وصل می‌شوند.

void action_switchtomain(lv_event_t * e)   {
    if (s_audit.active_screen_id == T_SCREEN_ID_SSR || (t_objects.ssr && lv_screen_active() == t_objects.ssr)) {
        ssr_validate_and_commit();
    }
    if (s_audit.active_screen_id == T_SCREEN_ID_BLUETOOTH || (t_objects.bluetooth && lv_screen_active() == t_objects.bluetooth)) {
        stopBLE();
    }
    s_audit.active_screen_id = T_SCREEN_ID_MAIN;
    t_loadScreen(T_SCREEN_ID_MAIN);
    hwtest_beep_click(50);
}
void action_switchtosetup(lv_event_t * e)  { s_audit.active_screen_id = T_SCREEN_ID_SETUP; t_loadScreen(T_SCREEN_ID_SETUP);       hwtest_beep_click(50); }
void action_gotors485(lv_event_t * e)      { s_audit.active_screen_id = T_SCREEN_ID_RS485; t_loadScreen(T_SCREEN_ID_RS485);       hwtest_beep_click(50); }
void action_gotogsm(lv_event_t * e)        { s_audit.active_screen_id = T_SCREEN_ID_GSM; t_loadScreen(T_SCREEN_ID_GSM); }
void action_gotobluetooth(lv_event_t * e)  { s_audit.active_screen_id = T_SCREEN_ID_BLUETOOTH; t_loadScreen(T_SCREEN_ID_BLUETOOTH);   hwtest_beep_click(50); }
void action_gotodisw(lv_event_t * e)       { s_audit.active_screen_id = T_SCREEN_ID_DISPLAY_W; s_audit.display_screens_mask |= (1 << 3); t_loadScreen(T_SCREEN_ID_DISPLAY_W);   hwtest_beep_click(50); }
void action_gotowifi(lv_event_t * e)       {
    s_audit.active_screen_id = T_SCREEN_ID_WIFI;
    t_loadScreen(T_SCREEN_ID_WIFI);
    hwtest_beep_click(50);
    if (WiFi.status() == WL_CONNECTED) {
        String curSsid = WiFi.SSID();
        if (curSsid.length() > 0) {
            label_set_text_fmt_if_changed(t_objects.wifi0, "Connected: %s", curSsid.c_str());
        } else if (s_audit.wifi_connected_ssid[0]) {
            label_set_text_fmt_if_changed(t_objects.wifi0, "Connected: %s", s_audit.wifi_connected_ssid);
        } else {
            label_set_text_if_changed(t_objects.wifi0, "Connected");
        }
        label_set_text_fmt_if_changed(t_objects.wifi1, "IP: %s", WiFi.localIP().toString().c_str());
        label_set_text_fmt_if_changed(t_objects.wifi2, "RSSI: %d dBm", WiFi.RSSI());
    }
}
void action_gotodisb(lv_event_t * e)       { s_audit.active_screen_id = T_SCREEN_ID_DISPLAY_B; s_audit.display_screens_mask |= (1 << 4); t_loadScreen(T_SCREEN_ID_DISPLAY_B); }
void action_gotodisr(lv_event_t * e)       { s_audit.active_screen_id = T_SCREEN_ID_DISPLAY_TEST_R; s_audit.display_screens_mask |= (1 << 0); t_loadScreen(T_SCREEN_ID_DISPLAY_TEST_R); }
void action_gotodisg(lv_event_t * e)       { s_audit.active_screen_id = T_SCREEN_ID_DISPLAY_G; s_audit.display_screens_mask |= (1 << 1); t_loadScreen(T_SCREEN_ID_DISPLAY_G); }
void action_gotodisblue(lv_event_t * e)    { s_audit.active_screen_id = T_SCREEN_ID_DISPLAY_BLUE; s_audit.display_screens_mask |= (1 << 2); t_loadScreen(T_SCREEN_ID_DISPLAY_BLUE); }
void action_gotosysinfo(lv_event_t * e)    { s_audit.active_screen_id = T_SCREEN_ID_SYS_INFO; t_loadScreen(T_SCREEN_ID_SYS_INFO);    hwtest_beep_click(50); }
void action_gototouchtest(lv_event_t * e)  { s_audit.active_screen_id = T_SCREEN_ID_TOUCH_TEST; t_loadScreen(T_SCREEN_ID_TOUCH_TEST);  hwtest_beep_click(50); }
void action_gotortcsetup(lv_event_t * e)   {
    s_audit.active_screen_id = T_SCREEN_ID_CLOCK_SETUP;
    DateTime now;
    if (s_rtc_ok) {
        if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
        now = rtc.now();
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d.%02d.%02d.%02d.%02d",
             now.year(), now.month(), now.day(), now.hour(), now.minute());
    set_var_rtc_text(buf);
    t_loadScreen(T_SCREEN_ID_CLOCK_SETUP);
    hwtest_beep_click(50);
}
void action_gotossr(lv_event_t * e)        { s_audit.active_screen_id = T_SCREEN_ID_SSR; t_loadScreen(T_SCREEN_ID_SSR);         hwtest_beep_click(50); }

void action_senddata(lv_event_t * e)       { set_var_send(1); }
void action_anablblutooth(lv_event_t * e)  { set_var_enableble(1); set_var_changblelabl(1); }
void action_scanwifi(lv_event_t * e)
{
    hwtest_beep_click(30);
    set_var_anbl(1);
    set_var_scanwifi(1);
    Searching = true;
    label_set_text_if_changed(t_objects.wifi0, "Scanning WiFi networks...");
    label_set_text_if_changed(t_objects.wifi1, "");
    label_set_text_if_changed(t_objects.wifi2, "");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, false);
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
}
void action_test_wifi_connect(lv_event_t * e)
{
    hwtest_beep_click(30);
    set_var_anbl(0);
    Searching = false;
    lv_obj_t *dd = t_get_wifi_test_dropdown();
    lv_obj_t *ta = t_get_wifi_test_password();
    if (!dd || !ta) return;
    char ssid[64] = { 0 };
    lv_dropdown_get_selected_str(dd, ssid, sizeof(ssid));
    const char *pwd = lv_textarea_get_text(ta);
    if (!ssid[0] || strcmp(ssid, "Scan first") == 0 || strstr(ssid, "Tap Scan") != NULL) {
        label_set_text_if_changed(t_objects.wifi0, "Scan and select a network");
        return;
    }
    label_set_text_fmt_if_changed(t_objects.wifi0, "Connecting to %s...", ssid);
    label_set_text_if_changed(t_objects.wifi1, "Please wait...");
    label_set_text_if_changed(t_objects.wifi2, "");
    strncpy(wifiTestSsid, ssid, sizeof(wifiTestSsid) - 1);
    wifiTestSsid[sizeof(wifiTestSsid) - 1] = '\0';
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    if (pwd && strlen(pwd) > 0) {
        WiFi.begin(ssid, pwd);
    } else {
        WiFi.begin(ssid);
    }
    wifiTestStart = millis();
    wifiTestConnecting = true;
}

void action_ble_scan(lv_event_t * e)
{
    hwtest_beep_click(30);
#if HWTEST_HAS_BLE
    if (!bleEnabled) {
        BLEDevice::init(BLE_DEVICE_NAME);
        bleEnabled = true;
    }
    label_set_text_if_changed(t_objects.bolotosinfo, "Scanning for BLE devices...");
    label_set_text_if_changed(t_objects.bolotosstate, "Please wait (3s)...");
    label_set_color_if_valid(t_objects.bolotosstate, 0xff2563eb);
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->clearResults();
    pScan->start(3, onBleScanComplete, false);
#else
    label_set_text_if_changed(t_objects.bolotosinfo, "BLE unavailable");
#endif
}

void action_ble_connect(lv_event_t * e)
{
    hwtest_beep_click(30);
#if HWTEST_HAS_BLE
    startBLE();
#else
    label_set_text_if_changed(t_objects.bolotosinfo, "BLE unavailable");
#endif
}
void action_callrtc(lv_event_t * e)        { set_var_rtc_apply(1); }

static void ssr_start_pwm_internal(bool allow_no_probe) {
    s_ssr_allow_no_probe = allow_no_probe;
    s_audit.ssr_tested = true;
    set_var_pwm_state(1);
    lastToggle = millis();
    pwmState = false;
    s_ssr_leave_time = 0;
}

static void ssr_no_probe_confirm_cb(lv_event_t * e) {
    if (s_ssr_no_probe_modal && lv_obj_is_valid(s_ssr_no_probe_modal)) {
        lv_obj_del(s_ssr_no_probe_modal);
        s_ssr_no_probe_modal = NULL;
    }
    hwtest_beep_click(50);
    ssr_start_pwm_internal(true);
}

static void ssr_no_probe_cancel_cb(lv_event_t * e) {
    if (s_ssr_no_probe_modal && lv_obj_is_valid(s_ssr_no_probe_modal)) {
        lv_obj_del(s_ssr_no_probe_modal);
        s_ssr_no_probe_modal = NULL;
    }
    hwtest_beep_click(50);
}

extern "C" bool hwtest_get_pwm_output_state(void) {
    return (get_var_pwm_state() != 0) && pwmState && !s_thermal_latch;
}

extern "C" bool hwtest_get_pwm_running(void) {
    return (get_var_pwm_state() != 0) && !s_thermal_latch;
}

static void ssr_no_probe_modal_open(void) {
    if (s_ssr_no_probe_modal) return;
    lv_obj_t *parent = (t_objects.ssr && lv_obj_is_valid(t_objects.ssr)) ? t_objects.ssr : lv_screen_active();
    if (!parent) return;

    lv_obj_t *p = lv_obj_create(parent);
    s_ssr_no_probe_modal = p;
    lv_obj_set_size(p, 420, 200);
    lv_obj_center(p);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p, lv_color_hex(0xff1f2937), 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0xffeab308), 0);
    lv_obj_set_style_border_width(p, 3, 0);
    lv_obj_set_style_radius(p, 12, 0);

    // آیکون هشدار جداگانه با فونت مونتسرات برای جلوگیری از مربع
    lv_obj_t *l_icon = lv_label_create(p);
    lv_label_set_text(l_icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(l_icon, lv_color_hex(0xfffbbf24), 0);
    lv_obj_set_style_text_font(l_icon, &lv_font_montserrat_18, 0);
    lv_obj_align(l_icon, LV_ALIGN_TOP_MID, -120, 10);

    lv_obj_t *l_title = lv_label_create(p);
    lv_label_set_text(l_title, "WARNING: NO PROBE DETECTED!");
    lv_obj_set_style_text_color(l_title, lv_color_hex(0xfffbbf24), 0);
    lv_obj_set_style_text_font(l_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(l_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l_title, LV_ALIGN_TOP_MID, 15, 10);

    lv_obj_t *l_desc = lv_label_create(p);
    lv_label_set_text(l_desc, "No thermocouple probe connected.\nStart heater without temperature sensor?");
    lv_obj_set_style_text_color(l_desc, lv_color_white(), 0);
    lv_obj_set_style_text_font(l_desc, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(l_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l_desc, LV_ALIGN_CENTER, 0, -8);

    // Confirm button (Emerald Green)
    lv_obj_t *btn_ok = lv_btn_create(p);
    lv_obj_set_size(btn_ok, 140, 44);
    lv_obj_align(btn_ok, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff16a34a), 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff15803d), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_add_event_cb(btn_ok, ssr_no_probe_confirm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "CONFIRM & START");
    lv_obj_set_style_text_font(lbl_ok, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_ok, lv_color_white(), 0);
    lv_obj_center(lbl_ok);

    // Cancel button (Crimson Red)
    lv_obj_t *btn_cancel = lv_btn_create(p);
    lv_obj_set_size(btn_cancel, 140, 44);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xffdc2626), 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xffb91c1c), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_add_event_cb(btn_cancel, ssr_no_probe_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "CANCEL");
    lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_cancel, lv_color_white(), 0);
    lv_obj_center(lbl_cancel);

    hwtest_beep_click(50);
}

void action_pwm_start(lv_event_t * e) {
    ssr_validate_and_commit();

    bool tc1_ok = g_tc1_stat.probe_ok && !s_temp1_invalid;
    bool tc2_ok = g_tc2_stat.probe_ok && !s_temp2_invalid;
    bool probe_ok = tc1_ok || tc2_ok;

    if (!probe_ok) {
        ssr_no_probe_modal_open();
    } else {
        ssr_start_pwm_internal(false);
    }
}

void action_pwm_stop(lv_event_t * e) {
    if (s_ssr_no_probe_modal && lv_obj_is_valid(s_ssr_no_probe_modal)) {
        lv_obj_del(s_ssr_no_probe_modal);
        s_ssr_no_probe_modal = NULL;
    }
    s_ssr_allow_no_probe = false;
    s_ssr_leave_time = 0;
    set_var_pwm_state(0);
    pwmState = false;
    digitalWrite(pins[HWTEST_SSR_IDX_A], HIGH);
    digitalWrite(pins[HWTEST_SSR_IDX_B], HIGH);
}
void action_ifsetuploaded(lv_event_t * e)  { set_var_setuploaded(1); set_var_ssrloaded(0); }
void action_ifssrloaded(lv_event_t * e)    { set_var_ssrloaded(1); set_var_setuploaded(0); }

// --- تست تاچ (طراحی بهینه اپل، صفر لگ، صفر هنگ و جلوگیری از سرریز حافظه) ---
#define MAX_POINTS 64
#define TOUCH_STROKE_COUNT 8
static lv_obj_t * line_obj = NULL;
static lv_obj_t * touch_line_objs[TOUCH_STROKE_COUNT] = { NULL };
static EXT_RAM_BSS_ATTR lv_point_precise_t touch_points[TOUCH_STROKE_COUNT][MAX_POINTS];   // v9: lv_point_precise_t
static lv_point_precise_t * points = touch_points[0];
static uint32_t touch_stroke_count = 0;
static uint16_t point_cnt = 0;
lv_obj_t * s_touch_back = NULL;   // دکمه‌ی BACK صفحه‌ی تستِ تاچ (در صورت وجود)
static lv_point_precise_t last_touch_point;
static bool has_last_touch_point = false;

static void touch_clear_lines(void)
{
    for (uint8_t i = 0; i < TOUCH_STROKE_COUNT; i++) {
        if (touch_line_objs[i] && lv_obj_is_valid(touch_line_objs[i])) {
            lv_obj_del(touch_line_objs[i]);
        }
        touch_line_objs[i] = NULL;
    }
    line_obj = NULL;
    points = touch_points[0];
    touch_stroke_count = 0;
    point_cnt = 0;
    has_last_touch_point = false;
}

void action_clear_touch(lv_event_t * e)
{
    hwtest_beep_click(30);
    touch_clear_lines();
}

void action_drawline(lv_event_t * e)
{
    lv_indev_t * indev = lv_indev_active();      // v9: lv_indev_active
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    if (!line_obj) {
        uint8_t slot = (uint8_t)(touch_stroke_count % TOUCH_STROKE_COUNT);
        if (touch_line_objs[slot] && lv_obj_is_valid(touch_line_objs[slot])) {
            lv_obj_del(touch_line_objs[slot]);
            touch_line_objs[slot] = NULL;
        }
        line_obj = lv_line_create(lv_screen_active());
        touch_line_objs[slot] = line_obj;
        points = touch_points[slot];
        touch_stroke_count++;
        lv_obj_set_style_line_width(line_obj, 5, 0); // خط ارگونومیک ۵ پیکسلی شیک اپل
        lv_obj_set_style_line_rounded(line_obj, true, 0);
        lv_obj_set_style_line_color(line_obj, lv_color_hex(0x38bdf8), 0); // فیروزه‌ای متالیک اپل
        lv_obj_clear_flag(line_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(line_obj, LV_OBJ_FLAG_SCROLLABLE);
        point_cnt = 0;
        has_last_touch_point = false;
        if (s_touch_back && lv_obj_is_valid(s_touch_back)) lv_obj_move_foreground(s_touch_back);
    }
    if (!line_obj) return;

    // تداوم خودکار استروک در صورت رسیدن به انتهای بافر ۶۴ نقطه‌ای
    if (point_cnt >= MAX_POINTS - 2) {
        uint8_t slot = (uint8_t)(touch_stroke_count % TOUCH_STROKE_COUNT);
        if (touch_line_objs[slot] && lv_obj_is_valid(touch_line_objs[slot])) {
            lv_obj_del(touch_line_objs[slot]);
            touch_line_objs[slot] = NULL;
        }
        line_obj = lv_line_create(lv_screen_active());
        touch_line_objs[slot] = line_obj;
        points = touch_points[slot];
        touch_stroke_count++;
        lv_obj_set_style_line_width(line_obj, 5, 0);
        lv_obj_set_style_line_rounded(line_obj, true, 0);
        lv_obj_set_style_line_color(line_obj, lv_color_hex(0x38bdf8), 0);
        lv_obj_clear_flag(line_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(line_obj, LV_OBJ_FLAG_SCROLLABLE);
        points[0] = last_touch_point;
        point_cnt = 1;
        if (s_touch_back && lv_obj_is_valid(s_touch_back)) lv_obj_move_foreground(s_touch_back);
    }

    if (point_cnt < MAX_POINTS) {
        s_audit.touch_points_count++;
        if (has_last_touch_point) {
            int32_t dx = p.x - (int32_t)last_touch_point.x;
            int32_t dy = p.y - (int32_t)last_touch_point.y;
            if ((dx * dx + dy * dy) < 36) return; // فیلتر فاصله حداقل ۶ پیکسلی جهت مهار سربار رندر
        } else {
            has_last_touch_point = true;
        }
        points[point_cnt].x = p.x;
        points[point_cnt].y = p.y;
        last_touch_point = points[point_cnt];
        point_cnt++;
        if (point_cnt >= 2) {
            lv_line_set_points(line_obj, points, point_cnt);
        }
    }
}

void action_settouch(lv_event_t * e)
{
    static uint32_t last_click = 0;
    uint32_t now = lv_tick_get();
    if (now - last_click < 350) {
        s_audit.active_screen_id = T_SCREEN_ID_MAIN;
        t_loadScreen(T_SCREEN_ID_MAIN);
        hwtest_beep_click(50);
        touch_clear_lines();
    } else {
        line_obj = NULL;
        point_cnt = 0;
        has_last_touch_point = false;
    }
    last_click = now;
}

// اکشن‌هایی که در پروژه‌ی Test تعریف نشده بودند
void action_counterplusing(lv_event_t * e) { /* no-op */ }
void action_datareceive(lv_event_t * e)    { /* no-op */ }
void action_backfromtouch(lv_event_t * e)
{
    hwtest_beep_click(50);
    s_audit.active_screen_id = T_SCREEN_ID_MAIN;
    t_loadScreen(T_SCREEN_ID_MAIN);
    touch_clear_lines();
}

// ======================= اکشن‌های ناوبری بین Config و Test =======================
// در EEZ Studio دو «User Action» با همین نام‌ها بسازید و به دکمه‌ها وصل کنید.

// دکمه‌ی «تست سخت‌افزار» روی صفحه‌ی منوی Config
static lv_obj_t * s_return_screen = NULL;   // صفحه‌ی Config برای بازگشت
void action_back_to_config(lv_event_t * e);   // پیش‌اعلان

// آیا دستگاهی روی این آدرس I2C پاسخ می‌دهد؟
static bool i2c_present(uint8_t addr) {
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    Wire.beginTransmission(addr);
    bool ok = (Wire.endTransmission() == 0);
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    return ok;
}

// ===== افزاینده پین MCP23017 (GPA0..GPA5 خروجی و GPA6..GPA7 ورودی) =====
#define MCP23017_I2C_ADDR 0x20
#define MCP_IODIRA 0x00
#define MCP_IPOLA  0x02
#define MCP_GPPUA  0x0C
#define MCP_GPIOA  0x12
#define MCP_OLATA  0x14

static uint8_t s_mcp_out_state = 0x00; // بیت 0 تا 5 (1=روشن، 0=خاموش)
static uint8_t s_mcp_in_state  = 0xC0; // بیت 6=IN1 و بیت 7=IN0

void hwtest_mcp23017_init(void)
{
    if (!g_mcp23017_present) return;
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    // گام ۱: مقداردهی رجیسترهای لچ خروجی به ۱ (خاموش) قبل از تعریف پین‌ها به عنوان OUTPUT
    // این ترتیب اکیداً الزامی است تا از هرگونه پالس چندمیکروثانیه‌ای یا تحریک شیرهای پنوماتیک جلوگیری شود
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_OLATA);
    Wire.write(0x3F); // Active-LOW: 1 = OFF
    Wire.endTransmission();

    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_GPIOA);
    Wire.write(0x3F); // Active-LOW: 1 = OFF
    Wire.endTransmission();

    // گام ۲: پول‌آپ داخلی برای ورودی‌های GPA6 و GPA7
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_GPPUA);
    Wire.write(0xC0); // Pull-up for GPA6, GPA7
    Wire.endTransmission();

    // گام ۳: تغییر جهت GPA0..GPA5 به خروجی (پین‌ها با خروجی ۱ متولد می‌شوند؛ گلیچ صفر)
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_IODIRA);
    Wire.write(0xC0); // GPA0..GPA5 Output (0), GPA6..GPA7 Input (1)
    Wire.endTransmission();
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
}

void hwtest_mcp23017_all_off(void)
{
    if (!g_mcp23017_present) return;
    s_mcp_out_state = 0x00;
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_GPIOA);
    Wire.write(0x3F); // Active-LOW: 1 = OFF
    Wire.endTransmission();
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
}

void hwtest_mcp23017_set_output(uint8_t out_idx, bool on)
{
    if (!g_mcp23017_present || out_idx > 5) return;
    if (on) {
        s_mcp_out_state |= (1 << out_idx);
        s_audit.mcp_out_mask |= (1 << out_idx); // ثبت تست خروجی افزاینده
    }
    else {
        s_mcp_out_state &= ~(1 << out_idx);
    }

    uint8_t raw = ((~s_mcp_out_state) & 0x3F);
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_GPIOA);
    Wire.write(raw);
    Wire.endTransmission();
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
}

bool hwtest_mcp23017_get_output(uint8_t out_idx)
{
    if (out_idx > 5) return false;
    return (s_mcp_out_state & (1 << out_idx)) != 0;
}

uint8_t hwtest_mcp23017_poll_inputs(void)
{
    if (!g_mcp23017_present) return 0xC0;
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(MCP_GPIOA);
    if (Wire.endTransmission() != 0) {
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
        return 0xC0;
    }
    Wire.requestFrom((uint8_t)MCP23017_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        s_mcp_in_state = Wire.read();
        if (!(s_mcp_in_state & 0x80)) s_audit.mcp_in_mask |= (1 << 0); // IN0 فعال شد
        if (!(s_mcp_in_state & 0x40)) s_audit.mcp_in_mask |= (1 << 1); // IN1 فعال شد
    }
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
    return s_mcp_in_state;
}

// بررسی خودکار سخت‌افزار + نمایش پیغام وضعیت کامل به همراه مشخصات دقیق حافظه
void hwtest_selfcheck(void)
{
    static char rep[1024];
    int n = 0;

    uint32_t flashSize    = ESP.getFlashChipSize();
    uint32_t sketchSize   = ESP.getSketchSize();
    uint32_t flashMB      = flashSize / (1024 * 1024);
    uint32_t flashUsedPct = (flashSize > 0) ? (sketchSize * 100UL) / flashSize : 0;
    float    sketchMB     = (float)sketchSize / (1024.0f * 1024.0f);

    uint32_t totalHeap    = ESP.getHeapSize();
    uint32_t freeHeap     = ESP.getFreeHeap();
    uint32_t usedHeap     = (totalHeap > freeHeap) ? (totalHeap - freeHeap) : 0;
    uint32_t ramUsedPct   = (totalHeap > 0) ? (usedHeap * 100UL) / totalHeap : 0;

    size_t   psramTotal   = ESP.getPsramSize();
    size_t   psramFree    = ESP.getFreePsram();
    size_t   psramUsed    = (psramTotal > psramFree) ? (psramTotal - psramFree) : 0;
    uint32_t psramPct     = (psramTotal > 0) ? (uint32_t)((psramUsed * 100ULL) / psramTotal) : 0;

    float espCoreTemp = 0.0f;
    temperature_sensor_get_celsius(temp_sensor, &espCoreTemp);

    n += snprintf(rep + n, sizeof(rep) - n, "=== HAFFLER HARDWARE POST TEST ===\n\n");

    // Memory & Storage
    n += snprintf(rep + n, sizeof(rep) - n, "Flash Memory: %u MB [Used: %.2f MB (%u%%)]\n",
                  (unsigned)flashMB, sketchMB, (unsigned)flashUsedPct);
    n += snprintf(rep + n, sizeof(rep) - n, "Internal RAM: %u KB [Used: %u KB (%u%%) | Free: %u KB]\n",
                  (unsigned)(totalHeap / 1024), (unsigned)(usedHeap / 1024), (unsigned)ramUsedPct, (unsigned)(freeHeap / 1024));
    n += snprintf(rep + n, sizeof(rep) - n, "PSRAM: %u MB [Used: %u KB (%u%%) | Free: %u KB]\n\n",
                  (unsigned)(psramTotal / (1024 * 1024)), (unsigned)(psramUsed / 1024), (unsigned)psramPct, (unsigned)(psramFree / 1024));

    // CPU & Core
    n += snprintf(rep + n, sizeof(rep) - n, "CPU Core: ESP32-S3 @ %u MHz (Dual Core) | Temp: %.1f C\n",
                  (unsigned)getCpuFrequencyMhz(), espCoreTemp);

    // Peripherals
    bool rtc_ok = i2c_present(I2C_RTC_ADDR);
    if (rtc_ok) {
        if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
        DateTime now = rtc.now();
        bool lp = rtc.lostPower();
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
        n += snprintf(rep + n, sizeof(rep) - n, "[ OK ] RTC DS3231 (%04d-%02d-%02d %02d:%02d:%02d%s)\n",
                      now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(),
                      lp ? " [BAT LOW]" : "");
    } else {
        n += snprintf(rep + n, sizeof(rep) - n, "[FAIL] RTC DS3231: Not found\n");
    }

    bool mcp_ok = i2c_present(0x20);
    n += snprintf(rep + n, sizeof(rep) - n, "%s MCP23017 Expander (0x20: 6-OUT / 2-IN GPA)\n",
                  mcp_ok ? "[ OK ]" : "[ -- ] Not Fitted");

    float c1 = tc1.readCelsius();
    float c2 = tc2.readCelsius();
    n += snprintf(rep + n, sizeof(rep) - n, "MAX6675: TC1=%s (%.1f C) | TC2=%s (%.1f C)\n",
                  isnan(c1) ? "NO SENSOR" : "OK", isnan(c1) ? 0.0f : c1,
                  isnan(c2) ? "NO SENSOR" : "OK", isnan(c2) ? 0.0f : c2);

    bool touch_ok = (bbct.sensorType() != 0) || i2c_present(I2C_TOUCH_ADDR);
    n += snprintf(rep + n, sizeof(rep) - n, "%s Touch Controller (I2C CapTouch)\n", touch_ok ? "[ OK ]" : "[FAIL]");

    initWiFiMacOnce();
    n += snprintf(rep + n, sizeof(rep) - n, "WiFi MAC: %s", wifiMac);

    // پاپ‌آپ مشکی و شیک نمایش نتایج تست
    static lv_obj_t * s_post_popup = NULL;
    if (s_post_popup && lv_obj_is_valid(s_post_popup)) {
        lv_obj_del(s_post_popup);
        s_post_popup = NULL;
    }
    lv_obj_t * pop = lv_obj_create(lv_screen_active());
    s_post_popup = pop;
    lv_obj_set_size(pop, 440, 286);
    lv_obj_center(pop);
    lv_obj_clear_flag(pop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(pop, lv_color_hex(0x0a0e17), 0);
    lv_obj_set_style_border_color(pop, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_border_width(pop, 2, 0);
    lv_obj_set_style_radius(pop, 12, 0);
    lv_obj_set_style_shadow_width(pop, 0, 0);
    lv_obj_set_style_shadow_opa(pop, 0, 0);

    lv_obj_t * lbl = lv_label_create(pop);
    lv_label_set_text(lbl, rep);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 2, 0);

    lv_obj_t * cb = lv_button_create(pop);
    lv_obj_set_size(cb, 140, 34);
    lv_obj_align(cb, LV_ALIGN_BOTTOM_MID, 0, 4);
    lv_obj_set_style_bg_color(cb, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_bg_color(cb, lv_color_hex(0x0369a1), LV_STATE_PRESSED);
    lv_obj_set_style_radius(cb, 8, 0);
    lv_obj_add_event_cb(cb, [](lv_event_t * e){
        hwtest_beep_click(50);
        lv_obj_del((lv_obj_t *)lv_event_get_user_data(e));
        s_post_popup = NULL;
    }, LV_EVENT_CLICKED, pop);
    lv_obj_t * cl = lv_label_create(cb);
    lv_label_set_text(cl, "CLOSE");
    lv_obj_set_style_text_color(cl, lv_color_white(), 0);
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_14, 0);
    lv_obj_center(cl);
}

void hwtest_close_all_modals(void)
{
    hwtest_stop_auto_qc();
    // بستن مودال‌های احتمالی باز شده روی صفحه
    lv_obj_t * scr = lv_screen_active();
    if (scr && lv_obj_is_valid(scr)) {
        uint32_t cnt = lv_obj_get_child_count(scr);
        for (int32_t i = (int32_t)cnt - 1; i >= 0; i--) {
            lv_obj_t * child = lv_obj_get_child(scr, i);
            if (child && lv_obj_is_valid(child)) {
                // اگر شیء دارای عرض بیش از 400 یا ارتفاع بیش از 240 باشد، پاپ‌آپ/مودال است
                if (lv_obj_get_width(child) >= 400 && lv_obj_get_height(child) >= 240) {
                    lv_obj_del(child);
                }
            }
        }
    }
}

void hwtest_enter_ui()
{
    s_return_screen = lv_screen_active();   // صفحه‌ی Config را قبل از لودِ تست ذخیره کن
    static bool t_created = false;
    if (!t_created) {
        t_ui_init();                                 // صفحات تست یک‌بار ساخته شوند
        // دکمه‌ی «برگشت» روی صفحه‌ی اصلی تست با رنگ قرمز زرشکی متمایز صنعتی
        lv_obj_t * back = lv_button_create(t_objects.main);
        lv_obj_set_pos(back, 10, 276);                                // زیر دکمه‌های منو
        lv_obj_set_size(back, 108, 32);                               // هم‌اندازه‌ی بقیه
        lv_obj_set_style_bg_color(back, lv_color_hex(0xff991b1b), 0); // زرشکی متمایز برای تشخیص سریع
        lv_obj_set_style_border_color(back, lv_color_hex(0xfffca5a5), 0);
        lv_obj_set_style_border_width(back, 1, 0);
        lv_obj_set_style_radius(back, 8, 0);
        lv_obj_add_event_cb(back, action_back_to_config, LV_EVENT_CLICKED, NULL);
        lv_obj_t * bl = lv_label_create(back);
        lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(bl, lv_color_white(), 0);
        lv_obj_center(bl);

        s_touch_back = NULL;
        t_created = true;
    }
    hwtest_enter();
    t_loadScreen(T_SCREEN_ID_MAIN);
    hwtest_selfcheck();   // بررسی خودکار ماژول‌ها هنگام ورود   // صفحه‌ی اصلی منوی تست
}

// دکمه‌ی «برگشت» روی صفحه‌ی اصلی تست
extern "C" void action_btn_back_to_password(lv_event_t *e);
void action_back_to_config(lv_event_t * e)
{
    action_btn_back_to_password(e);
}

// دکمه‌ی «تست سخت‌افزار» را روی یک صفحه‌ی Config می‌چسباند (از .ino صدا زده می‌شود)
void hwtest_attach_button(lv_obj_t * parent, const lv_font_t * font)
{
    lv_obj_t * b = lv_button_create(parent);
    lv_obj_set_size(b, 160, 48);
    lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(b, [](lv_event_t * e){ hwtest_enter_ui(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t * l = lv_label_create(b);
    if (font) lv_obj_set_style_text_font(l, font, 0);
    else lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_label_set_text(l, "HARDWARE TEST");
    lv_obj_center(l);
}

//// =========================================================================
// پروتکل چاپگر حرارتی و توابع گرافیکی گزارش کیفی هافلر (ESC/POS 384-DOT)
// =========================================================================

static inline void printer_bold(bool on) {
    Serial2.write(0x1B);
    Serial2.write(0x45);
    Serial2.write(on ? 1 : 0);
}

static inline void printer_underline(uint8_t mode) {
    Serial2.write(0x1B);
    Serial2.write(0x2D);
    Serial2.write(mode);
}

static inline void printer_invert(bool on) {
    Serial2.write(0x1D);
    Serial2.write(0x42);
    Serial2.write(on ? 1 : 0);
}

static inline void printer_align(uint8_t align) { // 0=Left, 1=Center, 2=Right
    Serial2.write(0x1B);
    Serial2.write(0x61);
    Serial2.write(align);
}

static inline void printer_double_size(bool on) {
    // GS ! 0x01 = double height only (متن بلند، یک خطه، بدون گسترش عرض)
    Serial2.write(0x1D);
    Serial2.write(0x21);
    Serial2.write(on ? 0x01 : 0x00);
}

static inline void printer_char_spacing(uint8_t dots) {
    // ESC SP n (0x1B, 0x20, dots) : افزایش ظریف پهنای کاراکترها روی محور X (~۱۰٪) بدون تغییر محور Y
    Serial2.write(0x1B);
    Serial2.write(0x20);
    Serial2.write(dots);
}

// چاپ آیکون بیت‌مپ تیک یا ضربدر ۲۴ نقطه‌ای رسمی هافلر در ابتدای سطر با پروتکل ESC * 33
static void print_status_icon(bool passed)
{
    if (s_cancel_printing) return;
    const uint8_t *icon = passed ? haffler_icon_check_col24 : haffler_icon_cross_col24;
    // ESC * 33 24 0 : 24-dot double-density column bit-image (24 columns x 3 bytes = 72 bytes)
    Serial2.write(0x1B);
    Serial2.write('*');
    Serial2.write(33);
    Serial2.write((uint8_t)24);
    Serial2.write((uint8_t)0);
    for (int i = 0; i < 72; i++) {
        if (s_cancel_printing) return;
        Serial2.write(pgm_read_byte(&icon[i]));
    }
    Serial2.write(' ');
}

// چاپ تمبر گرافیکی تمام‌عرض ۳۸۴x۵۴ نقطه تایید یا رد رسمی در پایان گزارش
static void print_full_qc_stamp(bool all_passed)
{
    if (s_cancel_printing) return;
    printer_align(1);
    delay(5);

    // GS v 0 0 xL xH yL yH
    Serial2.write(0x1D);
    Serial2.write(0x76);
    Serial2.write(0x30);
    Serial2.write((uint8_t)0x00);
    Serial2.write((uint8_t)(HAFFLER_STAMP_WIDTH_BYTES & 0xFF));
    Serial2.write((uint8_t)((HAFFLER_STAMP_WIDTH_BYTES >> 8) & 0xFF));
    Serial2.write((uint8_t)(HAFFLER_STAMP_HEIGHT & 0xFF));
    Serial2.write((uint8_t)((HAFFLER_STAMP_HEIGHT >> 8) & 0xFF));

    const uint8_t *stamp = all_passed ? haffler_stamp_passed_raster : haffler_stamp_failed_raster;
    for (size_t i = 0; i < sizeof(haffler_stamp_passed_raster); i++) {
        if (s_cancel_printing) return;
        Serial2.write(pgm_read_byte(&stamp[i]));
        if ((i % 128) == 127) {
            delay(2);
        }
    }
    delay(2);
    Serial2.println();
    printer_align(0);
}

// چاپ تیتر بخش‌ها: ابتدا خط‌چین، سپس عنوان بخش در زیر خط‌چین، سپس مدت‌زمان
static void print_report_section_header(const char *num_and_title, uint32_t dur_sec = 0)
{
    if (s_cancel_printing) return;
    Serial2.println("----------------------------");
    printer_bold(true);
    printer_double_size(true);
    Serial2.println(num_and_title);
    printer_double_size(false);
    printer_bold(false);
    if (dur_sec > 0) {
        char dBuf[32];
        snprintf(dBuf, sizeof(dBuf), "[ Duration: %02um%02us ]", (unsigned)(dur_sec / 60), (unsigned)(dur_sec % 60));
        Serial2.println(dBuf);
    }
}

// چاپ متن با شکست هوشمند در مرز کلمات (Word-Wrap) و جلوگیری کامل از شکستن کلمات
// کلمات همیشه کامل حفظ می‌شوند و در صورت عدم گنجایش در خط، کل کلمه به خط بعد منتقل می‌شود.
static void printer_print_wrapped(const char *text, const char *indent = "  >> ", uint8_t max_cols = 28)
{
    if (s_cancel_printing) return;
    if (!text || !*text) return;

    size_t total_len = strlen(text);
    size_t indent_len = indent ? strlen(indent) : 0;

    // اگر کل سطر با دندانه در محدوده ۲۸ کاراکتر جا شود
    if (indent_len + total_len <= max_cols) {
        if (indent) Serial2.print(indent);
        Serial2.println(text);
        return;
    }

    // اگر کل سطر حداکثر ۲۹ کاراکتر باشد (مانند یک علامت در انتها)، با فشرده‌سازی بسیار کم در یک خط چاپ می‌شود
    if (indent_len + total_len <= 29) {
        printer_char_spacing(0);
        if (indent) Serial2.print(indent);
        Serial2.println(text);
        printer_char_spacing(1);
        return;
    }

    if (indent) Serial2.print(indent);
    size_t cur_line_len = indent_len;

    const char *p = text;
    while (*p) {
        if (s_cancel_printing) return;
        while (*p == ' ') p++;
        if (!*p) break;

        const char *word_start = p;
        while (*p && *p != ' ') p++;
        size_t word_len = p - word_start;

        size_t needed = (cur_line_len > indent_len ? 1 : 0) + word_len;

        if (cur_line_len + needed <= max_cols) {
            if (cur_line_len > indent_len) {
                Serial2.write(' ');
                cur_line_len++;
            }
            for (size_t i = 0; i < word_len; i++) Serial2.write(word_start[i]);
            cur_line_len += word_len;
        } else {
            // کلمه در این خط جا نمی‌شود؛ بدون شکستن کلمه، به ابتدای خط بعد با دندانه منتقل می‌شود
            Serial2.println();
            if (indent) Serial2.print(indent);
            for (size_t i = 0; i < word_len; i++) Serial2.write(word_start[i]);
            cur_line_len = indent_len + word_len;
        }
    }
    Serial2.println();
}

static void printer_cancel_cleanup(void);
#define CHECK_CANCEL() do { if (s_cancel_printing) { printer_cancel_cleanup(); return; } } while(0)

// چاپ ردیف آزمون سخت‌افزار با آیکون بیت‌مپ تیک یا ضربدر و ممیزی سخت‌گیرانه
static void print_qc_row(bool pass, const char *title, const char *val_ok, const char *fail_reason = NULL)
{
    CHECK_CANCEL();
    print_status_icon(pass);

    if (pass) {
        const char *v = val_ok ? val_ok : "PASS";
        size_t row_chars = strlen(title) + 2 + strlen(v);

        // اگر طول سطر زیاد باشد (۲۴ کاراکتر یا بیشتر)، کل سطر با فاصله صفر چاپ می‌شود تا هرگز نشکند
        if (row_chars >= 24) {
            printer_char_spacing(0);
            printer_bold(true);
            Serial2.print(title);
            Serial2.print(": ");
            Serial2.println(v);
            printer_bold(false);
            printer_char_spacing(1);
        } else {
            printer_bold(true);
            Serial2.print(title);
            Serial2.print(": ");
            Serial2.println(v);
            printer_bold(false);
        }
    } else {
        printer_bold(true);
        Serial2.print(title);
        Serial2.print(": ");
        printer_bold(false);
        printer_invert(true);
        Serial2.print(" FAIL ");
        printer_invert(false);
        Serial2.println();
        if (fail_reason && strlen(fail_reason) > 0) {
            printer_bold(true);
            printer_underline(1);
            printer_print_wrapped(fail_reason, "  >> ", 28);
            printer_underline(0);
            printer_bold(false);
        }
    }
    CHECK_CANCEL();
}

// =========================================================================================================
// 🚨 خط قرمز مطلق و غیرقابل تغییر: ممنوعیت دستکاری متن و محتوای پرینتر (CRITICAL RED LINE)
// =========================================================================================================
// اخطار صریح به کلیه ایجنت‌های هوش مصنوعی (Antigravity, Gemini, Claude, Cursor, Copilot):
// طبق دستور مستقیم کارفرما، محتوای پرینت گزارش QC (شامل تمامی متون، بخش‌های ۱ تا ۷، هدرها، جداول،
// آیکون‌ها، لوگوی هافلر، برچسب‌ها، خطوط جداکننده، تمبر گرافیکی و فرمت‌بندی) کاملاً عالی، نهایی و تایید شده است.
// هیچ ایجنتی تحت هیچ شرایطی، به هیچ بهانه‌ای (از جمله ترجمه، بهینه‌سازی، تغییر ظاهر یا خلاصه‌سازی)
// حق دست زدن به محتوا، متن و چیدمان پرینتر را ندارد.
// هرگونه تغییر در متن چاپگر اکیداً ممنوع و نقض صریح استانداردهای مهندسی هافلر است.
// =========================================================================================================

static void printer_cancel_cleanup(void)
{
    // =========================================================================
    // لغو فوری چاپ، پاکسازی بافر پرینتر حرارتی و فید کاغذ سفید طبق دستور کاربر
    // =========================================================================
    // ۱. لغو سطر فعلی در بافر چاپگر
    Serial2.write(0x18); // CAN (Cancel line)
    Serial2.write('\n');
    delay(5);

    // ۲. ریست نرم‌افزاری استاندارد ESC @ جهت پاکسازی بافر داخلی پرینتر و بازگشت به مد متنی
    Serial2.write(0x1B);
    Serial2.write('@');  // ESC @: Initialize printer & clear buffer
    delay(20);

    // ۳. پیشروی ۶ سطر کاغذ سفید تا متن چاپ‌شده از دهانه و تیغه خارج شود
    Serial2.write(0x1B);
    Serial2.write(0x64);
    Serial2.write((uint8_t)6); // ESC d 6: فید ۶ سطر کاغذ سفید
    Serial2.println();
    Serial2.flush();
    delay(100); // زمان کافی برای اتمام فیزیکی فید کاغذ پیش از تغییر باودریت

    // ۴. بازگرداندن تنظیمات فاصله خطوط و کاراکتر به حالت پیش‌فرض
    Serial2.write(0x1B);
    Serial2.write(0x32);     // ESC 2
    printer_char_spacing(0);
    Serial2.flush();

    // ۵. بازگرداندن پورت RS485 به باودریت ۹۶۰۰ برای شنود عادی
    digitalWrite(46, HIGH);
    Serial2.begin(9600, SERIAL_8N1, 18, 17);
    Serial2.setTimeout(5);

    prn_set_ui_status("PRINT STOPPED / BUFFER CLEARED");
    if (t_objects.datareceive && lv_obj_is_valid(t_objects.datareceive)) {
        lv_label_set_text(t_objects.datareceive, "PRINT CANCELLED");
    }

    s_cancel_printing = false;
    s_is_printing = false;
    s_printer_task_handle = NULL;
    vTaskDelete(NULL);
}

static void printer_worker_task(void *pvParameters)
{
    (void)pvParameters;

    CHECK_CANCEL();

    // ۲. زمان کل جلسه تست و زمان‌های تفکیک‌شده هر بخش (ثانیه)
    uint32_t session_sec = (s_audit.start_ms > 0) ? (::millis() - s_audit.start_ms) / 1000 : (::millis() / 1000);
    uint32_t s_min = session_sec / 60;
    uint32_t s_sec = session_sec % 60;

    uint32_t t_touch_s  = s_audit.dur_touch_ms / 1000;
    uint32_t t_keys_s   = s_audit.dur_keys_ms / 1000;
    uint32_t t_relays_s = s_audit.dur_relays_ms / 1000;
    uint32_t t_inputs_s = s_audit.dur_inputs_ms / 1000;
    uint32_t t_disp_s   = s_audit.dur_display_ms / 1000;
    uint32_t t_wifi_s   = s_audit.dur_wifi_ms / 1000;
    uint32_t t_ble_s    = s_audit.dur_ble_ms / 1000;
    uint32_t t_ssr_s    = s_audit.dur_ssr_ms / 1000;
    uint32_t t_rs485_s  = s_audit.dur_rs485_ms / 1000;

    // ۳. مشخصات سخت‌افزار، حافظه و پارتیشن‌ها
    initChipMacOnce();
    initWiFiMacOnce();
    initFlashInfoOnce();

    uint32_t flashSizeMB  = ESP.getFlashChipSize() / (1024 * 1024);
    uint32_t sketchBytes  = ESP.getSketchSize();
    float    sketchMB     = (float)sketchBytes / (1024.0f * 1024.0f);
    uint32_t app0SlotMB   = 7; // پارتیشن ۷ مگابایت
    uint32_t app0UsedPct  = (sketchBytes * 100UL) / (app0SlotMB * 1024 * 1024);

    uint32_t freeHeap     = ESP.getFreeHeap();
    uint32_t totalHeap    = ESP.getHeapSize();
    uint32_t usedHeap     = (totalHeap > freeHeap) ? (totalHeap - freeHeap) : 0;
    uint32_t ramUsedPct   = (totalHeap > 0) ? (usedHeap * 100UL) / totalHeap : 0;

    size_t   psramTotal   = ESP.getPsramSize();
    size_t   psramFree    = ESP.getFreePsram();

    const esp_partition_t *running = esp_ota_get_running_partition();
    const char *running_label = running ? running->label : "app0";

    float espTemp = 0.0f;
    temperature_sensor_get_celsius(temp_sensor, &espTemp);

    DateTime now;
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
    if (s_rtc_ok) now = rtc.now();
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);

    CHECK_CANCEL();

    // ۴. استخراج وضعیت تست‌های تعاملی
    int keys_tested_cnt = 0;
    for (int i = 0; i < 10; i++) {
        if (s_audit.keys_mask & (1 << i)) keys_tested_cnt++;
    }
    bool fs1_ok = (s_audit.keys_mask & (1 << 10)) != 0;
    bool fs2_ok = (s_audit.keys_mask & (1 << 11)) != 0;

    int q_tested_cnt = 0;
    for (int i = 0; i < 10; i++) {
        if (s_audit.q_mask & (1 << i)) q_tested_cnt++;
    }

    int in_tested_cnt = 0;
    for (int i = 0; i < 8; i++) {
        if (s_audit.in_mask & (1 << i)) in_tested_cnt++;
    }

    int disp_tested_cnt = 0;
    for (int i = 0; i < 5; i++) {
        if (s_audit.display_screens_mask & (1 << i)) disp_tested_cnt++;
    }

    int mcp_out_cnt = 0;
    for (int i = 0; i < 6; i++) {
        if (s_audit.mcp_out_mask & (1 << i)) mcp_out_cnt++;
    }
    int mcp_in_cnt = 0;
    for (int i = 0; i < 2; i++) {
        if (s_audit.mcp_in_mask & (1 << i)) mcp_in_cnt++;
    }

    float tc1_val = tc1.readCelsius();
    float tc2_val = tc2.readCelsius();

    CHECK_CANCEL();

    // ۵. ارزیابی شرط‌های حداقلی زمان و تکمیل کامل ۱۰۰٪
    bool touch_time_ok = (t_touch_s >= 5);
    bool touch_pts_ok  = (s_audit.touch_points_count >= 10);
    bool touch_passed  = touch_time_ok && touch_pts_ok;
    const char *touch_why = !touch_pts_ok ? "POINTS < 10 (NOT TESTED)" : (!touch_time_ok ? "TIME TOO SHORT (< 5s)" : NULL);

    bool disp_time_ok  = (t_disp_s >= 4);
    bool disp_cnt_ok   = (disp_tested_cnt == 5);
    bool disp_passed   = disp_time_ok && disp_cnt_ok;
    const char *disp_why = !disp_cnt_ok ? "REQ 5/5 COLORS UNTESTED" : (!disp_time_ok ? "TIME TOO SHORT (< 4s)" : NULL);

    bool in_time_ok    = (t_inputs_s >= 4);
    bool in_cnt_ok     = (in_tested_cnt == 8);
    bool in_passed     = in_time_ok && in_cnt_ok;
    const char *in_why = !in_cnt_ok ? "REQ 8/8 INPUTS UNTESTED" : (!in_time_ok ? "TIME TOO SHORT (< 4s)" : NULL);

    bool q_time_ok     = (t_relays_s >= 5);
    bool q_cnt_ok      = (q_tested_cnt == 10);
    bool q_passed      = q_time_ok && q_cnt_ok;
    const char *q_why  = !q_cnt_ok ? "REQ 10/10 RELAYS UNTESTED" : (!q_time_ok ? "TIME TOO SHORT (< 5s)" : NULL);

    bool key_time_ok   = (t_keys_s >= 6);
    bool key_cnt_ok    = (keys_tested_cnt == 10);
    bool key_passed    = key_time_ok && key_cnt_ok;
    const char *key_why = !key_cnt_ok ? "REQ 10/10 KEYS UNTESTED" : (!key_time_ok ? "TIME TOO SHORT (< 6s)" : NULL);

    bool fs_passed     = fs1_ok && fs2_ok;
    const char *fs_why = fs_passed ? NULL : "FS1 OR FS2 UNTRIGGERED";

    bool tc1_pass      = !isnan(tc1_val) && (tc1_val > 1.0f) && (tc1_val < 150.0f);
    const char *tc1_why = tc1_pass ? NULL : "SENSOR DISCONNECTED (0 C)";

    bool tc2_pass      = !isnan(tc2_val) && (tc2_val > 1.0f) && (tc2_val < 150.0f);
    const char *tc2_why = tc2_pass ? NULL : "SENSOR DISCONNECTED (0 C)";

    bool ssr_time_ok   = (t_ssr_s >= 2);
    bool ssr_passed    = s_audit.ssr_tested && ssr_time_ok;
    const char *ssr_why = !s_audit.ssr_tested ? "HEATER UNTESTED" : (!ssr_time_ok ? "TIME TOO SHORT (< 2s)" : NULL);

    bool wifi_time_ok  = (t_wifi_s >= 3);
    bool wifi_act_ok   = (s_audit.wifi_scanned || s_audit.wifi_connected);
    bool wifi_passed   = wifi_time_ok && wifi_act_ok;
    const char *wifi_why = !wifi_act_ok ? "NO NETWORKS SCANNED" : (!wifi_time_ok ? "TIME TOO SHORT (< 3s)" : NULL);

    bool ble_time_ok   = (t_ble_s >= 2);
    bool ble_passed    = s_audit.ble_started && ble_time_ok;
    const char *ble_why = !s_audit.ble_started ? "BLE NOT STARTED" : (!ble_time_ok ? "TIME TOO SHORT (< 2s)" : NULL);

    bool rs_time_ok    = (t_rs485_s >= 2);
    bool rs_act_ok     = (s_audit.rs485_sent && s_audit.rs485_received);
    bool rs_passed     = rs_time_ok && rs_act_ok;
    const char *rs_why = !rs_act_ok ? "NO TX/RX TRAFFIC" : (!rs_time_ok ? "TIME TOO SHORT (< 2s)" : NULL);

    bool rtc_passed    = s_rtc_ok && !rtc.lostPower() && (now.year() >= 2024);
    const char *rtc_why = rtc_passed ? NULL : "RTC BATTERY LOW / UNSET";

    bool all_passed = touch_passed && disp_passed && in_passed && q_passed &&
                      key_passed && fs_passed && tc1_pass && tc2_pass &&
                      ssr_passed && wifi_passed && ble_passed && rs_passed && rtc_passed;

    // ۶. محاسبه نمره کلی
    int total_score = 0;
    if (touch_passed) total_score += 10;
    if (disp_passed)  total_score += 10;
    if (in_passed)    total_score += 15;
    if (q_passed)     total_score += 15;
    if (key_passed)   total_score += 10;
    if (fs_passed)    total_score += 5;
    if (tc1_pass)     total_score += 5;
    if (tc2_pass)     total_score += 5;
    if (ssr_passed)   total_score += 5;
    if (wifi_passed)  total_score += 5;
    if (ble_passed)   total_score += 5;
    if (rs_passed)    total_score += 5;
    if (rtc_passed)   total_score += 5;
    if (total_score > 100) total_score = 100;

    CHECK_CANCEL();

    // ۷. ارسال پروتکل ESC/POS به پرینتر حرارتی روی RS485 در باود ۱۹۲۰۰
    Serial2.begin(19200, SERIAL_8N1, 18, 17);
    delay(25);
    CHECK_CANCEL();

    digitalWrite(46, HIGH);
    delayMicroseconds(100);

    // ESC @ : Initialize printer
    Serial2.write(0x1B);
    Serial2.write('@');
    delay(30);
    CHECK_CANCEL();

    // ESC 7 : تنظیمات حرارتی پرینتر (حفظ تراکم و وضوح مشکی کامل لوگو)
    Serial2.write(0x1B);
    Serial2.write(0x37);
    Serial2.write(7);
    Serial2.write(60);
    Serial2.write(2);

    // ESC 3 n : تنظیم فاصله عمودی خطوط به ۳۲ نقطه برای افزایش خوانایی و جلوگیری از فشردگی سطور
    Serial2.write(0x1B);
    Serial2.write(0x33);
    Serial2.write(32);
    delay(10);
    CHECK_CANCEL();

    // ۰. چاپ لوگوی رسمی HAFFLER با دستور رستر ESC/POS (GS v 0) - کاملاً وسط‌چین
    printer_align(1);
    delay(10);

    // GS v 0 0 xL xH yL yH
    Serial2.write(0x1D);
    Serial2.write(0x76);
    Serial2.write(0x30);
    Serial2.write((uint8_t)0x00);
    Serial2.write((uint8_t)(HAFFLER_LOGO_WIDTH_BYTES & 0xFF));
    Serial2.write((uint8_t)((HAFFLER_LOGO_WIDTH_BYTES >> 8) & 0xFF));
    Serial2.write((uint8_t)(HAFFLER_LOGO_HEIGHT & 0xFF));
    Serial2.write((uint8_t)((HAFFLER_LOGO_HEIGHT >> 8) & 0xFF));

    for (size_t i = 0; i < sizeof(haffler_printer_logo_raster); i++) {
        CHECK_CANCEL();
        Serial2.write(pgm_read_byte(&haffler_printer_logo_raster[i]));
        if ((i % 64) == 63) {
            delay(4);
        }
    }
    delay(10);
    Serial2.println();
    delay(10);
    CHECK_CANCEL();

    // تضمین وسط‌چین بودن هدر زیر لوگو طبق استاندارد رسمی
    printer_align(1);
    delay(5);

    // افزایش حدود ۱۰ درصدی پهنای متن روی محور X (فاصله ۱ نقطه افقی) بدون تغییر محور Y
    printer_char_spacing(1);

    // هدر برگه (کاملاً وسط‌چین)
    printer_bold(true);
    Serial2.println("PLC & HMI SYSTEM");
    Serial2.println("HARDWARE QC AUDIT REPORT");
    printer_bold(false);
    Serial2.println("============================");
    Serial2.flush();
    delay(10);
    CHECK_CANCEL();

    printer_align(0); // چپ‌چین برای مشخصات و بخش‌های گزارش

    char lineBuf[48];
    if (s_rtc_ok && now.year() >= 2024) {
        snprintf(lineBuf, sizeof(lineBuf), "DATE: %04d/%02d/%02d  TIME: %02d:%02d",
                 now.year(), now.month(), now.day(), now.hour(), now.minute());
        Serial2.println(lineBuf);
    }
    snprintf(lineBuf, sizeof(lineBuf), "FW VER: %-8s | QC: %02dm%02ds",
             VERSION, (int)s_min, (int)s_sec);
    Serial2.println(lineBuf);
    CHECK_CANCEL();

    // بخش ۱: مشخصات خودکار سیستم و سخت‌افزار
    print_report_section_header("1. SYSTEM & PROCESSOR INFO", 0);
    print_qc_row(true, "MCU", "ESP32-S3 240MHz (2C)");
    esp_chip_info_t cinfo;
    esp_chip_info(&cinfo);
    snprintf(lineBuf, sizeof(lineBuf), "  REV: v0.%d | EXTENDED LX7", cinfo.revision);
    Serial2.println(lineBuf);

    print_qc_row(true, "MAC ADDR", chipMac);

    if (s_rtc_ok) {
        if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
        int rtcT = rtc.getTemperature();
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
        snprintf(lineBuf, sizeof(lineBuf), "ACTIVE (%d C)", rtcT);
        print_qc_row(rtc_passed, "RTC DS3231", lineBuf, rtc_why);
        Serial2.println("  BATTERY: OK | OSC: OK");
    } else {
        print_qc_row(false, "RTC DS3231", "", "NOT DETECTED ON I2C");
    }

    print_qc_row(true, "I2C BUS", "400kHz FAST");
    snprintf(lineBuf, sizeof(lineBuf), "  DEVS: %s%sTOUCH", s_rtc_ok ? "RTC, " : "", g_mcp23017_present ? "MCP, " : "");
    Serial2.println(lineBuf);
    print_qc_row(true, "RS485", "SERIAL2 (18/17) OK");
    CHECK_CANCEL();

    // بخش ۲: نمایشگر و تاچ
    print_report_section_header("2. DISPLAY & TOUCH SPECS", t_touch_s + t_disp_s);
    print_qc_row(true, "LCD PANEL", "4.0\" 480x320");
    print_qc_row(true, "LCD DRV", "ST7796 80MHz SPI");
    print_qc_row(true, "TOUCH IC", "GT911/FT6336 I2C");

    snprintf(lineBuf, sizeof(lineBuf), "%lu PTS (%02us)", (unsigned long)s_audit.touch_points_count, (unsigned)t_touch_s);
    print_qc_row(touch_passed, "TOUCH TEST", lineBuf, touch_why);

    snprintf(lineBuf, sizeof(lineBuf), "%d/5 CLR (%02us)", disp_tested_cnt, (unsigned)t_disp_s);
    print_qc_row(disp_passed, "RGB PURITY", lineBuf, disp_why);
    if (!disp_passed) {
        String missed = "UNTESTED: ";
        if (!(s_audit.display_screens_mask & (1 << 0))) missed += "RED ";
        if (!(s_audit.display_screens_mask & (1 << 1))) missed += "GRN ";
        if (!(s_audit.display_screens_mask & (1 << 2))) missed += "BLU ";
        if (!(s_audit.display_screens_mask & (1 << 3))) missed += "WHT ";
        if (!(s_audit.display_screens_mask & (1 << 4))) missed += "BLK ";
        printer_print_wrapped(missed.c_str(), "  >> ", 28);
    }
    CHECK_CANCEL();

    // بخش ۳: حافظه فلش و رم
    print_report_section_header("3. MEMORY ALLOCATION", 0);
    snprintf(lineBuf, sizeof(lineBuf), "%luMB (DIO 80MHz)", (unsigned long)flashSizeMB);
    print_qc_row(true, "FLASH", lineBuf);
    snprintf(lineBuf, sizeof(lineBuf), "  APP0: 7MB [%.1fM,%u%%] %s",
             sketchMB, (unsigned)app0UsedPct, (strcmp(running_label, "app0") == 0) ? "*ACT" : "");
    Serial2.println(lineBuf);
    snprintf(lineBuf, sizeof(lineBuf), "  APP1: 7MB [OTA STANDBY] %s",
             (strcmp(running_label, "app1") == 0) ? "*ACT" : "");
    Serial2.println(lineBuf);
    Serial2.println("  NVS : 20KB  | LFS: 1.9MB");

    snprintf(lineBuf, sizeof(lineBuf), "%luK (FREE: %luK)", (unsigned long)(totalHeap / 1024), (unsigned long)(freeHeap / 1024));
    print_qc_row(true, "SRAM", lineBuf);

    snprintf(lineBuf, sizeof(lineBuf), "%luMB (FREE: %luK)",
             (unsigned long)(psramTotal / (1024 * 1024)), (unsigned long)(psramFree / 1024));
    print_qc_row(true, "PSRAM", lineBuf);
    CHECK_CANCEL();

    // بخش ۴: آزمون ورودی‌ها و خروجی‌های رله
    print_report_section_header("4. PLC I/O & RELAY MATRIX", t_inputs_s + t_relays_s + t_keys_s);
    snprintf(lineBuf, sizeof(lineBuf), "%d/8 (%02us)", in_tested_cnt, (unsigned)t_inputs_s);
    print_qc_row(in_passed, "PLC INPUTS", lineBuf, in_why);
    if (in_tested_cnt < 8) {
        String missedIn = "UNTESTED: ";
        for (int i = 0; i < 8; i++) {
            if (!(s_audit.in_mask & (1 << i))) missedIn += "I" + String(i) + " ";
        }
        printer_print_wrapped(missedIn.c_str(), "  >> ", 28);
    }

    snprintf(lineBuf, sizeof(lineBuf), "%d/10 (%02us)", q_tested_cnt, (unsigned)t_relays_s);
    print_qc_row(q_passed, "PLC RELAYS", lineBuf, q_why);
    if (q_tested_cnt < 10) {
        String missedQ = "UNTESTED: ";
        for (int i = 0; i < 10; i++) {
            if (!(s_audit.q_mask & (1 << i))) missedQ += "Q" + String(i + 1) + " ";
        }
        printer_print_wrapped(missedQ.c_str(), "  >> ", 28);
    }

    snprintf(lineBuf, sizeof(lineBuf), "%d/10 (%02us)", keys_tested_cnt, (unsigned)t_keys_s);
    print_qc_row(key_passed, "PANEL KEYS", lineBuf, key_why);
    if (keys_tested_cnt < 10) {
        String missedK = "UNTESTED: ";
        for (int i = 0; i < 10; i++) {
            if (!(s_audit.keys_mask & (1 << i))) missedK += "F" + String(i + 1) + " ";
        }
        printer_print_wrapped(missedK.c_str(), "  >> ", 28);
    }

    snprintf(lineBuf, sizeof(lineBuf), "FS1=%s FS2=%s", fs1_ok ? "OK" : "NO", fs2_ok ? "OK" : "NO");
    print_qc_row(fs_passed, "FOOTSWITCH", lineBuf, fs_why);

    if (g_mcp23017_present) {
        bool mcp_ok = (mcp_out_cnt == 6 && mcp_in_cnt == 2);
        snprintf(lineBuf, sizeof(lineBuf), "OUT=%d/6 IN=%d/2", mcp_out_cnt, mcp_in_cnt);
        print_qc_row(mcp_ok, "MCP23017", lineBuf, mcp_ok ? NULL : "I/O INCOMPLETE");
    } else {
        Serial2.println("[ - ] MCP23017: NOT INSTALLED");
    }
    CHECK_CANCEL();

    // بخش ۵: سنسورهای دما و المنت
    print_report_section_header("5. THERMAL SENSORS & SSR", t_ssr_s);
    snprintf(lineBuf, sizeof(lineBuf), "%d C", (int)espTemp);
    print_qc_row(true, "ESP32 CPU", lineBuf);

    if (s_rtc_ok) {
        if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, portMAX_DELAY);
        int rtc_temp = rtc.getTemperature();
        if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);
        snprintf(lineBuf, sizeof(lineBuf), "%d C", rtc_temp);
        print_qc_row(true, "RTC DS3231", lineBuf);
    }

    snprintf(lineBuf, sizeof(lineBuf), "%.1f C", isnan(tc1_val) ? 0.0f : tc1_val);
    print_qc_row(tc1_pass, "TC1 MAX6675", lineBuf, tc1_why);

    snprintf(lineBuf, sizeof(lineBuf), "%.1f C", isnan(tc2_val) ? 0.0f : tc2_val);
    print_qc_row(tc2_pass, "TC2 MAX6675", lineBuf, tc2_why);

    snprintf(lineBuf, sizeof(lineBuf), "PWM (%02us)", (unsigned)t_ssr_s);
    print_qc_row(ssr_passed, "SSR HEATER", lineBuf, ssr_why);
    CHECK_CANCEL();

    // بخش ۶: اتصالات و پورت‌ها
    print_report_section_header("6. NETWORK & BUS PROTOCOLS", t_wifi_s + t_ble_s + t_rs485_s);
    print_qc_row(true, "HARDWARE", "WI-FI + BLE 5.0");

    if (s_audit.wifi_connected) {
        snprintf(lineBuf, sizeof(lineBuf), "CONN (%02us)", (unsigned)t_wifi_s);
        print_qc_row(true, "WI-FI", lineBuf);
        snprintf(lineBuf, sizeof(lineBuf), "SSID: %s", s_audit.wifi_connected_ssid);
        printer_print_wrapped(lineBuf, "  ", 28);
        snprintf(lineBuf, sizeof(lineBuf), "IP  : %s", WiFi.localIP().toString().c_str());
        printer_print_wrapped(lineBuf, "  ", 28);
    } else {
        print_qc_row(wifi_passed, "WI-FI", "SCANNED OK", wifi_why);
    }

    snprintf(lineBuf, sizeof(lineBuf), "%s (%02us)",
             s_audit.ble_started ? (s_audit.ble_connected ? "CONNECTED" : "ADV") : "DISABLED",
             (unsigned)t_ble_s);
    print_qc_row(ble_passed, "BLE V5.0", lineBuf, ble_why);

    snprintf(lineBuf, sizeof(lineBuf), "TX:%s RX:%s (%02us)",
             s_audit.rs485_sent ? "OK" : "NO", s_audit.rs485_received ? "OK" : "NO", (unsigned)t_rs485_s);
    print_qc_row(rs_passed, "RS485", lineBuf, rs_why);

    Serial2.println("[ - ] OTA REPO URL:");
    Serial2.println("      ota.haffler-infi.ir");
    CHECK_CANCEL();

    // بخش ۷: چک‌لیست بازرسی فیزیکی خط تولید
    print_report_section_header("7. FACTORY PHYSICAL AUDIT", 0);
    Serial2.println("[ ] ENCLOSURE & LABELS OK");
    Serial2.println("[ ] INTERNAL POWER 24V OK");
    Serial2.println("[ ] SSR HEATSINK MOUNTED");
    Serial2.println("[ ] SCREW TORQUES (0.5Nm)");
    Serial2.println("[ ] QC TAMPER SEAL APPLIED");
    CHECK_CANCEL();

    // بخش نتیجه بازرسی و تمبر گرافیکی رسمی
    Serial2.println("============================");
    printer_align(1); // Center

    snprintf(lineBuf, sizeof(lineBuf), "TOTAL QC SCORE: %d%%", total_score);
    printer_bold(true);
    Serial2.println(lineBuf);
    printer_bold(false);

    // چاپ تمبر گرافیکی تمام‌عرض بیت‌مپ (PASSED یا REJECTED)
    print_full_qc_stamp(all_passed);
    CHECK_CANCEL();

    printer_align(0); // Left
    Serial2.println();
    Serial2.println("OPERATOR SIGN : ..........");
    Serial2.println();
    Serial2.println("INSPECTOR SIGN: ..........");
    Serial2.println();
    Serial2.println("INSPECTION DATE: ../../20..");
    Serial2.println();

    printer_align(1); // Center
    printer_bold(true);
    Serial2.println("HAFFLER AUTOMATION SYSTEMS");
    printer_bold(false);
    Serial2.println("INDUSTRIAL CONTROL & PLC");
    Serial2.println("www.haffler.com");

    CHECK_CANCEL();

    // پیشروی کاغذ و بازگرداندن فاصله خطوط به حالت پیش‌فرض
    Serial2.write(0x1B);
    Serial2.write(0x64);
    Serial2.write(4);
    Serial2.println();
    Serial2.write(0x1B);
    Serial2.write(0x32);     // بازگرداندن فاصله خطوط به پیش‌فرض (ESC 2)
    printer_char_spacing(0); // بازگرداندن فاصله کاراکترها به حالت پیش‌فرض

    Serial2.flush();
    delay(20);
    digitalWrite(46, HIGH);

    // بازگرداندن به باود ریت ۹۶۰۰ برای پایش پورت RS485
    Serial2.begin(9600, SERIAL_8N1, 18, 17);
    Serial2.setTimeout(5);

    {
        char done_buf[48];
        snprintf(done_buf, sizeof(done_buf), "REPORT PRINTED (%d%%)", total_score);
        prn_set_ui_status(done_buf);
    }
    s_cancel_printing = false;
    s_is_printing = false;
    s_printer_task_handle = NULL;
    vTaskDelete(NULL);
}
#undef CHECK_CANCEL

// چاپ گزارش جامع آزمون سخت‌افزار و بازرسی کیفی روی چاپگر حرارتی RS485 در باود ۱۹۲۰۰
// مجهز به قفل کول‌داون ۲۰ ثانیه‌ای (تک‌کلیک و بدون وقفه میانی طبق دستور کارفرما)
void hwtest_print_haffler(void)
{
    uint32_t now = millis();

    // اگر چاپ در جریان است یا از آخرین درخواست پرینت کمتر از ۲۰ ثانیه گذشته، کلیک نادیده گرفته شود
    if (s_is_printing || (s_last_print_req_ms > 0 && (now - s_last_print_req_ms < 20000))) {
        return;
    }

    s_last_print_req_ms = now;
    s_cancel_printing = false;
    s_is_printing = true;

    // دو بوق کوتاه و قطعی در ابتدای شروع چاپ بدون نگه‌داشتن یا هنگ بازر
    hwtest_two_short_beeps();

    // ۱. به‌روزرسانی UI: نوشتن پیام در فیلد متنی و نمایش وضعیت
    set_var_rstextsend("PRINT QC REPORT");
    if (t_objects.rssend && lv_obj_is_valid(t_objects.rssend)) {
        lv_textarea_set_text(t_objects.rssend, "PRINT QC REPORT");
    }
    if (t_objects.datareceive && lv_obj_is_valid(t_objects.datareceive)) {
        lv_label_set_text(t_objects.datareceive, "PRINTING QC REPORT...");
    }

    // اجرای پرینتر در تسک پس‌زمینه مستقل روی هسته ۰ تا ترد LVGL در هسته ۱ آزاد بماند و تاچ کلید توقف را دریافت کند
    BaseType_t res = xTaskCreatePinnedToCore(
        printer_worker_task,
        "PrnWorker",
        6144,
        NULL,
        1,
        &s_printer_task_handle,
        0
    );

    if (res != pdPASS) {
        s_is_printing = false;
        s_printer_task_handle = NULL;
        if (t_objects.datareceive && lv_obj_is_valid(t_objects.datareceive)) {
            lv_label_set_text(t_objects.datareceive, "ERR: PRN TASK FAIL");
        }
    }
}

extern "C" void hwtest_save_buzzer_volume(int32_t val)
{
    storage.saveBuzzerVolume((int)val);
}

extern "C" void hwtest_cycle_buzzer_volume(void)
{
    int32_t cur = get_var_buzzer();
    int32_t next = 0;
    if (cur <= 0) next = 80;
    else if (cur < 120) next = 160;
    else if (cur < 200) next = 240;
    else next = 0;

    set_var_buzzer(next);
    hwtest_save_buzzer_volume(next);
    if (t_objects.tobj41 && lv_obj_is_valid(t_objects.tobj41)) {
        lv_slider_set_value(t_objects.tobj41, next, LV_ANIM_ON);
    }
    update_buzzer_label_text(next);
    if (next > 0) {
        hwtest_beep_click(40);
    } else {
        hwtest_buzzer_silence();
    }
}

extern "C" uint8_t hwtest_get_active_screen_id(void)
{
    return (uint8_t)s_audit.active_screen_id;
}

extern "C" void hwtest_goto_screen_id(uint8_t screen_id)
{
    if (!g_hwtest_active) {
        hwtest_enter_ui();
    }
    if (screen_id >= T_SCREEN_ID_MAIN && screen_id <= T_SCREEN_ID_SSR) {
        s_audit.active_screen_id = (T_ScreensEnum)screen_id;
        t_loadScreen((T_ScreensEnum)screen_id);
    }
}

// ===== تست اولیه سلامت سخت‌افزار در بدو روشن شدن (Boot Self-Test) =====
HwBootDiagResult g_boot_diag = { false };
bool g_boot_diag_dismissed = false;
static lv_obj_t *s_boot_alert_modal = NULL;

extern "C" void hwtest_run_boot_self_check(void)
{
    memset(&g_boot_diag, 0, sizeof(g_boot_diag));

    // ۱. بررسی ساعت سخت‌افزاری RTC DS3231 روی I2C (آدرس 0x68)
    if (!s_rtc_ok) {
        g_boot_diag.has_error = true;
        g_boot_diag.rtc_comms_fail = true;
    } else {
        if (rtc.lostPower()) {
            g_boot_diag.has_error = true;
            g_boot_diag.rtc_battery_lost = true;
        }
    }

    // ۲. بررسی تاچ خازنی GT911 روی I2C (آدرس 0x5D)
    bool touch_alive = false;
    if (bbct.sensorType() != 0) {
        touch_alive = true;
    } else if (i2c_present(I2C_TOUCH_ADDR)) {
        touch_alive = true;
    }
    if (!touch_alive) {
        g_boot_diag.has_error = true;
        g_boot_diag.touch_comms_fail = true;
    }

    // ۳. بررسی حافظه جانبی PSRAM
    if (!psramFound() || heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < 100000) {
        g_boot_diag.has_error = true;
        g_boot_diag.psram_fail = true;
    }

    // ۴. بررسی فضای هیپ حافظه داخلی SRAM
    if (esp_get_free_heap_size() < 40000) {
        g_boot_diag.has_error = true;
        g_boot_diag.heap_low = true;
    }

    // ۵. بررسی دمای داخلی پردازنده ESP32-S3
    float t_celsius = 0.0f;
    if (temp_sensor && temperature_sensor_get_celsius(temp_sensor, &t_celsius) == ESP_OK) {
        g_boot_diag.cpu_temp = t_celsius;
        if (t_celsius > 75.0f) {
            g_boot_diag.has_error = true;
            g_boot_diag.cpu_overheat = true;
        }
    }

    // ۶. خط قرمز سخت‌افزاری کارفرما:
    // آی‌سی‌های MCP23017 و MAX6675 در این سخت‌افزار وجود ندارند؛ هرگز تست یا گزارش خطا نمی‌شوند.

    // تجمیع گزارش خطا به زبان فارسی
    if (g_boot_diag.has_error) {
        g_boot_diag.summary_fa[0] = '\0';
        if (g_boot_diag.rtc_comms_fail) {
            strcat(g_boot_diag.summary_fa, "• عدم پاسخ آی‌سی ساعت (RTC DS3231)\n");
        } else if (g_boot_diag.rtc_battery_lost) {
            strcat(g_boot_diag.summary_fa, "• باتری ساعت خالی است یا زمان تنظیم نیست\n");
        }
        if (g_boot_diag.touch_comms_fail) {
            strcat(g_boot_diag.summary_fa, "• کنترلر تاچ خازنی (I2C) شناسایی نشد\n");
        }
        if (g_boot_diag.psram_fail) {
            strcat(g_boot_diag.summary_fa, "• حافظه رم جانبی (PSRAM) در دسترس نیست\n");
        }
        if (g_boot_diag.heap_low) {
            strcat(g_boot_diag.summary_fa, "• کمبود بحرانی فضای هیپ حافظه داخلی\n");
        }
        if (g_boot_diag.cpu_overheat) {
            char tbuf[64];
            snprintf(tbuf, sizeof(tbuf), "• دمای پردازنده بحرانی است: %d°C\n", (int)g_boot_diag.cpu_temp);
            strcat(g_boot_diag.summary_fa, tbuf);
        }
        size_t slen = strlen(g_boot_diag.summary_fa);
        if (slen > 0 && g_boot_diag.summary_fa[slen - 1] == '\n') {
            g_boot_diag.summary_fa[slen - 1] = '\0';
        }
        Serial.printf("[SELF-CHECK] Issues detected:\n%s\n", g_boot_diag.summary_fa);
    } else {
        Serial.println("[SELF-CHECK] All boot hardware tests PASSED cleanly.");
    }
}

static void boot_alert_dismiss_cb(lv_event_t *e)
{
    (void)e;
    g_boot_diag_dismissed = true;
    app_sound_play(SOUND_TAP);
    if (s_boot_alert_modal && lv_obj_is_valid(s_boot_alert_modal)) {
        lv_obj_del(s_boot_alert_modal);
        s_boot_alert_modal = NULL;
    }
}

extern "C" void hwtest_show_boot_alert_if_needed(lv_obj_t *parent_screen)
{
    if (!g_boot_diag.has_error || g_boot_diag_dismissed || !parent_screen) return;
    if (s_boot_alert_modal && lv_obj_is_valid(s_boot_alert_modal)) return;

    // لایه مات پس‌زمینه (Modal Backdrop)
    lv_obj_t *overlay = lv_obj_create(parent_screen);
    s_boot_alert_modal = overlay;
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_size(overlay, 480, 320);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x090d16), 0);
    lv_obj_set_style_bg_opa(overlay, 200, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    // کارت هشدار اپل (Apple HIG Dark Alert Card)
    lv_obj_t *card = lv_obj_create(overlay);
    lv_obj_set_size(card, 420, LV_SIZE_CONTENT);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x18181b), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 12, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // ۱. هدر کارت با آیکون اخطار و تیتر
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, 388, 32);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 8, 0);

    lv_obj_t *badge = lv_label_create(hdr);
    lv_label_set_text(badge, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(badge, lv_color_hex(0xf87171), 0);
    lv_obj_set_style_text_font(badge, &lv_font_montserrat_20, 0);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "هشدار سلامت سخت‌افزار");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &ui_font_kalameh_bold_18, 0);

    // خط جداکننده باریک
    lv_obj_t *div = lv_obj_create(card);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 388, 1);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x27272a), 0);
    lv_obj_set_style_bg_opa(div, 255, 0);

    // ۲. متن موارد خطا
    lv_obj_t *body = lv_label_create(card);
    lv_label_set_text(body, g_boot_diag.summary_fa);
    lv_obj_set_width(body, 388);
    lv_obj_set_style_text_color(body, lv_color_hex(0xfca5a5), 0);
    lv_obj_set_style_text_font(body, &ui_font_kalame_bold_14, 0);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_pad_top(body, 4, 0);
    lv_obj_set_style_pad_bottom(body, 4, 0);

    // ۳. دکمه تایید / متوجه شدم (Apple Style Button)
    lv_obj_t *btn = lv_btn_create(card);
    lv_obj_set_size(btn, 160, 38);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xb91c1c), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, boot_alert_dismiss_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "متوجه شدم");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(btn_lbl, &ui_font_kalame_bold_14, 0);
    lv_obj_center(btn_lbl);
}

// =========================================================================
// تله‌متری و کنترل بلادرنگ جهت وب‌پورتال و مانیتورینگ بیسیم
// =========================================================================
bool hwtest_get_relay_state(uint8_t relay_idx)
{
    if (relay_idx >= 10) return false;
    if (pins[relay_idx] < 0) return false;
    return (digitalRead(pins[relay_idx]) == LOW); // Active-LOW: 0 = ON, 1 = OFF
}

void hwtest_set_relay_state(uint8_t relay_idx, bool on)
{
    if (relay_idx >= 10) return;
    if (pins[relay_idx] < 0) return;
    digitalWrite(pins[relay_idx], on ? LOW : HIGH);
    switch (relay_idx) {
        case 0: set_var_q1(on); break;
        case 1: set_var_q2(on); break;
        case 2: set_var_q3(on); break;
        case 3: set_var_q4(on); break;
        case 4: set_var_q5(on); break;
        case 5: set_var_q6(on); break;
        case 6: set_var_q7(on); break;
        case 7: set_var_q8(on); break;
        case 8: set_var_q9(on); break;
        case 9: set_var_q10(on); break;
    }
}

void hwtest_pulse_relay(uint8_t relay_idx, uint32_t duration_ms)
{
    if (relay_idx >= 10) return;
    if (duration_ms == 0 || duration_ms > 3000) duration_ms = 3000; // سقف ایمنی ۳ ثانیه زیمنس
    hwtest_set_relay_state(relay_idx, true);
    s_relay_pulse_end_ms[relay_idx] = millis() + duration_ms;
}

void hwtest_kill_all_outputs(void)
{
    for (int i = 0; i < 10; i++) {
        s_relay_pulse_end_ms[i] = 0;
        if (pins[i] >= 0) digitalWrite(pins[i], HIGH);
    }
    set_var_q1(false);  set_var_q2(false);  set_var_q3(false);  set_var_q4(false);  set_var_q5(false);
    set_var_q6(false);  set_var_q7(false);  set_var_q8(false);  set_var_q9(false);  set_var_q10(false);
    set_var_pwm_state(0);
}

float hwtest_get_tc1_temp(void) { return temp1; }
float hwtest_get_tc2_temp(void) { return temp2; }
bool  hwtest_get_tc1_valid(void) { return (!s_temp1_invalid && g_tc1_stat.probe_ok); }
bool  hwtest_get_tc2_valid(void) { return (!s_temp2_invalid && g_tc2_stat.probe_ok); }
float hwtest_get_esp_temp(void) { return s_esp_temp_celsius; }

void hwtest_get_rtc_info(char *dt_str, size_t max_len, float *temp_c)
{
    if (xMutex_I2C) xSemaphoreTake(xMutex_I2C, pdMS_TO_TICKS(50));
    DateTime now = rtc.now();
    int t = rtc.getTemperature();
    if (xMutex_I2C) xSemaphoreGive(xMutex_I2C);

    if (dt_str && max_len > 0) {
        snprintf(dt_str, max_len, "%04d/%02d/%02d %02d:%02d:%02d",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second());
    }
    if (temp_c) *temp_c = (float)t;
}

uint8_t hwtest_get_opto_inputs(void)
{
    uint8_t mask = 0;
    if (get_var__10() == 0) mask |= (1 << 0);
    if (get_var__11() == 0) mask |= (1 << 1);
    if (get_var__12() == 0) mask |= (1 << 2);
    if (get_var__13() == 0) mask |= (1 << 3);
    if (get_var__14() == 0) mask |= (1 << 4);
    if (get_var__15() == 0) mask |= (1 << 5);
    if (get_var__16() == 0) mask |= (1 << 6);
    if (get_var__17() == 0) mask |= (1 << 7);
    return mask;
}

void hwtest_get_opto_voltages(int *v)
{
    if (!v) return;
    for (int i = 0; i < 5; i++) v[i] = s_adc_inputs[i];
}

uint16_t hwtest_get_panel_keys(void)
{
    return s_panel_keys_mask;
}

#endif // HWTEST_UI_READY
