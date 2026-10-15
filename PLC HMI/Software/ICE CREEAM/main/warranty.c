#include "warranty.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_http_client.h"
#include "wifi.h"
#include "ds3231.h"
#include "day_lock_core.h"
#include "buzzer.h"
#include "lvgl_port.h"
#include "ui_lang.h"
#include "ui/screens.h"

static const char *TAG = "WARRANTY";

#define NVS_WARRANTY_NS       "warranty"
#define SMS_RECIPIENT_PRIMARY "09121058755"

/* IPPanel (Yekta Panel) Credentials */
#define IPPANEL_REST_URL      "http://rest.ippanel.com/v1/messages"
#define IPPANEL_API_KEY       "YTJjYTZjYzUtODNhYS00MzdkLWJkYjAtZGQ1NzFlN2ZlZDU4ZmZkMDRhOTk2ZDk2OGQ5NTYwMjZiMWI5ODU1OTJjMmM="
#define IPPANEL_ORIGINATOR    "+983000505"

/* SMS.IR Fallback Credentials */
#define SMSIR_REST_URL        "https://api.sms.ir/v1/send/bulk"
#define SMSIR_API_KEY         "5SHlWl0KXVb0cT3anLv9gHjC6ehbbKJ1VpokaXA4VlafRQTk"
#define SMSIR_LINE_NUMBER     30002108037986ULL

static warranty_info_t s_info;
static uint32_t s_grace_seconds_left = WARRANTY_GRACE_PERIOD_SEC;
static bool s_expired_and_locked = false;
static TaskHandle_t s_sms_task_handle = NULL;

extern void plc_stop_all(void);
extern void open_warranty_expired_dialog(void);
extern void ui_screens_update_warranty_f6(void);

/* Weekday names in Persian (0=Sunday .. 6=Saturday) */
static const char *s_fa_wdays[7] = {
    "یک‌شنبه", "دوشنبه", "سه‌شنبه", "چهارشنبه", "پنج‌شنبه", "جمعه", "شنبه"
};

/* --- Luhn Algorithm Helpers --- */

static int calculate_luhn_check_digit(const char *ten_digits)
{
    if (!ten_digits) return 0;
    int sum = 0;
    /* 10 digits: indices 0..9.
     * Check digit will be at position 1 (rightmost).
     * Index 9 is at pos 2 from right (even) -> doubled.
     * Index 8 is at pos 3 from right (odd)  -> not doubled.
     */
    for (int i = 9; i >= 0; i--) {
        int val = ten_digits[i] - '0';
        if (val < 0 || val > 9) val = 0;
        int pos_from_right = (10 - i) + 1; /* pos 2..11 */
        if (pos_from_right % 2 == 0) {
            val *= 2;
            if (val > 9) val -= 9;
        }
        sum += val;
    }
    return (10 - (sum % 10)) % 10;
}

bool warranty_verify_customer_code(const char *code)
{
    if (!code || strlen(code) != 11) return false;
    int sum = 0;
    for (int i = 10; i >= 0; i--) {
        if (code[i] < '0' || code[i] > '9') return false;
        int val = code[i] - '0';
        int pos_from_right = 11 - i; /* 1..11, check digit is 1 */
        if (pos_from_right % 2 == 0) {
            val *= 2;
            if (val > 9) val -= 9;
        }
        sum += val;
    }
    return (sum % 10 == 0);
}

void warranty_get_customer_code(char *out_code, size_t max_sz)
{
    if (!out_code || max_sz < 12) return;

    if (s_info.customer_code[0] != '\0' && strlen(s_info.customer_code) == 11) {
        if (out_code != s_info.customer_code) {
            strncpy(out_code, s_info.customer_code, max_sz - 1);
            out_code[max_sz - 1] = '\0';
        }
        return;
    }

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    /* SplitMix64 deterministic hash seeded with factory MAC bytes */
    uint64_t seed = ((uint64_t)mac[0] << 40) |
                    ((uint64_t)mac[1] << 32) |
                    ((uint64_t)mac[2] << 24) |
                    ((uint64_t)mac[3] << 16) |
                    ((uint64_t)mac[4] << 8)  |
                    ((uint64_t)mac[5]);

    uint64_t z = (seed + 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);

    /* 10 decimal digits guaranteed without leading zero issues */
    uint64_t d10 = 1000000000ULL + (z % 9000000000ULL);
    char ten_str[12];
    snprintf(ten_str, sizeof(ten_str), "%010llu", (unsigned long long)d10);

    /* 11th digit is mathematical Luhn checksum */
    int cd = calculate_luhn_check_digit(ten_str);
    snprintf(out_code, max_sz, "%s%d", ten_str, cd);
}

