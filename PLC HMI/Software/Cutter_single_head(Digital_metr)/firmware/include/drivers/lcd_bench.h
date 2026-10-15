#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#define LCD_BENCH_W 480
#define LCD_BENCH_H 320
#define LCD_BENCH_ENABLE_DMA 1

extern TFT_eSPI tft;

static volatile bool lcd_bench_pending = false;
static Stream *lcd_bench_port = nullptr;

static inline uint16_t lcd_bench_swap565(uint16_t c) {
  return (uint16_t)((c << 8) | (c >> 8));
}

static inline void lcd_bench_request(Stream *port) {
  lcd_bench_port = port;
  lcd_bench_pending = true;
}

static uint32_t lcd_bench_full_push(uint16_t *buf, int lines, int frames, bool lib_swap) {
  const uint32_t chunk_px = (uint32_t)LCD_BENCH_W * (uint32_t)lines;
  uint32_t t0 = millis();
  for (int f = 0; f < frames; f++) {
    uint16_t color = (uint16_t)(0x001f + ((f * 0x0411) & 0xffff));
    uint16_t out = lib_swap ? color : lcd_bench_swap565(color);
    for (uint32_t i = 0; i < chunk_px; i++) buf[i] = out;
    tft.startWrite();
    tft.setAddrWindow(0, 0, LCD_BENCH_W, LCD_BENCH_H);
    int y = 0;
    while (y < LCD_BENCH_H) {
      int h = LCD_BENCH_H - y;
      if (h > lines) h = lines;
      tft.pushColors(buf, (uint32_t)LCD_BENCH_W * (uint32_t)h, lib_swap);
      y += h;
    }
    tft.endWrite();
    vTaskDelay(1);
  }
  return millis() - t0;
}

static uint32_t lcd_bench_partial_push(uint16_t *buf, int w, int h, int frames) {
  const uint32_t px = (uint32_t)w * (uint32_t)h;
  uint32_t t0 = millis();
  for (int f = 0; f < frames; f++) {
    uint16_t color = lcd_bench_swap565((uint16_t)(0xf800 - ((f * 0x0180) & 0xffff)));
    for (uint32_t i = 0; i < px; i++) buf[i] = color;
    int x = (f * 17) % (LCD_BENCH_W - w);
    int y = 28 + ((f * 11) % (LCD_BENCH_H - h - 28));
    tft.startWrite();
    tft.setAddrWindow(x, y, w, h);
    tft.pushColors(buf, px, false);
    tft.endWrite();
    vTaskDelay(1);
  }
  return millis() - t0;
}

static uint32_t lcd_bench_dma_push(uint16_t *buf, int lines, int frames) {
  const uint32_t chunk_px = (uint32_t)LCD_BENCH_W * (uint32_t)lines;
  uint32_t t0 = millis();
  for (int f = 0; f < frames; f++) {
    uint16_t color = lcd_bench_swap565((uint16_t)(0x07e0 + ((f * 0x0821) & 0xffff)));
    for (uint32_t i = 0; i < chunk_px; i++) buf[i] = color;
    int y = 0;
    tft.startWrite();
    while (y < LCD_BENCH_H) {
      int h = LCD_BENCH_H - y;
      if (h > lines) h = lines;
      tft.pushImageDMA(0, y, LCD_BENCH_W, h, (const uint16_t *)buf);
      uint32_t dma_wstart = micros();
      while (tft.dmaBusy() && (uint32_t)(micros() - dma_wstart) < 25000U) taskYIELD();
      y += h;
    }
    tft.endWrite();
    vTaskDelay(1);
  }
  return millis() - t0;
}

