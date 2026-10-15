#include "haffler_ui_builder.h"
#include "machine_fsm.h"
#include "haffler_recipes.h"
#include "haffler_ui_lang.h"
#include "haffler_pins.h"
#include "ui_fonts.h"
#include <stdio.h>
#include <string.h>

#ifndef LV_SYMBOL_KEY
#define LV_SYMBOL_KEY LV_SYMBOL_KEYBOARD
#endif

haffler_ui_ctx_t g_ui_ctx;

extern void relay_set(uint8_t idx, bool active);
extern bool relay_get(uint8_t idx);

static char s_dl_input_buf[16] = "";

// =============================================================================
// NAVIGATION & EVENT HANDLERS
// =============================================================================
static void on_back_to_main_click(lv_event_t* e) {
    (void)e;
    haffler_ui_switch_screen(HAFFLER_SCREEN_MAIN);
}

static void on_open_settings_click(lv_event_t* e) {
    (void)e;
    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
}

static void on_start_click(lv_event_t* e) {
    (void)e;
    g_machine_fsm.startCycle();
}

static void on_stop_click(lv_event_t* e) {
    (void)e;
    g_machine_fsm.stopCycle();
}

static void on_override_click(lv_event_t* e) {
    (void)e;
    g_machine_fsm.overrideTempWait();
}

static void on_save_config_click(lv_event_t* e) {
    (void)e;
    g_machine_fsm.saveConfigToNVS();
    haffler_ui_mark_settings_dirty(false);
    if (g_ui_ctx.lbl_save_btn) {
        lv_label_set_text(g_ui_ctx.lbl_save_btn, LV_SYMBOL_OK " ذخیره شد");
    }
}

static void on_tab_btn_click(lv_event_t* e) {
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < 5) {
        haffler_ui_set_settings_tab((uint8_t)idx);
    }
}

// Recipe Cycling in Tab 0
static void on_cycle_recipe_click(lv_event_t* e) {
    (void)e;
    uint8_t next_idx = (g_recipes.getActiveIndex() + 1) % 5;
    g_recipes.setActive(next_idx);
    HafflerRecipe* r = g_recipes.getActive();
    if (r) {
        g_machine_fsm.setMixTimeSec((uint32_t)r->params[0]);
        g_machine_fsm.setDosingMode((uint8_t)r->params[1]);
        g_machine_fsm.setWaterTargetLiters(r->params[2]);
        g_machine_fsm.setWaterTargetTemp(r->params[3]);
        haffler_ui_set_recipe_name(r->name);
    }
    haffler_ui_mark_settings_dirty(true);
    haffler_ui_refresh_steppers();
}

