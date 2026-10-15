#include "panel_nav.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "storage.h"
#include "sound.h"
#include "ui_notification.h"
#include "lang.h"
#include "day_lock.h"

enum ScreenMap {
    WARNING,
    MAIN,
    AUTOMATIC,
    MANUAL,
    SETTING,
    DIAGNOSTICS
};

// External screen transition functions defined in lvgl_function.h
extern void go_to_menu(lv_obj_t *from);
extern void go_to_automaticMode(lv_obj_t *from);
extern void go_to_manualMode(lv_obj_t *from);
extern void go_to_setting(lv_obj_t *from);
extern void go_to_diagnostics(lv_obj_t *from);

extern Storage storage;
extern ScreenMap currentScreen;
extern Lang g_lang;

static PanelNavManager g_nav = {
    .state = NAV_STATE_IDLE,
    .focus_obj = NULL,
    .items = {0},
    .item_count = 0,
    .current_index = -1,
    .last_activity_ms = 0
};

static QueueHandle_t s_nav_queue = NULL;

void panel_nav_init(void)
{
    if (s_nav_queue == NULL) {
        s_nav_queue = xQueueCreate(16, sizeof(uint16_t));
    }
    g_nav.state = NAV_STATE_IDLE;
    g_nav.focus_obj = NULL;
    g_nav.item_count = 0;
    g_nav.current_index = -1;
    g_nav.last_activity_ms = millis();
}

void panel_nav_queue_key(uint8_t col, uint8_t key)
{
    if (s_nav_queue == NULL) return;
    uint16_t ev = (uint16_t)(((uint16_t)col << 8) | (uint16_t)key);
    xQueueSend(s_nav_queue, &ev, 0);
}

bool panel_nav_is_active(void)
{
    return (g_nav.state != NAV_STATE_IDLE);
}

static bool is_actionable(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;

    // Check parent hierarchy for visibility
    lv_obj_t *parent = lv_obj_get_parent(obj);
    while (parent) {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) return false;
        parent = lv_obj_get_parent(parent);
    }

    // Never focus standalone static labels (buttons with labels are focused at button level)
    if (lv_obj_check_type(obj, &lv_label_class)) return false;

    // Standard interactive LVGL 8.3 classes
    if (lv_obj_check_type(obj, &lv_btn_class)) return true;
    if (lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class)) return true;
    if (lv_obj_check_type(obj, &lv_arc_class)) return true;
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
    if (lv_obj_check_type(obj, &lv_roller_class)) return true;
    if (lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_check_type(obj, &lv_keyboard_class)) return true;
    if (lv_obj_check_type(obj, &lv_spinbox_class)) return true;
    if (lv_obj_check_type(obj, &lv_btnmatrix_class)) return true;

    // Custom clickable objects (e.g. image buttons, clickable cards)
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        if (!lv_obj_get_parent(obj)) return false;
        lv_coord_t w = lv_obj_get_width(obj);
        lv_coord_t h = lv_obj_get_height(obj);
        if (w >= 450 && h >= 250) return false; // Exclude full-screen containers
        return true;
    }

    return false;
}

static void clear_focus_ring(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_btnmatrix_class)) {
        lv_btnmatrix_set_selected_btn(obj, LV_BTNMATRIX_BTN_NONE);
        lv_obj_clear_state(obj, LV_STATE_FOCUSED);
    }
}

static void apply_focus_ring(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_update_layout(obj);

    // Apple HIG Neon Cyan Focus Ring (3px, smooth corner offset)
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x0EA5E9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Smooth auto-scroll widget into view
    lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON);

    // Detect virtual keyboard or textarea focus
    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_textarea_class)) {
        g_nav.state = NAV_STATE_KEYBOARD;
    }
}

void panel_nav_exit_mode(void)
{
    if (g_nav.focus_obj) {
        clear_focus_ring(g_nav.focus_obj);
        g_nav.focus_obj = NULL;
    }
    g_nav.state = NAV_STATE_IDLE;
    g_nav.current_index = -1;
    g_nav.item_count = 0;
}

void panel_nav_notify_touch(void)
{
    if (g_nav.state != NAV_STATE_IDLE) {
        panel_nav_exit_mode();
    }
}

void panel_nav_on_screen_loaded(void)
{
    panel_nav_exit_mode();
}

