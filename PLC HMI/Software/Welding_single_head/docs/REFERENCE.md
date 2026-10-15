# Technical Reference — Welding Single Head HMI/PLC

## 1. Platform Specification

| Item | Value |
|---|---|
| Controller/HMI | ESP32-S3 (DevKitC-1), dual-core 240 MHz, Arduino ESP32 core |
| Memory | 16 MB Flash (FlashSize=16M), 8 MB embedded PSRAM (PSRAM=enabled) |
| Partition Scheme | partitions.csv (app0 / app1 OTA scheme ~5.3 MB each, NVS @ 0x9000) |
| Display | 480x320 ST7796/ILI9341 TFT, SPI 40 MHz (TFT_BGR, rotation 3) |
| Touch | Capacitive touch over I2C (b_captouch / GT911 @ 400 kHz) |
| Primary Upload Port | COM8 / COM17 / COM4 (detected Silicon Labs CP210x @ 921600 / 460800 / 115200 baud). NEVER use COM3 (Intel AMT trap). |
| UI Framework | LVGL 9.4.0 + EEZ Studio / Flow Engine |
| Screenshot Engine | Direct RGB565LE accumulation in PSRAM via `shot_on_flush` (<3s transmission @ 921600 baud, 0% UI lag) |
| Security Protocol | HMAC-SHA256 authenticated frame protocol (`$WELD`) with anti-replay protection |

## 2. Hardware Pin Mapping

### Outputs (Active LOW: ON = LOW, OFF = HIGH)
- SSR_1 (GPIO15): Solid State Relay for Heating Plate (المنت حرارتی)
- TABLE (GPIO16): Moving Table Jack (جک صفحه / میز Q0)
- PRESS_LEFT (GPIO42): Left Profile Clamp Jack (جک پرس 1 / چپ Q1)
- PRESS_RIGHT (GPIO39): Right Profile Clamp Jack (جک پرس 2 / راست Q2)
- ELEMENT (GPIO48): Heating Element Movement Jack (جک پیشروی المنت Q3)
- SHABLON (GPIO40): Zero Shablon Positioning Jack (جک شابلون صفر Q4)
- ZARBEH1 (GPIO35): Knive / Striker Jack 1 (ضربه 1 Q6)
- ZARBEH2 (GPIO36): Knive / Striker Jack 2 (ضربه 2 Q7)
- BUZZER (GPIO45): Piezo Buzzer (LEDC PWM @ 2.7-4 kHz)
- SPARE_Q8 (GPIO38) / SPARE_Q9 (GPIO41): Unassigned auxiliary terminals

### Inputs & Sensors
- DEFAULT_SENSOR_ELEMENT (GPIO6): Analog band (I0 = Element Retracted/Down, I1 = Element Extended/Up)
- DEFAULT_SENSOR_SHABLON (GPIO5): Analog band (I2 = Shablon Retracted/Down, I3 = Shablon Extended/Up)
- DEFAULT_SENSOR_TABLE (GPIO4): Analog band (I4 = Table Retracted/Down, I5 = Table Forward/Up)
- EMERGENCY (GPIO3): E-Stop Input (I6)
- PEDAL (GPIO7): Foot Pedal Start Input (I7)
- THERMO_1_CS (GPIO21) / THERMO_2_CS (GPIO0): MAX6675 Chip Select
- THERMO_SCK (GPIO47) / THERMO_MISO (GPIO37): Thermocouple SPI bus
- F-Keys Ladder (GPIO1, GPIO2): Analog resistor ladder for physical front panel buttons (F1..F10)

### Analog Sensor Voltage Bands
- ~2000 (1850-2240): Transition / Deadband (باند مرده جهت جلوگیری از خطای گذرای خازنی)
- ~1550 (1300-1800): Retracted Sensor Active (سنسور باز فعال)
- >3500 (~4095): Extended Sensor Active (سنسور بسته فعال)
- ~2500 (2350-2750): Both Active (فقط برای میز + برگشت ممکن است)

## 3. Firmware Architecture Map

| File | Responsibility |
|---|---|
| src/main.cpp | System setup, FreeRTOS tasks spawn, Core 0/1 setup, main loop |
| src/screens.c / include/screens.h | UI screens (Main, Welding Cycle, Temp Gauge, Admin, IO Remap, Daily Stats) |
| include/tasks.h | Thermal control loop, Jack sequences, Welding Cycle state machine, E-Stop watchdog |
| src/display_port.cpp | LVGL display driver (my_disp_flush), 32-line DMA buffers |
| include/storage.h / include/setting.h | NVS storage, user recipes, machine calibration, PIN codes |
| include/actions.h | UI action dispatchers, button callbacks, mode switches |
| include/agent_hmi.h | Serial automated testing, HMAC-SHA256 protocol, remote navigation & control |
| include/screenshot.h | Direct RGB565 PSRAM framebuffer accumulator & Core 0 async Base64 streamer |
| docs/SERIAL_SECURITY_PROTOCOL.md | Comprehensive cryptographic frame and command dictionary specifications |
| tools/hmi_commander.py | Python client for HMAC-SHA256 signed navigation, control and screenshots |
| tools/haffler_voice_reporter.py | Microsoft Edge Neural Persian TTS audio report generator |
| include/defines.h | Hardware constants, pin definitions, version macro (VERSION) |

