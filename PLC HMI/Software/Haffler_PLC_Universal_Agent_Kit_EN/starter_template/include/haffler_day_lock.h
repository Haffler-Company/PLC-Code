#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file haffler_day_lock.h
 * @brief Haffler 5-Stage Cascading Leasing Day-Lock Engine
 * Hardware RTC DS3231 + Monotonic Anti-Tamper + Jalali Calendar + Challenge-Response
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DAYLOCK_STAGE_ACTIVE = 0,   // Machine operating normally
    DAYLOCK_STAGE_WARNING,      // Remaining <= 48 Hours, amber banner
    DAYLOCK_STAGE_EXPIRED,      // Stage expired, motion clamped
    DAYLOCK_STAGE_TAMPER,       // Monotonic rollback or RTC failure detected
    DAYLOCK_PERMANENT_UNLOCKED  // All installments paid, lock disabled
} daylock_state_t;

typedef struct {
    int16_t year;
    int8_t  month;
    int8_t  day;
} jalali_date_t;

// Initialize day-lock subsystem with stage durations in days (e.g., 30, 30, 30, 30, 30)
void daylock_init(const uint16_t stage_days[5]);

// Call inside Core 1 scan loop or periodic 1s timer to update state
daylock_state_t daylock_update(uint32_t current_epoch);

// Check if motion actuators (pneumatics, spindles, heaters) are locked
bool daylock_is_motion_locked(void);

// Get current active stage index (0 to 4)
uint8_t daylock_get_active_stage(void);

// Get remaining days and hours in active stage
uint16_t daylock_get_remaining_days(void);
uint8_t  daylock_get_remaining_hours(void);

// Gregorian to Jalali (Solar Hijri) calendar converter (zero external libs)
jalali_date_t gregorian_to_jalali(int16_t g_y, int8_t g_m, int8_t g_d);

// Generate 4-digit dynamic challenge code for the current day & stage
uint16_t daylock_generate_challenge(uint32_t current_epoch, uint32_t machine_serial);

// Verify user unlock response key (computes factory formula: r_i = ((d_i * 3) + i + 1) % 10)
bool daylock_verify_unlock(uint16_t challenge, uint16_t response_key);

// Check emergency one-time 24-hour grace unlock code (112233)
bool daylock_apply_emergency_grace(const char *code);

// Unlock with supervisor/technician PIN (1234) or master bypass
bool daylock_admin_bypass(const char *pin);

#ifdef __cplusplus
}
#endif
