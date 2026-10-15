# 🚀 Turnkey Universal AI Agent Prompt for New Industrial Machines (English)
## Haffler Autonomous Machine Synthesis & Zero-to-Production Master Prompt

---

```markdown
Hello AI Agent! I want to architect and build a brand-new, turnkey industrial ESP32-S3 PLC + Apple HIG HMI project for Haffler Automation.

📌 Project Name: [PROJECT_NAME_HERE, e.g. Edge_Bander, Powder_Packaging, or Stone_Saw]
📌 Machine Archetype & Concept: [Brief description of machine mechanics, e.g. Automatic 4-station PVC Edge Bander]

📋 Hardware & Process Specifications:
1. Relay Outputs (Active-LOW, Q0 to Q5):
   - Q0: [e.g. Infeed Conveyor Motor Contactor]
   - Q1: [e.g. Tape Guillotine Cylinder Solenoid]
   - Q2: [e.g. End Trimming Saw High-Speed Spindle]
   - Q3: [e.g. Glue Pot Applicator Roller Solenoid]
   - Q4: [e.g. Pneumatic Air Blast Cleaning Solenoid]
   - Q5: [e.g. Glue Pot Heating Element PID SSR]
2. Digital Inputs (Active-LOW, I0 to I7):
   - I0: [e.g. Emergency Stop Pushbutton NC]
   - I1: [e.g. Workpiece Entry Optical Sensor NO]
   - I2: [e.g. Guillotine Retracted Limit Reed Switch]
   - I3: [e.g. Guillotine Extended Limit Reed Switch]
   - I4: [e.g. Operator Cycle Start Foot Pedal NO]
   - I5: [e.g. Compressed Air Pressure Switch OK]
   - I6: [e.g. Safety Hood Enclosure Closed Interlock]
   - I7: [e.g. Conveyor Inverter Fault Trip NC]
3. Analog / Temperature / Pulse Sensors:
   - [e.g. Glue pot K-type thermocouple MAX6675 via SPI CS=GPIO 21]
   - [e.g. Optical pulse wheel / encoder for tape length measurement]
4. Primary Hero Metric on HMI Screen:
   - [e.g. GLUE POT TEMPERATURE: 190.0 °C, and Total Meters Applied: 1,420 m]
5. Special Operational Constraints:
   - [e.g. Interlock preventing conveyor start until glue temperature reaches 180°C, 180s anti-short cycle, etc.]

---

🎯 Your Autonomous Directive as Haffler Senior Project Lead:
1. Take 100% autonomous ownership per Haffler_PLC_Universal_Agent_Kit_EN and AGENTS.md.
2. Bootstrap the project codebase using `tools/scaffold_new_machine.py`.
3. Implement deterministic Siemens PackML FSM on Core 0 (50ms scan cycle) with robust timeout handling.
4. Design the Apple HIG dark matte HMI (#334155, 1px border, 44px touch targets, zero software shadows, mathematical centering via lv_obj_center) on Core 1.
5. Enforce MISRA C:2012 safety standards (zero dynamic memory allocation in loops, explicit fixed-width types, NULL validation).
6. Build a Trilingual Zero-Tofu dictionary (Persian, English, Turkish).
7. Configure the 5-stage Day-Lock leasing subsystem with monotonic DS3231 RTC protection.
8. Package the OTA release in `release/` (firmware.bin and 8-section lastfile with NO buzzer pin).
9. Audit the codebase with `tools/haffler_safety_linter.py` and `tools/haffler_swarm.py --batch-audit`, delivering a 0-violation proof.

⛔ Non-Negotiable Red Lines:
- Zero Desktop Policy: NEVER write files to `C:\Users\Haffler\Desktop`.
- Flashing Prohibition: NEVER flash physical hardware without explicit keyword «فلش کن».
- Wrap all Persian explanations in `<div dir="rtl" style="text-align: right;">...</div>`.
```
