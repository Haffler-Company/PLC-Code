# 16. Pneumatic Actuators, Solenoid Sequences, and Motion Control (Stepper / Servo)

## Executive Summary
Over 90% of Haffler industrial machines (single/double-head UPVC cutters, welders, corner cleaners, aluminum punchers, gas fillers) rely on a combination of pneumatic cylinders for profile clamping/actuation and stepper or servo motors for precision linear positioning. 

This document details deterministic pneumatic cylinder control, a battle-tested C++ cylinder management class with reed-switch timeout detection, and high-frequency motion control with left-only mechanical backlash compensation.

---

## 1. Industrial Pneumatic Cylinder Control Architecture

In noisy workshop environments, air compressor pressure fluctuations, valve spool sticking, or mechanical jams can cause severe machine damage (e.g., advancing a saw blade while profile clamps are loose). Cylinders must never be actuated on blind delays alone.

### The 3 Pillars of Pneumatic Safety:
1. **Dual Reed Switches:** High-risk cylinders feature two magnetic position sensors: retracted limit (`RETRACT_SENSOR`) and advanced limit (`ADVANCE_SENSOR`).
2. **Motion Timeout Fault Detection:** If a cylinder fails to reach its target limit switch within a defined deadline (e.g., $1500\text{ ms}$), motion is aborted immediately and an audible alarm is raised.
3. **Failsafe Auto-Retract:** On Emergency Stop ($I_0$) or line pressure collapse, high-hazard actuators (saw blade ascend cylinders, punch rams) must de-energize and mechanically retract to a safe state.

### Battle-Tested C++ Cylinder Manager Class
```cpp
#pragma once
#include <stdint.h>
#include <stdbool.h>

enum CylinderState {
    CYL_RETRACTED,
    CYL_ADVANCING,
    CYL_ADVANCED,
    CYL_RETRACTING,
    CYL_FAULT_TIMEOUT
};

class PneumaticCylinder {
private:
    uint8_t       m_relay_advance;   // Relay index for advance solenoid
    uint8_t       m_relay_retract;   // Relay index for retract solenoid (255 if single-solenoid spring return)
    int8_t        m_sensor_advance;  // Optocoupler input index for advance sensor (-1 if none)
    int8_t        m_sensor_retract;  // Optocoupler input index for retract sensor (-1 if none)
    uint32_t      m_timeout_ms;      // Maximum allowed travel time in milliseconds
    uint32_t      m_move_start_time; // Millisecond timestamp when stroke initiated
    CylinderState m_state;
    const char*   m_name;

public:
    PneumaticCylinder(const char* name, uint8_t relay_adv, uint8_t relay_ret,
                      int8_t sen_adv, int8_t sen_ret, uint32_t timeout_ms = 2000)
        : m_name(name), m_relay_advance(relay_adv), m_relay_retract(relay_ret),
          m_sensor_advance(sen_adv), m_sensor_retract(sen_ret),
          m_timeout_ms(timeout_ms), m_move_start_time(0), m_state(CYL_RETRACTED) {}

    // Initiate advance stroke
    void advance() {
        if (m_relay_retract != 255) plc_relay_set(m_relay_retract, false);
        plc_relay_set(m_relay_advance, true);
        m_move_start_time = millis();
        m_state = CYL_ADVANCING;
    }

    // Initiate retract stroke
    void retract() {
        plc_relay_set(m_relay_advance, false);
        if (m_relay_retract != 255) plc_relay_set(m_relay_retract, true);
        m_move_start_time = millis();
        m_state = CYL_RETRACTING;
    }

    // Call inside Core 1 50ms deterministic scan loop
    CylinderState update(const bool input_states[]) {
        uint32_t elapsed = millis() - m_move_start_time;

        if (m_state == CYL_ADVANCING) {
            bool reached = (m_sensor_advance >= 0) ? (input_states[m_sensor_advance] == true) : (elapsed >= m_timeout_ms);
            if (reached) {
                m_state = CYL_ADVANCED;
            } else if (elapsed > m_timeout_ms) {
                m_state = CYL_FAULT_TIMEOUT;
            }
        } 
        else if (m_state == CYL_RETRACTING) {
            bool reached = (m_sensor_retract >= 0) ? (input_states[m_sensor_retract] == true) : (elapsed >= m_timeout_ms);
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
```

---

## 2. Stepper & Servo Motion Control (Step / Direction Generation)

High-speed carriage positioning on linear guides requires precise microsecond pulse generation and smooth acceleration profiles.

### Motion Interface Signals:
- **PULSE (STEP):** Frequency-modulated pulse train determining motor rotational speed.
- **DIRECTION (DIR):** Logic level determining travel direction ($0 = \text{Forward/Right}, 1 = \text{Reverse/Left}$).
- **ENABLE (ENA):** Active-LOW power bridge control ($0 = \text{Holding Torque Active}, 1 = \text{Freewheeling}$).

### Trapezoidal Acceleration Profile
Starting a stepper motor abruptly at full target velocity causes immediate torque collapse and lost steps. The motion profile consists of 3 distinct phases:
1. **Acceleration Phase:** Linear ramp from $F_{\text{start}}$ (e.g. $500\text{ Hz}$) to $F_{\text{max}}$ (e.g. $10\text{ kHz}$).
2. **Cruise Phase:** Constant velocity motion at $F_{\text{max}}$.
3. **Deceleration Phase:** Controlled deceleration to a gentle stop exactly on the destination pulse.

---

## 3. Unilateral Left-Only Backlash Compensation

In rack-and-pinion and ball-screw drivetrains found in industrial workshops, mechanical gear clearance produces between $0.2\text{ mm}$ and $0.5\text{ mm}$ of play (backlash).

### The Golden Law of Left-Approach:
To eliminate mechanical play entirely, **the axis must always arrive at its final target coordinate from the exact same physical direction (Left to Right)**.

- **Forward Move ($Target > Current$):** Axis moves directly to destination with standard acceleration and stops.
- **Reverse Move ($Target < Current$):**
  1. Axis intentional overshoots the destination by a calibrated distance:
     $$\text{Overshoot Pos} = Target - \text{BacklashDistance} \quad (\text{e.g. } Target - 3.0\text{ mm})$$
  2. Axis halts completely.
  3. Axis reverses direction and creeps forward at low speed by $3.0\text{ mm}$, loading the gear teeth in the positive direction before locking on target.

This software technique reduces positioning error on worn mechanical gearboxes to **under $0.05\text{ mm}$**.

---

## 4. Hardware Homing State Machine (Homing FSM)

At system power-on, the carriage location is uncalibrated. The automatic homing routine establishes the absolute machine datum:

1. **SEEK_FAST:** Drive carriage in reverse at $60\%$ speed toward rear limit switch.
2. **SWITCH_HIT:** Hardware limit switch opens on contact; pulse train halts instantly.
3. **RETRACT_SLOW:** Drive carriage forward by $5.0\text{ mm}$ to release the switch.
4. **TOUCH_PRECISE:** Approach switch at $10\%$ creep speed until precise micro-switch actuation edge.
5. **ZERO_DATUM:** Reset hardware PCNT pulse counter to $0$; load mechanical reference offset.
6. **SOFT_LIMITS_ACTIVE:** Software prevents operator jog past `SOFT_LIMIT_MAX_MM` or below `0.0 mm`.
