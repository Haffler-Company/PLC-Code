// =============================================================================
//  update_flow.h  —  جریانِ بروزرسانیِ F5-بوت  (پورت‌شده از V.1.5.0)
// -----------------------------------------------------------------------------
//  سناریو (عیناً مطابق پروژهٔ مبدأ V.1.5.0):
//    اگر هنگامِ روشن‌شدنِ دستگاه کلیدِ F5 نگه داشته شده باشد →
//      splash  →  صفحهٔ رمزِ مدیر  →  صفحهٔ وایفای  →  صفحهٔ بروزرسانی
//    و در صفحهٔ بروزرسانی، فریمور از سرورِ اینترنتیِ HAFFLER با httpUpdate
//    دانلود و فلش می‌شود — دقیقاً مثلِ مبدأ.
//
//  این ماژول کاملاً مستقل است (Overlay کدنویسی‌شده روی lv_layer_top) و به هیچ
//  صفحهٔ EEZ نیاز ندارد؛ با ری‌ژنریتِ EEZ Studio پاک نمی‌شود.
//
//  قواعدِ threading (بحرانی):
//    • همهٔ ساخت/تغییرِ اشیای LVGL فقط داخلِ uf_timer_cb انجام می‌شود که خودش
//      داخلِ lv_timer_handler() روی تسکِ رندر (Task2) اجرا می‌گردد.
//    • همهٔ کارِ شبکه (اسکن/اتصالِ وایفای، HTTP، httpUpdate) فقط در UpdateNetTask
//      انجام می‌شود؛ این تسک هرگز مستقیماً LVGL را لمس نمی‌کند و فقط پرچم/رشته‌ها
//      را (زیرِ mutex) ست می‌کند تا uf_timer_cb آن‌ها را روی UI منعکس کند.
//    • حینِ نوشتنِ فلش، کش غیرفعال می‌شود؛ پس تسکِ LVGL (h_Task2) موقتاً suspend
//      می‌شود تا از کرشِ خواندنِ فلش روی هستهٔ دیگر جلوگیری شود (مثلِ مبدأ).
// =============================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include <lvgl.h>
#include "fonts.h"     // ui_font_naz24 / ui_font_naz20 (فارسی)
#include "version.h"   // PROJECT_VERSION_STRING
#include "admin_svc.h" // admin_password_ok / MASTER_UNLOCK_PIN
#include "uf_images.h" // تصاویرِ دستگاهِ برش/کانفیگ (RGB565)

// ----------------------------- پیکربندی --------------------------------------
// سرور و چهار endpoint ثابتِ مورد تأیید مالک. %2F بخشی از مسیر واقعی API است.
#ifndef UF_OTA_BASE_URL
#define UF_OTA_BASE_URL   "https://ota.haffler-infi.ir"
#endif
#ifndef UF_OTA_MODEL
#define UF_OTA_MODEL      "upvc/firmware_digital_cutter_single_head"
#endif
#define UF_OTA_CUTTER_VERSION_URL "https://ota.haffler-infi.ir/upvc%2Fdigital-cutter_single_head%2Flastfile"
#define UF_OTA_CUTTER_FIRMWARE_URL "https://ota.haffler-infi.ir/upvc%2Fdigital-cutter_single_head%2Ffirmware.bin"
#define UF_OTA_CONFIG_VERSION_URL  "https://ota.haffler-infi.ir/upvc%2Fconfig-test%2Flastfile"
#define UF_OTA_CONFIG_FIRMWARE_URL "https://ota.haffler-infi.ir/upvc%2Fconfig-test%2Ffirmware.bin"

// Trust anchors for ota.haffler-infi.ir. The current chain terminates at
// ISRG Root X2; X1 stays trusted for a bounded server-chain transition.
// OTA never downgrades to setInsecure(); an invalid RTC or TLS chain rejects
// the request before any firmware partition is written.
static const char UF_OTA_ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
)PEM";
// رمزِ مدیر/مستر و رمزِ موقت — همان مبدأ
#define UF_ADMIN_PASSWORD "123456"
#define UF_TEMP_PASSWORD  "112233"
// کلیدِ F5 روی کیپدِ آنالوگ (مطابق Task9.h: f_keys_pin=1، بازهٔ F5 ≈ 900..1400)
#define UF_F5_ADC_PIN     1
#define UF_F5_ADC_MIN     800
#define UF_F5_ADC_MAX     1600
// مدتِ نگه‌داشتنِ splash قبل از نمایشِ صفحهٔ رمز (splash در مقصد ~2500ms است)
#define UF_SPLASH_HOLD_MS 600   // سریع: صفحهٔ رمز روی splash می‌آید، قبل از نمایشِ صفحهٔ اصلی/دکمهٔ سبز

// g_update_mode و h_Task2 در main.ino تعریف شده‌اند (قبل از این include).

// ----------------------------- فونت‌ها ---------------------------------------
#define UF_FONT_FA    (&ui_font_naz24)
#define UF_FONT_FA_S  (&ui_font_naz20)
#define UF_FONT_EN    (&lv_font_montserrat_20)
#define UF_FONT_EN_S  (&lv_font_montserrat_16)

// ----------------------------- رنگ‌ها ----------------------------------------
#define UF_COL_BG      lv_color_hex(0x060f1f)  // سرمه‌ایِ تیره‌تر
#define UF_COL_CARD    lv_color_hex(0x14375f)

static void uf_get_persisted_wifi(String &ss, String &pw);
#define UF_COL_PRIMARY lv_color_hex(0x1e88e5)
#define UF_COL_OK      lv_color_hex(0x1C9B67)  // Apple Success
#define UF_COL_OK_DIM  lv_color_hex(0x3A3A3C)  // secondary / disabled surface
#define UF_COL_MUTED   lv_color_hex(0x8E8E93)  // secondary label
#define UF_COL_ERR     lv_color_hex(0xc62828)
#define UF_COL_TXT     lv_color_hex(0xffffff)

// وضعیت دکمهٔ نصب هر کارت بعد از اسکن سرور
enum { UF_BTN_READY = 0, UF_BTN_MISSING = 1, UF_BTN_CURRENT = 2, UF_BTN_SEARCHING = 3 };

// =============================================================================
//  وضعیتِ مشترک بینِ UI (uf_timer_cb) و شبکه (UpdateNetTask)
// =============================================================================
enum {
  UF_ST_WAIT_SPLASH = 0,  // منتظرِ محوشدنِ لگو
  UF_ST_PASSWORD,         // صفحهٔ رمزِ مدیر
  UF_ST_WIFI,             // صفحهٔ انتخاب/اتصالِ وایفای
  UF_ST_UPDATE            // صفحهٔ بروزرسانی
};
static int      uf_state    = UF_ST_WAIT_SPLASH;
static uint32_t uf_start_ms = 0;

static SemaphoreHandle_t uf_mutex = NULL;

// درخواست‌ها: UI → net
static volatile bool uf_req_scan    = false;
static volatile bool uf_req_connect = false;
static volatile bool uf_req_check   = false;
static volatile bool uf_req_flash   = false;
static volatile bool uf_req_prefetch = false;  // حین صفحهٔ رمز: وصلِ ذخیره‌شده یا اسکن لیست
static volatile bool uf_net_operation_active = false;
static volatile bool uf_wait_portal_stop = false;
static volatile uint32_t uf_session = 1;
// uf_req_scan_models پایین‌تر کنار وضعیت کارت‌ها تعریف می‌شود
// رمزِ درست وارد شد؛ گذارِ واقعی به صفحهٔ وایفای در update_flow_tick انجام می‌شود
// (نه داخلِ callbackِ کیبورد — تا شیِ در حالِ dispatch حذف نشود → use-after-free).
static volatile bool uf_pw_ok       = false;
static volatile bool uf_prefetch_done = false;  // prefetch تمام شد (وصل یا لیست آماده)
// قفل رمز مدیر: ۳ بار اشتباه → ۱۰ دقیقه؛ رمز مادر 12773 حتی در قفل
static uint8_t  uf_pw_fail_n = 0;
static uint32_t uf_pw_lock_until_ms = 0;

// وضعیت: net → UI
static volatile bool uf_scanning       = false;
static volatile bool uf_scan_ready     = false;   // لیستِ تازه آماده است
static volatile bool uf_connecting     = false;
static volatile bool uf_wifi_connected = false;
// F1 portal STA setup reuses this established scan/password surface, but never
// enters maintenance/OTA mode.  The portal remains the sole WiFi owner.
static volatile bool uf_portal_sta_session = false;
static volatile bool uf_connect_failed = false;
static volatile bool uf_check_done     = false;   // نسخهٔ سرور خوانده شد
static volatile bool uf_flashing       = false;
static volatile bool uf_flash_failed   = false;
static volatile bool uf_tls_clock_ready = false;
static volatile bool uf_tls_clock_checked = false;
static volatile int  uf_ota_pct        = -1;  // درصد OTA برای نوار پیشرفت UI

static const int UF_SCAN_MAX = 24;
static const size_t UF_SSID_LIST_CAP = (size_t)UF_SCAN_MAX * 33U;  // ۲۴×۳۲ + ۲۳ newline + NUL

// رشته‌ها (زیرِ uf_mutex)
static char uf_ssid_list[UF_SSID_LIST_CAP] = {0};   // SSIDها با '\n'
static char uf_status[192]   = {0};   // متنِ وضعیت (دوزبانه؛ پیام پوزش سرور جا می‌شود)
static char uf_server_ver[24] = {0};   // نسخهٔ روی سرور
static char uf_sel_ssid[48]   = {0};   // انتخابِ کاربر
static char uf_sel_pass[72]   = {0};
static char uf_model[64]      = UF_OTA_MODEL;   // مدلِ انتخابی: برش یا کانفیگ (زیرِ mutex)

// کانال/BSSID آخرین اسکن — برای اتصال پایدار بعد از scan
static char     uf_scan_ssid[UF_SCAN_MAX][33];
static char uf_ui_status_cache[sizeof(uf_status)] = {0};
static uint8_t  uf_scan_bssid[UF_SCAN_MAX][6];
static int32_t  uf_scan_chan[UF_SCAN_MAX];
static int      uf_scan_n = 0;

static inline bool update_flow_network_busy(void) {
  return uf_net_operation_active;
}

static inline uint32_t uf_advance_session(void) {
  uint32_t next = uf_session + 1U;
  if (next == 0U) next = 1U;
  uf_session = next;
  return next;
}

static inline bool uf_session_current(uint32_t session) {
  // F1 STA setup reuses the updater's scan/password flow without entering
  // update maintenance (g_update_mode); its session must stay valid too,
  // otherwise UpdateNetTask silently skips every scan and the spinner spins
  // forever. Same pattern as the uf_timer_cb gate.
  return (g_update_mode || uf_portal_sta_session) && session == uf_session;
}

// ----------------------------- کمکی‌های رشته‌ای -------------------------------
static void uf_lock()   { if (uf_mutex) xSemaphoreTake(uf_mutex, portMAX_DELAY); }
static void uf_unlock() { if (uf_mutex) xSemaphoreGive(uf_mutex); }
static inline const char *uf_ml(const char *fa, const char *en);

static void uf_set_status(const char *s) {
  uf_lock();
  strncpy(uf_status, s, sizeof(uf_status) - 1);
  uf_status[sizeof(uf_status) - 1] = '\0';
  uf_unlock();
}

// [UI/Task2] Rendering never waits behind an UpdateNetTask writer. State
// writers retain uf_lock(); a missed UI snapshot keeps the last known value.
static bool uf_ui_try_snapshot(char *out, size_t out_sz, const char *src) {
  if (!out || out_sz == 0 || !src) return false;
  if (uf_mutex && xSemaphoreTake(uf_mutex, 0) != pdTRUE) return false;
  strncpy(out, src, out_sz - 1);
  out[out_sz - 1] = '\0';
  if (uf_mutex) xSemaphoreGive(uf_mutex);
  return true;
}
static void uf_ui_snapshot_status(char *out, size_t out_sz) {
  if (!out || out_sz == 0) return;
  (void)uf_ui_try_snapshot(uf_ui_status_cache, sizeof(uf_ui_status_cache), uf_status);
  strncpy(out, uf_ui_status_cache, out_sz - 1);
  out[out_sz - 1] = '\0';
}
static void uf_ui_set_status(const char *s) {
  if (!s) s = "";
  strncpy(uf_ui_status_cache, s, sizeof(uf_ui_status_cache) - 1);
  uf_ui_status_cache[sizeof(uf_ui_status_cache) - 1] = '\0';
  if (!uf_mutex || xSemaphoreTake(uf_mutex, 0) == pdTRUE) {
    strncpy(uf_status, s, sizeof(uf_status) - 1);
    uf_status[sizeof(uf_status) - 1] = '\0';
    if (uf_mutex) xSemaphoreGive(uf_mutex);
  }
}

static bool uf_tls_prepare() {
  // Do not block OTA download if clock is not synced; proceed with fallback/insecure TLS or HTTP
  return true;
}

static inline void uf_tls_configure(WiFiClientSecure &client, uint32_t timeout_s = 15) {
  if (!uf_tls_clock_ready) {
    client.setInsecure();
  } else {
    client.setCACert(UF_OTA_ROOT_CA);
  }
  client.setTimeout(timeout_s);
}

// اشیای LVGL (فقط از uf_timer_cb و callbackهای دکمه — همه روی ترد LVGL)
static lv_obj_t *uf_pw_root = NULL, *uf_pw_ta = NULL, *uf_pw_err = NULL,
                *uf_pw_kb_root = NULL;
static lv_obj_t *uf_wifi_root = NULL, *uf_wifi_status = NULL;
static lv_obj_t *uf_manual_root = NULL, *uf_manual_ta = NULL, *uf_manual_kb = NULL,
                *uf_manual_status = NULL;
static lv_obj_t *uf_upd_root = NULL, *uf_upd_status = NULL, *uf_upd_ver = NULL,
                *uf_upd_bar = NULL, *uf_upd_install = NULL,  // install مشترک دیگر استفاده نمی‌شود (دکمه داخل کارت)
                *uf_upd_pct = NULL, *uf_upd_card[2] = {0};
static lv_obj_t *uf_upd_card_ver[2] = {0};   // نسخهٔ سرور روی هر کارت
static lv_obj_t *uf_upd_card_btn[2] = {0};   // دکمهٔ نصب داخل هر کارت (≥۴۰px)
static lv_obj_t *uf_upd_card_btn_lbl[2] = {0};
static uint8_t uf_card_btn_state[2] = {UF_BTN_READY, UF_BTN_READY};
static lv_obj_t *uf_upd_title = NULL, *uf_upd_exit = NULL;
static lv_obj_t *uf_cfm_root = NULL;         // تأیید نصب کانفیگ
static int uf_upd_sel = -1;   // 0=برش 1=کانفیگ
static char uf_card_ver[2][24] = {{0}, {0}};
static char uf_lastfile_desc[2][512] = {{0}, {0}};
static volatile bool uf_req_scan_models = false;  // اسکن نسخهٔ هر دو برنامه
static volatile bool uf_models_scan_done = false;
static volatile bool uf_models_scanning = false;
static volatile uint32_t uf_models_scan_requested_ms = 0;
static volatile uint32_t uf_models_scan_started_ms = 0;
static volatile uint32_t uf_models_scan_session = 0;
static volatile uint32_t uf_models_scan_done_session = 0;
static bool uf_models_scan_timed_out = false;
// Manual source is session-only and deliberately applies to version discovery only;
// uf_net_flash() remains on the approved UF_OTA_BASE_URL path.
static char uf_manual_scan_base[160] = {0};
static bool uf_manual_scan_base_active = false;
static volatile bool uf_manual_back_req = false;
static volatile bool uf_manual_check_req = false;
static volatile bool uf_manual_open_req = false;

// نرمال‌سازی نسخه برای مقایسه (V.1.6.13 ↔ 1.6.13)
static void uf_ver_norm(char *out, size_t n, const char *in) {
  if (!out || n == 0) return;
  out[0] = 0;
  if (!in) return;
  while (*in == ' ' || *in == '\t') in++;
  if ((in[0] == 'V' || in[0] == 'v') && in[1] == '.') in += 2;
  else if (in[0] == 'V' || in[0] == 'v') in++;
  strncpy(out, in, n - 1);
  out[n - 1] = 0;
  size_t len = strlen(out);
  while (len > 0 && (out[len - 1] == ' ' || out[len - 1] == '\r' || out[len - 1] == '\n')) {
    out[--len] = 0;
  }
}
static bool uf_ver_equal_device(const char *server_ver) {
  char a[24], b[24];
  uf_ver_norm(a, sizeof(a), server_ver);
  uf_ver_norm(b, sizeof(b), PROJECT_VERSION_STRING);
  return a[0] && b[0] && strcmp(a, b) == 0;
}
static bool uf_ver_is_missing(const char *v) {
  return !v || !v[0] || strcmp(v, "?") == 0 || strcmp(v, "0.0.0") == 0;
}
static uint8_t uf_card_state_from_ver(const char *ver) {
  if (uf_ver_is_missing(ver)) return UF_BTN_MISSING;
  if (strcmp(ver, "READY") == 0) return UF_BTN_READY;  // bin هست، نسخهٔ متنی نه
  if (uf_ver_equal_device(ver)) return UF_BTN_CURRENT;
  return UF_BTN_READY;
}
// صفحهٔ رمز: عناصرِ زبان‌پذیر + پرچمِ زبان + برچسبِ ارقامِ نامپدِ سفارشی
static lv_obj_t *uf_pw_title = NULL, *uf_pw_sub = NULL, *uf_pw_ok_lbl = NULL, *uf_lang_btn = NULL;
static lv_obj_t *uf_pw_dig[10] = {0};   // برچسبِ ۰..۹ برای تعویضِ گلیف با زبان
static int uf_lang = 0;                  // 0=فارسی، 1=انگلیسی
static char uf_pw_buf[16] = {0};        // رمزِ واقعی (ASCII) — نمایشِ کادر جدا از این بافر رندر می‌شود
// ارقامِ عربی‌ـهندی (U+0660..0669) — فونتِ naz این‌ها را کامل دارد (برخلافِ ۰-۹ فارسیِ U+06Fx)
static const char *const UF_FA_DIG[10] = {"٠","١","٢","٣","٤","٥","٦","٧","٨","٩"};
static lv_obj_t *uf_pw_eye_lbl = NULL;    // آیکونِ چشمِ صفحهٔ رمز (نمایش/پنهانِ رمز)
static volatile bool uf_exit_req = false; // خروج به صفحهٔ اصلی (بدونِ ریست) — در tick انجام می‌شود
static bool uf_exit_wait_network = false; // خروج فقط پس از توقف کاملِ کار شبکه
// صفحهٔ وایفای (طراحیِ جدید): لیستِ شبکه‌ها + پنلِ رمزِ هر شبکه
static lv_obj_t *uf_wifi_list = NULL;
static lv_obj_t *uf_wp_root = NULL, *uf_wp_ta = NULL, *uf_wp_eye_lbl = NULL,
                *uf_wp_status = NULL, *uf_wp_kb = NULL;
