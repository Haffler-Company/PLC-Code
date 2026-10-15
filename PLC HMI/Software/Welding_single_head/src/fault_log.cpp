/**
 * @file fault_log.cpp
 * @brief Industrial ISA-18.2 Compliant Fault Logging Implementation
 * @author Haffler Automation Systems
 */

#include "fault_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>
#include <Arduino.h>

static const char *TAG = "FAULT_LOG";
#define NVS_NAMESPACE "welder_cfg"

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

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        size_t size = sizeof(s_log);
        if (nvs_get_blob(handle, "flt_log", &s_log, &size) != ESP_OK || size != sizeof(s_log)) {
            s_log.count = 0;
            s_log.head = 0;
            memset(s_log.entries, 0, sizeof(s_log.entries));
        }
        nvs_get_u32(handle, "flt_tot", &s_fault_total_logged);
        nvs_get_u32(handle, "flt_view", &s_fault_last_viewed);
        nvs_close(handle);
    } else {
        s_log.count = 0;
        s_log.head = 0;
        memset(s_log.entries, 0, sizeof(s_log.entries));
        s_fault_total_logged = 0;
        s_fault_last_viewed = 0;
    }

    if (s_fault_total_logged < s_log.count) {
        s_fault_total_logged = s_log.count;
        s_fault_last_viewed = s_log.count;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "ISA-18.2 Fault log ready. Count: %d, Total: %lu, Viewed: %lu",
             s_log.count, (unsigned long)s_fault_total_logged, (unsigned long)s_fault_last_viewed);
}

void fault_log_record(fault_code_t code, int16_t value)
{
    if (!s_initialized) fault_log_init();
    if (code == FAULT_NONE) return;

    // Rate limiting: suppress duplicate identical fault within 60 seconds to protect SPI flash
    uint32_t now_sec = (uint32_t)(millis() / 1000UL);
    if (s_log.count > 0) {
        int last_idx = (s_log.head - 1 + FAULT_LOG_MAX_ENTRIES) % FAULT_LOG_MAX_ENTRIES;
        if (s_log.entries[last_idx].code == (uint8_t)code &&
            (now_sec - s_log.entries[last_idx].timestamp) < 60U) {
            return;
        }
    }

    s_log.entries[s_log.head].timestamp = now_sec;
    s_log.entries[s_log.head].code = (uint8_t)code;
    s_log.entries[s_log.head].value = value;

    s_log.head = (s_log.head + 1) % FAULT_LOG_MAX_ENTRIES;
    if (s_log.count < FAULT_LOG_MAX_ENTRIES) {
        s_log.count++;
    }
    s_fault_total_logged++;

    // Persist to NVS
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "flt_log", &s_log, sizeof(s_log));
        nvs_set_u32(handle, "flt_tot", s_fault_total_logged);
        nvs_commit(handle);
        nvs_close(handle);
    }

    ESP_LOGW(TAG, "Fault Logged: [%s] Code=%d, Val=%d, Total=%lu",
             fault_code_to_isa_str(code), (int)code, (int)value, (unsigned long)s_fault_total_logged);
}

int fault_log_get_count(void)
{
    if (!s_initialized) fault_log_init();
    return s_log.count;
}

bool fault_log_get_entry(int index, fault_entry_t *out)
{
    if (!s_initialized) fault_log_init();
    if (!out || index < 0 || index >= (int)s_log.count) return false;

    // Index 0 = newest entry
    int actual_idx = (s_log.head - 1 - index + FAULT_LOG_MAX_ENTRIES * 2) % FAULT_LOG_MAX_ENTRIES;
    *out = s_log.entries[actual_idx];
    return true;
}

void fault_log_clear(void)
{
    if (!s_initialized) fault_log_init();
    s_log.count = 0;
    s_log.head = 0;
    memset(s_log.entries, 0, sizeof(s_log.entries));
    s_fault_last_viewed = s_fault_total_logged;

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "flt_log", &s_log, sizeof(s_log));
        nvs_set_u32(handle, "flt_view", s_fault_last_viewed);
        nvs_commit(handle);
        nvs_close(handle);
    }
    ESP_LOGI(TAG, "Fault log cleared.");
}

