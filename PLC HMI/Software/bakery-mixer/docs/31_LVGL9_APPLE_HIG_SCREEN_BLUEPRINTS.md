# 22. LVGL 9 Apple HIG Screen Blueprints & Component Pack
## Complete Ready-to-Copy C/C++ UI Components for 480x320 Industrial Touchscreens

> **Document ID:** `22_LVGL9_APPLE_HIG_SCREEN_BLUEPRINTS.md`  
> **Target Framework:** LVGL v9.4+ on 480x320 ST7796 SPI TFT & GT911 Capacitive Touch.  
> **Mission:** Instant copy-paste code snippets for the 7 primary screen components. Eliminates layout drift, guarantees 50+ FPS, and enforces Apple HIG Dark Slate aesthetics.

---

## The 7 Universal Industrial UI Components

```
+-----------------------------------------------------------------------------------------------+
| Component 1: Apple HIG Industrial Header Bar (Status Pill, Real-Time Clock, Language Capsule) |
| Component 2: Centered Hero Metric Display (Bold Digits, Centered Unit, Zero Software Shadow)  |
| Component 3: Balanced 2x2 Sub-Cards Grid (Cycle Timer, Piece Counter, Temperature, DRAM)      |
| Component 4: Industrial Steppers with 16px Breathing Margins & 44px Touch Targets             |
| Component 5: ISA-18.2 Compliant Alarm Banner & Modal Dialog                                   |
| Component 6: Virtual Keypad with PIN Privacy Eye Mask Toggle                                  |
| Component 7: Real-Time I/O Diagnostics Grid with 14px Neon Green Status Lamps                 |
+-----------------------------------------------------------------------------------------------+
```

---

## Component 1: Apple HIG Industrial Header Bar

```cpp
#include "lvgl.h"
#include "haffler_ui_theme.h"

lv_obj_t* create_apple_header(lv_obj_t *parent, const char *machine_title) {
    // 1. Header Container (Height: 36px, Full Width: 480px)
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, 480, 36);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(header, 4, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Machine Title
    lv_obj_t *lbl_title = lv_label_create(header);
    lv_label_set_text(lbl_title, machine_title);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF0F6FC), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 10, 0);

    // 3. PackML State Pill Badge (e.g. "RUNNING" or "IDLE")
    lv_obj_t *badge = lv_obj_create(header);
    lv_obj_set_size(badge, 84, 22);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(0x059669), 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(badge, LV_ALIGN_CENTER, -40, 0);

    lv_obj_t *lbl_badge = lv_label_create(badge);
    lv_label_set_text(lbl_badge, "RUNNING");
    lv_obj_set_style_text_color(lbl_badge, lv_color_hex(0x34D399), 0);
    lv_obj_set_style_text_font(lbl_badge, &lv_font_montserrat_10, 0);
    lv_obj_center(lbl_badge);

    // 4. Real-Time Clock
    lv_obj_t *lbl_clock = lv_label_create(header);
    lv_label_set_text(lbl_clock, "14:28");
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_clock, LV_ALIGN_RIGHT_MID, -70, 0);

    // 5. Language Capsule Toggle (FA / EN / TR)
    lv_obj_t *btn_lang = lv_button_create(header);
    lv_obj_set_size(btn_lang, 56, 22);
    lv_obj_set_style_bg_color(btn_lang, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_border_color(btn_lang, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(btn_lang, 1, 0);
    lv_obj_set_style_radius(btn_lang, 6, 0);
    lv_obj_align(btn_lang, LV_ALIGN_RIGHT_MID, -6, 0);

    lv_obj_t *lbl_lang = lv_label_create(btn_lang);
    lv_label_set_text(lbl_lang, "EN");
    lv_obj_set_style_text_color(lbl_lang, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_lang, &lv_font_montserrat_10, 0);
    lv_obj_center(lbl_lang);

    return header;
}
```

---

## Component 2: Centered Hero Metric Display Card

```cpp
lv_obj_t* create_hero_metric_card(lv_obj_t *parent, const char *unit_label, lv_obj_t **out_label_val) {
    // 1. Hero Card Container (Full width 460px, Height: 120px)
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 460, 120);
    lv_obj_set_pos(card, 10, 42);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 0, 0); // MANDATORY: Pad 0 prevents downward digit drop
    lv_obj_set_style_shadow_width(card, 0, 0); // Zero software shadows for 50+ FPS
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Hero Value (Big bold digits centered mathematically)
    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, "124.5");
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_48, 0); // Or custom bold digit font
    lv_obj_align(lbl_val, LV_ALIGN_CENTER, 0, -10);
    if (out_label_val) *out_label_val = lbl_val;

    // 3. Unit Subtitle
    lv_obj_t *lbl_unit = lv_label_create(card);
    lv_label_set_text(lbl_unit, unit_label);
    lv_obj_set_style_text_color(lbl_unit, lv_color_hex(0x38BDF8), 0); // Apple Accent
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_unit, LV_ALIGN_CENTER, 0, 32);

    return card;
}
```

---

## Component 3: Balanced 2x2 Sub-Cards Grid

