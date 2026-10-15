#include <SPIFFS.h>
#include <FS.h>
#include <EEPROM.h>
#include <SPI.h>
#include <max6675.h>
#include "src/DS3231/DS3231.h"
// #include <RTClib.h>
#include <Wire.h>
#include <AceTMI.h>      // SimpleTmi1637Interface
#include <AceSegment.h>  // Tm1637Module
#include <Adafruit_NeoPixel.h>
#include "consts.h"
#include <rom/rtc.h>
#include <esp_task_wdt.h>
//#include "Adafruit_MPR121.h" //////////////

/* -=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=
  //*                  --> ///////////  LOCAL_LIBRARIES MODE ///////////
                Skecth uses 2 Aditional Libraries that you have to install manually
  TODO   Dont Worry if You don't need updated Libraries All you need is to set <LOCAL_LIBRARIES> --> true
  ?                 Additional Localized Libraries can be Found at ./src/<LibraryName>
  -=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=---=-=-==--=-=-=-=-=-==-=---=-=-==--=-=-=-=-=-===-==-=--=-=*/
#define LOCAL_LIBRARIES true  //?
#if LOCAL_LIBRARIES
#include "src/ArduinoJson/ArduinoJson.h"
#include "src/WebSockets/WebSocketsServer.h"
// #include "src/Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#else
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
// #include <Adafruit_NeoPixel.h>
#endif

#define ver 460  // V.4.60
#define WDT_TIMEOUT 10
#define ON_ 52
//*-*-*-*-*-*-*-*-*-*-*-*-* functions -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
/////////////////////////////////////functions jamshidi //////////////
void heater_contorl(void);
void setupPin(void);
void rest_pin_out(void);
void upvc_welder(void);
void save_var(void);
void display_eeprom(void);
void select_function(void);
void display_eeprom(void);
void information(void);
void Delay(int step_delay);
void update_f(void);
void update_lec(void);
void EMR(void);
void check_hardware(void);
int readTemp(void);
bool read_key(int key_pin);
void read_write_eeprom(bool flash_eeprom);
void error_display(int step_error);
void display_eeprom(void);

void seg_1(int number, int bright);
void seg_2(int number, int bright);
void seg_3(int number, int bright);
void in_seg(int number, int select_seg);
void up_seg(int S_1, int S_2, int S_3, int S_4, int S_5, int S_6, int S_7, int bright);
void up_seg_step(int S_1, int S_2, int S_3, int S_4, int S_5, int S_6, int S_7, int bright);
void buzzer(int setp_beeb);
void buzzer_warning(int setp_beeb);
void rgb_teme(void);
void average(void);
int scan_key(void);
void demo(void);
void sement_off(void);
void check_lock_time(void);
uint32_t Wheel(byte WheelPos);
bool isLeapYear(int year);
long calculateDays(void);
void read_time_ds3231(void);
void setRTCDateTime(void);
int monthToNumber(String month);
void clear_memory(void);
uint8_t readRegister(uint8_t reg);
void writeRegister(uint8_t reg, uint8_t value);
uint8_t calculateDayOfWeek(void);
void fade_seven_sgment(void);
void admin_eeprom(void);
void rest_eeprom_admin(void);
bool check_jack(bool sensor_val);
int sen_read(int key_pin, bool sen_stat);
void programs_selection(void);
//*-*-*-*-*-*-*-*-*-*-*-*-* functions -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

/////////////////////////////////////
// #define move_jack 13
// #define jack_left 26
// #define jack_righ 32
// #define shablon   14
// #define heater    21
// #define jack_heat 25
// #define tap_move  33
// #define jack_left_2 33
// #define jack_righ_2 27

#define move_jack 14    // TABLA   OUT0
#define jack_left 26    // 2.PRES  OUT1
#define jack_righ 04    // 1.PRES  OUT2
#define jack_heat 25    // LAMA    OUT3
#define shablon 13      // REZ     OUT4
#define heater 21       // SSR     OUT5
#define tap_move_1 15   // 1OUT    OUT6
#define tap_move_2 27   // 1OUT    OUT7
#define jack_left_2 15  // 1OUT    OUT6
#define jack_righ_2 27  // 2OUT    OUT7

/////////////////////////////////////
// #define blade_mode_1         shablon
// #define blade_mode_2         jack_left
//
// #define motor_1              jack_righ
// #define motor_2              move_jack
// #define profile_holder_jack  heater
// #define motor_replace        tap_move_1
// #define motor_blade_move     jack_heat
// #define up_down_nails        jack_righ_2

/////////////////////////////////////digital meter
// #define L1 move_jack
// #define L2 jack_righ
// #define L3 jack_left
// #define L4 shablon
// #define min_speed_hz 100