/* --- Storage Save / Load --- */

static esp_err_t warranty_save_to_nvs(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_WARRANTY_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open write failed: %s", esp_err_to_name(err));
        return err;
    }
    nvs_set_u8(h, "active", s_info.active);
    nvs_set_u8(h, "expired", s_expired_and_locked ? 1 : 0);
    nvs_set_u32(h, "grace_left", s_grace_seconds_left);
    nvs_set_str(h, "name", s_info.buyer_name);
    nvs_set_str(h, "city", s_info.city);
    nvs_set_str(h, "phone", s_info.phone);
    nvs_set_str(h, "date", s_info.date_str);
    nvs_set_str(h, "code", s_info.customer_code);
    nvs_set_u8(h, "sms_sent", s_info.sms_sent);
    nvs_set_u8(h, "sms_pend", s_info.sms_pending);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Warranty NVS saved (active=%u, expired=%u, grace_left=%lu, code=%s, sms_sent=%u)",
             (unsigned)s_info.active, (unsigned)s_expired_and_locked, (unsigned long)s_grace_seconds_left,
             s_info.customer_code, (unsigned)s_info.sms_sent);
    return ESP_OK;
}

esp_err_t warranty_init(void)
{
    memset(&s_info, 0, sizeof(s_info));
    s_grace_seconds_left = WARRANTY_GRACE_PERIOD_SEC;
    s_expired_and_locked = false;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_WARRANTY_NS, NVS_READONLY, &h);
    if (err == ESP_OK) {
        nvs_get_u8(h, "active", &s_info.active);
        size_t sz = sizeof(s_info.buyer_name);
        nvs_get_str(h, "name", s_info.buyer_name, &sz);
        sz = sizeof(s_info.city);
        nvs_get_str(h, "city", s_info.city, &sz);
        sz = sizeof(s_info.phone);
        nvs_get_str(h, "phone", s_info.phone, &sz);
        sz = sizeof(s_info.date_str);
        nvs_get_str(h, "date", s_info.date_str, &sz);
        sz = sizeof(s_info.customer_code);
        nvs_get_str(h, "code", s_info.customer_code, &sz);
        nvs_get_u8(h, "sms_sent", &s_info.sms_sent);
        nvs_get_u8(h, "sms_pend", &s_info.sms_pending);
        nvs_close(h);
    } else {
        ESP_LOGI(TAG, "No warranty data in NVS, generating initial customer code");
    }

    /* Ensure customer code is always generated */
    if (s_info.customer_code[0] == '\0' || strlen(s_info.customer_code) != 11) {
        warranty_get_customer_code(s_info.customer_code, sizeof(s_info.customer_code));
    }

    /* اگر گارانتی فعال باشد، قفل انقضا بی‌اثر است */
    if (s_info.active) {
        s_expired_and_locked = false;
        s_grace_seconds_left = 0;
    } else {
        /* طبق دستور کارفرما: با هر ریست/بوت دستگاه، تایمر مهلت تست مجدداً ۱۵ دقیقه فرصت می‌دهد */
        s_grace_seconds_left = WARRANTY_GRACE_PERIOD_SEC;
        s_expired_and_locked = false;
    }

    ESP_LOGI(TAG, "Warranty Subsystem initialized. Active: %s, Locked: %s, Customer Code: %s, Grace: %u s",
             s_info.active ? "YES" : "NO", s_expired_and_locked ? "YES" : "NO",
             s_info.customer_code, (unsigned)s_grace_seconds_left);

    return ESP_OK;
}

bool warranty_is_active(void)
{
    return (s_info.active != 0);
}

bool warranty_is_expired_and_locked(void)
{
    if (s_info.active) return false;
    return s_expired_and_locked;
}

void warranty_dismiss_expired_lock(void)
{
    /* توجه: طبق دستور صریح کارفرما، با بستن دیالوگ قفل به هیچ وجه نباید باز شود؛
     * تا زمان فعال‌سازی رسمی گارانتی، خروجی‌ها و کلیدها قفل قطعی باقی می‌مانند */
    ESP_LOGI(TAG, "Warranty expired dialog dismissed by user. Lockout remains STRICTLY ACTIVE.");
}

uint32_t warranty_get_grace_seconds_left(void)
{
    if (s_info.active) return 0;
    return s_grace_seconds_left;
}

const warranty_info_t *warranty_get_info(void)
{
    return &s_info;
}

/* --- Countdown Tick (1Hz) --- */

