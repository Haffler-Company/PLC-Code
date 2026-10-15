#pragma once

// شمارنده‌های سبک همیشه فعال‌اند تا فرمان سریال PERF (QA کوتاه) بدون rebuild کار کند.
// نوار LCD فقط با انتخاب پایدار اپراتور در F5 ساخته می‌شود؛ در حالت عادی هیچ شیء LVGL ندارد.

#ifndef PERF_OVERLAY_ENABLE
#define PERF_OVERLAY_ENABLE 1
#endif

static uint32_t perf_stat_flush_frames = 0;
static uint32_t perf_stat_flush_pixels = 0;
static uint32_t perf_stat_busy_us = 0;
// [UI/Task2] One call is made at the end of every Task2 pass.  Keep these
// counters scalar-only so the diagnostic path has no allocation or LVGL work.
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
static Stream *perf_qa_port = nullptr;

static const uint32_t PERF_SCREEN_PIXELS = 480UL * 320UL;
static const uint32_t PERF_QA_DURATION_MS = 20000UL;
static const uint32_t PERF_QA_SAMPLE_MS = 500UL;

static inline void perf_overlay_note_flush(bool last_flush, uint32_t px_count) {
  perf_stat_flush_pixels += px_count;
  if (last_flush) perf_stat_flush_frames++;
}

static inline void perf_overlay_add_busy(uint32_t busy_us) {
  const uint32_t sample_us = micros();
  if (perf_t2_last_sample_us != 0) {
    // Unsigned subtraction intentionally keeps the interval valid over wrap.
    const uint32_t loop_us = sample_us - perf_t2_last_sample_us;
    perf_t2_last_loop_us = loop_us;
    if (loop_us > perf_t2_max_loop_us) perf_t2_max_loop_us = loop_us;
  }
  perf_t2_last_sample_us = sample_us;
  perf_t2_last_busy_us = busy_us;
  if (busy_us > perf_t2_max_busy_us) perf_t2_max_busy_us = busy_us;
  perf_stat_busy_us += busy_us;
  if (perf_qa_until_ms) perf_qa_busy_window_us += busy_us;
}

// PERF روی سریال: ۲۰ ثانیه نمونه FPS/CPU/RAM — بدون نوار روی LCD تولید
static inline void perf_qa_start(Stream *port) {
  perf_qa_port = port ? port : &Serial;
  uint32_t now = millis();
  // Start a fresh bounded window: retained boot/navigation spikes must not be
  // mistaken for a lag observed during this operator-requested sample.
  perf_t2_last_loop_us = 0;
  perf_t2_max_loop_us = 0;
  perf_t2_last_busy_us = 0;
  perf_t2_max_busy_us = 0;
  perf_t2_last_sample_us = 0;
  perf_qa_until_ms = now + PERF_QA_DURATION_MS;
  perf_qa_last_ms = now;
  perf_qa_last_frames = perf_stat_flush_frames;
  perf_qa_last_pixels = perf_stat_flush_pixels;
  perf_qa_busy_window_us = 0;
  if (perf_qa_port) {
    perf_qa_port->println("PERF_START 20s sample=500ms (jog then watch lines)");
  }
}

static inline void perf_qa_stop(const char *why) {
  if (perf_qa_port) {
    perf_qa_port->printf("PERF_END %s\n", why ? why : "done");
  }
  perf_qa_until_ms = 0;
  perf_qa_port = nullptr;
  perf_qa_busy_window_us = 0;
}

