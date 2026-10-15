#include "day_lock.h"
#include "a2_eeprom.h"
#include "variables.h"
#include "panel_keys.h"
#include <Arduino.h>
#include <Preferences.h>
#include <esp_rom_crc.h>
#include <esp_mac.h>
#include <Wire.h>
#include <time.h>
#include <lvgl.h>
#include "fonts.h"
#include "ui_vazir_fonts.h"
#include "draw/lv_draw_private.h"

int day_lock_remain = 0;
uint8_t day_lock_active_stage = 0;
int day_lock_stage_days[5] = {0, 0, 0, 0, 0};
volatile bool device_locked = false;
bool day_lock_grace10_used = false;
uint32_t dev_lock_challenge = 1234;

static DayLockRecord_t s_lock_data;
static Preferences s_lock_pref;
static const char *NVS_NAMESPACE = "haffler_lock";
static const char *KEY_SLOT_A    = "meta_a";
static const char *KEY_SLOT_B    = "meta_b";

static uint32_t s_last_tick_ms = 0;
static uint32_t s_brute_lockout_end_ms = 0;

/* LVGL UI elements for Apple HIG Pure White Lock Screen */
static lv_obj_t *s_lock_modal = NULL;
static lv_obj_t *s_lock_card = NULL;
static lv_obj_t *s_lock_title = NULL;
static lv_obj_t *s_lock_pin_lbl = NULL;
static lv_obj_t *s_lock_challenge_lbl = NULL;
static lv_obj_t *s_lock_err_lbl = NULL;
static lv_obj_t *s_lock_btnm = NULL;
static lv_obj_t *s_lock_help_modal = NULL;

static char s_entered_pin[16] = {0};
static uint8_t s_pin_len = 0;
static uint8_t s_selected_btn_idx = 0;

static uint32_t calc_record_crc(const DayLockRecord_t *rec) {
    return esp_rom_crc32_le(0, (const uint8_t*)rec, sizeof(DayLockRecord_t) - sizeof(uint32_t));
}

static uint8_t calc_digit_sum(uint64_t n) {
    uint8_t sum = 0;
    do {
        sum = (uint8_t)(sum + (n % 10ULL));
        n /= 10ULL;
    } while (n);
    return sum;
}

static void calc_stage_code(char *out, size_t out_sz, uint8_t stage) {
    if (!out || out_sz == 0) return;
    const char *pin = (EEprom6_AdminPin[0] >= '0' && EEprom6_AdminPin[0] <= '9') ? EEprom6_AdminPin : DEFAULT_ADMIN_PIN;
    size_t n = 0;
    uint16_t dsum = 0;
    const uint8_t mult = (uint8_t)(stage + 1U); // Stage 1 -> x2, Stage 2 -> x3, etc.
    for (const char *s = pin; *s && n + 2 < out_sz; ++s) {
        if (*s < '0' || *s > '9') continue;
        const uint8_t d = (uint8_t)(((*s - '0') * mult) % 10U);
        out[n++] = (char)('0' + d);
        dsum = (uint16_t)(dsum + d);
    }
    if (n == 0) { out[0] = 0; return; }
    out[n] = (char)('0' + (dsum % 10U));
    out[n + 1] = '\0';
}

static void calc_challenge_code(char *out, size_t out_sz, uint32_t challenge) {
    if (!out || out_sz == 0) return;
    uint64_t base = (uint64_t)challenge * 2ULL;
    snprintf(out, out_sz, "%llu%u", (unsigned long long)base, (unsigned)(calc_digit_sum(base) % 10U));
}

static uint32_t hal_rtc_get_epoch(bool *out_valid) {
    if (out_valid) *out_valid = false;
    time_t now = time(NULL);
    if (now > 1672531200) { // Valid time after 2023-01-01
        if (out_valid) *out_valid = true;
        return (uint32_t)now;
    }
    return 0;
}

