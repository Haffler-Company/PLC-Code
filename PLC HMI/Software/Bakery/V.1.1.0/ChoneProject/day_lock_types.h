#ifndef BAKERY_DAY_LOCK_TYPES_H
#define BAKERY_DAY_LOCK_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAY_LOCK_MAX_STAGES       5
#define DAY_LOCK_MAGIC            0x4841464CUL /* "HAFL" */
#define DAY_LOCK_SALT             "HAFFLER_LEASING_2026_SECRET"

#define MASTER_PIN_1              "12773"
#define MASTER_PIN_2              "123456"
#define MASTER_PIN_3              "12733"
#define EMERGENCY_GRACE_PIN       "112233"
#define DEFAULT_ADMIN_PIN_1       "1234"
#define DEFAULT_ADMIN_PIN_2       "654321"

/**
 * @brief 6-Stage Industrial Leasing FSM (PackML Aligned)
 */
typedef enum {
    LOCK_STAGE_FACTORY_FREE = 0, ///< 100% unrestricted normal lifetime run
    LOCK_STAGE_ACTIVE_LEASE  = 1, ///< Active lease countdown running (T1..T5)
    LOCK_STAGE_GRACE_10      = 2, ///< 10-day emergency grace period running
    LOCK_STAGE_SOFT_LOCK     = 3, ///< Cycle finishes cleanly, no new cycles allowed
    LOCK_STAGE_HARD_LOCK     = 4, ///< Total inhibit of all actuators, lock modal active
    LOCK_STAGE_TAMPER        = 5  ///< Critical lock: RTC rollback or hardware tampering detected
} DayLockState_t;

/**
 * @brief Mirrored NVS Record with CRC32 Checksum
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;                              ///< Magic header 0x4841464C ("HAFL")
    uint8_t  active_stage;                       ///< Current FSM state / stage (0..5)
    int16_t  remain_days;                        ///< Remaining days in active stage
    int16_t  stage_days[DAY_LOCK_MAX_STAGES];    ///< Days configured for stages T1..T5
    bool     grace10_used;                       ///< Immutable one-time emergency grace flag
    uint32_t last_known_epoch;                   ///< Monotonic RTC epoch guard
    uint32_t sec_accumulator;                    ///< Seconds counter in RAM (hourly NVS flush)
    uint8_t  failed_attempts;                    ///< Anti-brute force failure count
    uint32_t lockout_until_ms;                   ///< Anti-brute freeze target timestamp (millis)
    uint32_t crc32;                              ///< CRC32 of preceding struct bytes
} DayLockRecord_t;

#ifdef __cplusplus
}
#endif

#endif /* BAKERY_DAY_LOCK_TYPES_H */
