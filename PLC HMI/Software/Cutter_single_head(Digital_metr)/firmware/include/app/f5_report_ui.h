#pragma once
// F5 overlays: changelog polish, warranty remain, CPU/RTC temp, cut report
#include "fw_changelog.h"
#include "fw_install_history.h"
#include "cut_stats.h"
#include "iran_official_calendar.h"
#include "jalali.h"
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>

extern float cpu_temp_alarm_c;
void save_cpu_temp_alarm();
extern uint32_t total_cuts;
extern bool warranty_active;
extern char warranty_date[24];
void ui_toast(const char *fa, const char *en, uint32_t ms);
void settings_mark_dirty();
void ui_resume_restore_release_notes();
extern bool diagnostics_overlay_enabled;
void save_diagnostics_overlay_enabled();

static lv_obj_t *f5_ov_root = NULL;
static lv_obj_t *f5_history_scroll = NULL;
static lv_obj_t *f5_clock_time = NULL;
static lv_obj_t *f5_clock_hour = NULL;
static lv_obj_t *f5_clock_min = NULL;
static lv_obj_t *f5_clock_sec = NULL;
static lv_obj_t *f5_clock_date = NULL;
static lv_obj_t *f5_clock_weekday = NULL;
static lv_obj_t *f5_clock_event = NULL;
static lv_obj_t *f5_ov_close_btn = NULL;
static lv_obj_t *f5_clock_close_btn = NULL;
static lv_point_precise_t f5_clock_hour_pts[2];
static lv_point_precise_t f5_clock_min_pts[2];
static lv_point_precise_t f5_clock_sec_pts[2];
static uint32_t f5_clock_last_poll_ms = 0;
static int f5_clock_last_second = -1;
static uint16_t f5_history_page = 0;
static constexpr uint16_t F5_HISTORY_PAGE_SIZE = 10;
enum {
  F5_OV_KIND_NONE = 0,
  F5_OV_KIND_WARR = 1,
  F5_OV_KIND_CUTS = 2,
  F5_OV_KIND_CPU = 3,
  F5_OV_KIND_HISTORY = 4,
  F5_OV_KIND_RTC = 5,
  F5_OV_KIND_RELEASE = 6,
  F5_OV_KIND_MAKER = 7,
  F5_OV_KIND_CLOCK = 8,
};
static uint8_t f5_ov_kind = F5_OV_KIND_NONE;

void ui_resume_checkpoint(void);  // GUI_Func.h

static void f5_ov_close(void) {
  if (f5_ov_root) {
    lv_obj_delete(f5_ov_root);
    f5_ov_root = NULL;
  }
  f5_history_scroll = NULL;
  f5_clock_time = NULL;
  f5_clock_hour = f5_clock_min = f5_clock_sec = NULL;
  f5_clock_date = f5_clock_weekday = f5_clock_event = NULL;
  f5_ov_close_btn = f5_clock_close_btn = NULL;
  f5_clock_last_poll_ms = 0;
  f5_clock_last_second = -1;
  f5_ov_kind = F5_OV_KIND_NONE;
  GUI.lock_screen = false;
  ui_resume_checkpoint();
}

static void f5_ov_close_cb(lv_event_t *e) {
  (void)e;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  // ریشهٔ مودال را داخل callback همان فرزند حذف نکن.
  lv_async_call([](void *) { f5_ov_close(); }, NULL);
}

