#include "project_config.h"
#include "bus_handles.h"
#include "lcd_ili9488.h"
#include <time.h>
#include "gt911.h"
#include "lvgl_port.h"
#include "ui/ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "buzzer.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "wifi.h"
#include "storage.h"
#include "stdbool.h"
#include "ui/screens.h"
#include "ui/ui_lang.h"
#include "lvgl.h"
#include "i2c_bus.h"
#include "actions.h"
#include "ds3231.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "hal/gpio_hal.h"
#include "timer.h"
#include "ui_refresh.h"
#include "adc.h"
#include "styles.h"
#include "fault_log.h"
#include "extra_modes.h"
#include "day_lock_core.h"
#include "serial_hmi.h"
#include "max6675.h"
#include "ui_resume.h"
#include "ota_update.h"
#include "panel_nav.h"
#include "boot_diag.h"
#include "esp_ota_ops.h"
#include "cloud_client.h"
#include "analog_inputs.h"
#include "machine_advisor.h"
#include "warranty.h"

static const char *TAG = "plc";

int32_t paddle_time, make_time, ice_time, ds_year, ds_month, ds_day, ds_hour, ds_minute,
    comp3phase_time, comp1phase_time, current_set_1, current_set_2, icecream_counter, adc1, adc2;
int32_t icecream_counter_daily = 0, icecream_counter_total = 0;
int32_t s_comp_cooldown_left_s = 0;
int32_t s_freeze_progress_pct = 0;
int32_t s_wash_countdown_s = 0;
int32_t paddle_timer_1 = 15, paddle_timer_2 = 15, startup_timer = MAKE_COMP_DELAY_S, mid_eye_timer = INDUCTIVE_TIMEOUT_S;
bool s_pulley_en = true;
uint8_t s_eye_fault_strike_cnt = 0;
int32_t restart_time_1 = HOLD_BIG_PERIOD_S, restart_time_2 = HOLD_COMP_DELAY_S, modbus_delay = 1;
int32_t current_set_1a = 430, current_set_1b = 0, current_set_2a = 430, current_set_2b = 0;
int32_t screensaver_timeout_s = DEFAULT_SS_TIMEOUT;
int32_t cone_price = DEFAULT_CONE_PRICE;
int32_t dispense_comp_run_s = DISPENSE_COMP_RUN_S;
int32_t front_hold_comp_s = FRONT_HOLD_COMP_S;
int32_t scoops_for_comp = SCOOPS_FOR_COMP;
int32_t max_freeze_timeout_m = (MAX_FREEZE_TIMEOUT_S / 60);
int32_t dispense_lever_max_s = DISPENSE_LEVER_MAX_S;
bool night_scheduler_en = false;
int32_t night_start_h = DEFAULT_NIGHT_SH;
int32_t night_start_m = DEFAULT_NIGHT_SM;
int32_t night_end_h = DEFAULT_NIGHT_EH;
int32_t night_end_m = DEFAULT_NIGHT_EM;
int32_t maint_gasket_scoops = 0;
int32_t maint_condenser_days = 0;
bool s_drain_mode_active = false;
int32_t s_drain_timer_s = 0;
/* متغیرهای کنترل مشترک بین Core 0 (PLC scan) و Core 1 (LVGL callbacks) — volatile ضروری جهت عدم بهینه‌سازی کامپایلر */
volatile bool paddle_start, make_start, ice_start, ice_time_off, make_time_off;
volatile bool paddle_start_act, make_start_act, ice_start_act;
volatile bool paddle_start_b, make_start_b, ice_start_b;
jalali_time_t jt;

spi_device_handle_t g_spi_lcd = NULL;
bool g_boot_f5_ota_requested = false;

int32_t paddle_counter = 0;
static int32_t make_counter = 0;
static int32_t ice_counter = 0;
int32_t make_seq_counter = 0;
static int32_t ice_seq_counter = 0;

typedef enum {
    PLC_PHASE_IDLE = 0,
    PLC_PHASE_PRODUCE,
    PLC_PHASE_ENDING,
    PLC_PHASE_READY,
    PLC_PHASE_DISPENSE,
    PLC_PHASE_RECHILL,
} plc_phase_t;

static plc_phase_t s_phase = PLC_PHASE_IDLE;
static int32_t s_ending_timer = 0;
static int32_t s_rechill_timer = 0;
static int32_t s_session_scoops = 0;
static int32_t s_front_hold_ms = 0;
static bool s_front_hold_triggered = false;
static plc_trigger_t s_last_start_trigger = TRIGGER_NONE;
static plc_stop_trigger_t s_last_stop_trigger = STOP_TRIGGER_NONE;
static int32_t s_prod_autocheck_timer_s = 0; /* تایمر شمارش معکوس/صعودی بررسی دوره‌ای سفتی بستنی در مد نگهداری/آماده */
static int32_t s_paddle_refresh_timer = 0;   /* تایمر کارکرد ۱۰ ثانیه‌ای پارو در زمان سفت بودن بستنی */

static bool s_front_prev = false;
static bool s_rear_prev = false;
static volatile bool s_front_edge = false;
static volatile bool s_rear_edge = false;

#if INDUCTIVE_ENABLED
static int s_eye_prev = -1;
static int32_t s_eye_ms_since_pulse = 0;
static int32_t s_motor_on_ms = 0;
static bool s_fault_eye = false;
static uint32_t s_pulley_pulses_seen = 0;
#endif

#if LOAD_CTRL_ENABLED
static int32_t s_load_motor_fault_ms = 0;
static int32_t s_load_comp_fault_ms = 0;
static bool s_fault_load_motor = false;
static bool s_fault_load_comp = false;
static int32_t s_reset_pulse_left_ms = 0;
static int32_t s_post_reset_check_ms = 0;  /* تأخیر معوق بررسی خطا پس از ریست */
#endif

static uint32_t s_nvs_lazy_flush_deadline_ms = 0;
static bool s_fault_freeze_timeout = false;
static bool s_fault_stuck_lever = false;
static bool s_fault_stuck_rear = false;
static volatile uint8_t s_interactive_fault_pending = 0;

#define BREADCRUMB_MAGIC 0xBC842109UL

typedef struct {
    uint32_t magic;
    uint32_t magic_inv;
    char action_fa[96];
    char action_en[64];
    char action_tr[64];
} rtc_breadcrumb_t;

RTC_NOINIT_ATTR static rtc_breadcrumb_t s_rtc_breadcrumb;

void breadcrumb_set(const char *fa, const char *en, const char *tr)
{
    s_rtc_breadcrumb.magic = BREADCRUMB_MAGIC;
    s_rtc_breadcrumb.magic_inv = ~BREADCRUMB_MAGIC;
    if (fa) {
        strncpy(s_rtc_breadcrumb.action_fa, fa, sizeof(s_rtc_breadcrumb.action_fa) - 1);
        s_rtc_breadcrumb.action_fa[sizeof(s_rtc_breadcrumb.action_fa) - 1] = '\0';
    } else {
        s_rtc_breadcrumb.action_fa[0] = '\0';
    }
    if (en) {
        strncpy(s_rtc_breadcrumb.action_en, en, sizeof(s_rtc_breadcrumb.action_en) - 1);
        s_rtc_breadcrumb.action_en[sizeof(s_rtc_breadcrumb.action_en) - 1] = '\0';
    } else {
        s_rtc_breadcrumb.action_en[0] = '\0';
    }
    if (tr) {
        strncpy(s_rtc_breadcrumb.action_tr, tr, sizeof(s_rtc_breadcrumb.action_tr) - 1);
        s_rtc_breadcrumb.action_tr[sizeof(s_rtc_breadcrumb.action_tr) - 1] = '\0';
    } else {
        s_rtc_breadcrumb.action_tr[0] = '\0';
    }
}

static bool breadcrumb_get(char *buf, size_t max_len, ui_lang_t lang)
{
    if (!buf || max_len == 0) return false;
    if (s_rtc_breadcrumb.magic != BREADCRUMB_MAGIC || s_rtc_breadcrumb.magic_inv != ~BREADCRUMB_MAGIC) {
        return false;
    }
    const char *src = (lang == UI_LANG_FA) ? s_rtc_breadcrumb.action_fa :
                      ((lang == UI_LANG_TR) ? s_rtc_breadcrumb.action_tr : s_rtc_breadcrumb.action_en);
    if (src && src[0] != '\0') {
        strncpy(buf, src, max_len - 1);
        buf[max_len - 1] = '\0';
        return true;
    }
    return false;
}

static bool s_fault_msg_pending = false;
static const char *s_fault_msg = NULL;
static bool s_freeze_trip_is_thermal = false;

void nvs_request_lazy_flush(void)
{
    s_nvs_lazy_flush_deadline_ms = pdTICKS_TO_MS(xTaskGetTickCount()) + NVS_FLUSH_DEBOUNCE_MS;
}

static inline void set_big_compressor(bool on)
{
    if (on) {
        if (s_comp_cooldown_left_s > 0) {
            ESP_LOGW(TAG, "Big compressor requested ON but anti-short-cycle active (%ld s left)", (long)s_comp_cooldown_left_s);
            pin_write(Make_Icecream, 1);
            return;
        }
        pin_write(Make_Icecream, 0);
    } else {
        if (gpio_get_level(Make_Icecream) == 0) {
            s_comp_cooldown_left_s = COMP_ANTI_SHORT_CYCLE_S;
            ESP_LOGI(TAG, "Big compressor turned OFF -> %ds anti-short-cycle cooldown initiated", COMP_ANTI_SHORT_CYCLE_S);
        }
        pin_write(Make_Icecream, 1);
    }
}

void plc_trigger_fault_interactive(uint8_t code);

