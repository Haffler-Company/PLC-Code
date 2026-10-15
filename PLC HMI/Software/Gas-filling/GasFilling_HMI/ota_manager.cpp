#include "ota_manager.h"
#include "version.h"
#include "panel_keys.h"

#include <WiFi.h>
#include <Preferences.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define OTA_SERVER_HOST "http://ota.haffler-infi.ir"
#define OTA_SERVER_HOST_HTTP "http://ota.haffler-infi.ir"

static const char *s_prog_models[OTA_PROG_COUNT][4] = {
    { "upvc/gas-filling", "upvc%2Fgas-filling", "gas-filling", NULL },
    { "upvc/config-test", "upvc%2Fconfig-test", "config-test", NULL }
};

static ota_status_t s_status = OTA_STATUS_IDLE;
static ota_info_t s_info = {0};
static ota_info_t s_prog_info[OTA_PROG_COUNT] = {{0}, {0}};
static volatile bool s_scan_busy = false;
static volatile bool s_scan_done = false;
static ota_ui_progress_fn s_progress_cb = NULL;
static ota_ui_status_fn s_status_cb = NULL;

extern volatile int ui_clock_force_tick;

static TaskHandle_t s_ota_task_handle = NULL;
static TaskHandle_t s_wifi_task_handle = NULL;
static volatile bool s_scan_in_progress = false;
static volatile bool s_cancel_requested = false;
static char s_wifi_ssid[33] = {0};
static char s_wifi_pass[65] = {0};
static char s_used_candidate[64] = "upvc/gas-filling";
static char s_prog_candidates[OTA_PROG_COUNT][64] = { "upvc/gas-filling", "upvc/config-test" };
static int s_target_install_prog = 0;
static char s_custom_ota_url[256] = {0};

static volatile uint32_t s_downloaded_bytes = 0;
static volatile uint32_t s_total_bytes = 0;
static volatile uint8_t s_percent = 0;
static volatile float s_speed_kbps = 0.0f;
static char s_status_message[128] = "آماده";
static volatile bool s_notify_status_pending = false;
static volatile bool s_notify_progress_pending = false;

/* Background Update Checker & Snooze State */
static bool s_update_dismissed_this_boot = false;
static bool s_pending_update_prompt = false;
static char s_pending_update_ver[32] = {0};
static uint32_t s_last_bg_check_ms = 0;
static uint32_t s_last_wifi_reconn_ms = 0;

static void set_status(ota_status_t st, const char *msg)
{
    s_status = st;
    if (msg) {
        strncpy(s_status_message, msg, sizeof(s_status_message) - 1);
        s_status_message[sizeof(s_status_message) - 1] = '\0';
    }
    s_notify_status_pending = true;
    Serial.printf("[OTA] Status: %d - %s\n", (int)st, s_status_message);
}

static bool parse_version_tuple(const char *ver, int *v1, int *v2, int *v3)
{
    if (!ver) return false;
    while (*ver == 'v' || *ver == 'V' || *ver == ' ') ver++;
    *v1 = *v2 = *v3 = 0;
    return sscanf(ver, "%d.%d.%d", v1, v2, v3) >= 2;
}

static bool is_version_newer(const char *remote, const char *local)
{
    int r1 = 0, r2 = 0, r3 = 0;
    int l1 = 0, l2 = 0, l3 = 0;
    if (!parse_version_tuple(remote, &r1, &r2, &r3)) return false;
    if (!parse_version_tuple(local, &l1, &l2, &l3)) return true;

    if (r1 > l1) return true;
    if (r1 < l1) return false;
    if (r2 > l2) return true;
    if (r2 < l2) return false;
    return r3 > l3;
}

static int cfg_utf8_cut(const String &s, int cut)
{
    while (cut > 0 && ((uint8_t)s[cut] & 0xC0) == 0x80) cut--;
    return cut;
}

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

static bool cfg_is_turkish_utf8(const String &s)
{
    int len = s.length();
    for (int i = 0; i < len - 1; i++) {
        uint8_t c1 = (uint8_t)s[i];
        uint8_t c2 = (uint8_t)s[i + 1];
        if ((c1 == 0xC3 && (c2 == 0xA7 || c2 == 0x87 || c2 == 0xB6 || c2 == 0x96 || c2 == 0xBC || c2 == 0x9C)) ||
            (c1 == 0xC4 && (c2 == 0x9F || c2 == 0x9E || c2 == 0xB1 || c2 == 0xB0)) ||
            (c1 == 0xC5 && (c2 == 0x9F || c2 == 0x9E))) {
            return true;
        }
    }
    return false;
}

