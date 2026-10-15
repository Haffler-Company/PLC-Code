#pragma once

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

void agent_hmi_init(void);
void agent_hmi_poll(Stream &serial_port);
void agent_hmi_inject_touch(int16_t x, int16_t y, uint32_t duration_ms);
bool agent_hmi_get_touch(int16_t *x, int16_t *y);

#ifdef __cplusplus
}
#endif
