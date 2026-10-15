#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_select_as_key_board_earia_l(lv_event_t * e);
extern void action_start_button_pressed(lv_event_t * e);
extern void action_stop_button_pressed(lv_event_t * e);
extern void action_select_as_key_board_earia_h(lv_event_t * e);
extern void action_select_as_key_board_earia_d(lv_event_t * e);
extern void action_select_as_key_board_earia_p(lv_event_t * e);
extern void action_gotosetting(lv_event_t * e);
extern void action_gotohome(lv_event_t * e);
extern void action_start_button_pressed1(lv_event_t * e);
extern void action_stop_button_pressed1(lv_event_t * e);
extern void action_clear_dimensions_left(lv_event_t * e);
extern void action_clear_dimensions_right(lv_event_t * e);
extern void action_select_as_key_board_earia_l1(lv_event_t * e);
extern void action_select_as_key_board_earia_h1(lv_event_t * e);
extern void action_select_as_key_board_earia_d1(lv_event_t * e);
extern void action_select_as_key_board_earia_p1(lv_event_t * e);
extern void action_select_as_key_board_earia_p2(lv_event_t * e);
extern void action_select_as_key_board_earia_qs(lv_event_t * e);
extern void action_save_button_pressed(lv_event_t * e);
extern void action_keyboard_pressed(lv_event_t * e);
extern void action_dimension_value_changed(lv_event_t * e);
extern void action_enter_pressd_1(lv_event_t * e);
extern void action_close_main_keyboard(lv_event_t * e);
extern void action_tab_pressed1(lv_event_t * e);
extern void action_buzzer_sw_pressed(lv_event_t * e);
extern void action_touch_sw_pressed(lv_event_t * e);
extern void action_type_out_sw_pressed(lv_event_t * e);
extern void action_valve1_sw_pressed(lv_event_t * e);
extern void action_valve2_sw_pressed(lv_event_t * e);
extern void action_buzzer_out_sw_pressed(lv_event_t * e);
extern void action_buzzer_pressed(lv_event_t * e);
extern void action_enter_pressed_setting(lv_event_t * e);
extern void action_lang_changed(lv_event_t * e);

void calculate_all();
void ChangeBar(unsigned char BarValue);
void ChangeBar1(unsigned char BarValue);
void HideLogo(void);
void Set_SWTypeOutPut(char type);
void ApplyOutputTypeSafe(char type);
void ChangeBuzzerAvailble(bool Active_Buzzer);
void ChangeTouchFeedback(bool Active_Touch);
void ChangeLanguage(char language);
void a2_eeprom_load_to_ui(void);
void a2_eeprom_save_dimensions_from_ui(void);
void a2_eeprom_save_process_from_ui(void);
/***********************MyAction***********************/

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
