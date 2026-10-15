# 08. Cloud OTA Deployment, Lastfile Specification, and Rollup Changelogs

## Executive Summary
Field upgrades across hundreds of industrial machines must be reliable, bandwidth-efficient, and transparent to the factory operator. Haffler controllers utilize a centralized S3-compatible cloud repository on ArvanCloud (`https://ota.haffler-infi.ir`), strict 2-file release packaging, hardware I/O pinout declarations, and cumulative multilingual rollup changelogs.

---

## 1. The Strict Zero-Desktop & Direct Cloud Deployment Mandate

Whenever a firmware release is generated (via the Persian trigger *"فایل برای سرور بده"* or English *"Give server release files"*), the agent must build the target binary and output **strictly to the project's internal `release/` folder**:

1. `<project_root>/release/lastfile` (Metadata manifest conforming to the 8-section standard)
2. `<project_root>/release/firmware.bin` (Compiled target binary)

> **CRITICAL ZERO-DESKTOP RULE:** Under no circumstances may any file (binary, manifest, script, or temporary archive) be placed on the operator's Desktop (`C:\Users\Haffler\Desktop`). The desktop must remain 100% clean. All releases must be published directly to the ArvanCloud S3 repository (`ota-plc`) via `tools/haffler_ota_publisher.py`.

![OTA Server Deployment & Cloud Release Packaging](./assets/images/ota_server_and_desktop_packaging.png)

---

## 2. Definitive `lastfile` Specification (8 Sections)

The `lastfile` is an INI-style metadata manifest that the PLC downloads over HTTPS to evaluate compatibility, display localized changelogs, and verify binary integrity before initiating flash writing.

### Mandatory 8 Sections Structure
```ini
[company]
name=Haffler Automation Co.
brand=Haffler

[version]
version=V.1.8.2
build=20260912
min_supported_hw=REV_C
size_bytes=1441792
sha256=a1b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0

[binary]
file=firmware.bin
url=https://ota.haffler-infi.ir/<category>/<project-name>/firmware.bin

[description_fa]
1=بهینه‌سازی سرعت رندرینگ و حذف لگ صفحات نمایشگر
2=تصحیح تراز ریاضی ارقام و کادرهای تنظیمات
3=ارتقای پایداری ماشین وضعیت و تایم‌اوت‌های ایمنی

[description_en]
1=Optimized UI rendering speed with zero frame drops
2=Corrected mathematical centering of all numeric values
3=Enhanced deterministic FSM safety and actuator timeouts

[description_tr]
1=Ekran olusturma hizi optimize edildi ve gecikmeler giderildi
2=Sayisal degerlerin matematiksel hizalamasi duzeltildi
3=FSM guvenligi ve calisma zaman asimi kontrolleri artirildi

[inputs]
I0=Optical_Sensor_Infeed (NO)
I1=Position_Home_Switch (NO)
I2=Pressure_Safety_Switch (NC)
I3=Fluid_Level_OK_Float (NO)
I4=Safety_Guard_Interlock (NC)
I5=Thermal_Overload_Trip (NC)
I6=Emergency_Stop_Pushbutton (NC)
I7=Cycle_Start_Pedal (NO)

[outputs]
Q0=Primary_Actuator_Pulse_Or_Contactor
Q1=Direction_Control_Or_Digital_Output_1
Q2=Driver_Enable_Or_Digital_Output_2
Q3=Pneumatic_Valve_Advance_Solenoid
Q4=Pneumatic_Valve_Retract_Solenoid
Q5=Solid_State_Relay_1_PWM_Heater
Q6=Auxiliary_Motor_Or_Pump_Contactor
Q7=Cooling_Fan_Or_Vacuum_Blower
Q8=Solid_State_Relay_2_PWM_Heater
Q9=Stack_Light_Alarm_Beacon
M0=Auxiliary Air Booster Pump
M1=Chip Extraction Collector Starter
M2=Bar Feeder Synchronization Pulse
M3=Pneumatic Unloader Ejector
M4=Reserved Safety Channel A
M5=Reserved Safety Channel B
M6=External Warning Siren Relay
M7=Factory MES Production Pulse
```

### The Buzzer Exclusion Mandate
- **NEVER list the buzzer (`BZ`) under `[outputs]`:** The buzzer is driven by internal PWM (`GPIO 45`) via the LEDC peripheral. It is not an external industrial relay and must never be exposed as an output channel.
- **Mandatory M0..M7 Auxiliary Channels:** All 8 software-mapped auxiliary channels (`M0` through `M7`) must be documented in `[outputs]` to allow field technicians to map auxiliary external equipment (e.g. vacuum dust extractors, chip collectors, bar feeders).

---

