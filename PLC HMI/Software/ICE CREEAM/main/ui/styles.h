#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_COLOR_ORANGE   0xC45A1A
#define UI_COLOR_CYAN     0x7FDBDA
#define UI_COLOR_OFF_BG   0x334155
#define UI_COLOR_ON_BG    0x16A34A
#define UI_COLOR_READY_BG 0x0284C7

void ui_style_status_label(lv_obj_t *lbl, bool on);
void ui_style_status_ready(lv_obj_t *lbl);
void ui_style_orange_btn(lv_obj_t *btn);
void ui_style_cyan_value(lv_obj_t *obj);
lv_obj_t *ui_make_orange_btn(lv_obj_t *parent, const char *txt, int32_t x, int32_t y, int32_t w, int32_t h);
lv_obj_t *ui_make_cyan_value(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/
