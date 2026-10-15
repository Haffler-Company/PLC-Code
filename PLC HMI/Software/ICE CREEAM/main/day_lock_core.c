#include "day_lock_core.h"
#include "day_lock_storage.h"
#include "warranty.h"
#include "ds3231.h"
#include "buzzer.h"
#include "fault_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "day_lock_core";

static DayLockRecord_t s_lock_data;
static uint32_t s_active_challenge = 58219;
static bool s_cycle_idle = true;
static int64_t s_last_tick_us = 0;

/* ارجاع به توابع لایه رابط کاربری */
extern void day_lock_ui_show(void);
extern void day_lock_ui_hide(void);
extern bool day_lock_ui_is_visible(void);

static uint8_t calc_digit_sum(uint64_t n)
{
    uint8_t sum = 0;
    while (n > 0) {
        sum = (uint8_t)(sum + (n % 10ULL));
        n /= 10ULL;
    }
    return sum;
}

static uint32_t get_system_uptime_sec(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

/* تبدیل قطعی تاریخ میلادی به زمان یونیکس UTC بدون وابستگی به mktime یا متغیر محیطی TZ */
static uint32_t calendar_to_utc_epoch(int year, int month, int day, int hour, int minute, int second)
{
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) return 0;

    static const uint16_t days_before_month[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    int y = year;
    int leap_years = (y - 1969) / 4 - (y - 1901) / 100 + (y - 1601) / 400;
    uint32_t days = (uint32_t)((y - 1970) * 365 + leap_years + days_before_month[month - 1] + (day - 1));

    bool is_leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    if (is_leap && month > 2) {
        days++;
    }

    return (days * 86400U + (uint32_t)hour * 3600U + (uint32_t)minute * 60U + (uint32_t)second);
}

static uint32_t get_rtc_epoch(bool *out_valid)
{
    if (out_valid) *out_valid = false;

    ds3231_time_t t;
    if (ds3231_get_time(&t) == ESP_OK) {
        if (t.year >= 24 && t.month >= 1 && t.month <= 12 && t.date >= 1 && t.date <= 31) {
            uint32_t ep = calendar_to_utc_epoch(2000 + (int)t.year, (int)t.month, (int)t.date,
                                               (int)t.hour, (int)t.minute, (int)t.second);
            if (ep > 1700000000U) {
                if (out_valid) *out_valid = true;
                return ep;
            }
        }
    }

    time_t now = time(NULL);
    if (now > 1700000000) {
        if (out_valid) *out_valid = true;
        return (uint32_t)now;
    }
    return 0;
}

void day_lock_generate_challenge(void)
{
    uint8_t mac[6] = {0};
    (void)esp_efuse_mac_get_default(mac);
    uint32_t r = esp_random();
    uint32_t seed = r ^ ((uint32_t)mac[4] << 8 | (uint32_t)mac[5]);
    s_active_challenge = 1000U + (seed % 9000U);
    ESP_LOGI(TAG, "New 4-digit challenge generated: %lu", (unsigned long)s_active_challenge);

    nvs_handle_t h;
    if (nvs_open("haf_lock", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u32(h, "dl_chal", s_active_challenge);
        nvs_commit(h);
        nvs_close(h);
    }
}

uint32_t day_lock_get_active_challenge(void)
{
    return s_active_challenge;
}

void day_lock_calc_challenge_response_4digit(char *out_buf, size_t buf_sz, uint32_t challenge)
{
    if (!out_buf || buf_sz < 5) return;
    char s_chal[16];
    snprintf(s_chal, sizeof(s_chal), "%04lu", (unsigned long)(challenge % 10000U));
    for (int i = 0; i < 4; i++) {
        int d = s_chal[i] - '0';
        int r = ((d * 3) + (i + 1)) % 10;
        out_buf[i] = (char)('0' + r);
    }
    out_buf[4] = '\0';
}

void day_lock_calc_challenge_response(char *out_buf, size_t buf_sz, uint32_t challenge)
{
    if (!out_buf || buf_sz < 8) return;
    uint64_t base = (uint64_t)challenge * 2ULL;
    uint8_t cs = calc_digit_sum(base) % 10U;
    snprintf(out_buf, buf_sz, "%llu%u", (unsigned long long)base, (unsigned)cs);
}

void day_lock_get_future_date_str(int16_t add_days, char *out_str, size_t out_sz, bool is_jalali)
{
    if (!out_str || out_sz < 11) return;
    if (add_days < 0) {
        snprintf(out_str, out_sz, "---");
        return;
    }

    ds3231_time_t dt;
    int cur_year = 2026, cur_month = 9, cur_day = 30;

    if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24 && dt.month >= 1 && dt.month <= 12 && dt.date >= 1 && dt.date <= 31) {
        cur_year = 2000 + dt.year;
        cur_month = dt.month;
        cur_day = dt.date;
    } else {
        time_t now = time(NULL);
        if (now > 1700000000) {
            struct tm now_tm;
            gmtime_r(&now, &now_tm);
            cur_year = now_tm.tm_year + 1900;
            cur_month = now_tm.tm_mon + 1;
            cur_day = now_tm.tm_mday;
        }
    }

    uint32_t base_epoch = calendar_to_utc_epoch(cur_year, cur_month, cur_day, 12, 0, 0);
    if (base_epoch == 0) base_epoch = 1789053000;

    time_t future_epoch = (time_t)base_epoch + ((time_t)add_days * 86400LL);
    struct tm future_tm;
    gmtime_r(&future_epoch, &future_tm);

    int gy = future_tm.tm_year + 1900;
    int gm = future_tm.tm_mon + 1;
    int gd = future_tm.tm_mday;

    if (is_jalali) {
        int jy = 0, jm = 0, jd = 0;
        gregorian_to_jalali(gy, gm, gd, &jy, &jm, &jd);
        /* جهت جلوگیری از معکوس شدن اجزای تاریخ در موتور BiDi و نمایش استاندارد سال/ماه/روز */
        snprintf(out_str, out_sz, "%02d/%02d/%04d", jd, jm, jy);
    } else {
        snprintf(out_str, out_sz, "%04d/%02d/%02d", gy, gm, gd);
    }
}

void day_lock_get_future_jalali_str(int16_t add_days, char *out_str, size_t out_sz)
{
    day_lock_get_future_date_str(add_days, out_str, out_sz, true);
}

void day_lock_calc_stage_code(char *out_buf, size_t buf_sz, uint8_t stage, const char *admin_pin)
{
    if (!out_buf || buf_sz == 0) return;
    if (!admin_pin || !admin_pin[0]) admin_pin = DEFAULT_ADMIN_PIN;

    const uint8_t mult = (uint8_t)(stage + 1U);
    size_t n = 0;
    uint16_t dsum = 0;
    for (const char *s = admin_pin; *s && (n + 2 < buf_sz); ++s) {
        if (*s < '0' || *s > '9') continue;
        const uint8_t d = (uint8_t)(((*s - '0') * mult) % 10U);
        out_buf[n++] = (char)('0' + d);
        dsum = (uint16_t)(dsum + d);
    }
    if (n == 0) { out_buf[0] = '\0'; return; }
    out_buf[n] = (char)('0' + (dsum % 10U));
    out_buf[n + 1] = '\0';
}

DayLockState_t day_lock_get_state(void)
{
    return (DayLockState_t)s_lock_data.lock_state;
}

int16_t day_lock_get_remain_days(void)
{
    return s_lock_data.remain_days;
}

DayLockTimeUnit_t day_lock_get_remain_time_unit(int32_t *out_val)
{
    int32_t dummy = 0;
    int32_t *v = out_val ? out_val : &dummy;

    if (s_lock_data.remain_days <= 0) {
        *v = 0;
        return DAY_LOCK_UNIT_DAYS;
    }

    if (s_lock_data.remain_days > 1) {
        *v = (int32_t)s_lock_data.remain_days;
        return DAY_LOCK_UNIT_DAYS;
    }

    /* remain_days == 1: آخرین روز قسط (کمتر از ۲۴ ساعت تا ساعت ۲۴:۰۰ / ۱۲ شب) */
    uint32_t rem_sec = 0;
    ds3231_time_t dt;
    if (ds3231_get_time(&dt) == ESP_OK && dt.hour <= 23 && dt.minute <= 59 && dt.second <= 59) {
        /* محاسبه دقیق زمان باقیمانده تا پایان روز جاری (ساعت ۱۲ شب / ۲۴:۰۰) بر مبنای ساعت سخت‌افزاری */
        uint32_t elapsed_today = (uint32_t)dt.hour * 3600UL + (uint32_t)dt.minute * 60UL + (uint32_t)dt.second;
        rem_sec = (elapsed_today < 86400UL) ? (86400UL - elapsed_today) : 0;
    } else {
        rem_sec = (s_lock_data.sec_accumulator < 86400UL) ? (86400UL - s_lock_data.sec_accumulator) : 0;
    }

    if (rem_sec >= 3600UL) {
        /* گرد کردن به نزدیک‌ترین ساعت کامل تا ۱۲ شب (مثلاً ۱ ساعت و ۴۷ دقیقه -> ۲ ساعت) */
        int32_t hrs = (int32_t)((rem_sec + 1800UL) / 3600UL);
        if (hrs > 24) hrs = 24;
        if (hrs <= 0) hrs = 1;
        *v = hrs;
        return DAY_LOCK_UNIT_HOURS;
    } else {
        int32_t mins = (int32_t)(rem_sec / 60UL);
        if (mins <= 0) mins = 1;
        if (mins > 59) mins = 59;
        *v = mins;
        return DAY_LOCK_UNIT_MINUTES;
    }
}

uint8_t day_lock_get_active_stage(void)
{
    return s_lock_data.active_stage;
}

uint8_t day_lock_get_total_stages(void)
{
    uint8_t total = 0;
    for (int i = DAY_LOCK_MAX_STAGES - 1; i >= 0; i--) {
        if (s_lock_data.stage_days[i] > 0 || day_lock_is_stage_settled((uint8_t)(i + 1))) {
            total = (uint8_t)(i + 1);
            break;
        }
    }
    if (s_lock_data.active_stage > total) {
        total = s_lock_data.active_stage;
    }
    if (total == 0) {
        total = (s_lock_data.active_stage > 0) ? s_lock_data.active_stage : 1;
    }
    if (total > DAY_LOCK_MAX_STAGES) {
        total = DAY_LOCK_MAX_STAGES;
    }
    return total;
}

int16_t day_lock_get_stage_days(uint8_t stage)
{
    if (stage < 1 || stage > DAY_LOCK_MAX_STAGES) return 0;
    return s_lock_data.stage_days[stage - 1];
}

void day_lock_set_stage_days(uint8_t stage, int16_t days)
{
    if (stage < 1 || stage > DAY_LOCK_MAX_STAGES) return;
    if (days < 0) days = 0;
    if (days > 365) days = 365;
    s_lock_data.stage_days[stage - 1] = days;
    day_lock_storage_save(&s_lock_data);
}

bool day_lock_is_grace_used(void)
{
    return s_lock_data.grace10_used;
}

bool day_lock_is_locked(void)
{
    return (s_lock_data.lock_state == LOCK_STAGE_HARD_LOCK ||
            s_lock_data.lock_state == LOCK_STAGE_TAMPER);
}

bool day_lock_is_soft_locked(void)
{
    return (s_lock_data.lock_state == LOCK_STAGE_SOFT_LOCK);
}

bool day_lock_is_brute_locked(uint32_t *out_remaining_sec)
{
    uint32_t now_sec = get_system_uptime_sec();
    if (s_lock_data.lockout_until_sec > 0) {
        if (now_sec < s_lock_data.lockout_until_sec) {
            if (out_remaining_sec) {
                *out_remaining_sec = s_lock_data.lockout_until_sec - now_sec;
            }
            return true;
        } else {
            /* زمان مسدودی ۱۰ دقیقه‌ای به پایان رسید -> بازنشانی فریز و صفر کردن شمارنده */
            s_lock_data.lockout_until_sec = 0;
            s_lock_data.failed_attempts = 0;
            day_lock_storage_save(&s_lock_data);
        }
    }
    if (out_remaining_sec) *out_remaining_sec = 0;
    return false;
}

uint8_t day_lock_get_failed_attempts(void)
{
    return s_lock_data.failed_attempts;
}

uint8_t day_lock_get_remaining_attempts(void)
{
    if (s_lock_data.failed_attempts >= 3) return 0;
    return (uint8_t)(3 - s_lock_data.failed_attempts);
}

void day_lock_reset_failed_attempts(void)
{
    s_lock_data.failed_attempts = 0;
    s_lock_data.lockout_until_sec = 0;
    day_lock_storage_save(&s_lock_data);
}

void day_lock_clear_after_unlock(void)
{
    s_lock_data.lock_state = LOCK_STAGE_FACTORY_FREE;
    s_lock_data.active_stage = 0;
    s_lock_data.remain_days = 0;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) {
        s_lock_data.stage_days[i] = 0;
    }
    s_lock_data.failed_attempts = 0;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.lockout_until_sec = 0;
    s_lock_data.grace10_used = false;
    for (int i = 0; i < EMERGENCY_GRACE_COUNT; i++) {
        s_lock_data.grace_used[i] = false;
    }
    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
    ESP_LOGI(TAG, "Factory Master Override accepted -> System set to FACTORY_FREE");
}

