# INDUSTRIAL PLC HMI: 100% TOUCHLESS FRONT-PANEL KEYPAD NAVIGATION & "MODE" KEY SYSTEM SPECIFICATION

> **AGENT DIRECTIVE / SYSTEM PROMPT FOR AI CODING ASSISTANTS**
> 
> **Role:** You are an expert Principal Industrial Embedded Firmware Engineer specializing in safety-critical PLC HMI development (combining **Siemens Industrial Determinism & Rigor** with **Apple HIG Visual Craftsmanship & Ergonomics**).
>
> **Mission:** Implement a deterministic, zero-lag, 100% touchless navigation system utilizing physical front-panel tactile keys alongside an LVGL GUI display (ESP32-S3, STM32, or industrial RTOS controller).
>
> **Core Purpose:** The machine operator must be able to navigate every screen, adjust every setting, enter numbers/passwords, and operate the PLC with 100% reliability using **physical front-panel keys alone**—even when wearing thick industrial gloves, or if the capacitive/resistive touch digitizer fails or shatters.

---

## 1. HARDWARE KEYPAD ARCHITECTURE & PIN MAPPING

The industrial front panel features **10 physical tactile/membrane keys** organized into two distinct functional columns (or rows of 5 keys):

```
+-------------------------------------------------------------+
|                     INDUSTRIAL PLC HMI                      |
|                                                             |
|   [F1] Left / Fast Jump 1           [SAVE]   NVS Commit     |
|   [F2] Right / Fast Jump 2          [CANCEL] Back / Exit    |
|   [F3] Select / Enter               [MODE]   Focus Cycle    |
|   [F4] Auxiliary / Shortcut         [START]  Confirm / Inc  |
|   [F5] Menu / Tab Switch            [STOP]   Dec / Abort    |
|                                                             |
+-------------------------------------------------------------+
```

### 1.1 Keypad Layout & Default Assignment

| Key Name | Index | Primary Function (Normal Running) | Navigation Function (`MODE` Active) |
| :--- | :---: | :--- | :--- |
| **F1** | 1 (Col 1) | Screen/Tab Shortcut 1 | Direct jump to Section 1 |
| **F2** | 2 (Col 1) | Screen/Tab Shortcut 2 | Direct jump to Section 2 |
| **F3** | 3 (Col 1) | Screen/Tab Shortcut 3 | Direct jump to Section 3 |
| **F4** | 4 (Col 1) | Screen/Tab Shortcut 4 | Direct jump to Section 4 |
| **F5** | 5 (Col 1) | Settings / Main Menu | Direct jump to Section 5 |
| **SAVE** | 1 (Col 2) | Parameter Save / Commit to NVS | Persist changes immediately to flash |
| **CANCEL** | 2 (Col 2) | Back / Dismiss Modal | Clear focus cursor / Exit navigation |
| **MODE** | 3 (Col 2) | **Activate Navigation Mode** | **Cycle focus to next actionable widget** |
| **START** | 4 (Col 2) | **Start Physical Machine Cycle** | **Confirm, Click, Toggle, or Increment (+1)** |
| **STOP** | 5 (Col 2) | **Stop / Emergency Halt Cycle** | **Decrement (-1), Backspace, or Close** |

### 1.2 Signal Conditioning & Debouncing (Siemens Standard)
If keys are multiplexed via an analog resistor ladder (ADC):
1. **5-Sample Median Filter:** Eliminate analog noise from industrial motors/contactors (`readMedian5()`).
2. **Hardware Mutex Lock:** Protect ADC peripheral across concurrent RTOS tasks.
3. **Debounce Window:** 45ms to 50ms stable window required before confirming key state.
4. **Thread Segregation:** The keypad scanning task runs on the **System Core (Core 0)**; LVGL runs on the **UI Core (Core 1)**. **NEVER call LVGL APIs from the keypad task or ISR.** Feed key events into a thread-safe atomic queue or FreeRTOS message queue (`queue_panel_navigation(row, key)`).

---

## 2. THE "MODE" KEY OPERATIONAL SCENARIO & STATE MACHINE

The system is governed by a strict 3-State Finite State Machine (FSM):

