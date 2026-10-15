# Project Structure

## Root

| Path | Purpose |
| --- | --- |
| `README.md` | Project overview and first pointers. |
| `GasFilling_HMI/` | Active Arduino ESP32-S3 firmware. |
| `tools/` | Host-side helper scripts. |
| `docs/` | Canonical agent-facing documentation. |
| `artifacts/` | Build helper files and generated screenshots. |
| `build/` | Arduino build output. |

## Firmware Folder

| File or group | Purpose |
| --- | --- |
| `GasFilling_HMI.ino` | Main sketch, hardware init, LVGL display/touch setup, loop. |
| `agent_hmi.h` | Serial protocol, virtual touch, screenshot capture, safe output helpers. |
| `screens.c/.h` | Generated LVGL screen construction. |
| `actions.c/.h` | UI event handlers, start/stop, settings, formula actions. |
| `variables.c/.h` | Runtime process variables and output state. |
| `vars.c/.h` | EEZ/LVGL variable bindings. |
| `ui_image_*.c` | Generated image assets converted for LVGL 9 `RGB565A8`. |
| `fonts.*` | UI font declarations and assets. |

## Tools

| File | Purpose |
| --- | --- |
| `tools/hmi_agent.py` | Serial helper for commands, screenshots, and smoke test. |

## Documentation

The English documents in `docs/` are the canonical guide for future agents. The imported files under `docs/reference-projects/` are source references and may preserve original naming or language.
