---
name: gas-filling-hmi
description: >-
  Core firmware & HMI engineering skill for Gas Filling machine: screens, actions,
  variables, LVGL event handlers, dual glass channels, and serial agent bridge.
---

# Gas Filling HMI Engineering Skill

مرجع فنی: `docs/REFERENCE.md` | عملیات: `docs/OPERATIONS.md`

## ساختار لایه‌های کاربری فریم‌ور
- `screens.c` / `screens.h`: تعریف اشیاء، کارت‌ها، کانتینرها، دکمه‌های شروع و توقف، نوارهای پیشرفت و پنجره‌های مودال.
- `actions.c` / `actions.h`: توابع پاسخ به رویدادها (`action_start_button_pressed`, `action_stop_button_pressed`, `action_gotosetting`, `action_gotohome`, `ui_clear_dimensions`).
- `variables.c` / `variables.h`: متغیرهای وضعیت، تایمرهای شیرها (`TimerValve1`, `TimerValve2`), مقادیر ذخیره شده (`TimerValve1_Saved`, `TimerValve2_Saved`) و فلگ‌های همگام‌سازی وقفه.

## نکات کلیدی برنامه‌نویسی HMI
1. برای باز کردن کیبورد مجازی از `ui_open_main_keyboard` استفاده کن و حتماً با `lv_obj_move_foreground` پنل کیبورد را به بالاترین لایه بیاور.
2. برای پاک کردن مقادیر کانال از تابع امن `ui_clear_dimensions(station)` استفاده کن.
3. در زمان تغییر صفحات همیشه انتقال را به صورت آسنکرون (`lv_async_call`) انجام بده.
