/**
 * @file thermal_pid.h
 * @brief Industrial PID Temperature Controller for Heating Element
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float ki;
    float kd;
    float setpoint;
    float integral;
    float prev_error;
    float output_min;
    float output_max;
    uint32_t last_time_ms;
} thermal_pid_controller_t;

void thermal_pid_init(thermal_pid_controller_t *pid, float kp, float ki, float kd, float min_out, float max_out);
void thermal_pid_set_setpoint(thermal_pid_controller_t *pid, float setpoint);
float thermal_pid_compute(thermal_pid_controller_t *pid, float current_temp, uint32_t current_time_ms);
void thermal_pid_reset(thermal_pid_controller_t *pid);

#ifdef __cplusplus
}
#endif
