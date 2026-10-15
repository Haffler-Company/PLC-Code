# Project Master Onboarding & Routing Guide — Bakery HMI

This document is the primary entry point for every firmware engineer, technician, and AI assistant (Gemini, Claude, Cursor, Antigravity). Read this file first before navigating or modifying the repository.

**Project:** Industrial Bakery Dough Divider & Rounder HMI / PLC on ESP32-S3 + LVGL 8.3 + FreeRTOS + Frequency Inverter Drive + PlatformIO.

---

## 0. Fundamental Engineering Principles

- **Iron-Clad Siemens & Apple Standards Mandate:** All firmware architecture, inverter motion logic, and UI design must strictly comply with Siemens industrial determinism and safety and Apple HIG craftsmanship standards.
- **Active Version Tracking:** Automatically stamped on each build via `version.json` and `V.1.1.0/ChoneProject/build_time.h`.
- **9-Pillar Operational Governance:** [`docs/AGENT_PLAYBOOK.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/AGENT_PLAYBOOK.md).
- **Master Engineering Contract & Anti-Hallucination Guardrails:** [`AGENTS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/AGENTS.md).

---

## 1. Technical Documentation Navigation Map

| Document | When to Read | Purpose & Scope |
|---|---|---|
| [`docs/BAKERY_HMI_KNOWLEDGE.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/BAKERY_HMI_KNOWLEDGE.md) | Before modifying machine logic | Complete encyclopedia of dough portioner mechanics, electronics, timing, and diagnostics |
| [`docs/BAKERY_CYCLE_FSM.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/BAKERY_CYCLE_FSM.md) | Before modifying `tasks.h` | Finite State Machine (FSM), phase breakdowns, timing, and safety interlocks |
| [`docs/HARDWARE_IO_PINOUT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/HARDWARE_IO_PINOUT.md) | Before checking pins or voltages | Complete GPIO mapping, electrical characteristics, inductive sensors, inverters |
| [`docs/REFERENCE.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/REFERENCE.md) | During implementation | Quick lookup for pin numbers, NVS keys, FreeRTOS stack allocations, and limits |
| [`docs/OPERATOR_VOCABULARY.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/OPERATOR_VOCABULARY.md) | When analyzing user requests | Industrial workshop glossary (Persian dough-making terms mapped to English code) |
| [`docs/UI_SCREEN_INVENTORY.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/UI_SCREEN_INVENTORY.md) | Before altering UI screens | Screen widget hierarchy, event callbacks, modal popups, and keypad handlers |
| [`docs/ARCHITECTURE_DECISIONS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/ARCHITECTURE_DECISIONS.md) | Before structural refactors | Architecture Decision Records (ADRs) explaining system design rationales |
| [`docs/KNOWN_ISSUES_AND_DEBT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/KNOWN_ISSUES_AND_DEBT.md) | Before bugfixing or maintenance | Documented vulnerabilities, stack over-allocations, and race conditions |
| [`docs/TODO_ROADMAP.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/TODO_ROADMAP.md) | Before planning new features | Strategic implementation roadmap benchmarked against `Welding_single_head` |
| [`docs/INTER_FILE_API_CONTRACT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/INTER_FILE_API_CONTRACT.md) | Before altering module links | Data flow and shared variable contracts between UI, Tasks, and Storage |
| [`docs/WORKLOG.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/WORKLOG.md) | Before and after any work | Historical engineering log, active locks, and discrete change tracking |

---

## 2. Implementation Specifications (Ready-to-Implement Blueprints)

These step-by-step blueprints guide the direct implementation of features without guesswork:

1. [`docs/SPEC_FIRMWARE_CORE_AND_FSM.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/SPEC_FIRMWARE_CORE_AND_FSM.md): FSM refactoring, sensor timeout guards, LVGL mutex thread-safety, and FreeRTOS stack optimization.
2. [`docs/SPEC_WIFI_AND_WEBAPP.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/SPEC_WIFI_AND_WEBAPP.md): Embedded WiFi Access Point, captive portal, and real-time mobile dashboard.
3. [`docs/SPEC_WEB_OTA_UPDATE.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/SPEC_WEB_OTA_UPDATE.md): Browser-based binary OTA flashing with dual partition slots and verification.
4. [`docs/SPEC_HW_DIAGNOSTICS_SCREEN.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/SPEC_HW_DIAGNOSTICS_SCREEN.md): Interactive technician diagnostic screen for live testing of all relays and sensors.
5. [`docs/SPEC_SHIFT_STATISTICS_DS3231.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/SPEC_SHIFT_STATISTICS_DS3231.md): Shift-based dough production accounting and flour estimation with DS3231 RTC.
6. [`docs/SPEC_AUDIO_MELODIES_PWM.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/SPEC_AUDIO_MELODIES_PWM.md): Multi-tone melody buzzer synthesizer over LEDC PWM.

---

## 3. Build, Verification & Tool Commands

1. **Local Build (Compile Only):**
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
   ```
2. **Rolling Backup & Git Micro-Commit (Required after every 1-2 edits):**
   ```powershell
   python tools/backup_manager.py commit "<discrete_change_description>"
   ```
3. **Flashing Hardware (⚠️ Strictly requires explicit owner command):**
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COMx
   ```

---

## 4. Core Firmware Source Map (`V.1.1.0/ChoneProject/`)

| File | Subsystem & Responsibility |
|---|---|
| `ChoneProject.ino` | Hardware initialization, display/touch drivers, FreeRTOS dual-core task spawns |
| `defines.h` | Clean hardware GPIO mapping, timing limits, and logic level constants |
| `general.h` | Low-level relay drivers (`pistonDown`, `funneleLeft`), analog keypad ADC debouncing |
| `tasks.h` | FreeRTOS tasks (LVGL tick, LVGL timer, Main App portioning loop, Keypad, Buzzer) |
| `storage.h` | NVS persistence wrapper (`Preferences`) for weight, interval, counter, limits |
| `lvgl_function.h` | UI event callbacks, virtual keypads, and synchronization between UI and machine state |
| `lv_conf.h` | LVGL 8.3 library configuration, buffer settings, fonts, color depth |
| `ui.c` / `ui.h` | SquareLine Studio generated widget tree and screen descriptors |