void day_lock_arm_chain(int16_t days_t1)
{
    if (days_t1 <= 0) {
        day_lock_clear_after_unlock();
        return;
    }
    s_lock_data.active_stage = 1;
    s_lock_data.remain_days = days_t1;
    s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
    s_lock_data.stage_days[0] = days_t1;
    day_lock_clear_stage_settled(1);
    ds3231_time_t dt;
    uint32_t today_elapsed_sec = 0;
    if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24) {
        today_elapsed_sec = (uint32_t)dt.hour * 3600U + (uint32_t)dt.minute * 60U + (uint32_t)dt.second;
        if (today_elapsed_sec >= 86400U) today_elapsed_sec = 0;
    }
    s_lock_data.sec_accumulator = today_elapsed_sec;
    s_lock_data.failed_attempts = 0;
    s_lock_data.lockout_until_sec = 0;

    bool valid = false;
    uint32_t ep = get_rtc_epoch(&valid);
    if (valid) s_lock_data.last_known_epoch = ep;

    day_lock_generate_challenge();
    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
    ESP_LOGI(TAG, "Installment chain armed with Stage 1 = %d days (pre-acc=%lu s for midnight lock)",
             (int)days_t1, (unsigned long)today_elapsed_sec);
}

void day_lock_arm_5stages(const int16_t days[DAY_LOCK_MAX_STAGES])
{
    if (!days) return;
    uint8_t first_active_stg = 0;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) {
        s_lock_data.stage_days[i] = (days[i] >= 0 && days[i] <= 365) ? days[i] : 0;
        if (s_lock_data.stage_days[i] > 0) {
            if (!day_lock_is_stage_settled((uint8_t)(i + 1))) {
                if (first_active_stg == 0) {
                    first_active_stg = (uint8_t)(i + 1);
                }
            }
        }
    }
    if (first_active_stg > 0) {
        if (s_lock_data.active_stage != first_active_stg || s_lock_data.remain_days <= 0 || s_lock_data.remain_days > s_lock_data.stage_days[first_active_stg - 1]) {
            s_lock_data.remain_days = s_lock_data.stage_days[first_active_stg - 1];
        }
        s_lock_data.active_stage = first_active_stg;
        s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
    } else {
        s_lock_data.active_stage = 0;
        s_lock_data.remain_days = 0;
        s_lock_data.lock_state = LOCK_STAGE_FACTORY_FREE;
    }
    ds3231_time_t dt5;
    uint32_t today_elapsed_sec5 = 0;
    if (ds3231_get_time(&dt5) == ESP_OK && dt5.year >= 24) {
        today_elapsed_sec5 = (uint32_t)dt5.hour * 3600U + (uint32_t)dt5.minute * 60U + (uint32_t)dt5.second;
        if (today_elapsed_sec5 >= 86400U) today_elapsed_sec5 = 0;
    }
    s_lock_data.sec_accumulator = today_elapsed_sec5;
    s_lock_data.failed_attempts = 0;
    s_lock_data.lockout_until_sec = 0;

    bool valid = false;
    uint32_t ep = get_rtc_epoch(&valid);
    if (valid) s_lock_data.last_known_epoch = ep;

    day_lock_generate_challenge();
    day_lock_storage_save(&s_lock_data);
    ESP_LOGI(TAG, "All 5 installment stages configured. Active Stage %u = %d days (pre-acc=%lu s)",
             (unsigned)s_lock_data.active_stage, (int)s_lock_data.remain_days, (unsigned long)today_elapsed_sec5);
}

