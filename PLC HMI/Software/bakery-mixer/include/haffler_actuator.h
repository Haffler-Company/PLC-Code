#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include "haffler_pins.h"

/**
 * @file haffler_actuator.h
 * @brief Universal Industrial Actuator & Cylinder Controller (HAL)
 * @details Handles Active-LOW relays, pneumatic single/double solenoids,
 *          dual-sensor analog voltage bands (GPIO 4, 5, 6), and travel timeout watchdogs.
 */

enum ActuatorType {
    ACTUATOR_SINGLE_SOLENOID = 0, // 1 relay: ENERGIZED = Extend, DE-ENERGIZED = Retract
    ACTUATOR_DOUBLE_SOLENOID,     // 2 relays: Relay A = Extend pulse, Relay B = Retract pulse
    ACTUATOR_MOTOR_CONTACTOR      // Simple run contactor
};

enum CylinderPosition {
    CYL_POS_RETRACTED = 0,
    CYL_POS_IN_TRANSIT,
    CYL_POS_EXTENDED,
    CYL_POS_FAULT_TIMEOUT,
    CYL_POS_UNKNOWN
};

class HafflerActuator {
private:
    const char*      m_name;
    uint8_t          m_relay_ext_pin;
    uint8_t          m_relay_ret_pin;
    ActuatorType     m_type;
    int8_t           m_sensor_analog_pin; // GPIO 4, 5, 6 or -1 if none
    uint32_t         m_travel_timeout_ms;
    uint32_t         m_motion_start_ms;
    bool             m_command_extend;
    CylinderPosition m_last_position;
    bool             m_timeout_fault;

public:
    HafflerActuator(const char* name, uint8_t relay_ext, int8_t sensor_pin = -1,
                    uint32_t timeout_ms = 3000)
        : m_name(name), m_relay_ext_pin(relay_ext), m_relay_ret_pin(255),
          m_type(ACTUATOR_SINGLE_SOLENOID), m_sensor_analog_pin(sensor_pin),
          m_travel_timeout_ms(timeout_ms), m_motion_start_ms(0),
          m_command_extend(false), m_last_position(CYL_POS_UNKNOWN),
          m_timeout_fault(false) {}

    void init() {
        // Safe clamping on boot: Active-LOW (1 = OFF)
        gpio_set_level((gpio_num_t)m_relay_ext_pin, 1);
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << m_relay_ext_pin),
            .mode = GPIO_MODE_INPUT_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }

    void extend(uint32_t now_ms = 0) {
        if (m_timeout_fault) return;
        m_command_extend = true;
        m_motion_start_ms = (now_ms == 0) ? millis() : now_ms;
        gpio_set_level((gpio_num_t)m_relay_ext_pin, 0); // Active-LOW: 0 = ON
    }

    void retract(uint32_t now_ms = 0) {
        m_command_extend = false;
        m_motion_start_ms = (now_ms == 0) ? millis() : now_ms;
        gpio_set_level((gpio_num_t)m_relay_ext_pin, 1); // Active-LOW: 1 = OFF
    }

    void emergencyRetract() {
        gpio_set_level((gpio_num_t)m_relay_ext_pin, 1); // Clamp to OFF
        m_command_extend = false;
        m_timeout_fault = false;
    }

    // Call from Core 1 PLC task every 50ms
    void update(uint32_t now_ms, int adc_mv = -1) {
        // 1. Evaluate Sensor Position
        if (m_sensor_analog_pin >= 0 && adc_mv >= 0) {
            if (adc_mv >= SENSOR_BAND_CLOSED) {
                m_last_position = CYL_POS_EXTENDED;
            } else if (adc_mv >= SENSOR_BAND_OPEN_LO && adc_mv <= SENSOR_BAND_OPEN_HI) {
                m_last_position = CYL_POS_RETRACTED;
            } else if (adc_mv >= SENSOR_BAND_NONE_LO && adc_mv <= SENSOR_BAND_NONE_HI) {
                m_last_position = CYL_POS_IN_TRANSIT;
            }
        }

        // 2. Evaluate Travel Watchdog Timeout
        if (m_motion_start_ms > 0 && m_travel_timeout_ms > 0) {
            uint32_t elapsed = now_ms - m_motion_start_ms;
            if (elapsed > m_travel_timeout_ms) {
                if (m_command_extend && m_last_position != CYL_POS_EXTENDED) {
                    m_timeout_fault = true;
                    m_last_position = CYL_POS_FAULT_TIMEOUT;
                    emergencyRetract(); // Auto-retract on jam
                } else if (!m_command_extend && m_last_position != CYL_POS_RETRACTED) {
                    m_timeout_fault = true;
                    m_last_position = CYL_POS_FAULT_TIMEOUT;
                }
            }
        }
    }

    bool isExtended() const { return m_last_position == CYL_POS_EXTENDED; }
    bool isRetracted() const { return m_last_position == CYL_POS_RETRACTED; }
    bool is_extended() const { return isExtended(); }
    bool is_retracted() const { return isRetracted(); }
    bool isFault() const { return m_timeout_fault; }
    void clearFault() { m_timeout_fault = false; }
    const char* getName() const { return m_name; }
    CylinderPosition getPosition() const { return m_last_position; }
};
