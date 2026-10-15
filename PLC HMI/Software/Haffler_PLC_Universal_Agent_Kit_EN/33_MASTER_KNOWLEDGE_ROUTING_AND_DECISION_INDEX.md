# 33. Master Knowledge Routing, Cognitive Decision Index & Anti-Sloppiness Guardrails
## The Definitive Zero-Guesswork Navigation Bridge for Autonomous AI Agents and Senior Engineers

> **Document ID:** `33_MASTER_KNOWLEDGE_ROUTING_AND_DECISION_INDEX.md`  
> **Target Audience:** All AI Coding Agents (Claude Code, OpenAI Codex, GitHub Copilot, Cursor AI, Windsurf, Google Gemini, Antigravity) and Senior Systems Engineers.  
> **Core Mission:** Eliminate 100% of cognitive confusion, rambling responses, sloppy trial-and-error edits, and hallucinations. Route any engineering symptom or task to the exact authoritative module, skill, and source code line instantly.

---

## 1. Instant Symptom-to-Solution Routing Matrix

When an anomaly occurs or an engineering feature is requested, consult this table immediately. Do NOT guess or invent solutions.

| Practical Workshop Symptom / Request | Root Cause Category | Primary Master Module | Specialized Skill | Target C++ Class / File | Decisive Industrial Action |
| :--- | :--- | :---: | :--- | :--- | :--- |
| **Relays click/chatter on boot or reset** | Floating GPIO during boot | **01** | `siemens-industrial-hardening` | `hal_relays_boot_clamp()` | Drive pins HIGH ($1=\text{OFF}$) with pull-ups in the first line of `setup()`. |
| **Relay state cannot be read back** | ESP32 GPIO input read bug | **01** | `siemens-industrial-hardening` | `s_active_relay_mask` | Use a volatile software status mask; never call `gpio_get_level()` on output pins. |
| **Tactile keypad registers erratic clicks** | Resistor ladder ADC noise | **02** | `haffler-touch-and-display-speed` | `readMedian5()` | Sample ADC 5 times, sort, take median. Enforce 45ms debounce window. |
| **Operator requests 100% touchless mode** | Dirty/gloved workshop hands | **03** | `apple-industrial-hmi-design` | `touchless_fsm.cpp` | Deploy 3-state FSM (`IDLE`, `ACTIVE`, `KEYBOARD`) with 2px Apple Blue `#007AFF` focus ring. |
| **HMI stutters or drops below 50 FPS** | Software shadows / redundant redraws | **04** | `lvgl-zero-lag-engine` | `lv_style_set_shadow_width` | Set `shadow_width = 0`. Implement dirty-driven guards: update labels only on value changes. |
| **Hero numbers sag or fall out of alignment** | Manual padding / layout drift | **04** | `apple-lvgl-component-pack` | `lv_obj_center()` | Place label in dedicated non-scrolling card with `pad_all = 0`; call `lv_obj_center()`. |
| **Persian time inverted: "51:00" instead of "00:51"** | BiDi colon punctuation flip | **04, 14** | `haffler-bidi-and-multilingual-guard`| `ui_lang.c` | **NEVER use `%02d:%02d` in Persian!** Use descriptive text ("۱ دقیقه و ۲۰ ثانیه") or LRO locks. |
| **Text cut off on bottom line in Persian** | Baseline dip below canvas | **04** | `haffler-bidi-and-multilingual-guard`| `lv_obj_set_height` | Add $+20\text{ px}$ vertical buffer to guide containers and error modals. |
| **Missing characters or square boxes `[]`** | Missing font glyphs (Tofu) | **04, 14** | `haffler-bidi-and-multilingual-guard`| `ui_font_yekan_20` | Chain Persian font fallback to Montserrat. In Turkish, use ASCII-safe standard characters. |
| **Mobile phone disconnects from SoftAP** | Missing Captive Portal 204 | **05** | `haffler-industrial-webportal` | `web_server.cpp` | Return HTTP `204 No Content` to `/generate_204` and `/hotspot-detect.html` requests. |
| **System crashes when WebApp is opened** | SRAM DRAM starvation ($<40\text{KB}$) | **05** | `haffler-industrial-webportal` | `ESP.getFreeHeap()` | Bundle web assets with GZIP ($<16\text{KB}$). Allocate large web buffers strictly in PSRAM. |
| **Clock rolls back or day-lock tampered** | Operator altered DS3231 RTC | **06** | `haffler-security-and-pin-engine` | `day_lock_core.cpp` | Detect backward jumps $>300\text{s}$; assert `STAGE_TAMPER` and lock automatic cycle. |
| **Flash wear failure after months of operation** | Too many NVS flash writes | **06** | `haffler-security-and-pin-engine` | `HOURLY_COMMIT_INTERVAL` | Accumulate day-lock runtime in battery-backed RTC SRAM; commit to NVS only once per hour. |
| **Customer cannot unlock expired lease** | Factory unlock code needed | **06** | `haffler-security-and-pin-engine` | `day_lock_ui.cpp` | Generate 5-digit challenge code; use `haffler_day_lock_keygen.py` or emergency grace `112233`. |
| **Pneumatic cylinder stops midway / jams** | Mechanical bind or dropped reed | **16** | `haffler-modbus-and-motion-control` | `haffler_actuator.h` | Enforce travel watchdog ($1500..5000\text{ms}$). Auto-retract on timeout and trip Alarm `E02`. |
| **Motor trips circuit breaker on reverse** | Directional arc / lack of dead-time | **17** | `haffler-modbus-and-motion-control` | `modbus_inverter.h` | Enforce strict $\ge 100\text{ ms}$ software dead-time with both directions de-energized. |
| **Stepper axis loses positional accuracy** | Mechanical leadscrew backlash | **16** | `haffler-modbus-and-motion-control` | `stepper_motion.cpp` | Implement unilateral left-only backlash compensation ($0.1..0.5\text{ mm}$). |
| **E-Stop pressed but machine stops slowly** | Software queue latency | **01, 18** | `siemens-industrial-hardening` | `task_plc_engine` | Scan Input $I_6$ on Core 1 every $50\text{ ms}$. Direct hardware relay cutoff in $<20\text{ ms}$. |
| **Sudden line voltage drop causes reboot** | Workshop power sag / brownout | **13** | `haffler-hardware-scenario-fuzzer` | `esp_reset_reason()` | Enable Level 7 ($2.8\text{V}$) brownout detector; decode `ESP_RST_BROWNOUT` in `setup()`. |
| **Machine crashes without error log** | Core panic or task watchdog trip | **13** | `haffler-hardware-scenario-fuzzer` | `rtc_blackbox.cpp` | Read RTC Fast SRAM crash flight recorder; display ISA-18.2 error code on reboot. |
| **Language switched but some English remains** | Hardcoded text without dictionary | **14** | `haffler-bidi-and-multilingual-guard`| `ui_lang_get()` | Register all strings in `s_dict`; eliminate 100% of hardcoded C-strings from UI screens. |
| **User prompt: "خفنش کن"** | Needs modern Apple HIG polish | **04, 22, 31**| `apple-industrial-hmi-design` | `haffler_ui_builder.h` | Apply Dark Slate `#0A0C10`, 1px borders, rounded cards, 44px targets, 0 software shadows. |
| **User prompt: "صنعتیش کن"** | Needs deterministic Siemens rigor | **01, 02, 18**| `siemens-industrial-hardening` | `haffler_sequencer.h` | Enforce 50ms Core 1 scan, PackML FSM, bounded mutexes ($\le 50\text{ms}$), safety interlocks. |
| **User prompt: "لقی داره"** | Mechanical backlash compensation | **16** | `haffler-modbus-and-motion-control` | `haffler_recipe.h` | Adjust parameter `backlash_comp` in 0.1 mm increments. |
| **User prompt: "گیر میکنه"** | Actuator / sensor timeout | **02, 16** | `siemens-industrial-hardening` | `haffler_actuator.h` | Check reed switch debouncing, examine travel timeout limits, verify pneumatic line pressure. |
| **User prompt: "فایل برای سرور بده"** | Release packaging & cloud deploy | **08** | `strict-hmi-firmware-judge` | `tools/gen_lastfile.py` | Package in `release/lastfile` + `release/firmware.bin` (Zero-Desktop). Deploy to ArvanCloud S3. |
| **User provides new machine name from zero** | Autonomous synthesis from scratch | **32** | `haffler-autonomous-machine-synthesis`| `scaffold_new_machine.py`| Run 7-phase physical deconstruction, assign universal pins, build PackML FSM, design HMI. |

