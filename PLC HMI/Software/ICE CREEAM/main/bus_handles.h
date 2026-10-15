#pragma once
#include "driver/spi_master.h"

// هندل SPI که LCD ازش استفاده می‌کنه (توسط main مقداردهی می‌شود)
extern spi_device_handle_t g_spi_lcd;
