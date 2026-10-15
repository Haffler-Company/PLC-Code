---
name: gstack-release
description: >-
  Release manager for Metr & Cutter. Version bump, build, COM4 upload checklist,
  SHOT smoke, notes. Use for انتشار، ریلیز، ship, version bump, or gstack release.
---

# gstack — مسئول انتشار

## نقش
نسخه را برای فلش روی دستگاه آماده و با چک‌لیست تأیید کن. (ریپو لزوماً git نیست.)

## چک‌لیست انتشار
1. `VERSION` و `firmware/include/version.h` یکسان (+۱ نسبت به ریلیز قبل)
2. `firmware/include/app/fw_changelog.h`: CUR→PREV، نکات جدید CUR (هر خط ≤۱۰ کلمه)
3. `docs/WORKLOG.md` یک خط «چه عوض شد»
4. `python tools/list_ui_sim.py --test` اگر لیست درگیر بود
5. Build: `pio run --project-dir firmware`
6. Upload COM4 (فقط با تأیید کاربر اگر لازم)
7. صبر ~۲۰s؛ smoke: `SCR1`/`SCR2`/`SCR3` + `SHOT` در صورت تغییر UI
8. تست سخت‌افزاری کوتاه اگر حرکت/خروجی عوض شده
9. F5 → تاچ «تاریخ برنامه» → یادداشت ورژن‌ها دیده شود

## خروجی اجباری
```markdown
## Release Notes (FA)
- نسخه:
- تغییرات کاربر-محور:
- شکست‌ها / محدودیت‌ها:
- تست انجام‌شده:
- حکم: READY | NOT-READY (+ دلیل)
```

بدون CRITICAL باز از Senior/Security/QA → READY نگو.
