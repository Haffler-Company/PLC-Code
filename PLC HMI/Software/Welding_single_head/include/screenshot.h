// Serial framebuffer screenshot — Async Zero-Lag Non-Blocking Hardware Driver
// ─────────────────────────────────────────────────────────────────────────────
// "SHOT\n" or "$WELD,...,SHOT,...*<HMAC>" on Serial (UART0/CP210x, 921600) ->
// the screen is invalidated, flushed regions accumulated directly into PSRAM via
// shot_on_flush(), then streamed via background worker task on Core 0 without
// blocking the UI thread (Core 1) or causing frame drops on the LCD.
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
static inline void shot_dump();

static void shot_worker_task(void *pvParameters)
{
    (void)pvParameters;
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static char line[SHOT_W * 2 * 4 / 3 + 4];

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!shot_buf || !shot_port || shot_state != 3) {
            continue;
        }

        Stream *p = shot_port;
        p->printf("SHOT_BEGIN %d %d RGB565LE\n", SHOT_W, SHOT_H);
        p->flush();

        const uint8_t *raw = (const uint8_t *)shot_buf;
        for (int y = 0; y < SHOT_H; y++) {
            const uint8_t *row = &raw[y * SHOT_W * 2];
            int o = 0;
            for (int i = 0; i < SHOT_W * 2; i += 3) {
                uint32_t v = ((uint32_t)row[i] << 16) | ((uint32_t)row[i + 1] << 8) | row[i + 2];
                line[o++] = b64[(v >> 18) & 63];
                line[o++] = b64[(v >> 12) & 63];
                line[o++] = b64[(v >> 6) & 63];
                line[o++] = b64[v & 63];
            }
            line[o++] = '\n';
            p->write((const uint8_t *)line, o);
            p->flush();
            vTaskDelay(pdMS_TO_TICKS(1)); // ۱ میلی‌ثانیه تخلیه بافر برای هر سطر جهت پایداری بدون سرریز UART
        }
        p->println("SHOT_END");
        p->flush();

        if (shot_buf) {
            heap_caps_free(shot_buf);
            shot_buf = nullptr;
        }
        shot_port = nullptr;
        shot_state = 0;
    }
}

static inline void shot_init_worker()
{
    if (!shot_worker_task_handle) {
        xTaskCreatePinnedToCore(shot_worker_task, "ShotWorker", 4096, NULL, 1, &shot_worker_task_handle, 0);
    }
}

static inline void shot_request(Stream *port)
{
    if (shot_state != 0) {
        if (millis() - shot_t0 > 1000) {
            if (shot_buf) {
                heap_caps_free(shot_buf);
                shot_buf = nullptr;
            }
            shot_state = 0;
        } else {
            return;
        }
    }
    shot_port = port;
    shot_web_owner = false;
    shot_state = 1;
}

static inline bool shot_request_web()
{
    if (shot_state != 0) return false;
    shot_port = nullptr;
    shot_web_owner = true;
    shot_state = 1;
    return true;
}

static inline const uint16_t *shot_claim_web_frame()
{
    if (shot_state == 3 && shot_web_owner && shot_buf) {
        shot_state = 4;
        return shot_buf;
    }
    return nullptr;
}

static inline void shot_cancel_web_request()
{
    if (shot_web_owner && shot_state == 1) {
        shot_port = nullptr;
        shot_web_owner = false;
        shot_state = 0;
    }
}

static inline void shot_release_web_frame()
{
    if (shot_state == 4 && shot_web_owner) {
        if (shot_buf) heap_caps_free(shot_buf);
        shot_buf = nullptr;
        shot_port = nullptr;
        shot_web_owner = false;
        shot_state = 0;
    }
}

static inline void shot_on_flush(const lv_area_t *area, const uint16_t *px, bool last = false)
{
    if (shot_state != 2 || !shot_buf || !area || !px) return;
    int x1 = area->x1 < 0 ? 0 : area->x1;
    int x2 = area->x2 >= SHOT_W ? SHOT_W - 1 : area->x2;
    int w_src = area->x2 - area->x1 + 1;
    if (x2 < x1 || w_src <= 0) return;

    for (int y = area->y1; y <= area->y2; y++) {
        if (y < 0 || y >= SHOT_H) {
            px += w_src;
            continue;
        }
        uint16_t *dst = &shot_buf[y * SHOT_W + x1];
        const uint16_t *src = px + (x1 - area->x1);
        int copy_w = x2 - x1 + 1;
        memcpy(dst, src, copy_w * sizeof(uint16_t));
        px += w_src;

        uint8_t mask = (uint8_t)(1u << (y & 7));
        uint16_t idx = (uint16_t)(y >> 3);
        if (!(shot_row_seen[idx] & mask)) {
            shot_row_seen[idx] |= mask;
            shot_rows_copied++;
        }
    }

    if (last || shot_rows_copied >= (SHOT_H - 10)) {
        shot_state = 3;
        shot_dump();
    }
}

static inline void shot_arm()
{
    if (shot_state != 1) return;
    shot_t0 = millis();
    shot_rows_copied = 0;
    memset(shot_row_seen, 0, sizeof(shot_row_seen));
    if (!shot_buf) {
        shot_buf = (uint16_t *)heap_caps_malloc(SHOT_W * SHOT_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (!shot_buf) {
        shot_buf = (uint16_t *)malloc(SHOT_W * SHOT_H * 2);
    }
    if (!shot_buf) {
        if (shot_port) shot_port->println("SHOT_ERR alloc");
        shot_port = nullptr;
        shot_state = 0;
        return;
    }
    shot_state = 2; // در حال ضبط پیکسل‌ها هنگام Flush
    lv_obj_t *act_scr = lv_screen_active();
    if (act_scr && lv_obj_is_valid(act_scr)) {
        lv_area_t full = {0, 0, SHOT_W - 1, SHOT_H - 1};
        lv_obj_invalidate_area(act_scr, &full);
        lv_obj_invalidate(act_scr);
    }
    lv_obj_t *top_l = lv_layer_top();
    if (top_l && lv_obj_is_valid(top_l)) lv_obj_invalidate(top_l);
    lv_obj_t *sys_l = lv_layer_sys();
    if (sys_l && lv_obj_is_valid(sys_l)) lv_obj_invalidate(sys_l);
}

static inline void shot_dump()
{
    if (shot_state != 3 || !shot_buf) return;
    if (shot_web_owner) return;
    if (!shot_port) return;
    shot_init_worker();
    if (shot_worker_task_handle) {
        xTaskNotifyGive(shot_worker_task_handle);
    }
}

static inline void shot_poll()
{
    if (shot_state == 1) {
        shot_arm();
    }
    if (shot_state == 2) {
        if (shot_rows_copied >= (SHOT_H - 4)) {
            shot_state = 3;
            shot_dump();
        } else if (millis() - shot_t0 > SHOT_TIMEOUT_MS) {
            if (shot_rows_copied > 0) {
                // Screen flush finished or partially captured, dump!
                shot_state = 3;
                shot_dump();
            } else {
                if (shot_port) shot_port->printf("SHOT_ERR timeout rows=%u\n", (unsigned)shot_rows_copied);
                if (shot_buf) {
                    heap_caps_free(shot_buf);
                    shot_buf = nullptr;
                }
                shot_port = nullptr;
                shot_state = 0;
            }
        }
    }
}
