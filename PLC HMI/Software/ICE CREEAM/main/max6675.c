#include "max6675.h"
#include "ds18b20.h"
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "fault_log.h"
#include "storage.h"

static const char *TAG = "TEMP_DRV";

static temp_sensor_mode_t s_sensor_mode = TEMP_SENSOR_MODE_DS18B20;

static float s_temp_top = 0.0f;
static float s_temp_cyl = 0.0f;
static float s_cal_offset_top = 0.0f;
static float s_cal_offset_cyl = 0.0f;
static bool s_top_valid = false;
static bool s_cyl_valid = false;
static bool s_top_enabled = true;
static uint16_t s_raw_top = 0xFFFFU;
static uint16_t s_raw_cyl = 0xFFFFU;

static SemaphoreHandle_t s_max_mutex = NULL;

/* وضعیت ماشین حالت غیرمسدودکننده DS18B20 (رفرش دقیق هر ۲ ثانیه) */
static uint8_t  s_ds_phase = 0U;
static uint32_t s_last_reprobe_ms = 0U;
static bool     s_top_fault_logged = false;
static bool     s_cyl_fault_logged = false;
static uint8_t  s_top_fail_cnt = 0U;
static uint8_t  s_cyl_fail_cnt = 0U;
static uint8_t  s_cyl_outlier_count = 0U;

/* پشتیبانی از حالت دو سنسور موازی روی یک خط 1-Wire در صورت اتصال مشترک */
static bool          s_ds_shared_bus = false;
static gpio_num_t    s_ds_shared_pin = DS18B20_TOP_GPIO;
static ds18b20_rom_t s_ds_shared_roms[2] = {0};

static inline void direct_reg_gpio_set_low(gpio_num_t pin) {
    if ((uint32_t)pin < 32U) {
        GPIO.out_w1tc = (1UL << (uint32_t)pin);
    } else {
        GPIO.out1_w1tc.val = (1UL << ((uint32_t)pin - 32U));
    }
}

static inline void direct_reg_gpio_set_high(gpio_num_t pin) {
    if ((uint32_t)pin < 32U) {
        GPIO.out_w1ts = (1UL << (uint32_t)pin);
    } else {
        GPIO.out1_w1ts.val = (1UL << ((uint32_t)pin - 32U));
    }
}

static inline uint8_t direct_reg_gpio_get_level(gpio_num_t pin) {
    if ((uint32_t)pin < 32U) {
        return (uint8_t)((GPIO.in >> (uint32_t)pin) & 0x01U);
    } else {
        return (uint8_t)((GPIO.in1.val >> ((uint32_t)pin - 32U)) & 0x01U);
    }
}

#define MEDIAN_WINDOW 10
#define DS_MEDIAN_WIN 3

static float s_top_window[MEDIAN_WINDOW];
static uint8_t s_top_win_idx = 0U;
static uint8_t s_top_win_cnt = 0U;

static float s_cyl_window[MEDIAN_WINDOW];
static uint8_t s_cyl_win_idx = 0U;
static uint8_t s_cyl_win_cnt = 0U;

static float s_top_ema = 0.0f;
static float s_cyl_ema = 0.0f;
static bool s_top_ema_init = false;
static bool s_cyl_ema_init = false;

