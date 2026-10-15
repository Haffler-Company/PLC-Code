#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FAULT_LOG_MAX_ENTRIES 10

typedef enum {
    FAULT_NONE = 0,
    FAULT_MOTOR_OVERLOAD = 1,     /* کنترل‌بار موتور پارو */
    FAULT_COMP_OVERLOAD = 2,      /* کنترل‌بار کمپرسور برودت */
    FAULT_EMERGENCY_STOP = 3,     /* شستی قطع اضطراری پنل */
    FAULT_INDUCTIVE_STALL = 4,    /* توقف چرخش پولی گیربکس */
    FAULT_LEASE_EXPIRED = 5,      /* انقضای اعتبار لایسنس */
    FAULT_TORQUE_PREDICT = 6,     /* آلارم پیش‌بینی فشار بار موتور */
    FAULT_FREEZE_TIMEOUT = 7,     /* زمان بیشینه انجماد بدون تریپ عقب */
    FAULT_STUCK_LEVER    = 8,     /* باز ماندن غیرعادی اهرم تخلیه */
    FAULT_STUCK_REAR     = 9,     /* درگیر بودن میکروسوئیچ عقب در استارت */
    FAULT_THERMO_TOP_DISCONNECT = 10, /* قطعی سنسور دمای مخزن شیر T1 */
    FAULT_THERMO_CYL_DISCONNECT = 11, /* قطعی سنسور دمای سیلندر انجماد T2 */
    FAULT_BROWNOUT              = 12, /* افت ولتاژ برق ورودی (Brownout Reset) */
    FAULT_WATCHDOG_RESET        = 13, /* تریپ واچ‌داگ سخت‌افزاری/نرم‌افزاری */
    FAULT_CORE_PANIC            = 14, /* کرش هسته پردازنده (Panic Reset) */
    FAULT_RTC_ERROR             = 15, /* خطای ارتباط با ساعت سخت‌افزاری DS3231 */
    FAULT_TOUCH_ERROR           = 16, /* خطای ارتباط با کنترلر تاچ GT911 */
    FAULT_MEMORY_ERROR          = 17, /* خطای حافظه رم یا هیپ پردازنده */
    FAULT_CPU_OVERHEAT          = 18, /* دمای بیش از حد مجاز پردازنده */
    FAULT_MAX6675_TOP_MISSING   = 19, /* عدم پاسخ یا قطعی آی‌سی MAX6675 مخزن شیر T1 */
    FAULT_MAX6675_CYL_MISSING   = 20, /* عدم پاسخ یا قطعی آی‌سی MAX6675 سیلندر انجماد T2 */
} fault_code_t;

typedef struct {
    uint32_t timestamp; /* unix timestamp from DS3231 RTC */
    uint8_t  code;      /* fault_code_t */
    int16_t  value;     /* مقدار الکتریکال یا شماره خطای مربوطه */
} fault_entry_t;

void fault_log_init(void);
void fault_log_record(fault_code_t code, int16_t value);
int  fault_log_get_count(void);
bool fault_log_get_entry(int index, fault_entry_t *out);
void fault_log_clear(void);
const char* fault_code_to_str(fault_code_t code);
const char* fault_code_to_isa_str(fault_code_t code);
bool fault_log_has_unread(void);
void fault_log_mark_read(void);
uint32_t fault_log_get_total_count(void);

#ifdef __cplusplus
}
#endif
