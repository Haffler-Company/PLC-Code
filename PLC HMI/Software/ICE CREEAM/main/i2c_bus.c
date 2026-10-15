#include "i2c_bus.h"
#include "esp_log.h"

i2c_master_bus_handle_t g_i2c_bus = NULL;
static const char *TAG = "I2C_BUS";

esp_err_t i2c_bus_init(int port, int sda_gpio, int scl_gpio, uint32_t hz, bool use_internal_pullup)
{
    if (g_i2c_bus) return ESP_OK;

    i2c_master_bus_config_t cfg = {
        .i2c_port = port,
        .sda_io_num = sda_gpio,
        .scl_io_num = scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = use_internal_pullup
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &g_i2c_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "I2C bus ready: port=%d SDA=%d SCL=%d", port, sda_gpio, scl_gpio);
    return ESP_OK;
}

void i2c_bus_scan(void)
{
    if (!g_i2c_bus) { ESP_LOGW(TAG, "scan: bus not inited"); return; }
    ESP_LOGI(TAG, "I2C scan...");
    for (uint8_t a=1; a<0x7F; ++a) {
        if (i2c_master_probe(g_i2c_bus, a, 50) == ESP_OK) {
            ESP_LOGI(TAG, "  found 0x%02X", a);
        }
    }
    ESP_LOGI(TAG, "I2C scan done.");
}
