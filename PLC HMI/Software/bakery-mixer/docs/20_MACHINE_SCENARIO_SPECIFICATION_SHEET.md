# 20. Machine Scenario & Engineering Specification Sheet

## Executive Summary
The primary cause of wasted engineering cycles, repeated back-and-forth questioning, and hallucinated controller behavior is an **underspecified initial machine requirement**. 

This document provides a standardized, battle-tested **Machine Specification Template**. Either the client completes this sheet before task delegation, or the autonomous agent drafts this sheet based on the client's terse prompt and requests a single binary ("Approve / Edit") confirmation before writing a single line of firmware code.

---

## The Master Specification Sheet Template

```markdown
# Machine Technical Specification: [Machine Name - e.g., UPVC Double-Head Miter Saw]
**Project Code:** [e.g., HAFFLER-CUT-02]  
**Creation Date:** [YYYY/MM/DD]  
**Initial Firmware Revision:** V.1.0.0  

---

### 1. Digital Inputs Allocation Table (Optocoupled I0..I7)

| Input | Signal Name | Sensor Type (NC / NO) | Voltage | Workshop Function & Wiring |
| :---: | :--- | :---: | :---: | :--- |
| **I0** | `ESTOP_BUTTON` | NC (Normally Closed) | 24V | Mushroom emergency stop button circuit |
| **I1** | `SAFETY_GUARD` | NC (Normally Closed) | 24V | Acrylic saw blade hood magnetic interlock switch |
| **I2** | `CLAMP_SENS_DOWN` | NO (Normally Open) | 24V | Magnetic reed switch on pneumatic profile clamp |
| **I3** | `FEEDER_SENS_FWD` | NO (Normally Open) | 24V | Inductive proximity sensor on carriage datum |
| **I4** | `START_PEDAL` | NO (Normally Open) | 24V | Operator foot pedal NO switch for cycle start |
| **I5** | `STOP_BUTTON` | NC (Normally Closed) | 24V | Physical red stop button on front console |
| **I6** | `PROFILE_DETECT` | NO (Normally Open) | 24V | Photoelectric sensor confirming workpiece presence |
| **I7** | `AIR_PRESSURE_OK` | NO (Normally Open) | 24V | Pneumatic line pressure switch threshold (>= 6.0 Bar) |

---

### 2. Relay Outputs & Actuators Table (Q0..Q9 & M0..M7)

| Output | Signal Name | Actuator Type | Electrical Load | Workshop Function |
| :---: | :--- | :--- | :--- | :--- |
| **Q0** | `SAW_MOTOR_RUN` | Contactor / Inverter | 3-Phase 2.2kW | Start main saw blade spindle rotation |
| **Q1** | `CLAMP_SOLENOID` | 5/2 Pneumatic Valve | 24VDC 0.5A | Drive pneumatic profile clamp downward |
| **Q2** | `SAW_FEED_CYL` | 5/2 Pneumatic Valve | 24VDC 0.5A | Hydro-pneumatic saw blade feed ascend |
| **Q3** | `MIST_LUBRICANT` | Solenoid Valve | 24VDC 0.2A | Spray cutting coolant mist onto blade teeth |
| **Q4** | `GUARD_HOOD_LOCK`| Solenoid Pin Lock | 24VDC 0.8A | Mechanical lock on safety cover until motor stops |
| **Q5** | `BLOW_OFF_AIR` | Solenoid Valve | 24VDC 0.3A | Air nozzle clearing chips off cutting fence |
| **Q6** | `RESERVED_Q6` | - | - | Auxiliary expansion channel |
| **Q7** | `RESERVED_Q7` | - | - | Auxiliary expansion channel |
| **Q8** | `STACK_LIGHT_GRN`| Signal Tower Lamp | 24VDC LED | Visual green light: Machine Running / Ready |
| **Q9** | `STACK_LIGHT_RED`| Signal Tower Lamp | 24VDC LED | Visual red light: Fault / Emergency Stop Active |
| **M0** | `VACUUM_COLLECT` | Vacuum Contactor | 220VAC | Start external sawdust dust extractor unit |
| **M1..M7**| - | Virtual Aux Relays | - | Software-mapped auxiliary channels |

---

### 3. Automatic Cycle Sequencer Table (Step-by-Step)

| Step | State Identifier | Active Outputs | Transition Condition to Next Step | Timeout Limit | Fault Action |
| :---: | :--- | :--- | :--- | :---: | :--- |
| **1** | `IDLE_WAIT` | All Relays OFF | Foot pedal pressed (I4) && Air pressure OK (I7) | - | Standby for operator command |
| **2** | `CLAMP_DOWN` | Solenoid Q1 | Clamp sensor closed (I2 == 1) | 2000 ms | Abort, unclamp, trigger Alarm E02 |
| **3** | `MOTOR_START`| Spindle Q0 | Wait 1.5s until motor reaches full 3000 RPM | 2500 ms | Cut motor power, trigger Alarm E03 |
| **4** | `SAW_ADVANCE`| Cylinder Q2 + Mist Q3| Saw upper limit reed switch triggered | 4000 ms | Emergency retract, trigger Alarm E04 |
| **5** | `SAW_RETRACT`| Cylinder Q2 de-energized| Saw home reed switch triggered | 3000 ms | Cut motor power, trigger Alarm E05 |
| **6** | `MOTOR_STOP` | Motor Q0 OFF | Wait 2.0s for blade deceleration | - | Advance to next step |
| **7** | `UNCLAMP` | Clamp Q1 OFF | Profile released; workpiece removed | 1500 ms | Increment piece counter; loop to Step 1 |

---

### 4. Settings & Adjustable Parameters Table

| Parameter Title | Unit | Default | Min | Max | Step | Access Tier | NVS Key |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| Target Cut Dimension | mm | 1450.0 | 250.0 | 4500.0 | 0.5 | Operator (No PIN) | `target_len_mm` |
| Clamp Dwell Time | ms | 800 | 200 | 3000 | 50 | Technician (1234) | `clamp_dwell_ms` |
| Saw Feed Speed | % | 75 | 10 | 100 | 5 | Technician (1234) | `saw_speed_pct` |
| Coolant Mist Duration | ms | 1200 | 0 | 5000 | 100 | Technician (1234) | `coolant_ms` |
| Backlash Compensation | 0.1 mm | 3 | 0 | 20 | 1 | Supervisor (5678) | `backlash_comp` |
| Leasing Lock Stage 1 | Days | 30 | 0 | 180 | 1 | Factory Master (9999)| `lock_days_stg1` |

---

### 5. ISA-18.2 Alarms & Machine Faults Matrix

| Fault Code | English Label | Cause / Trigger Condition | Protective Interlock | Recovery Action |
| :---: | :--- | :--- | :--- | :--- |
| **E01** | Emergency Stop Active | Contact open on input I0 | De-energize all relays; vent main air line | Release E-stop mushroom + press STOP |
| **E02** | Clamp Motion Timeout | Sensor I2 failed to close in 2000ms | Inhibit blade ascend; release clamp | Clear jammed profile + press STOP |
| **E03** | Safety Guard Open | Acrylic cover I1 opened during cycle | Instantly cut spindle motor power | Close acrylic cover + press SET |
| **E04** | Air Line Low Pressure | Air switch I7 opened (<6.0 Bar) | Suspend cycle until line pressure recovers | Automatic once pressure >= 6.0 Bar |
| **W01** | Leasing Payment Due | DS3231 countdown reached 0 | Inhibit automatic cycle; allow manual jog | Enter factory unlock PIN |
```

---

## Agent Protocol When Receiving a New Machine Scenario
1. When the client enters a terse prompt (e.g. *"I need a controller for a PVC copy router"*):
   - The agent drafts the filled specification table above inside the project as `docs/MACHINE_SPEC.md`.
   - The agent presents the table and asks: *"I drafted the machine specification above. Does this I/O map and 7-step sequence match your mechanical design?"*
2. Once confirmed, firmware generation proceeds with 100% precision and zero rework.
