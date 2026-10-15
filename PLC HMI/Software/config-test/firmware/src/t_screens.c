#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t_screens.h"
#include "t_images.h"
#include "t_fonts.h"
#include "t_actions.h"
#include "t_vars.h"
#include "t_styles.h"
#include "t_ui.h"
#include "fonts.h"
#include "drivers/keyboard_style.h"

// زدن روی «OUTPUT»: یک‌بار همه‌ی خروجی‌ها روشن + متن بولد، بارِ بعد همه خاموش + متن نازک
static void event_handler_output_all_toggle(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    static bool all_on = false;
    all_on = !all_on;
    bool v = all_on;
    set_var_q1(v); set_var_q2(v); set_var_q3(v); set_var_q4(v); set_var_q5(v);
    set_var_q6(v); set_var_q7(v); set_var_q8(v); set_var_q9(v); set_var_q10(v);
    lv_obj_t *lbl = lv_event_get_target(e);
    lv_obj_set_style_text_font(lbl, all_on ? &ui_font_kalameh_bold_18 : &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void hwtest_start_auto_qc(void);
extern void action_btn_back_to_password(lv_event_t *e);
static void action_auto_qc_click(lv_event_t *e) {
    hwtest_start_auto_qc();
}

#include <string.h>

t_objects_t t_objects;
lv_obj_t *t_tick_value_change_obj;
uint32_t t_active_theme_index = 0;

static lv_obj_t *ssr_on_ta;
static lv_obj_t *ssr_off_ta;
static lv_obj_t *rtc_year_ta;
static lv_obj_t *rtc_month_ta;
static lv_obj_t *rtc_day_ta;
static lv_obj_t *rtc_hour_ta;
static lv_obj_t *rtc_min_ta;
static bool rtc_editing = false;   // در حالِ ویرایش → آپدیتِ خودکارِ فیلدها از ساعت متوقف شود
static lv_obj_t *wifi_test_dd;
static lv_obj_t *wifi_test_pass_ta;
static lv_obj_t *wifi_test_kb;
static lv_obj_t *ble_test_dd = NULL;
extern lv_obj_t *s_touch_back;

lv_obj_t *t_get_wifi_test_dropdown(void) { return wifi_test_dd; }
lv_obj_t *t_get_wifi_test_password(void) { return wifi_test_pass_ta; }
lv_obj_t *t_get_ble_test_dropdown(void) { return ble_test_dd; }

static const char *wifi_kb_map[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "ABC", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "@", "\n",
    "?#@", "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

static const char *wifi_kb_map_upper[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "abc", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "@", "\n",
    "?#@", "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

static const char *wifi_kb_map_sym[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "abc", "@", "#", "$", "%", "&", "-", "+", "(", ")", "\n",
    "*", "\"", "'", ":", ";", "/", "!", "?", "_", "=", "\n",
    "abc", "<", ">", "[", "]", "{", "}", ".", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t wifi_kb_ctrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2, 2
};

static void wifi_pass_focus_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if ((code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) &&
        wifi_test_kb && wifi_test_pass_ta) {
        lv_obj_clear_flag(wifi_test_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(wifi_test_kb);
    }
}

static void wifi_kb_button_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !wifi_test_pass_ta) return;
    lv_obj_t *kb = lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(kb);
    const char *txt = lv_buttonmatrix_get_button_text(kb, id);
    if (!txt) return;
    if (strcmp(txt, "ABC") == 0) {
        lv_buttonmatrix_set_map(kb, wifi_kb_map_upper);
        lv_buttonmatrix_set_ctrl_map(kb, wifi_kb_ctrl);
    } else if (strcmp(txt, "abc") == 0) {
        lv_buttonmatrix_set_map(kb, wifi_kb_map);
        lv_buttonmatrix_set_ctrl_map(kb, wifi_kb_ctrl);
    } else if (strcmp(txt, "?#@") == 0) {
        lv_buttonmatrix_set_map(kb, wifi_kb_map_sym);
        lv_buttonmatrix_set_ctrl_map(kb, wifi_kb_ctrl);
    } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_delete_char(wifi_test_pass_ta);
    } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        action_test_wifi_connect(NULL);
    } else {
        lv_textarea_add_text(wifi_test_pass_ta, txt);
    }
}

static void wifi_pass_eye_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    bool show = (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
        code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
    if (wifi_test_pass_ta) lv_textarea_set_password_mode(wifi_test_pass_ta, !show);
    lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
    if (lbl) lv_label_set_text(lbl, show ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
}

static void wifi_double_tap_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    static uint32_t last_click = 0;
    uint32_t now = lv_tick_get();
    if (now - last_click < 300) {
        action_switchtomain(e);
    }
    last_click = now;
}

static void rs485_double_tap_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    static uint32_t last_click = 0;
    uint32_t now = lv_tick_get();
    if (now - last_click < 300) {
        action_switchtomain(e);
    }
    last_click = now;
}

static void gpio_double_tap_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;
    static uint32_t last_click = 0;
    uint32_t now = lv_tick_get();
    if (now - last_click < 300) {
        action_switchtomain(e);
    }
    last_click = now;
}

static void gpio_attach_double_tap_recursive(lv_obj_t *obj) {
    if (!obj) return;
    lv_obj_add_event_cb(obj, gpio_double_tap_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    uint32_t child_count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < child_count; i++) {
        gpio_attach_double_tap_recursive(lv_obj_get_child(obj, i));
    }
}

static void bluetooth_double_tap_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    static uint32_t last_click = 0;
    uint32_t now = lv_tick_get();
    if (now - last_click < 300) {
        action_switchtomain(e);
    }
    last_click = now;
}

static void ssr_double_tap_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    static uint32_t last_click = 0;
    uint32_t now = lv_tick_get();
    if (now - last_click < 300) {
        action_switchtomain(e);
    }
    last_click = now;
}

static void add_test_top_back_button(lv_obj_t *parent) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, 8, 7);
    lv_obj_set_size(btn, 82, 34);
    lv_obj_add_event_cb(btn, action_switchtomain, LV_EVENT_PRESSED, (void *)0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff991b1b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff7f1d1d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xfffca5a5), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl);
    lv_obj_move_foreground(btn);
}

static void wifi_kb_done_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(lv_event_get_target(e), LV_OBJ_FLAG_HIDDEN);
    }
}

static int ta_int(lv_obj_t *ta, int fallback, int min_v, int max_v) {
    if (!ta) return fallback;
    const char *txt = lv_textarea_get_text(ta);
    int v = (txt && txt[0]) ? atoi(txt) : fallback;
    if (v < min_v) v = min_v;
    if (v > max_v) v = max_v;
    return v;
}

static int s_last_valid_ssr_on = 800;
static int s_last_valid_ssr_off = 200;
static lv_obj_t *s_ssr_lamp_lbl = NULL;

static bool s_in_ssr_field_update = false;

static void ssr_validate_field(lv_obj_t *ta, int *last_valid) {
    if (!ta) return;
    const char *txt = lv_textarea_get_text(ta);
    int val = 0;
    bool valid = false;
    if (txt && txt[0] != '\0') {
        char *endp = NULL;
        long v = strtol(txt, &endp, 10);
        if (endp != txt && *endp == '\0' && v > 0 && v <= 30000) {
            val = (int)v;
            valid = true;
        }
    }
    if (valid) {
        *last_valid = val;
    } else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", *last_valid);
        s_in_ssr_field_update = true;
        lv_textarea_set_text(ta, buf);
        s_in_ssr_field_update = false;
    }
}

void ssr_validate_and_commit(void) {
    if (ssr_on_ta) ssr_validate_field(ssr_on_ta, &s_last_valid_ssr_on);
    if (ssr_off_ta) ssr_validate_field(ssr_off_ta, &s_last_valid_ssr_off);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d.%d", s_last_valid_ssr_on, s_last_valid_ssr_off);
    set_var_time_text(buf);
}

static void ssr_commit_time_text(void) {
    ssr_validate_and_commit();
}

static void rtc_commit_text(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d.%02d.%02d.%02d.%02d",
             ta_int(rtc_year_ta, 2026, 2020, 2099),
             ta_int(rtc_month_ta, 1, 1, 12),
             ta_int(rtc_day_ta, 1, 1, 31),
             ta_int(rtc_hour_ta, 0, 0, 23),
             ta_int(rtc_min_ta, 0, 0, 59));
    set_var_rtc_text(buf);
}

// پرچم و متغیر طول برای رفتار هوشمند کلیدهای عددی و پاک‌کردن کادرهای SSR
static bool s_ssr_field_fresh = false;
static int s_ssr_field_initial_len = 0;

static void ssr_field_event_cb(lv_event_t *e) {
    if (s_in_ssr_field_update) return;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        if (t_objects.tobj89 && lv_obj_is_valid(t_objects.tobj89))
            lv_keyboard_set_textarea(t_objects.tobj89, ta);
        
        // نمایش کادر فعال با کادر آبی برجسته
        if (ssr_on_ta && lv_obj_is_valid(ssr_on_ta)) {
            bool act = (ta == ssr_on_ta);
            lv_obj_set_style_border_color(ssr_on_ta, act ? lv_color_hex(0xff0284c7) : lv_color_hex(0xff94a3b8), 0);
            lv_obj_set_style_border_width(ssr_on_ta, act ? 2 : 1, 0);
        }
        if (ssr_off_ta && lv_obj_is_valid(ssr_off_ta)) {
            bool act = (ta == ssr_off_ta);
            lv_obj_set_style_border_color(ssr_off_ta, act ? lv_color_hex(0xff0284c7) : lv_color_hex(0xff94a3b8), 0);
            lv_obj_set_style_border_width(ssr_off_ta, act ? 2 : 1, 0);
        }

        s_ssr_field_fresh = true;  // کادر تازه فوکوس گرفت
        const char *t = lv_textarea_get_text(ta);
        s_ssr_field_initial_len = t ? (int)strlen(t) : 0;
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        if (s_ssr_field_fresh) {
            s_ssr_field_fresh = false;
            const char *txt = lv_textarea_get_text(ta);
            int new_len = txt ? (int)strlen(txt) : 0;
            s_in_ssr_field_update = true;
            if (new_len < s_ssr_field_initial_len) {
                // کلیک Backspace: پاک‌کردن کامل مقدار
                lv_textarea_set_text(ta, "");
                s_ssr_field_initial_len = 0;
            } else if (new_len > s_ssr_field_initial_len) {
                // تایپ رقم جدید: جایگزین کردن مقدار قبلی با رقم تازه
                char last = txt[new_len - 1];
                if (last >= '0' && last <= '9') {
                    char buf[2] = { last, '\0' };
                    lv_textarea_set_text(ta, buf);
                    s_ssr_field_initial_len = 1;
                }
            }
            s_in_ssr_field_update = false;
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        s_ssr_field_fresh = false;
        ssr_validate_and_commit();
    }
}


static void rtc_field_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        rtc_editing = true;   // کاربر شروع به ویرایش کرد → آپدیتِ خودکار متوقف
        if (t_objects.tobj81) lv_keyboard_set_textarea(t_objects.tobj81, lv_event_get_target(e));
    } else if (code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_DEFOCUSED) {
        rtc_commit_text();
    }
}

static void rtc_apply_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;
    rtc_commit_text();
    action_callrtc(e);
    rtc_editing = false;   // سیو شد → آپدیتِ خودکار دوباره از سر گرفته شود
}

