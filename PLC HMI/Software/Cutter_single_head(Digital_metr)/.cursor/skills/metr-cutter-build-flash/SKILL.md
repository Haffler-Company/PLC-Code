---
name: metr-cutter-build-flash
description: >-
  PlatformIO build/flash for Metr & Cutter ESP32-S3 on COM4. Use when Build fails,
  upload hangs, LVGL rebuild is slow, libdeps/srcFilter/OSAL linker errors,
  montserrat font undeclared, IDE task buttons, or C:\fwbuild mirror builds.
---

# Metr & Cutter — Build / Flash

## Paths

| What | Path |
|------|------|
| PlatformIO project | `firmware/` (never repo root alone) |
| Local libs | `libraries/{lvgl,TFT_eSPI,bb_captouch,EspUsbHost}` |
| LVGL conf | `firmware/config/lv_conf.h` |
| Version | `VERSION` + `firmware/include/version.h` (keep in sync) |
| IDE tasks | `.vscode/tasks.json` → `MC: Build`, `MC: Fast Flash`, `MC: Build + Upload` |
| Optional short path | `C:\fwbuild\` (mirror without spaces; sync from `D:\metr and cutter1`) |

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "D:\metr and cutter1\firmware"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM4 --project-dir "D:\metr and cutter1\firmware"
# flash-only (~40s) after bin exists:
firmware\fast_flash.bat
```

COM4 = CP210x (یا هر پورت CP210x که `fast_firmware.ps1` خودکار پیدا کند). Upload baud 921600. Monitor/SHOT/boot_verify baud **115200** (`SERIAL_DEBUG_BAUD`) with `dtr=False, rts=False`.

روزمره: `tools/fast_firmware.ps1 -Upload` — boot verify بعد از فلش. هنگ بوت: `-FullFlash`. OTA: `artifacts/builds/firmware_latest.bin`.


## After Disk Cleanup / Slow Full Rebuild

1. Prefer `lib_deps = file://../libraries/...` once LVGL is slim (~tens of MB). Huge `file://` copies of full LVGL (~600MB) hurt disk.
2. Speed comes from `libraries/lvgl/library.json` → `build.srcFilter` exclusions (drivers, gltf, gif, svg, unused fonts…).
3. **Never exclude all of `osal/`.** With `LV_USE_OS LV_OS_NONE`, keep `lv_os.c` + `lv_os_none.c`. Excluding OSAL → linker: `undefined reference to lv_os_init` / `lv_lock` / `lv_unlock`.
4. After editing `library.json`, refresh libdeps copy or delete `.pio/libdeps/.../lvgl` so PlatformIO reinstalls.
5. Stale `.pio/build` after kill → `pio run -t clean` or delete `.pio/build`; missing `.o` (e.g. gif) → linker `cannot find ...gif.c.o`.

## Compile Errors Seen In Field

| Error | Fix |
|-------|-----|
| `lv_font_montserrat_10` not declared | Enable in `lv_conf.h` **or** use an enabled font (`montserrat_14`+). Do not assume all Montserrat sizes exist. |
| Flood of `lv_part_t \| lv_state_t` warnings | Normal LVGL + GCC14; not bugs | `-Wno-deprecated-enum-enum-conversion` in `platformio.ini` + `.clangd` |
| Stale `montserrat_10` in Problems panel | Old diagnostic cache | Code uses `montserrat_14`; restart clangd / Reload Window |
| `main.ino.cpp.o` Error 1 with no error in terminal | Scroll truncated / process killed — rebuild and filter `error:` / `undefined reference`. |
| Windows Long Path warning | Prefer short mirror `C:\fwbuild` or enable Win32 long paths. |
| Upload hangs on "Connecting" | Kill hung `esptool`/`pio`; close Monitor; free COM4; retry `fast_flash.bat`. |

## Agent Rules For Flash

- User Persian: confirm upload when ambiguous; do not leave hang fixes unflashed if user asked to ship.
- After flash, if UI unchanged → **power-cycle** board, wait ~20 s, then `SHOT`.
- Do not change pins, LVGL version, or TFT_eSPI `User_Setup.h` unless user explicitly asks.

## Related

- Active operational summary: `docs/OPERATIONS.md` and `docs/REFERENCE.md`
- Handoff / test ethics: `.cursor/skills/metr-cutter-agent-handoff/SKILL.md`
- Core HMI skill: `.cursor/skills/metr-cutter-hmi/SKILL.md`
