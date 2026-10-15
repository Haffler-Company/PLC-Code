/**
 * @file haffler_warranty.cpp
 * @brief Haffler Industrial Warranty & Day-Lock Security Engine Implementation
 * @author Haffler Industrial Engineering
 */

#include "haffler_warranty.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Arduino.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <lvgl.h>
#include <RTClib.h>
#include "build_time.h"
#include "date_utils.h"
#include "storage.h"
#include "ui_localization.h"
#include "fault_log.h"

using namespace ui_localization;

extern Storage storage;
extern DateTime rtc_now_s();
extern "C" void jack_touch_beep(void);
extern "C" int get_ui_language_index(void);
extern "C" lv_obj_t *fw_nav_warranty_btn(void);
extern const lv_font_t ui_font_kalameh_bold_14;
extern const lv_font_t ui_font_kalameh_bold_18;

static const char *TAG = "WARRANTY";

#define NVS_WARRANTY_NS       "haff_warr"
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
volatile bool g_warranty_expired_dialog_request = false;
volatile bool g_warranty_screen_request = false;
static TaskHandle_t s_sms_task_handle = NULL;

/* UI Objects */
static lv_obj_t *s_warr_modal_backdrop = NULL;
static lv_obj_t *s_ta_warr_name = NULL;
static lv_obj_t *s_ta_warr_city = NULL;
static lv_obj_t *s_ta_warr_phone = NULL;
static lv_obj_t *s_lbl_ph_name = NULL;
static lv_obj_t *s_lbl_ph_city = NULL;
static lv_obj_t *s_lbl_ph_phone = NULL;
static lv_obj_t *s_warr_active_ta = NULL;
static lv_obj_t *s_warr_kb = NULL;
static lv_obj_t *s_lbl_warr_status = NULL;

static lv_obj_t *s_warr_expired_backdrop = NULL;
static lv_obj_t *s_warr_no_net_backdrop = NULL;

bool is_warranty_screen_open(void)
{
    return (s_warr_modal_backdrop != NULL);
}

bool warranty_expired_dialog_is_open(void)
{
    return (s_warr_expired_backdrop != NULL);
}

/* --- Luhn Algorithm Helpers --- */

static int calculate_luhn_check_digit(const char *ten_digits)
{
    if (!ten_digits) return 0;
    int sum = 0;
    for (int i = 9; i >= 0; i--) {
        int val = ten_digits[i] - '0';
        if (val < 0 || val > 9) val = 0;
        int pos_from_right = (10 - i) + 1;
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
        int pos_from_right = 11 - i;
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

    uint64_t d10 = 1000000000ULL + (z % 9000000000ULL);
    char ten_str[12];
    snprintf(ten_str, sizeof(ten_str), "%010llu", (unsigned long long)d10);

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

    // Also mirror to storage.setWarrantyDate
    if (s_info.active && s_info.date_str[0]) {
        storage.setWarrantyDate(String(s_info.date_str));
    }

    ESP_LOGI(TAG, "Warranty NVS saved (active=%u, code=%s, sms_sent=%u)",
             (unsigned)s_info.active, s_info.customer_code, (unsigned)s_info.sms_sent);
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
        ESP_LOGI(TAG, "No warranty record in NVS; checking storage compatibility");
        if (storage.isWarrantyActive()) {
            s_info.active = 1;
            strncpy(s_info.date_str, storage.getWarrantyDate().c_str(), sizeof(s_info.date_str) - 1);
        }
    }

    if (s_info.customer_code[0] == '\0' || strlen(s_info.customer_code) != 11) {
        warranty_get_customer_code(s_info.customer_code, sizeof(s_info.customer_code));
    }

    if (s_info.active) {
        s_expired_and_locked = false;
        s_grace_seconds_left = 0;
    } else {
        s_grace_seconds_left = WARRANTY_GRACE_PERIOD_SEC;
        s_expired_and_locked = false;
    }

    ESP_LOGI(TAG, "Warranty initialized: active=%d, code=%s, grace=%us",
             (int)s_info.active, s_info.customer_code, (unsigned)s_grace_seconds_left);
    return ESP_OK;
}

bool warranty_is_active(void)
{
    return (s_info.active != 0) || storage.isWarrantyActive();
}

bool warranty_is_expired_and_locked(void)
{
    if (warranty_is_active()) return false;
    return s_expired_and_locked;
}

void warranty_dismiss_expired_lock(void)
{
    ESP_LOGI(TAG, "Warranty expired dialog dismissed. Lockout remains STRICTLY ACTIVE.");
}

uint32_t warranty_get_grace_seconds_left(void)
{
    if (warranty_is_active()) return 0;
    return s_grace_seconds_left;
}

