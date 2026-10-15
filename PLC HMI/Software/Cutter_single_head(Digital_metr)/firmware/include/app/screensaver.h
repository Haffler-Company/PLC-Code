#pragma once

// اسکرین سیور سبک Cutter، برگرفته از سناریوی Welding_single_head.
// فقط Task2 این توابع را صدا می زند؛ هیچ تماس LVGL از تسک دیگر انجام نمی شود.
static constexpr uint32_t CUTTER_SS_IDLE_OPTIONS_MS[7] = {
  60000UL, 600000UL, 1800000UL, 3600000UL, 7200000UL, 21600000UL, 43200000UL
};
static lv_obj_t *cutter_ss_root = NULL;
static lv_obj_t *cutter_ss_ball = NULL;
static lv_obj_t *cutter_ss_paddle = NULL;
static lv_obj_t *cutter_ss_clock = NULL;
static lv_obj_t *cutter_ss_zone_exit = NULL;
static lv_obj_t *cutter_ss_zone_play = NULL;
static uint32_t cutter_ss_last_activity_ms = 0;
static uint32_t cutter_ss_last_frame_ms = 0;
static uint32_t cutter_ss_last_clock_ms = 0;
static int16_t cutter_ss_ball_x = 221;
static int16_t cutter_ss_ball_y = 28;
static int8_t cutter_ss_vx = 4;
static int8_t cutter_ss_vy = 4;
static int16_t cutter_ss_paddle_x = 175;
static bool cutter_ss_dragging = false;
static bool cutter_ss_consume_panel_release = false;
static bool cutter_ss_touch_consumed = false;
static bool cutter_ss_next_hit_right = true;
static lv_obj_t *cutter_ss_settings_root = NULL;

static inline bool cutter_screensaver_active() {
  return cutter_ss_root && lv_obj_is_valid(cutter_ss_root);
}

static void cutter_ss_destroy_async() {
  if (cutter_ss_root && lv_obj_is_valid(cutter_ss_root)) lv_obj_delete_async(cutter_ss_root);
  cutter_ss_root = cutter_ss_ball = cutter_ss_paddle = cutter_ss_clock = NULL;
  cutter_ss_zone_exit = cutter_ss_zone_play = NULL;
  cutter_ss_dragging = false;
  cutter_ss_touch_consumed = false;
  cutter_ss_last_activity_ms = millis();
}

static void cutter_ss_settings_close() {
  if (cutter_ss_settings_root && lv_obj_is_valid(cutter_ss_settings_root))
    lv_obj_delete_async(cutter_ss_settings_root);
  cutter_ss_settings_root = NULL;
}

static void cutter_ss_settings_select_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  screensaver_timeout_mode = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  save_screensaver_timeout_mode();
  cutter_ss_last_activity_ms = millis();
  GUI.buzzer_cmd_short = true;
  cutter_ss_settings_close();
}

static void cutter_ss_settings_open() {
  if (cutter_ss_settings_root && lv_obj_is_valid(cutter_ss_settings_root)) return;
  const bool en = GUI.system_language == 2;
  cutter_ss_settings_root = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(cutter_ss_settings_root, 0, 0);
  lv_obj_set_size(cutter_ss_settings_root, 480, 320);
  lv_obj_set_style_bg_color(cutter_ss_settings_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(cutter_ss_settings_root, LV_OPA_60, 0);
  lv_obj_set_style_border_width(cutter_ss_settings_root, 0, 0);
  lv_obj_set_style_pad_all(cutter_ss_settings_root, 0, 0);
  lv_obj_clear_flag(cutter_ss_settings_root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *card = lv_obj_create(cutter_ss_settings_root);
  lv_obj_set_pos(card, 12, 12);
  lv_obj_set_size(card, 456, 296);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xF7F8FA), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x0A84FF), 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_width(title, 420);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x1C1C1E), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(title, en ? "Screen saver delay" : "زمان اسکرین سیور");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *hint = lv_label_create(card);
  lv_obj_set_width(hint, 420);
  lv_obj_set_style_text_font(hint, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x636366), 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(hint, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(hint, en ? "Choose idle time before sleep" : "زمان بی کاری تا حالت استراحت را انتخاب کنید");
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 39);

  static const char *fa[8] = {"1 دقیقه", "10 دقیقه", "30 دقیقه", "1 ساعت", "2 ساعت", "6 ساعت", "12 ساعت", "اصلا نباشد"};
  static const char *en_txt[8] = {"1 min", "10 min", "30 min", "1 hour", "2 hours", "6 hours", "12 hours", "Off"};
  for (int i = 0; i < 8; ++i) {
    const int col = i & 1, row = i >> 1;
    lv_obj_t *btn = lv_button_create(card);
    lv_obj_set_pos(btn, 18 + col * 211, 68 + row * 51);
    lv_obj_set_size(btn, 209, 44);
    const bool selected = screensaver_timeout_mode == i;
    lv_obj_set_style_bg_color(btn, lv_color_hex(selected ? 0x0A84FF : 0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(selected ? 0x0A84FF : 0xD1D1D6), 0);
    lv_obj_set_style_border_width(btn, selected ? 2 : 1, 0);
    lv_obj_set_style_radius(btn, 11, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(btn, cutter_ss_settings_select_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, en ? &lv_font_montserrat_14 : &ui_font_naz20b, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(selected ? 0xFFFFFF : 0x1C1C1E), 0);
    lv_label_set_text(lbl, en ? en_txt[i] : fa[i]);
    lv_obj_center(lbl);
  }
}

static void cutter_ss_exit_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  // لمس اسکرین‌سیور نباید به کنترل یا راهنمای صفحهٔ زیرین نشت کند.
  lv_event_stop_bubbling(e);
  if (code == LV_EVENT_LONG_PRESSED) {
    cutter_ss_touch_consumed = true;
    ui_help_suppress_release = true;
    GUI.buzzer_cmd_short = true;
    cutter_ss_destroy_async();
    cutter_ss_settings_open();
  } else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && !cutter_ss_touch_consumed) {
    ui_help_suppress_release = true;
    GUI.buzzer_cmd_short = true;
    cutter_ss_destroy_async();
  }
}

