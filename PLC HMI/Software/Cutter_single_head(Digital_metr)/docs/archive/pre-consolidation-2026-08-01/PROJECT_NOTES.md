# ESP32-S3 Metr & Cutter Project Notes

## Purpose

Firmware for the HAFFLER Metr & Cutter: an ESP32-S3 industrial stepper cutting machine with a 480x320 touch HMI, list-based cutting, settings, web portal, serial import, printer support, and safety inputs.

**Owner working style / agent expectations:** `docs/OWNER_PROFILE_FA.md` (read only when the task is substantial or behavior-sensitive).

## Build Target

| Item | Value |
|---|---|
| PlatformIO project | `firmware/` |
| Environment | `esp32-s3-devkitc-1` |
| Framework | Arduino |
| MCU | ESP32-S3 |
| Flash | 8 MB |
| PSRAM flag | `BOARD_HAS_PSRAM` |
| Upload port | COM4 |
| Upload speed | 921600 |
| Monitor speed | 115200 |
| Current version | see `VERSION` / `firmware/include/version.h` (path 1.5.10x; owner profile lessons through ~1.5.109) |

## Libraries

| Library | Path | Purpose |
|---|---|---|
| LVGL 9.4.0 | `libraries/lvgl` | HMI UI |
| TFT_eSPI | `libraries/TFT_eSPI` | TFT display driver |
| bb_captouch | `libraries/bb_captouch` | Capacitive touch |
| EspUsbHost | `libraries/EspUsbHost` | USB keyboard/input support |

## Display And Touch

| Function | Value |
|---|---|
| Display size | 480x320 |
| TFT driver | `ILI9341_DRIVER` in `libraries/TFT_eSPI/User_Setup.h` |
| TFT color order | `TFT_BGR` |
| TFT SPI frequency | 40 MHz stability ceiling |
| TFT MISO | GPIO11 |
| TFT MOSI | GPIO10 |
| TFT SCLK | GPIO12 |
| TFT CS | GPIO13 |
| TFT DC | GPIO14 |
| TFT RST | -1 |
| Runtime SPI begin | TFT_eSPI-owned explicit `USE_FSPI_PORT` FSPI/SPI2 |
| Rotation | `tft.setRotation(3)` |
| Touch library | `bb_captouch` |
| Touch SDA | GPIO8 |
| Touch SCL | GPIO9 |
| Touch RST | -1 |
| Touch INT | -1 |
| Touch orientation | 270 |
| Backlight/control observed | GPIO45 set OUTPUT LOW at setup |

## Machine Output Pins

Outputs are active LOW. HIGH is the safe/off state for most machine outputs.

| Function | GPIO | Notes |
|---|---:|---|
| Stepper PUL | 16 | Pulse output |
| Stepper DIR | 42 | Direction output |
| Stepper EN | 39 | LOW enables driver, HIGH disables |
| Upper clamp | 48 | Active LOW |
| Front clamp | 40 | Active LOW |
| Spray | 15 | Active LOW |
| Saw | 35 | Active LOW |
| Lamp | 36 | Active LOW |
| Saw lift | 38 | Active LOW |
| Q9 | 41 | Active LOW spare/output |

## Inputs

| Function | GPIO | Source |
|---|---:|---|
| Reference sensor ADC | 6 | `Task6.h` / docs |
| Joystick ADC | 4 | `Task7.h` / docs |
| E-stop + guard ADC | 5 | `Task8.h` / docs |
| Start1 | 3 | `Task8.h` / docs |
| Start2 | 7 | `Task8.h` / docs |
| F-keys | 1 | `Task9.h` / docs |
| Save/cancel panel | 2 | `Task10.h` / docs |

## Motion Facts

| Item | Value |
|---|---|
| Default PPR | 6400 |
| Axis pitch | 20 mm |
| Default stock length | 6000 mm |
| Kerf | 3.0 mm, used for optimization packing |
| Trim | 0 mm |
| Backlash compensation | 0.5 mm |
| Homing fast interval | 30 us |
| Homing creep interval | 200 us |
| Homing timeout | 30 s per stage |
| Motion timer | 1 us hardware timer |

## FreeRTOS / Task Map

| Task | Core | Purpose |
|---|---:|---|
| Task1 | 0 | LVGL tick using `millis()` delta |
| Task2 | 0 | LVGL render loop, `lv_timer_handler`, UI signal processing |
| Task3 | 1 | Stepper motion, homing, timer ISR |
| Task4 | 1 | Touch/input related task |
| Task5 | 0 | Buzzer/input service |
| Task6 | 1 | Reference/limit input ADC |
| Task7 | 1 | Joystick ADC |
| Task8 | 1 | Cutting sequence, safety, start buttons |
| Task9 | 0 | F-keys |
| Task10 | 0 | Menu/save/cancel panel |
| Task11 | 1 | Additional machine service |
| WebPortal | 0 | Deferred web portal |
| SerialImport | 0 | Deferred serial import |
| PrinterTask | 0 | Printer service |

