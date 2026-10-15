/**
 * @file hal_rtc.cpp
 * @brief Thread-Safe DS3231 RTC Driver Implementation
 */

#include "hal_rtc.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "HAL_RTC";

static SemaphoreHandle_t s_rtc_mutex = NULL;
static hal_rtc_time_t s_cached_time = {2026, 9, 2, 12, 0, 0, 3};
static uint64_t s_last_read_us = 0;

static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

esp_err_t hal_rtc_init(void) {
    ESP_LOGI(TAG, "Initializing DS3231 RTC Driver...");

    if (!s_rtc_mutex) {
        s_rtc_mutex = xSemaphoreCreateMutex();
    }

    // Configure I2C_NUM_0 if not already initialized
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)RTC_PIN_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)RTC_PIN_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    
    // Note: i2c_param_config and i2c_driver_install may be shared with touch
    i2c_param_config(RTC_I2C_PORT, &conf);
    i2c_driver_install(RTC_I2C_PORT, conf.mode, 0, 0, 0);

    // Initial read to populate cache
    hal_rtc_get_time();
    ESP_LOGI(TAG, "DS3231 RTC initialized.");
    return ESP_OK;
}

hal_rtc_time_t hal_rtc_get_time(void) {
    uint64_t now_us = esp_timer_get_time();
    
    // If read within the last 500ms, return cached time immediately (zero I2C bus traffic for UI)
    if ((now_us - s_last_read_us) < 500000ULL) {
        return s_cached_time;
    }

    // Try to acquire mutex with a small 10ms timeout
    if (s_rtc_mutex && xSemaphoreTake(s_rtc_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint8_t reg = 0x00;
        uint8_t data[7] = {0};

        esp_err_t err = i2c_master_write_read_device(RTC_I2C_PORT, RTC_DS3231_ADDR, &reg, 1, data, 7, pdMS_TO_TICKS(50));
        if (err == ESP_OK) {
            s_cached_time.second      = bcd2bin(data[0] & 0x7F);
            s_cached_time.minute      = bcd2bin(data[1] & 0x7F);
            s_cached_time.hour        = bcd2bin(data[2] & 0x3F);
            s_cached_time.day_of_week = bcd2bin(data[3] & 0x07);
            s_cached_time.day         = bcd2bin(data[4] & 0x3F);
            s_cached_time.month       = bcd2bin(data[5] & 0x1F);
            s_cached_time.year        = 2000 + bcd2bin(data[6]);
            s_last_read_us = now_us;
        }
        xSemaphoreGive(s_rtc_mutex);
    }

    return s_cached_time;
}

esp_err_t hal_rtc_set_time(const hal_rtc_time_t *time) {
    if (!time || !s_rtc_mutex) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(s_rtc_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    uint8_t data[8];
    data[0] = 0x00; // Register 0
    data[1] = bin2bcd(time->second);
    data[2] = bin2bcd(time->minute);
    data[3] = bin2bcd(time->hour);
    data[4] = bin2bcd(time->day_of_week);
    data[5] = bin2bcd(time->day);
    data[6] = bin2bcd(time->month);
    data[7] = bin2bcd((uint8_t)(time->year % 100));

    esp_err_t err = i2c_master_write_to_device(RTC_I2C_PORT, RTC_DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        s_cached_time = *time;
        s_last_read_us = esp_timer_get_time();
    }
    xSemaphoreGive(s_rtc_mutex);
    return err;
}

void hal_rtc_format_time_str(char *buf, size_t max_len) {
    hal_rtc_time_t t = hal_rtc_get_time();
    snprintf(buf, max_len, "%02d:%02d:%02d", t.hour, t.minute, t.second);
}

void hal_rtc_format_date_str(char *buf, size_t max_len) {
    hal_rtc_time_t t = hal_rtc_get_time();
    snprintf(buf, max_len, "%04d/%02d/%02d", t.year, t.month, t.day);
}
