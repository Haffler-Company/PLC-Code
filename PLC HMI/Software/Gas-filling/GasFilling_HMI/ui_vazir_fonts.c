#include "ui_vazir_fonts.h"
#include <stddef.h>
#include <stdint.h>

extern const uint8_t vazirmatn_ttf[];
extern const size_t vazirmatn_ttf_size;

lv_font_t *ui_font_vazir_18 = NULL;
lv_font_t *ui_font_vazir_22 = NULL;
lv_font_t *ui_font_vazir_26 = NULL;

void ui_vazir_fonts_init(void)
{
    /*
     * DISPLAY FIX (half black / half white + dead USB agent):
     * Creating TinyTTF from embedded Vazir during setup caused LVGL to stall
     * mid-first-refresh. loop() never ran → PING timeout, LCD stuck half-drawn.
     * Keep pointers NULL so ui_font_vazir_or() uses Montserrat fallbacks and
     * the full frame can paint. Re-enable TinyTTF only after display is stable.
     */
    (void)vazirmatn_ttf;
    (void)vazirmatn_ttf_size;
    ui_font_vazir_18 = NULL;
    ui_font_vazir_22 = NULL;
    ui_font_vazir_26 = NULL;
}

const lv_font_t *ui_font_vazir_or(const lv_font_t *fallback, int size_px)
{
    (void)size_px;
    if (ui_font_vazir_22) {
        return ui_font_vazir_22;
    }
    return fallback;
}
