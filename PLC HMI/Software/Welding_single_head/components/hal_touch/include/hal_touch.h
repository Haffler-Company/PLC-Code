/**
 * @file hal_touch.h
 * @brief Capacitive Touch Driver with Anti-Jitter Filter & LVGL 9.4 Indev Registration
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_PIN_SDA    8
#define TOUCH_PIN_SCL    9
#define TOUCH_PIN_RST    (-1)
#define TOUCH_PIN_INT    (-1)

/**
 * @brief Initialize touch controller and register LVGL pointer indev
 * @return lv_indev_t* Handle to the registered LVGL input device
 */
lv_indev_t *hal_touch_init(void);

/**
 * @brief Notify touch subsystem of user activity (for screensaver reset)
 */
uint32_t hal_touch_get_last_activity_ms(void);

#ifdef __cplusplus
}
#endif
