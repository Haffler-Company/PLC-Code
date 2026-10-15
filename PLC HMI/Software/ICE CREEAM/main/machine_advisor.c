#include "machine_advisor.h"
#include "storage.h"
#include "fault_log.h"
#include "project_config.h"
#include "esp_log.h"
#include "esp_random.h"
#include "wifi.h"
#include "ui/ui_lang.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "MACHINE_ADVISOR";

/* پایش تکرار خطاها (Fault Recurrence Tracker) */
typedef struct {
    uint8_t  strikes;
    uint32_t window_remain_s;
} fault_recurrence_item_t;

#define MAX_TRACKED_FAULTS 24
static fault_recurrence_item_t s_fault_recurrence[MAX_TRACKED_FAULTS];

/* کلیدهای پایدار NVS */
#define KEY_OTA_DISMISS_CNT     "ota_dism_cnt"
#define KEY_OVL_STRIKES         "ovl_strikes"
#define KEY_OVL_CD_ACT          "ovl_cd_act"
#define KEY_OVL_LOCKED          "ovl_locked"
#define KEY_OVL_CHAL            "ovl_chal"
#define KEY_GRID_BROWNOUTS      "grid_brw_cnt"
#define KEY_HRS_COMP            "hrs_comp"
#define KEY_HRS_MOTOR           "hrs_mot"
#define KEY_HRS_SERVICE         "hrs_serv"
#define KEY_CONDENSER_WARN      "cond_warn"

/* وضعیت درونی هوشمندی ماشین */
static int32_t           s_ota_dismiss_cnt = 0;
static volatile uint32_t s_pulley_bypass_sec = 0;
static volatile bool     s_thermo_fallback_en = false;

/* هوشمندی تمیزی کندانسور */
static volatile bool     s_freeze_in_progress = false;
static volatile uint32_t s_freeze_cycle_sec = 0;
static volatile bool     s_condenser_clean_warn = false;

/* پایش ولتاژ شبکه و Brownout */
static uint32_t          s_brownout_count = 0;

/* حالت بهینه اکو در ساعات خلوتی */
static volatile uint32_t s_idle_dispense_sec = 0;
static volatile bool     s_eco_idle_active = false;

/* ردیاب ساعات کارکرد قطعات و سرویس */
static uint32_t          s_comp_total_sec = 0;
static uint32_t          s_motor_total_sec = 0;
static uint32_t          s_service_elapsed_sec = 0;
static uint32_t          s_tracker_flush_timer = 0;

static volatile overload_status_t s_ovl_status = OVERLOAD_STATUS_NORMAL;
static volatile uint8_t           s_ovl_strikes = 0;
static volatile uint32_t          s_cooldown_remain_sec = 0;
static volatile uint32_t          s_ovl_challenge = 0;
static volatile uint32_t          s_healthy_run_sec = 0;
static volatile uint32_t          s_ovl_window_remain_sec = 0;

static uint8_t calc_digit_sum(uint64_t v)
{
    uint8_t sum = 0;
    while (v > 0) {
        sum += (uint8_t)(v % 10ULL);
        v /= 10ULL;
    }
    return sum;
}

static uint32_t generate_5digit_challenge(void)
{
    uint32_t r = esp_random();
    return 10000U + (r % 90000U);
}

void machine_advisor_init(void)
{
    s_ota_dismiss_cnt = storage_get_i32(KEY_OTA_DISMISS_CNT, 0);

    /* بازخوانی هشدارهای هوشمند و ساعات کارکرد قطعات */
    s_condenser_clean_warn = storage_get_bool(KEY_CONDENSER_WARN, false);
    s_brownout_count = storage_get_u32(KEY_GRID_BROWNOUTS, 0);
    s_comp_total_sec = storage_get_u32(KEY_HRS_COMP, 0);
    s_motor_total_sec = storage_get_u32(KEY_HRS_MOTOR, 0);
    s_service_elapsed_sec = storage_get_u32(KEY_HRS_SERVICE, 0);
    s_idle_dispense_sec = 0;
    s_eco_idle_active = false;
    s_freeze_in_progress = false;
    s_freeze_cycle_sec = 0;

    /* دستور کارفرما: اگر دستگاه ریست شد، از اول بشماره (پاک‌سازی استرایک‌ها و قفل با ریست دستگاه) */
    s_ovl_strikes = 0;
    s_ovl_status = OVERLOAD_STATUS_NORMAL;
    s_cooldown_remain_sec = 0;
    s_ovl_challenge = 0;
    s_ovl_window_remain_sec = 0;
    s_healthy_run_sec = 0;
    storage_set_i32(KEY_OVL_STRIKES, 0);
    storage_set_bool(KEY_OVL_CD_ACT, false);
    storage_set_bool(KEY_OVL_LOCKED, false);
    storage_set_i32(KEY_OVL_CHAL, 0);

    for (int i = 0; i < MAX_TRACKED_FAULTS; i++) {
        s_fault_recurrence[i].strikes = 0;
        s_fault_recurrence[i].window_remain_s = 0;
    }

    ESP_LOGI(TAG, "Machine Advisor ready: Overload strikes reset, smart tracking initialized (Comp: %lu h, Mot: %lu h, Serv: %lu h, Brownouts: %lu).",
             (unsigned long)(s_comp_total_sec / 3600), (unsigned long)(s_motor_total_sec / 3600),
             (unsigned long)(s_service_elapsed_sec / 3600), (unsigned long)s_brownout_count);
}

