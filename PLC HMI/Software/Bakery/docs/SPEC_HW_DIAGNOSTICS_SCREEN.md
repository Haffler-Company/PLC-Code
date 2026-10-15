# Implementation Blueprint: Technician Hardware I/O Diagnostics Screen

**Document ID:** SPEC-004  
**Target Subsystem:** User Interface & Field Diagnostic Tool  
**Benchmarked Standard:** `Welding_single_head/docs/ACCEPTANCE_TEST_SPEC.md` & `screen_hw_diag`  
**Security:** Accessible via Technical PIN `123456` or Boot holding F5 key

---

## 1. Executive Summary & Objective

In field service visits, technicians need a direct, intuitive way to verify physical wiring, relay click contacts, VFD response, and sensor alignment without dismantling the machine or using a digital multimeter.

This specification provides the blueprint for an interactive **Hardware I/O Diagnostics Screen (`ui_HardwareDiagnostics`)** that allows:
1. Individual manual jogging/toggling of each relay output.
2. Real-time visual indicator lamps for digital inputs (inductive sensors).
3. Raw analog voltage readouts for ADC keypads.
4. Live dual thermocouple temperature readings.

---

## 2. Screen Layout & Widget Architecture (480x320)

```text
+-------------------------------------------------------------+
| [BACK]            HARDWARE I/O DIAGNOSTICS         [RTC: OK] |
+-------------------------------------------------------------+
| DIGITAL OUTPUTS (RELAYS)      | DIGITAL INPUTS (SENSORS)    |
| [Q1] Funnel Motor   [TOGGLE]  | [I6] Funnel Home   (● GREEN)|
| [Q7] Piston Motor   [TOGGLE]  | [I7] Piston Home   (○ GRAY) |
| [Q2] Inverter FWD   [MOMENT]  +-----------------------------+
| [Q3] Inverter REV   [MOMENT]  | ANALOG READINGS             |
| [Q5] Warning Buzzer [BEEP]    | KEY_F ADC: 4092 (No Key)    |
|                               | KEY_M ADC: 2045 (M4 Pressed)|
|                               | THERMO 1: 26.5 °C           |
|                               | THERMO 2: 31.0 °C           |
+-------------------------------------------------------------+
```

---

## 3. Widget Specifications & Binding Table

### 3.1. Output Relays Section:
- **`btn_diag_funnel_motor`:** Toggles `FUNNELE_MOTOR` (GPIO 42).
- **`btn_diag_piston_motor`:** Toggles `PISTON_MOTOR` (GPIO 36).
- **`btn_diag_inv_fwd`:** Momentary press drives `INVERTER_FORWARD` (GPIO 39) with hardware interlock.
- **`btn_diag_inv_rev`:** Momentary press drives `INVERTER_REVERSE` (GPIO 48) with hardware interlock.
- **`btn_diag_buzzer`:** Triggers a 200ms tone on `BUZZER` (GPIO 45).

### 3.2. Live Sensor Polling:
- Polled every 50ms inside `tick_screen_hw_diag()`:
  ```cpp
  void tick_screen_hw_diag() {
      // Funnel Sensor (I6)
      bool f_state = (digitalRead(FUNNELE_SENSOR) == LOW);
      lv_obj_set_style_bg_color(led_funnel_sensor, 
          f_state ? lv_color_hex(0x00FF00) : lv_color_hex(0x404040), 0);

      // Piston Sensor (I7)
      bool p_state = (digitalRead(PISTON_SENSOR) == LOW);
      lv_obj_set_style_bg_color(led_piston_sensor, 
          p_state ? lv_color_hex(0x00FF00) : lv_color_hex(0x404040), 0);

      // ADC Voltages
      int rawF = analogRead(KEY_F);
      int rawM = analogRead(KEY_M);
      lv_label_set_text_fmt(lbl_adc_f, "KEY_F: %d", rawF);
      lv_label_set_text_fmt(lbl_adc_m, "KEY_M: %d", rawM);

      // Thermocouple readings
      float t1 = thermocouple_1.readCelsius();
      float t2 = thermocouple_2.readCelsius();
      lv_label_set_text_fmt(lbl_thermo_1, "T1: %.1f C", t1);
      lv_label_set_text_fmt(lbl_thermo_2, "T2: %.1f C", t2);
  }
  ```

---

## 4. Safety Interlocks While in Diagnostics Mode

1. Exiting the diagnostics screen unconditionally cuts off all motor relays (`funneleStop()`, `pistonStop()`).
2. Automatic portioning FSM is completely suspended while this screen is active.
3. Inverter Forward and Reverse remain software-interlocked even in manual diagnostics mode.
