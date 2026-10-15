#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "screenshot.h"
#include "agent_hmi.h"

// Shared framebuffer state across translation units
uint16_t *shot_buf = nullptr;
volatile uint8_t shot_state = 0;
volatile bool shot_web_owner = false;
Stream *shot_port = nullptr;
uint32_t shot_t0 = 0;
uint16_t shot_rows_copied = 0;
uint8_t shot_row_seen[(SHOT_H + 7) / 8] = {0};
TaskHandle_t shot_worker_task_handle = nullptr;

// Shared agent touch & navigation state across translation units
volatile bool g_agent_touch_active = false;
volatile int16_t g_agent_touch_x = 0;
volatile int16_t g_agent_touch_y = 0;
volatile uint32_t g_agent_touch_release_ms = 0;
volatile int g_agent_nav_request = 0;
