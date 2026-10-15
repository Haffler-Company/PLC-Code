#pragma once
#include <lvgl.h>

/**
 * @file haffler_ui_theme.h
 * @brief Haffler Apple HIG UI Theme, Ergonomic Standards & Mathematical Centering Helpers
 * 480x320 Dark Slate Architecture + 1px Borders + Zero Software Shadows
 */

#ifdef __cplusplus
extern "C" {
#endif

// Official Industrial Apple HIG Palettes
#define COLOR_BG_CANVAS         lv_color_hex(0x0A0C10) // Deep Matte Slate Canvas
#define COLOR_CARD_BG           lv_color_hex(0x161B22) // Grouping Card Dark Charcoal
#define COLOR_CARD_BORDER       lv_color_hex(0x30363D) // 1px Hairline Crisp Border
#define COLOR_HERO_TEXT         lv_color_hex(0xFFFFFF) // Pure White High-Contrast Digits
#define COLOR_SEMANTIC_READY    lv_color_hex(0x22C55E) // Emerald Neon Green (Ready/Start)
#define COLOR_SEMANTIC_ACTIVE   lv_color_hex(0x007AFF) // Apple System Blue (Selection)
#define COLOR_SEMANTIC_WARN     lv_color_hex(0xF59E0B) // Industrial Amber (Hold/Day-Lock)
#define COLOR_SEMANTIC_DANGER   lv_color_hex(0xEF4444) // Coral Red (E-Stop/Fault)

// Helper: Apply Apple HIG styling to grouping cards with 1px border and ZERO software shadow
static inline void ui_card_apply_style(lv_obj_t *card) {
    lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(card, COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_shadow_width(card, 0, 0); // Mandatory 0 for 50+ FPS
}

// Helper: Mathematical Centering of Hero Metric numeric label inside dedicated container
static inline void ui_metric_container_setup(lv_obj_t *cont, lv_obj_t *lbl_num) {
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_gap(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(lbl_num);
}

// Helper: 16px Breathing Margin for Stepper increment buttons
#define STEPPER_BREATHING_MARGIN_PX 16

#ifdef __cplusplus
}
#endif
