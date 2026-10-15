#include "fault_log.h"
#include "storage.h"
#include "esp_log.h"
#include "ui/ui_lang.h"
#include <string.h>
#include <time.h>

static const char *TAG = "FAULT_LOG";

typedef struct {
    uint8_t count;
    uint8_t head;
    fault_entry_t entries[FAULT_LOG_MAX_ENTRIES];
} fault_log_storage_t;

static fault_log_storage_t s_log;
static bool s_initialized = false;
static uint32_t s_fault_total_logged = 0;
static uint32_t s_fault_last_viewed = 0;

void fault_log_init(void)
{
    if (s_initialized) return;
    memset(&s_log, 0, sizeof(s_log));

    int res = storage_get_blob("flt_log", &s_log, sizeof(s_log));
    if (res != (int)sizeof(s_log)) {
        s_log.count = 0;
        s_log.head = 0;
        memset(s_log.entries, 0, sizeof(s_log.entries));
    }
    s_fault_total_logged = storage_get_u32("flt_tot", 0);
    s_fault_last_viewed  = storage_get_u32("flt_view", 0);

    /* همگام‌سازی اولیه در صورت عدم ثبت قبلی شمارنده */
    if (s_fault_total_logged < s_log.count) {
        s_fault_total_logged = s_log.count;
        s_fault_last_viewed  = s_log.count;
        storage_set_u32("flt_tot", s_fault_total_logged);
        storage_set_u32("flt_view", s_fault_last_viewed);
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Fault log initialized. Total in log: %d, Counter: %lu, Viewed: %lu",
             s_log.count, (unsigned long)s_fault_total_logged, (unsigned long)s_fault_last_viewed);
}

void fault_log_record(fault_code_t code, int16_t value)
{
    if (!s_initialized) fault_log_init();
    if (code == FAULT_NONE) return;

    /* اگر آخرین خطای ثبت‌شده دقیقاً همین خطا در ۵ ثانیه گذشته بوده، برای عدم پر شدن حلقه نادیده بگیر */
    time_t now = time(NULL);
    if (s_log.count > 0) {
        int last_idx = (s_log.head - 1 + FAULT_LOG_MAX_ENTRIES) % FAULT_LOG_MAX_ENTRIES;
        if (s_log.entries[last_idx].code == code && (now - (time_t)s_log.entries[last_idx].timestamp) < 5) {
            return;
        }
    }

    s_log.entries[s_log.head].timestamp = (uint32_t)now;
    s_log.entries[s_log.head].code = (uint8_t)code;
    s_log.entries[s_log.head].value = value;

    s_log.head = (s_log.head + 1) % FAULT_LOG_MAX_ENTRIES;
    if (s_log.count < FAULT_LOG_MAX_ENTRIES) {
        s_log.count++;
    }

    s_fault_total_logged++;
    storage_set_u32("flt_tot", s_fault_total_logged);
    storage_set_blob("flt_log", &s_log, sizeof(s_log));
    ESP_LOGW(TAG, "New fault recorded: code=%d, val=%d, time=%lu, total=%lu",
             code, value, (unsigned long)now, (unsigned long)s_fault_total_logged);
}

int fault_log_get_count(void)
{
    if (!s_initialized) fault_log_init();
    return s_log.count;
}

bool fault_log_get_entry(int index, fault_entry_t *out)
{
    if (!s_initialized) fault_log_init();
    if (out == NULL || index < 0 || index >= (int)s_log.count) return false;

    /* شاخص ۰ = جدیدترین خطا */
    int actual_idx = (s_log.head - 1 - index + FAULT_LOG_MAX_ENTRIES * 2) % FAULT_LOG_MAX_ENTRIES;
    *out = s_log.entries[actual_idx];
    return true;
}

bool fault_log_has_unread(void)
{
    if (!s_initialized) fault_log_init();
    if (s_log.count == 0) return false;
    return (s_fault_total_logged > s_fault_last_viewed);
}

void fault_log_mark_read(void)
{
    if (!s_initialized) fault_log_init();
    s_fault_last_viewed = s_fault_total_logged;
    storage_set_u32("flt_view", s_fault_last_viewed);
}

