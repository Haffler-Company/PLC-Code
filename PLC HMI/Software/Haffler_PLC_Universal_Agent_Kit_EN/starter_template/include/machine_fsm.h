#pragma once
#include <stdint.h>
#include <stdbool.h>

enum MachineState {
    STATE_BOOT = 0,
    STATE_HOMING,
    STATE_IDLE,
    STATE_STARTING,
    STATE_AUTO_CYCLE,
    STATE_HOLDING,
    STATE_HELD,
    STATE_STOPPING,
    STATE_STOPPED,
    STATE_ALARM,
    STATE_ESTOP
};

typedef struct {
    uint8_t  step_id;
    const char* name;
    uint32_t timeout_ms;
    uint32_t started_ms;
} cycle_step_t;

class MachineFSM {
private:
    MachineState m_state;
    uint8_t      m_current_step;
    uint8_t      m_total_steps;
    uint32_t     m_state_entered_ms;
    uint32_t     m_cycle_start_ms;
    uint32_t     m_last_cycle_duration_ms;
    uint32_t     m_total_parts_counter;
    uint16_t     m_last_alarm_code;

public:
    MachineFSM() : m_state(STATE_BOOT), m_current_step(0), m_total_steps(0),
                   m_state_entered_ms(0), m_cycle_start_ms(0), m_last_cycle_duration_ms(0),
                   m_total_parts_counter(0), m_last_alarm_code(0) {}

    void init();
    void setState(MachineState new_state);
    MachineState getState() const { return m_state; }

    void startCycle(uint8_t total_steps);
    void advanceStep();
    uint8_t getCurrentStep() const { return m_current_step; }
    bool isStepTimeout(uint32_t timeout_ms) const;

    void triggerAlarm(uint16_t alarm_code);
    void clearAlarm();
    uint16_t getAlarmCode() const { return m_last_alarm_code; }

    void triggerEStop();
    void resetEStop();

    uint32_t getCycleDurationMs() const { return m_last_cycle_duration_ms; }
    uint32_t getPartsCount() const { return m_total_parts_counter; }
    void incrementPartsCount() { m_total_parts_counter++; }
    void resetPartsCount() { m_total_parts_counter = 0; }
};

extern MachineFSM g_machine_fsm;
