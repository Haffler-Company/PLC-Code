#include "webportal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "web_page_gz.h"
#include <Update.h>
#include <Preferences.h>
#include "a2_eeprom.h"
#include "day_lock.h"
#include "variables.h"
#include "version.h"
#include "ota_manager.h"
#include "panel_keys.h"

static WebServer server(80);
static DNSServer dnsServer;
static bool s_running = false;
static char s_ip_str[24] = "192.168.4.1";
static char s_ap_ssid[40] = "Haffler-GasFiller";

#include "actions.h"

/* Embedded Web Application (Pre-compressed GZIP in web_page_gz.h - edit web_app.html) */
/* Safe auto-off timer for manual I/O test */
static uint32_t s_test_pin_off_ms = 0;
static uint8_t s_active_test_pin = 0;

static void handle_root(void) {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.setContentLength(PAGE_HTML_GZ_LEN);
    server.send(200, "text/html; charset=UTF-8", "");

    const char *ptr = (const char*)PAGE_HTML_GZ;
    size_t rem = PAGE_HTML_GZ_LEN;
    while (rem > 0 && server.client().connected()) {
        size_t chunk = (rem > 1024) ? 1024 : rem;
        server.sendContent_P(ptr, chunk);
        ptr += chunk;
        rem -= chunk;
        delay(1);
    }
}

static void handle_captive_redirect(void) {
    IPAddress client_ip = server.client().remoteIP();
    if (client_ip[0] == 192 && client_ip[1] == 168 && client_ip[2] == 4) {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.sendHeader("Connection", "close");
        server.send(302, "text/plain", "");
    } else {
        handle_root();
    }
}

static void handle_captive_success(void) {
    server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
}

static void handle_no_content(void) {
    server.send(204);
}

static void handle_not_found(void) {
    String uri = server.uri();
    if (uri == "/index.html" || uri == "/index.htm" || uri == "/") {
        handle_root();
        return;
    }
    if (uri == "/generate_204" || uri == "/gen_204") {
        handle_no_content();
        return;
    }
    if (uri == "/hotspot-detect.html") {
        handle_captive_success();
        return;
    }
    IPAddress client_ip = server.client().remoteIP();
    bool is_ap_client = (client_ip[0] == 192 && client_ip[1] == 168 && client_ip[2] == 4);
    if (is_ap_client) {
        String host = server.hostHeader();
        if (host.length() > 0 && host != "192.168.4.1" && !host.startsWith("192.168.4.1:") && host != "haffler.local") {
            server.sendHeader("Location", "http://192.168.4.1/", true);
            server.sendHeader("Connection", "close");
            server.send(302, "text/plain", "");
            return;
        }
    }
    server.send(404, "text/plain", "Not Found");
}

