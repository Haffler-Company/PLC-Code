#ifndef T_EEZ_LVGL_UI_VARS_H
#define T_EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_ANBL = 0,
    FLOW_GLOBAL_VARIABLE_BUZZER = 1,
    FLOW_GLOBAL_VARIABLE_Q1 = 2,
    FLOW_GLOBAL_VARIABLE_Q2 = 3,
    FLOW_GLOBAL_VARIABLE_Q3 = 4,
    FLOW_GLOBAL_VARIABLE_Q4 = 5,
    FLOW_GLOBAL_VARIABLE_Q5 = 6,
    FLOW_GLOBAL_VARIABLE_Q6 = 7,
    FLOW_GLOBAL_VARIABLE_Q7 = 8,
    FLOW_GLOBAL_VARIABLE_Q8 = 9,
    FLOW_GLOBAL_VARIABLE_Q9 = 10,
    FLOW_GLOBAL_VARIABLE_Q10 = 11,
    FLOW_GLOBAL_VARIABLE_F1 = 12,
    FLOW_GLOBAL_VARIABLE_F2 = 13,
    FLOW_GLOBAL_VARIABLE_F3 = 14,
    FLOW_GLOBAL_VARIABLE_F4 = 15,
    FLOW_GLOBAL_VARIABLE_F5 = 16,
    FLOW_GLOBAL_VARIABLE_F6 = 17,
    FLOW_GLOBAL_VARIABLE_F7 = 18,
    FLOW_GLOBAL_VARIABLE_F8 = 19,
    FLOW_GLOBAL_VARIABLE_F9 = 20,
    FLOW_GLOBAL_VARIABLE_F10 = 21,
    FLOW_GLOBAL_VARIABLE__15 = 22,
    FLOW_GLOBAL_VARIABLE__14 = 23,
    FLOW_GLOBAL_VARIABLE__13 = 24,
    FLOW_GLOBAL_VARIABLE__12 = 25,
    FLOW_GLOBAL_VARIABLE__11 = 26,
    FLOW_GLOBAL_VARIABLE__10 = 27,
    FLOW_GLOBAL_VARIABLE__16 = 28,
    FLOW_GLOBAL_VARIABLE__17 = 29,
    FLOW_GLOBAL_VARIABLE_SEND = 30,
    FLOW_GLOBAL_VARIABLE_RECEIVE = 31,
    FLOW_GLOBAL_VARIABLE_RECEIVE_MESSAGE = 32,
    FLOW_GLOBAL_VARIABLE_SEND_MESSAGE = 33,
    FLOW_GLOBAL_VARIABLE_ENABLEBLE = 34,
    FLOW_GLOBAL_VARIABLE_SCANWIFI = 35,
    FLOW_GLOBAL_VARIABLE_CHANGBLELABL = 36,
    FLOW_GLOBAL_VARIABLE_NEW = 37,
    FLOW_GLOBAL_VARIABLE_TOUCH_ERROR = 38,
    FLOW_GLOBAL_VARIABLE_RTC_ERROR = 39,
    FLOW_GLOBAL_VARIABLE_RTC_TEXT = 40,
    FLOW_GLOBAL_VARIABLE_RTC_APPLY = 41,
    FLOW_GLOBAL_VARIABLE_TIME_TEXT = 42,
    FLOW_GLOBAL_VARIABLE_PWM_STATE = 43,
    FLOW_GLOBAL_VARIABLE_SETUPLOADED = 44,
    FLOW_GLOBAL_VARIABLE_SSRLOADED = 45,
    FLOW_GLOBAL_VARIABLE_RSTEXTSEND = 46
};

// Native global variables

extern bool get_var_anbl();
extern void set_var_anbl(bool value);
extern int32_t get_var_buzzer();
extern void set_var_buzzer(int32_t value);
extern bool get_var_q1();
extern void set_var_q1(bool value);
extern bool get_var_q2();
extern void set_var_q2(bool value);
extern bool get_var_q3();
extern void set_var_q3(bool value);
extern bool get_var_q4();
extern void set_var_q4(bool value);
extern bool get_var_q5();
extern void set_var_q5(bool value);
extern bool get_var_q6();
extern void set_var_q6(bool value);
extern bool get_var_q7();
extern void set_var_q7(bool value);
extern bool get_var_q8();
extern void set_var_q8(bool value);
extern bool get_var_q9();
extern void set_var_q9(bool value);
extern bool get_var_q10();
extern void set_var_q10(bool value);
extern bool get_var_f1();
extern void set_var_f1(bool value);
extern bool get_var_f2();
extern void set_var_f2(bool value);
extern bool get_var_f3();
extern void set_var_f3(bool value);
extern bool get_var_f4();
extern void set_var_f4(bool value);
extern bool get_var_f5();
extern void set_var_f5(bool value);
extern bool get_var_f6();
extern void set_var_f6(bool value);
extern bool get_var_f7();
extern void set_var_f7(bool value);
extern bool get_var_f8();
extern void set_var_f8(bool value);
extern bool get_var_f9();
extern void set_var_f9(bool value);
extern bool get_var_f10();
extern void set_var_f10(bool value);
extern bool get_var__15();
extern void set_var__15(bool value);
extern bool get_var__14();
extern void set_var__14(bool value);
extern bool get_var__13();
extern void set_var__13(bool value);
extern bool get_var__12();
extern void set_var__12(bool value);
extern bool get_var__11();
extern void set_var__11(bool value);
extern bool get_var__10();
extern void set_var__10(bool value);
extern bool get_var__16();
extern void set_var__16(bool value);
extern bool get_var__17();
extern void set_var__17(bool value);
extern bool get_var_send();
extern void set_var_send(bool value);
extern bool get_var_receive();
extern void set_var_receive(bool value);
extern bool get_var_receive_message();
extern void set_var_receive_message(bool value);
extern bool get_var_send_message();
extern void set_var_send_message(bool value);
extern bool get_var_enableble();
extern void set_var_enableble(bool value);
extern bool get_var_scanwifi();
extern void set_var_scanwifi(bool value);
extern bool get_var_changblelabl();
extern void set_var_changblelabl(bool value);
extern bool get_var_new();
extern void set_var_new(bool value);
extern bool get_var_touch_error();
extern void set_var_touch_error(bool value);
extern bool get_var_rtc_error();
extern void set_var_rtc_error(bool value);
extern const char *get_var_rtc_text();
extern void set_var_rtc_text(const char *value);
extern bool get_var_rtc_apply();
extern void set_var_rtc_apply(bool value);
extern const char *get_var_time_text();
extern void set_var_time_text(const char *value);
extern bool get_var_pwm_state();
extern void set_var_pwm_state(bool value);
extern bool get_var_setuploaded();
extern void set_var_setuploaded(bool value);
extern bool get_var_ssrloaded();
extern void set_var_ssrloaded(bool value);
extern const char *get_var_rstextsend();
extern void set_var_rstextsend(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*T_EEZ_LVGL_UI_VARS_H*/