static void dispatch_interactive_fault(uint8_t code)
{
    if (objects.interactive_fault_modal && lv_obj_is_valid(objects.interactive_fault_modal)) {
        return; /* پنجره خطا هم‌اکنون باز است؛ جلوگیری از ایجاد لوپ مکرر، پر شدن حافظه و هنگ سیستم */
    }
    const char *title = NULL;
    const char *cause = NULL;
    const char *sol = NULL;
    static char s_dynamic_cause_buf[512];
    static char s_dynamic_sol_buf[1536];
    char last_act[64];
    ui_lang_t lang = ui_lang_get();
    bool has_crumb = breadcrumb_get(last_act, sizeof(last_act), lang);

    /* ثبت وقوع خطا و بررسی آستانه تکرار مکرر (Smart Fault Escalation) */
    machine_advisor_record_fault(code);
    if (machine_advisor_is_fault_escalated(code)) {
        if (machine_advisor_get_escalated_advice(code, &title, &cause, s_dynamic_sol_buf, sizeof(s_dynamic_sol_buf))) {
            sol = s_dynamic_sol_buf;
            show_interactive_fault_dialog(code, title, cause, sol);
            return;
        }
    }

    if (lang == UI_LANG_EN) {
        switch (code) {
            case FAULT_EMERGENCY_STOP:
                title = "Alarm E00: Milk Hopper Empty (Low Mix)";
                cause = "Milk mix level in the upper hopper is empty. Hopper cooling was halted to protect dry tank.";
                sol = "1. Refill liquid ice cream mix into upper hopper.\n2. Sensor resets automatically upon filling.\n3. Touch [Retry] to resume cooling.";
                break;
            case FAULT_INDUCTIVE_STALL:
                title = "Fault E01: Agitator Pulley Stall";
                cause = "Inductive pulse sensor detected no rotation while motor was running. Belt slippage or heavy freeze-up possible.";
                if (s_eye_fault_strike_cnt >= 3) {
                    sol = "1. Disconnect power and inspect belt tension.\n2. Use Wash mode if cylinder is frozen.\n3. Check sensor gap (2-3mm).\n4. [Notice] Pulley sensor faulted 3 times. You may disable the sensor using the button below.\n5. Touch [Retry] to resume.";
                } else {
                    sol = "1. Disconnect power and inspect belt tension.\n2. Use Wash mode if cylinder is frozen.\n3. Check sensor gap (2-3mm), then touch [Retry].";
                }
                break;
            case FAULT_MOTOR_OVERLOAD:
                title = "Fault E02: Mixer Motor Overload Trip";
                cause = "Motor current exceeded overload threshold due to high viscosity or mechanical resistance.";
                sol = "1. Check ice cream firmness and rear microswitch spring.\n2. Verify motor bearings and capacitor.\n3. Reset overload relay and touch [Retry].";
                break;
            case FAULT_COMP_OVERLOAD:
                title = "Fault E03: Compressor Overload Trip";
                cause = "Compressor current exceeded limit or line voltage dropped significantly.";
                sol = "1. Check mains voltage and clean condenser fins.\n2. Ensure fan airflow is clear.\n3. Wait 3 minutes for gas pressure equalization, then touch [Retry].";
                break;
            case FAULT_THERMO_TOP_DISCONNECT:
                title = "Fault E04: Hopper Sensor Disconnected (T1)";
                cause = "Mix hopper temperature sensor signal (Terminal T1) is disconnected, damaged, or M1 supply is loose.";
                sol = "1. Turn OFF power and check Hopper sensor wire on Terminal T1.\n2. Verify sensor ground/power wire on Terminal M1.\n3. Inspect sensor probe for physical damage.\n4. Touch [Retry] after verifying connections.";
                break;
            case FAULT_THERMO_CYL_DISCONNECT:
                title = "Fault E05: Cylinder Sensor Disconnected (T2)";
                cause = "Freezing cylinder temperature sensor signal (Terminal T2) is disconnected, damaged, or M1 supply is loose.";
                sol = "1. Turn OFF power and check Cylinder sensor wire on Terminal T2.\n2. Verify sensor ground/power wire on Terminal M1.\n3. Inspect sensor probe and cable for physical damage.\n4. Touch [Retry] after verifying connections.";
                break;
            case FAULT_TORQUE_PREDICT:
                title = "Alert E06: High Viscosity Torque Warning";
                cause = "Motor load approached safety torque limit.";
                sol = "1. Dispense ice cream to reduce cylinder load.\n2. Verify target freeze temperature setting.\n3. Touch [Retry] to proceed.";
                break;
            case FAULT_LEASE_EXPIRED:
                title = "Fault E07: Lease / Day-Lock Expired";
                cause = "The authorized operation period for this machine has expired.";
                sol = "1. Contact factory support to receive your renewal code.\n2. Enter the 5-digit unlock code in the lease dialog.\n3. The machine will unlock automatically upon verification.";
                break;
            case FAULT_STUCK_LEVER:
                title = "Fault E08: Dispense Lever Held Open";
                cause = "Front dispense microswitch remained active beyond safe time limit.";
                sol = "1. Release dispense handle and check return spring.\n2. Clean and lubricate front door plunger.\n3. Touch [Retry] when lever is resting closed.";
                break;
            case FAULT_STUCK_REAR:
                title = "Fault E09: Rear Microswitch Engaged at Start";
                cause = "Rear clutch microswitch was detected engaged before production started.";
                sol = "1. Adjust rear microswitch screw to ensure contact is open.\n2. Verify free rotation of shaft.\n3. Touch [Retry] when clutch is free.";
                break;
            case FAULT_FREEZE_TIMEOUT:
                if (s_freeze_trip_is_thermal) {
                    title = "Fault E10: Cylinder Critical Over-Freeze Trip";
                    cause = "Freezing cylinder temperature reached safety red line (-22C) on valid sensor. Compressors halted to protect beater shaft and gearbox.";
                    sol = "1. Verify liquid mix volume and temperature settings.\n2. Use Wash mode to soften if cylinder is frozen solid.\n3. Touch [Retry] to resume.";
                } else {
                    title = "Fault E10: Maximum Freezing Duration Timeout";
                    cause = "Production ran longer than maximum allowed time (15 min) without rear microswitch (firmness) trip. Process halted to prevent continuous dry run.";
                    sol = "1. Verify liquid mix level in hopper and ensure front door is closed.\n2. Inspect rear clutch microswitch adjustment and spring tension.\n3. Check refrigerant pressure and touch [Retry].";
                }
                break;
            case FAULT_BROWNOUT:
                title = "Fault E12: Mains Voltage Drop (Brownout)";
                cause = "Microcontroller DC supply dropped below safe operating threshold (2.8V).\nCause: Sudden line dip or heavy inrush current during compressor/motor start.";
                sol = "1. Verify mains supply voltage is above 205 VAC.\n2. Ensure power line is stable, then touch [Retry].";
                break;
            case FAULT_WATCHDOG_RESET:
                title = "Fault E13: CPU Hang / Watchdog Reset (Task WDT)";
                if (has_crumb) {
                    snprintf(s_dynamic_cause_buf, sizeof(s_dynamic_cause_buf),
                             "Processor task timeout detected.\nCause: Task hung in blocking loop during [%s]. Watchdog rebooted system safely.",
                             last_act);
                    cause = s_dynamic_cause_buf;
                } else {
                    cause = "Processor task timeout detected.\nCause: Task hung in blocking loop exceeding allowed limit. Watchdog rebooted system safely.";
                }
                sol = "1. System state has been safely restored.\n2. Touch [Retry] to resume production.";
                break;
            case FAULT_CORE_PANIC:
                title = "Fault E14: CPU Crash (Core Panic)";
                if (has_crumb) {
                    snprintf(s_dynamic_cause_buf, sizeof(s_dynamic_cause_buf),
                             "Hardware CPU core crash occurred.\nCause: Memory exception or invalid pointer access during [%s]. Outputs were safely isolated.",
                             last_act);
                    cause = s_dynamic_cause_buf;
                } else {
                    cause = "Hardware CPU core crash occurred.\nCause: Memory exception, invalid pointer access, or stack overflow. Outputs were safely isolated.";
                }
                sol = "1. If issue recurs, cycle main power switch.\n2. Touch [Retry] to continue.";
                break;
            case FAULT_MAX6675_TOP_MISSING:
                title = "Fault E19: Hopper Sensor Fault (T1)";
                cause = "Hopper temperature sensor on Terminal T1 did not respond. Wire may be disconnected, sensor probe damaged, or M1 supply loose.";
                sol = "1. Check sensor wiring on Terminal T1.\n2. Ensure sensor power/GND wire on Terminal M1 is secure.\n3. Check sensor cable for damage or breaks.\n4. Touch [Retry] to test.";
                break;
            case FAULT_MAX6675_CYL_MISSING:
                title = "Fault E20: Cylinder Sensor Fault (T2)";
                cause = "Cylinder temperature sensor on Terminal T2 did not respond. Wire may be disconnected, sensor probe damaged, or M1 supply loose.";
                sol = "1. Check sensor wiring on Terminal T2.\n2. Ensure sensor power/GND wire on Terminal M1 is secure.\n3. Check sensor cable for damage or breaks.\n4. Touch [Retry] to test.";
                break;
            default:
                title = "System Protection Alert";
                cause = "An abnormal operating condition was detected.";
                sol = "Inspect machine conditions and touch [Retry].";
                break;
        }
    } else if (lang == UI_LANG_TR) {
        switch (code) {
            case FAULT_EMERGENCY_STOP:
                title = "Alarm E00: Sut Tanki Bos (Dusuk Seviye)";
                cause = "Ust haznedeki sut seviyesi dustu. Kuru tank donmasini onlemek icin sogutma durduruldu.";
                sol = "1. Ust hazneye sivi dondurma karisimi ekleyin.\n2. Tank dolunca sensor otomatik duzelir.\n3. [Yeniden Dene] butonuna basin.";
                break;
            case FAULT_INDUCTIVE_STALL:
                title = "Hata E01: Kasnak Donus Durmasi";
                cause = "Motor calisirken sensor darbe algilamadi. Kayis kaymasi veya asiri donma olabilir.";
                if (s_eye_fault_strike_cnt >= 3) {
                    sol = "1. Gucu kesin ve kayis gerginligini kontrol edin.\n2. Silindir donmussa Yikama modunu acin.\n3. Sensor mesafesini kontrol edin.\n4. [Bilgi] Kasnak sensoru 3 kez hata verdi. Asagidaki butondan sensoru kapatabilirsiniz.\n5. [Yeniden Dene] basin.";
                } else {
                    sol = "1. Gucu kesin ve kayis gerginligini kontrol edin.\n2. Silindir donmussa Yikama modunu acin.\n3. Sensor mesafesini kontrol edip [Yeniden Dene] basin.";
                }
                break;
            case FAULT_MOTOR_OVERLOAD:
                title = "Hata E02: Karistirici Motor Asiri Yuk";
                cause = "Motor akimi yuksek kivam veya mekanik sikisma nedeniyle limiti asti.";
                sol = "1. Dondurma sertligini ve arka mikrosalter ayarini kontrol edin.\n2. Motor rulmanlarini ve kondansatoru inceleyin.\n3. Termik roleyi sifirlayip [Yeniden Dene] basin.";
                break;
            case FAULT_COMP_OVERLOAD:
                title = "Hata E03: Kompresor Asiri Yuk";
                cause = "Kompresor akimi asildi veya sebeke voltaji dustu.";
                sol = "1. Sebeke voltajini ve kondenser temizligini kontrol edin.\n2. Fan hava yolunun acik oldugundan emin olun.\n3. 3 dakika gaz dengesini bekleyip [Yeniden Dene] basin.";
                break;
            case FAULT_THERMO_TOP_DISCONNECT:
                title = "Hata E04: Sut Tanki Sensoru Kesik (T1)";
                cause = "Sut tanki sicaklik sensoru (Klemens T1) sinyali kesildi, koptu veya M1 beslemesi temassiz.";
                sol = "1. Klemens T1 ve M1 sensor kablo baglantilarini kontrol edin.\n2. Sensor probunun saglamligini kontrol edin.\n3. Baglantiyi saglayip [Yeniden Dene] basin.";
                break;
            case FAULT_THERMO_CYL_DISCONNECT:
                title = "Hata E05: Silindir Sensoru Kesik (T2)";
                cause = "Silindir sicaklik sensoru (Klemens T2) sinyali kesildi, koptu veya M1 beslemesi temassiz.";
                sol = "1. Klemens T2 ve M1 sensor kablo baglantilarini kontrol edin.\n2. Sensor probunun saglamligini kontrol edin.\n3. Baglantiyi tamamlayip [Yeniden Dene] basin.";
                break;
            case FAULT_TORQUE_PREDICT:
                title = "Uyari E06: Yuksek Tork ve Kivam Uyarisi";
                cause = "Motor yuku guvenlik tork sinirina yaklasti.";
                sol = "1. Kolu acarak bir miktar dondurma alin.\n2. Ayarlanan dondurma derecesini kontrol edin.\n3. [Yeniden Dene] butonuna basin.";
                break;
            case FAULT_LEASE_EXPIRED:
                title = "Hata E07: Kira / Taksit Suresi Doldu";
                cause = "Cihazin kullanim suresi sona erdi.";
                sol = "1. Kilit acma kodunu almak icin ASTIAJ destek ile iletisime gecin.\n2. 5 haneli kodu kilit penceresine girin.\n3. Kod onaylandiginda cihaz otomatik acilacaktir.";
                break;
            case FAULT_STUCK_LEVER:
                title = "Hata E08: Dondurma Kolu Acik Kaldi";
                cause = "On mikrosalter guvenli surenin uzerinde basili kaldi.";
                sol = "1. Dondurma kolunu birakin ve yayin geri donusunu kontrol edin.\n2. On kapi pistonunu yaglayin.\n3. Kol serbestken [Yeniden Dene] basin.";
                break;
            case FAULT_STUCK_REAR:
                title = "Hata E09: Arka Mikrosalter Basili";
                cause = "Uretim baslamadan once arka debriyaj mikrosalteri aktif tespit edildi.";
                sol = "1. Arka mikrosalter ayar vidasini gevsetin.\n2. Saftin serbest dondugunden emin olun.\n3. [Yeniden Dene] butonuna basin.";
                break;
            case FAULT_FREEZE_TIMEOUT:
                if (s_freeze_trip_is_thermal) {
                    title = "Hata E10: Silindir Asiri Donma Korumasi";
                    cause = "Silindir sicakligi gecerli sensor tarafindan -22C kritik sinirinda olculdu. Karistirici mili ve disli kutusunu korumak icin kompresor durduruldu.";
                    sol = "1. Sut karisimi miktarini ve sogutma ayarlarini kontrol edin.\n2. Silindir asiri donmussa Yikama modu ile yumusatin.\n3. [Yeniden Dene] butonuna basin.";
                } else {
                    title = "Hata E10: Maksimum Dondurma Suresi Asimi";
                    cause = "Uretim suresi maksimum limiti (15 dk) asti ancak arka mikrosalter (kivam algilayici) tetiklenmedi. Bos calismayi onlemek icin sistem durduruldu.";
                    sol = "1. Tanktaki sut karisimi miktarini ve on kapak contasini kontrol edin.\n2. Arka mikrosalter yay gerginligini ve ayar vidasini kontrol edin.\n3. Gaz basincini kontrol edip [Yeniden Dene] basin.";
                }
                break;
            case FAULT_BROWNOUT:
                title = "Hata E12: Dusuk Voltaj Kesintisi (Brownout)";
                cause = "Besleme voltaji calisma sinirinin (2.8V) altina dustu.\nNeden: Kompresor/motor kalkis anindaki asiri akim veya sebeke dalgalanmasi.";
                sol = "1. Sebeke voltajinin 205V uzerinde oldugunu kontrol edin.\n2. Voltaj stabil olunca [Yeniden Dene] basin.";
                break;
            case FAULT_WATCHDOG_RESET:
                title = "Hata E13: Islemci Donmasi ve Watchdog (WDT)";
                if (has_crumb) {
                    snprintf(s_dynamic_cause_buf, sizeof(s_dynamic_cause_buf),
                             "Islemci gorevi zaman asimina ugradi.\nNeden: [%s] sirasinda gorev kilitlendi ve sistem guvenle yeniden baslatildi.",
                             last_act);
                    cause = s_dynamic_cause_buf;
                } else {
                    cause = "Islemci gorevi zaman asimina ugradi.\nNeden: Bir gorev izin verilen sureden fazla kilitlendi ve sistem guvenle yeniden baslatildi.";
                }
                sol = "1. Sistem durumu basariyla geri yuklendi.\n2. [Yeniden Dene] basin.";
                break;
            case FAULT_CORE_PANIC:
                title = "Hata E14: Islemci Cokmesi (Core Panic)";
                if (has_crumb) {
                    snprintf(s_dynamic_cause_buf, sizeof(s_dynamic_cause_buf),
                             "Donanimsal islemci cekirdek cokmesi yakalandi.\nNeden: [%s] sirasinda bellek istisnasi veya gecersiz isaretci erisimi.",
                             last_act);
                    cause = s_dynamic_cause_buf;
                } else {
                    cause = "Donanimsal islemci cekirdek cokmesi yakalandi.\nNeden: Bellek istisnasi, gecersiz isaretci erisimi veya yigin tasmasi.";
                }
                sol = "1. Sorun tekrarlarsa ana salteri kapatip acin.\n2. [Yeniden Dene] basin.";
                break;
            case FAULT_MAX6675_TOP_MISSING:
                title = "Hata E19: Sut Tanki Sensoru Yok (T1)";
                cause = "Klemens T1 uzerindeki sut tanki sicaklik sensoru ile iletisim kurulamadi. Kablo kopuk, M1 hatti temassiz veya sensor arizali.";
                sol = "1. T1 klemens soketini ve M1 sensor besleme hattini kontrol edin.\n2. Sensor probunun hasarsiz oldugundan emin olun.\n3. [Yeniden Dene] basin.";
                break;
            case FAULT_MAX6675_CYL_MISSING:
                title = "Hata E20: Silindir Sensoru Yok (T2)";
                cause = "Klemens T2 uzerindeki silindir sicaklik sensoru ile iletisim kurulamadi. Kablo kopuk, M1 hatti temassiz veya sensor arizali.";
                sol = "1. T2 klemens soketini ve M1 sensor besleme hattini kontrol edin.\n2. Sensor probunun hasarsiz oldugundan emin olun.\n3. [Yeniden Dene] basin.";
                break;
            default:
                title = "Sistem Koruma Uyarisi";
                cause = "Anormal calisma durumu tespit edildi.";
                sol = "Cihazi kontrol edip [Yeniden Dene] butonuna basin.";
                break;
        }
    } else {
        /* UI_LANG_FA */
        switch (code) {
            case FAULT_EMERGENCY_STOP:
                title = "کد هشدار ۰۰: مخزن شیر خالی است (اتمام مایع بستنی)";
                cause = "سنسور سطح شیر در مخزن بن‌ماری فعال شده و مایع بستنی تمام شده است. جهت حفاظت از مدار تبرید و جلوگیری از یخ‌زدگی مخزن خشک، نگهدارنده شیر خاموش شد.";
                sol = "- مایع بستنی تازه به مخزن بالا (بن‌ماری) اضافه کنید.\n- پس از پر شدن مخزن، سنسور به صورت خودکار آزاد می‌شود.\n- سپس دکمه [تلاش مجدد] را لمس کنید تا سرمایش مجدداً فعال گردد.";
                break;
            case FAULT_INDUCTIVE_STALL:
                title = "کد خطای ۰۱: عدم گردش شفت پارو (توقف پولی پارو)";
                cause = "سنسور القایی پولی در حین کارکرد موتور پالسی دریافت نکرد. احتمال پاره شدن یا سرخوردن تسمه، قفل شدن پارو به دلیل انجماد سنگین، یا خرابی سنسور وجود دارد.";
                sol = "- برق دستگاه را قطع کرده و سلامت تسمه موتور را بررسی نمایید.\n- در صورت انجماد شدید سیلندر، حالت شستشو را فعال کنید تا یخ نرم شود.\n- فاصله سنسور القایی تا پولی (۲ تا ۳ میلی‌متر) را تنظیم کنید.\n- پس از بررسی، دکمه [تلاش مجدد] را لمس کنید.";
                break;
            case FAULT_MOTOR_OVERLOAD:
                title = "کد خطای ۰۲: اضافه بار کنترل‌بار موتور پارو";
                cause = "جریان مصرفی موتور اصلی فراتر از حد مجاز رله کنترل بار رفته است. این امر معمولا ناشی از سفتی بیش از حد مایع بستنی یا گیر مکانیکی است.";
                sol = "- سفتی بستنی و پیچ رگلاژ فنر میکروسوئیچ عقب را بررسی کنید.\n- از سلامت خازن راه‌انداز و بلبرینگ‌های موتور پارو اطمینان یابید.\n- پس از خنک شدن موتور، دکمه ریست کنترل بار را فشار داده و دکمه [تلاش مجدد] را بزنید.";
                break;
            case FAULT_COMP_OVERLOAD:
                title = "کد خطای ۰۳: اضافه بار کنترل بار کمپرسور";
                cause = "جریان مصرفی کمپرسور اصلی از حد مجاز فراتر رفته یا ولتاژ ورودی شبکه دچار افت شدید شده است.";
                sol = "- ولتاژ تغذیه برق شهر و کندانسور (رادیاتور خنک‌کننده) را تمیز کنید.\n- فن خنک‌کننده کندانسور را بررسی نمایید تا مسیر هوا باز باشد.\n- ۳ دقیقه صبر کنید تا فشار گاز متعادل شود، سپس دکمه [تلاش مجدد] را لمس کنید.";
                break;
            case FAULT_THERMO_TOP_DISCONNECT:
                title = "کد خطای ۰۴: قطعی سنسور دمای مخزن شیر (ترمینال T1)";
                cause = "سیگنال سنسور دمای مخزن شیر (ترمینال T1) دریافت نمی‌شود؛ ممکن است سیم ترمینال T1 قطع شده، تغذیه از ترمینال M1 شل باشد یا پراب سنسور آسیب دیده باشد.";
                sol = "۱. سیم‌بندی سنسور روی ترمینال خارجی T1 را بررسی و محکم نمایید.\n۲. اتصال سیم تغذیه/منفی مشترک سنسور به ترمینال M1 را کنترل کنید.\n۳. از سلامت فیزیکی پراب و عدم قطعی یا لهیدگی کابل اطمینان یابید.\n۴. پس از رفع عیب، دکمه [تلاش مجدد] را لمس کنید.";
                break;
            case FAULT_THERMO_CYL_DISCONNECT:
                title = "کد خطای ۰۵: قطعی سنسور دمای سیلندر انجماد (ترمینال T2)";
                cause = "سیگنال سنسور دمای سیلندر انجماد (ترمینال T2) قطع است؛ ممکن است سیم ترمینال T2 قطع شده، اتصال M1 شل باشد یا پراب سنسور آسیب دیده باشد.";
                sol = "۱. اتصالات سنسور روی ترمینال خارجی T2 را بررسی و محکم نمایید.\n۲. اتصال سیم تغذیه/منفی مشترک سنسور به ترمینال M1 را کنترل کنید.\n۳. از سلامت فیزیکی پراب و عدم قطعی کابل سیلندر اطمینان یابید.\n۴. پس از رفع قطعی، دکمه [تلاش مجدد] را لمس کنید.";
                break;
            case FAULT_TORQUE_PREDICT:
                title = "کد هشدار ۰۶: افزایش بار مکانیکی و غلظت سیلندر";
                cause = "بار مکانیکی موتور پارو به حد بحرانی نزدیک شده است.";
                sol = "- اهرم خروج را باز کرده و مقداری بستنی تخلیه کنید.\n- دمای تنظیمی انجماد را کنترل فرمایید.\n- دکمه [تلاش مجدد] را لمس نمایید.";
                break;
            case FAULT_LEASE_EXPIRED:
                title = "کد خطای ۰۷: انقضای اعتبار روزشمار / اقساط";
                cause = "مهلت زمانی مجاز استفاده از دستگاه به پایان رسیده است.";
                sol = "- با پشتیبانی شرکت آستیاج جهت دریافت کد تمدید تماس بگیرید.\n- کد ۵ رقمی را در پنجره قفل وارد کنید.\n- پس از تایید کد، دستگاه بلافاصله فعال می‌گردد.";
                break;
            case FAULT_STUCK_LEVER:
                title = "کد خطای ۰۸: فشرده ماندن اهرم خروج بستنی";
                cause = "میکروسوئیچ اهرم جلو بیش از ۴۵ ثانیه به طور مداوم فعال مانده است. احتمال گیر کردن اهرم یا عدم بازگشت فنر دسته وجود دارد.";
                sol = "- اهرم خروج بستنی را بالا داده و روان بودن فنر را بررسی کنید.\n- پیستون و واشرهای درب جلو را چرب و سرویس کنید.\n- سپس دکمه [تلاش مجدد] را فشار دهید.";
                break;
            case FAULT_STUCK_REAR:
                title = "کد خطای ۰۹: درگیر بودن میکروسوئیچ عقب پیش از استارت";
                cause = "پیش از استارت حالت تولید، میکروسوئیچ عقب در وضعیت تحریک قرار دارد. ممکن است اهرم مکانیکی گیر کرده یا فنر شکسته باشد.";
                sol = "۱. پیچ تنظیم میکروسوئیچ عقب را کمی شل کنید تا پلاتین آزاد شود.\n۲. از چرخش آزادانه شفت گیربکس در حالت خاموش مطمئن شوید.\n۳. پس از اطمینان از آزاد بودن میکروسوئیچ، دکمه [تلاش مجدد] را لمس کنید.";
                break;
            case FAULT_FREEZE_TIMEOUT:
                if (s_freeze_trip_is_thermal) {
                    title = "کد خطای ۱۰: حد بحرانی برودت سیلندر (انجماد بیش از حد)";
                    cause = "دمای سیلندر انجماد بر اساس قرائت سنسور معتبر به زیر ۲۲- درجه سانتی گراد رسیده است. جهت حفاظت از شفت پارو و گیربکس در برابر شکستگی، کمپرسور متوقف شد.";
                    sol = "۱. مقدار مایع بستنی در مخزن و تنظیمات برودتی را کنترل نمایید.\n۲. در صورت یخ زدگی شدید، با حالت شستشو سیلندر را نرم کنید.\n۳. پس از بررسی، دکمه [تلاش مجدد] را لمس کنید.";
                } else {
                    title = "کد خطای ۱۰: انقضای زمان بیشینه انجماد سیلندر";
                    cause = "فرآیند تولید بستنی بیش از زمان مجاز (۱۵ دقیقه) ادامه یافت اما میکروسوئیچ کلاچ عقب (سفتی بستنی) فعال نشد. جهت حفاظت از کمپرسور و موتور، فرآیند متوقف گردید.";
                    sol = "۱. مقدار مایع بستنی در مخزن و بسته بودن درب جلو را بررسی کنید.\n۲. رگلاژ فنر و اهرم میکروسوئیچ عقب مخزن را چک نمایید.\n۳. فشار گاز و کارکرد کمپرسور را بازبینی کرده و [تلاش مجدد] را بزنید.";
                }
                break;
            case FAULT_BROWNOUT:
                title = "کد خطای ۱۲: افت شدید ولتاژ برق ورودی (Brownout)";
                cause = "ولتاژ مدار کنترل به زیر ۲.۸ ولت افت کرده است.\nعلت: شوک جریانی ناشی از استارت کمپرسور/موتور، نوسان شدید شبکه برق یا ضعف منبع تغذیه ۲۴ ولت تابلو.";
                sol = "۱. ولتاژ برق ورودی تابلو را بررسی نمایید (حداقل ۲۰۵ ولت).\n۲. از تثبیت ولتاژ مطمئن شده و دکمه [تلاش مجدد] را لمس کنید.";
                break;
            case FAULT_WATCHDOG_RESET:
                title = "کد خطای ۱۳: هنگ پردازنده و تریپ واچ داگ (Task WDT)";
                if (has_crumb) {
                    snprintf(s_dynamic_cause_buf, sizeof(s_dynamic_cause_buf),
                             "پردازنده به دلیل وقفه زمانی دچار هنگ شده است.\nعلت رخداد: توقف در حلقه مسدودکننده در حین فرآیند: «%s».\nجهت حفظ کنترل، سیستم ریست امنیتی خودکار انجام داد.",
                             last_act);
                    cause = s_dynamic_cause_buf;
                } else {
                    cause = "پردازنده به دلیل وقفه زمانی دچار هنگ شده است.\nعلت رخداد: توقف یکی از تسک‌های پردازشی در حلقه مسدودکننده بیش از حد مجاز.\nجهت حفظ کنترل، سیستم ریست امنیتی خودکار انجام داد.";
                }
                sol = "- وضعیت پایدار بودن تغذیه و اتصالات را کنترل فرمایید.\n- دکمه [تلاش مجدد] را جهت از سرگیری تولید لمس کنید.";
                break;
            case FAULT_CORE_PANIC:
                title = "کد خطای ۱۴: کرش پردازنده (Core Panic)";
                if (has_crumb) {
                    snprintf(s_dynamic_cause_buf, sizeof(s_dynamic_cause_buf),
                             "کرش سخت‌افزاری در هسته پردازنده رخ داده است.\nعلت رخداد: بروز خطای استثنای حافظه در حین فرآیند: «%s».\nکلیه خروجی‌ها جهت حفاظت مکانیکی ایمن‌سازی شدند.",
                             last_act);
                    cause = s_dynamic_cause_buf;
                } else {
                    cause = "کرش سخت‌افزاری در هسته پردازنده رخ داده است.\nعلت رخداد: بروز خطای استثنای حافظه (دسترسی غیرمجاز حافظه، پوینتر خالی یا سرریز پشته).\nکلیه خروجی‌ها جهت حفاظت مکانیکی ایمن‌سازی شدند.";
                }
                sol = "- دکمه تلاش مجدد را جهت بازنشانی و ادامه لمس نمایید.\n- در صورت تکرار، برق اصلی دستگاه را یک‌بار قطع و وصل فرمایید.";
                break;
            case FAULT_MAX6675_TOP_MISSING:
                title = "کد خطای ۱۹: عدم شناسایی سنسور دمای مخزن شیر (ترمینال T1)";
                cause = "ارتباط با سنسور دمای مخزن شیر (ترمینال T1) برقرار نشد؛ ممکن است سیم سنسور روی ترمینال T1 قطع شده، تغذیه سنسور از ترمینال M1 متصل نباشد یا سنسور سوخته/آسیب دیده باشد.";
                sol = "۱. اتصال سیم سنسور مخزن روی ترمینال خارجی T1 را بررسی کنید.\n۲. اتصال سیم تغذیه/منفی سنسور به ترمینال M1 را چک و محکم فرمایید.\n۳. از سلامت فیزیکی کابل سنسور و پراب آن اطمینان حاصل کنید.\n۴. پس از رفع عیب، دکمه [تلاش مجدد] را لمس نمایید.";
                break;
            case FAULT_MAX6675_CYL_MISSING:
                title = "کد خطای ۲۰: عدم شناسایی سنسور دمای سیلندر (ترمینال T2)";
                cause = "ارتباط با سنسور دمای سیلندر انجماد (ترمینال T2) برقرار نشد؛ ممکن است سیم سنسور روی ترمینال T2 قطع شده، تغذیه سنسور از ترمینال M1 متصل نباشد یا سنسور سوخته/آسیب دیده باشد.";
                sol = "۱. اتصال سیم سنسور سیلندر روی ترمینال خارجی T2 را بررسی کنید.\n۲. اتصال سیم تغذیه/منفی سنسور به ترمینال M1 را چک و محکم فرمایید.\n۳. از سلامت فیزیکی کابل سنسور و پراب آن اطمینان حاصل کنید.\n۴. پس از رفع عیب، دکمه [تلاش مجدد] را لمس نمایید.";
                break;
            default:
                title = "خطای حفاظتی سیستم";
                cause = "وضعیت غیرعادی در ورودی ها یا بار الکتریکی دستگاه شناسایی گردید.";
                sol = "دستگاه را بررسی نموده و پس از رفع علت خطا، دکمه [تلاش مجدد] را بزنید.";
                break;
        }
    }

    show_interactive_fault_dialog(code, title, cause, sol);
}