```
                   +------------------------+
                   |    NAV_STATE_IDLE      |  (Cursor Hidden)
                   |  (Normal Touch / Run)  |  (Start/Stop run machine)
                   +------------------------+
                             |     ^
               Press [MODE]  |     |  10s Inactivity /
                             v     |  Touch detected /
                   +------------------------+  [CANCEL] / End of list
                   |   NAV_STATE_ACTIVE     |
                   | (Focus Cursor Visible) |
                   |  (Start/Stop adjust UI)|
                   +------------------------+
                             |     ^
             Enter Numpad /  |     |  [STOP] key /
             Virtual Keybd   v     |  Keyboard Dismiss
                   +------------------------+
                   |   NAV_STATE_KEYBOARD   |
                   | (Cursor inside Virtual |
                   |  Keyboard ButtonMatrix)|
                   +------------------------+
```

### 2.1 Transition Specifications

#### A. Waking Navigation Mode (`NAV_STATE_IDLE` -> `NAV_STATE_ACTIVE`)
- Operator presses **`MODE` (Key 3)**.
- The visual focus ring appears immediately on the **first actionable item** on the active screen.
- An immediate short audio click (buzzer notification) provides tactile feedback.
- Activity timestamp is recorded: `last_activity_ms = millis()`.

#### B. Sequential Focus Traversal
- Each subsequent press of **`MODE`** advances focus to the next actionable item in natural reading order:
  $$\text{index}_{\text{next}} = (\text{index}_{\text{current}} + 1) \pmod N$$
- The previous widget's focus style/state is cleanly wiped.
- The next widget is highlighted with the focus ring.
- Automatic scrolling: If the newly focused widget is inside a scrollable container, call `lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON)` so the widget smoothly rolls into view.

#### C. End-of-Cycle Dismissal (Wrap-Around Rule)
- When focus reaches the **last actionable item** on the screen and the operator presses **`MODE`** again:
  - The focus ring is completely removed.
  - State returns to **`NAV_STATE_IDLE`**.
  - A brief, subtle toast notification ("Navigation Exited") is optionally shown.
  - Pressing `MODE` once more will restart the cycle from **item 0**.

#### D. Inactivity Auto-Dismissal (10-Second Watchdog)
- If navigation is active and no front-panel key is pressed for **10.0 seconds**:
  - Automatically call `panel_nav_exit_mode()`.
  - Cursor is hidden, and `NAV_STATE_IDLE` is restored.
  - Machine returns to standard physical running mode.

#### E. Touchscreen Override
- Any physical touch detected on the display digitizer (`LV_EVENT_PRESSED`) **instantly dismisses** the focus ring and resets the state machine to `NAV_STATE_IDLE`. Physical touch always maintains absolute operational priority.

#### F. Screen Transition & Modal Purge
- Whenever a screen transition occurs (e.g., changing pages, loading a new view) or a modal dialog is spawned:
  - All existing focus pointers are immediately invalidated (`NULL`).
  - Active focus rings on the previous view are destroyed to prevent lingering visual ghosts.

---

## 3. SMART FOCUS CURSOR & WIDGET FILTERING

### 3.1 Strict "Actionable-Only" Filter (Zero Visual Clutter)
**RED LINE:** The focus cursor must **NEVER** land on inert or decorative widgets:
- ❌ **Forbidden:** Static labels, explanatory text, background cards, image logos, decorative borders, non-interactive metric displays.
- ✅ **Permitted (Actionable Only):**
  1. Buttons (`lv_button_t` / `lv_btn_t`)
  2. Toggle Switches / Checkboxes (`lv_switch_t`, `LV_OBJ_FLAG_CHECKABLE`)
  3. Numeric Sliders & Arc Gauges (`lv_slider_t`, `lv_arc_t`)
  4. Spinboxes, Textareas & Numeric Input Fields
  5. Dropdowns & Rollers (`lv_dropdown_t`, `lv_roller_t`)
  6. Clickable Cards & List Elements

### 3.2 Modal Dialog Priority & Isolation
When a modal dialog (e.g., Emergency Alert, Confirmation Dialog, Factory Reset Prompt, Help Sheet) is displayed on screen:
- **Absolute Focus Confinement:** The item collector **MUST ONLY** collect the action buttons belonging to that modal (e.g., `[Confirm]`, `[Cancel]`, `[Close]`).
- **Background Quarantine:** Background widgets, sidebar tabs, and main navigation buttons **MUST BE STRICTLY EXCLUDED** from the focus list while the modal is open.
- This guarantees the cursor never "disappears" behind modal backdrops or triggers invisible background actions.

### 3.3 Apple HIG Visual Craftsmanship (Zero Layout Drift)
- **Zero Geometry Modification:** Setting focus must NEVER alter `x`, `y`, `width`, `height`, or margins of the target widget.
- **Visual Style:** Apply a clean, prominent 2px to 4px focus ring with smooth rounded corners matching the target widget's own radius:
  ```c
  lv_obj_set_style_outline_color(obj, lv_color_hex(0x0EA5E9), LV_PART_MAIN | LV_STATE_DEFAULT); // Apple Neon Cyan or Industrial Amber
  lv_obj_set_style_outline_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_pad(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  ```
