/**
 * @file ota_service.h
 * @brief Fail-Safe Crash-Proof HTTPS OTA Update Engine with Dual Rollback
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CONNECTING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_SUCCESS,
    OTA_STATE_FAILED
} ota_state_t;

typedef void (*ota_progress_callback_t)(uint32_t bytes_downloaded, uint32_t total_bytes, uint8_t percent);

esp_err_t ota_service_init(void);

/**
 * @brief Start background HTTPS firmware download & flash
 * @param url Full HTTPS download URL (e.g. https://ota.haffler-infi.ir/upvc/welding-single-head/firmware.bin)
 * @param cb Progress callback for LVGL HMI UI
 */
esp_err_t ota_service_start_update(const char *url, ota_progress_callback_t cb);

/**
 * @brief Get current OTA engine status
 */
ota_state_t ota_service_get_state(void);

/**
 * @brief Confirm that current firmware is healthy (cancels bootloader auto-rollback)
 */
esp_err_t ota_service_confirm_valid_boot(void);

#ifdef __cplusplus
}
#endif