void plc_trigger_fault_interactive(uint8_t code)
{
    dispatch_interactive_fault(code);
}

static void raise_fault_msg(const char *msg);
static void bump_scoop_counter(void);
static void plc_outputs_all_off(void);

/* ================================================================
 * شبیه‌سازی ورودی سنسوری از طریق سریال (بدون نیاز به سخت‌افزار)
 * نام‌های معتبر: "front", "rear", "eye", "stop", "start", "load_m", "load_c"
 * ================================================================ */
void plc_sim_inject_input(const char *name, bool active)
{
    if (!name) return;

    if (strcasecmp(name, "rear") == 0) {
        analog_inputs_sim_pulse(TERM_I2, active);
        /* شبیه‌سازی تریپ میکروسوئیچ عقب (سفت شدن بستنی در تولید یا شارژ سرما) */
        if (active) {
            s_rear_edge = true;
            ESP_LOGI(TAG, "[SIM] Rear microswitch edge injected (phase=%d)", (int)s_phase);
        } else {
            s_rear_edge = false;
            ESP_LOGI(TAG, "[SIM] Rear microswitch released");
        }
    } else if (strcasecmp(name, "front") == 0 || strcasecmp(name, "scoop") == 0) {
        analog_inputs_sim_pulse(TERM_I4, active);
        /* شبیه‌سازی کشیدن اهرم جلو (یک قیف) */
        if (active) {
            s_front_edge = true;
            bump_scoop_counter();
            ESP_LOGI(TAG, "[SIM] Front lever / scoop edge injected");
        } else {
            s_front_edge = false;
        }
    } else if (strcasecmp(name, "stop") == 0) {
        analog_inputs_sim_pulse(TERM_I6, active);
        /* شبیه‌سازی استپ اضطراری */
        if (active) {
            plc_stop_all();
            fault_log_record(FAULT_EMERGENCY_STOP, 0);
            raise_fault_msg("[SIM] توقف اضطراری شبیه‌سازی شد");
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_EMERGENCY_STOP;
            ESP_LOGI(TAG, "[SIM] Emergency STOP injected");
        }
    } else if (strcasecmp(name, "start") == 0) {
        analog_inputs_sim_pulse(TERM_I7, active);
        /* شبیه‌سازی استارت دکمه فیزیکی */
        if (active) {
            plc_hw_start_make();
            ESP_LOGI(TAG, "[SIM] Start button injected -> plc_hw_start_make");
        }
    } else if (strcasecmp(name, "eye") == 0) {
        analog_inputs_sim_pulse(TERM_I0, active);
        /* شبیه‌سازی پالس سنسور القایی پولی */
        if (active) {
#if INDUCTIVE_ENABLED
            s_eye_ms_since_pulse = 0; /* ریست تایمر پالس */
            s_pulley_pulses_seen++;
            ESP_LOGI(TAG, "[SIM] Inductive sensor pulse injected (total=%lu)", (unsigned long)s_pulley_pulses_seen);
#endif
        }
    } else if (strcasecmp(name, "load_m") == 0) {
        /* شبیه‌سازی تریپ کنترل‌بار موتور */
        if (active) {
#if LOAD_CTRL_ENABLED
            s_fault_load_motor = true;
            raise_fault_msg("خطای کنترل‌بار موتور — ریست ۲ ثانیه");
            fault_log_record(FAULT_MOTOR_OVERLOAD, 0);
            machine_advisor_record_overload(FAULT_MOTOR_OVERLOAD);
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_MOTOR_OVERLOAD;
            ESP_LOGI(TAG, "[SIM] Motor load trip injected");
#endif
        }
    } else if (strcasecmp(name, "load_c") == 0) {
        /* شبیه‌سازی تریپ کنترل‌بار کمپرسور */
        if (active) {
#if LOAD_CTRL_ENABLED
            s_fault_load_comp = true;
            raise_fault_msg("خطای کنترل‌بار کمپرسور — ریست ۲ ثانیه");
            fault_log_record(FAULT_COMP_OVERLOAD, 0);
            machine_advisor_record_overload(FAULT_COMP_OVERLOAD);
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_COMP_OVERLOAD;
            ESP_LOGI(TAG, "[SIM] Comp load trip injected");
#endif
        }
    } else if (strcasecmp(name, "lever_stuck") == 0 || strcasecmp(name, "front_stuck") == 0) {
        if (active) {
            const int32_t lever_limit_s = (dispense_lever_max_s >= 15 && dispense_lever_max_s <= 120) ? dispense_lever_max_s : DISPENSE_LEVER_MAX_S;
            s_front_hold_ms = (lever_limit_s * 1000) + 1000;
            s_fault_stuck_lever = true;
            fault_log_record(FAULT_STUCK_LEVER, (int16_t)lever_limit_s);
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_STUCK_LEVER;
            ESP_LOGI(TAG, "[SIM] Lever stuck fault injected");
        }
    } else if (strcasecmp(name, "rear_stuck") == 0) {
        if (active) {
            s_fault_stuck_rear = true;
            fault_log_record(FAULT_STUCK_REAR, 0);
            s_interactive_fault_pending = FAULT_STUCK_REAR;
            buzzer_beep_alarm();
            ESP_LOGI(TAG, "[SIM] Rear micro stuck fault injected");
        }
    } else if (strcasecmp(name, "freeze_timeout") == 0) {
        if (active) {
            const int32_t freeze_limit_s = (max_freeze_timeout_m >= 5 && max_freeze_timeout_m <= 30) ? (max_freeze_timeout_m * 60) : MAX_FREEZE_TIMEOUT_S;
            make_seq_counter = freeze_limit_s + 10;
            s_fault_freeze_timeout = true;
            fault_log_record(FAULT_FREEZE_TIMEOUT, (int16_t)(freeze_limit_s / 60));
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_FREEZE_TIMEOUT;
            ESP_LOGI(TAG, "[SIM] Freeze timeout fault injected");
        }
    } else {
        ESP_LOGW(TAG, "[SIM] Unknown input name: '%s'", name);
    }
}

static void clear_phase_idle(void);
static void ui_status_on(lv_obj_t *status, lv_obj_t *btn);
static void ui_status_off(lv_obj_t *status, lv_obj_t *btn);
static void begin_make_ending(void);
static void enter_ready(void);
static bool try_start_rechill(void);

static int32_t inductive_timeout_ms(void)
{
    int32_t s = mid_eye_timer;
    if (s < 1 || s > 30) {
        s = INDUCTIVE_TIMEOUT_S;
    }
    return s * 1000;
}

static bool paddle_motor_should_run(void)
{
    /* شستشو منحصراً با پمپ Q7 کار می‌کند و ارتباطی با موتور پارو Q2 ندارد */
    if (s_drain_mode_active) {
        return true;
    }
    if (paddle_start) {
        return true;
    }
    if (make_start && s_phase == PLC_PHASE_PRODUCE) {
        return true;
    }
    if (s_phase == PLC_PHASE_ENDING || s_phase == PLC_PHASE_DISPENSE ||
        s_phase == PLC_PHASE_RECHILL) {
        return true;
    }
    if (s_paddle_refresh_timer > 0) {
        return true;
    }
    return false;
}

static bool load_input_tripped(gpio_num_t pin)
{
    if (pin == LOAD_MOTOR_GPIO) return load_motor_tripped();
    if (pin == LOAD_COMP_GPIO)  return load_comp_tripped();
    return gpio_get_level(pin) == LOAD_FAULT_LEVEL;
}

static void raise_fault_msg(const char *msg)
{
    s_fault_msg = msg;
    s_fault_msg_pending = true;
}

static void plc_outputs_all_off(void)
{
    clear_phase_idle();
    make_start = false;
    make_start_act = false;
    make_counter = 0;
    make_seq_counter = 0;
    paddle_start = false;
    paddle_counter = 0;
    ice_start = false;
    ice_start_act = false;
    ice_counter = 0;
    ice_seq_counter = 0;
    s_drain_mode_active = false;
    s_drain_timer_s = 0;
    pin_write(Paddle, 1);
    set_big_compressor(false);
    pin_write(Compressor, 1);
    extra_modes_stop_all();
    ui_status_off(objects.comp_status, objects.compressor_btn);
    ui_status_off(objects.paddle_status, objects.icecreem_btn);
    ui_status_off(objects.fridge_status, objects.fridge_btn);
    if (s_nvs_lazy_flush_deadline_ms > 0) {
        s_nvs_lazy_flush_deadline_ms = 0;
        app_state_save_to_nvs();
    }
}

static int32_t rear_paddle_off_limit_s(void)
{
    if (paddle_timer_2 >= 10 && paddle_timer_2 <= 20) {
        return paddle_timer_2;
    }
    return REAR_PADDLE_OFF_S;
}

static int32_t __attribute__((unused)) dispense_run_limit_s(void)
{
    if (dispense_comp_run_s >= 5 && dispense_comp_run_s <= 60) {
        return dispense_comp_run_s;
    }
    return DISPENSE_COMP_RUN_S;
}

/* HMI: تايمر شروع به کار → تأخیر کمپرسور بزرگ پس از پارو (۰..۱۰ ثانیه) */
int32_t make_comp_delay_s(void)
{
    if (startup_timer >= 0 && startup_timer <= 10) {
        return startup_timer;
    }
    return MAKE_COMP_DELAY_S;
}

/* HMI: RESTART TIME 2 یا کمپرسور نگهدارنده → تأخیر کمپرسور کوچک */
int32_t hold_small_delay_s(void)
{
    if (restart_time_2 >= 10 && restart_time_2 <= 3600) {
        return restart_time_2;
    }
    if (comp1phase_time >= 10 && comp1phase_time <= 3600) {
        return comp1phase_time;
    }
    return HOLD_COMP_DELAY_S;
}

