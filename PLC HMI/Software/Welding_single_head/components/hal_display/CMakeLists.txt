idf_component_register(
    SRCS "src/hal_display.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd driver lvgl esp_timer freertos
)