void machine_advisor_tick_1s(void)
{
    /* ۱. شمارش معکوس بای‌پس موقت سنسور پولی */
    if (s_pulley_bypass_sec > 0) {
        s_pulley_bypass_sec--;
        if (s_pulley_bypass_sec == 0) {
            ESP_LOGI(TAG, "Temporary pulley sensor bypass expired. Monitoring re-enabled.");
        }
    }

    /* ۱-ب. پایش پنجره زمانی تکرار خطاها (۱ ساعت) */
    for (int i = 0; i < MAX_TRACKED_FAULTS; i++) {
        if (s_fault_recurrence[i].window_remain_s > 0) {
            s_fault_recurrence[i].window_remain_s--;
            if (s_fault_recurrence[i].window_remain_s == 0) {
                if (s_fault_recurrence[i].strikes > 0) {
                    ESP_LOGI(TAG, "Fault code %d recurrence window expired. Reset strikes from %u to 0.",
                             i, (unsigned)s_fault_recurrence[i].strikes);
                    s_fault_recurrence[i].strikes = 0;
                }
            }
        }
    }

    /* ۲. شمارش معکوس خنک‌سازی سیم‌پیچ موتور/کمپرسور (۳۰ دقیقه) */
    if (s_ovl_status == OVERLOAD_STATUS_COOLDOWN) {
        if (s_cooldown_remain_sec > 0) {
            s_cooldown_remain_sec--;
        }
        if (s_cooldown_remain_sec == 0) {
            s_ovl_status = OVERLOAD_STATUS_WARNED;
            storage_set_bool(KEY_OVL_CD_ACT, false);
            ESP_LOGI(TAG, "Thermal cooldown successfully completed. Machine ready to operate.");
        }
    }

    /* ۳. انقضای پنجره زمانی ۲ ساعته اورلود:
     * اگر در محدوده ۲ ساعت به ۳ تریپ نرسید، استرایک‌ها صفر شده و از اول می‌شمارد */
    if (s_ovl_status != OVERLOAD_STATUS_HARD_LOCK) {
        if (s_ovl_window_remain_sec > 0) {
            s_ovl_window_remain_sec--;
            if (s_ovl_window_remain_sec == 0) {
                if (s_ovl_strikes > 0) {
                    ESP_LOGI(TAG, "Overload 2-hour window elapsed without 3 strikes. Resetting strikes from %d to 0.",
                             (int)s_ovl_strikes);
                    s_ovl_strikes = 0;
                    s_ovl_status = OVERLOAD_STATUS_NORMAL;
                    storage_set_i32(KEY_OVL_STRIKES, 0);
                    storage_set_bool(KEY_OVL_CD_ACT, false);
                }
            }
        }
    }

    /* ۴. پاک‌سازی تدریجی در صورت کارکرد سالم و بدون تریپ به مدت ۶۰ دقیقه */
    if ((make_start || paddle_start) && s_ovl_status < OVERLOAD_STATUS_COOLDOWN) {
        s_healthy_run_sec++;
        if (s_healthy_run_sec >= 3600) {
            s_healthy_run_sec = 0;
            if (s_ovl_strikes > 0) {
                s_ovl_strikes--;
                storage_set_i32(KEY_OVL_STRIKES, s_ovl_strikes);
                if (s_ovl_strikes == 0) {
                    s_ovl_status = OVERLOAD_STATUS_NORMAL;
                    s_ovl_window_remain_sec = 0;
                }
                ESP_LOGI(TAG, "Healthy 60-min run achieved. Decremented overload strikes to %d", (int)s_ovl_strikes);
            }
        }
    } else {
        s_healthy_run_sec = 0;
    }

    /* ۵. پایش زمان انجماد سیلندر جهت تشخیص غبارگرفتگی رادیاتور کندانسور */
    if (s_freeze_in_progress) {
        s_freeze_cycle_sec++;
        if (s_freeze_cycle_sec >= CONDENSER_MAX_FREEZE_TIME_S && !s_condenser_clean_warn) {
            s_condenser_clean_warn = true;
            storage_set_bool(KEY_CONDENSER_WARN, true);
            ESP_LOGW(TAG, "ADVISORY: Freeze cycle exceeded %d s without reaching hardness. Condenser cleaning recommended.", CONDENSER_MAX_FREEZE_TIME_S);
        }
    }

    /* ۶. پایش توزیع و فعال‌سازی اکو در ساعات خلوتی */
    s_idle_dispense_sec++;
    if (s_idle_dispense_sec >= ECO_IDLE_TRIGGER_S && !s_eco_idle_active) {
        s_eco_idle_active = true;
        ESP_LOGI(TAG, "ADVISORY: Machine idle for %u s. Dynamic Eco-Idle activated.", (unsigned)s_idle_dispense_sec);
    }

    /* ۷. ذخیره‌سازی دوره‌ای ساعات کارکرد قطعات (هر ۱۰ دقیقه یکبار) */
    s_tracker_flush_timer++;
    if (s_tracker_flush_timer >= 600) {
        s_tracker_flush_timer = 0;
        storage_set_u32(KEY_HRS_COMP, s_comp_total_sec);
        storage_set_u32(KEY_HRS_MOTOR, s_motor_total_sec);
        storage_set_u32(KEY_HRS_SERVICE, s_service_elapsed_sec);
    }
}

/* -------------------------------------------------------------------------- */
/* ۱. هوشمندی به‌روزرسانی OTA                                                 */
/* -------------------------------------------------------------------------- */
bool machine_advisor_is_ota_snoozed(void)
{
    return (s_ota_dismiss_cnt >= OTA_MAX_BOOT_DISMISS_COUNT);
}

