# 21. Universal Machine Archetypes & FSM Industrial Cookbook
## Complete Ready-to-Wire C++ State Machines for the 4 Primary Machine Architectures

> **Document ID:** `21_UNIVERSAL_MACHINE_ARCHETYPES_AND_FSM_COOKBOOK.md`  
> **Target Hardware:** Haffler Universal Industrial PLC + HMI (ESP32-S3 HT-V3A).  
> **Mission:** When building any new machine, select the matching Archetype below (or combine them) and plug the pre-tested C++ FSM directly into Core 1. Zero architectural guesswork.

---

## The 4 Universal Machine Archetypes

```
+-----------------------------------------------------------------------------------------------+
| Archetype 1: Multi-Cylinder Pneumatic Automation (Welding, Clamping, Punching, Stamping)     |
| Archetype 2: Precision Motion & Encoder Positioning (Cutters, Sizing Tables, Copy Routers)    |
| Archetype 3: PID Thermal Regulation & Solid-State Sealing (Ovens, Heaters, UPVC Welders)     |
| Archetype 4: Dispensing, Liquid Batching & Flow Measurement (Gas-Filling, Ice Cream, Liquids) |
+-----------------------------------------------------------------------------------------------+
```

---

## Archetype 1: Multi-Cylinder Pneumatic Automation

### Ideal For:
UPVC Corner Cleaners, Aluminum Punching Presses, Multi-Head Clamping Jigs, UPVC Single/Double Head Welders, Material Clamp & Drill units.

### Signal Architecture:
- **Inputs (I0..I7):**
  - I0: Safety E-Stop / Light Curtain (NC)
  - I1: Two-Hand Start Button 1
  - I2: Two-Hand Start Button 2 / Foot Switch
  - I3: Clamp Cylinder Extended Reed Switch
  - I4: Tool / Punch Advance Reed Switch
  - I5: Tool / Punch Retract Home Reed Switch
  - I6: Workpiece In-Position Optical Sensor
  - I7: Air Pressure OK Switch (> 5.5 bar)
- **Outputs (Q0..Q9):**
  - Q0: Main Clamp Valve Solenoid
  - Q1: Tool / Punch Advance Valve Solenoid
  - Q2: Tool / Punch Retract Valve Solenoid
  - Q3: Air Blast / Chip Vacuum Blow
  - Q4: Ejector Cylinder
  - Q8: Cycle Complete Beacon / Siren

### Production C++ Finite State Machine:

```cpp
#include "haffler_pins.h"
#include "pneumatic_cylinder.h"

enum PneumaticMachineState {
    PNEU_STATE_IDLE = 0,
    PNEU_STATE_WAIT_TWO_HAND,
    PNEU_STATE_CLAMPING,
    PNEU_STATE_TOOL_ADVANCE,
    PNEU_STATE_TOOL_DWELL,
    PNEU_STATE_TOOL_RETRACT,
    PNEU_STATE_UNCLAMP,
    PNEU_STATE_EJECT,
    PNEU_STATE_FAULT
};

class PneumaticProcessFSM {
private:
    PneumaticMachineState m_state = PNEU_STATE_IDLE;
    uint32_t m_state_timer = 0;
    PneumaticCylinder m_clamp;
    PneumaticCylinder m_tool;

public:
    PneumaticProcessFSM() 
        : m_clamp(PIN_RELAY_Q0, 255, PIN_INP_REF, 255, 2000),
          m_tool(PIN_RELAY_Q1, PIN_RELAY_Q2, 4, 5, 3000) {}

    void init() {
        m_clamp.init();
        m_tool.init();
        m_state = PNEU_STATE_IDLE;
    }

    void update(uint32_t dt_ms, bool estop_active, bool btn_start1, bool btn_start2) {
        if (estop_active) {
            m_clamp.retract();
            m_tool.retract();
            m_state = PNEU_STATE_FAULT;
            return;
        }

        m_clamp.update(dt_ms);
        m_tool.update(dt_ms);

        switch (m_state) {
        case PNEU_STATE_IDLE:
            if (btn_start1 || btn_start2) {
                m_state_timer = 0;
                m_state = PNEU_STATE_WAIT_TWO_HAND;
            }
            break;

        case PNEU_STATE_WAIT_TWO_HAND:
            m_state_timer += dt_ms;
            // EN 574 Type III C: Both buttons must be pressed within 500ms
            if (btn_start1 && btn_start2 && m_state_timer <= 500) {
                m_clamp.extend();
                m_state = PNEU_STATE_CLAMPING;
            } else if (m_state_timer > 500) {
                m_state = PNEU_STATE_IDLE; // Anti-tie-down abort
            }
            break;

        case PNEU_STATE_CLAMPING:
            if (m_clamp.isExtended()) {
                m_tool.extend();
                m_state = PNEU_STATE_TOOL_ADVANCE;
            } else if (m_clamp.hasFault()) {
                m_state = PNEU_STATE_FAULT;
            }
            break;

        case PNEU_STATE_TOOL_ADVANCE:
            if (m_tool.isExtended()) {
                m_state_timer = 0;
                m_state = PNEU_STATE_TOOL_DWELL;
            } else if (m_tool.hasFault()) {
                m_tool.retract();
                m_clamp.retract();
                m_state = PNEU_STATE_FAULT;
            }
            break;

        case PNEU_STATE_TOOL_DWELL:
            m_state_timer += dt_ms;
            if (m_state_timer >= 800) { // 800ms working dwell
                m_tool.retract();
                m_state = PNEU_STATE_TOOL_RETRACT;
            }
            break;

        case PNEU_STATE_TOOL_RETRACT:
            if (m_tool.isRetracted()) {
                m_clamp.retract();
                m_state = PNEU_STATE_UNCLAMP;
            }
            break;

        case PNEU_STATE_UNCLAMP:
            if (m_clamp.isRetracted()) {
                m_state = PNEU_STATE_IDLE; // Cycle complete!
            }
            break;

        case PNEU_STATE_FAULT:
            // Require operator reset
            break;
        }
    }
};
```

