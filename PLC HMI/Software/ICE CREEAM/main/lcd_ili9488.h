#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lvgl.h"

void lcd_init_basic(void);
void lcd_set_rotation(uint8_t rot);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   uint8_t r, uint8_t g, uint8_t b);
void lcd_draw_bitmap_rgb565(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            const uint16_t *src_le);
void lcd_fill_screen(uint8_t r, uint8_t g, uint8_t b);

/* توابع بهینه‌سازی شده برای LVGL 9 Flush Pipeline */
void lcd_ll_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_ll_queue_flush(void *px_map, size_t len_bytes);
void lcd_ll_wait_idle(void);
void lcd_ll_data_sync(const void *data, size_t len);
void lcd_ll_register_disp(lv_display_t *disp);
void lcd_ll_flush_done(lv_display_t *disp);

