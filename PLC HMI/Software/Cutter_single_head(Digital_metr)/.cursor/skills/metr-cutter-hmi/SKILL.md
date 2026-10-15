---
name: metr-cutter-hmi
description: >-
  ESP32-S3 / LVGL 9.4 / FreeRTOS Metr & Cutter HMI. Use for list paging chaos,
  trash hang, settings/web/opt, black boot, PlatformIO COM4 upload, motion
  safety, or when editing GUI_Func, ui_extras, Task2, Task3, Task8, Pref_Func,
  or main.ino in metr and cutter1. For build failures / flash / LVGL srcFilter
  also load metr-cutter-build-flash; for onboarding another agent load
  metr-cutter-agent-handoff.
---

# Metr & Cutter HMI

## Read Order

1. `docs/START.md` → `docs/OPERATIONS.md` → `docs/REFERENCE.md`
2. `.cursor/rules/*.mdc` (شامل `metr-cutter.mdc`, `60-hmi-design-apple`, `70-agent-design-loop`)
3. `docs/REFERENCE.md` (only the relevant section)
4. `.cursor/skills/metr-cutter-agent-handoff/SKILL.md` (اخلاق/تست/Done)
5. `docs/WORKLOG.md` برای کار UI
6. `docs/REFERENCE.md` برای سناریو و شبیه‌ساز
8. Grep before reading huge generated files like `firmware/src/ui/screens.c`
9. Visual polish → also load `.cursor/skills/metr-cutter-hmi-design/SKILL.md`
10. Build/flash failures → `.cursor/skills/metr-cutter-build-flash/SKILL.md`

## User Style

- Active owner rules: `docs/OPERATIONS.md`.
- The user writes Persian. Answer in Persian, short and practical.
- For clear bugs, fix -> build -> upload if requested -> verify.
- Do not leave half-done hang / jog-stop / missing-indicator fixes.
- If the same bug is repeated, the previous fix failed — change the root.
- Pixel directions are literal; do not invert.
- Explain real hardware tests after any motion/output/safety change.

## Non-Negotiable Project Facts

- **خط وحشتناک قرمز کیبرد USB:** do not touch `Task4.h` / `EspUsbHost` / USB plug-edge side effects unless owner explicitly names «کیبرد USB». Rule: `.cursor/rules/06-usb-keyboard-redline.mdc`.
- PlatformIO project is `firmware/`, not the repository root.
- Board env: `esp32-s3-devkitc-1`.
- Framework: Arduino.
- LVGL version: 9.4.0. Do not casual-upgrade.
- Display: 480x320 TFT through TFT_eSPI.
- Touch: bb_captouch on SDA GPIO8 / SCL GPIO9.
- Outputs are active LOW. HIGH is safe/off for the machine outputs.
- Upload port in current config: COM4.
- Version must stay synchronized in `VERSION` and `firmware/include/version.h`.

## Exact Screenshot From The Real Device

Device COM4 (UART0, 115200) supports remote debug (`screenshot.h`, Task2):

- `SHOT\n` dumps the real LVGL framebuffer → `tools/hmi_screenshot.py` → `artifacts/screenshots/*.png`
- `SCR1\n` / `SCR2\n` / `SCR3\n` → main / settings / list via `pending_goto_scr`

```powershell
python "D:\metr and cutter1\tools\hmi_screenshot.py" --port COM4
python "D:\metr and cutter1\tools\designer_f1_shot.py"   # F1 HTML reference 480x320
```

Open serial with `dtr=False, rts=False` or the board resets. After reset wait ~20 s.
ALWAYS verify UI layout with a real `SHOT` when hardware is connected.
For simulator tools, read only the matching part of `docs/REFERENCE.md`.

## Simulate Before Flash

```powershell
python "D:\metr and cutter1\tools\list_ui_sim.py" --test
python "D:\metr and cutter1\tools\list_ui_sim.py"
# F1 visual designer: simulator/settings-f1-designer.html
```

