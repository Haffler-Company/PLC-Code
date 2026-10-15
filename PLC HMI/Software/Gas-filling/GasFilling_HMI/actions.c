#include "actions.h"
#include "screens.h"
#include "ui.h"
#include "stdlib.h"
#include "stdio.h"
#include "variables.h"
#include "a2_eeprom.h"
#include "ui_breath.h"
#include "ui_vazir_fonts.h"
#include "fonts.h"
#include "day_lock.h"
#include "warranty_mgr.h"
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

/***********************MyAction on .h***********************/
// void calculate_all();
// void ChangeBar(unsigned char BarValue);
// void ChangeBar1(unsigned char BarValue);
// void HideLogo(void);
// void Set_SWTypeOutPut(bool type)
// void ChangeBuzzerAvailble(bool Active_Buzzer);
// void ChangeLanguage(char language)
/***********************MyAction***********************/

static void update_tabview_titles(const char *t0, const char *t1, const char *t2, const char *t3)
{
    (void)t3;
    if (!objects.tabview_setting) return;
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.tabview_setting);
    if (!tab_bar) return;
    const char *titles[3] = {t0, t1, t2};
    uint32_t cnt = lv_obj_get_child_count(tab_bar);
    for (uint32_t i = 0; i < cnt && i < 3; i++) {
        lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
        if (btn) {
            uint32_t btn_cnt = lv_obj_get_child_count(btn);
            for (uint32_t j = 0; j < btn_cnt; j++) {
                lv_obj_t *lbl = lv_obj_get_child(btn, j);
                if (lbl && lv_obj_check_type(lbl, &lv_label_class)) {
                    lv_label_set_text(lbl, titles[i]);
                }
            }
        }
    }
}

static void ui_keyboard_detach(lv_obj_t *kb)
{
    if (!kb) return;
    lv_obj_t *ta = lv_keyboard_get_textarea(kb);
    lv_keyboard_set_textarea(kb, NULL);
    if (ta) lv_obj_clear_state(ta, LV_STATE_FOCUSED);
}

static void ui_open_main_keyboard(lv_obj_t *textarea, int translate_x)
{
    if (!objects.key_board_panle || !objects.kb_key_board || !textarea) return;
    lv_obj_set_style_translate_x(objects.key_board_panle, translate_x, LV_PART_MAIN);
    lv_obj_clear_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
    /* Unit overlays are created later than the keyboard; bring the modal back on top. */
    lv_obj_move_foreground(objects.key_board_panle);
    lv_keyboard_set_textarea(objects.kb_key_board, textarea);
}

static void ui_clear_dimensions(uint8_t station)
{
    lv_obj_t *f[3];
    if (station == 0) {
        f[0] = objects.ta_length; f[1] = objects.ta_depth; f[2] = objects.ta_heigth;
        TimerValve1 = TimerValve1_Saved = 0;
        ui_main_cancel_bar_reset(0);
        ChangeBar(0);
        if (objects.t_target_value) lv_label_set_text(objects.t_target_value, "0 s");
        EEprom_Glass2_Length = 0.0f;
        EEprom_Glass2_Height = 0.0f;
        EEprom_Glass2_Depth = 0.0f;
    } else {
        f[0] = objects.ta_length1; f[1] = objects.ta_depth1; f[2] = objects.ta_heigth1;
        TimerValve2 = TimerValve2_Saved = 0;
        ui_main_cancel_bar_reset(1);
        ChangeBar1(0);
        if (objects.t_target_value1) lv_label_set_text(objects.t_target_value1, "0 s");
        EEprom_Glass1_Length = 0.0f;
        EEprom_Glass1_Height = 0.0f;
        EEprom_Glass1_Depth = 0.0f;
    }
    for (uint8_t i = 0; i < 3; i++) if (f[i]) lv_textarea_set_text(f[i], "");
    if (objects.key_board_panle) {
        lv_obj_add_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
        ui_keyboard_detach(objects.kb_key_board);
    }
    a2_eeprom_write_all();
    calculate_all();
    buzzer_key;
}
void action_clear_dimensions_left(lv_event_t *e) { (void)e; ui_clear_dimensions(0); }
void action_clear_dimensions_right(lv_event_t *e) { (void)e; ui_clear_dimensions(1); }

void ui_gotosScreen(enum ScreensEnum screenId)
{
    loadScreen(screenId);
}

/* Loading a screen from inside LV_EVENT_RELEASED can invalidate the active
   event target on LVGL 9.  Defer it until the touch event has completed. */
static void ui_open_settings_async(void *user_data)
{
    (void)user_data;
    ui_gotosScreen(SCREEN_ID_SETTING);
}

