/**
 * @file hal_rtc.h
 * @brief Thread-Safe DS3231 RTC Driver with Non-blocking Cached Fallback
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_I2C_PORT       I2C_NUM_0
#define RTC_PIN_SDA        8
#define RTC_PIN_SCL        9
#define RTC_DS3231_ADDR    0x68

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  day_of_week;
} hal_rtc_time_t;

/**
 * @brief Initialize DS3231 I2C bus and internal mutex
 */
esp_err_t hal_rtc_init(void);

/**
 * @brief Get current date and time safely (non-blocking cached read for Core 1 UI)
 */
hal_rtc_time_t hal_rtc_get_time(void);

/**
 * @brief Set RTC date and time
 */
esp_err_t hal_rtc_set_time(const hal_rtc_time_t *time);

/**
 * @brief Format time string (HH:MM:SS)
 */
void hal_rtc_format_time_str(char *buf, size_t max_len);

/**
 * @brief Format date string (YYYY/MM/DD)
 */
void hal_rtc_format_date_str(char *buf, size_t max_len);

#ifdef __cplusplus
}
#endif
