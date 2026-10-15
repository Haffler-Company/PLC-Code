#include "adc.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"

static const char *TAG = "ADC";

#define ADC_CH_IO1   ADC_CHANNEL_0   // GPIO1 (Keypad line 1)
#define ADC_CH_IO2   ADC_CHANNEL_1   // GPIO2 (Keypad line 2)

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static volatile int s_io1_val = 0;
static volatile int s_io2_val = 0;
static volatile int s_gpio4_val = 2048;
static volatile int s_gpio5_val = 2048;
static volatile int s_gpio6_val = 2048;
static uint32_t s_period_ms = 20;
static int      s_samples   = 4;

static SemaphoreHandle_t s_adc_mutex = NULL;

static inline bool adc_lock(void)
{
    if (!s_adc_mutex) {
        s_adc_mutex = xSemaphoreCreateMutex();
    }
    if (!s_adc_mutex) return true;
    return (xSemaphoreTake(s_adc_mutex, pdMS_TO_TICKS(50)) == pdTRUE);
}

static inline void adc_unlock(void)
{
    if (s_adc_mutex) {
        xSemaphoreGive(s_adc_mutex);
    }
}

static int adc_read_median5(adc_channel_t ch)
{
    if (!s_adc1_handle) return 0;

    int samples[5] = {0};
    int valid_cnt = 0;

    for (int i = 0; i < 5; i++) {
        int r = 0;
        if (adc_lock()) {
            esp_err_t err = adc_oneshot_read(s_adc1_handle, ch, &r);
            adc_unlock();
            if (err == ESP_OK) {
                samples[valid_cnt++] = r;
            }
        }
        esp_rom_delay_us(25);
    }

    if (valid_cnt == 0) return 0;

    /* مرتب‌سازی حبابی ۵ نمونه جهت استخراج میانه واقعی (Median) */
    for (int i = 0; i < valid_cnt - 1; i++) {
        for (int j = i + 1; j < valid_cnt; j++) {
            if (samples[j] < samples[i]) {
                int temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }

    return samples[valid_cnt / 2];
}

static int adc_read_avg(adc_channel_t ch, int samples)
{
    if (!s_adc1_handle || samples <= 0) return 0;
    if (samples > 8) samples = 8;

    int total = 0;
    int valid_cnt = 0;

    for (int i = 0; i < samples; i++) {
        int r = 0;
        if (adc_lock()) {
            esp_err_t err = adc_oneshot_read(s_adc1_handle, ch, &r);
            adc_unlock();
            if (err == ESP_OK) {
                total += r;
                valid_cnt++;
            }
        }
    }

    return valid_cnt > 0 ? (total / valid_cnt) : 0;
}

esp_err_t adc_init(void)
{
    if (s_adc1_handle) return ESP_OK;

    if (!s_adc_mutex) {
        s_adc_mutex = xSemaphoreCreateMutex();
    }

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&init_config1, &s_adc1_handle), TAG, "new unit");

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc1_handle, ADC_CH_IO1, &chan_config), TAG, "atten io1");
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc1_handle, ADC_CH_IO2, &chan_config), TAG, "atten io2");
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_3, &chan_config), TAG, "atten gpio4");
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_4, &chan_config), TAG, "atten gpio5");
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_5, &chan_config), TAG, "atten gpio6");

    /* پیکربندی پین‌های کی‌پد به صورت شناور مطابق استاندارد Cutter Task9 جهت ممانعت از خطای تقسیم ولتاژ نردبان مقاومتی */
    gpio_set_pull_mode((gpio_num_t)IO1_PIN, GPIO_FLOATING);
    gpio_set_pull_mode((gpio_num_t)IO2_PIN, GPIO_FLOATING);

    ESP_LOGI(TAG, "Modern ADC1 Oneshot Initialized: Keypad (IO1=CH0, IO2=CH1) + Inputs (GPIO4=CH3, GPIO5=CH4, GPIO6=CH5).");
    return ESP_OK;
}

