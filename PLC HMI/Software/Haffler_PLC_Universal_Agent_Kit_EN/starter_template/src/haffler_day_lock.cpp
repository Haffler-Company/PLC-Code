#include "haffler_day_lock.h"
#include <stdio.h>
#include <string.h>

#define MAX_STAGES 5
#define ROLLBACK_THRESHOLD_SEC 300 // 5 minutes tolerance
#define EMERGENCY_GRACE_CODE "112233"
#define TECH_MASTER_PIN "1234"
#define HOURLY_COMMIT_INTERVAL_SEC 3600 // Flash endurance mitigation: hourly NVS commits (>10 yrs life)

static uint16_t s_stage_durations_days[MAX_STAGES] = {30, 30, 30, 30, 30};
static uint8_t  s_current_stage = 0;
static uint32_t s_last_valid_epoch = 0;
static uint32_t s_stage_start_epoch = 0;
static uint32_t s_last_nvs_commit_epoch = 0;
static daylock_state_t s_lock_state = DAYLOCK_STAGE_ACTIVE;
static bool s_grace_used = false;

jalali_date_t gregorian_to_jalali(int16_t g_y, int8_t g_m, int8_t g_d) {
    static const int16_t g_days_in_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    int32_t gy = g_y - 1600;
    int32_t gm = g_m - 1;
    int32_t gd = g_d - 1;

    int32_t g_day_no = 365 * gy + (gy + 3) / 4 - (gy + 99) / 100 + (gy + 399) / 400;
    g_day_no += g_days_in_month[gm] + gd;
    if (gm > 1 && ((gy % 4 == 0 && gy % 100 != 0) || (gy % 400 == 0))) {
        g_day_no++;
    }

    int32_t j_day_no = g_day_no - 79;
    int32_t j_np = j_day_no / 12053;
    j_day_no %= 12053;

    int32_t jy = 979 + 33 * j_np + 4 * (j_day_no / 1461);
    j_day_no %= 1461;

    if (j_day_no >= 366) {
        jy += (j_day_no - 1) / 365;
        j_day_no = (j_day_no - 1) % 365;
    }

    jalali_date_t j_date;
    j_date.year = jy;

    int8_t i;
    for (i = 0; i < 11 && j_day_no >= (i < 6 ? 31 : 30); ++i) {
        j_day_no -= (i < 6 ? 31 : 30);
    }
    j_date.month = i + 1;
    j_date.day   = j_day_no + 1;

    return j_date;
}

void daylock_init(const uint16_t stage_days[5]) {
    if (stage_days) {
        for (int i = 0; i < MAX_STAGES; ++i) {
            s_stage_durations_days[i] = stage_days[i];
        }
    }
    // In production, load s_current_stage, s_last_valid_epoch, s_stage_start_epoch from NVS
    s_lock_state = DAYLOCK_STAGE_ACTIVE;
}

daylock_state_t daylock_update(uint32_t current_epoch) {
    if (s_lock_state == DAYLOCK_PERMANENT_UNLOCKED) {
        return DAYLOCK_PERMANENT_UNLOCKED;
    }

    // 1. Anti-tamper monotonic guard: detect RTC backward rollback
    if (s_last_valid_epoch > 0 && current_epoch + ROLLBACK_THRESHOLD_SEC < s_last_valid_epoch) {
        s_lock_state = DAYLOCK_STAGE_TAMPER;
        return DAYLOCK_STAGE_TAMPER;
    }

    if (current_epoch > s_last_valid_epoch) {
        s_last_valid_epoch = current_epoch;
    }

    // 2. Evaluate remaining time in current stage
    uint32_t stage_duration_sec = (uint32_t)s_stage_durations_days[s_current_stage] * 86400UL;
    uint32_t elapsed_sec = (current_epoch > s_stage_start_epoch) ? (current_epoch - s_stage_start_epoch) : 0;

    if (elapsed_sec >= stage_duration_sec) {
        s_lock_state = DAYLOCK_STAGE_EXPIRED;
    } else if ((stage_duration_sec - elapsed_sec) <= 172800UL) { // <= 48 Hours
        s_lock_state = DAYLOCK_STAGE_WARNING;
    } else {
        s_lock_state = DAYLOCK_STAGE_ACTIVE;
    }

    return s_lock_state;
}

bool daylock_is_motion_locked(void) {
    return (s_lock_state == DAYLOCK_STAGE_EXPIRED || s_lock_state == DAYLOCK_STAGE_TAMPER);
}

uint8_t daylock_get_active_stage(void) {
    return s_current_stage;
}

uint16_t daylock_get_remaining_days(void) {
    uint32_t stage_duration_sec = (uint32_t)s_stage_durations_days[s_current_stage] * 86400UL;
    uint32_t elapsed_sec = (s_last_valid_epoch > s_stage_start_epoch) ? (s_last_valid_epoch - s_stage_start_epoch) : 0;
    if (elapsed_sec >= stage_duration_sec) return 0;
    return (stage_duration_sec - elapsed_sec) / 86400UL;
}

uint8_t daylock_get_remaining_hours(void) {
    uint32_t stage_duration_sec = (uint32_t)s_stage_durations_days[s_current_stage] * 86400UL;
    uint32_t elapsed_sec = (s_last_valid_epoch > s_stage_start_epoch) ? (s_last_valid_epoch - s_stage_start_epoch) : 0;
    if (elapsed_sec >= stage_duration_sec) return 0;
    return ((stage_duration_sec - elapsed_sec) % 86400UL) / 3600UL;
}

uint16_t daylock_generate_challenge(uint32_t current_epoch, uint32_t machine_serial) {
    uint32_t seed = (current_epoch / 86400UL) ^ machine_serial;
    uint16_t c = (seed % 9000) + 1000; // 4-digit code (1000..9999)
    return c;
}

bool daylock_verify_unlock(uint16_t challenge, uint16_t response_key) {
    uint8_t d[4], r_expected[4];
    d[0] = (challenge / 1000) % 10;
    d[1] = (challenge / 100) % 10;
    d[2] = (challenge / 10) % 10;
    d[3] = challenge % 10;

    for (uint8_t i = 0; i < 4; ++i) {
        r_expected[i] = ((d[i] * 3) + i + 1) % 10;
    }

    uint16_t computed_response = r_expected[0] * 1000 + r_expected[1] * 100 + r_expected[2] * 10 + r_expected[3];
    if (response_key == computed_response) {
        if (s_current_stage < MAX_STAGES - 1) {
            s_current_stage++;
            s_stage_start_epoch = s_last_valid_epoch;
            s_lock_state = DAYLOCK_STAGE_ACTIVE;
        } else {
            s_lock_state = DAYLOCK_PERMANENT_UNLOCKED;
        }
        return true;
    }
    return false;
}

bool daylock_apply_emergency_grace(const char *code) {
    if (!code || s_grace_used) return false;
    if (strcmp(code, EMERGENCY_GRACE_CODE) == 0) {
        s_grace_used = true;
        s_stage_start_epoch += 86400UL; // Add 24 Hours
        s_lock_state = DAYLOCK_STAGE_ACTIVE;
        return true;
    }
    return false;
}

bool daylock_admin_bypass(const char *pin) {
    if (!pin) return false;
    if (strcmp(pin, TECH_MASTER_PIN) == 0) {
        s_lock_state = DAYLOCK_PERMANENT_UNLOCKED;
        return true;
    }
    return false;
}
