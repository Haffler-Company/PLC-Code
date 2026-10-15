# Industrial Hardware Architecture & Definitive Pinout Matrix
## ESP32-S3 SoC Architecture, Physical Dimensions, Terminal Mapping, Boot Safe Clamping, and Pin Reassignment Rules

> **Document ID:** `01_HARDWARE_ARCHITECTURE_AND_PINOUT.md`  
> **Target Hardware:** HAFFLER ET-V3A Universal Integrated Industrial PLC + HMI Controller (ESP32-S3).  
> **Standard Reference:** Siemens Industrial PLC Architecture (IEC 61131-3) & Haffler Master Hardware Specifications.

---

## 0. Physical Hardware Showcase, Dimensions & Electrical Interface

The **HAFFLER ET-V3A** is an integrated industrial PLC + HMI controller engineered specifically for harsh workshop environments (metalworking, UPVC welding, cutting, and automation machinery). It combines a 4.0-inch IPS color touchscreen, 10 front tactile membrane buttons, high-isolation optocoupled I/O, dual 220V/10A SSR outputs, and multi-protocol industrial communication in a powder-coated antistatic metal enclosure.

### 0.1. Physical Controller Showcase
| Front Panel & 10 Tactile Keys | Isometric Panel Enclosure |
| :---: | :---: |
| ![HAFFLER ET-V3A Front Panel](./assets/images/haffler_et_v3a_front_panel.jpg) | ![HAFFLER ET-V3A Perspective](./assets/images/haffler_et_v3a_perspective.jpg) |
| *Front View: 4.0" LCD + 10 Physical Keys (F1..F5 Softkeys, SAVE, CANCEL, MODE, START, STOP)* | *Antistatic Metal Chassis with Panel Retaining Clamp Slots* |

| Rear Terminal Strip & Safety Ground | Physical Front Panel CAD Layout |
| :---: | :---: |
| ![Rear Terminals & Fuses](./assets/images/haffler_et_v3a_rear_terminals.jpg) | ![Front Panel CAD Layout](./assets/images/haffler_et_v3a_panel_cad_layout.png) |
| *Pluggable Screw Terminals, Dual 10A SSR Fuses, 1A PLC Fuse & Earth Ground Post* | *Front Panel Vector CAD Layout with Softkey Alignment Guidelines* |

| Mechanical CAD Dimensions & Cutout | Complete Pinout & Wiring Table |
| :---: | :---: |
| ![Mechanical Dimensions](./assets/images/haffler_et_v3a_mechanical_dimensions.png) | ![Pinout & Wiring Diagram](./assets/images/haffler_et_v3a_pinout_and_wiring_diagram.png) |
| *Front Bezel (141.40 × 74.80 mm), Cutout (131.90 × 65.45 mm), Depth (76.20 mm)* | *Complete 25-Pin Screw Terminal Assignments and Side Connectors* |

| Port Locations & Interface Diagram | Real-Time I/O Diagnostic LEDs |
| :---: | :---: |
| ![Port Locations](./assets/images/haffler_et_v3a_port_locations_diagram.png) | ![LED Status Indicators](./assets/images/haffler_et_v3a_led_status_indicators.png) |
| *RS485 Side Port, Dual USB (Programming & Host), High-Power SSR Connectors* | *LED Monitoring: Power (Green), Outputs Q0..Q9 (Red), Inputs I0..I7 (Green)* |

---

### 0.2. Mechanical & Environmental Specifications
* **Front Bezel Outer Dimensions:** 141.40 mm (W) × 74.80 mm (H) × 4.0 mm flange thickness (Overall front: 145 mm × 74.5 mm)
* **Cabinet Panel Cutout Requirement:** 131.90 mm (W) × 65.45 mm (H)
* **Overall Controller Depth:** 76.20 mm (chassis body) / 93.0 mm (including rear pluggable terminals and fuse caps)
* **Chassis Material:** Antistatic powder-coated industrial metal housing with top and bottom ventilation grilles
* **Mounting Mechanism:** Dual side screw clamp brackets for vibration-resistant flush cabinet mounting
* **Unit Weight:** 800 g
* **Operating Temperature:** -10 °C to +80 °C
* **Storage Temperature:** -20 °C to +85 °C

