/**
 * @file haffler_cloud_scada.h
 * @brief Haffler IIoT Cloud SCADA & WebSerial Bridge for ESP32-S3
 * @version 1.0.0
 * 
 * Standards:
 * - Dual-Core FreeRTOS Isolation: Runs strictly on Core 0. Never blocks Core 1 PLC loop.
 * - Cross-City Cloud SCADA: Communicates with haffler-ai.ir:1883 over MQTT.
 * - WebSerial Direct Cable: Emits STAT:... telemetry strings to USB CDC.
 * - ISA-18.2 Compliant Command Dispatch & Acknowledgment.
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*haffler_cmd_callback_t)(const char *action, const char *params_json);

typedef struct {
    float hero_val;          // Primary metric (e.g. Temp °C, Length mm, RPM)
    float target_val;        // Target setpoint
    const char *state_name;  // Current FSM step (IDLE, HEATING, CUTTING, WELDING, etc.)
    uint32_t cycle_count;    // Lifetime or shift cycle count
    uint16_t active_relays;  // 10-bit bitmask of active relays Q0-Q9
    uint16_t active_inputs;  // 16-bit bitmask of active opto-inputs I0-I15
    bool error_flag;         // Error or warning present
    uint32_t error_code;     // Error code if error_flag == true
} haffler_telemetry_data_t;

/**
 * @brief Initialize Cloud SCADA, Wi-Fi Station & MQTT Client on Core 0
 * @param machine_slug Fleet slug (e.g. "weld_single", "cutter_single", "code_run", "ice_cream")
 * @param device_id Device identifier (e.g. "WELD-01", "CUT-01", "CODERUN-01", "ICE-01")
 * @param cmd_cb Callback executed upon receiving remote or serial commands
 */
void haffler_cloud_scada_init(const char *machine_slug, const char *device_id, haffler_cmd_callback_t cmd_cb);

/**
 * @brief Thread-safe update of current machine telemetry from Core 1
 * Called by PLC engine to push fresh metrics to Core 0 network queue
 */
void haffler_cloud_scada_update_telemetry(const haffler_telemetry_data_t *data);

/**
 * @brief Send command execution acknowledgment back to Web Portal
 * @param action Action name (e.g. "start_weld", "clamp_toggle")
 * @param status Status string ("accepted", "rejected", "completed")
 */
void haffler_cloud_scada_send_ack(const char *action, const char *status);

/**
 * @brief Check if Cloud MQTT Broker is actively connected
 */
bool haffler_cloud_scada_is_connected(void);

/**
 * @brief Process incoming WebSerial commands received from USB cable
 * @param line Serial command string received from USB CDC
 */
void haffler_cloud_scada_handle_serial_input(const char *line);

#ifdef __cplusplus
}
#endif
