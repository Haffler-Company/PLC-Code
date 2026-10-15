#include "vars.h"
#include <string.h>

static int32_t clamp_int32(int32_t value, int32_t min_value, int32_t max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

int32_t element_temperature;

int32_t get_var_element_temperature() {
    return element_temperature;
}

void set_var_element_temperature(int32_t value) {
    element_temperature = value;
}

///////////////////////////////////////

int32_t element_temperature_target;

int32_t get_var_element_temperature_target() {
    return element_temperature_target;
}

void set_var_element_temperature_target(int32_t value) {
    element_temperature_target = clamp_int32(value, 0, 350);
}

/////////////////////////////////////////

int32_t welding_time;

int32_t get_var_welding_time() {
    return welding_time;
}

void set_var_welding_time(int32_t value) {
    welding_time = clamp_int32(value, 1, 9999);
}

//////////////////////////////////

double jack_profil;

double get_var_jack_profil() {
    return jack_profil;
}

void set_var_jack_profil(double value) {
    jack_profil = value;
}

/////////////////////////////////////

double jack_shablon;

double get_var_jack_shablon() {
    return jack_shablon;
}

void set_var_jack_shablon(double value) {
    jack_shablon = value;
}

//////////////////////////////////////

int32_t cooling_time;

int32_t get_var_cooling_time() {
    return cooling_time;
}

void set_var_cooling_time(int32_t value) {
    cooling_time = clamp_int32(value, 1, 9999);
}

///////////////////////////////////////

bool emergency_stop;

bool get_var_emergency_stop() {
    return emergency_stop;
}

void set_var_emergency_stop(bool value) {
    emergency_stop = value;
}

////////////////////////////////////////

char enable_mode[100] = { 0 };

const char *get_var_enable_mode() {
    return enable_mode;
}

void set_var_enable_mode(const char *value) {
    strncpy(enable_mode, value, sizeof(enable_mode) / sizeof(char));
    enable_mode[sizeof(enable_mode) / sizeof(char) - 1] = 0;
}

/////////////////////////////////////////

bool cycel_started;

bool get_var_cycel_started() {
    return cycel_started;
}

void set_var_cycel_started(bool value) {
    cycel_started = value;
}

//////////////////////////////////////////

int32_t setting_general_temperature_caliber;

int32_t get_var_setting_general_temperature_caliber() {
    return setting_general_temperature_caliber;
}

void set_var_setting_general_temperature_caliber(int32_t value) {
    setting_general_temperature_caliber = value;
}

//////////////////////////////////////

int32_t setting_general_pid;

int32_t get_var_setting_general_pid() {
    return setting_general_pid;
}

void set_var_setting_general_pid(int32_t value) {
    setting_general_pid = value;
}

/////////////////////////////////////

bool setting_general_speaker;

bool get_var_setting_general_speaker() {
    return setting_general_speaker;
}

void set_var_setting_general_speaker(bool value) {
    setting_general_speaker = value;
}

bool setting_general_wifi;

bool get_var_setting_general_wifi() {
    return setting_general_wifi;
}

void set_var_setting_general_wifi(bool value) {
    setting_general_wifi = value;
}

/////////////////////////////////////

bool setting_general_automatic_start;

bool get_var_setting_general_automatic_start() {
    return setting_general_automatic_start;
}

void set_var_setting_general_automatic_start(bool value) {
    setting_general_automatic_start = value;
}

////////////////////////////////////////

bool setting_general_final_jack;

bool get_var_setting_general_final_jack() {
    return setting_general_final_jack;
}

void set_var_setting_general_final_jack(bool value) {
    setting_general_final_jack = value;
}

bool setting_general_sensor_jack;

bool get_var_setting_general_sensor_jack() {
    return setting_general_sensor_jack;
}

void set_var_setting_general_sensor_jack(bool value) {
    setting_general_sensor_jack = value;
}

/////////////////////////////////////
bool setting_general_temperature_limit;

bool get_var_setting_general_temperature_limit() {
    return setting_general_temperature_limit;
}

void set_var_setting_general_temperature_limit(bool value) {
    setting_general_temperature_limit = value;
}

/////////////////////////////////

bool setting_general_pedal_number2;

bool get_var_setting_general_pedal_number2() {
    return setting_general_pedal_number2;
}

void set_var_setting_general_pedal_number2(bool value) {
    setting_general_pedal_number2 = value;
}

//////////////////////////////////

bool setting_general_pedal_number3;

bool get_var_setting_general_pedal_number3() {
    return setting_general_pedal_number3;
}

void set_var_setting_general_pedal_number3(bool value) {
    setting_general_pedal_number3 = value;
}

//////////////////////////////////

bool setting_admin_thermocouple_j;

bool get_var_setting_admin_thermocouple_j() {
    return setting_admin_thermocouple_j;
}

void set_var_setting_admin_thermocouple_j(bool value) {
    setting_admin_thermocouple_j = value;
}

////////////////////////////////////

bool setting_admin_thermocouple_k;

bool get_var_setting_admin_thermocouple_k() {
    return setting_admin_thermocouple_k;
}

void set_var_setting_admin_thermocouple_k(bool value) {
    setting_admin_thermocouple_k = value;
}

////////////////////////////////////

int32_t setting_admin_countdown_timer;

int32_t get_var_setting_admin_countdown_timer() {
    return setting_admin_countdown_timer;
}

void set_var_setting_admin_countdown_timer(int32_t value) {
    setting_admin_countdown_timer = value;
}

/////////////////////////////////////

bool setting_admin_emergency_switch;

bool get_var_setting_admin_emergency_switch() {
    return setting_admin_emergency_switch;
}

void set_var_setting_admin_emergency_switch(bool value) {
    setting_admin_emergency_switch = value;
}

//////////////////////////////////////

bool setting_admin_jack_sensor;

bool get_var_setting_admin_jack_sensor() {
    return setting_admin_jack_sensor;
}

void set_var_setting_admin_jack_sensor(bool value) {
    setting_admin_jack_sensor = value;
}

///////////////////////////////////////

char setting_admin_password_placeholder[100] = { 0 };

const char *get_var_setting_admin_password_placeholder() {
    return setting_admin_password_placeholder;
}

void set_var_setting_admin_password_placeholder(const char *value) {
    strncpy(setting_admin_password_placeholder, value, sizeof(setting_admin_password_placeholder) / sizeof(char));
    setting_admin_password_placeholder[sizeof(setting_admin_password_placeholder) / sizeof(char) - 1] = 0;
}

///////////////////////////////////////

char setting_admin_re_enter_password_placeholder[100] = { 0 };

const char *get_var_setting_admin_re_enter_password_placeholder() {
    return setting_admin_re_enter_password_placeholder;
}

void set_var_setting_admin_re_enter_password_placeholder(const char *value) {
    strncpy(setting_admin_re_enter_password_placeholder, value, sizeof(setting_admin_re_enter_password_placeholder) / sizeof(char));
    setting_admin_re_enter_password_placeholder[sizeof(setting_admin_re_enter_password_placeholder) / sizeof(char) - 1] = 0;
}

////////////////////////////////////////

char setting_admin_pass_error[100] = { 0 };

const char *get_var_setting_admin_pass_error() {
    return setting_admin_pass_error;
}

void set_var_setting_admin_pass_error(const char *value) {
    strncpy(setting_admin_pass_error, value, sizeof(setting_admin_pass_error) / sizeof(char));
    setting_admin_pass_error[sizeof(setting_admin_pass_error) / sizeof(char) - 1] = 0;
}

////////////////////////////////////////

char admin_pass[100] = { 0 };

const char *get_var_admin_pass() {
    return admin_pass;
}

void set_var_admin_pass(const char *value) {
    strncpy(admin_pass, value, sizeof(admin_pass) / sizeof(char));
    admin_pass[sizeof(admin_pass) / sizeof(char) - 1] = 0;
}

//////////////////////////////////////////

char jack_map[100] = { 0 };

const char *get_var_jack_map() {
    return jack_map;
}

void set_var_jack_map(const char *value) {
    strncpy(jack_map, value, sizeof(jack_map) / sizeof(char));
    jack_map[sizeof(jack_map) / sizeof(char) - 1] = 0;
}

////////////////////////////////////////

double setting_jack_safhe_raft;

double get_var_setting_jack_safhe_raft() {
    return setting_jack_safhe_raft;
}

void set_var_setting_jack_safhe_raft(double value) {
    setting_jack_safhe_raft = value;
}

//////////////////////////////////

double setting_jack_safhe_bargasht;

double get_var_setting_jack_safhe_bargasht() {
    return setting_jack_safhe_bargasht;
}

void set_var_setting_jack_safhe_bargasht(double value) {
    setting_jack_safhe_bargasht = value;
}

////////////////////////////////////////

double setting_jack_shablon_raft;

double get_var_setting_jack_shablon_raft() {
    return setting_jack_shablon_raft;
}

void set_var_setting_jack_shablon_raft(double value) {
    setting_jack_shablon_raft = value;
}

///////////////////////////////////
double setting_jack_shablon_bargasht;

double get_var_setting_jack_shablon_bargasht() {
    return setting_jack_shablon_bargasht;
}

void set_var_setting_jack_shablon_bargasht(double value) {
    setting_jack_shablon_bargasht = value;
}

/////////////////////////////////////////

double setting_jack_profil_raft;

double get_var_setting_jack_profil_raft() {
    return setting_jack_profil_raft;
}

void set_var_setting_jack_profil_raft(double value) {
    setting_jack_profil_raft = value;
}

//////////////////////////////////////////////

double setting_jack_profil_bargasht;

double get_var_setting_jack_profil_bargasht() {
    return setting_jack_profil_bargasht;
}

void set_var_setting_jack_profil_bargasht(double value) {
    setting_jack_profil_bargasht = value;
}

//////////////////////////////////

double setting_jack_josh_raft;

double get_var_setting_jack_josh_raft() {
    return setting_jack_josh_raft;
}

void set_var_setting_jack_josh_raft(double value) {
    setting_jack_josh_raft = value;
}
///////////////////////////////////////
double setting_jack_josh_bargasht;

double get_var_setting_jack_josh_bargasht() {
    return setting_jack_josh_bargasht;
}

void set_var_setting_jack_josh_bargasht(double value) {
    setting_jack_josh_bargasht = value;
}

///////////////////////////////////////

double jack_dot2_zarbe1_raft;

double get_var_jack_dot2_zarbe1_raft() {
    return jack_dot2_zarbe1_raft;
}

void set_var_jack_dot2_zarbe1_raft(double value) {
    jack_dot2_zarbe1_raft = value;
}
//////////////////////////////////////

double jack_dot2_zarbe2_raft;

double get_var_jack_dot2_zarbe2_raft() {
    return jack_dot2_zarbe2_raft;
}

void set_var_jack_dot2_zarbe2_raft(double value) {
    jack_dot2_zarbe2_raft = value;
}

//////////////////////////////////////////

bool admin_pass_verifyed;

bool get_var_admin_pass_verifyed() {
    return admin_pass_verifyed;
}

void set_var_admin_pass_verifyed(bool value) {
    admin_pass_verifyed = value;
}

////////////////////////////////////

bool admin_can_save;

bool get_var_admin_can_save() {
    return admin_can_save;
}

void set_var_admin_can_save(bool value) {
    admin_can_save = value;
}

/////////////////////////////////////

char software_version[100] = { 0 };

const char *get_var_software_version() {
    return software_version;
}

void set_var_software_version(const char *value) {
    strncpy(software_version, value, sizeof(software_version) / sizeof(char));
    software_version[sizeof(software_version) / sizeof(char) - 1] = 0;
}

////////////////////////////////////

char header_time_text[16] = { 0 };

const char *get_var_header_time_text() {
    return header_time_text;
}

void set_var_header_time_text(const char *value) {
    strncpy(header_time_text, value, sizeof(header_time_text) / sizeof(char));
    header_time_text[sizeof(header_time_text) / sizeof(char) - 1] = 0;
}

////////////////////////////////////

char header_date_text[16] = { 0 };

const char *get_var_header_date_text() {
    return header_date_text;
}

void set_var_header_date_text(const char *value) {
    strncpy(header_date_text, value, sizeof(header_date_text) / sizeof(char));
    header_date_text[sizeof(header_date_text) / sizeof(char) - 1] = 0;
}

////////////////////////////////////

char cpu_temperature_text[16] = { 0 };

const char *get_var_cpu_temperature_text() {
    return cpu_temperature_text;
}

void set_var_cpu_temperature_text(const char *value) {
    strncpy(cpu_temperature_text, value, sizeof(cpu_temperature_text) / sizeof(char));
    cpu_temperature_text[sizeof(cpu_temperature_text) / sizeof(char) - 1] = 0;
}

////////////////////////////////////

char internal_temperature_text[16] = { 0 };

const char *get_var_internal_temperature_text() {
    return internal_temperature_text;
}

void set_var_internal_temperature_text(const char *value) {
    strncpy(internal_temperature_text, value, sizeof(internal_temperature_text) / sizeof(char));
    internal_temperature_text[sizeof(internal_temperature_text) / sizeof(char) - 1] = 0;
}

////////////////////////////////////

char second_temperature_text[16] = { 0 };

const char *get_var_second_temperature_text() {
    return second_temperature_text;
}

void set_var_second_temperature_text(const char *value) {
    strncpy(second_temperature_text, value, sizeof(second_temperature_text) / sizeof(char));
    second_temperature_text[sizeof(second_temperature_text) / sizeof(char) - 1] = 0;
}

////////////////////////////////////

int32_t total_frame_numbers;

int32_t get_var_total_frame_numbers() {
    return total_frame_numbers;
}

void set_var_total_frame_numbers(int32_t value) {
    total_frame_numbers = value;
}

////////////////////////////////////

int32_t admin_set_hour;

int32_t get_var_admin_set_hour() {
    return admin_set_hour;
}

void set_var_admin_set_hour(int32_t value) {
    admin_set_hour = value;
}

////////////////////////////////////

int32_t admin_set_minute;

int32_t get_var_admin_set_minute() {
    return admin_set_minute;
}

void set_var_admin_set_minute(int32_t value) {
    admin_set_minute = value;
}

////////////////////////////////////

int32_t admin_set_year;

int32_t get_var_admin_set_year() {
    return admin_set_year;
}

void set_var_admin_set_year(int32_t value) {
    admin_set_year = value;
}

////////////////////////////////////

int32_t admin_set_month;

int32_t get_var_admin_set_month() {
    return admin_set_month;
}

void set_var_admin_set_month(int32_t value) {
    admin_set_month = value;
}

////////////////////////////////////

int32_t admin_set_day;

int32_t get_var_admin_set_day() {
    return admin_set_day;
}

void set_var_admin_set_day(int32_t value) {
    admin_set_day = value;
}

////////////////////////////////////

char update_error_text[100] = { 0 };

const char *get_var_update_error_text() {
    return update_error_text;
}

void set_var_update_error_text(const char *value) {
    strncpy(update_error_text, value, sizeof(update_error_text) / sizeof(char));
    update_error_text[sizeof(update_error_text) / sizeof(char) - 1] = 0;

}

//////////////////////////////////////

int32_t progress_vlaue;

int32_t get_var_progress_vlaue() {
    return progress_vlaue;
}

void set_var_progress_vlaue(int32_t value) {
    progress_vlaue = value;
}

bool numeric_keyboard_open;
bool get_var_numeric_keyboard_open() { return numeric_keyboard_open; }
void set_var_numeric_keyboard_open(bool value) { numeric_keyboard_open = value; }

///////////////////////////////////////

int32_t res_countdown;

int32_t get_var_res_countdown() {
    return res_countdown;
}

void set_var_res_countdown(int32_t value) {
    res_countdown = value;
}


////////////////////////////////////////ashkan

int step_number;

int get_var_step_number() {
    return step_number;
}

void set_var_step_number(int value) {
    step_number = value;
}
///////////////////////////////////////

bool step_process;

bool get_var_step_process() {
    return step_process;
}

void set_var_step_process(bool value) {
    step_process = value;
}

////////////////////////////////////////


bool curent_pedal_status;

bool get_var_curent_pedal_status() {
    return curent_pedal_status;
}

void set_var_curent_pedal_status(bool value) {
    curent_pedal_status = value;
}


////////////////////////////////////////


bool back_button_pressed;

bool get_var_back_button_pressed() {
    return back_button_pressed;
}

void set_var_back_button_pressed(bool value) {
    back_button_pressed = value;
}

////////////////////////////////////////


bool current_back_button_status;

bool get_var_current_back_button_status() {
    return current_back_button_status;
}

void set_var_current_back_button_status(bool value) {
    current_back_button_status = value;
}
///////////////////////////////////////



double setting_jack_press_right_raft;

double get_var_setting_jack_press_right_raft() {
    return setting_jack_press_right_raft;
}

void set_var_setting_jack_press_right_raft(double value) {
    setting_jack_press_right_raft = value;
}
////////////////////////////////////////


double setting_jack_press_left_raft;

double get_var_setting_jack_press_left_raft() {
    return setting_jack_press_left_raft;
}

void set_var_setting_jack_press_left_raft(double value) {
    setting_jack_press_left_raft = value;
}


////////////////////////////////////

bool josh_out_display;

bool get_var_josh_out_display() {
    return josh_out_display;
}

void set_var_josh_out_display(bool value) {
    josh_out_display = value;
}

bool shablon_out_display;

bool get_var_shablon_out_display() {
    return shablon_out_display;
}

void set_var_shablon_out_display(bool value) {
    shablon_out_display = value;
}

bool safhe_out_display;

bool get_var_safhe_out_display() {
    return safhe_out_display;
}

void set_var_safhe_out_display(bool value) {
    safhe_out_display = value;
}

bool press1_out_display;

bool get_var_press1_out_display() {
    return press1_out_display;
}

void set_var_press1_out_display(bool value) {
    press1_out_display = value;
}


bool press2_out_display;

bool get_var_press2_out_display() {
    return press2_out_display;
}

void set_var_press2_out_display(bool value) {
    press2_out_display = value;
}


bool zarbe1_out_display;

bool get_var_zarbe1_out_display() {
    return zarbe1_out_display;
}

void set_var_zarbe1_out_display(bool value) {
    zarbe1_out_display = value;
}


bool zarbe2_out_display;

bool get_var_zarbe2_out_display() {
    return zarbe2_out_display;
}

void set_var_zarbe2_out_display(bool value) {
    zarbe2_out_display = value;
}

bool header_show_date;

bool get_var_header_show_date() {
    return header_show_date;
}

void set_var_header_show_date(bool value) {
    header_show_date = value;
}

bool test_mode;
bool test_ssr;
bool test_shablon;
bool test_safhe;
bool test_press1;
bool test_press2;
bool test_josh;
bool test_zarbe1;
bool test_zarbe2;

bool get_var_test_mode() { return test_mode; }
void set_var_test_mode(bool value) { test_mode = value; }
bool get_var_test_ssr() { return test_ssr; }
void set_var_test_ssr(bool value) { test_ssr = value; }
bool get_var_test_shablon() { return test_shablon; }
void set_var_test_shablon(bool value) { test_shablon = value; }
bool get_var_test_safhe() { return test_safhe; }
void set_var_test_safhe(bool value) { test_safhe = value; }
bool get_var_test_press1() { return test_press1; }
void set_var_test_press1(bool value) { test_press1 = value; }
bool get_var_test_press2() { return test_press2; }
void set_var_test_press2(bool value) { test_press2 = value; }
bool get_var_test_josh() { return test_josh; }
void set_var_test_josh(bool value) { test_josh = value; }
bool get_var_test_zarbe1() { return test_zarbe1; }
void set_var_test_zarbe1(bool value) { test_zarbe1 = value; }
bool get_var_test_zarbe2() { return test_zarbe2; }
void set_var_test_zarbe2(bool value) { test_zarbe2 = value; }

//ota_ashkan


/////////////////////////////////////////////////////////

char admin_password[100] = { 0 };

const char *get_var_admin_password() {
    return admin_password;
}

void set_var_admin_password(const char *value) {
    strncpy(admin_password, value, sizeof(admin_password) / sizeof(char));
    admin_password[sizeof(admin_password) / sizeof(char) - 1] = 0;
}



/////////////////////////////////////////////

char admin_waiting_stutus_lable[100] = { 0 };

const char *get_var_admin_waiting_stutus_lable() {
    return admin_waiting_stutus_lable;
}

void set_var_admin_waiting_stutus_lable(const char *value) {
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

////////////////////////////////////////////////

char app_version[100] = { 0 };

const char *get_var_app_version() {
    return app_version;
}

void set_var_app_version(const char *value) {
    strncpy(app_version, value, sizeof(app_version) / sizeof(char));
    app_version[sizeof(app_version) / sizeof(char) - 1] = 0;
}

///////////////////////////////


char available_wifi_networks[1024] = { 0 };

const char *get_var_available_wifi_networks() {
    return available_wifi_networks;
}

void set_var_available_wifi_networks(const char *value) {
    if (!value) value = "";
    strncpy(available_wifi_networks, value, sizeof(available_wifi_networks) / sizeof(char));
    available_wifi_networks[sizeof(available_wifi_networks) / sizeof(char) - 1] = 0;
}
///////////////////////////////////

char connection_error_label[192] = { 0 };

const char *get_var_connection_error_label() {
    return connection_error_label;
}

void set_var_connection_error_label(const char *value) {
    strncpy(connection_error_label, value, sizeof(connection_error_label) / sizeof(char));
    connection_error_label[sizeof(connection_error_label) / sizeof(char) - 1] = 0;
}
/////////////////////////////////////////

char device_serial_number[100] = { 0 };

const char *get_var_device_serial_number() {
    return device_serial_number;
}

void set_var_device_serial_number(const char *value) {
    strncpy(device_serial_number, value, sizeof(device_serial_number) / sizeof(char));
    device_serial_number[sizeof(device_serial_number) / sizeof(char) - 1] = 0;
}
/////////////////////////////////////////////////
char download_lable_info_text[100] = { 0 };

const char *get_var_download_lable_info_text() {
    return download_lable_info_text;
}

void set_var_download_lable_info_text(const char *value) {
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


bool is_wifi_connected;

bool get_var_is_wifi_connected() {
    return is_wifi_connected;
}

void set_var_is_wifi_connected(bool value) {
    is_wifi_connected = value;
}

/////////////////////////////////

int32_t selected_wifi_index;

int32_t get_var_selected_wifi_index() {
    return selected_wifi_index;
}

void set_var_selected_wifi_index(int32_t value) {
    selected_wifi_index = value;
}
char selected_wifi_network[100] = { 0 };

const char *get_var_selected_wifi_network() {
    return selected_wifi_network;
}

void set_var_selected_wifi_network(const char *value) {
    if (!value) value = "";
    strncpy(selected_wifi_network, value, sizeof(selected_wifi_network) / sizeof(char));
    selected_wifi_network[sizeof(selected_wifi_network) / sizeof(char) - 1] = 0;
}
bool waitig_panle_state;

bool get_var_waitig_panle_state() {
    return waitig_panle_state;
}

void set_var_waitig_panle_state(bool value) {
    waitig_panle_state = value;
}

char wifi_password[100] = { 0 };

const char *get_var_wifi_password() {
    return wifi_password;
}

void set_var_wifi_password(const char *value) {
    if (!value) value = "";
    strncpy(wifi_password, value, sizeof(wifi_password) / sizeof(char));
    wifi_password[sizeof(wifi_password) / sizeof(char) - 1] = 0;
}
/////////////////////////////////////////////////////////////////////
char wrong_password_atemp[100] = { 0 };

const char *get_var_wrong_password_atemp() {
    return wrong_password_atemp;
}

void set_var_wrong_password_atemp(const char *value) {
    if (!value) value = "";
    strncpy(wrong_password_atemp, value, sizeof(wrong_password_atemp) / sizeof(char));
    wrong_password_atemp[sizeof(wrong_password_atemp) / sizeof(char) - 1] = 0;
}

////////////////////////////////////////////////

bool ota_enable;

bool get_var_ota_enable() {
    return ota_enable;
}

void set_var_ota_enable(bool value) {
    ota_enable = value;
}
////////////////////////////////////////

bool josh_is_working;

bool get_var_josh_is_working() {
    return josh_is_working;
}

void set_var_josh_is_working(bool value) {
    josh_is_working = value;
}
////////////////////////////////////////

bool cool_is_working;

bool get_var_cool_is_working() {
    return cool_is_working;
}

void set_var_cool_is_working(bool value) {
    cool_is_working = value;
}
////////////////////////////////////////

///////////////////////////////////////

int32_t josh_counter;

int32_t get_var_josh_counter() {
    return josh_counter;
}

void set_var_josh_counter(int32_t value) {
    josh_counter = value;
}

////////////////////////////////////////



int32_t cooling_counter;

int32_t get_var_cooling_counter() {
    return cooling_counter;
}

void set_var_cooling_counter(int32_t value) {
    cooling_counter = value;
}
////////////////////////////////////////



int32_t frame_numbers;

int32_t get_var_frame_numbers() {
    return frame_numbers;
}

void set_var_frame_numbers(int32_t value) {
    frame_numbers = value;
}
////////////////////////////////////////

bool thermocouple_error;

bool get_var_thermocouple_error() {
    return thermocouple_error;
}

void set_var_thermocouple_error(bool value) {
    thermocouple_error = value;
}
////////////////////////////////////////



double temp_offset;

double get_var_temp_offset() {
    return temp_offset;
}

void set_var_temp_offset(double value) {
    temp_offset = value;
}


//////////////////////////////////////////

int32_t lang;

int32_t get_var_lang() {
    return lang;
}

void set_var_lang(int32_t value) {
    lang = value;
}
///////////////////////////////////////
bool back_button_pressed = false;
bool current_back_button_status = false;