static bool parse_lastfile_content(const String &body, ota_info_t *out)
{
    if (!out) return false;
    memset(out, 0, sizeof(ota_info_t));
    out->changelog[0] = '\0';
    out->raw_pinmap[0] = '\0';
    out->raw_pinmap_ok = false;

    // Pinmap extraction: from [inputs] up to [install] or end of body
    int inPos = body.indexOf("[inputs]");
    if (inPos < 0) inPos = body.indexOf("[input]");
    if (inPos >= 0) {
        int instPos = body.indexOf("[install]", inPos);
        int endPos = (instPos > inPos) ? instPos : body.length();
        String pinText = body.substring(inPos, endPos);
        pinText.trim();
        if (pinText.length() > 0 && pinText.length() < OTA_PINMAP_MAX_LEN) {
            strncpy(out->raw_pinmap, pinText.c_str(), OTA_PINMAP_MAX_LEN - 1);
            out->raw_pinmap[OTA_PINMAP_MAX_LEN - 1] = '\0';
            out->raw_pinmap_ok = true;
        }
    }

    String section = "";
    String legacy = "";
    bool sawSection = false;
    bool legacyDone = false;
    int len = body.length();
    int from = 0;

    while (from <= len) {
        int nl = body.indexOf('\n', from);
        if (nl < 0) nl = len;
        String line = body.substring(from, nl);
        from = nl + 1;
        int cr = line.indexOf('\r');
        if (cr >= 0) line.remove(cr);
        line.trim();
        if (!line.length() || line[0] == '#') continue;

        if (line[0] == '[' && line[line.length() - 1] == ']') {
            section = line.substring(1, line.length() - 1);
            section.toLowerCase();
            sawSection = true;
            continue;
        }

        int eq = line.indexOf('=');
        if (eq <= 0) {
            if (!sawSection && !legacyDone) { legacy = line; legacyDone = true; }
            continue;
        }

        String key = line.substring(0, eq);
        key.trim();
        key.toLowerCase();
        String val = line.substring(eq + 1);
        val.trim();
        if (!val.length()) continue;

        if (section == "company" || section == "manufacturer") {
            if (key == "name" || key == "company") {
                strncpy(out->company_name, val.c_str(), sizeof(out->company_name) - 1);
            } else if (key == "website" || key == "site" || key == "url") {
                strncpy(out->company_website, val.c_str(), sizeof(out->company_website) - 1);
            }
        } else if (section == "program") {
            if (key == "name") {
                strncpy(out->prog_name, val.c_str(), sizeof(out->prog_name) - 1);
            }
        } else if (section == "version") {
            if (key == "display" || (key == "code" && out->latest_version[0] == '\0')) {
                String vclean = val;
                if (vclean.startsWith("v") || vclean.startsWith("V")) vclean = vclean.substring(1);
                strncpy(out->latest_version, vclean.c_str(), sizeof(out->latest_version) - 1);
            } else if (key == "build_date") {
                strncpy(out->latest_build_date, val.c_str(), sizeof(out->latest_build_date) - 1);
            } else if (key == "build_time") {
                strncpy(out->latest_build_time, val.c_str(), sizeof(out->latest_build_time) - 1);
            }
        } else if (section == "binary") {
            if (key == "sha256") {
                strncpy(out->expected_sha256, val.c_str(), sizeof(out->expected_sha256) - 1);
            } else if (key == "size_bytes" || key == "size") {
                out->file_size = (uint32_t)val.toInt();
            }
        } else if (section.startsWith("desc") || section.startsWith("changelog")) {
            if (val.length() > OTA_DESC_LINE_LEN - 1) {
                val = val.substring(0, cfg_utf8_cut(val, OTA_DESC_LINE_LEN - 1));
            }
            bool isFaSec = (section == "description_fa" || section == "changelog_fa" || section == "description.fa");
            bool isEnSec = (section == "description_en" || section == "changelog_en" || section == "description.en");
            bool isTrSec = (section == "description_tr" || section == "changelog_tr" || section == "description.tr");

            if (isFaSec) {
                if (out->desc_fa_count < OTA_DESC_MAX_LINES) {
                    strncpy(out->desc_fa[out->desc_fa_count++], val.c_str(), OTA_DESC_LINE_LEN - 1);
                }
            } else if (isEnSec) {
                if (out->desc_en_count < OTA_DESC_MAX_LINES) {
                    strncpy(out->desc_en[out->desc_en_count++], val.c_str(), OTA_DESC_LINE_LEN - 1);
                }
            } else if (isTrSec) {
                if (out->desc_tr_count < OTA_DESC_MAX_LINES) {
                    strncpy(out->desc_tr[out->desc_tr_count++], val.c_str(), OTA_DESC_LINE_LEN - 1);
                }
            } else {
                bool isFa = (key.startsWith("fa") || key.indexOf("_fa") >= 0 || cfg_is_persian_utf8(val));
                bool isTr = (key.startsWith("tr") || key.indexOf("_tr") >= 0 || cfg_is_turkish_utf8(val));
                bool isEn = (key.startsWith("en") || key.indexOf("_en") >= 0 || (!isFa && !isTr));

                if (isFa && out->desc_fa_count < OTA_DESC_MAX_LINES) {
                    strncpy(out->desc_fa[out->desc_fa_count++], val.c_str(), OTA_DESC_LINE_LEN - 1);
                }
                if (isTr && out->desc_tr_count < OTA_DESC_MAX_LINES) {
                    strncpy(out->desc_tr[out->desc_tr_count++], val.c_str(), OTA_DESC_LINE_LEN - 1);
                }
                if (isEn && out->desc_en_count < OTA_DESC_MAX_LINES) {
                    strncpy(out->desc_en[out->desc_en_count++], val.c_str(), OTA_DESC_LINE_LEN - 1);
                }

                if (strlen(out->changelog) + val.length() + 4 < sizeof(out->changelog)) {
                    strcat(out->changelog, "- ");
                    strcat(out->changelog, val.c_str());
                    strcat(out->changelog, "\n");
                }
            }
        }
    }

    if (out->latest_version[0] == '\0' && legacy.length() > 0) {
        String vclean = legacy;
        if (vclean.startsWith("v") || vclean.startsWith("V")) vclean = vclean.substring(1);
        strncpy(out->latest_version, vclean.c_str(), sizeof(out->latest_version) - 1);
    }

    out->update_available = is_version_newer(out->latest_version, FW_VERSION);
    return strlen(out->latest_version) > 0;
}

static bool ensure_wifi_connected(uint32_t timeout_ms = 12000)
{
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    /* Wait if a WiFi scan is currently running on the radio (max 2 seconds) */
    uint32_t wait_scan_start = millis();
    while (s_scan_in_progress && (millis() - wait_scan_start < 2000)) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (strlen(s_wifi_ssid) == 0) {
        ota_manager_load_wifi_credentials(s_wifi_ssid, sizeof(s_wifi_ssid), s_wifi_pass, sizeof(s_wifi_pass));
    }

    set_status(OTA_STATUS_CONNECTING_WIFI, "در حال اتصال به شبکه Wi-Fi...");

    /* Clean STA reset without disrupting SoftAP */
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(50));

    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
    }
    WiFi.setSleep(false);

    if (strlen(s_wifi_ssid) > 0) {
        WiFi.begin(s_wifi_ssid, strlen(s_wifi_pass) > 0 ? s_wifi_pass : NULL);
    } else {
        WiFi.begin(); /* Auto-reconnect with NVS saved credentials */
    }

    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        wl_status_t st = WiFi.status();
        if (st == WL_CONNECTED) {
            break;
        }
        if (s_cancel_requested) {
            WiFi.disconnect(false, false);
            set_status(OTA_STATUS_IDLE, "عملیات متوقف شد");
            return false;
        }
        /* Early exit if password failed or network vanished after 10.0 seconds */
        if ((millis() - start > 10000) && (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL)) {
            Serial.printf("[WIFI] Early connection fail status=%d\n", (int)st);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (WiFi.status() == WL_CONNECTED) {
        set_status(OTA_STATUS_WIFI_CONNECTED, "اتصال Wi-Fi برقرار شد");
        if (strlen(s_wifi_ssid) > 0) {
            ota_manager_save_wifi_credentials(s_wifi_ssid, s_wifi_pass);
        }
        /* Industrial NTP Time Sync: Synchronize real-time clock via NTP pool */
        configTime(3.5 * 3600, 0, "pool.ntp.org", "time.google.com");
        return true;
    }

    set_status(OTA_STATUS_ERROR, "خطا در اتصال به Wi-Fi");
    return false;
}

