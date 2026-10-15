/**
 * @file hal_io.cpp
 * @brief Ultra-Fast Direct Hardware Register Level GPIO, ADC, PWM & E-Stop Driver
 * @author Haffler Automation Systems
 */

#include "hal_io.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/ledc.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

static const char *TAG = "HAL_IO_REG";

static portMUX_TYPE s_io_spinlock = portMUX_INITIALIZER_UNLOCKED;

// Active pin numbers (supports runtime remap from NVS)
static int s_pin_ssr = HAL_PIN_SSR_1;
static int s_pin_table = HAL_PIN_TABLE;
static int s_pin_press_left = HAL_PIN_PRESS_LEFT;
static int s_pin_press_right = HAL_PIN_PRESS_RIGHT;
static int s_pin_element = HAL_PIN_ELEMENT;
static int s_pin_shablon = HAL_PIN_SHABLON;
static int s_pin_zarbeh1 = HAL_PIN_ZARBEH1;
static int s_pin_zarbeh2 = HAL_PIN_ZARBEH2;

// ADC Handles
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static adc_cali_handle_t s_adc1_cali_handle = NULL;
static bool s_adc_cali_enabled = false;

// Buzzer LEDC Config
#define BUZZER_LEDC_TIMER       LEDC_TIMER_0
#define BUZZER_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL     LEDC_CHANNEL_0
#define BUZZER_LEDC_DUTY_RES    LEDC_TIMER_10_BIT
#define BUZZER_LEDC_DUTY        (512) // 50% duty cycle

/* =========================================================================
 * Direct Hardware Register Access Functions (<10ns Execution)
 * ========================================================================= */

static inline void direct_reg_gpio_set_low(int pin) {
    if (pin < 0) return;
    if (pin < 32) {
        GPIO.out_w1tc.val = (1UL << pin);
    } else if (pin < 49) {
        GPIO.out1_w1tc.val = (1UL << (pin - 32));
    }
}

static inline void direct_reg_gpio_set_high(int pin) {
    if (pin < 0) return;
    if (pin < 32) {
        GPIO.out_w1ts.val = (1UL << pin);
    } else if (pin < 49) {
        GPIO.out1_w1ts.val = (1UL << (pin - 32));
    }
}

static inline int direct_reg_gpio_get_level(int pin) {
    if (pin < 0) return 0;
    if (pin < 32) {
        return (GPIO.in.val >> pin) & 0x01;
    } else if (pin < 49) {
        return (GPIO.in1.val >> (pin - 32)) & 0x01;
    }
    return 0;
}

static void init_single_output_pin(int pin) {
    if (pin < 0) return;
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    direct_reg_gpio_set_high(pin); // Guarantee OFF at boot
}

esp_err_t hal_io_init(void) {
    ESP_LOGI(TAG, "Initializing Direct Register-Level HAL IO Subsystem...");

    // 1. Initialize all Solenoid / Relay Outputs
    init_single_output_pin(s_pin_ssr);
    init_single_output_pin(s_pin_table);
    init_single_output_pin(s_pin_press_left);
    init_single_output_pin(s_pin_press_right);
    init_single_output_pin(s_pin_element);
    init_single_output_pin(s_pin_shablon);
    init_single_output_pin(s_pin_zarbeh1);
    init_single_output_pin(s_pin_zarbeh2);
    init_single_output_pin(HAL_PIN_SPARE_Q8);
    init_single_output_pin(HAL_PIN_SPARE_Q9);

    // 2. Initialize E-Stop and Pedal Inputs
    gpio_config_t in_conf = {};
    in_conf.intr_type = GPIO_INTR_DISABLE;
    in_conf.mode = GPIO_MODE_INPUT;
    in_conf.pin_bit_mask = (1ULL << HAL_PIN_EMERGENCY) | (1ULL << HAL_PIN_PEDAL);
    in_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    in_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&in_conf);

    // 3. Initialize ADC1 for Analog Jack Sensors & F-Keys
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &s_adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_3, &chan_config); // GPIO4
    adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_4, &chan_config); // GPIO5
    adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_5, &chan_config); // GPIO6
    adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_0, &chan_config); // GPIO1
    adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_1, &chan_config); // GPIO2

    // ADC Calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_5,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &s_adc1_cali_handle) == ESP_OK) {
        s_adc_cali_enabled = true;
    }

    // 4. Initialize LEDC Buzzer PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = BUZZER_LEDC_MODE,
        .duty_resolution  = BUZZER_LEDC_DUTY_RES,
        .timer_num        = BUZZER_LEDC_TIMER,
        .freq_hz          = 2700,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = HAL_PIN_BUZZER,
        .speed_mode     = BUZZER_LEDC_MODE,
        .channel        = BUZZER_LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = BUZZER_LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    hal_io_all_outputs_off();
    ESP_LOGI(TAG, "Direct Register-Level HAL IO initialized successfully.");
    return ESP_OK;
}

