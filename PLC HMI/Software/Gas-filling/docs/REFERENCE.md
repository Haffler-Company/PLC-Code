# Technical Reference (Gas-filling)

## Platform

| Item | Value |
|---|---|
| Controller/HMI | ESP32-S3 (QFN56 rev v0.2), dual-core 240 MHz, Arduino ESP32 core |
| Memory | 16 MB Flash (`FlashSize=16M`), 2 MB embedded PSRAM (`PSRAM=enabled`) |
| Partition Scheme | Dual OTA 7MB + 7MB (`partitions.csv`: `app0` @ 0x10000, `app1` @ 0x710000), NVS preserved |
| Display | 480x320 ST7796 TFT, TFT_eSPI 2.5.43, SPI 40 MHz, `TFT_BGR`, rotation 3, `setSwapBytes(true)` |
| Touch | GT911 Capacitive touch over I2C (`bb_captouch` / direct driver) |
| Serial / Upload | USB-CDC Native (`VID_303A`), default **COM8** (or COM3) @ 921600 baud. DTR/RTS kept low. |
| UI Framework | LVGL 9.5.0 (`LV_COLOR_FORMAT_RGB565`, 40-line partial buffer) |

### Complete Hardware Pin Mapping

| Subsystem | Signal | GPIO | Notes / Electrical Characteristics |
|---|---|---|---|
| **Display (SPI)** | MOSI | GPIO10 | HSPI / FSPI data |
| | MISO | GPIO11 | HSPI / FSPI data |
| | SCLK | GPIO12 | SPI Clock (40 MHz) |
| | CS | GPIO13 | Chip Select |
| | DC | GPIO14 | Data / Command |
| | RST | -1 | Tied to system reset circuit |
| **Touch (I2C)** | SDA | GPIO8 | I2C Data (400 kHz) |
| | SCL | GPIO9 | I2C Clock (400 kHz) |
| | INT | GPIO40 | Touch Interrupt |
| | RST | GPIO39 | Touch Reset |
| **Outputs** | Valve 1 | GPIO16 | Q0 relay: Glass 2 (Left) Argon Valve output (active state depends on N.O/N.C) |
| | Valve 2 | GPIO42 | Q1 relay: Glass 1 (Right) Argon Valve output (active state depends on N.O/N.C) |
| | Buzzer | GPIO45 | Audible feedback (LEDC PWM @ 7000 Hz) |
| | Relay Q2 | GPIO15 | On-board relay output (Spare / Diagnostic) |
| | Relay Q3 | GPIO35 | On-board relay output (Spare / Diagnostic) |
| | Relay Q4 | GPIO36 | On-board relay output (Spare / Diagnostic) |
| | Relay Q5 | GPIO38 | On-board relay output (Spare / Diagnostic) |
| | Relay Q6 | GPIO41 | On-board relay output (Spare / Diagnostic) |
| | Relay Q7 | GPIO48 | On-board relay output (Spare / Diagnostic) |
| **Inputs (Analog)** | F-Key Ladder | GPIO1 | Band-encoded ADC (800..4095): F1, F2, F3, F4, F5. Configured with `ADC_11db`. |
| | Panel Keys | GPIO2 | Band-encoded ADC (800..4095): MODE, START, STOP, SAVE, CANCEL. Configured with `ADC_11db`. |
| **Inputs (Opto)** | Foot Pedal | GPIO7 | Optocoupled digital input (Active-LOW: short to GND triggers Start) |
| | Emergency Stop | GPIO3 | Optocoupled digital input (Active-LOW: short to GND triggers instant stop <70ms) |

## Firmware Map

| File | Responsibility |
|---|---|
| `GasFilling_HMI/GasFilling_HMI.ino` | Startup, hardware init, display flush, GT911 touch driver, 100ms hardware timer ISR, main loop |
| `GasFilling_HMI/actions.c` / `.h` | UI event handlers, keyboard management, Start/Stop logic, screen transitions, calculations |
| `GasFilling_HMI/screens.c` / `.h` | LVGL 9 screen layouts (Main, Setting: General, Process, Test, Guide, Admin, OTA modal) |
| `GasFilling_HMI/panel_keys.cpp` / `.h` | 10-key front-panel ADC scanner, optocoupled pedal & E-stop handler, touchless operation |
| `GasFilling_HMI/ota_manager.cpp` / `.h` | Server OTA update client: lastfile parsing, HTTP 16KB stream, SHA-256 validation, Core 0 worker |
| `GasFilling_HMI/webportal.cpp` / `.h` | SoftAP `192.168.4.1` web server, live telemetry, CSV production log export, web OTA |
| `GasFilling_HMI/day_lock.cpp` / `.h` | 5-stage leasing day lock, offline challenge-response security, emergency code unlock |
| `GasFilling_HMI/a2_eeprom.cpp` / `.h` | EEPROM / NVS persistence with is_dirty() shadow buffer to eliminate flash wear |
| `GasFilling_HMI/variables.c` / `.h` | Global shared process variables, ISR flags, dimension values, output states (`extern "C"`) |
| `GasFilling_HMI/agent_hmi.h` | Serial agent protocol (`PING`, `STAT`, `SHOT`, `SAFE`, `PRESS`, `REL`, `PERF`) |
| `GasFilling_HMI/ui_toast.cpp` / `.h` | Non-blocking industrial notification toasts (Info, Success, Warning, Error) |
| `GasFilling_HMI/ui_breath.c` / `.h` | Ambient UI status pulsing animation on Start button during injection |
| `GasFilling_HMI/ui_vazir_fonts.c` / `.h` | Persian Vazirmatn font glyph loader and bitmap providers |
| `GasFilling_HMI/version.h` | Auto-generated semantic version header (`FW_VERSION`, `FW_VERSION_LABEL`) |
| `GasFilling_HMI/fw_changelog.h` | Structured firmware changelog entries (13-word max per version) |

