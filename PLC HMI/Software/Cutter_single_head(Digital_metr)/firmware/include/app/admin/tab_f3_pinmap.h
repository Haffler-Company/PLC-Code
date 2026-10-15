#pragma once
// ===================== F3 pin map =====================
static lv_obj_t *f3_pin_lbls[14] = {0};
static lv_obj_t *f3_pin_q9_lbl = NULL;
lv_obj_t *f3_angle_val_lbl = NULL;
static lv_obj_t *f3_dot_lbls[20] = {0};  // تا ۱۰ ردیف × ۲
static lv_obj_t *f3_q_cap = NULL;  // کپسول بالای بلوک خروجی
static lv_obj_t *f3_q_step_hits[3] = {NULL, NULL, NULL};
static lv_obj_t *f3_q_step_badges[3] = {NULL, NULL, NULL};
static lv_obj_t *f3_q_step_labels[3] = {NULL, NULL, NULL};
static lv_obj_t *f3_q_run_pulse_cap = NULL;
static lv_obj_t *f3_q_run_pulse_value = NULL;
static lv_obj_t *f3_q_run_time_cap = NULL;
static lv_obj_t *f3_q_run_time_value = NULL;
static lv_obj_t *f3_stepper_stop_root = NULL;
static bool f3_q_run_status_visible = false;
static uint32_t f3_q_run_last_pulses = UINT32_MAX;
static uint32_t f3_q_run_last_seconds = UINT32_MAX;
static lv_obj_t *f3_stepper_params_root = NULL;
static lv_timer_t *f3_stepper_adjust_timer = NULL;
static int f3_stepper_adjust_code = 0;
static bool f3_stepper_adjust_repeat = false;
static bool f3_stepper_adjust_skip_click = false;
static bool f3_stepper_boundary_notice = false;
static bool f3_pin_wired = false;
static uint8_t f3_layout_ver = 0;
static uint32_t f3_pending_revision_seen = 0;
static uint8_t f3_language_seen = 0;
static bool f3_joy_enabled_seen = true;
#define F3_LAYOUT_VER 89
#define F3_STEPPER_ADJUST_HOLD_MS 1000U
#define F3_STEPPER_ADJUST_REPEAT_MS 100U
#define F3H_STEPPER_PARAMS 100
// راهنمای لمس‌طولانی پایۀ F3 - برای برگشت سریع: 0 کنید یا این بلوک را revert کنید
#ifndef F3_PIN_HELP_ENABLE
#define F3_PIN_HELP_ENABLE 1
#endif

// جویستیک T1/T2 — ردیف اضافه در پنل ورودی (نه I4/I5)
static lv_obj_t *f3_joy_led_l = NULL;
static lv_obj_t *f3_joy_led_r = NULL;
static lv_obj_t *f3_joy_lab_fa_l = NULL;
static lv_obj_t *f3_joy_lab_en_l = NULL;
static lv_obj_t *f3_joy_lab_fa_r = NULL;
static lv_obj_t *f3_joy_lab_en_r = NULL;
static lv_obj_t *f3_joy_badge_l = NULL;
static lv_obj_t *f3_joy_badge_r = NULL;
static bool f3_joy_prev_l = true;
static bool f3_joy_prev_r = true;

// حرف I با خط بالا/پایین - مقیاس نسبت به دایره با inset تا به لبه نچسبد
static void f3_attach_serif_I(lv_obj_t *dot, lv_obj_t *digit_lbl, int badge_sz) {
  if (!dot || !digit_lbl) return;
  const int inset = 3;  // فاصلۀ امن از لبۀ دایره
  const int usable = badge_sz - 2 * inset;
  // نسبت ثابت برای همۀ بج‌های I (هم‌سایز بصری)
  const int bar_w = 6;
  const int bar_h = 2;
  const int stem_w = 2;
  int ih = usable - 2;
  if (ih < 10) ih = 10;
  if (ih > 16) ih = 16;

  lv_obj_t *I = lv_obj_create(dot);
  lv_obj_set_size(I, bar_w, ih);
  lv_obj_set_style_bg_opa(I, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(I, 0, 0);
  lv_obj_set_style_pad_all(I, 0, 0);
  lv_obj_set_style_radius(I, 0, 0);
  lv_obj_clear_flag(I, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

  auto mk = [&](int x, int y, int w, int h) {
    lv_obj_t *o = lv_obj_create(I);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  };
  mk(0, 0, bar_w, bar_h);
  mk((bar_w - stem_w) / 2, bar_h, stem_w, ih - 2 * bar_h);
  mk(0, ih - bar_h, bar_w, bar_h);

  lv_obj_set_style_pad_all(digit_lbl, 0, 0);
  lv_obj_update_layout(digit_lbl);
  int dw = (int)lv_obj_get_width(digit_lbl);
  int dh = (int)lv_obj_get_height(digit_lbl);
  if (dw < 1) dw = 9;
  if (dh < 1) dh = 14;
  const int gap = 1;
  const int total = bar_w + gap + dw;
  const int pair_h = (ih > dh) ? ih : dh;
  const int x0 = (badge_sz - total) / 2;
  const int y0 = (badge_sz - pair_h) / 2;
  lv_obj_set_pos(I, x0, y0 + (pair_h - ih) / 2);
  lv_obj_set_pos(digit_lbl, x0 + bar_w + gap, y0 + (pair_h - dh) / 2);
}

static lv_obj_t *f3_make_pin_lbl(lv_obj_t *parent, const char *txt, int sz, bool navy_metal) {
  // بج پین: navy_metal=سرمه‌ای متالیک (دکمه‌های Q)؛ وگرنه مشکی جمع‌وجور (ورودی‌های I)
  if (sz < 18) sz = 18;
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_set_size(dot, sz, sz);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(dot, 0, 0);
  lv_obj_set_style_clip_corner(dot, false, 0);  // تا متن داخل دایره بریده نشود
  if (navy_metal) {
    // سرمه‌ای + لبۀ فولادی روشن برای حس متالیک (بدون سایۀ سنگین)
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x1A365D), 0);
    lv_obj_set_style_border_width(dot, 2, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(0x9BB8D4), 0);
    lv_obj_set_style_outline_width(dot, 1, 0);
    lv_obj_set_style_outline_pad(dot, 0, 0);
    lv_obj_set_style_outline_color(dot, lv_color_hex(0x2E5A87), 0);
    lv_obj_set_style_outline_opa(dot, LV_OPA_50, 0);
  } else {
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x1c1c1e), 0);
    lv_obj_set_style_border_width(dot, 1, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(0x3a3a3c), 0);
  }
  lv_obj_clear_flag(dot, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_t *l = lv_label_create(dot);
  // داخل دایره: فقط فونت‌های فعال پروژه (۱۲/۱۴/۱۶…) — رقم I با ۱۴ خوانا
  const bool i_digit = (!navy_metal && txt && txt[0] == 'I' && txt[1] >= '0' && txt[1] <= '9');
  lv_obj_set_style_text_font(l, i_digit ? &lv_font_montserrat_14 : &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_opa(l, LV_OPA_COVER, 0);
  lv_obj_set_style_text_letter_space(l, 0, 0);
  if (i_digit) {
    char dig[2] = { txt[1], '\0' };
    lv_label_set_text(l, dig);
    f3_attach_serif_I(dot, l, sz);
  } else {
    lv_label_set_text(l, txt);
    lv_obj_center(l);
  }
  lv_obj_move_foreground(l);
  return dot;
}

static void f3_set_badge_active_yellow(lv_obj_t *dot, bool active) {
  if (!dot || !lv_obj_is_valid(dot)) return;
  uintptr_t prev = (uintptr_t)lv_obj_get_user_data(dot);
  uintptr_t cur = active ? 2 : 1;
  if (prev == cur) return; // گارد ضد لگ: فقط در صورت تغییر وضعیت اجرا شود
  lv_obj_set_user_data(dot, (void *)cur);
  if (active) {
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xffcc00), 0); // زرد خروجی فعال
    lv_obj_set_style_border_color(dot, lv_color_hex(0xffe066), 0);
    uint32_t cn = lv_obj_get_child_count(dot);
    for (uint32_t ci = 0; ci < cn; ci++) {
      lv_obj_t *ch = lv_obj_get_child(dot, ci);
      if (ch && lv_obj_check_type(ch, &lv_label_class)) {
        lv_obj_set_style_text_color(ch, lv_color_hex(0x000000), 0); // متن تیره روی زمینه زرد
      }
    }
  } else {
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x1A365D), 0); // سرمه‌ای متالیک
    lv_obj_set_style_border_color(dot, lv_color_hex(0x9BB8D4), 0);
    uint32_t cn = lv_obj_get_child_count(dot);
    for (uint32_t ci = 0; ci < cn; ci++) {
      lv_obj_t *ch = lv_obj_get_child(dot, ci);
      if (ch && lv_obj_check_type(ch, &lv_label_class)) {
        lv_obj_set_style_text_color(ch, lv_color_hex(0xffffff), 0);
      }
    }
  }
}

static void f3_input_source_tag(uint8_t source, char *buf, size_t buf_size) {
  if (!buf || buf_size < 2) return;
  if (source < 8)
    snprintf(buf, buf_size, "I%u", (unsigned)source);
  else if (source == 8)
    snprintf(buf, buf_size, "T1");
  else if (source == 9)
    snprintf(buf, buf_size, "T2");
  else
    snprintf(buf, buf_size, "X");
}