bool day_lock_advance_stage(void)
{
    for (uint8_t next_stage = s_lock_data.active_stage + 1; next_stage <= DAY_LOCK_MAX_STAGES; ++next_stage) {
        int16_t days = s_lock_data.stage_days[next_stage - 1];
        if (days <= 0) continue;

        s_lock_data.active_stage = next_stage;
        s_lock_data.remain_days = days;
        s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
        s_lock_data.sec_accumulator = 0;
        s_lock_data.failed_attempts = 0;
        s_lock_data.lockout_until_sec = 0;
        day_lock_generate_challenge();
        day_lock_storage_save(&s_lock_data);
        day_lock_ui_hide();
        ESP_LOGI(TAG, "Installment advanced to Stage %u (%d days)", (unsigned)next_stage, (int)days);
        return true;
    }

    /* کلیه مراحل اقساط با موفقیت پرداخت و به پایان رسید -> آزادسازی کامل دائمی کارخانه */
    day_lock_clear_after_unlock();
    ESP_LOGI(TAG, "All installment stages completed -> Permanent FACTORY_FREE unlock");
    return false;
}

bool day_lock_apply_grace10(void)
{
    if (s_lock_data.grace10_used) {
        ESP_LOGW(TAG, "Emergency grace 10d (legacy 112233) has already been used!");
        return false;
    }

    s_lock_data.grace10_used = true;
    if (s_lock_data.remain_days < 0) s_lock_data.remain_days = 0;
    s_lock_data.remain_days += 10;
    s_lock_data.lock_state = LOCK_STAGE_GRACE_10;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    s_lock_data.lockout_until_sec = 0;
    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
    ESP_LOGI(TAG, "One-Time Emergency Grace 10 Days (legacy) activated!");
    return true;
}