## Process Calculation & Formulas

The firmware calculates gas volume and filling duration from operator-entered glass dimensions:

1. **Internal Glass Volume ($V$ in liters):**
   $$V = \frac{\text{Length (cm)} \times \text{Height (cm)} \times \text{Width (cm)}}{1000}$$

2. **Argon Gas Quantity ($Gas$ in normal liters):**
   $$Gas = \left(\frac{P_1}{P_2}\right) \times V$$
   *(Where $P_1$ is source pressure and $P_2$ is ambient/target pressure; $P_2 > 0$ strictly guarded).*

3. **Required Injection Duration ($t$ in seconds):**
   $$t = \left(\frac{Gas}{Q_s}\right) \times 60$$
   *(Where $Q_s$ is the calibrated flow rate in L/min; $Q_s > 0$ strictly guarded).*

4. **Hardware Timer Ticks ($100\text{ ms}$ resolution):**
   $$\text{TimerValveSaved} = \text{uint32\_t}\left(t \times 10.0\text{f} + 0.5\text{f}\right)$$
   *(Deterministic rounding prevents truncation errors of the legacy `(int)t * 10`).*

5. **Hard Watchdog Cutoff:**
   Maximum injection duration is capped at **180 seconds** ($1800$ timer ticks). If an operator enters an abnormal dimension or a valve gets stuck, the firmware unconditionally closes the valve at 180s to prevent emptying the Argon cylinder.

## Runtime Rules Learned from Hardware

- **ISR Safety (Siemens Determinism):** The 100 ms timer ISR (`onTimer`) MUST NOT call any LVGL function (`lv_*`, `ChangeBar`, `HideLogo`). The ISR sets atomic volatile flags (`ui_bar1_pending`, `ui_complete1_pending`), which are safely consumed in `loop()` by `ui_process_isr_requests()` before `lv_timer_handler()`.
- **GT911 Touch Mapping:** The GT911 controller operates in native portrait coordinates ($0..319 \times 0..479$). When rotated 270° for 480x320 landscape, the correct mapping is:
  $$\text{display\_x} = 479 - \text{raw\_y}, \quad \text{display\_y} = \text{raw\_x}$$
- **GT911 Release Acknowledgment:** GT911 status register `0x814E` must be acknowledged even for 0-point release frames; otherwise the controller retains stale state and stops publishing touches.
- **Display Flush & Partial Redraw:** Display buffer size is partial 40 lines ($480 \times 40 \times 2 = 38,400\text{ bytes}$). Flush callback calls `lv_display_flush_ready()` immediately after pixel transfer.
- **Boot Sequence:** Wi-Fi is disabled immediately (`WiFi.mode(WIFI_OFF)`), touch I2C is initialized early, LVGL buffers are registered before `flush_cb`, initial UI is rendered, and indev is attached after first paint to prevent race conditions.
- **EEPROM Shadow Buffer & Zero Wear:** Parameter commits (`a2_eeprom_write_all()`) use an in-memory shadow structure and `is_dirty()` comparison. If values are unchanged, flash write and `EEPROM.commit()` are bypassed completely, eliminating flash wear and UI stutter upon modal/keyboard closure.
- **Server OTA Architecture:**
  - Endpoint: `http://ota.haffler-infi.ir` over plain HTTP port 80 (avoids mbedTLS heap exhaustion on ESP32-S3).
  - Background Task: Runs on FreeRTOS Core 0 with 8KB stack, communicating with Core 1 LVGL UI via atomic flags and `ota_manager_poll()` in `loop()`.
  - Fail-Safe Stream: 16KB HTTP streaming buffer, on-the-fly SHA-256 verification against server `lastfile`, rollback-safe `esp_ota_set_boot_partition()`.