static bool ota_http_get_string(const String &url, String &out_body)
{
    String http_url = url;
    if (http_url.startsWith("https://")) {
        http_url.replace("https://", "http://");
    }

    // 1. Direct lightweight HTTP (Zero TLS heap overhead, ~40ms response)
    {
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setReuse(false);
        http.setConnectTimeout(8000);
        http.setTimeout(12000);

        WiFiClient plainClient;
        plainClient.setTimeout(12);
        plainClient.setNoDelay(true);
        if (http.begin(plainClient, http_url)) {
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            int code = http.GET();
            if (code == 200) {
                out_body = http.getString();
                http.end();
                return true;
            }
            http.end();
        }
    }

    // 2. HTTPS fallback only if explicit https requested and plain HTTP failed
    if (url.startsWith("https://")) {
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setReuse(false);
        http.setConnectTimeout(8000);
        http.setTimeout(12000);

        WiFiClientSecure secClient;
        secClient.setInsecure();
        secClient.setTimeout(12);
        secClient.setNoDelay(true);
        if (http.begin(secClient, url)) {
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            int code = http.GET();
            if (code == 200) {
                out_body = http.getString();
                http.end();
                return true;
            }
            http.end();
        }
    }
    return false;
}

static void ota_check_worker(void *param)
{
    (void)param;
    s_cancel_requested = false;
    s_scan_busy = true;

    if (!ensure_wifi_connected()) {
        s_scan_busy = false;
        s_scan_done = true;
        s_ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    time_t now = time(nullptr);
    if (now < 100000) {
        configTime(3.5 * 3600, 0, "pool.ntp.org", "time.google.com");
        int retry = 0;
        while (time(nullptr) < 100000 && ++retry < 10) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        if (time(nullptr) >= 100000) {
            ui_clock_force_tick++;
        }
    }

    set_status(OTA_STATUS_CHECKING_SERVER, "در حال بررسی سرور بروزرسانی...");

    bool got_any = false;
    for (int p = 0; p < OTA_PROG_COUNT; p++) {
        if (p == 1 && !is_f5_boot_active()) continue;
        bool got_p = false;
        for (int i = 0; s_prog_models[p][i] != NULL && !got_p; i++) {
            if (s_cancel_requested) break;
            String url = String(OTA_SERVER_HOST) + "/" + s_prog_models[p][i] + "/lastfile";
            String body;
            if (ota_http_get_string(url, body)) {
                if (parse_lastfile_content(body, &s_prog_info[p])) {
                    strncpy(s_prog_candidates[p], s_prog_models[p][i], sizeof(s_prog_candidates[p]) - 1);
                    got_p = true;
                    got_any = true;
                    s_prog_info[p].scan_done = true;
                    s_prog_info[p].scan_error = false;
                    Serial.printf("[OTA] Prog %d found at %s, ver=%s\n",
                                  p, url.c_str(), s_prog_info[p].latest_version);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // Fallback: If pinmap not parsed from lastfile, attempt to fetch pinmap.txt
        if (got_p && !s_prog_info[p].raw_pinmap_ok) {
            String pinUrl = String(OTA_SERVER_HOST) + "/" + s_prog_candidates[p] + "/pinmap.txt";
            String pinBody;
            if (ota_http_get_string(pinUrl, pinBody)) {
                pinBody.trim();
                if (pinBody.length() > 0 && pinBody.length() < OTA_PINMAP_MAX_LEN) {
                    strncpy(s_prog_info[p].raw_pinmap, pinBody.c_str(), OTA_PINMAP_MAX_LEN - 1);
                    s_prog_info[p].raw_pinmap[OTA_PINMAP_MAX_LEN - 1] = '\0';
                    s_prog_info[p].raw_pinmap_ok = true;
                    Serial.printf("[OTA] Prog %d fetched external pinmap (%d bytes)\n", p, (int)pinBody.length());
                }
            }
        }
        if (!got_p) {
            s_prog_info[p].scan_error = true;
        }
    }

    s_info = s_prog_info[0];
    strncpy(s_used_candidate, s_prog_candidates[0], sizeof(s_used_candidate) - 1);

    if (!got_any) {
        set_status(OTA_STATUS_ERROR, "عدم پاسخ سرور یا فایل نامعتبر");
    } else if (s_info.update_available) {
        char msg[128];
        snprintf(msg, sizeof(msg), "نسخه جدید %s موجود است", s_info.latest_version);
        set_status(OTA_STATUS_UPDATE_AVAILABLE, msg);
        if (!s_update_dismissed_this_boot && ota_manager_get_snooze_count() < 5) {
            strncpy(s_pending_update_ver, s_info.latest_version, sizeof(s_pending_update_ver) - 1);
            s_pending_update_ver[sizeof(s_pending_update_ver) - 1] = '\0';
            s_pending_update_prompt = true;
        }
    } else {
        set_status(OTA_STATUS_UP_TO_DATE, "دستگاه کاملا بروز است");
    }

    s_scan_busy = false;
    s_scan_done = true;
    s_ota_task_handle = NULL;
    vTaskDelete(NULL);
}

static void ota_download_worker(void *param)
{
    (void)param;
    s_cancel_requested = false;

    if (!ensure_wifi_connected()) {
        s_ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    int p_idx = (s_target_install_prog >= 0 && s_target_install_prog < OTA_PROG_COUNT) ? s_target_install_prog : 0;
    const char *target_cand = s_prog_candidates[p_idx];
    if (!target_cand || target_cand[0] == '\0') {
        target_cand = s_prog_models[p_idx][0];
    }
    String bin_url;
    if (s_target_install_prog == -1 && s_custom_ota_url[0] != '\0') {
        bin_url = String(s_custom_ota_url);
    } else {
        bin_url = String(OTA_SERVER_HOST) + "/" + target_cand + "/firmware.bin";
    }
    set_status(OTA_STATUS_DOWNLOADING, "شروع دانلود میان افزار...");

    bool is_sec = bin_url.startsWith("https://");
    WiFiClient plainClient;
    WiFiClientSecure *secClient = NULL;
    if (is_sec) {
        secClient = new WiFiClientSecure();
        if (secClient) {
            secClient->setInsecure();
            secClient->setTimeout(15);
            secClient->setNoDelay(true);
        }
    } else {
        plainClient.setTimeout(15);
        plainClient.setNoDelay(true);
    }

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setReuse(false);

    bool begun = is_sec ? (secClient ? http.begin(*secClient, bin_url) : false) : http.begin(plainClient, bin_url);
    if (!begun) {
        if (secClient) delete secClient;
        set_status(OTA_STATUS_ERROR, "خطا در اتصال به فایل میان افزار");
        s_ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    http.addHeader("User-Agent", "Haffler-OTA/1.5");
    http.addHeader("Accept", "*/*");
    http.addHeader("Connection", "close");

    int httpCode = http.GET();
    if (httpCode != 200) {
        /* Fallback: try plain HTTP or without .bin */
        http.end();
        if (is_sec) {
            bin_url.replace("https://", "http://");
            if (secClient) {
                delete secClient;
                secClient = NULL;
            }
            plainClient.setTimeout(15);
            plainClient.setNoDelay(true);
            is_sec = false;
            begun = http.begin(plainClient, bin_url);
        } else {
            bin_url = String(OTA_SERVER_HOST) + "/" + target_cand + "/firmware";
            begun = http.begin(plainClient, bin_url);
        }
        if (begun) {
            http.addHeader("User-Agent", "Haffler-OTA/1.5");
            http.addHeader("Accept", "*/*");
            http.addHeader("Connection", "close");
            httpCode = http.GET();
        }
    }

    if (httpCode != 200) {
        http.end();
        if (secClient) {
            delete secClient;
            secClient = NULL;
        }
        set_status(OTA_STATUS_ERROR, "فایل باینری در سرور یافت نشد");
        s_ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    int total = http.getSize();
    if (total <= 0 && s_info.file_size > 0) total = s_info.file_size;
    size_t update_size = (total > 0) ? (size_t)total : UPDATE_SIZE_UNKNOWN;

    if (!Update.begin(update_size, U_FLASH)) {
        http.end();
        set_status(OTA_STATUS_ERROR, "فضای پارتیشن OTA کافی نیست");
        s_ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    size_t BUFSZ = 4096;
    uint8_t *buf = (uint8_t *)malloc(BUFSZ);
    if (!buf) {
        Update.abort();
        http.end();
        set_status(OTA_STATUS_ERROR, "کمبود حافظه RAM برای بافر استریم");
        s_ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0);

    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    uint32_t last_data_ms = millis();
    uint32_t last_speed_ms = millis();
    size_t last_speed_bytes = 0;

    s_downloaded_bytes = 0;
    s_total_bytes = total > 0 ? (uint32_t)total : 0;
    s_percent = 0;
    s_speed_kbps = 0.0f;

    while (http.connected() && (total <= 0 || written < (size_t)total)) {
        if (s_cancel_requested) {
            Update.abort();
            set_status(OTA_STATUS_IDLE, "دانلود توسط کاربر لغو شد");
            break;
        }

        size_t avail = stream->available();
        if (avail) {
            size_t to_read = avail < BUFSZ ? avail : BUFSZ;
            int r = stream->readBytes(buf, to_read);
            if (r > 0) {
                mbedtls_sha256_update(&sha_ctx, buf, r);
                if (Update.write(buf, r) != (size_t)r) {
                    Update.abort();
                    set_status(OTA_STATUS_ERROR, "خطا در رایت روی حافظه فلش");
                    break;
                }
                written += r;
                last_data_ms = millis();

                s_downloaded_bytes = (uint32_t)written;
                uint8_t new_pct = (total > 0) ? (uint8_t)((written * 100) / total) : 0;
                bool pct_changed = (new_pct != s_percent);
                s_percent = new_pct;

                uint32_t now = millis();
                if (pct_changed || (now - last_speed_ms >= 200)) {
                    float elapsed_sec = (float)(now - last_speed_ms) / 1000.0f;
                    if (elapsed_sec > 0.05f) {
                        s_speed_kbps = ((float)(written - last_speed_bytes) / 1024.0f) / elapsed_sec;
                        last_speed_ms = now;
                        last_speed_bytes = written;
                    }
                    s_notify_progress_pending = true;
                }
                vTaskDelay(pdMS_TO_TICKS(2)); /* Yield to LVGL graphics task to eliminate stutter */
            }
        } else {
            if (millis() - last_data_ms > 15000) {
                Update.abort();
                set_status(OTA_STATUS_ERROR, "توقف دریافت داده (Timeout)");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

    free(buf);
    http.end();
    if (secClient) {
        delete secClient;
        secClient = NULL;
    }

    uint8_t sha_out[32];
    mbedtls_sha256_finish(&sha_ctx, sha_out);
    mbedtls_sha256_free(&sha_ctx);

    char actual_sha[65];
    for (int i = 0; i < 32; i++) snprintf(actual_sha + (i * 2), 3, "%02x", sha_out[i]);
    actual_sha[64] = '\0';

    bool sha_match = true;
    if (strlen(s_info.expected_sha256) == 64) {
        sha_match = (strcasecmp(actual_sha, s_info.expected_sha256) == 0);
    }

    if (!s_cancel_requested && (total <= 0 || written == (size_t)total) && !Update.hasError() && sha_match && Update.end(true) && Update.isFinished()) {
        s_percent = 100;
        s_notify_progress_pending = true;
        set_status(OTA_STATUS_SUCCESS_REBOOTING, "بروزرسانی با موفقیت انجام شد. راه اندازی مجدد...");
        vTaskDelay(pdMS_TO_TICKS(1500));
        ESP.restart();
    } else if (!sha_match) {
        Update.abort();
        set_status(OTA_STATUS_ERROR, "خطای عدم تطابق هش SHA-256");
    } else if (Update.hasError() || (total > 0 && written < (size_t)total)) {
        Update.abort();
        set_status(OTA_STATUS_ERROR, "خطا در بارگیری کامل داده ها");
    }

    s_ota_task_handle = NULL;
    vTaskDelete(NULL);
}

#define EEPROM_WIFI_OFFSET 200
#define EEPROM_WIFI_MAGIC0 0x57 // 'W'
#define EEPROM_WIFI_MAGIC1 0x46 // 'F'

void ota_manager_init(void)
{
    s_status = OTA_STATUS_IDLE;
    memset(&s_info, 0, sizeof(s_info));
    ota_manager_load_wifi_credentials(s_wifi_ssid, sizeof(s_wifi_ssid), s_wifi_pass, sizeof(s_wifi_pass));
}

void ota_manager_set_callbacks(ota_ui_progress_fn progress_cb, ota_ui_status_fn status_cb)
{
    s_progress_cb = progress_cb;
    s_status_cb = status_cb;
}

ota_status_t ota_manager_get_status(void)
{
    return s_status;
}

uint8_t ota_manager_get_percent(void)
{
    return s_percent;
}

const char* ota_manager_get_status_message(void)
{
    return s_status_message;
}

const ota_info_t* ota_manager_get_info(void)
{
    return &s_info;
}

const ota_info_t* ota_manager_get_prog_info(int prog_idx)
{
    if (prog_idx < 0 || prog_idx >= OTA_PROG_COUNT) return &s_info;
    return &s_prog_info[prog_idx];
}

const char* ota_info_get_prog_name(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].prog_name[0]) {
        return s_prog_info[prog_idx].prog_name;
    }
    return (prog_idx == 1) ? "Haffler Diagnostic & Config" : "Haffler Double-Glass Argon Filler";
}

const char* ota_info_get_company_name(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].company_name[0]) {
        return s_prog_info[prog_idx].company_name;
    }
    return "Haffler Industrial Automation";
}

const char* ota_info_get_company_website(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].company_website[0]) {
        return s_prog_info[prog_idx].company_website;
    }
    return "www.haffler.ir";
}

const char* ota_info_get_version(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].latest_version[0]) {
        return s_prog_info[prog_idx].latest_version;
    }
    return "";
}

const char* ota_info_get_build_date(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].latest_build_date[0]) {
        return s_prog_info[prog_idx].latest_build_date;
    }
    return "";
}

const char* ota_info_get_build_time(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].latest_build_time[0]) {
        return s_prog_info[prog_idx].latest_build_time;
    }
    return "";
}