static void ui_open_main_async(void *user_data)
{
    (void)user_data;
    ui_gotosScreen(SCREEN_ID_MAIN);
}
void action_select_as_key_board_earia_l(lv_event_t *e)
{
    ui_open_main_keyboard(objects.ta_length, 238);
    buzzer_key;//BuzzerKeyStart();

}
void action_select_as_key_board_earia_h(lv_event_t *e)
{
    ui_open_main_keyboard(objects.ta_heigth, 238);
    buzzer_key;//BuzzerKeyStart();

}
void action_select_as_key_board_earia_d(lv_event_t *e)
{
    ui_open_main_keyboard(objects.ta_depth, 238);
    buzzer_key;//BuzzerKeyStart();

}
void action_select_as_key_board_earia_l1(lv_event_t *e)
{
    ui_open_main_keyboard(objects.ta_length1, 0);
    buzzer_key;//BuzzerKeyStart();

}
void action_select_as_key_board_earia_h1(lv_event_t *e)
{
    ui_open_main_keyboard(objects.ta_heigth1, 0);
    buzzer_key;//BuzzerKeyStart();

}
void action_select_as_key_board_earia_d1(lv_event_t *e)
{
    ui_open_main_keyboard(objects.ta_depth1, 0);
    buzzer_key;//BuzzerKeyStart();

}

void action_start_button_pressed(lv_event_t *e)
{
    if (stop_input_is_active()) {
        buzzer_key;
        return;
    }
    if (device_locked || day_lock_is_soft_locked()) {
        dev_lock_ui_show();
        buzzer_key;
        return;
    }
    float equal = 0;
    calculate_all();
    if (TimerValve1_Saved == 0) {
        ChangeBar(0);
        ui_main_show_station_status(0, UI_MAIN_STATUS_INPUT_ERROR);
        buzzer_key;
        return;
    }
    ui_main_cancel_bar_reset(0);
    TimerValve1 = TimerValve1_Saved;
    equal = TimerValve1;
    equal = equal / TimerValve1_Saved;
    equal = equal *100;
    ChangeBar(100 - equal);
    ui_main_show_station_status(0, UI_MAIN_STATUS_INJECTING);
    q2_alert_start();
}
void action_stop_button_pressed(lv_event_t *e)
{
    ui_main_cancel_bar_reset(0);
    TimerValve1 = 0; 
    HwSetValve1(false);
    ChangeBar(0);
    ui_main_show_station_status(0, UI_MAIN_STATUS_STOPPED);
    buzzer_key;
}
void action_start_button_pressed1(lv_event_t *e)
{
    if (stop_input_is_active()) {
        buzzer_key;
        return;
    }
    if (device_locked || day_lock_is_soft_locked()) {
        dev_lock_ui_show();
        buzzer_key;
        return;
    }
    float equal = 0;
    calculate_all();
    if (TimerValve2_Saved == 0) {
        ChangeBar1(0);
        ui_main_show_station_status(1, UI_MAIN_STATUS_INPUT_ERROR);
        buzzer_key;
        return;
    }
    ui_main_cancel_bar_reset(1);
    TimerValve2 = TimerValve2_Saved;
    equal = TimerValve2;
    equal = equal / TimerValve2_Saved;
    equal = equal *100;
    ChangeBar1(100 - equal);
    ui_main_show_station_status(1, UI_MAIN_STATUS_INJECTING);
    q2_alert_start();
}
void action_stop_button_pressed1(lv_event_t *e)
{
    ui_main_cancel_bar_reset(1);
    TimerValve2 = 0;
    HwSetValve2(false);
    ChangeBar1(0);
    ui_main_show_station_status(1, UI_MAIN_STATUS_STOPPED);
    buzzer_key;
}

/* Open Setting immediately — no imgbtn transform/anim (LVGL9 hung with zoom/rotation). */
void action_gotosetting(lv_event_t *e)
{
    (void)e;
    lv_obj_add_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
    ui_keyboard_detach(objects.kb_key_board);
    lv_async_call(ui_open_settings_async, NULL);
    buzzer_key;
}
void action_gotohome(lv_event_t *e)
{
    // loadScreen(SCREEN_ID_MAIN);
    ui_breath_on_leave();
    lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
    ui_keyboard_detach(objects.kb_key_board_1);
    if (TimerValve1 == 0 && TimerValve2 == 0) {
        HwSetValve1(false);
        HwSetValve2(false);
        HwSetBuzzer(false);
    }
    lv_async_call(ui_open_main_async, NULL);
    buzzer_key;//BuzzerKeyStart();
}

void action_select_as_key_board_earia_p(lv_event_t *e)
{
    buzzer_key;//BuzzerKeyStart();
}

void action_select_as_key_board_earia_p1(lv_event_t *e)
{
    if (objects.kb_key_board_1 && objects.ta_p1) {
        lv_keyboard_set_textarea(objects.kb_key_board_1, objects.ta_p1);
    }
    buzzer_key;//BuzzerKeyStart();
}
void action_select_as_key_board_earia_p2(lv_event_t *e)
{
    if (objects.kb_key_board_1 && objects.ta_p2) {
        lv_keyboard_set_textarea(objects.kb_key_board_1, objects.ta_p2);
    }
}
void action_select_as_key_board_earia_qs(lv_event_t *e)
{
    if (objects.key_board_panle_setting) lv_obj_clear_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
    if (objects.kb_key_board_1 && objects.ta_qs) {
        lv_keyboard_set_textarea(objects.kb_key_board_1, objects.ta_qs);
    }
    buzzer_key;//BuzzerKeyStart();
}


