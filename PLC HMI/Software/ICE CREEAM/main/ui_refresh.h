#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

// هرجا وضعیت برنامه عوض شد، فقط اینو صدا بزن:
void ui_request_apply(void);          // فقط اعمال تغییراتِ لازم روی ویجت‌های تغییرکرده
void ui_request_full_redraw(void);    // در صورت نیاز: رفرش کامل صفحه

// این رو مطابق پروژه‌ی خودت پیاده کن (در ui_refresh.c):
void ui_apply_state(void);
