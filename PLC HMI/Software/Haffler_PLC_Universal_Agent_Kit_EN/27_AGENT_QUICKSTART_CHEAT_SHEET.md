# 27. AI Agent Fast-Track Operational Playbook (1-Page Cheat Sheet)
## How to Bootstrap, Develop, and Deliver Any Industrial Machine in 5 Steps with Zero Friction

> **Document ID:** `27_AGENT_QUICKSTART_CHEAT_SHEET.md`  
> **Audience:** Autonomous AI Coding Agents (Antigravity, Gemini, Claude, Cursor, Copilot).  
> **Mission:** Deliver a 100% production-ready, compiling, tested machine firmware + Apple HIG HMI with zero hand-holding and zero user friction.

---

## The 5-Step Autonomous Execution Flow

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant Agent as AI Agent (Lead Engineer)
    participant Scaffolder as tools/scaffold_new_machine.py
    participant Compiler as PlatformIO (Core 1 + Core 0)
    participant Release as tools/gen_lastfile.py

    User->>Agent: "Project: [NAME], Scenario: [BRIEF HINTS]"
    Note over Agent: 1. Greet: Ask ONLY for Name + Hints.<br/>Once provided: ZERO follow-up questions!
    Agent->>Scaffolder: Run scaffolding CLI with archetype type
    Scaffolder-->>Agent: Compilable project with FSM & Apple HIG UI
    Agent->>Agent: Wire specific sensors, actuators & timers
    Agent->>Compiler: Run pio run (verify 0 errors, DRAM > 45KB)
    Agent->>Release: Generate release/lastfile + release/firmware.bin
    Agent->>User: Telegraphic Persian completion report (%)
```

---

### Step 1: Output the Standard Greeting (and Nothing Else)
When initialized or invited to a project, greet with the exact standard prompt from `00_AUTONOMOUS_PROJECT_LEAD_DIRECTIVE.md`:
```
Hello! I am your Senior Industrial PLC & HMI Lead Engineer.

Please provide:
1. Project / Machine Name (e.g., Edge_Bander, PVC_Double_Cutter, Commercial_Bakery_Oven)
2. Brief Scenario Outline & Machine Requirements (e.g., actuators, sensors, speed/temperature targets, special options)

Once you provide these, I will autonomously research, architect the FSM, design the Apple HIG HMI, configure safety interlocks, implement day-locks and web portals, run automated tests, and deliver a production-ready package with zero friction.
```

---

### Step 2: Match Archetype & Run Scaffolding CLI
Analyze the user's scenario keywords using `21_UNIVERSAL_MACHINE_RECIPES_AND_PRESETS.md`:
- "اره" / "برش" / "cutter" $\rightarrow$ `cutting`
- "جوش" / "welder" $\rightarrow$ `welding`
- "گاز" / "آرگون" / "gas" $\rightarrow$ `dispensing`
- "لبه چسبان" / "edge bander" $\rightarrow$ `edge_banding`
- "تمیزکن" / "گوشه" / "corner clean" $\rightarrow$ `milling`
- "کپی روتر" / "router" $\rightarrow$ `copy_router`
- "فر" / "قنادی" / "bakery" $\rightarrow$ `bakery`
- "بستنی" / "ice cream" $\rightarrow$ `refrigeration`

Run the automated scaffolder CLI:
```bash
python tools/scaffold_new_machine.py --name "Edge_Bander" --type "edge_banding" --dir "../Edge_Bander"
```
This generates the full compilable repository with all drivers, FSM, UI screens, day-locks, and web portal in 3 seconds.

---

### Step 3: Wire Machine Specifics (Zero Guesswork)
1. Open `include/haffler_pins.h`: verify specific sensor assignments ($I_0..I_7$) and relay assignments ($Q_0..Q_9$).
2. Open `src/haffler_packml_fsm.cpp`: set state dwell times (e.g., clamp dwell, blade stroke timeout).
3. Open `src/ui_screen_dashboard.cpp`: verify Hero Metric label (`mm`, `°C`, `%`, `RPM`) and ensure mathematical centering (`pad_all = 0`, `lv_obj_center()`).
4. Open `src/ui_screen_settings.cpp`: verify 16px breathing margins for steppers.

---

### Step 4: Verification & Automated Compliance Audit
Run the automated verification suite:
```bash
# 1. Compile firmware
pio run

# 2. Run 30-gate industrial compliance auditor
python tools/verify_project_compliance.py .
```
Verify that all gates pass (`[PASS]`), free DRAM is $> 45\text{ KB}$, and no compiler warnings are emitted.

---

### Step 5: Cloud-Only Release & Telegraphic Report (Zero-Desktop)
Generate the release package inside `<project>/release/`:
```bash
python tools/gen_lastfile.py --version V.1.0.0 --bin .pio/build/esp32-s3-wroom-1-n16r8/firmware.bin --out release/
```
Verify that **strictly 2 files** reside in `<project>/release/`:
- `release/lastfile`
- `release/firmware.bin`
*(Confirm Desktop `C:\Users\Haffler\Desktop` is 100% clean).*
Deploy directly to ArvanCloud S3 bucket `ota-plc` using `python tools/haffler_ota_publisher.py --publish <project>`.

Reply to the user in ultra-concise, telegraphic Persian with:
- Completion percentage (`۱۰۰٪`)
- List of generated files
- Summary of verified features