static bool ui_read_valid_number(lv_obj_t *textarea, float minimum, float maximum, float *value)
{
    const char *text;
    char *end;
    float parsed;
    bool valid;
    if (!textarea || !value) return false;

    text = lv_textarea_get_text(textarea);
    errno = 0;
    parsed = strtof(text ? text : "", &end);
    while (end && *end && isspace((unsigned char)*end)) end++;
    valid = text && text[0] && end != text && end && *end == '\0' && errno == 0 &&
            isfinite(parsed) && parsed >= minimum && parsed <= maximum;

    lv_obj_set_style_border_width(textarea, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(textarea,
        lv_color_hex(valid ? 0xffdedede : 0xffe53935), LV_PART_MAIN);
    lv_obj_set_style_bg_color(textarea,
        lv_color_hex(valid ? 0xffffffff : 0xffffebee), LV_PART_MAIN);
    *value = valid ? parsed : 0.0f;
    return valid;
}

void a2_eeprom_load_to_ui(void)
{
    char buf[32];
    if (objects.ta_qs) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Process_Qs);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_qs, buf);
    }
    if (objects.ta_p1) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Process_P1);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_p1, buf);
    }
    if (objects.ta_p2) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Process_P2);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_p2, buf);
    }

    if (objects.ta_length && EEprom_Glass2_Length > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Glass2_Length);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_length, buf);
    }
    if (objects.ta_heigth && EEprom_Glass2_Height > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Glass2_Height);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_heigth, buf);
    }
    if (objects.ta_depth && EEprom_Glass2_Depth > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Glass2_Depth);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_depth, buf);
    }

    if (objects.ta_length1 && EEprom_Glass1_Length > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Glass1_Length);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_length1, buf);
    }
    if (objects.ta_heigth1 && EEprom_Glass1_Height > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Glass1_Height);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_heigth1, buf);
    }
    if (objects.ta_depth1 && EEprom_Glass1_Depth > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f", EEprom_Glass1_Depth);
        char *dot = strchr(buf, '.');
        if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
        lv_textarea_set_text(objects.ta_depth1, buf);
    }

    calculate_all();
}

void a2_eeprom_save_dimensions_from_ui(void)
{
    float val;
    if (objects.ta_length && ui_read_valid_number(objects.ta_length, 1.0f, 1000.0f, &val)) {
        EEprom_Glass2_Length = val;
    }
    if (objects.ta_heigth && ui_read_valid_number(objects.ta_heigth, 1.0f, 1000.0f, &val)) {
        EEprom_Glass2_Height = val;
    }
    if (objects.ta_depth && ui_read_valid_number(objects.ta_depth, 0.01f, 100.0f, &val)) {
        EEprom_Glass2_Depth = val;
    }

    if (objects.ta_length1 && ui_read_valid_number(objects.ta_length1, 1.0f, 1000.0f, &val)) {
        EEprom_Glass1_Length = val;
    }
    if (objects.ta_heigth1 && ui_read_valid_number(objects.ta_heigth1, 1.0f, 1000.0f, &val)) {
        EEprom_Glass1_Height = val;
    }
    if (objects.ta_depth1 && ui_read_valid_number(objects.ta_depth1, 0.01f, 100.0f, &val)) {
        EEprom_Glass1_Depth = val;
    }
    a2_eeprom_write_all();
}

void a2_eeprom_save_process_from_ui(void)
{
    float val;
    if (objects.ta_qs && ui_read_valid_number(objects.ta_qs, 0.01f, 500.0f, &val)) {
        EEprom_Process_Qs = val;
    }
    if (objects.ta_p1 && ui_read_valid_number(objects.ta_p1, 0.01f, 100.0f, &val)) {
        EEprom_Process_P1 = val;
    }
    if (objects.ta_p2 && ui_read_valid_number(objects.ta_p2, 0.01f, 100.0f, &val)) {
        EEprom_Process_P2 = val;
    }
    a2_eeprom_write_all();
}

void action_save_button_pressed(lv_event_t * e)
{
    (void)e;
    buzzer_key;
    a2_eeprom_save_process_from_ui();
    calculate_all();
    lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
    ui_keyboard_detach(objects.kb_key_board_1);
}

static void set_label_if_changed(lv_obj_t *label, const char *text)
{
    if (!label || !text) return;
    const char *cur = lv_label_get_text(label);
    if (cur && strcmp(cur, text) == 0) return;
    lv_label_set_text(label, text);
}

