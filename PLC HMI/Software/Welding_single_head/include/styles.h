#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: EnS
lv_style_t *get_style_en_s_MAIN_DEFAULT();
void add_style_en_s(lv_obj_t *obj);
void remove_style_en_s(lv_obj_t *obj);

// Style: Dns
lv_style_t *get_style_dns_MAIN_DEFAULT();
void add_style_dns(lv_obj_t *obj);
void remove_style_dns(lv_obj_t *obj);

// Style: disable_Mode_btn
lv_style_t *get_style_disable_mode_btn_MAIN_DEFAULT();
void add_style_disable_mode_btn(lv_obj_t *obj);
void remove_style_disable_mode_btn(lv_obj_t *obj);

// Style: enable_Mode_btn
lv_style_t *get_style_enable_mode_btn_MAIN_DEFAULT();
lv_style_t *get_style_enable_mode_btn_MAIN_CHECKED();
void add_style_enable_mode_btn(lv_obj_t *obj);
void remove_style_enable_mode_btn(lv_obj_t *obj);

// Style: kc
lv_style_t *get_style_kc_MAIN_DEFAULT();
void add_style_kc(lv_obj_t *obj);
void remove_style_kc(lv_obj_t *obj);

// Style: out_display
lv_style_t *get_style_out_display_INDICATOR_DEFAULT();
lv_style_t *get_style_out_display_MAIN_PRESSED();
lv_style_t *get_style_out_display_MAIN_DEFAULT();
void add_style_out_display(lv_obj_t *obj);
void remove_style_out_display(lv_obj_t *obj);

// Style: Error_Style
lv_style_t *get_style_error_style_MAIN_DEFAULT();
void add_style_error_style(lv_obj_t *obj);
void remove_style_error_style(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/