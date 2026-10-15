#pragma once
// ===================== F4 admin =====================
#define F4_LAYOUT_VER 46
#define F4_CUT_TIMER_MAX_S 60
#define F4_DAY_ROLLER_MAX 200
#define F4_PIN_LOCK_SEC 600
static uint8_t f4_layout_ver = 0;
static bool f4_reflow_done = false;
static uint8_t f4_reflow_lang_seen = 0xff;
static bool f4_reflow_unlocked_seen = false;
static uint8_t f4_reflow_layout_seen = 0;
static lv_obj_t *f4_extra = NULL;
static lv_obj_t *f4_pin1 = NULL;
static lv_obj_t *f4_pin2 = NULL;
static lv_obj_t *f4_pin_ok = NULL;
static lv_obj_t *f4_day_card = NULL;
static lv_obj_t *f4_day_title = NULL;
static lv_obj_t *f4_day_roller = NULL;  // منسوخ — فقط برای پاک‌سازی نسخۀ قدیم
static lv_obj_t *f4_day_lbl = NULL;
static lv_obj_t *f4_day_ta = NULL;     // کادر عدد روزشمار
static lv_obj_t *f4_timer_ta = NULL;   // کادر عدد زمان تیغه
static lv_obj_t *f4_timer_s_lbl = NULL; // واحد ثانیه داخل کادر
static lv_obj_t *f4_active_ta = NULL;
static lv_obj_t *f4_ta_hh = NULL;
static lv_obj_t *f4_ta_mi = NULL;
static lv_obj_t *f4_ta_dd = NULL;
static lv_obj_t *f4_ta_mo = NULL;
static lv_obj_t *f4_ta_yy = NULL;
static bool f4_clock_time_edited = false;
static bool f4_clock_syncing = false;
static int64_t f4_clock_live_minute = -1;
static lv_obj_t *f4_kb = NULL;          // نامپد نرم مثل لیست برش
static lv_obj_t *f4_kb_root = NULL;     // ظرف قابل‌جابجایی
static lv_obj_t *f4_kb_handle = NULL;
static lv_obj_t *f4_kb_del = NULL;
static lv_obj_t *f4_kb_ok = NULL;
static lv_point_t f4_kb_drag_pt;
static int f4_kb_drag_ox = 0, f4_kb_drag_oy = 0;
static bool f4_kb_moved = false;   // اپراتور پد را جابه‌جا کرده؛ جای پیش‌فرض دیگر تحمیل نمی‌شود
static bool f4_kb_dragging = false;
static lv_obj_t *f4_fac_btn = NULL;
static lv_obj_t *f4_cal_btn = NULL;
static lv_obj_t *f4_apply_btn = NULL;

static uint8_t f4_event_cb_count(lv_obj_t *obj, lv_event_cb_t cb) {
  if (!obj || !lv_obj_is_valid(obj)) return 0;
  uint8_t found = 0;
  const uint32_t count = lv_obj_get_event_count(obj);
  for (uint32_t i = 0; i < count; ++i) {
    lv_event_dsc_t *dsc = lv_obj_get_event_dsc(obj, i);
    if (dsc && lv_event_dsc_get_cb(dsc) == cb && found < UINT8_MAX) ++found;
  }
  return found;
}