/* HMI: RESTART TIME 1 → دوره روشن شدن کمپرسور بزرگ در نگهدارنده */
static int32_t __attribute__((unused)) hold_big_period_s(void)
{
    if (restart_time_1 >= 60) {
        return restart_time_1;
    }
    return HOLD_BIG_PERIOD_S;
}

/* HMI: کمپرسور اصلی → مدت روشن بودن کمپرسور بزرگ در هر دوره */
static int32_t __attribute__((unused)) hold_big_run_s(void)
{
    int32_t run = (comp3phase_time >= 10) ? comp3phase_time : HOLD_BIG_RUN_S;
    int32_t period = hold_big_period_s();
    if (run >= period) {
        run = period / 2;
        if (run < 1) {
            run = 1;
        }
    }
    return run;
}

static void ui_status_on(lv_obj_t *status, lv_obj_t *btn)
{
    if (btn && lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        return; /* قبلاً روشن بوده — جلوگیری از Redraw و لگ مکرر در هر ثانیه */
    }
    if (lvgl_lock(pdMS_TO_TICKS(30))) {
        ui_style_status_label(status, true);
        if (btn) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        }
        lvgl_unlock();
    }
}

static void ui_status_off(lv_obj_t *status, lv_obj_t *btn)
{
    if (btn && !lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        return; /* قبلاً خاموش بوده — جلوگیری از Redraw و لگ مکرر در هر ثانیه */
    }
    if (lvgl_lock(pdMS_TO_TICKS(30))) {
        ui_style_status_label(status, false);
        if (btn) {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
        lvgl_unlock();
    }
}

static void enter_ready(void)
{
    s_phase = PLC_PHASE_READY;
    s_ending_timer = 0;
    s_rechill_timer = 0;
    s_session_scoops = 0;
    s_front_hold_ms = 0;
    s_front_hold_triggered = false;
    s_front_edge = false;
    s_rear_edge = false;

    s_prod_autocheck_timer_s = 0; /* شروع زمان‌سنجی دوره چک خودکار نگهداری از صفر */
    s_paddle_refresh_timer = 0;

    make_start = true; /* آماده‌فروش: فرآیند تولید فعال و در فاز فروش باقی می‌ماند */
    make_start_act = false;
    make_counter = 0;
    make_seq_counter = 0;

    pin_write(Paddle, 1);
    set_big_compressor(false);
    pin_write(Compressor, 1);

    if (lvgl_lock(pdMS_TO_TICKS(30))) {
        ui_status_off(objects.fridge_status, objects.fridge_btn);
        ui_status_off(objects.paddle_status, objects.icecreem_btn);
        if (objects.comp_status) {
            ui_style_status_ready(objects.comp_status);
        }
        if (objects.compressor_btn) {
            lv_obj_add_state(objects.compressor_btn, LV_STATE_CHECKED);
        }
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "phase -> READY (autocheck timer reset)");
    app_state_save_to_nvs();
    ui_apply_state();
}

static void clear_phase_idle(void)
{
    s_phase = PLC_PHASE_IDLE;
    s_ending_timer = 0;
    s_rechill_timer = 0;
    s_session_scoops = 0;
    s_front_hold_ms = 0;
    s_front_edge = false;
    s_rear_edge = false;
    s_prod_autocheck_timer_s = 0;
    s_paddle_refresh_timer = 0;
}

bool plc_is_ready(void)
{
    return s_phase == PLC_PHASE_READY;
}

bool plc_is_ending(void)
{
    return s_phase == PLC_PHASE_ENDING;
}

int32_t plc_get_ending_rem_s(void)
{
    if (s_phase != PLC_PHASE_ENDING) return 0;
    int32_t rem = rear_paddle_off_limit_s() - s_ending_timer;
    return (rem > 0) ? rem : 0;
}

bool plc_is_dispense(void)
{
    return s_phase == PLC_PHASE_DISPENSE || s_phase == PLC_PHASE_RECHILL;
}

bool plc_is_rechill(void)
{
    return s_phase == PLC_PHASE_RECHILL;
}

int32_t plc_get_autocheck_timer_s(void)
{
    return s_prod_autocheck_timer_s;
}

int32_t plc_get_paddle_refresh_timer_s(void)
{
    return s_paddle_refresh_timer;
}

bool plc_is_idle(void)
{
    return (s_phase == PLC_PHASE_IDLE && !make_start && !ice_start && !paddle_start && !wash_is_active());
}

int32_t plc_get_session_scoops(void)
{
    return s_session_scoops;
}

int32_t plc_get_rechill_timer(void)
{
    return s_rechill_timer;
}

int32_t plc_get_scoops_target(void)
{
    return (scoops_for_comp >= 1 && scoops_for_comp <= 10) ? scoops_for_comp : SCOOPS_FOR_COMP;
}

void plc_reset_session_scoops(void)
{
    s_session_scoops = 0;
}

plc_trigger_t plc_get_start_trigger(void)
{
    return s_last_start_trigger;
}

plc_stop_trigger_t plc_get_stop_trigger(void)
{
    return s_last_stop_trigger;
}

void plc_set_start_trigger(plc_trigger_t trig)
{
    s_last_start_trigger = trig;
}

void plc_set_stop_trigger(plc_stop_trigger_t trig)
{
    s_last_stop_trigger = trig;
}

bool plc_is_fault(void)
{
    bool f = false;
#if INDUCTIVE_ENABLED
    f = f || (s_pulley_en && s_fault_eye);
#endif
#if LOAD_CTRL_ENABLED
    f = f || s_fault_load_motor || s_fault_load_comp;
#endif
    f = f || s_fault_freeze_timeout || s_fault_stuck_lever || s_fault_stuck_rear;
    return f;
}

uint8_t plc_get_active_fault_code(void)
{
#if LOAD_CTRL_ENABLED
    if (s_fault_load_motor) return (uint8_t)FAULT_MOTOR_OVERLOAD;
    if (s_fault_load_comp)  return (uint8_t)FAULT_COMP_OVERLOAD;
#endif
#if INDUCTIVE_ENABLED
    if (s_pulley_en && s_fault_eye) return (uint8_t)FAULT_INDUCTIVE_STALL;
#endif
    if (s_fault_freeze_timeout) return (uint8_t)FAULT_FREEZE_TIMEOUT;
    if (s_fault_stuck_lever)    return (uint8_t)FAULT_STUCK_LEVER;
    if (s_fault_stuck_rear)     return (uint8_t)FAULT_STUCK_REAR;
    if (stop_button_active())   return (uint8_t)FAULT_EMERGENCY_STOP;
    if (lease_is_locked() || day_lock_is_soft_locked()) return (uint8_t)FAULT_LEASE_EXPIRED;
    if (s_interactive_fault_pending != 0) return s_interactive_fault_pending;
    if (plc_is_fault() && fault_log_get_count() > 0) {
        fault_entry_t e;
        if (fault_log_get_entry(0, &e)) return e.code;
    }
    return 0;
}

bool plc_try_clear_fault(void)
{
    bool ok = true;

#if INDUCTIVE_ENABLED
    if (s_fault_eye) {
        s_fault_eye = false;
        s_motor_on_ms = 0;
        s_eye_ms_since_pulse = 0;
        s_pulley_pulses_seen = 0;
        ESP_LOGI(TAG, "eye fault cleared by operator reset");
    }
#endif

#if LOAD_CTRL_ENABLED
    if (s_fault_load_motor) {
        if (!load_input_tripped(LOAD_MOTOR_GPIO)) {
            s_fault_load_motor = false;
            s_load_motor_fault_ms = 0;
            ESP_LOGI(TAG, "motor load fault cleared");
        } else {
            ESP_LOGW(TAG, "motor load still tripped");
            ok = false;
        }
    }
    if (s_fault_load_comp) {
        if (!load_input_tripped(LOAD_COMP_GPIO)) {
            s_fault_load_comp = false;
            s_load_comp_fault_ms = 0;
            ESP_LOGI(TAG, "comp load fault cleared");
        } else {
            ESP_LOGW(TAG, "comp load still tripped");
            ok = false;
        }
    }
#endif

    if (s_fault_freeze_timeout) {
        s_fault_freeze_timeout = false;
        make_seq_counter = 0;
        ESP_LOGI(TAG, "freeze timeout fault cleared");
    }
    if (s_fault_stuck_lever) {
        if (!micro_front_active()) {
            s_fault_stuck_lever = false;
            s_front_hold_ms = 0;
            ESP_LOGI(TAG, "stuck lever fault cleared");
        } else {
            ESP_LOGW(TAG, "front lever still active/held");
            ok = false;
        }
    }
    if (s_fault_stuck_rear) {
        if (!micro_rear_active()) {
            s_fault_stuck_rear = false;
            ESP_LOGI(TAG, "stuck rear micro fault cleared");
        } else {
            ESP_LOGW(TAG, "rear micro still active/stuck");
            ok = false;
        }
    }

    if (ok && !plc_is_fault()) {
        s_fault_msg_pending = false;
        s_fault_msg = NULL;
        s_interactive_fault_pending = 0;
    }
    return ok && !plc_is_fault();
}

void plc_request_control_reset(void)
{
#if LOAD_CTRL_ENABLED
    /* دستور قطعی کارفرما: رله ریست مدار فرمان (Q6) منحصراً و فقط برای خطاهای اضافه‌بار مجاز است.
     * برای خطای پولی، تایم‌اوت انجماد، میکروسوئیچ‌ها و سایر بخش‌ها تحت هیچ عنوان نباید از ریست استفاده شود. */
    bool is_overload = s_fault_load_motor || s_fault_load_comp ||
                       load_input_tripped(LOAD_MOTOR_GPIO) || load_input_tripped(LOAD_COMP_GPIO);
    bool in_io_test = io_test_is_active();

    if (is_overload || in_io_test) {
        /* قطع لحظه‌ای برق کنترل از رله و وصل مجدد برای رفع تریپ کنترل‌بار */
        pin_write(RESET_RELAY_GPIO, 0); /* فعال کردن رله قطع */
        s_reset_pulse_left_ms = RESET_PULSE_MS;
        ESP_LOGI(TAG, "control-power reset pulse %d ms (overload=%d, io_test=%d)",
                 RESET_PULSE_MS, (int)is_overload, (int)in_io_test);
    } else {
        ESP_LOGI(TAG, "Control-power reset skipped: isolated strictly for overloads only (no pulley/other resets)");
    }
#endif
    (void)plc_try_clear_fault();
}

void plc_retry_current_fault(void)
{
    ESP_LOGI(TAG, "Operator requested fault retry");
#if LOAD_CTRL_ENABLED
    /* ثبت وقوع اضافه‌بار پیش از ریست پرچم‌ها */
    bool had_overload = (s_fault_load_motor || s_fault_load_comp ||
                         load_input_tripped(LOAD_MOTOR_GPIO) || load_input_tripped(LOAD_COMP_GPIO));
#endif
#if INDUCTIVE_ENABLED
    s_fault_eye = false;
    s_eye_ms_since_pulse = 0;
    s_motor_on_ms = 0;
    s_pulley_pulses_seen = 0;
#endif
#if LOAD_CTRL_ENABLED
    s_fault_load_motor = false;
    s_fault_load_comp = false;
    s_load_motor_fault_ms = 0;
    s_load_comp_fault_ms = 0;
    if (had_overload) {
        /* منحصراً در صورتی که خطا ناشی از اضافه‌بار بوده باشد پالس ریست اعمال می‌شود */
        pin_write(RESET_RELAY_GPIO, 0);
        s_reset_pulse_left_ms = RESET_PULSE_MS;
        ESP_LOGI(TAG, "Overload retry: control-power reset pulse %d ms", RESET_PULSE_MS);
    }
#endif
    s_fault_freeze_timeout = false;
    s_freeze_trip_is_thermal = false;
    make_seq_counter = 0;
    make_counter = 0;
    if (s_phase == PLC_PHASE_PRODUCE) {
        make_start = false;
        make_start_act = false;
        s_phase = PLC_PHASE_IDLE;
    }
    s_fault_stuck_lever = false;
    s_fault_stuck_rear = false;
    s_fault_msg_pending = false;
    s_fault_msg = NULL;
    s_interactive_fault_pending = 0;

    max6675_reprobe();
    hide_interactive_fault_dialog();
    buzzer_beep_confirm();
    ui_request_apply();
}

void plc_dismiss_current_fault(void)
{
    ESP_LOGI(TAG, "Operator dismissed fault -> stopping all & clearing faults");
    plc_stop_all();
#if INDUCTIVE_ENABLED
    s_fault_eye = false;
    s_eye_ms_since_pulse = 0;
    s_motor_on_ms = 0;
    s_pulley_pulses_seen = 0;
#endif
#if LOAD_CTRL_ENABLED
    s_fault_load_motor = false;
    s_fault_load_comp = false;
    s_load_motor_fault_ms = 0;
    s_load_comp_fault_ms = 0;
#endif
    s_fault_freeze_timeout = false;
    s_freeze_trip_is_thermal = false;
    make_seq_counter = 0;
    make_counter = 0;
    s_fault_stuck_lever = false;
    s_fault_stuck_rear = false;
    s_fault_msg_pending = false;
    s_fault_msg = NULL;
    s_interactive_fault_pending = 0;

    hide_interactive_fault_dialog();
    buzzer_beep_confirm();
    ui_request_apply();
}

void plc_poll_inputs(void)
{
    breadcrumb_set("اسکن ورودی‌های دیجیتال و موتور پارو", "Digital Inputs & Mixer Control", "Dijital Girisler ve Mikser");

    /* نمونه‌برداری آنالوگ تقسیم مقاومتی و ورودی‌های دیجیتال دیبانس‌شده */
    analog_inputs_poll();

    /* ۱. نظارت بلادرنگ بر سنسور خالی بودن مخزن شیر با فیلتر ضد اسپایک ۸۰ms */
    if (stop_button_active()) {
        /* حفاظت قطعی: قطع بلادرنگ کمپرسور کوچک نگهدارنده مخزن شیر جهت جلوگیری از یخ‌زدگی مخزن خشک */
        pin_write(Compressor, 1);

        if (screensaver_is_active()) {
            hide_screensaver();
        }
        screensaver_notify_activity();
        if (make_start || paddle_start || ice_start || s_phase != PLC_PHASE_IDLE ||
            wash_is_active() || edge_is_active() || rainbow_is_active()) {
            plc_stop_all();
            fault_log_record(FAULT_EMERGENCY_STOP, 0);
            raise_fault_msg("مخزن شیر خالی است: لطفا مایع اضافه کنید");
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_EMERGENCY_STOP;
            return;
        }
    }
    if (start_button_active()) {
        if (screensaver_is_active()) {
            hide_screensaver();
        }
        screensaver_notify_activity();
    }

    const bool front = micro_front_active();
    const bool rear = micro_rear_active();

    if (front && !s_front_prev) {
        s_front_edge = true;
        if (screensaver_is_active()) {
            hide_screensaver();
        }
        screensaver_notify_activity();
    }

    /* میکرو عقب (کلاچ) — analog_inputs_poll قبلاً Schmitt ON=2×70ms=140ms دیبانس کرده.
     * لبه‌یابی مستقیم بدون دیبانس اضافه. */
    if (rear && !s_rear_prev) {
        s_rear_edge = true;
        s_rear_prev = true;
    } else if (!rear) {
        s_rear_prev = false;
    }

    if (front) {
        s_front_hold_ms += MICRO_POLL_MS;
        const int32_t hold_target_s = (front_hold_comp_s >= 2 && front_hold_comp_s <= 15) ? front_hold_comp_s : FRONT_HOLD_COMP_S;
        if (s_front_hold_ms >= (hold_target_s * 1000)) {
            s_front_hold_triggered = true;
        }
        const int32_t lever_limit_s = (dispense_lever_max_s >= 15 && dispense_lever_max_s <= 120) ? dispense_lever_max_s : DISPENSE_LEVER_MAX_S;
        if (s_front_hold_ms > (lever_limit_s * 1000) && !s_fault_stuck_lever && !s_drain_mode_active) {
            s_fault_stuck_lever = true;
            fault_log_record(FAULT_STUCK_LEVER, (int16_t)lever_limit_s);
            ESP_LOGE(TAG, "Dispense lever stuck open > %ld s!", (long)lever_limit_s);
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_STUCK_LEVER;
        }
    } else {
        s_front_hold_ms = 0;
    }

    s_front_prev = front;

#if INDUCTIVE_ENABLED
    const int eye = inductive_active() ? 1 : 0;
    if (s_eye_prev < 0) {
        s_eye_prev = eye;
        s_eye_ms_since_pulse = 0;
    } else if (eye != s_eye_prev) {
        s_eye_prev = eye;
        s_eye_ms_since_pulse = 0;
        s_pulley_pulses_seen++;
    } else {
        s_eye_ms_since_pulse += MICRO_POLL_MS;
        if (s_eye_ms_since_pulse > 60000) {
            s_eye_ms_since_pulse = 60000;
        }
    }

    const bool motor_is_active = !io_test_is_active() && !wash_is_active() && (paddle_motor_should_run() || gpio_get_level(Paddle) == 0);
    if (s_pulley_en && !machine_advisor_is_pulley_bypassed() && motor_is_active && !s_fault_eye) {
        s_motor_on_ms += MICRO_POLL_MS;
        const int32_t limit_ms = inductive_timeout_ms();
        static bool s_torque_warn_logged = false;
        /* مهلت اولیه راه‌اندازی منطبق با زمان تنظیمی کاربر (limit_ms) */
        if (s_motor_on_ms >= limit_ms) {
            if (s_eye_ms_since_pulse >= limit_ms || s_pulley_pulses_seen == 0) {
                s_fault_eye = true;
                s_eye_fault_strike_cnt++;
                raise_fault_msg("خطای چشم وسط: گردش پولی قطع شد");
                fault_log_record(FAULT_INDUCTIVE_STALL, (int16_t)s_eye_ms_since_pulse);
                ESP_LOGE(TAG, "INDUCTIVE FAULT #%u: no pulse for %ld ms (pulses=%lu)",
                         (unsigned)s_eye_fault_strike_cnt, (long)s_eye_ms_since_pulse, (unsigned long)s_pulley_pulses_seen);
                plc_outputs_all_off();
                buzzer_beep_alarm();
                s_interactive_fault_pending = FAULT_INDUCTIVE_STALL;
            } else if (s_eye_ms_since_pulse >= (limit_ms * 80 / 100)) {
                if (!s_torque_warn_logged) {
                    s_torque_warn_logged = true;
                    fault_log_record(FAULT_TORQUE_PREDICT, (int16_t)s_eye_ms_since_pulse);
                    ESP_LOGW(TAG, "TORQUE PREDICT WARNING: slowdown detected (%ld ms)", (long)s_eye_ms_since_pulse);
                }
            } else if (s_eye_ms_since_pulse < (limit_ms * 50 / 100)) {
                s_torque_warn_logged = false;
                if (s_pulley_pulses_seen > 15) {
                    s_eye_fault_strike_cnt = 0; /* کارکرد سالم و بدون توقف پولی -> ریست خطا */
                    machine_advisor_clear_fault_strikes(FAULT_INDUCTIVE_STALL);
                }
            }
        }
    } else if (!motor_is_active) {
        s_motor_on_ms = 0;
        s_eye_ms_since_pulse = 0;
        s_pulley_pulses_seen = 0;
    }
#endif

#if LOAD_CTRL_ENABLED
    if (s_reset_pulse_left_ms > 0) {
        s_reset_pulse_left_ms -= MICRO_POLL_MS;
        if (s_reset_pulse_left_ms <= 0) {
            s_reset_pulse_left_ms = 0;
            pin_write(RESET_RELAY_GPIO, 1); /* وصل مجدد برق کنترل */
            ESP_LOGI(TAG, "control-power restored");
            /* بعد از ریست: ۲۰۰ms فرصت برای stable شدن ورودی (۳ تیک بعدی) */
            s_post_reset_check_ms = 200;
        }
    }

    /* بررسی معوق خطای باقیمانده پس از ریست (بدون vTaskDelay) */
    if (s_post_reset_check_ms > 0) {
        s_post_reset_check_ms -= MICRO_POLL_MS;
        if (s_post_reset_check_ms <= 0) {
            s_post_reset_check_ms = 0;
            bool still_motor = s_fault_load_motor && load_input_tripped(LOAD_MOTOR_GPIO);
            bool still_comp  = s_fault_load_comp  && load_input_tripped(LOAD_COMP_GPIO);

            if (!still_motor && s_fault_load_motor) {
                s_fault_load_motor = false;
                s_load_motor_fault_ms = 0;
                ESP_LOGI(TAG, "Motor load fault cleared after control reset");
            } else if (still_motor) {
                ESP_LOGE(TAG, "Motor load fault STILL ACTIVE after reset!");
                raise_fault_msg("خطای کنترل‌بار موتور پس از ریست باقی ماند — دستگاه را خاموش/روشن کنید");
                s_interactive_fault_pending = FAULT_MOTOR_OVERLOAD;
                buzzer_beep_alarm();
            }

            if (!still_comp && s_fault_load_comp) {
                s_fault_load_comp = false;
                s_load_comp_fault_ms = 0;
                ESP_LOGI(TAG, "Comp load fault cleared after control reset");
            } else if (still_comp) {
                ESP_LOGE(TAG, "Comp load fault STILL ACTIVE after reset!");
                raise_fault_msg("خطای کنترل‌بار کمپرسور پس از ریست باقی ماند — دستگاه را خاموش/روشن کنید");
                s_interactive_fault_pending = FAULT_COMP_OVERLOAD;
                buzzer_beep_alarm();
            }

            if (!plc_is_fault()) {
                s_fault_msg_pending = false;
                s_fault_msg = NULL;
                s_interactive_fault_pending = 0;
            }
        }
    }

    bool in_io_test = io_test_is_active() || (ui_get_settings_tab() == 4 && ui_get_screen_id() == SCREEN_ID_SETTINGS_PG);
    if (!in_io_test) {
        if (!s_fault_load_motor) {
            if (load_input_tripped(LOAD_MOTOR_GPIO)) {
                s_load_motor_fault_ms += MICRO_POLL_MS;
                if (s_load_motor_fault_ms >= LOAD_DEBOUNCE_MS) {
                    s_fault_load_motor = true;
                    raise_fault_msg("خطای کنترل‌بار موتور — ریست ۲ ثانیه");
                    fault_log_record(FAULT_MOTOR_OVERLOAD, (int16_t)analog_inputs_get_raw_counts(1));
                    machine_advisor_record_overload(FAULT_MOTOR_OVERLOAD);
                    ESP_LOGE(TAG, "MOTOR LOAD FAULT -> shutdown outputs and activate 2s reset");
                    plc_outputs_all_off();
                    plc_request_control_reset();
                    buzzer_beep_alarm();
                    s_interactive_fault_pending = FAULT_MOTOR_OVERLOAD;
                }
            } else {
                s_load_motor_fault_ms = 0;
            }
        }

        if (!s_fault_load_comp) {
            if (load_input_tripped(LOAD_COMP_GPIO)) {
                s_load_comp_fault_ms += MICRO_POLL_MS;
                if (s_load_comp_fault_ms >= LOAD_DEBOUNCE_MS) {
                    s_fault_load_comp = true;
                    raise_fault_msg("خطای کنترل‌بار کمپرسور — ریست ۲ ثانیه");
                    fault_log_record(FAULT_COMP_OVERLOAD, (int16_t)analog_inputs_get_raw_counts(2));
                    machine_advisor_record_overload(FAULT_COMP_OVERLOAD);
                    ESP_LOGE(TAG, "COMP LOAD FAULT -> shutdown outputs and activate 2s reset");
                    plc_outputs_all_off();
                    plc_request_control_reset();
                    buzzer_beep_alarm();
                    s_interactive_fault_pending = FAULT_COMP_OVERLOAD;
                }
            } else {
                s_load_comp_fault_ms = 0;
            }
        }
    } else {
        s_load_motor_fault_ms = 0;
        s_load_comp_fault_ms = 0;
    }
#endif

    /* Fast-path control response (<70 ms) */
    if (plc_is_fault()) {
        return;
    }

    /* فاز ۲: پایان تولید بلافاصله با لبه میکرو عقب */
    if (s_phase == PLC_PHASE_PRODUCE && make_start) {
        if (s_rear_edge) {
            s_rear_edge = false;
            begin_make_ending();
            return;
        }
    }

    /* قفل ایمنی گارانتی: ممانعت از کارکرد اهرم تخلیه و پارو در صورت انقضای مهلت تست */
    if (warranty_is_expired_and_locked()) {
        if (s_front_edge) {
            s_front_edge = false;
            buzzer_beep_alarm();
            extern void open_warranty_expired_dialog(void);
            if (lvgl_lock(pdMS_TO_TICKS(50))) {
                open_warranty_expired_dialog();
                lvgl_unlock();
            }
        }
        return;
    }

    /* فاز ۲: شروع فروش بلافاصله با کشیدن اهرم جلویی */
    if (s_phase == PLC_PHASE_READY) {
        if (s_paddle_refresh_timer > 0) {
            pin_write(Paddle, 0);
        }
        if (s_front_edge) {
            s_front_edge = false;
            s_phase = PLC_PHASE_DISPENSE;
            pin_write(Paddle, 0);
            bump_scoop_counter();
            if (try_start_rechill()) {
                return;
            }
        } else {
            /* اگر شرایط شارژ برودت برقرار است و میکرو عقب قطع است، کمپرسور روشن شود */
            try_start_rechill();
        }
    } else if (s_phase == PLC_PHASE_DISPENSE) {
        if (front) {
            pin_write(Paddle, 0);
            if (try_start_rechill()) {
                return;
            }
        } else {
            /* رها شدن اهرم: در صورت فعال بودن چرخه بازرسی یا تازه‌سازی ۱۰ ثانیه‌ای، پارو روشن می‌ماند */
            s_front_edge = false;
            s_phase = PLC_PHASE_READY;
            ui_apply_state();
            if (try_start_rechill()) {
                return;
            }
            if (s_paddle_refresh_timer > 0) {
                pin_write(Paddle, 0);
            } else {
                pin_write(Paddle, 1);
            }
        }
    } else if (s_phase == PLC_PHASE_RECHILL) {
        /* قطع سریع ریچیل در صورت سفت شدن بستنی و تریپ میکرو عقب (<70ms) مشابه فاز تولید */
        if (s_rear_edge || rear) {
            s_rear_edge = false;
            s_session_scoops = 0;
            s_front_hold_ms = 0;
            s_front_hold_triggered = false;
            s_front_edge = false;
            begin_make_ending();
            return;
        } else if (s_front_edge) {
            s_front_edge = false;
            bump_scoop_counter();
        }
    } else if (s_front_edge) {
        /* در سایر وضعیت‌ها (مانند آیدل کارگاهی)، کشیدن اهرم شمارنده را ثبت و بوق می‌زند */
        s_front_edge = false;
        bump_scoop_counter();
    }
}

void plc_notify_paddle_started(void)
{
    clear_phase_idle();
    paddle_counter = 0;
#if INDUCTIVE_ENABLED
    s_pulley_pulses_seen = 0;
    s_motor_on_ms = 0;
    s_eye_ms_since_pulse = 0;
    s_fault_eye = false;
#endif
}

void plc_notify_make_started(void)
{
    s_phase = PLC_PHASE_PRODUCE;
    machine_advisor_notify_freeze_start();
    s_ending_timer = 0;
    s_rechill_timer = 0;
    s_session_scoops = 0;
    s_front_edge = false;
    s_rear_edge = false;
    s_prod_autocheck_timer_s = 0;
    s_paddle_refresh_timer = 0;
    make_counter = 0;
    make_seq_counter = 0;
#if INDUCTIVE_ENABLED
    s_pulley_pulses_seen = 0;
    s_motor_on_ms = 0;
    s_eye_ms_since_pulse = 0;
#endif
}

void plc_notify_ice_started(void)
{
    clear_phase_idle();
    ice_counter = 0;
    ice_seq_counter = 0;
}

void plc_notify_mode_cleared(void)
{
    clear_phase_idle();
    pin_write(Paddle, 1);
    set_big_compressor(false);
    pin_write(Compressor, 1);
    ice_start = false;
    ice_start_act = false;
    ice_counter = 0;
    ice_seq_counter = 0;
    make_start = false;
    make_start_act = false;
    make_counter = 0;
    make_seq_counter = 0;
    paddle_start = false;
    paddle_counter = 0;
}

void plc_set_big_compressor(bool on)
{
    set_big_compressor(on);
}

/* هنگامی که از plc_stop_all فراخوانی می‌شود، توابع داخلی NVS نمی‌نویسند */
static bool s_batch_stop_active = false;

static void stop_paddle(void)
{
    paddle_start = false;
    paddle_counter = 0;
    pin_write(Paddle, 1);
    ui_status_off(objects.paddle_status, objects.icecreem_btn);
    if (!s_batch_stop_active) {
        app_state_save_to_nvs();
        ui_apply_state();
    }
}

static void stop_make(void)
{
    make_start = false;
    make_start_act = false;
    make_counter = 0;
    make_seq_counter = 0;
    if (s_phase == PLC_PHASE_PRODUCE || s_phase == PLC_PHASE_ENDING) {
        clear_phase_idle();
    }
    pin_write(Paddle, 1);
    set_big_compressor(false);
    pin_write(Compressor, 1);
    ui_status_off(objects.comp_status, objects.compressor_btn);
    ui_status_off(objects.paddle_status, objects.icecreem_btn);
    ui_status_off(objects.fridge_status, objects.fridge_btn);
    if (!s_batch_stop_active) {
        app_state_save_to_nvs();
        ui_apply_state();
    }
}

static void stop_ice(void)
{
    ice_start = false;
    ice_start_act = false;
    ice_counter = 0;
    ice_seq_counter = 0;
    pin_write(Compressor, 1);
    set_big_compressor(false);
    ui_status_off(objects.fridge_status, objects.fridge_btn);
    ui_status_off(objects.comp_status, objects.compressor_btn);
    if (!s_batch_stop_active) {
        app_state_save_to_nvs();
        ui_apply_state();
    }
}

void plc_stop_all(void)
{
    bool was_active = (make_start || paddle_start || ice_start ||
                       make_start_b || paddle_start_b || ice_start_b ||
                       s_phase != PLC_PHASE_IDLE ||
                       wash_is_active() || edge_is_active() || rainbow_is_active() ||
                       s_drain_mode_active);
    s_last_stop_trigger = STOP_TRIGGER_USER_STOP;
    s_batch_stop_active = true;
    clear_phase_idle();
    stop_make();
    stop_paddle();
    stop_ice();
    s_batch_stop_active = false;
    extra_modes_stop_all();
    io_test_turn_off_all_relays();

    make_start_b = false;
    ice_start_b = false;
    paddle_start_b = false;
    ui_status_off(objects.comp_status_b, objects.compressor_btn_b);
    ui_status_off(objects.fridge_status_b, objects.fridge_btn_b);
    ui_status_off(objects.paddle_status_b, objects.icecreem_btn_b);
#if INDUCTIVE_ENABLED
    s_motor_on_ms = 0;
#endif
    if (s_drain_mode_active) {
        drain_mode_stop();
    }
    /* یک write تجمیع‌یافته — به‌جای ۳ write مجزا */
    s_nvs_lazy_flush_deadline_ms = 0;
    app_state_save_to_nvs();
    ui_apply_state();

    if (was_active && !plc_is_fault() && s_interactive_fault_pending == 0) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_info((lang == UI_LANG_FA) ? "توقف کامل: کلیه فرآیندها با کلید استپ متوقف شدند" :
                             ((lang == UI_LANG_TR) ? "Tam Durdurma: Tum islemler Stop tusuyla durduruldu" :
                              "Full Stop: All operations halted by Stop key"));
        buzzer_beep_click();
    }
}