static float calc_median(float *arr, uint8_t cnt)
{
    if (!arr || cnt == 0U) return 0.0f;
    if (cnt == 1U) return arr[0];
    float sorted[MEDIAN_WINDOW];
    for (uint8_t i = 0U; i < cnt; i++) {
        sorted[i] = arr[i];
    }
    for (uint8_t i = 1U; i < cnt; i++) {
        float key = sorted[i];
        int8_t j = (int8_t)(i - 1U);
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
    return sorted[cnt / 2U];
}

static float apply_anti_jitter_filter(float *ema_state, bool *is_init, float new_val)
{
    if (!(*is_init)) {
        *ema_state = new_val;
        *is_init = true;
        return new_val;
    }
    float diff = new_val - *ema_state;
    if (fabsf(diff) < 0.25f) {
        *ema_state = (*ema_state * 0.94f) + (new_val * 0.06f);
    } else {
        *ema_state = (*ema_state * 0.82f) + (new_val * 0.18f);
    }
    const float max_slew = 0.35f;
    float slew = *ema_state - new_val;
    if (slew > max_slew)       *ema_state = new_val + max_slew;
    else if (slew < -max_slew) *ema_state = new_val - max_slew;

    return *ema_state;
}

static float apply_ds18b20_filter(float *ema_state, bool *is_init, float new_val)
{
    if (!(*is_init)) {
        *ema_state = new_val;
        *is_init = true;
        return new_val;
    }
    float diff = new_val - *ema_state;
    if (fabsf(diff) < 0.08f) {
        return *ema_state;
    }
    if (fabsf(diff) < 0.35f) {
        *ema_state = (*ema_state * 0.40f) + (new_val * 0.60f);
    } else {
        *ema_state = new_val;
    }
    return *ema_state;
}

static void configure_pins_for_max6675(void)
{
    esp_rom_gpio_pad_select_gpio(MAX6675_CS_TOP_GPIO);
    esp_rom_gpio_pad_select_gpio(MAX6675_CS_CYL_GPIO);
    esp_rom_gpio_pad_select_gpio(MAX6675_SCK_GPIO);
    esp_rom_gpio_pad_select_gpio(MAX6675_MISO_GPIO);

    gpio_config_t out_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MAX6675_CS_TOP_GPIO) |
                        (1ULL << MAX6675_CS_CYL_GPIO) |
                        (1ULL << MAX6675_SCK_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&out_conf);
    gpio_set_pull_mode(MAX6675_SCK_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(MAX6675_CS_TOP_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(MAX6675_CS_CYL_GPIO, GPIO_PULLUP_ONLY);

    gpio_config_t in_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << MAX6675_MISO_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&in_conf);

    direct_reg_gpio_set_high(MAX6675_CS_TOP_GPIO);
    direct_reg_gpio_set_high(MAX6675_CS_CYL_GPIO);
    direct_reg_gpio_set_low(MAX6675_SCK_GPIO);
}

static void configure_pins_for_ds18b20(void)
{
    /* غیرفعال‌سازی خروجی‌های SCK و CS2 مربوط به MAX6675 در حالت DS18B20 */
    gpio_config_t idle_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << MAX6675_CS_CYL_GPIO) | (1ULL << MAX6675_SCK_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&idle_conf);

    ds18b20_init_pin(DS18B20_TOP_GPIO);
    ds18b20_init_pin(DS18B20_CYL_GPIO);
}

static uint16_t read_raw_chip(gpio_num_t cs_pin)
{
    uint16_t raw = 0U;

    if (s_max_mutex) {
        xSemaphoreTake(s_max_mutex, pdMS_TO_TICKS(100));
    }

    direct_reg_gpio_set_low(MAX6675_SCK_GPIO);
    esp_rom_delay_us(2);

    direct_reg_gpio_set_low(cs_pin);
    esp_rom_delay_us(25);

    for (int8_t i = 15; i >= 0; i--) {
        direct_reg_gpio_set_high(MAX6675_SCK_GPIO);
        esp_rom_delay_us(6);

        if (direct_reg_gpio_get_level(MAX6675_MISO_GPIO) != 0U) {
            raw |= (uint16_t)(1U << (uint8_t)i);
        }

        direct_reg_gpio_set_low(MAX6675_SCK_GPIO);
        esp_rom_delay_us(6);
    }

    esp_rom_delay_us(10);
    direct_reg_gpio_set_high(cs_pin);
    esp_rom_delay_us(25);

    if (s_max_mutex) {
        xSemaphoreGive(s_max_mutex);
    }

    return raw;
}

static bool parse_max6675_raw(uint16_t raw, float *out_temp, bool *out_open_circuit)
{
    if (!out_temp || !out_open_circuit) return false;
    *out_open_circuit = false;

    if ((raw == 0xFFFFU) || (raw == 0x0000U) || ((raw & 0x8000U) != 0U) || ((raw & 0x0002U) != 0U)) {
        return false;
    }

    if ((raw & 0x0004U) != 0U) {
        *out_open_circuit = true;
        return false;
    }

    int16_t val = (int16_t)((raw >> 3U) & 0x0FFFU);
    float t = (float)val * 0.25f;

    if (t < -40.0f || t > 300.0f) {
        return false;
    }

    *out_temp = t;
    return true;
}

