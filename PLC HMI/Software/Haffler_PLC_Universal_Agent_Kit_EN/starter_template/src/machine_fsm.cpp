#include "machine_fsm.h"
#include <Arduino.h>

MachineFSM g_machine_fsm;

void MachineFSM::init() {
    m_state = STATE_IDLE;
    m_current_step = 0;
    m_total_steps = 0;
    m_state_entered_ms = millis();
}

void MachineFSM::setState(MachineState new_state) {
    if (m_state == STATE_ESTOP && new_state != STATE_IDLE) {
        // Cannot leave E-Stop without explicit reset to IDLE
        return;
    }
    m_state = new_state;
    m_state_entered_ms = millis();
}

void MachineFSM::startCycle(uint8_t total_steps) {
    if (m_state != STATE_IDLE) return;

    m_total_steps = total_steps;
    m_current_step = 1;
    m_cycle_start_ms = millis();
    m_state_entered_ms = millis();
    m_state = STATE_AUTO_CYCLE;
}

void MachineFSM::advanceStep() {
    if (m_state != STATE_AUTO_CYCLE) return;

    m_current_step++;
    m_state_entered_ms = millis();

    if (m_current_step > m_total_steps) {
        // Cycle completed successfully
        m_last_cycle_duration_ms = millis() - m_cycle_start_ms;
        m_total_parts_counter++;
        m_state = STATE_IDLE;
        m_current_step = 0;
    }
}

bool MachineFSM::isStepTimeout(uint32_t timeout_ms) const {
    if (timeout_ms == 0) return false;
    return (millis() - m_state_entered_ms) > timeout_ms;
}

void MachineFSM::triggerAlarm(uint16_t alarm_code) {
    m_last_alarm_code = alarm_code;
    m_state = STATE_ALARM;
    m_state_entered_ms = millis();
}

void MachineFSM::clearAlarm() {
    if (m_state == STATE_ALARM) {
        m_last_alarm_code = 0;
        m_state = STATE_IDLE;
        m_current_step = 0;
    }
}

void MachineFSM::triggerEStop() {
    m_state = STATE_ESTOP;
    m_current_step = 0;
    m_state_entered_ms = millis();
}

void MachineFSM::resetEStop() {
    if (m_state == STATE_ESTOP) {
        m_state = STATE_IDLE;
        m_current_step = 0;
    }
}
