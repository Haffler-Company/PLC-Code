# HAFFLER SOFT ICE CREAM MACHINE — MASTER KNOWLEDGE BASE & ARCHITECTURE SPEC
**Model:** Haffler / ASTIAJ CM-700  
**Controller Hardware:** ESP32-S3-WROOM-1 (N16R8 — 16MB Quad SPI Flash, 8MB Octal PSRAM)  
**Display & Touch:** 3.5" ILI9488 Parallel 8080 8-bit (480x320) + Goodix GT911 Capacitive Touch (I2C)  
**Firmware Architecture:** ESP-IDF v5.5.5 / FreeRTOS Dual-Core SMP / LVGL 8.3.x  

---

## 1. HARDWARE PINOUT & ELECTRICAL SPECIFICATION

### 1.1 Actuators & Relay Outputs (Active-LOW, 0 = ON, 1 = OFF)
> [!IMPORTANT]
> All outputs must be pulled HIGH before configuring as OUTPUT during boot sequence (`outputs_init()`) to prevent relay glitching during reset.

| Signal | GPIO | Type | Function & Process Description |
|---|---|---|---|
| **Q0** | GPIO 16 | Relay (Active-LOW) | **Compressor Small Tank:** Pre-cooling hopper storage (Maintains mix at +4°C). |
| **Q1** | GPIO 42 | Relay (Active-LOW) | **Compressor Main Cylinder:** Deep freezing barrel refrigeration. Hard-locked with **180-second Anti-Short-Cycle** timer. |
| **Q2** | GPIO 39 | Relay (Active-LOW) | **Motor Agitator (Beater):** Cylinder internal auger/stirrer. Monitored by I0 inductive sensor. |
| **Q3** | GPIO 48 | Relay (Active-LOW) | **Pump Cream Nozzle:** Dispensing assistance / syrup injection pump. |
| **Q4** | GPIO 40 | Relay (Active-LOW) | **Pump Rainbow Hole:** Color/syrup stripe injection pump. |
| **Q5** | GPIO 38 | SSR (Active-LOW) | **Air Valve Nozzle:** Overrun air aeration solenoid valve with high-frequency PWM/duty capability. |
| **Relay** | GPIO 18 | Relay (Active-LOW) | **Power Board Reset:** 500ms active pulse to reset secondary peripheral power boards. |
| **Buzzer** | GPIO 45 | Push-Pull Output | **Piezo Buzzer (24V Driver):** Acoustic feedback for touch clicks, process transitions, and alarm warnings. |

### 1.2 Digital Inputs (Active-LOW with Internal Pull-Ups)
| Signal | GPIO | Function | Debounce / Filtering |
|---|---|---|---|
| **I0** | GPIO 6 | **Inductive Pulley Sensor:** Measures motor shaft rotation. Calculates mix viscosity and detects mechanical stall. | Hardware interrupt + 50ms software window |
| **I2** | GPIO 5 | **Rear Clamp Microswitch:** Physical limit switch detecting high mechanical hardness of product. | 100ms debounce filter |
| **I4** | GPIO 4 | **Front Clamp Microswitch:** Dispense lever switch; counts delivered ice cream cones. | 150ms debounce filter |
| **I6** | GPIO 3 | **E-Stop Button:** Emergency stop. Immediate interrupt triggering `plc_outputs_all_off()` in <20ms. | Interrupt-driven, latching |
| **I7** | GPIO 7 | **Cycle Start Button:** Physical front-panel push button to initiate auto-cooling cycle. | 50ms debounce filter |

### 1.3 Communication Buses & Sensors
- **Capacitive Touch (GT911):** I2C bus on **SDA = GPIO 8**, **SCL = GPIO 9**.
  - *Anti-Freeze Mandate:* Register `0x814E` must be cleared with `0x00` immediately following every touch reading.
- **Thermocouple SPI (MAX6675 / MAX31865):**
  - **MISO:** GPIO 37
  - **SCK:** GPIO 47
  - **T1_CS (Hopper Storage Temperature):** GPIO 21
  - **T2_CS (Cylinder Freezing Temperature):** GPIO 0

