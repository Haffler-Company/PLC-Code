#include "day_lock.h"
#include "day_lock_storage.h"
#include <Arduino.h>
#include <lvgl.h>
#include <RTClib.h>
#include "ui.h"
#include "ui_notification.h"
#include "sound.h"
#include "defines.h"

extern "C" bool lv_lock_c(uint32_t timeout_ms);
extern "C" void lv_unlock_c(void);

extern RTC_DS3231 rtc;
extern bool rtc_available;

/* Backward-compatible globals */
int day_lock_remain = 0;
uint8_t day_lock_active_stage = 0;
int day_lock_stage_days[DAY_LOCK_MAX_STAGES] = {0, 0, 0, 0, 0};
volatile bool device_locked = false;
bool day_lock_grace10_used = false;
uint32_t dev_lock_challenge = 12345;

/* Internal state record and timers */
static DayLockRecord_t s_rec;
static uint32_t s_last_tick_ms = 0;

/* LVGL 8.3 Apple HIG Pure White UI Elements */
static lv_obj_t *s_modal_mask = NULL;
static lv_obj_t *s_card = NULL;
static lv_obj_t *s_pin_label = NULL;
static lv_obj_t *s_chal_badge = NULL;
static lv_obj_t *s_chal_digits = NULL;
static lv_obj_t *s_err_banner = NULL;
static lv_obj_t *s_btnm = NULL;
static lv_obj_t *s_help_dialog = NULL;

static char s_entered_pin[16] = {0};
static uint8_t s_pin_len = 0;
static uint8_t s_selected_btnm_idx = 0;

/* Helper: Digit sum calculation */
static uint8_t calc_digit_sum(uint64_t n) {
    uint8_t sum = 0;
    while (n > 0) {
        sum = (uint8_t)(sum + (n % 10ULL));
        n /= 10ULL;
    }
    return sum;
}

/* Helper: 5-Stage Chain Multiplier code */
static void calc_stage_code(char *out_buf, size_t buf_sz, uint8_t stage, const char *admin_pin) {
    if (!out_buf || buf_sz == 0 || !admin_pin) return;
    const uint8_t mult = (uint8_t)(stage + 1U);
    size_t n = 0;
    uint16_t dsum = 0;
    for (const char *s = admin_pin; *s && n + 2 < buf_sz; ++s) {
        if (*s < '0' || *s > '9') continue;
        const uint8_t d = (uint8_t)(((*s - '0') * mult) % 10U);
        out_buf[n++] = (char)('0' + d);
        dsum = (uint16_t)(dsum + d);
    }
    if (n == 0) { out_buf[0] = 0; return; }
    out_buf[n] = (char)('0' + (dsum % 10U));
    out_buf[n + 1] = '\0';
}

/* Helper: Offline Challenge-Response code */
static void calc_challenge_response(char *out_buf, size_t buf_sz, uint32_t challenge) {
    if (!out_buf || buf_sz == 0) return;
    uint64_t base = (uint64_t)challenge * 2ULL;
    uint8_t checksum = (uint8_t)(calc_digit_sum(base) % 10U);
    snprintf(out_buf, buf_sz, "%llu%u", (unsigned long long)base, (unsigned)checksum);
}

/* Sync internal record to backward-compatible global symbols */
static void sync_globals_from_record(void) {
    day_lock_remain = s_rec.remain_days;
    day_lock_active_stage = s_rec.active_stage;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) {
        day_lock_stage_days[i] = s_rec.stage_days[i];
    }
    day_lock_grace10_used = s_rec.grace10_used;

    device_locked = (s_rec.active_stage == LOCK_STAGE_HARD_LOCK || 
                     s_rec.active_stage == LOCK_STAGE_TAMPER ||
                     (s_rec.active_stage > LOCK_STAGE_FACTORY_FREE && s_rec.remain_days <= 0));
}

/* Sync globals to record before saving */
static void sync_record_from_globals(void) {
    s_rec.remain_days = (int16_t)day_lock_remain;
    s_rec.active_stage = day_lock_active_stage;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) {
        s_rec.stage_days[i] = (int16_t)day_lock_stage_days[i];
    }
    s_rec.grace10_used = day_lock_grace10_used;
}

DayLockState_t day_lock_get_state(void) {
    return (DayLockState_t)s_rec.active_stage;
}

