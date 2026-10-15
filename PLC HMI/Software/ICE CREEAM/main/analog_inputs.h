#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PINMAP_TERMINAL_COUNT   8
#define PINMAP_ROLE_COUNT       8
#define PINMAP_NC               255

/* Physical terminal index on ASTIAJ PLC Board: I0 .. I7 (100% Digital Active-LOW with Internal Pull-Up) */
typedef enum {
    TERM_I0 = 0,  /* GPIO 6 دیجیتال (I0 - سنسور القایی دور پولی) */
    TERM_I1 = 1,  /* کمکی یدکی (I1) */
    TERM_I2 = 2,  /* GPIO 5 دیجیتال (I2 - میکروسوئیچ کلاچ عقب / سفتی) */
    TERM_I3 = 3,  /* GPIO 15 دیجیتال (I3 - کنترل‌بار موتور پارو) */
    TERM_I4 = 4,  /* GPIO 4 دیجیتال (I4 - میکروسوئیچ اهرم جلو / قیف) */
    TERM_I5 = 5,  /* GPIO 17 دیجیتال (I5 - کنترل‌بار کمپرسور بزرگ) */
    TERM_I6 = 6,  /* GPIO 3 دیجیتال (I6 - سنسور خالی بودن مخزن شیر) */
    TERM_I7 = 7   /* GPIO 7 دیجیتال (I7 - شستی استارت چرخه پنل) */
} pinmap_terminal_t;

/* Logical functional roles for ice cream machine */
typedef enum {
    ROLE_PULLEY_EYE      = 0, /* سنسور القایی پولی / دور موتور */
    ROLE_REAR_CLUTCH     = 1, /* میکروسوئیچ سفتی عقب */
    ROLE_FRONT_LEVER     = 2, /* میکروسوئیچ اهرم تخلیه جلو */
    ROLE_LOAD_MOTOR      = 3, /* کنترل‌بار موتور پارو */
    ROLE_LOAD_COMP       = 4, /* کنترل‌بار کمپرسور برودت */
    ROLE_SPARE_AUX       = 5, /* ورودی کمکی یدکی */
    ROLE_EMERGENCY_STOP  = 6, /* سنسور خالی بودن مخزن شیر (فلوتر شناور بن‌ماری) */
    ROLE_CYCLE_START     = 7  /* شستی استارت چرخه */
} pinmap_role_t;

#define ROLE_HOPPER_EMPTY ROLE_EMERGENCY_STOP

/* Subsystem Initialization & Periodic Sampling */
esp_err_t analog_inputs_init(void);
void analog_inputs_poll(void);
void analog_inputs_sim_pulse(uint8_t term, bool active);

/* Real-time Physical Status Query */
bool pinmap_terminal_is_active(uint8_t term_idx);
uint16_t analog_inputs_get_raw_counts(uint8_t pair_idx);

/* Logical Role Queries (Mapped through runtime pinmap) */
bool pinmap_role_is_active(pinmap_role_t role);
bool micro_front_active(void);
bool micro_rear_active(void);
bool inductive_active(void);
bool stop_button_active(void);
bool hopper_empty_active(void);
bool start_button_active(void);
bool load_motor_tripped(void);
bool load_comp_tripped(void);

/* Runtime Pin Remapping API */
uint8_t pinmap_get_source(pinmap_role_t role);
esp_err_t pinmap_set_source(pinmap_role_t role, uint8_t term_idx);
void pinmap_reset_defaults(void);
const char *pinmap_get_terminal_name(uint8_t term_idx);
const char *pinmap_get_role_name(pinmap_role_t role);
const char *pinmap_get_role_name_fa(pinmap_role_t role);
const char *pinmap_get_role_name_en(pinmap_role_t role);
const char *pinmap_get_role_name_tr(pinmap_role_t role);
int8_t pinmap_get_role_for_terminal(uint8_t term_idx);

/* تنظیم پولاریته کنترل‌بارها: true = N.O. (Active-LOW), false = N.C. (Active-HIGH) */
bool analog_inputs_get_overload_polarity(bool is_motor);
void analog_inputs_set_overload_polarity(bool is_motor, bool is_no);

#ifdef __cplusplus
}
#endif
