#include "haffler_touchless_nav.h"
#include "haffler_pins.h"
#include <Arduino.h>

static mode_focus_state_t s_focus_state = FOCUS_NORMAL;
static uint32_t s_last_key_activity_ms = 0;

// Bubble sort 5 elements for fast median filtering
static uint16_t get_median_5(uint16_t arr[5]) {
    for (uint8_t i = 0; i < 4; ++i) {
        for (uint8_t j = 0; j < 4 - i; ++j) {
            if (arr[j] > arr[j + 1]) {
                uint16_t tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
    return arr[2]; // Middle element
}

static uint16_t read_median_adc_mv(uint8_t pin) {
    uint16_t samples[5];
    for (uint8_t i = 0; i < 5; ++i) {
        samples[i] = (uint16_t)analogReadMilliVolts(pin);
        delayMicroseconds(50);
    }
    return get_median_5(samples);
}

void touchless_nav_init(void) {
    pinMode(PIN_KEY_F, INPUT_PULLUP);
    pinMode(PIN_KEY_M, INPUT_PULLUP);
    s_focus_state = FOCUS_NORMAL;
    s_last_key_activity_ms = millis();
}

static panel_key_t decode_ladder_key(uint16_t mv, bool is_column_f) {
    // Idle 3.3V pull-up (>3100mV) or disconnected / ground (<300mV)
    if (mv > 3100 || mv < 300) return KEY_NONE;

    if (is_column_f) {
        if (mv < 700)  return KEY_F1;
        if (mv < 1400) return KEY_F2;
        if (mv < 2100) return KEY_F3;
        if (mv < 2800) return KEY_F4;
        return KEY_F5;
    } else {
        if (mv < 700)  return KEY_SAVE;
        if (mv < 1400) return KEY_CANCEL;
        if (mv < 2100) return KEY_MODE;
        if (mv < 2800) return KEY_START;
        return KEY_STOP;
    }
}

static panel_key_t s_prev_raw_key = KEY_NONE;
static uint32_t s_last_press_ms = 0;

panel_key_t touchless_nav_scan(void) {
    // 1. Inactivity watchdog: reset focus after 10s of silence
    if (s_focus_state != FOCUS_NORMAL && (millis() - s_last_key_activity_ms > 10000)) {
        s_focus_state = FOCUS_NORMAL;
    }

    panel_key_t cur_raw = KEY_NONE;

    // 2. Scan Column 1 (F1..F5)
    uint16_t mv_f = read_median_adc_mv(PIN_KEY_F);
    panel_key_t key_f = decode_ladder_key(mv_f, true);
    if (key_f != KEY_NONE) {
        cur_raw = key_f;
    } else {
        // 3. Scan Column 2 (SAVE, CANCEL, MODE, START, STOP)
        uint16_t mv_m = read_median_adc_mv(PIN_KEY_M);
        panel_key_t key_m = decode_ladder_key(mv_m, false);
        if (key_m != KEY_NONE) {
            cur_raw = key_m;
        }
    }

    // Single-shot edge trigger with 250ms debounce
    uint32_t now = millis();
    if (cur_raw != KEY_NONE && cur_raw != s_prev_raw_key && (now - s_last_press_ms > 250)) {
        s_prev_raw_key = cur_raw;
        s_last_press_ms = now;
        s_last_key_activity_ms = now;

        if (cur_raw == KEY_MODE) {
            touchless_nav_cycle_mode();
        } else if (cur_raw == KEY_CANCEL) {
            touchless_nav_reset_focus();
        }
        return cur_raw;
    }

    if (cur_raw == KEY_NONE) {
        s_prev_raw_key = KEY_NONE;
    }

    return KEY_NONE;
}

mode_focus_state_t touchless_nav_get_focus_state(void) {
    return s_focus_state;
}

mode_focus_state_t touchless_nav_cycle_mode(void) {
    switch (s_focus_state) {
        case FOCUS_NORMAL:
            s_focus_state = FOCUS_STEPPER_PARAM;
            break;
        case FOCUS_STEPPER_PARAM:
            s_focus_state = FOCUS_ACTIVE_EDIT;
            break;
        case FOCUS_ACTIVE_EDIT:
            s_focus_state = FOCUS_NORMAL;
            break;
    }
    return s_focus_state;
}

void touchless_nav_reset_focus(void) {
    s_focus_state = FOCUS_NORMAL;
}
