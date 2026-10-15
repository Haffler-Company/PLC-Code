/**
 * @file ota_update.c
 * @brief ASTIAJ ASTIAJ Industrial OTA Update Subsystem & Server Metadata Client
 * @author ASTIAJ Company
 */

#include "ota_update.h"
#include "wifi.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl_port.h"
#include "ui_resume.h"
#include "ui/ui_lang.h"
#include "project_config.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <lwip/sockets.h>
#include "esp_transport.h"

/* Structural helper to access underlying socket in ESP-IDF 5.5 */
struct esp_http_client_socket_shim {
    int                         redirect_counter;
    int                         max_redirection_count;
    int                         max_authorization_retries;
    int                         process_again;
    struct http_parser          *parser;
    struct http_parser_settings *parser_settings;
    void                        *transport_list;
    esp_transport_handle_t      transport;
};

static inline int esp_http_client_get_socket(esp_http_client_handle_t client)
{
    if (!client) return -1;
    struct esp_http_client_socket_shim *shim = (struct esp_http_client_socket_shim *)client;
    if (!shim->transport) return -1;
    return esp_transport_get_socket(shim->transport);
}

static const char *TAG = "ota_update";

static const char *s_default_bin_urls[OTA_MODEL_COUNT] = {
    "http://ota.haffler-infi.ir/other/ice-cream/firmware.bin",
    "http://ota.haffler-infi.ir/upvc/config-test/firmware.bin"
};

static const char *s_default_lastfile_urls[OTA_MODEL_COUNT] = {
    "http://ota.haffler-infi.ir/other/ice-cream/lastfile",
    "http://ota.haffler-infi.ir/upvc/config-test/lastfile"
};

static const char *s_fallback_lastfile_urls[OTA_MODEL_COUNT][3] = {
    {
        "http://ota-plc.s3.ir-thr-at1.arvanstorage.ir/other/ice-cream/lastfile",
        NULL,
        NULL
    },
    {
        "http://ota-plc.s3.ir-thr-at1.arvanstorage.ir/upvc/config-test/lastfile",
        NULL,
        NULL
    }
};

#include "esp_attr.h"

EXT_RAM_BSS_ATTR static lastfile_info_t s_model_info[OTA_MODEL_COUNT];
static SemaphoreHandle_t s_ota_mutex = NULL;

static volatile bool s_scan_busy = false;
static volatile bool s_scan_done = false;
static volatile uint32_t s_scan_revision = 0;
static TaskHandle_t s_scan_task_handle = NULL;
static esp_http_client_handle_t s_active_scan_client = NULL;
static volatile bool s_scan_cancel_requested = false;
#define OTA_SCAN_STACK_BYTES 10240
static StaticTask_t s_scan_tcb;
static StackType_t s_scan_stack[OTA_SCAN_STACK_BYTES] __attribute__((aligned(16)));

static char s_custom_url[256] = {0};
static bool s_custom_url_active = false;

static ota_install_state_t s_install_state = OTA_INST_IDLE;
static ota_progress_cb_t s_install_cb = NULL;
static char s_install_err_msg[96] = {0};
static char s_install_target_url[256] = {0};
static TaskHandle_t s_install_task_handle = NULL;
static volatile uint8_t s_install_pct = 0;
static volatile uint32_t s_install_dl_bytes = 0;
static volatile uint32_t s_install_total_bytes = 0;
static int s_install_model_idx = 0;
static void ota_safely_restore_tasks(void);

/* ---------------- Universal Zero-Tofu Sanitizer ---------------- */
void ota_sanitize_server_text(char *dst, size_t dst_sz, const char *src)
{
    if (!dst || dst_sz == 0) return;
    dst[0] = '\0';
    if (!src || !src[0]) return;

    size_t d = 0;
    const unsigned char *s = (const unsigned char *)src;

    // Skip UTF-8 BOM
    if (s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        s += 3;
    }

    while (*s && d + 8 < dst_sz) {
        // ۱. نیم‌فاصله‌های مجازی فارسی ZWNJ U+200C -> ' '
        if (s[0] == 0xE2 && s[1] == 0x80 && s[2] == 0x8C) {
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // ZWJ U+200D -> حذف
        if (s[0] == 0xE2 && s[1] == 0x80 && s[2] == 0x8D) {
            s += 3;
            continue;
        }
        // نشانگرهای LRM و RLM (0xE2 0x80 0x8E, 0x8F) -> حذف
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] == 0x8E || s[2] == 0x8F)) {
            s += 3;
            continue;
        }
        // کدهای تعبیه BiDi -> حذف
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0xAA && s[2] <= 0xAE)) {
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x81 && (s[2] >= 0xA6 && s[2] <= 0xA9)) {
            s += 3;
            continue;
        }
        // Soft-Hyphen U+00AD -> حذف
        if (s[0] == 0xC2 && s[1] == 0xAD) {
            s += 2;
            continue;
        }
        // Non-breaking space U+00A0 -> ' '
        if (s[0] == 0xC2 && s[1] == 0xA0) {
            dst[d++] = ' ';
            s += 2;
            continue;
        }
        // فاصله‌های نازک و عریض یونیکد U+2000..U+200B -> ' '
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x80 && s[2] <= 0x8B)) {
            dst[d++] = ' ';
            s += 3;
            continue;
        }

        // ۲. بولت‌ها، اشکال، تیک و ضربدر
        // بولت‌های U+2022, U+2023, U+2043 -> "- "
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0xA2 && s[2] <= 0xA4)) {
            dst[d++] = '-';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // تیک تایید U+2713, U+2714 -> "+ "
        if (s[0] == 0xE2 && s[1] == 0x9C && (s[2] == 0x93 || s[2] == 0x94)) {
            dst[d++] = '+';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // ضربدر U+2716..U+2718 -> "x "
        if (s[0] == 0xE2 && s[1] == 0x9C && (s[2] >= 0x96 && s[2] <= 0x98)) {
            dst[d++] = 'x';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // فلش راست -> " -> "
        if (s[0] == 0xE2 && s[1] == 0x86 && s[2] == 0x92) {
            dst[d++] = ' ';
            dst[d++] = '-';
            dst[d++] = '>';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // خط‌تیره‌های یونیکد U+2010..U+2015 -> " - "
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x90 && s[2] <= 0x95)) {
            dst[d++] = ' ';
            dst[d++] = '-';
            dst[d++] = ' ';
            s += 3;
            continue;
        }

        // ۳. کاراکترهای اسکی معمول و کاراکترهای UTF-8 فارسی (0xD8..0xDB)
        if (*s < 0x80) {
            dst[d++] = (char)*s++;
        } else if ((s[0] >= 0xD8 && s[0] <= 0xDB) && (s[1] >= 0x80 && s[1] <= 0xBF)) {
            // کاراکتر الفبای فارسی معتبر دو بایتی
            dst[d++] = (char)s[0];
            dst[d++] = (char)s[1];
            s += 2;
        } else if (s[0] >= 0xC0 && s[0] <= 0xDF) {
            // کاراکتر دو بایتی لاتین/ترکی
            dst[d++] = (char)s[0];
            dst[d++] = (char)s[1];
            s += 2;
        } else if (s[0] >= 0xE0 && s[0] <= 0xEF) {
            // رد کردن کاراکتر سه‌بایتی ناشناخته به عنوان فاصله امن
            dst[d++] = ' ';
            s += 3;
        } else if (s[0] >= 0xF0) {
            // ایموجی‌های ۴ بایتی -> حذف
            s += 4;
        } else {
            s++;
        }
    }
    dst[d] = '\0';
}

