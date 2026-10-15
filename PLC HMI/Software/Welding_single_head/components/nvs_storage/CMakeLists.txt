idf_component_register(
    SRCS "src/nvs_storage.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_timer freertos
)
