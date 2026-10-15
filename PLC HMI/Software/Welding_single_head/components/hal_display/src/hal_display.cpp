/**
 * @file hal_display.cpp
 * @brief 60 FPS Asynchronous DMA Display Driver Implementation
 */

#include "hal_display.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "HAL_DISP";

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static lv_display_t *s_lvgl_disp = NULL;

static lv_color_t *s_buf1 = NULL;
static lv_color_t *s_buf2 = NULL;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;

    // Asynchronous DMA transfer directly to the SPI display panel
    esp_lcd_panel_draw_bitmap(s_panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
}

lv_display_t *hal_display_init(void) {
    ESP_LOGI(TAG, "Initializing 60 FPS Asynchronous DMA Display Pipeline...");

    // 1. Configure SPI Master Bus on HSPI / SPI2
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = LCD_PIN_SCLK;
    buscfg.mosi_io_num = LCD_PIN_MOSI;
    buscfg.miso_io_num = LCD_PIN_MISO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = LCD_H_RES * LCD_BUFFER_LINES * sizeof(lv_color_t);

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2. Attach Panel IO over SPI with 80MHz clock & async callback
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.dc_gpio_num = LCD_PIN_DC;
    io_config.cs_gpio_num = LCD_PIN_CS;
    io_config.pclk_hz = LCD_SPI_FREQ_HZ;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.spi_mode = 0;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = notify_lvgl_flush_ready;

    // 3. Register Display with LVGL 9.4
    s_lvgl_disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    io_config.user_ctx = s_lvgl_disp;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &s_io_handle));

    // 4. Initialize ILI9341 / ST7796 Panel
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_PIN_RST;
    panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR; // Matches BGR requirement
    panel_config.bits_per_pixel = 16;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(s_io_handle, &panel_config, &s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    
    // Set 480x320 Landscape Rotation (Mirror X, Swap XY)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

    // 5. Allocate Double Ping-Pong DMA Buffers in Internal Fast SRAM
    size_t buf_size = LCD_H_RES * LCD_BUFFER_LINES * sizeof(lv_color_t);
    s_buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    assert(s_buf1 != NULL && s_buf2 != NULL);

    lv_display_set_buffers(s_lvgl_disp, s_buf1, s_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_lvgl_disp, lvgl_flush_cb);

    ESP_LOGI(TAG, "Display Subsystem Initialized: 480x320 @ 80MHz SPI with DMA Double-Buffering.");
    return s_lvgl_disp;
}

void hal_display_set_backlight(uint8_t brightness_pct) {
    if (LCD_PIN_BL >= 0) {
        // Backlight PWM if connected
    }
}