static void f3_style_q_btn_label(lv_obj_t *lbl, bool persian, int ox, int oy, int line_sp, int max_w) {
  if (!lbl) return;
  lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (max_w > 0) {
    lv_obj_set_width(lbl, max_w);
    lv_obj_set_height(lbl, LV_SIZE_CONTENT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
  }
  lv_obj_align(lbl, LV_ALIGN_CENTER, ox, oy);
  if (persian) {
    lv_obj_set_style_text_font(lbl, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(lbl, line_sp, LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

#if F3_PIN_HELP_ENABLE
// ---- F3 long-press pin help (کادر تیره ملایم؛ خاموش با F3_PIN_HELP_ENABLE=0) ----
enum {
  F3H_I2 = 1, F3H_I0, F3H_I1, F3H_I3, F3H_I6, F3H_I7, F3H_I4, F3H_I5,
  F3H_T1, F3H_T2,
  F3H_Q5, F3H_Q3, F3H_Q6, F3H_Q4, F3H_Q8, F3H_Q7, F3H_Q0, F3H_Q1, F3H_Q2
};

static lv_obj_t *f3_help_root = NULL;
static int f3_help_active_id = 0;
static bool f3_help_q_armed = false;
static lv_obj_t *f3_help_q_btn = NULL;
static bool f3_help_q_ev_wired = false;
static uint32_t f3_help_opened_ms = 0;
static bool f3_help_close_armed = false;  // بعد از باز شدن، اولین رها را برای بستن نادیده بگیر
static bool f3_help_wait_finger_up = false;  // همان انگشت long-press نباید کادر را ببندد
static uint32_t f3_q_suppress_until_ms = 0;  // تا این زمان RELEASED روی Q تاگل/GPIO نزند
static lv_obj_t *f3_pinmap_root = NULL;
static lv_obj_t *f3_pinmap_confirm_btn = NULL;
static lv_obj_t *f3_pinmap_cancel_btn = NULL;
static lv_obj_t *f3_pinmap_reset_btn = NULL;
static lv_obj_t *f3_pinmap_choice_btn[11] = {0};
static lv_obj_t *f3_pinmap_test_pulses_value = NULL;
static lv_obj_t *f3_pinmap_test_period_value = NULL;
static lv_obj_t *f3_pinmap_reset_confirm_root = NULL;
static lv_obj_t *f3_pinmap_reset_confirm_bar = NULL;
static int f3_pinmap_role = -1;
static uint8_t f3_pinmap_pending_source = PINMAP_IN_NC;
static bool f3_pinmap_is_output = false;
static bool f3_pinmap_apply_busy = false;
static bool f3_pinmap_reset_confirm_output = false;
static uint32_t f3_pinmap_reset_confirm_started_ms = 0;
static volatile bool f3_pinmap_open_atomic = false;
static volatile bool f3_pinmap_external_close_req = false;
#define F3_PINMAP_RESET_CONFIRM_MS 6000U

static void f3_hold_cancel(void);
static void f3_pin_help_close(void);
static void f3_pinmap_close(void);
static void f3_stepper_params_close(void);
static void f3_stepper_params_open(void);
static void f3_pinmap_reset_confirm_close(void);
static const char *f3_pin_help_code(int id);
static const char *f3_pin_help_title_fa(int id);
static const char *f3_pin_help_title_en(int id);
static int f3_pin_help_id_from_obj(lv_obj_t *obj);
void f3_pinmap_reflow();

static int f3_pinmap_input_role_from_help(int id) {
  switch (id) {
    case F3H_I0: return 0;
    case F3H_I1: return 1;
    case F3H_I2: return 2;
    case F3H_I3: return 3;
    case F3H_I4: return 4;
    case F3H_I5: return 5;
    case F3H_I6: return 6;
    case F3H_I7: return 7;
    case F3H_T1: return 8;
    case F3H_T2: return 9;
    default: return -1;
  }
}

static int f3_pinmap_output_role_from_help(int id) {
  switch (id) {
    case F3H_Q0: return 0;
    case F3H_Q1: return 1;
    case F3H_Q2: return 2;
    case F3H_Q3: return 3;
    case F3H_Q4: return 4;
    case F3H_Q5: return 5;
    case F3H_Q6: return 6;
    case F3H_Q7: return 7;
    case F3H_Q8: return 8;
    default: return -1;
  }
}

static bool f3_pinmap_machine_safe() {
  if (STP.motorIs_moving || STP.moveStepperCMD || STP.jogEnabled || keyboardJogActive ||
      STP.flag_is_cutting || STP.goref_cmd || STP.go_sampling_CMD || STP.begin_calibration ||
      IOS.saw_is_on || IOS.clamp_engaged || IOS.cut_confirm_pending ||
      f3_stepper_test_cmd || f3_stepper_test_busy || pitch_cal_busy())
    return false;
  return true;
}

bool f3_pinmap_dialog_is_open() {
  return f3_pinmap_root && lv_obj_is_valid(f3_pinmap_root);
}

bool ui_resume_f3_pinmap_is_open() { return f3_pinmap_dialog_is_open(); }

int ui_resume_f3_pinmap_pack() {
  if (!f3_pinmap_dialog_is_open() || f3_pinmap_role < 0) return -1;
  return (f3_pinmap_is_output ? 0x100 : 0) | (f3_pinmap_role & 0xFF);
}

int ui_resume_f3_pinhelp_id() {
  if (!f3_help_root || !lv_obj_is_valid(f3_help_root) ||
      lv_obj_has_flag(f3_help_root, LV_OBJ_FLAG_HIDDEN))
    return 0;
  return f3_help_active_id;
}

void f3_pinmap_request_external_close() {
  if (f3_pinmap_open_atomic) f3_pinmap_external_close_req = true;
}

static void f3_pinmap_runtime_tick() {
  if (f3_pinmap_reset_confirm_root && lv_obj_is_valid(f3_pinmap_reset_confirm_root)) {
    const uint32_t elapsed = (uint32_t)(millis() - f3_pinmap_reset_confirm_started_ms);
    if (f3_pinmap_reset_confirm_bar) {
      const int width = (int)((elapsed >= F3_PINMAP_RESET_CONFIRM_MS) ? 280U
        : (280UL * elapsed) / F3_PINMAP_RESET_CONFIRM_MS);
      lv_obj_set_width(f3_pinmap_reset_confirm_bar, width);
    }
    if (elapsed >= F3_PINMAP_RESET_CONFIRM_MS) f3_pinmap_reset_confirm_close();
  }
  // چراغ هر ردیف همان منبع فیزیکیِ نوشته‌شده روی بج را نشان می‌دهد؛
  // نگاشت موقت F3 نباید چراغ را روی نگاشت ذخیره‌شدۀ قبلی نگه دارد.
  if (GUI.scr_id == 2 && objects.tabview2_1 &&
      (int)lv_tabview_get_tab_act(objects.tabview2_1) == 2) {
    static uint8_t prev_led_br[8] = {0};
    static int8_t prev_tab_seen = -1;
    bool force = (prev_tab_seen != 2);
    prev_tab_seen = 2;
    lv_obj_t *in_leds[8] = {
      objects.led2_3_1, objects.led2_3_2, objects.led2_3_8, objects.led2_3_4,
      objects.led2_3_5, objects.led2_3_6, objects.led2_3_3, objects.led2_3_7
    };
    for (int r = 0; r < 8; r++) {
      if (!in_leds[r] || !lv_obj_is_valid(in_leds[r])) continue;
      const uint8_t src = pinmap_in_get_pending_source(r);
      const bool act = pinmap_in_source_active(src);
      const uint8_t br = act ? 255 : 0;
      if (force || prev_led_br[r] != br) {
        prev_led_br[r] = br;
        lv_led_set_brightness(in_leds[r], br);
      }
    }
    const bool source_l_active = pinmap_in_source_active(pinmap_in_get_pending_source(9));
    const bool source_r_active = pinmap_in_source_active(pinmap_in_get_pending_source(8));
    if (f3_joy_led_l && lv_obj_is_valid(f3_joy_led_l)) {
      if (force || f3_joy_prev_l != source_l_active) {
        lv_led_set_brightness(f3_joy_led_l, source_l_active ? 255 : 0);
        f3_joy_prev_l = source_l_active;
      }
    }
    if (f3_joy_led_r && lv_obj_is_valid(f3_joy_led_r)) {
      if (force || f3_joy_prev_r != source_r_active) {
        lv_led_set_brightness(f3_joy_led_r, source_r_active ? 255 : 0);
        f3_joy_prev_r = source_r_active;
      }
    }

    // بروزرسانی رنگ زرد پس‌زمینه بج‌های Q و کلیدها هنگام فعال/روشن بودن خروجی از هر نقطه سیستم
    // رله‌های خروجی و جک‌ها Active-LOW هستند (LOW = فعال، HIGH = خاموش)
    const bool q5_act = (IOS.spray_pin >= 0 && pinmap_out_read(IOS.spray_pin) == LOW);
    const bool q3_act = (IOS.u_clip_pin >= 0 && pinmap_out_read(IOS.u_clip_pin) == LOW);
    const bool q6_act = (IOS.saw_pin >= 0 && pinmap_out_read(IOS.saw_pin) == LOW);
    const bool q4_act = (IOS.f_clip_pin >= 0 && pinmap_out_read(IOS.f_clip_pin) == LOW);
    const bool q8_lift_act = (IOS.sawl_ift_pin >= 0 && pinmap_out_read(IOS.sawl_ift_pin) == LOW);

    bool q_acts[5] = { q5_act, q3_act, q6_act, q4_act, q8_lift_act };
    lv_obj_t *q_btns[5] = {
      objects.btn2_3_1, objects.btn2_3_2, objects.btn2_3_3,
      objects.btn2_3_4, objects.btn2_3_5
    };
    for (int i = 0; i < 5; i++) {
      if (f3_pin_lbls[i]) {
        f3_set_badge_active_yellow(f3_pin_lbls[i], q_acts[i]);
      }
      if (q_btns[i] && lv_obj_is_valid(q_btns[i])) {
        if (q_acts[i] && !lv_obj_has_state(q_btns[i], LV_STATE_CHECKED)) {
          lv_obj_add_state(q_btns[i], LV_STATE_CHECKED);
        } else if (!q_acts[i] && lv_obj_has_state(q_btns[i], LV_STATE_CHECKED)) {
          lv_obj_clear_state(q_btns[i], LV_STATE_CHECKED);
        }
      }
    }
    // در کلید زاویه تیغه: مشخص شدن خروجی فعال با رنگ زرد (Q8 برای -۴۵° و Q9 برای ۹۰°)
    const bool q8_ang_act = (f3_blade_angle_mode == 1);
    const bool q9_ang_act = (f3_blade_angle_mode == 2);
    if (f3_pin_lbls[5]) {
      f3_set_badge_active_yellow(f3_pin_lbls[5], q8_ang_act);
    }
    if (f3_pin_q9_lbl) {
      f3_set_badge_active_yellow(f3_pin_q9_lbl, q9_ang_act);
    }
    if (objects.btn2_3_6 && lv_obj_is_valid(objects.btn2_3_6)) {
      bool angle_active = (f3_blade_angle_mode != 0);
      if (angle_active && !lv_obj_has_state(objects.btn2_3_6, LV_STATE_CHECKED)) {
        lv_obj_add_state(objects.btn2_3_6, LV_STATE_CHECKED);
      } else if (!angle_active && lv_obj_has_state(objects.btn2_3_6, LV_STATE_CHECKED)) {
        lv_obj_clear_state(objects.btn2_3_6, LV_STATE_CHECKED);
      }
    }
    if (f3_angle_val_lbl && lv_obj_is_valid(f3_angle_val_lbl)) {
      const char *cur_ang_str = q9_ang_act ? "90°" : (q8_ang_act ? "-45°" : "45°");
      lv_label_set_text(f3_angle_val_lbl, cur_ang_str);
    }
    // در کلیدهای استپر موتور: مشخص شدن خروجی‌های فعال (پالس در حین حرکت، جهت فقط در حرکت، فعال‌ساز بر اساس درایور)
    const bool step_act = f3_stepper_test_busy || f3_stepper_test_cmd || f3_stepper_test_recover_cmd || STP.motorIs_moving || STP.jogEnabled;
    const bool dir_act = step_act && (IOS.dir_pin >= 0 && pinmap_out_read(IOS.dir_pin) == HIGH);
    const bool enb_act = (IOS.enb_pin >= 0 && pinmap_out_read(IOS.enb_pin) == LOW);
    if (f3_q_step_badges[0]) f3_set_badge_active_yellow(f3_q_step_badges[0], step_act);
    if (f3_q_step_badges[1]) f3_set_badge_active_yellow(f3_q_step_badges[1], dir_act);
    if (f3_q_step_badges[2]) f3_set_badge_active_yellow(f3_q_step_badges[2], enb_act);
  }
  if (f3_pinmap_external_close_req) {
    f3_pinmap_external_close_req = false;
    if (f3_pinmap_dialog_is_open()) f3_pinmap_close();
  }
  const uint8_t reason = pinmap_notice_take();
  if (reason == PINMAP_NOTICE_NONE) return;
  switch (reason) {
    case PINMAP_NOTICE_I0:
      ui_toast("سنسور اول وصل نیست؛ موقعیت یابی خودکار انجام نمی شود",
               "First sensor disconnected: automatic homing is unavailable", 3800);
      break;
    case PINMAP_NOTICE_I1:
      ui_toast("سنسور آخر وصل نیست؛ حفاظت فیزیکی آخر خط فعال نیست",
               "Last sensor disconnected: physical end protection is unavailable", 3800);
      break;
    case PINMAP_NOTICE_Q0:
      ui_toast("موتور روشن نمی شود؛ پایه پالس استپر قطع است",
               "Motor blocked: stepper pulse is disconnected", 3800);
      break;
    case PINMAP_NOTICE_Q1:
      ui_toast("موتور روشن نمی شود؛ پایه جهت استپر قطع است",
               "Motor blocked: stepper direction is disconnected", 3800);
      break;
    case PINMAP_NOTICE_Q2:
      ui_toast("موتور روشن نمی شود؛ پایه فعال سازی استپر قطع است",
               "Motor blocked: stepper enable is disconnected", 3800);
      break;
    case PINMAP_NOTICE_I2:
      ui_toast("برش شروع نمی شود؛ سوییچ درب به پایه ای وصل نیست",
               "Cut blocked: door switch has no pin", 3800);
      break;
    case PINMAP_NOTICE_Q3:
      ui_toast("برش شروع نمی شود؛ جک نگهدارنده به پایه ای وصل نیست",
               "Cut blocked: holder jack has no pin", 3800);
      break;
    case PINMAP_NOTICE_Q4:
      ui_toast("برش شروع نمی شود؛ جک محافظ به پایه ای وصل نیست",
               "Cut blocked: guard jack has no pin", 3800);
      break;
    case PINMAP_NOTICE_Q6:
      ui_toast("برش شروع نمی شود؛ موتور اره به پایه ای وصل نیست",
               "Cut blocked: saw motor has no pin", 3800);
      break;
    case PINMAP_NOTICE_Q8:
      ui_toast("برش شروع نمی شود؛ جک تیغه به پایه ای وصل نیست",
               "Cut blocked: saw lift has no pin", 3800);
      break;
    default:
      ui_toast("اتصال موتور هنوز آماده نیست؛ کمی صبر کنید",
               "Motor connection is not ready yet", 2600);
      break;
  }
}

bool f3_pinmap_commit_on_save() {
  if (!pinmap_pending_has_changes()) return true;
  if (!f3_pinmap_machine_safe()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("برای ذخیره پایه ها، دستگاه باید کاملا متوقف باشد",
             "Stop the machine before saving pin changes", 3500);
    return false;
  }
  if (!pinmap_pending_commit()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("ذخیره پایه ها انجام نشد؛ دوباره تلاش کنید",
             "Pin changes were not saved; try again", 3500);
    return false;
  }
  f3_pin_wired = false;
  f3_layout_ver = 0;
  ui_toast("تغییرات پایه ها ذخیره شد",
           "Pin changes saved", 2200);
  return true;
}

static void f3_pinmap_close() {
  panel_five_nav_exit();
  f3_stepper_params_close();
  f3_pinmap_reset_confirm_close();
  if (f3_pinmap_root && lv_obj_is_valid(f3_pinmap_root)) lv_obj_delete(f3_pinmap_root);
  f3_pinmap_root = NULL;
  f3_pinmap_confirm_btn = NULL;
  f3_pinmap_cancel_btn = NULL;
  f3_pinmap_reset_btn = NULL;
  f3_pinmap_test_pulses_value = NULL;
  f3_pinmap_test_period_value = NULL;
  for (int i = 0; i < 11; ++i) f3_pinmap_choice_btn[i] = NULL;
  f3_pinmap_role = -1;
  f3_pinmap_pending_source = PINMAP_IN_NC;
  f3_pinmap_is_output = false;
  f3_pinmap_apply_busy = false;
  f3_pinmap_open_atomic = false;
  f3_pinmap_external_close_req = false;
  GUI.lock_screen = false;
  ui_resume_checkpoint();
}

static void f3_pinmap_test_values_sync() {
  char buf[8];
  if (f3_pinmap_test_pulses_value && lv_obj_is_valid(f3_pinmap_test_pulses_value)) {
    snprintf(buf, sizeof(buf), "%d", (int)f3_stepper_test_pulses);
    lv_label_set_text(f3_pinmap_test_pulses_value, buf);
  }
  if (f3_pinmap_test_period_value && lv_obj_is_valid(f3_pinmap_test_period_value)) {
    snprintf(buf, sizeof(buf), "%d", (int)f3_stepper_test_period_ms);
    lv_label_set_text(f3_pinmap_test_period_value, buf);
  }
}

static void f3_stepper_adjust(int code, int step) {
  const bool pulse = abs(code) == 1;
  int value = 0;
  if (pulse) value = (int)f3_stepper_test_pulses;
  else if (abs(code) == 2) value = (int)f3_stepper_test_period_ms;
  else return;
  const int next = constrain(value + (code > 0 ? step : -step), 10, 1000);
  if (pulse) f3_stepper_test_pulses = next;
  else f3_stepper_test_period_ms = next;
  settings_mark_dirty();
  f3_pinmap_test_values_sync();
  GUI.buzzer_cmd_short = true;
  const bool at_limit = code > 0 ? next >= 1000 : next <= 10;
  if (at_limit && !f3_stepper_boundary_notice) {
    if (pulse) {
      ui_toast(code > 0 ? "تعداد پالس به حداکثر ۱۰۰۰ رسید" : "تعداد پالس کمتر از ۱۰ نمی‌شود",
               code > 0 ? "Pulse count maximum: 1000" : "Pulse count minimum: 10", 1800);
    } else {
      ui_toast(code > 0 ? "فاصلۀ پالس به حداکثر ۱۰۰۰ رسید" : "فاصلۀ پالس کمتر از ۱۰ میلی‌ثانیه نمی‌شود",
               code > 0 ? "Pulse interval maximum: 1000 ms" : "Pulse interval minimum: 10 ms", 1800);
    }
    f3_stepper_boundary_notice = true;
  } else if (!at_limit) {
    f3_stepper_boundary_notice = false;
  }
}

static void f3_stepper_adjust_timer_cb(lv_timer_t *timer) {
  (void)timer;
  f3_stepper_adjust_repeat = true;
  f3_stepper_adjust(f3_stepper_adjust_code, 10);
  lv_timer_set_period(f3_stepper_adjust_timer, F3_STEPPER_ADJUST_REPEAT_MS);
}

static void f3_stepper_adjust_btn_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  const int adjust = (int)(intptr_t)lv_event_get_user_data(e);
  if (code == LV_EVENT_PRESSED) {
    f3_stepper_adjust_code = adjust;
    f3_stepper_adjust_repeat = false;
    f3_stepper_adjust_skip_click = false;
    f3_stepper_boundary_notice = false;
    if (f3_stepper_adjust_timer) lv_timer_delete(f3_stepper_adjust_timer);
    f3_stepper_adjust_timer = lv_timer_create(f3_stepper_adjust_timer_cb, F3_STEPPER_ADJUST_HOLD_MS, NULL);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (f3_stepper_adjust_timer) {
      lv_timer_delete(f3_stepper_adjust_timer);
      f3_stepper_adjust_timer = NULL;
    }
    f3_stepper_adjust_skip_click = f3_stepper_adjust_repeat;
    f3_stepper_boundary_notice = false;
  } else if (code == LV_EVENT_CLICKED) {
    if (f3_stepper_adjust_skip_click) {
      f3_stepper_adjust_skip_click = false;
      return;
    }
    f3_stepper_adjust(adjust, 1);
  }
}

static void f3_pinmap_add_test_control(lv_obj_t *parent, int x, const char *fa, const char *en,
                                        lv_obj_t **value_out, int minus_code, int plus_code, bool english) {
  lv_obj_t *minus = lv_button_create(parent);
  lv_obj_set_pos(minus, x, 3);
  lv_obj_set_size(minus, 48, 48);
  lv_obj_set_style_bg_color(minus, lv_color_hex(0x636366), 0);
  lv_obj_set_style_bg_color(minus, lv_color_hex(0x48484a), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(minus, 8, 0);
  lv_obj_add_event_cb(minus, f3_stepper_adjust_btn_cb, LV_EVENT_ALL, (void *)(intptr_t)minus_code);
  lv_obj_t *minus_label = lv_label_create(minus);
  lv_obj_set_style_text_font(minus_label, &lv_font_montserrat_20, 0);
  lv_label_set_text(minus_label, LV_SYMBOL_MINUS);
  lv_obj_center(minus_label);

  lv_obj_t *caption = lv_label_create(parent);
  lv_obj_set_pos(caption, x + 52, 2);
  lv_obj_set_size(caption, 124, 20);
  lv_obj_set_style_text_font(caption, english ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(caption, lv_color_hex(0xd8dee6), 0);
  lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(caption, english ? en : fa);

  *value_out = lv_label_create(parent);
  lv_obj_set_pos(*value_out, x + 52, 23);
  lv_obj_set_size(*value_out, 124, 24);
  lv_obj_set_style_text_font(*value_out, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(*value_out, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(*value_out, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *plus = lv_button_create(parent);
  lv_obj_set_pos(plus, x + 180, 3);
  lv_obj_set_size(plus, 48, 48);
  lv_obj_set_style_bg_color(plus, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_bg_color(plus, lv_color_hex(0x0066cc), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(plus, 8, 0);
  lv_obj_add_event_cb(plus, f3_stepper_adjust_btn_cb, LV_EVENT_ALL, (void *)(intptr_t)plus_code);
  lv_obj_t *plus_label = lv_label_create(plus);
  lv_obj_set_style_text_font(plus_label, &lv_font_montserrat_20, 0);
  lv_label_set_text(plus_label, LV_SYMBOL_PLUS);
  lv_obj_center(plus_label);
}

static void f3_stepper_params_add_control(lv_obj_t *parent, int y, const char *fa, const char *en,
                                          lv_obj_t **value_out, int minus_code, int plus_code, bool english) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_pos(card, 12, y);
  lv_obj_set_size(card, 456, 64);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x202a34), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x344250), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *minus = lv_button_create(card);
  lv_obj_set_pos(minus, 6, 6);
  lv_obj_set_size(minus, 52, 52);
  lv_obj_set_style_bg_color(minus, lv_color_hex(0x636366), 0);
  lv_obj_set_style_bg_color(minus, lv_color_hex(0x48484a), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(minus, 10, 0);
  ui_control_help_unbind(minus);
  lv_obj_add_flag(minus, LV_OBJ_FLAG_USER_2);
  lv_obj_add_event_cb(minus, f3_stepper_adjust_btn_cb, LV_EVENT_ALL, (void *)(intptr_t)minus_code);
  lv_obj_t *minus_label = lv_label_create(minus);
  lv_obj_set_style_text_font(minus_label, &lv_font_montserrat_20, 0);
  lv_label_set_text(minus_label, LV_SYMBOL_MINUS);
  lv_obj_set_style_translate_y(minus_label, -2, 0);
  lv_obj_center(minus_label);

  lv_obj_t *caption = lv_label_create(card);
  lv_obj_set_pos(caption, 66, 3);
  lv_obj_set_size(caption, 324, 26);
  lv_obj_set_style_text_font(caption, english ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(caption, lv_color_hex(0xd8dee6), 0);
  lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(caption, LV_LABEL_LONG_CLIP);
  if (!english) lv_obj_set_style_base_dir(caption, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(caption, english ? en : fa);

  *value_out = lv_label_create(card);
  lv_obj_set_pos(*value_out, 66, 28);
  lv_obj_set_size(*value_out, 324, 30);
  lv_obj_set_style_text_font(*value_out, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(*value_out, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(*value_out, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *plus = lv_button_create(card);
  lv_obj_set_pos(plus, 398, 6);
  lv_obj_set_size(plus, 52, 52);
  lv_obj_set_style_bg_color(plus, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_bg_color(plus, lv_color_hex(0x0066cc), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(plus, 10, 0);
  ui_control_help_unbind(plus);
  lv_obj_add_flag(plus, LV_OBJ_FLAG_USER_2);
  lv_obj_add_event_cb(plus, f3_stepper_adjust_btn_cb, LV_EVENT_ALL, (void *)(intptr_t)plus_code);
  lv_obj_t *plus_label = lv_label_create(plus);
  lv_obj_set_style_text_font(plus_label, &lv_font_montserrat_20, 0);
  lv_label_set_text(plus_label, LV_SYMBOL_PLUS);
  lv_obj_set_style_translate_y(plus_label, -2, 0);
  lv_obj_center(plus_label);
}

static int f3_pinmap_owner_of_source(uint8_t src, int except_role) {
  if (src >= PINMAP_IN_COUNT) return -1;
  for (int role = 0; role < PINMAP_IN_COUNT; ++role)
    if (role != except_role && pinmap_in_get_pending_source(role) == src) return role;
  return -1;
}

static int f3_pinmap_output_owner_of_source(uint8_t src, int except_role) {
  if (src > 9) return -1;
  for (int role = 0; role <= 9; ++role)
    if (role != except_role && pinmap_out_get_pending_source(role) == src) return role;
  return -1;
}

static void f3_pinmap_refresh_choices() {
  for (int src = 0; src <= 10; ++src) {
    lv_obj_t *btn = f3_pinmap_choice_btn[src];
    if (!btn || !lv_obj_is_valid(btn)) continue;
    const bool selected = (f3_pinmap_pending_source == (uint8_t)src);
    uint32_t color = 0x3a3a3c;
    if ((!f3_pinmap_is_output && src == PINMAP_IN_NC) ||
        (f3_pinmap_is_output && src == PINMAP_OUT_NC))
      color = 0xff3b30;
    else if ((!f3_pinmap_is_output &&
              f3_pinmap_owner_of_source((uint8_t)src, f3_pinmap_role) >= 0) ||
             (f3_pinmap_is_output &&
              f3_pinmap_output_owner_of_source((uint8_t)src, f3_pinmap_role) >= 0))
      color = 0x8a6218;
    if (selected) color = 0x0a84ff;
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_outline_width(btn, selected ? 3 : 0, 0);
    lv_obj_set_style_outline_color(btn, lv_color_hex(0x8fd3ff), 0);
  }
}

static void f3_pinmap_item_help_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) return;
  const int id = (int)(intptr_t)lv_event_get_user_data(e);
  ui_help_suppress_release = true;
  if (id == -1) {
    main_ctrl_help_show("انصراف: این پنجره را بدون تغییر اتصال می‌بندد.",
                        "Cancel: closes this window without changing the connection.");
  } else if (id == -2) {
    main_ctrl_help_show(f3_pinmap_is_output
      ? "حالت اول خروجی‌ها: فقط چینش پایه‌های خروجی را موقتاً به حالت استاندارد برمی‌گرداند."
      : "حالت اول ورودی‌ها: فقط چینش پایه‌های ورودی را موقتاً به حالت استاندارد برمی‌گرداند.",
      f3_pinmap_is_output
      ? "Output defaults: temporarily restores only output pin assignments."
      : "Input defaults: temporarily restores only input pin assignments.");
  } else if (id == -3) {
    main_ctrl_help_show("تأیید: انتخاب این پنجره را موقت اعمال می‌کند؛ ثبت دائمی فقط با ذخیره بالای F3 است.",
                        "Confirm: applies this choice temporarily; top Save stores it permanently.");
  } else if (id >= 0) {
    const bool nc = (!f3_pinmap_is_output && id == PINMAP_IN_NC) ||
                    (f3_pinmap_is_output && id == PINMAP_OUT_NC);
    const int owner = nc ? -1 :
      (f3_pinmap_is_output ? f3_pinmap_output_owner_of_source((uint8_t)id, f3_pinmap_role)
                           : f3_pinmap_owner_of_source((uint8_t)id, f3_pinmap_role));
    if (nc)
      main_ctrl_help_show("قطع اتصال: این نقش دیگر به هیچ پایه‌ای وصل نخواهد بود.",
                          "Disconnect: this role will no longer be connected to a pin.");
    else if (owner >= 0)
      main_ctrl_help_show("این شماره در حال استفاده است؛ با تأیید، جای دو اتصال با هم عوض می‌شود.",
                          "This number is occupied; confirming swaps the two connections.");
    else
      main_ctrl_help_show("با تأیید، اتصال فعلی به این شماره منتقل می‌شود.",
                          "Confirming moves the current connection to this number.");
  }
}

static void f3_pinmap_choice_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (ui_help_take_suppress()) return;
  if (f3_pinmap_apply_busy) return;
  const int src = (int)(intptr_t)lv_event_get_user_data(e);
  if (src < 0 || src > 10) return;
  f3_pinmap_pending_source = (uint8_t)src;
  f3_pinmap_refresh_choices();
  GUI.buzzer_cmd_short = true;
}

static void f3_pinmap_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) { f3_pinmap_close(); }, NULL);
}

static void f3_pinmap_confirm_async(void *) {
  if (!f3_pinmap_dialog_is_open() || f3_pinmap_apply_busy) return;
  f3_pinmap_apply_busy = true;
  if (f3_pinmap_role < 0) {
    f3_pinmap_apply_busy = false;
    GUI.buzzer_cmd_long = true;
    ui_toast("انتخاب پایه معتبر نیست", "Invalid pin selection", 2200);
    return;
  }
  const uint8_t previous_source = f3_pinmap_is_output
    ? pinmap_out_get_pending_source(f3_pinmap_role)
    : pinmap_in_get_pending_source(f3_pinmap_role);
  const bool selection_changed = previous_source != f3_pinmap_pending_source;
  const bool staged = f3_pinmap_is_output
    ? pinmap_out_stage(f3_pinmap_role, f3_pinmap_pending_source)
    : pinmap_in_stage(f3_pinmap_role, f3_pinmap_pending_source);
  if (!staged) {
    f3_pinmap_apply_busy = false;
    GUI.buzzer_cmd_long = true;
    ui_toast("تغییر پایه انجام نشد", "Pin change failed", 2400);
    return;
  }
  if (f3_pinmap_is_output) {
    lv_obj_t *qbtns[] = {
      objects.btn2_3_1, objects.btn2_3_2, objects.btn2_3_3,
      objects.btn2_3_4, objects.btn2_3_5, objects.btn2_3_6
    };
    for (unsigned i = 0; i < sizeof(qbtns) / sizeof(qbtns[0]); ++i)
      if (qbtns[i] && lv_obj_is_valid(qbtns[i])) lv_obj_clear_state(qbtns[i], LV_STATE_CHECKED);
  }
  GUI.buzzer_cmd_short = true;
  f3_pinmap_close();
  f3_pinmap_reflow();
  if (selection_changed) {
    settings_mark_dirty();
    ui_toast("تغییر موقت انجام شد؛ برای ثبت، ذخیره بالا را بزنید",
             "Temporary change; use the top Save button", 3000);
  } else {
    ui_toast("اتصال این پایه تغییری نکرد", "This pin connection did not change", 1800);
  }
}

static void f3_pinmap_confirm_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (ui_help_take_suppress()) return;
  lv_async_call(f3_pinmap_confirm_async, NULL);
}

static void f3_pinmap_reset_async(void *) {
  if (!f3_pinmap_dialog_is_open() || f3_pinmap_apply_busy) return;
  f3_pinmap_apply_busy = true;
  const bool was_output = f3_pinmap_is_output;
  bool group_changed = false;
  if (was_output) {
    for (int i = 0; i < 10; ++i)
      if (pinmap_out_get_pending_source(i) != (uint8_t)i) { group_changed = true; break; }
  } else {
    for (int i = 0; i < PINMAP_IN_COUNT; ++i)
      if (pinmap_in_get_pending_source(i) != (uint8_t)i) { group_changed = true; break; }
  }
  if (was_output) pinmap_out_stage_defaults();
  else pinmap_in_stage_defaults();
  GUI.buzzer_cmd_short = true;
  f3_pinmap_close();
  f3_pinmap_reflow();
  if (group_changed) {
    settings_mark_dirty();
    ui_toast(was_output ? "خروجی ها موقت به حالت اول برگشتند؛ ذخیره را بزنید"
                        : "ورودی ها موقت به حالت اول برگشتند؛ ذخیره را بزنید",
             was_output ? "Outputs temporarily reset; press Save"
                        : "Inputs temporarily reset; press Save",
             3000);
  } else {
    ui_toast(was_output ? "خروجی ها از قبل در حالت اول بودند"
                        : "ورودی ها از قبل در حالت اول بودند",
             was_output ? "Outputs are already at defaults"
                        : "Inputs are already at defaults",
             1800);
  }
}

static void f3_pinmap_reset_confirm_close(void) {
  if (f3_pinmap_reset_confirm_root && lv_obj_is_valid(f3_pinmap_reset_confirm_root))
    lv_obj_delete(f3_pinmap_reset_confirm_root);
  f3_pinmap_reset_confirm_root = NULL;
  f3_pinmap_reset_confirm_bar = NULL;
  f3_pinmap_reset_confirm_started_ms = 0;
}

static void f3_pinmap_reset_confirm_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) { f3_pinmap_reset_confirm_close(); }, NULL);
}

static void f3_pinmap_reset_confirm_apply_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (!f3_pinmap_dialog_is_open() || f3_pinmap_is_output != f3_pinmap_reset_confirm_output) return;
  if (!f3_pinmap_machine_safe()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("اول همه حرکت‌ها و خروجی‌ها را متوقف کنید",
             "Stop all motion and outputs first", 3000);
    lv_async_call([](void *) { f3_pinmap_reset_confirm_close(); }, NULL);
    return;
  }
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) {
    f3_pinmap_reset_confirm_close();
    f3_pinmap_reset_async(NULL);
  }, NULL);
}

static void f3_pinmap_reset_confirm_show() {
  if (!f3_pinmap_dialog_is_open() || f3_pinmap_reset_confirm_root) return;
  const bool en = (GUI.system_language == 2);
  f3_pinmap_reset_confirm_output = f3_pinmap_is_output;
  f3_pinmap_reset_confirm_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f3_pinmap_reset_confirm_root, 480, 320);
  lv_obj_set_pos(f3_pinmap_reset_confirm_root, 0, 0);
  lv_obj_set_style_bg_color(f3_pinmap_reset_confirm_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f3_pinmap_reset_confirm_root, LV_OPA_60, 0);
  lv_obj_set_style_border_width(f3_pinmap_reset_confirm_root, 0, 0);
  lv_obj_set_style_pad_all(f3_pinmap_reset_confirm_root, 0, 0);
  lv_obj_clear_flag(f3_pinmap_reset_confirm_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f3_pinmap_reset_confirm_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(f3_pinmap_reset_confirm_root);
  lv_obj_set_size(card, 312, 164);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_width(body, 280);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
  if (!en) lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(body, en
    ? (f3_pinmap_reset_confirm_output ? "Reset output pins to defaults?" : "Reset input pins to defaults?")
    : (f3_pinmap_reset_confirm_output ? "بازگردانی حالت اول خروجی‌ها؟" : "بازگردانی حالت اول ورودی‌ها؟"));
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *track = lv_obj_create(card);
  lv_obj_set_size(track, 280, 10);
  lv_obj_align(track, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_style_bg_color(track, lv_color_hex(0xe5e5ea), 0);
  lv_obj_set_style_radius(track, 5, 0);
  lv_obj_set_style_border_width(track, 0, 0);
  lv_obj_set_style_pad_all(track, 0, 0);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);
  f3_pinmap_reset_confirm_bar = lv_obj_create(track);
  lv_obj_set_size(f3_pinmap_reset_confirm_bar, 0, 10);
  lv_obj_set_style_bg_color(f3_pinmap_reset_confirm_bar, lv_color_hex(0xff9f0a), 0);
  lv_obj_set_style_radius(f3_pinmap_reset_confirm_bar, 5, 0);
  lv_obj_set_style_border_width(f3_pinmap_reset_confirm_bar, 0, 0);
  lv_obj_clear_flag(f3_pinmap_reset_confirm_bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(f3_pinmap_reset_confirm_bar, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *cancel = lv_button_create(card);
  lv_obj_set_size(cancel, 124, 42);
  lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 8, -4);
  lv_obj_set_style_bg_color(cancel, lv_color_hex(0x636366), 0);
  lv_obj_set_style_radius(cancel, 10, 0);
  lv_obj_add_event_cb(cancel, f3_pinmap_reset_confirm_cancel_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *cancel_label = lv_label_create(cancel);
  lv_obj_set_style_text_font(cancel_label, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_label_set_text(cancel_label, en ? "Cancel" : "انصراف");
  lv_obj_center(cancel_label);

  lv_obj_t *confirm = lv_button_create(card);
  lv_obj_set_size(confirm, 124, 42);
  lv_obj_align(confirm, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
  lv_obj_set_style_bg_color(confirm, lv_color_hex(0xff9f0a), 0);
  lv_obj_set_style_radius(confirm, 10, 0);
  lv_obj_add_event_cb(confirm, f3_pinmap_reset_confirm_apply_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *confirm_label = lv_label_create(confirm);
  lv_obj_set_style_text_font(confirm_label, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_label_set_text(confirm_label, en ? "Reset" : "بازگردانی");
  lv_obj_center(confirm_label);

  f3_pinmap_reset_confirm_started_ms = millis();
}

static void f3_pinmap_reset_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (ui_help_take_suppress()) return;
  f3_pinmap_reset_confirm_show();
}

static void f3_pinmap_open(bool is_output, int role) {
  if ((!is_output && (role < 0 || role >= PINMAP_IN_COUNT)) ||
      (is_output && (role < 0 || role > 9)))
    return;
  // نقش‌های T1/T2 فقط وقتی جویستیک F4 روشن است قابل تنظیم‌اند؛ نگاشت ذخیره‌شده دست‌نخورده می‌ماند.
  if (!is_output && role >= 8 && !GUI.joystick_enable) return;
  if (!f3_pinmap_machine_safe()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("اول همه حرکت ها و خروجی ها را متوقف کنید",
             "Stop all motion and outputs first", 3000);
    return;
  }
  f3_pin_help_close();
  f3_pinmap_close();
  f3_pinmap_is_output = is_output;
  f3_pinmap_role = role;
  f3_pinmap_pending_source = is_output
    ? pinmap_out_get_pending_source(role)
    : pinmap_in_get_pending_source(role);
  const bool en = (GUI.system_language == 2);

  f3_pinmap_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f3_pinmap_root, 480, 320);
  lv_obj_set_pos(f3_pinmap_root, 0, 0);
  lv_obj_set_style_bg_color(f3_pinmap_root, lv_color_hex(0x111820), 0);
  lv_obj_set_style_bg_opa(f3_pinmap_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f3_pinmap_root, 0, 0);
  lv_obj_set_style_pad_all(f3_pinmap_root, 0, 0);
  lv_obj_clear_flag(f3_pinmap_root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(f3_pinmap_root);
  lv_obj_set_size(title, 382, 30);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_20 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  static const int input_help_id[PINMAP_IN_COUNT] = {
    F3H_I0, F3H_I1, F3H_I2, F3H_I3, F3H_I4,
    F3H_I5, F3H_I6, F3H_I7, F3H_T1, F3H_T2
  };
  static const int output_help_id[10] = {
    F3H_Q0, F3H_Q1, F3H_Q2, F3H_Q3, F3H_Q4, F3H_Q5, F3H_Q6, F3H_Q7, F3H_Q8, F3H_Q7
  };
  const int help_id = is_output ? (role == 8 || role == 9 ? F3H_Q7 : output_help_id[role]) : input_help_id[role];
  lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
  if (is_output && (role == 8 || role == 9)) {
    if (role == 8) {
      lv_label_set_text(title, en ? "Angle (Q8: 45°)" : "زاویه تیغه (Q8: ۴۵ درجه)");
    } else {
      lv_label_set_text(title, en ? "Angle (Q9: 90°)" : "زاویه تیغه (Q9: ۹۰ درجه)");
    }
    lv_obj_set_pos(title, 136, 5);
    lv_obj_set_size(title, 332, 30);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  } else {
    lv_label_set_text(title, en ? f3_pin_help_title_en(help_id)
                                : f3_pin_help_title_fa(help_id));
    lv_obj_set_pos(title, 49, 5);
    lv_obj_set_size(title, 382, 30);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  }

  // کد اتصال جدا از متن فارسی ساخته می‌شود تا هیچ نویسه‌ای مربع نشود.
  if (is_output && (role == 8 || role == 9)) {
    // دو کلید واضح Q8 و Q9 در کنار هم در سمت چپ بالای صفحه با تفکیک روشن/خاموش
    lv_obj_t *badge8 = lv_button_create(f3_pinmap_root);
    lv_obj_set_pos(badge8, 12, 4);
    lv_obj_set_size(badge8, 56, 28);
    lv_obj_set_style_radius(badge8, 8, 0);
    lv_obj_set_style_pad_all(badge8, 0, 0);
    if (role == 8) {
      lv_obj_set_style_bg_color(badge8, lv_color_hex(0x00A854), 0); // روشن / فعال
      lv_obj_set_style_border_color(badge8, lv_color_hex(0xffffff), 0);
      lv_obj_set_style_border_width(badge8, 2, 0);
    } else {
      lv_obj_set_style_bg_color(badge8, lv_color_hex(0x2c3e50), 0); // خاموش / غیرفعال
      lv_obj_set_style_border_color(badge8, lv_color_hex(0x5a6b7c), 0);
      lv_obj_set_style_border_width(badge8, 1, 0);
    }
    lv_obj_t *l8 = lv_label_create(badge8);
    lv_obj_set_style_text_font(l8, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l8, lv_color_hex(0xffffff), 0);
    lv_label_set_text(l8, "Q8");
    lv_obj_center(l8);
    lv_obj_add_event_cb(badge8, [](lv_event_t *) {
      if (f3_pinmap_role != 8) {
        f3_pinmap_open(true, 8);
        if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
      }
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *badge9 = lv_button_create(f3_pinmap_root);
    lv_obj_set_pos(badge9, 74, 4);
    lv_obj_set_size(badge9, 56, 28);
    lv_obj_set_style_radius(badge9, 8, 0);
    lv_obj_set_style_pad_all(badge9, 0, 0);
    if (role == 9) {
      lv_obj_set_style_bg_color(badge9, lv_color_hex(0x00A854), 0); // روشن / فعال
      lv_obj_set_style_border_color(badge9, lv_color_hex(0xffffff), 0);
      lv_obj_set_style_border_width(badge9, 2, 0);
    } else {
      lv_obj_set_style_bg_color(badge9, lv_color_hex(0x2c3e50), 0); // خاموش / غیرفعال
      lv_obj_set_style_border_color(badge9, lv_color_hex(0x5a6b7c), 0);
      lv_obj_set_style_border_width(badge9, 1, 0);
    }
    lv_obj_t *l9 = lv_label_create(badge9);
    lv_obj_set_style_text_font(l9, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l9, lv_color_hex(0xffffff), 0);
    lv_label_set_text(l9, "Q9");
    lv_obj_center(l9);
    lv_obj_add_event_cb(badge9, [](lv_event_t *) {
      if (f3_pinmap_role != 9) {
        f3_pinmap_open(true, 9);
        if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
      }
    }, LV_EVENT_CLICKED, NULL);
  } else {
    char header_tag[8];
    if (is_output) {
      if (f3_pinmap_pending_source < PINMAP_OUT_NC)
        snprintf(header_tag, sizeof(header_tag), "Q%u", (unsigned)f3_pinmap_pending_source);
      else
        snprintf(header_tag, sizeof(header_tag), "X");
    } else {
      f3_input_source_tag(f3_pinmap_pending_source, header_tag, sizeof(header_tag));
    }
    lv_obj_t *header_badge = f3_make_pin_lbl(f3_pinmap_root, header_tag, 30, is_output);
    if (header_badge) lv_obj_set_pos(header_badge, 28, 4);
  }

  lv_obj_t *hint = lv_label_create(f3_pinmap_root);
  lv_obj_set_width(hint, 448);
  lv_obj_set_style_text_font(hint, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex((!is_output && role == 3) ? 0xff9f0a : 0xaeb8c2), 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(hint, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(hint, (!is_output && role == 3)
    ? (en ? "Choose another pin; disconnecting Stop keeps the machine locked."
          : "یک شماره دیگر انتخاب کنید؛ قطع کلید استپ دستگاه را قفل می‌کند.")
    : (en ? "Choose a number below; an occupied number swaps the two connections."
          : "شماره دلخواه را انتخاب کنید؛ شماره استفاده‌شده جای دو اتصال را عوض می‌کند."));
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 38);

  const bool show_stepper_test_controls = false;
  if (show_stepper_test_controls) {
    lv_obj_t *test_row = lv_obj_create(f3_pinmap_root);
    lv_obj_set_pos(test_row, 12, 60);
    lv_obj_set_size(test_row, 456, 54);
    lv_obj_set_style_bg_color(test_row, lv_color_hex(0x202a34), 0);
    lv_obj_set_style_bg_opa(test_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(test_row, 0, 0);
    lv_obj_set_style_radius(test_row, 10, 0);
    lv_obj_set_style_pad_all(test_row, 0, 0);
    lv_obj_clear_flag(test_row, LV_OBJ_FLAG_SCROLLABLE);
    f3_pinmap_add_test_control(test_row, 0, "تعداد پالس", "Pulse count",
                               &f3_pinmap_test_pulses_value, -1, 1, en);
    f3_pinmap_add_test_control(test_row, 228, "فاصله پالس", "Pulse interval (ms)",
                               &f3_pinmap_test_period_value, -2, 2, en);
    f3_pinmap_test_values_sync();
  }

  lv_obj_t *list = lv_obj_create(f3_pinmap_root);
  lv_obj_set_pos(list, 12, show_stepper_test_controls ? 120 : 70);
  lv_obj_set_size(list, 456, show_stepper_test_controls ? 134 : 184);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(list, 4, 0);
  lv_obj_set_style_pad_row(list, 6, 0);
  lv_obj_set_style_bg_color(list, lv_color_hex(0x202a34), 0);
  lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_radius(list, 10, 0);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);

  const int src_first = 0;
  const int src_last = 10;
  for (int src = src_first; src <= src_last; ++src) {
    // با جویستیک خاموش، T1/T2 نه نقش قابل ویرایش‌اند و نه ترمینال قابل انتخاب.
    if (!is_output && !GUI.joystick_enable && (src == 8 || src == 9)) {
      f3_pinmap_choice_btn[src] = NULL;
      continue;
    }
    const bool is_nc = (!is_output && src == PINMAP_IN_NC) ||
                       (is_output && src == PINMAP_OUT_NC);
    // اتصال فعلی انتخاب تازه نیست. وقتی نقش از قبل قطع است نیز گزینۀ
    // «قطع اتصال» دوباره نشان داده نمی‌شود؛ پایه‌های بدون مالک خودکار آزادند.
    if ((uint8_t)src == f3_pinmap_pending_source || (is_nc && f3_pinmap_pending_source == (uint8_t)src)) {
      f3_pinmap_choice_btn[src] = NULL;
      continue;
    }
    if (!is_output && !GUI.joystick_enable &&
        f3_pinmap_owner_of_source((uint8_t)src, f3_pinmap_role) >= 8) {
      f3_pinmap_choice_btn[src] = NULL;
      continue;
    }
    lv_obj_t *btn = lv_button_create(list);
    f3_pinmap_choice_btn[src] = btn;
    lv_obj_set_size(btn, 436, 54);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    // بج فارسی در x=-10 قرار می‌گیرد؛ برای نمایش کامل آن برش فرزند را بردار.
    lv_obj_add_flag(btn, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_event_cb(btn, f3_pinmap_choice_cb, LV_EVENT_CLICKED, (void *)(intptr_t)src);
    lv_obj_add_event_cb(btn, f3_pinmap_item_help_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)src);
    lv_obj_add_flag(btn, (lv_obj_flag_t)(LV_OBJ_FLAG_USER_1 | LV_OBJ_FLAG_USER_2));
    const int owner = is_nc ? -1 :
      (is_output ? f3_pinmap_output_owner_of_source((uint8_t)src, role)
                 : f3_pinmap_owner_of_source((uint8_t)src, role));
    char source_tag[8];
    if (is_nc)
      snprintf(source_tag, sizeof(source_tag), "X");
    else if (is_output)
      snprintf(source_tag, sizeof(source_tag), "Q%d", src);
    else
      f3_input_source_tag((uint8_t)src, source_tag, sizeof(source_tag));
    lv_obj_t *source_badge = f3_make_pin_lbl(btn, source_tag, 34, is_output && !is_nc);
    // فارسی: بج I/Q بیست پیکسل چپ‌تر از قبل؛ انگلیسی: ستون کد در سمت راست.
    if (source_badge)
      lv_obj_align(source_badge, en ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, -10, 0);

    lv_obj_t *desc_lbl = lv_label_create(btn);
    lv_obj_clear_flag(desc_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(desc_lbl, 366);
    lv_obj_set_style_text_font(desc_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(desc_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(desc_lbl, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_base_dir(desc_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
    lv_label_set_text(desc_lbl, is_nc
      ? (en ? "Disconnect this connection"
            : "قطع اتصال؛ این نقش دیگر به پایه‌ای وصل نیست")
      : (owner >= 0
          ? (en ? "In use; confirm to swap the two connections"
                : "در حال استفاده است؛ با تأیید جای دو اتصال عوض می‌شود")
          : (en ? "Available; move this connection here"
                : "آزاد است؛ اتصال فعلی به این شماره منتقل می‌شود")));
    // ستون متن انگلیسی پیش از بج راست تمام می‌شود؛ در فارسی جای بج چپ محفوظ می‌ماند.
    if (en) lv_obj_align(desc_lbl, LV_ALIGN_LEFT_MID, 8, 0);
    else lv_obj_align(desc_lbl, LV_ALIGN_RIGHT_MID, -8, 0);
  }

  f3_pinmap_cancel_btn = lv_button_create(f3_pinmap_root);
  lv_obj_set_pos(f3_pinmap_cancel_btn, 12, 266);
  lv_obj_set_size(f3_pinmap_cancel_btn, 142, 46);
  lv_obj_set_style_bg_color(f3_pinmap_cancel_btn, lv_color_hex(0x636366), 0);
  lv_obj_set_style_radius(f3_pinmap_cancel_btn, 10, 0);
  lv_obj_add_event_cb(f3_pinmap_cancel_btn, f3_pinmap_cancel_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(f3_pinmap_cancel_btn, f3_pinmap_item_help_cb, LV_EVENT_LONG_PRESSED,
                      (void *)(intptr_t)-1);
  lv_obj_add_flag(f3_pinmap_cancel_btn, (lv_obj_flag_t)(LV_OBJ_FLAG_USER_1 | LV_OBJ_FLAG_USER_2));
  lv_obj_t *cl = lv_label_create(f3_pinmap_cancel_btn);
  lv_obj_set_style_text_font(cl, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_label_set_text(cl, en ? "Cancel" : "انصراف");
  lv_obj_center(cl);

  f3_pinmap_reset_btn = lv_button_create(f3_pinmap_root);
  lv_obj_set_pos(f3_pinmap_reset_btn, 169, 266);
  lv_obj_set_size(f3_pinmap_reset_btn, 142, 46);
  lv_obj_set_style_bg_color(f3_pinmap_reset_btn, lv_color_hex(0x8a6218), 0);
  lv_obj_set_style_radius(f3_pinmap_reset_btn, 10, 0);
  lv_obj_add_event_cb(f3_pinmap_reset_btn, f3_pinmap_reset_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(f3_pinmap_reset_btn, f3_pinmap_item_help_cb, LV_EVENT_LONG_PRESSED,
                      (void *)(intptr_t)-2);
  lv_obj_add_flag(f3_pinmap_reset_btn, (lv_obj_flag_t)(LV_OBJ_FLAG_USER_1 | LV_OBJ_FLAG_USER_2));
  lv_obj_t *rst = lv_label_create(f3_pinmap_reset_btn);
  lv_obj_set_style_text_font(rst, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(rst, lv_color_hex(0xffffff), 0);
  lv_label_set_text(rst, en
    ? (is_output ? "Reset outputs" : "Reset inputs")
    : (is_output ? "حالت اول خروجی ها" : "حالت اول ورودی ها"));
  lv_obj_center(rst);

  f3_pinmap_confirm_btn = lv_button_create(f3_pinmap_root);
  lv_obj_set_pos(f3_pinmap_confirm_btn, 326, 266);
  lv_obj_set_size(f3_pinmap_confirm_btn, 142, 46);
  lv_obj_set_style_bg_color(f3_pinmap_confirm_btn, lv_color_hex(0x34c759), 0);
  lv_obj_set_style_radius(f3_pinmap_confirm_btn, 10, 0);
  lv_obj_add_event_cb(f3_pinmap_confirm_btn, f3_pinmap_confirm_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(f3_pinmap_confirm_btn, f3_pinmap_item_help_cb, LV_EVENT_LONG_PRESSED,
                      (void *)(intptr_t)-3);
  lv_obj_add_flag(f3_pinmap_confirm_btn, (lv_obj_flag_t)(LV_OBJ_FLAG_USER_1 | LV_OBJ_FLAG_USER_2));
  lv_obj_t *ok = lv_label_create(f3_pinmap_confirm_btn);
  lv_obj_set_style_text_font(ok, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_label_set_text(ok, en ? "Confirm" : "تایید");
  lv_obj_center(ok);

  f3_pinmap_refresh_choices();
  f3_pinmap_open_atomic = true;
  GUI.lock_screen = true;
  GUI.buzzer_cmd_short = true;
  ui_resume_checkpoint();
}

bool f3_pinmap_panel_adjust(lv_obj_t *obj, int dir) {
  if (!f3_pinmap_dialog_is_open()) {
    if (dir >= 0 || GUI.scr_id != 2 || !objects.tabview2_1 ||
        (int)lv_tabview_get_tab_act(objects.tabview2_1) != 2)
      return false;
    const int id = f3_pin_help_id_from_obj(obj);
    const int in_role = f3_pinmap_input_role_from_help(id);
    const int out_role = f3_pinmap_output_role_from_help(id);
    if (in_role >= 0) {
      f3_pinmap_open(false, in_role);
      return true;
    }
    if (out_role >= 0) {
      f3_pinmap_open(true, out_role);
      return true;
    }
    return false;
  }
  if (dir < 0) {
    f3_pinmap_close();
    GUI.buzzer_cmd_short = true;
    return true;
  }
  if (obj && lv_obj_is_valid(obj)) lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL);
  return true;
}

bool f3_pinmap_panel_confirm() {
  if (!f3_pinmap_dialog_is_open() || !f3_pinmap_confirm_btn ||
      !lv_obj_is_valid(f3_pinmap_confirm_btn))
    return false;
  lv_obj_send_event(f3_pinmap_confirm_btn, LV_EVENT_CLICKED, NULL);
  return true;
}

bool f3_pinmap_panel_cancel() {
  if (!f3_pinmap_dialog_is_open()) return false;
  f3_pinmap_close();
  GUI.buzzer_cmd_short = true;
  return true;
}

static void f3_pinmap_open_from_help_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const int role = (int)(intptr_t)lv_event_get_user_data(e);
  lv_async_call([](void *p) { f3_pinmap_open(false, (int)(intptr_t)p); },
                (void *)(intptr_t)role);
}

static void f3_pinmap_open_output_from_help_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const int role = (int)(intptr_t)lv_event_get_user_data(e);
  lv_async_call([](void *p) { f3_pinmap_open(true, (int)(intptr_t)p); },
                (void *)(intptr_t)role);
}

static void f3_pin_help_disarm_q() {
  f3_help_q_armed = false;
  f3_help_q_btn = NULL;
}

static bool f3_q_suppress_active() {
  if (f3_help_q_armed) return true;
  if (!f3_q_suppress_until_ms) return false;
  return (int32_t)(millis() - f3_q_suppress_until_ms) < 0;
}

static void f3_pin_help_close() {
  f3_hold_cancel();
  if (f3_help_root && lv_obj_is_valid(f3_help_root)) {
    lv_obj_delete(f3_help_root);
  }
  f3_help_root = NULL;
  f3_help_active_id = 0;
  f3_help_opened_ms = 0;
  f3_help_close_armed = false;
  f3_help_wait_finger_up = false;
  f3_pin_help_disarm_q();  // مهم: وگرنه بعد از PRESS_LOST همۀ Qها تا ریست مرده‌اند
  // suppress زمانی را نگه دار تا رها شدن همان long-press تاگل نزند
  ui_resume_checkpoint();
}

static void f3_pin_help_close_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  // همیشه فلگ Q را خنثی کن (حتی اگر هنوز کادر باز بماند)
  f3_pin_help_disarm_q();

  // overlay زیر همان انگشت باز می‌شود → PRESSED/RELEASED/CLICKED همان ژست نباید ببندد
  if (f3_help_wait_finger_up) {
    if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST || c == LV_EVENT_CLICKED) {
      f3_help_wait_finger_up = false;
    }
    return;
  }

  if (c == LV_EVENT_PRESSED) {
    // لمس عمدی بعدی روی بک‌دراپ → اجازۀ بستن با رها کردن
    if (f3_help_opened_ms && (uint32_t)(millis() - f3_help_opened_ms) > 200U) {
      f3_help_close_armed = true;
    }
    return;
  }
  if (c != LV_EVENT_CLICKED && c != LV_EVENT_RELEASED) return;
  if (!f3_help_close_armed) return;
  f3_help_close_armed = false;
  // حذف هم‌زمان ریشه در callback لمس باعث ارجاع نامعتبر و هنگ دوره‌ای می‌شد.
  lv_async_call([](void *) { f3_pin_help_close(); }, NULL);
  GUI.buzzer_cmd_short = true;
}

static void f3_help_card_eat_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) lv_event_stop_bubbling(e);
}

static void f3_pin_help_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) { f3_pin_help_close(); }, NULL);
}

static const char *f3_pin_help_code(int id) {
  switch (id) {
    case F3H_I2: return "I2";
    case F3H_I0: return "I0";
    case F3H_I1: return "I1";
    case F3H_I3: return "I3";
    case F3H_I6: return "I6";
    case F3H_I7: return "I7";
    case F3H_I4: return "I4";
    case F3H_I5: return "I5";
    case F3H_T1: return "T1";
    case F3H_T2: return "T2";
    case F3H_Q5: return "Q5";
    case F3H_Q3: return "Q3";
    case F3H_Q6: return "Q6";
    case F3H_Q4: return "Q4";
    case F3H_Q8: return "Q8";
    case F3H_Q7: return "Q7";
    case F3H_Q0: return "Q0";
    case F3H_Q1: return "Q1";
    case F3H_Q2: return "Q2";
    default: return "";
  }
}

static const char *f3_pin_help_title_fa(int id) {
  switch (id) {
    case F3H_I2: return "سوییچ درب محافظ";
    case F3H_I0: return "سنسور اول خط";
    case F3H_I1: return "سنسور آخر خط";
    case F3H_I3: return "کلید استپ";
    case F3H_I6: return "کلید استارت یک";
    case F3H_I7: return "کلید استارت دو";
    case F3H_I4: return "سنسور برگشت تیغه";
    case F3H_I5: return "خطای موتور";
    case F3H_T1: return "جویستیک راست";
    case F3H_T2: return "جویستیک چپ";
    case F3H_Q5: return "اسپری";
    case F3H_Q3: return "جک نگهدارنده";
    case F3H_Q6: return "موتور اره";
    case F3H_Q4: return "جک محافظ";
    case F3H_Q8: return "جک تیغه";
    case F3H_Q7: return "زاویه ۹۰°";
    case F3H_Q0: return "پالس درایو استپر";
    case F3H_Q1: return "جهت درایو استپر";
    case F3H_Q2: return "فعال سازی درایو استپر";
    default: return "راهنمای پایه";
  }
}

static const char *f3_pin_help_title_en(int id) {
  switch (id) {
    case F3H_I2: return "Door Guard";
    case F3H_I0: return "First Sensor";
    case F3H_I1: return "Last Sensor";
    case F3H_I3: return "Stop";
    case F3H_I6: return "Start 1";
    case F3H_I7: return "Start 2";
    case F3H_I4: return "Blade Return";
    case F3H_I5: return "Motor Fault";
    case F3H_T1: return "Jog Right";
    case F3H_T2: return "Jog Left";
    case F3H_Q5: return "Spray";
    case F3H_Q3: return "Holder Jack";
    case F3H_Q6: return "Saw";
    case F3H_Q4: return "Guard Jack";
    case F3H_Q8: return "Saw Lift";
    case F3H_Q7: return "Angle 90°";
    case F3H_Q0: return "Stepper Pulse";
    case F3H_Q1: return "Stepper Direction";
    case F3H_Q2: return "Stepper Enable";
    default: return "Pin help";
  }
}

static const char *f3_pin_help_body_fa(int id) {
  switch (id) {
    case F3H_I2:
      return "وضعیت بسته بودن درب محافظ را می‌خواند. چراغ سبز یعنی درب بسته و آماده است؛ بدون این ورودی، برش شروع نمی‌شود.";
    case F3H_I0:
      return "ابتدای مسیر محور را مشخص می‌کند. بدون آن موقعیت‌یابی خودکار انجام نمی‌شود و حرکت فقط با حدود تنظیم‌شده است.";
    case F3H_I1:
      return "انتهای مسیر محور را مشخص می‌کند. بدون آن حفاظت فیزیکی آخر خط ندارید؛ قبل از حرکت، حدود محور را بررسی کنید.";
    case F3H_I3:
      return "کلید توقف دستگاه است. با فعال شدن، موتور و خروجی‌ها خاموش می‌شوند؛ بدون پایه، دستگاه برای ایمنی متوقف می‌ماند.";
    case F3H_I6:
      return "کلید استارت یک است و فقط هم‌زمان با استارت دو برش را شروع می‌کند. بدون پایه، برش آغاز نمی‌شود.";
    case F3H_I7:
      return "کلید استارت دو است و فقط هم‌زمان با استارت یک برش را شروع می‌کند. بدون پایه، برش آغاز نمی‌شود.";
    case F3H_I4:
      return "وضعیت برگشت تیغه را می‌خواند. بدون پایه، وضعیت برگشت تیغه قابل تشخیص نیست.";
    case F3H_I5:
      return "خطای موتور را می‌خواند. با فعال شدن، حرکت و برش فوری متوقف می‌شوند؛ بدون پایه، این حفاظت در دسترس نیست.";
    case F3H_T2:
      return "نقش جویستیک چپ است و به هر ورودی وصل می‌شود. برای ایمنی، هر دو جویستیک باید یک‌بار آزاد دیده شوند.";
    case F3H_T1:
      return "نقش جویستیک راست است و به هر ورودی وصل می‌شود. برای ایمنی، هر دو جویستیک باید یک‌بار آزاد دیده شوند.";
    case F3H_Q5:
      return "شیر اسپری را کنترل می‌کند. لمس کوتاه فقط تست روشن و خاموش است؛ بدون پایه، اسپری روشن نمی‌شود.";
    case F3H_Q3:
      return "جک نگهدارندۀ قطعه را کنترل می‌کند. لمس کوتاه فقط تست است؛ بدون پایه، جک کار نمی‌کند و برش شروع نمی‌شود.";
    case F3H_Q6:
      return "موتور اره را کنترل می‌کند. لمس کوتاه فقط تست است؛ دست و قطعه از تیغه دور باشد. بدون پایه، اره و برش روشن نمی‌شوند.";
    case F3H_Q4:
      return "جک محافظ جلوی تیغه را کنترل می‌کند. لمس کوتاه فقط تست است؛ بدون پایه، محافظ کار نمی‌کند و برش شروع نمی‌شود.";
    case F3H_Q8:
      return "جک بالا و پایین رفتن تیغه را کنترل می‌کند. لمس کوتاه فقط تست است؛ بدون پایه، تیغه حرکت نمی‌کند و برش شروع نمی‌شود.";
    case F3H_Q7:
      return "لامپ دستگاه را کنترل می‌کند. لمس کوتاه فقط تست است؛ بدون پایه، فقط لامپ در دسترس نیست و حرکت دستگاه آزاد می‌ماند.";
    case F3H_Q0:
      return "فرمان پالس حرکت را به درایو استپر می‌فرستد. بدون آن موتور حرکت نمی‌کند و برنامه علت را نشان می‌دهد.";
    case F3H_Q1:
      return "جهت حرکت موتور را به درایو استپر می‌دهد. بدون آن موتور حرکت نمی‌کند و برنامه علت را نشان می‌دهد.";
    case F3H_Q2:
      return "درایو استپر را فعال می‌کند. بدون آن درایو خاموش می‌ماند و موتور حرکت نمی‌کند؛ برنامه علت را نشان می‌دهد.";
    default:
      return "برای راهنما پایه را نگه دارید.";
  }
}

static const char *f3_pin_help_body_en(int id) {
  switch (id) {
    case F3H_I2:
      return "Shows whether the guard door is closed. Green means safe. Cutting is blocked when this input has no pin.";
    case F3H_I0:
      return "Marks the start of travel. Without it, normal motion uses configured limits, but automatic homing/calibration is unavailable, size error is more likely, and rail or motor damage is possible.";
    case F3H_I1:
      return "Marks the end of travel. Without it, normal motion uses configured limits, but physical end protection is unavailable and rail or motor damage is possible.";
    case F3H_I3:
      return "Stops the motor and outputs. With no assigned pin, the machine stays stopped for safety.";
    case F3H_I6:
      return "Start key 1 works together with Start key 2. With no pin, this key cannot start a cut.";
    case F3H_I7:
      return "Start key 2 works together with Start key 1. With no pin, this key cannot start a cut.";
    case F3H_I4:
      return "Blade return sensor (I4). Shows blade-return status. With no pin, it is not read.";
    case F3H_I5:
      return "Motor fault (I5). When active, motion and cutting stop immediately. With no pin, it is not read.";
    case F3H_T2:
      return "Jog-left input. It can use any input terminal. Motion is allowed only after both jog inputs are seen released.";
    case F3H_T1:
      return "Jog-right input. It can use any input terminal. Motion is allowed only after both jog inputs are seen released.";
    case F3H_Q5:
      return "Controls the spray valve. Short tap tests it. With no pin, spray stays off and a message explains why.";
    case F3H_Q3:
      return "Controls the work holder jack. Short tap tests it. Cutting is blocked when this output has no pin.";
    case F3H_Q6:
      return "Controls the saw motor. Short tap tests it; keep clear of the blade. Cutting is blocked when it has no pin.";
    case F3H_Q4:
      return "Controls the front guard jack. Short tap tests it. Cutting is blocked when this output has no pin.";
    case F3H_Q8:
      return "Raises and lowers the saw assembly. Short tap tests it. Cutting is blocked when it has no pin.";
    case F3H_Q7:
      return "Controls the work lamp. Short tap tests it. With no pin, only the lamp is unavailable; motion remains allowed.";
    case F3H_Q0:
      return "Sends motion pulses to the stepper drive. Without it, the motor cannot move and the screen shows the exact reason.";
    case F3H_Q1:
      return "Sets the stepper direction. Without it, the motor cannot start and the screen shows the exact reason.";
    case F3H_Q2:
      return "Enables the stepper drive. Without it, the drive stays off and the screen shows the exact reason.";
    default:
      return "Hold a pin for help.";
  }
}

static bool f3_pin_help_is_q(int id) {
  return id >= F3H_Q5 && id <= F3H_Q2;
}

static lv_obj_t *f3_pin_help_q_btn_from_id(int id) {
  switch (id) {
    case F3H_Q5: return objects.btn2_3_1;
    case F3H_Q3: return objects.btn2_3_2;
    case F3H_Q6: return objects.btn2_3_3;
    case F3H_Q4: return objects.btn2_3_4;
    case F3H_Q8: return objects.btn2_3_5;
    case F3H_Q7: return objects.btn2_3_6;
    case F3H_Q0: return f3_q_step_hits[0];
    case F3H_Q1: return f3_q_step_hits[1];
    case F3H_Q2: return f3_q_step_hits[2];
    default: return NULL;
  }
}

static void f3_pin_help_show(int id) {
  if (id <= 0) return;
  // فقط کادر قبلی را بردار؛ armed را اینجا خنثی نکن (open_id قبل از show آرم می‌کند)
  if (f3_help_root && lv_obj_is_valid(f3_help_root)) {
    lv_obj_delete(f3_help_root);
  }
  f3_help_root = NULL;
  f3_help_close_armed = false;
  f3_help_wait_finger_up = true;  // رها شدن همان long-press کادر را نبیند

  const bool en = (GUI.system_language == 2);

  f3_help_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f3_help_root, 480, 320);
  lv_obj_set_pos(f3_help_root, 0, 0);
  lv_obj_set_style_bg_color(f3_help_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f3_help_root, LV_OPA_50, 0);
  lv_obj_set_style_border_width(f3_help_root, 0, 0);
  lv_obj_set_style_pad_all(f3_help_root, 0, 0);
  lv_obj_set_style_radius(f3_help_root, 0, 0);
  lv_obj_clear_flag(f3_help_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f3_help_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f3_help_root, f3_pin_help_close_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f3_help_root, f3_pin_help_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(f3_help_root, f3_pin_help_close_cb, LV_EVENT_CLICKED, NULL);
  f3_help_opened_ms = millis();
  f3_help_close_armed = false;

  lv_obj_t *card = lv_obj_create(f3_help_root);
  lv_obj_set_size(card, 404, 304);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x2c2c2e), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_left(card, 14, 0);
  lv_obj_set_style_pad_right(card, 14, 0);
  lv_obj_set_style_pad_top(card, 8, 0);
  lv_obj_set_style_pad_bottom(card, 8, 0);
  lv_obj_set_style_pad_row(card, 4, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_40, 0);
  lv_obj_set_style_shadow_offset_y(card, 4, 0);
  lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, f3_help_card_eat_click_cb, LV_EVENT_CLICKED, NULL);

  const int input_role = f3_pinmap_input_role_from_help(id);
  const int output_role = f3_pinmap_output_role_from_help(id);
  const uint8_t display_src = input_role >= 0
    ? pinmap_in_get_pending_source(input_role)
    : (output_role >= 0 ? pinmap_out_get_pending_source(output_role) : 0xff);
  const bool display_input_pin = input_role >= 0 && display_src < PINMAP_IN_NC;
  const bool display_serif_i = display_input_pin && display_src < 8;
  char help_code_buf[8];
  if (display_input_pin) f3_input_source_tag(display_src, help_code_buf, sizeof(help_code_buf));
  else if (output_role >= 0 && display_src < PINMAP_OUT_NC)
    snprintf(help_code_buf, sizeof(help_code_buf), "Q%u", (unsigned)display_src);
  else if (input_role >= 0 || output_role >= 0)
    snprintf(help_code_buf, sizeof(help_code_buf), "N.C.");
  else
    snprintf(help_code_buf, sizeof(help_code_buf), "%s", f3_pin_help_code(id));
  lv_obj_t *code_box = lv_obj_create(card);
  lv_obj_set_size(code_box, 376, input_role >= 0 ? 62 : 48);
  lv_obj_set_style_bg_opa(code_box, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(code_box, 0, 0);
  lv_obj_set_style_pad_all(code_box, 0, 0);
  lv_obj_clear_flag(code_box, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_pos(code_box, 0, 0);
  if (display_serif_i) {
    // I با خط بالا و پایین تا روی LCD با عدد 1 اشتباه نشود.
    const int group_w = 60;
    const int gx = (376 - group_w) / 2;
    lv_obj_t *serif_i = lv_obj_create(code_box);
    lv_obj_set_pos(serif_i, gx, 4);
    lv_obj_set_size(serif_i, 22, 40);
    lv_obj_set_style_bg_opa(serif_i, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(serif_i, 0, 0);
    lv_obj_set_style_pad_all(serif_i, 0, 0);
    lv_obj_clear_flag(serif_i, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    auto make_i_part = [&](int x, int y, int w, int h) {
      lv_obj_t *part = lv_obj_create(serif_i);
      lv_obj_set_pos(part, x, y);
      lv_obj_set_size(part, w, h);
      lv_obj_set_style_radius(part, 0, 0);
      lv_obj_set_style_bg_color(part, lv_color_hex(0x0a84ff), 0);
      lv_obj_set_style_bg_opa(part, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(part, 0, 0);
      lv_obj_set_style_pad_all(part, 0, 0);
      lv_obj_clear_flag(part, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    };
    make_i_part(0, 0, 22, 3);
    make_i_part(8, 3, 6, 34);
    make_i_part(0, 37, 22, 3);
    lv_obj_t *digit = lv_label_create(code_box);
    lv_obj_clear_flag(digit, LV_OBJ_FLAG_CLICKABLE);
    char digit_txt[2] = {(char)('0' + display_src), '\0'};
    lv_label_set_text(digit, digit_txt);
    lv_obj_set_size(digit, 30, 44);
    lv_label_set_long_mode(digit, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(digit, &lv_font_montserrat_38, 0);
    lv_obj_set_style_text_color(digit, lv_color_hex(0x0a84ff), 0);
    lv_obj_set_style_text_align(digit, LV_TEXT_ALIGN_CENTER, 0);
    // طبق تنظیم تصویری مالک: رقم ۱۰px چپ و ۱۴px پایین‌تر از جای قبلی.
    lv_obj_set_pos(digit, gx + 20, 16);
  } else {
    lv_obj_t *code = lv_label_create(code_box);
    lv_obj_clear_flag(code, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(code, 376);
    lv_obj_set_style_text_font(code, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(code, lv_color_hex(0x0a84ff), 0);
    lv_obj_set_style_text_align(code, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(code, help_code_buf);
    lv_obj_center(code);
  }

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_size(title, 376, 28);
  lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(title, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(title, en ? f3_pin_help_title_en(id) : f3_pin_help_title_fa(id));
  // عنوان سفید ورودی دقیقاً ۱۵px بالاتر از چیدمان قبلیِ flex قرار می‌گیرد.
  lv_obj_set_pos(title, 0, 51);

  // متن کوتاه و مستقیم است: هیچ قاب یا اسکرولی وجود ندارد و همۀ راهنما در یک نگاه دیده می‌شود.
  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_size(body, 376, 108);
  lv_obj_set_pos(body, 0, 84);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0xe5e5ea), 0);
  lv_obj_set_style_text_line_space(body, 2, 0);
  lv_obj_set_style_text_align(body, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(body, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(body, en ? f3_pin_help_body_en(id) : f3_pin_help_body_fa(id));

  if (input_role >= 0 || output_role >= 0) {
    lv_obj_t *map_btn = lv_button_create(card);
    lv_obj_set_size(map_btn, 376, 40);
    lv_obj_set_pos(map_btn, 0, 202);
    lv_obj_set_style_bg_color(map_btn, lv_color_hex(0x0a84ff), 0);
    lv_obj_set_style_radius(map_btn, 10, 0);
    lv_obj_add_event_cb(map_btn,
                        input_role >= 0 ? f3_pinmap_open_from_help_cb
                                        : f3_pinmap_open_output_from_help_cb,
                        LV_EVENT_CLICKED,
                        (void *)(intptr_t)(input_role >= 0 ? input_role : output_role));
    lv_obj_t *ml = lv_label_create(map_btn);
    lv_obj_set_style_text_font(ml, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
    lv_obj_set_style_text_color(ml, lv_color_hex(0xffffff), 0);
    lv_label_set_text(ml, en ? "Configure connection" : "تنظیم اتصال");
    lv_obj_center(ml);
  }

  lv_obj_t *cancel = lv_button_create(card);
  lv_obj_set_size(cancel, 376, 40);
  // با فاصلۀ ۸px از کلید تنظیم، کف محتوای کارت را پر می‌کند.
  lv_obj_set_pos(cancel, 0, 250);
  lv_obj_set_style_bg_color(cancel, lv_color_hex(0x636366), 0);
  lv_obj_set_style_radius(cancel, 10, 0);
  lv_obj_add_event_cb(cancel, f3_pin_help_cancel_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *cancel_lbl = lv_label_create(cancel);
  lv_obj_set_style_text_font(cancel_lbl, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(cancel_lbl, en ? "Cancel" : "انصراف");
  lv_obj_center(cancel_lbl);

  GUI.buzzer_cmd_short = true;
}

static int f3_pin_help_id_from_q_btn(lv_obj_t *btn) {
  if (!btn) return 0;
  if (btn == objects.btn2_3_1) return F3H_Q5;
  if (btn == objects.btn2_3_2) return F3H_Q3;
  if (btn == objects.btn2_3_3) return F3H_Q6;
  if (btn == objects.btn2_3_4) return F3H_Q4;
  if (btn == objects.btn2_3_5) return F3H_Q8;
  if (btn == objects.btn2_3_6) return F3H_Q7;
  return 0;
}

static int f3_pin_help_id_from_obj(lv_obj_t *obj) {
  if (!obj) return 0;
  for (lv_obj_t *cur = obj; cur; cur = lv_obj_get_parent(cur)) {
    int qid = f3_pin_help_id_from_q_btn(cur);
    if (qid) return qid;
    for (int i = 0; i < 3; ++i) {
      if (cur == f3_q_step_hits[i]) {
        static const int step_ids[3] = {F3H_Q0, F3H_Q1, F3H_Q2};
        return step_ids[i];
      }
    }

    for (int i = 0; i < 6; i++) {
      if (f3_pin_lbls[i] && cur == f3_pin_lbls[i]) {
        static const int qmap[6] = {F3H_Q5, F3H_Q3, F3H_Q6, F3H_Q4, F3H_Q8, F3H_Q7};
        return qmap[i];
      }
    }
    for (int i = 0; i < 8; i++) {
      if (f3_pin_lbls[6 + i] && cur == f3_pin_lbls[6 + i]) {
        static const int imap[8] = {F3H_I0, F3H_I1, F3H_I2, F3H_I3, F3H_I4, F3H_I5, F3H_I6, F3H_I7};
        return imap[i];
      }
    }

    struct { lv_obj_t *o; int id; } hit[] = {
      {objects.lablef2_29, F3H_I2}, {objects.lablee2_29, F3H_I2}, {objects.led2_3_8, F3H_I2},
      {objects.lablef2_19, F3H_I0}, {objects.lablee2_19, F3H_I0}, {objects.led2_3_1, F3H_I0},
      {objects.lablef2_20, F3H_I1}, {objects.lablee2_20, F3H_I1}, {objects.led2_3_2, F3H_I1},
      {objects.lablef2_22, F3H_I3}, {objects.lablee2_22, F3H_I3}, {objects.led2_3_4, F3H_I3},
      {objects.lablef2_21, F3H_I6}, {objects.lablee2_21, F3H_I6}, {objects.led2_3_3, F3H_I6},
      {objects.lablef2_28, F3H_I7}, {objects.lablee2_28, F3H_I7}, {objects.led2_3_7, F3H_I7},
      {objects.lablef2_26, F3H_I4}, {objects.lablee2_26, F3H_I4}, {objects.led2_3_5, F3H_I4},
      {objects.lablef2_23, F3H_I5}, {objects.lablee2_23, F3H_I5}, {objects.led2_3_6, F3H_I5},
      {objects.lablef2_14, F3H_Q5}, {objects.lablee2_14, F3H_Q5},
      {objects.lablef2_13, F3H_Q3}, {objects.lablee2_13, F3H_Q3},
      {objects.lablef2_16, F3H_Q6}, {objects.lablee2_16, F3H_Q6},
      {objects.lablef2_15, F3H_Q4}, {objects.lablee2_15, F3H_Q4},
      {objects.lablef2_18, F3H_Q8}, {objects.lablee2_18, F3H_Q8},
      {objects.lablef2_17, F3H_Q7}, {objects.lablee2_17, F3H_Q7},
    };
    for (unsigned i = 0; i < sizeof(hit) / sizeof(hit[0]); i++) {
      if (hit[i].o && cur == hit[i].o) return hit[i].id;
    }
  }
  return 0;
}

static void f3_pin_help_arm_q(lv_obj_t *btn) {
  f3_help_q_armed = true;
  f3_help_q_btn = btn;
  // پوشش PRESS_LOST + رها شدن بعدی؛ روی همۀ بردها یکسان
  f3_q_suppress_until_ms = millis() + 1200U;
}

static void f3_pin_help_open_id(int id) {
  if (id <= 0) return;
  if (f3_pin_help_is_q(id)) {
    f3_pin_help_arm_q(f3_pin_help_q_btn_from_id(id));
  }
  f3_pin_help_show(id);
  ui_resume_checkpoint();
}

void ui_resume_restore_f3_pinmap(bool is_output, int role) {
  f3_pinmap_open(is_output, role);
}

void ui_resume_restore_f3_pinhelp(int id) {
  if (id > 0) f3_pin_help_open_id(id);
}

// ---- نگهداشت اختصاصی F3 (مستقل از LONG_PRESSED سراسریِ ۲ثانیه) ----
#define F3_HELP_HOLD_MS 600U
#define F3_HELP_MOVE_CANCEL_PX 64
static lv_timer_t *f3_hold_tm = NULL;
static int f3_hold_id = 0;
static lv_point_t f3_hold_pt;
static bool f3_hold_pt_valid = false;

static void f3_hold_cancel(void) {
  if (f3_hold_tm) {
    lv_timer_delete(f3_hold_tm);
    f3_hold_tm = NULL;
  }
  f3_hold_id = 0;
  f3_hold_pt_valid = false;
}

static void f3_hold_timer_cb(lv_timer_t *t) {
  (void)t;
  f3_hold_tm = NULL;  // repeat=1 → بعد از این کال‌بک حذف می‌شود
  int id = f3_hold_id;
  f3_hold_id = 0;
  f3_hold_pt_valid = false;
  if (id == F3H_STEPPER_PARAMS) {
    if (f3_stepper_test_cmd || f3_stepper_test_busy || f3_stepper_test_recover_cmd) return;
    f3_stepper_params_open();
    return;
  }
  if (id > 0 && f3_pin_help_is_q(id)) {
    const int role = f3_pinmap_output_role_from_help(id);
    if (role >= 0) {
      // نگه‌داشت خروجی، تنظیم همان خروجی را باز می‌کند؛ رها شدن نباید تست Q را بزند.
      f3_pin_help_arm_q(f3_pin_help_q_btn_from_id(id));
      f3_pinmap_open(true, role);
      return;
    }
  }
  if (id > 0) f3_pin_help_open_id(id);
}

static void f3_hold_start(int id) {
  if (id <= 0) return;
  f3_hold_cancel();
  f3_hold_id = id;
  f3_hold_pt_valid = false;
  lv_indev_t *indev = lv_indev_active();
  if (indev) {
    lv_indev_get_point(indev, &f3_hold_pt);
    f3_hold_pt_valid = true;
  }
  f3_hold_tm = lv_timer_create(f3_hold_timer_cb, F3_HELP_HOLD_MS, NULL);
  if (f3_hold_tm) lv_timer_set_repeat_count(f3_hold_tm, 1);
}

static void f3_hold_ev_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  int id = (int)(intptr_t)lv_event_get_user_data(e);
  if (id <= 0) id = f3_pin_help_id_from_obj((lv_obj_t *)lv_event_get_target(e));

  if (code == LV_EVENT_PRESSED) {
    f3_hold_start(id);
    return;
  }
  if (code == LV_EVENT_PRESSING) {
    if (!f3_hold_tm || !f3_hold_pt_valid) return;
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    int dx = (int)p.x - (int)f3_hold_pt.x;
    int dy = (int)p.y - (int)f3_hold_pt.y;
    if (LV_ABS(dx) >= F3_HELP_MOVE_CANCEL_PX || LV_ABS(dy) >= F3_HELP_MOVE_CANCEL_PX) {
      f3_hold_cancel();
    }
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    // اگر هنوز راهنما باز نشده، تایمر را لغو کن (لمس کوتاه = تاگل)
    if (f3_hold_tm) f3_hold_cancel();
  }
}

// long-press → overlay روی انگشت می‌آید → اغلب PRESS_LOST نه RELEASED.
// suppress زمانی + PRESS_LOST با هم جلوی گیرکردن و تاگل ناخواسته را می‌گیرند.
static void f3_q_help_release_pre_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
  if (!f3_q_suppress_active()) return;
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (f3_help_q_btn && btn && btn != f3_help_q_btn) return;
  if (btn) lv_obj_clear_state(btn, LV_STATE_PRESSED);
  if (code == LV_EVENT_RELEASED) {
    f3_pin_help_disarm_q();
    f3_q_suppress_until_ms = 0;
    lv_event_stop_processing(e);
  } else {
    // PRESS_LOST: فلگ دکمه را بردار ولی پنجرۀ زمانی بماند تا RELEASED بعدی هم قطع شود
    f3_pin_help_disarm_q();
    if (!f3_q_suppress_until_ms || (int32_t)(millis() - f3_q_suppress_until_ms) >= 0) {
      f3_q_suppress_until_ms = millis() + 800U;
    }
  }
}

static void f3_pin_help_bind_target(lv_obj_t *obj, int id) {
  if (!obj || id <= 0) return;
  ui_control_help_unbind(obj);  // PRESSING عمومی تایمر سراسری را می‌کشد
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_USER_1);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_USER_2);  // جلوگیری از راهنمای عمومی روی F3
  lv_obj_add_flag(obj, LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_remove_event_cb(obj, f3_hold_ev_cb);
  lv_obj_add_event_cb(obj, f3_hold_ev_cb, LV_EVENT_PRESSED, (void *)(intptr_t)id);
  lv_obj_add_event_cb(obj, f3_hold_ev_cb, LV_EVENT_PRESSING, (void *)(intptr_t)id);
  lv_obj_add_event_cb(obj, f3_hold_ev_cb, LV_EVENT_RELEASED, (void *)(intptr_t)id);
  lv_obj_add_event_cb(obj, f3_hold_ev_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)id);
}

// هایلایت زرد ورودی‌ها هنگام تاچ (متن مشکی ← زرد ← مشکی)
static void f3_i_row_set_hl(int id, bool on) {
  const uint32_t yel = 0xFFCC00u;
  const uint32_t blk = 0x000000u;
  const uint32_t badge_blk = 0x1c1c1eu;
  struct { lv_obj_t *fa; lv_obj_t *en; lv_obj_t *badge; int id; } rows[] = {
    {objects.lablef2_19, objects.lablee2_19, f3_pin_lbls[6],  F3H_I0},
    {objects.lablef2_20, objects.lablee2_20, f3_pin_lbls[7],  F3H_I1},
    {objects.lablef2_29, objects.lablee2_29, f3_pin_lbls[8],  F3H_I2},
    {objects.lablef2_22, objects.lablee2_22, f3_pin_lbls[9],  F3H_I3},
    {objects.lablef2_26, objects.lablee2_26, f3_pin_lbls[10], F3H_I4},
    {objects.lablef2_23, objects.lablee2_23, f3_pin_lbls[11], F3H_I5},
    {objects.lablef2_21, objects.lablee2_21, f3_pin_lbls[12], F3H_I6},
    {objects.lablef2_28, objects.lablee2_28, f3_pin_lbls[13], F3H_I7},
    {f3_joy_lab_fa_r, f3_joy_lab_en_r, f3_joy_badge_r, F3H_T1},
    {f3_joy_lab_fa_l, f3_joy_lab_en_l, f3_joy_badge_l, F3H_T2},
  };
  for (unsigned i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
    if (rows[i].id != id) continue;
    if (rows[i].fa)
      lv_obj_set_style_text_color(rows[i].fa, lv_color_hex(on ? yel : blk), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (rows[i].en)
      lv_obj_set_style_text_color(rows[i].en, lv_color_hex(on ? yel : blk), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *badge = rows[i].badge;
    if (badge && lv_obj_is_valid(badge))
      lv_obj_set_style_bg_color(badge, lv_color_hex(on ? yel : badge_blk), 0);
    break;
  }
}

static void f3_i_touch_hl_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  int id = (int)(intptr_t)lv_event_get_user_data(e);
  if (id <= 0) return;
  if (code == LV_EVENT_PRESSED) f3_i_row_set_hl(id, true);
  else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) f3_i_row_set_hl(id, false);
}

static void f3_pin_help_wire_after_reflow() {
  lv_obj_t *qbtns[] = {
    objects.btn2_3_1, objects.btn2_3_2, objects.btn2_3_3,
    objects.btn2_3_4, objects.btn2_3_5, objects.btn2_3_6
  };
  static const int q_ids[6] = {F3H_Q5, F3H_Q3, F3H_Q6, F3H_Q4, F3H_Q8, F3H_Q7};
  for (unsigned i = 0; i < 6; i++) {
    if (!qbtns[i]) continue;
    ui_control_help_unbind(qbtns[i]);
    lv_obj_add_flag(qbtns[i], LV_OBJ_FLAG_USER_1);
    lv_obj_add_flag(qbtns[i], LV_OBJ_FLAG_USER_2);
    lv_obj_add_flag(qbtns[i], LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_remove_event_cb(qbtns[i], f3_hold_ev_cb);
    lv_obj_add_event_cb(qbtns[i], f3_hold_ev_cb, LV_EVENT_PRESSED, (void *)(intptr_t)q_ids[i]);
    lv_obj_add_event_cb(qbtns[i], f3_hold_ev_cb, LV_EVENT_PRESSING, (void *)(intptr_t)q_ids[i]);
    lv_obj_add_event_cb(qbtns[i], f3_hold_ev_cb, LV_EVENT_RELEASED, (void *)(intptr_t)q_ids[i]);
    lv_obj_add_event_cb(qbtns[i], f3_hold_ev_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)q_ids[i]);
    if (!f3_help_q_ev_wired) {
      lv_obj_add_event_cb(qbtns[i], f3_q_help_release_pre_cb,
                          (lv_event_code_t)(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS), NULL);
      lv_obj_add_event_cb(qbtns[i], f3_q_help_release_pre_cb,
                          (lv_event_code_t)(LV_EVENT_PRESS_LOST | LV_EVENT_PREPROCESS), NULL);
    }
  }
  f3_help_q_ev_wired = true;

  static const int i_ids[8] = {
    F3H_I0, F3H_I1, F3H_I2, F3H_I3, F3H_I4, F3H_I5, F3H_I6, F3H_I7
  };

  // بج/متن روی دکمۀ Q را CLICKABLE نکن — تاچ کوتاه باید به خود دکمه برسد (تاگل خروجی).
  for (int i = 0; i < 6; i++) {
    if (!f3_pin_lbls[i]) continue;
    ui_control_help_unbind(f3_pin_lbls[i]);
    if (i == 5) {
      // بج‌های Q8 و Q9 مستقیماً قابل لمس برای تعویض پایه
      lv_obj_add_flag(f3_pin_lbls[i], LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_event_cb(f3_pin_lbls[i], f3_hold_ev_cb);
      lv_obj_add_event_cb(f3_pin_lbls[i], [](lv_event_t *) {
        f3_pinmap_open(true, 8);
        if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
      }, LV_EVENT_CLICKED, NULL);
    } else {
      lv_obj_clear_flag(f3_pin_lbls[i], LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_event_cb(f3_pin_lbls[i], f3_hold_ev_cb);
    }
  }
  if (f3_pin_q9_lbl) {
    ui_control_help_unbind(f3_pin_q9_lbl);
    lv_obj_add_flag(f3_pin_q9_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_event_cb(f3_pin_q9_lbl, f3_hold_ev_cb);
    lv_obj_add_event_cb(f3_pin_q9_lbl, [](lv_event_t *) {
      f3_pinmap_open(true, 9);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    }, LV_EVENT_CLICKED, NULL);
  }

  for (int i = 0; i < 8; i++) {
    if (f3_pin_lbls[6 + i]) f3_pin_help_bind_target(f3_pin_lbls[6 + i], i_ids[i]);
  }

  struct { lv_obj_t *a; lv_obj_t *b; lv_obj_t *led; int id; } rows[] = {
    {objects.lablef2_19, objects.lablee2_19, objects.led2_3_1, F3H_I0},
    {objects.lablef2_20, objects.lablee2_20, objects.led2_3_2, F3H_I1},
    {objects.lablef2_29, objects.lablee2_29, objects.led2_3_8, F3H_I2},
    {objects.lablef2_22, objects.lablee2_22, objects.led2_3_4, F3H_I3},
    {objects.lablef2_26, objects.lablee2_26, objects.led2_3_5, F3H_I4},
    {objects.lablef2_23, objects.lablee2_23, objects.led2_3_6, F3H_I5},
    {objects.lablef2_21, objects.lablee2_21, objects.led2_3_3, F3H_I6},
    {objects.lablef2_28, objects.lablee2_28, objects.led2_3_7, F3H_I7},
  };
  for (unsigned i = 0; i < 8; i++) {
    f3_pin_help_bind_target(rows[i].a, rows[i].id);
    f3_pin_help_bind_target(rows[i].b, rows[i].id);
    f3_pin_help_bind_target(rows[i].led, rows[i].id);
    // هایلایت زرد هنگام تاچ ورودی (متن + بج)
    if (rows[i].a) {
      lv_obj_remove_event_cb(rows[i].a, f3_i_touch_hl_cb);
      lv_obj_add_event_cb(rows[i].a, f3_i_touch_hl_cb, LV_EVENT_PRESSED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(rows[i].a, f3_i_touch_hl_cb, LV_EVENT_RELEASED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(rows[i].a, f3_i_touch_hl_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)rows[i].id);
    }
    if (rows[i].b) {
      lv_obj_remove_event_cb(rows[i].b, f3_i_touch_hl_cb);
      lv_obj_add_event_cb(rows[i].b, f3_i_touch_hl_cb, LV_EVENT_PRESSED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(rows[i].b, f3_i_touch_hl_cb, LV_EVENT_RELEASED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(rows[i].b, f3_i_touch_hl_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)rows[i].id);
    }
    if (rows[i].led) {
      lv_obj_remove_event_cb(rows[i].led, f3_i_touch_hl_cb);
      lv_obj_add_event_cb(rows[i].led, f3_i_touch_hl_cb, LV_EVENT_PRESSED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(rows[i].led, f3_i_touch_hl_cb, LV_EVENT_RELEASED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(rows[i].led, f3_i_touch_hl_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)rows[i].id);
    }
    if (f3_pin_lbls[6 + i]) {
      lv_obj_remove_event_cb(f3_pin_lbls[6 + i], f3_i_touch_hl_cb);
      lv_obj_add_event_cb(f3_pin_lbls[6 + i], f3_i_touch_hl_cb, LV_EVENT_PRESSED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(f3_pin_lbls[6 + i], f3_i_touch_hl_cb, LV_EVENT_RELEASED, (void *)(intptr_t)rows[i].id);
      lv_obj_add_event_cb(f3_pin_lbls[6 + i], f3_i_touch_hl_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)rows[i].id);
    }
  }

  // لیبل‌های متن Q: غیرقابل‌کلیک تا تاچ به دکمه برسد
  lv_obj_t *qlabs[] = {
    objects.lablef2_14, objects.lablee2_14,
    objects.lablef2_13, objects.lablee2_13,
    objects.lablef2_16, objects.lablee2_16,
    objects.lablef2_15, objects.lablee2_15,
    objects.lablef2_18, objects.lablee2_18,
    objects.lablef2_17, objects.lablee2_17,
  };
  for (unsigned i = 0; i < sizeof(qlabs) / sizeof(qlabs[0]); i++) {
    if (!qlabs[i]) continue;
    ui_control_help_unbind(qlabs[i]);
    lv_obj_clear_flag(qlabs[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_event_cb(qlabs[i], f3_hold_ev_cb);
  }

  static const int step_ids[3] = {F3H_Q0, F3H_Q1, F3H_Q2};
  for (int i = 0; i < 3; ++i)
    if (f3_q_step_hits[i]) f3_pin_help_bind_target(f3_q_step_hits[i], step_ids[i]);

  // دو نقش جویستیک؛ منبع فیزیکی آن‌ها نیز از pinmap خوانده می‌شود.
  if (GUI.joystick_enable) {
    f3_pin_help_bind_target(f3_joy_led_r, F3H_T1);
    f3_pin_help_bind_target(f3_joy_lab_fa_r, F3H_T1);
    f3_pin_help_bind_target(f3_joy_lab_en_r, F3H_T1);
    f3_pin_help_bind_target(f3_joy_badge_r, F3H_T1);
    f3_pin_help_bind_target(f3_joy_led_l, F3H_T2);
    f3_pin_help_bind_target(f3_joy_lab_fa_l, F3H_T2);
    f3_pin_help_bind_target(f3_joy_lab_en_l, F3H_T2);
    f3_pin_help_bind_target(f3_joy_badge_l, F3H_T2);

    // جویستیک‌ها همان بازخورد لمسی زردِ سایر ورودی‌ها را دارند؛ رهاشدن فوراً رنگ پایه را برمی‌گرداند.
    struct { lv_obj_t *obj; int id; } joy_targets[] = {
      {f3_joy_led_r, F3H_T1}, {f3_joy_lab_fa_r, F3H_T1},
      {f3_joy_lab_en_r, F3H_T1}, {f3_joy_badge_r, F3H_T1},
      {f3_joy_led_l, F3H_T2}, {f3_joy_lab_fa_l, F3H_T2},
      {f3_joy_lab_en_l, F3H_T2}, {f3_joy_badge_l, F3H_T2},
    };
    for (const auto &target : joy_targets) {
      if (!target.obj) continue;
      lv_obj_remove_event_cb(target.obj, f3_i_touch_hl_cb);
      lv_obj_add_event_cb(target.obj, f3_i_touch_hl_cb, LV_EVENT_PRESSED, (void *)(intptr_t)target.id);
      lv_obj_add_event_cb(target.obj, f3_i_touch_hl_cb, LV_EVENT_RELEASED, (void *)(intptr_t)target.id);
      lv_obj_add_event_cb(target.obj, f3_i_touch_hl_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)target.id);
    }
  }
}

bool f3_try_pin_help(lv_obj_t *obj) {
  if (!obj || GUI.scr_id != 2 || !objects.tabview2_1) return false;
  if ((int)lv_tabview_get_tab_act(objects.tabview2_1) != 2) return false;
  int id = f3_pin_help_id_from_obj(obj);
  if (!id) return false;
  if (!GUI.joystick_enable && (id == F3H_T1 || id == F3H_T2)) return false;
  f3_pin_help_open_id(id);
  return true;
}
#else
bool f3_try_pin_help(lv_obj_t *obj) {
  (void)obj;
  return false;
}
bool f3_pinmap_dialog_is_open() { return false; }
bool ui_resume_f3_pinmap_is_open() { return false; }
int ui_resume_f3_pinmap_pack() { return -1; }
int ui_resume_f3_pinhelp_id() { return 0; }
void ui_resume_restore_f3_pinmap(bool is_output, int role) { (void)is_output; (void)role; }
void ui_resume_restore_f3_pinhelp(int id) { (void)id; }
void f3_pinmap_request_external_close() {}
bool f3_pinmap_commit_on_save() { return true; }
bool f3_pinmap_panel_confirm() { return false; }
bool f3_pinmap_panel_cancel() { return false; }
bool f3_pinmap_panel_adjust(lv_obj_t *obj, int dir) {
  (void)obj; (void)dir;
  return false;
}
static void f3_pinmap_runtime_tick() {}
static void f3_pin_help_close() {}
static void f3_pin_help_wire_after_reflow() {}
#endif


// لمس کوتاه Q0/Q1/Q2: تست پالس استپر (اجرا در Task3)؛ رنگ کپسول ثابت می‌ماند.
static lv_obj_t *f3_q012_lit = NULL;

static void f3_q012_set_lit(lv_obj_t *hit, bool on) {
  // تغییرنداشتن وضعیت نباید هر ۵۰ms کادر F3 را دوباره invalidate کند.
  if ((on && f3_q012_lit == hit) || (!on && f3_q012_lit == NULL)) return;
  // لمس بدنۀ کپسول، خود f3_q_cap را هدف می‌گیرد؛ شفاف‌کردن آن آبی پایه را
  // تا پایان تست حذف می‌کرد. بدنه همیشه آبی می‌ماند و بازخورد فقط روی Q لمس‌شده است.
  if (f3_q_cap && lv_obj_is_valid(f3_q_cap))
    lv_obj_set_style_bg_opa(f3_q_cap, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (f3_q012_lit && lv_obj_is_valid(f3_q012_lit) && f3_q012_lit != hit) {
    if (f3_q012_lit != f3_q_cap)
      lv_obj_set_style_bg_opa(f3_q012_lit, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  f3_q012_lit = on ? hit : NULL;
}

static void f3_q012_sync_run_status(bool active) {
  if (!f3_q_run_pulse_cap || !f3_q_run_pulse_value ||
      !f3_q_run_time_cap || !f3_q_run_time_value) return;
  if (f3_q_run_status_visible != active) {
    for (int i = 0; i < 3; ++i) {
      if (f3_q_step_badges[i]) {
        if (active) lv_obj_add_flag(f3_q_step_badges[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(f3_q_step_badges[i], LV_OBJ_FLAG_HIDDEN);
      }
      if (f3_q_step_labels[i]) {
        if (active) lv_obj_add_flag(f3_q_step_labels[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(f3_q_step_labels[i], LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (active) {
      lv_obj_clear_flag(f3_q_run_pulse_cap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(f3_q_run_pulse_value, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(f3_q_run_time_cap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(f3_q_run_time_value, LV_OBJ_FLAG_HIDDEN);
      f3_q_run_last_pulses = UINT32_MAX;
      f3_q_run_last_seconds = UINT32_MAX;
    } else {
      lv_obj_add_flag(f3_q_run_pulse_cap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(f3_q_run_pulse_value, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(f3_q_run_time_cap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(f3_q_run_time_value, LV_OBJ_FLAG_HIDDEN);
    }
    f3_q_run_status_visible = active;
  }
  if (!active) return;
  const uint32_t pulses = f3_stepper_test_pulse_count;
  const uint32_t deadline = f3_stepper_test_deadline_ms;
  const int32_t left_ms = deadline ? (int32_t)(deadline - millis()) : 0;
  const uint32_t left_s = left_ms > 0 ? ((uint32_t)left_ms + 999U) / 1000U : 0U;
  if (f3_q_run_last_pulses != pulses) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)pulses);
    lv_label_set_text(f3_q_run_pulse_value, buf);
    f3_q_run_last_pulses = pulses;
  }
  if (f3_q_run_last_seconds != left_s) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(left_s / 60U),
             (unsigned long)(left_s % 60U));
    lv_label_set_text(f3_q_run_time_value, buf);
    f3_q_run_last_seconds = left_s;
  }
}

static void f3_stepper_stop_modal_close() {
  if (f3_stepper_stop_root && lv_obj_is_valid(f3_stepper_stop_root))
    lv_obj_delete(f3_stepper_stop_root);
  f3_stepper_stop_root = NULL;
  GUI.lock_screen = false;
}

static void f3_stepper_stop_confirm_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (IOS.stop_button_state || sensor_alert_motion_lock || g_update_mode) {
    GUI.buzzer_cmd_long = true;
    ui_toast("ابتدا استاپ اضطراری را آزاد کنید", "Release STOP before returning", 2600);
    return;
  }
  f3_stepper_test_stop_fault = false;
  f3_stepper_test_abort = false;
  f3_stepper_test_recover_cmd = true;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) { f3_stepper_stop_modal_close(); }, NULL);
}

void f3_stepper_test_ui_tick() {
  if (!f3_stepper_test_stop_fault || !f3_stepper_test_recovery_needed ||
      f3_stepper_stop_root) return;
  const bool en = (GUI.system_language == 2);
  f3_stepper_stop_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f3_stepper_stop_root, 480, 320);
  lv_obj_set_pos(f3_stepper_stop_root, 0, 0);
  lv_obj_set_style_bg_color(f3_stepper_stop_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f3_stepper_stop_root, LV_OPA_60, 0);
  lv_obj_set_style_border_width(f3_stepper_stop_root, 0, 0);
  lv_obj_set_style_pad_all(f3_stepper_stop_root, 0, 0);
  lv_obj_clear_flag(f3_stepper_stop_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f3_stepper_stop_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(f3_stepper_stop_root);
  lv_obj_set_size(card, 408, 238);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_width(title, 376);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xff3b30), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  if (!en) lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(title, en ? "Stepper test stopped" : "تست استپر متوقف شد");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_width(body, 360);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
  if (!en) lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, 0);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_label_set_text(body, en
      ? "Motor calibration may be disturbed. Power-cycle the device, or release STOP and confirm return to the previous point."
      : "کالیبره موتور ممکن است به هم خورده باشد. دستگاه را خاموش و روشن کنید، یا پس از رها کردن استاپ، تایید بازگشت را بزنید تا استپر به جای قبلی برگردد.");
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 58);

  lv_obj_t *confirm = lv_button_create(card);
  lv_obj_set_size(confirm, 300, 48);
  lv_obj_align(confirm, LV_ALIGN_BOTTOM_MID, 0, -14);
  lv_obj_set_style_bg_color(confirm, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_bg_color(confirm, lv_color_hex(0x0066cc), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(confirm, 10, 0);
  lv_obj_add_event_cb(confirm, f3_stepper_stop_confirm_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *label = lv_label_create(confirm);
  lv_obj_set_style_text_font(label, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_label_set_text(label, en ? "Confirm return" : "تایید بازگشت");
  lv_obj_center(label);
  GUI.lock_screen = true;
}

void f3_q012_sync_active_green() {
  const bool active = f3_stepper_test_busy || f3_stepper_test_cmd ||
                      f3_stepper_test_recover_cmd;
  f3_q012_sync_run_status(active);
  if (active) {
    if (f3_q012_lit && lv_obj_is_valid(f3_q012_lit)) f3_q012_set_lit(f3_q012_lit, true);
  } else {
    f3_q012_set_lit(NULL, false);
  }
}

static void f3_q012_short_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (GUI.lock_screen || IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
#if F3_PIN_HELP_ENABLE
  if (f3_help_root || f3_q_suppress_active()) return;
#endif
  if (pinmap_pending_has_changes()) {
    GUI.buzzer_cmd_long = true;
    ui_toast("تغییر پایه ها هنوز ذخیره نشده؛ اول ذخیره را بزنید",
             "Pin changes are not saved; press Save first", 3200);
    return;
  }
  // حین تست: دوباره بزن = برگرد به نقطۀ شروع و خاموش
  if (f3_stepper_test_cmd || f3_stepper_test_busy) {
    f3_stepper_test_abort = true;
    GUI.buzzer_cmd_short = true;
    return;
  }
  if (STP.flag_is_cutting || STP.motorIs_moving || STP.jogEnabled || STP.moveStepperCMD) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (pinmap_motion_missing_reason() != PINMAP_NOTICE_NONE) {
    pinmap_notice_request_motion_block();
    GUI.buzzer_cmd_long = true;
    return;
  }
  lv_obj_t *hit = (lv_obj_t *)lv_event_get_target(e);
  f3_q012_set_lit(hit, true);
  f3_stepper_test_abort = false;
  f3_stepper_test_cmd = true;
  GUI.buzzer_cmd_short = true;
}

static void f3_q_step_set_hl(int role, bool on) {
  if (role < 0 || role >= 3) return;
  // واکنش لمس فقط روی خود گزینه است؛ وضعیت فعال رنگ کل نوار را تغییر نمی‌دهد.
  lv_obj_t *hit = f3_q_step_hits[role];
  if (!hit || !lv_obj_is_valid(hit)) return;
  lv_obj_set_style_bg_color(hit, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(hit, on ? LV_OPA_20 : LV_OPA_TRANSP,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void f3_q_step_touch_hl_cb(lv_event_t *e) {
  const int role = (int)(intptr_t)lv_event_get_user_data(e);
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) f3_q_step_set_hl(role, true);
  else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) f3_q_step_set_hl(role, false);
}

static void f3_stepper_params_close(void) {
  if (f3_stepper_adjust_timer) {
    lv_timer_delete(f3_stepper_adjust_timer);
    f3_stepper_adjust_timer = NULL;
  }
  if (f3_stepper_params_root && lv_obj_is_valid(f3_stepper_params_root))
    lv_obj_delete(f3_stepper_params_root);
  f3_stepper_params_root = NULL;
  f3_pinmap_test_pulses_value = NULL;
  f3_pinmap_test_period_value = NULL;
  GUI.lock_screen = false;
}

static void f3_stepper_params_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) { f3_stepper_params_close(); }, NULL);
}

static void f3_stepper_params_open(void) {
  if (f3_stepper_params_root || GUI.lock_screen || f3_pinmap_dialog_is_open()) return;
  const bool en = (GUI.system_language == 2);
  f3_stepper_params_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f3_stepper_params_root, 480, 320);
  lv_obj_set_pos(f3_stepper_params_root, 0, 0);
  lv_obj_set_style_bg_color(f3_stepper_params_root, lv_color_hex(0x111820), 0);
  lv_obj_set_style_bg_opa(f3_stepper_params_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f3_stepper_params_root, 0, 0);
  lv_obj_set_style_pad_all(f3_stepper_params_root, 0, 0);
  lv_obj_clear_flag(f3_stepper_params_root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(f3_stepper_params_root);
  lv_obj_set_width(title, 456);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_22 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  if (!en) lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(title, en ? "Stepper test settings" : "تنظیم تست استپر");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t *hint = lv_label_create(f3_stepper_params_root);
  lv_obj_set_width(hint, 456);
  lv_obj_set_style_text_font(hint, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(0xaeb9c5), 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  if (!en) lv_obj_set_style_base_dir(hint, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(hint, en ? "Range 10-1000  |  Tap 1  |  Hold after 1s: 10" :
                            "حد ده تا هزار؛ لمس یکی؛ نگه داشتن: ده تایی");
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 54);

  f3_stepper_params_add_control(f3_stepper_params_root, 90, "تعداد پالس", "Pulse count",
                                &f3_pinmap_test_pulses_value, -1, 1, en);
  f3_stepper_params_add_control(f3_stepper_params_root, 160, "فاصله هر پالس (میلی ثانیه)", "Pulse interval (ms)",
                                &f3_pinmap_test_period_value, -2, 2, en);
  f3_pinmap_test_values_sync();

  lv_obj_t *save_hint = lv_label_create(f3_stepper_params_root);
  lv_obj_set_width(save_hint, 456);
  lv_obj_set_style_text_font(save_hint, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(save_hint, lv_color_hex(0xaeb9c5), 0);
  lv_obj_set_style_text_align(save_hint, LV_TEXT_ALIGN_CENTER, 0);
  if (!en) lv_obj_set_style_base_dir(save_hint, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(save_hint, en ? "To store changes, press the green F3 Save button." :
                                 "برای ثبت تغییرات، کلید سبز ذخیره را بزنید.");
  lv_obj_align(save_hint, LV_ALIGN_TOP_MID, 0, 232);

  lv_obj_t *close = lv_button_create(f3_stepper_params_root);
  lv_obj_set_size(close, 456, 48);
  lv_obj_set_pos(close, 12, 264);
  lv_obj_set_style_bg_color(close, lv_color_hex(0x485665), 0);
  lv_obj_set_style_bg_color(close, lv_color_hex(0x343f4b), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(close, 10, 0);
  lv_obj_add_event_cb(close, f3_stepper_params_close_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *close_label = lv_label_create(close);
  lv_obj_set_style_text_font(close_label, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_label_set_text(close_label, en ? "Close" : "بستن");
  lv_obj_center(close_label);

  GUI.lock_screen = true;
  GUI.buzzer_cmd_short = true;
}

static void f3_pinmap_sync_badge_text(lv_obj_t *badge, const char *new_text) {
  if (!badge || !lv_obj_is_valid(badge)) return;
  uint32_t cn = lv_obj_get_child_count(badge);
  for (uint32_t ci = 0; ci < cn; ci++) {
    lv_obj_t *ch = lv_obj_get_child(badge, ci);
    if (!ch || !lv_obj_check_type(ch, &lv_label_class)) continue;
    lv_label_set_text(ch, new_text);
    lv_obj_center(ch);
    return;
  }
}

void f3_pinmap_reflow() {
  if (!objects.obj14) return;
  if (f3_pin_wired && f3_layout_ver == F3_LAYOUT_VER &&
      f3_language_seen == GUI.system_language &&
      f3_joy_enabled_seen == GUI.joystick_enable) {
    if (f3_pending_revision_seen == pinmap_pending_revision) return;
    char imap_tag[8][4];
    for (unsigned i = 0; i < 8; i++) {
      const uint8_t src = pinmap_in_get_pending_source((int)i);
      f3_input_source_tag(src, imap_tag[i], sizeof(imap_tag[i]));
      f3_pinmap_sync_badge_text(f3_pin_lbls[6 + i], imap_tag[i]);
    }
    struct { int role; } qo[] = { {5}, {3}, {6}, {4}, {8}, {7} };
    char qmap_tag[6][4];
    for (int i = 0; i < 6; i++) {
      if (i == 4) {
        f3_pinmap_sync_badge_text(f3_pin_lbls[i], "Q8");
        f3_pinmap_sync_badge_text(f3_pin_q9_lbl, "Q9");
      } else {
        const uint8_t src = pinmap_out_get_pending_source(qo[i].role);
        if (src <= 9) snprintf(qmap_tag[i], sizeof(qmap_tag[i]), "Q%u", (unsigned)src);
        else snprintf(qmap_tag[i], sizeof(qmap_tag[i]), "X");
        f3_pinmap_sync_badge_text(f3_pin_lbls[i], qmap_tag[i]);
      }
    }
    char step_tag[3][4];
    for (int role = 0; role < 3; ++role) {
      const uint8_t src = pinmap_out_get_pending_source(role);
      if (src <= 9) snprintf(step_tag[role], sizeof(step_tag[role]), "Q%u", (unsigned)src);
      else snprintf(step_tag[role], sizeof(step_tag[role]), "X");
      f3_pinmap_sync_badge_text(f3_q_step_badges[role], step_tag[role]);
    }
    if (f3_joy_enabled_seen) {
      char joy_tag_r[8];
      char joy_tag_l[8];
      f3_input_source_tag(pinmap_in_get_pending_source(8), joy_tag_r, sizeof(joy_tag_r));
      f3_input_source_tag(pinmap_in_get_pending_source(9), joy_tag_l, sizeof(joy_tag_l));
      f3_pinmap_sync_badge_text(f3_joy_badge_r, joy_tag_r);
      f3_pinmap_sync_badge_text(f3_joy_badge_l, joy_tag_l);
    }
    f3_pending_revision_seen = pinmap_pending_revision;
    return;
  }
  if (f3_pin_wired) {
#if F3_PIN_HELP_ENABLE
    f3_pin_help_close();
#endif
    for (int i = 0; i < 14; i++) {
      if (f3_pin_lbls[i] && lv_obj_is_valid(f3_pin_lbls[i])) {
        lv_obj_delete(f3_pin_lbls[i]);
        f3_pin_lbls[i] = NULL;
      }
    }
    if (f3_pin_q9_lbl && lv_obj_is_valid(f3_pin_q9_lbl)) {
      lv_obj_delete(f3_pin_q9_lbl);
      f3_pin_q9_lbl = NULL;
    }
    if (f3_angle_val_lbl && lv_obj_is_valid(f3_angle_val_lbl)) {
      lv_obj_delete(f3_angle_val_lbl);
      f3_angle_val_lbl = NULL;
    }
    for (int i = 0; i < 20; i++) {
      if (f3_dot_lbls[i] && lv_obj_is_valid(f3_dot_lbls[i])) {
        lv_obj_delete(f3_dot_lbls[i]);
        f3_dot_lbls[i] = NULL;
      }
    }
    if (f3_q_cap && lv_obj_is_valid(f3_q_cap)) {
      lv_obj_delete(f3_q_cap);
      f3_q_cap = NULL;
      f3_q012_lit = NULL;
    }
    f3_pin_wired = false;
  }

  // obj14/تب پد تم دارند و کادر را از پایین می‌بُرند → گوشۀ گرد پایین مربع می‌شود
  lv_obj_add_flag(objects.obj14, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  if (objects.obj13) lv_obj_add_flag(objects.obj13, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

  // ---- Title card ----
  if (objects.lablef2_3) {
    lv_obj_t *title_box = lv_obj_get_parent(objects.lablef2_3);
    if (title_box) {
      lv_obj_set_pos(title_box, 64, 4);
      lv_obj_set_size(title_box, 280, 40);
      lv_obj_set_style_radius(title_box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(title_box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(title_box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }

  // هم‌راستا با هدر: خانه x=6 تا لبۀ راست ذخیره x=408 (F1_HOME_X .. F1_SAVE_X+F1_HDR_W).
  // obj14 روی (-16,-16) + پد تب → screen_x = local_x + 16.
  lv_obj_t *ipanel = objects.led2_3_8 ? lv_obj_get_parent(objects.led2_3_8) : NULL;
  const int SCR_L = 6, SCR_R = 408;   // خانه … ذخیره
  const int LOC_OFF = 16;             // local → screen
  const int IP_X = SCR_L - LOC_OFF;   // -10
  const int GAP_WB = 8;               // سفید → آبی
  const int GAP_QQ = 8;               // ستون آبی
  const int QW = 90;                      // ۱۰۰ × ۰٫۹
  const int span = SCR_R - SCR_L;     // 402
  const int IP_W = span - GAP_WB - GAP_QQ - 2 * QW;  // 206
  const int IP_H = 252;
  // سفید: خط بالا ۴px بالاتر از پایۀ ۴۸ → IP_Y=44؛ ارتفاع همان
  const int IP_Y = 44;
  if (ipanel) {
    lv_obj_set_pos(ipanel, IP_X, IP_Y);
    lv_obj_set_size(ipanel, IP_W, IP_H);
    lv_obj_set_style_radius(ipanel, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ipanel, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ipanel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ipanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ipanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ipanel, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ipanel, LV_OBJ_FLAG_SCROLLABLE);
    // خود پنل سفید نباید لمس را بدزدد؛ فقط ردیف‌ها/بج‌ها clickable اند
    lv_obj_clear_flag(ipanel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_shadow_width(ipanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  const int LED_SZ = 18;
  const int I_BADGE = 24;  // رقم montserrat_14 + حرف I داخل دایره
  auto place_led = [LED_SZ, I_BADGE](lv_obj_t *led, int x, int y) {
    if (!led) return;
    lv_obj_set_pos(led, x, y + (I_BADGE - LED_SZ) / 2);
    lv_obj_set_size(led, LED_SZ, LED_SZ);
  };
  // متن وسط‌چین افقی؛ مرکز عمودی روی دایرۀ I و چراغ؛ ۵px بالاتر
  auto place_lab = [I_BADGE](lv_obj_t *fa, lv_obj_t *en, int x, int y, int max_w) {
    const int fa_y = y + (I_BADGE - 18) / 2 - 7;
    const int en_y = y + (I_BADGE - 14) / 2 - 1;
    if (fa) {
      lv_obj_set_pos(fa, x, fa_y);
      lv_obj_set_width(fa, max_w);
      lv_obj_set_height(fa, LV_SIZE_CONTENT);
      lv_label_set_long_mode(fa, LV_LABEL_LONG_CLIP);
      lv_obj_set_style_text_font(fa, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(fa, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(fa, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(fa, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_line_space(fa, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      // فشرده‌سازی ۱px برای جلوگیری از بریدن نوشته‌های بلند، بدون کوچک‌شدن فونت.
      lv_obj_set_style_text_letter_space(fa, -1, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (en) {
      lv_obj_set_pos(en, x, en_y);
      lv_obj_set_width(en, max_w);
      lv_obj_set_height(en, LV_SIZE_CONTENT);
      lv_label_set_long_mode(en, LV_LABEL_LONG_CLIP);
      lv_obj_set_style_text_font(en, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(en, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(en, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  };
  auto place_dot_bridge = [](lv_obj_t *parent, lv_obj_t **slot, int x0, int x1, int cy, int n_force) {
    if (!parent || !slot) return;
    if (*slot && lv_obj_is_valid(*slot)) {
      lv_obj_delete(*slot);
      *slot = NULL;
    }
    int span = x1 - x0;
    if (span < 8) return;
    int n = 5;
    if (n_force > 0) n = n_force;
    else if (span < 16) n = 2;
    else if (span < 24) n = 3;
    else if (span < 32) n = 4;
    // ظرف شفاف؛ نقطه‌ها دایرۀ واقعی روی مرکز عمودی لامپ/I (نه گلیف پایین فونت)
    *slot = lv_obj_create(parent);
    if (!*slot) return;
    const int DOT = 3;
    lv_obj_set_pos(*slot, x0, cy - DOT / 2 - 1);
    lv_obj_set_size(*slot, span, DOT + 2);
    lv_obj_set_style_bg_opa(*slot, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(*slot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(*slot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(*slot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(*slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(*slot, LV_OBJ_FLAG_CLICKABLE);
    for (int i = 0; i < n; i++) {
      lv_obj_t *d = lv_obj_create(*slot);
      if (!d) continue;
      // فاصلۀ یکنواخت داخل شکاف؛ اولین/آخرین نقطه به لبۀ لامپ/I نچسبد
      int dx = (n <= 1) ? (span - DOT) / 2 : (DOT / 2 + (span - DOT) * i / (n - 1));
      if (dx < 0) dx = 0;
      if (dx > span - DOT) dx = span - DOT;
      lv_obj_set_pos(d, dx, 1);
      lv_obj_set_size(d, DOT, DOT);
      lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(d, lv_color_hex(0x8e8e93), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(d, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(d, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(d, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(d, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(d, LV_OBJ_FLAG_CLICKABLE);
    }
  };

  const bool joystick_enabled = GUI.joystick_enable;
  // با جویستیک روشن، چیدمان ۱۰ ردیف فعلی ثابت می‌ماند. با خاموش‌بودن، ۸ ورودی
  // از لبۀ بالایی تا پایینی همان کارت سفید به‌طور یکنواخت پخش می‌شوند.
  const int ROW0 = 6;
  const int ROW1 = joystick_enabled ? 30 : 37;
  const int ROW2 = joystick_enabled ? 54 : 68;
  const int ROW3 = joystick_enabled ? 78 : 99;
  const int ROW4 = joystick_enabled ? 102 : 129;
  const int ROW5 = joystick_enabled ? 126 : 160;
  const int ROW6 = joystick_enabled ? 150 : 191;
  const int ROW7 = joystick_enabled ? 174 : 222;
  const int ROW_T1 = 198, ROW_T2 = 222;
  const int PAD_L = 4;
  const int PAD_R = 6;
  const int DOT_SLOT = 18;
  const int LED_A = PAD_L;
  const int I_COL = IP_W - PAD_R - I_BADGE;
  const int LAB_A = LED_A + LED_SZ + 2 + DOT_SLOT;
  const int LAB_A_W = I_COL - DOT_SLOT - 2 - LAB_A;
  const int rows_y[10] = {
    ROW0, ROW1, ROW2, ROW3, ROW4, ROW5, ROW6, ROW7, ROW_T1, ROW_T2
  };

  place_led(objects.led2_3_1, LED_A, ROW0);  // I0
  place_lab(objects.lablef2_19, objects.lablee2_19, LAB_A, ROW0, LAB_A_W);
  place_led(objects.led2_3_2, LED_A, ROW1);  // I1
  place_lab(objects.lablef2_20, objects.lablee2_20, LAB_A, ROW1, LAB_A_W);
  place_led(objects.led2_3_8, LED_A, ROW2);  // I2
  // ۲px فضای اضافه از هر سمت تا «سوییچ» روی LCD بریده نشود.
  place_lab(objects.lablef2_29, objects.lablee2_29, LAB_A - 2, ROW2, LAB_A_W + 4);
  place_led(objects.led2_3_4, LED_A, ROW3);  // I3
  place_lab(objects.lablef2_22, objects.lablee2_22, LAB_A, ROW3, LAB_A_W);
  place_led(objects.led2_3_5, LED_A, ROW4);  // I4
  place_lab(objects.lablef2_26, objects.lablee2_26, LAB_A, ROW4, LAB_A_W);
  place_led(objects.led2_3_6, LED_A, ROW5);  // I5
  place_lab(objects.lablef2_23, objects.lablee2_23, LAB_A, ROW5, LAB_A_W);
  place_led(objects.led2_3_3, LED_A, ROW6);  // I6
  place_lab(objects.lablef2_21, objects.lablee2_21, LAB_A, ROW6, LAB_A_W);
  place_led(objects.led2_3_7, LED_A, ROW7);  // I7
  place_lab(objects.lablef2_28, objects.lablee2_28, LAB_A, ROW7, LAB_A_W);

  // متن کامل + رقم عربی‌هندی (naz رقم فارسی U+06Fx ندارد)
  if (objects.lablef2_29) lv_label_set_text(objects.lablef2_29, "سوییچ درب محافظ");
  if (objects.lablee2_29) lv_label_set_text(objects.lablee2_29, "Door Guard");
  if (objects.lablef2_19) lv_label_set_text(objects.lablef2_19, "سنسور اول خط");
  if (objects.lablee2_19) lv_label_set_text(objects.lablee2_19, "First Sensor");
  if (objects.lablef2_20) lv_label_set_text(objects.lablef2_20, "سنسور آخر خط");
  if (objects.lablee2_20) lv_label_set_text(objects.lablee2_20, "Last Sensor");
  if (objects.lablef2_22) lv_label_set_text(objects.lablef2_22, "کلید استپ");
  if (objects.lablee2_22) lv_label_set_text(objects.lablee2_22, "Stop");
  if (objects.lablef2_21) lv_label_set_text(objects.lablef2_21, "کلید استارت ١");
  if (objects.lablee2_21) lv_label_set_text(objects.lablee2_21, "Start 1");
  if (objects.lablef2_28) lv_label_set_text(objects.lablef2_28, "کلید استارت ٢");
  if (objects.lablee2_28) lv_label_set_text(objects.lablee2_28, "Start 2");
  if (objects.lablef2_26) lv_label_set_text(objects.lablef2_26, "سنسور پایین تیغه");
  if (objects.lablee2_26) lv_label_set_text(objects.lablee2_26, "Blade Lower Sensor");
  if (objects.lablef2_23) lv_label_set_text(objects.lablef2_23, "سنسور بالا تیغه");
  if (objects.lablee2_23) lv_label_set_text(objects.lablee2_23, "Blade Upper Sensor");

  auto set_joy_rows_visible = [](bool visible) {
    lv_obj_t *joy_objs[] = {
      f3_joy_led_r, f3_joy_lab_fa_r, f3_joy_lab_en_r, f3_joy_badge_r,
      f3_joy_led_l, f3_joy_lab_fa_l, f3_joy_lab_en_l, f3_joy_badge_l,
    };
    for (lv_obj_t *obj : joy_objs) {
      if (!obj || !lv_obj_is_valid(obj)) continue;
      if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
  };

  // جویستیک T1/T2 — ردیف‌های اضافه فقط هنگام فعال‌بودن جویستیک F4 ساخته و نمایش داده می‌شوند.
  auto ensure_joy_row = [&](lv_obj_t *&led, lv_obj_t *&fa, lv_obj_t *&en, lv_obj_t *&badge,
                            int row_y, const char *fa_txt, const char *en_txt, const char *tag) {
    lv_obj_t *dpar = ipanel ? ipanel : objects.obj14;
    if (!dpar) return;
    if (!led || !lv_obj_is_valid(led)) {
      led = lv_led_create(dpar);
      lv_led_set_color(led, lv_color_hex(0x34c759));
      lv_led_set_brightness(led, 0);
    }
    place_led(led, LED_A, row_y);
    if (!fa || !lv_obj_is_valid(fa)) {
      fa = lv_label_create(dpar);
      en = lv_label_create(dpar);
    }
    place_lab(fa, en, LAB_A, row_y, LAB_A_W);
    if (fa) lv_label_set_text(fa, fa_txt);
    if (en) lv_label_set_text(en, en_txt);
    if (badge && lv_obj_is_valid(badge)) lv_obj_delete(badge);
    badge = f3_make_pin_lbl(dpar, tag, I_BADGE, false);
    if (badge) {
      lv_obj_set_pos(badge, I_COL, row_y);
      lv_obj_move_foreground(badge);
    }
  };
  if (joystick_enabled) {
    char joy_tag_r[8];
    char joy_tag_l[8];
    f3_input_source_tag(pinmap_in_get_pending_source(8), joy_tag_r, sizeof(joy_tag_r));
    f3_input_source_tag(pinmap_in_get_pending_source(9), joy_tag_l, sizeof(joy_tag_l));
    ensure_joy_row(f3_joy_led_r, f3_joy_lab_fa_r, f3_joy_lab_en_r, f3_joy_badge_r,
                   ROW_T1, "جویستیک راست", "Jog Right", joy_tag_r);
    ensure_joy_row(f3_joy_led_l, f3_joy_lab_fa_l, f3_joy_lab_en_l, f3_joy_badge_l,
                   ROW_T2, "جویستیک چپ", "Jog Left", joy_tag_l);
    set_joy_rows_visible(true);
    const bool en = (GUI.system_language == 2);
    if (f3_joy_lab_fa_r) {
      if (en) lv_obj_add_flag(f3_joy_lab_fa_r, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(f3_joy_lab_fa_r, LV_OBJ_FLAG_HIDDEN);
    }
    if (f3_joy_lab_en_r) {
      if (en) lv_obj_clear_flag(f3_joy_lab_en_r, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(f3_joy_lab_en_r, LV_OBJ_FLAG_HIDDEN);
    }
    if (f3_joy_lab_fa_l) {
      if (en) lv_obj_add_flag(f3_joy_lab_fa_l, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(f3_joy_lab_fa_l, LV_OBJ_FLAG_HIDDEN);
    }
    if (f3_joy_lab_en_l) {
      if (en) lv_obj_clear_flag(f3_joy_lab_en_l, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(f3_joy_lab_en_l, LV_OBJ_FLAG_HIDDEN);
    }
    f3_joy_prev_l = !pinmap_in_source_active(pinmap_in_get_pending_source(9));
    f3_joy_prev_r = !pinmap_in_source_active(pinmap_in_get_pending_source(8));
  } else {
    set_joy_rows_visible(false);
  }

  // نقطه‌چین لامپ→متن و متن→بج برای هر ۱۰ ردیف (۸×I + T1 + T2)
  {
    lv_obj_t *dpar = ipanel ? ipanel : objects.obj14;
    const bool en_lang = (GUI.system_language == 2);
    lv_obj_t *fa_labs[10] = {
      objects.lablef2_19, objects.lablef2_20, objects.lablef2_29, objects.lablef2_22,
      objects.lablef2_26, objects.lablef2_23, objects.lablef2_21, objects.lablef2_28,
      f3_joy_lab_fa_r, f3_joy_lab_fa_l
    };
    lv_obj_t *en_labs[10] = {
      objects.lablee2_19, objects.lablee2_20, objects.lablee2_29, objects.lablee2_22,
      objects.lablee2_26, objects.lablee2_23, objects.lablee2_21, objects.lablee2_28,
      f3_joy_lab_en_r, f3_joy_lab_en_l
    };
    const int input_row_count = joystick_enabled ? 10 : 8;
    for (int i = 0; i < input_row_count; i++) {
      // مرکز قطعی ردیف؛ وابسته‌نبودن به مختصات والد EEZ جلوی افتادن نقطه‌ها در ردیف بعد را می‌گیرد.
      const int cy = rows_y[i] + I_BADGE / 2;
      // شکاف بعد از لامپ / قبل از بج — نه از وسط لامپ (نقطه داخل لامپ نرود)
      int left0 = LED_A + LED_SZ + 2;
      int left1 = LAB_A - 2;
      int right0 = LAB_A + LAB_A_W + 2;
      int right1 = I_COL - 2;
      int lab_x = LAB_A;
      int lab_w = LAB_A_W;
      if (i == 2) {
        lab_x -= 2;
        lab_w += 4;
        right0 = lab_x + lab_w + 2;
      }
      lv_obj_t *lab = en_lang ? en_labs[i] : fa_labs[i];
      if (lab) {
        f4_wdt_feed();  // بازسازی پین‌مپ F3 فازِ سنگین مجاز است؛ مثل F4 بین ردیف‌ها فید شود
        lv_obj_update_layout(lab);
        const char *txt = lv_label_get_text(lab);
        if (txt && txt[0]) {
          const lv_font_t *f = en_lang ? (const lv_font_t *)&lv_font_montserrat_14
                                       : (const lv_font_t *)&ui_font_naz18b;
          lv_point_t sz = {0, 0};
          // بدون سقف عرض — عرض واقعی متن برای شکاف نقطه‌چین
          lv_text_get_size(&sz, txt, f, en_lang ? 0 : -1, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
          int tw = sz.x;
          if (tw > lab_w) tw = lab_w;
          if (tw < 8) tw = 8;
          int side = (lab_w - tw) / 2;
          if (side < 2) side = 2;
          left1 = lab_x + side - 2;
          right0 = lab_x + lab_w - side + 2;
        }
      }
      if (left1 < left0 + 8) left1 = left0 + 8;
      if (right1 < right0 + 8) right0 = right1 - 8;
      if (right0 < left1 + 4) right0 = left1 + 4;
      // سوییچ درب محافظ (ردیف ۲): دقیقاً ۳ نقطه چپ و ۳ نقطه راست
      const int n_force = (i == 2) ? 3 : 0;
      place_dot_bridge(dpar, &f3_dot_lbls[i * 2], left0, left1, cy, n_force);
      place_dot_bridge(dpar, &f3_dot_lbls[i * 2 + 1], right0, right1, cy, n_force);
    }
  }

  // ---- Q buttons 2x3 — کمی فشرده‌تر تا کپسول سه‌خطه جا شود ----
  const int QH = 54;
  const int QX0 = IP_X + IP_W + GAP_WB;   // ۱۹۴
  const int QX1 = QX0 + QW + GAP_QQ;      // ۲۹۲
  const int Q_GAP = 6;
  const int Q_PITCH = QH + Q_GAP;         // ۶۰
  const int IP_BOTTOM = IP_Y + IP_H;      // ۲۹۶
  const int QY2 = IP_BOTTOM - QH;         // ۲۴۲
  const int QY1 = QY2 - Q_PITCH;          // ۱۸۲
  const int QY0 = QY1 - Q_PITCH;          // ۱۲۲

  // کپسول: خط۱ فقط Q0=پالس ؛ خط۲ Q1=جهت و Q2=فعال سازی کنار هم
  {
    const int CAP_GAP_TOP = 2;
    const int CAP_GAP_BOT = 7;  // +3: کپسول آبی از پایین ۳px کوتاه‌تر
    const int CAP_X = QX0;
    const int CAP_W = (QX1 + QW) - QX0;   // ۱۸۸
    const int CAP_Y = IP_Y + CAP_GAP_TOP;
    int CAP_H = QY0 - CAP_GAP_BOT - CAP_Y;  // ~۶۹
    if (CAP_H < 48) CAP_H = 48;
    if (CAP_H >= 40 && objects.obj14) {
      if (f3_q_cap && lv_obj_is_valid(f3_q_cap)) {
        lv_obj_delete(f3_q_cap);
        f3_q_cap = NULL;
        f3_q012_lit = NULL;
        f3_q_run_pulse_cap = NULL;
        f3_q_run_pulse_value = NULL;
        f3_q_run_time_cap = NULL;
        f3_q_run_time_value = NULL;
        f3_q_run_status_visible = false;
      }
      for (int i = 0; i < 3; ++i) {
        f3_q_step_hits[i] = NULL;
        f3_q_step_badges[i] = NULL;
        f3_q_step_labels[i] = NULL;
      }
      f4_wdt_feed();  // ساخت کپسول Q0..Q2 نیز بخشی از همان فاز سنگین بازسازی F3 است
      f3_q_cap = lv_obj_create(objects.obj14);
      lv_obj_set_pos(f3_q_cap, CAP_X, CAP_Y);
      lv_obj_set_size(f3_q_cap, CAP_W, CAP_H);
      lv_obj_set_style_radius(f3_q_cap, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(f3_q_cap, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(f3_q_cap, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(f3_q_cap, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(f3_q_cap, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(f3_q_cap, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_clip_corner(f3_q_cap, false, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_clear_flag(f3_q_cap, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(f3_q_cap, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(f3_q_cap, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
      lv_obj_set_style_base_dir(f3_q_cap, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(f3_q_cap, lv_color_hex(0x0066cc), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_event_cb(f3_q_cap, f3_q012_short_cb, LV_EVENT_CLICKED, NULL);
#if F3_PIN_HELP_ENABLE
      lv_obj_add_event_cb(f3_q_cap, f3_hold_ev_cb, LV_EVENT_PRESSED, (void *)(intptr_t)F3H_STEPPER_PARAMS);
      lv_obj_add_event_cb(f3_q_cap, f3_hold_ev_cb, LV_EVENT_PRESSING, (void *)(intptr_t)F3H_STEPPER_PARAMS);
      lv_obj_add_event_cb(f3_q_cap, f3_hold_ev_cb, LV_EVENT_RELEASED, (void *)(intptr_t)F3H_STEPPER_PARAMS);
      lv_obj_add_event_cb(f3_q_cap, f3_hold_ev_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)F3H_STEPPER_PARAMS);
#endif

      const bool en = (GUI.system_language == 2);
      f3_q_run_pulse_cap = lv_label_create(f3_q_cap);
      f3_q_run_pulse_value = lv_label_create(f3_q_cap);
      f3_q_run_time_cap = lv_label_create(f3_q_cap);
      f3_q_run_time_value = lv_label_create(f3_q_cap);
      if (f3_q_run_pulse_cap && f3_q_run_pulse_value &&
          f3_q_run_time_cap && f3_q_run_time_value) {
        lv_obj_t *caps[] = {f3_q_run_pulse_cap, f3_q_run_time_cap};
        const char *cap_text[] = {en ? "Pulses" : "شمارش پالس",
                                  en ? "Time left" : "زمان باقی"};
        for (int i = 0; i < 2; ++i) {
          lv_obj_set_pos(caps[i], 12, 6 + i * 31);
          lv_obj_set_width(caps[i], 102);
          lv_obj_set_style_text_font(caps[i], en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
          lv_obj_set_style_text_color(caps[i], lv_color_hex(0xffffff), 0);
          lv_obj_set_style_text_align(caps[i], en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
          lv_obj_set_style_base_dir(caps[i], en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
          lv_label_set_text(caps[i], cap_text[i]);
          lv_obj_add_flag(caps[i], LV_OBJ_FLAG_HIDDEN);
          lv_obj_clear_flag(caps[i], LV_OBJ_FLAG_CLICKABLE);
        }
        lv_obj_t *values[] = {f3_q_run_pulse_value, f3_q_run_time_value};
        for (int i = 0; i < 2; ++i) {
          lv_obj_set_pos(values[i], 120, 8 + i * 31);
          lv_obj_set_width(values[i], 56);
          lv_obj_set_style_text_font(values[i], &lv_font_montserrat_14, 0);
          lv_obj_set_style_text_color(values[i], lv_color_hex(0xffffff), 0);
          lv_obj_set_style_text_align(values[i], LV_TEXT_ALIGN_RIGHT, 0);
          lv_obj_set_style_base_dir(values[i], LV_BASE_DIR_LTR, 0);
          lv_label_set_text(values[i], i == 0 ? "0" : "00:00");
          lv_obj_add_flag(values[i], LV_OBJ_FLAG_HIDDEN);
          lv_obj_clear_flag(values[i], LV_OBJ_FLAG_CLICKABLE);
        }
      }
      f3_q_run_status_visible = false;
      const int PAD_X = 6;
      const int PAD_Y = 4;
      const int ROW_H = (CAP_H - 2 * PAD_Y) / 2;
      int BD = ROW_H - 6;
      if (BD < 16) BD = 16;
      if (BD > 22) BD = 22;
      const int GAP = 4;

      // item: clickable hit + badge + label (badge_dx / lab_dx جدا)
      auto place_cap_item = [&](const char *tag, const char *txt, int x0, int row_top,
                                int lab_w, int badge_dx, int lab_dx, int role, int help_id) {
        int badge_y = row_top + (ROW_H - BD) / 2;
        badge_y += (role == 0) ? 3 : 2;  // Q0: ۳px پایین؛ Q1/Q2: ۲px پایین
        const int badge_cy = badge_y + BD / 2;
        int badge_x = x0 + badge_dx;
        int lab_x = x0 + BD + GAP + lab_dx;
        int hit_x = badge_x;
        if (lab_x < hit_x) hit_x = lab_x;
        int hit_r = lab_x + lab_w;
        if (hit_r < badge_x + BD) hit_r = badge_x + BD;
        hit_r += 2;
        if (hit_x < 0) hit_x = 0;

        lv_obj_t *hit = lv_obj_create(f3_q_cap);
        if (!hit) return;
        lv_obj_set_pos(hit, hit_x, row_top);
        lv_obj_set_size(hit, hit_r - hit_x, ROW_H);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(hit, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(hit, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(hit, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(hit, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        if (role >= 0 && role < 3) f3_q_step_hits[role] = hit;
#if F3_PIN_HELP_ENABLE
        f3_pin_help_bind_target(hit, help_id);
#endif
        lv_obj_add_event_cb(hit, f3_q012_short_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *dot = f3_make_pin_lbl(hit, tag, BD, true);
        if (!dot) return;
        lv_obj_set_style_border_width(dot, 1, 0);
        lv_obj_set_style_outline_width(dot, 0, 0);
        {
          uint32_t cn = lv_obj_get_child_count(dot);
          for (uint32_t ci = 0; ci < cn; ci++) {
            lv_obj_t *ch = lv_obj_get_child(dot, ci);
            if (!ch || !lv_obj_check_type(ch, &lv_label_class)) continue;
            lv_obj_set_style_text_font(ch, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_letter_space(ch, -1, 0);
            lv_obj_center(ch);
          }
        }
        lv_obj_set_pos(dot, badge_x - hit_x, badge_y - row_top);
        if (role >= 0 && role < 3) f3_q_step_badges[role] = dot;

        lv_obj_t *lab = lv_label_create(hit);
        lv_label_set_text(lab, txt);
        lv_obj_set_style_text_color(lab, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(lab, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(lab, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (en) {
          lv_obj_set_style_text_font(lab, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
          lv_obj_set_style_text_font(lab, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_set_width(lab, lab_w);
        lv_obj_set_height(lab, LV_SIZE_CONTENT);
        lv_label_set_long_mode(lab, LV_LABEL_LONG_CLIP);
        lv_obj_clear_flag(lab, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_update_layout(lab);
        int lab_h = (int)lv_obj_get_height(lab);
        if (lab_h < 1) lab_h = en ? 14 : 18;
        lv_obj_set_pos(lab, lab_x - hit_x, badge_cy - row_top - lab_h / 2);
        if (role >= 0 && role < 3) {
          f3_q_step_labels[role] = lab;
          lv_obj_add_event_cb(hit, f3_q_step_touch_hl_cb, LV_EVENT_PRESSED, (void *)(intptr_t)role);
          lv_obj_add_event_cb(hit, f3_q_step_touch_hl_cb, LV_EVENT_RELEASED, (void *)(intptr_t)role);
          lv_obj_add_event_cb(hit, f3_q_step_touch_hl_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)role);
        }

        lv_obj_move_foreground(dot);
        lv_obj_move_foreground(lab);
      };

      const char *fa0 = "پالس";
      const char *fa1 = "جهت";
      const char *fa2 = "فعال سازی";
      const char *en0 = "Pulse";
      const char *en1 = "Dir";
      const char *en2 = "Enable";
      char step_tag[3][4];
      for (int role = 0; role < 3; ++role) {
        const uint8_t src = pinmap_out_get_pending_source(role);
        if (src <= 9) snprintf(step_tag[role], sizeof(step_tag[role]), "Q%u", (unsigned)src);
        else snprintf(step_tag[role], sizeof(step_tag[role]), "X");
      }

      // row1 Q0: نوشته پالس +۱۰ راست (قبلاً +۷)
      {
        const int row0 = PAD_Y;
        const int lab_w0 = en ? 48 : 56;
        const int block_w0 = BD + GAP + lab_w0;
        const int x0 = (CAP_W - block_w0) / 2;
        place_cap_item(step_tag[0], en ? en0 : fa0, x0, row0, lab_w0,
                       17, 5 + 7 + 3, 0, F3H_Q0);
      }

      // row2: نوشته جهت −۵ چپ؛ نوشته Q1 −۳ چپ
      {
        const int row1 = PAD_Y + ROW_H;
        const int cell = (CAP_W - 2 * PAD_X) / 2;
        const int lab_w1 = en ? 36 : 40;
        const int lab_w2 = en ? 52 : 72;
        const int bw1 = BD + GAP + lab_w1;
        const int bw2 = BD + GAP + lab_w2;
        int x1 = PAD_X + (cell - bw1) / 2;
        int x2 = PAD_X + cell + (cell - bw2) / 2;
        x1 += 4;
        x2 -= 4;
        x1 -= 8;
        x2 -= 4;
        x1 -= 12;
        x1 -= 3;  // ver72
        x2 += 3;  // ver75: Q2 راست‌تر — فاصله از Q1 (درخواست مالک)
        x1 -= 3;  // ver73
        x1 -= 3;  // ver74
        if (x1 < PAD_X) x1 = PAD_X;
        if (x2 + bw2 > CAP_W - PAD_X) x2 = CAP_W - PAD_X - bw2;
        if (x2 < x1 + bw1 + 10) x2 = x1 + bw1 + 10;  // حداقل ۱۰px فاصله؛ بیرون از کپسول نمی‌رود
        place_cap_item(step_tag[1], en ? en1 : fa1, x1, row1, lab_w1,
                       -3, -5, 1, F3H_Q1);
        place_cap_item(step_tag[2], en ? en2 : fa2, x2 - 8, row1, lab_w2,
                       15, 13, 2, F3H_Q2);  // فعال‌سازی+Q2: ۶px بیشتر به چپ
      }

      lv_obj_move_foreground(f3_q_cap);
    }
  }

  struct { lv_obj_t *btn; int x; int y; } qpos[] = {
    {objects.btn2_3_3, QX0, QY0},  // Q6 saw
    {objects.btn2_3_2, QX1, QY0},  // Q3 holder
    {objects.btn2_3_5, QX0, QY1},  // Q8 lift (جک تیغه) — ردیف وسط
    {objects.btn2_3_4, QX1, QY1},  // Q4 guard
    {objects.btn2_3_1, QX1, QY2},  // Q5 spray
    {objects.btn2_3_6, QX0, QY2},  // Q7 angle (زاویه تیغه) — ردیف پایین
  };
  for (unsigned i = 0; i < 6; i++) {
    if (!qpos[i].btn) continue;
    lv_obj_set_pos(qpos[i].btn, qpos[i].x, qpos[i].y);
    lv_obj_set_size(qpos[i].btn, QW, QH);
    lv_obj_set_style_radius(qpos[i].btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(qpos[i].btn, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(qpos[i].btn, lv_color_hex(0x34c759), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(qpos[i].btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);  // جای دقیق بج در گوشه
    lv_obj_set_style_shadow_width(qpos[i].btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(qpos[i].btn);  // همیشه روی کارت سفید برای hit-test پایدار
  }

  // FA/EN: فونت همان (naz24 / montserrat_16)؛ فقط عرض متناسب کادر ۹۰px
  // جک‌های دوخطه: line_sp −۱۲ → «جک» ≈۳px پایین، خط دوم ≈۳px بالا (نسبت به −۶ قبلی)
  const int Q_TXT_W = QW - 10;  // ۸۰
  f3_style_q_btn_label(objects.lablef2_14, true, 3, 0, -4, 0);
  f3_style_q_btn_label(objects.lablee2_14, false, 8, 2, 0, 0);          // Spray
  f3_style_q_btn_label(objects.lablef2_13, true, 3, 0, -12, Q_TXT_W);    // جک / نگهدارنده
  f3_style_q_btn_label(objects.lablee2_13, false, 10, 4, -2, Q_TXT_W);    // Holder Jack
  f3_style_q_btn_label(objects.lablef2_16, true, 3, 0, -4, 0);
  f3_style_q_btn_label(objects.lablee2_16, false, 4, 2, 0, 0);          // Saw
  f3_style_q_btn_label(objects.lablef2_15, true, 3, 0, -12, Q_TXT_W);    // جک / محافظ
  f3_style_q_btn_label(objects.lablee2_15, false, 10, 4, -2, Q_TXT_W);    // Guard Jack
  f3_style_q_btn_label(objects.lablef2_18, true, 3, 0, -12, Q_TXT_W);    // جک / تیغه
  if (objects.lablee2_18) lv_label_set_text(objects.lablee2_18, "Lift\nSaw");
  f3_style_q_btn_label(objects.lablee2_18, false, 6, 4, -2, Q_TXT_W);
  if (objects.lablef2_17) lv_label_set_text(objects.lablef2_17, "زاویه تیغه");
  if (objects.lablee2_17) lv_label_set_text(objects.lablee2_17, "Blade Angle");
  f3_style_q_btn_label(objects.lablef2_17, true, 0, 8, 0, Q_TXT_W);   // متن زاویه تیغه در پایین
  f3_style_q_btn_label(objects.lablee2_17, false, 0, 8, 0, Q_TXT_W);  // Blade Angle در پایین

  if (!f3_angle_val_lbl || !lv_obj_is_valid(f3_angle_val_lbl)) {
    f3_angle_val_lbl = lv_label_create(objects.btn2_3_6);
  }
  lv_obj_set_style_text_font(f3_angle_val_lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(f3_angle_val_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(f3_angle_val_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  const char *cur_ang_str = (f3_blade_angle_mode == 0) ? "45°" : ((f3_blade_angle_mode == 1) ? "-45°" : "90°");
  lv_label_set_text(f3_angle_val_lbl, cur_ang_str);
  lv_obj_align(f3_angle_val_lbl, LV_ALIGN_CENTER, 26, -12);  // عدد زاویه در بالا
  lv_obj_move_foreground(f3_angle_val_lbl);

  // بج Q متناسب کادر (−۱۰٪)؛ فونت داخل دایره همان montserrat_12
  const int Q_BADGE = 25;  // ۲۸ × ۰٫۹
  struct { lv_obj_t *btn; int role; } qo[] = {
    {objects.btn2_3_1, 5},
    {objects.btn2_3_2, 3},
    {objects.btn2_3_3, 6},
    {objects.btn2_3_4, 4},
    {objects.btn2_3_5, 7},  // جک تیغه -> Q7
    {objects.btn2_3_6, 8},  // زاویه برش -> Q8, Q9
  };
  char qmap_tag[6][8];
  for (unsigned i = 0; i < 6; i++) {
    if (!qo[i].btn) continue;
    if (qo[i].role == 8) {
      f3_pin_lbls[i] = f3_make_pin_lbl(qo[i].btn, "Q8", 22, true);
      lv_obj_set_style_border_width(f3_pin_lbls[i], 1, 0);
      lv_obj_set_style_outline_width(f3_pin_lbls[i], 0, 0);
      lv_obj_align(f3_pin_lbls[i], LV_ALIGN_TOP_LEFT, 2, 2);
      lv_obj_move_foreground(f3_pin_lbls[i]);

      f3_pin_q9_lbl = f3_make_pin_lbl(qo[i].btn, "Q9", 22, true);
      lv_obj_set_style_border_width(f3_pin_q9_lbl, 1, 0);
      lv_obj_set_style_outline_width(f3_pin_q9_lbl, 0, 0);
      lv_obj_align(f3_pin_q9_lbl, LV_ALIGN_TOP_LEFT, 26, 2);
      lv_obj_move_foreground(f3_pin_q9_lbl);
    } else {
      const uint8_t src = pinmap_out_get_pending_source(qo[i].role);
      if (src <= 9) snprintf(qmap_tag[i], sizeof(qmap_tag[i]), "Q%u", (unsigned)src);
      else snprintf(qmap_tag[i], sizeof(qmap_tag[i]), "X");
      f3_pin_lbls[i] = f3_make_pin_lbl(qo[i].btn, qmap_tag[i], Q_BADGE, true);
      lv_obj_set_style_border_width(f3_pin_lbls[i], 1, 0);
      lv_obj_set_style_outline_width(f3_pin_lbls[i], 0, 0);
      lv_obj_align(f3_pin_lbls[i], LV_ALIGN_TOP_LEFT, 2, 2);
      lv_obj_move_foreground(f3_pin_lbls[i]);
    }
  }

  // I badges - همه در یک ستون تراز (مثل ۴ خط اول)
  lv_obj_t *par = ipanel ? ipanel : objects.obj14;
  struct { int x; int y; } iabs[] = {
    {I_COL, ROW0}, {I_COL, ROW1}, {I_COL, ROW2}, {I_COL, ROW3},
    {I_COL, ROW4}, {I_COL, ROW5}, {I_COL, ROW6}, {I_COL, ROW7},
  };
  char imap_tag[8][4];
  for (unsigned i = 0; i < 8; i++) {
    const uint8_t src = pinmap_in_get_pending_source((int)i);
    f3_input_source_tag(src, imap_tag[i], sizeof(imap_tag[i]));
    f3_pin_lbls[6 + i] = f3_make_pin_lbl(par, imap_tag[i], I_BADGE, false);
    if (!f3_pin_lbls[6 + i]) continue;
    lv_obj_set_pos(f3_pin_lbls[6 + i], iabs[i].x, iabs[i].y);
    lv_obj_move_foreground(f3_pin_lbls[6 + i]);
  }

#if F3_PIN_HELP_ENABLE
  f3_pin_help_wire_after_reflow();
#endif

  f3_pin_wired = true;
  f3_layout_ver = F3_LAYOUT_VER;
  f3_pending_revision_seen = pinmap_pending_revision;
  f3_language_seen = GUI.system_language;
  f3_joy_enabled_seen = GUI.joystick_enable;
}