static void handle_status(void) {
    char json[960];
    uint8_t g1_pct = 0, g2_pct = 0;
    /* Station 1 = Glass 1 (Right): Valve 2 / TimerValve2
       Station 2 = Glass 2 (Left): Valve 1 / TimerValve1 */
    if (TimerValve2_Saved > 0) g1_pct = (uint8_t)(100 - ((TimerValve2 * 100) / TimerValve2_Saved));
    if (TimerValve1_Saved > 0) g2_pct = (uint8_t)(100 - ((TimerValve1 * 100) / TimerValve1_Saved));

    bool buzzer_active = (s_active_test_pin == 45 && millis() <= s_test_pin_off_ms);

    Preferences prefs_cfg;
    prefs_cfg.begin("haffler_cfg", true);
    String cust_name = prefs_cfg.getString("customer_nm", "");
    prefs_cfg.end();

    snprintf(json, sizeof(json),
        "{"
        "\"version\":\"%s\","
        "\"customer_name\":\"%s\","
        "\"g1_time\":%lu,\"g1_saved\":%lu,\"g1_pct\":%u,\"g1_running\":%s,"
        "\"g1_l\":%.1f,\"g1_h\":%.1f,\"g1_d\":%.2f,"
        "\"g2_time\":%lu,\"g2_saved\":%lu,\"g2_pct\":%u,\"g2_running\":%s,"
        "\"g2_l\":%.1f,\"g2_h\":%.1f,\"g2_d\":%.2f,"
        "\"filled_g1\":%lu,\"filled_g2\":%lu,\"total_filled\":%lu,"
        "\"qs\":%.1f,\"p1\":%.1f,\"p2\":%.1f,\"out_type\":%d,\"buzzer\":%s,"
        "\"buzzer_active\":%s,"
        "\"locked\":%s,\"lock_remain\":%d,\"lock_stage\":%u,\"challenge\":%04u,"
        "\"in1\":%s,\"in2\":%s,\"v1\":%s,\"v2\":%s,"
        "\"wifi_connected\":%s,\"wifi_ip\":\"%s\",\"wifi_ssid\":\"%s\","
        "\"free_dram\":%u"
        "}",
        FW_VERSION,
        cust_name.c_str(),
        (unsigned long)TimerValve2, (unsigned long)TimerValve2_Saved, (unsigned)g1_pct, (TimerValve2 > 0 ? "true" : "false"),
        EEprom_Glass1_Length, EEprom_Glass1_Height, EEprom_Glass1_Depth,
        (unsigned long)TimerValve1, (unsigned long)TimerValve1_Saved, (unsigned)g2_pct, (TimerValve1 > 0 ? "true" : "false"),
        EEprom_Glass2_Length, EEprom_Glass2_Height, EEprom_Glass2_Depth,
        (unsigned long)EEprom_TotalFilled_Glass1, (unsigned long)EEprom_TotalFilled_Glass2,
        (unsigned long)(EEprom_TotalFilled_Glass1 + EEprom_TotalFilled_Glass2),
        EEprom_Process_Qs, EEprom_Process_P1, EEprom_Process_P2, (int)EEprom7_OutputType, (EEprom3_Buzzer ? "true" : "false"),
        (buzzer_active ? "true" : "false"),
        (device_locked ? "true" : "false"), day_lock_remain, (unsigned)day_lock_active_stage, (unsigned)dev_lock_challenge,
        (digitalRead(7) == LOW ? "true" : "false"), (digitalRead(3) == LOW ? "true" : "false"),
        ((TimerValve1 > 0 || (s_active_test_pin == 16 && millis() <= s_test_pin_off_ms)) ? "true" : "false"),
        ((TimerValve2 > 0 || (s_active_test_pin == 42 && millis() <= s_test_pin_off_ms)) ? "true" : "false"),
        (WiFi.status() == WL_CONNECTED ? "true" : "false"),
        (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : ""),
        (WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : ""),
        (unsigned)(esp_get_free_internal_heap_size() / 1024)
    );
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json", json);
}

