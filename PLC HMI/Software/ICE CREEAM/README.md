# ICE CREAM PLC/HMI — ESP32-S3 (Haffler ASTIAJ)

[![Firmware Verification](https://img.shields.io/badge/Firmware%20Tests-10%2F10%20PASSED-brightgreen.svg)]()
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3--N16R8-blue.svg)]()
[![Display](https://img.shields.io/badge/Display-ILI9488%20480x320%20RGB565-orange.svg)]()
[![Touch](https://img.shields.io/badge/Touch-GT911%20Capacitive-yellow.svg)]()
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20SMP-purple.svg)]()

Industrial PLC and Human-Machine Interface (HMI) firmware for commercial soft-serve ice cream machines, developed for **ASTIAJ Industrial Machinery**.

---

## 1. System Specifications

| Subsystem | Specification / Component | Details |
|---|---|---|
| **MCU** | ESP32-S3-WROOM-1 (N16R8) | Dual-Core Xtensa LX7 @ 240 MHz, 16 MB Octal Flash, 8 MB Octal PSRAM |
| **HMI Screen** | 3.5" IPS LCD (ILI9488) | 480 × 320, 16-bit RGB565LE, Hardware SPI @ 80 MHz, LVGL 9.4 |
| **Touch Panel** | Goodix GT911 Capacitive | Hardware I2C0 @ 400 kHz, DSP noise filter, edge rejection |
| **Real-Time Clock** | Dallas DS3231 | Temperature-compensated RTC, Jalali calendar converter, POSIX clock sync |
| **Thermocouples** | Dual MAX6675 | Direct register bit-bang SPI: Milk Hopper (T1) & Cylinder (T2) |
| **Partition Table** | `partitions_16MB_2x7M_ota.csv` | Dual 7.0 MB OTA application slots, NVS storage |
| **Connectivity** | Wi-Fi 802.11 b/g/n + WebPortal | SoftAP + Station mode, responsive dashboard, OTA updater |
| **Control Timing** | FreeRTOS Multi-Core | Core 0: LVGL HMI & Network; Core 1: 70ms Deterministic PLC Loop |

---

## 2. Hardware I/O Pinout Map

> [!IMPORTANT]
> `main/project_config.h` and `main/extra_modes.h` are the single sources of truth. All relay outputs are **Active-LOW** (`pin_write(pin, 0)` activates, `pin_write(pin, 1)` deactivates).

### 2.1 Relay & Auxiliary Outputs
| Symbol | GPIO | Output Function | Polarity |
|---|---|---|---|
| `Compressor` | **GPIO16** | Small Compressor (Hopper Cooling) | Active-LOW |
| `Make_Icecream` | **GPIO42** | Big Compressor (Cylinder Freezing) | Active-LOW |
| `Paddle` | **GPIO39** | Beater / Auger Motor Contactor | Active-LOW |
| `CREAM_PUMP_GPIO` | **GPIO48** | Edge Syrup Pump (Colored Edge) | Active-LOW |
| `AIR_SOLENOID_GPIO` | **GPIO38** | Air Injection Solenoid Valve | Active-LOW |
| `RAINBOW_GPIO` | **GPIO40** | Rainbow Syrup Valve (Multi-color) | Active-LOW |
| `BUZZER_GPIO` | **GPIO45** | Audio Alarm Buzzer | Active-HIGH |
| `RESET_RELAY_GPIO` | **GPIO18** | Main Control Power Cycle Relay | Active-LOW (500ms pulse) |

### 2.2 Digital & Sensor Inputs
| Symbol | GPIO | Input Function | Electrical Type |
|---|---|---|---|
| `KEY_STOP_GPIO` | **GPIO3** | Panel Emergency Stop Mushroom Button | Active-LOW (Pull-up) |
| `KEY_START_GPIO` | **GPIO7** | Panel Cycle Start Pushbutton | Active-LOW (Pull-up) |
| `MICRO_FRONT` | **GPIO4** | Front Dispense Lever Microswitch | Active-LOW (Pull-up) |
| `MICRO_REAR` | **GPIO5** | Rear Hardness / Tension Microswitch | Active-LOW (Pull-up) |
| `INDUCTIVE_GPIO` | **GPIO6** | Beater Pulley Rotation Sensor | Pulse / Edge Detection |
| `LOAD_MOTOR_GPIO` | **GPIO15** | Beater Motor Overload Trip | Active-LOW (GND = Fault) |
| `LOAD_COMP_GPIO` | **GPIO17** | Compressor Overload Trip | Active-LOW (GND = Fault) |

### 2.3 Buses & Peripherals
| Bus | Signals | Connected Devices |
|---|---|---|
| **SPI2 (LCD)** | MOSI=10, SCLK=12, CS=13, DC=14 | ILI9488 Display Controller |
| **I2C0** | SDA=8, SCL=9 | GT911 Touch Controller & DS3231 RTC |
| **MAX6675 SPI** | MISO=37, SCK=47, CS_TOP=21, CS_CYL=0 | Dual K-Type Thermocouple Converters |
| **USB-JTAG** | USB_D+=20, USB_D-=19 | Secure HMAC-SHA256 Serial HMI & Logger |

---

## 3. Dual-Core Architecture

```
                 ESP32-S3 DUAL-CORE XTENSA LX7
┌──────────────────────────────────────┬──────────────────────────────────────┐
│                CORE 0                │                CORE 1                │
│    (User Interface & Networking)     │      (Hard Real-Time PLC Logic)      │
├──────────────────────────────────────┼──────────────────────────────────────┤
│ • lvgl_handler_task (LVGL 9.4, 25fps)│ • actions_task (70ms Deterministic)  │
│ • ILI9488 SPI DMA Flush Engine       │ • State Machine: Make/Hold/Wash/Edge │
│ • GT911 Touch DSP Filter & Input     │ • Emergency Stop (<70ms Cutoff)      │
│ • DS3231 1Hz Jalali Clock Hook       │ • Microswitch Debounce & Logic       │
│ • Wi-Fi Manager (SoftAP + STA)       │ • Motor & Comp Overload Protection   │
│ • WebPortal HTTP & OTA Server        │ • MAX6675 Dual Probe Poll & Bounds   │
│ • Serial HMI (921.6k HMAC Commander) │ • NVS Debounced Persistence (8s)     │
└──────────────────────────────────────┴──────────────────────────────────────┘
                   ▲                                      ▲
                   └────────── Recursive Mutex ───────────┘
                                (`lvgl_lock`)
```

---

## 4. Software Features & Capabilities

1. **Dual Live Temperature Displays**: Symmetrical capsules flanking the central scoop counter on the main screen show Hopper (Milk T1) and Cylinder (T2) temperatures in real time with sensor disconnect warnings.
2. **Apple Standby Screensaver**: Glassmorphic digital clock, Jalali Persian date, and machine state indicator with automatic wake-on-touch.
3. **Safety Interlocks**:
   - Cylinder Freeze Interlock: Automatically blocks big compressor if cylinder temp $\le -2.0^\circ\text{C}$ in Hold Mode, or triggers hard emergency shutdown if $\le -22.0^\circ\text{C}$.
   - Anti-Short Cycle: Enforces 180s gas pressure stabilization after big compressor shutoff, with live trilingual countdown in HMI monitor.
   - Pulley Inductive Timeout: Shuts down motor if rotation pulses cease for $>4$ seconds, with 4-hour temporary bypass option after 3 strikes.
   - Hopper Empty Protection: Float sensor trips E00 and immediately turns off small compressor to protect dry tank.
4. **Interactive Hardware Diagnostics & Help (Tabs F1, F3, F6)**: Technicians can live-monitor inputs, manually trigger relays, and touch any title to open glassmorphic explanations.
5. **Smart Tiered Fault Advisor**: Tracks fault recurrence in a 1-hour sliding window, escalating persistent faults (3+ strikes) to Tier-2 checklists with factory support hotlines.
6. **Integrated WebPortal**: Connect to the machine's Wi-Fi network to monitor status, view shift counters, chat with the AI assistant, or upload OTA firmware updates.

---

## 5. Toolchain & Simulation

Complete documentation for developer tools is available in [`tools/README.md`](tools/README.md).

```powershell
# Run the 10-test automated offline firmware verification suite
python tools/verify_firmware.py

# Launch the interactive Windows DevPanel simulation
.\tools\DevPanel.bat

# Generate Persian PDF User Manual and Wiring Diagram
python tools/build_user_manual_and_wiring_pdf.py
```

---

## 6. Safety Red Lines & Working Rules

> [!CAUTION]
> 1. **NEVER FLASH HARDWARE** without explicit owner authorization ("تا نگفتم پروگرم نزن").
> 2. Respect `HARDWARE_LOCK.md` (COM8 may be assigned to active factory lines).
> 3. Preserve all verified GPIO pin assignments and Active-LOW polarities.
> 4. Always run `python tools/verify_firmware.py` and `git diff --check` before handoff.
