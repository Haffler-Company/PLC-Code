# PROJECT SNAPSHOT — Bakery Belt (تقسیم‌کننده خمیر نانوایی)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## سخت‌افزار هدف
- **MCU:** ESP32-S3 DevKitC-1 — 16MB Flash، PSRAM، 240MHz
- **Display:** 480×320 TFT ILI9341/ST7796، SPI، rotation 3
- **Touch:** GT911 خازنی — I2C (SDA=8, SCL=9)
- **Thermocouple x2:** MAX6675 — CS1=GPIO4، CS2=GPIO5، SCK=12، MISO=37
- **RTC:** DS3231 — I2C مشترک با Touch
- **Framework:** Arduino/PlatformIO + LVGL + EEZ

## پین‌اوت کامل

| GPIO | سیگنال | جهت | عملکرد |
|---|---|---|---|
| 1 | KEY_F | Input ADC | کلیدهای F1..F5 پنل جلو (Analog Ladder) |
| 2 | KEY_M | Input ADC | کلیدهای M1..M5 پنل میانی |
| 3 | FUNNELE_SENSOR | Input Active-LOW | سنسور بازگشت قیف/هوپر (I6) |
| 4 | THERMO_1_CS | Output | MAX6675 ترموکوپل ۱ |
| 5 | THERMO_2_CS | Output | MAX6675 ترموکوپل ۲ |
| 7 | PISTON_SENSOR | Input Active-LOW | سنسور بالای پیستون (I7) |
| 8 | TOUCH_SDA | Bidirectional | I2C Data (Touch + RTC) |
| 9 | TOUCH_SCL | Output | I2C Clock 400kHz |
| 10 | TFT_MOSI | Output | Display SPI |
| 12 | TFT_SCLK | Output | Display + Thermo SPI Clock |
| 13 | TFT_CS | Output Active-LOW | Display Chip Select |
| 14 | TFT_DC | Output | Command/Data Select |
| 36 | PISTON_MOTOR (Q7) | Output Active-LOW | موتور پیستون |
| 37 | TFT_MISO | Input | SPI MISO |
| 38 | TFT_BL | Output Active-HIGH | بک‌لایت LCD |
| 39 | INVERTER_FORWARD (Q2) | Output Active-LOW | اینورتر جلو — هرگز همزمان با REV |
| 42 | FUNNELE_MOTOR (Q1) | Output Active-LOW | موتور قیف/هوپر |
| 45 | BUZZER | Output Active-HIGH | بوق اخطار |
| 48 | INVERTER_REVERSE (Q3) | Output Active-LOW | اینورتر معکوس — هرگز همزمان با FWD |

## خطوط قرمز ایمنی — ABSOLUTE

- ❌ **هرگز** INVERTER_FORWARD و INVERTER_REVERSE هم‌زمان روشن (SHORT CIRCUIT)
- ❌ مینیمم dead-time 100ms بین تغییر جهت اینورتر
- ❌ سنسور موشن timeout 5000ms — بعد از timeout قطع همه موتورها
- ❌ E-Stop (F5/M5 ADC≈1100-1400) → قطع فوری همه خروجی‌ها
- ✅ پولاریته رله: Active-LOW (`ON = LOW`, `OFF = HIGH`)

## ADC Ladder Thresholds (F-Keys & M-Keys)

| کلید | ADC Range | عملکرد |
|---|---|---|
| F1/M1 | 4000-5000 | Command 1 |
| F2/M2 | 3300-3600 | Command 2 |
| F3/M3 | 2700-2900 | Command 3 |
| F4/M4 | 1900-2100 | **START Cycle** |
| F5/M5 | 1100-1400 | **E-STOP** |
| Idle | <1000 | بدون کلید |

## فایل‌های اصلی

| فایل | محتوا |
|---|---|
| `V.1.1.0/ChoneProject/bakery_cycle_fsm.h` | FSM اصلی تقسیم خمیر |
| `V.1.1.0/ChoneProject/day_lock.cpp` | سیستم قفل اقساطی |
| `V.1.1.0/ChoneProject/lang.c` | سیستم سه‌زبانه |
| `V.1.1.0/ChoneProject/defines.h` | ثابت‌ها و پین‌ها |
| `V.1.1.0/ChoneProject/general.h` | توابع مشترک |
| `docs/HARDWARE_IO_PINOUT.md` | **مرجع کامل پین‌ها** |
| `docs/BAKERY_HMI_KNOWLEDGE.md` | دانش‌نامه کامل |

## وضعیت
- **OTA Path:** ArvanCloud `ota-plc/bakery/belt-divider/`
- **Build:** `pio run`