void warranty_tick_1s(void)
{
    if (s_info.active) return;
    if (s_expired_and_locked) return;

    /* در اسپلش اسکرین زمان‌گیری مهلت تست هنوز شروع نمی‌شود */
    extern int ui_get_screen_id(void);
    if (ui_get_screen_id() == SCREEN_ID_SPLASH_SCREEN) {
        return;
    }

    if (s_grace_seconds_left > 0) {
        s_grace_seconds_left--;
        if (s_grace_seconds_left == 0) {
            s_expired_and_locked = true;
            warranty_save_to_nvs();
            ESP_LOGW(TAG, "Warranty test grace expired! Lockout engaged. Shutting down all production outputs.");
            
            /* Stop all relays and machines */
            plc_stop_all();
            buzzer_beep_alarm();

            /* Show alert popup safely inside UI lock */
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                open_warranty_expired_dialog();
                lvgl_unlock();
            }
        }
    }
}

/* --- SMS Dispatching Task --- */

static const char *get_jalali_weekday_for_offset(int16_t add_days)
{
    ds3231_time_t dt;
    struct tm ti;
    memset(&ti, 0, sizeof(ti));

    if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24) {
        ti.tm_sec = dt.second;
        ti.tm_min = dt.minute;
        ti.tm_hour = dt.hour;
        ti.tm_mday = dt.date;
        ti.tm_mon = (dt.month > 0) ? (dt.month - 1) : 0;
        ti.tm_year = (2000 + dt.year) - 1900;
        ti.tm_isdst = -1;
    } else {
        time_t now = time(NULL);
        if (now > 1700000000) {
            gmtime_r(&now, &ti);
        } else {
            ti.tm_year = 2026 - 1900;
            ti.tm_mon = 8;
            ti.tm_mday = 10;
        }
    }

    time_t base_epoch = mktime(&ti);
    if (base_epoch <= 0) base_epoch = 1789053000;
    time_t future_epoch = base_epoch + ((time_t)add_days * 86400LL);
    struct tm future_tm;
    gmtime_r(&future_epoch, &future_tm);
    int wday = future_tm.tm_wday;
    if (wday >= 0 && wday < 7) {
        return s_fa_wdays[wday];
    }
    return "";
}

void warranty_build_sms_text(char *out_buf, size_t buf_sz)
{
    if (!out_buf || buf_sz < 128) return;

    char stage_text[512] = {0};
    bool has_stages = false;
    int16_t cum_days = 0;

    for (uint8_t s = 1; s <= 5; s++) {
        int16_t d = day_lock_get_stage_days(s);
        if (d > 0) {
            has_stages = true;
            cum_days += d;
            char j_date[32] = {0};
            day_lock_get_future_jalali_str(cum_days, j_date, sizeof(j_date));
            const char *wname = get_jalali_weekday_for_offset(cum_days);

            char line[96];
            snprintf(line, sizeof(line), "\nقسط %u (%d روز) - سررسید: %s (%s)",
                     (unsigned)s, (int)d, j_date, wname);
            strncat(stage_text, line, sizeof(stage_text) - strlen(stage_text) - 1);
        }
    }

    if (!has_stages) {
        snprintf(stage_text, sizeof(stage_text), "\nدستگاه نقدی (فاقد قفل اقساط)");
    }

    int y = 0, m = 0, d = 0, hr = 0, mi = 0;
    char j_date_time[64] = {0};
    int matched = sscanf(s_info.date_str, "%d/%d/%d %d:%d", &y, &m, &d, &hr, &mi);
    if (matched < 3) {
        matched = sscanf(s_info.date_str, "%d-%d-%d %d:%d", &y, &m, &d, &hr, &mi);
    }
    if (matched >= 3) {
        if (matched < 5) {
            ds3231_time_t cur_dt;
            if (ds3231_get_time(&cur_dt) == ESP_OK) {
                hr = cur_dt.hour;
                mi = cur_dt.minute;
            }
        }
        if (y >= 2000) {
            int jy = 0, jm = 0, jd = 0;
            gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
            snprintf(j_date_time, sizeof(j_date_time), "%04d/%02d/%02d - ساعت %02d:%02d", jy, jm, jd, hr, mi);
        } else {
            snprintf(j_date_time, sizeof(j_date_time), "%04d/%02d/%02d - ساعت %02d:%02d", y, m, d, hr, mi);
        }
    } else {
        snprintf(j_date_time, sizeof(j_date_time), "%s", s_info.date_str[0] ? s_info.date_str : "نامشخص");
    }

    snprintf(out_buf, buf_sz,
        "فعال‌سازی گارانتی بستنی‌ساز آستیاژ CM-700\n"
        "خریدار: %s\n"
        "شهر: %s\n"
        "همراه: %s\n"
        "کد مشتری: %s\n"
        "تاریخ ثبت: %s\n"
        "--- وضعیت اقساط ---%s\n"
        "--- راهنمای بازگشایی چالش ---\n"
        "در زمان قفل، کد ۵ رقمی چالش صفحه را خوانده و با نرم‌افزار آستیاج رمز ۵ رقمی مرحله را دریافت کنید.\n"
        "اگر رمز مدیر وارد کنه تمام تایمر ها غیر فعال میشود\n"
        "کدهای موقت ۱۰ روزه اضطراری: 112233\n"
        "فقط یکبار میتواند استفاده کند",
        s_info.buyer_name[0] ? s_info.buyer_name : "نامشخص",
        s_info.city[0] ? s_info.city : "نامشخص",
        s_info.phone[0] ? s_info.phone : "نامشخص",
        s_info.customer_code,
        j_date_time,
        stage_text
    );
}

