# Metr & Cutter — Stepper Cutter Firmware

Industrial stepper-based cutting machine firmware for **ESP32-S3**, with LVGL touch UI.

**Current version:** V.1.4.0 | **LVGL:** 9.4.0 | **Upload:** 921600 baud

## Project structure

```
metr and cutter1/
├── VERSION                 # Semantic version (1.4.0)
├── README.md
├── docs/                   # Documentation and reviews
├── releases/v1.4.0/        # Pre-built firmware binaries (.bin)
├── libraries/              # External Arduino/PlatformIO libraries
│   ├── bb_captouch/
│   ├── EspUsbHost/
│   ├── lvgl/
│   └── TFT_eSPI/
└── firmware/               # Main PlatformIO project
    ├── platformio.ini
    ├── config/             # LVGL and board configuration
    ├── include/
    │   ├── version.h       # Central version definitions
    │   ├── app/            # Application logic (GUI, preferences, structs)
    │   ├── tasks/          # FreeRTOS task modules (Task1–Task11)
    │   ├── drivers/        # Hardware drivers (display, printer, USB, WiFi)
    │   └── ui/             # LVGL UI headers (EEZ Studio export)
    ├── src/
    │   ├── main.ino        # Application entry point
    │   └── ui/             # LVGL UI sources, fonts, images
    ├── data/               # SPIFFS / filesystem assets
    └── logs/               # Build and upload logs (gitignored)
```

## Build

Requirements: [PlatformIO](https://platformio.org/) — extension **PlatformIO IDE** in Cursor/VS Code recommended.

### Why Build/Upload buttons may be missing

`platformio.ini` is inside `firmware/`, not the repo root. The PlatformIO sidebar only appears when the **opened folder contains `platformio.ini`**.

**Option A (recommended):** Open `metr-cutter.code-workspace` (multi-root: root + `firmware/`). Use the **Firmware (PlatformIO)** folder for PIO buttons.

**Option B:** File → Open Folder → select `firmware` only.

**Option C:** Run tasks from Cursor: `Ctrl+Shift+B` (Build) or Terminal → Run Task → `PlatformIO: Build` / `PlatformIO: Upload`.

**Option D:** Terminal (PowerShell):

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "d:\metr and cutter1\firmware"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM12 --project-dir "d:\metr and cutter1\firmware"
```

Install extension if missing: Extensions (`Ctrl+Shift+X`) → search **PlatformIO IDE** → Install.

### Build commands

```bash
cd firmware
pio run
```

Upload to board:

```bash
pio run -t upload
```

## Versioning

Version is defined in:

- `VERSION` (project root)
- `firmware/include/version.h` (firmware macros)

Update both when releasing a new version. Display string on the HMI uses `PROJECT_VERSION_DISPLAY` (e.g. `V.1.4.0`).

## Hardware

- Board: ESP32-S3 DevKitC-1
- Display: TFT via TFT_eSPI
- Touch: bb_captouch
- Stepper driver outputs on configurable GPIO (see `main.ino`)

## Libraries

Custom and vendored libraries live in `libraries/` and are referenced from `platformio.ini` via `lib_extra_dirs`.
