/**
 * @file hal_touch.cpp
 * @brief Capacitive Touch Driver Implementation
 */

#include "hal_touch.h"
#include "hal_io.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "HAL_TOUCH";

#define TOUCH_I2C_PORT      I2C_NUM_0
#define FT6X36_ADDR         0x38
#define CST816_ADDR         0x15

static lv_indev_t *s_touch_indev = NULL;
static uint32_t s_last_activity_ms = 0;

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static bool read_touch_ft6x36(int16_t *x, int16_t *y) {
    uint8_t reg = 0x02; // TD_STATUS
    uint8_t data[5] = {0};

    esp_err_t err = i2c_master_write_read_device(TOUCH_I2C_PORT, FT6X36_ADDR, &reg, 1, data, 5, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return false;

    uint8_t touch_count = data[0] & 0x0F;
    if (touch_count == 0) return false;

    uint16_t raw_x = ((data[1] & 0x0F) << 8) | data[2];
    uint16_t raw_y = ((data[3] & 0x0F) << 8) | data[4];

    // Map 270 degree rotation: landscape 480x320
    *x = (int16_t)raw_y;
    *y = 320 - (int16_t)raw_x;

    // Clamp
    if (*x < 0) *x = 0;
    if (*x > 479) *x = 479;
    if (*y < 0) *y = 0;
    if (*y > 319) *y = 319;

    return true;
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    static bool was_pressed = false;
    int16_t x = 0, y = 0;

    bool pressed = read_touch_ft6x36(&x, &y);
    if (pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
        s_last_activity_ms = get_time_ms();

        // Audio tactile feedback on initial press
        if (!was_pressed) {
            hal_io_play_buzzer(BUZZER_TONE_TOUCH);
            was_pressed = true;
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        was_pressed = false;
    }
}

lv_indev_t *hal_touch_init(void) {
    ESP_LOGI(TAG, "Initializing Capacitive Touch Driver...");

    // Reset pulse if RST pin is available
    if (TOUCH_PIN_RST >= 0) {
        gpio_set_direction((gpio_num_t)TOUCH_PIN_RST, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)TOUCH_PIN_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)TOUCH_PIN_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    s_touch_indev = lv_indev_create();
    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch_indev, touch_read_cb);

    ESP_LOGI(TAG, "Capacitive Touch Subsystem Initialized.");
    return s_touch_indev;
}

uint32_t hal_touch_get_last_activity_ms(void) {
    return s_last_activity_ms;
}
