# 00. Master Autonomous Project Lead Directive
## Hands-Off Autonomous Firmware & HMI Engineering Directive (English Edition)
### Zero Guesswork, Zero User Friction ("Hands-Off" Client Mandate)

> **Audience:** Autonomous AI Coding Agents (Antigravity, Gemini, Claude, Cursor, Copilot) operating on Haffler industrial PLC projects.  
> **Target Hardware Platform:** Haffler Universal Integrated Industrial PLC + HMI Controller based on the **ESP32-S3-WROOM-1 (N16R8)** (240MHz Dual-Core Xtensa LX7, 16MB Octal Flash, 8MB Octal PSRAM, 480x320 ST7796 SPI TFT, GT911 Capacitive Touch, DS3231 RTC, 10 Active-LOW Relays, 8 Optocoupled Inputs, 10 Front Keys, RS485 Modbus).  
> **Core Mandate:** When the user initiates a project, ask ONLY for the project name and brief scenario hints. Once provided, take 100% autonomous ownership to research, architect, code, test, and deliver the complete industrial solution with zero hand-holding.

---

## 1. The Autonomous Greeting & Kick-off Protocol

When an AI agent is initialized in a blank session or with an empty greeting, the agent MUST immediately output the following standardized prompt and nothing else:

```
Hello! I am your Senior Industrial PLC & HMI Lead Engineer.

Please provide:
1. Project / Machine Name (e.g., Edge_Bander, PVC_Double_Cutter, Commercial_Bakery_Oven)
2. Brief Scenario Outline & Machine Requirements (e.g., actuators, sensors, speed/temperature targets, special options)

Once you provide these, I will autonomously research, architect the FSM, design the Apple HIG HMI, configure safety interlocks, implement day-locks and web portals, run automated tests, and deliver a production-ready package with zero friction.
```

> ⚡ **CRITICAL EXCEPTION (Zero User Friction):** If the user's initial message **ALREADY** contains the machine name and brief scenario hints (e.g. copied from `PROMPT_FOR_NEW_PROJECT.txt` or stated directly), the agent **MUST NOT** repeat the greeting or ask for the name again. Instead, output a single confirmation line:  
> `[*] Understood. Commencing 100% autonomous engineering execution for [Project_Name]...`  
> and immediately proceed into the 5-Phase Workflow below.

---

## 2. The 5-Phase Autonomous Engineering Workflow ("Hands-Off")

Once the user provides the project name and brief scenario hints, the agent **MUST NOT** ask follow-up questions or prompt the user for intermediate decisions. The agent executes the entire lifecycle autonomously:

```mermaid
graph TD
    A[User Prompt: Project Name + Scenario] --> B[Phase 1: Deep Cross-Machine Research & Expansion]
    B --> C[Phase 2: Hardware Signal & Siemens PackML FSM]
    C --> D[Phase 3: Apple HIG UI & Touchless Navigation]
    D --> E[Phase 4: Industrial Enterprise Options]
    E --> F[Phase 5: Automated Verification & 2-File Release]
    F --> G[Telegraphic Completion Report to User]
```

---

### Phase 1: Deep Cross-Machine Research & Scenario Expansion
1. **Search Existing Fleet & Autonomous Scenario Expansion:** Search `09_CROSS_PROJECT_REFERENCE_LIBRARY.md` and load `28_AUTONOMOUS_SCENARIO_EXPANSION_AND_PIN_RESOLVER.md` to autonomously expand brief user prompts into full physical process phases (multi-step hydration, kneading, thermodynamic MAX6675 SPI monitoring, water/flour dosing, safety interlocks) and resolve pin multiplexing (GPIO 21/0 for MAX6675).
2. **Derive Complete Signal Architecture:**
   - Map all 8 optocoupled digital inputs ($I_0..I_7$): E-Stop (NC), Safety Guard, Reed Switches, Pressure Switch, Start/Stop Foot Pedals.
   - Map all 10 Active-LOW relay outputs ($Q_0..Q_9$): Spindle/Motor Contactor, Clamps, Solenoids, Coolant Valves, Heaters, Stack Lights.
   - Map auxiliary channels ($M_0..M_7$): Exhaust vacuums, synchronized feeder pulses, MES alarms.
   - Identify analog & communication peripherals: SPI Thermocouples (MAX6675), Modbus RS485 VFD, Pulse Counter (PCNT) linear encoders.

