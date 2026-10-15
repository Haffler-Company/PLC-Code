---
name: metr-cutter-agent-handoff
description: >-
  Handoff guide for any agent working on Metr & Cutter: user working style,
  ethics, test gates, project-built tools/extensions, and how to finish work.
  Use when starting a new session, onboarding another agent, or asking how the
  owner expects bugs/UI/motion to be verified and closed.
---

# Metr & Cutter — Agent Handoff

For the next agent: read this **before** large edits. Persian answers for the user; keep this skill bilingual-short.

## Owner Working Style (اخلاق کار)

Active owner rules: `docs/OPERATIONS.md` — read only the relevant part on a new HMI/motion session.

- Speaks **Persian**; wants **short, practical** answers — what broke, what you fixed, what to test on the machine.
- Prefers **shipped fixes** over long analysis: fix → build → upload (when in scope) → verify. Do not leave hang/UI/jog-stop half-done.
- If he repeats the same bug 2–4 times, the fix is still wrong — do not argue; change the root and flash.
- Pixel instructions are literal (direction + edge). Do not invert “from the left / down”.
- Visual feedback must match behavior (e.g. fine-jog speed **and** blue «کم» triangle).
- Values **speed + quality**: real board `SHOT`, not mockup-only claims. Designer HTML is a reference, not proof.
- Dislikes 40-minute full LVGL rebuilds and broken IDE Build/Flash buttons — keep PlatformIO path healthy (`metr-cutter-build-flash`).
- Explicitly asked for disk cleanup, F1 visual match to **his** photos, and status-bar Build+Upload that actually works.
- **Safety-critical machine**: never casual pin/polarity/motion changes; after motion/output edits, list real hardware tests. Releasing jog keys must stop — runaway to end-of-travel is unacceptable.
- **خط وحشتناک قرمز — کیبرد USB:** never touch `Task4.h` / `EspUsbHost/**` / USB plug-edge LED/mouse-reset/list-reflow unless owner explicitly orders a fix named «کیبرد USB». Soft-KB ≠ USB host. See `.cursor/rules/06-usb-keyboard-redline.mdc`.
- Commits only when he asks. Do not force-push or rewrite history.

## Read Order (every new agent)

1. `docs/START.md` → `docs/OPERATIONS.md` → `docs/REFERENCE.md`
2. `.cursor/rules/*.mdc` (especially `metr-cutter.mdc`, motion, LVGL, hardware)
3. `docs/REFERENCE.md` (only the task-relevant section)
4. `docs/WORKLOG.md`
5. Skills: `metr-cutter-hmi`, `metr-cutter-build-flash`, `metr-cutter-hmi-design` (UI polish)
6. Personal: `haffler-esp32-lvgl-hmi` (patterns across Haffler HMIs)

## Definition Of Done

| Change type | Done when |
|-------------|-----------|
| List/paging/trash/motion logic | python tools/sim_check.py green (includes list + motion twin) |
| Interactive UI draft | simulator/hmi_hub.html + optional python tools/hub_shot.py |
| F1 / layout claim | Designer shot **and** real `SHOT` PNG compared (Read the images) |
| Hang / NVS / settings | Flashed if user wants ship; touch-test settings + language + save without freeze |
| Motion / outputs / safety | Document tests: home, jog both ways, stop/E-stop, dry cut if safe |
| Build tooling | `MC: Build` or `pio run` succeeds; upload only with free COM4 |

## Test Conditions (شرایط تست)

**Serial / SHOT**

- Port **COM4**, UART0 **115200**, open with **dtr=False, rts=False** (else board resets).
- After reset/power-cycle wait **~20 s** before `SHOT` / `SCR1|2|3`.
- Commands: `SHOT\n`, `SCR1\n`, `SCR2\n`, `SCR3\n` (see `screenshot.h` + Task2).

**UI**

- Do not call done on layout from memory or HTML alone.
- List parent must stay **non-LVGL-scrollable** with manual paging (`LIST_MAX=50`, 10 rows/page).
- Touch callbacks: flags only — no `loadScreen`, no bulk NVS, no long `vTaskDelay`.

**Motion**

- Outputs **active LOW**; boot safe = HIGH for clamps/saw/spray/lamp/lift.
- ISR tiny: no Serial, no NVS, no alloc.

**Build**

- Project dir = `firmware/`. Upload port COM4 @ 921600.
- If UI unchanged after upload → **power-cycle**, not soft reset alone.

## Project-Built Tools / Extensions (افزونه‌های ساخته‌شده)

Use these instead of reinventing:

| Asset | Role |
|-------|------|
| `.vscode/tasks.json` | Status-bar: **MC: Build**, **Fast Flash**, **Build+Upload**, Monitor, Clean |
| `firmware/fast_flash.bat` | Flash existing `firmware.bin` only (~40s) |
| `firmware/do_build_upload.bat` / `quick_build_flash.bat` | Scripted build±upload with logs |
| `tools/hmi_screenshot.py` | Real LCD → `artifacts/screenshots/*.png` |
| `tools/list_ui_sim.py` | List logic twin + `--test` |
| `tools/designer_f1_shot.py` | 480×320 capture of F1 HTML designer |
| `simulator/settings-f1-designer.html` | Visual F1 playground / user target alignment |
| `libraries/lvgl/library.json` `srcFilter` | Slim LVGL compile (keep OSAL none!) |
| `include/drivers/screenshot.h` | On-device `SHOT` / `SCRn` |
| Cursor rules `00`–`80` + skills under `.cursor/skills/` | Agent defaults |

Shot folder: `artifacts/screenshots/` — name every capture clearly by date and purpose.

Optional mirror without spaces: `C:\fwbuild\` — if used, **sync** source/`library.json` from `D:\metr and cutter1` before building; IDE tasks still point at workspace `firmware/`.

## How To Complete The Product (نسبت به تکمیل پروژه)

1. **Stability first**: no hang on settings/list/trash; deferred NVS/language/save via Task2 flags.
2. **Visual match**: F1 tools column ↔ user photo / designer; E-STOP composite; list header alignment — design loop skill + real SHOT.
3. **Scenario coverage**: use `docs/REFERENCE.md` on hardware after motion/UI shipping.
4. **Backlog**: track in `docs/WORKLOG.md`; bump version only when shipping a coherent drop.
5. **Do not**: upgrade LVGL casually, rewrite EEZ `screens.c` for chrome, change pins, or claim board UI without `SHOT`.

## Anti-Patterns The Owner Already Hit

- Full LVGL rebuild compiling gltf/svg/desktop drivers → hours / killed builds.
- Excluding entire `osal` folder → link fail `lv_os_init`.
- Using `lv_font_montserrat_10` when only 14+ enabled in `lv_conf.h`.
- NVS / `vTaskDelay` inside LVGL events → settings hang.
- Mockup-only “fixed” UI while board still wrong.
- Homing failure flags set in Task3 but never consumed in Task2 → green spinner forever.
- Jog-stop debounce that resets on empty HID report → motor runs to end of travel.
- Fine-jog speed works but badge missing (edge-only UI sync / recreate-every-frame / not on `lv_layer_top`).
- Stepped “triangle” rows when owner asked for a **straight** hypotenuse.

## Point The User

After a fix: one sentence what changed + **exact** hardware checks. Persian, concise.

