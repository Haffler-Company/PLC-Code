#ifndef UI_RESUME_H
#define UI_RESUME_H

#include <Arduino.h>
#include <esp_system.h>
#include "screens.h"

#define UI_RESUME_SCR_MAGIC 0xA5C3E147

#ifdef __cplusplus
extern "C" {
#endif

// توابع مدیریت بازیابی و ثبت ورود/خروج صفحات در حافظه RTC Slow
void ui_resume_boot_latch_early(void);
bool ui_resume_reset_is_hang(esp_reset_reason_t rr);
void ui_resume_record_screen_entry(int16_t screen_id);
void ui_resume_record_screen_exit(int16_t from_screen_id, int16_t to_screen_id);
bool ui_resume_boot_should_restore(int16_t *out_screen);
int16_t ui_resume_boot_get_screen(void);
void ui_resume_notify_screen_applied(void);
void ui_resume_clear(void);
bool ui_resume_is_hang_recovery(void);

// توابع سازگار برای فراخوانی‌های موجود در کد
void rtc_save_active_screen(int16_t screen_id);
int16_t get_saved_active_screen(void);

#ifdef __cplusplus
}
#endif

#endif // UI_RESUME_H