---

### 0.3. Electrical Power Supply & Safety Protocols
The HAFFLER ET-V3A incorporates an internal dual AC/DC power supply:
* **AC Mains Input:** 200V – 240V AC (50/60 Hz), 800 mA AC max (Nominal: 0.5A)
* **DC Power Input:** 24V DC ± 10%, 2A (up to 4A peak DC)
* **Total Power Consumption:** 115 W peak (under full relay and SSR trigger load)
* **Main PLC Fuse:** 1A Fast-Blow (5×20 mm cartridge on right side panel)
* **SSR Fuses:** Two 10A Industrial Ceramic Fuses (rear panel twist-lock holders)

> [!CAUTION]
> **CRITICAL HARDWARE WIRING RULES:**
> 1. **CONNECT ONLY ONE POWER SOURCE:** Never connect both 220V AC and 24V DC simultaneously. Doing so will permanently destroy the internal power stage.
> 2. **MANDATORY PROTECTIVE EARTH (PE):** The yellow/green Earth wire MUST be bolted securely to the machine frame / cabinet PE ground lug to prevent electrostatic buildup and ground-loop noise.

---

### 0.4. Complete 25-Terminal Rear Connector Strip Matrix

```
+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
|                                                                                 HAFFLER ET-V3A REAR TERMINAL CONNECTOR STRIP                                                                          |
+------+-----------+-------------------------+-------------------------------------------------------------+------------------------------------------------------------------------------------+
| Term | Label     | Hardware Designation    | Electrical Specification                                    | Universal Industrial Electrical Assignment & Capabilities                          |
+------+-----------+-------------------------+-------------------------------------------------------------+------------------------------------------------------------------------------------+
| 1    | N         | AC Neutral Input        | 200-240 VAC Neutral                                         | Factory 220V Mains Neutral Line                                                    |
| 2    | L         | AC Live Input           | 200-240 VAC Live (Fused 1A)                                 | Factory 220V Mains Phase Line                                                      |
| 3    | +24V      | 24V DC Auxiliary Out    | +24V DC Out, Max 3.0A                                       | Field Sensors, Inductive Proximity Switches, Optical Beams, Solenoid Common        |
| 4    | GND       | DC System Ground        | 0V DC Common Reference                                      | Common return for 24V field sensors and DC power supply                            |
| 5    | Q0        | Transistor Output 0     | 24V DC Open-Collector (2A) / High-Speed Pulse               | Fast Actuator Pulse (STEP_PUL, Dosing Pulse, VFD Speed) or Contactor/Solenoid 0    |
| 6    | Q1        | Transistor Output 1     | 24V DC Open-Collector (2A) / Direction                      | Direction Control (STEP_DIR, Reverse Interlock) or Digital Power Output 1          |
| 7    | Q2        | Transistor Output 2     | 24V DC Open-Collector (2A) / Enable                         | Motion Driver Enable (STEP_EN) or Digital Power Output 2                           |
| 8    | Q3        | Transistor Output 3     | 24V DC Open-Collector (2A) / Active-LOW                     | Pneumatic/Hydraulic Valve Advance, Contactor, Pump, or Motor Starter               |
| 9    | Q4        | Transistor Output 4     | 24V DC Open-Collector (2A) / Active-LOW                     | Pneumatic/Hydraulic Valve Retract, Dosing Solenoid, or Position Actuator          |
| 10   | Q5        | Transistor Output 5     | 24V DC Open-Collector (2A) / SSR1 Trigger                   | Onboard Solid-State Relay 1 (High-Duty Cycle PID Heating Element, Fast AC Solenoid)|
| 11   | Q6        | Transistor Output 6     | 24V DC Open-Collector (2A) / Active-LOW                     | Auxiliary Motor Starter, Agitator, Circulation Pump, or Tool Actuator              |
| 12   | Q7        | Transistor Output 7     | 24V DC Open-Collector (2A) / Active-LOW                     | Cooling Fan, Blower, Exhaust Damper, Vacuum Pump, or Compressor Starter            |
| 13   | Q8        | Transistor Output 8     | 24V DC Open-Collector (2A) / SSR2 Trigger                   | Onboard Solid-State Relay 2 (Secondary Heating Element, Fast Solenoid) or Spare    |
| 14   | Q9        | Transistor Output 9     | 24V DC Open-Collector (2A) / Active-LOW                     | Industrial Stack Light Alarm Beacon, Acoustic Horn, Work Illuminator, or Spare    |
| 15   | I0        | Opto Input 0            | 24V DC Optocoupled / Analog Band                            | Infeed Presence Sensor, Axis Home Prox, or Cylinder Retracted Reed Switch Pair 1    |
| 16   | I1        | Opto Input 1            | 24V DC Optocoupled / Analog Band                            | Infeed Position OK, Pulse Flowmeter, or Cylinder Extended Reed Switch Pair 1       |
| 17   | I2        | Opto Input 2            | 24V DC Optocoupled / Analog Band                            | Process Pressure OK Switch, Vacuum Threshold, or Cylinder Reed Switch Pair 2       |
| 18   | I3        | Opto Input 3            | 24V DC Optocoupled / Digital                                | Fluid Level OK / Low Float Switch, Hopper Empty, or Thermal Overload Trip          |
| 19   | I4        | Opto Input 4            | 24V DC Optocoupled / Digital                                | Safety Enclosure Guard Interlock, Light Curtain OSSD, or Tool Position Interlock   |
| 20   | I5        | Opto Input 5            | 24V DC Optocoupled / Digital                                | Operator Enable Switch, Foot Jog, or Auxiliary Diagnostic Sensor                   |
| 21   | I6        | Opto Input 6 (STOP)     | 24V DC Optocoupled / Digital                                | Physical Emergency Stop (E-Stop) NC Pushbutton (<20ms Instant Trip / Pause)         |
| 22   | I7        | Opto Input 7 (START)    | 24V DC Optocoupled / Digital                                | Operator Cycle Start Pushbutton / Pneumatic Foot Pedal Trigger                      |
| 23   | COM-      | Digital Inputs Common    | 0V DC Return for Optocouplers                                | Common ground return for optocoupled inputs I0..I7                                  |
| 24   | T2+       | Thermocouple 2 Positive | MAX6675 SPI K-Type (+)                                      | K-Type Thermocouple Channel 2 (+) (Zone 2, 0°C to +1024°C, 0.25°C resolution)       |
| 25   | T1+       | Thermocouple 1 Positive | MAX6675 SPI K-Type (+)                                      | K-Type Thermocouple Channel 1 (+) (Zone 1, 0°C to +1024°C, 0.25°C resolution)       |
+------+-----------+-------------------------+-------------------------------------------------------------+------------------------------------------------------------------------------------+
```