## LVGL Rules Learned From Bugs

- Do not call `loadScreen()` from touch callbacks. Use `GUI.pending_goto_scr`.
- `Task2` must be created immediately after `init_widgets()` before heavy splash/extras.
- Heavy services like WebPortal and SerialImport are intentionally deferred after boot.
- Do not bulk-write Preferences/NVS inside LVGL events.
- Chunk NVS/list cleanup inside `Task2`.
- `GUI.lock_screen` must be cleared on all dialog close paths.
- Fade transitions caused screen bleed; use immediate screen load.
- List screen uses `LIST_MAX=100`, `LIST_ROWS_PER_PAGE=5`, and `LIST_PAGE_COUNT=20`.
- Page changes must force label refresh.

## Main Files

| Path | Role |
|---|---|
| `firmware/src/main.ino` | Startup, globals, structs, task creation, display/touch init |
| `firmware/include/app/GUI_Func.h` | UI callbacks, display flush, touch, settings/list handlers |
| `firmware/include/app/ui_extras.h` | Splash, toast, keypad, list/optimization helpers |
| `firmware/include/app/Pref_Func.h` | Preferences/NVS |
| `firmware/include/tasks/Task2.h` | LVGL render + UI signal loop |
| `firmware/include/tasks/Task3.h` | Stepper ISR/motion/reference |
| `firmware/include/tasks/Task8.h` | Cut flow and safety |
| `firmware/include/drivers/webportal.h` | WiFi web settings/OTA/import |
| `firmware/include/drivers/serial_import.h` | Serial size-list import |
| `firmware/include/drivers/screenshot.h` | Real-device screenshot |

## Build Commands

```powershell
powershell -ExecutionPolicy Bypass -File tools/fast_firmware.ps1
powershell -ExecutionPolicy Bypass -File tools/fast_firmware.ps1 -Upload
```

```powershell
python "D:\metr and cutter1\tools\list_ui_sim.py" --test
python "D:\metr and cutter1\tools\hmi_screenshot.py" --port COM4
```

## Cursor Instructions

When working in Cursor, say:

```text
First read `docs/PROJECT_NOTES.md` (via `docs/INDEX_FA.md`) and `.cursor/rules`.
This is ESP32-S3 Metr & Cutter firmware.
Do not change pins, output polarity, LVGL version, or TFT_eSPI setup unless I ask.
For UI changes, use Task2 flags and pending_goto_scr.
For motion changes, explain real hardware tests before upload.
Build from firmware/platformio.ini.
```

## Docs Pointers

- Active technical knowledge: `docs/TECHNICAL_KNOWLEDGE.md`
- Changelog / backlog (FA): `docs/CHANGELOG_AND_BACKLOG_FA.md`
- Apple-inspired HMI design: `docs/HMI_DESIGN_SYSTEM_APPLE_FA.md`
- Agent design loop image: `docs/assets/agent-design-loop-fa.png`
- Behavior scenarios: `docs/SCENARIO_FA.md`
- Simulator ladder: `docs/SIMULATOR_FA.md`

## 2026-07-13 main screen hang

- Confirm-to-main hang was reproduced with a temporary `REF1` serial command.
- `loadScreen(SCREEN_ID_MAIN_1)` returned; the stall was in the first `lv_timer_handler()` after loading main screen.
- Hardware root cause: EEZ `objects.obj1` full-screen container in `main_1`. Hiding children did not help; hiding `obj1` did.
- Fix: `ensure_safe_main1()` hides `obj1` and builds a lightweight runtime main screen directly on `objects.main_1`.
- USB keyboard fix: Task4 no longer touches LVGL/commit functions directly; Task2 consumes Enter via `keyboard_pending_enter`.
- Temporary `REF1` bypass command was removed after testing.

## 2026-07-15 LCD DMA speed fix

- Raw `LCDB` hardware test proved the old blocking TFT path was capped around 23 FPS full-screen.
- Guarded TFT_eSPI DMA reached about 42 FPS full-screen on the same board.
- The uploaded firmware now enables TFT DMA and uses it for LVGL flush chunks that fit under the 64 KB DMA limit.

## 2026-07-19 physical LCD freeze correction

- The 80 MHz HSPI/hardware-CS image is rejected: UART and SHOT remained alive while the physical panel froze.
- TFT_eSPI is now the only SPI owner, using default FSPI/SPI2 at 40 MHz.
- LVGL keeps manual CS low across address commands and pixel DMA.
- The active DMA queue call is zero-wait; failure falls back to blocking transfer.
- The corrected register-host mapping was built and uploaded; clean boot, DMA transfers, `QSTA buz=0`, and physical LCD progress were confirmed.
- Current 40 MHz hardware baseline: blocking full-screen `13.71 FPS`, queued DMA `15.62 FPS`, and partial 120x48 `332.4 FPS`.
- Real motor-motion stress remains pending operator validation.