static bool verify_max6675_chip_mounted(gpio_num_t cs_pin)
{
    uint16_t r[3] = {0};
    float    t[3] = {0.0f};
    bool     open_c[3] = {false};
    uint8_t  valid_temp_cnt = 0U;
    uint8_t  valid_frame_cnt = 0U;

    for (uint8_t i = 0U; i < 3U; i++) {
        r[i] = read_raw_chip(cs_pin);
        if ((r[i] != 0xFFFFU) && (r[i] != 0x0000U) && ((r[i] & 0x8002U) == 0U)) {
            valid_frame_cnt++;
            if (parse_max6675_raw(r[i], &t[i], &open_c[i])) {
                valid_temp_cnt++;
            }
        }
        esp_rom_delay_us(500);
    }

    /* اگر هر ۳ نمونه دمای معتبر و پایدار برگرداندند، آی‌سی MAX6675 قطعاً مونتاژ است */
    if ((valid_temp_cnt >= 2U) && (fabsf(t[0] - t[1]) <= 3.0f)) {
        return true;
    }

    /* اگر هر ۳ نمونه فریم معتبر با بیت Open-Circuit (0x0004) داشتند، آی‌سی مونتاژ است اما ترموکوپل باز است */
    if ((valid_frame_cnt == 3U) && open_c[0] && open_c[1] && open_c[2]) {
        return true;
    }

    return false;
}

static void reset_filter_states(void)
{
    s_top_win_idx = 0U;
    s_top_win_cnt = 0U;
    s_cyl_win_idx = 0U;
    s_cyl_win_cnt = 0U;
    s_top_ema_init = false;
    s_cyl_ema_init = false;
    s_top_fail_cnt = 0U;
    s_cyl_fail_cnt = 0U;
    s_cyl_outlier_count = 0U;
    s_top_fault_logged = false;
    s_cyl_fault_logged = false;
}

static void poll_max6675_internal(void);