static bool day_lock_storage_load(DayLockRecord_t *out_rec) {
    if (!out_rec) return false;
    s_lock_pref.begin(NVS_NAMESPACE, true);
    
    DayLockRecord_t recA, recB;
    size_t szA = s_lock_pref.getBytes(KEY_SLOT_A, &recA, sizeof(DayLockRecord_t));
    size_t szB = s_lock_pref.getBytes(KEY_SLOT_B, &recB, sizeof(DayLockRecord_t));
    
    bool legacy_has = s_lock_pref.isKey("remain");
    int leg_remain = s_lock_pref.getInt("remain", 0);
    uint8_t leg_stage = (uint8_t)s_lock_pref.getUChar("stage", 0);
    int leg_stg0 = s_lock_pref.getInt("stg0", 0);
    int leg_stg1 = s_lock_pref.getInt("stg1", 0);
    int leg_stg2 = s_lock_pref.getInt("stg2", 0);
    int leg_stg3 = s_lock_pref.getInt("stg3", 0);
    int leg_stg4 = s_lock_pref.getInt("stg4", 0);
    bool leg_grace = s_lock_pref.getBool("grace", false);
    uint32_t leg_acc = s_lock_pref.getUInt("sec_acc", 0);
    s_lock_pref.end();

    bool validA = (szA == sizeof(DayLockRecord_t)) && (recA.magic == DAY_LOCK_MAGIC) && 
                  (calc_record_crc(&recA) == recA.crc32);
    bool validB = (szB == sizeof(DayLockRecord_t)) && (recB.magic == DAY_LOCK_MAGIC) && 
                  (calc_record_crc(&recB) == recB.crc32);

    if (validA) {
        *out_rec = recA;
        return true;
    } else if (validB) {
        *out_rec = recB;
        return true;
    } else if (legacy_has) {
        /* Migrate legacy settings into new mirrored struct */
        memset(out_rec, 0, sizeof(DayLockRecord_t));
        out_rec->magic = DAY_LOCK_MAGIC;
        out_rec->remain_days = leg_remain;
        out_rec->active_stage = (leg_stage >= 1 && leg_stage <= 5) ? leg_stage : (leg_remain > 0 ? 1 : 0);
        out_rec->lock_state = (leg_stage >= LOCK_STAGE_HARD_LOCK || (leg_stage > 0 && leg_remain <= 0)) ? 
                               (uint8_t)LOCK_STAGE_HARD_LOCK : (out_rec->active_stage > 0 ? (uint8_t)LOCK_STAGE_ACTIVE_LEASE : 0);
        out_rec->stage_days[0] = leg_stg0;
        out_rec->stage_days[1] = leg_stg1;
        out_rec->stage_days[2] = leg_stg2;
        out_rec->stage_days[3] = leg_stg3;
        out_rec->stage_days[4] = leg_stg4;
        out_rec->grace10_used = leg_grace;
        out_rec->sec_accumulator = leg_acc;
        out_rec->crc32 = calc_record_crc(out_rec);
        return true;
    }
    return false; // First time boot
}

static void day_lock_storage_save(DayLockRecord_t *rec) {
    if (!rec) return;
    rec->magic = DAY_LOCK_MAGIC;
    rec->crc32 = calc_record_crc(rec);

    s_lock_pref.begin(NVS_NAMESPACE, false);
    s_lock_pref.putBytes(KEY_SLOT_A, rec, sizeof(DayLockRecord_t));
    s_lock_pref.putBytes(KEY_SLOT_B, rec, sizeof(DayLockRecord_t));
    /* Maintain legacy keys for full backwards compatibility */
    s_lock_pref.putInt("remain", rec->remain_days);
    s_lock_pref.putUChar("stage", rec->active_stage);
    s_lock_pref.putInt("stg0", rec->stage_days[0]);
    s_lock_pref.putInt("stg1", rec->stage_days[1]);
    s_lock_pref.putInt("stg2", rec->stage_days[2]);
    s_lock_pref.putInt("stg3", rec->stage_days[3]);
    s_lock_pref.putInt("stg4", rec->stage_days[4]);
    s_lock_pref.putBool("grace", rec->grace10_used);
    s_lock_pref.putBool("locked", (rec->lock_state >= LOCK_STAGE_HARD_LOCK));
    s_lock_pref.putUInt("sec_acc", rec->sec_accumulator);
    s_lock_pref.end();
}

DayLockState_t day_lock_get_state(void) {
    return (DayLockState_t)s_lock_data.lock_state;
}

int16_t day_lock_get_remain_days(void) {
    return s_lock_data.remain_days;
}

uint8_t day_lock_get_failed_attempts(void) {
    return s_lock_data.failed_attempts;
}

bool day_lock_is_soft_locked(void) {
    return (s_lock_data.lock_state == LOCK_STAGE_SOFT_LOCK);
}

bool day_lock_is_tamper_locked(void) {
    return (s_lock_data.lock_state == LOCK_STAGE_TAMPER);
}

bool day_lock_is_brute_locked(uint32_t *out_remaining_sec) {
    uint32_t now = millis();
    if (s_brute_lockout_end_ms > 0 && now < s_brute_lockout_end_ms) {
        if (out_remaining_sec) *out_remaining_sec = (s_brute_lockout_end_ms - now) / 1000UL;
        return true;
    }
    if (now >= s_brute_lockout_end_ms) {
        s_brute_lockout_end_ms = 0;
    }
    return false;
}

