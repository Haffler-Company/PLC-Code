extern String g_company_name;

static lv_obj_t *dev_lock_root = NULL;
static lv_obj_t *dev_lock_ta = NULL;
static lv_obj_t *dev_lock_err = NULL;
static lv_obj_t *dev_lock_eye_lbl = NULL;
static lv_obj_t *dev_lock_eye_btn = NULL;
static lv_obj_t *dev_lock_help_btn = NULL;
static lv_obj_t *dev_lock_help_root = NULL;
static lv_obj_t *dev_lock_kb = NULL;
static lv_obj_t *dev_lock_challenge_lbl = NULL;
static uint32_t dev_lock_challenge = 0;
static bool dev_lock_challenge_issued = false;
static bool dev_lock_security_loaded = false;
static bool dev_lock_attempts_blocked = false;
static uint8_t dev_lock_fail_count = 0;
static uint32_t dev_lock_block_until_ms = 0;
static uint32_t dev_lock_last_left_sec = UINT32_MAX;
static bool dev_lock_block_ui_applied = false;
static uint8_t dev_lock_last_fail_shown = 0xff;
static bool dev_lock_wait_for_splash = false;

void device_lock_ui_hold_for_splash(bool hold) {
  dev_lock_wait_for_splash = hold;
}

#define DEV_LOCK_MAX_FAILS 3
#define DEV_LOCK_BLOCK_MS 300000UL  // ۵ دقیقه قفل امنیتی طبق دستور مالک

static void dev_lock_issue_challenge() {
  dev_lock_challenge = (uint32_t)((esp_random() % 90000UL) + 10000UL); // عدد تصادفی ۵ رقمی
  dev_lock_challenge_issued = true;
  if (dev_lock_challenge_lbl) {
    char b[48];
    if (GUI.system_language == 2) snprintf(b, sizeof(b), "Recovery:  %05u", (unsigned)dev_lock_challenge);
    else snprintf(b, sizeof(b), "عدد بازیابی:  %05u", (unsigned)dev_lock_challenge);
    lv_label_set_text(dev_lock_challenge_lbl, b);
    lv_obj_clear_flag(dev_lock_challenge_lbl, LV_OBJ_FLAG_HIDDEN);
  }
}

static void dev_lock_help_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    if (dev_lock_help_root) {
      lv_obj_delete(dev_lock_help_root);
      dev_lock_help_root = NULL;
    }
  }
}

