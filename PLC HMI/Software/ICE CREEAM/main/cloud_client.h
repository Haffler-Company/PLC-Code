#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Cloud Telemetry & Remote Control Client (FreeRTOS Core 0 Task).
 */
void cloud_client_init(void);

/**
 * @brief Notify cloud client that WiFi STA connection is established with an IP.
 */
void cloud_client_on_wifi_connected(void);

/**
 * @brief Notify cloud client that WiFi STA connection is lost.
 */
void cloud_client_on_wifi_disconnected(void);

/**
 * @brief Check if MQTT cloud broker is currently connected.
 */
bool cloud_client_is_connected(void);

/**
 * @brief Get unique device identifier (e.g. "ASTIAJ-ICE-A1B2C3").
 */
const char *cloud_client_get_device_id(void);

/**
 * @brief Trigger immediate transmission of telemetry packet.
 */
void cloud_client_publish_telemetry_now(void);

/**
 * @brief Change cloud broker URI dynamically and reconnect.
 */
void cloud_client_set_broker_uri(const char *broker_uri);

/**
 * @brief Stop cloud client and close connections.
 */
void cloud_client_stop(void);
void cloud_client_pause(void);
void cloud_client_resume(void);

/**
 * @brief Customer and location metadata profile.
 */
const char *cloud_client_get_customer_name(void);
const char *cloud_client_get_city(void);
const char *cloud_client_get_serial_no(void);
void cloud_client_set_profile(const char *cust, const char *city, const char *sn);

#ifdef __cplusplus
}
#endif