void drain_mode_start(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked() || plc_is_fault() || warranty_is_expired_and_locked()) {
        return;
    }
    if (wash_is_active()) {
        extra_modes_stop_all();
    }
    clear_phase_idle();
    stop_make();
    stop_ice();
    stop_paddle();
#if INDUCTIVE_ENABLED
    s_pulley_pulses_seen = 0;
    s_motor_on_ms = 0;
    s_eye_ms_since_pulse = 0;
    s_fault_eye = false;
#endif
    s_drain_mode_active = true;
    s_drain_timer_s = DRAIN_TIMEOUT_S;
    pin_write(Paddle, 0);
    ui_status_on(objects.paddle_status, objects.icecreem_btn);
    buzzer_beep_confirm();
    ESP_LOGI(TAG, "Continuous Drain Mode started (%d s)", DRAIN_TIMEOUT_S);
    ui_apply_state();
}

void drain_mode_stop(void)
{
    if (!s_drain_mode_active) {
        return;
    }
    s_drain_mode_active = false;
    s_drain_timer_s = 0;
    pin_write(Paddle, 1);
    ui_status_off(objects.paddle_status, objects.icecreem_btn);
    ESP_LOGI(TAG, "Continuous Drain Mode stopped");
    ui_apply_state();
}

bool drain_mode_is_active(void)
{
    return s_drain_mode_active;
}

void maint_gasket_reset(void)
{
    maint_gasket_scoops = 0;
    storage_set_i32(KEY_MAINT_GSK, 0);
    buzzer_beep_confirm();
    ESP_LOGI(TAG, "Maintenance gasket counter reset to 0");
}

void maint_condenser_reset(void)
{
    maint_condenser_days = 0;
    storage_set_i32(KEY_MAINT_CND, 0);
    buzzer_beep_confirm();
    ESP_LOGI(TAG, "Maintenance condenser days reset to 0");
}

bool plc_is_running_production(void)
{
    return (make_start || paddle_start || ice_start || wash_is_active() ||
            gpio_get_level(Compressor) == 0 ||
            gpio_get_level(Make_Icecream) == 0 ||
            gpio_get_level(Paddle) == 0);
}

