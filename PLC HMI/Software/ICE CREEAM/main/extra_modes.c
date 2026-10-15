#include "extra_modes.h"
#include "day_lock_core.h"
#include "project_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "storage.h"
#include "ds3231.h"
#include "wifi.h"
#include "buzzer.h"
#include "time.h"
#include "ui_refresh.h"
#include "fault_log.h"
#include "max6675.h"
#include "panel_nav.h"
#include "esp_timer.h"
#include "analog_inputs.h"
#include "machine_advisor.h"
#include "ui/ui.h"
#include "ui/ui_lang.h"
#include "ui/actions.h"
#include "warranty.h"
#include "lvgl_port.h"

static const char *TAG = "extra";

/* Lazy NVS flush — prevents direct flash writes from LVGL Core 1 callbacks */
extern void nvs_request_lazy_flush(void);



static bool s_wash;
static bool s_edge_prod;
static bool s_edge_wash;
static bool s_rp;
static bool s_rpw;
static bool s_air_manual;
static int32_t s_wash_t;
static int32_t s_edge_t;
static int32_t s_rb_t;
static int32_t s_temp_off = HOLD_TEMP_OFF_C;
static int32_t s_temp_on = HOLD_TEMP_ON_C;
static int32_t s_fallback_on_m = 3;
static int32_t s_fallback_off_m = 5;
static int32_t s_milk_on_s = 20;
static int32_t s_milk_off_m = 5;
static uint32_t s_fallback_t0 = 0;
static bool     s_fallback_active = false;
static bool     s_cooling_active = true;

static void out_set(gpio_num_t pin, bool on)
{
    /* on=true → سطح ۰ مثل بقیه خروجی‌های دستگاه */
    pin_write(pin, on ? 0 : 1);
}

void extra_outputs_init(void)
{
    /* خروجی‌ها در outputs_init با استاندارد زیمنس و ضدگلیچ به صورت یکپارچه پیکربندی شده‌اند */
    out_set(CREAM_PUMP_GPIO, false);
    out_set(AIR_SOLENOID_GPIO, false);
    out_set(RAINBOW_GPIO, false);
    out_set(SPARE_RELAY_Q8_GPIO, false); /* Q8 رزرو Active-LOW: سطح ۱ = خاموش و غیرفعال قطعی */
    out_set(SPARE_RELAY_Q9_GPIO, false); /* Q9 رزرو Active-LOW: سطح ۱ = خاموش و غیرفعال قطعی */
    out_set(SPARE_RELAY_Q6_GPIO, false);
}

void extra_modes_init(void)
{
    s_temp_off = storage_get_i32("ht_off", HOLD_TEMP_OFF_C);
    s_temp_on = storage_get_i32("ht_on", HOLD_TEMP_ON_C);
    if (s_temp_off < -20) s_temp_off = HOLD_TEMP_OFF_C;
    if (s_temp_on > 30) s_temp_on = HOLD_TEMP_ON_C;
    if (s_temp_on <= s_temp_off) s_temp_on = s_temp_off + 4;

    s_fallback_on_m = storage_get_i32("fb_on_m", 3);
    s_fallback_off_m = storage_get_i32("fb_off_m", 5);
    if (s_fallback_on_m < 1 || s_fallback_on_m > 30) s_fallback_on_m = 3;
    if (s_fallback_off_m < 1 || s_fallback_off_m > 60) s_fallback_off_m = 5;

    s_milk_on_s = storage_get_i32("mix_on_s", 20);
    s_milk_off_m = storage_get_i32("mix_off_m", 5);
    if (s_milk_on_s < 5 || s_milk_on_s > 120) s_milk_on_s = 20;
    if (s_milk_off_m < 1 || s_milk_off_m > 60) s_milk_off_m = 5;

    /* مقداردهی موتور روزشمار و امنیت قفل آستیاج */
    day_lock_init();
}

void extra_modes_stop_all(void)
{
    s_wash = false;
    s_edge_prod = false;
    s_edge_wash = false;
    s_rp = false;
    s_rpw = false;
    s_air_manual = false;
    s_wash_t = 0;
    s_wash_countdown_s = 0;
    s_edge_t = 0;
    s_rb_t = 0;
    out_set(CREAM_PUMP_GPIO, false);
    out_set(AIR_SOLENOID_GPIO, false);
    out_set(RAINBOW_GPIO, false);
    out_set(WASH_PUMP_GPIO, false);
    pin_write(Paddle, 1);
}

