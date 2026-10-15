/**
 * @file ota_pre_erase.cpp
 * @brief Haffler High-Speed Zero-Lag OTA Engine - Background Flash Pre-Erase Implementation
 * @author Haffler Industrial Engineering
 */

#include "ota_pre_erase.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

static const char *TAG = "OTA_PRE_ERASE";

static volatile bool s_pre_erase_running = false;
static volatile bool s_pre_erase_done = false;
static const esp_partition_t *s_erased_partition = NULL;
static uint32_t s_erased_upto_offset = 0;
static uint32_t s_erase_target_size = 0;
static TaskHandle_t s_pre_erase_task_handle = NULL;

static void ota_pre_erase_worker(void *arg)
{
    (void)arg;
    s_pre_erase_running = true;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition) {
        if (s_erased_partition != update_partition) {
            s_erased_partition = update_partition;
            s_erased_upto_offset = 0;
            s_pre_erase_done = false;
        }
        const uint32_t BLOCK = 32768u;
        // Erase up to partition size (typically 3.5MB - 5.5MB)
        uint32_t erase_size = update_partition->size;
        s_erase_target_size = erase_size;

        ESP_LOGI(TAG, "Background pre-erase starting on partition %s (%u bytes, from offset 0x%x)...",
                 update_partition->label, (unsigned)erase_size, (unsigned)s_erased_upto_offset);

        for (uint32_t offset = s_erased_upto_offset; offset < erase_size; offset += BLOCK) {
            if (!s_pre_erase_running) break;
            uint32_t len = ((offset + BLOCK) <= erase_size) ? BLOCK : (erase_size - offset);
            esp_err_t er = esp_partition_erase_range(update_partition, offset, len);
            if (er != ESP_OK) {
                ESP_LOGE(TAG, "Pre-erase block failed at 0x%x: %s", (unsigned)offset, esp_err_to_name(er));
                break;
            }
            s_erased_upto_offset = offset + len;
            // 25ms yield to Core 0 tasks and Core 1 LVGL task to guarantee zero CPU starvation
            vTaskDelay(pdMS_TO_TICKS(25));
        }

        if (s_pre_erase_running && s_erased_upto_offset >= erase_size) {
            s_pre_erase_done = true;
            ESP_LOGI(TAG, "Background pre-erase COMPLETE! Target partition %s is 100%% clean.",
                     update_partition->label);
        }
    }

    s_pre_erase_running = false;
    s_pre_erase_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_pre_erase_start(void)
{
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGW(TAG, "No valid passive OTA partition found to pre-erase");
        return;
    }

    if (s_erased_partition != update_partition) {
        s_erased_partition = update_partition;
        s_erased_upto_offset = 0;
        s_pre_erase_done = false;
    }

    if (s_pre_erase_done || s_pre_erase_running || s_pre_erase_task_handle != NULL) {
        return;
    }

    ESP_LOGI(TAG, "Spawning ota_pre_erase background task on Core 0 (Priority 1)...");
    xTaskCreatePinnedToCore(ota_pre_erase_worker, "ota_pre_erase", 3584, NULL, 1, &s_pre_erase_task_handle, 0);
}

void ota_pre_erase_stop(void)
{
    if (s_pre_erase_running) {
        s_pre_erase_running = false;
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

bool ota_is_pre_erase_done(void)
{
    return s_pre_erase_done;
}

void ota_pre_erase_ensure_clean_blocking(void)
{
    ota_pre_erase_stop();

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) return;

    if (s_erased_partition != update_partition) {
        s_erased_partition = update_partition;
        s_erased_upto_offset = 0;
        s_pre_erase_done = false;
    }

    if (s_pre_erase_done && s_erased_upto_offset >= update_partition->size) {
        return;
    }

    const uint32_t BLOCK = 32768u;
    uint32_t erase_size = update_partition->size;
    ESP_LOGI(TAG, "Clean Flash Guarantee: Finishing remaining pre-erase (0x%x to 0x%x) BEFORE opening HTTP...",
             (unsigned)s_erased_upto_offset, (unsigned)erase_size);

    for (uint32_t offset = s_erased_upto_offset; offset < erase_size; offset += BLOCK) {
        uint32_t len = ((offset + BLOCK) <= erase_size) ? BLOCK : (erase_size - offset);
        esp_partition_erase_range(update_partition, offset, len);
        s_erased_upto_offset = offset + len;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    s_pre_erase_done = true;
    ESP_LOGI(TAG, "Clean Flash Guarantee achieved. Partition 100%% clean.");
}

uint8_t ota_pre_erase_get_progress(void)
{
    if (s_pre_erase_done) return 100;
    if (s_erase_target_size == 0) return 0;
    uint32_t pct = (s_erased_upto_offset * 100u) / s_erase_target_size;
    return (pct > 100u) ? 100u : (uint8_t)pct;
}