static int16_t cutter_ss_touch_x() {
  lv_indev_t *in = lv_indev_active();
  if (!in) return 240;
  lv_point_t p;
  lv_indev_get_point(in, &p);
  return (int16_t)p.x;
}

static void cutter_ss_set_paddle_from_touch() {
  int16_t x = cutter_ss_touch_x() - 65;
  if (x < 0) x = 0;
  if (x > 350) x = 350;
  cutter_ss_paddle_x = x;
  if (cutter_ss_paddle) lv_obj_set_x(cutter_ss_paddle, cutter_ss_paddle_x);
}

static void cutter_ss_play_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    cutter_ss_dragging = true;
    cutter_ss_set_paddle_from_touch();
  } else if (code == LV_EVENT_PRESSING) {
    if (cutter_ss_dragging) cutter_ss_set_paddle_from_touch();
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    cutter_ss_dragging = false;
  }
}

static void cutter_ss_zone_style(lv_obj_t *obj) {
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_radius(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void cutter_ss_update_clock() {
  if (!cutter_ss_clock) return;
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  if (!ds3231_read_time(&y, &m, &d, &hh, &mi, &ss)) {
    const int64_t ep = soft_now_epoch();
    if (ep > 0) epoch_to_civil(ep, &y, &m, &d, &hh, &mi, &ss);
  }
  char b[8];
  if (y > 0) snprintf(b, sizeof(b), "%02d:%02d", hh, mi);
  else snprintf(b, sizeof(b), "--:--");
  lv_label_set_text(cutter_ss_clock, b);
}

static void cutter_ss_build() {
  if (cutter_screensaver_active()) return;
  cutter_ss_root = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(cutter_ss_root, 0, 0);
  lv_obj_set_size(cutter_ss_root, 480, 320);
  lv_obj_set_style_bg_color(cutter_ss_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(cutter_ss_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cutter_ss_root, 0, 0);
  lv_obj_set_style_radius(cutter_ss_root, 0, 0);
  lv_obj_set_style_pad_all(cutter_ss_root, 0, 0);
  lv_obj_clear_flag(cutter_ss_root, LV_OBJ_FLAG_SCROLLABLE);

  cutter_ss_zone_exit = lv_obj_create(cutter_ss_root);
  lv_obj_set_pos(cutter_ss_zone_exit, 0, 0);
  lv_obj_set_size(cutter_ss_zone_exit, 480, 220);
  cutter_ss_zone_style(cutter_ss_zone_exit);
  lv_obj_add_event_cb(cutter_ss_zone_exit, cutter_ss_exit_cb, LV_EVENT_ALL, NULL);

  cutter_ss_zone_play = lv_obj_create(cutter_ss_root);
  lv_obj_set_pos(cutter_ss_zone_play, 0, 220);
  lv_obj_set_size(cutter_ss_zone_play, 480, 100);
  cutter_ss_zone_style(cutter_ss_zone_play);
  lv_obj_add_event_cb(cutter_ss_zone_play, cutter_ss_exit_cb, LV_EVENT_ALL, NULL);

  cutter_ss_ball_x = 221;
  cutter_ss_ball_y = 28;
  cutter_ss_vx = 4;
  cutter_ss_vy = 4;
  cutter_ss_next_hit_right = true;
  cutter_ss_ball = lv_obj_create(cutter_ss_root);
  lv_obj_set_pos(cutter_ss_ball, cutter_ss_ball_x, cutter_ss_ball_y);
  lv_obj_set_size(cutter_ss_ball, 38, 38);
  lv_obj_set_style_bg_opa(cutter_ss_ball, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cutter_ss_ball, 0, 0);
  lv_obj_set_style_pad_all(cutter_ss_ball, 0, 0);
  lv_obj_clear_flag(cutter_ss_ball, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
  for (int i = 0; i < 4; i++) {
    lv_obj_t *pane = lv_obj_create(cutter_ss_ball);
    lv_obj_set_pos(pane, 2 + (i & 1) * 19, 2 + (i >> 1) * 19);
    lv_obj_set_size(pane, 16, 16);
    lv_obj_set_style_bg_color(pane, lv_color_hex(0x5AA9E6), 0);
    lv_obj_set_style_bg_opa(pane, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pane, 0, 0);
    lv_obj_set_style_radius(pane, 6, 0);
    lv_obj_clear_flag(pane, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
  }

  cutter_ss_paddle_x = 175;
  cutter_ss_paddle = lv_obj_create(cutter_ss_root);
  lv_obj_set_pos(cutter_ss_paddle, cutter_ss_paddle_x, 230);
  lv_obj_set_size(cutter_ss_paddle, 130, 46);
  lv_obj_set_style_bg_color(cutter_ss_paddle, lv_color_hex(0x263238), 0);
  lv_obj_set_style_bg_opa(cutter_ss_paddle, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(cutter_ss_paddle, lv_color_hex(0x5AA9E6), 0);
  lv_obj_set_style_border_width(cutter_ss_paddle, 2, 0);
  lv_obj_set_style_radius(cutter_ss_paddle, 9, 0);
  lv_obj_set_style_pad_all(cutter_ss_paddle, 0, 0);
  lv_obj_clear_flag(cutter_ss_paddle, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

  cutter_ss_clock = lv_label_create(cutter_ss_paddle);
  lv_obj_set_style_text_font(cutter_ss_clock, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(cutter_ss_clock, lv_color_hex(0xCFD8DC), 0);
  lv_obj_center(cutter_ss_clock);
  cutter_ss_update_clock();

  lv_obj_t *caption = lv_label_create(cutter_ss_root);
  lv_obj_set_width(caption, 440);
  lv_obj_set_style_text_font(caption,
    GUI.system_language == 2 ? (const lv_font_t *)&lv_font_montserrat_14
                             : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(caption, lv_color_hex(0x607D8B), 0);
  lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(caption,
    GUI.system_language == 2 ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(caption,
    GUI.system_language == 2 ? "Standing by - touch upper area to return"
                             : "حالت استراحت  برای بازگشت قسمت بالا را لمس کنید");
  lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -2);

  lv_obj_move_foreground(cutter_ss_ball);
  lv_obj_move_foreground(cutter_ss_paddle);
  lv_obj_move_foreground(caption);
  lv_obj_move_foreground(cutter_ss_zone_exit);
  lv_obj_move_foreground(cutter_ss_zone_play);
  // توپ و تخته باید روی ناحیه های شفاف دیده شوند؛ zone ها فقط hit target هستند.
  lv_obj_move_foreground(cutter_ss_ball);
  lv_obj_move_foreground(cutter_ss_paddle);
  lv_obj_move_foreground(caption);
  cutter_ss_last_frame_ms = millis();
  cutter_ss_last_clock_ms = 0;
}

static bool cutter_ss_allowed_now() {
  if (GUI.scr_id != 1 || GUI.ui_screen_settle > 0 || GUI.lock_screen) return false;
  if (STP.motorIs_moving || STP.moveStepperCMD || STP.flag_is_cutting || STP.begin_calibration) return false;
  if (IOS.stop_button_state || g_update_mode) return false;
  if (IOS.saw_is_on || IOS.clamp_engaged || IOS.cut_confirm_pending) return false;
  return true;
}

static void cutter_screensaver_tick() {
  const uint32_t now = millis();
  if (cutter_ss_last_activity_ms == 0) cutter_ss_last_activity_ms = now;

  // فشاری که فقط برای بیدارکردن بوده، بعد از رهاشدن نباید حافظه F را اجرا کند.
  if (!cutter_screensaver_active() && cutter_ss_consume_panel_release) {
    cutter_ss_last_activity_ms = now;
    if (panel_f_press_edge) {
      panel_f_press_edge = 0;
      cutter_ss_consume_panel_release = false;
    } else if (!panel_f_held) {
      cutter_ss_consume_panel_release = false;
    }
    return;
  }

  bool touching = false;
  for (lv_indev_t *in = lv_indev_get_next(NULL); in; in = lv_indev_get_next(in)) {
    if (lv_indev_get_type(in) == LV_INDEV_TYPE_POINTER &&
        lv_indev_get_state(in) == LV_INDEV_STATE_PRESSED) {
      touching = true;
      break;
    }
  }

  if (!cutter_screensaver_active()) {
    if (touching || panel_f_held || panel_f_press_edge || !cutter_ss_allowed_now()) {
      cutter_ss_last_activity_ms = now;
      return;
    }
    if (screensaver_timeout_mode >= 7) return;
    if ((uint32_t)(now - cutter_ss_last_activity_ms) >= CUTTER_SS_IDLE_OPTIONS_MS[screensaver_timeout_mode])
      cutter_ss_build();
    return;
  }

  if (!cutter_ss_allowed_now()) {
    cutter_ss_destroy_async();
    return;
  }
  if (panel_f_held || panel_f_press_edge || GUI.flag_panel_nav_exit_key) {
    cutter_ss_consume_panel_release = panel_f_held || panel_f_press_edge;
    panel_f_press_edge = 0;
    GUI.flag_panel_nav_exit_key = false;
    cutter_ss_destroy_async();
    return;
  }
  if ((uint32_t)(now - cutter_ss_last_clock_ms) >= 1000UL) {
    cutter_ss_last_clock_ms = now;
    cutter_ss_update_clock();
  }
  if ((uint32_t)(now - cutter_ss_last_frame_ms) < 33UL) return;
  cutter_ss_last_frame_ms = now;

  if (!cutter_ss_dragging) {
    const int ball_center = cutter_ss_ball_x + 19;
    // ساعت را یک‌بار از گوشه راست و دفعه بعد از گوشه چپ بزن.
    const int desired_hit = cutter_ss_next_hit_right ? 45 : -45;
    const int paddle_target = constrain(ball_center - 65 - desired_hit, 0, 350);
    if (paddle_target > cutter_ss_paddle_x + 3) cutter_ss_paddle_x += 4;
    else if (paddle_target < cutter_ss_paddle_x - 3) cutter_ss_paddle_x -= 4;
    if (cutter_ss_paddle_x < 0) cutter_ss_paddle_x = 0;
    if (cutter_ss_paddle_x > 350) cutter_ss_paddle_x = 350;
    lv_obj_set_x(cutter_ss_paddle, cutter_ss_paddle_x);
  }

  cutter_ss_ball_x += cutter_ss_vx;
  cutter_ss_ball_y += cutter_ss_vy;
  if (cutter_ss_ball_x <= 0 || cutter_ss_ball_x >= 442) {
    cutter_ss_ball_x = constrain(cutter_ss_ball_x, 0, 442);
    cutter_ss_vx = -cutter_ss_vx;
  }
  if (cutter_ss_ball_y <= 8) {
    cutter_ss_ball_y = 8;
    cutter_ss_vy = -cutter_ss_vy;
  }
  if (cutter_ss_vy > 0 && cutter_ss_ball_y + 38 >= 230 &&
      cutter_ss_ball_y <= 276 &&
      cutter_ss_ball_x + 38 >= cutter_ss_paddle_x &&
      cutter_ss_ball_x <= cutter_ss_paddle_x + 130) {
    cutter_ss_ball_y = 192;
    cutter_ss_vy = -cutter_ss_vy;
    int hit = (cutter_ss_ball_x + 19) - (cutter_ss_paddle_x + 65);
    // برخورد با گوشه های ساعت: زاویه تندتر و متنوع، تا توپ همه صفحه را ببیند.
    cutter_ss_vx = (int8_t)constrain(hit / 9, -7, 7);
    if (cutter_ss_vx >= -1 && cutter_ss_vx <= 1) cutter_ss_vx = (hit < 0) ? -4 : 4;
    cutter_ss_next_hit_right = !cutter_ss_next_hit_right;
  }
  if (cutter_ss_ball_y > 278) {
    cutter_ss_ball_x = 221;
    cutter_ss_ball_y = 28;
    cutter_ss_vy = 4;
  }
  lv_obj_set_pos(cutter_ss_ball, cutter_ss_ball_x, cutter_ss_ball_y);
}