uint32_t fault_log_get_total_count(void)
{
    if (!s_initialized) fault_log_init();
    return s_fault_total_logged;
}

void fault_log_clear(void)
{
    if (!s_initialized) fault_log_init();
    s_log.count = 0;
    s_log.head = 0;
    memset(s_log.entries, 0, sizeof(s_log.entries));
    storage_set_blob("flt_log", &s_log, sizeof(s_log));
    s_fault_last_viewed = s_fault_total_logged;
    storage_set_u32("flt_view", s_fault_last_viewed);
    ESP_LOGI(TAG, "Fault log cleared");
}

const char* fault_code_to_str(fault_code_t code)
{
    ui_lang_t lang = ui_lang_get();
    if (lang == UI_LANG_EN) {
        switch (code) {
            case FAULT_MOTOR_OVERLOAD:        return "Mixer Motor Overload Trip";
            case FAULT_COMP_OVERLOAD:         return "Compressor Overload Trip";
            case FAULT_EMERGENCY_STOP:        return "Milk Hopper Empty (Low Mix)";
            case FAULT_INDUCTIVE_STALL:       return "Pulley Rotation Stall";
            case FAULT_LEASE_EXPIRED:         return "Lease / Day-Lock Expired";
            case FAULT_TORQUE_PREDICT:        return "High Viscosity Torque Alert";
            case FAULT_FREEZE_TIMEOUT:        return "Maximum Freezing Timeout";
            case FAULT_STUCK_LEVER:           return "Dispense Lever Stuck Open";
            case FAULT_STUCK_REAR:            return "Rear Microswitch Stuck";
            case FAULT_THERMO_TOP_DISCONNECT: return "Hopper Sensor Disconnect T1";
            case FAULT_THERMO_CYL_DISCONNECT: return "Cylinder Sensor Disconnect T2";
            case FAULT_BROWNOUT:              return "Brownout Voltage Drop";
            case FAULT_WATCHDOG_RESET:        return "Task Watchdog Reset";
            case FAULT_CORE_PANIC:            return "Core Panic Reset";
            case FAULT_RTC_ERROR:             return "RTC Clock Communication Error";
            case FAULT_TOUCH_ERROR:           return "Touch Controller Comm Error";
            case FAULT_MEMORY_ERROR:          return "RAM / Heap Memory Fault";
            case FAULT_CPU_OVERHEAT:          return "CPU Thermal Overheat Warning";
            case FAULT_MAX6675_TOP_MISSING:   return "Hopper Temperature Sensor Disconnect T1";
            case FAULT_MAX6675_CYL_MISSING:   return "Cylinder Temperature Sensor Disconnect T2";
            default:                          return "System Fault";
        }
    } else if (lang == UI_LANG_TR) {
        switch (code) {
            case FAULT_MOTOR_OVERLOAD:        return "Karistirici Motor Asiri Yuk";
            case FAULT_COMP_OVERLOAD:         return "Kompresor Asiri Yuk";
            case FAULT_EMERGENCY_STOP:        return "Sut Tanki Bos (Dusuk Seviye)";
            case FAULT_INDUCTIVE_STALL:       return "Kasnak Donus Durmasi";
            case FAULT_LEASE_EXPIRED:         return "Kira Suresi Doldu";
            case FAULT_TORQUE_PREDICT:        return "Yuksek Tork Uyarisi";
            case FAULT_FREEZE_TIMEOUT:        return "Maks Dondurma Suresi";
            case FAULT_STUCK_LEVER:           return "Dondurma Kolu Acik Kaldi";
            case FAULT_STUCK_REAR:            return "Arka Debriyaj Takildi";
            case FAULT_THERMO_TOP_DISCONNECT: return "Sut Tanki Sensor Kesik T1";
            case FAULT_THERMO_CYL_DISCONNECT: return "Silindir Sensor Kesik T2";
            case FAULT_BROWNOUT:              return "Dusuk Voltaj Kesintisi";
            case FAULT_WATCHDOG_RESET:        return "Watchdog Reset (WDT)";
            case FAULT_CORE_PANIC:            return "Cekirdek Core Panic";
            case FAULT_RTC_ERROR:             return "RTC Saat Iletisim Hatasi";
            case FAULT_TOUCH_ERROR:           return "Dokunmatik Iletisim Hatasi";
            case FAULT_MEMORY_ERROR:          return "RAM / Bellek Hatasi";
            case FAULT_CPU_OVERHEAT:          return "CPU Asiri Isinma Uyarisi";
            case FAULT_MAX6675_TOP_MISSING:   return "Sut Tanki Sensor Kesik T1";
            case FAULT_MAX6675_CYL_MISSING:   return "Silindir Sensor Kesik T2";
            default:                          return "Sistem Hatasi";
        }
    } else {
        switch (code) {
            case FAULT_MOTOR_OVERLOAD:        return "اضافه بار موتور پارو";
            case FAULT_COMP_OVERLOAD:         return "کنترل‌بار کمپرسور برودت";
            case FAULT_EMERGENCY_STOP:        return "مخزن شیر خالی است";
            case FAULT_INDUCTIVE_STALL:       return "توقف چرخش پولی گیربکس";
            case FAULT_LEASE_EXPIRED:         return "انقضای اعتبار روزشمار";
            case FAULT_TORQUE_PREDICT:        return "هشدار فشار بار و غلظت";
            case FAULT_FREEZE_TIMEOUT:        return "زمان بیشینه انجماد سیلندر";
            case FAULT_STUCK_LEVER:           return "باز ماندن اهرم تخلیه بستنی";
            case FAULT_STUCK_REAR:            return "درگیر بودن میکروسوئیچ عقب";
            case FAULT_THERMO_TOP_DISCONNECT: return "قطعی سنسور دمای مخزن شیر (T1)";
            case FAULT_THERMO_CYL_DISCONNECT: return "قطعی سنسور دمای سیلندر (T2)";
            case FAULT_BROWNOUT:              return "افت ولتاژ برق ورودی (Brownout)";
            case FAULT_WATCHDOG_RESET:        return "تریپ واچ‌داگ نرم‌افزاری (Task WDT)";
            case FAULT_CORE_PANIC:            return "کرش هسته پردازنده (Core Panic)";
            case FAULT_RTC_ERROR:             return "خطای ارتباط با ساعت سخت‌افزاری";
            case FAULT_TOUCH_ERROR:           return "عدم پاسخ کنترلر لمسی پنل";
            case FAULT_MEMORY_ERROR:          return "خطای فضای حافظه موقت سیستم";
            case FAULT_CPU_OVERHEAT:          return "دمای بیش از حد مجاز پردازنده";
            case FAULT_MAX6675_TOP_MISSING:   return "قطعی سنسور دمای مخزن شیر (T1)";
            case FAULT_MAX6675_CYL_MISSING:   return "قطعی سنسور دمای سیلندر انجماد (T2)";
            default:                          return "خطای سیستم";
        }
    }
}

const char* fault_code_to_isa_str(fault_code_t code)
{
    switch (code) {
        case FAULT_MOTOR_OVERLOAD:        return "E02";
        case FAULT_COMP_OVERLOAD:         return "E03";
        case FAULT_EMERGENCY_STOP:        return "E00";
        case FAULT_INDUCTIVE_STALL:       return "E01";
        case FAULT_LEASE_EXPIRED:         return "E07";
        case FAULT_TORQUE_PREDICT:        return "E06";
        case FAULT_FREEZE_TIMEOUT:        return "E10";
        case FAULT_STUCK_LEVER:           return "E08";
        case FAULT_STUCK_REAR:            return "E09";
        case FAULT_THERMO_TOP_DISCONNECT: return "E04";
        case FAULT_THERMO_CYL_DISCONNECT: return "E05";
        case FAULT_BROWNOUT:              return "E12";
        case FAULT_WATCHDOG_RESET:        return "E13";
        case FAULT_CORE_PANIC:            return "E14";
        case FAULT_MAX6675_TOP_MISSING:   return "E04";
        case FAULT_MAX6675_CYL_MISSING:   return "E05";
        default:                          return "E01";
    }
}

