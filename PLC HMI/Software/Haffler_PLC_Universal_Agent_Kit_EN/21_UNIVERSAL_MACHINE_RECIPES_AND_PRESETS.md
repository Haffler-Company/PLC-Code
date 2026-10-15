# 21. Universal Machine Recipes, Signal Maps & FSM Blueprint Encyclopedia
## Production-Tested Engineering Profiles for the 8 Core Industrial Machine Classes

> **Document ID:** `21_UNIVERSAL_MACHINE_RECIPES_AND_PRESETS.md`  
> **Target Platform:** Haffler Universal Integrated PLC + HMI Controller (ESP32-S3 N16R8).  
> **Standard:** IEC 61131-3, PackML State Architecture, ISO 13849-1 Machine Safety, Apple HIG 480x320.  
> **Audience:** Autonomous AI Agents and Senior Industrial Systems Engineers.

---

## Executive Summary
This document provides complete, production-ready engineering blueprints for the 8 core industrial machine archetypes manufactured by Haffler and its partners. When assigned a new project name and brief scenario hints, the AI agent matches the machine against these pre-tested profiles to derive pins, FSM transitions, safety watchdogs, Apple HIG metrics, and release metadata without asking clarifying questions.

---

## 1. Machine Archetype 1: UPVC / Aluminum Profile Cutting Machine

### 1.1. Overview & Process Physics
Linear positioning of extrusion profiles via precision stepper/servo and magnetic linear encoder (PCNT), dual pneumatic vertical/horizontal clamping, high-speed tungsten saw blade spinning at 2800 RPM, pneumatic blade stroke advance with cooling mist atomization, and automatic profile feed indexing.

### 1.2. Complete Hardware I/O Allocation
```
+-------------+---------------+-----------------------+------------------------------------------+
| Terminal    | Pin / Channel | Electrical Standard   | Functional Machine Assignment            |
+-------------+---------------+-----------------------+------------------------------------------+
| I0          | GPIO 5        | Opto-Isolated 24V NC  | Emergency Stop Pushbutton (Mushroom)     |
| I1          | GPIO 4        | Opto-Isolated 24V NO  | Safety Hood Cover Closed Interlock       |
| I2          | GPIO 3        | Opto-Isolated 24V NO  | Horizontal Clamps Locked Pressure Sw     |
| I3          | GPIO 7        | Opto-Isolated 24V NO  | Vertical Clamps Locked Pressure Sw       |
| I4          | GPIO 6        | Opto-Isolated 24V NO  | Saw Blade Return Limit Switch (Home)     |
| I5          | GPIO 21       | Opto-Isolated 24V NO  | Saw Blade Forward End-of-Stroke Switch   |
| I6          | GPIO 37       | Opto-Isolated 24V NO  | Material Infeed Optical Part Detector    |
| I7          | GPIO 47       | Opto-Isolated 24V NO  | Air Supply Pressure Sensor (>= 5.5 Bar)  |
| Q0          | GPIO 48       | Active-LOW Relay (0=ON)| Saw Spindle Motor Contactor / VFD Run   |
| Q1          | GPIO 40       | Active-LOW Relay (0=ON)| Horizontal Pneumatic Clamps Solenoid     |
| Q2          | GPIO 35       | Active-LOW Relay (0=ON)| Vertical Pneumatic Clamps Solenoid       |
| Q3          | GPIO 36       | Active-LOW Relay (0=ON)| Saw Blade Hydro-Pneumatic Advance Solenoid|
| Q4          | GPIO 38       | Active-LOW Relay (0=ON)| Saw Blade Retract Solenoid               |
| Q5          | GPIO 15       | High-Speed SSR PWM    | Blade Mist Lubricant / Coolant Pulse     |
| Q6          | GPIO 17       | Active-LOW Relay (0=ON)| Profile Feeder Pusher Advance Valve      |
| Q7          | GPIO 18       | Active-LOW Relay (0=ON)| Chip & Dust Extraction Blower Contactor  |
| Q8          | GPIO 19       | Active-LOW Relay (0=ON)| Safety Hood Pneumatic Lock Solenoid      |
| Q9          | GPIO 20       | Active-LOW Relay (0=ON)| Stack Light Green (Production In Progress)|
| M0..M7      | Virtual Aux   | Modbus / MES Aux      | M0: Vacuum, M1: Auto Unloader, M2: Count |
+-------------+---------------+-----------------------+------------------------------------------+
```

