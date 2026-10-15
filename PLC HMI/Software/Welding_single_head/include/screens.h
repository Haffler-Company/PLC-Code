#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>
#include "lvgl_project_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_SCREEN_SPLASH = 1,
    SCREEN_ID_SCREEN_MENU = 2,
    SCREEN_ID_SCREEN_SETTING_GENERAL = 3,
    SCREEN_ID_SCREEN_SETTING_JACK_SETTING = 4,
    SCREEN_ID_SCREEN_SETTING_ADMIN = 5,
    SCREEN_ID_SCREEN_ADMIN_PASSWORD = 6,
    SCREEN_ID_SCREEN_LOADING = 7,
    SCREEN_ID_SCREEN_UPDATE = 8,
    SCREEN_ID_SCREEN_FRAMWARE = 9,
    SCREEN_ID_SCREEN_PASSWORD = 10,
    SCREEN_ID_SCREEN_WIFI = 11,
    _SCREEN_ID_LAST = 11
};

typedef struct _objects_t {
    lv_obj_t *screen_splash;
    lv_obj_t *screen_menu;
    lv_obj_t *screen_setting_general;
    lv_obj_t *screen_setting_jack_setting;
    lv_obj_t *screen_setting_admin;
    lv_obj_t *screen_admin_password;
    lv_obj_t *screen_loading;
    lv_obj_t *screen_update;
    lv_obj_t *screen_framware;
    lv_obj_t *screen_password;
    lv_obj_t *screen_wifi;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *header_continer;
    lv_obj_t *lamp_axe;
    lv_obj_t *obj2;
    lv_obj_t *lamp_plate;
    lv_obj_t *obj3;
    lv_obj_t *lamp_p1;
    lv_obj_t *obj4;
    lv_obj_t *lamp_p2;
    lv_obj_t *obj5;
    lv_obj_t *lamp_res;
    lv_obj_t *obj6;
    lv_obj_t *lamp_out2;
    lv_obj_t *obj7;
    lv_obj_t *lamp_out1;
    lv_obj_t *obj8;
    lv_obj_t *ssr;
    lv_obj_t *obj9;
    lv_obj_t *temputre_arcs;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *mode_side_bar;
    lv_obj_t *btn_f1_mode;
    lv_obj_t *btn_f2_mode;
    lv_obj_t *btn_f3_mode;
    lv_obj_t *btn_f4_mode;
    lv_obj_t *btn_f5_mode;
    lv_obj_t *panle_emergency_stop;
    lv_obj_t *obj15;
    lv_obj_t *header_continer_1;
    lv_obj_t *setting_imageicon;
    lv_obj_t *date_time;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *panle_emergency_stop_1;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *obj31;
    lv_obj_t *mode_side_bar_1;
    lv_obj_t *btn_f1_mode_1;
    lv_obj_t *btn_f2_mode_1;
    lv_obj_t *btn_f3_mode_1;
    lv_obj_t *btn_f4_mode_1;
    lv_obj_t *btn_f5_mode_1;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *general_speaker1;
    lv_obj_t *general_auto_start;
    lv_obj_t *general_finaljeck;
    lv_obj_t *general_limit;
    lv_obj_t *obj37;
    lv_obj_t *obj38;
    lv_obj_t *obj39;
    lv_obj_t *obj40;
    lv_obj_t *obj41;
    lv_obj_t *obj42;
    lv_obj_t *obj43;
    lv_obj_t *mode_side_bar_2;
    lv_obj_t *btn_f1_mode_2;
    lv_obj_t *btn_f2_mode_2;
    lv_obj_t *btn_f3_mode_2;
    lv_obj_t *btn_f4_mode_2;
    lv_obj_t *btn_f5_mode_2;
    lv_obj_t *obj44;
    lv_obj_t *obj45;
    lv_obj_t *obj46;
    lv_obj_t *obj47;
    lv_obj_t *obj48;
    lv_obj_t *obj49;
    lv_obj_t *obj50;
    lv_obj_t *obj51;
    lv_obj_t *obj52;
    lv_obj_t *obj53;
    lv_obj_t *obj54;
    lv_obj_t *obj55;
    lv_obj_t *obj56;
    lv_obj_t *obj57;
    lv_obj_t *obj58;
    lv_obj_t *obj59;
    lv_obj_t *obj60;
    lv_obj_t *panle_jack_modify;
    lv_obj_t *obj61;
    lv_obj_t *obj62;
    lv_obj_t *jack_keyboard;
    lv_obj_t *jack_bargasht_holder;
    lv_obj_t *jack_raft_holder;
    lv_obj_t *obj63;
    lv_obj_t *jack_bargasht_m;
    lv_obj_t *jack_bargasht_p;
    lv_obj_t *obj64;
    lv_obj_t *obj65;
    lv_obj_t *obj66;
    lv_obj_t *mode_side_bar_3;
    lv_obj_t *btn_f1_mode_3;
    lv_obj_t *btn_f2_mode_3;
    lv_obj_t *btn_f3_mode_3;
    lv_obj_t *btn_f4_mode_3;
    lv_obj_t *btn_f5_mode_3;
    lv_obj_t *obj67;
    lv_obj_t *obj68;
    lv_obj_t *obj69;
    lv_obj_t *obj70;
    lv_obj_t *btn_setting_admin_thermocouple_k;
    lv_obj_t *obj71;
    lv_obj_t *btn_setting_admin_thermocouple_j;
    lv_obj_t *obj72;
    lv_obj_t *obj73;
    lv_obj_t *obj74;
    lv_obj_t *obj75;
    lv_obj_t *obj76;
    lv_obj_t *obj77;
    lv_obj_t *obj78;
    lv_obj_t *setting_admin_pass_change_panle;
    lv_obj_t *obj79;
    lv_obj_t *obj80;
    lv_obj_t *change_value_keyboard1;
    lv_obj_t *change_value_admin_pass_1;
    lv_obj_t *obj81;
    lv_obj_t *setting_admin_pass_change_panle_1;
    lv_obj_t *obj82;
    lv_obj_t *obj83;
    lv_obj_t *admin_pass_prof_keyboard;
    lv_obj_t *admin_pass_prof_keyboard_place_holder;
    lv_obj_t *obj84;
    lv_obj_t *mode_side_bar_4;
    lv_obj_t *btn_f1_mode_4;
    lv_obj_t *btn_f2_mode_4;
    lv_obj_t *btn_f3_mode_4;
    lv_obj_t *btn_f4_mode_4;
    lv_obj_t *btn_f5_mode_4;
    lv_obj_t *obj85;
    lv_obj_t *obj86;
    lv_obj_t *obj87;
    lv_obj_t *obj88;
    lv_obj_t *obj89;
    lv_obj_t *info_panle;
    lv_obj_t *obj90;
    lv_obj_t *obj91;
    lv_obj_t *obj92;
    lv_obj_t *seriall_number;
    lv_obj_t *obj93;
    lv_obj_t *obj94;
    lv_obj_t *obj95;
    lv_obj_t *obj96;
    lv_obj_t *obj97;
    lv_obj_t *obj98;
    lv_obj_t *obj99;
    lv_obj_t *obj100;
    lv_obj_t *chane_gir;
    lv_obj_t *obj101;
    lv_obj_t *dubble_head_button;
    lv_obj_t *btn_single_dubble_install;
    lv_obj_t *corner_cleaner_button;
    lv_obj_t *btn_single_head_install;
    lv_obj_t *single_head_button;
    lv_obj_t *obj102;
    lv_obj_t *waiting_panle_1;
    lv_obj_t *obj103;
    lv_obj_t *obj104;
    lv_obj_t *panle_wifi_password_1;
    lv_obj_t *obj105;
    lv_obj_t *frameware_password_keybord;
    lv_obj_t *text_area_framware_password;
    lv_obj_t *obj106;
    lv_obj_t *obj107;
    lv_obj_t *waiting_panle_2;
    lv_obj_t *obj108;
    lv_obj_t *obj109;
    lv_obj_t *obj110;
    lv_obj_t *drop_down_wifi_list;
    lv_obj_t *btn_wifi_connect;
    lv_obj_t *btn_wifi_redresh;
    lv_obj_t *panle_wifi_password;
    lv_obj_t *obj111;
    lv_obj_t *obj112;
    lv_obj_t *keybord;
    lv_obj_t *text_area_wifi_password;
    lv_obj_t *waiting_panle;
    lv_obj_t *obj113;
    lv_obj_t *obj114;
    lv_obj_t *language_panle;
    lv_obj_t *obj115;
    lv_obj_t *obj116;
    lv_obj_t *obj117;
    lv_obj_t *obj118;
    lv_obj_t *link_entry_panel;
    lv_obj_t *link_entry_btn;
    lv_obj_t *link_entry_cancel;
    lv_obj_t *link_entry_confirm;
    lv_obj_t *link_entry_error;
    lv_obj_t *link_entry_textarea;
    lv_obj_t *link_entry_keyboard;
} objects_t;

