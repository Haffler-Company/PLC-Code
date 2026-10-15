# Project Info — Bakery HMI / PLC

- **Project:** Bakery Dough Divider & Rounder / Chone HMI & PLC
- **Path:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Bakery`
- **Firmware Path:** `V.1.1.0\ChoneProject`
- **Current Version:** V.1.1.0
- **Target Microcontroller:** Espressif ESP32-S3 DevKitC-1
- **Flash Size:** 16MB (Quad SPI / DIO)
- **PSRAM:** Enabled (Quad SPI)
- **Display Driver:** 480x320 SPI TFT (ILI9341 / ST7796 compatible via TFT_eSPI)
- **Touch Driver:** Capacitive Touch (`bb_captouch` over I2C: SDA 8, SCL 9)
- **RTC:** Maxim DS3231 Industrial Real-Time Clock (I2C)
- **Thermocouple:** Dual MAX6675 SPI (K-type thermocouples)
- **Industrial Automation:** Inverter Forward/Reverse relays, Piston motor, Funnel motor, Inductive sensors
- **UI Framework:** LVGL with FreeRTOS multi-core task scheduling
- **Build Engine:** PlatformIO Core (esp32-s3-devkitc-1 environment)
- **Documentation Suite:** `docs/`
- **Engineering Tools:** `tools/`