bool day_lock_is_soft_locked(void) {
    return (s_rec.active_stage == LOCK_STAGE_SOFT_LOCK);
}

bool day_lock_is_brute_locked(uint32_t *out_remaining_sec) {
    uint32_t now = millis();
    if (s_rec.lockout_until_ms > now) {
        if (out_remaining_sec) {
            *out_remaining_sec = (s_rec.lockout_until_ms - now) / 1000UL;
        }
        return true;
    }
    return false;
}

static uint32_t get_rtc_epoch(bool *out_valid) {
    if (out_valid) *out_valid = false;
    if (!rtc_available) return 0;

    DateTime now = rtc.now();
    if (now.year() < 2025) {
        return 0; // Year < 2025 indicates uninitialized or lost power
    }
    if (out_valid) *out_valid = true;
    return (uint32_t)now.unixtime();
}

void day_lock_issue_challenge(void) {
    uint32_t r = (uint32_t)esp_random();
    dev_lock_challenge = (uint32_t)((r % 90000UL) + 10000UL);
    if (s_chal_digits && lv_obj_is_valid(s_chal_digits)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%05u", (unsigned)dev_lock_challenge);
        lv_label_set_text(s_chal_digits, buf);
    }
}

void day_lock_init(void) {
    memset(&s_rec, 0, sizeof(DayLockRecord_t));

    if (!day_lock_storage_load(&s_rec)) {
        // First boot / factory initialization
        s_rec.magic = DAY_LOCK_MAGIC;
        s_rec.active_stage = LOCK_STAGE_FACTORY_FREE;
        s_rec.remain_days = 0;
        for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) s_rec.stage_days[i] = 0;
        s_rec.grace10_used = false;
        s_rec.last_known_epoch = 0;
        s_rec.sec_accumulator = 0;
        s_rec.failed_attempts = 0;
        s_rec.lockout_until_ms = 0;
        day_lock_storage_save(&s_rec);
    }

    sync_globals_from_record();
    day_lock_issue_challenge();
    s_last_tick_ms = millis();

    // Monotonic Anti-Tamper Check on Boot
    bool rtc_valid = false;
    uint32_t current_epoch = get_rtc_epoch(&rtc_valid);

    if (rtc_valid && s_rec.last_known_epoch > 0) {
        // Rollback check: If RTC time was wound back by more than 300 seconds
        if (current_epoch + 300UL < s_rec.last_known_epoch) {
            s_rec.active_stage = LOCK_STAGE_TAMPER;
            device_locked = true;
            day_lock_storage_save(&s_rec);
            sync_globals_from_record();
        }
    }

    if (rtc_valid && current_epoch > s_rec.last_known_epoch) {
        s_rec.last_known_epoch = current_epoch;
        day_lock_storage_save(&s_rec);
    }

    // Audible reminder on boot if days are low
    if (s_rec.active_stage == LOCK_STAGE_ACTIVE_LEASE || s_rec.active_stage == LOCK_STAGE_GRACE_10) {
        if (s_rec.remain_days > 0 && s_rec.remain_days <= 3) {
            sound_play(SOUND_ALARM_FAULT);
        }
    }
}

void day_lock_save(void) {
    sync_record_from_globals();
    day_lock_storage_save(&s_rec);
}

void day_lock_arm(int days) {
    if (days < 0) days = 0;
    s_rec.remain_days = (int16_t)days;
    s_rec.sec_accumulator = 0;
    s_rec.failed_attempts = 0;
    s_rec.lockout_until_ms = 0;
    if (days > 0) {
        s_rec.active_stage = LOCK_STAGE_ACTIVE_LEASE;
        device_locked = false;
    } else if (s_rec.active_stage > LOCK_STAGE_FACTORY_FREE) {
        s_rec.active_stage = LOCK_STAGE_HARD_LOCK;
        device_locked = true;
    }
    day_lock_storage_save(&s_rec);
    sync_globals_from_record();
}

void day_lock_start_chain(int days) {
    if (days <= 0) {
        day_lock_clear_after_unlock();
        return;
    }
    s_rec.stage_days[0] = (int16_t)days;
    s_rec.active_stage = LOCK_STAGE_ACTIVE_LEASE;
    day_lock_arm(days);
}

