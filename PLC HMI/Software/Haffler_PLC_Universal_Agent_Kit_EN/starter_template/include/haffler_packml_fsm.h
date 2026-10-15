#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file haffler_packml_fsm.h
 * @brief PackML ISA-TR88.00.02 Industrial State Machine Engine for Core 1 (50ms Cycle)
 * Standardized Machine States, Transitions, Safety Interlocks & Cycle Time Profiling
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATE_UNDEFINED = 0,
    STATE_CLEARING,
    STATE_STOPPED,
    STATE_STARTING,
    STATE_IDLE,
    STATE_SUSPENDED,
    STATE_EXECUTE,
    STATE_STOPPING,
    STATE_ABORTING,
    STATE_ABORTED,
    STATE_HOLDING,
    STATE_HELD,
    STATE_UNHOLDING,
    STATE_SUSPENDING,
    STATE_UNSUSPENDING,
    STATE_RESETTING,
    STATE_COMPLETING,
    STATE_COMPLETE
} packml_state_t;

typedef struct {
    packml_state_t current_state;
    packml_state_t previous_state;
    uint32_t       state_entry_time_ms;
    uint32_t       cycle_duration_ms;
    uint32_t       total_pieces_produced;
    bool           emergency_stop_latched;
    uint8_t        active_alarm_code;
} packml_fsm_t;

// Initialize state machine
static inline void packml_init(packml_fsm_t *fsm) {
    if (!fsm) return;
    fsm->current_state = STATE_STOPPED;
    fsm->previous_state = STATE_UNDEFINED;
    fsm->state_entry_time_ms = 0;
    fsm->cycle_duration_ms = 0;
    fsm->total_pieces_produced = 0;
    fsm->emergency_stop_latched = false;
    fsm->active_alarm_code = 0;
}

// Transition state and record entry timestamp
static inline void packml_transition(packml_fsm_t *fsm, packml_state_t next_state, uint32_t now_ms) {
    if (!fsm || fsm->current_state == next_state) return;
    fsm->previous_state = fsm->current_state;
    fsm->current_state = next_state;
    fsm->state_entry_time_ms = now_ms;
}

// Get string name for PackML state
static inline const char* packml_state_name(packml_state_t state) {
    switch (state) {
        case STATE_STOPPED:    return "STOPPED";
        case STATE_STARTING:   return "STARTING";
        case STATE_IDLE:       return "IDLE / READY";
        case STATE_EXECUTE:    return "RUNNING";
        case STATE_STOPPING:   return "STOPPING";
        case STATE_ABORTING:   return "ABORTING";
        case STATE_ABORTED:    return "ABORTED (E-STOP)";
        case STATE_HOLDING:    return "HOLDING";
        case STATE_HELD:       return "HELD";
        case STATE_COMPLETING: return "COMPLETING";
        case STATE_COMPLETE:   return "COMPLETE";
        default:               return "UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif
