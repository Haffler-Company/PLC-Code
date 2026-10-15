#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "project_config.h"

#ifndef CREAM_PUMP_GPIO
#define CREAM_PUMP_GPIO     GPIO_NUM_48
#endif
#ifndef AIR_SOLENOID_GPIO
#define AIR_SOLENOID_GPIO   GPIO_NUM_15
#endif
#ifndef RAINBOW_GPIO
#define RAINBOW_GPIO        GPIO_NUM_40
#endif
#ifndef WASH_PUMP_GPIO
#define WASH_PUMP_GPIO      GPIO_NUM_36
#endif

#define EDGE_PUMP_MIN_S     5
#define EDGE_WASH_MAX_S     (7 * 60)
#define WASH_DEFAULT_S      (10 * 60)
#define LEASE_FREE_DAYS     120
#define LEASE_EXTEND_DAYS   30
#define LEASE_UNLOCK_CODE   1204
#define LEASE_MASTER_CODE   12773
#define LEASE_EMERGENCY_CODE 112233

#define HOLD_TEMP_OFF_C     2
#define HOLD_TEMP_ON_C      6
#define TEMP_SENSOR_ENABLED 1
#define CYLINDER_MIN_SAFE_TEMP_C -22.0f

void extra_outputs_init(void);
void extra_modes_init(void);
void extra_modes_stop_all(void);
void extra_modes_tick_1hz(void);

bool wash_is_active(void);
bool edge_is_active(void);
bool rainbow_is_active(void);
bool air_is_active(void);

void wash_toggle(void);
void edge_toggle_produce(void);
void edge_toggle_wash(void);
void rainbow_toggle_rp(void);
void rainbow_toggle_rpw(void);
void air_toggle_manual(void);

bool hold_temp_allows_cool(void);
bool hold_cylinder_temp_allows_cool(void);
bool cylinder_freeze_protection_trip(void);
int32_t hold_temp_off_c(void);
int32_t hold_temp_on_c(void);
void hold_temp_set_off_c(int32_t v);
void hold_temp_set_on_c(int32_t v);
int32_t fallback_on_time_m(void);
int32_t fallback_off_time_m(void);
void fallback_set_on_time_m(int32_t m);
void fallback_set_off_time_m(int32_t m);
int32_t milk_mixer_on_time_s(void);
int32_t milk_mixer_off_time_m(void);
void milk_mixer_set_on_time_s(int32_t s);
void milk_mixer_set_off_time_m(int32_t m);
int32_t wash_get_countdown_s(void);
bool hold_is_fallback_mode(void);
int32_t hold_get_fallback_rem_s(bool *out_is_cooling);
int32_t rainbow_get_cycle_rem_s(bool *out_is_running);

bool lease_is_locked(void);
bool lease_try_unlock_code(int32_t code);
bool lease_try_unlock_str(const char *pin_or_code);
int32_t lease_days_left(void);
int32_t lease_get_challenge_code(void);
bool lease_verify_challenge(int32_t response);
void lease_extend_days(int32_t days);
bool lease_is_emergency_used(void);

/* سناریوی چندمرحله‌ای اقساط و چک‌های دستگاه (مشابه دستگاه برش) */
int32_t lease_stage_days_get(uint8_t stage);
void lease_stage_days_set(uint8_t stage, int32_t days);
uint8_t lease_active_stage_get(void);
int32_t lease_stage_remain_get(void);
void lease_start_chain(int32_t days_t1);
void lease_clear_all(void);
void lease_stage_code_digits(char *out, size_t out_sz, uint8_t stage);
bool lease_advance_stage(void);
bool lease_apply_grace10(void);

#ifdef __cplusplus
}
#endif
