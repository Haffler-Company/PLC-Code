# PROJECT SNAPSHOT — Aluminum Punching (پانچ آلومینیوم)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## وضعیت پروژه
> ⚠️ **در انتظار کد** — ساختار مستندات آماده است.

## سخت‌افزار هدف
- **MCU:** ESP32-S3 (N16R8) — 16MB Flash، 8MB PSRAM
- **Display:** ILI9488 3.5"، تاچ خازنی GT911
- **Framework:** ESP-IDF/Arduino + LVGL

## پین‌اوت (HAFFLER ETV3A Standard — Active-LOW)

| GPIO | سیگنال | عملکرد |
|---|---|---|
| 16 | Q0 CLAMP | کلمپ پروفیل آلومینیوم |
| 42 | Q1 PUNCH_DOWN | پانچ پایین (پرس) |
| 39 | Q2 PUNCH_UP | پانچ بالا (بازگشت) |
| 48 | Q3 FEED | پیشروی قطعه |
| 40 | Q4 EJECT | خروج/پران قطعه |
| 15 | Q5 MOTOR | موتور هیدرولیک یا پنوماتیک |
| 45 | BUZZER | بوق |
| 3 | E-STOP | توقف اضطراری — قطع همه |
| 7 | START | شستی شروع |
| 5 | HOME_PUNCH | سنسور خانه پانچ |
| 4 | STROKE_SENSOR | سنسور کنترل ضربه |
| 6 | COUNTER_SENSOR | شمارنده پانچ |

## توالی (Safety-Critical FSM)

```
۱. کلمپ CLAMP
۲. پیشروی FEED به موقعیت
۳. پانچ: PUNCH_DOWN
۴. تأیید STROKE_SENSOR
۵. PUNCH_UP → Home
۶. EJECT (اختیاری)
۷. باز شدن CLAMP
```

## خطوط قرمز — ABSOLUTE

- ❌ هرگز PUNCH_DOWN و PUNCH_UP هم‌زمان فعال (مکانیکی بسیار خطرناک)
- ❌ E-Stop → PUNCH_UP فوری + قطع موتور + CLAMP باز
- ❌ هرگز FEED بدون CLAMP فعال حرکت کند
- ✅ شمارنده بر اساس COUNTER_SENSOR — نه نرم‌افزاری

## الگوی پیاده‌سازی
از **Bakery Mixer** (`bakery-mixer/`) به عنوان Reference Architecture استفاده شود.

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/aluminum-punching/`
- **مرجع:** `HAFFLER_CLOUD_INTEGRATION_AND_TODO.md`، `docs/`
