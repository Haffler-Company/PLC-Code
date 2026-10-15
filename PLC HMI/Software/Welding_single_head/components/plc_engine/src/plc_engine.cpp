/**
 * @file plc_engine.cpp
 * @brief Industrial 14-Step Welding Cycle State Machine Implementation
 */

#include "plc_engine.h"
#include "hal_io.h"
#include "hal_thermo.h"
#include "nvs_storage.h"
#include "thermal_pid.h"
#include "fault_log.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "PLC_ENGINE";

static plc_cycle_status_t s_status = {
    .step = WELD_STEP_0_IDLE,
    .in_progress = false,
    .emergency_stopped = false,
    .step_elapsed_ms = 0,
    .remaining_countdown_sec = 0,
    .current_temperature_c = 0.0f,
    .target_temperature_c = 250.0f,
    .heater_ssr_state = false
};

static nvs_machine_config_t s_cfg;
static thermal_pid_controller_t s_pid;
static uint64_t s_step_start_us = 0;
static SemaphoreHandle_t s_status_mutex = NULL;

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

esp_err_t plc_engine_init(void) {
    ESP_LOGI(TAG, "Initializing PLC Welding FSM Engine...");

    if (!s_status_mutex) {
        s_status_mutex = xSemaphoreCreateMutex();
    }

    nvs_storage_load_config(&s_cfg);
    thermal_pid_init(&s_pid, 4.0f, 0.05f, 15.0f, 0.0f, 100.0f);
    thermal_pid_set_setpoint(&s_pid, (float)s_cfg.target_temp_c);

    s_status.target_temperature_c = (float)s_cfg.target_temp_c;
    s_status.step = WELD_STEP_0_IDLE;
    s_status.in_progress = false;

    ESP_LOGI(TAG, "PLC Engine initialized. Target Temp: %d °C, Weld Time: %d s, Cool Time: %d s",
             s_cfg.target_temp_c, s_cfg.weld_time_sec, s_cfg.cool_time_sec);
    return ESP_OK;
}

void plc_engine_abort(void) {
    if (s_status_mutex) xSemaphoreTake(s_status_mutex, portMAX_DELAY);
    s_status.step = WELD_STEP_0_IDLE;
    s_status.in_progress = false;
    hal_io_all_outputs_off();
    hal_io_set_shablon(true); // Return shablon to home
    if (s_status_mutex) xSemaphoreGive(s_status_mutex);
    ESP_LOGW(TAG, "PLC Cycle Aborted - All outputs de-energized.");
}

void plc_engine_pedal_trigger(void) {
    if (s_status.in_progress) return; // Ignore if already running

    // Check E-Stop
    if (hal_io_is_emergency_active()) {
        hal_io_play_buzzer(BUZZER_TONE_ERROR);
        fault_log_record(FAULT_EMERGENCY_STOP, 0);
        ESP_LOGE(TAG, "Cannot start cycle: E-Stop active!");
        return;
    }

    // Check Thermocouple health
    if (!hal_thermo_is_healthy()) {
        hal_io_play_buzzer(BUZZER_TONE_ERROR);
        fault_log_record(FAULT_THERMO_DISCONNECT, -999);
        ESP_LOGE(TAG, "Cannot start cycle: Thermocouple fault!");
        return;
    }

    if (s_status_mutex) xSemaphoreTake(s_status_mutex, portMAX_DELAY);
    s_status.step = WELD_STEP_1_PRESS_LEFT;
    s_status.in_progress = true;
    s_step_start_us = esp_timer_get_time();
    hal_io_set_press_left(true);
    hal_io_play_buzzer(BUZZER_TONE_CYCLE_START);
    if (s_status_mutex) xSemaphoreGive(s_status_mutex);

    ESP_LOGI(TAG, "Welding Cycle Started -> Step 1: Press Left engaged.");
}