void calculate_all()
{
    float float_L = 0.0f;
    float float_H = 0.0f;
    float float_W = 0.0f;
    float float_L1 = 0.0f;
    float float_H1 = 0.0f;
    float float_W1 = 0.0f;
    float float_P1 = (EEprom_Process_P1 > 0.001f) ? EEprom_Process_P1 : 1.0f;
    float float_P2 = (EEprom_Process_P2 > 0.001f) ? EEprom_Process_P2 : 1.0f;
    float float_Qs = (EEprom_Process_Qs > 0.001f) ? EEprom_Process_Qs : 30.0f;

    float V1,V2,t;
    float V11,V22,t1;
    char buffer_string[32];

    /* Practical glass-entry limits in centimetres. */
    bool left_l_ok = ui_read_valid_number(objects.ta_length, 1.0f, 1000.0f, &float_L);
    bool left_h_ok = ui_read_valid_number(objects.ta_heigth, 1.0f, 1000.0f, &float_H);
    bool left_w_ok = ui_read_valid_number(objects.ta_depth, 0.01f, 100.0f, &float_W);
    bool right_l_ok = ui_read_valid_number(objects.ta_length1, 1.0f, 1000.0f, &float_L1);
    bool right_h_ok = ui_read_valid_number(objects.ta_heigth1, 1.0f, 1000.0f, &float_H1);
    bool right_w_ok = ui_read_valid_number(objects.ta_depth1, 0.01f, 100.0f, &float_W1);
    bool left_ok = left_l_ok && left_h_ok && left_w_ok;
    bool right_ok = right_l_ok && right_h_ok && right_w_ok;

    /* Read Qs / P1 / P2 if textareas exist, otherwise use stored EEPROM values */
    if (objects.ta_qs) {
        float q;
        if (ui_read_valid_number(objects.ta_qs, 0.01f, 500.0f, &q)) float_Qs = q;
    }
    if (objects.ta_p1) {
        float p;
        if (ui_read_valid_number(objects.ta_p1, 0.01f, 100.0f, &p)) float_P1 = p;
    }
    if (objects.ta_p2) {
        float p;
        if (ui_read_valid_number(objects.ta_p2, 0.01f, 100.0f, &p)) float_P2 = p;
    }

    V1 = left_ok ? float_L * float_H * float_W : 0.0f;
    V1 = V1 / 1000;
    V11 = right_ok ? float_L1 * float_H1 * float_W1 : 0.0f;
    V11 = V11 / 1000;

    t = 0;
    t1 = 0;
    if (float_P2 > 0.0001f && float_Qs > 0.0001f) {
        V2 = (float_P1 / float_P2) * V1;
        V22 = (float_P1 / float_P2) * V11;
        t = (V2 / float_Qs) * 60.0f;
        t1 = (V22 / float_Qs) * 60.0f;
    }

    if (!left_ok || !isfinite(t) || t <= 0.0f || t > ((float)UINT_MAX / 10.0f)) t = 0.0f;
    if (!right_ok || !isfinite(t1) || t1 <= 0.0f || t1 > ((float)UINT_MAX / 10.0f)) t1 = 0.0f;

    /* Siemens Industrial Safety: Cap max injection time to 180 seconds to prevent argon tank depletion */
    const float MAX_INJECTION_TIME_SEC = 180.0f;
    if (t > MAX_INJECTION_TIME_SEC) t = MAX_INJECTION_TIME_SEC;
    if (t1 > MAX_INJECTION_TIME_SEC) t1 = MAX_INJECTION_TIME_SEC;

    TimerValve1_Saved = t > 0.0f ? (unsigned int)(t * 10.0f + 0.5f) : 0;
    TimerValve2_Saved = t1 > 0.0f ? (unsigned int)(t1 * 10.0f + 0.5f) : 0;

    snprintf(buffer_string, sizeof(buffer_string), "%.0f s", t);
    set_label_if_changed(objects.t_target_value, buffer_string);

    snprintf(buffer_string, sizeof(buffer_string), "%.0f s", t1);
    set_label_if_changed(objects.t_target_value1, buffer_string);
}

void action_keyboard_pressed(lv_event_t * e)
{
    calculate_all();
    buzzer_key;//BuzzerKeyStart();


}

/* Side bars: cold blue (0%) → yellow (50%) → warm red (100%). */
void action_dimension_value_changed(lv_event_t * e)
{
    (void)e;
    calculate_all();
}

static lv_color_t ui_bar_fill_color(unsigned char pct)
{
    uint8_t r, g, b;
    if (pct <= 50) {
        r = (uint8_t)(30 + ((249 - 30) * pct) / 50);
        g = (uint8_t)(136 + ((168 - 136) * pct) / 50);
        b = (uint8_t)(229 - ((229 - 37) * pct) / 50);
    } else {
        unsigned char p = (unsigned char)(pct - 50);
        r = (uint8_t)(249 - ((249 - 229) * p) / 50);
        g = (uint8_t)(168 - ((168 - 57) * p) / 50);
        b = (uint8_t)(37 - ((37 - 53) * p) / 50);
    }
    return lv_color_make(r, g, b);
}

static void ui_set_progress_fill(lv_obj_t *fill, unsigned char pct)
{
    const int full_height = 146;
    int height;
    if (!fill) return;
    height = (full_height * pct) / 100;
    if (pct && height == 0) height = 1;
    lv_obj_set_pos(fill, 0, full_height - height);
    lv_obj_set_size(fill, 34, height);
    lv_obj_set_style_bg_color(fill, ui_bar_fill_color(pct), LV_PART_MAIN);
}

