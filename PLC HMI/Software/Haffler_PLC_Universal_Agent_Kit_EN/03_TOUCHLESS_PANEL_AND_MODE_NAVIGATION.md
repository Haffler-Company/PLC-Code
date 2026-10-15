# 100% Touchless Front-Panel Navigation & "MODE" Key Subsystem
## Work-Glove Ergonomics, 3-State Navigation FSM, and Production LVGL Touchless Architecture

> **Document ID:** `03_TOUCHLESS_PANEL_AND_MODE_NAVIGATION.md`  
> **Target Hardware:** All Haffler Industrial Machinery with 10 Physical Front-Panel Keys.  
> **Standards:** Siemens Deterministic Field HMI Navigation & Apple HIG Focus Grammar.

---

## 1. Engineering Motivation & Operational Purpose

In real-world manufacturing plants:
1. Operators wear thick leather or greasy work gloves, making capacitive touch screens difficult to operate.
2. Touch digitizer glass can crack or become obscured by flying metal chips or oil mist.
3. **The Haffler Mandate:** The entire machine HMI (every screen, parameter, setting, virtual keyboard, and modal dialog) must be **100% operable using the 10 physical front-panel keys alone**, without touching the screen a single time.

---

## 2. 3-State Navigation Finite State Machine (FSM)

```text
                   +------------------------+
                   |    NAV_STATE_IDLE      |  (Focus cursor hidden)
                   |  (Standard Touch / Run)|  (START/STOP control physical machine)
                   +------------------------+
                             |     ^
                Press [MODE] |     |  10s Inactivity /
                             v     |  Touch detected / [CANCEL] / End of items
                   +------------------------+
                   |   NAV_STATE_ACTIVE     |
                   | (Neon Blue Ring Visible|
                   | (START/STOP adjust UI) |
                   +------------------------+
                             |     ^
                Enter Virtual|     |  [CANCEL] key /
                Numpad/Keybd v     |  Dismiss keyboard
                   +------------------------+
                   |   NAV_STATE_KEYBOARD   |
                   | (Traversing Virtual    |
                   |  Keypad Button Matrix) |
                   +------------------------+
```

![Touchless Front-Panel Navigation & Mode Traversal](./assets/images/touchless_keypad_nav.png)

---

## 3. The "MODE" Key Behavioral Specification

The **MODE key (Column 2, Key 3)** serves as the central navigation focus controller:

1. **Waking Navigation Mode:**
   - In `NAV_STATE_IDLE`, the first press of `MODE` immediately displays the neon cyan focus ring (`#0EA5E9`) on the **first actionable widget** on the active screen.
   - An instant 15ms acoustic click provides tactile confirmation.
2. **Sequential Focus Traversal:**
   - Each subsequent press of `MODE` advances the focus ring to the next actionable item:
     $$\text{Index}_{\text{next}} = (\text{Index}_{\text{current}} + 1) \pmod N$$
   - **Smooth Auto-Scroll:** If the newly focused widget is located outside the visible viewport (inside a scrollable container), the system calls `lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON)` to smoothly roll the widget into view.
3. **Wrap-Around Exit Rule:**
   - When focus reaches the **last actionable item** on the screen and the operator presses `MODE` again, navigation mode terminates automatically and returns to `NAV_STATE_IDLE`. Pressing `MODE` once more re-enters at item 0.
4. **10-Second Inactivity Watchdog:**
   - If no front-panel key is pressed for **10.0 seconds** while in navigation mode, the focus ring is cleared and the machine automatically returns to `NAV_STATE_IDLE`.
5. **Physical Touchscreen Override:**
   - Any physical touch on the display digitizer (`LV_EVENT_PRESSED`) **instantly dismisses** the focus ring and restores `NAV_STATE_IDLE`. Touch retains absolute operational priority.

---

## 4. Dual Personality of "START" and "STOP" Keys

```
+---------------+-----------------------------+------------------------------------+
| Physical Key  | NORMAL RUNNING (IDLE)       | NAVIGATION MODE (ACTIVE)           |
+---------------+-----------------------------+------------------------------------+
| START (Key 4) | Starts physical machine PLC | Confirm / Click / Toggle / Inc (+1)|
| STOP  (Key 5) | Immediate Cycle Stop / Halt | Decrement (-1) / Backspace / Close |
+---------------+-----------------------------+------------------------------------+
```

### 4.1. Detailed Widget Interaction Matrix:
1. **Push Buttons (`lv_button`):**
   - **START:** Emits `LV_EVENT_CLICKED`.
   - **STOP:** **Does nothing.** (Crucial Safety Rule: STOP must never trigger positive actions).
2. **Toggle Switches (`lv_switch`):**
   - **START:** Toggles switch state between ON and OFF.
3. **Sliders, Arcs & Numeric Fields (`lv_slider`, `lv_arc`, `lv_spinbox`):**
   - **START:** Increments value by 1 unit (`+1`).
   - **STOP:** Decrements value by 1 unit (`-1`).
   - **Auto-Repeat:** Holding START or STOP continuously for $> 1000\text{ms}$ engages rapid increment/decrement every 150ms.
4. **Virtual Keyboard & Numpad:**
   - **MODE:** Cycles focus across keyboard keys.
   - **START:** Inserts the selected character.
   - **STOP:** Performs Backspace (`lv_textarea_delete_char`).
   - **CANCEL:** Exits keyboard mode back to screen widgets.

---

## 5. Actionable-Only Filter & Modal Quarantine

