#pragma once
#include <stdint.h>

/**
 * @file haffler_pins.h
 * @brief Haffler Industrial ESP32-S3 Hardware Pinout Definitions (HAFFLER ET-V3A)
 * @details Universal Single Source of Truth for all machine variants.
 * 
 * Target Hardware: HAFFLER ET-V3A Integrated PLC + HMI Controller
 *
 * UNIVERSAL HARDWARE CHARACTERISTICS:
 * - 10 Active-LOW Power Outputs (Q0..Q9: 0 = ON/Energized, 1 = OFF/De-energized)
 * - Q0, Q1, Q2: High-Speed Pulse/Direction/Enable or 2A Digital Drivers
 * - Q5, Q8: Dedicated Onboard 10A Solid-State Relays (SSRs) for PID Heaters / Fast Switching
 * - Q3, Q4, Q6, Q7, Q9: 24V DC / 2A Active-LOW Solenoid & Contactor Drivers
 * - 8 Optocoupled Digital Inputs (I0..I7: Active-LOW, 24V tolerant with internal pull-up)
 * - Dual K-Type Thermocouples (T1, T2 via MAX6675 SPI: 0°C to 1024°C)
 * - I2C0 Bus (GT911 Touch + DS3231 RTC)
 * - SPI2 Bus (480x320 TFT LCD)
 * - RS485 Bus (Modbus RTU / Inverter / Printer)
 */

// ==============================================================================
// 1. FRONT-PANEL KEYPAD ADC CHANNELS (ADC1 ONLY - IMMUNE TO WI-FI)
// ==============================================================================
#define PIN_KEY_F                 1   // ADC1_CH0 - Shortcut Keys F1..F5
#define PIN_KEY_M                 2   // ADC1_CH1 - Process Keys (SAVE, CANCEL, MODE, START, STOP)

// Resistor Ladder Calibrated ADC Centers (12-bit ADC: 0..4095)
#define KEYPAD_ADC_CENTER_1       4050
#define KEYPAD_ADC_CENTER_2       3350
#define KEYPAD_ADC_CENTER_3       2700
#define KEYPAD_ADC_CENTER_4       1950
#define KEYPAD_ADC_CENTER_5       1250
#define KEYPAD_PRIMARY_WINDOW     360
#define KEYPAD_SEPARATION_MARGIN  170
#define KEYPAD_DEBOUNCE_MS        45

// ==============================================================================
// 2. INDUSTRIAL DIGITAL & ANALOG INPUTS (ACTIVE-LOW, 24V OPTO-ISOLATED)
// ==============================================================================
// Standard Discrete / Dedicated Digital Inputs
#define PIN_EMERGENCY             3   // Input I6 - Mushroom E-Stop Button (NC, Safety Critical <20ms)
#define PIN_START_PEDAL           7   // Input I7 - Cycle Start Pushbutton / Foot Pedal (NO)

// Universal Optocoupled Inputs / Dual-Sensor Analog Band Channels
#define PIN_INPUT_I0              6   // GPIO 6: Input I0 (Discrete Sensor or Reed Pair Low Band)
#define PIN_INPUT_I1              6   // GPIO 6: Input I1 (Reed Pair High Band)
#define PIN_INPUT_I2              5   // GPIO 5: Input I2 (Discrete Sensor or Reed Pair Low Band)
#define PIN_INPUT_I3              5   // GPIO 5: Input I3 (Reed Pair High Band)
#define PIN_INPUT_I4              4   // GPIO 4: Input I4 (Discrete Sensor or Reed Pair Low Band)
#define PIN_INPUT_I5              4   // GPIO 4: Input I5 (Reed Pair High Band)
#define PIN_INPUT_I6              PIN_EMERGENCY   // GPIO 3: Input I6 (E-Stop NC / Safety Trip)
#define PIN_INPUT_I7              PIN_START_PEDAL // GPIO 7: Input I7 (Start Command / Cycle Trigger)

// Universal Sensor Analog Voltage Band Thresholds (ADC mV readings if using 2-wire dividers)
#define SENSOR_BAND_OPEN_LO       1300 // Retracted / Open: 1300..1800 mV
#define SENSOR_BAND_OPEN_HI       1800
#define SENSOR_BAND_NONE_LO       1850 // In-Transit Deadband (Neither Active): 1850..2240 mV
#define SENSOR_BAND_NONE_HI       2240
#define SENSOR_BAND_BOTH_LO       2350 // Both Sensors Active / Table Return: 2350..2750 mV
#define SENSOR_BAND_BOTH_HI       2750
#define SENSOR_BAND_CLOSED        3500 // Extended / Closed: > 3500 mV (up to 4095 mV)
#define SENSOR_TIMEOUT_DEFAULT_MS 5000 // Universal Movement Watchdog Alarm Timeout