### 1.3. PackML FSM State Machine Sequence
1. `STATE_IDLE`: Spindle OFF, Clamps Open, Hood Unlocked. Ready for dimension input.
2. `STATE_STARTING`: Verify Air Pressure ($I_7$), verify Hood Closed ($I_1$), lock Hood Solenoid ($Q_8$). Start Spindle ($Q_0$), dwell 1200ms to achieve rated RPM.
3. `STATE_EXECUTE`: Energize Clamps ($Q_1, Q_2$), wait for Clamped signals ($I_2, I_3$, timeout 1500ms). Energize Blade Advance ($Q_3$) and Coolant ($Q_5$).
4. `STATE_COMPLETING`: Detect Blade Forward switch ($I_5$). De-energize Advance, energize Retract ($Q_4$). Wait for Saw Home switch ($I_6$, timeout 2000ms). De-energize Clamps. Unlock Hood. Increment piece counter.
5. `STATE_STOPPING` / `STATE_ABORTED`: De-energize Advance immediately, retract blade, cut Spindle ($Q_0$) via electronic braking resistor.

### 1.4. Safety Watchdog Limits
- **Saw Stroke Timeout:** 3500ms max travel time. If $I_5$ is not reached within 3.5s, trigger `E04` and retract blade.
- **Air Drop Trip:** If $I_7$ drops below 5.0 Bar during stroke, stop forward stroke immediately.

### 1.5. Apple HIG Screen Specification
- **Hero Metric:** Target Cut Length (`mm`), e.g., `1250.0 mm` (Font `digit80`, mathematically centered).
- **Steppers (16px margin):** Target Length (`mm`), Quantity (`pcs`), Saw Feed Speed (`mm/s`), Blade Dwell (`ms`).

---

## 2. Machine Archetype 2: UPVC Window Profile Welding Machine

### 2.1. Overview & Process Physics
Dual-axis pneumatic melting and joint fusion of UPVC profile corners. Dual temperature regulation (dual MAX6675 SPI thermocouples, PID SSR heating to 240°C - 260°C). Sequence: Clamp profiles -> Heater plate in -> Melt dwell (20s - 30s) -> Heater plate out -> Fusion squeeze dwell (25s - 35s) -> Cool down -> Unclamp.

### 2.2. Complete Hardware I/O Allocation
```
+-------------+---------------+-----------------------+------------------------------------------+
| Terminal    | Pin / Channel | Electrical Standard   | Functional Machine Assignment            |
+-------------+---------------+-----------------------+------------------------------------------+
| I0          | GPIO 5        | Opto-Isolated 24V NC  | Emergency Stop Pushbutton                |
| I1          | GPIO 4        | Opto-Isolated 24V NO  | Safety Laser Curtain / Two-Hand Button 1 |
| I2          | GPIO 3        | Opto-Isolated 24V NO  | Two-Hand Anti-Tie-Down Button 2          |
| I3          | GPIO 7        | Opto-Isolated 24V NO  | Profile In-Position Contact Sensor Left  |
| I4          | GPIO 6        | Opto-Isolated 24V NO  | Profile In-Position Contact Sensor Right |
| I5          | GPIO 21       | Opto-Isolated 24V NO  | Heater Plate Inserted Reed Switch        |
| I6          | GPIO 37       | Opto-Isolated 24V NO  | Heater Plate Retracted Reed Switch       |
| I7          | GPIO 47       | Opto-Isolated 24V NO  | Air Pressure Normal Threshold Sensor     |
| Q0          | GPIO 48       | Active-LOW Relay (0=ON)| Left Profile Clamps Down Solenoid        |
| Q1          | GPIO 40       | Active-LOW Relay (0=ON)| Right Profile Clamps Down Solenoid       |
| Q2          | GPIO 35       | Active-LOW Relay (0=ON)| Heater Plate Cylinder Advance Solenoid   |
| Q3          | GPIO 36       | Active-LOW Relay (0=ON)| Heater Plate Cylinder Retract Solenoid   |
| Q4          | GPIO 38       | Active-LOW Relay (0=ON)| Carriage Fusion Squeeze Solenoid         |
| Q5          | GPIO 15       | High-Speed SSR PWM    | Main Heating Plate Element (PID SSR)     |
| Q6          | GPIO 17       | Active-LOW Relay (0=ON)| Weld Seam Cooling Blower Fan             |
| Q7          | GPIO 18       | Active-LOW Relay (0=ON)| Secondary Heating Zone / Knife Heater    |
| Q8          | GPIO 19       | Active-LOW Relay (0=ON)| Auxiliary Profile Support Jack           |
| Q9          | GPIO 20       | Active-LOW Relay (0=ON)| Cycle Complete Acoustic Alert Buzzer/Lamp|
| M0..M7      | Virtual Aux   | Modbus / MES Aux      | M0: Welded Bar Ejector, M1: Auto Transfer|
+-------------+---------------+-----------------------+------------------------------------------+
```

