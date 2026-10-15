/**
 * @file ui_bridge.cpp
 * @brief UI Engine Bridge Implementation
 */

#include "ui_engine.h"
#include "ui.h"
#include "screens.h"
#include "vars.h"
#include "plc_engine.h"
#include "hal_thermo.h"
#include "hal_rtc.h"
#include "nvs_storage.h"
#include "esp_log.h"

static const char *TAG = "UI_ENGINE";

esp_err_t ui_engine_init(void) {
    ESP_LOGI(TAG, "Initializing EEZ Studio UI Screens, Fonts & Styles...");
    ui_init();
    ESP_LOGI(TAG, "UI Screens initialized successfully.");
    return ESP_OK;
}

void ui_engine_tick(void) {
    // 1. Sync Live PLC Status into UI variables
    plc_cycle_status_t status = plc_engine_get_status();

    set_var_temp_real((int32_t)status.current_temperature_c);
    set_var_target_temp((int32_t)status.target_temperature_c);
    set_var_step_number((int32_t)status.step);
    set_var_step_process(status.in_progress);
    set_var_emergency_stop(status.emergency_stopped);

    if (status.in_progress) {
        set_var_josh_counter((int32_t)status.remaining_countdown_sec);
    }

    // 2. Sync Real-Time Clock
    hal_rtc_time_t rtc_time = hal_rtc_get_time();
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", rtc_time.hour, rtc_time.minute);
    set_var_date_time(time_str);

    // 3. Execute EEZ Studio Screen Tick
    ui_tick();
}