bool wash_is_active(void) { return s_wash; }
bool edge_is_active(void) { return s_edge_prod || s_edge_wash; }
bool rainbow_is_active(void) { return s_rp || s_rpw; }
bool air_is_active(void) { return s_air_manual || (gpio_get_level(AIR_SOLENOID_GPIO) == 0); }

static bool main_busy(void)
{
    return make_start || ice_start || paddle_start || plc_is_ready() ||
           plc_is_ending() || plc_is_dispense();
}

void wash_toggle(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked()) {
        panel_nav_toast_error("لیزینگ منقضی — رمز را وارد کنید");
        buzzer_beep_alarm();
        return;
    }
    if (warranty_is_expired_and_locked()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "گارانتی فعال نشده است — خروجی‌ها قفل می‌باشند" :
                              ((lang == UI_LANG_TR) ? "Garanti etkinlestirilmedi - Cikislar kilitli" :
                               "Warranty not activated - Outputs locked"));
        buzzer_beep_alarm();
        extern void open_warranty_expired_dialog(void);
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            open_warranty_expired_dialog();
            lvgl_unlock();
        }
        return;
    }
    if (plc_is_fault()) {
        /* پاکسازی خودکار خطای قبلی با لمس مجدد جهت تجربه کاربری روان بدون نیاز به شستی استپ */
        extern void plc_retry_current_fault(void);
        plc_retry_current_fault();
        panel_nav_toast_info("خطای قبلی ریست شد — برای شروع شستشو دوباره لمس کنید");
        buzzer_beep_confirm();
        return;
    }
    if (!s_wash) {
        /* اگر تولید، نگهدارنده، یا پارو فعال است، برای حفظ ایمنی باید ابتدا متوقف شوند */
        if (main_busy()) {
            panel_nav_toast_warning("ابتدا تولید یا نگهدارنده را متوقف کنید");
            buzzer_beep_alarm();
            return;
        }
        /* انتقال روان صنعتی: اگر رینبو یا لبه رنگی فعال است، به طور خودکار خاموش و شستشو شروع شود */
        if (rainbow_is_active()) {
            s_rp = false;
            s_rpw = false;
            out_set(RAINBOW_GPIO, false);
            ice_start_b = false;
        }
        if (edge_is_active()) {
            s_edge_prod = false;
            s_edge_wash = false;
            out_set(CREAM_PUMP_GPIO, false);
            out_set(AIR_SOLENOID_GPIO, false);
            make_start_b = false;
        }
    }
    s_wash = !s_wash;
    s_wash_t = 0;
    if (!s_wash) {
        s_wash_countdown_s = 0;
        out_set(WASH_PUMP_GPIO, false);
        pin_write(Paddle, 1);
        out_set(CREAM_PUMP_GPIO, false);
        out_set(AIR_SOLENOID_GPIO, false);
        paddle_start_b = false;
    } else {
        s_wash_countdown_s = WASH_DEFAULT_S;
        /* شست‌وشو: منحصراً پمپ شستشو (Q7)؛ بدون موتور پارو Q2؛ بدون کمپرسور؛ بدون پمپ‌های اضافی */
        plc_set_big_compressor(false);
        pin_write(Compressor, 1);
        pin_write(Paddle, 1);
        out_set(WASH_PUMP_GPIO, true);
        out_set(CREAM_PUMP_GPIO, false);
        out_set(AIR_SOLENOID_GPIO, false);
        paddle_start_b = false;
        buzz(120);
    }
    ui_apply_state();
}

void edge_toggle_produce(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked()) {
        panel_nav_toast_error("لیزینگ منقضی — رمز را وارد کنید");
        buzzer_beep_alarm();
        return;
    }
    if (warranty_is_expired_and_locked()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "گارانتی فعال نشده است — خروجی‌ها قفل می‌باشند" :
                              ((lang == UI_LANG_TR) ? "Garanti etkinlestirilmedi - Cikislar kilitli" :
                               "Warranty not activated - Outputs locked"));
        buzzer_beep_alarm();
        extern void open_warranty_expired_dialog(void);
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            open_warranty_expired_dialog();
            lvgl_unlock();
        }
        return;
    }
    if (plc_is_fault()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "خطای حفاظتی — STOP برای ریست" :
                              ((lang == UI_LANG_TR) ? "Guvenlik Hatasi — Sifirlamak icin STOP basin" :
                               "Safety Fault — Press STOP to reset"));
        buzzer_beep_alarm();
        return;
    }
    if (s_wash) {
        panel_nav_toast_warning("شست‌وشو روشن است — لبه رنگی مجاز نیست");
        buzzer_beep_alarm();
        return;
    }
    if (s_edge_wash) {
        s_edge_wash = false;
    }
    s_edge_prod = !s_edge_prod;
    s_edge_t = 0;
    if (!s_edge_prod) {
        out_set(CREAM_PUMP_GPIO, false);
        out_set(AIR_SOLENOID_GPIO, false);
    } else {
        out_set(CREAM_PUMP_GPIO, true);
        buzz(100);
    }
}

