# Bakery Industrial Operator Vocabulary & Glossary

This document maps Persian workshop terminology, bakery slang, and operator phrases to standardized English engineering concepts and codebase identifiers.

---

## 1. Technical Vocabulary Mapping

| Persian Term | Romanized | English Engineering Term | Code Identifier | Functional Role |
|---|---|---|---|---|
| **چانه** | Chone | Dough Ball / Portion | `chone` / `portion` | Calibrated dough piece portioned for bread baking |
| **چانه‌گیر / چانه‌کن** | Chone-Gir | Dough Divider & Rounder | `Bakery` / `ChoneProject` | Automatic dough portioning machine |
| **قیف** | Gheef (Funnel) | Dough Hopper / Feeder | `funnle` / `FUNNELE_MOTOR` | Funnel-shaped bulk dough feeder |
| **پیستون** | Piston | Cutting & Ejecting Plunger | `piston` / `PISTON_MOTOR` | Cylindrical plunger that draws, cuts, and pushes dough |
| **وزن چانه** | Vazn-e Chone | Portion Weight | `auto_weight` | Discharge stroke duration determining portion mass |
| **وقفه پیستون (سرعت)** | Vaghteh / Sor'at | Cycle Interval / Pace | `auto_speed` | Rest pause between successive portions |
| **حد قیف** | Hadd-e Gheef | Funnel Stroke Limit | `setting_max_funnle` | Maximum forward travel duration for hopper |
| **حد پیستون** | Hadd-e Piston | Piston Stroke Limit | `setting_max_piston` | Maximum retraction travel duration for plunger |
| **شمارنده** | Shomarandeh | Cumulative Portion Tally | `chone_counter` | Production shift piece counter |
| **سنسور قیف** | Sensor-e Gheef | Funnel Home Sensor | `FUNNELE_SENSOR` (I6) | Inductive sensor detecting hopper zero return |
| **سنسور پیستون**| Sensor-e Piston| Piston Top Home Sensor | `PISTON_SENSOR` (I7) | Inductive sensor detecting plunger top position |
| **اینورتر** | Inverter | Variable Frequency Drive | `inverter` | Electronic motor speed & direction drive |
| **راست‌گرد** | Rast-Gard | Inverter Forward | `INVERTER_FORWARD` (Q2) | Forward/downward motion direction |
| **چپ‌گرد** | Chap-Gard | Inverter Reverse | `INVERTER_REVERSE` (Q3) | Backward/upward motion direction |
| **حالت اتوماتیک** | Halat-e Automatic | Automatic Continuous Mode | `AUTOMATIC` / `ui_AutomaticMode` | Continuous portioning cycle |
| **حالت دستی** | Halat-e Dasti | Manual / Jog Mode | `MANUAL` / `ui_Manual_Mode` | Individual actuator jogging for maintenance |
| **تنظیمات** | Tanzimat | Settings Menu | `SETTING` / `ui_SettingPage` | Password-protected parameter calibration |
| **ریست شمارشگر** | Reset-e Shomarandeh| Counter Reset | `automatic_CounterResetButton` | Password-protected portion tally clear |

---

## 2. Common Operator Phrases & Root Cause Mapping

- **"دستگاه خمیر پرت نمیکنه" (Machine does not eject dough):** Verify `PISTON_MOTOR` relay (GPIO 36) and check if `auto_weight` is set to 0.
- **"وزن چانه‌ها یکدست نیست" (Inconsistent portion weights):** Inspect hopper filling level or adjust `setting_max_funnle` to ensure full chamber charge.
- **"دستگاه گیر کرده و موتور خاموش نمیشه" (Actuator jammed, motor runs indefinitely):** Inductive sensor signal missing; trigger E-Stop and inspect `PISTON_SENSOR` or `FUNNELE_SENSOR` alignment.
- **"سرعت ضربه زیاده، نانوا نمی‌رسه" (Cadence too fast for operator):** Increase `auto_speed` value on Automatic screen.