#define Start 39
#define Stop 36
#define mic_sw_blade 34
#define mic_sw_motor 35

#define mic_end mic_sw_blade
#define mic_first mic_sw_motor

#define Start_2 mic_sw_blade
#define back_ward mic_sw_motor
#define back_ward_2 18

#define s_jack_shablon 35
#define s_jack_heater 34
#define s_jack_table 18

#define miso 19
#define mosi 23
#define scl 12

#define F1 1
#define F2 2
#define F3 3
#define F4 4
#define F5 5
#define F6 6
#define F7 7
#define F8 8
#define F9 9
#define F10 10

#define CLK_PIN_1 16
#define DIO_PIN_1 17

#define CLK_PIN_2 05
#define DIO_PIN_2 22

#define PIN_RGB 00
#define PIN_BUZZER  2  // 18

#define ADC_KEY_1 33
#define ADC_KEY_2 32

#define NUM_DIGITS 6
#define DELAY_MICROS 100

#define NUMPIXELS 12

#define PasswordAdmin 1011228    // password = 011228
#define PasswordAdmin_2 1127730  // password = 127730
#define PasswordAddTime 1112233  // password = 112233

#define Enable LOW
#define Disabled HIGH

#define stop_signal 0x01
#define wifi_signal 0x00  //0x04
#define error_div   0x04
#define ok_signal   0x02
#define set_signal  0x00  //0x08
#define temp_signal 0x10

#define all_key_on  0xE8
#define all_key_off 0x17

#define key_start   0x08
#define key_stop    0x20
#define key_temp    0x40
#define key_melting 0x80
#define key_welding 0x01

#define A_ 21
#define B_ 22
#define C_ 23
#define c_ 47
#define D_ 24
#define E_ 25
#define F_ 26
#define G_ 27
#define H_ 28
#define I_ 29
#define J_ 30
#define K_ 31
#define L_ 32
#define M_ 33
#define N_ 34
#define O_ 35
#define o_ 48
#define E_E 49
#define P_ 36
#define Q_ 37
#define R_ 38
#define S_ 39
#define T_ 40
#define U_ 41
#define V_ 42
#define W_ 43
#define X_ 44
#define Y_ 45
#define Z_ 46
#define _ 50
#define D_D  51
#define rate 52
#define L_L  53
#define OFF_ 20

// down_seg(U_, 2 + 10, 8, ON_, 7);  ver : 2.8

typedef enum {
  MODE_WELDER_SINGLE = 1,
  MODE_WELDER_DOUBLE = 2,
  MODE_CLEANER = 3,
  DIGITAL_METER_MODE = 4,
  MODES_SIZE
} Mode;

bool saveParams(int e, int c, int s, int pid, bool J, int _mot, int _abil, int _blade, int lang);
bool saveDeviceMode(Mode mode);
/////////////////////////////////
#define EEPROM_SIZE 1000
////////////////////////////////

using ace_segment::Tm1637Module;
using ace_tmi::SimpleTmi1637Interface;

using TmiInterface_tm_1 = SimpleTmi1637Interface;
TmiInterface_tm_1 tmiInterface_tm_1(DIO_PIN_1, CLK_PIN_1, DELAY_MICROS);
Tm1637Module<TmiInterface_tm_1, NUM_DIGITS> ledModule_1(tmiInterface_tm_1);

using TmiInterface_tm_2 = SimpleTmi1637Interface;
TmiInterface_tm_2 tmiInterface_tm_2(DIO_PIN_2, CLK_PIN_2, DELAY_MICROS);
Tm1637Module<TmiInterface_tm_2, NUM_DIGITS> ledModule_2(tmiInterface_tm_2);

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);

MAX6675 thermocouple(scl, mosi, miso);

//Adafruit_MPR121 cap = Adafruit_MPR121();

DS3231 rtc;

