#include "ds18b20.h"
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DS18B20";

/* قفل بحرانی صرفاً در بازه‌های ۶۰ میکروثانیه‌ای تک‌بیت جهت مصونیت ۱۰۰٪ تایمینگ 1-Wire از وقفه‌ها */
static portMUX_TYPE s_ow_mux = portMUX_INITIALIZER_UNLOCKED;

/* دستورات استاندارد Dallas/Maxim DS18B20 */
#define DS18B20_CMD_SEARCH_ROM      0xF0U
#define DS18B20_CMD_READ_ROM        0x33U
#define DS18B20_CMD_MATCH_ROM       0x55U
#define DS18B20_CMD_SKIP_ROM        0xCCU
#define DS18B20_CMD_CONVERT_T       0x44U
#define DS18B20_CMD_WRITE_SCRATCH   0x4EU
#define DS18B20_CMD_READ_SCRATCH    0xBEU

/* ============================================================================
 * توابع سطح رجیستر GPIO (پشتیبانی از Bank 0: GPIO 0..31 و Bank 1: GPIO 32..48)
 * ============================================================================ */

static inline void ow_drive_low(gpio_num_t pin)
{
    uint32_t p = (uint32_t)pin;
    if (p < 32U) {
        GPIO.out_w1tc = (1UL << p);
        GPIO.enable_w1ts = (1UL << p);
    } else {
        uint32_t bit = (1UL << (p - 32U));
        GPIO.out1_w1tc.val = bit;
        GPIO.enable1_w1ts.val = bit;
    }
}

static inline void ow_drive_high_strong(gpio_num_t pin)
{
    uint32_t p = (uint32_t)pin;
    if (p < 32U) {
        GPIO.out_w1ts = (1UL << p);
        GPIO.enable_w1ts = (1UL << p);
    } else {
        uint32_t bit = (1UL << (p - 32U));
        GPIO.out1_w1ts.val = bit;
        GPIO.enable1_w1ts.val = bit;
    }
}

static inline void ow_release_bus(gpio_num_t pin)
{
    uint32_t p = (uint32_t)pin;
    if (p < 32U) {
        /* شارژ لحظه‌ای خازن کابل به 3.3V و سپس رهاسازی به حالت ورودی با پول‌آپ */
        GPIO.out_w1ts = (1UL << p);
        GPIO.enable_w1tc = (1UL << p);
    } else {
        uint32_t bit = (1UL << (p - 32U));
        GPIO.out1_w1ts.val = bit;
        GPIO.enable1_w1tc.val = bit;
    }
}

static inline uint8_t ow_read_level(gpio_num_t pin)
{
    uint32_t p = (uint32_t)pin;
    if (p < 32U) {
        return (uint8_t)((GPIO.in >> p) & 0x01U);
    } else {
        return (uint8_t)((GPIO.in1.val >> (p - 32U)) & 0x01U);
    }
}

/* ============================================================================
 * محاسبه CRC-8 استاندارد Dallas/Maxim (چندجمله‌ای X^8 + X^5 + X^4 + 1 = 0x8C)
 * ============================================================================ */
static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    if (!data) {
        return 0xFFU;
    }
    uint8_t crc = 0U;
    for (uint8_t i = 0U; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0U; j < 8U; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01U;
            crc >>= 1U;
            if (mix != 0U) {
                crc ^= 0x8CU;
            }
            inbyte >>= 1U;
        }
    }
    return crc;
}

esp_err_t ds18b20_init_pin(gpio_num_t pin)
{
    esp_rom_gpio_pad_select_gpio(pin);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << (uint32_t)pin),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    ow_release_bus(pin);
    esp_rom_delay_us(10);
    return err;
}

