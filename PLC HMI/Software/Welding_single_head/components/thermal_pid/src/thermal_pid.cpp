/**
 * @file thermal_pid.cpp
 * @brief Industrial PID Temperature Controller Implementation
 */

#include "thermal_pid.h"

void thermal_pid_init(thermal_pid_controller_t *pid, float kp, float ki, float kd, float min_out, float max_out) {
    if (!pid) return;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = 250.0f;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output_min = min_out;
    pid->output_max = max_out;
    pid->last_time_ms = 0;
}

void thermal_pid_set_setpoint(thermal_pid_controller_t *pid, float setpoint) {
    if (!pid) return;
    pid->setpoint = setpoint;
}

float thermal_pid_compute(thermal_pid_controller_t *pid, float current_temp, uint32_t current_time_ms) {
    if (!pid) return 0.0f;

    float dt = (current_time_ms - pid->last_time_ms) / 1000.0f;
    if (dt <= 0.0f || dt > 2.0f) dt = 0.2f; // Fallback 200ms
    pid->last_time_ms = current_time_ms;

    float error = pid->setpoint - current_temp;

    // Proportional term
    float p_term = pid->kp * error;

    // Integral term with anti-windup clamping
    pid->integral += error * dt;
    if (pid->integral > 50.0f) pid->integral = 50.0f;
    if (pid->integral < -50.0f) pid->integral = -50.0f;
    float i_term = pid->ki * pid->integral;

    // Derivative term with filtering
    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error = error;
    float d_term = pid->kd * derivative;

    // Total output
    float output = p_term + i_term + d_term;
    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;

    return output;
}

void thermal_pid_reset(thermal_pid_controller_t *pid) {
    if (!pid) return;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}
