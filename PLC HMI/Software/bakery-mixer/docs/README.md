# Haffler Universal Industrial PLC HMI Platform & Agent Kit (English Edition)
## Master Engineering Reference, Architecture Guide, and Autonomous Agent Directive

> **Audience:** Haffler Enterprise Leadership, Principal Embedded Firmware Engineers, and Autonomous AI Coding Agents (Gemini, Claude, Antigravity, Cursor, Copilot).  
> **Target Hardware Platform:** Haffler Universal Integrated PLC + HMI Controller based on the **ESP32-S3-WROOM-1 (N16R8)** (240MHz Dual-Core, 16MB Octal Flash, 8MB Octal PSRAM, 480x320 IPS Touchscreen, 8 Optocoupled Inputs, 10 Digital Outputs / SSR, Dual MAX6675 SPI Thermocouples, DS3231 RTC, RS485 Modbus, and 10 Front-Panel Tactile Keys).  
> **Directory Location:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Haffler_PLC_Universal_Agent_Kit_EN\`  
> **Mission Statement:** Combine **Siemens Industrial Determinism, Safety Interlocks & IEC 61508 Rigor** with **Apple Human Interface Guidelines (HIG) Visual Craftsmanship, Ergonomics & Fluid 50+ FPS Performance**. Enable any AI agent or engineer to bootstrap, develop, debug, and ship any industrial machinery project with **zero guesswork, zero regression, and zero friction**.

---

## 1. The 10 Ironclad Operating Laws (Non-Negotiable)

1. **Ultra-Brevity or Complete Silence (No Pleasantries):**
   - Zero conversational filler, pleasantries, flattery, repetition of the user's prompt, or boilerplate excuses.
   - Status updates must be purely telegraphic bullet points, completion percentage (%), modified file links, and decisive binary choices.
2. **Deep Persian Workshop Terminology Understanding:**
   - The owner speaks in concise, practical workshop idioms:
     - *"خفنش کن"* = Polish to Apple HIG elegance (dark matte slate, 1px crisp borders, 44px touch ergonomics, 50+ FPS, zero software shadows).
     - *"صنعتیش کن"* = Enforce Siemens determinism (Core 1 50ms scan, PackML FSM, strict debouncing, hardware interlocks).
     - *"گیر میکنه / هنگ میکنه"* = Actuator timeout, sensor debounce fault, recursive lock contention, or I2C bus lockup.
     - *"لقی داره"* = Backlash compensation issue in encoder/stepper positioning.
     - *"کلید مود بذار"* = Implement 100% touchless front-panel navigation with 10 tactile keys and the `MODE` button.
     - *"فایل برای سرور بده"* = Generate the standard 2-file Desktop release package (`lastfile` + `firmware.bin`).
     - *"ایده بده"* = Deliver structured 4-pillar industrial suggestions (Speed, Safety, Connectivity, Service).
3. **Absolute Prohibition of Flashing Without Explicit Order:**
   - **RED LINE:** Never execute `pio run -t upload`, `esptool.py`, `upload.bat`, or network flashing commands unless the owner explicitly writes: **"فلش کن"**, **"پروگرم کن"**, or **"آپلود کن"**. Compilation, unit tests, and headless simulator verification are permitted.
4. **Zero Guesswork & Absolute Truth Mandate:**
   - Never speculate on pin mappings, variable states, actuator timings, or bug causes. Every statement must be 100% verified against source code or hardware logs.
5. **Zero Regression (Do Not Break Working Code):**
   - Verified, working features must never be broken or speculatively rewritten. Edits must be strictly surgical and localized to the target lines.
6. **Forward-Fix Rigor (No Blind Reverts):**
   - Never run `git reset --hard` or blindly roll back code when a bug appears; rolling back destroys days of hard-won progress. Instead, print targeted diagnostic telemetry (`Serial.printf("[DIAG] ...\n")`), isolate the root cause, and fix it forward.
7. **Rolling 3-Step Backup & Micro-Commit Cadence:**
   - After every 2 to 3 successful modifications or major milestone, commit to Git or run `python tools/backup_manager.py create "desc"` to create a timestamped rolling backup in `backups/`.
8. **Lean Workspace & Zero Clutter:**
   - Never leave temporary scratch files, duplicated test directories, or abandoned scripts. Keep the project repository pristine, structured, and lightweight.
9. **Continuous Knowledge Capture:**
   - Every newly discovered hardware timing nuance, pin behavior, or operator convention must be documented immediately in the repository markdown documentation.
10. **Proactive Cross-Project Research & Smart Scoping:**
    - Before writing a single line of code for a new machine scenario:
      1. Search existing projects in `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\` to reuse proven, field-tested code.
      2. Scope requirements thoroughly and ask at most 2 targeted, multiple-choice clarifying questions to eliminate ambiguity.
      3. Propose smart industrial enhancements before execution to eliminate owner fatigue.

---

## 2. Complete Technical Documentation Index (32 Master Modules)

| Module # | Document File | Scope & Technical Content |
| :---: | :--- | :--- |
| **00** | [`00_AUTONOMOUS_PROJECT_LEAD_DIRECTIVE.md`](./00_AUTONOMOUS_PROJECT_LEAD_DIRECTIVE.md) | Universal autonomous AI agent directive: hands-off client protocol, initial greeting ("Hello! Provide project name and scenario"), 5-phase engineering workflow, and zero user friction mandate. |
| **01** | [`01_HARDWARE_ARCHITECTURE_AND_PINOUT.md`](./01_HARDWARE_ARCHITECTURE_AND_PINOUT.md) | ESP32-S3 N16R8 SoC architecture, dual-core task segregation, definitive pinout matrix, active-LOW relays, boot safe clamping, relay status mask `s_active_relay_mask`, and pin reassignment rules. |
| **02** | [`02_INPUTS_KEYPAD_AND_NOISE_REJECTION.md`](./02_INPUTS_KEYPAD_AND_NOISE_REJECTION.md) | Reading 8 optocoupled 24V/5V inputs with 50ms multi-sample debounce filter, reading 10 front-panel tactile keys via dual ADC channels with 5-sample median filter `readMedian5()`, event handling, and FreeRTOS queue segregation. |
| **03** | [`03_TOUCHLESS_PANEL_AND_MODE_NAVIGATION.md`](./03_TOUCHLESS_PANEL_AND_MODE_NAVIGATION.md) | 100% touchless front-panel navigation, 3-state FSM (`IDLE`, `ACTIVE`, `KEYBOARD`), MODE key traversal, smooth auto-scroll, 10s inactivity watchdog, touch override, START/STOP dual-personality, actionable widget filtering, modal quarantine, and production C/C++ driver code. |
| **04** | [`04_APPLE_HIG_AND_ZERO_LAG_LVGL.md`](./04_APPLE_HIG_AND_ZERO_LAG_LVGL.md) | Apple HIG for 480x320 landscape HMI, dark slate & pure white palettes, 44px touch affordance, 1px crisp borders, zero software shadows (`shadow_width = 0`) for 50+ FPS, dirty-driven rendering, mathematical centering of hero metrics, 16px breathing margins, and Persian RTL BiDi isolation with zero tofu characters. |
| **05** | [`05_EMBEDDED_WEBAPP_AND_CONNECTIVITY.md`](./05_EMBEDDED_WEBAPP_AND_CONNECTIVITY.md) | Zero-hang embedded web server on ESP32-S3, internal DRAM protection (>40KB free SRAM), 100% GZIP compression (<16KB bundle), mobile captive portal 204 probe response, 250ms WebSocket telemetry, dual Wi-Fi (SoftAP + Station), Bluetooth BLE 5.0, and RS485 Modbus RTU / Thermal Printer. |
| **06** | [`06_5_STAGE_DAY_LOCK_AND_SECURITY.md`](./06_5_STAGE_DAY_LOCK_AND_SECURITY.md) | 5-stage cascading leasing time-lock engine, library-free Jalali Solar Hijri calendar converter, DS3231 RTC monotonic anti-tamper clock guard (>300s rollback detection), flash wear-leveling (hourly commits = 11+ years life), 4-tier authentication, offline 5-digit challenge-response, and emergency 10-day grace code (`112233`). |
| **07** | [`07_DIAGNOSTICS_IO_MAPPING_AND_TESTING.md`](./07_DIAGNOSTICS_IO_MAPPING_AND_TESTING.md) | Real-time I/O diagnostic screen with 14px circular neon green indicators (`#22C55E`), manual relay toggle with software mask and cycle interlock, UART0 headless test matrix at 921600 baud, `SHOT` framebuffer capture command, `MEM`, and `PERF` diagnostics. |
| **08** | [`08_OTA_SERVER_DEPLOYMENT_AND_ROLLUP.md`](./08_OTA_SERVER_DEPLOYMENT_AND_ROLLUP.md) | Mandatory 2-file Desktop release package (`lastfile` + `firmware.bin`), 8-section `lastfile` standard, absolute exclusion of buzzer (BZ), inclusion of M0..M7 auxiliary pins, cumulative rollup changelog (<= 13 words/line), ArvanCloud S3 deployment, and 4-step health verification. |
| **09** | [`09_CROSS_PROJECT_REFERENCE_LIBRARY.md`](./09_CROSS_PROJECT_REFERENCE_LIBRARY.md) | Knowledge map and cross-pollination across all 11 existing Haffler machines (Cutter, Metr, Welding, Ice Cream, Gas Filling, Bakery, Corner Cleaning, Copy Router, Punching). |
| **10** | [`10_MASTER_AGENTS_PROJECT_TEMPLATE.md`](./10_MASTER_AGENTS_PROJECT_TEMPLATE.md) | Plug-and-play `AGENTS.md` template for any new project repository to bootstrap autonomous AI agents with zero hand-holding. |
| **11** | [`11_PRE_FLIGHT_VERIFICATION_CHECKLIST.md`](./11_PRE_FLIGHT_VERIFICATION_CHECKLIST.md) | 30-gate pre-flight industrial commissioning checklist across electrical safety, UI performance, touchless navigation, web stability, and release integrity. |
| **12** | [`12_STANDARD_HMI_SCREEN_FLOW_AND_BRAINSTORMING.md`](./12_STANDARD_HMI_SCREEN_FLOW_AND_BRAINSTORMING.md) | Standard 3-tier screen architecture (Startup Splash with Haffler branding, version tag & machine icon -> Main Operation Dashboard with Hero Metric -> F1..F5 Settings Cards -> Diagnostics), 4-pillar industrial brainstorming framework, and zero layout drift rules. |
| **13** | [`13_CRASH_RESILIENCE_BLACKBOX_AND_ALARMS.md`](./13_CRASH_RESILIENCE_BLACKBOX_AND_ALARMS.md) | Hardware reset reason decoding (`esp_reset_reason`), zero flash-wear blackbox logger in battery-backed RTC SRAM, power-loss safety reboot interlock, ISA-18.2 3-tier alarm matrix, and E01..E10 standard error codes. |
| **14** | [`14_MULTILINGUAL_DICTIONARY_AND_RECIPES.md`](./14_MULTILINGUAL_DICTIONARY_AND_RECIPES.md) | Official trilingual industrial dictionary (Persian, English, Turkish), machine recipe management data structures (16 presets), and one-click JSON backup/cloning via WebPortal. |
| **15** | [`15_ZERO_REGRESSION_AND_OPERATIONAL_DISCIPLINE.md`](./15_ZERO_REGRESSION_AND_OPERATIONAL_DISCIPLINE.md) | The 10 Ironclad Operating Laws, anti-frustration protocol, forward-fix discipline, rolling 3-step micro-commit workflow, and proactive deep search. |
| **16** | [`16_PNEUMATICS_AND_MOTION_CONTROL.md`](./16_PNEUMATICS_AND_MOTION_CONTROL.md) | Industrial pneumatic cylinder control, C++ reed-switch timeout manager class, stepper/servo trapezoidal acceleration profiles, and unilateral left-only backlash compensation. |
| **17** | [`17_RS485_MODBUS_AND_INVERTER_GUIDE.md`](./17_RS485_MODBUS_AND_INVERTER_GUIDE.md) | Non-blocking RS485 Modbus RTU master engine, Delta/Inovance/LS VFD inverter profiles, real-time current monitoring, dual-head synchronization, and discrete relay directional dead-time interlocks (>100ms). |
| **18** | [`18_SAFETY_INTERLOCKS_AND_MACHINERY_DIRECTIVE.md`](./18_SAFETY_INTERLOCKS_AND_MACHINERY_DIRECTIVE.md) | Machine safety standards (ISO 13849 & IEC 62061), hardware E-Stop priority over software, EN 574 Two-Hand Anti-Tie-Down control, and air pressure thresholds. |
| **19** | [`19_ZERO_TO_PRODUCTION_TUTORIAL.md`](./19_ZERO_TO_PRODUCTION_TUTORIAL.md) | Comprehensive 7-stage tutorial from an empty folder to a factory-tested production machine in 30 minutes. |
| **20** | [`20_MACHINE_SCENARIO_SPECIFICATION_SHEET.md`](./20_MACHINE_SCENARIO_SPECIFICATION_SHEET.md) | Master machine scenario & technical specification template (I/O maps, sequencer steps, parameters, ISA-18.2 alarms) to eliminate 100% guesswork. |
| **21** | [`21_UNIVERSAL_MACHINE_RECIPES_AND_PRESETS.md`](./21_UNIVERSAL_MACHINE_RECIPES_AND_PRESETS.md) | Universal machine recipes, signal maps, and FSM blueprint encyclopedia covering all 8 core industrial machine classes (Cutting, Welding, Gas Filling, Edge Bander, Corner Cleaning, Copy Router, Bakery Dough/Oven, Ice Cream). |
| **22** | [`22_APPLE_HIG_LVGL_UI_COMPONENT_COOKBOOK.md`](./22_APPLE_HIG_LVGL_UI_COMPONENT_COOKBOOK.md) | Ready-to-use C++ component recipes for Apple HIG slate cards, mathematically centered hero metrics, 16px steppers, pill status badges, and alarm sheets. |
| **23** | [`23_FACTORY_QA_HARDWARE_TEST_SUITE_AND_PRINTER.md`](./23_FACTORY_QA_HARDWARE_TEST_SUITE_AND_PRINTER.md) | Factory QA hardware test suite (PIN 0000), stress test timeouts, dual MAX6675 open-wire detection, MCP23017 expander, and RS485 ESC/POS 58mm thermal printer inspection certificate printing. |
| **24** | [`24_SHIFT_STATISTICS_MATERIAL_ACCOUNTING_AND_AUDIO.md`](./24_SHIFT_STATISTICS_MATERIAL_ACCOUNTING_AND_AUDIO.md) | Multi-shift production accounting (Morning, Afternoon, Night), raw material math (flour kg, argon gas liters, profile meters), and non-blocking FreeRTOS LEDC PWM melodic audio synthesizer. |
| **25** | [`25_AUTONOMOUS_ONE_PROMPT_ENGINEERING_PLAYBOOK.md`](./25_AUTONOMOUS_ONE_PROMPT_ENGINEERING_PLAYBOOK.md) | Master operational runbook for 100% hands-off AI agent execution: handling prompts, 5-phase execution engine, conflict resolution rules, and zero user friction delivery. |
| **26** | [`26_AGENT_PROMPT_RECIPES_AND_WORKFLOWS.md`](./26_AGENT_PROMPT_RECIPES_AND_WORKFLOWS.md) | Golden copy-paste prompt recipes for machine owners to drive autonomous agents across bootstrapping, UI styling, touchless navigation, and OTA releases. |
| **27** | [`27_AGENT_QUICKSTART_CHEAT_SHEET.md`](./27_AGENT_QUICKSTART_CHEAT_SHEET.md) | AI Agent 1-page fast-track operational playbook: 5-step autonomous flow from user greeting to production release with zero user friction. |
| **28** | [`28_AUTONOMOUS_SCENARIO_EXPANSION_AND_PIN_RESOLVER.md`](./28_AUTONOMOUS_SCENARIO_EXPANSION_AND_PIN_RESOLVER.md) | Autonomous scenario expansion engine, physical process phase generator, MAX6675 SPI thermodynamic sensing integration, pin contention rules (GPIO 21/0), and complete Bakery Spiral Mixer reference specification. |
| **29** | [`29_STEP_BY_STEP_MACHINE_DEVELOPMENT_WALKTHROUGH.md`](./29_STEP_BY_STEP_MACHINE_DEVELOPMENT_WALKTHROUGH.md) | Foolproof 9-step zero-friction developer & agent walkthrough: from 1-line scenario prompt to certified Zero-Desktop release in release/ with zero guesswork. |
| **30** | [`30_UNIVERSAL_MACHINE_ARCHETYPES_AND_FSM_COOKBOOK.md`](./30_UNIVERSAL_MACHINE_ARCHETYPES_AND_FSM_COOKBOOK.md) | Ready-to-wire C++ FSMs for the 4 primary machine architectures (Sequential Pneumatics, Motion with Backlash, PID Thermal, and Two-Stage Volume Batching). |
| **31** | [`31_LVGL9_APPLE_HIG_SCREEN_BLUEPRINTS.md`](./31_LVGL9_APPLE_HIG_SCREEN_BLUEPRINTS.md) | Ready-to-copy C/C++ LVGL 9 code for the 7 primary industrial UI components (Header bar, Centered Hero Metric, 2x2 sub-cards, 16px steppers, ISA-18.2 alarms, virtual keypad, I/O diagnostics). |

