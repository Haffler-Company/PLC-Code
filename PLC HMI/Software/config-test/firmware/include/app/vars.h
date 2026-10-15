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
    FLOW_GLOBAL_VARIABLE_NONE
};

// Native global variables

extern const char *get_var_selected_wifi_network();
extern void set_var_selected_wifi_network(const char *value);
extern const char *get_var_available_wifi_networks();
extern void set_var_available_wifi_networks(const char *value);
extern const char *get_var_connection_error_label();
extern void set_var_connection_error_label(const char *value);
extern int32_t get_var_selected_wifi_index();
extern void set_var_selected_wifi_index(int32_t value);
extern const char *get_var_wifi_password();
extern void set_var_wifi_password(const char *value);
extern bool get_var_waitig_panle_state();
extern void set_var_waitig_panle_state(bool value);
extern const char *get_var_device_serial_number();
extern void set_var_device_serial_number(const char *value);
extern bool get_var_is_wifi_connected();
extern void set_var_is_wifi_connected(bool value);
extern const char *get_var_download_lable_info_text();
extern void set_var_download_lable_info_text(const char *value);
extern bool get_var_download_panle_state();
extern void set_var_download_panle_state(bool value);
extern bool get_var_is_admin_password_verifyed();
extern void set_var_is_admin_password_verifyed(bool value);
extern const char *get_var_app_version();
extern void set_var_app_version(const char *value);
extern const char *get_var_wrong_password_atemp();
extern void set_var_wrong_password_atemp(const char *value);
extern const char *get_var_admin_password();
extern void set_var_admin_password(const char *value);
extern const char *get_var_admin_waiting_stutus_lable();
extern void set_var_admin_waiting_stutus_lable(const char *value);
extern bool get_var_admin_waiting_password_stutus();
extern void set_var_admin_waiting_password_stutus(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/