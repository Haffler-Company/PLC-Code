#pragma once
#include <Arduino.h>
#include "haffler_pins.h"

enum CylinderState {
    CYL_RETRACTED = 0,
    CYL_ADVANCING,
    CYL_ADVANCED,
    CYL_RETRACTING,
    CYL_FAULT_TIMEOUT
};

extern void relay_set(uint8_t idx, bool active);

class PneumaticCylinder {
private:
    uint8_t       m_relay_adv;
    uint8_t       m_relay_ret;
    int8_t        m_sen_adv;
    int8_t        m_sen_ret;
    uint32_t      m_timeout_ms;
    uint32_t      m_start_ms;
    CylinderState m_state;
    const char*   m_name;

public:
    PneumaticCylinder(const char* name, uint8_t r_adv, uint8_t r_ret,
                      int8_t s_adv, int8_t s_ret, uint32_t timeout_ms = 2000)
        : m_name(name), m_relay_adv(r_adv), m_relay_ret(r_ret),
          m_sen_adv(s_adv), m_sen_ret(s_ret),
          m_timeout_ms(timeout_ms), m_start_ms(0), m_state(CYL_RETRACTED) {}

    void advance() {
        if (m_relay_ret != 255) relay_set(m_relay_ret, false);
        relay_set(m_relay_adv, true);
        m_start_ms = millis();
        m_state = CYL_ADVANCING;
    }

    void retract() {
        relay_set(m_relay_adv, false);
        if (m_relay_ret != 255) relay_set(m_relay_ret, true);
        m_start_ms = millis();
        m_state = CYL_RETRACTING;
    }

    CylinderState update(const bool inputs[]) {
        uint32_t elapsed = millis() - m_start_ms;

        if (m_state == CYL_ADVANCING) {
            bool reached = (m_sen_adv >= 0) ? (inputs[m_sen_adv] == true) : (elapsed >= m_timeout_ms);
            if (reached) {
                m_state = CYL_ADVANCED;
            } else if (elapsed > m_timeout_ms) {
                m_state = CYL_FAULT_TIMEOUT;
            }
        } else if (m_state == CYL_RETRACTING) {
            bool reached = (m_sen_ret >= 0) ? (inputs[m_sen_ret] == true) : (elapsed >= m_timeout_ms);
            if (reached) {
                m_state = CYL_RETRACTED;
            } else if (elapsed > m_timeout_ms) {
                m_state = CYL_FAULT_TIMEOUT;
            }
        }
        return m_state;
    }

    bool isAdvanced() const { return m_state == CYL_ADVANCED; }
    bool isRetracted() const { return m_state == CYL_RETRACTED; }
    bool isFault() const { return m_state == CYL_FAULT_TIMEOUT; }
    const char* getName() const { return m_name; }
};