---

## 2. DUAL-CORE FREERTOS ARCHITECTURE & TASK PARTITIONING

```
           +-------------------------------------------------------+
           |              ESP32-S3 Dual-Core SMP                   |
           +---------------------------+---------------------------+
           |          CORE 0           |          CORE 1           |
           | (Comms, Web, OTA, Cloud)  |  (PLC Control, UI, Touch) |
           +---------------------------+---------------------------+
           | • WiFi / Captive Portal   | • plc_control_task (50ms) |
           | • HTTP Web Server (REST)  | • lvgl_render_task (60FPS)|
           | • WebSocket SCADA Stream  | • gt911_touch_driver      |
           | • Day-Lock Verification   | • Sensor Acquisition (SPI)|
           | • ArvanCloud OTA Agent    | • Active-LOW Relay Matrix |
           +---------------------------+---------------------------+
```

### 2.1 Core Separation Rules
1. **Core 1 Isolation:** The display engine and PLC process scan loop reside exclusively on Core 1 to ensure zero frame stutter and zero jitter on critical actuator timings.
2. **Core 0 Communication:** All TCP/IP, mDNS, HTTP requests, JSON parsing, and cryptographic operations (SHA-256 for Day-Lock / OTA) execute on Core 0.
3. **Cross-Core Mutex Synchronization:**
   - GUI operations from background events must acquire `lvgl_lock(pdMS_TO_TICKS(50))`.
   - PLC state updates to web portal use atomic structs or shallow message queues.

---

## 3. FINITE STATE MACHINE (FSM) & SAFETY INTERLOCKS

### 3.1 States
- `STATE_IDLE`: Ready, compressors off, monitoring mix temperatures.
- `STATE_AUTO_COOLING`: Main refrigeration active (Q1 on after 180s delay, Q2 agitator running).
- `STATE_MAINTAIN`: Hopper cooling active (Q0 cycling) to hold product between 2°C and 4°C.
- `STATE_DISPENSING`: Dispense lever opened (I4 triggered); Q3/Q4 injection and Q5 air overrun active.
- `STATE_CLEANING`: High-speed agitation with heating/water rinse; refrigeration locked out.
- `STATE_DEFROST`: Hot gas bypass or controlled thaw cycle.
- `STATE_LOCKED`: Day-Lock lease expired; full UI lock with challenge screen.
- `STATE_EMERGENCY_STOP`: E-stop (I6) depressed or fatal sensor fault; all relays shut down within 20ms.

### 3.2 Non-Negotiable Safety Interlocks
1. **180-Second Anti-Short-Cycle:** Compressor Q1 must never be started if fewer than 180 seconds have elapsed since its previous shutdown. Protects hermetic motor windings against high head pressure.
2. **Agitator Stall Protection:** If motor Q2 is commanded ON, at least one pulse from I0 must arrive every 4 seconds. Failure trips `ALARM_E01_STALL` and halts Q1 and Q2.
3. **Low-Temperature Cutoff:** If Cylinder Temperature (T2) drops below `-22.0°C`, all refrigeration immediately ceases to prevent freeze-up fracture of auger and gearbox.

---

## 4. DAY-LOCK INDUSTRIAL LEASING ENGINE

The machine incorporates a tamper-resistant leasing controller:
- **Monotonic Clock:** Tracks elapsed operating hours and RTC timestamps via NVS.
- **5-Digit Challenge Response:**
  ```python
  base = challenge * 2
  checksum = sum(int(d) for d in str(base)) % 10
  unlock_code = f"{base}{checksum}"
  ```
- **4-Digit Manager Challenge Response:**
  ```python
  # Multiplies each digit by 3, adds 1-based index, modulo 10
  unlock_code = "".join(str(((int(c) * 3) + (i + 1)) % 10) for i, c in enumerate(f"{chal:04d}"))
  ```