void plc_hw_start_make(void)
{
    if (lease_is_locked() || day_lock_is_soft_locked()) {
        panel_nav_toast_error("روزشمار منقضی - دستگاه قفل است");
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
    if (machine_advisor_is_overload_locked()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "قفل حفاظتی اضافه‌بار — کد چالش لازم است" :
                              ((lang == UI_LANG_TR) ? "Asiri yuk kilidi — Guvenlik kodu gerekli" :
                               "Overload Lock — Challenge code required"));
        buzzer_beep_alarm();
        return;
    }
    if (machine_advisor_is_cooldown_active()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_warning((lang == UI_LANG_FA) ? "سیم‌پیچ داغ — زمان استراحت حرارتی موتور" :
                                ((lang == UI_LANG_TR) ? "Motor sicak — Termal soguma suresi aktif" :
                                 "Hot Winding — Thermal cooldown active"));
        buzzer_beep_alarm();
        return;
    }
    if (hopper_empty_active()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "مخزن شیر خالی است — شارژ مایع بستنی" :
                              ((lang == UI_LANG_TR) ? "Sut haznesi bos — Dondurma karisimi ekleyin" :
                               "Milk hopper empty — Refill ice cream mix"));
        buzzer_beep_alarm();
        return;
    }
    if (cylinder_freeze_protection_trip()) {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "سیلندر منجمد است (۲۲- درجه) — شروع مجاز نیست" :
                              ((lang == UI_LANG_TR) ? "Silindir asiri donuk (-22C) — Baslatma engellendi" :
                               "Cylinder frozen solid (-22C) — Start prohibited"));
        buzzer_beep_alarm();
        return;
    }
    if (wash_is_active()) {
        panel_nav_toast_warning("شستشو روشن است - تولید مجاز نیست");
        buzzer_beep_alarm();
        return;
    }
    if (micro_rear_active()) {
        ESP_LOGE(TAG, "Make start blocked: rear micro already active!");
        s_fault_stuck_rear = true;
        fault_log_record(FAULT_STUCK_REAR, 0);
        s_interactive_fault_pending = FAULT_STUCK_REAR;
        buzzer_beep_alarm();
        return;
    }
    if (plc_is_fault()) {
        panel_nav_toast_error("خطای حفاظتی - STOP برای ریست");
        buzzer_beep_alarm();
        return;
    }
    if (make_start || ice_start || paddle_start) {
        return;
    }
    if (s_phase == PLC_PHASE_ENDING || s_phase == PLC_PHASE_DISPENSE ||
        s_phase == PLC_PHASE_RECHILL) {
        return;
    }
    s_last_start_trigger = TRIGGER_PANEL_KEY;
    make_start = true;
    plc_notify_make_started();
    ui_status_on(objects.comp_status, objects.compressor_btn);
    pin_write(Paddle, 0);
    if (s_comp_cooldown_left_s > 0) {
        ui_lang_t lang = ui_lang_get();
        char cd_msg[128];
        snprintf(cd_msg, sizeof(cd_msg),
                 (lang == UI_LANG_FA) ? "پارو روشن شد — کمپرسور پس از تعادل گاز (%ld ثانیه) فعال می‌شود" :
                 ((lang == UI_LANG_TR) ? "Palet calisti — Kompresor %ld sn sonra gaz dengesiyle acilacak" :
                  "Paddle started — Compressor starts after %ld s gas cooldown"),
                 (long)s_comp_cooldown_left_s);
        panel_nav_toast_info(cd_msg);
    }
    app_state_save_to_nvs();
    ui_apply_state();
}

static void begin_make_ending(void)
{
    ESP_LOGI(TAG, "rear micro -> ENDING");
    s_last_stop_trigger = STOP_TRIGGER_REAR_CLUTCH;
    s_rear_edge = false;
    s_phase = PLC_PHASE_ENDING;
    s_ending_timer = 0;
    machine_advisor_notify_freeze_done();

    make_start = true; /* کلید تولید روی پنل همواره روشن و فعال می‌ماند */
    make_start_act = false;
    make_seq_counter = 0;
    make_counter = 0;

    set_big_compressor(false);
    pin_write(Compressor, 1);
    pin_write(Paddle, 0);

    if (objects.comp_status) {
        ui_style_status_ready(objects.comp_status);
    }
    if (objects.compressor_btn) {
        lv_obj_add_state(objects.compressor_btn, LV_STATE_CHECKED);
    }
    buzz(120);
    app_state_save_to_nvs();
    ui_apply_state();
}

static void bump_scoop_counter(void)
{
    machine_advisor_notify_dispense();
    icecream_counter_daily++;
    icecream_counter_total++;
    maint_gasket_scoops++;
    icecream_counter = icecream_counter_daily;
    s_session_scoops++;
    ESP_LOGI(TAG, "scoop=%ld daily=%ld total=%ld gsk=%ld",
             (long)s_session_scoops, (long)icecream_counter_daily,
             (long)icecream_counter_total, (long)maint_gasket_scoops);
    buzzer_beep_click();
    nvs_request_lazy_flush();
    ui_update_icecream_counter();
    ui_apply_state();
}

static void begin_rechill(void)
{
    const bool rear_firm = micro_rear_active() || s_rear_edge;
    const bool too_cold = cylinder_freeze_protection_trip();
    const bool overload = s_fault_load_motor || s_fault_load_comp;

    if (rear_firm || too_cold || overload) {
        ESP_LOGW(TAG, "rechill blocked by interlock: rear=%d cold=%d ovl=%d -> 10s paddle refresh only",
                 rear_firm, too_cold, overload);
        s_paddle_refresh_timer = 10;
        pin_write(Paddle, 0);
        set_big_compressor(false);
        pin_write(Compressor, 1);
        s_phase = PLC_PHASE_READY;
        s_prod_autocheck_timer_s = 0;
        ui_apply_state();
        return;
    }

    ESP_LOGI(TAG, "RECHILL start (scoops=%ld hold_ms=%ld trig=%d)",
             (long)s_session_scoops, (long)s_front_hold_ms, (int)s_last_start_trigger);
    s_phase = PLC_PHASE_RECHILL;
    s_rechill_timer = 0;
    s_front_edge = false;
    s_session_scoops = 0;
    s_front_hold_ms = 0;
    s_paddle_refresh_timer = 0;

    /* استارت توالی مشابه فاز تولید: ابتدا پارو بلافاصله؛ کمپرسور بزرگ پس از make_comp_delay_s() */
    pin_write(Paddle, 0);
    set_big_compressor(false);
    pin_write(Compressor, 1);

    ui_status_on(objects.comp_status, objects.compressor_btn);
    ui_status_off(objects.paddle_status, objects.icecreem_btn);
    buzz(150);
    ui_apply_state();
}

static bool try_start_rechill(void)
{
    const int32_t scp_target = (scoops_for_comp >= 1 && scoops_for_comp <= 10) ? scoops_for_comp : SCOOPS_FOR_COMP;
    const int32_t hold_target_s = (front_hold_comp_s >= 2 && front_hold_comp_s <= 15) ? front_hold_comp_s : FRONT_HOLD_COMP_S;
    const bool scoops_ok = (s_session_scoops >= scp_target);
    const bool hold_ok = (s_front_hold_ms >= (hold_target_s * 1000)) || s_front_hold_triggered;
    if (!scoops_ok && !hold_ok) {
        return false;
    }

    const bool rear_firm = micro_rear_active() || s_rear_edge;
    const bool too_cold = cylinder_freeze_protection_trip();
    const bool overload = s_fault_load_motor || s_fault_load_comp;

    if (rear_firm || too_cold || overload) {
        /* اگر میکروسوئیچ عقب وصل است (بستنی سفت)، فقط ۱۰ ثانیه پارو بزند و زمان‌گیری مجدد از پایان این ۱۰ ثانیه شروع شود */
        s_paddle_refresh_timer = 10;
        pin_write(Paddle, 0);
        set_big_compressor(false);
        pin_write(Compressor, 1);
        s_session_scoops = 0;
        s_front_hold_ms = 0;
        s_front_hold_triggered = false;
        s_phase = PLC_PHASE_READY;
        s_prod_autocheck_timer_s = 0;
        ESP_LOGI(TAG, "Rechill threshold reached but rear micro firm -> 10s paddle refresh only");
        ui_apply_state();
        return true;
    }

    if (scoops_ok) {
        s_last_start_trigger = TRIGGER_SCOOPS_REACHED;
    } else if (hold_ok) {
        s_last_start_trigger = TRIGGER_FRONT_HELD;
    }
    s_front_hold_triggered = false;
    begin_rechill();
    return true;
}

/* فاز ۱ + فاز ۲: تولید / پایان / آماده / فروش + حفاظت چشم وسط */
static void one_hz_hook(void *ctx)
{
    (void)ctx;
    /* عدم اجرای نظارت‌های حرارتی، تراکنش‌های SPI و رایت NVS در حین بروزرسانی OTA */
    extern ota_install_state_t ota_get_install_state(void);
    ota_install_state_t ota_st = ota_get_install_state();
    if (ota_st == OTA_INST_CONNECTING || ota_st == OTA_INST_DOWNLOADING || ota_st == OTA_INST_VERIFYING) {
        return;
    }

    breadcrumb_set("پایش سنسورهای حرارتی و چرخه تولید", "Thermal Sensors & Machine State", "Sicaklik Sensorleri ve Uretim");

    static uint32_t s_stable_sec = 0;
    if (s_stable_sec < 15) {
        s_stable_sec++;
        if (s_stable_sec == 5) {
            ota_pre_erase_start();
        }
        if (s_stable_sec == 10) {
            ui_resume_mark_stable();
        }
    }

    max6675_poll();
    machine_advisor_tick_1s();
    machine_advisor_track_actuator_seconds(
        (gpio_get_level(Make_Icecream) == 0 || gpio_get_level(Compressor) == 0),
        (gpio_get_level(Paddle) == 0)
    );
    warranty_tick_1s();

    if (cylinder_freeze_protection_trip()) {
        if (gpio_get_level(Make_Icecream) == 0 || gpio_get_level(Compressor) == 0 || make_start || s_phase == PLC_PHASE_PRODUCE || s_phase == PLC_PHASE_RECHILL) {
            ESP_LOGE(TAG, "THERMAL RED LINE: Cylinder freeze protection tripped (temp <= %.1f C)! Shutting off all production.", CYLINDER_MIN_SAFE_TEMP_C);
            s_freeze_trip_is_thermal = true;
            s_fault_freeze_timeout = true;
            make_start = false;
            make_start_act = false;
            s_phase = PLC_PHASE_IDLE;
            make_seq_counter = 0;
            make_counter = 0;
            s_rechill_timer = 0;
            s_session_scoops = 0;
            s_front_hold_ms = 0;
            s_front_hold_triggered = false;
            s_front_edge = false;
            plc_outputs_all_off();
            buzzer_beep_alarm();
            fault_log_record(FAULT_FREEZE_TIMEOUT, (int16_t)(max6675_get_temp_cylinder() * 10));
            s_interactive_fault_pending = FAULT_FREEZE_TIMEOUT;
            ui_apply_state();
            return;
        }
    }

    if (s_comp_cooldown_left_s > 0) {
        s_comp_cooldown_left_s--;
    }

    if (s_drain_mode_active) {
        if (s_drain_timer_s > 0) {
            s_drain_timer_s--;
            if (s_drain_timer_s == 0) {
                ESP_LOGI(TAG, "Continuous Drain Mode completed timeout (%d s)", DRAIN_TIMEOUT_S);
                drain_mode_stop();
                buzz(300);
            }
        }
    }

    if (s_nvs_lazy_flush_deadline_ms > 0) {
        uint32_t now_ms = pdTICKS_TO_MS(xTaskGetTickCount());
        if (now_ms >= s_nvs_lazy_flush_deadline_ms) {
            s_nvs_lazy_flush_deadline_ms = 0;
            app_state_save_to_nvs();
            ESP_LOGI(TAG, "Lazy NVS flush executed");
        }
    }

    if (s_phase == PLC_PHASE_PRODUCE && make_start) {
        int32_t target_s = (make_time > 60) ? make_time : 360;
        int32_t progress = (make_seq_counter * 95) / target_s;
        if (progress > 95) progress = 95;
        s_freeze_progress_pct = progress;
    } else if (s_phase == PLC_PHASE_READY) {
        s_freeze_progress_pct = 100;
    } else if (s_phase == PLC_PHASE_ENDING) {
        s_freeze_progress_pct = 98;
    } else if (s_phase == PLC_PHASE_DISPENSE) {
        s_freeze_progress_pct = 92;
    } else if (s_phase == PLC_PHASE_RECHILL) {
        int32_t big_at = make_comp_delay_s();
        int32_t pct = 80;
        if (s_rechill_timer > big_at) {
            pct = 85 + ((s_rechill_timer - big_at) * 10) / 45;
            if (pct > 96) pct = 96;
        }
        s_freeze_progress_pct = pct;
    } else {
        s_freeze_progress_pct = 0;
    }

#if INDUCTIVE_ENABLED || LOAD_CTRL_ENABLED
    if (plc_is_fault()) {
        if (make_start || paddle_start || ice_start ||
            s_phase == PLC_PHASE_PRODUCE || s_phase == PLC_PHASE_ENDING ||
            s_phase == PLC_PHASE_READY || s_phase == PLC_PHASE_DISPENSE ||
            s_phase == PLC_PHASE_RECHILL) {
            plc_outputs_all_off();
#if INDUCTIVE_ENABLED
            s_motor_on_ms = 0;
#endif
            buzz(400);
            app_state_save_to_nvs();
            ui_apply_state();
        }
        if (s_interactive_fault_pending != 0) {
            uint8_t fault_code = s_interactive_fault_pending;
            s_interactive_fault_pending = 0;
            dispatch_interactive_fault(fault_code);
        }
        if (s_fault_msg_pending) {
            s_fault_msg_pending = false;
            panel_nav_toast_error(s_fault_msg ? s_fault_msg : "خطای حفاظتی دستگاه");
            buzzer_beep_alarm();
        }
        return;
    }
#endif

    /* --- فاز ۲: پایان تولید پس از میکرو عقب یا تایمر بیشینه انجماد --- */
    if (s_phase == PLC_PHASE_PRODUCE && make_start) {
        const int32_t freeze_limit_s = (max_freeze_timeout_m >= 5 && max_freeze_timeout_m <= 30) ? (max_freeze_timeout_m * 60) : MAX_FREEZE_TIMEOUT_S;
        if (make_seq_counter > freeze_limit_s) {
            ESP_LOGE(TAG, "Max freeze timeout (%ld s) exceeded without rear micro trip!", (long)freeze_limit_s);
            s_fault_freeze_timeout = true;
            s_freeze_trip_is_thermal = false;
            s_last_stop_trigger = STOP_TRIGGER_TIMEOUT;
            make_start = false;
            make_start_act = false;
            s_phase = PLC_PHASE_IDLE;
            make_seq_counter = 0;
            make_counter = 0;
            fault_log_record(FAULT_FREEZE_TIMEOUT, (int16_t)(freeze_limit_s / 60));
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_FREEZE_TIMEOUT;
            return;
        }
        if (s_rear_edge) {
            s_rear_edge = false; /* کلیر صریح پیش از فراخوانی — جلوگیری از تریگر دوباره */
            begin_make_ending();
        }
    }

    if (s_phase == PLC_PHASE_ENDING) {
        s_ending_timer++;
        pin_write(Paddle, 0);
        set_big_compressor(false);
        pin_write(Compressor, 1);
        if (s_ending_timer >= rear_paddle_off_limit_s()) {
            enter_ready();
        }
        return;
    }

    /* --- فاز ۲: فروش و نگهداری هوشمند (پایش دوره‌ای و شارژ مجدد) --- */
    if (s_phase == PLC_PHASE_READY) {
        /* اگر چرخه ۱۰ ثانیه‌ای چرخش تازه کردن پارو فعال است */
        if (s_paddle_refresh_timer > 0) {
            s_paddle_refresh_timer--;
            pin_write(Paddle, 0);
            set_big_compressor(false);
            pin_write(Compressor, 1);
            if (s_paddle_refresh_timer == 0) {
                pin_write(Paddle, 1);
                s_prod_autocheck_timer_s = 0; /* شروع مجدد زمان‌گیری ۱۰ دقیقه از پایان کارکرد پارو */
                s_session_scoops = 0;
                s_front_hold_ms = 0;
                s_front_hold_triggered = false;
                ESP_LOGI(TAG, "Paddle 10s refresh complete -> autocheck timer reset");
                ui_apply_state();
            }
            return;
        }

        /* زمان‌سنجی دوره چک خودکار سفتی بستنی (تنظیم در F3 - پیش‌فرض ۱۰ دقیقه) */
        s_prod_autocheck_timer_s++;
        const int32_t base_interval_s = (restart_time_1 >= 5 && restart_time_1 <= 3600) ? restart_time_1 : 600;
        int32_t check_interval_s = base_interval_s;
        if (machine_advisor_is_eco_idle()) {
            check_interval_s = (base_interval_s * 5) / 4; /* +25% در ساعات خلوتی جهت کاهش استهلاک کمپرسور */
        }
        if (s_prod_autocheck_timer_s >= check_interval_s) {
            s_prod_autocheck_timer_s = 0;
            const bool rear_firm = micro_rear_active() || s_rear_edge;
            const bool too_cold = cylinder_freeze_protection_trip();
            const bool overload = s_fault_load_motor || s_fault_load_comp;

            if (rear_firm || too_cold || overload) {
                /* بستنی سفت است یا شرایط اجازه استارت کمپرسور نمی‌دهد: فقط ۱۰ ثانیه پارو */
                s_paddle_refresh_timer = 10;
                pin_write(Paddle, 0);
                set_big_compressor(false);
                pin_write(Compressor, 1);
                s_session_scoops = 0;
                s_front_hold_ms = 0;
                ESP_LOGI(TAG, "Autocheck expired: rear micro firm -> 10s paddle refresh only");
                ui_apply_state();
                return;
            } else {
                /* بستنی شل شده است: استارت چرخه کامل سرمایش */
                s_last_start_trigger = TRIGGER_AUTOCHECK_EXPIRED;
                begin_rechill();
                return;
            }
        }

        if (s_front_edge) {
            s_front_edge = false;
            s_phase = PLC_PHASE_DISPENSE;
            pin_write(Paddle, 0);
            bump_scoop_counter();
            if (try_start_rechill()) {
                return;
            }
        }
        return;
    }

    if (s_phase == PLC_PHASE_DISPENSE) {
        if (micro_front_active()) {
            pin_write(Paddle, 0);
            if (try_start_rechill()) {
                return;
            }
        } else {
            /* رها شدن دسته */
            if (try_start_rechill()) {
                return;
            }
            if (s_paddle_refresh_timer > 0) {
                pin_write(Paddle, 0);
            } else {
                pin_write(Paddle, 1);
                s_phase = PLC_PHASE_READY;
            }
            s_front_edge = false;
            ui_apply_state();
        }
        return;
    }

    if (s_phase == PLC_PHASE_RECHILL) {
        /* حفاظت بیشینه زمان انجماد در شارژ برودت مانند فاز تولید */
        const int32_t freeze_limit_s = (max_freeze_timeout_m >= 5 && max_freeze_timeout_m <= 30) ? (max_freeze_timeout_m * 60) : MAX_FREEZE_TIMEOUT_S;
        if (s_rechill_timer > freeze_limit_s) {
            ESP_LOGE(TAG, "Max freeze timeout (%ld s) in rechill exceeded without rear micro trip!", (long)freeze_limit_s);
            s_fault_freeze_timeout = true;
            s_freeze_trip_is_thermal = false;
            s_phase = PLC_PHASE_IDLE;
            s_rechill_timer = 0;
            s_session_scoops = 0;
            s_front_hold_ms = 0;
            s_front_hold_triggered = false;
            s_front_edge = false;
            fault_log_record(FAULT_FREEZE_TIMEOUT, (int16_t)(freeze_limit_s / 60));
            plc_outputs_all_off();
            buzzer_beep_alarm();
            s_interactive_fault_pending = FAULT_FREEZE_TIMEOUT;
            return;
        }

        /* پایان شارژ برودت با سفت شدن بستنی و تریپ میکرو عقب: قطع فوری کمپرسور و چرخه پایانی پارو ۱۰ تا ۲۰ ثانیه */
        if (s_rear_edge || micro_rear_active()) {
            s_rear_edge = false;
            s_session_scoops = 0;
            s_front_hold_ms = 0;
            s_front_hold_triggered = false;
            s_front_edge = false;
            begin_make_ending();
            return;
        }

        s_rechill_timer++;
        pin_write(Paddle, 0);

        const int32_t big_at = make_comp_delay_s();

#if INDUCTIVE_ENABLED
        /* نظارت صلب صنعتی بر گردش پولی مستقل از تاخیر کمپرسور بزرگ */
        const int32_t ind_limit_ms = inductive_timeout_ms();
        if (s_pulley_en && !machine_advisor_is_pulley_bypassed() && (s_fault_eye || (s_motor_on_ms >= ind_limit_ms && s_pulley_pulses_seen == 0))) {
            s_fault_eye = true;
            raise_fault_msg("خطای چشم وسط: پولی گردش ندارد");
            fault_log_record(FAULT_INDUCTIVE_STALL, (int16_t)s_eye_ms_since_pulse);
            ESP_LOGE(TAG, "Rechill halted: inductive pulley stall (pulses=%lu)", (unsigned long)s_pulley_pulses_seen);
            s_phase = PLC_PHASE_IDLE;
            make_start = false;
            make_seq_counter = 0;
            s_rechill_timer = 0;
            plc_outputs_all_off();
            buzzer_beep_alarm();
            dispatch_interactive_fault(FAULT_INDUCTIVE_STALL);
            app_state_save_to_nvs();
            ui_apply_state();
            return;
        }
#endif

        if (s_rechill_timer > big_at) {
            set_big_compressor(true);
        } else {
            set_big_compressor(false);
        }
        return;
    }

    /* --- فاز ۱: پارو تکی --- */
    if (paddle_start) {
        paddle_counter++;
        pin_write(Paddle, 0);

        int32_t limit = (paddle_time > 0) ? paddle_time : PADDLE_DEFAULT_S;
        if (paddle_counter >= limit) {
            ESP_LOGI(TAG, "paddle timeout (%ld s)", (long)limit);
            buzz(250);
            stop_paddle();
        }
    } else {
        paddle_counter = 0;
    }

    /* --- فاز ۱: تولید --- */
    if (make_start && s_phase == PLC_PHASE_PRODUCE) {
        make_seq_counter++;
        pin_write(Paddle, 0);

        const int32_t big_at = make_comp_delay_s();
        const int32_t small_at = big_at + hold_small_delay_s();

#if INDUCTIVE_ENABLED
        /* نظارت صلب صنعتی: بررسی گردش پولی مستقل از تاخیر کمپرسور از ابتدای استارت پارو */
        const int32_t ind_limit_ms = inductive_timeout_ms();
        if (s_pulley_en && !machine_advisor_is_pulley_bypassed() && (s_fault_eye || (s_motor_on_ms >= ind_limit_ms && s_pulley_pulses_seen == 0))) {
            s_fault_eye = true;
            raise_fault_msg("خطای چشم وسط: پولی گردش ندارد");
            fault_log_record(FAULT_INDUCTIVE_STALL, (int16_t)s_eye_ms_since_pulse);
            ESP_LOGE(TAG, "Production halted: inductive pulley stall (pulses=%lu)", (unsigned long)s_pulley_pulses_seen);
            s_phase = PLC_PHASE_IDLE;
            make_start = false;
            make_seq_counter = 0;
            plc_outputs_all_off();
            buzzer_beep_alarm();
            dispatch_interactive_fault(FAULT_INDUCTIVE_STALL);
            app_state_save_to_nvs();
            ui_apply_state();
            return;
        }
#endif

        if (make_seq_counter > big_at) {
            if (s_comp_cooldown_left_s == 0) {
                set_big_compressor(true);
                make_start_act = true;
                ui_status_on(objects.comp_status, objects.compressor_btn);
            } else {
                set_big_compressor(false);
                make_start_act = false;
                ui_status_off(objects.comp_status, objects.compressor_btn);
            }
        }

        if (make_seq_counter > small_at) {
            if (hold_temp_allows_cool()) {
                pin_write(Compressor, 0);
            } else {
                pin_write(Compressor, 1);
            }
        }

        if (make_time > 0) {
            make_counter++;
            if (make_counter >= make_time) {
                ESP_LOGI(TAG, "make timeout (%ld s)", (long)make_time);
                buzz(250);
                stop_make();
            }
        }
    } else if (!paddle_start && s_phase == PLC_PHASE_IDLE) {
        make_counter = 0;
        make_seq_counter = 0;
    }

    /* --- فاز ۱: نگهدارنده (کوچک فوری با کنترل ترموستات مخزن شیر) --- */
    if (ice_start) {
        ice_seq_counter++;
        ice_counter++;

        /* کمپرسور کوچک مخزن شیر (Q0): در مد نگهدارنده بلافاصله با تایید ترموستات روشن می‌شود */
        if (hold_temp_allows_cool()) {
            pin_write(Compressor, 0);
            ice_start_act = true;
            ui_status_on(objects.fridge_status, objects.fridge_btn);
        } else {
            pin_write(Compressor, 1);
            ice_start_act = false;
        }

        /* در مد نگهدارنده مخزن، کمپرسور بزرگ سیلندر به هیچ وجه روشن نمی‌شود و کلید تولید تغییر نمی‌کند */
        if (!make_start && !plc_is_ready() && !plc_is_rechill()) {
            set_big_compressor(false);
        }

        if (ice_time > 0 && ice_counter >= ice_time) {
            ESP_LOGI(TAG, "hold timeout (%ld s)", (long)ice_time);
            buzz(250);
            stop_ice();
        }
    } else {
        ice_counter = 0;
        ice_seq_counter = 0;
        if (!make_start && !io_test_is_active() && !plc_is_ready() && !plc_is_rechill()) {
            pin_write(Compressor, 1);
            set_big_compressor(false);
        }
    }

    extra_modes_tick_1hz();
    day_lock_tick_1000ms();
    bool is_idle = (!make_start && !ice_start && !paddle_start && !wash_is_active() && s_phase == PLC_PHASE_IDLE);
    day_lock_notify_cycle_idle(is_idle);
}

