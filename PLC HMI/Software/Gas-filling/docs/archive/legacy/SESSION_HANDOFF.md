# Session Handoff — 2026-07-19

Read this first in a new chat, then `docs/AGENT_START_HERE.md` and `docs/GAS_FILLING_TECHNICAL_KNOWLEDGE.md`.

## User request (stop point)

User said stop here and continue later. Work is saved in git + this handoff.

## Do next (priority order)

1. **Flash latest source** with force sketch rebuild (do not use plain `-Clean` unless cache corrupt):
   ```powershell
   cd "c:\Users\HP\Documents\GitHub\PLC\PLC HMI\Software\Gas-filling"
   .\scripts\doctor.ps1
   .\scripts\program.ps1 -Port COM3 -Force -Knowledge "touch fix flash: WIFI_OFF first, no Wire.setClock after bbct, logo hidden at create, no imgbtn zoom, TOUCHDIAG"
   ```
2. After boot, check serial line `TOUCH_INIT sensor=... ok=...` and:
   ```powershell
   python .\tools\hmi_agent.py --port COM3 --settle 2 cmd STAT
   python .\tools\hmi_agent.py --port COM3 --settle 2 cmd TOUCH
   ```
   Ask user to touch the LCD, then run `TOUCH` again — `got=1` / `hits` should rise.
3. If physical touch still dead: diagnose I2C (`sensor=0` = chip not detected). Do **not** re-enable WiFi/NTP in `setup`.
4. Sync Tehran clock: `TIMESET YYYY MM DD HH MM SS` (local IRST wall clock from PC).
5. Screenshot Main + Setting; verify UI polish on real LCD.

## Firmware / version

| Item | Status |
|------|--------|
| `VERSION` in tree | **1.2.16** |
| On device last seen in chat | **v1.2.13** (PING) — later bumps may not be fully verified on hardware |
| Touch fix code in tree | Partially applied; flash with `-Force` still required |
| Build infra | Hardened (`doctor.ps1`, `-Force`/`-CleanSketch`, upload retries, fingerprint) |

## Touch fixes already in source (must flash)

- `WiFi.mode(WIFI_OFF)` **before** touch/I2C init (not after).
- Removed `Wire.setClock(400000)` after `bbct.init` (can leave FT6336 dead).
- `hafler_panle` created `HIDDEN` + not clickable; `HideLogo()` still at end of setup.
- Removed `transform_zoom` on settings/back imgbtn (LVGL9 hang after press/TAP).
- Agent: `TOUCH` / `TOUCHDIAG`, richer `STAT` (`touch_ok`, `sensor`, `last`, `hits`).
- Orientation unchanged: `setOrientation(270, 320, 480)` (same as Bakery).

## CRITICAL — do not break touch again

- Never `WiFi.begin()` / blocking NTP in `setup()`.
- Never `Wire.end()` after `bbct.init(...)`.
- Prefer `-Force` over `-Clean` for rebuilds.
- Agent: `REL` after virtual touch; settle ≥2s for CDC PING.
- One TinyTTF Vazirmatn face only; small caches in `lv_conf.h`.

## UI already in source (verify on LCD after flash)

- Blue header `#2196F3` (settings accent), Vazirmatn, cm labels طول/عرض/ارتفاع, Start green / Stop red
- Bars white→red to 100%, `%` inside bar
- Tehran TZ `IRST-3:30` + `TIMESET`

## Build infra (done)

- `scripts/common.ps1`, `build.ps1`, `upload.ps1`, `program.ps1`, `rollback.ps1`, `doctor.ps1`
- Keep `artifacts/build-cache/core`; use `-Force` for sketch-only rebuild
- Upload retries ×3; COM port array unwrap fixed (`COM3` not `C`)
