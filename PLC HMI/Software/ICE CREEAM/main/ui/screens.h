#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>
#include "ui_image_icons.h"
#include "ui_lang.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *splash_screen;
    lv_obj_t *main_pg;
    lv_obj_t *settings_pg;
    lv_obj_t *screen_framware;
    lv_obj_t *boot_wifi_pg;
    lv_obj_t *screen_warranty;
    lv_obj_t *btn_warr_activate;
    lv_obj_t *lbl_warr_activate_btn;
    lv_obj_t *lbl_warr_status_date;
    lv_obj_t *lbl_warr_status_code;
    lv_obj_t *paddle_timer_slider;
    lv_obj_t *comp_timer_slider;
    lv_obj_t *make_timer_slider;
    lv_obj_t *comp3phase_timer_slider;
    lv_obj_t *comp1phase_timer_slider;
    lv_obj_t *obj0;
    lv_obj_t *fridge_btn;
    lv_obj_t *compressor_btn;
    lv_obj_t *icecreem_btn;
    lv_obj_t *settings_btn;
    lv_obj_t *fridge_status;
    lv_obj_t *comp_status;
    lv_obj_t *paddle_status;
    /* سمت راست (بانک B) — فقط UI/NVS */
    lv_obj_t *fridge_btn_b;
    lv_obj_t *compressor_btn_b;
    lv_obj_t *icecreem_btn_b;
    lv_obj_t *fridge_status_b;
    lv_obj_t *comp_status_b;
    lv_obj_t *paddle_status_b;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *date_lbl;
    lv_obj_t *time_lbl;
    lv_obj_t *clock_cont;
    lv_obj_t *make_icecream_counter_1;
    lv_obj_t *astiaj_title_lbl;
    /* تنظیمات نمونه Delta */
    lv_obj_t *val_paddle_t1;
    lv_obj_t *val_paddle_t2;
    lv_obj_t *val_startup_t;
    lv_obj_t *val_eye_t;
    lv_obj_t *val_modbus;
    lv_obj_t *val_rst_t1;
    lv_obj_t *val_rst_t2;
    lv_obj_t *val_cur1a;
    lv_obj_t *val_cur1b;
    lv_obj_t *val_cur2a;
    lv_obj_t *val_cur2b;
    lv_obj_t *sample_kb;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *tab_f1_btn;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *paddle_time_lbl;
    lv_obj_t *make_time_lbl;
    lv_obj_t *make_time_status_lbl;
    lv_obj_t *comp_time_lbl;
    lv_obj_t *comp_time_status_lbl;
    lv_obj_t *comp3phas_act_time_lbl;
    lv_obj_t *comp1phas_act_time_lbl;
    lv_obj_t *tab_f2_btn;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *day_lbl;
    lv_obj_t *month_lbl;
    lv_obj_t *year_lbl;
    lv_obj_t *min_lbl;
    lv_obj_t *hour_lbl;
    lv_obj_t *down_year_btn;
    lv_obj_t *up_year_btn;
    lv_obj_t *down_mont_btn;
    lv_obj_t *up_mont_btn;
    lv_obj_t *down_day_btn;
    lv_obj_t *up_day_btn;
    lv_obj_t *down_hour_btn;
    lv_obj_t *up_hour_btn;
    lv_obj_t *down_min_btn;
    lv_obj_t *up_min_btn;
    lv_obj_t *tab_f3_btn;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *set_current1_btn;
    lv_obj_t *current_set_1;
    lv_obj_t *current_set_1_key;
    lv_obj_t *set_current2_btn;
    lv_obj_t *current_set_2;
    lv_obj_t *current_set_2_key;
    lv_obj_t *reset_counter_icecream;
    lv_obj_t *make_icecream_counter_2;
    lv_obj_t *lbl_cnt_title;
    lv_obj_t *lbl_cnt_rst;
    lv_obj_t *lbl_ss_title;
    lv_obj_t *lbl_ss_val;
    lv_obj_t *tab_f4_btn;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *ui_scan_list;
    lv_obj_t *scan_wifi_btn;
    lv_obj_t *connect_wifi_btn;
    lv_obj_t *wifi_pass_key;
    lv_obj_t *wifi_pass_txtarea;
    lv_obj_t *wifi_pass_cancel_btn;
    lv_obj_t *wifi_pass_eye_btn;
    lv_obj_t *wifi_pass_eye_lbl;
    lv_obj_t *wifi_pass_connect_btn;
    lv_obj_t *wifi_kb_overlay;
    lv_obj_t *wifi_status_messagebox;
    lv_obj_t *btn_goto_framware;
    lv_obj_t *tab_f5_btn;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *save_btn;
    lv_obj_t *help_btn;
    lv_obj_t *buzzer_sw;
    lv_obj_t *buzzer_sw_lbl;
    lv_obj_t *tab_f6_btn;
    lv_obj_t *led_in_front;
    lv_obj_t *led_in_rear;
    lv_obj_t *led_in_eye;
    lv_obj_t *led_in_stop;
    lv_obj_t *led_in_start;
    lv_obj_t *led_in_loadm;
    lv_obj_t *led_in_loadc;
    lv_obj_t *led_in[8];
    lv_obj_t *lbl_in[8];
    lv_obj_t *row_in[8];
    lv_obj_t *btn_test_comp;
    lv_obj_t *btn_test_make;
    lv_obj_t *btn_test_pdl;
    lv_obj_t *btn_test_cream;
    lv_obj_t *btn_test_air;
    lv_obj_t *btn_test_rb;
    lv_obj_t *btn_test_rst;
    lv_obj_t *btn_test_wash;
    lv_obj_t *lease_card;
    lv_obj_t *lease_days_lbl;
    lv_obj_t *lease_status_badge;
    lv_obj_t *lease_btn_extend;
    lv_obj_t *lease_btn_5timers;
    lv_obj_t *btn_factory_reset;
    lv_obj_t *lbl_factory_reset;
    lv_obj_t *lease_modal;
    lv_obj_t *lease_ta_code;
    lv_obj_t *help_modal;
    lv_obj_t *fault_btn;
    lv_obj_t *viscosity_bar;
    lv_obj_t *viscosity_lbl;
    lv_obj_t *comp_cooldown_badge;
    lv_obj_t *comp_cooldown_lbl;
    lv_obj_t *wash_timer_badge;
    lv_obj_t *wash_timer_lbl;
    lv_obj_t *daily_counter_lbl;
    lv_obj_t *total_counter_lbl;
    lv_obj_t *interactive_fault_modal;
    lv_obj_t *btn_ss_timeout;
    lv_obj_t *btn_pulley_en;
    lv_obj_t *lbl_pulley_title;
    lv_obj_t *lbl_pulley_val;
    lv_obj_t *wifi_sw_btn;
    lv_obj_t *wifi_sw_lbl;
    /* ویجت‌های مانیتورینگ دمای مخزن و سیلندر */
    lv_obj_t *temp_hopper_box;
    lv_obj_t *temp_hopper_lbl;
    lv_obj_t *temp_hopper_cont;
    lv_obj_t *temp_hopper_val;
    lv_obj_t *temp_hopper_unit;
    lv_obj_t *temp_cyl_box;
    lv_obj_t *temp_cyl_lbl;
    lv_obj_t *temp_cyl_cont;
    lv_obj_t *temp_cyl_val;
    lv_obj_t *temp_cyl_unit;
    lv_obj_t *diag_temp_hopper_val;
    lv_obj_t *diag_temp_cyl_val;
    /* پاپ‌آپ اسلایدر ولوم استایل iOS */
    lv_obj_t *volume_popup_card;
    lv_obj_t *volume_slider;
    lv_obj_t *volume_pct_lbl;
    lv_obj_t *volume_icon_lbl;
    /* برچسب‌های متن برای تغییر زنده زبان */
    lv_obj_t *save_lbl;
    lv_obj_t *help_lbl;
    lv_obj_t *fault_lbl;
    lv_obj_t *hdr_f6_lbl;
    lv_obj_t *lbl_mode_make;
    lv_obj_t *lbl_mode_hold;
    lv_obj_t *lbl_mode_pdl;
    lv_obj_t *lbl_mode_edge;
    lv_obj_t *lbl_mode_rb;
    lv_obj_t *lbl_mode_wash;
    lv_obj_t *f6_in_lbls[7];
    lv_obj_t *f6_out_lbls[8];
    lv_obj_t *diag_t1_lbl;
    lv_obj_t *diag_t2_lbl;
    lv_obj_t *wifi_scan_lbl;
    lv_obj_t *wifi_connect_lbl;
    lv_obj_t *wifi_ota_lbl;
    lv_obj_t *lbl_dl_hdr;
    lv_obj_t *lbl_ch_title;
    lv_obj_t *lbl_dl_sup;
    lv_obj_t *lbl_dl_emg;
    lv_obj_t *f6_in_title_lbl;
    lv_obj_t *f6_out_title_lbl;
    lv_obj_t *wifi_pass_title_lbl;
    /* Live Process Monitoring Card */
    lv_obj_t *live_monitor_card;
    lv_obj_t *live_monitor_status_badge;
    lv_obj_t *live_monitor_status_lbl;
    lv_obj_t *live_monitor_timer_badge;
    lv_obj_t *live_monitor_timer_lbl;
    lv_obj_t *live_monitor_next_lbl;
    lv_obj_t *btn_ntp_sync;
    lv_obj_t *lbl_ntp_sync;
    /* Splash Screen & Boot Diagnostics Widgets */
    lv_obj_t *splash_loader_bar;
    lv_obj_t *splash_status_lbl;
    lv_obj_t *splash_ver_lbl;
    lv_obj_t *boot_alert_modal;
    /* Tab F7 & New Stepper Objects */
    lv_obj_t *tab_f7_btn;
    lv_obj_t *hdr_f7_lbl;
    lv_obj_t *val_dsp_run;
    lv_obj_t *val_frt_hold;
    lv_obj_t *val_scp_comp;
    lv_obj_t *val_frz_to;
    lv_obj_t *val_lev_max;
    lv_obj_t *val_t1_on;
    lv_obj_t *val_t1_off;
    lv_obj_t *val_fallback_on;
    lv_obj_t *val_fallback_off;
    lv_obj_t *val_comp_fb_on;
    lv_obj_t *val_comp_fb_off;
    lv_obj_t *card_locked_timers;
    lv_obj_t *lbl_locked_hdr;
    lv_obj_t *lbl_locked_badge;
    lv_obj_t *lbl_fixed_cd;
    lv_obj_t *lbl_fixed_rst;
    lv_obj_t *lbl_fixed_deb;
    lv_obj_t *lbl_fixed_cyl;
    lv_obj_t *lbl_fixed_eye;
} objects_t;