static void lv_rtc_timer_cb(lv_timer_t *t)
{
    (void)t;

    if (ds3231_get_time_jalali(&jt) == ESP_OK) {
        static int s_last_min = -1;
        static int s_last_hour = -1;
        static int s_last_day = -1;

        if (objects.time_lbl && (jt.minute != s_last_min || jt.hour != s_last_hour)) {
            bool hour_width_changed = ((s_last_hour < 10) != (jt.hour < 10));
            s_last_min = jt.minute;
            s_last_hour = jt.hour;
            lv_obj_set_style_base_dir(objects.time_lbl, LV_BASE_DIR_LTR, 0);
            lv_label_set_text_fmt(objects.time_lbl, "\xE2\x80\xAD%02d:%02d\xE2\x80\xAC", jt.hour, jt.minute);
            /* re-align فقط اگر عرض متن تغییر کرده (مثلاً ۹:۵۹ → ۱۰:۰۰) */
            if (hour_width_changed) {
                lv_obj_align(objects.time_lbl, LV_ALIGN_LEFT_MID, 0, 0);
            }
        }
        if (s_last_day != -1 && jt.jd != s_last_day) {
            maint_condenser_days++;
            storage_set_i32(KEY_MAINT_CND, maint_condenser_days);
        }
        bool day_changed = (jt.jd != s_last_day);
        s_last_day = jt.jd;

        static ui_lang_t s_last_hdr_date_lang = (ui_lang_t)-1;
        bool lang_changed = (ui_lang_get() != s_last_hdr_date_lang);
        if (objects.date_lbl && (day_changed || lang_changed)) {
            s_last_hdr_date_lang = ui_lang_get();
            lv_obj_set_style_base_dir(objects.date_lbl, LV_BASE_DIR_LTR, 0);
            if (ui_lang_get() == UI_LANG_FA) {
                static const char *const WDAY_NAME[7] = {
                    "یک شنبه", "دو شنبه", "سه شنبه", "چهار شنبه", "پنج شنبه", "جمعه", "شنبه"};
                int wday_idx = (jt.wday >= 1 && jt.wday <= 7) ? (jt.wday - 1) : 0;
                lv_label_set_text_fmt(objects.date_lbl, "%04d/%02d/%02d %s",
                                      jt.jy, jt.jm, jt.jd, WDAY_NAME[wday_idx]);
            } else {
                int gy = 0, gm = 0, gd = 0;
                jalali_to_gregorian((jt.jy > 0) ? jt.jy : 1404,
                                    (jt.jm > 0) ? jt.jm : 1,
                                    (jt.jd > 0) ? jt.jd : 1,
                                    &gy, &gm, &gd);
                static const char *const WDAY_NAME_EN[7] = {
                    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
                static const char *const WDAY_NAME_TR[7] = {
                    "Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};
                int wday_idx = (jt.wday >= 1 && jt.wday <= 7) ? (jt.wday - 1) : 0;
                const char *wname = (ui_lang_get() == UI_LANG_TR) ? WDAY_NAME_TR[wday_idx] : WDAY_NAME_EN[wday_idx];
                lv_label_set_text_fmt(objects.date_lbl, "%04d/%02d/%02d %s",
                                      gy, gm, gd, wname);
            }
        }

        /* بررسی زمان‌بندی خودکار شب و روز (Auto Night/Day Scheduler) */
        if (night_scheduler_en && !lease_is_locked() && !day_lock_is_soft_locked() && !plc_is_fault() && !warranty_is_expired_and_locked()) {
            static int s_last_sched_min = -1;
            if (jt.minute != s_last_sched_min) {
                s_last_sched_min = jt.minute;
                if (jt.hour == night_start_h && jt.minute == night_start_m) {
                    if (!ice_start && !make_start) {
                        ESP_LOGI(TAG, "Auto Night Schedule -> Starting Ice (Hold) mode");
                        ice_start = true;
                        plc_notify_ice_started();
                        ui_status_on(objects.fridge_status, objects.fridge_btn);
                        nvs_request_lazy_flush();
                        ui_apply_state();
                    }
                } else if (jt.hour == night_end_h && jt.minute == night_end_m) {
                    if (!make_start) {
                        ESP_LOGI(TAG, "Auto Day Schedule -> Starting Make mode");
                        if (ice_start) {
                            stop_ice();
                        }
                        plc_hw_start_make();
                    }
                }
            }
        }
    }

    /* بررسی خطای معلق تعاملی (Interactive Fault Modal) */
    if (s_interactive_fault_pending != 0) {
        uint8_t fault_code = s_interactive_fault_pending;
        s_interactive_fault_pending = 0;
        dispatch_interactive_fault(fault_code);
    }

#if INDUCTIVE_ENABLED || LOAD_CTRL_ENABLED
    if (s_fault_msg_pending) {
        s_fault_msg_pending = false;
        panel_nav_toast_error(s_fault_msg ? s_fault_msg : "خطای حفاظتی دستگاه");
        buzzer_beep_alarm();
    }
#endif
    screensaver_tick_1s();
    ui_apply_state();
}

void app_params_load_from_nvs(void)
{
    if (!storage_init()) {
        return;
    }

    paddle_time = storage_get_i32(KEY_PADDLE_TIME, paddle_time);
    make_time = storage_get_i32(KEY_MAKE_TIME, make_time);
    ice_time = storage_get_i32(KEY_ICE_TIME, ice_time);
    icecream_counter_daily = storage_get_i32(KEY_ICE_DAILY, 0);
    icecream_counter = storage_get_i32(KEY_ICE_COUNTER, icecream_counter_daily);
    icecream_counter_total = storage_get_i32(KEY_ICE_TOTAL, 0);

    comp1phase_time = storage_get_i32(KEY_COMP_1PHASE, comp1phase_time);
    comp3phase_time = storage_get_i32(KEY_COMP_3PHASE, comp3phase_time);

    paddle_timer_1 = storage_get_i32(KEY_PDL_T1, paddle_timer_1);
    paddle_timer_2 = storage_get_i32(KEY_PDL_T2, paddle_timer_2);
    startup_timer = storage_get_i32(KEY_START_T, startup_timer);
    mid_eye_timer = storage_get_i32(KEY_EYE_T, mid_eye_timer);
    restart_time_1 = storage_get_i32(KEY_RST_T1, restart_time_1);
    restart_time_2 = storage_get_i32(KEY_RST_T2, restart_time_2);
    modbus_delay = storage_get_i32(KEY_MB_DLY, modbus_delay);
    current_set_1a = storage_get_i32(KEY_CUR1A, current_set_1a);
    current_set_1b = storage_get_i32(KEY_CUR1B, current_set_1b);
    current_set_2a = storage_get_i32(KEY_CUR2A, current_set_2a);
    current_set_2b = storage_get_i32(KEY_CUR2B, current_set_2b);
    screensaver_timeout_s = storage_get_i32(KEY_SS_TIMEOUT, DEFAULT_SS_TIMEOUT);
    if (screensaver_timeout_s < 0 ||
        (screensaver_timeout_s != 0 && screensaver_timeout_s != 900 &&
         screensaver_timeout_s != 1800 && screensaver_timeout_s != 3600 &&
         screensaver_timeout_s != 7200)) {
        screensaver_timeout_s = DEFAULT_SS_TIMEOUT;
    }
    cone_price = storage_get_i32(KEY_CONE_PRICE, DEFAULT_CONE_PRICE);
    if (cone_price < 0 || cone_price > 10000000) {
        cone_price = DEFAULT_CONE_PRICE;
    }
    night_scheduler_en = (storage_get_i32(KEY_NIGHT_EN, 0) != 0);
    night_start_h = storage_get_i32(KEY_NIGHT_SH, DEFAULT_NIGHT_SH);
    night_start_m = storage_get_i32(KEY_NIGHT_SM, DEFAULT_NIGHT_SM);
    night_end_h = storage_get_i32(KEY_NIGHT_EH, DEFAULT_NIGHT_EH);
    night_end_m = storage_get_i32(KEY_NIGHT_EM, DEFAULT_NIGHT_EM);
    maint_gasket_scoops = storage_get_i32(KEY_MAINT_GSK, 0);
    maint_condenser_days = storage_get_i32(KEY_MAINT_CND, 0);

    dispense_comp_run_s = storage_get_i32(KEY_DSP_RUN_S, DISPENSE_COMP_RUN_S);
    if (dispense_comp_run_s < 5 || dispense_comp_run_s > 60) {
        dispense_comp_run_s = DISPENSE_COMP_RUN_S;
    }
    front_hold_comp_s = storage_get_i32(KEY_FRT_HOLD_S, FRONT_HOLD_COMP_S);
    if (front_hold_comp_s < 2 || front_hold_comp_s > 15) {
        front_hold_comp_s = FRONT_HOLD_COMP_S;
    }
    scoops_for_comp = storage_get_i32(KEY_SCP_COMP, SCOOPS_FOR_COMP);
    if (scoops_for_comp < 1 || scoops_for_comp > 10) {
        scoops_for_comp = SCOOPS_FOR_COMP;
    }
    max_freeze_timeout_m = storage_get_i32(KEY_FRZ_TO_M, (MAX_FREEZE_TIMEOUT_S / 60));
    if (max_freeze_timeout_m < 5 || max_freeze_timeout_m > 30) {
        max_freeze_timeout_m = (MAX_FREEZE_TIMEOUT_S / 60);
    }
    dispense_lever_max_s = storage_get_i32(KEY_LEV_MAX_S, DISPENSE_LEVER_MAX_S);
    if (dispense_lever_max_s < 15 || dispense_lever_max_s > 120) {
        dispense_lever_max_s = DISPENSE_LEVER_MAX_S;
    }
    s_pulley_en = storage_get_bool(KEY_PULLEY_EN, true);

    /* نرمال‌سازی مقادیر قدیمی NVS با محدودهٔ سناریو */
    if (startup_timer < 0 || startup_timer > 10) {
        startup_timer = MAKE_COMP_DELAY_S;
    }
    if (mid_eye_timer < 1 || mid_eye_timer > 30) {
        mid_eye_timer = INDUCTIVE_TIMEOUT_S;
    }
    if (restart_time_1 < 60) {
        restart_time_1 = HOLD_BIG_PERIOD_S;
    }
    if (restart_time_2 < 30 || restart_time_2 > 600) {
        restart_time_2 = HOLD_COMP_DELAY_S;
    }

    /* ارتقاء امن تنظیمات قدیمی NVS (تبدیل ساعت‌های ذخیره شده با ضریب ۶۰ به ۳۶۰۰ ثانیه) */
    if (ice_time > 0 && ice_time <= 24 * 60 && (ice_time % 60 == 0)) {
        ice_time = (ice_time / 60) * 3600;
    }
    if (make_time > 0 && make_time <= 24 * 60 && (make_time % 60 == 0)) {
        make_time = (make_time / 60) * 3600;
    }

    /* همگام‌سازی paddle_time با تایمر پارو ۱ (دقیقه → ثانیه) */
    if (paddle_timer_1 > 0) {
        int32_t synced = paddle_timer_1 * 60;
        if (synced < PADDLE_MIN_S) {
            synced = PADDLE_MIN_S;
            paddle_timer_1 = synced / 60;
        } else if (synced > PADDLE_MAX_S) {
            synced = PADDLE_MAX_S;
            paddle_timer_1 = synced / 60;
        }
        paddle_time = synced;
    } else if (paddle_time < PADDLE_MIN_S) {
        paddle_time = PADDLE_DEFAULT_S;
        paddle_timer_1 = paddle_time / 60;
    } else if (paddle_time > PADDLE_MAX_S) {
        paddle_time = PADDLE_MAX_S;
        paddle_timer_1 = paddle_time / 60;
    }
}

void app_state_load_from_nvs(void)
{
    /* بوت امن: حالت‌های GPIO خاموش؛ بانک B فقط UI */
    paddle_start = false;
    make_start = false;
    ice_start = false;
    paddle_start_b = false;
    make_start_b = false;
    ice_start_b = false;

    app_params_load_from_nvs();
}

void app_state_save_to_nvs(void)
{
    storage_init();

    storage_set_i32(KEY_PADDLE_TIME, paddle_time);
    storage_set_i32(KEY_MAKE_TIME, make_time);
    storage_set_i32(KEY_ICE_TIME, ice_time);
    icecream_counter = icecream_counter_daily;
    storage_set_i32(KEY_ICE_COUNTER, icecream_counter);
    storage_set_i32(KEY_ICE_DAILY, icecream_counter_daily);
    storage_set_i32(KEY_ICE_TOTAL, icecream_counter_total);
    storage_set_i32(KEY_COMP_1PHASE, comp1phase_time);
    storage_set_i32(KEY_COMP_3PHASE, comp3phase_time);

    storage_set_i32(KEY_PDL_T1, paddle_timer_1);
    storage_set_i32(KEY_PDL_T2, paddle_timer_2);
    storage_set_i32(KEY_START_T, startup_timer);
    storage_set_i32(KEY_EYE_T, mid_eye_timer);
    storage_set_i32(KEY_RST_T1, restart_time_1);
    storage_set_i32(KEY_RST_T2, restart_time_2);
    storage_set_i32(KEY_MB_DLY, modbus_delay);
    storage_set_i32(KEY_CUR1A, current_set_1a);
    storage_set_i32(KEY_CUR1B, current_set_1b);
    storage_set_i32(KEY_CUR2A, current_set_2a);
    storage_set_i32(KEY_CUR2B, current_set_2b);
    storage_set_i32(KEY_MAKE_B, make_start_b ? 1 : 0);
    storage_set_i32(KEY_ICE_B, ice_start_b ? 1 : 0);
    storage_set_i32(KEY_PADDLE_B, paddle_start_b ? 1 : 0);
    storage_set_i32(KEY_SS_TIMEOUT, screensaver_timeout_s);
    storage_set_i32(KEY_CONE_PRICE, cone_price);
    storage_set_i32(KEY_NIGHT_EN, night_scheduler_en ? 1 : 0);
    storage_set_i32(KEY_NIGHT_SH, night_start_h);
    storage_set_i32(KEY_NIGHT_SM, night_start_m);
    storage_set_i32(KEY_NIGHT_EH, night_end_h);
    storage_set_i32(KEY_NIGHT_EM, night_end_m);
    storage_set_i32(KEY_MAINT_GSK, maint_gasket_scoops);
    storage_set_i32(KEY_MAINT_CND, maint_condenser_days);
    storage_set_i32(KEY_DSP_RUN_S, dispense_comp_run_s);
    storage_set_i32(KEY_FRT_HOLD_S, front_hold_comp_s);
    storage_set_i32(KEY_SCP_COMP, scoops_for_comp);
    storage_set_i32(KEY_FRZ_TO_M, max_freeze_timeout_m);
    storage_set_i32(KEY_LEV_MAX_S, dispense_lever_max_s);
    storage_set_bool(KEY_PULLEY_EN, s_pulley_en);
}

void app_factory_reset_defaults(void)
{
    paddle_timer_1 = 20;
    paddle_timer_2 = 15;
    startup_timer = 4;
    mid_eye_timer = 4;
    restart_time_1 = 7200;
    restart_time_2 = 120;
    modbus_delay = 1;
    current_set_1a = 0;
    current_set_1b = 430;
    current_set_2a = 0;
    current_set_2b = 430;
    comp1phase_time = 0;
    comp3phase_time = 0;
    paddle_time = 20 * 60;
    make_time = 0;
    ice_time = 0;
    icecream_counter_daily = 0;
    icecream_counter = 0;
    screensaver_timeout_s = DEFAULT_SS_TIMEOUT;
    cone_price = DEFAULT_CONE_PRICE;
    dispense_comp_run_s = DISPENSE_COMP_RUN_S;
    front_hold_comp_s = FRONT_HOLD_COMP_S;
    scoops_for_comp = SCOOPS_FOR_COMP;
    max_freeze_timeout_m = (MAX_FREEZE_TIMEOUT_S / 60);
    dispense_lever_max_s = DISPENSE_LEVER_MAX_S;
    night_scheduler_en = false;
    night_start_h = DEFAULT_NIGHT_SH;
    night_start_m = DEFAULT_NIGHT_SM;
    night_end_h = DEFAULT_NIGHT_EH;
    night_end_m = DEFAULT_NIGHT_EM;
    maint_gasket_scoops = 0;
    maint_condenser_days = 0;
    s_pulley_en = true;
    storage_set_bool(KEY_PULLEY_EN, true);
    hold_temp_set_on_c(4);
    hold_temp_set_off_c(2);
    milk_mixer_set_on_time_s(20);
    milk_mixer_set_off_time_m(5);
    /* icecream_counter_total به عنوان کیلومترشمار کل دستگاه حفظ می‌شود */
    app_state_save_to_nvs();
}

static void hardware_emergency_cutoff(void)
{
    pin_write(Paddle, 1);
    set_big_compressor(false);
    pin_write(Compressor, 1);
#if LOAD_CTRL_ENABLED
    pin_write(RESET_RELAY_GPIO, 1);
#endif
    pin_write(CREAM_PUMP_GPIO, 1);
    pin_write(AIR_SOLENOID_GPIO, 1);
    pin_write(RAINBOW_GPIO, 1);
    pin_write(SPARE_PIN_18, 1);
    pin_write(SPARE_RELAY_Q8_GPIO, 1); /* Q8 رزرو Active-LOW: سطح ۱ = قطع و غیرفعال قطعی */
    pin_write(SPARE_RELAY_Q9_GPIO, 1); /* Q9 رزرو Active-LOW: سطح ۱ = قطع و غیرفعال قطعی */
    buzzer_silence();
    io_test_turn_off_all_relays();
}

void outputs_init(void)
{
    const gpio_num_t relay_pins[] = {
        Compressor,            /* GPIO 16 (Q0) */
        Make_Icecream,         /* GPIO 42 (Q1) */
        Paddle,                /* GPIO 39 (Q2) */
        CREAM_PUMP_GPIO,       /* GPIO 48 (Q3) */
        RAINBOW_GPIO,          /* GPIO 40 (Q4) */
        AIR_SOLENOID_GPIO,     /* GPIO 15 (Q5: SSR1) */
#if LOAD_CTRL_ENABLED
        RESET_RELAY_GPIO,      /* GPIO 35 (Q6) */
#endif
        WASH_PUMP_GPIO,        /* GPIO 36 (Q7) */
        SPARE_PIN_18,          /* GPIO 18 (پین رزرو: همیشه ۱ منطقی / کاملاً خاموش) */
        SPARE_RELAY_Q8_GPIO,   /* GPIO 38 (Q8 رزرو: Active-LOW - خروجی معرفی شده و سطح ۱ منطقی = غیرفعال قطعی) */
        SPARE_RELAY_Q9_GPIO    /* GPIO 41 (Q9 رزرو: Active-LOW - خروجی معرفی شده و سطح ۱ منطقی = غیرفعال قطعی) */
    };

    /* استاندارد صلب زیمنس و ضد گلیچ: لچ مستقیم ثبات‌های سخت‌افزاری خروجی با سطح ۱ (OFF برای Active-LOW)
     * قبل از اتصال پد و فعال‌سازی درایور پین جهت ریشه‌کنی قطعی هرگونه گلیچ یا چتر لحظه‌ای بوبین رله در زمان بوت */
    for (size_t i = 0; i < sizeof(relay_pins) / sizeof(relay_pins[0]); i++) {
        gpio_num_t p = relay_pins[i];
        if ((uint32_t)p < 32) {
            GPIO.out_w1ts = (1UL << p);
        } else {
            GPIO.out1_w1ts.val = (1UL << (p - 32));
        }
        gpio_reset_pin(p);
        gpio_set_level(p, 1);
        gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << p),
            .mode = GPIO_MODE_INPUT_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        gpio_set_level(p, 1);
    }

    /* بازر پنل (GPIO 45) در حالت بوت کاملاً سایلنت (سطح ۰) */
    esp_rom_gpio_pad_select_gpio(BUZZER_GPIO);
    gpio_pulldown_en(BUZZER_GPIO);
    gpio_pullup_dis(BUZZER_GPIO);
    gpio_set_level(BUZZER_GPIO, 0);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);

    extra_outputs_init();
    esp_register_shutdown_handler(hardware_emergency_cutoff);
}

