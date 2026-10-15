/**
 * @file nvs_storage.h
 * @brief Industrial NVS Flash Storage Subsystem for Welding Machine Calibration & Recipes
 * @author Haffler Automation Systems
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t target_temp_c;       // Default: 250 °C
    int16_t  temp_cali_offset_c;  // Default: 0 °C
    uint16_t weld_time_sec;       // Default: 25 s
    uint16_t cool_time_sec;       // Default: 25 s
    uint16_t table_wait_ms;       // Default: 500 ms
    uint8_t  language;            // 0: FA, 1: EN, 2: TR
    uint16_t screensaver_min;     // Default: 30 min
    uint32_t admin_pass;          // Default: 1234
    uint32_t firmware_pass;       // Default: 5678
} nvs_machine_config_t;

typedef struct {
    uint32_t daily_sash_count;
    uint32_t daily_window_count;
    uint32_t daily_total_weld_sec;
    uint32_t total_lifetime_cycles;
    uint32_t teflon_wear_cycles;
    uint32_t last_stat_date_int; // YYYYMMDD
} nvs_production_stats_t;

typedef struct {
    int ssr;
    int table;
    int press_left;
    int press_right;
    int element;
    int shablon;
    int zarbeh1;
    int zarbeh2;
    int emergency;
    int pedal;
} nvs_pinmap_t;

esp_err_t nvs_storage_init(void);

esp_err_t nvs_storage_load_config(nvs_machine_config_t *cfg);
esp_err_t nvs_storage_save_config(const nvs_machine_config_t *cfg);

esp_err_t nvs_storage_load_stats(nvs_production_stats_t *stats);
esp_err_t nvs_storage_save_stats(const nvs_production_stats_t *stats);
esp_err_t nvs_storage_record_cycle(uint32_t weld_seconds);
esp_err_t nvs_storage_reset_teflon_counter(void);

esp_err_t nvs_storage_load_pinmap(nvs_pinmap_t *pinmap);
esp_err_t nvs_storage_save_pinmap(const nvs_pinmap_t *pinmap);

#ifdef __cplusplus
}
#endif
