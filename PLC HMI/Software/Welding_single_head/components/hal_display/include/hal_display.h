/**
 * @file hal_display.h
 * @brief High-Performance 60 FPS Asynchronous DMA Display Driver (esp_lcd + LVGL 9.4)
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

#define LCD_H_RES              480
#define LCD_V_RES              320
#define LCD_BUFFER_LINES       32

#define LCD_PIN_MOSI           10
#define LCD_PIN_MISO           11
#define LCD_PIN_SCLK           12
#define LCD_PIN_CS             13
#define LCD_PIN_DC             14
#define LCD_PIN_RST            (-1)
#define LCD_PIN_BL             (-1)

#define LCD_SPI_FREQ_HZ        (80 * 1000 * 1000)

/**
 * @brief Initialize SPI bus, ILI9341 panel driver and register LVGL 9.4 display
 * @return lv_display_t* Handle to the registered LVGL display
 */
lv_display_t *hal_display_init(void);

/**
 * @brief Set screen brightness (if backlight pin configured)
 */
void hal_display_set_backlight(uint8_t brightness_pct);

#ifdef __cplusplus
}
#endif
