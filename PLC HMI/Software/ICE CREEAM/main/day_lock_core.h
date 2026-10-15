#pragma once

#include "day_lock_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief راه‌اندازی اولیه موتور روزشمار و قفل سخت‌افزاری
 */
void day_lock_init(void);

/**
 * @brief تیک پریودیک ۱ ثانیه‌ای (اجرا در Core 0 / تسک کنترل دوره‌ای)
 */
void day_lock_tick_1000ms(void);

/**
 * @brief وضعیت فعلی ماشین حالت قفل
 */
DayLockState_t day_lock_get_state(void);

/**
 * @brief روزهای باقیمانده کارکرد
 */
int16_t day_lock_get_remain_days(void);

/**
 * @brief محاسبه زمان باقیمانده قسط جاری بر اساس قانون تک‌واحدی:
 *        - بیش از ۱ روز: روز (مثلاً ۱۲ روز، ۸۰ روز)
 *        - زیر ۱ روز: ساعت (مثلاً ۱۸ ساعت، ۳ ساعت)
 *        - زیر ۱ ساعت: دقیقه (مثلاً ۳۴ دقیقه، ۵ دقیقه)
 * @param out_val مقدار عددی خروجی
 * @return واحد زمانی مربوطه (DayLockTimeUnit_t)
 */
DayLockTimeUnit_t day_lock_get_remain_time_unit(int32_t *out_val);

/**
 * @brief شماره مرحله فعال اقساط (0=آزاد، 1..5=مراحل)
 */
uint8_t day_lock_get_active_stage(void);

/**
 * @brief تعداد کل مراحل فعال یا تنظیم‌شده اقساط (1 تا 5)
 */
uint8_t day_lock_get_total_stages(void);

/**
 * @brief خواندن روزهای تنظیم‌شده برای یک مرحله
 */
int16_t day_lock_get_stage_days(uint8_t stage);

/**
 * @brief تنظیم روزهای یک مرحله (1 تا 5)
 */
void day_lock_set_stage_days(uint8_t stage, int16_t days);

/**
 * @brief آیا کد اضطراری یکبار مصرف قدیمی (112233) استفاده شده است؟
 */
bool day_lock_is_grace_used(void);

/**
 * @brief بررسی و اعمال رمز اضطراری ۱۰ روزه یکبار مصرف (11, 22 یا 33)
 * @param idx  اندیس رمز (0=«11», 1=«22», 2=«33»)
 * @return true در صورت پذیرش و اعمال، false اگر قبلاً مصرف شده
 */
bool day_lock_apply_grace10_indexed(uint8_t idx);

/**
 * @brief بررسی اینکه آیا کد اضطراری مورد نظر قبلاً مصرف شده است یا خیر
 */
bool day_lock_is_grace_indexed_used(uint8_t idx);

/**
 * @brief بررسی قفل کامل سیستم (Hard Lock یا Tamper Lock)
 */
bool day_lock_is_locked(void);

/**
 * @brief بررسی قفل نرم (Soft Lock: چرخه جاری تمام شود، چرخه جدید ممنوع)
 */
bool day_lock_is_soft_locked(void);

/**
 * @brief دریافت کد ۵ رقمی چالش جاری دستگاه
 */
uint32_t day_lock_get_active_challenge(void);

/**
 * @brief تولید کد چالش جدید شبه‌تصادفی مشتق از مک‌آدرس و زمان
 */
void day_lock_generate_challenge(void);

/**
 * @brief بررسی وضعیت فریز بروت‌فورس کی‌پد
 * @param out_remaining_sec ثانیه‌های باقیمانده فریز
 */
bool day_lock_is_brute_locked(uint32_t *out_remaining_sec);

/**
 * @brief دریافت تعداد تلاش‌های ناموفق جاری
 */
uint8_t day_lock_get_failed_attempts(void);

/**
 * @brief دریافت تعداد فرصت‌های باقیمانده تا مسدود شدن ۱۰ دقیقه‌ای کی‌پد
 */
uint8_t day_lock_get_remaining_attempts(void);

/**
 * @brief بازنشانی شمارنده تلاش‌های ناموفق
 */
void day_lock_reset_failed_attempts(void);