- **Layout Pre-Calculation:** Always execute `lv_obj_update_layout(obj)` before calculating bounding coordinates, ensuring the focus ring renders in the exact correct position even immediately after dynamic screen creation.

---

## 4. DUAL PERSONALITY OF "START" & "STOP" KEYS

The behavior of `START` (Key 4) and `STOP` (Key 5) dynamically shifts based on the active state:

```
+---------------+-----------------------------+------------------------------------+
| KEY           | NORMAL RUNNING (IDLE)       | NAVIGATION MODE (ACTIVE)           |
+---------------+-----------------------------+------------------------------------+
| START (Key 4) | Starts physical machine PLC | Confirm / Click / Toggle / Inc (+1)|
| STOP  (Key 5) | Immediate Cycle Stop / Halt | Decrement (-1) / Back / Delete     |
+---------------+-----------------------------+------------------------------------+
```

### 4.1 Detailed Widget Action Mapping

#### 1. Push Buttons (`lv_button` / Clickable Objects)
- **`START`:** Emits `LV_EVENT_CLICKED` to trigger the button's callback.
- **`STOP`:** **DOES NOTHING.** (Crucial Safety Rule: `STOP` must never accidentally trigger a positive button action).

#### 2. Toggle Switches (`lv_switch` / Checkable Items)
- **`START`:** Inverts state (`LV_STATE_CHECKED`), then emits `LV_EVENT_VALUE_CHANGED`.

#### 3. Numeric Sliders, Arcs & Spinners
- **`START`:** Increments value by 1 unit (`+1`).
- **`STOP`:** Decrements value by 1 unit (`-1`).
- **Safety Boundary Clamping:** Clamped strictly between `min_value` and `max_value`. Attempting to exceed bounds triggers a gentle visual warning toast.
- **Continuous Hold / Auto-Repeat:**
  - Holding `START` or `STOP` continuously for **$> 1000\text{ms}$** activates auto-repeat.
  - Ticks every **$150\text{ms}$** with smooth acceleration, allowing rapid parameter modification without repeated tapping.

#### 4. Dropdowns & Rollers
- **`START`:** Selects next option (`sel++`).
- **`STOP`:** Selects previous option (`sel--`).

#### 5. Virtual Keyboard / Numpad Mode
- When focus enters a textarea or numpad:
  - State switches to **`NAV_STATE_KEYBOARD`**.
  - **`MODE`:** Moves focus across individual keyboard keys (emits `LV_EVENT_KEY` with `LV_KEY_RIGHT`).
  - **`START`:** Presses the highlighted virtual key (emits `LV_EVENT_VALUE_CHANGED`).
  - **`STOP`:** Executes Backspace (`lv_textarea_del_char(ta)`).
  - Pressing **`CANCEL`** exits keyboard mode and returns to screen-level navigation.

---

## 5. TIMED BANNER & TOAST SYSTEM (DRAINING PROGRESS BAR & TOUCH-FREEZE)

Whenever feedback or limits are communicated (e.g., "Max Limit Reached", "Settings Saved", "Invalid Range"):

```
+-------------------------------------------------------------+
|  (i) Maximum Temperature Limit Reached: 260°C               |
|  [==========================>                              ] | <-- Draining timer bar
+-------------------------------------------------------------+
```

1. **Floating Card:** Rendered on `lv_layer_top()` with Apple-style matte dark aesthetic (`#1E293B`), 12px corner radius, and crisp multilingual typography.
2. **Draining Progress Bar:** A slim 3px bar at the bottom edge starts at 100% and smoothly drains to 0% over **3.5 to 4.0 seconds**. When it hits zero, the banner auto-dismisses.
3. **Touch-and-Hold Freeze:** If the operator presses and holds their finger on the toast banner:
   - The draining animation **immediately pauses**.
   - The banner stays visible indefinitely as long as contact is maintained, allowing operators full time to read complex error messages.
   - Releasing contact resumes the timer countdown.
   - A single short tap immediately closes the toast.

---

## 6. CANCEL & SAVE KEY BEHAVIORS

### 6.1 CANCEL Key (Key 2)
- If a modal dialog or popup is open $\rightarrow$ Closes the dialog.
- If Virtual Keyboard is focused $\rightarrow$ Exits keyboard mode back to general widgets.
- If `NAV_STATE_ACTIVE` $\rightarrow$ Clears focus cursor and returns to `NAV_STATE_IDLE`.
- If on a sub-settings screen $\rightarrow$ Exits sub-screen and returns to the Home Dashboard.

