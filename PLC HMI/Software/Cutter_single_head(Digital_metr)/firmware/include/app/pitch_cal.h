#pragma once
// F4 pitch calibration. Task2 owns LVGL; Task3 owns timer/driver motion.
// Auto: I0 seek/release, then I1 seek/release or exactly 15*drv_ppr pulses.

enum {
  PC_IDLE = 0, PC_CONFIRM, PC_END_SENSOR, PC_HOME, PC_SAMPLE,
  PC_END_RELEASE, PC_ENTER_MM, PC_ENTER_SCALE, PC_MANUAL_CONFIRM,
  PC_RESULT, PC_ERROR
};

enum {
  PCERR_NONE = 0, PCERR_ABORT, PCERR_TIMEOUT, PCERR_WRONG_DIR,
  PCERR_BAD_MEASURE, PCERR_BUSY, PCERR_FIRST_SENSOR, PCERR_END_SENSOR,
  PCERR_SENSOR_STUCK, PCERR_UNEXPECTED_END
};

static volatile int pitch_cal_stage = PC_IDLE;
static volatile int pitch_cal_err = PCERR_NONE;
static volatile int32_t pitch_cal_pulses = 0;
static volatile bool pitch_cal_motion_busy = false;
static volatile bool pitch_cal_has_end_sensor = false;
static volatile bool pitch_cal_cancel_requested = false;
static volatile int32_t pitch_cal_end_release_pulses = 0;
static float pitch_cal_um_pp = 0.0f;
static float pitch_cal_mm_pp = 0.0f;
static float pitch_cal_pitch = 0.0f;
static float pitch_cal_measured = 0.0f;
static float pitch_cal_first_mm = 0.0f;
static float pitch_cal_travel = 0.0f;
static String pitch_cal_buf = "";

static lv_obj_t *pc_root = NULL;
static lv_obj_t *pc_title = NULL;
static lv_obj_t *pc_body = NULL;
static lv_obj_t *pc_ta = NULL;
static lv_obj_t *pc_kb = NULL;
static lv_obj_t *pc_btn_ok = NULL;
static lv_obj_t *pc_btn_alt = NULL;
static lv_obj_t *pc_btn_x = NULL;      // ✕ بستن (جایگزین انصراف، بالا-راست کارت)
static lv_obj_t *pc_unit = NULL;       // واحد داخل کادر عدد (راست)
static lv_obj_t *pc_status = NULL;

void save_axis_pitch();
void save_um_ppulse();
void save_init_pos();
extern SemaphoreHandle_t lable1_1_Semaphore;
void save_final_pos();
void save_last_pos_p();
void ui_toast(const char *fa, const char *en, uint32_t ms);
bool usb_keyboard_present();  // Task4 — کیبورد فیزیکی وصل است؟

static inline bool pitch_cal_busy() {
  return pitch_cal_motion_busy || pitch_cal_stage == PC_HOME ||
         pitch_cal_stage == PC_SAMPLE || pitch_cal_stage == PC_END_RELEASE;
}

static inline float pitch_cal_first_line_mm() {
  float ppmm = axisPulsesPerMm();
  if (ppmm < 1.0f) ppmm = 1.0f;
  return (float)STP.initial_Position_p / ppmm;
}

// [Task3] Publish only after timer/driver are stopped. This prevents a UI cancel
// from changing the sampling ISR into a different motion mode mid-cycle.
static inline void pitch_cal_task3_fail(int err) {
  pitch_cal_motion_busy = false;
  STP.motorIs_moving = false;
  STP.mvm_interupt = false;
  pitch_cal_err = pitch_cal_cancel_requested ? PCERR_ABORT : err;
  pitch_cal_stage = PC_ERROR;
  GUI.buzzer_cmd_long = true;
}

// [UI] Request; Task3 observes this flag and performs the physical stop.
static inline void pitch_cal_abort(int err) {
  pitch_cal_err = err;
  pitch_cal_cancel_requested = true;
  STP.mvm_interupt = true;
  if (!pitch_cal_motion_busy) {
    pitch_cal_stage = PC_ERROR;
    GUI.buzzer_cmd_long = true;
  }
}

static void pitch_cal_reset_state() {
  pitch_cal_stage = PC_IDLE;
  pitch_cal_err = PCERR_NONE;
  pitch_cal_pulses = 0;
  pitch_cal_motion_busy = false;
  pitch_cal_has_end_sensor = false;
  pitch_cal_cancel_requested = false;
  pitch_cal_end_release_pulses = 0;
  pitch_cal_buf = "";
  GUI.lock_screen = false;
  if (pc_kb) lv_obj_add_flag(pc_kb, LV_OBJ_FLAG_HIDDEN);
  if (pc_root) lv_obj_add_flag(pc_root, LV_OBJ_FLAG_HIDDEN);
}

void pitch_cal_do_home();
void pitch_cal_do_sample();
void pitch_cal_do_end_release();
static inline void pitch_cal_task3_poll() {
  if (pitch_cal_stage == PC_HOME && !pitch_cal_motion_busy) pitch_cal_do_home();
  else if (pitch_cal_stage == PC_SAMPLE && !pitch_cal_motion_busy) pitch_cal_do_sample();
  else if (pitch_cal_stage == PC_END_RELEASE && !pitch_cal_motion_busy) pitch_cal_do_end_release();
}