---

## 3. Visual Design System & Apple HIG Reference Mockups

The platform includes comprehensive reference visual designs illustrating exact Apple HIG dark ergonomics, layout geometry, typography, touchless navigation, OTA deployment, and factory testing standards:

| HMI Main Dashboard | Parameters & Steppers (16px Margin) |
| :---: | :---: |
| ![HMI Dashboard](./assets/images/hmi_main_dashboard.png) | ![Settings Steppers](./assets/images/hmi_settings_steppers.png) |
| *Apple HIG Dark Slate 480x320 HMI with Centered Hero Metric* | *16px Breathing Margin, 44px Touch Targets & Recipe Badges* |

| 5-Stage Leasing Day-Lock Modal | Hardware I/O Diagnostics Screen |
| :---: | :---: |
| ![Day-Lock Modal](./assets/images/day_lock_leasing_ui.png) | ![I/O Diagnostics](./assets/images/io_diagnostics_screen.png) |
| *Chained 5-Stage Day-Lock with Jalali RTC & 4-Digit Challenge* | *Real-Time 14px Neon Green Inputs & Active-LOW Relay Toggles* |

| Embedded Mobile WebApp Portal | ISA-18.2 Alarms & RTC Blackbox |
| :---: | :---: |
| ![Mobile WebApp](./assets/images/mobile_webapp_portal.png) | ![Alarms Blackbox](./assets/images/hmi_alarm_blackbox.png) |
| *Zero-Hang Embedded WebApp with GZIP (<16KB) & WebSocket Telemetry* | *ISA-18.2 3-Tier Alarms, Acknowledge State & RTC SRAM Blackbox* |

