#ifndef DISPLAY_PORT_H
#define DISPLAY_PORT_H

#include <lvgl.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <TFT_eSPI.h>
#include <bb_captouch.h>

extern "C" {
/** ESP32-S3 ILI9341 + bb_captouch — LVGL 9 display/input port. */
bool display_port_init(TFT_eSPI *tft, BBCapTouch *touch);
bool display_port_touch_detected(void);
void display_port_touch_info(uint8_t *chip, uint8_t *addr, uint32_t *reads, uint32_t *errs);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** DMA API kept for compatibility; enabling is a no-op (black screen on this board). */
void display_port_set_dma_enabled(bool enabled);

/** Card image bake memory management (PSRAM / Internal fallback) */
void *display_port_allocate_card_buffer(size_t nbytes);
void display_port_free_card_buffer(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
