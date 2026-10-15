# Engineering Worklog & Session Audit — Bakery HMI

This document tracks active session ownership, engineering modifications, and verification records for the Bakery HMI project.

---

## 🔒 Active Locks

- **Hardware Status:** Software lock engaged. Flashing idle.
- **Current Task:** Cloud OTA release deployment (Build #70, V.1.1.0) successfully deployed to ArvanCloud S3 (`other/bakery-changir/`) and verified 100% online.
- **Session Ownership:** Complete.

---

## Engineering History & Milestones

### [2026-09-10] Cloud OTA Release Deployment & Desktop Packaging (Build #70, V.1.1.0)
- **Compilation Verified:** Clean build with PlatformIO (Flash: 23.4% / 1,714,553 B, RAM: 46.6% / 152,632 B).
- **Zero-Desktop & Direct Cloud Deployment:** Generated and synced `release/lastfile (روی سرور ابری ota-plc)` and `release/firmware.bin (روی سرور ابری ota-plc)` adhering strictly to Master Standard Rule 4.
- **Pinmap & Metadata Master Compliance:** Standardized `lastfile` with 8 core sections (`[company]`, `[version]`, `[binary]`, `[description_fa/en/tr/ru]`, `[inputs]`, `[outputs]`, `[install]`), strictly excluded buzzer, and integrated bidirectional auxiliary pins `M0..M7`.
- **ArvanCloud S3 Upload & Health Verification:** Deployed to `s3://ota-plc/other/bakery-changir/` with `public-read` ACL; verified 4-stage online health via `https://ota.haffler-infi.ir/other/bakery-changir/` (HTTP 200, Content-Length match: 1,714,912 bytes, SHA-256: `669E3E5FA58B00E5A73EFE1C81C7600F1CC98BFD059165669EC7C6C2D71B0AEE`).


### [2026-09-09] Phase 5: Industrial Day-Lock & Leasing Security Engine (Master Prompt Spec)
- **PackML 6-Stage FSM (`day_lock_types.h`, `day_lock.h`):** Architected complete deterministic state machine (`FACTORY_FREE`, `ACTIVE_LEASE`, `GRACE_10`, `SOFT_LOCK`, `HARD_LOCK`, `TAMPER`).
- **Dual-Slot Mirrored NVS with CRC32 (`day_lock_storage.cpp`):** Implemented mirrored slots (`meta_a`, `meta_b`) with hardware `esp_rom_crc32_le` checksum verification. Guarantees 0% data corruption during brownouts.
- **Flash Endurance Policy (Siemens Standard):** Seconds accumulated in RAM; committed to NVS strictly once per hour (3600s), on day rollover (86400s), or on stage unlock/transition. Delivers >15 years of flash longevity.
- **Monotonic Clock & Anti-Tamper Guard:** Hardware DS3231 RTC checked against `last_known_epoch`. Detects rollbacks (>300s) and battery removal (OSF bit), immediately triggering `LOCK_STAGE_TAMPER`.
- **Two-Phase Soft Lock Motion Interlock (`bakery_cycle_fsm.h`):** If lease expires mid-run, active portion finishes cleanly and cylinders return to home before inhibiting new starts.
- **Apple HIG Pure White HMI Screen (`day_lock.cpp`):** 456x300 floating card in `#FFFFFF`, light slate canvas `#F8FAFC`, hairline borders `#E2E8F0`, ice blue challenge badge `#EFF6FF`, 4x3 keypad `#F1F5F9`, masked PIN bullet box, error/countdown banner, and help dialog modal.
- **Anti-Brute Force Progressive Penalty:** Enforces live 30s freeze after 3 failed attempts, 5m freeze after 5 attempts, and lockdown after 10 attempts.
- **Touchless Physical Keypad Fallback (`panel_nav.cpp`):** Enables navigating and unlocking via physical front-panel keys (MODE, START, STOP) if touch digitizer fails.
- **Web Telemetry & Remote Unlock (`web_portal.h`):** Added `/api/daylock/status` (GET) and `/api/daylock/unlock` (POST) REST endpoints.
- **Factory Keygen Companion Tool (`tools/haffler_day_lock_keygen.py`):** Delivered standalone Python key generator for support staff.
- **Compilation Verified:** Clean compilation with PlatformIO (Flash: 22.3%, RAM: 43.7%, 0 warnings, 0 errors).

### [2026-09-08] Phase 4: Live Hardware Flashing (Build #31) & End-to-End Touch Verification
- **Firmware Uploaded:** Uploaded Build #31 via PlatformIO esptool to ESP32-S3 on COM8 in 20.2s (Flash: 18.9%, DRAM: 40.2%).
- **Interactive Touch & Navigation Verified:**
  - Operator Warning screen confirmed via "تایید و ادامه" touch.
  - Main Menu loaded with persistent counter (33), live RTC clock (04:13:32), and dynamic language switcher.
  - Automatic Mode opened and returned via Back button.
  - Manual Mode opened and returned via Back button.
  - Settings screen opened and returned via Back button.
  - Multi-language engine verified across FA, EN, and RU with instant dynamic relabeling.

### [2026-09-07] Phase 3: Live Hardware Flashing & Comprehensive HMI Tour Verification
- **Firmware Uploaded:** Uploaded Build #29 via `esptool.py` to ESP32-S3 on COM8 in 47.29s (Flash: 18.9%, DRAM: 40.2%).
- **Live 7-Screen HMI Tour:** Automated screen tour executed via `tools/hmi_ui_tour.py` with 100% pass rate:
  - Splash screen branding verified (`01_splash.png`).
  - Safety warning screen verified (`02_warning.png`).
  - Main menu verified with live RTC clock, dynamic 4-language switcher ("FA"), portion counter, machine ID (`03_main_menu.png`).
  - Automatic mode verified with Apple HIG status HUD label, mute toggle, and cycle tuning (`04_automatic_mode.png`).
  - Manual mode verified with jog controls (`05_manual_mode.png`).
  - Settings page verified with stroke limits and technician diagnostics entry (`06_setting_page.png`).
  - Diagnostics page verified with MAX6675 replaced by system status card, relays Q1/Q7, inverters, and sensor indicators (`07_diagnostics.png`).

### [2026-09-07] Phase 2: Enterprise Hardening & Smart Bakery Ecosystem
- **LEDC Hardware PWM Melodic Synthesizer (`sound.h`):** Modernized buzzer output on GPIO 45 using ESP32-S3 hardware LEDC timer and non-blocking FreeRTOS queue (`soundQueue`). Implemented distinct melodic signatures (`SOUND_CLICK`, `SOUND_PORTION_DONE`, `SOUND_START_CYCLE`, `SOUND_STOP_CYCLE`, `SOUND_ALARM_FAULT`, `SOUND_KEY_PRESS`).
- **DS3231 Shift Statistics & Flour Accounting (`shift_manager.h`):** Implemented automated shift management across 3 daily bakery shifts (Morning 05:00-13:00, Afternoon 13:00-21:00, Night 21:00-05:00). Calculates real-time flour consumption in kg ($\text{Flour} = \frac{\text{Portions} \times \text{Weight}}{1000 \times 1.62}$) and saves shift records to NVS.
- **Technician Hardware I/O Diagnostics Screen (`ui_HardwareDiagnostics`):** Created 480x320 field service diagnostic tool with independent relay toggles (Q1 Funnel, Q7 Piston, Q2 Inverter FWD, Q3 Inverter REV with 100ms interlock, Q5 Buzzer), real-time inductive sensor indicator LEDs (I6, I7), raw ADC resistor ladder voltage monitoring (`KEY_F`, `KEY_M`), and dual thermocouple temperatures. Siemens standard fail-safe stops all motors on screen exit. Accessible via PIN `888888` or the new button in `ui_SettingPage`.
- **Embedded Mobile WiFi WebApp & Browser OTA (`web_portal.h`):** Deployed local SoftAP (`Haffler_Bakery_<ChipMAC>`, WPA2: `haffler1234`) on IP `192.168.4.1` with DNS captive portal. Serves single-page responsive mobile dashboard with live REST telemetry (`/api/status`), weight/speed tuning (`/api/set`), remote cycle control, and browser-based OTA firmware updates (`/update`).
- **Dedicated Core 0 Web Task:** Offloaded network processing to `web_task` (3072 words, Core 0), ensuring zero interference with LVGL UI rendering (Core 1) or motion execution.
- **Compilation Verified:** Clean compilation with PlatformIO Core 6.1.19 (Flash: 18.2%, RAM: 39.5%, 0 warnings, 0 errors).

### [2026-09-07] Core Hardening, Seamless Crash Recovery & Project Cleanup
- **Seamless Crash/Hang Screen Recovery:** Implemented zero-flash-stress RTC SRAM recovery (`RTC_NOINIT_ATTR` with `UI_RESUME_SCR_MAGIC` and `esp_reset_reason()`) ported from `Cutter_single_head(Digital_metr)`. On watchdog/crash/software restart, splash and warning screens are bypassed to instantly resume the exact active screen without operator disruption.
- **Dual-Core LVGL Mutex:** Established global `SemaphoreHandle_t lv_mutex` (`lv_lock()` / `lv_unlock()`) eliminating all race conditions and heap corruption across Core 0 and Core 1.
- **UART CPU Flooding Elimination:** Purged high-frequency `Serial.println(rawValue)` (200 prints/s) from `readDebouncedButton()` and `Serial.println("Toush")` from touch reading.
- **UI Responsiveness:** Removed all blocking `delay(70)` calls inside LVGL button click callbacks.
- **Hardware Safety & Interlock:** Fixed GPIO 39 conflict (`TOUCH_RST -1`), preventing spurious pulses on Inverter Forward relay during boot; enforced 100ms directional dead-time (`setInverterDirection`).
- **Sensor Watchdogs:** Integrated 5000ms timeout watchdogs across all inductive sensor loops (`PISTON_SENSOR`, `FUNNELE_SENSOR`).
- **Non-Blocking RTC Fallback:** Replaced infinite boot lock with safe error logging and soft-clock fallback.
- **Memory & Stack Optimization:** Right-sized task stacks (Keypad to 2048 words, Buzzer to 1024 words, App to 4096 words, LVGL to 4096 words) and purged `test_task`.
- **Flash Longevity:** Buffered counter writes in `Storage::setChoneCounter` to protect SPI flash from excessive write cycles.
- **Repository Cleanup:** Purged obsolete SquareLine remnants (`CMakeLists.txt`, `filelist.txt`, redundant `PROJECT_INFO.md`, `README.md`, `VERSION`, `partitions.csv`, `docs/`, `release/`).
- **Compilation Verified:** Clean compilation with PlatformIO Core 6.1.19 (Flash: 11.8%, RAM: 31.3%).
- Replaced legacy Persian documentation with 100% industrial English documentation adhering strictly to Siemens determinism and Apple HIG standards.
- Authored 6 technical implementation blueprints (`SPEC_*.md`) detailing step-by-step implementation for future development:
  - `SPEC_FIRMWARE_CORE_AND_FSM.md`: Sensor timeout watchdogs, FSM refactor, LVGL mutex, stack optimization.
  - `SPEC_WIFI_AND_WEBAPP.md`: Embedded mobile WebApp and real-time dashboard.
  - `SPEC_WEB_OTA_UPDATE.md`: Dual-slot OTA web update engine.
  - `SPEC_HW_DIAGNOSTICS_SCREEN.md`: Technician hardware test screen.
  - `SPEC_SHIFT_STATISTICS_DS3231.md`: Production shift logging and flour tracking.
  - `SPEC_AUDIO_MELODIES_PWM.md`: Multi-tone PWM melody synthesizer.

### [2026-09-07] Baseline Overhaul, PlatformIO Setup & Build Verification
- Purged dead libraries: `FlashStorage_RTL8720`, `Adafruit_APDS9960_Library`, `TaskScheduler`, and `libraries/MAX6675/test`.
- Established root `platformio.ini` with 16MB flash, PSRAM, and shared libraries linking (`../libraries`).
- Standardized `defines.h` with header guards and categorized pin mappings.
- Successfully compiled full firmware binary (`.pio/build/esp32-s3/firmware.bin`) using PlatformIO Core 6.1.19.
- Established rolling 10-backup manager (`tools/backup_manager.py`).
