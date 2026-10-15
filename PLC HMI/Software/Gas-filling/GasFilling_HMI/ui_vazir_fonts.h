#ifndef UI_VAZIR_FONTS_H
#define UI_VAZIR_FONTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime TinyTTF fonts from embedded Vazirmatn-Regular.ttf */
extern lv_font_t *ui_font_vazir_18;
extern lv_font_t *ui_font_vazir_22;
extern lv_font_t *ui_font_vazir_26;

void ui_vazir_fonts_init(void);
const lv_font_t *ui_font_vazir_or(const lv_font_t *fallback, int size_px);

#ifdef __cplusplus
}
#endif

#endif