const char* fault_code_to_str(fault_code_t code, int lang)
{
    switch (code) {
        case FAULT_EMERGENCY_STOP:
            return (lang == 2) ? "Acil Durdurma" : ((lang == 1) ? "Emergency Stop" : "قطع اضطراری");
        case FAULT_THERMO_DISCONNECT:
            return (lang == 2) ? "Termokupl Kesik" : ((lang == 1) ? "Thermocouple Open" : "قطعی ترموکوپل");
        case FAULT_HEATER_TIMEOUT:
            return (lang == 2) ? "Isitici Zaman Asimi" : ((lang == 1) ? "Heater Timeout" : "خطای زمان گرمایش المنت");
        case FAULT_OVERTEMP:
            return (lang == 2) ? "Asiri Sicaklik" : ((lang == 1) ? "Over-Temperature" : "دمای بحرانی المنت");
        case FAULT_SHABLON_TIMEOUT:
            return (lang == 2) ? "Sablon Sensor Hatasi" : ((lang == 1) ? "Template Jack Timeout" : "خطای سنسور شابلون");
        case FAULT_ELEMENT_TIMEOUT:
            return (lang == 2) ? "Rezistans Kriko Hatasi" : ((lang == 1) ? "Element Jack Timeout" : "خطای سنسور المنت");
        case FAULT_TABLE_TIMEOUT:
            return (lang == 2) ? "Tabla Sensor Hatasi" : ((lang == 1) ? "Table Jack Timeout" : "خطای سنسور میز");
        case FAULT_RTC_ERROR:
            return (lang == 2) ? "RTC Saat Hatasi" : ((lang == 1) ? "RTC Clock Fault" : "خطای ساعت سخت افزاری");
        case FAULT_TOUCH_ERROR:
            return (lang == 2) ? "Dokunmatik Hatasi" : ((lang == 1) ? "Touch Controller Fault" : "خطای کنترلر لمسی");
        case FAULT_LEASE_EXPIRED:
            return (lang == 2) ? "Kullanim Suresi Doldu" : ((lang == 1) ? "License / Lease Expired" : "انقضای گارانتی / قفل روزانه");
        case FAULT_BROWNOUT:
            return (lang == 2) ? "Dusuk Voltaj Sifirlama" : ((lang == 1) ? "Brownout Voltage Reset" : "افت ولتاژ برق شبکه");
        case FAULT_WATCHDOG_RESET:
            return (lang == 2) ? "Guvenlik Zamanlayici Sifirlama" : ((lang == 1) ? "Watchdog Reset" : "ریست واچ داگ ایمنی");
        default:
            return (lang == 2) ? "Normal" : ((lang == 1) ? "Normal" : "عادی");
    }
}

const char* fault_code_to_isa_str(fault_code_t code)
{
    switch (code) {
        case FAULT_EMERGENCY_STOP:    return "[E-01] EMERGENCY STOP ACTIVE";
        case FAULT_THERMO_DISCONNECT: return "[E-02] THERMOCOUPLE OPEN / DISCONNECTED";
        case FAULT_HEATER_TIMEOUT:    return "[E-03] HEATING TIMEOUT (>120s)";
        case FAULT_OVERTEMP:          return "[E-04] CRITICAL OVER-TEMPERATURE (>270C)";
        case FAULT_SHABLON_TIMEOUT:   return "[E-05] SHABLON CYLINDER SENSOR TIMEOUT";
        case FAULT_ELEMENT_TIMEOUT:   return "[E-06] ELEMENT CYLINDER SENSOR TIMEOUT";
        case FAULT_TABLE_TIMEOUT:     return "[E-07] TABLE CYLINDER SENSOR TIMEOUT";
        case FAULT_RTC_ERROR:         return "[E-08] DS3231 I2C RTC COMMUNICATION ERROR";
        case FAULT_TOUCH_ERROR:       return "[E-09] GT911 TOUCH CONTROLLER ERROR";
        case FAULT_LEASE_EXPIRED:     return "[E-10] SYSTEM DAY-LOCK / LEASE EXPIRED";
        case FAULT_BROWNOUT:          return "[E-11] BROWNOUT VOLTAGE DIP DETECTED";
        case FAULT_WATCHDOG_RESET:    return "[E-12] HARDWARE WATCHDOG RESET TRIPPED";
        default:                      return "[E-00] NO ACTIVE FAULT";
    }
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
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u32(handle, "flt_view", s_fault_last_viewed);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

uint32_t fault_log_get_total_count(void)
{
    if (!s_initialized) fault_log_init();
    return s_fault_total_logged;
}
