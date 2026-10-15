# PROJECT SNAPSHOT — Welding Double Head (جوش دوسر UPVC)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## وضعیت پروژه
> ⚠️ **در انتظار کد** — این پروژه هنوز فریمور تولیدی ندارد. معماری سخت‌افزار مشابه Welding Single Head است.

## سخت‌افزار هدف
- **MCU:** ESP32-S3 (N16R8) — 16MB Flash، 8MB PSRAM
- **Display:** ILI9488 3.5"، تاچ خازنی GT911
- **Thermocouple x2:** MAX6675 — یکی برای هر المنت (چپ/راست)
- **Framework:** ESP-IDF/Arduino + LVGL

## پین‌اوت (مشابه HAFFLER ETV3A Standard)

| GPIO | سیگنال | عملکرد |
|---|---|---|
| 16 | Q0 (TABLE) | جک صفحه/میز |
| 42 | Q1 (PRESS_LEFT) | کلمپ چپ |
| 39 | Q2 (PRESS_RIGHT) | کلمپ راست |
| 48 | Q3 (ELEMENT) | جک پیشروی صفحه داغ |
| 40 | Q4 (SHABLON) | جک شابلون صفر |
| 15 | SSR_1 | المنت گرمایشی ۱ (چپ) |
| 35 | SSR_2 | المنت گرمایشی ۲ (راست) |
| 45 | BUZZER | بوق |
| 3 | E-Stop | توقف اضطراری |
| 7 | PEDAL | پدال پا |
| 21 | THERMO_CS1 | MAX6675 المنت چپ |
| 0 | THERMO_CS2 | MAX6675 المنت راست |

## توالی جوشکاری دوسر (Safety-Critical FSM)

```
۱. کلمپ هر دو هد (PRESS_LEFT + PRESS_RIGHT)
۲. چک دمای هر دو المنت (±5°C tolerance window)
۳. ورود صفحه داغ (ELEMENT forward)
۴. Melting Time → پرس ملایم
۵. خروج صفحه داغ ظرف <2s
۶. Welding Pressure → زمان انجماد
۷. باز شدن کلمپ‌ها
```

## خطوط قرمز — ABSOLUTE

- ❌ هرگز چرخه جوش بدون هر دو المنت در دمای مجاز (240°C-260°C) شروع شود
- ❌ E-Stop → خروج فوری صفحه داغ + قطع برق المنت‌ها
- ❌ هرگز هر دو SSR بدون interlock مستقل روشن شوند
- ✅ تمام رله‌ها Active-LOW، مقداردهی HIGH در بوت

## الگوی پیاده‌سازی
برای شروع کد از **Welding Single Head** (`Welding_single_head/`) الگوبرداری شود — معماری کامپوننتی آن Reference است. تفاوت اصلی: دو کانال دما + دو هد متحرک.

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/upvc/welding-double-head/`
- **Build:** `pio run` (پس از اضافه شدن کد)
- **مرجع:** `docs/` و `HAFFLER_CLOUD_INTEGRATION_AND_TODO.md`