static void dev_lock_help_open() {
  if (dev_lock_help_root) return;
  const bool en = (GUI.system_language == 2);
  dev_lock_help_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(dev_lock_help_root, 480, 320);
  lv_obj_set_pos(dev_lock_help_root, 0, 0);
  lv_obj_set_style_bg_color(dev_lock_help_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(dev_lock_help_root, LV_OPA_70, 0);
  lv_obj_set_style_border_width(dev_lock_help_root, 0, 0);
  lv_obj_clear_flag(dev_lock_help_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dev_lock_help_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(dev_lock_help_root);
  lv_obj_set_size(card, 440, 270);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // هدر دیالوگ
  lv_obj_t *hdr = lv_obj_create(card);
  lv_obj_set_pos(hdr, 0, 0);
  lv_obj_set_size(hdr, 440, 44);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(0xf8fafc), 0);
  lv_obj_set_style_radius(hdr, 16, 0);
  lv_obj_set_style_border_width(hdr, 1, 0);
  lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(hdr, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_pad_all(hdr, 0, 0);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(hdr);
  lv_obj_set_size(title, 340, 44);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);
  lv_label_set_text(title, en ? "Device Unlock Guide" : "راهنمای بازگشایی دستگاه");
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  // دکمه ضربدر بستن
  lv_obj_t *close_btn = lv_button_create(hdr);
  lv_obj_set_pos(close_btn, 396, 6);
  lv_obj_set_size(close_btn, 32, 32);
  lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfee2e2), 0);
  lv_obj_set_style_border_width(close_btn, 0, 0);
  lv_obj_set_style_pad_all(close_btn, 0, 0);
  lv_obj_add_event_cb(close_btn, dev_lock_help_close_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *close_ic = lv_label_create(close_btn);
  lv_label_set_text(close_ic, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_font(close_ic, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(close_ic, lv_color_hex(0xdc2626), 0);
  lv_obj_center(close_ic);

  // متن راهنما همراه با نام شرکت
  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_pos(body, 16, 52);
  lv_obj_set_size(body, 408, 150);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0x334155), 0);
  lv_obj_set_style_text_align(body, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(body, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);

  const String cname = (g_company_name.length() > 0) ? g_company_name : String("HAFFLER");
  char msg[320];
  if (en) {
    snprintf(msg, sizeof(msg),
             "To unlock the device, please contact the company where you purchased the machine.\n\n"
             "If you do not have their phone number, search online for:\n"
             "\" %s \"\n"
             "to find their contact info and support.",
             cname.c_str());
  } else {
    snprintf(msg, sizeof(msg),
             "برای باز کردن رمز با شرکتی که دستگاه را از آن خریداری کرده‌اید تماس بگیرید.\n\n"
             "اگر شماره آن شرکت را ندارید، در اینترنت جستجو فرمایید:\n"
             "« %s »\n"
             "تا شماره تماس پشتیبانی را بیابید و دریافت کنید.",
             cname.c_str());
  }
  lv_label_set_text(body, msg);

  // دکمه متوجه شدم
  lv_obj_t *ok_btn = lv_button_create(card);
  lv_obj_set_pos(ok_btn, 16, 212);
  lv_obj_set_size(ok_btn, 408, 44);
  lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_radius(ok_btn, 10, 0);
  lv_obj_set_style_border_width(ok_btn, 0, 0);
  lv_obj_add_event_cb(ok_btn, dev_lock_help_close_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ok_lbl = lv_label_create(ok_btn);
  lv_label_set_text(ok_lbl, en ? "I Understand" : "متوجه شدم");
  lv_obj_set_style_text_font(ok_lbl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_center(ok_lbl);
}

static void dev_lock_help_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    dev_lock_help_open();
  }
}

static void dev_lock_security_persist() {
  pref.begin("params", false);
  pref_put_uchar_if_changed(pref, "dl_fails", dev_lock_fail_count);
  pref_put_bool_if_changed(pref, "dl_lock", dev_lock_attempts_blocked);
  pref.end();
}

static void dev_lock_security_load() {
  if (dev_lock_security_loaded) return;
  dev_lock_security_loaded = true;
  bool saved_lock = false;
  if (pref.begin("params", true)) {
    saved_lock = pref.getBool("dl_lock", false);
    pref.end();
  }
  if (saved_lock) {
    // طبق درخواست مالک: بعد از هر ریست، ۵ دقیقه از اول شمرده شود
    dev_lock_fail_count = DEV_LOCK_MAX_FAILS;
    dev_lock_attempts_blocked = true;
    dev_lock_block_until_ms = millis() + DEV_LOCK_BLOCK_MS;
  } else {
    dev_lock_fail_count = 0;
    dev_lock_attempts_blocked = false;
  }
}

static void dev_lock_security_clear() {
  dev_lock_fail_count = 0;
  dev_lock_attempts_blocked = false;
  dev_lock_block_until_ms = 0;
  dev_lock_last_left_sec = UINT32_MAX;
  dev_lock_last_fail_shown = 0;
  dev_lock_security_persist();
  if (dev_lock_err) lv_obj_add_flag(dev_lock_err, LV_OBJ_FLAG_HIDDEN);
  if (dev_lock_ta) lv_textarea_set_text(dev_lock_ta, "");
}

static void dev_lock_security_start_block() {
  dev_lock_fail_count = DEV_LOCK_MAX_FAILS;
  dev_lock_attempts_blocked = true;
  dev_lock_block_until_ms = millis() + DEV_LOCK_BLOCK_MS;
  dev_lock_last_left_sec = UINT32_MAX;
  dev_lock_security_persist();
}

static uint32_t dev_lock_security_left_sec() {
  if (!dev_lock_attempts_blocked) return 0;
  const int32_t left_ms = (int32_t)(dev_lock_block_until_ms - millis());
  if (left_ms <= 0) return 0;
  return ((uint32_t)left_ms + 999UL) / 1000UL;
}

static void dev_lock_security_sync_ui() {
  if (!dev_lock_err) return;
  uint32_t left = dev_lock_security_left_sec();
  if (dev_lock_attempts_blocked && left == 0) {
    dev_lock_fail_count = 0;
    dev_lock_attempts_blocked = false;
    dev_lock_block_until_ms = 0;
    dev_lock_last_left_sec = UINT32_MAX;
    dev_lock_security_persist();
    if (dev_lock_ta) lv_textarea_set_text(dev_lock_ta, "");
  }

  if (dev_lock_attempts_blocked) {
    if (left != dev_lock_last_left_sec) {
      dev_lock_last_left_sec = left;
      char msg[64];
      if (GUI.system_language == 2) {
        snprintf(msg, sizeof(msg), "3 wrong tries; lock %02u:%02u",
                 (unsigned)(left / 60UL), (unsigned)(left % 60UL));
      } else {
        snprintf(msg, sizeof(msg), "سه رمز اشتباه؛ قفل %02u:%02u",
                 (unsigned)(left / 60UL), (unsigned)(left % 60UL));
      }
      lv_label_set_text(dev_lock_err, msg);
    }
    lv_obj_clear_flag(dev_lock_err, LV_OBJ_FLAG_HIDDEN);
    if (!dev_lock_block_ui_applied) {
      if (dev_lock_ta) lv_obj_add_state(dev_lock_ta, LV_STATE_DISABLED);
      if (dev_lock_eye_btn) lv_obj_add_state(dev_lock_eye_btn, LV_STATE_DISABLED);
      if (dev_lock_kb) lv_obj_add_state(dev_lock_kb, LV_STATE_DISABLED);
      dev_lock_block_ui_applied = true;
    }
    dev_lock_last_fail_shown = 0xff;
  } else {
    if (dev_lock_block_ui_applied) {
      if (dev_lock_ta) {
        lv_obj_clear_state(dev_lock_ta, LV_STATE_DISABLED);
        lv_obj_add_state(dev_lock_ta, LV_STATE_FOCUSED);
      }
      if (dev_lock_eye_btn) lv_obj_clear_state(dev_lock_eye_btn, LV_STATE_DISABLED);
      if (dev_lock_kb) {
        lv_obj_clear_state(dev_lock_kb, LV_STATE_DISABLED);
        if (dev_lock_ta) lv_keyboard_set_textarea(dev_lock_kb, dev_lock_ta);
      }
      dev_lock_block_ui_applied = false;
    }
    if (dev_lock_fail_count != dev_lock_last_fail_shown) {
      dev_lock_last_fail_shown = dev_lock_fail_count;
      if (dev_lock_fail_count > 0) {
        char msg[48];
        if (GUI.system_language == 2) {
          snprintf(msg, sizeof(msg), "Wrong PIN (%u of %u)",
                   (unsigned)dev_lock_fail_count, (unsigned)DEV_LOCK_MAX_FAILS);
        } else {
          snprintf(msg, sizeof(msg), "رمز اشتباه (%u از %u)",
                   (unsigned)dev_lock_fail_count, (unsigned)DEV_LOCK_MAX_FAILS);
        }
        lv_label_set_text(dev_lock_err, msg);
        lv_obj_clear_flag(dev_lock_err, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(dev_lock_err, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
}

// همان کیبرد عددی طراحی‌شدۀ دروازۀ F4؛ تأیید فقط با تیک سبز کیبرد.
static const char * const dev_lock_kb_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t dev_lock_kb_ctrl[] = {
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1
};

static void dev_lock_kb_draw_cb(lv_event_t *e) {
  lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
  if (!draw_task) return;
  lv_draw_dsc_base_t *base = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
  if (!base || base->part != LV_PART_ITEMS) return;
  const uint32_t id = base->id1;
  uint32_t bg = 0;
  if (id == 9) bg = 0xFF3B30u;
  else if (id == 11) bg = 0x34C759u;
  else return;
  lv_draw_fill_dsc_t *fill = lv_draw_task_get_fill_dsc(draw_task);
  if (fill) fill->color = lv_color_hex(bg);
  lv_draw_label_dsc_t *lab = lv_draw_task_get_label_dsc(draw_task);
  if (lab) lab->color = lv_color_hex(0xffffff);
}

// مقایسهٔ فقط-ارقامی ورودی (نویز/فاصلهٔ لمسی نادیده گرفته می‌شود)
static bool dev_lock_digits_eq(const char *p, const char *want) {
  char dig[16];
  size_t n = 0;
  for (const char *s = p; s && *s && n + 1 < sizeof(dig); ++s)
    if (*s >= '0' && *s <= '9') dig[n++] = *s;
  dig[n] = 0;
  return strcmp(dig, want) == 0;
}

static void dev_lock_try_unlock() {
  if (!dev_lock_ta) return;
  dev_lock_security_load();
  if (dev_lock_attempts_blocked) {
    GUI.buzzer_cmd_long = true;
    dev_lock_security_sync_ui();
    return;
  }
  const char *p = lv_textarea_get_text(dev_lock_ta);
  if (day_lock_stage_password_ok(p) ||
      (dev_lock_challenge_issued && day_lock_one_time_password_ok(p, dev_lock_challenge))) {
    dev_lock_security_clear();
    const bool next_stage = day_lock_advance_stage();
    save_admin_svc_state();
    if (dev_lock_root) lv_obj_add_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN);
    dev_lock_challenge_issued = false;
    GUI.lock_screen = false;
    if (!STP.referenced) GUI.pending_goto_scr = 0;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    ui_toast(next_stage ? "مرحله بعدی روزشمار شروع شد" : "همه مرحله‌ها تمام شد",
             next_stage ? "Next lock stage started" : "All lock stages complete", 1800);
  } else if (dev_lock_digits_eq(p, "112233")) {
    // رمز اضطراری یک‌بارمصرف: از همین امروز +۱۰ روز به همهٔ تایمرهای فعال
    if (day_lock_apply_grace10()) {
      dev_lock_security_clear();
      save_admin_svc_state();
      if (dev_lock_root) lv_obj_add_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN);
      dev_lock_challenge_issued = false;
      GUI.lock_screen = false;
      if (!STP.referenced) GUI.pending_goto_scr = 0;
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
      ui_toast("ده روز مهلت به تایمرها اضافه شد",
               "10 days added to all active timers", 2000);
    } else {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      if (dev_lock_fail_count < DEV_LOCK_MAX_FAILS) dev_lock_fail_count++;
      if (dev_lock_fail_count >= DEV_LOCK_MAX_FAILS) dev_lock_security_start_block();
      else dev_lock_security_persist();
      lv_textarea_set_text(dev_lock_ta, "");
      dev_lock_security_sync_ui();
    }
  } else if (admin_password_ok(p)) {
    // ورود رمز مدیر یا مستر: پاک شدن دائم کلیه تایمرها
    dev_lock_security_clear();
    day_lock_clear_after_unlock();
    save_admin_svc_state();
    if (dev_lock_root) lv_obj_add_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN);
    dev_lock_challenge_issued = false;
    GUI.lock_screen = false;
    if (!STP.referenced) GUI.pending_goto_scr = 0;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    ui_toast("قفل باز شد و تایمرها غیرفعال شدند", "Unlocked & Timers Cleared", 1800);
  } else {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    if (dev_lock_fail_count < DEV_LOCK_MAX_FAILS) dev_lock_fail_count++;
    if (dev_lock_fail_count >= DEV_LOCK_MAX_FAILS) dev_lock_security_start_block();
    else dev_lock_security_persist();
    lv_textarea_set_text(dev_lock_ta, "");
    dev_lock_security_sync_ui();
  }
}
static void dev_lock_ok_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_READY) return;
  dev_lock_try_unlock();
}

static void dev_lock_eye_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED || !dev_lock_ta) return;
  const bool hidden = lv_textarea_get_password_mode(dev_lock_ta);
  lv_textarea_set_password_mode(dev_lock_ta, !hidden);
  if (dev_lock_eye_lbl)
    lv_label_set_text(dev_lock_eye_lbl, hidden ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
  GUI.buzzer_cmd_short = true;
}