void hal_io_set_ssr(bool on) {
    if (s_pin_ssr >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_ssr);
        else direct_reg_gpio_set_high(s_pin_ssr);
    }
}
void hal_io_set_table(bool on) {
    if (s_pin_table >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_table);
        else direct_reg_gpio_set_high(s_pin_table);
    }
}
void hal_io_set_press_left(bool on) {
    if (s_pin_press_left >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_press_left);
        else direct_reg_gpio_set_high(s_pin_press_left);
    }
}
void hal_io_set_press_right(bool on) {
    if (s_pin_press_right >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_press_right);
        else direct_reg_gpio_set_high(s_pin_press_right);
    }
}
void hal_io_set_element(bool on) {
    if (s_pin_element >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_element);
        else direct_reg_gpio_set_high(s_pin_element);
    }
}
void hal_io_set_shablon(bool on) {
    if (s_pin_shablon >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_shablon);
        else direct_reg_gpio_set_high(s_pin_shablon);
    }
}
void hal_io_set_zarbeh1(bool on) {
    if (s_pin_zarbeh1 >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_zarbeh1);
        else direct_reg_gpio_set_high(s_pin_zarbeh1);
    }
}
void hal_io_set_zarbeh2(bool on) {
    if (s_pin_zarbeh2 >= 0) {
        if (on) direct_reg_gpio_set_low(s_pin_zarbeh2);
        else direct_reg_gpio_set_high(s_pin_zarbeh2);
    }
}

/**
 * @brief Immediate <10ns Atomic Emergency Cutoff via Direct Hardware Register Writes
 */
void hal_io_all_outputs_off(void) {
    portENTER_CRITICAL(&s_io_spinlock);

    // Bank 0 (pins 15, 16): Turn OFF (Set HIGH)
    GPIO.out_w1ts.val = (1UL << HAL_PIN_SSR_1) | (1UL << HAL_PIN_TABLE);

    // Bank 1 (pins 35, 36, 38, 39, 40, 41, 42, 48): Turn OFF (Set HIGH)
    uint32_t mask1 = (1UL << (HAL_PIN_ZARBEH1 - 32)) |
                     (1UL << (HAL_PIN_ZARBEH2 - 32)) |
                     (1UL << (HAL_PIN_SPARE_Q8 - 32)) |
                     (1UL << (HAL_PIN_PRESS_RIGHT - 32)) |
                     (1UL << (HAL_PIN_SHABLON - 32)) |
                     (1UL << (HAL_PIN_SPARE_Q9 - 32)) |
                     (1UL << (HAL_PIN_PRESS_LEFT - 32)) |
                     (1UL << (HAL_PIN_ELEMENT - 32));
    GPIO.out1_w1ts.val = mask1;

    portEXIT_CRITICAL(&s_io_spinlock);
    hal_io_buzzer_stop();
}

bool hal_io_is_emergency_active(void) {
    // Direct register read of GPIO3
    return (direct_reg_gpio_get_level(HAL_PIN_EMERGENCY) == 0);
}

