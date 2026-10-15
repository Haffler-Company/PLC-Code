# Bakery Dough Divider & Rounder Technical Encyclopedia — Machine Knowledge Base

This document serves as the master engineering encyclopedia covering the mechanical architecture, electronic drive systems, portioning cycle physics, sensor instrumentation, and fault diagnosis of the Industrial Bakery Dough Divider (Chone Machine).

---

## 1. Machine Philosophy & Process Engineering

The **Bakery Dough Divider (Chone Machine)** is a precision food processing machine designed for commercial and industrial bakeries (Lavash, Taftoon, Barbari, Baguette, Pita). Its primary function is to divide bulk dough from a hopper into discrete portions (dough balls or "chones") with repeatable weight, consistent density, and uniform geometric volume, while preserving the delicate gluten network and trapped gas bubbles.

### Key Industrial Requirements:
1. **Weight Repeatability:** Portion weight must remain consistent across batches despite variations in dough hydration, temperature, and bulk fermentation level.
2. **Gluten Network Preservation:** The cutting and portioning action must minimize shear stress to avoid degassing or tearing the dough.
3. **Pace Synchronization:** The dividing rate (strokes per minute) must align precisely with downstream provers, rounders, and baking ovens.

---

## 2. Mechanical Actuation & Kinematics

```text
               ┌──────────────────────────────┐
               │     Dough Hopper (Funnel)    │
               └──────────────┬───────────────┘
                              │
                              ▼
    ┌─────────────────► [ Portioning Chamber ] ◄──────────────┐
    │                                                         │
    │                                                         │
┌───┴──────────┐                                       ┌──────┴───────┐
│ Funnel Motor │                                       │ Piston Motor │
│   & Sensor   │                                       │   & Sensor   │
│ (GPIO 42/3)  │                                       │ (GPIO 36/7)  │
└──────────────┘                                       └──────┬───────┘
                                                              │
                                                              ▼
                                                   [ Dough Portion Ejection ]
```

### 2.1. Dough Hopper & Funnel Feeder (`FUNNELE_MOTOR`)
- **Function:** Feeds fresh dough into the intake throat of the portioning chamber.
- **Actuator:** AC induction motor driven via relay `FUNNELE_MOTOR` (GPIO 42 / Q1).
- **Position Sensor:** Inductive proximity sensor `FUNNELE_SENSOR` (GPIO 3 / I6). Active low indicates the funnel is in its retracted Home position.
- **Forward Travel Limit:** Bounded by `setting_max_funnle` milliseconds.

### 2.2. Cutting & Ejecting Piston (`PISTON_MOTOR`)
- **Function:** Retracts to pull a predetermined volume of dough into the calibrated cylinder, cuts the intake stream, and advances to eject the dough portion onto the outfeed belt.
- **Actuator:** AC induction motor driven via relay `PISTON_MOTOR` (GPIO 36 / Q7).
- **Position Sensor:** Inductive proximity sensor `PISTON_SENSOR` (GPIO 7 / I7). Active low indicates the piston has reached its top limit (Home position).
- **Stroke Duration (Weight Control):** Bounded by `auto_weight` milliseconds.

---

## 3. Electrical Drive System & Inverter Control

The machine employs a Variable Frequency Drive (VFD / Inverter) to provide soft starting, controlled deceleration, and bidirectional rotation:

- **Inverter Forward (`INVERTER_FORWARD`, GPIO 39 / Q2):** Drives the actuators forward/downward during charging and ejection strokes.
- **Inverter Reverse (`INVERTER_REVERSE`, GPIO 48 / Q3):** Drives the actuators backward/upward toward their Home sensor positions.

### Critical Inverter Interlock & Dead-Time:
Simultaneously driving Forward and Reverse control inputs causes an internal VFD trip or damage to the IGBT power stage.
The firmware enforces two rules:
1. De-energize the opposing direction before activating any direction line.
2. Maintain an explicit 100ms dead-time delay (`INVERTER_DEADTIME_MS`) between directional reversals.

---

## 4. Operational Process Parameters

| Parameter Name | Code Variable | NVS Key | Range | Engineering Description |
|---|---|---|---|---|
| **Dough Weight** | `auto_weight` | `"auto_weight"` | 0 to 1000 ms | Duration of piston discharge stroke; higher duration = greater volume/weight |
| **Cycle Interval (Pace)** | `auto_speed` | `"auto_speed"` | 0 to 2000 ms | Rest pause between successive portions to set operator working cadence |
| **Portion Counter** | `chone_counter`| `"chone_counter"`| 0 to 999,999 | Cumulative dough portion tally for current production shift |
| **Funnel Stroke Limit** | `setting_max_funnle`| `"max_funnle"`| 0 to 2000 ms | Maximum allowed forward duration for funnel feeding mechanism |
| **Piston Stroke Limit** | `setting_max_piston`| `"max_piston"`| 0 to 2000 ms | Maximum allowed upward return stroke for portioning piston |
| **Portion Beep** | `activeCounterSound`| `"ACSound"` | bool (true/false) | Enables acoustic buzzer feedback upon every portion completion |

---

## 5. Sensor Instrumentation & Peripheral Buses

### 5.1. Inductive Proximity Sensors
- **Piston Sensor (`PISTON_SENSOR`, GPIO 7):** Senses top dead center of the piston.
- **Funnel Sensor (`FUNNELE_SENSOR`, GPIO 3):** Senses home position of the funnel.
- **Signal Filtering:** Hardware pull-ups with software debouncing. Sensors must be inspected for flour dust buildup and physical alignment (2-4 mm nominal gap).

### 5.2. Maxim DS3231 Precision Real-Time Clock
- Connected via I2C (`SDA=GPIO 8`, `SCL=GPIO 9`).
- Provides battery-backed timestamps for shift logging, operating hour accumulation, and production records without requiring internet connectivity.

### 5.3. Dual MAX6675 K-Type Thermocouples
- Channels connected via SPI (`THERMO_SCK=GPIO 12`, `THERMO_MISO=GPIO 37`, `THERMO_1_CS=GPIO 4`, `THERMO_2_CS=GPIO 5`).
- Monitored for dough chamber ambient temperature and gearbox hydraulic oil thermal limits.

---

## 6. Industrial Troubleshooting Guide

| Symptom / Alarm | Root Cause | Engineering Solution |
|---|---|---|
| **Piston halts at top limit and cycle aborts** | Piston sensor failed to detect metal target or cable disconnected | Inspect GPIO 7 LED indicator, adjust sensor air gap to 2-3 mm |
| **Inconsistent dough portion weight** | Dough hopper empty, gluten inconsistency, or improper `auto_weight` | Calibrate `auto_weight`, check dough lubrication and hydration |
| **Funnel strikes mechanical limit** | `setting_max_funnle` set excessively high | Reduce funnel stroke duration in settings menu |
| **Keypad button inputs erratic** | Analog resistor ladder voltage shift due to electrical ground noise | Verify ground wiring; adjust ADC threshold windows in `getKeyFromAnalog` |
| **Capacitive touch unresponsive** | I2C bus lockup or reset pin stuck | Trigger hardware reset pulse on `TOUCH_RST` (GPIO 39) |
