#include "haffler_web_portal.h"
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>

static const char *TAG = "WEB_PORTAL";
static httpd_handle_t s_server = NULL;

#define MIN_SAFE_DRAM_BYTES 40000

#include "haffler_webapp_bundle.h"

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

// Mobile Captive Portal 204 response (prevents Android/iOS from dropping AP)
static esp_err_t captive_204_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

esp_err_t web_portal_start(void) {
    if (s_server) return ESP_OK;

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

    ESP_LOGI(TAG, "WebPortal initialized on Core 0 with captive portal 204 handler");
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
    // Broadcast to WebSocket clients if configured
}
