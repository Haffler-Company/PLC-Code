#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ============================================================================
 * درایور صنعتی سنسور دمای دیجیتال DS18B20 (پروتکل 1-Wire مستقیم سطح رجیستر)
 * - رزولوشن ۱۲ بیت واقعی (دقت 0.0625 درجه سانتی‌گراد)
 * - اعتبارسنجی سخت‌گیرانه CRC-8 полином 0x8C (Dallas/Maxim) روی هر ۹ بایت Scratchpad
 * - پشتیبانی از هر دو بانک GPIO در ESP32-S3 (پین‌های زیر ۳۲ مانند GPIO 21 و بالای ۳۲ مانند GPIO 37)
 * - پشتیبانی از تک‌سنسور روی هر پایه (Skip ROM) و چندسنسوری روی یک پایه (Search/Match ROM)
 * ============================================================================
 */

#define DS18B20_FAMILY_CODE     0x28U
#define DS18S20_FAMILY_CODE     0x10U
#define DS1822_FAMILY_CODE      0x22U

typedef struct {
    uint8_t rom[8];
    bool    valid;
} ds18b20_rom_t;

/**
 * راه‌اندازی پایه GPIO برای باس 1-Wire با پول‌آپ فعال و حالت آزاد (HIGH)
 */
esp_err_t ds18b20_init_pin(gpio_num_t pin);

/**
 * ارسال پالس ریست 480µs و بررسی دقیق پالس حضور (Presence Pulse) سنسور
 * و اطمینان از عدم اتصال کوتاه خط به زمین
 */
bool ds18b20_reset_and_check_presence(gpio_num_t pin);

/**
 * بررسی وجود سنسور DS18B20 روی پین مشخص‌شده با تایید CRC-8 و تنظیم رزولوشن ۱۲ بیت
 */
bool ds18b20_probe(gpio_num_t pin);

/**
 * جستجوی آدرس‌های ۶۴ بیتی ROM روی یک پین (حداکثر max_devs سنسور روی یک خط)
 * بازگشت: تعداد سنسورهای معتبر با CRC صحیح
 */
uint8_t ds18b20_search_roms(gpio_num_t pin, ds18b20_rom_t *out_roms, uint8_t max_devs);

/**
 * ارسال فرمان شروع تبدیل دمای ۱۲ بیتی (0x44) به تمام سنسورهای روی پین (بدون بلاک کردن CPU)
 */
esp_err_t ds18b20_start_conversion(gpio_num_t pin);

/**
 * خواندن دمای تبدیل‌شده از Scratchpad سنسور (تک‌سنسوره روی پین با Skip ROM)
 * همراه با بررسی CRC-8، بررسی رجیستر کانفیگ، رد مقدار ریست 85.0°C و تلاش مجدد در صورت نویز
 */
esp_err_t ds18b20_read_temperature(gpio_num_t pin, float *out_temp_c, uint16_t *out_raw);

/**
 * خواندن دمای سنسور خاص با آدرس ۶۴ بیتی ROM (Match ROM 0x55)
 */
esp_err_t ds18b20_read_temperature_by_rom(gpio_num_t pin, const uint8_t rom[8], float *out_temp_c, uint16_t *out_raw);

#ifdef __cplusplus
}
#endif
