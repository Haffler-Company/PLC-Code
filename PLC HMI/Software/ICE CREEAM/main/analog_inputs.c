/**
 * @file analog_inputs.c
 * @brief Industrial Digital Input Sampling, Pulse-Width Glitch Filter, EMI Radar & Schmitt Debounce
 * @author ASTIAJ Automation Systems
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * لایه‌های حفاظت نرم‌افزاری در برابر نویز الکترومغناطیسی صنعتی (EMI):
 *
 *  I0  سنسور القایی پولی:
 *       ① فیلتر حداقل عرض پالس: ≥2 سیکل (140ms) برای تایید، آنی برای خاموشی
 *       ② رادار EMI: >6 لبه/ثانیه → قفل نویز (ضد جرقه کنتاکتور)
 *  I2  میکروسوئیچ کلاچ عقب  → Schmitt ON=3 (210ms) / OFF=1
 *  I4  میکروسوئیچ اهرم جلو  → Schmitt ON=3 (210ms) / OFF=1
 *  I6  استپ اضطراری قارچی   → Schmitt ON=1 (70ms)  / OFF=2  ← ایمنی، واکنش آنی
 *  I7  شستی استارت چرخه     → Schmitt ON=1 (70ms)  / OFF=1
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 */

#include "analog_inputs.h"
#include "adc.h"
#include "project_config.h"
#include "ui/ui_lang.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "storage.h"
#include <string.h>

static const char *TAG = "ANALOG_IN";

/* پولاریته رله‌های کنترل‌بار: true = N.O. (Active-LOW), false = N.C. (Active-HIGH) */
static bool s_load_m_no = true;
static bool s_load_c_no = true;

static const uint8_t s_default_pinmap[PINMAP_ROLE_COUNT] = {
    TERM_I0, /* ROLE_PULLEY_EYE      -> I0 GPIO 6 */
    TERM_I2, /* ROLE_REAR_CLUTCH     -> I2 GPIO 5 */
    TERM_I4, /* ROLE_FRONT_LEVER     -> I4 GPIO 4 */
    TERM_I3, /* ROLE_LOAD_MOTOR      -> I3 GPIO 5 */
    TERM_I5, /* ROLE_LOAD_COMP       -> I5 GPIO 4 */
    TERM_I1, /* ROLE_SPARE_AUX       -> I1 GPIO 6 */
    TERM_I6, /* ROLE_EMERGENCY_STOP  -> I6 GPIO 3 */
    TERM_I7  /* ROLE_CYCLE_START     -> I7 GPIO 7 */
};

static uint8_t s_pinmap[PINMAP_ROLE_COUNT];
static volatile bool     s_phys_active[PINMAP_TERMINAL_COUNT] = {false};
static volatile uint16_t s_raw_counts[3] = {2048, 2048, 2048};
static portMUX_TYPE      s_mux = portMUX_INITIALIZER_UNLOCKED;

/* ── ① فیلتر سنسور پولی ────────────────────────────────────────────────────
 * پالس‌های میکروثانیه‌ای در سطح سیلیکون توسط Glitch Filter سخت‌افزاری حذف می‌شوند.
 * در سطح نرم‌افزار (اسکن ۷۰ms)، نمونه ۱ تایید پالس است تا حتی در دورهای
 * بالای پولی (۱۲۰ الی ۱۵۰ RPM) هیچ پالسی گم نشود و خطای کاذب E01 صادر نگردد. */
#define PULLEY_MIN_PULSE_CYCLES    1u

/* ── ② رادار EMI (Over-Frequency Guard) ────────────────────────────────────
 * دور فیزیکی پولی: حداکثر ۱۵۰ RPM = ۲.۵ دور/ثانیه × ۲ بادامک = ۵ پالس (۱۰ لبه/ثانیه)
 * نویز ۵۰Hz یا جرقه‌های متوالی کنتاکتور: >۲۰ الی ۱۰۰ لبه/ثانیه
 * آستانه امن ۱۲ لبه در ثانیه: عبور تضمینی دورهای تند پولی + قفل قطعی نویز کنتاکتور */
#define PULLEY_MAX_EDGES_PER_SEC   12u
#define PULLEY_EMI_WINDOW_CYCLES   14u

