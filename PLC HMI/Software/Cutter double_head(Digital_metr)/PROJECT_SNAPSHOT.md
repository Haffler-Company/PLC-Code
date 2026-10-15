# PROJECT SNAPSHOT — Cutter Double Head (اره دوسر دیجیتال)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## وضعیت پروژه
> ⚠️ **در انتظار کد** — معماری سخت‌افزار مشابه Cutter Single Head است با افزودن هد دوم.

## سخت‌افزار هدف
- **MCU:** ESP32-S3 (N16R8) — 16MB Flash، PSRAM
- **Display:** 480×320 ILI9341، SPI، TFT_eSPI
- **Touch:** GT911 — I2C (SDA=8, SCL=9)
- **Framework:** Arduino/PlatformIO + LVGL 9.4

## پین‌اوت (HAFFLER ETV3A Standard)

| GPIO | سیگنال | عملکرد |
|---|---|---|
| 16 | PUL_1 | استپر هد ۱ (Pulse) |
| 42 | DIR_1 | استپر هد ۱ (Direction) |
| 39 | EN_1 | استپر هد ۱ (Enable) |
| 35 | PUL_2 | استپر هد ۲ |
| 36 | DIR_2 | استپر هد ۲ |
| 38 | EN_2 | استپر هد ۲ |
| 48 | CLAMP_1 | گیره هد ۱ |
| 40 | CLAMP_2 | گیره هد ۲ |
| 15 | SAW | موتور اره |
| 6 | REF_ADC | سنسور مرجع/هوم هد ۱ |
| 5 | GUARD_ADC | گارد + E-Stop |
| 3 | START_1 | شستی ۱ |
| 7 | START_2 | شستی ۲ |
| 21 | JOY_T1 | جوی‌استیک ۱ |
| 37 | JOY_T2 | جوی‌استیک ۲ |
| 1 | KEY_F | F-Keys Analog |
| 2 | KEY_M | M-Keys Analog |

## خطوط قرمز — ABSOLUTE

- ❌ هرگز هر دو هد هم‌زمان حرکت بدون هماهنگی موقعیت
- ❌ E-Stop → توقف فوری هر دو استپر + خاموشی اره
- ❌ backlash compensation فقط LEFT-only روی هر هد جداگانه
- ❌ `Task4.h` (USB keyboard) لمس نشود

## الگوی پیاده‌سازی
از **Cutter Single Head** (`Cutter_single_head(Digital_metr)/`) الگوبرداری شود. تفاوت: دو استپر مستقل با sync رویداد.

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/upvc/cutter-double-head/`
- **مرجع:** `docs/` و `HAFFLER_CLOUD_INTEGRATION_AND_TODO.md`
