#include "ui_components.h"
#include <string.h>

lv_obj_t* ui_create_apple_card(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, 
                               lv_coord_t w, lv_coord_t h, const char* title) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    if (title != NULL && strlen(title) > 0) {
        lv_obj_t* lbl_title = lv_label_create(card);
        lv_label_set_text(lbl_title, title);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x8B949E), 0);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);
    }
    return card;
}

hero_metric_t ui_create_hero_metric(lv_obj_t* parent, lv_coord_t w, lv_coord_t h, 
                                    const char* unit_str, const lv_font_t* digit_font) {
    hero_metric_t metric;
    memset(&metric, 0, sizeof(hero_metric_t));

    metric.container = lv_obj_create(parent);
    lv_obj_set_size(metric.container, w, h);
    lv_obj_set_style_bg_opa(metric.container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(metric.container, 0, 0);
    lv_obj_set_style_pad_all(metric.container, 0, 0);
    lv_obj_clear_flag(metric.container, LV_OBJ_FLAG_SCROLLABLE);

    metric.lbl_value = lv_label_create(metric.container);
    lv_label_set_text(metric.lbl_value, "0.0");
    lv_obj_set_style_text_color(metric.lbl_value, lv_color_hex(0xFFFFFF), 0);
    if (digit_font) lv_obj_set_style_text_font(metric.lbl_value, digit_font, 0);
    lv_obj_center(metric.lbl_value);

    if (unit_str != NULL) {
        metric.lbl_unit = lv_label_create(metric.container);
        lv_label_set_text(metric.lbl_unit, unit_str);
        lv_obj_set_style_text_color(metric.lbl_unit, lv_color_hex(0x64748B), 0);
        lv_obj_set_style_text_font(metric.lbl_unit, &lv_font_montserrat_14, 0);
        lv_obj_align(metric.lbl_unit, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
    }
    return metric;
}

void ui_update_hero_metric(hero_metric_t* m, const char* new_val) {
    if (m && m->lbl_value && strcmp(m->last_val_str, new_val) != 0) {
        lv_label_set_text(m->lbl_value, new_val);
        strncpy(m->last_val_str, new_val, sizeof(m->last_val_str) - 1);
    }
}

lv_obj_t* ui_create_stepper(lv_obj_t* parent, lv_coord_t y_pos, const char* title, 
                            const char* unit_str, lv_event_cb_t minus_cb, lv_event_cb_t plus_cb) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, 440, 48);
    lv_obj_set_pos(row, 12, y_pos);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_all(row, 2, 0);

    lv_obj_t* btn_minus = lv_btn_create(row);
    lv_obj_set_size(btn_minus, 44, 44);
    lv_obj_align(btn_minus, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_radius(btn_minus, 6, 0);
    if (minus_cb) lv_obj_add_event_cb(btn_minus, minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_minus = lv_label_create(btn_minus);
    lv_label_set_text(lbl_minus, LV_SYMBOL_MINUS);
    lv_obj_center(lbl_minus);

    lv_obj_t* val_box = lv_obj_create(row);
    lv_obj_set_size(val_box, 80, 40);
    lv_obj_set_pos(val_box, 50, 2);
    lv_obj_set_style_bg_color(val_box, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_border_color(val_box, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(val_box, 1, 0);
    lv_obj_set_style_pad_all(val_box, 0, 0);
    lv_obj_t* lbl_val = lv_label_create(val_box);
    lv_label_set_text(lbl_val, "0.0");
    lv_obj_center(lbl_val);

    lv_obj_t* btn_plus = lv_btn_create(row);
    lv_obj_set_size(btn_plus, 44, 44);
    lv_obj_set_pos(btn_plus, 134, 0);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_radius(btn_plus, 6, 0);
    if (plus_cb) lv_obj_add_event_cb(btn_plus, plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_plus = lv_label_create(btn_plus);
    lv_label_set_text(lbl_plus, LV_SYMBOL_PLUS);
    lv_obj_center(lbl_plus);

    lv_obj_t* lbl_param = lv_label_create(row);
    lv_label_set_text_fmt(lbl_param, "%s %s", title ? title : "", unit_str ? unit_str : "");
    lv_obj_set_style_text_color(lbl_param, lv_color_hex(0xF0F6FC), 0);
    lv_obj_align(lbl_param, LV_ALIGN_RIGHT_MID, -12, 0);

    return row;
}

lv_obj_t* ui_create_pill_badge(lv_obj_t* parent, const char* text, uint32_t bg_hex, uint32_t text_hex) {
    lv_obj_t* pill = lv_obj_create(parent);
    lv_obj_set_size(pill, LV_SIZE_CONTENT, 22);
    lv_obj_set_style_bg_color(pill, lv_color_hex(bg_hex), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(pill, 0, 0);
    lv_obj_set_style_pad_hor(pill, 8, 0);
    lv_obj_set_style_pad_ver(pill, 2, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl = lv_label_create(pill);
    lv_label_set_text(lbl, text ? text : "");
    lv_obj_set_style_text_color(lbl, lv_color_hex(text_hex), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(lbl);

    return pill;
}
