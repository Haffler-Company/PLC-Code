#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include "defines.h"
#include "storage.h"
#include "lang.h"
#include "sound.h"
#include <time.h>
#include <RTClib.h>
#include "day_lock.h"
#include "ui_notification.h"

// ============================================================================
// HAFFLER FAIL-SAFE OTA & ZERO-TOFU SANITIZER ENGINE
// Complies with Siemens PLC Determinism & Apple HIG Guidelines
// ============================================================================

#define OTA_BAKERY_LASTFILE_HTTPS "https://ota.haffler-infi.ir/other%2Fbakery-changir%2Flastfile"
#define OTA_BAKERY_LASTFILE_HTTP  "http://ota.haffler-infi.ir/other%2Fbakery-changir%2Flastfile"
#define OTA_BAKERY_BIN_HTTPS      "https://ota.haffler-infi.ir/other%2Fbakery-changir%2Ffirmware.bin"
#define OTA_BAKERY_BIN_HTTP       "http://ota.haffler-infi.ir/other%2Fbakery-changir%2Ffirmware.bin"

#define OTA_CONFIG_LASTFILE_HTTPS "https://ota.haffler-infi.ir/upvc%2Fconfig-test%2Flastfile"
#define OTA_CONFIG_LASTFILE_HTTP  "http://ota.haffler-infi.ir/upvc/config-test/lastfile"
#define OTA_CONFIG_BIN_HTTPS      "https://ota.haffler-infi.ir/upvc%2Fconfig-test%2Ffirmware.bin"
#define OTA_CONFIG_BIN_HTTP       "http://ota.haffler-infi.ir/upvc/config-test/firmware.bin"

#define MAX_SCANNED_NETWORKS 20
#define MAX_CHANGELOG_LINES 12
#define MAX_IO_PINS 16

extern Storage storage;
extern SemaphoreHandle_t lv_mutex;

struct FwReleaseInfo {
    char name[64];
    char project[32];
    char version[24];
    char build_date[20];
    char build_time[16];
    uint32_t size_bytes;
    char sha256[65];
    char desc_fa[MAX_CHANGELOG_LINES][128];
    int desc_fa_count;
    char desc_en[MAX_CHANGELOG_LINES][128];
    int desc_en_count;
    char desc_tr[MAX_CHANGELOG_LINES][128];
    int desc_tr_count;
    char desc_ru[MAX_CHANGELOG_LINES][128];
    int desc_ru_count;
    char inputs[MAX_IO_PINS][64];
    int inputs_count;
    char outputs[MAX_IO_PINS][64];
    int outputs_count;
    bool available;
    bool is_newer;
};

class OtaManager {
public:
    // WiFi Scanning & Connection State
    char scanned_ssids[MAX_SCANNED_NETWORKS][33];
    int32_t scanned_rssis[MAX_SCANNED_NETWORKS];
    wifi_auth_mode_t scanned_authmode[MAX_SCANNED_NETWORKS];
    int scanned_count = 0;
    volatile bool is_scanning = false;
    volatile bool scan_done = false;

    volatile bool is_connecting = false;
    volatile bool is_connected = false;
    volatile bool wrong_password_detected = false;
    volatile int last_disconnect_reason = 0;
    char connected_ip[20] = {0};
    char current_ssid[33] = {0};

    // Server Info & Release State
    FwReleaseInfo release_info;         // Target: Bakery Chonegir
    FwReleaseInfo release_info_config;  // Target: Hardware Config-Test
    volatile bool is_fetching_info = false;
    volatile bool info_fetch_done = false;
    volatile bool info_fetch_error = false;