extern objects_t objects;

void create_screen_screen_splash();
void delete_screen_screen_splash();
void tick_screen_screen_splash();

void create_screen_screen_menu();
void delete_screen_screen_menu();
void tick_screen_screen_menu();

void create_screen_screen_setting_general();
void delete_screen_screen_setting_general();
void tick_screen_screen_setting_general();

void create_screen_screen_setting_jack_setting();
void delete_screen_screen_setting_jack_setting();
void tick_screen_screen_setting_jack_setting();

void create_screen_screen_setting_admin();
void delete_screen_screen_setting_admin();
void tick_screen_screen_setting_admin();

void create_screen_screen_admin_password();
void delete_screen_screen_admin_password();
void tick_screen_screen_admin_password();

void create_screen_screen_loading();
void delete_screen_screen_loading();
void tick_screen_screen_loading();

void create_screen_screen_update();
void delete_screen_screen_update();
void tick_screen_screen_update();

void create_screen_screen_framware();
void delete_screen_screen_framware();
void tick_screen_screen_framware();

void create_screen_screen_password();
void delete_screen_screen_password();
void tick_screen_screen_password();

void create_screen_screen_wifi();
void delete_screen_screen_wifi();
void tick_screen_screen_wifi();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

// Groups

typedef struct _groups_t {
    lv_group_t *jjj;
} groups_t;