static bool pitch_cal_compute_from_measure(float measured_mm) {
  pitch_cal_first_mm = pitch_cal_first_line_mm();
  pitch_cal_measured = measured_mm;
  pitch_cal_travel = measured_mm - pitch_cal_first_mm;
  if (pitch_cal_pulses < (STP.drv_ppr / 2) || pitch_cal_travel < 5.0f) {
    pitch_cal_err = PCERR_BAD_MEASURE;
    return false;
  }
  pitch_cal_mm_pp = pitch_cal_travel / (float)pitch_cal_pulses;
  pitch_cal_um_pp = pitch_cal_mm_pp * 1000.0f;
  pitch_cal_pitch = pitch_cal_travel * (float)STP.drv_ppr / (float)pitch_cal_pulses;
  if (!isfinite(pitch_cal_um_pp) || pitch_cal_um_pp <= 0.0001f ||
      !isfinite(pitch_cal_pitch) || pitch_cal_pitch < 0.1f || pitch_cal_pitch > 200.0f) {
    pitch_cal_err = PCERR_BAD_MEASURE;
    return false;
  }
  return true;
}

static bool pitch_cal_compute_manual_scale(float um_per_pulse) {
  const float pitch = um_per_pulse * (float)STP.drv_ppr / 1000.0f;
  if (!isfinite(um_per_pulse) || um_per_pulse <= 0.0001f ||
      !isfinite(pitch) || pitch < 0.1f || pitch > 200.0f) {
    pitch_cal_err = PCERR_BAD_MEASURE;
    return false;
  }
  pitch_cal_um_pp = um_per_pulse;
  pitch_cal_mm_pp = um_per_pulse / 1000.0f;
  pitch_cal_pitch = pitch;
  return true;
}

static bool pitch_cal_parse_decimal(float *out) {
  if (!out || !pitch_cal_buf.length()) return false;
  const char *s = pitch_cal_buf.c_str();
  bool dot = false, digit = false;
  float value = 0.0f, place = 0.1f;
  for (size_t i = 0; s[i]; ++i) {
    if (s[i] == '.') {
      if (dot) return false;
      dot = true;
    } else if (s[i] >= '0' && s[i] <= '9') {
      digit = true;
      if (!dot) value = value * 10.0f + (float)(s[i] - '0');
      else { value += (float)(s[i] - '0') * place; place *= 0.1f; }
    } else return false;
  }
  if (!digit || !isfinite(value)) return false;
  *out = value;
  return true;
}

static inline bool pitch_cal_stored_scale_valid() {
  return isfinite(STP.um_per_pulse) && STP.um_per_pulse > 0.0001f &&
         isfinite(STP.Axis_Pitch) && STP.Axis_Pitch >= 0.1f && STP.Axis_Pitch <= 200.0f;
}

// با ذخیرهٔ موفق کالیبره تمام شد؛ قبل از pitch_cal_commit_save تعریف می‌شود
static bool pc_saved_ok = false;