int32_t machine_advisor_get_ota_dismiss_count(void)
{
    return s_ota_dismiss_cnt;
}

void machine_advisor_record_ota_dismiss(void)
{
    s_ota_dismiss_cnt++;
    storage_set_i32(KEY_OTA_DISMISS_CNT, s_ota_dismiss_cnt);
    ESP_LOGI(TAG, "OTA boot prompt dismissed (count=%ld). Snoozed=%d",
             (long)s_ota_dismiss_cnt, (int)machine_advisor_is_ota_snoozed());
}

void machine_advisor_reset_ota_dismiss(void)
{
    s_ota_dismiss_cnt = 0;
    storage_set_i32(KEY_OTA_DISMISS_CNT, 0);
    ESP_LOGI(TAG, "OTA dismiss counter reset to 0.");
}

/* -------------------------------------------------------------------------- */
/* ۲. بای‌پس موقت سنسور القایی دور پولی                                       */
/* -------------------------------------------------------------------------- */
bool machine_advisor_is_pulley_bypassed(void)
{
    return (s_pulley_bypass_sec > 0);
}

uint32_t machine_advisor_get_pulley_bypass_remain_s(void)
{
    return s_pulley_bypass_sec;
}

void machine_advisor_enable_pulley_bypass(uint32_t seconds)
{
    if (seconds == 0) seconds = PULLEY_BYPASS_DEFAULT_SEC;
    s_pulley_bypass_sec = seconds;
    fault_log_record(FAULT_INDUCTIVE_STALL, (int16_t)(seconds / 60));
    ESP_LOGW(TAG, "Operator temporarily bypassed pulley stall sensor for %lu seconds (%lu h)",
             (unsigned long)seconds, (unsigned long)(seconds / 3600));
}

void machine_advisor_disable_pulley_bypass(void)
{
    s_pulley_bypass_sec = 0;
    ESP_LOGI(TAG, "Operator manually restored pulley stall sensor monitoring.");
}

/* -------------------------------------------------------------------------- */
/* ۳. حالت اضطراری برودت سنسور دمای مخزن T1                                 */
/* -------------------------------------------------------------------------- */
bool machine_advisor_is_thermo_fallback_active(void)
{
    return s_thermo_fallback_en;
}

void machine_advisor_set_thermo_fallback(bool enable)
{
    s_thermo_fallback_en = enable;
    ESP_LOGW(TAG, "T1 safe thermo duty-cycle fallback %s by operator.", enable ? "ENABLED" : "DISABLED");
}

/* -------------------------------------------------------------------------- */
/* ۴. سیستم ضد سواستفاده و حفاظت حرارتی اورلود                               */
/* -------------------------------------------------------------------------- */
overload_status_t machine_advisor_get_overload_status(void)
{
    return s_ovl_status;
}

uint8_t machine_advisor_get_overload_strikes(void)
{
    return s_ovl_strikes;
}

uint32_t machine_advisor_get_cooldown_remain_s(void)
{
    return s_cooldown_remain_sec;
}

uint32_t machine_advisor_get_window_remain_s(void)
{
    return s_ovl_window_remain_sec;
}

bool machine_advisor_is_cooldown_active(void)
{
    return (s_ovl_status == OVERLOAD_STATUS_COOLDOWN && s_cooldown_remain_sec > 0);
}

bool machine_advisor_is_overload_locked(void)
{
    return (s_ovl_status == OVERLOAD_STATUS_HARD_LOCK);
}

uint32_t machine_advisor_get_overload_challenge(void)
{
    return s_ovl_challenge;
}

void machine_advisor_record_overload(uint8_t fault_code)
{
    (void)fault_code;

    /* پایش پنجره زمانی ۲ ساعته:
     * اگر از زمان خطای قبلی بیش از ۲ ساعت گذشته باشد یا تایمر صفر شده باشد،
     * شمارش از ۱ شروع می‌شود. */
    if (s_ovl_window_remain_sec == 0 || s_ovl_strikes == 0) {
        s_ovl_strikes = 1;
        ESP_LOGI(TAG, "Overload Strike 1: Started new 2-hour monitoring window.");
    } else {
        s_ovl_strikes++;
        ESP_LOGW(TAG, "Overload Strike %d within 2-hour window!", (int)s_ovl_strikes);
    }

    /* شارژ مجدد پنجره زمانی ۲ ساعته (۷۲۰۰ ثانیه) */
    s_ovl_window_remain_sec = OVERLOAD_STRIKE_WINDOW_S;
    storage_set_i32(KEY_OVL_STRIKES, (int32_t)s_ovl_strikes);

    if (s_ovl_strikes == 1) {
        s_ovl_status = OVERLOAD_STATUS_WARNED;
        ESP_LOGW(TAG, "Overload Strike 1: Standard reset permitted.");
    } else if (s_ovl_strikes == 2) {
        s_ovl_status = OVERLOAD_STATUS_COOLDOWN;
        s_cooldown_remain_sec = OVERLOAD_COOLDOWN_DURATION_S;
        storage_set_bool(KEY_OVL_CD_ACT, true);
        ESP_LOGW(TAG, "Overload Strike 2: 30-minute thermal cooldown engaged.");
    } else {
        s_ovl_status = OVERLOAD_STATUS_HARD_LOCK;
        storage_set_bool(KEY_OVL_LOCKED, true);
        s_ovl_challenge = generate_5digit_challenge();
        storage_set_i32(KEY_OVL_CHAL, (int32_t)s_ovl_challenge);
        ESP_LOGE(TAG, "Overload Strike 3 (in 2h window): SYSTEM HARD-LOCKED! Challenge=%lu", (unsigned long)s_ovl_challenge);
    }
}

