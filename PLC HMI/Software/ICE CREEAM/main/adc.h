#pragma once
#include <stdint.h>
#include "esp_err.h"

#define IO1_PIN   (1)   // GPIO1 -> ADC1_CHANNEL_0
#define IO2_PIN   (2)   // GPIO2 -> ADC1_CHANNEL_1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

extern bool g_boot_f5_ota_requested;

esp_err_t adc_init(void);
bool adc_boot_check_f5_hold(void);

esp_err_t adc_start(uint32_t period_ms, int samples_per_read);

void adc_stop(void);

int adc_get_io1(void);
int adc_get_io2(void);
int adc_get_gpio4(void);
int adc_get_gpio5(void);
int adc_get_gpio6(void);
int adc_sample_gpio4(void);
int adc_sample_gpio5(void);
int adc_sample_gpio6(void);

void adc_set_period(uint32_t period_ms);
void adc_set_samples(int samples_per_read);

#ifdef __cplusplus
}
#endif
