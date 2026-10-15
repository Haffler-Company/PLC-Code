idf_component_register(
    SRCS "src/hal_thermo.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver hal_io esp_timer freertos
)
