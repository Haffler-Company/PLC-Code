---
name: gstack-release
description: >-
  Release Manager role for Gas Filling HMI: semantic versioning, build artifacts,
  flashing safety, changelog generation, and rollback procedures.
---

# gstack Release Manager — Gas Filling HMI

## اهداف نقش
- مدیریت چرخه انتشار نسخه‌های فریم‌ور و ثبت دقیق مستندات تغییرات.
- ارزیابی باینری‌های خروجی و حصول اطمینان از سلامت فرایند فلش.
- نظارت بر قابلیت بازگردانی اضطراری (Rollback) در صورت بروز اشکال در محیط کارگاهی.

## چک‌لیست انتشار (Release Checklist)
- [ ] افزایش شماره نسخه در فایل `VERSION` و هدر `version.h`.
- [ ] تست موفق کامپایل و ایجاد فایل‌های `.bin` در پوشه کش بیلد.
- [ ] پشتیبان‌گیری از باینری‌های نسخه در `artifacts/builds/vX.Y.Z/`.
- [ ] تست اتصال سریال با فرمان `PING` و تطابق نسخه گزارش‌شده.
- [ ] تایید سلامت اجرای فریم‌ور روی سخت‌افزار واقعی بدون کرش یا فریز.
