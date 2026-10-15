#include "splash_screen.h"
#include <lvgl.h>
#include <stdio.h>

static lv_obj_t *s_splash_scr = NULL;
static lv_obj_t *s_bar = NULL;
static lv_obj_t *s_lbl_status = NULL;
static void (*s_complete_callback)(void) = NULL;

static void anim_ready_cb(lv_anim_t *a) {
    (void)a;
    if (s_complete_callback) {
        s_complete_callback();
    }
    if (s_splash_scr) {
        lv_obj_del_async(s_splash_scr);
        s_splash_scr = NULL;
    }
}

void splash_screen_show(const char *machine_name, const char *version_str, void (*on_complete_cb)(void)) {
    s_complete_callback = on_complete_cb;

    // 1. Create Base Splash Screen on Top
    s_splash_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_splash_scr, lv_color_hex(0x0F172A), 0); // Deep Slate
    lv_obj_clear_flag(s_splash_scr, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Machine Icon / Graphic Container (Circular glowing icon)
    lv_obj_t *icon_box = lv_obj_create(s_splash_scr);
    lv_obj_set_size(icon_box, 72, 72);
    lv_obj_align(icon_box, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_bg_color(icon_box, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_color(icon_box, lv_color_hex(0x007AFF), 0);
    lv_obj_set_style_border_width(icon_box, 2, 0);
    lv_obj_set_style_radius(icon_box, 36, 0); // Circle
    lv_obj_set_style_shadow_width(icon_box, 0, 0); // Zero software shadow

    lv_obj_t *lbl_icon = lv_label_create(icon_box);
    lv_label_set_text(lbl_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(lbl_icon, lv_color_hex(0x007AFF), 0);
    lv_obj_center(lbl_icon);

    // 3. HAFFLER Brand Typography
    lv_obj_t *lbl_brand = lv_label_create(s_splash_scr);
    lv_label_set_text(lbl_brand, "H A F F L E R");
    lv_obj_set_style_text_color(lbl_brand, lv_color_white(), 0);
    lv_obj_align_to(lbl_brand, icon_box, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);

    // 4. Machine Subtitle
    lv_obj_t *lbl_sub = lv_label_create(s_splash_scr);
    lv_label_set_text(lbl_sub, machine_name ? machine_name : "INDUSTRIAL AUTOMATION SYSTEM");
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0x94A3B8), 0); // Slate 400
    lv_obj_align_to(lbl_sub, lbl_brand, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    // 5. Loading Progress Bar (Apple HIG slim bar)
    s_bar = lv_bar_create(s_splash_scr);
    lv_obj_set_size(s_bar, 240, 4);
    lv_obj_align(s_bar, LV_ALIGN_CENTER, 0, 40);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x007AFF), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar, 2, LV_PART_MAIN | LV_PART_INDICATOR);

    // 6. Status message
    s_lbl_status = lv_label_create(s_splash_scr);
    lv_label_set_text(s_lbl_status, "Hardware Self-Test in Progress...");
    lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x64748B), 0);
    lv_obj_align_to(s_lbl_status, s_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // 7. Footer Version & Hardware Tag
    lv_obj_t *lbl_ver = lv_label_create(s_splash_scr);
    char ver_buf[48];
    snprintf(ver_buf, sizeof(ver_buf), "%s  |  ESP32-S3 N16R8", version_str ? version_str : "V.1.0.0");
    lv_label_set_text(lbl_ver, ver_buf);
    lv_obj_set_style_text_color(lbl_ver, lv_color_hex(0x475569), 0);
    lv_obj_align(lbl_ver, LV_ALIGN_BOTTOM_MID, 0, -12);

    // Load splash screen
    lv_scr_load(s_splash_scr);

    // Animate progress 0 -> 100% over 1800ms
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_bar);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_time(&a, 1800);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_bar_set_value);
    lv_anim_set_ready_cb(&a, anim_ready_cb);
    lv_anim_start(&a);
}

void splash_screen_set_progress(uint8_t percent, const char *status_msg) {
    if (s_bar) {
        lv_bar_set_value(s_bar, percent, LV_ANIM_ON);
    }
    if (s_lbl_status && status_msg) {
        lv_label_set_text(s_lbl_status, status_msg);
    }
}
