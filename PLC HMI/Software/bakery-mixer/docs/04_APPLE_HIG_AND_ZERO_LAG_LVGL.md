# Apple HIG Craftsmanship & Zero-Lag LVGL Rendering
## Industrial Touchscreen UI Design for 480x320 Displays, 50+ FPS Guarantee, and RTL BiDi Isolation

> **Document ID:** `04_APPLE_HIG_AND_ZERO_LAG_LVGL.md`  
> **Target Hardware:** 4" 480x320 Landscape TFT with LVGL (v8 or v9) on ESP32-S3.  
> **Standards:** Apple Human Interface Guidelines (HIG) & LVGL Zero-Lag Engine.

---

## 1. Apple Industrial Aesthetic Philosophy

An industrial HMI machine controller should feel as precise, refined, and intuitive as an Apple iPad or medical-grade instrument:

```
+-------------------------------------------------------------+
| 480x320 LANDSCAPE - APPLE HIG CARD-BASED GEOMETRY           |
|                                                             |
| +---------------------------------------------------------+ |
| | Header 40px: Machine Status       DS3231 Time   [F5 Tab]| |
| +---------------------------------------------------------+ |
|                                                             |
| +------------------------+   +----------------------------+ |
| | Hero Metric Card       |   | Settings & Stepper Card    | |
| | Mathematical Centering |   | 16px Breathing Margin      | |
| | 44px Touch Targets     |   | 1px Crisp Subtle Border    | |
| +------------------------+   +----------------------------+ |
|                                                             |
| +---------------------------------------------------------+ |
| | Footer 44px: Emerald Green Start / Apple Blue Actions   | |
| +---------------------------------------------------------+ |
+-------------------------------------------------------------+
```

![Apple HIG Industrial Touchscreen Dashboard](./assets/images/hmi_main_dashboard.png)

### 1.1. Approved Industrial Color Palettes:
1. **Dark Matte Slate Palette (Factory Default):**
   - Canvas Base: Deep slate navy (`#0A0C10` to `#0F172A`).
   - Grouping Cards: Matte dark charcoal (`#161B22` or `#1E293B`).
   - Borders: Ultra-clean 1px hairline lines (`#30363D` or `#334155`).
   - Semantic Colors: Emerald Green (`#10B981` / `#22C55E`) for Start/Ready, Apple System Blue (`#007AFF` / `#0EA5E9`) for active selections, Industrial Amber (`#F59E0B`) for safety hold, Coral Red (`#EF4444`) for critical E-stop.
2. **Apple Pure White Palette (Optional for Cleanrooms / Medical / Food):**
   - Canvas Base: Light slate canvas (`#F8FAFC`).
   - Grouping Cards: Pure snow-white (`#FFFFFF`) with 1px border `#E2E8F0`.
   - Typography: Deep slate charcoal (`#0F172A`) for $>12:1$ contrast ratio.

---

## 2. The 3 Pillars of 50+ FPS Zero-Lag Rendering

### 2.1. Absolute Prohibition of Software Shadows (Zero Shadows)
Computing blurred box shadows in software on the ESP32-S3 consumes millions of floating-point operations per second, instantly dropping rendering performance from 60 FPS to $< 15$ FPS.
* **MANDATORY RULE:** Explicitly disable software shadows on all dynamic cards, buttons, and badges:
  ```c
  lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  ```
* Visual depth and elevation must be achieved purely through background contrast and 1px crisp borders (`border_width = 1`).

### 2.2. Dirty-Driven Rendering Architecture
Calling `lv_label_set_text` or setting styles unconditionally inside periodic 20ms render loops invalidates the display buffer and forces heavy recalculation even when the underlying value hasn't changed.
* **Mandatory Setter Guard Pattern:**
  ```c
  void update_metric_display(float new_val) {
      static float s_last_val = -9999.0f;
      if (fabsf(new_val - s_last_val) > 0.01f) {
          s_last_val = new_val;
          char buf[16];
          snprintf(buf, sizeof(buf), "%.2f", new_val);
          lv_label_set_text(lbl_metric, buf); // Only executes when data actually changes
      }
  }
  ```

### 2.3. Zero Dynamic Memory Allocation in Periodic Loops
* Never call `malloc`, `new`, or perform dynamic string concatenation inside periodic UI render loops or FreeRTOS timers.
* Use static stack buffers (`char buf[32]`) or pre-allocated PSRAM pools.

---

## 3. Mathematical Centering of Numbers & Hero Metrics

In industrial touchscreens, dropped or off-center digits look amateurish:
1. **Zero-Padding Containers:** The metric container must have zero padding and zero gap:
   ```c
   lv_obj_set_style_pad_all(card_num, 0, 0);
   lv_obj_set_style_pad_gap(card_num, 0, 0);
   ```
