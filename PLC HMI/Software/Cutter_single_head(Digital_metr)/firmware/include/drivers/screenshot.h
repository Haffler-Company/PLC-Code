// Serial framebuffer screenshot — Async Zero-Lag Non-Blocking Port
// ─────────────────────────────────────────────────────────────────────────────
// "SHOT\n" on Serial (USB-CDC) or Serial0 (UART0/CP210x, 921600) →
// the whole screen is invalidated, flushed regions accumulated into PSRAM,
// then streamed via background task on Core 0 without blocking the UI thread.
#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>

#define SHOT_W 480
#define SHOT_H 320
#define SHOT_RX_BUDGET_BYTES 16U
#define SHOT_TIMEOUT_MS 4000UL

volatile int shot_uf_dbg = 0;
static uint16_t *shot_buf = nullptr;
static volatile uint8_t shot_state = 0; // 0 idle, 1 requested, 2 copying, 3 ready, 4 claimed
static volatile bool shot_web_owner = false;
static volatile bool shot_uart0_owned = false;
static Stream *shot_port = nullptr;
static uint32_t shot_t0 = 0;
static uint16_t shot_rows_copied = 0;
static uint8_t shot_row_seen[(SHOT_H + 7) / 8];
static TaskHandle_t shot_worker_task_handle = nullptr;

static void shot_worker_task(void *pvParameters) {
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
      vTaskDelay(pdMS_TO_TICKS(1)); // ۱ میلی‌ثانیه تخلیه بافر برای هر سطر
    }
    p->println("SHOT_END");
    p->flush();

    heap_caps_free(shot_buf);
    shot_buf = nullptr;
    shot_port = nullptr;
    shot_state = 0;
  }
}

static inline void shot_init_worker() {
  if (!shot_worker_task_handle) {
    xTaskCreatePinnedToCore(shot_worker_task, "ShotWorker", 4096, NULL, 1, &shot_worker_task_handle, 0);
  }
}

static inline void shot_request(Stream *port) {
  if (shot_state != 0) return;
  shot_port = port;
  shot_web_owner = false;
  shot_state = 1;
}

static inline bool shot_request_web() {
  if (shot_state != 0) return false;
  shot_port = nullptr;
  shot_web_owner = true;
  shot_state = 1;
  return true;
}

static inline const uint16_t *shot_claim_web_frame() {
  if (shot_state == 3 && shot_web_owner && shot_buf) {
    shot_state = 4;
    return shot_buf;
  }
  return nullptr;
}

static inline void shot_cancel_web_request() {
  if (shot_web_owner && shot_state == 1) {
    shot_port = nullptr;
    shot_web_owner = false;
    shot_state = 0;
  }
}

static inline void shot_release_web_frame() {
  if (shot_state == 4 && shot_web_owner) {
    if (shot_buf) heap_caps_free(shot_buf);
    shot_buf = nullptr;
    shot_port = nullptr;
    shot_web_owner = false;
    shot_state = 0;
  }
}

static inline void shot_on_flush(const lv_area_t *area, const uint16_t *px, bool last = false) {
  if (shot_state != 2 || !shot_buf) return;
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
}

static inline void shot_arm() {
  if (shot_state != 1) return;
  shot_t0 = millis();
  shot_rows_copied = 0;
  memset(shot_row_seen, 0, sizeof(shot_row_seen));
  if (!shot_buf) shot_buf = (uint16_t *)heap_caps_malloc(SHOT_W * SHOT_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!shot_buf) shot_buf = (uint16_t *)malloc(SHOT_W * SHOT_H * 2);
  if (!shot_buf) {
    if (shot_port) shot_port->println("SHOT_ERR alloc");
    shot_port = nullptr;
    shot_state = 0;
    return;
  }
  shot_state = 2;
  lv_obj_invalidate(lv_screen_active());
  lv_obj_invalidate(lv_layer_top());
  lv_obj_invalidate(lv_layer_sys());
}

static inline void shot_dump() {
  if (shot_state == 2) {
    if (shot_rows_copied >= SHOT_H) {
      shot_state = 3;
    } else if (millis() - shot_t0 > SHOT_TIMEOUT_MS) {
      if (shot_port) shot_port->println("SHOT_ERR timeout");
      if (shot_buf) {
        heap_caps_free(shot_buf);
        shot_buf = nullptr;
      }
      shot_port = nullptr;
      shot_state = 0;
      return;
    }
  }
  if (shot_state != 3 || !shot_buf) return;
  if (shot_web_owner) return; // Handled asynchronously by WebPortalTask via shot_claim_web_frame()
  if (!shot_port) return;
  shot_init_worker();
  if (shot_worker_task_handle) {
    xTaskNotifyGive(shot_worker_task_handle);
  }
}


volatile uint8_t g_remote_ui_cmd = 0;

