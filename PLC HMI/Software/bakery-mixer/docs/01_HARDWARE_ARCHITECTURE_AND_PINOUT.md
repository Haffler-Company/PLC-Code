# Industrial Hardware Architecture & Definitive Pinout Matrix
## ESP32-S3 SoC Architecture, Dual-Core Partitioning, Active-LOW Relays, Boot Safe Clamping, and Pin Reassignment Rules

> **Document ID:** `01_HARDWARE_ARCHITECTURE_AND_PINOUT.md`  
> **Target Hardware:** Haffler Universal Integrated Industrial PLC + HMI Controller (ESP32-S3).  
> **Standard Reference:** Siemens Industrial PLC Architecture (IEC 61131-3) & Haffler Master Hardware Specifications.

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

// Default hardware pins
#define DEFAULT_TABLE        16  // Q0
#define DEFAULT_PRESS_LEFT   42  // Q1
#define DEFAULT_PRESS_RIGHT  39  // Q2
#define DEFAULT_ELEMENT      48  // Q3
#define DEFAULT_SHABLON      40  // Q4
#define DEFAULT_SSR_1        15  // Q5 (SSR)
#define DEFAULT_ZARBEH1      35  // Q6
#define DEFAULT_ZARBEH2      36  // Q7
#define DEFAULT_SPARE_Q8     38  // Q8 (Unassigned Spare)
#define DEFAULT_SPARE_Q9     41  // Q9 (Unassigned Spare)

// Runtime dynamic pins (loaded from NVS before setupPin())
extern int TABLE;
extern int PRESS_LEFT;
extern int PRESS_RIGHT;
extern int ELEMENT;
extern int SHABLON;
extern int SSR_1;
extern int ZARBEH1;
extern int ZARBEH2;
```

### 5.2. Field Recovery Workflow via HMI Dialog
1. If a relay (e.g. `Q1` - Press Left) burns out, the technician enters **Menu $\rightarrow$ Information $\rightarrow$ Pin Remapping** (protected by Technician PIN `1234`).
2. The technician selects `Press Left` and rebinds it to spare terminal `Q8 (GPIO 38)` or `Q9 (GPIO 41)`.
3. The new assignment is committed to NVS (`storage.savePinAssignment("PRESS_LEFT", 38)`).
4. Upon the next cycle or reboot, the system drives the new physical pin seamlessly. Zero code modification or firmware reflashing is required.

### 5.3. Strictly Forbidden Pins
- Octal SPI flash/PSRAM pins (`GPIO 26` through `GPIO 36`) must NEVER be touched under any circumstances (causes immediate MCU panic).
- Display SPI bus (`GPIO 10, 11, 12, 13, 14`) and primary I2C bus (`GPIO 8, 9`).
- ADC1 Keypad Ladder pins (`GPIO 1, 2`): If reassigned, new pins must belong to ADC1 (ADC2 channels are disabled whenever Wi-Fi is active).
- Virtual auxiliary pins `M0..M7` are preserved in software registries for auxiliary field equipment.