### 2.3. Safety Watchdog Limits
- **Thermal Runaway Guard:** Hard heater cutoff if thermocouple reads $> 275^\circ\text{C}$ or if temperature does not rise by $2^\circ\text{C}$ in 60s when heating.
- **Thermocouple Open-Circuit Trip:** MAX6675 bit D2 = 1 triggers immediate heater cutoff and alarm `E08: THERMOCOUPLE OPEN`.

### 2.4. Apple HIG Screen Specification
- **Hero Metric:** Welding Plate Temperature (`°C`), e.g., `248 °C` (Emerald Neon Green when within $\pm 3^\circ\text{C}$ tolerance band; Industrial Amber when pre-heating).
- **Steppers (16px margin):** Setpoint Temp (`°C`), Melting Time (`s`), Squeezing Time (`s`), Profile Allowance (`mm`).

---

## 3. Machine Archetype 3: Industrial Argon Gas Filling Machine (IGU)

### 3.1. Overview & Process Physics
Argon gas injection into insulated glass units (IGU) with simultaneous oxygen evacuation and zirconia/acoustic gas concentration monitoring. Dual station operation with independent solenoid valves, differential pressure regulation, vacuum suction, and 180-second hard safety watchdog.

### 3.2. Complete Hardware I/O Allocation
```
+-------------+---------------+-----------------------+------------------------------------------+
| Terminal    | Pin / Channel | Electrical Standard   | Functional Machine Assignment            |
+-------------+---------------+-----------------------+------------------------------------------+
| I0          | GPIO 5        | Opto-Isolated 24V NC  | Emergency Stop Pushbutton                |
| I1          | GPIO 4        | Opto-Isolated 24V NO  | Glass Unit Edge Seal Sensor Station 1    |
| I2          | GPIO 3        | Opto-Isolated 24V NO  | Glass Unit Edge Seal Sensor Station 2    |
| I3          | GPIO 7        | Opto-Isolated 24V NO  | Start / Foot Pedal Station 1             |
| I4          | GPIO 6        | Opto-Isolated 24V NO  | Start / Foot Pedal Station 2             |
| I5          | GPIO 21       | Opto-Isolated 24V NO  | Gas Bottle Supply Pressure OK (>2.5 Bar) |
| I6          | GPIO 37       | Opto-Isolated 24V NO  | Vacuum Return Flow Sensor                |
| I7          | GPIO 47       | Opto-Isolated 24V NO  | Overpressure Relief Detection Switch     |
| Q0          | GPIO 48       | Active-LOW Relay (0=ON)| Station 1 Argon Injection Solenoid Valve |
| Q1          | GPIO 40       | Active-LOW Relay (0=ON)| Station 1 Air/Gas Suction Vacuum Valve   |
| Q2          | GPIO 35       | Active-LOW Relay (0=ON)| Station 2 Argon Injection Solenoid Valve |
| Q3          | GPIO 36       | Active-LOW Relay (0=ON)| Station 2 Air/Gas Suction Vacuum Valve   |
| Q4          | GPIO 38       | Active-LOW Relay (0=ON)| Vacuum Pump Motor Contactor              |
| Q5          | GPIO 15       | High-Speed SSR PWM    | Gas Pre-Heater Element                   |
| Q6          | GPIO 17       | Active-LOW Relay (0=ON)| Atmospheric Exhaust Vent Solenoid        |
| Q7          | GPIO 18       | Active-LOW Relay (0=ON)| Gas Calibration Purge Valve              |
| Q8          | GPIO 19       | Active-LOW Relay (0=ON)| Station 1 Seal Clamp Solenoid            |
| Q9          | GPIO 20       | Active-LOW Relay (0=ON)| Station 2 Seal Clamp Solenoid            |
| M0..M7      | Virtual Aux   | Modbus / MES Aux      | M0: Gas Cylinder Bank Auto-Switchover    |
+-------------+---------------+-----------------------+------------------------------------------+
```