static uint8_t s_eye_pulse_cycles  = 0;
static bool    s_eye_confirmed     = false;
static uint8_t s_eye_edge_cnt      = 0;
static uint8_t s_eye_window_cycles = 0;
static bool    s_eye_emi_blocked   = false;
static int     s_eye_raw_prev      = -1;  /* -1 = اولین سیکل، بدون تاریخچه */

/* ── ③ کانترهای Schmitt-Trigger هر ورودی ────────────────────────────────── */
static uint8_t s_stop_cnt   = 0;
static uint8_t s_start_cnt  = 0;
static uint8_t s_front_cnt  = 0;
static uint8_t s_rear_cnt   = 0;
static uint8_t s_load_m_cnt = 0;
static uint8_t s_load_c_cnt = 0;
static uint8_t s_i1_cnt     = 0;

/* خروجی دیبانس‌شده با هیسترزیس دوطرفه */
static bool s_stop_debounced   = false;
static bool s_start_debounced  = false;
static bool s_front_debounced  = false;
static bool s_rear_debounced   = false;
static bool s_load_m_debounced = false;
static bool s_load_c_debounced = false;
static bool s_i1_debounced     = false;

static volatile bool s_sim_active[PINMAP_TERMINAL_COUNT] = {false};

static const char *s_term_names[PINMAP_TERMINAL_COUNT] = {
    "I0", "I1", "I2", "I3", "I4", "I5", "I6", "I7"
};

static const char *s_role_names_fa[PINMAP_ROLE_COUNT] = {
    "سنسور القایی پولی",
    "میکروسوئیچ عقب",
    "میکروسوئیچ جلو",
    "کنترل‌بار موتور",
    "کنترل‌بار کمپرسور",
    "ورودی کمکی یدکی",
    "سنسور خالی شیر",
    "شستی استارت"
};

static const char *s_role_names_en[PINMAP_ROLE_COUNT] = {
    "Pulley Sensor",
    "Rear Microswitch",
    "Front Lever",
    "Motor Overload",
    "Comp Overload",
    "Spare Aux Input",
    "Milk Low Sensor",
    "Start Button"
};

static const char *s_role_names_tr[PINMAP_ROLE_COUNT] = {
    "Kasnak Sensoru",
    "Arka Mikroswitch",
    "On Kol Anahtari",
    "Motor Asiri Yuk",
    "Komp. Koruma",
    "Yedek Giris",
    "Sut Bos Sensoru",
    "Baslat Butonu"
};

static void nvs_load_pinmap(void)
{
    memcpy(s_pinmap, s_default_pinmap, sizeof(s_pinmap));
    nvs_handle_t h;
    if (nvs_open("pinmap", NVS_READONLY, &h) == ESP_OK) {
        size_t sz = sizeof(s_pinmap);
        nvs_get_blob(h, "in_map8", s_pinmap, &sz);
        nvs_close(h);
    }
    ESP_LOGI(TAG, "Pinmap loaded: I0=%s, I1=%s, I2=%s, I3=%s, I4=%s, I5=%s, I6=%s, I7=%s",
             s_term_names[s_pinmap[0]], s_term_names[s_pinmap[1]], s_term_names[s_pinmap[2]],
             s_term_names[s_pinmap[3]], s_term_names[s_pinmap[4]], s_term_names[s_pinmap[5]],
             s_term_names[s_pinmap[6]], s_term_names[s_pinmap[7]]);
}

static void nvs_save_pinmap(void)
{
    nvs_handle_t h;
    if (nvs_open("pinmap", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, "in_map8", s_pinmap, sizeof(s_pinmap));
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "Pinmap saved to NVS successfully");
    }
}

