#ifndef BOOT_GUARD_H
#define BOOT_GUARD_H

#include <Arduino.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#ifdef __cplusplus
extern "C" {
#endif

// بررسی اولیه وضعیت بوت در خط اول setup()
// در صورت تشخیص ۲ بار کرش متوالی زودهنگام (قبل از رسیدن به وضعیت پایدار ۲۵ ثانیه‌ای)،
// پارتیشن بوت به صورت ۱۰۰٪ خودکار به برنامه کانفیگ/پارتیشن پشتیبان برگردانده می‌شود.
void boot_guard_early_check(void);

// پایش دوره‌ای و تثبیت وضعیت پایدار (Steady-State) پس از ۲۵ ثانیه کارکرد سالم
// با رسیدن به وضعیت پایدار، رول‌بک لغو و فریمور به عنوان نسخه معتبر تایید می‌شود.
void boot_guard_check_steady_state(void);

// تثبیت دستی وضعیت پایدار
void boot_guard_mark_steady_state(void);

// دریافت تعداد کرش‌های زودهنگام ثبت‌شده در حافظه RTC
uint8_t boot_guard_get_early_crash_count(void);

// آیا فریمور به وضعیت پایدار رسیده است؟
bool boot_guard_is_steady_state(void);

// اجرای رول‌بک اضطراری به پارتیشن پشتیبان (مانند کانفیگ-تست)
bool boot_guard_rollback_to_fallback_partition(void);

#ifdef __cplusplus
}
#endif

#endif // BOOT_GUARD_H