void day_lock_init(void) {
    if (!day_lock_storage_load(&s_lock_data)) {
        memset(&s_lock_data, 0, sizeof(DayLockRecord_t));
        s_lock_data.magic = DAY_LOCK_MAGIC;
        s_lock_data.active_stage = LOCK_STAGE_FACTORY_FREE;
        s_lock_data.lock_state = LOCK_STAGE_FACTORY_FREE;
        s_lock_data.remain_days = 0;
        day_lock_storage_save(&s_lock_data);
    }

    // Sync globals
    day_lock_remain = s_lock_data.remain_days;
    day_lock_active_stage = s_lock_data.active_stage;
    for (int i = 0; i < 5; i++) day_lock_stage_days[i] = s_lock_data.stage_days[i];
    day_lock_grace10_used = s_lock_data.grace10_used;

    bool rtc_valid = false;
    uint32_t current_epoch = hal_rtc_get_epoch(&rtc_valid);

    // Monotonic Anti-Tamper Guard: Clock rollback detection (> 300s)
    if (rtc_valid && s_lock_data.last_known_epoch > 0) {
        if (current_epoch + 300 < s_lock_data.last_known_epoch) {
            s_lock_data.lock_state = LOCK_STAGE_TAMPER;
            day_lock_storage_save(&s_lock_data);
        }
    }

    if (rtc_valid && current_epoch > s_lock_data.last_known_epoch) {
        s_lock_data.last_known_epoch = current_epoch;
        day_lock_storage_save(&s_lock_data);
    }

    if (s_lock_data.lock_state == LOCK_STAGE_TAMPER || s_lock_data.lock_state == LOCK_STAGE_HARD_LOCK || 
       (s_lock_data.active_stage > 0 && s_lock_data.remain_days <= 0)) {
        device_locked = true;
        if (s_lock_data.lock_state < LOCK_STAGE_HARD_LOCK) {
            s_lock_data.lock_state = LOCK_STAGE_HARD_LOCK;
        }
    } else {
        device_locked = false;
    }

    // Acoustic ISA-18.2 Warning on Boot (Non-blocking buzzer)
    if (buzzerEnable && EEprom3_Buzzer) {
        if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
            buzzerTimeVlaue = 4;
        } else if (device_locked) {
            buzzerTimeVlaue = 3;
        } else if (s_lock_data.active_stage > 0 && s_lock_data.remain_days <= 3 && s_lock_data.remain_days > 0) {
            buzzerTimeVlaue = 2;
        }
    } else {
        buzzerTimeVlaue = 0;
    }

    s_last_tick_ms = millis();
    day_lock_issue_challenge();
}

void day_lock_save(void) {
    s_lock_data.remain_days = (int16_t)day_lock_remain;
    s_lock_data.active_stage = day_lock_active_stage;
    for (int i = 0; i < 5; i++) s_lock_data.stage_days[i] = (int16_t)day_lock_stage_days[i];
    s_lock_data.grace10_used = day_lock_grace10_used;
    day_lock_storage_save(&s_lock_data);
}

void day_lock_issue_challenge(void) {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint32_t mac_hash = esp_rom_crc32_le(0, mac, sizeof(mac));
    time_t now = time(NULL);
    uint32_t seed = esp_rom_crc32_le(mac_hash, (const uint8_t*)&now, sizeof(now)) ^ esp_random();
    dev_lock_challenge = (uint32_t)((seed % 9000UL) + 1000UL);
    if (s_lock_challenge_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04u", (unsigned)dev_lock_challenge);
        lv_label_set_text(s_lock_challenge_lbl, buf);
    }
}

void day_lock_arm(int days) {
    if (days < 0) days = 0;
    day_lock_remain = days;
    s_lock_data.remain_days = (int16_t)days;
    s_lock_data.active_stage = 1;
    day_lock_active_stage = 1;
    s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    s_brute_lockout_end_ms = 0;
    device_locked = (days <= 0);
    day_lock_save();
}

void day_lock_start_chain(int days) {
    if (days <= 0) {
        day_lock_clear_after_unlock();
        return;
    }
    day_lock_stage_days[0] = days;
    day_lock_arm(days);
}

void day_lock_set_stage_days(uint8_t stage_idx, int days) {
    if (stage_idx >= 5) return;
    if (days < 0) days = 0;
    if (days > 999) days = 999;
    day_lock_stage_days[stage_idx] = days;
    s_lock_data.stage_days[stage_idx] = (int16_t)days;
    day_lock_save();
}

int day_lock_get_stage_days(uint8_t stage_idx) {
    if (stage_idx >= 5) return 0;
    return day_lock_stage_days[stage_idx];
}

void day_lock_arm_chain(void) {
    for (uint8_t i = 0; i < 5; i++) {
        if (day_lock_stage_days[i] > 0) {
            day_lock_active_stage = (uint8_t)(i + 1);
            s_lock_data.active_stage = (uint8_t)(i + 1);
            s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
            day_lock_remain = day_lock_stage_days[i];
            s_lock_data.remain_days = (int16_t)day_lock_remain;
            s_lock_data.sec_accumulator = 0;
            s_lock_data.failed_attempts = 0;
            s_brute_lockout_end_ms = 0;
            device_locked = false;
            day_lock_save();
            return;
        }
    }
    day_lock_clear_after_unlock();
}