// Auxiliary Digital / Analog Inputs (Optional)
#define PIN_AUX_IN_1              21  // Auxiliary Input 1 (Note: Shared with TC1_CS)
#define PIN_AUX_IN_2              37  // Auxiliary Input 2 (Note: Shared with TC_MISO)

// ==============================================================================
// 3. I2C INDUSTRIAL BUS (GT911 TOUCH + DS3231 RTC)
// ==============================================================================
#define PIN_I2C_SDA               8    // Hardware I2C0 Data
#define PIN_I2C_SCL               9    // Hardware I2C0 Clock
#define DS3231_I2C_ADDR           0x68 // TCXO Industrial RTC Address
#define GT911_I2C_ADDR            0x5D // Capacitive Touch Address (Alternative: 0x14)

// ==============================================================================
// 4. SPI2 / FSPI HIGH-SPEED DISPLAY BUS (480x320 IPS TFT)
// ==============================================================================
#define PIN_LCD_CS                13  // Chip Select
#define PIN_LCD_DC                14  // Data / Command Select
#define PIN_LCD_MOSI              10  // Master Out Slave In
#define PIN_LCD_SCK               12  // SPI Serial Clock (up to 80MHz)
#define PIN_LCD_MISO              11  // Master In Slave Out
#define PIN_LCD_RST               (-1)// Hardware Reset tied to EN/RC
#define PIN_LCD_BL                (-1)// Backlight tied to 3.3V plane

// ==============================================================================
// 5. UNIVERSAL INDUSTRIAL RELAY OUTPUTS (ACTIVE-LOW: 0 = ON, 1 = OFF)
// ==============================================================================
// Authoritative HAFFLER ET-V3A Hardware Pin Mapping
#define PIN_RELAY_Q0              16  // Output Q0 (Pulse / High-Speed Driver / Contactor)
#define PIN_RELAY_Q1              42  // Output Q1 (Direction / Digital Driver 1)
#define PIN_RELAY_Q2              39  // Output Q2 (Enable / Digital Driver 2)
#define PIN_RELAY_Q3              48  // Output Q3 (2A Solenoid / Valve / Motor Contactor)
#define PIN_RELAY_Q4              40  // Output Q4 (2A Solenoid / Valve / Actuator)
#define PIN_RELAY_Q5              15  // Output Q5 (Onboard 10A Solid-State Relay 1 - SSR PWM)
#define PIN_RELAY_Q6              35  // Output Q6 (2A Contactor / Motor / Pump / Auxiliary)
#define PIN_RELAY_Q7              36  // Output Q7 (2A Contactor / Fan / Blower / Vacuum)
#define PIN_RELAY_Q8              38  // Output Q8 (Onboard 10A Solid-State Relay 2 - SSR PWM / Spare)
#define PIN_RELAY_Q9              41  // Output Q9 (2A Alarm Stack Light / Valve / Spare)

// High-Speed Stepper / Servo Motion Aliases (Shared on Terminals Q0, Q1, Q2)
#define PIN_STEP_PUL              PIN_RELAY_Q0 // GPIO 16 - Pulse Output (PCNT/LEDC/RMT)
#define PIN_STEP_DIR              PIN_RELAY_Q1 // GPIO 42 - Direction Output
#define PIN_STEP_EN               PIN_RELAY_Q2 // GPIO 39 - Driver Enable Output

// ==============================================================================
// 6. DUAL THERMOCOUPLE SPI BUS (MAX6675 K-TYPE THERMOCOUPLES: 0°C TO 1024°C)
// ==============================================================================
#define PIN_THERMO_CS1            21  // Thermocouple 1 Chip Select (Zone 1 / Head 1)
#define PIN_THERMO_CS2            0   // Thermocouple 2 Chip Select (Zone 2 / Head 2)
#define PIN_THERMO_SCK            47  // Dedicated Thermocouple SPI Clock Bus
#define PIN_THERMO_MISO           37  // Dedicated Thermocouple SPI Data Out (SO)

// ==============================================================================
// 7. RS485 INDUSTRIAL FIELDBUS & TELEMETRY SERIAL
// ==============================================================================
#define PIN_RS485_TX              43  // UART2 TX / Alternative UART0
#define PIN_RS485_RX              44  // UART2 RX / Alternative UART0
#define PIN_RS485_DE              46  // RS485 Direction Control (DE/RE)
#define PIN_RS485_DIR             PIN_RS485_DE // Direction Control Alias

// ==============================================================================
// 8. ACOUSTIC BUZZER (LEDC HARDWARE PWM TIMER)
// ==============================================================================
#define PIN_BUZZER                45  // Piezo Buzzer (LEDC Channel)