### 3.3. Safety Watchdog Limits
- **180s Cylinder Exhaustion Cutoff:** Hard software timer limits any continuous gas injection to 180.0 seconds maximum to prevent asphyxiation hazards and argon bottle draining.
- **Overpressure Trip:** Automatic cutoff if internal cavity pressure exceeds $+15\text{ mbar}$ relative to atmosphere.

### 3.4. Apple HIG Screen Specification
- **Hero Metric:** Argon Purity Percentage (`%`), e.g., `92.4 %` (Color-coded: Emerald Green $\ge 90\%$, Amber $85\%..89\%$, Red $<85\%$).
- **Steppers (16px margin):** Target Concentration (`%`), Cavity Thickness (`mm`), Vacuum Flow Rate (`L/min`).

---

## 4. Machine Archetype 4: Automatic Edge Bander & Trimming Machine

### 4.1. Overview & Process Physics
Woodworking edge-banding of MDF/chipboard panels. Hot-melt glue pot heating (180°C - 210°C), infeed panel detection, motorized feed track, PVC edge coil feeding with pneumatic guillotine cutter, multiple pressure rollers, high-speed end-trimming saw (12000 RPM), top/bottom flush trimming cutter, and buffing wheel.

### 4.2. Complete Hardware I/O Allocation
```
+-------------+---------------+-----------------------+------------------------------------------+
| Terminal    | Pin / Channel | Electrical Standard   | Functional Machine Assignment            |
+-------------+---------------+-----------------------+------------------------------------------+
| I0          | GPIO 5        | Opto-Isolated 24V NC  | Emergency Stop Pushbutton                |
| I1          | GPIO 4        | Opto-Isolated 24V NO  | Panel Infeed Optical Photoelectric Eye   |
| I2          | GPIO 3        | Opto-Isolated 24V NO  | Panel Outfeed Optical Photoelectric Eye  |
| I3          | GPIO 7        | Opto-Isolated 24V NO  | Tape Coil Depleted Microswitch           |
| I4          | GPIO 6        | Opto-Isolated 24V NO  | Guillotine Blade Retracted Reed Switch   |
| I5          | GPIO 21       | Opto-Isolated 24V NO  | End-Trim Saw Carriage Home Position      |
| I6          | GPIO 37       | Opto-Isolated 24V NO  | Air Pressure Normal Sensor (>= 6 Bar)    |
| I7          | GPIO 47       | Opto-Isolated 24V NO  | Top Pressure Beam Safety Switch          |
| Q0          | GPIO 48       | Active-LOW Relay (0=ON)| Feed Track Motor Contactor / VFD Run     |
| Q1          | GPIO 40       | Active-LOW Relay (0=ON)| Glue Roller Motor & Infeed Gate          |
| Q2          | GPIO 35       | Active-LOW Relay (0=ON)| Pneumatic Tape Feeder Pinch Roller       |
| Q3          | GPIO 36       | Active-LOW Relay (0=ON)| Pneumatic Tape Guillotine Knife Solenoid |
| Q4          | GPIO 38       | Active-LOW Relay (0=ON)| Pressure Rollers Engage Solenoid         |
| Q5          | GPIO 15       | High-Speed SSR PWM    | Glue Pot Main Heater (PID SSR)           |
| Q6          | GPIO 17       | Active-LOW Relay (0=ON)| Glue Spindle Auxiliary Nozzle Heater     |
| Q7          | GPIO 18       | Active-LOW Relay (0=ON)| End-Trim High-Speed Saw Motors Contactor |
| Q8          | GPIO 19       | Active-LOW Relay (0=ON)| Flush Trimming Milling Motors Contactor  |
| Q9          | GPIO 20       | Active-LOW Relay (0=ON)| Buffing Wheels Cleaning Motors           |
| M0..M7      | Virtual Aux   | Modbus / MES Aux      | M0: Antistatic Mist, M1: Chip Dust Vac   |
+-------------+---------------+-----------------------+------------------------------------------+
```