---

### 0.5. Side Panel Interfaces & Expansion Connectors
* **Right Side Panel:**
  * **RS485 Connector (3-Pin Terminal):** `A` (Data + / Non-inverting), `B` (Data - / Inverting), `GND` (Isolated Ground Reference). Used for VFD Inverter control (Modbus RTU) and external ESC/POS thermal printers.
  * **USB 2 (Standard USB Type-A):** Host / Diagnostic peripheral connection.
  * **Main Power Fuse (1A):** Protection for the low-voltage logic and ESP32-S3 motherboard.
* **Left Side Panel:**
  * **USB 1 (Micro-USB / Type-C):** Direct UART programming port (CP2102) for firmware flashing, headless serial telemetry (`921600` baud), and screen capture via `SHOT`.
  * **M1..M4 Expansion Module Connector (4-Pin):** Dedicated virtual auxiliary channels reserved for plug-in telemetry and sensor modules.
  * **SSR In / Out (High-Power Red Pluggable Connector):** 220V AC / 10A Solid-State Relay switched power contacts for direct heavy heating element or high-power AC load driving without external contactors.

---

### 0.6. Front-Panel Keypad & Visual Alignment
The front panel provides **10 industrial tactile buttons** divided into two columns:
* **Left Column (Softkeys F1..F5):**
  * Aligned directly via printed dashed guide lines with the left softkey buttons of the 480×320 LCD screen.
  * Electrically connected to `GPIO 1` (ADC1_CH0) via a precision calibrated resistor ladder.
  * Pressing `F1`..`F5` immediately navigates to the corresponding functional screen / tab without needing touch input.
