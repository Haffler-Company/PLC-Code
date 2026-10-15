idf_component_register(
    SRCS "src/ota_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_https_ota app_update esp_http_client mbedtls esp_timer freertos
)
