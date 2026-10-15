#ifndef HWTEST_H
#define HWTEST_H
//
// ماژول «تست سخت‌افزار» — منطق پروژه‌ی Haffler Test (V.2.0.0 / LVGL 8)
// که برای زندگی داخل فریمور Config (LVGL 9 + FreeRTOS) پورت شده است.
//
// تقسیم وظایف:
//   * بخش UI (صفحات/آبجکت‌ها/متغیرها/اکشن‌ها) توسط پروژه‌ی EEZ ادغام‌شده تولید می‌شود
//     و باید با تارگت LVGL 9 دوباره generate شود. این ماژول به آن نام‌ها وابسته است
//     (objects.* و get_var_*/set_var_* و SCREEN_ID_* صفحات تست).
//   * این ماژول فقط «منطق سخت‌افزاری» تست + پریفرال‌هایی که Config ندارد را در اختیار دارد.
//
// نمایشگر/تاچ متعلق به Config است (extern) و اینجا دوباره تعریف نمی‌شوند.
//
// نحوه‌ی استفاده (در plc_config_project.ino):
//   - در setup() بعد از ui_init():            hwtest_setup();
//   - در lvgl_task() بعد از ui_tick():         if (g_hwtest_active) hwtest_loop();
// و در اکشن دکمه‌ها (داخل .ino یا hwtest.cpp): hwtest_enter() / hwtest_exit()
//

#include <Arduino.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t get_var_buzzer(void);
void set_var_buzzer(int32_t value);

// true تا زمانی که کاربر داخل صفحات «تست سخت‌افزار» است.
extern bool g_hwtest_active;

// تشخیص قطعات برای سازگاریِ برنامه‌ها (در setup یک‌بار صدا زده می‌شود)
extern int  g_max6675_count;      // تعداد MAX6675 شناسایی‌شده (0..2)
extern bool g_mcp23017_present;   // وجود MCP23017 روی I2C
void hwtest_detect_hardware(void);

// خاموشیِ بلادرنگ و فوق‌سریعِ سخت‌افزاری تمام خروجی‌ها در نخستین نانوثانیه‌های بوت (Zero-Glitch Boot Safe)
void hwtest_early_gpio_safety_kill(void);
void hwtest_kill_all_outputs_safe(void);

// راه‌اندازی یک‌باره‌ی پریفرال‌های تست (RTC، سنسور دما، Serial2/RS485، ADC، اسکن I2C).
void hwtest_setup(void);

// تنظیمِ RTC سخت‌افزاری از epoch یونیکس (مثلاً نتیجه‌ی SNTP پس از اتصال وای‌فای).
// اگر RTC راه‌اندازی نشده باشد یا epoch معتبر نباشد (< 2020) کاری نمی‌کند.
void hwtest_set_rtc_from_epoch(uint32_t epoch);

// تنظیم تاریخ و ساعت محلی DS3231
bool hwtest_set_rtc_ymdhms(int year, int month, int day, int hour, int minute, int second);

// راه‌اندازی همگام‌سازی ساعت با اینترنت (NTP) در پس‌زمینه
void app_trigger_ntp_sync(void);
extern volatile bool g_ntp_sync_success;
extern volatile bool g_hwtest_ntp_synced_flag;
extern char g_ntp_synced_time_str[32];

// ورود به محیط تست: پین‌های خروجی رله را OUTPUT می‌کند و بازر PWM (GPIO45) را attach می‌کند.
// توجه: روی این برد GPIO39/40 با خط تاچ مشترک‌اند؛ هنگام تست به‌عنوان خروجی استفاده می‌شوند.
void hwtest_enter(void);

// خروج از محیط تست: خروجی‌ها امن می‌شوند، BLE خاموش، بازر به حالت Config برمی‌گردد و
// کنترلر تاچ دوباره init می‌شود تا GPIO39/40 به نقش تاچ برگردند.
void hwtest_exit(void);

// منطق حلقه‌ی تست — هر تکرار lvgl_task وقتی g_hwtest_active==true فراخوانی می‌شود.
// (معادل بدنه‌ی loop() پروژه‌ی Test، منهای lv_timer_handler/ui_tick که Config خودش می‌زند.)
void hwtest_loop(void);