const char* ota_info_get_sha256(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].expected_sha256[0]) {
        return s_prog_info[prog_idx].expected_sha256;
    }
    return "";
}

const char* ota_info_get_pinmap_text(int prog_idx)
{
    if (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT && s_prog_info[prog_idx].raw_pinmap[0]) {
        return s_prog_info[prog_idx].raw_pinmap;
    }
    return "";
}

int ota_info_get_desc_count_lang(int prog_idx, int lang)
{
    if (prog_idx < 0 || prog_idx >= OTA_PROG_COUNT) return 0;
    if (lang == 0 && s_prog_info[prog_idx].desc_fa_count > 0) return s_prog_info[prog_idx].desc_fa_count;
    if (lang == 1 && s_prog_info[prog_idx].desc_en_count > 0) return s_prog_info[prog_idx].desc_en_count;
    if (lang == 2 && s_prog_info[prog_idx].desc_tr_count > 0) return s_prog_info[prog_idx].desc_tr_count;
    if (lang == 3 && s_prog_info[prog_idx].desc_en_count > 0) return s_prog_info[prog_idx].desc_en_count;
    if (s_prog_info[prog_idx].desc_fa_count > 0) return s_prog_info[prog_idx].desc_fa_count;
    return 0;
}

const char* ota_info_get_desc_line_lang(int prog_idx, int lang, int line)
{
    if (prog_idx < 0 || prog_idx >= OTA_PROG_COUNT || line < 0) return "";
    if (lang == 0 && s_prog_info[prog_idx].desc_fa_count > 0 && line < s_prog_info[prog_idx].desc_fa_count) {
        return s_prog_info[prog_idx].desc_fa[line];
    }
    if (lang == 1 && s_prog_info[prog_idx].desc_en_count > 0 && line < s_prog_info[prog_idx].desc_en_count) {
        return s_prog_info[prog_idx].desc_en[line];
    }
    if (lang == 2 && s_prog_info[prog_idx].desc_tr_count > 0 && line < s_prog_info[prog_idx].desc_tr_count) {
        return s_prog_info[prog_idx].desc_tr[line];
    }
    if (lang == 3 && s_prog_info[prog_idx].desc_en_count > 0 && line < s_prog_info[prog_idx].desc_en_count) {
        return s_prog_info[prog_idx].desc_en[line];
    }
    if (line < s_prog_info[prog_idx].desc_fa_count) return s_prog_info[prog_idx].desc_fa[line];
    return "";
}

