# Project Info — Bakery Spiral Mixer HMI / PLC

- **Project:** Commercial Bakery Spiral Dough Mixer (میکسر خمیر و ترکیب آرد و آب نانوایی)
- **Path:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\bakery-mixer`
- **Current Version:** V.1.0.0
- **Target Microcontroller:** Espressif ESP32-S3 DevKitC-1 (WROOM-1 N16R8)
- **Flash Size:** 16MB (Octal / DIO mode)
- **PSRAM:** 8MB Octal SPI Enabled
- **Display Driver:** 480x320 SPI IPS TFT (ST7796 / ILI9341 via TFT_eSPI)
- **Touch Driver:** Goodix GT911 Capacitive Touch (`bb_captouch` over I2C: SDA 8, SCL 9)
- **RTC:** Maxim DS3231 Industrial Real-Time Clock with TCXO (I2C)
- **Thermodynamic Sensing:** Maxim MAX6675 SPI K-Type Thermocouple (Water Tank Temperature Probe)
- **Motor Control:** Single-speed Mixer Motor Contactor via Q0 (Active-LOW, 0=ON)
- **Water Dosing:** Dual-Mode: Pulse Flowmeter (GPIO 6) or Seconds Timer (Q4 Solenoid Valve)
- **Safety Interlocks:** Emergency Stop (GPIO 3), Safety Bowl Guard (GPIO 5), Motor Overload (GPIO 4)
- **UI Framework:** LVGL 9 with FreeRTOS Multi-Core Task Scheduling (Core 0: UI, Core 1: Deterministic 50ms PLC)
- **Security & Finance:** Haffler 5-Stage Cascading Day-Lock with Jalali RTC Calendar & 4-digit Challenge-Response
- **Connectivity:** Embedded WebPortal (GZIP compressed, DRAM protection > 40KB, Captive Portal 204 handler)
- **Build Engine:** PlatformIO Core (esp32-s3-devkitc-1 environment)
