#pragma once
/**
 * @file haffler_ui_builder.h
 * @brief High-Level Apple HIG UI Screen Builder for LVGL 9
 * @version 1.0.0
 * 
 * Enables autonomous AI agents to construct complete, 50+ FPS Apple HIG screens
 * with zero layout drift, guaranteed 44px touch targets, zero software shadows,
 * and mathematically centered typography.
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
    HAFFLER_SCREEN_DIAGNOSTICS,
    HAFFLER_SCREEN_DAY_LOCK
} haffler_screen_id_t;

typedef struct {
    lv_obj_t* scr_main;
    lv_obj_t* scr_settings;
    lv_obj_t* scr_diagnostics;
    lv_obj_t* scr_day_lock;

    // Main Dashboard widgets
    lv_obj_t* lbl_title;
    lv_obj_t* badge_state;
    lv_obj_t* badge_daylock;
    hero_metric_t hero_metric;
    lv_obj_t* footer_f1;
    lv_obj_t* footer_f2;
    lv_obj_t* footer_f3;
    lv_obj_t* footer_f4;
    lv_obj_t* footer_f5;

    // Diagnostic widgets
    lv_obj_t* in_lamps[8];
    lv_obj_t* out_lamps[10];

    haffler_screen_id_t current_screen;
} haffler_ui_ctx_t;

extern haffler_ui_ctx_t g_ui_ctx;

/**
 * @brief Initializes UI context and creates the complete standard 4-screen application.
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
 * @brief Updates the PackML machine state badge.
 */
void haffler_ui_set_machine_state(const char* state_str, uint32_t bg_hex);

/**
 * @brief Updates the Day-Lock lease countdown pill badge.
 */
void haffler_ui_set_day_lock_days(uint8_t remaining_days);

/**
 * @brief Updates hardware 24V I/O lamps on the diagnostics screen.
 */
void haffler_ui_set_input_lamp(uint8_t input_idx, bool active);
void haffler_ui_set_output_lamp(uint8_t output_idx, bool active);

#ifdef __cplusplus
}
#endif
