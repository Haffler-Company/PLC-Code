#include "panel_nav.h"
#include "ui.h"
#include "screens.h"
#include "ui_toast.h"
#include "actions.h"
#include "variables.h"
#include <Arduino.h>
#include <string.h>

static PanelNavManager g_nav = {};
static volatile uint16_t g_pending_key_event = 0;

void panel_nav_init(void) {
    memset(&g_nav, 0, sizeof(g_nav));
    g_nav.state = NAV_STATE_IDLE;
    g_nav.current_index = -1;
    g_nav.current_screen_id = -1;
    g_nav.last_activity_ms = millis();
    g_pending_key_event = 0;
}

void panel_nav_queue_key(uint8_t row, uint8_t key) {
    g_pending_key_event = (uint16_t)((row << 8) | key);
}

bool panel_nav_is_active(void) {
    return (g_nav.state != NAV_STATE_IDLE);
}

PanelNavState panel_nav_get_state(void) {
    return g_nav.state;
}

static void clear_focus_ring(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_buttonmatrix_class)) {
        lv_buttonmatrix_set_selected_button(obj, LV_BUTTONMATRIX_BUTTON_NONE);
        lv_obj_clear_state(obj, LV_STATE_FOCUSED);
    }
}

static void apply_focus_ring(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    lv_obj_update_layout(obj);

    /* Apple HIG Neon Cyan Focus Ring (3px, pad 2px, zero layout drift) */
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x0EA5E9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Smooth auto-scroll into viewport if scrollable container */
    lv_obj_scroll_to_view_recursive(obj, LV_ANIM_ON);

    if (lv_obj_check_type(obj, &lv_keyboard_class)) {
        g_nav.state = NAV_STATE_KEYBOARD;
        lv_buttonmatrix_set_selected_button(obj, 0);
        lv_obj_add_state(obj, LV_STATE_FOCUSED);
    } else {
        g_nav.state = NAV_STATE_ACTIVE;
    }
}

void panel_nav_exit_mode(void) {
    if (g_nav.focus_obj) {
        clear_focus_ring(g_nav.focus_obj);
        g_nav.focus_obj = NULL;
    }
    g_nav.state = NAV_STATE_IDLE;
    g_nav.current_index = -1;
    g_nav.item_count = 0;
}

void panel_nav_notify_touch(void) {
    /* Touchscreen Override: Touch digitizer instantly dismisses focus cursor */
    if (g_nav.state != NAV_STATE_IDLE) {
        panel_nav_exit_mode();
    }
}

static bool has_interactive_descendants(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return false;
    uint32_t c_cnt = lv_obj_get_child_count(obj);
    for (uint32_t ci = 0; ci < c_cnt; ci++) {
        lv_obj_t *ch = lv_obj_get_child(obj, ci);
        if (!ch || !lv_obj_is_valid(ch)) continue;
        if (lv_obj_check_type(ch, &lv_button_class) ||
            lv_obj_check_type(ch, &lv_textarea_class) ||
            lv_obj_check_type(ch, &lv_slider_class) ||
            lv_obj_check_type(ch, &lv_switch_class) ||
            lv_obj_check_type(ch, &lv_dropdown_class) ||
            lv_obj_check_type(ch, &lv_roller_class) ||
            lv_obj_check_type(ch, &lv_arc_class)) {
            return true;
        }
        if (has_interactive_descendants(ch)) return true;
    }
    return false;
}

static bool is_leaf_actionable(lv_obj_t *obj) {
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;

    /* Skip active toast container and all its elements */
    if (ui_toast_is_container(obj)) return false;

    /* Check parent hierarchy for hidden state or toast container */
    lv_obj_t *parent = lv_obj_get_parent(obj);
    while (parent) {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) return false;
        if (ui_toast_is_container(parent)) return false;
        parent = lv_obj_get_parent(parent);
    }

    /* Screen roots, header backgrounds, and top layers are never actionable */
    if (obj == lv_screen_active() || obj == lv_layer_top() || obj == lv_layer_sys()) return false;
    if (obj == objects.ib_setting) return false;

    /* Exclude large layout containers and microscopic dots */
    int32_t w = lv_obj_get_width(obj);
    int32_t h = lv_obj_get_height(obj);
    if (w >= 390 || h >= 220 || w <= 8 || h <= 8) return false;

    /* Standard actionable widget types */
    if (lv_obj_check_type(obj, &lv_button_class)) return true;
    if (lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class)) return true;
    if (lv_obj_check_type(obj, &lv_arc_class)) return true;
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
    if (lv_obj_check_type(obj, &lv_roller_class)) return true;
    if (lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_check_type(obj, &lv_keyboard_class)) return true;

    /* Clickable leaf elements (e.g. icon boxes) */
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        if (w >= 180 || h >= 90) return false;
        /* If it has interactive descendants, it is a container, not a leaf */
        if (has_interactive_descendants(obj)) return false;
        return true;
    }

    return false;
}