/**
 * @brief اعتبارسنجی رشته کد ورودی با لایه‌های ۴ گانه احراز هویت
 * @return true در صورت پذیرش کد
 */
bool day_lock_validate_pin(const char *pin);

/**
 * @brief شروع زنجیره اقساط ۵ مرحله‌ای با مقدار اولیه مرحله ۱
 */
void day_lock_arm_chain(int16_t days_t1);

/**
 * @brief پاک‌سازی کامل قفل و بازگشت به حالت کارخانه آزاد (Stage 0)
 */
void day_lock_clear_after_unlock(void);

/**
 * @brief پیشروی دستی به مرحله بعدی اقساط
 */
bool day_lock_advance_stage(void);

/**
 * @brief اعمال مهلت اضطراری ۱۰ روزه یکبار مصرف
 */
bool day_lock_apply_grace10(void);

/**
 * @brief تمدید دستی روزها بدون تغییر مرحله (مثلاً با کد چالش)
 */
void day_lock_extend_days(int16_t days);

/**
 * @brief محاسبه کد مرحله اقساط بر اساس ضریب مرحله
 */
void day_lock_calc_stage_code(char *out_buf, size_t buf_sz, uint8_t stage, const char *admin_pin);

/**
 * @brief محاسبه پاسخ چالش ۵ رقمی (Legacy)
 */
void day_lock_calc_challenge_response(char *out_buf, size_t buf_sz, uint32_t challenge);

/**
 * @brief محاسبه پاسخ چالش ۴ رقمی مدیر
 */
void day_lock_calc_challenge_response_4digit(char *out_buf, size_t buf_sz, uint32_t challenge);

/**
 * @brief محاسبه تاریخ شمسی با افزودن N روز به تاریخ امروز دستگاه
 */
void day_lock_get_future_jalali_str(int16_t add_days, char *out_str, size_t out_sz);

/**
 * @brief محاسبه تاریخ با افزودن N روز به تاریخ امروز (شمسی یا میلادی)
 */
void day_lock_get_future_date_str(int16_t add_days, char *out_str, size_t out_sz, bool is_jalali);

/**
 * @brief ذخیره و فعال‌سازی هم‌زمان زنجیره ۵ مرحله‌ای اقساط
 */
void day_lock_arm_5stages(const int16_t days[DAY_LOCK_MAX_STAGES]);

/**
 * @brief اطلاع‌رسانی وضعیت بی‌کاری (Idle) دستگاه جهت گذار از Soft Lock به Hard Lock
 */
void day_lock_notify_cycle_idle(bool is_idle);

/**
 * @brief بررسی نیاز به هشدار صوتی در بوت (<=3 روز یا مهلت اضطراری)
 */
void day_lock_check_boot_acoustic_alert(void);

/**
 * @brief بررسی خودکار رفع خطای تمپر ساعت در صورت تنظیم یا همگام‌سازی زمان معتبر
 * @return true در صورت رفع تمپر و آزادسازی، false در غیر این صورت
 */
bool day_lock_check_rtc_recovery(void);

/**
 * @brief اطلاع‌رسانی تنظیم دستی زمان جهت به‌روزرسانی مونوتونیک بدون افتادن در خطای تمپر کاذب
 * @param new_epoch زمان جدید (0 = خواندن خودکار از DS3231/سیستم)
 */
void day_lock_notify_time_adjusted(uint32_t new_epoch);

/**
 * @brief بررسی تسویه و تایید بودن یک مرحله از اقساط
 */
bool day_lock_is_stage_settled(uint8_t stage);

/**
 * @brief دریافت تاریخ شمسی تسویه یک مرحله از اقساط
 */
bool day_lock_get_stage_settled_date(uint8_t stage, char *out_str, size_t out_sz);

/**
 * @brief ثبت تاریخ روز جاری به عنوان تاریخ تسویه یک مرحله از اقساط در NVS
 */
void day_lock_record_stage_settled(uint8_t stage);

/**
 * @brief پاک‌سازی وضعیت تسویه یک مرحله (در صورت افزودن روز توسط کاربر)
 */
void day_lock_clear_stage_settled(uint8_t stage);

#ifdef __cplusplus
}
#endif