static inline void shot_handle_cmd_str(Stream &port, const char *cmd) {
  if (strstr(cmd, "SHOT")) {
    shot_request(&port);
  } else if (strstr(cmd, "S_WAR")) {
    g_remote_ui_cmd = 1;
    port.println("ACK_WAR");
  } else if (strstr(cmd, "S_VOL")) {
    g_remote_ui_cmd = 2;
    port.println("ACK_VOL");
  } else if (strstr(cmd, "S_OTA")) {
    g_remote_ui_cmd = 3;
    port.println("ACK_OTA");
  } else if (strstr(cmd, "S_CLS")) {
    g_remote_ui_cmd = 4;
    port.println("ACK_CLS");
  } else if (strstr(cmd, "S_UPD")) {
    g_remote_ui_cmd = 5;
    port.println("ACK_UPD");
  } else if (strstr(cmd, "S_F1")) {
    g_remote_ui_cmd = 6;
    port.println("ACK_F1");
  } else if (strstr(cmd, "S_F2") || strstr(cmd, "S_SCR1")) {
    g_remote_ui_cmd = 7;
    port.println("ACK_F2");
  } else if (strstr(cmd, "S_OOBE_P1") || strstr(cmd, "S_OOBE_1")) {
    g_remote_ui_cmd = 8;
    port.println("ACK_OOBE_P1");
  } else if (strstr(cmd, "S_OOBE_P2") || strstr(cmd, "S_OOBE_2")) {
    g_remote_ui_cmd = 9;
    port.println("ACK_OOBE_P2");
  } else if (strstr(cmd, "S_OOBE_P3") || strstr(cmd, "S_OOBE_3")) {
    g_remote_ui_cmd = 10;
    port.println("ACK_OOBE_P3");
  } else if (strstr(cmd, "S_OOBE_RST")) {
    g_remote_ui_cmd = 11;
    port.println("ACK_OOBE_RST");
  } else if (strstr(cmd, "S_SPL0")) {
    g_remote_ui_cmd = 12;
    port.println("ACK_SPL0");
  } else if (strstr(cmd, "S_SPL1")) {
    g_remote_ui_cmd = 13;
    port.println("ACK_SPL1");
  } else if (strstr(cmd, "S_SCR0")) {
    g_remote_ui_cmd = 14;
    port.println("ACK_SCR0");
  } else if (strstr(cmd, "S_SCR2")) {
    g_remote_ui_cmd = 15;
    port.println("ACK_SCR2");
  } else if (strstr(cmd, "S_SCR3")) {
    g_remote_ui_cmd = 16;
    port.println("ACK_SCR3");
  }
}

struct shot_rx_state_t { char cmd[32]; uint8_t cl; };
static shot_rx_state_t shot_rx_usb, shot_rx_uart0;

static inline void shot_poll_port(Stream &port, shot_rx_state_t &st) {
  for (uint16_t rx = 0; rx < SHOT_RX_BUDGET_BYTES && port.available(); ++rx) {
    char c = (char)port.read();
    if (c == '\n' || c == '\r') {
      st.cmd[st.cl] = 0;
      if (st.cl > 0) {
        shot_handle_cmd_str(port, st.cmd);
      }
      st.cl = 0;
    } else if (st.cl < sizeof(st.cmd) - 1) {
      st.cmd[st.cl++] = c;
    }
  }
}

static inline void shot_poll() {
  shot_poll_port(Serial, shot_rx_usb);
  shot_poll_port(Serial0, shot_rx_uart0);
  if (shot_state == 1) shot_arm();
  if (shot_state == 2) {
    if (shot_rows_copied >= SHOT_H) {
      shot_state = 3;
      shot_dump();
    } else if (millis() - shot_t0 > SHOT_TIMEOUT_MS) {
      if (shot_port) shot_port->println("SHOT_ERR timeout");
      if (shot_buf) {
        heap_caps_free(shot_buf);
        shot_buf = nullptr;
      }
      shot_port = nullptr;
      shot_state = 0;
    }
  }
}

extern TaskHandle_t h_Task1, h_Task2, h_Task3, h_Task4, h_Task5, h_Task6, h_Task7, h_Task8, h_Task9, h_Task10, h_Task11;

static inline void shot_report_stack_watermarks(Stream *port) {
  struct task_diag_t {
    const char *name;
    TaskHandle_t handle;
  } tasks[] = {
      {"Task1", h_Task1}, {"Task2", h_Task2}, {"Task3", h_Task3},
      {"Task4", h_Task4}, {"Task5", h_Task5}, {"Task6", h_Task6},
      {"Task7", h_Task7}, {"Task8", h_Task8}, {"Task9", h_Task9},
      {"Task10", h_Task10}, {"Task11", h_Task11},
  };
  if (!port) return;
  port->printf("MEM heap=%u internal=%u largest=%u\n", (unsigned)ESP.getFreeHeap(),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  for (const task_diag_t &task : tasks) {
    if (!task.handle) continue;
    const UBaseType_t free_bytes = uxTaskGetStackHighWaterMark(task.handle);
    port->printf("MEM %s free=%uB\n", task.name, (unsigned)free_bytes);
  }
}

#define SHOT_UART0_RX_BUDGET_BYTES 64U

static inline void shot_poll_uart0() {
  shot_poll_port(Serial, shot_rx_usb);
  if (!shot_uart0_owned) {
    shot_poll_port(Serial0, shot_rx_uart0);
  }
}

