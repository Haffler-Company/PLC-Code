#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

void buzzer_init(gpio_num_t gpio, bool active_high);
void buzzer_set_enabled(bool en);
bool buzzer_is_enabled(void);
void buzzer_set_volume(uint8_t vol);
uint8_t buzzer_get_volume(void);

void buzz(uint32_t duration_ms);
void buzzer_beep_click(void);
void buzzer_beep_confirm(void);
void buzzer_beep_error(void);
void buzzer_beep_alarm(void);
void buzzer_silence(void);

#ifdef __cplusplus
}
#endif