- **Factory Master Override PINs:** `12773`, `123456`, `12733`, `1127730`
- **Emergency Grace PIN:** `112233` (Grants 10-day emergency operation).

---

## 5. APPLE-GRADE LVGL HMI DESIGN SYSTEM

- **Resolution:** 480 × 320 Landscape.
- **Theme:** Dark Slate Industrial Luxury (`#0F172A` background, `#1E293B` cards, `#0284C7` Apple Cyan accents).
- **Zero Software Shadows:** `shadow_width = 0` across all styles to maintain steady 60 FPS on ESP32-S3.
- **Touch Ergonomics:** All touch targets expanded to at least 44 × 44 pixels (`lv_obj_set_ext_click_area`).
- **Trilingual Dictionary:** Seamless zero-tofu rendering in Persian (RTL), English, and Turkish.
- **Dirty-Driven UI:** Labels and bars update only when value differs from previous frame (`cur != last`).

---

## 6. DEVELOPER & AGENT COMMAND CENTER

All operational commands have been consolidated into `tools/haffler_icecream_cli.py`:

```powershell
# 1. Inspect complete machine & firmware health:
python tools/haffler_icecream_cli.py status

# 2. View colorized pinout & interlocks:
python tools/haffler_icecream_cli.py pins

# 3. High-velocity Ninja build (<20 seconds):
python tools/haffler_icecream_cli.py build

# 4. Execute 4-tier industrial test matrix:
python tools/haffler_icecream_cli.py test

# 5. Calculate Day-Lock challenge responses:
python tools/haffler_icecream_cli.py daylock --challenge 48192

# 6. Launch browser-based Web SCADA simulator:
python tools/haffler_icecream_cli.py sim

# 7. Synchronize AST knowledge graph:
python tools/haffler_icecream_cli.py sync-graph
```

---

## 7. WEB PORTAL SCADA & REST API SPECIFICATION

The onboard web application (`tools/web_portal.html`) communicates with the ESP32-S3 HTTP daemon:

| Endpoint | Method | Payload / Description |
|---|---|---|
| `/api/status` | GET | Comprehensive live telemetry (temps, currents, FSM state, timers, I/O states). |
| `/api/sysinfo` | GET | Chip model, ESP-IDF version, free heap, uptime, flash size. |
| `/api/faults` | GET | Active alarm list and historical fault log. |
| `/api/faults/clear` | POST | Clears acknowledged non-latched alarms. |
| `/api/cmd` | POST | `{"cmd": "make"|"wash"|"hold"|"pdl"|"drain"|"stop"}` Process FSM triggers. |
| `/api/settings` | POST | `{"visc": 75, "pdl_run": 10, "pdl_pause": 25, "buzzer_vol": 80}` Saves NVS configuration. |
| `/api/sales` | POST | `{"sold": 150, "total_cones": 12450}` Cone counter accounting records. |
| `/api/rtc` | POST | `{"year": 2026, "month": 9, "day": 30, "hour": 6, "min": 45}` Sets DS3231 RTC hardware clock. |
| `/api/daylock/unlock` | POST | `{"pin": "123456"}` Sends lease renewal PIN to unlock machine. |
| `/api/wifi/toggle` | POST | `{"enable": true/false}` Enables/disables WiFi transceiver. |
| `/api/wifi/ap` | POST | `{"ssid": "...", "pass": "..."}` Reconfigures SoftAP credentials. |

---

## 8. RELATED DOCUMENTATION & SKILLS
- **Developer Experience & Pitfalls:** [`docs/DEVELOPER_EXPERIENCE_AND_PITFALLS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/ICE%20CREEAM/docs/DEVELOPER_EXPERIENCE_AND_PITFALLS.md)
- **Haffler Ice Cream Master Skill:** [`.agents/skills/haffler-icecream-master/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/ICE%20CREEAM/.agents/skills/haffler-icecream-master/SKILL.md)
- **Engineering Rules:** [`.agents/rules/icecream_engineering.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/ICE%20CREEAM/.agents/rules/icecream_engineering.md)

