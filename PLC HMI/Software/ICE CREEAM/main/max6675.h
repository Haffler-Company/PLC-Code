#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * نگاشت پایه‌های صنعتی سنسور دمای دوحالته (MAX6675 یا DS18B20):
 * حالت ۱ (مونتاژ MAX6675 روی برد):
 * - MISO: GPIO 37
 * - SCK : GPIO 47 (کلاک مشترک)
 * - CS1 : GPIO 21 (دمای مخزن بالا شیر / T1 / پین ۲۵)
 * - CS2 : GPIO 0  (دمای سیلندر انجماد / T2 / پین ۲۴)
 *
 * حالت ۲ (حذف MAX6675 و اتصال مستقیم سنسور دیجیتال DS18B20):
 * - DS18B20 کانال ۱ (مخزن شیر / T1): GPIO 21
 * - DS18B20 کانال ۲ (سیلندر انجماد / T2): GPIO 37
 */

#define MAX6675_MISO_GPIO       GPIO_NUM_37
#define MAX6675_SCK_GPIO        GPIO_NUM_47  /* SCK مشترک هر دو سنسور دما */
#define MAX6675_CS_TOP_GPIO     GPIO_NUM_21  /* T1+: سنسور دمای مخزن شیر (پین ۲۵) */
#define MAX6675_CS_CYL_GPIO     GPIO_NUM_0   /* T2+: سنسور دمای سیلندر انجماد (پین ۲۴) */

#define DS18B20_TOP_GPIO        GPIO_NUM_21  /* پایه دیتای سنسور اول DS18B20 (مخزن شیر / T1) */
#define DS18B20_CYL_GPIO        GPIO_NUM_37  /* پایه دیتای سنسور دوم DS18B20 (سیلندر انجماد / T2) */

typedef enum {
    TEMP_SENSOR_MODE_DS18B20 = 0, /* حالت سنسورهای دیجیتال DS18B20 روی GPIO 21 و GPIO 37 */
    TEMP_SENSOR_MODE_MAX6675 = 1, /* حالت آی‌سی‌های ترموکوپل MAX6675 مونتاژشده روی برد */
} temp_sensor_mode_t;

/* وضعیت سنسور */
typedef enum {
    MAX6675_STATUS_OK = 0,
    MAX6675_STATUS_OPEN_CIRCUIT = 1,  /* سیم سنسور قطع است */
    MAX6675_STATUS_UNINITIALIZED = 2
} max6675_status_t;

void max6675_init(void);
void max6675_poll(void);
void max6675_reprobe(void);
temp_sensor_mode_t max6675_get_sensor_mode(void);

float max6675_get_temp_top(void);           /* دمای مخزن بالا (شیر) به سانتی‌گراد */
float max6675_get_temp_cylinder(void);      /* دمای سیلندر انجماد به سانتی‌گراد */

bool max6675_is_top_valid(void);
bool max6675_is_cylinder_valid(void);
bool max6675_is_top_enabled(void);
void max6675_set_top_enabled(bool en);
bool max6675_is_cyl_enabled(void);
void max6675_set_cyl_enabled(bool en);

uint16_t max6675_get_raw_top(void);
uint16_t max6675_get_raw_cylinder(void);

/* توابع کالیبراسیون و جبران خطای دما (دامنه: 15.0- الی 15.0+ درجه) */
float max6675_get_cal_offset_top(void);
void max6675_set_cal_offset_top(float offset);
float max6675_get_cal_offset_cyl(void);
void max6675_set_cal_offset_cyl(float offset);
float max6675_get_raw_temp_top(void);
float max6675_get_raw_temp_cyl(void);

#ifdef __cplusplus
}
#endif
