#include "haffler_ui_builder.h"
#include <stdio.h>
#include <string.h>

haffler_ui_ctx_t g_ui_ctx;

static void on_back_to_main_click(lv_event_t* e) {
    (void)e;
    haffler_ui_switch_screen(HAFFLER_SCREEN_MAIN);
}

static void on_nav_f4_click(lv_event_t* e) {
    (void)e;
    haffler_ui_switch_screen(HAFFLER_SCREEN_DIAGNOSTICS);
}

static void on_nav_f5_click(lv_event_t* e) {
    (void)e;
    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
}

void haffler_ui_builder_init(const char* machine_title, const char* hero_title, 
                             const char* hero_unit, float default_val) {
    memset(&g_ui_ctx, 0, sizeof(haffler_ui_ctx_t));

    // =========================================================================
    // 1. MAIN DASHBOARD SCREEN (480x320)
    // =========================================================================
    g_ui_ctx.scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_ui_ctx.scr_main, lv_color_hex(0x0A0C10), 0);
    lv_obj_clear_flag(g_ui_ctx.scr_main, LV_OBJ_FLAG_SCROLLABLE);

    // Header Bar (y: 0, h: 36)
    lv_obj_t* header = lv_obj_create(g_ui_ctx.scr_main);
    lv_obj_set_size(header, 480, 36);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 12, 0);
    lv_obj_set_style_pad_ver(header, 4, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    g_ui_ctx.lbl_title = lv_label_create(header);
    lv_label_set_text(g_ui_ctx.lbl_title, machine_title ? machine_title : "HAFFLER PLC");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_title, lv_color_hex(0xF0F6FC), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(g_ui_ctx.lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    g_ui_ctx.badge_daylock = ui_create_pill_badge(header, "Lease: 30D", 0xD97706, 0xFFFBEB);
    lv_obj_align(g_ui_ctx.badge_daylock, LV_ALIGN_RIGHT_MID, -90, 0);

    g_ui_ctx.badge_state = ui_create_pill_badge(header, "READY", 0x16A34A, 0xDCFCE7);
    lv_obj_align(g_ui_ctx.badge_state, LV_ALIGN_RIGHT_MID, 0, 0);

    // Hero Metric Center Card (y: 44, w: 460, h: 228)
    lv_obj_t* hero_card = ui_create_apple_card(g_ui_ctx.scr_main, 10, 44, 460, 228, hero_title);
    
    g_ui_ctx.hero_metric = ui_create_hero_metric(hero_card, 440, 180, hero_unit, &lv_font_montserrat_48);
    lv_obj_center(g_ui_ctx.hero_metric.container);
    char init_buf[32];
    snprintf(init_buf, sizeof(init_buf), "%.1f", default_val);
    ui_update_hero_metric(&g_ui_ctx.hero_metric, init_buf);

    // Footer Navigation Bar (y: 280, h: 40)
    lv_obj_t* footer = lv_obj_create(g_ui_ctx.scr_main);
    lv_obj_set_size(footer, 480, 40);
    lv_obj_set_pos(footer, 0, 280);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 2, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    const char* f_labels[] = {"[F1] Manual", "[F2] Recipe", "[F3] Zero", "[F4] Diag", "[F5] Setup"};
    lv_obj_t** f_btns[] = {&g_ui_ctx.footer_f1, &g_ui_ctx.footer_f2, &g_ui_ctx.footer_f3, 
                           &g_ui_ctx.footer_f4, &g_ui_ctx.footer_f5};

    for (int i = 0; i < 5; ++i) {
        lv_obj_t* btn = lv_btn_create(footer);
        lv_obj_set_size(btn, 90, 34);
        lv_obj_set_pos(btn, 5 + (i * 95), 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x21262D), 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, f_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);

        *f_btns[i] = btn;
    }
    lv_obj_add_event_cb(g_ui_ctx.footer_f4, on_nav_f4_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(g_ui_ctx.footer_f5, on_nav_f5_click, LV_EVENT_CLICKED, NULL);

    // =========================================================================
    // 2. SETTINGS SCREEN (F5)
    // =========================================================================
    g_ui_ctx.scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_ui_ctx.scr_settings, lv_color_hex(0x0A0C10), 0);
    lv_obj_clear_flag(g_ui_ctx.scr_settings, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* set_header = lv_obj_create(g_ui_ctx.scr_settings);
    lv_obj_set_size(set_header, 480, 36);
    lv_obj_set_pos(set_header, 0, 0);
    lv_obj_set_style_bg_color(set_header, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(set_header, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(set_header, 1, 0);
    lv_obj_clear_flag(set_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_set_title = lv_label_create(set_header);
    lv_label_set_text(lbl_set_title, "Machine Settings (F5)");
    lv_obj_set_style_text_color(lbl_set_title, lv_color_hex(0xF0F6FC), 0);
    lv_obj_align(lbl_set_title, LV_ALIGN_LEFT_MID, 12, 0);

    lv_obj_t* btn_back = lv_btn_create(set_header);
    lv_obj_set_size(btn_back, 80, 28);
    lv_obj_align(btn_back, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x238636), 0);
    lv_obj_add_event_cb(btn_back, on_back_to_main_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "[ Back ]");
    lv_obj_center(lbl_back);

    // Add standard steppers
    ui_create_stepper(g_ui_ctx.scr_settings, 46, "Target Setpoint", hero_unit, NULL, NULL);
    ui_create_stepper(g_ui_ctx.scr_settings, 102, "Clamp Hold Delay", "s", NULL, NULL);
    ui_create_stepper(g_ui_ctx.scr_settings, 158, "Feed Travel Speed", "%", NULL, NULL);
    ui_create_stepper(g_ui_ctx.scr_settings, 214, "Cooling / Squeeze Time", "s", NULL, NULL);

    // =========================================================================
    // 3. HARDWARE DIAGNOSTICS SCREEN (F4)
    // =========================================================================
    g_ui_ctx.scr_diagnostics = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_ui_ctx.scr_diagnostics, lv_color_hex(0x0A0C10), 0);
    lv_obj_clear_flag(g_ui_ctx.scr_diagnostics, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* diag_header = lv_obj_create(g_ui_ctx.scr_diagnostics);
    lv_obj_set_size(diag_header, 480, 36);
    lv_obj_set_pos(diag_header, 0, 0);
    lv_obj_set_style_bg_color(diag_header, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(diag_header, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(diag_header, 1, 0);
    lv_obj_clear_flag(diag_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_diag_title = lv_label_create(diag_header);
    lv_label_set_text(lbl_diag_title, "24V Hardware Diagnostics (F4)");
    lv_obj_set_style_text_color(lbl_diag_title, lv_color_hex(0xF0F6FC), 0);
    lv_obj_align(lbl_diag_title, LV_ALIGN_LEFT_MID, 12, 0);

    lv_obj_t* btn_diag_back = lv_btn_create(diag_header);
    lv_obj_set_size(btn_diag_back, 80, 28);
    lv_obj_align(btn_diag_back, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(btn_diag_back, lv_color_hex(0x238636), 0);
    lv_obj_add_event_cb(btn_diag_back, on_back_to_main_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_diag_back = lv_label_create(btn_diag_back);
    lv_label_set_text(lbl_diag_back, "[ Back ]");
    lv_obj_center(lbl_diag_back);

    // Inputs Card (Left: w: 225, h: 270)
    lv_obj_t* in_card = ui_create_apple_card(g_ui_ctx.scr_diagnostics, 10, 42, 225, 270, "Digital Inputs (I0..I7)");
    for (int i = 0; i < 8; ++i) {
        lv_obj_t* lamp = lv_obj_create(in_card);
        lv_obj_set_size(lamp, 14, 14);
        lv_obj_set_pos(lamp, 6, 24 + (i * 28));
        lv_obj_set_style_radius(lamp, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(lamp, lv_color_hex(0x21262D), 0);
        lv_obj_set_style_border_color(lamp, lv_color_hex(0x484F58), 0);
        lv_obj_set_style_border_width(lamp, 1, 0);
        lv_obj_clear_flag(lamp, LV_OBJ_FLAG_SCROLLABLE);
        g_ui_ctx.in_lamps[i] = lamp;

        lv_obj_t* lbl = lv_label_create(in_card);
        char in_name[32];
        snprintf(in_name, sizeof(in_name), "Input I%d", i);
        lv_label_set_text(lbl, in_name);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xC9D1D9), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(lbl, 28, 22 + (i * 28));
    }

    // Outputs Card (Right: w: 225, h: 270)
    lv_obj_t* out_card = ui_create_apple_card(g_ui_ctx.scr_diagnostics, 245, 42, 225, 270, "Relay Outputs (Q0..Q9)");
    for (int i = 0; i < 10; ++i) {
        lv_obj_t* lamp = lv_obj_create(out_card);
        lv_obj_set_size(lamp, 14, 14);
        lv_obj_set_pos(lamp, 6, 22 + (i * 23));
        lv_obj_set_style_radius(lamp, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(lamp, lv_color_hex(0x21262D), 0);
        lv_obj_set_style_border_color(lamp, lv_color_hex(0x484F58), 0);
        lv_obj_set_style_border_width(lamp, 1, 0);
        lv_obj_clear_flag(lamp, LV_OBJ_FLAG_SCROLLABLE);
        g_ui_ctx.out_lamps[i] = lamp;

        lv_obj_t* lbl = lv_label_create(out_card);
        char out_name[32];
        snprintf(out_name, sizeof(out_name), "Relay Q%d", i);
        lv_label_set_text(lbl, out_name);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xC9D1D9), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(lbl, 28, 20 + (i * 23));
    }

    g_ui_ctx.current_screen = HAFFLER_SCREEN_MAIN;
    lv_scr_load(g_ui_ctx.scr_main);
}

void haffler_ui_switch_screen(haffler_screen_id_t target_screen) {
    lv_obj_t* target = g_ui_ctx.scr_main;
    switch (target_screen) {
        case HAFFLER_SCREEN_MAIN:        target = g_ui_ctx.scr_main; break;
        case HAFFLER_SCREEN_SETTINGS:    target = g_ui_ctx.scr_settings; break;
        case HAFFLER_SCREEN_DIAGNOSTICS: target = g_ui_ctx.scr_diagnostics; break;
        default:                         target = g_ui_ctx.scr_main; break;
    }
    if (target) {
        g_ui_ctx.current_screen = target_screen;
        lv_scr_load(target);
    }
}

void haffler_ui_set_hero_value(float value, uint8_t decimals) {
    char buf[32];
    if (decimals == 0) {
        snprintf(buf, sizeof(buf), "%.0f", value);
    } else if (decimals == 1) {
        snprintf(buf, sizeof(buf), "%.1f", value);
    } else {
        snprintf(buf, sizeof(buf), "%.2f", value);
    }
    ui_update_hero_metric(&g_ui_ctx.hero_metric, buf);
}

void haffler_ui_set_machine_state(const char* state_str, uint32_t bg_hex) {
    if (g_ui_ctx.badge_state && state_str) {
        lv_obj_t* lbl = lv_obj_get_child(g_ui_ctx.badge_state, 0);
        if (lbl) lv_label_set_text(lbl, state_str);
        lv_obj_set_style_bg_color(g_ui_ctx.badge_state, lv_color_hex(bg_hex), 0);
    }
}

void haffler_ui_set_day_lock_days(uint8_t remaining_days) {
    if (g_ui_ctx.badge_daylock) {
        lv_obj_t* lbl = lv_obj_get_child(g_ui_ctx.badge_daylock, 0);
        if (lbl) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Lease: %dD", remaining_days);
            lv_label_set_text(lbl, buf);
        }
        uint32_t color = (remaining_days <= 3) ? 0xDC2626 : ((remaining_days <= 7) ? 0xD97706 : 0x2563EB);
        lv_obj_set_style_bg_color(g_ui_ctx.badge_daylock, lv_color_hex(color), 0);
    }
}

void haffler_ui_set_input_lamp(uint8_t input_idx, bool active) {
    if (input_idx < 8 && g_ui_ctx.in_lamps[input_idx]) {
        uint32_t col = active ? 0x22C55E : 0x21262D;
        lv_obj_set_style_bg_color(g_ui_ctx.in_lamps[input_idx], lv_color_hex(col), 0);
    }
}

void haffler_ui_set_output_lamp(uint8_t output_idx, bool active) {
    if (output_idx < 10 && g_ui_ctx.out_lamps[output_idx]) {
        uint32_t col = active ? 0x007AFF : 0x21262D;
        lv_obj_set_style_bg_color(g_ui_ctx.out_lamps[output_idx], lv_color_hex(col), 0);
    }
}
