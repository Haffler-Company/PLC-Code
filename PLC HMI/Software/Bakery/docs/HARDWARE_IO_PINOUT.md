# Hardware Specifications & I/O Pinout — Bakery HMI

This document provides the definitive electrical mapping, GPIO pin assignments, logic levels, sensor interfaces, and schematic details for the ESP32-S3 Bakery Dough Divider controller.

---

## 1. System Electrical Architecture

```text
                                 ┌───────────────────┐
      [24V DC Field Power] ─────►│ Industrial Regs   │────────► 3.3V (ESP32) & 5V (Relays)
                                 └───────────────────┘
                                           │
                                           ▼
┌─────────────────────────┐      ┌───────────────────┐      ┌─────────────────────────┐
│     Digital Inputs      │      │                   │      │     Digital Outputs     │
│ Funnel Sensor  (GPIO 3) ├─────►│                   ├─────►│ Funnel Motor (Q1 GPIO42)│
│ Piston Sensor  (GPIO 7) ├─────►│                   ├─────►│ Piston Motor (Q7 GPIO36)│
│                         │      │     ESP32-S3      ├─────►│ Inverter FWD (Q2 GPIO39)│
│      Analog Inputs      │      │   DevKitC-1       ├─────►│ Inverter REV (Q3 GPIO48)│
│ Keypad F Panel (GPIO 1) ├─────►│                   ├─────►│ Warning Buzzer (GPIO 45)│
│ Keypad M Panel (GPIO 2) ├─────►│                   │      └─────────────────────────┘
└─────────────────────────┘      │                   │
                                 │                   │      ┌─────────────────────────┐
┌─────────────────────────┐      │                   │      │ Display & Peripherals   │
│ Thermocouple 1 (GPIO 4) ├─────►│                   ├─────►│ SPI TFT LCD (10,12,13,14)│
│ Thermocouple 2 (GPIO 5) ├─────►│                   ├─────►│ I2C Cap Touch (8, 9)    │
│ DS3231 RTC     (GPIO 8) ├─────►│                   ├─────►│ Backlight Ctrl (GPIO 38)│
└─────────────────────────┘      └───────────────────┘      └─────────────────────────┘
```

---

## 2. Complete GPIO Allocation Table

| GPIO Pin | Signal Name | Direction | Peripheral Subsystem | Logic Level / Electrical Characteristics |
|---|---|---|---|---|
| **GPIO 1** | `KEY_F` | Input (ADC1_CH0) | F-Panel Keypad (F1..F5) | Analog input via resistor divider (0..3.3V) |
| **GPIO 2** | `KEY_M` | Input (ADC1_CH1) | M-Panel Keypad (M1..M5) | Analog input via resistor divider (0..3.3V) |
| **GPIO 3** | `FUNNELE_SENSOR` | Input | Funnel Retract Home Sensor (I6) | Active Low (LOW = Target Detected) |
| **GPIO 4** | `THERMO_1_CS` | Output | MAX6675 Thermocouple 1 CS | Active Low Chip Select |
| **GPIO 5** | `THERMO_2_CS` | Output | MAX6675 Thermocouple 2 CS | Active Low Chip Select |
| **GPIO 7** | `PISTON_SENSOR` | Input | Piston Top Home Sensor (I7) | Active Low (LOW = Target Detected) |
| **GPIO 8** | `TOUCH_SDA` | Bidirectional | I2C Data (Touch & DS3231 RTC) | External 4.7kΩ pull-up to 3.3V |
| **GPIO 9** | `TOUCH_SCL` | Output | I2C Clock (Touch & DS3231 RTC) | 400 kHz standard fast mode |
| **GPIO 10**| `TFT_MOSI` | Output | Display SPI MOSI | Master Out Slave In (80 MHz) |
| **GPIO 12**| `TFT_SCLK` | Output | Display SPI Clock / Thermocouple SCK | Shared SPI Master Clock |
| **GPIO 13**| `TFT_CS` | Output | Display SPI Chip Select | Active Low |
| **GPIO 14**| `TFT_DC` | Output | Display Data / Command Select | LOW = Command, HIGH = Data |
| **GPIO 25**| `HEAT_SENSOR` | Input (ADC2) | Auxiliary Temperature Input | Reserved Analog Input |
| **GPIO 36**| `PISTON_MOTOR` | Output | Piston Motor Relay Drive (Q7) | Active Low relay output (`ON = LOW`) |
| **GPIO 37**| `TFT_MISO` | Input | Display / Thermocouple SPI MISO | Master In Slave Out |
| **GPIO 38**| `TFT_BL` | Output | LCD Backlight Inverter Control | Active High (HIGH = Backlight ON) |
| **GPIO 39**| `INVERTER_FORWARD` | Output | Inverter Forward Command Relay (Q2)| Active Low relay output (`ON = LOW`) |
| **GPIO 40**| `TOUCH_INT` | Input | Capacitive Touch Interrupt | Active Low interrupt line |
| **GPIO 42**| `FUNNELE_MOTOR`| Output | Funnel Motor Relay Drive (Q1) | Active Low relay output (`ON = LOW`) |
| **GPIO 45**| `BUZZER` | Output | Acoustic Warning Buzzer Drive | Active High transistor drive |
| **GPIO 48**| `INVERTER_REVERSE` | Output | Inverter Reverse Command Relay (Q3)| Active Low relay output (`ON = LOW`) |

---

## 3. Analog Resistor Ladder ADC Threshold Windows

Both `KEY_F` and `KEY_M` panels utilize a precision resistor ladder network sampled by 12-bit ADC (0..4095 range):

| Key Identifier | Raw ADC Window (0..4095) | Nominal Voltage | Assigned Operational Role |
|---|---|---|---|
| **F1 / M1** | 4000 to 5000 | ~3.1V to 3.3V | Auxiliary Command 1 |
| **F2 / M2** | 3300 to 3600 | ~2.6V to 2.8V | Auxiliary Command 2 |
| **F3 / M3** | 2700 to 2900 | ~2.1V to 2.3V | Auxiliary Command 3 |
| **F4 / M4** | 1900 to 2100 | ~1.5V to 1.7V | **Start Automatic Cycle (Start)** |
| **F5 / M5** | 1100 to 1400 | ~0.9V to 1.1V | **Emergency Stop / Abort Cycle (Stop)** |
| **Idle / Released** | < 1000 | < 0.8V | No key pressed |

---

## 4. Electrical Isolation & Field Noise Immunity

1. **Optocoupler Isolation:** All digital output GPIOs are routed through optocouplers (PC817 / TLP521) to isolate high-voltage inductive spikes from relay coils and VFD terminals.
2. **Flyback Diodes:** 1N4007 fast freewheeling diodes are placed anti-parallel across DC relay coils to clamp inductive kickback.
3. **Shielded Sensor Cabling:** Inductive sensor signal lines must use braided shielded pairs grounded exclusively at the main electrical cabinet PE bus bar to prevent false triggers from VFD harmonic emissions.