2. **Centering Function:** Call `lv_obj_center(lbl_num)` inside the dedicated metric container to lock the digits at the exact optical and mathematical center.
3. **Decoupled Unit Labels:** Never concatenate unit strings (`mm`, `°C`, `Bar`) inside the numeric label. Units must be independent label objects positioned alongside the numeric card to prevent font baseline distortion.

---


---

## 4. 16px Breathing Margin Rule for Steppers

When designing parameter increment/decrement controls (`+` and `-` buttons):
* The distance between the increment button edge and the title label must be **at least 16 pixels** of breathing space:
  ```c
  lv_obj_align_to(lbl_title, btn_plus, LV_ALIGN_OUT_RIGHT_MID, 16, 0);
  ```
* Button touch targets must measure at least **44x44 pixels** for effortless fingertip operation.

![Apple HIG Settings & Steppers Layout](./assets/images/hmi_settings_steppers.png)

### 4.1. Recipe Preset Cards
* Store up to 16 presets in NVS with dedicated title and parameter fields.
* Active preset indicator uses an Apple HIG pill badge (`#007AFF` blue active glow, 52x24px).

---

## 5. Touchless Front-Panel Focus Navigation
* When navigating without touch, active widgets display a 2px Apple Blue (`#007AFF`) outline:
  ```c
  lv_obj_set_style_outline_width(obj, 2, LV_STATE_FOCUSED);
  lv_obj_set_style_outline_color(obj, lv_color_hex(0x007AFF), LV_STATE_FOCUSED);
  lv_obj_set_style_outline_pad(obj, 2, LV_STATE_FOCUSED);
  ```

![100% Touchless Front-Panel Navigation](./assets/images/touchless_mode_navigation.png)

## 5. Multilingual Typography & Persian RTL BiDi Isolation

### 5.1. Direction Isolation (BiDi Isolation)
* For Persian and Arabic containers and text, enforce RTL direction:
  ```c
  lv_obj_set_style_base_dir(obj_fa, LV_BASE_DIR_RTL, 0);
  ```
* For numbers, IP addresses, URLs, and Latin technical terms, enforce LTR direction:
  ```c
  lv_obj_set_style_base_dir(obj_en, LV_BASE_DIR_LTR, 0);
  ```
* **Prohibition:** Never embed English words or parentheses directly into Persian strings without explicit direction isolation; doing so triggers reverse character scrambling in LVGL's BiDi engine.

### 5.2. Zero Character Box (Zero Tofu) Guarantee
* Ensure fonts include all necessary Persian glyphs (Kalameh, Dana, Vazir) with fallback chains to Montserrat for ASCII numbers.

---

## 6. Apple HIG Keypad with Eye Toggle Component

For PIN entry, calibration, and lease day-lock:
* **Eye Toggle Button:** Allows the operator to switch freely between masked bullet mode (`• • • •`) and visible numbers in noisy or dimly lit workshop environments.
* **Semantic Button Coloring:** Emerald Green (`#10B981`) for OK confirmation with press drop (+2px Y-offset); Coral Red (`#EF4444`) for Clear/DEL.

---

## 7. Locked Virtual Keyboard Vertical Offsets Mandate

All on-screen virtual keyboards and keypads have rigorously calibrated vertical offsets to guarantee pixel-perfect icon rendering, preventing button label clipping or glyph truncation during dynamic layout recomputation:

```cpp
// MANDATORY LOCKED VERTICAL OFFSETS (DO NOT ALTER)
#define KB_Y_OFF_LINK_ENTRY    (-4) // Link Entry Keyboard (objects.link_entry_keyboard)
#define KB_Y_OFF_WIFI_PW       (-4) // WiFi Password Keyboard (objects.keybord)
#define KB_Y_OFF_COMPANY_NAME  ( 3) // Company Name Keyboard (company_kb)
#define KB_Y_OFF_DEFAULT_NUM   ( 8) // Admin Numeric Keypads
```

> 🚨 **PROHIBITION:** Never alter these vertical offsets or attempt to merge separate keyboard instances into a single shared object. Doing so breaks the layout alignment and causes backspace, enter, and toggle glyphs to clip vertically on 480x320 panels.

---

## 8. EEZ Studio 20ms Tick Protection Mandate

In projects utilizing EEZ Studio for UI code generation:
1. Every screen features a dedicated `tick_screen_*` function executed every 20ms by the LVGL render loop.
2. In each frame, `tick_screen_*` actively **overwrites widget properties and label text** from the central variable registry in `src/vars.c`.
3. **CRITICAL DEV TRAP:** Modifying a widget solely inside `create_screen_*` or calling `lv_label_set_text` ad-hoc will appear to work for 1 frame, but will be completely erased and overwritten 20ms later by `tick_screen_*`!
4. **The Safe Workflow:**
   - Always update the backing variable in `src/vars.c` (e.g. `set_var_blade_speed(3200)`).
   - Alternatively, place dynamic label formatting directly inside the screen's `tick_screen_*` function with a dirty-flag guard (`if_changed`).
