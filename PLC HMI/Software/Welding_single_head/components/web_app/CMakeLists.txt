idf_component_register(
    SRCS "src/web_app.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server esp_wifi esp_spiffs plc_engine nvs_storage hal_thermo freertos
)