const warranty_info_t *warranty_get_info(void)
{
    return &s_info;
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
            digits[d_idx++] = '0' + ((unsigned char)in[i+1] - 0xB0);
            i++;
        } else if (c == 0xD9 && (unsigned char)in[i+1] >= 0xA0 && (unsigned char)in[i+1] <= 0xA9) {
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

void warranty_tick_1s(void)
{
    if (warranty_is_active()) return;
    if (s_expired_and_locked) return;

    if (s_grace_seconds_left > 0) {
        s_grace_seconds_left--;
        if (s_grace_seconds_left == 0) {
            s_expired_and_locked = true;
            ESP_LOGW(TAG, "WARRANTY 15-MINUTE TEST GRACE PERIOD EXPIRED! Locking machine.");
            fault_log_record(FAULT_LEASE_EXPIRED, 0);
            g_warranty_expired_dialog_request = true;
        }
    }
}

/* --- JSON Escape & SMS Sending --- */

static void json_escape_string(const char *in, char *out, size_t out_sz)
{
    if (!in || !out || out_sz == 0) return;
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 4 < out_sz; i++) {
        if (in[i] == '"' || in[i] == '\\') {
            out[j++] = '\\'; out[j++] = in[i];
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

    esp_http_client_config_t config = {};
    config.url = IPPANEL_REST_URL;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 8000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "AccessKey %s", IPPANEL_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    return (err == ESP_OK && (status_code == 200 || status_code == 201));
}

static bool send_sms_smsir(const char *recipient, const char *sms_text)
{
    char escaped_msg[1200] = {0};
    json_escape_string(sms_text, escaped_msg, sizeof(escaped_msg));

    char post_data[1400] = {0};
    snprintf(post_data, sizeof(post_data),
        "{\"lineNumber\":%llu,\"messageText\":\"%s\",\"mobiles\":[\"%s\"]}",
        (unsigned long long)SMSIR_LINE_NUMBER, escaped_msg, (recipient && recipient[0]) ? recipient : SMS_RECIPIENT_PRIMARY);

    esp_http_client_config_t config = {};
    config.url = SMSIR_REST_URL;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 8000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-API-KEY", SMSIR_API_KEY);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    return (err == ESP_OK && (status_code == 200 || status_code == 201));
}

bool haffler_send_sms_direct(const char *recipient, const char *sms_text)
{
    if (!recipient || !recipient[0] || !sms_text || !sms_text[0]) return false;
    bool ok = send_sms_ippanel(recipient, sms_text);
    if (!ok) {
        ok = send_sms_smsir(recipient, sms_text);
    }
    return ok;
}

typedef struct {
    char phone[20];
    uint32_t otp;
} mfg_otp_task_param_t;

static void mfg_otp_task(void *pv)
{
    mfg_otp_task_param_t *p = (mfg_otp_task_param_t *)pv;
    if (!p) { vTaskDelete(NULL); return; }
    char sms_body[256];
    snprintf(sms_body, sizeof(sms_body),
        "کد تایید سازنده هافلر: %05u\n"
        "جهت فعال‌سازی هشدارهای دستگاه صنعتی جوش تک‌سر",
        (unsigned)p->otp);
    ESP_LOGI(TAG, "Sending OTP %05u to manufacturer phone %s...", (unsigned)p->otp, p->phone);
    bool ok = haffler_send_sms_direct(p->phone, sms_body);
    ESP_LOGI(TAG, "OTP SMS send result: %s", ok ? "SUCCESS" : "FAILED");
    free(p);
    vTaskDelete(NULL);
}

void haffler_send_mfg_otp_sms(const char *phone, uint32_t otp)
{
    if (!phone || !phone[0]) return;
    mfg_otp_task_param_t *param = (mfg_otp_task_param_t *)malloc(sizeof(mfg_otp_task_param_t));
    if (!param) return;
    strncpy(param->phone, phone, sizeof(param->phone) - 1);
    param->phone[sizeof(param->phone) - 1] = '\0';
    BaseType_t ok = xTaskCreatePinnedToCore(mfg_otp_task, "OtpSMS", 4096, param, 2, NULL, 0);
    if (ok != pdPASS) {
        ok = xTaskCreatePinnedToCore(mfg_otp_task, "OtpSMS", 3584, param, 2, NULL, 0);
    }
    if (ok != pdPASS) {
        xTaskCreatePinnedToCore(mfg_otp_task, "OtpSMS", 3072, param, 2, NULL, 0);
    }
}

void warranty_build_sms_text(char *out_buf, size_t buf_sz)
{
    if (!out_buf || buf_sz < 128) return;
    String comp_name = storage.getCompanyName();
    if (comp_name.length() == 0) comp_name = "هافلر (Haffler)";
    String mfg_ph = storage.getManufacturerPhone();
    if (mfg_ph.length() == 0) mfg_ph = SMS_RECIPIENT_PRIMARY;

    snprintf(out_buf, buf_sz,
        "فعال‌سازی گارانتی دستگاه جوش تک‌سر UPVC\n"
        "برند: %s\n"
        "خریدار: %s\n"
        "شهر: %s\n"
        "همراه: %s\n"
        "کد مشتری: %s\n"
        "تاریخ ثبت: %s\n"
        "پشتیبانی فنی: %s",
        comp_name.c_str(),
        s_info.buyer_name[0] ? s_info.buyer_name : "نامشخص",
        s_info.city[0] ? s_info.city : "نامشخص",
        s_info.phone[0] ? s_info.phone : "نامشخص",
        s_info.customer_code,
        s_info.date_str[0] ? s_info.date_str : "نامشخص",
        mfg_ph.c_str()
    );
}

void warranty_build_customer_sms_text(char *out_buf, size_t buf_sz)
{
    if (!out_buf || buf_sz < 128) return;
    String comp_name = storage.getCompanyName();
    if (comp_name.length() == 0) comp_name = "هافلر (Haffler)";
    String mfg_ph = storage.getManufacturerPhone();
    if (mfg_ph.length() == 0) mfg_ph = SMS_RECIPIENT_PRIMARY;

    snprintf(out_buf, buf_sz,
        "خریدار گرامی %s،\n"
        "با تبریک خرید و تشکر از حسن انتخاب شما، گارانتی رسمی دستگاه با موفقیت فعال گردید:\n\n"
        "دستگاه: دستگاه جوش تک‌سر UPVC\n"
        "شرکت سازنده: %s\n"
        "کد اختصاصی مشتری: %s\n"
        "تاریخ ثبت: %s\n"
        "مدت گارانتی: ۱ سال طلایی\n"
        "پشتیبانی و قطعات: ۱۰ سال\n\n"
        "پشتیبانی کارخانه: %s\n"
        "وبسایت رسمی: www.haffler-ai.ir",
        s_info.buyer_name[0] ? s_info.buyer_name : "مشتری گرامی",
        comp_name.c_str(),
        s_info.customer_code,
        s_info.date_str[0] ? s_info.date_str : "نامشخص",
        mfg_ph.c_str()
    );
}

static void warranty_sms_task(void *pvParam)
{
    (void)pvParam;
    ESP_LOGI(TAG, "Warranty SMS task running on Core 0...");

    char factory_sms[600] = {0};
    warranty_build_sms_text(factory_sms, sizeof(factory_sms));

    const char *mgr_recipient = SMS_RECIPIENT_PRIMARY;
    String mfg_ph = storage.getManufacturerPhone();
    if (mfg_ph.length() >= 10) {
        mgr_recipient = mfg_ph.c_str();
    }

    bool ok_mgr = haffler_send_sms_direct(mgr_recipient, factory_sms);
    if (strcmp(mgr_recipient, SMS_RECIPIENT_PRIMARY) != 0) {
        // همچنین به شماره اولیه کارخانه هم به عنوان پشتیبان کارخانه ارسال شود
        haffler_send_sms_direct(SMS_RECIPIENT_PRIMARY, factory_sms);
    }

    if (s_info.phone[0] == '0' && s_info.phone[1] == '9') {
        char cust_sms[600] = {0};
        warranty_build_customer_sms_text(cust_sms, sizeof(cust_sms));
        haffler_send_sms_direct(s_info.phone, cust_sms);
    }

    if (ok_mgr) {
        s_info.sms_sent = 1;
        s_info.sms_pending = 0;
        warranty_save_to_nvs();
        ESP_LOGI(TAG, "Warranty activation SMS successfully dispatched!");
    } else {
        s_info.sms_pending = 1;
        warranty_save_to_nvs();
        ESP_LOGW(TAG, "Warranty SMS dispatch queued for later.");
    }

    s_sms_task_handle = NULL;
    vTaskDelete(NULL);
}

void warranty_trigger_sms_dispatch(void)
{
    if (s_sms_task_handle == NULL) {
        BaseType_t ok = xTaskCreatePinnedToCore(warranty_sms_task, "WarrSMS", 4096, NULL, 2, &s_sms_task_handle, 0);
        if (ok != pdPASS) {
            ok = xTaskCreatePinnedToCore(warranty_sms_task, "WarrSMS", 3584, NULL, 2, &s_sms_task_handle, 0);
        }
        if (ok != pdPASS) {
            xTaskCreatePinnedToCore(warranty_sms_task, "WarrSMS", 3072, NULL, 2, &s_sms_task_handle, 0);
        }
    }
}

void warranty_on_wifi_connected(void)
{
    if (s_info.active && s_info.sms_pending && !s_info.sms_sent) {
        ESP_LOGI(TAG, "WiFi connected -> Dispatching queued warranty activation SMS...");
        warranty_trigger_sms_dispatch();
    }
}

esp_err_t warranty_activate(const char *name, const char *city, const char *phone, bool is_offline_pin)
{
    s_info.active = 1;
    s_expired_and_locked = false;
    s_grace_seconds_left = 0;

    if (name) strncpy(s_info.buyer_name, name, sizeof(s_info.buyer_name) - 1);
    if (city) strncpy(s_info.city, city, sizeof(s_info.city) - 1);
    if (phone) warranty_sanitize_phone(phone, s_info.phone, sizeof(s_info.phone));

    // Get current date string from RTC (or build date fallback)
    DateTime now = rtc_now_s();
    int gy = now.year();
    int gm = now.month();
    int gd = now.day();
    if (gy < 2020 || gy > 2099 || gm < 1 || gm > 12 || gd < 1 || gd > 31) {
        gy = BUILD_YEAR;
        gm = BUILD_MONTH;
        gd = BUILD_DAY;
    }
    int jy = 1400, jm = 1, jd = 1;
    date_utils::gregorian_to_jalali(gy, gm, gd, jy, jm, jd);
    snprintf(s_info.date_str, sizeof(s_info.date_str), "%04d-%02d-%02d", jy, jm, jd);

    if (s_info.customer_code[0] == '\0') {
        warranty_get_customer_code(s_info.customer_code, sizeof(s_info.customer_code));
    }

    if (is_offline_pin) {
        s_info.sms_pending = 1;
        s_info.sms_sent = 0;
    } else {
        if (WiFi.status() == WL_CONNECTED) {
            warranty_trigger_sms_dispatch();
        } else {
            s_info.sms_pending = 1;
        }
    }

    warranty_save_to_nvs();

    // Hide warranty button in update screen if visible
    lv_obj_t *wbtn = fw_nav_warranty_btn();
    if (wbtn) lv_obj_add_flag(wbtn, LV_OBJ_FLAG_HIDDEN);

    close_warranty_screen();
    hide_warranty_expired_dialog();
    return ESP_OK;
}

/* Forward declarations */
static void action_warr_submit_clicked(lv_event_t *e);
static void action_warr_offline_clicked(lv_event_t *e);
static void action_warr_close_clicked(lv_event_t *e);

/* Helper to synchronize overlaid placeholder labels */
static void warranty_update_placeholders(void)
{
    if (s_lbl_ph_name && s_ta_warr_name && lv_obj_is_valid(s_lbl_ph_name) && lv_obj_is_valid(s_ta_warr_name)) {
        const char *t = lv_textarea_get_text(s_ta_warr_name);
        if (t && strlen(t) > 0) lv_obj_add_flag(s_lbl_ph_name, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(s_lbl_ph_name, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_lbl_ph_city && s_ta_warr_city && lv_obj_is_valid(s_lbl_ph_city) && lv_obj_is_valid(s_ta_warr_city)) {
        const char *t = lv_textarea_get_text(s_ta_warr_city);
        if (t && strlen(t) > 0) lv_obj_add_flag(s_lbl_ph_city, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(s_lbl_ph_city, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_lbl_ph_phone && s_ta_warr_phone && lv_obj_is_valid(s_lbl_ph_phone) && lv_obj_is_valid(s_ta_warr_phone)) {
        const char *t = lv_textarea_get_text(s_ta_warr_phone);
        if (t && strlen(t) > 0) lv_obj_add_flag(s_lbl_ph_phone, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(s_lbl_ph_phone, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Virtual Keyboard maps: Pure Persian and Clean 4x3 Numeric */
static const char * const s_kb_map_fa[] = {
    "ض", "ص", "ث", "ق", "ف", "غ", "ع", "ه", "خ", "ح", "ژ", "\n",
    "ش", "س", "ی", "ب", "ل", "ا", "ت", "ن", "م", "ک", "گ", "\n",
    "ظ", "ط", "ز", "ر", "ذ", "د", "پ", "و", "چ", "ج", "آ", "\n",
    "فاصله", "پاک", "تایید", ""
};
#define W_1 (lv_buttonmatrix_ctrl_t)1
#define W_2 (lv_buttonmatrix_ctrl_t)2
#define W_3 (lv_buttonmatrix_ctrl_t)3
#define W_4 (lv_buttonmatrix_ctrl_t)4
#define W_5 (lv_buttonmatrix_ctrl_t)5

static const lv_buttonmatrix_ctrl_t s_kb_ctrl_fa[] = {
    W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1,
    W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1,
    W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1, W_1,
    W_5, W_3, W_3
};

static const char * const s_kb_map_num[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    "پاک", "0", "تایید", ""
};
static const lv_buttonmatrix_ctrl_t s_kb_ctrl_num[] = {
    W_1, W_1, W_1,
    W_1, W_1, W_1,
    W_1, W_1, W_1,
    W_1, W_1, W_1
};

static void warranty_kb_color_draw_task_cb(lv_event_t *e)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    if (!draw_task) draw_task = (lv_draw_task_t *)lv_event_get_param(e);
    if (!draw_task) return;
    lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
    if (!base_dsc || base_dsc->part != LV_PART_ITEMS) return;

    int32_t id = base_dsc->id1;
    if (id < 0) return;
    const char *txt = lv_buttonmatrix_get_button_text(obj, (uint32_t)id);
    if (!txt || txt[0] == '\0') return;

    bool is_pressed = (lv_buttonmatrix_get_selected_button(obj) == (uint32_t)id) && (lv_obj_get_state(obj) & LV_STATE_PRESSED);

    lv_draw_fill_dsc_t *fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
    lv_draw_label_dsc_t *label_dsc = lv_draw_task_get_label_dsc(draw_task);
    lv_draw_border_dsc_t *border_dsc = lv_draw_task_get_border_dsc(draw_task);
    if (!fill_dsc && !label_dsc && !border_dsc) return;

    bool is_bksp = (strcmp(txt, "پاک") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcasecmp(txt, "del") == 0);
    bool is_ok   = (strcmp(txt, "تایید") == 0 || strcmp(txt, LV_SYMBOL_OK) == 0 || strcasecmp(txt, "ok") == 0);

    if (is_bksp) {
        if (fill_dsc) {
            fill_dsc->color = is_pressed ? lv_color_hex(0xb91c1c) : lv_color_hex(0xdc2626); // RED
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 6;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_hex(0xffffff);
        }
        if (border_dsc) {
            border_dsc->color = lv_color_hex(0xef4444);
            border_dsc->width = 1;
        }
    } else if (is_ok) {
        if (fill_dsc) {
            fill_dsc->color = is_pressed ? lv_color_hex(0x15803d) : lv_color_hex(0x16a34a); // GREEN
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 6;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_hex(0xffffff);
        }
        if (border_dsc) {
            border_dsc->color = lv_color_hex(0x22c55e);
            border_dsc->width = 1;
        }
    }
}

static void warranty_kb_event_cb(lv_event_t *e)
{
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
        if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;
        const char *txt = lv_buttonmatrix_get_button_text(kb, btn_id);
        if (!txt) return;

        jack_touch_beep();

        if (!s_warr_active_ta || !lv_obj_is_valid(s_warr_active_ta)) return;

        if (strcmp(txt, "فاصله") == 0 || strcmp(txt, "Space") == 0) {
            lv_textarea_add_char(s_warr_active_ta, ' ');
        } else if (strcmp(txt, "پاک") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            lv_textarea_delete_char(s_warr_active_ta);
        } else if (strcmp(txt, "تایید") == 0 || strcmp(txt, LV_SYMBOL_OK) == 0) {
            if (s_warr_active_ta == s_ta_warr_name && s_ta_warr_city) {
                s_warr_active_ta = s_ta_warr_city;
                if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) {
                    lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex(0xcbd5e1), 0);
                    lv_obj_set_style_border_width(s_ta_warr_name, 1, 0);
                }
                if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
                    lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0x0284c7), 0);
                    lv_obj_set_style_border_width(s_ta_warr_city, 2, 0);
                }
                lv_buttonmatrix_set_map(kb, s_kb_map_fa);
                lv_buttonmatrix_set_ctrl_map(kb, s_kb_ctrl_fa);
            } else if (s_warr_active_ta == s_ta_warr_city && s_ta_warr_phone) {
                s_warr_active_ta = s_ta_warr_phone;
                if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
                    lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0xcbd5e1), 0);
                    lv_obj_set_style_border_width(s_ta_warr_city, 1, 0);
                }
                if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) {
                    lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex(0x0284c7), 0);
                    lv_obj_set_style_border_width(s_ta_warr_phone, 2, 0);
                }
                lv_buttonmatrix_set_map(kb, s_kb_map_num);
                lv_buttonmatrix_set_ctrl_map(kb, s_kb_ctrl_num);
            } else {
                action_warr_submit_clicked(e);
            }
        } else {
            /* Normal letter or number */
            lv_textarea_add_text(s_warr_active_ta, txt);
        }

        warranty_update_placeholders();
    }
}

