#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===== Firmware Version (Single Source of Truth) =====
#define FIRMWARE_VERSION_STR     "v4.7.7"
#define FIRMWARE_VERSION_CODE    "4.7.7"
#define FIRMWARE_MODEL_STR       "ASTIAJ CM-700"
#define FIRMWARE_SYSTEM_DESC     "INDUSTRIAL GELATO SYSTEM"

// ===== Pins =====
#define PIN_MOSI   10
#define PIN_MISO   -1
#define PIN_SCLK   12
#define PIN_CS     13
#define PIN_DC     14
#define PIN_RST    -1
#define PIN_BL     -1

// ===== Params =====
#define SPI_HOST   SPI2_HOST
#define SPI_MHZ    80

#define TFT_W      320
#define TFT_H      480

#define LCD_USE_RGB565   1
#if LCD_USE_RGB565
  #define BYTES_PER_PIXEL 2
#else
  #define BYTES_PER_PIXEL 3
#endif

#define CHUNK_PIXELS   (480 * 32)
#define CHUNK_BYTES    (CHUNK_PIXELS * BYTES_PER_PIXEL)

// --- ILI9488 commands ---
#define CMD_SWRESET   0x01
#define CMD_SLP_OUT   0x11
#define CMD_DISP_ON   0x29
#define CMD_COLMOD    0x3A
#define CMD_MADCTL    0x36
#define CMD_CASET     0x2A
#define CMD_PASET     0x2B
#define CMD_RAMWR     0x2C

// MADCTL bits
#define MADCTL_MY   0x80
#define MADCTL_MX   0x40
#define MADCTL_MV   0x20
#define MADCTL_ML   0x10
#define MADCTL_BGR  0x08
#define MADCTL_MH   0x04

#define USE_BGR      1
#define LCD_MIRROR_X 1
#define LCD_MIRROR_Y 0

// ===== I2C + GT911 =====
#define I2C_PORT     I2C_NUM_0
#define I2C_SCL_IO   9
#define I2C_SDA_IO   8
#define I2C_FREQ_HZ  400000

#define GT911_ADDR_A 0x5D
#define GT911_ADDR_B 0x14

#define GT911_REG_PRODUCT_ID 0x8140
#define GT911_REG_STATUS     0x814E
#define GT911_REG_XY         0x814F

// ===== LVGL =====
#define LVGL_ENABLE       1

/* خروجی‌های رله صنعتی استاندارد آستیاج (Active-LOW: 0 = فعال/روشن، 1 = قطع/خاموش) */
#define Compressor          GPIO_NUM_16     /* Q0: کمپرسور کوچک مخزن شیر */
#define Make_Icecream       GPIO_NUM_42     /* Q1: کمپرسور بزرگ سیلندر انجماد */
#define Paddle              GPIO_NUM_39     /* Q2: موتور پارو */
#define CREAM_PUMP_GPIO     GPIO_NUM_48     /* Q3: پمپ کرم لبه رنگی */
#define RAINBOW_GPIO        GPIO_NUM_40     /* Q4: موتور پارو مخزن شیر */
#define AIR_SOLENOID_GPIO   GPIO_NUM_15     /* Q5: خروجی SSR1 برد صنعتی (شیر برقی باد) */
#define RESET_RELAY_GPIO    GPIO_NUM_35     /* Q6: رله ریست مدار فرمان (پایه استاندارد رله برد صنعتی هافلر) */
#define WASH_PUMP_GPIO      GPIO_NUM_36     /* Q7: خروجی شستشو */
#define SPARE_RELAY_Q8_GPIO GPIO_NUM_38     /* Q8: خروجی رله/SSR رزرو برد صنعتی (Active-LOW: سطح ۱ = قطع و ۱۰۰٪ غیرفعال) */
#define SPARE_RELAY_Q9_GPIO GPIO_NUM_41     /* Q9: خروجی رله/SSR رزرو برد صنعتی (Active-LOW: سطح ۱ = قطع و ۱۰۰٪ غیرفعال) */
#define SPARE_PIN_18        GPIO_NUM_18     /* پین رزرو آزاد برد صنعتی (سطح ۱ = قطع و غیرفعال) */
#define SPARE_RELAY_Q6_GPIO RESET_RELAY_GPIO /* سازگاری با ارجاعات پیشین */

/* ورودی‌های سنسور دیجیتال صنعتی آستیاج (Active-LOW با Pull-Up داخلی) */
#define MICRO_FRONT         GPIO_NUM_4      /* سنسور کشیدن اهرم جلو (I4) */
#define MICRO_REAR          GPIO_NUM_5      /* میکروسوئیچ کلاچ عقب / سفتی بستنی (I2) */
#define INDUCTIVE_GPIO      GPIO_NUM_6      /* سنسور القایی دور پولی پارو (I0) */
#define MICRO_ACTIVE_LEVEL  0
#define INDUCTIVE_ENABLED   1

