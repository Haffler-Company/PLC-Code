# Architecture Decision Records (ADR) — Bakery HMI

This document records the architectural decisions, design context, and technical trade-offs governing the Bakery HMI firmware and hardware systems.

---

## ADR-001: Adoption of PlatformIO as Unified Build & Dependency System
- **Status:** Approved.
- **Context:** The legacy project was structured as an Arduino `.ino` file with flat directory contents, making dependency resolution, flash/PSRAM build flags, and reproducible builds brittle.
- **Decision:** Establish a root `platformio.ini` targeting `esp32-s3-devkitc-1` with 16MB flash, PSRAM enabled (`-DBOARD_HAS_PSRAM`), shared libraries linking, and custom partition tables.
- **Consequences:** Deterministic single-command local builds (`pio run`), alignment with enterprise projects (`Welding_single_head`), and seamless CI/CD readiness.

---

## ADR-002: Dual-Core FreeRTOS Task Segregation
- **Status:** Approved.
- **Context:** Industrial motion control (motor relays, VFD direction, inductive sensors, keypad debouncing) requires deterministic real-time timing. Concurrently, rendering LVGL 8.3 on a 480x320 display with capacitive touch introduces variable execution latencies.
- **Decision:**
  - **Core 1 (UI & Rendering Dedicated):** `lv_tick_task` and `lvgl_task` run exclusively on Core 1.
  - **Core 0 (Industrial Motion & Machine Control Dedicated):** `app_task`, `keypad_task`, and `buzzer_task` run exclusively on Core 0.
- **Consequences:** Eliminates frame stutter, guarantees immediate response to operator Stop commands, and prevents display lag from interfering with actuator timing.

---

## ADR-003: NVS Storage with Magic Number Integrity Verification
- **Status:** Approved.
- **Context:** Bakeries experience frequent power interruptions. Machine settings and portion counts must survive abrupt power loss without data corruption or resetting to uncalibrated defaults.
- **Decision:** Utilize the ESP-IDF `Preferences` API under namespace `"chone"`. Validate storage validity using `MAGIC_NUMBER = 0xBBFF0FF0`. If missing or invalid, initialize calibrated factory defaults.
- **Consequences:** Guarantees data persistence across power cycles and avoids uninitialized memory values after initial flashing.

---

## ADR-004: Inverter Directional Software Interlock & Hardware Dead-Time
- **Status:** Approved.
- **Context:** Driving both forward and reverse inputs of a frequency inverter simultaneously can damage internal driver circuits or trip overcurrent protections.
- **Decision:** All motion functions (`pistonDown`, `pistonUP`, `funneleRight`, `funneleLeft`) strictly clear the opposing directional relay before asserting the requested direction, with an enforced dead-time of at least 100ms (`INVERTER_DEADTIME_MS`).
- **Consequences:** 100% protection against inverter control short-circuits.

---

## ADR-005: 16MB Flash Partition Table with Dual OTA Slots
- **Status:** Approved.
- **Context:** Upgrading firmware in industrial bakeries must support safe Over-The-Air (OTA) flashing with automated rollback in case of corrupted uploads.
- **Decision:** Allocate two 7MB application partitions (`app0` and `app1`) alongside a 1.87MB `spiffs` partition at `0xE20000` via `partitions.csv`.
- **Consequences:** Enables full dual-slot OTA firmware updates matching the enterprise standard of `Welding_single_head`.
