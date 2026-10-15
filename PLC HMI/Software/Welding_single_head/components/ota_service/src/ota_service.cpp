/**
 * @file ota_service.cpp
 * @brief Fail-Safe HTTPS OTA Update Engine Implementation
 */

#include "ota_service.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "OTA_SERVICE";

static ota_state_t s_ota_state = OTA_STATE_IDLE;
static ota_progress_callback_t s_progress_cb = NULL;
static char s_target_url[256] = {0};

esp_err_t ota_service_init(void) {
    ESP_LOGI(TAG, "Initializing OTA Service Subsystem...");
    s_ota_state = OTA_STATE_IDLE;
    return ESP_OK;
}

esp_err_t ota_service_confirm_valid_boot(void) {
    esp_ota_img_states_t ota_state;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "New firmware verified healthy -> Canceling bootloader rollback.");
            return esp_ota_mark_app_valid_cancel_rollback();
        }
    }
    return ESP_OK;
}

static void ota_worker_task(void *pvParameter) {
    ESP_LOGI(TAG, "Starting HTTPS OTA Download from: %s", s_target_url);
    s_ota_state = OTA_STATE_CONNECTING;

    esp_http_client_config_t http_config = {};
    http_config.url = s_target_url;
    http_config.timeout_ms = 15000;
    http_config.keep_alive_enable = true;
    http_config.skip_cert_common_name_check = true;

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &http_config;

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed (%s)", esp_err_to_name(err));
        s_ota_state = OTA_STATE_FAILED;
        vTaskDelete(NULL);
        return;
    }

    s_ota_state = OTA_STATE_DOWNLOADING;
    int total_bytes = esp_https_ota_get_image_size(https_ota_handle);
    int downloaded_bytes = 0;

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        downloaded_bytes = esp_https_ota_get_image_len_read(https_ota_handle);
        if (total_bytes > 0 && s_progress_cb) {
            uint8_t pct = (uint8_t)((downloaded_bytes * 100) / total_bytes);
            s_progress_cb(downloaded_bytes, total_bytes, pct);
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle)) {
        s_ota_state = OTA_STATE_VERIFYING;
        err = esp_https_ota_finish(https_ota_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA Download & Verification SUCCESS! Rebooting...");
            s_ota_state = OTA_STATE_SUCCESS;
            if (s_progress_cb) s_progress_cb(total_bytes, total_bytes, 100);
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Finish Failed (%s)", esp_err_to_name(err));
            s_ota_state = OTA_STATE_FAILED;
        }
    } else {
        ESP_LOGE(TAG, "OTA Data Incomplete or Connection Aborted");
        esp_https_ota_abort(https_ota_handle);
        s_ota_state = OTA_STATE_FAILED;
    }

    vTaskDelete(NULL);
}

esp_err_t ota_service_start_update(const char *url, ota_progress_callback_t cb) {
    if (!url || s_ota_state == OTA_STATE_DOWNLOADING) return ESP_ERR_INVALID_STATE;

    strncpy(s_target_url, url, sizeof(s_target_url) - 1);
    s_progress_cb = cb;

    xTaskCreatePinnedToCore(ota_worker_task, "ota_worker", 8192, NULL, 5, NULL, 0);
    return ESP_OK;
}

ota_state_t ota_service_get_state(void) {
    return s_ota_state;
}