void warranty_build_customer_sms_text(char *out_buf, size_t buf_sz)
{
    if (!out_buf || buf_sz < 128) return;

    ui_lang_t lang = ui_lang_get();
    int y = 0, m = 0, d = 0;
    int matched = sscanf(s_info.date_str, "%d/%d/%d", &y, &m, &d);
    if (matched != 3) {
        matched = sscanf(s_info.date_str, "%d-%d-%d", &y, &m, &d);
    }

    char start_date[32] = {0};
    char warr_end[32] = {0};
    char serv_end[32] = {0};

    if (lang == UI_LANG_FA) {
        /* زبان فارسی: تاریخ کاملاً شمسی */
        int jy = y, jm = m, jd = d;
        if (matched == 3) {
            if (y >= 2000) {
                gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
            }
            snprintf(start_date, sizeof(start_date), "%04d/%02d/%02d", jy, jm, jd);
            snprintf(warr_end, sizeof(warr_end), "%04d/%02d/%02d", jy + 1, jm, jd);
            snprintf(serv_end, sizeof(serv_end), "%04d/%02d/%02d", jy + 10, jm, jd);
        } else {
            snprintf(start_date, sizeof(start_date), "%s", s_info.date_str[0] ? s_info.date_str : "نامشخص");
            snprintf(warr_end, sizeof(warr_end), "۱ سال پس از ثبت");
            snprintf(serv_end, sizeof(serv_end), "۱۰ سال پس از ثبت");
        }

        snprintf(out_buf, buf_sz,
            "خریدار گرامی %s،\n"
            "با تبریک خرید و تشکر صمیمانه از حسن اعتماد و انتخاب مجموعه صنعتی آستیاج، گارانتی دستگاه شما با موفقیت فعال گردید:\n\n"
            "دستگاه: بستنی‌ساز صنعتی آستیاژ (مدل ASTIAJ CM-700)\n"
            "شرکت سازنده: ماشین‌سازی ASTIAJ\n"
            "کد اختصاصی مشتری: %s\n"
            "تاریخ شروع گارانتی: %s\n"
            "تاریخ اتمام گارانتی: %s (۱ سال طلایی)\n"
            "پایان خدمات پس از فروش: %s (۱۰ سال تامین قطعات و پشتیبانی)\n\n"
            "پشتیبانی و خدمات کارخانه: 09102367062\n"
            "وبسایت رسمی: www.astiaj.ir",
            s_info.buyer_name[0] ? s_info.buyer_name : "مشتری گرامی",
            s_info.customer_code,
            start_date,
            warr_end,
            serv_end
        );
    } else if (lang == UI_LANG_TR) {
        /* Türkçe: Miladi Tarih */
        int gy = y, gm = m, gd = d;
        if (matched == 3) {
            if (y < 2000) {
                jalali_to_gregorian(y, m, d, &gy, &gm, &gd);
            }
            snprintf(start_date, sizeof(start_date), "%04d/%02d/%02d", gy, gm, gd);
            snprintf(warr_end, sizeof(warr_end), "%04d/%02d/%02d", gy + 1, gm, gd);
            snprintf(serv_end, sizeof(serv_end), "%04d/%02d/%02d", gy + 10, gm, gd);
        } else {
            snprintf(start_date, sizeof(start_date), "%s", s_info.date_str[0] ? s_info.date_str : "Belirtilmedi");
            snprintf(warr_end, sizeof(warr_end), "Kayittan 1 yil sonra");
            snprintf(serv_end, sizeof(serv_end), "Kayittan 10 yil sonra");
        }

        snprintf(out_buf, buf_sz,
            "Sayin Musterimiz %s,\n"
            "Alisverisiniz icin tebrik eder, ASTIAJ Endustriyel Grubu'na guveniniz icin tesekkur ederiz. Cihaz garantiniz basariyla etkinlestirilmistir:\n\n"
            "Cihaz: ASTIAJ Sanayi Tipi Dondurma Makinesi (Model ASTIAJ CM-700)\n"
            "Uretici: ASTIAJ Machinery\n"
            "Musteri Kodu: %s\n"
            "Garanti Baslangic Tarihi: %s\n"
            "Garanti Bitis Tarihi: %s (1 Yil Altin Garanti)\n"
            "Satis Sonrasi Destek Sonu: %s (10 Yil Parca ve Destek)\n\n"
            "Fabrika Destek: 09102367062\n"
            "Resmi Web Sitesi: www.astiaj.ir",
            s_info.buyer_name[0] ? s_info.buyer_name : "Degerli Musterimiz",
            s_info.customer_code,
            start_date,
            warr_end,
            serv_end
        );
    } else {
        /* English: Gregorian Date */
        int gy = y, gm = m, gd = d;
        if (matched == 3) {
            if (y < 2000) {
                jalali_to_gregorian(y, m, d, &gy, &gm, &gd);
            }
            snprintf(start_date, sizeof(start_date), "%04d/%02d/%02d", gy, gm, gd);
            snprintf(warr_end, sizeof(warr_end), "%04d/%02d/%02d", gy + 1, gm, gd);
            snprintf(serv_end, sizeof(serv_end), "%04d/%02d/%02d", gy + 10, gm, gd);
        } else {
            snprintf(start_date, sizeof(start_date), "%s", s_info.date_str[0] ? s_info.date_str : "Unknown");
            snprintf(warr_end, sizeof(warr_end), "1 Year After Reg");
            snprintf(serv_end, sizeof(serv_end), "10 Years After Reg");
        }

        snprintf(out_buf, buf_sz,
            "Dear Customer %s,\n"
            "Congratulations on your purchase and thank you for choosing ASTIAJ Industrial Group. Your machine warranty has been successfully activated:\n\n"
            "Machine: ASTIAJ Industrial Soft-Serve (Model ASTIAJ CM-700)\n"
            "Manufacturer: ASTIAJ Machinery\n"
            "Customer Code: %s\n"
            "Warranty Start Date: %s\n"
            "Warranty Expiry Date: %s (1 Year Golden)\n"
            "End of After-Sales Service: %s (10 Years Parts & Support)\n\n"
            "Factory Support: 09102367062\n"
            "Official Website: www.astiaj.ir",
            s_info.buyer_name[0] ? s_info.buyer_name : "Valued Customer",
            s_info.customer_code,
            start_date,
            warr_end,
            serv_end
        );
    }
}

