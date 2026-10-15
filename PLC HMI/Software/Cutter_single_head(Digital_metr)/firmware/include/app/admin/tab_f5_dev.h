#pragma once
// ===================== F5 developer =====================
// ver18: کارت +۵ پایین؛ نیم‌نوار بروزرسانی + نیم‌نوار گارانتی سبز
#define F5_LAYOUT_VER 27
#define F5_CONFIRM_MS 6000
static lv_obj_t *f5_root = NULL;
static lv_obj_t *f5_stats = NULL;
static lv_obj_t *f5_life_lbl = NULL;
static lv_obj_t *f5_user_lbl = NULL;
static lv_obj_t *f5_info_ver = NULL;
static lv_obj_t *f5_info_mfr = NULL;
static lv_obj_t *f5_info_temp = NULL;
static lv_obj_t *f5_info_clock = NULL;   // ساعت و تاریخ سیستم (زنده)
static lv_obj_t *f5_info_prog = NULL;    // تاریخ برنامه (بیلد)
static int f5_layout_ver = 0;
static lv_obj_t *f5_btn_update = NULL;
static lv_obj_t *f5_life_cap = NULL, *f5_user_cap = NULL;
static lv_obj_t *f5_cap_ver = NULL, *f5_cap_mfr = NULL, *f5_cap_temp = NULL, *f5_cap_clock = NULL;
static lv_obj_t *f5_cap_prog = NULL;
static lv_obj_t *f5_tile_user = NULL;
static lv_obj_t *f5_cap_heap = NULL, *f5_info_heap = NULL;
static lv_obj_t *f5_cap_up = NULL, *f5_info_up = NULL;
static lv_obj_t *f5_btn_warr = NULL;
static lv_obj_t *f5_warr_tile = NULL;
static lv_obj_t *f5_warr_cap = NULL;
static lv_obj_t *f5_warr_val = NULL;

static lv_obj_t *f5_cfm_root = NULL;
static lv_obj_t *f5_cfm_bar = NULL;
static uint32_t f5_cfm_t0 = 0;
static uint32_t f5_cfm_paused_accum = 0;
static uint32_t f5_cfm_press_at = 0;
static bool f5_cfm_holding = false;

static void f5_update_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  flag_start_update_flow = true;
}

// F5 tiles (overlay callbacks live in f5_report_ui.h)
static lv_obj_t *f5_tile_prog = NULL;
static lv_obj_t *f5_tile_ver = NULL;
static lv_obj_t *f5_tile_life = NULL;
static lv_obj_t *f5_tile_temp = NULL;
static lv_obj_t *f5_tile_rtc = NULL;
static lv_obj_t *f5_tile_warr = NULL;
static lv_obj_t *f5_tile_mfr = NULL;
static lv_obj_t *f5_tile_clock = NULL;
static lv_obj_t *f5_release_scroll = NULL;
static uint8_t f5_release_page = 0;
static constexpr uint8_t F5_RELEASE_PAGE_SIZE = 6;

static void f5_release_render();
static void f5_release_render_async(void *) { f5_release_render(); }

static void f5_release_page_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  const int dir = (int)(intptr_t)lv_event_get_user_data(e);
  const uint16_t total = 2U;
  const uint8_t pages = (uint8_t)((total + F5_RELEASE_PAGE_SIZE - 1U) / F5_RELEASE_PAGE_SIZE);
  if (dir < 0 && f5_release_page > 0) f5_release_page--;
  else if (dir > 0 && f5_release_page + 1U < pages) f5_release_page++;
  else { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  lv_async_call(f5_release_render_async, NULL);
}

