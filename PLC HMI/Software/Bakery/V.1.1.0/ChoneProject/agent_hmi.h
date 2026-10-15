#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "screenshot.h"

extern volatile bool g_agent_touch_active;
extern volatile int16_t g_agent_touch_x;
extern volatile int16_t g_agent_touch_y;
extern volatile uint32_t g_agent_touch_release_ms;

void agent_hmi_init();
void agent_hmi_poll();
void agent_navigate(const char *screen_name);