static lv_obj_t *f5_ov_make_shell_ex(const char *title_fa, const char *title_en,
                                     bool scrollable, int body_up_px) {
  f5_ov_close();
  const bool en = (GUI.system_language == 2);
  GUI.lock_screen = true;
  f5_ov_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f5_ov_root, 480, 320);
  lv_obj_set_pos(f5_ov_root, 0, 0);
  lv_obj_set_style_bg_color(f5_ov_root, lv_color_hex(0xF2F2F7), 0);
  lv_obj_set_style_bg_opa(f5_ov_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(f5_ov_root, 0, 0);
  lv_obj_set_style_pad_all(f5_ov_root, 0, 0);
  lv_obj_set_style_radius(f5_ov_root, 0, 0);
  lv_obj_clear_flag(f5_ov_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f5_ov_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *hdr = lv_obj_create(f5_ov_root);
  lv_obj_set_size(hdr, 480, 48);
  lv_obj_set_pos(hdr, 0, 0);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(hdr, 0, 0);
  lv_obj_set_style_pad_all(hdr, 0, 0);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *ht = lv_label_create(hdr);
  lv_obj_set_style_text_font(ht, en ? (const lv_font_t *)&lv_font_montserrat_16
                                    : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(ht, lv_color_hex(0x1C1C1E), 0);
  lv_label_set_text(ht, en ? title_en : title_fa);
  lv_obj_align(ht, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *xb = lv_button_create(hdr);
  f5_ov_close_btn = xb;
  lv_obj_set_size(xb, 40, 40);
  lv_obj_align(xb, LV_ALIGN_RIGHT_MID, -8, 0);
  lv_obj_set_style_bg_color(xb, lv_color_hex(0xFF3B30), 0);
  lv_obj_set_style_radius(xb, 10, 0);
  lv_obj_set_style_shadow_width(xb, 0, 0);
  lv_obj_add_event_cb(xb, f5_ov_close_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *xl = lv_label_create(xb);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_center(xl);

  int y = 48 - body_up_px;
  if (y < 36) y = 36;
  if (body_up_px > 0) {
    lv_obj_set_height(hdr, y);
    lv_obj_set_size(xb, 32, 32);
    lv_obj_align(xb, LV_ALIGN_RIGHT_MID, -8, 0);
  }
  lv_obj_t *scroll = lv_obj_create(f5_ov_root);
  lv_obj_set_size(scroll, 480, 320 - y);
  lv_obj_set_pos(scroll, 0, y);
  lv_obj_set_style_bg_color(scroll, lv_color_hex(0xF2F2F7), 0);
  lv_obj_set_style_bg_opa(scroll, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scroll, 0, 0);
  lv_obj_set_style_pad_hor(scroll, 16, 0);
  lv_obj_set_style_pad_ver(scroll, scrollable ? 12 : 4, 0);
  lv_obj_set_style_pad_row(scroll, scrollable ? 10 : 2, 0);
  lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
  if (scrollable) {
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(scroll, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(scroll, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  } else {
    lv_obj_clear_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(scroll, LV_DIR_NONE);
  }
  if (body_up_px > 0) {
    lv_obj_set_parent(xb, f5_ov_root);
    lv_obj_set_pos(xb, 424, 9);
    lv_obj_set_size(xb, 48, 48);
    lv_obj_set_style_radius(xb, 12, 0);
    lv_obj_set_style_text_font(xl, &lv_font_montserrat_24, 0);
    lv_obj_center(xl);
    lv_obj_move_foreground(xb);
  }
  return scroll;
}

static lv_obj_t *f5_ov_make_shell(const char *title_fa, const char *title_en) {
  return f5_ov_make_shell_ex(title_fa, title_en, true, 0);
}

static void f5_ov_set_header_title_font(const lv_font_t *fa_font, const lv_font_t *en_font) {
  if (!f5_ov_root) return;
  lv_obj_t *hdr = lv_obj_get_child(f5_ov_root, 0);
  if (!hdr) return;
  lv_obj_t *ht = lv_obj_get_child(hdr, 0);
  if (!ht) return;
  lv_obj_set_style_text_font(ht, (GUI.system_language == 2) ? en_font : fa_font, 0);
}

static void f5_ov_set_header_title_y(int y) {
  if (!f5_ov_root) return;
  lv_obj_t *hdr = lv_obj_get_child(f5_ov_root, 0);
  if (!hdr) return;
  lv_obj_t *ht = lv_obj_get_child(hdr, 0);
  if (ht) lv_obj_align(ht, LV_ALIGN_CENTER, 0, y);
}

static void f5_maker_add_text(lv_obj_t *card, const char *fa, const char *en_txt) {
  const bool en = (GUI.system_language == 2);
  lv_obj_t *lab = lv_label_create(card);
  lv_obj_set_width(lab, 440);
  lv_obj_set_style_text_font(lab, en ? (const lv_font_t *)&lv_font_montserrat_14
                                     : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lab, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(lab, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
  lv_label_set_text(lab, en ? en_txt : fa);
}

static void f5_maker_open() {
  const bool en = (GUI.system_language == 2);
  // بدنه از هدر و کلید بستن جداست؛ کل صفحه سفید و بدون بریدگی کارت‌ها
  lv_obj_t *scroll = f5_ov_make_shell_ex("سازنده دستگاه", "Device Manufacturer", false, 0);
  f5_ov_kind = F5_OV_KIND_MAKER;
  ui_resume_checkpoint();
  lv_obj_set_style_bg_color(scroll, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_pad_all(scroll, 0, 0);
  lv_obj_set_style_pad_top(scroll, 6, 0);
  lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *card = lv_obj_create(scroll);
  lv_obj_set_width(card, 480);
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(card, 0, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_hor(card, 20, 0);
  lv_obj_set_style_pad_ver(card, 2, 0);
  lv_obj_set_style_pad_row(card, 8, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  f5_maker_add_text(card,
    "تولید تخصصی PLC و تجهیزات الکترونیکی صنعتی",
    "Specialized manufacturing of industrial PLCs and electronic equipment.");
  f5_maker_add_text(card,
    "مرکز تخصصی خدمات فنی، تامین قطعات و پشتیبانی مستقیم مشتریان",
    "Dedicated technical service, genuine parts, and direct customer support.");

  const char *caps[] = {"Email", "Website", "Phone"};
  const char *vals[] = {"haffler.com@gmail.com", "haffler.com", "0913-169-7476"};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_size(row, 440, 42);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_pad_hor(row, 16, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cap = lv_label_create(row);
    lv_obj_set_style_text_font(cap, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(0x475569), 0);
    lv_label_set_text(cap, caps[i]);
    lv_obj_align(cap, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *val = lv_label_create(row);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_base_dir(val, LV_BASE_DIR_LTR, 0);
    lv_label_set_text(val, vals[i]);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, 0, 0);
  }

  // کلید اجرای ویزارد راه‌اندازی اولیه
  lv_obj_t *wiz_btn = lv_button_create(card);
  lv_obj_set_size(wiz_btn, 440, 36);
  lv_obj_set_style_bg_color(wiz_btn, lv_color_hex(0x0284C7), 0);
  lv_obj_set_style_radius(wiz_btn, 8, 0);
  lv_obj_add_event_cb(wiz_btn, [](lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    GUI.buzzer_cmd_short = true;
    f5_ov_close();
    oobe_wizard_start();
  }, LV_EVENT_RELEASED, NULL);
  const bool en_mkr = (GUI.system_language == 2);
  lv_obj_t *wb_lbl = lv_label_create(wiz_btn);
  lv_obj_set_style_text_font(wb_lbl, en_mkr ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(wb_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(wb_lbl, en_mkr ? "Re-run Initial Setup Wizard (OOBE)" : "اجرای مجدد ویزارد راه‌اندازی دستگاه (OOBE)");
  lv_obj_center(wb_lbl);
}

static void f5_maker_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED || ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  f5_maker_open();
}

static const char *f5_today_event(int gm, int gd, int jy, int jm, int jd, bool en) {
  const F5IranCalendarEvent lunar = f5_iran_calendar_event(jy, jm, jd);
  const char *lunar_text = f5_iran_calendar_event_text(lunar, en);
  if (lunar_text) return lunar_text;
  if (en) {
    if (gm == 1 && gd == 1) return "New Year's Day";
    if (gm == 3 && gd == 8) return "International Women's Day";
    if (gm == 5 && gd == 1) return "International Workers' Day";
    if (gm == 12 && gd == 25) return "Christmas Day";
    if (!f5_iran_calendar_has_year(jy)) return "Official lunar calendar unavailable for this year";
    return "No fixed-date occasion today";
  }
  if (jm == 1 && jd == 1) return "آغاز نوروز";
  if (jm == 1 && jd == 12) return "روز جمهوری اسلامی";
  if (jm == 1 && jd == 13) return "روز طبیعت";
  if (jm == 2 && jd == 12) return "روز معلم";
  if (jm == 3 && jd == 14) return "رحلت امام خمینی";
  if (jm == 3 && jd == 15) return "قیام پانزده خرداد";
  if (jm == 11 && jd == 22) return "پیروزی انقلاب اسلامی";
  if (jm == 12 && jd == 29) return "ملی شدن صنعت نفت";
  if (!f5_iran_calendar_has_year(jy)) return "تقویم رسمی قمری این سال ثبت نشده";
  return "امروز مناسبت ثبت شده ندارد";
}

static void f5_clock_set_hand(lv_obj_t *line, lv_point_precise_t pts[2],
                              float angle_deg, int length) {
  if (!line) return;
  const float a = (angle_deg - 90.0f) * 0.01745329252f;
  pts[0].x = 80; pts[0].y = 80;
  pts[1].x = (lv_value_precise_t)lroundf(80.0f + cosf(a) * length);
  pts[1].y = (lv_value_precise_t)lroundf(80.0f + sinf(a) * length);
  lv_line_set_points(line, pts, 2);
}

static bool f5_today_is_official_holiday(int gm, int gd, int jy, int jm, int jd, bool en) {
  if (f5_iran_calendar_event_is_holiday(f5_iran_calendar_event(jy, jm, jd))) return true;
  if (en) return (gm == 1 && gd == 1) || (gm == 12 && gd == 25);
  return (jm == 1 && jd >= 1 && jd <= 4) ||
         (jm == 1 && (jd == 12 || jd == 13)) ||
         (jm == 3 && (jd == 14 || jd == 15)) ||
         (jm == 11 && jd == 22) || (jm == 12 && jd == 29);
}

static void f5_clock_set_text_if_changed(lv_obj_t *label, const char *text) {
  if (!label || !text) return;
  const char *current = lv_label_get_text(label);
  if (!current || strcmp(current, text) != 0) lv_label_set_text(label, text);
}

static void f5_clock_overlay_tick() {
  if (f5_ov_kind != F5_OV_KIND_CLOCK || !f5_clock_hour) return;
  const uint32_t now = millis();
  if (f5_clock_last_poll_ms && (uint32_t)(now - f5_clock_last_poll_ms) < 125UL) return;
  f5_clock_last_poll_ms = now;
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  if (!ds3231_read_time(&y, &m, &d, &hh, &mi, &ss)) {
    const int64_t ep = soft_now_epoch();
    if (ep <= 0) return;
    epoch_to_civil(ep, &y, &m, &d, &hh, &mi, &ss);
  }
  if (f5_clock_last_second == ss) return;
  f5_clock_last_second = ss;
  f5_clock_set_hand(f5_clock_hour, f5_clock_hour_pts,
                    (hh % 12) * 30.0f + mi * 0.5f + ss / 120.0f, 43);
  f5_clock_set_hand(f5_clock_min, f5_clock_min_pts, mi * 6.0f + ss * 0.1f, 61);
  f5_clock_set_hand(f5_clock_sec, f5_clock_sec_pts, ss * 6.0f, 68);
  if (f5_clock_time) {
    char tb[24];
    snprintf(tb, sizeof(tb), "%02d:%02d:%02d", hh, mi, ss);
    f5_clock_set_text_if_changed(f5_clock_time, tb);
  }

  const bool en = (GUI.system_language == 2);
  int jy = 0, jm = 0, jd = 0;
  gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
  char db[32];
  snprintf(db, sizeof(db), en ? "%04d-%02d-%02d" : "%04d/%02d/%02d",
           en ? y : jy, en ? m : jm, en ? d : jd);
  f5_clock_set_text_if_changed(f5_clock_date, db);
  int64_t ep = civil_to_epoch(y, m, d, 0, 0, 0);
  int wd = (int)((ep / 86400LL + 4LL) % 7LL);
  if (wd < 0) wd += 7;
  const bool weekly_holiday = en ? (wd == 0) : (wd == 5);
  const bool official_holiday = f5_today_is_official_holiday(m, d, jy, jm, jd, en);
  static const char *en_days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  static const char *fa_days[] = {"یکشنبه", "دوشنبه", "سه شنبه", "چهارشنبه", "پنجشنبه", "جمعه", "شنبه"};
  static const char *en_months[] = {"January", "February", "March", "April", "May", "June",
                                    "July", "August", "September", "October", "November", "December"};
  static const char *fa_months[] = {"فروردین", "اردیبهشت", "خرداد", "تیر", "مرداد", "شهریور",
                                    "مهر", "آبان", "آذر", "دی", "بهمن", "اسفند"};
  if (f5_clock_weekday) {
    char weekday[64];
    snprintf(weekday, sizeof(weekday), "%s, %s", en ? en_days[wd] : fa_days[wd],
             en ? en_months[m - 1] : fa_months[jm - 1]);
    f5_clock_set_text_if_changed(f5_clock_weekday, weekday);
    lv_obj_set_style_text_color(f5_clock_weekday,
                                lv_color_hex(weekly_holiday ? 0xFF3B30 : 0x636366), 0);
  }
  f5_clock_set_text_if_changed(f5_clock_event, f5_today_event(m, d, jy, jm, jd, en));
  if (f5_clock_event)
    lv_obj_set_style_text_color(f5_clock_event,
                                lv_color_hex((weekly_holiday || official_holiday) ? 0xFF3B30 : 0x0A84FF), 0);
}

static void f5_clock_open() {
  const bool en = (GUI.system_language == 2);
  lv_obj_t *body = f5_ov_make_shell_ex("ساعت و تقویم", "Clock and calendar", false, 12);
  f5_ov_set_header_title_font(&ui_font_naz24, &lv_font_montserrat_18);
  f5_ov_set_header_title_y(0);
  f5_ov_kind = F5_OV_KIND_CLOCK;
  f5_clock_last_poll_ms = 0;
  f5_clock_last_second = -1;
  lv_obj_set_layout(body, LV_LAYOUT_NONE);
  lv_obj_set_style_pad_all(body, 0, 0);
  // این صفحه یک سطح سفید پیوسته است؛ زمینهٔ خاکستری نباید فقط زیر ضربدر دیده شود.
  lv_obj_set_style_bg_color(body, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
  lv_obj_t *face = lv_obj_create(body);
  // موقعیت منفی زیر هدر کلیپ می‌شود؛ ساعت از لبهٔ بالایی بدنه شروع می‌شود.
  lv_obj_set_pos(face, 160, 0);
  lv_obj_set_size(face, 160, 160);
  lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(face, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(face, 2, 0);
  lv_obj_set_style_border_color(face, lv_color_hex(0xD1D1D6), 0);
  lv_obj_set_style_pad_all(face, 0, 0);
  lv_obj_clear_flag(face, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
  for (int n = 1; n <= 12; n++) {
    const float a = (n * 30.0f - 90.0f) * 0.01745329252f;
    lv_obj_t *mark = lv_label_create(face);
    lv_obj_set_style_text_font(mark, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(mark, lv_color_hex(0x636366), 0);
    lv_label_set_text_fmt(mark, "%d", n);
    lv_obj_align(mark, LV_ALIGN_TOP_LEFT,
                 (int)lroundf(72.0f + cosf(a) * 66.0f),
                 (int)lroundf(72.0f + sinf(a) * 66.0f));
  }
  auto mk_hand = [&](uint32_t color, int width) {
    lv_obj_t *line = lv_line_create(face);
    lv_obj_set_pos(line, 0, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_clear_flag(line, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    return line;
  };
  f5_clock_hour = mk_hand(0x1C1C1E, 5);
  f5_clock_min = mk_hand(0x1C1C1E, 3);
  f5_clock_sec = mk_hand(0xFF3B30, 2);
  f5_clock_time = lv_label_create(body);
  lv_obj_set_style_text_font(f5_clock_time, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(f5_clock_time, lv_color_hex(0x0A84FF), 0);
  lv_obj_align(f5_clock_time, LV_ALIGN_TOP_MID, 0, 166);
  f5_clock_date = lv_label_create(body);
  lv_obj_set_style_text_font(f5_clock_date, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(f5_clock_date, lv_color_hex(0x1C1C1E), 0);
  lv_obj_align(f5_clock_date, LV_ALIGN_TOP_MID, 0, 198);
  f5_clock_weekday = lv_label_create(body);
  lv_obj_set_style_text_font(f5_clock_weekday, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(f5_clock_weekday, lv_color_hex(0x636366), 0);
  lv_obj_align(f5_clock_weekday, LV_ALIGN_TOP_MID, 0, 222);
  f5_clock_event = lv_label_create(body);
  lv_obj_set_width(f5_clock_event, 448);
  lv_obj_set_style_text_font(f5_clock_event, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(f5_clock_event, lv_color_hex(0x0A84FF), 0);
  lv_obj_set_style_text_align(f5_clock_event, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(f5_clock_event, LV_ALIGN_TOP_MID, 0, 246);

  // [UI/Task2] This read-only screen has one explicit exit target.
  if (f5_ov_close_btn) lv_obj_add_flag(f5_ov_close_btn, LV_OBJ_FLAG_HIDDEN);
  f5_clock_close_btn = lv_button_create(f5_ov_root);
  lv_obj_set_pos(f5_clock_close_btn, 424, 4);
  lv_obj_set_size(f5_clock_close_btn, 48, 48);
  lv_obj_set_style_bg_color(f5_clock_close_btn, lv_color_hex(0xFF3B30), 0);
  lv_obj_set_style_bg_opa(f5_clock_close_btn, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(f5_clock_close_btn, 12, 0);
  lv_obj_set_style_border_width(f5_clock_close_btn, 0, 0);
  lv_obj_set_style_shadow_width(f5_clock_close_btn, 0, 0);
  lv_obj_add_event_cb(f5_clock_close_btn, f5_ov_close_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *close_label = lv_label_create(f5_clock_close_btn);
  lv_obj_set_style_text_font(close_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(close_label, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
  lv_obj_center(close_label);
  lv_obj_move_foreground(f5_clock_close_btn);
  f5_clock_overlay_tick();
  ui_resume_checkpoint();
}

static void f5_clock_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED || ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  f5_clock_open();
}

static void f5_ov_fmt_at(char *b, size_t n, const char *stored_at, bool en) {
  if (stored_at && stored_at[0]) {
    snprintf(b, n, "%s", stored_at);
    return;
  }
  static const char mons[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mon[4] = {0};
  int day = 0, year = 0, h = 0, min = 0, sec = 0;
  if (sscanf(__DATE__, "%3s %d %d", mon, &day, &year) == 3 &&
      sscanf(__TIME__, "%d:%d:%d", &h, &min, &sec) >= 2) {
    const char *p = strstr(mons, mon);
    if (p && ((p - mons) % 3) == 0) {
      int y = year, m = (int)((p - mons) / 3) + 1, d = day;
      if (!en) {
        int jy = 0, jm = 0, jd = 0;
        gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
        snprintf(b, n, "%04d/%02d/%02d  %02d:%02d", jy, jm, jd, h, min);
      } else {
        snprintf(b, n, "%04d-%02d-%02d  %02d:%02d", y, m, d, h, min);
      }
      return;
    }
  }
  snprintf(b, n, "--");
}

static void f5_cl_add_section(lv_obj_t *scroll, const char *ver, const char *at_stored,
                              const char * const *lines, int n, bool current) {
  if (!scroll || !ver || n <= 0 || !lines) return;
  const bool en = (GUI.system_language == 2);
  lv_color_t title_col = current ? lv_color_hex(0x000000) : lv_color_hex(0x8E8E93);
  lv_color_t body_col = current ? lv_color_hex(0x1C1C1E) : lv_color_hex(0x636366);
  lv_color_t card_bg = current ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xEBEBF0);
  lv_color_t accent = current ? lv_color_hex(0x0A84FF) : lv_color_hex(0x8E8E93);

  lv_obj_t *card = lv_obj_create(scroll);
  lv_obj_set_width(card, 448);
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(card, card_bg, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 14, 0);
  lv_obj_set_style_pad_row(card, 6, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *accent_bar = lv_obj_create(card);
  lv_obj_set_size(accent_bar, 420, 3);
  lv_obj_set_style_bg_color(accent_bar, accent, 0);
  lv_obj_set_style_bg_opa(accent_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(accent_bar, 2, 0);
  lv_obj_set_style_border_width(accent_bar, 0, 0);
  lv_obj_set_style_pad_all(accent_bar, 0, 0);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(title, title_col, 0);
  lv_label_set_text(title, ver);

  char atb[40];
  f5_ov_fmt_at(atb, sizeof(atb), at_stored, en);
  lv_obj_t *at = lv_label_create(card);
  lv_obj_set_style_text_font(at, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(at, lv_color_hex(0x8E8E93), 0);
  lv_label_set_text(at, atb);

  for (int i = 0; i < n; i++) {
    if (!lines[i] || !lines[i][0]) continue;
    lv_obj_t *lab = lv_label_create(card);
    lv_obj_set_width(lab, 420);
    // متن دفترچه همیشه فارسی است؛ تغییر زبان نباید آن را با فونت لاتین و مربع نمایش دهد.
    lv_obj_set_style_text_font(lab, &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(lab, body_col, 0);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_base_dir(lab, LV_BASE_DIR_RTL, 0);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lab, lines[i]);
  }
}

static void f5_history_format_epoch(char *out, size_t n, int64_t epoch, bool en) {
  if (!out || n == 0) return;
  if (epoch <= 0) {
    snprintf(out, n, "--");
    return;
  }
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  epoch_to_civil(epoch, &y, &m, &d, &hh, &mi, &ss);
  if (en) {
    snprintf(out, n, "%04d-%02d-%02d  %02d:%02d:%02d", y, m, d, hh, mi, ss);
  } else {
    int jy = 0, jm = 0, jd = 0;
    gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
    snprintf(out, n, "%04d/%02d/%02d  %02d:%02d:%02d", jy, jm, jd, hh, mi, ss);
  }
}

static void f5_history_add_record(lv_obj_t *parent, const FwInstallHistoryRecord &r,
                                  bool current) {
  if (!parent) return;
  const bool en = (GUI.system_language == 2);
  const uint32_t accent = current ? 0x0A84FF : 0x8E8E93;

  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_width(card, 448);
  lv_obj_set_height(card, 84);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(current ? 0xB9DAFF : 0xD1D1D6), 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *bar = lv_obj_create(card);
  lv_obj_set_pos(bar, en ? 0 : 442, 0);
  lv_obj_set_size(bar, 6, 74);
  lv_obj_set_style_bg_color(bar, lv_color_hex(accent), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 3, 0);
  lv_obj_set_style_pad_all(bar, 0, 0);
  lv_obj_clear_flag(bar, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

  lv_obj_t *ver = lv_label_create(card);
  lv_obj_set_style_text_font(ver, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(ver, lv_color_hex(current ? 0x0A84FF : 0x1C1C1E), 0);
  lv_label_set_text_fmt(ver, "V%s", r.version);
  lv_obj_align(ver, LV_ALIGN_TOP_LEFT, 14, 7);

  lv_obj_t *seq = lv_label_create(card);
  lv_obj_set_style_text_font(seq, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(seq, lv_color_hex(0x8E8E93), 0);
  lv_label_set_text_fmt(seq, "#%lu", (unsigned long)r.seq);
  lv_obj_align(seq, LV_ALIGN_TOP_RIGHT, -14, 9);

  char build_at[32];
  char install_at[32];
  f5_history_format_epoch(build_at, sizeof(build_at), r.build_epoch, en);
  f5_history_format_epoch(install_at, sizeof(install_at), r.install_epoch, en);

  auto add_time_row = [&](int y, const char *fa, const char *en_txt, const char *value) {
    lv_obj_t *cap = lv_label_create(card);
    lv_obj_set_width(cap, 118);
    lv_obj_set_style_text_font(cap, en ? (const lv_font_t *)&lv_font_montserrat_14
                                       : (const lv_font_t *)&ui_font_naz18b, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(0x636366), 0);
    lv_obj_set_style_text_align(cap, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_base_dir(cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
    lv_label_set_text(cap, en ? en_txt : fa);
    lv_obj_set_pos(cap, en ? 14 : 316, y);

    lv_obj_t *val = lv_label_create(card);
    lv_obj_set_width(val, 286);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_text_align(val, en ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_base_dir(val, LV_BASE_DIR_LTR, 0);
    lv_label_set_text(val, value);
    lv_obj_set_pos(val, en ? 148 : 16, y + 5);
  };
  add_time_row(28, "زمان ساخت برنامه", "Code build", build_at);
  add_time_row(51, "نصب روی دستگاه", "Installed", install_at);
}

static void f5_history_render();

static void f5_history_render_async(void *) {
  f5_history_render();
}

static void f5_history_page_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const int dir = (int)(intptr_t)lv_event_get_user_data(e);
  uint32_t total = 0;
  uint16_t count = 0;
  fw_install_history_stats(&total, &count);
  (void)total;
  const uint16_t pages =
    count ? (uint16_t)((count + F5_HISTORY_PAGE_SIZE - 1U) / F5_HISTORY_PAGE_SIZE) : 1;
  if (dir < 0 && f5_history_page > 0) f5_history_page--;
  else if (dir > 0 && f5_history_page + 1U < pages) f5_history_page++;
  else {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  lv_async_call(f5_history_render_async, NULL);
}

static void f5_history_render() {
  if (!f5_history_scroll || !lv_obj_is_valid(f5_history_scroll)) return;
  lv_obj_clean(f5_history_scroll);
  const bool en = (GUI.system_language == 2);
  uint32_t total = 0;
  uint16_t count = 0;
  fw_install_history_stats(&total, &count);
  (void)total;
  const uint16_t pages =
    count ? (uint16_t)((count + F5_HISTORY_PAGE_SIZE - 1U) / F5_HISTORY_PAGE_SIZE) : 1;
  const bool show_pager = pages > 1U;
  if (f5_history_page >= pages) f5_history_page = (uint16_t)(pages - 1U);

  lv_obj_t *summary = lv_obj_create(f5_history_scroll);
  lv_obj_set_width(summary, 448);
  lv_obj_set_height(summary, 56);
  lv_obj_set_style_bg_color(summary, lv_color_hex(0xE4EEFB), 0);
  lv_obj_set_style_radius(summary, 12, 0);
  lv_obj_set_style_border_width(summary, 0, 0);
  lv_obj_set_style_pad_all(summary, 0, 0);
  lv_obj_clear_flag(summary, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *sum_cap = lv_label_create(summary);
  lv_obj_set_style_text_font(sum_cap, en ? (const lv_font_t *)&lv_font_montserrat_14
                                         : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(sum_cap, lv_color_hex(0x3A3A3C), 0);
  lv_label_set_text(sum_cap, en ? "Uploads recorded on this PLC"
                                : "سوابق نسخه های ثبت شده روی دستگاه");
  lv_obj_align(sum_cap, LV_ALIGN_TOP_MID, 0, 5);

  lv_obj_t *sum_val = lv_label_create(summary);
  lv_obj_set_style_text_font(sum_val, en ? (const lv_font_t *)&lv_font_montserrat_16
                                         : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(sum_val, lv_color_hex(0x0A84FF), 0);
  lv_label_set_text_fmt(sum_val, en ? "Page %u of %u" : "صفحه %u از %u",
                        (unsigned)(f5_history_page + 1U), (unsigned)pages);
  lv_obj_align(sum_val, LV_ALIGN_BOTTOM_MID, 0, -4);

  if (count == 0) {
    lv_obj_t *empty = lv_label_create(f5_history_scroll);
    lv_obj_set_width(empty, 448);
    lv_obj_set_style_text_font(empty, en ? (const lv_font_t *)&lv_font_montserrat_14
                                         : (const lv_font_t *)&ui_font_naz18b, 0);
    lv_obj_set_style_text_color(empty, lv_color_hex(0x636366), 0);
    lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(empty, en ? "No installation has been recorded yet."
                                : "هنوز نصب جدیدی روی این دستگاه ثبت نشده است");
  } else {
    const uint16_t first = (uint16_t)(f5_history_page * F5_HISTORY_PAGE_SIZE);
    const uint16_t end =
      (uint16_t)min((uint32_t)count, (uint32_t)first + F5_HISTORY_PAGE_SIZE);
    for (uint16_t i = first; i < end; ++i) {
      FwInstallHistoryRecord r{};
      if (fw_install_history_read_newest(i, &r))
        f5_history_add_record(f5_history_scroll, r, i == 0);
    }
  }

  if (show_pager) {
    lv_obj_t *pager = lv_obj_create(f5_history_scroll);
    lv_obj_set_width(pager, 448);
    lv_obj_set_height(pager, 48);
    lv_obj_set_style_bg_opa(pager, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pager, 0, 0);
    lv_obj_set_style_pad_all(pager, 0, 0);
    lv_obj_clear_flag(pager, LV_OBJ_FLAG_SCROLLABLE);

    auto make_page_btn = [&](lv_align_t align, int x, const char *txt, int dir, bool enabled) {
    lv_obj_t *btn = lv_button_create(pager);
    lv_obj_set_size(btn, 120, 42);
    lv_obj_align(btn, align, x, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(enabled ? 0x0A84FF : 0xC7C7CC), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    if (enabled)
      lv_obj_add_event_cb(btn, f5_history_page_cb, LV_EVENT_CLICKED, (void *)(intptr_t)dir);
    else
      lv_obj_add_state(btn, LV_STATE_DISABLED);
    lv_obj_t *lab = lv_label_create(btn);
    lv_obj_set_style_text_font(lab, en ? (const lv_font_t *)&lv_font_montserrat_14
                                       : (const lv_font_t *)&ui_font_naz18b, 0);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
    };
    make_page_btn(LV_ALIGN_LEFT_MID, 0, en ? "Newer" : "جدیدتر", -1, f5_history_page > 0);
    make_page_btn(LV_ALIGN_RIGHT_MID, 0, en ? "Older" : "قدیمی تر", 1,
                  f5_history_page + 1U < pages);

    lv_obj_t *page_lbl = lv_label_create(pager);
    lv_obj_set_style_text_font(page_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(page_lbl, lv_color_hex(0x636366), 0);
    lv_label_set_text_fmt(page_lbl, "%u / %u",
                          (unsigned)(f5_history_page + 1U), (unsigned)pages);
    lv_obj_center(page_lbl);
  }
  lv_obj_scroll_to_y(f5_history_scroll, 0, LV_ANIM_OFF);
}

static void f5_history_open() {
  fw_install_history_register_current();
  f5_history_page = 0;
  f5_history_scroll = f5_ov_make_shell("سوابق نصب برنامه", "Firmware install history");
  lv_obj_set_style_width(f5_history_scroll, 8, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(f5_history_scroll, 4, LV_PART_SCROLLBAR);
  f5_ov_kind = F5_OV_KIND_HISTORY;
  f5_history_render();
  ui_resume_checkpoint();
}

static void f5_changelog_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  f5_history_open();
}

static bool f5_parse_warranty_start(int *gy, int *gm, int *gd) {
  if (!warranty_active || !warranty_date[0]) return false;
  int a = 0, b = 0, c = 0;
  if (sscanf(warranty_date, "%d/%d/%d", &a, &b, &c) == 3) {
    jalali_to_gregorian(a, b, c, gy, gm, gd);
    return true;
  }
  if (sscanf(warranty_date, "%d-%d-%d", &a, &b, &c) == 3) {
    *gy = a;
    *gm = b;
    *gd = c;
    return true;
  }
  return false;
}

static int f5_days_remain(int years) {
  int sy = 0, sm = 0, sd = 0;
  if (!f5_parse_warranty_start(&sy, &sm, &sd)) return -1;
  int ey = sy + years, em = sm, ed = sd;
  if (ed > 28) ed = 28;
  int64_t end = civil_to_epoch(ey, em, ed, 0, 0, 0);
  int64_t now = soft_now_epoch();
  if (now <= 0) {
    int y, m, d, hh, mi, ss;
    if (ds3231_read_time(&y, &m, &d, &hh, &mi, &ss))
      now = civil_to_epoch(y, m, d, hh, mi, ss);
  }
  if (now <= 0) return -1;
  int64_t rem = (end - now) / 86400;
  if (rem < 0) rem = 0;
  return (int)rem;
}

static void f5_warr_add_row(lv_obj_t *scroll, const char *fa, const char *en_t, int years, uint32_t color) {
  const bool en = (GUI.system_language == 2);
  int days = f5_days_remain(years);
  lv_obj_t *card = lv_obj_create(scroll);
  lv_obj_set_width(card, 448);
  lv_obj_set_height(card, 54);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 6, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_obj_set_style_text_font(t, en ? (const lv_font_t *)&lv_font_montserrat_14
                                   : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x1C1C1E), 0);
  lv_label_set_text(t, en ? en_t : fa);
  lv_obj_align(t, en ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t *v = lv_label_create(card);
  lv_obj_set_style_text_font(v, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_color(v, lv_color_hex(color), 0);
  if (days < 0) lv_label_set_text(v, "--");
  else lv_label_set_text_fmt(v, "%d", days);
  lv_obj_align(v, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  lv_obj_t *u = lv_label_create(card);
  lv_obj_set_style_text_font(u, en ? (const lv_font_t *)&lv_font_montserrat_14
                                   : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(u, lv_color_hex(0x8E8E93), 0);
  lv_label_set_text(u, en ? "days left" : "\u0631\u0648\u0632 \u0645\u0627\u0646\u062f\u0647");
  lv_obj_align_to(u, v, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
}

static void f5_warranty_report_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (!warranty_active) {
    GUI.buzzer_cmd_short = true;
    open_warranty_screen();
    return;
  }
  GUI.buzzer_cmd_short = true;
  const bool en = (GUI.system_language == 2);
  // ۱۵px بالا + بدون اسکرول — در یک صفحه جا می‌شود
  lv_obj_t *scroll = f5_ov_make_shell_ex("\u067e\u0648\u0634\u0634 \u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u0645\u062d\u0635\u0648\u0644",
                                         "Product coverage", false, -30);
  f5_ov_kind = F5_OV_KIND_WARR;
  ui_resume_checkpoint();
  lv_obj_set_style_pad_row(scroll, 6, 0);
  lv_obj_t *intro = lv_label_create(scroll);
  lv_obj_set_width(intro, 448);
  lv_obj_set_style_text_font(intro, en ? (const lv_font_t *)&lv_font_montserrat_14
                                       : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(intro, lv_color_hex(0x3A3A3C), 0);
  lv_obj_set_style_text_align(intro, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(intro, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_long_mode(intro, LV_LABEL_LONG_WRAP);
  lv_label_set_text(intro, en
    ? "Days left from warranty start until each coverage end."
    : "\u0627\u0632 \u062a\u0627\u0631\u06cc\u062e \u0634\u0631\u0648\u0639 \u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u062a\u0627 \u067e\u0627\u06cc\u0627\u0646 \u0647\u0631 \u067e\u0648\u0634\u0634\u060c \u0631\u0648\u0632 \u0645\u0627\u0646\u062f\u0647 \u0646\u0634\u0627\u0646 \u062f\u0627\u062f\u0647 \u0645\u06cc \u0634\u0648\u062f\u002e");

  f5_warr_add_row(scroll, "\u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u0645\u062d\u0635\u0648\u0644 \u06cc\u06a9 \u0633\u0627\u0644\u0647",
                  "1-year product warranty", 1, 0x0A84FF);
  f5_warr_add_row(scroll, "\u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u0645\u062d\u0635\u0648\u0644 \u062f\u0648 \u0633\u0627\u0644\u0647",
                  "2-year product warranty", 2, 0x34C759);
  f5_warr_add_row(scroll, "\u062e\u062f\u0645\u0627\u062a \u067e\u0633 \u0627\u0632 \u0641\u0631\u0648\u0634 \u062f\u0647 \u0633\u0627\u0644\u0647",
                  "10-year after-sales service", 10, 0xFF9500);
}

static void f5_diagnostics_overlay_switch_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
  diagnostics_overlay_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
  // مانند تمام تنظیمات F1..F5: تا Save فقط در RAM است و با تعویض صفحه برمی‌گردد.
  settings_mark_dirty();
  GUI.buzzer_cmd_short = true;
}

extern SemaphoreHandle_t touchWireMutex;

// PCF8574/PCF8574A روی بازهٔ استاندارد I2C پاسخ می‌دهند. این probe فقط هنگام
// بازشدن پنجره اجرا می‌شود و هیچ رجیستر یا خروجی ماژول را تغییر نمی‌دهد.
static bool f5_i2c_device_present(uint8_t address) {
  bool ok = false;
  if (touchWireMutex) {
    if (xSemaphoreTakeRecursive(touchWireMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
  }
  touchWire.beginTransmission(address);
  ok = (touchWire.endTransmission(true) == 0);
  if (touchWireMutex) xSemaphoreGiveRecursive(touchWireMutex);
  return ok;
}

static bool f5_i2c_gpio_expander_present(uint8_t *address) {
  for (uint8_t a = 0x20; a <= 0x27; ++a) {
    if (f5_i2c_device_present(a)) { if (address) *address = a; return true; }
  }
  for (uint8_t a = 0x38; a <= 0x3F; ++a) {
    if (f5_i2c_device_present(a)) { if (address) *address = a; return true; }
  }
  return false;
}

static void f5_cpu_temp_open() {
  const bool en = (GUI.system_language == 2);
  lv_obj_t *body = f5_ov_make_shell_ex("دمای پردازنده و اطلاعات",
                                       "CPU information", false, 0);
  f5_ov_set_header_title_font(&ui_font_naz18b, &lv_font_montserrat_16);
  f5_ov_kind = F5_OV_KIND_CPU;
  ui_resume_checkpoint();

  lv_obj_set_layout(body, LV_LAYOUT_NONE);
  lv_obj_set_style_pad_all(body, 0, 0);

  const uint32_t heap_total = ESP.getHeapSize();
  const uint32_t heap_free = ESP.getFreeHeap();
  const uint32_t psram_total = ESP.getPsramSize();
  const uint32_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  // درصد برنامه باید با ظرفیت پارتیشنِ درحال اجرا سنجیده شود، نه کل چیپ 16MiB.
  const esp_partition_t *running_app = esp_ota_get_running_partition();
  const uint32_t app_total = running_app ? running_app->size : 0x700000UL;
  const uint32_t firmware_used = ESP.getSketchSize();
  const uint32_t flash_pct = app_total ? min(100UL, (firmware_used * 100UL) / app_total) : 0;
  const uint32_t ram_pct = heap_total ? min(100UL, ((heap_total - heap_free) * 100UL) / heap_total) : 0;
  const uint32_t psram_pct = psram_total ? min(100UL, ((psram_total - psram_free) * 100UL) / psram_total) : 0;
  const float cpu_temp = temperatureRead();
  uint8_t expander_addr = 0;
  const bool expander_present = f5_i2c_gpio_expander_present(&expander_addr);

  auto add_metric = [&](int x, int y, const char *fa, const char *en_txt, const char *value,
                        uint32_t value_col, bool value_fa = false) {
    lv_obj_t *m = lv_obj_create(body);
    lv_obj_set_pos(m, x, y);
    lv_obj_set_size(m, 214, 50);
    lv_obj_set_style_bg_color(m, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(m, 16, 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_set_style_pad_hor(m, 10, 0);
    lv_obj_set_style_pad_ver(m, 0, 0);
    lv_obj_set_style_shadow_width(m, 0, 0);
    lv_obj_clear_flag(m, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cap = lv_label_create(m);
    lv_obj_set_width(cap, 192);
    lv_obj_set_style_text_font(cap, en ? (const lv_font_t *)&lv_font_montserrat_12
                                       : (const lv_font_t *)&ui_font_naz18b, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(0x636366), 0);
    lv_obj_set_style_text_align(cap, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_base_dir(cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
    lv_label_set_long_mode(cap, LV_LABEL_LONG_CLIP);
    lv_label_set_text(cap, en ? en_txt : fa);
    lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t *val = lv_label_create(m);
    lv_obj_set_width(val, 192);
    lv_obj_set_style_text_font(val, value_fa ? (const lv_font_t *)&ui_font_naz18b
                                             : (const lv_font_t *)&lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(value_col), 0);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(val, value_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_label_set_text(val, value);
    lv_obj_align(val, LV_ALIGN_BOTTOM_MID, 0, -3);
  };

  char v_flash[32], v_ram[24], v_psram[32], v_temp[24], v_expander[32], v_rtc[24], v_alarm[24];
  const uint32_t mib = 1024UL * 1024UL;
  const uint32_t used_tenth_mib = mib ? ((firmware_used % mib) * 10UL) / mib : 0;
  snprintf(v_flash, sizeof(v_flash), "%lu.%lu/%lu MB | %lu%%",
           (unsigned long)(firmware_used / mib), (unsigned long)used_tenth_mib,
           (unsigned long)(app_total / mib), (unsigned long)flash_pct);
  snprintf(v_ram, sizeof(v_ram), "%lu%% | %lu KB", (unsigned long)ram_pct,
           (unsigned long)(heap_free / 1024UL));
  if (psram_total) {
    snprintf(v_psram, sizeof(v_psram), "%lu MB | %lu%%", (unsigned long)(psram_total / mib),
             (unsigned long)psram_pct);
  } else {
    snprintf(v_psram, sizeof(v_psram), en ? "Not available" : "در دسترس نیست");
  }
  snprintf(v_temp, sizeof(v_temp), "%.1f C", (double)cpu_temp);
  snprintf(v_alarm, sizeof(v_alarm), "%.0f C", (double)cpu_temp_alarm_c);
  if (expander_present) snprintf(v_expander, sizeof(v_expander), "PCF8574 @ 0x%02X", expander_addr);
  else snprintf(v_expander, sizeof(v_expander), en ? "Not connected" : "وصل نیست");
  // آی‌سی ساعت روی همان باس تاچ است؛ پروبِ تک‌تایی می‌تواند هم‌زمان با خواندن تاچ خطای کاذب بدهد
  // → پاسخ کاملِ آی‌سی (خواندن ساعت) هم دلیل حضور است. متن هم دوزبانه شد.
  bool f5_rtc_ok = f5_i2c_device_present(DS3231_I2C_ADDR);
  if (!f5_rtc_ok) {
    int y_, m_, d_, h_, mi_, s_;
    f5_rtc_ok = ds3231_read_time(&y_, &m_, &d_, &h_, &mi_, &s_);
  }
  snprintf(v_rtc, sizeof(v_rtc), f5_rtc_ok ? "DS3231" : (en ? "Not found" : "پیدا نشد"));

  // دما، اول و واضح دیده می‌شود؛ اطلاعات فرعی زیر آن در شبکهٔ آرام و خوانا می‌آیند.
  lv_obj_t *temp_hero = lv_obj_create(body);
  lv_obj_set_pos(temp_hero, 12, 8);
  lv_obj_set_size(temp_hero, 456, 58);
  lv_obj_set_style_bg_color(temp_hero, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(temp_hero, 16, 0);
  lv_obj_set_style_border_width(temp_hero, 0, 0);
  lv_obj_set_style_pad_hor(temp_hero, 16, 0);
  lv_obj_set_style_shadow_width(temp_hero, 0, 0);
  lv_obj_clear_flag(temp_hero, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *temp_cap = lv_label_create(temp_hero);
  lv_obj_set_style_text_font(temp_cap, en ? (const lv_font_t *)&lv_font_montserrat_14
                                           : (const lv_font_t *)&ui_font_naz20b, 0);
  lv_obj_set_style_text_color(temp_cap, lv_color_hex(0x636366), 0);
  lv_obj_set_style_text_align(temp_cap, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(temp_cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(temp_cap, en ? "CPU temperature" : "دمای پردازنده");
  lv_obj_align(temp_cap, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_t *temp_val = lv_label_create(temp_hero);
  lv_obj_set_style_text_font(temp_val, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(temp_val, lv_color_hex(cpu_temp >= cpu_temp_alarm_c ? 0xFF3B30 : 0x0A84FF), 0);
  lv_label_set_text(temp_val, v_temp);
  lv_obj_align(temp_val, en ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);

  add_metric(12, 74,  "برنامه و حافظه", "Firmware / app partition", v_flash, 0xFF9500);
  add_metric(254, 74, "رم آزاد", "RAM used / free", v_ram, 0x0A84FF);
  add_metric(12, 126, "پی اس رم", "PSRAM / used space", v_psram,
              psram_total ? 0x34C759 : 0xFF3B30, !en && !psram_total);
  add_metric(254, 126, "برد ورودی خروجی", "I2C GPIO expander", v_expander,
              expander_present ? 0x34C759 : 0xFF3B30, !en && !expander_present);
  add_metric(12, 178, "ساعت داخلی", "Hardware clock", v_rtc, f5_rtc_ok ? 0x34C759 : 0xFF3B30);
  add_metric(254, 178, "هشدار دما", "Temperature alarm", v_alarm, 0xFF9500);

  // این کلید تنها محل کنترل نوار اختیاریِ اطلاعات زنده است؛ در حالت عادی خاموش می‌ماند.
  lv_obj_t *diag = lv_obj_create(body);
  lv_obj_set_pos(diag, 12, 232);
  lv_obj_set_size(diag, 456, 36);
  lv_obj_set_style_bg_color(diag, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(diag, 14, 0);
  lv_obj_set_style_border_width(diag, 1, 0);
  lv_obj_set_style_border_color(diag, lv_color_hex(diagnostics_overlay_enabled ? 0x0A84FF : 0xD1D1D6), 0);
  lv_obj_set_style_pad_hor(diag, 12, 0);
  lv_obj_clear_flag(diag, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *diag_cap = lv_label_create(diag);
  lv_obj_set_width(diag_cap, 330);
  lv_obj_set_style_text_font(diag_cap, en ? (const lv_font_t *)&lv_font_montserrat_14
                                            : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(diag_cap, lv_color_hex(0x1C1C1E), 0);
  lv_obj_set_style_text_align(diag_cap, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(diag_cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(diag_cap, en ? "Show live hardware status" : "نمایش اطلاعات زنده سخت افزار");
  lv_obj_align(diag_cap, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_t *diag_sw = lv_switch_create(diag);
  lv_obj_set_size(diag_sw, 52, 30);
  if (diagnostics_overlay_enabled) lv_obj_add_state(diag_sw, LV_STATE_CHECKED);
  lv_obj_add_event_cb(diag_sw, f5_diagnostics_overlay_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_align(diag_sw, en ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);
}

static void f5_cpu_temp_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  f5_cpu_temp_open();
}

static void f5_rtc_add_p(lv_obj_t *card, const char *fa, const char *en_t) {
  const bool en = (GUI.system_language == 2);
  lv_obj_t *lab = lv_label_create(card);
  lv_obj_set_width(lab, 416);
  lv_obj_set_style_text_font(lab, en ? (const lv_font_t *)&lv_font_montserrat_14
                                     : (const lv_font_t *)&ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lab, lv_color_hex(0x1C1C1E), 0);
  lv_obj_set_style_text_align(lab, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_base_dir(lab, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
  lv_label_set_text(lab, en ? en_t : fa);
}

static void f5_rtc_temp_open() {
  lv_obj_t *scroll = f5_ov_make_shell("\u062f\u0645\u0627\u06cc \u062f\u0627\u062e\u0644\u06cc \u062a\u0627\u0628\u0644\u0648",
                                      "Cabinet temperature");
  f5_ov_kind = F5_OV_KIND_RTC;
  ui_resume_checkpoint();
  lv_obj_t *card = lv_obj_create(scroll);
  lv_obj_set_width(card, 448);
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 16, 0);
  lv_obj_set_style_pad_row(card, 10, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  f5_rtc_add_p(card,
    "\u062f\u0645\u0627\u06cc \u0645\u0646\u0627\u0633\u0628 \u062f\u0627\u062e\u0644 \u062a\u0627\u0628\u0644\u0648 \u0645\u0639\u0645\u0648\u0644\u0627 \u0628\u06cc\u0646 \u0628\u06cc\u0633\u062a \u0648 \u067e\u0646\u062c \u062a\u0627 \u0686\u0647\u0644 \u062f\u0631\u062c\u0647 \u0627\u0633\u062a\u002e",
    "Healthy cabinet temp is usually 25 to 40 C.");
  f5_rtc_add_p(card,
    "\u0628\u0627\u0644\u0627\u06cc \u0686\u0647\u0644 \u0648 \u067e\u0646\u062c \u062f\u0631\u062c\u0647 \u0647\u0634\u062f\u0627\u0631 \u0632\u0631\u062f \u0627\u0633\u062a \u0648 \u062a\u0647\u0648\u06cc\u0647 \u0631\u0627 \u0686\u06a9 \u06a9\u0646\u06cc\u062f\u002e",
    "Above 45 C shows yellow warning. Check ventilation.");
  f5_rtc_add_p(card,
    "\u0628\u0627\u0644\u0627\u06cc \u0634\u0635\u062a \u0648 \u067e\u0646\u062c \u062f\u0631\u062c\u0647 \u062e\u0637\u0631\u0646\u0627\u06a9 \u0627\u0633\u062a \u0648 \u0642\u0631\u0645\u0632 \u0646\u0634\u0627\u0646 \u062f\u0627\u062f\u0647 \u0645\u06cc \u0634\u0648\u062f\u002e",
    "Above 65 C is critical and shown in red.");
  f5_rtc_add_p(card,
    "\u0627\u06af\u0631 \u062f\u0645\u0627 \u067e\u06cc\u0648\u0633\u0627\u0646\u062a \u0628\u0627\u0644\u0627 \u0645\u0627\u0646\u062f \u0641\u0646 \u06cc\u0627 \u0645\u0633\u06cc\u0631 \u0647\u0648\u0627\u06cc \u062a\u0627\u0628\u0644\u0648 \u0631\u0627 \u0628\u0631\u0631\u0633\u06cc \u06a9\u0646\u06cc\u062f\u002e",
    "If temp stays high, check fan and airflow path.");
}

static void f5_rtc_temp_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  f5_rtc_temp_open();
}

static void f5_cuts_add_row(lv_obj_t *scroll, const char *fa, const char *en_t,
                            uint32_t n, uint32_t color, bool primary = false) {
  const bool en = (GUI.system_language == 2);
  lv_obj_t *card = lv_obj_create(scroll);
  lv_obj_set_width(card, 448);
  lv_obj_set_height(card, primary ? 58 : 42);
  lv_obj_set_style_bg_color(card, lv_color_hex(primary ? 0xE4EEFB : 0xFFFFFF), 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_border_width(card, primary ? 2 : 0, 0);
  if (primary) lv_obj_set_style_border_color(card, lv_color_hex(0x0A84FF), 0);
  lv_obj_set_style_pad_hor(card, 12, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *t = lv_label_create(card);
  lv_obj_set_style_text_font(t, primary ? (en ? (const lv_font_t *)&lv_font_montserrat_18
                                               : (const lv_font_t *)&ui_font_naz20)
                                       : (en ? (const lv_font_t *)&lv_font_montserrat_14
                                             : (const lv_font_t *)&ui_font_naz18b), 0);
  lv_label_set_text(t, en ? en_t : fa);
  lv_obj_align(t, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_t *v = lv_label_create(card);
  lv_obj_set_style_text_font(v, primary ? &lv_font_montserrat_24 : &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(v, lv_color_hex(color), 0);
  lv_label_set_text_fmt(v, "%lu", (unsigned long)n);
  lv_obj_align(v, en ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);
}

static void f5_cuts_report_open();

static void f5_blade_service_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_COMPLETE);
  int y = 2026, m = 1, d = 1, hh = 0, mi = 0, ss = 0;
  if (soft_clock_valid && soft_now_epoch() > 0) epoch_to_civil(soft_now_epoch(), &y, &m, &d, &hh, &mi, &ss);
  else ds3231_read_time(&y, &m, &d, &hh, &mi, &ss);
  int jy = 0, jm = 0, jd = 0;
  gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
  char dt[24];
  snprintf(dt, sizeof(dt), "%04d/%02d/%02d", jy, jm, jd);
  blade_service_reset(dt);
  ui_toast("سرویس تیغه با موفقیت ثبت شد", "Blade service recorded", 1500);
  f5_cuts_report_open();
}

static void f5_blade_limit_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  uint32_t next = 5000;
  if (blade_limit == 2500) next = 5000;
  else if (blade_limit == 5000) next = 10000;
  else if (blade_limit == 10000) next = 25000;
  else next = 2500;
  blade_set_limit(next);
  f5_cuts_report_open();
}

static void f5_blade_rem_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  blade_set_reminder(!blade_reminder_enabled);
  f5_cuts_report_open();
}

static void f5_cuts_report_open() {
  cut_stats_ensure_day();
  const bool en = (GUI.system_language == 2);
  lv_obj_t *scroll = f5_ov_make_shell_ex("سلامت تیغه و آمار کارکرد",
                                         "Blade Health & Operation Stats", true, -10);
  f5_ov_kind = F5_OV_KIND_CUTS;
  ui_resume_checkpoint();

  // ---------- ۱. کارت سلامت و طول عمر تیغه ----------
  lv_obj_t *bcard = lv_obj_create(scroll);
  lv_obj_set_width(bcard, 448);
  lv_obj_set_height(bcard, 94);
  uint32_t hp = blade_health_pct();
  uint32_t col = (hp > 25) ? 0x34C759 : ((hp > 10) ? 0xFF9500 : 0xFF3B30);
  lv_obj_set_style_bg_color(bcard, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(bcard, 12, 0);
  lv_obj_set_style_border_width(bcard, 2, 0);
  lv_obj_set_style_border_color(bcard, lv_color_hex(col), 0);
  lv_obj_set_style_pad_all(bcard, 8, 0);
  lv_obj_clear_flag(bcard, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *btit = lv_label_create(bcard);
  lv_obj_set_style_text_font(btit, en ? &lv_font_montserrat_16 : &ui_font_naz20, 0);
  lv_obj_set_style_text_color(btit, lv_color_hex(0x1C1C1E), 0);
  lv_label_set_text(btit, en ? "Active Blade Life" : "طول عمر و سلامت تیغه");
  lv_obj_align(btit, en ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t *bpct = lv_label_create(bcard);
  lv_obj_set_style_text_font(bpct, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(bpct, lv_color_hex(col), 0);
  lv_label_set_text_fmt(bpct, "%lu%%", (unsigned long)hp);
  lv_obj_align(bpct, en ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *pbar = lv_bar_create(bcard);
  lv_obj_set_size(pbar, 428, 10);
  lv_obj_align(pbar, LV_ALIGN_CENTER, 0, 4);
  lv_bar_set_range(pbar, 0, 100);
  lv_bar_set_value(pbar, (int32_t)hp, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(pbar, lv_color_hex(0xE5E5EA), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(pbar, lv_color_hex(col), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(pbar, 5, 0);

  lv_obj_t *bstat = lv_label_create(bcard);
  lv_obj_set_style_text_font(bstat, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(bstat, lv_color_hex(0x6E6E73), 0);
  char bstat_buf[64];
  if (en) snprintf(bstat_buf, sizeof(bstat_buf), "Cuts: %lu / %lu  (Last: %s)", (unsigned long)blade_cuts, (unsigned long)blade_limit, blade_service_date[0] ? blade_service_date : "None");
  else snprintf(bstat_buf, sizeof(bstat_buf), "تعداد برش: %lu از %lu  (آخرین سرویس: %s)", (unsigned long)blade_cuts, (unsigned long)blade_limit, blade_service_date[0] ? blade_service_date : "ثبت نشده");
  lv_label_set_text(bstat, bstat_buf);
  lv_obj_align(bstat, en ? LV_ALIGN_BOTTOM_LEFT : LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  // ---------- ۲. ردیف دکمه‌های کنترل و سرویس ----------
  lv_obj_t *btn_row = lv_obj_create(scroll);
  lv_obj_set_size(btn_row, 448, 48);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

  // دکمه ثبت تعویض تیغه
  lv_obj_t *b_reset = lv_button_create(btn_row);
  lv_obj_set_size(b_reset, 170, 42);
  lv_obj_align(b_reset, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(b_reset, lv_color_hex(0x34C759), 0);
  lv_obj_set_style_radius(b_reset, 10, 0);
  lv_obj_add_event_cb(b_reset, f5_blade_service_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lbl_br = lv_label_create(b_reset);
  lv_obj_set_style_text_font(lbl_br, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lbl_br, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(lbl_br, en ? "Reset Blade" : "ثبت تعویض تیغه");
  lv_obj_center(lbl_br);

  // دکمه تنظیم سقف مجاز برش
  lv_obj_t *b_lim = lv_button_create(btn_row);
  lv_obj_set_size(b_lim, 134, 42);
  lv_obj_align(b_lim, LV_ALIGN_CENTER, en ? 20 : -20, 0);
  lv_obj_set_style_bg_color(b_lim, lv_color_hex(0x0A84FF), 0);
  lv_obj_set_style_radius(b_lim, 10, 0);
  lv_obj_add_event_cb(b_lim, f5_blade_limit_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lbl_bl = lv_label_create(b_lim);
  lv_obj_set_style_text_font(lbl_bl, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lbl_bl, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text_fmt(lbl_bl, en ? "Limit: %lu" : "سقف: %lu", (unsigned long)blade_limit);
  lv_obj_center(lbl_bl);

  // دکمه یادآور هوشمند
  lv_obj_t *b_rem = lv_button_create(btn_row);
  lv_obj_set_size(b_rem, 130, 42);
  lv_obj_align(b_rem, en ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_bg_color(b_rem, lv_color_hex(blade_reminder_enabled ? 0x5856D6 : 0x8E8E93), 0);
  lv_obj_set_style_radius(b_rem, 10, 0);
  lv_obj_add_event_cb(b_rem, f5_blade_rem_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lbl_bm = lv_label_create(b_rem);
  lv_obj_set_style_text_font(lbl_bm, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lbl_bm, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(lbl_bm, blade_reminder_enabled ? (en ? "Alert: ON" : "یادآور: فعال") : (en ? "Alert: OFF" : "یادآور: خاموش"));
  lv_obj_center(lbl_bm);

  // ---------- ۳. سطرهای آمار کل و روزانه ----------
  f5_cuts_add_row(scroll, "کارکرد کل دستگاه (همیشگی)", "Lifetime Total Cuts", total_cuts, 0x0A84FF, true);
  f5_cuts_add_row(scroll, "تعداد برش‌های امروز", "Today Total Cuts", cuts_day_sum(), 0x34C759);
  f5_cuts_add_row(scroll, "برش‌های دستی", "Lifetime Manual Cuts", cuts_life_manual, 0x1C1C1E);
  f5_cuts_add_row(scroll, "برش‌های لیست برش", "Lifetime List Cuts", cuts_life_list, 0xFF9500);
  f5_cuts_add_row(scroll, "برش‌های بهینه‌ساز", "Lifetime Optimize Cuts", cuts_life_opt, 0xAF52DE);
}

static void f5_cuts_report_open_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  GUI.buzzer_cmd_short = true;
  f5_cuts_report_open();
}

int ui_resume_query_f5_ov_kind() {
  return (f5_ov_root && lv_obj_is_valid(f5_ov_root)) ? (int)f5_ov_kind : 0;
}

void ui_resume_restore_f5_ov(int kind) {
  if (kind == F5_OV_KIND_WARR) {
    if (!warranty_active) return;
    const bool en = (GUI.system_language == 2);
    lv_obj_t *scroll = f5_ov_make_shell_ex("\u067e\u0648\u0634\u0634 \u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u0645\u062d\u0635\u0648\u0644",
                                           "Product coverage", false, -30);
    f5_ov_kind = F5_OV_KIND_WARR;
    lv_obj_set_style_pad_row(scroll, 6, 0);
    lv_obj_t *intro = lv_label_create(scroll);
    lv_obj_set_width(intro, 448);
    lv_obj_set_style_text_font(intro, en ? (const lv_font_t *)&lv_font_montserrat_14
                                         : (const lv_font_t *)&ui_font_naz18b, 0);
    lv_obj_set_style_text_color(intro, lv_color_hex(0x3A3A3C), 0);
    lv_obj_set_style_text_align(intro, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_base_dir(intro, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
    lv_label_set_long_mode(intro, LV_LABEL_LONG_WRAP);
    lv_label_set_text(intro, en
      ? "Days left from warranty start until each coverage end."
      : "\u0627\u0632 \u062a\u0627\u0631\u06cc\u062e \u0634\u0631\u0648\u0639 \u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u062a\u0627 \u067e\u0627\u06cc\u0627\u0646 \u0647\u0631 \u067e\u0648\u0634\u0634\u060c \u0631\u0648\u0632 \u0645\u0627\u0646\u062f\u0647 \u0646\u0634\u0627\u0646 \u062f\u0627\u062f\u0647 \u0645\u06cc \u0634\u0648\u062f\u002e");
    f5_warr_add_row(scroll, "\u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u0645\u062d\u0635\u0648\u0644 \u06cc\u06a9 \u0633\u0627\u0644\u0647",
                    "1-year product warranty", 1, 0x0A84FF);
    f5_warr_add_row(scroll, "\u06af\u0627\u0631\u0627\u0646\u062a\u06cc \u0645\u062d\u0635\u0648\u0644 \u062f\u0648 \u0633\u0627\u0644\u0647",
                    "2-year product warranty", 2, 0x34C759);
    f5_warr_add_row(scroll, "\u062e\u062f\u0645\u0627\u062a \u067e\u0633 \u0627\u0632 \u0641\u0631\u0648\u0634 \u062f\u0647 \u0633\u0627\u0644\u0647",
                    "10-year after-sales service", 10, 0xFF9500);
  } else if (kind == F5_OV_KIND_CUTS) {
    cut_stats_ensure_day();
    lv_obj_t *scroll = f5_ov_make_shell_ex("\u06af\u0632\u0627\u0631\u0634 \u062a\u0639\u062f\u0627\u062f \u0628\u0631\u0634",
                                           "Cut count report", false, 0);
    f5_ov_kind = F5_OV_KIND_CUTS;
    f5_cuts_add_row(scroll, "\u062a\u0639\u062f\u0627\u062f \u06a9\u0644 \u0628\u0631\u0634", "Lifetime total", total_cuts, 0x0A84FF, true);
    f5_cuts_add_row(scroll, "\u062a\u0639\u062f\u0627\u062f \u06a9\u0644 \u0627\u0645\u0631\u0648\u0632", "Today total", cuts_day_sum(), 0x34C759);
    f5_cuts_add_row(scroll, "\u062a\u0639\u062f\u0627\u062f \u06a9\u0644 \u062f\u0633\u062a\u06cc", "Lifetime manual", cuts_life_manual, 0x1C1C1E);
    f5_cuts_add_row(scroll, "\u062a\u0639\u062f\u0627\u062f \u06a9\u0644 \u0644\u06cc\u0633\u062a \u0628\u0631\u0634", "Lifetime cut list", cuts_life_list, 0xFF9500);
    f5_cuts_add_row(scroll, "\u062a\u0639\u062f\u0627\u062f \u06a9\u0644 \u0628\u0647\u06cc\u0646\u0647", "Lifetime optimize", cuts_life_opt, 0xAF52DE);
  } else if (kind == F5_OV_KIND_CPU) {
    f5_cpu_temp_open();
  } else if (kind == F5_OV_KIND_HISTORY) {
    f5_history_open();
  } else if (kind == F5_OV_KIND_RTC) {
    f5_rtc_temp_open();
  } else if (kind == F5_OV_KIND_RELEASE) {
    ui_resume_restore_release_notes();
  } else if (kind == F5_OV_KIND_MAKER) {
    f5_maker_open();
  } else if (kind == F5_OV_KIND_CLOCK) {
    f5_clock_open();
  }
}