/* --- UI Modal Implementations --- */

static void action_warr_ta_click(lv_event_t *e)
{
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    s_warr_active_ta = ta;

    if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) {
        lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex((ta == s_ta_warr_name) ? 0x0284c7 : 0xcbd5e1), 0);
        lv_obj_set_style_border_width(s_ta_warr_name, (ta == s_ta_warr_name) ? 2 : 1, 0);
    }
    if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
        lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex((ta == s_ta_warr_city) ? 0x0284c7 : 0xcbd5e1), 0);
        lv_obj_set_style_border_width(s_ta_warr_city, (ta == s_ta_warr_city) ? 2 : 1, 0);
    }
    if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) {
        lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex((ta == s_ta_warr_phone) ? 0x0284c7 : 0xcbd5e1), 0);
        lv_obj_set_style_border_width(s_ta_warr_phone, (ta == s_ta_warr_phone) ? 2 : 1, 0);
    }

    if (s_warr_kb && lv_obj_is_valid(s_warr_kb)) {
        if (ta == s_ta_warr_phone) {
            lv_buttonmatrix_set_map(s_warr_kb, s_kb_map_num);
            lv_buttonmatrix_set_ctrl_map(s_warr_kb, s_kb_ctrl_num);
        } else {
            lv_buttonmatrix_set_map(s_warr_kb, s_kb_map_fa);
            lv_buttonmatrix_set_ctrl_map(s_warr_kb, s_kb_ctrl_fa);
        }
        lv_obj_clear_flag(s_warr_kb, LV_OBJ_FLAG_HIDDEN);
    }
    warranty_update_placeholders();
}

