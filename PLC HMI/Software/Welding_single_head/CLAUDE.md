# AI Model & Agent Universal Rule Pointer

All AI assistants (Gemini, Claude, Cursor, Aider, Copilot, ChatGPT) working in this repository MUST unconditionally load and strictly follow:
1. `AGENTS.md` - Master Industrial Contract & Anti-Hallucination Guardrails
2. `docs/AGENT_PLAYBOOK_FA.md` - 9-Pillar Operational Governance
3. `docs/START.md` - Architecture & Routing Guide
4. `docs/REFERENCE.md` - Hardware Pinouts, FreeRTOS & OTA Release Standards

## Mandatory Directives for ALL AI Models:
1. Zero False Claims: Never claim "problem resolved" without showing exact git diff and verifying compilation with `pio run`.
2. Surgical Edits (Zero Layout Drift): When changing color, modify ONLY local styles (`lv_obj_set_style_*`). When moving objects, modify ONLY the specific target offset (`y` or `x`). Never mix `lv_obj_set_pos` with existing `lv_obj_align`.
3. EEZ Studio Protection: In `src/screens.c`, properties are updated every 20ms in `tick_screen_*`. Change variables in `src/vars.c` or inside `tick_screen_*`, never solely in `create_screen_*`.
4. Token Throttling: Bounded reads only (20-50 lines). Never dump large files in chat. Never load `docs/WELDING_HMI_KNOWLEDGE_FA.md` entirely.