static void collect_from_tree(lv_obj_t *parent)
{
    if (!parent || !lv_obj_is_valid(parent)) return;
    if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) return;

    uint32_t cnt = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < cnt && g_nav.item_count < 64; ++i) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (!child || !lv_obj_is_valid(child)) continue;

        if (is_actionable(child)) {
            g_nav.items[g_nav.item_count++] = child;
        }

        // Only traverse into containers, not into leaf compound widgets like keyboards
        if (!lv_obj_check_type(child, &lv_keyboard_class) && !lv_obj_check_type(child, &lv_btnmatrix_class)) {
            collect_from_tree(child);
        }
    }
}

static void scan_actionable_widgets(void)
{
    g_nav.item_count = 0;

    // 1. Modal Dialog Priority & Isolation (Quarantine background if top-layer modal exists)
    lv_obj_t *top = lv_layer_top();
    if (top) {
        uint32_t top_cnt = lv_obj_get_child_cnt(top);
        for (uint32_t i = 0; i < top_cnt; ++i) {
            lv_obj_t *c = lv_obj_get_child(top, i);
            if (c && lv_obj_is_valid(c) && !lv_obj_has_flag(c, LV_OBJ_FLAG_HIDDEN)) {
                int prev = g_nav.item_count;
                if (is_actionable(c)) {
                    g_nav.items[g_nav.item_count++] = c;
                }
                collect_from_tree(c);
                if (g_nav.item_count > prev) {
                    // Confined to active modal dialog on top layer!
                    return;
                }
            }
        }
    }

    // 2. Scan active screen
    lv_obj_t *scr = lv_scr_act();
    if (scr) {
        collect_from_tree(scr);
    }
}

