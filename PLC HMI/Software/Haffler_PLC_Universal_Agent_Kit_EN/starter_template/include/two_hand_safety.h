#pragma once
#include <Arduino.h>

class TwoHandSafety {
private:
    uint32_t m_press_time_b1;
    uint32_t m_press_time_b2;
    bool     m_b1_active;
    bool     m_b2_active;
    bool     m_cycle_triggered;

public:
    TwoHandSafety() : m_press_time_b1(0), m_press_time_b2(0),
                      m_b1_active(false), m_b2_active(false), m_cycle_triggered(false) {}

    bool evaluate(bool b1_pressed, bool b2_pressed) {
        uint32_t now = millis();

        if (b1_pressed && !m_b1_active) m_press_time_b1 = now;
        if (b2_pressed && !m_b2_active) m_press_time_b2 = now;

        m_b1_active = b1_pressed;
        m_b2_active = b2_pressed;

        if (!b1_pressed && !b2_pressed) {
            m_cycle_triggered = false;
            return false;
        }

        if (m_cycle_triggered) return false;

        if (b1_pressed && b2_pressed) {
            int32_t delta = abs((int32_t)(m_press_time_b1 - m_press_time_b2));
            if (delta <= 500) {
                m_cycle_triggered = true;
                return true;
            }
        }
        return false;
    }
};
