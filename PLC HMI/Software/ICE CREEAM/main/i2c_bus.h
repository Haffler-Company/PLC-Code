#pragma once
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

extern i2c_master_bus_handle_t g_i2c_bus;

esp_err_t i2c_bus_init(int port, int sda_gpio, int scl_gpio, uint32_t hz, bool use_internal_pullup);
void i2c_bus_scan(void); // اختیاری برای تست

#ifdef __cplusplus
}
#endif
