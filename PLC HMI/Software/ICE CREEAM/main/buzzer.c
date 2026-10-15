#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "storage.h"

#define BUZZER_LEDC_TIMER       LEDC_TIMER_0
#define BUZZER_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL     LEDC_CHANNEL_0
#define BUZZER_LEDC_DUTY_RES    LEDC_TIMER_10_BIT

static gpio_num_t s_buzz_gpio = GPIO_NUM_NC;
static bool s_active_high = true;
static bool s_enabled = true;
static uint8_t s_volume = 40; /* ولوم ملایم و استاندارد کارگاهی (پیش‌فرض ۴۰٪) */
static esp_timer_handle_t s_buzz_timer = NULL;
static portMUX_TYPE s_buzz_mux = portMUX_INITIALIZER_UNLOCKED;

#define PATTERN_MAX_STEPS 8
static uint16_t s_pattern[PATTERN_MAX_STEPS];
static uint8_t s_pattern_len = 0;
static uint8_t s_pattern_idx = 0;

/* فیلتر سخت‌گیرانه نرخ بازر (Rate-Limiter) جهت جلوگیری مطلق از رگبار بوق */
static int64_t s_last_alarm_us = 0;
static int64_t s_last_error_us = 0;

static inline void set_level_raw(bool on)
{
    if (!s_enabled || s_volume == 0) {
        on = false;
    }
    if (s_buzz_gpio >= 0) {
        uint32_t duty = 0;
        if (on) {
            /* تعدیل دیوتی‌سایکل متناسب با ولوم (سقف ۵۱۲) */
            duty = (uint32_t)((uint32_t)s_volume * 512U / 100U);
            if (duty == 0 && s_volume > 0) duty = 1;
        }
        ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, duty);
        ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
    }
}

static void buzz_timer_cb(void *arg)
{
    (void)arg;
    portENTER_CRITICAL(&s_buzz_mux);
    if (s_pattern_idx < s_pattern_len) {
        bool is_on = (s_pattern_idx % 2 == 0);
        set_level_raw(is_on);
        uint16_t dur = s_pattern[s_pattern_idx++];
        if (dur == 0) dur = 10;
        portEXIT_CRITICAL(&s_buzz_mux);
        esp_timer_start_once(s_buzz_timer, (uint64_t)dur * 1000ULL);
    } else {
        set_level_raw(false);
        s_pattern_len = 0;
        s_pattern_idx = 0;
        portEXIT_CRITICAL(&s_buzz_mux);
    }
}

void buzzer_silence(void)
{
    portENTER_CRITICAL(&s_buzz_mux);
    if (s_buzz_timer) {
        esp_timer_stop(s_buzz_timer);
    }
    s_pattern_len = 0;
    s_pattern_idx = 0;
    set_level_raw(false);
    portEXIT_CRITICAL(&s_buzz_mux);
}

void buzzer_init(gpio_num_t gpio, bool active_high)
{
    s_buzz_gpio = gpio;
    s_active_high = active_high;

    /* ۱. پیکربندی تایمر LEDC با فرکانس رزونانس ۲۷۰۰ هرتز */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = BUZZER_LEDC_MODE,
        .duty_resolution  = BUZZER_LEDC_DUTY_RES,
        .timer_num        = BUZZER_LEDC_TIMER,
        .freq_hz          = 2700,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    /* ۲. اتصال پایه سخت‌افزاری به کانال PWM با سطح اولیه صفر مطلق */
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = gpio,
        .speed_mode     = BUZZER_LEDC_MODE,
        .channel        = BUZZER_LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = BUZZER_LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    /* ۳. بارگذاری تنظیمات ماندگار از NVS با سقف پیش‌فرض ۴۰٪ */
    s_enabled = storage_get_bool("buz_en", true);
    s_volume  = (uint8_t)storage_get_u32("buz_vol", 40);
    if (s_volume > 100) s_volume = 40;

    set_level_raw(false);

    if (s_buzz_timer == NULL) {
        esp_timer_create_args_t timer_args = {
            .callback = buzz_timer_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "buz_hw_tmr"
        };
        esp_timer_create(&timer_args, &s_buzz_timer);
    }
}

void buzzer_set_enabled(bool en)
{
    s_enabled = en;
    storage_set_bool("buz_en", en);
    if (!en) {
        buzzer_silence();
    }
}

bool buzzer_is_enabled(void)
{
    return s_enabled;
}

void buzzer_set_volume(uint8_t vol)
{
    if (vol > 100) vol = 100;
    s_volume = vol;
    storage_set_u32("buz_vol", (uint32_t)vol);
    if (vol == 0) {
        buzzer_silence();
    }
}

uint8_t buzzer_get_volume(void)
{
    return s_volume;
}

static void play_pattern(const uint16_t *steps, uint8_t count)
{
    if (s_buzz_timer == NULL) return;
    if (!s_enabled || s_volume == 0) return;
    if (count > PATTERN_MAX_STEPS) count = PATTERN_MAX_STEPS;

    portENTER_CRITICAL(&s_buzz_mux);
    esp_timer_stop(s_buzz_timer);

    for (uint8_t i = 0; i < count; i++) {
        s_pattern[i] = steps[i];
    }
    s_pattern_len = count;
    s_pattern_idx = 1;

    set_level_raw(true);
    uint16_t first_dur = s_pattern[0];
    if (first_dur == 0) first_dur = 10;
    /* سقف سخت‌افزاری حداکثر طول هر مرحله بوق (حداکثر ۱۰۰ms) جهت جلوگیری از آزار صوتی */
    if (first_dur > 100) first_dur = 100;

    portEXIT_CRITICAL(&s_buzz_mux);

    esp_timer_start_once(s_buzz_timer, (uint64_t)first_dur * 1000ULL);
}

void buzz(uint32_t duration_ms)
{
    if (s_buzz_timer == NULL) return;
    if (!s_enabled || s_volume == 0 || duration_ms == 0) {
        set_level_raw(false);
        return;
    }
    if (duration_ms > 100) duration_ms = 100; /* مهار زمان به حداکثر ۱۰۰ms */
    uint16_t steps[1] = { (uint16_t)duration_ms };
    play_pattern(steps, 1);
}

void buzzer_beep_click(void)
{
    /* تیک فوق‌کوتاه و لطیف ۲۵ms جهت فیدبک لمس تاچ */
    uint16_t steps[1] = { 25 };
    play_pattern(steps, 1);
}

void buzzer_beep_confirm(void)
{
    /* دو بیپ کوتاه و سریع */
    uint16_t steps[3] = { 35, 30, 35 };
    play_pattern(steps, 3);
}

void buzzer_beep_error(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_last_error_us < 1200000LL) {
        return; /* ریت لیمیتر: حداکثر ۱ بار در ۱.۲ ثانیه */
    }
    s_last_error_us = now;

    uint16_t steps[3] = { 50, 40, 50 };
    play_pattern(steps, 3);
}

void buzzer_beep_alarm(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_last_alarm_us < 2000000LL) {
        return; /* ریت لیمیتر حیاتی: در رگبار خطاها یا نگهداشتن کلید، حداکثر ۱ بار در هر ۲ ثانیه */
    }
    s_last_alarm_us = now;

    /* الگوی استاندارد آلارم: کوتاه، شیک، بدون کشیدگی و آزار */
    uint16_t steps[3] = { 70, 50, 70 };
    play_pattern(steps, 3);
}
