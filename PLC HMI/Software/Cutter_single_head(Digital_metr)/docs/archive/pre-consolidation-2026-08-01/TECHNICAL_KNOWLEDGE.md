# Technical Knowledge

This is the current operational summary. Historical investigations and superseded details belong in `docs/archive/` and are not start-up reading.

## Platform and safety baseline

- ESP32-S3 / Arduino / PlatformIO / LVGL 9.4; display is 480x320 landscape.
- Firmware source is `firmware/`; version truth is `VERSION` and `firmware/include/version.h`.
- Build only through `tools/fast_firmware.ps1` from the current tree. Never clean routinely or mix the long workspace and `X:` paths during a build cycle.
- Upload only after explicit approval; application-only upload preserves NVS by default.
- Pins, polarity, homing, E-stop, guard, cutting, jog release, destination, and backlash are safety-sensitive. Change none without a named owner request and hardware verification.
- `Task4.h` and `libraries/EspUsbHost/**` are an absolute USB-keyboard red line without an explicit “USB keyboard / کیبرد USB” request.

## UI and LVGL

- Render/UI signals belong in `Task2`; callbacks should set flags or `GUI.pending_goto_scr`, never directly load a screen.
- Do not create LVGL object trees, timers, or full-screen invalidations for small visual changes.
- Use existing objects and static styles. Keep list rows bounded/virtualized.
- Real LCD `SHOT` is visual proof. HTML/PC simulators are only design or logic aids.
- Persian UI text must stay UTF-8 and use a project Persian font. Do not mix Latin text, ASCII digits, or unsupported punctuation into a Persian-font label.
- Dialogs and help overlays must block background touches; their close/header controls must remain outside scrollable content.

## Display and boot

- The accepted display transport is TFT_eSPI-owned FSPI/SPI2 at 40 MHz with manual CS and guarded DMA fallback. The rejected 80 MHz hardware-CS path froze the physical panel.
- Keep partial/per-digit redraw; do not regress measured FPS or move UI work into the stepper ISR.
- Heavy WebPortal and SerialImport work is deferred after boot. A direct `loadScreen()` from a callback can stall the first LVGL handler pass.

## State and persistence

- Avoid bulk Preferences/NVS writes inside LVGL events; queue/chunk large cleanup work in `Task2`.
- Preserve `GUI.lock_screen` correctness on every modal close path.
- List page changes must refresh visible labels. Keep list and optimization limits bounded.

## Evidence and diagnostics

- `SHOT` dumps the real framebuffer; open the serial port with DTR/RTS disabled. `SCR1`, `SCR2`, and `SCR3` must only schedule deferred navigation.
- `PERF` and `LCDB` are diagnostics, not UI features. Store only concise conclusions in active docs; generated logs and captures are disposable artifacts.
- For exact commands, use `docs/SKILLS_AND_TOOLS.md`; for hardware behavior, use `docs/SCENARIO_FA.md`.

## Known hardware validation still needed

- Verify the accepted 40 MHz display path during real motor-motion stress after a relevant approved upload.
- Hardware-verify five-key navigation coverage after changes to navigation, modal, or keyboard behavior.
