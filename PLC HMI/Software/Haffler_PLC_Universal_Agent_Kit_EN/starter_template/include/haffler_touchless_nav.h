#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file haffler_touchless_nav.h
 * @brief Haffler 100% Touchless Front-Panel Tactile Navigation Engine
 * Dual ADC Ladder Median Filtering + 3-State MODE Focus Traversal FSM
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEY_NONE = 0,
    // Column 1 (KEY_F - GPIO 1)
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    // Column 2 (KEY_M - GPIO 2)
    KEY_SAVE,
    KEY_CANCEL,
    KEY_MODE,
    KEY_START,
    KEY_STOP
} panel_key_t;

typedef enum {
    FOCUS_NORMAL = 0,        // Standard screen navigation (F1..F5 tabs)
    FOCUS_STEPPER_PARAM,     // Parameter cards selected via UP/DOWN
    FOCUS_ACTIVE_EDIT        // Parameter being edited via UP/DOWN increments
} mode_focus_state_t;

// Initialize tactile keypad engine on ADC1
void touchless_nav_init(void);

// Read raw keypad state with 5-sample median filter (call in Core 0 UI loop)
panel_key_t touchless_nav_scan(void);

// Current focus state of the 3-state MODE button FSM
mode_focus_state_t touchless_nav_get_focus_state(void);

// Cycle focus state when MODE key is pressed
mode_focus_state_t touchless_nav_cycle_mode(void);

// Reset focus state to FOCUS_NORMAL upon 10s inactivity or CANCEL press
void touchless_nav_reset_focus(void);

#ifdef __cplusplus
}
#endif
