#ifndef LV_METER_COMPAT_H
#define LV_METER_COMPAT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef lv_obj_t lv_meter_scale_t;

typedef struct _lv_meter_indicator_t {
    lv_scale_section_t * section;
    lv_obj_t * meter;
    int32_t start_value;
    int32_t end_value;
    lv_opa_t opa;
} lv_meter_indicator_t;

lv_obj_t * lv_meter_create(lv_obj_t * parent);
lv_meter_scale_t * lv_meter_add_scale(lv_obj_t * meter);
void lv_meter_set_scale_ticks(lv_obj_t * meter, lv_meter_scale_t * scale, uint16_t cnt, uint16_t width,
                              uint16_t len, lv_color_t color);
void lv_meter_set_scale_range(lv_obj_t * meter, lv_meter_scale_t * scale, int32_t min, int32_t max,
                              uint32_t angle_range, uint32_t rotation);
lv_meter_indicator_t * lv_meter_add_scale_lines(lv_obj_t * meter, lv_meter_scale_t * scale,
                                                lv_color_t color_start, lv_color_t color_end,
                                                bool local, int16_t width_mod);
void lv_meter_set_indicator_start_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_end_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);

#ifdef __cplusplus
}
#endif

#endif
