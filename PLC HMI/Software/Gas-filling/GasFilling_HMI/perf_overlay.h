#ifndef GASFILLING_PERF_OVERLAY_H
#define GASFILLING_PERF_OVERLAY_H

#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>

// Zero-overhead serial telemetry engine for GasFilling HMI
// Adheres strictly to Siemens Industrial Determinism standards:
// Scalar counters only, zero dynamic allocation, zero LVGL objects created in loop.

static uint32_t perf_stat_flush_frames = 0;
static uint32_t perf_stat_flush_pixels = 0;
static uint32_t perf_stat_busy_us = 0;

static uint32_t perf_t2_last_loop_us = 0;
static uint32_t perf_t2_max_loop_us = 0;
static uint32_t perf_t2_last_busy_us = 0;
static uint32_t perf_t2_max_busy_us = 0;
static uint32_t perf_t2_last_sample_us = 0;

static uint32_t perf_qa_until_ms = 0;
static uint32_t perf_qa_last_ms = 0;
static uint32_t perf_qa_last_frames = 0;
static uint32_t perf_qa_last_pixels = 0;
static uint32_t perf_qa_busy_window_us = 0;
static bool perf_qa_continuous = false;
static Stream *perf_qa_port = nullptr;

static const uint32_t PERF_SCREEN_PIXELS = 480UL * 320UL;
static const uint32_t PERF_QA_DURATION_MS = 20000UL;
static const uint32_t PERF_QA_SAMPLE_MS = 500UL;

static inline void perf_overlay_note_flush(bool last_flush, uint32_t px_count)
{
    perf_stat_flush_pixels += px_count;
    if (last_flush) {
        perf_stat_flush_frames++;
    }
}

static inline void perf_overlay_add_busy(uint32_t busy_us)
{
    const uint32_t sample_us = micros();
    if (perf_t2_last_sample_us != 0) {
        // Unsigned subtraction intentionally keeps interval valid across 32-bit micros() wrap
        const uint32_t loop_us = sample_us - perf_t2_last_sample_us;
        perf_t2_last_loop_us = loop_us;
        if (loop_us > perf_t2_max_loop_us) {
            perf_t2_max_loop_us = loop_us;
        }
    }
    perf_t2_last_sample_us = sample_us;
    perf_t2_last_busy_us = busy_us;
    if (busy_us > perf_t2_max_busy_us) {
        perf_t2_max_busy_us = busy_us;
    }
    perf_stat_busy_us += busy_us;
    if (perf_qa_until_ms != 0 || perf_qa_continuous) {
        perf_qa_busy_window_us += busy_us;
    }
}

static inline void perf_qa_stop(const char *why = nullptr)
{
    if (perf_qa_port) {
        perf_qa_port->printf("PERF_END %s\n", why ? why : "done");
    }
    perf_qa_until_ms = 0;
    perf_qa_continuous = false;
    perf_qa_port = nullptr;
    perf_qa_busy_window_us = 0;
}

static inline void perf_qa_start(Stream *port, bool continuous = false)
{
    perf_qa_port = port ? port : &Serial;
    perf_qa_continuous = continuous;
    uint32_t now = millis();

    perf_t2_last_loop_us = 0;
    perf_t2_max_loop_us = 0;
    perf_t2_last_busy_us = 0;
    perf_t2_max_busy_us = 0;
    perf_t2_last_sample_us = 0;

    perf_qa_until_ms = continuous ? 0xFFFFFFFFUL : (now + PERF_QA_DURATION_MS);
    perf_qa_last_ms = now;
    perf_qa_last_frames = perf_stat_flush_frames;
    perf_qa_last_pixels = perf_stat_flush_pixels;
    perf_qa_busy_window_us = 0;

    if (perf_qa_port) {
        perf_qa_port->printf("PERF_START mode=%s sample=%ums duration=%ums\n",
                             continuous ? "continuous" : "windowed",
                             (unsigned)PERF_QA_SAMPLE_MS,
                             continuous ? 0 : (unsigned)PERF_QA_DURATION_MS);
    }
}

static inline void perf_qa_tick()
{
    if (!perf_qa_port || perf_qa_until_ms == 0) return;

    uint32_t now = millis();
    if (!perf_qa_continuous && (int32_t)(now - perf_qa_until_ms) >= 0) {
        perf_qa_stop("timeout");
        return;
    }

    uint32_t elapsed_ms = now - perf_qa_last_ms;
    if (elapsed_ms < PERF_QA_SAMPLE_MS) return;

    uint32_t frame_delta = perf_stat_flush_frames - perf_qa_last_frames;
    uint32_t pixel_delta = perf_stat_flush_pixels - perf_qa_last_pixels;
    uint32_t fps = (frame_delta * 1000UL + (elapsed_ms / 2)) / elapsed_ms;
    if (fps > 60) fps = 60;

    uint32_t rf_pct = (pixel_delta * 100UL + (PERF_SCREEN_PIXELS / 2UL)) / PERF_SCREEN_PIXELS;

    uint32_t heap_total = ESP.getHeapSize();
    uint32_t heap_free = ESP.getFreeHeap();
    uint32_t ram_pct = heap_total ? (((heap_total - heap_free) * 100UL) / heap_total) : 0;
    if (ram_pct > 100) ram_pct = 100;

    uint32_t psram_total = ESP.getPsramSize();
    uint32_t psram_free = ESP.getFreePsram();
    uint32_t psr_pct = psram_total ? (((psram_total - psram_free) * 100UL) / psram_total) : 0;
    if (psr_pct > 100) psr_pct = 100;

    uint32_t cpu_pct = (perf_qa_busy_window_us + (elapsed_ms * 5UL)) / (elapsed_ms * 10UL);
    if (cpu_pct > 99) cpu_pct = 99;

    perf_qa_port->printf(
        "PERF FPS=%lu CPU=%lu%% RAM=%lu%% PSR=%lu%% RF=%lu%% loop=%luus loopMax=%luus busy=%luus busyMax=%luus\n",
        (unsigned long)fps, (unsigned long)cpu_pct, (unsigned long)ram_pct,
        (unsigned long)psr_pct, (unsigned long)rf_pct,
        (unsigned long)perf_t2_last_loop_us, (unsigned long)perf_t2_max_loop_us,
        (unsigned long)perf_t2_last_busy_us, (unsigned long)perf_t2_max_busy_us);

    lv_mem_monitor_t lv_mem;
    lv_mem_monitor(&lv_mem);
    const size_t lv_used = (lv_mem.total_size >= lv_mem.free_size) ?
                           (lv_mem.total_size - lv_mem.free_size) : 0;
    perf_qa_port->printf(
        "PERF LVGL total=%u used=%u free=%u max=%u frag=%u%%\n",
        (unsigned)lv_mem.total_size, (unsigned)lv_used, (unsigned)lv_mem.free_size,
        (unsigned)lv_mem.free_biggest_size, (unsigned)lv_mem.frag_pct);

    perf_qa_last_ms = now;
    perf_qa_last_frames = perf_stat_flush_frames;
    perf_qa_last_pixels = perf_stat_flush_pixels;
    perf_qa_busy_window_us = 0;
}

#endif // GASFILLING_PERF_OVERLAY_H