### 5.1. Strict Actionable Filter (Zero Visual Clutter)
**RED LINE:** The focus cursor must NEVER land on inert or non-interactive elements:
* ❌ **Forbidden:** Static labels, background cards, image logos, decorative borders.
* ✅ **Permitted (Actionable Only):** Buttons (`lv_button`), switches (`lv_switch`), sliders (`lv_slider`), dropdowns (`lv_dropdown`), textareas (`lv_textarea`), and clickable cards.

### 5.2. Modal Dialog Priority & Confinement (Modal Quarantine)
When a modal dialog (alert, error prompt, password entry) appears:
* **Confinement:** The focus collector collects **only the action buttons of that modal** (e.g. `[Confirm]`, `[Cancel]`).
* Background widgets are strictly unreachable to prevent the cursor from disappearing behind backdrops.

---

## 6. Production Reference Architecture (`panel_nav.h` & `panel_nav.c`)

### 6.1. Header Interface: `panel_nav.h`
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
    NAV_STATE_IDLE = 0,
    NAV_STATE_ACTIVE = 1,
    NAV_STATE_KEYBOARD = 2
} PanelNavState;

void panel_nav_init(void);
void panel_nav_process_key(uint8_t col, uint8_t key, bool is_repeat);
void panel_nav_tick(void); // Call periodically from LVGL UI task
void panel_nav_exit_mode(void);
bool panel_nav_is_active(void);
void panel_nav_notify_touch(void);

#ifdef __cplusplus
}
#endif

#endif // PANEL_NAV_H
```

### 6.2. Implementation Engine: `panel_nav.c`
```c
#include "panel_nav.h"

typedef struct {
    PanelNavState state;
    lv_obj_t *focus_obj;
    lv_obj_t *items[64];
    int item_count;
    int current_index;
    uint32_t last_activity_ms;
} PanelNavContext_t;

static PanelNavContext_t g_nav = {0};

static bool is_widget_actionable(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;

    if (lv_obj_check_type(obj, &lv_button_class)) return true;
    if (lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class)) return true;
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
    if (lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) return true;

    return false;
}

static void apply_focus_style(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x0EA5E9), LV_PART_MAIN);
    lv_obj_set_style_outline_width(obj, 3, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(obj, 2, LV_PART_MAIN);
    lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON);
}

static void clear_focus_style(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
}

void panel_nav_exit_mode(void) {
    if (g_nav.focus_obj) {
        clear_focus_style(g_nav.focus_obj);
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

static void collect_widgets_recursive(lv_obj_t *parent) {
    if (!parent) return;
    uint32_t cnt = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < cnt && g_nav.item_count < 64; ++i) {
        lv_obj_t *c = lv_obj_get_child(parent, i);
        if (is_widget_actionable(c)) {
            g_nav.items[g_nav.item_count++] = c;
        }
        collect_widgets_recursive(c);
    }
}

void panel_nav_tick(void) {
    if (g_nav.state != NAV_STATE_IDLE) {
        if (lv_tick_elaps(g_nav.last_activity_ms) >= 10000) {
            panel_nav_exit_mode();
        }
    }
}

void panel_nav_process_key(uint8_t col, uint8_t key, bool is_repeat) {
    g_nav.last_activity_ms = lv_tick_get();

    // CANCEL Key (Col 2, Key 2)
    if (col == 2 && key == 2) {
        panel_nav_exit_mode();
        return;
    }

    // MODE Key (Col 2, Key 3)
    if (col == 2 && key == 3 && !is_repeat) {
        if (g_nav.state == NAV_STATE_IDLE) {
            g_nav.item_count = 0;
            collect_widgets_recursive(lv_screen_active());
            if (g_nav.item_count == 0) return;
            g_nav.current_index = 0;
            g_nav.state = NAV_STATE_ACTIVE;
            g_nav.focus_obj = g_nav.items[0];
            apply_focus_style(g_nav.focus_obj);
            return;
        }

        if (g_nav.state == NAV_STATE_ACTIVE) {
            clear_focus_style(g_nav.focus_obj);
            g_nav.current_index++;
            if (g_nav.current_index >= g_nav.item_count) {
                panel_nav_exit_mode();
                return;
            }
            g_nav.focus_obj = g_nav.items[g_nav.current_index];
            apply_focus_style(g_nav.focus_obj);
            return;
        }
    }

    // START Key (Col 2, Key 4) -> Confirm / Increment (+1)
    if (g_nav.state == NAV_STATE_ACTIVE && col == 2 && key == 4) {
        if (lv_obj_check_type(g_nav.focus_obj, &lv_slider_class)) {
            int32_t val = lv_slider_get_value(g_nav.focus_obj) + 1;
            lv_slider_set_value(g_nav.focus_obj, val, LV_ANIM_ON);
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else if (lv_obj_check_type(g_nav.focus_obj, &lv_switch_class)) {
            if (!is_repeat) {
                if (lv_obj_has_state(g_nav.focus_obj, LV_STATE_CHECKED)) 
                    lv_obj_clear_state(g_nav.focus_obj, LV_STATE_CHECKED);
                else 
                    lv_obj_add_state(g_nav.focus_obj, LV_STATE_CHECKED);
                lv_obj_send_event(g_nav.focus_obj, LV_EVENT_VALUE_CHANGED, NULL);
            }
        } else if (!is_repeat) {
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_CLICKED, NULL);
        }
        return;
    }

    // STOP Key (Col 2, Key 5) -> Decrement (-1)
    if (g_nav.state == NAV_STATE_ACTIVE && col == 2 && key == 5) {
        if (lv_obj_check_type(g_nav.focus_obj, &lv_slider_class)) {
            int32_t val = lv_slider_get_value(g_nav.focus_obj) - 1;
            lv_slider_set_value(g_nav.focus_obj, val, LV_ANIM_ON);
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }
}
```
