#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>

#define SHOT_W 480
#define SHOT_H 320
#define SHOT_RX_BUDGET_BYTES 16U
#define SHOT_TIMEOUT_MS 4000UL

extern uint16_t *shot_buf;
extern volatile uint8_t shot_state; // 0: idle, 1: requested, 2: copying, 3: ready, 4: web claimed
extern volatile bool shot_web_owner;
extern Stream *shot_port;
extern uint32_t shot_t0;
extern uint16_t shot_rows_copied;
extern uint8_t shot_row_seen[(SHOT_H + 7) / 8];
extern TaskHandle_t shot_worker_task_handle;

void shot_dump();
void shot_init_worker();
void shot_request(Stream *port);
bool shot_request_web();
const uint16_t *shot_claim_web_frame();
void shot_cancel_web_request();
void shot_release_web_frame();
void shot_on_flush(const lv_area_t *area, const uint16_t *px, bool last = false);
void shot_arm();
void shot_poll();
