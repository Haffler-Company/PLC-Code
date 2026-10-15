# PROJECT SNAPSHOT — Corner Cleaning (تمیزکننده گوشه جوش UPVC)
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
| 16 | Q0 CLAMP_H | کلمپ افقی پروفیل |
| 42 | Q1 CLAMP_V | کلمپ عمودی پروفیل |
| 39 | Q2 CHISEL_TOP | تیغچه تمیزکننده رو |
| 48 | Q3 CHISEL_BOTTOM | تیغچه تمیزکننده زیر |
| 40 | Q4 CORNER_SAW | تیغه شیارزن گوشه بیرونی |
| 15 | Q5 DRILL | دریل سوراخ آبراه (اختیاری) |
| 35 | SAW_MOTOR | موتور اره |
| 45 | BUZZER | بوق |
| 3 | E-STOP | توقف اضطراری |
| 7 | START | شستی شروع |
| 5 | HOME_CHISEL | سنسور خانه تیغچه |
| 4 | HOME_SAW | سنسور خانه اره |

## توالی پنوماتیک (Safety-Critical FSM)

```
۱. کلمپ CLAMP_H + CLAMP_V
۲. تیغچه‌های رو و زیر (CHISEL_TOP + CHISEL_BOTTOM)
۳. ورود CORNER_SAW
۴. [اختیاری] DRILL
۵. بازگشت ALL تیغه‌ها به Home
۶. باز شدن کلمپ‌ها
```

## خطوط قرمز — ABSOLUTE

- ❌ **Interlock برخورد:** CHISEL_TOP/BOTTOM و CORNER_SAW هرگز هم‌زمان در zone
- ❌ E-Stop → بازگشت فوری همه جک‌ها به Home + قطع موتور اره
- ❌ هرگز کلمپ‌ها باز شوند قبل از Home شدن همه تیغه‌ها
- ✅ Active-LOW: مقداردهی HIGH در بوت

## الگوی پیاده‌سازی
از **Bakery Mixer** (`bakery-mixer/`) به عنوان Reference Architecture استفاده شود.

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/upvc/corner-cleaning/`
- **مرجع:** `HAFFLER_CLOUD_INTEGRATION_AND_TODO.md`، `docs/`