bool day_lock_apply_grace10_indexed(uint8_t idx)
{
    if (idx >= EMERGENCY_GRACE_COUNT) {
        ESP_LOGW(TAG, "grace10_indexed: invalid index %u", (unsigned)idx);
        return false;
    }
    if (s_lock_data.grace_used[idx]) {
        ESP_LOGW(TAG, "Emergency grace code \"%s\" has already been used!", EMERGENCY_GRACE_PINS[idx]);
        return false;
    }

    s_lock_data.grace_used[idx] = true;
    if (s_lock_data.remain_days < 0) {
        s_lock_data.remain_days = 0;
    }
    s_lock_data.remain_days += 10;
    s_lock_data.lock_state = LOCK_STAGE_GRACE_10;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    s_lock_data.lockout_until_sec = 0;

    bool valid = false;
    uint32_t ep = get_rtc_epoch(&valid);
    if (valid && ep > s_lock_data.last_known_epoch) {
        s_lock_data.last_known_epoch = ep;
    }

    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
    ESP_LOGI(TAG, "One-Time Emergency Grace 10 Days [slot %u / code \"%s\"] activated! remain=%d",
             (unsigned)idx, EMERGENCY_GRACE_PINS[idx], (int)s_lock_data.remain_days);
    return true;
}

bool day_lock_is_grace_indexed_used(uint8_t idx)
{
    if (idx >= EMERGENCY_GRACE_COUNT) return true;
    return s_lock_data.grace_used[idx];
}

void day_lock_extend_days(int16_t days)
{
    if (days <= 0) return;
    s_lock_data.remain_days += days;
    s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    s_lock_data.lockout_until_sec = 0;
    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
    ESP_LOGI(TAG, "Lease extended +%d days (now %d days)", (int)days, (int)s_lock_data.remain_days);
}

