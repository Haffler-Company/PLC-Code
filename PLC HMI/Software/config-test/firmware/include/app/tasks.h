#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <time.h>   // SNTP: time_t/localtime_r/asctime/configTzTime (configTzTime در Arduino.h)
#include <mbedtls/sha256.h>
#include <lvgl.h>
#include "general.h"
#include "vars.h"
#include "ui.h"
#include "screens.h"
#include "fonts.h"
#include "storage.h"
#include "lang.h"
#include "hwtest.h"
#include "app_sound.h"
#include "web_portal.h"
extern "C" void t_ui_tick();   // تیک UI تست (فضای‌نام t_)

extern HafflerWebPortal g_web_portal;

extern "C" int16_t eez_flow_get_current_screen();
extern "C" void eez_flow_set_screen(int16_t screenId, lv_scr_load_anim_t animType, uint32_t speed, uint32_t delay);
extern bool g_restore_needed;
extern uint8_t g_restore_is_hwtest;
extern uint8_t g_restore_screen_id;
extern uint8_t g_restore_suite;
extern AppRestoreContext s_rtc_restore_ctx;

extern SemaphoreHandle_t xMutex_Wifi;
extern SemaphoreHandle_t g_lvgl_mutex;   // میوتکسِ سریال‌سازیِ LVGL: فقط سمتِ app_task قفل می‌شود؛ lvgl_task صاحبِ اصلی است

extern bool isSearching;
extern String wifi_names;
extern String selectedWifi;
extern String wifiPassword;
extern String wifis[20];
extern bool connectingToWifi;
extern Storage storage;
extern "C" void config_start_wifi_scan(void);

extern volatile int wifi_conncting_counter;   // B-14: volatile — از caching compiler جلوگیری می‌کند

extern String ota_download_model;
extern bool ota_download;
extern String ota_custom_url;    // URL دلخواهِ «ورود لینک»
extern bool ota_custom_download; // پرچمِ دانلود از URL دلخواه
extern volatile bool g_link_check_req;
extern volatile bool g_link_check_done;
extern volatile LinkCheckStatus g_link_check_status;
extern String g_link_check_url;
extern String g_link_check_result;
extern char s_link_check_result_buf[256];
extern bool g_knownAutoAttempt;
extern volatile bool g_wifi_ok_msg;   // پیامِ «متصل شد» — ست در app_task، مصرف در tick وای‌فای
extern char g_wifi_ok_ssid[65];       // SSIDِ اتصالِ موفق (UTF-8)
extern volatile bool g_wifi_wrong_password;
extern volatile int g_wifi_last_disconnect_reason;
// تعریف واقعی در config_hwtest.ino — اینجا فقط extern تا ODR violation نشود
extern volatile bool g_ota_cancel_requested;
extern "C" void action_download_cancel(lv_event_t *e);
extern "C" void ota_guard_tick(void);

static const char *host = "ota.haffler-infi.ir"; // static: هر TU نسخه‌ی محلی → بدون ODR

// سازنده‌ی URL سرورِ OTA: مسیرِ «دسته/برنامه/فایل» (پروتکل استاندارد ArvanCloud S3 روی پورت 80 HTTP معمولی)
static String ota_url(const String &model, const char *file)
{
    String m = model;
    m.trim();

    // هندل آدرس کامل در صورتی که پاس شده باشد
    if (m.startsWith("http://") || m.startsWith("https://")) {
        if (m.startsWith("https://ota.haffler-infi.ir")) {
            m.replace("https://", "http://");
        }
        return m;
    }

    uint16_t slash_guard = 0;
    while (m.startsWith("/") && slash_guard++ < 32) m.remove(0, 1);
    slash_guard = 0;
    while (m.endsWith("/") && slash_guard++ < 32) m.remove(m.length() - 1);
    if (m.startsWith("plc_hmi/")) m.remove(0, 8);

    // پیشوند دسته (upvc یا other) در صورت نبود
    if (!m.startsWith("upvc/") && !m.startsWith("upvc%2F") && !m.startsWith("upvc%2f") &&
        !m.startsWith("other/") && !m.startsWith("other%2F") && !m.startsWith("other%2f")) {
        if (g_firmware_suite == SUITE_OTHER) {
            m = "other/" + m;
        } else {
            m = "upvc/" + m;
        }
    }

    const char *f = (file != NULL) ? file : "";
    uint16_t f_guard = 0;
    while (*f == '/' && f_guard++ < 32) f++;

    // در صورتی که کاندیدای مدل مستقیماً با %2F تعریف شده باشد، از %2F استفاده می‌شود
    if (m.indexOf("%2F") >= 0 || m.indexOf("%2f") >= 0) {
        return "http://" + String(host) + "/" + m + "%2F" + f;
    } else {
        // مسیر مستقیم استاندارد اسلش روی S3 (پاسخ آنی در کمتر از ۲۰۰ میلی‌ثانیه)
        return "http://" + String(host) + "/" + m + "/" + f;
    }
}

void download_firmware(String model);
void download_firmware_from_url(String url);

extern SemaphoreHandle_t xMutex_I2C;

// ==== جدول ۹ برنامه ماشین‌آلات UPVC و آلومینیوم (رمز 123) ====
// اولویت اول مسیر استاندارد با اسلش مستقیم (تست‌شده روی S3 با پاسخ 200 فوری)
static const char *g_card_model_candidates[9][6] = {
    { "upvc/welding-single-head", "upvc%2Fwelding-single-head", "welding-single-head", "Welding_single_head", NULL }, // 0 جوش تک‌سر
    { "upvc/welding-double-head", "upvc%2Fwelding-double-head", "welding-double-head", "Welding_double_head", NULL }, // 1 جوش دوسر
    { "upvc/digital-cutter_single_head", "upvc%2Fdigital-cutter_single_head", "digital-cutter_single_head", "upvc/digital-cutter-single-head", NULL }, // 2 برش تک‌سر
    { "upvc/digital-cutter-double-head", "upvc%2Fdigital-cutter-double-head", "digital-cutter-double-head", "upvc%2Fdigital-cutter%20double_head", NULL }, // 3 برش دوسر
    { "upvc/gas-filling", "upvc%2Fgas-filling", "gas-filling", "Gas-filling", NULL },                         // 4 تزریق گاز شیشه
    { "upvc/corner-cleaning", "upvc%2Fcorner-cleaning", "corner-cleaning", "Corner-cleaning", NULL },             // 5 گوشه‌تمیزکن
    { "upvc/copy-router", "upvc%2Fcopy-router", "copy-router", "Copy-router", NULL },                         // 6 کپی فرز
    { "upvc/aluminum-punching", "upvc%2Faluminum-punching", "aluminum-punching", "Aluminum-punching", NULL },       // 7 پانچ آلومینیوم
    { "upvc/config-test", "upvc%2Fconfig-test", "config-test", "Config-test", NULL }                          // 8 برنامه‌ی Config
};

static const StrId g_card_name[9] = {
    STR_PROD_SINGLE, STR_PROD_WELDER_DOUBLE, STR_PROD_CUTTER_SINGLE, STR_PROD_CUTTER_DOUBLE,
    STR_PROD_GAS, STR_PROD_CORNER, STR_PROD_COPY, STR_PROD_PUNCH, STR_PROD_CONFIG,
};
// نیاز سخت‌افزاری: 0=ندارد، 1=حداقل یک MAX6675، 2=دو MAX6675 + MCP23017، 3=MCP23017
static const uint8_t g_card_hwreq[9] = { 1, 2, 0, 3, 0, 0, 0, 0, 0 };

// ==== جدول ۴ برنامه سایر شرکت‌ها (رمز 321) ====
static const char *g_other_card_model_candidates[4][6] = {
    { "other/ice-cream", "other/ice_cream", "ice-cream", "other%2Fice-cream", NULL },           // 0 دستگاه بستنی‌ساز
    { "other/bakery-changir", "other/changir", "bakery-changir", "other%2Fbakery-changir", NULL }, // 1 چانه‌گیر نانوایی
    { "other/bakery-mixer", "other/bakery_mixer", "bakery-mixer", "other%2Fbakery-mixer", NULL }, // 2 خمیرگیر اسپیرال
    { "upvc/config-test", "config-test", "upvc%2Fconfig-test", NULL }                            // 3 برنامه‌ی Config
};

static const StrId g_other_card_name[4] = {
    STR_PROD_ICE_CREAM, STR_PROD_BAKERY_CHANGIR, STR_PROD_BAKERY_MIXER, STR_PROD_CONFIG
};
static const uint8_t g_other_card_hwreq[4] = { 0, 0, 0, 0 };

// B-03: static — اگر tasks.h در چند TU include شد، هر کدام نسخه‌ی مجزا دارند (ODR-safe)
static String g_card_active_model[9]; // نام مسیر فعال روی سرور که lastfile با موفقیت از آن دریافت شد
static String g_card_ver[9];
// فراداده‌ی lastfile استاندارد (per-card، در نخ App پر می‌شوند؛ فقط رشته‌ها + پرچم‌ها):
static String g_card_desc[9][10];     // عموم توضیحات / fallback
static String g_card_desc_fa[9][10];  // توضیحات فارسی
static String g_card_desc_en[9][10];  // توضیحات انگلیسی
static String g_card_desc_tr[9][10];  // توضیحات ترکی
static String g_card_desc_ru[9][10];  // توضیحات روسی
static String g_card_bdate[9];     // «[version] build_date=»
static String g_card_btime[9];     // «[version] build_time=»
static String g_card_inputs[9];    // ورودی‌های پارس‌شده از سرور [inputs]
static String g_card_outputs[9];   // خروجی‌های پارس‌شده از سرور [outputs]
static bool   g_card_exists[9] = { false };       // آیا برنامه روی سرور هست
static bool   g_card_checked[9] = { false };      // آیا این کارت از سرور استعلام شده است
static volatile bool g_card_ver_fetched = false;  // یک‌بار پس از اتصال وای‌فای خوانده می‌شود
static volatile bool g_card_ver_ready = false;    // پرچم انتقال به نخ LVGL
static uint32_t s_last_card_query_time = 0;       // زمان آخرین استعلام کارت‌ها
extern "C" {
uint32_t g_card_query_start_ms = 0;        // زمان آغاز استعلام سرور (پایش سقف ۱۵ ثانیه - موردنیاز screens.c)
void config_reset_card_query(void) {
    g_card_ver_fetched = false;
    g_card_ver_ready = false;
    g_card_query_start_ms = millis();
    s_last_card_query_time = 0;
    for (int i = 0; i < 9; i++) {
        g_card_exists[i] = false;
        g_card_checked[i] = false;
        g_card_ver[i] = "";
        g_card_inputs[i] = "";
        g_card_outputs[i] = "";
        for (int d = 0; d < 10; d++) {
            g_card_desc[i][d] = "";
            g_card_desc_fa[i][d] = "";
            g_card_desc_en[i][d] = "";
            g_card_desc_tr[i][d] = "";
            g_card_desc_ru[i][d] = "";
        }
    }
}
const char* get_card_version_str(int idx) {
    if (idx < 0 || idx >= 9) return "";
    return g_card_ver[idx].c_str();
}
const char* get_card_bdate_str(int idx) {
    if (idx < 0 || idx >= 9) return "";
    return g_card_bdate[idx].c_str();
}
const char* get_card_btime_str(int idx) {
    if (idx < 0 || idx >= 9) return "";
    return g_card_btime[idx].c_str();
}
const char* get_card_desc_line(int idx, int line) {
    if (idx < 0 || idx >= 9 || line < 0 || line >= 10) return "";
    if (g_lang == LANG_FA) {
        if (g_card_desc_fa[idx][line].length()) return g_card_desc_fa[idx][line].c_str();
    } else if (g_lang == LANG_TR) {
        if (g_card_desc_tr[idx][line].length()) return g_card_desc_tr[idx][line].c_str();
        if (g_card_desc_en[idx][line].length()) return g_card_desc_en[idx][line].c_str();
    } else if (g_lang == LANG_RU) {
        if (g_card_desc_ru[idx][line].length()) return g_card_desc_ru[idx][line].c_str();
        if (g_card_desc_en[idx][line].length()) return g_card_desc_en[idx][line].c_str();
    } else {
        if (g_card_desc_en[idx][line].length()) return g_card_desc_en[idx][line].c_str();
    }
    return g_card_desc[idx][line].c_str();
}
const char* get_card_inputs_str(int idx) {
    if (idx < 0 || idx >= 9) return "";
    return g_card_inputs[idx].c_str();
}
const char* get_card_outputs_str(int idx) {
    if (idx < 0 || idx >= 9) return "";
    return g_card_outputs[idx].c_str();
}
bool get_card_exists_flag(int idx) {
    if (idx < 0 || idx >= 9) return false;
    return g_card_exists[idx];
}
uint8_t get_card_hwreq_val(int idx) {
    if (idx < 0 || idx >= 9) return 0;
    return g_card_hwreq[idx];
}
static char s_lastfile_url_buf[160];
const char* get_card_lastfile_url(int idx) {
    if (idx < 0 || idx >= 9) return "";
    const char *defModel = (g_firmware_suite == SUITE_OTHER && idx < 4) ?
        g_other_card_model_candidates[idx][0] : g_card_model_candidates[idx][0];
    const char *m = g_card_active_model[idx].length() > 0 ? g_card_active_model[idx].c_str() : defModel;
    String u = ota_url(String(m), "lastfile");
    strncpy(s_lastfile_url_buf, u.c_str(), sizeof(s_lastfile_url_buf) - 1);
    s_lastfile_url_buf[sizeof(s_lastfile_url_buf) - 1] = '\0';
    return s_lastfile_url_buf;
}
}

static int g_pending_card = -1;                    // B-03: static — کارتِ منتظرِ تأیید نصب

// درصد دانلود برای حلقه‌ی (arc) دور آیکون دانلود
static volatile int  g_dl_pct = 0;
static volatile bool g_dl_pct_dirty = false;

static bool versionNewer(String server, String current);
extern "C" void firmware_confirm_countdown_start(void);
extern "C" void firmware_confirm_countdown_stop(void);
extern "C" void ota_guard_arm_for_next_boot(void);

// متن پایه‌ی پنل دانلود برای چسباندن درصد
static String g_dl_base = "";
static uint32_t s_last_speed_tick = 0;
static int s_last_speed_bytes = 0;
static float s_smooth_speed_kbps = 0.0f;
static float s_smooth_rem_sec = 0.0f;

static void ota_progress_reset(void)
{
    s_last_speed_tick = 0;
    s_last_speed_bytes = 0;
    s_smooth_speed_kbps = 0.0f;
    s_smooth_rem_sec = 0.0f;
}

static void ota_progress_cb(int cur, int total)
{
    int pct = (total > 0) ? (int)((int64_t)cur * 100 / total) : 0;
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    g_dl_pct = pct; g_dl_pct_dirty = true;   // به‌روزرسانی حلقه‌ی دور آیکون

    uint32_t now = millis();
    if (s_last_speed_tick == 0 || now - s_last_speed_tick >= 350 || cur == total)
    {
        if (s_last_speed_tick > 0 && now > s_last_speed_tick)
        {
            uint32_t dt = now - s_last_speed_tick;
            int db = cur - s_last_speed_bytes;
            if (db > 0 && dt > 0) {
                float inst_speed = (float)((int64_t)db * 1000 / (dt * 1024));
                if (s_smooth_speed_kbps <= 0.1f) {
                    s_smooth_speed_kbps = inst_speed;
                } else {
                    // فیلتر میانگین متحرک نمایی (EMA) برای رفع جهش‌های سرعت
                    s_smooth_speed_kbps = (0.70f * s_smooth_speed_kbps) + (0.30f * inst_speed);
                }
            }
        }
        s_last_speed_tick = now;
        s_last_speed_bytes = cur;

        float cur_mb = (float)cur / (1024.0f * 1024.0f);
        float tot_mb = (float)total / (1024.0f * 1024.0f);
        int display_speed_kbps = (int)(s_smooth_speed_kbps + 0.5f);

        int rem_bytes = (total > cur) ? (total - cur) : 0;
        char rem_str[16] = "--s";
        if (cur == total || pct >= 100) {
            s_smooth_rem_sec = 0.0f;
            snprintf(rem_str, sizeof(rem_str), "0s");
        } else if (s_smooth_speed_kbps > 1.0f && rem_bytes > 0) {
            float inst_rem = (float)rem_bytes / (s_smooth_speed_kbps * 1024.0f);
            if (s_smooth_rem_sec <= 0.1f) {
                s_smooth_rem_sec = inst_rem;
            } else {
                // فیلتر نرم‌کننده نوسانات زمان باقیمانده (EMA)
                s_smooth_rem_sec = (0.75f * s_smooth_rem_sec) + (0.25f * inst_rem);
            }
            int disp_sec = (int)(s_smooth_rem_sec + 0.5f);
            if (disp_sec < 1) disp_sec = 1;
            snprintf(rem_str, sizeof(rem_str), "%ds", disp_sec);
        }

        char metrics_str[96];
        snprintf(metrics_str, sizeof(metrics_str), "%%%d   |   %.2f / %.2f MB   |   %d KB/s   |   %s", pct, cur_mb, tot_mb, display_speed_kbps, rem_str);

        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
        {
            lv_label_set_text(objects.download_metrics_label, metrics_str);
        }
        // حذف درصد تکراری از متن بالای دایره (درخواست صریح کارفرما — درصد فقط در سطر مشخصات پایین)
        set_var_download_lable_info_text(g_dl_base.c_str());

        // اگر دانلود به ۹۵٪ یا بالاتر رسید، لغو غیرفعال و به «نهایی‌سازی فریمور...» تغییر می‌یابد
        if (pct >= 95 && objects.download_cancel_btn && lv_obj_is_valid(objects.download_cancel_btn))
        {
            if (!lv_obj_has_state(objects.download_cancel_btn, LV_STATE_DISABLED))
            {
                lv_obj_add_state(objects.download_cancel_btn, LV_STATE_DISABLED);
                lv_obj_set_style_bg_color(objects.download_cancel_btn, lv_color_hex(0xff555555), 0);
                if (objects.download_cancel_label && lv_obj_is_valid(objects.download_cancel_label))
                {
                    lv_label_set_text(objects.download_cancel_label, tr(STR_DL_FINALIZING));
                }
            }
        }
        xSemaphoreGive(g_lvgl_mutex);
    }
}

