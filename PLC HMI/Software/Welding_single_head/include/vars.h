#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_MAP_KEYBOARD = 0,
    FLOW_GLOBAL_VARIABLE_SETTING_ADMIN_KEYBOARD_MAP = 1,
    FLOW_GLOBAL_VARIABLE_SCREEN_FROM = 2,
    FLOW_GLOBAL_VARIABLE_SCREEN_TO = 3
};

// Native global variables

extern int32_t get_var_element_temperature();
extern void set_var_element_temperature(int32_t value);
extern int32_t get_var_element_temperature_target();
extern void set_var_element_temperature_target(int32_t value);
extern int32_t get_var_welding_time();
extern void set_var_welding_time(int32_t value);
extern int32_t get_var_cooling_time();
extern void set_var_cooling_time(int32_t value);
extern bool get_var_emergency_stop();
extern void set_var_emergency_stop(bool value);
extern const char *get_var_enable_mode();
extern void set_var_enable_mode(const char *value);
extern bool get_var_cycel_started();
extern void set_var_cycel_started(bool value);
extern int32_t get_var_setting_general_temperature_caliber();
extern void set_var_setting_general_temperature_caliber(int32_t value);
extern int32_t get_var_setting_general_pid();
extern void set_var_setting_general_pid(int32_t value);
extern bool get_var_setting_general_speaker();
extern void set_var_setting_general_speaker(bool value);
extern bool get_var_setting_general_wifi();
extern void set_var_setting_general_wifi(bool value);
extern bool get_var_setting_general_automatic_start();
extern void set_var_setting_general_automatic_start(bool value);
extern bool get_var_setting_general_final_jack();
extern void set_var_setting_general_final_jack(bool value);
extern bool get_var_setting_general_sensor_jack();
extern void set_var_setting_general_sensor_jack(bool value);
extern bool get_var_setting_general_temperature_limit();
extern void set_var_setting_general_temperature_limit(bool value);
extern bool get_var_setting_general_pedal_number2();
extern void set_var_setting_general_pedal_number2(bool value);
extern bool get_var_setting_admin_thermocouple_j();
extern void set_var_setting_admin_thermocouple_j(bool value);
extern int32_t get_var_setting_admin_countdown_timer();
extern void set_var_setting_admin_countdown_timer(int32_t value);
extern bool get_var_setting_admin_emergency_switch();
extern void set_var_setting_admin_emergency_switch(bool value);
extern bool get_var_setting_admin_jack_sensor();
extern void set_var_setting_admin_jack_sensor(bool value);
extern const char *get_var_setting_admin_password_placeholder();
extern void set_var_setting_admin_password_placeholder(const char *value);
extern const char *get_var_setting_admin_re_enter_password_placeholder();
extern void set_var_setting_admin_re_enter_password_placeholder(const char *value);
extern const char *get_var_setting_admin_pass_error();
extern void set_var_setting_admin_pass_error(const char *value);
extern const char *get_var_admin_pass();
extern void set_var_admin_pass(const char *value);
extern const char *get_var_jack_map();
extern void set_var_jack_map(const char *value);
extern double get_var_setting_jack_safhe_raft();
extern void set_var_setting_jack_safhe_raft(double value);
extern double get_var_setting_jack_safhe_bargasht();
extern void set_var_setting_jack_safhe_bargasht(double value);
extern double get_var_setting_jack_shablon_raft();
extern void set_var_setting_jack_shablon_raft(double value);
extern double get_var_setting_jack_shablon_bargasht();
extern void set_var_setting_jack_shablon_bargasht(double value);
extern double get_var_setting_jack_profil_raft();
extern void set_var_setting_jack_profil_raft(double value);
extern double get_var_setting_jack_profil_bargasht();
extern void set_var_setting_jack_profil_bargasht(double value);
extern double get_var_setting_jack_josh_raft();
extern void set_var_setting_jack_josh_raft(double value);
extern double get_var_setting_jack_josh_bargasht();
extern void set_var_setting_jack_josh_bargasht(double value);
extern double get_var_jack_dot2_zarbe1_raft();
extern void set_var_jack_dot2_zarbe1_raft(double value);
extern double get_var_jack_dot2_zarbe2_raft();
extern void set_var_jack_dot2_zarbe2_raft(double value);
extern bool get_var_setting_general_pedal_number3();
extern void set_var_setting_general_pedal_number3(bool value);
extern bool get_var_setting_admin_thermocouple_k();
extern void set_var_setting_admin_thermocouple_k(bool value);
extern bool get_var_admin_pass_verifyed();
extern void set_var_admin_pass_verifyed(bool value);
extern bool get_var_admin_can_save();
extern void set_var_admin_can_save(bool value);
extern const char *get_var_software_version();
extern void set_var_software_version(const char *value);
extern const char *get_var_header_time_text();
extern void set_var_header_time_text(const char *value);
extern const char *get_var_header_date_text();
extern void set_var_header_date_text(const char *value);
extern const char *get_var_cpu_temperature_text();
extern void set_var_cpu_temperature_text(const char *value);
extern const char *get_var_internal_temperature_text();
extern void set_var_internal_temperature_text(const char *value);
extern const char *get_var_second_temperature_text();
extern void set_var_second_temperature_text(const char *value);
extern int32_t get_var_total_frame_numbers();
extern void set_var_total_frame_numbers(int32_t value);
extern int32_t get_var_admin_set_hour();
extern void set_var_admin_set_hour(int32_t value);
extern int32_t get_var_admin_set_minute();
extern void set_var_admin_set_minute(int32_t value);
extern int32_t get_var_admin_set_year();
extern void set_var_admin_set_year(int32_t value);
extern int32_t get_var_admin_set_month();
extern void set_var_admin_set_month(int32_t value);
extern int32_t get_var_admin_set_day();
extern void set_var_admin_set_day(int32_t value);
extern const char *get_var_update_error_text();
extern void set_var_update_error_text(const char *value);
extern int32_t get_var_res_countdown();
extern void set_var_res_countdown(int32_t value);
extern int32_t get_var_progress_vlaue();
extern void set_var_progress_vlaue(int32_t value);
extern bool get_var_numeric_keyboard_open();
extern void set_var_numeric_keyboard_open(bool value);
extern bool get_var_josh_out_display();
extern void set_var_josh_out_display(bool value);
extern bool get_var_shablon_out_display();
extern void set_var_shablon_out_display(bool value);
extern bool get_var_safhe_out_display();
extern void set_var_safhe_out_display(bool value);
extern bool get_var_press1_out_display();
extern void set_var_press1_out_display(bool value);
extern bool get_var_press2_out_display();
extern void set_var_press2_out_display(bool value);
extern bool get_var_zarbe1_out_display();
extern void set_var_zarbe1_out_display(bool value);
extern bool get_var_zarbe2_out_display();
extern void set_var_zarbe2_out_display(bool value);
extern bool get_var_header_show_date();
extern void set_var_header_show_date(bool value);
extern bool get_var_test_mode();
extern void set_var_test_mode(bool value);
extern bool get_var_test_ssr();
extern void set_var_test_ssr(bool value);
extern bool get_var_test_shablon();
extern void set_var_test_shablon(bool value);
extern bool get_var_test_safhe();
extern void set_var_test_safhe(bool value);
extern bool get_var_test_press1();
extern void set_var_test_press1(bool value);
extern bool get_var_test_press2();
extern void set_var_test_press2(bool value);
extern bool get_var_test_josh();
extern void set_var_test_josh(bool value);
extern bool get_var_test_zarbe1();
extern void set_var_test_zarbe1(bool value);
extern bool get_var_test_zarbe2();
extern void set_var_test_zarbe2(bool value);
extern const char *get_var_admin_password();
extern void set_var_admin_password(const char *value);
extern const char *get_var_admin_waiting_stutus_lable();
extern void set_var_admin_waiting_stutus_lable(const char *value);
extern bool get_var_admin_waiting_password_stutus();
extern void set_var_admin_waiting_password_stutus(bool value);
extern const char *get_var_app_version();
extern void set_var_app_version(const char *value);
extern const char *get_var_available_wifi_networks();
extern void set_var_available_wifi_networks(const char *value);
extern const char *get_var_connection_error_label();
extern void set_var_connection_error_label(const char *value);
extern const char *get_var_device_serial_number();
extern void set_var_device_serial_number(const char *value);
extern const char *get_var_download_lable_info_text();
extern void set_var_download_lable_info_text(const char *value);
extern bool get_var_download_panle_state();
extern void set_var_download_panle_state(bool value);
extern bool get_var_is_admin_password_verifyed();
extern void set_var_is_admin_password_verifyed(bool value);
extern bool get_var_is_wifi_connected();
extern void set_var_is_wifi_connected(bool value);
extern int32_t get_var_selected_wifi_index();
extern void set_var_selected_wifi_index(int32_t value);
extern const char *get_var_selected_wifi_network();
extern void set_var_selected_wifi_network(const char *value);
extern bool get_var_waitig_panle_state();
extern void set_var_waitig_panle_state(bool value);
extern const char *get_var_wifi_password();
extern void set_var_wifi_password(const char *value);
extern const char *get_var_wrong_password_atemp();
extern void set_var_wrong_password_atemp(const char *value);
extern bool get_var_ota_enable();
extern void set_var_ota_enable(bool value);
extern bool get_var_josh_is_working();
extern void set_var_josh_is_working(bool value);
extern int32_t get_var_josh_counter();
extern void set_var_josh_counter(int32_t value);
extern bool get_var_cool_is_working();
extern void set_var_cool_is_working(bool value);
extern int32_t get_var_cooling_counter();
extern void set_var_cooling_counter(int32_t value);
extern int32_t get_var_step_number();
extern void set_var_step_number(int32_t value);
extern int32_t get_var_frame_numbers();
extern void set_var_frame_numbers(int32_t value);
extern bool get_var_step_process();
extern void set_var_step_process(bool value);
extern bool get_var_thermocouple_error();
extern void set_var_thermocouple_error(bool value);
extern int32_t get_var_lang();
extern void set_var_lang(int32_t value);
extern double get_var_setting_jack_press_left_raft();
extern void set_var_setting_jack_press_left_raft(double value);
extern double get_var_setting_jack_press_right_raft();
extern void set_var_setting_jack_press_right_raft(double value);
extern bool get_var_curent_pedal_status();
extern void set_var_curent_pedal_status(bool value);
extern bool get_var_back_button_pressed();
extern void set_var_back_button_pressed(bool value);
extern bool get_var_current_back_button_status();
extern void set_var_current_back_button_status(bool value);
extern double get_var_temp_offset();
extern void set_var_temp_offset(double value);



#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/