const uint32_t COLOR_RGB[11] = {
  0Xffffff,  // white         //0
  0X3f48cc,  // indigo        //1
  0Xffff00,  // yellow        //2
  0X20aba6,  // blue & green  //3
  0X008a27,  // green dark    //4
  0X0aff0a,  // green         //5
  0X00ffff,  // turguoise     //6
  0Xff0000,  // red           //7
  0X0000ff,  // blue          //8
  0Xff00ff,  // pink          //9
  0X000000,  // black         //10
};
const byte test_seg[] = {
  0b00000001,  //   A
  0b00000010,  //   B
  0b00000100,  //   C
  0b00001000,  //   D
  0b00010000,  //   E
  0b00100000,  //   F
  0b01000000,  //   G
  0b10000000,  //   H
  0b11111111   //   ALL
};
const byte pin_seg[] = {
  4,
  3,
  0,
  2,
  4,
  3,
  5
};
const byte alphabetPattern[] = {
  0b00111111,  // 0   -  0
  0b00000110,  // 1   -  1
  0b01011011,  // 2   -  2
  0b01001111,  // 3   -  3
  0b01100110,  // 4   -  4
  0b01101101,  // 5   -  5
  0b01111101,  // 6   -  6
  0b00000111,  // 7   -  7
  0b01111111,  // 8   -  8
  0b01101111,  // 9   -  9
  0b10111111,  // 0.  -  10
  0b10000110,  // 1.  -  11
  0b11011011,  // 2.  -  12
  0b11001111,  // 3.  -  13
  0b11100110,  // 4.  -  14
  0b11101101,  // 5.  -  15
  0b11111101,  // 6.  -  16
  0b10000111,  // 7.  -  17
  0b11111111,  // 8.  -  18
  0b11101111,  // 9.  -  19
  B00000000,   // off -  20
  B01110111,   // A    -  21
  B01111100,   // B    -  22
  B00111001,   // C    -  23
  B01011110,   // D    -  24
  B01111001,   // E    -  25
  B01110001,   // F    -  26
  B01111101,   // G    -  27
  B01110110,   // H    -  28
  B00000110,   // I    -  29
  B00011110,   // J    -  30
  B01111000,   // K    -  31
  B00111000,   // L    -  32
  B01110101,   // M    -  33
  B00110111,   // N    -  34
  B00111111,   // O    -  35
  B01110011,   // P    -  36
  B01100111,   // Q    -  37
  B01010000,   // R    -  38
  B01101101,   // S    -  39
  B01111000,   // T    -  40
  B00111110,   // U    -  41
  B00111110,   // V    -  42
  B01111110,   // W    -  43
  B01110110,   // X    -  44
  B01101110,   // Y    -  45
  B01011011,   // Z    -  46
  B01011000,   // c    -  47
  B01011100,   // o    -  48
  B01001000,   // =    -  49
  B01000000,   // -    -  50
  B10000000,   // .    -  51
  B01100011,   // ^    -  52
  B01001001    // ---  -  53
};

Mode device_mode = MODE_WELDER_SINGLE;
// MODE_WELDER_SINGLE
// MODE_CLEANER
// MODE_WELDER_DOUBLE
// DIGITAL_METER_MODE
unsigned long int time_u_lcd = -500, ADC_1 = 0, ADC_2 = 0;
int e_up[6] = { 1, 20, 18, 16, 14, 10 };
int c_up[6] = { 1, 205, 215, 230, 235, 240 };
int s_up[6] = { 1, 20, 18, 16, 14, 10 };
int PID = 15;
int eTime[6] = { 0, 0, 0, 0, 0, 0 };
int cTemp[6] = { 0, 0, 0, 0, 0, 0 };
int sTime[6] = { 0, 0, 0, 0, 0, 0 };
int address = 0, c_up_copy = 0, c_up_copy_n = 0, welding_temp = 0;
int keyNum = 100, keyNum_copy = 100, map1 = 0, map2 = 0, temp = 1000, show_temp = -31999;
int ax_ready = 1200, ax_back = 1200, table_ready = 1000, table_back = 600, heater_ready = 2100, heater_back = 2100, press_ready = 600, press_back = 500, tap_jack_1 = 3, tap_jack_2 = 6;  //! HELLO
int lock_time = 0, lock_timeing = 0, password_p = 1111111, lock_en = 0;
int temp_dis = 1000, temp_dis_2 = 1000, temp_adj = 0;
int copy_time = 0, buzzer_en = 1, led_stat = 0, led_stat_copy = 0 , led_stat_2 = 0;

int seven_seg_1_copy = 10000, seven_seg_2_copy = 10000, seven_seg_3_copy = 10000;
int program_no = 1, p_function = 0, step_rgb = -50, enter_time = -10000;
bool p_buzzer = false, p_buzzer_2 = false, dontcare_f10 = 0, en_pedal = 0, set_faun_val = 0;

bool isJ = false, con_temp = 0, adding_time = 0, hardware_test = 1;
bool on_wi_fi = false, update_data = false, finish_time = true;
bool p_stat = 0, p_stat_1 = 0, p_stat_2 = 0, p_stat_3 = 0, lcd_disabled = false;
bool stat_down = 0, ref_key_num = 0, dontcare_stop = 0, stat_stop = 1, val_ok = 0;
bool updat_data_lcd = 1, enter_function = 0, auto_start = 1;