// بررسی نویسه‌های فارسی / عربی در UTF-8
static bool cfg_is_persian_utf8(const String &s)
{
    int len = (int)s.length();
    if (len < 2) return false;   // گارد: رشته خالی یا تک‌بایتی هیچ‌گاه UTF-8 عربی ندارد
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

// بررسی نویسه‌های سیریلیک / روسی در UTF-8
static bool cfg_is_cyrillic_utf8(const String &s)
{
    int len = (int)s.length();
    if (len < 2) return false;   // گارد: رشته خالی یا تک‌بایتی هیچ‌گاه سیریلیک UTF-8 ندارد
    for (int i = 0; i < len - 1; i++) {
        uint8_t c1 = (uint8_t)s[i];
        if (c1 == 0xD0 || c1 == 0xD1) return true;
    }
    return false;
}

static void cfg_sanitize_display_text(String &s);
static void cfg_strip_bullet(String &s);

// قالب‌بندی صنعتیِ خطوط پایه‌ها بر اساس استاندارد زیمنس و پایه‌های PLC (I, Q, M, T)
// حذف ارجاعات میکروکنترلری (GPIO, ADC, I2C, SPI) و نمایش برچسب‌های تابلوییِ صنعتی
static String cfg_format_plc_line(const String &raw, bool isInput, int defaultIdx)
{
    String s = raw;
    s.trim();
    if (!s.length()) return "";

    // اگر خط توضیح/نکته باشد (note=)
    if (s.startsWith("note=") || s.startsWith("note:") || s.startsWith("Note")) {
        if (g_lang == LANG_FA) {
            return "[نکته: کلیه خروجی‌ها ایزوله با سطح منطقی Active LOW]";
        } else {
            return "[Note: All outputs are isolated Active LOW]";
        }
    }

    // حذف پیشوندهای inX= یا outX=
    int eq = s.indexOf('=');
    if (eq > 0 && eq < 8) {
        s = s.substring(eq + 1);
        s.trim();
    }

    // تشخیص قطعی پایه‌های نصب‌نشده (Not fitted / Reserved)
    if (s.indexOf("not fitted") >= 0 || s.indexOf("Not fitted") >= 0) {
        String tag = isInput ? ("I" + String(defaultIdx)) : ("Q" + String(defaultIdx));
        int qPos = s.indexOf("Q");
        if (qPos >= 0 && qPos + 1 < s.length() && isDigit(s[qPos + 1])) {
            tag = "Q" + String(s[qPos + 1]);
        }
        if (g_lang == LANG_FA) return "- " + tag + ": رزرو (نصب‌نشده در این مدل)";
        if (g_lang == LANG_TR) return "- " + tag + ": Rezerve (Bu modelde yok)";
        if (g_lang == LANG_RU) return "- " + tag + ": Rezerv (Ne ustanovleno)";
        return "- " + tag + ": Reserved (Not fitted on this model)";
    }

    // استخراج تگ صنعتی PLC (Q, I, M, T/TC)
    String tag = "";
    int len = s.length();

    if (s.indexOf("I10..I17") >= 0 || s.indexOf("I10-I17") >= 0) {
        tag = "I10..I17";
    } else if (s.indexOf("F1..F5") >= 0) {
        tag = "I0..I4 (F1..F5)";
    } else if (s.indexOf("F6") >= 0 && s.indexOf("F10") >= 0) {
        tag = "I5..I9 (F6..F10)";
    } else if (s.indexOf("TC1") >= 0 || s.indexOf("tc1") >= 0 || s.indexOf("CS1") >= 0) {
        tag = "TC1 (T1)";
    } else if (s.indexOf("TC2") >= 0 || s.indexOf("tc2") >= 0 || s.indexOf("CS2") >= 0) {
        tag = "TC2 (T2)";
    } else if (s.indexOf("MCP23017") >= 0 || s.indexOf("expander") >= 0) {
        tag = "EXP (I/O)";
    } else if (s.indexOf("buzzer") >= 0 || s.indexOf("Buzzer") >= 0) {
        tag = "Q10 (BUZZ)";
    } else {
        for (int i = 0; i < len - 1; i++) {
            char c = s[i];
            char c2 = s[i + 1];
            if ((c == 'Q' || c == 'q') && isDigit(c2)) {
                tag = "Q" + String(c2);
                break;
            }
            if ((c == 'I' || c == 'i') && isDigit(c2)) {
                tag = "I" + String(c2);
                if (i + 2 < len && isDigit(s[i + 2])) tag += String(s[i + 2]);
                break;
            }
            if ((c == 'M' || c == 'm') && isDigit(c2)) {
                tag = "M" + String(c2);
                break;
            }
            if ((c == 'T' || c == 't') && isDigit(c2)) {
                tag = "T" + String(c2);
                break;
            }
        }
    }

    if (!tag.length()) {
        tag = isInput ? ("I" + String(defaultIdx)) : ("Q" + String(defaultIdx));
    }

    // پاک‌سازی عبارات سخت‌افزاری تراشه: GPIOxx, ADC, I2C, SPI, SO=.., SCK=..
    String desc = s;
    if (tag.length()) {
        int tp = desc.indexOf(tag);
        if (tp >= 0) desc.remove(tp, tag.length());
    }
    for (int d = 0; d <= 9; d++) {
        desc.replace("Q" + String(d), "");
        desc.replace("q" + String(d), "");
        desc.replace("I" + String(d), "");
        desc.replace("i" + String(d), "");
    }
    for (int d = 10; d <= 17; d++) {
        desc.replace("I" + String(d), "");
        desc.replace("i" + String(d), "");
    }

    while (true) {
        int gp = desc.indexOf("GPIO");
        if (gp < 0) gp = desc.indexOf("gpio");
        if (gp < 0) break;
        int endp = gp + 4;
        if (endp < desc.length() && (desc[endp] == '-' || desc[endp] == '.' || isDigit(desc[endp]))) {
            while (endp < desc.length() && (desc[endp] == '-' || desc[endp] == '.' || isDigit(desc[endp]))) endp++;
        }
        desc.remove(gp, endp - gp);
    }

    auto strip_sub = [&](const char *pat) {
        int p = desc.indexOf(pat);
        if (p >= 0) {
            int op = desc.lastIndexOf('(', p);
            int cp = desc.indexOf(')', p);
            if (op >= 0 && cp > op) {
                desc.remove(op, cp - op + 1);
            } else {
                desc.replace(pat, "");
            }
        }
    };
    strip_sub("SO=");
    strip_sub("SDA=");
    strip_sub("software PWM");
    strip_sub("band-encoded");
    strip_sub("addr 0x");
    strip_sub("hwtest");

    desc.replace("band-encoded ADC", "");
    desc.replace("band-encoded pairs", "");
    desc.replace("ADC inputs", "");
    desc.replace("ADC", "");
    desc.replace("I2C", "");
    desc.replace("CS1", "");
    desc.replace("CS2", "");
    desc.replace("relay", "");
    desc.replace("Relay", "");

    desc.trim();
    while (desc.startsWith(":") || desc.startsWith("-") || desc.startsWith(",") || desc.startsWith(".") || desc.startsWith("=")) {
        desc.remove(0, 1);
        desc.trim();
    }
    while (desc.endsWith(":") || desc.endsWith("-") || desc.endsWith(",") || desc.endsWith(".") || desc.endsWith("=")) {
        desc.remove(desc.length() - 1);
        desc.trim();
    }

    // ترجمه و بهینه‌سازی صنعتی اصطلاحات بر اساس نوع قطعه و زبان انتخابی
    String finalDesc = "";
    if (g_lang == LANG_FA) {
        if (tag.startsWith("Q") && (tag == "Q5" || tag == "Q9" || s.indexOf("SSR") >= 0 || s.indexOf("heater") >= 0)) {
            finalDesc = "خروجی المنت حرارتی SSR (Software PWM)";
        } else if (tag.startsWith("Q10") || tag.indexOf("BUZZ") >= 0 || s.indexOf("buzzer") >= 0) {
            finalDesc = "هشدار صوتی بازر تابلویی";
        } else if (tag.startsWith("Q")) {
            if (desc.length() && cfg_is_persian_utf8(desc)) finalDesc = desc;
            else finalDesc = desc.length() ? (desc + " (رله خروجی)") : "رله خروجی صنعتی ۲۲۰V";
        } else if (tag.startsWith("TC") || tag.startsWith("T") || s.indexOf("MAX6675") >= 0) {
            finalDesc = "سنسور دمای ترموکوپل نوع K (MAX6675)";
        } else if (tag.indexOf("F1..F5") >= 0) {
            finalDesc = "کلیدهای عملیاتی F1 تا F5 پنل";
        } else if (tag.indexOf("F6..F10") >= 0) {
            finalDesc = "کلیدهای کنترلی استارت/استاپ/حالت";
        } else if (tag.indexOf("I10..I17") >= 0 || s.indexOf("photoswitch") >= 0) {
            finalDesc = "ورودی سنسورها و فتوسوئیچ‌های نوری";
        } else if (tag.indexOf("EXP") >= 0) {
            finalDesc = "افزاینده ۱۶ کاناله ورودی/خروجی MCP23017";
        } else {
            finalDesc = desc.length() ? desc : "ورودی دیجیتال صنعتی";
        }
    } else if (g_lang == LANG_TR) {
        if (tag.startsWith("Q") && (tag == "Q5" || tag == "Q9" || s.indexOf("SSR") >= 0)) {
            finalDesc = "SSR Isitici Rezistans Cikisi (PWM)";
        } else if (tag.startsWith("Q10") || tag.indexOf("BUZZ") >= 0) {
            finalDesc = "Sesli Uyari Buzzeri";
        } else if (tag.startsWith("Q")) {
            finalDesc = desc.length() ? (desc + " (Röle)") : "Endüstriyel Röle Çıkışı";
        } else if (tag.startsWith("TC") || tag.startsWith("T")) {
            finalDesc = "Termokupl Sicaklik Sensoru (MAX6675)";
        } else if (tag.indexOf("F1..F5") >= 0 || tag.indexOf("F6..F10") >= 0) {
            finalDesc = "Panel Fonksiyon Tuslari";
        } else if (tag.indexOf("I10..I17") >= 0) {
            finalDesc = "Fotosel ve Optik Sensor Girisleri";
        } else {
            finalDesc = desc.length() ? desc : "Endüstriyel Dijital Giris";
        }
    } else if (g_lang == LANG_RU) {
        if (tag.startsWith("Q") && (tag == "Q5" || tag == "Q9" || s.indexOf("SSR") >= 0)) {
            finalDesc = "Vyhod nagrevatelya SSR (PWM)";
        } else if (tag.startsWith("Q10") || tag.indexOf("BUZZ") >= 0) {
            finalDesc = "Zvukovoy signal buzzer";
        } else if (tag.startsWith("Q")) {
            finalDesc = desc.length() ? (desc + " (Rele)") : "Releyniy vyhod 220V";
        } else if (tag.startsWith("TC") || tag.startsWith("T")) {
            finalDesc = "Datchik temperatury termopary";
        } else if (tag.indexOf("F1..F5") >= 0 || tag.indexOf("F6..F10") >= 0) {
            finalDesc = "Funktsionalnye klavishi paneli";
        } else if (tag.indexOf("I10..I17") >= 0) {
            finalDesc = "Vhody opticheskikh datchikov";
        } else {
            finalDesc = desc.length() ? desc : "Promyshlenny tsifrovoy vhod";
        }
    } else {
        if (tag.startsWith("Q") && (tag == "Q5" || tag == "Q9" || s.indexOf("SSR") >= 0)) {
            finalDesc = "SSR Heater Output (Software PWM)";
        } else if (tag.startsWith("Q10") || tag.indexOf("BUZZ") >= 0) {
            finalDesc = "Audible Warning Buzzer";
        } else if (tag.startsWith("Q")) {
            finalDesc = desc.length() ? (desc + " (Relay)") : "Industrial Relay Output 220V";
        } else if (tag.startsWith("TC") || tag.startsWith("T")) {
            finalDesc = "MAX6675 Type-K Thermocouple Sensor";
        } else if (tag.indexOf("F1..F5") >= 0) {
            finalDesc = "Front Panel Function Keys F1..F5";
        } else if (tag.indexOf("F6..F10") >= 0) {
            finalDesc = "Front Panel Control Keys (Start/Stop/Mode)";
        } else if (tag.indexOf("I10..I17") >= 0) {
            finalDesc = "Optical Photoswitch Sensor Inputs";
        } else if (tag.indexOf("EXP") >= 0) {
            finalDesc = "MCP23017 16-ch I/O Expander";
        } else {
            finalDesc = desc.length() ? desc : "Industrial Digital Input";
        }
    }

    cfg_sanitize_display_text(finalDesc);
    return "- " + tag + ": " + finalDesc;
}

// فراداده‌ی استخراج‌شده از lastfile استاندارد (فقط Stringها — بدون تخصیصِ اضافه)
struct LastfileInfo {
    String ver;             // display= یا code= یا خطِ قدیمیِ تخت
    String bdate, btime;    // build_date= / build_time=
    String desc[10];        // تا ۱۰ خطِ توضیحِ تغییرات (هر خط ≤۷۰ نویسه)
    int    desc_count = 0;
    String desc_fa[10];     // توضیحات فارسی
    int    desc_fa_count = 0;
    String desc_en[10];     // توضیحات انگلیسی
    int    desc_en_count = 0;
    String desc_tr[10];     // توضیحات ترکی
    int    desc_tr_count = 0;
    String desc_ru[10];     // توضیحات روسی
    int    desc_ru_count = 0;
    String sha256;          // هش SHA-256 فایل فریمور از [binary]
    String bin_file;        // file= از [binary]
    String inputs;          // ورودی‌های پارس‌شده از [inputs]
    String outputs;         // خروجی‌های پارس‌شده از [outputs]
};

// پالایش کامل متن‌های دریافتی از سرور جهت جلوگیری ۱۰۰٪ از نمایش گلیف‌های نامعتبر یا مربعی (Tofu):
// ۱. حذف \r (CR) که در LVGL منجر به کاراکتر مربعی می‌شود.
// ۲. تبدیل نیم‌فاصله‌ها، فاصله‌های نشکن (NBSP: U+00A0) و خط‌تیره‌های یونیکد به اسکی استاندارد.
// ۳. تبدیل نقل‌قول‌ها، بالت‌ها و سه‌نقطه به کاراکترهای قابل رندر در فونت‌های صنعتی.
static void cfg_sanitize_display_text(String &s)
{
    // حذف کامل CR و کاراکترهای کنترلی مضر
    s.replace("\r", "");

    // جایگزینی فاصله نشکن (NBSP: U+00A0) با فاصله استاندارد
    s.replace("\xC2\xA0", " ");

    // استانداردسازی انواع خط‌تیره‌های یونیکد (En-dash, Em-dash)
    s.replace("\xE2\x80\x93", "-");
    s.replace("\xE2\x80\x94", "-");

    // استانداردسازی بالت دایره‌ای (Bullet: U+2022) به خط‌تیره
    s.replace("\xE2\x80\xA2", "-");

    // استانداردسازی سه‌نقطه یونیکد (Ellipsis: U+2026)
    s.replace("\xE2\x80\xA6", "...");

    // استانداردسازی گیومه‌ها و نقل‌قول‌های دوتایی و تکی
    s.replace("\xE2\x80\x9C", "\"");
    s.replace("\xE2\x80\x9D", "\"");
    s.replace("\xE2\x80\x98", "'");
    s.replace("\xE2\x80\x99", "'");
    s.replace("\xC2\xAB", "\"");
    s.replace("\xC2\xBB", "\"");

    // حذف کدهای کنترلی غیرقابل چاپ زیر 0x20 به جز \n و \t
    for (int i = 0; i < (int)s.length(); i++) {
        uint8_t c = (uint8_t)s[i];
        if (c < 0x20 && c != '\n' && c != '\t') {
            s.remove(i, 1);
            i--;
        }
    }
}

// حذف بالت‌ها و نشانه‌های اضافی ابتدای سطر تا از درج بالت تکراری جلوگیری شود
static void cfg_strip_bullet(String &s)
{
    s.trim();
    while (s.startsWith("-") || s.startsWith("*") || s.startsWith("•") || s.startsWith("=")) {
        s.remove(0, 1);
        s.trim();
    }
}

// برگشت به مرزِ معتبرِ UTF-8 برای برشِ طولِ ایمن: اگر بایتِ محلِ برش «ادامه‌ی» یک نویسه‌ی
// چندبایتی باشد (10xxxxxx) عقب می‌رویم تا ابتدای همان نویسه — وگرنه کاراکتر شکسته و گلیفِ مربعی می‌افتد
static int cfg_utf8_cut(const String &s, int maxCut)
{
    if ((int)s.length() <= maxCut) return s.length();
    int cut = maxCut;
    while (cut > 0 && ((uint8_t)s[cut] & 0xC0) == 0x80) cut--;
    return cut;
}

// تجزیه‌ی lastfile استاندارد (پشتیبانی کامل از استاندارد کلید=مقدار و بلوک‌های چندسطری بدون مساوی نظیر Bakery)
static void config_parse_lastfile(const String &body, LastfileInfo &out)
{
    String section = "";
    String legacy = "";        // خطِ قدیمیِ تخت (کمترینِ اولویت‌ها)
    bool sawSection = false;
    bool legacyDone = false;
    int in_line_idx = 1;
    int out_line_idx = 0;
    int len = body.length();
    int from = 0;
    while (from <= len)
    {
        int nl = body.indexOf('\n', from);
        if (nl < 0) nl = len;
        String line = body.substring(from, nl);
        from = nl + 1;
        int cr = line.indexOf('\r');
        if (cr >= 0) line.remove(cr);          // CRLF-safe
        line.trim();
        if (!line.length() || line[0] == '#') continue;   // خطِ خالی/کامنت

        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            section = line.substring(1, line.length() - 1);
            section.toLowerCase();
            section.trim();
            sawSection = true;
            continue;
        }

        int eq = line.indexOf('=');
        String key = "";
        String val = "";

        if (eq > 0)
        {
            key = line.substring(0, eq);
            key.trim(); key.toLowerCase();
            val = line.substring(eq + 1);
            val.trim();
        }
        else
        {
            // پشتیبانی همزمان از فایل‌های استاندارد بدون مساوی (نظیر سفت‌افزار Bakery نانوایی)
            if (!sawSection && !legacyDone) {
                legacy = line;
                legacyDone = true;
                continue;
            }
            val = line;
            val.trim();
            if (section == "version" && !out.ver.length()) {
                cfg_sanitize_display_text(val);
                out.ver = val;
                continue;
            } else if (section == "sha256" && !out.sha256.length()) {
                out.sha256 = val;
                continue;
            } else if (section == "size" || section == "url") {
                continue;
            } else if (section.startsWith("desc") || section == "changelog") {
                key = "line";
            } else if (section == "binary") {
                if (val.length() == 64) {
                    out.sha256 = val;
                    continue;
                } else if (val.endsWith(".bin") || val == "firmware") {
                    out.bin_file = val;
                    continue;
                }
            } else {
                continue;
            }
        }

        if (!val.length()) continue;
        cfg_sanitize_display_text(val);

        if (section == "version")
        {
            if (key == "display") out.ver = val;                       // اولویتِ display
            else if (key == "code" && !out.ver.length()) out.ver = val; // fallback
            else if (key == "build_date") out.bdate = val;
            else if (key == "build_time") out.btime = val;
        }
        else if (section.startsWith("desc") || section == "changelog")
        {
            cfg_strip_bullet(val);
            if (val.length() > 160) val = val.substring(0, cfg_utf8_cut(val, 160));   // سقف ایمن ۱۶۰ بایت روی مرز دقیق UTF-8
            if (!val.length()) continue;

            if (out.desc_count < 10) {
                out.desc[out.desc_count++] = val;
            }

            bool isFa = (section == "description_fa" || key.startsWith("fa") || cfg_is_persian_utf8(val));
            bool isRu = (section == "description_ru" || key.startsWith("ru") || cfg_is_cyrillic_utf8(val));
            bool isTr = (section == "description_tr" || key.startsWith("tr"));
            bool isEn = (section == "description_en" || key.startsWith("en") || (!isFa && !isRu && !isTr));

            if (isFa && out.desc_fa_count < 10) out.desc_fa[out.desc_fa_count++] = val;
            if (isTr && out.desc_tr_count < 10) out.desc_tr[out.desc_tr_count++] = val;
            if (isRu && out.desc_ru_count < 10) out.desc_ru[out.desc_ru_count++] = val;
            if (isEn && out.desc_en_count < 10) out.desc_en[out.desc_en_count++] = val;
        }
        else if (section == "inputs")
        {
            String plc = cfg_format_plc_line(val, true, in_line_idx++);
            if (plc.length()) {
                if (out.inputs.length()) out.inputs += "\n";
                out.inputs += plc;
            }
        }
        else if (section == "outputs")
        {
            String plc = cfg_format_plc_line((key == "note") ? ("note=" + val) : val, false, out_line_idx++);
            if (plc.length()) {
                if (out.outputs.length()) out.outputs += "\n";
                out.outputs += plc;
            }
        }
        else if (section == "binary")
        {
            if (key == "sha256") out.sha256 = val;
            else if (key == "file") out.bin_file = val;
        }
    }
    if (!out.ver.length() && legacy.length()) {
        cfg_sanitize_display_text(legacy);
        out.ver = legacy;   // آخرین fallback: خطِ قدیمیِ تخت
    }
}