extern objects_t objects;

void ui_screens_refresh_language(void);
void ui_update_toolbar_buttons(void);
void ui_volume_popup_show(void);
void ui_volume_popup_hide(void);
void ui_volume_popup_toggle(void);
void show_modal_help_dialog(const char *title, const char *content);
void hide_modal_help_dialog(void);
void ui_show_single_param_help(uint8_t param_id, ui_str_key_t key);
void show_lease_unlock_dialog(void);
void hide_lease_unlock_dialog(void);
void ui_setup_industrial_numpad(lv_obj_t *kb);
void show_fault_history_dialog(void);
void hide_fault_history_dialog(void);
void show_screensaver(void);
void hide_screensaver(void);
bool screensaver_is_active(void);
void screensaver_tick_1s(void);
void screensaver_notify_activity(void);
void screens_notify_touch_point(int16_t x, int16_t y);
void show_counter_details_dialog(void);
void hide_counter_details_dialog(void);
void action_toggle_ss_timeout(lv_event_t *e);
const char *get_ss_timeout_text(int32_t s);
void update_ss_timeout_ui(void);
void action_toggle_pulley_en(lv_event_t *e);
void update_pulley_en_ui(void);
void show_temp_calibration_modal(bool is_hopper);
void hide_temp_calibration_modal(void);
void show_overload_polarity_modal(bool is_motor);
void hide_overload_polarity_modal(void);
void show_language_selector_modal(void);
void hide_language_selector_modal(void);
void ui_show_tooltip(const char *title, const char *desc);
void ui_hide_tooltip(void);

