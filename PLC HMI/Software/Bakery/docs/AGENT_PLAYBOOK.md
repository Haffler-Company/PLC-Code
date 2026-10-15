# Operational Governance & The 9-Pillar Playbook — Bakery HMI

This document governs the operational execution standards for all software engineers and AI agents working on the Bakery Dough Divider HMI / PLC system.

---

## Pillar 1: High Execution Speed & Zero Token Waste
- **Bounded Reads Only:** Never dump large source files into context. Use targeted `grep_search` and bounded `view_file` ranges (20 to 50 lines).
- **Binary Asset Isolation:** Never inspect large binary C arrays (`ui_img_*.c`, `ui_font_*.c`, `.bin` files) unless explicitly modifying that specific asset.
- **Surgical Edits:** Apply minimal, highly targeted line replacements via `replace_file_content` rather than massive file overwrites.

---

## Pillar 2: Siemens Industrial Determinism & Zero Guesswork
- **Strict Anti-Hallucination Mandate:** Every claim about variables, pins, logic states, or timing must be 100% verified using tool inspection before assertion.
- **Inverter & Motion Interlocks:**
  - Forward and Reverse inverter lines (`INVERTER_FORWARD` and `INVERTER_REVERSE`) must never be energized concurrently.
  - A minimum dead-time of 100ms must be enforced on direction changes.
- **Sensor Timeout Guards:** No motion loop waiting on inductive sensors (`PISTON_SENSOR` or `FUNNELE_SENSOR`) shall run without a watchdog timeout (maximum 5000ms).

---

## Pillar 3: Prompt Expansion & Technical Translation
- Brief Persian user requests (e.g., "وزن چانه رو درست کن" or "وقفه رو زیاد کن") must be automatically translated and expanded into comprehensive industrial English technical specifications covering:
  - Affected firmware modules (`tasks.h`, `storage.h`, `defines.h`, `lvgl_function.h`)
  - Impact on non-volatile NVS variables and UI reactivity
  - Full verification via PlatformIO compilation (`pio run`).

---

## Pillar 4: Continuous Knowledge Capture
- Any newly discovered hardware pin, electrical characteristic, sensor subtlety, or operational convention must be immediately documented in:
  - Hardware specifications: [`docs/HARDWARE_IO_PINOUT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/HARDWARE_IO_PINOUT.md)
  - Machine knowledge: [`docs/BAKERY_HMI_KNOWLEDGE.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/BAKERY_HMI_KNOWLEDGE.md)
  - Change log: [`docs/WORKLOG.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/WORKLOG.md).

---

## Pillar 5: Multi-Session Concurrency & Locking
- Before editing any file, inspect `git status -s` and verify the `Active Locks` section of `docs/WORKLOG.md` to prevent race conditions across parallel sessions.
- Release locks in `docs/WORKLOG.md` upon completion.

---

## Pillar 6: Zero Wandering & Focused Scope
- Avoid exploring unrelated directories. Route directly to target files using [`docs/START.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Bakery/docs/START.md).

---

## Pillar 7: Grounded & Benchmarked Innovation
- Feature suggestions must be backed by industrial field standards (e.g., features proven in `Welding_single_head`), memory and CPU profiling (DRAM/PSRAM/Flash), and operator safety value.

---

## Pillar 8: Executive Persian Reporting
- All engineering documentation, codebase comments, and system specs are strictly in English.
- Direct conversational responses to the project owner must be concise, executive, and in clear Persian with direct markdown links to modified files.

---

## Pillar 9: Autonomous 360° Delivery
- A task is defined as "DONE" only when:
  1. Firmware logic is completely implemented.
  2. NVS persistence is handled cleanly.
  3. FreeRTOS core segregation and LVGL thread-safety are respected.
  4. Apple HIG visual standards are satisfied.
  5. Local compilation (`pio run`) completes with 100% success.
  6. Rolling backup and git micro-commit are recorded.