// خواندن lastfile + تشخیص وجود برنامه روی سرور (در نخ App).
// نسخه/توضیحات/بیلد را تجزیه و در آرایه‌های per-card ذخیره می‌کند؛ خروجی = نسخه.
static String config_fetch_lastfile(int idx, bool *exists, bool *serverContacted = NULL)
{
    if (idx < 0 || idx >= 9) {
        if (exists) *exists = false;
        if (serverContacted) *serverContacted = false;
        return "";
    }
    // فراداده‌ی کارت همیشه از صفر شروع شود (هم برایِ پاسخِ تازه، هم برایِ خطا/404)
    for (int d = 0; d < 10; d++) {
        g_card_desc[idx][d] = "";
        g_card_desc_fa[idx][d] = "";
        g_card_desc_en[idx][d] = "";
        g_card_desc_tr[idx][d] = "";
        g_card_desc_ru[idx][d] = "";
    }
    g_card_bdate[idx] = "";
    g_card_btime[idx] = "";
    g_card_ver[idx] = "";
    g_card_inputs[idx] = "";
    g_card_outputs[idx] = "";
    if (exists) *exists = false;
    if (serverContacted) *serverContacted = false;

    const char * const *candidates = (g_firmware_suite == SUITE_OTHER && idx < 4) ?
        g_other_card_model_candidates[idx] : g_card_model_candidates[idx];

    bool reachedServer = false;

    for (int c = 0; candidates[c] != NULL && c < 5; c++)
    {
        String model = String(candidates[c]);
        String url = ota_url(model, "lastfile");
        WiFiClient client;
        HTTPClient http;
        http.begin(client, url);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setConnectTimeout(800);    // پاسخ فوق‌سریع سرور (۸۰۰ میلی‌ثانیه)
        http.setTimeout(1200);           // سقف پاسخ ۱۲۰۰ میلی‌ثانیه برای جلوگیری از فریز لوپ
        http.addHeader("User-Agent", "Haffler-OTA/1.5");
        http.addHeader("Accept", "*/*");
        http.addHeader("Connection", "close");
        int code = http.GET();

        // در صورت خطای شبکه (code < 0 مانند تاخیر DNS)، یک تلاش مجدد سریع پس از ۱۵۰ میلی‌ثانیه
        if (code < 0)
        {
            http.end();
            client.stop();
            vTaskDelay(pdMS_TO_TICKS(150));
            http.begin(client, url);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http.setConnectTimeout(800);
            http.setTimeout(1200);
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            code = http.GET();
        }

        // در صورت عدم دریافت ۲۰۰ و وجود %2F، بررسی با اسلش معمولی
        if (code != 200 && code != 404 && (url.indexOf("%2F") >= 0 || url.indexOf("%2f") >= 0))
        {
            String slashUrl = url;
            slashUrl.replace("%2F", "/");
            slashUrl.replace("%2f", "/");
            http.end();
            client.stop();
            http.begin(client, slashUrl);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http.setConnectTimeout(800);
            http.setTimeout(1200);
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            code = http.GET();
        }

        if (code > 0)
        {
            reachedServer = true;
        }

        if (code == 200)
        {
            LastfileInfo info;
            config_parse_lastfile(http.getString(), info);
            g_card_ver[idx] = info.ver;
            for (int d = 0; d < 10; d++) {
                g_card_desc[idx][d] = info.desc[d];
                g_card_desc_fa[idx][d] = info.desc_fa[d];
                g_card_desc_en[idx][d] = info.desc_en[d];
                g_card_desc_tr[idx][d] = info.desc_tr[d];
                g_card_desc_ru[idx][d] = info.desc_ru[d];
            }
            g_card_bdate[idx] = info.bdate;
            g_card_btime[idx] = info.btime;
            g_card_inputs[idx] = info.inputs;
            g_card_outputs[idx] = info.outputs;
            g_card_active_model[idx] = model;
            if (exists) *exists = true;
            if (serverContacted) *serverContacted = true;
            http.end();
            client.stop();
            return info.ver;
        }
        http.end();
        client.stop();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (serverContacted) *serverContacted = reachedServer;
    // در صورت پیدا نشدن، اولین مدل پیش‌فرض انتخاب می‌شود
    g_card_active_model[idx] = String(candidates[0]);
    return "";
}

// آیا سخت‌افزارِ موردنیازِ یک برنامه موجود است؟
static bool hwreq_ok(uint8_t r)
{
    switch (r)
    {
        case 1: return g_max6675_count >= 1;                           // حداقل یک MAX6675
        case 2: return g_max6675_count >= 2 && g_mcp23017_present;     // دو MAX6675 + MCP23017
        case 3: return g_mcp23017_present;                             // MCP23017
        default: return true;                                         // بدون نیاز
    }
}

static bool config_card_has_newer_server_version(int idx)
{
    if (idx < 0 || idx >= 9) return false;
    if (!g_card_exists[idx]) return false;
    return versionNewer(g_card_ver[idx], String(VERSION));
}

static String config_card_version_text(int idx)
{
    if (idx < 0 || idx >= 9) return "";
    if (!g_card_exists[idx]) return String(tr(STR_NOT_AVAILABLE));
    String t = g_card_ver[idx];
    if (t.length() && t[0] != 'v' && t[0] != 'V') t = "V" + t;
    return t;
}

static void config_apply_card_button_color(int idx)
{
    if (idx < 0 || idx >= 9) return;
    lv_obj_t *btn = objects.card_btn[idx];
    if (!btn || !lv_obj_is_valid(btn)) return;

    lv_obj_t *bl = lv_obj_get_child(btn, 0);
    bool isInstalledProgram = (idx == 8 || (g_firmware_suite == SUITE_OTHER && idx == 3));

    // اولویت ۱: برنامه روی سرور موجود است (استعلام با موفقیت انجام شده و آماده نصب است)
    if (g_card_exists[idx])
    {
        bool isNewer = versionNewer(g_card_ver[idx], String(VERSION));

        if (isInstalledProgram && !isNewer)
        {
            // برنامه بروز است -> دکمه زرد با عنوان «بروزرسانی مجدد»
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xffd97706), 0);
            if (bl && lv_obj_is_valid(bl)) lv_label_set_text(bl, tr(STR_REINSTALL));
        }
        else
        {
            // نسخه جدید یا برنامه آماده نصب -> دکمه سبز زمردی اپل
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xff059669), 0);
            if (bl && lv_obj_is_valid(bl)) lv_label_set_text(bl, tr(STR_INSTALL));
        }
        return;
    }

    // اولویت ۲: کارت استعلام شده اما روی سرور وجود ندارد
    if (g_card_checked[idx])
    {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xffdc2626), 0); // قرمز
        if (bl && lv_obj_is_valid(bl)) lv_label_set_text(bl, tr(STR_UNAVAILABLE_BTN));
        return;
    }

    // اولویت ۳: پیش‌فرض دکمه قبل از اتمام استعلام: دکمه سبز «نصب» (یا کهربایی «بروزرسانی مجدد»)
    // کلید هرگز روی «در حال بررسی» ۱۰ دقیقه معطل نمی‌ماند و همیشه آماده لمس سریع است
    if (isInstalledProgram)
    {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xffd97706), 0);
        if (bl && lv_obj_is_valid(bl)) lv_label_set_text(bl, tr(STR_REINSTALL));
    }
    else
    {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xff059669), 0);
        if (bl && lv_obj_is_valid(bl)) lv_label_set_text(bl, tr(STR_INSTALL));
    }
}

// اعمال سازگاریِ سخت‌افزار روی همه‌ی کارت‌ها (فقط از نخ LVGL)
static void config_apply_hw_compat()
{
    int card_count = (g_firmware_suite == SUITE_OTHER) ? 4 : 9;
    for (int i = 0; i < card_count; i++)
    {
        lv_obj_t *inc = objects.card_incompat[i];
        if (inc && lv_obj_is_valid(inc)) lv_obj_add_flag(inc, LV_OBJ_FLAG_HIDDEN);
        config_apply_card_button_color(i);
    }
}

// نمایش وضعیت نسخه و به‌روزرسانی کارت‌ها (فقط از نخ LVGL)
static void config_apply_card_versions_with_incompat()
{
    int card_count = (g_firmware_suite == SUITE_OTHER) ? 4 : 9;
    for (int i = 0; i < card_count; i++)
    {
        lv_obj_t *l = objects.card_ver[i];
        if (!l || !lv_obj_is_valid(l)) continue;

        lv_obj_t *inc = objects.card_incompat[i];
        if (inc && lv_obj_is_valid(inc)) lv_obj_add_flag(inc, LV_OBJ_FLAG_HIDDEN);

        // نمایش آنی نسخه به محض دریافت از سرور (بدون معطل ماندن برای سایر کارت‌ها)
        if (g_card_exists[i])
        {
            bool isInstalledProgram = (i == 8 || (g_firmware_suite == SUITE_OTHER && i == 3));
            bool isNewer = versionNewer(g_card_ver[i], String(VERSION));
            String v = g_card_ver[i];
            if (v.length() && v[0] != 'v' && v[0] != 'V') v = "V" + v;

            if (isInstalledProgram && !isNewer)
            {
                String lbl = String(tr(STR_PROGRAM_UP_TO_DATE)) + " (" + v + ")";
                lv_label_set_text(l, lbl.c_str());
                lv_obj_set_style_text_color(l, lv_color_hex(0xfff59e0b), 0); // زرد
            }
            else
            {
                String lbl = isNewer ? (String(tr(STR_NEW_VERSION)) + v) : v;
                lv_label_set_text(l, lbl.c_str());
                lv_obj_set_style_text_color(l, lv_color_hex(0xff10b981), 0); // سبز زمردی
            }
        }
        else if (g_card_checked[i])
        {
            lv_label_set_text(l, tr(STR_NOT_ON_SERVER));
            lv_obj_set_style_text_color(l, lv_color_hex(0xffef4444), 0); // قرمز
        }
        else if (!g_card_ver_fetched && (WiFi.status() != WL_CONNECTED || connectingToWifi || isSearching || (g_card_query_start_ms > 0 && millis() - g_card_query_start_ms < 15000)))
        {
            lv_label_set_text(l, tr(STR_CONNECTING_TO_SERVER));
            lv_obj_set_style_text_color(l, lv_color_hex(0xfff59e0b), 0); // زرد کهربایی
        }
        else
        {
            lv_label_set_text(l, tr(STR_NOT_ON_SERVER));
            lv_obj_set_style_text_color(l, lv_color_hex(0xffef4444), 0); // قرمز
        }
        config_apply_card_button_color(i);
    }
}

static void config_apply_card_versions()
{
    config_apply_card_versions_with_incompat();
}

// کوتاه‌کردنِ هوشمندِ خطِ توضیح با اندازه‌گیری دقیق عرض متن بر اساس فونت کلمه بولد (ui_font_kalame_bold_14)
// و حذف کران‌دار بر اساس مرز کاراکترهای معتبر UTF-8 تا هیچ‌گاه کاراکتر چندبایتی ناقص نشده و گلیف مربعی نمایش داده نشود.
static String config_fit_confirm_line(const String &s)
{
    int32_t maxW = 396;
    if (objects.confirm_label && lv_obj_is_valid(objects.confirm_label))
    {
        int32_t w = lv_obj_get_style_width(objects.confirm_label, LV_PART_MAIN);
        if (w > 40) maxW = w - 8;
    }
    String t = s;
    cfg_strip_bullet(t);
    cfg_sanitize_display_text(t);

    while (t.length() > 0)
    {
        String probe = "- " + t;
        lv_point_t size;
        lv_text_get_size(&size, probe.c_str(), &ui_font_kalame_bold_14, 0, 0, 0, LV_TEXT_FLAG_NONE);
        if (size.x <= maxW) return probe;

        // حذف امن آخرین کاراکتر معتبر UTF-8 با بازگشت از بایت‌های دنباله 10xxxxxx
        int lastIdx = (int)t.length() - 1;
        while (lastIdx > 0 && ((uint8_t)t[lastIdx] & 0xC0) == 0x80) {
            lastIdx--;
        }
        t.remove(lastIdx);
        t.trim();
    }
    return "- " + t;
}