esp_err_t analog_inputs_init(void)
{
    nvs_load_pinmap();
    s_load_m_no = storage_get_bool("load_m_no", true);
    s_load_c_no = storage_get_bool("load_c_no", true);

    /* ورودی‌های مستقل دیجیتال I6 (استپ) و I7 (استارت) */
    const gpio_num_t digital_pins[] = {
        KEY_STOP_GPIO,   /* GPIO 3 - استپ قارچی (I6) */
        KEY_START_GPIO   /* GPIO 7 - شستی استارت (I7) */
    };

    for (size_t i = 0; i < sizeof(digital_pins) / sizeof(digital_pins[0]); i++) {
        gpio_num_t p = digital_pins[i];
        esp_rom_gpio_pad_select_gpio(p);
        gpio_set_direction(p, GPIO_MODE_INPUT);
        gpio_set_pull_mode(p, GPIO_PULLUP_ONLY);
        gpio_pullup_en(p);
        gpio_pulldown_dis(p);

        /* فیلتر سخت‌افزاری سیلیکون ESP32-S3: حذف اسپایک‌های نانوثانیه‌ای بوبین رله‌ها و کنتاکتور */
        gpio_pin_glitch_filter_config_t filter_cfg = {
            .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
            .gpio_num = p,
        };
        gpio_glitch_filter_handle_t filter_handle = NULL;
        if (gpio_new_pin_glitch_filter(&filter_cfg, &filter_handle) == ESP_OK && filter_handle != NULL) {
            gpio_glitch_filter_enable(filter_handle);
        }
    }

    /* پایه‌های GPIO 4, 5, 6 توسط درایور adc_oneshot کانفیگ و خوانده می‌شوند */
    ESP_LOGI(TAG, "ASTIAJ PLC Inputs Initialized (ETV3A Resistor-Divider Pairs on GPIO 4,5,6 + Digital on GPIO 3,7).");
    return ESP_OK;
}

void analog_inputs_sim_pulse(uint8_t term, bool active)
{
    if (term < PINMAP_TERMINAL_COUNT) {
        portENTER_CRITICAL(&s_mux);
        s_sim_active[term] = active;
        portEXIT_CRITICAL(&s_mux);
    }
}

/* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * ماکرو Schmitt-Trigger با هیسترزیس دوطرفه
 *  ON_THR:  سیکل‌های پیوسته فعال برای تایید روشن شدن
 *  OFF_THR: سیکل‌های پیوسته خاموش برای تایید قطع شدن
 *  MAX_CNT: سقف کانتر
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
#define SCHMITT_UPDATE(raw, cnt, debounced, ON_THR, OFF_THR, MAX_CNT)  \
    do {                                                                 \
        if (raw) {                                                       \
            if ((cnt) < (MAX_CNT)) (cnt)++;                             \
            if ((cnt) >= (ON_THR)) (debounced) = true;                  \
        } else {                                                         \
            if ((cnt) > 0u) (cnt)--;                                    \
            if ((cnt) <= (OFF_THR)) (debounced) = false;                \
        }                                                                \
    } while (0)

void analog_inputs_poll(void)
{
    /* ── نمونه‌برداری آنالوگ کانال‌های تقسیم مقاومتی ─────────────────── */
    int r_ch5 = adc_get_gpio6(); /* GPIO 6 -> I0 / I1 */
    int r_ch4 = adc_get_gpio5(); /* GPIO 5 -> I2 / I3 */
    int r_ch3 = adc_get_gpio4(); /* GPIO 4 -> I4 / I5 */

    /* مقادیر خام برای تله‌متری صفحه دیاگ F6 */
    s_raw_counts[0] = (uint16_t)r_ch5;
    s_raw_counts[1] = (uint16_t)r_ch4;
    s_raw_counts[2] = (uint16_t)r_ch3;

    /* دیکودینگ باندهای تقسیم مقاومتی مطابق استاندارد سخت‌افزار آستیاج ETV3A:
     *  - بیکار (Idle / هیچ کلیدی فشرده نشده): 1850..2240 (حوالی 2048) -> هر دو قطع
     *  - باند ۱ (پایه‌های زوج: I0, I2, I4): <= 1800 (یا اتصال به زمین 0V) -> زوج وصل
     *  - باند ۰ (پایه‌های فرد: I1, I3, I5): >= 3500 -> فرد وصل
     *  - باند ۲ (هر دو پایه همزمان): 2350..2850 -> هر دو وصل
     */
    const bool raw_i0 = (r_ch5 <= 1800) || (r_ch5 >= 2350 && r_ch5 <= 2850);
    const bool raw_i1 = (r_ch5 >= 3500) || (r_ch5 >= 2350 && r_ch5 <= 2850);

    const bool raw_i2 = (r_ch4 <= 1800) || (r_ch4 >= 2350 && r_ch4 <= 2850);
    const bool raw_i3 = (r_ch4 >= 3500) || (r_ch4 >= 2350 && r_ch4 <= 2850);

    const bool raw_i4 = (r_ch3 <= 1800) || (r_ch3 >= 2350 && r_ch3 <= 2850);
    const bool raw_i5 = (r_ch3 >= 3500) || (r_ch3 >= 2350 && r_ch3 <= 2850);

    /* اعمال پولاریته انتخابی کاربر برای کنترل‌بارها: N.O. (وصل در خطا) یا N.C. (قطع در خطا) */
    const bool raw_i3_eff = s_load_m_no ? raw_i3 : !raw_i3;
    const bool raw_i5_eff = s_load_c_no ? raw_i5 : !raw_i5;

    /* ورودی‌های مستقل دیجیتال Active-LOW (0 = فعال) */
    const bool raw_i6 = (gpio_get_level(KEY_STOP_GPIO)  == 0); /* GPIO 3 - استپ */
    const bool raw_i7 = (gpio_get_level(KEY_START_GPIO) == 0); /* GPIO 7 - استارت */

    /* ── ① فیلتر پالس سنسور القایی پولی ──────────────────────────────────────
     * اسپایک‌های میکروثانیه‌ای در ورودی سیلیکون توسط HW Glitch Filter حذف شده‌اند.
     * در پریود ۷۰ms اسکن PLC، نمونه ۱ تایید پالس است تا حتی در دورهای
     * تند پولی (۱۲۰ الی ۱۵۰ RPM) هیچ پالس چرخش شفت گم نشود.             */
    if (raw_i0) {
        if (s_eye_pulse_cycles < PULLEY_MIN_PULSE_CYCLES) {
            s_eye_pulse_cycles++;
        }
        if (s_eye_pulse_cycles >= PULLEY_MIN_PULSE_CYCLES) {
            s_eye_confirmed = true;
        }
    } else {
        s_eye_pulse_cycles = 0;
        s_eye_confirmed    = false;
    }

    /* ── ② رادار EMI سنسور پولی (Over-Frequency Guard) ────────────────────
     * لبه = هر تغییر وضعیت s_eye_confirmed نسبت به سیکل قبل.
     * پنجره ≈1 ثانیه (PULLEY_EMI_WINDOW_CYCLES × 70ms = 980ms).
     * اگر >12 لبه در ثانیه دریافت شود (معادل بیش از 300 RPM که از نظر
     * فیزیکی برای پارو غیرممکن است) -> نویز الکتریکی جرقه تشخیص داده شده
     * و I0 قفل می‌شود تا مانع ریست کاذب تایمر استال ۴ ثانیه‌ای گردد.       */
    const int eye_now = s_eye_confirmed ? 1 : 0;
    if (s_eye_raw_prev < 0) {
        s_eye_raw_prev = eye_now; /* اولین سیکل: مقداردهی اولیه */
    }
    if (eye_now != s_eye_raw_prev) {
        s_eye_edge_cnt++;
    }
    s_eye_raw_prev = eye_now;

    s_eye_window_cycles++;
    if (s_eye_window_cycles >= PULLEY_EMI_WINDOW_CYCLES) {
        if (s_eye_edge_cnt > PULLEY_MAX_EDGES_PER_SEC) {
            if (!s_eye_emi_blocked) {
                s_eye_emi_blocked = true;
                ESP_LOGW(TAG, "EMI RADAR: pulley noise burst! (%u edges/s) I0 locked",
                         (unsigned)s_eye_edge_cnt);
            }
        } else {
            if (s_eye_emi_blocked) {
                s_eye_emi_blocked = false;
                ESP_LOGI(TAG, "EMI RADAR: I0 signal clean — unlocked");
            }
        }
        s_eye_edge_cnt      = 0;
        s_eye_window_cycles = 0;
    }

    /* ── ③ دیبانس Schmitt-Trigger با هیسترزیس صنعتی اختصاصی هر ورودی ───────
     *
     *  ورودی           ON_THR  OFF_THR   تاخیر روشن     دلیل طراحی
     *  ──────────────  ──────  ───────   ─────────────   ────────────────────────
     *  I1 ورودی یدکی     2       1        140 ms         حذف نویز
     *  I2 میکرو عقب      2       1        140 ms         تشخیص سریع سفتی + حذف چتر
     *  I3 کنترل‌بار م.    2       1        140 ms         تشخیص اضافه‌بار پارو + حذف نویز
     *  I4 اهرم جلو       2       1        140 ms         شمارش قطعی قیف + فیلتر فنر
     *  I5 کنترل‌بار ک.    2       1        140 ms         تشخیص اضافه‌بار کمپرسور + حذف نویز
     *  I6 استپ قارچی     1       2         70 ms         ایمنی: واکنش آنی زیر ۱۰۰ms
     *  I7 شستی استارت    2       1        140 ms         تایید لمس ارادی اپراتور
     *
     *  هیسترزیس (ON≠OFF): پایداری قطعی خروجی و حذف کامل چشمک‌زدن لامپ‌های HMI. */
    SCHMITT_UPDATE(raw_i1, s_i1_cnt,     s_i1_debounced,     2u, 1u, 3u);
    SCHMITT_UPDATE(raw_i2, s_rear_cnt,   s_rear_debounced,   2u, 1u, 3u);
    SCHMITT_UPDATE(raw_i3_eff, s_load_m_cnt, s_load_m_debounced, 2u, 1u, 3u);
    SCHMITT_UPDATE(raw_i4, s_front_cnt,  s_front_debounced,  2u, 1u, 3u);
    SCHMITT_UPDATE(raw_i5_eff, s_load_c_cnt, s_load_c_debounced, 2u, 1u, 3u);
    SCHMITT_UPDATE(raw_i6, s_stop_cnt,   s_stop_debounced,   1u, 2u, 3u);
    SCHMITT_UPDATE(raw_i7, s_start_cnt,  s_start_debounced,  2u, 1u, 3u);

    /* ── ④ ترکیب خروجی نهایی ────────────────────────────────────────────────
     * I0 = فیلتر عرض پالس AND NOT(قفل EMI) OR شبیه‌ساز
     * بقیه = دیبانس Schmitt OR شبیه‌ساز                               */
    const bool i0 = (!s_eye_emi_blocked && s_eye_confirmed) || s_sim_active[TERM_I0];
    const bool i1 = s_i1_debounced     || s_sim_active[TERM_I1];
    const bool i2 = s_rear_debounced   || s_sim_active[TERM_I2];
    const bool i3 = s_load_m_debounced || s_sim_active[TERM_I3];
    const bool i4 = s_front_debounced  || s_sim_active[TERM_I4];
    const bool i5 = s_load_c_debounced || s_sim_active[TERM_I5];
    const bool i6 = s_stop_debounced   || s_sim_active[TERM_I6];
    const bool i7 = s_start_debounced  || s_sim_active[TERM_I7];

    portENTER_CRITICAL(&s_mux);
    s_phys_active[TERM_I0] = i0;
    s_phys_active[TERM_I1] = i1;
    s_phys_active[TERM_I2] = i2;
    s_phys_active[TERM_I3] = i3;
    s_phys_active[TERM_I4] = i4;
    s_phys_active[TERM_I5] = i5;
    s_phys_active[TERM_I6] = i6;
    s_phys_active[TERM_I7] = i7;
    portEXIT_CRITICAL(&s_mux);
}