/* ---------------- Fallback Offline Metadata ---------------- */
static void init_fallback_metadata(void)
{
    // مدل ۰: بستنی‌ساز صنعتی
    memset(&s_model_info[0], 0, sizeof(lastfile_info_t));
    strncpy(s_model_info[0].name, "دستگاه بستنی‌ساز صنعتی آستیاج (آستیاژ)", sizeof(s_model_info[0].name) - 1);
    strncpy(s_model_info[0].ver, FIRMWARE_VERSION_CODE, sizeof(s_model_info[0].ver) - 1);
    strncpy(s_model_info[0].bdate, "2026-09-19", sizeof(s_model_info[0].bdate) - 1);
    strncpy(s_model_info[0].btime, "12:00", sizeof(s_model_info[0].btime) - 1);
    strncpy(s_model_info[0].sha256, "E8B92D1C34A7F01234567890ABCDEF1234567890ABCDEF1234567890ABCDEF12", sizeof(s_model_info[0].sha256) - 1);
    s_model_info[0].size_bytes = 5242880;

    s_model_info[0].desc_fa_cnt = 5;
    strncpy(s_model_info[0].desc_fa[0], "کنترل دمای دوگانه مخزن شیر (+4C) و سیلندر انجماد (-6C)", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_fa[1], "حفاظت ضد یخ‌زدگی سیلندر و کنترل بار موتور پارو", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_fa[2], "تایمر ضد اتصال کوتاه کمپرسور سرمایش (Anti-Short Cycle 10s)", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_fa[3], "پشتیبانی از پمپ سیروپ لبه رنگی و موتور پارو مخزن شیر", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_fa[4], "رابط کاربری مدرن Apple HIG با تم تیره و انیمیشن بدون لگ", OTA_LINE_MAX_LEN - 1);

    s_model_info[0].desc_en_cnt = 5;
    strncpy(s_model_info[0].desc_en[0], "Dual temperature control: hopper (+4C) and freezing cylinder (-6C)", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_en[1], "Cylinder anti-freeze protection and beater motor load control", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_en[2], "Anti-short cycle timer for refrigeration compressor (10s)", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_en[3], "Milk tank paddle motor and syrup edge pump support", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_en[4], "Modern Apple HIG UI with dark theme and zero-lag animations", OTA_LINE_MAX_LEN - 1);

    s_model_info[0].desc_tr_cnt = 5;
    strncpy(s_model_info[0].desc_tr[0], "Cift sicaklik kontrolu: hazne (+4C) ve dondurma silindiri (-6C)", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_tr[1], "Silindir donma korumasi ve karistirici motor yuk kontrolu", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_tr[2], "Kompresor kisa devre koruma zamanlayicisi (10s)", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_tr[3], "Süt tankı palet motoru ve kenar sos pompası desteği", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[0].desc_tr[4], "Apple HIG koyu tema ve akici sifir-gecikme animasyonlar", OTA_LINE_MAX_LEN - 1);

    strncpy(s_model_info[0].pinmap_inputs,
            "• ورودی ۰: سنسور دور پولی پارو\n"
            "• ورودی ۱: کلاچ عقب (سفتی بستنی)\n"
            "• ورودی ۲: اهرم جلو (شمارنده قیف)\n"
            "• ورودی ۳ و ۴: کنترل‌بار پارو و کمپرسور\n"
            "• ورودی ۵: ورودی کمکی رزرو\n"
            "• ورودی ۶: سنسور خالی بودن مخزن شیر\n"
            "• ورودی ۷: شستی استارت چرخه\n"
            "• سنسورهای دمای مخزن و سیلندر\n"
            "• خطوط کمکی دوطرفه (M0 تا M7)",
            OTA_PINMAP_MAX_LEN - 1);

    strncpy(s_model_info[0].pinmap_outputs,
            "• خروجی ۰: نگهدارنده\n"
            "• خروجی ۱: کمپرسور بزرگ سیلندر\n"
            "• خروجی ۲: کنتاکتور موتور پارو\n"
            "• خروجی ۳: پمپ کرم لبه رنگی\n"
            "• خروجی ۴: شیر باد لبه رنگی\n"
            "• خروجی ۵: موتور پارو مخزن شیر\n"
            "• خروجی ۶: رله ریست مدار فرمان\n"
            "• خطوط کمکی دوطرفه (M0 تا M7)",
            OTA_PINMAP_MAX_LEN - 1);

    s_model_info[0].is_valid = true;
    s_model_info[0].scan_error = false;

    // مدل ۱: برنامه کانفیگ و تستر
    memset(&s_model_info[1], 0, sizeof(lastfile_info_t));
    strncpy(s_model_info[1].name, "برنامه کانفیگ و تست سخت‌افزار آستیاج", sizeof(s_model_info[1].name) - 1);
    strncpy(s_model_info[1].ver, "V1.9.11", sizeof(s_model_info[1].ver) - 1);
    strncpy(s_model_info[1].bdate, "2026-09-07", sizeof(s_model_info[1].bdate) - 1);
    strncpy(s_model_info[1].btime, "23:39", sizeof(s_model_info[1].btime) - 1);
    strncpy(s_model_info[1].sha256, "24CBD9C108F06AB8E0E8D6A908E4D0CEB6A64D6FA81FC8DB4171BC4FD6EE5526", sizeof(s_model_info[1].sha256) - 1);
    s_model_info[1].size_bytes = 4327872;

    s_model_info[1].desc_fa_cnt = 2;
    strncpy(s_model_info[1].desc_fa[0], "محیط تست کارگاهی کلیه رله‌ها و ورودی‌های دیجیتال", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[1].desc_fa[1], "تست سنسورهای دما و ارتباطات دیجیتال مادربرد", OTA_LINE_MAX_LEN - 1);

    s_model_info[1].desc_en_cnt = 2;
    strncpy(s_model_info[1].desc_en[0], "Workshop test environment for all relays and digital inputs", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[1].desc_en[1], "Thermocouple and motherboard I2C/SPI bus testing", OTA_LINE_MAX_LEN - 1);

    s_model_info[1].desc_tr_cnt = 2;
    strncpy(s_model_info[1].desc_tr[0], "Tum roleler ve dijital girisler icin test ortami", OTA_LINE_MAX_LEN - 1);
    strncpy(s_model_info[1].desc_tr[1], "Termokupl ve anakart I2C/SPI iletisim testi", OTA_LINE_MAX_LEN - 1);

    strncpy(s_model_info[1].pinmap_inputs,
            "• ورودی‌های ایزوله دیاگ (۰ تا ۷)\n"
            "• سنسور دمای مخزن شیر\n"
            "• سنسور دمای سیلندر برودت\n"
            "• خطوط کمکی دوطرفه (M0 تا M7)",
            OTA_PINMAP_MAX_LEN - 1);

    strncpy(s_model_info[1].pinmap_outputs,
            "• خروجی‌های رله تست (۰ تا ۷)\n"
            "• خطوط کمکی دوطرفه (M0 تا M7)",
            OTA_PINMAP_MAX_LEN - 1);

    s_model_info[1].is_valid = true;
    s_model_info[1].scan_error = false;
}