static void handle_start(void) {
    if (device_locked) {
        server.send(403, "application/json", "{\"ok\":false,\"err\":\"device_locked\"}");
        return;
    }
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        int st = 0;
        if (body.indexOf("\"station\":1") >= 0) st = 1;
        calculate_all();
        /* Station 0 = Glass 1 (Right): Valve 2 / TimerValve2
           Station 1 = Glass 2 (Left): Valve 1 / TimerValve1 */
        if (st == 0 && TimerValve2_Saved > 0) {
            TimerValve2 = TimerValve2_Saved;
            ChangeBar1(0);
        } else if (st == 1 && TimerValve1_Saved > 0) {
            TimerValve1 = TimerValve1_Saved;
            ChangeBar(0);
        }
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_stop(void) {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        /* Station 0 = Glass 1 (Right): Valve 2 / TimerValve2
           Station 1 = Glass 2 (Left): Valve 1 / TimerValve1 */
        if (body.indexOf("\"station\":0") >= 0) {
            TimerValve2 = 0;
            HwSetValve2(false);
            ChangeBar1(0);
        } else if (body.indexOf("\"station\":1") >= 0) {
            TimerValve1 = 0;
            HwSetValve1(false);
            ChangeBar(0);
        } else {
            TimerValve1 = 0;
            TimerValve2 = 0;
            HwSetValve1(false);
            HwSetValve2(false);
            ChangeBar(0);
            ChangeBar1(0);
        }
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_settings(void) {
    if (server.hasArg("plain")) {
        String b = server.arg("plain");
        int idx;
        if ((idx = b.indexOf("\"qs\":")) >= 0) EEprom_Process_Qs = b.substring(idx + 5).toFloat();
        if ((idx = b.indexOf("\"p1\":")) >= 0) EEprom_Process_P1 = b.substring(idx + 5).toFloat();
        if ((idx = b.indexOf("\"p2\":")) >= 0) EEprom_Process_P2 = b.substring(idx + 5).toFloat();
        if ((idx = b.indexOf("\"buzzer\":")) >= 0) EEprom3_Buzzer = (b.substring(idx + 9).toInt() != 0);
        if ((idx = b.indexOf("\"g1_l\":")) >= 0) EEprom_Glass1_Length = b.substring(idx + 7).toFloat();
        if ((idx = b.indexOf("\"g1_h\":")) >= 0) EEprom_Glass1_Height = b.substring(idx + 7).toFloat();
        if ((idx = b.indexOf("\"g1_d\":")) >= 0) EEprom_Glass1_Depth = b.substring(idx + 7).toFloat();
        if ((idx = b.indexOf("\"g2_l\":")) >= 0) EEprom_Glass2_Length = b.substring(idx + 7).toFloat();
        if ((idx = b.indexOf("\"g2_h\":")) >= 0) EEprom_Glass2_Height = b.substring(idx + 7).toFloat();
        if ((idx = b.indexOf("\"g2_d\":")) >= 0) EEprom_Glass2_Depth = b.substring(idx + 7).toFloat();
        calculate_all();
        if ((idx = b.indexOf("\"customer_name\":\"")) >= 0) {
            int s = idx + 17;
            int e = b.indexOf("\"", s);
            if (e > s) {
                String name = b.substring(s, e);
                Preferences prefs;
                prefs.begin("haffler_cfg", false);
                prefs.putString("customer_nm", name);
                prefs.end();
            }
        }
        a2_eeprom_write_all();
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_unlock(void) {
    if (server.hasArg("plain")) {
        String b = server.arg("plain");
        int idx = b.indexOf("\"pin\":\"");
        if (idx >= 0) {
            String pin = b.substring(idx + 7);
            int end_idx = pin.indexOf("\"");
            if (end_idx > 0) pin = pin.substring(0, end_idx);
            bool check_only = (b.indexOf("\"check_only\":true") >= 0);
            if (check_only) {
                if (pin == DEFAULT_ADMIN_PIN || pin == MASTER_UNLOCK_PIN) {
                    server.send(200, "application/json", "{\"ok\":true}");
                    return;
                }
            } else {
                if (pin == DEFAULT_ADMIN_PIN) {
                    day_lock_clear_after_unlock();
                    server.send(200, "application/json", "{\"ok\":true}");
                    return;
                }
                if (day_lock_try_unlock(pin.c_str())) {
                    server.send(200, "application/json", "{\"ok\":true}");
                    return;
                }
            }
        }
    }
    server.send(200, "application/json", "{\"ok\":false}");
}

static void handle_daylock_status(void) {
    uint32_t rem_freeze = 0;
    bool is_frozen = day_lock_is_brute_locked(&rem_freeze);
    DayLockState_t st = day_lock_get_state();
    const char *st_str = (st == LOCK_STAGE_FACTORY_FREE) ? "FACTORY_FREE" :
                         (st == LOCK_STAGE_ACTIVE_LEASE)  ? "ACTIVE_LEASE" :
                         (st == LOCK_STAGE_GRACE_10)      ? "GRACE_10" :
                         (st == LOCK_STAGE_SOFT_LOCK)     ? "SOFT_LOCK" :
                         (st == LOCK_STAGE_HARD_LOCK)     ? "HARD_LOCK" :
                         (st == LOCK_STAGE_TAMPER)        ? "TAMPER_LOCK" : "UNKNOWN";
    char json[256];
    snprintf(json, sizeof(json),
        "{\"ok\":true,\"state\":\"%s\",\"stage\":%u,\"remain_days\":%d,\"grace_used\":%s,"
        "\"challenge_code\":%05u,\"failed_attempts\":%u,\"brute_locked\":%s,\"freeze_sec\":%u}",
        st_str, (unsigned)day_lock_active_stage, day_lock_remain,
        (day_lock_grace10_used ? "true" : "false"), (unsigned)dev_lock_challenge,
        (unsigned)day_lock_get_failed_attempts(), (is_frozen ? "true" : "false"), (unsigned)rem_freeze
    );
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json", json);
}

static void handle_daylock_unlock(void) {
    if (server.hasArg("plain")) {
        String b = server.arg("plain");
        int idx = b.indexOf("\"pin\"");
        if (idx >= 0) {
            int colon = b.indexOf(":", idx);
            int q1 = b.indexOf("\"", colon);
            int q2 = b.indexOf("\"", q1 + 1);
            if (q1 > 0 && q2 > q1) {
                String pin = b.substring(q1 + 1, q2);
                if (day_lock_try_unlock(pin.c_str())) {
                    char resp[128];
                    snprintf(resp, sizeof(resp),
                        "{\"ok\":true,\"message\":\"Lease unlocked successfully\",\"stage\":%u,\"remain_days\":%d}",
                        (unsigned)day_lock_active_stage, day_lock_remain);
                    server.send(200, "application/json", resp);
                    return;
                } else {
                    uint32_t rem_sec = 0;
                    day_lock_is_brute_locked(&rem_sec);
                    char resp[128];
                    snprintf(resp, sizeof(resp),
                        "{\"ok\":false,\"err\":\"invalid_pin\",\"failed_attempts\":%u,\"freeze_sec\":%u}",
                        (unsigned)day_lock_get_failed_attempts(), (unsigned)rem_sec);
                    server.send(401, "application/json", resp);
                    return;
                }
            }
        }
    }
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"missing_pin\"}");
}


static void handle_stages(void) {
    if (server.hasArg("plain")) {
        String b = server.arg("plain");
        int idx = b.indexOf("\"stages\":[");
        if (idx >= 0) {
            int s = idx + 10;
            for (int i = 0; i < 5; i++) {
                int comma = b.indexOf(",", s);
                int bracket = b.indexOf("]", s);
                int end = (comma >= 0 && (bracket < 0 || comma < bracket)) ? comma : bracket;
                if (end > s) {
                    int val = b.substring(s, end).toInt();
                    day_lock_set_stage_days(i, val);
                    s = end + 1;
                }
            }
            day_lock_arm_chain();
            day_lock_save();
            server.send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }
    server.send(400, "application/json", "{\"ok\":false}");
}

static void handle_test_io(void) {
    String pin_str = "";
    if (server.hasArg("pin")) {
        pin_str = server.arg("pin");
    } else if (server.hasArg("plain")) {
        String b = server.arg("plain");
        int idx = b.indexOf("\"pin\"");
        if (idx >= 0) {
            int colon = b.indexOf(":", idx);
            if (colon >= 0) {
                int end = b.indexOf(",", colon);
                int end_brace = b.indexOf("}", colon);
                if (end < 0 || (end_brace >= 0 && end_brace < end)) end = end_brace;
                if (end > colon) {
                    pin_str = b.substring(colon + 1, end);
                    pin_str.replace("\"", "");
                    pin_str.trim();
                }
            }
        }
    }
    pin_str.toUpperCase();
    bool is_q0 = (pin_str == "Q0" || pin_str == "16");
    bool is_q1 = (pin_str == "Q1" || pin_str == "42");
    bool is_q2 = (pin_str == "Q2" || pin_str == "45");
    if (is_q0) {
        HwSetValve1(true);
        s_active_test_pin = 16;
        s_test_pin_off_ms = millis() + 3000;
    } else if (is_q1) {
        HwSetValve2(true);
        s_active_test_pin = 42;
        s_test_pin_off_ms = millis() + 3000;
    } else if (is_q2) {
        HwSetBuzzer(true);
        s_active_test_pin = 45;
        s_test_pin_off_ms = millis() + 1000;
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_wifi_scan(void) {
    if (ota_manager_is_scan_running()) {
        server.send(200, "application/json", "[]");
        return;
    }
    char ssids[16][33];
    int n = ota_manager_get_scan_results(ssids, 16);
    if (n <= 0) {
        ota_manager_start_scan_async();
        server.send(200, "application/json", "[]");
        return;
    }
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"";
        json += ssids[i];
        json += "\",\"rssi\":-70}";
    }
    json += "]";
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json", json);
}

static void handle_wifi_connect(void) {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Bad Request");
        return;
    }
    String body = server.arg("plain");
    int s1 = body.indexOf("\"ssid\":\"");
    int p1 = body.indexOf("\"pass\":\"");
    if (s1 >= 0) {
        s1 += 8;
        int s2 = body.indexOf("\"", s1);
        String ssid = body.substring(s1, s2);
        String pass = "";
        if (p1 >= 0) {
            p1 += 8;
            int p2 = body.indexOf("\"", p1);
            pass = body.substring(p1, p2);
        }
        Preferences prefs;
        prefs.begin("haffler_cfg", false);
        prefs.putString("wifi_ssid", ssid);
        prefs.putString("wifi_pass", pass);
        prefs.end();

        ota_manager_save_wifi_credentials(ssid.c_str(), pass.c_str());
        ota_manager_connect_wifi_async(ssid.c_str(), pass.c_str());

        server.send(200, "application/json", "{\"ok\":true,\"status\":\"connecting\"}");
        return;
    }
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"missing_ssid\"}");
}

static void handle_ota_check(void) {
    bool wifi_ok = ota_manager_is_wifi_connected();
    if (wifi_ok && !ota_manager_is_scan_running()) {
        ota_manager_start_check(NULL, NULL);
    }
    ota_status_t st = ota_manager_get_status();
    const char *st_str = (st == OTA_STATUS_IDLE) ? "IDLE" :
                         (st == OTA_STATUS_CHECKING_SERVER) ? "CHECKING" :
                         (st == OTA_STATUS_UPDATE_AVAILABLE) ? "AVAILABLE" :
                         (st == OTA_STATUS_UP_TO_DATE) ? "UP_TO_DATE" :
                         (st == OTA_STATUS_DOWNLOADING) ? "DOWNLOADING" :
                         (st == OTA_STATUS_SUCCESS_REBOOTING) ? "SUCCESS" : "ERROR";

    String json = "{\"ok\":true,\"wifi_connected\":";
    json += wifi_ok ? "true" : "false";
    json += ",\"status\":\"";
    json += st_str;
    json += "\",\"status_msg\":\"";
    json += ota_manager_get_status_message();
    json += "\",\"progress\":";
    json += String(ota_manager_get_percent());
    json += ",\"progs\":[";

    // Prog 0: Gas Filling
    json += "{\"id\":0,\"name\":\"";
    json += ota_info_get_prog_name(0);
    json += "\",\"version\":\"";
    json += ota_info_get_version(0);
    json += "\",\"cur_version\":\"";
    json += FW_VERSION;
    json += "\",\"desc\":\"";
    const char *d0 = ota_info_get_desc_line_lang(0, EEprom2_Language, 0);
    if (d0 && d0[0]) json += d0;
    else json += "برنامه اصلی تزریق گاز آرگون صنعتی";
    json += "\"}";

    if (is_f5_boot_active()) {
        // Prog 1: Diagnostic & Config (Technician boot only)
        json += ",{\"id\":1,\"name\":\"";
        json += ota_info_get_prog_name(1);
        json += "\",\"version\":\"";
        json += ota_info_get_version(1);
        json += "\",\"cur_version\":\"";
        json += FW_VERSION;
        json += "\",\"desc\":\"";
        const char *d1 = ota_info_get_desc_line_lang(1, EEprom2_Language, 0);
        if (d1 && d1[0]) json += d1;
        else json += "بسته آزمایش و عیب یابی کارخانه ای هافلر";
        json += "\"}";
    }

    json += "]}";
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json", json);
}

static void handle_ota_install(void) {
    int prog_idx = 0;
    if (server.hasArg("plain")) {
        String b = server.arg("plain");
        if (b.indexOf("\"prog\":1") >= 0) prog_idx = 1;
    }
    bool started = ota_manager_start_install_prog(prog_idx);
    char resp[96];
    snprintf(resp, sizeof(resp), "{\"ok\":%s,\"prog\":%d}", started ? "true" : "false", prog_idx);
    server.send(200, "application/json", resp);
}

static void handle_export_csv(void) {
    char csv[512];
    snprintf(csv, sizeof(csv),
        "Parameter,Value,Unit\r\n"
        "Machine,Haffler Argon Gas Filler,\r\n"
        "Firmware Version,%s,\r\n"
        "Glass 1 Total Filled,%lu,pcs\r\n"
        "Glass 1 Total Injection Time,%lu,sec\r\n"
        "Glass 2 Total Filled,%lu,pcs\r\n"
        "Glass 2 Total Injection Time,%lu,sec\r\n"
        "Total Production Count,%lu,pcs\r\n"
        "Flow Rate Qs,%.1f,L/min\r\n"
        "Pressure P1,%.1f,bar\r\n"
        "Pressure P2,%.1f,bar\r\n",
        FW_VERSION,
        (unsigned long)EEprom_TotalFilled_Glass1,
        (unsigned long)EEprom_TotalSeconds_Glass1,
        (unsigned long)EEprom_TotalFilled_Glass2,
        (unsigned long)EEprom_TotalSeconds_Glass2,
        (unsigned long)(EEprom_TotalFilled_Glass1 + EEprom_TotalFilled_Glass2),
        EEprom_Process_Qs, EEprom_Process_P1, EEprom_Process_P2
    );
    server.sendHeader("Content-Disposition", "attachment; filename=\"haffler_production_stats.csv\"");
    server.send(200, "text/csv; charset=UTF-8", csv);
}

static void handle_update_finish(void) {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "UPDATE_FAIL" : "UPDATE_OK");
    delay(100);
    ESP.restart();
}

static void handle_update_upload(void) {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("OTA Web Upload Start: %s\n", upload.filename.c_str());
        HwSetValve1(false);
        HwSetValve2(false);
        HwSetBuzzer(false);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("OTA Web Upload Success: %u bytes\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}


/* ============================================================
 * W7 — Daily Stats (31-day rolling in NVS Preferences)
 * Key schema: "ds_d0".."ds_d30" = day totals (uint32_t)
 *             "ds_today_g1", "ds_today_g2" = today counters
 *             "ds_day_idx" = circular index (0..30)
 * ============================================================ */
#define STATS_DAYS 31

void stats_increment(uint32_t g1_delta, uint32_t g2_delta) {
    Preferences prefs;
    prefs.begin("haffler_stats", false);
    uint32_t g1 = prefs.getUInt("ds_today_g1", 0) + g1_delta;
    uint32_t g2 = prefs.getUInt("ds_today_g2", 0) + g2_delta;
    prefs.putUInt("ds_today_g1", g1);
    prefs.putUInt("ds_today_g2", g2);
    prefs.end();
}

/* Call once per day (in main loop when date changes) */
void stats_roll_day(void) {
    Preferences prefs;
    prefs.begin("haffler_stats", false);
    uint8_t idx = (uint8_t)prefs.getUInt("ds_day_idx", 0);
    uint32_t g1 = prefs.getUInt("ds_today_g1", 0);
    uint32_t g2 = prefs.getUInt("ds_today_g2", 0);
    char key[12];
    snprintf(key, sizeof(key), "ds_d%u_g1", (unsigned)idx);
    prefs.putUInt(key, g1);
    snprintf(key, sizeof(key), "ds_d%u_g2", (unsigned)idx);
    prefs.putUInt(key, g2);
    prefs.putUInt("ds_today_g1", 0);
    prefs.putUInt("ds_today_g2", 0);
    prefs.putUInt("ds_day_idx", (idx + 1) % STATS_DAYS);
    prefs.end();
}

static void handle_stats(void) {
    Preferences prefs;
    prefs.begin("haffler_stats", true);
    uint32_t today_g1 = prefs.getUInt("ds_today_g1", 0);
    uint32_t today_g2 = prefs.getUInt("ds_today_g2", 0);
    uint8_t cur_idx   = (uint8_t)prefs.getUInt("ds_day_idx", 0);

    String json = "{";
    json += "\"today_g1\":" + String(today_g1) + ",";
    json += "\"today_g2\":" + String(today_g2) + ",";
    json += "\"alltime\":" + String((unsigned long)(EEprom_TotalFilled_Glass1 + EEprom_TotalFilled_Glass2)) + ",";
    json += "\"daily\":[";

    for (int i = 0; i < STATS_DAYS; i++) {
        uint8_t day_idx = (uint8_t)((cur_idx + i) % STATS_DAYS);
        char key[12];
        snprintf(key, sizeof(key), "ds_d%u_g1", (unsigned)day_idx);
        uint32_t d_g1 = prefs.getUInt(key, 0);
        snprintf(key, sizeof(key), "ds_d%u_g2", (unsigned)day_idx);
        uint32_t d_g2 = prefs.getUInt(key, 0);
        if (i > 0) json += ",";
        json += "{\"total\":" + String(d_g1 + d_g2) + ",\"g1\":" + String(d_g1) + ",\"g2\":" + String(d_g2) + "}";
    }
    json += "]}";
    prefs.end();
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json", json);
}

static void handle_export_stats(void) {
    String mode = "day";
    if (server.hasArg("mode")) mode = server.arg("mode");

    Preferences prefs;
    prefs.begin("haffler_stats", true);
    uint32_t today_g1 = prefs.getUInt("ds_today_g1", 0);
    uint32_t today_g2 = prefs.getUInt("ds_today_g2", 0);
    uint8_t cur_idx   = (uint8_t)prefs.getUInt("ds_day_idx", 0);

    String csv = ""; // UTF-8 BOM for Excel
    csv += "Date Index,Glass 1,Glass 2,Total\r\n";

    if (mode == "day") {
        /* Export today only */
        csv += "Today," + String(today_g1) + "," + String(today_g2) + "," + String(today_g1+today_g2) + "\r\n";
    } else {
        /* Export all 31 days */
        for (int i = 0; i < STATS_DAYS; i++) {
            uint8_t day_idx = (uint8_t)((cur_idx + i) % STATS_DAYS);
            char key[12];
            snprintf(key, sizeof(key), "ds_d%u_g1", (unsigned)day_idx);
            uint32_t d_g1 = prefs.getUInt(key, 0);
            snprintf(key, sizeof(key), "ds_d%u_g2", (unsigned)day_idx);
            uint32_t d_g2 = prefs.getUInt(key, 0);
            csv += "Day " + String(i+1) + "," + String(d_g1) + "," + String(d_g2) + "," + String(d_g1+d_g2) + "\r\n";
        }
        csv += "Today," + String(today_g1) + "," + String(today_g2) + "," + String(today_g1+today_g2) + "\r\n";
        csv += "AllTime Grand Total,,,";
        csv += String((unsigned long)(EEprom_TotalFilled_Glass1 + EEprom_TotalFilled_Glass2)) + "\r\n";
    }
    prefs.end();

    server.sendHeader("Content-Disposition", "attachment; filename=\"haffler_stats_" + mode + ".csv\"");
    server.send(200, "text/csv; charset=UTF-8", csv);
}

static void handle_key(void) {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        int key_id = 0;
        int idx = body.indexOf("\"key\":");
        if (idx >= 0) {
            String val = body.substring(idx + 6);
            val.trim();
            if (val.startsWith("\"")) {
                val = val.substring(1);
                int endQuote = val.indexOf("\"");
                if (endQuote >= 0) val = val.substring(0, endQuote);
                if (val == "F1") key_id = KEY_F1;
                else if (val == "F2") key_id = KEY_F2;
                else if (val == "F3") key_id = KEY_F3;
                else if (val == "F4") key_id = KEY_F4;
                else if (val == "F5") key_id = KEY_F5;
                else if (val == "MODE") key_id = KEY_MODE;
                else if (val == "START") key_id = KEY_START;
                else if (val == "STOP") key_id = KEY_STOP;
                else if (val == "SAVE") key_id = KEY_SAVE;
                else if (val == "CANCEL") key_id = KEY_CANCEL;
            } else {
                key_id = val.toInt();
            }
        }
        if (key_id > 0 && key_id <= (int)KEY_OPTO_STOP) {
            panel_keys_trigger_key((panel_key_id_t)key_id);
        }
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

void webportal_init(void) {
    snprintf(s_ap_ssid, sizeof(s_ap_ssid), "Haffler-GasFiller");
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleep(false); /* Keep radio active to eliminate latency & AP drops */
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(s_ap_ssid, NULL, 1, 0, 4);
    dnsServer.start(53, "*", local_ip);
    if (MDNS.begin("haffler")) {
        MDNS.addService("http", "tcp", 80);
    }

    server.on("/", HTTP_GET, handle_root);
    server.on("/index.html", HTTP_GET, handle_root);
    server.on("/index.htm", HTTP_GET, handle_root);
    server.on("/api/status", HTTP_GET, handle_status);
    server.on("/api/start", HTTP_POST, handle_start);
    server.on("/api/stop", HTTP_POST, handle_stop);
    server.on("/api/settings", HTTP_POST, handle_settings);
    server.on("/api/unlock", HTTP_POST, handle_unlock);
    server.on("/api/daylock/status", HTTP_GET, handle_daylock_status);
    server.on("/api/daylock/unlock", HTTP_POST, handle_daylock_unlock);
    server.on("/api/stages", HTTP_POST, handle_stages);
    server.on("/api/test_io", HTTP_POST, handle_test_io);
    server.on("/api/key", HTTP_POST, handle_key);
    server.on("/api/wifi_scan", HTTP_GET, handle_wifi_scan);
    server.on("/api/wifi_connect", HTTP_POST, handle_wifi_connect);
    server.on("/api/export_csv", HTTP_GET, handle_export_csv);
    server.on("/api/stats", HTTP_GET, handle_stats);
    server.on("/api/export_stats", HTTP_GET, handle_export_stats);
    server.on("/api/ota_check", HTTP_GET, handle_ota_check);
    server.on("/api/ota_install", HTTP_POST, handle_ota_install);

    /* Fast 204 response for browser icon probes (avoids resending 16KB) */
    server.on("/favicon.ico", HTTP_GET, handle_no_content);
    server.on("/apple-touch-icon.png", HTTP_GET, handle_no_content);
    server.on("/apple-touch-icon-precomposed.png", HTTP_GET, handle_no_content);

    /* Captive Portal Probes (Android, iOS, Windows) */
    server.on("/generate_204", HTTP_GET, handle_no_content);
    server.on("/gen_204", HTTP_GET, handle_no_content);
    server.on("/hotspot-detect.html", HTTP_GET, handle_captive_success);
    server.on("/fwlink", handle_captive_redirect);
    server.on("/canonical.html", handle_captive_redirect);
    server.on("/connecttest.txt", handle_captive_redirect);
    server.on("/ncsi.txt", handle_captive_redirect);
    server.onNotFound(handle_not_found);

    /* OTA Web Update */
    server.on("/update", HTTP_POST, handle_update_finish, handle_update_upload);

    server.begin();
    s_running = true;
    Serial.println("WEBPORTAL_INIT OK AP=192.168.4.1");
}

void webportal_poll(void) {
    if (!s_running) return;

    /* Safety auto-off for manual pin test: immediate check every tick */
    if (s_active_test_pin && millis() > s_test_pin_off_ms) {
        if (s_active_test_pin == 16) HwSetValve1(false);
        else if (s_active_test_pin == 42) HwSetValve2(false);
        else if (s_active_test_pin == 45) HwSetBuzzer(false);
        s_active_test_pin = 0;
    }

    /* Daily production stats rollover: check RTC day change once per second */
    static uint32_t s_last_stats_check = 0;
    uint32_t now_ms = millis();
    if (now_ms - s_last_stats_check >= 1000) {
        s_last_stats_check = now_ms;
        static int s_last_stats_day = -1;
        time_t t_now = time(NULL);
        if (t_now > 100000) {
            struct tm tm_info;
            localtime_r(&t_now, &tm_info);
            if (s_last_stats_day != -1 && s_last_stats_day != tm_info.tm_mday) {
                stats_roll_day();
            }
            s_last_stats_day = tm_info.tm_mday;
        }
    }

    /* Process DNS and client requests promptly */
    dnsServer.processNextRequest();
    server.handleClient();
}

bool webportal_is_running(void) {
    return s_running;
}

void webportal_set_enabled(bool enable) {
    if (enable && !s_running) {
        webportal_init();
    } else if (!enable && s_running) {
        server.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        s_running = false;
        Serial.println("WEBPORTAL STOPPED");
    }
}

bool webportal_is_enabled(void) {
    return s_running;
}

const char* webportal_get_ip(void) {
    return s_ip_str;
}

const char* webportal_get_ssid(void) {
    return s_ap_ssid;
}

bool webportal_is_sta_connected(void) {
    return (WiFi.status() == WL_CONNECTED);
}