/* کنترل‌بار موتور پارو و کمپرسور (دیکود شده از نردبان تقسیم مقاومتی ADC1: I3 روی GPIO 5 و I5 روی GPIO 4) */
#define LOAD_MOTOR_GPIO         ((gpio_num_t)5) /* کنترل‌بار موتور پارو (I3 روی کانال ADC1_CH4) */
#define LOAD_COMP_GPIO          ((gpio_num_t)4) /* کنترل‌بار کمپرسور بزرگ (I5 روی کانال ADC1_CH3) */
#define LOAD_FAULT_LEVEL        0
#define LOAD_CTRL_ENABLED       1
#define LOAD_DEBOUNCE_MS        140

/* رله قطع لحظه‌ای برق کنترل برای ریست خطاها (خروجی رله ایزوله استاندارد آستیاج) */
#define RESET_PULSE_MS          2000

/* بازر هشدار پنل (خروجی) */
#define BUZZER_GPIO             GPIO_NUM_45

/* کلیدهای دیجیتال پنل (راهنمای PLC) — active-low با pull-up */
#define KEY_STOP_GPIO       GPIO_NUM_3
#define KEY_START_GPIO      GPIO_NUM_7
#define KEY_DIGITAL_ACTIVE  0
#define KEYPAD_ADC_RESISTOR_LADDER 1 /* 1: فعال‌سازی نردبان مقاومتی ۵ کلید پنل جلویی */
#define KEYPAD_ADC_COL2_5KEYS_ENABLED 1 /* 1: فعال‌سازی ۵ کلید اصلی ستون ۲ روی GPIO 2 (SAVE, CANCEL, MODE, START, STOP) */
#define KEYPAD_ADC_COL1_FKEYS_ENABLED 1 /* 1: فعال‌سازی ستون ۱ (F1..F5 روی GPIO 1) جهت ناوبری مستقیم به تب‌های F1..F5 */

/* آستانه‌های پیوسته و کالیبره‌شده نردبان مقاومتی ستون ۲ (GPIO 2 / ADC1_CH1) - بدون ناحیه مرده (Dead-Zone) */
#define KEYPAD_ADC_IDLE_RAW_MIN     4080 /* ولتاژ پول‌آپ بیکار / خط باز */
#define KEYPAD_ADC_SAVE_RAW_MIN     3601
#define KEYPAD_ADC_SAVE_RAW_MAX     4070 /* کلید SAVE (کلید ۱) */
#define KEYPAD_ADC_CANCEL_RAW_MIN   2850
#define KEYPAD_ADC_CANCEL_RAW_MAX   3600 /* کلید CANCEL (کلید ۲) */
#define KEYPAD_ADC_MODE_RAW_MIN     2100
#define KEYPAD_ADC_MODE_RAW_MAX     2849 /* کلید MODE (کلید ۳ - پوشش ولتاژ ۲۲۵۰mV) */
#define KEYPAD_ADC_START_RAW_MIN    1300
#define KEYPAD_ADC_START_RAW_MAX    2099 /* کلید START (کلید ۴) */
#define KEYPAD_ADC_STOP_RAW_MIN     500
#define KEYPAD_ADC_STOP_RAW_MAX     1299 /* کلید STOP (کلید ۵) */

/* زمان‌های سناریو (ثانیه) — پیش‌فرض؛ مقدار واقعی از HMI خوانده می‌شود */
#define MAKE_COMP_DELAY_S       4           /* تأخیر کمپرسور بزرگ پس از پارو (startup_timer: 0..10) */
#define HOLD_COMP_DELAY_S       120         /* تأخیر کمپرسور کوچک (restart_time_2 یا comp1phase) */
#define MAKE_SMALL_COMP_AT_S    (MAKE_COMP_DELAY_S + HOLD_COMP_DELAY_S) /* فقط مرجع؛ در runtime محاسبه می‌شود */
#define PADDLE_DEFAULT_S        (10 * 60)   /* حداکثر پارو تکی پیش‌فرض */
#define PADDLE_MIN_S            (10 * 60)
#define PADDLE_MAX_S            (20 * 60)

/* نگهدارنده — کمپرسور بزرگ زمانی (مثال سناریو: ۱ دقیقه در هر ۲ ساعت) */
#define HOLD_BIG_PERIOD_S       (2 * 60 * 60) /* دوره: restart_time_1 */
#define HOLD_BIG_RUN_S          60            /* مدت روشن: comp3phase_time */