static void f5_release_render() {
  if (!f5_release_scroll || !lv_obj_is_valid(f5_release_scroll)) return;
  lv_obj_clean(f5_release_scroll);
  const uint16_t total = 2U;
  const uint8_t pages = (uint8_t)((total + F5_RELEASE_PAGE_SIZE - 1U) / F5_RELEASE_PAGE_SIZE);
  if (f5_release_page >= pages) f5_release_page = pages - 1U;
  const uint16_t first = (uint16_t)f5_release_page * F5_RELEASE_PAGE_SIZE;
  const uint16_t end = min((uint16_t)(first + F5_RELEASE_PAGE_SIZE), total);
  for (uint16_t i = first; i < end; i++) {
    if (i == 0) {
      f5_cl_add_section(f5_release_scroll, FW_CHANGELOG_CUR_VER, FW_CHANGELOG_CUR_AT,
                        FW_CHANGELOG_CUR, FW_CHANGELOG_CUR_N, true);
    } else if (i == 1) {
      f5_cl_add_section(f5_release_scroll, FW_CHANGELOG_PREV_VER, FW_CHANGELOG_PREV_AT,
                        FW_CHANGELOG_PREV, FW_CHANGELOG_PREV_N, false);
    }
  }
  if (pages > 1U) {
  lv_obj_t *pager = lv_obj_create(f5_release_scroll);
  lv_obj_set_size(pager, 448, 46);
  lv_obj_set_style_bg_opa(pager, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(pager, 0, 0);
  lv_obj_set_style_pad_all(pager, 0, 0);
  lv_obj_clear_flag(pager, LV_OBJ_FLAG_SCROLLABLE);
  const bool en = (GUI.system_language == 2);
  auto mk = [&](lv_align_t align, const char *txt, int dir, bool enabled) {
    lv_obj_t *b = lv_button_create(pager);
    lv_obj_set_size(b, 116, 40);
    lv_obj_align(b, align, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(enabled ? 0x0A84FF : 0xC7C7CC), 0);
    lv_obj_set_style_radius(b, 10, 0);
    if (enabled) lv_obj_add_event_cb(b, f5_release_page_cb, LV_EVENT_RELEASED, (void *)(intptr_t)dir);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
  };
  mk(LV_ALIGN_LEFT_MID, en ? "Newer" : "جدیدتر", -1, f5_release_page > 0);
  mk(LV_ALIGN_RIGHT_MID, en ? "Older" : "قدیمی تر", 1, f5_release_page + 1U < pages);
  lv_obj_t *pl = lv_label_create(pager);
  lv_obj_set_style_text_font(pl, &lv_font_montserrat_16, 0);
  lv_label_set_text_fmt(pl, "%u / %u", (unsigned)(f5_release_page + 1U), (unsigned)pages);
  lv_obj_center(pl);
  }
  lv_obj_scroll_to_y(f5_release_scroll, 0, LV_ANIM_OFF);
}

void ui_resume_restore_release_notes() {
  f5_release_page = 0;
  f5_release_scroll = f5_ov_make_shell("اصلاحات برنامه", "Firmware changes");
  f5_ov_kind = F5_OV_KIND_RELEASE;
  f5_release_render();
  ui_resume_checkpoint();
}

static void f5_release_notes_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  ui_resume_restore_release_notes();
}

static void f5_warranty_sync_ui();
static void f5_warranty_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  open_warranty_screen();
}

static void f5_close_confirm(bool unlock) {
  if (f5_cfm_root && lv_obj_is_valid(f5_cfm_root)) lv_obj_delete(f5_cfm_root);
  f5_cfm_root = NULL;
  f5_cfm_bar = NULL;
  f5_cfm_holding = false;
  f5_cfm_press_at = 0;
  f5_cfm_paused_accum = 0;
  if (unlock) GUI.lock_screen = false;
}

static uint32_t f5_cfm_elapsed_ms() {
  uint32_t now = millis();
  uint32_t pause = f5_cfm_paused_accum;
  if (f5_cfm_holding && f5_cfm_press_at) {
    pause += (uint32_t)(now - f5_cfm_press_at);
  }
  uint32_t raw = (uint32_t)(now - f5_cfm_t0);
  return (raw > pause) ? (raw - pause) : 0;
}

static void f5_cfm_hold_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (!f5_cfm_root) return;
  if (c == LV_EVENT_PRESSED) {
    f5_cfm_holding = true;
    f5_cfm_press_at = millis();
  } else if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST) {
    if (f5_cfm_holding && f5_cfm_press_at) {
      f5_cfm_paused_accum += (uint32_t)(millis() - f5_cfm_press_at);
    }
    f5_cfm_holding = false;
    f5_cfm_press_at = 0;
  }
}

static void f5_refresh_info();
static void f5_refresh_cuts();

static void f5_cuts_yes_cb(lv_event_t *e) {
  (void)e;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) {
    f5_close_confirm(true);
    user_cuts = 0;
    save_user_cuts();
    f5_refresh_cuts();
    ui_toast("تعداد برش صفر شد", "Cut count cleared", 1600);
  }, NULL);
}
static void f5_cuts_no_cb(lv_event_t *e) {
  (void)e;
  GUI.buzzer_cmd_short = true;
  lv_async_call([](void *) { f5_close_confirm(true); }, NULL);
}