void rtc_cycle_focus(void) {
    lv_obj_t *fields[5] = { rtc_year_ta, rtc_month_ta, rtc_day_ta, rtc_hour_ta, rtc_min_ta };
    lv_obj_t *cur = (t_objects.tobj81 && lv_obj_is_valid(t_objects.tobj81)) ? lv_keyboard_get_textarea(t_objects.tobj81) : NULL;
    int cur_idx = -1;
    for (int i = 0; i < 5; i++) {
        if (fields[i] && fields[i] == cur) {
            cur_idx = i;
            break;
        }
    }
    int next_idx = (cur_idx + 1) % 5;
    lv_obj_t *target = fields[next_idx];
    if (target && lv_obj_is_valid(target)) {
        if (cur && lv_obj_is_valid(cur)) {
            lv_obj_clear_state(cur, LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(cur, lv_color_hex(0xff94a3b8), 0);
            lv_obj_set_style_border_width(cur, 1, 0);
        }
        lv_obj_add_state(target, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(target, lv_color_hex(0xff0284c7), 0);
        lv_obj_set_style_border_width(target, 2, 0);
        if (t_objects.tobj81 && lv_obj_is_valid(t_objects.tobj81)) {
            lv_keyboard_set_textarea(t_objects.tobj81, target);
        }
        rtc_editing = true;
    }
}

// نقشه‌ی کیبردِ عددیِ تنظیم ساعت: فقط عدد + پاک‌کردن (Backspace) + تایید (OK) — بدون نقطه و بدون کلیدِ تعویضِ کیبرد
static const char *rtc_kb_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t rtc_kb_ctrl[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// زدنِ تایید (OK) روی کیبرد = اعمالِ ساعت روی RTC
static void rtc_kb_ready_cb(lv_event_t *e) {
    rtc_commit_text();
    action_callrtc(e);
    rtc_editing = false;   // سیو شد → آپدیتِ خودکار دوباره از سر گرفته شود
}

// نقشه‌ی کیبردِ عددیِ SSR: بدون علامت‌های اضافی و بدون تایید، ردیف آخر فقط Backspace (آبی) و 0 دوبرابر
static const char *ssr_kb_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", ""
};
static const lv_buttonmatrix_ctrl_t ssr_kb_ctrl[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
    1, 2
};

static void ssr_kb_ready_cb(lv_event_t *e) {
    ssr_validate_and_commit();
}

static void ssr_keyboard_action_keys_draw_cb(lv_event_t *e) {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    if (!draw_task) return;

    lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
    if (!base_dsc) return;

    if (base_dsc->part == LV_PART_ITEMS) {
        uint32_t btn_id = base_dsc->id1;
        const char *txt = lv_buttonmatrix_get_button_text(obj, btn_id);
        if (!txt) return;

        bool is_backspace = (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "DEL") == 0 || strcmp(txt, "⌫") == 0 ||
                             strcmp(txt, "Clear") == 0 || strcmp(txt, "BS") == 0);

        if (is_backspace) {
            bool pressed = (lv_buttonmatrix_get_selected_button(obj) == btn_id && lv_obj_has_state(obj, LV_STATE_PRESSED));

            lv_draw_fill_dsc_t *fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_dsc) {
                fill_dsc->color = pressed ? lv_color_hex(0xff1d4ed8) : lv_color_hex(0xff2563eb); // Apple Blue
            }
            lv_draw_border_dsc_t *border_dsc = lv_draw_task_get_border_dsc(draw_task);
            if (border_dsc) {
                border_dsc->color = pressed ? lv_color_hex(0xff1e40af) : lv_color_hex(0xff2563eb);
            }
            lv_draw_label_dsc_t *label_dsc = lv_draw_task_get_label_dsc(draw_task);
            if (label_dsc) {
                label_dsc->color = lv_color_white();
            }
        }
    }
}

static void style_ssr_keyboard_action_keys(lv_obj_t *kb) {
    if (!kb) return;
    lv_obj_add_event_cb(kb, ssr_keyboard_action_keys_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
}

static void event_handler_cb_main_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj2(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj4(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj5(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj6(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj7(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj8(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_main_obj9(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

extern void hwtest_save_buzzer_volume(int32_t val);
extern void hwtest_beep_click(uint32_t ms);
extern void hwtest_buzzer_silence(void);

void update_buzzer_label_text(int32_t val) {
    if (!t_objects.tobj42 || !lv_obj_is_valid(t_objects.tobj42)) return;
    char bbuf[32];
    if (val <= 0) {
        snprintf(bbuf, sizeof(bbuf), "%s  Buzzer: MUTE", LV_SYMBOL_MUTE);
        lv_obj_set_style_text_color(t_objects.tobj42, lv_color_hex(0xffef4444), 0);
    } else {
        int pct = (val * 100) / 255;
        snprintf(bbuf, sizeof(bbuf), "%s  Buzzer: %d%%", (pct > 50) ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_VOLUME_MID, pct);
        lv_obj_set_style_text_color(t_objects.tobj42, lv_color_hex(0xff334155), 0);
    }
    lv_label_set_text(t_objects.tobj42, bbuf);
}

static int32_t s_last_unmuted_buzzer = 180;

static void event_handler_cb_main_obj42(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int32_t cur = get_var_buzzer();
    if (cur > 0) {
        s_last_unmuted_buzzer = cur;
        set_var_buzzer(0);
        hwtest_save_buzzer_volume(0);
        hwtest_buzzer_silence();
        if (t_objects.tobj41 && lv_obj_is_valid(t_objects.tobj41)) {
            lv_slider_set_value(t_objects.tobj41, 0, LV_ANIM_ON);
        }
        update_buzzer_label_text(0);
    } else {
        int32_t restore = (s_last_unmuted_buzzer > 20) ? s_last_unmuted_buzzer : 180;
        set_var_buzzer(restore);
        hwtest_save_buzzer_volume(restore);
        if (t_objects.tobj41 && lv_obj_is_valid(t_objects.tobj41)) {
            lv_slider_set_value(t_objects.tobj41, restore, LV_ANIM_ON);
        }
        update_buzzer_label_text(restore);
        hwtest_beep_click(30);
    }
}

static void event_handler_cb_main_obj41(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *slider = lv_event_get_target(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        if (t_tick_value_change_obj != slider) {
            int32_t val = lv_slider_get_value(slider);
            set_var_buzzer(val);
            update_buzzer_label_text(val);
            if (val <= 0) {
                hwtest_buzzer_silence();
            } else {
                s_last_unmuted_buzzer = val;
            }
        }
    } else if (event == LV_EVENT_RELEASED || event == LV_EVENT_PRESS_LOST) {
        int32_t val = lv_slider_get_value(slider);
        set_var_buzzer(val);
        hwtest_save_buzzer_volume(val);
        update_buzzer_label_text(val);
        if (val > 0) {
            s_last_unmuted_buzzer = val;
            hwtest_beep_click(30);
        } else {
            hwtest_buzzer_silence();
        }
    }
}

static void event_handler_cb_setup_obj10(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj11(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj12(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj13(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj14(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj15(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj16(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj17(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
}

static void event_handler_cb_setup_obj18(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q1(value);
        }
    }
}

static void event_handler_cb_setup_obj19(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q3(value);
        }
    }
}

static void event_handler_cb_setup_obj20(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q2(value);
        }
    }
}

static void event_handler_cb_setup_obj21(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q8(value);
        }
    }
}

static void event_handler_cb_setup_obj22(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q7(value);
        }
    }
}

static void event_handler_cb_setup_obj23(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q6(value);
        }
    }
}

static void event_handler_cb_setup_obj24(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q5(value);
        }
    }
}

static void event_handler_cb_setup_obj25(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q4(value);
        }
    }
}

static void event_handler_cb_setup_obj26(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q10(value); // کلید Q9 (موقعیت پایین راست، متصل به SSR2 / GPIO38)
        }
    }
}

static void event_handler_cb_setup_obj27(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_q9(value); // کلید Q8 (موقعیت ردیف ۴ راست، متصل به Relay Q8 / GPIO41)
        }
    }
}

static void event_handler_cb_rs485_rssend(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_rstextsend(value);
        }
    }
}

static void event_handler_cb_gsm_obj28(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_send_message(value);
        }
    }
}

static void event_handler_cb_gsm_obj29(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_receive_message(value);
        }
    }
}

static void event_handler_cb_bluetooth_bluetooth(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            set_var_enableble(value);
        }
    }
}

static void event_handler_cb_clock_setup_ta_rtc(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_rtc_text(value);
        }
    }
}

