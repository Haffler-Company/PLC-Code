#include "app/app_sound.h"
#include <Arduino.h>
#include "app/storage.h"
#include "app/vars.h"

extern Storage storage;

// ساختار گام‌های صوتی با قابلیت مدولاسیون بلندی نسبی برای ریتم‌های موزیکال و ملایم
struct SoundStep {
    uint16_t on_ms;
    uint16_t off_ms;
    uint8_t  vol_pct; // درصد ولوم نسبی (۰ تا ۱۰۰)
};

// ۱. کلیک لمسی سبک، سریع و تمیز (حذف تقه و کلیک‌های زننده)
static const SoundStep PATTERN_TAP[] = {
    { 12, 0, 75 }
};

// ۲. هشدار خطای ملایم (دو ضربه مقطع و آرام، حذف وزوز ممتد و آزاردهنده)
static const SoundStep PATTERN_ERROR[] = {
    { 40, 45, 80 },
    { 48,  0, 65 }
};

// ۳. آهنگ موفقیت و تایید رمز: دو ضربه کوتاه و سریع (beep-beep)
static const SoundStep PATTERN_SUCCESS[] = {
    { 25, 35, 80 },
    { 25,  0, 90 }
};

// ۴. اسکرول بسیار ملایم
static const SoundStep PATTERN_SCROLL[] = {
    { 6, 0, 45 }
};

// ۵. آهنگ تایید نهایی نصب فریمور
static const SoundStep PATTERN_INSTALL[] = {
    { 16, 22, 65 },
    { 16, 26, 75 },
    { 40, 30, 90 },
    { 70,  0, 100 }
};

#include "esp_timer.h"

static const SoundStep *s_cur_pattern = nullptr;
static uint8_t s_cur_pattern_len = 0;
static uint8_t s_cur_step_idx = 0;
static bool s_in_on_phase = false;
static uint32_t s_phase_end_ms = 0;
static uint32_t s_cur_vol = 0;
static esp_timer_handle_t s_sound_timer = NULL;

static inline uint32_t calc_step_duty(uint32_t base_vol, uint8_t pct)
{
    if (base_vol == 0 || pct == 0) return 0;
    uint32_t duty = (base_vol * pct) / 100;
    if (duty == 0 && base_vol > 0) duty = 1;
    if (duty > 255) duty = 255;
    return duty;
}

extern "C" void app_sound_stop(void)
{
    if (s_sound_timer) {
        esp_timer_stop(s_sound_timer);
    }
    ledcWrite(BUZZER, 0);
    s_cur_pattern = nullptr;
    s_cur_pattern_len = 0;
    s_cur_step_idx = 0;
    s_in_on_phase = false;
}

static void IRAM_ATTR sound_timer_callback(void *arg)
{
    (void)arg;
    if (!s_cur_pattern || s_cur_vol == 0) {
        ledcWrite(BUZZER, 0);
        s_cur_pattern = nullptr;
        return;
    }

    if (s_in_on_phase) {
        // پایان فاز روشن -> قطع بلادرنگ صدای بازر در سطح سخت‌افزار
        ledcWrite(BUZZER, 0);
        s_in_on_phase = false;
        uint16_t off_ms = s_cur_pattern[s_cur_step_idx].off_ms;
        if (off_ms > 0) {
            // زمان سکوت بین ضربه‌ها
            s_phase_end_ms = millis() + off_ms;
            esp_timer_start_once(s_sound_timer, (uint64_t)off_ms * 1000ULL);
        } else {
            // انتقال به گام بعدی
            s_cur_step_idx++;
            if (s_cur_step_idx < s_cur_pattern_len) {
                s_in_on_phase = true;
                s_phase_end_ms = millis() + s_cur_pattern[s_cur_step_idx].on_ms;
                ledcWrite(BUZZER, calc_step_duty(s_cur_vol, s_cur_pattern[s_cur_step_idx].vol_pct));
                esp_timer_start_once(s_sound_timer, (uint64_t)s_cur_pattern[s_cur_step_idx].on_ms * 1000ULL);
            } else {
                s_cur_pattern = nullptr;
            }
        }
    } else {
        // پایان فاز سکوت -> شروع صدای گام بعدی
        s_cur_step_idx++;
        if (s_cur_step_idx < s_cur_pattern_len) {
            s_in_on_phase = true;
            s_phase_end_ms = millis() + s_cur_pattern[s_cur_step_idx].on_ms;
            ledcWrite(BUZZER, calc_step_duty(s_cur_vol, s_cur_pattern[s_cur_step_idx].vol_pct));
            esp_timer_start_once(s_sound_timer, (uint64_t)s_cur_pattern[s_cur_step_idx].on_ms * 1000ULL);
        } else {
            ledcWrite(BUZZER, 0);
            s_cur_pattern = nullptr;
        }
    }
}