static void ui_set_start_running(uint8_t station)
{
    static int8_t last[2] = { -1, -1 };
    static unsigned char last_language[2] = { 255, 255 };
    bool running = station == 0 ? TimerValve1 != 0 : TimerValve2 != 0;
    if (last[station] == (int8_t)running && last_language[station] == EEprom2_Language) return;
    last[station] = (int8_t)running;
    last_language[station] = EEprom2_Language;
    lv_obj_t *btn = station == 0 ? objects.obj0 : objects.obj1;
    lv_obj_t *text = station == 0 ? objects.t_start_l : objects.t_start_r;
    lv_obj_t *fields[3] = {
        station == 0 ? objects.ta_length : objects.ta_length1,
        station == 0 ? objects.ta_depth : objects.ta_depth1,
        station == 0 ? objects.ta_heigth : objects.ta_heigth1
    };
    lv_obj_t *clear_btn = station == 0 ? objects.bt_clear_left : objects.bt_clear_right;
    if (!btn || !text) return;
    lv_obj_t *icon = lv_obj_get_child(btn, 0);
    if (running) {
        for (uint8_t i = 0; i < 3; i++) if (fields[i]) lv_obj_add_state(fields[i], LV_STATE_DISABLED);
        if (clear_btn) lv_obj_add_state(clear_btn, LV_STATE_DISABLED);
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xff546e7a), LV_PART_MAIN);
        lv_obj_set_style_pad_hor(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_column(btn, 0, LV_PART_MAIN);
        if (icon) {
            lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
        }
        if (EEprom2_Language == L_english) {
            lv_obj_set_style_text_font(text, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_label_set_text(text, "Injecting");
        } else if (EEprom2_Language == L_Turkish) {
            lv_obj_set_style_text_font(text, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_label_set_text(text, "Dolum");
        } else if (EEprom2_Language == L_Russian) {
            lv_obj_set_style_text_font(text, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_label_set_text(text, "Zapravka");
        } else {
            lv_obj_set_style_text_font(text, ui_font_vazir_or(&ui_font_arial_regular17, 11), LV_PART_MAIN);
            lv_label_set_text(text, "در حال تزریق");
        }
    } else {
        for (uint8_t i = 0; i < 3; i++) if (fields[i]) lv_obj_clear_state(fields[i], LV_STATE_DISABLED);
        if (clear_btn) lv_obj_clear_state(clear_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xff16a34a), LV_PART_MAIN);
        lv_obj_set_style_pad_hor(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_column(btn, 6, LV_PART_MAIN);
        if (icon) {
            lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(icon, LV_SYMBOL_PLAY);
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        }
        if (EEprom2_Language == L_english) {
            lv_obj_set_style_text_font(text, &lv_font_montserrat_16, LV_PART_MAIN);
            lv_label_set_text(text, "Start");
        } else if (EEprom2_Language == L_Turkish) {
            lv_obj_set_style_text_font(text, &lv_font_montserrat_16, LV_PART_MAIN);
            lv_label_set_text(text, "Baslat");
        } else if (EEprom2_Language == L_Russian) {
            lv_obj_set_style_text_font(text, &lv_font_montserrat_16, LV_PART_MAIN);
            lv_label_set_text(text, "Pusk");
        } else {
            lv_obj_set_style_text_font(text, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
            lv_label_set_text(text, "شروع");
        }
    }
}

void ChangeBar(unsigned char BarValue)
{
    if (BarValue > 100) BarValue = 100;
    static unsigned char s_last_bar0 = 255;
    if (BarValue == s_last_bar0) return;
    s_last_bar0 = BarValue;

    char buffer_val[8];
    snprintf(buffer_val, sizeof(buffer_val), "%u%%", (unsigned)BarValue);
    if (objects.t_target_percent) {
        lv_label_set_text(objects.t_target_percent, buffer_val);
        lv_obj_set_style_text_color(objects.t_target_percent, (BarValue > 30) ? lv_color_white() : lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_obj_align(objects.t_target_percent, LV_ALIGN_CENTER, 0, 0);
        lv_obj_move_foreground(objects.t_target_percent);
    }
    if (objects.bar_gaspercent) {
        ui_set_progress_fill(objects.bar_gaspercent, BarValue);
    }
    ui_set_start_running(0);
}
void ChangeBar1(unsigned char BarValue)
{
    if (BarValue > 100) BarValue = 100;
    static unsigned char s_last_bar1 = 255;
    if (BarValue == s_last_bar1) return;
    s_last_bar1 = BarValue;

    char buffer_val[8];
    snprintf(buffer_val, sizeof(buffer_val), "%u%%", (unsigned)BarValue);
    if (objects.t_target_percent1) {
        lv_label_set_text(objects.t_target_percent1, buffer_val);
        lv_obj_set_style_text_color(objects.t_target_percent1, (BarValue > 30) ? lv_color_white() : lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_obj_align(objects.t_target_percent1, LV_ALIGN_CENTER, 0, 0);
        lv_obj_move_foreground(objects.t_target_percent1);
    }
    if (objects.bar_gaspercent1) {
        ui_set_progress_fill(objects.bar_gaspercent1, BarValue);
    }
    ui_set_start_running(1);
}
void HideLogo()
{
    if (!objects.hafler_panle) return;
    lv_obj_add_flag(objects.hafler_panle, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.hafler_panle, LV_OBJ_FLAG_CLICKABLE);
}

void action_enter_pressd_1(lv_event_t * e)
{
    action_close_main_keyboard(e);
}
void action_close_main_keyboard(lv_event_t * e)
{
    (void)e;
    buzzer_key;
    calculate_all();
    a2_eeprom_save_dimensions_from_ui();
    lv_obj_add_flag(objects.key_board_panle, LV_OBJ_FLAG_HIDDEN);
    ui_keyboard_detach(objects.kb_key_board);
}
void action_tab_pressed1(lv_event_t * e)
{

}

void action_buzzer_sw_pressed(lv_event_t * e)
{
    if ((lv_obj_get_state(objects.sw_buzzer) & LV_STATE_CHECKED)) {
        buzzerEnable = 1;
        EEprom3_Buzzer = true;
        buzzerTimeVlaue = 2; /* preview Sound on (ignore touch-feedback gate) */
    } else {
        buzzerEnable = 0;
        EEprom3_Buzzer = false;
    }
    a2_eeprom_write_all();
}
void ChangeBuzzerAvailble(bool Active_Buzzer)
{
    if (objects.sw_buzzer) {
        if (Active_Buzzer) {
            lv_obj_add_state(objects.sw_buzzer, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(objects.sw_buzzer, LV_STATE_CHECKED);
        }
    }
    buzzerEnable = Active_Buzzer ? 1 : 0;
}
void ChangeTouchFeedback(bool Active_Touch)
{
    if (objects.sw_touch) {
        if (Active_Touch) {
            lv_obj_add_state(objects.sw_touch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(objects.sw_touch, LV_STATE_CHECKED);
        }
    }
    touchFeedbackEnable = Active_Touch ? 1 : 0;
}
void action_touch_sw_pressed(lv_event_t * e)
{
    if (!objects.sw_touch) return;
    if ((lv_obj_get_state(objects.sw_touch) & LV_STATE_CHECKED)) {
        touchFeedbackEnable = 1;
        EEprom4_TouchAvailable = true;
        if (buzzerEnable) buzzerTimeVlaue = 2; /* preview */
    } else {
        touchFeedbackEnable = 0;
        EEprom4_TouchAvailable = false;
    }
    a2_eeprom_write_all();
}
void action_type_out_sw_pressed(lv_event_t * e)
{
    //lv_label_set_text(obj, "N.C");
    if (objects.key_board_panle_setting) lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
    if (!objects.sw_type_out_put) return;
    if (!(lv_obj_get_state(objects.sw_type_out_put) & LV_STATE_CHECKED))
    {    
        if (objects.t_type_output) lv_label_set_text(objects.t_type_output, "N.O");
        vr_OutPutType = 0;
    }
    else 
    {    
        if (objects.t_type_output) lv_label_set_text(objects.t_type_output, "N.C");
        vr_OutPutType = 1;
    }
    EEprom7_OutputType = (unsigned char)vr_OutPutType;
    a2_eeprom_write_all();
    ApplyOutputTypeSafe(vr_OutPutType);
    buzzer_key;
}
void Set_SWTypeOutPut(char type)
{
    if (objects.t_type_output) {
        lv_label_set_text(objects.t_type_output, type ? "N.C" : "N.O");
    }
    if (objects.sw_type_out_put) {
        if (type) {
            lv_obj_add_state(objects.sw_type_out_put, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(objects.sw_type_out_put, LV_STATE_CHECKED);
        }
    }
}
void action_valve1_sw_pressed(lv_event_t * e)
{
    buzzer_key;//BuzzerKeyStart();
}
void action_valve2_sw_pressed(lv_event_t * e)
{
    buzzer_key;//BuzzerKeyStart();
}
void action_buzzer_out_sw_pressed(lv_event_t * e)
{
    buzzer_key;//BuzzerKeyStart();
}
void action_buzzer_pressed(lv_event_t * e)
{
    buzzer_key;//BuzzerKeyStart();
    lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
}
void action_enter_pressed_setting(lv_event_t * e)
{
    lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
    ui_keyboard_detach(objects.kb_key_board_1);
    buzzer_key;//BuzzerKeyStart();
    a2_eeprom_save_process_from_ui();
    calculate_all();
}
static inline void safe_label_set_text(lv_obj_t *obj, const char *txt)
{
    if (obj && lv_obj_is_valid(obj) && txt) {
        lv_label_set_text(obj, txt);
    }
}

void inchange_languge()
{
    int dd = EEprom2_Language;
    if (objects.dd_language && lv_obj_is_valid(objects.dd_language)) {
        dd = lv_dropdown_get_selected(objects.dd_language);
    }
    switch (dd)
    {
    case L_english:
    {
        EEprom2_Language = dd; a2_eeprom_write_all();        
        safe_label_set_text(objects.t_language, "Language");
        safe_label_set_text(objects.t_touch, "Touch feedback");
        safe_label_set_text(objects.t_buzzer, "Sound");
        safe_label_set_text(objects.t_enter_key1, "Enter");
        safe_label_set_text(objects.t_enter_key2, "Enter");
        safe_label_set_text(objects.t_tab_key1, "Tab");
        safe_label_set_text(objects.t_save, "Save");
        safe_label_set_text(objects.t_flowrate, "Flow Rate (L/M)");
        safe_label_set_text(objects.t_type_out, "Type Output");
        safe_label_set_text(objects.t_type_output, "N.C");
        safe_label_set_text(objects.t_input, "Digital Inputs");
        safe_label_set_text(objects.t_start, "Start Input");
        safe_label_set_text(objects.t_stop, "Stop Input");
        safe_label_set_text(objects.t_buzzer_out, "Buzzer (Q2)");
        safe_label_set_text(objects.t_valve1, "Valve 1 (Q0)");
        safe_label_set_text(objects.t_out_put, "Industrial Outputs");
        safe_label_set_text(objects.t_valve2, "Valve 2 (Q1)");
        safe_label_set_text(objects.t_gaid5, "Q5 -> BuzzerOut");
        safe_label_set_text(objects.t_gaid4, "Q4 -> Valve2");
        safe_label_set_text(objects.t_gaid3, "Q3 -> Valve1");
        safe_label_set_text(objects.t_gaid2, "Q2 -> Stop");
        safe_label_set_text(objects.t_gaid1, "Q1 -> Start");
        safe_label_set_text(objects.t_headerprj, "Argon Gas Filler");
        safe_label_set_text(objects.t_length_l, "Length");
        safe_label_set_text(objects.t_width_r, "Width");
        safe_label_set_text(objects.t_heigth_l, "Height");
        safe_label_set_text(objects.t_length_r, "Length");
        safe_label_set_text(objects.t_width_l, "Width");
        safe_label_set_text(objects.t_heigth_r, "Height");
        safe_label_set_text(objects.t_start_l, "Start");
        safe_label_set_text(objects.t_stop_l, "Stop");
        safe_label_set_text(objects.t_start_r, "Start");
        safe_label_set_text(objects.t_stop_r, "Stop");
        safe_label_set_text(objects.t_setting, "Setting");
        update_tabview_titles("General", "Test", "Admin", NULL);
    }
    break;
    case L_Persion:
    {
        EEprom2_Language = dd; a2_eeprom_write_all();
        safe_label_set_text(objects.t_language, "زبان");
        safe_label_set_text(objects.t_touch, "بازخورد لمس");
        safe_label_set_text(objects.t_buzzer, "صدا و بازر");
        safe_label_set_text(objects.t_enter_key1,"تایید");
        safe_label_set_text(objects.t_enter_key2,"تایید");
        safe_label_set_text(objects.t_tab_key1,"بعدی");
        safe_label_set_text(objects.t_save,"ذخیره");
        safe_label_set_text(objects.t_flowrate, "نرخ جریان (دبی)");
        safe_label_set_text(objects.t_type_out,"نوع خروجی");
        safe_label_set_text(objects.t_type_output,"N.C");
        safe_label_set_text(objects.t_input,"ورودی های دیجیتال");
        safe_label_set_text(objects.t_start,"شاسی شروع");
        safe_label_set_text(objects.t_stop,"استپ");
        safe_label_set_text(objects.t_buzzer_out,"هشدار صوتی (Q2)");
        safe_label_set_text(objects.t_valve1,"شیربرقی 1 (Q0)");
        safe_label_set_text(objects.t_out_put,"خروجی های صنعتی");
        safe_label_set_text(objects.t_valve2,"شیربرقی 2 (Q1)");
        safe_label_set_text(objects.t_gaid5,"Q5 -> BuzzerOut");
        safe_label_set_text(objects.t_gaid4,"Q4 -> Valve2");
        safe_label_set_text(objects.t_gaid3,"Q3 -> Valve1");
        safe_label_set_text(objects.t_gaid2,"Q2 -> Stop");
        safe_label_set_text(objects.t_gaid1,"Q1 -> Start");
        safe_label_set_text(objects.t_headerprj,"تزریق گاز آرگون");
        safe_label_set_text(objects.t_length_l,"طول");
        safe_label_set_text(objects.t_width_r,"عرض");
        safe_label_set_text(objects.t_heigth_l,"ارتفاع");
        safe_label_set_text(objects.t_length_r,"طول");
        safe_label_set_text(objects.t_width_l,"عرض");
        safe_label_set_text(objects.t_heigth_r,"ارتفاع");
        safe_label_set_text(objects.t_start_l,"شروع");
        safe_label_set_text(objects.t_stop_l,"توقف");
        safe_label_set_text(objects.t_start_r,"شروع");
        safe_label_set_text(objects.t_stop_r,"توقف");
        safe_label_set_text(objects.t_setting,"تنظیمات");

        update_tabview_titles("عمومی", "تست", "مدیر", NULL);
    }
    break;
    case L_Turkish:
    {
        EEprom2_Language = dd; a2_eeprom_write_all();
        safe_label_set_text(objects.t_language, "Dil");
        safe_label_set_text(objects.t_touch, "Dokunmatik");
        safe_label_set_text(objects.t_buzzer, "Ses");
        safe_label_set_text(objects.t_enter_key1, "Giris");
        safe_label_set_text(objects.t_enter_key2, "Giris");
        safe_label_set_text(objects.t_tab_key1, "Sekme");
        safe_label_set_text(objects.t_save, "Kaydet");
        safe_label_set_text(objects.t_flowrate, "Debi (L/D)");
        safe_label_set_text(objects.t_type_out, "Cikis Tipi");
        safe_label_set_text(objects.t_type_output, "N.C");
        safe_label_set_text(objects.t_input, "Dijital Girisler");
        safe_label_set_text(objects.t_start, "Baslat Girisi");
        safe_label_set_text(objects.t_stop, "Durdur Girisi");
        safe_label_set_text(objects.t_buzzer_out, "Sesli Alarm (Q2)");
        safe_label_set_text(objects.t_valve1, "Vana 1 (Q0)");
        safe_label_set_text(objects.t_out_put, "Endustriyel Cikislar");
        safe_label_set_text(objects.t_valve2, "Vana 2 (Q1)");
        safe_label_set_text(objects.t_gaid5, "Q5 -> BuzzerOut");
        safe_label_set_text(objects.t_gaid4, "Q4 -> Valve2");
        safe_label_set_text(objects.t_gaid3, "Q3 -> Valve1");
        safe_label_set_text(objects.t_gaid2, "Q2 -> Stop");
        safe_label_set_text(objects.t_gaid1, "Q1 -> Start");
        safe_label_set_text(objects.t_headerprj, "Argon Gaz Dolumu");
        safe_label_set_text(objects.t_length_l, "Boy");
        safe_label_set_text(objects.t_width_r, "En");
        safe_label_set_text(objects.t_heigth_l, "Yuks.");
        safe_label_set_text(objects.t_length_r, "Boy");
        safe_label_set_text(objects.t_width_l, "En");
        safe_label_set_text(objects.t_heigth_r, "Yuks.");
        safe_label_set_text(objects.t_start_l, "Baslat");
        safe_label_set_text(objects.t_stop_l, "Durdur");
        safe_label_set_text(objects.t_start_r, "Baslat");
        safe_label_set_text(objects.t_stop_r, "Durdur");
        safe_label_set_text(objects.t_setting, "Ayarlar");

        update_tabview_titles("Genel", "Test", "Yonetici", NULL);
    }
    break;
    case L_Russian:
    {
        EEprom2_Language = dd; a2_eeprom_write_all();
        safe_label_set_text(objects.t_language, "Yazyk");
        safe_label_set_text(objects.t_touch, "Otklik");
        safe_label_set_text(objects.t_buzzer, "Zvuk");
        safe_label_set_text(objects.t_enter_key1, "Vvod");
        safe_label_set_text(objects.t_enter_key2, "Vvod");
        safe_label_set_text(objects.t_tab_key1, "Tab");
        safe_label_set_text(objects.t_save, "Sohranit");
        safe_label_set_text(objects.t_flowrate, "Raskhod (L/M)");
        safe_label_set_text(objects.t_type_out, "Tip vykhoda");
        safe_label_set_text(objects.t_type_output, "N.C");
        safe_label_set_text(objects.t_input, "Tsifr. Vkhody");
        safe_label_set_text(objects.t_start, "Pusk Vkhod");
        safe_label_set_text(objects.t_stop, "Stop Vkhod");
        safe_label_set_text(objects.t_buzzer_out, "Sirena (Q2)");
        safe_label_set_text(objects.t_valve1, "Klapan 1 (Q0)");
        safe_label_set_text(objects.t_out_put, "Prom. Vykhody");
        safe_label_set_text(objects.t_valve2, "Klapan 2 (Q1)");
        safe_label_set_text(objects.t_gaid5, "Q5 -> BuzzerOut");
        safe_label_set_text(objects.t_gaid4, "Q4 -> Valve2");
        safe_label_set_text(objects.t_gaid3, "Q3 -> Valve1");
        safe_label_set_text(objects.t_gaid2, "Q2 -> Stop");
        safe_label_set_text(objects.t_gaid1, "Q1 -> Start");
        safe_label_set_text(objects.t_headerprj, "Argon Filler");
        safe_label_set_text(objects.t_length_l, "Dlina");
        safe_label_set_text(objects.t_width_r, "Shirina");
        safe_label_set_text(objects.t_heigth_l, "Vysota");
        safe_label_set_text(objects.t_length_r, "Dlina");
        safe_label_set_text(objects.t_width_l, "Shirina");
        safe_label_set_text(objects.t_heigth_r, "Vysota");
        safe_label_set_text(objects.t_start_l, "Pusk");
        safe_label_set_text(objects.t_stop_l, "Stop");
        safe_label_set_text(objects.t_start_r, "Pusk");
        safe_label_set_text(objects.t_stop_r, "Stop");
        safe_label_set_text(objects.t_setting, "Nastroyki");

        update_tabview_titles("Obshchie", "Test", "Admin", NULL);
    }
    break;
    
    default:
        break;
    }

    /* Keep the dimension labels on their fixed rows for every language. */
    ui_apply_main_dimension_label_layout((uint8_t)dd);
    ui_station_show_name(0);
    ui_station_show_name(1);
    ui_refresh_process_runtime_labels();
    ui_refresh_test_runtime_labels();
    ui_refresh_admin_runtime_labels();
    ui_refresh_wifi_runtime_labels();
    ui_refresh_firmware_runtime_labels();
    ui_style_setting_tab_buttons();
}
void action_lang_changed(lv_event_t * e)
{
    buzzer_key;
    inchange_languge();
    ui_set_start_running(0);
    ui_set_start_running(1);
    ui_refresh_general_language_flag();
    ui_refresh_general_runtime_labels();
    ui_style_setting_tab_buttons();
}
void ChangeLanguage(char language)
{
    lv_dropdown_set_selected(objects.dd_language, language);
    inchange_languge();
    ui_set_start_running(0);
    ui_set_start_running(1);
    ui_refresh_general_language_flag();
    ui_refresh_general_runtime_labels();
    ui_style_setting_tab_buttons();
}
