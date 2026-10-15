#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERIAL_HMI_BAUD            921600
#define SERIAL_HMI_RX_BUF_SIZE     4096
#define SERIAL_HMI_TX_BUF_SIZE     16384
#define SERIAL_MASTER_SECRET       "ICE_CREAM_PLC_SECURE_KEY_2026"
#define SERIAL_SEC_BYPASS          0

#define SHOT_W                     480
#define SHOT_H                     320
#define SHOT_TIMEOUT_MS            4000

/**
 * @brief Initialize the serial HMI communication task and drivers on Core 0.
 *
 * Configures UART0 (and USB Serial JTAG if available) at 921,600 baud,
 * registers protocol handlers, and spawns the background worker task.
 */
esp_err_t serial_hmi_init(void);

/**
 * @brief Hook called during LVGL display flush to capture rendered pixels.
 * Zero-cost when screenshot is not active.
 */
void serial_hmi_on_flush(const lv_area_t *area, const uint16_t *px);

/**
 * @brief Request an asynchronous zero-lag screenshot.
 */
void serial_hmi_request_shot(void);

/**
 * @brief Request an ultra-fast raw binary screenshot (~3.3s at 921600 baud).
 */
void serial_hmi_request_raw_shot(void);

/**
 * @brief Synchronous capture for HTTP Web Portal and MQTT endpoints.
 * Arms capture, requests LVGL redraw, and waits up to timeout_ms.
 */
bool serial_hmi_capture_sync(uint32_t timeout_ms);

/**
 * @brief Get pointer to 480x320 16-bit RGB565 raw pixel buffer.
 */
const uint16_t* serial_hmi_get_buffer(void);

/**
 * @brief Release the internal shot buffer back to IDLE state.
 * Must be called after HTTP handler finishes reading the buffer,
 * so the next /api/screenshot request can trigger a fresh capture.
 */
void serial_hmi_release_shot(void);

#ifdef __cplusplus
}
#endif