void machine_advisor_on_healthy_run_60m(void)
{
    if (s_ovl_strikes > 0 && s_ovl_status < OVERLOAD_STATUS_COOLDOWN) {
        s_ovl_strikes--;
        storage_set_i32(KEY_OVL_STRIKES, s_ovl_strikes);
        if (s_ovl_strikes == 0) {
            s_ovl_status = OVERLOAD_STATUS_NORMAL;
            s_ovl_window_remain_sec = 0;
        }
        ESP_LOGI(TAG, "Healthy run milestone: Overload strikes reduced to %d", (int)s_ovl_strikes);
    }

    /* پاک‌سازی تدریجی استرایک‌های تکرار خطا در صورت کارکرد ۶۰ دقیقه کاملاً سالم */
    for (int i = 0; i < MAX_TRACKED_FAULTS; i++) {
        if (s_fault_recurrence[i].strikes > 0) {
            s_fault_recurrence[i].strikes--;
            if (s_fault_recurrence[i].strikes == 0) {
                s_fault_recurrence[i].window_remain_s = 0;
            }
        }
    }
}

bool machine_advisor_verify_unlock_code(const char *code_str)
{
    if (!code_str || strlen(code_str) == 0) return false;

    /* مستر کد مجاز کارخانه آستیاج (از project_config.h) */
    if (strcmp(code_str, MASTER_ADMIN_PIN) == 0) {
        machine_advisor_force_reset_overload();
        ESP_LOGI(TAG, "Overload unlocked via Master Admin PIN.");
        return true;
    }

    if (s_ovl_challenge == 0) return false;

    uint64_t base = (uint64_t)s_ovl_challenge * 2ULL;
    uint8_t cs = calc_digit_sum(base) % 10U;
    char expected[16];
    snprintf(expected, sizeof(expected), "%llu%u", (unsigned long long)base, (unsigned)cs);

    if (strcmp(code_str, expected) == 0) {
        machine_advisor_force_reset_overload();
        ESP_LOGI(TAG, "Overload unlocked via valid Challenge-Response code.");
        return true;
    }

    return false;
}

void machine_advisor_force_reset_overload(void)
{
    s_ovl_strikes = 0;
    s_ovl_status = OVERLOAD_STATUS_NORMAL;
    s_cooldown_remain_sec = 0;
    s_ovl_challenge = 0;
    s_ovl_window_remain_sec = 0;
    s_healthy_run_sec = 0;
    storage_set_i32(KEY_OVL_STRIKES, 0);
    storage_set_bool(KEY_OVL_CD_ACT, false);
    storage_set_bool(KEY_OVL_LOCKED, false);
    storage_set_i32(KEY_OVL_CHAL, 0);
    ESP_LOGI(TAG, "Overload system completely reset to NORMAL.");
}

/* -------------------------------------------------------------------------- */
/* ۵. دستیار هوشمند تمیزی کندانسور (Smart Condenser Clean Advisor)           */
/* -------------------------------------------------------------------------- */
bool machine_advisor_is_condenser_clean_warn(void)
{
    return s_condenser_clean_warn;
}

void machine_advisor_reset_condenser_clean_warn(void)
{
    s_condenser_clean_warn = false;
    s_freeze_cycle_sec = 0;
    storage_set_bool(KEY_CONDENSER_WARN, false);
    ESP_LOGI(TAG, "Condenser clean warning cleared by operator.");
}

void machine_advisor_notify_freeze_start(void)
{
    s_freeze_in_progress = true;
    s_freeze_cycle_sec = 0;
}

void machine_advisor_notify_freeze_done(void)
{
    s_freeze_in_progress = false;
    s_freeze_cycle_sec = 0;
}

/* -------------------------------------------------------------------------- */
/* ۶. پایش پایداری ولتاژ شبکه و ثبت افت ولتاژ (Mains Grid Stability Logger)    */
/* -------------------------------------------------------------------------- */
uint32_t machine_advisor_get_brownout_count(void)
{
    return s_brownout_count;
}

void machine_advisor_record_brownout(void)
{
    s_brownout_count = storage_get_u32(KEY_GRID_BROWNOUTS, 0) + 1;
    storage_set_u32(KEY_GRID_BROWNOUTS, s_brownout_count);
    ESP_LOGW(TAG, "Mains brownout event recorded. Total events: %lu", (unsigned long)s_brownout_count);
}

/* -------------------------------------------------------------------------- */
/* ۷. حالت هوشمند اکو در ساعات خلوتی (Dynamic Eco-Idle Mode)                 */
/* -------------------------------------------------------------------------- */
bool machine_advisor_is_eco_idle(void)
{
    return s_eco_idle_active;
}

uint32_t machine_advisor_get_idle_dispense_s(void)
{
    return s_idle_dispense_sec;
}

void machine_advisor_notify_dispense(void)
{
    s_idle_dispense_sec = 0;
    s_eco_idle_active = false;
}

/* -------------------------------------------------------------------------- */
/* ۸. ردگیر ساعات کارکرد و سرویس قطعات مصرفی (Consumables Lifetime Tracker)  */
/* -------------------------------------------------------------------------- */
uint32_t machine_advisor_get_comp_hours(void)
{
    return s_comp_total_sec / 3600;
}

uint32_t machine_advisor_get_motor_hours(void)
{
    return s_motor_total_sec / 3600;
}

