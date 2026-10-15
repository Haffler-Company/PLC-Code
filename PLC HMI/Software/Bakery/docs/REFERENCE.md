# Engineering Quick Reference & Constants — Bakery HMI

This document provides a fast-lookup cheat sheet for GPIO pin mappings, NVS persistence keys, FreeRTOS task configurations, and security defaults.

---

## 1. Quick GPIO Pinout Sheet

```text
+-------------------+-------+-------+-----------------------------+
| Signal Identifier | GPIO  | Dir   | Electrical Type / Level     |
+-------------------+-------+-------+-----------------------------+
| TFT_MOSI          | 10    | OUT   | SPI Master Out (80 MHz)     |
| TFT_MISO          | 37    | IN    | SPI Master In               |
| TFT_SCLK          | 12    | OUT   | SPI Clock                   |
| TFT_CS            | 13    | OUT   | Chip Select (Active LOW)    |
| TFT_DC            | 14    | OUT   | Data / Command              |
| TFT_BL            | 38    | OUT   | Backlight (HIGH = ON)       |
| TOUCH_SDA         | 8     | I/O   | I2C Data (400 kHz)          |
| TOUCH_SCL         | 9     | OUT   | I2C Clock                   |
| TOUCH_INT         | 40    | IN    | Touch Interrupt (Active LOW)|
| TOUCH_RST         | 39    | OUT   | Touch Hardware Reset        |
| THERMO_1_CS       | 4     | OUT   | MAX6675 Channel 1 CS        |
| THERMO_2_CS       | 5     | OUT   | MAX6675 Channel 2 CS        |
| PISTON_MOTOR      | 36    | OUT   | Q7: Relay (Active LOW)      |
| FUNNELE_MOTOR     | 42    | OUT   | Q1: Relay (Active LOW)      |
| INVERTER_FORWARD  | 39    | OUT   | Q2: Inverter FWD (Active LOW|
| INVERTER_REVERSE  | 48    | OUT   | Q3: Inverter REV (Active LOW|
| PISTON_SENSOR     | 7     | IN    | I7: Inductive (Active LOW)  |
| FUNNELE_SENSOR    | 3     | IN    | I6: Inductive (Active LOW)  |
| KEY_F             | 1     | IN    | ADC1_CH0 (0..3.3V)          |
| KEY_M             | 2     | IN    | ADC1_CH1 (0..3.3V)          |
| BUZZER            | 45    | OUT   | Warning Sound (HIGH = Beep) |
+-------------------+-------+-------+-----------------------------+
```

---

## 2. NVS Storage Keys (Preferences Namespace: `"chone"`)

| NVS Key Name | Type | Factory Default | Functional Description |
|---|---|---|---|
| `"magic"` | uint32 | `0xBBFF0FF0` | Storage format validation magic number |
| `"version"` | String | `"1.0.0"` | Stored firmware version string |
| `"auto_weight"`| int | `11` | Piston stroke discharge duration in ms (Weight) |
| `"auto_speed"` | int | `22` | Rest interval between portions in ms (Pace) |
| `"chone_counter"`| int | `33` | Cumulative portion count |
| `"max_piston"` | int | `44` | Maximum allowable piston return stroke in ms |
| `"max_funnle"` | int | `55` | Maximum allowable funnel forward feed in ms |
| `"ACSound"` | bool | `true` | Acoustic feedback beep on portion completion |
| `"firmware_partion_select"`| int | `0` | Active OTA boot slot (0 = OTA0, 1 = OTA1) |

---

## 3. FreeRTOS Task Matrix

| Task Name | Function Entry | Priority | Core Affinity | Stack Depth (4-byte Words) | Role & Scope |
|---|---|---|---|---|---|
| `"LVGL Tick"` | `lv_tick_task` | 3 (High) | Core 1 | 2048 words | Increments 1ms LVGL system tick |
| `"LVGL"` | `lvgl_task` | 2 | Core 1 | 4096 words | Executes periodic `lv_timer_handler()` |
| `"App"` | `app_task` | 1 | Core 0 | 4096 words | Executes dough portioning FSM & relays |
| `"Keypad"` | `keypad_task` | 1 | Core 0 | 2048 words | Samples & debounces ADC analog keypads |
| `"Buzzer"` | `buzzer_task` | 1 | Core 0 | 1024 words | Asynchronously drives acoustic alerts |

---

## 4. Default Passwords & Security Limits

- **Settings Menu Access Password:** `123456`
- **Portion Counter Reset Password:** `654321`
- **Sensor Watchdog Timeout:** 5000 ms (`SENSOR_MOTION_TIMEOUT_MS`)
- **Inverter Direction Dead-Time:** 100 ms (`INVERTER_DEADTIME_MS`)

---

## 5. Build & Compilation Verification

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```