bool hal_io_is_pedal_pressed(void) {
    // Direct register read of GPIO7
    return (direct_reg_gpio_get_level(HAL_PIN_PEDAL) == 0);
}

static uint32_t read_adc_voltage_mv(adc_channel_t chan) {
    if (!s_adc1_handle) return 0;
    int raw = 0;
    if (adc_oneshot_read(s_adc1_handle, chan, &raw) != ESP_OK) return 0;
    
    int voltage = 0;
    if (s_adc_cali_enabled && s_adc1_cali_handle) {
        adc_cali_raw_to_voltage(s_adc1_cali_handle, raw, &voltage);
        return (uint32_t)voltage;
    }
    // Fallback: 12-bit DB_12 scaling (~3.3V reference)
    return (uint32_t)((raw * 3300) / 4095);
}

static hal_jack_sensor_state_t classify_sensor_band(uint32_t mv) {
    if (mv >= HAL_BAND_BOTH_MIN && mv <= HAL_BAND_BOTH_MAX) {
        return JACK_SENSOR_BOTH;
    }
    if (mv >= HAL_BAND_EXTEND_MIN) {
        return JACK_SENSOR_EXTENDED;
    }
    if (mv >= HAL_BAND_RETRACT_MIN && mv <= HAL_BAND_RETRACT_MAX) {
        return JACK_SENSOR_RETRACTED;
    }
    if (mv >= HAL_BAND_DEAD_MIN && mv <= HAL_BAND_DEAD_MAX) {
        return JACK_SENSOR_MOVING;
    }
    return JACK_SENSOR_NONE;
}

hal_jack_sensor_state_t hal_io_read_element_sensor(void) {
    uint32_t mv = read_adc_voltage_mv(ADC_CHANNEL_5); // GPIO6
    return classify_sensor_band(mv);
}

hal_jack_sensor_state_t hal_io_read_shablon_sensor(void) {
    uint32_t mv = read_adc_voltage_mv(ADC_CHANNEL_4); // GPIO5
    return classify_sensor_band(mv);
}

hal_jack_sensor_state_t hal_io_read_table_sensor(void) {
    uint32_t mv = read_adc_voltage_mv(ADC_CHANNEL_3); // GPIO4
    return classify_sensor_band(mv);
}

void hal_io_play_buzzer(hal_buzzer_tone_t tone) {
    uint32_t freq = 2700;
    switch (tone) {
        case BUZZER_TONE_TOUCH:       freq = 3200; break;
        case BUZZER_TONE_CONFIRM:     freq = 3600; break;
        case BUZZER_TONE_ERROR:       freq = 2000; break;
        case BUZZER_TONE_CYCLE_START: freq = 4000; break;
        case BUZZER_TONE_CYCLE_END:   freq = 3000; break;
        case BUZZER_TONE_ALARM:       freq = 2500; break;
    }
    ledc_set_freq(BUZZER_LEDC_MODE, BUZZER_LEDC_TIMER, freq);
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_LEDC_DUTY);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

void hal_io_buzzer_stop(void) {
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

void hal_io_remap_pins(int ssr, int table, int press_l, int press_r, int element, int shablon, int zarbeh1, int zarbeh2) {
    portENTER_CRITICAL(&s_io_spinlock);
    if (ssr >= 0) s_pin_ssr = ssr;
    if (table >= 0) s_pin_table = table;
    if (press_l >= 0) s_pin_press_left = press_l;
    if (press_r >= 0) s_pin_press_right = press_r;
    if (element >= 0) s_pin_element = element;
    if (shablon >= 0) s_pin_shablon = shablon;
    if (zarbeh1 >= 0) s_pin_zarbeh1 = zarbeh1;
    if (zarbeh2 >= 0) s_pin_zarbeh2 = zarbeh2;
    portEXIT_CRITICAL(&s_io_spinlock);
}