---

## 2. The 10 Ironclad Anti-Sloppiness & Zero-Hallucination Guardrails

To prevent AI models from acting lazy, sloppy ("شول و ول"), or guessing, every agent operating in this repository MUST adhere to these 10 rules:

1. **The Exact-Pin Authority Rule:** Never guess or invent pin numbers. `include/haffler_pins.h` is the sole source of truth. If a pin is not in `haffler_pins.h`, it does not exist.
2. **The Targeted Surgical Edit Rule:** Never rewrite an entire 300-line file to change 5 lines of logic. Locate the exact lines with search tools, verify the surrounding context, and apply minimal surgical modifications.
3. **The Non-Blocking PLC Rule:** Never call `delay()`, `vTaskDelay()`, or blocking I/O inside Core 1's 50ms PLC scan task (`task_plc_engine`). Use non-blocking millisecond state timers.
4. **The Bounded Mutex Rule:** Every mutex acquisition touching LVGL or shared resources (`lvgl_lock()`) must enforce a timeout $\le 50\text{ ms}$. Unbounded mutexes (`portMAX_DELAY`) are strictly prohibited.
5. **The Zero-Desktop Rule:** Never write, copy, or output any file to `C:\Users\Haffler\Desktop`. All releases must reside in `<project>/release/`.
6. **The Flashing Prohibition Rule:** Never execute `pio run -t upload` or flashing commands without explicit operator authorization ("فلش کن", "پروگرم کن", "آپلود کن").
7. **The Free DRAM Protection Rule:** Internal SRAM must never drop below $40\text{ KB}$. Large buffers (display frames, web assets, JSON logs) must be allocated in PSRAM (`MALLOC_CAP_SPIRAM`).
8. **The BiDi Colon Prohibition Rule:** Never format time with raw colons (`%02d:%02d`) inside Persian strings. Use descriptive Persian or Unicode LRO/PDF wrappers.
9. **The Empirical Proof-of-Done Rule:** Never declare a task finished without running automated tests (`python tools/simulate_machine.py --test-all` and `python tools/verify_project_compliance.py`).
10. **The Ultra-Brevity Rule:** Status reports must be concise bullet points in Persian with progress percentages (%) and modified file links. No conversational filler.