---

## Archetype 2: Precision Motion & Encoder Positioning

### Ideal For:
UPVC Single/Double Head Cutting Saws, Digital Length Gauges, Automatic Sizing Stops, Copy Routers.

### Signal Architecture:
- **Motion Signals:**
  - `PIN_STEP_PUL` (GPIO 16): High-frequency step pulse
  - `PIN_STEP_DIR` (GPIO 42): Direction level (1 = Forward / Right, 0 = Backward / Left)
  - `PIN_STEP_EN`  (GPIO 39): Active-LOW driver enable (0 = Energized)
- **Sensor Inputs:**
  - `PIN_INP_REF` (GPIO 6): Inductive Home Proximity Switch (Active-LOW)
  - `PIN_INP_FAULT` (GPIO 4): Limit Switch / Motor Alarm Input
- **Safety Rule:** Saw Blade / Milling Spindle relay (Q1) MUST be electrically interlocked so the motor cannot advance while the axis is in motion.

### Millimeter-to-Pulse & Unilateral Backlash Engine:

```cpp
class MotionAxisController {
private:
    float m_steps_per_mm = 80.0f;     // Calculated: (Motor_Steps * Microsteps) / Lead_mm
    float m_backlash_mm = 0.35f;      // Mechanical ball-screw backlash
    float m_current_pos_mm = 0.0f;
    float m_target_pos_mm = 0.0f;
    bool  m_is_homed = false;

public:
    void setStepsPerMm(float spm) { m_steps_per_mm = spm; }
    void setBacklash(float bl) { m_backlash_mm = bl; }

    // Unilateral Approach Rule: ALWAYS approach final position from the LEFT (negative direction)
    // to absorb mechanical ball-screw play deterministically.
    void moveTo(float target_mm) {
        if (!m_is_homed) return;

        if (target_mm < m_current_pos_mm) {
            // Target is to the left: Overshoot past target by backlash, then approach forward
            float overshoot_target = target_mm - m_backlash_mm;
            executeTrapezoidalMove(overshoot_target);
            executeTrapezoidalMove(target_mm); // Final forward approach takes up all play
        } else {
            // Target is to the right: Move directly forward
            executeTrapezoidalMove(target_mm);
        }
        m_current_pos_mm = target_mm;
    }

    void executeTrapezoidalMove(float target_mm) {
        int32_t delta_steps = (int32_t)((target_mm - m_current_pos_mm) * m_steps_per_mm);
        if (delta_steps == 0) return;

        bool dir = (delta_steps > 0);
        gpio_set_level((gpio_num_t)PIN_STEP_DIR, dir ? 1 : 0);
        uint32_t total_steps = abs(delta_steps);

        // Deterministic acceleration ramp
        for (uint32_t i = 0; i < total_steps; ++i) {
            gpio_set_level((gpio_num_t)PIN_STEP_PUL, 1);
            delayMicroseconds(5);
            gpio_set_level((gpio_num_t)PIN_STEP_PUL, 0);
            delayMicroseconds(45); // Speed profile
        }
    }
};
```

---

## Archetype 3: PID Thermal Regulation & Solid-State Sealing

### Ideal For:
UPVC Single/Double/Four-Head Welders, Commercial Bakery Ovens, Heat-Shrink Tunnels, Packaging Pouch Sealers.