uint32_t machine_advisor_get_service_remain_hours(void)
{
    uint32_t elapsed_h = s_service_elapsed_sec / 3600;
    if (elapsed_h >= SERVICE_INTERVAL_MOTOR_HOURS) {
        return 0;
    }
    return SERVICE_INTERVAL_MOTOR_HOURS - elapsed_h;
}

void machine_advisor_reset_service_interval(void)
{
    s_service_elapsed_sec = 0;
    storage_set_u32(KEY_HRS_SERVICE, 0);
    ESP_LOGI(TAG, "Consumables lubrication & service interval reset to %d hours.", SERVICE_INTERVAL_MOTOR_HOURS);
}

void machine_advisor_track_actuator_seconds(bool comp_active, bool motor_active)
{
    if (comp_active) {
        s_comp_total_sec++;
    }
    if (motor_active) {
        s_motor_total_sec++;
        s_service_elapsed_sec++;
    }
}

/* -------------------------------------------------------------------------- */
/* ۹. موتور هوشمند پایش تکرار خطا و هدایت تشخیصی (Fault Recurrence & Smart Escalation) */
/* -------------------------------------------------------------------------- */
void machine_advisor_record_fault(uint8_t fault_code)
{
    if (fault_code >= MAX_TRACKED_FAULTS) return;

    if (s_fault_recurrence[fault_code].window_remain_s == 0 || s_fault_recurrence[fault_code].strikes == 0) {
        s_fault_recurrence[fault_code].strikes = 1;
    } else {
        if (s_fault_recurrence[fault_code].strikes < 255) {
            s_fault_recurrence[fault_code].strikes++;
        }
    }
    s_fault_recurrence[fault_code].window_remain_s = FAULT_RECURRENCE_WINDOW_S;

    ESP_LOGW(TAG, "Fault code %u recorded: strike %u in 1h window (escalated=%d)",
             (unsigned)fault_code, (unsigned)s_fault_recurrence[fault_code].strikes,
             (int)(s_fault_recurrence[fault_code].strikes >= FAULT_RECURRENCE_THRESHOLD));

    if (fault_code == FAULT_MOTOR_OVERLOAD || fault_code == FAULT_COMP_OVERLOAD) {
        machine_advisor_record_overload(fault_code);
    }
}

uint8_t machine_advisor_get_fault_strikes(uint8_t fault_code)
{
    if (fault_code >= MAX_TRACKED_FAULTS) return 0;
    return s_fault_recurrence[fault_code].strikes;
}

bool machine_advisor_is_fault_escalated(uint8_t fault_code)
{
    if (fault_code >= MAX_TRACKED_FAULTS) return false;
    return (s_fault_recurrence[fault_code].strikes >= FAULT_RECURRENCE_THRESHOLD);
}

void machine_advisor_clear_fault_strikes(uint8_t fault_code)
{
    if (fault_code >= MAX_TRACKED_FAULTS) return;
    s_fault_recurrence[fault_code].strikes = 0;
    s_fault_recurrence[fault_code].window_remain_s = 0;
}