void day_lock_unlock_to_stage(uint8_t target_stage) {
    if (target_stage < 1 || target_stage > DAY_LOCK_MAX_STAGES) return;
    s_lock_data.active_stage = target_stage;
    day_lock_active_stage = target_stage;
    int days = s_lock_data.stage_days[target_stage - 1];
    if (days <= 0) days = 30; // 30-day default
    s_lock_data.remain_days = (int16_t)days;
    day_lock_remain = days;
    s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    s_brute_lockout_end_ms = 0;
    device_locked = false;
    day_lock_save();
    dev_lock_ui_hide();
}

void day_lock_clear_after_unlock(void) {
    device_locked = false;
    day_lock_remain = 0;
    day_lock_active_stage = 0;
    s_lock_data.active_stage = LOCK_STAGE_FACTORY_FREE;
    s_lock_data.lock_state = LOCK_STAGE_FACTORY_FREE;
    s_lock_data.remain_days = 0;
    for (int i = 0; i < 5; i++) {
        day_lock_stage_days[i] = 0;
        s_lock_data.stage_days[i] = 0;
    }
    s_lock_data.failed_attempts = 0;
    s_lock_data.sec_accumulator = 0;
    s_brute_lockout_end_ms = 0;
    day_lock_save();
    dev_lock_ui_hide();
}

bool day_lock_apply_grace10(void) {
    if (day_lock_grace10_used || s_lock_data.grace10_used) return false;
    day_lock_grace10_used = true;
    s_lock_data.grace10_used = true;
    day_lock_remain += 10;
    s_lock_data.remain_days = day_lock_remain;
    s_lock_data.lock_state = LOCK_STAGE_GRACE_10;
    if (s_lock_data.active_stage == 0) {
        s_lock_data.active_stage = 1;
        day_lock_active_stage = 1;
    }
    device_locked = false;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    s_brute_lockout_end_ms = 0;
    day_lock_save();
    dev_lock_ui_hide();
    return true;
}

bool day_lock_advance_stage(void) {
    uint8_t next_stage = s_lock_data.active_stage + 1;
    if (next_stage <= DAY_LOCK_MAX_STAGES && s_lock_data.stage_days[next_stage - 1] > 0) {
        day_lock_unlock_to_stage(next_stage);
        return true;
    }
    day_lock_clear_after_unlock();
    return false;
}

bool day_lock_try_unlock(const char *raw_pin) {
    if (!raw_pin || !raw_pin[0]) return false;

    // Check anti-brute force lockout
    if (day_lock_is_brute_locked(NULL)) return false;

    /* 1. Tier 1: Factory Master PINs */
    if (strcmp(raw_pin, MASTER_UNLOCK_PIN) == 0 || strcmp(raw_pin, MASTER_PIN_2) == 0 || strcmp(raw_pin, MASTER_PIN_3) == 0) {
        day_lock_clear_after_unlock();
        return true;
    }

    /* 2. Tier 4: Emergency Grace 10-day code */
    if (strcmp(raw_pin, EMERGENCY_GRACE_PIN) == 0) {
        if (day_lock_apply_grace10()) return true;
    }

    /* 3. Tier 3: 5-Stage Installment Chain Multiplier PINs */
    for (uint8_t stg = 1; stg <= DAY_LOCK_MAX_STAGES; ++stg) {
        char exp_stage[24] = {0};
        calc_stage_code(exp_stage, sizeof(exp_stage), stg);
        if (strcmp(raw_pin, exp_stage) == 0) {
            day_lock_unlock_to_stage(stg);
            return true;
        }
    }

    /* 4. Tier 2: Dynamic One-Time Challenge Code */
    char exp_chal[32] = {0};
    calc_challenge_code(exp_chal, sizeof(exp_chal), dev_lock_challenge);
    if (strcmp(raw_pin, exp_chal) == 0) {
        /* Challenge code gives 30-day lease extension */
        if (day_lock_remain <= 0) day_lock_remain = 30;
        s_lock_data.remain_days = day_lock_remain;
        s_lock_data.lock_state = LOCK_STAGE_ACTIVE_LEASE;
        if (s_lock_data.active_stage == 0) {
            s_lock_data.active_stage = 1;
            day_lock_active_stage = 1;
        }
        device_locked = false;
        s_lock_data.failed_attempts = 0;
        s_brute_lockout_end_ms = 0;
        day_lock_save();
        dev_lock_ui_hide();
        return true;
    }

    /* Failed attempt handling: Progressive Brute-Force Penalty */
    s_lock_data.failed_attempts++;
    if (s_lock_data.failed_attempts >= 10) {
        s_lock_data.lock_state = LOCK_STAGE_TAMPER;
        device_locked = true;
    } else if (s_lock_data.failed_attempts >= 5) {
        s_brute_lockout_end_ms = millis() + (300UL * 1000UL); // 5 min freeze
    } else if (s_lock_data.failed_attempts >= 3) {
        s_brute_lockout_end_ms = millis() + (30UL * 1000UL);  // 30s freeze
    }

    day_lock_issue_challenge();
    day_lock_save();
    return false;
}

