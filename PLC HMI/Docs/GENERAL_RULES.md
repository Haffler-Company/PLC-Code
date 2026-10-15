# PLC HMI General Rules

## Workflow Rules
- Always create a timestamped backup before high-impact UI/control edits.
- Keep changes incremental: apply, compile, and verify after each logical block.
- Do not remove user logic blindly; wrap and clamp it first, then refactor.

## Safety Rules
- Any timer-based lock must be day-based and persisted across restart.
- For lock features, add anti-rollback logic against RTC time reversal.
- If RTC health is uncertain while lock is enabled, fail safe to locked state.
- Emergency extension codes must be one-time only and persisted.

## UI Rules
- Runtime language switch must update all critical operational labels, not only static design-time text.
- Keep operator-critical pages non-scroll when possible; fit primary controls in one viewport.
- Show clear status text for lock/timer (`Disabled`, `N days left`, `Locked`).
- Display units consistently; keep `°C` on the right side of numeric temperature values.

## Data Integrity Rules
- Clamp persisted settings before applying to runtime (`temperature target`, `timer days`, etc.).
- Store normalized values back to non-volatile memory after clamping.
- Avoid derived-parameter math without denominator validation (no divide-by-zero paths).

## Collaboration / Owner Preferences (learned)
- Token economy: keep explanations short and to the point; avoid long write-ups.
- Fixes must never introduce bugs. Self-verify every change (compile/check) BEFORE presenting it.
- Prefer incremental, modular refactor over a full from-scratch rewrite (stability first).
- At the start of every new chat, fully read this project's `GENERAL_RULES.md` and `KNOWLEDGE.md` before acting.
- Keep a short per-project changelog INSIDE the project folder (e.g. `CHANGELOG_AI.md`) so past work need not be re-reviewed.
- Whatever new ethics/working rules are learned -> append here; whatever new technical facts are learned -> append to `KNOWLEDGE.md`.
- Ask for exact hardware/board settings before flashing; wrong PSRAM/Flash can bootloop.
- Build/upload tooling: use whichever fits best — `arduino-cli` for Arduino `.ino` sketches, `PlatformIO` for `platformio.ini` projects (or when arduino-cli is awkward). Owner is fine with either; pick the better tool per project.

## Maintainability Rules
- Keep multilingual strings centralized in helper functions when possible.
- Prefer runtime text override for generated LVGL screens instead of deep manual rewrite of generated blocks.
- Document every added persisted key and anti-bypass condition in `KNOWLEDGE.md`.
- For high-frequency LVGL tick functions, avoid rewriting long multilingual text every cycle; update only when the related state or language actually changes.
- For RTL languages, avoid depending only on per-tick text-compare against rendered widget text; use explicit state caches for updates.
- Apply explicit readable font styling for language dropdowns and their option lists.