void max6675_reprobe(void)
{
    reset_filter_states();
    s_ds_shared_bus = false;

    /* گام ۱: بررسی حضور آی‌سی‌های مونتاژشده MAX6675 روی برد */
    configure_pins_for_max6675();
    esp_rom_delay_us(2000);

    bool max_t1_mounted = verify_max6675_chip_mounted(MAX6675_CS_TOP_GPIO);
    bool max_t2_mounted = verify_max6675_chip_mounted(MAX6675_CS_CYL_GPIO);

    /* گام ۲: بررسی حضور سنسور دیجیتال DS18B20 روی GPIO 21 و GPIO 37 */
    configure_pins_for_ds18b20();
    esp_rom_delay_us(1000);

    bool ds_t1_present = ds18b20_probe(DS18B20_TOP_GPIO);
    bool ds_t2_present = ds18b20_probe(DS18B20_CYL_GPIO);

    /* اگر سنسور DS18B20 با CRC معتبر پاسخ داد، قطعاً برد در مد DS18B20 است */
    if (ds_t1_present || ds_t2_present) {
        s_sensor_mode = TEMP_SENSOR_MODE_DS18B20;
        ESP_LOGI(TAG, "Auto-Detect: DS18B20 mode active (GPIO21_T1=%d, GPIO37_T2=%d)",
                 (int)ds_t1_present, (int)ds_t2_present);

        /* اگر کاربر سنسور T1 را روی GPIO 21 وصل کرده باشد، کانال T1 را خودکار فعال کن */
        if (ds_t1_present && !s_top_enabled) {
            s_top_enabled = true;
            storage_set_bool("tc1_en", true);
        }

        /* بررسی حالت خاص: اگر هر ۲ سنسور DS18B20 به صورت موازی روی یک پین بسته شده باشند */
        if (ds_t1_present && !ds_t2_present) {
            uint8_t cnt = ds18b20_search_roms(DS18B20_TOP_GPIO, s_ds_shared_roms, 2U);
            if (cnt >= 2U) {
                s_ds_shared_bus = true;
                s_ds_shared_pin = DS18B20_TOP_GPIO;
                ESP_LOGI(TAG, "Detected 2 DS18B20 sensors sharing GPIO %d", (int)DS18B20_TOP_GPIO);
            }
        } else if (ds_t2_present && !ds_t1_present) {
            uint8_t cnt = ds18b20_search_roms(DS18B20_CYL_GPIO, s_ds_shared_roms, 2U);
            if (cnt >= 2U) {
                s_ds_shared_bus = true;
                s_ds_shared_pin = DS18B20_CYL_GPIO;
                s_top_enabled = true;
                ESP_LOGI(TAG, "Detected 2 DS18B20 sensors sharing GPIO %d", (int)DS18B20_CYL_GPIO);
            }
        }

        /* انجام اولین تبدیل ۱۲ بیتی همگام در زمان راه‌اندازی جهت آماده‌سازی دمای دقیق اولیه */
        if (ds_t1_present) {
            ds18b20_start_conversion(DS18B20_TOP_GPIO);
        }
        if (ds_t2_present) {
            ds18b20_start_conversion(DS18B20_CYL_GPIO);
        }
        vTaskDelay(pdMS_TO_TICKS(760));

        float t1 = 0.0f, t2 = 0.0f;
        uint16_t raw1 = 0xFFFFU, raw2 = 0xFFFFU;

        if (s_ds_shared_bus) {
            if (ds18b20_read_temperature_by_rom(s_ds_shared_pin, s_ds_shared_roms[0].rom, &t1, &raw1) == ESP_OK) {
                s_temp_top = apply_ds18b20_filter(&s_top_ema, &s_top_ema_init, t1);
                s_top_window[0] = t1;
                s_top_win_cnt = 1U;
                s_top_win_idx = 1U;
                s_top_valid = true;
                s_raw_top = raw1;
            }
            if (ds18b20_read_temperature_by_rom(s_ds_shared_pin, s_ds_shared_roms[1].rom, &t2, &raw2) == ESP_OK) {
                s_temp_cyl = apply_ds18b20_filter(&s_cyl_ema, &s_cyl_ema_init, t2);
                s_cyl_window[0] = t2;
                s_cyl_win_cnt = 1U;
                s_cyl_win_idx = 1U;
                s_cyl_valid = true;
                s_raw_cyl = raw2;
            }
        } else {
            if (ds_t1_present && s_top_enabled && (ds18b20_read_temperature(DS18B20_TOP_GPIO, &t1, &raw1) == ESP_OK)) {
                s_temp_top = apply_ds18b20_filter(&s_top_ema, &s_top_ema_init, t1);
                s_top_window[0] = t1;
                s_top_win_cnt = 1U;
                s_top_win_idx = 1U;
                s_top_valid = true;
                s_raw_top = raw1;
            } else {
                s_top_valid = false;
                s_temp_top = 999.0f;
                s_raw_top = 0xFFFFU;
            }

            if (ds_t2_present && (ds18b20_read_temperature(DS18B20_CYL_GPIO, &t2, &raw2) == ESP_OK)) {
                s_temp_cyl = apply_ds18b20_filter(&s_cyl_ema, &s_cyl_ema_init, t2);
                s_cyl_window[0] = t2;
                s_cyl_win_cnt = 1U;
                s_cyl_win_idx = 1U;
                s_cyl_valid = true;
                s_raw_cyl = raw2;
            } else {
                s_cyl_valid = false;
                s_temp_cyl = -999.0f;
                s_raw_cyl = 0xFFFFU;
            }
        }

        /* شروع تبدیل بعدی برای چرخه ۲ ثانیه‌ای زمان اجرا */
        if (s_top_enabled || s_ds_shared_bus) {
            ds18b20_start_conversion(DS18B20_TOP_GPIO);
        }
        ds18b20_start_conversion(DS18B20_CYL_GPIO);
        s_ds_phase = 1U;
        s_last_reprobe_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        return;
    }

    /* اگر DS18B20 نبود ولی آی‌سی MAX6675 روی برد پاسخ داده بود، وارد مد MAX6675 شو */
    if (max_t1_mounted || max_t2_mounted) {
        s_sensor_mode = TEMP_SENSOR_MODE_MAX6675;
        configure_pins_for_max6675();
        esp_rom_delay_us(1000);
        ESP_LOGI(TAG, "Auto-Detect: MAX6675 mode active (T1_IC=%d, T2_IC=%d)",
                 (int)max_t1_mounted, (int)max_t2_mounted);
        poll_max6675_internal();
        s_last_reprobe_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        return;
    }

    /* گام ۳: اگر هیچ‌کدام (نه MAX6675 و نه DS18B20) پاسخ ندادند:
     * طبق دستور کارفرما، چون MAX6675 لحیم‌شده است و خودبه‌خود حذف نمی‌شود، عدم حضور هر دو
     * یعنی برد در حالت DS18B20 است و سیم سنسورهای DS18B20 قطع یا متصل نشده است. */
    s_sensor_mode = TEMP_SENSOR_MODE_DS18B20;
    configure_pins_for_ds18b20();
    s_top_valid = false;
    s_cyl_valid = false;
    s_temp_top = 999.0f;
    s_temp_cyl = -999.0f;
    s_raw_top = 0xFFFFU;
    s_raw_cyl = 0xFFFFU;
    s_ds_phase = 0U;
    s_last_reprobe_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    ESP_LOGW(TAG, "Auto-Detect: Neither MAX6675 nor DS18B20 detected -> Defaulting to DS18B20 mode (sensor wire disconnected)");
}

