/**
 * General tab "Breath" — Apple-like living status + clean control rows.
 * Tap = buzzer preview | Long-press = focus | Double-tap = day/night theme.
 */
#include "ui_breath.h"

#include "actions.h"
#include "fonts.h"
#include "screens.h"
#include "ui.h"
#include "variables.h"
#include "version.h"
#include "ui_vazir_fonts.h"

#include <stdio.h>
#include <string.h>

#define UI_BREATH_IDLE_MS     2400
#define UI_BREATH_FILL_MS     900
#define UI_BREATH_CORE        56
#define UI_BREATH_HALO        88

static lv_obj_t *s_halo;
static lv_obj_t *s_core;
static lv_obj_t *s_caption;
static lv_obj_t *s_hint;
static lv_obj_t *s_rows;
static bool s_focus;
static bool s_night;
static bool s_anim_running;
static int s_last_period_ms = -1;

static void ui_breath_set_halo_opa(void *obj, int32_t v)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)obj, (lv_opa_t)v, LV_PART_MAIN);
}

static void ui_breath_stop_anim(void)
{
    if (s_halo) lv_anim_delete(s_halo, ui_breath_set_halo_opa);
    s_anim_running = false;
    s_last_period_ms = -1;
}

static void ui_breath_start_anim(int period_ms)
{
    if (!s_halo) return;
    if (s_anim_running && s_last_period_ms == period_ms) return;

    ui_breath_stop_anim();
    s_last_period_ms = period_ms;
    s_anim_running = true;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_halo);
    lv_anim_set_values(&a, 35, 110);
    lv_anim_set_duration(&a, (uint32_t)period_ms);
    lv_anim_set_reverse_duration(&a, (uint32_t)period_ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, ui_breath_set_halo_opa);
    lv_anim_start(&a);
}

static void ui_breath_apply_theme(void)
{
    lv_color_t page = s_night ? lv_color_hex(0x12151a) : lv_color_hex(0xf5f5f5);
    lv_color_t row  = s_night ? lv_color_hex(0x1c2128) : lv_color_hex(0xffffff);
    lv_color_t txt  = s_night ? lv_color_hex(0xf2f2f7) : lv_color_hex(0x1c1c1e);
    lv_color_t sub  = s_night ? lv_color_hex(0x8e8e93) : lv_color_hex(0x8e8e93);
    lv_color_t line = s_night ? lv_color_hex(0x2c2c2e) : lv_color_hex(0xe5e5ea);
    lv_color_t accent = lv_color_hex(0x2196f3);

    if (objects.setting) {
        lv_obj_set_style_bg_color(objects.setting, page, LV_PART_MAIN);
    }
    if (objects.t_general) {
        lv_obj_set_style_bg_color(objects.t_general, page, LV_PART_MAIN);
    }
    if (s_rows) {
        lv_obj_set_style_bg_color(s_rows, row, LV_PART_MAIN);
        lv_obj_set_style_border_color(s_rows, line, LV_PART_MAIN);
    }
    if (objects.t_language) lv_obj_set_style_text_color(objects.t_language, txt, LV_PART_MAIN);
    if (objects.t_buzzer)   lv_obj_set_style_text_color(objects.t_buzzer, txt, LV_PART_MAIN);
    if (objects.t_touch)    lv_obj_set_style_text_color(objects.t_touch, txt, LV_PART_MAIN);
    if (s_caption) lv_obj_set_style_text_color(s_caption, txt, LV_PART_MAIN);
    if (s_hint) lv_obj_set_style_text_color(s_hint, sub, LV_PART_MAIN);
    if (s_core) {
        lv_obj_set_style_bg_color(s_core, accent, LV_PART_MAIN);
        lv_obj_set_style_border_color(s_core, accent, LV_PART_MAIN);
    }
    if (s_halo) {
        lv_obj_set_style_bg_color(s_halo, accent, LV_PART_MAIN);
    }
}