### 6.2 SAVE Key (Key 1)
- Batches all pending configuration changes on the current screen.
- Commits changes to non-volatile storage (NVS / EEPROM / Flash).
- Triggers a crisp double-beep on the buzzer.
- Displays an Apple-style green confirmation toast ("Settings Saved Successfully").

---

## 7. PRODUCTION-READY C/C++ REFERENCE ARCHITECTURE

AI agents must structure the implementation around this clean, modular architecture:

### 7.1 Header Interface (`panel_nav.h`)

```c
#ifndef PANEL_NAV_H
#define PANEL_NAV_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NAV_STATE_IDLE = 0,       // Cursor hidden; normal touch & machine operation
    NAV_STATE_ACTIVE = 1,     // Cursor active; navigating screen widgets
    NAV_STATE_KEYBOARD = 2    // Cursor navigating on-screen virtual keyboard/numpad
} PanelNavState;

typedef struct {
    PanelNavState state;
    lv_obj_t *focus_obj;
    lv_obj_t *items[64];
    int item_count;
    int current_index;
    uint32_t last_activity_ms;
    int current_screen_id;
} PanelNavManager;

// Core API
void panel_nav_init(void);
void panel_nav_queue_key(uint8_t row, uint8_t key); // Thread-safe queue entry
void panel_nav_poll_tick(void);                     // Called from LVGL periodic timer
void panel_nav_exit_mode(void);
bool panel_nav_is_active(void);
void panel_nav_notify_touch(void);                  // Call on any digitizer touch

#ifdef __cplusplus
}
#endif

#endif // PANEL_NAV_H
```

### 7.2 Implementation Engine (`panel_nav.c`)

