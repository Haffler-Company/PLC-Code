#pragma once

#include <Arduino.h>
#include <bb_captouch.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#define TOUCH_ORIENTATION 270
#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_INT -1
#define TOUCH_RST -1
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);
BBCapTouch bbct;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

class HmiDisplayManager {
public:
  static HmiDisplayManager &instance() {
    static HmiDisplayManager manager;
    return manager;
  }

  bool begin() {
    if (initialized_) {
      return true;
    }

    display_ = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display_) {
      Serial.println("LVGL display create failed");
      return false;
    }

    allocateBuffers();
    if (!buf1 || !buf2) {
      Serial.println("LCD buffer alloc failed");
      return false;
    }

    tft.setSwapBytes(true);
    dma_enabled_ = tft.initDMA(false);

    lv_display_set_buffers(display_, buf1, buf2, buffer_px_count_ * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display_, flushCallback);
    lv_display_set_default(display_);

    input_ = lv_indev_create();
    lv_indev_set_type(input_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input_, touchReadCallback);

    initialized_ = true;
    return true;
  }

private:
  HmiDisplayManager() = default;

  void allocateBuffers() {
    static const uint16_t preferred_lines[] = {64, 56, 48, 40, 32, 24};

    for (uint16_t lines : preferred_lines) {
      const size_t candidate_px_count = SCREEN_WIDTH * lines;
      lv_color_t *candidate1 = static_cast<lv_color_t *>(heap_caps_malloc(candidate_px_count * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
      lv_color_t *candidate2 = static_cast<lv_color_t *>(heap_caps_malloc(candidate_px_count * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
      if (candidate1 && candidate2) {
        buf1 = candidate1;
        buf2 = candidate2;
        buffer_px_count_ = candidate_px_count;
        Serial.printf("LCD buffers: %u lines, DMA=%s\n", lines, dma_enabled_ ? "ready" : "pending");
        return;
      }
      if (candidate1) {
        heap_caps_free(candidate1);
      }
      if (candidate2) {
        heap_caps_free(candidate2);
      }
    }
  }

  void flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    const uint32_t width = area->x2 - area->x1 + 1;
    const uint32_t height = area->y2 - area->y1 + 1;
    uint16_t *buffer = reinterpret_cast<uint16_t *>(color_p);

    if (dma_enabled_ && (width * height) <= 0x4000) {
      tft.pushImageDMA(area->x1, area->y1, width, height, buffer);
      while (tft.dmaBusy()) {
        taskYIELD();
      }
    } else {
      tft.startWrite();
      tft.setAddrWindow(area->x1, area->y1, width, height);
      tft.pushColors(buffer, width * height, false);
      tft.endWrite();
    }

    lv_display_flush_ready(disp);
  }

  void readTouch(lv_indev_data_t *data) {
    TOUCHINFO touch_info;
    if (!bbct.getSamples(&touch_info)) {
      data->state = LV_INDEV_STATE_RELEASED;
      return;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = constrain(touch_info.x[0], 0, SCREEN_WIDTH - 1);
    data->point.y = constrain(touch_info.y[0], 0, SCREEN_HEIGHT - 1);
  }

  static void flushCallback(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    instance().flush(disp, area, color_p);
  }

  static void touchReadCallback(lv_indev_t *indev, lv_indev_data_t *data) {
    LV_UNUSED(indev);
    instance().readTouch(data);
  }

  lv_display_t *display_ = nullptr;
  lv_indev_t *input_ = nullptr;
  size_t buffer_px_count_ = 0;
  bool initialized_ = false;
  bool dma_enabled_ = false;
};
