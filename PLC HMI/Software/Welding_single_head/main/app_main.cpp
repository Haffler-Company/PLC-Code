/**
 * @file app_main.cpp
 * @brief Industrial ESP-IDF Native Entry Point for Single-Head UPVC Welding PLC/HMI
 * @author Haffler Automation Systems
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#include "hal_io.h"
#include "hal_thermo.h"
#include "hal_rtc.h"
#include "hal_display.h"
#include "hal_touch.h"
#include "nvs_storage.h"
#include "plc_engine.h"
#include "ui_engine.h"
#include "web_app.h"
#include "ota_service.h"
#include "lvgl.h"

static const char *TAG = "APP_MAIN";

// UI / LVGL Task on Core 1 (Isolated from PLC and Network)
static void lvgl_task(void *pvParameter) {
    ESP_LOGI(TAG, "Starting 60 FPS LVGL & UI Thread on Core 1...");
    
    // Initialize Display, Touch & EEZ Studio Screens
    hal_display_init();
    hal_touch_init();
    ui_engine_init();

    uint32_t last_tick = (uint32_t)(esp_timer_get_time() / 1000ULL);

    while (1) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        lv_tick_inc(now - last_tick);
        last_tick = now;

        // Run UI variable sync & screen tick
        ui_engine_tick();

        uint32_t time_till_next = lv_timer_handler();
        if (time_till_next < 1) time_till_next = 1;
        if (time_till_next > 8) time_till_next = 8; // Clamped to 8ms for 60-120 FPS responsiveness

        vTaskDelay(pdMS_TO_TICKS(time_till_next));
    }
}

// PLC Machine & Process Safety Task on Core 0 (Priority 10)
static void plc_task(void *pvParameter) {
    ESP_LOGI(TAG, "Starting Industrial PLC & Process Safety Thread on Core 0...");
    plc_engine_init();

    while (1) {
        // Poll Pedal
        if (hal_io_is_pedal_pressed()) {
            plc_engine_pedal_trigger();
        }

        // Run 14-Step FSM & Thermal Safety Loop
        plc_engine_tick();

        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz deterministic PLC loop
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, "  Haffler Single-Head UPVC Welding PLC/HMI System (ESP-IDF) ");
    ESP_LOGI(TAG, "  Standards: Siemens Industrial Safety + Apple HIG 60 FPS   ");
    ESP_LOGI(TAG, "==========================================================");

    // 1. Initialize NVS Flash
    ESP_ERROR_CHECK(nvs_storage_init());

    // 2. Cancel Bootloader Rollback on Verified Boot
    ESP_ERROR_CHECK(ota_service_confirm_valid_boot());

    // 3. Initialize Hardware HAL
    ESP_ERROR_CHECK(hal_io_init());
    ESP_ERROR_CHECK(hal_thermo_init());
    ESP_ERROR_CHECK(hal_rtc_init());
    ESP_ERROR_CHECK(web_app_init());
    ESP_ERROR_CHECK(ota_service_init());

    // 4. Print Memory Diagnostics
    ESP_LOGI(TAG, "Free Internal SRAM: %u bytes", (unsigned int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "Free PSRAM (SPIRAM): %u bytes", (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // 5. Initialize LVGL Engine
    lv_init();

    // 6. Spawn Core 1 UI Thread
    xTaskCreatePinnedToCore(lvgl_task, "LVGL_UI", 8192, NULL, 5, NULL, 1);

    // 7. Spawn Core 0 Industrial PLC Safety Thread
    xTaskCreatePinnedToCore(plc_task, "PLC_Core", 8192, NULL, 10, NULL, 0);

    ESP_LOGI(TAG, "System Startup Completed Successfully. All Cores Active.");
}
