# Agent HMI Protocol

This document explains how an agent can interact with the real Gas Filling HMI after the firmware is uploaded to the ESP32-S3 board.

## Firmware Integration

Agent support lives in:

```text
GasFilling_HMI/agent_hmi.h
tools/hmi_agent.py
```

The sketch integrates the agent at these points:

- `agent_begin()` in `setup()`
- `agent_on_flush(...)` inside the LVGL display flush callback
- `agent_read_touch(data)` before physical touch is read
- `agent_poll()`, `agent_before_lvgl()`, `lv_timer_handler()`, `ui_tick()`, and `agent_after_lvgl()` inside `loop()`

The agent captures the actual LVGL flush output into PSRAM, so screenshots are from the real rendered display path, not a desktop mock.

## Serial Commands

Commands are line-based over USB serial.

| Command | Purpose |
| --- | --- |
| `PING` | Connectivity check. Expected response: `PONG`. |
| `HELP` | List supported commands. |
| `STAT` | Report screen, timers, buzzer state, heap, PSRAM, and screenshot state. |
| `SHOT` | Capture and stream the current 480x320 RGB565 screen as base64 rows. |
| `SCR1` or `MAIN` | Navigate to the Main screen. |
| `SCR2`, `SET`, or `SETTING` | Navigate to the Setting screen. |
| `TAP x y [ms]` | Inject a short touch at screen coordinate `x,y`. |
| `PRESS x y` | Start an injected touch and keep it pressed. |
| `REL` | Release the injected touch. |
| `SAFE` | Stop valve timers, stop buzzer activity, and return outputs to safe state. |
| `PULSE1 [ms]` | Pulse Valve 1 output for a short hardware test. Use only with explicit user approval. |
| `PULSE2 [ms]` | Pulse Valve 2 output for a short hardware test. Use only with explicit user approval. |
| `BUZ [ms]` | Pulse the buzzer for a short hardware test. Use only with explicit user approval if noise or safety matters. |

## Python Helper

From the project root:

```powershell
python .\tools\hmi_agent.py --port COM4 cmd PING
python .\tools\hmi_agent.py --port COM4 cmd STAT
python .\tools\hmi_agent.py --port COM4 shot --out .\artifacts\shots\main.png
python .\tools\hmi_agent.py --port COM4 smoke
```

Use another port if Windows assigns a different COM number.

## Screenshot Format

`SHOT` output begins with:

```text
SHOT_BEGIN 480 320 RGB565LE
```

Then the firmware sends one base64 line per screen row, followed by `SHOT_END`. The Python helper converts this into a PNG.

Requirements:

- Build with `PSRAM=enabled`.
- Runtime `STAT` should report nonzero free PSRAM.
- A full screen capture needs 480 * 320 * 2 bytes = 307,200 bytes.

If `SHOT_ERR alloc_psram` appears, the board was probably compiled with a PSRAM option that does not match the hardware. `PSRAM=opi` compiled but returned zero runtime PSRAM on the tested board; `PSRAM=enabled` worked.

## Smoke Test

`python .\tools\hmi_agent.py --port COM4 smoke` performs a safe UI-level check:

1. `PING`
2. `STAT`
3. open Main and save `artifacts/shots/smoke_main.png`
4. open Setting and save `artifacts/shots/smoke_setting.png`
5. send `SAFE`

This does not activate the valve outputs.

## Useful Coordinates

Screen size is 480x320.

| Target | Coordinate | Notes |
| --- | ---: | --- |
| Main settings icon | `430 30` | Opens Setting. |
| Setting back button | `430 30` | Returns to Main. |
| Left Start area | near `20 245` | Hardware action; prefer manual confirmation before real output tests. |
| Left Stop area | near `120 245` | Stops left channel. |
| Right Start area | near `260 245` | Hardware action; prefer manual confirmation before real output tests. |
| Right Stop area | near `360 245` | Stops right channel. |

Use screenshots after every navigation test. Coordinates may need small adjustments after UI edits.

## Safety Rules

- Do not run `PULSE1`, `PULSE2`, or long output tests without explicit user approval.
- Always send `SAFE` after a hardware-facing test.
- For visual-only UI changes, use `smoke` and screenshots.
- If the LCD shows three color bands, half image, missing image, or bad colors, read `docs/GAS_FILLING_TECHNICAL_KNOWLEDGE.md` before changing random display settings.