static void f5_user_reset_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (ui_help_take_suppress()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  if (f5_cfm_root) return;
  GUI.buzzer_cmd_short = true;

  const bool en = (GUI.system_language == 2);
  f5_cfm_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f5_cfm_root, 480, 320);
  lv_obj_set_pos(f5_cfm_root, 0, 0);
  lv_obj_set_style_bg_color(f5_cfm_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f5_cfm_root, LV_OPA_50, 0);
  lv_obj_set_style_border_width(f5_cfm_root, 0, 0);
  lv_obj_set_style_radius(f5_cfm_root, 0, 0);
  lv_obj_set_style_pad_all(f5_cfm_root, 0, 0);
  lv_obj_clear_flag(f5_cfm_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f5_cfm_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f5_cfm_root, f5_cfm_hold_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f5_cfm_root, f5_cfm_hold_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(f5_cfm_root, f5_cfm_hold_cb, LV_EVENT_PRESS_LOST, NULL);

  lv_obj_t *card = lv_obj_create(f5_cfm_root);
  lv_obj_set_size(card, 340, 168);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, f5_cfm_hold_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(card, f5_cfm_hold_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(card, f5_cfm_hold_cb, LV_EVENT_PRESS_LOST, NULL);

  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_width(body, 308);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0x1c1c1e), 0);
  if (!en) lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(body, en ? "Zero the cut count?" : "می‌خواهید تعداد برش را صفر کنید؟");
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 8);

  /* نوار زمان ۶ثانیه‌ای (خالی→پر می‌شود)؛ نگه داشتن انگشت تایمر را متوقف می‌کند */
  lv_obj_t *track = lv_obj_create(card);
  lv_obj_set_size(track, 308, 10);
  lv_obj_align(track, LV_ALIGN_TOP_MID, 0, 72);
  lv_obj_set_style_bg_color(track, lv_color_hex(0xe5e5ea), 0);
  lv_obj_set_style_radius(track, 5, 0);
  lv_obj_set_style_border_width(track, 0, 0);
  lv_obj_set_style_pad_all(track, 0, 0);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);

  f5_cfm_bar = lv_obj_create(track);
  lv_obj_set_size(f5_cfm_bar, 0, 10);
  lv_obj_set_pos(f5_cfm_bar, 0, 0);
  lv_obj_set_style_bg_color(f5_cfm_bar, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_radius(f5_cfm_bar, 5, 0);
  lv_obj_set_style_border_width(f5_cfm_bar, 0, 0);
  lv_obj_clear_flag(f5_cfm_bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(f5_cfm_bar, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *bn = lv_button_create(card);
  lv_obj_set_size(bn, 130, 40);
  lv_obj_align(bn, LV_ALIGN_BOTTOM_LEFT, 8, -4);
  lv_obj_set_style_bg_color(bn, lv_color_hex(0xff3b30), 0);
  lv_obj_set_style_radius(bn, 10, 0);
  lv_obj_add_event_cb(bn, f5_cuts_no_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ln = lv_label_create(bn);
  lv_obj_set_style_text_font(ln, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_obj_set_style_text_color(ln, lv_color_hex(0xffffff), 0);
  lv_label_set_text(ln, en ? "No" : "خیر");
  lv_obj_center(ln);

  lv_obj_t *by = lv_button_create(card);
  lv_obj_set_size(by, 130, 40);
  lv_obj_align(by, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
  lv_obj_set_style_bg_color(by, lv_color_hex(0x34c759), 0);
  lv_obj_set_style_radius(by, 10, 0);
  lv_obj_add_event_cb(by, f5_cuts_yes_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ly = lv_label_create(by);
  lv_obj_set_style_text_font(ly, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_obj_set_style_text_color(ly, lv_color_hex(0xffffff), 0);
  lv_label_set_text(ly, en ? "Confirm" : "بله");
  lv_obj_center(ly);

  f5_cfm_t0 = millis();
  f5_cfm_paused_accum = 0;
  f5_cfm_holding = false;
  f5_cfm_press_at = 0;
  GUI.lock_screen = true;
}

static void f5_confirm_tick() {
  if (!f5_cfm_root || !lv_obj_is_valid(f5_cfm_root)) return;
  uint32_t el = f5_cfm_elapsed_ms();
  if (f5_cfm_bar) {
    int w = (int)((el >= F5_CONFIRM_MS) ? 308 : (308UL * el) / F5_CONFIRM_MS);
    lv_obj_set_width(f5_cfm_bar, w);
  }
  if (el >= F5_CONFIRM_MS && !f5_cfm_holding) {
    f5_close_confirm(true);
  }
}

static inline void f5_set_text_if_changed(lv_obj_t *label, const char *text) {
  if (!label || !text) return;
  const char *old = lv_label_get_text(label);
  if (!old || strcmp(old, text) != 0) lv_label_set_text(label, text);
}

static int f5_caption_y(bool en) {
  return en ? 1 : -4;  // EN +7px, FA +2px نسبت به جای قبلی -6px
}

// زمان پروگرم/بیلد از __DATE__/__TIME__ (بدون ثانیه)
static bool f5_parse_build_stamp(int *y, int *m, int *d, int *hh, int *mi) {
  static const char mons[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mon[4] = {0};
  int day = 0, year = 0, h = 0, min = 0, sec = 0;
  if (sscanf(__DATE__, "%3s %d %d", mon, &day, &year) != 3) return false;
  if (sscanf(__TIME__, "%d:%d:%d", &h, &min, &sec) < 2) return false;
  const char *p = strstr(mons, mon);
  if (!p || ((p - mons) % 3) != 0) return false;
  *y = year;
  *m = (int)((p - mons) / 3) + 1;
  *d = day;
  *hh = h;
  *mi = min;
  return true;
}

// فارسی→شمسی، انگلیسی→میلادی؛ قالب: تاریخ / ساعت (بدون ثانیه)
static void f5_format_program_clock(char *b, size_t n, bool en) {
  int y = 0, m = 0, d = 0, hh = 0, mi = 0;
  if (!f5_parse_build_stamp(&y, &m, &d, &hh, &mi)) {
    snprintf(b, n, "%.11s / %.5s", __DATE__, __TIME__);  // fallback کوتاه
    return;
  }
  if (!en) {
    int jy = 0, jm = 0, jd = 0;
    gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
    snprintf(b, n, "%04d/%02d/%02d / %02d:%02d", jy, jm, jd, hh, mi);
  } else {
    snprintf(b, n, "%04d-%02d-%02d / %02d:%02d", y, m, d, hh, mi);
  }
}

static void f5_format_system_clock(char *b, size_t n, bool en) {
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  if (ds3231_read_time(&y, &m, &d, &hh, &mi, &ss)) {
    // همگام نرم‌افزاری برای بقیۀ سیستم
    soft_clock_set_civil(y, m, d, hh, mi, ss);
  } else {
    int64_t now = soft_now_epoch();
    if (now <= 0) {
      snprintf(b, n, "--");
      (void)en;
      return;
    }
    epoch_to_civil(now, &y, &m, &d, &hh, &mi, &ss);
  }
  if (!en) {
    int jy = 0, jm = 0, jd = 0;
    gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
    snprintf(b, n, "%04d/%02d/%02d / %02d:%02d", jy, jm, jd, hh, mi);
  } else {
    snprintf(b, n, "%04d-%02d-%02d / %02d:%02d", y, m, d, hh, mi);
  }
}

// عنوان سلول: همان جای فارسی (TOP_MID)؛ EN با montserrat_18 هم‌سایز naz18b
static void f5_set_cap(lv_obj_t *cap, const char *fa, const char *en_txt, bool en) {
  if (!cap) return;
  const lv_font_t *font = en ? (const lv_font_t *)&lv_font_montserrat_14
                             : (const lv_font_t *)&ui_font_naz18b;
  if (lv_obj_get_style_text_font(cap, LV_PART_MAIN) != font)
    lv_obj_set_style_text_font(cap, font, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(cap, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(cap, -4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_letter_space(cap, en ? 0 : -1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(cap, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_t *cell = lv_obj_get_parent(cap);
  (void)cell;
  lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, f5_caption_y(en));
  f5_set_text_if_changed(cap, en ? en_txt : fa);
}

static void f5_warranty_sync_ui() {
  const bool en = (GUI.system_language == 2);
  if (warranty_active) {
    if (f5_btn_warr) lv_obj_add_flag(f5_btn_warr, LV_OBJ_FLAG_HIDDEN);
    if (f5_warr_tile) {
      lv_obj_clear_flag(f5_warr_tile, LV_OBJ_FLAG_HIDDEN);
      if (f5_warr_cap) {
        lv_obj_set_style_text_font(f5_warr_cap, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
        lv_obj_align(f5_warr_cap, LV_ALIGN_TOP_MID, 0, f5_caption_y(en));
        f5_set_text_if_changed(f5_warr_cap, en ? "Warranty start" : "تاریخ شروع گارانتی");
      }
      if (f5_warr_val) {
        f5_set_text_if_changed(f5_warr_val, warranty_date[0] ? warranty_date : (en ? "--" : "--"));
      }
    }
  } else {
    if (f5_warr_tile) lv_obj_add_flag(f5_warr_tile, LV_OBJ_FLAG_HIDDEN);
    if (f5_btn_warr) {
      lv_obj_clear_flag(f5_btn_warr, LV_OBJ_FLAG_HIDDEN);
      lv_obj_t *lab = lv_obj_get_child(f5_btn_warr, 0);
      if (lab) {
        lv_obj_set_style_text_font(lab, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
        f5_set_text_if_changed(lab, en ? "Warranty" : "فعال سازی گارانتی");
        lv_obj_center(lab);
      }
    }
  }
}

static void f5_refresh_info() {
  const bool en = (GUI.system_language == 2);
  f5_set_cap(f5_cap_ver,   "نسخه برنامه", "Firmware", en);
  f5_set_cap(f5_cap_mfr,   "سازنده", "Maker", en);
  f5_set_cap(f5_cap_temp,  "دمای پردازنده و اطلاعات", "CPU information", en);
  f5_set_cap(f5_cap_clock, "ساعت و تاریخ سیستم", "System clock", en);
  f5_set_cap(f5_cap_prog,  "تاریخ برنامه", "Build date", en);
  f5_set_cap(f5_cap_heap,  "دما داخلی", "RTC temp", en);
  f5_set_cap(f5_cap_up,    "زمان روشن بودن", "Uptime", en);
  if (f5_info_heap) {
    float tc = 0.0f;
    char b[24];
    if (ds3231_read_temp_c(&tc))
      snprintf(b, sizeof(b), "%.1f C", (double)tc);
    else
      snprintf(b, sizeof(b), "--");
    f5_set_text_if_changed(f5_info_heap, b);
      lv_color_t rtc_col = tc > 65.0f ? lv_color_hex(0xFF3B30) :
                         (tc > 45.0f ? lv_color_hex(0xFF9500) : lv_color_hex(0x6e6e73));
    if (!lv_color_eq(lv_obj_get_style_text_color(f5_info_heap, LV_PART_MAIN), rtc_col))
      lv_obj_set_style_text_color(f5_info_heap, rtc_col, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (f5_info_up) {
    uint32_t s = millis() / 1000;
    char b[24];
    snprintf(b, sizeof(b), "%02u:%02u:%02u", (unsigned)(s / 3600), (unsigned)((s / 60) % 60), (unsigned)(s % 60));
    f5_set_text_if_changed(f5_info_up, b);
  }
  f5_set_text_if_changed(f5_info_ver, PROJECT_VERSION_DISPLAY);
  f5_set_text_if_changed(f5_info_mfr, "Haffler");
  if (f5_info_temp) {
    float tc = temperatureRead();
    char b[24]; snprintf(b, sizeof(b), "%.1f C", (double)tc);
    f5_set_text_if_changed(f5_info_temp, b);
    lv_color_t temp_color = tc > 70.0f ? lv_color_hex(0xFF3B30) :
                            (tc > 50.0f ? lv_color_hex(0xFF9500) : lv_color_hex(0x6e6e73));
    if (!lv_color_eq(lv_obj_get_style_text_color(f5_info_temp, LV_PART_MAIN), temp_color))
      lv_obj_set_style_text_color(f5_info_temp, temp_color, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (f5_info_clock) {
    char b[40];
    f5_format_system_clock(b, sizeof(b), en);
    f5_set_text_if_changed(f5_info_clock, b);
  }
  if (f5_info_prog) {
    char b[40];
    f5_format_program_clock(b, sizeof(b), en);
    f5_set_text_if_changed(f5_info_prog, b);
  }
  f5_warranty_sync_ui();
}

static void f5_refresh_cuts() {
  const bool en = (GUI.system_language == 2);
  f5_set_cap(f5_life_cap, "تعداد برش و کارکرد تیغه", "Blade Cuts & Ops", en);
  f5_set_cap(f5_user_cap, "تعداد کل برش", "Total Cuts", en);
  if (f5_life_lbl) {
    char b[32];
    uint32_t hp = blade_health_pct();
    snprintf(b, sizeof(b), "%lu%% (%lu/%lu)", (unsigned long)hp, (unsigned long)blade_cuts, (unsigned long)blade_limit);
    f5_set_text_if_changed(f5_life_lbl, b);
  }
  if (f5_user_lbl) {
    char b[32];
    if (lv_obj_get_style_text_font(f5_user_lbl, LV_PART_MAIN) != &lv_font_montserrat_20)
      lv_obj_set_style_text_font(f5_user_lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    snprintf(b, sizeof(b), "%lu", (unsigned long)total_cuts);
    f5_set_text_if_changed(f5_user_lbl, b);
  }
  if (f5_btn_update) {
    lv_obj_t *lab = lv_obj_get_child(f5_btn_update, 0);
    if (lab) {
      const lv_font_t *font = en ? &lv_font_montserrat_14 : &ui_font_naz18b;
      if (lv_obj_get_style_text_font(lab, LV_PART_MAIN) != font)
        lv_obj_set_style_text_font(lab, font, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      f5_set_text_if_changed(lab, en ? "Update" : "بروزرسانی");
      lv_obj_center(lab);
    }
  }
  f5_refresh_info();
}

void f5_dev_reflow() {
  if (!objects.obj20) return;
  f5_confirm_tick();
  // EEZ leftovers (Software Version card و هر چیز غیر از عنوان) را مخفی کن
  uint32_t n = lv_obj_get_child_count(objects.obj20);
  for (uint32_t i = 0; i < n; i++) {
    lv_obj_t *ch = lv_obj_get_child(objects.obj20, i);
    if (!ch || ch == f5_root) continue;
    int cy = lv_obj_get_y(ch);
    int chh = lv_obj_get_height(ch);
    int chw = lv_obj_get_width(ch);
    bool title_like = (cy < 30 && chh <= 60 && chw >= 200);
    if (!title_like) lv_obj_add_flag(ch, LV_OBJ_FLAG_HIDDEN);
  }
  if (f5_root && f5_layout_ver != F5_LAYOUT_VER) {
    if (f5_root && lv_obj_is_valid(f5_root)) lv_obj_delete(f5_root);
    f5_root = NULL;
    f5_stats = NULL;
    f5_life_lbl = NULL;
    f5_user_lbl = NULL;
    f5_info_ver = NULL;
    f5_info_mfr = NULL;
    f5_info_temp = NULL;
    f5_info_clock = NULL;
    f5_info_prog = NULL;
    f5_tile_prog = NULL;
    f5_tile_ver = NULL;
    f5_tile_life = f5_tile_temp = f5_tile_rtc = f5_tile_warr = NULL;
    f5_tile_mfr = f5_tile_clock = NULL;
    f5_btn_update = NULL;
    f5_life_cap = f5_user_cap = NULL;
    f5_cap_ver = f5_cap_mfr = f5_cap_temp = f5_cap_clock = f5_cap_prog = NULL;
    f5_cap_heap = f5_info_heap = f5_cap_up = f5_info_up = NULL;
    f5_tile_user = NULL;
    f5_btn_warr = NULL;
    f5_warr_tile = f5_warr_cap = f5_warr_val = NULL;
  }
  if (!f5_root) {
    // عنوان اصلی F5: سلسله‌مراتب ثابت و خوانا، بدون جابه‌جایی یا تغییر رفتار هدر.
    if (objects.lablef2_5) {
      lv_obj_set_style_text_font(objects.lablef2_5, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(objects.lablef2_5, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (objects.lablee2_5) {
      lv_obj_set_style_text_font(objects.lablee2_5, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(objects.lablee2_5, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    // obj20 روی (-16,-16) → screen_x = local_x + 16
    const int SCR_L = 6;
    const int SCR_R = 408;
    const int LOC_OFF = 16;
    const int CARD_X = SCR_L - LOC_OFF;          // -10
    const int CARD_W = SCR_R - SCR_L;            // 402
    // جمع‌وجورتر + فضای بیشتر: +۷ از بالا، +۳۰ از پایین نسبت به ۱۹۴ قبلی → ۲۳۱
    const int PAD = 10;
    const int GAP = 6;
    const int COL_W = (CARD_W - 2 * PAD - GAP) / 2;
    const int X0 = PAD;
    const int X1 = PAD + COL_W + GAP;
    const int ROW1_H = 42;  // +۲ نسبت به فضای +۱۲ کارت
    const int ROW2_H = 42;
    const int BTN_H = 38;   // +۴
    const int BTN_GAP = 6;
    const int Y0 = PAD;
    const int Y1 = Y0 + ROW1_H + GAP;
    const int Y2 = Y1 + ROW2_H + GAP;
    const int Y3 = Y2 + ROW2_H + GAP;
    const int Y_BTN = Y3 + ROW2_H + GAP;
    const int CARD_H = Y_BTN + BTN_H + 7;
    const int BTN_ROW_W = CARD_W - 2 * PAD;
    const int HALF_W = (BTN_ROW_W - BTN_GAP) / 2;

    f5_root = lv_obj_create(objects.obj20);
    lv_obj_set_pos(f5_root, CARD_X, 41);
    lv_obj_set_size(f5_root, CARD_W, CARD_H);
    lv_obj_set_style_bg_opa(f5_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(f5_root, 0, 0);
    lv_obj_set_style_pad_all(f5_root, 0, 0);
    lv_obj_clear_flag(f5_root, LV_OBJ_FLAG_SCROLLABLE);

    f5_stats = lv_obj_create(f5_root);
    lv_obj_set_pos(f5_stats, 0, 0);
    lv_obj_set_size(f5_stats, CARD_W, CARD_H);
    lv_obj_set_style_bg_color(f5_stats, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(f5_stats, 14, 0);
    lv_obj_set_style_border_width(f5_stats, 0, 0);
    lv_obj_set_style_shadow_width(f5_stats, 0, 0);
    lv_obj_set_style_pad_all(f5_stats, 0, 0);
    lv_obj_set_style_clip_corner(f5_stats, false, 0);
    lv_obj_clear_flag(f5_stats, LV_OBJ_FLAG_SCROLLABLE);

    auto mk_cell = [](lv_obj_t *parent, lv_obj_t **cap, lv_obj_t **val,
                       int x, int y, int w, int h, bool /*big*/, bool tile) -> lv_obj_t * {
      lv_obj_t *c = lv_obj_create(parent);
      lv_obj_set_pos(c, x, y); lv_obj_set_size(c, w, h);
      lv_obj_set_style_bg_color(c, lv_color_hex(tile ? 0xe4eefb : 0xf2f2f7), 0);
      lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
      lv_obj_set_style_radius(c, 10, 0);
      lv_obj_set_style_border_width(c, 0, 0);
      lv_obj_set_style_pad_all(c, 0, 0);
      lv_obj_set_style_clip_corner(c, false, 0);
      lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
      *cap = lv_label_create(c);
      lv_obj_set_style_text_color(*cap, lv_color_hex(0x1c1c1e), 0);
      lv_obj_set_style_text_align(*cap, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_pad_all(*cap, 0, 0);
      lv_obj_set_style_text_line_space(*cap, -4, 0);
      lv_obj_set_width(*cap, w - 8);
      lv_label_set_long_mode(*cap, LV_LABEL_LONG_CLIP);
      lv_obj_align(*cap, LV_ALIGN_TOP_MID, 0, -6);  // عنوان ۳+۳ بالا
      *val = lv_label_create(c);
      lv_obj_set_style_text_font(*val, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(*val, lv_color_hex(0x6e6e73), 0);
      lv_obj_set_style_text_align(*val, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_width(*val, w - 8);
      lv_label_set_long_mode(*val, LV_LABEL_LONG_CLIP);
      lv_obj_align(*val, LV_ALIGN_BOTTOM_MID, 0, -2);
      return c;
    };
    auto add_page_mark = [](lv_obj_t *tile) {
      if (!tile) return;
      lv_obj_t *mark = lv_label_create(tile);
      lv_obj_set_style_text_font(mark, &lv_font_montserrat_20, 0);
      lv_obj_set_style_text_color(mark, lv_color_hex(0x8e8e93), 0);
      lv_label_set_text(mark, ">");
      lv_obj_align(mark, LV_ALIGN_RIGHT_MID, -6, 0);
      lv_obj_clear_flag(mark, LV_OBJ_FLAG_CLICKABLE);
    };

    f5_tile_life = mk_cell(f5_stats, &f5_life_cap, &f5_life_lbl, X1, Y0, COL_W, ROW1_H, false, true);
    if (f5_tile_life) {
      lv_obj_add_flag(f5_tile_life, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_life, f5_cuts_report_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_life);
    }
    f5_tile_user = mk_cell(f5_stats, &f5_user_cap, &f5_user_lbl, X0, Y0, COL_W, ROW1_H, false, true);
    if (f5_tile_user) {
      lv_obj_add_flag(f5_tile_user, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_user, f5_user_reset_cb, LV_EVENT_RELEASED, NULL);
      if (f5_user_lbl) lv_obj_align(f5_user_lbl, LV_ALIGN_BOTTOM_MID, 0, 1);
    }

    mk_cell(f5_stats, &f5_cap_ver,   &f5_info_ver,   X1, Y1, COL_W, ROW2_H, false, true);
    f5_tile_ver = lv_obj_get_parent(f5_info_ver);
    if (f5_tile_ver) {
      lv_obj_add_flag(f5_tile_ver, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_ver, f5_release_notes_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_ver);
    }
    f5_tile_mfr = mk_cell(f5_stats, &f5_cap_mfr, &f5_info_mfr, X0, Y1, COL_W, ROW2_H, false, true);
    if (f5_tile_mfr) {
      lv_obj_add_flag(f5_tile_mfr, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_mfr, f5_maker_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_mfr);
    }
    f5_tile_clock = mk_cell(f5_stats, &f5_cap_clock, &f5_info_clock, X1, Y2, COL_W, ROW2_H, false, true);
    if (f5_tile_clock) {
      lv_obj_add_flag(f5_tile_clock, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_clock, f5_clock_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_clock);
    }
    f5_tile_temp = mk_cell(f5_stats, &f5_cap_temp,  &f5_info_temp,  X0, Y2, COL_W, ROW2_H, false, true);  // دما پردازنده
    if (f5_tile_temp) {
      lv_obj_add_flag(f5_tile_temp, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_temp, f5_cpu_temp_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_temp);
    }
    mk_cell(f5_stats, &f5_cap_prog,  &f5_info_prog,  X1, Y3, COL_W, ROW2_H, false, true);  // تاریخ برنامه
    f5_tile_prog = lv_obj_get_parent(f5_info_prog);
    if (f5_tile_prog) {
      lv_obj_add_flag(f5_tile_prog, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_prog, f5_changelog_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_prog);
    }
    f5_tile_rtc = mk_cell(f5_stats, &f5_cap_heap,  &f5_info_heap,  X0, Y3, COL_W, ROW2_H, false, true);  // دما داخلی DS3231
    if (f5_tile_rtc) {
      lv_obj_add_flag(f5_tile_rtc, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_tile_rtc, f5_rtc_temp_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_tile_rtc);
    }
    if (f5_info_clock) {
      lv_obj_set_style_text_font(f5_info_clock, &lv_font_montserrat_14, 0);
      lv_obj_set_width(f5_info_clock, COL_W - 8);
      lv_obj_align(f5_info_clock, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    if (f5_info_prog) {
      lv_obj_set_style_text_font(f5_info_prog, &lv_font_montserrat_14, 0);
      lv_obj_set_width(f5_info_prog, COL_W - 8);
      lv_obj_align(f5_info_prog, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    if (f5_info_heap) {
      lv_obj_set_style_text_font(f5_info_heap, &lv_font_montserrat_14, 0);
      lv_obj_set_width(f5_info_heap, COL_W - 8);
      lv_obj_align(f5_info_heap, LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    // نیم‌چپ بروزرسانی (آبی) + نیم‌راست فعال‌سازی گارانتی (سبز) — داخل کادر سفید
    f5_btn_update = lv_button_create(f5_stats);
    lv_obj_set_pos(f5_btn_update, PAD, Y_BTN);
    lv_obj_set_size(f5_btn_update, HALF_W, BTN_H);
    lv_obj_set_style_bg_color(f5_btn_update, lv_color_hex(0x0a84ff), 0);
    lv_obj_set_style_radius(f5_btn_update, 10, 0);
    lv_obj_set_style_shadow_width(f5_btn_update, 0, 0);
    lv_obj_set_style_pad_hor(f5_btn_update, 4, 0);
    lv_obj_add_event_cb(f5_btn_update, f5_update_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *lu = lv_label_create(f5_btn_update);
    lv_obj_set_style_text_color(lu, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lu, &ui_font_naz18b, 0);
    lv_label_set_long_mode(lu, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lu, HALF_W - 8);
    lv_obj_set_style_text_align(lu, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lu, "بروزرسانی");
    lv_obj_center(lu);

    f5_btn_warr = lv_button_create(f5_stats);
    lv_obj_set_pos(f5_btn_warr, PAD + HALF_W + BTN_GAP, Y_BTN);
    lv_obj_set_size(f5_btn_warr, HALF_W, BTN_H);
    lv_obj_set_style_bg_color(f5_btn_warr, lv_color_hex(0x34c759), 0);
    lv_obj_set_style_radius(f5_btn_warr, 10, 0);
    lv_obj_set_style_shadow_width(f5_btn_warr, 0, 0);
    lv_obj_set_style_pad_hor(f5_btn_warr, 4, 0);
    lv_obj_add_event_cb(f5_btn_warr, f5_warranty_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *lw = lv_label_create(f5_btn_warr);
    lv_obj_set_style_text_color(lw, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lw, &ui_font_naz18b, 0);
    lv_label_set_long_mode(lw, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lw, HALF_W - 8);
    lv_obj_set_style_text_align(lw, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lw, "فعال سازی گارانتی");
    lv_obj_center(lw);

    // بعد از فعال‌سازی: همان نیم‌راست فقط تاریخ ثابت
    f5_warr_tile = lv_obj_create(f5_stats);
    lv_obj_set_pos(f5_warr_tile, PAD + HALF_W + BTN_GAP, Y_BTN);
    lv_obj_set_size(f5_warr_tile, HALF_W, BTN_H);
    lv_obj_set_style_bg_color(f5_warr_tile, lv_color_hex(0xe4eefb), 0);
    lv_obj_set_style_radius(f5_warr_tile, 10, 0);
    lv_obj_set_style_border_width(f5_warr_tile, 0, 0);
    lv_obj_set_style_pad_all(f5_warr_tile, 0, 0);
    lv_obj_clear_flag(f5_warr_tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(f5_warr_tile, LV_OBJ_FLAG_HIDDEN);
    f5_warr_cap = lv_label_create(f5_warr_tile);
    lv_obj_set_style_text_color(f5_warr_cap, lv_color_hex(0x1c1c1e), 0);
    lv_obj_set_style_text_align(f5_warr_cap, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(f5_warr_cap, &ui_font_naz18b, 0);
    lv_obj_set_width(f5_warr_cap, HALF_W - 8);
    lv_obj_align(f5_warr_cap, LV_ALIGN_TOP_MID, 0, 1 - 6 - 3);  // تاریخ شروع گارانتی ۳px دیگر بالا
    f5_warr_val = lv_label_create(f5_warr_tile);
    lv_obj_set_style_text_font(f5_warr_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(f5_warr_val, lv_color_hex(0x6e6e73), 0);
    lv_obj_set_style_text_align(f5_warr_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(f5_warr_val, HALF_W - 8);
    lv_obj_align(f5_warr_val, LV_ALIGN_BOTTOM_MID, 0, 1);
    f5_tile_warr = f5_warr_tile;
    if (f5_warr_tile) {
      lv_obj_add_flag(f5_warr_tile, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(f5_warr_tile, f5_warranty_report_open_cb, LV_EVENT_RELEASED, NULL);
      add_page_mark(f5_warr_tile);
    }

    f5_layout_ver = F5_LAYOUT_VER;
  }
  f5_refresh_cuts();
}

void f5_dev_runtime_tick() {
  if (!f5_root || !objects.tabview2_1 || lv_tabview_get_tab_act(objects.tabview2_1) != 4) return;
  // ساعت باز هر 125ms sample می‌شود، اما فقط روی ثانیۀ واقعی بازطراحی دارد.
  f5_clock_overlay_tick();
  static uint32_t last_refresh_ms = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - last_refresh_ms) < 1000UL) return;
  last_refresh_ms = now;
  f5_refresh_cuts();
}