void warranty_sanitize_phone(const char *in, char *out, size_t out_sz)
{
    if (!in || !out || out_sz < 12) return;
    char digits[32] = {0};
    size_t d_idx = 0;
    for (size_t i = 0; in[i] && d_idx < sizeof(digits) - 1; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c >= '0' && c <= '9') {
            digits[d_idx++] = in[i];
        } else if (c == 0xDB && (unsigned char)in[i+1] >= 0xB0 && (unsigned char)in[i+1] <= 0xB9) {
            /* Persian UTF-8 digits ۰..۹: 0xDB 0xB0 .. 0xDB 0xB9 */
            digits[d_idx++] = '0' + ((unsigned char)in[i+1] - 0xB0);
            i++;
        } else if (c == 0xD9 && (unsigned char)in[i+1] >= 0xA0 && (unsigned char)in[i+1] <= 0xA9) {
            /* Arabic UTF-8 digits ٠..٩: 0xD9 0xA0 .. 0xD9 0xA9 */
            digits[d_idx++] = '0' + ((unsigned char)in[i+1] - 0xA0);
            i++;
        }
    }
    digits[d_idx] = '\0';

    if (d_idx == 10 && digits[0] == '9') {
        snprintf(out, out_sz, "0%s", digits);
    } else if (d_idx == 12 && digits[0] == '9' && digits[1] == '8') {
        snprintf(out, out_sz, "0%s", digits + 2);
    } else if (d_idx == 13 && digits[0] == '0' && digits[1] == '0' && digits[2] == '9' && digits[3] == '8') {
        snprintf(out, out_sz, "0%s", digits + 4);
    } else if (d_idx >= 11) {
        strncpy(out, digits, out_sz - 1);
        out[out_sz - 1] = '\0';
    } else {
        strncpy(out, in, out_sz - 1);
        out[out_sz - 1] = '\0';
    }
}

static void sanitize_phone(const char *in, char *out, size_t out_sz)
{
    warranty_sanitize_phone(in, out, out_sz);
}