## 3. Cumulative Multilingual Rollup Changelog Engine

In industrial environments, a machine might jump multiple software revisions during an upgrade (e.g., from `V.1.2.0` directly to `V.1.8.2`). If the HMI only showed the single change in `V.1.8.2`, the factory owner would miss critical operational improvements introduced in intermediate releases.

### How Rollup Operates:
1. The HMI parses the installed firmware version from local flash (`V_CURRENT = V.1.2.0`).
2. The HMI parses the server manifest (`V_TARGET = V.1.8.2`).
3. The OTA subsystem iterates through all intermediate entries stored in the remote changelog database and concatenates the bullet points for the currently selected operator language:
   $$\text{RollupNotes} = \sum_{v = V_{\text{CURRENT}}+1}^{V_{\text{TARGET}}} \text{Notes}(v, \text{Lang})$$
4. **Zero Tofu Guarantee:** All text is filtered for unsupported Unicode glyphs to prevent square box (`[]`) rendering on custom LVGL fonts.
5. **Length Cap:** Every changelog bullet must be **at most 13 words** for rapid comprehension on 4-inch screens.

---

## 4. ArvanCloud S3 Cloud Deployment

Haffler distributes firmware via ArvanCloud Object Storage (S3-compatible bucket `ota-plc`).

### Bucket Directory Structure
```
ota-plc/
  ├── upvc/
  │   ├── welding-single-head/
  │   │   ├── lastfile
  │   │   └── firmware.bin
  │   ├── welding-double-head/
  │   │   ├── lastfile
  │   │   └── firmware.bin
  │   ├── cutter-single-head/
  │   │   ├── lastfile
  │   │   └── firmware.bin
  │   └── corner-cleaning/
  │       ├── lastfile
  │       └── firmware.bin
  └── food/
      └── ice-cream-dispenser/
          ├── lastfile
          └── firmware.bin
```

---

## 5. Automated 4-Step Health Verification Protocol

Prior to confirming any deployment to the client, the automated script `tools/verify_ota.py` executes 4 rigorous checks against the live CDN URL:

1. **Step 1: HTTP 200 OK:** Performs an HTTP `HEAD` request to verify the file is publicly accessible (`public-read`) and DNS resolves properly.
2. **Step 2: Content-Length Match:** Compares remote HTTP `Content-Length` header byte-for-byte against the compiled local file size (`size_bytes`).
3. **Step 3: Cryptographic SHA-256 Validation:** Downloads the remote binary stream, computes its SHA-256 hash, and verifies zero bit-flips against the local binary hash.
4. **Step 4: Green PASS Verification Report:** Emits a concise tabular audit report.

```
+-------------------+------------------------------------------+--------+
| CHECK STAGE       | EXPECTED VALUE                           | STATUS |
+-------------------+------------------------------------------+--------+
| HTTP Response     | 200 OK                                   | PASS   |
| Remote File Size  | 1,441,792 Bytes                          | PASS   |
| SHA-256 Checksum  | a1b2c3d4e5f60718293a4b5c...             | PASS   |
| S3 Readability    | public-read (Anonymous Access)           | PASS   |
+-------------------+------------------------------------------+--------+
| RESULT: ALL OTA HEALTH GATES PASSED (100% READY FOR PLC OTA)         |
+-----------------------------------------------------------------------+
```

---

## 6. Technician Boot Key F5 & OTA Card Segregation Mandate

To safeguard factory machines from accidentally loading unreleased test builds while allowing service engineers to deploy diagnostic packages:

### 6.1. Dual Boot Modes
1. **Technician Mode (Boot Holding Key F5):**
   - If physical front-panel key **F5** is held during power-on, the bootloader engages `is_f5_boot_active() == true`.
   - The OTA update screen displays **both cards**:
     1. Production Firmware Card (e.g. `upvc/welding-single-head`)
     2. Diagnostic / Config-Test Card (e.g. `upvc/config-test`)
2. **Normal Operator Mode (Standard Boot):**
   - When accessed normally through **Menu $\rightarrow$ Information $\rightarrow$ Firmware Update**:
   - Exactly **one card** is instantiated (`cards_to_show = 1`).
   - The test/config card is completely omitted from the LVGL widget tree and excluded from network polling.
   - Dual-layer callback interlocks block any unauthorized execution of non-production builds.

### 6.2. On-Premise Manual Link Entry Panel
For facilities operating in air-gapped environments without cloud access:
- A dedicated manual link entry modal (`link_entry_panel`) allows typing a local LAN IP or custom URL (e.g. `http://192.168.1.100/firmware.bin`).
- The link entry keyboard utilizes locked vertical offset `KB_Y_OFF_LINK_ENTRY = -4` to guarantee clean icon rendering.
