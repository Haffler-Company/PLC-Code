#pragma once
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} ds3231_time_t;

esp_err_t ds3231_init(void);
esp_err_t ds3231_get_time(ds3231_time_t *t);
esp_err_t ds3231_set_time(const ds3231_time_t *t);

esp_err_t rtc_set(uint16_t yyyy, uint8_t mm, uint8_t dd,
                  uint8_t hh, uint8_t mi, uint8_t ss,
                  uint8_t wday /* 1..7 */);
typedef struct {
    uint16_t jy;     // سال شمسی
    uint8_t  jm;     // ماه 1..12
    uint8_t  jd;     // روز 1..31
    uint8_t  hour;   // 0..23
    uint8_t  minute; // 0..59
    uint8_t  second; // 0..59
    uint8_t  wday;   // 1..7 مطابق DS3231: 1=Sunday ... 7=Saturday
} jalali_time_t;

// خواندن از DS3231 و تحویل زمان شمسی
esp_err_t ds3231_get_time_jalali(jalali_time_t *jt);

// ست‌کردن DS3231 با ورودی شمسی (تبدیل داخلی به میلادی)
esp_err_t rtc_set_jalali(uint16_t jyy, uint8_t jmm, uint8_t jdd,
                         uint8_t hh, uint8_t mi, uint8_t ss);

// تبدیل تقویم میلادی به شمسی و بالعکس
void gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd);
void jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd);

// همگام‌سازی ساعت سیستم با DS3231 جهت تطبیق پوزیکس و تایم‌استمپ‌های خطا
esp_err_t ds3231_sync_system_time(void);

// محاسبه زمان ثانیه‌ای پوزیکس (Epoch) از روی ساعت سخت‌افزاری DS3231
time_t ds3231_get_epoch(void);