bool ds18b20_reset_and_check_presence(gpio_num_t pin)
{
    ow_release_bus(pin);
    esp_rom_delay_us(5);

    /* اگر خط قبل از شروع ریست به زمین اتصال کوتاه شده باشد، سنسور معتبری نیست */
    if (ow_read_level(pin) == 0U) {
        esp_rom_delay_us(50);
        if (ow_read_level(pin) == 0U) {
            return false;
        }
    }

    /* ۱. پالس ریست Master به مدت 480 میکروثانیه */
    portENTER_CRITICAL(&s_ow_mux);
    ow_drive_low(pin);
    esp_rom_delay_us(480);
    ow_drive_high_strong(pin);
    esp_rom_delay_us(1);
    ow_release_bus(pin);

    /* ۲. بررسی بالا رفتن خط در ۱۰ میکروثانیه اول (تایید عدم اتصال کوتاه خط) */
    esp_rom_delay_us(10);
    uint8_t line_high_after_release = ow_read_level(pin);

    /* ۳. نمونه‌برداری پنجره حضور سنسور (بازه 30 تا 210 میکروثانیه پس از رهاسازی) */
    bool presence_seen = false;
    for (uint8_t wait_u = 0U; wait_u < 36U; wait_u++) {
        esp_rom_delay_us(5);
        if (ow_read_level(pin) == 0U) {
            presence_seen = true;
            break;
        }
    }
    portEXIT_CRITICAL(&s_ow_mux);

    /* ۴. تکمیل بازه 480 میکروثانیه‌ای حضور و اطمینان از بازگشت خط به سطح HIGH */
    esp_rom_delay_us(420);
    uint8_t line_recovered = ow_read_level(pin);

    return (line_high_after_release != 0U) && presence_seen && (line_recovered != 0U);
}

static void ow_write_bit(gpio_num_t pin, uint8_t bit)
{
    portENTER_CRITICAL(&s_ow_mux);
    if (bit != 0U) {
        /* نوشتن بیت ۱: پالس LOW کوتاه 6µs و سپس رهاسازی تا پایان اسلات 64µs */
        ow_drive_low(pin);
        esp_rom_delay_us(6);
        ow_drive_high_strong(pin);
        esp_rom_delay_us(1);
        ow_release_bus(pin);
        esp_rom_delay_us(58);
    } else {
        /* نوشتن بیت ۰: نگه‌داشتن LOW به مدت 60µs و سپس بازیابی 5µs */
        ow_drive_low(pin);
        esp_rom_delay_us(60);
        ow_drive_high_strong(pin);
        esp_rom_delay_us(1);
        ow_release_bus(pin);
        esp_rom_delay_us(5);
    }
    portEXIT_CRITICAL(&s_ow_mux);
}

static uint8_t ow_read_bit(gpio_num_t pin)
{
    uint8_t bit = 0U;
    portENTER_CRITICAL(&s_ow_mux);
    ow_drive_low(pin);
    esp_rom_delay_us(3);
    ow_release_bus(pin);
    esp_rom_delay_us(10); /* نمونه‌برداری در ثانیه ۱۳ میکروثانیه پس از لبه پایین‌رونده (پنجره طلایی دیتاشیت) */
    bit = ow_read_level(pin);
    esp_rom_delay_us(50);
    portEXIT_CRITICAL(&s_ow_mux);
    return bit;
}

static void ow_write_byte(gpio_num_t pin, uint8_t data)
{
    for (uint8_t i = 0U; i < 8U; i++) {
        ow_write_bit(pin, (uint8_t)((data >> i) & 0x01U));
    }
}

static uint8_t ow_read_byte(gpio_num_t pin)
{
    uint8_t val = 0U;
    for (uint8_t i = 0U; i < 8U; i++) {
        if (ow_read_bit(pin) != 0U) {
            val |= (uint8_t)(1U << i);
        }
    }
    return val;
}

static bool validate_scratchpad(const uint8_t sp[9])
{
    /* ۱. رد فریم‌های تمام صفر یا تمام یک (خط قطع یا زمین‌شده) */
    bool all_zero = true;
    bool all_ff   = true;
    for (uint8_t i = 0U; i < 9U; i++) {
        if (sp[i] != 0x00U) all_zero = false;
        if (sp[i] != 0xFFU) all_ff   = false;
    }
    if (all_zero || all_ff) {
        return false;
    }

    /* ۲. در آی‌سی اصلی و کلون‌های استاندارد DS18B20، بیت‌های 4..0 بایت ۴ (Configuration) همواره ۱ هستند (0x1F)
     * و یا بایت ۷ برابر 0x10 است. بررسی این شرط جلوی هرگونه CRC تصادفی روی باس نویزی را می‌گیرد. */
    if (((sp[4] & 0x1FU) != 0x1FU) && (sp[7] != 0x10U)) {
        return false;
    }

    /* ۳. بررسی دقیق CRC-8 روی ۸ بایت اول در برابر بایت نهم */
    uint8_t calc_crc = ds18b20_crc8(sp, 8U);
    return (calc_crc == sp[8]);
}