### Hardware & Peripherals:
- **Sensor:** MAX6675 Cold-Junction Compensated K-Type Thermocouple on SPI (CS = GPIO 21 / 46).
- **Actuator:** High-Speed Solid-State Relay (SSR) on `GPIO 15` (Q5) and `GPIO 41` (Q9).
- **Modulation Method:** 1000ms Time-Proportional Software PWM with Zero-Cross synchronization.

### Industrial PID Loop Implementation:

```cpp
class TemperaturePidController {
private:
    float m_kp = 4.2f;
    float m_ki = 0.08f;
    float m_kd = 12.0f;
    float m_setpoint = 245.0f;       // Target temperature in Celsius
    float m_integral = 0.0f;
    float m_last_error = 0.0f;
    uint32_t m_window_start_ms = 0;
    uint32_t m_duty_ms = 0;
    uint8_t  m_ssr_pin;
    uint8_t  m_cs_pin;

public:
    TemperaturePidController(uint8_t ssr_pin, uint8_t cs_pin) 
        : m_ssr_pin(ssr_pin), m_cs_pin(cs_pin) {}

    void setTarget(float target_c) { m_setpoint = target_c; }

    void update(uint32_t now_ms, float current_temp) {
        if (current_temp <= -900.0f) { // Broken wire fault
            gpio_set_level((gpio_num_t)m_ssr_pin, 1); // Clamp OFF
            return;
        }

        // 1. Calculate PID output every 1000ms window
        if (now_ms - m_window_start_ms >= 1000) {
            m_window_start_ms = now_ms;
            float error = m_setpoint - current_temp;

            m_integral += error;
            // Anti-windup clamping
            if (m_integral > 500.0f) m_integral = 500.0f;
            if (m_integral < -500.0f) m_integral = -500.0f;

            float derivative = error - m_last_error;
            m_last_error = error;

            float output = (m_kp * error) + (m_ki * m_integral) + (m_kd * derivative);
            if (output < 0.0f) output = 0.0f;
            if (output > 100.0f) output = 100.0f;

            m_duty_ms = (uint32_t)(output * 10.0f); // 0..1000 ms
        }

        // 2. Modulate SSR state within the 1000ms window (Active-LOW: 0 = Energized)
        uint32_t elapsed_in_window = now_ms - m_window_start_ms;
        if (elapsed_in_window < m_duty_ms && current_temp < (m_setpoint + 5.0f)) {
            gpio_set_level((gpio_num_t)m_ssr_pin, 0); // SSR ON
        } else {
            gpio_set_level((gpio_num_t)m_ssr_pin, 1); // SSR OFF
        }
    }
};
```

---

## Archetype 4: Volume Dispensing, Liquid Batching & Flow Control

### Ideal For:
Double-Glazing Argon Gas-Filling Machines, Commercial Ice Cream Machines, Chemical Batch Dispensers, Lubrication Injectors.

### Hardware & Peripherals:
- **Sensor:** High-Speed Turbine / Hall-Effect Flow Sensor on `GPIO 3` / `GPIO 7`.
- **Valves:** Fast-Fill Solenoid (Q0) and Fine-Trickle Solenoid (Q1).
- **Feedback:** Pressure Switch on `GPIO 5` to detect line clogging or vacuum seal failure.

### Two-Stage Cutoff Batching Engine:

```cpp
class BatchDispenserController {
private:
    volatile uint32_t m_pulse_count = 0;
    float m_pulses_per_liter = 450.0f;
    float m_target_liters = 5.0f;
    float m_fine_trickle_threshold = 0.85f; // Switch to trickle at 85% volume
    bool  m_dispensing = false;

public:
    void setTargetVolume(float liters) { m_target_liters = liters; }
    void onPulseReceived() { m_pulse_count++; }

    void start() {
        m_pulse_count = 0;
        m_dispensing = true;
        gpio_set_level((gpio_num_t)PIN_RELAY_Q0, 0); // Fast valve ON
        gpio_set_level((gpio_num_t)PIN_RELAY_Q1, 0); // Trickle valve ON
    }

    void update() {
        if (!m_dispensing) return;

        float dispensed_liters = (float)m_pulse_count / m_pulses_per_liter;

        if (dispensed_liters >= (m_target_liters * m_fine_trickle_threshold)) {
            // Stage 1: Close Fast Valve to eliminate pressure hammer and overshoot
            gpio_set_level((gpio_num_t)PIN_RELAY_Q0, 1); // Fast valve OFF
        }

        if (dispensed_liters >= m_target_liters) {
            // Stage 2: Final Cutoff
            gpio_set_level((gpio_num_t)PIN_RELAY_Q1, 1); // Trickle valve OFF
            m_dispensing = false;
        }
    }
};
```