## 4. Official OTA Release Matrix (All 9 Machines)

Base URL: `https://ota.haffler-infi.ir`

| # | Machine / Program | Slug | lastfile URL | firmware.bin URL |
|---|---|---|---|---|
| 1 | جوش تک‌سر (Single-Head Welder) | `upvc/welding-single-head` | `https://ota.haffler-infi.ir/upvc/welding-single-head/lastfile` | `https://ota.haffler-infi.ir/upvc/welding-single-head/firmware.bin` |
| 2 | جوش دوسر (Double-Head Welder) | `upvc/welding-double-head` | `https://ota.haffler-infi.ir/upvc/welding-double-head/lastfile` | `https://ota.haffler-infi.ir/upvc/welding-double-head/firmware.bin` |
| 3 | برش تک‌سر دیجیتال (Single-Head Digital Cutter) | `upvc/digital-cutter-single-head` | `https://ota.haffler-infi.ir/upvc/digital-cutter-single-head/lastfile` | `https://ota.haffler-infi.ir/upvc/digital-cutter-single-head/firmware.bin` |
| 4 | برش دوسر دیجیتال (Double-Head Digital Cutter) | `upvc/digital-cutter-double-head` | `https://ota.haffler-infi.ir/upvc/digital-cutter-double-head/lastfile` | `https://ota.haffler-infi.ir/upvc/digital-cutter-double-head/firmware.bin` |
| 5 | تزریق گاز شیشه (Gas Filling) | `upvc/gas-filling` | `https://ota.haffler-infi.ir/upvc/gas-filling/lastfile` | `https://ota.haffler-infi.ir/upvc/gas-filling/firmware.bin` |
| 6 | گوشه‌تمیزکن (Corner Cleaning) | `upvc/corner-cleaning` | `https://ota.haffler-infi.ir/upvc/corner-cleaning/lastfile` | `https://ota.haffler-infi.ir/upvc/corner-cleaning/firmware.bin` |
| 7 | کپی فرز (Copy Router) | `upvc/copy-router` | `https://ota.haffler-infi.ir/upvc/copy-router/lastfile` | `https://ota.haffler-infi.ir/upvc/copy-router/firmware.bin` |
| 8 | پانچ آلومینیوم (Aluminum Punching) | `upvc/aluminum-punching` | `https://ota.haffler-infi.ir/upvc/aluminum-punching/lastfile` | `https://ota.haffler-infi.ir/upvc/aluminum-punching/firmware.bin` |
| 9 | برنامه کانفیگ و تستر (Config & Test) | `upvc/config-test` | `https://ota.haffler-infi.ir/upvc/config-test/lastfile` | `https://ota.haffler-infi.ir/upvc/config-test/firmware.bin` |

## 5. FreeRTOS Task Stacks & WiFi Infrastructure Standards

### FreeRTOS Stack Allocation on ESP32-S3
- **Unit of Stack:** The parameter `usStackDepth` in `xTaskCreatePinnedToCore` is in **4-byte Words**, NOT bytes. A value of 8192 words equals 32,768 bytes; 12288 words equals 49,152 bytes.
- **Internal DRAM Fragmentation:** The ESP32-S3 internal SRAM pool is 320 KB total. When static BSS + data (~171 KB) and LVGL double DMA line buffers (~40 KB) are active, contiguous blocks in DRAM rarely exceed 35 KB.
- **Task Stack Limits (Industrial Red Line):**
  - `WifiConn` (`wifi_connect_worker_task`): 3072 words (12 KB), fallback to 2048 words (8 KB).
  - `WifiScanStart` (`wifi_scan_starter_task`): 3072 words (12 KB), fallback to 2048 words (8 KB).
  - `SelfUpd` (`self_update_task`): 6144 words (24 KB), fallback to 5120 words (20 KB), fallback to 4096 words (16 KB).
  - `WebApp` (`webapp_task`): 8192 words (32 KB) — frees 16 KB DRAM compared to 12288.
- **Radio Resource Preemption:**
  - Before spawning scan or connection tasks, always preempt SoftAP via `webAppEnabled = false; if (webApp.isApUp()) webApp.wStopAp(); WiFi.scanDelete();` to immediately free ~18 KB of internal WiFi rx/tx heap buffers.
- **WiFi Connect Handshake Protocol:**
  - In `WiFi.begin(ssid, pass, 0, NULL, false)`, specify `connect=false` to configure netif without premature association. This permits `esp_wifi_set_config` to succeed (`ESP_OK`) without triggering `ESP_ERR_WIFI_STATE` (0x3006).
- **SPIFFS Filesystem Image (`0xE20000`):**
  - All web app files (`index.html.gz`, `jvs.js.gz`, `imgs.js.gz`) reside in the 2MB SPIFFS partition at `0xE20000`. Full production uploads must flash `bootloader.bin` (`0x0`), `partitions.bin` (`0x8000`), `firmware.bin` (`0x10000`), and `spiffs.bin` (`0xE20000`).