bool ota_info_is_available(int prog_idx)
{
    if (prog_idx < 0 || prog_idx >= OTA_PROG_COUNT) return false;
    return s_prog_info[prog_idx].latest_version[0] != '\0';
}

bool ota_info_is_scan_done(void)
{
    return s_scan_done;
}

bool ota_info_is_scan_busy(void)
{
    return s_scan_busy;
}

void ota_manager_request_rescan(void)
{
    if (!s_scan_busy && ota_manager_is_wifi_connected()) {
        ota_manager_start_check(NULL, NULL);
    }
}

bool ota_manager_start_check(const char *ssid, const char *pass)
{
    if (s_ota_task_handle != NULL) return false;
    if (ssid) strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
    if (pass) strncpy(s_wifi_pass, pass, sizeof(s_wifi_pass) - 1);

    /* GEMINI.md Rule 16: Cap transient task stack to 3072 words to prevent heap exhaustion */
    xTaskCreatePinnedToCore(ota_check_worker, "OtaCheck", 3072, NULL, 1, &s_ota_task_handle, 0);
    return true;
}

bool ota_manager_start_install(void)
{
    return ota_manager_start_install_prog(0);
}

bool ota_manager_start_install_prog(int prog_idx)
{
    if (s_ota_task_handle != NULL) return false;
    s_target_install_prog = (prog_idx >= 0 && prog_idx < OTA_PROG_COUNT) ? prog_idx : 0;
    xTaskCreatePinnedToCore(ota_download_worker, "OtaDownload", 4096, NULL, 2, &s_ota_task_handle, 0);
    return true;
}