// Steppers for Recipe / Process (Tab 0)
static void on_step_mix_time_minus(lv_event_t* e) {
    (void)e;
    uint32_t val = g_machine_fsm.getConfig().mix_time_sec;
    if (val > 30) {
        g_machine_fsm.setMixTimeSec(val - 30);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_mix_time_plus(lv_event_t* e) {
    (void)e;
    uint32_t val = g_machine_fsm.getConfig().mix_time_sec;
    if (val < 1800) {
        g_machine_fsm.setMixTimeSec(val + 30);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_water_liters_minus(lv_event_t* e) {
    (void)e;
    float cur = g_machine_fsm.getConfig().water_target_liters;
    if (cur > 2.0f) {
        g_machine_fsm.setWaterTargetLiters(cur - 1.0f);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_water_liters_plus(lv_event_t* e) {
    (void)e;
    float cur = g_machine_fsm.getConfig().water_target_liters;
    if (cur < 80.0f) {
        g_machine_fsm.setWaterTargetLiters(cur + 1.0f);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_temp_target_minus(lv_event_t* e) {
    (void)e;
    float cur = g_machine_fsm.getConfig().water_target_temp_c;
    if (cur > 10.0f) {
        g_machine_fsm.setWaterTargetTemp(cur - 0.5f);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_temp_target_plus(lv_event_t* e) {
    (void)e;
    float cur = g_machine_fsm.getConfig().water_target_temp_c;
    if (cur < 45.0f) {
        g_machine_fsm.setWaterTargetTemp(cur + 0.5f);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

// Steppers for Calibration / Hardware (Tab 2)
static void on_step_flowrate_minus(lv_event_t* e) {
    (void)e;
    float cur = g_machine_fsm.getConfig().flow_rate_l_per_min;
    if (cur > 0.5f) {
        g_machine_fsm.setFlowRateLPerMin(cur - 0.2f);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_flowrate_plus(lv_event_t* e) {
    (void)e;
    float cur = g_machine_fsm.getConfig().flow_rate_l_per_min;
    if (cur < 20.0f) {
        g_machine_fsm.setFlowRateLPerMin(cur + 0.2f);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_brake_minus(lv_event_t* e) {
    (void)e;
    uint32_t cur = g_machine_fsm.getConfig().brake_on_time_sec;
    if (cur > 1) {
        g_machine_fsm.setBowlBrakeParams(true, cur - 1, 2);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_brake_plus(lv_event_t* e) {
    (void)e;
    uint32_t cur = g_machine_fsm.getConfig().brake_on_time_sec;
    if (cur < 30) {
        g_machine_fsm.setBowlBrakeParams(true, cur + 1, 2);
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_flour_mode_toggle(lv_event_t* e) {
    (void)e;
    uint8_t cur = g_machine_fsm.getConfig().flour_mode;
    g_machine_fsm.setFlourMode(cur == 1 ? 0 : 1);
    haffler_ui_mark_settings_dirty(true);
    haffler_ui_refresh_steppers();
}

static void on_step_pretemp_minus(lv_event_t* e) {
    (void)e;
    uint32_t cur = g_machine_fsm.getConfig().pre_temp_lead_min;
    if (cur > 5) {
        g_machine_fsm.setPreTempLeadMin((uint8_t)(cur - 5));
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

static void on_step_pretemp_plus(lv_event_t* e) {
    (void)e;
    uint32_t cur = g_machine_fsm.getConfig().pre_temp_lead_min;
    if (cur < 120) {
        g_machine_fsm.setPreTempLeadMin((uint8_t)(cur + 5));
        haffler_ui_mark_settings_dirty(true);
        haffler_ui_refresh_steppers();
    }
}

// Steppers for Shift Timings & RTC Clock (Tab 1)
static void on_rtc_hour_minus(lv_event_t* e) {
    (void)e;
    g_machine_fsm.adjustRTCHour(-1);
    haffler_ui_refresh_steppers();
}

static void on_rtc_hour_plus(lv_event_t* e) {
    (void)e;
    g_machine_fsm.adjustRTCHour(+1);
    haffler_ui_refresh_steppers();
}

static void on_rtc_min_minus(lv_event_t* e) {
    (void)e;
    g_machine_fsm.adjustRTCMinute(-1);
    haffler_ui_refresh_steppers();
}

static void on_rtc_min_plus(lv_event_t* e) {
    (void)e;
    g_machine_fsm.adjustRTCMinute(+1);
    haffler_ui_refresh_steppers();
}

static void on_shift1_hour_plus(lv_event_t* e) {
    (void)e;
    uint8_t h = (g_machine_fsm.getConfig().shifts[0].hour + 1) % 24;
    g_machine_fsm.setShiftSchedule(0, true, h, g_machine_fsm.getConfig().shifts[0].minute);
    haffler_ui_mark_settings_dirty(true);
    haffler_ui_refresh_steppers();
}

static void on_shift2_hour_plus(lv_event_t* e) {
    (void)e;
    uint8_t h = (g_machine_fsm.getConfig().shifts[1].hour + 1) % 24;
    g_machine_fsm.setShiftSchedule(1, true, h, g_machine_fsm.getConfig().shifts[1].minute);
    haffler_ui_mark_settings_dirty(true);
    haffler_ui_refresh_steppers();
}

static void on_shift3_hour_plus(lv_event_t* e) {
    (void)e;
    uint8_t h = (g_machine_fsm.getConfig().shifts[2].hour + 1) % 24;
    g_machine_fsm.setShiftSchedule(2, true, h, g_machine_fsm.getConfig().shifts[2].minute);
    haffler_ui_mark_settings_dirty(true);
    haffler_ui_refresh_steppers();
}

// Day-Lock Keypad Callbacks (Tab 4)
static void update_dl_display(void) {
    if (g_ui_ctx.lbl_dl_input) {
        char disp[32];
        if (s_dl_input_buf[0] == '\0') {
            snprintf(disp, sizeof(disp), "[ _ _ _ _ ]");
        } else {
            snprintf(disp, sizeof(disp), "[ %s ]", s_dl_input_buf);
        }
        lv_label_set_text(g_ui_ctx.lbl_dl_input, disp);
    }
}

static void on_dl_digit_click(lv_event_t* e) {
    intptr_t digit = (intptr_t)lv_event_get_user_data(e);
    size_t len = strlen(s_dl_input_buf);
    if (len < 6) {
        s_dl_input_buf[len] = '0' + (char)digit;
        s_dl_input_buf[len + 1] = '\0';
        update_dl_display();
    }
}

static void on_dl_backspace_click(lv_event_t* e) {
    (void)e;
    size_t len = strlen(s_dl_input_buf);
    if (len > 0) {
        s_dl_input_buf[len - 1] = '\0';
        update_dl_display();
    }
}

static void on_dl_confirm_click(lv_event_t* e) {
    (void)e;
    size_t len = strlen(s_dl_input_buf);
    if (len == 0) return;

    bool ok = false;
    if (len == 4) {
        uint16_t key = (uint16_t)atoi(s_dl_input_buf);
        if (g_machine_fsm.verifyDayLockUnlock(key)) {
            ok = true;
        } else if (g_machine_fsm.applyDayLockAdminBypass(s_dl_input_buf)) {
            ok = true;
        }
    } else if (len == 6) {
        if (g_machine_fsm.applyDayLockEmergency(s_dl_input_buf)) {
            ok = true;
        }
    }

    if (ok) {
        if (g_ui_ctx.lbl_dl_status) {
            lv_label_set_text(g_ui_ctx.lbl_dl_status, "قفل باز شد ✔");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_status, lv_color_hex(0x22C55E), 0);
        }
        s_dl_input_buf[0] = '\0';
        update_dl_display();
    } else {
        if (g_ui_ctx.lbl_dl_status) {
            lv_label_set_text(g_ui_ctx.lbl_dl_status, "کلید اشتباه است ✖");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_status, lv_color_hex(0xDC2626), 0);
        }
    }
}

// Diag Relay toggle callback (Tab 3)
static void on_diag_relay_toggle(lv_event_t* e) {
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    if (g_machine_fsm.getState() == STATE_IDLE && idx >= 0 && idx < 10) {
        relay_set((uint8_t)idx, !relay_get((uint8_t)idx));
    }
}

// =============================================================================
// SCREEN BUILDER INITIALIZATION
// =============================================================================
void haffler_ui_builder_init(const char* machine_title, const char* hero_title, 
                             const char* hero_unit, float default_val) {
    (void)machine_title;
    (void)hero_title;
    (void)hero_unit;
    memset(&g_ui_ctx, 0, sizeof(haffler_ui_ctx_t));

    // =========================================================================
    // 1. MAIN DASHBOARD SCREEN (480x320)
    // =========================================================================
    g_ui_ctx.scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_ui_ctx.scr_main, lv_color_hex(0xF8FAFC), 0);
    lv_obj_clear_flag(g_ui_ctx.scr_main, LV_OBJ_FLAG_SCROLLABLE);

    // Header Bar (y: 0, h: 38, w: 480)
    lv_obj_t* header = lv_obj_create(g_ui_ctx.scr_main);
    lv_obj_set_size(header, 480, 38);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 10, 0);
    lv_obj_set_style_pad_ver(header, 4, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Header Clock (Left)
    g_ui_ctx.lbl_clock = lv_label_create(header);
    lv_label_set_text(g_ui_ctx.lbl_clock, "04:00:00");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_clock, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_clock, &lv_font_montserrat_14, 0);
    lv_obj_align(g_ui_ctx.lbl_clock, LV_ALIGN_LEFT_MID, 4, 0);

    // Recipe Badge (Center-Left)
    g_ui_ctx.badge_recipe = ui_create_pill_badge(header, LV_SYMBOL_LIST " بربری", 0x4338CA, 0xFFFFFF);
    lv_obj_align(g_ui_ctx.badge_recipe, LV_ALIGN_CENTER, -60, 0);

    // State Badge (Center-Right)
    g_ui_ctx.badge_state = ui_create_pill_badge(header, LV_SYMBOL_OK " آماده", 0x16A34A, 0xFFFFFF);
    lv_obj_align(g_ui_ctx.badge_state, LV_ALIGN_CENTER, 60, 0);

    // Settings Hub Button (Top-Right)
    g_ui_ctx.btn_header_settings = lv_btn_create(header);
    lv_obj_set_size(g_ui_ctx.btn_header_settings, 104, 28);
    lv_obj_align(g_ui_ctx.btn_header_settings, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(g_ui_ctx.btn_header_settings, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_color(g_ui_ctx.btn_header_settings, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(g_ui_ctx.btn_header_settings, 1, 0);
    lv_obj_set_style_radius(g_ui_ctx.btn_header_settings, 6, 0);
    lv_obj_set_style_shadow_width(g_ui_ctx.btn_header_settings, 0, 0);
    lv_obj_add_event_cb(g_ui_ctx.btn_header_settings, on_open_settings_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t* lbl_set_btn = lv_label_create(g_ui_ctx.btn_header_settings);
    lv_label_set_text(lbl_set_btn, LV_SYMBOL_SETTINGS " تنظیمات");
    lv_obj_set_style_text_color(lbl_set_btn, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(lbl_set_btn, &ui_font_Dana_Bold13, 0);
    lv_obj_center(lbl_set_btn);

    // =========================================================================
    // GRAPHIC PROCESS PIPELINE (5 STAGES) (y: 42, w: 460, h: 48)
    // =========================================================================
    g_ui_ctx.pipeline_container = lv_obj_create(g_ui_ctx.scr_main);
    lv_obj_set_pos(g_ui_ctx.pipeline_container, 10, 42);
    lv_obj_set_size(g_ui_ctx.pipeline_container, 460, 48);
    lv_obj_set_style_bg_color(g_ui_ctx.pipeline_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(g_ui_ctx.pipeline_container, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(g_ui_ctx.pipeline_container, 1, 0);
    lv_obj_set_style_radius(g_ui_ctx.pipeline_container, 8, 0);
    lv_obj_set_style_shadow_width(g_ui_ctx.pipeline_container, 0, 0);
    lv_obj_set_style_pad_all(g_ui_ctx.pipeline_container, 4, 0);
    lv_obj_clear_flag(g_ui_ctx.pipeline_container, LV_OBJ_FLAG_SCROLLABLE);

    const char* stage_titles[5] = {
        "۱. دمای آب", "۲. آبگیری", "۳. آرد", "۴. میکس خمیر", "۵. پایان پخت"
    };

    for (int i = 0; i < 5; ++i) {
        g_ui_ctx.step_capsules[i] = lv_obj_create(g_ui_ctx.pipeline_container);
        lv_obj_set_size(g_ui_ctx.step_capsules[i], 86, 24);
        lv_obj_set_pos(g_ui_ctx.step_capsules[i], 4 + (i * 90), 2);
        lv_obj_set_style_bg_color(g_ui_ctx.step_capsules[i], lv_color_hex(0xF1F5F9), 0);
        lv_obj_set_style_border_color(g_ui_ctx.step_capsules[i], lv_color_hex(0xCBD5E1), 0);
        lv_obj_set_style_border_width(g_ui_ctx.step_capsules[i], 1, 0);
        lv_obj_set_style_radius(g_ui_ctx.step_capsules[i], 12, 0);
        lv_obj_set_style_pad_all(g_ui_ctx.step_capsules[i], 0, 0);
        lv_obj_clear_flag(g_ui_ctx.step_capsules[i], LV_OBJ_FLAG_SCROLLABLE);

        g_ui_ctx.lbl_step_names[i] = lv_label_create(g_ui_ctx.step_capsules[i]);
        lv_label_set_text(g_ui_ctx.lbl_step_names[i], stage_titles[i]);
        lv_obj_set_style_text_color(g_ui_ctx.lbl_step_names[i], lv_color_hex(0x475569), 0);
        lv_obj_set_style_text_font(g_ui_ctx.lbl_step_names[i], &ui_font_Dana_Bold13, 0);
        lv_obj_center(g_ui_ctx.lbl_step_names[i]);
    }

    g_ui_ctx.lbl_next_stage_hint = lv_label_create(g_ui_ctx.pipeline_container);
    lv_label_set_text(g_ui_ctx.lbl_next_stage_hint, "مرحله بعد: آبگیری خمیر (۲۰.۰ لیتر)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_next_stage_hint, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_next_stage_hint, &ui_font_Dana_Bold13, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_next_stage_hint, 8, 28);

    // =========================================================================
    // HERO METRIC CARD WITH PROGRESS BAR (y: 94, w: 256, h: 164)
    // =========================================================================
    lv_obj_t* hero_card = ui_create_apple_card(g_ui_ctx.scr_main, 10, 94, 256, 164, "زمان باقیمانده میکس");
    g_ui_ctx.hero_metric = ui_create_hero_metric(hero_card, 236, 76, "ثانیه", &lv_font_montserrat_48);
    lv_obj_set_pos(g_ui_ctx.hero_metric.container, 10, 24);
    char init_buf[32];
    snprintf(init_buf, sizeof(init_buf), "%.0f", default_val);
    ui_update_hero_metric(&g_ui_ctx.hero_metric, init_buf);

    // Mix Progress Bar (x: 12, y: 104, w: 232, h: 10)
    g_ui_ctx.bar_mix_progress = lv_bar_create(hero_card);
    lv_obj_set_size(g_ui_ctx.bar_mix_progress, 232, 10);
    lv_obj_set_pos(g_ui_ctx.bar_mix_progress, 12, 106);
    lv_bar_set_range(g_ui_ctx.bar_mix_progress, 0, 100);
    lv_bar_set_value(g_ui_ctx.bar_mix_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_ui_ctx.bar_mix_progress, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_ui_ctx.bar_mix_progress, lv_color_hex(0x0284C7), LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_ui_ctx.bar_mix_progress, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(g_ui_ctx.bar_mix_progress, 5, LV_PART_INDICATOR);

    // Progress percentage label
    g_ui_ctx.lbl_mix_percent = lv_label_create(hero_card);
    lv_label_set_text(g_ui_ctx.lbl_mix_percent, "پیشرفت میکس: ۰٪");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_mix_percent, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_mix_percent, &ui_font_Dana_Bold13, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_mix_percent, 12, 122);

    // =========================================================================
    // LIVE PROCESS TELEMETRY CARD (y: 94, x: 272, w: 198, h: 164)
    // =========================================================================
    lv_obj_t* info_card = ui_create_apple_card(g_ui_ctx.scr_main, 272, 94, 198, 164, "وضعیت فرآیند");

    g_ui_ctx.lbl_water_info = lv_label_create(info_card);
    lv_label_set_text(g_ui_ctx.lbl_water_info, LV_SYMBOL_TINT " آب: ۲۰.۰ لیتر (دبی ۳.۰)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_water_info, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_water_info, &ui_font_Dana_Bold13, 0);
    lv_obj_set_size(g_ui_ctx.lbl_water_info, 184, 18);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_water_info, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_water_info, 4, 18);

    g_ui_ctx.lbl_temp_info = lv_label_create(info_card);
    lv_label_set_text(g_ui_ctx.lbl_temp_info, LV_SYMBOL_TINT " دمای آب: ۲۴.۰°C (آماده)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_temp_info, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_temp_info, &ui_font_Dana_Bold13, 0);
    lv_obj_set_size(g_ui_ctx.lbl_temp_info, 184, 18);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_temp_info, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_temp_info, 4, 38);

    g_ui_ctx.lbl_flour_info = lv_label_create(info_card);
    lv_label_set_text(g_ui_ctx.lbl_flour_info, LV_SYMBOL_DOWNLOAD " آرد: دستی (آماده)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_flour_info, lv_color_hex(0x334155), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_flour_info, &ui_font_Dana_Bold13, 0);
    lv_obj_set_size(g_ui_ctx.lbl_flour_info, 184, 18);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_flour_info, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_flour_info, 4, 58);

    g_ui_ctx.lbl_brake_info = lv_label_create(info_card);
    lv_label_set_text(g_ui_ctx.lbl_brake_info, LV_SYMBOL_LOOP " ترمز کاسه: آزاد");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_brake_info, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_brake_info, &ui_font_Dana_Bold13, 0);
    lv_obj_set_size(g_ui_ctx.lbl_brake_info, 184, 18);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_brake_info, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_brake_info, 4, 78);

    g_ui_ctx.lbl_guard_info = lv_label_create(info_card);
    lv_label_set_text(g_ui_ctx.lbl_guard_info, LV_SYMBOL_SHUFFLE " توری: بسته (ایمن)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_guard_info, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_guard_info, &ui_font_Dana_Bold13, 0);
    lv_obj_set_size(g_ui_ctx.lbl_guard_info, 184, 18);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_guard_info, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_guard_info, 4, 98);

    g_ui_ctx.lbl_batch_info = lv_label_create(info_card);
    lv_label_set_text(g_ui_ctx.lbl_batch_info, LV_SYMBOL_LIST " تعداد پخت: ۰ شیفت");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_batch_info, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_batch_info, &ui_font_Dana_Bold13, 0);
    lv_obj_set_size(g_ui_ctx.lbl_batch_info, 184, 18);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_batch_info, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_batch_info, 4, 118);

    // Override Button
    g_ui_ctx.btn_override = lv_btn_create(info_card);
    lv_obj_set_size(g_ui_ctx.btn_override, 184, 22);
    lv_obj_set_pos(g_ui_ctx.btn_override, 4, 138);
    lv_obj_set_style_bg_color(g_ui_ctx.btn_override, lv_color_hex(0xEA580C), 0);
    lv_obj_set_style_radius(g_ui_ctx.btn_override, 6, 0);
    lv_obj_set_style_shadow_width(g_ui_ctx.btn_override, 0, 0);
    lv_obj_add_event_cb(g_ui_ctx.btn_override, on_override_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_ov = lv_label_create(g_ui_ctx.btn_override);
    lv_label_set_text(lbl_ov, LV_SYMBOL_PLAY " تایید دما و شروع");
    lv_obj_set_style_text_color(lbl_ov, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_ov, &ui_font_Dana_Bold13, 0);
    lv_obj_center(lbl_ov);

    // =========================================================================
    // ACTION BUTTONS BAR (y: 264, h: 48) - Footer removed for maximum space
    // =========================================================================
    g_ui_ctx.btn_start = lv_btn_create(g_ui_ctx.scr_main);
    lv_obj_set_size(g_ui_ctx.btn_start, 225, 46);
    lv_obj_set_pos(g_ui_ctx.btn_start, 10, 264);
    lv_obj_set_style_bg_color(g_ui_ctx.btn_start, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_radius(g_ui_ctx.btn_start, 8, 0);
    lv_obj_set_style_shadow_width(g_ui_ctx.btn_start, 0, 0);
    lv_obj_add_event_cb(g_ui_ctx.btn_start, on_start_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_start = lv_label_create(g_ui_ctx.btn_start);
    lv_label_set_text(lbl_start, LV_SYMBOL_PLAY " شروع چرخه (START)");
    lv_obj_set_style_text_font(lbl_start, &ui_font_Dana_Bold18, 0);
    lv_obj_center(lbl_start);

    g_ui_ctx.btn_stop = lv_btn_create(g_ui_ctx.scr_main);
    lv_obj_set_size(g_ui_ctx.btn_stop, 225, 46);
    lv_obj_set_pos(g_ui_ctx.btn_stop, 245, 264);
    lv_obj_set_style_bg_color(g_ui_ctx.btn_stop, lv_color_hex(0xDC2626), 0);
    lv_obj_set_style_radius(g_ui_ctx.btn_stop, 8, 0);
    lv_obj_set_style_shadow_width(g_ui_ctx.btn_stop, 0, 0);
    lv_obj_add_event_cb(g_ui_ctx.btn_stop, on_stop_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_stop = lv_label_create(g_ui_ctx.btn_stop);
    lv_label_set_text(lbl_stop, LV_SYMBOL_STOP " توقف اضطراری (STOP)");
    lv_obj_set_style_text_font(lbl_stop, &ui_font_Dana_Bold18, 0);
    lv_obj_center(lbl_stop);

    // =========================================================================
    // 2. UNIFIED SETTINGS HUB SCREEN (F1..F5 TABS)
    // =========================================================================
    g_ui_ctx.scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_ui_ctx.scr_settings, lv_color_hex(0xF8FAFC), 0);
    lv_obj_clear_flag(g_ui_ctx.scr_settings, LV_OBJ_FLAG_SCROLLABLE);

    // Settings Header Bar (y: 0, h: 38)
    lv_obj_t* set_header = lv_obj_create(g_ui_ctx.scr_settings);
    lv_obj_set_size(set_header, 480, 38);
    lv_obj_set_pos(set_header, 0, 0);
    lv_obj_set_style_bg_color(set_header, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(set_header, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(set_header, 1, 0);
    lv_obj_set_style_radius(set_header, 0, 0);
    lv_obj_clear_flag(set_header, LV_OBJ_FLAG_SCROLLABLE);

    // Back Button (Left)
    lv_obj_t* btn_back = lv_btn_create(set_header);
    lv_obj_set_size(btn_back, 88, 28);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_radius(btn_back, 6, 0);
    lv_obj_set_style_shadow_width(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, on_back_to_main_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " بازگشت");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(lbl_back, &ui_font_Dana_Bold13, 0);
    lv_obj_center(lbl_back);

    // Title (Center)
    lv_obj_t* lbl_set_title = lv_label_create(set_header);
    lv_label_set_text(lbl_set_title, LV_SYMBOL_SETTINGS " تنظیمات عمومی میکسر");
    lv_obj_set_style_text_color(lbl_set_title, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(lbl_set_title, &ui_font_Dana_Bold18, 0);
    lv_obj_align(lbl_set_title, LV_ALIGN_CENTER, 0, 0);

    // Reactive Save Button (Right)
    g_ui_ctx.btn_save_config = lv_btn_create(set_header);
    lv_obj_set_size(g_ui_ctx.btn_save_config, 100, 28);
    lv_obj_align(g_ui_ctx.btn_save_config, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_style_bg_color(g_ui_ctx.btn_save_config, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_color(g_ui_ctx.btn_save_config, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(g_ui_ctx.btn_save_config, 1, 0);
    lv_obj_set_style_radius(g_ui_ctx.btn_save_config, 6, 0);
    lv_obj_set_style_shadow_width(g_ui_ctx.btn_save_config, 0, 0);
    lv_obj_add_event_cb(g_ui_ctx.btn_save_config, on_save_config_click, LV_EVENT_CLICKED, NULL);

    g_ui_ctx.lbl_save_btn = lv_label_create(g_ui_ctx.btn_save_config);
    lv_label_set_text(g_ui_ctx.lbl_save_btn, LV_SYMBOL_SAVE " ذخیره");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_save_btn, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_save_btn, &ui_font_Dana_Bold13, 0);
    lv_obj_center(g_ui_ctx.lbl_save_btn);

    // =========================================================================
    // TAB SELECTOR BAR (y: 40, h: 34, w: 460, x: 10)
    // =========================================================================
    const char* tab_names[5] = {
        "[F1] رسپی", "[F2] شیفت", "[F3] آب و دما", "[F4] دیاگ", "[F5] اقساط"
    };

    for (int i = 0; i < 5; ++i) {
        g_ui_ctx.tab_btns[i] = lv_btn_create(g_ui_ctx.scr_settings);
        lv_obj_set_size(g_ui_ctx.tab_btns[i], 88, 32);
        lv_obj_set_pos(g_ui_ctx.tab_btns[i], 10 + (i * 93), 40);
        lv_obj_set_style_radius(g_ui_ctx.tab_btns[i], 6, 0);
        lv_obj_set_style_shadow_width(g_ui_ctx.tab_btns[i], 0, 0);
        lv_obj_set_style_border_width(g_ui_ctx.tab_btns[i], 1, 0);
        lv_obj_add_event_cb(g_ui_ctx.tab_btns[i], on_tab_btn_click, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* lbl = lv_label_create(g_ui_ctx.tab_btns[i]);
        lv_label_set_text(lbl, tab_names[i]);
        lv_obj_set_style_text_font(lbl, &ui_font_Dana_Bold13, 0);
        lv_obj_center(lbl);

        // Container for each tab
        g_ui_ctx.tab_containers[i] = lv_obj_create(g_ui_ctx.scr_settings);
        lv_obj_set_pos(g_ui_ctx.tab_containers[i], 10, 76);
        lv_obj_set_size(g_ui_ctx.tab_containers[i], 460, 238);
        lv_obj_set_style_bg_color(g_ui_ctx.tab_containers[i], lv_color_hex(0xF8FAFC), 0);
        lv_obj_set_style_border_width(g_ui_ctx.tab_containers[i], 0, 0);
        lv_obj_set_style_pad_all(g_ui_ctx.tab_containers[i], 0, 0);
        lv_obj_clear_flag(g_ui_ctx.tab_containers[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    // -------------------------------------------------------------------------
    // TAB 0: [F1] RECIPE SETTINGS
    // -------------------------------------------------------------------------
    lv_obj_t* tab0 = g_ui_ctx.tab_containers[0];

    // Recipe Switcher Card
    lv_obj_t* rec_card = lv_obj_create(tab0);
    lv_obj_set_pos(rec_card, 0, 0);
    lv_obj_set_size(rec_card, 460, 44);
    lv_obj_set_style_bg_color(rec_card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(rec_card, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(rec_card, 1, 0);
    lv_obj_set_style_radius(rec_card, 8, 0);
    lv_obj_set_style_pad_all(rec_card, 3, 0);
    lv_obj_clear_flag(rec_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_cycle_recipe = lv_btn_create(rec_card);
    lv_obj_set_size(btn_cycle_recipe, 170, 36);
    lv_obj_align(btn_cycle_recipe, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_color(btn_cycle_recipe, lv_color_hex(0x4338CA), 0);
    lv_obj_set_style_radius(btn_cycle_recipe, 6, 0);
    lv_obj_set_style_shadow_width(btn_cycle_recipe, 0, 0);
    lv_obj_add_event_cb(btn_cycle_recipe, on_cycle_recipe_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_cr = lv_label_create(btn_cycle_recipe);
    lv_label_set_text(lbl_cr, LV_SYMBOL_REFRESH " تغییر نان فعال");
    lv_obj_set_style_text_font(lbl_cr, &ui_font_Dana_Bold13, 0);
    lv_obj_set_style_text_color(lbl_cr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_cr);

    g_ui_ctx.lbl_step_active_recipe = lv_label_create(rec_card);
    lv_label_set_text(g_ui_ctx.lbl_step_active_recipe, "رسپی: ۱. بربری (۱۲۰s, ۲۰L, ۲۴°C)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_step_active_recipe, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_step_active_recipe, &ui_font_Dana_Bold13, 0);
    lv_obj_align(g_ui_ctx.lbl_step_active_recipe, LV_ALIGN_RIGHT_MID, -12, 0);

    // Steppers for Tab 0
    ui_create_stepper(tab0, 48, "زمان میکس خمیر", "(ثانیه)", on_step_mix_time_minus, on_step_mix_time_plus, &g_ui_ctx.lbl_step_mix_time);
    ui_create_stepper(tab0, 96, "حجم آب تزریقی", "(لیتر)", on_step_water_liters_minus, on_step_water_liters_plus, &g_ui_ctx.lbl_step_water_liters);
    ui_create_stepper(tab0, 144, "دمای مطلوب آب", "(درجه)", on_step_temp_target_minus, on_step_temp_target_plus, &g_ui_ctx.lbl_step_temp_target);

    // Bottom info note
    lv_obj_t* rec_hint = lv_obj_create(tab0);
    lv_obj_set_pos(rec_hint, 0, 192);
    lv_obj_set_size(rec_hint, 460, 40);
    lv_obj_set_style_bg_color(rec_hint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(rec_hint, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(rec_hint, 1, 0);
    lv_obj_set_style_radius(rec_hint, 8, 0);
    lv_obj_set_style_pad_all(rec_hint, 4, 0);
    lv_obj_clear_flag(rec_hint, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_hint = lv_label_create(rec_hint);
    lv_label_set_text(lbl_hint, LV_SYMBOL_OK " ذخیره خودکار در حافظه با تغییر رسپی یا کلید ذخیره");
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_text_font(lbl_hint, &ui_font_Dana_Bold13, 0);
    lv_obj_center(lbl_hint);

    // -------------------------------------------------------------------------
    // TAB 1: [F2] SHIFTS & RTC CLOCK
    // -------------------------------------------------------------------------
    lv_obj_t* tab1 = g_ui_ctx.tab_containers[1];
    ui_create_stepper(tab1, 0, "ساعت دستگاه", "(ساعت)", on_rtc_hour_minus, on_rtc_hour_plus, &g_ui_ctx.lbl_step_rtc_hour);
    ui_create_stepper(tab1, 46, "دقیقه دستگاه", "(دقیقه)", on_rtc_min_minus, on_rtc_min_plus, &g_ui_ctx.lbl_step_rtc_min);
    ui_create_stepper(tab1, 92, "شیفت ۱ صبح", "(ساعت)", NULL, on_shift1_hour_plus, &g_ui_ctx.lbl_step_shift1);
    ui_create_stepper(tab1, 138, "شیفت ۲ ظهر", "(ساعت)", NULL, on_shift2_hour_plus, &g_ui_ctx.lbl_step_shift2);
    ui_create_stepper(tab1, 184, "شیفت ۳ عصر", "(ساعت)", NULL, on_shift3_hour_plus, &g_ui_ctx.lbl_step_shift3);

    // -------------------------------------------------------------------------
    // TAB 2: [F3] WATER, DOSING & CALIBRATION
    // -------------------------------------------------------------------------
    lv_obj_t* tab2 = g_ui_ctx.tab_containers[2];
    ui_create_stepper(tab2, 0, "کالیبراسیون دبی فلومتر", "(لیتر بر دقیقه)", on_step_flowrate_minus, on_step_flowrate_plus, &g_ui_ctx.lbl_step_flowrate);
    ui_create_stepper(tab2, 46, "زمان ترمز کاسه", "(ثانیه)", on_step_brake_minus, on_step_brake_plus, &g_ui_ctx.lbl_step_brake);
    ui_create_stepper(tab2, 92, "آماده‌سازی دما قبل شیفت", "(دقیقه)", on_step_pretemp_minus, on_step_pretemp_plus, &g_ui_ctx.lbl_step_pretemp);

    // Flour Mode Card
    lv_obj_t* flour_card = lv_obj_create(tab2);
    lv_obj_set_pos(flour_card, 0, 140);
    lv_obj_set_size(flour_card, 460, 44);
    lv_obj_set_style_bg_color(flour_card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(flour_card, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(flour_card, 1, 0);
    lv_obj_set_style_radius(flour_card, 8, 0);
    lv_obj_set_style_pad_all(flour_card, 3, 0);
    lv_obj_clear_flag(flour_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_flour_mode = lv_btn_create(flour_card);
    lv_obj_set_size(btn_flour_mode, 170, 36);
    lv_obj_align(btn_flour_mode, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_color(btn_flour_mode, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_radius(btn_flour_mode, 6, 0);
    lv_obj_set_style_shadow_width(btn_flour_mode, 0, 0);
    lv_obj_add_event_cb(btn_flour_mode, on_step_flour_mode_toggle, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_fm = lv_label_create(btn_flour_mode);
    lv_label_set_text(lbl_fm, LV_SYMBOL_REFRESH " تغییر حالت آرد");
    lv_obj_set_style_text_font(lbl_fm, &ui_font_Dana_Bold13, 0);
    lv_obj_set_style_text_color(lbl_fm, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_fm);

    g_ui_ctx.lbl_step_flour_mode = lv_label_create(flour_card);
    lv_label_set_text(g_ui_ctx.lbl_step_flour_mode, "تزریق آرد: دستی (آماده)");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_step_flour_mode, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_step_flour_mode, &ui_font_Dana_Bold13, 0);
    lv_obj_align(g_ui_ctx.lbl_step_flour_mode, LV_ALIGN_RIGHT_MID, -12, 0);

    // -------------------------------------------------------------------------
    // TAB 3: [F4] HARDWARE DIAGNOSTICS
    // -------------------------------------------------------------------------
    lv_obj_t* tab3 = g_ui_ctx.tab_containers[3];

    // Inputs Column
    lv_obj_t* in_card = ui_create_apple_card(tab3, 0, 0, 226, 234, "ورودی دیجیتال (I0..I7)");
    const char* in_pins[] = {"I0", "I1", "I2", "I3", "I4", "I5", "I6", "I7"};
    const char* in_labels[] = {
        "استپ اضطراری", "استارت کلید", "توری حفاظ",
        "حرارتی موتور", "فلومتر آب", "دمای آب ۱",
        "دمای آب ۲", "سنسور رزرو"
    };

    for (int i = 0; i < 8; ++i) {
        int y_pos = 16 + (i * 26);
        g_ui_ctx.in_lamps[i] = lv_obj_create(in_card);
        lv_obj_set_size(g_ui_ctx.in_lamps[i], 12, 12);
        lv_obj_set_style_radius(g_ui_ctx.in_lamps[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(g_ui_ctx.in_lamps[i], lv_color_hex(0xE2E8F0), 0);
        lv_obj_set_style_border_color(g_ui_ctx.in_lamps[i], lv_color_hex(0xCBD5E1), 0);
        lv_obj_set_style_border_width(g_ui_ctx.in_lamps[i], 1, 0);
        lv_obj_set_pos(g_ui_ctx.in_lamps[i], 4, y_pos + 2);

        lv_obj_t* pin_lbl = lv_label_create(in_card);
        lv_label_set_text(pin_lbl, in_pins[i]);
        lv_obj_set_style_text_color(pin_lbl, lv_color_hex(0x0284C7), 0);
        lv_obj_set_style_text_font(pin_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(pin_lbl, 20, y_pos + 1);

        lv_obj_t* lbl = lv_label_create(in_card);
        lv_label_set_text(lbl, in_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x1E293B), 0);
        lv_obj_set_style_text_font(lbl, &ui_font_Dana_Bold13, 0);
        lv_obj_set_pos(lbl, 42, y_pos);
    }

    // Outputs Column
    lv_obj_t* out_card = ui_create_apple_card(tab3, 234, 0, 226, 234, "خروجی رله (Q0..Q9)");
    const char* out_pins[] = {"Q0", "Q1", "Q2", "Q3", "Q4", "Q5", "Q6", "Q7", "Q8", "Q9"};
    const char* out_labels[] = {
        "موتور اصلی", "تزریق آرد", "ترمز کاسه", "رزرو کمکی",
        "تزریق آب", "دمای آب", "رزرو کمکی", "فن مکنده",
        "سیگنال سبز", "سیگنال قرمز"
    };

    for (int i = 0; i < 10; ++i) {
        int y_pos = 14 + (i * 21);
        g_ui_ctx.out_lamps[i] = lv_btn_create(out_card);
        lv_obj_set_size(g_ui_ctx.out_lamps[i], 14, 14);
        lv_obj_set_style_radius(g_ui_ctx.out_lamps[i], 3, 0);
        lv_obj_set_style_bg_color(g_ui_ctx.out_lamps[i], lv_color_hex(0xF1F5F9), 0);
        lv_obj_set_style_border_color(g_ui_ctx.out_lamps[i], lv_color_hex(0xCBD5E1), 0);
        lv_obj_set_style_border_width(g_ui_ctx.out_lamps[i], 1, 0);
        lv_obj_set_style_shadow_width(g_ui_ctx.out_lamps[i], 0, 0);
        lv_obj_set_pos(g_ui_ctx.out_lamps[i], 4, y_pos + 1);
        lv_obj_add_event_cb(g_ui_ctx.out_lamps[i], on_diag_relay_toggle, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* pin_lbl = lv_label_create(out_card);
        lv_label_set_text(pin_lbl, out_pins[i]);
        lv_obj_set_style_text_color(pin_lbl, lv_color_hex(0x0284C7), 0);
        lv_obj_set_style_text_font(pin_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(pin_lbl, 22, y_pos + 1);

        lv_obj_t* lbl = lv_label_create(out_card);
        lv_label_set_text(lbl, out_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x1E293B), 0);
        lv_obj_set_style_text_font(lbl, &ui_font_Dana_Bold13, 0);
        lv_obj_set_pos(lbl, 46, y_pos);
    }

    // -------------------------------------------------------------------------
    // TAB 4: [F5] 5-STAGE DAY-LOCK LEASE MANAGEMENT
    // -------------------------------------------------------------------------
    lv_obj_t* tab4 = g_ui_ctx.tab_containers[4];

    // Left Info Card
    lv_obj_t* dl_info_card = ui_create_apple_card(tab4, 0, 0, 215, 234, "وضعیت اقساط");

    g_ui_ctx.lbl_dl_stage = lv_label_create(dl_info_card);
    lv_label_set_text(g_ui_ctx.lbl_dl_stage, LV_SYMBOL_SETTINGS " مرحله: ۱ از ۵");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_stage, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_dl_stage, &ui_font_Dana_Bold18, 0);
    lv_obj_set_size(g_ui_ctx.lbl_dl_stage, 195, 22);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_dl_stage, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_dl_stage, 4, 22);

    g_ui_ctx.lbl_dl_days = lv_label_create(dl_info_card);
    lv_label_set_text(g_ui_ctx.lbl_dl_days, LV_SYMBOL_OK " اعتبار: ۳۰ روز");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_days, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_dl_days, &ui_font_Dana_Bold18, 0);
    lv_obj_set_size(g_ui_ctx.lbl_dl_days, 195, 22);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_dl_days, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_dl_days, 4, 52);

    g_ui_ctx.lbl_dl_jalali = lv_label_create(dl_info_card);
    lv_label_set_text(g_ui_ctx.lbl_dl_jalali, LV_SYMBOL_BELL " سررسید: ۱۴۰۵/۰۷/۱۰");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_jalali, lv_color_hex(0xD97706), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_dl_jalali, &ui_font_Dana_Bold18, 0);
    lv_obj_set_size(g_ui_ctx.lbl_dl_jalali, 195, 22);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_dl_jalali, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_dl_jalali, 4, 82);

    g_ui_ctx.lbl_dl_challenge = lv_label_create(dl_info_card);
    lv_label_set_text(g_ui_ctx.lbl_dl_challenge, LV_SYMBOL_KEY " چالش: ۴۸۲۹");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_challenge, lv_color_hex(0xDB2777), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_dl_challenge, &ui_font_Dana_Bold18, 0);
    lv_obj_set_size(g_ui_ctx.lbl_dl_challenge, 195, 22);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_dl_challenge, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_dl_challenge, 4, 112);

    g_ui_ctx.lbl_dl_status = lv_label_create(dl_info_card);
    lv_label_set_text(g_ui_ctx.lbl_dl_status, LV_SYMBOL_REFRESH " مجاز به کار");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_status, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_dl_status, &ui_font_Dana_Bold18, 0);
    lv_obj_set_size(g_ui_ctx.lbl_dl_status, 195, 22);
    lv_obj_set_style_text_align(g_ui_ctx.lbl_dl_status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(g_ui_ctx.lbl_dl_status, 4, 142);

    lv_obj_t* lbl_dl_support = lv_label_create(dl_info_card);
    lv_label_set_text(lbl_dl_support, LV_SYMBOL_CALL " پشتیبانی: ۰۲۱-۸۸۸۸");
    lv_obj_set_style_text_color(lbl_dl_support, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(lbl_dl_support, &ui_font_Dana_Bold18, 0);
    lv_obj_set_size(lbl_dl_support, 195, 22);
    lv_obj_set_style_text_align(lbl_dl_support, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(lbl_dl_support, 4, 172);

    // Right Keypad Card
    lv_obj_t* dl_key_card = ui_create_apple_card(tab4, 222, 0, 238, 234, NULL);

    lv_obj_t* input_box = lv_obj_create(dl_key_card);
    lv_obj_set_size(input_box, 218, 30);
    lv_obj_set_pos(input_box, 4, 4);
    lv_obj_set_style_bg_color(input_box, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_border_color(input_box, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(input_box, 1, 0);
    lv_obj_set_style_radius(input_box, 6, 0);
    lv_obj_clear_flag(input_box, LV_OBJ_FLAG_SCROLLABLE);

    g_ui_ctx.lbl_dl_input = lv_label_create(input_box);
    lv_label_set_text(g_ui_ctx.lbl_dl_input, "[ _ _ _ _ ]");
    lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_input, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_text_font(g_ui_ctx.lbl_dl_input, &ui_font_Dana_Bold18, 0);
    lv_obj_center(g_ui_ctx.lbl_dl_input);

    const char* k_labels[12] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9",
        LV_SYMBOL_CLOSE " پاک", "0", LV_SYMBOL_OK " ثبت"
    };

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 3; ++c) {
            int k_idx = (r * 3) + c;
            lv_obj_t* k_btn = lv_btn_create(dl_key_card);
            lv_obj_set_size(k_btn, 68, 38);
            lv_obj_set_pos(k_btn, 4 + (c * 74), 40 + (r * 44));
            lv_obj_set_style_radius(k_btn, 6, 0);
            lv_obj_set_style_shadow_width(k_btn, 0, 0);

            if (k_idx == 9) {
                lv_obj_set_style_bg_color(k_btn, lv_color_hex(0xDC2626), 0);
                lv_obj_set_style_border_color(k_btn, lv_color_hex(0xB91C1C), 0);
                lv_obj_set_style_border_width(k_btn, 1, 0);
                lv_obj_add_event_cb(k_btn, on_dl_backspace_click, LV_EVENT_CLICKED, NULL);
            } else if (k_idx == 11) {
                lv_obj_set_style_bg_color(k_btn, lv_color_hex(0x16A34A), 0);
                lv_obj_set_style_border_color(k_btn, lv_color_hex(0x15803D), 0);
                lv_obj_set_style_border_width(k_btn, 1, 0);
                lv_obj_add_event_cb(k_btn, on_dl_confirm_click, LV_EVENT_CLICKED, NULL);
            } else {
                int digit = (k_idx == 10) ? 0 : (k_idx + 1);
                lv_obj_set_style_bg_color(k_btn, lv_color_hex(0xF1F5F9), 0);
                lv_obj_set_style_border_color(k_btn, lv_color_hex(0xCBD5E1), 0);
                lv_obj_set_style_border_width(k_btn, 1, 0);
                lv_obj_add_event_cb(k_btn, on_dl_digit_click, LV_EVENT_CLICKED, (void*)(intptr_t)digit);
            }

            lv_obj_t* k_lbl = lv_label_create(k_btn);
            lv_label_set_text(k_lbl, k_labels[k_idx]);
            lv_obj_set_style_text_font(k_lbl, &ui_font_Dana_Bold18, 0);
            if (k_idx == 9 || k_idx == 11) {
                lv_obj_set_style_text_color(k_lbl, lv_color_hex(0xFFFFFF), 0);
            } else {
                lv_obj_set_style_text_color(k_lbl, lv_color_hex(0x0F172A), 0);
            }
            lv_obj_center(k_lbl);
        }
    }

    // Default settings tab is Tab 0 (Recipe)
    haffler_ui_set_settings_tab(0);

    g_ui_ctx.current_screen = HAFFLER_SCREEN_MAIN;
    haffler_ui_refresh_steppers();
    lv_scr_load(g_ui_ctx.scr_main);
}

// =============================================================================
// REFRESH & UPDATE HELPERS
// =============================================================================
void haffler_ui_mark_settings_dirty(bool dirty) {
    g_ui_ctx.is_settings_dirty = dirty;
    if (g_ui_ctx.btn_save_config && g_ui_ctx.lbl_save_btn) {
        if (dirty) {
            lv_obj_set_style_bg_color(g_ui_ctx.btn_save_config, lv_color_hex(0x16A34A), 0);
            lv_obj_set_style_border_color(g_ui_ctx.btn_save_config, lv_color_hex(0x15803D), 0);
            lv_obj_set_style_text_color(g_ui_ctx.lbl_save_btn, lv_color_hex(0xFFFFFF), 0);
            lv_label_set_text(g_ui_ctx.lbl_save_btn, LV_SYMBOL_SAVE " ذخیره *");
        } else {
            lv_obj_set_style_bg_color(g_ui_ctx.btn_save_config, lv_color_hex(0xF1F5F9), 0);
            lv_obj_set_style_border_color(g_ui_ctx.btn_save_config, lv_color_hex(0xCBD5E1), 0);
            lv_obj_set_style_text_color(g_ui_ctx.lbl_save_btn, lv_color_hex(0x64748B), 0);
            lv_label_set_text(g_ui_ctx.lbl_save_btn, LV_SYMBOL_SAVE " ذخیره");
        }
    }
}

void haffler_ui_set_settings_tab(uint8_t tab_idx) {
    if (tab_idx >= 5) tab_idx = 0;
    g_ui_ctx.active_settings_tab = tab_idx;

    for (uint8_t i = 0; i < 5; ++i) {
        if (g_ui_ctx.tab_btns[i]) {
            if (i == tab_idx) {
                lv_obj_set_style_bg_color(g_ui_ctx.tab_btns[i], lv_color_hex(0x0284C7), 0);
                lv_obj_set_style_border_color(g_ui_ctx.tab_btns[i], lv_color_hex(0x0369A1), 0);
                lv_obj_t* lbl = lv_obj_get_child(g_ui_ctx.tab_btns[i], 0);
                if (lbl) lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
            } else {
                lv_obj_set_style_bg_color(g_ui_ctx.tab_btns[i], lv_color_hex(0xF1F5F9), 0);
                lv_obj_set_style_border_color(g_ui_ctx.tab_btns[i], lv_color_hex(0xCBD5E1), 0);
                lv_obj_t* lbl = lv_obj_get_child(g_ui_ctx.tab_btns[i], 0);
                if (lbl) lv_obj_set_style_text_color(lbl, lv_color_hex(0x1E293B), 0);
            }
        }
        if (g_ui_ctx.tab_containers[i]) {
            if (i == tab_idx) {
                lv_obj_clear_flag(g_ui_ctx.tab_containers[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(g_ui_ctx.tab_containers[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void haffler_ui_set_pipeline_stage(uint8_t stage_idx, const char* cur_name, const char* next_name) {
    if (stage_idx >= 5) stage_idx = 4;

    for (uint8_t i = 0; i < 5; ++i) {
        if (!g_ui_ctx.step_capsules[i]) continue;
        if (i < stage_idx) {
            // Completed stage (Green)
            lv_obj_set_style_bg_color(g_ui_ctx.step_capsules[i], lv_color_hex(0x16A34A), 0);
            lv_obj_set_style_border_color(g_ui_ctx.step_capsules[i], lv_color_hex(0x15803D), 0);
            if (g_ui_ctx.lbl_step_names[i]) {
                lv_obj_set_style_text_color(g_ui_ctx.lbl_step_names[i], lv_color_hex(0xFFFFFF), 0);
            }
        } else if (i == stage_idx) {
            // Active stage (Apple Blue)
            lv_obj_set_style_bg_color(g_ui_ctx.step_capsules[i], lv_color_hex(0x0284C7), 0);
            lv_obj_set_style_border_color(g_ui_ctx.step_capsules[i], lv_color_hex(0x0369A1), 0);
            if (g_ui_ctx.lbl_step_names[i]) {
                lv_obj_set_style_text_color(g_ui_ctx.lbl_step_names[i], lv_color_hex(0xFFFFFF), 0);
            }
        } else {
            // Pending stage (Soft Slate)
            lv_obj_set_style_bg_color(g_ui_ctx.step_capsules[i], lv_color_hex(0xF1F5F9), 0);
            lv_obj_set_style_border_color(g_ui_ctx.step_capsules[i], lv_color_hex(0xCBD5E1), 0);
            if (g_ui_ctx.lbl_step_names[i]) {
                lv_obj_set_style_text_color(g_ui_ctx.lbl_step_names[i], lv_color_hex(0x64748B), 0);
            }
        }
    }

    if (g_ui_ctx.lbl_next_stage_hint) {
        char buf[80];
        if (next_name && strlen(next_name) > 0) {
            snprintf(buf, sizeof(buf), "مرحله بعد: %s", next_name);
        } else if (cur_name && strlen(cur_name) > 0) {
            snprintf(buf, sizeof(buf), "در حال اجرا: %s", cur_name);
        } else {
            snprintf(buf, sizeof(buf), "وضعیت: آماده شروع چرخه پخت");
        }
        lv_label_set_text(g_ui_ctx.lbl_next_stage_hint, buf);
    }
}

void haffler_ui_set_mix_progress(uint8_t percent) {
    if (percent > 100) percent = 100;
    if (g_ui_ctx.bar_mix_progress) {
        lv_bar_set_value(g_ui_ctx.bar_mix_progress, percent, LV_ANIM_OFF);
    }
    if (g_ui_ctx.lbl_mix_percent) {
        char buf[32];
        snprintf(buf, sizeof(buf), "پیشرفت میکس: %u٪", (unsigned)percent);
        lv_label_set_text(g_ui_ctx.lbl_mix_percent, buf);
    }
}

void haffler_ui_refresh_steppers(void) {
    const bakery_config_t& cfg = g_machine_fsm.getConfig();
    const bakery_live_t& live = g_machine_fsm.getLive();
    char buf[64];

    if (g_ui_ctx.lbl_step_mix_time) {
        snprintf(buf, sizeof(buf), "%u", (unsigned)cfg.mix_time_sec);
        lv_label_set_text(g_ui_ctx.lbl_step_mix_time, buf);
    }
    if (g_ui_ctx.lbl_step_water_liters) {
        snprintf(buf, sizeof(buf), "%.1f", cfg.water_target_liters);
        lv_label_set_text(g_ui_ctx.lbl_step_water_liters, buf);
    }
    if (g_ui_ctx.lbl_step_flowrate) {
        snprintf(buf, sizeof(buf), "%.1f", cfg.flow_rate_l_per_min);
        lv_label_set_text(g_ui_ctx.lbl_step_flowrate, buf);
    }
    if (g_ui_ctx.lbl_step_temp_target) {
        snprintf(buf, sizeof(buf), "%.1f", cfg.water_target_temp_c);
        lv_label_set_text(g_ui_ctx.lbl_step_temp_target, buf);
    }
    if (g_ui_ctx.lbl_step_brake) {
        snprintf(buf, sizeof(buf), "%u", (unsigned)cfg.brake_on_time_sec);
        lv_label_set_text(g_ui_ctx.lbl_step_brake, buf);
    }
    if (g_ui_ctx.lbl_step_rtc_hour) {
        snprintf(buf, sizeof(buf), "%02u", live.rtc_hour);
        lv_label_set_text(g_ui_ctx.lbl_step_rtc_hour, buf);
    }
    if (g_ui_ctx.lbl_step_rtc_min) {
        snprintf(buf, sizeof(buf), "%02u", live.rtc_minute);
        lv_label_set_text(g_ui_ctx.lbl_step_rtc_min, buf);
    }
    if (g_ui_ctx.lbl_step_shift1) {
        snprintf(buf, sizeof(buf), "%02u:%02u", cfg.shifts[0].hour, cfg.shifts[0].minute);
        lv_label_set_text(g_ui_ctx.lbl_step_shift1, buf);
    }
    if (g_ui_ctx.lbl_step_shift2) {
        snprintf(buf, sizeof(buf), "%02u:%02u", cfg.shifts[1].hour, cfg.shifts[1].minute);
        lv_label_set_text(g_ui_ctx.lbl_step_shift2, buf);
    }
    if (g_ui_ctx.lbl_step_shift3) {
        snprintf(buf, sizeof(buf), "%02u:%02u", cfg.shifts[2].hour, cfg.shifts[2].minute);
        lv_label_set_text(g_ui_ctx.lbl_step_shift3, buf);
    }
    if (g_ui_ctx.lbl_step_pretemp) {
        snprintf(buf, sizeof(buf), "%u", (unsigned)cfg.pre_temp_lead_min);
        lv_label_set_text(g_ui_ctx.lbl_step_pretemp, buf);
    }
    if (g_ui_ctx.lbl_step_active_recipe) {
        HafflerRecipe* r = g_recipes.getActive();
        if (r) {
            snprintf(buf, sizeof(buf), "رسپی: %s (%us, %.0fL)", r->name, (unsigned)cfg.mix_time_sec, cfg.water_target_liters);
            lv_label_set_text(g_ui_ctx.lbl_step_active_recipe, buf);
        }
    }
    if (g_ui_ctx.lbl_step_flour_mode) {
        if (cfg.flour_mode == 1) {
            snprintf(buf, sizeof(buf), "تزریق آرد: ماردون (%us)", (unsigned)cfg.flour_feed_time_sec);
        } else {
            snprintf(buf, sizeof(buf), "تزریق آرد: دستی (آماده)");
        }
        lv_label_set_text(g_ui_ctx.lbl_step_flour_mode, buf);
    }
}

void haffler_ui_switch_screen(haffler_screen_id_t target_screen) {
    lv_obj_t* target = g_ui_ctx.scr_main;
    switch (target_screen) {
        case HAFFLER_SCREEN_MAIN:
            target = g_ui_ctx.scr_main;
            break;
        case HAFFLER_SCREEN_SETTINGS:
            target = g_ui_ctx.scr_settings;
            break;
        case HAFFLER_SCREEN_SCHEDULE:
            target = g_ui_ctx.scr_settings;
            haffler_ui_set_settings_tab(1);
            break;
        case HAFFLER_SCREEN_DIAGNOSTICS:
            target = g_ui_ctx.scr_settings;
            haffler_ui_set_settings_tab(3);
            break;
        case HAFFLER_SCREEN_DAY_LOCK:
            target = g_ui_ctx.scr_settings;
            haffler_ui_set_settings_tab(4);
            break;
        default:
            target = g_ui_ctx.scr_main;
            break;
    }
    if (target) {
        g_ui_ctx.current_screen = target_screen;
        haffler_ui_refresh_steppers();
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
        if (lbl) {
            char buf[32];
            const char* sym = LV_SYMBOL_OK;
            if (strstr(state_str, "RUN") || strstr(state_str, "MIX") || strstr(state_str, "کار")) {
                sym = LV_SYMBOL_PLAY;
            } else if (strstr(state_str, "STOP") || strstr(state_str, "توقف")) {
                sym = LV_SYMBOL_STOP;
            } else if (strstr(state_str, "ALARM") || strstr(state_str, "ERR") || strstr(state_str, "خطا")) {
                sym = LV_SYMBOL_WARNING;
            }
            snprintf(buf, sizeof(buf), "%s %s", sym, state_str);
            lv_label_set_text(lbl, buf);
        }
        lv_obj_set_style_bg_color(g_ui_ctx.badge_state, lv_color_hex(bg_hex), 0);
    }
}

void haffler_ui_set_clock(uint8_t h, uint8_t m, uint8_t s) {
    if (g_ui_ctx.lbl_clock) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
        lv_label_set_text(g_ui_ctx.lbl_clock, buf);
    }
}

void haffler_ui_set_recipe_name(const char* recipe_name) {
    if (g_ui_ctx.badge_recipe && recipe_name) {
        lv_obj_t* lbl = lv_obj_get_child(g_ui_ctx.badge_recipe, 0);
        if (lbl) {
            char buf[32];
            snprintf(buf, sizeof(buf), LV_SYMBOL_LIST " %s", recipe_name);
            lv_label_set_text(lbl, buf);
        }
    }
}

void haffler_ui_set_water_info(float liters, float target_l) {
    if (g_ui_ctx.lbl_water_info) {
        char buf[64];
        snprintf(buf, sizeof(buf), LV_SYMBOL_TINT " آب: %.1f از %.1f لیتر", liters, target_l);
        lv_label_set_text(g_ui_ctx.lbl_water_info, buf);
    }
}

void haffler_ui_set_temp_info(float cur_temp, float target_temp, bool ready) {
    (void)target_temp;
    if (g_ui_ctx.lbl_temp_info) {
        char buf[64];
        snprintf(buf, sizeof(buf), LV_SYMBOL_TINT " دمای آب: %.1f°C (%s)", cur_temp, ready ? "آماده" : "در حال تنظیم");
        lv_label_set_text(g_ui_ctx.lbl_temp_info, buf);
        lv_obj_set_style_text_color(g_ui_ctx.lbl_temp_info, ready ? lv_color_hex(0x16A34A) : lv_color_hex(0xD97706), 0);
    }
}

void haffler_ui_set_flour_info(bool motorized, uint32_t duration_sec) {
    if (g_ui_ctx.lbl_flour_info) {
        char buf[64];
        if (motorized) {
            snprintf(buf, sizeof(buf), LV_SYMBOL_DOWNLOAD " آرد: ماردون (%u ثانیه)", (unsigned)duration_sec);
        } else {
            snprintf(buf, sizeof(buf), LV_SYMBOL_DOWNLOAD " آرد: دستی (آماده)");
        }
        lv_label_set_text(g_ui_ctx.lbl_flour_info, buf);
    }
}

void haffler_ui_set_brake_info(bool enabled, bool currently_braking) {
    if (g_ui_ctx.lbl_brake_info) {
        if (!enabled) {
            lv_label_set_text(g_ui_ctx.lbl_brake_info, LV_SYMBOL_LOOP " ترمز کاسه: غیرفعال");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_brake_info, lv_color_hex(0x8B949E), 0);
        } else if (currently_braking) {
            lv_label_set_text(g_ui_ctx.lbl_brake_info, LV_SYMBOL_LOOP " ترمز کاسه: [درگیر]");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_brake_info, lv_color_hex(0xDC2626), 0);
        } else {
            lv_label_set_text(g_ui_ctx.lbl_brake_info, LV_SYMBOL_LOOP " ترمز کاسه: آزاد");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_brake_info, lv_color_hex(0x16A34A), 0);
        }
    }
}

void haffler_ui_set_guard_status(bool closed_ok) {
    if (g_ui_ctx.lbl_guard_info) {
        if (closed_ok) {
            lv_label_set_text(g_ui_ctx.lbl_guard_info, LV_SYMBOL_SHUFFLE " توری: بسته (ایمن)");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_guard_info, lv_color_hex(0x16A34A), 0);
        } else {
            lv_label_set_text(g_ui_ctx.lbl_guard_info, LV_SYMBOL_SHUFFLE " توری: باز (توقف!)");
            lv_obj_set_style_text_color(g_ui_ctx.lbl_guard_info, lv_color_hex(0xDC2626), 0);
        }
    }
}

void haffler_ui_set_batch_count(uint32_t count) {
    if (g_ui_ctx.lbl_batch_info) {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_LIST " پخت: %u شیفت", (unsigned)count);
        lv_label_set_text(g_ui_ctx.lbl_batch_info, buf);
    }
}

void haffler_ui_set_input_lamp(uint8_t input_idx, bool active) {
    if (input_idx < 8 && g_ui_ctx.in_lamps[input_idx]) {
        uint32_t col = active ? 0x22C55E : 0xE2E8F0;
        lv_obj_set_style_bg_color(g_ui_ctx.in_lamps[input_idx], lv_color_hex(col), 0);
    }
}

void haffler_ui_set_output_lamp(uint8_t output_idx, bool active) {
    if (output_idx < 10 && g_ui_ctx.out_lamps[output_idx]) {
        uint32_t col = active ? 0x0284C7 : 0xF1F5F9;
        lv_obj_set_style_bg_color(g_ui_ctx.out_lamps[output_idx], lv_color_hex(col), 0);
    }
}

void haffler_ui_update_day_lock_screen(uint8_t stage, uint16_t days_left, uint16_t challenge, const char* jalali_str) {
    if (g_ui_ctx.lbl_dl_stage) {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_SETTINGS " مرحله: %u از ۵ (%s)", stage + 1, (days_left == 0) ? "منقضی" : "فعال");
        lv_label_set_text(g_ui_ctx.lbl_dl_stage, buf);
    }
    if (g_ui_ctx.lbl_dl_days) {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_OK " اعتبار: %u روز", days_left);
        lv_label_set_text(g_ui_ctx.lbl_dl_days, buf);
        uint32_t col = (days_left <= 2) ? 0xDC2626 : ((days_left <= 7) ? 0xD97706 : 0x16A34A);
        lv_obj_set_style_text_color(g_ui_ctx.lbl_dl_days, lv_color_hex(col), 0);
    }
    if (g_ui_ctx.lbl_dl_jalali && jalali_str) {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_BELL " سررسید: %s", jalali_str);
        lv_label_set_text(g_ui_ctx.lbl_dl_jalali, buf);
    }
    if (g_ui_ctx.lbl_dl_challenge) {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_KEY " چالش: %04u", challenge);
        lv_label_set_text(g_ui_ctx.lbl_dl_challenge, buf);
    }
}
