#ifndef UI_HARDWARE_DIAGNOSTICS_H
#define UI_HARDWARE_DIAGNOSTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Screen object
extern lv_obj_t * ui_HardwareDiagnostics;

// Control widgets (Outputs)
extern lv_obj_t * ui_diag_btn_funnel;
extern lv_obj_t * ui_diag_lbl_funnel;
extern lv_obj_t * ui_diag_btn_piston;
extern lv_obj_t * ui_diag_lbl_piston;
extern lv_obj_t * ui_diag_btn_inv_fwd;
extern lv_obj_t * ui_diag_btn_inv_rev;
extern lv_obj_t * ui_diag_btn_buzzer;
extern lv_obj_t * ui_diag_btn_back;

// Status widgets (Inputs & Sensors)
extern lv_obj_t * ui_diag_led_funnel_sensor;
extern lv_obj_t * ui_diag_led_piston_sensor;
extern lv_obj_t * ui_diag_lbl_adc_f;
extern lv_obj_t * ui_diag_lbl_adc_m;
extern lv_obj_t * ui_diag_lbl_sys_health;

void ui_HardwareDiagnostics_screen_init(void);
void ui_HardwareDiagnostics_reset_cache(void);
void ui_HardwareDiagnostics_update(int rawF, int rawM, bool funnelSensor, bool pistonSensor);

#ifdef __cplusplus
}
#endif

#endif // UI_HARDWARE_DIAGNOSTICS_H