void edge_toggle_wash(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked()) {
        panel_nav_toast_error("لیزینگ منقضی — رمز را وارد کنید");
        buzzer_beep_alarm();
        return;
    }
    if (warranty_is_expired_and_locked()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "گارانتی فعال نشده است — خروجی‌ها قفل می‌باشند" :
                              ((lang == UI_LANG_TR) ? "Garanti etkinlestirilmedi - Cikislar kilitli" :
                               "Warranty not activated - Outputs locked"));
        buzzer_beep_alarm();
        extern void open_warranty_expired_dialog(void);
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            open_warranty_expired_dialog();
            lvgl_unlock();
        }
        return;
    }
    if (plc_is_fault()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "خطای حفاظتی — STOP برای ریست" :
                              ((lang == UI_LANG_TR) ? "Guvenlik Hatasi — Sifirlamak icin STOP basin" :
                               "Safety Fault — Press STOP to reset"));
        buzzer_beep_alarm();
        return;
    }
    if (s_wash) {
        panel_nav_toast_warning("شست‌وشوی اصلی روشن است");
        buzzer_beep_alarm();
        return;
    }
    if (s_edge_prod) {
        s_edge_prod = false;
    }
    s_edge_wash = !s_edge_wash;
    s_edge_t = 0;
    if (!s_edge_wash) {
        out_set(CREAM_PUMP_GPIO, false);
        out_set(AIR_SOLENOID_GPIO, false);
    } else {
        out_set(CREAM_PUMP_GPIO, true);
        out_set(AIR_SOLENOID_GPIO, true);
        buzz(100);
    }
}

void rainbow_toggle_rp(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked()) {
        panel_nav_toast_error("لیزینگ منقضی — رمز را وارد کنید");
        buzzer_beep_alarm();
        return;
    }
    if (warranty_is_expired_and_locked()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "گارانتی فعال نشده است — خروجی‌ها قفل می‌باشند" :
                              ((lang == UI_LANG_TR) ? "Garanti etkinlestirilmedi - Cikislar kilitli" :
                               "Warranty not activated - Outputs locked"));
        buzzer_beep_alarm();
        extern void open_warranty_expired_dialog(void);
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            open_warranty_expired_dialog();
            lvgl_unlock();
        }
        return;
    }
    if (plc_is_fault()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "خطای حفاظتی — STOP برای ریست" :
                              ((lang == UI_LANG_TR) ? "Guvenlik Hatasi — Sifirlamak icin STOP basin" :
                               "Safety Fault — Press STOP to reset"));
        buzzer_beep_alarm();
        return;
    }
    if (s_wash) {
        panel_nav_toast_warning("شست‌وشو روشن است — پارو مخزن شیر مجاز نیست");
        buzzer_beep_alarm();
        return;
    }
    if (s_rpw) {
        s_rpw = false;
    }
    s_rp = !s_rp;
    s_rb_t = 0;
    out_set(RAINBOW_GPIO, s_rp);
    if (s_rp) {
        buzz(100);
    }
}