void max6675_init(void)
{
    ESP_LOGI(TAG, "Initializing Dual-Mode Temperature Subsystem (MAX6675 SPI / DS18B20 1-Wire)...");

    if (!s_max_mutex) {
        s_max_mutex = xSemaphoreCreateMutex();
    }

    s_top_enabled = storage_get_bool("tc1_en", true);

    int32_t off1_x10 = storage_get_i32("t1_off_x10", 0);
    int32_t off2_x10 = storage_get_i32("t2_off_x10", 0);
    s_cal_offset_top = (float)off1_x10 / 10.0f;
    s_cal_offset_cyl = (float)off2_x10 / 10.0f;
    if (s_cal_offset_top < -15.0f) s_cal_offset_top = -15.0f;
    if (s_cal_offset_top > 15.0f)  s_cal_offset_top = 15.0f;
    if (s_cal_offset_cyl < -15.0f) s_cal_offset_cyl = -15.0f;
    if (s_cal_offset_cyl > 15.0f)  s_cal_offset_cyl = 15.0f;

    max6675_reprobe();
}

temp_sensor_mode_t max6675_get_sensor_mode(void)
{
    return s_sensor_mode;
}

static void poll_ds18b20_internal(void)
{
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    /* جلوگیری از بازنویسی مقادیر اولیه بلافاصله پس از reprobe در زمان بوت */
    if ((s_last_reprobe_ms != 0U) && ((now_ms - s_last_reprobe_ms) < 900U)) {
        return;
    }

    /* فاز ۰ (ثانیه‌های فرد): ارسال فرمان شروع تبدیل دما (0x44) روی خطوط 1-Wire
     * فاز ۱ (ثانیه‌های زوج): خواندن Scratchpad پس از گذشت ۱ ثانیه کامل (>750ms) -> رفرش دقیق هر ۲ ثانیه */
    if (s_ds_phase == 0U) {
        if (s_ds_shared_bus) {
            ds18b20_start_conversion(s_ds_shared_pin);
        } else {
            if (s_top_enabled) {
                ds18b20_start_conversion(DS18B20_TOP_GPIO);
            }
            ds18b20_start_conversion(DS18B20_CYL_GPIO);
        }
        s_ds_phase = 1U;
        return;
    }

    s_ds_phase = 0U;
    bool need_immediate_reconvert = false;

    /* ۱. قرائت کانال ۱: مخزن شیر (T1 روی GPIO 21) */
    if (!s_top_enabled) {
        s_top_valid = false;
        s_temp_top = 999.0f;
        s_raw_top = 0U;
        s_top_win_cnt = 0U;
    } else {
        float t1 = 0.0f;
        uint16_t raw1 = 0xFFFFU;
        esp_err_t err1 = s_ds_shared_bus
            ? ds18b20_read_temperature_by_rom(s_ds_shared_pin, s_ds_shared_roms[0].rom, &t1, &raw1)
            : ds18b20_read_temperature(DS18B20_TOP_GPIO, &t1, &raw1);

        if (err1 == ESP_ERR_INVALID_STATE) {
            /* سنسور تازه متصل شده و مقدار پیش‌فرض 85.0°C دارد -> استارت فوری تبدیل بدون ثبت خطا */
            ds18b20_start_conversion(s_ds_shared_bus ? s_ds_shared_pin : DS18B20_TOP_GPIO);
            need_immediate_reconvert = true;
        } else if (err1 == ESP_OK) {
            s_raw_top = raw1;
            s_top_window[s_top_win_idx] = t1;
            s_top_win_idx = (uint8_t)((s_top_win_idx + 1U) % DS_MEDIAN_WIN);
            if (s_top_win_cnt < DS_MEDIAN_WIN) s_top_win_cnt++;

            float med1 = calc_median(s_top_window, s_top_win_cnt);
            s_temp_top = apply_ds18b20_filter(&s_top_ema, &s_top_ema_init, med1);
            s_top_valid = true;
            s_top_fail_cnt = 0U;
            s_top_fault_logged = false;
        } else {
            s_raw_top = 0xFFFFU;
            s_top_fail_cnt++;
            if (s_top_fail_cnt >= 2U) {
                s_top_valid = false;
                s_temp_top = 999.0f;
                s_top_win_cnt = 0U;
                s_top_ema_init = false;
                if (!s_top_fault_logged) {
                    s_top_fault_logged = true;
                    fault_log_record(FAULT_MAX6675_TOP_MISSING, (int16_t)DS18B20_TOP_GPIO);
                    ESP_LOGW(TAG, "DS18B20 T1 (Hopper, GPIO %d) disconnected", (int)DS18B20_TOP_GPIO);
                }
            }
        }
    }

    /* ۲. قرائت کانال ۲: سیلندر انجماد (T2 روی GPIO 37) */
    float t2 = 0.0f;
    uint16_t raw2 = 0xFFFFU;
    esp_err_t err2 = s_ds_shared_bus
        ? ds18b20_read_temperature_by_rom(s_ds_shared_pin, s_ds_shared_roms[1].rom, &t2, &raw2)
        : ds18b20_read_temperature(DS18B20_CYL_GPIO, &t2, &raw2);

    if (err2 == ESP_ERR_INVALID_STATE) {
        ds18b20_start_conversion(s_ds_shared_bus ? s_ds_shared_pin : DS18B20_CYL_GPIO);
        need_immediate_reconvert = true;
    } else if (err2 == ESP_OK) {
        s_raw_cyl = raw2;
        s_cyl_fail_cnt = 0U;
        s_cyl_fault_logged = false;

        if (s_cyl_valid && (fabsf(t2 - s_temp_cyl) > 4.5f)) {
            s_cyl_outlier_count++;
            if (s_cyl_outlier_count < 2U) {
                ESP_LOGW(TAG, "Rejected thermal spike on DS18B20 T2: %.2f C (current=%.2f C)", t2, s_temp_cyl);
                if (need_immediate_reconvert) {
                    s_ds_phase = 1U;
                }
                return;
            }
        }
        s_cyl_outlier_count = 0U;

        s_cyl_window[s_cyl_win_idx] = t2;
        s_cyl_win_idx = (uint8_t)((s_cyl_win_idx + 1U) % DS_MEDIAN_WIN);
        if (s_cyl_win_cnt < DS_MEDIAN_WIN) s_cyl_win_cnt++;

        float med2 = calc_median(s_cyl_window, s_cyl_win_cnt);
        s_temp_cyl = apply_ds18b20_filter(&s_cyl_ema, &s_cyl_ema_init, med2);
        s_cyl_valid = true;
    } else {
        s_raw_cyl = 0xFFFFU;
        s_cyl_fail_cnt++;
        if (s_cyl_fail_cnt >= 2U) {
            s_cyl_valid = false;
            s_temp_cyl = -999.0f;
            s_cyl_win_cnt = 0U;
            s_cyl_outlier_count = 0U;
            s_cyl_ema_init = false;
            if (!s_cyl_fault_logged) {
                s_cyl_fault_logged = true;
                fault_log_record(FAULT_MAX6675_CYL_MISSING, (int16_t)DS18B20_CYL_GPIO);
                ESP_LOGW(TAG, "DS18B20 T2 (Cylinder, GPIO %d) disconnected", (int)DS18B20_CYL_GPIO);
            }
        }
    }

    if (need_immediate_reconvert) {
        s_ds_phase = 1U;
    }
}

