# PROJECT SNAPSHOT — Bakery Mixer (میکسر مارپیچی نانوایی)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان
## ⚡ این جدیدترین معماری هافلر است — Reference Implementation

---

## سخت‌افزار هدف
- **MCU:** ESP32-S3-WROOM-1 (N16R8) — 16MB Flash، 8MB PSRAM، HAFFLER ETV3A
- **Display:** 480×320 IPS TFT — SPI2/FSPI تا 80MHz
- **Touch:** GT911 — I2C0 (SDA=8, SCL=9)، addr=0x5D
- **RTC:** DS3231 — I2C0 مشترک، addr=0x68
- **Thermocouple:** MAX6675 — CS=GPIO21، SCK=GPIO47، MISO=GPIO37
- **RS485:** UART2 (TX=43، RX=44، DE/RE=46)
- **Framework:** ESP-IDF + FreeRTOS + LVGL (جدیدترین pattern هافلر)

## پین‌اوت کامل — HAFFLER ETV3A Standard

| GPIO | تعریف | عملکرد میکسر |
|---|---|---|
| 1 | PIN_KEY_F | کلیدهای F1..F5 (ADC Ladder) |
| 2 | PIN_KEY_M | کلیدهای M (SAVE/CANCEL/MODE/START/STOP) |
| 3 | PIN_EMERGENCY | E-Stop قارچی NC |
| 4 | PIN_SENSOR_PAIR_4_5 | Motor Overload Relay (NC) |
| 5 | PIN_SENSOR_PAIR_2_3 | Safety Bowl Guard Interlock (NC) |
| 6 | PIN_SENSOR_PAIR_0_1 | Water Flow Meter Pulse (Opto) |
| 7 | PIN_START_PEDAL | شستی استارت فیزیکی |
| 8 | PIN_I2C_SDA | I2C Data |
| 9 | PIN_I2C_SCL | I2C Clock 400kHz |
| 10 | PIN_LCD_MOSI | Display SPI |
| 11 | PIN_LCD_MISO | Display SPI |
| 12 | PIN_LCD_SCK | Display SPI Clock |
| 13 | PIN_LCD_CS | Display CS Active-LOW |
| 14 | PIN_LCD_DC | Display Command/Data |
| 16 | PIN_MIXER_MOTOR (Q0) | **کنتاکتور موتور میکسر** Active-LOW |
| 21 | PIN_WATER_TEMP_CS | MAX6675 دمای آب CS |
| 35 | Q6 Knife/Aux | کمکی |
| 36 | Q7 Knife/Aux | کمکی |
| 37 | PIN_THERMO_MISO | MAX6675 MISO |
| 38 | PIN_LIGHT_GREEN (Q8) | چراغ Stack سبز (Active Mixing) |
| 39 | PIN_BOWL_BRAKE (Q2) | ترمز کاسه اصطکاکی Active-LOW |
| 40 | PIN_WATER_VALVE (Q4) | شیر دوزینگ آب Active-LOW |
| 41 | PIN_LIGHT_RED (Q9) | چراغ Stack قرمز (Fault/Temp) |
| 42 | PIN_FLOUR_FEEDER (Q1) | اوگر آرد Active-LOW |
| 43 | PIN_RS485_TX | RS485 TX |
| 44 | PIN_RS485_RX | RS485 RX |
| 45 | PIN_BUZZER | پیزو PWM LEDC |
| 46 | PIN_RS485_DE | RS485 Direction |
| 47 | PIN_THERMO_SCK | MAX6675 Clock |
| 15 | PIN_WATER_TEMP_CTRL (Q5) | SSR کنترل دمای آب |

## معماری جدید (Reference Pattern)

| فایل | مسئولیت |
|---|---|
| `include/haffler_pins.h` | **Single Source of Truth** همه پین‌ها |
| `include/haffler_packml_fsm.h` | FSM PackML صنعتی |
| `include/haffler_actuator.h` | لایه abstraction خروجی‌ها |
| `include/haffler_sequencer.h` | Sequencer تسک‌های موازی |
| `include/haffler_ui_builder.h` | UI builder LVGL |
| `include/haffler_ui_lang.h` | سیستم سه‌زبانه |
| `include/haffler_ui_theme.h` | پالت Apple HIG |
| `include/haffler_day_lock.h` | قفل اقساطی |
| `include/haffler_recipes.h` | دستورالعمل‌های میکس |
| `include/haffler_web_portal.h` | WebApp GZIP |
| `include/fw_changelog.h` | Rollup changelog |

## خطوط قرمز

- ❌ هرگز موتور میکسر بدون Guard Interlock (GPIO5) روشن شود
- ❌ هرگز دمای آب از SETPOINT+5°C بالاتر رود بدون قطع SSR
- ❌ Flow Meter Pulse بدون تایم‌اوت → قطع شیر آب

## وضعیت
- **Architecture:** جدیدترین — برای سایر پروژه‌ها الگو گیری شود
- **OTA Path:** ArvanCloud `ota-plc/bakery/spiral-mixer/`
- **Build:** `idf.py build` یا `pio run`