void rainbow_toggle_rpw(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked()) {
        panel_nav_toast_error("لیزینگ منقضی — رمز را وارد کنید");
        buzzer_beep_alarm();
        return;
    }
    if (warranty_is_expired_and_locked()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "گارانتی فعال نشده است — خروجی‌ها قفل می‌باشند" :
                              ((lang == UI_LANG_TR) ? "Garanti etkinlestirilmedi - Cikislar kilitli" :
                               "Warranty not activated - Outputs locked"));
        buzzer_beep_alarm();
        extern void open_warranty_expired_dialog(void);
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            open_warranty_expired_dialog();
            lvgl_unlock();
        }
        return;
    }
    if (plc_is_fault()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "خطای حفاظتی — STOP برای ریست" :
                              ((lang == UI_LANG_TR) ? "Guvenlik Hatasi — Sifirlamak icin STOP basin" :
                               "Safety Fault — Press STOP to reset"));
        buzzer_beep_alarm();
        return;
    }
    if (s_wash) {
        panel_nav_toast_warning("شست‌وشوی اصلی روشن است");
        buzzer_beep_alarm();
        return;
    }
    if (s_rp) {
        s_rp = false;
    }
    s_rpw = !s_rpw;
    s_rb_t = 0;
    out_set(RAINBOW_GPIO, s_rpw);
    if (s_rpw) {
        out_set(CREAM_PUMP_GPIO, true);
        buzz(100);
    } else {
        if (!s_edge_prod && !s_edge_wash && !s_wash) {
            out_set(CREAM_PUMP_GPIO, false);
        }
    }
}

void air_toggle_manual(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked() || warranty_is_expired_and_locked() || plc_is_fault()) {
        buzzer_beep_alarm();
        return;
    }
    s_air_manual = !s_air_manual;
    out_set(AIR_SOLENOID_GPIO, s_air_manual);
    if (s_air_manual) {
        buzz(80);
    }
    ui_apply_state();
}

void extra_modes_tick_1hz(void)
{
    /* در وضعیت تست خروجی‌ها (IO Test در تب F5)، تیک ثانیه‌ای نباید رله‌ها را بازنویسی کند */
    if (io_test_is_active()) {
        return;
    }

    if (s_wash) {
        s_wash_t++;
        int32_t lim = WASH_DEFAULT_S;
        s_wash_countdown_s = (lim > s_wash_t) ? (lim - s_wash_t) : 0;
        pin_write(Paddle, 1);
        plc_set_big_compressor(false);
        pin_write(Compressor, 1);
        out_set(WASH_PUMP_GPIO, true);
        out_set(CREAM_PUMP_GPIO, false);
        out_set(AIR_SOLENOID_GPIO, s_air_manual);
        if (s_wash_t >= lim) {
            ESP_LOGI(TAG, "wash timeout");
            wash_toggle();
            s_wash_countdown_s = 0;
            ui_lang_t lang = ui_lang_get();
            panel_nav_toast_success((lang == UI_LANG_FA) ? "چرخه شستشو با موفقیت به پایان رسید" :
                                    ((lang == UI_LANG_TR) ? "Yikama dongusu basariyla tamamlandi" :
                                     "Wash cycle completed successfully"));
            buzzer_beep_confirm();
            ui_request_apply();
        }
    }

    if (s_edge_prod) {
        s_edge_t++;
        out_set(CREAM_PUMP_GPIO, true);
        /* شیر باد با اجازه میکرو جلو یا فعال‌سازی دستی */
        out_set(AIR_SOLENOID_GPIO, s_air_manual || (micro_front_active() && (s_edge_t >= EDGE_PUMP_MIN_S)));
    }

    if (s_edge_wash) {
        s_edge_t++;
        out_set(CREAM_PUMP_GPIO, true);
        out_set(AIR_SOLENOID_GPIO, true);
        if (s_edge_t >= EDGE_WASH_MAX_S) {
            edge_toggle_wash();
            buzz(250);
            ui_request_apply();
        }
    }

    if (s_rp) {
        s_rb_t++;
        uint32_t on_s = (uint32_t)s_milk_on_s;
        uint32_t total_s = on_s + (uint32_t)(s_milk_off_m * 60);
        if (total_s == 0) total_s = 320;
        uint32_t phase = ((uint32_t)(s_rb_t - 1)) % total_s;
        bool should_run = (phase < on_s);
        out_set(RAINBOW_GPIO, should_run);
    } else if (s_rpw) {
        s_rb_t++;
        out_set(RAINBOW_GPIO, true);
        if (s_rb_t >= EDGE_WASH_MAX_S) {
            rainbow_toggle_rpw();
            buzz(250);
            ui_request_apply();
        }
    } else {
        if (!io_test_is_active()) {
            out_set(RAINBOW_GPIO, false);
        }
        s_rb_t = 0;
    }
}

