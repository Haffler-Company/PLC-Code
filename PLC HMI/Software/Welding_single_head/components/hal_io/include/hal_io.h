/**
 * @file hal_io.h
 * @brief Industrial GPIO, ADC, PWM & E-Stop HAL for Single-Head Welding PLC
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Output Level Invariants (Siemens Industrial Standard: Active LOW)
 * ========================================================================= */
#define HAL_OUTPUT_ON   0
#define HAL_OUTPUT_OFF  1

/* =========================================================================
 * Default Physical Hardware GPIO Mapping
 * ========================================================================= */
#define HAL_PIN_NC             (-1)

#define HAL_PIN_SSR_1          GPIO_NUM_15 // Solid State Relay - Heater
#define HAL_PIN_TABLE          GPIO_NUM_16 // Moving Table Jack Q0
#define HAL_PIN_PRESS_LEFT     GPIO_NUM_42 // Left Profile Clamp Jack Q1
#define HAL_PIN_PRESS_RIGHT    GPIO_NUM_39 // Right Profile Clamp Jack Q2
#define HAL_PIN_ELEMENT        GPIO_NUM_48 // Heating Element Jack Q3
#define HAL_PIN_SHABLON        GPIO_NUM_40 // Zero Shablon Jack Q4
#define HAL_PIN_ZARBEH1        GPIO_NUM_35 // Striker/Knive Jack 1 Q6
#define HAL_PIN_ZARBEH2        GPIO_NUM_36 // Striker/Knive Jack 2 Q7
#define HAL_PIN_SPARE_Q8       GPIO_NUM_38 // Auxiliary output terminal Q8
#define HAL_PIN_SPARE_Q9       GPIO_NUM_41 // Auxiliary output terminal Q9
#define HAL_PIN_BUZZER         GPIO_NUM_45 // Piezo Buzzer (LEDC PWM)

#define HAL_PIN_EMERGENCY      GPIO_NUM_3  // E-Stop Input (Active HIGH / LOW depending on wiring)
#define HAL_PIN_PEDAL          GPIO_NUM_7  // Foot Pedal Start Input (I7)

#define HAL_ADC_ELEMENT        GPIO_NUM_6  // I0=Element Retracted, I1=Element Extended
#define HAL_ADC_SHABLON        GPIO_NUM_5  // I2=Shablon Retracted, I3=Shablon Extended
#define HAL_ADC_TABLE          GPIO_NUM_4  // I4=Table Retracted, I5=Table Forward
#define HAL_ADC_FKEYS_1        GPIO_NUM_1  // F-Keys Ladder 1 (F1..F5)
#define HAL_ADC_FKEYS_2        GPIO_NUM_2  // F-Keys Ladder 2 (F6..F10)

/* =========================================================================
 * Analog Sensor Band Thresholds (Millivolts)
 * ========================================================================= */
#define HAL_BAND_DEAD_MIN      1850
#define HAL_BAND_DEAD_MAX      2240
#define HAL_BAND_RETRACT_MIN   1300
#define HAL_BAND_RETRACT_MAX   1800
#define HAL_BAND_EXTEND_MIN    3300
#define HAL_BAND_BOTH_MIN      2350
#define HAL_BAND_BOTH_MAX      2750

typedef enum {
    JACK_SENSOR_NONE = 0,
    JACK_SENSOR_RETRACTED,
    JACK_SENSOR_EXTENDED,
    JACK_SENSOR_BOTH,
    JACK_SENSOR_MOVING
} hal_jack_sensor_state_t;

typedef enum {
    BUZZER_TONE_TOUCH = 0,
    BUZZER_TONE_CONFIRM,
    BUZZER_TONE_ERROR,
    BUZZER_TONE_CYCLE_START,
    BUZZER_TONE_CYCLE_END,
    BUZZER_TONE_ALARM
} hal_buzzer_tone_t;

/* =========================================================================
 * HAL IO Public API
 * ========================================================================= */
esp_err_t hal_io_init(void);

// Solenoid / Relay Outputs
void hal_io_set_ssr(bool on);
void hal_io_set_table(bool on);
void hal_io_set_press_left(bool on);
void hal_io_set_press_right(bool on);
void hal_io_set_element(bool on);
void hal_io_set_shablon(bool on);
void hal_io_set_zarbeh1(bool on);
void hal_io_set_zarbeh2(bool on);
void hal_io_all_outputs_off(void);

// Safety & Emergency Stop
bool hal_io_is_emergency_active(void);
bool hal_io_is_pedal_pressed(void);

// Jack Analog Sensors
hal_jack_sensor_state_t hal_io_read_element_sensor(void);
hal_jack_sensor_state_t hal_io_read_shablon_sensor(void);
hal_jack_sensor_state_t hal_io_read_table_sensor(void);

// Buzzer & Audio Feedback
void hal_io_play_buzzer(hal_buzzer_tone_t tone);
void hal_io_buzzer_stop(void);

// Dynamic Pin Remap Support (from NVS)
void hal_io_remap_pins(int ssr, int table, int press_l, int press_r, int element, int shablon, int zarbeh1, int zarbeh2);

#ifdef __cplusplus
}
#endif
