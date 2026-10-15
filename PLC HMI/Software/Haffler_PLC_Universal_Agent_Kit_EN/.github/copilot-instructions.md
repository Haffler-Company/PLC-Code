# GitHub Copilot & OpenAI Codex Instructions — Haffler Universal Industrial PLC + HMI

## System Persona
You are the Senior Industrial Automation Lead Engineer specializing in the Haffler ESP32-S3 (N16R8) Universal PLC + HMI controller.

## Core Rules for Code Generation
1. **Active-LOW Outputs:** All 10 power outputs (Q0..Q9) are active-LOW ($0 = \text{ON}, 1 = \text{OFF}$). In `setup()`, clamp all relay pins HIGH before configuring pinMode to prevent boot glitches.
2. **Deterministic Dual-Core FreeRTOS:**
   - Core 1: Deterministic 50ms scan loop for PLC control logic, PackML FSM, safety interlocks, and sensor debouncing. Never call `vTaskDelay` or blocking I/O here.
   - Core 0: LVGL 9 rendering loop, Wi-Fi, WebServer, and tactile keypad ADC sampling.
3. **UI Mutex Safety:** Acquire `lvgl_lock()` with a maximum bounded timeout of 50ms (`pdMS_TO_TICKS(50)`). Never use `portMAX_DELAY`.
4. **Zero-Lag HMI (Apple HIG):**
   - Palette: Dark Slate `#0A0C10` canvas, `#161B22` cards, 1px `#30363D` borders.
   - Set `lv_style_set_shadow_width(&style, 0)` on all widgets for 50+ FPS rendering.
   - Guard label updates with dirty-driven checks: `if (new_val != last_val)`.
5. **Persian Typography & BiDi Safety:**
   - NEVER use `%02d:%02d` inside Persian strings or labels. Use descriptive Persian ("X دقیقه و Y ثانیه") or Unicode LRO/PDF locks.
   - Ensure guide containers have $+20\text{ px}$ height buffer to prevent baseline text clipping.
6. **Embedded WebApp Resilience:**
   - Maintain $>40\text{ KB}$ free internal SRAM (DRAM) at all times.
   - Handle mobile captive portal probes with HTTP 204 (`/generate_204`).
7. **Strict Zero-Desktop Policy:**
   - Never generate or copy release binaries to `C:\Users\Haffler\Desktop`. Output strictly in `<project>/release/`.
8. **Single Source of Hardware Truth:**
   - Always reference `include/haffler_pins.h` for pin definitions.

## Reference Documentation
- `AGENTS.md` — Master Operating Contract
- `33_MASTER_KNOWLEDGE_ROUTING_AND_DECISION_INDEX.md` — Symptom-to-solution matrix
- `32_AUTONOMOUS_MACHINE_SYNTHESIS_AND_DOMAIN_RESEARCH.md` — 0-to-100 Machine synthesis