typedef enum {
    GUIDE_ID_MAKE = 1,
    GUIDE_ID_HOLD,
    GUIDE_ID_PADDLE,
    GUIDE_ID_EDGE,
    GUIDE_ID_RAINBOW,
    GUIDE_ID_WASH,
    GUIDE_ID_SETTINGS,
    GUIDE_ID_TAB_F1,
    GUIDE_ID_TAB_F2,
    GUIDE_ID_TAB_F3,
    GUIDE_ID_TAB_F4,
    GUIDE_ID_TAB_F5,
    GUIDE_ID_TAB_F6,
    GUIDE_ID_COUNTER,
    GUIDE_ID_TEMP_HOPPER,
    GUIDE_ID_TEMP_CYL,
    GUIDE_ID_WARRANTY,
    GUIDE_ID_FAULTS,
} guide_id_t;

void ui_show_smart_guide(guide_id_t id);
bool ui_is_smart_guide_active(void);
uint32_t ui_get_last_guide_long_press_tick(void);
uint32_t ui_get_mode_btn_hold_duration_ms(uint8_t btn_id);
void ui_show_overload_cooldown_dialog(void);
void ui_show_overload_hard_lock_dialog(void);
void ui_hide_overload_dialog(void);

void ui_set_save_btn_state(bool modified);
void ui_mark_settings_modified(void);
void show_unsaved_changes_dialog(void);
void hide_unsaved_changes_dialog(void);

