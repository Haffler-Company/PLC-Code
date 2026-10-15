#ifndef DEFINES_H
#define DEFINES_H

#include <Arduino.h>

//? ============================================================================
//? 1. DISPLAY & GRAPHICS (SPI ILI9341 / ST7796)
//? ============================================================================
#define TFT_CS              13
#define TFT_DC              14
#define TFT_RST             -1
#define TFT_BL              38      // Backlight Control (High = ON)

#define TFT_MOSI            10
#define TFT_MISO            37
#define TFT_SCLK            12

#define SCREEN_WIDTH        480
#define SCREEN_HEIGHT       320
#define SCREEN_ROTATION     3

//? ============================================================================
//? 2. CAPACITIVE TOUCH (I2C bb_captouch)
//? ============================================================================
#define TOUCH_SDA           8
#define TOUCH_SCL           9
#define TOUCH_INT           40
#define TOUCH_RST           -1      // Disabled: Pin 39 is dedicated to INVERTER_FORWARD
#define TOUCH_ORIENTATION   270

//? ============================================================================
//? 3. UNASSIGNED EXPANSION PINS (Formerly Thermocouple SPI)
//? ============================================================================
// GPIO 4 and GPIO 5 are free for future industrial digital expansion.

//? ============================================================================
//? 4. DIGITAL OUTPUTS & RELAYS (Active LOW: ON=LOW, OFF=HIGH)
//? ============================================================================
#define ON                  LOW
#define OFF                 HIGH

#define BUZZER              45      // Industrial warning buzzer
#define PISTON_MOTOR        36      // Q7: Piston cutting & discharge motor
#define FUNNELE_MOTOR       42      // Q1: Dough feeding funnel motor
#define INVERTER_FORWARD    39      // Q2: Inverter Forward rotation command
#define INVERTER_REVERSE    48      // Q3: Inverter Reverse rotation command

//? ============================================================================
//? 5. DIGITAL SENSOR INPUTS (Inductive Proximity Sensors)
//? ============================================================================
#define PISTON_SENSOR       7       // I7: Piston Home / Top limit position
#define FUNNELE_SENSOR      3       // I6: Funnel Home / Back limit position

//? ============================================================================
//? 6. ANALOG INPUTS & KEYPADS (ADC Resistor Ladders)
//? ============================================================================
#define KEY_F               1       // ADC Channel 1: F1..F5 Keypad Panel
#define KEY_M               2       // ADC Channel 2: M1..M5 Keypad Panel

#define HEAT_SENSOR         25      // Reserved Analog Input
#define LAMA_SENSOR         -1      // Reserved (Pin 36 is PISTON_MOTOR)

//? ============================================================================
//? 7. SAFETY & MOTION TIMEOUT LIMITS (Milliseconds)
//? ============================================================================
#define SENSOR_MOTION_TIMEOUT_MS 5000   // Max duration waiting for inductive sensor before trip
#define INVERTER_DEADTIME_MS      100   // Mandatory delay between FWD and REV switching

//? ============================================================================
//? 8. NVS STORAGE & RTOS CONSTANTS
//? ============================================================================
#define MAGIC_NUMBER        0xBBFF0FF0
#define LVGL_TICK_PERIOD    1

//? ============================================================================
//? 9. SEAMLESS CRASH & HANG RECOVERY CONSTANTS (RTC SRAM)
//? ============================================================================
#define UI_RESUME_SCR_MAGIC       0xD4A61E5AUL
#define UI_RESUME_VIEW_PREFIX     0xA7000000UL

#endif // DEFINES_H