bool ds18b20_probe(gpio_num_t pin)
{
    ds18b20_init_pin(pin);

    for (uint8_t attempt = 0U; attempt < 3U; attempt++) {
        if (!ds18b20_reset_and_check_presence(pin)) {
            esp_rom_delay_us(200);
            continue;
        }

        /* ابتدا تلاش برای خواندن Scratchpad با Skip ROM (هم برای تک‌سنسور و هم باس دارای سنسور معتبر) */
        ow_write_byte(pin, DS18B20_CMD_SKIP_ROM);
        ow_write_byte(pin, DS18B20_CMD_READ_SCRATCH);

        uint8_t sp[9] = {0};
        for (uint8_t i = 0U; i < 9U; i++) {
            sp[i] = ow_read_byte(pin);
        }

        if (validate_scratchpad(sp)) {
            /* اگر رزولوشن روی ۱۲ بیت (0x7F) نبود، آن را روی ۱۲ بیت تنظیم کن */
            if ((sp[4] & 0x60U) != 0x60U) {
                if (ds18b20_reset_and_check_presence(pin)) {
                    ow_write_byte(pin, DS18B20_CMD_SKIP_ROM);
                    ow_write_byte(pin, DS18B20_CMD_WRITE_SCRATCH);
                    ow_write_byte(pin, 0x4BU); /* TH */
                    ow_write_byte(pin, 0x46U); /* TL */
                    ow_write_byte(pin, 0x7FU); /* 12-bit resolution */
                }
            }
            return true;
        }

        /* در صورتی که ۲ سنسور موازی روی یک خط باشند، Read ROM یا Search ROM را نیز بررسی کن */
        ds18b20_rom_t roms[2];
        if (ds18b20_search_roms(pin, roms, 2U) > 0U) {
            return true;
        }

        esp_rom_delay_us(250);
    }

    return false;
}

uint8_t ds18b20_search_roms(gpio_num_t pin, ds18b20_rom_t *out_roms, uint8_t max_devs)
{
    if (!out_roms || max_devs == 0U) {
        return 0U;
    }

    uint8_t found = 0U;
    uint8_t last_discrepancy = 0U;
    bool    last_device_flag = false;
    uint8_t rom_no[8] = {0};

    while (!last_device_flag && (found < max_devs)) {
        if (!ds18b20_reset_and_check_presence(pin)) {
            break;
        }

        uint8_t id_bit_number = 1U;
        uint8_t last_zero = 0U;
        uint8_t rom_byte_number = 0U;
        uint8_t rom_byte_mask = 1U;
        bool    search_ok = true;

        ow_write_byte(pin, DS18B20_CMD_SEARCH_ROM);

        do {
            uint8_t id_bit = ow_read_bit(pin);
            uint8_t cmp_id_bit = ow_read_bit(pin);

            if ((id_bit == 1U) && (cmp_id_bit == 1U)) {
                search_ok = false; /* هیچ پاسخی روی باس وجود ندارد */
                break;
            }

            uint8_t search_direction = 0U;
            if (id_bit != cmp_id_bit) {
                search_direction = id_bit;
            } else {
                if (id_bit_number < last_discrepancy) {
                    search_direction = ((rom_no[rom_byte_number] & rom_byte_mask) > 0U) ? 1U : 0U;
                } else {
                    search_direction = (id_bit_number == last_discrepancy) ? 1U : 0U;
                }
                if (search_direction == 0U) {
                    last_zero = id_bit_number;
                }
            }

            if (search_direction == 1U) {
                rom_no[rom_byte_number] |= rom_byte_mask;
            } else {
                rom_no[rom_byte_number] &= (uint8_t)~rom_byte_mask;
            }

            ow_write_bit(pin, search_direction);

            id_bit_number++;
            rom_byte_mask <<= 1U;
            if (rom_byte_mask == 0U) {
                rom_byte_number++;
                rom_byte_mask = 1U;
            }
        } while (rom_byte_number < 8U);

        if (!search_ok || (id_bit_number <= 64U)) {
            break;
        }

        last_discrepancy = last_zero;
        if (last_discrepancy == 0U) {
            last_device_flag = true;
        }

        /* بررسی CRC-8 کد ۶۴ بیتی ROM و کد خانواده DS18B20 */
        if ((rom_no[0] == DS18B20_FAMILY_CODE || rom_no[0] == DS18S20_FAMILY_CODE || rom_no[0] == DS1822_FAMILY_CODE) &&
            (ds18b20_crc8(rom_no, 7U) == rom_no[7])) {
            memcpy(out_roms[found].rom, rom_no, 8U);
            out_roms[found].valid = true;
            found++;
        } else {
            break;
        }
    }

    return found;
}