int blade_mode = 1, ability = 1, motor_type = 1;
int firstPixelHue = 0;
int blade_mode_copy = 1, motor_type_copy = 1, ch_mode = 1;
int intterupt_timer = 0, color_rgb_sin = 0, temp_offset_int = 0;

int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, dayOfWeek = 9, temperature = 100;
bool val_ds3231 = 0, no_battery = 0, heater_en = 1, set_time_ds3231 = 0, falt_ds3231 = 0 , val_mpr121 = 0;
bool falt_ssr = 0, falt_ssr_copy = 0, break_rgb_fun = 0;

int unit_space = 0, number_of_saves = 0;
float unit_welding = 0, unit_welding_p = 0, temp_offset = 1;
int work_time = 0, temp_copy = 0, lan_ = 1, temp_last = 0 ,  step_stat_rgb = 12;
bool e_s_heater = 0 , e_s_tabel = 0 , e_s_shablon = 0 , start_av = 1, falt_over_temp = 0;
// bool century = false;
// DateTime now;

enum JACKID {
  JACK_PAGE = 0,
  JACK_SHABOLUN = 1,
  JACK_PROFILE = 2,
  JACK_WELDING = 3,
  JACK_CLEANER_1 = 4,
  JACK_CLEANER_2 = 5,
};
struct Config {
  int PID;
  int tempCalib;
  bool spk;
  bool aut;
  bool end;
  bool emg;
  bool tLmt;
  uint8_t pedalCount;
};
struct Jack {
  JACKID id;
  int load;
  int unload;
};
enum PROGRAMID {
  P_1 = 1,
  P_2 = 2,
  P_3 = 3,
  P_4 = 4,
  P_5 = 5
};
struct Program {
  PROGRAMID id;
  int low;
  int high;
  int temp;
};
uint8_t pedal_Count = 3;
bool jackSensore_enabled = false;