// اعمالِ سریعِ خروجی‌ها (Q0..Q9) روی رله‌ها — سبک (فقط ۱۰ digitalWrite، بدونِ بلاک).
// هر فریمِ LVGL صدا زده می‌شود تا رله بلافاصله سوییچ را دنبال کند (به‌جای تأخیرِ ۳۰msِ hwtest_loop).
void hwtest_apply_outputs(void);

// چسباندن دکمه‌ی «تست سخت‌افزار» روی یک صفحه‌ی Config (parent) با فونت دلخواه
void hwtest_attach_button(lv_obj_t *parent, const lv_font_t *font);

// ورود به محیط تست (با رمز 0000 از صفحه‌ی رمز فراخوانی می‌شود)
void hwtest_enter_ui(void);

// دو بوق کوتاه و سریع بدون هنگ یا نگه‌داشتن بازر
void hwtest_two_short_beeps(void);

// توابع کنترل افزاینده آی‌سی MCP23017 (۶ خروجی GPA0..GPA5 و ۲ ورودی GPA7, GPA6)
void hwtest_mcp23017_set_output(uint8_t out_idx, bool on);
bool hwtest_mcp23017_get_output(uint8_t out_idx);
uint8_t hwtest_mcp23017_poll_inputs(void);

// چاپ تست HAFFLER روی پورت RS485 / Serial2 (با کنترل دوضرب شروع/توقف، تخلیه بافر و فید کاغذ)
void hwtest_print_haffler(void);
bool hwtest_is_printing(void);
void hwtest_stop_print(void);

// تست خودکار جامع خط تولید (Auto QC Batch Test)
void hwtest_start_auto_qc(void);
void hwtest_stop_auto_qc(void);
bool hwtest_is_auto_qc_running(void);
void hwtest_close_all_modals(void);

// کالیبراسیون و تنظیم آفست دمای ترموکوپل‌ها (NVS)
void hwtest_adjust_tc_offset(int tc_num, float delta);
float hwtest_get_tc_offset(int tc_num);

// کنترل بازر، بوق کلیک و ذخیره‌سازی ولوم در NVS
void hwtest_beep_click(uint32_t ms);
void hwtest_buzzer_silence(void);
void hwtest_save_buzzer_volume(int32_t val);
void storage_save_buzzer_volume(int32_t vol);
int32_t storage_get_buzzer_volume(void);

// دریافت و هدایت مستقیم به صفحات تست سخت‌افزار برای بازیابی خودکار پس از ریست
uint8_t hwtest_get_active_screen_id(void);
void hwtest_goto_screen_id(uint8_t screen_id);

// ساختار و توابع تست اولیه سلامت سخت‌افزار در بدو روشن شدن (Boot Self-Test)
typedef struct {
    bool has_error;
    bool rtc_comms_fail;
    bool rtc_battery_lost;
    bool touch_comms_fail;
    bool psram_fail;
    bool heap_low;
    bool cpu_overheat;
    float cpu_temp;
    char summary_fa[384];
} HwBootDiagResult;

extern HwBootDiagResult g_boot_diag;
extern bool g_boot_diag_dismissed;

void hwtest_run_boot_self_check(void);
void hwtest_show_boot_alert_if_needed(lv_obj_t *parent_screen);

// تله‌متری و کنترل بلادرنگ جهت وب‌پورتال و مانیتورینگ بیسیم
bool hwtest_get_relay_state(uint8_t relay_idx);
void hwtest_pulse_relay(uint8_t relay_idx, uint32_t duration_ms);
void hwtest_set_relay_state(uint8_t relay_idx, bool on);
void hwtest_kill_all_outputs(void);

float hwtest_get_tc1_temp(void);
float hwtest_get_tc2_temp(void);
bool  hwtest_get_tc1_valid(void);
bool  hwtest_get_tc2_valid(void);
float hwtest_get_esp_temp(void);

void hwtest_get_rtc_info(char *dt_str, size_t max_len, float *temp_c);
uint8_t hwtest_get_opto_inputs(void);
void hwtest_get_opto_voltages(int *v);
uint16_t hwtest_get_panel_keys(void);

#ifdef __cplusplus
}
#endif

#endif // HWTEST_H