### 4.3. Apple HIG Screen Specification
- **Hero Metric:** Glue Pot Temperature (`°C`), e.g., `195 °C`.
- **Steppers (16px margin):** Glue Temp (`°C`), Standby Temp (`°C`), Track Speed (`m/min`), Tape Overhang (`mm`).

---

## 5. Machine Archetype 5: UPVC Corner Cleaning & CNC Trimming Machine

### 5.1. Overview & Process Physics
Automatic trimming of weld beads on welded UPVC sash and frame corners. Multi-tool pneumatic turret or linear axis: surface grooving knives (top and bottom), outside radius contour cutter, inside seal scraper, and glazing bead drill.

### 5.2. Complete Hardware I/O Allocation
```
+-------------+---------------+-----------------------+------------------------------------------+
| Terminal    | Pin / Channel | Electrical Standard   | Functional Machine Assignment            |
+-------------+---------------+-----------------------+------------------------------------------+
| I0          | GPIO 5        | Opto-Isolated 24V NC  | Emergency Stop Pushbutton                |
| I1          | GPIO 4        | Opto-Isolated 24V NO  | Frame Corner In-Position Sensor (Centering)|
| I2          | GPIO 3        | Opto-Isolated 24V NO  | Vertical Clamp Cylinder Down Reed Sw     |
| I3          | GPIO 7        | Opto-Isolated 24V NO  | Horizontal Clamp Cylinder In Reed Sw     |
| I4          | GPIO 6        | Opto-Isolated 24V NO  | Top Knife Cylinder Home Switch           |
| I5          | GPIO 21       | Opto-Isolated 24V NO  | Bottom Knife Cylinder Home Switch        |
| I6          | GPIO 37       | Opto-Isolated 24V NO  | Milling Spindle Forward Limit Switch     |
| I7          | GPIO 47       | Opto-Isolated 24V NO  | Start Pushbutton / Foot Pedal            |
| Q0          | GPIO 48       | Active-LOW Relay (0=ON)| Vertical Clamp Cylinder Solenoid         |
| Q1          | GPIO 40       | Active-LOW Relay (0=ON)| Horizontal Centering Clamps Solenoid     |
| Q2          | GPIO 35       | Active-LOW Relay (0=ON)| Top Weld-Bead Grooving Knife Solenoid    |
| Q3          | GPIO 36       | Active-LOW Relay (0=ON)| Bottom Weld-Bead Grooving Knife Solenoid |
| Q4          | GPIO 38       | Active-LOW Relay (0=ON)| Outside Contour Milling Spindle Run      |
| Q5          | GPIO 15       | High-Speed SSR PWM    | Milling Carriage Feed Speed Valve        |
| Q6          | GPIO 17       | Active-LOW Relay (0=ON)| Inside Rubber Gasket Scraper Solenoid    |
| Q7          | GPIO 18       | Active-LOW Relay (0=ON)| Drainage Slot Drilling Spindle Run       |
| Q8          | GPIO 19       | Active-LOW Relay (0=ON)| Waste Shaving Air Blow-Off Jet           |
| Q9          | GPIO 20       | Active-LOW Relay (0=ON)| Cycle Finished Indicator Light / Buzzer  |
| M0..M7      | Virtual Aux   | Modbus / MES Aux      | M0: Robotic Outfeed Transfer Table       |
+-------------+---------------+-----------------------+------------------------------------------+
```