/* فاز ۲ — پایان تولید و فروش */
#define REAR_PADDLE_OFF_S       15          /* خاموشی پارو پس از میکرو عقب */
#define FRONT_HOLD_COMP_S       6           /* نگه‌داشت میکرو جلو برای کمپرسور */
#define SCOOPS_FOR_COMP         3           /* تعداد قیف برای روشن شدن کمپرسور */
#define DISPENSE_COMP_RUN_S     15          /* مدت چرخه کوتاه کمپرسور در فروش */
#define MICRO_POLL_MS           70          /* فاصلهٔ نمونه‌برداری لبه میکرو */
#define INDUCTIVE_TIMEOUT_S     4           /* سنسور القایی — mid_eye_timer */

#ifndef ACTIVE_LOW_OUTPUTS
#define ACTIVE_LOW_OUTPUTS 0
#endif
static inline void pin_write(gpio_num_t pin, int v) {
    int level = ACTIVE_LOW_OUTPUTS ? !v : v;
    gpio_set_level(pin, level);
}
void outputs_init(void);
void inputs_init(void);
bool micro_front_active(void);
bool micro_rear_active(void);
void plc_poll_inputs(void);
bool plc_is_ready(void);
bool plc_is_ending(void);
bool plc_is_dispense(void);
bool plc_is_rechill(void);
bool plc_is_idle(void);
int32_t plc_get_session_scoops(void);
int32_t plc_get_ending_rem_s(void);
int32_t plc_get_rechill_timer(void);
int32_t plc_get_autocheck_timer_s(void);
int32_t plc_get_paddle_refresh_timer_s(void);
int32_t plc_get_scoops_target(void);
void plc_reset_session_scoops(void);
bool plc_is_fault(void);
bool plc_try_clear_fault(void);
void plc_request_control_reset(void); /* پالس رله ریست + تلاش برای رفع قفل */

typedef enum {
    TRIGGER_NONE = 0,
    TRIGGER_PANEL_KEY,       /* شستی فیزیکی استارت پنل */
    TRIGGER_HMI_TOUCH,       /* کلید لمسی نمایشگر */
    TRIGGER_SCOOPS_REACHED,  /* تحریک اهرم جلو (شمارش قیف) */
    TRIGGER_FRONT_HELD,      /* نگه داشتن اهرم جلو */
    TRIGGER_AUTOCHECK_EXPIRED, /* انقضای تایمر بررسی دوره‌ای F3 */
    TRIGGER_AUTO_RESUME,     /* بازیابی خودکار */
} plc_trigger_t;

typedef enum {
    STOP_TRIGGER_NONE = 0,
    STOP_TRIGGER_REAR_CLUTCH, /* تحریک میکروسوئیچ کلاچ عقب (قوام بستنی) */
    STOP_TRIGGER_TIMEOUT,     /* پایان سقف مجاز انجماد */
    STOP_TRIGGER_USER_STOP,   /* شستی یا کلید استپ */
    STOP_TRIGGER_CYL_FREEZE,  /* حفاظت سرمایش بیش از حد */
} plc_stop_trigger_t;

plc_trigger_t plc_get_start_trigger(void);
plc_stop_trigger_t plc_get_stop_trigger(void);
void plc_set_start_trigger(plc_trigger_t trig);
void plc_set_stop_trigger(plc_stop_trigger_t trig);

#define KEY_PADDLE_START   "pdl_start"
#define KEY_MAKE_START     "mk_start"
#define KEY_ICE_START      "ice_start"

#define KEY_PADDLE_TIME    "pdl_time"
#define KEY_MAKE_TIME      "mk_time"
#define KEY_ICE_TIME       "ice_time"
#define KEY_ICE_COUNTER       "ice_counter"
#define KEY_COMP_1PHASE       "comp1phase_time"
#define KEY_COMP_3PHASE       "comp3phase_time"

/* پارامترهای UI نمونه Delta ASTIAJ */
#define KEY_PDL_T1         "pdl_t1"
#define KEY_PDL_T2         "pdl_t2"
#define KEY_START_T        "start_t"
#define KEY_EYE_T          "eye_t"
#define KEY_RST_T1         "rst_t1"
#define KEY_RST_T2         "rst_t2"
#define KEY_MB_DLY         "mb_dly"
#define KEY_CUR1A          "cur1a"
#define KEY_CUR1B          "cur1b"
#define KEY_CUR2A          "cur2a"
#define KEY_CUR2B          "cur2b"
#define KEY_MAKE_B         "mk_b"
#define KEY_ICE_B          "ice_b"
#define KEY_PADDLE_B       "pdl_b"

