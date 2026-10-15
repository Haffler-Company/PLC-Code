# 07. Diagnostics, I/O Mapping, and Headless Testing

## Executive Summary
Rapid fault isolation on factory floors requires comprehensive, live, on-screen diagnostics and a deterministic headless serial interface. In the Haffler PLC architecture, the **Diagnostics Subsystem** allows technicians to monitor every optocoupled input, manually toggle pneumatic relays (under safety interlocks), verify front-panel key voltages, inspect free memory, and capture raw screen buffers over USB without specialized test equipment.

---

## 1. On-Screen Real-Time Diagnostics Interface

The diagnostics screen (`ui_screen_diag`) is accessible via **F5 (Settings)** $\rightarrow$ **Diagnostics**, or by entering the technician PIN (`1234`).

### Visual Layout & Ergonomics (Apple HIG Industrial Dark)
```
+-------------------------------------------------------------+
|  [< Back]     HARDWARE DIAGNOSTICS & I/O MAP     [CPU: 12%] |
+-------------------------------------------------------------+
| DIGITAL INPUTS (Opto-Isolated)                              |
|  [I0]  [I1]  [I2]  [I3]  [I4]  [I5]  [I6]  [I7]             |
|   ( )   (*)   ( )   ( )   (*)   ( )   ( )   ( )             |
|                                                             |
| RELAY OUTPUTS (Manual Toggle - Safe Interlock Active)       |
|  [Q0: Clmp] [Q1: Saw] [Q2: Lub] [Q3: Air] [Q4: Mtr]         |
|   [ OFF ]    [ OFF ]   [ OFF ]   [ ON  ]   [ OFF ]          |
|                                                             |
| FRONT KEYS & ADC VOLTAGES                                   |
|  KEY_F (GPIO 1): 2450 mV [NONE]   KEY_M (GPIO 2): 1820 mV [SET]  |
|                                                             |
| SYSTEM VITALS                                               |
|  DRAM: 62.4 KB Free  |  PSRAM: 7.2 MB Free  |  RTC: 24.5 °C |
+-------------------------------------------------------------+
```

![Hardware Diagnostics & I/O Mapping UI](./assets/images/io_diagnostics_screen.png)

### Signal Lamp Specifications
- **Input Lamps:** 14px circular indicator widgets (`lv_obj_set_style_radius(..., LV_RADIUS_CIRCLE)`).
  - **Active State (LOW / Sinking):** Emerald Neon `#22C55E` with subtle 2px border.
  - **Inactive State (HIGH / Open):** Slate Dark `#1E293B` with border `#334155`.
- **Relay Manual Buttons:** Require explicit touch and key-selection confirmation.
- **Safety Interlock:** If the machine state machine is in `STATE_RUNNING`, `STATE_HOMING`, or `STATE_CUTTING`, manual relay toggling is **strictly disabled** with an amber lock badge.

---

## 2. The ESP32 Output Read Bug & Software Status Mask

### The Hardware Quirk
On the ESP32-S3 microcontroller, calling `gpio_get_level(GPIO_NUM_x)` on a pin configured strictly as `GPIO_MODE_OUTPUT` returns undefined or zero readings on certain silicon revisions because the input buffer is disconnected.

### The Standard Solution
1. Configure all relay pins with bidirectional input-output mode and internal pull-up:
   ```cpp
   gpio_config_t io_conf = {};
   io_conf.intr_type = GPIO_INTR_DISABLE;
   io_conf.mode = GPIO_MODE_INPUT_OUTPUT; // Bidirectional enables readback
   io_conf.pin_bit_mask = RELAY_PIN_MASK;
   io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
   io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
   gpio_config(&io_conf);
   ```
2. **Mandatory Software State Mirror (`s_active_relay_mask`):**
   Firmware must maintain an authoritative 16-bit state mirror. Never rely solely on hardware register readback:
   ```cpp
   static uint16_t s_active_relay_mask = 0x0000; // Bit 0 = Q0, Bit 1 = Q1, ...

   void plc_relay_set(uint8_t relay_idx, bool state_on) {
       if (relay_idx >= 10) return;
       
       if (state_on) {
           s_active_relay_mask |= (1 << relay_idx);
           gpio_set_level(RELAY_PINS[relay_idx], 0); // Active-LOW: 0 = Energized
       } else {
           s_active_relay_mask &= ~(1 << relay_idx);
           gpio_set_level(RELAY_PINS[relay_idx], 1); // Active-LOW: 1 = De-energized
       }
   }

   bool plc_relay_get(uint8_t relay_idx) {
       if (relay_idx >= 10) return false;
       return (s_active_relay_mask & (1 << relay_idx)) != 0;
   }
   ```

---

## 3. Headless Serial Diagnostic Protocol (UART0)

For automated factory testing, end-of-line verification, and remote debugging over USB without screen interaction, UART0 operates at **921600 baud, 8N1**.

### Protocol Syntax (ASCII line-terminated `\n`)
Every incoming command returns a JSON-formatted response for deterministic script parsing:

