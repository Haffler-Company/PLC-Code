#pragma once
/**
 * @file haffler_ui_builder.h
 * @brief High-Level Apple HIG UI Screen Builder for Advanced Bakery Spiral Mixer (LVGL 9)
 * @details Real-time 50+ FPS, Zero-Lag, 44px Touch Targets, Zero Software Shadows.
 */

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"
#include "ui_components.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAFFLER_SCREEN_SPLASH = 0,
    HAFFLER_SCREEN_MAIN,
    HAFFLER_SCREEN_SETTINGS,
    HAFFLER_SCREEN_SCHEDULE,
    HAFFLER_SCREEN_DIAGNOSTICS,
    HAFFLER_SCREEN_DAY_LOCK
} haffler_screen_id_t;

typedef struct {
    lv_obj_t* scr_main;
    lv_obj_t* scr_settings;
    lv_obj_t* scr_schedule;
    lv_obj_t* scr_diagnostics;
    lv_obj_t* scr_day_lock;

    // Main Dashboard widgets
    lv_obj_t* lbl_clock;
    lv_obj_t* lbl_title;
    lv_obj_t* badge_recipe;
    lv_obj_t* badge_state;
    lv_obj_t* btn_header_settings;

    // Process Pipeline Widgets (5 Stages)
    lv_obj_t* pipeline_container;
    lv_obj_t* step_capsules[5];
    lv_obj_t* lbl_step_names[5];
    lv_obj_t* lbl_next_stage_hint;

    // Hero Metric and Progress Bar
    hero_metric_t hero_metric;
    lv_obj_t* bar_mix_progress;
    lv_obj_t* lbl_mix_percent;

    // Live Process Telemetry widgets
    lv_obj_t* lbl_water_info;
    lv_obj_t* lbl_temp_info;
    lv_obj_t* lbl_flour_info;
    lv_obj_t* lbl_brake_info;
    lv_obj_t* lbl_guard_info;
    lv_obj_t* lbl_batch_info;

    lv_obj_t* btn_start;
    lv_obj_t* btn_stop;
    lv_obj_t* btn_override;

    // Unified Settings Hub (F1..F5)
    lv_obj_t* btn_save_config;
    lv_obj_t* lbl_save_btn;
    lv_obj_t* tab_btns[5];
    lv_obj_t* tab_containers[5];
    uint8_t   active_settings_tab;
    bool      is_settings_dirty;

    // Diagnostic widgets
    lv_obj_t* in_lamps[8];
    lv_obj_t* out_lamps[10];

    // Day-Lock Screen widgets
    lv_obj_t* lbl_dl_stage;
    lv_obj_t* lbl_dl_days;
    lv_obj_t* lbl_dl_jalali;
    lv_obj_t* lbl_dl_challenge;
    lv_obj_t* lbl_dl_input;
    lv_obj_t* lbl_dl_status;

    // Stepper Value Display Labels
    lv_obj_t* lbl_step_mix_time;
    lv_obj_t* lbl_step_water_liters;
    lv_obj_t* lbl_step_flowrate;
    lv_obj_t* lbl_step_temp_target;
    lv_obj_t* lbl_step_brake;

    lv_obj_t* lbl_step_rtc_hour;
    lv_obj_t* lbl_step_rtc_min;
    lv_obj_t* lbl_step_shift1;
    lv_obj_t* lbl_step_shift2;
    lv_obj_t* lbl_step_shift3;
    lv_obj_t* lbl_step_pretemp;
    lv_obj_t* lbl_step_active_recipe;
    lv_obj_t* lbl_step_flour_mode;

    haffler_screen_id_t current_screen;
} haffler_ui_ctx_t;

extern haffler_ui_ctx_t g_ui_ctx;

/**
 * @brief Refreshes stepper numeric labels from live machine FSM configuration.
 */
void haffler_ui_refresh_steppers(void);

/**
 * @brief Initializes UI context and creates the complete multi-screen application.
 */
void haffler_ui_builder_init(const char* machine_title, const char* hero_title, 
                             const char* hero_unit, float default_val);

/**
 * @brief Transitions active display screen with zero tear.
 */
void haffler_ui_switch_screen(haffler_screen_id_t target_screen);

/**
 * @brief Thread-safe update of the main dashboard Hero Metric value (dirty-driven).
 */
void haffler_ui_set_hero_value(float value, uint8_t decimals);

/**
 * @brief Updates the machine state badge with status string and color.
 */
void haffler_ui_set_machine_state(const char* state_str, uint32_t bg_hex);

/**
 * @brief Updates header clock and recipe.
 */
void haffler_ui_set_clock(uint8_t h, uint8_t m, uint8_t s);
void haffler_ui_set_recipe_name(const char* recipe_name);

/**
 * @brief Updates graphic process pipeline stage (0..4) and stage descriptions.
 */
void haffler_ui_set_pipeline_stage(uint8_t stage_idx, const char* cur_name, const char* next_name);

/**
 * @brief Updates mix countdown progress bar (0..100%).
 */
void haffler_ui_set_mix_progress(uint8_t percent);

/**
 * @brief Updates live sub-HUD status cards.
 */
void haffler_ui_set_water_info(float liters, float target_l);
void haffler_ui_set_temp_info(float cur_temp, float target_temp, bool ready);
void haffler_ui_set_flour_info(bool motorized, uint32_t duration_sec);
void haffler_ui_set_brake_info(bool enabled, bool currently_braking);
void haffler_ui_set_guard_status(bool closed_ok);
void haffler_ui_set_batch_count(uint32_t count);

/**
 * @brief Updates hardware 24V I/O lamps on the diagnostics screen.
 */
void haffler_ui_set_input_lamp(uint8_t input_idx, bool active);
void haffler_ui_set_output_lamp(uint8_t output_idx, bool active);

/**
 * @brief Updates Day-Lock screen data (stage, days left, challenge, Jalali date).
 */
void haffler_ui_update_day_lock_screen(uint8_t stage, uint16_t days_left, uint16_t challenge, const char* jalali_str);

/**
 * @brief Settings Hub reactive dirty-state and tab switcher.
 */
void haffler_ui_mark_settings_dirty(bool dirty);
void haffler_ui_set_settings_tab(uint8_t tab_idx);

#ifdef __cplusplus
}
#endif
