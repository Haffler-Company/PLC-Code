/**
 * @file plc_engine.h
 * @brief Industrial 14-Step Welding Cycle FSM & Process Safety Interlock Engine
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WELD_STEP_0_IDLE = 0,
    WELD_STEP_1_PRESS_LEFT,
    WELD_STEP_2_PRESS_RIGHT,
    WELD_STEP_3_SHABLON_RETRACT,
    WELD_STEP_4_ELEMENT_ADVANCE,
    WELD_STEP_5_TABLE_MELT_ADVANCE,
    WELD_STEP_6_MELTING_COUNTDOWN,
    WELD_STEP_7_TABLE_MELT_RETRACT,
    WELD_STEP_8_ELEMENT_RETRACT,
    WELD_STEP_9_TABLE_FUSION_ADVANCE,
    WELD_STEP_10_COOLING_COUNTDOWN,
    WELD_STEP_11_STRIKER_ENGAGE,
    WELD_STEP_12_TABLE_RETRACT_CLAMPS_OPEN,
    WELD_STEP_13_SHABLON_RETURN_COMPLETE
} plc_weld_step_t;

typedef struct {
    plc_weld_step_t step;
    bool in_progress;
    bool emergency_stopped;
    uint32_t step_elapsed_ms;
    uint32_t remaining_countdown_sec;
    float current_temperature_c;
    float target_temperature_c;
    bool heater_ssr_state;
} plc_cycle_status_t;

esp_err_t plc_engine_init(void);

/**
 * @brief Main periodic tick for Core 0 PLC State Machine (10-20ms cadence)
 */
void plc_engine_tick(void);

/**
 * @brief Trigger foot pedal or panel start event
 */
void plc_engine_pedal_trigger(void);

/**
 * @brief Abort current cycle safely and de-energize all jacks
 */
void plc_engine_abort(void);

/**
 * @brief Get snapshot of current cycle state (thread-safe for UI)
 */
plc_cycle_status_t plc_engine_get_status(void);

#ifdef __cplusplus
}
#endif
