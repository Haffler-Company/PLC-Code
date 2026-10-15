#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file haffler_sequencer.h
 * @brief Industrial Deterministic Step Sequencer Engine (Siemens IEC 61131 SFC / PackML)
 * @details Lightweight, zero-heap, deterministic step-sequencing state machine.
 *          Designed for ESP32-S3 Core 1 real-time cycle (50ms scan).
 */

#define MAX_SEQUENCER_STEPS 16

typedef void (*StepActionFn)(void);
typedef bool (*StepConditionFn)(void);

typedef struct {
    uint8_t         step_id;
    const char*     name;
    uint32_t        timeout_ms;
    StepActionFn    on_enter;
    StepConditionFn is_complete;
} SequencerStep_t;

enum SequencerState {
    SEQ_STATE_IDLE = 0,
    SEQ_STATE_RUNNING,
    SEQ_STATE_PAUSED,
    SEQ_STATE_COMPLETED,
    SEQ_STATE_TIMEOUT_FAULT,
    SEQ_STATE_ABORTED
};

class HafflerSequencer {
private:
    SequencerStep_t m_steps[MAX_SEQUENCER_STEPS];
    uint8_t         m_step_count;
    int8_t          m_current_index;
    SequencerState  m_state;
    uint32_t        m_step_start_ms;
    uint32_t        m_cycle_start_ms;
    uint32_t        m_total_cycle_ms;
    uint8_t         m_fault_step_id;

public:
    HafflerSequencer() : m_step_count(0), m_current_index(-1),
                         m_state(SEQ_STATE_IDLE), m_step_start_ms(0),
                         m_cycle_start_ms(0), m_total_cycle_ms(0), m_fault_step_id(0) {}

    void reset() {
        m_step_count = 0;
        m_current_index = -1;
        m_state = SEQ_STATE_IDLE;
        m_fault_step_id = 0;
    }

    bool addStep(uint8_t step_id, const char* name, uint32_t timeout_ms,
                 StepActionFn on_enter, StepConditionFn is_complete) {
        if (m_step_count >= MAX_SEQUENCER_STEPS) return false;
        m_steps[m_step_count] = {step_id, name, timeout_ms, on_enter, is_complete};
        m_step_count++;
        return true;
    }

    bool add_step(uint8_t step_id, const char* name, uint32_t timeout_ms,
                  StepActionFn on_enter, StepConditionFn is_complete) {
        return addStep(step_id, name, timeout_ms, on_enter, is_complete);
    }

    bool start(uint32_t now_ms) {
        if (m_step_count == 0) return false;
        m_current_index = 0;
        m_state = SEQ_STATE_RUNNING;
        m_cycle_start_ms = now_ms;
        m_step_start_ms = now_ms;
        if (m_steps[0].on_enter) m_steps[0].on_enter();
        return true;
    }

    void abort() {
        m_state = SEQ_STATE_ABORTED;
        m_current_index = -1;
    }

    void update(uint32_t now_ms) {
        if (m_state != SEQ_STATE_RUNNING || m_current_index < 0) return;

        const SequencerStep_t* cur = &m_steps[m_current_index];
        uint32_t elapsed = now_ms - m_step_start_ms;

        // 1. Check Step Travel Timeout
        if (cur->timeout_ms > 0 && elapsed > cur->timeout_ms) {
            m_state = SEQ_STATE_TIMEOUT_FAULT;
            m_fault_step_id = cur->step_id;
            abort();
            return;
        }

        // 2. Check Step Completion Condition
        if (cur->is_complete && cur->is_complete()) {
            m_current_index++;
            if (m_current_index >= m_step_count) {
                // Whole cycle complete
                m_state = SEQ_STATE_COMPLETED;
                m_total_cycle_ms = now_ms - m_cycle_start_ms;
                m_current_index = -1;
            } else {
                // Advance to next step
                m_step_start_ms = now_ms;
                if (m_steps[m_current_index].on_enter) {
                    m_steps[m_current_index].on_enter();
                }
            }
        }
    }

    SequencerState getState() const { return m_state; }
    int8_t getCurrentStepIndex() const { return m_current_index; }
    uint8_t getCurrentStepId() const {
        return (m_current_index >= 0 && m_current_index < m_step_count) ? m_steps[m_current_index].step_id : 0;
    }
    const char* getCurrentStepName() const {
        return (m_current_index >= 0 && m_current_index < m_step_count) ? m_steps[m_current_index].name : "IDLE";
    }
    const char* get_active_step_name() const { return getCurrentStepName(); }
    uint8_t getFaultStepId() const { return m_fault_step_id; }
    uint32_t getTotalCycleMs() const { return m_total_cycle_ms; }
    uint32_t getStepElapsedMs(uint32_t now_ms) const {
        return (m_state == SEQ_STATE_RUNNING) ? (now_ms - m_step_start_ms) : 0;
    }
};