bool ota_manager_start_install_url(const char *url)
{
    if (s_ota_task_handle != NULL || !url || !url[0]) return false;
    strncpy(s_custom_ota_url, url, sizeof(s_custom_ota_url) - 1);
    s_custom_ota_url[sizeof(s_custom_ota_url) - 1] = '\0';
    s_target_install_prog = -1;
    xTaskCreatePinnedToCore(ota_download_worker, "OtaDownload", 4096, NULL, 2, &s_ota_task_handle, 0);
    return true;
}

bool ota_manager_is_wifi_connected(void)
{
    return WiFi.status() == WL_CONNECTED;
}

static void ota_wifi_connect_task(void *param)
{
    (void)param;
    ensure_wifi_connected(15000);
    s_wifi_task_handle = NULL;
    vTaskDelete(NULL);
}

bool ota_manager_connect_wifi_async(const char *ssid, const char *pass)
{
    if (s_wifi_task_handle != NULL) return false;
    if (ssid) {
        strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
        s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0';
    }
    if (pass) {
        strncpy(s_wifi_pass, pass, sizeof(s_wifi_pass) - 1);
        s_wifi_pass[sizeof(s_wifi_pass) - 1] = '\0';
    }
    /* GEMINI.md Rule 16: Cap WiFi transient task stack to 3072 words (12KB) to prevent heap exhaustion */
    xTaskCreatePinnedToCore(ota_wifi_connect_task, "WifiConn", 3072, NULL, 1, &s_wifi_task_handle, 0);
    return true;
}

void ota_manager_cancel(void)
{
    s_cancel_requested = true;
}

void ota_manager_poll(void)
{
    if (s_notify_status_pending) {
        s_notify_status_pending = false;
        if (s_status_cb) {
            s_status_cb(s_status, s_status_message);
        }
    }
    if (s_notify_progress_pending) {
        s_notify_progress_pending = false;
        if (s_progress_cb) {
            s_progress_cb(s_downloaded_bytes, s_total_bytes, s_percent, s_speed_kbps);
        }
    }

    uint32_t now = millis();

    /* 30-Second Automatic Reconnect Monitor */
    if (now - s_last_wifi_reconn_ms >= 30000U) {
        s_last_wifi_reconn_ms = now;
        if (strlen(s_wifi_ssid) == 0) {
            ota_manager_load_wifi_credentials(s_wifi_ssid, sizeof(s_wifi_ssid), s_wifi_pass, sizeof(s_wifi_pass));
        }
        if (strlen(s_wifi_ssid) > 0 && WiFi.status() != WL_CONNECTED &&
            s_wifi_task_handle == NULL && s_ota_task_handle == NULL && !s_scan_in_progress) {
            ota_manager_connect_wifi_async(s_wifi_ssid, s_wifi_pass);
        }
    }

    /* 60-Second Background Update Checker (if Wi-Fi connected and not snoozed) */
    if (now - s_last_bg_check_ms >= 60000U) {
        s_last_bg_check_ms = now;
        if (WiFi.status() == WL_CONNECTED &&
            s_wifi_task_handle == NULL && s_ota_task_handle == NULL && !s_scan_in_progress &&
            !s_update_dismissed_this_boot && ota_manager_get_snooze_count() < 5) {
            ota_manager_start_check(NULL, NULL);
        }
    }
}

static void make_nvs_wifi_key(const char *ssid, char *out_key, size_t max_len)
{
    if (!ssid || !out_key || max_len < 8) return;
    /* FNV-1a 16-bit hash of full SSID string */
    uint16_t hash = 0x811c;
    for (const char *p = ssid; *p; p++) {
        hash ^= (uint8_t)(*p);
        hash *= 0x0103;
    }
    /* Clean alphanumeric characters only (replace spaces/symbols with '_') */
    char clean[8] = {0};
    size_t c_idx = 0;
    for (const char *p = ssid; *p && c_idx < 6; p++) {
        char ch = *p;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            clean[c_idx++] = ch;
        }
    }
    clean[c_idx] = '\0';
    if (c_idx == 0) {
        snprintf(clean, sizeof(clean), "net");
    }
    /* Key format: "w_xxxx_clean" (e.g. "w_a1b2_Office"), strictly <= 15 chars */
    snprintf(out_key, max_len, "w_%04x_%s", hash, clean);
}