static void action_warr_submit_clicked(lv_event_t *e)
{
    (void)e;
    jack_touch_beep();

    const char *name = s_ta_warr_name ? lv_textarea_get_text(s_ta_warr_name) : "";
    const char *city = s_ta_warr_city ? lv_textarea_get_text(s_ta_warr_city) : "";
    const char *phone = s_ta_warr_phone ? lv_textarea_get_text(s_ta_warr_phone) : "";

    char clean_phone[20] = {0};
    warranty_sanitize_phone(phone, clean_phone, sizeof(clean_phone));

    if (strlen(name) < 2) {
        if (s_lbl_warr_status) lv_label_set_text(s_lbl_warr_status, get_warr_name_err_text());
        return;
    }
    if (strlen(clean_phone) < 10) {
        if (s_lbl_warr_status) lv_label_set_text(s_lbl_warr_status, get_warr_phone_err_text());
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        open_warranty_no_net_dialog();
        return;
    }

    warranty_activate(name, city, clean_phone, false);
}

static void action_warr_offline_clicked(lv_event_t *e)
{
    (void)e;
    jack_touch_beep();
    const char *name = (s_ta_warr_name && strlen(lv_textarea_get_text(s_ta_warr_name)) > 0) ? lv_textarea_get_text(s_ta_warr_name) : "";
    const char *city = (s_ta_warr_city && strlen(lv_textarea_get_text(s_ta_warr_city)) > 0) ? lv_textarea_get_text(s_ta_warr_city) : "محل کارخانه";
    const char *phone = (s_ta_warr_phone && strlen(lv_textarea_get_text(s_ta_warr_phone)) > 0) ? lv_textarea_get_text(s_ta_warr_phone) : "";

    char clean_phone[20] = {0};
    warranty_sanitize_phone(phone, clean_phone, sizeof(clean_phone));

    if (strlen(name) < 2) {
        if (s_lbl_warr_status) lv_label_set_text(s_lbl_warr_status, get_warr_name_err_text());
        return;
    }
    if (strlen(clean_phone) < 10) {
        if (s_lbl_warr_status) lv_label_set_text(s_lbl_warr_status, get_warr_phone_err_text());
        return;
    }

    warranty_activate(name, city, clean_phone, true);
}

