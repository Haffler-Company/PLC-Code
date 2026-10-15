#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t actions_start(uint32_t period_ms, UBaseType_t prio);
void actions_stop(void);
void actions_pause(void);
void actions_resume(void);
void actions_tick(void);
void settings_refresh_labels(void);
void open_settings_tab(uint32_t tab_idx);
void hwkey_save_and_home(void);
uint32_t plc_get_scan_us(void);

extern void action_go_settings(lv_event_t * e);
extern void action_go_menu(lv_event_t * e);
extern void action_go_compressor_on(lv_event_t * e);
extern void action_go_make_icecream_on(lv_event_t * e);
extern void action_go_paddle_on(lv_event_t * e);
extern void action_go_make_b_on(lv_event_t * e);
extern void action_go_hold_b_on(lv_event_t * e);
extern void action_go_paddle_b_on(lv_event_t * e);
extern void action_sample_value_focus(lv_event_t * e);
extern void action_sample_value_defocus(lv_event_t * e);
extern void action_up_timer_btn(lv_event_t * e);
extern void action_down_timer_btn(lv_event_t * e);
extern void action_start_wifi_scan(lv_event_t * e);
extern void action_down_year_act(lv_event_t * e);
extern void action_up_year_act(lv_event_t * e);
extern void action_down_mont_act(lv_event_t * e);
extern void action_up_mont_act(lv_event_t * e);
extern void action_down_day_act(lv_event_t * e);
extern void action_up_day_act(lv_event_t * e);
extern void action_down_hour_act(lv_event_t * e);
extern void action_up_hour_act(lv_event_t * e);
extern void action_down_min_act(lv_event_t * e);
extern void action_up_min_act(lv_event_t * e);
extern void action_comp_timer_act(lv_event_t * e);
extern void action_make_timer_act(lv_event_t * e);
extern void action_paddle_timer_act(lv_event_t * e);
extern void action_f1_act(lv_event_t * e);
extern void action_f2_act(lv_event_t * e);
extern void action_f3_act(lv_event_t * e);
extern void action_f4_act(lv_event_t * e);
extern void action_f5_act(lv_event_t * e);
extern void action_f6_act(lv_event_t * e);
extern void action_f7_act(lv_event_t * e);
extern void action_settings_tab_changed(lv_event_t * e);
extern void action_f5_adjust(lv_event_t * e);
extern void action_active_key(lv_event_t * e);
extern void action_headen_active_key(lv_event_t * e);
extern void action_act_set_current1(lv_event_t * e);
extern void action_act_set_current2(lv_event_t * e);
extern void action_active_key_1(lv_event_t * e);
extern void action_headen_active_key_2(lv_event_t * e);
extern void action_reset_counter_icecream(lv_event_t * e);
extern void action_reset_counter_confirm(lv_event_t * e);
extern void action_reset_counter_cancel(lv_event_t * e);
extern void action_scan_wifi_start(lv_event_t * e);
extern void action_connect_btn_act(lv_event_t * e);
extern void action_wifi_key_act(lv_event_t * e);
extern void action_wifi_status_act(lv_event_t * e);
void ui_update_icecream_counter(void);
extern void action_save_settings(lv_event_t * e);
extern void action_save_btn_event_cb(lv_event_t * e);
extern void action_help_dialog(lv_event_t * e);
extern void action_close_help_dialog(lv_event_t * e);
extern void action_open_lease_dialog(lv_event_t * e);
extern void action_close_lease_dialog(lv_event_t * e);
extern void action_submit_lease_code(lv_event_t * e);
extern void action_buzzer_sw_changed(lv_event_t * e);
extern void action_io_relay_toggle(lv_event_t * e);
extern void action_io_pulse_reset(lv_event_t * e);
void io_test_turn_off_all_relays(void);
bool io_test_is_active(void);
extern void keyboard_color_draw_task_cb(lv_event_t * e);
void ui_update_lease_display(void);
extern void action_open_fault_history(lv_event_t * e);
extern void action_close_fault_history(lv_event_t * e);
extern void action_clear_fault_history(lv_event_t * e);

