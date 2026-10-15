---
name: gas-filling-build-flash
description: >-
  Build, compilation, flashing, and recovery procedures for Gas Filling ESP32-S3 firmware.
---

# Gas Filling Build & Flash Skill

## دستورات عملیاتی
```powershell
# اجرای بیلد کامل و آپلود با افزایش پچ نسخه
.\scripts\program.ps1 -Port COM3 -Knowledge "<یافته جدید>"

# بیلد سریع فقط برای تغییرات اسکچ (نگه‌داشتن کش کتابخانه‌ها)
.\scripts\program.ps1 -Port COM3 -Force -Knowledge "<یافته جدید>"

# فلش مستقیم باینری بدون کامپایل مجدد
.\scripts\upload.ps1 -Port COM3 -SkipBuild -Knowledge none

# در صورت عدم پاسخ‌دهی پورت سریال، اجرای FullFlash
.\scripts\upload.ps1 -Port COM3 -SkipBuild -FullFlash -Knowledge none
```

## رفع خطاهای رایج کامپایلر
- خطای `undefined reference to ui_process_isr_requests`: بررسی وجود `extern "C"` در `variables.h`.
- خطای `TinyTTF stall`: غیرفعال ماندن لودر فونت ترو‌تایپ در حین بوت تا زمان راه‌اندازی بافر نمایشگر.
- خطای `SHOT_ERR alloc_psram`: اطمینان از تنظیم `PSRAM=enabled` در FQBN.