```cpp
void create_sub_cards_2x2(lv_obj_t *parent, lv_obj_t **lbls_out) {
    const char *titles[4] = {"CYCLE TIME", "PIECES", "TEMPERATURE", "FEED RATE"};
    const char *initial[4] = {"00:14.2", "1,248 pcs", "245.0 °C", "600 mm/min"};

    int xs[4] = {10, 245, 10, 245};
    int ys[4] = {168, 168, 224, 224};

    for (int i = 0; i < 4; ++i) {
        lv_obj_t *sub = lv_obj_create(parent);
        lv_obj_set_size(sub, 225, 50);
        lv_obj_set_pos(sub, xs[i], ys[i]);
        lv_obj_set_style_bg_color(sub, lv_color_hex(0x161B22), 0);
        lv_obj_set_style_border_color(sub, lv_color_hex(0x30363D), 0);
        lv_obj_set_style_border_width(sub, 1, 0);
        lv_obj_set_style_radius(sub, 10, 0);
        lv_obj_set_style_pad_all(sub, 6, 0);
        lv_obj_remove_flag(sub, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_t = lv_label_create(sub);
        lv_label_set_text(lbl_t, titles[i]);
        lv_obj_set_style_text_color(lbl_t, lv_color_hex(0x8B949E), 0);
        lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_10, 0);
        lv_obj_align(lbl_t, LV_ALIGN_TOP_LEFT, 6, 2);

        lv_obj_t *lbl_v = lv_label_create(sub);
        lv_label_set_text(lbl_v, initial[i]);
        lv_obj_set_style_text_color(lbl_v, lv_color_hex(0xF0F6FC), 0);
        lv_obj_set_style_text_font(lbl_v, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_v, LV_ALIGN_BOTTOM_LEFT, 6, -2);

        if (lbls_out) lbls_out[i] = lbl_v;
    }
}
```

---

## Component 4: Industrial Steppers with 16px Breathing Margins

```cpp
lv_obj_t* create_apple_stepper(lv_obj_t *parent, int y, const char *param_name, 
                               int val, int min_val, int max_val, 
                               lv_event_cb_t on_change) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 460, 48);
    lv_obj_set_pos(row, 10, y);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Minus Button (44px affordance)
    lv_obj_t *btn_m = lv_button_create(row);
    lv_obj_set_size(btn_m, 44, 40);
    lv_obj_set_style_bg_color(btn_m, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_radius(btn_m, 8, 0);
    lv_obj_align(btn_m, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_t *lbl_m = lv_label_create(btn_m);
    lv_label_set_text(lbl_m, "-");
    lv_obj_center(lbl_m);

    // Value Display Container
    lv_obj_t *val_box = lv_obj_create(row);
    lv_obj_set_size(val_box, 70, 40);
    lv_obj_set_style_bg_color(val_box, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_border_color(val_box, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(val_box, 1, 0);
    lv_obj_set_style_radius(val_box, 8, 0);
    lv_obj_align(val_box, LV_ALIGN_LEFT_MID, 52, 0);
    lv_obj_t *lbl_v = lv_label_create(val_box);
    lv_label_set_text_fmt(lbl_v, "%d", val);
    lv_obj_center(lbl_v);

    // Plus Button (44px affordance)
    lv_obj_t *btn_p = lv_button_create(row);
    lv_obj_set_size(btn_p, 44, 40);
    lv_obj_set_style_bg_color(btn_p, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_radius(btn_p, 8, 0);
    lv_obj_align(btn_p, LV_ALIGN_LEFT_MID, 126, 0);
    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_label_set_text(lbl_p, "+");
    lv_obj_center(lbl_p);

    // Title Label: MANDATORY 16px Breathing Margin after '+' button (126 + 44 + 16 = 186)
    lv_obj_t *lbl_title = lv_label_create(row);
    lv_label_set_text(lbl_title, param_name);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF0F6FC), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 186, 0);

    return row;
}
```

---

## Component 5: ISA-18.2 Compliant Alarm Banner & Modal

```cpp
lv_obj_t* create_alarm_modal(lv_obj_t *parent, const char *code, const char *desc, lv_event_cb_t on_ack) {
    // 1. Dimmed Backdrop
    lv_obj_t *modal = lv_obj_create(parent);
    lv_obj_set_size(modal, 480, 320);
    lv_obj_set_pos(modal, 0, 0);
    lv_obj_set_style_bg_color(modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_70, 0);
    lv_obj_move_foreground(modal);

    // 2. Alert Sheet Card
    lv_obj_t *card = lv_obj_create(modal);
    lv_obj_set_size(card, 380, 180);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xDC2626), 0); // Ruby Red
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 16, 0);

    // 3. Error Code
    lv_obj_t *lbl_c = lv_label_create(card);
    lv_label_set_text_fmt(lbl_c, "CRITICAL FAULT [%s]", code);
    lv_obj_set_style_text_color(lbl_c, lv_color_hex(0xEF4444), 0);
    lv_obj_align(lbl_c, LV_ALIGN_TOP_MID, 0, 10);

    // 4. Description
    lv_obj_t *lbl_d = lv_label_create(card);
    lv_label_set_text(lbl_d, desc);
    lv_obj_set_style_text_color(lbl_d, lv_color_hex(0xF0F6FC), 0);
    lv_obj_set_width(lbl_d, 340);
    lv_label_set_long_mode(lbl_d, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_d, LV_ALIGN_CENTER, 0, -5);

    // 5. Acknowledge Button
    lv_obj_t *btn_ack = lv_button_create(card);
    lv_obj_set_size(btn_ack, 200, 44);
    lv_obj_align(btn_ack, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_ack, lv_color_hex(0xDC2626), 0);
    lv_obj_set_style_radius(btn_ack, 10, 0);
    lv_obj_t *lbl_ack = lv_label_create(btn_ack);
    lv_label_set_text(lbl_ack, "ACKNOWLEDGE");
    lv_obj_center(lbl_ack);
    lv_obj_add_event_cb(btn_ack, on_ack, LV_EVENT_CLICKED, modal);

    return modal;
}
```