void show_manager_pin_dialog(void);
void hide_manager_pin_dialog(void);
void show_manager_dashboard_dialog(bool is_master);
void hide_manager_dashboard_dialog(void);
void show_factory_reset_confirm_dialog(void);
void hide_factory_reset_confirm_dialog(void);
void show_reset_counter_confirm_dialog(void);
void hide_reset_counter_confirm_dialog(void);
void show_interactive_fault_dialog(uint8_t code, const char *title, const char *cause, const char *solution);
void hide_interactive_fault_dialog(void);
void show_boot_timed_alert(void);
void hide_boot_timed_alert(void);
void show_boot_wifi_prompt_dialog(void);
void hide_boot_wifi_prompt_dialog(void);
void show_boot_ota_prompt_dialog(const char *new_ver);
void hide_boot_ota_prompt_dialog(void);
void ota_ui_start_install_model(int idx);


enum ScreensEnum {
    SCREEN_ID_SPLASH_SCREEN = 1,
    SCREEN_ID_MAIN_PG = 2,
    SCREEN_ID_SETTINGS_PG = 3,
    SCREEN_ID_SCREEN_FRAMWARE = 4,
    SCREEN_ID_BOOT_WIFI = 5,
    SCREEN_ID_WARRANTY = 6,
};

void create_screen_splash_screen();
void tick_screen_splash_screen();

void create_screen_main_pg();
void tick_screen_main_pg();

void create_screen_settings_pg();
void tick_screen_settings_pg();

void create_screen_screen_framware();
void delete_screen_screen_framware();
void tick_screen_screen_framware();

void create_screen_boot_wifi(void);
void delete_screen_boot_wifi(void);
void tick_screen_boot_wifi(void);
void boot_wifi_on_pin_passed(void);

void create_screen_warranty(void);
void delete_screen_warranty(void);
void tick_screen_warranty(void);
void open_warranty_screen(void);
void open_warranty_expired_dialog(void);
void hide_warranty_expired_dialog(void);
void open_warranty_no_net_dialog(void);
void hide_warranty_no_net_dialog(void);
void open_warranty_details_dialog(void);
void hide_warranty_details_dialog(void);
void ui_screens_update_warranty_f6(void);
lv_obj_t *ui_get_warranty_kb(void);

void show_ota_pinmap_dialog(int model_idx);
void hide_ota_pinmap_dialog(void);
void screens_framware_on_load(void);

void show_ota_update_congrats_dialog(const char *prev_ver, const char *new_ver);
void hide_ota_update_congrats_dialog(void);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

extern bool g_boot_f5_service_mode;
extern bool s_pin_for_boot_wifi;
extern bool s_pin_for_warranty_edit;

uint32_t ui_get_mode_btn_hold_duration_ms(uint8_t btn_id);
void ui_reset_mode_btn_hold_duration(uint8_t btn_id);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/