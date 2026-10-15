#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>


#ifndef GLOBAL_H
#define GLOBAL_H
extern volatile bool printertest;
extern volatile bool stopstate;
extern volatile bool stopstate_save_request;
extern volatile uint32_t bl_pulse_until_ms;
extern volatile uint32_t buzzer_hold_until_ms;
extern volatile bool ref_failed_flag;
extern volatile bool ref_wrong_dir_flag;
extern volatile int ref_fail_reason;
extern volatile bool ref_error_ui_active;
#endif


#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_NONE
};

// Native global variables



#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/