extern groups_t groups;

void ui_create_groups();

// Color themes

enum Themes {
    THEME_ID_DEFAULT,
};
enum Colors {
    COLOR_ID_DEFAULT_MODE,
    COLOR_ID_CHEKED_MODE,
    COLOR_ID_DISABLE_LAMP,
    COLOR_ID_DEFAULT_HEADER,
    COLOR_ID_DEFAULT_MENU_BACKGROUND,
    COLOR_ID_DEFAULT_MENU_TEXT_COLOR,
    COLOR_ID_ENABLE_CYCEL,
    COLOR_ID_DISABLE_CYCEL,
    COLOR_ID_ENABLE_MODE,
    COLOR_ID_BUTTON_GREY_1,
    COLOR_ID_G1,
    COLOR_ID_B1,
};
void change_color_theme(uint32_t themeIndex);
extern uint32_t theme_colors[1][12];

void wifi_scan_spinner_set_visible(bool visible);

extern const char *wifi_keyboard_map_lower[];
extern const lv_buttonmatrix_ctrl_t wifi_keyboard_ctrl_lower[];
extern const char *wifi_keyboard_map_upper[];
extern const lv_buttonmatrix_ctrl_t wifi_keyboard_ctrl_upper[];
extern const char *wifi_keyboard_map_special[];
extern const lv_buttonmatrix_ctrl_t wifi_keyboard_ctrl_special[];
void keyboard_color_draw_task_cb(lv_event_t *e);
void ui_request_toast_colored(const char *msg, uint32_t color, uint32_t ms);
void rtc_save_active_screen(int16_t screen_id);
int16_t get_saved_active_screen(void);
bool screensaver_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/