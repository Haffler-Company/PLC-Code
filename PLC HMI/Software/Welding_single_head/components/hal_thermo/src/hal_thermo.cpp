/**
 * @file hal_thermo.cpp
 * @brief Direct Hardware Register Level MAX6675 Driver with Instant Thermal Safety Interlock
 * @author Haffler Automation Systems
 */

#include "hal_thermo.h"
#include "hal_io.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static const char *TAG = "HAL_THERMO_REG";

static bool s_primary_healthy = false;
static portMUX_TYPE s_thermo_spinlock = portMUX_INITIALIZER_UNLOCKED;

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static inline void direct_reg_gpio_set_low(int pin) {
    if (pin < 32) {
        GPIO.out_w1tc.val = (1UL << pin);
    } else {
        GPIO.out1_w1tc.val = (1UL << (pin - 32));
    }
}

static inline void direct_reg_gpio_set_high(int pin) {
    if (pin < 32) {
        GPIO.out_w1ts.val = (1UL << pin);
    } else {
        GPIO.out1_w1ts.val = (1UL << (pin - 32));
    }
}

static inline int direct_reg_gpio_get_level(int pin) {
    if (pin < 32) {
        return (GPIO.in.val >> pin) & 0x01;
    } else {
        return (GPIO.in1.val >> (pin - 32)) & 0x01;
    }
}

esp_err_t hal_thermo_init(void) {
    ESP_LOGI(TAG, "Initializing Direct Register-Level MAX6675 Driver...");

    gpio_config_t out_conf = {};
    out_conf.intr_type = GPIO_INTR_DISABLE;
    out_conf.mode = GPIO_MODE_OUTPUT;
    out_conf.pin_bit_mask = (1ULL << THERMO_PIN_SCK) | (1ULL << THERMO_PIN_CS_1) | (1ULL << THERMO_PIN_CS_2);
    out_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    out_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&out_conf);

    gpio_config_t in_conf = {};
    in_conf.intr_type = GPIO_INTR_DISABLE;
    in_conf.mode = GPIO_MODE_INPUT;
    in_conf.pin_bit_mask = (1ULL << THERMO_PIN_MISO);
    in_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    in_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&in_conf);

    // Initial idle states
    direct_reg_gpio_set_high(THERMO_PIN_CS_1);
    direct_reg_gpio_set_high(THERMO_PIN_CS_2);
    direct_reg_gpio_set_low(THERMO_PIN_SCK);

    ESP_LOGI(TAG, "Direct Register MAX6675 Driver initialized.");
    return ESP_OK;
}

static uint16_t read_raw_max6675_reg(int cs_pin) {
    uint16_t raw = 0;

    portENTER_CRITICAL(&s_thermo_spinlock);

    direct_reg_gpio_set_low(cs_pin); // Assert CS
    esp_rom_delay_us(1);

    for (int i = 15; i >= 0; i--) {
        direct_reg_gpio_set_high(THERMO_PIN_SCK);
        esp_rom_delay_us(1);
        if (direct_reg_gpio_get_level(THERMO_PIN_MISO)) {
            raw |= (1 << i);
        }
        direct_reg_gpio_set_low(THERMO_PIN_SCK);
        esp_rom_delay_us(1);
    }

    direct_reg_gpio_set_high(cs_pin); // De-assert CS

    portEXIT_CRITICAL(&s_thermo_spinlock);
    return raw;
}

hal_thermo_reading_t hal_thermo_read_primary(void) {
    hal_thermo_reading_t res = {
        .temperature_c = 0.0f,
        .status = THERMO_STATUS_OK,
        .last_read_timestamp_ms = get_time_ms()
    };

    uint16_t raw = read_raw_max6675_reg(THERMO_PIN_CS_1);

    // Check Bit 2: Open Circuit Fault
    if (raw & 0x0004) {
        res.status = THERMO_STATUS_OPEN_CIRCUIT;
        s_primary_healthy = false;
        // CRITICAL SIEMENS PROCESS SAFETY: Immediate direct register SSR de-energization
        hal_io_set_ssr(false);
        ESP_LOGE(TAG, "THERMAL RED LINE: Thermocouple Open-Circuit Detected (0x04)! SSR cut off.");
        return res;
    }

    // Bits 3..14: 12-bit temp reading in 0.25°C increments
    int16_t temp_raw = (raw >> 3) & 0x0FFF;
    float temp_c = temp_raw * 0.25f;

    // Check bounds
    if (temp_c < THERMO_MIN_SAFE_TEMP_C || temp_c > THERMO_MAX_SAFE_TEMP_C) {
        res.status = THERMO_STATUS_OUT_OF_BOUNDS;
        res.temperature_c = temp_c;
        s_primary_healthy = false;
        hal_io_set_ssr(false);
        ESP_LOGE(TAG, "THERMAL RED LINE: Temp out of bounds (%.2f°C)! SSR cut off.", temp_c);
        return res;
    }

    res.temperature_c = temp_c;
    res.status = THERMO_STATUS_OK;
    s_primary_healthy = true;
    return res;
}

hal_thermo_reading_t hal_thermo_read_secondary(void) {
    hal_thermo_reading_t res = {
        .temperature_c = 0.0f,
        .status = THERMO_STATUS_OK,
        .last_read_timestamp_ms = get_time_ms()
    };

    uint16_t raw = read_raw_max6675_reg(THERMO_PIN_CS_2);
    if (raw & 0x0004) {
        res.status = THERMO_STATUS_OPEN_CIRCUIT;
        return res;
    }
    int16_t temp_raw = (raw >> 3) & 0x0FFF;
    res.temperature_c = temp_raw * 0.25f;
    return res;
}

bool hal_thermo_is_healthy(void) {
    return s_primary_healthy;
}