bool day_lock_validate_pin(const char *pin)
{
    if (!pin || !pin[0]) return false;

    char dig[32] = {0};
    size_t n = 0;
    for (const char *s = pin; *s && (n + 1 < sizeof(dig)); ++s) {
        if (*s >= '0' && *s <= '9') dig[n++] = *s;
    }
    dig[n] = '\0';
    if (dig[0] == '\0') return false;

    uint32_t rem_sec = 0;
    if (day_lock_is_brute_locked(&rem_sec)) {
        ESP_LOGW(TAG, "Keypad input rejected: brute-force frozen (%lu s left)", (unsigned long)rem_sec);
        return false;
    }

    /* ۱. کدهای مستر محرمانه سازنده: غیرفعال‌سازی همه تایمرها و آزادسازی دستگاه */
    if (strcmp(dig, MASTER_PIN_1) == 0 ||
        strcmp(dig, MASTER_PIN_3) == 0 ||
        strcmp(dig, MASTER_PIN_ALT) == 0) {
        day_lock_clear_after_unlock();
        buzzer_beep_confirm();
        return true;
    }

    /* ۲. رمزهای اضطراری ۱۰ روزه یکبار مصرف ("11", "22", "33") */
    for (uint8_t gi = 0; gi < EMERGENCY_GRACE_COUNT; gi++) {
        if (strcmp(dig, EMERGENCY_GRACE_PINS[gi]) == 0) {
            if (day_lock_apply_grace10_indexed(gi)) {
                buzzer_beep_confirm();
                warranty_send_installment_unlock_sms(0, dig, 10);
                return true;
            }
            /* رمز قبلاً مصرف شده */
            buzzer_beep_error();
            return false;
        }
    }

    /* ۲ب. رمز قدیمی 112233 (backward compat) */
    if (strcmp(dig, EMERGENCY_GRACE_PIN_LEGACY) == 0) {
        if (day_lock_apply_grace10()) {
            buzzer_beep_confirm();
            warranty_send_installment_unlock_sms(0, dig, 10);
            return true;
        }
        buzzer_beep_error();
        return false;
    }

    /* ۳. پاسخ چالش ۴ رقمی مدیر برای بازگشایی و پیشروی مرحله اقساط */
    char exp_chal4[8] = {0};
    day_lock_calc_challenge_response_4digit(exp_chal4, sizeof(exp_chal4), s_active_challenge);
    if (strcmp(dig, exp_chal4) == 0) {
        uint8_t prev_stg = s_lock_data.active_stage;
        int16_t d = 30;
        if (s_lock_data.active_stage >= 1 && s_lock_data.active_stage <= DAY_LOCK_MAX_STAGES) {
            d = s_lock_data.stage_days[s_lock_data.active_stage - 1];
            day_lock_record_stage_settled(prev_stg);
            day_lock_advance_stage();
        } else {
            day_lock_extend_days(30);
        }
        s_lock_data.failed_attempts = 0;
        s_lock_data.lockout_until_sec = 0;
        day_lock_generate_challenge();
        buzzer_beep_confirm();
        warranty_send_installment_unlock_sms(prev_stg, dig, d);
        return true;
    }

    /* ۴. احراز هویت لایه ۳ (کدهای مراحل اقساط T1..T5) */
    for (uint8_t stg = 1; stg <= DAY_LOCK_MAX_STAGES; stg++) {
        char exp_stage[24] = {0};
        day_lock_calc_stage_code(exp_stage, sizeof(exp_stage), stg, DEFAULT_ADMIN_PIN);
        if (strcmp(dig, exp_stage) == 0) {
            /* در صورتی که کد مرحله بعدی یا مرحله جلوتر وارد شده باشد */
            if (stg == s_lock_data.active_stage + 1 || (day_lock_is_locked() && stg > s_lock_data.active_stage)) {
                for (uint8_t ps = 1; ps < stg; ps++) {
                    day_lock_record_stage_settled(ps);
                }
                s_lock_data.active_stage = stg;
                s_lock_data.remain_days = s_lock_data.stage_days[stg - 1];
                if (s_lock_data.remain_days <= 0 && s_lock_data.stage_days[stg - 1] <= 0) s_lock_data.remain_days = 30;
                s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
                s_lock_data.sec_accumulator = 0;
                s_lock_data.failed_attempts = 0;
                s_lock_data.lockout_until_sec = 0;
                day_lock_generate_challenge();
                day_lock_storage_save(&s_lock_data);
                day_lock_ui_hide();
                buzzer_beep_confirm();
                warranty_send_installment_unlock_sms(stg, dig, s_lock_data.remain_days);
                ESP_LOGI(TAG, "Stage %u code verified: advanced to Stage %u (%d days)", (unsigned)stg, (unsigned)stg, (int)s_lock_data.remain_days);
                return true;
            } else if (stg == s_lock_data.active_stage) {
                /* کد مرحله فعلی مجدداً وارد شده: بازنشانی روزها و رفع قفل */
                s_lock_data.remain_days = s_lock_data.stage_days[stg - 1];
                if (s_lock_data.remain_days <= 0 && s_lock_data.stage_days[stg - 1] <= 0) s_lock_data.remain_days = 30;
                s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
                s_lock_data.sec_accumulator = 0;
                s_lock_data.failed_attempts = 0;
                s_lock_data.lockout_until_sec = 0;
                day_lock_generate_challenge();
                day_lock_storage_save(&s_lock_data);
                day_lock_ui_hide();
                buzzer_beep_confirm();
                warranty_send_installment_unlock_sms(stg, dig, s_lock_data.remain_days);
                ESP_LOGI(TAG, "Current stage code verified: refreshed Stage %u (%d days)", (unsigned)stg, (int)s_lock_data.remain_days);
                return true;
            }
        }
    }

    /* ۴ب. کد تسویه نهایی اقساط (مرحله ۶ = آزادسازی دائم کارخانه) */
    char exp_final[24] = {0};
    day_lock_calc_stage_code(exp_final, sizeof(exp_final), DAY_LOCK_MAX_STAGES + 1, DEFAULT_ADMIN_PIN);
    if (strcmp(dig, exp_final) == 0) {
        for (uint8_t ps = 1; ps <= DAY_LOCK_MAX_STAGES; ps++) {
            day_lock_record_stage_settled(ps);
        }
        day_lock_clear_after_unlock();
        buzzer_beep_confirm();
        warranty_send_installment_unlock_sms(DAY_LOCK_MAX_STAGES + 1, dig, 0);
        ESP_LOGI(TAG, "Final installment settlement code verified -> Permanent FACTORY_FREE unlock");
        return true;
    }

    /* ۵. چالش قدیمی ۵ رقمی (پشتیبانی تکمیلی) */
    char exp_chal[32] = {0};
    day_lock_calc_challenge_response(exp_chal, sizeof(exp_chal), s_active_challenge);
    if (strcmp(dig, exp_chal) == 0) {
        day_lock_extend_days(30);
        s_lock_data.failed_attempts = 0;
        s_lock_data.lockout_until_sec = 0;
        day_lock_generate_challenge();
        buzzer_beep_confirm();
        return true;
    }

    /* کد اشتباه: اعمال جریمه ضد بروت‌فورس */
    s_lock_data.failed_attempts++;
    uint32_t now_sec = get_system_uptime_sec();

    if (s_lock_data.failed_attempts >= 10) {
        s_lock_data.lock_state = LOCK_STAGE_TAMPER;
        fault_log_record(FAULT_LEASE_EXPIRED, -2);
        ESP_LOGE(TAG, "Brute-force limit exceeded (10 errors) -> TAMPER LOCKDOWN!");
    } else if (s_lock_data.failed_attempts >= 3) {
        s_lock_data.lockout_until_sec = now_sec + 600U; /* ۱۰ دقیقه فریز کامل کی‌پد */
        ESP_LOGW(TAG, "3 failed attempts reached -> 10-minute (600s) keypad lockout activated");
    } else {
        ESP_LOGW(TAG, "Invalid PIN entered: %u/3 attempts used (%u remaining)",
                 (unsigned)s_lock_data.failed_attempts, (unsigned)(3 - s_lock_data.failed_attempts));
    }

    day_lock_generate_challenge();
    day_lock_storage_save(&s_lock_data);
    buzzer_beep_error();
    return false;
}

void day_lock_notify_cycle_idle(bool is_idle)
{
    s_cycle_idle = is_idle;
    if (s_cycle_idle && s_lock_data.lock_state == LOCK_STAGE_SOFT_LOCK) {
        s_lock_data.lock_state = LOCK_STAGE_HARD_LOCK;
        day_lock_storage_save(&s_lock_data);
        day_lock_ui_show();
        ESP_LOGI(TAG, "Machine returned to IDLE -> SOFT_LOCK promoted to HARD_LOCK");
    }
}

