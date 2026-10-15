#include "../include/lv_conf.h"
#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

#include "lvgl.h"
#include <esp_heap_caps.h>
#include <stddef.h>

void lv_mem_init(void) {}
void lv_mem_deinit(void) {}

// مهم: اول از PSRAM بگیر، بعد در صورت نبودِ PSRAM از RAM داخلی. قبلاً برعکس بود (اول داخلی) →
// LVGL کلِ ~۲۳۰KB رمِ داخلی را پُر می‌کرد و برای درایورِ WiFi و استکِ تسکِ اسکن (که فقط از رمِ
// داخلی قابل‌تخصیص‌اند) چیزی نمی‌ماند → «WifiScanStart task create failed» و لیستِ وای‌فای خالی.
// بافرهای DMAِ نمایشگر جداگانه با MALLOC_CAP_INTERNAL تخصیص می‌شوند، پس این تغییر سرعتِ رندر را کم نمی‌کند.
void * lv_malloc_core(size_t size)
{
    return heap_caps_malloc_prefer(size, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

void * lv_realloc_core(void * p, size_t new_size)
{
    return heap_caps_realloc_prefer(p, new_size, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

void lv_free_core(void * p)
{
    heap_caps_free(p);
}

void lv_mem_monitor_core(lv_mem_monitor_t * mon_p)
{
    if (!mon_p) return;
    mon_p->total_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    mon_p->free_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    mon_p->used_pct = (mon_p->total_size > 0)
        ? (uint8_t)(100 - (mon_p->free_size * 100 / mon_p->total_size))
        : 0;
}

lv_result_t lv_mem_test_core(void)
{
    return LV_RESULT_OK;
}

#endif
