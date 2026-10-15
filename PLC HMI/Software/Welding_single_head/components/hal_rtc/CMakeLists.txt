idf_component_register(
    SRCS "src/hal_rtc.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer freertos
)
