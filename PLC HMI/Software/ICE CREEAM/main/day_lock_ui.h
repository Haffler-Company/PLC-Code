#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief نمایش مودال تمام‌صفحه قفل روزشمار با تم سفید اپل
 */
void day_lock_ui_show(void);

/**
 * @brief بستن و حذف مودال قفل پس از بازگشایی موفق
 */
void day_lock_ui_hide(void);

/**
 * @brief آیا مودال قفل در حال حاضر بر روی صفحه نمایش است؟
 */
bool day_lock_ui_is_visible(void);

/**
 * @brief نمایش پنجره تنظیم ۵ تایمر اقساط به همراه محاسبه تاریخ شمسی
 */
void day_lock_ui_show_5timers_dialog(void);

/**
 * @brief بستن پنجره تنظیم ۵ تایمر اقساط
 */
void day_lock_ui_hide_5timers_dialog(void);

/**
 * @brief پردازش کلیدهای ناوبری فیزیکی پنل (MODE, START, STOP, F-Keys) در صورت خرابی تاچ
 */
void day_lock_ui_handle_physical_key(uint8_t row, uint8_t key);

/**
 * @brief به‌روزرسانی متن و تاریخ ۵ تایمر اقساط هنگام تغییر زبان
 */
void day_lock_ui_refresh_5timers(void);

/**
 * @brief اعلام نتیجه همگام‌سازی ساعت با اینترنت به مودال قفل
 */
void day_lock_ui_notify_time_sync(bool success);

#ifdef __cplusplus
}
#endif