| Command | Arguments | Example Response | Description |
| :--- | :--- | :--- | :--- |
| `PING` | None | `{"status":"OK","pong":true}` | Heartbeat check |
| `STATUS` | None | `{"state":"IDLE","dram_free":63200,"psram_free":7549747,"uptime_s":452}` | Machine and memory health |
| `IO_READ` | None | `{"inputs":18,"relays":1,"key_f_mv":3300,"key_m_mv":3300}` | Full snapshot of all inputs and outputs |
| `RELAY_SET` | `<idx> <0\|1>` | `{"relay":2,"state":1,"status":"OK"}` | Toggles relay (if machine is idle) |
| `BUZZER_TEST`| `<freq_hz> <ms>`| `{"buzzer":"TEST_DONE"}` | Fires PWM tone at resonant frequency |
| `MEM_REPORT`| None | `{"min_dram":48200,"cur_dram":62400,"dma_free":31000}` | DRAM fragmentation check |
| `SHOT` | None | `[RAW RGB565 / HEX DUMP]` | Dumps current 480x320 framebuffer for screenshot capture |

### Framebuffer Screen Dump (`SHOT`)
The command `SHOT` dumps the active LVGL screen buffer row by row or in chunks over the 921,600 baud link. The companion Python script `tools/capture_screen.py` reads this stream, converts RGB565 to PNG, and saves the screenshot directly on the PC, enabling 100% accurate visual regression tests without hardware capture cards.

---

## 4. Encoder and High-Frequency Pulse Diagnostics

On machines equipped with optical linear scales, magnetic tapes, or rotary incremental encoders (e.g. Cutters and Metr systems):

```cpp
#include "driver/pulse_cnt.h"

// Real-time pulse counter diagnostics
struct EncoderDiag {
    int32_t raw_pulses;
    float   calibrated_mm;
    float   frequency_khz;
    bool    a_channel_state;
    bool    b_channel_state;
};

void diag_read_encoder(EncoderDiag* diag) {
    int pulse_count = 0;
    pcnt_unit_get_count(pcnt_unit, &pulse_count);
    diag->raw_pulses = pulse_count;
    diag->calibrated_mm = (float)pulse_count * MM_PER_PULSE;
    diag->a_channel_state = gpio_get_level(PIN_ENCODER_A);
    diag->b_channel_state = gpio_get_level(PIN_ENCODER_B);
}
```

Technicians can inspect:
- Directional decoding (Phase A leading vs. Phase B leading).
- Frequency jitter caused by mechanical vibration.
- Missing pulse detection during fast carriage jog.

---

## 5. Pre-Flight Diagnostic Acceptance Matrix

Before shipping any machine from the assembly line, the operator must execute the following physical checklist in Diagnostics mode:

1. **Input Loopback Test:** Ground each terminal $I_0$ through $I_7$ with a jumper wire; verify that the on-screen neon lamp illuminates instantly with zero flicker.
2. **Keypad Ladder Sweep:** Press every tactile key (`F1`..`F5`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `SET`, `START`, `STOP`); verify that the displayed millivolt value matches the calibrated reference within $\pm 100\text{ mV}$.
3. **Relay Continuity Test:** Touch each relay button; listen for distinct mechanical relay click and verify multimeter continuity across terminals $Q_0$ through $Q_9$.
4. **Buzzer Tone Test:** Trigger `BUZZER_TEST 2700 200`; verify sharp $2.7\text{ kHz}$ resonant chime at $>85\text{ dB}$ SPL at $10\text{ cm}$.
5. **DRAM Memory Margin:** Verify free internal DRAM $> 45\text{ KB}$ while screen diagnostics are fully rendered.

---

## 6. Runtime Dynamic Pin Remapping Subsystem

In factory deployments, an inductive voltage spike or relay coil degradation may occasionally damage a single physical relay terminal (e.g. `Q1`). In traditional PLC systems, this requires replacing the entire board.

### 6.1. On-Screen Field Rebinding Dialog
On the Haffler controller platform, physical output pin mappings are decoupled from compiled constants and managed as **runtime variables persisted in NVS Preferences**:

```
+-------------------------------------------------------------+
|  [< Back]        PIN REASSIGNMENT MANAGER        [PIN: 1234]|
+-------------------------------------------------------------+
| SELECT LOGICAL ACTUATOR:                                    |
|   (*) Table (Q0: GPIO 16)      ( ) Press Left (Q1: GPIO 42) |
|   ( ) Press Right (Q2: GPIO 39)( ) Heater Element (Q3: 48)  |
|                                                             |
| REBIND TO PHYSICAL TERMINAL:                                |
|   [ Q0 ]  [ Q1 ]  [ Q2 ]  [ Q3 ]  [ Q4 ]  [ Q5 ]  [ Q6 ]    |
|   [ Q7 ]  [ Q8 - SPARE ] [ Q9 - SPARE ] [ NC - UNASSIGNED ]|
|                                                             |
| [ SAVE TO NVS ]                           [ FACTORY DEFAULTS]|
+-------------------------------------------------------------+
```

### 6.2. Field Recovery Protocol
1. Technician logs in with Technician PIN (`1234`).
2. Selects the malfunctioning logical actuator (e.g. `Press Left`).
3. Rebinds it to unused spare terminal `Q8 (GPIO 38)` or `Q9 (GPIO 41)`.
4. Saves changes to NVS via `storage.savePinMapping("PRESS_LEFT", 38)`.
5. The machine immediately swaps the active driver pin without recompilation or factory reflashing, restoring full production within 2 minutes.