void inputs_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << KEY_STOP_GPIO) | (1ULL << KEY_START_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    analog_inputs_init();
}

void app_main(void)
{
    /* ۱. تایید فوری و بدون قیدوشرط پارتیشن جاری در سطر اول app_main جهت ابطال قطعی رول‌بک بوت‌لودر */
    esp_ota_mark_app_valid_cancel_rollback();

    outputs_init();
    storage_init();
    fault_log_init();
    machine_advisor_init();

    const esp_partition_t *running_part = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    bool is_pending_verify = false;
    if (running_part && esp_ota_get_state_partition(running_part, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY || ota_state == ESP_OTA_IMG_VALID) {
            is_pending_verify = true;
            ESP_LOGI(TAG, "OTA App Validated cleanly on partition %s -> Rollback cancelled.", running_part->label);
        }
    }

    /* ۲. بازرسی علت راه‌اندازی و ثبت خطاهای اضطراری ریست بر اساس استاندارد زیمنس ISA-18.2 */
    esp_reset_reason_t rr = esp_reset_reason();
    char last_boot_ver[32] = {0};
    storage_get_str("app_boot_ver", "", last_boot_ver, sizeof(last_boot_ver));
    bool is_ver_upgrade = (strcmp(last_boot_ver, FIRMWARE_VERSION_STR) != 0);
    bool is_post_ota = (storage_get_u32("ota_upd_ok", 0) == 1) || is_ver_upgrade || is_pending_verify;
    if (!is_post_ota) {
        if (rr == ESP_RST_BROWNOUT) {
            machine_advisor_record_brownout();
            fault_log_record(FAULT_BROWNOUT, (int16_t)rr);
            s_interactive_fault_pending = FAULT_BROWNOUT;
            ESP_LOGE(TAG, "DIAGNOSTIC ALERT: Hardware reset due to Brownout / Voltage Drop (0x%02X)", (int)rr);
        } else if (rr == ESP_RST_TASK_WDT || rr == ESP_RST_INT_WDT || rr == ESP_RST_WDT) {
            fault_log_record(FAULT_WATCHDOG_RESET, (int16_t)rr);
            s_interactive_fault_pending = FAULT_WATCHDOG_RESET;
            ESP_LOGE(TAG, "DIAGNOSTIC ALERT: Hardware reset due to Watchdog Timer Trip (0x%02X)", (int)rr);
        } else if (rr == ESP_RST_PANIC) {
            fault_log_record(FAULT_CORE_PANIC, (int16_t)rr);
            s_interactive_fault_pending = FAULT_CORE_PANIC;
            ESP_LOGE(TAG, "DIAGNOSTIC ALERT: Hardware reset due to CPU Core Panic (0x%02X)", (int)rr);
        }
    } else {
        ESP_LOGI(TAG, "Clean Post-OTA / New Version boot detected (rr=%d, ver_upgrade=%d, pending=%d, prev='%s', cur='%s'). Suppressing all reset diagnostics.",
                 (int)rr, (int)is_ver_upgrade, (int)is_pending_verify, last_boot_ver, FIRMWARE_VERSION_STR);
        s_interactive_fault_pending = 0;
        /* پاکسازی کامل ردپای ریست در حافظه RTC جهت ممانعت قطعی از نمایش خطای کرش فانتوم */
        s_rtc_breadcrumb.magic = 0;
        s_rtc_breadcrumb.magic_inv = 0;
    }

    app_state_load_from_nvs();
    s_comp_cooldown_left_s = 0;
    inputs_init();
    if (adc_boot_check_f5_hold()) {
        g_boot_f5_ota_requested = true;
        ESP_LOGW(TAG, ">>> BOOT F5 HELD: Direct Wi-Fi / OTA Update Mode Activated <<<");
    }
    max6675_init();

    /* ۱. اولویت مطلق تخصیص بافرهای DMA نمایشگر در ابتدای کار در RAM داخلی بکر و بدون قطعه‌قطعه شدن */
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (TFT_W * TFT_H * BYTES_PER_PIXEL) + 16,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MHZ * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 10,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &g_spi_lcd));

    lcd_init_basic();
    buzzer_init(BUZZER_GPIO, true);

    ESP_ERROR_CHECK(i2c_master_init());
    vTaskDelay(pdMS_TO_TICKS(100));
    gt911_init();

#if LVGL_ENABLE
    lvgl_port_init();
#endif

    /* ۲. راه‌اندازی رادیو، پشته شبکه و سرویس‌های ابری پس از تضمین سلامت بافرهای سخت‌افزاری نمایشگر */
    wifi_init_once();
    ota_update_init();
    cloud_client_init();
    app_timers_init();
    app_timers_set_hook(one_hz_hook, NULL);

    setenv("TZ", "IRST-3:30", 1);
    tzset();
    esp_err_t rtc_err = ds3231_init();
    if (rtc_err != ESP_OK) {
        ESP_LOGW(TAG, "DS3231 RTC not detected (%s) - running with fallback clock", esp_err_to_name(rtc_err));
    }
    extra_modes_init();
    warranty_init();
    boot_diag_run_self_test();

#if LVGL_ENABLE
    /* UI را قبل از فعال‌شدن handler بساز تا race/WDT نشود */
    ESP_LOGI(TAG, "ui_init begin (total=%u, internal=%u)", (unsigned)esp_get_free_heap_size(), (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ui_init();
    panel_nav_init();
    lv_timer_create(lv_rtc_timer_cb, 1000, NULL);
    lvgl_start();
#if KEYPAD_ADC_RESISTOR_LADDER
    adc_init();
    adc_start(50, 16);
#endif
    actions_start(70, 4);
    serial_hmi_init();
#endif
}