Canvas/logic twin is 480x320. F1 chrome → `ui_extras.h`; compare designer shot vs board `SHOT` before calling layout done.
See also `docs/REFERENCE.md`.

## Build / Upload

See `.cursor/skills/metr-cutter-build-flash/SKILL.md` for failures, OSAL, fonts, COM4 hangs.

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "D:\metr and cutter1\firmware"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM4 --project-dir "D:\metr and cutter1\firmware"
```

IDE: `.vscode/tasks.json` → `MC: Build` / `MC: Fast Flash` / `MC: Build + Upload`. Fast path: `firmware/fast_flash.bat`.

## Known Failure Modes And Fix Patterns

| Symptom | Likely Cause | Fix Pattern |
|---|---|---|
| Black screen after boot | Task2 failed, heap fragmentation, heavy tasks too early | Keep Task2 creation immediately after `init_widgets`; defer WebPortal/SerialImport |
| UI hang after touch | `loadScreen` or NVS in event callback | Set flags, process in Task2 |
| Lock screen feels stuck | Dialog close path did not clear `GUI.lock_screen` | Clear lock on every close/cancel/success path |
| Rows blink or vanish | Native LVGL scroll mixed with manual paging | Keep list parent non-scrollable and page-aware |
| Wrong list numbers after page | Labels not refreshed on `list_page` change | Force label flags on page change |
| Trash/list clear hangs | Bulk Preferences writes | Chunk NVS in Task2 |
| Settings / language hang | NVS or mass label hide in LVGL event | `flag_apply_language` / deferred saves in Task2 |
| Display corruption | Wrong driver/pins/rotation/SPI assumptions | Check `User_Setup.h`, `SPI.begin`, rotation, buffer allocation |
| Touch offset/inversion | Orientation or width/height mismatch | Check `bbct.setOrientation(270, SCREEN_HEIGHT, SCREEN_WIDTH)` |

## Never In Touch Callback

| Never | Instead |
|---|---|
| `loadScreen()` / `lv_screen_load()` | `request_goto_screen()` -> Task2 |
| Bulk `Preferences` writes | Flag + chunked Task2 writes |
| Heavy list rebuilds | Flag + bounded updates |
| Long loops or delays | Fast flag setting |

## Motion Safety

- Stepper motion and homing live in `Task3.h`.
- Cutting and machine sequence safety live in `Task8.h`.
- Keep ISR code small: no logs, allocation, NVS, or heavy math.
- Preserve stop checks: sensors, stop button, `STP.mvm_interupt`.
- Preserve wrong-direction and limit recalibration flags.
- Safe output boot state is HIGH for active-LOW outputs.

## File Map

- `firmware/src/main.ino`: startup, global state, task creation, display/touch init.
- `firmware/include/app/GUI_Func.h`: LVGL callbacks, touch/display setup, settings/list handlers.
- `firmware/include/app/ui_extras.h`: splash, toast, numpad, extra UI behavior.
- `firmware/include/app/Pref_Func.h`: NVS settings.
- `firmware/include/tasks/Task1.h`: LVGL tick.
- `firmware/include/tasks/Task2.h`: LVGL render loop and UI signal processing.
- `firmware/include/tasks/Task3.h`: stepper ISR/motion/reference.
- `firmware/include/tasks/Task8.h`: cut flow, safety, start buttons.
- `firmware/include/drivers/webportal.h`: WiFi portal, OTA, web settings.
- `firmware/include/drivers/serial_import.h`: serial import.
- `firmware/include/drivers/screenshot.h`: real-device screenshot.

## Constants

- `LIST_MAX = 50`
- `LIST_ROWS_PER_PAGE = 10`
- `LIST_PAGE_COUNT = 5`
- Default PPR: 6400
- Axis pitch: 20 mm
- Default stock length: 6000 mm
- Kerf: 3.0 mm, packing only
- Web AP: `ESP32-UPLOAD` / `12345678`
- Default web PIN: `1234`
