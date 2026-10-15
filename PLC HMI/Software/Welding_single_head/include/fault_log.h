/**
 * @file fault_log.h
 * @brief Industrial ISA-18.2 Compliant Fault Logging & Diagnostics Engine
 * @author Haffler Automation Systems
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FAULT_LOG_MAX_ENTRIES 10

typedef enum {
    FAULT_NONE = 0,
    FAULT_EMERGENCY_STOP    = 1,   /* شستی قطع اضطراری پنل (E-Stop) */
    FAULT_THERMO_DISCONNECT = 2,   /* قطعی یا خطای سنسور ترموکوپل MAX6675 */
    FAULT_HEATER_TIMEOUT    = 3,   /* خطای زمان گرمایش بیش از حد المنت */
    FAULT_OVERTEMP          = 4,   /* دمای بیش از حد مجاز المنت (>270°C) */
    FAULT_SHABLON_TIMEOUT   = 5,   /* خطای زمان رسیدن جک شابلون */
    FAULT_ELEMENT_TIMEOUT   = 6,   /* خطای زمان رسیدن جک المنت */
    FAULT_TABLE_TIMEOUT     = 7,   /* خطای زمان رسیدن جک میز */
    FAULT_RTC_ERROR         = 8,   /* خطای ارتباط با ساعت سخت‌افزاری DS3231 */
    FAULT_TOUCH_ERROR       = 9,   /* خطای ارتباط با کنترلر لمسی GT911 */
    FAULT_LEASE_EXPIRED     = 10,  /* انقضای مهلت تست یا لایسنس روزانه */
    FAULT_BROWNOUT          = 11,  /* افت ولتاژ شبکه (Brownout Reset) */
    FAULT_WATCHDOG_RESET    = 12,  /* ریست ناشی از واچ‌داگ (Watchdog Reset) */
} fault_code_t;

typedef struct {
    uint32_t timestamp; /* unix timestamp or system uptime seconds */
    uint8_t  code;      /* fault_code_t */
    int16_t  value;     /* Associated electrical or process value */
} fault_entry_t;

void fault_log_init(void);
void fault_log_record(fault_code_t code, int16_t value);
int  fault_log_get_count(void);
bool fault_log_get_entry(int index, fault_entry_t *out);
void fault_log_clear(void);
const char* fault_code_to_str(fault_code_t code, int lang);
const char* fault_code_to_isa_str(fault_code_t code);
bool fault_log_has_unread(void);
void fault_log_mark_read(void);
uint32_t fault_log_get_total_count(void);

#ifdef __cplusplus
}
#endif