void day_lock_start_leasing(const int16_t stages[DAY_LOCK_MAX_STAGES]) {
    if (!stages) return;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) {
        s_rec.stage_days[i] = stages[i];
    }
    s_rec.active_stage = LOCK_STAGE_ACTIVE_LEASE;
    s_rec.remain_days = stages[0];
    s_rec.sec_accumulator = 0;
    s_rec.failed_attempts = 0;
    s_rec.lockout_until_ms = 0;
    device_locked = (s_rec.remain_days <= 0);

    bool rtc_valid = false;
    uint32_t epoch = get_rtc_epoch(&rtc_valid);
    if (rtc_valid) s_rec.last_known_epoch = epoch;

    day_lock_storage_save(&s_rec);
    sync_globals_from_record();
}

void day_lock_clear_after_unlock(void) {
    s_rec.active_stage = LOCK_STAGE_FACTORY_FREE;
    s_rec.remain_days = 0;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) s_rec.stage_days[i] = 0;
    s_rec.failed_attempts = 0;
    s_rec.sec_accumulator = 0;
    s_rec.lockout_until_ms = 0;
    device_locked = false;
    day_lock_storage_save(&s_rec);
    sync_globals_from_record();
    dev_lock_ui_hide();
}

bool day_lock_apply_grace10(void) {
    if (s_rec.grace10_used) return false;
    s_rec.grace10_used = true;
    s_rec.remain_days = (int16_t)(s_rec.remain_days + 10);
    s_rec.active_stage = LOCK_STAGE_GRACE_10;
    s_rec.sec_accumulator = 0;
    s_rec.failed_attempts = 0;
    s_rec.lockout_until_ms = 0;
    device_locked = false;
    day_lock_storage_save(&s_rec);
    sync_globals_from_record();
    dev_lock_ui_hide();
    return true;
}

bool day_lock_advance_stage(void) {
    for (uint8_t i = s_rec.active_stage; i < DAY_LOCK_MAX_STAGES; ++i) {
        const int16_t days = s_rec.stage_days[i];
        if (days <= 0) continue;
        s_rec.active_stage = (uint8_t)(i + 1);
        s_rec.remain_days = days;
        s_rec.sec_accumulator = 0;
        s_rec.failed_attempts = 0;
        s_rec.lockout_until_ms = 0;
        device_locked = false;
        day_lock_storage_save(&s_rec);
        sync_globals_from_record();
        dev_lock_ui_hide();
        return true;
    }
    day_lock_clear_after_unlock();
    return false;
}

void day_lock_sync_authorized_epoch(uint32_t new_epoch) {
    if (new_epoch > 1700000000UL) { // Valid timestamp (> Nov 2023)
        s_rec.last_known_epoch = new_epoch;
        day_lock_storage_save(&s_rec);
    }
}

bool day_lock_try_unlock(const char *pin) {
    if (!pin || !pin[0]) return false;

    // Check anti-brute force lockout
    if (day_lock_is_brute_locked(NULL)) return false;

    // 1. Tier 1: Factory Master Clear PINs
    if (strcmp(pin, MASTER_PIN_1) == 0 || strcmp(pin, MASTER_PIN_2) == 0 || strcmp(pin, MASTER_PIN_3) == 0) {
        day_lock_clear_after_unlock();
        return true;
    }

    // 2. Tier 4: One-Time Emergency Grace PIN
    if (strcmp(pin, EMERGENCY_GRACE_PIN) == 0) {
        if (day_lock_apply_grace10()) return true;
    }

    // 3. Tier 3: 5-Stage Installment Chain PINs
    if (s_rec.active_stage >= 1 && s_rec.active_stage <= DAY_LOCK_MAX_STAGES) {
        char exp_code1[24] = {0};
        char exp_code2[24] = {0};
        calc_stage_code(exp_code1, sizeof(exp_code1), s_rec.active_stage, DEFAULT_ADMIN_PIN_1);
        calc_stage_code(exp_code2, sizeof(exp_code2), s_rec.active_stage, DEFAULT_ADMIN_PIN_2);
        if (strcmp(pin, exp_code1) == 0 || strcmp(pin, exp_code2) == 0) {
            day_lock_advance_stage();
            return true;
        }
    }

    // 4. Tier 2: Dynamic One-Time Challenge Response
    char exp_chal[32] = {0};
    calc_challenge_response(exp_chal, sizeof(exp_chal), dev_lock_challenge);
    if (strcmp(pin, exp_chal) == 0) {
        if (s_rec.remain_days <= 0) s_rec.remain_days = 30; // 30-day challenge extension
        s_rec.active_stage = LOCK_STAGE_ACTIVE_LEASE;
        s_rec.sec_accumulator = 0;
        s_rec.failed_attempts = 0;
        s_rec.lockout_until_ms = 0;
        device_locked = false;
        day_lock_storage_save(&s_rec);
        sync_globals_from_record();
        dev_lock_ui_hide();
        return true;
    }

    // Failed attempt handling: Progressive Brute-Force Penalty
    s_rec.failed_attempts++;
    if (s_rec.failed_attempts >= 10) {
        s_rec.active_stage = LOCK_STAGE_TAMPER;
        device_locked = true;
    } else if (s_rec.failed_attempts >= 5) {
        s_rec.lockout_until_ms = millis() + (300UL * 1000UL); // 5 min freeze
    } else if (s_rec.failed_attempts >= 3) {
        s_rec.lockout_until_ms = millis() + (30UL * 1000UL);  // 30s freeze
    }

    day_lock_issue_challenge();
    day_lock_storage_save(&s_rec);
    sync_globals_from_record();
    return false;
}