bool machine_advisor_get_escalated_advice(uint8_t fault_code,
                                          const char **out_title,
                                          const char **out_cause,
                                          char *sol_buf, size_t sol_buf_size)
{
    if (fault_code >= MAX_TRACKED_FAULTS || !out_title || !out_cause || !sol_buf || sol_buf_size == 0) {
        return false;
    }
    if (s_fault_recurrence[fault_code].strikes < FAULT_RECURRENCE_THRESHOLD) {
        return false;
    }

    uint8_t strikes = s_fault_recurrence[fault_code].strikes;
    ui_lang_t lang = ui_lang_get();
    bool is_online = wifi_is_connected();

    static char s_esc_title[128];
    static char s_esc_cause[384];

    if (lang == UI_LANG_EN) {
        const char *name = "System Protection";
        const char *chk = "";
        switch (fault_code) {
            case FAULT_INDUCTIVE_STALL:
                name = "Pulley Stall (E01)";
                chk = "1. Cut main power; inspect beater motor drive belt for slip or breakage.\n"
                      "2. High Viscosity: If cylinder is frozen solid, use Wash mode to soften.\n"
                      "3. Verify inductive sensor gap to pulley (must be 2-3 mm).\n"
                      "4. Emergency Bypass: Tap [4h Bypass] below to keep selling during rush hours.\n";
                break;
            case FAULT_MOTOR_OVERLOAD:
                name = "Motor Overload (E02)";
                chk = "1. Slightly loosen rear clutch microswitch screw to reduce mechanical beater load.\n"
                      "2. Refrigeration Setting: Increase target freeze temp by 1-2C in F3 tab.\n"
                      "3. Check motor starting capacitor and verify smooth shaft rotation.\n"
                      "4. Allow 15-30 minutes for motor thermal winding cooldown.\n";
                break;
            case FAULT_COMP_OVERLOAD:
                name = "Compressor Overload (E03)";
                chk = "1. Clean condenser fins on rear radiator with compressed air or brush.\n"
                      "2. Keep at least 30 cm clearance between rear wall and machine for airflow.\n"
                      "3. Check mains supply voltage (drops below 205 VAC trip compressor thermal relay).\n"
                      "4. Wait 3 minutes for refrigerant gas pressures to equalize before retry.\n";
                break;
            case FAULT_THERMO_TOP_DISCONNECT:
            case FAULT_MAX6675_TOP_MISSING:
                name = "Hopper Sensor T1 (E04)";
                chk = "1. Check external Terminal T1 wiring and common M1 supply/ground.\n"
                      "2. Inspect Hopper sensor probe and cable for breaks or damage.\n"
                      "3. Emergency Mode: Tap [Safe Fallback Cooling] below to preserve milk mix.\n";
                break;
            case FAULT_THERMO_CYL_DISCONNECT:
            case FAULT_MAX6675_CYL_MISSING:
                name = "Cylinder Sensor T2 (E05)";
                chk = "1. Check external Terminal T2 wiring and common M1 supply/ground.\n"
                      "2. Ensure Cylinder probe cable is not pinched or damaged, and firmly seated.\n";
                break;
            case FAULT_STUCK_LEVER:
                name = "Stuck Dispense Lever (E08)";
                chk = "1. Move dispense lever all the way up; ensure it is not resting half-open.\n"
                      "2. Lubricate front door plunger and food-grade O-rings for smooth return.\n"
                      "3. Inspect lever return spring to ensure it is not dislodged or broken.\n";
                break;
            case FAULT_STUCK_REAR:
                name = "Engaged Rear Microswitch (E09)";
                chk = "1. Loosen rear microswitch adjustment screw until contacts open before start.\n"
                      "2. Ensure rear mechanical clutch moves smoothly under spring tension.\n"
                      "3. Verify free rotation of beater shaft by hand when machine is unpowered.\n";
                break;
            case FAULT_FREEZE_TIMEOUT:
                name = "Freeze Timeout (E10)";
                chk = "1. Check liquid mix level in hopper; empty tank prevents reaching hardness.\n"
                      "2. Verify refrigerant gas pressure and compressor cooling efficiency.\n"
                      "3. If cylinder is deeply frozen, run Wash mode for 5 min to thaw.\n";
                break;
            case FAULT_BROWNOUT:
                name = "Mains Voltage Drop (E12)";
                chk = "1. Measure incoming AC mains voltage (must remain above 205 VAC under load).\n"
                      "2. Use dedicated power line with at least 4mm wire; avoid extension cords.\n"
                      "3. Install a certified industrial voltage stabilizer.\n";
                break;
            default:
                chk = "1. Inspect power wiring, control relays, and sensor terminal connectors.\n"
                      "2. Turn off main breaker for 30 seconds, then reboot.\n";
                break;
        }

        snprintf(s_esc_title, sizeof(s_esc_title), "Fault E%02u: %s — (Repeated %u times)",
                 (unsigned)fault_code, name, (unsigned)strikes);
        snprintf(s_esc_cause, sizeof(s_esc_cause),
                 "This fault recurred %u times after pressing [Retry].\nRoot cause: Physical condition remains unresolved. Immediate inspection required to protect machine components.",
                 (unsigned)strikes);

        const char *net_status = is_online ?
            "\n🌐 Network: Machine is online. Check F4 tab for available firmware updates.\n" :
            "\n🌐 Network: Machine is offline. Connect Wi-Fi in F4 tab for automatic updates & bug fixes.\n";

        snprintf(sol_buf, sol_buf_size,
                 "📌 Urgent Diagnostic Checklist:\n%s"
                 "%s"
                 "☎️ ASTIAJ Factory Support:\n  • Tech Support: +989102367062\n  • Service Center: +989131697476\n  • Website: ASTIAJ.ir",
                 chk, net_status);

    } else if (lang == UI_LANG_TR) {
        const char *name = "Sistem Koruma";
        const char *chk = "";
        switch (fault_code) {
            case FAULT_INDUCTIVE_STALL:
                name = "Kasnak Donus Durmasi (E01)";
                chk = "1. Ana gucu kesin; karistirici motor kayisini kontrol edin (kayma veya kopma).\n"
                      "2. Silindir donmussa, dondurmayi yumusatmak icin Yikama modunu acin.\n"
                      "3. Enduktif sensor mesafesini (2-3 mm) kontrol edin.\n"
                      "4. Acil Baypas: Yogun saatlerde satisa devam etmek icin [4s Baypas] butonuna basin.\n";
                break;
            case FAULT_MOTOR_OVERLOAD:
                name = "Motor Asiri Yuk (E02)";
                chk = "1. Mekanik yuku azaltmak icin arka mikrosalter ayar vidasini hafifce gevsetin.\n"
                      "2. Sogutma Ayari: F3 sekmesinden hedef dondurma derecesini 1-2C yukseltin.\n"
                      "3. Motor kalkis kondansatorunu ve rulman donusunu kontrol edin.\n"
                      "4. Motor sargilarinin sogumasi icin 15-30 dakika bekleyin.\n";
                break;
            case FAULT_COMP_OVERLOAD:
                name = "Kompresor Asiri Yuk (E03)";
                chk = "1. Arka kondenser radyatör peteklerini basincli hava veya firca ile temizleyin.\n"
                      "2. Hava akisi icin makinenin arkasinda en az 30 cm bosluk birakin.\n"
                      "3. Sebeke voltajini kontrol edin (205V altina dususler kompresoru durdurur).\n"
                      "4. Gaz basincinin dengelenmesi icin 3 dakika bekleyip calistirin.\n";
                break;
            case FAULT_THERMO_TOP_DISCONNECT:
            case FAULT_MAX6675_TOP_MISSING:
                name = "Sut Tanki Sensoru T1 (E04)";
                chk = "1. Harici Klemens T1 ve ortak M1 sensor besleme baglantilarini kontrol edin.\n"
                      "2. Sut tanki sensor probunun saglam ve takili oldugundan emin olun.\n"
                      "3. Acil Mod: Sutun bozulmasini onlemek icin [Zamanli Guvenli Sogutma] butonuna basin.\n";
                break;
            case FAULT_THERMO_CYL_DISCONNECT:
            case FAULT_MAX6675_CYL_MISSING:
                name = "Silindir Sensoru T2 (E05)";
                chk = "1. Harici Klemens T2 ve ortak M1 sensor besleme baglantilarini kontrol edin.\n"
                      "2. Silindir probu ve kablosunda kopukluk veya ezilme olmadigini dogrulayin.\n";
                break;
            case FAULT_STUCK_LEVER:
                name = "Dondurma Kolu Takili (E08)";
                chk = "1. Dondurma kolunu tamamen yukari kaldirin; yarim acik kalmadigindan emin olun.\n"
                      "2. On kapi pistonunu ve gida tipi O-ringleri yaglayin.\n"
                      "3. Kol geri donus yayinin saglamligini kontrol edin.\n";
                break;
            case FAULT_STUCK_REAR:
                name = "Arka Mikrosalter Basili (E09)";
                chk = "1. Baslamadan once arka salter ayar vidasini gevseterek platini serbest birakin.\n"
                      "2. Mekanik debriyajin yay gucuyle rahatca hareket ettigini dogrulayin.\n"
                      "3. Makine kapaliyken karistirici milini elle cevirerek donusunu test edin.\n";
                break;
            case FAULT_FREEZE_TIMEOUT:
                name = "Dondurma Zaman Asimi (E10)";
                chk = "1. Sut haznesini kontrol edin; sutun bitmesi sertlige ulasilmasini engeller.\n"
                      "2. Gaz basincini ve kompresor verimini kontrol edin.\n"
                      "3. Asiri donma varsa, silindiri cozdurmek icin 5 dakika Yikama yapin.\n";
                break;
            case FAULT_BROWNOUT:
                name = "Dusuk Voltaj (E12)";
                chk = "1. Sebeke voltajini olcun (yuk altinda 205 VAC altina inmemelidir).\n"
                      "2. En az 4 mm bakir kablo kullanin ve ortak prizlerden kacinin.\n"
                      "3. Standart bir regulator (regulator) kullanilmasi onerilir.\n";
                break;
            default:
                chk = "1. Kablo ve soket baglantilarini kontrol edin.\n"
                      "2. Cihazi 30 saniye kapatip tekrar acin.\n";
                break;
        }

        snprintf(s_esc_title, sizeof(s_esc_title), "Hata E%02u: %s — (%u Kez Tekrarlandi)",
                 (unsigned)fault_code, name, (unsigned)strikes);
        snprintf(s_esc_cause, sizeof(s_esc_cause),
                 "Bu hata [Yeniden Dene] sonrasinda %u kez tekrarlandi.\nKok neden: Fiziksel sorun cozulmedi. Bilesenleri korumak icin acil mudahale gereklidir.",
                 (unsigned)strikes);

        const char *net_status = is_online ?
            "\n🌐 Ag Durumu: Cihaz cevrimici. F4 sekmesinden guncellemeleri kontrol edebilirsiniz.\n" :
            "\n🌐 Ag Durumu: Cihaz cevrimdisi. Otomatik yazilim duzeltmeleri icin F4'ten Wi-Fi baglayin.\n";

        snprintf(sol_buf, sol_buf_size,
                 "📌 Hizli Ariza Tespit Kontrol Listesi:\n%s"
                 "%s"
                 "☎️ ASTIAJ Fabrika Destek:\n  • Teknik Destek: +989102367062\n  • Merkez Servis: +989131697476\n  • Web: ASTIAJ.ir",
                 chk, net_status);

    } else {
        /* UI_LANG_FA */
        const char *name = "حفاظت سیستم";
        const char *chk = "";
        switch (fault_code) {
            case FAULT_INDUCTIVE_STALL:
                name = "توقف گردش پولی (E01)";
                chk = "۱. برق اصلی را قطع کرده و تسمه موتور را بررسی کنید (شل بودن یا پارگی تسمه).\n"
                      "۲. اگر بستنی در سیلندر منجمد و قفل شده، دکمه شستشو را بزنید تا نرم شود.\n"
                      "۳. فاصله سنسور القایی تا آهنربای پولی را ۲ تا ۳ میلی‌متر تنظیم نمایید.\n"
                      "۴. راهکار اضطراری: در ساعات شلوغی با دکمه [بای‌پس ۴ ساعته] بدون سنسور کار کنید.\n";
                break;
            case FAULT_MOTOR_OVERLOAD:
                name = "اضافه‌بار موتور پارو (E02)";
                chk = "۱. پیچ رگلاژ میکروسوئیچ عقب را کمی شل کنید تا فشار گیربکس آزاد گردد.\n"
                      "۲. دمای انجماد را در منوی F3 یک یا دو درجه بالاتر ببرید تا بستنی روان‌تر شود.\n"
                      "۳. خازن راه‌انداز موتور و سلامت بلبرینگ‌های شفت را کنترل فرمایید.\n"
                      "۴. سیم‌پیچ موتور داغ است؛ ۱۵ تا ۳۰ دقیقه مهلت خنک‌سازی بدهید.\n";
                break;
            case FAULT_COMP_OVERLOAD:
                name = "اضافه‌بار کمپرسور (E03)";
                chk = "۱. رادیاتور کندانسور پشت دستگاه را با فشار باد یا برس کاملاً غبارزدایی کنید.\n"
                      "۲. فاصله پشت دستگاه تا دیوار حداقل ۳۰ سانتی‌متر باشد تا گردش هوا برقرار گردد.\n"
                      "۳. ولتاژ برق ورودی مغازه را بسنجید (افت زیر ۲۰۵ ولت کمپرسور را تریپ می‌دهد).\n"
                      "۴. ۳ دقیقه صبر فرمایید تا فشار گاز مبرد متعادل شود، سپس استارت نمایید.\n";
                break;
            case FAULT_THERMO_TOP_DISCONNECT:
            case FAULT_MAX6675_TOP_MISSING:
                name = "سنسور دمای مخزن T1 (E04)";
                chk = "۱. سیم‌بندی ترمینال خارجی T1 و اتصال سیم تغذیه/منفی سنسور در ترمینال M1 را بررسی و محکم کنید.\n"
                      "۲. از سلامت کابل سنسور مخزن بالا و عدم قطعی، کشیدگی یا آسیب‌دیدگی پراب مطمئن شوید.\n"
                      "۳. راهکار اضطراری: دکمه [سرمایش زمانی ایمن] را بزنید تا شیر ترش نشود.\n";
                break;
            case FAULT_THERMO_CYL_DISCONNECT:
            case FAULT_MAX6675_CYL_MISSING:
                name = "سنسور دمای سیلندر T2 (E05)";
                chk = "۱. سیم‌بندی ترمینال خارجی T2 و اتصال سیم تغذیه/منفی مشترک در ترمینال M1 را بازبینی فرمایید.\n"
                      "۲. از عدم اتصالی سیم به بدنه فلزی و سلامت پراب سنسور سیلندر انجماد مطمئن شوید.\n";
                break;
            case FAULT_STUCK_LEVER:
                name = "فشرده ماندن اهرم جلو (E08)";
                chk = "۱. اهرم خروج را بالا بدهید؛ مطمئن شوید دسته در وضعیت نیمه‌باز نمانده باشد.\n"
                      "۲. پیستون درب جلو و واشرها را با گریس خوراکی چرب کنید تا دسته روان بازگردد.\n"
                      "۳. فنر بازگرداننده اهرم را کنترل کنید که نشکسته یا خارج نشده باشد.\n";
                break;
            case FAULT_STUCK_REAR:
                name = "درگیر بودن کلاچ عقب (E09)";
                chk = "۱. پیش از استارت، پیچ تنظیم میکروسوئیچ عقب را شل کنید تا پلاتین آزاد شود.\n"
                      "۲. اهرم کلاچ مکانیکی پشت گیربکس باید به راحتی با نیروی فنر بازی کند.\n"
                      "۳. در حالت خاموش، از چرخش آزادانه شفت پارو با دست اطمینان یابید.\n";
                break;
            case FAULT_FREEZE_TIMEOUT:
                name = "تایم‌اوت انجماد سیلندر (E10)";
                chk = "۱. موجودی شیر در مخزن را بررسی نمایید؛ کمبود مایع مانع رسیدن به سفتی می‌شود.\n"
                      "۲. میزان شارژ گاز مبرد و رادیاتور خنک‌کننده کمپرسور را بازبینی فرمایید.\n"
                      "۳. اگر سیلندر یخ بسته، ۵ دقیقه حالت شستشو را فعال کنید تا یخ باز شود.\n";
                break;
            case FAULT_BROWNOUT:
                name = "افت شدید ولتاژ برق (E12)";
                chk = "۱. ولتاژ برق ورودی مغازه را بسنجید (در زیر بار نباید به زیر ۲۰۵ ولت افت کند).\n"
                      "۲. از کابل تغذیه حداقل ۴ میلی‌متر و پریز صنعتی اختصاصی استفاده نمایید.\n"
                      "۳. استفاده از استابلایزر ولتاژ استاندارد با توان مناسب شدیداً توصیه می‌شود.\n";
                break;
            default:
                chk = "۱. اتصالات کابل‌ها و ولتاژ تغذیه مدار فرمان را بررسی فرمایید.\n"
                      "۲. کلید برق اصلی دستگاه را ۳۰ ثانیه قطع کرده و مجدداً وصل نمایید.\n";
                break;
        }

        snprintf(s_esc_title, sizeof(s_esc_title), "کد خطای %02u: %s — (تکرار مکرر: %u بار)",
                 (unsigned)fault_code, name, (unsigned)strikes);
        snprintf(s_esc_cause, sizeof(s_esc_cause),
                 "این خطا %u بار پیاپی با زدن دکمه «تلاش مجدد» تکرار شده است.\nعلت ریشه‌ای: نقص فنی فیزیکی برطرف نشده است. جهت پیشگیری از آسیب جدی به قطعات الکتریکی و مکانیکی دستگاه، رفع علت فیزیکی الزامی است.",
                 (unsigned)strikes);

        const char *net_status = is_online ?
            "\n🌐 وضعیت شبکه: دستگاه آنلاین است. در منوی F4 می‌توانید وضعیت بروزرسانی نرم‌افزار را بررسی کنید.\n" :
            "\n🌐 وضعیت شبکه: دستگاه آفلاین است. پیشنهاد می‌شود در تب F4 وای‌فای را متصل کنید تا بروزرسانی‌های خودکار دریافت گردند.\n";

        snprintf(sol_buf, sol_buf_size,
                 "📌 اقدامات تشخیصی کارشناسی:\n%s"
                 "%s"
                 "☎️ پشتیبانی رسمی کارخانه آستیاج:\n  • مهندسی و فنی: ۰۹۱۰۲۳۶۷۰۶۲\n  • خدمات مرکزی: ۰۹۱۳۱۶۹۷۴۷۶\n  • تارنما: ASTIAJ.ir",
                 chk, net_status);
    }

    *out_title = s_esc_title;
    *out_cause = s_esc_cause;
    return true;
}