void plc_engine_tick(void) {
    // 1. Process E-Stop Immediate Interlock (<10ms)
    if (hal_io_is_emergency_active()) {
        if (!s_status.emergency_stopped) {
            s_status.emergency_stopped = true;
            plc_engine_abort();
            hal_io_play_buzzer(BUZZER_TONE_ALARM);
            fault_log_record(FAULT_EMERGENCY_STOP, 0);
            ESP_LOGE(TAG, "CRITICAL: E-Stop triggered! Instant safety cutoff executed.");
        }
        return;
    } else {
        s_status.emergency_stopped = false;
    }

    // 2. Thermal Control Loop
    hal_thermo_reading_t thermo = hal_thermo_read_primary();
    s_status.current_temperature_c = thermo.temperature_c;

    if (thermo.status == THERMO_STATUS_OK) {
        float pid_out = thermal_pid_compute(&s_pid, thermo.temperature_c, get_time_ms());
        // Simple time-proportional SSR modulation
        bool ssr_on = (pid_out > 20.0f) && (thermo.temperature_c < s_status.target_temperature_c + 2.0f);
        hal_io_set_ssr(ssr_on);
        s_status.heater_ssr_state = ssr_on;
    } else {
        hal_io_set_ssr(false);
        s_status.heater_ssr_state = false;
        if (s_status.in_progress) {
            plc_engine_abort();
        }
    }

    // 3. 14-Step Welding FSM Tick
    if (!s_status.in_progress) return;

    uint32_t elapsed_ms = (uint32_t)((esp_timer_get_time() - s_step_start_us) / 1000ULL);
    s_status.step_elapsed_ms = elapsed_ms;

    switch (s_status.step) {
        case WELD_STEP_1_PRESS_LEFT:
            if (elapsed_ms >= 500) {
                s_status.step = WELD_STEP_2_PRESS_RIGHT;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_press_right(true);
            }
            break;

        case WELD_STEP_2_PRESS_RIGHT:
            if (elapsed_ms >= 500) {
                s_status.step = WELD_STEP_3_SHABLON_RETRACT;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_shablon(false); // Retract zero shablon
            }
            break;

        case WELD_STEP_3_SHABLON_RETRACT:
            if ((elapsed_ms >= 600 && hal_io_read_shablon_sensor() == JACK_SENSOR_RETRACTED) || elapsed_ms >= 1500) {
                // HEATER SAFETY INTERLOCK: Verify clamps
                s_status.step = WELD_STEP_4_ELEMENT_ADVANCE;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_element(true); // Advance heater plate
            } else if (elapsed_ms > 7000) {
                ESP_LOGE(TAG, "Shablon retract timeout! Aborting cycle.");
                fault_log_record(FAULT_SHABLON_TIMEOUT, 1);
                plc_engine_abort();
            }
            break;

        case WELD_STEP_4_ELEMENT_ADVANCE:
            if ((elapsed_ms >= 800 && hal_io_read_element_sensor() == JACK_SENSOR_EXTENDED) || elapsed_ms >= 2000) {
                s_status.step = WELD_STEP_5_TABLE_MELT_ADVANCE;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_table(true); // Advance table to touch heater
            } else if (elapsed_ms > 7000) {
                ESP_LOGE(TAG, "Element advance timeout! Aborting cycle.");
                fault_log_record(FAULT_ELEMENT_TIMEOUT, 2);
                plc_engine_abort();
            }
            break;

        case WELD_STEP_5_TABLE_MELT_ADVANCE:
            if ((elapsed_ms >= 600 && hal_io_read_table_sensor() == JACK_SENSOR_EXTENDED) || elapsed_ms >= 1500) {
                s_status.step = WELD_STEP_6_MELTING_COUNTDOWN;
                s_step_start_us = esp_timer_get_time();
                s_status.remaining_countdown_sec = s_cfg.weld_time_sec;
            } else if (elapsed_ms > 7000) {
                ESP_LOGE(TAG, "Table melt advance timeout! Aborting cycle.");
                fault_log_record(FAULT_TABLE_TIMEOUT, 3);
                plc_engine_abort();
            }
            break;

        case WELD_STEP_6_MELTING_COUNTDOWN: {
            uint32_t sec_elapsed = elapsed_ms / 1000;
            if (sec_elapsed < s_cfg.weld_time_sec) {
                s_status.remaining_countdown_sec = s_cfg.weld_time_sec - sec_elapsed;
            } else {
                s_status.step = WELD_STEP_7_TABLE_MELT_RETRACT;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_table(false); // Release table from heater
            }
            break;
        }

        case WELD_STEP_7_TABLE_MELT_RETRACT:
            if ((elapsed_ms >= 400 && hal_io_read_table_sensor() == JACK_SENSOR_RETRACTED) || elapsed_ms >= 1200) {
                s_status.step = WELD_STEP_8_ELEMENT_RETRACT;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_element(false); // Retract heater plate
            } else if (elapsed_ms > 7000) {
                ESP_LOGE(TAG, "Table melt retract timeout! Aborting cycle.");
                fault_log_record(FAULT_TABLE_TIMEOUT, 4);
                plc_engine_abort();
            }
            break;

        case WELD_STEP_8_ELEMENT_RETRACT:
            if ((elapsed_ms >= 800 && hal_io_read_element_sensor() == JACK_SENSOR_RETRACTED) || elapsed_ms >= 2000) {
                s_status.step = WELD_STEP_9_TABLE_FUSION_ADVANCE;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_table(true); // Advance table to press profiles together
            } else if (elapsed_ms > 7000) {
                ESP_LOGE(TAG, "Element retract timeout! Aborting cycle.");
                fault_log_record(FAULT_ELEMENT_TIMEOUT, 5);
                plc_engine_abort();
            }
            break;

        case WELD_STEP_9_TABLE_FUSION_ADVANCE:
            if ((elapsed_ms >= 500 && hal_io_read_table_sensor() == JACK_SENSOR_EXTENDED) || elapsed_ms >= 1500) {
                s_status.step = WELD_STEP_10_COOLING_COUNTDOWN;
                s_step_start_us = esp_timer_get_time();
                s_status.remaining_countdown_sec = s_cfg.cool_time_sec;
            } else if (elapsed_ms > 7000) {
                ESP_LOGE(TAG, "Table fusion advance timeout! Aborting cycle.");
                fault_log_record(FAULT_TABLE_TIMEOUT, 6);
                plc_engine_abort();
            }
            break;

        case WELD_STEP_10_COOLING_COUNTDOWN: {
            uint32_t sec_elapsed = elapsed_ms / 1000;
            if (sec_elapsed < s_cfg.cool_time_sec) {
                s_status.remaining_countdown_sec = s_cfg.cool_time_sec - sec_elapsed;
            } else {
                s_status.step = WELD_STEP_11_STRIKER_ENGAGE;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_zarbeh1(true);
                hal_io_set_zarbeh2(true);
            }
            break;
        }

        case WELD_STEP_11_STRIKER_ENGAGE:
            if (elapsed_ms >= 400) {
                s_status.step = WELD_STEP_12_TABLE_RETRACT_CLAMPS_OPEN;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_zarbeh1(false);
                hal_io_set_zarbeh2(false);
                hal_io_set_table(false);
                hal_io_set_press_left(false);
                hal_io_set_press_right(false);
            }
            break;

        case WELD_STEP_12_TABLE_RETRACT_CLAMPS_OPEN:
            if (elapsed_ms >= 600) {
                s_status.step = WELD_STEP_13_SHABLON_RETURN_COMPLETE;
                s_step_start_us = esp_timer_get_time();
                hal_io_set_shablon(true); // Shablon back to home
                nvs_storage_record_cycle(s_cfg.weld_time_sec);
                hal_io_play_buzzer(BUZZER_TONE_CYCLE_END);
            }
            break;

        case WELD_STEP_13_SHABLON_RETURN_COMPLETE:
            if (elapsed_ms >= 500) {
                s_status.step = WELD_STEP_0_IDLE;
                s_status.in_progress = false;
                ESP_LOGI(TAG, "Welding Cycle Completed Successfully.");
            }
            break;

        default:
            s_status.step = WELD_STEP_0_IDLE;
            s_status.in_progress = false;
            break;
    }
}

plc_cycle_status_t plc_engine_get_status(void) {
    plc_cycle_status_t copy;
    if (s_status_mutex) xSemaphoreTake(s_status_mutex, portMAX_DELAY);
    copy = s_status;
    if (s_status_mutex) xSemaphoreGive(s_status_mutex);
    return copy;
}