static inline void perf_qa_tick() {
  if (!perf_qa_until_ms || !perf_qa_port) return;
  uint32_t now = millis();
  if ((int32_t)(now - perf_qa_until_ms) >= 0) {
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

  uint32_t cpu_pct = (perf_qa_busy_window_us + (elapsed_ms * 5UL)) / (elapsed_ms * 10UL);
  if (cpu_pct > 99) cpu_pct = 99;

  const bool moving = STP.motorIs_moving || STP.jogEnabled;
  perf_qa_port->printf(
      "PERF FPS=%lu CPU=%lu%% RAM=%lu%% RF=%lu%% move=%d scr=%d T2last=%luus T2max=%luus T2busy=%luus T2busyMax=%luus\n",
      (unsigned long)fps, (unsigned long)cpu_pct, (unsigned long)ram_pct,
      (unsigned long)rf_pct, moving ? 1 : 0, (int)GUI.scr_id,
      (unsigned long)perf_t2_last_loop_us, (unsigned long)perf_t2_max_loop_us,
      (unsigned long)perf_t2_last_busy_us, (unsigned long)perf_t2_max_busy_us);

  lv_mem_monitor_t lv_mem;
  lv_mem_monitor(&lv_mem);
  const size_t lv_used = (lv_mem.total_size >= lv_mem.free_size) ?
                         (lv_mem.total_size - lv_mem.free_size) : 0;
  perf_qa_port->printf(
      "PERF LVGL total=%u used=%u free=%u largest=%u frag=%u%%\n",
      (unsigned)lv_mem.total_size, (unsigned)lv_used, (unsigned)lv_mem.free_size,
      (unsigned)lv_mem.free_biggest_size, (unsigned)lv_mem.frag_pct);

  perf_qa_last_ms = now;
  perf_qa_last_frames = perf_stat_flush_frames;
  perf_qa_last_pixels = perf_stat_flush_pixels;
  perf_qa_busy_window_us = 0;
}

#if PERF_OVERLAY_ENABLE

static lv_obj_t *perf_overlay_cont = NULL;
static lv_obj_t *perf_overlay_lbl = NULL;
static uint32_t perf_overlay_last_ms = 0;
static uint32_t perf_overlay_last_frames = 0;
static uint32_t perf_overlay_last_pixels = 0;
static uint32_t perf_overlay_last_busy_us = 0;
static char perf_overlay_text[76] = "CPU --%  RAM --%  PSR --%  FPS --";

static void perf_overlay_build() {
  if (perf_overlay_cont) return;
  perf_overlay_cont = lv_obj_create(lv_layer_top());
  // نوار اطلاعات سخت افزار: فقط محور X، 32 درصد کوچک‌تر از 456px قبلی.
  lv_obj_set_size(perf_overlay_cont, 310, 22);
  lv_obj_align(perf_overlay_cont, LV_ALIGN_TOP_MID, 0, 3);
  lv_obj_clear_flag(perf_overlay_cont, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_color(perf_overlay_cont, lv_color_hex(0x101820), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(perf_overlay_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(perf_overlay_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(perf_overlay_cont, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(perf_overlay_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  perf_overlay_lbl = lv_label_create(perf_overlay_cont);
  lv_obj_set_style_text_font(perf_overlay_lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(perf_overlay_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(perf_overlay_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_width(perf_overlay_lbl, 298);
  lv_obj_center(perf_overlay_lbl);
  lv_label_set_text_static(perf_overlay_lbl, perf_overlay_text);
  lv_obj_move_foreground(perf_overlay_cont);
}

static void perf_overlay_tick() {
  perf_qa_tick();
  if (device_locked) {
    if (perf_overlay_cont && lv_obj_is_valid(perf_overlay_cont)) {
      lv_obj_add_flag(perf_overlay_cont, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
  if (!diagnostics_overlay_enabled) {
    if (perf_overlay_cont && lv_obj_is_valid(perf_overlay_cont)) lv_obj_delete(perf_overlay_cont);
    perf_overlay_cont = NULL;
    perf_overlay_lbl = NULL;
    perf_overlay_last_ms = 0;
    perf_overlay_last_busy_us = 0;
    return;
  }
  perf_overlay_build();
  if (!perf_overlay_cont || !perf_overlay_lbl) return;
  if (lv_obj_has_flag(perf_overlay_cont, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_clear_flag(perf_overlay_cont, LV_OBJ_FLAG_HIDDEN);
  }
  uint32_t now = millis();
  if (perf_overlay_last_ms == 0) {
    perf_overlay_last_ms = now;
    perf_overlay_last_frames = perf_stat_flush_frames;
    perf_overlay_last_pixels = perf_stat_flush_pixels;
    perf_overlay_last_busy_us = perf_stat_busy_us;
    return;
  }
  uint32_t elapsed_ms = now - perf_overlay_last_ms;
  const uint32_t sample_ms = (GUI.scr_id == 1 && STP.motorIs_moving) ? 1000UL : 500UL;
  if (elapsed_ms < sample_ms) return;

  uint32_t frame_delta = perf_stat_flush_frames - perf_overlay_last_frames;
  uint32_t fps = (frame_delta * 1000UL + (elapsed_ms / 2)) / elapsed_ms;
  if (fps > 60) fps = 60;

  uint32_t heap_free = ESP.getFreeHeap();
  uint32_t heap_total = ESP.getHeapSize();
  uint32_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  uint32_t psram_total = ESP.getPsramSize();

  uint32_t busy = perf_stat_busy_us;
  // unsigned subtraction intentionally preserves the elapsed interval across a 32-bit wrap.
  uint32_t busy_delta = busy - perf_overlay_last_busy_us;
  uint32_t cpu_pct = (busy_delta + (elapsed_ms * 5UL)) / (elapsed_ms * 10UL);
  if (cpu_pct > 99) cpu_pct = 99;
  const uint32_t ram_pct = heap_total ? min(100UL, ((heap_total - heap_free) * 100UL) / heap_total) : 0;
  const uint32_t psram_pct = psram_total ? min(100UL, ((psram_total - psram_free) * 100UL) / psram_total) : 0;

  char b[76];
  snprintf(b, sizeof(b), "CPU %lu%%  RAM %lu%%  PSR %lu%%  FPS %lu",
           (unsigned long)cpu_pct, (unsigned long)ram_pct,
           (unsigned long)psram_pct, (unsigned long)fps);
  // نوار باید روی لایهٔ بالا بماند، اما جلو آوردن آن در هر دور Task2 باعث
  // کار اضافهٔ LVGL می‌شد. همگام با نرخ نمونهٔ نمایش، کافی و بدون تغییر ظاهری است.
  lv_obj_move_foreground(perf_overlay_cont);
  if (strcmp(perf_overlay_text, b) != 0) {
    strncpy(perf_overlay_text, b, sizeof(perf_overlay_text) - 1);
    perf_overlay_text[sizeof(perf_overlay_text) - 1] = '\0';
    lv_label_set_text_static(perf_overlay_lbl, perf_overlay_text);
  }

  perf_overlay_last_ms = now;
  perf_overlay_last_frames = perf_stat_flush_frames;
  perf_overlay_last_pixels = perf_stat_flush_pixels;
  perf_overlay_last_busy_us = busy;
}

#else

static inline void perf_overlay_tick() {
  perf_qa_tick();
}

#endif
