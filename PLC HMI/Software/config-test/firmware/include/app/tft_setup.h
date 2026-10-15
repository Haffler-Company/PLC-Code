#pragma once
#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED
#endif

#define ST7796_DRIVER

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

#define TFT_MISO 11
#define TFT_MOSI 10
#define TFT_SCLK 12
#define TFT_CS   13
#define TFT_DC   14
#define TFT_RST  -1

#define USE_FSPI_PORT

#define SPI_FREQUENCY       80000000
#define SPI_READ_FREQUENCY  20000000

#define SUPPORT_TRANSACTIONS

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