static void lcd_bench_run_if_pending(bool allow_dma) {
  if (!lcd_bench_pending) return;
  lcd_bench_pending = false;
  Stream *port = lcd_bench_port ? lcd_bench_port : &Serial;

  // Do not mix a benchmark transaction with an outstanding LVGL DMA flush.
  if (tft.DMA_Enabled) tft.dmaWait();

  int lines = 40;
  uint16_t *buf = nullptr;
  while (lines >= 8) {
    buf = (uint16_t *)heap_caps_malloc((size_t)LCD_BENCH_W * (size_t)lines * sizeof(uint16_t),
                                       MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf) break;
    lines /= 2;
  }
  if (!buf) {
    port->println("LCDB_ERR alloc");
    return;
  }

  port->printf("LCDB_BEGIN heap=%u largest=%u lines=%d\n",
               (unsigned)ESP.getFreeHeap(),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
               lines);

  const int full_frames = 12;
  uint32_t ms_fast = lcd_bench_full_push(buf, lines, full_frames, false);
  uint32_t px_full = (uint32_t)LCD_BENCH_W * (uint32_t)LCD_BENCH_H * (uint32_t)full_frames;
  port->printf("LCDB full_push_no_swap frames=%d ms=%lu fps_x100=%lu kpix_s=%lu\n",
               full_frames, (unsigned long)ms_fast,
               (unsigned long)((full_frames * 100000UL) / (ms_fast ? ms_fast : 1)),
               (unsigned long)(px_full / (ms_fast ? ms_fast : 1)));

  const int swap_frames = 4;
  uint32_t ms_swap = lcd_bench_full_push(buf, lines, swap_frames, true);
  uint32_t px_swap = (uint32_t)LCD_BENCH_W * (uint32_t)LCD_BENCH_H * (uint32_t)swap_frames;
  port->printf("LCDB full_push_lib_swap frames=%d ms=%lu fps_x100=%lu kpix_s=%lu\n",
               swap_frames, (unsigned long)ms_swap,
               (unsigned long)((swap_frames * 100000UL) / (ms_swap ? ms_swap : 1)),
               (unsigned long)(px_swap / (ms_swap ? ms_swap : 1)));

  const int part_frames = 120;
  uint32_t ms_part = lcd_bench_partial_push(buf, 120, 48, part_frames);
  uint32_t px_part = 120UL * 48UL * (uint32_t)part_frames;
  port->printf("LCDB partial_120x48 frames=%d ms=%lu fps_x100=%lu kpix_s=%lu\n",
               part_frames, (unsigned long)ms_part,
               (unsigned long)((part_frames * 100000UL) / (ms_part ? ms_part : 1)),
               (unsigned long)(px_part / (ms_part ? ms_part : 1)));

#if LCD_BENCH_ENABLE_DMA
  // Runtime display initialization is the only owner of DMA setup. LCDB must
  // never re-enable DMA after initialization failure or health degradation.
  bool dma_ok = allow_dma && tft.DMA_Enabled;
  if (dma_ok) {
    const int dma_frames = 12;
    const int dma_lines = (lines > 32) ? 32 : lines;
    uint32_t ms_dma = lcd_bench_dma_push(buf, dma_lines, dma_frames);
    uint32_t px_dma = (uint32_t)LCD_BENCH_W * (uint32_t)LCD_BENCH_H * (uint32_t)dma_frames;
    port->printf("LCDB dma_push frames=%d lines=%d ms=%lu fps_x100=%lu kpix_s=%lu\n",
                 dma_frames, dma_lines, (unsigned long)ms_dma,
                 (unsigned long)((dma_frames * 100000UL) / (ms_dma ? ms_dma : 1)),
                 (unsigned long)(px_dma / (ms_dma ? ms_dma : 1)));
  } else {
    port->println(allow_dma ? "LCDB dma_unavailable" : "LCDB dma_runtime_disabled");
  }
#else
  port->println("LCDB dma_disabled");
#endif

  if (tft.DMA_Enabled) tft.dmaWait();
  heap_caps_free(buf);
  lv_obj_invalidate(lv_screen_active());
  lv_obj_invalidate(lv_layer_top());
  port->println("LCDB_END");
}