extern "C" void app_sound_init(void)
{
    ledcAttach(BUZZER, 7000, 8);
    ledcWrite(BUZZER, 0);
    s_cur_pattern = nullptr;
    s_cur_pattern_len = 0;
    s_cur_step_idx = 0;
    s_in_on_phase = false;

    if (!s_sound_timer) {
        esp_timer_create_args_t timer_args = {
            .callback = &sound_timer_callback,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "app_snd_tmr",
            .skip_unhandled_events = true
        };
        esp_timer_create(&timer_args, &s_sound_timer);
    }
}

extern "C" bool app_sound_is_enabled(void)
{
    return storage.isSoundEnabled();
}

extern "C" void app_sound_set_enabled(bool en)
{
    storage.setSoundEnabled(en);
    if (!en) {
        app_sound_stop();
    }
}

extern "C" void app_sound_play(SoundType snd)
{
    app_sound_stop();

    if (!storage.isSoundEnabled()) {
        return;
    }

    int vol = storage.getBuzzerVolume();
    if (vol <= 0) {
        return;
    }

    s_cur_vol = (uint32_t)vol;

    const SoundStep *pat = nullptr;
    uint8_t len = 0;

    switch (snd) {
        case SOUND_TAP:
            pat = PATTERN_TAP;
            len = sizeof(PATTERN_TAP) / sizeof(PATTERN_TAP[0]);
            break;
        case SOUND_ERROR:
            pat = PATTERN_ERROR;
            len = sizeof(PATTERN_ERROR) / sizeof(PATTERN_ERROR[0]);
            break;
        case SOUND_SUCCESS:
            pat = PATTERN_SUCCESS;
            len = sizeof(PATTERN_SUCCESS) / sizeof(PATTERN_SUCCESS[0]);
            break;
        case SOUND_SCROLL:
            pat = PATTERN_SCROLL;
            len = sizeof(PATTERN_SCROLL) / sizeof(PATTERN_SCROLL[0]);
            break;
        case SOUND_INSTALL:
            pat = PATTERN_INSTALL;
            len = sizeof(PATTERN_INSTALL) / sizeof(PATTERN_INSTALL[0]);
            break;
        default:
            return;
    }

    s_cur_pattern = pat;
    s_cur_pattern_len = len;
    s_cur_step_idx = 0;
    s_in_on_phase = true;
    s_phase_end_ms = millis() + pat[0].on_ms;

    ledcWrite(BUZZER, calc_step_duty(s_cur_vol, pat[0].vol_pct));
    if (s_sound_timer && pat[0].on_ms > 0) {
        esp_timer_start_once(s_sound_timer, (uint64_t)pat[0].on_ms * 1000ULL);
    }
}

extern "C" void app_sound_tick(void)
{
    // این تابع به عنوان گارد پشتیبان (Failsafe) در ترد اصلی اجرا می‌شود تا اگر تایمر سخت‌افزاری
    // تحت شرایط استثنایی صدا را نبسته بود، بوق ممتد رخ ندهد
    if (!s_cur_pattern) return;

    if (!storage.isSoundEnabled() || storage.getBuzzerVolume() <= 0) {
        app_sound_stop();
        return;
    }

    uint32_t now = millis();
    if ((int32_t)(now - s_phase_end_ms) >= 0) {
        if (s_in_on_phase) {
            uint16_t off_ms = s_cur_pattern[s_cur_step_idx].off_ms;
            if (off_ms > 0) {
                ledcWrite(BUZZER, 0);
                s_in_on_phase = false;
                s_phase_end_ms = now + off_ms;
                if (s_sound_timer) {
                    esp_timer_stop(s_sound_timer);
                    esp_timer_start_once(s_sound_timer, (uint64_t)off_ms * 1000ULL);
                }
            } else {
                s_cur_step_idx++;
                if (s_cur_step_idx < s_cur_pattern_len) {
                    s_in_on_phase = true;
                    s_phase_end_ms = now + s_cur_pattern[s_cur_step_idx].on_ms;
                    ledcWrite(BUZZER, calc_step_duty(s_cur_vol, s_cur_pattern[s_cur_step_idx].vol_pct));
                    if (s_sound_timer) {
                        esp_timer_stop(s_sound_timer);
                        esp_timer_start_once(s_sound_timer, (uint64_t)s_cur_pattern[s_cur_step_idx].on_ms * 1000ULL);
                    }
                } else {
                    app_sound_stop();
                }
            }
        } else {
            s_cur_step_idx++;
            if (s_cur_step_idx < s_cur_pattern_len) {
                s_in_on_phase = true;
                s_phase_end_ms = now + s_cur_pattern[s_cur_step_idx].on_ms;
                ledcWrite(BUZZER, calc_step_duty(s_cur_vol, s_cur_pattern[s_cur_step_idx].vol_pct));
                if (s_sound_timer) {
                    esp_timer_stop(s_sound_timer);
                    esp_timer_start_once(s_sound_timer, (uint64_t)s_cur_pattern[s_cur_step_idx].on_ms * 1000ULL);
                }
            } else {
                app_sound_stop();
            }
        }
    }
}
