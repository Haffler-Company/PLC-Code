#include "screenshot.h"

uint16_t *shot_buf = nullptr;
volatile uint8_t shot_state = 0; // 0: idle, 1: requested, 2: copying, 3: ready, 4: web claimed
volatile bool shot_web_owner = false;
Stream *shot_port = nullptr;
uint32_t shot_t0 = 0;
uint16_t shot_rows_copied = 0;
uint8_t shot_row_seen[(SHOT_H + 7) / 8] = {0};
TaskHandle_t shot_worker_task_handle = nullptr;
static volatile bool shot_is_raw = false;

static void shot_worker_task(void *pvParameters)
{
    (void)pvParameters;
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static char line[SHOT_W * 2 * 4 / 3 + 4];

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!shot_buf || !shot_port || shot_state != 3)
        {
            continue;
        }

        Stream *p = shot_port;

        if (shot_is_raw)
        {
            uint32_t total_bytes = (uint32_t)SHOT_W * (uint32_t)SHOT_H * 2UL;
            p->printf("RAW_BEGIN %d %d %lu\n", SHOT_W, SHOT_H, (unsigned long)total_bytes);
            p->flush();

            const uint8_t *raw = (const uint8_t *)shot_buf;
            size_t remaining = total_bytes;
            while (remaining > 0)
            {
                size_t chunk = (remaining > 4096) ? 4096 : remaining;
                p->write(raw, chunk);
                p->flush();
                raw += chunk;
                remaining -= chunk;
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            p->print("\nRAW_END\n");
            p->flush();
        }
        else
        {
            p->printf("SHOT_BEGIN %d %d RGB565LE\n", SHOT_W, SHOT_H);
            p->flush();

            const uint8_t *raw = (const uint8_t *)shot_buf;
            for (int y = 0; y < SHOT_H; y++)
            {
                const uint8_t *row = &raw[y * SHOT_W * 2];
                int o = 0;
                for (int i = 0; i < SHOT_W * 2; i += 3)
                {
                    uint32_t v = ((uint32_t)row[i] << 16) | ((uint32_t)row[i + 1] << 8) | row[i + 2];
                    line[o++] = b64[(v >> 18) & 63];
                    line[o++] = b64[(v >> 12) & 63];
                    line[o++] = b64[(v >> 6) & 63];
                    line[o++] = b64[v & 63];
                }
                line[o++] = '\n';
                p->write((const uint8_t *)line, o);
                p->flush();
                vTaskDelay(pdMS_TO_TICKS(1)); // 1ms delay per row to prevent UART overflow
            }
            p->println("SHOT_END");
            p->flush();
        }

        if (shot_buf)
        {
            heap_caps_free(shot_buf);
            shot_buf = nullptr;
        }
        shot_port = nullptr;
        shot_state = 0;
    }
}

void shot_init_worker()
{
    if (!shot_worker_task_handle)
    {
        xTaskCreatePinnedToCore(shot_worker_task, "ShotWorker", 4096, NULL, 1, &shot_worker_task_handle, 0);
    }
}

void shot_request(Stream *port)
{
    if (shot_state != 0) return;
    shot_port = port;
    shot_web_owner = false;
    shot_is_raw = false;
    shot_state = 1;
}

void shot_request_raw(Stream *port)
{
    if (shot_state != 0) return;
    shot_port = port;
    shot_web_owner = false;
    shot_is_raw = true;
    shot_state = 1;
}

bool shot_request_web()
{
    if (shot_state != 0) return false;
    shot_port = nullptr;
    shot_web_owner = true;
    shot_state = 1;
    return true;
}

const uint16_t *shot_claim_web_frame()
{
    if (shot_state == 3 && shot_web_owner && shot_buf)
    {
        shot_state = 4;
        return shot_buf;
    }
    return nullptr;
}

void shot_cancel_web_request()
{
    if (shot_web_owner && shot_state == 1)
    {
        shot_port = nullptr;
        shot_web_owner = false;
        shot_state = 0;
    }
}

void shot_release_web_frame()
{
    if (shot_state == 4 && shot_web_owner)
    {
        if (shot_buf) heap_caps_free(shot_buf);
        shot_buf = nullptr;
        shot_port = nullptr;
        shot_web_owner = false;
        shot_state = 0;
    }
}

void shot_dump()
{
    if (shot_state != 3 || !shot_buf) return;
    if (shot_web_owner) return;
    if (!shot_port) return;
    shot_init_worker();
    if (shot_worker_task_handle)
    {
        xTaskNotifyGive(shot_worker_task_handle);
    }
}

void shot_on_flush(const lv_area_t *area, const uint16_t *px, bool last)
{
    if (shot_state != 2 || !shot_buf || !area || !px) return;
    int x1 = area->x1 < 0 ? 0 : area->x1;
    int x2 = area->x2 >= SHOT_W ? SHOT_W - 1 : area->x2;
    int w_src = area->x2 - area->x1 + 1;
    if (x2 < x1 || w_src <= 0) return;

    for (int y = area->y1; y <= area->y2; y++)
    {
        if (y < 0 || y >= SHOT_H)
        {
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
        if (!(shot_row_seen[idx] & mask))
        {
            shot_row_seen[idx] |= mask;
            shot_rows_copied++;
        }
    }

    if (last || shot_rows_copied >= (SHOT_H - 10))
    {
        shot_state = 3;
        shot_dump();
    }
}

void shot_arm()
{
    if (shot_state != 1) return;
    shot_t0 = millis();
    shot_rows_copied = 0;
    memset(shot_row_seen, 0, sizeof(shot_row_seen));
    if (!shot_buf)
    {
        shot_buf = (uint16_t *)heap_caps_malloc(SHOT_W * SHOT_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (!shot_buf)
    {
        shot_buf = (uint16_t *)malloc(SHOT_W * SHOT_H * 2);
    }
    if (!shot_buf)
    {
        if (shot_port) shot_port->println("SHOT_ERR alloc");
        shot_port = nullptr;
        shot_state = 0;
        return;
    }
    shot_state = 2; // Ingest pixels on flush
    lv_obj_t *act_scr = lv_scr_act();
    if (act_scr && lv_obj_is_valid(act_scr))
    {
        lv_area_t full = {0, 0, SHOT_W - 1, SHOT_H - 1};
        lv_obj_invalidate_area(act_scr, &full);
        lv_obj_invalidate(act_scr);
    }
    lv_obj_t *top_l = lv_layer_top();
    if (top_l && lv_obj_is_valid(top_l)) lv_obj_invalidate(top_l);
    lv_obj_t *sys_l = lv_layer_sys();
    if (sys_l && lv_obj_is_valid(sys_l)) lv_obj_invalidate(sys_l);
}

void shot_poll()
{
    if (shot_state == 1)
    {
        shot_arm();
    }
    if (shot_state == 2)
    {
        if (shot_rows_copied >= (SHOT_H - 4))
        {
            shot_state = 3;
            shot_dump();
        }
        else if (millis() - shot_t0 > SHOT_TIMEOUT_MS)
        {
            if (shot_rows_copied > 0)
            {
                shot_state = 3;
                shot_dump();
            }
            else
            {
                if (shot_port) shot_port->printf("SHOT_ERR timeout rows=%u\n", (unsigned)shot_rows_copied);
                if (shot_buf)
                {
                    heap_caps_free(shot_buf);
                    shot_buf = nullptr;
                }
                shot_port = nullptr;
                shot_state = 0;
            }
        }
    }
}
