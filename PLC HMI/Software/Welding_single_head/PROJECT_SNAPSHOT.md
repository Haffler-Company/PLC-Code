# PROJECT SNAPSHOT — Welding Single Head (جوش تک‌سر UPVC)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## سخت‌افزار هدف
- **MCU:** ESP32-S3 DevKitC-1 — 16MB Flash، 8MB PSRAM، 240MHz dual-core
- **Display:** 480×320 ST7796/ILI9341 TFT، SPI 40MHz، rotation 3
- **Touch:** GT911 خازنی — I2C 400kHz (`bb_captouch`)
- **Thermocouple x2:** MAX6675 SPI — CS1=GPIO21، CS2=GPIO0، SCK=47، MISO=37
- **Framework:** Arduino ESP32 core + LVGL 9.4.0 + EEZ Studio/Flow Engine
- **Upload Port:** COM8/COM17/COM4 CP210x — هرگز COM3 (Intel AMT trap)

## پین‌اوت کامل (تمام Active-LOW)

| سیگنال | GPIO | عملکرد |
|---|---|---|
| SSR_1 | 15 | المنت حرارتی (Solid State Relay) |
| TABLE (Q0) | 16 | جک صفحه/میز |
| PRESS_LEFT (Q1) | 42 | جک پرس چپ |
| PRESS_RIGHT (Q2) | 39 | جک پرس راست |
| ELEMENT (Q3) | 48 | جک پیشروی المنت |
| SHABLON (Q4) | 40 | جک شابلون صفر |
| ZARBEH1 (Q6) | 35 | جک ضربه ۱ |
| ZARBEH2 (Q7) | 36 | جک ضربه ۲ |
| BUZZER | 45 | پیزو LEDC PWM 2.7-4kHz |
| I0 سنسور المنت | 6 | Analog Band (Retracted≈1550، Extended>3500) |
| I2 سنسور شابلون | 5 | Analog Band |
| I4 سنسور میز | 4 | Analog Band |
| I6 E-Stop قارچی | 3 | قطع فوری همه خروجی‌ها |
| I7 پدال پا | 7 | شروع چرخه جوش |
| F-Keys (F1..F10) | 1, 2 | Analog Resistor Ladder |

## معماری کامپوننت‌ها

| کامپوننت | مسیر | مسئولیت |
|---|---|---|
| hal_io | `components/hal_io/` | I/O، debounce، E-Stop |
| hal_display | `components/hal_display/` | LVGL flush، DMA 32-line |
| hal_thermo | `components/hal_thermo/` | MAX6675، NaN guard |
| hal_touch | `components/hal_touch/` | GT911 I2C |
| hal_rtc | `components/hal_rtc/` | DS3231 |
| plc_engine | `components/plc_engine/` | FSM جوش، Thermal PID |
| thermal_pid | `components/thermal_pid/` | کنترل دمای دقیق |
| ui_engine | `components/ui_engine/` | LVGL screens، زبان سه‌گانه |
| web_app | `components/web_app/` | WebApp GZIP، MQTT |
| ota_service | `components/ota_service/` | OTA، rollup changelog |
| nvs_storage | `components/nvs_storage/` | settings، calibration |

## خطوط قرمز ایمنی — ABSOLUTE

- ❌ **هرگز** المنت (ELEMENT) بدون هر دو پرس چپ و راست (PRESS_LEFT + PRESS_RIGHT) جلو برود
- ❌ **هرگز** SSR_1 روشن با `NaN` یا disconnect روی ترموکوپل
- ❌ **هرگز** E-Stop را دور بزن — قطع <20ms همه خروجی‌ها
- ❌ **هرگز** پولاریته پین عوض شود
- ❌ **هرگز** COM3 برای آپلود
- ✅ ولتاژ باند مرده ADC: 1850-2240 (هیچ state)

## فایل‌های اصلی

| فایل | محتوا |
|---|---|
| `src/main.cpp` | setup، FreeRTOS spawn، Core 0/1 |
| `include/tasks.h` | Thermal loop، Jack FSM، E-Stop watchdog |
| `src/screens.c` | UI screens LVGL |
| `include/agent_hmi.h` | HMAC-SHA256 serial test protocol |
| `docs/REFERENCE.md` | **کامل‌ترین مرجع سخت‌افزار** |
| `docs/START.md` | نقطه ورود تسک |
| `docs/WORKLOG.md` | قفل‌های فعال، آخرین تغییرات |

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/upvc/welding-single-head/`
- **Build:** `pio run` (PlatformIO)
- **Upload:** فقط با «فلش کن» صریح → COM8/COM17/COM4