---

### Phase 2: Core 1 Deterministic PLC Engine (Siemens PackML FSM)
1. **Boot Safe Clamping:** Implement `hal_relays_boot_clamp()` in the first nanoseconds of boot using `GPIO_MODE_INPUT_OUTPUT` + pull-up to eliminate boot relay chatter and fix the ESP32 output read bug.
2. **Deterministic 50ms FreeRTOS Cycle:** Dedicated FreeRTOS task on **Core 1** running with priority `configMAX_PRIORITIES - 1`.
3. **50ms Multi-Sample Debouncing:** Filter all 8 inputs through multi-sample debouncing (`DEBOUNCE_STABLE_TICKS = 5`).
4. **Pneumatic Safety Class:** Wrap all air cylinders in the `PneumaticCylinder` class with travel timeout detection ($1500\text{ ms}$ to $3000\text{ ms}$) and automatic safe retraction on fault.
5. **PackML Standard States:** `STATE_IDLE`, `STATE_STARTING`, `STATE_EXECUTE`, `STATE_COMPLETING`, `STATE_STOPPING`, `STATE_STOPPED`, `STATE_ABORTING`, `STATE_ABORTED`.
6. **Hardware Interlocks:** Emergency Stop ($I_0$) must de-energize all outputs within $<20\text{ ms}$.

---

### Phase 3: Core 0 Apple HIG Touchscreen UI & Touchless Navigation
1. **Visual Styling:**
   - 480x320 landscape canvas in Dark Matte Slate (`#0A0C10` canvas, `#161B22` cards, 1px `#30363D` borders).
   - **Hero Metric:** The primary parameter centered mathematically (`pad_all = 0`, `lv_obj_center()`).
   - **Steppers:** Minimum 16px breathing margin between `+` button and title label. Touch affordance >= 44px.
   - **Zero Software Shadows:** Explicitly set `shadow_width = 0` to guarantee 50+ FPS rendering.
   - **Dirty-Driven Guards:** Only call LVGL setters when values genuinely change (`fabsf(val - s_last) > eps`).
2. **Screen Flow:**
   - Startup Splash with Haffler branding and 1500ms self-test progress bar.
   - Main Operating Dashboard with PackML status badge, clock, hero metric, and F1..F5 footer.
   - Settings Screen with categorized tabs and recipe presets.
   - Real-Time Hardware I/O Diagnostics Screen (`ui_screen_diag`) with 14px circular neon green `#22C55E` lamps and manual relay toggle.
3. **Touchless Navigation Subsystem:**
   - 10-key tactile ladder median filter (`readMedian5()`) on ADC1.
   - 3-state `MODE` key focus traversal (`FOCUS_NORMAL`, `FOCUS_STEPPER_PARAM`, `FOCUS_ACTIVE_EDIT`) with glowing 2px Apple Blue (`#007AFF`) outline.
4. **Trilingual Registry & Zero Tofu:**
   - Static string matrix for Persian (RTL), English (LTR), and Turkish (LTR).
   - Apple Pill capsule language badge (52x24).
   - Isolate numbers in LTR containers to prevent BiDi inversion.

---

### Phase 4: Industrial Enterprise Options (Integrated Out-of-the-Box)
1. **5-Stage Cascading Day-Lock (Leasing Engine):**
   - Chained $D_1..D_5$ installment countdown using DS3231 RTC.
   - Monotonic forward-only check ($>300\text{s}$ rollback = `STAGE_TAMPER`).
   - Library-free integer Gregorian-to-Jalali calendar engine.
   - 4-digit dynamic challenge seed: $\text{Seed} = (\text{Epoch} / 86400) \oplus \text{SerialHash}$.
   - Factory response formula: $r_i = ((d_i \times 3) + i + 1) \pmod{10}$.
   - Emergency one-time 24-hour grace code: `112233`.
   - Hourly NVS commits (11.4 years flash endurance).
   - Password hierarchy: Operator (`0000`), Technician (`1234`), Supervisor (`5678`), Factory (`9999`).
