/**
 * @file ota_pre_erase.h
 * @brief Haffler High-Speed Zero-Lag OTA Engine - Background Flash Pre-Erase
 * @author Haffler Industrial Engineering
 */

#ifndef OTA_PRE_ERASE_H
#define OTA_PRE_ERASE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_partition.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start autonomous background pre-erase on the passive OTA partition.
 * Erases in 64KB blocks with 15ms yields on Core 0 so Core 1 UI stays 60 FPS.
 */
void ota_pre_erase_start(void);

/**
 * @brief Stop background pre-erase worker task if running.
 */
void ota_pre_erase_stop(void);

/**
 * @brief Check if the passive OTA partition is 100% clean and ready for direct write.
 */
bool ota_is_pre_erase_done(void);

/**
 * @brief Ensure the passive partition is 100% erased before opening HTTP socket.
 * Clean Flash Guarantee: Never stream TCP packets while SPI flash is erasing!
 */
void ota_pre_erase_ensure_clean_blocking(void);

/**
 * @brief Get pre-erase progress percentage (0..100).
 */
uint8_t ota_pre_erase_get_progress(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_PRE_ERASE_H