void day_lock_check_boot_acoustic_alert(void)
{
    if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
        /* آژیر ممتد در خطای دستکاری */
        buzzer_beep_alarm();
    } else if (s_lock_data.lock_state == LOCK_STAGE_GRACE_10) {
        /* هشدار صوتی ۳ ثانیه‌ای در حالت مهلت اضطراری */
        buzz(3000);
    } else if (s_lock_data.lock_state == LOCK_STAGE_ACTIVE_LEASE && s_lock_data.remain_days <= 3) {
        /* دابل چیرپ هشدار پایان اعتبار */
        buzz(150);
    }
}

void day_lock_init(void)
{
    if (!day_lock_storage_load(&s_lock_data)) {
        memset(&s_lock_data, 0, sizeof(DayLockRecord_t));
        s_lock_data.magic = DAY_LOCK_MAGIC;
        s_lock_data.lock_state = LOCK_STAGE_FACTORY_FREE;
        s_lock_data.active_stage = 0;
        s_lock_data.remain_days = 0;
        day_lock_storage_save(&s_lock_data);
    }

    s_lock_data.boot_counter++;

    /* پایش یکپارچگی زمان و گارد مونوتونیک */
    bool rtc_valid = false;
    uint32_t current_epoch = get_rtc_epoch(&rtc_valid);

    bool is_lease_active = (s_lock_data.lock_state == LOCK_STAGE_ACTIVE_LEASE ||
                            s_lock_data.lock_state == LOCK_STAGE_GRACE_10);

    if (rtc_valid && is_lease_active && s_lock_data.last_known_epoch > 1700000000ULL) {
        /* آستانه صنعتی ۱۲ ساعت (۴۳۲۰۰ ثانیه) جهت مهار قطعی نوسان منطقه زمانی و کریستال */
        if (current_epoch + 43200U < s_lock_data.last_known_epoch) {
            s_lock_data.lock_state = LOCK_STAGE_TAMPER;
            fault_log_record(FAULT_LEASE_EXPIRED, -1);
            ESP_LOGE(TAG, "RTC ROLLBACK DETECTED! rtc=%lu last=%lu",
                     (unsigned long)current_epoch, (unsigned long)s_lock_data.last_known_epoch);
        } else if (current_epoch > s_lock_data.last_known_epoch) {
            /* محاسبه و کسر زمان واقعی سپری‌شده در زمان خاموشی دستگاه */
            uint32_t elapsed_offline_sec = current_epoch - s_lock_data.last_known_epoch;
            s_lock_data.last_known_epoch = current_epoch;
            s_lock_data.sec_accumulator += elapsed_offline_sec;
            while (s_lock_data.sec_accumulator >= 86400UL && s_lock_data.remain_days > 0) {
                s_lock_data.sec_accumulator -= 86400UL;
                s_lock_data.remain_days--;
            }
            if (s_lock_data.remain_days <= 0) {
                s_lock_data.lock_state = LOCK_STAGE_SOFT_LOCK;
                day_lock_generate_challenge();
                fault_log_record(FAULT_LEASE_EXPIRED, 0);
                ESP_LOGW(TAG, "Offline time elapsed (%lu s) -> Lease expired, entering SOFT_LOCK",
                         (unsigned long)elapsed_offline_sec);
            }
        }
    } else if (rtc_valid && !is_lease_active) {
        /* در مد آزاد کارخانه یا عدم وجود اقساط فعال، هرگز خطای تمپر صادر نمی‌شود */
        if (current_epoch > s_lock_data.last_known_epoch) {
            s_lock_data.last_known_epoch = current_epoch;
        }
        /* در صورتی که سیستم قبلاً به اشتباه در مد تمپر قفل شده بود، پاکسازی آنی */
        if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
            s_lock_data.lock_state = (s_lock_data.active_stage > 0) ? LOCK_STAGE_HARD_LOCK : LOCK_STAGE_FACTORY_FREE;
            s_lock_data.failed_attempts = 0;
            s_lock_data.lockout_until_sec = 0;
            ESP_LOGI(TAG, "Auto-cleared false tamper lock state");
        }
    }

    /* بررسی وضعیت قفل بروت‌فورس پس از ریبوت سخت‌افزاری */
    if (s_lock_data.lockout_until_sec > 0) {
        /* با توجه به ریست شدن آپ‌تایم سیستم در بوت، حداکثر ۱۰ دقیقه (۶۰۰ ثانیه) فریز اعمال می‌شود */
        s_lock_data.lockout_until_sec = 600U;
    }

    /* بازیابی کد چالش پایدار از حافظه NVS جهت جلوگیری از تغییر کد پس از ریبوت */
    nvs_handle_t h_chal;
    uint32_t saved_chal = 0;
    if (nvs_open("haf_lock", NVS_READONLY, &h_chal) == ESP_OK) {
        if (nvs_get_u32(h_chal, "dl_chal", &saved_chal) == ESP_OK && saved_chal >= 1000 && saved_chal <= 9999) {
            s_active_challenge = saved_chal;
        }
        nvs_close(h_chal);
    }
    if (s_active_challenge == 0 || s_active_challenge < 1000 || s_active_challenge > 9999) {
        day_lock_generate_challenge();
    }
    day_lock_storage_save(&s_lock_data);
    s_last_tick_us = esp_timer_get_time();

    day_lock_check_boot_acoustic_alert();

    /* اگر مرحله فعال کنونی تسویه شده باشد، هدایت آنی به اولین مرحله تسویه نشده */
    if (s_lock_data.active_stage > 0 && day_lock_is_stage_settled(s_lock_data.active_stage)) {
        uint8_t next_s = 0;
        for (uint8_t s = s_lock_data.active_stage + 1; s <= DAY_LOCK_MAX_STAGES; s++) {
            if (!day_lock_is_stage_settled(s) && s_lock_data.stage_days[s - 1] > 0) {
                next_s = s;
                break;
            }
        }
        if (next_s > 0) {
            s_lock_data.active_stage = next_s;
            s_lock_data.remain_days = s_lock_data.stage_days[next_s - 1];
            s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
            s_lock_data.sec_accumulator = 0;
            day_lock_storage_save(&s_lock_data);
        }
    }

    ESP_LOGI(TAG, "ASTIAJ Day-Lock Engine initialized (State=%d, Stage=%u, Remain=%d days, Boot=%lu)",
             s_lock_data.lock_state, (unsigned)s_lock_data.active_stage, (int)s_lock_data.remain_days, (unsigned long)s_lock_data.boot_counter);
}

