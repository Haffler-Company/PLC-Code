#ifndef T_EEZ_LVGL_UI_SCREENS_H
#define T_EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _t_objects_t {
    lv_obj_t *main;
    lv_obj_t *setup;
    lv_obj_t *rs485;
    lv_obj_t *gsm;
    lv_obj_t *bluetooth;
    lv_obj_t *wifi;
    lv_obj_t *display_w;
    lv_obj_t *display_b;
    lv_obj_t *display_test_r;
    lv_obj_t *display_g;
    lv_obj_t *display_blue;
    lv_obj_t *sys_info;
    lv_obj_t *touch_test;
    lv_obj_t *clock_setup;
    lv_obj_t *ssr;
    lv_obj_t *tobj0;
    lv_obj_t *tobj1;
    lv_obj_t *tobj2;
    lv_obj_t *tobj3;
    lv_obj_t *tobj4;
    lv_obj_t *tobj5;
    lv_obj_t *tobj6;
    lv_obj_t *tobj7;
    lv_obj_t *tobj8;
    lv_obj_t *tobj9;
    lv_obj_t *tobj10;
    lv_obj_t *tobj11;
    lv_obj_t *tobj12;
    lv_obj_t *tobj13;
    lv_obj_t *tobj14;
    lv_obj_t *tobj15;
    lv_obj_t *tobj16;
    lv_obj_t *tobj17;
    lv_obj_t *tobj18;
    lv_obj_t *tobj19;
    lv_obj_t *tobj20;
    lv_obj_t *tobj21;
    lv_obj_t *tobj22;
    lv_obj_t *tobj23;
    lv_obj_t *tobj24;
    lv_obj_t *tobj25;
    lv_obj_t *tobj26;
    lv_obj_t *tobj27;
    lv_obj_t *tobj28;
    lv_obj_t *tobj29;
    lv_obj_t *tobj30;
    lv_obj_t *m;
    lv_obj_t *h;
    lv_obj_t *yer;
    lv_obj_t *mont;
    lv_obj_t *dy;
    lv_obj_t *tobj31;
    lv_obj_t *tobj32;
    lv_obj_t *tobj33;
    lv_obj_t *t1;
    lv_obj_t *t2;
    lv_obj_t *tobj34;
    lv_obj_t *espt;
    lv_obj_t *tobj35;
    lv_obj_t *tobj36;
    lv_obj_t *readfs_1;
    lv_obj_t *tobj37;
    lv_obj_t *tobj38;
    lv_obj_t *tobj39;
    lv_obj_t *readfs;
    lv_obj_t *tobj40;
    lv_obj_t *tobj41;
    lv_obj_t *tobj42;
    lv_obj_t *tobj43;
    lv_obj_t *tobj44;
    lv_obj_t *tobj45;
    lv_obj_t *tobj46;
    lv_obj_t *tobj47;
    lv_obj_t *rtctemp;
    lv_obj_t *tobj48;
    lv_obj_t *tobj49;
    lv_obj_t *tobj50;
    lv_obj_t *exp_button;
    lv_obj_t *tobj51;
    lv_obj_t *v14_15;
    lv_obj_t *v10_11;
    lv_obj_t *v12_13;
    lv_obj_t *v16;
    lv_obj_t *v17;
    lv_obj_t *tobj52;
    lv_obj_t *tobj53;
    lv_obj_t *tobj54;
    lv_obj_t *tobj55;
    lv_obj_t *datareceive;
    lv_obj_t *tobj56;
    lv_obj_t *tobj57;
    lv_obj_t *tobj58;
    lv_obj_t *rssend;
    lv_obj_t *tobj59;
    lv_obj_t *tobj60;
    lv_obj_t *receivemessage;
    lv_obj_t *sendmessage;
    lv_obj_t *tobj61;
    lv_obj_t *tobj62;
    lv_obj_t *gsmrefresh;
    lv_obj_t *tobj63;
    lv_obj_t *bolotosinfo;
    lv_obj_t *bolotosstate;
    lv_obj_t *tobj64;
    lv_obj_t *tobj65;
    lv_obj_t *tobj66;
    lv_obj_t *tobj67;
    lv_obj_t *tobj68;
    lv_obj_t *tobj69;
    lv_obj_t *tobj70;
    lv_obj_t *wifi1;
    lv_obj_t *wifi2;
    lv_obj_t *tobj71;
    lv_obj_t *wifi0;
    lv_obj_t *tobj72;
    lv_obj_t *tobj73;
    lv_obj_t *tobj74;
    lv_obj_t *tobj75;
    lv_obj_t *tobj76;
    lv_obj_t *sys_cpu;
    lv_obj_t *sys_wifi;
    lv_obj_t *sys_uptime;
    lv_obj_t *sys_ram;
    lv_obj_t *sys_flash;
    lv_obj_t *sys_i2c;
    lv_obj_t *sys_chipmac;
    lv_obj_t *tobj77;
    lv_obj_t *tobj78;
    lv_obj_t *tobj79;
    lv_obj_t *tobj80;
    lv_obj_t *tobj81;
    lv_obj_t *ta_rtc;
    lv_obj_t *tobj82;
    lv_obj_t *tobj83;
    lv_obj_t *tobj84;
    lv_obj_t *tobj85;
    lv_obj_t *tobj86;
    lv_obj_t *tobj87;
    lv_obj_t *tobj88;
    lv_obj_t *tobj89;
    lv_obj_t *tobj90;
    lv_obj_t *pwm_time_text_tarea;
    lv_obj_t *tobj91;
    lv_obj_t *tobj92;
    lv_obj_t *tobj93;
    lv_obj_t *tobj94;
    lv_obj_t *tobj95;
    lv_obj_t *tobj96;
    lv_obj_t *esptssr;
    lv_obj_t *esptssr_1;
    lv_obj_t *t1ssr;
    lv_obj_t *esptssr_2;
    lv_obj_t *t2ssr;
    lv_obj_t *esptssr_3;
    lv_obj_t *ssr_signal_lamp;  // لامپ سیگنال PWM (چشمک‌زن)
} t_objects_t;