* **Right Column (Global Navigation & Control):**
  * **`SAVE`:** Saves all modified machine parameters directly to persistent NVS flash.
  * **`CANCEL`:** Dismisses modals, closes keyboards, or navigates back one menu level.
  * **`MODE`:** Cycles active focus through on-screen widgets (Apple HIG blue focus ring `#007AFF`).
  * **`START` (Green with ▲ Icon):** Starts machine cycle or increments values in edit mode.
  * **`STOP` (Red with ▼ Icon):** Halts machine cycle or decrements values in edit mode.
  * Electrically connected to `GPIO 2` (ADC1_CH1) via a precision calibrated resistor ladder.

---

### 0.7. Factory Network & Web Application Default Settings
* **Integrated Wi-Fi:** 2.4 GHz 802.11 b/g/n SoftAP + Station mode.
* **Default SoftAP SSID:** `HAFFLER_ET_V3A` (or device model string).
* **Default SoftAP Password:** `haffler.com`
* **Default IP Address:** `192.168.4.1` (Mobile captive portal automatically redirects to the embedded WebApp dashboard).
* **GSM Cellular Support:** Integrated hardware footprint for remote IoT telemetry and SMS alerts.

---

## 1. Central Processing Unit & Memory Specifications

All modern Haffler industrial machines are built upon the **ESP32-S3-WROOM-1 (N16R8)** system-on-chip:

```
+---------------------------------------------------------------------------------------+
|                                ESP32-S3-WROOM-1 (N16R8)                               |
+----------------------+----------------------------------------------------------------+
| Technical Property   | Engineering Specification                                      |
+----------------------+----------------------------------------------------------------+
| Processor Cores      | Dual-core 32-bit Xtensa® LX7 up to 240 MHz with Vector DSP/AI  |
| Flash Memory (ROM)   | 16 MB Octal SPI Flash with dual A/B OTA partitions (app0/app1) |
| External RAM (PSRAM) | 8 MB Octal SPI PSRAM at 80 MHz (UI buffers, web assets)        |
| Internal SRAM        | 512 KB SRAM (>40 KB must remain free for TCP/IP network stack) |
| Wireless Interfaces  | Wi-Fi 2.4 GHz (802.11 b/g/n, +19.5 dBm) + Bluetooth BLE 5.0    |
| Real-Time Clock (RTC)| External Dallas DS3231 I2C with TCXO temp-compensation + CR1220|
+----------------------+----------------------------------------------------------------+
```

### 1.1. Deterministic Dual-Core Task Segregation
To ensure zero frame drops in graphics and zero jitter in industrial machine control, responsibilities are strictly divided:
* **Core 0 (System, Connectivity & Web):**
  - LVGL graphics rendering task (20ms to 30ms period).
  - Goodix GT911 capacitive touch polling (400 kHz I2C).
  - Internal web server (`esp_http_server` / WebApp) and real-time WebSocket telemetry.
  - Automatic cloud OTA update client.
  - Front-panel 10-key keypad scanner and event queue dispatcher.
* **Core 1 (Deterministic Industrial PLC Engine):**
  - Machine control scan cycle (rigid 50ms to 70ms deterministic scan).
  - Multi-sample debouncing of 8 optocoupled industrial inputs.
  - PackML standard state machine (FSM) execution.
  - PID thermal regulation loop via dual MAX6675 SPI thermocouples and SSR PWM.
  - Hardware E-Stop monitor and relay output drivers.
  - **GOLDEN RULE:** Never call LVGL APIs or blocking I/O (flash writes, long serial prints) from Core 1.

---

## 2. Definitive Hardware Pinout Matrix