static void poll_max6675_internal(void)
{
    if (!s_top_enabled) {
        s_top_valid = false;
        s_temp_top = 999.0f;
        s_raw_top = 0U;
        s_top_win_cnt = 0U;
    } else {
        uint16_t r1_a = read_raw_chip(MAX6675_CS_TOP_GPIO);
        vTaskDelay(pdMS_TO_TICKS(1));
        uint16_t r1_b = read_raw_chip(MAX6675_CS_TOP_GPIO);
        s_raw_top = r1_b;

        float t1_a = 0.0f, t1_b = 0.0f;
        bool open1_a = false, open1_b = false;
        bool ok1_a = parse_max6675_raw(r1_a, &t1_a, &open1_a);
        bool ok1_b = parse_max6675_raw(r1_b, &t1_b, &open1_b);

        if (open1_a || open1_b || (!ok1_a && !ok1_b)) {
            s_top_valid = false;
            s_temp_top = 999.0f;
            s_top_win_cnt = 0U;
            s_top_ema_init = false;
            s_top_fail_cnt++;
            if (!s_top_fault_logged && s_top_fail_cnt >= 2U) {
                s_top_fault_logged = true;
                fault_log_record(FAULT_THERMO_TOP_DISCONNECT, (int16_t)r1_b);
                ESP_LOGW(TAG, "T1 (Hopper) sensor fault recorded (raw=0x%04X)", r1_b);
            }
        } else {
            float chosen_t1 = ok1_b ? t1_b : t1_a;
            if (ok1_a && ok1_b) {
                chosen_t1 = (fabsf(t1_a - t1_b) < 1.0f) ? ((t1_a + t1_b) * 0.5f) : t1_b;
            }

            s_top_window[s_top_win_idx] = chosen_t1;
            s_top_win_idx = (uint8_t)((s_top_win_idx + 1U) % MEDIAN_WINDOW);
            if (s_top_win_cnt < MEDIAN_WINDOW) s_top_win_cnt++;

            float median_t1 = calc_median(s_top_window, s_top_win_cnt);
            s_temp_top = apply_anti_jitter_filter(&s_top_ema, &s_top_ema_init, median_t1);
            s_top_valid = true;
            s_top_fail_cnt = 0U;
            s_top_fault_logged = false;
        }
    }

    esp_rom_delay_us(100);

    uint16_t r2[3];
    float t2[3];
    bool open2[3] = {false};
    bool ok2[3] = {false};
    uint8_t valid_cnt = 0U;

    for (int k = 0; k < 3; k++) {
        r2[k] = read_raw_chip(MAX6675_CS_CYL_GPIO);
        ok2[k] = parse_max6675_raw(r2[k], &t2[k], &open2[k]);
        if (ok2[k]) valid_cnt++;
        if (k < 2) vTaskDelay(pdMS_TO_TICKS(1));
    }
    s_raw_cyl = r2[2];

    if (valid_cnt == 0U) {
        s_cyl_valid = false;
        s_temp_cyl = -999.0f;
        s_cyl_win_cnt = 0U;
        s_cyl_outlier_count = 0U;
        s_cyl_ema_init = false;
        s_cyl_fail_cnt++;
        if (!s_cyl_fault_logged && s_cyl_fail_cnt >= 2U) {
            s_cyl_fault_logged = true;
            fault_log_record(FAULT_THERMO_CYL_DISCONNECT, (int16_t)r2[2]);
            ESP_LOGW(TAG, "T2 (Cylinder) sensor fault recorded (raw=0x%04X)", r2[2]);
        }
    } else {
        s_cyl_fail_cnt = 0U;
        s_cyl_fault_logged = false;
        float target_t2 = 0.0f;
        if (valid_cnt == 3U) {
            float a = t2[0], b = t2[1], c = t2[2];
            target_t2 = (a > b) ? ((b > c) ? b : ((a > c) ? c : a))
                                : ((a > c) ? a : ((b > c) ? c : b));
        } else if (valid_cnt == 2U) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++) if (ok2[k]) sum += t2[k];
            target_t2 = sum * 0.5f;
        } else {
            for (int k = 0; k < 3; k++) if (ok2[k]) { target_t2 = t2[k]; break; }
        }

        if (s_cyl_valid && fabsf(target_t2 - s_temp_cyl) > 3.5f) {
            s_cyl_outlier_count++;
            if (s_cyl_outlier_count < 2U) {
                ESP_LOGW(TAG, "Rejected thermal spike on T2: %.1f C (current=%.1f C)", target_t2, s_temp_cyl);
                return;
            }
        }
        s_cyl_outlier_count = 0U;

        s_cyl_window[s_cyl_win_idx] = target_t2;
        s_cyl_win_idx = (uint8_t)((s_cyl_win_idx + 1U) % MEDIAN_WINDOW);
        if (s_cyl_win_cnt < MEDIAN_WINDOW) s_cyl_win_cnt++;

        float median_t2 = calc_median(s_cyl_window, s_cyl_win_cnt);
        s_temp_cyl = apply_anti_jitter_filter(&s_cyl_ema, &s_cyl_ema_init, median_t2);
        s_cyl_valid = true;
    }
}

