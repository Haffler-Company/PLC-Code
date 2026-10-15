# Haffler — Component Compatibility Matrix
## جدول سازگاری قطعات و IC‌ها

> **AGENT MANDATE:** Before suggesting hardware alternatives or substitutions,
> check this matrix. Never assume components are interchangeable.

---

## 🖥️ Display Controllers

| IC | Interface | Resolution | Used In | Compatible With | NOT Compatible |
|---|---|---|---|---|---|
| **ILI9488** | 8-bit Parallel | 480×320 | All 11 PLC machines | ILI9486 (same pinout, minor init diff) | ILI9341 (SPI only, different resolution) |
| **ILI9341** | SPI | 320×240 | Coffee Mixer | ST7789 (SPI, similar init) | ILI9488 (parallel, different resolution) |
| **ST7789** | SPI | 240×320 or 320×240 | — (potential future) | ILI9341 (minor init change) | ILI9488 |

**Key Rule:** PLC machines use **parallel** interface for speed. Coffee Mixer uses **SPI** for simplicity.
Swapping between parallel and SPI requires PCB redesign.

---

## 👆 Touch Controllers

| IC | Interface | Protocol | Used In | Interchangeable With | Notes |
|---|---|---|---|---|---|
| **GT911** | I2C | Proprietary | All 11 PLC machines + Coffee Mixer | GT928 (different I2C address, same register layout) | XPT2046 (resistive, completely different) |
| **GT928** | I2C | Proprietary | — (potential upgrade) | GT911 (change I2C address 0x5D→0x14 or 0x5D) | Same driver with address change |
| **XPT2046** | SPI | Proprietary | — (old designs) | NOT compatible with GT911 | Resistive touch, needs pressure |

**Critical Trap (ALL GT9xx):** Register `0x814E` must be cleared on every poll. Same for GT911 and GT928.
**I2C Addresses:** GT911 = `0x5D` (default) or `0x14` (alt). Set by RST/INT pin timing during power-on.

---

## 🌡️ Thermocouple ICs

| IC | Interface | Accuracy | Max Temp | Used In | Upgrade Path |
|---|---|---|---|---|---|
| **MAX6675** | SPI | ±2°C | 1024°C | ICE CREAM (T1, T2) | MAX31855 |
| **MAX31855** | SPI | ±2°C typ, ±6°C max | 1800°C | — (potential upgrade) | Backward compatible with MAX6675 in most cases |
| **MAX31865** | SPI | ±0.5°C | — | — | For PT100/PT1000 RTD only — NOT for thermocouples |

**Read Timing:** MAX6675 requires **minimum 170ms** between reads (datasheet). Faster = invalid reading.
**CS Pins (ICE CREAM):** T1 = GPIO21, T2 = GPIO0. Both share MISO=GPIO37, SCK=GPIO47.
**MAX6675 vs MAX31855 Drop-in:** MAX31855 has same SPI footprint but 4-byte response vs 2-byte. Driver update needed.

---

## ⏰ Real-Time Clock ICs

| IC | Interface | Accuracy | Battery | Used In | Alternative |
|---|---|---|---|---|---|
| **DS3231** | I2C | ±2ppm (±1 min/year) | CR2032 | All PLC machines (Day-Lock) | DS3232 (same + internal EEPROM) |
| **DS1307** | I2C | ±20ppm (poor) | CR2032 | — (rejected) | DO NOT use — too inaccurate for Day-Lock tamper resistance |
| **PCF8563** | I2C | ±4.3ppm | CR2032 | — (alternative) | Acceptable substitute for DS3231 with driver change |

**Why DS3231 only:** Day-Lock anti-tamper requires accurate monotonic timekeeping.
DS1307 drifts ±20 minutes/year → customers can exploit drift to extend lease without paying.

---

## 💾 External Flash ICs

| IC | Interface | Capacity | Used In | Compatible With |
|---|---|---|---|---|
| **W25Q32** | SPI | 32Mb (4MB) | Coffee Mixer | W25Q64 (larger, same pinout/protocol) |
| **W25Q128** | SPI | 128Mb (16MB) | — (future) | Same W25Q family |
| **GD25Q32** | SPI | 32Mb | — (alternative) | Drop-in compatible with W25Q32 |

**SPI Bus Conflict:** Coffee Mixer W25Q32 shares SPI bus with TFT. CS pins must NEVER be simultaneously active.

---

## 🔌 Main Controllers

| IC | Flash | RAM | PSRAM | Used In | Notes |
|---|---|---|---|---|---|
| **ESP32-S3 N16R8** | 16MB | 512KB | 8MB | All 11 PLC machines | PSRAM required for LVGL double buffer |
| **ESP32 (standard)** | 4MB | 520KB | None | Coffee Mixer | No PSRAM = no LVGL = use TFT_eSPI |
| **ESP32-S3 N8R8** | 8MB | 512KB | 8MB | — (acceptable alt) | 8MB flash may be tight for OTA + filesystem |
| **ESP32-C3** | — | — | None | NOT RECOMMENDED | Single-core, no dual-core FreeRTOS isolation |
| **ESP32-S2** | — | — | None | NOT RECOMMENDED | No Bluetooth, single-core |

**Critical Rule:** PLC machines require PSRAM (N16R8 or N8R8). Coffee Mixer must use standard ESP32 to match existing PCB.

---

## 💡 LED Drivers

| IC/Type | Interface | Used In | Compatible With | Notes |
|---|---|---|---|---|
| **WS2812B** | Single-wire 800kHz | Coffee Mixer (10px) | WS2812, SK6812 (RGBW) | Timing-critical — use RMT peripheral |
| **SK6812** | Single-wire 800kHz | — (alternative) | WS2812B (same protocol) | Adds White channel (RGBW) |
| **APA102** | SPI (2-wire) | — (potential upgrade) | NOT directly compatible | Clock + Data, immune to timing issues, no RMT needed |

**WiFi Interference:** WS2812B and SK6812 are both vulnerable to WiFi interrupt timing corruption.
**APA102 Advantage:** SPI-based, so hardware-clocked → immune to interrupt timing issues.

---

## 🔌 Communication Interfaces Summary

| Interface | Speed | Used For | Shared Bus Rules |
|---|---|---|---|
| SPI (PLC) | 40MHz | ILI9488 LCD | Single master, CS per device |
| SPI (PLC) | 2MHz | MAX6675 T1, T2 | CS per sensor (GPIO21, GPIO0) |
| SPI (Coffee) | 40MHz | ILI9341 + W25Q32 | NEVER assert both CS simultaneously |
| I2C (PLC) | 400kHz Fast | GT911 + DS3231 | SDA=GPIO8, SCL=GPIO9 |
| I2C (Coffee) | 400kHz Fast | GT911 | SDA=GPIO4, SCL=GPIO16 (check settings.h) |
| UART (PLC) | 115200 | Debug serial, RS485 Modbus | — |