static void ui_breath_apply_focus(void)
{
    if (!s_rows) return;
    if (s_focus) {
        lv_obj_add_flag(s_rows, LV_OBJ_FLAG_HIDDEN);
        if (s_hint) lv_label_set_text(s_hint, "Focus");
        if (s_caption) lv_label_set_text(s_caption, "Hold to exit");
    } else {
        lv_obj_clear_flag(s_rows, LV_OBJ_FLAG_HIDDEN);
        if (s_hint) lv_label_set_text(s_hint, "Tap | Hold | Double");
        if (s_caption) {
            if (TimerValve1 || TimerValve2) lv_label_set_text(s_caption, "Filling");
            else lv_label_set_text(s_caption, "Ready");
        }
    }
}

static void ui_breath_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SHORT_CLICKED) {
        buzzer_key;
        return;
    }
    if (code == LV_EVENT_LONG_PRESSED) {
        s_focus = !s_focus;
        ui_breath_apply_focus();
        buzzer_key;
        return;
    }
    if (code == LV_EVENT_DOUBLE_CLICKED) {
        s_night = !s_night;
        ui_breath_apply_theme();
        buzzer_key;
        return;
    }
}

static void ui_breath_style_row_label(lv_obj_t *lab)
{
    if (!lab) return;
    lv_obj_set_style_text_font(lab, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
}

void ui_breath_create(lv_obj_t *general_tab)
{
    if (!general_tab) return;

    lv_obj_clear_flag(general_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(general_tab, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(general_tab, lv_color_hex(0xf5f5f5), LV_PART_MAIN);

    /* --- Breath cluster (center-top) --- */
    const int tab_w = 380; /* ~400 content minus tab bar */
    const int cx = (tab_w - UI_BREATH_HALO) / 2;
    const int cy = 4;

    s_halo = lv_obj_create(general_tab);
    lv_obj_set_pos(s_halo, cx, cy);
    lv_obj_set_size(s_halo, UI_BREATH_HALO, UI_BREATH_HALO);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_halo, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_halo, lv_color_hex(0x2196f3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_halo, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_halo, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_halo, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_halo, ui_breath_event, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(s_halo, ui_breath_event, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(s_halo, ui_breath_event, LV_EVENT_DOUBLE_CLICKED, NULL);

    s_core = lv_obj_create(s_halo);
    lv_obj_set_size(s_core, UI_BREATH_CORE, UI_BREATH_CORE);
    lv_obj_center(s_core);
    lv_obj_set_style_radius(s_core, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_core, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_core, lv_color_hex(0x2196f3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_core, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_core, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_core, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_core, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_core, LV_OBJ_FLAG_CLICKABLE);

    s_caption = lv_label_create(general_tab);
    lv_obj_set_width(s_caption, tab_w);
    lv_obj_set_pos(s_caption, 0, cy + UI_BREATH_HALO + 2);
    lv_obj_set_style_text_align(s_caption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_caption, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
    lv_label_set_text(s_caption, "Ready");

    s_hint = lv_label_create(general_tab);
    lv_obj_set_width(s_hint, tab_w);
    lv_obj_set_pos(s_hint, 0, cy + UI_BREATH_HALO + 24);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_hint, lv_color_hex(0x8e8e93), LV_PART_MAIN);
    lv_label_set_text(s_hint, "Tap | Hold | Double");

    /* Version whisper under hint */
    lv_obj_t *ver = lv_label_create(general_tab);
    lv_obj_set_width(ver, tab_w);
    lv_obj_set_pos(ver, 0, cy + UI_BREATH_HALO + 42);
    lv_obj_set_style_text_align(ver, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(ver, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(ver, lv_color_hex(0xc7c7cc), LV_PART_MAIN);
    lv_label_set_text(ver, FW_VERSION_LABEL);

    /* --- Grouped control card (iOS Settings rows) --- */
    s_rows = lv_obj_create(general_tab);
    lv_obj_set_pos(s_rows, 8, 148);
    lv_obj_set_size(s_rows, tab_w - 16, 92);
    lv_obj_set_style_radius(s_rows, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_rows, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_rows, lv_color_hex(0xe5e5ea), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_rows, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_rows, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_rows, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_shadow_width(s_rows, 0, LV_PART_MAIN);

    /* Reparent existing controls into the card */
    if (objects.t_language) {
        lv_obj_set_parent(objects.t_language, s_rows);
        lv_obj_set_pos(objects.t_language, 14, 6);
        lv_label_set_text(objects.t_language, "Language");
        ui_breath_style_row_label(objects.t_language);
    }
    if (objects.dd_language) {
        lv_obj_set_parent(objects.dd_language, s_rows);
        lv_obj_set_pos(objects.dd_language, 150, 2);
        lv_obj_set_size(objects.dd_language, 200, 28);
        lv_obj_set_style_radius(objects.dd_language, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(objects.dd_language, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(objects.dd_language, lv_color_hex(0xf2f2f7), LV_PART_MAIN);
    }

    lv_obj_t *div1 = lv_obj_create(s_rows);
    lv_obj_set_pos(div1, 14, 30);
    lv_obj_set_size(div1, tab_w - 44, 1);
    lv_obj_set_style_bg_color(div1, lv_color_hex(0xe5e5ea), LV_PART_MAIN);
    lv_obj_set_style_border_width(div1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(div1, 0, LV_PART_MAIN);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_CLICKABLE);

    if (objects.t_buzzer) {
        lv_obj_set_parent(objects.t_buzzer, s_rows);
        lv_obj_set_pos(objects.t_buzzer, 14, 36);
        lv_label_set_text(objects.t_buzzer, "Sound");
        ui_breath_style_row_label(objects.t_buzzer);
    }
    if (objects.sw_buzzer) {
        lv_obj_set_parent(objects.sw_buzzer, s_rows);
        lv_obj_set_pos(objects.sw_buzzer, 300, 34);
        lv_obj_set_size(objects.sw_buzzer, 48, 26);
    }

    lv_obj_t *div2 = lv_obj_create(s_rows);
    lv_obj_set_pos(div2, 14, 60);
    lv_obj_set_size(div2, tab_w - 44, 1);
    lv_obj_set_style_bg_color(div2, lv_color_hex(0xe5e5ea), LV_PART_MAIN);
    lv_obj_set_style_border_width(div2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(div2, 0, LV_PART_MAIN);
    lv_obj_clear_flag(div2, LV_OBJ_FLAG_CLICKABLE);

    if (objects.t_touch) {
        lv_obj_set_parent(objects.t_touch, s_rows);
        lv_obj_set_pos(objects.t_touch, 14, 66);
        lv_label_set_text(objects.t_touch, "Touch");
        ui_breath_style_row_label(objects.t_touch);
    }
    if (objects.sw_touch) {
        lv_obj_set_parent(objects.sw_touch, s_rows);
        lv_obj_set_pos(objects.sw_touch, 300, 64);
        lv_obj_set_size(objects.sw_touch, 48, 26);
    }

    s_focus = false;
    s_night = false;
    ui_breath_apply_theme();
    ui_breath_apply_focus();
    /* Anim starts in ui_breath_tick() only while Setting is active. */
}

void ui_breath_tick(void)
{
    if (!s_halo) return;
    if (ui_current_screen_id() != SCREEN_ID_SETTING) {
        ui_breath_stop_anim();
        return;
    }

    bool filling = (TimerValve1 != 0) || (TimerValve2 != 0);
    int period = filling ? UI_BREATH_FILL_MS : UI_BREATH_IDLE_MS;
    ui_breath_start_anim(period);

    if (!s_focus && s_caption) {
        const char *want = filling ? "Filling" : "Ready";
        if (strcmp(lv_label_get_text(s_caption), want) != 0) {
            lv_label_set_text(s_caption, want);
        }
    }
}

void ui_breath_on_leave(void)
{
    s_focus = false;
    ui_breath_apply_focus();
    ui_breath_stop_anim();
}