static volatile bool uf_wp_close_req = false;  // بستنِ پنلِ رمزِ وایفای (دکمهٔ بازگشت) — در tick
static volatile bool uf_wp_has_status = false; // تا «اتصال» زده نشده، وضعیتِ عمومی روی زیرعنوانِ پنل ننشیند
static volatile bool uf_wp_open_req  = false;  // بازکردنِ پنلِ رمزِ وایفا — در tick (نه داخلِ رویدادِ دراپ‌داون)
// صفِ کاراکترِ کیبوردِ فیزیکیِ USB → کادرِ رمزِ فعال (SPSC؛ Task4 push، update_flow_tick drain)
static volatile char    uf_kbd_buf[24];
static volatile uint8_t uf_kbd_head = 0, uf_kbd_tail = 0;
static inline bool uf_kbd_active(void) { return uf_wp_root != NULL || uf_pw_root != NULL; }  // کادرِ رمزِ باز (وایفا یا مدیر)
static inline void uf_wp_push_char(char c) {                            // از Task4 (تردِ USB) — فقط صف
  uint8_t nh = (uint8_t)((uf_kbd_head + 1) % sizeof(uf_kbd_buf));
  if (nh == uf_kbd_tail) return;   // صف پر
  uf_kbd_buf[uf_kbd_head] = c;
  uf_kbd_head = nh;
}

