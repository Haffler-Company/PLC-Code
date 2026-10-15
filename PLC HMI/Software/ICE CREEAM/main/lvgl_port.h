#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

void lvgl_port_init(void);
void lvgl_start(void);

bool lvgl_lock(TickType_t timeout);
void lvgl_unlock(void);
void lvgl_port_wake(void);

void lvgl_port_inject_touch(uint16_t x, uint16_t y, bool pressed);
void lvgl_port_clear_inject_touch(void);

uint32_t lvgl_port_get_fps(void);
uint32_t lvgl_port_get_flush_us(void);
uint32_t lvgl_port_get_render_us(void);
bool lvgl_port_is_touch_active(void);
void lvgl_port_suppress_touch_until_release(void);

#ifdef __cplusplus
}
#endif
