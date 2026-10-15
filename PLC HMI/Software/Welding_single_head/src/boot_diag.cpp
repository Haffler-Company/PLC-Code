/**
 * @file boot_diag.cpp
 * @brief Industrial Power-On Self-Test (POST) & Hardware Diagnostics Implementation
 * @author Haffler Automation Systems
 */

#include "boot_diag.h"
#include "fault_log.h"
#include "vars.h"
#include "esp_system.h"
#include "esp_log.h"
#include <Arduino.h>
#include <Wire.h>

static const char *TAG = "BOOT_DIAG";
static boot_diag_result_t s_result = {0};
static bool s_tested = false;

extern bool g_hw_rtc_missing;
extern bool g_hw_touch_missing;
extern int readTemp(void);

void boot_diag_run_self_test(void)
{
    if (s_tested) return;
    memset(&s_result, 0, sizeof(s_result));

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "=== HAFFLER INDUSTRIAL CONTROLLER POST START ===");
    ESP_LOGI(TAG, "=================================================");

    // 1. Reset Reason Inspection
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG, "Reset Reason: %d", (int)reason);
    if (reason == ESP_RST_BROWNOUT) {
        s_result.brownout_detected = true;
        s_result.has_error = true;
        fault_log_record(FAULT_BROWNOUT, 0);
        ESP_LOGE(TAG, "POST WARNING: Prior reset caused by Mains Brownout dip!");
    } else if (reason == ESP_RST_WDT || reason == ESP_RST_TASK_WDT) {
        s_result.watchdog_detected = true;
        s_result.has_error = true;
        fault_log_record(FAULT_WATCHDOG_RESET, 0);
        ESP_LOGE(TAG, "POST WARNING: Prior reset caused by Watchdog Trip!");
    }

    // 2. Memory Checks
    s_result.free_heap_bytes = esp_get_free_heap_size();
    s_result.free_psram_bytes = psramFound() ? ESP.getFreePsram() : 0;
    ESP_LOGI(TAG, "Free SRAM: %lu bytes | Free PSRAM: %lu bytes",
             (unsigned long)s_result.free_heap_bytes, (unsigned long)s_result.free_psram_bytes);

    if (s_result.free_heap_bytes < 60000) {
        s_result.heap_low = true;
        s_result.has_error = true;
        ESP_LOGW(TAG, "POST WARNING: Internal SRAM heap low (< 60KB)");
    }
    if (!psramFound() || s_result.free_psram_bytes < 500000) {
        s_result.psram_fail = true;
        s_result.has_error = true;
        ESP_LOGE(TAG, "POST ERROR: External PSRAM allocation failure!");
    }

    // 3. I2C Bus & RTC Check
    if (g_hw_rtc_missing) {
        s_result.rtc_fail = true;
        s_result.has_error = true;
        fault_log_record(FAULT_RTC_ERROR, 0);
        ESP_LOGW(TAG, "POST WARNING: DS3231 RTC not responding on I2C bus");
    } else {
        ESP_LOGI(TAG, "POST OK: DS3231 RTC active and communicating");
    }

    // 4. GT911 Touch Controller Ping (I2C addr 0x5D or 0x14)
    Wire.beginTransmission(0x5D);
    if (Wire.endTransmission() != 0) {
        Wire.beginTransmission(0x14);
        if (Wire.endTransmission() != 0) {
            s_result.touch_fail = true;
            ESP_LOGW(TAG, "POST WARNING: GT911 Touch Controller not responding on I2C");
        } else {
            ESP_LOGI(TAG, "POST OK: GT911 Touch Controller detected at 0x14");
        }
    } else {
        ESP_LOGI(TAG, "POST OK: GT911 Touch Controller detected at 0x5D");
    }

    // 5. MAX6675 Thermocouple Test
    int raw_t = readTemp();
    float temp_c = (raw_t > -500) ? ((float)raw_t / 4.0f) : -999.0f;
    s_result.current_temp_c = temp_c;
    if (get_var_thermocouple_error() || temp_c < -100.0f) {
        s_result.thermo_fail = true;
        s_result.has_error = true;
        fault_log_record(FAULT_THERMO_DISCONNECT, -999);
        ESP_LOGW(TAG, "POST NOTE: MAX6675 Thermocouple open-circuit or probe disconnected (T=%.1f C)", temp_c);
    } else {
        ESP_LOGI(TAG, "POST OK: Thermocouple reading valid (T=%.1f C)", temp_c);
    }

    s_tested = true;
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "=== POST COMPLETE: Health=%s ===", s_result.has_error ? "ATTENTION_REQUIRED" : "ALL_CLEAR_100%");
    ESP_LOGI(TAG, "=================================================");
}

const boot_diag_result_t* boot_diag_get_result(void)
{
    if (!s_tested) boot_diag_run_self_test();
    return &s_result;
}

bool boot_diag_has_error(void)
{
    if (!s_tested) boot_diag_run_self_test();
    return s_result.has_error;
}