extern t_objects_t t_objects;

enum T_ScreensEnum {
    T_SCREEN_ID_MAIN = 1,
    T_SCREEN_ID_SETUP = 2,
    T_SCREEN_ID_RS485 = 3,
    T_SCREEN_ID_GSM = 4,
    T_SCREEN_ID_BLUETOOTH = 5,
    T_SCREEN_ID_WIFI = 6,
    T_SCREEN_ID_DISPLAY_W = 7,
    T_SCREEN_ID_DISPLAY_B = 8,
    T_SCREEN_ID_DISPLAY_TEST_R = 9,
    T_SCREEN_ID_DISPLAY_G = 10,
    T_SCREEN_ID_DISPLAY_BLUE = 11,
    T_SCREEN_ID_SYS_INFO = 12,
    T_SCREEN_ID_TOUCH_TEST = 13,
    T_SCREEN_ID_CLOCK_SETUP = 14,
    T_SCREEN_ID_SSR = 15,
};

void create_screen_main();
void tick_screen_main();

void create_screen_setup();
void tick_screen_setup();

void create_screen_rs485();
void tick_screen_rs485();

void create_screen_gsm();
void tick_screen_gsm();

void create_screen_bluetooth();
void tick_screen_bluetooth();

void create_screen_wifi();
void tick_screen_wifi();

void create_screen_display_w();
void tick_screen_display_w();

void create_screen_display_b();
void tick_screen_display_b();

void create_screen_display_test_r();
void tick_screen_display_test_r();

void create_screen_display_g();
void tick_screen_display_g();

void create_screen_display_blue();
void tick_screen_display_blue();

void create_screen_sys_info();
void tick_screen_sys_info();

void create_screen_touch_test();
void tick_screen_touch_test();

void create_screen_clock_setup();
void tick_screen_clock_setup();

void create_screen_ssr();
void tick_screen_ssr();

void t_tick_screen_by_id(enum T_ScreensEnum screenId);
void t_tick_screen(int screen_index);

void t_create_screens();
void ssr_validate_and_commit(void);
void rtc_cycle_focus(void);


#ifdef __cplusplus
}
#endif

#endif /*T_EEZ_LVGL_UI_SCREENS_H*/