void ota_manager_save_wifi_credentials(const char *ssid, const char *pass)
{
    if (ssid && strlen(ssid) > 0) {
        strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
        s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0';
    }
    if (pass) {
        strncpy(s_wifi_pass, pass, sizeof(s_wifi_pass) - 1);
        s_wifi_pass[sizeof(s_wifi_pass) - 1] = '\0';
    }
    Preferences prefs;
    bool b_ok = prefs.begin("haffler_cfg", false);
    size_t w1 = 0, w2 = 0;
    if (b_ok) {
        w1 = prefs.putString("wifi_ssid", s_wifi_ssid);
        w2 = prefs.putString("wifi_pass", s_wifi_pass);
        if (ssid && strlen(ssid) > 0) {
            char key[16] = {0};
            make_nvs_wifi_key(ssid, key, sizeof(key));
            prefs.putString(key, s_wifi_pass);
        }
        prefs.end();
    }

    /* Industrial Dual-Redundant Backup: write to pre-allocated EEPROM offset 200..299 */
    if (strlen(s_wifi_ssid) > 0) {
        EEPROM.write(EEPROM_WIFI_OFFSET, EEPROM_WIFI_MAGIC0);
        EEPROM.write(EEPROM_WIFI_OFFSET + 1, EEPROM_WIFI_MAGIC1);
        for (int i = 0; i < 33; i++) {
            char c = (i < (int)strlen(s_wifi_ssid)) ? s_wifi_ssid[i] : 0;
            EEPROM.write(EEPROM_WIFI_OFFSET + 2 + i, (uint8_t)c);
        }
        for (int i = 0; i < 65; i++) {
            char c = (i < (int)strlen(s_wifi_pass)) ? s_wifi_pass[i] : 0;
            EEPROM.write(EEPROM_WIFI_OFFSET + 35 + i, (uint8_t)c);
        }
        EEPROM.commit();
        Serial.println("[EEPROM] Wi-Fi credentials saved to offset 200");
    }
    Serial.printf("[NVS] save_wifi b_ok=%d w1=%u w2=%u ssid='%s'\n", (int)b_ok, (unsigned)w1, (unsigned)w2, s_wifi_ssid);
}

void ota_manager_load_wifi_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len)
{
    Preferences prefs;
    bool b_ok = prefs.begin("haffler_cfg", true);
    String s = "";
    String p = "";
    if (b_ok) {
        s = prefs.getString("wifi_ssid", "");
        p = prefs.getString("wifi_pass", "");
        prefs.end();
    }

    /* Fallback to EEPROM dual-storage if Preferences has no SSID */
    if (s.length() == 0) {
        if (EEPROM.read(EEPROM_WIFI_OFFSET) == EEPROM_WIFI_MAGIC0 &&
            EEPROM.read(EEPROM_WIFI_OFFSET + 1) == EEPROM_WIFI_MAGIC1) {
            char eep_ssid[34] = {0};
            char eep_pass[66] = {0};
            for (int i = 0; i < 33; i++) eep_ssid[i] = (char)EEPROM.read(EEPROM_WIFI_OFFSET + 2 + i);
            for (int i = 0; i < 65; i++) eep_pass[i] = (char)EEPROM.read(EEPROM_WIFI_OFFSET + 35 + i);
            eep_ssid[33] = '\0';
            eep_pass[65] = '\0';
            if (strlen(eep_ssid) > 0) {
                s = String(eep_ssid);
                p = String(eep_pass);
                Serial.printf("[EEPROM] Restored credentials: ssid='%s'\n", eep_ssid);
                /* Sync back to Preferences */
                Preferences p_sync;
                if (p_sync.begin("haffler_cfg", false)) {
                    p_sync.putString("wifi_ssid", s);
                    p_sync.putString("wifi_pass", p);
                    char key[16] = {0};
                    make_nvs_wifi_key(eep_ssid, key, sizeof(key));
                    p_sync.putString(key, p);
                    p_sync.end();
                }
            }
        }
    }

    if (ssid && ssid_len > 0) {
        strncpy(ssid, s.c_str(), ssid_len - 1);
        ssid[ssid_len - 1] = '\0';
    }
    if (pass && pass_len > 0) {
        strncpy(pass, p.c_str(), pass_len - 1);
        pass[pass_len - 1] = '\0';
    }
    Serial.printf("[NVS] load_wifi b_ok=%d ssid='%s'\n", (int)b_ok, s.c_str());
}

bool ota_manager_get_saved_wifi_pass(const char *ssid, char *out_pass, size_t max_len)
{
    if (!ssid || !ssid[0] || !out_pass || max_len == 0) return false;
    out_pass[0] = '\0';
    Preferences prefs;
    if (prefs.begin("haffler_cfg", true)) {
        String stored_ssid = prefs.getString("wifi_ssid", "");
        String stored_pass = prefs.getString("wifi_pass", "");
        if (stored_ssid.length() > 0 && strcmp(stored_ssid.c_str(), ssid) == 0) {
            strncpy(out_pass, stored_pass.c_str(), max_len - 1);
            out_pass[max_len - 1] = '\0';
            prefs.end();
            return strlen(out_pass) > 0;
        }
        char key[16] = {0};
        make_nvs_wifi_key(ssid, key, sizeof(key));
        String p = prefs.getString(key, "");
        /* Also fallback to legacy key if exists */
        if (p.length() == 0) {
            String legacy_key = "w_" + String(ssid).substring(0, 12);
            p = prefs.getString(legacy_key.c_str(), "");
        }
        prefs.end();
        if (p.length() > 0) {
            strncpy(out_pass, p.c_str(), max_len - 1);
            out_pass[max_len - 1] = '\0';
            return true;
        }
    }

    /* Fallback to EEPROM */
    if (EEPROM.read(EEPROM_WIFI_OFFSET) == EEPROM_WIFI_MAGIC0 &&
        EEPROM.read(EEPROM_WIFI_OFFSET + 1) == EEPROM_WIFI_MAGIC1) {
        char eep_ssid[34] = {0};
        char eep_pass[66] = {0};
        for (int i = 0; i < 33; i++) eep_ssid[i] = (char)EEPROM.read(EEPROM_WIFI_OFFSET + 2 + i);
        for (int i = 0; i < 65; i++) eep_pass[i] = (char)EEPROM.read(EEPROM_WIFI_OFFSET + 35 + i);
        eep_ssid[33] = '\0';
        eep_pass[65] = '\0';
        if (strcmp(eep_ssid, ssid) == 0 && strlen(eep_pass) > 0) {
            strncpy(out_pass, eep_pass, max_len - 1);
            out_pass[max_len - 1] = '\0';
            return true;
        }
    }
    return false;
}

int ota_manager_scan_wifi(char out_ssids[][33], int max_count)
{
    int n = WiFi.scanNetworks(false, true);
    if (n < 0) return 0;
    int count = n < max_count ? n : max_count;
    for (int i = 0; i < count; i++) {
        strncpy(out_ssids[i], WiFi.SSID(i).c_str(), 32);
        out_ssids[i][32] = '\0';
    }
    return count;
}