esp_err_t ds18b20_start_conversion(gpio_num_t pin)
{
    for (uint8_t attempt = 0U; attempt < 2U; attempt++) {
        if (ds18b20_reset_and_check_presence(pin)) {
            ow_write_byte(pin, DS18B20_CMD_SKIP_ROM);
            ow_write_byte(pin, DS18B20_CMD_CONVERT_T);
            /* نگه‌داشتن Strong Pull-Up پس از فرمان 0x44 جهت تامین جریان پایدار مبدل ۱۲ بیتی ADC */
            ow_drive_high_strong(pin);
            return ESP_OK;
        }
        esp_rom_delay_us(150);
    }
    ow_release_bus(pin);
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t decode_scratchpad_temp(const uint8_t sp[9], uint8_t family_code, float *out_temp_c, uint16_t *out_raw)
{
    int16_t raw = (int16_t)(((uint16_t)sp[1] << 8U) | (uint16_t)sp[0]);
    if (out_raw) {
        *out_raw = (uint16_t)raw;
    }

    /* رد مقدار پیش‌فرض پاورآن ریست سنسور (+85.0 درجه = 0x0550) در صورتی که تبدیل واقعی انجام نشده باشد */
    if (raw == 0x0550) {
        return ESP_ERR_INVALID_STATE;
    }

    float temp_c = 0.0f;
    if (family_code == DS18S20_FAMILY_CODE) {
        temp_c = (float)raw * 0.5f;
    } else {
        temp_c = (float)raw * 0.0625f;
    }

    /* بررسی بازه فیزیکی مجاز سنسور (-55 تا +125 درجه سانتی‌گراد) */
    if ((temp_c < -55.0f) || (temp_c > 125.0f)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (out_temp_c) {
        *out_temp_c = temp_c;
    }
    return ESP_OK;
}

esp_err_t ds18b20_read_temperature(gpio_num_t pin, float *out_temp_c, uint16_t *out_raw)
{
    if (!out_temp_c) {
        return ESP_ERR_INVALID_ARG;
    }

    ow_release_bus(pin);
    esp_rom_delay_us(10);

    /* حداکثر ۳ بار تلاش مجدد فوری برای خواندن Scratchpad در صورت بروز نویز لحظه‌ای کنتاکتور */
    for (uint8_t attempt = 0U; attempt < 3U; attempt++) {
        if (!ds18b20_reset_and_check_presence(pin)) {
            esp_rom_delay_us(200);
            continue;
        }

        ow_write_byte(pin, DS18B20_CMD_SKIP_ROM);
        ow_write_byte(pin, DS18B20_CMD_READ_SCRATCH);

        uint8_t sp[9] = {0};
        for (uint8_t i = 0U; i < 9U; i++) {
            sp[i] = ow_read_byte(pin);
        }

        if (validate_scratchpad(sp)) {
            return decode_scratchpad_temp(sp, DS18B20_FAMILY_CODE, out_temp_c, out_raw);
        }
        esp_rom_delay_us(200);
    }

    if (out_raw) {
        *out_raw = 0xFFFFU;
    }
    return ESP_FAIL;
}

esp_err_t ds18b20_read_temperature_by_rom(gpio_num_t pin, const uint8_t rom[8], float *out_temp_c, uint16_t *out_raw)
{
    if (!rom || !out_temp_c) {
        return ESP_ERR_INVALID_ARG;
    }

    ow_release_bus(pin);
    esp_rom_delay_us(10);

    for (uint8_t attempt = 0U; attempt < 3U; attempt++) {
        if (!ds18b20_reset_and_check_presence(pin)) {
            esp_rom_delay_us(200);
            continue;
        }

        ow_write_byte(pin, DS18B20_CMD_MATCH_ROM);
        for (uint8_t r = 0U; r < 8U; r++) {
            ow_write_byte(pin, rom[r]);
        }
        ow_write_byte(pin, DS18B20_CMD_READ_SCRATCH);

        uint8_t sp[9] = {0};
        for (uint8_t i = 0U; i < 9U; i++) {
            sp[i] = ow_read_byte(pin);
        }

        if (validate_scratchpad(sp)) {
            return decode_scratchpad_temp(sp, rom[0], out_temp_c, out_raw);
        }
        esp_rom_delay_us(200);
    }

    if (out_raw) {
        *out_raw = 0xFFFFU;
    }
    return ESP_FAIL;
}