bool adc_boot_check_f5_hold(void)
{
    if (!s_adc1_handle) {
        adc_init();
    }

    gpio_set_pull_mode((gpio_num_t)IO1_PIN, GPIO_FLOATING);
    gpio_set_pull_mode((gpio_num_t)IO2_PIN, GPIO_FLOATING);
    /* ۱۵ نمونه × ۲۰ms = ۳۰۰ms — کافی برای تشخیص فشردن پیوسته کلید F5 */

    int hits = 0, run = 0, best_run = 0;
    int last_v1 = 0, last_v2 = 0;

    for (int i = 0; i < 15; i++) {
        int v1 = 0, v2 = 0;
        esp_err_t err1 = ESP_FAIL;
        esp_err_t err2 = ESP_FAIL;
        if (adc_lock()) {
            err1 = adc_oneshot_read(s_adc1_handle, ADC_CH_IO1, &v1);
            err2 = adc_oneshot_read(s_adc1_handle, ADC_CH_IO2, &v2);
            adc_unlock();
        }
        if (err1 == ESP_OK) last_v1 = v1;
        if (err2 == ESP_OK) last_v2 = v2;

        bool f5_detected = ((err1 == ESP_OK && v1 >= 350 && v1 <= 1850) ||
                            (err2 == ESP_OK && v2 >= 350 && v2 <= 1850));
        if (f5_detected) {
            hits++;
            run++;
            if (run > best_run) best_run = run;
        } else {
            run = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /* شرط احراز: حداقل ۳ نمونه و ۳ تکرار متوالی */
    bool held = (hits >= 3 && best_run >= 3);
    ESP_LOGI(TAG, "[BOOT-F5] sample_io1=%d sample_io2=%d hits=%d/15 best_run=%d -> f5_held=%d",
             last_v1, last_v2, hits, best_run, (int)held);
    return held;
}

esp_err_t adc_start(uint32_t period_ms, int samples_per_read)
{
    if (period_ms) s_period_ms = period_ms;
    if (samples_per_read > 0) s_samples = samples_per_read;
    return ESP_OK;
}

void adc_stop(void)
{
}

int adc_sample_gpio4(void)
{
    /* نمونه‌برداری حداکثر هر 20ms یک‌بار جهت هماهنگی با اسکن سریع PLC */
    static int64_t s_last_sample4 = 0;
    int64_t now = esp_timer_get_time() / 1000LL;
    if (now - s_last_sample4 >= 20) {
        s_last_sample4 = now;
        int v = adc_read_avg(ADC_CHANNEL_3, s_samples);
        /* C2: write اتمیک روی ESP32-S3 (32-bit aligned) — spinlock غیرضروری است */
        s_gpio4_val = v;
    }
    return s_gpio4_val;
}

int adc_sample_gpio5(void)
{
    static int64_t s_last_sample5 = 0;
    int64_t now = esp_timer_get_time() / 1000LL;
    if (now - s_last_sample5 >= 20) {
        s_last_sample5 = now;
        int v = adc_read_avg(ADC_CHANNEL_4, s_samples);
        s_gpio5_val = v;
    }
    return s_gpio5_val;
}

int adc_sample_gpio6(void)
{
    static int64_t s_last_sample6 = 0;
    int64_t now = esp_timer_get_time() / 1000LL;
    if (now - s_last_sample6 >= 20) {
        s_last_sample6 = now;
        int v = adc_read_avg(ADC_CHANNEL_5, s_samples);
        s_gpio6_val = v;
    }
    return s_gpio6_val;
}

int adc_get_io1(void)
{
    int v = adc_read_median5(ADC_CH_IO1);
    s_io1_val = v;
    return v;
}

int adc_get_io2(void)
{
    int v = adc_read_median5(ADC_CH_IO2);
    s_io2_val = v;
    return v;
}

int adc_get_gpio4(void) { return adc_sample_gpio4(); }
int adc_get_gpio5(void) { return adc_sample_gpio5(); }
int adc_get_gpio6(void) { return adc_sample_gpio6(); }

void adc_set_period(uint32_t period_ms)
{
    if (period_ms) s_period_ms = period_ms;
}

void adc_set_samples(int samples_per_read)
{
    if (samples_per_read > 0) s_samples = samples_per_read;
}
