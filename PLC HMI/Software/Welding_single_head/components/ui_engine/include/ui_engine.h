/**
 * @file ui_engine.h
 * @brief Zero-Lag UI Engine Bridge for LVGL 9.4 & EEZ Studio Screens
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all EEZ Studio screens, fonts, and styles
 */
esp_err_t ui_engine_init(void);

/**
 * @brief Periodic UI tick (invoked from lvgl_task on Core 1)
 */
void ui_engine_tick(void);

#ifdef __cplusplus
}
#endif
