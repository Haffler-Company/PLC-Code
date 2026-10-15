#ifndef BAKERY_DAY_LOCK_H
#define BAKERY_DAY_LOCK_H

#include "day_lock_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Backward-compatible globals matching existing project symbols */
extern int day_lock_remain;
extern uint8_t day_lock_active_stage;
extern int day_lock_stage_days[DAY_LOCK_MAX_STAGES];
extern volatile bool device_locked;
extern bool day_lock_grace10_used;
extern uint32_t dev_lock_challenge;

/**
 * @brief Checks if device is locked in HARD_LOCK or TAMPER state.
 */
static inline bool day_lock_is_locked(void) { return device_locked; }

/**
 * @brief Returns current FSM state (0..5).
 */
DayLockState_t day_lock_get_state(void);

/**
 * @brief Checks if machine is in soft lockout (allows completing running cycle, inhibits start).
 */
bool day_lock_is_soft_locked(void);

/**
 * @brief Checks if keypad is temporarily frozen due to progressive brute-force penalty.
 * @param out_remaining_sec Optional output for remaining countdown seconds
 */
bool day_lock_is_brute_locked(uint32_t *out_remaining_sec);

/* Core Day-Lock Lifecycle APIs */
void day_lock_init(void);
void day_lock_save(void);
void day_lock_tick_1s(void);
void day_lock_arm(int days);
void day_lock_start_chain(int days);
void day_lock_start_leasing(const int16_t stages[DAY_LOCK_MAX_STAGES]);
bool day_lock_try_unlock(const char *pin);
void day_lock_issue_challenge(void);
bool day_lock_apply_grace10(void);
void day_lock_clear_after_unlock(void);
bool day_lock_advance_stage(void);
void day_lock_sync_authorized_epoch(uint32_t new_epoch);

/* UI Modal hooks (Apple HIG Pure White) */
void dev_lock_ui_show(void);
void dev_lock_ui_hide(void);
bool dev_lock_ui_is_active(void);
void dev_lock_ui_process_key(uint8_t nav_key);

#ifdef __cplusplus
}
#endif

#endif /* BAKERY_DAY_LOCK_H */