bool day_lock_check_rtc_recovery(void)
{
    if (s_lock_data.lock_state != LOCK_STAGE_TAMPER) {
        return false;
    }

    bool rtc_valid = false;
    uint32_t current_epoch = get_rtc_epoch(&rtc_valid);

    if (!rtc_valid) {
        return false;
    }

    /* اگر ساعت با اینترنت یا تنظیم دستی به زمان معتبر بازگشته است */
    if (current_epoch + 43200U >= s_lock_data.last_known_epoch) {
        if (current_epoch > s_lock_data.last_known_epoch) {
            s_lock_data.last_known_epoch = current_epoch;
        }
        if (s_lock_data.remain_days > 0) {
            s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
        } else {
            s_lock_data.lock_state = (s_lock_data.active_stage > 0) ? LOCK_STAGE_HARD_LOCK : LOCK_STAGE_FACTORY_FREE;
        }
        s_lock_data.failed_attempts = 0;
        s_lock_data.lockout_until_sec = 0;
        day_lock_storage_save(&s_lock_data);
        day_lock_ui_hide();
        ESP_LOGI(TAG, "RTC Tamper recovered successfully! (epoch=%lu, state=%d, stage=%u)",
                 (unsigned long)current_epoch, (int)s_lock_data.lock_state, (unsigned)s_lock_data.active_stage);
        return true;
    }

    return false;
}

void day_lock_tick_1000ms(void)
{
    int64_t now_us = esp_timer_get_time();
    if (now_us - s_last_tick_us < 1000000LL) return;
    s_last_tick_us = now_us;

    if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
        /* تلاش برای بازیابی خودکار در صورت رفع انحراف ساعت */
        if (day_lock_check_rtc_recovery()) {
            return;
        }
    }

    if ((s_lock_data.lock_state == LOCK_STAGE_ACTIVE_LEASE ||
         s_lock_data.lock_state == LOCK_STAGE_GRACE_10) &&
        s_lock_data.remain_days > 0)
    {
        bool rtc_valid = false;
        uint32_t current_epoch = get_rtc_epoch(&rtc_valid);
        if (rtc_valid && s_lock_data.last_known_epoch > 1700000000ULL && current_epoch > s_lock_data.last_known_epoch) {
            uint32_t delta = current_epoch - s_lock_data.last_known_epoch;
            s_lock_data.last_known_epoch = current_epoch;
            s_lock_data.sec_accumulator += delta;
        } else {
            s_lock_data.sec_accumulator++;
            if (rtc_valid && current_epoch > 1700000000ULL) {
                s_lock_data.last_known_epoch = current_epoch;
            }
        }

        /* کسر یک روز هر ۲۴ ساعت (۸۶۴۰۰ ثانیه کارکرد یا گذر زمان واقعی) */
        while (s_lock_data.sec_accumulator >= 86400UL && s_lock_data.remain_days > 0) {
            s_lock_data.sec_accumulator -= 86400UL;
            s_lock_data.remain_days--;

            if (s_lock_data.remain_days <= 0) {
                s_lock_data.lock_state = LOCK_STAGE_SOFT_LOCK;
                day_lock_generate_challenge();
                fault_log_record(FAULT_LEASE_EXPIRED, (int8_t)s_lock_data.active_stage);
                ESP_LOGW(TAG, "Stage %u lease days reached zero -> Entering SOFT_LOCK", (unsigned)s_lock_data.active_stage);
                break;
            }
            day_lock_storage_save(&s_lock_data);
        }
        /* محافظت طول‌عمر فلش: فلاش فقط هر ۱۰ دقیقه (۶۰۰ ثانیه) */
        if (s_lock_data.remain_days > 0 && (s_lock_data.sec_accumulator % 600UL) == 0) {
            day_lock_storage_save(&s_lock_data);
        }
    }
    else if ((s_lock_data.lock_state == LOCK_STAGE_ACTIVE_LEASE ||
              s_lock_data.lock_state == LOCK_STAGE_GRACE_10) &&
             s_lock_data.remain_days <= 0)
    {
        s_lock_data.lock_state = LOCK_STAGE_SOFT_LOCK;
        day_lock_storage_save(&s_lock_data);
    }

    /* اتمام چرخه جاری و گذار به قفل سخت */
    if (s_lock_data.lock_state == LOCK_STAGE_SOFT_LOCK && s_cycle_idle) {
        s_lock_data.lock_state = LOCK_STAGE_HARD_LOCK;
        day_lock_storage_save(&s_lock_data);
        day_lock_ui_show();
    }

    /* نمایش صفحه قفل در صورت قفل سخت یا تمپر */
    if (day_lock_is_locked()) {
        day_lock_ui_show();
    }
}