static TaskHandle_t s_wifi_scan_task_handle = NULL;
static char s_scanned_ssids[16][33];
static int s_scanned_count = 0;

static void ota_wifi_scan_worker(void *param)
{
    (void)param;
    s_scan_in_progress = true;
    wifi_mode_t m = WiFi.getMode();
    if (m != WIFI_AP_STA && m != WIFI_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(60);
    }
    WiFi.setSleep(false);
    WiFi.scanDelete();

    // Synchronous scan inside dedicated FreeRTOS worker on Core 0
    // Completely immune to async event-loop drops and scanTimeout bugs
    int16_t n = WiFi.scanNetworks(false, false, false, 250, 0);
    Serial.printf("[WIFI_SCAN_WORKER] Done count=%d\n", (int)n);
    Serial0.printf("[WIFI_SCAN_WORKER] Done count=%d\n", (int)n);
    s_scanned_count = 0;
    if (n > 0) {
        int count = n < 16 ? n : 16;
        for (int i = 0; i < count; i++) {
            String s = WiFi.SSID(i);
            if (s.length() == 0) {
                strncpy(s_scanned_ssids[i], "[Hidden]", 32);
            } else {
                strncpy(s_scanned_ssids[i], s.c_str(), 32);
            }
            s_scanned_ssids[i][32] = '\0';
        }
        s_scanned_count = count;
    }
    WiFi.scanDelete();
    s_scan_in_progress = false;
    s_wifi_scan_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_manager_start_scan_async(void)
{
    if (s_scan_in_progress || s_wifi_scan_task_handle != NULL || s_wifi_task_handle != NULL) return;
    s_scanned_count = 0;
    s_scan_in_progress = true;
    xTaskCreatePinnedToCore(ota_wifi_scan_worker, "WifiScan", 5120, NULL, 1, &s_wifi_scan_task_handle, 0);
}

bool ota_manager_is_scan_running(void)
{
    return s_scan_in_progress;
}

int ota_manager_get_scan_results(char out_ssids[][33], int max_count)
{
    if (s_scan_in_progress) return 0;
    int count = s_scanned_count < max_count ? s_scanned_count : max_count;
    for (int i = 0; i < count; i++) {
        strncpy(out_ssids[i], s_scanned_ssids[i], 32);
        out_ssids[i][32] = '\0';
    }
    return count;
}

void ota_manager_auto_connect(void)
{
    if (strlen(s_wifi_ssid) == 0) {
        ota_manager_load_wifi_credentials(s_wifi_ssid, sizeof(s_wifi_ssid), s_wifi_pass, sizeof(s_wifi_pass));
    }
    if (strlen(s_wifi_ssid) > 0 && WiFi.status() != WL_CONNECTED && s_wifi_task_handle == NULL) {
        if (WiFi.getMode() != WIFI_AP_STA) {
            WiFi.mode(WIFI_AP_STA);
        }
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        ota_manager_connect_wifi_async(s_wifi_ssid, s_wifi_pass);
    }
}

bool ota_manager_sync_ntp_now(void)
{
    if (WiFi.status() != WL_CONNECTED) return false;
    configTime(3.5 * 3600, 0, "pool.ntp.org", "time.google.com", "time.windows.com");
    uint32_t start = millis();
    time_t now = 0;
    while (millis() - start < 3000) {
        now = time(NULL);
        if (now > 1700000000) {
            return true;
        }
        delay(100);
    }
    return false;
}

bool ota_manager_validate_url(const char *url, size_t *out_size_bytes)
{
    if (!url || !url[0]) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    bool is_sec = (strncmp(url, "https://", 8) == 0);
    WiFiClientSecure *secClient = NULL;
    WiFiClient plainClient;
    bool begun = false;

    if (is_sec) {
        secClient = new WiFiClientSecure();
        if (secClient) {
            secClient->setInsecure();
            secClient->setTimeout(10);
            begun = http.begin(*secClient, url);
        }
    } else {
        plainClient.setTimeout(10);
        begun = http.begin(plainClient, url);
    }

    if (!begun) {
        if (secClient) delete secClient;
        return false;
    }

    http.setUserAgent("Haffler-OTA/1.5");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int code = http.sendRequest("HEAD");
    if (code <= 0 || code == 405) {
        code = http.GET();
    }

    bool ok = (code == 200 || code == 206);
    if (ok && out_size_bytes) {
        int sz = http.getSize();
        *out_size_bytes = (sz > 0) ? (size_t)sz : 0;
    }

    http.end();
    if (secClient) delete secClient;
    return ok;
}

uint8_t ota_manager_get_snooze_count(void)
{
    Preferences prefs;
    prefs.begin("haffler_cfg", true);
    uint8_t sn = (uint8_t)prefs.getUChar("ota_snooze", 0);
    prefs.end();
    return sn;
}

void ota_manager_increment_snooze(void)
{
    Preferences prefs;
    prefs.begin("haffler_cfg", false);
    uint8_t sn = (uint8_t)prefs.getUChar("ota_snooze", 0);
    if (sn < 5) sn++;
    prefs.putUChar("ota_snooze", sn);
    prefs.end();
}

void ota_manager_reset_snooze(void)
{
    Preferences prefs;
    prefs.begin("haffler_cfg", false);
    prefs.putUChar("ota_snooze", 0);
    prefs.end();
}

void ota_manager_dismiss_for_boot(void)
{
    s_update_dismissed_this_boot = true;
    s_pending_update_prompt = false;
}

bool ota_manager_is_dismissed_this_boot(void)
{
    return s_update_dismissed_this_boot;
}

bool ota_manager_is_update_prompt_pending(char *out_ver, size_t max_len)
{
    if (!s_pending_update_prompt) return false;
    if (out_ver && max_len > 0) {
        strncpy(out_ver, s_pending_update_ver, max_len - 1);
        out_ver[max_len - 1] = '\0';
    }
    return true;
}

void ota_manager_clear_update_prompt_pending(void)
{
    s_pending_update_prompt = false;
}