---

## 3. Physical Process Archetypes & Universal Blueprint Mapping

When building a new machine, identify its physical domain to select the appropriate software architecture:

```
                              [Universal Machine Input]
                                         │
        ┌──────────────────┬─────────────┴─────────────┬──────────────────┐
        ▼                  ▼                           ▼                  ▼
[Sequential Motion]  [PID Thermodynamic]      [Continuous Flow]     [Precision Motion]
UPVC Cutters,        Welding, Ovens,          Ice Cream, Liquid     CNC Routers,
Corner Cleaners,     Autoclaves, Heaters      Bottlers, Mixers      Drilling, Feeders
        │                  │                           │                  │
 ├── Travel Timeouts  ├── Dual Thermocouple    ├── Pulse Flowmeter   ├── High-Speed Pulse
 │   (1500..5000ms)  │   T1/T2 (MAX6675)      │   Edge Counting     │   Q0=STEP, Q1=DIR
 ├── Reed Switch Band ├── SSR Q5/Q8 (10A)      ├── Contactor / Pump  ├── Unilateral
 │   (Analog ADC mV)  │   PID Time-Proportion  │   Anti-Short Cycle  │   Backlash Comp
 └── PackML FSM       └── Over-Temp Interlock  └── Level Sensors     └── Trapezoidal Ramp
```

---

## 4. Multi-Agent Ecosystem Compatibility

This kit is architected for zero-friction interoperability across all leading AI development platforms:

| AI Development Tool | Native Configuration File | Auto-Discovery Behavior |
| :--- | :--- | :--- |
| **Anthropic Claude Code CLI** | `CLAUDE.md` | Loads root commands, operating laws, and test workflows automatically on startup. |
| **Cursor IDE** | `.cursorrules` & `.cursor/rules/` | Enforces Apple HIG, Siemens determinism, and Zero-Desktop rules on every prompt. |
| **GitHub Copilot / Codex** | `.github/copilot-instructions.md` | Automatically injected into Copilot context during code completion and chat. |
| **Windsurf IDE (Codeium)** | `.windsurfrules` | Guides Cascade engine to respect 30 industrial gates and dual-core segregation. |
| **Google Gemini / Antigravity** | `GEMINI.md` & `AGENTS.md` | Comprehensive system prompt, tool mappings, and autonomous leadership directive. |
| **Aider / Open-Source Agents** | `AGENTS.md` | Standard markdown instructions loaded via `--read AGENTS.md`. |

---

## 5. Verification Command Cheatsheet

Before concluding any session or shipping any firmware release, run these automated commands:

```powershell
# 1. Verify all 30 Industrial Compliance Gates (must be 100.0% PASS)
python tools/verify_project_compliance.py .

# 2. Run Headless Finite State Machine & Interlock Simulation (must be 15/15 PASS)
python tools/simulate_machine.py --test-all

# 3. Generate Zero-Desktop Cloud-Ready Release Package
python tools/gen_lastfile.py --out release --ver 1.0.0 --bin .pio/build/esp32-s3-devkitc-1/firmware.bin

# 4. Verify Git Repository Integrity (no trailing whitespace errors)
git diff --check
```