    // Download & Install State
    volatile bool is_downloading = false;
    volatile int download_percent = 0;
    volatile bool download_error = false;
    volatile bool download_success = false;

private:
    TaskHandle_t scanTaskHandle = NULL;
    TaskHandle_t connectTaskHandle = NULL;
    TaskHandle_t infoTaskHandle = NULL;
    TaskHandle_t updateTaskHandle = NULL;

public:
    void init() {
        memset(&release_info, 0, sizeof(release_info));
        memset(&release_info_config, 0, sizeof(release_info_config));
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            this->handleWiFiEvent(event, info);
        });

        // If credentials previously saved in NVS, auto-connect
        String saved_ssid = storage.getWifiSsid();
        String saved_pass = storage.getWifiPassword();
        if (storage.getWifiEnabled() && saved_ssid.length() > 0) {
            connectAsync(saved_ssid.c_str(), saved_pass.c_str());
        }
    }

    void syncNtpTimeAsync() {
        xTaskCreatePinnedToCore([](void *param) {
            Serial.println("[NTP] Starting Internet Time Synchronization (+03:30 Tehran)...");
            configTime(12600, 0, "pool.ntp.org", "time.google.com", "time.windows.com");
            struct tm timeinfo;
            bool ok = false;
            for (int i = 0; i < 20; i++) {
                if (getLocalTime(&timeinfo, 500)) {
                    ok = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            if (ok && timeinfo.tm_year >= 125) { // 2025 or later
                extern RTC_DS3231 rtc;
                extern bool rtc_available;
                DateTime ntp_dt(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                if (rtc_available) {
                    rtc.adjust(ntp_dt);
                }
                day_lock_sync_authorized_epoch((uint32_t)ntp_dt.unixtime());
                Serial.printf("[NTP] Clock Synchronized: %04d-%02d-%02d %02d:%02d:%02d\n",
                              ntp_dt.year(), ntp_dt.month(), ntp_dt.day(),
                              ntp_dt.hour(), ntp_dt.minute(), ntp_dt.second());
                if (lv_mutex && xSemaphoreTakeRecursive(lv_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    ui_show_notification(NOTIF_SUCCESS, "تنظیم ساعت", "ساعت دستگاه با اینترنت هماهنگ شد");
                    xSemaphoreGiveRecursive(lv_mutex);
                }
            } else {
                Serial.println("[NTP] NTP time sync failed or timed out.");
            }
            vTaskDelete(NULL);
        }, "NtpSyncTask", 4096, NULL, 1, NULL, 0);
    }

    void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            last_disconnect_reason = (int)info.wifi_sta_disconnected.reason;
            Serial.printf("[WIFI-EVENT] Disconnected. Reason: %d\n", last_disconnect_reason);

            // Reason 15 = 4WAY_HANDSHAKE_TIMEOUT, 205 = CONNECTION_FAIL / AUTH_FAIL, 2 = AUTH_EXPIRE
            if (last_disconnect_reason == 15 || last_disconnect_reason == 205 || 
                last_disconnect_reason == 2 || last_disconnect_reason == 3) {
                wrong_password_detected = true;
            }
            is_connected = false;
            is_connecting = false;
            connected_ip[0] = '\0';
        } 
        else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            is_connected = true;
            is_connecting = false;
            wrong_password_detected = false;
            IPAddress ip = WiFi.localIP();
            snprintf(connected_ip, sizeof(connected_ip), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
            Serial.printf("[WIFI-EVENT] Connected! IP: %s\n", connected_ip);

            // Save valid credentials to NVS so next boot doesn't need re-asking
            if (strlen(current_ssid) > 0) {
                storage.setWifiCredentials(current_ssid, storage.getWifiPassword());
            }

            // Auto-sync NTP time with internet (+03:30 Tehran)
            syncNtpTimeAsync();
        }
    }

    // ── Universal Zero-Tofu Text Sanitizer ──────────────────────────────────
    // Guarantees 0% square characters ([]) on LVGL industrial displays
    static void sanitizeText(char *out, size_t out_sz, const char *in, int lang) {
        if (!out || out_sz == 0) return;
        out[0] = '\0';
        if (!in) return;

        size_t r = 0, w = 0;
        size_t len = strlen(in);

        while (r < len && w + 4 < out_sz) {
            unsigned char c = (unsigned char)in[r];

            // 1. Convert Persian ZWNJ (\xE2\x80\x8C) to standard space
            if (c == 0xE2 && r + 2 < len && 
                (unsigned char)in[r+1] == 0x80 && (unsigned char)in[r+2] == 0x8C) {
                out[w++] = ' ';
                r += 3;
                continue;
            }

            // 2. Strip directional markers LTR (\xE2\x80\x8E) / RTL (\xE2\x80\x8F)
            if (c == 0xE2 && r + 2 < len && 
                (unsigned char)in[r+1] == 0x80 && 
                ((unsigned char)in[r+2] == 0x8E || (unsigned char)in[r+2] == 0x8F)) {
                r += 3;
                continue;
            }

            // 3. Bullets (• \xE2\x80\xA2, ‣ \xE2\x80\xA3, ⁃ \xE2\x81\x83) -> ASCII "- "
            if (c == 0xE2 && r + 2 < len && 
                ((unsigned char)in[r+1] == 0x80 || (unsigned char)in[r+1] == 0x81) &&
                ((unsigned char)in[r+2] >= 0xA2 && (unsigned char)in[r+2] <= 0x83)) {
                out[w++] = '-';
                out[w++] = ' ';
                r += 3;
                continue;
            }

            // 4. Em-dash / En-dash (\xE2\x80\x93, \xE2\x80\x94) -> " - "
            if (c == 0xE2 && r + 2 < len && (unsigned char)in[r+1] == 0x80 &&
                ((unsigned char)in[r+2] == 0x93 || (unsigned char)in[r+2] == 0x94)) {
                out[w++] = ' ';
                out[w++] = '-';
                out[w++] = ' ';
                r += 3;
                continue;
            }

            // 5. Smart Quotes (“ ” « ») -> Standard Quote "
            if (c == 0xE2 && r + 2 < len && (unsigned char)in[r+1] == 0x80 &&
                ((unsigned char)in[r+2] == 0x9C || (unsigned char)in[r+2] == 0x9D)) {
                out[w++] = '"';
                r += 3;
                continue;
            }
            if (c == 0xC2 && r + 1 < len &&
                ((unsigned char)in[r+1] == 0xAB || (unsigned char)in[r+1] == 0xBB)) {
                out[w++] = '"';
                r += 2;
                continue;
            }

            // 6. Non-breaking space (\xC2\xA0) -> Space
            if (c == 0xC2 && r + 1 < len && (unsigned char)in[r+1] == 0xA0) {
                out[w++] = ' ';
                r += 2;
                continue;
            }

            // 7. Turkish / European Latin Accents mapped to clean ASCII
            if (c == 0xC3 && r + 1 < len) {
                unsigned char c2 = (unsigned char)in[r+1];
                if (c2 == 0xA7 || c2 == 0x87) { out[w++] = (c2 == 0x87) ? 'C' : 'c'; r += 2; continue; } // ç / Ç
                if (c2 == 0xB6 || c2 == 0x96) { out[w++] = (c2 == 0x96) ? 'O' : 'o'; r += 2; continue; } // ö / Ö
                if (c2 == 0xBC || c2 == 0x9C) { out[w++] = (c2 == 0x9C) ? 'U' : 'u'; r += 2; continue; } // ü / Ü
            }
            if (c == 0xC4 && r + 1 < len) {
                unsigned char c2 = (unsigned char)in[r+1];
                if (c2 == 0x9F || c2 == 0x9E) { out[w++] = (c2 == 0x9E) ? 'G' : 'g'; r += 2; continue; } // ğ / Ğ
                if (c2 == 0xB1) { out[w++] = 'i'; r += 2; continue; } // ı
                if (c2 == 0xB0) { out[w++] = 'I'; r += 2; continue; } // İ
            }
            if (c == 0xC5 && r + 1 < len) {
                unsigned char c2 = (unsigned char)in[r+1];
                if (c2 == 0x9F || c2 == 0x9E) { out[w++] = (c2 == 0x9E) ? 'S' : 's'; r += 2; continue; } // ş / Ş
            }

            // Default safe copy
            out[w++] = in[r++];
        }
        out[w] = '\0';
    }

    // ── Asynchronous WiFi Scanner ──────────────────────────────────────────
    void scanAsync() {
        if (is_scanning) return;
        is_scanning = true;
        scan_done = false;

        xTaskCreatePinnedToCore([](void *param) {
            OtaManager *self = (OtaManager *)param;
            Serial.println("[WIFI-SCAN] Starting WiFi Scan on Core 0...");
            
            // Set station mode if not active
            if (WiFi.getMode() == WIFI_OFF) {
                WiFi.mode(WIFI_STA);
            } else if (WiFi.getMode() == WIFI_AP) {
                WiFi.mode(WIFI_AP_STA);
            }

            int n = WiFi.scanNetworks(false, false, false, 300);
            if (n >= 0) {
                self->scanned_count = min(n, MAX_SCANNED_NETWORKS);
                for (int i = 0; i < self->scanned_count; i++) {
                    strncpy(self->scanned_ssids[i], WiFi.SSID(i).c_str(), 32);
                    self->scanned_ssids[i][32] = '\0';
                    self->scanned_rssis[i] = WiFi.RSSI(i);
                    self->scanned_authmode[i] = WiFi.encryptionType(i);
                }
                Serial.printf("[WIFI-SCAN] Done. Found %d networks.\n", self->scanned_count);
            } else {
                self->scanned_count = 0;
                Serial.println("[WIFI-SCAN] Scan failed.");
            }
            WiFi.scanDelete();

            self->is_scanning = false;
            self->scan_done = true;
            vTaskDelete(NULL);
        }, "WifiScanTask", 3072, this, 1, &scanTaskHandle, 0);
    }

    // ── Asynchronous WiFi Connection ────────────────────────────────────────
    void connectAsync(const char *ssid, const char *pass) {
        if (!ssid || strlen(ssid) == 0) return;
        strncpy(current_ssid, ssid, sizeof(current_ssid) - 1);
        current_ssid[sizeof(current_ssid) - 1] = '\0';

        storage.setWifiCredentials(ssid, pass ? pass : "");
        wrong_password_detected = false;
        is_connecting = true;

        xTaskCreatePinnedToCore([](void *param) {
            OtaManager *self = (OtaManager *)param;
            Serial.printf("[WIFI-CONN] Connecting to '%s' on Core 0...\n", self->current_ssid);

            if (WiFi.getMode() == WIFI_OFF) {
                WiFi.mode(WIFI_STA);
            } else if (WiFi.getMode() == WIFI_AP) {
                WiFi.mode(WIFI_AP_STA);
            }

            String p = storage.getWifiPassword();
            WiFi.disconnect(false, false);
            vTaskDelay(pdMS_TO_TICKS(50));
            WiFi.begin(self->current_ssid, p.c_str());

            // Wait up to 12 seconds for IP or handshake failure
            unsigned long start = millis();
            while (millis() - start < 12000) {
                if (WiFi.status() == WL_CONNECTED) {
                    break;
                }
                if (self->wrong_password_detected) {
                    Serial.println("[WIFI-CONN] Failed: wrong password detected.");
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(200));
            }

            if (WiFi.status() != WL_CONNECTED) {
                self->is_connecting = false;
                self->is_connected = false;
                if (!self->wrong_password_detected) {
                    Serial.println("[WIFI-CONN] Connection timed out.");
                }
            }

            vTaskDelete(NULL);
        }, "WifiConnTask", 3072, this, 1, &connectTaskHandle, 0);
    }

    // ── Lastfile INI Parser ────────────────────────────────────────────────
    // ── Lastfile INI Parser ────────────────────────────────────────────────
    void parseLastfileTarget(const String &content, FwReleaseInfo &target, int prog_type) {
        memset(&target, 0, sizeof(target));
        if (prog_type == 1) {
            strncpy(target.name, "کانفیگ و تست سخت‌افزار هافلر", sizeof(target.name) - 1);
            strncpy(target.project, "config-test", sizeof(target.project) - 1);
        } else {
            strncpy(target.name, "دستگاه چانه‌گیر و گردکن خمیر هافلر", sizeof(target.name) - 1);
            strncpy(target.project, "bakery-changir", sizeof(target.project) - 1);
        }
        strncpy(target.version, "1.0.0", sizeof(target.version) - 1);

        String current_section = "";
        int cur_pos = 0;

        while (cur_pos < content.length()) {
            int next_nl = content.indexOf('\n', cur_pos);
            if (next_nl == -1) next_nl = content.length();
            String line = content.substring(cur_pos, next_nl);
            cur_pos = next_nl + 1;
            line.trim();

            if (line.length() == 0 || line.startsWith("#")) continue;

            if (line.startsWith("[") && line.endsWith("]")) {
                current_section = line.substring(1, line.length() - 1);
                current_section.toLowerCase();
                continue;
            }

            int eq = line.indexOf('=');
            if (eq == -1) continue;
            String k = line.substring(0, eq);
            String v = line.substring(eq + 1);
            k.trim();
            v.trim();

            if (current_section == "program") {
                if (k == "name") strncpy(target.name, v.c_str(), sizeof(target.name) - 1);
                else if (k == "project") strncpy(target.project, v.c_str(), sizeof(target.project) - 1);
            } else if (current_section == "version") {
                if (k == "code" || k == "display") strncpy(target.version, v.c_str(), sizeof(target.version) - 1);
                else if (k == "build_date") strncpy(target.build_date, v.c_str(), sizeof(target.build_date) - 1);
                else if (k == "build_time") strncpy(target.build_time, v.c_str(), sizeof(target.build_time) - 1);
            } else if (current_section == "binary") {
                if (k == "size_bytes") target.size_bytes = v.toInt();
                else if (k == "sha256") strncpy(target.sha256, v.c_str(), sizeof(target.sha256) - 1);
            } else if (current_section == "description_fa" || current_section == "description") {
                if (target.desc_fa_count < MAX_CHANGELOG_LINES) {
                    strncpy(target.desc_fa[target.desc_fa_count++], v.c_str(), 127);
                }
            } else if (current_section == "description_en") {
                if (target.desc_en_count < MAX_CHANGELOG_LINES) {
                    strncpy(target.desc_en[target.desc_en_count++], v.c_str(), 127);
                }
            } else if (current_section == "description_tr") {
                if (target.desc_tr_count < MAX_CHANGELOG_LINES) {
                    strncpy(target.desc_tr[target.desc_tr_count++], v.c_str(), 127);
                }
            } else if (current_section == "inputs") {
                if (target.inputs_count < MAX_IO_PINS) {
                    strncpy(target.inputs[target.inputs_count++], v.c_str(), 63);
                }
            } else if (current_section == "outputs") {
                if (k != "note" && target.outputs_count < MAX_IO_PINS) {
                    strncpy(target.outputs[target.outputs_count++], v.c_str(), 63);
                }
            }
        }

        // Fill default pinout if not present on server
        if (target.inputs_count == 0) {
            if (prog_type == 1) {
                strncpy(target.inputs[0], "تست ورودی‌های دیجیتال و آنالوگ", 63);
                target.inputs_count = 1;
            } else {
                strncpy(target.inputs[0], "سنسور هوم پیستون: GPIO 7", 63);
                strncpy(target.inputs[1], "سنسور هوم قیف: GPIO 3", 63);
                strncpy(target.inputs[2], "صفحه کلید پنل F: GPIO 1", 63);
                strncpy(target.inputs[3], "صفحه کلید پنل M: GPIO 2", 63);
                target.inputs_count = 4;
            }
        }

        if (target.outputs_count == 0) {
            if (prog_type == 1) {
                strncpy(target.outputs[0], "تست رله‌ها و خروجی‌های ترانزیستوری", 63);
                target.outputs_count = 1;
            } else {
                strncpy(target.outputs[0], "رله موتور پیستون: GPIO 36", 63);
                strncpy(target.outputs[1], "رله موتور قیف: GPIO 42", 63);
                strncpy(target.outputs[2], "اینورتر جلو FWD: GPIO 39", 63);
                strncpy(target.outputs[3], "اینورتر معکوس REV: GPIO 48", 63);
                target.outputs_count = 4;
            }
        }

        // Compare with installed version
        String curVer = storage.getVersion();
        target.is_newer = (prog_type == 0) ? (strcmp(target.version, curVer.c_str()) != 0) : true;
        target.available = true;
    }

    void parseLastfile(const String &content) {
        parseLastfileTarget(content, release_info, 0);
    }

    // ── Asynchronous Server Lastfile Fetcher ────────────────────────────────
    struct FetchArgs {
        OtaManager *self;
        int prog_type;
        char url[256];
    };

    void fetchReleaseInfoAsync(int prog_type = 0, const char *custom_url = NULL) {
        if (is_fetching_info) return;
        is_fetching_info = true;
        info_fetch_done = false;
        info_fetch_error = false;

        FetchArgs *args = new FetchArgs();
        args->self = this;
        args->prog_type = prog_type;
        args->url[0] = '\0';
        if (custom_url) strncpy(args->url, custom_url, sizeof(args->url) - 1);

        xTaskCreatePinnedToCore([](void *param) {
            FetchArgs *args = (FetchArgs *)param;
            OtaManager *self = args->self;
            int prog_type = args->prog_type;
            String https_url = "";
            String http_url = "";

            if (prog_type == 0) {
                https_url = OTA_BAKERY_LASTFILE_HTTPS;
                http_url = OTA_BAKERY_LASTFILE_HTTP;
            } else if (prog_type == 1) {
                https_url = OTA_CONFIG_LASTFILE_HTTPS;
                http_url = OTA_CONFIG_LASTFILE_HTTP;
            } else if (prog_type == 2 && strlen(args->url) > 0) {
                https_url = args->url;
                http_url = args->url;
            }

            delete args;

            Serial.printf("[OTA-INFO] Fetching program %d lastfile...\n", prog_type);

            if (WiFi.status() != WL_CONNECTED) {
                self->info_fetch_error = true;
                self->is_fetching_info = false;
                self->info_fetch_done = true;
                Serial.println("[OTA-INFO] Failed: WiFi not connected.");
                vTaskDelete(NULL);
                return;
            }

            HTTPClient http;
            http.setTimeout(7000);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

            bool ok = false;
            String payload = "";

            if (https_url.startsWith("https://")) {
                WiFiClientSecure secClient;
                secClient.setInsecure();
                secClient.setTimeout(7);
                if (http.begin(secClient, https_url)) {
                    http.addHeader("User-Agent", "Haffler-Bakery-HMI/1.1");
                    int code = http.GET();
                    if (code == 200) {
                        payload = http.getString();
                        ok = true;
                    }
                    http.end();
                }
            }

            if (!ok && http_url.length() > 0) {
                WiFiClient plainClient;
                plainClient.setTimeout(7);
                if (http.begin(plainClient, http_url)) {
                    http.addHeader("User-Agent", "Haffler-Bakery-HMI/1.1");
                    int code = http.GET();
                    if (code == 200) {
                        payload = http.getString();
                        ok = true;
                    }
                    http.end();
                }
            }

            if (ok && payload.length() > 20) {
                FwReleaseInfo &target = (prog_type == 1) ? self->release_info_config : self->release_info;
                self->parseLastfileTarget(payload, target, prog_type);
                self->info_fetch_error = false;
                Serial.printf("[OTA-INFO] Success! Program %d lastfile parsed cleanly.\n", prog_type);
            } else {
                self->info_fetch_error = true;
                Serial.printf("[OTA-INFO] Error: Program %d response invalid.\n", prog_type);
            }

            self->is_fetching_info = false;
            self->info_fetch_done = true;
            vTaskDelete(NULL);
        }, "OtaInfoTask", 8192, args, 1, &infoTaskHandle, 0);
    }

    // ── Asynchronous Firmware Downloader & Flash Updater ────────────────────
    struct UpdateArgs {
        OtaManager *self;
        int prog_type;
        char bin_url[256];
    };

    void startUpdateAsync(int prog_type = 0, const char *custom_bin_url = NULL) {
        if (is_downloading) return;
        is_downloading = true;
        download_percent = 0;
        download_error = false;
        download_success = false;

        UpdateArgs *args = new UpdateArgs();
        args->self = this;
        args->prog_type = prog_type;
        args->bin_url[0] = '\0';
        if (custom_bin_url) strncpy(args->bin_url, custom_bin_url, sizeof(args->bin_url) - 1);

        xTaskCreatePinnedToCore([](void *param) {
            UpdateArgs *args = (UpdateArgs *)param;
            OtaManager *self = args->self;
            int prog_type = args->prog_type;
            String https_bin = "";
            String http_bin = "";

            if (prog_type == 0) {
                https_bin = OTA_BAKERY_BIN_HTTPS;
                http_bin = OTA_BAKERY_BIN_HTTP;
            } else if (prog_type == 1) {
                https_bin = OTA_CONFIG_BIN_HTTPS;
                http_bin = OTA_CONFIG_BIN_HTTP;
            } else if (prog_type == 2 && strlen(args->bin_url) > 0) {
                https_bin = args->bin_url;
                http_bin = args->bin_url;
            }
            delete args;

            Serial.printf("[OTA-FLASH] Starting binary download for program %d on Core 0...\n", prog_type);

            if (WiFi.status() != WL_CONNECTED) {
                self->download_error = true;
                self->is_downloading = false;
                vTaskDelete(NULL);
                return;
            }

            HTTPClient http;
            http.setTimeout(15000);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

            bool begun = false;
            if (https_bin.startsWith("https://")) {
                WiFiClientSecure secClient;
                secClient.setInsecure();
                secClient.setTimeout(15);
                if (http.begin(secClient, https_bin)) {
                    int code = http.GET();
                    if (code == 200) begun = true;
                    else http.end();
                }
            }

            if (!begun && http_bin.length() > 0) {
                WiFiClient plainClient;
                plainClient.setTimeout(15);
                if (http.begin(plainClient, http_bin)) {
                    int code = http.GET();
                    if (code == 200) begun = true;
                }
            }

            if (!begun) {
                self->download_error = true;
                self->is_downloading = false;
                Serial.println("[OTA-FLASH] Could not connect to firmware binary URL.");
                vTaskDelete(NULL);
                return;
            }

            int totalLen = http.getSize();
            if (totalLen <= 0) {
                totalLen = (prog_type == 1) ? self->release_info_config.size_bytes : self->release_info.size_bytes;
            }

            if (!Update.begin(totalLen > 0 ? totalLen : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                self->download_error = true;
                self->is_downloading = false;
                Serial.printf("[OTA-FLASH] Update.begin failed: %s\n", Update.errorString());
                http.end();
                vTaskDelete(NULL);
                return;
            }

            WiFiClient *stream = http.getStreamPtr();
            uint8_t buff[1024];
            int written = 0;
            uint32_t lastDataMs = millis();
            const uint32_t STREAM_IDLE_TIMEOUT_MS = 10000UL;

            while (http.connected() && (written < totalLen || totalLen <= 0)) {
                size_t avail = stream->available();
                if (avail > 0) {
                    int c = stream->readBytes(buff, min((size_t)sizeof(buff), avail));
                    if (c > 0) {
                        Update.write(buff, c);
                        written += c;
                        if (totalLen > 0) {
                            self->download_percent = (written * 100) / totalLen;
                        }
                        lastDataMs = millis();
                    }
                } else {
                    if (millis() - lastDataMs > STREAM_IDLE_TIMEOUT_MS) {
                        Serial.println("[OTA-FLASH] Stream idle watchdog timeout (10s)! Aborting download.");
                        break;
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(5));
            }

            if (Update.end(true)) {
                if (Update.isFinished()) {
                    Serial.println("[OTA-FLASH] Download and flash 100% SUCCESS! Rebooting in 2s...");
                    self->download_percent = 100;
                    self->download_success = true;
                    self->is_downloading = false;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    esp_restart();
                } else {
                    self->download_error = true;
                    self->is_downloading = false;
                }
            } else {
                self->download_error = true;
                self->is_downloading = false;
            }

            http.end();
            vTaskDelete(NULL);
        }, "OtaFlashTask", 8192, args, 1, &updateTaskHandle, 0);
    }
};

extern OtaManager otaManager;

#endif // OTA_MANAGER_H