static void event_handler_cb_ssr_pwm_time_text_tarea(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (t_tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_time_text(value);
        }
    }
}

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;

        // =========================================================================
        // ستون سمت چپ: دکمه‌های ناوبری ۸ تایی متقارن (x=10, w=108, h=32, فاصله ۶px)
        // =========================================================================
        const struct {
            lv_obj_t **target;
            int y;
            const char *label;
            lv_event_cb_t cb;
            uint32_t bg_color;
            uint32_t press_color;
            uint32_t border_color;
        } left_nav_btns[8] = {
            { &t_objects.tobj46, 10,  "RS485",        action_gotors485,            0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj49, 48,  "Touch Test",   action_gototouchtest,        0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj30, 86,  "GPIO",         action_switchtosetup,        0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj44, 124, "Bluetooth",    action_gotobluetooth,        0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj45, 162, "WIFI",         action_gotowifi,             0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj43, 200, "SSR",          action_gotossr,              0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj38, 238, "Display Test", action_gotodisw,             0xff312e81, 0xff4338ca, 0xff6366f1 },
            { &t_objects.tobj34, 276, LV_SYMBOL_LEFT " Exit", action_btn_back_to_password, 0xff991b1b, 0xff7f1d1d, 0xfff87171 }
        };

        for (int i = 0; i < 8; i++) {
            lv_obj_t *btn = lv_btn_create(parent_obj);
            *(left_nav_btns[i].target) = btn;
            lv_obj_set_pos(btn, 10, left_nav_btns[i].y);
            lv_obj_set_size(btn, 108, 32);
            lv_obj_add_event_cb(btn, left_nav_btns[i].cb, LV_EVENT_PRESSED, (void *)0);
            lv_obj_add_event_cb(btn, left_nav_btns[i].cb, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(left_nav_btns[i].bg_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn, lv_color_hex(left_nav_btns[i].press_color), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(btn, lv_color_hex(left_nav_btns[i].border_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *lbl = lv_label_create(btn);
            lv_obj_center(lbl);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(lbl, left_nav_btns[i].label);
        }

        // =========================================================================
        // ستون وسط: ساعت، تاریخ، بازر و کارت دماها (x: 128..306, w=178)
        // =========================================================================
        
        // ساعت (HH : MM) به صورت کاملاً متمرکز در وسط ستون وسط
        {
            lv_obj_t *clock_cont = lv_obj_create(parent_obj);
            lv_obj_set_pos(clock_cont, 128, 8);
            lv_obj_set_size(clock_cont, 178, 36);
            lv_obj_clear_flag(clock_cont, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_opa(clock_cont, 0, 0);
            lv_obj_set_style_border_width(clock_cont, 0, 0);
            lv_obj_set_style_pad_all(clock_cont, 0, 0);
            lv_obj_set_style_pad_gap(clock_cont, 4, 0);
            lv_obj_set_flex_flow(clock_cont, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(clock_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_add_event_cb(clock_cont, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(clock_cont, LV_OBJ_FLAG_CLICKABLE);

            // h
            lv_obj_t *obj = lv_label_create(clock_cont);
            t_objects.h = obj;
            lv_obj_add_event_cb(obj, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_ext_click_area(obj, 10);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "09");

            // colon
            lv_obj_t *colon = lv_label_create(clock_cont);
            t_objects.tobj51 = colon;
            lv_obj_set_style_text_font(colon, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(colon, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(colon, ":");

            // m
            lv_obj_t *obj_m = lv_label_create(clock_cont);
            t_objects.m = obj_m;
            lv_obj_add_event_cb(obj_m, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(obj_m, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_ext_click_area(obj_m, 10);
            lv_obj_set_style_text_font(obj_m, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj_m, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj_m, "56");
        }

        // تاریخ (YYYY / MM / DD) به صورت کاملاً متمرکز در وسط ستون وسط
        {
            lv_obj_t *date_cont = lv_obj_create(parent_obj);
            lv_obj_set_pos(date_cont, 128, 44);
            lv_obj_set_size(date_cont, 178, 20);
            lv_obj_clear_flag(date_cont, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_opa(date_cont, 0, 0);
            lv_obj_set_style_border_width(date_cont, 0, 0);
            lv_obj_set_style_pad_all(date_cont, 0, 0);
            lv_obj_set_style_pad_gap(date_cont, 4, 0);
            lv_obj_set_flex_flow(date_cont, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(date_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_add_event_cb(date_cont, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(date_cont, LV_OBJ_FLAG_CLICKABLE);

            // yer
            lv_obj_t *obj = lv_label_create(date_cont);
            t_objects.yer = obj;
            lv_obj_add_event_cb(obj, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_ext_click_area(obj, 10);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "2026");

            lv_obj_t *sep1 = lv_label_create(date_cont);
            lv_obj_set_style_text_font(sep1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(sep1, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(sep1, "/");

            // mont
            lv_obj_t *obj_m = lv_label_create(date_cont);
            t_objects.mont = obj_m;
            lv_obj_add_event_cb(obj_m, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(obj_m, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_ext_click_area(obj_m, 10);
            lv_obj_set_style_text_font(obj_m, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj_m, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj_m, "01");

            lv_obj_t *sep2 = lv_label_create(date_cont);
            lv_obj_set_style_text_font(sep2, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(sep2, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(sep2, "/");

            // dy
            lv_obj_t *obj_d = lv_label_create(date_cont);
            t_objects.dy = obj_d;
            lv_obj_add_event_cb(obj_d, action_gotortcsetup, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_add_flag(obj_d, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_ext_click_area(obj_d, 10);
            lv_obj_set_style_text_font(obj_d, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj_d, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj_d, "26");
        }

        // کارت و اسلایدر ولوم بازر (Buzzer Volume Card) به سبک مدرن اپل
        {
            lv_obj_t *buzzer_card = lv_obj_create(parent_obj);
            lv_obj_set_pos(buzzer_card, 128, 68);
            lv_obj_set_size(buzzer_card, 178, 48);
            lv_obj_clear_flag(buzzer_card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(buzzer_card, lv_color_hex(0xffffffff), 0);
            lv_obj_set_style_border_width(buzzer_card, 2, 0);
            lv_obj_set_style_border_color(buzzer_card, lv_color_hex(0xffcbd5e1), 0);
            lv_obj_set_style_radius(buzzer_card, 10, 0);
            lv_obj_set_style_pad_all(buzzer_card, 0, 0);

            // لیبل وضعیت بازر (قابل کلیک برای قطع/وصل سریع صدا - Mute Toggle)
            lv_obj_t *obj = lv_label_create(buzzer_card);
            t_objects.tobj42 = obj;
            lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 5);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff334155), 0);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, 0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_ext_click_area(obj, 6);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj42, LV_EVENT_CLICKED, 0);
            update_buzzer_label_text(get_var_buzzer());

            // اسلایدر ولوم با ابعاد دقیق، تراز شده و بدون خروج از کادر (Apple HIG)
            lv_obj_t *slider = lv_slider_create(buzzer_card);
            t_objects.tobj41 = slider;
            lv_obj_set_size(slider, 144, 8);
            lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -8);
            lv_slider_set_range(slider, 0, 255);
            lv_slider_set_value(slider, get_var_buzzer(), LV_ANIM_OFF);
            lv_obj_add_event_cb(slider, event_handler_cb_main_obj41, LV_EVENT_ALL, 0);
            lv_obj_set_ext_click_area(slider, 10);

            // ریل اسلایدر (Track)
            lv_obj_set_style_bg_color(slider, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);

            // نشانگر پرشده (Indicator)
            lv_obj_set_style_bg_color(slider, lv_color_hex(0xff2563eb), LV_PART_INDICATOR);
            lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

            // ناب دایره‌ای سفید با حلقه آبی (Knob - iOS Style)
            lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffffff), LV_PART_KNOB);
            lv_obj_set_style_border_width(slider, 2, LV_PART_KNOB);
            lv_obj_set_style_border_color(slider, lv_color_hex(0xff2563eb), LV_PART_KNOB);
            lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
            lv_obj_set_style_pad_all(slider, 3, LV_PART_KNOB);
            lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
        }

        // کارت دماها (Temperature Card)
        {
            lv_obj_t *card = lv_obj_create(parent_obj);
            t_objects.tobj31 = card;
            lv_obj_set_pos(card, 128, 122);
            lv_obj_set_size(card, 178, 186);
            lv_obj_add_event_cb(card, action_gotosysinfo, LV_EVENT_LONG_PRESSED, (void *)0);
            lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(card, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(card, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(card, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

            // عنوان کارت دما و آیکون
            {
                lv_obj_t *title = lv_label_create(card);
                lv_obj_set_pos(title, 4, 2);
                lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(title, "TEMPERATURES");
            }
            {
                lv_obj_t *ticon = lv_img_create(card);
                lv_obj_set_pos(ticon, 142, 2);
                lv_img_set_src(ticon, &img_temps);
                lv_img_set_zoom(ticon, 220);
            }

            // سطر ۱: ESP Core Temp
            {
                lv_obj_t *lbl = lv_label_create(card);
                t_objects.tobj34 = lbl;
                lv_obj_set_pos(lbl, 6, 36);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "ESP:");
            }
            {
                lv_obj_t *val = lv_label_create(card);
                t_objects.espt = val;
                lv_obj_set_pos(val, 60, 34);
                lv_obj_set_style_text_font(val, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(val, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(val, "00");
            }
            {
                lv_obj_t *u = lv_label_create(card);
                t_objects.tobj40 = u;
                lv_obj_set_pos(u, 116, 36);
                lv_obj_set_style_text_font(u, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(u, lv_color_hex(0xff64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(u, "°C");
            }

            // سطر ۲: RTC Temp
            {
                lv_obj_t *lbl = lv_label_create(card);
                t_objects.tobj47 = lbl;
                lv_obj_set_pos(lbl, 6, 72);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "RTC:");
            }
            {
                lv_obj_t *val = lv_label_create(card);
                t_objects.rtctemp = val;
                lv_obj_set_pos(val, 60, 70);
                lv_obj_set_style_text_font(val, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(val, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(val, "00");
            }
            {
                lv_obj_t *u = lv_label_create(card);
                t_objects.tobj48 = u;
                lv_obj_set_pos(u, 116, 72);
                lv_obj_set_style_text_font(u, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(u, lv_color_hex(0xff64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(u, "°C");
            }

            // سطر ۳: کوره ۱ (TC1)
            {
                lv_obj_t *lbl = lv_label_create(card);
                lv_obj_set_pos(lbl, 6, 108);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff2563eb), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "TC1:");
            }
            {
                lv_obj_t *val = lv_label_create(card);
                t_objects.t1 = val;
                lv_obj_set_pos(val, 60, 106);
                lv_obj_set_style_text_font(val, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(val, lv_color_hex(0xff2563eb), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(val, "000");
            }
            {
                lv_obj_t *u = lv_label_create(card);
                t_objects.tobj32 = u;
                lv_obj_set_pos(u, 116, 108);
                lv_obj_set_style_text_font(u, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(u, lv_color_hex(0xff2563eb), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(u, "°C");
            }

            // سطر ۴: کوره ۲ (TC2)
            {
                lv_obj_t *lbl = lv_label_create(card);
                lv_obj_set_pos(lbl, 6, 144);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff7c3aed), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "TC2:");
            }
            {
                lv_obj_t *val = lv_label_create(card);
                t_objects.t2 = val;
                lv_obj_set_pos(val, 60, 142);
                lv_obj_set_style_text_font(val, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(val, lv_color_hex(0xff7c3aed), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(val, "000");
            }
            {
                lv_obj_t *u = lv_label_create(card);
                t_objects.tobj33 = u;
                lv_obj_set_pos(u, 116, 144);
                lv_obj_set_style_text_font(u, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(u, lv_color_hex(0xff7c3aed), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(u, "°C");
            }
        }

        // =========================================================================
        // ستون سمت راست: کلید AUTO QC و کادر کلیدهای خارجی F1..F10 (x: 316..470)
        // =========================================================================
        {
            // کلید AUTO QC
            lv_obj_t *btn_qc = lv_btn_create(parent_obj);
            lv_obj_set_pos(btn_qc, 316, 10);
            lv_obj_set_size(btn_qc, 154, 36);
            lv_obj_add_event_cb(btn_qc, action_auto_qc_click, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(btn_qc, lv_color_hex(0xff059669), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_qc, lv_color_hex(0xff047857), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_radius(btn_qc, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_qc, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(btn_qc, lv_color_hex(0xff34d399), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_qc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl = lv_label_create(btn_qc);
                lv_obj_center(lbl);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, LV_SYMBOL_CHARGE " AUTO QC");
            }
        }

        // کادر کلیدهای اکسترنال F1..F10
        {
            lv_obj_t *card = lv_obj_create(parent_obj);
            t_objects.tobj35 = card;
            lv_obj_set_pos(card, 316, 52);
            lv_obj_set_size(card, 154, 256);
            lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(card, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(card, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(card, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

            // عنوان
            {
                lv_obj_t *title = lv_label_create(card);
                t_objects.tobj37 = title;
                lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
                lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(title, "External Keys");
            }

            // سطر وضعیت ADC و Exp متقارن بالای ستون‌های F1..F5 و F6..F10
            {
                lv_obj_t *lbl = lv_label_create(card);
                t_objects.tobj50 = lbl;
                lv_obj_set_pos(lbl, 6, 24);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "Exp:");
            }
            {
                lv_obj_t *val = lv_label_create(card);
                t_objects.exp_button = val;
                lv_obj_set_pos(val, 34, 24);
                lv_obj_set_style_text_color(val, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(val, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(val, "0000");
            }
            {
                lv_obj_t *lbl = lv_label_create(card);
                t_objects.tobj39 = lbl;
                lv_obj_set_pos(lbl, 74, 24);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "ADC:");
            }
            {
                lv_obj_t *val = lv_label_create(card);
                t_objects.readfs = val;
                lv_obj_set_pos(val, 104, 24);
                lv_obj_set_style_text_color(val, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(val, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(val, "0000");
            }

            // نگه داشتن پوینترهای استفاده‌نشده به صورت پنهان
            {
                lv_obj_t *lbl = lv_label_create(card);
                t_objects.tobj36 = lbl;
                lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(lbl, "");
            }
            {
                lv_obj_t *lbl = lv_label_create(card);
                t_objects.readfs_1 = lbl;
                lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(lbl, "");
            }

            // آرایش کاملاً متقارن دکمه‌های F1..F5 و F6..F10 در دو ستون
            // ستون ۱ (F1..F5): x = 6, w = 62, h = 34
            // ستون ۲ (F6..F10): x = 74, w = 62, h = 34
            const struct {
                lv_obj_t **target;
                int x, y;
                const char *name;
                lv_event_cb_t cb;
            } ext_btns[10] = {
                { &t_objects.tobj0, 6,  46,  "F1",  event_handler_cb_main_obj0 },
                { &t_objects.tobj1, 6,  86,  "F2",  event_handler_cb_main_obj1 },
                { &t_objects.tobj2, 6,  126, "F3",  event_handler_cb_main_obj2 },
                { &t_objects.tobj3, 6,  166, "F4",  event_handler_cb_main_obj3 },
                { &t_objects.tobj4, 6,  206, "F5",  event_handler_cb_main_obj4 },

                { &t_objects.tobj5, 74, 46,  "F6",  event_handler_cb_main_obj5 },
                { &t_objects.tobj6, 74, 86,  "F7",  event_handler_cb_main_obj6 },
                { &t_objects.tobj7, 74, 126, "F8",  event_handler_cb_main_obj7 },
                { &t_objects.tobj8, 74, 166, "F9",  event_handler_cb_main_obj8 },
                { &t_objects.tobj9, 74, 206, "F10", event_handler_cb_main_obj9 }
            };

            for (int i = 0; i < 10; i++) {
                lv_obj_t *btn = lv_btn_create(card);
                *(ext_btns[i].target) = btn;
                lv_obj_set_pos(btn, ext_btns[i].x, ext_btns[i].y);
                lv_obj_set_size(btn, 62, 34);
                lv_obj_add_event_cb(btn, ext_btns[i].cb, LV_EVENT_ALL, 0);
                lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xff4f46e5), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DISABLED);
                lv_obj_set_style_radius(btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_t *lbl = lv_label_create(btn);
                lv_obj_center(lbl);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, ext_btns[i].name);
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    {
        bool new_val = get_var_f1();
        bool cur_val = lv_obj_has_state(t_objects.tobj0, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj0;
            if (new_val) lv_obj_add_state(t_objects.tobj0, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj0, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f2();
        bool cur_val = lv_obj_has_state(t_objects.tobj1, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj1;
            if (new_val) lv_obj_add_state(t_objects.tobj1, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj1, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f3();
        bool cur_val = lv_obj_has_state(t_objects.tobj2, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj2;
            if (new_val) lv_obj_add_state(t_objects.tobj2, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj2, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f4();
        bool cur_val = lv_obj_has_state(t_objects.tobj3, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj3;
            if (new_val) lv_obj_add_state(t_objects.tobj3, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj3, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f5();
        bool cur_val = lv_obj_has_state(t_objects.tobj4, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj4;
            if (new_val) lv_obj_add_state(t_objects.tobj4, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj4, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f6();
        bool cur_val = lv_obj_has_state(t_objects.tobj5, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj5;
            if (new_val) lv_obj_add_state(t_objects.tobj5, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj5, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f7();
        bool cur_val = lv_obj_has_state(t_objects.tobj6, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj6;
            if (new_val) lv_obj_add_state(t_objects.tobj6, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj6, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f8();
        bool cur_val = lv_obj_has_state(t_objects.tobj7, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj7;
            if (new_val) lv_obj_add_state(t_objects.tobj7, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj7, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f9();
        bool cur_val = lv_obj_has_state(t_objects.tobj8, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj8;
            if (new_val) lv_obj_add_state(t_objects.tobj8, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj8, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_f10();
        bool cur_val = lv_obj_has_state(t_objects.tobj9, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj9;
            if (new_val) lv_obj_add_state(t_objects.tobj9, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj9, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_buzzer();
        int32_t cur_val = lv_slider_get_value(t_objects.tobj41);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj41;
            lv_slider_set_value(t_objects.tobj41, new_val, LV_ANIM_OFF);
            update_buzzer_label_text(new_val);
            t_tick_value_change_obj = NULL;
        }
    }
}

extern bool g_mcp23017_present;
void hwtest_mcp23017_set_output(uint8_t out_idx, bool on);
bool hwtest_mcp23017_get_output(uint8_t out_idx);
uint8_t hwtest_mcp23017_poll_inputs(void);

static lv_obj_t *s_mcp_modal = NULL;
static lv_obj_t *s_mcp_out_btns[6] = { NULL };
static lv_obj_t *s_mcp_in_lbls[2] = { NULL };

static void mcp_out_btn_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx > 5) return;
    bool cur = hwtest_mcp23017_get_output(idx);
    bool next = !cur;
    hwtest_mcp23017_set_output(idx, next);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_set_style_bg_color(btn, lv_color_hex(next ? 0xff10b981 : 0xff334155), 0);
}

static void mcp_all_toggle_cb(lv_event_t *e) {
    static bool all_on = false;
    all_on = !all_on;
    for (int i = 0; i < 6; i++) {
        hwtest_mcp23017_set_output(i, all_on);
        if (s_mcp_out_btns[i] && lv_obj_is_valid(s_mcp_out_btns[i])) {
            lv_obj_set_style_bg_color(s_mcp_out_btns[i], lv_color_hex(all_on ? 0xff10b981 : 0xff334155), 0);
        }
    }
}

static void mcp_close_cb(lv_event_t *e) {
    if (s_mcp_modal && lv_obj_is_valid(s_mcp_modal)) {
        lv_obj_add_flag(s_mcp_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void mcp_open_modal(lv_event_t *e) {
    if (s_mcp_modal && lv_obj_is_valid(s_mcp_modal)) {
        lv_obj_clear_flag(s_mcp_modal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_mcp_modal);
        return;
    }

    s_mcp_modal = lv_obj_create(t_objects.setup);
    lv_obj_set_size(s_mcp_modal, 440, 270);
    lv_obj_center(s_mcp_modal);
    lv_obj_clear_flag(s_mcp_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_mcp_modal, lv_color_hex(0xff0f172a), 0);
    lv_obj_set_style_border_color(s_mcp_modal, lv_color_hex(0xff38bdf8), 0);
    lv_obj_set_style_border_width(s_mcp_modal, 2, 0);
    lv_obj_set_style_radius(s_mcp_modal, 12, 0);
    lv_obj_set_style_shadow_width(s_mcp_modal, 0, 0);
    lv_obj_set_style_shadow_opa(s_mcp_modal, 0, 0);

    // Title
    lv_obj_t *title = lv_label_create(s_mcp_modal);
    lv_label_set_text(title, "MCP23017 EXPANSION (GPA0..5: OUT | GPA6..7: IN)");
    lv_obj_set_style_text_color(title, lv_color_hex(0xff38bdf8), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, -4);

    // 6 Output Buttons in 2 rows of 3
    const char *out_names[6] = { "OUT 0", "OUT 1", "OUT 2", "OUT 3", "OUT 4", "OUT 5" };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *b = lv_button_create(s_mcp_modal);
        s_mcp_out_btns[i] = b;
        int row = i / 3;
        int col = i % 3;
        lv_obj_set_pos(b, 10 + col * 136, 32 + row * 48);
        lv_obj_set_size(b, 126, 42);
        bool st = hwtest_mcp23017_get_output(i);
        lv_obj_set_style_bg_color(b, lv_color_hex(st ? 0xff10b981 : 0xff334155), 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_set_style_border_width(b, 1, 0);
        lv_obj_set_style_border_color(b, lv_color_hex(0xff64748b), 0);
        lv_obj_add_event_cb(b, mcp_out_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text_fmt(l, "%s (GPA%d)", out_names[i], i);
        lv_obj_set_style_text_color(l, lv_color_white(), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_center(l);
    }

    // Master Toggle Button
    lv_obj_t *all_btn = lv_button_create(s_mcp_modal);
    lv_obj_set_pos(all_btn, 10, 134);
    lv_obj_set_size(all_btn, 126, 38);
    lv_obj_set_style_bg_color(all_btn, lv_color_hex(0xff6366f1), 0);
    lv_obj_set_style_radius(all_btn, 8, 0);
    lv_obj_add_event_cb(all_btn, mcp_all_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *all_lbl = lv_label_create(all_btn);
    lv_label_set_text(all_lbl, "ALL TOGGLE");
    lv_obj_set_style_text_color(all_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(all_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(all_lbl);

    // 2 Inputs (GPA7 = IN0, GPA6 = IN1)
    for (int i = 0; i < 2; i++) {
        lv_obj_t *in_card = lv_obj_create(s_mcp_modal);
        lv_obj_set_pos(in_card, 146 + i * 136, 134);
        lv_obj_set_size(in_card, 126, 38);
        lv_obj_clear_flag(in_card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(in_card, lv_color_hex(0xff1e293b), 0);
        lv_obj_set_style_border_color(in_card, lv_color_hex(0xff475569), 0);
        lv_obj_set_style_border_width(in_card, 1, 0);
        lv_obj_set_style_radius(in_card, 8, 0);
        lv_obj_set_style_pad_all(in_card, 0, 0);

        lv_obj_t *l = lv_label_create(in_card);
        s_mcp_in_lbls[i] = l;
        lv_label_set_text_fmt(l, "IN%d (GPA%d)", i, (i == 0) ? 7 : 6);
        lv_obj_set_style_text_color(l, lv_color_hex(0xff94a3b8), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_center(l);
    }

    // Close button
    lv_obj_t *cls = lv_button_create(s_mcp_modal);
    lv_obj_align(cls, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_size(cls, 140, 34);
    lv_obj_set_style_bg_color(cls, lv_color_hex(0xff0284c7), 0);
    lv_obj_set_style_radius(cls, 8, 0);
    lv_obj_add_event_cb(cls, mcp_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clsl = lv_label_create(cls);
    lv_label_set_text(clsl, "CLOSE");
    lv_obj_set_style_text_color(clsl, lv_color_white(), 0);
    lv_obj_set_style_text_font(clsl, &lv_font_montserrat_14, 0);
    lv_obj_center(clsl);
}

void create_screen_setup() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.setup = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_ifsetuploaded, LV_EVENT_SCREEN_LOADED, (void *)0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, -11, -10);
            lv_obj_set_size(obj, 503, 60);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 3); // ۳ پیکسل پایین‌تر
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT); // بزرگتر و خواناتر
                    lv_label_set_text(obj, "GPIO");
                }
            }
        }
        if (g_mcp23017_present) {
            lv_obj_t *mcp_btn = lv_button_create(parent_obj);
            lv_obj_set_pos(mcp_btn, 286, 8);
            lv_obj_set_size(mcp_btn, 184, 36);
            lv_obj_set_style_bg_color(mcp_btn, lv_color_hex(0xff0284c7), 0);
            lv_obj_set_style_radius(mcp_btn, 8, 0);
            lv_obj_set_style_pad_hor(mcp_btn, 4, 0);
            lv_obj_add_event_cb(mcp_btn, mcp_open_modal, LV_EVENT_CLICKED, NULL);
            lv_obj_t *mcp_lbl = lv_label_create(mcp_btn);
            lv_label_set_text(mcp_lbl, "+ MCP23017 (6Q / 2I)");
            lv_obj_set_style_text_color(mcp_lbl, lv_color_white(), 0);
            lv_obj_set_style_text_font(mcp_lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(mcp_lbl);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 148, 135);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 240, 58);
            lv_obj_set_size(obj, 231, 254);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    t_objects.tobj10 = obj;
                    lv_obj_set_pos(obj, 147, 33);
                    lv_obj_set_size(obj, 57, 28);
                    lv_obj_add_event_cb(obj, event_handler_cb_setup_obj10, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "IN4");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    t_objects.tobj11 = obj;
                    lv_obj_set_pos(obj, 147, 71);
                    lv_obj_set_size(obj, 57, 28);
                    lv_obj_add_event_cb(obj, event_handler_cb_setup_obj11, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "IN5");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    t_objects.tobj12 = obj;
                    lv_obj_set_pos(obj, 71, 33);
                    lv_obj_set_size(obj, 57, 28);
                    lv_obj_add_event_cb(obj, event_handler_cb_setup_obj12, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "IN2");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    t_objects.tobj13 = obj;
                    lv_obj_set_pos(obj, 71, 71);
                    lv_obj_set_size(obj, 57, 28);
                    lv_obj_add_event_cb(obj, event_handler_cb_setup_obj13, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "IN3");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    t_objects.tobj14 = obj;
                    lv_obj_set_pos(obj, 24, 153);
                    lv_obj_set_size(obj, 57, 28);
                    lv_obj_add_event_cb(obj, event_handler_cb_setup_obj14, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "IN6");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    t_objects.tobj15 = obj;
                    lv_obj_set_pos(obj, 119, 153);
                    lv_obj_set_size(obj, 57, 28);
                    lv_obj_add_event_cb(obj, event_handler_cb_setup_obj15, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "IN7");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 73, -15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "INPUT");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj16 = obj;
            lv_obj_set_pos(obj, 254, 109);
            lv_obj_set_size(obj, 57, 28);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj16, LV_EVENT_ALL, 0);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "IN0");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj17 = obj;
            lv_obj_set_pos(obj, 254, 147);
            lv_obj_set_size(obj, 57, 28);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj17, LV_EVENT_ALL, 0);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff43a106), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "IN1");
                }
            }
        }
        {
            // v14_15
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.v14_15 = obj;
            lv_obj_set_pos(obj, 412, 196);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0000");
        }
        {
            // v10_11
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.v10_11 = obj;
            lv_obj_set_pos(obj, 260, 196);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0000");
        }
        {
            // v12_13
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.v12_13 = obj;
            lv_obj_set_pos(obj, 336, 196);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0000");
        }
        {
            // v16
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.v16 = obj;
            lv_obj_set_pos(obj, 289, 284);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0000");
        }
        {
            // v17
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.v17 = obj;
            lv_obj_set_pos(obj, 384, 284);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0000");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj53 = obj;
            lv_obj_set_pos(obj, 11, 58);
            lv_obj_set_size(obj, 198, 254);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffececec), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 9, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff717171), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj18 = obj;
            lv_obj_set_pos(obj, 23, 89);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj18, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj20 = obj;
            lv_obj_set_pos(obj, 23, 135);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj20, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj19 = obj;
            lv_obj_set_pos(obj, 23, 181);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj19, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj21 = obj;
            lv_obj_set_pos(obj, 121, 181);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj21, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj22 = obj;
            lv_obj_set_pos(obj, 121, 135);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj22, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj23 = obj;
            lv_obj_set_pos(obj, 121, 89);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj23, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj24 = obj;
            lv_obj_set_pos(obj, 23, 270);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj24, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj25 = obj;
            lv_obj_set_pos(obj, 23, 225);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj25, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj26 = obj;
            lv_obj_set_pos(obj, 121, 270);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj26, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            t_objects.tobj27 = obj;
            lv_obj_set_pos(obj, 121, 225);
            lv_obj_set_size(obj, 50, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_setup_obj27, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 74, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q0");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 172, 140);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q6");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 172, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q5");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 74, 275);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q4");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 74, 230);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q3");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 74, 185);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q2");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 74, 140);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q1");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 172, 275);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q9");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 172, 230);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q8");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 172, 185);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Q7");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 68, 61);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "OUTPUT");
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(obj, event_handler_output_all_toggle, LV_EVENT_CLICKED, (void *)0);
        }
    }
    
    gpio_attach_double_tap_recursive(t_objects.setup);
    add_test_top_back_button(t_objects.setup);
    tick_screen_setup();
}

void tick_screen_setup() {
    {
        bool new_val = get_var_anbl();
        bool cur_val = lv_obj_has_flag(t_objects.setup, LV_OBJ_FLAG_CLICKABLE);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.setup;
            if (new_val) lv_obj_add_flag(t_objects.setup, LV_OBJ_FLAG_CLICKABLE);
            else lv_obj_clear_flag(t_objects.setup, LV_OBJ_FLAG_CLICKABLE);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__14();
        bool cur_val = lv_obj_has_state(t_objects.tobj10, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj10;
            if (new_val) lv_obj_add_state(t_objects.tobj10, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj10, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__15();
        bool cur_val = lv_obj_has_state(t_objects.tobj11, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj11;
            if (new_val) lv_obj_add_state(t_objects.tobj11, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj11, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__12();
        bool cur_val = lv_obj_has_state(t_objects.tobj12, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj12;
            if (new_val) lv_obj_add_state(t_objects.tobj12, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj12, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__13();
        bool cur_val = lv_obj_has_state(t_objects.tobj13, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj13;
            if (new_val) lv_obj_add_state(t_objects.tobj13, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj13, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__16();
        bool cur_val = lv_obj_has_state(t_objects.tobj14, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj14;
            if (new_val) lv_obj_add_state(t_objects.tobj14, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj14, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__17();
        bool cur_val = lv_obj_has_state(t_objects.tobj15, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj15;
            if (new_val) lv_obj_add_state(t_objects.tobj15, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj15, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__10();
        bool cur_val = lv_obj_has_state(t_objects.tobj16, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj16;
            if (new_val) lv_obj_add_state(t_objects.tobj16, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj16, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var__11();
        bool cur_val = lv_obj_has_state(t_objects.tobj17, LV_STATE_DISABLED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj17;
            if (new_val) lv_obj_add_state(t_objects.tobj17, LV_STATE_DISABLED);
            else lv_obj_clear_state(t_objects.tobj17, LV_STATE_DISABLED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q1();
        bool cur_val = lv_obj_has_state(t_objects.tobj18, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj18;
            if (new_val) lv_obj_add_state(t_objects.tobj18, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj18, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q3();
        bool cur_val = lv_obj_has_state(t_objects.tobj19, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj19;
            if (new_val) lv_obj_add_state(t_objects.tobj19, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj19, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q2();
        bool cur_val = lv_obj_has_state(t_objects.tobj20, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj20;
            if (new_val) lv_obj_add_state(t_objects.tobj20, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj20, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q8();
        bool cur_val = lv_obj_has_state(t_objects.tobj21, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj21;
            if (new_val) lv_obj_add_state(t_objects.tobj21, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj21, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q7();
        bool cur_val = lv_obj_has_state(t_objects.tobj22, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj22;
            if (new_val) lv_obj_add_state(t_objects.tobj22, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj22, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q6();
        bool cur_val = lv_obj_has_state(t_objects.tobj23, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj23;
            if (new_val) lv_obj_add_state(t_objects.tobj23, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj23, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q5();
        bool cur_val = lv_obj_has_state(t_objects.tobj24, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj24;
            if (new_val) lv_obj_add_state(t_objects.tobj24, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj24, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q4();
        bool cur_val = lv_obj_has_state(t_objects.tobj25, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj25;
            if (new_val) lv_obj_add_state(t_objects.tobj25, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj25, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q10(); // tobj26 = Q9 (SSR2 / GPIO38)
        bool cur_val = lv_obj_has_state(t_objects.tobj26, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj26;
            if (new_val) lv_obj_add_state(t_objects.tobj26, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj26, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_q9(); // tobj27 = Q8 (Relay Q8 / GPIO41)
        bool cur_val = lv_obj_has_state(t_objects.tobj27, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj27;
            if (new_val) lv_obj_add_state(t_objects.tobj27, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj27, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    if (g_mcp23017_present && s_mcp_modal && !lv_obj_has_flag(s_mcp_modal, LV_OBJ_FLAG_HIDDEN)) {
        uint8_t in_val = hwtest_mcp23017_poll_inputs();
        bool in0_active = !(in_val & (1 << 7)); // GPA7 (Active LOW)
        bool in1_active = !(in_val & (1 << 6)); // GPA6 (Active LOW)

        if (s_mcp_in_lbls[0] && lv_obj_is_valid(s_mcp_in_lbls[0])) {
            lv_obj_set_style_text_color(s_mcp_in_lbls[0], lv_color_hex(in0_active ? 0xff10b981 : 0xff94a3b8), 0);
            lv_label_set_text_fmt(s_mcp_in_lbls[0], "IN0: %s", in0_active ? "ON (LOW)" : "OFF (HIGH)");
        }
        if (s_mcp_in_lbls[1] && lv_obj_is_valid(s_mcp_in_lbls[1])) {
            lv_obj_set_style_text_color(s_mcp_in_lbls[1], lv_color_hex(in1_active ? 0xff10b981 : 0xff94a3b8), 0);
            lv_label_set_text_fmt(s_mcp_in_lbls[1], "IN1: %s", in1_active ? "ON (LOW)" : "OFF (HIGH)");
        }
    }
}

extern const lv_image_dsc_t img_printer;
void hwtest_print_haffler(void);
bool hwtest_is_printing(void);
void hwtest_stop_print(void);

// =========================================================================================================
// 🚨 خط قرمز: متن و محتوای پرینت تثبیت‌شده و کامل است. هیچ ایجنتی حق دستکاری محتوای چاپگر را ندارد.
// فشردن کلید: شروع پرینت از ابتدا — فشردن مجدد حین چاپ: لغو و توقف فوری + فید کاغذ سفید
// =========================================================================================================
static lv_obj_t *s_rs485_prn_btn = NULL;
static bool s_rs485_prn_last_state = false;

static void rs485_printer_btn_cb(lv_event_t *e) {
    hwtest_print_haffler();
}

void create_screen_rs485() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.rs485 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, rs485_double_tap_cb, LV_EVENT_CLICKED, (void *)0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj54 = obj;
            lv_obj_set_pos(obj, 13, 61);
            lv_obj_set_size(obj, 147, 73);
            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff087735), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -8, -12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Data Send:");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj55 = obj;
            lv_obj_set_pos(obj, 320, 61);
            lv_obj_set_size(obj, 147, 73);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffa13232), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -13, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Data Received:");
                }
                {
                    // datareceive
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.datareceive = obj;
                    lv_obj_set_pos(obj, -13, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj56 = obj;
            lv_obj_set_pos(obj, -10, -23);
            lv_obj_set_size(obj, 499, 70);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.tobj57 = obj;
                    lv_obj_set_pos(obj, 199, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "RS485");
                }
            }
        }
        {
            // کلید پرینتر با حلقه هشدار قرمز دایره‌ای دور دکمه
            lv_obj_t *prn_btn = lv_button_create(parent_obj);
            s_rs485_prn_btn = prn_btn;
            s_rs485_prn_last_state = !hwtest_is_printing();
            lv_obj_set_pos(prn_btn, 426, 4);
            lv_obj_set_size(prn_btn, 40, 40);
            lv_obj_set_style_bg_color(prn_btn, lv_color_hex(0xff0284c7), 0);
            lv_obj_set_style_bg_color(prn_btn, lv_color_hex(0xff0369a1), LV_STATE_PRESSED);
            lv_obj_set_style_radius(prn_btn, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(prn_btn, 3, 0);
            lv_obj_set_style_border_color(prn_btn, lv_color_hex(0xffef4444), 0);
            lv_obj_set_style_border_color(prn_btn, lv_color_hex(0xffdc2626), LV_STATE_PRESSED);
            lv_obj_set_style_pad_all(prn_btn, 0, 0);
            lv_obj_set_style_shadow_width(prn_btn, 0, 0);
            lv_obj_add_event_cb(prn_btn, rs485_printer_btn_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *img = lv_image_create(prn_btn);
            lv_image_set_src(img, &img_printer);
            lv_image_set_scale(img, 150);
            lv_obj_center(img);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 160, 94);
            lv_obj_set_size(obj, 160, LV_SIZE_CONTENT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "<--------->");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 160, 75);
            lv_obj_set_size(obj, 160, LV_SIZE_CONTENT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Data Transmission");
        }
        {
            lv_obj_t *obj = lv_keyboard_create(parent_obj);
            t_objects.tobj60 = obj;
            lv_obj_set_pos(obj, 0, 160);
            lv_obj_set_size(obj, 480, 160);
            lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_USER_1);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            style_keyboard_action_keys(obj);
        }
        {
            // rssend
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            t_objects.rssend = obj;
            lv_obj_set_pos(obj, 24, 88);
            lv_obj_set_size(obj, 124, 32);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, event_handler_cb_rs485_rssend, LV_EVENT_ALL, 0);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 195, 118);
            lv_obj_set_size(obj, 90, 34);
            lv_obj_add_event_cb(obj, action_senddata, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d8fcb), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "send");
                    lv_obj_center(obj);
                }
            }
        }
    }
    lv_keyboard_set_textarea(t_objects.tobj60, t_objects.rssend);
    add_test_top_back_button(t_objects.rs485);
    
    tick_screen_rs485();
}

void tick_screen_rs485() {
    {
        const char *new_val = get_var_rstextsend();
        const char *cur_val = lv_textarea_get_text(t_objects.rssend);
        uint32_t max_length = lv_textarea_get_max_length(t_objects.rssend);
        if (strncmp(new_val, cur_val, max_length) != 0) {
            t_tick_value_change_obj = t_objects.rssend;
            lv_textarea_set_text(t_objects.rssend, new_val);
            t_tick_value_change_obj = NULL;
        }
    }
    if (s_rs485_prn_btn && lv_obj_is_valid(s_rs485_prn_btn)) {
        bool is_prn = hwtest_is_printing();
        if (is_prn != s_rs485_prn_last_state) {
            s_rs485_prn_last_state = is_prn;
            if (is_prn) {
                // حالت چاپ فعال: تغییر به رنگ کهربایی (Amber) جهت آگاهی اپراتور از امکان لمس مجدد برای توقف
                lv_obj_set_style_bg_color(s_rs485_prn_btn, lv_color_hex(0xffd97706), 0);
                lv_obj_set_style_border_color(s_rs485_prn_btn, lv_color_hex(0xfff59e0b), 0);
            } else {
                // حالت عادی/توقف: آبی استاندارد اپل با رینگ قرمز هشدار
                lv_obj_set_style_bg_color(s_rs485_prn_btn, lv_color_hex(0xff0284c7), 0);
                lv_obj_set_style_border_color(s_rs485_prn_btn, lv_color_hex(0xffef4444), 0);
            }
        }
    }
}

void create_screen_gsm() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.gsm = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc7ced0), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 23, 100);
            lv_obj_set_size(obj, 149, 141);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 308, 100);
            lv_obj_set_size(obj, 149, 141);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // receivemessage
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.receivemessage = obj;
                    lv_obj_set_pos(obj, 16, 42);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "message");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj28 = obj;
            lv_obj_set_pos(obj, 53, 55);
            lv_obj_set_size(obj, 90, 35);
            lv_obj_add_event_cb(obj, event_handler_cb_gsm_obj28, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff30296a), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "   send\nmessage");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj29 = obj;
            lv_obj_set_pos(obj, 338, 56);
            lv_obj_set_size(obj, 90, 35);
            lv_obj_add_event_cb(obj, event_handler_cb_gsm_obj29, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff30296a), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " receive\nmessage");
                }
            }
        }
        {
            // sendmessage
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.sendmessage = obj;
            lv_obj_set_pos(obj, 57, 160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "message");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj61 = obj;
            lv_obj_set_pos(obj, -7, -22);
            lv_obj_set_size(obj, 491, 69);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff46008c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.tobj62 = obj;
            lv_obj_set_pos(obj, 212, 10);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "GSM");
        }
        {
            // gsmrefresh
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.gsmrefresh = obj;
            lv_obj_set_pos(obj, 221, 145);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff5b4949), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "00");
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj63 = obj;
            lv_obj_set_pos(obj, 11, 280);
            lv_obj_set_size(obj, 98, 36);
            lv_obj_add_event_cb(obj, action_switchtomain, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2c4d67), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Back");
                }
            }
        }
    }
    
    tick_screen_gsm();
}

void tick_screen_gsm() {
    {
        bool new_val = get_var_send_message();
        bool cur_val = lv_obj_has_state(t_objects.tobj28, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj28;
            if (new_val) lv_obj_add_state(t_objects.tobj28, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj28, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_receive_message();
        bool cur_val = lv_obj_has_state(t_objects.tobj29, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj29;
            if (new_val) lv_obj_add_state(t_objects.tobj29, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.tobj29, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
}

void create_screen_bluetooth() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.bluetooth = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, event_handler_cb_bluetooth_bluetooth, LV_EVENT_ALL, 0);
    lv_obj_add_event_cb(obj, bluetooth_double_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj65 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 48);
            lv_obj_add_event_cb(obj, bluetooth_double_tap_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.tobj66 = obj;
                    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Bluetooth Diagnostic");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj64 = obj;
            lv_obj_set_pos(obj, 20, 60);
            lv_obj_set_size(obj, 116, 36);
            lv_obj_add_event_cb(obj, action_ble_scan, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t *lbl = lv_label_create(obj);
            lv_label_set_text(lbl, LV_SYMBOL_REFRESH " Scan");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(lbl);
        }
        {
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            ble_test_dd = obj;
            lv_obj_set_pos(obj, 20, 106);
            lv_obj_set_size(obj, 310, 36);
            lv_dropdown_set_options(obj, "Tap Scan to search BLE devices...");
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 342, 106);
            lv_obj_set_size(obj, 118, 36);
            lv_obj_add_event_cb(obj, action_ble_connect, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff10b981), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t *lbl = lv_label_create(obj);
            lv_label_set_text(lbl, LV_SYMBOL_OK " Connect");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(lbl);
        }
        // کارت وضعیت شکیل اپل
        {
            lv_obj_t *card = lv_obj_create(parent_obj);
            lv_obj_set_pos(card, 20, 154);
            lv_obj_set_size(card, 440, 100);
            lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(card, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(card, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *obj1 = lv_label_create(card);
            t_objects.bolotosinfo = obj1;
            lv_obj_set_pos(obj1, 4, 4);
            lv_obj_set_size(obj1, 400, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj1, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj1, "Bluetooth status: Standby");

            lv_obj_t *obj2 = lv_label_create(card);
            t_objects.bolotosstate = obj2;
            lv_obj_set_pos(obj2, 4, 38);
            lv_obj_set_size(obj2, 400, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj2, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj2, lv_color_hex(0xff2563eb), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj2, "Ready for scan or connection");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.tobj69 = obj;
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.tobj67 = obj;
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(obj, "");
        }
    }
    
    add_test_top_back_button(t_objects.bluetooth);
    tick_screen_bluetooth();
}

void tick_screen_bluetooth() {
    {
        bool new_val = get_var_enableble();
        bool cur_val = lv_obj_has_state(t_objects.bluetooth, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.bluetooth;
            if (new_val) lv_obj_add_state(t_objects.bluetooth, LV_STATE_CHECKED);
            else lv_obj_clear_state(t_objects.bluetooth, LV_STATE_CHECKED);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_enableble();
        bool cur_val = lv_obj_has_flag(t_objects.tobj69, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj69;
            if (new_val) lv_obj_add_flag(t_objects.tobj69, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(t_objects.tobj69, LV_OBJ_FLAG_HIDDEN);
            t_tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = get_var_changblelabl();
        bool cur_val = lv_obj_has_flag(t_objects.tobj67, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            t_tick_value_change_obj = t_objects.tobj67;
            if (new_val) lv_obj_add_flag(t_objects.tobj67, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(t_objects.tobj67, LV_OBJ_FLAG_HIDDEN);
            t_tick_value_change_obj = NULL;
        }
    }
}

void create_screen_wifi() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.wifi = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, wifi_double_tap_cb, LV_EVENT_CLICKED, (void *)0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_state(obj, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        
        // ۱. نوار هدر بالا (سرمه‌ای متالیک تمیز با تراز وسط و دکمه بازگشت)
        {
            lv_obj_t *hdr = lv_obj_create(parent_obj);
            t_objects.tobj72 = hdr;
            lv_obj_set_pos(hdr, 0, 0);
            lv_obj_set_size(hdr, 480, 48);
            lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(hdr, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(hdr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(hdr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *title = lv_label_create(hdr);
            t_objects.tobj73 = title;
            lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(title, "WiFi Diagnostic & Test");
        }

        // ۲. سطر ۱: دراپ‌داون شبکه‌ها (۳۳۰ پیکسل) + دکمه اسکن (۱۱۶ پیکسل)
        {
            lv_obj_t *dd = lv_dropdown_create(parent_obj);
            wifi_test_dd = dd;
            lv_obj_set_pos(dd, 12, 52);
            lv_obj_set_size(dd, 330, 34);
            lv_dropdown_set_options(dd, "Tap Scan to search networks...");
            lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(dd, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(dd, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(dd, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(dd, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(dd, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *btn_scan = lv_btn_create(parent_obj);
            t_objects.tobj71 = btn_scan;
            lv_obj_set_pos(btn_scan, 352, 52);
            lv_obj_set_size(btn_scan, 116, 34);
            lv_obj_add_event_cb(btn_scan, action_scanwifi, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(btn_scan, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(btn_scan, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_scan, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *lbl = lv_label_create(btn_scan);
            t_objects.tobj76 = lbl;
            lv_label_set_text(lbl, LV_SYMBOL_REFRESH " Scan");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(lbl);
        }

        // ۳. سطر ۲: ورودی رمز (۲۸۶ پیکسل) + دکمه چشم (۳۸ پیکسل) + دکمه اتصال (۱۱۶ پیکسل)
        {
            lv_obj_t *ta = lv_textarea_create(parent_obj);
            wifi_test_pass_ta = ta;
            lv_obj_set_pos(ta, 12, 90);
            lv_obj_set_size(ta, 286, 34);
            lv_textarea_set_one_line(ta, true);
            lv_textarea_set_password_mode(ta, true);
            lv_textarea_set_placeholder_text(ta, "Enter password...");
            lv_obj_add_event_cb(ta, wifi_pass_focus_cb, LV_EVENT_ALL, NULL);
            lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ta, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ta, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ta, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ta, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *btn_eye = lv_btn_create(parent_obj);
            lv_obj_set_pos(btn_eye, 304, 90);
            lv_obj_set_size(btn_eye, 38, 34);
            lv_obj_add_event_cb(btn_eye, wifi_pass_eye_cb, LV_EVENT_ALL, NULL);
            lv_obj_set_style_bg_color(btn_eye, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(btn_eye, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_eye, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(btn_eye, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_eye, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *lbl = lv_label_create(btn_eye);
            lv_label_set_text(lbl, LV_SYMBOL_EYE_CLOSE);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 2);
        }
        {
            lv_obj_t *btn_conn = lv_btn_create(parent_obj);
            lv_obj_set_pos(btn_conn, 352, 90);
            lv_obj_set_size(btn_conn, 116, 34);
            lv_obj_add_event_cb(btn_conn, action_test_wifi_connect, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(btn_conn, lv_color_hex(0xff10b981), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(btn_conn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_conn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *lbl = lv_label_create(btn_conn);
            lv_label_set_text(lbl, LV_SYMBOL_OK " Connect");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(lbl);
        }

        // ۴. سطر ۳: نوار وضعیت و تشخیصی شبکه (y = 130)
        {
            lv_obj_t *st = lv_label_create(parent_obj);
            t_objects.wifi0 = st;
            lv_obj_set_pos(st, 14, 130);
            lv_obj_set_size(st, 320, 22);
            lv_label_set_long_mode(st, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_font(st, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(st, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(st, "Status: Ready");
        }
        {
            lv_obj_t *ip = lv_label_create(parent_obj);
            t_objects.wifi1 = ip;
            lv_obj_set_pos(ip, 336, 130);
            lv_obj_set_size(ip, 132, 22);
            lv_obj_set_style_text_font(ip, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ip, lv_color_hex(0xff10b981), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ip, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(ip, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.wifi2 = obj;
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj70 = obj;
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.tobj74 = obj;
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(obj, "");
        }

        // ۵. سطر ۴: کیبورد مجازی کامل مدرن (y = 153, h = 161 - افزایش ۱۵ پیکسل به سمت بالا)
        {
            lv_obj_t *kb = lv_buttonmatrix_create(parent_obj);
            wifi_test_kb = kb;
            lv_obj_set_pos(kb, 6, 153);
            lv_obj_set_size(kb, 468, 161);
            lv_buttonmatrix_set_map(kb, wifi_kb_map);
            lv_buttonmatrix_set_ctrl_map(kb, wifi_kb_ctrl);
            lv_obj_add_event_cb(kb, wifi_kb_button_cb, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_style_bg_color(kb, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(kb, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(kb, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(kb, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_gap(kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

            // استایل کلیدهای کیبورد (حذف کامل سایه نرم‌افزاری برای حداکثر سرعت ۶۰ فریم)
            lv_obj_set_style_bg_color(kb, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(kb, lv_color_hex(0xffe2e8f0), LV_PART_ITEMS | LV_STATE_PRESSED);
            lv_obj_set_style_text_color(kb, lv_color_hex(0xff0f172a), LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(kb, 6, LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(kb, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(kb, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
            style_keyboard_action_keys(kb);
        }
    }
    if (wifi_test_kb) {
        lv_obj_clear_flag(wifi_test_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(wifi_test_kb);
    }
    add_test_top_back_button(t_objects.wifi);
    tick_screen_wifi();
}

void tick_screen_wifi() {
    // عدم دستکاری وضعیت نمایش کلید اسکن تا آیکون و نوشته همیشه پایدار و قابل مشاهده بماند
}

void create_screen_display_w() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.display_w = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_gotodisb, LV_EVENT_RELEASED, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    tick_screen_display_w();
}

void tick_screen_display_w() {
}

void create_screen_display_b() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.display_b = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_gotodisr, LV_EVENT_RELEASED, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    tick_screen_display_b();
}

void tick_screen_display_b() {
}

void create_screen_display_test_r() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.display_test_r = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_gotodisg, LV_EVENT_RELEASED, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff30000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    tick_screen_display_test_r();
}

void tick_screen_display_test_r() {
}

void create_screen_display_g() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.display_g = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_gotodisblue, LV_EVENT_RELEASED, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    tick_screen_display_g();
}

void tick_screen_display_g() {
}

void create_screen_display_blue() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.display_blue = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_switchtomain, LV_EVENT_RELEASED, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0000ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    tick_screen_display_blue();
}

void tick_screen_display_blue() {
}

void create_screen_sys_info() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.sys_info = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 37, 52);
            lv_obj_set_size(obj, 400, 196);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // sys_cpu
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_cpu = obj;
                    lv_obj_set_pos(obj, -4, 177);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "t_objects.sys_Freq");
                }
                {
                    // sys_wifi
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_wifi = obj;
                    lv_obj_set_pos(obj, -4, 256);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "sys_wifi");
                }
                {
                    // sys_uptime
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_uptime = obj;
                    lv_obj_set_pos(obj, -4, 534);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "sys_uptime");
                }
                {
                    // sys_ram
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_ram = obj;
                    lv_obj_set_pos(obj, -4, 100);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "t_objects.sys_ram");
                }
                {
                    // sys_flash
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_flash = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "t_objects.sys_flash");
                }
                {
                    // sys_i2c
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_i2c = obj;
                    lv_obj_set_pos(obj, -4, 430);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "sys_i2c");
                }
                {
                    // sys_chipmac
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.sys_chipmac = obj;
                    lv_obj_set_pos(obj, -4, 341);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "sys_chipmac");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj77 = obj;
            lv_obj_set_pos(obj, -15, -20);
            lv_obj_set_size(obj, 503, 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 195, 7);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "Sys info");
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj78 = obj;
            lv_obj_set_pos(obj, 7, 278);
            lv_obj_set_size(obj, 98, 36);
            lv_obj_add_event_cb(obj, action_switchtomain, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2c4d67), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Back");
                }
            }
        }
    }
    
    tick_screen_sys_info();
}

void tick_screen_sys_info() {
}

void create_screen_touch_test() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.touch_test = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_drawline, LV_EVENT_PRESSING, (void *)0);
    lv_obj_add_event_cb(obj, action_settouch, LV_EVENT_RELEASED, (void *)0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    s_touch_back = NULL;
    
    tick_screen_touch_test();
}

void tick_screen_touch_test() {
}

void create_screen_clock_setup() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.clock_setup = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj79 = obj;
            lv_obj_set_pos(obj, -11, -12);
            lv_obj_set_size(obj, 503, 60);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.tobj80 = obj;
            lv_obj_set_pos(obj, 154, 7);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Clock settings");
        }
        {
            lv_obj_t *obj = lv_keyboard_create(parent_obj);
            t_objects.tobj81 = obj;
            lv_obj_set_pos(obj, 0, 160);
            lv_obj_set_size(obj, 480, 160);
            lv_keyboard_set_map(obj, LV_KEYBOARD_MODE_USER_1, rtc_kb_map, rtc_kb_ctrl);
            lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_USER_1);
            lv_obj_add_event_cb(obj, rtc_kb_ready_cb, LV_EVENT_READY, NULL);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff6e6e6e), LV_PART_MAIN | LV_STATE_DEFAULT);
            style_keyboard_action_keys(obj);
        }
        {
            // ta_rtc
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            t_objects.ta_rtc = obj;
            lv_obj_set_pos(obj, 35, 100);
            lv_obj_set_size(obj, 411, 47);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, event_handler_cb_clock_setup_ta_rtc, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc5c5c5), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj82 = obj;
            lv_obj_set_pos(obj, 75, 55);
            lv_obj_set_size(obj, 331, 33);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff58636c), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Year    Month    Day     Hour     Min");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj83 = obj;
            lv_obj_set_pos(obj, 292, 145);
            lv_obj_set_size(obj, 184, 32);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);   // OKِ مستقل حذف شد (روی کیبرد می‌افتاد)؛ تاییدِ کیبرد جایگزین است
            lv_obj_add_event_cb(obj, rtc_apply_event_cb, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.tobj84 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "ok");
                }
            }
        }
        {
            lv_obj_t *fields[5];
            const int xs[5] = { 22, 116, 204, 292, 380 };
            const int ws[5] = { 78, 72, 72, 72, 72 };
            const char *txt[5] = { "2026", "01", "01", "00", "00" };
            lv_obj_t **refs[5] = { &rtc_year_ta, &rtc_month_ta, &rtc_day_ta, &rtc_hour_ta, &rtc_min_ta };
            for (int i = 0; i < 5; i++) {
                lv_obj_t *ta = lv_textarea_create(parent_obj);
                fields[i] = ta;
                *refs[i] = ta;
                lv_obj_set_pos(ta, xs[i], 100);
                lv_obj_set_size(ta, ws[i], 42);
                lv_textarea_set_one_line(ta, true);
                lv_textarea_set_max_length(ta, i == 0 ? 4 : 2);
                lv_textarea_set_password_mode(ta, false);
                lv_textarea_set_text(ta, txt[i]);
                lv_obj_add_event_cb(ta, rtc_field_event_cb, LV_EVENT_ALL, 0);
                lv_obj_set_style_bg_color(ta, lv_color_hex(0xffc5c5c5), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            (void)fields;
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj85 = obj;
            lv_obj_set_pos(obj, 8, 7);   // برگشت → بالا-چپِ صفحه
            lv_obj_set_size(obj, 82, 34);
            lv_obj_add_event_cb(obj, action_switchtomain, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2c4d67), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.tobj86 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Back");
                }
            }
        }
    }
    lv_keyboard_set_textarea(t_objects.tobj81, rtc_year_ta);
    rtc_editing = false;   // ورودِ تازه به صفحه → نمایشِ زمانِ فعلی، آپدیتِ خودکار فعال
    tick_screen_clock_setup();
}

void tick_screen_clock_setup() {
    {
        const char *new_val = get_var_rtc_text();
        const char *cur_val = lv_textarea_get_text(t_objects.ta_rtc);
        uint32_t max_length = lv_textarea_get_max_length(t_objects.ta_rtc);
        if (strncmp(new_val, cur_val, max_length) != 0) {
            t_tick_value_change_obj = t_objects.ta_rtc;
            lv_textarea_set_text(t_objects.ta_rtc, new_val);
            t_tick_value_change_obj = NULL;
        }
    }
    if (!rtc_editing && rtc_year_ta && rtc_month_ta && rtc_day_ta && rtc_hour_ta && rtc_min_ta) {
        int y, mo, d, h, mi;
        const char *txt = get_var_rtc_text();
        if (txt && sscanf(txt, "%d.%d.%d.%d.%d", &y, &mo, &d, &h, &mi) == 5) {
            char b[8];
            snprintf(b, sizeof(b), "%04d", y);
            if (strcmp(lv_textarea_get_text(rtc_year_ta), b) != 0) lv_textarea_set_text(rtc_year_ta, b);
            snprintf(b, sizeof(b), "%02d", mo);
            if (strcmp(lv_textarea_get_text(rtc_month_ta), b) != 0) lv_textarea_set_text(rtc_month_ta, b);
            snprintf(b, sizeof(b), "%02d", d);
            if (strcmp(lv_textarea_get_text(rtc_day_ta), b) != 0) lv_textarea_set_text(rtc_day_ta, b);
            snprintf(b, sizeof(b), "%02d", h);
            if (strcmp(lv_textarea_get_text(rtc_hour_ta), b) != 0) lv_textarea_set_text(rtc_hour_ta, b);
            snprintf(b, sizeof(b), "%02d", mi);
            if (strcmp(lv_textarea_get_text(rtc_min_ta), b) != 0) lv_textarea_set_text(rtc_min_ta, b);
        }
    }
}

void create_screen_ssr() {
    lv_obj_t *obj = lv_obj_create(0);
    t_objects.ssr = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_ifssrloaded, LV_EVENT_SCREEN_LOADED, (void *)0);
    lv_obj_add_event_cb(obj, ssr_double_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj87 = obj;
            lv_obj_set_pos(obj, -11, -10);
            lv_obj_set_size(obj, 503, 60);
            lv_obj_add_event_cb(obj, ssr_double_tap_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff460096), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.tobj88 = obj;
            lv_obj_set_pos(obj, 219, 10);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "SSR");
        }
        {
            lv_obj_t *obj = lv_keyboard_create(parent_obj);
            t_objects.tobj89 = obj;
            lv_obj_set_pos(obj, 191, 50);
            lv_obj_set_size(obj, 178, 264);
            lv_keyboard_set_map(obj, LV_KEYBOARD_MODE_USER_1, ssr_kb_map, ssr_kb_ctrl);
            lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_USER_1);
            lv_obj_add_event_cb(obj, ssr_kb_ready_cb, LV_EVENT_READY, NULL);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff7d7d7d), LV_PART_MAIN | LV_STATE_DEFAULT);
            style_ssr_keyboard_action_keys(obj);
        }
        {
            // pwm_time_text_tarea
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            t_objects.pwm_time_text_tarea = obj;
            lv_obj_set_pos(obj, 12, 101);
            lv_obj_set_size(obj, 162, 42);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, event_handler_cb_ssr_pwm_time_text_tarea, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc5c5c5), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj91 = obj;
            lv_obj_set_pos(obj, 376, 56);
            lv_obj_set_size(obj, 90, 58);
            lv_obj_add_event_cb(obj, action_pwm_start, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff16a34a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.tobj92 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "START");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj93 = obj;
            lv_obj_set_pos(obj, 376, 122);
            lv_obj_set_size(obj, 90, 58);
            lv_obj_add_event_cb(obj, action_pwm_stop, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffdc2626), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb91c1c), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    t_objects.tobj94 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "STOP");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            t_objects.tobj95 = obj;
            lv_obj_set_pos(obj, 12, 56);
            lv_obj_set_size(obj, 166, 38);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff58636c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "ON ms        OFF ms");
                }
            }
        }
        {
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            ssr_on_ta = obj;
            lv_obj_set_pos(obj, 12, 101);
            lv_obj_set_size(obj, 78, 42);
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_max_length(obj, 6);
            lv_textarea_set_text(obj, "800");
            lv_obj_add_event_cb(obj, ssr_field_event_cb, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc5c5c5), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            ssr_off_ta = obj;
            lv_obj_set_pos(obj, 100, 101);
            lv_obj_set_size(obj, 78, 42);
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_max_length(obj, 6);
            lv_textarea_set_text(obj, "200");
            lv_obj_add_event_cb(obj, ssr_field_event_cb, LV_EVENT_ALL, 0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc5c5c5), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // کادر سه دما (عریض‌تر، از x=10 تا 184 جهت تفکیک کامل اعداد از °C)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            t_objects.tobj96 = obj;
            lv_obj_set_pos(obj, 10, 155);
            lv_obj_set_size(obj, 174, 88);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff2f2f2), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 18, 163);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "esp :");
        }
        {
            // esptssr
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.esptssr = obj;
            lv_obj_set_pos(obj, 58, 163);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "000");
        }
        {
            // esptssr_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.esptssr_1 = obj;
            lv_obj_set_pos(obj, 146, 163);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "°C");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 18, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "t1 :");
        }
        {
            // t1ssr
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.t1ssr = obj;
            lv_obj_set_pos(obj, 58, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "000");
        }
        {
            // esptssr_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.esptssr_2 = obj;
            lv_obj_set_pos(obj, 146, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "°C");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 18, 213);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "t2 :");
        }
        {
            // t2ssr
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.t2ssr = obj;
            lv_obj_set_pos(obj, 58, 213);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "000");
        }
        {
            // esptssr_3
            lv_obj_t *obj = lv_label_create(parent_obj);
            t_objects.esptssr_3 = obj;
            lv_obj_set_pos(obj, 146, 213);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "°C");
        }
        {
            // لامپ سیگنال خروجی المنت SSR با طراحی کپسولی شکیل و تراز با کلیدهای استارت و استپ (عرض ۹۰ در x=376)
            lv_obj_t *badge = lv_obj_create(parent_obj);
            lv_obj_set_pos(badge, 376, 10);
            lv_obj_set_size(badge, 90, 32);
            lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(badge, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(badge, lv_color_hex(0xff334155), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(badge, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            // چراغ ال‌ای‌دی دایره‌ای در داخل کپسول
            lv_obj_t *led = lv_obj_create(badge);
            t_objects.ssr_signal_lamp = led;
            lv_obj_set_pos(led, 8, 8);
            lv_obj_set_size(led, 16, 16);
            lv_obj_set_style_radius(led, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(led, lv_color_hex(0xff374151), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(led, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(led, lv_color_hex(0xff6b7280), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(led, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            // لیبل وضعیت خروجی
            lv_obj_t *lbl = lv_label_create(badge);
            s_ssr_lamp_lbl = lbl;
            lv_obj_set_pos(lbl, 28, 7);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(lbl, "STOP");
        }
    }
    lv_keyboard_set_textarea(t_objects.tobj89, ssr_on_ta);
    if (ssr_on_ta) {
        lv_obj_set_style_border_color(ssr_on_ta, lv_color_hex(0xff0284c7), 0);
        lv_obj_set_style_border_width(ssr_on_ta, 2, 0);
        s_ssr_field_fresh = true;
        const char *t = lv_textarea_get_text(ssr_on_ta);
        s_ssr_field_initial_len = t ? (int)strlen(t) : 0;
    }
    if (ssr_off_ta) {
        lv_obj_set_style_border_color(ssr_off_ta, lv_color_hex(0xff94a3b8), 0);
        lv_obj_set_style_border_width(ssr_off_ta, 1, 0);
    }
    ssr_commit_time_text();
    
    add_test_top_back_button(t_objects.ssr);
    tick_screen_ssr();
}

void tick_screen_ssr() {
    {
        const char *new_val = get_var_time_text();
        const char *cur_val = lv_textarea_get_text(t_objects.pwm_time_text_tarea);
        uint32_t max_length = lv_textarea_get_max_length(t_objects.pwm_time_text_tarea);
        if (strncmp(new_val, cur_val, max_length) != 0) {
            t_tick_value_change_obj = t_objects.pwm_time_text_tarea;
            lv_textarea_set_text(t_objects.pwm_time_text_tarea, new_val);
            t_tick_value_change_obj = NULL;
        }
    }
    if (ssr_on_ta && ssr_off_ta) {
        bool on_focused = (t_objects.tobj89 && lv_keyboard_get_textarea(t_objects.tobj89) == ssr_on_ta);
        bool off_focused = (t_objects.tobj89 && lv_keyboard_get_textarea(t_objects.tobj89) == ssr_off_ta);
        int on_ms, off_ms;
        const char *txt = get_var_time_text();
        if (txt && sscanf(txt, "%d.%d", &on_ms, &off_ms) == 2) {
            char b[16];
            if (!on_focused) {
                snprintf(b, sizeof(b), "%d", on_ms);
                if (strcmp(lv_textarea_get_text(ssr_on_ta), b) != 0) {
                    lv_textarea_set_text(ssr_on_ta, b);
                    s_last_valid_ssr_on = on_ms;
                }
            }
            if (!off_focused) {
                snprintf(b, sizeof(b), "%d", off_ms);
                if (strcmp(lv_textarea_get_text(ssr_off_ta), b) != 0) {
                    lv_textarea_set_text(ssr_off_ta, b);
                    s_last_valid_ssr_off = off_ms;
                }
            }
        }
    }

    // به‌روزرسانی زنده لامپ سیگنال المنت SSR بر اساس وضعیت واقعی پین خروجی و زمان‌بندی PWM
    if (t_objects.ssr_signal_lamp && lv_obj_is_valid(t_objects.ssr_signal_lamp)) {
        extern bool hwtest_get_pwm_output_state(void);
        extern bool hwtest_get_pwm_running(void);
        bool running = hwtest_get_pwm_running();
        bool out_active = hwtest_get_pwm_output_state();

        static int8_t s_last_lamp_state = -1;
        int8_t cur_state = !running ? 0 : (out_active ? 2 : 1);

        if (cur_state != s_last_lamp_state) {
            s_last_lamp_state = cur_state;
            if (cur_state == 2) {
                // روشن (ON): سبز درخشان
                lv_obj_set_style_bg_color(t_objects.ssr_signal_lamp, lv_color_hex(0xff22c55e), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(t_objects.ssr_signal_lamp, lv_color_hex(0xff86efac), LV_PART_MAIN | LV_STATE_DEFAULT);
                if (s_ssr_lamp_lbl && lv_obj_is_valid(s_ssr_lamp_lbl)) {
                    lv_obj_set_style_text_color(s_ssr_lamp_lbl, lv_color_hex(0xff4ade80), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(s_ssr_lamp_lbl, "OUT ON");
                }
            } else if (cur_state == 1) {
                // فاز خاموش چرخه (Cycle OFF): نارنجی
                lv_obj_set_style_bg_color(t_objects.ssr_signal_lamp, lv_color_hex(0xffea580c), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(t_objects.ssr_signal_lamp, lv_color_hex(0xfffdba74), LV_PART_MAIN | LV_STATE_DEFAULT);
                if (s_ssr_lamp_lbl && lv_obj_is_valid(s_ssr_lamp_lbl)) {
                    lv_obj_set_style_text_color(s_ssr_lamp_lbl, lv_color_hex(0xfffb923c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(s_ssr_lamp_lbl, "OUT OFF");
                }
            } else {
                // متوقف شده (STOP): خاکستری
                lv_obj_set_style_bg_color(t_objects.ssr_signal_lamp, lv_color_hex(0xff374151), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(t_objects.ssr_signal_lamp, lv_color_hex(0xff6b7280), LV_PART_MAIN | LV_STATE_DEFAULT);
                if (s_ssr_lamp_lbl && lv_obj_is_valid(s_ssr_lamp_lbl)) {
                    lv_obj_set_style_text_color(s_ssr_lamp_lbl, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(s_ssr_lamp_lbl, "STOP");
                }
            }
        }
    }
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t t_tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_setup,
    tick_screen_rs485,
    tick_screen_gsm,
    tick_screen_bluetooth,
    tick_screen_wifi,
    tick_screen_display_w,
    tick_screen_display_b,
    tick_screen_display_test_r,
    tick_screen_display_g,
    tick_screen_display_blue,
    tick_screen_sys_info,
    tick_screen_touch_test,
    tick_screen_clock_setup,
    tick_screen_ssr,
};
void t_tick_screen(int screen_index) {
    t_tick_screen_funcs[screen_index]();
}
void t_tick_screen_by_id(enum T_ScreensEnum screenId) {
    t_tick_screen_funcs[screenId - 1]();
}

void t_create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
    create_screen_setup();
    create_screen_rs485();
    create_screen_gsm();
    create_screen_bluetooth();
    create_screen_wifi();
    create_screen_display_w();
    create_screen_display_b();
    create_screen_display_test_r();
    create_screen_display_g();
    create_screen_display_blue();
    create_screen_sys_info();
    create_screen_touch_test();
    create_screen_clock_setup();
    create_screen_ssr();
}
