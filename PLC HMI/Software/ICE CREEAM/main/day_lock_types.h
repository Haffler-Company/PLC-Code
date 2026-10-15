#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAY_LOCK_MAX_STAGES       5
#define DAY_LOCK_MAGIC            0x4841464CUL /* "HAFL" */
#define DAY_LOCK_SALT             "ASTIAJ_LEASING_2026_SECRET"

/* کدهای مستر مدیریت کارخانه آستیاج (Tier 1) */
#define MASTER_PIN_1              "12773"
#define MASTER_PIN_2              "123456"
#define MASTER_PIN_3              "12733"
#define MASTER_PIN_ALT            "1127730"

/* رمزهای اضطراری ۱۰ روزه یکبار مصرف (هر کدام مستقل):
 *   "11"  → ۱۰ روز + مصرف فلگ [0]
 *   "22"  → ۱۰ روز + مصرف فلگ [1]
 *   "33"  → ۱۰ روز + مصرف فلگ [2]
 * کد قدیمی 112233 نیز حفظ شده (backward compat) */
#define EMERGENCY_GRACE_PIN_LEGACY "112233"
#define EMERGENCY_GRACE_COUNT      3
static const char * const EMERGENCY_GRACE_PINS[EMERGENCY_GRACE_COUNT] = { "11", "22", "33" };

/* رمز پایه مدیر پیش‌فرض جهت الگوریتم اقساط */
#define DEFAULT_ADMIN_PIN         "123456"

/**
 * @brief ماشین وضعیت ۶ مرحله‌ای منطبق با استاندارد زیمنس و PackML
 */
typedef enum {
    LOCK_STAGE_FACTORY_FREE = 0,  /* کارکرد دائم بدون محدودیت و بدون روزشمار */
    LOCK_STAGE_ACTIVE_LEASE  = 1,  /* دوره لیزینگ فعال و شمارش معکوس روزها */
    LOCK_STAGE_GRACE_10      = 2,  /* مهلت اضطراری ۱۰ روزه فعال شده با کد ۱۱۲۲۳۳ */
    LOCK_STAGE_SOFT_LOCK     = 3,  /* پایان روزها - اجازه اتمام چرخه جاری، ممانعت از چرخه جدید */
    LOCK_STAGE_HARD_LOCK     = 4,  /* قفل کامل سخت‌افزاری و بی‌برق شدن خروجی‌ها */
    LOCK_STAGE_TAMPER        = 5   /* خطای دستکاری و عقب کشیدن ساعت یا بروت‌فورس */
} DayLockState_t;

/**
 * @brief واحد زمان باقیمانده قسط جاری بر اساس قانون تک‌واحدی
 */
typedef enum {
    DAY_LOCK_UNIT_DAYS = 0,
    DAY_LOCK_UNIT_HOURS = 1,
    DAY_LOCK_UNIT_MINUTES = 2
} DayLockTimeUnit_t;

/**
 * @brief ساختار ذخیره‌سازی قدیمی جهت مهاجرت بدون وقفه داده‌های پیشین NVS
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;                          /* 0x4841464C ("HAFL") */
    uint8_t  active_stage;                   /* 0=Free, 1..5=Stages */
    int16_t  remain_days;                    /* روزهای باقیمانده در استیج جاری */
    int16_t  stage_days[DAY_LOCK_MAX_STAGES];/* روزهای پیش‌تنظیم شده مراحل ۵ گانه */
    bool     grace10_used;                   /* فلگ backward-compat قدیمی (112233) */
    uint32_t last_known_epoch;               /* آخرین زمان ثبت شده (گارد مونوتونیک) */
    uint32_t sec_accumulator;                /* انباشت ثانیه‌ها در رم برای محافظت فلش */
    uint8_t  failed_attempts;                /* شمارنده تلاش‌های ناموفق رمز */
    uint32_t lockout_until_sec;              /* زمان پایان فریز بروت‌فورس (ثانیه سیستم) */
    uint32_t boot_counter;                   /* شمارنده دفعات بوت سیستم */
    bool     grace_used[3];                  /* فلگ مصرف رمزهای "11","22","33" (جداگانه) */
    uint32_t crc32;                          /* چک‌سام ساختار */
} DayLockLegacyRecord_t;

/**
 * @brief رکورد ذخیره‌سازی آینه‌ای در NVS با محافظت CRC32 و تفکیک وضعیت از مرحله اقساط
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;                          /* 0x4841464C ("HAFL") */
    uint8_t  lock_state;                     /* DayLockState_t: 0=Free, 1=Active, 2=Grace, 3=SoftLock, 4=HardLock, 5=Tamper */
    uint8_t  active_stage;                   /* مرحله فعال اقساط: 1..5 (T1..T5)، یا 0 برای حالت آزاد */
    int16_t  remain_days;                    /* روزهای باقیمانده در استیج جاری */
    int16_t  stage_days[DAY_LOCK_MAX_STAGES];/* روزهای پیش‌تنظیم شده مراحل ۵ گانه */
    bool     grace10_used;                   /* فلگ backward-compat قدیمی (112233) */
    uint32_t last_known_epoch;               /* آخرین زمان ثبت شده (گارد مونوتونیک) */
    uint32_t sec_accumulator;                /* انباشت ثانیه‌ها در رم برای محافظت فلش */
    uint8_t  failed_attempts;                /* شمارنده تلاش‌های ناموفق رمز */
    uint32_t lockout_until_sec;              /* زمان پایان فریز بروت‌فورس (ثانیه سیستم) */
    uint32_t boot_counter;                   /* شمارنده دفعات بوت سیستم */
    bool     grace_used[3];                  /* فلگ مصرف رمزهای "11","22","33" (جداگانه) */
    uint32_t crc32;                          /* چک‌سام ساختار */
} DayLockRecord_t;

#ifdef __cplusplus
}
#endif
