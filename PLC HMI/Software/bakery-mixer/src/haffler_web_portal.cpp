#include "haffler_web_portal.h"
#include "machine_fsm.h"
#include "haffler_pins.h"
#include "haffler_day_lock.h"
#include "haffler_webapp_bundle.h"
#include <WiFi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "WEB_PORTAL";
static httpd_handle_t s_server = NULL;

#define MIN_SAFE_DRAM_BYTES 40000

extern bool relay_get(uint8_t idx);

bool web_portal_is_dram_healthy(void) {
    return (esp_get_free_internal_heap_size() >= MIN_SAFE_DRAM_BYTES);
}

static esp_err_t index_get_handler(httpd_req_t *req) {
    if (!web_portal_is_dram_healthy()) {
        ESP_LOGW(TAG, "Low internal DRAM (<40KB)! Refusing heavy HTTP request");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}

// Mobile Captive Portal 204 response
static esp_err_t captive_204_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

// Telemetry GET Endpoint
static esp_err_t telemetry_get_handler(httpd_req_t *req) {
    if (!web_portal_is_dram_healthy()) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    const bakery_live_t& live = g_machine_fsm.getLive();
    const bakery_config_t& cfg = g_machine_fsm.getConfig();

    uint8_t inputs_mask = 0;
    if (digitalRead(PIN_INPUT_ESTOP) == 0) inputs_mask |= (1 << 0);
    if (digitalRead(PIN_INPUT_START) == 0) inputs_mask |= (1 << 1);
    if (digitalRead(PIN_INPUT_GUARD) == 0) inputs_mask |= (1 << 2);
    if (digitalRead(PIN_INPUT_OVERLOAD) == 0) inputs_mask |= (1 << 3);
    if (digitalRead(PIN_INPUT_FLOWMETER) == 0) inputs_mask |= (1 << 4);
    if (live.water_temp_valid) inputs_mask |= (1 << 5);

    uint16_t outputs_mask = 0;
    for (int i = 0; i < 10; ++i) {
        if (relay_get(i)) outputs_mask |= (1 << i);
    }

    char rtc_buf[16];
    snprintf(rtc_buf, sizeof(rtc_buf), "%02u:%02u:%02u", live.rtc_hour, live.rtc_minute, live.rtc_second);

    char shift_info_buf[32] = "شیفت فعال نیست";
    if (live.next_shift_idx >= 0 && live.next_shift_idx < 3) {
        snprintf(shift_info_buf, sizeof(shift_info_buf), "ش%d (%02u:%02u - مانده %uد)",
                 live.next_shift_idx + 1,
                 cfg.shifts[live.next_shift_idx].hour,
                 cfg.shifts[live.next_shift_idx].minute,
                 live.minutes_until_next_shift);
    }

    const char* phase_step = "آماده به کار";
    switch (g_machine_fsm.getState()) {
        case STATE_PRE_TEMP_REGULATION: phase_step = "پیش‌تنظیم دمای آب"; break;
        case STATE_FLOUR_FEEDING:       phase_step = "تزریق آرد ماردون"; break;
        case STATE_WATER_DOSING:        phase_step = "تزریق دوزینگ آب"; break;
        case STATE_MIXING:              phase_step = "میکس خمیر + ترمز کاسه"; break;
        case STATE_CYCLE_COMPLETE:      phase_step = "اتمام فرآیند میکس"; break;
        case STATE_PAUSED_GUARD:        phase_step = "توقف: توری محافظ باز است"; break;
        case STATE_FAULT:               phase_step = "خطا: بی‌متال یا قفل منقضی"; break;
        case STATE_ESTOP:               phase_step = "توقف اضطراری قارچی"; break;
        default:                        phase_step = "آماده به کار"; break;
    }

    uint16_t challenge = g_machine_fsm.getDayLockChallenge();

    char resp[512];
    snprintf(resp, sizeof(resp),
        "{\"hero\":%lu,\"state\":\"%s\",\"rtc\":\"%s\",\"temp\":%.1f,\"temp_target\":%.1f,\"temp_ready\":%s,"
        "\"water_cur\":%.1f,\"water_target\":%.1f,\"flowrate\":%.1f,\"flour_mode\":%u,\"flour_sec\":%lu,"
        "\"brake_active\":%s,\"batches\":%lu,\"shift_info\":\"%s\",\"phase_step\":\"%s\","
        "\"dl_stage\":%u,\"dl_days\":%u,\"dl_challenge\":%u,\"dl_locked\":%s,"
        "\"inputs\":%u,\"outputs\":%u}",
        (unsigned long)live.remaining_mix_sec,
        g_machine_fsm.getStateName(),
        rtc_buf,
        live.water_temp_c,
        cfg.water_target_temp_c,
        live.water_temp_ready ? "true" : "false",
        live.current_water_dosed_l,
        cfg.water_target_liters,
        cfg.flow_rate_l_per_min,
        cfg.flour_mode,
        (unsigned long)cfg.flour_feed_time_sec,
        live.bowl_brake_active ? "true" : "false",
        (unsigned long)live.batches_count,
        shift_info_buf,
        phase_step,
        live.daylock_stage,
        live.daylock_days_left,
        challenge,
        live.daylock_locked ? "true" : "false",
        inputs_mask,
        outputs_mask
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

// Control Command Endpoint
static esp_err_t control_handler(httpd_req_t *req) {
    char query[256];
    bool ok = false;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char cmd[32] = {0};
        if (httpd_query_key_value(query, "cmd", cmd, sizeof(cmd)) == ESP_OK) {
            if (strcmp(cmd, "START") == 0) {
                g_machine_fsm.startCycle();
                ok = true;
            } else if (strcmp(cmd, "STOP") == 0) {
                g_machine_fsm.stopCycle();
                ok = true;
            } else if (strcmp(cmd, "OVERRIDE") == 0) {
                g_machine_fsm.overrideTempWait();
                ok = true;
            } else if (strcmp(cmd, "SET_TIME") == 0) {
                char y_str[8] = {0}, m_str[8] = {0}, d_str[8] = {0};
                char h_str[8] = {0}, min_str[8] = {0}, s_str[8] = {0};
                httpd_query_key_value(query, "year", y_str, sizeof(y_str));
                httpd_query_key_value(query, "month", m_str, sizeof(m_str));
                httpd_query_key_value(query, "day", d_str, sizeof(d_str));
                httpd_query_key_value(query, "hour", h_str, sizeof(h_str));
                httpd_query_key_value(query, "minute", min_str, sizeof(min_str));
                httpd_query_key_value(query, "second", s_str, sizeof(s_str));

                uint16_t y = y_str[0] ? (uint16_t)atoi(y_str) : 2026;
                uint8_t mo = m_str[0] ? (uint8_t)atoi(m_str) : 9;
                uint8_t da = d_str[0] ? (uint8_t)atoi(d_str) : 12;
                uint8_t h = h_str[0] ? (uint8_t)atoi(h_str) : 4;
                uint8_t mi = min_str[0] ? (uint8_t)atoi(min_str) : 0;
                uint8_t se = s_str[0] ? (uint8_t)atoi(s_str) : 0;
                g_machine_fsm.setRTCDateTime(y, mo, da, h, mi, se);
                ok = true;
            } else if (strcmp(cmd, "UNLOCK") == 0) {
                char key[16] = {0};
                if (httpd_query_key_value(query, "key", key, sizeof(key)) == ESP_OK) {
                    if (strlen(key) == 4) {
                        uint16_t k = (uint16_t)atoi(key);
                        ok = g_machine_fsm.verifyDayLockUnlock(k) || g_machine_fsm.applyDayLockAdminBypass(key);
                    } else if (strlen(key) == 6) {
                        ok = g_machine_fsm.applyDayLockEmergency(key);
                    }
                }
            }
        }
    }

    char resp[64];
    snprintf(resp, sizeof(resp), "{\"success\":%s}", ok ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

esp_err_t web_portal_start(void) {
    if (s_server) return ESP_OK;

    // 1. Initialize Wi-Fi SoftAP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("HAFFLER-BAKERY-MIXER", "12345678");
    ESP_LOGI(TAG, "Wi-Fi SoftAP started. SSID: HAFFLER-BAKERY-MIXER, IP: 192.168.4.1");

    // 2. Start HTTP server on Core 0
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 4;
    config.recv_wait_timeout = 5;
    config.send_wait_timeout = 5;
    config.stack_size = 8192;
    config.core_id = 0; // STRICTLY CORE 0

    if (httpd_start(&s_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    // Register endpoints
    httpd_uri_t uri_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_index);

    httpd_uri_t uri_telemetry = {
        .uri = "/api/telemetry",
        .method = HTTP_GET,
        .handler = telemetry_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_telemetry);

    httpd_uri_t uri_control_get = {
        .uri = "/api/control",
        .method = HTTP_GET,
        .handler = control_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_control_get);

    httpd_uri_t uri_control_post = {
        .uri = "/api/control",
        .method = HTTP_POST,
        .handler = control_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_control_post);

    httpd_uri_t uri_204_android = {
        .uri = "/generate_204",
        .method = HTTP_GET,
        .handler = captive_204_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_204_android);

    httpd_uri_t uri_204_ios = {
        .uri = "/hotspot-detect.html",
        .method = HTTP_GET,
        .handler = captive_204_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_204_ios);

    httpd_uri_t uri_204_win = {
        .uri = "/ncsi.txt",
        .method = HTTP_GET,
        .handler = captive_204_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_204_win);

    ESP_LOGI(TAG, "WebPortal initialized on Core 0 with /api/telemetry & /api/control");
    return ESP_OK;
}

void web_portal_stop(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}

void web_portal_broadcast_telemetry(const char *json_payload) {
    if (!s_server || !json_payload) return;
}