/* Escape a UTF-8 string into JSON string format */
static void json_escape_string(const char *in, char *out, size_t out_sz)
{
    if (!in || !out || out_sz == 0) return;
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 4 < out_sz; i++) {
        if (in[i] == '"') {
            out[j++] = '\\'; out[j++] = '"';
        } else if (in[i] == '\\') {
            out[j++] = '\\'; out[j++] = '\\';
        } else if (in[i] == '\n') {
            out[j++] = '\\'; out[j++] = 'n';
        } else if (in[i] == '\r') {
            out[j++] = '\\'; out[j++] = 'r';
        } else if (in[i] == '\t') {
            out[j++] = '\\'; out[j++] = 't';
        } else {
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
}

static bool send_sms_ippanel(const char *recipient, const char *sms_text)
{
    char escaped_msg[1200] = {0};
    json_escape_string(sms_text, escaped_msg, sizeof(escaped_msg));

    char post_data[1400] = {0};
    snprintf(post_data, sizeof(post_data),
        "{\"originator\":\"%s\",\"recipients\":[\"%s\"],\"message\":\"%s\"}",
        IPPANEL_ORIGINATOR, (recipient && recipient[0]) ? recipient : SMS_RECIPIENT_PRIMARY, escaped_msg);

    esp_http_client_config_t config = {
        .url = IPPANEL_REST_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 8000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init http client for IPPanel");
        return false;
    }

    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "AccessKey %s", IPPANEL_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && (status_code == 200 || status_code == 201)) {
        ESP_LOGI(TAG, "SMS to %s delivered via IPPanel successfully! (status=%d)", recipient, status_code);
        return true;
    }

    ESP_LOGW(TAG, "IPPanel send to %s failed (err=%s, status=%d)", recipient, esp_err_to_name(err), status_code);
    return false;
}

static bool send_sms_smsir(const char *recipient, const char *sms_text)
{
    char escaped_msg[1200] = {0};
    json_escape_string(sms_text, escaped_msg, sizeof(escaped_msg));

    char post_data[1400] = {0};
    snprintf(post_data, sizeof(post_data),
        "{\"lineNumber\":%llu,\"messageText\":\"%s\",\"mobiles\":[\"%s\"]}",
        (unsigned long long)SMSIR_LINE_NUMBER, escaped_msg, (recipient && recipient[0]) ? recipient : SMS_RECIPIENT_PRIMARY);

    esp_http_client_config_t config = {
        .url = SMSIR_REST_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 8000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init http client for SMS.IR");
        return false;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "x-api-key", SMSIR_API_KEY);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && (status_code == 200 || status_code == 201)) {
        ESP_LOGI(TAG, "SMS to %s delivered via SMS.IR successfully! (status=%d)", recipient, status_code);
        return true;
    }

    ESP_LOGW(TAG, "SMS.IR send to %s failed (err=%s, status=%d)", recipient, esp_err_to_name(err), status_code);
    return false;
}

static bool send_sms_with_fallback(const char *recipient, const char *text)
{
    if (!recipient || recipient[0] == '\0') return false;
    bool ok = send_sms_ippanel(recipient, text);
    if (!ok) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ok = send_sms_smsir(recipient, text);
    }
    return ok;
}

static void warranty_sms_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "Warranty SMS background task started");

    /* 1. Build and send Management notification SMS */
    char mgr_sms[1024] = {0};
    warranty_build_sms_text(mgr_sms, sizeof(mgr_sms));
    bool mgr_ok = send_sms_with_fallback(SMS_RECIPIENT_PRIMARY, mgr_sms);
    if (mgr_ok) {
        ESP_LOGI(TAG, "Management warranty SMS delivered successfully");
    } else {
        ESP_LOGW(TAG, "Management warranty SMS failed");
    }

    /* 2. Build and send Customer congratulatory & warranty SMS */
    char clean_phone[32] = {0};
    sanitize_phone(s_info.phone, clean_phone, sizeof(clean_phone));

    bool cust_ok = false;
    if (strlen(clean_phone) >= 10) {
        vTaskDelay(pdMS_TO_TICKS(1500)); /* Safe spacing between gateway requests */
        char cust_sms[1024] = {0};
        warranty_build_customer_sms_text(cust_sms, sizeof(cust_sms));
        cust_ok = send_sms_with_fallback(clean_phone, cust_sms);
        if (cust_ok) {
            ESP_LOGI(TAG, "Customer warranty SMS delivered to %s successfully", clean_phone);
        } else {
            ESP_LOGW(TAG, "Customer warranty SMS to %s failed", clean_phone);
        }
    } else {
        ESP_LOGW(TAG, "Invalid customer phone number (%s), skipping customer SMS", s_info.phone);
    }

    if (mgr_ok || cust_ok) {
        s_info.sms_sent = 1;
        s_info.sms_pending = 0;
        warranty_save_to_nvs();
        ESP_LOGI(TAG, "Warranty SMS batch finished and saved to NVS");
    } else {
        ESP_LOGW(TAG, "All SMS deliveries failed, keeping pending=1");
        s_info.sms_pending = 1;
        warranty_save_to_nvs();
    }

    s_sms_task_handle = NULL;
    vTaskDelete(NULL);
}

