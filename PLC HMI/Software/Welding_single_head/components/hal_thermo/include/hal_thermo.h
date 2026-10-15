/**
 * @file hal_thermo.h
 * @brief MAX6675 Thermocouple Driver with Process Safety & Thermal Runaway Interlock
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THERMO_PIN_MISO    37
#define THERMO_PIN_SCK     47
#define THERMO_PIN_CS_1    21
#define THERMO_PIN_CS_2    0

#define THERMO_MAX_SAFE_TEMP_C   350.0f
#define THERMO_MIN_SAFE_TEMP_C   0.0f

typedef enum {
    THERMO_STATUS_OK = 0,
    THERMO_STATUS_OPEN_CIRCUIT,
    THERMO_STATUS_OUT_OF_BOUNDS,
    THERMO_STATUS_COMM_ERROR
} hal_thermo_status_t;

typedef struct {
    float temperature_c;
    hal_thermo_status_t status;
    uint32_t last_read_timestamp_ms;
} hal_thermo_reading_t;

/**
 * @brief Initialize MAX6675 SPI and CS GPIOs
 */
esp_err_t hal_thermo_init(void);

/**
 * @brief Read temperature from primary thermocouple (Element)
 * @details Automatically triggers SSR emergency cutoff if sensor fault or thermal runaway is detected.
 */
hal_thermo_reading_t hal_thermo_read_primary(void);

/**
 * @brief Read temperature from secondary thermocouple (if equipped)
 */
hal_thermo_reading_t hal_thermo_read_secondary(void);

/**
 * @brief Check if primary thermocouple is currently healthy
 */
bool hal_thermo_is_healthy(void);

#ifdef __cplusplus
}
#endif