void max6675_poll(void)
{
    if (s_sensor_mode == TEMP_SENSOR_MODE_DS18B20) {
        poll_ds18b20_internal();
    } else {
        poll_max6675_internal();
    }
}

float max6675_get_temp_top(void)
{
    if (!s_top_valid) return s_temp_top;
    return s_temp_top + s_cal_offset_top;
}

float max6675_get_temp_cylinder(void)
{
    if (!s_cyl_valid) return s_temp_cyl;
    return s_temp_cyl + s_cal_offset_cyl;
}

float max6675_get_raw_temp_top(void)
{
    return s_temp_top;
}

float max6675_get_raw_temp_cyl(void)
{
    return s_temp_cyl;
}

float max6675_get_cal_offset_top(void)
{
    return s_cal_offset_top;
}

void max6675_set_cal_offset_top(float offset)
{
    if (offset < -15.0f) offset = -15.0f;
    if (offset > 15.0f)  offset = 15.0f;
    s_cal_offset_top = offset;
    storage_set_i32("t1_off_x10", (int32_t)roundf(offset * 10.0f));
}

float max6675_get_cal_offset_cyl(void)
{
    return s_cal_offset_cyl;
}

void max6675_set_cal_offset_cyl(float offset)
{
    if (offset < -15.0f) offset = -15.0f;
    if (offset > 15.0f)  offset = 15.0f;
    s_cal_offset_cyl = offset;
    storage_set_i32("t2_off_x10", (int32_t)roundf(offset * 10.0f));
}

bool max6675_is_top_valid(void)
{
    return s_top_valid;
}

bool max6675_is_cylinder_valid(void)
{
    return s_cyl_valid;
}

bool max6675_is_top_enabled(void)
{
    return s_top_enabled;
}

void max6675_set_top_enabled(bool en)
{
    s_top_enabled = en;
    storage_set_bool("tc1_en", en);
    if (!en) {
        s_top_valid = false;
        s_temp_top = 999.0f;
    } else if (s_sensor_mode == TEMP_SENSOR_MODE_DS18B20) {
        ds18b20_start_conversion(DS18B20_TOP_GPIO);
        s_ds_phase = 1U;
    }
}

bool max6675_is_cyl_enabled(void)
{
    return true;
}

void max6675_set_cyl_enabled(bool en)
{
    (void)en;
}

uint16_t max6675_get_raw_top(void)
{
    return s_raw_top;
}

uint16_t max6675_get_raw_cylinder(void)
{
    return s_raw_cyl;
}