void warranty_trigger_sms_dispatch(void)
{
    if (s_sms_task_handle != NULL) {
        ESP_LOGW(TAG, "SMS dispatch task already running");
        return;
    }
    xTaskCreatePinnedToCore(warranty_sms_task, "warr_sms_tsk", 10240, NULL, 2, &s_sms_task_handle, 0);
}

void warranty_on_wifi_connected(void)
{
    if (s_info.active && s_info.sms_pending && !s_info.sms_sent) {
        ESP_LOGI(TAG, "Wi-Fi connected and pending warranty SMS detected, triggering dispatcher");
        warranty_trigger_sms_dispatch();
    }
}

/* --- Activation API --- */

esp_err_t warranty_activate(const char *name, const char *city, const char *phone, bool is_offline_pin)
{
    s_info.active = 1;
    strncpy(s_info.buyer_name, name ? name : "", sizeof(s_info.buyer_name) - 1);
    s_info.buyer_name[sizeof(s_info.buyer_name) - 1] = '\0';

    strncpy(s_info.city, city ? city : "", sizeof(s_info.city) - 1);
    s_info.city[sizeof(s_info.city) - 1] = '\0';

    strncpy(s_info.phone, phone ? phone : "", sizeof(s_info.phone) - 1);
    s_info.phone[sizeof(s_info.phone) - 1] = '\0';

    /* Generate / preserve 11-digit customer code */
    warranty_get_customer_code(s_info.customer_code, sizeof(s_info.customer_code));

    /* Format registration date and time */
    jalali_time_t jt;
    if (ds3231_get_time_jalali(&jt) == ESP_OK && jt.jy >= 1400) {
        snprintf(s_info.date_str, sizeof(s_info.date_str), "%04d/%02d/%02d %02d:%02d",
                 jt.jy, jt.jm, jt.jd, jt.hour, jt.minute);
    } else {
        ds3231_time_t dt;
        if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24) {
            snprintf(s_info.date_str, sizeof(s_info.date_str), "20%02d/%02d/%02d %02d:%02d",
                     dt.year, dt.month, dt.date, dt.hour, dt.minute);
        } else {
            snprintf(s_info.date_str, sizeof(s_info.date_str), "1405/06/15 12:00");
        }
    }

    s_expired_and_locked = false;

    if (wifi_is_connected()) {
        s_info.sms_pending = 1;
        warranty_save_to_nvs();
        warranty_trigger_sms_dispatch();
    } else {
        s_info.sms_pending = 1; /* will send on next Wi-Fi connect */
        warranty_save_to_nvs();
        ESP_LOGI(TAG, "Warranty activated offline (is_offline_pin=%d). SMS queued for when Wi-Fi connects.",
                 is_offline_pin);
    }

    /* Update Screen F6 UI */
    if (lvgl_lock(pdMS_TO_TICKS(100))) {
        ui_screens_update_warranty_f6();
        lvgl_unlock();
    }

    return ESP_OK;
}

void warranty_get_display_date(char *out_str, size_t out_sz, bool is_jalali)
{
    if (!out_str || out_sz < 12) return;
    if (!s_info.date_str[0]) {
        snprintf(out_str, out_sz, "---");
        return;
    }
    int y = 0, m = 0, d = 0;
    if (sscanf(s_info.date_str, "%d/%d/%d", &y, &m, &d) != 3 &&
        sscanf(s_info.date_str, "%d-%d-%d", &y, &m, &d) != 3) {
        snprintf(out_str, out_sz, "%s", s_info.date_str);
        return;
    }

    if (y >= 1300 && y < 1600) {
        /* تاریخ ذخیره شده شمسی است */
        if (is_jalali) {
            snprintf(out_str, out_sz, "%04d/%02d/%02d", y, m, d);
        } else {
            int gy = 0, gm = 0, gd = 0;
            jalali_to_gregorian(y, m, d, &gy, &gm, &gd);
            snprintf(out_str, out_sz, "%04d/%02d/%02d", gy, gm, gd);
        }
    } else if (y >= 2000 && y < 2100) {
        /* تاریخ ذخیره شده میلادی است */
        if (!is_jalali) {
            snprintf(out_str, out_sz, "%04d/%02d/%02d", y, m, d);
        } else {
            int jy = 0, jm = 0, jd = 0;
            gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
            snprintf(out_str, out_sz, "%04d/%02d/%02d", jy, jm, jd);
        }
    } else {
        snprintf(out_str, out_sz, "%04d/%02d/%02d", y, m, d);
    }
}

