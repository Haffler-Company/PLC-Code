# Transferred Rules From Reference Projects

These rules were adapted from the cutter and welding projects and should guide future work on Gas Filling.

## Display Rules

1. Validate display changes on real hardware with screenshots.
2. Do not treat compile success as visual success.
3. Driver, rotation, color order, byte swap, and push function must be considered together.
4. A single corrupted icon usually points to asset format, not necessarily LCD driver.
5. Avoid heavy first-frame rendering paths that can stall embedded LVGL screens.

## Agent And Test Rules

1. Keep a serial control path available for every serious UI project.
2. Use `SHOT` screenshots for feedback instead of relying on verbal descriptions alone.
3. Smoke tests should be safe by default and must not activate dangerous outputs.
4. Hardware output pulse commands must be explicit, short, and followed by `SAFE`.

## HMI Product Rules

1. The operator should understand the main action within a few seconds.
2. Main status, target time, progress, and Start/Stop controls must stay visually obvious.
3. Do not hide critical output state behind decorative UI.
4. Touch targets must remain large enough for industrial use.
5. Avoid changes that make the screen look better in a simulator but worse on the LCD.

## Code Rules

1. Keep LVGL calls out of ISRs.
2. Guard every formula against division by zero.
3. Preserve UTF-8 encoding when editing multilingual strings.
4. Keep technical discoveries in `docs/GAS_FILLING_TECHNICAL_KNOWLEDGE.md`.
5. Keep user-facing behavior documented in `docs/GAS_FILLING_SCENARIO.md`.