bool pinmap_terminal_is_active(uint8_t term_idx)
{
    if (term_idx >= PINMAP_TERMINAL_COUNT) return false;
    portENTER_CRITICAL(&s_mux);
    bool act = s_phys_active[term_idx];
    portEXIT_CRITICAL(&s_mux);
    return act;
}

uint16_t analog_inputs_get_raw_counts(uint8_t pair_idx)
{
    if (pair_idx > 2) return 0;
    return s_raw_counts[pair_idx];
}

bool pinmap_role_is_active(pinmap_role_t role)
{
    if (role >= PINMAP_ROLE_COUNT) return false;
    uint8_t term;
    portENTER_CRITICAL(&s_mux);
    term = s_pinmap[role];
    portEXIT_CRITICAL(&s_mux);

    if (term >= PINMAP_TERMINAL_COUNT) {
        /* Fail-safe: if Emergency Stop is disconnected/unassigned, remain active! */
        if (role == ROLE_EMERGENCY_STOP) return true;
        return false;
    }
    return pinmap_terminal_is_active(term);
}

/* Logical Role Queries used throughout the firmware */
bool micro_front_active(void)
{
    return pinmap_role_is_active(ROLE_FRONT_LEVER);
}

bool micro_rear_active(void)
{
    return pinmap_role_is_active(ROLE_REAR_CLUTCH);
}

