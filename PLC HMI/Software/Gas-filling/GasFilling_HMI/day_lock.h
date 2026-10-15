#ifndef GASFILLING_DAY_LOCK_H
#define GASFILLING_DAY_LOCK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAY_LOCK_MAX_STAGES       5
#define DAY_LOCK_MAGIC            0x4841464CUL /* "HAFL" */
#define MASTER_UNLOCK_PIN         "12773"
#define MASTER_PIN_2              "123456"
#define MASTER_PIN_3              "12733"
#define EMERGENCY_GRACE_PIN       "112233"
#define DEFAULT_ADMIN_PIN         "1234"

typedef enum {
    LOCK_STAGE_FACTORY_FREE = 0,
    LOCK_STAGE_ACTIVE_LEASE  = 1,
    LOCK_STAGE_GRACE_10      = 2,
    LOCK_STAGE_SOFT_LOCK     = 3,
    LOCK_STAGE_HARD_LOCK     = 4,
    LOCK_STAGE_TAMPER        = 5
} DayLockState_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;               // 0x4841464C ("HAFL")
    uint8_t  active_stage;        // 0 = Free, 1..5 = Active Stage (1 to 5)
    uint8_t  lock_state;          // DayLockState_t: 0=FREE, 1=ACTIVE, 2=GRACE, 3=SOFT_LOCK, 4=HARD_LOCK, 5=TAMPER
    int16_t  remain_days;         // Days remaining in active stage
    int16_t  stage_days[DAY_LOCK_MAX_STAGES]; // Pre-configured days per stage
    bool     grace10_used;        // Immutable flag for one-time grace
    uint32_t last_known_epoch;    // Monotonic timestamp guard
    uint32_t sec_accumulator;     // Hourly flash-wear accumulator
    uint8_t  failed_attempts;     // Brute-force counter
    uint32_t lockout_until_ms;    // Timestamp for anti-brute freeze
    uint32_t crc32;               // Checksum of structure
} DayLockRecord_t;

extern int day_lock_remain;
extern uint8_t day_lock_active_stage;
extern int day_lock_stage_days[5];
extern volatile bool device_locked;
extern bool day_lock_grace10_used;
extern uint32_t dev_lock_challenge;

void day_lock_init(void);
void day_lock_save(void);
void day_lock_tick_1s(void);
void day_lock_arm(int days);
void day_lock_start_chain(int days);
void day_lock_set_stage_days(uint8_t stage_idx, int days);
int day_lock_get_stage_days(uint8_t stage_idx);
void day_lock_arm_chain(void);
bool day_lock_try_unlock(const char *pin);
void day_lock_issue_challenge(void);
bool day_lock_apply_grace10(void);
void day_lock_clear_after_unlock(void);
bool day_lock_advance_stage(void);
void day_lock_unlock_to_stage(uint8_t target_stage);

DayLockState_t day_lock_get_state(void);
bool day_lock_is_brute_locked(uint32_t *out_remaining_sec);
bool day_lock_is_soft_locked(void);
bool day_lock_is_tamper_locked(void);
uint8_t day_lock_get_failed_attempts(void);

/* UI Modal hooks */
void dev_lock_ui_show(void);
void dev_lock_ui_hide(void);
bool dev_lock_ui_is_active(void);

/* Physical Keypad Touchless Fallback Hook */
void day_lock_handle_panel_key(uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* GASFILLING_DAY_LOCK_H */