- **EEPROM Storage Layout:**
  - `0x0A`: Magic Byte (`0xEA`)
  - `0x0B`: Language (0=EN, 1=FA, 2=TR, 3=RU)
  - `0x0C`: Buzzer Enabled (bool)
  - `0x0D`: Touch Key Feedback (bool)
  - `0x0E..0x11`: 4-digit Admin PIN (char array, default "1234")
  - `0x12`: Output Polarity `vr_OutPutType` (0=N.O, 1=N.C)
  - `0x20..0x3F`: Day lock counter, challenge seed, leasing expiry timestamp

## Machine Behavior and Verification

- **Real Device Commands:** Serial agent (`hmi_agent.py`) supports:
  - `PING` $\to$ Returns `PONG GasFilling_HMI VER=x.y.z`
  - `STAT` $\to$ Returns current screen, timers, touch state, memory
  - `SHOT` $\to$ Captures full 480x320 framebuffer (requires 307.2 KB PSRAM buffer)
  - `SAFE` $\to$ Immediately forces all valves and buzzer OFF
  - `PRESS x,y` / `REL` $\to$ Injects calibrated touch events
  - `PERF` $\to$ Telemetry stream (FPS, CPU load, RAM free, refresh rate)
- **N.O / N.C Polarity Rule:**
  - When $vr\_OutPutType = 0$ (N.O): Active valve writes `0` (LOW), Inactive writes `1` (HIGH).
  - When $vr\_OutPutType = 1$ (N.C): Active valve writes `1` (HIGH), Inactive writes `0` (LOW).
  - Any polarity toggle immediately resets active timers to 0 and forces inactive levels.

## UI Layout & Craftsmanship Rules (Apple HIG & Siemens)

- **Clock & Jalali Date:** In Persian language mode, Gregorian RTC time is dynamically converted to Jalali (Shamsi) date format (`YYYY/MM/DD`). On cold boot without RTC battery, clock automatically seeds from firmware compile timestamp (`__DATE__`, `__TIME__`) to prevent `--:--` placeholders.
- **Main Screen Alignment Grid:**
  - Header: 48px metallic navy bar (`y=0..47`). Clock at `x=30, y=10`, Date at `x=18, y=34`.
  - Station Badges ("شیشه ۲" / "شیشه ۱"): 139x34 px rounded cards at `x=94` (left) and `x=247` (right) centered over input columns.
  - Reset Buttons (🔄): 36x36 px circular buttons at `x=54` and `x=394` (`y=69`) aligned over dimension label columns.
  - Dimension Text Areas: 132x38 px at `x=97` (left) and `x=251` (right) with center-aligned numbers and right-aligned `cm` overlays (`TA_Y0=128`, `TA_DY=42`, shifted +28px down in V1.8.35/V1.8.37).
  - Dimension Side Labels: 53x24 px at `x=42` (left) and `x=385` (right) aligned at `y=135, 177, 219` (`y = 135 + i * 42`).
  - Start/Stop Buttons: Identical 94x44 px rounded capsules at `y=266` (shifted +10px down, leaving 10px bottom margin to y=320) with 10px gap between Start and Stop on both channels (`x=38, 142` on left; `x=244, 348` on right).
  - Progress Stacks: 36x146 px capsules at `UI_BAR_Y = 117` on outer flanks (`x=4` and `x=440`) with 40px target time readouts (`T_TargetValue`) centered above.

## Typography & Zero-Tofu Standard (استاندارد الزامی صفر درصد مربع در فونت)

- **Absolute Prohibition on Persian Numerals in UI Strings:** Embedded Persian fonts (`ui_font_vazir_*`) contain ASCII Latin digits (`0..9`), standard Persian alphabet glyphs, and common symbols, but **do not include Unicode Eastern Arabic / Persian numerals (`۰۱۲۳۴۵۶۷۸۹`, `\u06F0-\u06F9`, `\u0660-\u0669`)**. Passing Persian numerals directly to `lv_label_set_text` or `snprintf` causes LVGL to render missing-glyph rectangular fallback boxes (`[ ]` tofu).
- **Mandatory ASCII Digits:** All numeric values in UI strings (titles, buttons, status texts, day counters, dates, and times) MUST use standard ASCII numerals `0..9` (e.g., `"قفل 5 مرحله ای"`, `"ساعت 24 ساعته"`, `"1405"`, `"30 لیتر"`).
- **Prohibition on ZWNJ (\u200c):** Zero-width non-joiners inside Persian strings cause tofu glyph errors in raster fonts. Use spaces or standard spellings without ZWNJ (e.g. `"سخت افزار"`, `"کار میکند"`).
- **Audit Gate:** Any new UI string must be verified to contain zero `[\u06F0-\u06F9]` and zero `\u200c` characters before release.

