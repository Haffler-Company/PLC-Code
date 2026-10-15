# Strategic Implementation & Architecture Roadmap — Bakery TODO

This document defines the phased implementation roadmap for refactoring, hardening, and enhancing the Bakery HMI system, benchmarked against enterprise capabilities proven in `Welding_single_head`.

---

## Phase 1: Core Reliability, Safety Hardening & FSM Refactor (Priority 1 — Completed)

- [x] **Seamless Crash & Hang Recovery (Zero Flash Wear):**
  - Implement `RTC_NOINIT_ATTR` screen state tracking in internal RTC SRAM (ported from `Cutter_single_head(Digital_metr)`).
  - Check `esp_reset_reason()` on boot; bypass splash/warning on watchdog/panic/SW reset to resume last screen instantaneously.
- [x] **Sensor Watchdog Timeout Protection:**
  - Enforced `SENSOR_MOTION_TIMEOUT_MS = 5000` across all inductive sensor loops.
  - Implemented immediate fail-safe motor stop on sensor timeout.
- [x] **Thread-Safe LVGL Mutex Architecture:**
  - Instantiated global `SemaphoreHandle_t lv_mutex` (`lv_lock()` / `lv_unlock()`).
  - Wrapped all cross-task LVGL mutations and `lv_timer_handler()` in mutex guards.
- [x] **Inverter Interlock & Dead-Time (Siemens Standard):**
  - Enforced 100ms hardware dead-time (`INVERTER_DEADTIME_MS`) between forward and reverse rotation commands.
  - Resolved GPIO 39 conflict (`TOUCH_RST -1`), eliminating false pulse on Inverter Forward relay during boot.
- [x] **UART CPU Flooding & Lag Elimination:**
  - Removed 200 prints/s from ADC keypad debouncer and touch polling.
  - Removed all `delay(70)` blocking calls from LVGL button click events.
- [x] **FreeRTOS Stack Rightsizing:**
  - Eliminated diagnostic `test_task`.
  - Downscaled `keypad_task` to 2048 words, `buzzer_task` to 1024 words, `app_task` to 4096 words, `lvgl_task` to 4096 words.
- [x] **Flash Memory Protection:**
  - Implemented buffered counter updates in NVS storage to protect flash wear-out.

---

## Phase 2: Enterprise Features Ported from Welding_single_head (Priority 2 — Completed)

- [x] **Embedded WiFi Mobile Dashboard (WebApp):**
  - Launched internal ESP32 soft-AP (`Haffler_Bakery_<ChipMAC>`) with DNS captive portal.
  - Served lightweight responsive WebApp allowing bakery managers to monitor live portion counts, batch weight, and speed directly on smartphones.
- [x] **Browser-Based Web OTA Firmware Updates:**
  - Implemented dual-slot OTA firmware upload via web browser, enabling wireless updates in flour-rich bakery environments without physical USB cables.
- [x] **Technician Hardware I/O Diagnostic Screen:**
  - Added password-protected screen to independently toggle relays (`PISTON_MOTOR`, `FUNNELE_MOTOR`, `INVERTER_FORWARD`, `INVERTER_REVERSE`, `BUZZER`) and monitor live inductive sensor states.
- [x] **Shift-Based Production Statistics (DS3231 RTC):**
  - Recorded daily, morning, and evening shift portion tallies, calculating estimated flour consumption (kg) and storing historical records in NVS.
- [x] **Multi-Tone Acoustic Melodies (LEDC PWM):**
  - Synthesized distinctive melodic audio cues for cycle start, portion completion, shift end, and emergency fault alarms using hardware PWM.

---

## Phase 3: Visual Craftsmanship & Multi-Language Support (Priority 3)

- [ ] **Tri-Language Internationalization Engine:**
  - Implement runtime language switching between Persian, English, and Turkish.
- [ ] **Industrial Backlight Screensaver:**
  - Automatically dim or shut off display backlight after 5 minutes of idle time to prevent display burn-in and save energy.
- [ ] **Analog Resistor Ladder Auto-Calibration:**
  - Add on-screen ADC calibration wizard to automatically learn voltage thresholds if electrical supply noise causes keypad drift.