| Touchless Keypad Navigation & Focus Ring | Factory QA Test Suite (PIN 0000) |
| :---: | :---: |
| ![Touchless Navigation](./assets/images/touchless_mode_navigation.png) | ![Factory QA Suite](./assets/images/factory_hardware_test_suite_ui.png) |
| *2px Apple Blue (#007AFF) Focus Ring with 3-State MODE Traversal* | *Factory QA Suite with ESC/POS Thermal Printer & Relay Testing* |

### Master Engineering Architecture Schematics

| Hardware Architecture & Dual-Sensor Circuit | FreeRTOS Dual-Core Task Segregation |
| :---: | :---: |
| ![Hardware Architecture](./assets/images/hardware_pinout_and_dual_sensor_circuit.png) | ![FreeRTOS Dual-Core](./assets/images/freertos_dual_core_architecture.png) |
| *HAFFLER ETV3A Pinout & Dual-Reed Resistor Divider Circuit* | *Core 1 50ms PLC vs Core 0 UI & Memory Hierarchy* |

| 5-Stage Day-Lock Timeline & Challenge Flow | Touchless Navigation 3-State FSM |
| :---: | :---: |
| ![Day-Lock Timeline](./assets/images/day_lock_timeline_and_challenge.png) | ![Touchless Navigation FSM](./assets/images/touchless_navigation_fsm.png) |
| *Monotonic RTC, Jalali Conversion & Phone Unlock Flow* | *10 Front Keys, 3-State FSM & Mode Key Traversal* |

---

## 4. Turn-Key Developer Assets & Automation Tools

1. **[`create_new_project.bat`](./create_new_project.bat):** Single-click Windows script to scaffold a new project directory in the master workspace with pre-populated configuration, code skeleton, and `AGENTS.md`.
2. **[`starter_template/`](./starter_template/):** Production-ready, fully compilable PlatformIO project containing:
   - `platformio.ini`: Fine-tuned ESP32-S3 configuration (16MB Octal Flash, 8MB PSRAM, -O2 speed optimization, local library paths).
   - `partitions_16mb_ota7m.csv`: 16MB flash layout with dual 7MB OTA partitions (`app0` / `app1`) and protected NVS.
   - `haffler_pins.h`: Definitive hardware pin definitions (Active-LOW relays, analog band sensors, ADC1 keypad).
   - `haffler_sequencer.h`: Zero-allocation deterministic step sequencer with step timeouts and completion callbacks.
   - `haffler_actuator.h`: Universal Actuator & Cylinder HAL managing active-LOW relays, dual-reed switch voltage bands, and auto-retract travel watchdogs.
   - `haffler_recipe.h`: Universal NVS recipe & parameter manager (up to 10 stored recipes, 16 bounds-checked parameters, CRC checksum).
   - `haffler_ui_builder.h`: High-level Apple HIG screen builder (Dashboard, Stepper Settings, Diagnostics).
   - `main.cpp`: FreeRTOS dual-core task segregation (Core 1 50ms PLC cycle, Core 0 UI/Network), zero-nanosecond boot clamping, and software status mask.
   - `splash_screen.h` & `splash_screen.cpp`: Apple HIG startup splash and loading animation.
   - `pneumatic_cylinder.h`: Ready-to-use C++ pneumatic cylinder manager with reed switch timeout alarms.
   - `modbus_inverter.h`: Ready-to-use non-blocking RS485 Modbus RTU inverter drive controller.
   - `two_hand_safety.h`: Ready-to-use EN 574 Type III C two-hand anti-tie-down safety controller.
3. **[`tools/`](./tools/):** Automated Python utility suite:
   - `scaffold_new_machine.py`: One-command CLI project scaffolder tailored to 8 machine archetypes.
   - `simulate_machine.py`: Offline CLI sequence and safety interlock simulator with automated E-Stop and timeout tests.
   - `hmi_simulator.html`: Interactive 480x320 browser-based hardware simulator with 10 tactile keys and 24V I/O lamps (100% English).
   - `verify_project_compliance.py`: 30-gate automated industrial and Apple HIG compliance auditor.
   - `capture_screen.py`: Serial framebuffer screen capture via `SHOT` command to PNG.
   - `pack_webapp.py`: Single-file HTML to GZIP C-header compressor (<16KB).
   - `gen_lastfile.py`: 8-section `lastfile` and 2-file Desktop release packager.
   - `verify_ota.py`: 4-step health verifier for ArvanCloud S3 deployments.
   - `backup_manager.py`: Rolling 10-snapshot backup and Git micro-commit manager.

---

## 5. How to Bootstrap Any New Project

1. Run [`create_new_project.bat`](./create_new_project.bat) on Windows and enter the project name (e.g. `Edge_Bander`).
2. Open your AI agent or IDE and provide the machine scenario:
   > *"Bootstrap a new project based on the Haffler Universal Industrial PLC Platform and the Desktop English Agent Kit. Machine Scenario: [Describe the machine requirements]."*
3. The AI agent will immediately operate as a **Senior Industrial Project Lead**, following all Siemens determinism and Apple HIG principles with zero guesswork and zero regression.