### 5.3. Apple HIG Screen Specification
- **Hero Metric:** Cycle Duration (`s`), e.g., `14.2 s` (Centered, pure white digits).
- **Steppers (16px margin):** Grooving Depth (`mm`), Milling Feed (`mm/s`), Clamp Dwell (`ms`), Tool Select.

---

## 6. Machine Archetype 6: High-Speed Copy Router (Slot Milling)

### 6.1. Overview & Process Physics
Triple-spindle lock-casing and water drainage milling machine for window and door profiles. High-speed router motor (18,000 RPM) with copy template stylus, pneumatic profile clamping, and pneumatic tool feed.

### 6.2. Hardware I/O Allocation
- $I_0$: E-Stop, $I_1$: Clamps Down Sensor, $I_2$: Stylus In-Position Switch, $I_3$: Spindle Thermal Overload.
- $Q_0$: High-Frequency Router Spindle Contactor, $Q_1$: Triple-Hole Horizontal Spindle Contactor, $Q_2$: Vertical Pneumatic Clamps, $Q_3$: Cooling Mist Spray, $Q_4$: Hydro-Check Feed Solenoid.
- **Hero Metric:** Spindle Speed (`RPM`), e.g., `18,000 RPM`.

---

## 7. Machine Archetype 7: Commercial Bakery Deck & Tunnel Oven

### 7.1. Overview & Process Physics
Precise multi-zone thermal baking profile. Top heating zone, bottom heating zone, pulsed high-pressure steam injection generator, motorized exhaust steam damper, baking countdown timer, and interior halogen inspection lighting.

### 7.2. Hardware I/O Allocation
- $I_0$: E-Stop, $I_1$: Oven Door Closed Switch, $I_2$: Steam Boiler Water Level Normal, $I_3$: Gas Burner Flame Sensor / Air Pressure Switch.
- $Q_0$: Top Heating Element Contactor / Gas Valve 1, $Q_1$: Bottom Heating Element Contactor / Gas Valve 2, $Q_2$: Steam Injection Solenoid Valve, $Q_3$: Motorized Exhaust Damper Open Solenoid, $Q_4$: Internal Inspection Lamp, $Q_5$: SSR PID Top Trim, $Q_9$: End-of-Bake Resonant Chime.
- **Hero Metric:** Chamber Temperature (`°C`), e.g., `230 °C`. Secondary: Steam Duration (`s`).

---

## 8. Machine Archetype 8: Commercial Soft Ice Cream Machine

### 8.1. Overview & Process Physics
Dual-cylinder soft ice cream freezing dispenser. Semi-hermetic compressor contactor, dual beater auger motors with current-sense viscosity detection, hopper refrigeration with magnetic mix level probes, defrost hot-gas bypass valves, and 5-stage day-lock leasing lock.

### 8.2. Hardware I/O Allocation
- $I_0$: E-Stop, $I_1$: Left Dispense Handle Switch, $I_2$: Right Dispense Handle Switch, $I_3$: Left Hopper Low Mix Probe, $I_4$: Right Hopper Low Mix Probe, $I_5$: Condenser High-Pressure Switch.
- $Q_0$: Main Freezing Compressor Contactor, $Q_1$: Left Beater Auger Motor, $Q_2$: Right Beater Auger Motor, $Q_3$: Auxiliary Hopper Chiller Compressor, $Q_4$: Hot-Gas Defrost Solenoid Left, $Q_5$: Hot-Gas Defrost Solenoid Right, $Q_6$: Hopper Mix Agitator Motor.
- **Hero Metric:** Freezing Cylinder Viscosity / Hardness (`%`), e.g., `85 %`.

---

## 9. Quick Lookup Matrix for Incoming AI Agents

