#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>

/**
 * @file haffler_ui_lang.h
 * @brief Haffler Zero-Leak Trilingual Localization Engine (FA / EN / TR)
 * Zero Dynamic Allocation + Apple Pill Capsule Switcher + Zero Tofu Character Mapping
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LANG_FA = 0, // Persian (RTL)
    LANG_EN = 1, // English (LTR)
    LANG_TR = 2  // Turkish (LTR)
} app_lang_t;

typedef enum {
    STR_READY = 0,
    STR_RUNNING,
    STR_STOPPED,
    STR_TARGET_VAL,
    STR_ACTUAL_VAL,
    STR_PIECE_COUNT,
    STR_SETTINGS,
    STR_DIAGNOSTICS,
    STR_DAY_LOCK_WARN,
    STR_CONFIRM,
    STR_CANCEL,
    STR_RESET,
    STR_ERROR_ESTOP,
    STR_MIX_TIME,
    STR_WATER_TEMP,
    STR_WATER_DOSE,
    STR_DOSING_MODE,
    STR_FLOWMETER,
    STR_TIMER,
    STR_GUARD_OPEN,
    STR_TEMP_HIGH,
    STR_OVERLOAD,
    STR_START_BTN,
    STR_STOP_BTN,
    STR_BATCH_COUNT,
    STR_SCHEDULE,
    STR_DAY_LOCK,
    STR_CHALLENGE,
    STR_ENTER_PIN,
    STR_UNLOCK_SUCCESS,
    STR_UNLOCK_FAIL,
    STR_CLOCK_SET,
    STR_HOUR,
    STR_MINUTE,
    STR_PRE_TEMP,
    STR_FLOUR_FEED,
    STR_BRAKE_BOWL,
    STR_BACK_BTN,
    STR_NAV_F1,
    STR_NAV_F2,
    STR_NAV_F3,
    STR_NAV_F4,
    STR_NAV_F5,
    STR_MAX_KEYS
} str_id_t;

// Initialize language subsystem (loads active language from NVS, default: LANG_EN)
void ui_lang_init(app_lang_t default_lang);

// Get localized text for a string ID
const char* ui_lang_get(str_id_t id);

// Set active language and immediately refresh all registered labels
void ui_lang_set(app_lang_t lang);

// Get currently active language
app_lang_t ui_lang_current(void);

// Register an LVGL label to be automatically updated upon language switch
void ui_lang_register_label(lv_obj_t *lbl, str_id_t str_id);

// Create an Apple Pill capsule language switcher badge (52x24) on parent
lv_obj_t* ui_lang_create_pill_badge(lv_obj_t *parent, lv_coord_t x, lv_coord_t y);

#ifdef __cplusplus
}
#endif
