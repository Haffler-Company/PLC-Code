# 18. Industrial Safety Interlocks & Machinery Safety Directives (ISO 13849 & IEC 62061)

## Executive Summary
Haffler industrial machines operate high-speed saw blades ($3000\text{ RPM}$), high-tonnage pneumatic clamps, and $260^\circ\text{C}$ heating platens. Operator physical safety is paramount. This document establishes hardware and software safety standards, emergency stop circuits, anti-tie-down two-hand controls, and machine protection interlocks.

---

## 1. The Fundamental Principle: Hardware Priority over Software

> **SIEMENS INDUSTRIAL RED LINE:** Microcontroller software must never be the sole protective measure guarding human life. Microcontrollers can experience lockups, brownouts, or severe electromagnetic interference.

### Dual-Channel Emergency Stop Architecture
1. **Direct Contactor De-energization (Hardware Channel):** The mushroom E-Stop button incorporates two independent, positively driven, Normally Closed (NC) contacts:
   - Contact 1 breaks the coil supply to the master line contactors, immediately dropping motor power and de-energizing pneumatic valves in $<10\text{ ms}$.
   - Contact 2 connects to optocoupled input $I_0$ as a digital monitoring signal to notify software of the stop condition.
2. **Pneumatic Safety Dump Valve:** Upon E-Stop assertion, an exhaust dump valve vents compressed air from motion cylinders, rendering actuators compliant and incapable of applying pinch force.

---

## 2. EN 574 Type III C Two-Hand Anti-Tie-Down Control

For aluminum punching presses, copy routers, and unguarded cut cycles, the operator must utilize both hands simultaneously to initiate motion, keeping hands clear of pinch points.

### Concurrency & Anti-Tie-Down Algorithms:
- **Concurrency Window ($\le 500\text{ ms}$):** The operator must depress button $B_1$ and button $B_2$ within a strict $500\text{ ms}$ delta:
  $$|t_{\text{press}}(B_1) - t_{\text{press}}(B_2)| \le 500\text{ ms}$$
- **Anti-Tie-Down Mandate:** If an operator attempts to wedge, tape, or weigh down one button permanently, the machine refuses to cycle. **Both buttons must be fully released** before a new cycle can be armed.

```cpp
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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

    bool evaluate(bool b1_pressed, bool b2_pressed, uint32_t now_ms) {
        if (b1_pressed && !m_b1_active) m_press_time_b1 = now_ms;
        if (b2_pressed && !m_b2_active) m_press_time_b2 = now_ms;

        m_b1_active = b1_pressed;
        m_b2_active = b2_pressed;

        // Both buttons must be released to re-arm
        if (!b1_pressed && !b2_pressed) {
            m_cycle_triggered = false;
            return false;
        }

        if (m_cycle_triggered) return false;

        // Verify both depressed within 500ms
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
```

---

## 3. Machine Guarding & Pneumatic Pressure Thresholds

### 1. Air Pressure Switch ($I_7$):
- Minimum required pneumatic line pressure is **$6.0\text{ Bar}$**.
- If pressure drops below $5.0\text{ Bar}$, the pressure switch contacts open on $I_7$.
- The PLC immediately suspends automatic cycles and displays `W04_AIR_PRESSURE_LOW` to prevent weak clamping and profile kickback.

### 2. Guard Hood Reed Switch & Solenoid Lock:
- The acrylic safety cover features a magnetic safety switch on $I_1$ and a solenoid lock on $Q_4$.
- **Interlock 1:** The saw blade cannot ascend if $I_1$ is open.
- **Interlock 2:** Solenoid $Q_4$ locks the guard in the closed position while the saw blade motor is spinning, unlocking only after rotor deceleration finishes.

---

## 4. Haffler Safety Interlock Matrix

| Input Channel | Monitored Actuator | Safe Requirement | Violation Response |
| :--- | :--- | :--- | :--- |
| **$I_0$ E-Stop** | All Relays ($Q_0..Q_9$) | Contact Closed ($0\text{V}$) | Instant de-energize to $1$ (OFF), dump air |
| **$I_1$ Saw Hood** | Saw Motor ($Q_0$), Cylinder ($Q_2$) | Hood Closed ($0\text{V}$) | Halt blade motor, emergency retract cylinder |
| **$I_2$ Clamp Sensor**| Saw Cylinder ($Q_2$) | Clamp Closed ($0\text{V}$) | Inhibit saw advance until profile clamped |
| **$I_7$ Pressure Sw** | All Pneumatics | Pressure $>6\text{ Bar}$ ($0\text{V}$) | Suspend cycle, display low pressure alarm |

---

## 5. Thermal Runaway Protection & Thermocouple Sensor Faults

For machines with high-temperature heating platens, sealing irons, and glue pots (such as UPVC Welders and Edge Banders):

### 5.1. MAX6675 Open Thermocouple Detection
The MAX6675 Cold-Junction-Compensated K-Thermocouple-to-Digital Converter chip incorporates an internal open-circuit detection circuit:
* Bit D2 of the 16-bit SPI response word is the **Thermocouple Input Open Bit**.
* If the thermocouple wire breaks, becomes loose, or is disconnected from terminals $T_1 / T_2$, bit D2 is asserted (`0x04`), and the reading returns $1024.0^\circ\text{C}$ or `NaN`.

```cpp
bool thermocouple_read_validated(int cs_pin, float &out_temp_c) {
    uint16_t raw_word = spi_read_word(cs_pin);
    
    // Check Bit 2: Open Thermocouple Fault
    if (raw_word & 0x0004) {
        // Sensor disconnected or broken wire!
        out_temp_c = -999.0f;
        return false;
    }
    
    // Bits D14..D3 contain 12-bit temperature (0.25°C resolution)
    out_temp_c = (float)((raw_word >> 3) & 0x0FFF) * 0.25f;
    return true;
}
```

### 5.2. Mandatory Thermal Fail-Safe Interlock
1. **Immediate SSR De-Energization:** If `thermocouple_read_validated` returns `false` or temperature exceeds $280^\circ\text{C}$ on UPVC welders, the firmware must immediately and unconditionally de-energize the Solid State Relay (`SSR_1` on `PIN_RELAY_Q5`) to level HIGH ($1 = \text{OFF}$).
2. **Heater Travel Safety Interlock:** The pneumatic heating element cylinder (`ELEMENT` on `PIN_RELAY_Q3`) must **NEVER** be commanded to advance into the machine zone unless both left and right profile clamps (`PRESS_LEFT` on Q1, `PRESS_RIGHT` on Q2) are in their verified, sensor-confirmed engaged state.