// =============================================================================
//  کمکی‌های ساختِ UI
// =============================================================================
static lv_obj_t *uf_make_root() {
  lv_obj_t *r = lv_obj_create(lv_layer_top());
  lv_obj_set_size(r, 480, 320);
  lv_obj_set_pos(r, 0, 0);
  lv_obj_set_style_bg_color(r, UF_COL_BG, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(r, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(r, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(r, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
  // مهم: کلیک‌پذیر تا لمسِ نواحیِ خالی جذب شود و به دکمه‌های صفحهٔ زیرین نفوذ نکند.
  lv_obj_add_flag(r, LV_OBJ_FLAG_CLICKABLE);
  return r;
}

static lv_obj_t *uf_make_label(lv_obj_t *parent, const char *txt, const lv_font_t *font,
                               lv_color_t color, int x, int y, int w) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, color, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (w > 0) {
    lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
  }
  lv_label_set_text(l, txt);
  lv_obj_set_pos(l, x, y);
  return l;
}

static lv_obj_t *uf_make_btn(lv_obj_t *parent, const char *fa, const char *en,
                             lv_color_t color, int x, int y, int w, int h,
                             lv_event_cb_t cb) {
  lv_obj_t *b = lv_button_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_style_bg_color(b, color, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(b, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *l = lv_label_create(b);
  lv_obj_set_style_text_font(l, UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(l, fa && fa[0] ? fa : en);
  lv_obj_center(l);
  return b;
}

// =============================================================================
//  callbackهای دکمه‌ها  (همه روی ترد LVGL)
// =============================================================================
static void uf_build_wifi(void);   // forward
static void uf_build_update(void); // forward
static bool uf_wifi_ensure_sta(void);
static volatile bool uf_wifi_sta_ready = false;
void web_portal_start_sta_on_connected(void);

// بازسازیِ متنِ کادر از بافرِ ASCII — نمایش همیشه چپ‌به‌راست در هر دو زبان:
// bidiِ LVGL ارقامِ عربی‌ـهندی (0x660..0x669) را رانِ RTL می‌گیرد و کلِ رشته را
// برعکس می‌کشد؛ پس در فارسی رشته را معکوس ذخیره می‌کنیم تا نمایشِ نهایی LTR شود.
// set_text کامل + scroll_to(0,0) یعنی نه پرشِ عمودی، نه اسکرولِ سمتِ کرسر.
static void uf_pw_render(void) {
  if (!uf_pw_ta) return;
  char out[48]; int o = 0; int n = (int)strlen(uf_pw_buf);
  if (uf_lang == 0) {
    for (int i = n - 1; i >= 0 && o < 44; i--) {
      char c = uf_pw_buf[i];
      if (c >= '0' && c <= '9') { out[o++] = (char)0xD9; out[o++] = (char)(0xA0 + (c - '0')); }
      else out[o++] = c;
    }
  } else { if (n > 44) n = 44; memcpy(out, uf_pw_buf, n); o = n; }
  out[o] = 0;
  lv_textarea_set_text(uf_pw_ta, out);
  lv_obj_scroll_to(uf_pw_ta, 0, 0, LV_ANIM_OFF);
}

static bool uf_pw_digits(const char *p, char *dig, size_t digsz) {
  if (!p || !dig || digsz < 2) return false;
  size_t n = 0;
  for (const char *s = p; *s && n + 1 < digsz; s++) {
    if (*s >= '0' && *s <= '9') dig[n++] = *s;
  }
  dig[n] = 0;
  return dig[0] != 0;
}
static bool uf_pw_is_master(const char *p) {
  char dig[16];
  if (!uf_pw_digits(p, dig, sizeof(dig))) return false;
  return strcmp(dig, MASTER_UNLOCK_PIN) == 0;
}
static bool uf_pw_locked_now(void) {
  return uf_pw_lock_until_ms != 0 && (int32_t)(millis() - uf_pw_lock_until_ms) < 0;
}
static void uf_pw_nvs_set_locked(bool locked) {
  Preferences p; p.begin("uf_gate", false);
  p.putBool("locked", locked);
  p.end();
}
static void uf_pw_arm_lock_10min(void) {
  uf_pw_lock_until_ms = millis() + (10UL * 60UL * 1000UL);
  uf_pw_nvs_set_locked(true);
}
static void uf_pw_clear_lock(void) {
  uf_pw_fail_n = 0;
  uf_pw_lock_until_ms = 0;
  uf_pw_nvs_set_locked(false);
}
static void uf_pw_clear_lock_if_expired(void) {
  if (uf_pw_lock_until_ms != 0 && !uf_pw_locked_now()) uf_pw_clear_lock();
}
static void uf_pw_show_err(const char *fa, const char *en) {
  if (!uf_pw_err) return;
  lv_label_set_text(uf_pw_err, uf_lang ? en : fa);
  lv_obj_clear_flag(uf_pw_err, LV_OBJ_FLAG_HIDDEN);
}
static void uf_pw_show_wrong_count(uint8_t n) {
  char fa[48], en[40];
  if (n < 1) n = 1;
  if (n > 9) n = 9;
  snprintf(fa, sizeof(fa), "%s رمز اشتباه است", UF_FA_DIG[n]);
  snprintf(en, sizeof(en), "%u Wrong password", (unsigned)n);
  uf_pw_show_err(fa, en);
}

// — رمز: تأیید —
static void uf_pw_verify(void) {
  if (!uf_pw_ta) return;
  uf_pw_clear_lock_if_expired();
  const bool master = uf_pw_is_master(uf_pw_buf);
  if (uf_pw_locked_now() && !master) {
    GUI.buzzer_cmd_long = true;
    uf_pw_show_err("رمز قفل شد. ده دقیقه صبر کنید", "Locked. Wait 10 minutes");
    uf_pw_buf[0] = 0;
    uf_pw_render();
    return;
  }
  if (admin_password_ok(uf_pw_buf)) {
    GUI.buzzer_cmd_short = true;
    uf_pw_clear_lock();
    uf_pw_ok = true;
  } else {
    GUI.buzzer_cmd_long = true;
    if (uf_pw_fail_n < 255) uf_pw_fail_n++;
    if (uf_pw_fail_n >= 3) {
      uf_pw_fail_n = 0;
      uf_pw_arm_lock_10min();
      uf_pw_show_err("رمز قفل شد. ده دقیقه صبر کنید", "Locked. Wait 10 minutes");
    } else {
      uf_pw_show_wrong_count(uf_pw_fail_n);
    }
    uf_pw_buf[0] = 0;
    uf_pw_render();
  }
}
static void uf_pw_ok_cb(lv_event_t *e) { (void)e; uf_pw_verify(); }

// رقم/بک‌اسپیس — فقط ارقام (بدون ممیز)
static void uf_pw_key_cb(lv_event_t *e) {
  if (!uf_pw_ta) return;
  int c = (int)(intptr_t)lv_event_get_user_data(e);
  GUI.buzzer_cmd_short = true;
  if (uf_pw_err) lv_obj_add_flag(uf_pw_err, LV_OBJ_FLAG_HIDDEN);
  int n = (int)strlen(uf_pw_buf);
  if (c == 8) { if (n) uf_pw_buf[n - 1] = 0; }
  else if (c >= '0' && c <= '9' && n < 12) { uf_pw_buf[n] = (char)c; uf_pw_buf[n + 1] = 0; }
  uf_pw_render();
}

// — خروج بدونِ ریست → صفحهٔ اصلی. حذفِ overlay در tick انجام می‌شود (نه داخلِ event) —
static void uf_exit_cb(lv_event_t *e) {
  (void)e;
  if (uf_flashing) return;   // وسط فلش خروج ممنوع
  uf_exit_req = true;
}

static inline const char *uf_ml(const char *fa, const char *en) {
  return uf_lang ? en : fa;
}

static void uf_reset_session_flags(void) {
  uf_tls_clock_ready = false;
  uf_tls_clock_checked = false;
  uf_req_scan = false;
  uf_req_connect = false;
  uf_req_check = false;
  uf_req_flash = false;
  uf_req_prefetch = false;
  uf_req_scan_models = false;
  uf_scanning = false;
  uf_scan_ready = false;
  uf_connecting = false;
  uf_connect_failed = false;
  uf_wifi_connected = false;
  uf_check_done = false;
  uf_models_scanning = false;
  uf_models_scan_done = false;
  uf_models_scan_requested_ms = 0;
  uf_models_scan_started_ms = 0;
  uf_models_scan_session = 0;
  uf_models_scan_done_session = 0;
  uf_models_scan_timed_out = false;
  uf_manual_scan_base[0] = 0;
  uf_manual_scan_base_active = false;
  uf_manual_back_req = false;
  uf_manual_check_req = false;
  uf_manual_open_req = false;
  uf_prefetch_done = false;
  uf_flash_failed = false;
}

static bool uf_machine_busy_for_update(void) {
  return STP.motorIs_moving || STP.moveStepperCMD || STP.jogEnabled ||
         keyboardJogActive || STP.goref_cmd || STP.go_sampling_CMD ||
         STP.begin_calibration || STP.flag_is_cutting ||
         IOS.saw_is_on || IOS.clamp_engaged || IOS.cut_confirm_pending ||
         IOS.both_start_button_state || OPT.active ||
         f3_stepper_test_cmd || f3_stepper_test_busy;
}

// Acquire both maintenance and WiFi ownership before disabling the motion
// tasks. A running portal is atomically marked for handoff; local OTA wins or
// loses the same owner transition, so the two flash paths cannot overlap.
static bool uf_begin_update_maintenance(void) {
  if (g_update_mode || uf_exit_wait_network) {
    GUI.buzzer_cmd_long = true;
    ui_toast("بروزرسانی دیگری در حال اجرا است",
             "Another update session is active", 3000);
    return false;
  }
  if (uf_machine_busy_for_update()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("ابتدا حرکت و برش را متوقف کنید",
             "Stop motion and cutting first", 3500);
    return false;
  }
  if (!uf_mutex) uf_mutex = xSemaphoreCreateMutex();
  if (!uf_mutex) {
    critical_task_start_failed = true;
    panel_request_stop_motion();
    GUI.buzzer_cmd_long = true;
    ui_toast("حافظه کافی نیست؛ دستگاه را راه اندازی مجدد کنید",
             "Not enough memory; restart the device", 4500);
    return false;
  }
  if (!wifi_owner_begin_updater()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("شبکه در حال تغییر یا نصب برنامه است",
             "WiFi is changing state or installing", 3500);
    return false;
  }
  // Close the race with a command that arrived after the first idle check.
  if (uf_machine_busy_for_update()) {
    wifi_owner_cancel_updater();
    GUI.buzzer_cmd_long = true;
    ui_toast("فرمان حرکت تازه دریافت شد؛ بروزرسانی لغو شد",
             "A motion command arrived; update cancelled", 4000);
    return false;
  }
  g_update_mode = true;
  panel_request_stop_motion(); // outputs and every pending motion command safe
  return true;
}

static void uf_schedule_prefetch_after_wifi_handoff(void) {
  web_portal_start_request = false;
  const uint8_t owner = wifi_owner_get();
  if (web_portal_running || web_portal_transitioning ||
      owner == WIFI_OWNER_UPDATER_PENDING || uf_net_operation_active) {
    web_portal_stop_request = true;
    uf_wait_portal_stop = true;
    uf_set_status(uf_ml("در حال آماده سازی وای فای", "Preparing WiFi"));
    return;
  }
  if (owner == WIFI_OWNER_NONE && !wifi_owner_try_acquire(WIFI_OWNER_UPDATER)) {
    uf_wait_portal_stop = true;
    uf_set_status(uf_ml("شبکه در حال تغییر است", "WiFi is changing state"));
    return;
  }
  if (wifi_owner_get() != WIFI_OWNER_UPDATER) {
    uf_wait_portal_stop = true;
    uf_set_status(uf_ml("شبکه در اختیار عملیات دیگری است", "WiFi is busy"));
    return;
  }
  web_portal_stop_request = false;
  uf_wait_portal_stop = false;
  uf_req_prefetch = true;
}

// — کیبورد نباید با دکمهٔ «بستن» مخفی بماند (وگرنه کاربر گیر می‌کند) → دوباره نمایش بده —
static void uf_kb_keep_cb(lv_event_t *e) {
  lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
  if (kb) lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

// — چشمِ نمایش/پنهانِ رمز (مشترکِ صفحهٔ رمز و پنلِ وایفای) —
static void uf_toggle_pw(lv_obj_t *ta, lv_obj_t *icon_lbl) {
  if (!ta) return;
  bool was_hidden = lv_textarea_get_password_mode(ta);
  bool now_hidden = !was_hidden;
  lv_textarea_set_password_mode(ta, now_hidden);
  if (icon_lbl) lv_label_set_text(icon_lbl, was_hidden ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
  if (ta == uf_pw_ta) {
    lv_obj_set_style_pad_top(ta, now_hidden ? 3 : 5, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(ta, LV_SCROLLBAR_MODE_OFF);
  lv_obj_scroll_to(ta, 0, 0, LV_ANIM_OFF);
}
static void uf_pw_eye_cb(lv_event_t *e) { (void)e; uf_toggle_pw(uf_pw_ta, uf_pw_eye_lbl); }
static void uf_wp_eye_cb(lv_event_t *e) { (void)e; uf_toggle_pw(uf_wp_ta, uf_wp_eye_lbl); }

// — وایفای: اسکنِ مجدد —
static void uf_wifi_rescan_cb(lv_event_t *e) {
  (void)e;
  if (uf_scanning || uf_connecting) return;
  uf_set_status("در حال سرچ");
  uf_req_scan = true;
}
static void uf_open_wifi_pass(const char *ssid);   // forward — سازندهٔ پنلِ رمز
// — انتخابِ یک شبکه از دراپ‌داون → بازکردنِ پنلِ رمز —
static void uf_wifi_row_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (!btn) return;
  lv_indev_t *indev = lv_indev_active();
  if (indev && lv_indev_get_scroll_obj(indev) != NULL) return; // گارد ضد لمس هنگام اسکرول
  const char *ssid = (const char *)lv_event_get_user_data(e);
  if (ssid && ssid[0]) {
    uf_lock();
    strncpy(uf_sel_ssid, ssid, sizeof(uf_sel_ssid) - 1);
    uf_sel_ssid[sizeof(uf_sel_ssid) - 1] = 0;
    uf_unlock();
    uf_wp_open_req = true; // پنل در tick باز می‌شود
  }
}

static void uf_wifi_dd_cb(lv_event_t *e) {
  lv_obj_t *dd = (lv_obj_t *)lv_event_get_target(e);
  char ssid[48] = {0};
  lv_dropdown_get_selected_str(dd, ssid, sizeof(ssid) - 1);
  const bool placeholder =
      strcmp(ssid, "...") == 0 ||
      strcmp(ssid, "(no networks)") == 0 ||
      strcmp(ssid, "(WiFi unavailable)") == 0;
  if (ssid[0] && !placeholder) {
    uf_lock();
    strncpy(uf_sel_ssid, ssid, sizeof(uf_sel_ssid) - 1); uf_sel_ssid[sizeof(uf_sel_ssid) - 1] = 0;
    uf_unlock();
    uf_wp_open_req = true;
  }
}
// — پنلِ رمزِ وایفای: اتصال —
static void uf_wifi_pick_cb(lv_event_t *e) {
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  const char *ssid = NULL;
  uint32_t cnt = lv_obj_get_child_count(btn);
  for (uint32_t i = 0; i < cnt; i++) {
    lv_obj_t *child = lv_obj_get_child(btn, i);
    if (lv_obj_check_type(child, &lv_label_class)) {
      const char *txt = lv_label_get_text(child);
      if (txt && txt[0]) ssid = txt;
    }
  }
  if (ssid && ssid[0]) uf_open_wifi_pass(ssid);
}

static void uf_wp_connect_cb(lv_event_t *e) {
  (void)e;
  if (!uf_wp_ta) return;
  const char *p = lv_textarea_get_text(uf_wp_ta);
  uf_lock();
  strncpy(uf_sel_pass, p ? p : "", sizeof(uf_sel_pass) - 1); uf_sel_pass[sizeof(uf_sel_pass) - 1] = 0;
  uf_unlock();
  uf_connect_failed = false;
  uf_wp_has_status = true;
  uf_set_status("در حال اتصال");
  if (uf_wp_status) lv_label_set_text(uf_wp_status, "در حال اتصال");
  uf_req_connect = true;
}
// — پنلِ رمزِ وایفای: بازگشت (حذف در tick) —
static void uf_wp_back_cb(lv_event_t *e) { (void)e; uf_wp_close_req = true; }

// — بروزرسانی: شروع نصب برای مدلِ idx —
static void uf_cfm_close(void);
static void uf_upd_set_install_enabled(bool on);
static void uf_begin_install(int idx) {
  if (uf_flashing || uf_models_scanning) return;
  if (idx < 0 || idx > 1) return;
  if (uf_card_btn_state[idx] != UF_BTN_READY) return;
  uf_upd_sel = idx;
  uf_lock();
  strncpy(uf_model,
          idx == 1 ? "upvc/firmware_config" : "upvc/firmware_digital_cutter_single_head",
          sizeof(uf_model) - 1);
  uf_model[sizeof(uf_model) - 1] = 0;
  uf_unlock();
  for (int i = 0; i < 2; i++) {
    if (uf_upd_card[i]) {
      lv_obj_set_style_border_color(uf_upd_card[i],
        i == idx ? lv_color_hex(0x1e88e5) : lv_color_hex(0x2a4a6e), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(uf_upd_card[i], i == idx ? 3 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
  uf_upd_set_install_enabled(false);
  if (uf_upd_exit) lv_obj_add_flag(uf_upd_exit, LV_OBJ_FLAG_HIDDEN);
  uf_flash_failed = false;
  uf_ota_pct = -1;
  uf_set_status(uf_ml("در حال نصب. صبر کنید", "Installing. Please wait"));
  uf_req_flash = true;
}

static void uf_cfm_yes_cb(lv_event_t *e) {
  (void)e;
  GUI.buzzer_cmd_short = true;
  uf_cfm_close();
  uf_begin_install(1);
}
static void uf_cfm_no_cb(lv_event_t *e) {
  (void)e;
  GUI.buzzer_cmd_short = true;
  uf_cfm_close();
}
static void uf_cfm_close(void) {
  if (uf_cfm_root) {
    lv_obj_delete(uf_cfm_root);
    uf_cfm_root = NULL;
  }
}
static void uf_show_config_confirm(void) {
  if (uf_cfm_root || uf_flashing) return;
  uf_cfm_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(uf_cfm_root, 480, 320);
  lv_obj_set_pos(uf_cfm_root, 0, 0);
  lv_obj_set_style_bg_color(uf_cfm_root, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(uf_cfm_root, LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(uf_cfm_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_cfm_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(uf_cfm_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(uf_cfm_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(uf_cfm_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(uf_cfm_root);
  lv_obj_set_size(card, 360, 168);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 330);
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(msg, uf_lang ? &lv_font_montserrat_16 : &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(msg, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
  if (!uf_lang) lv_obj_set_style_base_dir(msg, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(msg, uf_ml(
    "برنامه کانفیگ جایگزین برنامه برش می‌شود. مطمئنید؟",
    "Config will replace Cutter firmware. Continue?"));
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *bn = lv_button_create(card);
  lv_obj_set_size(bn, 140, 44);
  lv_obj_align(bn, LV_ALIGN_BOTTOM_LEFT, 8, -4);
  lv_obj_set_style_bg_color(bn, lv_color_hex(0x8e8e93), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(bn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(bn, uf_cfm_no_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ln = lv_label_create(bn);
  lv_obj_set_style_text_font(ln, uf_lang ? &lv_font_montserrat_16 : &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ln, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(ln, uf_ml("انصراف", "Cancel"));
  lv_obj_center(ln);

  lv_obj_t *by = lv_button_create(card);
  lv_obj_set_size(by, 140, 44);
  lv_obj_align(by, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
  lv_obj_set_style_bg_color(by, lv_color_hex(0xff3b30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(by, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(by, uf_cfm_yes_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ly = lv_label_create(by);
  lv_obj_set_style_text_font(ly, uf_lang ? &lv_font_montserrat_16 : &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ly, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(ly, uf_ml("نصب کانفیگ", "Install Config"));
  lv_obj_center(ly);
}

static void uf_upd_style_one_btn(int i) {
  if (i < 0 || i > 1 || !uf_upd_card_btn[i]) return;
  lv_obj_t *btn = uf_upd_card_btn[i];
  lv_obj_t *lbl = uf_upd_card_btn_lbl[i];
  uint8_t st = uf_card_btn_state[i];
  const char *txt;
  lv_color_t bg, fg;
  bool long_txt = true;
  if (st == UF_BTN_SEARCHING) {
    txt = uf_ml("در حال جستجو", "Searching...");
    bg = lv_color_hex(0x3A3A3C);
    fg = lv_color_hex(0xAEAEB2);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  } else if (st == UF_BTN_MISSING) {
    txt = uf_ml("نصب ناموجود", "Install N/A");
    bg = UF_COL_OK_DIM;
    fg = UF_COL_MUTED;
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  } else if (st == UF_BTN_CURRENT) {
    txt = uf_ml("نصب کد بروز هست", "Up to date");
    bg = lv_color_hex(0x2C2C2E);
    fg = lv_color_hex(0xAEAEB2);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  } else {
    txt = uf_ml("نصب", "Install");
    bg = UF_COL_OK;
    fg = UF_COL_TXT;
    long_txt = false;
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);  // هرگز پنهان نکن — فقط متن/رنگ عوض شود
  lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN | LV_STATE_PRESSED);
  if (lbl) {
    lv_obj_set_style_text_font(lbl,
      uf_lang ? (long_txt ? &lv_font_montserrat_14 : &lv_font_montserrat_16)
              : UF_FONT_FA_S,
      LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, fg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl, txt);
    lv_obj_center(lbl);
  }
}

// on=true → ظاهر از uf_card_btn_state؛ on=false → مشغول/فلش (پنهان نمی‌شود)
static void uf_upd_set_install_enabled(bool on) {
  for (int i = 0; i < 2; i++) {
    if (!uf_upd_card_btn[i]) continue;
    if (on) {
      uf_upd_style_one_btn(i);
      continue;
    }
    lv_obj_clear_flag(uf_upd_card_btn[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uf_upd_card_btn[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(uf_upd_card_btn[i], lv_color_hex(0x3A3A3C), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (uf_upd_card_btn_lbl[i]) {
      lv_label_set_text(uf_upd_card_btn_lbl[i], uf_ml("صبر کنید", "Please wait"));
      lv_obj_set_style_text_font(uf_upd_card_btn_lbl[i],
        uf_lang ? &lv_font_montserrat_14 : UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(uf_upd_card_btn_lbl[i], UF_COL_MUTED, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(uf_upd_card_btn_lbl[i]);
    }
  }
}

// — بروزرسانی: نصب از دکمهٔ داخل کارت —
static void uf_upd_install_cb(lv_event_t *e) {
  lv_event_stop_bubbling(e);
  if (uf_flashing || uf_models_scanning) return;
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  if (idx < 0 || idx > 1) return;
  if (uf_card_btn_state[idx] != UF_BTN_READY) return;  // ناموجود / بروز → نصب نشود
  GUI.buzzer_cmd_short = true;
  if (idx == 1) { uf_show_config_confirm(); return; }
  uf_begin_install(0);
}

// — انتخابِ بصری کارت (نصب با دکمهٔ سبز داخل کارت) —
static void uf_upd_card_cb(lv_event_t *e) {
  if (uf_flashing) return;
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  uf_upd_sel = idx;
  GUI.buzzer_cmd_short = true;
  for (int i = 0; i < 2; i++) {
    if (uf_upd_card[i]) {
      lv_obj_set_style_border_color(uf_upd_card[i],
        i == idx ? lv_color_hex(0x1e88e5) : lv_color_hex(0x2a4a6e), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(uf_upd_card[i], i == idx ? 3 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
}

// =============================================================================
//  ساختِ صفحه‌ها  (فقط از ترد LVGL صدا زده می‌شود)
// =============================================================================
// دکمهٔ تکیِ نامپد (رقم/نقطه/بک‌اسپیس) با استایلِ کارت
static void uf_key_btn(lv_obj_t *parent, int x, int y, int w, int h, const char *txt,
                       const lv_font_t *font, bool op, int code, lv_obj_t **out_lbl) {
  lv_obj_t *b = lv_button_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_style_radius(b, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  bool bs = (code == 8);   // بک‌اسپیس قرمز
  lv_obj_set_style_bg_color(b, bs ? lv_color_hex(0xc62828) : (op ? lv_color_hex(0xdfeaf5) : lv_color_hex(0xffffff)), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(b, uf_pw_key_cb, LV_EVENT_CLICKED, (void *)(intptr_t)code);
  lv_obj_t *l = lv_label_create(b);
  lv_obj_set_style_text_font(l, font, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, bs ? lv_color_hex(0xffffff) : (op ? lv_color_hex(0x3a608c) : lv_color_hex(0x17384d)), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  if (out_lbl) *out_lbl = l;
}

// پرچمِ روی دکمهٔ زبان — در فارسی پرچمِ انگلیس (کلیک→EN)، در انگلیسی پرچمِ ایران
static void uf_pw_make_flag(void) {
  if (!uf_lang_btn) return;
  lv_obj_clean(uf_lang_btn);
  lv_obj_t *fl = f1_mk_mini_flag(uf_lang_btn, uf_lang ? 1 : 0);   // پرچمِ زبانِ فعلی (0=ایران/فارسی، 1=انگلیس)
  lv_obj_center(fl);
}

// اعمالِ زبانِ جاری روی نوشته‌ها و گلیفِ ارقام
static void uf_pw_apply_lang(void) {
  for (int i = 0; i < 10; i++) {
    if (!uf_pw_dig[i]) continue;
    char en[2] = {(char)('0' + i), 0};
    lv_label_set_text(uf_pw_dig[i], uf_lang ? en : UF_FA_DIG[i]);
    // فونت هم عوض شود: انگلیسی=montserrat (ارقامِ لاتین)، فارسی=naz (عربی‌ـهندی)
    lv_obj_set_style_text_font(uf_pw_dig[i], uf_lang ? (const lv_font_t *)&lv_font_montserrat_24 : (const lv_font_t *)&ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (uf_pw_title) {
    lv_label_set_text(uf_pw_title, uf_lang ? "Firmware update" : "بروز رسانی نرم افزار");
    lv_obj_set_style_text_font(uf_pw_title, uf_lang ? &lv_font_montserrat_24 : &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (uf_pw_sub) {
    lv_label_set_text(uf_pw_sub, uf_lang ? "Enter admin password" : "رمز مدیر را وارد کنید");
    lv_obj_set_style_text_font(uf_pw_sub, uf_lang ? (const lv_font_t *)&lv_font_montserrat_16 : UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (uf_pw_ok_lbl) {
    lv_label_set_text(uf_pw_ok_lbl, uf_lang ? "OK" : "تأیید");
    lv_obj_set_style_text_font(uf_pw_ok_lbl, uf_lang ? (const lv_font_t *)&lv_font_montserrat_20 : UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (uf_pw_err && !lv_obj_has_flag(uf_pw_err, LV_OBJ_FLAG_HIDDEN)) {
    if (uf_pw_locked_now())
      lv_label_set_text(uf_pw_err, uf_lang ? "Locked. Wait 10 minutes" : "رمز قفل شد. ده دقیقه صبر کنید");
    else if (uf_pw_fail_n > 0)
      uf_pw_show_wrong_count(uf_pw_fail_n);
    else
      lv_label_set_text(uf_pw_err, uf_lang ? "Wrong password" : "رمز اشتباه است");
  }
  if (uf_pw_ta) {   // فونت + محتوای کادر هم با زبان عوض شود (ارقامِ واردشده تبدیل می‌شوند)
    lv_obj_set_style_text_font(uf_pw_ta, uf_lang ? (const lv_font_t *)&lv_font_montserrat_20 : (const lv_font_t *)&ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    uf_pw_render();
  }
  uf_pw_make_flag();
}
static void uf_lang_toggle_cb(lv_event_t *e) { (void)e; uf_lang ^= 1; uf_pw_apply_lang(); }

static inline void uf_start_net_task_once();  // forward — prefetch از صفحهٔ رمز

static void uf_build_password(void) {
  uf_pw_buf[0] = 0;
  uf_lang = (GUI.system_language == 2) ? 1 : 0;   // شروع طبقِ زبانِ فعلیِ دستگاه
  { Preferences p; p.begin("uf_gate", true);
    bool locked = p.getBool("locked", false);
    p.end();
    if (locked) {
      if (uf_pw_lock_until_ms == 0) uf_pw_arm_lock_10min();
      else if (!uf_pw_locked_now()) uf_pw_clear_lock();
    } else {
      uf_pw_clear_lock_if_expired();
    }
  }
  uf_pw_root = uf_make_root();

  // خروج (آیکونِ خانه) — بالا چپ → صفحهٔ اصلی بدونِ ریست
  lv_obj_t *ex = lv_button_create(uf_pw_root);
  lv_obj_set_size(ex, 40, 34); lv_obj_set_pos(ex, 12, 10);
  lv_obj_set_style_bg_color(ex, lv_color_hex(0x1e5f9e), LV_PART_MAIN | LV_STATE_DEFAULT);  // آبی
  lv_obj_set_style_radius(ex, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ex, uf_exit_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *exl = lv_label_create(ex);
  lv_obj_set_style_text_font(exl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(exl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(exl, LV_SYMBOL_HOME);
  lv_obj_center(exl);

  // دکمهٔ زبان (پرچم) — بالا راست
  uf_lang_btn = lv_button_create(uf_pw_root);
  lv_obj_set_size(uf_lang_btn, 52, 36); lv_obj_set_pos(uf_lang_btn, 416, 9);
  lv_obj_set_style_bg_color(uf_lang_btn, lv_color_hex(0x0f2947), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(uf_lang_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(uf_lang_btn, lv_color_hex(0x2f5b86), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_lang_btn, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(uf_lang_btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(uf_lang_btn, uf_lang_toggle_cb, LV_EVENT_CLICKED, NULL);

  // عنوان + زیرعنوان (زبان‌پذیر — فونتِ naz برای رفعِ مربع/tofu)
  // عنوان ۹px چپ؛ زیرعنوان/کادر/چشم در موقعیت توافق‌شده قبل از خرابکاری استیکر
  uf_pw_title = uf_make_label(uf_pw_root, "بروز رسانی نرم افزار", &ui_font_naz30, UF_COL_TXT, -9, 2, 480);
  uf_pw_sub   = uf_make_label(uf_pw_root, "رمز مدیر را وارد کنید", UF_FONT_FA_S, lv_color_hex(0x9fc4e8), -7, 43, 480);

  // فیلدِ رمز (نقطه‌ای)
  uf_pw_ta = lv_textarea_create(uf_pw_root);
  lv_textarea_set_one_line(uf_pw_ta, true);
  lv_textarea_set_password_mode(uf_pw_ta, false);
  lv_textarea_set_max_length(uf_pw_ta, 12);
  lv_textarea_set_placeholder_text(uf_pw_ta, "----");
  lv_obj_set_size(uf_pw_ta, 208, 44);
  lv_obj_set_pos(uf_pw_ta, 124, 74);
  lv_obj_set_style_text_align(uf_pw_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(uf_pw_ta, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(uf_pw_ta, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(uf_pw_ta, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(uf_pw_ta, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_top(uf_pw_ta, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(uf_pw_ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

  { // چشمِ نمایش/پنهانِ رمز — سمتِ راستِ کادر
    lv_obj_t *eye = lv_button_create(uf_pw_root);
    lv_obj_set_size(eye, 38, 42); lv_obj_set_pos(eye, 338, 73);
    lv_obj_set_style_bg_color(eye, lv_color_hex(0x14375f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(eye, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(eye, uf_pw_eye_cb, LV_EVENT_CLICKED, NULL);
    uf_pw_eye_lbl = lv_label_create(eye);
    lv_obj_set_style_text_font(uf_pw_eye_lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(uf_pw_eye_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(uf_pw_eye_lbl, LV_SYMBOL_EYE_OPEN);   // پیش‌فرض نمایان (چشمِ باز)
    lv_obj_center(uf_pw_eye_lbl);
  }

  uf_pw_err = uf_make_label(uf_pw_root, "رمز اشتباه است", UF_FONT_FA_S, UF_COL_ERR, 0, 118, 480);
  if (uf_pw_locked_now()) {
    lv_label_set_text(uf_pw_err, uf_lang ? "Locked. Wait 10 minutes" : "رمز قفل شد. ده دقیقه صبر کنید");
    lv_obj_clear_flag(uf_pw_err, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(uf_pw_err, LV_OBJ_FLAG_HIDDEN);
  }

  // نامپد 3×4 بزرگ‌تر: بدون ممیز؛ پاک‌کردن قرمز چپ؛ تأیید سبز راست؛ +۱۶px پایین
  uf_pw_kb_root = lv_obj_create(uf_pw_root);
  lv_obj_set_size(uf_pw_kb_root, 480, 320);
  lv_obj_set_pos(uf_pw_kb_root, 0, 0);
  lv_obj_set_style_bg_opa(uf_pw_kb_root, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(uf_pw_kb_root, 0, 0);
  lv_obj_set_style_pad_all(uf_pw_kb_root, 0, 0);
  lv_obj_clear_flag(uf_pw_kb_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(uf_pw_kb_root, LV_OBJ_FLAG_CLICKABLE);
  const int cx[3] = {86, 190, 294};
  const int cy[4] = {156, 196, 236, 276};
  const int KW = 100, KH = 36;
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 3; c++) {
      int n = r * 3 + c + 1;
      char t[2] = {(char)('0' + n), 0};
      uf_key_btn(uf_pw_kb_root, cx[c], cy[r], KW, KH, t, &ui_font_naz24, false, '0' + n, &uf_pw_dig[n]);
    }
  uf_key_btn(uf_pw_kb_root, cx[0], cy[3], KW, KH, LV_SYMBOL_BACKSPACE, &lv_font_montserrat_20, true, 8, NULL);
  uf_key_btn(uf_pw_kb_root, cx[1], cy[3], KW, KH, "0", &ui_font_naz24, false, '0', &uf_pw_dig[0]);
  {
    lv_obj_t *okb = lv_button_create(uf_pw_kb_root);
    lv_obj_set_size(okb, KW, KH);
    lv_obj_set_pos(okb, cx[2], cy[3]);
    lv_obj_set_style_bg_color(okb, lv_color_hex(0x1c9b67), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(okb, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(okb, uf_pw_ok_cb, LV_EVENT_CLICKED, NULL);
    uf_pw_ok_lbl = lv_label_create(okb);
    lv_obj_set_style_text_font(uf_pw_ok_lbl, UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(uf_pw_ok_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(uf_pw_ok_lbl, "تأیید");
    lv_obj_center(uf_pw_ok_lbl);
  }

  uf_pw_apply_lang();   // اعمالِ زبانِ اولیه (گلیفِ ارقام + پرچم + نوشته‌ها)
  if (!GUI.virtual_kb_enable) lv_obj_add_flag(uf_pw_kb_root, LV_OBJ_FLAG_HIDDEN);

  // شبکه را اینجا شروع نکن — روی صفحهٔ رمز هنگ می‌آورد (WiFi/scan با LVGL می‌جنگد).
  // بعد از تأیید رمز در UF_ST_PASSWORD شروع می‌شود.
}

// دکمهٔ آیکونیِ گوشه (خروج/جستجو/بازگشت)
static lv_obj_t *uf_corner_btn(lv_obj_t *parent, int x, lv_color_t col, const char *sym, lv_event_cb_t cb) {
  lv_obj_t *b = lv_button_create(parent);
  lv_obj_set_size(b, 40, 34); lv_obj_set_pos(b, x, 8);
  lv_obj_set_style_bg_color(b, col, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(b, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *l = lv_label_create(b);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(l, sym); lv_obj_center(l);
  return b;
}

static void uf_build_wifi(void) {
  uf_wifi_root = uf_make_root();
  lv_obj_t *home = uf_corner_btn(uf_wifi_root, 3, lv_color_hex(0x0a84ff), LV_SYMBOL_HOME, uf_exit_cb);
  lv_obj_set_size(home, 44, 39);
  lv_obj_set_pos(home, 3, 2);
  lv_obj_set_style_radius(home, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (lv_obj_get_child_count(home) > 0) {
    lv_obj_set_style_text_font(lv_obj_get_child(home, 0), &lv_font_montserrat_26,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  uf_make_label(uf_wifi_root, "شبکه های وای فای", UF_FONT_FA, UF_COL_TXT, 0, 10, 480);
  uf_wifi_status = uf_make_label(uf_wifi_root, "در حال جستجو...", UF_FONT_FA_S, lv_color_hex(0x9fc4e8), 0, 52, 480);

  // دکمه جستجوی مجدد در بالای سمت راست
  lv_obj_t *sb = lv_button_create(uf_wifi_root);
  lv_obj_set_size(sb, 44, 39); lv_obj_set_pos(sb, 433, 2);
  lv_obj_set_style_bg_color(sb, UF_COL_PRIMARY, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(sb, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(sb, uf_wifi_rescan_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sl = lv_label_create(sb);
  lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(sl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(sl, LV_SYMBOL_REFRESH); lv_obj_center(sl);

  // کادر اسکرول لیست شبکه‌های وای‌فای
  uf_wifi_list = lv_obj_create(uf_wifi_root);
  lv_obj_set_size(uf_wifi_list, 456, 218); lv_obj_set_pos(uf_wifi_list, 12, 94);
  lv_obj_set_flex_flow(uf_wifi_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(uf_wifi_list, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(uf_wifi_list, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(uf_wifi_list, lv_color_hex(0x0c1622), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_wifi_list, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(uf_wifi_list, lv_color_hex(0x2a4a6e), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(uf_wifi_list, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(uf_wifi_list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(uf_wifi_list, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(uf_wifi_list, LV_DIR_VER);
}

#define UF_KB_BTN(w)   ((lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_POPOVER | (w)))
#define UF_KB_FLAGS(w) ((lv_buttonmatrix_ctrl_t)(LV_KEYBOARD_CTRL_BUTTON_FLAGS | (w)))
#define UF_KB_CHK(w)   ((lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED | (w)))
#define UF_KB_RAW(w)   ((lv_buttonmatrix_ctrl_t)(w))

static const char * const uf_clean_kb_map_lc[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
  "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "/", "\n",
  "a", "s", "d", "f", "g", "h", "j", "k", "l", ":", ".", "\n",
  "ABC", "z", "x", "c", "v", "b", "n", "m", "-", "1#", LV_SYMBOL_OK, ""
};

#define UF_KB_1 (lv_buttonmatrix_ctrl_t)1
#define UF_KB_F (lv_buttonmatrix_ctrl_t)(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2)

static const lv_buttonmatrix_ctrl_t uf_clean_kb_ctrl_lc[] = {
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_F,
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1,
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1,
  UF_KB_F, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_F, UF_KB_F
};

static const char * const uf_clean_kb_map_uc[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
  "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "/", "\n",
  "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", ".", "\n",
  "abc", "Z", "X", "C", "V", "B", "N", "M", "_", "1#", LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t uf_clean_kb_ctrl_uc[] = {
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_F,
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1,
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1,
  UF_KB_F, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_F, UF_KB_F
};

static const char * const uf_clean_kb_map_spec[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
  "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "/", "\n",
  "-", "_", "=", "+", "[", "]", "{", "}", "\\", "|", "~", "\n",
  "abc", ";", "\'", "\"", "<", ">", ",", ".", "?", "ABC", LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t uf_clean_kb_ctrl_spec[] = {
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_F,
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1,
  UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1,
  UF_KB_F, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_1, UF_KB_F, UF_KB_F
};

static void uf_apply_clean_keyboard_maps(lv_obj_t *kb) {
  if (!kb) return;
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, uf_clean_kb_map_lc, uf_clean_kb_ctrl_lc);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, uf_clean_kb_map_uc, uf_clean_kb_ctrl_uc);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, uf_clean_kb_map_spec, uf_clean_kb_ctrl_spec);
#if LV_USE_ARABIC_PERSIAN_CHARS == 1
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_ARABIC, uf_clean_kb_map_lc, uf_clean_kb_ctrl_lc);
#endif
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
}

// پنلِ رمزِ یک شبکه (روی لیست باز می‌شود): نامِ شبکه + فیلدِ رمز + چشم + کیبورد + اتصال + بازگشت
static void uf_open_wifi_pass(const char *ssid) {
  if (uf_wp_root) return;
  uf_lock();
  strncpy(uf_sel_ssid, ssid, sizeof(uf_sel_ssid) - 1); uf_sel_ssid[sizeof(uf_sel_ssid) - 1] = 0;
  uf_unlock();

  uf_wp_root = uf_make_root();
  uf_corner_btn(uf_wp_root, 6, lv_color_hex(0x1e5f9e), LV_SYMBOL_LEFT, uf_wp_back_cb);  // بازگشت — آبی، ۶px چپ‌تر

  uf_make_label(uf_wp_root, ssid, &lv_font_montserrat_20, UF_COL_TXT, -29, 10, 480);
  uf_wp_status = uf_make_label(uf_wp_root, "رمز وایفا را وارد کنید", UF_FONT_FA_S, lv_color_hex(0x9fc4e8), -42, 35, 480);

  uf_wp_ta = lv_textarea_create(uf_wp_root);
  lv_textarea_set_one_line(uf_wp_ta, true);
  lv_textarea_set_password_mode(uf_wp_ta, true);
  lv_textarea_set_placeholder_text(uf_wp_ta, "password");
  lv_obj_set_size(uf_wp_ta, 250, 40); lv_obj_set_pos(uf_wp_ta, 68, 65);
  lv_obj_set_style_text_align(uf_wp_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(uf_wp_ta, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(uf_wp_ta, LV_SCROLLBAR_MODE_OFF);

  { lv_obj_t *eye = lv_button_create(uf_wp_root);
    lv_obj_set_size(eye, 38, 40); lv_obj_set_pos(eye, 324, 65);
    lv_obj_set_style_bg_color(eye, lv_color_hex(0x14375f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(eye, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(eye, uf_wp_eye_cb, LV_EVENT_CLICKED, NULL);
    uf_wp_eye_lbl = lv_label_create(eye);
    lv_obj_set_style_text_font(uf_wp_eye_lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(uf_wp_eye_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(uf_wp_eye_lbl, LV_SYMBOL_EYE_CLOSE); lv_obj_center(uf_wp_eye_lbl); }

  { lv_obj_t *cn = lv_button_create(uf_wp_root);   // اتصال — کنارِ کادر، سبز
    lv_obj_set_size(cn, 96, 40); lv_obj_set_pos(cn, 372, 65);
    lv_obj_set_style_bg_color(cn, UF_COL_OK, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(cn, uf_wp_connect_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(cn);
    lv_obj_set_style_text_font(l, UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(l, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(l, "اتصال"); lv_obj_center(l); }

  // کیبوردِ کاملِ LVGL: حروف/اعداد/علائم با دکمه‌های ABC و 1# خودش — بزرگ و خوانا
  uf_wp_kb = lv_keyboard_create(uf_wp_root);
  uf_apply_clean_keyboard_maps(uf_wp_kb);
  lv_keyboard_set_textarea(uf_wp_kb, uf_wp_ta);
  lv_obj_set_size(uf_wp_kb, 480, 196);
  lv_obj_align(uf_wp_kb, LV_ALIGN_BOTTOM_MID, 0, 0);   // چسبیده به پایین (set_pos روی کیبورد جابه‌جا می‌افتاد)
  lv_obj_set_style_text_font(uf_wp_kb, &lv_font_montserrat_18, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(uf_wp_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(uf_wp_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(uf_wp_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(uf_wp_kb, lv_color_hex(0x0b1c33), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(uf_wp_kb, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(uf_wp_kb, lv_color_hex(0x17384d), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_wp_kb, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(uf_wp_kb, uf_kb_keep_cb, LV_EVENT_CANCEL, uf_wp_kb);
  lv_obj_add_event_cb(uf_wp_kb, [](lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    }
  }, LV_EVENT_VALUE_CHANGED, NULL);

  uf_wp_has_status = false;
  // اگر رمزِ همین شبکه قبلاً ذخیره شده → پرکردنِ کادر و اتصالِ خودکارِ سریع
  {
    String ss, pw;
    uf_get_persisted_wifi(ss, pw);
    if (ss.length() && pw.length() && ss == String(ssid)) {
      lv_textarea_set_text(uf_wp_ta, pw.c_str());
      uf_lock();
      strncpy(uf_sel_pass, pw.c_str(), sizeof(uf_sel_pass) - 1); uf_sel_pass[sizeof(uf_sel_pass) - 1] = 0;
      uf_unlock();
      uf_connect_failed = false;
      uf_wp_has_status = true;
      uf_set_status("در حال اتصال");
      if (uf_wp_status) lv_label_set_text(uf_wp_status, "در حال اتصال");
      uf_req_connect = true;
    }
  }
}

// کارتِ انتخابِ دستگاه (الگوی V.1.5.0): عنوان + عکس + نسخه + دکمهٔ نصب داخل کارت
// Manual HTTPS source is session-only and used only for version discovery;
// uf_net_flash() remains on the approved UF_OTA_BASE_URL path.
static bool uf_manual_base_valid(const char *in, char *out, size_t out_sz) {
  if (!in || !out || out_sz < 10) return false;
  while (*in == ' ') in++;
  const size_t n = strlen(in);
  if (n < 8 || n >= out_sz) return false;
  const bool is_https = (strncmp(in, "https://", 8) == 0);
  const bool is_http  = (strncmp(in, "http://", 7) == 0);
  if (!is_https && !is_http) return false;
  const size_t start = is_https ? 8 : 7;
  size_t host_n = 0;
  for (size_t i = start; i < n; i++) {
    const uint8_t c = (uint8_t)in[i];
    if (c <= 0x20 || c == 0x7f || c == '@' || c == '#' || c == '?') return false;
    if (c == '/' || c == ':') break;
    host_n++;
  }
  if (host_n == 0) return false;
  memcpy(out, in, n); out[n] = 0;
  while (strlen(out) > start && out[strlen(out) - 1] == '/') out[strlen(out) - 1] = 0;
  return out[start] != 0;
}
static void uf_manual_back_cb(lv_event_t *e) { (void)e; uf_manual_back_req = true; }
static void uf_manual_check_cb(lv_event_t *e) {
  (void)e;
  char base[sizeof(uf_manual_scan_base)];
  if (!uf_manual_ta || !uf_manual_base_valid(lv_textarea_get_text(uf_manual_ta), base, sizeof(base))) {
    GUI.buzzer_cmd_long = true;
    if (uf_manual_status) lv_label_set_text(uf_manual_status, uf_ml("نشانی HTTP یا HTTPS معتبر وارد کنید", "Enter a valid HTTP or HTTPS URL"));
    return;
  }
  GUI.buzzer_cmd_short = true;
  uf_lock(); strncpy(uf_manual_scan_base, base, sizeof(uf_manual_scan_base) - 1);
  uf_manual_scan_base[sizeof(uf_manual_scan_base) - 1] = 0; uf_manual_scan_base_active = true; uf_unlock();
  uf_manual_check_req = true;  // delete/build happens in Task2 tick, after this event returns
}
static lv_obj_t *uf_info_modal = NULL;

static void uf_info_modal_close(void) {
  if (uf_info_modal) {
    lv_obj_delete(uf_info_modal);
    uf_info_modal = NULL;
  }
}

static void uf_show_card_info(int idx) {
  uf_info_modal_close();
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  uf_info_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_size(uf_info_modal, 440, 280);
  lv_obj_center(uf_info_modal);
  lv_obj_set_style_bg_color(uf_info_modal, lv_color_hex(0x162438), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_info_modal, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(uf_info_modal, lv_color_hex(0x2a527a), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(uf_info_modal, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(uf_info_modal, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Close X button
  lv_obj_t *xb = lv_button_create(uf_info_modal);
  lv_obj_set_size(xb, 36, 36);
  lv_obj_align(xb, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(xb, lv_color_hex(0xef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(xb, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(xb, [](lv_event_t *e) {
    (void)e;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    uf_info_modal_close();
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *xl = lv_label_create(xb);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_center(xl);

  // Title
  lv_obj_t *title = lv_label_create(uf_info_modal);
  lv_obj_set_style_text_font(title, uf_lang ? &lv_font_montserrat_16 : &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(title, idx == 0 ? uf_ml("اطلاعات نرم‌افزار برش تک‌سر", "Single-Head Cutter Info") : uf_ml("اطلاعات نرم‌افزار کانفیگ", "Config Test Info"));
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 6, 4);

  // Version banner
  char vstr[64];
  snprintf(vstr, sizeof(vstr), "%s: %s", uf_ml("نسخه سرور", "Server Version"), uf_card_ver[idx][0] ? uf_card_ver[idx] : "—");
  lv_obj_t *vlbl = lv_label_create(uf_info_modal);
  lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(vlbl, lv_color_hex(0x7fa8d0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(vlbl, vstr);
  lv_obj_set_pos(vlbl, 6, 36);

  // Description content box
  lv_obj_t *box = lv_obj_create(uf_info_modal);
  lv_obj_set_size(box, 416, 178);
  lv_obj_set_pos(box, 2, 60);
  lv_obj_set_style_bg_color(box, lv_color_hex(0x0c1622), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *dlbl = lv_label_create(box);
  lv_obj_set_width(dlbl, 396);
  lv_obj_set_style_text_font(dlbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(dlbl, lv_color_hex(0xe0e6ed), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(dlbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  const char *desc = uf_lastfile_desc[idx][0] ? uf_lastfile_desc[idx] : (uf_lang ? "No release notes available from server." : "توضیحاتی از سرور دریافت نشد.");
  lv_label_set_text(dlbl, desc);
}

static void uf_open_manual_source(void) {
  if (uf_manual_root) return;
  uf_manual_root = uf_make_root();
  uf_corner_btn(uf_manual_root, 6, lv_color_hex(0x1e5f9e), LV_SYMBOL_LEFT, uf_manual_back_cb);
  uf_make_label(uf_manual_root, uf_ml("ورود نشانی بروزرسانی", "Manual Update Link"), uf_lang ? &lv_font_montserrat_18 : &ui_font_naz18b, UF_COL_TXT, 0, 4, 480);
  
  uf_manual_ta = lv_textarea_create(uf_manual_root);
  lv_textarea_set_one_line(uf_manual_ta, true);
  lv_textarea_set_placeholder_text(uf_manual_ta, "http://ota.haffler-infi.ir/...");
  lv_obj_set_size(uf_manual_ta, 236, 34);
  lv_obj_set_pos(uf_manual_ta, 8, 34);
  lv_obj_set_style_text_font(uf_manual_ta, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(uf_manual_ta, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(uf_manual_ta, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(uf_manual_ta, LV_SCROLLBAR_MODE_OFF);

  // کلید درج لینک پیش‌فرض (الگوی بستنی)
  lv_obj_t *btn_def = lv_button_create(uf_manual_root);
  lv_obj_set_size(btn_def, 74, 34);
  lv_obj_set_pos(btn_def, 250, 34);
  lv_obj_set_style_bg_color(btn_def, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_def, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_def, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_def, [](lv_event_t *e) {
    (void)e;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    if (uf_manual_ta) {
      lv_textarea_set_text(uf_manual_ta, "http://ota.haffler-infi.ir/upvc/digital-cutter_single_head/firmware.bin");
    }
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *def_lbl = lv_label_create(btn_def);
  lv_obj_set_style_text_font(def_lbl, uf_lang ? &lv_font_montserrat_14 : &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(def_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(def_lbl, uf_ml("پیشفرض", "Default"));
  lv_obj_center(def_lbl);

  // کلید پاک کردن سریع (الگوی بستنی)
  lv_obj_t *btn_clr = lv_button_create(uf_manual_root);
  lv_obj_set_size(btn_clr, 54, 34);
  lv_obj_set_pos(btn_clr, 330, 34);
  lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0x64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_clr, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_clr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_clr, [](lv_event_t *e) {
    (void)e;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    if (uf_manual_ta) lv_textarea_set_text(uf_manual_ta, "");
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *clr_lbl = lv_label_create(btn_clr);
  lv_obj_set_style_text_font(clr_lbl, uf_lang ? &lv_font_montserrat_14 : &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(clr_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(clr_lbl, uf_ml("پاک", "Clear"));
  lv_obj_center(clr_lbl);

  // کلید بررسی و دانلود
  lv_obj_t *check = lv_button_create(uf_manual_root);
  lv_obj_set_size(check, 84, 34);
  lv_obj_set_pos(check, 390, 34);
  lv_obj_set_style_bg_color(check, UF_COL_OK, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(check, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(check, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(check, uf_manual_check_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *check_lbl = lv_label_create(check);
  lv_obj_set_style_text_font(check_lbl, uf_lang ? &lv_font_montserrat_14 : &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(check_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(check_lbl, uf_ml("بررسی", "Check"));
  lv_obj_center(check_lbl);

  uf_manual_status = uf_make_label(uf_manual_root, uf_ml("نشانی فایل را وارد کنید یا کلید پیشفرض را بزنید", "Enter link or tap Default"), uf_lang ? &lv_font_montserrat_12 : &ui_font_naz18b, UF_COL_MUTED, 8, 70, 464);
  
  uf_manual_kb = lv_keyboard_create(uf_manual_root);
  uf_apply_clean_keyboard_maps(uf_manual_kb);
  lv_keyboard_set_textarea(uf_manual_kb, uf_manual_ta);
  lv_obj_set_size(uf_manual_kb, 480, 226);
  lv_obj_align(uf_manual_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_text_font(uf_manual_kb, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(uf_manual_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(uf_manual_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(uf_manual_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(uf_manual_kb, lv_color_hex(0x0b1c33), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(uf_manual_kb, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(uf_manual_kb, lv_color_hex(0x17384d), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_manual_kb, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(uf_manual_kb, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(uf_manual_kb, uf_kb_keep_cb, LV_EVENT_CANCEL, uf_manual_kb);
  lv_obj_add_event_cb(uf_manual_kb, [](lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    }
  }, LV_EVENT_VALUE_CHANGED, NULL);
}

static void uf_manual_open_cb(lv_event_t *e) {
  (void)e;
  if (uf_flashing || uf_manual_root) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  uf_manual_open_req = true;
}

static void uf_upd_make_card(int idx, int x, const lv_image_dsc_t *img,
                             const char *fa, const char *en) {
  const int CARD_H = 209;
  const int CARD_W = 224;
  lv_obj_t *c = lv_obj_create(uf_upd_root);
  lv_obj_set_size(c, CARD_W, CARD_H);
  lv_obj_set_pos(c, x, 59);
  lv_obj_set_style_bg_color(c, lv_color_hex(0x2B2B2B), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(c, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(c, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(c, lv_color_hex(0x2a4a6e), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(c, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(c, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(c, uf_upd_card_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
  lv_obj_add_event_cb(c, [](lv_event_t *e) {
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    uf_show_card_info(id);
  }, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)idx);

  // عکس از بیک ۱۵۰px
  lv_obj_t *im = lv_image_create(c);
  lv_image_set_src(im, img);
  lv_obj_set_style_bg_opa(im, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(im, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_align(im, LV_ALIGN_TOP_MID, 0, 24);
  lv_image_set_scale(im, 256);

  lv_obj_t *title = lv_label_create(c);
  lv_obj_set_width(title, CARD_W - 16);
  lv_obj_set_style_text_font(title, uf_lang ? &lv_font_montserrat_16 : UF_FONT_FA_S, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(title, uf_ml(fa, en));
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_add_flag(title, LV_OBJ_FLAG_EVENT_BUBBLE);

  // نسخهٔ سرور زیر عنوان
  uf_upd_card_ver[idx] = lv_label_create(c);
  lv_obj_set_width(uf_upd_card_ver[idx], CARD_W - 20);
  lv_obj_set_style_text_font(uf_upd_card_ver[idx], &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(uf_upd_card_ver[idx], lv_color_hex(0x7fa8d0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(uf_upd_card_ver[idx], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(uf_upd_card_ver[idx], LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(uf_upd_card_ver[idx], "");
  lv_obj_align(uf_upd_card_ver[idx], LV_ALIGN_TOP_MID, 0, 32);
  lv_obj_add_flag(uf_upd_card_ver[idx], LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_flag(uf_upd_card_ver[idx], LV_OBJ_FLAG_HIDDEN);

  // دکمهٔ نصب
  uf_upd_card_btn[idx] = lv_button_create(c);
  lv_obj_set_size(uf_upd_card_btn[idx], CARD_W - 28, 42);
  lv_obj_set_pos(uf_upd_card_btn[idx], 14, 158);
  lv_obj_set_style_radius(uf_upd_card_btn[idx], 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(uf_upd_card_btn[idx], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(uf_upd_card_btn[idx], uf_upd_install_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
  uf_upd_card_btn_lbl[idx] = lv_label_create(uf_upd_card_btn[idx]);
  uf_card_btn_state[idx] = UF_BTN_SEARCHING;
  uf_upd_style_one_btn(idx);

  uf_upd_card[idx] = c;
}

static void uf_build_update(void) {
  uf_upd_sel = -1;
  uf_cfm_close();
  uf_card_ver[0][0] = uf_card_ver[1][0] = 0;
  uf_models_scan_done = false;
  uf_models_scanning = false;
  uf_models_scan_requested_ms = millis();
  uf_models_scan_started_ms = 0;
  uf_models_scan_session = 0;
  uf_models_scan_done_session = 0;
  uf_models_scan_timed_out = false;
  uf_upd_card_ver[0] = uf_upd_card_ver[1] = NULL;
  uf_upd_card_btn[0] = uf_upd_card_btn[1] = NULL;
  uf_upd_card_btn_lbl[0] = uf_upd_card_btn_lbl[1] = NULL;
  uf_card_btn_state[0] = uf_card_btn_state[1] = UF_BTN_READY;
  uf_upd_install = NULL;

  if (GUI.system_language == 2) uf_lang = 1;
  else if (GUI.system_language == 1) uf_lang = 0;

  uf_upd_root = uf_make_root();

  uf_upd_exit = lv_button_create(uf_upd_root);
  lv_obj_set_size(uf_upd_exit, 40, 40);
  lv_obj_set_pos(uf_upd_exit, 10, 6);
  lv_obj_set_style_bg_color(uf_upd_exit, lv_color_hex(0x1e5f9e), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(uf_upd_exit, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(uf_upd_exit, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(uf_upd_exit, uf_exit_cb, LV_EVENT_CLICKED, NULL);
  {
    lv_obj_t *hl = lv_label_create(uf_upd_exit);
    lv_obj_set_style_text_font(hl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(hl, LV_SYMBOL_HOME);
    lv_obj_center(hl);
  }

  uf_upd_title = uf_make_label(uf_upd_root,
    uf_ml("بروزرسانی نرم افزار", "Firmware update"),
    uf_lang ? &lv_font_montserrat_22 : &ui_font_naz30, UF_COL_TXT, 54, 6, 334);

  // دکمه لینک دستی بروزرسانی
  lv_obj_t *manual = lv_button_create(uf_upd_root);
  lv_obj_set_size(manual, 74, 36);
  lv_obj_set_pos(manual, 396, 8);
  lv_obj_set_style_bg_color(manual, lv_color_hex(0x2a4a6e), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(manual, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(manual, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(manual, uf_manual_open_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *manual_lbl = lv_label_create(manual);
  lv_obj_set_style_text_font(manual_lbl, uf_lang ? &lv_font_montserrat_14 : UF_FONT_FA_S,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(manual_lbl, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(manual_lbl, uf_ml("لینک", "Link"));
  lv_obj_center(manual_lbl);

  uf_upd_make_card(0, 12, &uf_img_cutter, "تک‌سر برش", "Cutter");
  uf_upd_make_card(1, 244, &uf_img_config, "کانفیگ", "Config");

  // ورژن دستگاه — همیشه LTR (فارسی و انگلیسی برعکس نشوند)؛ −۳px بالاتر
  {
    char dbuf[56];
    if (uf_lang) {
      snprintf(dbuf, sizeof(dbuf), "device version %s", PROJECT_VERSION_STRING);
      uf_upd_ver = uf_make_label(uf_upd_root, dbuf, &lv_font_montserrat_12,
                                 lv_color_hex(0x7fa8d0), 0, 269, 480);
    } else {
      // «ورژن دستگاه » + ارقام عربی‌ـهندی — base_dir = LTR
      const char *v = PROJECT_VERSION_STRING;
      int o = 0;
      const char *prefix = "ورژن دستگاه ";
      while (*prefix && o < 44) dbuf[o++] = *prefix++;
      for (int i = 0; v[i] && o < 54; i++) {
        char c = v[i];
        if (c >= '0' && c <= '9') { dbuf[o++] = (char)0xD9; dbuf[o++] = (char)(0xA0 + (c - '0')); }
        else dbuf[o++] = c;
      }
      dbuf[o] = 0;
      uf_upd_ver = uf_make_label(uf_upd_root, dbuf, UF_FONT_FA_S,
                                 lv_color_hex(0x7fa8d0), 0, 269, 480);
    }
    lv_obj_set_style_text_align(uf_upd_ver, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(uf_upd_ver, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // وضعیت: فارسی/انگلیسی −۳px دیگر
  {
    const int st_y = uf_lang ? 295 : 289;
    uf_upd_status = uf_make_label(uf_upd_root,
      uf_ml("در حال بررسی نسخه", "Checking versions..."),
      uf_lang ? &lv_font_montserrat_14 : UF_FONT_FA_S, UF_COL_TXT, 8, st_y, 464);
  }

  uf_upd_bar = lv_bar_create(uf_upd_root);
  lv_obj_set_size(uf_upd_bar, 300, 10);
  lv_obj_set_pos(uf_upd_bar, 70, 311);
  lv_bar_set_range(uf_upd_bar, 0, 100);
  lv_bar_set_value(uf_upd_bar, 0, LV_ANIM_OFF);
  lv_obj_add_flag(uf_upd_bar, LV_OBJ_FLAG_HIDDEN);
  uf_upd_pct = uf_make_label(uf_upd_root, "", &lv_font_montserrat_14, UF_COL_TXT, 380, 307, 70);
  lv_obj_add_flag(uf_upd_pct, LV_OBJ_FLAG_HIDDEN);

  // اسکن نسخهٔ هر دو برنامه (مثل InstScan در V.1.5.0)
  uf_req_scan_models = true;
}

// =============================================================================
//  ماشینِ حالت  —  از حلقهٔ Task2 (ترد LVGL) هر تکرار صدا زده می‌شود
//  (مثلِ lock_overlay_timer_cb مبدأ، ولی بدونِ lv_timer تا هیچ شیِ LVGL از
//   تردِ setup ساخته نشود → کاملاً بدونِ race).
// =============================================================================
void update_flow_start_from_ui();          // forward — برای فرمانِ دیباگ UPD2
static void uf_dbg_start_password(void);   // forward — UPD1

static void UpdateNetTask(void *pv);
static bool uf_net_task_started = false;
#define UF_NET_TASK_STACK_BYTES 8192

static inline void uf_start_net_task_once() {
  if (uf_net_task_started) return;
  // استک باید داخلی بماند: همین تسک OTA را روی فلش می‌نویسد و هنگام
  // خاموش‌شدن کش، استک PSRAM قابل اتکا نیست. 8KB برای WiFi/TLS کافی است
  // و 4KB داخلی را برای تسک و بافرهای خود درایور WiFi آزاد نگه می‌دارد.
  BaseType_t ok = xTaskCreatePinnedToCore(
      UpdateNetTask, "UpdateNet", UF_NET_TASK_STACK_BYTES, NULL, 2, NULL, 0);
  uf_net_task_started = (ok == pdPASS);
  Serial.printf("[UF] UpdateNet create=%d stack=%u heap=%u internal=%u largest=%u\n",
                (int)ok, (unsigned)UF_NET_TASK_STACK_BYTES,
                (unsigned)ESP.getFreeHeap(),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  if (!uf_net_task_started)
    uf_set_status(uf_ml("حافظه شبکه کافی نیست", "Not enough network memory"));
}

static void update_flow_tick(void) {
  // فرمان‌های دیباگِ سریال (UPD1..4 از screenshot.h) — قبل از گاردِ g_update_mode
  if (shot_uf_dbg) {
    int m = shot_uf_dbg; shot_uf_dbg = 0;
    if      (m == 1) uf_dbg_start_password();
    else if (m == 2) update_flow_start_from_ui();
    else if (m == 3 && uf_pw_ta) {   // شبیه‌سازیِ تایپِ 123
      size_t n = strlen(uf_pw_buf);
      if (n < 9) { memcpy(uf_pw_buf + n, "123", 4); }
      uf_pw_render();
    }
    else if (m == 3 && uf_wifi_root && !uf_wp_root) {   // دیباگ پنلِ رمزِ وایفا
      if (uf_wifi_list) lv_dropdown_close(uf_wifi_list);
      uf_open_wifi_pass("TestWiFi");
    }
    else if (m == 4 && uf_pw_root) { uf_lang ^= 1; uf_pw_apply_lang(); }
    else if (m == 5) {   // دیباگ: مستقیم صفحهٔ بروزرسانی (بدونِ وایفای — فقط UI)
      if (!uf_begin_update_maintenance()) return;
      uf_exit_req = false; uf_ui_set_status("");
      uf_exit_wait_network = false;
      if (uf_pw_root)   { lv_obj_delete(uf_pw_root);   uf_pw_root = NULL; uf_pw_ta = NULL; uf_pw_err = uf_pw_kb_root = NULL; }
      if (uf_wifi_root) { lv_obj_delete(uf_wifi_root); uf_wifi_root = NULL; uf_wifi_list = uf_wifi_status = NULL; }
      if (!uf_upd_root) uf_build_update();
      uf_state = UF_ST_UPDATE;
      uf_schedule_prefetch_after_wifi_handoff();
    }
  }
  if ((!g_update_mode && !uf_portal_sta_session) || !uf_mutex) return;
  // [UI] Cancel of F1 STA setup: no credentials/mode are committed and the
  // radio is returned to OFF before the overlay is removed.
  if (uf_portal_sta_session && uf_exit_req) {
    uf_exit_req = false;
    uf_advance_session();
    uf_reset_session_flags();
    WiFi.scanDelete();
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    uf_wifi_sta_ready = false;
    wifi_owner_release(WIFI_OWNER_PORTAL);
    if (uf_wp_root) { lv_obj_delete(uf_wp_root); uf_wp_root = NULL; uf_wp_ta = uf_wp_eye_lbl = uf_wp_status = uf_wp_kb = NULL; }
    if (uf_wifi_root) { lv_obj_delete(uf_wifi_root); uf_wifi_root = NULL; uf_wifi_list = uf_wifi_status = NULL; }
    uf_portal_sta_session = false;
    GUI.flag_web_portal_chip_refresh = true;
    return;
  }
  // This callback runs exclusively in Task2, the sole owner of the shared
  // touch/RTC I2C bus.  The network task only consumes the resulting flag.
  if (!uf_tls_clock_checked) {
    uf_tls_clock_ready = system_clock_sync_from_ds3231();
    uf_tls_clock_checked = true;
  }
  if (uf_pw_kb_root) {
    if (GUI.virtual_kb_enable) {
      if (lv_obj_has_flag(uf_pw_kb_root, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(uf_pw_kb_root, LV_OBJ_FLAG_HIDDEN);
    } else {
      if (!lv_obj_has_flag(uf_pw_kb_root, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(uf_pw_kb_root, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (uf_wp_kb) {
    // Wi-Fi credentials always retain their touch input, including with USB HID.
    if (lv_obj_has_flag(uf_wp_kb, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(uf_wp_kb, LV_OBJ_FLAG_HIDDEN);
  }
  if (uf_manual_kb) {
    // The session-only HTTPS source has the same touch-only availability rule.
    if (lv_obj_has_flag(uf_manual_kb, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(uf_manual_kb, LV_OBJ_FLAG_HIDDEN);
  }
  if (uf_manual_open_req) {
    uf_manual_open_req = false;
    // The Source button may be pressed while the trusted discovery request is
    // still running. Invalidate that generation before rebuilding the page so
    // its late result cannot be presented as a manual-source result.
    uf_req_scan_models = false;
    uf_advance_session();
    uf_models_scanning = false;
    uf_models_scan_done = false;
    uf_models_scan_done_session = 0;
    uf_models_scan_requested_ms = 0;
    uf_models_scan_started_ms = 0;
    if (uf_upd_root) {
      lv_obj_delete(uf_upd_root);
      uf_upd_root = NULL;
      uf_upd_status = uf_upd_ver = uf_upd_bar = uf_upd_install = uf_upd_pct = NULL;
      uf_upd_title = uf_upd_exit = NULL;
      uf_upd_card[0] = uf_upd_card[1] = NULL;
      uf_upd_card_ver[0] = uf_upd_card_ver[1] = NULL;
      uf_upd_card_btn[0] = uf_upd_card_btn[1] = NULL;
      uf_upd_card_btn_lbl[0] = uf_upd_card_btn_lbl[1] = NULL;
    }
    uf_open_manual_source();
  }
  if (uf_manual_check_req || uf_manual_back_req) {
    const bool check_manual = uf_manual_check_req;
    uf_manual_check_req = false;
    uf_manual_back_req = false;
    if (!check_manual) {
      uf_lock();
      uf_manual_scan_base[0] = 0;
      uf_manual_scan_base_active = false;
      uf_unlock();
    }
    if (uf_manual_root) {
      lv_obj_delete(uf_manual_root);
      uf_manual_root = uf_manual_ta = uf_manual_kb = uf_manual_status = NULL;
    }
    uf_build_update();
  }

  // Start the updater only after WebPortal (or stale work from the previous
  // updater session) has fully released the global WiFi driver.
  if (uf_wait_portal_stop &&
      !web_portal_running && !web_portal_transitioning &&
      !web_portal_start_request && !uf_net_operation_active) {
    if (wifi_owner_get() == WIFI_OWNER_UPDATER_PENDING)
      (void)wifi_owner_transition(WIFI_OWNER_UPDATER_PENDING, WIFI_OWNER_UPDATER);
    if (wifi_owner_get() == WIFI_OWNER_NONE)
      (void)wifi_owner_try_acquire(WIFI_OWNER_UPDATER);
    if (wifi_owner_get() == WIFI_OWNER_UPDATER) {
      web_portal_stop_request = false;
      uf_wait_portal_stop = false;
      uf_ui_set_status(uf_ml("در حال سرچ", "Searching"));
      uf_req_prefetch = true;
    }
  }

  char status_snap[192];
  uf_ui_snapshot_status(status_snap, sizeof(status_snap));

  // تخلیهٔ کاراکترهای کیبوردِ فیزیکیِ USB به کادرِ رمزِ فعال (وایفا یا مدیر) — روی ترد LVGL
  { lv_obj_t *kt = uf_wp_ta ? uf_wp_ta : uf_pw_ta;
    if (kt) {
      while (uf_kbd_head != uf_kbd_tail) {
        char c = uf_kbd_buf[uf_kbd_tail];
        uf_kbd_tail = (uint8_t)((uf_kbd_tail + 1) % sizeof(uf_kbd_buf));
        if (kt == uf_pw_ta) {   // کادرِ رمزِ مدیر از بافرِ ASCII رندر می‌شود (LTR در هر دو زبان)
          int n = (int)strlen(uf_pw_buf);
          if (c == '\b' || c == 127)       { if (n) uf_pw_buf[n - 1] = 0; uf_pw_render(); }
          else if (c == '\r' || c == '\n') uf_pw_verify();
          else if ((uint8_t)c >= 32 && n < 12) { uf_pw_buf[n] = c; uf_pw_buf[n + 1] = 0; uf_pw_render(); }
        } else {
          if (c == '\b' || c == 127)       lv_textarea_delete_char(kt);
          else if (c == '\r' || c == '\n') uf_wp_connect_cb(NULL);
          else if ((uint8_t)c >= 32)       lv_textarea_add_char(kt, (uint32_t)(uint8_t)c);
        }
      }
    }
  }

  // خروجِ بدونِ ریست: اول نشست و درخواست‌ها باطل می‌شوند. UI تا برگشت کامل
  // UpdateNetTask و تحویل مالکیت رادیو باقی می‌ماند؛ سپس حرکت دوباره آزاد می‌شود.
  if (uf_exit_req && !uf_exit_wait_network) {
    uf_exit_req = false;
    uf_exit_wait_network = true;
    uf_advance_session();        // نتیجهٔ scan/connect نشست قبلی دیگر معتبر نیست
    uf_reset_session_flags();
    web_portal_start_request = false;
    if (web_portal_running || web_portal_transitioning ||
        wifi_owner_get() == WIFI_OWNER_UPDATER_PENDING) {
      web_portal_stop_request = true;
    }
    uf_ui_set_status(uf_ml("در حال بستن امن شبکه", "Stopping WiFi safely"));
  }
  if (uf_exit_wait_network) {
    if (web_portal_running || web_portal_transitioning) {
      web_portal_stop_request = true;
      return;
    }
    if (wifi_owner_get() == WIFI_OWNER_UPDATER_PENDING)
      (void)wifi_owner_transition(WIFI_OWNER_UPDATER_PENDING, WIFI_OWNER_UPDATER);
    if (uf_net_operation_active) return;
    if (wifi_owner_get() == WIFI_OWNER_NONE)
      (void)wifi_owner_try_acquire(WIFI_OWNER_UPDATER);
    if (wifi_owner_get() != WIFI_OWNER_UPDATER) {
      uf_ui_set_status(uf_ml("منتظر آزاد شدن شبکه", "Waiting for WiFi owner"));
      return;
    }

    // g_update_mode stays true until both motion outputs and WiFi are quiescent.
    panel_request_stop_motion();
    WiFi.scanDelete();
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    uf_wifi_sta_ready = false;
    wifi_owner_release(WIFI_OWNER_UPDATER);
    web_portal_stop_request = false;
    uf_wait_portal_stop = false;
    uf_kbd_tail = uf_kbd_head;
    if (uf_wp_root)   { lv_obj_delete(uf_wp_root);   uf_wp_root = NULL; uf_wp_ta = uf_wp_eye_lbl = uf_wp_status = uf_wp_kb = NULL; }
    if (uf_manual_root) { lv_obj_delete(uf_manual_root); uf_manual_root = uf_manual_ta = uf_manual_kb = uf_manual_status = NULL; }
    if (uf_pw_root)   { lv_obj_delete(uf_pw_root);   uf_pw_root = NULL; uf_pw_ta = uf_pw_kb_root = NULL; }
    if (uf_wifi_root) { lv_obj_delete(uf_wifi_root); uf_wifi_root = NULL; uf_wifi_list = uf_wifi_status = NULL; }
    if (uf_upd_root)  { lv_obj_delete(uf_upd_root);  uf_upd_root = NULL;
                        uf_upd_status = uf_upd_ver = uf_upd_bar = uf_upd_install = uf_upd_pct = NULL;
                        uf_upd_title = uf_upd_exit = NULL;
                        uf_upd_card[0] = uf_upd_card[1] = NULL;
                        uf_upd_card_ver[0] = uf_upd_card_ver[1] = NULL;
                        uf_upd_card_btn[0] = uf_upd_card_btn[1] = NULL;
                        uf_upd_card_btn_lbl[0] = uf_upd_card_btn_lbl[1] = NULL;
                        uf_card_btn_state[0] = uf_card_btn_state[1] = UF_BTN_READY;
                        uf_upd_sel = -1; }
    uf_cfm_close();
    uf_exit_wait_network = false;
    g_update_mode = false;        // Task3/Task8 فقط بعد از safe shutdown دوباره فعال
    request_goto_screen(1);       // صفحهٔ اصلی
    return;
  }

  switch (uf_state) {
    case UF_ST_WAIT_SPLASH:
      if (lv_tick_elaps(uf_start_ms) >= UF_SPLASH_HOLD_MS) {
        uf_build_password();     // اول صفحهٔ رمزِ مدیر (نه مستقیم وایفای)
        uf_state = UF_ST_PASSWORD;
      }
      break;

    case UF_ST_PASSWORD:
      // گذارِ رمز→وایفای/آپدیت اینجا (خارج از callbackِ کیبورد) تا حذفِ overlay امن باشد
      if (uf_pw_ok) {
        uf_pw_ok = false;
        if (uf_pw_root) { lv_obj_delete(uf_pw_root); uf_pw_root = NULL; uf_pw_ta = NULL; uf_pw_err = uf_pw_kb_root = NULL; }
        // شبکه فقط بعد از رمز (نه روی صفحهٔ رمز — علت هنگ)
        uf_prefetch_done = false;
        uf_wifi_connected = false;
        uf_ssid_list[0] = 0;
        uf_start_net_task_once();
        uf_schedule_prefetch_after_wifi_handoff();
        uf_build_wifi();
        uf_state = UF_ST_WIFI;
      }
      break;

    case UF_ST_WIFI:
      // وضعیت: اگر پنلِ رمز باز است روی آن، وگرنه روی صفحهٔ لیست
      if (uf_wp_root) {
        if (uf_wp_status && status_snap[0] && uf_wp_has_status) {
          const char *cur = lv_label_get_text(uf_wp_status);
          if (!cur || strcmp(cur, status_snap) != 0) lv_label_set_text(uf_wp_status, status_snap);
        }
      } else if (uf_wifi_status && status_snap[0]) {
        const char *cur = lv_label_get_text(uf_wifi_status);
        if (!cur || strcmp(cur, status_snap) != 0) lv_label_set_text(uf_wifi_status, status_snap);
      }
      // بستنِ پنلِ رمز (دکمهٔ بازگشت)
      if (uf_wp_close_req) {
        uf_wp_close_req = false;
        if (uf_wp_root) { lv_obj_delete(uf_wp_root); uf_wp_root = NULL; uf_wp_ta = uf_wp_eye_lbl = uf_wp_status = uf_wp_kb = NULL; }
      }
      // بازکردنِ پنلِ رمزِ وایفا (انتخاب از دراپ‌داون) — اینجا امن است، نه داخلِ رویداد
      if (uf_wp_open_req && !uf_wp_root) {
        char s[48];
        if (uf_ui_try_snapshot(s, sizeof(s), uf_sel_ssid)) {
          uf_wp_open_req = false;
          uf_open_wifi_pass(s);
        }
      }
      // لیستِ تازهٔ شبکه‌ها → ساخت دکمه‌های اسکرول‌خورده شبکه‌ها
      if (uf_scan_ready && uf_wifi_list) {
        uf_scan_ready = false;
        lv_obj_clean(uf_wifi_list);
        if (uf_scan_n == 0) {
          lv_obj_t *emp = lv_label_create(uf_wifi_list);
          lv_obj_set_style_text_font(emp, UF_FONT_FA_S, 0);
          lv_obj_set_style_text_color(emp, lv_color_hex(0x8e8e93), 0);
          lv_label_set_text(emp, "شبکه‌ای یافت نشد. دکمه جستجو را بزنید.");
        } else {
          for (int i = 0; i < uf_scan_n; i++) {
            lv_obj_t *row = lv_button_create(uf_wifi_list);
            lv_obj_set_size(row, 444, 42);
            lv_obj_set_style_bg_color(row, lv_color_hex(0x16283e), 0);
            lv_obj_set_style_radius(row, 8, 0);
            lv_obj_set_style_pad_hor(row, 12, 0);
            lv_obj_set_style_pad_ver(row, 0, 0);
            lv_obj_add_event_cb(row, uf_wifi_row_click_cb, LV_EVENT_CLICKED, (void *)uf_scan_ssid[i]);

            lv_obj_t *wico = lv_label_create(row);
            lv_obj_set_style_text_font(wico, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(wico, lv_color_hex(0x0a84ff), 0);
            lv_label_set_text(wico, LV_SYMBOL_WIFI);
            lv_obj_align(wico, LV_ALIGN_LEFT_MID, 0, 0);

            lv_obj_t *slbl = lv_label_create(row);
            lv_obj_set_style_text_font(slbl, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(slbl, lv_color_hex(0xffffff), 0);
            lv_label_set_text(slbl, uf_scan_ssid[i]);
            lv_obj_align(slbl, LV_ALIGN_LEFT_MID, 32, 0);

            lv_obj_t *arr = lv_label_create(row);
            lv_obj_set_style_text_font(arr, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(arr, lv_color_hex(0x64748b), 0);
            lv_label_set_text(arr, LV_SYMBOL_RIGHT);
            lv_obj_align(arr, LV_ALIGN_RIGHT_MID, 0, 0);
          }
        }
      }
      if (uf_wifi_connected && uf_portal_sta_session) {
        // Credentials were written only by uf_net_connect after WL_CONNECTED.
        // Hand the already-connected STA radio back to the portal; no OTA page.
        if (uf_wp_root)   { lv_obj_delete(uf_wp_root);   uf_wp_root = NULL; uf_wp_ta = uf_wp_eye_lbl = uf_wp_status = uf_wp_kb = NULL; }
        if (uf_wifi_root) { lv_obj_delete(uf_wifi_root); uf_wifi_root = NULL; uf_wifi_list = uf_wifi_status = NULL; }
        uf_portal_sta_session = false;
        web_portal_start_sta_on_connected();
        return;
      }
      if (uf_wifi_connected) {   // اتصال موفق → صفحهٔ بروزرسانی
        if (uf_wp_root)   { lv_obj_delete(uf_wp_root);   uf_wp_root = NULL; uf_wp_ta = uf_wp_eye_lbl = uf_wp_status = uf_wp_kb = NULL; }
        if (uf_wifi_root) { lv_obj_delete(uf_wifi_root); uf_wifi_root = NULL; uf_wifi_list = uf_wifi_status = NULL; }
        uf_build_update();
        uf_state = UF_ST_UPDATE;   // چکِ نسخه بعد از انتخابِ کارتِ دستگاه
      }
      break;

    case UF_ST_UPDATE:
      if (uf_upd_status && status_snap[0]) {
        // ارقامِ ASCII در فونتِ naz مربع می‌شوند → تبدیل به عربی‌ـهندی (فقط فارسی)
        if (uf_lang) {
          lv_label_set_text(uf_upd_status, status_snap);
        } else {
          char fa[192]; int o = 0;
          for (int i = 0; status_snap[i] && o < 188; i++) {
            char c = status_snap[i];
            if (c >= '0' && c <= '9') { fa[o++] = (char)0xD9; fa[o++] = (char)(0xA0 + (c - '0')); }
            else fa[o++] = c;
          }
          fa[o] = 0;
          lv_label_set_text(uf_upd_status, fa);
        }
      }
      // اسکن نسخهٔ دو کارت تمام شد → نسخه فقط اگر واقعی؛ دکمه با متن نهایی
      if (!uf_models_scan_timed_out && uf_models_scan_requested_ms != 0 &&
          (uint32_t)(millis() - uf_models_scan_requested_ms) >= 15000UL) {
        // Cancel only this discovery generation.  The network task observes the
        // new session before publishing any card result; OTA/flash is untouched.
        uf_req_scan_models = false;
        uf_advance_session();
        uf_models_scanning = false;
        uf_models_scan_done = false;
        uf_models_scan_timed_out = true;
        if (uf_upd_root) {
          for (int i = 0; i < 2; i++) {
            uf_card_btn_state[i] = UF_BTN_MISSING;
            uf_upd_style_one_btn(i);
          }
          uf_set_status(uf_ml("سرور در دسترس نیست یا اینترنت قطع است", "Server unavailable or offline"));
        }
        break;
      }
      if (uf_models_scan_done && uf_models_scan_done_session == uf_session && !uf_models_scan_timed_out) {
        uf_models_scan_done = false;
        bool any_available = false;
        for (int i = 0; i < 2; i++) {
          char snap[24];
          uf_lock();
          strncpy(snap, uf_card_ver[i], sizeof(snap) - 1);
          snap[sizeof(snap) - 1] = 0;
          uf_unlock();
          if (!snap[0]) { snap[0] = '?'; snap[1] = 0; }
          uf_card_btn_state[i] = uf_card_state_from_ver(snap);
          if (uf_card_btn_state[i] != UF_BTN_MISSING) any_available = true;
          if (uf_upd_card_ver[i]) {
            // فقط نسخهٔ واقعی سرور — نه "?" / "..." / خالی (مربع نشود)
            if (!uf_ver_is_missing(snap) && strcmp(snap, "READY") != 0) {
              lv_label_set_text(uf_upd_card_ver[i], snap);
              lv_obj_clear_flag(uf_upd_card_ver[i], LV_OBJ_FLAG_HIDDEN);
            } else {
              lv_label_set_text(uf_upd_card_ver[i], "");
              lv_obj_add_flag(uf_upd_card_ver[i], LV_OBJ_FLAG_HIDDEN);
            }
          }
        }
        bool manual_source = false;
        uf_lock(); manual_source = uf_manual_scan_base_active; uf_unlock();
        if (!any_available) {
          uf_set_status(uf_ml(
            "ارتباط با سرور موقتاً قطع است. لطفاً در زمان دیگر تلاش کنید.",
            "Server is temporarily unavailable. Please try again later."));
          if (uf_upd_ver) lv_obj_add_flag(uf_upd_ver, LV_OBJ_FLAG_HIDDEN);
          if (uf_upd_status) {
            lv_obj_set_y(uf_upd_status, uf_lang ? 283 : 275);  // −۳px
            lv_obj_set_style_text_color(uf_upd_status, lv_color_hex(0xAEAEB2), LV_PART_MAIN | LV_STATE_DEFAULT);
          }
        } else {
          uf_set_status(manual_source
            ? uf_ml("منبع دستی بررسی شد؛ نصب فقط از منبع تاییدشده است", "Manual source checked; install uses approved source")
            : uf_ml("برنامه را انتخاب و نصب کنید", "Select a program and install"));
          if (uf_upd_ver) lv_obj_clear_flag(uf_upd_ver, LV_OBJ_FLAG_HIDDEN);
          if (uf_upd_status) {
            lv_obj_set_y(uf_upd_status, uf_lang ? 295 : 289);  // −۳px
            lv_obj_set_style_text_color(uf_upd_status, UF_COL_TXT, LV_PART_MAIN | LV_STATE_DEFAULT);
          }
        }
        if (!uf_flashing) {
          uf_upd_set_install_enabled(!manual_source);
          if (uf_upd_exit) lv_obj_clear_flag(uf_upd_exit, LV_OBJ_FLAG_HIDDEN);
        }
      }
      if (uf_flashing && uf_ota_pct >= 0 && uf_upd_bar) {   // درصدِ دانلود
        if (uf_upd_exit) lv_obj_add_flag(uf_upd_exit, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(uf_upd_bar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(uf_upd_bar, uf_ota_pct, LV_ANIM_OFF);
        if (uf_upd_pct) {
          lv_obj_clear_flag(uf_upd_pct, LV_OBJ_FLAG_HIDDEN);
          char b[16]; snprintf(b, sizeof(b), "%d%%", uf_ota_pct);
          lv_label_set_text(uf_upd_pct, b);
        }
      }
      if (uf_check_done) {
        uf_check_done = false;  // مسیر قدیمی تک‌مدل — نسخه روی کارت‌ها از scan_models می‌آید
      }
      if (uf_flash_failed) {
        uf_flash_failed = false;
        if (uf_upd_bar) lv_obj_add_flag(uf_upd_bar, LV_OBJ_FLAG_HIDDEN);
        if (uf_upd_pct) lv_obj_add_flag(uf_upd_pct, LV_OBJ_FLAG_HIDDEN);
        uf_upd_set_install_enabled(true);
        if (uf_upd_exit) lv_obj_clear_flag(uf_upd_exit, LV_OBJ_FLAG_HIDDEN);
      }
      break;
  }
}

// =============================================================================
//  تسکِ شبکه  (اسکن/اتصالِ وایفای + چکِ نسخه + httpUpdate)
// =============================================================================
static String uf_base_url() {
  String b = UF_OTA_BASE_URL; b.trim();
  while (b.endsWith("/")) b.remove(b.length() - 1);
  return b;
}
static String uf_scan_base_url() {
  char snap[sizeof(uf_manual_scan_base)];
  bool manual = false;
  uf_lock();
  manual = uf_manual_scan_base_active;
  strncpy(snap, uf_manual_scan_base, sizeof(snap) - 1);
  snap[sizeof(snap) - 1] = 0;
  uf_unlock();
  if (!manual) return uf_base_url();
  String b = snap;
  while (b.endsWith("/")) b.remove(b.length() - 1);
  return b;
}
static String uf_model_url_part() {
  char msnap[64];
  uf_lock(); strncpy(msnap, uf_model, sizeof(msnap) - 1); msnap[sizeof(msnap) - 1] = 0; uf_unlock();
  String m = msnap; m.trim();
  while (m.startsWith("/")) m.remove(0, 1);
  String b = uf_base_url();
  // همان نرمال‌سازیِ مبدأ: اگر base با /plc_hmi تمام و model با plc_hmi/ شروع شد
  if (b.endsWith("/plc_hmi") && m.startsWith("plc_hmi/")) m.remove(0, 8);
  return m;
}

static bool uf_selected_model_is_config() {
  const String model = uf_model_url_part();
  return model.endsWith("firmware_config");
}

static String uf_compiled_version_url() {
  return String(uf_selected_model_is_config() ? UF_OTA_CONFIG_VERSION_URL
                                               : UF_OTA_CUTTER_VERSION_URL);
}

static String uf_compiled_firmware_url() {
  return String(uf_selected_model_is_config() ? UF_OTA_CONFIG_FIRMWARE_URL
                                               : UF_OTA_CUTTER_FIRMWARE_URL);
}

static void uf_sanitize_ssid_display(const char *in, char *out, size_t out_sz) {
  if (!in || !out || out_sz == 0) return;
  size_t w = 0;
  bool last_was_space = false;
  for (size_t r = 0; in[r] != '\0' && w + 1 < out_sz;) {
    uint8_t c = (uint8_t)in[r];
    if (c >= 0x20 && c <= 0x7E) {
      if (c == ' ') {
        if (!last_was_space && w > 0) {
          out[w++] = ' ';
          last_was_space = true;
        }
      } else {
        out[w++] = (char)c;
        last_was_space = false;
      }
      r++;
    } else if ((c & 0x80) == 0) {
      r++;
    } else if ((c & 0xE0) == 0xC0) {
      r += (in[r + 1] ? 2 : 1);
    } else if ((c & 0xF0) == 0xE0) {
      r++; if (in[r]) r++; if (in[r]) r++;
    } else if ((c & 0xF8) == 0xF0) {
      r++; if (in[r]) r++; if (in[r]) r++; if (in[r]) r++;
    } else {
      r++;
    }
  }
  while (w > 0 && out[w - 1] == ' ') w--;
  out[w] = '\0';
  if (w == 0) {
    strncpy(out, "WiFi-Network", out_sz - 1);
    out[out_sz - 1] = '\0';
  }
}

static void uf_net_scan() {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return;
  uf_scanning = true;
  uf_set_status("در حال سرچ");
  WiFi.persistent(false);
  WiFi.setSleep(false);

  // STA قبلاً در UpdateNetTask آماده شده است. خاموش/روشن‌کردن کامل درایور
  // اینجا هم حافظه را تکه‌تکه می‌کرد و هم بعد از begin نیمه‌کاره اسکن را می‌کشت.
  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  vTaskDelay(pdMS_TO_TICKS(220));
  if (!uf_session_current(session)) return;
  if (WiFi.getMode() != WIFI_MODE_STA && !WiFi.mode(WIFI_STA)) {
    Serial.printf("[UF] scan abort: STA mode failed internal=%u largest=%u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    if (!uf_session_current(session)) return;
    uf_lock();
    strncpy(uf_ssid_list, "(WiFi unavailable)", sizeof(uf_ssid_list) - 1);
    uf_ssid_list[sizeof(uf_ssid_list) - 1] = '\0';
    uf_unlock();
    uf_scan_n = 0;
    uf_scan_ready = true;
    uf_scanning = false;
    uf_set_status(uf_ml("حافظه داخلی وایفای کافی نیست", "WiFi internal memory unavailable"));
    return;
  }
  WiFi.setSleep(false);
  vTaskDelay(pdMS_TO_TICKS(250));
  uf_wifi_sta_ready = true;

  // Retry without deinitializing the driver. Repeated OFF/ON cycles fragment
  // internal WiFi memory and can leave scan/begin in a half-started state.
  int n = WIFI_SCAN_FAILED;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (!uf_session_current(session)) {
      WiFi.scanDelete();
      return;
    }
    if (attempt > 0) {
      WiFi.scanDelete();
      WiFi.disconnect(false, false);
      vTaskDelay(pdMS_TO_TICKS(220));
      if (!uf_session_current(session)) return;
    }
    // اسکن کامل فقط در حالت خالص STA انجام می‌شود؛ AP هم‌زمان نتیجه را به ۲-۳ شبکه می‌رساند
    if (WiFi.getMode() != WIFI_MODE_STA) {
      if (!WiFi.mode(WIFI_STA)) break;
      WiFi.setSleep(false);
      vTaskDelay(pdMS_TO_TICKS(250));
    }
    n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false, /*passive=*/false, /*ms_per_chan=*/120);
    Serial.printf("[UF] scan#%d n=%d mode=%d st=%d heap=%u\n",
                  attempt, n, (int)WiFi.getMode(), (int)WiFi.status(), (unsigned)ESP.getFreeHeap());
    if (n > 0) break;
    vTaskDelay(pdMS_TO_TICKS(150));
  }

  if (!uf_session_current(session)) {
    WiFi.scanDelete();
    return;
  }

  char list[UF_SSID_LIST_CAP] = {0};
  size_t list_used = 0;
  int added = 0;
  uf_scan_n = 0;
  for (int i = 0; i < n && added < UF_SCAN_MAX; i++) {
    String s = WiFi.SSID(i);
    Serial.printf("[UF]  ssid[%d]=%s rssi=%d ch=%d\n", i, s.c_str(), (int)WiFi.RSSI(i), (int)WiFi.channel(i));
    if (s.length() == 0 || s.length() > 32) continue;
    // تکراری با همان نام را رد کن (اولین = قوی‌تر معمولاً)
    bool dup = false;
    for (int k = 0; k < uf_scan_n; k++) {
      if (s == uf_scan_ssid[k]) { dup = true; break; }
    }
    if (dup) continue;
    strncpy(uf_scan_ssid[uf_scan_n], s.c_str(), 32);
    uf_scan_ssid[uf_scan_n][32] = 0;
    const uint8_t *b = WiFi.BSSID(i);
    if (b) memcpy(uf_scan_bssid[uf_scan_n], b, 6);
    else   memset(uf_scan_bssid[uf_scan_n], 0, 6);
    uf_scan_chan[uf_scan_n] = WiFi.channel(i);
    char clean_name[40] = {0};
    uf_sanitize_ssid_display(s.c_str(), clean_name, sizeof(clean_name));
    const size_t c_len = strlen(clean_name);
    if (added && list_used + 1U < sizeof(list)) list[list_used++] = '\n';
    const size_t room = sizeof(list) - list_used - 1U;
    const size_t copy_n = min(c_len, room);
    memcpy(&list[list_used], clean_name, copy_n);
    list_used += copy_n;
    list[list_used] = '\0';
    uf_scan_n++;
    added++;
  }
  WiFi.scanDelete();  // نتایج را آزاد کن تا begin بعدی گیر نکند

  if (!uf_session_current(session)) return;
  if (added == 0) {
    strncpy(list, "(no networks)", sizeof(list) - 1);
    list[sizeof(list) - 1] = '\0';
  }
  uf_lock();
  strncpy(uf_ssid_list, list, sizeof(uf_ssid_list) - 1);
  uf_ssid_list[sizeof(uf_ssid_list) - 1] = '\0';
  uf_unlock();
  uf_scan_ready = true;
  uf_scanning = false;
  if (added > 0) uf_set_status("وارد لیست وایفا شوید");
  else           uf_set_status("شبکه ای یافت نشد");
}

static void uf_trim_inplace(char *s) {
  if (!s) return;
  char *a = s;
  while (*a == ' ' || *a == '\t' || *a == '\r' || *a == '\n') a++;
  if (a != s) memmove(s, a, strlen(a) + 1);
  size_t n = strlen(s);
  while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) {
    s[--n] = 0;
  }
}

static int uf_scan_find_ssid(const char *ss) {
  if (!ss || !ss[0]) return -1;
  for (int i = 0; i < uf_scan_n; i++) {
    if (strcmp(uf_scan_ssid[i], ss) == 0) return i;
  }
  return -1;
}

static void uf_set_connect_failure_status(wl_status_t st) {
  if (st == WL_NO_SSID_AVAIL) uf_set_status(uf_ml("شبکه پیدا نشد؛ هات‌اسپات/مودم را روشن کنید", "SSID not found; turn on hotspot/router"));
  else if (st == WL_CONNECT_FAILED) uf_set_status(uf_ml("رمز یا نوع امنیت شبکه پذیرفته نشد", "Password or WiFi security was rejected"));
  else if (st == WL_CONNECTION_LOST) uf_set_status(uf_ml("سیگنال شبکه قطع شد؛ نزدیک‌تر شوید", "WiFi signal was lost; move closer"));
  else uf_set_status(uf_ml("اتصال زمان‌دار تمام شد؛ DHCP/سیگنال مودم را بررسی کنید", "Connection timed out; check DHCP/router signal"));
}

static bool uf_net_connect_once(const char *ss, const char *pw, int max_wait_slots,
                                bool use_scan_anchor = true) {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return false;
  // قبل از begin: اسکن باقی‌مانده را پاک کن و STA را جدا کن
  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  vTaskDelay(pdMS_TO_TICKS(250));
  if (!uf_session_current(session)) return false;

  int idx = uf_scan_find_ssid(ss);
  if (use_scan_anchor && idx >= 0 && uf_scan_chan[idx] > 0) {
    // اتصال با کانال + BSSID — پایدارتر بعد از اسکن
    WiFi.begin(ss, (pw && pw[0]) ? pw : nullptr,
               (int32_t)uf_scan_chan[idx], uf_scan_bssid[idx], true);
    Serial.printf("[UF] begin ch=%ld\n", (long)uf_scan_chan[idx]);
  } else {
    WiFi.begin(ss, (pw && pw[0]) ? pw : nullptr);
    Serial.println("[UF] begin plain");
  }

  for (int i = 0; i < max_wait_slots; i++) {
    if (!uf_session_current(session)) {
      WiFi.disconnect(false, false);
      return false;
    }
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) return true;
    if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
      Serial.printf("[UF] connect abort st=%d at slot=%d\n", (int)st, i);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
  return WiFi.status() == WL_CONNECTED;
}

static void uf_get_persisted_wifi(String &ss, String &pw) {
  Preferences p;
  p.begin("params", true);
  ss = p.getString("wifi_ssid", "");
  pw = p.getString("wifi_pass", "");
  p.end();
  if (ss.length() == 0) {
    p.begin("chone", true);
    ss = p.getString("ws0", "");
    pw = p.getString("wp0", "");
    p.end();
    if (ss.length() > 0) {
      // Sync into params namespace for future fast lookup
      p.begin("params", false);
      p.putString("wifi_ssid", ss);
      p.putString("wifi_pass", pw);
      p.end();
    }
  }
}

static void uf_save_persisted_wifi(const char *ss, const char *pw) {
  if (!ss || !ss[0]) return;
  Preferences p;
  // 1. Save to Cutter "params"
  p.begin("params", false);
  p.putString("wifi_ssid", String(ss));
  p.putString("wifi_pass", String(pw ? pw : ""));
  p.end();
  // 2. Save to config-test / Welding "chone"
  p.begin("chone", false);
  p.putString("ws0", String(ss));
  p.putString("wp0", String(pw ? pw : ""));
  p.putInt("wc", 1);
  p.end();
}

static bool uf_net_connect_saved() {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return false;
  String ss, pw;
  uf_get_persisted_wifi(ss, pw);
  if (ss.length() == 0) return false;

  uf_connecting = true;
  uf_set_status("اتصال به شبکه ذخیره شده");
  uf_wifi_ensure_sta();
  WiFi.setSleep(false);

  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ss) {
    uf_lock();
    strncpy(uf_sel_ssid, ss.c_str(), sizeof(uf_sel_ssid) - 1); uf_sel_ssid[sizeof(uf_sel_ssid) - 1] = 0;
    uf_unlock();
    uf_wifi_connected = true;
    uf_connecting = false;
    return true;
  }

  // فقط یک تلاش کوتاه — prefetch نباید UI را قفل کند
  // Saved SSIDs must roam: never reuse a prior scan BSSID/channel.
  bool ok = uf_net_connect_once(ss.c_str(), pw.c_str(), 24, false);  // ~6s
  if (!uf_session_current(session)) return false;
  Serial.printf("[UF] saved connect ok=%d st=%d\n", (int)ok, (int)WiFi.status());
  uf_connecting = false;
  if (ok) {
    uf_lock();
    strncpy(uf_sel_ssid, ss.c_str(), sizeof(uf_sel_ssid) - 1); uf_sel_ssid[sizeof(uf_sel_ssid) - 1] = 0;
    uf_unlock();
    uf_wifi_connected = true;
    return true;
  }
  WiFi.disconnect(false, false);
  vTaskDelay(pdMS_TO_TICKS(150));
  return false;
}

static void uf_net_connect() {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return;
  char ss[48], pw[72];
  uf_lock();
  strncpy(ss, uf_sel_ssid, sizeof(ss) - 1); ss[sizeof(ss) - 1] = 0;
  strncpy(pw, uf_sel_pass, sizeof(pw) - 1); pw[sizeof(pw) - 1] = 0;
  uf_unlock();
  uf_trim_inplace(ss);
  uf_trim_inplace(pw);
  uf_connecting = true;
  uf_connect_failed = false;
  uf_set_status("در حال اتصال");
  uf_wifi_ensure_sta();
  WiFi.setSleep(false);

  bool ok = false;
  for (int try_n = 0; try_n < 3 && !ok; try_n++) {
    if (!uf_session_current(session)) return;
    uf_set_status(try_n == 0 ? "در حال اتصال" : "تلاش مجدد اتصال");
    // تلاش اول: با کانال/BSSID؛ تلاش‌های بعد: plain (اگر کانال عوض شده باشد)
    if (try_n > 0) {
      for (int k = 0; k < uf_scan_n; k++) uf_scan_chan[k] = 0;  // force plain begin
    }
    ok = uf_net_connect_once(ss, pw, 48, try_n == 0);  // ~12s هر تلاش
    if (!uf_session_current(session)) return;
    Serial.printf("[UF] manual connect try=%d ssid=%s ok=%d st=%d\n",
                  try_n, ss, (int)ok, (int)WiFi.status());
    if (!ok) {
      WiFi.disconnect(false, false);
      vTaskDelay(pdMS_TO_TICKS(400));
    }
  }
  uf_connecting = false;
  if (ok) {
    uf_save_persisted_wifi(ss, pw);
    uf_wifi_connected = true;
    uf_set_status("متصل شد");
  } else {
    uf_connect_failed = true;
    wl_status_t st = WiFi.status();
    uf_set_connect_failure_status(st);
  }
}

static void uf_net_prefetch() {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return;
  // ریشهٔ هنگ: قبلاً اول تا ۱۴ثانیه وصلِ ذخیره‌شده می‌کرد و UI روی «در حال سرچ» می‌ماند.
  // حالا اول اسکن → لیست سریع؛ بعد اگر لیست آماده بود یک وصل کوتاه به ذخیره‌شده.
  Serial.println("[UF] prefetch start (scan-first)");
  uf_wifi_ensure_sta();
  uf_net_scan();
  if (!uf_session_current(session)) return;
  uf_prefetch_done = true;

  String ss, pw;
  uf_get_persisted_wifi(ss, pw);
  if (ss.length() && uf_scan_find_ssid(ss.c_str()) >= 0 && !uf_wifi_connected) {
    uf_lock();
    strncpy(uf_sel_ssid, ss.c_str(), sizeof(uf_sel_ssid) - 1); uf_sel_ssid[sizeof(uf_sel_ssid) - 1] = 0;
    strncpy(uf_sel_pass, pw.c_str(), sizeof(uf_sel_pass) - 1); uf_sel_pass[sizeof(uf_sel_pass) - 1] = 0;
    uf_unlock();
    uf_connecting = true;
    uf_set_status("اتصال به شبکه ذخیره شده");
    bool ok = uf_net_connect_once(ss.c_str(), pw.c_str(), 28, false);
    if (!uf_session_current(session)) return;
    uf_connecting = false;
    if (ok) {
      uf_wifi_connected = true;
      uf_set_status("متصل شد");
      Serial.println("[UF] prefetch: saved WiFi connected after scan");
    } else {
      WiFi.disconnect(false, false);
      uf_set_connect_failure_status(WiFi.status());
      Serial.println("[UF] prefetch: saved connect failed — list ready");
    }
  }
  Serial.println("[UF] prefetch: done");
}

static void uf_net_check_version() {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return;
  if (!uf_tls_prepare()) { uf_check_done = true; return; }
  String versionUrl = uf_compiled_version_url();
  uf_set_status(uf_ml("در حال بررسی نسخه", "Checking versions..."));

  WiFiClientSecure sc;
  uf_tls_configure(sc, 15);
  HTTPClient http;
  http.begin(sc, versionUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);
  int code = http.GET();
  if (!uf_session_current(session)) {
    http.end();
    return;
  }
  if (code == 200) {
    String v = http.getString(); v.trim();
    if (v.length() == 0 || v.length() > 20) v = "?";
    uf_lock();
    strncpy(uf_server_ver, v.c_str(), sizeof(uf_server_ver) - 1); uf_server_ver[sizeof(uf_server_ver) - 1] = 0;
    uf_unlock();
    uf_set_status(uf_ml("آماده برای نصب", "Ready to install"));
  } else {
    uf_lock();
    strncpy(uf_server_ver, "?", sizeof(uf_server_ver) - 1);
    uf_unlock();
    uf_set_status(uf_ml("نسخه خوانده نشد. نصب ممکن است", "Version unknown. Install still possible"));
  }
  http.end();
  if (!uf_session_current(session)) return;
  uf_check_done = true;
}

// اسکن نسخهٔ هر دو برنامه روی سرور (الگوی InstScan در V.1.5.0)
static void uf_net_scan_models() {
  const uint32_t session = uf_session;
  if (!uf_session_current(session)) return;
  uf_models_scan_session = session;
  uf_models_scan_started_ms = millis();
  uf_models_scan_timed_out = false;
  if (!uf_tls_prepare()) {
    if (uf_session_current(session)) {
      uf_models_scanning = false;
      uf_models_scan_done_session = session;
      uf_models_scan_done = true;
    }
    return;
  }
  static const char *models[2] = {
    "upvc/firmware_digital_cutter_single_head",
    "upvc/firmware_config",
  };
  static const char *version_urls[2] = {
    UF_OTA_CUTTER_VERSION_URL,
    UF_OTA_CONFIG_VERSION_URL,
  };
  static const char *firmware_urls[2] = {
    UF_OTA_CUTTER_FIRMWARE_URL,
    UF_OTA_CONFIG_FIRMWARE_URL,
  };
  uf_models_scanning = true;
  uf_models_scan_done = false;
  uf_set_status(uf_ml("در حال بررسی نسخه", "Checking versions..."));
  String base = uf_scan_base_url();
  bool manual_source = false;
  uf_lock(); manual_source = uf_manual_scan_base_active; uf_unlock();
  for (int i = 0; i < 2; i++) {
    if (!uf_session_current(session)) return;
    String versionUrl = manual_source
      ? (base.endsWith(".bin") ? base : base + "/" + models[i] + "/lastfile")
      : String(version_urls[i]);
    char ver[24] = "?";
    WiFiClientSecure sc;
    WiFiClient plain;
    HTTPClient http;
    if (versionUrl.startsWith("https://")) {
      uf_tls_configure(sc, 15);
      http.begin(sc, versionUrl);
    } else {
      plain.setTimeout(15);
      http.begin(plain, versionUrl);
    }
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(8000);
    int code = http.GET();
    if (code == 200) {
      String raw = http.getString();
      char desc_buf[512] = {0};
      int desc_len = 0;
      char found_ver[32] = {0};
      int line_start = 0;
      while (line_start < raw.length()) {
        int line_end = raw.indexOf('\n', line_start);
        if (line_end < 0) line_end = raw.length();
        String line = raw.substring(line_start, line_end);
        line.trim();
        if (line.startsWith("display=") && found_ver[0] == 0) {
          strncpy(found_ver, line.substring(8).c_str(), sizeof(found_ver) - 1);
        } else if (line.startsWith("code=") && found_ver[0] == 0) {
          strncpy(found_ver, line.substring(5).c_str(), sizeof(found_ver) - 1);
        } else if (line.startsWith("line")) {
          int eq = line.indexOf('=');
          if (eq > 0) {
            String val = line.substring(eq + 1);
            if (desc_len + val.length() + 4 < sizeof(desc_buf)) {
              if (desc_len > 0) desc_buf[desc_len++] = '\n';
              memcpy(&desc_buf[desc_len], val.c_str(), val.length());
              desc_len += val.length();
              desc_buf[desc_len] = 0;
            }
          }
        }
        line_start = line_end + 1;
      }
      if (found_ver[0] != 0) {
        strncpy(ver, found_ver, sizeof(ver) - 1);
      } else {
        String v = raw; v.trim();
        if (v.length() > 0 && v.length() <= 20 && v != "0.0.0") {
          strncpy(ver, v.c_str(), sizeof(ver) - 1);
        }
      }
      uf_lock();
      if (desc_buf[0] != 0) {
        strncpy(uf_lastfile_desc[i], desc_buf, sizeof(uf_lastfile_desc[i]) - 1);
        uf_lastfile_desc[i][sizeof(uf_lastfile_desc[i]) - 1] = 0;
      }
      uf_unlock();
    } else {
      // fallback: اگر lastfile نبود ولی bin هست
      http.end();
      String binUrl = manual_source
        ? (base.endsWith(".bin") ? base : base + "/" + models[i] + "/firmware.bin")
        : String(firmware_urls[i]);
      WiFiClientSecure sc2;
      WiFiClient plain2;
      HTTPClient http2;
      if (binUrl.startsWith("https://")) {
        uf_tls_configure(sc2, 15);
        http2.begin(sc2, binUrl);
      } else {
        plain2.setTimeout(15);
        http2.begin(plain2, binUrl);
      }
      http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      http2.setTimeout(8000);
      int c2 = http2.sendRequest("HEAD");
      if (c2 == 405 || c2 == 403) c2 = http2.GET();
      http2.end();
      if (!uf_session_current(session)) return;
      if (c2 == 200) strncpy(ver, "READY", sizeof(ver) - 1);
      uf_lock();
      strncpy(uf_card_ver[i], ver, sizeof(uf_card_ver[i]) - 1);
      uf_card_ver[i][sizeof(uf_card_ver[i]) - 1] = 0;
      uf_unlock();
      Serial.printf("[UF] model[%d] ver=%s (fallback)\n", i, ver);
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    http.end();
    if (!uf_session_current(session)) return;
    uf_lock();
    strncpy(uf_card_ver[i], ver, sizeof(uf_card_ver[i]) - 1);
    uf_card_ver[i][sizeof(uf_card_ver[i]) - 1] = 0;
    uf_unlock();
    Serial.printf("[UF] model[%d] ver=%s\n", i, ver);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  if (!uf_session_current(session)) return;
  uf_models_scanning = false;
  uf_models_scan_done_session = session;
  uf_models_scan_done = true;
  uf_set_status(uf_ml("در حال بررسی نسخه", "Checking versions..."));
}

static void uf_ota_progress_cb(int cur, int total) {
  uf_ota_pct = (total > 0) ? (int)((int64_t)cur * 100 / total) : 0;
}

static void uf_net_flash() {
  if (!uf_tls_prepare()) { uf_flash_failed = true; return; }
  String binUrl = uf_compiled_firmware_url();
  uf_flashing = true;
  uf_ota_pct = -1;
  uf_set_status(uf_ml("در حال اتصال به سرور", "Connecting to server"));
  WiFi.setSleep(false);

  const bool is_https = binUrl.startsWith("https://");

  // ۱) چک دسترسی سرور
  {
    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (is_https) {
      WiFiClientSecure probe;
      uf_tls_configure(probe, 10);
      http.begin(probe, binUrl);
    } else {
      WiFiClient probe;
      probe.setTimeout(10);
      http.begin(probe, binUrl);
    }
    int code = http.GET();
    int len  = http.getSize();
    http.end();
    if (code != 200 || len == 0) {
      char m[96];
      if (uf_lang) snprintf(m, sizeof(m), "Download failed (%d)", code);
      else         snprintf(m, sizeof(m), "خطا: دانلود نشد (%d). رها شد", code);
      uf_set_status(m);
      uf_flashing = false; uf_flash_failed = true;
      return;
    }
  }

  // ۲) سرور در دسترس → دانلود و نصب
  uf_set_status(uf_ml("در حال دانلود", "Downloading"));
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.onProgress(uf_ota_progress_cb);

  t_httpUpdate_return ret;
  if (is_https) {
    WiFiClientSecure binClient;
    uf_tls_configure(binClient, 20);
    ret = httpUpdate.update(binClient, binUrl);
  } else {
    WiFiClient binClient;
    binClient.setTimeout(20);
    ret = httpUpdate.update(binClient, binUrl);
  }

  if (ret == HTTP_UPDATE_OK) {
    uf_set_status(uf_ml("نصب کامل شد. راه اندازی مجدد", "Done. Rebooting"));
    delay(500);
    ESP.restart();
    return;   // نمی‌رسد
  }
  {
    char msg[128];
    if (uf_lang) snprintf(msg, sizeof(msg), "Install error %d. Try again", (int)httpUpdate.getLastError());
    else         snprintf(msg, sizeof(msg), "خطا در نصب %d. دوباره تلاش کنید", (int)httpUpdate.getLastError());
    uf_set_status(msg);
  }
  uf_flashing = false;
  uf_flash_failed = true;
}

static bool uf_wifi_ensure_sta(void) {
  wifi_mode_t m = WiFi.getMode();
  // اگر واقعاً STA هستیم و قبلاً آماده شده — برگرد
  if (uf_wifi_sta_ready && m == WIFI_MODE_STA) {
    WiFi.setSleep(false);
    return true;
  }
  WiFi.persistent(false);
  vTaskDelay(pdMS_TO_TICKS(50));
  // softAPdisconnect روی درایورِ خاموش، wifi را با AP بی‌کانفیگ start می‌کند →
  // کرشِ LoadProhibited در wifi_softap_start (ppTask هستهٔ ۰). فقط اگر AP روشن است قطع کن.
  m = WiFi.getMode();
  if (m == WIFI_MODE_AP || m == WIFI_MODE_APSTA) {
    WiFi.softAPdisconnect(false);
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (!WiFi.mode(WIFI_STA)) {
    uf_wifi_sta_ready = false;
    Serial.printf("[UF] STA init failed internal=%u largest=%u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    return false;
  }
  WiFi.setSleep(false);
  vTaskDelay(pdMS_TO_TICKS(200));
  WiFi.disconnect(false, false);
  vTaskDelay(pdMS_TO_TICKS(250));
  uf_wifi_sta_ready = true;
  return true;
}

static void UpdateNetTask(void *pv) {
  (void)pv;
  for (;;) {
    const uint32_t session = uf_session;
    const bool need_sta =
        uf_req_scan || uf_req_connect || uf_req_check || uf_req_flash ||
        uf_req_scan_models || uf_req_prefetch;
    if (!need_sta) {
      uf_net_operation_active = false;
      vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }
    const uint8_t expected_owner = uf_portal_sta_session ? WIFI_OWNER_PORTAL : WIFI_OWNER_UPDATER;
    if (wifi_owner_get() != expected_owner) {
      uf_net_operation_active = false;
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    uf_net_operation_active = true;
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    const bool sta_ok = uf_wifi_ensure_sta();
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      continue;
    }
    if (!sta_ok) {
      const bool scan_was_requested = uf_req_scan || uf_req_prefetch;
      const bool flash_was_requested = uf_req_flash;
      uf_req_scan = false;
      uf_req_connect = false;
      uf_req_check = false;
      uf_req_flash = false;
      uf_req_scan_models = false;
      uf_req_prefetch = false;
      uf_scanning = false;
      uf_connecting = false;
      uf_models_scanning = false;
      if (flash_was_requested) uf_flash_failed = true;
      if (scan_was_requested) {
        uf_lock();
        strncpy(uf_ssid_list, "(WiFi unavailable)", sizeof(uf_ssid_list) - 1);
        uf_ssid_list[sizeof(uf_ssid_list) - 1] = '\0';
        uf_unlock();
        uf_scan_n = 0;
        uf_scan_ready = true;
      }
      uf_set_status(uf_ml("حافظه داخلی وایفای کافی نیست", "WiFi internal memory unavailable"));
      uf_net_operation_active = false;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (uf_req_prefetch) {
      uf_req_prefetch = false;
      uf_net_prefetch();
    }
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      continue;
    }
    if (uf_req_scan) {
      uf_req_scan = false;
      if (uf_wifi_connected) {
        // already online
      } else {
        // همیشه اسکن واقعی — قبلاً اگر "(no networks)" در بافر بود دوباره سرچ نمی‌کرد
        uf_net_scan();
      }
    }
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      continue;
    }
    if (uf_req_connect)     { uf_req_connect = false;     uf_net_connect(); }
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      continue;
    }
    if (uf_req_scan_models) { uf_req_scan_models = false; uf_net_scan_models(); }
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      continue;
    }
    if (uf_req_check)       { uf_req_check = false;       uf_net_check_version(); }
    if (!uf_session_current(session)) {
      uf_net_operation_active = false;
      continue;
    }
    if (uf_req_flash)       { uf_req_flash = false;       uf_net_flash(); }
    uf_net_operation_active = false;
    vTaskDelay(pdMS_TO_TICKS(60));
  }
}

// =============================================================================
//  APIِ عمومی — از main.ino صدا زده می‌شود
// =============================================================================

// در setup()، خیلی زود صدا زده می‌شود: اگر F5 نگه داشته شده باشد g_update_mode را true می‌کند.
static inline void update_flow_boot_check() {
  pinMode(UF_F5_ADC_PIN, INPUT);   // مثلِ Task9 — INPUT_PULLUP مقدارِ کیپدِ مقاومتی را خراب می‌کرد
  analogReadResolution(12);
  delay(250);
  int hits = 0, v = 0, min_v = 4095, max_v = 0, run = 0, best_run = 0;
  for (int i = 0; i < 50; i++) {
    v = analogRead(UF_F5_ADC_PIN);
    if (v < min_v) min_v = v;
    if (v > max_v) max_v = v;
    if (v > UF_F5_ADC_MIN && v < UF_F5_ADC_MAX) {
      hits++;
      run++;
      if (run > best_run) best_run = run;
    } else {
      run = 0;
    }
    delay(20);
  }
  g_update_mode = (hits >= 45 && best_run >= 25);
  Serial.printf("[UF] boot F5 stable raw=%d min=%d max=%d hits=%d/50 run=%d -> update_mode=%d\n",
                v, min_v, max_v, hits, best_run, (int)g_update_mode);
}

// uf_net_task_started is declared before update_flow_tick so the net task can be deferred safely.

// بعد از راه‌اندازیِ UI (splash) صدا زده می‌شود؛ فقط وقتی g_update_mode==true.
static inline void update_flow_start() {
  if (!g_update_mode) return;
  if (!uf_mutex) uf_mutex = xSemaphoreCreateMutex();
  if (!uf_mutex) {
    critical_task_start_failed = true;
    panel_request_stop_motion();
    g_update_mode = false;
    Serial.println("[UF] FATAL: update mutex allocation failed");
    return;
  }
  if (!wifi_owner_begin_updater()) {
    critical_task_start_failed = true;
    panel_request_stop_motion();
    g_update_mode = false;
    Serial.println("[UF] FATAL: WiFi owner unavailable at update boot");
    return;
  }
  panel_request_stop_motion();
  uf_advance_session();
  uf_reset_session_flags();
  uf_wait_portal_stop = false;
  uf_exit_wait_network = false;
  uf_state = UF_ST_WAIT_SPLASH;
  uf_start_ms = lv_tick_get();
  uf_set_status("");
  Serial.println("[UF] update flow armed (F5-boot, net task deferred)");
}

// از دکمهٔ بروزرسانی F5 داخل برنامه — مستقیم صفحهٔ وای‌فای، بدون رمز مدیر و بدون ریست
void update_flow_start_from_ui() {
  if (web_portal_ota_active) {
    GUI.buzzer_cmd_long = true;
    ui_toast("نصب محلی در حال انجام است",
             "Local firmware install is in progress", 3500);
    return;
  }
  if (!uf_begin_update_maintenance()) return;
  uf_advance_session();
  uf_reset_session_flags();
  uf_lang = (GUI.system_language == 2) ? 1 : 0;
  uf_ssid_list[0] = 0;
  uf_scan_n = 0;
  uf_start_net_task_once();
  uf_pw_ok = false;      // از UI مستقیم وایفای — مرحلهٔ رمز نیست
  uf_exit_req = false;
  uf_exit_wait_network = false;
  uf_state = UF_ST_WIFI;
  uf_start_ms = 0;
  // اگر قبلاً صفحهٔ رمز ساخته شده بود ببند
  if (uf_pw_root) { lv_obj_delete(uf_pw_root); uf_pw_root = NULL; uf_pw_ta = NULL; uf_pw_err = uf_pw_kb_root = NULL; }
  if (!uf_wifi_root) uf_build_wifi();
  uf_schedule_prefetch_after_wifi_handoff();
  Serial.println("[UF] update flow started from F5 UI (WiFi, no PIN)");
}

// [UI] Reuse only the updater's proven scan/password overlay for F1 STA.
// This deliberately does not call uf_begin_update_maintenance: cutting/motion
// policy and OTA security remain untouched.
void update_flow_start_portal_sta() {
  if (g_update_mode || update_flow_network_busy() || web_portal_running ||
      web_portal_transitioning || !wifi_owner_try_acquire(WIFI_OWNER_PORTAL)) {
    GUI.buzzer_cmd_long = true;
    ui_toast("وای فای در اختیار عملیات دیگر است", "WiFi is busy", 3000);
    return;
  }
  if (!uf_mutex) uf_mutex = xSemaphoreCreateMutex();
  if (!uf_mutex) {
    wifi_owner_release(WIFI_OWNER_PORTAL);
    GUI.buzzer_cmd_long = true;
    ui_toast("حافظه برای وای فای کافی نیست", "Not enough memory for WiFi", 3500);
    return;
  }
  uf_portal_sta_session = true;
  uf_advance_session();
  uf_reset_session_flags();
  uf_lang = (GUI.system_language == 2) ? 1 : 0;
  uf_ssid_list[0] = 0;
  uf_scan_n = 0;
  uf_exit_req = false;
  uf_state = UF_ST_WIFI;
  if (!uf_wifi_root) uf_build_wifi();
  uf_start_net_task_once();
  uf_req_prefetch = true;
}

// دیباگ (UPD1): ورود به جریانِ بروزرسانی از صفحهٔ رمز — بدونِ نگه‌داشتنِ F5
static void uf_dbg_start_password(void) {
  if (!uf_begin_update_maintenance()) return;
  uf_advance_session();
  uf_reset_session_flags();
  uf_wait_portal_stop = false;
  uf_start_net_task_once();
  uf_exit_req = false; uf_exit_wait_network = false; uf_pw_ok = false;
  uf_set_status("");
  uf_state = UF_ST_WAIT_SPLASH;
  uf_start_ms = lv_tick_get();
  Serial.println("[UF] debug: password screen (UPD1)");
}
