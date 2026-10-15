# PROJECT SNAPSHOT — Copy Router (روتر کپی آلومینیوم/UPVC)
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
| 16 | Q0 CLAMP_WORK | کلمپ قطعه کار |
| 42 | Q1 CLAMP_COPY | کلمپ الگو (Copy) |
| 39 | Q2 ROUTER_LIFT | بالابر روتر (Z-Axis) |
| 48 | Q3 ROUTER_FEED | پیشروی روتر (X-Axis) |
| 40 | Q4 COOLANT | پاشنده روانکار |
| 15 | SPINDLE_SSR | موتور فرز (SSR یا رله) |
| 35 | Q6 Aux | کمکی |
| 45 | BUZZER | بوق |
| 3 | E-STOP | توقف اضطراری |
| 7 | START | شستی شروع |
| 5 | COPY_SENSOR | سنسور تماس الگو |
| 4 | WORK_SENSOR | سنسور تماس قطعه |
| 6 | HOME_REF | سنسور مرجع/هوم |

## توالی فرآیند (FSM)

```
۱. کلمپ قطعه کار + کلمپ الگو
۲. روشن شدن موتور فرز
۳. پیشروی Z تا تماس با الگو
۴. کپی مسیر الگو با پیشروی X
۵. بالابر Z → Home
۶. باز شدن کلمپ‌ها
```

## خطوط قرمز — ABSOLUTE

- ❌ هرگز موتور فرز بدون هر دو کلمپ فعال روشن شود
- ❌ E-Stop → بالا رفتن فوری روتر + قطع موتور
- ❌ COOLANT فقط در زمان فرزکاری فعال باشد

## الگوی پیاده‌سازی
از **Bakery Mixer** (`bakery-mixer/`) به عنوان Reference Architecture استفاده شود.

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/copy-router/`
- **مرجع:** `HAFFLER_CLOUD_INTEGRATION_AND_TODO.md`، `docs/`
