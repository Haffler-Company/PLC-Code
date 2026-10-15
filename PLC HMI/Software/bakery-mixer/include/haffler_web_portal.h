#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_http_server.h>

/**
 * @file haffler_web_portal.h
 * @brief Haffler Hardened Embedded WebPortal & Mobile Telemetry Engine
 * Zero-Hang WebServer on Core 0 + Internal DRAM Protection (>40KB) + Captive 204 + WebSocket
 */

#ifdef __cplusplus
extern "C" {
#endif

// Start hardened embedded web server on Core 0
esp_err_t web_portal_start(void);

// Stop web server
void web_portal_stop(void);

// Broadcast real-time JSON telemetry packet to all connected WebSocket clients (250ms cadence)
void web_portal_broadcast_telemetry(const char *json_payload);

// Check if internal DRAM is above safety threshold (>40KB)
bool web_portal_is_dram_healthy(void);

#ifdef __cplusplus
}
#endif