```c
#include "panel_nav.h"
#include <string.h>

static PanelNavManager g_nav = {0};
static volatile uint16_t g_pending_key_event = 0;

// Thread-safe function called from Keypad Task (Core 0)
void panel_nav_queue_key(uint8_t row, uint8_t key) {
    g_pending_key_event = (uint16_t)((row << 8) | key);
}

// Check if widget is actionable
static bool is_actionable(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;
    
    // Check if widget itself or parent is hidden
    lv_obj_t *parent = lv_obj_get_parent(obj);
    while (parent) {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) return false;
        parent = lv_obj_get_parent(parent);
    }
    
    // Actionable types
    if (lv_obj_check_type(obj, &lv_button_class)) return true;
    if (lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class)) return true;
    if (lv_obj_check_type(obj, &lv_arc_class)) return true;
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
    if (lv_obj_check_type(obj, &lv_roller_class)) return true;
    if (lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_check_type(obj, &lv_keyboard_class)) return true;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) return true;
    
    return false;
}

// Clear focus visual style
static void clear_focus_ring(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_buttonmatrix_class)) {
        lv_buttonmatrix_set_selected_button(obj, LV_BUTTONMATRIX_BUTTON_NONE);
        lv_obj_clear_state(obj, LV_STATE_FOCUSED);
    }
}

// Apply focus visual style
static void apply_focus_ring(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_update_layout(obj);
    
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x0EA5E9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Smooth auto-scroll
    lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON);
}

void panel_nav_exit_mode(void) {
    if (g_nav.focus_obj) {
        clear_focus_ring(g_nav.focus_obj);
        g_nav.focus_obj = NULL;
    }
    g_nav.state = NAV_STATE_IDLE;
    g_nav.current_index = -1;
}

void panel_nav_notify_touch(void) {
    if (g_nav.state != NAV_STATE_IDLE) {
        panel_nav_exit_mode();
    }
}

// Collect actionable widgets on current active screen
static void collect_actionable_widgets(lv_obj_t *parent) {
    if (!parent) return;
    uint32_t cnt = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < cnt && g_nav.item_count < 64; ++i) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (is_actionable(child)) {
            g_nav.items[g_nav.item_count++] = child;
        }
        collect_actionable_widgets(child);
    }
}

// Adjust focused value with START (dir = +1) or STOP (dir = -1)
static void adjust_focused_widget(int dir) {
    lv_obj_t *obj = g_nav.focus_obj;
    if (!obj || !lv_obj_is_valid(obj)) return;

    if (lv_obj_check_type(obj, &lv_slider_class)) {
        int32_t v = lv_slider_get_value(obj) + dir;
        if (v < lv_slider_get_min_value(obj)) v = lv_slider_get_min_value(obj);
        if (v > lv_slider_get_max_value(obj)) v = lv_slider_get_max_value(obj);
        lv_slider_set_value(obj, v, LV_ANIM_ON);
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }
    
    if (lv_obj_check_type(obj, &lv_switch_class)) {
        if (dir > 0) {
            if (lv_obj_has_state(obj, LV_STATE_CHECKED)) lv_obj_clear_state(obj, LV_STATE_CHECKED);
            else lv_obj_add_state(obj, LV_STATE_CHECKED);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }
    
    if (lv_obj_check_type(obj, &lv_keyboard_class)) {
        if (dir > 0) {
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL); // Press selected key
        } else {
            lv_obj_t *ta = lv_keyboard_get_textarea(obj);
            if (ta) lv_textarea_delete_char(ta); // Backspace
        }
        return;
    }
    
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        if (dir > 0) {
            lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL);
        }
        return;
    }
}

// Periodic tick handler executing on UI Task (Core 1)
void panel_nav_poll_tick(void) {
    uint32_t now = lv_tick_get();

    // 10-Second Inactivity Timeout
    if (g_nav.state != NAV_STATE_IDLE) {
        if ((now - g_nav.last_activity_ms) >= 10000U) {
            panel_nav_exit_mode();
            return;
        }
    }

    if (g_pending_key_event == 0) return;

    uint8_t row = (g_pending_key_event >> 8) & 0xFF;
    uint8_t key = g_pending_key_event & 0xFF;
    g_pending_key_event = 0;
    g_nav.last_activity_ms = now;

    // CANCEL Key (Row 2, Key 2)
    if (row == 2 && key == 2) {
        panel_nav_exit_mode();
        return;
    }

    // MODE Key (Row 2, Key 3) -> Advance Focus Cycle
    if (row == 2 && key == 3) {
        if (g_nav.state == NAV_STATE_IDLE) {
            g_nav.item_count = 0;
            collect_actionable_widgets(lv_screen_active());
            if (g_nav.item_count == 0) return;
            g_nav.current_index = 0;
            g_nav.state = NAV_STATE_ACTIVE;
            g_nav.focus_obj = g_nav.items[0];
            apply_focus_ring(g_nav.focus_obj);
            return;
        }

        if (g_nav.state == NAV_STATE_ACTIVE) {
            clear_focus_ring(g_nav.focus_obj);
            g_nav.current_index++;
            
            // Reached end of items -> Auto-dismiss
            if (g_nav.current_index >= g_nav.item_count) {
                panel_nav_exit_mode();
                return;
            }

            g_nav.focus_obj = g_nav.items[g_nav.current_index];
            apply_focus_ring(g_nav.focus_obj);
            return;
        }

        if (g_nav.state == NAV_STATE_KEYBOARD) {
            // Cycle keys on virtual keyboard
            uint32_t right_key = LV_KEY_RIGHT;
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_KEY, &right_key);
            return;
        }
    }

    // START (Key 4) -> Increment / Click (+1)
    if (g_nav.state != NAV_STATE_IDLE && row == 2 && key == 4) {
        adjust_focused_widget(+1);
        return;
    }

    // STOP (Key 5) -> Decrement / Delete (-1)
    if (g_nav.state != NAV_STATE_IDLE && row == 2 && key == 5) {
        adjust_focused_widget(-1);
        return;
    }
}
```

---

## 8. SUMMARY CHECKLIST FOR THE IMPLEMENTING AGENT

When implementing this touchless navigation architecture in your PLC project, verify every item:
- [ ] **Hardware Isolation:** Keypad ADC/GPIO reads are median-filtered and completely decoupled from LVGL via thread-safe queues.
- [ ] **Modal Quarantine:** When an alert or modal pops up, focus is 100% restricted to the modal's buttons; background widgets are unreachable.
- [ ] **Inactivity Auto-Exit:** Focus ring disappears automatically after 10 seconds of no key presses.
- [ ] **Touch Override:** Touching the display instantly hides the focus ring and restores `NAV_STATE_IDLE`.
- [ ] **Zero Layout Drift:** Highlighting an object never causes layout jitter or size alterations.
- [ ] **Dual Personality:** `START` and `STOP` run the physical machine when in `IDLE`, but adjust UI parameters when `MODE` is active.
- [ ] **Auto-Repeat:** Holding `START`/`STOP` on sliders/spinners for $>1\text{s}$ triggers high-speed auto-increment/decrement.
- [ ] **Zero Memory Leak:** All focus pointers are null-checked and reset across screen changes.
