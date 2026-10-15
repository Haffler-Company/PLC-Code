#include "styles.h"
#include "fonts.h"
#include "ui_lang.h"
#include <stdbool.h>
#include <string.h>

static bool text_needs_persian_font(const char *s)
{
    if (!s) {
        return false;
    }
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        if (*p >= 0x80) {
            return true;
        }
    }
    return false;
}

void ui_style_status_label(lv_obj_t *target_obj, bool on)
{
    if (!target_obj) {
        return;
    }
    lv_obj_t *lbl = target_obj;
    lv_obj_t *box = target_obj;
    if (lv_obj_get_child_count(target_obj) > 0) {
        lbl = lv_obj_get_child(target_obj, 0);
    }
    const char *target = on ? ui_lang_str(STR_STATUS_ON) : ui_lang_str(STR_STATUS_OFF);
    const char *cur = lv_label_get_text(lbl);
    if (cur && strcmp(cur, target) == 0) {
        return; /* مقدار قبلاً برابر بوده است؛ رندر مجدد لازم نیست */
    }
    lv_obj_set_style_text_font(lbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl, target);
    lv_obj_center(lbl);

    if (on) {
        lv_obj_set_style_bg_color(box, lv_color_hex(UI_COLOR_ON_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_color(box, lv_color_hex(UI_COLOR_OFF_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_style_status_ready(lv_obj_t *target_obj)
{
    if (!target_obj) {
        return;
    }
    lv_obj_t *lbl = target_obj;
    lv_obj_t *box = target_obj;
    if (lv_obj_get_child_count(target_obj) > 0) {
        lbl = lv_obj_get_child(target_obj, 0);
    }
    const char *target = ui_lang_str(STR_STATUS_READY);
    const char *cur = lv_label_get_text(lbl);
    if (cur && strcmp(cur, target) == 0) {
        return; /* قبلاً در وضعیت آماده است */
    }
    lv_obj_set_style_text_font(lbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl, target);
    lv_obj_center(lbl);

    lv_obj_set_style_bg_color(box, lv_color_hex(UI_COLOR_READY_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_style_orange_btn(lv_obj_t *btn)
{
    if (!btn) {
        return;
    }
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xEA580C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xC2410C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_style_cyan_value(lv_obj_t *obj)
{
    if (!obj) {
        return;
    }
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xF1F5F9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xCBD5E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0x0F172A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *ui_make_orange_btn(lv_obj_t *parent, const char *txt, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    ui_style_orange_btn(btn);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, w - 8);
    lv_label_set_text(lbl, txt ? txt : "");

    if (text_needs_persian_font(txt)) {
        lv_obj_set_style_text_font(lbl, &ui_font_yekan_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        /* لاتین با یکان خراب می‌شود */
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl);
    return btn;
}

lv_obj_t *ui_make_cyan_value(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_pos(ta, x, y);
    lv_obj_set_size(ta, w, h);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 8);
    lv_textarea_set_align(ta, LV_TEXT_ALIGN_CENTER);
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
    ui_style_cyan_value(ta);
    /* اعداد با Montserrat تا وسط‌چین و لاتین درست دیده شوند */
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(ta, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    return ta;
}
