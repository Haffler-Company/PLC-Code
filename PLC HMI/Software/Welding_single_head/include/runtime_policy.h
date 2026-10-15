#ifndef RUNTIME_POLICY_H
#define RUNTIME_POLICY_H

#include <stdint.h>

// Central runtime tuning points for deterministic behavior.
// Keep timing and rendering policy here to avoid hidden magic numbers.
namespace runtime_policy {

// LVGL scheduling — استاندارد بهینه زیمنس/اپل:
// بازه ۱ تا ۸ میلی‌ثانیه برای حداکثر واکنش‌گرایی UI (۶۰ فریم بر ثانیه) و پاسخ آنی به لمس و رویدادها.
constexpr uint32_t LVGL_DELAY_MIN_MS = 1;
constexpr uint32_t LVGL_DELAY_MAX_MS = 8;

// DMA روی این برد صفحه را خراب کرده — همیشه خاموش بماند (مثل Config).
constexpr bool EXPERIMENTAL_DMA_FLUSH = false;
constexpr bool EXPERIMENTAL_DMA_SWAP_BYTES = false;
constexpr uint32_t UI_TICK_TOUCH_MS = 16;
constexpr uint32_t UI_TICK_ACTIVE_MS = 33;
constexpr uint32_t UI_TICK_IDLE_MS = 60;

// Web app task polling period:
// ۱5 میلی‌ثانیه برای کاهش بار پردازشی هسته ۰ در حالت idle.
constexpr uint32_t WEBAPP_LOOP_DELAY_MS = 15;

} // namespace runtime_policy

#endif // RUNTIME_POLICY_H