static void config_request_install(int idx)
{
    if (idx < 0 || idx >= 9) return;
    g_pending_card = idx;

    // اگر کارت هنوز چک نشده، app_task در Core0 به‌زودی آن را به‌روز می‌کند.
    // هرگز HTTP blocking را از Core1 (LVGL thread) صدا نزن — UI فریز می‌شود.
    // در این حالت همانند «موجود نیست» رفتار می‌کنیم تا اپراتور بداند صبر کند.

    if (!g_card_exists[idx])
    {
        // برنامه روی سرور نیست یا هنوز استعلام نشده -> نمایش مدال هشدار شفاف بدون دانلود
        StrId nameId = (g_firmware_suite == SUITE_OTHER && idx < 4) ? g_other_card_name[idx] : g_card_name[idx];
        String txt = String(tr(nameId)) + "\n\n";
        bool notCheckedYet = !g_card_checked[idx];
        if (notCheckedYet && (WiFi.status() == WL_CONNECTED || connectingToWifi || isSearching || (g_card_query_start_ms > 0 && millis() - g_card_query_start_ms < 15000))) {
            // در حال اتصال به شبکه یا استعلام سرور — کاربر باید چند لحظه صبر کند
            if (g_lang == LANG_FA)       txt += "در حال اتصال به اینترنت و بررسی سرور...\nلطفاً چند لحظه صبر کنید و مجدداً امتحان نمایید.";
            else if (g_lang == LANG_TR)  txt += "Sunucu kontrol ediliyor...\nLutfen birkac saniye bekleyin ve tekrar deneyin.";
            else if (g_lang == LANG_RU)  txt += "Proverka servera...\nPodozhdite neskol'ko sekund i poprobuyte snova.";
            else                         txt += "Connecting and checking server...\nPlease wait a few moments and try again.";
        } else {
            if (g_lang == LANG_FA)       txt += "برنامه مورد نظر روی سرور موجود نیست\nیا ارتباط با سرور برقرار نشد.\n\nلطفاً اتصال اینترنت و سرور را بررسی فرمایید.";
            else if (g_lang == LANG_TR)  txt += "Ilgili program sunucuda mevcut degil\nveya baglanti kurulamadi.\n\nLutfen internet ve sunucu baglantisini kontrol edin.";
            else if (g_lang == LANG_RU)  txt += "Programma ne naydena na servere\nili net soedineniya.\n\nPozhaluysta prover'te podklyuchenie k internetu.";
            else                         txt += "Program not available on server\nor server connection failed.\n\nPlease check internet and server connection.";
        }

        if (objects.confirm_label && lv_obj_is_valid(objects.confirm_label))
            lv_label_set_text(objects.confirm_label, txt.c_str());

        // مخفی‌کردن دکمه بله و وسط‌چین کردن دکمه بستن
        if (objects.confirm_yes && lv_obj_is_valid(objects.confirm_yes))
            lv_obj_add_flag(objects.confirm_yes, LV_OBJ_FLAG_HIDDEN);

        if (objects.confirm_no && lv_obj_is_valid(objects.confirm_no))
        {
            lv_obj_align(objects.confirm_no, LV_ALIGN_BOTTOM_MID, 0, -14);
            lv_obj_t *nlbl = lv_obj_get_child(objects.confirm_no, 0);
            if (nlbl && lv_obj_is_valid(nlbl))
                lv_label_set_text(nlbl, g_lang == LANG_FA ? "متوجه شدم" : "OK");
        }

        if (objects.confirm_panel && lv_obj_is_valid(objects.confirm_panel))
        {
            lv_obj_move_foreground(objects.confirm_panel);
            lv_obj_clear_flag(objects.confirm_panel, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    // برنامه موجود است -> بازگرداندن دکمه‌های تایید و انصراف به حالت استاندارد
    if (objects.confirm_yes && lv_obj_is_valid(objects.confirm_yes))
    {
        lv_obj_clear_flag(objects.confirm_yes, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(objects.confirm_yes, LV_ALIGN_BOTTOM_LEFT, 20, -14);
    }
    if (objects.confirm_no && lv_obj_is_valid(objects.confirm_no))
    {
        lv_obj_align(objects.confirm_no, LV_ALIGN_BOTTOM_RIGHT, -20, -14);
        lv_obj_t *nlbl = lv_obj_get_child(objects.confirm_no, 0);
        if (nlbl && lv_obj_is_valid(nlbl))
            lv_label_set_text(nlbl, tr(STR_NO));
    }

    String ver = g_card_ver[idx];
    if (ver.length() && ver[0] != 'v' && ver[0] != 'V') ver = "V" + ver;
    const char *verLabel = (g_lang == LANG_FA) ? "ورژن: " : (g_lang == LANG_TR ? "Versiyon: " : (g_lang == LANG_RU ? "Versiya: " : "Version: "));
    StrId nameId = (g_firmware_suite == SUITE_OTHER && idx < 4) ? g_other_card_name[idx] : g_card_name[idx];
    String txt = String(tr(nameId)) + "\n" + verLabel + ver + "\n";
    // توضیحاتِ کوتاهِ تغییرات (تا ۸ خطِ «- ») به زبان کاربر + خطِ بیلد
    for (int d = 0; d < 8; d++) {
        const char *cd = get_card_desc_line(idx, d);
        if (cd && cd[0]) {
            txt += config_fit_confirm_line(String(cd)) + "\n";
        }
    }
    if (g_card_bdate[idx].length())
        txt += "build: " + g_card_bdate[idx]
             + (g_card_btime[idx].length() ? (" " + g_card_btime[idx]) : String("")) + "\n";
    txt += tr(STR_CONFIRM_Q);

    if (objects.confirm_label && lv_obj_is_valid(objects.confirm_label))
        lv_label_set_text(objects.confirm_label, txt.c_str());

    if (objects.confirm_panel && lv_obj_is_valid(objects.confirm_panel))
    {
        lv_obj_move_foreground(objects.confirm_panel);
        lv_obj_clear_flag(objects.confirm_panel, LV_OBJ_FLAG_HIDDEN);
        firmware_confirm_countdown_start();
    }
}

static void config_hide_confirm()
{
    firmware_confirm_countdown_stop();
    if (objects.confirm_panel && lv_obj_is_valid(objects.confirm_panel))
        lv_obj_add_flag(objects.confirm_panel, LV_OBJ_FLAG_HIDDEN);
}

// شروع دانلودِ برنامه‌ی تأییدشده
static void config_start_install(int idx)
{
    if (idx < 0 || idx >= 9) return;
    if (objects.waiting_panle_1 && lv_obj_is_valid(objects.waiting_panle_1))
        lv_obj_move_foreground(objects.waiting_panle_1);
    set_var_download_panle_state(true);
    StrId nameId = (g_firmware_suite == SUITE_OTHER && idx < 4) ? g_other_card_name[idx] : g_card_name[idx];
    set_var_download_lable_info_text((String(tr(STR_DL_PREFIX)) + tr(nameId)).c_str());
    if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
    {
        const char *defModel = (g_firmware_suite == SUITE_OTHER && idx < 4) ?
            g_other_card_model_candidates[idx][0] : g_card_model_candidates[idx][0];
        ota_download_model = g_card_active_model[idx].length() ? g_card_active_model[idx] : String(defModel);
        ota_download = true;
        xSemaphoreGive(xMutex_Wifi);
    }
}

static void config_confirm_yes()
{
    config_hide_confirm();
    if (g_pending_card >= 0) { config_start_install(g_pending_card); g_pending_card = -1; }
}
static void config_confirm_no()
{
    config_hide_confirm();
    g_pending_card = -1;
    // بازگرداندن دکمه‌های تایید و انصراف برای استفاده‌های بعدی
    if (objects.confirm_yes && lv_obj_is_valid(objects.confirm_yes))
    {
        lv_obj_clear_flag(objects.confirm_yes, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(objects.confirm_yes, LV_ALIGN_BOTTOM_LEFT, 20, -14);
    }
    if (objects.confirm_no && lv_obj_is_valid(objects.confirm_no))
    {
        lv_obj_align(objects.confirm_no, LV_ALIGN_BOTTOM_RIGHT, -20, -14);
        lv_obj_t *nlbl = lv_obj_get_child(objects.confirm_no, 0);
        if (nlbl && lv_obj_is_valid(nlbl))
            lv_label_set_text(nlbl, tr(STR_NO));
    }
}

// مقایسه‌ی نسخه‌ی معنایی (major.minor.patch). true اگر server از current جدیدتر باشد.
static bool versionNewer(String server, String current)
{
    server.trim();
    current.trim();
    if (server.length() && (server[0] == 'v' || server[0] == 'V')) server = server.substring(1);
    if (current.length() && (current[0] == 'v' || current[0] == 'V')) current = current.substring(1);
    int a[3] = {0, 0, 0}, b[3] = {0, 0, 0};
    sscanf(server.c_str(), "%d.%d.%d", &a[0], &a[1], &a[2]);
    sscanf(current.c_str(), "%d.%d.%d", &b[0], &b[1], &b[2]);
    for (int i = 0; i < 3; i++)
        if (a[i] != b[i]) return a[i] > b[i];
    return false;
}

// (lv_tick_task حذف شد: تیک تایمر مستقیماً توسط سخت‌افزار esp_timer با دقت میکروثانیه مدیریت می‌شود)

extern volatile uint32_t g_flush_count;

#ifndef HMI_PERF_SERIAL_LOG
#define HMI_PERF_SERIAL_LOG 0
#endif

// ===================== ناوبریِ کلیدهای پنل (صفحاتِ Config، بدونِ تاچ) =====================
// کلیدهای خارجی روی GPIO2 (آنالوگ، باندِ ولتاژ) — مثلِ hwtest. نگاشتِ کاربر:
//   F6=SAVE(~4095)  F7=CANCEL(~3320)  F8=MODE(~2710)  F9=START(~1955)  F10=STOP(~1211)
// MODE=حرکتِ فوکوس، START/STOP=تأیید (یا بالا/پایین در دراپ‌داون)، SAVE=تأیید، CANCEL=انصراف/بستنِ دیالوگ
enum PanelKey { PK_NONE = 0, PK_SAVE, PK_CANCEL, PK_MODE, PK_START, PK_STOP };

// سه وضعیت اصلی ماشین حالت FSM ناوبری پنل (بند ۲ TOUCHLESS_PANEL_NAVIGATION_PROMPT)
typedef enum {
    NAV_STATE_IDLE = 0,       // نشانگر محو؛ حالت اجرای عادی دستگاه و کار با تاچ
    NAV_STATE_ACTIVE = 1,     // نشانگر فوکوس فعال؛ پیمایش و تنظیم ویجت‌های صفحه
    NAV_STATE_KEYBOARD = 2    // نشانگر داخل ماتریس کلیدهای کیبورد مجازی
} PanelNavState;

extern lv_indev_t *g_touch_indev;   // دستگاه تاچ خازنی جهت لغو فوری ناوبری هنگام لمس

// معماری تک‌نشانگر هوشمند پنل (Apple HIG & Siemens Industrial Rigor)
#define NAV_MAX_ITEMS 64
static lv_obj_t      *s_nav_items[NAV_MAX_ITEMS];
static uint16_t       s_nav_count = 0;
static int16_t        s_nav_index = -1;
static lv_obj_t      *s_cur_focused_obj = NULL;
static PanelNavState  s_nav_state = NAV_STATE_IDLE;
static uint32_t       s_last_key_activity_ms = 0;

static lv_obj_t   *g_nav_root_sig = NULL;
static uint32_t    g_nav_child_sig = 0;
static bool        g_nav_modal_visible_sig = false;
static bool        g_nav_off = false;

// متغیرهای تکرار خودکار (Auto-Repeat) کلیدهای START و STOP (بند ۴.۱.۳)
static uint32_t s_hold_start_ms = 0;
static uint32_t s_last_repeat_tick_ms = 0;
static int      s_holding_key = PK_NONE;

// ===== سیستم کارت بنر و توست شناور تایمردار (بند ۵ TOUCHLESS_PANEL_NAVIGATION_PROMPT) =====
static lv_obj_t *s_toast_card = NULL;
static lv_obj_t *s_toast_label = NULL;
static lv_obj_t *s_toast_bar = NULL;
static uint32_t  s_toast_start_ms = 0;
static uint32_t  s_toast_duration_ms = 0;
static bool      s_toast_paused = false;
static uint32_t  s_toast_pause_start_ms = 0;

static void toast_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_toast_paused = true;
        s_toast_pause_start_ms = millis();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_toast_paused) {
            uint32_t held_ms = millis() - s_toast_pause_start_ms;
            s_toast_paused = false;
            if (held_ms < 300) {
                if (s_toast_card && lv_obj_is_valid(s_toast_card)) {
                    lv_obj_del(s_toast_card);
                    s_toast_card = NULL;
                    s_toast_label = NULL;
                    s_toast_bar = NULL;
                }
                return;
            }
            s_toast_start_ms += held_ms;
        }
    } else if (code == LV_EVENT_CLICKED) {
        if (s_toast_card && lv_obj_is_valid(s_toast_card)) {
            lv_obj_del(s_toast_card);
            s_toast_card = NULL;
            s_toast_label = NULL;
            s_toast_bar = NULL;
        }
    }
}

static void panel_toast_dismiss(void)
{
    if (s_toast_card && lv_obj_is_valid(s_toast_card)) {
        lv_obj_del(s_toast_card);
    }
    s_toast_card = NULL;
    s_toast_label = NULL;
    s_toast_bar = NULL;
    s_toast_paused = false;
}

static void panel_toast_show(const char *msg, lv_color_t color_accent, uint32_t duration_ms = 3500)
{
    if (!msg || msg[0] == '\0') return;

    panel_toast_dismiss();

    lv_obj_t *top_layer = lv_layer_top();
    if (!top_layer) return;

    // کارت شناور مدرن اپل در بالاترین لایه نمایش
    s_toast_card = lv_obj_create(top_layer);
    lv_obj_set_size(s_toast_card, 380, LV_SIZE_CONTENT);
    lv_obj_align(s_toast_card, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_clear_flag(s_toast_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_toast_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(s_toast_card, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_toast_card, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_toast_card, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_toast_card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_toast_card, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_toast_card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_toast_card, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_top(s_toast_card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(s_toast_card, 12, LV_PART_MAIN);
    lv_obj_add_event_cb(s_toast_card, toast_event_cb, LV_EVENT_ALL, NULL);

    s_toast_label = lv_label_create(s_toast_card);
    lv_label_set_text(s_toast_label, msg);
    lv_obj_set_width(s_toast_label, LV_PCT(100));
    lv_obj_set_style_text_align(s_toast_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_toast_label, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_toast_label, &ui_font_kalame_bold_14, LV_PART_MAIN);

    s_toast_bar = lv_bar_create(s_toast_card);
    lv_obj_set_size(s_toast_bar, LV_PCT(100), 3);
    lv_obj_align(s_toast_bar, LV_ALIGN_BOTTOM_MID, 0, 8);
    lv_bar_set_range(s_toast_bar, 0, 1000);
    lv_bar_set_value(s_toast_bar, 1000, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_toast_bar, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_toast_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_toast_bar, color_accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_toast_bar, LV_OPA_COVER, LV_PART_INDICATOR);

    s_toast_start_ms = millis();
    s_toast_duration_ms = (duration_ms > 0) ? duration_ms : 3500;
    s_toast_paused = false;
}

static void panel_toast_tick(uint32_t now)
{
    if (!s_toast_card || !lv_obj_is_valid(s_toast_card)) {
        s_toast_card = NULL;
        return;
    }

    if (s_toast_paused) return;

    uint32_t elapsed = now - s_toast_start_ms;
    if (elapsed >= s_toast_duration_ms) {
        panel_toast_dismiss();
        return;
    }

    if (s_toast_bar && lv_obj_is_valid(s_toast_bar)) {
        int32_t val = (int32_t)(((int64_t)(s_toast_duration_ms - elapsed) * 1000) / s_toast_duration_ms);
        if (val < 0) val = 0;
        lv_bar_set_value(s_toast_bar, val, LV_ANIM_OFF);
    }
}

// نگاشت کلیدهای کنترل پنل F6..F10 با استفاده از فیلتر میانه ۵ نمونه‌ای و باندهای عریض استاندارد
static inline int pk_classify(int adc)
{
    uint8_t k = getKeyFromAnalog((uint16_t)adc);
    switch (k) {
        case 1: return PK_SAVE;    // F6 (~4095)
        case 2: return PK_CANCEL;  // F7 (~3320)
        case 3: return PK_MODE;    // F8 (~2710)
        case 4: return PK_START;   // F9 (~1955)
        case 5: return PK_STOP;    // F10 (~1211)
        default: return PK_NONE;
    }
}

static inline int pk_read(void)
{
    return pk_classify((int)readMedian5(2));
}

// نگاشت کلیدهای فانکشن F1..F5 روی GPIO1
enum FkKey { FK_NONE = 0, FK_F1 = 1, FK_F2 = 2, FK_F3 = 3, FK_F4 = 4, FK_F5 = 5 };

static inline int fk_classify(int adc)
{
    uint8_t k = getKeyFromAnalog((uint16_t)adc);
    switch (k) {
        case 1: return FK_F1;      // F1 (~4095)
        case 2: return FK_F2;      // F2 (~3320)
        case 3: return FK_F3;      // F3 (~2710)
        case 4: return FK_F4;      // F4 (~1955)
        case 5: return FK_F5;      // F5 (~1211)
        default: return FK_NONE;
    }
}

static inline int fk_read(void)
{
    return fk_classify((int)readMedian5(1));
}

// فیلتر انحصاری و دقیق المان‌های سناریودار (بند ۳.۱ TOUCHLESS_PANEL_NAVIGATION_PROMPT)
static bool is_actionable_object(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;

    // بررسی سلسله‌مراتب کامل والدها: اگر والدی پنهان باشد، فرزند هرگز فوکوس نمی‌گیرد
    lv_obj_t *p = lv_obj_get_parent(obj);
    while (p) {
        if (lv_obj_has_flag(p, LV_OBJ_FLAG_HIDDEN)) return false;
        p = lv_obj_get_parent(p);
    }

    // المان‌های صرفاً بصری، متنی و دکوراتیو هرگز نباید نشانگر بگیرند
    if (lv_obj_check_type(obj, &lv_label_class)) return false;
    if (lv_obj_check_type(obj, &lv_image_class)) return false;
    if (lv_obj_check_type(obj, &lv_line_class)) return false;
    if (lv_obj_check_type(obj, &lv_bar_class)) return false;

    // المان‌های سناریودار و تعاملی مجاز
    if (lv_obj_check_type(obj, &lv_button_class)) return true;
    if (lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class)) return true;
    if (lv_obj_check_type(obj, &lv_arc_class)) return true;
    if (lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
    if (lv_obj_check_type(obj, &lv_checkbox_class)) return true;
    if (lv_obj_check_type(obj, &lv_roller_class)) return true;
    if (lv_obj_check_type(obj, &lv_spinbox_class)) return true;
    if (lv_obj_check_type(obj, &lv_keyboard_class)) return true;
    if (lv_obj_check_type(obj, &lv_buttonmatrix_class)) return true;
    if (lv_obj_check_type(obj, &lv_imagebutton_class)) return true;
    if (lv_obj_check_type(obj, &lv_list_button_class)) return true;

    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
    {
        if (!lv_obj_check_type(obj, &lv_obj_class)) return true;
        // کارت‌های تعاملی با حداکثر ۳ فرزند
        if (lv_obj_get_child_count(obj) <= 3) return true;
    }

    return false;
}

static void nav_collect_actionable(lv_obj_t *parent)
{
    if (!parent || !lv_obj_is_valid(parent)) return;
    if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) return;

    if (is_actionable_object(parent))
    {
        if (s_nav_count < NAV_MAX_ITEMS)
        {
            s_nav_items[s_nav_count++] = parent;
        }
        return;
    }

    uint32_t n = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < n; i++)
    {
        lv_obj_t *c = lv_obj_get_child(parent, i);
        nav_collect_actionable(c);
    }
}

// مرتب‌سازی هندسی نشانگر: حرکت کاملاً ترتیبی و استاندارد بر اساس موقعیت بصری المان‌ها (بالا به پایین، چپ به راست)
static void nav_sort_items_geometric(void)
{
    if (s_nav_count <= 1) return;

    lv_area_t areas[NAV_MAX_ITEMS];
    for (int i = 0; i < s_nav_count; i++) {
        lv_obj_get_coords(s_nav_items[i], &areas[i]);
    }

    for (int i = 0; i < s_nav_count - 1; i++) {
        for (int j = 0; j < s_nav_count - i - 1; j++) {
            int32_t y1 = areas[j].y1;
            int32_t y2 = areas[j + 1].y1;
            int32_t x1 = areas[j].x1;
            int32_t x2 = areas[j + 1].x1;

            bool swap = false;
            if (abs((int)(y1 - y2)) > 16) {
                if (y1 > y2) swap = true;
            } else {
                if (x1 > x2) swap = true;
            }

            if (swap) {
                lv_obj_t *tmp_obj = s_nav_items[j];
                s_nav_items[j] = s_nav_items[j + 1];
                s_nav_items[j + 1] = tmp_obj;

                lv_area_t tmp_area = areas[j];
                areas[j] = areas[j + 1];
                areas[j + 1] = tmp_area;
            }
        }
    }
}

static uint32_t nav_child_signature(lv_obj_t *root)
{
    if (!root) return 0;
    return lv_obj_get_child_count(root);
}

extern "C" lv_obj_t *get_pinmap_panel(void);

// قرنطینه کامل مودال‌ها (بند ۳.۲ TOUCHLESS_PANEL_NAVIGATION_PROMPT)
static lv_obj_t *nav_root(lv_obj_t *scr)
{
    if (objects.confirm_panel && lv_obj_is_valid(objects.confirm_panel)
        && !lv_obj_has_flag(objects.confirm_panel, LV_OBJ_FLAG_HIDDEN))
        return objects.confirm_panel;
    if (objects.link_entry_panel && lv_obj_is_valid(objects.link_entry_panel)
        && !lv_obj_has_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN))
        return objects.link_entry_panel;
    if (objects.version_panel && lv_obj_is_valid(objects.version_panel)
        && !lv_obj_has_flag(objects.version_panel, LV_OBJ_FLAG_HIDDEN))
        return objects.version_panel;
    if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password)
        && !lv_obj_has_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN))
        return objects.panle_wifi_password;
    lv_obj_t *pm = get_pinmap_panel();
    if (pm && lv_obj_is_valid(pm) && !lv_obj_has_flag(pm, LV_OBJ_FLAG_HIDDEN))
        return pm;
    return scr;
}

static bool nav_modal_visible(void)
{
    if (objects.confirm_panel && lv_obj_is_valid(objects.confirm_panel)
        && !lv_obj_has_flag(objects.confirm_panel, LV_OBJ_FLAG_HIDDEN))
        return true;
    if (objects.link_entry_panel && lv_obj_is_valid(objects.link_entry_panel)
        && !lv_obj_has_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN))
        return true;
    if (objects.version_panel && lv_obj_is_valid(objects.version_panel)
        && !lv_obj_has_flag(objects.version_panel, LV_OBJ_FLAG_HIDDEN))
        return true;
    if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password)
        && !lv_obj_has_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN))
        return true;
    lv_obj_t *pm = get_pinmap_panel();
    if (pm && lv_obj_is_valid(pm) && !lv_obj_has_flag(pm, LV_OBJ_FLAG_HIDDEN))
        return true;
    return false;
}

static inline bool is_keyboard_type(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return false;
    return lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_buttonmatrix_class);
}

static void nav_clear_focus(void)
{
    if (s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj))
    {
        lv_obj_set_style_outline_width(s_cur_focused_obj, 0, LV_STATE_DEFAULT);
        lv_obj_clear_state(s_cur_focused_obj, LV_STATE_FOCUSED);
        if (is_keyboard_type(s_cur_focused_obj))
        {
            lv_buttonmatrix_set_selected_button(s_cur_focused_obj, LV_BUTTONMATRIX_BUTTON_NONE);
            lv_obj_invalidate(s_cur_focused_obj);
        }
    }
    s_cur_focused_obj = NULL;
    s_nav_index = -1;
    s_nav_state = NAV_STATE_IDLE;
}

