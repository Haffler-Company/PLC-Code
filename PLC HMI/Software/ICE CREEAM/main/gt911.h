#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include <stdint.h>
#include "driver/i2c_master.h"

esp_err_t i2c_master_init(void);
i2c_master_bus_handle_t i2c_get_bus(void);
bool gt911_init(void);
bool gt911_get_touch(uint16_t *x, uint16_t *y);