bool inductive_active(void)
{
    return pinmap_role_is_active(ROLE_PULLEY_EYE);
}

bool stop_button_active(void)
{
    return pinmap_role_is_active(ROLE_EMERGENCY_STOP);
}

bool hopper_empty_active(void)
{
    return pinmap_role_is_active(ROLE_HOPPER_EMPTY);
}

bool start_button_active(void)
{
    return pinmap_role_is_active(ROLE_CYCLE_START);
}

bool load_motor_tripped(void)
{
    return pinmap_role_is_active(ROLE_LOAD_MOTOR);
}

bool load_comp_tripped(void)
{
    return pinmap_role_is_active(ROLE_LOAD_COMP);
}

uint8_t pinmap_get_source(pinmap_role_t role)
{
    if (role >= PINMAP_ROLE_COUNT) return PINMAP_NC;
    portENTER_CRITICAL(&s_mux);
    uint8_t src = s_pinmap[role];
    portEXIT_CRITICAL(&s_mux);
    return src;
}

esp_err_t pinmap_set_source(pinmap_role_t role, uint8_t term_idx)
{
    if (role >= PINMAP_ROLE_COUNT || term_idx >= PINMAP_TERMINAL_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_mux);
    /* If another role was using this terminal, swap them */
    uint8_t old_term = s_pinmap[role];
    for (int r = 0; r < PINMAP_ROLE_COUNT; r++) {
        if (r != (int)role && s_pinmap[r] == term_idx) {
            s_pinmap[r] = old_term;
            break;
        }
    }
    s_pinmap[role] = term_idx;
    portEXIT_CRITICAL(&s_mux);

    nvs_save_pinmap();
    return ESP_OK;
}

void pinmap_reset_defaults(void)
{
    portENTER_CRITICAL(&s_mux);
    memcpy(s_pinmap, s_default_pinmap, sizeof(s_pinmap));
    portEXIT_CRITICAL(&s_mux);
    nvs_save_pinmap();
}

const char *pinmap_get_terminal_name(uint8_t term_idx)
{
    if (term_idx >= PINMAP_TERMINAL_COUNT) return "--";
    return s_term_names[term_idx];
}

const char *pinmap_get_role_name(pinmap_role_t role)
{
    if (role >= PINMAP_ROLE_COUNT) return "";
    ui_lang_t lang = ui_lang_get();
    if (lang == UI_LANG_TR) return s_role_names_tr[role];
    if (lang == UI_LANG_EN) return s_role_names_en[role];
    return s_role_names_fa[role];
}

const char *pinmap_get_role_name_fa(pinmap_role_t role)
{
    if (role >= PINMAP_ROLE_COUNT) return "نامشخص";
    return s_role_names_fa[role];
}

const char *pinmap_get_role_name_en(pinmap_role_t role)
{
    if (role >= PINMAP_ROLE_COUNT) return "Unknown";
    return s_role_names_en[role];
}

const char *pinmap_get_role_name_tr(pinmap_role_t role)
{
    if (role >= PINMAP_ROLE_COUNT) return "Bilinmeyen";
    return s_role_names_tr[role];
}

int8_t pinmap_get_role_for_terminal(uint8_t term_idx)
{
    if (term_idx >= PINMAP_TERMINAL_COUNT) return -1;
    portENTER_CRITICAL(&s_mux);
    for (int r = 0; r < PINMAP_ROLE_COUNT; r++) {
        if (s_pinmap[r] == term_idx) {
            portEXIT_CRITICAL(&s_mux);
            return (int8_t)r;
        }
    }
    portEXIT_CRITICAL(&s_mux);
    return -1; /* Spare / Unassigned */
}

bool analog_inputs_get_overload_polarity(bool is_motor)
{
    return is_motor ? s_load_m_no : s_load_c_no;
}

void analog_inputs_set_overload_polarity(bool is_motor, bool is_no)
{
    if (is_motor) {
        s_load_m_no = is_no;
        storage_set_bool("load_m_no", is_no);
        ESP_LOGI(TAG, "Motor overload (LOAD_M) polarity set to: %s", is_no ? "N.O." : "N.C.");
    } else {
        s_load_c_no = is_no;
        storage_set_bool("load_c_no", is_no);
        ESP_LOGI(TAG, "Compressor overload (LOAD_C) polarity set to: %s", is_no ? "N.O." : "N.C.");
    }
}