void onUpdateDVmode(Mode dvMode) {
  if (dvMode == MODE_WELDER_SINGLE)
    device_mode = Mode(1);
  if (dvMode == MODE_WELDER_DOUBLE)
  {
    device_mode = Mode(2);
    jackSensore_enabled = 0;
  }
  update_data = true;
  save_var();
}
void onUpdateJKmode(bool _isJ) {
  isJ = _isJ;
  update_data = true;
  save_var();
}
void onUpdateEmgncymode(bool emg) {
  stat_stop = emg;
  update_data = true;
  save_var();
}
void onUpdateJackSensore(bool _enable) {
  if (device_mode == MODE_WELDER_SINGLE)
  {
    jackSensore_enabled = _enable;  // TODO --> is jack sensor
  } else jackSensore_enabled = 0;
  update_data = true;
  save_var();
}
void onUpdateConfig(Config config) {
  if (config.PID >= 0 && config.PID <= 30)
    PID = config.PID;
  if (0 < temp_dis_2 + config.tempCalib - 1) {
    temp_dis_2 = temp_dis_2 / temp_offset;
    temp_offset = (config.tempCalib + temp_dis_2) / float(temp_dis_2);

    temp_offset_int = round((temp_offset * temp_dis_2) - temp_dis_2);

    temp_dis_2 = temp_dis_2 * temp_offset;
  }
  buzzer_en = config.spk;    // EN speaker
  auto_start = config.aut;   // stat automatic
  finish_time = config.end;  // step finsh
  // stat_stop = config.emg;
  val_ok = config.tLmt;             // limet temp
  pedal_Count = config.pedalCount;  // TODO -->  config.pedalCount  <3 OR 4>
  update_data = true;
  Serial.println(temp_offset);
  Serial.println(temp_offset_int);
  save_var();
}
void onUpdateJack(Jack jack) {
  int load = jack.load;
  int unload = jack.unload;
  if (map1 == 0) {
    switch (jack.id) {
      case JACK_PAGE:
        table_ready = load;
        table_back = unload;
        break;
      case JACK_SHABOLUN:
        ax_ready = load;
        ax_back = unload;
        break;
      case JACK_PROFILE:
        press_ready = load;
        press_back = unload;
        break;
      case JACK_WELDING:
        heater_ready = load;
        heater_back = unload;
        break;
      case JACK_CLEANER_1:
        tap_jack_2 = load;
        // unload;
        break;
      case JACK_CLEANER_2:
        tap_jack_1 = load;
        // unload;
        break;

      default:
        break;
    }
    update_data = true;
    save_var();
  }
}
void onStartJack(int jackIndex) {
  Serial.println("ON jack");
  if (map1 == 0) {
    switch (jackIndex) {
      case JACK_PAGE:
        digitalWrite(move_jack, Enable);
        break;
      case JACK_SHABOLUN:
        digitalWrite(shablon, Enable);
        break;
      case JACK_PROFILE:
        digitalWrite(jack_righ, Enable);
        digitalWrite(jack_left, Enable);
        break;
      case JACK_WELDING:
        digitalWrite(jack_heat, Enable);
        break;
      case JACK_CLEANER_1:
        digitalWrite(jack_righ_2, Enable);
        break;
      case JACK_CLEANER_2:
        digitalWrite(jack_left_2, Enable);
        break;

      default:
        Serial.println("flase slect jack");
        break;
    }
    buzzer_warning(2);
  }
}
void onStopJack() {
  Serial.println("OFF jack");
  if (map1 == 0)
    rest_pin_out();
}
void setCurrentProgramById(int programIndex) {
  if (program_no <= 5 && program_no >= 0)
    program_no = programIndex;  // programIndex is between 1 and 5
  update_data = true;
  save_var();
}
void onUpdateProgram(Program program) {
  if (program.low < 100 && program.low > 0)
    s_up[program.id] = program.low;
  if (program.high < 100 && program.high > 0)
    e_up[program.id] = program.high;
  if (program.temp < 600 && program.temp > 0)
    c_up[program.id] = program.temp;
  update_data = true;
  save_var();
}
void updateKillSwitch(bool isOn, uint32_t days) {
  lock_en = isOn;
  if (isOn != 0 && days > 0) {
    lock_time = days + calculateDays();  // days is between 1 and 200
    lock_timeing = calculateDays();

    EEPROM.writeInt(180, year);
    EEPROM.writeInt(184, month);
    EEPROM.writeInt(188, day);
    EEPROM.writeInt(192, days);
    delay(10);
    EEPROM.commit();
    delay(10);
    Serial.print("Activation Time Of the Timer Lock = ");
    Serial.println(String(EEPROM.readInt(180)) + "." + String(EEPROM.readInt(184)) + "." + String(EEPROM.readInt(188)));

    Serial.print("Lock Duration(Day) = ");
    Serial.println(EEPROM.readInt(192));
  } else {
    lock_time = 0;  // days is between 1 and 200
    lock_timeing = 0;
  }
  adding_time = 0;

  update_data = true;
  save_var();
}
bool checkPassword(String password) {
  return (password == String(password_p) || password == String(PasswordAdmin) || password == String(PasswordAdmin_2));
}
bool onChangePassword(uint32_t newPassword) {
  if (newPassword < 10000000)
    password_p = newPassword;
  update_data = true;
  save_var();
  return true;
}
void onResetCounter() {
  unit_welding = 0;
  unit_welding_p = int(unit_welding_p);
  update_data = true;
  save_var();
}
void onResetCounterAll() {
  unit_welding = 0;
  unit_welding_p = 0;
  update_data = true;
  save_var();
}
void onRessetFactory() {
  Serial.println("RessetFactory");
  read_write_eeprom(1);
  display_eeprom();
  buzzer_warning(2);
}
void onsaveDefaults() {
  Serial.println("Save Val Admin");
  admin_eeprom();
  delay(10);
  EEPROM.commit();
  delay(10);
  read_write_eeprom(1);
  display_eeprom();
  buzzer_warning(2);
}
////////////////**********************************////////////////////

/*-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=
  //!START --> /////////// webAPP Section ///////////
  compact Library Wroted By Soheil Asami to make IOT Easy :)
  -=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=*/
#include "webApp.h"
/////////*****************************************///////////////////
//#define SSID_NAME "Turantek PLC 5k ⚙️ " + String(ver / 100) + "." + String((ver % 100) / 10)
//#define SSID_PASS "turantek"
#define SSID_NAME "Haffler PLC 5k ⚙️ " + String(ver / 100) + "." + String((ver % 100) / 10)
#define SSID_PASS "haffler.com"
//#define SSID_NAME "UPVC PLC 5k ⚙️ " + String(ver / 100) + "." + String((ver % 100) / 10)
//#define SSID_PASS "upvcplc12"
/////////*****************************************///////////////////
#define HTTP_PORT 80
#define WS_PORT 81
#define FLAG_WEB_FILE_UPLOADER true
WebApp webApp(HTTP_PORT, WS_PORT, FLAG_WEB_FILE_UPLOADER, SSID_NAME, SSID_PASS);

/*-=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=
  //!END   --> \\\\\\\\\\\\ webAPP Section \\\\\\\\\\\\
  -=-==-=--=-==-=--=-=-==--=-=-=-=-=-==-=--=-==-=--=-=*/