static void f4_remove_event_cb_all(lv_obj_t *obj, lv_event_cb_t cb) {
  if (!obj || !lv_obj_is_valid(obj)) return;
  for (uint32_t i = lv_obj_get_event_count(obj); i > 0; --i) {
    lv_event_dsc_t *dsc = lv_obj_get_event_dsc(obj, i - 1);
    if (dsc && lv_event_dsc_get_cb(dsc) == cb) lv_obj_remove_event_dsc(obj, dsc);
  }
}
static lv_obj_t *f4_pin_bar = NULL;
static lv_obj_t *f4_stop_btn = NULL;
static lv_obj_t *f4_stop_title = NULL;
static lv_obj_t *f4_stop_state = NULL;
static lv_obj_t *f4_dt_bar = NULL;
static lv_obj_t *f4_dd_cap = NULL;
static lv_obj_t *f4_mo_cap = NULL;
static lv_obj_t *f4_yy_cap = NULL;
static lv_obj_t *f4_fac_lbl = NULL;
static lv_obj_t *f4_cal_lbl = NULL;
static lv_obj_t *f4_pin_lbl = NULL;       // عنوان روی کادر اول (رمز)
static lv_obj_t *f4_pin_cfm_cap = NULL;   // عنوان روی کادر دوم (تکرار)
static lv_obj_t *f4_pin_admin_mid = NULL; // وسط: رمز مدیر (درشت‌تر)
static lv_obj_t *f4_pin_eye_lbl = NULL;
static bool f4_pin_revealed = false;
static lv_obj_t *f4_clk_lbl = NULL;
static lv_obj_t *f4_hh_cap = NULL;
static lv_obj_t *f4_mi_cap = NULL;
static lv_obj_t *f4_dt_colon = NULL;
static lv_obj_t *f4_dt_div = NULL;
static lv_obj_t *f4_dt_sep_day_month = NULL;
static lv_obj_t *f4_dt_sep_month_year = NULL;
static lv_obj_t *f4_date_lbl = NULL;
static lv_obj_t *f4_apply_lbl = NULL;  // حذف UI؛ اشاره برای سازگاری null
static lv_obj_t *f4_js_btn = NULL;
static lv_obj_t *f4_js_lbl = NULL;
static lv_obj_t *f4_hdr_mask = NULL;
static lv_obj_t *f4_err_btn = NULL;       // کادر آبی اصلاح خطا
static lv_obj_t *f4_err_btn_lbl = NULL;
static lv_obj_t *f4_err_num_box = NULL;  // کادر سفید عدد داخل آبی
static lv_obj_t *f4_err_ta = NULL;
static lv_obj_t *f4_err_unit = NULL;
static lv_obj_t *f4_fac_cfm = NULL;
static lv_obj_t *f4_fac_cfm_bar = NULL;
static lv_obj_t *f4_fac_cfm_track = NULL;
static lv_obj_t *f4_fac_cfm_count = NULL;
static volatile char f4_usb_q[24];
static volatile uint8_t f4_usb_q_head = 0;
static volatile uint8_t f4_usb_q_tail = 0;
static volatile bool f4_usb_edit_armed = false;
static uint32_t f4_fac_cfm_t0 = 0;
static bool f4_fac_cfm_holding = false;
// پنل زنجیرۀ قفل: فقط در F4 و فقط روی ترد LVGL ساخته می‌شود.
static lv_obj_t *f4_chain_root = NULL;
static lv_obj_t *f4_chain_help_root = NULL;
static lv_obj_t *f4_chain_cards[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t *f4_chain_vals[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t *f4_chain_minus[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t *f4_chain_plus[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t *f4_chain_plus_lbl[4] = {NULL, NULL, NULL, NULL};
static int f4_chain_draft[4] = {0, 0, 0, 0};
static uint8_t f4_chain_stage_base = 2;  // روزشمارِ تنظیمات عدد دارد = T2..T5؛ خالی است = T1..T4
static lv_obj_t *f4_chain_info[4] = {NULL, NULL, NULL, NULL};  // تاریخ قفل + رمز هر مرحله
static bool f4_chain_draft_valid = false;
// مالکیت قفل رابط برای مودال‌های F4: هر بازکننده یک نسل یکتا می‌گیرد.
// بستن مودال فقط همان نسل را آزاد می‌کند و هر قفل ایمنی هم‌زمان پابرجا می‌ماند.
enum : uint8_t { F4_UI_LOCK_NONE = 0, F4_UI_LOCK_TIMER_CHAIN = 1 };
static uint8_t f4_ui_lock_owner = F4_UI_LOCK_NONE;
static uint32_t f4_ui_lock_generation = 0;
static uint32_t f4_chain_lock_generation = 0;

// نشست رمز: بین تب‌های F می‌ماند؛ فقط با خروج به صفحه اصلی قفل می‌شود
static bool f4_unlocked = false;
static uint8_t f4_pin_fails = 0;
static int64_t f4_lock_until_ep = 0;   // soft epoch پایان قفل
static uint32_t f4_lock_until_ms = 0;  // millis پایان وقتی ساعت نرم نیست

// اسکرول: تأیید فقط با انگشت ثابت
static lv_point_t f4_tap_pt;
static bool f4_tap_moved = false;
static bool f4_tap_armed = false;
static int f4_y = 2026, f4_mo = 7, f4_d = 14, f4_hh = 12, f4_mi = 0;
static bool f4_roller_opts_set = false;
static bool f4_day_opts_set = false;
static char f4_day_opts[(F4_DAY_ROLLER_MAX + 1) * 4];  // "0\n".."200"
static volatile bool f4_gate_open = false;  // برای USB بدون لمس LVGL از Task4
static lv_obj_t *f4_gate_root = NULL;
static lv_obj_t *f4_gate_ta = NULL;
static lv_obj_t *f4_gate_err = NULL;
static lv_obj_t *f4_gate_kb = NULL;
static lv_obj_t *f4_gate_title = NULL;
static lv_obj_t *f4_gate_sub = NULL;
static lv_obj_t *f4_gate_eye_lbl = NULL;

static void f4_day_sync_ui();  // forward
static void f4_gate_ensure();
static void f4_gate_show(bool show);
static void f4_style_sec_roller(lv_obj_t *r);
static void f4_chain_open();
static void f4_chain_help_close();
static void f4_chain_delete_async(void *ptr) { if (ptr && lv_obj_is_valid((lv_obj_t *)ptr)) lv_obj_delete((lv_obj_t *)ptr); }
static void f4_chain_block_cb(lv_event_t *e) { (void)e; }
static bool f4_chain_safety_lock_active() {
  return device_locked || sensor_alert_motion_lock || critical_task_start_failed || IOS.stop_button_state;
}

static void f4_chain_rep_stop();  // تعریف پایین‌تر است؛ بستن مودال باید تکرار ± را بکشد

static void f4_chain_close() {
  f4_chain_rep_stop();
  f4_chain_help_close();
  lv_obj_t *root = f4_chain_root;
  f4_chain_root = NULL;
  for (uint8_t i = 0; i < 4; ++i) {
    f4_chain_cards[i] = f4_chain_vals[i] = f4_chain_minus[i] = f4_chain_plus[i] = f4_chain_plus_lbl[i] = NULL;
    f4_chain_info[i] = NULL;
  }
  const uint32_t token = f4_chain_lock_generation;
  f4_chain_lock_generation = 0;
  if (token && f4_ui_lock_owner == F4_UI_LOCK_TIMER_CHAIN && f4_ui_lock_generation == token) {
    f4_ui_lock_owner = F4_UI_LOCK_NONE;
    // رخدادهای ایمنی می‌توانند هنگام نمایش پنجره قفل رابط را بگیرند؛
    // مودال تایمر حق آزادسازی آن قفل‌ها را ندارد.
    if (!f4_chain_safety_lock_active()) GUI.lock_screen = false;
  }
  if (root && lv_obj_is_valid(root)) lv_async_call(f4_chain_delete_async, root);
}

static void f4_chain_help_delete_async(void *ptr) { if (ptr && lv_obj_is_valid((lv_obj_t *)ptr)) lv_obj_delete((lv_obj_t *)ptr); }
static void f4_chain_help_close() {
  lv_obj_t *root = f4_chain_help_root;
  f4_chain_help_root = NULL;
  if (root && lv_obj_is_valid(root)) lv_async_call(f4_chain_help_delete_async, root);
}
static void f4_chain_help_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    f4_chain_help_close();
  }
}

void f4_chain_pending_begin() {
  if (day_lock_t1_from_chain) {
    f4_chain_draft[0] = constrain(day_lock_remain, 0, 50);
    for (uint8_t i = 0; i < 3; ++i)
      f4_chain_draft[i + 1] = constrain(day_lock_stage_days[i + 1], 0, 50);
  } else {
    for (uint8_t i = 0; i < 4; ++i)
      f4_chain_draft[i] = constrain(day_lock_stage_days[i + 1], 0, 50);
  }
  f4_chain_draft_valid = true;
}

void f4_chain_pending_discard() {
  f4_chain_pending_begin();
}

bool f4_chain_pending_has_changes() {
  if (!f4_chain_draft_valid) return false;
  if (day_lock_t1_from_chain) {
    if (f4_chain_draft[0] != constrain(day_lock_remain, 0, 50)) return true;
    for (uint8_t i = 0; i < 3; ++i) {
      if (f4_chain_draft[i + 1] != constrain(day_lock_stage_days[i + 1], 0, 50)) return true;
    }
  } else {
    for (uint8_t i = 0; i < 4; ++i) {
      if (f4_chain_draft[i] != constrain(day_lock_stage_days[i + 1], 0, 50)) return true;
    }
  }
  return false;
}

static int f4_chain_t1_days_now();

bool f4_chain_commit_pending_on_header_save() {
  if (!f4_chain_pending_has_changes()) return false;
  const uint8_t base = (f4_chain_t1_days_now() > 0 && !day_lock_t1_from_chain) ? 2 : 1;
  if (base == 1) {
    // کارت روزشمارِ تنظیمات خالی است: همین چهار تایمر T1..T4 هستند؛ زنجیره از T1 مسلح می‌شود
    for (uint8_t i = 0; i < 3; ++i) day_lock_stage_days[i + 1] = f4_chain_draft[i + 1];
    day_lock_stage_days[4] = 0;
    if (f4_chain_draft[0] > 0) {
      day_lock_t1_from_chain = true;   // بعد از ریست هم کارت روزشمار از زنجیره است
      day_lock_start_chain(f4_chain_draft[0]);
    } else {
      day_lock_t1_from_chain = false;
      day_lock_remain = 0;
    }
  } else {
    day_lock_t1_from_chain = false;
    for (uint8_t i = 0; i < 4; ++i) day_lock_stage_days[i + 1] = f4_chain_draft[i];
  }
  return true;
}

// ---- اطلاعات قابل‌عکس‌برداریِ هر مرحله: تاریخ قفلِ آینده + رمز همان مرحله ----
// تاریخ = امروز + جمع تجمعی روزهای مراحلِ فعال تا آن مرحله؛ فارسی شمسی، انگلیسی میلادی
static void f4_chain_stage_date_buf(int cum_days, char *out, size_t out_sz) {
  int64_t now = soft_now_epoch();
  if (now <= 0 || cum_days <= 0) { snprintf(out, out_sz, "-"); return; }
  int y, m, d, hh, mi, ss;
  epoch_to_civil(midnight_of(now) + (int64_t)cum_days * 86400LL, &y, &m, &d, &hh, &mi, &ss);
  if (GUI.system_language == 2) {
    snprintf(out, out_sz, "%04d/%02d/%02d", y, m, d);  // میلادی
    return;
  }
  int jy, jm, jd;
  gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
  snprintf(out, out_sz, "%04d/%02d/%02d", jy, jm, jd);
}

// رمز مرحله = فرمول مالک (رقم‌به‌رقم) — همان که صفحهٔ قفل می‌پذیرد
static void f4_chain_stage_code_buf(uint8_t stage, char *out, size_t out_sz) {
  day_lock_stage_code_digits(out, out_sz, stage);
}

// روزهای مرحلهٔ اول: پیش‌نویسِ کادر روزشمار اگر معتبر بود، وگرنه مقدار ذخیره‌شده
static int f4_chain_t1_days_now() {
  const char *raw = f4_day_ta ? lv_textarea_get_text(f4_day_ta) : NULL;
  int v = (raw && raw[0]) ? atoi(raw) : day_lock_remain;
  return v < 0 ? 0 : v;
}

static void f4_chain_info_refresh() {
  const bool en = (GUI.system_language == 2);
  int cum = (f4_chain_stage_base == 2) ? f4_chain_t1_days_now() : 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (!f4_chain_info[i]) continue;
    if (f4_chain_draft[i] > 0) {
      cum += f4_chain_draft[i];
      char date[16], code[24];
      f4_chain_stage_date_buf(cum, date, sizeof(date));
      f4_chain_stage_code_buf((uint8_t)(i + f4_chain_stage_base), code, sizeof(code));
      // رمز خط اول، تاریخ زیر آن — وسط‌چین بین T و «روز»
      lv_label_set_text_fmt(f4_chain_info[i], en ? "Code: %s\nLock: %s" : "رمز:\u200E %s\u200E\nقفل:\u200E %s\u200E", code, date);
      lv_obj_clear_flag(f4_chain_info[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(f4_chain_info[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void f4_chain_refresh() {
  for (uint8_t i = 0; i < 4; ++i) {
    const bool enabled = (i == 0) || f4_chain_draft[i - 1] > 0;
    if (f4_chain_vals[i]) { char v[8]; snprintf(v, sizeof(v), "%d", f4_chain_draft[i]); lv_label_set_text(f4_chain_vals[i], v); }
    if (f4_chain_cards[i]) {
      lv_obj_set_style_bg_color(f4_chain_cards[i], lv_color_hex(enabled ? 0xffffff : 0xf2f2f7), 0);
      lv_obj_set_style_border_color(f4_chain_cards[i], lv_color_hex(enabled ? 0x0a84ff : 0xd1d1d6), 0);
      lv_obj_set_style_border_width(f4_chain_cards[i], enabled ? 2 : 1, 0);
    }
    if (f4_chain_vals[i]) lv_obj_set_style_text_color(f4_chain_vals[i], lv_color_hex(enabled ? 0x1d1d1f : 0x8e8e93), 0);
    lv_obj_t *adjust[2] = {f4_chain_minus[i], f4_chain_plus[i]};
    for (uint8_t j = 0; j < 2; ++j) {
      lv_obj_t *b = adjust[j];
      if (!b) continue;
      if (enabled) lv_obj_clear_state(b, LV_STATE_DISABLED);
      else lv_obj_add_state(b, LV_STATE_DISABLED);
      lv_obj_set_style_bg_color(b, lv_color_hex(enabled ? (j ? 0x0066cc : 0xe5e5ea) : 0xe5e5ea), 0);
      lv_obj_t *label = lv_obj_get_child(b, 0);
      if (label) lv_obj_set_style_text_color(label, lv_color_hex(enabled ? (j ? 0xffffff : 0x1d1d1f) : 0x8e8e93), 0);
    }
  }
}

static lv_timer_t *f4_chain_rep_tm = NULL;      // تکرار شتاب‌دار ± با نگه‌داشتن
static intptr_t f4_chain_rep_data = 0;
static uint16_t f4_chain_rep_cnt = 0;
static bool f4_chain_rep_click_skip = false;

static void f4_chain_adjust_apply(uint8_t idx, int delta, bool fast) {
  if (idx >= 4 || (idx && f4_chain_draft[idx - 1] <= 0)) { if (!fast) GUI.buzzer_cmd_long = true; return; }
  const int nv = constrain(f4_chain_draft[idx] + delta, 0, 50);
  if (nv == f4_chain_draft[idx]) return;
  f4_chain_draft[idx] = nv;
  if (!fast) GUI.buzzer_cmd_short = true;
  if (nv == 0 || !fast) {
    f4_chain_refresh();  // صفرشدن زنجیره یا کلیک تکی: همهٔ حالت‌ها تازه شوند
  } else if (f4_chain_vals[idx]) {
    char v[8]; snprintf(v, sizeof(v), "%d", nv);
    lv_label_set_text(f4_chain_vals[idx], v);  // مسیر تند: فقط عدد همان کارت
  }
  f4_chain_info_refresh();
  settings_mark_dirty();
}

static void f4_chain_rep_timer_cb(lv_timer_t *t) {
  (void)t;
  ++f4_chain_rep_cnt;
  // قانون شتاب مالک: اول ۱ به ۱، بعد ۲ به ۲، بعد ۵ به ۵، بعد ۱۰ به ۱۰
  const int step = (f4_chain_rep_cnt <= 6) ? 1 : (f4_chain_rep_cnt <= 16) ? 2
                : (f4_chain_rep_cnt <= 30) ? 5 : 10;
  lv_timer_set_period(t, (f4_chain_rep_cnt <= 6) ? 140 : (f4_chain_rep_cnt <= 16) ? 110 : 90);
  f4_chain_adjust_apply((uint8_t)(f4_chain_rep_data >> 1),
                        (f4_chain_rep_data & 1) ? step : -step, true);
}

static void f4_chain_rep_stop() {
  if (f4_chain_rep_tm) { lv_timer_delete(f4_chain_rep_tm); f4_chain_rep_tm = NULL; }
}

// ضربهٔ کوتاه = یک واحد؛ نگه‌داشتن = تکرار (اولین تکرار پس از ۴۵۰ms)
static void f4_chain_hold_cb(lv_event_t *e) {
  const lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED) {
    f4_chain_rep_stop();
    f4_chain_rep_data = (intptr_t)lv_event_get_user_data(e);
    f4_chain_rep_cnt = 0;
    f4_chain_rep_click_skip = false;
    f4_chain_rep_tm = lv_timer_create(f4_chain_rep_timer_cb, 450, NULL);
  } else if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST) {
    // اگر تکرار اجرا شد، CLICK پس از رها شدن نباید یک واحد اضافه بزند
    f4_chain_rep_click_skip = (f4_chain_rep_cnt > 0);
    f4_chain_rep_stop();
  }
}

static void f4_chain_adjust_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (f4_chain_rep_click_skip) { f4_chain_rep_click_skip = false; return; }
  const intptr_t data = (intptr_t)lv_event_get_user_data(e);
  f4_chain_adjust_apply((uint8_t)(data >> 1), (data & 1) ? 1 : -1, false);
}

static void f4_chain_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    f4_chain_close();
  }
}

static lv_obj_t *f4_chain_button(lv_obj_t *parent, int x, int y, int w, int h, const char *txt,
                                  uint32_t bg, uint32_t fg, const lv_font_t *font,
                                  lv_event_cb_t cb, void *data) {
  lv_obj_t *b = lv_button_create(parent); lv_obj_set_pos(b, x, y); lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0); lv_obj_set_style_radius(b, 10, 0); lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, data);
  lv_obj_t *l = lv_label_create(b); lv_label_set_text(l, txt); lv_obj_set_style_text_font(l, font, 0); lv_obj_set_style_text_color(l, lv_color_hex(fg), 0); lv_obj_center(l);
  return b;
}

static void f4_chain_help_open() {
  if (f4_chain_help_root || !f4_chain_root) return;
  const bool en = GUI.system_language == 2;
  f4_chain_help_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f4_chain_help_root, 480, 320);
  lv_obj_set_pos(f4_chain_help_root, 0, 0);
  lv_obj_set_style_bg_color(f4_chain_help_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f4_chain_help_root, LV_OPA_60, 0);
  lv_obj_set_style_border_width(f4_chain_help_root, 0, 0);
  lv_obj_clear_flag(f4_chain_help_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_chain_help_root, LV_OBJ_FLAG_CLICKABLE);

  // کارت مدرن دیالوگ به سبک اپل و زیمنس
  lv_obj_t *card = lv_obj_create(f4_chain_help_root);
  lv_obj_set_size(card, 456, 296);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // نوار هدر تفکیک‌شده
  lv_obj_t *hdr = lv_obj_create(card);
  lv_obj_set_pos(hdr, 0, 0);
  lv_obj_set_size(hdr, 456, 46);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(0xf8fafc), 0);
  lv_obj_set_style_radius(hdr, 16, 0);
  lv_obj_set_style_border_width(hdr, 1, 0);
  lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(hdr, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_pad_all(hdr, 0, 0);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(hdr);
  lv_obj_set_size(title, 350, 46);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);
  lv_label_set_text(title, en ? "Lock Timer Guide" : "راهنمای تایمر و رمز قفل");
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  // دکمه خروج دایره‌ای شیک درون هدر
  lv_obj_t *close_btn = lv_button_create(hdr);
  lv_obj_set_pos(close_btn, 414, 7);
  lv_obj_set_size(close_btn, 32, 32);
  lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfee2e2), 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfecaca), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(close_btn, 0, 0);
  lv_obj_set_style_pad_all(close_btn, 0, 0);
  lv_obj_add_event_cb(close_btn, f4_chain_help_close_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *close_ic = lv_label_create(close_btn);
  lv_label_set_text(close_ic, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_font(close_ic, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(close_ic, lv_color_hex(0xdc2626), 0);
  lv_obj_center(close_ic);

  // کانتینر اسکرول با اسکرول‌بار متمایل به لبه راست و فاصله استاندارد از متن
  lv_obj_t *scroll = lv_obj_create(card);
  lv_obj_set_pos(scroll, 8, 52);
  lv_obj_set_size(scroll, 440, 180);
  lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(scroll, 0, 0);
  lv_obj_set_style_pad_left(scroll, 6, 0);
  lv_obj_set_style_pad_right(scroll, 16, 0);
  lv_obj_set_style_pad_ver(scroll, 4, 0);
  lv_obj_set_style_pad_bottom(scroll, 24, 0);
  lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_ON);
  lv_obj_set_style_width(scroll, 4, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_right(scroll, 2, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(scroll, lv_color_hex(0x94a3b8), LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(scroll, LV_OPA_COVER, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(scroll, 2, LV_PART_SCROLLBAR);

  lv_obj_t *body = lv_label_create(scroll);
  lv_obj_set_width(body, 412);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0x334155), 0);
  lv_obj_set_style_text_align(body, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(body, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);

  const char *txt_fa =
    "۱. ساختار مراحل و روزشمار:\n"
    "اگر در صفحه تنظیمات مدیریت، برای «روزشمار قفل» عدد روز وارد شده باشد، تایمرهای این پنجره مراحل بعدی (T2 تا T5) را تشکیل می‌دهند. اگر کارت روزشمار خالی یا صفر باشد، همین تایمرها مراحل اول تا چهارم (T1 تا T4) می‌شوند.\n\n"
    "۲. تنظیم روزها و افزایش سریع:\n"
    "برای هر مرحله تا ۵۰ روز می‌توانید تعیین کنید. با فشردن کلیدهای + و - روزها تغییر می‌کنند و با نگه‌داشتن ممتد کلید، اعداد با سرعت و شتاب بالا کم یا زیاد می‌شوند.\n\n"
    "۳. تاریخ قفل و فرمول رمز هر مرحله:\n"
    "تاریخ زیر هر کارت، موعد دقیق قفل شدن دستگاه بر اساس تقویم سیستم است.\n"
    "ضریب هر تایمر:\n"
    "تایمر ۱ (T1): ضریب ۲ | تایمر ۲ (T2): ضریب ۳ | تایمر ۳ (T3): ضریب ۴ | تایمر ۴ (T4): ضریب ۵ | تایمر ۵ (T5): ضریب ۶\n"
    "روش محاسبه رمز: هر تک‌رقم رمز مدیر در ضریب ضرب شده و فقط رقم یکان هر ضرب نوشته می‌شود؛ در انتها، یکان حاصل‌جمع کل ارقام به انتهای عدد اضافه می‌گردد.\n"
    "نمونه ۱ (رمز مدیر 111111):\n"
    "تایمر ۱ (ضریب ۲): ۱×۲=۲ → 222222؛ جمع ارقام ۱۲ (یکان ۲) → رمز: 2222222\n"
    "تایمر ۲ (ضریب ۳): ۱×۳=۳ → 333333؛ جمع ارقام ۱۸ (یکان ۸) → رمز: 3333338\n"
    "نمونه ۲ (رمز مدیر 123456، تایمر ۱ ضریب ۲):\n"
    "۱×۲=۲، ۲×۲=۴، ۳×۲=۶، ۴×۲=۸، ۵×۲=۱۰ (یکان ۰)، ۶×۲=۱۲ (یکان ۲)\n"
    "عدد حاصل: 246802؛ جمع ارقام: ۲۲ (یکان ۲)؛ رمز مرحله: 2468022\n\n"
    "۴. بازگشایی و تسلسل مراحل:\n"
    "در زمان قفل هر مرحله، فقط رمز همان مرحله پذیرفته می‌شود. با ورود رمز صحیح، همان تایمر غیرفعال شده و بلافاصله زمان‌گیری مرحله بعدی آغاز می‌گردد.\n\n"
    "۵. رمز مدیر و غیرفعال‌سازی دائم (رمز مستر 12773):\n"
    "با وارد کردن رمز مدیر یا رمز مستر 12773 در صفحه قفل، تمامی تایمرها کلاً پاک و خاموش می‌شوند.\n\n"
    "۶. عدد بازیابی و رمز چالش:\n"
    "روی صفحه قفل یک عدد بازیابی تصادفی درج شده است.\n"
    "فرمول رمز چالش: عدد بازیابی ضربدر ۲، و اضافه کردن رقم یکان حاصل‌جمع ارقام در انتها.\n\n"
    "۷. کد اضطراری تمدید ۱۰ روزه (112233):\n"
    "کد 112233 فقط یک‌بار در طول عمر دستگاه کار می‌کند و از تاریخ جاری ۱۰ روز مهلت اضافه می‌دهد.\n\n"
    "۸. محافظت در برابر دستکاری ساعت و باتری RTC:\n"
    "برداشتن باتری ساعت یا عقب کشیدن تاریخ/ساعت باعث قفل شدن آنی دستگاه می‌شود.\n\n"
    "۹. مسدودسازی ۵ دقیقه‌ای (ضد ریست):\n"
    "ورود ۳ بار رمز اشتباه، دستگاه را ۵ دقیقه قفل می‌کند. ریست سخت‌افزاری نیز این ۵ دقیقه را از اول می‌شمارد.\n\n"
    "۱۰. ذخیره‌سازی قطعی:\n"
    "تغییرات فقط پس از زدن دکمه ذخیره بالای صفحه مدیریت ثبت می‌شوند. حتماً از این صفحه عکس بگیرید.";

  const char *txt_en =
    "1. Stage structure: If Day Lock has a value, timers are T2 to T5; if empty/zero, timers are T1 to T4.\n\n"
    "2. Setting days: Adjust up to 50 days with + and -. Long press enables accelerated fast stepping.\n\n"
    "3. Lock date & Stage code formula:\n"
    "Multipliers: T1 -> x2, T2 -> x3, T3 -> x4, T4 -> x5, T5 -> x6.\n"
    "Formula: Multiply each digit of admin PIN by multiplier, write ones digit of each product; append ones digit of total sum.\n"
    "Example 1 (Admin PIN 111111):\n"
    "Timer 1 (x2): 222222, sum=12 (ones 2) -> Code: 2222222\n"
    "Timer 2 (x3): 333333, sum=18 (ones 8) -> Code: 3333338\n"
    "Example 2 (Admin PIN 123456, Timer 1 x2):\n"
    "1x2=2, 2x2=4, 3x2=6, 4x2=8, 5x2=0, 6x2=2 -> 246802, sum=22 (ones 2) -> Code: 2468022\n\n"
    "4. Sequential Unlocking: Each stage only accepts its own code. Unlocking clears that timer and starts the next stage countdown.\n\n"
    "5. Master Unlock (12773 / Admin PIN): Entering admin PIN or master 12773 permanently clears all timers.\n\n"
    "6. Recovery Challenge: Lock screen shows random number. Formula: challenge x 2 + ones of digit sum.\n\n"
    "7. Emergency 10-day Grace (112233): Usable only once in device lifetime; grants 10 days from today.\n\n"
    "8. RTC Tamper Protection: Removing RTC battery or rolling clock backward immediately locks the machine.\n\n"
    "9. 5-Minute Lockout: 3 wrong entries trigger a 5-minute lockout (restarts from beginning on reboot).\n\n"
    "10. Permanent Save: Settings take effect ONLY after pressing the Save button. Always photograph this screen.";

  lv_label_set_text(body, en ? txt_en : txt_fa);

  // دکمه تایید مدرن پایین دیالوگ
  f4_chain_button(card, 14, 240, 428, 44, en ? "I Understand" : "متوجه شدم",
                  0x0a84ff, 0xffffff, en ? &lv_font_montserrat_18 : &ui_font_naz18b,
                  f4_chain_help_close_cb, NULL);
}
static void f4_chain_help_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) f4_chain_help_open(); }

static void f4_chain_open() {
  if (f4_chain_root) return;
  if (!f4_chain_draft_valid) f4_chain_pending_begin();
  const bool en = GUI.system_language == 2;
  f4_chain_root = lv_obj_create(lv_layer_top()); lv_obj_set_size(f4_chain_root, 480, 320); lv_obj_set_pos(f4_chain_root, 0, 0);
  // زمینۀ روشنِ گروه‌بندی‌شده: آرام، خوانا و مطابق سلسله‌مراتب کاربردی HMI.
  lv_obj_set_style_bg_color(f4_chain_root, lv_color_hex(0xf2f2f7), 0); lv_obj_set_style_bg_opa(f4_chain_root, LV_OPA_COVER, 0); lv_obj_set_style_border_width(f4_chain_root, 0, 0); lv_obj_set_style_pad_all(f4_chain_root, 0, 0); lv_obj_clear_flag(f4_chain_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_chain_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_chain_root, f4_chain_block_cb, LV_EVENT_ALL, NULL);
  f4_ui_lock_owner = F4_UI_LOCK_TIMER_CHAIN;
  f4_chain_lock_generation = ++f4_ui_lock_generation;
  GUI.lock_screen = true;
  f4_kb_hide();
  f4_clear_focus_style();
  lv_obj_t *title = lv_label_create(f4_chain_root); lv_obj_set_width(title, 340); lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0); lv_obj_set_style_text_color(title, lv_color_hex(0x1d1d1f), 0); lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz24, 0); lv_label_set_text(title, en ? "Lock timers" : "تایمرهای قفل دستگاه"); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_t *help_btn = f4_chain_button(f4_chain_root, 8, 6, 40, 40, "", 0x0a84ff, 0xffffff, &lv_font_montserrat_20, f4_chain_help_cb, NULL);
  f1_hdr_set_book_icon(help_btn);
  f4_chain_button(f4_chain_root, 432, 6, 40, 40, LV_SYMBOL_CLOSE, 0xd92d20, 0xffffff, &lv_font_montserrat_20, f4_chain_close_cb, NULL);
  // شماره‌گذاری تایمرها: روزشمارِ تنظیمات عدد دارد = T2..T5؛ خالی/صفر است = T1..T4
  f4_chain_stage_base = (f4_chain_t1_days_now() > 0 && !day_lock_t1_from_chain) ? 2 : 1;
  for (uint8_t i = 0; i < 4; ++i) {
    const int x = 8 + (i % 2) * 236, y = 78 + (i / 2) * 122;
    lv_obj_t *card = lv_obj_create(f4_chain_root); f4_chain_cards[i] = card; lv_obj_set_pos(card, x, y); lv_obj_set_size(card, 228, 118); lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0); lv_obj_set_style_radius(card, 12, 0); lv_obj_set_style_border_width(card, 1, 0); lv_obj_set_style_border_color(card, lv_color_hex(0xd1d1d6), 0); lv_obj_set_style_pad_all(card, 0, 0); lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *cap = lv_label_create(card); char stage[8]; snprintf(stage, sizeof(stage), "T%u", (unsigned)(i + f4_chain_stage_base)); lv_label_set_text(cap, stage); lv_obj_set_style_text_font(cap, &lv_font_montserrat_16, 0); lv_obj_set_style_text_color(cap, lv_color_hex(0x6e6e73), 0); lv_obj_set_pos(cap, 12, 6);
    f4_chain_minus[i] = f4_chain_button(card, 8, 60, 44, 48, "-", 0xe5e5ea, 0x1d1d1f, &lv_font_montserrat_20, f4_chain_adjust_cb, (void *)(intptr_t)(i << 1));
    f4_chain_plus[i] = f4_chain_button(card, 176, 60, 44, 48, "+", 0x0066cc, 0xffffff, &lv_font_montserrat_20, f4_chain_adjust_cb, (void *)(intptr_t)((i << 1) | 1));
    // نگه‌داشتن ± = تکرار شتاب‌دار؛ دادهٔ همان دکمه باید پاس شود تا تایمرِ خودش را تغییر دهد
    lv_obj_add_event_cb(f4_chain_minus[i], f4_chain_hold_cb, LV_EVENT_PRESSED, (void *)(intptr_t)(i << 1));
    lv_obj_add_event_cb(f4_chain_minus[i], f4_chain_hold_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(f4_chain_minus[i], f4_chain_hold_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(f4_chain_plus[i], f4_chain_hold_cb, LV_EVENT_PRESSED, (void *)(intptr_t)((i << 1) | 1));
    lv_obj_add_event_cb(f4_chain_plus[i], f4_chain_hold_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(f4_chain_plus[i], f4_chain_hold_cb, LV_EVENT_PRESS_LOST, NULL);
    f4_chain_plus_lbl[i] = lv_obj_get_child(f4_chain_plus[i], 0);
    lv_obj_t *v = lv_label_create(card); f4_chain_vals[i] = v; lv_obj_set_width(v, 112); lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_CENTER, 0); lv_obj_set_style_text_font(v, &lv_font_montserrat_26, 0); lv_obj_set_style_text_color(v, lv_color_hex(0x1d1d1f), 0); lv_obj_set_pos(v, 58, 69);
    lv_obj_t *unit = lv_label_create(card); lv_label_set_text(unit, en ? "days" : "روز"); lv_obj_set_style_text_font(unit, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0); lv_obj_set_style_text_color(unit, lv_color_hex(0x6e6e73), 0); lv_obj_set_style_base_dir(unit, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0); lv_obj_align(unit, LV_ALIGN_TOP_RIGHT, -12, en ? 8 : 4);
    lv_obj_t *info = lv_label_create(card);
    f4_chain_info[i] = info;
    lv_obj_set_width(info, 160);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(info, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(info, lv_color_hex(0x0a84ff), 0);
    lv_obj_set_style_base_dir(info, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
    lv_obj_set_pos(info, 34, 4);  // وسطِ میان عنوان T و «روز»؛ رمز بالا، تاریخ زیرش
  }
  // فرمولِ رمز بالای صفحه، جلوی چشم مدیر
  lv_obj_t *formula = lv_label_create(f4_chain_root);
  lv_obj_set_width(formula, 464);
  lv_obj_set_style_text_align(formula, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(formula, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(formula, lv_color_hex(0x6e6e73), 0);
  lv_obj_set_style_base_dir(formula, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(formula, LV_ALIGN_TOP_MID, 0, en ? 50 : 44);
  lv_label_set_long_mode(formula, LV_LABEL_LONG_DOT);  // فقط یک خط؛ هرگز بیرون نزند
  lv_label_set_text(formula, en ? "Timer codes: (PIN digit x Stage mult) + last digit of digit sum (T1:x2, T2:x3...)"
                                : "فرمول رمز: (هر رقم رمز مدیر × ضریب مرحله) + رقم یکان جمع ارقام (T1:ضربدر ۲، T2:ضربدر ۳...)");
  f4_chain_refresh();
  f4_chain_info_refresh();
  // پیام راهنما همراه خودِ صفحه باز شود
  f4_chain_help_open();
}

static void f4_day_chain_hold_feedback(bool held) {
  if (f4_day_card) {
    lv_obj_set_style_bg_color(f4_day_card, lv_color_hex(held ? 0xfff3cd : 0xffffff), 0);
    lv_obj_set_style_border_width(f4_day_card, held ? 2 : 1, 0);
    lv_obj_set_style_border_color(f4_day_card, lv_color_hex(held ? 0xff9f0a : 0xc6c6c8), 0);
  }
  if (f4_day_ta) {
    lv_obj_set_style_bg_color(f4_day_ta, lv_color_hex(held ? 0xffffe0 : 0xffffff), 0);
    lv_obj_set_style_border_width(f4_day_ta, held ? 2 : 1, 0);
    lv_obj_set_style_border_color(f4_day_ta, lv_color_hex(held ? 0xff9f0a : 0xc6c6c8), 0);
  }
}
static uint32_t f4_day_hold_start_ms = 0;
static void f4_day_chain_hold_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    f4_day_hold_start_ms = millis();
    f4_day_chain_hold_feedback(true);
  } else if (code == LV_EVENT_PRESSING) {
    if (f4_day_hold_start_ms && (uint32_t)(millis() - f4_day_hold_start_ms) >= 350 && !f4_chain_root) {
      f4_day_hold_start_ms = 0;
      f4_day_chain_hold_feedback(false);
      GUI.buzzer_cmd_short = true;
      f4_chain_open();
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    f4_day_hold_start_ms = 0;
    f4_day_chain_hold_feedback(false);
  } else if (code == LV_EVENT_LONG_PRESSED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    f4_day_hold_start_ms = 0;
    f4_day_chain_hold_feedback(false);
    GUI.buzzer_cmd_short = true;
    f4_chain_open();
  }
}

static void f4_style_sec_roller(lv_obj_t *r) {
  if (!r) return;
  lv_obj_set_size(r, 112, 100);
  lv_obj_align(r, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_roller_set_visible_row_count(r, 3);
  lv_obj_set_style_text_font(r, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(r, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(r, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(r, lv_color_hex(0xffffff), LV_PART_SELECTED | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(r, lv_color_hex(0x1c1c1e), LV_PART_SELECTED | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(r, 8, LV_PART_SELECTED | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(r, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// تب‌های F سمت راست ۶۰px — کادر رمز فقط تا لبۀ آن‌ها
#define F4_GATE_FBAR_W 60
#define F4_GATE_W (480 - F4_GATE_FBAR_W)
#define F4_GATE_KB_H 172  // 152 + 20px به بالا
#define F4_GATE_LAYOUT_VER 7
// Owner-approved temporary work mode: bypasses only the F4 gate while this screen is refined.
// Restore this to false before a production release.
static constexpr bool F4_CAPTURE_WORK_MODE = false;
static uint8_t f4_gate_layout_ver = 0;
static lv_obj_t *f4_gate_home = NULL;

// صف USB → LVGL فقط در Task2 (لمس مستقیم textarea از Task4 = یک رقم و بعد قفل)
static volatile char f4_gate_usb_q[24];
static volatile uint8_t f4_gate_usb_head = 0, f4_gate_usb_tail = 0;
static inline void f4_gate_usb_push(char c) {
  uint8_t nh = (uint8_t)((f4_gate_usb_head + 1u) % (uint8_t)sizeof(f4_gate_usb_q));
  if (nh == f4_gate_usb_tail) return;
  f4_gate_usb_q[f4_gate_usb_head] = c;
  f4_gate_usb_head = nh;
}
static inline bool f4_gate_visible() {
  return f4_gate_root && lv_obj_is_valid(f4_gate_root) &&
         !lv_obj_has_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN) && f4_gate_ta;
}

// کیبورد فقط: ارقام + حذف + تیک تأیید (بدون دکمۀ جدا OK)
static const char * const f4_gate_kb_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t f4_gate_kb_ctrl[] = {
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
    LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1
};

// id9 = پاک‌کردن قرمز، id11 = تیک تأیید سبز
static void f4_gate_kb_draw_cb(lv_event_t *e) {
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

static void f4_gate_pin_digits(char *out, size_t out_n) {
  if (!out || out_n == 0) return;
  out[0] = 0;
  if (!f4_gate_ta) return;
  const char *raw = lv_textarea_get_text(f4_gate_ta);
  if (!raw) return;
  size_t n = 0;
  for (const char *s = raw; *s && n + 1 < out_n; s++) {
    if (*s >= '0' && *s <= '9') out[n++] = *s;
  }
  out[n] = 0;
}

static void f4_pin_lock_persist() {
  // از pref سریال‌شده؛ کلیدهای بدون تغییر هم نوشته نمی‌شوند (عمر فلش)
  pref.begin("params", false);
  pref_put_uchar_if_changed(pref, "f4_fails", f4_pin_fails);
  pref.putLong64("f4_lk_ep", f4_lock_until_ep);
  uint32_t rem = 0;
  if (f4_lock_until_ms && (int32_t)(f4_lock_until_ms - millis()) > 0)
    rem = (f4_lock_until_ms - millis() + 999UL) / 1000UL;
  else if (f4_lock_until_ep > 0) {
    int64_t now = soft_now_epoch();
    if (now > 0 && f4_lock_until_ep > now) rem = (uint32_t)(f4_lock_until_ep - now);
  }
  pref_put_uint_if_changed(pref, "f4_lk_rem", rem);
  pref.end();
}

static void f4_pin_lock_load() {
  if (!pref.begin("params", true)) return;
  f4_pin_fails = pref.getUChar("f4_fails", 0);
  f4_lock_until_ep = pref.getLong64("f4_lk_ep", 0);
  uint32_t rem = pref.getUInt("f4_lk_rem", 0);
  pref.end();
  f4_lock_until_ms = 0;
  int64_t now = soft_now_epoch();
  if (f4_lock_until_ep > 0 && now > 0) {
    if (now >= f4_lock_until_ep) {
      f4_lock_until_ep = 0;
      f4_pin_fails = 0;
      f4_pin_lock_persist();
    }
    return;
  }
  if (rem > 0) {
    if (rem > F4_PIN_LOCK_SEC) rem = F4_PIN_LOCK_SEC;
    f4_lock_until_ms = millis() + rem * 1000UL;
  }
}

static bool f4_pin_is_locked(uint32_t *remain_sec_out) {
  int64_t now = soft_now_epoch();
  if (f4_lock_until_ep > 0 && now > 0) {
    if (now < f4_lock_until_ep) {
      if (remain_sec_out) *remain_sec_out = (uint32_t)(f4_lock_until_ep - now);
      return true;
    }
    f4_lock_until_ep = 0;
    f4_pin_fails = 0;
    f4_pin_lock_persist();
    return false;
  }
  if (f4_lock_until_ms != 0) {
    int32_t left = (int32_t)(f4_lock_until_ms - millis());
    if (left > 0) {
      if (remain_sec_out) *remain_sec_out = (uint32_t)((left + 999) / 1000);
      return true;
    }
    f4_lock_until_ms = 0;
    f4_pin_fails = 0;
    f4_pin_lock_persist();
  }
  return false;
}

static void f4_pin_start_lockout() {
  int64_t now = soft_now_epoch();
  if (now > 0) {
    f4_lock_until_ep = now + F4_PIN_LOCK_SEC;
    f4_lock_until_ms = 0;
  } else {
    f4_lock_until_ep = 0;
    f4_lock_until_ms = millis() + (uint32_t)F4_PIN_LOCK_SEC * 1000UL;
  }
  f4_pin_lock_persist();
}

static bool f4_tap_accept(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  lv_indev_t *in = lv_indev_active();
  if (c == LV_EVENT_PRESSED) {
    f4_tap_armed = true;
    f4_tap_moved = false;
    if (in) lv_indev_get_point(in, &f4_tap_pt);
    return false;
  }
  if (c == LV_EVENT_PRESSING && f4_tap_armed && in) {
    lv_point_t p;
    lv_indev_get_point(in, &p);
    if (LV_ABS(p.x - f4_tap_pt.x) >= 12 || LV_ABS(p.y - f4_tap_pt.y) >= 12)
      f4_tap_moved = true;
    return false;
  }
  if (c == LV_EVENT_RELEASED || c == LV_EVENT_CLICKED) {
    if (ui_help_take_suppress()) {
      f4_tap_armed = false;
      return false;
    }
    bool ok = f4_tap_armed && !f4_tap_moved;
    f4_tap_armed = false;
    return ok;
  }
  return false;
}

// [UI/Task2] فونت naz رقم ASCII و فارسی ندارد؛ ارقام عربی-هندی سازگار می‌سازد.
static void f4_format_naz_uint(uint32_t value, char *out, size_t out_size) {
  if (!out || out_size < 3U) return;
  char ascii[12];
  snprintf(ascii, sizeof(ascii), "%lu", (unsigned long)value);
  size_t pos = 0;
  for (const char *p = ascii; *p && (pos + 2U) < out_size; ++p) {
    out[pos++] = (char)0xD9;
    out[pos++] = (char)(0xA0 + (*p - '0'));
  }
  out[pos] = '\0';
}

static void f4_gate_show_locked_error(uint32_t remain_sec) {
  if (!f4_gate_err) return;
  const bool en = (GUI.system_language == 2);
  if (en) {
    char msg[48];
    snprintf(msg, sizeof(msg), "Locked %umin — admin PIN only", (unsigned)((remain_sec + 59U) / 60U));
    lv_label_set_text(f4_gate_err, msg);
    return;
  }
  char minutes[16], msg[64];
  f4_format_naz_uint((remain_sec + 59U) / 60U, minutes, sizeof(minutes));
  snprintf(msg, sizeof(msg), "قفل %s دقیقه — فقط رمز ادمین", minutes);
  lv_label_set_text(f4_gate_err, msg);
}

static void f4_gate_show_wrong_error(void) {
  if (!f4_gate_err) return;
  const bool en = (GUI.system_language == 2);
  if (en) {
    char msg[40];
    snprintf(msg, sizeof(msg), "Wrong (%u/3)", (unsigned)f4_pin_fails);
    lv_label_set_text(f4_gate_err, msg);
    return;
  }
  char attempts[16], max_attempts[16], msg[64];
  f4_format_naz_uint((uint32_t)f4_pin_fails, attempts, sizeof(attempts));
  f4_format_naz_uint(3U, max_attempts, sizeof(max_attempts));
  snprintf(msg, sizeof(msg), "رمز اشتباه (%s از %s)", attempts, max_attempts);
  lv_label_set_text(f4_gate_err, msg);
}

static void f4_gate_try_unlock() {
  if (!f4_gate_ta) return;
  if (f4_unlocked) return;
  char pin[16];
  f4_gate_pin_digits(pin, sizeof(pin));
  uint32_t rem = 0;
  const bool locked = f4_pin_is_locked(&rem);
  if (locked) {
    if (!admin_password_ok(pin)) {
      if (f4_gate_err) {
        f4_gate_show_locked_error(rem);
        lv_obj_clear_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);
      }
      lv_textarea_set_text(f4_gate_ta, "");
      f4_gate_usb_head = 0;
      f4_gate_usb_tail = 0;
      GUI.buzzer_cmd_long = true;
      return;
    }
    f4_lock_until_ep = 0;
    f4_lock_until_ms = 0;
    f4_pin_fails = 0;
    f4_pin_lock_persist();
  } else if (!admin_password_ok(pin)) {
    if (f4_pin_fails < 250) f4_pin_fails++;
    f4_pin_lock_persist();
    if (f4_gate_err) {
      f4_gate_show_wrong_error();
      lv_obj_clear_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);
    }
    lv_textarea_set_text(f4_gate_ta, "");
    f4_gate_usb_head = 0;
    f4_gate_usb_tail = 0;
    GUI.buzzer_cmd_long = true;
    if (f4_pin_fails >= 3) {
      f4_pin_start_lockout();
      if (f4_gate_err) {
        f4_gate_show_locked_error(F4_PIN_LOCK_SEC);
      }
    }
    return;
  }
  f4_unlocked = true;
  f4_pin_fails = 0;
  f4_lock_until_ep = 0;
  f4_lock_until_ms = 0;
  f4_pin_lock_persist();
  if (f4_gate_err) lv_obj_add_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);
  lv_textarea_set_text(f4_gate_ta, "");
  f4_gate_usb_head = 0;
  f4_gate_usb_tail = 0;
  f4_gate_show(false);
  if (objects.obj16) lv_obj_clear_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
  f4_admin_reflow();
  GUI.lock_screen = false;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  if (admin_pin == "123456") {
    ui_toast("هشدار: رمز مدیر پیش‌فرض کارخانه است؛ لطفاً آن را تغییر دهید",
             "Warning: Admin PIN is factory default (123456). Please change it.", 4000);
  }
  ui_resume_checkpoint();
}

// مصرف صف USB روی ترد LVGL (Task2)
static void f4_gate_usb_drain() {
  if (f4_gate_kb) {
    lv_obj_clear_flag(f4_gate_kb, LV_OBJ_FLAG_HIDDEN);
  }
  if (!f4_gate_visible()) {
    f4_gate_usb_tail = f4_gate_usb_head;
    return;
  }
  while (f4_gate_usb_tail != f4_gate_usb_head) {
    char c = f4_gate_usb_q[f4_gate_usb_tail];
    f4_gate_usb_tail = (uint8_t)((f4_gate_usb_tail + 1u) % (uint8_t)sizeof(f4_gate_usb_q));
    if (c == '\r' || c == '\n') {
      f4_gate_try_unlock();
    } else if (c == '\b' || c == 127) {
      lv_textarea_delete_char(f4_gate_ta);
    } else if (c >= '0' && c <= '9') {
      lv_textarea_add_char(f4_gate_ta, (uint32_t)(uint8_t)c);
      if (f4_gate_err) lv_obj_add_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void f4_gate_ta_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) f4_gate_try_unlock();
}

static void f4_gate_eye_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (!f4_gate_ta) return;
  bool hidden = lv_textarea_get_password_mode(f4_gate_ta);
  lv_textarea_set_password_mode(f4_gate_ta, !hidden);
  if (f4_gate_eye_lbl)
    lv_label_set_text(f4_gate_eye_lbl, hidden ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
  GUI.buzzer_cmd_short = true;
}

static void f4_gate_home_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  f4_session_lock();
  f4_gate_show(false);
  GUI.lock_screen = false;
  request_goto_screen(1);
}

static void f4_gate_ensure() {
  static bool f4_lock_loaded = false;
  if (!f4_lock_loaded) {
    f4_pin_lock_load();
    f4_lock_loaded = true;
  }
  if (f4_gate_root && lv_obj_is_valid(f4_gate_root) && f4_gate_layout_ver == F4_GATE_LAYOUT_VER) return;
  if (f4_gate_root && lv_obj_is_valid(f4_gate_root)) {
    lv_obj_delete(f4_gate_root);
  }
  f4_gate_root = f4_gate_ta = f4_gate_err = f4_gate_kb = f4_gate_title = f4_gate_sub = f4_gate_eye_lbl = NULL;
  f4_gate_home = NULL;

  f4_gate_root = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(f4_gate_root, 0, 0);
  lv_obj_set_size(f4_gate_root, F4_GATE_W, 320);
  lv_obj_set_style_bg_color(f4_gate_root, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_bg_opa(f4_gate_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f4_gate_root, 0, 0);
  lv_obj_set_style_radius(f4_gate_root, 0, 0);
  lv_obj_set_style_pad_all(f4_gate_root, 0, 0);
  lv_obj_clear_flag(f4_gate_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_gate_root, LV_OBJ_FLAG_CLICKABLE);

  // خانه — همان جای هدر F1..F5 (x=6 y=6 w=44 h=39)
  f4_gate_home = lv_button_create(f4_gate_root);
  lv_obj_set_pos(f4_gate_home, 6, 6);
  lv_obj_set_size(f4_gate_home, 44, 39);
  lv_obj_set_style_radius(f4_gate_home, 10, 0);
  lv_obj_set_style_bg_color(f4_gate_home, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_bg_opa(f4_gate_home, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f4_gate_home, 0, 0);
  lv_obj_set_style_shadow_width(f4_gate_home, 0, 0);
  lv_obj_set_style_pad_all(f4_gate_home, 0, 0);
  lv_obj_add_event_cb(f4_gate_home, f4_gate_home_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *home_ic = lv_label_create(f4_gate_home);
  lv_obj_set_style_text_font(home_ic, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(home_ic, lv_color_hex(0xffffff), 0);
  lv_label_set_text(home_ic, LV_SYMBOL_HOME);
  lv_obj_center(home_ic);

  f4_gate_title = lv_label_create(f4_gate_root);
  lv_obj_set_style_text_font(f4_gate_title, &ui_font_naz24, 0);
  lv_obj_set_style_text_color(f4_gate_title, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(f4_gate_title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(f4_gate_title, F4_GATE_W - 16);
  lv_label_set_text(f4_gate_title, "تنظیمات مدیریت");
  // قبلی (0,12) → ۱۲px بالا + ۲۲px چپ
  lv_obj_align(f4_gate_title, LV_ALIGN_TOP_MID, -22, 0);

  f4_gate_sub = lv_label_create(f4_gate_root);
  lv_obj_set_style_text_font(f4_gate_sub, &ui_font_naz20, 0);
  lv_obj_set_style_text_color(f4_gate_sub, lv_color_hex(0x8e8e93), 0);
  lv_obj_set_style_text_align(f4_gate_sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(f4_gate_sub, F4_GATE_W - 16);
  lv_label_set_text(f4_gate_sub, "رمز مدیریت را وارد کنید");
  // قبلی (-22,48) → ۱۰px بالا
  lv_obj_align(f4_gate_sub, LV_ALIGN_TOP_MID, -22, 38);

  // کادر رقم + چشم (بدون هالۀ نقطه — پیش‌فرض نمایان)
  const int ta_w = 220;
  const int eye_w = 44;
  const int row_w = ta_w + 8 + eye_w;
  const int row_x = (F4_GATE_W - row_w) / 2;
  const int row_y = 74;  // قبلی ۸۴ → ۱۰px بالا

  f4_gate_ta = lv_textarea_create(f4_gate_root);
  lv_obj_set_size(f4_gate_ta, ta_w, 44);
  lv_obj_set_pos(f4_gate_ta, row_x, row_y);
  lv_textarea_set_one_line(f4_gate_ta, true);
  lv_textarea_set_password_mode(f4_gate_ta, true);  // پیش‌فرض: پنهان
  lv_textarea_set_max_length(f4_gate_ta, 12);
  lv_textarea_set_accepted_chars(f4_gate_ta, "0123456789");
  lv_textarea_set_placeholder_text(f4_gate_ta, "PIN");
  lv_obj_set_style_text_align(f4_gate_ta, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(f4_gate_ta, &lv_font_montserrat_20, 0);
  lv_obj_set_style_radius(f4_gate_ta, 10, 0);
  lv_obj_set_style_bg_color(f4_gate_ta, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_opa(f4_gate_ta, LV_OPA_COVER, 0);
  lv_obj_add_event_cb(f4_gate_ta, f4_gate_ta_event_cb, LV_EVENT_READY, NULL);

  lv_obj_t *eye = lv_button_create(f4_gate_root);
  lv_obj_set_size(eye, eye_w, 44);
  lv_obj_set_pos(eye, row_x + ta_w + 8, row_y);
  lv_obj_set_style_bg_color(eye, lv_color_hex(0x3a3a3c), 0);
  lv_obj_set_style_radius(eye, 10, 0);
  lv_obj_set_style_border_width(eye, 0, 0);
  lv_obj_add_event_cb(eye, f4_gate_eye_cb, LV_EVENT_RELEASED, NULL);
  f4_gate_eye_lbl = lv_label_create(eye);
  lv_obj_set_style_text_font(f4_gate_eye_lbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(f4_gate_eye_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(f4_gate_eye_lbl, LV_SYMBOL_EYE_CLOSE);  // پنهان
  lv_obj_center(f4_gate_eye_lbl);

  f4_gate_err = lv_label_create(f4_gate_root);
  lv_obj_set_style_text_color(f4_gate_err, lv_color_hex(0xff3b30), 0);
  lv_obj_set_style_text_font(f4_gate_err, &ui_font_naz20, 0);
  lv_label_set_text(f4_gate_err, "رمز اشتباه");
  lv_obj_align(f4_gate_err, LV_ALIGN_TOP_MID, -22, 126);
  lv_obj_add_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);

  // تأیید فقط با تیک سبز روی کیبورد (بدون دکمۀ جدا)
  // مدیر فقط با USB PIN را وارد می‌کند؛ کیبرد لمسی روی صفحۀ رمز ایجاد نشود.
  f4_gate_kb = lv_keyboard_create(f4_gate_root);
  lv_keyboard_set_map(f4_gate_kb, LV_KEYBOARD_MODE_USER_1, f4_gate_kb_map, f4_gate_kb_ctrl);
  lv_keyboard_set_mode(f4_gate_kb, LV_KEYBOARD_MODE_USER_1);
  lv_obj_set_size(f4_gate_kb, F4_GATE_W, F4_GATE_KB_H);
  lv_obj_align(f4_gate_kb, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_keyboard_set_textarea(f4_gate_kb, f4_gate_ta);
  lv_obj_add_flag(f4_gate_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(f4_gate_kb, f4_gate_kb_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_event_cb(f4_gate_kb, f4_gate_ta_event_cb, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(f4_gate_kb, [](lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    }
  }, LV_EVENT_VALUE_CHANGED, NULL);
  f4_wdt_feed();  // کیبورد سنگین است — پنجرۀ WDT پیش از ادامه از نو

  lv_obj_add_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN);
  f4_gate_open = false;
  f4_gate_layout_ver = F4_GATE_LAYOUT_VER;
  f4_diag_log("gate build");
}

static void f4_gate_show(bool show) {
  f4_gate_ensure();
  if (!f4_gate_root) return;
  if (show && GUI.scr_id == 2) {
    const bool en = (GUI.system_language == 2);
    if (f4_gate_title) {
      lv_label_set_text(f4_gate_title, en ? "Admin Settings" : "تنظیمات مدیریت");
      lv_obj_set_style_text_font(f4_gate_title, en ? &lv_font_montserrat_24 : &ui_font_naz24, 0);
    }
    if (f4_gate_sub) {
      lv_label_set_text(f4_gate_sub, en ? "Enter admin password" : "رمز مدیریت را وارد کنید");
      lv_obj_set_style_text_font(f4_gate_sub, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
    }
    if (f4_gate_err) {
      uint32_t rem = 0;
      if (f4_pin_is_locked(&rem)) {
        f4_gate_show_locked_error(rem);
        lv_obj_clear_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_label_set_text(f4_gate_err, en ? "Wrong password" : "رمز اشتباه");
        lv_obj_add_flag(f4_gate_err, LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (f4_gate_ta) {
      lv_textarea_set_text(f4_gate_ta, "");
      lv_textarea_set_password_mode(f4_gate_ta, true);  // هر ورود: پنهان
      if (f4_gate_eye_lbl) lv_label_set_text(f4_gate_eye_lbl, LV_SYMBOL_EYE_CLOSE);
      lv_obj_add_state(f4_gate_ta, LV_STATE_FOCUSED);
    }
    f4_gate_usb_head = 0;
    f4_gate_usb_tail = 0;
    lv_obj_set_pos(f4_gate_root, 0, 0);
    lv_obj_set_size(f4_gate_root, F4_GATE_W, 320);
    lv_obj_clear_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN);
    if (f4_gate_kb) lv_obj_clear_flag(f4_gate_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(f4_gate_root);
    if (f4_gate_home) lv_obj_move_foreground(f4_gate_home);
    GUI.lock_screen = false;
    f4_gate_open = true;
  } else {
    f4_gate_open = false;
    f4_gate_usb_head = 0;
    f4_gate_usb_tail = 0;
    lv_obj_add_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN);
  }
}

static void f4_style_admin_card(lv_obj_t *c, int x, int y, int w, int h) {
  if (!c) return;
  lv_obj_set_pos(c, x, y);
  lv_obj_set_size(c, w, h);
  lv_obj_set_style_radius(c, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(c, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(c, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(c, lv_color_hex(0xc6c6c8), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(c, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
}

static void f4_style_compact_title(lv_obj_t *fa, lv_obj_t *en_lbl) {
  if (fa) {
    lv_obj_set_width(fa, 124);
    lv_label_set_long_mode(fa, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(fa, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(fa, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (en_lbl) {
    lv_obj_set_width(en_lbl, 124);
    lv_label_set_long_mode(en_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(en_lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(en_lbl, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(en_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void f4_style_section_title(lv_obj_t *fa, lv_obj_t *en_lbl) {
  f4_style_compact_title(fa, en_lbl);
}

static void f4_err_sync_ui();  // پایین‌تر

static void f4_stop_sync_ui() {
  if (!f4_stop_btn || !f4_stop_title || !f4_stop_state) return;
  const bool en = (GUI.system_language == 2);
  if (en) {
    lv_label_set_text(f4_stop_title, "Stop");
    lv_obj_set_style_text_font(f4_stop_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_base_dir(f4_stop_title, LV_BASE_DIR_LTR, 0);
    lv_label_set_text(f4_stop_state, stopstate ? "Normally\nClosed" : "Normally\nOpen");
    lv_obj_set_style_text_font(f4_stop_state, &lv_font_montserrat_12, 0);
    lv_obj_set_style_base_dir(f4_stop_state, LV_BASE_DIR_LTR, 0);
    return;
  }
  lv_label_set_text(f4_stop_title, "\u0627\u0633\u062a\u067e");
  lv_obj_set_style_text_font(f4_stop_title, &ui_font_naz18b, 0);
  lv_obj_set_style_base_dir(f4_stop_title, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(f4_stop_state, en ? (stopstate ? "N.Closed" : "N.Open")
                                        : (stopstate ? "عادی بسته" : "عادی باز"));
  lv_obj_set_style_text_font(f4_stop_state, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_base_dir(f4_stop_state, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
}

static void f4_stop_btn_cb(lv_event_t *e) {
  const lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_RELEASED || !f4_tap_accept(e)) return;
  if (!f4_unlocked || GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  stopstate = !stopstate;
  settings_mark_dirty();  // NVS فقط با Save هدر
  f4_stop_sync_ui();
}

static void f4_pin_eye_cb(lv_event_t *e) {
  const lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_RELEASED || !f4_tap_accept(e)) return;
  if (!f4_unlocked || GUI.lock_screen || !f4_pin1 || !f4_pin2) return;
  f4_pin_revealed = !f4_pin_revealed;
  lv_textarea_set_password_mode(f4_pin1, !f4_pin_revealed);
  lv_textarea_set_password_mode(f4_pin2, !f4_pin_revealed);
  if (f4_pin_eye_lbl)
    lv_label_set_text(f4_pin_eye_lbl, f4_pin_revealed ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
  GUI.buzzer_cmd_short = true;
}

static bool f4_pin_digits_valid(const char *pin) {
  const size_t len = pin ? strlen(pin) : 0U;
  return len >= 3U && len <= 8U;
}

static void f4_pin_confirm_set_enabled(bool enabled) {
  if (!f4_pin2) return;
  if (enabled) {
    lv_obj_clear_state(f4_pin2, LV_STATE_DISABLED);
    lv_obj_add_flag(f4_pin2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(f4_pin2, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    return;
  }
  lv_textarea_set_text(f4_pin2, "");
  lv_obj_clear_state(f4_pin2, LV_STATE_FOCUSED);
  lv_obj_add_state(f4_pin2, LV_STATE_DISABLED);
  lv_obj_clear_flag(f4_pin2, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(f4_pin2, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

// [UI/Task2] Editing the new PIN always requires a fresh confirmation.
static void f4_pin_new_changed_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) f4_pin_confirm_set_enabled(false);
}

static bool f4_pin_validate_confirmation(bool show_empty_error) {
  const char *a = f4_pin1 ? lv_textarea_get_text(f4_pin1) : "";
  const char *b = f4_pin2 ? lv_textarea_get_text(f4_pin2) : "";
  if (!a || !a[0]) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمز جدید را وارد کنید", "Enter the new PIN", 1800);
    return false;
  }
  if (!f4_pin_digits_valid(a)) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمز جدید باید ۳ تا ۸ رقم باشد", "New PIN must be 3 to 8 digits", 1800);
    return false;
  }
  if (!b || !b[0]) {
    if (show_empty_error) {
      GUI.buzzer_cmd_long = true;
      ui_toast("تکرار رمز را وارد کنید", "Enter the PIN confirmation", 1800);
    }
    return false;
  }
  if (!f4_pin_digits_valid(b)) {
    GUI.buzzer_cmd_long = true;
    ui_toast("تکرار رمز باید ۳ تا ۸ رقم باشد", "PIN confirmation must be 3 to 8 digits", 1800);
    return false;
  }
  if (strcmp(a, b) != 0) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمزها یکسان نیستند", "PIN mismatch", 1800);
    return false;
  }
  return true;
}

static void f4_apply_lang() {
  const bool en = (GUI.system_language == 2);
  // عناوین انگلیسی کارت‌های تب تایمرها ۷px پایین‌تر بنشینند (فارسی سر جای درستش است)
  {
    lv_obj_t *f4_en_titles[] = {objects.lablee2_24, objects.lablee2_27, f4_day_title,
                                f4_stop_title, f4_clk_lbl};
    static int f4_en_title_y0[sizeof(f4_en_titles) / sizeof(f4_en_titles[0])] = {0};
    static bool f4_en_title_captured = false;
    for (uint8_t i = 0; i < sizeof(f4_en_titles) / sizeof(f4_en_titles[0]); ++i) {
      lv_obj_t *l = f4_en_titles[i];
      if (!l) continue;
      if (!f4_en_title_captured) f4_en_title_y0[i] = (int)lv_obj_get_y(l);
      lv_obj_set_y(l, f4_en_title_y0[i] + (en ? 7 : 0));
    }
    f4_en_title_captured = true;
  }
  if (f4_day_title) {
    lv_label_set_text(f4_day_title, en ? "Day Lock" : "روزشمار قفل");
    lv_obj_set_style_text_font(f4_day_title, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(f4_day_title, lv_color_hex(0x1c1c1e), 0);
  }
  if (objects.lablef2_27) {
    lv_label_set_text(objects.lablef2_27, "جهت حرکت موتور");
    lv_obj_set_style_text_font(objects.lablef2_27, &ui_font_naz18b, 0);
    lv_obj_set_width(objects.lablef2_27, 124);
    lv_label_set_long_mode(objects.lablef2_27, LV_LABEL_LONG_WRAP);
  }
  if (objects.lablee2_27) {
    lv_label_set_text(objects.lablee2_27, "Motor direction");
    lv_obj_set_style_text_font(objects.lablee2_27, &lv_font_montserrat_14, 0);
    lv_obj_set_width(objects.lablee2_27, 124);
  }
  if (objects.lablef2_24) {
    lv_label_set_text(objects.lablef2_24, "تاخیر روشن شدن تیغه");
    lv_obj_set_style_text_font(objects.lablef2_24, &ui_font_naz18b, 0);
    // فاصله حروف کمی بیشتر از −۲؛ کمتر از بازتر قبلی
    lv_obj_set_style_text_letter_space(objects.lablef2_24, -2, 0);
    lv_obj_set_style_text_line_space(objects.lablef2_24, 0, 0);
    lv_obj_set_width(objects.lablef2_24, 118);
    lv_label_set_long_mode(objects.lablef2_24, LV_LABEL_LONG_CLIP);
  }
  if (objects.lablee2_24) {
    lv_label_set_text(objects.lablee2_24, "Blade start delay");
    lv_obj_set_style_text_font(objects.lablee2_24, &lv_font_montserrat_12, 0);
    lv_obj_set_width(objects.lablee2_24, 110);
  }
  if (objects.lablef_dir_chap) lv_label_set_text(objects.lablef_dir_chap, "چپ");
  if (objects.lablef_dir_rast) lv_label_set_text(objects.lablef_dir_rast, "راست");
  if (objects.lablee_dir_chap) lv_label_set_text(objects.lablee_dir_chap, "Left");
  if (objects.lablee_dir_rast) lv_label_set_text(objects.lablee_dir_rast, "Right");
    if (f4_fac_lbl) {
        lv_label_set_text(f4_fac_lbl, en ? "Save factory default" : "ذخیره پیش‌فرض کارخانه");
        lv_obj_set_style_text_font(f4_fac_lbl, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
        /* This label is created on a button whose base direction can be
           inherited from the active page.  Pin it here so Persian word order
           and centering never depend on the page that was opened before F4. */
        lv_obj_set_width(f4_fac_lbl, 213);
        lv_obj_set_style_base_dir(f4_fac_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_align(f4_fac_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(f4_fac_lbl);
    }
  if (f4_cal_lbl) {
    lv_label_set_text(f4_cal_lbl, en ? "Calibrate steps/mm" : "کالیبره عدد استپر");
    lv_obj_set_style_text_font(f4_cal_lbl, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
    lv_obj_set_width(f4_cal_lbl, 143);
    lv_label_set_long_mode(f4_cal_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(f4_cal_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(f4_cal_lbl);
  }
  if (f4_pin_lbl) {
    lv_label_set_text(f4_pin_lbl, en ? "New PIN" : "رمز جدید");
    lv_obj_set_style_text_font(f4_pin_lbl, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_align(f4_pin_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(f4_pin_lbl, 106);
    lv_obj_set_style_text_letter_space(f4_pin_lbl, 0, 0);
    lv_obj_set_style_base_dir(f4_pin_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  }
  if (f4_pin_cfm_cap) {
    lv_label_set_text(f4_pin_cfm_cap, en ? "Confirm PIN" : "تکرار رمز");
    lv_obj_set_style_text_font(f4_pin_cfm_cap, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_align(f4_pin_cfm_cap, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(f4_pin_cfm_cap, 106);
    lv_obj_set_style_text_letter_space(f4_pin_cfm_cap, 0, 0);
    lv_obj_set_style_base_dir(f4_pin_cfm_cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  }
  if (f4_pin1) {
    lv_textarea_set_placeholder_text(f4_pin1, en ? "PIN" : "\uFEAD\uFEE3\uFEB0");
    lv_obj_set_style_text_font(f4_pin1, en ? &lv_font_montserrat_20 : &ui_font_naz18b,
                               LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(f4_pin1, LV_BASE_DIR_LTR, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin1, LV_TEXT_ALIGN_CENTER, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_letter_space(f4_pin1, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin1, LV_TEXT_ALIGN_CENTER, 0);
  }
  if (f4_pin2) {
    lv_textarea_set_placeholder_text(f4_pin2, en ? "Confirm" : "\uFE97\uFB91\uFEAE\uFE8D\uFEAD");
    lv_obj_set_style_text_font(f4_pin2, en ? &lv_font_montserrat_20 : &ui_font_naz18b,
                               LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(f4_pin2, LV_BASE_DIR_LTR, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin2, LV_TEXT_ALIGN_CENTER, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_letter_space(f4_pin2, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin2, LV_TEXT_ALIGN_CENTER, 0);
  }
  if (f4_clk_lbl) lv_obj_add_flag(f4_clk_lbl, LV_OBJ_FLAG_HIDDEN);
  if (f4_hh_cap) {
    lv_label_set_text(f4_hh_cap, en ? "Hour" : "ساعت");
    lv_obj_set_style_text_font(f4_hh_cap, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  }
  if (f4_mi_cap) {
    lv_label_set_text(f4_mi_cap, en ? "Minute" : "دقیقه");
    lv_obj_set_style_text_font(f4_mi_cap, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  }
  if (f4_date_lbl) lv_obj_add_flag(f4_date_lbl, LV_OBJ_FLAG_HIDDEN);
  if (f4_dd_cap) {
    lv_label_set_text(f4_dd_cap, en ? "Day" : "روز");
    lv_obj_set_style_text_font(f4_dd_cap, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  }
  if (f4_mo_cap) {
    lv_label_set_text(f4_mo_cap, en ? "Month" : "ماه");
    lv_obj_set_style_text_font(f4_mo_cap, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  }
  if (f4_yy_cap) {
    lv_label_set_text(f4_yy_cap, en ? "Year" : "سال");
    lv_obj_set_style_text_font(f4_yy_cap, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  }
  f4_err_sync_ui();
}

static void f4_fac_cfm_close() {
  if (f4_fac_cfm && lv_obj_is_valid(f4_fac_cfm)) lv_obj_delete(f4_fac_cfm);
  f4_fac_cfm = f4_fac_cfm_bar = f4_fac_cfm_track = f4_fac_cfm_count = NULL;
  f4_fac_cfm_holding = false;
}

// The overlay itself owns touch interception.  It must never change GUI.lock_screen,
// because that flag is also owned by independent safety and device-lock overlays.
static void f4_fac_cfm_backdrop_cb(lv_event_t *e) {
  (void)e;
}

static void f4_fac_cfm_yes_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  lv_async_call([](void *) {
    f4_fac_cfm_close();
    save_factory_snapshot();
    GUI.buzzer_cmd_short = true;
    ui_toast("تنظیمات کارخانه ذخیره شد", "Factory settings saved", 2000);
  }, NULL);
}

static void f4_fac_cfm_no_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  lv_async_call([](void *) { f4_fac_cfm_close(); }, NULL);
}

static void f4_fac_cfm_tick() {
  if (!f4_fac_cfm || !lv_obj_is_valid(f4_fac_cfm)) return;
  uint32_t el = millis() - f4_fac_cfm_t0;
  const uint32_t MS = 6000;
  if (f4_fac_cfm_bar) {
    int w = (int)((el >= MS) ? 320 : (320UL * el) / MS);
    lv_obj_set_width(f4_fac_cfm_bar, w);
  }
  if (f4_fac_cfm_count) {
    uint32_t sec = (el >= MS) ? 0 : (MS - el + 999UL) / 1000UL;
    char b[24];
    snprintf(b, sizeof(b), "%lus", (unsigned long)sec);
    lv_label_set_text(f4_fac_cfm_count, b);
  }
  if (el >= MS && !f4_fac_cfm_holding) f4_fac_cfm_close();
}

static void f4_fac_save_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_RELEASED && c != LV_EVENT_CLICKED) return;
  if (!f4_tap_accept(e)) return;
  if (!f4_unlocked || GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  if (f4_fac_cfm && lv_obj_is_valid(f4_fac_cfm)) return;
  const bool en = (GUI.system_language == 2);
  f4_fac_cfm = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(f4_fac_cfm, 0, 0);
  lv_obj_set_size(f4_fac_cfm, 480, 320);
  lv_obj_set_style_bg_color(f4_fac_cfm, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f4_fac_cfm, LV_OPA_50, 0);
  lv_obj_set_style_border_width(f4_fac_cfm, 0, 0);
  lv_obj_clear_flag(f4_fac_cfm, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_fac_cfm, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_fac_cfm, f4_fac_cfm_backdrop_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_fac_cfm, f4_fac_cfm_backdrop_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *card = lv_obj_create(f4_fac_cfm);
  lv_obj_set_size(card, 356, 176);
  lv_obj_center(card);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 324);
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(msg, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_label_set_text(msg, en ? "Save the current settings as the factory default?" : "تنظیمات فعلی به‌عنوان پیش‌فرض کارخانه ذخیره شود؟");
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 14);
  lv_obj_t *hint = lv_label_create(card);
  lv_obj_set_width(hint, 324);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x6e6e73), 0);
  lv_obj_set_style_text_font(hint, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_label_set_text(hint, en ? "This confirmation closes automatically in 6 seconds." : "این تأیید تا ۶ ثانیه دیگر خودکار بسته می‌شود.");
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 58);
  f4_fac_cfm_track = lv_obj_create(card);
  lv_obj_set_pos(f4_fac_cfm_track, 18, 88);
  lv_obj_set_size(f4_fac_cfm_track, 320, 6);
  lv_obj_set_style_bg_color(f4_fac_cfm_track, lv_color_hex(0xe5e5ea), 0);
  lv_obj_set_style_radius(f4_fac_cfm_track, 3, 0);
  lv_obj_set_style_border_width(f4_fac_cfm_track, 0, 0);
  lv_obj_clear_flag(f4_fac_cfm_track, LV_OBJ_FLAG_CLICKABLE);
  f4_fac_cfm_bar = lv_obj_create(f4_fac_cfm_track);
  lv_obj_set_pos(f4_fac_cfm_bar, 0, 0);
  lv_obj_set_size(f4_fac_cfm_bar, 1, 6);
  lv_obj_set_style_bg_color(f4_fac_cfm_bar, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_radius(f4_fac_cfm_bar, 4, 0);
  lv_obj_set_style_border_width(f4_fac_cfm_bar, 0, 0);
  lv_obj_clear_flag(f4_fac_cfm_bar, LV_OBJ_FLAG_CLICKABLE);
  f4_fac_cfm_count = lv_label_create(card);
  lv_obj_set_style_text_font(f4_fac_cfm_count, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(f4_fac_cfm_count, lv_color_hex(0x6e6e73), 0);
  lv_label_set_text(f4_fac_cfm_count, "6s");
  lv_obj_align(f4_fac_cfm_count, LV_ALIGN_TOP_RIGHT, -18, 99);
  lv_obj_t *bn = lv_button_create(card);
  lv_obj_set_size(bn, 158, 42);
  lv_obj_align(bn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_set_style_bg_color(bn, lv_color_hex(0x587a9f), 0);
  lv_obj_set_style_radius(bn, 12, 0);
  lv_obj_set_style_border_width(bn, 0, 0);
  lv_obj_set_style_shadow_width(bn, 0, 0);
  lv_obj_add_event_cb(bn, f4_fac_cfm_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *ln = lv_label_create(bn);
  lv_label_set_text(ln, en ? "Cancel" : "انصراف");
  lv_obj_set_style_text_font(ln, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(ln, lv_color_hex(0xffffff), 0);
  lv_obj_center(ln);
  lv_obj_t *by = lv_button_create(card);
  lv_obj_set_size(by, 158, 42);
  lv_obj_align(by, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
  lv_obj_set_style_bg_color(by, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_radius(by, 12, 0);
  lv_obj_set_style_border_width(by, 0, 0);
  lv_obj_set_style_shadow_width(by, 0, 0);
  lv_obj_add_event_cb(by, f4_fac_cfm_yes_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *ly = lv_label_create(by);
  lv_label_set_text(ly, en ? "Save" : "ذخیره");
  lv_obj_set_style_text_font(ly, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(ly, lv_color_hex(0xffffff), 0);
  lv_obj_center(ly);
  f4_fac_cfm_t0 = millis();
  f4_fac_cfm_holding = false;
}

void f4_kb_hide() {
  if (f4_kb_root && !lv_obj_is_valid(f4_kb_root)) f4_kb_root = NULL;
  if (f4_kb && !lv_obj_is_valid(f4_kb)) f4_kb = NULL;
  if (f4_kb_del && !lv_obj_is_valid(f4_kb_del)) f4_kb_del = NULL;
  if (f4_kb_ok && !lv_obj_is_valid(f4_kb_ok)) f4_kb_ok = NULL;
  if (f4_kb_handle && !lv_obj_is_valid(f4_kb_handle)) f4_kb_handle = NULL;
  if (f4_kb_root) lv_obj_add_flag(f4_kb_root, LV_OBJ_FLAG_HIDDEN);
  else {
    if (f4_kb) lv_obj_add_flag(f4_kb, LV_OBJ_FLAG_HIDDEN);
    if (f4_kb_del) lv_obj_add_flag(f4_kb_del, LV_OBJ_FLAG_HIDDEN);
    if (f4_kb_ok) lv_obj_add_flag(f4_kb_ok, LV_OBJ_FLAG_HIDDEN);
  }
  f4_kb_dragging = false;
}

static bool f4_is_num_ta(lv_obj_t *o) {
  return o && (o == f4_pin1 || o == f4_pin2 || o == f4_ta_hh || o == f4_ta_mi ||
               o == f4_ta_dd || o == f4_ta_mo || o == f4_ta_yy || o == f4_day_ta ||
               o == f4_timer_ta || o == f4_err_ta);
}

static int f4_ta_int(lv_obj_t *ta, int defv);  // forward — پایین‌تر تعریف شده
static bool f4_ta_matches_text(lv_obj_t *ta, const char *expected);

// [UI/Task2] Rebuild a bounded numeric textarea so its baseline never scrolls while typing.
static bool f4_ta_append_char_stable(lv_obj_t *ta, char ch) {
  if (!ta) return false;
  const char *old = lv_textarea_get_text(ta);
  const size_t len = old ? strlen(old) : 0U;
  if (len >= sizeof(char[16]) - 1U || len >= lv_textarea_get_max_length(ta)) return false;
  char b[16];
  if (len) memcpy(b, old, len);
  b[len] = ch;
  b[len + 1U] = '\0';
  lv_textarea_set_text(ta, b);
  lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
  lv_obj_scroll_to(ta, 0, 0, LV_ANIM_OFF);
  return true;
}

static bool f4_ta_delete_char_stable(lv_obj_t *ta) {
  if (!ta) return false;
  const char *old = lv_textarea_get_text(ta);
  const size_t len = old ? strlen(old) : 0U;
  if (!len) return false;
  char b[16];
  const size_t kept = len - 1U;
  if (kept) memcpy(b, old, kept);
  b[kept] = '\0';
  lv_textarea_set_text(ta, b);
  lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
  lv_obj_scroll_to(ta, 0, 0, LV_ANIM_OFF);
  return true;
}

static bool f4_restore_invalid_int(lv_obj_t *ta, int fallback, int min_value, int max_value, int width) {
  const char *raw = ta ? lv_textarea_get_text(ta) : NULL;
  int value = 0;
  bool valid = raw && raw[0];
  for (const char *p = raw; valid && *p; ++p) {
    if (*p < '0' || *p > '9') valid = false;
    else value = value * 10 + (*p - '0');
  }
  if (valid && value >= min_value && value <= max_value) return false;
  char b[8];
  snprintf(b, sizeof(b), "%0*d", width, fallback);
  lv_textarea_set_text(ta, b);
  GUI.buzzer_cmd_long = true;
  ui_toast("مقدار نامعتبر است", "Invalid value restored", 1500);
  return true;
}

static void f4_validate_active_datetime() {
  if (!f4_active_ta) return;
  int dd = f4_d, mo = f4_mo, yy = f4_y;
  if (GUI.system_language == 1) gregorian_to_jalali(f4_y, f4_mo, f4_d, &yy, &mo, &dd);
  bool restored = false;
  if (f4_active_ta == f4_ta_hh) restored = f4_restore_invalid_int(f4_ta_hh, f4_hh, 0, 23, 2);
  else if (f4_active_ta == f4_ta_mi) restored = f4_restore_invalid_int(f4_ta_mi, f4_mi, 0, 59, 2);
  else if (f4_active_ta == f4_ta_dd) restored = f4_restore_invalid_int(f4_ta_dd, dd, 1, 31, 2);
  else if (f4_active_ta == f4_ta_mo) restored = f4_restore_invalid_int(f4_ta_mo, mo, 1, 12, 2);
  else if (f4_active_ta == f4_ta_yy)
    restored = f4_restore_invalid_int(f4_ta_yy, yy, GUI.system_language == 1 ? 1300 : 2000, 9999, 4);
  if (restored && (f4_active_ta == f4_ta_hh || f4_active_ta == f4_ta_mi)) f4_clock_time_edited = false;
}

static void f4_day_ta_commit() {
  if (!f4_day_ta || !f4_unlocked || GUI.lock_screen) return;
  const char *day_raw = lv_textarea_get_text(f4_day_ta);
  if (day_raw && day_raw[0]) {
    if (f4_restore_invalid_int(f4_day_ta, day_lock_remain, 0, F4_DAY_ROLLER_MAX, 1)) return;
  }
  // کادر خالی = صفر؛ ارور نمی‌دهد (درخواست مالک)
  int v = (day_raw && day_raw[0]) ? f4_ta_int(f4_day_ta, 0) : 0;
  if (v < 0) v = 0;
  if (v > F4_DAY_ROLLER_MAX) v = F4_DAY_ROLLER_MAX;
  char b[8];
  snprintf(b, sizeof(b), "%d", v);
  lv_textarea_set_text(f4_day_ta, b);
  if (v == day_lock_remain) return;
  day_lock_t1_from_chain = false;  // مقداردهی مستقیم کارت روزشمار، مالک مرحلهٔ اول است
  day_lock_start_chain(v);
  save_admin_svc_state();
  GUI.buzzer_cmd_short = true;
}

static void f4_err_fmt(char *buf, size_t n, float v) {
  float r = roundf(v * 10.0f) / 10.0f;
  if (fabsf(r - roundf(r)) < 0.05f)
    snprintf(buf, n, "%d", (int)lroundf(r));
  else
    snprintf(buf, n, "%.1f", (double)r);
}

static void f4_err_ta_commit() {
  if (!f4_err_ta || !f4_unlocked || GUI.lock_screen) return;
  const char *raw = lv_textarea_get_text(f4_err_ta);
  bool valid = raw && raw[0];
  uint8_t whole_digits = 0;
  uint8_t fraction_digits = 0;
  bool decimal_seen = false;
  for (const char *p = raw; valid && *p; ++p) {
    if (*p == '-' && p == raw) continue;
    if (*p == '.' && !decimal_seen) { decimal_seen = true; continue; }
    if (*p < '0' || *p > '9') { valid = false; break; }
    if (decimal_seen) ++fraction_digits;
    else ++whole_digits;
  }
  if (!valid || whole_digits == 0 || whole_digits > 2 || fraction_digits > 1 || (decimal_seen && fraction_digits == 0)) {
    char prior[8];
    f4_err_fmt(prior, sizeof(prior), blade_err_mm);
    lv_textarea_set_text(f4_err_ta, prior);
    GUI.buzzer_cmd_long = true;
    ui_toast("مقدار نامعتبر است", "Invalid value restored", 1500);
    return;
  }
  float v = (float)atof(raw);
  v = roundf(v * 10.0f) / 10.0f;
  if (v > 9.9f || v < -9.9f || fabsf(v) < 0.1f) {
    char prior[8];
    f4_err_fmt(prior, sizeof(prior), blade_err_mm);
    lv_textarea_set_text(f4_err_ta, prior);
    GUI.buzzer_cmd_long = true;
    ui_toast("مقدار نامعتبر است", "Invalid value restored", 1500);
    return;
  }
  blade_err_mm = v;
  PEND.blade_err_mm = v;
  char b[8];
  f4_err_fmt(b, sizeof(b), v);
  lv_textarea_set_text(f4_err_ta, b);
  settings_mark_dirty();
  GUI.buzzer_cmd_short = true;
  // تغییر مقدار اصلاح، مقصد روی صفحه را تغییر نمی‌دهد.
  STP.position_Setpoint_p = blade_err_motor_target_p(STP.Man_Sp_mm);
  AUT.in_position_flag = false;
  STP.in_position_flag = false;
  extern void blade_err_badge_sync();
  blade_err_badge_sync();
}

static void f4_err_sync_ui() {
  const bool on = blade_err_enable;
  if (f4_err_btn) {
    lv_obj_set_style_bg_color(f4_err_btn, lv_color_hex(on ? 0x0a84ff : 0x5a7ea3), 0);
    lv_obj_set_style_bg_opa(f4_err_btn, LV_OPA_COVER, 0);
  }
  if (f4_err_btn_lbl) {
    const bool en = (GUI.system_language == 2);
    lv_label_set_text(f4_err_btn_lbl, en ? "Error Offset" : "اصلاح خطا");
    lv_obj_set_style_text_font(f4_err_btn_lbl, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(f4_err_btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_opa(f4_err_btn_lbl, on ? LV_OPA_COVER : LV_OPA_COVER, 0);
    lv_obj_set_width(f4_err_btn_lbl, 118);
    lv_obj_set_style_text_align(f4_err_btn_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(f4_err_btn_lbl, LV_ALIGN_LEFT_MID, 6, 0);
  }
  if (f4_err_num_box) {
    if (on) lv_obj_clear_flag(f4_err_num_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(f4_err_num_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (f4_err_ta) {
    if (on) {
      lv_obj_clear_flag(f4_err_ta, LV_OBJ_FLAG_HIDDEN);
      if (f4_active_ta != f4_err_ta) {
        char b[8];
        f4_err_fmt(b, sizeof(b), blade_err_mm);
        lv_textarea_set_text(f4_err_ta, b);
      }
    } else {
      lv_obj_add_flag(f4_err_ta, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (f4_err_unit) {
    if (on) lv_obj_clear_flag(f4_err_unit, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(f4_err_unit, LV_OBJ_FLAG_HIDDEN);
  }
  extern void blade_err_badge_sync();
  blade_err_badge_sync();
}

static void f4_err_btn_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_RELEASED) return;
  if (!f4_tap_accept(e)) return;
  lv_obj_t *t = (lv_obj_t *)lv_event_get_target(e);
  if (t != f4_err_btn && t != f4_err_btn_lbl) return;  // عدد سفید = ویرایش، نه toggle
  if (!f4_unlocked || GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  blade_err_enable = !blade_err_enable;
  if (blade_err_enable) {
    blade_err_armed = false;  // اعمال فقط با لمس دایره روی اصلی
    if (fabsf(blade_err_mm) < 0.1f) {
      blade_err_mm = 2.0f;
      PEND.blade_err_mm = blade_err_mm;
    }
  } else {
    extern void blade_err_disarm_restore_destination();
    blade_err_disarm_restore_destination();
  }
  PEND.blade_err_enable = blade_err_enable;
  settings_mark_dirty();
  GUI.buzzer_cmd_short = true;
  f4_err_sync_ui();
}

static void f4_err_ta_ready_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) f4_err_ta_commit();
}

static void f4_timer_ta_commit() {
  if (!f4_timer_ta || !f4_unlocked || GUI.lock_screen) return;
  int cur = (int)constrain(PEND.cut_prcs_timer / 1000, 0, F4_CUT_TIMER_MAX_S);
  // خالی/ناعددی → مقدار قبلی؛ عددِ خارج از محدوده → محدود به سقف/کف (همسو با مسیر +/−)
  const char *raw = lv_textarea_get_text(f4_timer_ta);
  int sec = 0;
  bool valid = raw && raw[0];
  for (const char *p = raw; valid && *p; ++p) {
    if (*p < '0' || *p > '9') valid = false;
    else sec = sec * 10 + (*p - '0');
  }
  if (!valid) {
    char rb[8];
    snprintf(rb, sizeof(rb), "%d", cur);
    lv_textarea_set_text(f4_timer_ta, rb);
    GUI.buzzer_cmd_long = true;
    ui_toast("مقدار نامعتبر است", "Invalid value restored", 1500);
    return;
  }
  const bool clamped = (sec < 0 || sec > F4_CUT_TIMER_MAX_S);
  if (sec < 0) sec = 0;
  if (sec > F4_CUT_TIMER_MAX_S) sec = F4_CUT_TIMER_MAX_S;
  char b[8];
  snprintf(b, sizeof(b), "%d", sec);
  lv_textarea_set_text(f4_timer_ta, b);
  if (clamped) ui_toast("به محدوده مجاز محدود شد", "Clamped to allowed range", 1500);
  if (sec == cur) return;
  PEND.cut_prcs_timer = sec * 1000;
  settings_mark_dirty();
  GUI.buzzer_cmd_short = true;
}

static void f4_commit_active_num() {
  if (f4_active_ta == f4_day_ta) f4_day_ta_commit();
  else if (f4_active_ta == f4_timer_ta) f4_timer_ta_commit();
  else if (f4_active_ta == f4_err_ta) f4_err_ta_commit();
  else if (f4_active_ta == f4_pin1) { }
  else if (f4_active_ta == f4_pin2) (void)f4_pin_validate_confirmation(true);
  else f4_validate_active_datetime();
}

static void f4_clear_focus_style() {
  if (f4_active_ta && !lv_obj_is_valid(f4_active_ta)) f4_active_ta = NULL;
  f4_commit_active_num();
  lv_obj_t *all[] = {f4_pin1, f4_pin2, f4_ta_hh, f4_ta_mi, f4_ta_dd, f4_ta_mo, f4_ta_yy,
                     f4_day_ta, f4_timer_ta, f4_err_ta};
  for (unsigned i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
    if (!all[i] || !lv_obj_is_valid(all[i])) continue;
    lv_obj_clear_state(all[i], LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(all[i], lv_color_hex(0xc6c6c8), 0);
  }
  f4_active_ta = NULL;
  f4_usb_edit_armed = false;
}

static void f4_blank_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_PRESSED) return;
  lv_obj_t *t = (lv_obj_t *)lv_event_get_target(e);
  if (f4_is_num_ta(t)) return;
  if (f4_kb && (t == f4_kb || lv_obj_get_parent(t) == f4_kb)) return;
  if (t == f4_kb_del || t == f4_kb_ok || t == f4_kb_handle || t == f4_kb_root) return;
  if (f4_kb_root && (t == f4_kb_root || lv_obj_get_parent(t) == f4_kb_root)) return;
  // اگر تارگت فرزندِ یک کادر عدد بود رد شو
  lv_obj_t *p = t;
  for (int i = 0; i < 4 && p; i++, p = lv_obj_get_parent(p)) {
    if (f4_is_num_ta(p)) return;
  }
  f4_kb_hide();
  f4_clear_focus_style();
}

static inline bool f4_try_enter();  // forward

static void f4_kb_btn_event(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  uint32_t id = lv_buttonmatrix_get_selected_button(obj);
  const char *txt = lv_buttonmatrix_get_button_text(obj, id);
  if (!f4_unlocked || !f4_active_ta || GUI.lock_screen) {
    if (GUI.lock_screen) f4_kb_hide();
    return;
  }
  if (!txt) return;
  if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    (void)f4_ta_delete_char_stable(f4_active_ta);
    settings_mark_dirty();
  } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    f4_try_enter();
  } else if (txt[0] >= '0' && txt[0] <= '9' && txt[1] == 0) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    (void)f4_ta_append_char_stable(f4_active_ta, txt[0]);
    settings_mark_dirty();
  } else if (txt[0] == '.' && txt[1] == 0 && f4_active_ta == f4_err_ta) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    (void)f4_ta_append_char_stable(f4_active_ta, '.');
    settings_mark_dirty();
  }
}

static void f4_kb_draw_cb(lv_event_t *e) {
  lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
  if (!draw_task) return;
  lv_draw_dsc_base_t *base = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
  if (!base || base->part != LV_PART_ITEMS) return;
  const uint32_t id = base->id1;
  uint32_t bg = 0;
  if (id == 9) bg = 0xD92D20u;       // پاک کردن (چپ): قرمز
  else if (id == 11) bg = 0x1C9B67u; // تایید (راست): سبز
  else return;
  lv_draw_fill_dsc_t *fill = lv_draw_task_get_fill_dsc(draw_task);
  if (fill) fill->color = lv_color_hex(bg);
  lv_draw_label_dsc_t *lab = lv_draw_task_get_label_dsc(draw_task);
  if (lab) lab->color = lv_color_hex(0xffffff);
}

static void f4_kb_del_cb(lv_event_t *e) {
  (void)e;
}

static void f4_kb_ok_cb(lv_event_t *e) {
  (void)e;
}

static void f4_kb_drag_cb(lv_event_t *e) {
  if (!f4_kb_root) return;
  lv_event_code_t c = lv_event_get_code(e);
  lv_indev_t *in = lv_indev_active();
  if (!in) return;
  lv_point_t p;
  lv_indev_get_point(in, &p);
  if (c == LV_EVENT_PRESSED) {
    f4_kb_dragging = true;
    f4_kb_drag_pt = p;
    f4_kb_drag_ox = (int)lv_obj_get_x(f4_kb_root);
    f4_kb_drag_oy = (int)lv_obj_get_y(f4_kb_root);
    return;
  }
  if (c == LV_EVENT_PRESSING && f4_kb_dragging) {
    int nx = f4_kb_drag_ox + ((int)p.x - (int)f4_kb_drag_pt.x);
    int ny = f4_kb_drag_oy + ((int)p.y - (int)f4_kb_drag_pt.y);
    const int kw = (int)lv_obj_get_width(f4_kb_root);
    const int kh = (int)lv_obj_get_height(f4_kb_root);
    if (nx < 0) nx = 0;
    if (ny < 48) ny = 48;
    if (nx > 480 - kw) nx = 480 - kw;
    if (ny > 320 - kh) ny = 320 - kh;
    lv_obj_set_pos(f4_kb_root, nx, ny);
    f4_kb_moved = true;
    return;
  }
  if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST) f4_kb_dragging = false;
}

static void f4_kb_show_for(lv_obj_t *ta) {
  numpad_close();
  f2_hide_line_kb();
  static const char *map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
  };
  const int ROOT_W = 280;
  const int ROOT_H = 220;
  bool new_root = false;
  if (!f4_kb_root || !lv_obj_is_valid(f4_kb_root)) {
    new_root = true;
    f4_kb_root = lv_obj_create(lv_layer_top());
    lv_obj_set_size(f4_kb_root, ROOT_W, ROOT_H);
    lv_obj_set_style_bg_color(f4_kb_root, lv_color_hex(0x2c2c2e), 0);
    lv_obj_set_style_bg_opa(f4_kb_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(f4_kb_root, 1, 0);
    lv_obj_set_style_border_color(f4_kb_root, lv_color_hex(0x48484A), 0);
    lv_obj_set_style_pad_all(f4_kb_root, 0, 0);
    lv_obj_set_style_shadow_width(f4_kb_root, 0, 0);
    lv_obj_clear_flag(f4_kb_root, LV_OBJ_FLAG_SCROLLABLE);
    f4_kb = f4_kb_handle = NULL;

    f4_kb_handle = lv_obj_create(f4_kb_root);
    lv_obj_set_pos(f4_kb_handle, 0, 0);
    lv_obj_set_size(f4_kb_handle, ROOT_W, 28);
    lv_obj_set_style_bg_color(f4_kb_handle, lv_color_hex(0x3a3a3c), 0);
    lv_obj_set_style_bg_opa(f4_kb_handle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(f4_kb_handle, 0, 0);
    lv_obj_set_style_border_width(f4_kb_handle, 0, 0);
    lv_obj_set_style_pad_all(f4_kb_handle, 0, 0);
    lv_obj_clear_flag(f4_kb_handle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(f4_kb_handle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(f4_kb_handle, f4_kb_drag_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(f4_kb_handle, f4_kb_drag_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(f4_kb_handle, f4_kb_drag_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(f4_kb_handle, f4_kb_drag_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_t *grip = lv_obj_create(f4_kb_handle);
    lv_obj_set_size(grip, 48, 4);
    lv_obj_set_style_bg_color(grip, lv_color_hex(0x8e8e93), 0);
    lv_obj_set_style_bg_opa(grip, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(grip, 2, 0);
    lv_obj_set_style_border_width(grip, 0, 0);
    lv_obj_clear_flag(grip, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(grip, LV_ALIGN_CENTER, 0, 0);

    f4_kb = lv_buttonmatrix_create(f4_kb_root);
    lv_buttonmatrix_set_map(f4_kb, map);
    lv_obj_add_event_cb(f4_kb, f4_kb_btn_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(f4_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(f4_kb, f4_kb_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_set_style_bg_color(f4_kb, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(f4_kb, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f4_kb, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(f4_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(f4_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(f4_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f4_kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(f4_kb, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(f4_kb, lv_color_hex(0x1c1c1e), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(f4_kb, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f4_kb, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
  }
  lv_buttonmatrix_set_map(f4_kb, map);
  lv_buttonmatrix_clear_button_ctrl_all(f4_kb, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_obj_set_pos(f4_kb, 8, 34);
  lv_obj_set_size(f4_kb, 264, 176);
  if (new_root || !f4_kb_moved)
    lv_obj_set_pos(f4_kb_root, (ta == f4_timer_ta) ? 8 : 188, 62);
  lv_obj_clear_flag(f4_kb_root, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f4_kb_root);
}

static void f4_ta_focus_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  if (ta && lv_obj_has_state(ta, LV_STATE_DISABLED)) return;
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_FOCUSED && c != LV_EVENT_CLICKED && c != LV_EVENT_RELEASED) return;
  if (c == LV_EVENT_RELEASED || c == LV_EVENT_CLICKED) {
    if (!f4_tap_accept(e) && c != LV_EVENT_FOCUSED) return;
  }
  if (!f4_unlocked || GUI.lock_screen) { if (GUI.lock_screen) f4_kb_hide(); return; }
  f4_clear_focus_style();
  f4_active_ta = ta;
  lv_obj_add_state(ta, LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(ta, 2, 0);
  lv_obj_set_style_border_color(ta, lv_color_hex(0x0a84ff), 0);
  lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
  f4_usb_edit_armed = true;
  if (GUI.virtual_kb_enable && !usb_keyboard_present()) f4_kb_show_for(ta);
  else f4_kb_hide();
}

static bool f4_pin_open_confirmation() {
  const char *a = f4_pin1 ? lv_textarea_get_text(f4_pin1) : "";
  if (!a || !a[0]) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمز جدید را وارد کنید", "Enter the new PIN", 1800);
    return false;
  }
  if (!f4_pin_digits_valid(a)) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمز جدید باید ۳ تا ۸ رقم باشد", "New PIN must be 3 to 8 digits", 1800);
    return false;
  }
  f4_pin_confirm_set_enabled(true);
  f4_clear_focus_style();
  f4_active_ta = f4_pin2;
  lv_obj_add_state(f4_pin2, LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(f4_pin2, 2, 0);
  lv_obj_set_style_border_color(f4_pin2, lv_color_hex(0x0a84ff), 0);
  lv_textarea_set_cursor_pos(f4_pin2, LV_TEXTAREA_CURSOR_LAST);
  f4_usb_edit_armed = true;
  if (GUI.virtual_kb_enable && !usb_keyboard_present()) f4_kb_show_for(f4_pin2);
  else f4_kb_hide();
  GUI.buzzer_cmd_short = true;
  return true;
}

static lv_obj_t *f4_make_num_ta(lv_obj_t *p, int x, int y, int w, int max_len, const char *ph) {
  lv_obj_t *ta = lv_textarea_create(p);
  lv_obj_set_pos(ta, x, y);
  lv_obj_set_size(ta, w, 40);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_max_length(ta, max_len);
  lv_textarea_set_accepted_chars(ta, "0123456789");
  lv_textarea_set_placeholder_text(ta, ph);
  lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(ta, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_bg_color(ta, lv_color_hex(0xf2f2f7), 0);
  lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_top(ta, 4, 0);
  lv_obj_set_style_pad_bottom(ta, 6, 0);
  lv_obj_set_style_border_width(ta, 2, 0);
  lv_obj_set_style_border_color(ta, lv_color_hex(0xc6c6c8), 0);
  lv_obj_set_style_radius(ta, 10, 0);
  lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);  // عدد ثابت؛ اسکرول داخل کادر نه
  lv_obj_add_flag(ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_add_event_cb(ta, f4_ta_focus_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(ta, f4_ta_focus_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(ta, f4_ta_focus_cb, LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(ta, f4_ta_focus_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ta, f4_ta_focus_cb, LV_EVENT_RELEASED, NULL);
  return ta;
}

// [UI/Task2] Only a direct operator value freezes the two live time fields.
static void f4_clock_time_changed_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || f4_clock_syncing) return;
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  if (ta == f4_ta_hh || ta == f4_ta_mi) f4_clock_time_edited = true;
}

// USB / تایپ رقم روی کادر فوکوس‌شده F4 (یا دروازۀ رمز) — فقط منطق F4؛ بدون دست‌زدن به Task4/Host
static inline bool f4_try_digit(char d) {
  // فقط وقتی دروازه واقعاً دیده می‌شود رقم را ببلعد (نه پرچم گیرکرده)
  if (f4_gate_open && f4_gate_root && !lv_obj_has_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN)) {
    if (d >= '0' && d <= '9') f4_gate_usb_push(d);
    return true;
  }
  if (GUI.scr_id != 2) return false;
  if (!objects.tabview2_1 || (int)lv_tabview_get_tab_act(objects.tabview2_1) != 3) return false;
  if (!f4_unlocked || GUI.lock_screen) return false;
  // اگر هنوز فوکوس نشده، اولین کادر عدد را بگیر تا USB کار کند (Task4/Host دست نخورده)
  if (!f4_active_ta) {
    lv_obj_t *prefer = f4_day_ta ? f4_day_ta : (f4_pin1 ? f4_pin1 : f4_timer_ta);
    if (!prefer) return false;
    f4_active_ta = prefer;
    lv_obj_add_state(prefer, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(prefer, 2, 0);
    lv_obj_set_style_border_color(prefer, lv_color_hex(0x0a84ff), 0);
  }
  if (d >= '0' && d <= '9') {
    (void)f4_ta_append_char_stable(f4_active_ta, d);
    settings_mark_dirty();
    return true;
  }
  if ((d == '.' || d == '-') && f4_active_ta == f4_err_ta) {
    (void)f4_ta_append_char_stable(f4_active_ta, d);
    settings_mark_dirty();
    return true;
  }
  return false;
}
static inline bool f4_try_backspace() {
  if (f4_gate_open && f4_gate_root && !lv_obj_has_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN)) {
    f4_gate_usb_push('\b');
    return true;
  }
  if (GUI.scr_id != 2) return false;
  if (!objects.tabview2_1 || (int)lv_tabview_get_tab_act(objects.tabview2_1) != 3) return false;
  if (!f4_unlocked || GUI.lock_screen) return false;
  if (!f4_active_ta) {
    lv_obj_t *prefer = f4_day_ta ? f4_day_ta : (f4_pin1 ? f4_pin1 : f4_timer_ta);
    if (!prefer) return false;
    f4_active_ta = prefer;
  }
  (void)f4_ta_delete_char_stable(f4_active_ta);
  return true;
}
static inline bool f4_try_enter() {
  if (f4_gate_open && f4_gate_root && !lv_obj_has_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN)) {
    f4_gate_usb_push('\r');
    return true;
  }
  if (GUI.scr_id != 2) return false;
  if (!objects.tabview2_1 || (int)lv_tabview_get_tab_act(objects.tabview2_1) != 3) return false;
  if (!f4_unlocked || !f4_active_ta) return false;
  if (f4_active_ta == f4_day_ta || f4_active_ta == f4_timer_ta || f4_active_ta == f4_err_ta) {
    f4_kb_hide();
    f4_clear_focus_style();  // داخلش commit می‌شود
    return true;
  }
  if (f4_active_ta == f4_pin1) return f4_pin_open_confirmation();
  if (f4_active_ta == f4_pin2) return f4_pin_validate_confirmation(true);
  f4_kb_hide();
  return true;
}

// USB host runs outside the LVGL task.  It queues F4 keystrokes; Task2 alone edits widgets.
static inline bool f4_usb_enqueue(char c) {
  if (!f4_usb_edit_armed) return false;
  const uint8_t next = (uint8_t)((f4_usb_q_head + 1U) % sizeof(f4_usb_q));
  if (next == f4_usb_q_tail) return true;  // consume safely when the small queue is full
  f4_usb_q[f4_usb_q_head] = c;
  f4_usb_q_head = next;
  return true;
}

static inline bool f4_usb_try_digit(char d) {
  if (f4_gate_open) { f4_gate_usb_push(d); return true; }
  return f4_usb_enqueue(d);
}

static inline bool f4_usb_try_backspace() {
  if (f4_gate_open) { f4_gate_usb_push('\b'); return true; }
  return f4_usb_enqueue('\b');
}

static inline bool f4_usb_try_enter() {
  if (f4_gate_open) { f4_gate_usb_push('\r'); return true; }
  return f4_usb_enqueue('\r');
}

static void f4_usb_drain() {
  if (!f4_usb_edit_armed || !f4_active_ta || !f4_is_num_ta(f4_active_ta)) {
    f4_usb_q_tail = f4_usb_q_head;
    return;
  }
  while (f4_usb_q_tail != f4_usb_q_head) {
    const char c = f4_usb_q[f4_usb_q_tail];
    f4_usb_q_tail = (uint8_t)((f4_usb_q_tail + 1U) % sizeof(f4_usb_q));
    if (c == '\r') {
      (void)f4_try_enter();
      break;
    }
    if (c == '\b') {
      (void)f4_ta_delete_char_stable(f4_active_ta);
      settings_mark_dirty();
      continue;
    }
    if ((c >= '0' && c <= '9') || ((c == '.' || c == '-') && f4_active_ta == f4_err_ta)) {
      (void)f4_ta_append_char_stable(f4_active_ta, c);
      settings_mark_dirty();
    }
  }
}

static bool f4_pin_commit_from_header_save() {
  f4_kb_hide();
  const char *a = f4_pin1 ? lv_textarea_get_text(f4_pin1) : "";
  const char *b = f4_pin2 ? lv_textarea_get_text(f4_pin2) : "";
  if ((!a || !a[0]) && (!b || !b[0])) return true;
  if (!a || !a[0]) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمز جدید را وارد کنید", "Enter the new PIN", 1800);
    return false;
  }
  if (!f4_pin_digits_valid(a)) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمز جدید باید ۳ تا ۸ رقم باشد", "New PIN must be 3 to 8 digits", 1800);
    return false;
  }
  if (!b || !b[0]) {
    GUI.buzzer_cmd_long = true;
    ui_toast("تکرار رمز را وارد کنید", "Enter the PIN confirmation", 1800);
    return false;
  }
  if (!f4_pin_digits_valid(b)) {
    GUI.buzzer_cmd_long = true;
    ui_toast("تکرار رمز باید ۳ تا ۸ رقم باشد", "PIN confirmation must be 3 to 8 digits", 1800);
    return false;
  }
  if (strcmp(a, b) != 0) {
    GUI.buzzer_cmd_long = true;
    ui_toast("رمزها یکسان نیستند", "PIN mismatch", 1800);
    return false;
  }
  admin_pin = String(a);
  save_admin_pin();
  GUI.buzzer_cmd_short = true;
  ui_toast("رمز مدیر ذخیره شد", "Admin PIN saved", 1800);
  // Clear the valid PIN focus before emptying the fields.  Otherwise the later
  // header-save focus cleanup validates the just-cleared confirmation again.
  f4_clear_focus_style();
  if (f4_pin1) lv_textarea_set_text(f4_pin1, "");
  f4_pin_confirm_set_enabled(false);
  return true;
}

static void f4_day_ta_ready_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) f4_day_ta_commit();
}
static void f4_timer_ta_ready_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) f4_timer_ta_commit();
}

static void f4_ensure_day_opts() {
  if (f4_day_opts_set) return;
  size_t n = 0;
  for (int i = 0; i <= F4_DAY_ROLLER_MAX; i++) {
    int w = snprintf(f4_day_opts + n, sizeof(f4_day_opts) - n, (i < F4_DAY_ROLLER_MAX) ? "%d\n" : "%d", i);
    if (w <= 0) break;
    n += (size_t)w;
    if (n >= sizeof(f4_day_opts)) break;
  }
  f4_day_opts_set = true;
}

static void f4_day_sync_ui() {
  int v = day_lock_remain;
  if (day_lock_t1_from_chain) v = 0;  // زنجیره از T1 مودال مسلح شده؛ کارت روزشمار خالی می‌ماند
  if (v < 0) v = 0;
  if (v > F4_DAY_ROLLER_MAX) v = F4_DAY_ROLLER_MAX;
  if (f4_day_ta && f4_active_ta != f4_day_ta) {
    char b[8];
    snprintf(b, sizeof(b), "%d", v);
    lv_textarea_set_text(f4_day_ta, b);
  }
  if (f4_day_lbl) {
    char b2[40];
    const bool en = (GUI.system_language == 2);
    if (v <= 0) snprintf(b2, sizeof(b2), "%s", en ? "Off" : "خاموش");
    else snprintf(b2, sizeof(b2), "%d %s", v, en ? "days" : "روز");
    lv_label_set_text(f4_day_lbl, b2);
    lv_obj_set_style_text_font(f4_day_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_align(f4_day_lbl, LV_TEXT_ALIGN_CENTER, 0);
  }
}

static void f4_timer_sync_ui() {
  if (!f4_timer_ta || f4_active_ta == f4_timer_ta) return;
  int sec = (int)constrain(PEND.cut_prcs_timer / 1000, 0, F4_CUT_TIMER_MAX_S);
  char b[8];
  snprintf(b, sizeof(b), "%d", sec);
  lv_textarea_set_text(f4_timer_ta, b);
}

static int f4_ta_int(lv_obj_t *ta, int defv) {
  if (!ta) return defv;
  const char *t = lv_textarea_get_text(ta);
  if (!t || !*t) return defv;
  return atoi(t);
}

static void f4_fill_live_time_tas() {
  char b[8];
  snprintf(b, sizeof(b), "%02d", f4_hh);
  if (f4_ta_hh && !f4_clock_time_edited && !f4_ta_matches_text(f4_ta_hh, b)) {
    f4_clock_syncing = true;
    lv_textarea_set_text(f4_ta_hh, b);
    f4_clock_syncing = false;
  }
  snprintf(b, sizeof(b), "%02d", f4_mi);
  if (f4_ta_mi && !f4_clock_time_edited && !f4_ta_matches_text(f4_ta_mi, b)) {
    f4_clock_syncing = true;
    lv_textarea_set_text(f4_ta_mi, b);
    f4_clock_syncing = false;
  }
}

static void f4_fill_dt_tas() {
  char b[8];
  f4_fill_live_time_tas();
  int dy = f4_d, mo = f4_mo, yy = f4_y;
  if (GUI.system_language == 1) {
    int jy, jm, jd;
    gregorian_to_jalali(f4_y, f4_mo, f4_d, &jy, &jm, &jd);
    dy = jd; mo = jm; yy = jy;
  }
  snprintf(b, sizeof(b), "%02d", dy);
  if (f4_ta_dd && f4_active_ta != f4_ta_dd) lv_textarea_set_text(f4_ta_dd, b);
  snprintf(b, sizeof(b), "%02d", mo);
  if (f4_ta_mo && f4_active_ta != f4_ta_mo) lv_textarea_set_text(f4_ta_mo, b);
  snprintf(b, sizeof(b), "%04d", yy);
  if (f4_ta_yy && f4_active_ta != f4_ta_yy) lv_textarea_set_text(f4_ta_yy, b);
}

static void f4_load_dt_from_soft() {
  int64_t ep = soft_now_epoch();
  if (ep <= 0) {
    f4_y = 2026; f4_mo = 7; f4_d = 14; f4_hh = 12; f4_mi = 0;
  } else {
    int ss;
    epoch_to_civil(ep, &f4_y, &f4_mo, &f4_d, &f4_hh, &f4_mi, &ss);
  }
  f4_fill_dt_tas();
}

// [UI/Task2] Live source is the software clock; date editing remains untouched.
void f4_clock_live_tick() {
  if (GUI.scr_id != 2 || !objects.tabview2_1 ||
      (int)lv_tabview_get_tab_act(objects.tabview2_1) != 3) return;
  // [UI/Task2] Task4 only queues USB keys; LVGL text changes happen here.
  f4_usb_drain();
  static bool f4_usb_seen_valid = false;
  static bool f4_usb_seen = false;
  const bool usb_now = usb_keyboard_present();
  if (!f4_usb_seen_valid || usb_now != f4_usb_seen) {
    f4_usb_seen_valid = true;
    f4_usb_seen = usb_now;
    if (usb_now) f4_kb_hide();
    else if (GUI.virtual_kb_enable && f4_unlocked && f4_active_ta && f4_is_num_ta(f4_active_ta))
      f4_kb_show_for(f4_active_ta);
  }
  if (!f4_unlocked || f4_clock_time_edited) return;
  const int64_t epoch = soft_now_epoch();
  if (epoch <= 0) return;
  const int64_t minute = epoch / 60;
  if (minute == f4_clock_live_minute) return;
  f4_clock_live_minute = minute;
  int yy, mo, dd, hh, mi, ss;
  epoch_to_civil(epoch, &yy, &mo, &dd, &hh, &mi, &ss);
  f4_hh = hh;
  f4_mi = mi;
  f4_fill_live_time_tas();
}

static bool f4_ta_matches_text(lv_obj_t *ta, const char *expected) {
  if (!ta) return true;
  const char *actual = lv_textarea_get_text(ta);
  return actual && expected && strcmp(actual, expected) == 0;
}

bool f4_has_pending_header_save_edit() {
  if (!f4_unlocked) return false;
  if (f4_chain_pending_has_changes()) return true;
  const char *pin1 = f4_pin1 ? lv_textarea_get_text(f4_pin1) : "";
  const char *pin2 = f4_pin2 ? lv_textarea_get_text(f4_pin2) : "";
  if ((pin1 && pin1[0]) || (pin2 && pin2[0])) return true;

  char b[8];
  int day = day_lock_remain;
  if (day < 0) day = 0;
  snprintf(b, sizeof(b), "%d", day);
  if (!f4_ta_matches_text(f4_day_ta, b)) return true;

  const int timer_s = (int)constrain(PEND.cut_prcs_timer / 1000, 0, F4_CUT_TIMER_MAX_S);
  snprintf(b, sizeof(b), "%d", timer_s);
  if (!f4_ta_matches_text(f4_timer_ta, b)) return true;

  f4_err_fmt(b, sizeof(b), blade_err_mm);
  if (!f4_ta_matches_text(f4_err_ta, b)) return true;

  snprintf(b, sizeof(b), "%02d", f4_hh);
  if (!f4_ta_matches_text(f4_ta_hh, b)) return true;
  snprintf(b, sizeof(b), "%02d", f4_mi);
  if (!f4_ta_matches_text(f4_ta_mi, b)) return true;
  int day_of_month = f4_d, month = f4_mo, year = f4_y;
  if (GUI.system_language == 1) gregorian_to_jalali(f4_y, f4_mo, f4_d, &year, &month, &day_of_month);
  snprintf(b, sizeof(b), "%02d", day_of_month);
  if (!f4_ta_matches_text(f4_ta_dd, b)) return true;
  snprintf(b, sizeof(b), "%02d", month);
  if (!f4_ta_matches_text(f4_ta_mo, b)) return true;
  snprintf(b, sizeof(b), "%04d", year);
  return !f4_ta_matches_text(f4_ta_yy, b);
}

void f4_clock_commit_from_ui() {
  if (!f4_unlocked || !f4_ta_hh || !f4_ta_mi || !f4_ta_dd || !f4_ta_mo || !f4_ta_yy) return;
  f4_kb_hide();
  int hh = f4_ta_int(f4_ta_hh, f4_hh);
  int mi = f4_ta_int(f4_ta_mi, f4_mi);
  int dd = f4_ta_int(f4_ta_dd, f4_d);
  int mo = f4_ta_int(f4_ta_mo, f4_mo);
  int yy = f4_ta_int(f4_ta_yy, f4_y);
  if (hh < 0) hh = 0; if (hh > 23) hh = 23;
  if (mi < 0) mi = 0; if (mi > 59) mi = 59;
  if (GUI.system_language == 1) {
    if (yy < 1300) yy = 1405;
    if (mo < 1) mo = 1; if (mo > 12) mo = 12;
    if (dd < 1) dd = 1; if (dd > 31) dd = 31;
    jalali_to_gregorian(yy, mo, dd, &f4_y, &f4_mo, &f4_d);
  } else {
    if (yy < 2000) yy = 2026;
    if (mo < 1) mo = 1; if (mo > 12) mo = 12;
    if (dd < 1) dd = 1; if (dd > 31) dd = 31;
    f4_y = yy; f4_mo = mo; f4_d = dd;
  }
  f4_hh = hh; f4_mi = mi;
  soft_clock_set_civil(f4_y, f4_mo, f4_d, f4_hh, f4_mi, 0);
  (void)ds3231_write_time(f4_y, f4_mo, f4_d, f4_hh, f4_mi, 0);
  if (day_lock_remain > 0) day_lock_arm(day_lock_remain);
  f4_wdt_feed();  // نوشتن I2C ساعت تمام شد — NVS فاز بعدی است
  save_admin_svc_state();
  f4_clock_time_edited = false;
  f4_clock_live_minute = -1;
  f4_fill_dt_tas();
}

bool f4_panel_adjust_numeric(lv_obj_t *obj, int dir) {
  if (!obj || !f4_unlocked || GUI.lock_screen || (dir != -1 && dir != 1)) return false;
  if (obj == f4_day_ta) {
    char b[8];
    snprintf(b, sizeof(b), "%d", constrain(f4_ta_int(obj, day_lock_remain) + dir, 0, F4_DAY_ROLLER_MAX));
    lv_textarea_set_text(obj, b);
    f4_day_ta_commit();
    return true;
  }
  if (obj == f4_timer_ta) {
    char b[8];
    snprintf(b, sizeof(b), "%d", constrain(f4_ta_int(obj, 0) + dir, 0, F4_CUT_TIMER_MAX_S));
    lv_textarea_set_text(obj, b);
    f4_timer_ta_commit();
    return true;
  }
  if (obj == f4_err_ta) {
    const char *raw = lv_textarea_get_text(obj);
    float value = raw && raw[0] ? (float)atof(raw) : blade_err_mm;
    value = constrain(value + (float)dir * 0.1f, -9.9f, 9.9f);
    if (fabsf(value) < 0.1f) value = (dir < 0) ? -0.1f : 0.1f;
    char b[8];
    f4_err_fmt(b, sizeof(b), value);
    lv_textarea_set_text(obj, b);
    f4_err_ta_commit();
    return true;
  }

  int min_value = 0, max_value = 0, width = 0, fallback = 0;
  if (obj == f4_ta_hh) { min_value = 0; max_value = 23; width = 2; fallback = f4_hh; }
  else if (obj == f4_ta_mi) { min_value = 0; max_value = 59; width = 2; fallback = f4_mi; }
  else if (obj == f4_ta_dd) { min_value = 1; max_value = 31; width = 2; fallback = f4_d; }
  else if (obj == f4_ta_mo) { min_value = 1; max_value = 12; width = 2; fallback = f4_mo; }
  else if (obj == f4_ta_yy) {
    min_value = (GUI.system_language == 1) ? 1300 : 2000;
    max_value = 9999;
    width = 4;
    fallback = (GUI.system_language == 1) ? 1405 : 2026;
  } else {
    return false;  // PIN و هر فیلد خارج از قرارداد پنل، تغییر نمی‌کند.
  }
  char b[8];
  snprintf(b, sizeof(b), "%0*d", width, constrain(f4_ta_int(obj, fallback) + dir, min_value, max_value));
  lv_textarea_set_text(obj, b);
  settings_mark_dirty();
  GUI.buzzer_cmd_short = true;
  return true;
}

void f4_flush_open_edits_for_save() {
  if (!f4_unlocked) return;
  (void)f4_pin_commit_from_header_save();
  if (f4_active_ta == f4_day_ta) f4_day_ta_commit();
  else if (f4_active_ta == f4_timer_ta) f4_timer_ta_commit();
  else if (f4_active_ta == f4_err_ta) f4_err_ta_commit();
  else if (f4_err_ta && blade_err_enable) f4_err_ta_commit();
  f4_clear_focus_style();
  f4_kb_hide();
}

static void f4_dt_apply_cb(lv_event_t *e) {
  (void)e;
  // دکمه حذف شد؛ ساعت با Save هدر ذخیره می‌شود
}

static void f4_pitch_cal_btn_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_RELEASED) return;
  if (!f4_tap_accept(e)) return;
  pitch_cal_start_from_ui();
}

static void f4_null_extra_ptrs() {
  f4_extra = NULL;
  f4_pin1 = f4_pin2 = f4_pin_ok = NULL;
  f4_ta_hh = f4_ta_mi = f4_ta_dd = f4_ta_mo = f4_ta_yy = NULL;
  f4_fac_btn = f4_cal_btn = f4_apply_btn = NULL;
  f4_pin_bar = f4_stop_btn = f4_stop_title = f4_stop_state = f4_dt_bar = NULL;
  f4_fac_lbl = f4_cal_lbl = f4_pin_lbl = f4_pin_cfm_cap = f4_pin_admin_mid = f4_pin_eye_lbl = f4_clk_lbl = NULL;
  f4_pin_revealed = false;
  f4_hh_cap = f4_mi_cap = f4_dt_colon = f4_dt_div = f4_date_lbl = f4_apply_lbl = NULL;
  f4_dt_sep_day_month = f4_dt_sep_month_year = NULL;
  f4_dd_cap = f4_mo_cap = f4_yy_cap = NULL;
  f4_js_btn = f4_js_lbl = NULL;
  f4_err_btn = f4_err_btn_lbl = f4_err_num_box = f4_err_ta = f4_err_unit = NULL;
}

static void f4_js_sync_btn() {
  if (!f4_js_btn || !f4_js_lbl) return;
  const bool en = (GUI.system_language == 2);
  const bool on = GUI.joystick_enable;
  lv_obj_set_style_bg_color(f4_js_btn, lv_color_hex(on ? 0x0a84ff : 0x5a7ea3), 0);
  lv_obj_set_style_bg_opa(f4_js_btn, LV_OPA_COVER, 0);
  lv_label_set_text(f4_js_lbl, en ? (on ? "Joystick: ON" : "Joystick: OFF") : (on ? "جویستیک (روشن)" : "جویستیک (خاموش)"));
  lv_obj_set_style_text_font(f4_js_lbl, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_base_dir(f4_js_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_center(f4_js_lbl);
}

static void f4_js_btn_cb(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) { f4_tap_accept(e); return; }
  if (c != LV_EVENT_RELEASED) return;
  if (!f4_tap_accept(e)) return;
  if (!f4_unlocked || GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.joystick_enable = !GUI.joystick_enable;
  PEND.joystick_enable = GUI.joystick_enable;
  if (!GUI.joystick_enable) {
    IOS.joystick_state_L = false;
    IOS.joystick_state_R = false;
  }
  settings_mark_dirty();
  GUI.buzzer_cmd_short = true;
  f4_js_sync_btn();
}

static void f4_match_fa_en_title(lv_obj_t *fa, lv_obj_t *en_lbl) {
  if (!en_lbl) return;
  const bool en = (GUI.system_language == 2);
  lv_obj_set_width(en_lbl, 124);
  lv_label_set_long_mode(en_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(en_lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(en_lbl, LV_ALIGN_TOP_MID, 0, en ? 4 : 0);
  lv_obj_update_layout(en_lbl);
  if (!fa) return;
  lv_obj_set_width(fa, lv_obj_get_width(en_lbl));
  lv_label_set_long_mode(fa, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(fa, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(fa, LV_ALIGN_TOP_MID, 0, 0);
}

// فعال پررنگ / غیرفعال کم‌رنگ برای کلید چپ‌راست جهت موتور
void f4_dir_buttons_sync() {
  if (!objects.btn_dir_chap || !objects.btn_dir_rast) return;
  int dir = PEND.system_base_dir;
  if (dir == 0) dir = STP.system_base_dir;
  const bool right_on = (dir == 1);
  lv_obj_set_style_bg_color(objects.btn_dir_rast, lv_color_hex(right_on ? 0x0a84ff : 0x5a7ea3), 0);
  lv_obj_set_style_bg_color(objects.btn_dir_chap, lv_color_hex(right_on ? 0x5a7ea3 : 0x0a84ff), 0);
  lv_obj_set_style_bg_opa(objects.btn_dir_rast, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(objects.btn_dir_chap, LV_OPA_COVER, 0);
  if (objects.lablef_dir_rast)
    lv_obj_set_style_text_opa(objects.lablef_dir_rast, right_on ? LV_OPA_COVER : LV_OPA_70, 0);
  if (objects.lablee_dir_rast)
    lv_obj_set_style_text_opa(objects.lablee_dir_rast, right_on ? LV_OPA_COVER : LV_OPA_70, 0);
  if (objects.lablef_dir_chap)
    lv_obj_set_style_text_opa(objects.lablef_dir_chap, right_on ? LV_OPA_70 : LV_OPA_COVER, 0);
  if (objects.lablee_dir_chap)
    lv_obj_set_style_text_opa(objects.lablee_dir_chap, right_on ? LV_OPA_70 : LV_OPA_COVER, 0);
}

static void f4_rebuild_chrome_if_needed() {
  if (f4_layout_ver == F4_LAYOUT_VER && objects.obj16 && lv_obj_is_valid(objects.obj16) &&
      f4_extra && lv_obj_is_valid(f4_extra) && lv_obj_get_parent(f4_extra) == objects.obj16 &&
      f4_day_card && lv_obj_is_valid(f4_day_card) && lv_obj_get_parent(f4_day_card) == objects.obj16 &&
      f4_day_ta && lv_obj_is_valid(f4_day_ta) && f4_timer_ta && lv_obj_is_valid(f4_timer_ta) &&
      f4_err_num_box && lv_obj_is_valid(f4_err_num_box) && f4_js_btn && lv_obj_is_valid(f4_js_btn) &&
      !f4_apply_btn) return;

  f4_kb_hide();
  f4_clear_focus_style();
  f4_active_ta = NULL;

  // پاک‌سازی نسخۀ قبلی
  if (f4_day_card && lv_obj_is_valid(f4_day_card)) {
    lv_obj_delete(f4_day_card);
  } else {
    if (f4_day_roller && lv_obj_is_valid(f4_day_roller)) lv_obj_delete(f4_day_roller);
    if (f4_day_lbl && lv_obj_is_valid(f4_day_lbl)) lv_obj_delete(f4_day_lbl);
    if (f4_day_title && lv_obj_is_valid(f4_day_title)) lv_obj_delete(f4_day_title);
    if (f4_day_ta && lv_obj_is_valid(f4_day_ta)) lv_obj_delete(f4_day_ta);
  }
  f4_day_card = f4_day_roller = f4_day_lbl = f4_day_title = f4_day_ta = NULL;

  if (f4_timer_s_lbl && lv_obj_is_valid(f4_timer_s_lbl)) lv_obj_delete(f4_timer_s_lbl);
  f4_timer_s_lbl = NULL;
  if (f4_timer_ta && lv_obj_is_valid(f4_timer_ta)) lv_obj_delete(f4_timer_ta);
  f4_timer_ta = NULL;

  if (f4_extra && lv_obj_is_valid(f4_extra)) lv_obj_delete(f4_extra);
  f4_null_extra_ptrs();
  if (f4_hdr_mask && lv_obj_is_valid(f4_hdr_mask)) { lv_obj_delete(f4_hdr_mask); f4_hdr_mask = NULL; }
  f4_wdt_feed();  // فاز حذف تمام شد — پنجرۀ WDT از نو پیش از ساخت انبوه آبجکت‌ها

  // نوار پوشش ثابت بالای صفحه (ماسک هدر) — جلوگیری از تداخل آیتم‌های اسکرول با ۵ کلید بالای صفحه
  f4_hdr_mask = lv_obj_create(objects.obj16);
  lv_obj_set_pos(f4_hdr_mask, -16, -16);
  lv_obj_set_size(f4_hdr_mask, 440, 31);
  lv_obj_set_style_bg_color(f4_hdr_mask, lv_color_hex(0xf2f2f7), 0);
  lv_obj_set_style_bg_opa(f4_hdr_mask, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f4_hdr_mask, 0, 0);
  lv_obj_set_style_radius(f4_hdr_mask, 0, 0);
  lv_obj_set_style_pad_all(f4_hdr_mask, 0, 0);
  lv_obj_add_flag(f4_hdr_mask, LV_OBJ_FLAG_FLOATING);
  lv_obj_clear_flag(f4_hdr_mask, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

  // ---- کارت فشرده روزشمار قفل: عنوان + کادر عدد (−۸ ارتفاع از پایین، عنوان ۳↑) ----
  f4_day_card = lv_obj_create(objects.obj16);
  f4_style_admin_card(f4_day_card, -8, 39, 128, 88);
  lv_obj_add_flag(f4_day_card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_day_card, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(f4_day_card, f4_day_chain_hold_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_day_card, f4_day_chain_hold_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_day_card, f4_day_chain_hold_cb, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(f4_day_card, f4_day_chain_hold_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(f4_day_card, f4_day_chain_hold_cb, LV_EVENT_PRESS_LOST, NULL);

  f4_day_title = lv_label_create(f4_day_card);
  const bool f4_is_en = (GUI.system_language == 2);
  lv_obj_set_style_text_font(f4_day_title, f4_is_en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_base_dir(f4_day_title, f4_is_en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(f4_day_title, f4_is_en ? "Day Lock" : "روزشمار قفل");
  lv_obj_add_flag(f4_day_title, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_day_title, f4_day_chain_hold_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_day_title, f4_day_chain_hold_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_day_title, f4_day_chain_hold_cb, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(f4_day_title, f4_day_chain_hold_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(f4_day_title, f4_day_chain_hold_cb, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_set_width(f4_day_title, 118);
  lv_label_set_long_mode(f4_day_title, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(f4_day_title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_day_title, lv_color_hex(0x1c1c1e), 0);
  lv_obj_align(f4_day_title, LV_ALIGN_TOP_MID, -1, 1);  // ۳px بالا از ۴

  f4_day_ta = f4_make_num_ta(f4_day_card, 13, 36, 100, 3, "0");  // عدد ۸px بالا
  lv_obj_add_event_cb(f4_day_ta, f4_day_ta_ready_cb, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(f4_day_ta, f4_day_chain_hold_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_day_ta, f4_day_chain_hold_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_day_ta, f4_day_chain_hold_cb, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(f4_day_ta, f4_day_chain_hold_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(f4_day_ta, f4_day_chain_hold_cb, LV_EVENT_PRESS_LOST, NULL);

  f4_day_lbl = lv_label_create(f4_day_card);
  lv_obj_add_flag(f4_day_lbl, LV_OBJ_FLAG_HIDDEN);

  // ---- کادر عدد زمان تیغه روی کارت EEZ obj17 ----
  if (objects.obj17) {
    lv_obj_add_flag(objects.obj17, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.obj17, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);
    f4_timer_ta = f4_make_num_ta(objects.obj17, 14, 36, 100, 2, "0");
    lv_obj_set_style_pad_right(f4_timer_ta, 16, 0);
    lv_obj_add_event_cb(f4_timer_ta, f4_timer_ta_ready_cb, LV_EVENT_READY, NULL);
    f4_timer_s_lbl = lv_label_create(objects.obj17);
    lv_label_set_text(f4_timer_s_lbl, "S");
    lv_obj_set_style_text_font(f4_timer_s_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(f4_timer_s_lbl, lv_color_hex(0x8e8e93), 0);
    lv_obj_set_pos(f4_timer_s_lbl, 96, 46);
    lv_obj_clear_flag(f4_timer_s_lbl, LV_OBJ_FLAG_CLICKABLE);
  }

  // ---- بخش PIN / ساعت / کارخانه ----
  // در ردیف دوم دقیقاً ۸ پیکسل زیر ۳ کارت بالا (روزشمار/جهت/تیغه در ارتفاع ۷۸)
  f4_extra = lv_obj_create(objects.obj16);
  lv_obj_set_pos(f4_extra, -1, 101);
  lv_obj_set_size(f4_extra, 400, 286);
  lv_obj_set_style_bg_opa(f4_extra, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(f4_extra, 0, 0);
  lv_obj_set_style_pad_all(f4_extra, 0, 0);
  lv_obj_set_style_clip_corner(f4_extra, false, 0);
  lv_obj_add_flag(f4_extra, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_clear_flag(f4_extra, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_extra, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_extra, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);

  auto f4_style_thin_bar = [](lv_obj_t *b) {
    if (!b) return;
    lv_obj_set_style_bg_color(b, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0xc6c6c8), 0);
    lv_obj_set_style_pad_all(b, 4, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  };

  // ---- کارت رمز مدیر (هماهنگ با کارت تاخیر تیغه و روزشمار در ارتفاع ۷۸ پیکسل و گوشه ۱۲) ----
  f4_pin_bar = lv_obj_create(f4_extra);
  f4_style_admin_card(f4_pin_bar, 0, 0, 256, 78);
  lv_obj_set_style_clip_corner(f4_pin_bar, false, 0);
  lv_obj_add_flag(f4_pin_bar, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_pin_bar, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);

  f4_stop_btn = lv_button_create(f4_extra);
  lv_obj_set_pos(f4_stop_btn, 266, 0);
  lv_obj_set_size(f4_stop_btn, 118, 78);
  lv_obj_set_style_bg_color(f4_stop_btn, lv_color_hex(0xd32f2f), 0);
  lv_obj_set_style_bg_color(f4_stop_btn, lv_color_hex(0xa91f1f), LV_STATE_PRESSED);
  lv_obj_set_style_radius(f4_stop_btn, 12, 0);
  lv_obj_set_style_border_width(f4_stop_btn, 0, 0);
  lv_obj_set_style_pad_all(f4_stop_btn, 0, 0);
  lv_obj_add_event_cb(f4_stop_btn, f4_stop_btn_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_stop_btn, f4_stop_btn_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_stop_btn, f4_stop_btn_cb, LV_EVENT_RELEASED, NULL);
  f4_stop_title = lv_label_create(f4_stop_btn);
  lv_label_set_text(f4_stop_title, "استپ");
  lv_obj_set_style_text_font(f4_stop_title, &ui_font_naz20, 0);
  lv_obj_set_style_text_color(f4_stop_title, lv_color_hex(0xffffff), 0);
  lv_obj_set_width(f4_stop_title, 118);
  lv_obj_set_style_text_align(f4_stop_title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(f4_stop_title, 0, 8);
  lv_obj_clear_flag(f4_stop_title, LV_OBJ_FLAG_CLICKABLE);
  f4_stop_state = lv_label_create(f4_stop_btn);
  lv_obj_set_style_text_color(f4_stop_state, lv_color_hex(0xffffff), 0);
  lv_obj_set_width(f4_stop_state, 118);
  lv_obj_set_style_text_align(f4_stop_state, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(f4_stop_state, 0, 42);
  lv_obj_clear_flag(f4_stop_state, LV_OBJ_FLAG_CLICKABLE);

  f4_pin_lbl = lv_label_create(f4_pin_bar);
  lv_obj_set_pos(f4_pin_lbl, 134, -2);
  lv_obj_set_width(f4_pin_lbl, 114);
  lv_obj_set_style_text_align(f4_pin_lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_pin_lbl, lv_color_hex(0x1c1c1e), 0);

  f4_pin_cfm_cap = lv_label_create(f4_pin_bar);
  lv_obj_set_pos(f4_pin_cfm_cap, 8, -2);
  lv_obj_set_width(f4_pin_cfm_cap, 114);
  lv_obj_set_style_text_align(f4_pin_cfm_cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_pin_cfm_cap, lv_color_hex(0x1c1c1e), 0);

  f4_pin1 = f4_make_num_ta(f4_pin_bar, 134, 30, 114, 8, "PIN");
  lv_textarea_set_password_mode(f4_pin1, true);
  f4_pin2 = f4_make_num_ta(f4_pin_bar, 8, 30, 114, 8, "Confirm");
  lv_textarea_set_password_mode(f4_pin2, true);
  lv_obj_set_style_bg_color(f4_pin2, lv_color_hex(0xe5e5ea), LV_STATE_DISABLED);
  lv_obj_set_style_border_color(f4_pin2, lv_color_hex(0xc6c6c8), LV_STATE_DISABLED);
  lv_obj_set_style_text_color(f4_pin2, lv_color_hex(0x8e8e93), LV_STATE_DISABLED);
  lv_obj_add_event_cb(f4_pin1, f4_pin_new_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
  f4_pin_confirm_set_enabled(false);
  f4_pin_admin_mid = lv_button_create(f4_pin_bar);
  lv_obj_set_pos(f4_pin_admin_mid, 118, 0);
  lv_obj_set_size(f4_pin_admin_mid, 20, 26);
  lv_obj_set_style_bg_opa(f4_pin_admin_mid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(f4_pin_admin_mid, 0, 0);
  lv_obj_set_style_outline_width(f4_pin_admin_mid, 0, 0);
  lv_obj_set_style_shadow_width(f4_pin_admin_mid, 0, 0);
  lv_obj_set_style_pad_all(f4_pin_admin_mid, 0, 0);
  lv_obj_set_style_bg_opa(f4_pin_admin_mid, LV_OPA_TRANSP, LV_STATE_PRESSED);
  lv_obj_set_style_border_width(f4_pin_admin_mid, 0, LV_STATE_PRESSED);
  lv_obj_set_style_outline_width(f4_pin_admin_mid, 0, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(f4_pin_admin_mid, 0, LV_STATE_PRESSED);
  lv_obj_add_event_cb(f4_pin_admin_mid, f4_pin_eye_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_pin_admin_mid, f4_pin_eye_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_pin_admin_mid, f4_pin_eye_cb, LV_EVENT_RELEASED, NULL);
  f4_pin_eye_lbl = lv_label_create(f4_pin_admin_mid);
  lv_obj_set_style_text_font(f4_pin_eye_lbl, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(f4_pin_eye_lbl, lv_color_hex(0x5a7ea3), 0);
  lv_label_set_text(f4_pin_eye_lbl, LV_SYMBOL_EYE_CLOSE);
  lv_obj_align(f4_pin_eye_lbl, LV_ALIGN_CENTER, 0, 0);

  f4_clk_lbl = lv_label_create(f4_extra);
  lv_obj_add_flag(f4_clk_lbl, LV_OBJ_FLAG_HIDDEN);
  f4_wdt_feed();  // نوار PIN ساخته شد — نوار ساعت/تاریخ فاز بعدی است

  // نوار ساعت/تاریخ: ۵ نوشته+۵ کادر −۴ چپ؛ نوشته −۴ بالا؛ نوار از بالا −۲
  f4_dt_bar = lv_obj_create(f4_extra);
  lv_obj_set_pos(f4_dt_bar, 0, 76);
  lv_obj_set_size(f4_dt_bar, 392, 82);
  f4_style_thin_bar(f4_dt_bar);
  lv_obj_add_flag(f4_dt_bar, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_dt_bar, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);

  f4_hh_cap = lv_label_create(f4_dt_bar);
  lv_obj_set_pos(f4_hh_cap, -5, 0);
  lv_obj_set_width(f4_hh_cap, 64);
  lv_obj_set_style_text_align(f4_hh_cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_hh_cap, lv_color_hex(0x1c1c1e), 0);

  f4_mi_cap = lv_label_create(f4_dt_bar);
  lv_obj_set_pos(f4_mi_cap, 83, 0);
  lv_obj_set_width(f4_mi_cap, 64);
  lv_obj_set_style_text_align(f4_mi_cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_mi_cap, lv_color_hex(0x1c1c1e), 0);

  f4_dd_cap = lv_label_create(f4_dt_bar);
  lv_obj_set_pos(f4_dd_cap, 171, 0);
  lv_obj_set_width(f4_dd_cap, 56);
  lv_obj_set_style_text_align(f4_dd_cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_dd_cap, lv_color_hex(0x1c1c1e), 0);

  f4_mo_cap = lv_label_create(f4_dt_bar);
  lv_obj_set_pos(f4_mo_cap, 235, 0);
  lv_obj_set_width(f4_mo_cap, 56);
  lv_obj_set_style_text_align(f4_mo_cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_mo_cap, lv_color_hex(0x1c1c1e), 0);

  f4_yy_cap = lv_label_create(f4_dt_bar);
  lv_obj_set_pos(f4_yy_cap, 299, 0);
  lv_obj_set_width(f4_yy_cap, 76);
  lv_obj_set_style_text_align(f4_yy_cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(f4_yy_cap, lv_color_hex(0x1c1c1e), 0);

  f4_date_lbl = NULL;

  f4_ta_hh = f4_make_num_ta(f4_dt_bar, -5, 27, 64, 2, "HH");
  f4_dt_colon = lv_label_create(f4_dt_bar);
  lv_obj_set_style_text_font(f4_dt_colon, &lv_font_montserrat_20, 0);
  lv_label_set_text(f4_dt_colon, ":");
  lv_obj_set_pos(f4_dt_colon, 65, 36);
  f4_ta_mi = f4_make_num_ta(f4_dt_bar, 83, 27, 64, 2, "MM");
  lv_obj_add_event_cb(f4_ta_hh, f4_clock_time_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(f4_ta_mi, f4_clock_time_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // خط جداکننده بین ساعت و تاریخ: ۲ پیکسل به چپ
  f4_dt_div = lv_obj_create(f4_dt_bar);
  lv_obj_set_size(f4_dt_div, 1, 48);
  lv_obj_set_pos(f4_dt_div, 143, 20);
  lv_obj_set_style_bg_color(f4_dt_div, lv_color_hex(0xd1d1d6), 0);
  lv_obj_set_style_bg_opa(f4_dt_div, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f4_dt_div, 0, 0);
  lv_obj_set_style_pad_all(f4_dt_div, 0, 0);
  lv_obj_clear_flag(f4_dt_div, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

  f4_ta_dd = f4_make_num_ta(f4_dt_bar, 171, 27, 56, 2, "DD");
  f4_ta_mo = f4_make_num_ta(f4_dt_bar, 235, 27, 56, 2, "MM");
  f4_ta_yy = f4_make_num_ta(f4_dt_bar, 299, 27, 76, 4, "YYYY");

  // جداکننده‌های بصری بین روز/ماه و ماه/سال؛ داخل فاصلۀ خالی، هم‌قد کادرها.
  f4_dt_sep_day_month = lv_label_create(f4_dt_bar);
  f4_dt_sep_month_year = lv_label_create(f4_dt_bar);
  lv_obj_t *f4_dt_seps[2] = { f4_dt_sep_day_month, f4_dt_sep_month_year };
  for (int si = 0; si < 2; si++) {
    lv_obj_t *sep = f4_dt_seps[si];
    lv_label_set_text(sep, "/");
    lv_obj_set_style_text_font(sep, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sep, lv_color_hex(0x8e8e93), 0);
  }
  lv_obj_set_pos(f4_dt_sep_day_month, 216, 36);
  lv_obj_set_pos(f4_dt_sep_month_year, 286, 36);

  // اصلاح خطا (کادر آبی + عدد سفید داخلش) کنار جویستیک کوچک‌تر
  const int ERR_Y = 172;
  f4_err_btn = lv_obj_create(f4_extra);
  lv_obj_set_pos(f4_err_btn, 0, ERR_Y);
  lv_obj_set_size(f4_err_btn, 244, 46);
  lv_obj_set_style_radius(f4_err_btn, 12, 0);
  lv_obj_set_style_border_width(f4_err_btn, 0, 0);
  lv_obj_set_style_pad_all(f4_err_btn, 0, 0);
  lv_obj_set_style_bg_color(f4_err_btn, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_bg_opa(f4_err_btn, LV_OPA_COVER, 0);
  lv_obj_clear_flag(f4_err_btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_err_btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(f4_err_btn, f4_err_btn_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_err_btn, f4_err_btn_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_err_btn, f4_err_btn_cb, LV_EVENT_RELEASED, NULL);

  f4_err_btn_lbl = lv_label_create(f4_err_btn);
  lv_obj_set_width(f4_err_btn_lbl, 118);
  lv_obj_set_style_text_align(f4_err_btn_lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(f4_err_btn_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(f4_err_btn_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_align(f4_err_btn_lbl, LV_ALIGN_LEFT_MID, 6, 0);
  lv_obj_clear_flag(f4_err_btn_lbl, LV_OBJ_FLAG_CLICKABLE);

  f4_err_num_box = lv_obj_create(f4_err_btn);
  lv_obj_set_pos(f4_err_num_box, 130, 6);
  lv_obj_set_size(f4_err_num_box, 106, 34);
  lv_obj_set_style_bg_color(f4_err_num_box, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_opa(f4_err_num_box, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(f4_err_num_box, 8, 0);
  lv_obj_set_style_border_width(f4_err_num_box, 0, 0);
  lv_obj_set_style_pad_all(f4_err_num_box, 0, 0);
  lv_obj_clear_flag(f4_err_num_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f4_err_num_box, LV_OBJ_FLAG_CLICKABLE);  // کلیک سفید → toggle نشود
  lv_obj_clear_flag(f4_err_num_box, LV_OBJ_FLAG_EVENT_BUBBLE);

  f4_err_ta = f4_make_num_ta(f4_err_num_box, 2, 2, 66, 4, "2");
  lv_obj_set_size(f4_err_ta, 66, 30);
  lv_obj_set_style_bg_opa(f4_err_ta, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_ver(f4_err_ta, 2, 0);
  lv_textarea_set_accepted_chars(f4_err_ta, "0123456789.-");
  lv_obj_add_event_cb(f4_err_ta, f4_err_ta_ready_cb, LV_EVENT_READY, NULL);

  f4_err_unit = lv_label_create(f4_err_num_box);
  lv_label_set_text(f4_err_unit, "mm");
  lv_obj_set_style_text_font(f4_err_unit, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(f4_err_unit, lv_color_hex(0x8e8e93), 0);
  lv_obj_align(f4_err_unit, LV_ALIGN_RIGHT_MID, -5, 0);
  lv_obj_clear_flag(f4_err_unit, LV_OBJ_FLAG_CLICKABLE);

  f4_js_btn = lv_button_create(f4_extra);
  lv_obj_set_pos(f4_js_btn, 256, ERR_Y);
  lv_obj_set_size(f4_js_btn, 136, 43);
  lv_obj_set_style_radius(f4_js_btn, 12, 0);
  lv_obj_add_event_cb(f4_js_btn, f4_js_btn_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_js_btn, f4_js_btn_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_js_btn, f4_js_btn_cb, LV_EVENT_RELEASED, NULL);
  f4_js_lbl = lv_label_create(f4_js_btn);
  lv_obj_set_style_text_color(f4_js_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_center(f4_js_lbl);
  f4_js_sync_btn();

  // کارخانه / کالیبره — زیر ردیف اصلاح‌خطا+جویستیک
  const int FAC_W = 229;
  const int FAC_H = 40;
  const int CAL_X = FAC_W + 8;
  const int CAL_W = 392 - CAL_X;
  const int FAC_Y = 232;
  f4_fac_btn = lv_button_create(f4_extra);
  lv_obj_set_pos(f4_fac_btn, 0, FAC_Y);
  lv_obj_set_size(f4_fac_btn, FAC_W, FAC_H);
  lv_obj_set_style_bg_color(f4_fac_btn, lv_color_hex(0x5a7ea3), 0);
  lv_obj_set_style_radius(f4_fac_btn, 12, 0);
  lv_obj_set_style_border_width(f4_fac_btn, 0, 0);
  lv_obj_add_event_cb(f4_fac_btn, f4_fac_save_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_fac_btn, f4_fac_save_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_fac_btn, f4_fac_save_cb, LV_EVENT_RELEASED, NULL);
  f4_fac_lbl = lv_label_create(f4_fac_btn);
  lv_obj_center(f4_fac_lbl);

  f4_cal_btn = lv_button_create(f4_extra);
  lv_obj_set_pos(f4_cal_btn, CAL_X, FAC_Y);
  lv_obj_set_size(f4_cal_btn, CAL_W, FAC_H);
  lv_obj_set_style_bg_color(f4_cal_btn, lv_color_hex(0xd32f2f), 0);
  lv_obj_set_style_radius(f4_cal_btn, 12, 0);
  lv_obj_set_style_border_width(f4_cal_btn, 0, 0);
  lv_obj_add_event_cb(f4_cal_btn, f4_pitch_cal_btn_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(f4_cal_btn, f4_pitch_cal_btn_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(f4_cal_btn, f4_pitch_cal_btn_cb, LV_EVENT_RELEASED, NULL);
  f4_cal_lbl = lv_label_create(f4_cal_btn);
  lv_obj_center(f4_cal_lbl);

  f4_apply_btn = NULL;
  f4_apply_lbl = NULL;

  f4_err_sync_ui();

  f4_layout_ver = F4_LAYOUT_VER;
  if (objects.obj16) lv_obj_scroll_to_y(objects.obj16, 0, LV_ANIM_OFF);
  f4_diag_log("chrome rebuild");
}

static void f4_scroll_hide_kb_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
    // [UI/Task2] Scroll gesture owns the touch; never release a control as a tap.
    f4_tap_armed = false;
    f4_tap_moved = true;
    f4_kb_hide();
    f4_clear_focus_style();
  } else if (lv_event_get_code(e) == LV_EVENT_SCROLL) {
    f4_tap_moved = true;
  } else if (lv_event_get_code(e) == LV_EVENT_SCROLL_END) {
    if (f4_pin_lbl) lv_obj_invalidate(f4_pin_lbl);
    if (f4_pin_cfm_cap) lv_obj_invalidate(f4_pin_cfm_cap);
  }
}

void f4_admin_reflow() {
  if (GUI.scr_id != 2) return;
  if (!objects.obj16) return;
  f4_wdt_feed();  // reflow ممکن است داخل یک callback لمس اجرا شود — پنجره از نو
  // Capture/work mode bypasses only this F4 UI gate in the current firmware.
  // It never changes motion, cut, E-stop, guard, or stored credentials.
  if (F4_CAPTURE_WORK_MODE && !f4_unlocked) f4_unlocked = true;
  // Keep protected content hidden until the lock layer is already visible.
  if (!f4_unlocked) {
    lv_obj_add_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
    f4_gate_show(true);
    f4_reflow_done = false;
    return;
  }

  // Fast-path guard: if already reflowed for this state, do lightweight live syncs only
  if (f4_reflow_done && f4_reflow_lang_seen == (uint8_t)GUI.system_language &&
      f4_reflow_unlocked_seen == f4_unlocked && f4_reflow_layout_seen == f4_layout_ver) {
    f4_dir_buttons_sync();
    f4_stop_sync_ui();
    f4_js_sync_btn();
    return;
  }

  lv_obj_clear_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
  // برش محتوا زیر ۵ آیتم هدر (خانه/راهنما/عنوان/زبان/ذخیره)
  // محتوای F4 تا لبۀ امن زیر هدر بالا می‌آید؛ ستون و اسکرول‌بار نیز ۷px چپ‌تر می‌نشینند.
  // Keep the five fixed header items outside the scroll viewport.
  lv_obj_set_pos(objects.obj16, -23, 14);
  lv_obj_set_size(objects.obj16, 423, 306);
  lv_obj_add_flag(objects.obj16, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(objects.obj16, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_set_scroll_dir(objects.obj16, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(objects.obj16, LV_SCROLLBAR_MODE_ON);
  lv_obj_clear_flag(objects.obj16, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_clear_flag(objects.obj16, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_set_style_clip_corner(objects.obj16, false, 0);
  lv_obj_set_style_bg_color(objects.obj16, lv_color_hex(0xf2f2f7), 0);
  lv_obj_set_style_bg_opa(objects.obj16, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(objects.obj16, 0, 0);
  lv_obj_set_style_outline_width(objects.obj16, 0, 0);
  lv_obj_set_style_pad_bottom(objects.obj16, 8, 0);
  lv_obj_set_style_pad_right(objects.obj16, 18, 0);
  lv_obj_set_style_width(objects.obj16, 6, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_top(objects.obj16, 40, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_bottom(objects.obj16, 40, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(objects.obj16, lv_color_hex(0x8e8e93), LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(objects.obj16, LV_OPA_COVER, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(objects.obj16, lv_color_hex(0x0a84ff), LV_PART_SCROLLBAR | LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(objects.obj16, lv_color_hex(0x0a84ff), LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
  lv_obj_set_style_bg_opa(objects.obj16, LV_OPA_COVER, LV_PART_SCROLLBAR | LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(objects.obj16, LV_OPA_COVER, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
  lv_obj_set_style_radius(objects.obj16, 3, LV_PART_SCROLLBAR);
  if (f4_event_cb_count(objects.obj16, f4_scroll_hide_kb_cb) != 3) {
    f4_remove_event_cb_all(objects.obj16, f4_scroll_hide_kb_cb);
    lv_obj_add_event_cb(objects.obj16, f4_scroll_hide_kb_cb, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(objects.obj16, f4_scroll_hide_kb_cb, LV_EVENT_SCROLL, NULL);
    lv_obj_add_event_cb(objects.obj16, f4_scroll_hide_kb_cb, LV_EVENT_SCROLL_END, NULL);
  }

  // کارت عنوان (−۹px چپ)
  if (objects.lablef2_4) {
    lv_obj_t *title_box = lv_obj_get_parent(objects.lablef2_4);
    if (title_box) {
      lv_obj_set_pos(title_box, 60, 4);
      lv_obj_set_size(title_box, 252, 36);
      lv_obj_set_style_radius(title_box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(title_box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(title_box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_flag(title_box, LV_OBJ_FLAG_FLOATING);
      lv_obj_move_foreground(title_box);
      // ضربه روی فضای خالی هدر هم پد عددی را ببندد (تکمیل پوشش بستن پد)
      lv_obj_add_flag(title_box, LV_OBJ_FLAG_CLICKABLE);
      if (f4_event_cb_count(title_box, f4_blank_click_cb) == 0)
        lv_obj_add_event_cb(title_box, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_set_style_text_font(objects.lablef2_4, &ui_font_titr26, 0);
    lv_obj_set_style_text_color(objects.lablef2_4, lv_color_hex(0x1d1d1f), 0);
    lv_obj_align(objects.lablef2_4, LV_ALIGN_CENTER, 0, -2);
  }
  if (objects.lablee2_4) {
    lv_obj_set_style_text_font(objects.lablee2_4, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(objects.lablee2_4, lv_color_hex(0x1d1d1f), 0);
    lv_obj_align(objects.lablee2_4, LV_ALIGN_CENTER, 0, 0);
  }

  f4_rebuild_chrome_if_needed();

  // سه تنظیم نخست از بدو ورود کامل دیده می‌شوند؛ y=4 کمترین مقدارِ بدون برش در محتوای F4 است.
  f4_style_admin_card(objects.obj18, 128, 15, 128, 78);
  lv_obj_add_flag(objects.obj18, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(objects.obj18, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);
  f4_style_admin_card(objects.obj17, 266, 15, 118, 78);
  if (f4_day_card) f4_style_admin_card(f4_day_card, 0, 15, 118, 78);

  f4_style_compact_title(objects.lablef2_27, objects.lablee2_27);
  f4_style_compact_title(objects.lablef2_24, objects.lablee2_24);
  f4_match_fa_en_title(objects.lablef2_27, objects.lablee2_27);
  f4_match_fa_en_title(objects.lablef2_24, objects.lablee2_24);
  if (f4_day_title) lv_obj_align(f4_day_title, LV_ALIGN_TOP_MID, -1, 0);

  // سوئیچ جهت و رولر تایمر مخفی — فقط کادر عدد / کلید چپ‌راست
  if (objects.sw2_1) lv_obj_add_flag(objects.sw2_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.roller2_2) lv_obj_add_flag(objects.roller2_2, LV_OBJ_FLAG_HIDDEN);

  // کلید چپ / راست — ارگونومی اپل ۴۲ پیکسل
  if (objects.btn_dir_chap && objects.btn_dir_rast) {
    lv_obj_t *wrap = lv_obj_get_parent(objects.btn_dir_chap);
    if (wrap) {
      lv_obj_set_size(wrap, 118, 42);
      lv_obj_align(wrap, LV_ALIGN_BOTTOM_MID, 0, -4);
      lv_obj_set_style_base_dir(wrap, LV_BASE_DIR_LTR, 0);
      lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(wrap, 0, 0);
    }
    lv_obj_set_pos(objects.btn_dir_chap, 2, 0);
    lv_obj_set_size(objects.btn_dir_chap, 52, 42);
    lv_obj_set_style_radius(objects.btn_dir_chap, 10, 0);
    lv_obj_set_style_bg_color(objects.btn_dir_chap, lv_color_hex(0x5a7ea3), 0);
    lv_obj_set_style_border_width(objects.btn_dir_chap, 0, 0);
    lv_obj_set_pos(objects.btn_dir_rast, 62, 0);
    lv_obj_set_size(objects.btn_dir_rast, 52, 42);
    lv_obj_set_style_radius(objects.btn_dir_rast, 10, 0);
    lv_obj_set_style_bg_color(objects.btn_dir_rast, lv_color_hex(0x0a84ff), 0);
    lv_obj_set_style_border_width(objects.btn_dir_rast, 0, 0);
    if (objects.lablee_dir_chap) {
      lv_obj_set_style_text_font(objects.lablee_dir_chap, &lv_font_montserrat_14, 0);
      lv_obj_set_width(objects.lablee_dir_chap, 52);
      lv_obj_set_style_text_align(objects.lablee_dir_chap, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(objects.lablee_dir_chap, LV_ALIGN_CENTER, 0, 0);
    }
    if (objects.lablef_dir_chap) {
      lv_obj_set_style_text_font(objects.lablef_dir_chap, &ui_font_naz18b, 0);
      lv_obj_set_width(objects.lablef_dir_chap, 52);
      lv_obj_set_style_text_align(objects.lablef_dir_chap, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(objects.lablef_dir_chap, LV_ALIGN_CENTER, 0, 0);
    }
    if (objects.lablee_dir_rast) {
      lv_obj_set_style_text_font(objects.lablee_dir_rast, &lv_font_montserrat_14, 0);
      lv_obj_set_width(objects.lablee_dir_rast, 52);
      lv_obj_set_style_text_align(objects.lablee_dir_rast, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(objects.lablee_dir_rast, LV_ALIGN_CENTER, 0, 0);
    }
    if (objects.lablef_dir_rast) {
      lv_obj_set_style_text_font(objects.lablef_dir_rast, &ui_font_naz18b, 0);
      lv_obj_set_width(objects.lablef_dir_rast, 52);
      lv_obj_set_style_text_align(objects.lablef_dir_rast, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(objects.lablef_dir_rast, LV_ALIGN_CENTER, 0, 0);
    }
    f4_dir_buttons_sync();
  }

  if (f4_timer_ta) {
    lv_obj_set_pos(f4_timer_ta, 14, 32);
    lv_obj_set_size(f4_timer_ta, 74, 36);
    lv_obj_set_style_pad_left(f4_timer_ta, 0, 0);
    lv_obj_set_style_pad_right(f4_timer_ta, 0, 0);
    lv_obj_set_style_pad_top(f4_timer_ta, 4, 0);
    lv_obj_set_style_pad_bottom(f4_timer_ta, 4, 0);
    lv_obj_set_style_text_align(f4_timer_ta, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(f4_timer_ta, LV_OBJ_FLAG_SCROLLABLE);
  }
  if (f4_timer_s_lbl) {
    lv_obj_set_style_text_color(f4_timer_s_lbl, lv_color_hex(0x8e8e93), 0);
    lv_obj_set_pos(f4_timer_s_lbl, 94, 40);
    lv_obj_move_foreground(f4_timer_s_lbl);
  }
  if (f4_day_ta) {
    lv_obj_set_pos(f4_day_ta, 13, 32);
    lv_obj_set_size(f4_day_ta, 90, 36);
    lv_obj_set_style_pad_top(f4_day_ta, 4, 0);
    lv_obj_set_style_pad_bottom(f4_day_ta, 4, 0);
    lv_obj_set_style_text_align(f4_day_ta, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(f4_day_ta, LV_OBJ_FLAG_SCROLLABLE);
  }
  if (f4_extra) {
    // در ردیف دوم دقیقاً ۸ پیکسل زیر ۳ کارت بالا قرار می‌گیرد
    lv_obj_set_pos(f4_extra, -1, 101);
    lv_obj_set_size(f4_extra, 400, 286);
  }
  if (f4_pin_bar) {
    f4_style_admin_card(f4_pin_bar, 0, 0, 256, 78);
  }
  if (f4_stop_btn) {
    lv_obj_set_pos(f4_stop_btn, 266, 0);
    lv_obj_set_size(f4_stop_btn, 118, 78);
  }
  if (f4_err_btn_lbl) { lv_obj_set_width(f4_err_btn_lbl, 110); lv_obj_set_style_text_align(f4_err_btn_lbl, LV_TEXT_ALIGN_CENTER, 0); }
  if (f4_dt_bar) {
    f4_style_admin_card(f4_dt_bar, 0, 86, 384, 82);
  }
  const bool en = (GUI.system_language == 2);
  const int p1_x = en ? 10 : 140;
  const int p2_x = en ? 140 : 10;
  const int pw_w = 106;
  if (f4_pin_lbl) {
    lv_obj_set_pos(f4_pin_lbl, p1_x, -2);
    lv_obj_set_width(f4_pin_lbl, pw_w);
    lv_obj_set_style_text_letter_space(f4_pin_lbl, 0, 0);
    lv_obj_set_style_text_align(f4_pin_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(f4_pin_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  }
  if (f4_pin_cfm_cap) {
    lv_obj_set_pos(f4_pin_cfm_cap, p2_x, -2);
    lv_obj_set_width(f4_pin_cfm_cap, pw_w);
    lv_obj_set_style_text_letter_space(f4_pin_cfm_cap, 0, 0);
    lv_obj_set_style_text_align(f4_pin_cfm_cap, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(f4_pin_cfm_cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  }
  // دو کادر جمع‌وجور و هم‌تراز با پدینگ عمودی متقارن جهت وسط‌چینی دقیق متن و رقم
  if (f4_pin1) {
    lv_obj_set_pos(f4_pin1, p1_x, 30);
    lv_obj_set_size(f4_pin1, pw_w, 38);
    lv_obj_set_style_pad_left(f4_pin1, 0, 0);
    lv_obj_set_style_pad_right(f4_pin1, 0, 0);
    lv_obj_set_style_pad_top(f4_pin1, 2, 0);
    lv_obj_set_style_pad_bottom(f4_pin1, 2, 0);
    lv_obj_set_style_pad_left(f4_pin1, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_right(f4_pin1, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_top(f4_pin1, 2, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_bottom(f4_pin1, 2, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(f4_pin1, lv_color_hex(0x8e8e93), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(f4_pin1, LV_BASE_DIR_LTR, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin1, LV_TEXT_ALIGN_CENTER, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_letter_space(f4_pin1, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin1, LV_TEXT_ALIGN_CENTER, 0);
  }
  if (f4_pin2) {
    lv_obj_set_pos(f4_pin2, p2_x, 30);
    lv_obj_set_size(f4_pin2, pw_w, 38);
    lv_obj_set_style_pad_left(f4_pin2, 0, 0);
    lv_obj_set_style_pad_right(f4_pin2, 0, 0);
    lv_obj_set_style_pad_top(f4_pin2, 2, 0);
    lv_obj_set_style_pad_bottom(f4_pin2, 2, 0);
    lv_obj_set_style_pad_left(f4_pin2, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_right(f4_pin2, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_top(f4_pin2, 2, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_bottom(f4_pin2, 2, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(f4_pin2, lv_color_hex(0x8e8e93), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_base_dir(f4_pin2, LV_BASE_DIR_LTR, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin2, LV_TEXT_ALIGN_CENTER, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_letter_space(f4_pin2, 0, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(f4_pin2, LV_TEXT_ALIGN_CENTER, 0);
  }
  // دکمه چشم در ردیف عنوان بین دو کادر رمز (بدون سایه و کادر)
  if (f4_pin_admin_mid) {
    lv_obj_set_pos(f4_pin_admin_mid, 117, 0);
    lv_obj_set_size(f4_pin_admin_mid, 22, 26);
    lv_obj_set_style_bg_opa(f4_pin_admin_mid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(f4_pin_admin_mid, 0, 0);
    lv_obj_set_style_outline_width(f4_pin_admin_mid, 0, 0);
    lv_obj_set_style_shadow_width(f4_pin_admin_mid, 0, 0);
    lv_obj_set_style_pad_all(f4_pin_admin_mid, 0, 0);
    lv_obj_set_style_bg_opa(f4_pin_admin_mid, LV_OPA_TRANSP, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(f4_pin_admin_mid, 0, LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(f4_pin_admin_mid, 0, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(f4_pin_admin_mid, 0, LV_STATE_PRESSED);
  }
  const int dt_cap_y = en ? 4 : 0;
  if (f4_hh_cap) { lv_obj_set_pos(f4_hh_cap, 6, dt_cap_y); lv_obj_set_width(f4_hh_cap, 60); }
  if (f4_mi_cap) { lv_obj_set_pos(f4_mi_cap, 78, dt_cap_y); lv_obj_set_width(f4_mi_cap, 60); }
  // سال در چپ، ماه در وسط، روز در راست
  if (f4_yy_cap) { lv_obj_set_pos(f4_yy_cap, 152, dt_cap_y); lv_obj_set_width(f4_yy_cap, 68); }
  if (f4_mo_cap) { lv_obj_set_pos(f4_mo_cap, 234, dt_cap_y); lv_obj_set_width(f4_mo_cap, 54); }
  if (f4_dd_cap) { lv_obj_set_pos(f4_dd_cap, 304, dt_cap_y); lv_obj_set_width(f4_dd_cap, 54); }
  if (f4_date_lbl) lv_obj_add_flag(f4_date_lbl, LV_OBJ_FLAG_HIDDEN);
  if (f4_ta_hh) { lv_obj_set_pos(f4_ta_hh, 6, 30); lv_obj_set_size(f4_ta_hh, 60, 40); }
  if (f4_ta_mi) { lv_obj_set_pos(f4_ta_mi, 78, 30); lv_obj_set_size(f4_ta_mi, 60, 40); }
  if (f4_dt_div) {
    lv_obj_set_pos(f4_dt_div, 143, 20);
    lv_obj_set_size(f4_dt_div, 1, 48);
  }
  if (f4_ta_yy) { lv_obj_set_pos(f4_ta_yy, 152, 30); lv_obj_set_size(f4_ta_yy, 68, 40); }
  if (f4_ta_mo) { lv_obj_set_pos(f4_ta_mo, 234, 30); lv_obj_set_size(f4_ta_mo, 54, 40); }
  if (f4_ta_dd) { lv_obj_set_pos(f4_ta_dd, 304, 30); lv_obj_set_size(f4_ta_dd, 54, 40); }
  if (f4_dt_colon) lv_obj_set_pos(f4_dt_colon, 70, 38);
  if (f4_dt_sep_day_month) lv_obj_set_pos(f4_dt_sep_day_month, 223, 40);
  if (f4_dt_sep_month_year) lv_obj_set_pos(f4_dt_sep_month_year, 293, 40);
  if (f4_hdr_mask) {
    lv_obj_set_pos(f4_hdr_mask, -16, -16);
    lv_obj_set_size(f4_hdr_mask, 440, 31);
    lv_obj_move_foreground(f4_hdr_mask);
  }
  if (objects.lablef2_4) {
    lv_obj_t *tb = lv_obj_get_parent(objects.lablef2_4);
    if (tb) lv_obj_move_foreground(tb);
  }
  if (objects.btn2_1) lv_obj_move_foreground(objects.btn2_1);
  if (f1_help_btn) lv_obj_move_foreground(f1_help_btn);
  if (f1_lang_btn) lv_obj_move_foreground(f1_lang_btn);
  if (objects.btn_save) lv_obj_move_foreground(objects.btn_save);
  {
    const int FAC_W = 229;
    const int CAL_X = FAC_W + 8;
    const int CAL_W = 384 - CAL_X;
    if (f4_err_btn) {
      lv_obj_set_pos(f4_err_btn, 0, 176);
      lv_obj_set_size(f4_err_btn, 244, 46);
    }
    if (f4_err_btn_lbl) {
      lv_obj_set_width(f4_err_btn_lbl, 118);
      lv_obj_set_style_text_align(f4_err_btn_lbl, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(f4_err_btn_lbl, LV_ALIGN_LEFT_MID, 6, 0);
    }
    if (f4_err_num_box) {
      lv_obj_set_pos(f4_err_num_box, 130, 6);
      lv_obj_set_size(f4_err_num_box, 106, 34);
    }
    if (f4_err_ta) {
      lv_obj_set_pos(f4_err_ta, 2, 2);
      lv_obj_set_size(f4_err_ta, 66, 30);
    }
    if (f4_err_unit && f4_err_num_box)
      lv_obj_align(f4_err_unit, LV_ALIGN_RIGHT_MID, -5, -1);
    if (f4_js_btn) {
      lv_obj_set_pos(f4_js_btn, 252, 176);
      lv_obj_set_size(f4_js_btn, 132, 46);
    }
    if (f4_fac_btn) lv_obj_set_pos(f4_fac_btn, 0, 232);
    if (f4_cal_btn) {
      lv_obj_set_pos(f4_cal_btn, CAL_X, 232);
      lv_obj_set_size(f4_cal_btn, CAL_W, 40);
    }
    if (f4_apply_btn) lv_obj_add_flag(f4_apply_btn, LV_OBJ_FLAG_HIDDEN);
  }
  // کادرهای عدد: اسکرول داخلی خاموش
  lv_obj_t *f4_tas[] = { f4_pin1, f4_pin2, f4_ta_hh, f4_ta_mi, f4_ta_dd, f4_ta_mo, f4_ta_yy, f4_err_ta };
  for (unsigned ti = 0; ti < sizeof(f4_tas) / sizeof(f4_tas[0]); ti++) {
    if (f4_tas[ti]) lv_obj_clear_flag(f4_tas[ti], LV_OBJ_FLAG_SCROLLABLE);
  }

  if (objects.obj16 && f4_event_cb_count(objects.obj16, f4_blank_click_cb) != 1) {
    f4_remove_event_cb_all(objects.obj16, f4_blank_click_cb);
    lv_obj_add_flag(objects.obj16, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.obj16, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);
  }

  f4_apply_lang();
  f4_stop_sync_ui();
  f4_match_fa_en_title(objects.lablef2_27, objects.lablee2_27);
  f4_match_fa_en_title(objects.lablef2_24, objects.lablee2_24);
  f4_js_sync_btn();
  f4_err_sync_ui();
  f4_day_sync_ui();
  f4_timer_sync_ui();
  // ساعت همیشه تازه می‌شود؛ f4_fill_dt_tas فقط کادرِ در حال ویرایش را دست‌نخورده می‌گذارد.
  f4_load_dt_from_soft();

  if (!f4_unlocked) f4_gate_show(true);
  else f4_gate_show(false);
  // خانۀ مشترک هدر روی F4 همیشه در دسترس و بدون کادر اضافه باشد
  if (objects.btn2_1) {
    lv_obj_set_pos(objects.btn2_1, 6, 6);
    lv_obj_set_size(objects.btn2_1, 44, 42);
    lv_obj_set_style_border_width(objects.btn2_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(objects.btn2_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(objects.btn2_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(objects.btn2_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.btn2_1, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(objects.btn2_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.btn2_1);
  }

  f4_reflow_done = true;
  f4_reflow_lang_seen = (uint8_t)GUI.system_language;
  f4_reflow_unlocked_seen = f4_unlocked;
  f4_reflow_layout_seen = f4_layout_ver;
}

void admin_tabs_reflow() {
  f5_confirm_tick();
  f4_fac_cfm_tick();
  f4_usb_drain();
  {
    static uint32_t f4_lk_save_ms = 0;
    if (f4_pin_is_locked(NULL) && (millis() - f4_lk_save_ms) > 15000UL) {
      f4_pin_lock_persist();
      f4_lk_save_ms = millis();
    }
  }
  if (GUI.scr_id != 2) return;
  if (!objects.tabview2_1) return;
  int tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
  // ضربه روی نوار تب‌ها هم پد عددی را ببندد (تکمیل پوشش بستن پد)
  {
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(objects.tabview2_1);
    if (tab_btns && f4_event_cb_count(tab_btns, f4_blank_click_cb) == 0)
      lv_obj_add_event_cb(tab_btns, f4_blank_click_cb, LV_EVENT_CLICKED, NULL);
  }
#if F3_PIN_HELP_ENABLE
  if (tab != 2) {
    f3_pin_help_close();
    f3_q_suppress_until_ms = 0;
  } else {
    // کمربند ایمنی: suppress گیرکرده بیش از ۳ثانیه نماند
    if (f3_q_suppress_until_ms && (int32_t)(millis() - f3_q_suppress_until_ms) > 3000) {
      f3_q_suppress_until_ms = 0;
      f3_pin_help_disarm_q();
    }
    // اگر overlay رویداد رها نگرفت، با وضعیت indev فلگ را آزاد کن تا بستن ممکن شود
    if (f3_help_root && f3_help_wait_finger_up) {
      bool any_pressed = false;
      for (lv_indev_t *in = lv_indev_get_next(NULL); in; in = lv_indev_get_next(in)) {
        if (lv_indev_get_type(in) == LV_INDEV_TYPE_POINTER &&
            lv_indev_get_state(in) == LV_INDEV_STATE_PRESSED) {
          any_pressed = true;
          break;
        }
      }
      if (!any_pressed) f3_help_wait_finger_up = false;
    }
  }
#endif
  if (tab == 2) f3_pinmap_reflow();
  {
    static int f4_visit_tab = -1;
    if (tab == 3) {
      f4_admin_reflow();
      // هر ورود تازه به F4 از ابتدای فرم شروع می‌شود.
      if (f4_visit_tab != 3 && objects.obj16)
        lv_obj_scroll_to_y(objects.obj16, 0, LV_ANIM_OFF);
      // با USB کیبورد لمسی نشان نده تا فوکوس ورودی سخت‌افزاری پایدار بماند.
      if (GUI.virtual_kb_enable && !usb_keyboard_present() && f4_unlocked && f4_active_ta && f4_is_num_ta(f4_active_ta))
        f4_kb_show_for(f4_active_ta);
    } else {
      // بین تب‌های F رمز نخواه — فقط کیبورد/فوکوس F4 را ببند
      f4_chain_close();
      f4_kb_hide();
      f4_clear_focus_style();
      if (f4_gate_root && !lv_obj_has_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN)) {
        f4_gate_show(false);
        GUI.lock_screen = false;
      }
    }
    f4_visit_tab = tab;
  }
  if (tab == 4) f5_dev_reflow();
}

void f4_session_lock() {
  if (f4_active_ta || (f4_kb_root && lv_obj_is_valid(f4_kb_root) &&
                       !lv_obj_has_flag(f4_kb_root, LV_OBJ_FLAG_HIDDEN))) {
    GUI.buzzer_cmd_short = true;
    ui_toast("تغییرات ذخیره نشد", "Changes not saved", 1600);
  }
  f4_unlocked = false;
  f4_reflow_done = false;
  f4_clock_time_edited = false;
  f4_clock_live_minute = -1;
  f4_chain_close();
  f4_kb_hide();
  f4_clear_focus_style();
  f4_fac_cfm_close();
  // F4 must never flash its protected content before the PIN overlay appears.
  if (objects.obj16) {
    lv_obj_scroll_to_y(objects.obj16, 0, LV_ANIM_OFF);
    lv_obj_add_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
  }
  if (f4_gate_root && lv_obj_is_valid(f4_gate_root) &&
      !lv_obj_has_flag(f4_gate_root, LV_OBJ_FLAG_HIDDEN)) {
    f4_gate_show(false);
  }
  GUI.lock_screen = false;
  ui_resume_checkpoint();
}

static bool settings_help_is_inside(lv_obj_t *obj, lv_obj_t *target) {
  if (!obj || !target) return false;
  for (lv_obj_t *cur = obj; cur; cur = lv_obj_get_parent(cur))
    if (cur == target) return true;
  return false;
}

bool settings_control_help_try(lv_obj_t *obj) {
  if (!obj || !objects.tabview2_1) return false;
  const int tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
  if (tab == 2) return false;  // F3 راهنمای تخصصی مستقل دارد.

  if (tab == 0) {
    if (settings_help_is_inside(obj, f1_kb_chip))
      main_ctrl_help_show("کیبرد لمسی: نمایش صفحه‌کلید عددی روی نمایشگر را روشن یا خاموش می‌کند.",
                          "Touch keypad: enables or disables the on-screen number pad.");
    else if (settings_help_is_inside(obj, f1_buz_chip))
      main_ctrl_help_show("صدای کلیدها: بوق لمس و کلیدهای پنل را روشن یا خاموش می‌کند.",
                          "Key sound: enables or disables touch and panel-key beeps.");
    else if (settings_help_is_inside(obj, f1_print_btn))
      main_ctrl_help_show("چاپ: استفاده از خروجی چاپگر برچسب را روشن یا خاموش می‌کند.",
                          "Print: enables or disables the label-printer output.");
    else if (settings_help_is_inside(obj, f1_ncno) || settings_help_is_inside(obj, f1_estop_art))
      main_ctrl_help_show("نوع ورودی توقف اضطراری: حالت باز یا بسته بودن کنتاکت ایمنی را تعیین می‌کند.",
                          "Emergency-stop input type: selects normally-open or normally-closed wiring.");
    else if (settings_help_is_inside(obj, f1_center_card))
      main_ctrl_help_show("طول شاخه و عرض تیغه: عددهای پایه محاسبه طول و جبران برش را تنظیم می‌کند.",
                          "Stock length and kerf: sets the base length and blade-width compensation.");
    else if (settings_help_is_inside(obj, f1_lang_btn))
      main_ctrl_help_show("زبان: متن رابط کاربری را بین فارسی و انگلیسی عوض می‌کند.",
                          "Language: switches the interface between Persian and English.");
    else if (settings_help_is_inside(obj, f1_help_btn))
      main_ctrl_help_show("راهنما: توضیح کامل اتصالات و تنظیمات صفحه F1 را باز می‌کند.",
                          "Help: opens the complete F1 settings and wiring guide.");
    else
      main_ctrl_help_show("تنظیم عمومی F1؛ نگه‌داشتن فقط توضیح را نشان می‌دهد و مقدار را تغییر نمی‌دهد.",
                          "F1 general setting; holding shows help without changing it.");
    return true;
  }

  if (tab == 1) {
    if (settings_help_is_inside(obj, objects.obj9))
      main_ctrl_help_show("سایز اول خط: موقعیت سنسور ابتدای ریل را به میلی‌متر وارد می‌کند.",
                          "Start-line size: sets the rail start-sensor position in millimeters.");
    else if (settings_help_is_inside(obj, objects.obj10))
      main_ctrl_help_show("سایز آخر خط: حد نرم انتهای حرکت محور را به میلی‌متر تعیین می‌کند.",
                          "End-line size: sets the axis software end limit in millimeters.");
    else if (settings_help_is_inside(obj, objects.slider2_1))
      main_ctrl_help_show("سرعت حرکت موتور: سرعت اصلی محور را تنظیم می‌کند.",
                          "Motor speed: sets the axis main travel speed.");
    else if (settings_help_is_inside(obj, objects.slider2_2))
      main_ctrl_help_show("شتاب موتور: نرمی شروع و توقف حرکت خودکار را تنظیم می‌کند.",
                          "Motor ramp: adjusts smooth acceleration and deceleration.");
    else if (settings_help_is_inside(obj, f2_line_kb))
      main_ctrl_help_show("کیبرد عددی F2: عدد سایز اول یا آخر خط را وارد می‌کند؛ تأیید فقط تغییر موقت است.",
                          "F2 keypad: enters start/end size; Save stores the pending value.");
    else
      main_ctrl_help_show("تنظیم حرکت F2؛ نگه‌داشتن فقط راهنما را باز می‌کند و مقدار عوض نمی‌شود.",
                          "F2 motion setting; holding shows help without changing the value.");
    return true;
  }

  if (tab == 3) {
    if (settings_help_is_inside(obj, f4_stop_btn))
      main_ctrl_help_show("استاپ: حالت عادی بسته یا عادی باز کلید استاپ را تغییر می‌دهد؛ ثبت نهایی با ذخیره است.",
                          "Stop: changes the stop input between Normally Closed and Normally Open; Save confirms it.");
    else if (settings_help_is_inside(obj, f4_pin1))
      main_ctrl_help_show("رمز مدیر: رمز جدید مدیریت را وارد می‌کند.",
                          "Admin PIN: enters the new administrator code.");
    else if (settings_help_is_inside(obj, f4_pin2))
      main_ctrl_help_show("تکرار رمز مدیر: برای جلوگیری از اشتباه باید با کادر اول یکسان باشد.",
                          "Confirm PIN: must match the first field.");
    else if (settings_help_is_inside(obj, f4_dt_bar))
      main_ctrl_help_show("ساعت و تاریخ دستگاه: زمان سیستم را برای گزارش‌ها و گارانتی تنظیم می‌کند.",
                          "Device date and time: sets the clock used by reports and warranty.");
    else if (settings_help_is_inside(obj, f4_err_btn))
      main_ctrl_help_show("اصلاح خطا: مقدار جبران ثابت خطای اندازه‌گیری را روشن یا خاموش و تنظیم می‌کند.",
                          "Error correction: enables and sets fixed measurement compensation.");
    else if (settings_help_is_inside(obj, f4_js_btn))
      main_ctrl_help_show("جویستیک: اجازه حرکت دستی محور با ورودی جویستیک را روشن یا خاموش می‌کند.",
                          "Joystick: enables or disables manual axis control from the joystick.");
    else if (settings_help_is_inside(obj, f4_fac_btn))
      main_ctrl_help_show("ذخیره حالت کارخانه: تنظیمات فعلی را به‌عنوان مرجع بازگردانی مدیر ثبت می‌کند.",
                          "Factory snapshot: stores current settings as the administrator restore point.");
    else if (settings_help_is_inside(obj, f4_cal_btn)) {
      char fa[128];
      char en[128];
      const double mm_step = (double)STP.um_per_pulse / 1000.0;
      snprintf(fa, sizeof(fa), "هر استپ = %.6f میلی‌متر\nبرای کالیبره، دکمه را یک‌بار لمس کنید.", mm_step);
      snprintf(en, sizeof(en), "Each step = %.6f mm\nTap once to start calibration.", mm_step);
      main_ctrl_help_show(fa, en);
    }
    else
      main_ctrl_help_show("تنظیم مدیریتی F4؛ نگه‌داشتن فقط توضیح را نشان می‌دهد.",
                          "F4 administrator setting; holding only shows its help.");
    return true;
  }

  if (tab == 4) {
    if (settings_help_is_inside(obj, f5_tile_life))
      main_ctrl_help_show("تعداد برش کل: گزارش پنج شمارنده اصلیِ کل، امروز، دستی، لیست و بهینه را باز می‌کند.",
                          "Total cuts: opens the five main lifetime/today/mode counters.");
    else if (settings_help_is_inside(obj, f5_tile_user))
      main_ctrl_help_show("تعداد برش قابل صفر: با لمس کوتاه، پنجره تأیید صفرکردن این شمارنده را باز می‌کند.",
                          "Resettable cut count: opens its reset confirmation.");
    else if (settings_help_is_inside(obj, f5_tile_ver))
      main_ctrl_help_show("نسخه برنامه: فهرست اصلاحات نسخه فعلی و قبلی را باز می‌کند.",
                          "Firmware version: opens current and previous release notes.");
    else if (settings_help_is_inside(obj, f5_tile_prog))
      main_ctrl_help_show("تاریخ برنامه: سابقه نسخه‌های نصب‌شده روی همین سخت‌افزار را نشان می‌دهد.",
                          "Build date: opens this hardware's firmware installation history.");
    else if (settings_help_is_inside(obj, f5_tile_temp))
      main_ctrl_help_show("دمای پردازنده: پنل سخت افزار، دماها، درصد کار و حد هشدار را نشان می‌دهد.",
                          "CPU temperature: opens hardware metrics, temperatures, and the alarm threshold.");
    else if (settings_help_is_inside(obj, f5_tile_rtc))
      main_ctrl_help_show("دمای داخلی: دمای حسگر ساعت داخلی دستگاه را نشان می‌دهد.",
                          "Internal temperature: shows the RTC sensor temperature.");
    else if (settings_help_is_inside(obj, f5_btn_update))
      main_ctrl_help_show("بروزرسانی: صفحه انتخاب روش بروزرسانی USB یا وای‌فای را باز می‌کند.",
                          "Update: opens USB or Wi-Fi firmware update options.");
    else if (settings_help_is_inside(obj, f5_btn_warr) ||
             settings_help_is_inside(obj, f5_warr_tile))
      main_ctrl_help_show("گارانتی: تاریخ شروع را ثبت یا سه بازه پوشش و روزهای باقی‌مانده را نشان می‌دهد.",
                          "Warranty: records its start or shows three coverage periods and days remaining.");
    else
      main_ctrl_help_show("اطلاعات F5؛ نگه‌داشتن فقط توضیح این بخش را نشان می‌دهد.",
                          "F5 information; holding only shows this item's help.");
    return true;
  }
  return false;
}

bool ui_resume_f4_is_unlocked() { return f4_unlocked; }

void ui_resume_restore_f4_unlocked() {
  if (f4_unlocked) return;
  f4_unlocked = true;
  f4_gate_show(false);
  if (objects.obj16) lv_obj_clear_flag(objects.obj16, LV_OBJ_FLAG_HIDDEN);
  f4_admin_reflow();
  GUI.lock_screen = false;
}
