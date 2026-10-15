#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =========================================================================
 * زیرسیستم جامع تست خودکار سخت‌افزار در بدو روشن شدن (Power-On Self-Test - POST)
 * مطابق استانداردهای زیمنس ISA-18.2 و ارگونومی Apple HIG
 * =========================================================================
 */

typedef struct {
    bool has_error;
    bool user_resolvable;         /* true: قابل رفع توسط کاربر در کارگاه | false: نیازمند پشتیبانی کارخانه */
    
    /* جزئیات وضعیت قطعات */
    bool rtc_comms_fail;          /* عدم پاسخ چیپ ساعت DS3231 روی باس I2C */
    bool rtc_battery_lost;        /* باتری ساعت خالی است یا زمان تنظیم نیست */
    bool touch_comms_fail;        /* کنترلر لمسی خازنی GT911 شناسایی نشد */
    bool tc1_chip_fail;           /* عدم شناسایی تراشه MAX6675 مخزن شیر T1 */
    bool tc1_probe_disconnected;  /* قطعی پروب ترموکوپل مخزن شیر T1 (Open-Circuit) */
    bool tc2_chip_fail;           /* عدم شناسایی تراشه MAX6675 سیلندر انجماد T2 */
    bool tc2_probe_disconnected;  /* قطعی پروب ترموکوپل سیلندر T2 (Open-Circuit) */
    bool psram_fail;              /* خطای حافظه رم جانبی PSRAM */
    bool heap_low;                /* کمبود بحرانی فضای هیپ حافظه داخلی SRAM */
    bool nvs_fail;                /* خطای حافظه پایدار NVS */
    bool cpu_overheat;            /* دمای بحرانی پردازنده ESP32-S3 (>75°C) */
    
    float cpu_temp;
    float tc1_temp;
    float tc2_temp;
    
    /* رشته‌های توصیفی به ۳ زبان بدون نشت و تضمین Zero Tofu */
    char summary_fa[384];
    char cause_fa[256];
    char solution_fa[256];
    
    char summary_en[384];
    char cause_en[256];
    char solution_en[256];
    
    char summary_tr[384];
    char cause_tr[256];
    char solution_tr[256];
} boot_diag_result_t;

/* راه‌اندازی و اجرای تست خودکار سخت‌افزار (بدون بلاک‌کردن گرافیک) */
void boot_diag_run_self_test(void);

/* دسترسی به نتایج تست */
const boot_diag_result_t* boot_diag_get_result(void);

/* بررسی وجود خطا */
bool boot_diag_has_error(void);

/* بررسی آیا پیام هشدار توسط کاربر تایید/بسته شده است */
bool boot_diag_is_dismissed(void);

/* ثبت بسته شدن پیام هشدار */
void boot_diag_dismiss(void);

/* اجرای مجدد تست */
void boot_diag_retry(void);

#ifdef __cplusplus
}
#endif