static void nav_set_focus_to(lv_obj_t *obj)
{
    if (s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj) && s_cur_focused_obj != obj)
    {
        lv_obj_set_style_outline_width(s_cur_focused_obj, 0, LV_STATE_DEFAULT);
        lv_obj_clear_state(s_cur_focused_obj, LV_STATE_FOCUSED);
        if (is_keyboard_type(s_cur_focused_obj))
        {
            lv_buttonmatrix_set_selected_button(s_cur_focused_obj, LV_BUTTONMATRIX_BUTTON_NONE);
            lv_obj_invalidate(s_cur_focused_obj);
        }
    }

    s_cur_focused_obj = obj;

    if (obj && lv_obj_is_valid(obj))
    {
        lv_obj_update_layout(obj);
        lv_obj_add_state(obj, LV_STATE_FOCUSED);
        if (is_keyboard_type(obj))
        {
            s_nav_state = NAV_STATE_KEYBOARD;
            lv_obj_set_style_outline_width(obj, 0, LV_STATE_DEFAULT);
            lv_buttonmatrix_set_selected_button(obj, 0);
            lv_obj_invalidate(obj);
        }
        else
        {
            s_nav_state = NAV_STATE_ACTIVE;
            // نشانگر استاندارد اپل: ۳ پیکسل آبی نئونی (#0EA5E9) با پد ۲ پیکسل
            lv_obj_set_style_outline_width(obj, 3, LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0x0EA5E9), LV_STATE_DEFAULT);
            lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_STATE_DEFAULT);
            lv_obj_set_style_outline_pad(obj, 2, LV_STATE_DEFAULT);
        }
        lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON);
    }
    else
    {
        s_nav_state = NAV_STATE_IDLE;
    }
}

// چرخه پیمایش ترتیبی با کلید MODE (بند ۲.۱ TOUCHLESS_PANEL_NAVIGATION_PROMPT)
static void nav_do_mode(lv_obj_t *root)
{
    app_sound_play(SOUND_TAP);

    // حالت ناوبری داخل ماتریس کلیدهای کیبورد مجازی
    if (s_nav_state == NAV_STATE_KEYBOARD && s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj) && is_keyboard_type(s_cur_focused_obj))
    {
        uint32_t sel = lv_buttonmatrix_get_selected_button(s_cur_focused_obj);
        uint32_t total = 0;
        const char * const *map = lv_buttonmatrix_get_map(s_cur_focused_obj);
        if (map) {
            for (int m = 0; map[m] && map[m][0] != '\0'; m++) {
                if (strcmp(map[m], "\n") != 0) total++;
            }
        }
        if (total == 0) total = 12;

        if (sel == LV_BUTTONMATRIX_BUTTON_NONE) {
            lv_buttonmatrix_set_selected_button(s_cur_focused_obj, 0);
            lv_obj_invalidate(s_cur_focused_obj);
            return;
        } else if (sel + 1 < total) {
            lv_buttonmatrix_set_selected_button(s_cur_focused_obj, sel + 1);
            lv_obj_invalidate(s_cur_focused_obj);
            return;
        } else {
            // انتهای کلیدهای کیپد -> خروج تمیز از کیپد و رفتن به آیتم بعدی صفحه
            lv_buttonmatrix_set_selected_button(s_cur_focused_obj, LV_BUTTONMATRIX_BUTTON_NONE);
            lv_obj_clear_state(s_cur_focused_obj, LV_STATE_FOCUSED);
            lv_obj_invalidate(s_cur_focused_obj);
            s_nav_state = NAV_STATE_ACTIVE;
        }
    }
    else if (s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj)
        && lv_obj_check_type(s_cur_focused_obj, &lv_dropdown_class)
        && lv_dropdown_is_open(s_cur_focused_obj))
    {
        uint32_t cnt = lv_dropdown_get_option_cnt(s_cur_focused_obj);
        uint32_t sel = lv_dropdown_get_selected(s_cur_focused_obj);
        if (cnt) lv_dropdown_set_selected(s_cur_focused_obj, (sel + 1) % cnt);
        return;
    }

    // ورود به حالت ناوبری از وضعیت غیرفعال (NAV_STATE_IDLE)
    if (s_nav_state == NAV_STATE_IDLE || s_nav_index < 0 || !s_cur_focused_obj)
    {
        s_nav_count = 0;
        if (root) {
            nav_collect_actionable(root);
            nav_sort_items_geometric();
        }
        if (s_nav_count > 0)
        {
            s_nav_index = 0;
            nav_set_focus_to(s_nav_items[0]);
        }
        return;
    }

    // حرکت به آیتم بعدی
    s_nav_index++;

    // قاعده انتهای چرخه (Wrap-around): خروج کامل از ناوبری و بازگشت به IDLE
    if (s_nav_index >= s_nav_count)
    {
        nav_clear_focus();
        panel_toast_show("خروج از حالت ناوبری", lv_color_hex(0x0EA5E9), 2500);
        return;
    }

    // پیمایش به اولین آیتم فعال و مرئی بعدی
    while (s_nav_index < s_nav_count)
    {
        lv_obj_t *target = s_nav_items[s_nav_index];
        if (target && lv_obj_is_valid(target) && !lv_obj_has_flag(target, LV_OBJ_FLAG_HIDDEN) && !lv_obj_has_state(target, LV_STATE_DISABLED))
        {
            nav_set_focus_to(target);
            return;
        }
        s_nav_index++;
    }

    // اگر آیتم معتبر دیگری نبود، خروج تمیز
    nav_clear_focus();
    panel_toast_show("خروج از حالت ناوبری", lv_color_hex(0x0EA5E9), 2500);
}

// اکشن کلید START در حالت ناوبری (بند ۴.۱)
static void nav_do_start(void)
{
    if (!s_cur_focused_obj || !lv_obj_is_valid(s_cur_focused_obj)) return;

    app_sound_play(SOUND_TAP);

    if (lv_obj_check_type(s_cur_focused_obj, &lv_dropdown_class))
    {
        if (!lv_dropdown_is_open(s_cur_focused_obj)) {
            lv_dropdown_open(s_cur_focused_obj);
        } else {
            uint32_t cnt = lv_dropdown_get_option_cnt(s_cur_focused_obj);
            uint32_t sel = lv_dropdown_get_selected(s_cur_focused_obj);
            if (cnt) lv_dropdown_set_selected(s_cur_focused_obj, (sel + 1) % cnt);
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
    }
    else if (lv_obj_check_type(s_cur_focused_obj, &lv_switch_class))
    {
        bool st = lv_obj_has_state(s_cur_focused_obj, LV_STATE_CHECKED);
        if (st) lv_obj_clear_state(s_cur_focused_obj, LV_STATE_CHECKED);
        else lv_obj_add_state(s_cur_focused_obj, LV_STATE_CHECKED);
        lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else if (lv_obj_check_type(s_cur_focused_obj, &lv_slider_class))
    {
        int32_t v = lv_slider_get_value(s_cur_focused_obj);
        int32_t mx = lv_slider_get_max_value(s_cur_focused_obj);
        if (v < mx) {
            lv_slider_set_value(s_cur_focused_obj, v + 1, LV_ANIM_ON);
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else {
            panel_toast_show("حداکثر مقدار مجاز", lv_color_hex(0xF59E0B), 2500);
        }
    }
    else if (lv_obj_check_type(s_cur_focused_obj, &lv_arc_class))
    {
        int32_t v = lv_arc_get_value(s_cur_focused_obj);
        int32_t mx = lv_arc_get_max_value(s_cur_focused_obj);
        if (v < mx) {
            lv_arc_set_value(s_cur_focused_obj, v + 1);
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else {
            panel_toast_show("حداکثر مقدار مجاز", lv_color_hex(0xF59E0B), 2500);
        }
    }
    else if (is_keyboard_type(s_cur_focused_obj))
    {
        uint32_t sel = lv_buttonmatrix_get_selected_button(s_cur_focused_obj);
        if (sel != LV_BUTTONMATRIX_BUTTON_NONE) {
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
    }
    else if (lv_obj_has_flag(s_cur_focused_obj, LV_OBJ_FLAG_CLICKABLE))
    {
        lv_obj_send_event(s_cur_focused_obj, LV_EVENT_PRESSED, NULL);
        lv_obj_send_event(s_cur_focused_obj, LV_EVENT_CLICKED, NULL);
    }
}

// اکشن کلید STOP در حالت ناوبری (بند ۴.۱)
static void nav_do_stop(void)
{
    if (!s_cur_focused_obj || !lv_obj_is_valid(s_cur_focused_obj)) return;

    if (lv_obj_check_type(s_cur_focused_obj, &lv_slider_class))
    {
        app_sound_play(SOUND_TAP);
        int32_t v = lv_slider_get_value(s_cur_focused_obj);
        int32_t mn = lv_slider_get_min_value(s_cur_focused_obj);
        if (v > mn) {
            lv_slider_set_value(s_cur_focused_obj, v - 1, LV_ANIM_ON);
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else {
            panel_toast_show("حداقل مقدار مجاز", lv_color_hex(0xF59E0B), 2500);
        }
    }
    else if (lv_obj_check_type(s_cur_focused_obj, &lv_arc_class))
    {
        app_sound_play(SOUND_TAP);
        int32_t v = lv_arc_get_value(s_cur_focused_obj);
        int32_t mn = lv_arc_get_min_value(s_cur_focused_obj);
        if (v > mn) {
            lv_arc_set_value(s_cur_focused_obj, v - 1);
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else {
            panel_toast_show("حداقل مقدار مجاز", lv_color_hex(0xF59E0B), 2500);
        }
    }
    else if (lv_obj_check_type(s_cur_focused_obj, &lv_dropdown_class) && lv_dropdown_is_open(s_cur_focused_obj))
    {
        app_sound_play(SOUND_TAP);
        uint32_t cnt = lv_dropdown_get_option_cnt(s_cur_focused_obj);
        uint32_t sel = lv_dropdown_get_selected(s_cur_focused_obj);
        if (cnt) lv_dropdown_set_selected(s_cur_focused_obj, (sel + cnt - 1) % cnt);
        lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else if (is_keyboard_type(s_cur_focused_obj))
    {
        app_sound_play(SOUND_TAP);
        lv_obj_t *ta = lv_keyboard_get_textarea(s_cur_focused_obj);
        if (!ta && objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
            ta = objects.text_area_framware_password;
        }
        if (!ta && objects.text_area_wifi_password && lv_obj_is_valid(objects.text_area_wifi_password)) {
            ta = objects.text_area_wifi_password;
        }
        if (!ta && objects.link_entry_textarea && lv_obj_is_valid(objects.link_entry_textarea)) {
            ta = objects.link_entry_textarea;
        }
        if (ta) lv_textarea_delete_char(ta);
    }
    // خط قرمز ایمنی زیمنس: کلید STOP روی دکمه‌های فشاری هرگز اکشنی انجام نمی‌دهد!
}

// اکشن کلید SAVE (بند ۶.۲ TOUCHLESS_PANEL_NAVIGATION_PROMPT)
static void nav_do_save(void)
{
    if (s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj))
    {
        if (lv_obj_check_type(s_cur_focused_obj, &lv_dropdown_class) && lv_dropdown_is_open(s_cur_focused_obj))
        {
            lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
            lv_dropdown_close(s_cur_focused_obj);
            return;
        }
        else if (is_keyboard_type(s_cur_focused_obj))
        {
            uint32_t sel = lv_buttonmatrix_get_selected_button(s_cur_focused_obj);
            if (sel != LV_BUTTONMATRIX_BUTTON_NONE) {
                lv_obj_send_event(s_cur_focused_obj, LV_EVENT_VALUE_CHANGED, NULL);
            }
            return;
        }
    }

    // ذخیره سراسری در حافظه ماندگار NVS + بازخورد صوتی دوگانه + توست سبز اپل
    storage.save();
    app_sound_play(SOUND_SUCCESS);
    panel_toast_show("تنظیمات با موفقیت ذخیره شد", lv_color_hex(0x22C55E), 3500);
}

// اکشن کلید CANCEL (بند ۶.۱ TOUCHLESS_PANEL_NAVIGATION_PROMPT)
static void nav_do_cancel(void)
{
    if (objects.waiting_panle_1 && lv_obj_is_valid(objects.waiting_panle_1)
        && !lv_obj_has_flag(objects.waiting_panle_1, LV_OBJ_FLAG_HIDDEN))
    {
        action_download_cancel(NULL);
        nav_clear_focus();
        return;
    }
    if (objects.confirm_panel && lv_obj_is_valid(objects.confirm_panel)
        && !lv_obj_has_flag(objects.confirm_panel, LV_OBJ_FLAG_HIDDEN)
        && objects.confirm_no && lv_obj_is_valid(objects.confirm_no))
    {
        lv_obj_send_event(objects.confirm_no, LV_EVENT_CLICKED, NULL);
        nav_clear_focus();
        return;
    }
    if (objects.link_entry_panel && lv_obj_is_valid(objects.link_entry_panel)
        && !lv_obj_has_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_add_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN);
        nav_clear_focus();
        return;
    }
    if (objects.version_panel && lv_obj_is_valid(objects.version_panel)
        && !lv_obj_has_flag(objects.version_panel, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_add_flag(objects.version_panel, LV_OBJ_FLAG_HIDDEN);
        nav_clear_focus();
        return;
    }
    if (get_pinmap_panel() && lv_obj_is_valid(get_pinmap_panel())
        && !lv_obj_has_flag(get_pinmap_panel(), LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_add_flag(get_pinmap_panel(), LV_OBJ_FLAG_HIDDEN);
        nav_clear_focus();
        return;
    }
    if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password)
        && !lv_obj_has_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        nav_clear_focus();
        return;
    }

    // اگر در مد کیبورد هستیم -> خروج به مد ناوبری معمولی
    if (s_nav_state == NAV_STATE_KEYBOARD)
    {
        if (s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj) && is_keyboard_type(s_cur_focused_obj)) {
            lv_buttonmatrix_set_selected_button(s_cur_focused_obj, LV_BUTTONMATRIX_BUTTON_NONE);
            lv_obj_clear_state(s_cur_focused_obj, LV_STATE_FOCUSED);
            lv_obj_invalidate(s_cur_focused_obj);
        }
        s_nav_state = NAV_STATE_ACTIVE;
        return;
    }

    // اگر در مد ناوبری فعال هستیم -> پاکسازی نشانگر و بازگشت به IDLE
    if (s_nav_state == NAV_STATE_ACTIVE)
    {
        nav_clear_focus();
        return;
    }

    // اگر در صفحات فرعی تست سخت‌افزار هستیم -> بازگشت به منوی اصلی تست
    if (g_hwtest_active)
    {
        uint8_t cur_hwt = hwtest_get_active_screen_id();
        if (cur_hwt != T_SCREEN_ID_MAIN) {
            hwtest_goto_screen_id(T_SCREEN_ID_MAIN);
            return;
        }
    }

    // اگر در صفحه وای‌فای هستیم -> بازگشت به صفحه کارت‌ها
    if (objects.screen_wifi && lv_obj_is_valid(objects.screen_wifi) && lv_screen_active() == objects.screen_wifi)
    {
        if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)) {
            eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
            return;
        }
    }
}

static void panel_nav_tick(void)
{
    uint32_t now = millis();
    lv_obj_t *scr  = lv_screen_active();
    lv_obj_t *root = scr ? nav_root(scr) : NULL;
    bool modalVisible = nav_modal_visible();

    // به‌روزرسانی نوار پیشرفت توست بنر شناور
    panel_toast_tick(now);

    // تغییر ساختار صفحه یا باز/بسته شدن دیالوگ -> پاکسازی فوری فوکوس قبلی
    uint32_t curSig = nav_child_signature(root);
    if (root != g_nav_root_sig || curSig != g_nav_child_sig || modalVisible != g_nav_modal_visible_sig)
    {
        g_nav_root_sig = root;
        g_nav_child_sig = curSig;
        g_nav_modal_visible_sig = modalVisible;
        nav_clear_focus();
        s_nav_count = 0;
    }

    // اولویت مطلق لمس صفحه (بند ۲.۱ E): لمس تاچ فوراً حالت ناوبری را می‌بندد
    if (s_nav_state != NAV_STATE_IDLE && g_touch_indev
        && lv_indev_get_state(g_touch_indev) == LV_INDEV_STATE_PRESSED)
    {
        nav_clear_focus();
    }

    // سگ‌نگهبان ۱۰ ثانیه‌ای عدم فعالیت (بند ۲.۱ D)
    if (s_nav_state != NAV_STATE_IDLE && (now - s_last_key_activity_ms >= 10000))
    {
        nav_clear_focus();
    }

    // نمونه‌برداری با دوره ۴۵ms جهت ایمنی در برابر نویز
    static int lastKey = PK_NONE;
    static uint32_t lastSample = 0;
    if (now - lastSample < 45) return;
    lastSample = now;

    // پردازش کلیدهای ستون اول F1..F5 روی GPIO1
    static int lastFkKey = FK_NONE;
    int fKey = fk_read();
    int prevFk = lastFkKey;
    lastFkKey = fKey;

    if (fKey != FK_NONE && prevFk == FK_NONE)
    {
        s_last_key_activity_ms = now;

        if (s_nav_state == NAV_STATE_KEYBOARD && s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj) && is_keyboard_type(s_cur_focused_obj))
        {
            uint32_t sel = lv_buttonmatrix_get_selected_button(s_cur_focused_obj);
            uint32_t total = 0;
            const char * const *map = lv_buttonmatrix_get_map(s_cur_focused_obj);
            if (map) {
                for (int m = 0; map[m] && map[m][0] != '\0'; m++) {
                    if (strcmp(map[m], "\n") != 0) total++;
                }
            }
            if (total == 0) total = 12;

            if (fKey == FK_F1) // کلید قبلی
            {
                app_sound_play(SOUND_TAP);
                if (sel == LV_BUTTONMATRIX_BUTTON_NONE || sel == 0) sel = total - 1;
                else sel--;
                lv_buttonmatrix_set_selected_button(s_cur_focused_obj, sel);
                lv_obj_invalidate(s_cur_focused_obj);
                return;
            }
            if (fKey == FK_F2) // کلید بعدی
            {
                app_sound_play(SOUND_TAP);
                if (sel == LV_BUTTONMATRIX_BUTTON_NONE || sel + 1 >= total) sel = 0;
                else sel++;
                lv_buttonmatrix_set_selected_button(s_cur_focused_obj, sel);
                lv_obj_invalidate(s_cur_focused_obj);
                return;
            }
            if (fKey == FK_F3) // تایید کلید کیپد
            {
                nav_do_start();
                return;
            }
        }
        else if (s_nav_state == NAV_STATE_ACTIVE && s_cur_focused_obj != NULL && s_nav_count > 0)
        {
            if (fKey == FK_F1) // آیتم قبلی
            {
                app_sound_play(SOUND_TAP);
                if (s_nav_index <= 0) s_nav_index = s_nav_count - 1;
                else s_nav_index--;
                nav_set_focus_to(s_nav_items[s_nav_index]);
                return;
            }
            if (fKey == FK_F2) // آیتم بعدی
            {
                app_sound_play(SOUND_TAP);
                s_nav_index++;
                if (s_nav_index >= s_nav_count) s_nav_index = 0;
                nav_set_focus_to(s_nav_items[s_nav_index]);
                return;
            }
            if (fKey == FK_F3) // تایید / Enter
            {
                nav_do_start();
                return;
            }
        }
        else if (s_nav_state == NAV_STATE_IDLE && !modalVisible)
        {
            // ۱. صفحه رمز ورود ادمین: ورود مستقیم بدون تاچ PIN 123 یا PIN 0000
            if (objects.screen_password && lv_obj_is_valid(objects.screen_password)
                && lv_screen_active() == objects.screen_password)
            {
                lv_obj_t *ta = objects.text_area_framware_password;
                if (ta && lv_obj_is_valid(ta))
                {
                    app_sound_play(SOUND_TAP);
                    if (fKey == FK_F1) { lv_textarea_add_text(ta, "1"); return; }
                    if (fKey == FK_F2) { lv_textarea_add_text(ta, "2"); return; }
                    if (fKey == FK_F3) { lv_textarea_add_text(ta, "3"); return; }
                    if (fKey == FK_F4) { lv_textarea_add_text(ta, "0"); return; }
                    if (fKey == FK_F5) { action_admin_password_redy(NULL); return; }
                }
            }
            // ۲. صفحه لانچر کارت‌ها: میانبر مستقیم به کارت‌های اصلی
            else if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)
                && lv_screen_active() == objects.screen_framware)
            {
                if (fKey == FK_F1) { config_request_install(0); return; }
                if (fKey == FK_F2) { config_request_install(1); return; }
                if (fKey == FK_F3) { config_request_install(2); return; }
                if (fKey == FK_F4) { config_request_install(3); return; }
                if (fKey == FK_F5 && objects.link_entry_panel && lv_obj_is_valid(objects.link_entry_panel)) {
                    lv_obj_move_foreground(objects.link_entry_panel);
                    lv_obj_clear_flag(objects.link_entry_panel, LV_OBJ_FLAG_HIDDEN);
                    return;
                }
            }
        }
    }

    // پردازش کلیدهای ستون دوم F6..F10 روی GPIO2
    int key = pk_read();
    int prev = lastKey;
    lastKey = key;

    // ===== کلیدِ MODE (F8) =====
    static uint32_t modePressStart = 0;
    static bool     modeLongFired  = false;
    if (key == PK_MODE)
    {
        if (prev != PK_MODE) { modePressStart = now; modeLongFired = false; }
        if (!modeLongFired && now - modePressStart >= 3000)
        {
            modeLongFired = true;
            g_nav_off = !g_nav_off;
            nav_clear_focus();
        }
        return;
    }
    if (prev == PK_MODE)
    {
        if (!modeLongFired)
        {
            s_last_key_activity_ms = now;
            if (g_nav_off) g_nav_off = false;
            nav_do_mode(root);
        }
        return;
    }

    if (g_nav_off) return;

    // مدیریت نگه‌داشتن و Auto-Repeat برای کلیدهای START و STOP
    if (key == PK_START || key == PK_STOP)
    {
        s_last_key_activity_ms = now;

        if (prev != key)
        {
            // لبه اول فشردن
            s_holding_key = key;
            s_hold_start_ms = now;
            s_last_repeat_tick_ms = now;

            if (key == PK_START) {
                if (s_nav_state == NAV_STATE_IDLE) {
                    if (objects.screen_password && lv_obj_is_valid(objects.screen_password) && lv_screen_active() == objects.screen_password) {
                        action_admin_password_redy(NULL);
                        return;
                    }
                    nav_do_mode(root);
                } else {
                    nav_do_start();
                }
            } else if (key == PK_STOP) {
                if (s_nav_state == NAV_STATE_IDLE) {
                    if (objects.screen_password && lv_obj_is_valid(objects.screen_password) && lv_screen_active() == objects.screen_password) {
                        if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
                            app_sound_play(SOUND_TAP);
                            lv_textarea_delete_char(objects.text_area_framware_password);
                            return;
                        }
                    }
                }
                nav_do_stop();
            }
            return;
        }
        else
        {
            // وضعیت نگه‌داشتن پیوسته کلید
            if (s_nav_state != NAV_STATE_IDLE && (now - s_hold_start_ms >= 1000U))
            {
                if (now - s_last_repeat_tick_ms >= 150U)
                {
                    s_last_repeat_tick_ms = now;
                    if (s_cur_focused_obj && lv_obj_is_valid(s_cur_focused_obj))
                    {
                        if (lv_obj_check_type(s_cur_focused_obj, &lv_slider_class) || lv_obj_check_type(s_cur_focused_obj, &lv_arc_class))
                        {
                            if (key == PK_START) nav_do_start();
                            else if (key == PK_STOP) nav_do_stop();
                        }
                    }
                }
            }
            return;
        }
    }
    else
    {
        s_holding_key = PK_NONE;
        s_hold_start_ms = 0;
    }

    // سایر کلیدها روی لبه فشردن
    if (key == PK_NONE || prev != PK_NONE) return;

    s_last_key_activity_ms = now;

    switch (key)
    {
        case PK_SAVE:
            nav_do_save();
            break;
        case PK_CANCEL:
            if (s_nav_state == NAV_STATE_IDLE && objects.screen_password && lv_obj_is_valid(objects.screen_password) && lv_screen_active() == objects.screen_password) {
                if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
                    app_sound_play(SOUND_TAP);
                    lv_textarea_set_text(objects.text_area_framware_password, "");
                    return;
                }
            }
            nav_do_cancel();
            break;
    }
}

