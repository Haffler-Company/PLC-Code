#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_SETTING = 2,
    SCREEN_ID_WIFI = 3,
    SCREEN_ID_FRAMWARE = 4,
    _SCREEN_ID_LAST = 4
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *setting;
    lv_obj_t *screen_wifi;
    lv_obj_t *screen_framware;
    lv_obj_t *drop_down_wifi_list;
    lv_obj_t *btn_wifi_connect;
    lv_obj_t *btn_wifi_refresh;
    lv_obj_t *panle_wifi_password;
    lv_obj_t *text_area_wifi_password;
    lv_obj_t *kb_wifi_password;
    lv_obj_t *ib_setting;
    lv_obj_t *t_headerprj;
    lv_obj_t *ta_length;
    lv_obj_t *t_length_l;
    lv_obj_t *bt_clear_left;
    lv_obj_t *ta_heigth;
    lv_obj_t *t_heigth_l;
    lv_obj_t *ta_depth;
    lv_obj_t *t_width_r;
    lv_obj_t *obj0;
    lv_obj_t *t_start_l;
    lv_obj_t *bt_stop;
    lv_obj_t *t_stop_l;
    lv_obj_t *ta_length1;
    lv_obj_t *t_length_r;
    lv_obj_t *bt_clear_right;
    lv_obj_t *ta_heigth1;
    lv_obj_t *t_heigth_r;
    lv_obj_t *ta_depth1;
    lv_obj_t *t_width_l;
    lv_obj_t *obj1;
    lv_obj_t *t_start_r;
    lv_obj_t *bt_stop_1;
    lv_obj_t *t_stop_r;
    lv_obj_t *bar_gaspercent;
    lv_obj_t *t_target_value;
    lv_obj_t *t_target_percent;
    lv_obj_t *bar_gaspercent1;
    lv_obj_t *t_target_value1;
    lv_obj_t *t_target_percent1;
    lv_obj_t *ib_setting1;
    lv_obj_t *icon_wifi;
    lv_obj_t *btn_buzzer;
    lv_obj_t *icon_buzzer;
    lv_obj_t *key_board_panle;
    lv_obj_t *kb_key_board;
    lv_obj_t *obj2;
    lv_obj_t *t_enter_key1;
    lv_obj_t *obj3;
    lv_obj_t *t_tab_key1;
    lv_obj_t *obj4;
    lv_obj_t *bt_keyboard_close;
    lv_obj_t *t_time;
    lv_obj_t *t_date;
    lv_obj_t *hafler_panle;
    lv_obj_t *t_version_softwear;
    lv_obj_t *layer1_header_1;
    lv_obj_t *t_setting;
    lv_obj_t *ib_home;
    lv_obj_t *tabview_setting;
    lv_obj_t *t_general;
    lv_obj_t *t_touch;
    lv_obj_t *sw_buzzer;
    lv_obj_t *t_buzzer;
    lv_obj_t *dd_language;
    lv_obj_t *t_language;
    lv_obj_t *sw_touch;
    lv_obj_t *t_process;
    lv_obj_t *obj5;
    lv_obj_t *key_board_panle_setting;
    lv_obj_t *kb_key_board_1;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *t_enter_key2;
    lv_obj_t *obj9;
    lv_obj_t *t_save;
    lv_obj_t *t_flowrate;
    lv_obj_t *t_unit_qs;
    lv_obj_t *ta_qs;
    lv_obj_t *ta_p2;
    lv_obj_t *ta_p1;
    lv_obj_t *t_type_out;
    lv_obj_t *t_type_output;
    lv_obj_t *sw_type_out_put;
    lv_obj_t *t_test;
    lv_obj_t *obj10;
    lv_obj_t *t_input;
    lv_obj_t *t_start;
    lv_obj_t *s_start;
    lv_obj_t *t_stop;
    lv_obj_t *s_stop;
    lv_obj_t *obj11;
    lv_obj_t *s_buzzer_out;
    lv_obj_t *t_buzzer_out;
    lv_obj_t *s_valve1;
    lv_obj_t *t_valve1;
    lv_obj_t *t_out_put;
    lv_obj_t *s_valve2;
    lv_obj_t *t_valve2;
    lv_obj_t *t_guide;
    lv_obj_t *t_gaid5;
    lv_obj_t *t_gaid4;
    lv_obj_t *t_gaid3;
    lv_obj_t *t_gaid2;
    lv_obj_t *t_gaid1;
} objects_t;

extern objects_t objects;

typedef enum {
    UI_MAIN_STATUS_READY = 0,
    UI_MAIN_STATUS_INJECTING,
    UI_MAIN_STATUS_COMPLETE,
    UI_MAIN_STATUS_INPUT_ERROR,
    UI_MAIN_STATUS_STOPPED
} ui_main_status_t;

void ui_clock_init(void);
void ui_clock_force_refresh(void);
void ui_apply_main_dimension_label_layout(uint8_t language);
void ui_main_show_station_status(uint8_t station, ui_main_status_t status);
void ui_main_schedule_bar_reset(uint8_t station);
void ui_main_cancel_bar_reset(uint8_t station);
void ui_refresh_general_language_flag(void);
void ui_refresh_general_runtime_labels(void);
void ui_refresh_process_runtime_labels(void);
void ui_refresh_test_runtime_labels(void);
void ui_refresh_admin_runtime_labels(void);
void ui_refresh_wifi_runtime_labels(void);
void ui_wifi_on_screen_enter(void);
void ui_open_wifi_pw_modal(void);
void ui_refresh_firmware_runtime_labels(void);
void ui_station_show_name(uint8_t station);
void ui_style_setting_tab_buttons(void);
void ui_btn_alarm(lv_obj_t *btn, bool is_start);
void ui_open_ota_modal(void);
void ui_open_link_entry_modal(void);
void ui_close_link_entry_modal(void);
void ui_setting_help_open(lv_event_t *e);
void ui_pinout_diagram_open(lv_event_t *e);
void ui_close_all_modals(void);
void ui_admin_show_options(void);
void ui_admin_open_datetime_modal(void);
void ui_admin_open_daylock_modal(void);
void ui_admin_open_stats_modal(void);
void ui_daylock_open_numpad(void);
void create_screen_main();
void tick_screen_main();

void create_screen_setting();
void tick_screen_setting();

void create_screen_wifi();
void tick_screen_wifi();

void create_screen_framware();
void tick_screen_framware();
void ui_framware_on_screen_enter(void);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();
void ui_check_and_show_update_prompt(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
