---
name: gstack
description: >-
  gstack team router for Metr & Cutter. Routes to CEO strategist, tech lead,
  product designer, senior engineer review, QA, security, or release manager.
  Use when user says gstack, تیم مجازی, مدیرعامل, مدیر فنی, طراح محصول,
  code review, QA, امنیت, انتشار, or asks which specialist should handle work.
---

# gstack — تیم مجازی Metr & Cutter

الهام از [garrytan/gstack](https://github.com/garrytan/gstack)، بومی‌شده برای فریمور ESP32-S3 / LVGL / ماشین برش.

## مسیریابی (حتماً skill مربوط را Read کن)

| درخواست کاربر | Skill |
|---------------|--------|
| ایده، دامنه، اولویت محصول، «چی بسازیم» | `gstack-ceo-strategist` |
| معماری، تسک‌ها، پین، LVGL/FreeRTOS design | `gstack-tech-lead` |
| UI/UX، لی‌اوت، اپل‌محور، SHOT بصری | `gstack-product-designer` |
| Code review، باگ، diff، رگرسیون | `gstack-senior-engineer` |
| تست واقعی، شبیه‌ساز، SCR/SHOT، سناریو | `gstack-qa` |
| آسیب‌پذیری، وب‌پورتال، سریال، NVS | `gstack-security` |
| نسخه، آپلود، چک‌لیست انتشار | `gstack-release` |

مسیرها: `.cursor/skills/gstack-*/SKILL.md`

## قانون تیم
- یک نقش در هر پاسخ اصلی؛ اگر کار چندمرحله است، ترتیب پیشنهادی را بگو سپس نقش فعلی را اجرا کن.
- ترتیب اسپرینت پیشنهادی: CEO → Tech Lead → Designer → (پیاده‌سازی) → Senior Review → Security → QA → Release.
- همیشه محدودیت‌های ماشین واقعی را حفظ کن (Active-LOW، بدون تغییر پین بی‌اجازه، LVGL 9.4، بدون `loadScreen` در تاچ).
- پاسخ به کاربر فارسی و عملی.
