# Claude Code Directive — Haffler Universal Industrial PLC + HMI Platform

> **Target Platform:** HAFFLER ET-V3A Industrial PLC + HMI Controller (ESP32-S3 N16R8, 16MB Flash, 8MB PSRAM, 480x320 IPS Touchscreen, 10 Active-LOW Relays, 8 Optocoupled Inputs, Dual SSRs, RS485 Modbus).  
> **Master Directives:** Consult [`AGENTS.md`](./AGENTS.md) and [`33_MASTER_KNOWLEDGE_ROUTING_AND_DECISION_INDEX.md`](./33_MASTER_KNOWLEDGE_ROUTING_AND_DECISION_INDEX.md).

---

## 1. Top 4 Non-Negotiable Operating Laws

1. **Ultra-Brevity in Reports (گزارش‌دهی فوق‌العاده خلاصه):**
   - Zero pleasantries, zero fluff, zero question repetition.
   - Purely telegraphic bullet points, completion % in Persian, affected file paths, and decisive choices.
2. **Deep Persian Workshop Intent Parsing:**
   - *"خفنش کن"* = Apple HIG dark slate (`#0A0C10`/`#161B22`), 1px borders, 44px touch targets, zero software shadows.
   - *"صنعتیش کن"* = Siemens determinism, 50ms scan cycle on Core 1, PackML FSM, safety interlocks, ISA-18.2 alarms.
   - *"گیر میکنه / هنگ میکنه"* = Actuator travel timeout, debounce filter, or I2C bus lockup.
   - *"لقی داره"* = Leadscrew backlash; apply unilateral compensation.
   - *"فایل برای سرور بده"* = Package in `<project>/release/` and deploy to ArvanCloud S3 (`ota-plc`) under Zero-Desktop policy.
3. **Flashing Prohibition (ممنوعیت فلش سخت‌افزار بدون اجازه صریح):**
   - **RED LINE:** Never execute `pio run -t upload`, `esptool.py`, or `upload.bat` unless the user explicitly inputs: **"فلش کن"**, **"پروگرم کن"**, or **"آپلود کن"**. Compilation and simulation are allowed.
4. **Strict Zero-Desktop Policy (ممنوعیت مطلق دسکتاپ):**
   - **RED LINE:** Never output or copy any file to `C:\Users\Haffler\Desktop`. Output strictly in `<project>/release/` (`lastfile` + `firmware.bin`).

---

## 2. Core Architecture Rules

- **Deterministic Segregation:** Core 1 runs 50ms PLC engine (`task_plc_engine`). Core 0 runs LVGL UI and WebApp (`task_ui_engine`).
- **Boot Clamping:** Drive all relays HIGH ($1=\text{OFF}$) in the first line of `setup()`.
- **Zero Software Shadows:** `shadow_width = 0` on all LVGL objects to ensure 50+ FPS.
- **BiDi Colon Ban:** NEVER use `%02d:%02d` inside Persian strings! Use descriptive Persian or Unicode LRO locks.
- **Web DRAM Floor:** Guarantee free internal SRAM stays $>40\text{ KB}$ at all times.

---

## 3. Essential Verification Commands

```powershell
# Check all 30 industrial gates
python tools/verify_project_compliance.py .

# Run sequence simulation
python tools/simulate_machine.py --test-all

# Generate Zero-Desktop Cloud Release
python tools/gen_lastfile.py --out release --ver 1.0.0 --bin .pio/build/esp32-s3-devkitc-1/firmware.bin
```
