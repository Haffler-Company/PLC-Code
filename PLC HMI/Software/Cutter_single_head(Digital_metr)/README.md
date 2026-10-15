# Metr & Cutter - ESP32-S3

Firmware for the HAFFLER Metr & Cutter machine.

- Firmware: `firmware/`
- UI: LVGL 9.4 on a 480x320 ESP32-S3 HMI
- Upload port: COM4
- Documentation: [`docs/START.md`](docs/START.md)
- Active work and ownership: [`docs/WORKLOG.md`](docs/WORKLOG.md)

## Quick Build

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "d:\metr and cutter1\firmware"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM4 --project-dir "d:\metr and cutter1\firmware"
```

Important: do not build or upload unless the user explicitly asks for it.

Cursor tasks: `MC: Build` / `MC: Fast Flash` / `MC: Build + Upload` (`.vscode/tasks.json`).
