#include <string.h>
#include "vars.h"
#include "defines.h"

char selected_wifi_network[100] = { 0 };

const char *get_var_selected_wifi_network() {
    return selected_wifi_network;
}

void set_var_selected_wifi_network(const char *value) {
    if (!value) { selected_wifi_network[0] = 0; return; }
    strncpy(selected_wifi_network, value, sizeof(selected_wifi_network) / sizeof(char));
    selected_wifi_network[sizeof(selected_wifi_network) / sizeof(char) - 1] = 0;
}

///////////////////////////////


char available_wifi_networks[512] = { 0 };

const char *get_var_available_wifi_networks() {
    return available_wifi_networks;
}

void set_var_available_wifi_networks(const char *value) {
    if (!value) { available_wifi_networks[0] = 0; return; }
    strncpy(available_wifi_networks, value, sizeof(available_wifi_networks) / sizeof(char));
    available_wifi_networks[sizeof(available_wifi_networks) / sizeof(char) - 1] = 0;
}

///////////////////////////////////

char connection_error_label[100] = { 0 };

const char *get_var_connection_error_label() {
    return connection_error_label;
}

void set_var_connection_error_label(const char *value) {
    if (!value) { connection_error_label[0] = 0; return; }
    strncpy(connection_error_label, value, sizeof(connection_error_label) / sizeof(char));
    connection_error_label[sizeof(connection_error_label) / sizeof(char) - 1] = 0;
}

/////////////////////////////////

int32_t selected_wifi_index;

int32_t get_var_selected_wifi_index() {
    return selected_wifi_index;
}

void set_var_selected_wifi_index(int32_t value) {
    selected_wifi_index = value;
}

//////////////////////////////////


char wifi_password[100] = { 0 };

const char *get_var_wifi_password() {
    return wifi_password;
}

void set_var_wifi_password(const char *value) {
    if (!value) { wifi_password[0] = 0; return; }
    strncpy(wifi_password, value, sizeof(wifi_password) / sizeof(char));
    wifi_password[sizeof(wifi_password) / sizeof(char) - 1] = 0;
}

////////////////////////////////////////


bool waitig_panle_state;

bool get_var_waitig_panle_state() {
    return waitig_panle_state;
}

void set_var_waitig_panle_state(bool value) {
    waitig_panle_state = value;
}

/////////////////////////////////////////

char device_serial_number[100] = { 0 };

const char *get_var_device_serial_number() {
    return device_serial_number;
}

void set_var_device_serial_number(const char *value) {
    if (!value) { device_serial_number[0] = 0; return; }
    strncpy(device_serial_number, value, sizeof(device_serial_number) / sizeof(char));
    device_serial_number[sizeof(device_serial_number) / sizeof(char) - 1] = 0;
}

////////////////////////////////////////////////


bool is_wifi_connected;

bool get_var_is_wifi_connected() {
    return is_wifi_connected;
}

void set_var_is_wifi_connected(bool value) {
    is_wifi_connected = value;
}

/////////////////////////////////////////////////
char download_lable_info_text[256] = { 0 };   // ۱۰۰ کم بود — URL تا ~۲۰۰ کاراکتر + پیشوند

const char *get_var_download_lable_info_text() {
    return download_lable_info_text;
}

void set_var_download_lable_info_text(const char *value) {
    if (!value) { download_lable_info_text[0] = 0; return; }
    strncpy(download_lable_info_text, value, sizeof(download_lable_info_text) / sizeof(char));
    download_lable_info_text[sizeof(download_lable_info_text) / sizeof(char) - 1] = 0;
}

/////////////////////////////////////////////////////////
bool download_panle_state;

bool get_var_download_panle_state() {
    return download_panle_state;
}

void set_var_download_panle_state(bool value) {
    download_panle_state = value;
}

/////////////////////////////////////////////////////

bool is_admin_password_verifyed;

bool get_var_is_admin_password_verifyed() {
    return is_admin_password_verifyed;
}

void set_var_is_admin_password_verifyed(bool value) {
    is_admin_password_verifyed = value;
}


////////////////////////////////////////////////

char app_version[100] = { 0 };

const char *get_var_app_version() {
    return app_version;
}

void set_var_app_version(const char *value) {
    if (!value) { app_version[0] = 0; return; }
    strncpy(app_version, value, sizeof(app_version) / sizeof(char));
    app_version[sizeof(app_version) / sizeof(char) - 1] = 0;
}

/////////////////////////////////////////////////////////////////////
char wrong_password_atemp[100] = { 0 };

const char *get_var_wrong_password_atemp() {
    return wrong_password_atemp;
}

void set_var_wrong_password_atemp(const char *value) {
    if (!value) { wrong_password_atemp[0] = 0; return; }
    strncpy(wrong_password_atemp, value, sizeof(wrong_password_atemp) / sizeof(char));
    wrong_password_atemp[sizeof(wrong_password_atemp) / sizeof(char) - 1] = 0;
}

/////////////////////////////////////////////////////////

char admin_password[100] = { 0 };

const char *get_var_admin_password() {
    return admin_password;
}

void set_var_admin_password(const char *value) {
    if (!value) { admin_password[0] = 0; return; }
    strncpy(admin_password, value, sizeof(admin_password) / sizeof(char));
    admin_password[sizeof(admin_password) / sizeof(char) - 1] = 0;
}


/////////////////////////////////////////////

char admin_waiting_stutus_lable[100] = { 0 };

const char *get_var_admin_waiting_stutus_lable() {
    return admin_waiting_stutus_lable;
}

void set_var_admin_waiting_stutus_lable(const char *value) {
    if (!value) { admin_waiting_stutus_lable[0] = 0; return; }
    strncpy(admin_waiting_stutus_lable, value, sizeof(admin_waiting_stutus_lable) / sizeof(char));
    admin_waiting_stutus_lable[sizeof(admin_waiting_stutus_lable) / sizeof(char) - 1] = 0;
}


//////////////////////////////////////////

bool admin_waiting_password_stutus;

bool get_var_admin_waiting_password_stutus() {
    return admin_waiting_password_stutus;
}

void set_var_admin_waiting_password_stutus(bool value) {
    admin_waiting_password_stutus = value;
}