static void action_warr_close_clicked(lv_event_t *e)
{
    (void)e;
    jack_touch_beep();
    close_warranty_screen();
}

void close_warranty_screen(void)
{
    if (s_warr_modal_backdrop) {
        lv_obj_del(s_warr_modal_backdrop);
        s_warr_modal_backdrop = NULL;
        s_ta_warr_name = NULL;
        s_ta_warr_city = NULL;
        s_ta_warr_phone = NULL;
        s_lbl_ph_name = NULL;
        s_lbl_ph_city = NULL;
        s_lbl_ph_phone = NULL;
        s_warr_active_ta = NULL;
        s_warr_kb = NULL;
        s_lbl_warr_status = NULL;
    }
}

void open_warranty_screen(void)
{
    close_warranty_screen();

    // 1. Full-screen backdrop on top layer
    s_warr_modal_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_warr_modal_backdrop, 0, 0);
    lv_obj_set_size(s_warr_modal_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_warr_modal_backdrop, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_warr_modal_backdrop, 180, 0);
    lv_obj_clear_flag(s_warr_modal_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    // 2. White Card Container
    lv_obj_t *card = lv_obj_create(s_warr_modal_backdrop);
    lv_obj_set_size(card, 460, 304);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_style_base_dir(card, LV_BASE_DIR_LTR, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *ttl = lv_label_create(card);
    lv_obj_set_style_text_font(ttl, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_color(ttl, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_base_dir(ttl, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(ttl, get_warr_title_text());
    lv_obj_align(ttl, LV_ALIGN_TOP_MID, 0, 2);

    // Customer Code Badge (Dual-label flex container to eliminate BiDi punctuation flip)
    char code[16] = {0};
    warranty_get_customer_code(code, sizeof(code));
    lv_obj_t *code_cont = lv_obj_create(card);
    lv_obj_set_size(code_cont, LV_SIZE_CONTENT, 22);
    lv_obj_set_style_bg_opa(code_cont, 0, 0);
    lv_obj_set_style_border_width(code_cont, 0, 0);
    lv_obj_set_style_pad_all(code_cont, 0, 0);
    lv_obj_set_flex_flow(code_cont, LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_style_pad_gap(code_cont, 8, 0);
    lv_obj_clear_flag(code_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(code_cont, LV_ALIGN_TOP_MID, 0, 22);

    lv_obj_t *code_ttl = lv_label_create(code_cont);
    lv_obj_set_style_text_font(code_ttl, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_color(code_ttl, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(code_ttl, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(code_ttl, "کد مشتری");

    lv_obj_t *code_val = lv_label_create(code_cont);
    lv_obj_set_style_text_font(code_val, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_color(code_val, lv_color_hex(0x0284c7), 0);
    lv_label_set_text(code_val, code);

    // Form inputs (Row of 3 Textareas in deterministic LTR space)
    // Name (Right side for Persian)
    s_ta_warr_name = lv_textarea_create(card);
    lv_obj_set_size(s_ta_warr_name, 140, 32);
    lv_obj_set_pos(s_ta_warr_name, 296, 46);
    lv_textarea_set_placeholder_text(s_ta_warr_name, get_warr_name_ph());
    lv_textarea_set_one_line(s_ta_warr_name, true);
    lv_obj_set_style_text_font(s_ta_warr_name, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_font(s_ta_warr_name, &ui_font_kalameh_bold_14, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(s_ta_warr_name, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_width(s_ta_warr_name, 2, 0);
    lv_obj_add_event_cb(s_ta_warr_name, action_warr_ta_click, LV_EVENT_CLICKED, NULL);

    // City (Center)
    s_ta_warr_city = lv_textarea_create(card);
    lv_obj_set_size(s_ta_warr_city, 136, 32);
    lv_obj_set_pos(s_ta_warr_city, 152, 46);
    lv_textarea_set_placeholder_text(s_ta_warr_city, get_warr_city_ph());
    lv_textarea_set_one_line(s_ta_warr_city, true);
    lv_obj_set_style_text_font(s_ta_warr_city, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_font(s_ta_warr_city, &ui_font_kalameh_bold_14, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(s_ta_warr_city, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(s_ta_warr_city, 1, 0);
    lv_obj_add_event_cb(s_ta_warr_city, action_warr_ta_click, LV_EVENT_CLICKED, NULL);

    // Phone (Left side)
    s_ta_warr_phone = lv_textarea_create(card);
    lv_obj_set_size(s_ta_warr_phone, 136, 32);
    lv_obj_set_pos(s_ta_warr_phone, 8, 46);
    lv_textarea_set_placeholder_text(s_ta_warr_phone, "09xxxxxxxxx");
    lv_textarea_set_one_line(s_ta_warr_phone, true);
    lv_obj_set_style_text_font(s_ta_warr_phone, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_font(s_ta_warr_phone, &ui_font_kalameh_bold_14, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(s_ta_warr_phone, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(s_ta_warr_phone, 1, 0);
    lv_obj_add_event_cb(s_ta_warr_phone, action_warr_ta_click, LV_EVENT_CLICKED, NULL);

    // Action buttons bar (Row of 3 Buttons in deterministic LTR space)
    // Submit (Right side - Green)
    lv_obj_t *btn_submit = lv_btn_create(card);
    lv_obj_set_size(btn_submit, 140, 30);
    lv_obj_set_pos(btn_submit, 296, 82);
    lv_obj_set_style_bg_color(btn_submit, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_radius(btn_submit, 6, 0);
    lv_obj_add_event_cb(btn_submit, action_warr_submit_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_sub = lv_label_create(btn_submit);
    lv_label_set_text(lbl_sub, get_warr_submit_text());
    lv_obj_set_style_text_font(lbl_sub, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(lbl_sub, LV_BASE_DIR_RTL, 0);
    lv_obj_center(lbl_sub);

    // Offline (Center - Blue)
    lv_obj_t *btn_offline = lv_btn_create(card);
    lv_obj_set_size(btn_offline, 136, 30);
    lv_obj_set_pos(btn_offline, 152, 82);
    lv_obj_set_style_bg_color(btn_offline, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_radius(btn_offline, 6, 0);
    lv_obj_add_event_cb(btn_offline, action_warr_offline_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_off = lv_label_create(btn_offline);
    lv_label_set_text(lbl_off, get_warr_offline_text());
    lv_obj_set_style_text_font(lbl_off, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(lbl_off, LV_BASE_DIR_RTL, 0);
    lv_obj_center(lbl_off);

    // Cancel / Close (Left side - Red)
    lv_obj_t *btn_close = lv_btn_create(card);
    lv_obj_set_size(btn_close, 136, 30);
    lv_obj_set_pos(btn_close, 8, 82);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_add_event_cb(btn_close, action_warr_close_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_cls = lv_label_create(btn_close);
    lv_label_set_text(lbl_cls, get_warr_cancel_text());
    lv_obj_set_style_text_font(lbl_cls, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(lbl_cls, LV_BASE_DIR_RTL, 0);
    lv_obj_center(lbl_cls);

    // Status / instruction label
    s_lbl_warr_status = lv_label_create(card);
    lv_label_set_text(s_lbl_warr_status, get_warr_prompt_text());
    lv_obj_set_style_text_font(s_lbl_warr_status, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_color(s_lbl_warr_status, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(s_lbl_warr_status, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(s_lbl_warr_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_lbl_warr_status, LV_ALIGN_TOP_MID, 0, 116);

    // 3. Virtual Keypad (ButtonMatrix) docked at bottom with Apple Clean Light Style
    s_warr_kb = lv_buttonmatrix_create(card);
    lv_obj_set_size(s_warr_kb, 444, 150);
    lv_obj_align(s_warr_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_base_dir(s_warr_kb, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_bg_color(s_warr_kb, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_color(s_warr_kb, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(s_warr_kb, 1, 0);
    lv_obj_set_style_radius(s_warr_kb, 8, 0);
    lv_obj_set_style_pad_all(s_warr_kb, 2, 0);
    lv_obj_set_style_pad_gap(s_warr_kb, 2, 0);
    lv_obj_clear_flag(s_warr_kb, LV_OBJ_FLAG_SCROLLABLE);

    /* Apple HIG Clean buttons */
    lv_obj_set_style_bg_color(s_warr_kb, lv_color_hex(0xffffff), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_warr_kb, 255, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_warr_kb, lv_color_hex(0x0f172a), LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_warr_kb, lv_color_hex(0xcbd5e1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_warr_kb, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_warr_kb, 6, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(s_warr_kb, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_warr_kb, &ui_font_kalameh_bold_14, LV_PART_ITEMS);

    lv_buttonmatrix_set_map(s_warr_kb, s_kb_map_fa);
    lv_buttonmatrix_set_ctrl_map(s_warr_kb, s_kb_ctrl_fa);

    lv_obj_add_event_cb(s_warr_kb, warranty_kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_warr_kb, warranty_kb_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(s_warr_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    s_warr_active_ta = s_ta_warr_name;
}

/* --- Expired Lockout Dialog --- */

static void action_expired_activate(lv_event_t *e)
{
    (void)e;
    jack_touch_beep();
    hide_warranty_expired_dialog();
    open_warranty_screen();
}

static void action_expired_dismiss(lv_event_t *e)
{
    (void)e;
    jack_touch_beep();
    hide_warranty_expired_dialog();
}

void hide_warranty_expired_dialog(void)
{
    if (s_warr_expired_backdrop) {
        lv_obj_del(s_warr_expired_backdrop);
        s_warr_expired_backdrop = NULL;
    }
}

void open_warranty_expired_dialog(void)
{
    hide_warranty_expired_dialog();

    s_warr_expired_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_warr_expired_backdrop, 0, 0);
    lv_obj_set_size(s_warr_expired_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_warr_expired_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_warr_expired_backdrop, 210, 0);
    lv_obj_clear_flag(s_warr_expired_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *box = lv_obj_create(s_warr_expired_backdrop);
    lv_obj_set_size(box, 420, 240);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(box, 14, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_border_width(box, 3, 0);
    lv_obj_set_style_base_dir(box, LV_BASE_DIR_RTL, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(box);
    lv_label_set_text(t, get_warr_expired_text());
    lv_obj_set_style_text_font(t, &ui_font_kalameh_bold_18, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_base_dir(t, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *desc = lv_label_create(box);
    lv_obj_set_width(desc, 380);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_label_set_text(desc, get_warr_expired_desc());
    lv_obj_set_style_text_font(desc, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(desc, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_color(desc, lv_color_hex(0x334155), 0);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 48);

    lv_obj_t *btn_act = lv_btn_create(box);
    lv_obj_set_size(btn_act, 170, 42);
    lv_obj_align(btn_act, LV_ALIGN_BOTTOM_LEFT, 16, -16);
    lv_obj_set_style_bg_color(btn_act, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_radius(btn_act, 8, 0);
    lv_obj_add_event_cb(btn_act, action_expired_activate, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l1 = lv_label_create(btn_act);
    lv_label_set_text(l1, get_warr_submit_text());
    lv_obj_set_style_text_font(l1, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(l1, LV_BASE_DIR_RTL, 0);
    lv_obj_center(l1);

    lv_obj_t *btn_dis = lv_btn_create(box);
    lv_obj_set_size(btn_dis, 170, 42);
    lv_obj_align(btn_dis, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
    lv_obj_set_style_bg_color(btn_dis, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_radius(btn_dis, 8, 0);
    lv_obj_add_event_cb(btn_dis, action_expired_dismiss, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l2 = lv_label_create(btn_dis);
    lv_label_set_text(l2, get_warr_dismiss_text());
    lv_obj_set_style_text_font(l2, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(l2, LV_BASE_DIR_RTL, 0);
    lv_obj_center(l2);
}

void hide_warranty_no_net_dialog(void)
{
    if (s_warr_no_net_backdrop) {
        lv_obj_del(s_warr_no_net_backdrop);
        s_warr_no_net_backdrop = NULL;
    }
}

void open_warranty_no_net_dialog(void)
{
    hide_warranty_no_net_dialog();

    s_warr_no_net_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_warr_no_net_backdrop, 0, 0);
    lv_obj_set_size(s_warr_no_net_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_warr_no_net_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_warr_no_net_backdrop, 200, 0);

    lv_obj_t *box = lv_obj_create(s_warr_no_net_backdrop);
    lv_obj_set_size(box, 380, 200);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_base_dir(box, LV_BASE_DIR_RTL, 0);

    lv_obj_t *t = lv_label_create(box);
    lv_label_set_text(t, get_warr_no_net_title());
    lv_obj_set_style_text_font(t, &ui_font_kalameh_bold_18, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0xd97706), 0);
    lv_obj_set_style_base_dir(t, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *desc = lv_label_create(box);
    lv_obj_set_width(desc, 340);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_label_set_text(desc, get_warr_no_net_desc());
    lv_obj_set_style_text_font(desc, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(desc, LV_BASE_DIR_RTL, 0);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 48);

    lv_obj_t *btn_off = lv_btn_create(box);
    lv_obj_set_size(btn_off, 140, 36);
    lv_obj_align(btn_off, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(btn_off, lv_color_hex(0x0284c7), 0);
    lv_obj_add_event_cb(btn_off, [](lv_event_t *e){
        (void)e;
        hide_warranty_no_net_dialog();
        action_warr_offline_clicked(e);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l1 = lv_label_create(btn_off);
    lv_label_set_text(l1, get_warr_offline_text());
    lv_obj_set_style_text_font(l1, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(l1, LV_BASE_DIR_RTL, 0);
    lv_obj_center(l1);

    lv_obj_t *btn_cls = lv_btn_create(box);
    lv_obj_set_size(btn_cls, 140, 36);
    lv_obj_align(btn_cls, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0x64748b), 0);
    lv_obj_add_event_cb(btn_cls, [](lv_event_t *e){
        (void)e;
        hide_warranty_no_net_dialog();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l2 = lv_label_create(btn_cls);
    lv_label_set_text(l2, get_warr_cancel_text());
    lv_obj_set_style_text_font(l2, &ui_font_kalameh_bold_14, 0);
    lv_obj_set_style_base_dir(l2, LV_BASE_DIR_RTL, 0);
    lv_obj_center(l2);
}