// LVGL Render Task
void lvgl_task(void *param)
{
    uint32_t last_hb = 0;
    while (true)
    {
        // کلِ کارِ LVGLِ این فریم زیرِ قفل: app_task هم برای set_var_*/get_var_* همین قفل را می‌گیرد
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);

        // بازیابی آنی صفحه پس از کرش یا ریست نرم‌افزاری (بدون نمایش اسپلش یا تاخیر برای کاربر)
        static bool s_restore_executed = false;
        if (g_restore_needed && !s_restore_executed)
        {
            s_restore_executed = true;
            g_restore_needed = false;

            if (g_restore_is_hwtest)
            {
                hwtest_goto_screen_id(g_restore_screen_id);
            }
            else
            {
                g_firmware_suite = (FirmwareSuite)g_restore_suite;
                if (g_restore_screen_id == SCREEN_ID_SCREEN_FRAMWARE)
                {
                    set_var_is_admin_password_verifyed(true);
                    eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
                }
                else if (g_restore_screen_id == SCREEN_ID_SCREEN_WIFI)
                {
                    eez_flow_set_screen(SCREEN_ID_SCREEN_WIFI, LV_SCR_LOAD_ANIM_NONE, 0, 0);
                }
                else if (g_restore_screen_id == SCREEN_ID_SCREEN_PASSWORD)
                {
                    eez_flow_set_screen(SCREEN_ID_SCREEN_PASSWORD, LV_SCR_LOAD_ANIM_NONE, 0, 0);
                }
            }
        }

        uint32_t time_till_next = lv_timer_handler();
        app_sound_tick();
        if (g_hwtest_active)
        {
            t_ui_tick();
            hwtest_apply_outputs();   // خروجی‌های Q0..Q9 هر فریم اعمال شوند → پاسخِ فوریِ رله
            // hwtest_loop از فریمِ LVGL جدا شد: هر ~۳۰ms اجرا می‌شود تا lv_timer_handler
            // (رندر + خواندنِ تاچ) هر فریم آزاد باشد و منوی تست هنگ نکند.
            static uint32_t lastHw = 0;
            if (millis() - lastHw >= 30) { lastHw = millis(); hwtest_loop(); }
        }
        else
        {
            static uint32_t s_last_launcher_ui_tick = 0;
            uint32_t now_tick = millis();
            if (now_tick - s_last_launcher_ui_tick >= 20)
            {
                s_last_launcher_ui_tick = now_tick;
                ui_tick();                                          // داخل Config
                if (g_card_ver_ready) { config_apply_card_versions_with_incompat(); g_card_ver_ready = false; }
            }

            // حلقه‌ی (arc) دور آیکون دانلود را با درصد به‌روز کن
            if (g_dl_pct_dirty)
            {
                if (objects.obj29 && lv_obj_is_valid(objects.obj29)) lv_arc_set_value(objects.obj29, g_dl_pct);
                g_dl_pct_dirty = false;
            }

            // سازگاریِ سخت‌افزار + نسخه‌ها یک‌بار وقتی صفحه‌ی بروزرسانی ساخته شد (مستقل از وای‌فای)
            {
                static bool hwApplied = false;
                bool up = (objects.card_btn[0] && lv_obj_is_valid(objects.card_btn[0]));
                if (up && !hwApplied)
                {
                    config_apply_hw_compat();
                    // به‌روزرسانی آنی وضعیت و دکمه‌های کارت‌ها روی صفحه‌ی ساخته‌شده
                    config_apply_card_versions_with_incompat();
                    hwApplied = true;
                }
                if (!up) hwApplied = false;
            }

            // پایش تایم‌اوت ۱۵ ثانیه استعلام سرور برای کارت‌های برنامه (تغییر وضعیت به عدم دسترسی)
            {
                static bool s_timeout_applied = false;
                bool up = (objects.card_btn[0] && lv_obj_is_valid(objects.card_btn[0]));
                if (up && !g_card_ver_fetched && g_card_query_start_ms > 0 && (millis() - g_card_query_start_ms >= 15000))
                {
                    if (!s_timeout_applied)
                    {
                        config_apply_card_versions_with_incompat();
                        s_timeout_applied = true;
                    }
                }
                if (g_card_ver_fetched || !up)
                {
                    s_timeout_applied = false;
                }
            }
        }

        panel_nav_tick();   // ناوبریِ کلیدهای پنل (F6–F10 روی GPIO2) در تمام صفحات

        // ==== اطلاع‌رسانی موفقیت تنظیم ساعت و تاریخ از اینترنت ====
        if (g_ntp_sync_success)
        {
            g_ntp_sync_success = false;
            char toast_buf[80];
            if (g_ntp_synced_time_str[0]) {
                snprintf(toast_buf, sizeof(toast_buf), "ساعت و تاریخ با موفقیت تنظیم شد (%s)", g_ntp_synced_time_str);
            } else {
                snprintf(toast_buf, sizeof(toast_buf), "ساعت و تاریخ با موفقیت تنظیم شد");
            }
            panel_toast_show(toast_buf, lv_color_hex(0x22C55E), 4000); // سبز زمردی اپل، ۴ ثانیه
            app_sound_play(SOUND_SUCCESS);

            if (objects.obj16 && lv_obj_is_valid(objects.obj16)) {
                char okmsg[96];
                snprintf(okmsg, sizeof(okmsg), "متصل شد: %s (ساعت تنظیم شد)", g_wifi_ok_ssid[0] ? g_wifi_ok_ssid : "");
                lv_label_set_text(objects.obj16, okmsg);
                lv_obj_set_style_text_color(objects.obj16, lv_color_hex(0xff4caf50), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }

        // پایش و ثبت زنده صفحه فعال در حافظه RTC (بدون استهلاک فلش) و NVS (با محافظت ۳ ثانیه‌ای)
        {
            static uint8_t s_last_tracked_hwt = 0xFF;
            static uint8_t s_last_tracked_scr = 0xFF;
            static uint8_t s_last_tracked_ste = 0xFF;
            static uint32_t s_last_nvs_commit_ms = 0;

            uint8_t cur_hwt = g_hwtest_active ? 1 : 0;
            uint8_t cur_scr = 0;
            if (cur_hwt)
            {
                cur_scr = hwtest_get_active_screen_id();
            }
            else
            {
                lv_obj_t *act = lv_screen_active();
                if (act == objects.screen_framware) cur_scr = SCREEN_ID_SCREEN_FRAMWARE;
                else if (act == objects.screen_wifi) cur_scr = SCREEN_ID_SCREEN_WIFI;
                else if (act == objects.screen_password) cur_scr = SCREEN_ID_SCREEN_PASSWORD;
                else if (act == objects.screen_splash) cur_scr = SCREEN_ID_SCREEN_SPLASH;
                else cur_scr = (uint8_t)eez_flow_get_current_screen();
            }
            uint8_t cur_ste = (uint8_t)g_firmware_suite;

            // فقط در صورتی که در صفحه اسپلش (1) نباشیم و شناسه معتبر باشد
            if ((cur_hwt != 0 || cur_scr > 1) && cur_scr != 0)
            {
                if (cur_hwt != s_last_tracked_hwt || cur_scr != s_last_tracked_scr || cur_ste != s_last_tracked_ste)
                {
                    s_last_tracked_hwt = cur_hwt;
                    s_last_tracked_scr = cur_scr;
                    s_last_tracked_ste = cur_ste;

                    // بروزرسانی آنی حافظه RTC (سرعت RAM، صفر نوشتن در فلش)
                    s_rtc_restore_ctx.magic = APP_RESTORE_MAGIC;
                    s_rtc_restore_ctx.is_hwtest = cur_hwt;
                    s_rtc_restore_ctx.screen_id = cur_scr;
                    s_rtc_restore_ctx.suite = cur_ste;
                    s_rtc_restore_ctx.scroll_x = storage.getScrollX();
                    s_rtc_restore_ctx.checksum = app_restore_calc_checksum(&s_rtc_restore_ctx);

                    // تنظیم زمان‌بند برای ذخیره در NVS پس از ۳ ثانیه ثبات در صفحه
                    s_last_nvs_commit_ms = millis();
                }
                else if (s_last_nvs_commit_ms != 0 && (millis() - s_last_nvs_commit_ms >= 3000))
                {
                    storage.saveLastScreen(cur_hwt, cur_scr, cur_ste);
                    s_last_nvs_commit_ms = 0; // ذخیره شد
                }
            }
        }

        if (HMI_PERF_SERIAL_LOG && millis() - last_hb > 1000)
        {
            last_hb = millis();
            Serial0.printf("[HB] flush=%lu freeHeap=%lu freePSRAM=%lu\n",
                          (unsigned long)g_flush_count,
                          (unsigned long)ESP.getFreeHeap(),
                          (unsigned long)ESP.getFreePsram());
        }

        haf_poll();    // پروتکل امنیتی سریال هافلر (HSSRP-v1) برای ناوبری و تغییر صفحات (تحت قفل LVGL)
        shot_poll();   // پایش وضعیت استریم فریم‌بافر (تحت قفل LVGL جهت ایمنی کامل lv_obj_invalidate)
        ota_guard_tick(); // پایش سلامت فریمور جدید و لغو خودکار رول‌بک پس از ۳ ثانیه اجرای پایدار
        xSemaphoreGive(g_lvgl_mutex);

        // اجرای بلادرنگ ۶۰fps هنگام تعامل کاربر (لمس/درگ)، اسکرول روان کارت‌ها، یا انیمیشن‌های فعال
        bool is_active_touch = (g_touch_indev && lv_indev_get_state(g_touch_indev) == LV_INDEV_STATE_PRESSED);
        bool has_anim = (lv_anim_count_running() > 0);
        bool is_scrolling = (objects.cards_row && lv_obj_is_valid(objects.cards_row) && lv_obj_is_scrolling(objects.cards_row));
        if (is_active_touch || has_anim || is_scrolling || time_till_next == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(1)); // تاخیر استاندارد ۱ms برای تنفس سیستم، همگام‌سازی بافرها و روانی ۶۰fps
        }
        else
        {
            if (time_till_next > 10) time_till_next = 10;
            vTaskDelay(pdMS_TO_TICKS(time_till_next));
        }
    }
}


// (buzzer_task حذف شد — هیچ vTaskNotifyGive/xTaskNotify ای در کل کد نبود و تسک هرگز بیدار نمی‌شد)

