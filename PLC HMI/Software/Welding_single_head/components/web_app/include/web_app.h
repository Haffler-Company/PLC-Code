/**
 * @file web_app.h
 * @brief High-Performance Asynchronous WebApp & WebSocket Server for Mobile Monitoring
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEB_APP_HTTP_PORT    80
#define WEB_APP_WS_PORT      81

esp_err_t web_app_init(void);
esp_err_t web_app_start_ap(const char *ssid, const char *pass);
esp_err_t web_app_stop(void);
bool web_app_is_active(void);
uint8_t web_app_get_connected_stations(void);

#ifdef __cplusplus
}
#endif
