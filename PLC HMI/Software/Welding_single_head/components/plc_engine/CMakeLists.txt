idf_component_register(
    SRCS "src/plc_engine.cpp"
    INCLUDE_DIRS "include"
    REQUIRES hal_io hal_thermo nvs_storage thermal_pid esp_timer freertos
)