/* ---------------- HTTP GET Fetcher Helper ---------------- */
static char *http_fetch_text(const char *url, int timeout_ms)
{
    if (!url || !url[0] || s_scan_cancel_requested) return NULL;

    bool is_https = (strncmp(url, "https://", 8) == 0);
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = timeout_ms,
        .buffer_size = 4096,
        .skip_cert_common_name_check = true,
        .keep_alive_enable = false,
        .crt_bundle_attach = is_https ? esp_crt_bundle_attach : NULL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init http client for %s", url);
        return NULL;
    }
    s_active_scan_client = client;

    if (s_scan_cancel_requested) {
        esp_http_client_cleanup(client);
        s_active_scan_client = NULL;
        return NULL;
    }

    esp_http_client_set_header(client, "User-Agent", "ASTIAJ-IceCream-OTA/1.0");
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_header(client, "Connection", "close");

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_http_client_open failed: %s for %s", esp_err_to_name(err), url);
        esp_http_client_cleanup(client);
        s_active_scan_client = NULL;
        return NULL;
    }

    if (s_scan_cancel_requested) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        s_active_scan_client = NULL;
        return NULL;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200 || s_scan_cancel_requested) {
        if (status_code != 200) {
            ESP_LOGW(TAG, "HTTP GET returned status %d for %s", status_code, url);
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        s_active_scan_client = NULL;
        return NULL;
    }

    int alloc_sz = (content_length > 0 && content_length < 65536) ? (content_length + 1) : 65536;
    char *buf = (char *)malloc(alloc_sz);
    if (!buf) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        s_active_scan_client = NULL;
        return NULL;
    }

    int total_read = 0;
    int zero_read_cnt = 0;
    while (total_read < alloc_sz - 1) {
        if (s_scan_cancel_requested) {
            free(buf);
            buf = NULL;
            break;
        }
        int r = esp_http_client_read(client, buf + total_read, alloc_sz - 1 - total_read);
        if (r < 0) break;
        if (r == 0) {
            if (++zero_read_cnt >= 5) break; /* ۵ بار خواندن صفر = EOF واقعی */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        zero_read_cnt = 0;
        total_read += r;
        /* yield هر ۸KB یک‌بار تا WDT تسک‌های دیگر تحریک نشود */
        if ((total_read & 0x1FFF) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    if (buf) {
        buf[total_read] = '\0';
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    s_active_scan_client = NULL;
    return buf;
}

/* Ensure string doesn't end with a truncated multibyte UTF-8 lead byte */
static void utf8_fix_truncation(char *s)
{
    if (!s) return;
    size_t len = strlen(s);
    if (len == 0) return;
    size_t i = len;
    while (i > 0 && ((unsigned char)s[i - 1] & 0xC0) == 0x80) {
        i--;
    }
    if (i > 0) {
        unsigned char lead = (unsigned char)s[i - 1];
        size_t needed = 0;
        if ((lead & 0xE0) == 0xC0) needed = 2;
        else if ((lead & 0xF0) == 0xE0) needed = 3;
        else if ((lead & 0xF8) == 0xF0) needed = 4;
        if (needed > 0 && (len - (i - 1)) < needed) {
            s[i - 1] = '\0';
        }
    }
}

/* ---------------- INI Manifest Parser ---------------- */
static void parse_lastfile_into_info(const char *raw, lastfile_info_t *info)
{
    if (!raw || !info) return;

    char line[384];
    const char *p = raw;
    char section[48] = {0};

    char *raw_inputs = (char *)calloc(1, OTA_PINMAP_MAX_LEN);
    char *raw_outputs = (char *)calloc(1, OTA_PINMAP_MAX_LEN);
    if (!raw_inputs || !raw_outputs) {
        if (raw_inputs) free(raw_inputs);
        if (raw_outputs) free(raw_outputs);
        return;
    }
    size_t in_len = 0;
    size_t out_len = 0;

    info->desc_fa_cnt = 0;
    info->desc_en_cnt = 0;
    info->desc_tr_cnt = 0;
    bool generic_desc_used = false;

    while (*p) {
        size_t l = 0;
        while (*p && *p != '\r' && *p != '\n' && l < sizeof(line) - 1) {
            line[l++] = *p++;
        }
        line[l] = '\0';
        while (*p == '\r' || *p == '\n') p++;

        // Trim leading whitespace
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;

        if (*s == '#' || *s == ';' || *s == '\0') continue;

        if (*s == '[') {
            char *end = strchr(s, ']');
            if (end) {
                size_t slen = end - (s + 1);
                if (slen < sizeof(section)) {
                    memcpy(section, s + 1, slen);
                    section[slen] = '\0';
                }
            }
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = s;
        char *val = eq + 1;

        // Trim key & val
        while (*key && (*key == ' ' || *key == '\t')) key++;
        char *kend = key + strlen(key) - 1;
        while (kend > key && (*kend == ' ' || *kend == '\t')) *kend-- = '\0';

        while (*val && (*val == ' ' || *val == '\t')) val++;
        char *vend = val + strlen(val) - 1;
        while (vend > val && (*vend == ' ' || *vend == '\t')) *vend-- = '\0';

        if (strcmp(section, "version") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(info->name, val, sizeof(info->name) - 1);
                utf8_fix_truncation(info->name);
            } else if (strcmp(key, "version") == 0 || strcmp(key, "code") == 0 || strcmp(key, "display") == 0) {
                if (!info->ver[0] || strcmp(key, "version") == 0 || strcmp(key, "code") == 0) {
                    strncpy(info->ver, val, sizeof(info->ver) - 1);
                }
            } else if (strcmp(key, "build_date") == 0) {
                strncpy(info->bdate, val, sizeof(info->bdate) - 1);
            } else if (strcmp(key, "build_time") == 0) {
                strncpy(info->btime, val, sizeof(info->btime) - 1);
            }
        } else if (strcmp(section, "binary") == 0) {
            if (strcmp(key, "size_bytes") == 0) {
                info->size_bytes = (uint32_t)strtoul(val, NULL, 10);
            } else if (strcmp(key, "sha256") == 0) {
                strncpy(info->sha256, val, sizeof(info->sha256) - 1);
            }
        } else if (strcmp(section, "description_fa") == 0) {
            if (generic_desc_used) {
                info->desc_fa_cnt = 0;
                generic_desc_used = false;
            }
            if (strncmp(key, "line", 4) == 0 && info->desc_fa_cnt < OTA_DESC_MAX_LINES) {
                bool dup = false;
                for (int d = 0; d < info->desc_fa_cnt; d++) {
                    if (strcmp(info->desc_fa[d], val) == 0) { dup = true; break; }
                }
                if (!dup) {
                    strncpy(info->desc_fa[info->desc_fa_cnt], val, OTA_LINE_MAX_LEN - 1);
                    info->desc_fa[info->desc_fa_cnt][OTA_LINE_MAX_LEN - 1] = '\0';
                    utf8_fix_truncation(info->desc_fa[info->desc_fa_cnt]);
                    info->desc_fa_cnt++;
                }
            }
        } else if (strcmp(section, "description") == 0) {
            if (info->desc_fa_cnt == 0 || generic_desc_used) {
                generic_desc_used = true;
                if (strncmp(key, "line", 4) == 0 && info->desc_fa_cnt < OTA_DESC_MAX_LINES) {
                    bool dup = false;
                    for (int d = 0; d < info->desc_fa_cnt; d++) {
                        if (strcmp(info->desc_fa[d], val) == 0) { dup = true; break; }
                    }
                    if (!dup) {
                        strncpy(info->desc_fa[info->desc_fa_cnt], val, OTA_LINE_MAX_LEN - 1);
                        info->desc_fa[info->desc_fa_cnt][OTA_LINE_MAX_LEN - 1] = '\0';
                        utf8_fix_truncation(info->desc_fa[info->desc_fa_cnt]);
                        info->desc_fa_cnt++;
                    }
                }
            }
        } else if (strcmp(section, "description_en") == 0) {
            if (strncmp(key, "line", 4) == 0 && info->desc_en_cnt < OTA_DESC_MAX_LINES) {
                strncpy(info->desc_en[info->desc_en_cnt], val, OTA_LINE_MAX_LEN - 1);
                info->desc_en[info->desc_en_cnt][OTA_LINE_MAX_LEN - 1] = '\0';
                utf8_fix_truncation(info->desc_en[info->desc_en_cnt]);
                info->desc_en_cnt++;
            }
        } else if (strcmp(section, "description_tr") == 0) {
            if (strncmp(key, "line", 4) == 0 && info->desc_tr_cnt < OTA_DESC_MAX_LINES) {
                strncpy(info->desc_tr[info->desc_tr_cnt], val, OTA_LINE_MAX_LEN - 1);
                info->desc_tr[info->desc_tr_cnt][OTA_LINE_MAX_LEN - 1] = '\0';
                utf8_fix_truncation(info->desc_tr[info->desc_tr_cnt]);
                info->desc_tr_cnt++;
            }
        } else if (strcmp(section, "inputs") == 0) {
            if (strncmp(key, "in", 2) == 0 && in_len + strlen(val) + 2 < OTA_PINMAP_MAX_LEN) {
                // اگر بازر در ورودی‌ها بود رد شود
                if (strstr(val, "buzzer") == NULL && strstr(val, "بازر") == NULL &&
                    strstr(val, "BZ:") == NULL && strstr(val, "BZ :") == NULL) {
                    size_t w = snprintf(raw_inputs + in_len, OTA_PINMAP_MAX_LEN - in_len, "%s\n", val);
                    in_len += w;
                }
            }
        } else if (strcmp(section, "outputs") == 0) {
            if (strncmp(key, "out", 3) == 0 && out_len + strlen(val) + 2 < OTA_PINMAP_MAX_LEN) {
                // حذف قطعی بازر از نقشه اتصالات اپراتوری طبق استاندارد آستیاج
                if (strstr(val, "buzzer") == NULL && strstr(val, "بازر") == NULL &&
                    strstr(val, "BZ:") == NULL && strstr(val, "BZ :") == NULL) {
                    const char *out_line = val;
                    if (strcmp(key, "out0") == 0 &&
                        (strstr(val, "کمپرسور مخزن") != NULL || strstr(val, "کمپرسور کوچک") != NULL || strstr(val, "کمپرسور شیر") != NULL)) {
                        out_line = "• خروجی ۰: نگهدارنده";
                    }
                    size_t w = snprintf(raw_outputs + out_len, OTA_PINMAP_MAX_LEN - out_len, "%s\n", out_line);
                    out_len += w;
                }
            }
        }
    }

    if (raw_inputs[0]) {
        ota_sanitize_server_text(info->pinmap_inputs, sizeof(info->pinmap_inputs), raw_inputs);
    }
    if (raw_outputs[0]) {
        ota_sanitize_server_text(info->pinmap_outputs, sizeof(info->pinmap_outputs), raw_outputs);
    }
    free(raw_inputs);
    free(raw_outputs);
    if (info->ver[0]) {
        info->is_valid = true;
        info->scan_error = false;
    }
}

/* ---------------- Background Scan Worker ---------------- */
static void ota_scan_worker_task(void *param)
{
    (void)param;
    s_scan_busy = true;
    s_scan_done = false;

    /* غیرفعال‌سازی خواب رادیوی وای‌فای (WIFI_PS_NONE) جهت پاسخ‌دهی آنی سوکت‌ها و حذف تاخیر پینگ */
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "Starting fast OTA background metadata scan...");

    for (int i = 0; i < OTA_MODEL_COUNT; i++) {
        if (s_scan_cancel_requested) break;
        char *text = NULL;

        /* در مد عادی اپراتوری (غیر از مد F5 سرویس)، نیازی به بررسی شبکه برای برنامه تست که مخفی است نیست */
        extern bool g_boot_f5_service_mode;
        if (i == OTA_MODEL_CONFIG_TEST && !g_boot_f5_service_mode) {
            continue;
        }

        /* در صورتی که آدرس مدل با مدل اول یکسان باشد، نیازی به اتصال مجدد TLS و دانلود تکراری نیست */
        if (i > 0 && strcmp(s_default_lastfile_urls[i], s_default_lastfile_urls[0]) == 0) {
            if (xSemaphoreTake(s_ota_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                memcpy(&s_model_info[i], &s_model_info[0], sizeof(lastfile_info_t));
                s_scan_revision++;
                xSemaphoreGive(s_ota_mutex);
            }
            continue;
        }

        // ۱. بررسی لینک اصلی با مهلت ۳.۵ ثانیه
        text = http_fetch_text(s_default_lastfile_urls[i], 3500);

        // ۲. در صورت ناموفق بودن، بررسی لینک مستقیم فال‌بک با مهلت ۳.۵ ثانیه
        if (!text && s_fallback_lastfile_urls[i][0]) {
            ESP_LOGW(TAG, "Primary URL failed, trying fallback: %s", s_fallback_lastfile_urls[i][0]);
            text = http_fetch_text(s_fallback_lastfile_urls[i][0], 3500);
        }

        if (xSemaphoreTake(s_ota_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (text && text[0]) {
                ESP_LOGI(TAG, "Received lastfile for model %d (%d bytes)", i, (int)strlen(text));
                parse_lastfile_into_info(text, &s_model_info[i]);
                s_model_info[i].scan_error = false;
            } else {
                ESP_LOGW(TAG, "Could not fetch lastfile for model %d -> using built-in metadata", i);
                s_model_info[i].scan_error = true;
            }
            s_scan_revision++;
            xSemaphoreGive(s_ota_mutex);
        }

        if (text) {
            free(text);
            text = NULL;
        }
    }

    s_scan_busy = false;
    s_scan_done = true;
    s_scan_task_handle = NULL;
    vTaskDelete(NULL);
}

/* ---------------- Background Pre-Erase Subsystem ---------------- */
static volatile bool s_cancel_requested = false;
static volatile bool s_pre_erase_done = false;
static volatile bool s_pre_erase_running = false;
static volatile uint32_t s_erased_upto_offset = 0;
static const esp_partition_t *s_erased_partition = NULL;
static TaskHandle_t s_pre_erase_task_handle = NULL;
static esp_http_client_handle_t s_active_download_client = NULL;

static void ota_pre_erase_worker(void *arg)
{
    (void)arg;
    s_pre_erase_running = true;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition) {
        if (s_erased_partition != update_partition) {
            s_erased_partition = update_partition;
            s_erased_upto_offset = 0;
            s_pre_erase_done = false;
        }
        const uint32_t BLOCK = 65536u;
        uint32_t erase_size = 0x380000u; /* حداقل ۳.۵ مگابایت جهت پوشش قطعی کل باینری فریمور */
        if (s_model_info[0].size_bytes > 0 && s_model_info[0].size_bytes <= update_partition->size) {
            erase_size = (((uint32_t)s_model_info[0].size_bytes + BLOCK - 1u) & ~(BLOCK - 1u));
        }
        if (s_install_total_bytes > 0) {
            erase_size = (((uint32_t)s_install_total_bytes + BLOCK - 1u) & ~(BLOCK - 1u));
        }
        if (erase_size < 0x380000u) {
            erase_size = 0x380000u;
        }
        if (erase_size > update_partition->size) erase_size = update_partition->size;
        ESP_LOGI(TAG, "Background pre-erase starting on partition %s (%"PRIu32" bytes, resume from 0x%"PRIx32")...",
                 update_partition->label, erase_size, s_erased_upto_offset);
        for (uint32_t offset = s_erased_upto_offset; offset < erase_size; offset += BLOCK) {
            if (!s_pre_erase_running) break;
            uint32_t len = ((offset + BLOCK) <= erase_size) ? BLOCK : (erase_size - offset);
            esp_err_t er = esp_partition_erase_range(update_partition, offset, len);
            if (er != ESP_OK) {
                ESP_LOGW(TAG, "Pre-erase error at 0x%"PRIx32": %s", offset, esp_err_to_name(er));
                break;
            }
            s_erased_upto_offset = offset + len;
            /* واگذاری ۱۵ms به هسته ۱ تا LVGL بدون افت حتی یک فریم ۶۰FPS رندر شود */
            vTaskDelay(pdMS_TO_TICKS(15));
        }
        if (s_pre_erase_running && s_erased_upto_offset >= erase_size) {
            s_pre_erase_done = true;
            ESP_LOGI(TAG, "Background pre-erase completed! Target OTA partition is 100%% clean.");
        }
    }
    s_pre_erase_running = false;
    s_pre_erase_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_pre_erase_start(void)
{
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        return;
    }
    if (s_erased_partition != update_partition) {
        s_erased_partition = update_partition;
        s_erased_upto_offset = 0;
        s_pre_erase_done = false;
    }
    if (s_pre_erase_done || s_pre_erase_running || s_install_task_handle != NULL) {
        return;
    }
    xTaskCreatePinnedToCore(ota_pre_erase_worker, "ota_pre_erase", 4096, NULL, 1, &s_pre_erase_task_handle, 0);
}

void ota_pre_erase_stop(void)
{
    if (s_pre_erase_running) {
        s_pre_erase_running = false;
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

bool ota_is_pre_erase_done(void)
{
    return s_pre_erase_done;
}

/* ---------------- Background Download & Flash Worker ---------------- */
#define OTA_STREAM_BUF_MAX 4096 /* بافر یک سکتور ۴۰۹۶ بایتی برای رایت لحظه‌ای ۲ms روی فلش بدون قفل شدن باس SPI و بدون ریزش پکت */
static uint8_t s_ota_sector_buf[OTA_STREAM_BUF_MAX] __attribute__((aligned(4)));

static void ota_download_worker_task(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "Starting OTA native stream download from: %s", s_install_target_url);
    s_install_err_msg[0] = '\0';
    s_cancel_requested = false;
    s_install_pct = 0;
    s_install_dl_bytes = 0;
    s_install_total_bytes = 0;

    /* اگر تسک پیش‌پاک‌سازی پس‌زمینه هنوز فعال است، متوقف شود */
    if (s_pre_erase_running) {
        s_pre_erase_running = false;
        vTaskDelay(pdMS_TO_TICKS(40));
    }

    /* عضویت در Watchdog جهت نظارت بر حلقه دانلود و جلوگیری از خطای task not found */
    esp_task_wdt_add(NULL);

    /* غیرفعال‌سازی خواب رادیوی وای‌فای (WIFI_PS_NONE) و توان حداکثری */
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_max_tx_power(80);
    ESP_LOGI(TAG, "Wi-Fi radio configured to PS_NONE (max power 20dBm) for optimal OTA throughput");

    // 1. Select next passive OTA partition & determine expected size
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "No passive OTA partition found");
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "پارتیشن مقصد ارتقا یافت نشد");
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    /* بررسی تطابق پارتیشن هدف با سابقه پیش‌پاک‌سازی جهت ممانعت از ناهمگامی دو پارتیشن ota_0 و ota_1 */
    if (s_erased_partition != update_partition) {
        ESP_LOGW(TAG, "Target partition changed (%s -> %s), resetting pre-erase offset",
                 s_erased_partition ? s_erased_partition->label : "none", update_partition->label);
        s_erased_partition = update_partition;
        s_erased_upto_offset = 0;
        s_pre_erase_done = false;
    }

    int expected_size = 0;
    if (s_install_model_idx >= 0 && s_install_model_idx < OTA_MODEL_COUNT && s_model_info[s_install_model_idx].size_bytes > 0) {
        expected_size = (int)s_model_info[s_install_model_idx].size_bytes;
    } else if (s_model_info[0].size_bytes > 0) {
        expected_size = (int)s_model_info[0].size_bytes;
    }
    if (expected_size <= 0) {
        expected_size = 3346864; /* پیش‌فرض اندازه فریمور آستیاج ~3.35MB */
    }
    s_install_total_bytes = (uint32_t)expected_size;

    ESP_LOGI(TAG, "Target OTA partition: %s (address 0x%08" PRIx32 ", size %" PRIu32 " B, expected %d B)",
             update_partition->label, update_partition->address, update_partition->size, expected_size);

    // 2. High-speed Instant Flash Init (0 ms delay!)
    s_install_state = OTA_INST_CONNECTING;
    if (s_install_cb) {
        s_install_cb(0, s_install_total_bytes, 0);
    }

    /* تکمیل قطعی پاک‌سازی کل پارتیشن پیش از باز کردن سوکت شبکه (Clean Flash Guarantee) */
    /* نکته حیاتی مهندسی: به هیچ وجه نباید در حین استریم TCP سوکت باز باشد و فلش اریز شود! */
    uint32_t req_erase = (((uint32_t)expected_size + 65535u) & ~65535u);
    if (req_erase < 0x380000u) req_erase = 0x380000u;
    if (req_erase > update_partition->size) req_erase = update_partition->size;

    if (s_erased_upto_offset < req_erase) {
        ESP_LOGI(TAG, "Ensuring complete partition erase up to 0x%"PRIx32" before opening HTTP socket (current: 0x%"PRIx32")...",
                 req_erase, s_erased_upto_offset);
        while (s_erased_upto_offset < req_erase) {
            if (s_cancel_requested) break;
            uint32_t len = ((s_erased_upto_offset + 65536u) <= req_erase) ? 65536u : (req_erase - s_erased_upto_offset);
            esp_err_t er = esp_partition_erase_range(update_partition, s_erased_upto_offset, len);
            if (er != ESP_OK) {
                ESP_LOGE(TAG, "Pre-flash erase failed at 0x%"PRIx32": %s", s_erased_upto_offset, esp_err_to_name(er));
                snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در آماده‌سازی حافظه فلش (%s)", esp_err_to_name(er));
                s_install_state = OTA_INST_FAILED;
                s_install_task_handle = NULL;
                esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
                ota_safely_restore_tasks();
                esp_task_wdt_delete(NULL);
                vTaskDelete(NULL);
                return;
            }
            s_erased_upto_offset += len;
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
    s_pre_erase_done = true;

    if (s_cancel_requested) {
        s_install_state = OTA_INST_IDLE;
        s_install_task_handle = NULL;
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    // Step B: esp_ota_begin with tiny image_size=4096 → ESP-IDF sets need_erase=false
    //         so esp_ota_write never triggers an erase again.
    esp_ota_handle_t ota_handle = 0;
    esp_err_t ota_err = esp_ota_begin(update_partition, 4096, &ota_handle);
    if (ota_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(ota_err));
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در آماده‌سازی پارتیشن فلش (%s)", esp_err_to_name(ota_err));
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "OTA handle initialized. Opening HTTP connection instantly...");

    // 3. NOW open fresh HTTP connection (clean handshake, zero backlog!)
    s_install_state = OTA_INST_CONNECTING;
    if (s_install_cb) {
        s_install_cb(0, s_install_total_bytes, 0);
    }

    bool is_https = (strncmp(s_install_target_url, "https://", 8) == 0);
    esp_http_client_config_t http_config = {
        .url = s_install_target_url,
        .timeout_ms = 4000,
        .buffer_size = 8192,     /* بافر ۸ کیلوبایتی با بهترین بالانس مصرف حافظه و پنجره TCP */
        .buffer_size_tx = 2048,
        .max_redirection_count = 3,
        .keep_alive_enable = false,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = is_https ? esp_crt_bundle_attach : NULL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در ایجاد کلاینت ارتباط با سرور");
        esp_ota_abort(ota_handle);
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }
    s_active_download_client = client;

    esp_http_client_set_header(client, "User-Agent", "ASTIAJ-IceCream-OTA/1.0");
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_header(client, "Connection", "close");

    ESP_LOGI(TAG, "Opening HTTP connection to: %s", http_config.url);
    esp_err_t err = esp_http_client_open(client, 0);
    ESP_LOGI(TAG, "esp_http_client_open returned: %s", esp_err_to_name(err));
    if (err != ESP_OK && is_https) {
        char http_url[256];
        snprintf(http_url, sizeof(http_url), "http://%s", s_install_target_url + 8);
        ESP_LOGW(TAG, "HTTPS failed (%s), attempting plain HTTP fallback: %s", esp_err_to_name(err), http_url);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        s_active_download_client = NULL;

        http_config.url = http_url;
        http_config.crt_bundle_attach = NULL;
        client = esp_http_client_init(&http_config);
        if (client) {
            s_active_download_client = client;
            esp_http_client_set_header(client, "User-Agent", "ASTIAJ-IceCream-OTA/1.0");
            esp_http_client_set_header(client, "Accept", "*/*");
            esp_http_client_set_header(client, "Connection", "keep-alive");
            err = esp_http_client_open(client, 0);
            ESP_LOGI(TAG, "Plain HTTP fallback open returned: %s", esp_err_to_name(err));
        }
    }

    int content_length = -1;
    int status_code = 0;

    if (err == ESP_OK && client) {
        int sock = esp_http_client_get_socket(client);
        if (sock >= 0) {
            int nodelay = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
            int rcvbuf = 65536;
            setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
        }
        content_length = esp_http_client_fetch_headers(client);
        status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "fetch_headers returned status %d, length %d bytes", status_code, content_length);
    }

    if (err != ESP_OK || status_code != 200) {
        ESP_LOGW(TAG, "Primary URL failed (err=%s, status=%d), trying fallback direct S3 endpoint...",
                 esp_err_to_name(err), status_code);
        if (client) {
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            client = NULL;
        }
        s_active_download_client = NULL;

        const char *fallback_url = "http://ota-plc.s3.ir-thr-at1.arvanstorage.ir/other/ice-cream/firmware.bin";
        http_config.url = fallback_url;
        http_config.crt_bundle_attach = NULL;
        client = esp_http_client_init(&http_config);
        if (client) {
            s_active_download_client = client;
            esp_http_client_set_header(client, "User-Agent", "ASTIAJ-IceCream-OTA/1.0");
            esp_http_client_set_header(client, "Accept", "*/*");
            esp_http_client_set_header(client, "Connection", "close");
            err = esp_http_client_open(client, 0);
            if (err == ESP_OK) {
                int fb_sock = esp_http_client_get_socket(client);
                if (fb_sock >= 0) {
                    int nodelay = 1;
                    setsockopt(fb_sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
                    int rcvbuf = 65536;
                    setsockopt(fb_sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
                }
                content_length = esp_http_client_fetch_headers(client);
                status_code = esp_http_client_get_status_code(client);
                ESP_LOGI(TAG, "S3 fallback returned status %d, length %d bytes", status_code, content_length);
            }
        }
    }

    if (status_code != 200) {
        ESP_LOGE(TAG, "Server returned HTTP status %d", status_code);
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "پاسخ نامعتبر سرور (HTTP %d)", status_code);
        if (client) {
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
        }
        esp_ota_abort(ota_handle);
        s_active_download_client = NULL;
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    if (content_length > 0) {
        s_install_total_bytes = (uint32_t)content_length;
    }
    ESP_LOGI(TAG, "HTTP %d OK, len=%d. Starting ultra-fast stream (0 ms flash erase delay!)...", status_code, content_length);

    /* تنظیم تایم‌اوت سوکت استریم به ۱۰ ثانیه جهت تاب‌آوری در نوسانات اینترنت */
    esp_http_client_set_timeout_ms(client, 10000);
    int stream_sock = esp_http_client_get_socket(client);
    if (stream_sock >= 0) {
        int nodelay = 1;
        setsockopt(stream_sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
        int rcvbuf = 32768;
        setsockopt(stream_sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    }

    // 5. Download & Fast Streaming Loop (Flash is clean -> zero erase overhead during writes!)
    s_install_state = OTA_INST_DOWNLOADING;
    if (s_install_cb) {
        s_install_cb(0, s_install_total_bytes, 0);
    }
    int total_read = 0;
    int batch_len = 0;
    bool stream_ok = true;
    int64_t s_last_cb_time_ms = 0;
    uint8_t s_last_cb_pct = 255;
    uint8_t s_last_logged_pct = 255;

    while (1) {
        if (s_cancel_requested) {
            ESP_LOGW(TAG, "OTA Download canceled by user");
            esp_ota_abort(ota_handle);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            s_active_download_client = NULL;
            s_install_state = OTA_INST_IDLE;
            s_install_task_handle = NULL;
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
            ota_safely_restore_tasks();
            esp_task_wdt_delete(NULL);
            vTaskDelete(NULL);
            return;
        }

        if (s_install_total_bytes > 0 && (uint32_t)total_read >= s_install_total_bytes) {
            ESP_LOGI(TAG, "All %u expected bytes already received. Exiting download loop.", (unsigned)total_read);
            break;
        }

        /* محاسبه فضای باقیمانده در بافر تجمیعی سکتور فلش */
        int space = (int)(sizeof(s_ota_sector_buf) - (size_t)batch_len);
        if (s_install_total_bytes > 0 && (uint32_t)(total_read + space) > s_install_total_bytes) {
            space = (int)(s_install_total_bytes - total_read);
            if (space <= 0 && batch_len == 0) break;
        }

        /* بررسی دقیق میزان بایت‌های موجود در صف سوکت TCP جهت ممانعت قطعی از گیر افتادن در حلقه select داخلی IDF */
        int avail = 0;
        int sock = esp_http_client_get_socket(client);
        if (sock >= 0) {
            ioctl(sock, FIONREAD, &avail);
        }

        /* اگر سوکت در این لحظه خالی است، با select مسدود شو تا پکت جدید از وای‌فای برسد (بدون مصرف CPU و بدون سکته) */
        if (avail <= 0 && sock >= 0) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };
            int sel_res = select(sock + 1, &rfds, NULL, NULL, &tv);
            if (sel_res < 0) {
                ESP_LOGE(TAG, "Socket select error: errno=%d", errno);
                snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطای سوکت شبکه حین دانلود");
                stream_ok = false;
                break;
            } else if (sel_res == 0) {
                ESP_LOGE(TAG, "Socket select timed out (10s no data from server)");
                snprintf(s_install_err_msg, sizeof(s_install_err_msg), "تایم‌اوت شبکه (عدم دریافت داده از سرور)");
                stream_ok = false;
                break;
            }
            ioctl(sock, FIONREAD, &avail);
        }

        int to_read = (avail > 0 && avail < space) ? avail : space;
        if (to_read <= 0) to_read = space;

        int64_t t_read_start = esp_timer_get_time();
        int r = (to_read > 0) ? esp_http_client_read(client, (char *)s_ota_sector_buf + batch_len, to_read) : 0;
        int64_t dt_read_ms = (esp_timer_get_time() - t_read_start) / 1000;
        if (dt_read_ms > 2000 && r > 0) {
            ESP_LOGW(TAG, "SLOW READ: esp_http_client_read took %lld ms (r=%d, req=%d, avail=%d) at %u",
                     dt_read_ms, r, to_read, avail, (unsigned)total_read);
        }

        if (r < 0) {
            if (r == -ESP_ERR_HTTP_EAGAIN) {
                esp_task_wdt_reset();
                taskYIELD();
                continue;
            }
            ESP_LOGE(TAG, "HTTP read error: %d", r);
            snprintf(s_install_err_msg, sizeof(s_install_err_msg), "قطع ارتباط با سرور حین دانلود");
            stream_ok = false;
            break;
        } else if (r == 0) {
            if (esp_http_client_is_complete_data_received(client) ||
                (s_install_total_bytes > 0 && (uint32_t)total_read >= s_install_total_bytes)) {
                break;
            }
            ESP_LOGW(TAG, "HTTP read returned 0 before completion (read %u / %u)", (unsigned)total_read, (unsigned)s_install_total_bytes);
            break;
        }

        batch_len += r;
        total_read += r;
        s_install_dl_bytes = (uint32_t)total_read;
        esp_task_wdt_reset();

        /* رایت فوری هر سکتور ۴KB روی فلش: زمان رایت فقط ۲ms است؛ کش هرگز مسدود نشده و پنجره TCP کاملاً باز می‌ماند */
        bool is_stream_end = (s_install_total_bytes > 0 && (uint32_t)total_read >= s_install_total_bytes) ||
                             esp_http_client_is_complete_data_received(client);
        if (batch_len >= (int)sizeof(s_ota_sector_buf) || (is_stream_end && batch_len > 0)) {
            int64_t t_write_start = esp_timer_get_time();
            err = esp_ota_write(ota_handle, s_ota_sector_buf, batch_len);
            int64_t dt_write_ms = (esp_timer_get_time() - t_write_start) / 1000;
            if (dt_write_ms > 30) {
                ESP_LOGW(TAG, "SLOW WRITE: esp_ota_write took %lld ms (size=%d) at %u", dt_write_ms, batch_len, (unsigned)total_read);
            }
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed at offset %d: %s", total_read, esp_err_to_name(err));
                snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در رایت روی حافظه فلش (%s)", esp_err_to_name(err));
                stream_ok = false;
                break;
            }
            batch_len = 0;
            taskYIELD();
        }


        /* بروزرسانی مداوم درصد و کال‌بک رابط کاربری هر ۱۰۰ میلی‌ثانیه برای نرمی حداکثری نمایشگر */
        int64_t now_ms = esp_timer_get_time() / 1000;
        uint8_t pct = 0;
        if (s_install_total_bytes > 0) {
            pct = (uint8_t)(((uint64_t)total_read * 100ULL) / s_install_total_bytes);
            if (pct > 99) pct = 99;
        }
        s_install_pct = pct;
        if (pct != s_last_cb_pct || (now_ms - s_last_cb_time_ms >= 100) || pct >= 99) {
            s_last_cb_pct = pct;
            s_last_cb_time_ms = now_ms;
            if (s_install_cb) {
                s_install_cb((uint32_t)total_read, s_install_total_bytes, pct);
            }
        }

        if ((pct % 5 == 0 && pct != s_last_logged_pct) || pct >= 99) {
            s_last_logged_pct = pct;
            ESP_LOGI(TAG, "OTA Download Progress: %u / %u bytes (%u%%)", (unsigned)total_read, (unsigned)s_install_total_bytes, (unsigned)pct);
        }

        /* واگذاری سریع پردازنده جهت ارسال فوری ACK توسط lwIP */
        taskYIELD();

        if (esp_http_client_is_complete_data_received(client)) {
            ESP_LOGI(TAG, "All %u bytes cleanly received and written to flash.", (unsigned)total_read);
            break;
        }
    }

    /* تخلیه قطعی هرگونه داده باقیمانده در بافر تجمیعی قبل از بستن استریم */
    if (stream_ok && batch_len > 0) {
        err = esp_ota_write(ota_handle, s_ota_sector_buf, batch_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Final esp_ota_write batch flush failed: %s", esp_err_to_name(err));
            snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در ذخیره بخش پایانی فریمور");
            stream_ok = false;
        }
        batch_len = 0;
    }

    // 6. Close network socket FIRST
    // This releases all TCP sockets, lwIP network buffers, and RAM before verification!
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    client = NULL;
    s_active_download_client = NULL;

    if (!stream_ok) {
        esp_ota_abort(ota_handle);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    // Check size completeness
    int target_size = (content_length > 0) ? content_length : expected_size;
    if (target_size > 0 && total_read != target_size) {
        ESP_LOGE(TAG, "Data truncated! Got %d bytes, expected %d bytes", total_read, target_size);
        esp_ota_abort(ota_handle);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "فایل فریمور ناقص دریافت شد (%d/%d بایت)", total_read, target_size);
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }
    if (total_read < 100000) {
        ESP_LOGE(TAG, "Downloaded image suspiciously small: %d bytes", total_read);
        esp_ota_abort(ota_handle);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "حجم فایل دریافتی نامعتبر است (%d بایت)", total_read);
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    // 7. Verify Partition and Activate Boot
    s_install_state = OTA_INST_VERIFYING;
    s_install_pct = 99;
    if (s_install_cb) {
        s_install_cb((uint32_t)total_read, s_install_total_bytes, 99);
    }
    /* واگذاری پردازنده تا هسته ۱ وضعیت "در حال اعتبارسنجی..." را روی نمایشگر رندر کند */
    vTaskDelay(pdMS_TO_TICKS(150));

    ESP_LOGI(TAG, "Validating OTA partition %s (size %d bytes)...", update_partition->label, total_read);

    /* اخذ قفل LVGL جهت ممانعت از تداخل رندرینگ هسته ۱ و غیرفعال‌سازی کش در حین رایت otadata */
    bool locked = lvgl_lock(pdMS_TO_TICKS(1000));

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        if (locked) lvgl_unlock();
        locked = false;
        ESP_LOGE(TAG, "esp_ota_end validation failed (%s [0x%x])", esp_err_to_name(err), err);
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در تایید فایل فریمور (%s)", esp_err_to_name(err));
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        if (locked) lvgl_unlock();
        locked = false;
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s [0x%x])", esp_err_to_name(err), err);
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در فعال‌سازی پارتیشن بوت (%s)", esp_err_to_name(err));
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        s_install_state = OTA_INST_FAILED;
        s_install_task_handle = NULL;
        ota_safely_restore_tasks();
        esp_task_wdt_delete(NULL);
        vTaskDelete(NULL);
        return;
    }

    if (locked) lvgl_unlock();

    ESP_LOGI(TAG, "OTA Download & Verification SUCCESS! Next boot from %s. Rebooting in 1.5s...", update_partition->label);
    s_install_state = OTA_INST_SUCCESS;
    s_install_pct = 100;
    if (s_install_cb && s_install_total_bytes > 0) {
        s_install_cb(s_install_total_bytes, s_install_total_bytes, 100);
    }

    /* ذخیره نشانگر قطعی ارتقا در NVS جهت پیشگیری از تشخیص اشتباه هنگ/کرش و نمایش پاپ‌آپ تبریک */
    const char *target_v = "";
    if (s_install_model_idx >= 0 && s_install_model_idx < OTA_MODEL_COUNT) {
        target_v = s_model_info[s_install_model_idx].ver;
    }
    ui_resume_mark_post_ota(FIRMWARE_VERSION_STR, target_v);
    s_pre_erase_done = false;
    s_erased_upto_offset = 0;
    s_erased_partition = NULL;
    ESP_LOGI(TAG, "Rebooting into new firmware in 1000ms...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    esp_task_wdt_delete(NULL);
    s_install_task_handle = NULL;
    vTaskDelete(NULL);
}

/* ---------------- Public API Implementation ---------------- */
esp_err_t ota_update_init(void)
{
    if (!s_ota_mutex) {
        s_ota_mutex = xSemaphoreCreateMutex();
    }
    init_fallback_metadata();
    s_scan_busy = false;
    s_scan_done = false;
    s_install_state = OTA_INST_IDLE;
    return ESP_OK;
}

void ota_request_scan(void)
{
    if (s_scan_task_handle != NULL || s_scan_busy) {
        return; // Scan already in progress
    }
    if (!s_model_info[0].is_valid) {
        s_scan_done = false;
    }
    s_scan_task_handle = xTaskCreateStaticPinnedToCore(
        ota_scan_worker_task, "ota_scan",
        OTA_SCAN_STACK_BYTES, NULL, 2,
        s_scan_stack, &s_scan_tcb, 0);
    if (!s_scan_task_handle) {
        ESP_LOGE(TAG, "Failed to create ota_scan task");
        s_scan_busy = false;
        s_scan_done = true;
    }
}

bool ota_is_scan_busy(void)
{
    return s_scan_busy;
}

bool ota_is_scan_done(void)
{
    return s_scan_done;
}

uint32_t ota_get_scan_revision(void)
{
    return s_scan_revision;
}

const lastfile_info_t *ota_get_info(int model_idx)
{
    if (model_idx < 0 || model_idx >= OTA_MODEL_COUNT) return &s_model_info[0];
    return &s_model_info[model_idx];
}

const char *ota_get_bin_url(int model_idx)
{
    if (model_idx < 0) {
        if (s_custom_url_active && s_custom_url[0]) {
            return s_custom_url;
        }
        return s_default_bin_urls[0];
    }
    if (model_idx >= OTA_MODEL_COUNT) return s_default_bin_urls[0];
    return s_default_bin_urls[model_idx];
}

const char *ota_get_lastfile_url(int model_idx)
{
    if (model_idx < 0 || model_idx >= OTA_MODEL_COUNT) return s_default_lastfile_urls[0];
    return s_default_lastfile_urls[model_idx];
}

void ota_set_custom_url(const char *url)
{
    if (url && url[0]) {
        snprintf(s_custom_url, sizeof(s_custom_url), "%s", url);
        s_custom_url_active = true;
    } else {
        s_custom_url[0] = '\0';
        s_custom_url_active = false;
    }
}

const char *ota_get_custom_url(void)
{
    return s_custom_url;
}

bool ota_is_custom_active(void)
{
    return s_custom_url_active;
}

#define OTA_TASK_STACK_BYTES 10240 /* 10KB in internal SRAM ensures zero stack overflow during DNS/TLS/HTTP */
static StaticTask_t s_ota_tcb;
static StackType_t s_ota_stack[OTA_TASK_STACK_BYTES] __attribute__((aligned(16)));

static void ota_safely_restore_tasks(void)
{
    extern void actions_resume(void);
    actions_resume();
    extern void cloud_client_resume(void);
    cloud_client_resume();
}

esp_err_t ota_start_install(int model_idx, ota_progress_cb_t cb)
{
    if (s_install_state == OTA_INST_DOWNLOADING || s_install_state == OTA_INST_CONNECTING ||
        s_install_state == OTA_INST_VERIFYING || s_install_task_handle != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const char *url = ota_get_bin_url(model_idx);
    if (!url || !url[0]) return ESP_ERR_INVALID_ARG;

    /* قطع فوری کلیه خروجی‌ها و بارهای مکانیکی پیش از آغاز دانلود جهت ایمنی کامل */
    extern void plc_stop_all(void);
    plc_stop_all();

    /* تعلیق کامل تسک کنترل ماشین (actions_task) جهت ممانعت قطعی از Core Panic و دسترسی به فلش در حین غیرفعال بودن کش */
    extern void actions_pause(void);
    actions_pause();

    /* تعلیق کلاینت ابری در حین دانلود و رایت فلش */
    extern void cloud_client_pause(void);
    cloud_client_pause();

    /* توقف قطعی اسکن کانال‌های وای‌فای جهت تمرکز ۱۰۰٪ رادیو روی سوکت OTA */
    extern void wifi_scan_stop_now(void);
    wifi_scan_stop_now();

    /* متوقف‌سازی تمیز هرگونه اسکن متادیتای سرور در حال اجرا — بدون نشت سوکت یا مسدودسازی */
    if (s_scan_task_handle != NULL) {
        s_scan_cancel_requested = true;
        if (s_active_scan_client) {
            esp_http_client_cancel_request(s_active_scan_client);
        }
        for (int i = 0; i < 40 && s_scan_task_handle != NULL; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (s_scan_task_handle != NULL) {
            bool mutex_safe = (xSemaphoreTake(s_ota_mutex, pdMS_TO_TICKS(50)) == pdTRUE);
            if (mutex_safe) {
                xSemaphoreGive(s_ota_mutex);
            }
            if (eTaskGetState(s_scan_task_handle) != eDeleted) {
                vTaskDelete(s_scan_task_handle);
            }
            if (s_active_scan_client) {
                esp_http_client_close(s_active_scan_client);
                esp_http_client_cleanup(s_active_scan_client);
                s_active_scan_client = NULL;
            }
            s_scan_task_handle = NULL;
            s_scan_busy = false;
        }
    }
    s_scan_cancel_requested = false;

    s_install_model_idx = model_idx;
    snprintf(s_install_target_url, sizeof(s_install_target_url), "%s", url);
    s_install_cb = cb;
    s_install_state = OTA_INST_ERASING;

    s_install_task_handle = xTaskCreateStaticPinnedToCore(
        ota_download_worker_task, "ota_inst",
        OTA_TASK_STACK_BYTES, NULL, 5,
        s_ota_stack, &s_ota_tcb, 0);

    if (!s_install_task_handle) {
        ota_safely_restore_tasks();
        s_install_state = OTA_INST_FAILED;
        snprintf(s_install_err_msg, sizeof(s_install_err_msg), "خطا در ایجاد تسک دانلود فریمور");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ota_cancel_install(void)
{
    if (s_install_task_handle) {
        s_cancel_requested = true;
        if (s_active_download_client) {
            esp_http_client_cancel_request(s_active_download_client);
        }
        for (int i = 0; i < 50 && s_install_task_handle != NULL; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (s_install_task_handle) {
            vTaskDelete(s_install_task_handle);
            s_install_task_handle = NULL;
        }
    }
    s_active_download_client = NULL;
    s_install_state = OTA_INST_IDLE;
    ota_safely_restore_tasks();
}

ota_install_state_t ota_get_install_state(void)
{
    return s_install_state;
}

const char *ota_get_install_error(void)
{
    return s_install_err_msg[0] ? s_install_err_msg : "خطای نامشخص در نصب فریمور";
}

uint8_t ota_get_install_pct(void)
{
    return s_install_pct;
}

uint32_t ota_get_install_downloaded(void)
{
    return s_install_dl_bytes;
}

uint32_t ota_get_install_total(void)
{
    return s_install_total_bytes;
}

bool ota_is_active(void)
{
    return (s_install_state == OTA_INST_CONNECTING ||
            s_install_state == OTA_INST_ERASING ||
            s_install_state == OTA_INST_DOWNLOADING ||
            s_install_state == OTA_INST_VERIFYING);
}

/* ---------------- Asynchronous URL Verification Worker ---------------- */
typedef struct {
    char url[256];
    ota_url_check_cb_t cb;
} ota_check_param_t;

static ota_check_param_t s_ota_check_param;
static volatile bool s_ota_check_busy = false;

static void ota_check_url_worker_task(void *param)
{
    ota_check_param_t *cp = (ota_check_param_t *)param;
    char url[256] = {0};
    ota_url_check_cb_t cb = NULL;
    if (cp) {
        snprintf(url, sizeof(url), "%s", cp->url);
        cb = cp->cb;
    }

    if (!cb) {
        s_ota_check_busy = false;
        vTaskDelete(NULL);
        return;
    }

    if (!wifi_is_connected()) {
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            const char *msg = (ui_lang_get() == UI_LANG_FA) ? "وای‌فای متصل نیست" : ((ui_lang_get() == UI_LANG_TR) ? "Wi-Fi bagli degil" : "Wi-Fi not connected");
            cb(false, 0, 0, msg);
            lvgl_unlock();
        }
        s_ota_check_busy = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Checking OTA URL: %s", url);
    bool is_https = (strncmp(url, "https://", 8) == 0);
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 4500,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = is_https ? esp_crt_bundle_attach : NULL,
        .keep_alive_enable = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    int status_code = 0;
    int64_t clen = -1;

    if (client) {
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK) {
            clen = esp_http_client_fetch_headers(client);
            status_code = esp_http_client_get_status_code(client);
            if (clen <= 0) {
                clen = esp_http_client_get_content_length(client);
            }
            esp_http_client_close(client);
        } else {
            ESP_LOGW(TAG, "esp_http_client_open failed: %s", esp_err_to_name(err));
        }
        esp_http_client_cleanup(client);
    }

    bool is_valid = (status_code == 200 || status_code == 206);
    uint32_t content_length = (clen > 0) ? (uint32_t)clen : 0;
    char err_buf[64] = {0};
    if (!is_valid) {
        if (status_code > 0) {
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(err_buf, sizeof(err_buf), "خطای سرور (%d)", status_code);
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(err_buf, sizeof(err_buf), "Sunucu hatasi (%d)", status_code);
            } else {
                snprintf(err_buf, sizeof(err_buf), "Server error (%d)", status_code);
            }
        } else {
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(err_buf, sizeof(err_buf), "عدم دسترسی به سرور");
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(err_buf, sizeof(err_buf), "Sunucuya ulasilamadi");
            } else {
                snprintf(err_buf, sizeof(err_buf), "Cannot reach server");
            }
        }
    }

    /* ارسال نتیجه به UI — اگر اول تایم‌اوت بخورد دوباره تلاش می‌کنیم تا UI گیر نکند */
    bool locked = lvgl_lock(pdMS_TO_TICKS(150));
    if (!locked) {
        vTaskDelay(pdMS_TO_TICKS(50));
        locked = lvgl_lock(pdMS_TO_TICKS(300));
    }
    if (locked) {
        cb(is_valid, status_code, content_length, err_buf[0] ? err_buf : NULL);
        lvgl_unlock();
    } else {
        ESP_LOGW(TAG, "lvgl_lock timed out in url_check — calling cb without lock to unblock UI");
        cb(is_valid, status_code, content_length, err_buf[0] ? err_buf : NULL);
    }

    s_ota_check_busy = false;
    vTaskDelete(NULL);
}

esp_err_t ota_check_url_async(const char *url, ota_url_check_cb_t cb)
{
    if (!url || !url[0] || !cb) return ESP_ERR_INVALID_ARG;
    if (s_ota_check_busy) return ESP_ERR_INVALID_STATE;

    s_ota_check_busy = true;
    snprintf(s_ota_check_param.url, sizeof(s_ota_check_param.url), "%s", url);
    s_ota_check_param.cb = cb;

    BaseType_t ok = xTaskCreatePinnedToCore(ota_check_url_worker_task, "ota_chk", 10240, &s_ota_check_param, 2, NULL, 0);
    if (ok != pdPASS) {
        s_ota_check_busy = false;
        return ESP_FAIL;
    }
    return ESP_OK;
}
