#include "project_config.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "GT911";

static uint8_t gt911_addr = GT911_ADDR_A;

static i2c_master_bus_handle_t s_i2c_bus   = NULL;
static i2c_master_dev_handle_t s_gt911_dev = NULL;

/* ---------- راه‌اندازی باس I2C با بازیابی ۹ کلاک صنعتی ---------- */

esp_err_t i2c_master_init(void)
{
    if (s_i2c_bus) return ESP_OK;

    gpio_config_t conf_scl = {
        .pin_bit_mask = (1ULL << I2C_SCL_IO),
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf_scl);

    gpio_config_t conf_sda = {
        .pin_bit_mask = (1ULL << I2C_SDA_IO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf_sda);

    if (gpio_get_level(I2C_SDA_IO) == 0) {
        ESP_LOGW(TAG, "I2C SDA line stuck low - performing 9-clock bus recovery sequence");
        for (int i = 0; i < 9; i++) {
            gpio_set_level(I2C_SCL_IO, 0);
            vTaskDelay(pdMS_TO_TICKS(1));
            gpio_set_level(I2C_SCL_IO, 1);
            vTaskDelay(pdMS_TO_TICKS(1));
            if (gpio_get_level(I2C_SDA_IO) == 1) {
                ESP_LOGI(TAG, "I2C bus freed after %d clocks", i + 1);
                break;
            }
        }
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
}

i2c_master_bus_handle_t i2c_get_bus(void)
{
    return s_i2c_bus;
}

static esp_err_t gt911_attach_device(uint8_t addr)
{
    esp_err_t err;
    if (!s_i2c_bus) {
        err = i2c_master_init();
        if (err != ESP_OK) return err;
    }
    if (s_gt911_dev) {
        i2c_master_bus_rm_device(s_gt911_dev);
        s_gt911_dev = NULL;
    }
    i2c_device_config_t dev_cfg = {
        .device_address = addr,
        .scl_speed_hz   = I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_gt911_dev);
    if (err == ESP_OK) gt911_addr = addr;
    return err;
}

/* ---------- IO رجیسترها ---------- */

static esp_err_t gt911_read_reg(uint16_t reg, uint8_t *buf, size_t len)
{
    if (!s_gt911_dev) return ESP_ERR_INVALID_STATE;
    uint8_t regbuf[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    /* مهلت ۸ms — GT911 @ 400kHz: ۹ بایت ~0.25ms واقعی + حاشیه امن اشتراک باس */
    return i2c_master_transmit_receive(s_gt911_dev, regbuf, 2, buf, len, 8);
}

static esp_err_t gt911_write_reg(uint16_t reg, const uint8_t *data, size_t len)
{
    if (!s_gt911_dev) return ESP_ERR_INVALID_STATE;
    if (len > 64) return ESP_ERR_INVALID_SIZE;
    if (len > 0 && !data) return ESP_ERR_INVALID_ARG;
    uint8_t tmp[2 + 64];
    tmp[0] = (uint8_t)(reg >> 8);
    tmp[1] = (uint8_t)(reg & 0xFF);
    if (len && data) memcpy(&tmp[2], data, len);
    return i2c_master_transmit(s_gt911_dev, tmp, 2 + len, 8);
}

static bool gt911_probe(uint8_t addr)
{
    if (gt911_attach_device(addr) != ESP_OK) return false;
    uint8_t id[4] = {0};
    if (gt911_read_reg(GT911_REG_PRODUCT_ID, id, 4) == ESP_OK) {
        ESP_LOGI(TAG, "GT911 @0x%02X ID=%c%c%c%c", addr, id[0], id[1], id[2], id[3]);
        return true;
    }
    i2c_master_bus_rm_device(s_gt911_dev);
    s_gt911_dev = NULL;
    return false;
}

bool gt911_init(void)
{
    if (gt911_probe(GT911_ADDR_A)) return true;
    if (gt911_probe(GT911_ADDR_B)) return true;
    ESP_LOGE(TAG, "GT911 not found");
    return false;
}

/* ---------- بازیابی باس (Zero-Freeze Recovery) ---------- */

static uint32_t s_gt911_err_count = 0;

static void gt911_recover_bus(void)
{
    static int64_t s_last_recover_log_us = 0;
    int64_t now_us = esp_timer_get_time();
    if (now_us - s_last_recover_log_us > 10000000LL) {
        ESP_LOGW(TAG, "Touch err_count=%u — bus recovery", (unsigned)s_gt911_err_count);
        s_last_recover_log_us = now_us;
    }
    s_gt911_err_count = 0;
    if (gpio_get_level(I2C_SDA_IO) == 0) {
        ESP_LOGW(TAG, "SDA stuck — bus reset");
        if (s_i2c_bus) { i2c_master_bus_reset(s_i2c_bus); }
    }
    if (s_gt911_dev) {
        uint8_t zero = 0;
        gt911_write_reg(GT911_REG_STATUS, &zero, 1);
    }
}

/* ---------- خواندن مختصات با فیلتر ۳ مرحله‌ای DSP صنعتی ----------
 *
 * منطبق با استاندارد TEST 5 (Touch DSP Filter):
 *   ۱. حذف اسپایک EMI کنتاکتور (>120px پرش تک‌فریم در حین لمس پیوسته)
 *   ۲. باند مرده هیسترزیس (±2px) جهت حذف لرزش ریز دست مرطوب/دستکش
 *   ۳. هموارسازی EMA دو مرحله‌ای: (prev * 3 + raw) / 4
 * ---------------------------------------------------------------- */

static bool     s_touch_active_state = false;
static uint16_t s_last_touch_x = 0;
static uint16_t s_last_touch_y = 0;
static int64_t  s_last_touch_us = 0;

static uint16_t s_ema_x      = 0;
static uint16_t s_ema_y      = 0;
static bool     s_ema_init   = false;
static uint8_t  s_spike_cnt  = 0;

static inline void dsp_reset(void)
{
    s_ema_init  = false;
    s_spike_cnt = 0;
}

static void dsp_filter_point(uint16_t rx, uint16_t ry, uint16_t *ox, uint16_t *oy)
{
    if (!s_ema_init) {
        s_ema_x = rx;
        s_ema_y = ry;
        s_last_touch_x = rx;
        s_last_touch_y = ry;
        s_ema_init = true;
        s_spike_cnt = 0;
        *ox = rx;
        *oy = ry;
        return;
    }

    int32_t dx = (int32_t)rx - (int32_t)s_last_touch_x;
    int32_t dy = (int32_t)ry - (int32_t)s_last_touch_y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    /* ۱. Spike Rejection (>120px پرش ناگهانی ناشی از جرقه کنتاکتور در ۱ فریم) */
    if (dx > 120 || dy > 120) {
        s_spike_cnt++;
        if (s_spike_cnt < 2) {
            *ox = s_last_touch_x;
            *oy = s_last_touch_y;
            return;
        }
        /* اگر ۲ فریم متوالی در محل جدید بود، حرکت سریع واقعی انگشت است */
        s_ema_x = rx;
        s_ema_y = ry;
        s_last_touch_x = rx;
        s_last_touch_y = ry;
        s_spike_cnt = 0;
        *ox = rx;
        *oy = ry;
        return;
    }
    s_spike_cnt = 0;

    /* ۲. Deadband Hysteresis (نادیده گرفتن لرزش ±2px) */
    if (dx <= 2 && dy <= 2) {
        *ox = s_last_touch_x;
        *oy = s_last_touch_y;
        return;
    }

    /* ۳. هموارسازی سبک و سریع (Light 50% EMA): بدون تاخیر و دنباله کشدار لمس */
    s_ema_x = (uint16_t)(((uint32_t)s_ema_x + (uint32_t)rx) >> 1);
    s_ema_y = (uint16_t)(((uint32_t)s_ema_y + (uint32_t)ry) >> 1);
    s_last_touch_x = s_ema_x;
    s_last_touch_y = s_ema_y;
    *ox = s_ema_x;
    *oy = s_ema_y;
}

bool gt911_get_touch(uint16_t *x, uint16_t *y)
{
    if (!x || !y || !s_gt911_dev || !s_i2c_bus) return false;

    uint8_t raw[9] = {0};
    esp_err_t err = gt911_read_reg(GT911_REG_STATUS, raw, 9);

    if (err != ESP_OK) {
        s_gt911_err_count++;
        if (s_gt911_err_count >= 20) {
            gt911_recover_bus();
        }
        if (s_touch_active_state && (esp_timer_get_time() - s_last_touch_us) < 18000LL) {
            *x = s_last_touch_x;
            *y = s_last_touch_y;
            return true;
        }
        s_touch_active_state = false;
        dsp_reset();
        return false;
    }

    uint8_t status = raw[0];

    if (!(status & 0x80)) {
        s_gt911_err_count = 0;
        /* بین فریم‌های GT911 (10~17ms) وضعیت حفظ می‌شود تا اسکرول قطع نشود */
        if (s_touch_active_state && (esp_timer_get_time() - s_last_touch_us) < 16000LL) {
            *x = s_last_touch_x;
            *y = s_last_touch_y;
            return true;
        }
        s_touch_active_state = false;
        dsp_reset();
        return false;
    }

    uint8_t n = status & 0x0F;
    if (n == 0) {
        uint8_t zero = 0;
        gt911_write_reg(GT911_REG_STATUS, &zero, 1);
        s_gt911_err_count = 0;
        s_touch_active_state = false;
        dsp_reset();
        return false;
    }

    uint16_t rx = (uint16_t)(raw[2] | (raw[3] << 8));
    uint16_t ry = (uint16_t)(raw[4] | (raw[5] << 8));

    uint16_t fx = 0, fy = 0;
    dsp_filter_point(rx, ry, &fx, &fy);

    *x = fx;
    *y = fy;
    s_last_touch_us = esp_timer_get_time();
    s_touch_active_state = true;

    uint8_t zero = 0;
    gt911_write_reg(GT911_REG_STATUS, &zero, 1);
    s_gt911_err_count = 0;
    return true;
}
