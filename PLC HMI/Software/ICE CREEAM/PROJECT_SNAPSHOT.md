# PROJECT SNAPSHOT — ICE CREEAM (بستنی‌ساز هافلر)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان، فایل‌های دیگر را اسکن نکن

---

## سخت‌افزار هدف
- **MCU:** ESP32-S3 (N16R8) — 16MB Flash، 8MB PSRAM
- **Display:** ILI9488 3.5" موازی، DMA buffer 30KB، SPI2
- **Touch:** GT911 خازنی — I2C0 Fast-Mode 400kHz (SDA=8, SCL=9)
- **RTC:** DS3231 — همان I2C0
- **Thermocouple x2:** MAX6675 SPI — T1(CS=21), T2(CS=0), MISO=37, SCK=47
- **Framework:** ESP-IDF + FreeRTOS dual-core + LVGL

## پین‌اوت کامل

| سیگنال | GPIO | منطق | عملکرد |
|---|---|---|---|
| Q0 کمپرسور کوچک مخزن | 16 | Active-LOW | خنک‌کننده بن‌ماری |
| Q1 کمپرسور بزرگ سیلندر | 42 | Active-LOW | برودت اصلی، delay 180s |
| Q2 موتور پارو | 39 | Active-LOW | همزن سیلندر |
| Q3 پمپ کرم لبه رنگی | 48 | Active-LOW | تزریق کرم/شربت |
| Q4 پمپ رینبو ژله | 40 | Active-LOW | ژله رنگی |
| Q5 شیر باد لبه رنگی | 38 | Active-LOW SSR | هوای فشرده |
| رله ریست | 18 | Active-LOW | پالس 500ms |
| بازر | 45 | Output | پیزوالکتریک 24V |
| I0 سنسور القایی پولی | 6 | Digital Pull-Up | پالس دور — استال 4s |
| I2 میکروسوئیچ کلاچ عقب | 5 | Active-LOW | سختی بستنی |
| I4 میکروسوئیچ اهرم جلو | 4 | Active-LOW | شمارنده قیف |
| I6 کلید استپ قارچی | 3 | Active-LOW | E-Stop <20ms |
| I7 کلید استارت | 7 | Active-LOW | شروع چرخه |
| LOAD_M کنترل‌بار موتور | 15 | Active-LOW | تریپ جریان موتور |
| LOAD_C کنترل‌بار کمپرسور | 17 | Active-LOW | تریپ کمپرسور |
| Display MOSI/SCLK/CS/DC | 10/12/13/14 | SPI2 | ILI9488 |

## فایل‌های اصلی — مستقیم برو اینجا

| فایل | محتوا |
|---|---|
| `main/project_config.h` | **تک‌منبع حقیقت:** پین‌ها، ثابت‌های زمانی |
| `main/actions.c` | منطق PLC، FSM اصلی، تسک Core 0 |
| `main/ui/screens.c` | صفحات LVGL، UI renderer |
| `main/ui/styles.c` | پالت رنگی Apple HIG |
| `main/ui/ui_lang.c` | سیستم سه‌زبانه، دیکشنری STR_ |
| `main/day_lock_core.c` | سیستم قفل روزشمار اقساطی |
| `main/ota_update.c` | کلاینت OTA، rollup changelog |
| `main/panel_nav.h` | ناوبری پنل، Toast بنرها |
| `tools/haffler_day_lock_keygen.py` | تولید کد بازگشایی |

## کدهای خطا ISA-18.2

| کد | عنوان | رفتار |
|---|---|---|
| E00 | Emergency Stop | قطع <20ms همه رله‌ها |
| E01 | Inductive Stall | قطع موتور پارو |
| E02 | Motor Overload | قطع پارو + کمپرسور |
| E03 | Compressor Overload | قطع کمپرسور بزرگ |
| E04 | T1 Disconnect | قطع کمپرسور کوچک |
| E05 | T2 Disconnect | سوئیچ به کنترل زمانی |
| E07 | Lease Expired | قفل نرم اقساطی |
| E08 | Stuck Lever | هشدار صوتی |
| E12 | Brownout | بازیابی آنی |

## خطوط قرمز ایمنی

- ❌ هرگز تاخیر 180s کمپرسور بزرگ دور نزده شود
- ❌ هرگز دمای سیلندر زیر -22°C بدون قطع کمپرسور
- ❌ هرگز بدون `lvgl_lock()` از Core 0 به LVGL دست بزن
- ❌ هرگز `malloc` در حلقه periodic
- ✅ همه خروجی‌ها در بوت باید HIGH (OFF) باشند

## وضعیت فعلی فریمور
- **Build:** `build/ili9488_lvgl_btn.bin` (3.03 MB — 43.3% از 7MB)
- **حالت:** تولید — آپلود فقط با «فلش کن» صریح
- **OTA Path:** ArvanCloud `ota-plc/other/ice-cream/`

## دستور بیلد
```powershell
idf.py build
python tools/verify_firmware.py  # 31 symbol check
```
