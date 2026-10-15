// char WIFISSID[32] = "Haffler";
// char WIFIPass[32] = "haffler12";
//char ADPass[32] = "haffler12";

#include <SPIFFS.h>
#include <Wire.h>
#include <FS.h>
#include <LiquidCrystal.h>
#include <max6675.h>
#include <EEPROM.h>
#include <SPI.h>
#include <rom/rtc.h>
#include <esp_task_wdt.h>
#include "src/DS3231/DS3231.h"
#include <rom/rtc.h>
#include <esp_task_wdt.h>

#include "consts.h"

/* -=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=
//*                  --> ///////////  LOCAL_LIBRARIES MODE ///////////
                Skecth uses 2 Aditional Libraries that you have to install manually
TODO   Dont Worry if You don't need updated Libraries All you need is to set <LOCAL_LIBRARIES> --> true
?                 Additional Localized Libraries can be Found at ./src/<LibraryName>
-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=---=-=-==--=-=-=-=-=-==-=---=-=-==--=-=-=-=-=-===-==-=--=-=*/
#define LOCAL_LIBRARIES true //?
#if LOCAL_LIBRARIES
#include "src/ArduinoJson/ArduinoJson.h"
#include "src/WebSockets/WebSocketsServer.h"
#else
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#endif

//*-*-*-*-*-*-*-*-*-*-*-*-* functions -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

/////////////////////////////////////functions jamshidi //////////////
bool saveParams(int e, int c, int s, int pid, bool J, int _mot, int _abil, int _blade, int lang);
void setupPin(void);
void heater_contorl(void);
void upvc_welder(void);
void save_var(void);
void information(void);
void display_eeprom (void);
void select_function(void);
void average(void);
int  scan_key(void);
void Delay(int step_delay);
void update_f(void);
void update_lec(void);
void EMR(void);
void check_hardware(void);
int  readTemp(void);
bool read_key(int key_pin);
void read_write_eeprom(void);
void error_display(void);
void upvc_cleaner(void);
//void upvc_digital_meter(void);
//void steper_motor(float step_unit, int rotational_speed);
void check_lock_time (void);
void read_time_ds3231(void);
uint8_t readRegister(uint8_t reg);
void writeRegister(uint8_t reg, uint8_t value);
void setRTCDateTime();
int monthToNumber(String month);
long calculateDays(void);
uint8_t calculateDayOfWeek(void);
bool isLeapYear(int year);
//*-*-*-*-*-*-*-*-*-*-*-*-* END functions -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*


//*-*-*-*-*-*-*-*-*-*-*-*-*    define   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// #define stop_signal 0x01
// #define wifi_signal 0x04
// #define ok_signal 0x02
// #define set_signal 0x08
// #define temp_signal 0x10

// bool falt_ssr = 0;

#define ver 270

#define WDT_TIMEOUT 10

#define PasswordAdmin 1011228   // password = 011228
#define PasswordAdmin_2 1127730 // password = 127730
#define PasswordAddTime 1112233 // password = 112233

/////////////////////////////////////digital meter
#define L1 26
#define L2 27
#define L3 15
#define L4 25
#define min_speed_hz 100


#define New true // New = true , Old = flase
#if New
///////////////////////// New ////////////
#define ADC_KEY_1 33
#define ADC_KEY_2 32

#define move_jack 14
#define jack_left 26
#define jack_righ 04
#define jack_heat 25
#define shablon 13
#define heater 21
#define tap_move_1 15    /////// back_light_lcd  33
#define tap_move_2 27     
#define jack_left_2 15     
#define jack_righ_2 27

#define blade_mode_1 13
#define blade_mode_2 26

#define motor_1 04
#define motor_2 14
#define profile_holder_jack 21
#define motor_replace 15 /////// back_light_lcd  33
#define motor_blade_move 25
#define up_down_nails 27

#define Start        39
#define Stop         36
#define mic_sw_blade 34
#define mic_sw_motor 35

#define mic_end   mic_sw_blade
#define mic_first mic_sw_motor

#define Start_2   mic_sw_blade
#define back_ward mic_sw_motor

#define miso 19
#define mosi 23
#define scl 12

#define _rs 2
#define _en 5

#define d_4 16
#define d_5 17
#define d_6 22
#define d_7 18
#else
////////////////////// Old /////////////////////

#define ADC_KEY_1 34
#define ADC_KEY_2 32

#define move_jack 26
#define jack_left 15
#define jack_righ 27
#define jack_heat 21
#define shablon 25
#define heater 22
#define tap_move_1 33    /////// back_light_lcd  33
#define tap_move_2 14     
#define jack_left_2 33     
#define jack_righ_2 14

