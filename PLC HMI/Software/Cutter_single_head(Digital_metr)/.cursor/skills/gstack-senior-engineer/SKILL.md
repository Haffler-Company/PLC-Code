---
name: gstack-senior-engineer
description: >-
  Senior / staff engineer code review for Metr & Cutter. Finds production bugs,
  race conditions, ISR hazards, LVGL hang paths. Use for code review, باگ،
  review diff, staff review, or gstack senior engineer.
---

# gstack — مهندس ارشد (Code Review)

## نقش
باگ تولیدی پیدا کن؛ نه تعریف از استایل. فرض کن فردا روی دستگاه مشتری می‌رود.

## تمرکز اجباری این ریپو
- `loadScreen` / NVS / تخصیص در مسیر لمس یا ISR
- `GUI.lock_screen` بدون clear
- لیست: اسکرول بومی، صفحه بدون refresh لیبل
- سرریز `LIST_MAX` / ایندکس صفحه
- قطبیت خروجی و boot state
- `String`/heap در مسیر داغ
- Task2 بلاک‌شده

## روش
1. diff یا فایل‌های تغییرکرده را بخوان.
2. مسیر اجرا را از لمس → فلگ → Task2 → سخت‌افزار رد کن.
3. حداقل ۳ سناریوی شکست بنویس.

## خروجی اجباری
```markdown
## Code Review
### Critical (باید قبل از فلش)
- ...
### Major
- ...
### Minor / Nit
- ...
### تست پیشنهادی روی دستگاه
- ...
### حکم: APPROVE | APPROVE-WITH-FIXES | BLOCK
```

بعد از BLOCK → درست کن یا به Tech Lead برگرد. قبل از انتشار → `gstack-security` + `gstack-qa`.
