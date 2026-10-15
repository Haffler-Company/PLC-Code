# Change Readiness Checklist

Use this checklist before, during, and after any code change.

## Before Editing

- Read `docs/AGENT_START_HERE.md`.
- Read the relevant screen behavior in `docs/GAS_FILLING_SCENARIO.md`.
- Read LCD, image, PSRAM, timer, and output risks in `docs/GAS_FILLING_TECHNICAL_KNOWLEDGE.md`.
- Identify whether the change is visual-only, logic-only, hardware-output-related, or mixed.
- If the change may activate Valve 1, Valve 2, or the buzzer, get explicit user approval before hardware tests.

## Display Changes

Check these before touching LCD settings:

- Driver must remain `ST7796_DRIVER`.
- Resolution must match 480x320 landscape.
- `tft.setRotation(3)` is currently correct.
- `TFT_RGB_ORDER TFT_BGR` is currently correct.
- `tft.setSwapBytes(true)` and `pushColors(..., true)` are currently correct.
- If only one icon is corrupted, inspect LVGL image format first.

## UI Image Changes

For LVGL 9 `RGB565A8` images:

- RGB565 plane comes first.
- Alpha plane comes second.
- Descriptor format is `LV_COLOR_FORMAT_RGB565A8`.
- Stride is `width * 2`.

## Process Logic Changes

Review:

- left channel timer path
- right channel timer path
- `Qs`, `P1`, `P2`
- division by zero behavior
- output polarity through `vr_OutPutType`
- progress bar math
- ISR safety

## Real Device Verification

Minimum visual verification:

```powershell
python .\tools\hmi_agent.py --port COM3 cmd PING
python .\tools\hmi_agent.py --port COM3 cmd STAT
python .\tools\hmi_agent.py --port COM3 smoke
```

Expected smoke artifacts:

```text
artifacts/shots/smoke_main.png
artifacts/shots/smoke_setting.png
```

For hardware-output changes, add a manual safe test plan and end with:

```powershell
python .\tools\hmi_agent.py --port COM3 cmd SAFE
```

## Done Criteria

- Code compiles.
- Firmware uploads if the change needs device validation.
- Main screen screenshot looks correct.
- Setting screen screenshot looks correct.
- No unexpected valve or buzzer activation occurred.
- Any new technical discovery is recorded in `docs/GAS_FILLING_TECHNICAL_KNOWLEDGE.md`.