```
![Hardware Pinout & Dual Sensor Circuit](./assets/images/hardware_pinout_and_dual_sensor_circuit.png)
*Figure 1.1: Authoritative HAFFLER ETV3A Hardware Pinout Matrix and Dual-Sensor Resistor Divider Circuit.*

```
+-----------------------------------------------------------------------------------------------+
|                             HAFFLER CONTROLLER PIN ALLOCATION TABLE                           |
+---------------+---------------+-----------------------+---------------------------------------+
| Pin Name      | GPIO Number   | Signal Type           | Functional Assignment                 |
+---------------+---------------+-----------------------+---------------------------------------+
| KEY_F         | GPIO 1        | Analog Input (ADC1_0) | Ladder for front keys F1..F5          |
| KEY_M         | GPIO 2        | Analog Input (ADC1_1) | Ladder for keys F6..F10 & MODE        |
| START1        | GPIO 3        | Digital Input (Opto)  | Physical Start Button 1 (or E-Stop)   |
| START2        | GPIO 7        | Digital Input (Opto)  | Physical Start Button 2 / Pedal       |
| INP_FAULT     | GPIO 4        | Analog Band (ADC1_3)  | Dual Sensor Pair (Retracted / Ext)    |
| INP_GUARD     | GPIO 5        | Analog Band (ADC1_4)  | Dual Sensor Pair (Retracted / Ext)    |
| INP_REF       | GPIO 6        | Analog Band (ADC1_5)  | Dual Sensor Pair (Retracted / Ext)    |
| TOUCH_SDA     | GPIO 8        | I2C0 Hardware Bus     | GT911 Touch Data                      |
| TOUCH_SCL     | GPIO 9        | I2C0 Hardware Bus     | GT911 Touch Clock (400kHz)            |
| RTC_SDA/SCL   | GPIO 8 / 9    | Shared I2C0 Bus       | DS3231 RTC (0x68) & MCP23017 (0x20)   |
| LCD_MOSI      | GPIO 10       | SPI2 (FSPI) Bus       | TFT LCD Display Data                  |
| LCD_MISO      | GPIO 11       | SPI2 (FSPI) Bus       | TFT LCD Display Master In (or NC)     |
| LCD_SCK       | GPIO 12       | SPI2 (FSPI) Bus       | TFT LCD Display Clock (up to 80MHz)   |
| LCD_CS        | GPIO 13       | Digital Output        | TFT Chip Select                       |
| LCD_DC        | GPIO 14       | Digital Output        | TFT Data/Command Select               |
| LCD_RST       | -1 (RC Reset) | Hardware Reset        | Connected to MCU Hardware Reset RC    |
| Q0 / STEP_PUL | GPIO 16       | Active-LOW / Pulse    | Relay Q0 (or Stepper Pulse on Cutter) |
| Q1 / STEP_DIR | GPIO 42       | Active-LOW / Dir      | Relay Q1 (or Stepper Dir on Cutter)   |
| Q2 / STEP_EN  | GPIO 39       | Active-LOW / Enable   | Relay Q2 (or Stepper Enable on Cutter)|
| RELAY_Q3      | GPIO 48       | Active-LOW Output     | Relay Q3 (Pneumatic Solenoid / Valve) |
| RELAY_Q4      | GPIO 40       | Active-LOW Output     | Relay Q4 (Pneumatic Solenoid / Valve) |
| SSR_Q5        | GPIO 15       | Digital Output (PWM)  | High-Speed SSR 1 (Heater / Spindle)   |
| RELAY_Q6      | GPIO 35       | Active-LOW Output     | Relay Q6 (Aux Contactor / Coolant)    |
| RELAY_Q7      | GPIO 36       | Active-LOW Output     | Relay Q7 (Aux Contactor / Vacuum)     |
| RELAY_Q8      | GPIO 38       | Active-LOW Output     | Relay Q8 (Spare Terminal for Remap)   |
| RELAY_Q9      | GPIO 41       | Active-LOW Output     | Relay Q9 (Spare Terminal for Remap)   |
| BUZZER        | GPIO 45       | Piezo Output (LEDC)   | Acoustic Feedback & Alarms            |
| RS485_TX      | GPIO 43 (TXD2)| Hardware UART2        | RS485 / Thermal Printer Transmit      |
| RS485_RX      | GPIO 44 (RXD2)| Hardware UART2        | RS485 / Thermal Printer Receive       |
| RS485_DE      | GPIO 46       | Digital Output        | RS485 Transmit Direction (DE/RE)      |
| TC1_CS        | GPIO 21       | Dedicated SPI CS      | MAX6675 Thermocouple 1 CS             |
| TC2_CS        | GPIO 0        | Dedicated SPI CS      | MAX6675 Thermocouple 2 CS (Head 2)    |
| TC_SCK        | GPIO 47       | Dedicated SPI Bus     | Dual MAX6675 Clock Bus                |
| TC_SO         | GPIO 37       | Dedicated SPI Bus     | Dual MAX6675 Data Out (MISO)          |
| UART0_TX      | GPIO 43 (TXD0)| Hardware UART0        | CP2102 Serial Telemetry (921600 baud) |
| UART0_RX      | GPIO 44 (RXD0)| Hardware UART0        | CP2102 SHOT Framebuffer Capture       |
+---------------+---------------+-----------------------+---------------------------------------+
```

> **CRITICAL BOOT CONFIGURATION:** In `platformio.ini`, flash mode must be configured as `board_build.flash_mode = dio` and `memory_type = dio_qspi`. Do **NOT** use `qio_opi` as it triggers immediate bootloader crash loops on the HT-V3A motherboard.

---

## 3. Relay Outputs & Boot Zero-Glitch Clamping

### 3.1. Active-LOW Industrial Logic
All relay drivers on Haffler boards utilize active-LOW logic:
* **Logic Level LOW (`0`):** Coil energized $\rightarrow$ Relay contact closed (ON).
* **Logic Level HIGH (`1`):** Coil de-energized $\rightarrow$ Relay contact open (OFF).

### 3.2. Mandatory Boot Zero-Glitch Clamping Routine
To prevent relay chatter, contactor arcing, and unintended pneumatic actuator motion during the initial boot milliseconds before FreeRTOS starts, the output pin levels **MUST be clamped to 1 before setting direction**:

```c
// Mandatory in the very first line of setup() / app_main():
void hal_relays_boot_clamp(void) {
    const uint8_t relay_pins[] = {
        PIN_RELAY_Q0, PIN_RELAY_Q1, PIN_RELAY_Q2, PIN_RELAY_Q3,
        PIN_RELAY_Q4, PIN_RELAY_Q5, PIN_RELAY_Q6, PIN_RELAY_Q7,
        PIN_RELAY_Q8, PIN_RELAY_Q9
    };
    for (size_t i = 0; i < sizeof(relay_pins); ++i) {
        gpio_set_level((gpio_num_t)relay_pins[i], 1); // Clamp to OFF state first
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << relay_pins[i]),
            .mode = GPIO_MODE_INPUT_OUTPUT,          // Bidirectional mode
            .pull_up_en = GPIO_PULLUP_ENABLE,        // Internal pullup engaged
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
}
```

### 3.3. Solving the ESP32 Output Read Bug (Software Status Mask)
On the ESP32 architecture, calling `gpio_get_level()` on an output pin can return `0` or corrupted states depending on the driver silicon configuration.  
**Definitive Industrial Solution:** Maintain a 16-bit volatile status mask in RAM:

```c
static volatile uint16_t s_active_relay_mask = 0; // Bit i = 1 means Relay i is ON