#define COMP_ANTI_SHORT_CYCLE_S 0           /* حذف تاخیر تعادل گاز طبق سناریوی کارفرما */
#define MAX_FREEZE_TIMEOUT_S    900         /* حداکثر زمان مجاز انجماد بدون تریپ عقب (۱۵ دقیقه) */
#define DISPENSE_LEVER_MAX_S    45          /* حداکثر زمان مجاز فشرده ماندن اهرم جلو (ثانیه) */
#define NVS_FLUSH_DEBOUNCE_MS   8000        /* تاخیر ذخیره پایدار شمارنده در NVS پس از اتمام فروش (میلی‌ثانیه) */

#define KEY_ICE_DAILY      "ice_daily"
#define KEY_ICE_TOTAL      "ice_total"

extern volatile bool paddle_start, make_start, ice_start;
extern volatile bool paddle_start_b, make_start_b, ice_start_b;
extern int32_t paddle_time,  make_time,  ice_time, icecream_counter, comp1phase_time, comp3phase_time;
extern int32_t icecream_counter_daily, icecream_counter_total;
extern int32_t s_comp_cooldown_left_s;
extern int32_t s_freeze_progress_pct;
extern int32_t s_wash_countdown_s;
extern int32_t paddle_timer_1, paddle_timer_2, startup_timer, mid_eye_timer;
extern int32_t restart_time_1, restart_time_2, modbus_delay;
extern int32_t current_set_1a, current_set_1b, current_set_2a, current_set_2b;

extern int32_t make_seq_counter, paddle_counter;
int32_t make_comp_delay_s(void);
int32_t hold_small_delay_s(void);

#define KEY_MGR_PIN        "mgr_pin"
#define DEFAULT_MGR_PIN    "123456"
#define MASTER_ADMIN_PIN   "12773"

#define KEY_PULLEY_EN      "pulley_en"
extern bool s_pulley_en;
extern uint8_t s_eye_fault_strike_cnt;

#define KEY_SS_TIMEOUT     "ss_timeout"
#define DEFAULT_SS_TIMEOUT 7200 /* پیش‌فرض ۲ ساعت */
extern int32_t screensaver_timeout_s;

#define KEY_CONE_PRICE     "cone_price"
#define DEFAULT_CONE_PRICE 50000

#define KEY_NIGHT_EN       "night_en"
#define KEY_NIGHT_SH       "night_sh"
#define KEY_NIGHT_SM       "night_sm"
#define KEY_NIGHT_EH       "night_eh"
#define KEY_NIGHT_EM       "night_em"
#define DEFAULT_NIGHT_SH   23
#define DEFAULT_NIGHT_SM   30
#define DEFAULT_NIGHT_EH   9
#define DEFAULT_NIGHT_EM   0

#define KEY_MAINT_GSK      "maint_gsk"
#define KEY_MAINT_CND      "maint_cnd"
#define MAINT_GASKET_LIMIT 5000
#define MAINT_CONDENSER_D  30

#define DRAIN_TIMEOUT_S    180

#define KEY_DSP_RUN_S      "dsp_run_s"
#define KEY_FRT_HOLD_S     "frt_hold_s"
#define KEY_SCP_COMP       "scp_comp"
#define KEY_FRZ_TO_M       "frz_to_m"
#define KEY_LEV_MAX_S      "lev_max_s"

extern int32_t cone_price;
extern int32_t dispense_comp_run_s;
extern int32_t front_hold_comp_s;
extern int32_t scoops_for_comp;
extern int32_t max_freeze_timeout_m;
extern int32_t dispense_lever_max_s;
extern bool night_scheduler_en;
extern int32_t night_start_h, night_start_m, night_end_h, night_end_m;
extern int32_t maint_gasket_scoops, maint_condenser_days;
extern bool s_drain_mode_active;
extern int32_t s_drain_timer_s;

void drain_mode_start(void);
void drain_mode_stop(void);
bool drain_mode_is_active(void);
void maint_gasket_reset(void);
void maint_condenser_reset(void);

void app_state_load_from_nvs(void);   /* بوت: پارامترها + خاموش کردن حالت‌ها */
void app_params_load_from_nvs(void);  /* فقط زمان‌ها/شمارنده‌ها — بدون قطع حالت */
void app_state_save_to_nvs(void);
void app_factory_reset_defaults(void);
void nvs_request_lazy_flush(void);

void plc_notify_paddle_started(void);
void plc_notify_make_started(void);
void plc_notify_ice_started(void);
void plc_notify_mode_cleared(void);
void plc_set_big_compressor(bool on);
void plc_stop_all(void);
void plc_hw_start_make(void);
bool plc_is_running_production(void);
void plc_retry_current_fault(void);
void plc_dismiss_current_fault(void);

#ifdef __cplusplus
}
#endif
