/**

 * lv_meter → lv_scale compatibility for LVGL 9.

 * Maps v8 scale_lines tick coloring to scale sections (ITEMS part).

 */

#include "lv_meter_compat.h"

#include <string.h>



#define METER_INDIC_MAX 24



typedef struct {

    lv_meter_indicator_t indic;

    lv_style_t style;

    bool style_inited;

} meter_indic_storage_t;



static meter_indic_storage_t s_indics[METER_INDIC_MAX];

static uint8_t s_indic_count;



static meter_indic_storage_t * storage_from_indic(lv_meter_indicator_t *indic)

{

    if (!indic) return NULL;

    return (meter_indic_storage_t *)((char *)indic - offsetof(meter_indic_storage_t, indic));

}



static void indic_apply_range(meter_indic_storage_t *st)

{

    if (!st || !st->indic.section || !st->indic.meter) return;



    int32_t lo = st->indic.start_value;

    int32_t hi = st->indic.end_value;

    if (lo > hi) {

        int32_t t = lo;

        lo = hi;

        hi = t;

    }

    lv_scale_section_set_range(st->indic.section, lo, hi);

    lv_obj_invalidate(st->indic.meter);

}



lv_obj_t * lv_meter_create(lv_obj_t * parent)

{

    lv_obj_t *scale = lv_scale_create(parent);

    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);

    lv_obj_set_style_bg_opa(scale, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_border_width(scale, 0, LV_PART_MAIN);

    lv_obj_set_style_arc_opa(scale, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_pad_all(scale, 4, LV_PART_MAIN);

    return scale;

}



lv_meter_scale_t * lv_meter_add_scale(lv_obj_t * meter)

{

    return meter;

}



void lv_meter_set_scale_ticks(lv_obj_t * meter, lv_meter_scale_t * scale, uint16_t cnt,

                              uint16_t width, uint16_t len, lv_color_t color)

{

    (void)scale;

    if (!meter) return;

    if (cnt < 2) cnt = 2;

    lv_scale_set_total_tick_count(meter, cnt);

    lv_scale_set_major_tick_every(meter, cnt > 5 ? (cnt / 5) : 1);

    lv_obj_set_style_line_width(meter, width, LV_PART_ITEMS);

    lv_obj_set_style_length(meter, len, LV_PART_ITEMS);

    lv_obj_set_style_line_color(meter, color, LV_PART_ITEMS);

    lv_obj_set_style_line_width(meter, width, LV_PART_INDICATOR);

    lv_obj_set_style_length(meter, len, LV_PART_INDICATOR);

    lv_obj_set_style_line_color(meter, color, LV_PART_INDICATOR);

}



void lv_meter_set_scale_range(lv_obj_t * meter, lv_meter_scale_t * scale, int32_t min, int32_t max,

                              uint32_t angle_range, uint32_t rotation)

{

    (void)scale;

    if (!meter) return;

    lv_scale_set_range(meter, min, max);

    lv_scale_set_angle_range(meter, angle_range);

    lv_scale_set_rotation(meter, (int32_t)rotation);

    lv_scale_set_label_show(meter, false);

}



lv_meter_indicator_t * lv_meter_add_scale_lines(lv_obj_t * meter, lv_meter_scale_t * scale,

                                                lv_color_t color_start, lv_color_t color_end,

                                                bool local, int16_t width_mod)

{

    (void)scale;

    (void)local;

    (void)color_end;

    if (!meter || s_indic_count >= METER_INDIC_MAX) return NULL;



    meter_indic_storage_t *st = &s_indics[s_indic_count++];

    memset(st, 0, sizeof(*st));

    st->indic.meter = meter;

    st->indic.section = lv_scale_add_section(meter);

    st->indic.start_value = 0;

    st->indic.end_value = 0;

    st->indic.opa = LV_OPA_COVER;



    lv_style_init(&st->style);

    st->style_inited = true;

    lv_style_set_line_color(&st->style, color_start);

    lv_style_set_line_width(&st->style, (int32_t)(width_mod > 0 ? width_mod : 5));

    lv_style_set_line_rounded(&st->style, false);

    lv_scale_section_set_style(st->indic.section, LV_PART_ITEMS, &st->style);

    lv_scale_section_set_style(st->indic.section, LV_PART_INDICATOR, &st->style);

    lv_scale_section_set_range(st->indic.section, 0, 0);



    return &st->indic;

}



void lv_meter_set_indicator_start_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value)

{

    (void)meter;

    meter_indic_storage_t *st = storage_from_indic(indic);

    if (!st) return;

    st->indic.start_value = value;

    indic_apply_range(st);

}



void lv_meter_set_indicator_end_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value)

{

    (void)meter;

    meter_indic_storage_t *st = storage_from_indic(indic);

    if (!st) return;

    st->indic.end_value = value;

    if (st->indic.opa != LV_OPA_COVER && st->style_inited) {

        lv_style_set_line_opa(&st->style, st->indic.opa);

        lv_scale_section_set_style(st->indic.section, LV_PART_ITEMS, &st->style);

        lv_scale_section_set_style(st->indic.section, LV_PART_INDICATOR, &st->style);

    }

    indic_apply_range(st);

}



void lv_meter_set_indicator_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value)

{

    lv_meter_set_indicator_start_value(meter, indic, value);

    lv_meter_set_indicator_end_value(meter, indic, value);

}