/* =========================================================================
 * اطلاع‌رسانی پیامکی بازگشایی و تسویه اقساط به شماره همراه مدیر
 * ========================================================================= */

void warranty_build_installment_sms_text(char *out_buf, size_t buf_sz, uint8_t stage, const char *pin, int16_t days)
{
    if (!out_buf || buf_sz < 128) return;

    /* تاریخ و زمان شمسی دقیق */
    char date_time_str[64] = {0};
    ds3231_time_t dt;
    if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24) {
        int jy = 0, jm = 0, jd = 0;
        gregorian_to_jalali(2000 + dt.year, dt.month, dt.date, &jy, &jm, &jd);
        snprintf(date_time_str, sizeof(date_time_str), "%04d/%02d/%02d - %02d:%02d",
                 jy, jm, jd, dt.hour, dt.minute);
    } else {
        day_lock_get_future_date_str(0, date_time_str, sizeof(date_time_str), true);
    }

    /* شناسه مک دستگاه */
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);

    /* عنوان مرحله قسط */
    char stage_title[96] = {0};
    if (stage >= 1 && stage <= 5) {
        snprintf(stage_title, sizeof(stage_title), "قسط %u (مرحله %u)", (unsigned)stage, (unsigned)stage);
    } else if (stage > 5) {
        snprintf(stage_title, sizeof(stage_title), "تسویه نهایی و آزادسازی دائم");
    } else {
        snprintf(stage_title, sizeof(stage_title), "تمدید موقت ۱۰ روزه اضطراری");
    }

    snprintf(out_buf, buf_sz,
        "تایید و بازگشایی قسط دستگاه آستیاژ\n"
        "دستگاه: بستنی‌ساز صنعتی ASTIAJ CM-700\n"
        "وضعیت قسط: %s\n"
        "رمز ثبت شده: %s\n"
        "مدت تمدید: %d روز\n"
        "تاریخ و زمان: %s\n"
        "--- مشخصات مشتری ---\n"
        "نام خریدار: %s\n"
        "همراه: %s\n"
        "شهر: %s\n"
        "کد مشتری: %s\n"
        "مک آدرس: %02X:%02X:%02X:%02X:%02X:%02X\n"
        "سیستم: اتوماسیون صنعتی آستیاژ V1.5",
        stage_title,
        pin ? pin : "---",
        (int)days,
        date_time_str,
        s_info.buyer_name[0] ? s_info.buyer_name : "ثبت نشده",
        s_info.phone[0] ? s_info.phone : "ثبت نشده",
        s_info.city[0] ? s_info.city : "ثبت نشده",
        s_info.customer_code[0] ? s_info.customer_code : "---",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
}

typedef struct {
    uint8_t stage;
    char pin[32];
    int16_t days;
} inst_sms_task_arg_t;

static void installment_sms_task(void *pvParameters)
{
    inst_sms_task_arg_t *arg = (inst_sms_task_arg_t *)pvParameters;
    if (!arg) {
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Installment unlock SMS worker started for stage %u", (unsigned)arg->stage);
    char sms_body[1024] = {0};
    warranty_build_installment_sms_text(sms_body, sizeof(sms_body), arg->stage, arg->pin, arg->days);
    free(arg);

    bool ok = send_sms_with_fallback(SMS_RECIPIENT_PRIMARY, sms_body);
    if (ok) {
        ESP_LOGI(TAG, "Installment unlock SMS delivered to manager (%s) successfully!", SMS_RECIPIENT_PRIMARY);
    } else {
        ESP_LOGW(TAG, "Installment unlock SMS send failed (Wi-Fi offline or gateway unreachable)");
    }

    vTaskDelete(NULL);
}

void warranty_send_installment_unlock_sms(uint8_t stage, const char *pin, int16_t days)
{
    inst_sms_task_arg_t *arg = (inst_sms_task_arg_t *)malloc(sizeof(inst_sms_task_arg_t));
    if (!arg) return;
    arg->stage = stage;
    strncpy(arg->pin, pin ? pin : "", sizeof(arg->pin) - 1);
    arg->pin[sizeof(arg->pin) - 1] = '\0';
    arg->days = days;

    xTaskCreatePinnedToCore(installment_sms_task, "inst_sms", 10240, arg, 2, NULL, 0);
}

