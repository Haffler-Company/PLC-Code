#pragma once
#include <stdint.h>

/**
 * @file haffler_pins.h
 * @brief Haffler Industrial ESP32-S3 Hardware Pinout Definitions (HAFFLER ETV3A)
 * @details Single Source of Truth for all machine variants across the Haffler fleet.
 * 
 * Target Hardware: HAFFLER ETV3A Integrated PLC + HMI Controller
 * MCU: ESP32-S3-WROOM-1 (N16R8) - 240MHz Dual-Core, 16MB Flash, 8MB PSRAM
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
// 2. INDUSTRIAL DIGITAL & ANALOG INPUTS
// ==============================================================================
// Dedicated Optocoupled Digital Inputs (Active-LOW, Safe Hardware Pullup)
#define PIN_EMERGENCY             3   // I6 - Mushroom E-Stop Button (NC, Safety Critical)
#define PIN_START_PEDAL           7   // I7 - Cycle Start Foot Pedal / Dual Start Button

// Dual-Sensor Analog Band Channels (2 Reed Switches Encoded per Pin via Resistor Divider)
#define PIN_SENSOR_PAIR_0_1       6   // GPIO 6: I0 = Retracted (Low Band), I1 = Extended (High Band)
#define PIN_SENSOR_PAIR_2_3       5   // GPIO 5: I2 = Retracted (Low Band), I3 = Extended (High Band)
#define PIN_SENSOR_PAIR_4_5       4   // GPIO 4: I4 = Retracted (Low Band), I5 = Extended/Return (High Band)

// Standard Diagnostic & Discrete Input Aliases
#define PIN_START1                PIN_EMERGENCY
#define PIN_START2                PIN_START_PEDAL
#define PIN_INP_FAULT             PIN_SENSOR_PAIR_4_5
#define PIN_INP_GUARD             PIN_SENSOR_PAIR_2_3
#define PIN_INP_REF               PIN_SENSOR_PAIR_0_1

// Sensor Analog Voltage Band Thresholds (ADC mV readings)
#define SENSOR_BAND_OPEN_LO       1300 // Retracted / Open: 1300..1800 mV
#define SENSOR_BAND_OPEN_HI       1800
#define SENSOR_BAND_NONE_LO       1850 // In-Transit Deadband (Neither Active): 1850..2240 mV
#define SENSOR_BAND_NONE_HI       2240
#define SENSOR_BAND_BOTH_LO       2350 // Both Sensors Active / Table Return: 2350..2750 mV
#define SENSOR_BAND_BOTH_HI       2750
#define SENSOR_BAND_CLOSED        3500 // Extended / Closed: > 3500 mV (up to 4095 mV)
#define SENSOR_TIMEOUT_DEFAULT_MS 7000 // Movement Watchdog Alarm Timeout

// Auxiliary Digital Inputs / Joysticks (Optional)
#define PIN_JOY_T1                21  // Left Auxiliary Joystick / Pedal 1
#define PIN_JOY_T2                37  // Right Auxiliary Joystick / Pedal 2

// ==============================================================================
// 3. I2C INDUSTRIAL BUS (GT911 TOUCH + DS3231 RTC)
// ==============================================================================
#define PIN_I2C_SDA               8   // Hardware I2C0 Data
#define PIN_I2C_SCL               9   // Hardware I2C0 Clock
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
// 5. INDUSTRIAL RELAY OUTPUTS (ACTIVE-LOW: 0 = ON, 1 = OFF)
// ==============================================================================
// Authoritative HAFFLER ETV3A Hardware Pin Mapping
#define PIN_RELAY_Q0              16  // Output Q0 (Table Cylinder / STEP Pulse)
#define PIN_RELAY_Q1              42  // Output Q1 (Left Clamp Solenoid / STEP Dir)
#define PIN_RELAY_Q2              39  // Output Q2 (Right Clamp Solenoid / STEP Enable)
#define PIN_RELAY_Q3              48  // Output Q3 (Heating Element Advance Solenoid)
#define PIN_RELAY_Q4              40  // Output Q4 (Shablon / Zero Gauge Solenoid)
#define PIN_RELAY_Q5              15  // Output Q5 (Solid State Relay - SSR Heater PWM)
#define PIN_RELAY_Q6              35  // Output Q6 (Knife / Auxiliary Solenoid 1)
#define PIN_RELAY_Q7              36  // Output Q7 (Knife / Auxiliary Solenoid 2)
#define PIN_RELAY_Q8              38  // Output Q8 (Spare Terminal Q8 for Field Remap)
#define PIN_RELAY_Q9              41  // Output Q9 (Spare Terminal Q9 for Field Remap)

// Dual-Personality Stepper / Servo Motion Pins (Shared on Terminals Q0, Q1, Q2)
#define PIN_STEP_PUL              PIN_RELAY_Q0 // GPIO 16 - Pulse Output (PCNT/LEDC/RMT)
#define PIN_STEP_DIR              PIN_RELAY_Q1 // GPIO 42 - Direction Output
#define PIN_STEP_EN               PIN_RELAY_Q2 // GPIO 39 - Driver Enable Output

// ==============================================================================
// 6. DUAL THERMOCOUPLE SPI BUS (MAX6675 K-TYPE THERMOCOUPLES)
// ==============================================================================
#define PIN_THERMO_CS1            21  // Head 1 Thermocouple Chip Select
#define PIN_THERMO_CS2            0   // Head 2 Thermocouple Chip Select (Double-Head)
#define PIN_THERMO_SCK            47  // Thermocouple SPI Clock
#define PIN_THERMO_MISO           37  // Thermocouple SPI Data Out

// ==============================================================================
// 7. RS485 INDUSTRIAL FIELDBUS & TELEMETRY SERIAL
// ==============================================================================
#define PIN_RS485_TX              43  // UART2 TX / Alternative UART0
#define PIN_RS485_RX              44  // UART2 RX / Alternative UART0
#define PIN_RS485_DE              46  // RS485 Direction Control (DE/RE)
#define PIN_RS485_DIR             PIN_RS485_DE // Direction Control Alias

// ==============================================================================
// 8. ACOUSTIC BUZZER (PWM HARDWARE TIMER)
// ==============================================================================
#define PIN_BUZZER                45  // Piezo Buzzer (LEDC Channel)

// ==============================================================================
// 9. DEDICATED BAKERY SPIRAL MIXER ALIASES
// ==============================================================================
#define PIN_MIXER_MOTOR           PIN_RELAY_Q0  // GPIO 16 - Single-Speed Mixer Motor Contactor (Active-LOW)
#define PIN_FLOUR_FEEDER          PIN_RELAY_Q1  // GPIO 42 - Motorized Flour Feeder / Auger Screw (Active-LOW)
#define PIN_BOWL_BRAKE            PIN_RELAY_Q2  // GPIO 39 - Rotating Bowl Friction Brake Solenoid (Active-LOW)
#define PIN_WATER_VALVE           PIN_RELAY_Q4  // GPIO 40 - Water Dosing Solenoid Valve (Active-LOW)
#define PIN_WATER_TEMP_CTRL       PIN_RELAY_Q5  // GPIO 15 - Water Tank Temperature Regulation Relay/SSR (Active-LOW)
#define PIN_LIGHT_GREEN           PIN_RELAY_Q8  // GPIO 38 - Stack Light Green (Active Mixing)
#define PIN_LIGHT_RED             PIN_RELAY_Q9  // GPIO 41 - Stack Light Red (Fault / Temperature Alert)

#define PIN_INPUT_ESTOP           PIN_EMERGENCY // GPIO 3 - Mushroom E-Stop Button (NC)
#define PIN_INPUT_START           PIN_START_PEDAL // GPIO 7 - Physical Start Button (NO)
#define PIN_INPUT_GUARD           PIN_INP_GUARD // GPIO 5 - Safety Bowl Guard Interlock (NC)
#define PIN_INPUT_OVERLOAD        PIN_INP_FAULT // GPIO 4 - Motor Thermal Overload Relay (NC)
#define PIN_INPUT_FLOWMETER       PIN_INP_REF   // GPIO 6 - Water Flow Meter Pulse Input (Opto)

#define PIN_WATER_TEMP_CS         PIN_THERMO_CS1 // GPIO 21 - MAX6675 Water Tank Thermocouple CS
#define PIN_WATER_TEMP_SCK        PIN_THERMO_SCK // GPIO 47 - MAX6675 Clock
#define PIN_WATER_TEMP_MISO       PIN_THERMO_MISO // GPIO 37 - MAX6675 Data Out
