#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_timer_cb_t)(void *user_ctx);

/** راه‌اندازی تایمر 1Hz (Auto-reload). */
bool app_timers_init(void);

/** تعیین/تعویض تابع هوک که هر ثانیه صدا می‌خورد. */
void app_timers_set_hook(app_timer_cb_t cb, void *user_ctx);

/**
 * A3: ثبت handle تسک actions برای دریافت notification 1Hz.
 * باید بلافاصله پس از ایجاد actions_task فراخوانی شود.
 */
void app_timers_set_actions_task(TaskHandle_t task_handle);

/** بازگرداندن بیت notification مربوط به تایمر 1Hz. */
uint32_t app_timers_get_notify_bits(void);

/** A3: اجرای hook در context تسک فراخواننده (نه timer task). */
void app_timers_run_hook(void);

/** توقف و آزادسازی (اختیاری). */
void app_timers_deinit(void);

#ifdef __cplusplus
}
#endif