// USB: رقم / بک‌اسپیس / Enter روی قفل روزشمار (هر صفحه) — صف؛ drain در Task2
static volatile char dev_lock_usb_q[24];
static volatile uint8_t dev_lock_usb_head = 0, dev_lock_usb_tail = 0;
static inline void dev_lock_usb_push(char c) {
  uint8_t nh = (uint8_t)((dev_lock_usb_head + 1u) % (uint8_t)sizeof(dev_lock_usb_q));
  if (nh == dev_lock_usb_tail) return;
  dev_lock_usb_q[dev_lock_usb_head] = c;
  dev_lock_usb_head = nh;
}
static inline bool dev_lock_usb_active() {
  return device_locked && dev_lock_root && !lv_obj_has_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN) &&
         dev_lock_ta;
}
static inline bool dev_lock_try_digit(char d) {
  if (!device_locked) return false;
  if (d < '0' || d > '9') return false;
  if (dev_lock_attempts_blocked) return true;
  dev_lock_usb_push(d);
  return true;
}
static inline bool dev_lock_try_backspace() {
  if (!device_locked) return false;
  if (dev_lock_attempts_blocked) return true;
  dev_lock_usb_push('\b');
  return true;
}
static inline bool dev_lock_try_enter() {
  if (!device_locked) return false;
  if (dev_lock_attempts_blocked) return true;
  dev_lock_usb_push('\r');
  return true;
}
static void dev_lock_usb_drain() {
  if (!dev_lock_usb_active()) {
    dev_lock_usb_tail = dev_lock_usb_head;
    return;
  }
  if (dev_lock_attempts_blocked) {
    dev_lock_usb_tail = dev_lock_usb_head;
    dev_lock_security_sync_ui();
    return;
  }
  while (dev_lock_usb_tail != dev_lock_usb_head) {
    char c = dev_lock_usb_q[dev_lock_usb_tail];
    dev_lock_usb_tail = (uint8_t)((dev_lock_usb_tail + 1u) % (uint8_t)sizeof(dev_lock_usb_q));
    if (c == '\r' || c == '\n') {
      dev_lock_try_unlock();
      if (dev_lock_attempts_blocked) {
        dev_lock_usb_tail = dev_lock_usb_head;
        break;
      }
    } else if (c == '\b' || c == 127) {
      lv_textarea_delete_char(dev_lock_ta);
    } else if (c >= '0' && c <= '9') {
      lv_textarea_add_char(dev_lock_ta, (uint32_t)(uint8_t)c);
      if (dev_lock_err && dev_lock_fail_count == 0)
        lv_obj_add_flag(dev_lock_err, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void ensure_dev_lock_ui() {
  if (dev_lock_root) return;
  dev_lock_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(dev_lock_root, 480, 320);
  lv_obj_set_pos(dev_lock_root, 0, 0);
  lv_obj_set_style_bg_color(dev_lock_root, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_bg_opa(dev_lock_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(dev_lock_root, 0, 0);
  lv_obj_set_style_radius(dev_lock_root, 0, 0);
  lv_obj_set_style_pad_all(dev_lock_root, 0, 0);
  lv_obj_clear_flag(dev_lock_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dev_lock_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *t = lv_label_create(dev_lock_root);
  lv_obj_set_style_text_font(t, &ui_font_naz24, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0xffffff), 0);
  lv_label_set_text(t, "دستگاه قفل است");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 2);  // حذف padding والد؛ جای بصری قبلی ثابت

  // دکمه راهنمای بازگشایی و شماره تماس شرکت
  dev_lock_help_btn = lv_button_create(dev_lock_root);
  lv_obj_set_pos(dev_lock_help_btn, 12, 10);
  lv_obj_set_size(dev_lock_help_btn, 36, 36);
  lv_obj_set_style_radius(dev_lock_help_btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dev_lock_help_btn, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_border_width(dev_lock_help_btn, 0, 0);
  lv_obj_set_style_pad_all(dev_lock_help_btn, 0, 0);
  lv_obj_add_event_cb(dev_lock_help_btn, dev_lock_help_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *hlbl = lv_label_create(dev_lock_help_btn);
  lv_label_set_text(hlbl, "?");
  lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(hlbl, lv_color_hex(0xffffff), 0);
  lv_obj_center(hlbl);

  lv_obj_t *s = lv_label_create(dev_lock_root);
  lv_obj_set_style_text_font(s, &ui_font_naz20, 0);
  lv_obj_set_style_text_color(s, lv_color_hex(0x8e8e93), 0);
  lv_label_set_text(s, "رمز مرحله، مدیر یا مستر را وارد کنید");
  lv_obj_align(s, LV_ALIGN_TOP_MID, 0, 31);  // بصری: دقیقاً ۹px بالاتر

  const int ta_w = 220;
  const int eye_w = 44;
  const int row_w = ta_w + 8 + eye_w;
  const int row_x = (480 - row_w) / 2 + 26;
  const int row_y = 61;
  dev_lock_ta = lv_textarea_create(dev_lock_root);
  lv_obj_set_size(dev_lock_ta, ta_w, 44);
  lv_obj_set_pos(dev_lock_ta, row_x, row_y);
  lv_textarea_set_one_line(dev_lock_ta, true);
  lv_textarea_set_password_mode(dev_lock_ta, true);
  lv_textarea_set_max_length(dev_lock_ta, 12);
  lv_textarea_set_accepted_chars(dev_lock_ta, "0123456789");
  lv_textarea_set_placeholder_text(dev_lock_ta, "PIN");
  lv_obj_set_style_text_align(dev_lock_ta, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(dev_lock_ta, &lv_font_montserrat_20, 0);
  lv_obj_set_style_radius(dev_lock_ta, 10, 0);
  lv_obj_add_state(dev_lock_ta, LV_STATE_FOCUSED);

  dev_lock_eye_btn = lv_button_create(dev_lock_root);
  lv_obj_set_size(dev_lock_eye_btn, eye_w, 44);
  lv_obj_set_pos(dev_lock_eye_btn, row_x + ta_w + 8, row_y);
  lv_obj_set_style_bg_color(dev_lock_eye_btn, lv_color_hex(0x3a3a3c), 0);
  lv_obj_set_style_radius(dev_lock_eye_btn, 10, 0);
  lv_obj_set_style_border_width(dev_lock_eye_btn, 0, 0);
  lv_obj_add_event_cb(dev_lock_eye_btn, dev_lock_eye_cb, LV_EVENT_RELEASED, NULL);
  dev_lock_eye_lbl = lv_label_create(dev_lock_eye_btn);
  lv_obj_set_style_text_font(dev_lock_eye_lbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(dev_lock_eye_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(dev_lock_eye_lbl, LV_SYMBOL_EYE_CLOSE);
  lv_obj_center(dev_lock_eye_lbl);

  dev_lock_kb = lv_keyboard_create(dev_lock_root);
  lv_keyboard_set_map(dev_lock_kb, LV_KEYBOARD_MODE_USER_1, dev_lock_kb_map, dev_lock_kb_ctrl);
  lv_keyboard_set_mode(dev_lock_kb, LV_KEYBOARD_MODE_USER_1);
  lv_obj_set_size(dev_lock_kb, 480, 178);  // لبه پایین ثابت؛ لبه بالا دقیقاً ۱۹px پایین‌تر
  lv_obj_align(dev_lock_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(dev_lock_kb, dev_lock_ta);
  lv_obj_add_flag(dev_lock_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(dev_lock_kb, dev_lock_kb_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_event_cb(dev_lock_kb, dev_lock_ok_cb, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(dev_lock_kb, [](lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    }
  }, LV_EVENT_VALUE_CHANGED, NULL);

  dev_lock_err = lv_label_create(dev_lock_root);
  lv_obj_set_style_text_color(dev_lock_err, lv_color_hex(0xff3b30), 0);
  lv_obj_set_style_text_font(dev_lock_err, &ui_font_naz18b, 0);
  lv_label_set_text(dev_lock_err, "رمز اشتباه");
  lv_obj_set_pos(dev_lock_err, 6, 68);
  lv_obj_set_width(dev_lock_err, 120);
  lv_obj_set_style_text_align(dev_lock_err, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(dev_lock_err, LV_OBJ_FLAG_HIDDEN);
  dev_lock_challenge_lbl = lv_label_create(dev_lock_root);
  lv_obj_set_width(dev_lock_challenge_lbl, 480);
  lv_obj_set_style_text_align(dev_lock_challenge_lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(dev_lock_challenge_lbl, (GUI.system_language == 2) ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(dev_lock_challenge_lbl, lv_color_hex(0x0a84ff), 0);
  lv_obj_align(dev_lock_challenge_lbl, LV_ALIGN_TOP_MID, 0, 108);
  lv_label_set_text(dev_lock_challenge_lbl, (GUI.system_language == 2) ? "Recovery number" : "عدد بازیابی");
  lv_obj_add_flag(dev_lock_challenge_lbl, LV_OBJ_FLAG_HIDDEN);
  dev_lock_security_load();
  dev_lock_security_sync_ui();
}

static void f5_confirm_tick();
static void f3_pinmap_runtime_tick();
void f2_limit_seek_tick();

void device_lock_ui_tick() {
  // فقط وقتی از نیمه‌شب رد شد (حداکثر یک‌بار در روز) ذخیره کن — نه هر ۶۰ ثانیه
  if (day_lock_tick()) save_admin_svc_state();
  // هر قفل ایمنی، مودال تنظیم تایمر را از چرخۀ مالکیت خارج می‌کند.
  // پس بازشدن/آزادشدن قفل ایمنی هرگز صفحۀ زنده‌ای با GUI.lock_screen=false باقی نمی‌گذارد.
  if (device_locked || sensor_alert_motion_lock || critical_task_start_failed || IOS.stop_button_state)
    f4_chain_close();
  f5_confirm_tick();
  f3_pinmap_runtime_tick();
  if (device_locked && !dev_lock_wait_for_splash) {
    f4_kb_hide();
    f4_clear_focus_style();
    ensure_dev_lock_ui();
    if (!dev_lock_challenge_issued) dev_lock_issue_challenge();
    dev_lock_security_sync_ui();
    lv_obj_clear_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN);
    if (dev_lock_kb) {
      lv_obj_clear_flag(dev_lock_kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (dev_lock_ta && !dev_lock_attempts_blocked) {
      lv_obj_add_state(dev_lock_ta, LV_STATE_FOCUSED);
      if (dev_lock_kb && lv_keyboard_get_textarea(dev_lock_kb) != dev_lock_ta) {
        lv_keyboard_set_textarea(dev_lock_kb, dev_lock_ta);
      }
    }
    lv_obj_move_foreground(dev_lock_root);
    GUI.lock_screen = true;
  } else if (dev_lock_root && !lv_obj_has_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_add_flag(dev_lock_root, LV_OBJ_FLAG_HIDDEN);
  }
  if (!GUI.virtual_kb_enable) f4_kb_hide();
  // USB رمز: صف را فقط روی ترد LVGL خالی کن
  f4_gate_usb_drain();
  dev_lock_usb_drain();
}

