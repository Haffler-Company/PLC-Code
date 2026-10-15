idf_component_register(
    SRCS "src/hal_io.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_timer freertos
)
