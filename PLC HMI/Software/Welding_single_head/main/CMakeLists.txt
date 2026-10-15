idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES hal_io hal_thermo hal_rtc hal_display hal_touch nvs_storage thermal_pid plc_engine web_app ota_service ui_engine lvgl freertos esp_timer
)