void day_lock_tick_1s(void) {
    uint32_t now = millis();
    if (now - s_last_tick_ms < 1000) return;
    s_last_tick_ms = now;

    // Periodic Monotonic Clock Rollback Guard
    bool rtc_valid = false;
    uint32_t current_epoch = get_rtc_epoch(&rtc_valid);
    if (rtc_valid && s_rec.last_known_epoch > 0) {
        if (current_epoch + 300UL < s_rec.last_known_epoch) {
            s_rec.active_stage = LOCK_STAGE_TAMPER;
            device_locked = true;
            day_lock_storage_save(&s_rec);
            sync_globals_from_record();
        } else if (current_epoch > s_rec.last_known_epoch) {
            s_rec.last_known_epoch = current_epoch;
        }
    }

    // Lease countdown handling
    if ((s_rec.active_stage == LOCK_STAGE_ACTIVE_LEASE || s_rec.active_stage == LOCK_STAGE_GRACE_10) && 
        s_rec.remain_days > 0) 
    {
        s_rec.sec_accumulator++;
        if (s_rec.sec_accumulator >= 86400UL) {
            s_rec.sec_accumulator = 0;
            s_rec.remain_days--;
            if (s_rec.remain_days <= 0) {
                // Two-Phase Soft Lock Transition
                s_rec.active_stage = LOCK_STAGE_SOFT_LOCK;
                day_lock_issue_challenge();
                ui_show_notification(NOTIF_WARNING, "هشدار انقضای مهلت", "اعتبار کارکرد روزشمار به پایان رسید.");
            } else if (s_rec.remain_days <= 3) {
                char warnBuf[64];
                snprintf(warnBuf, sizeof(warnBuf), "تنها %d روز از اعتبار کارکرد باقی مانده است", s_rec.remain_days);
                ui_show_notification(NOTIF_WARNING, "هشدار مهلت لایسنس", warnBuf);
            }
            day_lock_storage_save(&s_rec);
            sync_globals_from_record();
        } else if ((s_rec.sec_accumulator % 3600UL) == 0) {
            // Hourly endurance flush to NVS
            day_lock_storage_save(&s_rec);
        }
    } else if (s_rec.active_stage == LOCK_STAGE_SOFT_LOCK) {
        // If not running active cutting cycle, transition to hard lock
        extern bool cycleStarted;
        if (!cycleStarted) {
            s_rec.active_stage = LOCK_STAGE_HARD_LOCK;
            device_locked = true;
            day_lock_storage_save(&s_rec);
            sync_globals_from_record();
        }
    } else if (s_rec.active_stage > LOCK_STAGE_FACTORY_FREE && s_rec.remain_days <= 0) {
        s_rec.active_stage = LOCK_STAGE_HARD_LOCK;
        device_locked = true;
    }

    // Modal UI trigger
    if ((s_rec.active_stage == LOCK_STAGE_HARD_LOCK || s_rec.active_stage == LOCK_STAGE_TAMPER) && !s_modal_mask) {
        dev_lock_ui_show();
    }

    // Update lockout banner countdown if active
    if (s_modal_mask && s_err_banner && lv_obj_is_valid(s_err_banner)) {
        uint32_t rem_sec = 0;
        if (day_lock_is_brute_locked(&rem_sec)) {
            char freeze_msg[64];
            snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است (%u ثانیه صبر کنید)", (unsigned)rem_sec);
            lv_label_set_text(s_err_banner, freeze_msg);
            lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* --------------------------------------------------------------------------
 * LVGL 8.3 Apple HIG Pure White UI Implementation
 * -------------------------------------------------------------------------- */
static void help_dialog_close_cb(lv_event_t *e) {
    (void)e;
    if (s_help_dialog && lv_obj_is_valid(s_help_dialog)) {
        lv_obj_del(s_help_dialog);
        s_help_dialog = NULL;
    }
}

static void help_dialog_open_cb(lv_event_t *e) {
    (void)e;
    if (s_help_dialog) return;

    s_help_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_help_dialog, 430, 260);
    lv_obj_center(s_help_dialog);
    lv_obj_set_style_bg_color(s_help_dialog, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_help_dialog, lv_color_hex(0x007AFF), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_help_dialog, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_help_dialog, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_help_dialog, 16, LV_PART_MAIN);
    lv_obj_clear_flag(s_help_dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(s_help_dialog);
    lv_obj_set_style_text_font(t, &ui_font_Dana_Bold18, LV_PART_MAIN);
    lv_label_set_text(t, "راهنمای پشتیبانی و فعال‌سازی هافلر");
    lv_obj_set_style_text_color(t, lv_color_hex(0x007AFF), LV_PART_MAIN);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *msg = lv_label_create(s_help_dialog);
    lv_obj_set_width(msg, 390);
    lv_obj_set_style_text_font(msg, &ui_font_Dana_Bold13, LV_PART_MAIN);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_label_set_text(msg, "۱. اعتبار روزشمار دستگاه چانه‌گیر به پایان رسیده است.\n"
                           "۲. کد ۵ رقمی مندرج در کادر آبی را به کارخانه اعلام نمایید.\n"
                           "۳. رمز دریافت شده را از طریق صفحه کلید وارد و کلید OK را لمس کنید.\n"
                           "مرکز پشتیبانی کارخانه: ۰۲۱-۶۶۲۶۵۴۳۰  |  پشتیبانی فنی هافلر");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -4);

    lv_obj_t *btn_close = lv_btn_create(s_help_dialog);
    lv_obj_set_size(btn_close, 110, 36);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x007AFF), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_close, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_close, help_dialog_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_c = lv_label_create(btn_close);
    lv_obj_set_style_text_font(lbl_c, &ui_font_Dana_Bold13, LV_PART_MAIN);
    lv_label_set_text(lbl_c, "بستن");
    lv_obj_center(lbl_c);
}

static void keypad_click_cb(lv_event_t *e) {
    uint32_t rem_sec = 0;
    if (day_lock_is_brute_locked(&rem_sec)) {
        if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
            char freeze_msg[64];
            snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است (%u ثانیه صبر کنید)", (unsigned)rem_sec);
            lv_label_set_text(s_err_banner, freeze_msg);
            lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    lv_obj_t *btnm = (lv_obj_t *)lv_event_get_target(e);
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(btnm);
    const char *txt = lv_btnmatrix_get_btn_text(btnm, btn_id);
    if (!txt) return;

    if (strcmp(txt, "OK") == 0) {
        if (s_pin_len > 0) {
            if (day_lock_try_unlock(s_entered_pin)) {
                dev_lock_ui_hide();
                ui_show_notification(NOTIF_SUCCESS, "فعالسازی موفق", "روزشمار دستگاه تمدید و قفل باز گردید");
            } else {
                s_pin_len = 0;
                s_entered_pin[0] = '\0';
                if (s_pin_label && lv_obj_is_valid(s_pin_label)) {
                    lv_label_set_text(s_pin_label, "••••••");
                }
                if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
                    lv_label_set_text(s_err_banner, "رمز نامعتبر است! مجدداً تلاش فرمایید");
                    lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
                }
                ui_show_notification(NOTIF_ERROR, "خطای رمز", "کد وارد شده صحیح نمی باشد");
            }
        }
    } else if (strcmp(txt, "DEL") == 0 || strcmp(txt, "حذف") == 0) {
        if (s_pin_len > 0) {
            s_pin_len--;
            s_entered_pin[s_pin_len] = '\0';
            char bullets[16] = {0};
            for (uint8_t i = 0; i < s_pin_len; i++) bullets[i] = '*';
            if (s_pin_label && lv_obj_is_valid(s_pin_label)) {
                lv_label_set_text(s_pin_label, s_pin_len == 0 ? "ورود رمز" : bullets);
            }
        }
    } else if (txt[0] >= '0' && txt[0] <= '9') {
        if (s_pin_len < 12) {
            s_entered_pin[s_pin_len++] = txt[0];
            s_entered_pin[s_pin_len] = '\0';
            char bullets[16] = {0};
            for (uint8_t i = 0; i < s_pin_len; i++) bullets[i] = '*';
            if (s_pin_label && lv_obj_is_valid(s_pin_label)) {
                lv_label_set_text(s_pin_label, bullets);
            }
            if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
                lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void dev_lock_ui_show(void) {
    if (s_modal_mask) return;
    if (!lv_lock_c(100)) return;

    // 1. Semi-translucent Light Overlay (Pure Apple HIG White Theme)
    s_modal_mask = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_modal_mask, 480, 320);
    lv_obj_set_pos(s_modal_mask, 0, 0);
    lv_obj_set_style_bg_color(s_modal_mask, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_modal_mask, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_modal_mask, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_modal_mask, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Central Pure White Floating Card
    s_card = lv_obj_create(s_modal_mask);
    lv_obj_set_size(s_card, 456, 300);
    lv_obj_center(s_card);
    lv_obj_set_style_bg_color(s_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(s_card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_card, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_card, 0, LV_PART_MAIN); // Zero-lag policy
    lv_obj_set_style_pad_all(s_card, 8, LV_PART_MAIN);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);

    // 3. Header Bar with Title and Help Button
    lv_obj_t *title = lv_label_create(s_card);
    lv_obj_set_style_text_font(title, &ui_font_Dana_Bold18, LV_PART_MAIN);
    lv_label_set_text(title, "دستگاه قفل است - روزشمار هافلر");
    lv_obj_set_style_text_color(title, lv_color_hex(0xEF4444), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *btn_help = lv_btn_create(s_card);
    lv_obj_set_size(btn_help, 36, 36);
    lv_obj_align(btn_help, LV_ALIGN_TOP_LEFT, 4, 0);
    lv_obj_set_style_bg_color(btn_help, lv_color_hex(0xF1F5F9), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_help, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_help, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_help, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_help, help_dialog_open_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_q = lv_label_create(btn_help);
    lv_obj_set_style_text_font(lbl_q, &ui_font_Dana_Bold18, LV_PART_MAIN);
    lv_label_set_text(lbl_q, "?");
    lv_obj_set_style_text_color(lbl_q, lv_color_hex(0x007AFF), LV_PART_MAIN);
    lv_obj_center(lbl_q);

    // 4. Challenge Readout Pill Badge (Left Side)
    s_chal_badge = lv_obj_create(s_card);
    lv_obj_set_size(s_chal_badge, 174, 76);
    lv_obj_align(s_chal_badge, LV_ALIGN_TOP_LEFT, 6, 42);
    lv_obj_set_style_bg_color(s_chal_badge, lv_color_hex(0xEFF6FF), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_chal_badge, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_chal_badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_chal_badge, 10, LV_PART_MAIN);
    lv_obj_clear_flag(s_chal_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_c_title = lv_label_create(s_chal_badge);
    lv_obj_set_style_text_font(lbl_c_title, &ui_font_Dana_Bold13, LV_PART_MAIN);
    lv_label_set_text(lbl_c_title, "کد بازیابی دستگاه");
    lv_obj_set_style_text_color(lbl_c_title, lv_color_hex(0x1E40AF), LV_PART_MAIN);
    lv_obj_align(lbl_c_title, LV_ALIGN_TOP_MID, 0, 2);

    s_chal_digits = lv_label_create(s_chal_badge);
    lv_obj_set_style_text_font(s_chal_digits, &ui_font_Dana_Bold18, LV_PART_MAIN);
    char buf[16];
    snprintf(buf, sizeof(buf), "%05u", (unsigned)dev_lock_challenge);
    lv_label_set_text(s_chal_digits, buf);
    lv_obj_set_style_text_color(s_chal_digits, lv_color_hex(0x007AFF), LV_PART_MAIN);
    lv_obj_align(s_chal_digits, LV_ALIGN_BOTTOM_MID, 0, -4);

    // 5. PIN Bullet Display Box
    lv_obj_t *pin_box = lv_obj_create(s_card);
    lv_obj_set_size(pin_box, 174, 46);
    lv_obj_align(pin_box, LV_ALIGN_TOP_LEFT, 6, 126);
    lv_obj_set_style_bg_color(pin_box, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_border_color(pin_box, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_box, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(pin_box, 8, LV_PART_MAIN);

    s_pin_label = lv_label_create(pin_box);
    lv_obj_set_style_text_font(s_pin_label, &ui_font_Dana_Bold18, LV_PART_MAIN);
    lv_label_set_text(s_pin_label, "ورود رمز");
    lv_obj_set_style_text_color(s_pin_label, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_center(s_pin_label);

    // 6. Error Banner
    s_err_banner = lv_label_create(s_card);
    lv_obj_set_style_text_font(s_err_banner, &ui_font_Dana_Bold13, LV_PART_MAIN);
    lv_label_set_text(s_err_banner, "");
    lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), LV_PART_MAIN);
    lv_obj_align(s_err_banner, LV_ALIGN_TOP_LEFT, 6, 178);
    lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);

    // 7. Tactile 4x3 Light ButtonMatrix Keypad (Right Side)
    static const char *kmap[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "DEL", "0", "OK", ""
    };
    s_btnm = lv_btnmatrix_create(s_card);
    lv_btnmatrix_set_map(s_btnm, kmap);
    lv_obj_set_size(s_btnm, 240, 220);
    lv_obj_align(s_btnm, LV_ALIGN_TOP_RIGHT, -4, 40);
    lv_obj_set_style_bg_opa(s_btnm, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_btnm, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_btnm, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_btnm, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_column(s_btnm, 4, LV_PART_MAIN);

    // Keypad Tile Style (Light Gray on White)
    lv_obj_set_style_bg_color(s_btnm, lv_color_hex(0xF1F5F9), LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_btnm, lv_color_hex(0xCBD5E1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_btnm, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_btnm, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_btnm, lv_color_hex(0x0F172A), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_btnm, &ui_font_Dana_Bold18, LV_PART_ITEMS);
    lv_obj_add_event_cb(s_btnm, keypad_click_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_unlock_c();
}

void dev_lock_ui_hide(void) {
    if (!s_modal_mask) return;
    if (!lv_lock_c(100)) return;

    if (s_help_dialog && lv_obj_is_valid(s_help_dialog)) {
        lv_obj_del(s_help_dialog);
        s_help_dialog = NULL;
    }
    if (s_modal_mask && lv_obj_is_valid(s_modal_mask)) {
        lv_obj_del(s_modal_mask);
        s_modal_mask = NULL;
        s_card = NULL;
        s_pin_label = NULL;
        s_chal_badge = NULL;
        s_chal_digits = NULL;
        s_err_banner = NULL;
        s_btnm = NULL;
        s_pin_len = 0;
        s_entered_pin[0] = '\0';
    }
    lv_unlock_c();
}

bool dev_lock_ui_is_active(void) {
    return (s_modal_mask != NULL);
}

void dev_lock_ui_process_key(uint8_t nav_key) {
    if (!s_btnm || !lv_obj_is_valid(s_btnm)) return;

    // Physical key fallback mappings:
    // nav_key 4 (MODE): Cycle selection through keys (0..11)
    // nav_key 5 (START): Click focused key
    // nav_key 6 (STOP): Delete char
    if (nav_key == 4) {
        s_selected_btnm_idx = (uint8_t)((s_selected_btnm_idx + 1) % 12);
        lv_btnmatrix_set_selected_btn(s_btnm, s_selected_btnm_idx);
    } else if (nav_key == 5) {
        lv_event_send(s_btnm, LV_EVENT_VALUE_CHANGED, NULL);
    } else if (nav_key == 6) {
        s_selected_btnm_idx = 9; // DEL button index
        lv_btnmatrix_set_selected_btn(s_btnm, s_selected_btnm_idx);
        lv_event_send(s_btnm, LV_EVENT_VALUE_CHANGED, NULL);
    }
}