void day_lock_notify_time_adjusted(uint32_t new_epoch)
{
    bool valid = false;
    uint32_t ep = new_epoch;
    if (ep == 0) {
        ep = get_rtc_epoch(&valid);
    } else {
        valid = (ep > 1700000000ULL);
    }
    if (valid) {
        bool is_lease_active = (s_lock_data.lock_state == LOCK_STAGE_ACTIVE_LEASE ||
                                s_lock_data.lock_state == LOCK_STAGE_GRACE_10);

        if (is_lease_active && s_lock_data.last_known_epoch > 1700000000ULL) {
            if (ep + 43200U < s_lock_data.last_known_epoch) {
                /* عقب کشیدن ساعت بیش از ۱۲ ساعت -> خطای دستکاری (Tamper) */
                s_lock_data.lock_state = LOCK_STAGE_TAMPER;
                fault_log_record(FAULT_LEASE_EXPIRED, -1);
                ESP_LOGE(TAG, "RTC ROLLBACK in notify_time_adjusted! ep=%lu last=%lu",
                         (unsigned long)ep, (unsigned long)s_lock_data.last_known_epoch);
                day_lock_generate_challenge();
                day_lock_storage_save(&s_lock_data);
                day_lock_ui_show();
                return;
            } else if (ep > s_lock_data.last_known_epoch) {
                /* محاسبه دقیق زمان سپری‌شده در جهش به جلو و کسر روزها */
                uint32_t delta = ep - s_lock_data.last_known_epoch;
                s_lock_data.last_known_epoch = ep;
                s_lock_data.sec_accumulator += delta;
                while (s_lock_data.sec_accumulator >= 86400UL && s_lock_data.remain_days > 0) {
                    s_lock_data.sec_accumulator -= 86400UL;
                    s_lock_data.remain_days--;
                }
                if (s_lock_data.remain_days <= 0) {
                    s_lock_data.remain_days = 0;
                    s_lock_data.lock_state = LOCK_STAGE_SOFT_LOCK;
                    day_lock_generate_challenge();
                    fault_log_record(FAULT_LEASE_EXPIRED, (int8_t)s_lock_data.active_stage);
                    ESP_LOGW(TAG, "Time adjustment -> Stage %u lease days expired -> SOFT_LOCK",
                             (unsigned)s_lock_data.active_stage);
                    if (s_cycle_idle) {
                        s_lock_data.lock_state = LOCK_STAGE_HARD_LOCK;
                        day_lock_ui_show();
                    }
                }
            }
        } else {
            /* مد آزاد کارخانه یا عدم وجود قسط فعال */
            s_lock_data.last_known_epoch = ep;
        }

        /* در صورتی که به خاطر خطای قبلی ساعت در مد تمپر بوده، بررسی بازیابی */
        if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
            day_lock_check_rtc_recovery();
        }
        day_lock_storage_save(&s_lock_data);
        ESP_LOGI(TAG, "Day-Lock notified of time adjustment: last_known_epoch=%lu, remain_days=%d, state=%d",
                 (unsigned long)s_lock_data.last_known_epoch, (int)s_lock_data.remain_days, s_lock_data.lock_state);
    }
}

/* =========================================================================
 * مدیریت وضعیت تسویه مراحل اقساط (Persistent Stage Settlement)
 * ========================================================================= */

void day_lock_record_stage_settled(uint8_t stage)
{
    if (stage < 1 || stage > DAY_LOCK_MAX_STAGES) return;
    char date_buf[32] = {0};
    day_lock_get_future_date_str(0, date_buf, sizeof(date_buf), true);

    nvs_handle_t h;
    if (nvs_open("haf_lock", NVS_READWRITE, &h) == ESP_OK) {
        char key[16];
        snprintf(key, sizeof(key), "stg_ok_%u", (unsigned)stage);
        nvs_set_str(h, key, date_buf);
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGI(TAG, "Stage %u marked as settled on %s in NVS", (unsigned)stage, date_buf);
}

bool day_lock_is_stage_settled(uint8_t stage)
{
    if (stage < 1 || stage > DAY_LOCK_MAX_STAGES) return false;

    /* اگر مرحله فعال جلوتر از این مرحله است، قطعی تسویه شده */
    if (s_lock_data.active_stage > stage) return true;

    /* بررسی کلید NVS */
    nvs_handle_t h;
    if (nvs_open("haf_lock", NVS_READONLY, &h) == ESP_OK) {
        char key[16];
        snprintf(key, sizeof(key), "stg_ok_%u", (unsigned)stage);
        size_t req = 0;
        esp_err_t err = nvs_get_str(h, key, NULL, &req);
        nvs_close(h);
        if (err == ESP_OK && req > 1) return true;
    }
    return false;
}

bool day_lock_get_stage_settled_date(uint8_t stage, char *out_str, size_t out_sz)
{
    if (!out_str || out_sz < 11 || stage < 1 || stage > DAY_LOCK_MAX_STAGES) return false;

    nvs_handle_t h;
    if (nvs_open("haf_lock", NVS_READONLY, &h) == ESP_OK) {
        char key[16];
        snprintf(key, sizeof(key), "stg_ok_%u", (unsigned)stage);
        size_t req = out_sz;
        esp_err_t err = nvs_get_str(h, key, out_str, &req);
        nvs_close(h);
        if (err == ESP_OK && out_str[0] != '\0') {
            if (strlen(out_str) == 10 && out_str[4] == '/' && out_str[7] == '/') {
                int p1 = 0, p2 = 0, p3 = 0;
                if (sscanf(out_str, "%d/%d/%d", &p1, &p2, &p3) == 3 && p1 > 1000) {
                    snprintf(out_str, out_sz, "%02d/%02d/%04d", p3, p2, p1);
                }
            }
            return true;
        }
    }

    if (s_lock_data.active_stage > stage) {
        day_lock_get_future_date_str(0, out_str, out_sz, true);
        return true;
    }
    return false;
}

void day_lock_clear_stage_settled(uint8_t stage)
{
    if (stage < 1 || stage > DAY_LOCK_MAX_STAGES) return;
    nvs_handle_t h;
    if (nvs_open("haf_lock", NVS_READWRITE, &h) == ESP_OK) {
        char key[16];
        snprintf(key, sizeof(key), "stg_ok_%u", (unsigned)stage);
        nvs_erase_key(h, key);
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGI(TAG, "Stage %u settlement cleared from NVS (reactivated)", (unsigned)stage);
}