static void pitch_cal_commit_save() {
  pc_saved_ok = true;  // اجرا با ذخیرهٔ موفق تمام شد؛ پیام «کالیبره نیست» نشان داده نشود
  float old_ppmm = axisPulsesPerMm();
  if (old_ppmm < 1.0f) old_ppmm = 1.0f;
  const float current_mm = (float)STP.currentPosition_p / old_ppmm;
  const float first_mm = (float)STP.initial_Position_p / old_ppmm;
  const float final_mm = STP.final_position_mm.toFloat();
  STP.Axis_Pitch = pitch_cal_pitch;
  STP.um_per_pulse = pitch_cal_um_pp;
  STP.initial_Position_p = (int)lroundf(first_mm * 1000.0f / pitch_cal_um_pp);
  STP.currentPosition_p = (int)lroundf(current_mm * 1000.0f / pitch_cal_um_pp);
  STP.final_position_p = (int)lroundf(final_mm * 1000.0f / pitch_cal_um_pp);
  STP.referenced = false;  // scale changed: reference before any positioning.
  save_axis_pitch(); save_um_ppulse(); save_init_pos(); save_final_pos(); save_last_pos_p();
  if (lable1_1_Semaphore && xSemaphoreTake(lable1_1_Semaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
    GUI.inputBuffer_sp = String((int)lroundf((float)STP.currentPosition_p / axisPulsesPerMm()));
    xSemaphoreGive(lable1_1_Semaphore);
  }
  GUI.flag_update_lable1_2 = true;
  GUI.update_lable1_1 = true;
  settings_mark_dirty();
}

// -------------------- Task2 UI --------------------
static int pc_last_stage = PC_IDLE;
static bool pc_stage_fresh = false;
static String pc_ta_synced = String('\x01');  // سنکِ buf→کادر فقط وقتی buf واقعاً عوض شد
// فونت‌های naz رقمِ فارسیِ U+06F0..9 و علائمِ جداشدۀ «؟/؛/اعراب/همزۀ ترکیبی» را ندارند
// (شاهد: update_flow.h UF_FA_DIG) → رقم به U+0660..9، «؟»→'?'، «؛»→';'، U+064B..U+0655 حذف.
static void pc_san(const char *in, char *out, size_t out_sz) {
  size_t o = 0;
  for (size_t i = 0; in[i] && o + 3 < out_sz; ) {
    const uint8_t a = (uint8_t)in[i], b = (uint8_t)in[i + 1];
    if (a == 0xDB && b >= 0xB0 && b <= 0xB9) {          // ۰..۹ → ٠..٩
      out[o++] = (char)0xD9; out[o++] = (char)(0xA0 + (b - 0xB0)); i += 2;
    } else if (a == 0xD9 && b >= 0x8B && b <= 0x95) {   // اعراب/همزۀ جدا: حذف
      i += 2;
    } else if (a == 0xD8 && b == 0x9F) { out[o++] = '?'; i += 2; }
    else if (a == 0xD8 && b == 0x9B) { out[o++] = ';'; i += 2; }
    else out[o++] = in[i++];
  }
  out[o] = 0;
}
static void pc_hide_kb() { if (pc_kb) lv_obj_add_flag(pc_kb, LV_OBJ_FLAG_HIDDEN); }
// تپ روی هر جای خارج از کادر عدد → کیبورد بسته و فوکس برداشته شود (کادر با تپ دوباره باز می‌کند)
static void pc_outside_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (!pc_kb || lv_obj_has_flag(pc_kb, LV_OBJ_FLAG_HIDDEN)) return;
  pc_hide_kb();
  if (pc_ta) lv_obj_remove_state(pc_ta, LV_STATE_FOCUSED);
}
static void pc_show_kb_for_ta() {
  if (!pc_ta) return;
  // کیبورد لمسی فقط با تنظیم vkb روشن و بدون کیبورد USB — همان الگوی F4 (admin_ui)
  if (!GUI.virtual_kb_enable || usb_keyboard_present()) return;
  if (!pc_kb) {
    pc_kb = lv_keyboard_create(lv_layer_top());
    lv_keyboard_set_mode(pc_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(pc_kb, 480, 140);
    lv_obj_align(pc_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(pc_kb, [](lv_event_t *e) {
      if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
      }
    }, LV_EVENT_VALUE_CHANGED, NULL);
  }
  lv_keyboard_set_textarea(pc_kb, pc_ta);
  lv_obj_clear_flag(pc_kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(pc_kb);
  lv_obj_add_state(pc_ta, LV_STATE_FOCUSED);
  lv_textarea_set_cursor_pos(pc_ta, LV_TEXTAREA_CURSOR_LAST);
}

static void pc_set_texts(const char *title, const char *body, bool show_ta,
                         bool show_ok, const char *ok_txt) {
  char t[96], b[336], k[48];
  if (pc_title && title) { pc_san(title, t, sizeof(t)); lv_label_set_text(pc_title, t); }
  if (pc_body && body) { pc_san(body, b, sizeof(b)); lv_label_set_text(pc_body, b); }
  if (pc_ta) {
    if (show_ta) {
      lv_obj_clear_flag(pc_ta, LV_OBJ_FLAG_HIDDEN);
      // فقط تغییرِ واقعیِ buf کادر را بازنویسی می‌کند؛ تایپِ روی کیبوردِ روی‌صفحه پاک نمی‌شود
      if (pitch_cal_buf != pc_ta_synced) {
        lv_textarea_set_text(pc_ta, pitch_cal_buf.c_str());
        pc_ta_synced = pitch_cal_buf;
      }
      if (pc_stage_fresh) pc_show_kb_for_ta();
    } else { lv_obj_add_flag(pc_ta, LV_OBJ_FLAG_HIDDEN); pc_hide_kb(); }
  }
  if (pc_btn_ok) {
    if (show_ok) lv_obj_clear_flag(pc_btn_ok, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(pc_btn_ok, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *l = lv_obj_get_child(pc_btn_ok, 0);
    if (l && ok_txt) { pc_san(ok_txt, k, sizeof(k)); lv_label_set_text(l, k); }
  }
}

static void pc_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (pitch_cal_motion_busy) { pitch_cal_abort(PCERR_ABORT); return; }
  GUI.buzzer_cmd_short = true;
  pitch_cal_reset_state();
}

static bool pitch_cal_start_auto(bool has_end_sensor) {
  if (device_locked || STP.flag_is_cutting || STP.motorIs_moving || STP.begin_calibration ||
      sensor_alert_motion_lock || IOS.stop_button_state) {
    pitch_cal_err = PCERR_BUSY;
    pitch_cal_stage = PC_ERROR;
    return false;
  }
  if (!pinmap_in_role_connected(0)) { pitch_cal_err = PCERR_FIRST_SENSOR; pitch_cal_stage = PC_ERROR; return false; }
  if (has_end_sensor && !pinmap_in_role_connected(1)) { pitch_cal_err = PCERR_END_SENSOR; pitch_cal_stage = PC_ERROR; return false; }
  pitch_cal_has_end_sensor = has_end_sensor;
  pitch_cal_first_mm = pitch_cal_first_line_mm();
  pitch_cal_cancel_requested = false;
  pitch_cal_end_release_pulses = 0;
  pitch_cal_stage = PC_HOME;
  return true;
}

static void pitch_cal_apply_manual_scale() {
  if (device_locked || STP.flag_is_cutting || STP.motorIs_moving || STP.begin_calibration) {
    pitch_cal_err = PCERR_BUSY; pitch_cal_stage = PC_ERROR; return;
  }
  if (pc_ta) pitch_cal_buf = String(lv_textarea_get_text(pc_ta));
  float scale = 0.0f;
  if (!pitch_cal_parse_decimal(&scale) || !pitch_cal_compute_manual_scale(scale)) { pitch_cal_stage = PC_ERROR; return; }
  float old_ppmm = axisPulsesPerMm();
  if (old_ppmm < 1.0f) old_ppmm = 1.0f;
  pitch_cal_measured = (float)STP.currentPosition_p / old_ppmm;
  pitch_cal_stage = PC_MANUAL_CONFIRM;
}

static void pc_ok_cb(lv_event_t *e) {
  if (e && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  if (pitch_cal_stage == PC_CONFIRM) { pitch_cal_stage = PC_END_SENSOR; return; }
  if (pitch_cal_stage == PC_END_SENSOR) { pitch_cal_start_auto(true); return; }
  if (pitch_cal_stage == PC_ENTER_MM) {
    if (pc_ta) pitch_cal_buf = String(lv_textarea_get_text(pc_ta));
    float measured = 0.0f;
    if (pitch_cal_parse_decimal(&measured) && pitch_cal_compute_from_measure(measured)) pitch_cal_stage = PC_RESULT;
    else pitch_cal_stage = PC_ERROR;
    return;
  }
  if (pitch_cal_stage == PC_ENTER_SCALE) { pitch_cal_apply_manual_scale(); return; }
  if (pitch_cal_stage == PC_RESULT || pitch_cal_stage == PC_MANUAL_CONFIRM) {
    pitch_cal_commit_save();
    ui_toast("کالیبره ذخیره شد", "Calibration saved", 2200);
    pitch_cal_reset_state();
    return;
  }
  if (pitch_cal_stage == PC_ERROR) pitch_cal_reset_state();
}

static void pc_alt_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  if (pitch_cal_stage == PC_CONFIRM) { pitch_cal_buf = ""; pitch_cal_stage = PC_ENTER_SCALE; }
  else if (pitch_cal_stage == PC_END_SENSOR) pitch_cal_start_auto(false);
}

static void pc_ta_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_FOCUSED) return;
  if (pitch_cal_stage == PC_ENTER_MM || pitch_cal_stage == PC_ENTER_SCALE) pc_show_kb_for_ta();
}

static void pitch_cal_ensure_ui() {
  if (pc_root) return;
  pc_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(pc_root, 480, 320); lv_obj_set_pos(pc_root, 0, 0);
  lv_obj_set_style_bg_color(pc_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(pc_root, LV_OPA_70, 0); lv_obj_set_style_border_width(pc_root, 0, 0);
  lv_obj_set_style_pad_all(pc_root, 0, 0); lv_obj_clear_flag(pc_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(pc_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(pc_root, pc_outside_click_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *card = lv_obj_create(pc_root);
  lv_obj_set_size(card, 400, 250); lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 24);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0); lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 0, 0); lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(card, pc_outside_click_cb, LV_EVENT_CLICKED, NULL);
  pc_title = lv_label_create(card);
  lv_obj_set_style_text_font(pc_title, &ui_font_naz24, 0); lv_obj_set_style_text_color(pc_title, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_text_align(pc_title, LV_TEXT_ALIGN_CENTER, 0); lv_obj_set_style_base_dir(pc_title, LV_BASE_DIR_RTL, 0);
  lv_obj_align(pc_title, LV_ALIGN_TOP_MID, 0, -9);
  pc_body = lv_label_create(card);
  lv_obj_set_style_text_font(pc_body, &ui_font_naz20, 0); lv_obj_set_style_text_color(pc_body, lv_color_hex(0x3a3a3c), 0);
  lv_obj_set_width(pc_body, 360); lv_label_set_long_mode(pc_body, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(pc_body, LV_TEXT_ALIGN_RIGHT, 0); lv_obj_set_style_base_dir(pc_body, LV_BASE_DIR_RTL, 0);
  lv_obj_align(pc_body, LV_ALIGN_TOP_MID, 0, 27);
  pc_status = lv_label_create(card);
  lv_obj_set_style_text_font(pc_status, &lv_font_montserrat_14, 0); lv_obj_set_style_text_color(pc_status, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_base_dir(pc_status, LV_BASE_DIR_LTR, 0); lv_obj_align(pc_status, LV_ALIGN_TOP_MID, 0, 128);
  lv_label_set_text(pc_status, "");
  pc_ta = lv_textarea_create(card);
  lv_obj_set_size(pc_ta, 110, 42);
  lv_textarea_set_one_line(pc_ta, true); lv_textarea_set_max_length(pc_ta, 10);
  lv_textarea_set_accepted_chars(pc_ta, "0123456789."); lv_textarea_set_placeholder_text(pc_ta, "");
  lv_obj_set_style_text_align(pc_ta, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_pad_left(pc_ta, 6, 0); lv_obj_set_style_pad_right(pc_ta, 6, 0);
  lv_obj_set_style_text_font(pc_ta, &lv_font_montserrat_20, 0);
  lv_obj_set_style_base_dir(pc_ta, LV_BASE_DIR_LTR, 0);
  lv_obj_add_event_cb(pc_ta, pc_ta_click_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(pc_ta, pc_ta_click_cb, LV_EVENT_FOCUSED, NULL); lv_obj_add_flag(pc_ta, LV_OBJ_FLAG_HIDDEN);
  pc_unit = lv_label_create(card);
  lv_obj_set_style_text_font(pc_unit, &ui_font_naz18b, 0); lv_obj_set_style_text_color(pc_unit, lv_color_hex(0x475569), 0);
  lv_label_set_text(pc_unit, "میکرومتر"); lv_obj_add_flag(pc_unit, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(pc_unit, LV_OBJ_FLAG_CLICKABLE);
  pc_btn_x = lv_button_create(card);
  lv_obj_set_size(pc_btn_x, 32, 32); lv_obj_align(pc_btn_x, LV_ALIGN_TOP_RIGHT, 10, -10);
  lv_obj_set_style_bg_color(pc_btn_x, lv_color_hex(0xd92d20), 0); lv_obj_set_style_radius(pc_btn_x, 10, 0);
  lv_obj_add_event_cb(pc_btn_x, pc_cancel_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *xl = lv_label_create(pc_btn_x); lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xffffff), 0); lv_obj_center(xl);
  pc_btn_ok = lv_button_create(card);
  lv_obj_set_size(pc_btn_ok, 120, 40); lv_obj_set_pos(pc_btn_ok, 140, 176);
  lv_obj_set_style_bg_color(pc_btn_ok, lv_color_hex(0x0a84ff), 0); lv_obj_set_style_radius(pc_btn_ok, 10, 0);
  lv_obj_add_event_cb(pc_btn_ok, pc_ok_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *ol = lv_label_create(pc_btn_ok); lv_obj_set_style_text_font(ol, &ui_font_naz18b, 0); lv_label_set_text(ol, "ادامه"); lv_obj_center(ol);
  pc_btn_alt = lv_button_create(card);
  lv_obj_set_size(pc_btn_alt, 120, 40); lv_obj_set_pos(pc_btn_alt, 208, 176);
  lv_obj_set_style_bg_color(pc_btn_alt, lv_color_hex(0xff9f0a), 0); lv_obj_set_style_radius(pc_btn_alt, 10, 0);
  lv_obj_add_event_cb(pc_btn_alt, pc_alt_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *al = lv_label_create(pc_btn_alt); lv_obj_set_style_text_font(al, &ui_font_naz18b, 0); lv_label_set_text(al, (GUI.system_language == 2 ? "Manual" : "ورود دستی")); lv_obj_center(al);
  lv_obj_add_flag(pc_root, LV_OBJ_FLAG_HIDDEN);
}

// چینش مرحله‌ای: صفحات ورود عدد (واحد راست + کادر وسط + دکمه چپ) و صفحات انتخاب (جفت وسط)
static void pc_apply_layout() {
  if (!pc_btn_ok) return;
  const bool enter = (pitch_cal_stage == PC_ENTER_MM || pitch_cal_stage == PC_ENTER_SCALE);
  const bool alt = (pitch_cal_stage == PC_CONFIRM || pitch_cal_stage == PC_END_SENSOR);
  const bool en = (GUI.system_language == 2);
  if (enter) {
    lv_obj_set_size(pc_btn_ok, 98, 42);
    if (en) {
      lv_obj_set_pos(pc_ta, 45, 86);
      if (pc_unit) {
        lv_obj_set_style_text_font(pc_unit, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(pc_unit, 165, 96);
        lv_obj_clear_flag(pc_unit, LV_OBJ_FLAG_HIDDEN);
      }
      lv_obj_set_pos(pc_btn_ok, 245, 86);
    } else {
      lv_obj_set_pos(pc_ta, 160, 86);
      if (pc_unit) {
        lv_obj_set_style_text_font(pc_unit, &ui_font_naz18b, 0);
        lv_obj_set_pos(pc_unit, 280, 96);
        lv_obj_clear_flag(pc_unit, LV_OBJ_FLAG_HIDDEN);
      }
      lv_obj_set_pos(pc_btn_ok, 50, 86);
    }
  } else {
    lv_obj_set_size(pc_btn_ok, 120, 40);
    lv_obj_set_pos(pc_btn_ok, alt ? 72 : 140, 176);
    if (pc_unit) lv_obj_add_flag(pc_unit, LV_OBJ_FLAG_HIDDEN);
  }
  if (pc_btn_alt && !alt) lv_obj_add_flag(pc_btn_alt, LV_OBJ_FLAG_HIDDEN);
}

static void pitch_cal_start_from_ui() {
  if (pitch_cal_busy()) { GUI.buzzer_cmd_long = true; ui_toast("کالیبره در حال اجرا است", "Calibration is already running", 1800); return; }
  if (device_locked || STP.flag_is_cutting || STP.motorIs_moving || STP.begin_calibration || IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true; ui_toast("اکنون کالیبره ممکن نیست؛ دستگاه مشغول یا استاپ فعال است", "Busy or STOP active", 2200); return;
  }
  pitch_cal_err = PCERR_NONE; pitch_cal_pulses = 0; pitch_cal_buf = "";
  pitch_cal_cancel_requested = false; pitch_cal_end_release_pulses = 0;
  pitch_cal_stage = PC_CONFIRM; GUI.lock_screen = true; GUI.buzzer_cmd_short = true;
}

// ---- پیام «محور کالیبره نیست» بعد از کالیبرهٔ ناتمام؛ تأیید = همان سناریوی کلید موقعیت‌یابی ----
static bool pc_run_started = false;   // کالیبره واقعاً به حرکت رسید (مرحلهٔ HOME)
static lv_obj_t *pc_uncal_root = NULL;

static void pc_uncal_close() {
  if (pc_uncal_root && lv_obj_is_valid(pc_uncal_root)) lv_obj_delete(pc_uncal_root);
  pc_uncal_root = NULL;
}
static void pc_uncal_close_cb(lv_event_t *e) {
  (void)e; if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true; pc_uncal_close();
}
static void pc_uncal_go_cb(lv_event_t *e) {
  (void)e; if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  pc_uncal_close();
  // درخواست مالک: در همان صفحه اجرا نشود؛ اول به صفحهٔ موقعیت‌یابی (اصلی) برو،
  // همان‌جا اجرا و تمام شود. تعویض صفحه فقط از صف Task2 (نه داخل callback لمس)
  GUI.pending_goto_scr = 0;
  STP.referenced = false;
  STP.goref_cmd = true;   // Task3 روی صفحهٔ اصلی اجرا می‌کند
  ui_toast("موقعیت‌یابی شروع شد", "Reference search started", 1600);
}
static void pc_uncal_prompt_show() {
  if (pc_uncal_root) return;
  const bool en = (GUI.system_language == 2);
  pc_uncal_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(pc_uncal_root, 480, 320); lv_obj_set_pos(pc_uncal_root, 0, 0);
  lv_obj_set_style_bg_color(pc_uncal_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(pc_uncal_root, LV_OPA_70, 0); lv_obj_set_style_border_width(pc_uncal_root, 0, 0);
  lv_obj_set_style_pad_all(pc_uncal_root, 0, 0); lv_obj_clear_flag(pc_uncal_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(pc_uncal_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *card = lv_obj_create(pc_uncal_root);
  lv_obj_set_size(card, 380, 170); lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0); lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 0, 0); lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *t = lv_label_create(card);
  lv_obj_set_width(t, 356); lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(t, en ? &lv_font_montserrat_20 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x1c1c1e), 0);
  lv_label_set_text(t, en ? "Axis is not calibrated" : "محور کالیبره نیست");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 6);
  lv_obj_t *b = lv_label_create(card);
  lv_obj_set_width(b, 356); lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(b, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(b, lv_color_hex(0x3a3a3c), 0);
  lv_label_set_text(b, en ? "Run the first reference zero now?" : "عدد موتور معتبر نیست. موقعیت‌یابی اولیه انجام شود؟");
  lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 48);
  lv_obj_t *go = lv_button_create(card);
  lv_obj_set_size(go, 160, 44); lv_obj_set_pos(go, 24, 112);
  lv_obj_set_style_bg_color(go, lv_color_hex(0x1c9b67), 0); lv_obj_set_style_radius(go, 10, 0); lv_obj_set_style_border_width(go, 0, 0);
  lv_obj_add_event_cb(go, pc_uncal_go_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *gl = lv_label_create(go); lv_label_set_text(gl, en ? "Reference" : "موقعیت‌یابی");
  lv_obj_set_style_text_font(gl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(gl, lv_color_hex(0xffffff), 0); lv_obj_center(gl);
  lv_obj_t *no = lv_button_create(card);
  lv_obj_set_size(no, 160, 44); lv_obj_set_pos(no, 196, 112);
  lv_obj_set_style_bg_color(no, lv_color_hex(0x8e8e93), 0); lv_obj_set_style_radius(no, 10, 0); lv_obj_set_style_border_width(no, 0, 0);
  lv_obj_add_event_cb(no, pc_uncal_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *nl = lv_label_create(no); lv_label_set_text(nl, en ? "Cancel" : "انصراف");
  lv_obj_set_style_text_font(nl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(nl, lv_color_hex(0xffffff), 0); lv_obj_center(nl);
}

static void pitch_cal_ui_tick() {
  if (pitch_cal_stage == PC_IDLE) {
    if (pc_root && !lv_obj_has_flag(pc_root, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(pc_root, LV_OBJ_FLAG_HIDDEN);
    if (pc_last_stage != PC_IDLE && pc_run_started) {
      pc_run_started = false;
      if (!pc_saved_ok) pc_uncal_prompt_show();  // کالیبرهٔ ناتمام: پیام + پیشنهاد موقعیت‌یابی
      pc_saved_ok = false;
    }
    pc_last_stage = PC_IDLE; return;
  }
  pitch_cal_ensure_ui(); lv_obj_clear_flag(pc_root, LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(pc_root); GUI.lock_screen = true;
  pc_stage_fresh = (pitch_cal_stage != pc_last_stage);
  if (pc_stage_fresh) pc_ta_synced = String('\x01');  // ورودِ مرحله: کادر یک‌بار از buf ساخته می‌شود
  if (pc_stage_fresh && pitch_cal_stage == PC_HOME && !pc_run_started) { pc_run_started = true; pc_saved_ok = false; }
  char st[96] = "";
  if (pitch_cal_stage == PC_CONFIRM) {
    char body[280], scale[48];
    if (pitch_cal_stored_scale_valid()) snprintf(scale, sizeof(scale), "%.4f میکرومتر/پالس", (double)STP.um_per_pulse);
    else snprintf(scale, sizeof(scale), "نامعتبر");
    snprintf(body, sizeof(body), (GUI.system_language == 2 ? "Saved value: %s\nAuto: measures with the sensors.\nManual: enter the um/pulse value." : "مقدار ذخیره‌شده: %s\nخودکار: با سنسورها اندازه می‌گیرد.\nورود دستی: مقدار میکرومتر بر پالس (um/pulse) را وارد می‌کنید."), scale);
    pc_set_texts((GUI.system_language == 2 ? "Axis Pitch Calibration" : "کالیبره گام محور"), body, false, true, (GUI.system_language == 2 ? "Auto" : "خودکار"));
    if (pc_btn_alt) { lv_obj_clear_flag(pc_btn_alt, LV_OBJ_FLAG_HIDDEN); lv_label_set_text(lv_obj_get_child(pc_btn_alt, 0), (GUI.system_language == 2 ? "Manual" : "ورود دستی")); }
  } else if (pitch_cal_stage == PC_END_SENSOR) {
    pc_set_texts((GUI.system_language == 2 ? "End-of-axis sensor" : "سنسور انتهای محور"), (GUI.system_language == 2 ? "Is the end-of-axis sensor (I1) connected?\nYes: sensor distance is measured.\nNo: the axis turns exactly 15 turns; enter the absolute length." : "آیا سنسور انتهای محور (ورودی ١) متصل و سالم است؟\nبله: فاصله دو سنسور اندازه‌گیری می‌شود.\nخیر: محور دقیقا ١٥ دور حرکت می‌کند و طول مطلق را وارد می‌کنید."), false, true, (GUI.system_language == 2 ? "Yes" : "بله"));
    if (pc_btn_alt) { lv_obj_clear_flag(pc_btn_alt, LV_OBJ_FLAG_HIDDEN); lv_label_set_text(lv_obj_get_child(pc_btn_alt, 0), (GUI.system_language == 2 ? "No" : "خیر")); }
  } else if (pitch_cal_stage == PC_HOME) {
    pc_set_texts((GUI.system_language == 2 ? "Axis Pitch Calibration" : "کالیبره گام محور"), (GUI.system_language == 2 ? "Half speed to the first sensor; controlled stop, slow release.\nPlease stay away from the axis." : "حرکت با نصف سرعت به سنسور اول؛ سپس توقف کنترل‌شده و رهاسازی آهسته.\nلطفاً به محور نزدیک نشوید."), false, false, NULL); snprintf(st, sizeof(st), "IN0 HOME");
  } else if (pitch_cal_stage == PC_SAMPLE) {
    if (pitch_cal_has_end_sensor) { pc_set_texts((GUI.system_language == 2 ? "Axis Pitch Calibration" : "کالیبره گام محور"), (GUI.system_language == 2 ? "Moving to the end sensor, counting real pulses.\nAfter the stop the sensor is released slowly." : "حرکت به سنسور انتها و شمارش پالس‌های واقعی.\nپس از توقف، سنسور آهسته رها می‌شود."), false, false, NULL); snprintf(st, sizeof(st), "IN1 SAMPLE"); }
    else { pc_set_texts((GUI.system_language == 2 ? "Axis Pitch Calibration" : "کالیبره گام محور"), "حرکت دقیقاً ۱۵ دور موتور؛ سپس طول مطلق محل محور را وارد کنید.", false, false, NULL); snprintf(st, sizeof(st), "15 REV"); }
  } else if (pitch_cal_stage == PC_END_RELEASE) {
    pc_set_texts((GUI.system_language == 2 ? "Axis Pitch Calibration" : "کالیبره گام محور"), (GUI.system_language == 2 ? "The 100 ms stop is done; slowly releasing the end sensor." : "توقف ۱۰۰ میلی‌ثانیه انجام شد؛ رهاسازی آهستۀ سنسور انتها در حال انجام است."), false, false, NULL); snprintf(st, sizeof(st), "IN1 RELEASE");
  } else if (pitch_cal_stage == PC_ENTER_MM) {
    char body[240]; snprintf(body, sizeof(body), (GUI.system_language == 2 ? "Motion finished (%ld pulses).\nEnter the absolute axis length in mm." : "حرکت تمام شد (%ld پالس).\nاندازه مطلق فعلی محور را به میلی‌متر وارد کنید."), (long)pitch_cal_pulses);
    lv_textarea_set_placeholder_text(pc_ta, "");
    if (pc_unit) lv_label_set_text(pc_unit, (GUI.system_language == 2 ? "mm" : "میلی‌متر"));
    pc_set_texts((GUI.system_language == 2 ? "Axis length" : "اندازه محور"), body, true, true, (GUI.system_language == 2 ? "Compute" : "محاسبه"));
  } else if (pitch_cal_stage == PC_ENTER_SCALE) {
    lv_textarea_set_placeholder_text(pc_ta, "");
    if (pc_unit) lv_label_set_text(pc_unit, (GUI.system_language == 2 ? "um" : "میکرومتر"));
    pc_set_texts((GUI.system_language == 2 ? "Manual pitch entry" : "ورود دستی گام"), (GUI.system_language == 2 ? "Enter the exact um/pulse.\nExample: 12.5\nAfter saving, a new reference is required." : "عدد دقیق میکرومتر بر پالس را وارد کنید."), true, true, (GUI.system_language == 2 ? "Check" : "بررسی"));
  } else if (pitch_cal_stage == PC_MANUAL_CONFIRM) {
    char body[260]; snprintf(body, sizeof(body), (GUI.system_language == 2 ? "New value: %.4f um/pulse\nAxis pitch: %.4f mm/turn\nMotion stays locked until re-referenced." : "مقدار جدید: %.4f میکرومتر/پالس\nگام محور: %.4f میلی‌متر/دور\nپس از ذخیره، حرکت تا رفرنس مجدد قفل می‌ماند."), (double)pitch_cal_um_pp, (double)pitch_cal_pitch);
    pc_set_texts((GUI.system_language == 2 ? "Confirm manual entry" : "تایید ورود دستی"), body, false, true, (GUI.system_language == 2 ? "Confirm & save" : "تایید و ذخیره"));
  } else if (pitch_cal_stage == PC_RESULT) {
    char body[280]; snprintf(body, sizeof(body), (GUI.system_language == 2 ? "Travel: %.1f mm | Pulses: %ld\n1 pulse = %.6f mm\n(%.3f um/pulse)\nPitch = %.3f mm/turn" : "سفر: %.1f میلی‌متر | پالس: %ld\nهر پالس = %.6f میلی‌متر\n(%.3f میکرومتر/پالس)\nگام محور = %.3f میلی‌متر/دور"), (double)pitch_cal_travel, (long)pitch_cal_pulses, (double)pitch_cal_mm_pp, (double)pitch_cal_um_pp, (double)pitch_cal_pitch);
    pc_set_texts((GUI.system_language == 2 ? "Calibration result" : "نتیجه کالیبره"), body, false, true, (GUI.system_language == 2 ? "Save" : "ذخیره"));
  } else if (pitch_cal_stage == PC_ERROR) {
    const char *msg = (GUI.system_language == 2 ? "Calibration error" : "خطا در کالیبره");
    if (pitch_cal_err == PCERR_TIMEOUT) msg = (GUI.system_language == 2 ? "Sensor not found in the computed time; check path and speed" : "سنسور در زمان محاسبه‌شده پیدا نشد؛ مسیر و سرعت را بررسی کنید");
    else if (pitch_cal_err == PCERR_ABORT) msg = "کالیبره با توقف/انصراف ایمن متوقف شد";
    else if (pitch_cal_err == PCERR_WRONG_DIR) msg = (GUI.system_language == 2 ? "End sensor fired before the first one; check motor direction" : "سنسور انتها پیش از سنسور اول فعال شد؛ جهت موتور را در F4 بررسی کنید");
    else if (pitch_cal_err == PCERR_BAD_MEASURE) msg = "عدد واردشده یا نسبت محاسبه‌شده نامعتبر است";
    else if (pitch_cal_err == PCERR_BUSY) msg = "دستگاه مشغول است یا استاپ فعال است";
    else if (pitch_cal_err == PCERR_FIRST_SENSOR) msg = (GUI.system_language == 2 ? "First sensor (input 0) not connected; auto calibration not started" : "سنسور اول (ورودی ٠) متصل نیست؛ کالیبره خودکار شروع نشد");
    else if (pitch_cal_err == PCERR_END_SENSOR) msg = (GUI.system_language == 2 ? "End sensor (input 1) not connected; choose No or fix wiring" : "سنسور انتها (ورودی ١) متصل نیست؛ گزینۀ خیر را بزنید یا اتصال را اصلاح کنید");
    else if (pitch_cal_err == PCERR_SENSOR_STUCK) msg = "سنسور پس از رهاسازی هنوز فعال است؛ مکانیک یا سیم‌کشی را بررسی کنید";
    else if (pitch_cal_err == PCERR_UNEXPECTED_END) msg = "در حرکت ۱۵ دور به حد فیزیکی رسید؛ مسیر/سنسور انتها را بررسی کنید";
    pc_set_texts("خطا", msg, false, true, "بستن");
  }
  if (pc_stage_fresh) pc_apply_layout();
  if (pc_status) lv_label_set_text(pc_status, st);
  pc_last_stage = pitch_cal_stage;
}

static inline bool pitch_cal_try_digit(char d) {
  if (pitch_cal_stage != PC_ENTER_MM && pitch_cal_stage != PC_ENTER_SCALE) return false;
  if (pitch_cal_buf.length() >= 10) return true;
  pitch_cal_buf += d; if (pc_ta) lv_textarea_set_text(pc_ta, pitch_cal_buf.c_str()); return true;
}
static inline bool pitch_cal_try_backspace() {
  if (pitch_cal_stage != PC_ENTER_MM && pitch_cal_stage != PC_ENTER_SCALE) return false;
  if (pitch_cal_buf.length()) pitch_cal_buf.remove(pitch_cal_buf.length() - 1);
  if (pc_ta) lv_textarea_set_text(pc_ta, pitch_cal_buf.c_str()); return true;
}
static inline bool pitch_cal_try_enter() {
  if (pitch_cal_stage != PC_ENTER_MM && pitch_cal_stage != PC_ENTER_SCALE) return false;
  pc_ok_cb(NULL); return true;
}
