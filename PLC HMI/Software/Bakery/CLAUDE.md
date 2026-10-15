# Bakery HMI Development Guidelines (Claude & Anthropic Assistants)

## Key Commands
- Compile only: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run`
- Create backup: `python tools/backup_manager.py create "<desc>"`
- Commit micro-fix: `python tools/backup_manager.py commit "<message>"`
- Flash firmware: NEVER run upload without explicit user command.

## Architecture
- Target: ESP32-S3 (16MB Flash, PSRAM, SPI Display 480x320, Capacitive Touch, DS3231 RTC)
- Framework: Arduino + FreeRTOS on PlatformIO
- Core 1: LVGL, Touch polling, Display rendering
- Core 0: Bakery machine automation, motor/inverter control, sensor monitoring, NVS storage, keypad debouncing

## Critical Safety Rules
- Inverter FWD (Pin 39) & REV (Pin 48) mutual exclusivity (dead-time >= 100ms)
- Sensor timeout guards on all motor movements (max 5000ms)
- Outputs active-low (`ON = LOW`, `OFF = HIGH`)
- Zero guesswork policy: verify all pins, variables and contracts directly with tools first.