// Main App
void app_task(void *param)
{
    static int scanFailRetries = 0;   // شمارنده‌ی تلاشِ مجددِ اسکنِ ناموفق — بینِ دورهای حلقه پایدار
    while (true)
    {
        // پردازش درخواست‌های وب‌پورتال و وب‌اپلیکیشن در هر فریم تسک App
        g_web_portal.tick();

        // در حین آزمون سخت‌افزار، تسک لانچر نباید با اسکن وای‌فای تداخل کند
        if (g_hwtest_active) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // در صورت حضور در منوی بروزرسانی و قطع بودن وای‌فای، اسکن دوره‌ای هر ۸ ثانیه برای اتصال به وای‌فای ذخیره‌شده
        static uint32_t s_last_menu_wifi_retry = 0;
        if (get_var_is_admin_password_verifyed() && WiFi.status() != WL_CONNECTED && !connectingToWifi && !isSearching && !ota_download && !ota_custom_download)
        {
            if (s_last_menu_wifi_retry == 0 || millis() - s_last_menu_wifi_retry >= 8000)
            {
                s_last_menu_wifi_retry = millis();
                xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                config_start_wifi_scan();
                xSemaphoreGive(g_lvgl_mutex);
            }
        }
        else if (WiFi.status() == WL_CONNECTED)
        {
            s_last_menu_wifi_retry = 0;
        }

        int wifi_search_status = WiFi.scanComplete();

        if (isSearching && wifi_search_status == WIFI_SCAN_FAILED)
        {
            // config_start_wifi_scan از set_var استفاده می‌کند → اینجا (نخِ App) زیرِ قفل صدا زده شود
            xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
            config_start_wifi_scan();
            xSemaphoreGive(g_lvgl_mutex);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (wifi_search_status >= 0)
        {
            const int MAXW = 20;
            String ssids[MAXW];
            int rssis[MAXW];
            int cnt = 0;

            // جمع‌آوری نتایج + حذف SSIDهای تکراری (قوی‌ترین آنتن می‌ماند)
            for (int i = 0; i < wifi_search_status && cnt < MAXW; ++i)
            {
                String s = WiFi.SSID(i);
                if (s.length() == 0) continue;
                int r = WiFi.RSSI(i);

                int found = -1;
                for (int j = 0; j < cnt; j++)
                    if (ssids[j] == s) { found = j; break; }

                if (found >= 0)
                {
                    if (r > rssis[found]) rssis[found] = r;
                }
                else
                {
                    ssids[cnt] = s;
                    rssis[cnt] = r;
                    cnt++;
                }
                vTaskDelay(pdMS_TO_TICKS(1));
            }

            // مرتب‌سازی: شبکه‌های دارای رمز ذخیره‌شده اول، سپس بر اساس قدرت آنتن (نزولی)
            for (int a = 0; a < cnt - 1; a++)
            {
                for (int b = 0; b < cnt - 1 - a; b++)
                {
                    bool kb  = storage.hasWifi(ssids[b]);
                    bool kb1 = storage.hasWifi(ssids[b + 1]);
                    bool swap;
                    if (kb != kb1)
                        swap = (!kb && kb1);              // شناخته‌شده جلوتر
                    else
                        swap = (rssis[b] < rssis[b + 1]); // آنتن بیشتر جلوتر
                    if (swap)
                    {
                        String ts = ssids[b]; ssids[b] = ssids[b + 1]; ssids[b + 1] = ts;
                        int tr = rssis[b]; rssis[b] = rssis[b + 1]; rssis[b + 1] = tr;
                    }
                }
            }

            // ساخت لیست "#"-جدا هم‌ترتیب با نگاشتِ ایندکس دراپ‌داون.
            // سقفِ ۱۲ شبکه: بافرِ متغیرِ available_wifi_networks فقط ۵۱۲ بایت است؛ ۲۰ SSIDِ
            // بلند/یونیکد جا نمی‌شود و برشِ خامِ strncpy وسطِ نویسه‌های UTF-8 می‌افتاد
            // (گلیف مربع + ناهم‌نگامیِ احتمالیِ ایندکسِ دراپ‌داون با wifis[]).
            const int LIST_MAX = 12;
            wifi_names = "";
            int knownCount = 0;
            int knownIdx = -1;
            for (int i = 0; i < cnt; i++)
            {
                if (storage.hasWifi(ssids[i]))
                {
                    knownCount++;
                    if (knownIdx < 0) knownIdx = i;
                }
                if (i < LIST_MAX)
                    wifi_names += ssids[i] + "#";
            }
            if (cnt == 0)
            {
                wifi_names = "No WiFi#";
            }

            xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
            set_var_available_wifi_networks(wifi_names.c_str());
            set_var_selected_wifi_index(0);
            xSemaphoreGive(g_lvgl_mutex);

            isSearching = false;
            WiFi.scanDelete();

            if (cnt == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
                xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);   // set_var داخل تابع → زیرِ قفل
                config_start_wifi_scan();
                xSemaphoreGive(g_lvgl_mutex);
                continue;
            }

            // اتصال خودکار به قوی‌ترین شبکه‌ی شناخته‌شده‌ی موجود (رمز ذخیره‌شده)
            if (!get_var_is_wifi_connected() && !connectingToWifi && knownCount >= 1 && knownIdx >= 0
                && storage.getWifiPassword(ssids[knownIdx]).length() > 0)
            {
                selectedWifi = ssids[knownIdx];
                wifiPassword = storage.getWifiPassword(selectedWifi);
                g_knownAutoAttempt = true;
                xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                set_var_selected_wifi_network(selectedWifi.c_str());
                set_var_wifi_password(wifiPassword.c_str());
                set_var_connection_error_label("");
                set_var_waitig_panle_state(true);
                xSemaphoreGive(g_lvgl_mutex);
                connectingToWifi = true;
            }
        }

        /////////////////////////////////////////
        static bool s_wifi_conn_in_progress = false;
        static uint32_t s_wifi_conn_start_ms = 0;

        if (connectingToWifi)
        {
            if (!s_wifi_conn_in_progress)
            {
                if (selectedWifi.length() == 0 || selectedWifi == "No WiFi" || selectedWifi == "Scanning...")
                {
                    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                    set_var_waitig_panle_state(false);
                    set_var_connection_error_label(tr(STR_WIFI_FAIL));
                    xSemaphoreGive(g_lvgl_mutex);
                    g_knownAutoAttempt = false;
                    connectingToWifi = false;
                }
                else
                {
                    s_wifi_conn_in_progress = true;
                    s_wifi_conn_start_ms = millis();
                    g_wifi_wrong_password = false;

                    WiFi.disconnect(false, false);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    WiFi.begin(selectedWifi.c_str(), wifiPassword.c_str());
                    #if HMI_PERF_SERIAL_LOG
                    Serial.printf("[WIFI] Connecting to '%s'...\n", selectedWifi.c_str());
                    #endif
                }
            }
            else
            {
                // بررسی غیربلاک‌کننده وضعیت در هر چرخه ۵۰ میلی‌ثانیه‌ای app_task
                if (WiFi.status() == WL_CONNECTED)
                {
                    #if HMI_PERF_SERIAL_LOG
                    Serial.println("[WIFI] Connected and ready!");
                    #endif
                    storage.saveWifi(selectedWifi, wifiPassword);

                    strncpy(g_wifi_ok_ssid, selectedWifi.c_str(), sizeof(g_wifi_ok_ssid) - 1);
                    g_wifi_ok_ssid[sizeof(g_wifi_ok_ssid) - 1] = 0;
                    g_wifi_ok_msg = true;

                    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                    set_var_connection_error_label("");
                    set_var_waitig_panle_state(false);
                    set_var_is_wifi_connected(true);
                    xSemaphoreGive(g_lvgl_mutex);

                    s_wifi_conn_in_progress = false;
                    g_knownAutoAttempt = false;
                    if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
                    {
                        connectingToWifi = false;
                        xSemaphoreGive(xMutex_Wifi);
                    }
                    app_trigger_ntp_sync();
                }
                else if (g_wifi_wrong_password)
                {
                    #if HMI_PERF_SERIAL_LOG
                    Serial.println("[WIFI] Failed: Wrong password detected by event!");
                    #endif
                    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                    set_var_waitig_panle_state(false);
                    set_var_connection_error_label(tr(STR_WIFI_WRONGSAVED));
                    xSemaphoreGive(g_lvgl_mutex);

                    s_wifi_conn_in_progress = false;
                    g_knownAutoAttempt = false;
                    if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
                    {
                        connectingToWifi = false;
                        xSemaphoreGive(xMutex_Wifi);
                    }
                }
                else if (millis() - s_wifi_conn_start_ms > 12000)
                {
                    #if HMI_PERF_SERIAL_LOG
                    Serial.println("[WIFI] Failed: Connection timed out (12s)!");
                    #endif
                    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                    set_var_waitig_panle_state(false);
                    if (g_knownAutoAttempt)
                    {
                        set_var_connection_error_label(tr(STR_WIFI_WRONGSAVED));
                    }
                    else
                    {
                        set_var_connection_error_label(tr(STR_WIFI_FAIL));
                    }
                    xSemaphoreGive(g_lvgl_mutex);

                    s_wifi_conn_in_progress = false;
                    g_knownAutoAttempt = false;
                    if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
                    {
                        connectingToWifi = false;
                        xSemaphoreGive(xMutex_Wifi);
                    }
                }
            }
        }
        else
        {
            s_wifi_conn_in_progress = false;
        }

        // پس از اتصال وای‌فای، نسخه‌ی برنامه‌ها را از سرور بخوان
        if ((WiFi.status() == WL_CONNECTED || get_var_is_wifi_connected()) && !g_card_ver_fetched && !ota_download && !ota_custom_download)
        {
            if (WiFi.status() == WL_CONNECTED && !get_var_is_wifi_connected()) {
                xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                set_var_is_wifi_connected(true);
                xSemaphoreGive(g_lvgl_mutex);
                vTaskDelay(pdMS_TO_TICKS(400)); // فرصت استقرار LwIP DHCP و DNS
            }
            if (s_last_card_query_time == 0 || millis() - s_last_card_query_time >= 3000)
            {
                s_last_card_query_time = millis();
                int card_count = (g_firmware_suite == SUITE_OTHER) ? 4 : 9;
                bool all_checked = true;
                for (int i = 0; i < card_count; i++)
                {
                    if (ota_download || ota_custom_download) break;
                    if (g_card_checked[i] || g_card_exists[i]) continue; // کارت‌های تکمیل‌شده مجدداً استعلام نشوند

                    bool ex = false;
                    bool contacted = false;
                    config_fetch_lastfile(i, &ex, &contacted);
                    g_card_exists[i] = ex;
                    if (ex || contacted) {
                        g_card_checked[i] = true;
                    } else {
                        all_checked = false; // خطای موقت شبکه، در دوره بعدی مجدداً چک شود
                    }
                    g_card_ver_ready = true;   // به‌روزرسانیِ تدریجی لیبل‌ها
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                if (!ota_download && !ota_custom_download && all_checked) {
                    g_card_ver_fetched = true;
                    g_card_ver_ready = true;
                }
            }
        }

        // ==== همگام‌سازی ساعت از اینترنت (NTP → RTC سخت‌افزاری) — یک‌بار در هر اتصال ====
        {
            static bool sntpWasConnected = false;
            if (WiFi.status() == WL_CONNECTED)
            {
                if (!sntpWasConnected)
                {
                    sntpWasConnected = true;
                    app_trigger_ntp_sync();
                }
            }
            else
            {
                if (sntpWasConnected)
                {
                    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
                    set_var_is_wifi_connected(false);
                    xSemaphoreGive(g_lvgl_mutex);
                }
                sntpWasConnected = false;                  // قطع شد → اتصالِ بعدی دوباره سنکرون کند
                g_card_ver_fetched = false;                // قطع شد → لیست نسخه‌ها دوباره خوانده شود (stale نماند)
            }
        }

        // دانلود از URL دلخواه («ورود لینک») — بدون منطقِ lastfile/نسخه، مستقیم روی URL
        String targetCustomUrl = "";
        bool doCustomDl = false;
        if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
        {
            if (ota_custom_download)
            {
                doCustomDl = true;
                targetCustomUrl = ota_custom_url;
                ota_custom_download = false;
            }
            xSemaphoreGive(xMutex_Wifi);
        }
        if (doCustomDl)
        {
            download_firmware_from_url(targetCustomUrl);
        }

        // بررسی لینک سفارشی روی سرور در پاسخ به کلید تایید کیبورد
        bool doLinkCheck = false;
        String targetCheckUrl = "";
        if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
        {
            if (g_link_check_req)
            {
                doLinkCheck = true;
                targetCheckUrl = g_link_check_url;
                g_link_check_req = false;
            }
            xSemaphoreGive(xMutex_Wifi);
        }
        if (doLinkCheck)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                g_link_check_result = tr(STR_WIFI_DISCONNECTED_ERR);
                g_link_check_status = LINK_CHECK_ERR_WIFI;
                g_link_check_done = true;
                strncpy(s_link_check_result_buf, g_link_check_result.c_str(), sizeof(s_link_check_result_buf) - 1);
                s_link_check_result_buf[sizeof(s_link_check_result_buf) - 1] = '\0';
            }
            else
            {
                String urlNoBin = targetCheckUrl;
                String urlBin = "";
                if (targetCheckUrl.endsWith("/"))
                {
                    urlNoBin = targetCheckUrl + "lastfile";
                    urlBin = targetCheckUrl + "firmware.bin";
                }
                else
                {
                    if (urlNoBin.endsWith(".bin") || urlNoBin.endsWith(".BIN"))
                    {
                        urlNoBin = urlNoBin.substring(0, urlNoBin.length() - 4);
                    }
                    urlBin = urlNoBin + ".bin";
                }

                // ۱. بررسی فایل بدون .bin (اطلاعات برنامه / lastfile)
                WiFiClient client;
                HTTPClient http;
                http.begin(client, urlNoBin);
                http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                http.setConnectTimeout(1500);
                http.setTimeout(2500);
                http.addHeader("User-Agent", "Haffler-OTA/1.5");
                http.addHeader("Accept", "*/*");
                http.addHeader("Connection", "close");
                int code = http.GET();
                if (code != 200 && (urlNoBin.indexOf("%2F") >= 0 || urlNoBin.indexOf("%2f") >= 0))
                {
                    String sUrl = urlNoBin;
                    sUrl.replace("%2F", "/");
                    sUrl.replace("%2f", "/");
                    http.end();
                    http.begin(client, sUrl);
                    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                    http.setConnectTimeout(1500);
                    http.setTimeout(2500);
                    http.addHeader("User-Agent", "Haffler-OTA/1.5");
                    http.addHeader("Accept", "*/*");
                    http.addHeader("Connection", "close");
                    code = http.GET();
                }

                if (code == 200)
                {
                    String body = http.getString();
                    http.end();
                    LastfileInfo info;
                    config_parse_lastfile(body, info);
                    String infoText = "";
                    if (info.ver.length() > 0)
                    {
                        infoText = String(tr(STR_VERSION_INFO)) + " " + info.ver;
                        if (info.bdate.length() > 0) infoText += " (" + info.bdate + ")";
                        String desc = "";
                        if (g_lang == LANG_FA && info.desc_fa[0].length() > 0) desc = info.desc_fa[0];
                        else if (g_lang == LANG_TR && info.desc_tr[0].length() > 0) desc = info.desc_tr[0];
                        else if (g_lang == LANG_RU && info.desc_ru[0].length() > 0) desc = info.desc_ru[0];
                        else if (info.desc[0].length() > 0) desc = info.desc[0];
                        if (desc.length() > 0)
                        {
                            cfg_strip_bullet(desc);
                            cfg_sanitize_display_text(desc);
                            infoText += "\n- " + desc;
                        }
                        else
                        {
                            infoText += String("\n") + tr(STR_LINK_VALID_INFO);
                        }
                    }
                    else
                    {
                        body.trim();
                        cfg_sanitize_display_text(body);
                        if (body.length() > 0 && body.length() < 100)
                            infoText = String(tr(STR_VERSION_INFO)) + ": " + body + "\n" + tr(STR_LINK_VALID_INFO);
                        else
                            infoText = String(tr(STR_LINK_VALID_INFO));
                    }
                    cfg_sanitize_display_text(infoText);
                    g_link_check_result = infoText;
                    g_link_check_status = LINK_CHECK_OK_INFO;
                    strncpy(s_link_check_result_buf, g_link_check_result.c_str(), sizeof(s_link_check_result_buf) - 1);
                    s_link_check_result_buf[sizeof(s_link_check_result_buf) - 1] = '\0';
                    g_link_check_done = true;
                }
                else
                {
                    // ۲. اگر فایل بدون .bin نبود، بررسی وجود فایل فریمور .bin
                    http.end();
                    http.begin(client, urlBin);
                    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                    http.setConnectTimeout(1500);
                    http.setTimeout(2500);
                    http.addHeader("User-Agent", "Haffler-OTA/1.5");
                    http.addHeader("Accept", "*/*");
                    http.addHeader("Range", "bytes=0-128");
                    http.addHeader("Connection", "close");
                    int binCode = http.GET();
                    if (binCode != 200 && binCode != 206 && (urlBin.indexOf("%2F") >= 0 || urlBin.indexOf("%2f") >= 0))
                    {
                        String sUrl = urlBin;
                        sUrl.replace("%2F", "/");
                        sUrl.replace("%2f", "/");
                        http.end();
                        http.begin(client, sUrl);
                        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                        http.setConnectTimeout(1500);
                        http.setTimeout(2500);
                        http.addHeader("User-Agent", "Haffler-OTA/1.5");
                        http.addHeader("Accept", "*/*");
                        http.addHeader("Range", "bytes=0-128");
                        http.addHeader("Connection", "close");
                        binCode = http.GET();
                    }
                    http.end();

                    if (binCode == 200 || binCode == 206)
                    {
                        g_link_check_result = tr(STR_LINK_VALID_BIN);
                        g_link_check_status = LINK_CHECK_OK_BIN;
                    }
                    else
                    {
                        g_link_check_result = tr(STR_LINK_NOT_FOUND);
                        g_link_check_status = LINK_CHECK_ERR_NOTFOUND;
                    }
                    strncpy(s_link_check_result_buf, g_link_check_result.c_str(), sizeof(s_link_check_result_buf) - 1);
                    s_link_check_result_buf[sizeof(s_link_check_result_buf) - 1] = '\0';
                    g_link_check_done = true;
                }
            }
        }

        String targetModel = "";
        bool doDl = false;
        if (xSemaphoreTake(xMutex_Wifi, portMAX_DELAY))
        {
            if (ota_download)
            {
                doDl = true;
                targetModel = ota_download_model;
                ota_download = false;
            }
            xSemaphoreGive(xMutex_Wifi);
        }
        if (doDl)
        {
            download_firmware(targetModel);
        }

        // web_portal.tick درون خود delay(1) دارد؛ throttle می‌کنیم تا هر 5ms یک‌بار صدا زده شود
        {
            static uint32_t s_portal_last = 0;
            uint32_t now = millis();
            if (now - s_portal_last >= 5)
            {
                s_portal_last = now;
                g_web_portal.tick();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void download_firmware(String model)
{
    // به‌روزرسانی خودِ برنامه‌ی Config: فقط اگر نسخه‌ی سرور جدیدتر باشد.
    bool selfUpdate = model.endsWith("config-test");
    g_ota_cancel_requested = false;

    // متن فعلی پنل («درحال دانلود X») را به عنوان پایه نگه می‌داریم تا درصد به آن بچسبد
    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
    if (objects.download_cancel_btn && lv_obj_is_valid(objects.download_cancel_btn))
    {
        lv_obj_clear_state(objects.download_cancel_btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(objects.download_cancel_btn, lv_color_hex(0xffd32f2f), 0);
        if (objects.download_cancel_label && lv_obj_is_valid(objects.download_cancel_label))
        {
            lv_label_set_text(objects.download_cancel_label, tr(STR_CANCEL_DL));
        }
    }
    if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
    {
        lv_label_set_text(objects.download_metrics_label, "%0   |   0.00 MB   |   0 KB/s");
    }
    g_dl_base = String(get_var_download_lable_info_text());
    xSemaphoreGive(g_lvgl_mutex);

    // --- مرحله ۱: خواندن نسخه با تلاش هوشمند روی کاندیداهای نام پوشه/مدل ---
    String activeModel = model;
    String latestVersion = "";
    String expectedSha256 = "";
    String expectedBinFile = "";
    int cardIdx = -1;
    bool isOtherSuite = (g_firmware_suite == SUITE_OTHER);
    if (isOtherSuite) {
        for (int i = 0; i < 4; i++) {
            for (int c = 0; g_other_card_model_candidates[i][c] != NULL; c++) {
                if (model == g_other_card_model_candidates[i][c]) { cardIdx = i; break; }
            }
            if (cardIdx >= 0) break;
        }
    }
    if (cardIdx < 0) {
        for (int i = 0; i < 9; i++) {
            for (int c = 0; g_card_model_candidates[i][c] != NULL; c++) {
                if (model == g_card_model_candidates[i][c]) { cardIdx = i; break; }
            }
            if (cardIdx >= 0) break;
        }
    }

    const char * const *candList = (isOtherSuite && cardIdx >= 0 && cardIdx < 4) ?
        g_other_card_model_candidates[cardIdx] : ((cardIdx >= 0) ? g_card_model_candidates[cardIdx] : NULL);

    bool foundLastfile = false;
    int candIdx = 0;
    while (!foundLastfile)
    {
        String curCandidate = (candList != NULL && candList[candIdx] != NULL)
                              ? String(candList[candIdx]) : activeModel;
        String url = ota_url(curCandidate, "lastfile");
        WiFiClient client;
        HTTPClient http;
        http.begin(client, url);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setConnectTimeout(4000);
        http.setTimeout(6000);
        http.addHeader("User-Agent", "Haffler-OTA/1.5");
        http.addHeader("Accept", "*/*");
        http.addHeader("Connection", "close");
        int httpCode = http.GET();
        if (httpCode != 200 && (url.indexOf("%2F") >= 0 || url.indexOf("%2f") >= 0))
        {
            String slashUrl = url;
            slashUrl.replace("%2F", "/");
            slashUrl.replace("%2f", "/");
            http.end();
            http.begin(client, slashUrl);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http.setConnectTimeout(4000);
            http.setTimeout(6000);
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            httpCode = http.GET();
        }

        if (g_ota_cancel_requested)
        {
            http.end();
            xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
            set_var_download_lable_info_text(tr(STR_DL_CANCELLED));
            if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
                lv_label_set_text(objects.download_metrics_label, "");
            xSemaphoreGive(g_lvgl_mutex);
            vTaskDelay(pdMS_TO_TICKS(1200));   // B-02: fix
            xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
            set_var_download_panle_state(false);
            xSemaphoreGive(g_lvgl_mutex);
            g_ota_cancel_requested = false;
            return;
        }

        if (httpCode == 200)
        {
            LastfileInfo lf;
            config_parse_lastfile(http.getString(), lf);
            latestVersion = lf.ver;
            expectedSha256 = lf.sha256;
            expectedBinFile = lf.bin_file;
            activeModel = curCandidate;
            if (cardIdx >= 0) g_card_active_model[cardIdx] = activeModel;
            foundLastfile = true;
            http.end();
            break;
        }
        http.end();

        if (candList != NULL && candList[candIdx + 1] != NULL) {
            candIdx++;
            vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            break;
        }
    }

    if (!foundLastfile)
    {
        Serial.printf("Program not on server for model: %s\n", model.c_str());
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    model = activeModel;

    if (g_ota_cancel_requested)
    {
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_CANCELLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(1200));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        g_ota_cancel_requested = false;
        return;
    }

    Serial.println(model + " : Latest version on server: " + latestVersion);

    Serial.println("Starting OTA update (unconditional)...");

    String binUrl = "";
    if (expectedBinFile.length() > 0) {
        binUrl = ota_url(model, expectedBinFile.c_str());
    } else {
        binUrl = ota_url(model, "firmware.bin");
    }

    // نمایش عنوان دستگاه روی پنل بدون درصد تکراری + ریستِ حلقه‌ی دور آیکون
    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
    set_var_download_lable_info_text(g_dl_base.c_str());
    xSemaphoreGive(g_lvgl_mutex);
    g_dl_pct = 0; g_dl_pct_dirty = true;

    Serial.printf("Before firmware GET: freeHeap=%lu largestBlock=%lu\n",
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

    WiFiClient binClient;
    binClient.setTimeout(15000);
    binClient.setNoDelay(true);   // غیرفعال‌کردن Nagle → throughput بالاتر برای دانلود

    HTTPClient http2;
    http2.begin(binClient, binUrl);
    http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http2.setReuse(false);
    http2.addHeader("User-Agent", "Haffler-OTA/1.5");
    http2.addHeader("Accept", "*/*");
    http2.addHeader("Connection", "close");
    int code = http2.GET();
    if (code < 0)
    {
        Serial.printf("firmware GET retry (first code=%d)\n", code);
        http2.end();
        vTaskDelay(pdMS_TO_TICKS(500));   // B-02: fix
        http2.begin(binClient, binUrl);
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        http2.addHeader("User-Agent", "Haffler-OTA/1.5");
        http2.addHeader("Accept", "*/*");
        http2.addHeader("Connection", "close");
        code = http2.GET();
    }
    // فال‌بک ۱: اگر فایل اختصاصی داده شده بود و 404 شد، firmware.bin چک شود
    if (code == 404 && expectedBinFile.length() > 0 && expectedBinFile != "firmware.bin")
    {
        http2.end();
        binUrl = ota_url(model, "firmware.bin");
        Serial.printf("Specific binary 404 -> trying firmware.bin: %s\n", binUrl.c_str());
        http2.begin(binClient, binUrl);
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        http2.addHeader("User-Agent", "Haffler-OTA/1.5");
        http2.addHeader("Accept", "*/*");
        http2.addHeader("Connection", "close");
        code = http2.GET();
    }
    // فال‌بک ۲: در صورتی که بدون پسوند نام‌گذاری شده باشد
    if (code == 404)
    {
        http2.end();
        binUrl = ota_url(model, "firmware");
        Serial.printf("firmware.bin 404 -> trying firmware: %s\n", binUrl.c_str());
        http2.begin(binClient, binUrl);
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        http2.addHeader("User-Agent", "Haffler-OTA/1.5");
        http2.addHeader("Accept", "*/*");
        http2.addHeader("Connection", "close");
        code = http2.GET();
    }
    // فال‌بک ۳: در صورت خطا، تست با اسلش مستقیم به جای %2F
    if (code != 200 && (binUrl.indexOf("%2F") >= 0 || binUrl.indexOf("%2f") >= 0))
    {
        String slashBinUrl = binUrl;
        slashBinUrl.replace("%2F", "/");
        slashBinUrl.replace("%2f", "/");
        Serial.printf("firmware %d -> trying slash URL: %s\n", code, slashBinUrl.c_str());
        http2.end();
        http2.begin(binClient, slashBinUrl);
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        http2.addHeader("User-Agent", "Haffler-OTA/1.5");
        http2.addHeader("Accept", "*/*");
        http2.addHeader("Connection", "close");
        code = http2.GET();
    }

    if (g_ota_cancel_requested)
    {
        http2.end();
        binClient.stop();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_CANCELLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(1200));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        g_ota_cancel_requested = false;
        return;
    }

    if (code != 200)
    {
        Serial.printf("firmware.bin GET failed: %d\n", code);
        http2.end();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    int total = http2.getSize();   // Content-Length
    ota_progress_reset();
    size_t updateSize = (total > 0) ? (size_t)total : UPDATE_SIZE_UNKNOWN;
    if (!Update.begin(updateSize))
    {
        Serial.printf("Update.begin failed (total=%d, err=%d)\n", total, (int)Update.getError());
        http2.end();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    // تخصیص بافر در SRAM داخلی جهت جلوگیری از Flash Cache Disabled Panic
    size_t BUFSZ = 16384;
    uint8_t *buf = (uint8_t *)heap_caps_malloc(BUFSZ, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf) { BUFSZ = 4096; buf = (uint8_t *)malloc(BUFSZ); }
    if (!buf) {
        // حافظه کافی نیست — OTA لغو می‌شود (NULL dereference قطعی جلوگیری می‌شود)
        Serial.println("[OTA] CRITICAL: malloc failed — aborting OTA");
        http2.end();
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text("خطا: حافظه کافی نیست");
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "Out of memory");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0); // 0 = SHA-256

    WiFiClient *stream = http2.getStreamPtr();
    if (!stream) {
        // اتصال قبل از خواندن stream قطع شد
        Serial.println("[OTA] stream pointer is NULL — aborting");
        heap_caps_free(buf);
        mbedtls_sha256_free(&sha_ctx);
        http2.end();
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_STALLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "Stream error");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }
    size_t written = 0;
    uint32_t lastData = millis();
    bool isStalled = false;
    size_t chunk_since_yield = 0;

    while (http2.connected() && (total <= 0 || written < (size_t)total))
    {
        if (g_ota_cancel_requested)
        {
            Serial.println("[OTA] Download cancelled by user in stream loop!");
            break;
        }

        size_t filled = 0;
        uint32_t batch_start = millis();
        while (filled < BUFSZ && (total <= 0 || (written + filled) < (size_t)total))
        {
            if (g_ota_cancel_requested) break;
            size_t avail = stream->available();
            if (avail > 0)
            {
                size_t toRead = (BUFSZ - filled) < avail ? (BUFSZ - filled) : avail;
                int r = stream->readBytes((char *)(buf + filled), toRead);
                if (r > 0)
                {
                    filled += r;
                    lastData = millis();
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (filled > 0) break; // دیتای خوانده‌شده را بدون معطلی در فلش بنویس
                if (millis() - lastData > 15000)
                {
                    Serial.println("[OTA] Download stalled (>15s)");
                    isStalled = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1));
                if (millis() - batch_start > 30) break;
            }
        }

        if (isStalled || g_ota_cancel_requested) break;

        if (filled > 0)
        {
            mbedtls_sha256_update(&sha_ctx, buf, filled);
            if (Update.write(buf, filled) != filled)
            {
                Serial.println("Update.write mismatch");
                break;
            }
            written += filled;
            ota_progress_cb((int)written, total > 0 ? total : (int)written);
            chunk_since_yield += filled;

            // تسلیم زمان‌بند فقط هر ۳۲KB جهت اجرای بدون لگ سایر تسک‌ها
            if (chunk_since_yield >= 32768)
            {
                chunk_since_yield = 0;
                taskYIELD();
            }
        }
    }

    if (buf) heap_caps_free(buf);
    http2.end();
    binClient.stop();

    uint8_t sha_out[32];
    mbedtls_sha256_finish(&sha_ctx, sha_out);
    mbedtls_sha256_free(&sha_ctx);

    char actualShaHex[65];
    for (int i = 0; i < 32; i++) snprintf(actualShaHex + (i * 2), 3, "%02x", sha_out[i]);
    actualShaHex[64] = '\0';

    expectedSha256.trim();
    bool shaOk = true;
    if (expectedSha256.length() == 64)
    {
        shaOk = (strcasecmp(actualShaHex, expectedSha256.c_str()) == 0);
        if (!shaOk) {
            Serial.printf("[OTA] SHA256 mismatch! Expected: %s, Got: %s\n", expectedSha256.c_str(), actualShaHex);
        } else {
            Serial.printf("[OTA] SHA256 verified OK: %s\n", actualShaHex);
        }
    }


    bool dl_complete = (total > 0) ? (written == (size_t)total) : (written > 0 && !isStalled);
    if (!g_ota_cancel_requested && !isStalled && dl_complete && shaOk && Update.end(true) && Update.isFinished())
    {
        Serial.println("OTA Update Successful. Rebooting...");
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(g_dl_base.c_str());
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "%100   •   نصب فریمور...");
        xSemaphoreGive(g_lvgl_mutex);
        ota_guard_arm_for_next_boot();
        vTaskDelay(pdMS_TO_TICKS(400));   // B-02: fix
        ESP.restart();
    }
    else if (g_ota_cancel_requested)
    {
        Serial.printf("[OTA] Aborted by user. Clearing partial write (%u/%d bytes)...\n", (unsigned)written, total);
        Update.abort(); // پاک‌سازی قطعی و برگرداندن پارتیشن فلش به وضعیت امن قبلی
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_CANCELLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(1200));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        g_ota_cancel_requested = false;
    }
    else if (isStalled)
    {
        Serial.printf("[OTA] Stalled. Aborting OTA (written=%u/%d)...\n", (unsigned)written, total);
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_STALLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "اینترنت را بررسی کنید");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
    }
    else if (!shaOk)
    {
        Serial.println("[OTA] Firmware hash check failed! Aborting flash.");
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text("خطای اعتبارسنجی فایل فریمور");
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "Checksum mismatch");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
    }
    else
    {
        Serial.printf("OTA failed (written=%u/%d, err=%d)\n", (unsigned)written, total, (int)Update.getError());
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
    }
}

