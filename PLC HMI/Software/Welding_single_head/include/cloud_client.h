#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Cloud Client subsystem and FreeRTOS worker task on Core 0.
 */
void cloud_client_init(void);

/**
 * @brief Notify Cloud Client that Wi-Fi station is connected and IP is acquired.
 */
void cloud_client_on_wifi_connected(void);

/**
 * @brief Notify Cloud Client that Wi-Fi station has disconnected.
 */
void cloud_client_on_wifi_disconnected(void);

/**
 * @brief Check if currently connected to MQTT Cloud Broker.
 */
bool cloud_client_is_connected(void);

/**
 * @brief Request immediate telemetry publish (non-blocking).
 */
void cloud_client_trigger_telemetry(void);

/**
 * @brief Get device unique cloud identifier.
 */
const char *cloud_client_get_device_id(void);

#ifdef __cplusplus
}
#endif
