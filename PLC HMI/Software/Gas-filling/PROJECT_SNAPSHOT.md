# PROJECT SNAPSHOT — Gas Filling (دستگاه پرکن آرگون)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## سخت‌افزار هدف
- **MCU:** ESP32-S3 — PSRAM، 240MHz
- **Display:** 480×320 ST7796 SPI TFT، 40MHz، `tft.setSwapBytes(true)` — هرگز ILI9341 یا 80MHz
- **Touch:** GT911 خازنی — I2C، تبدیل مختصات: `479 - raw_y`, `raw_x`
- **Framework:** Arduino/PlatformIO + LVGL 9.5.0 + EEZ

## پین‌اوت کلیدی

| GPIO | سیگنال | عملکرد |
|---|---|---|
| 16 | VALVE_1 | **خروجی Active-LOW** — شیر آرگون ۱ |
| 42 | VALVE_2 | **خروجی Active-LOW** — شیر آرگون ۲ |
| 45 | BUZZER | بوق اخطار |
| 3 | E-Stop (Opto) | قطع فوری هر دو شیر |
| 1 | KEY_F | F1..F5 پنل جلو (Analog ADC_11db) — هرگز INPUT_PULLUP |
| 2 | KEY_M | M-Keys: MODE/START/STOP/SAVE/CANCEL (Analog ADC_11db) |

## خطوط قرمز ایمنی — ABSOLUTE

- ❌ **Watchdog 180s** — قطع خودکار هر دو شیر، هرگز bypass نشود (جلوگیری از تخلیه سیلندر آرگون)
- ❌ **هرگز** هر دو شیر همزمان فعال نشوند (pressure mismatch)
- ❌ **E-Stop** → قطع فوری VALVE_1 + VALVE_2 + BUZZER
- ❌ **هرگز** پولاریته `vr_OutPutType` (N.O/N.C) بدون دستور عوض شود
- ❌ **هرگز** LVGL/UI از hardware timer ISR فراخوانی شود — فقط `volatile flag` + `loop()`
- ✅ کلیدهای Analog: `analogSetPinAttenuation(pin, ADC_11db)` — **هرگز** INPUT_PULLUP

## معماری فریمور

| فایل | مسئولیت |
|---|---|
| `GasFilling_HMI/actions.c` | منطق اصلی، UI callbacks، ISR flags |
| `GasFilling_HMI/ota_manager.cpp` | OTA، rollup changelog |
| `GasFilling_HMI/day_lock.cpp` | قفل اقساطی |
| `GasFilling_HMI/a2_eeprom.cpp` | ذخیره تنظیمات |
| `GasFilling_HMI/fw_changelog.h` | changelog (حداکثر ۱۳ کلمه/آیتم) |
| `docs/REFERENCE.md` | مرجع سخت‌افزار |
| `docs/WORKLOG.md` | قفل‌های فعال |

## قوانین خاص این پروژه

- **Version Bump:** هر ویرایش → bump patch version در `VERSION`, `version.h`, `fw_changelog.h`, `WORKLOG.md`
- **Changelog:** حداکثر ۱۳ کلمه در هر item
- **Touch:** هرگز GT911 I2C init یا coordinate transform عوض نشود بدون proof سخت‌افزاری
- **SPI:** ST7796 در 40MHz — هرگز 80MHz یا ILI9341

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/gas-filling/`
- **Build:** `pio run`