bool hold_temp_allows_cool(void)
{
    /* حفاظت قطعی: در صورت خالی بودن مخزن شیر، کمپرسور نگهدارنده اجازه روشن شدن ندارد */
    if (stop_button_active()) {
        return false;
    }
#if !TEMP_SENSOR_ENABLED
    (void)s_temp_off;
    (void)s_temp_on;
    return true;
#else

    if (!max6675_is_top_valid() || machine_advisor_is_thermo_fallback_active()) {
        /* حالت پشتیبان: خنک‌کاری duty-cycle تایمری ایمن بدون سنسور T1 */
        uint32_t now_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
        if (!s_fallback_active || s_fallback_t0 == 0) {
            s_fallback_t0    = now_s;   /* ریست چرخه از لحظه‌ای که fallback فعال می‌شود */
            s_fallback_active = true;
        }
        uint32_t on_s    = (uint32_t)(s_fallback_on_m * 60);
        uint32_t off_s   = (uint32_t)(s_fallback_off_m * 60);
        uint32_t total_s = on_s + off_s;
        if (total_s == 0) total_s = 480;
        uint32_t cycle_s = (now_s - s_fallback_t0) % total_s;
        return (cycle_s < on_s);
    }

    /* سنسور برگشته — ریست چرخه fallback تا دفعه بعد از صفر شروع شود */
    if (s_fallback_active) {
        s_fallback_t0    = 0;
        s_fallback_active = false;
    }

    float t = max6675_get_temp_top();
    if (t >= (float)s_temp_on) {
        s_cooling_active = true;
    } else if (t <= (float)s_temp_off) {
        s_cooling_active = false;
    }
    return s_cooling_active;
#endif
}

bool hold_cylinder_temp_allows_cool(void)
{
#if !TEMP_SENSOR_ENABLED
    return true;
#else
    if (max6675_is_cylinder_valid()) {
        float cyl_t = max6675_get_temp_cylinder();
        /* در مد نگهداری، کمپرسور بزرگ تنها در صورتی مجاز است که دمای معتبر سیلندر بالاتر از ۲- درجه باشد */
        if (cyl_t > -40.0f && cyl_t <= -2.0f) {
            return false;
        }
    }
    return true;
#endif
}

bool cylinder_freeze_protection_trip(void)
{
    if (max6675_is_cylinder_valid()) {
        float cyl_t = max6675_get_temp_cylinder();
        /* حفاظت انجماد بحرانی: منحصراً زمانی فعال می‌شود که سنسور کاملاً متصل و معتبر بوده
         * و دمای واقعی سیلندر به زیر ۲۲- درجه سانتی‌گراد رسیده باشد (نه سنسور قطع یا نویز) */
        if (cyl_t > -40.0f && cyl_t <= CYLINDER_MIN_SAFE_TEMP_C) {
            return true;
        }
    }
    return false;
}

int32_t hold_temp_off_c(void) { return s_temp_off; }
int32_t hold_temp_on_c(void) { return s_temp_on; }

void hold_temp_set_off_c(int32_t v)
{
    if (v < 0) v = 0;
    if (v > 10) v = 10;
    if (v >= s_temp_on) v = s_temp_on - 1;
    s_temp_off = v;
    storage_set_i32("ht_off", s_temp_off);
    nvs_request_lazy_flush();
}

void hold_temp_set_on_c(int32_t v)
{
    if (v < 3) v = 3;
    if (v > 20) v = 20;
    if (v <= s_temp_off) v = s_temp_off + 1;
    s_temp_on = v;
    storage_set_i32("ht_on", s_temp_on);
    nvs_request_lazy_flush();
}

int32_t fallback_on_time_m(void) { return s_fallback_on_m; }
int32_t fallback_off_time_m(void) { return s_fallback_off_m; }

void fallback_set_on_time_m(int32_t m)
{
    if (m < 1) m = 1;
    if (m > 30) m = 30;
    s_fallback_on_m = m;
    storage_set_i32("fb_on_m", s_fallback_on_m);
    nvs_request_lazy_flush();
}

void fallback_set_off_time_m(int32_t m)
{
    if (m < 1) m = 1;
    if (m > 60) m = 60;
    s_fallback_off_m = m;
    storage_set_i32("fb_off_m", s_fallback_off_m);
    nvs_request_lazy_flush();
}

int32_t milk_mixer_on_time_s(void) { return s_milk_on_s; }
int32_t milk_mixer_off_time_m(void) { return s_milk_off_m; }

