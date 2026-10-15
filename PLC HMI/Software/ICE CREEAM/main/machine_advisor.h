#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* ۱. هوشمندی به‌روزرسانی OTA (OTA Smart Snooze)                             */
/* -------------------------------------------------------------------------- */
#define OTA_MAX_BOOT_DISMISS_COUNT  3

bool     machine_advisor_is_ota_snoozed(void);
int32_t  machine_advisor_get_ota_dismiss_count(void);
void     machine_advisor_record_ota_dismiss(void);
void     machine_advisor_reset_ota_dismiss(void);

/* -------------------------------------------------------------------------- */
/* ۲. بای‌پس موقت سنسور القایی دور پولی (E01 Sensor Bypass)                  */
/* -------------------------------------------------------------------------- */
#define PULLEY_BYPASS_DEFAULT_SEC   (4 * 3600) /* ۴ ساعت بای‌پس موقت برای حفظ خط تولید */

bool     machine_advisor_is_pulley_bypassed(void);
uint32_t machine_advisor_get_pulley_bypass_remain_s(void);
void     machine_advisor_enable_pulley_bypass(uint32_t seconds);
void     machine_advisor_disable_pulley_bypass(void);

/* -------------------------------------------------------------------------- */
/* ۳. حالت اضطراری برودت سنسور دمای مخزن T1 (E04 Safe Duty Fallback)         */
/* -------------------------------------------------------------------------- */
bool     machine_advisor_is_thermo_fallback_active(void);
void     machine_advisor_set_thermo_fallback(bool enable);

/* -------------------------------------------------------------------------- */
/* ۴. سیستم ضد سواستفاده و حفاظت حرارتی اورلود (Overload Anti-Abuse System)   */
/* -------------------------------------------------------------------------- */
#define OVERLOAD_COOLDOWN_DURATION_S   (30 * 60) /* ۳۰ دقیقه مهلت خنک‌سازی سیم‌پیچ */
#define OVERLOAD_STRIKE_WINDOW_S       (2 * 3600) /* ۲ ساعت پنجره زمانی پایش خطاهای پیاپی اورلود */

typedef enum {
    OVERLOAD_STATUS_NORMAL = 0,     /* وضعیت عادی بدون تریپ اخیر */
    OVERLOAD_STATUS_WARNED = 1,     /* تریپ اول - ریست استاندارد مجاز است */
    OVERLOAD_STATUS_COOLDOWN = 2,   /* تریپ دوم - قفل حرارتی ۳۰ دقیقه‌ای */
    OVERLOAD_STATUS_HARD_LOCK = 3   /* تریپ سوم - قفل سخت با کد چالش */
} overload_status_t;

void     machine_advisor_init(void);
void     machine_advisor_tick_1s(void);

overload_status_t machine_advisor_get_overload_status(void);
uint8_t  machine_advisor_get_overload_strikes(void);
uint32_t machine_advisor_get_cooldown_remain_s(void);
uint32_t machine_advisor_get_window_remain_s(void);
bool     machine_advisor_is_cooldown_active(void);
bool     machine_advisor_is_overload_locked(void);
uint32_t machine_advisor_get_overload_challenge(void);

/* ثبت وقوع تریپ اضافه‌بار (E02 یا E03) */
void     machine_advisor_record_overload(uint8_t fault_code);

/* تایید سلامت ۶۰ دقیقه‌ای دستگاه و پاک‌سازی تدریجی استرایک‌ها */
void     machine_advisor_on_healthy_run_60m(void);

/* اعتبارسنجی کد بازگشایی چالش قفل اورلود */
bool     machine_advisor_verify_unlock_code(const char *code_str);

/* ریست کامل قفل و استرایک‌ها توسط ادمین / مستر کد */
void     machine_advisor_force_reset_overload(void);

/* -------------------------------------------------------------------------- */
/* ۵. دستیار هوشمند تمیزی کندانسور (Smart Condenser Clean Advisor)           */
/* -------------------------------------------------------------------------- */
#define CONDENSER_MAX_FREEZE_TIME_S (12 * 60) /* حداکثر زمان مجاز انجماد تا اعلام غبارگرفتگی رادیاتور */

bool     machine_advisor_is_condenser_clean_warn(void);
void     machine_advisor_reset_condenser_clean_warn(void);
void     machine_advisor_notify_freeze_start(void);
void     machine_advisor_notify_freeze_done(void);

/* -------------------------------------------------------------------------- */
/* ۶. پایش پایداری ولتاژ شبکه و ثبت افت ولتاژ (Mains Grid Stability Logger)    */
/* -------------------------------------------------------------------------- */
uint32_t machine_advisor_get_brownout_count(void);
void     machine_advisor_record_brownout(void);

/* -------------------------------------------------------------------------- */
/* ۷. حالت هوشمند اکو در ساعات خلوتی (Dynamic Eco-Idle Mode)                 */
/* -------------------------------------------------------------------------- */
#define ECO_IDLE_TRIGGER_S          (30 * 60) /* ۳۰ دقیقه عدم توزیع قیف = ورود به حالت کم‌مصرف اکو */

bool     machine_advisor_is_eco_idle(void);
uint32_t machine_advisor_get_idle_dispense_s(void);
void     machine_advisor_notify_dispense(void);

/* -------------------------------------------------------------------------- */
/* ۸. ردگیر ساعات کارکرد و سرویس قطعات مصرفی (Consumables Lifetime Tracker)  */
/* -------------------------------------------------------------------------- */
#define SERVICE_INTERVAL_MOTOR_HOURS 100 /* هر ۱۰۰ ساعت کارکرد موتور: سرویس و گریس‌کاری بهداشتی */

uint32_t machine_advisor_get_comp_hours(void);
uint32_t machine_advisor_get_motor_hours(void);
uint32_t machine_advisor_get_service_remain_hours(void);
void     machine_advisor_reset_service_interval(void);
void     machine_advisor_track_actuator_seconds(bool comp_active, bool motor_active);

/* -------------------------------------------------------------------------- */
/* ۹. موتور هوشمند پایش تکرار خطا و هدایت تشخیصی (Fault Recurrence & Smart Escalation) */
/* -------------------------------------------------------------------------- */
#define FAULT_RECURRENCE_WINDOW_S     3600 /* پنجره زمانی ۱ ساعته پایش تکرار */
#define FAULT_RECURRENCE_THRESHOLD    3    /* آستانه تکرار: ۳ بار رخداد در یک ساعت */

void     machine_advisor_record_fault(uint8_t fault_code);
uint8_t  machine_advisor_get_fault_strikes(uint8_t fault_code);
bool     machine_advisor_is_fault_escalated(uint8_t fault_code);
void     machine_advisor_clear_fault_strikes(uint8_t fault_code);
bool     machine_advisor_get_escalated_advice(uint8_t fault_code,
                                              const char **out_title,
                                              const char **out_cause,
                                              char *sol_buf, size_t sol_buf_size);

#ifdef __cplusplus
}
#endif