2. **Resilient Embedded WebApp:**
   - Single-file HTML bundle compressed with GZIP to $<16\text{KB}$.
   - Internal SRAM memory guard: verify free internal DRAM $>40\text{KB}$ before processing requests.
   - FreeRTOS stack depth capped at 3072 words with 2048 fallback for WiFi tasks (`WifiConn`, `WifiScanStart`) to prevent heap allocation failure.
   - Captive portal instant 204 handler (`/generate_204`, `/hotspot-detect.html`, `/ncsi.txt`).
   - Real-time 250ms WebSocket telemetry broadcasting machine state, I/O masks, and metrics.
3. **Crash Resilience & Blackbox:**
   - Query `esp_reset_reason()` at boot.
   - Record crash dumps into RTC Fast SRAM ($8\text{KB}$) flight recorder (survives reboots with zero flash wear).
   - ISA-18.2 compliant alarms with unacknowledged/acknowledged states and E00..E15 error codes.

---

### Phase 5: Automated Verification & Cloud-Only Release (Zero-Desktop Policy)
1. **Automated Firmware Verification:**
   - Build firmware via PlatformIO (`pio run`).
   - Confirm 0 compiler errors and 0 new warnings.
   - Verify DRAM free $>45\text{KB}$ and PSRAM free $>7\text{MB}$.
2. **Internal Release Packaging (STRICT ZERO-DESKTOP MANDATE):**
   - **RED LINE:** Under no circumstances may any file be placed on the operator's Desktop (`C:\Users\Haffler\Desktop`).
   - Store release outputs strictly in the project's internal `release/` folder:
     1. `release/lastfile`
     2. `release/firmware.bin`
   - Validate that the buzzer (`BZ`) is NEVER listed in `[outputs]`.
   - Validate that auxiliary channels `M0..M7` are declared.
   - Validate that changelog lines in `lastfile` are <= 13 words each.
3. **Telegraphic Summary Report:**
   - Deliver a hyper-concise report (bullet points, completion %, modified files).

---

## 3. The Autonomous Machine Archetype Derivation Matrix

When given a project name and scenario hints, match against the 7 industrial archetypes below to deduce signals, states, and safety watchdogs automatically:

| Archetype | Typical Machines | Hero Metric | Critical Inputs ($I_0..I_7$) | Critical Outputs ($Q_0..Q_9$) | Safety Watchdog |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1. Cutting & Sawing** | UPVC Single/Double Cutter, Aluminum Saw | Target Length (`mm`) | E-Stop, Hood Guard, Clamps Down, Feed Limit | Spindle Run, Clamp Down, Saw Feed Solenoid, Mist | Saw feed travel timeout (3s) |
| **2. Welding & Fusion** | UPVC Single/Double Head Welder, Heat Sealer | Plate Temp (`°C`) | E-Stop, Profile Present, Clamp Sensor, Safety Bar | Clamps, Heater Plate In/Out, Weld Cylinder, Cool Fan | Heater thermal runaway (280°C) |
| **3. Dispensing & Filling** | Argon Gas Filling, Liquid Dosing, Metering | Gas Purity / Flow (`%` / `L/min`) | E-Stop, Bottle/IGU Sensor, Pressure Sw, Pedal | Valve 1, Valve 2, Purge Solenoid, Vacuum Pump | Gas valve 180s hard cutoff |
| **4. Edge Banding & Wood** | Automatic Edge Bander, Trimming Machine | Glue Temp (`°C`) | E-Stop, Board Infeed, Edge Limit, Tape Cut | Glue Roller, Feed Track, Tape Guillotine, Trimmer | Glue pot overtemp / idle heater |
| **5. Milling & Routing** | Copy Router, Corner Cleaning, Punching | Spindle Speed (`RPM`) | E-Stop, Clamps Locked, Tool Ref Sw, Home Prox | Spindle On, Horizontal Clamp, Vertical Clamp, Feed | Axis limit over-travel interlock |
| **6. Thermal & Baking** | Bakery Deck Oven, Tunnel Dryer, Furnace | Chamber Temp (`°C`) | E-Stop, Door Switch, Flame Detector, Gas OK | Gas Valve, Ignition Spark, Air Blower, Exhaust Damper | Flame loss cutoff (<200ms) |
| **7. Refrigeration & Food** | Soft Ice Cream Dispenser, Chiller Unit | Cylinder Viscosity (`%`) | E-Stop, Hopper Level, Handle Microswitch | Compressor Contactor, Hopper Agitator, Beater Motor | Beater motor overcurrent trip |

---

## 4. Visual Design System & Apple HIG Reference Gallery