static void adjust_focused_widget(int dir)
{
    lv_obj_t *obj = g_nav.focus_obj;
    if (!obj || !lv_obj_is_valid(obj)) return;

    // 1. Sliders
    if (lv_obj_check_type(obj, &lv_slider_class)) {
        int32_t min_v = lv_slider_get_min_value(obj);
        int32_t max_v = lv_slider_get_max_value(obj);
        int32_t v = lv_slider_get_value(obj) + dir;
        if (v < min_v) v = min_v;
        if (v > max_v) v = max_v;
        lv_slider_set_value(obj, v, LV_ANIM_ON);
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    // 2. Arcs
    if (lv_obj_check_type(obj, &lv_arc_class)) {
        int16_t min_v = lv_arc_get_min_value(obj);
        int16_t max_v = lv_arc_get_max_value(obj);
        int16_t v = (int16_t)(lv_arc_get_value(obj) + dir);
        if (v < min_v) v = min_v;
        if (v > max_v) v = max_v;
        lv_arc_set_value(obj, v);
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    // 3. Spinboxes
    if (lv_obj_check_type(obj, &lv_spinbox_class)) {
        if (dir > 0) {
            lv_spinbox_increment(obj);
        } else {
            lv_spinbox_decrement(obj);
        }
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    // 4. Switches
    if (lv_obj_check_type(obj, &lv_switch_class)) {
        if (dir > 0) {
            if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
                lv_obj_clear_state(obj, LV_STATE_CHECKED);
            } else {
                lv_obj_add_state(obj, LV_STATE_CHECKED);
            }
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    // 5. Dropdowns
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        uint16_t opt_cnt = lv_dropdown_get_option_cnt(obj);
        uint16_t sel = lv_dropdown_get_selected(obj);
        if (dir > 0 && (sel + 1) < opt_cnt) {
            lv_dropdown_set_selected(obj, sel + 1);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else if (dir < 0 && sel > 0) {
            lv_dropdown_set_selected(obj, sel - 1);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    // 6. Rollers
    if (lv_obj_check_type(obj, &lv_roller_class)) {
        uint16_t opt_cnt = lv_roller_get_option_cnt(obj);
        uint16_t sel = lv_roller_get_selected(obj);
        if (dir > 0 && (sel + 1) < opt_cnt) {
            lv_roller_set_selected(obj, sel + 1, LV_ANIM_ON);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else if (dir < 0 && sel > 0) {
            lv_roller_set_selected(obj, sel - 1, LV_ANIM_ON);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    // 7. Virtual Keyboard
    if (lv_obj_check_type(obj, &lv_keyboard_class)) {
        if (dir > 0) {
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else {
            lv_obj_t *ta = lv_keyboard_get_textarea(obj);
            if (ta) lv_textarea_del_char(ta);
        }
        return;
    }

    // 8. Buttons & Clickable Elements
    if (lv_obj_check_type(obj, &lv_btn_class) || lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        if (dir > 0) {
            // START confirms / clicks button
            lv_event_send(obj, LV_EVENT_CLICKED, NULL);
        }
        // STOP does NOTHING on buttons (Mandatory Siemens safety rule)
        return;
    }
}

void panel_nav_poll_tick(void)
{
    uint32_t now = millis();

    // 10-Second Inactivity Watchdog
    if (g_nav.state != NAV_STATE_IDLE) {
        if ((now - g_nav.last_activity_ms) >= 10000U) {
            panel_nav_exit_mode();
            return;
        }
    }

    uint16_t key_event = 0;
    if (s_nav_queue == NULL || xQueueReceive(s_nav_queue, &key_event, 0) != pdTRUE) {
        return;
    }

    uint8_t col = (uint8_t)((key_event >> 8) & 0xFF);
    uint8_t key = (uint8_t)(key_event & 0xFF);
    g_nav.last_activity_ms = now;

    // Direct Touchless Fallback for Day-Lock UI Modal
    if (dev_lock_ui_is_active()) {
        if (col == 2) {
            if (key == 3) { // MODE -> Cycle keypad buttons
                dev_lock_ui_process_key(4);
                sound_play(SOUND_CLICK);
                return;
            } else if (key == 4) { // START -> Press focused button
                dev_lock_ui_process_key(5);
                sound_play(SOUND_CLICK);
                return;
            } else if (key == 5) { // STOP -> DEL
                dev_lock_ui_process_key(6);
                sound_play(SOUND_CLICK);
                return;
            }
        }
        return; // Prevent background navigation while day lock is modal
    }

    // --- ROW 1 (Col 1: F1..F5 Direct Navigation Jumps) ---
    if (col == 1) {
        panel_nav_exit_mode();
        switch (key) {
            case 1: go_to_menu(NULL); break;
            case 2: go_to_automaticMode(NULL); break;
            case 3: go_to_manualMode(NULL); break;
            case 4: go_to_diagnostics(NULL); break;
            case 5: go_to_setting(NULL); break;
            default: break;
        }
        return;
    }

    // --- ROW 2 (Col 2: M1..M5) ---
    if (col == 2) {
        // M1: SAVE
        if (key == 1) {
            storage.save();
            sound_play(SOUND_PORTION_DONE);
            ui_show_notification(NOTIF_SUCCESS,
                (g_lang == LANG_FA) ? "ذخیره تنظیمات" : "Settings Saved",
                (g_lang == LANG_FA) ? "تنظیمات با موفقیت در حافظه ذخیره شد" : "Settings saved to flash successfully");
            return;
        }

        // M2: CANCEL
        if (key == 2) {
            if (g_nav.state == NAV_STATE_KEYBOARD) {
                g_nav.state = NAV_STATE_ACTIVE;
                return;
            }
            if (g_nav.state == NAV_STATE_ACTIVE) {
                panel_nav_exit_mode();
                return;
            }
            // If in IDLE and on sub-screen, return to Main Menu
            if (currentScreen != MAIN && currentScreen != WARNING) {
                go_to_menu(NULL);
                return;
            }
            return;
        }

        // M3: MODE
        if (key == 3) {
            if (g_nav.state == NAV_STATE_IDLE) {
                scan_actionable_widgets();
                if (g_nav.item_count == 0) return;
                g_nav.current_index = 0;
                g_nav.state = NAV_STATE_ACTIVE;
                g_nav.focus_obj = g_nav.items[0];
                apply_focus_ring(g_nav.focus_obj);
                sound_play(SOUND_CLICK);
                return;
            }

            if (g_nav.state == NAV_STATE_ACTIVE) {
                if (g_nav.focus_obj) {
                    clear_focus_ring(g_nav.focus_obj);
                }
                g_nav.current_index++;

                // End-of-cycle wrap-around rule: Dismiss and return to IDLE
                if (g_nav.current_index >= g_nav.item_count) {
                    panel_nav_exit_mode();
                    return;
                }

                g_nav.focus_obj = g_nav.items[g_nav.current_index];
                apply_focus_ring(g_nav.focus_obj);
                sound_play(SOUND_CLICK);
                return;
            }

            if (g_nav.state == NAV_STATE_KEYBOARD) {
                if (g_nav.focus_obj) {
                    uint32_t right_key = LV_KEY_RIGHT;
                    lv_event_send(g_nav.focus_obj, LV_EVENT_KEY, &right_key);
                }
                return;
            }
            return;
        }

        // M4: START
        if (key == 4) {
            if (g_nav.state != NAV_STATE_IDLE) {
                adjust_focused_widget(+1);
            }
            return;
        }

        // M5: STOP
        if (key == 5) {
            if (g_nav.state != NAV_STATE_IDLE) {
                adjust_focused_widget(-1);
            }
            return;
        }
    }
}