extern void action_clock_long_press(lv_event_t * e);
extern void action_wifi_kb_ready(lv_event_t * e);
extern void action_wifi_eye_toggle_cb(lv_event_t * e);
extern void action_manager_pin_num(lv_event_t * e);
extern void action_manager_pin_del(lv_event_t * e);
extern void action_manager_pin_ok(lv_event_t * e);
extern void action_manager_pin_cancel(lv_event_t * e);
extern void action_manager_pin_eye_toggle(lv_event_t * e);
extern void action_manager_pin_reset(void);
extern void action_manager_pin_submit_str(const char *pin_str);
extern void action_manager_change_pin(lv_event_t * e);
extern void action_manager_stage_adj(lv_event_t * e);
extern void action_manager_chain_start(lv_event_t * e);
extern void action_manager_unlock_code(lv_event_t * e);
extern void action_manager_factory_reset(lv_event_t * e);
extern void action_manager_factory_confirm(lv_event_t * e);
extern void action_manager_factory_cancel(lv_event_t * e);

extern void action_fault_retry(lv_event_t * e);
extern void action_fault_dismiss(lv_event_t * e);
extern void action_live_monitor_click(lv_event_t * e);
extern void action_go_framware(lv_event_t * e);
extern void action_framware_back(lv_event_t * e);
void framware_set_return_target(int screen_id, uint32_t tab_idx);
extern void action_boot_wifi_cancel(lv_event_t * e);
extern void action_boot_wifi_goto(lv_event_t * e);
extern void action_boot_ota_cancel(lv_event_t * e);
extern void action_boot_ota_install(lv_event_t * e);

extern void action_universal_stepper(lv_event_t * e);
void ui_stepper_do_step(uint8_t param, int dir);
extern void action_lang_select_fa(lv_event_t * e);
extern void action_lang_select_en(lv_event_t * e);
extern void action_lang_select_tr(lv_event_t * e);
extern void action_lang_cycle(lv_event_t * e);
extern void action_open_day_lock_dialog(lv_event_t * e);
extern void action_open_5timers_dialog(lv_event_t * e);
extern void action_ntp_sync_cb(lv_event_t * e);
extern void action_wifi_sw_changed(lv_event_t * e);

/* Settings Dirty-State, Granular Diff & Wear-Leveling API */
typedef enum {
    SETTING_DIFF_NONE = 0,
    SETTING_DIFF_LANG,
    SETTING_DIFF_PADDLE_T1,
    SETTING_DIFF_PADDLE_T2,
    SETTING_DIFF_STARTUP_T,
    SETTING_DIFF_MID_EYE_T,
    SETTING_DIFF_SCREENSAVER,
    SETTING_DIFF_RTC_DATE,
    SETTING_DIFF_RTC_TIME,
    SETTING_DIFF_MODBUS_DELAY,
    SETTING_DIFF_RESTART_1,
    SETTING_DIFF_RESTART_2,
    SETTING_DIFF_CURRENT_1A,
    SETTING_DIFF_CURRENT_1B,
    SETTING_DIFF_CURRENT_2A,
    SETTING_DIFF_CURRENT_2B,
    SETTING_DIFF_COMP_TIMER,
    SETTING_DIFF_MAKE_TIMER,
    SETTING_DIFF_PADDLE_TIMER,
    SETTING_DIFF_COMP3P_TIME,
    SETTING_DIFF_COMP1P_TIME,
    SETTING_DIFF_DISP_COMP_RUN,
    SETTING_DIFF_FRONT_HOLD_COMP,
    SETTING_DIFF_SCOOPS_COMP,
    SETTING_DIFF_MAX_FREEZE_TO,
    SETTING_DIFF_DISP_LEVER_MAX,
    SETTING_DIFF_HOLD_TEMP_ON,
    SETTING_DIFF_HOLD_TEMP_OFF,
    SETTING_DIFF_MIXER_ON,
    SETTING_DIFF_MIXER_OFF,
    SETTING_DIFF_COMP_FALLBACK_ON,
    SETTING_DIFF_COMP_FALLBACK_OFF,
    SETTING_DIFF_PULLEY_EN,
    SETTING_DIFF_COUNT
} setting_diff_id_t;

typedef struct {
    setting_diff_id_t id;
    char name[48];
    char old_val[32];
    char new_val[32];
} setting_diff_item_t;

void settings_take_snapshot(void);
bool settings_is_dirty(void);
void settings_revert_to_snapshot(void);
void ui_check_settings_dirty(void);
void ui_set_save_btn_state(bool modified);
void ui_mark_settings_modified(void);
int ui_get_settings_diff_list(setting_diff_item_t *items, int max_items);
bool ui_revert_single_setting(setting_diff_id_t id);
extern void action_unsaved_revert_single_cb(lv_event_t *e);
extern void action_unsaved_save_and_exit_cb(lv_event_t *e);
extern void action_unsaved_discard_and_exit_cb(lv_event_t *e);
extern void action_unsaved_cancel_cb(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/