/////////////////////////////////////
#define blade_mode_1 25
#define blade_mode_2 15

#define motor_1 27
#define motor_2 26
#define profile_holder_jack 22
#define motor_replace 33 /////// back_light_lcd  33
#define motor_blade_move 21
#define up_down_nails 14

#define Start        13
#define Stop         35
#define mic_sw_blade 39
#define mic_sw_motor 36

#define mic_end   mic_sw_blade
#define mic_first mic_sw_motor

#define Start_2   36
#define back_ward 39

#define miso 19
#define mosi 23
#define scl 12

#define _rs 2
#define _en 4

#define d_4 16
#define d_5 17
#define d_6 5
#define d_7 18
#endif

#define F1 1
#define F2 2
#define F3 3
#define F4 4
#define F5 5
#define F6 6
#define F7 7
#define F8 8

#define english 1
#define turkce 2

#define PasswordAmin 1127730

#define Enable LOW
#define Disabled HIGH

typedef enum
{
    MODE_WELDER_SINGLE = 1,
    MODE_WELDER_DOUBLE = 2,
    MODE_CLEANER = 3,
    DIGITAL_METER_MODE = 4,
    MODES_SIZE
} Mode;

bool saveParams(int e, int c, int s, int pid, bool J, int _mot, int _abil, int _blade, int lang);
bool saveDeviceMode(Mode mode);
/////////////////////////////////
#define EEPROM_SIZE 640
////////////////////////////////

LiquidCrystal lcd(_rs, _en, d_4, d_5, d_6, d_7);
MAX6675 thermocouple(scl, mosi, miso);
DS3231 rtc;

byte customChar[] = {
    B01100,
    B10010,
    B10010,
    B01100,
    B00000,
    B00000,
    B00000,
    B00000};

byte customChar2[] = {
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B11111,
    B01110,
    B00100};

Mode device_mode = MODE_WELDER_SINGLE;
// MODE_WELDER_SINGLE
// MODE_CLEANER
// MODE_WELDER_DOUBLE
//  DIGITAL_METER_MODE
unsigned long int time_u_lcd = 0, ADC_1 = 0, ADC_2 = 0;
int e_up = 16, c_up = 235, s_up = 16, PID = 15;
int eTime = 0, cTemp = 0, sTime = 0, address = 0, c_up_copy = 0 , c_up_copy_n = 0 , welding_temp = 0;
int keyNum = 100, map1 = 0, map2 = 0, temp = 1000 , temp_copy = 0 , show_temp = -31999;
int ax_ready = 1200, ax_back = 1200, table_ready = 1000, table_back = 600 , heater_ready = 2100 , heater_back = 2100, press_ready = 600, press_back = 500, tap_jack_1 = 3, tap_jack_2 = 6;
int lock_time = 0, lock_timeing = 0, password = 1111111, lock_en = 0;
int temp_dis = 1000 , temp_dis_2 = 1000, temp_adj = 0 , en_show_temp = - 2000;
int check_var[40] , copy_time = 0;

bool isJ = false , con_temp = 0 , adding_time = 0 ,hardware_test = 1;
bool on_wi_fi = false, update_data = false , reset_temp = false;
bool p_stat = 0, p_stat_2 = 0 , p_stat_1 =0 , lcd_disabled = false ;
bool stat_down = 0;

int blade_mode = 1, ability = 1, motor_type = 1;
int language = english;
int blade_mode_copy = 1 , motor_type_copy = 1 , ch_mode = 1;

float last_location = 0;
float step_size = 1; // mm
bool start_meter = 0;
float Adjust_size = 0; //, move_speed = 1000;
int Coefficient = 0;
unsigned long int display_time_on = 0;
int  dayOfWeek = 0 , year = 0 , month = 0 , day = 0 , hour = 0 , minute = 0 , second = 0, temperature = 0;
bool val_ds3231 = 0, falt_ds3231 , no_battery = 0 , set_time_ds3231 = 0;

/*-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=
//!START --> /////////// webAPP Section ///////////
compact Library Wroted By Soheil Asami to make IOT Easy :)
-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=*/
#include "webApp.h"
#define SSID_NAME "Haffler 8 key ⚙️ " + String(ver / 100) + "." + String((ver % 100) / 10)
#define SSID_PASS "haffler.com"
#define HTTP_PORT 80
#define WS_PORT 81
#define FLAG_WEB_FILE_UPLOADER true
WebApp webApp(HTTP_PORT, WS_PORT, FLAG_WEB_FILE_UPLOADER, SSID_NAME, SSID_PASS);

/*-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=
//!END   --> \\\\\\\\\\\\ webAPP Section \\\\\\\\\\\\
-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=*/