// دانلود مستقیم فریمور از یک URL دلخواه (ویژگیِ «ورود لینک»).
// بدون منطقِ lastfile/نسخه: همان بدنه‌ی استریمِ download_firmware، اما مستقیم روی URL.
void download_firmware_from_url(String url)
{
    g_ota_cancel_requested = false;

    // متن فعلی پنل («درحال دانلود <url>») را به عنوان پایه نگه می‌داریم تا درصد به آن بچسبد
    xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
    if (objects.download_cancel_btn && lv_obj_is_valid(objects.download_cancel_btn))
    {
        lv_obj_clear_state(objects.download_cancel_btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(objects.download_cancel_btn, lv_color_hex(0xffd32f2f), 0);
        if (objects.download_cancel_label && lv_obj_is_valid(objects.download_cancel_label))
        {
            lv_label_set_text(objects.download_cancel_label, tr(STR_CANCEL_DL));
        }
    }
    if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
    {
        lv_label_set_text(objects.download_metrics_label, "%0   |   0.00 MB   |   0 KB/s");
    }
    g_dl_base = String(get_var_download_lable_info_text());
    set_var_download_lable_info_text(g_dl_base.c_str());
    xSemaphoreGive(g_lvgl_mutex);
    g_dl_pct = 0; g_dl_pct_dirty = true;

    Serial.println("Custom URL OTA from: " + url);
    Serial.printf("Before custom GET: freeHeap=%lu largestBlock=%lu\n",
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

    url.trim();
    if (url.startsWith("https://ota.haffler-infi.ir")) {
        url.replace("https://", "http://");
    }

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    bool isHttps = url.startsWith("https://");

    if (isHttps) {
        secureClient.setInsecure();
        secureClient.setTimeout(15000);
        secureClient.setNoDelay(true);
    } else {
        plainClient.setTimeout(15000);
        plainClient.setNoDelay(true);
    }

    HTTPClient http2;
    if (isHttps) {
        http2.begin(secureClient, url);
    } else {
        http2.begin(plainClient, url);
    }
    http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http2.setReuse(false);
    http2.addHeader("User-Agent", "Haffler-OTA/1.5");
    http2.addHeader("Accept", "*/*");
    http2.addHeader("Connection", "close");
    // یک بار تلاشِ مجدد در صورت خطای اتصال
    int code = http2.GET();
    if (code < 0)
    {
        Serial.printf("custom URL GET retry (first code=%d)\n", code);
        http2.end();
        vTaskDelay(pdMS_TO_TICKS(500));   // B-02: fix
        if (isHttps) {
            http2.begin(secureClient, url);
        } else {
            http2.begin(plainClient, url);
        }
        http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http2.setReuse(false);
        http2.addHeader("User-Agent", "Haffler-OTA/1.5");
        http2.addHeader("Accept", "*/*");
        http2.addHeader("Connection", "close");
        code = http2.GET();
    }

    if (g_ota_cancel_requested)
    {
        http2.end();
        if (isHttps) secureClient.stop(); else plainClient.stop();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_CANCELLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(1200));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        g_ota_cancel_requested = false;
        return;
    }

    if (code != 200)
    {
        Serial.printf("custom URL GET failed: %d\n", code);
        http2.end();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    int total = http2.getSize();   // Content-Length (در صورت Chunked ممکن است منفی باشد)
    size_t updateSize = (total > 0) ? (size_t)total : UPDATE_SIZE_UNKNOWN;
    if (!Update.begin(updateSize))
    {
        Serial.printf("Update.begin failed (total=%d, err=%d)\n", total, (int)Update.getError());
        http2.end();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    // تخصیص بافر در SRAM داخلی جهت جلوگیری از Flash Cache Disabled Panic
    size_t BUFSZ = 16384;
    uint8_t *buf = (uint8_t *)heap_caps_malloc(BUFSZ, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf) { BUFSZ = 4096; buf = (uint8_t *)malloc(BUFSZ); }
    if (!buf) {
        // حافظه کافی نیست — OTA لغو می‌شود
        Serial.println("[OTA] CRITICAL: malloc failed — aborting custom URL OTA");
        http2.end();
        if (isHttps) secureClient.stop(); else plainClient.stop();
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text("خطا: حافظه کافی نیست");
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "Out of memory");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }

    // SHA-256 برای اعتبارسنجی یکپارچگی فایل — همانند download_firmware
    mbedtls_sha256_context sha_ctx_cu;
    mbedtls_sha256_init(&sha_ctx_cu);
    mbedtls_sha256_starts(&sha_ctx_cu, 0);

    WiFiClient *stream = http2.getStreamPtr();
    if (!stream) {
        Serial.println("[OTA] stream pointer is NULL — aborting custom URL OTA");
        heap_caps_free(buf);
        mbedtls_sha256_free(&sha_ctx_cu);
        http2.end();
        if (isHttps) secureClient.stop(); else plainClient.stop();
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_STALLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "Stream error");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        return;
    }
    ota_progress_reset();
    size_t written = 0;
    uint32_t lastData = millis();
    bool isStalled = false;
    size_t chunk_since_yield_cu = 0;

    while (http2.connected() && (total <= 0 || written < (size_t)total))
    {
        if (g_ota_cancel_requested)
        {
            Serial.println("[OTA] Custom URL download cancelled by user in stream loop!");
            break;
        }

        size_t filled = 0;
        uint32_t batch_start = millis();
        while (filled < BUFSZ && (total <= 0 || (written + filled) < (size_t)total))
        {
            if (g_ota_cancel_requested) break;
            size_t avail = stream->available();
            if (avail > 0)
            {
                size_t toRead = (BUFSZ - filled) < avail ? (BUFSZ - filled) : avail;
                int r = stream->readBytes((char *)(buf + filled), toRead);
                if (r > 0)
                {
                    filled += r;
                    lastData = millis();
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (filled > 0) break; // دیتای خوانده‌شده را بدون معطلی در فلش بنویس
                if (millis() - lastData > 15000)
                {
                    Serial.println("[OTA] Custom URL download stalled (>15s)");
                    isStalled = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1));
                if (millis() - batch_start > 30) break;
            }
        }

        if (isStalled || g_ota_cancel_requested) break;

        if (filled > 0)
        {
            mbedtls_sha256_update(&sha_ctx_cu, buf, filled);
            if (Update.write(buf, filled) != filled)
            {
                Serial.println("Update.write mismatch");
                break;
            }
            written += filled;
            ota_progress_cb((int)written, total > 0 ? total : (int)written);
            chunk_since_yield_cu += filled;

            // تسلیم زمان‌بند فقط هر ۳۲KB جهت اجرای بدون لگ سایر تسک‌ها
            if (chunk_since_yield_cu >= 32768)
            {
                chunk_since_yield_cu = 0;
                taskYIELD();
            }
        }
    }

    if (buf) heap_caps_free(buf);
    http2.end();
    if (isHttps) secureClient.stop(); else plainClient.stop();

    // پایان SHA-256 و چاپ هش واقعی (برای مقایسه دستی اپراتور)
    uint8_t sha_out_cu[32];
    mbedtls_sha256_finish(&sha_ctx_cu, sha_out_cu);
    mbedtls_sha256_free(&sha_ctx_cu);
    char actualShaHex_cu[65];
    for (int i = 0; i < 32; i++) snprintf(actualShaHex_cu + (i * 2), 3, "%02x", sha_out_cu[i]);
    actualShaHex_cu[64] = '\0';
    Serial.printf("[OTA] Custom URL SHA256: %s\n", actualShaHex_cu);

    bool dl_complete = (total > 0) ? (written == (size_t)total) : (written > 0 && !isStalled);
    if (!g_ota_cancel_requested && !isStalled && dl_complete && Update.end(true) && Update.isFinished())
    {
        Serial.println("Custom URL OTA Successful. Rebooting...");
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(g_dl_base.c_str());
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "%100   •   نصب فریمور...");
        xSemaphoreGive(g_lvgl_mutex);
        ota_guard_arm_for_next_boot();
        vTaskDelay(pdMS_TO_TICKS(400));   // B-02: fix
        ESP.restart();
    }
    else if (g_ota_cancel_requested)
    {
        Serial.printf("[OTA] Aborted by user. Clearing partial write (%u/%d bytes)...\n", (unsigned)written, total);
        Update.abort(); // لغو و پاک‌سازی پارتیشن فلش
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_CANCELLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(1200));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
        g_ota_cancel_requested = false;
    }
    else if (isStalled)
    {
        Serial.printf("[OTA] Custom URL Stalled. Aborting (written=%u/%d)...\n", (unsigned)written, total);
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_DL_STALLED));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "اینترنت را بررسی کنید");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
    }
    else
    {
        Serial.printf("Custom URL OTA failed (written=%u/%d, err=%d)\n", (unsigned)written, total, (int)Update.getError());
        Update.abort();
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_lable_info_text(tr(STR_NOT_ON_SERVER));
        if (objects.download_metrics_label && lv_obj_is_valid(objects.download_metrics_label))
            lv_label_set_text(objects.download_metrics_label, "");
        xSemaphoreGive(g_lvgl_mutex);
        vTaskDelay(pdMS_TO_TICKS(3500));   // B-02: fix
        xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
        set_var_download_panle_state(false);
        xSemaphoreGive(g_lvgl_mutex);
    }
}
#endif
