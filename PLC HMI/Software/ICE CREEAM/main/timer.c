/* timer.c — بهینه‌سازی شده
 * A3: tick_cb دیگر مستقیماً one_hz_hook را فراخوانی نمی‌کند.
 *     در عوض یک task notification به actions_task می‌فرستد.
 *     actions_task در هر ثانیه max6675_poll() را اجرا می‌کند.
 *     این الگو با مستندات FreeRTOS کاملاً مطابقت دارد:
 *     "Timer callbacks must never block or run for a long time."
 */

#include "timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"

static TimerHandle_t  s_tick_timer  = NULL;
static app_timer_cb_t s_hook        = NULL;
static void          *s_hook_ctx    = NULL;

/* Handle تسک actions برای ارسال notification — در main.c ست می‌شود */
static TaskHandle_t   s_actions_task_handle = NULL;

/* بیت notification برای 1Hz tick */
#define APP_TIMER_NOTIFY_BIT   (1UL << 0)

#define APP_TICK_MS 1000

static void tick_cb(TimerHandle_t xTimer)
{
    (void)xTimer;

    /*
     * A3: به جای فراخوانی مستقیم one_hz_hook (که max6675_poll() را
     * با mutex و vTaskDelay‌های طولانی صدا می‌زند)، فقط یک notification
     * به actions_task می‌فرستیم.
     * actions_task این بیت را بررسی و one_hz_hook را در context خودش اجرا می‌کند.
     */
    if (s_actions_task_handle) {
        xTaskNotify(s_actions_task_handle,
                    APP_TIMER_NOTIFY_BIT,
                    eSetBits);
    } else {
        /* اگر هنوز actions_task ست نشده (مرحله boot)، مستقیم اجرا کن */
        if (s_hook) s_hook(s_hook_ctx);
    }
}

void app_timers_set_actions_task(TaskHandle_t task_handle)
{
    s_actions_task_handle = task_handle;
}

bool app_timers_init(void)
{
    if (s_tick_timer) return true;
    s_tick_timer = xTimerCreate("app_1hz",
                                pdMS_TO_TICKS(APP_TICK_MS),
                                pdTRUE,
                                NULL,
                                tick_cb);
    if (!s_tick_timer) return false;
    return (xTimerStart(s_tick_timer, 0) == pdPASS);
}

void app_timers_set_hook(app_timer_cb_t cb, void *user_ctx)
{
    s_hook = cb;
    s_hook_ctx = user_ctx;
}

uint32_t app_timers_get_notify_bits(void)
{
    return APP_TIMER_NOTIFY_BIT;
}

void app_timers_run_hook(void)
{
    if (s_hook) {
        s_hook(s_hook_ctx);
    }
}

void app_timers_deinit(void)
{
    if (s_tick_timer) {
        xTimerStop(s_tick_timer, pdMS_TO_TICKS(100));
        xTimerDelete(s_tick_timer, pdMS_TO_TICKS(100));
        s_tick_timer = NULL;
    }
    s_hook = NULL;
    s_hook_ctx = NULL;
    s_actions_task_handle = NULL;
}
