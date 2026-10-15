/**
 * @file nvs_storage.cpp
 * @brief NVS Flash Storage Implementation
 */

#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NVS_STORAGE";
#define NVS_NAMESPACE "welder_cfg"

esp_err_t nvs_storage_init(void) {
    ESP_LOGI(TAG, "Initializing NVS Flash Storage...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, re-initializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t nvs_storage_load_config(nvs_machine_config_t *cfg) {
    if (!cfg) return ESP_ERR_INVALID_ARG;

    // Set Defaults
    cfg->target_temp_c = 250;
    cfg->temp_cali_offset_c = 0;
    cfg->weld_time_sec = 25;
    cfg->cool_time_sec = 25;
    cfg->table_wait_ms = 500;
    cfg->language = 0;
    cfg->screensaver_min = 30;
    cfg->admin_pass = 1234;
    cfg->firmware_pass = 5678;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(nvs_machine_config_t);
    err = nvs_get_blob(handle, "cfg", cfg, &size);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_save_config(const nvs_machine_config_t *cfg) {
    if (!cfg) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, "cfg", cfg, sizeof(nvs_machine_config_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_load_stats(nvs_production_stats_t *stats) {
    if (!stats) return ESP_ERR_INVALID_ARG;
    memset(stats, 0, sizeof(nvs_production_stats_t));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(nvs_production_stats_t);
    err = nvs_get_blob(handle, "stats", stats, &size);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_save_stats(const nvs_production_stats_t *stats) {
    if (!stats) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, "stats", stats, sizeof(nvs_production_stats_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_record_cycle(uint32_t weld_seconds) {
    nvs_production_stats_t stats;
    nvs_storage_load_stats(&stats);

    stats.daily_sash_count++;
    stats.daily_window_count = stats.daily_sash_count / 4;
    stats.daily_total_weld_sec += weld_seconds;
    stats.total_lifetime_cycles++;
    stats.teflon_wear_cycles++;

    return nvs_storage_save_stats(&stats);
}

esp_err_t nvs_storage_reset_teflon_counter(void) {
    nvs_production_stats_t stats;
    nvs_storage_load_stats(&stats);
    stats.teflon_wear_cycles = 0;
    return nvs_storage_save_stats(&stats);
}

esp_err_t nvs_storage_load_pinmap(nvs_pinmap_t *pinmap) {
    if (!pinmap) return ESP_ERR_INVALID_ARG;

    // Defaults
    pinmap->ssr = 15;
    pinmap->table = 16;
    pinmap->press_left = 42;
    pinmap->press_right = 39;
    pinmap->element = 48;
    pinmap->shablon = 40;
    pinmap->zarbeh1 = 35;
    pinmap->zarbeh2 = 36;
    pinmap->emergency = 3;
    pinmap->pedal = 7;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(nvs_pinmap_t);
    err = nvs_get_blob(handle, "pinmap", pinmap, &size);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_save_pinmap(const nvs_pinmap_t *pinmap) {
    if (!pinmap) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, "pinmap", pinmap, sizeof(nvs_pinmap_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}
