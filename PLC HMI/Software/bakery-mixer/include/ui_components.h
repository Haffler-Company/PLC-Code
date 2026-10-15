#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* lbl_value;
    lv_obj_t* lbl_unit;
    char      last_val_str[32];
} hero_metric_t;

// Apple HIG dark card container (1px border, 8px radius, zero shadow)
lv_obj_t* ui_create_apple_card(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, 
                               lv_coord_t w, lv_coord_t h, const char* title);

// Mathematically centered hero metric (zero-pad container, center aligned)
hero_metric_t ui_create_hero_metric(lv_obj_t* parent, lv_coord_t w, lv_coord_t h, 
                                    const char* unit_str, const lv_font_t* digit_font);
void ui_update_hero_metric(hero_metric_t* m, const char* new_val);

// Parameter stepper row with guaranteed 16px breathing margin
lv_obj_t* ui_create_stepper(lv_obj_t* parent, lv_coord_t y_pos, const char* title, 
                            const char* unit_str, lv_event_cb_t minus_cb, lv_event_cb_t plus_cb,
                            lv_obj_t** out_lbl_val);

// Pill status badge (capsule shape)
lv_obj_t* ui_create_pill_badge(lv_obj_t* parent, const char* text, uint32_t bg_hex, uint32_t text_hex);

#ifdef __cplusplus
}
#endif