| User Prompt Keyword | Match Archetype | Primary Actuator ($Q_0$) | Critical Interlock ($I_0..I_7$) | Hero Metric Unit |
| :--- | :--- | :--- | :--- | :--- |
| "اره", "برش", "cutter", "saw" | Archetype 1: Cutting | Saw Spindle Motor | Hood Closed + Clamps Down | `mm` |
| "جوش", "welder", "fusion" | Archetype 2: Welding | Clamps + Heater Plate | Safety Curtain + Temp OK | `°C` |
| "گاز", "آرگون", "gas", "filling" | Archetype 3: Dispensing | Argon Valve 1 | E-Stop + 180s Watchdog | `%` |
| "لبه چسبان", "edge bander" | Archetype 4: Edge Bander| Track Motor + Glue Pot | Glue Temp >= Setpoint | `°C` |
| "تمیزکن", "گوشه", "corner clean" | Archetype 5: Corner Clean| Centering Clamps + Knife| Clamps Locked | `s` |
| "کپی روتر", "شیارزن", "router" | Archetype 6: Copy Router| 18,000 RPM Spindle | Profile Clamped | `RPM` |
| "فر", "قنادی", "bakery", "oven" | Archetype 7: Bakery Oven| Chamber Heaters | Door Closed + Flame OK | `°C` |
| "بستنی", "آبمیوه", "ice cream" | Archetype 8: Ice Cream | Freezing Compressor | Condenser Pressure Normal | `%` |

---

## 10. Universal Ready-to-Wire C++ State Machine Implementation Cookbook

When implementing Core 1 PLC logic for any machine, select from or combine the 4 standard production-tested C++ finite state machines below:

### 10.1. Archetype 1: Multi-Cylinder Pneumatic Automation FSM
**Ideal for:** UPVC Corner Cleaners, Aluminum Punching Presses, Multi-Head Clamping Jigs, UPVC Welders.

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
        : m_clamp("Clamp", PIN_RELAY_Q0, 255, PIN_INP_REF, -1, 2000),
          m_tool("Tool", PIN_RELAY_Q1, PIN_RELAY_Q2, 4, 5, 3000) {}

    void init() {
        m_state = PNEU_STATE_IDLE;
    }

    void update(uint32_t dt_ms, bool estop_active, bool btn_start1, bool btn_start2, const bool inputs[]) {
        if (estop_active) {
            m_clamp.retract();
            m_tool.retract();
            m_state = PNEU_STATE_FAULT;
            return;
        }

        m_clamp.update(inputs);
        m_tool.update(inputs);

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
                m_clamp.advance();
                m_state = PNEU_STATE_CLAMPING;
            } else if (m_state_timer > 500) {
                m_state = PNEU_STATE_IDLE; // Anti-tie-down abort
            }
            break;

        case PNEU_STATE_CLAMPING:
            if (m_clamp.isAdvanced()) {
                m_tool.advance();
                m_state = PNEU_STATE_TOOL_ADVANCE;
            } else if (m_clamp.isFault()) {
                m_state = PNEU_STATE_FAULT;
            }
            break;

        case PNEU_STATE_TOOL_ADVANCE:
            if (m_tool.isAdvanced()) {
                m_state_timer = 0;
                m_state = PNEU_STATE_TOOL_DWELL;
            } else if (m_tool.isFault()) {
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

### 10.2. Archetype 2: Precision Motion & Encoder Positioning Engine
**Ideal for:** UPVC Single/Double Head Cutting Saws, Digital Length Gauges, Automatic Sizing Stops, Copy Routers.

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

### 10.3. Archetype 3: PID Thermal Regulation & Solid-State Sealing Loop
**Ideal for:** UPVC Welders, Commercial Bakery Ovens, Heat-Shrink Tunnels, Packaging Pouch Sealers.

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
            gpio_set_level((gpio_num_t)m_ssr_pin, 1); // Clamp OFF (Active-LOW)
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

### 10.4. Archetype 4: Volume Dispensing & Two-Stage Batching Engine
**Ideal for:** Double-Glazing Argon Gas-Filling Machines, Commercial Ice Cream Machines, Chemical Batch Dispensers.

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