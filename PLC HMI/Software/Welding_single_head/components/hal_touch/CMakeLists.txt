idf_component_register(
    SRCS "src/hal_touch.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver hal_io lvgl esp_timer freertos
)