void relay_set(uint8_t relay_idx, bool state) {
    if (relay_idx >= 10) return;
    gpio_num_t pin = get_relay_pin(relay_idx);

    if (state) {
        gpio_set_level(pin, 0); // Active-LOW: ON
        s_active_relay_mask |= (1 << relay_idx);
    } else {
        gpio_set_level(pin, 1); // Active-LOW: OFF
        s_active_relay_mask &= ~(1 << relay_idx);
    }
}

bool relay_get_state(uint8_t relay_idx) {
    if (relay_idx >= 10) return false;
    return (s_active_relay_mask & (1 << relay_idx)) != 0;
}
```
In diagnostics screens and manual toggle routines, state is read exclusively from `s_active_relay_mask`, ensuring 100% synchronization between UI and hardware.

---

## 4. Acoustic Buzzer Policy

1. The piezo buzzer operates on `GPIO 45` using the LEDC PWM hardware peripheral (frequencies 2700 Hz to 4000 Hz).
2. **Standard Audio Patterns:**
   - **Tactile Touch Click:** Single 15ms pulse at 3500 Hz.
   - **Settings Saved:** Double-beep (`Beep-Beep`) 50ms ON / 50ms OFF / 50ms ON.
   - **Warning / Fault:** Triple 100ms pulses at 2400 Hz.
   - **Installment Lockout:** Continuous 5000ms tone on boot.
3. **Server Release Policy (CRITICAL):** The buzzer is an internal signaling element; **the buzzer (`BZ`) must NEVER be listed in the outputs section of `lastfile`**.

---

## 5. Runtime Dynamic Pin Remapping Subsystem

In harsh workshop environments, inductive spikes or relay coil wear can occasionally damage a single physical relay contact. In conventional PLCs, a damaged output requires discarding or desoldering the entire controller.

### 5.1. Dynamic Pin Variable Architecture
On the Haffler platform, physical output pin assignments are **runtime variables stored in NVS preferences**, initialized with sensible defaults at boot:

```c
#define PIN_NC (-1)