void day_lock_tick_1s(void) {
    uint32_t now = millis();
    if (now - s_last_tick_ms < 1000) return;
    s_last_tick_ms = now;

    // Update brute freeze countdown if modal is active
    if (s_lock_modal && s_lock_err_lbl) {
        uint32_t rem_sec = 0;
        if (day_lock_is_brute_locked(&rem_sec)) {
            char freeze_msg[64];
            snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است (%u ثانیه صبر کنید)", (unsigned)rem_sec);
            lv_label_set_text(s_lock_err_lbl, freeze_msg);
            lv_obj_clear_flag(s_lock_err_lbl, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if ((s_lock_data.lock_state == LOCK_STAGE_ACTIVE_LEASE || s_lock_data.lock_state == LOCK_STAGE_GRACE_10) && 
        day_lock_remain > 0) 
    {
        s_lock_data.sec_accumulator++;
        if (s_lock_data.sec_accumulator >= 86400UL) { // 1 day elapsed
            s_lock_data.sec_accumulator = 0;
            day_lock_remain--;
            s_lock_data.remain_days = day_lock_remain;
            if (day_lock_remain <= 0) {
                // Check if active injection cycle is currently running
                if (TimerValve1 > 0 || TimerValve2 > 0) {
                    s_lock_data.lock_state = LOCK_STAGE_SOFT_LOCK;
                } else {
                    s_lock_data.lock_state = LOCK_STAGE_HARD_LOCK;
                    device_locked = true;
                }
                day_lock_issue_challenge();
            }
            day_lock_save();
        } else if ((s_lock_data.sec_accumulator % 3600UL) == 0) {
            /* Siemens Industrial Determinism: Hourly NVS commit */
            bool valid = false;
            uint32_t ep = hal_rtc_get_epoch(&valid);
            if (valid) s_lock_data.last_known_epoch = ep;
            day_lock_save();
        }
    } else if (s_lock_data.lock_state == LOCK_STAGE_SOFT_LOCK) {
        // Soft lock: when both valves complete, transition to hard lock
        if (TimerValve1 == 0 && TimerValve2 == 0) {
            s_lock_data.lock_state = LOCK_STAGE_HARD_LOCK;
            device_locked = true;
            day_lock_save();
        }
    } else if (s_lock_data.lock_state >= LOCK_STAGE_HARD_LOCK) {
        device_locked = true;
    }

    if (device_locked && !s_lock_modal) {
        dev_lock_ui_show();
    }
}

/* --------------------------------------------------------------------------
 * Apple HIG Pure White Lock Screen UI
 * -------------------------------------------------------------------------- */
static void lock_help_close_cb(lv_event_t *e) {
    (void)e;
    if (s_lock_help_modal) {
        lv_obj_delete(s_lock_help_modal);
        s_lock_help_modal = NULL;
    }
}

static void lock_help_open_cb(lv_event_t *e) {
    (void)e;
    if (s_lock_help_modal) return;
    s_lock_help_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_lock_help_modal, 440, 260);
    lv_obj_center(s_lock_help_modal);
    lv_obj_set_style_bg_color(s_lock_help_modal, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_lock_help_modal, lv_color_hex(0xff007aff), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_lock_help_modal, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_lock_help_modal, 14, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_lock_help_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_lock_help_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_lock_help_modal);
    lv_obj_set_style_text_font(title, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff007aff), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_label_set_text(title, "راهنمای پشتیبانی و فعال سازی هافلر");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *msg = lv_label_create(s_lock_help_modal);
    lv_obj_set_width(msg, 400);
    lv_obj_set_style_text_font(msg, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_text_color(msg, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(msg, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_label_set_text(msg, "1. اعتبار کارکرد روزشمار دستگاه به پایان رسیده است.\n"
                           "2. کد بازیابی 4 رقمی مندرج در کادر آبی را یادداشت نمایید.\n"
                           "3. با واحد پشتیبانی تماس گرفته و کد بازگشایی را دریافت کنید.\n"
                           "سایت: haffler.com  |  تلفن پشتیبانی: 09131697476");
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *btn_close = lv_btn_create(s_lock_help_modal);
    lv_obj_set_size(btn_close, 120, 44);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0xff007aff), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_close, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_close, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_close, lock_help_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_obj_set_style_text_font(lbl_close, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_label_set_text(lbl_close, "بستن");
    lv_obj_center(lbl_close);
}

static void day_lock_btnm_draw_task_cb(lv_event_t *e) {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
    lv_draw_task_t *draw_task = (lv_draw_task_t *)lv_event_get_param(e);
    if (!draw_task || !draw_task->draw_dsc) return;
    if (((lv_draw_dsc_base_t *)draw_task->draw_dsc)->part != LV_PART_ITEMS) return;

    int32_t id = ((lv_draw_dsc_base_t *)draw_task->draw_dsc)->id1;
    const char *txt = lv_buttonmatrix_get_button_text(obj, id);
    if (!txt) return;

    bool is_selected = (s_selected_btn_idx == (uint8_t)id);
    bool is_pressed  = (lv_buttonmatrix_get_selected_button(obj) == (uint32_t)id) && (lv_obj_get_state(obj) & LV_STATE_PRESSED);

    lv_draw_fill_dsc_t *fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
    lv_draw_label_dsc_t *label_dsc = lv_draw_task_get_label_dsc(draw_task);

    bool is_bksp = (strcmp(txt, "DEL") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0);
    bool is_ok   = (strcmp(txt, "OK") == 0 || strcmp(txt, LV_SYMBOL_OK) == 0);

    if (is_bksp) {
        if (fill_dsc) {
            fill_dsc->color = is_pressed ? lv_color_hex(0xffef4444) : lv_color_hex(0xffdc2626);
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 8;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_white();
            label_dsc->font = &lv_font_montserrat_18;
        }
    } else if (is_ok) {
        if (fill_dsc) {
            fill_dsc->color = is_pressed ? lv_color_hex(0xff22c55e) : lv_color_hex(0xff16a34a);
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 8;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_white();
            label_dsc->font = &lv_font_montserrat_18;
        }
    } else {
        if (fill_dsc) {
            if (is_pressed) {
                fill_dsc->color = lv_color_hex(0xffe2e8f0);
            } else if (is_selected) {
                fill_dsc->color = lv_color_hex(0xffe0f2fe); // Apple HIG sky blue focus tint
            } else {
                fill_dsc->color = lv_color_hex(0xfff1f5f9);
            }
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 8;
        }
        if (label_dsc) {
            label_dsc->color = is_selected ? lv_color_hex(0xff0284c7) : lv_color_hex(0xff0f172a);
            label_dsc->font = &lv_font_montserrat_18;
        }
    }
}

static void update_lock_pin_display(void) {
    if (!s_lock_pin_lbl) return;
    if (s_pin_len == 0) {
        lv_label_set_text(s_lock_pin_lbl, "ورود رمز");
        lv_obj_set_style_text_font(s_lock_pin_lbl, ui_font_vazir_or(&ui_font_arial_regular17, 15), LV_PART_MAIN);
        lv_obj_set_style_text_color(s_lock_pin_lbl, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    } else {
        char bullets[32] = {0};
        uint8_t pos = 0;
        for (uint8_t i = 0; i < s_pin_len && pos < sizeof(bullets) - 2; i++) {
            bullets[pos++] = '*';
            if (i + 1 < s_pin_len) bullets[pos++] = ' ';
        }
        bullets[pos] = '\0';
        lv_label_set_text(s_lock_pin_lbl, bullets);
        lv_obj_set_style_text_font(s_lock_pin_lbl, &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_lock_pin_lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    }
    lv_obj_center(s_lock_pin_lbl);
}

static void lock_matrix_cb(lv_event_t *e) {
    uint32_t rem_sec = 0;
    if (day_lock_is_brute_locked(&rem_sec)) {
        if (s_lock_err_lbl) {
            char freeze_msg[64];
            snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است (%u ثانیه صبر کنید)", (unsigned)rem_sec);
            lv_label_set_text(s_lock_err_lbl, freeze_msg);
            lv_obj_clear_flag(s_lock_err_lbl, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(obj);
    const char *txt = lv_buttonmatrix_get_button_text(obj, btn_id);
    if (!txt) return;

    if (strcmp(txt, "OK") == 0) {
        if (s_pin_len > 0) {
            if (day_lock_try_unlock(s_entered_pin)) {
                dev_lock_ui_hide();
            } else {
                s_pin_len = 0;
                s_entered_pin[0] = '\0';
                update_lock_pin_display();
                if (s_lock_err_lbl) {
                    if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
                        lv_label_set_text(s_lock_err_lbl, "خطای دستکاری! سیستم مسدود است");
                    } else {
                        lv_label_set_text(s_lock_err_lbl, "رمز نامعتبر است! مجددا تلاش فرمایید");
                    }
                    lv_obj_clear_flag(s_lock_err_lbl, LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    } else if (strcmp(txt, "DEL") == 0 || strcmp(txt, "حذف") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        if (s_pin_len > 0) {
            s_pin_len--;
            s_entered_pin[s_pin_len] = '\0';
            update_lock_pin_display();
        }
    } else if (txt[0] >= '0' && txt[0] <= '9') {
        if (s_pin_len < 12) {
            s_entered_pin[s_pin_len++] = txt[0];
            s_entered_pin[s_pin_len] = '\0';
            update_lock_pin_display();
            if (s_lock_err_lbl) lv_obj_add_flag(s_lock_err_lbl, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void dev_lock_ui_show(void) {
    if (s_lock_modal) return;

    // 1. Semi-translucent Light Slate Canvas (Apple HIG Pure White Theme)
    s_lock_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_lock_modal, 480, 320);
    lv_obj_set_pos(s_lock_modal, 0, 0);
    lv_obj_set_style_bg_color(s_lock_modal, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_lock_modal, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_lock_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_lock_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_lock_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_lock_modal, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Central Pure White Floating Card
    s_lock_card = lv_obj_create(s_lock_modal);
    lv_obj_set_pos(s_lock_card, 8, 8);
    lv_obj_set_size(s_lock_card, 464, 304);
    lv_obj_set_style_bg_color(s_lock_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(s_lock_card, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_lock_card, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_lock_card, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_lock_card, 0, LV_PART_MAIN); // Strict zero-lag policy
    lv_obj_set_style_pad_all(s_lock_card, 8, LV_PART_MAIN);
    lv_obj_clear_flag(s_lock_card, LV_OBJ_FLAG_SCROLLABLE);

    // 3. Header Bar with Title and Help Button (Zero Tofu - clean Persian typography)
    s_lock_title = lv_label_create(s_lock_card);
    lv_obj_set_style_text_font(s_lock_title, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_lock_title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    if (s_lock_data.lock_state == LOCK_STAGE_TAMPER) {
        lv_label_set_text(s_lock_title, "خطای دستکاری ساعت سخت افزاری - سیستم مسدود");
        lv_obj_set_style_text_color(s_lock_title, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    } else {
        lv_label_set_text(s_lock_title, "روزشمار هافلر - دستگاه قفل است");
        lv_obj_set_style_text_color(s_lock_title, lv_color_hex(0xffef4444), LV_PART_MAIN);
    }
    lv_obj_align(s_lock_title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *btn_help = lv_btn_create(s_lock_card);
    lv_obj_set_size(btn_help, 40, 40);
    lv_obj_align(btn_help, LV_ALIGN_TOP_LEFT, 6, 2);
    lv_obj_set_style_bg_color(btn_help, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_help, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_help, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_help, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_help, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_help, lock_help_open_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_q = lv_label_create(btn_help);
    lv_label_set_text(lbl_q, "?");
    lv_obj_set_style_text_font(lbl_q, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_q, lv_color_hex(0xff007aff), LV_PART_MAIN);
    lv_obj_center(lbl_q);

    // 4. Challenge Readout Pill Badge (Left Side)
    lv_obj_t *chal_badge = lv_obj_create(s_lock_card);
    lv_obj_set_size(chal_badge, 178, 76);
    lv_obj_set_pos(chal_badge, 8, 44);
    lv_obj_set_style_bg_color(chal_badge, lv_color_hex(0xffeff6ff), LV_PART_MAIN);
    lv_obj_set_style_border_color(chal_badge, lv_color_hex(0xffbfdbfe), LV_PART_MAIN);
    lv_obj_set_style_border_width(chal_badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(chal_badge, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(chal_badge, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chal_badge, 4, LV_PART_MAIN);
    lv_obj_clear_flag(chal_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_c_title = lv_label_create(chal_badge);
    lv_label_set_text(lbl_c_title, "کد بازیابی دستگاه");
    lv_obj_set_style_text_font(lbl_c_title, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_c_title, lv_color_hex(0xff1e40af), LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_c_title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(lbl_c_title, LV_ALIGN_TOP_MID, 0, 2);

    s_lock_challenge_lbl = lv_label_create(chal_badge);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04u", (unsigned)dev_lock_challenge);
    lv_label_set_text(s_lock_challenge_lbl, buf);
    lv_obj_set_style_text_font(s_lock_challenge_lbl, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_lock_challenge_lbl, lv_color_hex(0xff007aff), LV_PART_MAIN);
    lv_obj_align(s_lock_challenge_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

    // 5. PIN Bullet Display Box
    lv_obj_t *pin_box = lv_obj_create(s_lock_card);
    lv_obj_set_size(pin_box, 178, 46);
    lv_obj_set_pos(pin_box, 8, 126);
    lv_obj_set_style_bg_color(pin_box, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(pin_box, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_box, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(pin_box, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(pin_box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pin_box, 2, LV_PART_MAIN);
    lv_obj_clear_flag(pin_box, LV_OBJ_FLAG_SCROLLABLE);

    s_lock_pin_lbl = lv_label_create(pin_box);
    lv_label_set_text(s_lock_pin_lbl, "ورود رمز");
    lv_obj_set_style_text_font(s_lock_pin_lbl, ui_font_vazir_or(&ui_font_arial_regular17, 15), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_lock_pin_lbl, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_lock_pin_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_center(s_lock_pin_lbl);

    // 6. Error Banner / Countdown
    s_lock_err_lbl = lv_label_create(s_lock_card);
    lv_obj_set_size(s_lock_err_lbl, 178, 34);
    lv_obj_set_pos(s_lock_err_lbl, 8, 174);
    lv_obj_set_style_text_font(s_lock_err_lbl, ui_font_vazir_or(&ui_font_arial_regular17, 12), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_lock_err_lbl, lv_color_hex(0xffef4444), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_lock_err_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_lock_err_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_label_set_long_mode(s_lock_err_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_lock_err_lbl, "");
    lv_obj_add_flag(s_lock_err_lbl, LV_OBJ_FLAG_HIDDEN);

    // 7. Instructions label (fills empty space between error and footer)
    lv_obj_t *lbl_inst = lv_label_create(s_lock_card);
    lv_obj_set_size(lbl_inst, 178, 42);
    lv_obj_set_pos(lbl_inst, 8, 210);
    lv_obj_set_style_text_font(lbl_inst, ui_font_vazir_or(&ui_font_arial_regular17, 11), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_inst, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_inst, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_inst, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(lbl_inst, 2, LV_PART_MAIN);
    lv_label_set_long_mode(lbl_inst, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl_inst, "برای دریافت رمز با پشتیبانی تماس بگیرید");

    // 8. Footer Contact Notice (haffler.com & 09131697476)
    lv_obj_t *lbl_contact = lv_label_create(s_lock_card);
    lv_obj_set_size(lbl_contact, 184, 18);
    lv_obj_set_pos(lbl_contact, 5, 256);
    lv_obj_set_style_text_font(lbl_contact, ui_font_vazir_or(&ui_font_arial_regular17, 11), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_contact, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_contact, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_contact, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_label_set_text(lbl_contact, "09131697476");

    lv_obj_t *lbl_site = lv_label_create(s_lock_card);
    lv_obj_set_size(lbl_site, 184, 16);
    lv_obj_set_pos(lbl_site, 5, 274);
    lv_obj_set_style_text_font(lbl_site, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_site, lv_color_hex(0xff475569), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_site, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_site, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_label_set_text(lbl_site, "haffler.com");

    // 8. Tactile 4x3 Light ButtonMatrix Keypad (Right Side)
    static const char *kmap[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "DEL", "0", "OK", ""
    };
    s_lock_btnm = lv_buttonmatrix_create(s_lock_card);
    lv_buttonmatrix_set_map(s_lock_btnm, kmap);
    lv_obj_set_pos(s_lock_btnm, 196, 44);
    lv_obj_set_size(s_lock_btnm, 256, 240);
    lv_obj_set_style_bg_opa(s_lock_btnm, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_lock_btnm, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_lock_btnm, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_lock_btnm, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_column(s_lock_btnm, 6, LV_PART_MAIN);

    // Keypad Tile Style (Light Gray on White)
    lv_obj_set_style_bg_color(s_lock_btnm, lv_color_hex(0xfff1f5f9), LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_lock_btnm, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_lock_btnm, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_lock_btnm, 10, LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_lock_btnm, &lv_font_montserrat_18, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_lock_btnm, lv_color_hex(0xff0f172a), LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(s_lock_btnm, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_lock_btnm, lv_color_hex(0xffe2e8f0), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_flag(s_lock_btnm, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(s_lock_btnm, day_lock_btnm_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(s_lock_btnm, lock_matrix_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_pin_len = 0;
    s_entered_pin[0] = '\0';
    s_selected_btn_idx = 0;
}

void dev_lock_ui_hide(void) {
    if (s_lock_help_modal) {
        lv_obj_delete(s_lock_help_modal);
        s_lock_help_modal = NULL;
    }
    if (s_lock_modal) {
        lv_obj_delete(s_lock_modal);
        s_lock_modal = NULL;
        s_lock_card = NULL;
        s_lock_title = NULL;
        s_lock_pin_lbl = NULL;
        s_lock_challenge_lbl = NULL;
        s_lock_err_lbl = NULL;
        s_lock_btnm = NULL;
    }
    s_pin_len = 0;
    s_entered_pin[0] = '\0';
}

bool dev_lock_ui_is_active(void) {
    return (s_lock_modal != NULL);
}

void day_lock_handle_panel_key(uint8_t key_code) {
    if (!s_lock_modal || !s_lock_btnm) return;

    if (s_lock_help_modal) {
        if (key_code == KEY_STOP || key_code == KEY_CANCEL || key_code == KEY_START || key_code == KEY_MODE) {
            lv_obj_delete(s_lock_help_modal);
            s_lock_help_modal = NULL;
            return;
        }
    }

    if (key_code == KEY_MODE) {
        s_selected_btn_idx = (s_selected_btn_idx + 1) % 12;
        lv_buttonmatrix_set_selected_button(s_lock_btnm, s_selected_btn_idx);
        lv_obj_invalidate(s_lock_btnm);
    } else if (key_code == KEY_START) {
        lv_buttonmatrix_set_selected_button(s_lock_btnm, s_selected_btn_idx);
        lv_obj_send_event(s_lock_btnm, LV_EVENT_VALUE_CHANGED, NULL);
    } else if (key_code == KEY_STOP || key_code == KEY_CANCEL) {
        // DEL is at index 9 (row 3, col 0)
        lv_buttonmatrix_set_selected_button(s_lock_btnm, 9);
        lv_obj_send_event(s_lock_btnm, LV_EVENT_VALUE_CHANGED, NULL);
        lv_buttonmatrix_set_selected_button(s_lock_btnm, s_selected_btn_idx);
        lv_obj_invalidate(s_lock_btnm);
    }
}