void milk_mixer_set_on_time_s(int32_t s)
{
    if (s < 5) s = 5;
    if (s > 120) s = 120;
    s_milk_on_s = s;
    storage_set_i32("mix_on_s", s_milk_on_s);
    nvs_request_lazy_flush();
}

void milk_mixer_set_off_time_m(int32_t m)
{
    if (m < 1) m = 1;
    if (m > 60) m = 60;
    s_milk_off_m = m;
    storage_set_i32("mix_off_m", s_milk_off_m);
    nvs_request_lazy_flush();
}

int32_t wash_get_countdown_s(void)
{
    return s_wash_countdown_s;
}

bool hold_is_fallback_mode(void)
{
#if !TEMP_SENSOR_ENABLED
    return true;
#else
    return (!max6675_is_top_valid() || machine_advisor_is_thermo_fallback_active());
#endif
}

int32_t hold_get_fallback_rem_s(bool *out_is_cooling)
{
    uint32_t now_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    if (!s_fallback_active || s_fallback_t0 == 0) {
        s_fallback_t0 = now_s;
        s_fallback_active = true;
    }
    uint32_t on_s = (uint32_t)(s_fallback_on_m * 60);
    uint32_t off_s = (uint32_t)(s_fallback_off_m * 60);
    uint32_t total_s = on_s + off_s;
    if (total_s == 0) total_s = 480;
    uint32_t cycle_s = (now_s - s_fallback_t0) % total_s;
    bool cooling = (cycle_s < on_s);
    if (out_is_cooling) *out_is_cooling = cooling;
    if (cooling) {
        return (int32_t)(on_s - cycle_s);
    } else {
        return (int32_t)(total_s - cycle_s);
    }
}

int32_t rainbow_get_cycle_rem_s(bool *out_is_running)
{
    if (!s_rp) {
        if (out_is_running) *out_is_running = false;
        return 0;
    }
    uint32_t on_s = (uint32_t)s_milk_on_s;
    uint32_t total_s = on_s + (uint32_t)(s_milk_off_m * 60);
    if (total_s == 0) total_s = 320;
    uint32_t phase = (s_rb_t > 0 ? (uint32_t)(s_rb_t - 1) : 0) % total_s;
    bool running = (phase < on_s);
    if (out_is_running) *out_is_running = running;
    if (running) {
        return (int32_t)(on_s - phase);
    } else {
        return (int32_t)(total_s - phase);
    }
}


bool lease_is_locked(void)
{
    return day_lock_is_locked();
}

int32_t lease_days_left(void)
{
    return (int32_t)day_lock_get_remain_days();
}

int32_t lease_get_challenge_code(void)
{
    return (int32_t)day_lock_get_active_challenge();
}

bool lease_verify_challenge(int32_t response)
{
    char resp[32];
    snprintf(resp, sizeof(resp), "%ld", (long)response);
    char exp[32] = {0};
    day_lock_calc_challenge_response(exp, sizeof(exp), day_lock_get_active_challenge());
    return (strcmp(resp, exp) == 0);
}

bool lease_is_emergency_used(void)
{
    return day_lock_is_grace_used();
}

void lease_extend_days(int32_t days)
{
    day_lock_extend_days((int16_t)days);
}

bool lease_try_unlock_code(int32_t code)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long)code);
    return day_lock_validate_pin(buf);
}

int32_t lease_stage_days_get(uint8_t stage)
{
    return (int32_t)day_lock_get_stage_days(stage);
}

void lease_stage_days_set(uint8_t stage, int32_t days)
{
    day_lock_set_stage_days(stage, (int16_t)days);
}

uint8_t lease_active_stage_get(void)
{
    return day_lock_get_active_stage();
}

int32_t lease_stage_remain_get(void)
{
    return (int32_t)day_lock_get_remain_days();
}

void lease_start_chain(int32_t days_t1)
{
    day_lock_arm_chain((int16_t)days_t1);
}

void lease_clear_all(void)
{
    day_lock_clear_after_unlock();
}

bool lease_advance_stage(void)
{
    return day_lock_advance_stage();
}

bool lease_apply_grace10(void)
{
    return day_lock_apply_grace10();
}

void lease_stage_code_digits(char *out, size_t out_sz, uint8_t stage)
{
    day_lock_calc_stage_code(out, out_sz, stage, DEFAULT_ADMIN_PIN);
}

bool lease_try_unlock_str(const char *raw)
{
    return day_lock_validate_pin(raw);
}
