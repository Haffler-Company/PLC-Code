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
    FLOW_GLOBAL_VARIABLE_COMP_TIMER_VALUE = 0,
    FLOW_GLOBAL_VARIABLE_MAKE_TIMER_VALUE = 1,
    FLOW_GLOBAL_VARIABLE_PADDLE_TIMER_VALUE = 2,
    FLOW_GLOBAL_VARIABLE_COMP3PHASE_TIME_VALUE = 3,
    FLOW_GLOBAL_VARIABLE_COMP1PHASE_TIME_VALUE = 4,
    FLOW_GLOBAL_VARIABLE_CURRENT_SET1_VALUE = 5
};

// Native global variables

extern int32_t get_var_comp_timer_value();
extern void set_var_comp_timer_value(int32_t value);
extern int32_t get_var_make_timer_value();
extern void set_var_make_timer_value(int32_t value);
extern int32_t get_var_paddle_timer_value();
extern void set_var_paddle_timer_value(int32_t value);
extern int32_t get_var_comp3phase_time_value();
extern void set_var_comp3phase_time_value(int32_t value);
extern int32_t get_var_comp1phase_time_value();
extern void set_var_comp1phase_time_value(int32_t value);
extern int32_t get_var_current_set1_value();
extern void set_var_current_set1_value(int32_t value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/