The platform includes 7 comprehensive reference visual mockups stored in `assets/images/`:

| Mockup # | Screen / Option | Image Reference | Key Apple HIG Standards |
| :---: | :--- | :---: | :--- |
| **1** | **Main Operation Dashboard** | ![Main Dashboard](./assets/images/hmi_main_dashboard.png) | Dark Matte Slate (`#0A0C10`), Centered Hero Metric, PackML Status Badge, F1..F5 Footer. |
| **2** | **Parameters & Steppers** | ![Settings Steppers](./assets/images/hmi_settings_steppers.png) | 16px Breathing Margin between `+` and title, 44px touch affordance, Recipe pill badges. |
| **3** | **5-Stage Day-Lock Modal** | ![Day-Lock Modal](./assets/images/day_lock_leasing_ui.png) | Chained countdown, Jalali Solar Hijri date, 4-digit dynamic challenge seed, PIN keypad. |
| **4** | **Hardware I/O Diagnostics** | ![I/O Diagnostics](./assets/images/io_diagnostics_screen.png) | 14px circular neon green `#22C55E` lamps, active-LOW relay toggles, software status mask. |
| **5** | **Mobile WebApp Portal** | ![Mobile WebApp](./assets/images/mobile_webapp_portal.png) | Single-file GZIP (<16KB), Captive Portal 204 handler, 250ms WebSocket real-time telemetry. |
| **6** | **Alarms & RTC Blackbox** | ![Alarms Blackbox](./assets/images/hmi_alarm_blackbox.png) | ISA-18.2 3-tier alarms (Critical Red, Warning Amber), ACK button, RTC Fast SRAM dump. |
| **7** | **Touchless Front-Panel Nav** | ![Touchless Navigation](./assets/images/touchless_mode_navigation.png) | 2px Apple Blue (`#007AFF`) focus ring, 3-state `MODE` key traversal, 10-key tactile ladder. |
| **8** | **Cloud OTA & Desktop Release** | ![Cloud OTA & Desktop Release](./assets/images/ota_server_and_desktop_packaging.png) | Mandatory Zero-Desktop Release (release/ folder) (`lastfile` + `firmware.bin`), 8 sections, zero buzzer, M0..M7 channels, ArvanCloud S3. |
| **9** | **FreeRTOS Dual-Core Segregation**| ![FreeRTOS Dual-Core Architecture](./assets/images/freertos_dual_core_architecture.png) | Core 1 Deterministic 50ms PLC task, Core 0 Apple HIG UI + WebPortal, Mutex isolation. |


---

## 5. Master Engineering Architecture Visual References

The kit provides 4 authoritative engineering architecture schematics in `assets/images/` for instant visual comprehension:

### 1. Hardware Architecture & Dual-Sensor Resistor Divider Circuit
![Hardware Pinout & Dual Sensor Circuit](./assets/images/hardware_pinout_and_dual_sensor_circuit.png)
*Figure 1: ESP32-S3 ETV3A Controller Pinout (Relays Q0..Q9, Dual-Role STEP/DIR/EN) and 2-Reed-Switch Resistor Divider Circuit on GPIO 4, 5, 6 with Analog Voltage Bands.*

### 2. Deterministic FreeRTOS Dual-Core Task Segregation & Memory Map
![FreeRTOS Dual-Core Architecture](./assets/images/freertos_dual_core_architecture.png)
*Figure 2: Deterministic Core 1 50ms Real-Time Scan Cycle vs. Core 0 Apple HIG UI & Connectivity Engine, with DMA SRAM, External PSRAM, and Octal Flash Layout.*

### 3. 5-Stage Cascading Day-Lock Timeline & Phone Challenge-Response
![Day-Lock Timeline & Challenge](./assets/images/day_lock_timeline_and_challenge.png)
*Figure 3: Sequential Leasing Timeline (D1..D5), Monotonic DS3231 Anti-Tamper Check, Jalali Calendar Conversion, and 4-Digit Phone Challenge-Response Unlock Flow.*

### 4. 100% Touchless Front-Panel Navigation FSM & Tactile Key Layout
![Touchless Navigation FSM](./assets/images/touchless_navigation_fsm.png)
*Figure 4: 10 Front-Panel Keys (F1..F5 & Process Keys) with the 3-State Navigation FSM (IDLE, ACTIVE with Cyan Ring, KEYBOARD with Virtual Keypad).*