// Default hardware outputs (mapped to physical terminals Q0..Q9)
#define DEFAULT_OUTPUT_Q0    16  // Q0: High-Speed Pulse / Transistor Contactor 0
#define DEFAULT_OUTPUT_Q1    42  // Q1: Direction / Digital Driver 1
#define DEFAULT_OUTPUT_Q2    39  // Q2: Enable / Digital Driver 2
#define DEFAULT_OUTPUT_Q3    48  // Q3: Primary Solenoid / Valve / Pump
#define DEFAULT_OUTPUT_Q4    40  // Q4: Secondary Solenoid / Actuator
#define DEFAULT_SSR_Q5       15  // Q5: Onboard 10A Solid-State Relay 1 (PID Heater / Fast AC)
#define DEFAULT_OUTPUT_Q6    35  // Q6: Motor Contactor / Spindle / Auxiliary
#define DEFAULT_OUTPUT_Q7    36  // Q7: Cooling Fan / Vacuum / Compressor
#define DEFAULT_SSR_Q8       38  // Q8: Onboard 10A Solid-State Relay 2 (PID Heater 2 / Spare)
#define DEFAULT_OUTPUT_Q9    41  // Q9: Alarm Stack Light / Valve / Spare

// Runtime dynamic pins (loaded from NVS preferences before setup)
extern int OUTPUT_Q0;
extern int OUTPUT_Q1;
extern int OUTPUT_Q2;
extern int OUTPUT_Q3;
extern int OUTPUT_Q4;
extern int SSR_Q5;
extern int OUTPUT_Q6;
extern int OUTPUT_Q7;
extern int SSR_Q8;
extern int OUTPUT_Q9;
```

### 5.2. Field Recovery Workflow via HMI Dialog
1. If a physical output contact (e.g. `Q3` - Primary Valve) is electrically damaged in the field, the technician enters **Menu $\rightarrow$ Diagnostics / Pin Remapping** (protected by Technician PIN `1234`).
2. The technician selects `Primary Valve` and rebinds it to spare terminal `Q8 (GPIO 38)` or `Q9 (GPIO 41)`.
3. The new assignment is committed to NVS (`storage.savePinAssignment("OUTPUT_Q3", 38)`).
4. Upon the next cycle or reboot, the system drives the new physical terminal seamlessly without code changes.

### 5.3. Strictly Forbidden Pins
- Octal SPI flash/PSRAM pins (`GPIO 26` through `GPIO 36`) must NEVER be touched under any circumstances (causes immediate MCU panic).
- Display SPI bus (`GPIO 10, 11, 12, 13, 14`) and primary I2C bus (`GPIO 8, 9`).
- ADC1 Keypad Ladder pins (`GPIO 1, 2`): If reassigned, new pins must belong to ADC1 (ADC2 channels are disabled whenever Wi-Fi is active).
- Virtual auxiliary pins `M0..M7` are preserved in software registries for auxiliary field equipment.
