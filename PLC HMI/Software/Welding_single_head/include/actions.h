#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_cycel_start_btn(lv_event_t * e);
extern void action_setting_admin_save_backup_values(lv_event_t * e);
extern void action_factory_reset_btn(lv_event_t * e);
extern void action_setting_admin_reset_the_counter(lv_event_t * e);
extern void action_setting_admin_save_btn(lv_event_t * e);
extern void action_admin_pass_keyboard_redy(lv_event_t * e);
extern void action_main_load(lv_event_t * e);
extern void action_mode(lv_event_t * e);
extern void action_welding_time_changed(lv_event_t * e);
extern void action_cooling_time_changed(lv_event_t * e);
extern void action_element_temp_target_changed(lv_event_t * e);
extern void action_base_values_changed(lv_event_t * e);
extern void action_jack_value_set(lv_event_t * e);
extern void action_setting_general_save_button(lv_event_t * e);
extern void action_admin_password_redy(lv_event_t * e);
extern void action_btn_chane_gir_install(lv_event_t * e);
extern void action_btn_connect(lv_event_t * e);
extern void action_btn_install_package1(lv_event_t * e);
extern void action_btn_single_head_install(lv_event_t * e);
extern void action_btn_install_test_app(lv_event_t * e);
extern void action_btn_wifi_connect_event(lv_event_t * e);
extern void action_btn_wifi_search_event(lv_event_t * e);
extern void start_install_model(const char *model);
extern void start_install_url(const char *url);
extern void action_link_check(void);
extern void action_link_check_start(const char *url);
extern bool link_check_is_done(void);
extern void link_check_clear_done(void);
extern const char *link_check_get_result(void);
extern int link_check_get_status(void);
extern const char *installer_model(int i);
extern const char *installer_version(int i);
extern int installer_compare_version_to_current(int idx);
extern const char *installer_build_date(int idx);
extern const char *installer_build_time(int idx);
extern const char *installer_desc_line(int idx, int line);
extern int installer_desc_count(int idx);
extern const char *installer_desc_line_lang(int idx, int lang, int line);
extern int installer_desc_count_lang(int idx, int lang);
extern const char *installer_sha256(int idx);
extern bool installer_available(int i);
extern bool installer_scan_done(void);
extern void installer_request_scan(void);
extern void installer_prepare_install(int idx);
extern void request_self_update_after_wifi(void);
extern bool has_saved_wifi(void);
extern void dbg0(const char *s);
extern void wifi_ui_request_refresh(void);
extern void wifi_sta_begin_network(const char *ssid, const char *pass, int scan_index);
extern bool wifi_sta_wait_connected(int max_half_sec_ticks, volatile bool *cancel_flag);
extern bool wifi_sta_connect_full(const char *ssid, const char *pass, int scan_index, int max_half_sec_ticks, volatile bool *cancel_flag);
extern void wifi_on_dropdown_selection_changed(int32_t index);
extern void wifi_password_panel_show(void);
extern void wifi_password_panel_hide(void);
extern bool wifi_selected_needs_password(void);
extern void start_info_self_update(void);
extern void self_update_ensure_task(void);
extern void cancel_info_self_update(void);
extern void safe_return_from_ota_to_menu(void);
extern void ota_request_cancel(void);
extern bool is_f5_boot_active(void);
extern void set_f5_boot_active(bool active);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
