/**
 * @file web_app.cpp
 * @brief WebApp Server Implementation using native esp_http_server & WebSockets
 */

#include "web_app.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "plc_engine.h"
#include "nvs_storage.h"
#include "hal_thermo.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "WEB_APP";

static httpd_handle_t s_http_server = NULL;
static bool s_is_active = false;

static esp_err_t root_handler(httpd_req_t *req) {
    // Serve gzip index.html from SPIFFS
    FILE *f = fopen("/spiffs/index.html.gz", "rb");
    if (!f) {
        f = fopen("/spiffs/index.html", "rb");
    }
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    char chunk[1024];
    size_t read_bytes;
    while ((read_bytes = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        httpd_resp_send_chunk(req, chunk, read_bytes);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket client handshake connected.");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t buf[256];
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, sizeof(buf) - 1);
    if (ret != ESP_OK) return ret;

    buf[ws_pkt.len] = '\0';
    // Process JSON Requests
    plc_cycle_status_t status = plc_engine_get_status();
    char resp_buf[256];
    snprintf(resp_buf, sizeof(resp_buf),
             "{\"temp\":%.1f,\"target\":%.1f,\"step\":%d,\"in_prog\":%d,\"e_stop\":%d}",
             status.current_temperature_c, status.target_temperature_c,
             (int)status.step, status.in_progress ? 1 : 0, status.emergency_stopped ? 1 : 0);

    httpd_ws_frame_t resp_pkt;
    memset(&resp_pkt, 0, sizeof(httpd_ws_frame_t));
    resp_pkt.payload = (uint8_t *)resp_buf;
    resp_pkt.len = strlen(resp_buf);
    resp_pkt.type = HTTPD_WS_TYPE_TEXT;

    return httpd_ws_send_frame(req, &resp_pkt);
}

esp_err_t web_app_init(void) {
    ESP_LOGI(TAG, "Initializing WebApp Subsystem & SPIFFS Mount...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
    }

    return ESP_OK;
}

esp_err_t web_app_start_ap(const char *ssid, const char *pass) {
    ESP_LOGI(TAG, "Starting SoftAP [%s] and HTTP/WS Server...", ssid);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_APP_HTTP_PORT;
    config.ctrl_port = 32768;
    config.max_open_sockets = 4;

    if (httpd_start(&s_http_server, &config) == ESP_OK) {
        httpd_uri_t uri_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_http_server, &uri_get);

        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = ws_handler,
            .user_ctx = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(s_http_server, &ws_uri);

        s_is_active = true;
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t web_app_stop(void) {
    if (s_http_server) {
        httpd_stop(s_http_server);
        s_http_server = NULL;
    }
    s_is_active = false;
    return ESP_OK;
}

bool web_app_is_active(void) {
    return s_is_active;
}

uint8_t web_app_get_connected_stations(void) {
    wifi_sta_list_t sta_list;
    if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK) {
        return (uint8_t)sta_list.num;
    }
    return 0;
}