static lv_obj_t *get_active_modal(void) {
    lv_obj_t *top = lv_layer_top();
    if (top) {
        uint32_t cnt = lv_obj_get_child_count(top);
        for (int32_t i = (int32_t)cnt - 1; i >= 0; --i) {
            lv_obj_t *child = lv_obj_get_child(top, (uint32_t)i);
            if (!child || !lv_obj_is_valid(child)) continue;
            if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
            if (ui_toast_is_container(child)) continue;
            return child;
        }
    }
    if (objects.key_board_panle && !lv_obj_has_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN)) {
        return objects.key_board_panle;
    }
    if (objects.key_board_panle_setting && !lv_obj_has_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN)) {
        return objects.key_board_panle_setting;
    }
    return NULL;
}

static void collect_actionable_widgets(lv_obj_t *parent) {
    if (!parent || !lv_obj_is_valid(parent)) return;
    if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) return;
    if (ui_toast_is_container(parent)) return;

    uint32_t cnt = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < cnt && g_nav.item_count < 64; ++i) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (!child || !lv_obj_is_valid(child)) continue;
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;

        if (is_leaf_actionable(child)) {
            g_nav.items[g_nav.item_count++] = child;
        }
        /* Always recurse into sub-containers unless it is a closed atomic widget */
        if (!lv_obj_check_type(child, &lv_keyboard_class) &&
            !lv_obj_check_type(child, &lv_roller_class) &&
            !lv_obj_check_type(child, &lv_dropdown_class) &&
            !lv_obj_check_type(child, &lv_textarea_class)) {
            collect_actionable_widgets(child);
        }
    }
}

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

    if (lv_obj_check_type(obj, &lv_arc_class)) {
        int32_t v = lv_arc_get_value(obj) + dir;
        if (v < lv_arc_get_min_value(obj)) v = lv_arc_get_min_value(obj);
        if (v > lv_arc_get_max_value(obj)) v = lv_arc_get_max_value(obj);
        lv_arc_set_value(obj, v);
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    if (lv_obj_check_type(obj, &lv_switch_class)) {
        if (dir > 0) {
            if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
                lv_obj_clear_state(obj, LV_STATE_CHECKED);
            } else {
                lv_obj_add_state(obj, LV_STATE_CHECKED);
            }
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        uint32_t sel = lv_dropdown_get_selected(obj);
        uint32_t cnt = lv_dropdown_get_option_count(obj);
        if (dir > 0 && sel + 1 < cnt) {
            lv_dropdown_set_selected(obj, sel + 1);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else if (dir < 0 && sel > 0) {
            lv_dropdown_set_selected(obj, sel - 1);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    if (lv_obj_check_type(obj, &lv_roller_class)) {
        uint32_t sel = lv_roller_get_selected(obj);
        uint32_t cnt = lv_roller_get_option_count(obj);
        if (dir > 0 && sel + 1 < cnt) {
            lv_roller_set_selected(obj, sel + 1, LV_ANIM_ON);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else if (dir < 0 && sel > 0) {
            lv_roller_set_selected(obj, sel - 1, LV_ANIM_ON);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    if (lv_obj_check_type(obj, &lv_keyboard_class)) {
        if (dir > 0) {
            uint32_t sel = lv_buttonmatrix_get_selected_button(obj);
            const char *txt = lv_buttonmatrix_get_button_text(obj, sel);
            if (txt) {
                if (!strcmp(txt, LV_SYMBOL_OK) || !strcmp(txt, "OK")) {
                    lv_obj_send_event(obj, LV_EVENT_READY, NULL);
                    if (objects.key_board_panle && !lv_obj_has_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_add_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (objects.key_board_panle_setting && !lv_obj_has_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
                    }
                    clear_focus_ring(g_nav.focus_obj);
                    g_nav.focus_obj = NULL;
                    g_nav.state = NAV_STATE_ACTIVE;
                } else if (!strcmp(txt, LV_SYMBOL_BACKSPACE) || !strcmp(txt, "BS")) {
                    lv_obj_t *ta = lv_keyboard_get_textarea(obj);
                    if (ta) lv_textarea_delete_char(ta);
                } else {
                    lv_obj_t *ta = lv_keyboard_get_textarea(obj);
                    if (ta) lv_textarea_add_text(ta, txt);
                }
            }
        } else {
            lv_obj_t *ta = lv_keyboard_get_textarea(obj);
            if (ta) lv_textarea_delete_char(ta);
        }
        return;
    }

    if (lv_obj_check_type(obj, &lv_textarea_class) || lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        if (dir > 0) {
            lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL);
            lv_obj_send_event(obj, LV_EVENT_FOCUSED, NULL);
        }
        return;
    }
}

void panel_nav_auto_repeat(int dir) {
    if (g_nav.state != NAV_STATE_ACTIVE || !g_nav.focus_obj) return;
    lv_obj_t *obj = g_nav.focus_obj;

    /* Continuous hold auto-repeat applies strictly to numeric/stepper widgets */
    if (lv_obj_check_type(obj, &lv_slider_class) ||
        lv_obj_check_type(obj, &lv_arc_class) ||
        lv_obj_check_type(obj, &lv_dropdown_class) ||
        lv_obj_check_type(obj, &lv_roller_class)) {
        adjust_focused_widget(dir);
        g_nav.last_activity_ms = millis();
    }
}

void panel_nav_poll_tick(void) {
    uint32_t now = millis();

    /* Screen transition purge: if active screen changed, exit navigation cleanly */
    int scr_id = ui_current_screen_id();
    if (g_nav.current_screen_id != scr_id) {
        if (g_nav.state != NAV_STATE_IDLE) {
            panel_nav_exit_mode();
        }
        g_nav.current_screen_id = scr_id;
    }

    /* 15-Second Inactivity Auto-Dismissal */
    if (g_nav.state != NAV_STATE_IDLE) {
        if ((now - g_nav.last_activity_ms) >= 15000U) {
            panel_nav_exit_mode();
            return;
        }
    }

    if (g_pending_key_event == 0) return;

    uint8_t row = (g_pending_key_event >> 8) & 0xFF;
    uint8_t key = g_pending_key_event & 0xFF;
    g_pending_key_event = 0;
    g_nav.last_activity_ms = now;

    /* SAVE Key (Row 2, Key 1): Save settings or confirm current input */
    if (row == 2 && key == 1) {
        if (g_nav.state == NAV_STATE_KEYBOARD) {
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_READY, NULL);
            if (objects.key_board_panle && !lv_obj_has_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
            }
            if (objects.key_board_panle_setting && !lv_obj_has_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
            }
            clear_focus_ring(g_nav.focus_obj);
            g_nav.focus_obj = NULL;
            g_nav.state = NAV_STATE_ACTIVE;
            return;
        }
        if (g_nav.state == NAV_STATE_ACTIVE && g_nav.focus_obj) {
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_CLICKED, NULL);
            return;
        }
        action_save_button_pressed(NULL);
        ui_toast_show("ذخیره تنظیمات", "پارامترها در حافظه پایدار ثبت گردید", 2500, TOAST_TYPE_SUCCESS);
        return;
    }

    /* CANCEL Key (Row 2, Key 2): Close keyboard, modal, or exit navigation */
    if (row == 2 && key == 2) {
        if (g_nav.state == NAV_STATE_KEYBOARD) {
            if (objects.key_board_panle && !lv_obj_has_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
            }
            if (objects.key_board_panle_setting && !lv_obj_has_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
            }
            clear_focus_ring(g_nav.focus_obj);
            g_nav.focus_obj = NULL;
            g_nav.state = NAV_STATE_ACTIVE;
            return;
        }
        panel_nav_exit_mode();
        return;
    }

    /* MODE Key (Row 2, Key 3): Focus Traversal Forward across all widgets */
    if (row == 2 && key == 3) {
        if (g_nav.state == NAV_STATE_KEYBOARD) {
            uint32_t sel = lv_buttonmatrix_get_selected_button(g_nav.focus_obj);
            uint32_t next = (sel == LV_BUTTONMATRIX_BUTTON_NONE) ? 0 : (sel + 1);
            if (next >= 12) next = 0;
            lv_buttonmatrix_set_selected_button(g_nav.focus_obj, next);
            return;
        }

        if (g_nav.state == NAV_STATE_IDLE) {
            g_nav.item_count = 0;
            lv_obj_t *modal = get_active_modal();
            if (modal) {
                collect_actionable_widgets(modal);
            } else {
                collect_actionable_widgets(lv_screen_active());
            }

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

            /* Continuous screen traversal wrap-around */
            if (g_nav.current_index >= g_nav.item_count) {
                g_nav.current_index = 0;
            }

            g_nav.focus_obj = g_nav.items[g_nav.current_index];
            apply_focus_ring(g_nav.focus_obj);
            return;
        }
    }

    /* START Key (Row 2, Key 4): Activate / Click / Toggle / Increment (+1) */
    if (g_nav.state != NAV_STATE_IDLE && row == 2 && key == 4) {
        adjust_focused_widget(+1);
        return;
    }

    /* STOP Key (Row 2, Key 5): Decrement / Backspace / Previous Widget */
    if (g_nav.state != NAV_STATE_IDLE && row == 2 && key == 5) {
        if (g_nav.state == NAV_STATE_KEYBOARD) {
            adjust_focused_widget(-1);
            return;
        }

        lv_obj_t *cur = g_nav.focus_obj;
        if (cur && (lv_obj_check_type(cur, &lv_slider_class) ||
                    lv_obj_check_type(cur, &lv_arc_class) ||
                    lv_obj_check_type(cur, &lv_dropdown_class) ||
                    lv_obj_check_type(cur, &lv_roller_class))) {
            adjust_focused_widget(-1);
            return;
        }

        /* On buttons/inputs, STOP acts as reverse navigation (Previous Widget) */
        if (g_nav.item_count > 1) {
            clear_focus_ring(g_nav.focus_obj);
            if (g_nav.current_index <= 0) {
                g_nav.current_index = g_nav.item_count - 1;
            } else {
                g_nav.current_index--;
            }
            g_nav.focus_obj = g_nav.items[g_nav.current_index];
            apply_focus_ring(g_nav.focus_obj);
        }
        return;
    }
}
