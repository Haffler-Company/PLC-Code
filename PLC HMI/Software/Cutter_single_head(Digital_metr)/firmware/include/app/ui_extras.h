// ui_extras: توست، نامپد، صفحه‌بندی لیست 50تایی، سوایپ، موتور و پنل بهینه‌سازی برش
// همهٔ ویجت‌ها runtime ساخته می‌شوند (بدون دست‌زدن به EEZ screens.c)
// قواعد: همهٔ ساخت/تغییر LVGL فقط از کانتکست Task2 یا هندلرهای رویداد (همان تِرد)
#pragma once
void f1_sync_wifi_chip();
void list_apply_visible_values_now();
void opt_refresh_labels();
bool opt_load_current_to_main(bool move_machine = false, bool sync_badge = true);
#define UI_EXTRAS_FULL_SETTINGS_MODULE 1
#include "images.h"
#include "admin_svc.h"
#include <SPIFFS.h>
#include <esp_heap_caps.h>

void device_lock_ui_hold_for_splash(bool hold);
static inline bool update_flow_network_busy(void);

//==================== توست (پیام موقت دوزبانه روی هر صفحه) ====================
static lv_obj_t *toast_cont = NULL;
static lv_obj_t *toast_catcher = NULL;  // تمام‌صفحه: تاچ زیر پیام نرود + لمس بیرون = بستن

bool is_toast_object(lv_obj_t *obj) {
  if (!obj) return false;
  if (toast_cont && (obj == toast_cont || lv_obj_get_parent(obj) == toast_cont)) return true;
  if (toast_catcher && obj == toast_catcher) return true;
  return false;
}
static lv_obj_t *toast_lbl_fa = NULL;
static lv_obj_t *toast_lbl_en = NULL;
static lv_obj_t *toast_bar = NULL;
static uint32_t toast_hide_at = 0;
static uint32_t toast_duration_ms = 0;
static uint32_t toast_start_ms = 0;
static uint32_t toast_paused_elapsed = 0;
static bool toast_paused = false;
static bool toast_holdable = false;
static bool toast_modal_active = false;   // راهنمای نگه‌داشتن: قفل سخت‌افزار تا بسته شود
static bool toast_dismiss_armed = false;  // لمس بیرون: اول ببلع، بعد از رها کامل ببند
static int toast_base_w = 420;
static int toast_base_h = 86;

// [UI/Task2] Physical START pressed outside Main: inform only; never command an output.
static lv_obj_t *cut_start_nav_overlay = NULL;
static lv_obj_t *cut_start_nav_card = NULL;
static lv_obj_t *cut_start_nav_title = NULL;
static lv_obj_t *cut_start_nav_body = NULL;
static lv_obj_t *cut_start_nav_stay = NULL;
static lv_obj_t *cut_start_nav_go = NULL;
static lv_obj_t *cut_start_nav_go_lbl = NULL;
static bool cut_start_nav_owns_lock = false;
static bool cut_start_nav_preexisting_lock = false;

bool cut_start_wrong_screen_modal_active() {
  return cut_start_ui_modal_active;
}

static void cut_start_nav_close() {
  if (cut_start_nav_overlay) lv_obj_add_flag(cut_start_nav_overlay, LV_OBJ_FLAG_HIDDEN);
  cut_start_ui_modal_active = false;
  if (cut_start_nav_owns_lock) {
    const bool safety_lock = IOS.stop_button_state || IOS.motor_fault_state ||
                             sensor_alert_motion_lock || device_locked ||
                             STP.motorIs_moving || STP.flag_is_cutting;
    if (!safety_lock) GUI.lock_screen = false;
  }
  cut_start_nav_owns_lock = false;
  cut_start_nav_preexisting_lock = false;
}

static void cut_start_nav_stay_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  cut_start_nav_close();
}

static void cut_start_nav_go_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (!cut_start_inputs_released) return;
  GUI.buzzer_cmd_short = true;
  cut_start_nav_close();
  request_goto_screen(1);
}

static void cut_start_nav_build() {
  if (cut_start_nav_overlay) return;
  cut_start_nav_overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(cut_start_nav_overlay, 0, 0);
  lv_obj_set_size(cut_start_nav_overlay, 480, 320);
  lv_obj_set_style_bg_color(cut_start_nav_overlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(cut_start_nav_overlay, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(cut_start_nav_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(cut_start_nav_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(cut_start_nav_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(cut_start_nav_overlay, LV_OBJ_FLAG_CLICKABLE);

  cut_start_nav_card = lv_obj_create(cut_start_nav_overlay);
  lv_obj_set_size(cut_start_nav_card, 448, 230);
  lv_obj_align(cut_start_nav_card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(cut_start_nav_card, lv_color_hex(0x1C1C1E), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(cut_start_nav_card, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(cut_start_nav_card, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(cut_start_nav_card, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(cut_start_nav_card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(cut_start_nav_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(cut_start_nav_card, LV_OBJ_FLAG_SCROLLABLE);

  cut_start_nav_title = lv_label_create(cut_start_nav_card);
  lv_obj_set_width(cut_start_nav_title, 416);
  lv_obj_set_style_text_color(cut_start_nav_title, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(cut_start_nav_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(cut_start_nav_title, LV_ALIGN_TOP_MID, 0, 2);

  cut_start_nav_body = lv_label_create(cut_start_nav_card);
  lv_obj_set_width(cut_start_nav_body, 410);
  lv_label_set_long_mode(cut_start_nav_body, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(cut_start_nav_body, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(cut_start_nav_body, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(cut_start_nav_body, LV_ALIGN_TOP_MID, 0, 48);

  cut_start_nav_stay = lv_button_create(cut_start_nav_card);
  lv_obj_set_size(cut_start_nav_stay, 180, 52);
  lv_obj_align(cut_start_nav_stay, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
  lv_obj_set_style_bg_color(cut_start_nav_stay, lv_color_hex(0x3A3A3C), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(cut_start_nav_stay, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(cut_start_nav_stay, cut_start_nav_stay_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *stay_lbl = lv_label_create(cut_start_nav_stay);
  lv_obj_center(stay_lbl);

  cut_start_nav_go = lv_button_create(cut_start_nav_card);
  lv_obj_set_size(cut_start_nav_go, 180, 52);
  lv_obj_align(cut_start_nav_go, LV_ALIGN_BOTTOM_LEFT, 4, -4);
  lv_obj_set_style_bg_color(cut_start_nav_go, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(cut_start_nav_go, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(cut_start_nav_go, cut_start_nav_go_cb, LV_EVENT_RELEASED, NULL);
  cut_start_nav_go_lbl = lv_label_create(cut_start_nav_go);
  lv_obj_center(cut_start_nav_go_lbl);

  lv_obj_add_flag(cut_start_nav_overlay, LV_OBJ_FLAG_HIDDEN);
}

void cut_start_wrong_screen_ui_tick() {
  const bool show_req = (cut_start_ui_notice_req != 0);
  if (show_req) {
    cut_start_ui_notice_req = 0;
    cut_start_nav_build();
    const bool already_visible = cut_start_wrong_screen_modal_active();
    if (!already_visible) {
      cut_start_nav_preexisting_lock = GUI.lock_screen;
      cut_start_nav_owns_lock = !GUI.lock_screen;
      if (cut_start_nav_owns_lock) GUI.lock_screen = true;
    }
    const bool fa = (GUI.system_language == 1);
    lv_label_set_text(cut_start_nav_title, fa ? "برش فقط در صفحه اصلی" : "Cut only on Main");
    lv_obj_set_style_text_font(cut_start_nav_title,
                               fa ? &ui_font_naz24 : &lv_font_montserrat_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(cut_start_nav_title, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(cut_start_nav_body,
                      fa ? "برش در این صفحه مجاز نیست. هر دو کلید استارت را رها کنید؛ سپس در صورت تمایل به صفحه اصلی بروید."
                         : "Cutting is not allowed on this page. Release both START keys, then go to Main if you wish.");
    lv_obj_set_style_text_font(cut_start_nav_body,
                               fa ? &ui_font_naz18b : &lv_font_montserrat_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(cut_start_nav_body, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *stay_lbl = lv_obj_get_child(cut_start_nav_stay, 0);
    if (stay_lbl) {
      lv_label_set_text(stay_lbl, fa ? "ماندن" : "Stay");
      lv_obj_set_style_text_font(stay_lbl, fa ? &ui_font_naz24 : &lv_font_montserrat_16,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_label_set_text(cut_start_nav_go_lbl, fa ? "رفتن به صفحه اصلی" : "Go to Main");
    lv_obj_set_style_text_font(cut_start_nav_go_lbl,
                               fa ? &ui_font_naz18b : &lv_font_montserrat_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    cut_start_ui_modal_active = true;
    lv_obj_clear_flag(cut_start_nav_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(cut_start_nav_overlay);
  }

  if (!cut_start_nav_overlay || lv_obj_has_flag(cut_start_nav_overlay, LV_OBJ_FLAG_HIDDEN)) return;
  if (cut_start_inputs_released && !cut_start_nav_preexisting_lock) {
    lv_obj_clear_state(cut_start_nav_go, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(cut_start_nav_go, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cut_start_nav_go_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_obj_add_state(cut_start_nav_go, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(cut_start_nav_go, lv_color_hex(0x48484A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cut_start_nav_go_lbl, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void toast_set_held_style(bool held);

static void toast_hide_now() {
  const bool suppress = toast_modal_active || toast_dismiss_armed;
  toast_paused = false;
  toast_holdable = false;
  toast_paused_elapsed = 0;
  toast_dismiss_armed = false;
  if (toast_modal_active) {
    toast_modal_active = false;
    GUI.lock_screen = false;
  }
  if (suppress) ui_help_suppress_release = true;
  if (toast_cont) {
    toast_set_held_style(false);
    lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
  }
  if (toast_catcher) lv_obj_add_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN);
}

static void toast_set_held_style(bool held) {
  if (!toast_cont) return;
  // عمداً اندازه عوض نشود — بزرگ‌شدن باعث PRESS_LOST و جدا شدن انگشت از ویجت می‌شود
  if (held) {
    lv_obj_set_style_border_width(toast_cont, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(toast_cont, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(toast_cont, lv_color_hex(0x455A64), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(toast_cont, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(toast_cont, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(toast_cont, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_obj_set_style_border_width(toast_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(toast_cont, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(toast_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void toast_catcher_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (toast_paused) return;  // در حال نگهداشتن خود پیام — catcher نباید ببندد
  // PRESSED: فقط پیام را پنهان کن؛ catcher بماند تا رها شدن — تاچ زیرین اجرا نشود
  if (code == LV_EVENT_PRESSED) {
    toast_dismiss_armed = true;
    toast_holdable = false;
    if (toast_cont) {
      toast_set_held_style(false);
      lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_CLICKED ||
      code == LV_EVENT_PRESS_LOST || code == LV_EVENT_LONG_PRESSED) {
    if (toast_dismiss_armed ||
        (toast_catcher && !lv_obj_has_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN))) {
      toast_hide_now();
    }
  }
}

static void toast_hold_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (!toast_cont || lv_obj_has_flag(toast_cont, LV_OBJ_FLAG_HIDDEN)) return;
  if (code == LV_EVENT_PRESSED) {
    if (!toast_paused) {
      toast_paused = true;
      toast_paused_elapsed = lv_tick_elaps(toast_start_ms);
      toast_set_held_style(true);
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (toast_paused) {
      toast_paused = false;
      toast_start_ms = lv_tick_get() - toast_paused_elapsed;
      toast_hide_at = toast_start_ms + toast_duration_ms;
      toast_set_held_style(false);
    }
  }
}

static void toast_build() {
  if (toast_cont) return;
  toast_catcher = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(toast_catcher, 0, 0);
  lv_obj_set_size(toast_catcher, 480, 320);
  lv_obj_set_style_bg_opa(toast_catcher, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(toast_catcher, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(toast_catcher, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(toast_catcher, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(toast_catcher, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(toast_catcher, toast_catcher_event_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(toast_catcher, toast_catcher_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(toast_catcher, toast_catcher_event_cb, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_add_event_cb(toast_catcher, toast_catcher_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(toast_catcher, toast_catcher_event_cb, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN);

  toast_cont = lv_obj_create(lv_layer_top());
  lv_obj_set_size(toast_cont, 440, 92);
  lv_obj_align(toast_cont, LV_ALIGN_BOTTOM_MID, 0, -48);
  lv_obj_set_style_bg_color(toast_cont, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(toast_cont, LV_OPA_90, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(toast_cont, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(toast_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(toast_cont, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(toast_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_PRESS_LOCK);  // انگشت روی پیام بماند حتی اگر کمی بلغزد
  lv_obj_add_event_cb(toast_cont, toast_hold_event_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(toast_cont, toast_hold_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(toast_cont, toast_hold_event_cb, LV_EVENT_PRESS_LOST, NULL);
  toast_lbl_fa = lv_label_create(toast_cont);
  lv_obj_set_width(toast_lbl_fa, 416);
  lv_label_set_long_mode(toast_lbl_fa, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(toast_lbl_fa, &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(toast_lbl_fa, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(toast_lbl_fa, LV_ALIGN_TOP_MID, 0, 6);
  toast_lbl_en = lv_label_create(toast_cont);
  lv_obj_set_style_text_font(toast_lbl_en, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(toast_lbl_en, lv_color_hex(0xb0bec5), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(toast_lbl_en, LV_ALIGN_BOTTOM_MID, 0, 6);
  lv_obj_add_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  toast_bar = lv_bar_create(toast_cont);
  lv_obj_set_size(toast_bar, 416, 6);
  lv_obj_align(toast_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_bar_set_range(toast_bar, 0, 100);
  lv_bar_set_value(toast_bar, 100, LV_ANIM_OFF);
  lv_obj_set_style_radius(toast_bar, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(toast_bar, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(toast_bar, lv_color_hex(0x455a64), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(toast_bar, lv_color_hex(0x34C759), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_flag(toast_bar, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
}

static void ui_toast_show(const char *fa, const char *en, uint32_t ms, bool holdable, bool beep = true) {
  (void)holdable;  // همهٔ توست‌ها قابل نگه‌داشتن‌اند
  toast_build();
  const bool english = (GUI.system_language == 2);
  toast_base_w = 440;
  toast_base_h = 92;
  lv_obj_set_size(toast_cont, toast_base_w, toast_base_h);
  lv_obj_set_width(toast_lbl_fa, toast_base_w - 24);
  if (toast_bar) lv_obj_set_size(toast_bar, toast_base_w - 24, 6);
  lv_label_set_text(toast_lbl_fa, english ? en : fa);
  lv_obj_set_style_text_font(toast_lbl_fa, english ? &lv_font_montserrat_16 : &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(toast_lbl_fa, english ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(toast_lbl_fa, LV_ALIGN_TOP_MID, 0, 6);
  lv_obj_add_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  toast_holdable = true;
  toast_paused = false;
  toast_paused_elapsed = 0;
  toast_duration_ms = ms;
  toast_start_ms = lv_tick_get();
  toast_hide_at = toast_start_ms + ms;
  toast_set_held_style(false);
  if (toast_bar) {
    lv_bar_set_value(toast_bar, 100, LV_ANIM_OFF);
    lv_obj_clear_flag(toast_bar, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_CLICKABLE);
  if (toast_catcher) {
    lv_obj_clear_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast_catcher);
  }
  lv_obj_clear_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(toast_cont);
  if (beep) GUI.buzzer_cmd_long = true;
}

void ui_toast(const char *fa, const char *en, uint32_t ms) {
  ui_toast_show(fa, en, ms, false);
}

static void ui_toast_silent(const char *fa, const char *en, uint32_t ms) {
  ui_toast_show(fa, en, ms, false, false);
}

// WebPortalTask owns the radio; Task2 only renders this already-published state.
bool web_portal_is_sta(void);
void web_portal_sta_ip(char *out, size_t out_sz);
void ui_toast_wifi_ready(const char *ssid, const char *password) {
  ui_toast_show("وای فای روشن شد", "WiFi portal is ready", 6500, false);
  // جعبهٔ بلندتر: عنوان و دو خط SSID/IP بدون هم‌پوشانی
  toast_base_h = 112;
  lv_obj_set_size(toast_cont, toast_base_w, toast_base_h);
  char credentials[96];
  if (web_portal_is_sta()) {
    char ip[20]; web_portal_sta_ip(ip, sizeof(ip));
    snprintf(credentials, sizeof(credentials), "SSID: %s  IP: %s\nhttp://haffler.local",
             ssid ? ssid : "", ip);
  } else {
    snprintf(credentials, sizeof(credentials), "SSID: %s   PASS: %s",
             ssid ? ssid : "", password ? password : "");
  }
  lv_obj_align(toast_lbl_fa, LV_ALIGN_TOP_MID, 0, 4);
  lv_label_set_text(toast_lbl_en, credentials);
  lv_obj_set_style_text_font(toast_lbl_en, &lv_font_montserrat_14,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(toast_lbl_en, lv_color_hex(0xffffff),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(toast_lbl_en, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(toast_lbl_en, LV_BASE_DIR_LTR,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(toast_lbl_en, LV_ALIGN_BOTTOM_MID, 0, -17);
  lv_obj_clear_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
}

// ذخیرهٔ F1..F5: حرف F فقط با Montserrat (naz گلیف لاتین ندارد → مربع)
void ui_toast_fmem_saved(int fk) {
  if (fk < 1 || fk > 5) fk = 1;
  toast_build();
  const bool english = (GUI.system_language == 2);
  char ftag[8];
  snprintf(ftag, sizeof(ftag), "F%d", fk);
  if (english) {
    char en[40];
    snprintf(en, sizeof(en), "Target saved to %s", ftag);
    lv_label_set_text(toast_lbl_fa, en);
    lv_obj_set_style_text_font(toast_lbl_fa, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(toast_lbl_fa, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(toast_lbl_fa, LV_ALIGN_CENTER, 0, -2);
    lv_obj_add_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  } else {
    // فارسی بدون حرف لاتین + برچسب F با Montserrat سفید (بدون مربع)
    lv_label_set_text(toast_lbl_fa, "این عدد مقصد ذخیره شد");
    lv_obj_set_style_text_font(toast_lbl_fa, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(toast_lbl_fa, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(toast_lbl_fa, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(toast_lbl_en, ftag);
    lv_obj_set_style_text_font(toast_lbl_en, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(toast_lbl_en, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(toast_lbl_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(toast_lbl_en, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_clear_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  }
  toast_holdable = false;
  toast_paused = false;
  toast_paused_elapsed = 0;
  toast_duration_ms = 2800;
  toast_start_ms = lv_tick_get();
  toast_hide_at = toast_start_ms + 2800;
  if (toast_bar) {
    lv_obj_set_size(toast_cont, 420, 78);
    lv_obj_add_flag(toast_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_CLICKABLE);  // لمس بیرون با catcher؛ خود پیام هم قابل لمس
  }
  if (toast_catcher) {
    lv_obj_clear_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast_catcher);
  }
  lv_obj_clear_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(toast_cont);
}

void ui_toast_cut_done(float mm) {
  // naz گلیف لاتین/خط تیره بلند ندارد → مربع؛ عدد + mm با Montserrat جدا
  toast_build();
  const bool english = (GUI.system_language == 2);
  char num[24];
  if (fabsf(mm - roundf(mm)) < 0.05f)
    snprintf(num, sizeof(num), "%d mm", (int)lroundf(mm));
  else
    snprintf(num, sizeof(num), "%.1f mm", (double)mm);
  if (english) {
    char en[48];
    snprintf(en, sizeof(en), "Cut complete  %s", num);
    lv_label_set_text(toast_lbl_fa, en);
    lv_obj_set_style_text_font(toast_lbl_fa, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(toast_lbl_fa, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(toast_lbl_fa, LV_ALIGN_CENTER, 0, -6);
    lv_obj_add_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(toast_lbl_fa, "برش کامل شد");
    lv_obj_set_style_text_font(toast_lbl_fa, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(toast_lbl_fa, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(toast_lbl_fa, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(toast_lbl_en, num);
    lv_obj_set_style_text_font(toast_lbl_en, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(toast_lbl_en, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(toast_lbl_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(toast_lbl_en, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_clear_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  }
  toast_holdable = true;
  toast_paused = false;
  toast_paused_elapsed = 0;
  toast_duration_ms = 5000;
  toast_start_ms = lv_tick_get();
  toast_hide_at = toast_start_ms + 5000;
  toast_base_w = 420;
  toast_base_h = 86;
  toast_set_held_style(false);
  if (toast_bar) {
    lv_bar_set_value(toast_bar, 100, LV_ANIM_OFF);
    lv_obj_clear_flag(toast_bar, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_CLICKABLE);
  if (toast_catcher) {
    lv_obj_clear_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast_catcher);
  }
  lv_obj_clear_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(toast_cont);
  GUI.buzzer_cmd_long = true;
}

void prcs_ui_toast() {  // در حلقهٔ Task2
  if (!toast_cont) return;
  if (lv_obj_has_flag(toast_cont, LV_OBJ_FLAG_HIDDEN)) return;

  // pause/resume فقط از toast_hold_event_cb (نه از تغییر سایز/STATE_PRESSED)
  if (toast_paused) return;

  if (toast_bar && toast_duration_ms > 0) {
    uint32_t elapsed = lv_tick_elaps(toast_start_ms);
    if (elapsed >= toast_duration_ms) {
      if (toast_bar) lv_bar_set_value(toast_bar, 0, LV_ANIM_OFF);
      toast_hide_now();
      return;
    }
    int remain = (int)(100UL - (elapsed * 100UL / toast_duration_ms));
    if (remain < 0) remain = 0;
    if (remain > 100) remain = 100;
    lv_bar_set_value(toast_bar, remain, LV_ANIM_OFF);
  } else if ((int32_t)(lv_tick_get() - toast_hide_at) >= 0) {
    toast_hide_now();
  }
}

void prcs_cut_done_toast() {
  if (!cut_done_notify_req) return;
  cut_done_notify_req = false;
  ui_toast_cut_done(cut_done_notify_mm);
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_COMPLETE);
}

//==================== مودال هشدار سنسور (لایه بالا — همیشه دیده می‌شود) ====================
static lv_obj_t *sa_catcher = NULL;
static lv_obj_t *sa_card = NULL;
static lv_obj_t *sa_lbl = NULL;
static lv_obj_t *sa_btn_x = NULL;
static lv_obj_t *sa_btn_ok = NULL;
static lv_obj_t *sa_ok_lbl = NULL;
static bool sa_visible = false;

void sensor_alert_close_now(void) {
  sa_visible = false;
  // اول قفل حرکت را بردار تا Task3 دیگر فرمان را نبلعد
  sensor_alert_motion_lock = false;
  sensor_alert_kind = 0;
  STP.mvm_interupt = false;
  STP.jogEnabled = false;
  STP.moveStepperCMD = false;
  GUI.lock_screen = false;
  // روی سنسور فشرده: اجازهٔ دور شدن بعد از تأیید (وگرنه جاگ/Enter قفل می‌ماند)
  if (IOS.ref_sensor_state) limit_escape_sensor = 1;
  else if (IOS.up_lim_sensor_state) limit_escape_sensor = 2;
  if (sa_catcher && lv_obj_is_valid(sa_catcher)) lv_obj_add_flag(sa_catcher, LV_OBJ_FLAG_HIDDEN);
  if (sa_card && lv_obj_is_valid(sa_card)) lv_obj_add_flag(sa_card, LV_OBJ_FLAG_HIDDEN);
}

static void sensor_alert_x_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  sensor_alert_close_now();
}

static void sensor_alert_build(void) {
  if (sa_card && lv_obj_is_valid(sa_card)) return;
  sa_catcher = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(sa_catcher, 0, 0);
  lv_obj_set_size(sa_catcher, 480, 320);
  lv_obj_set_style_bg_color(sa_catcher, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(sa_catcher, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(sa_catcher, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(sa_catcher, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(sa_catcher, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(sa_catcher, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(sa_catcher, LV_OBJ_FLAG_HIDDEN);

  // کارت کوتاه‌تر + متن بالاتر از دکمهٔ تأیید (متن زیر تأیید نرود)
  sa_card = lv_obj_create(lv_layer_top());
  lv_obj_set_size(sa_card, 420, 200);
  lv_obj_align(sa_card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(sa_card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(sa_card, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(sa_card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(sa_card, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(sa_card, lv_color_hex(0xFF9500), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(sa_card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(sa_card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(sa_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(sa_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(sa_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(sa_card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(sa_card, LV_OBJ_FLAG_HIDDEN);

  sa_btn_x = lv_button_create(sa_card);
  lv_obj_set_size(sa_btn_x, 40, 36);
  lv_obj_align(sa_btn_x, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(sa_btn_x, lv_color_hex(0xB90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(sa_btn_x, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(sa_btn_x, LV_OBJ_FLAG_HIDDEN);  // فقط تأیید — ضربدر روی متن نمی‌افتد
  lv_obj_t *xl = lv_label_create(sa_btn_x);
  lv_label_set_text(xl, "X");
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(xl);
  lv_obj_add_event_cb(sa_btn_x, sensor_alert_x_cb, LV_EVENT_CLICKED, NULL);

  sa_lbl = lv_label_create(sa_card);
  lv_obj_set_width(sa_lbl, 380);
  lv_obj_set_height(sa_lbl, 118);  // سقف متن — بالای دکمهٔ تأیید
  lv_label_set_long_mode(sa_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(sa_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(sa_lbl, lv_color_hex(0x1C1C1E), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(sa_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(sa_lbl, LV_ALIGN_TOP_MID, 0, 8);

  sa_btn_ok = lv_button_create(sa_card);
  lv_obj_set_size(sa_btn_ok, 120, 40);
  lv_obj_align(sa_btn_ok, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(sa_btn_ok, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(sa_btn_ok, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  sa_ok_lbl = lv_label_create(sa_btn_ok);
  lv_label_set_text(sa_ok_lbl, "OK");
  lv_obj_set_style_text_font(sa_ok_lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(sa_ok_lbl);
  lv_obj_add_event_cb(sa_btn_ok, sensor_alert_x_cb, LV_EVENT_CLICKED, NULL);
}

// kind: 1=جهت اشتباه  2=حد سنسور فیزیکی  3=حد نرم  4=سنسور گیرکرده پس از دور شدن
// which: 1=اول 2=آخر   err_mm: اختلاف (فقط kind 2)
void sensor_alert_open(int kind, int which, int err_mm) {
  sensor_alert_build();
  const bool fa = (GUI.system_language != 2);
  const bool is_min = (which == 1);
  static char buf[480];
  uint32_t border = 0xFF9500u;
  const char *fa_sens = is_min ? "اول خط" : "آخر خط";
  const char *en_sens = is_min ? "start-of-line" : "end-of-line";

  if (kind == 1) {
    border = 0xFF3B30u;
    if (fa) {
      snprintf(buf, sizeof(buf),
               "سنسور %s خورده. عدد کالیبره شد.\n"
               "جهت موتور اشتباه است یا سیم سنسور اول و آخر جابجا شده.",
               fa_sens);
    } else {
      snprintf(buf, sizeof(buf),
               "%s sensor hit. Position calibrated.\n"
               "Wrong motor direction or start/end sensors swapped.",
               en_sens);
    }
  } else if (kind == 4) {
    border = 0xFF9500u;
    if (fa) {
      snprintf(buf, sizeof(buf),
               "سنسور %s هنوز فعال است.\n"
               "از ان دور شدیم ولی ازاد نشد.\n"
               "احتمال خرابی سنسور یا حرکت نکردن موتور است.\n"
               "عیب را پیدا کنید و دوباره حرکت بدهید.",
               fa_sens);
    } else {
      snprintf(buf, sizeof(buf),
               "%s sensor is still active.\n"
               "We moved away but it did not release.\n"
               "Motor may not have moved, or the sensor is faulty.\n"
               "Find the fault and request motion again.",
               en_sens);
    }
  } else if (kind == 3) {
    border = 0xFF9500u;
    if (fa) {
      snprintf(buf, sizeof(buf),
               "به حد %s رسیدم ولی سنسور فعال نشد.\n"
               "سنسور %s و عدد آن در تنظیمات را چک کنید.",
               is_min ? "حداقل" : "حداکثر", fa_sens);
    } else {
      snprintf(buf, sizeof(buf),
               "Hit %s travel limit, but sensor did not trip.\n"
               "Check the %s sensor and its settings value.",
               is_min ? "minimum" : "maximum", en_sens);
    }
  } else if (err_mm > 8) {
    border = 0xFF3B30u;
    if (fa) {
      snprintf(buf, sizeof(buf),
               "سنسور %s خورده. عدد کالیبره شد.\n"
               "دیگر به این سمت نمی‌روم. اختلاف: %d میلی متر.\n"
               "سنسور و عدد %s در تنظیمات را چک کنید.",
               fa_sens, err_mm, fa_sens);
    } else {
      snprintf(buf, sizeof(buf),
               "%s sensor hit. Position calibrated.\n"
               "Cannot continue this way. Error: %d mm.\n"
               "Check sensor and the %s settings value.",
               en_sens, err_mm, en_sens);
    }
  } else if (err_mm > 0) {
    if (fa) {
      snprintf(buf, sizeof(buf),
               "سنسور %s خورده. عدد کالیبره شد\n"
               "(اختلاف %d میلی متر). دیگر به این سمت نمی‌روم.",
               fa_sens, err_mm);
    } else {
      snprintf(buf, sizeof(buf),
               "%s sensor hit. Calibrated (%d mm).\n"
               "Cannot continue this way.",
               en_sens, err_mm);
    }
  } else {
    if (fa) {
      snprintf(buf, sizeof(buf),
               "سنسور %s خورده. عدد کالیبره شد.\n"
               "دیگر به این سمت نمی‌روم.",
               fa_sens);
    } else {
      snprintf(buf, sizeof(buf),
               "%s sensor hit. Position calibrated.\n"
               "Cannot continue this way.",
               en_sens);
    }
  }

  // ارتفاع ثابت کوتاه: متن کوتاه‌شده بالای تأیید جا می‌شود
  lv_obj_set_size(sa_card, 420, 200);
  lv_obj_align(sa_card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_border_color(sa_card, lv_color_hex(border), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(sa_lbl, buf);
  lv_obj_align(sa_lbl, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_set_style_text_font(sa_lbl, fa ? &ui_font_naz18b : &lv_font_montserrat_14,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(sa_lbl, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  if (sa_ok_lbl) {
    // فارسی = naz ، انگلیسی = Montserrat (naz گلیف لاتین ندارد → مربع)
    lv_label_set_text(sa_ok_lbl, fa ? "تایید" : "OK");
    lv_obj_set_style_text_font(sa_ok_lbl, fa ? &ui_font_naz18b : &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(sa_ok_lbl);
  }

  sensor_alert_motion_lock = true;
  sensor_alert_kind = (uint8_t)kind;
  STP.jogEnabled = false;
  STP.moveStepperCMD = false;
  STP.mvm_interupt = true;
  GUI.lock_screen = true;
  sa_visible = true;
  lv_obj_clear_flag(sa_catcher, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(sa_card, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(sa_catcher);
  lv_obj_move_foreground(sa_card);
  GUI.buzzer_cmd_long = true;
}

bool sensor_alert_is_open(void) { return sa_visible; }

//==================== تأیید خروج از لیست/بهینه ====================
void opt_main_bar_lbl_hide();  // forward — تعریف پایین‌تر
void opt_main_bar_lbl_sync();
static void keyboardStopJog();
void opt_notify_list_edited();  // ویرایش سایز/تعداد لیست → بازچینی بهینه
bool opt_consume_home_preserve();
void jog_manual_badge_hide();
void jog_manual_badge_sync();
void jog_manual_badge_invalidate();
void blade_err_badge_sync();
void blade_err_disarm_restore_destination();
void ui_rebind_destination_help();  // بعد از recreate مقصد
void f1_help_deferred_tick();       // راهنمای F1 دوطرفه (بدون نامپد)
bool f1_help_blocks_numpad();
void splash_release_hold_cover();   // fwd — پوشش سیاه resume
void opt_finish_to_manual(bool show_toast);
static lv_obj_t *exit_q_cont = NULL;
static lv_obj_t *exit_q_lbl = NULL;
static lv_obj_t *exit_q_bar = NULL;
static lv_obj_t *exit_q_btn_cancel = NULL;
static lv_obj_t *exit_q_btn_confirm = NULL;
static uint32_t exit_q_start_ms = 0;
static uint32_t exit_q_dur_ms = 3000;
static uint32_t exit_q_paused_elapsed = 0;
static bool exit_q_paused = false;
static uint8_t exit_q_pending_action = 0;  // 1=رقم بعد از خروج صف  2=جاگ بعد از خروج صف  3=خروج از جاگ→عدد

bool main_queue_mode_active() {
  return OPT.active || AUT.selected_count_val >= 0;
}

void main_clear_destination_sp() {
  STP.Man_Sp_mm = 0;
  if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)10) == pdTRUE) {
    GUI.inputBuffer_sp = "";
    xSemaphoreGive(lable1_1_Semaphore);
  } else {
    GUI.inputBuffer_sp = "";
  }
  clear_buffer_sp_flag = true;
  GUI.update_lable1_1 = false;
  GUI.edit_mode_lable1_1 = false;
  // بعد از اتمام لیست/بهینه کادر مقصد نشان داده نشود
  if (objects.lable1_1) {
    lv_label_set_text(objects.lable1_1, "");
    lv_obj_add_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.panel1_2) lv_obj_add_flag(objects.panel1_2, LV_OBJ_FLAG_HIDDEN);
}

static String main_format_mm_tenth(float mm) {
  char b[16];
  const float rounded = roundf(mm * 10.0f) / 10.0f;
  if (fabsf(rounded - roundf(rounded)) < 0.05f)
    snprintf(b, sizeof(b), "%d", (int)lroundf(rounded));
  else
    snprintf(b, sizeof(b), "%.1f", (double)rounded);
  return String(b);
}

// بعد از اتمام صف / ورود بدون رفرنس کامل: مقصد = موقعیت فعلی، بدون گردکردن اعشار بوت.
void main_restore_destination_to_current_pos() {
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 1.0f) ppmm = 1.0f;
  float current_mm = roundf(((float)STP.currentPosition_p / ppmm) * 10.0f) / 10.0f;
  const float lim_lo = (float)STP.initial_Position_p / ppmm;
  const float lim_hi = STP.final_position_mm.toFloat();
  if (current_mm < lim_lo) current_mm = lim_lo;
  if (current_mm > lim_hi) current_mm = lim_hi;
  // فقط همسان‌سازی نمایش/مقصد در بوت؛ هیچ فرمان حرکت اینجا صادر نمی‌شود.
  STP.Man_Sp_mm = current_mm;
  STP.position_Setpoint_p = (int)lroundf(current_mm * ppmm);
  const String current_text = main_format_mm_tenth(current_mm);
  if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)10) == pdTRUE) {
    GUI.inputBuffer_sp = current_text;
    xSemaphoreGive(lable1_1_Semaphore);
  } else {
    GUI.inputBuffer_sp = current_text;
  }
  clear_buffer_sp_flag = true;
  GUI.edit_mode_lable1_1 = true;
  GUI.NumLock = false;
  GUI.update_lable1_1 = true;
  if (objects.panel1_2) lv_obj_clear_flag(objects.panel1_2, LV_OBJ_FLAG_HIDDEN);
  if (objects.lable1_1) lv_obj_clear_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN);
}

static void main_show_destination_chrome() {
  if (objects.panel1_2) lv_obj_clear_flag(objects.panel1_2, LV_OBJ_FLAG_HIDDEN);
  if (objects.lable1_1) lv_obj_clear_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN);
}

static void main_exit_queue_mode_now() {
  if (OPT.active) {
    OPT.active = false;
    OPT.waiting_move = false;
    OPT.flag_refresh = false;
  }
  opt_main_bar_lbl_hide();
  AUT.selected_count_val = -1;
  AUT.temp_list_count_Val = 0;
  AUT.in_position_flag = false;
  GUI.update_lable1_3 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  GUI.edit_mode_lable1_1 = true;
}

static void exit_q_close(bool do_exit) {
  uint8_t act = exit_q_pending_action;
  // خروج از جاگ حین حرکت: اول همان توقف نرمِ رهاکردن کلید، سپس خروج خودکار.
  if (do_exit && act == 3 && STP.motorIs_moving) {
    keyboardStopJog();
    exit_q_paused = false;
    return;
  }
  if (exit_q_cont && lv_obj_is_valid(exit_q_cont))
    lv_obj_add_flag(exit_q_cont, LV_OBJ_FLAG_HIDDEN);
  GUI.lock_screen = false;
  exit_q_paused = false;
  exit_q_pending_action = 0;
  if (!do_exit) return;
  if (act == 3) {
    // خروج از جاگ → حالت عدد
    STP.jogEnabled = false;
    keyboardJogActive = false;
    GUI.NumLock = false;
    GUI.edit_mode_lable1_1 = true;
    clear_buffer_sp_flag = true;
    usb_keyboard_set_numlock_led(true);  // حالت عدد = LED روشن
    jog_manual_badge_hide();
    ui_toast("حالت عدد فعال", "Number mode ON", 1200);
    if (GUI.virtual_kb_enable && GUI.scr_id == 1) numpad_open(1);
    ui_resume_checkpoint();
    return;
  }
  main_exit_queue_mode_now();
  if (act == 2) {
    GUI.NumLock = true;
    GUI.edit_mode_lable1_1 = false;
    usb_keyboard_set_numlock_led(false);  // جاگ = LED خاموش
    jog_manual_badge_sync();
    ui_resume_checkpoint();
    ui_toast("حالت جاگ فعال", "Jog mode ON", 1200);
  } else {
    usb_keyboard_set_numlock_led(true);
    jog_manual_badge_hide();
    // خروج از لیست/بهینه برای تایپ مقصد → نامپد تاچ را باز کن
    if (act == 1 && GUI.virtual_kb_enable && GUI.scr_id == 1) numpad_open(1);
  }
}

static void exit_q_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  exit_q_close(false);
}

static void exit_q_confirm_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  exit_q_close(true);
}

void request_exit_queue_mode_prompt(uint8_t why) {
  // why: 1=رقم کیبورد (خروج صف)  2=جاگ/NumLock (ورود جاگ از صف)  3=خروج از جاگ
  GUI.exit_queue_prompt_why = why;
  GUI.flag_exit_queue_prompt = true;
}

void prcs_exit_queue_mode_prompt() {
  if (GUI.flag_exit_queue_prompt) {
    GUI.flag_exit_queue_prompt = false;
    const uint8_t why = GUI.exit_queue_prompt_why;
    if (why == 3) {
      if (!GUI.NumLock || GUI.scr_id != 1) return;
    } else if (!main_queue_mode_active()) {
      return;
    }
    exit_q_pending_action = why;
    if (!exit_q_cont) {
      exit_q_cont = lv_obj_create(lv_layer_top());
      lv_obj_set_size(exit_q_cont, 440, 118);
      lv_obj_align(exit_q_cont, LV_ALIGN_CENTER, 0, 10);
      lv_obj_set_style_bg_color(exit_q_cont, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(exit_q_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(exit_q_cont, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(exit_q_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(exit_q_cont, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_clear_flag(exit_q_cont, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(exit_q_cont, LV_OBJ_FLAG_CLICKABLE);

      lv_obj_t *lbl = lv_label_create(exit_q_cont);
      exit_q_lbl = lbl;
      lv_obj_set_width(lbl, 410);
      lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 2);

      exit_q_btn_cancel = lv_button_create(exit_q_cont);
      lv_obj_set_size(exit_q_btn_cancel, 110, 34);
      lv_obj_align(exit_q_btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -4, -18);
      lv_obj_set_style_bg_color(exit_q_btn_cancel, lv_color_hex(0x34C759), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(exit_q_btn_cancel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_event_cb(exit_q_btn_cancel, exit_q_cancel_cb, LV_EVENT_RELEASED, NULL);
      lv_obj_t *cl = lv_label_create(exit_q_btn_cancel);
      lv_label_set_text(cl, (GUI.system_language == 1) ? "انصراف" : "Cancel");
      lv_obj_set_style_text_font(cl, (GUI.system_language == 1) ? &ui_font_naz24 : &lv_font_montserrat_16,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(cl);

      exit_q_btn_confirm = lv_button_create(exit_q_cont);
      lv_obj_set_size(exit_q_btn_confirm, 110, 34);
      lv_obj_align(exit_q_btn_confirm, LV_ALIGN_BOTTOM_LEFT, 4, -18);
      lv_obj_set_style_bg_color(exit_q_btn_confirm, lv_color_hex(0x34C759), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(exit_q_btn_confirm, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_event_cb(exit_q_btn_confirm, exit_q_confirm_cb, LV_EVENT_RELEASED, NULL);
      lv_obj_t *okl = lv_label_create(exit_q_btn_confirm);
      lv_label_set_text(okl, (GUI.system_language == 1) ? "تأیید" : "Confirm");
      lv_obj_set_style_text_font(okl, (GUI.system_language == 1) ? &ui_font_naz24 : &lv_font_montserrat_16,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(okl);

      exit_q_bar = lv_bar_create(exit_q_cont);
      lv_obj_set_size(exit_q_bar, 410, 8);
      lv_obj_align(exit_q_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
      lv_bar_set_range(exit_q_bar, 0, 1000);
      lv_bar_set_value(exit_q_bar, 1000, LV_ANIM_OFF);
      lv_obj_set_style_bg_color(exit_q_bar, lv_color_hex(0x3a3a3c), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(exit_q_bar, lv_color_hex(0xFF9500), LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(exit_q_bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(exit_q_bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    lv_obj_t *lbl = exit_q_lbl;
    const bool fa = (GUI.system_language == 1);
    const char *msg_fa;
    const char *msg_en;
    if (why == 3) {
      msg_fa = "در حالت جاگ هستید\nمی‌خواهید خارج شوید؟";
      msg_en = "Jog mode is on.\nExit to number mode?";
    } else if (OPT.active) {
      msg_fa = "از حالت بهینه برش خارج می‌شوید؟";
      msg_en = "Exit cut optimization mode?";
    } else {
      msg_fa = "از حالت لیست برش خارج می‌شوید؟";
      msg_en = "Exit cut-list mode?";
    }
    if (lbl) {
      lv_label_set_text(lbl, fa ? msg_fa : msg_en);
      lv_obj_set_style_text_font(lbl, fa ? &ui_font_naz24 : &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(lbl, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);
    }
    if (exit_q_btn_cancel) {
      lv_obj_set_style_bg_color(exit_q_btn_cancel, lv_color_hex(0x34C759), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_t *cl = lv_obj_get_child(exit_q_btn_cancel, 0);
      if (cl) {
        lv_label_set_text(cl, fa ? "انصراف" : "Cancel");
        lv_obj_set_style_text_font(cl, fa ? &ui_font_naz24 : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    if (exit_q_btn_confirm) {
      lv_obj_t *okl = lv_obj_get_child(exit_q_btn_confirm, 0);
      if (okl) {
        lv_label_set_text(okl, fa ? "تأیید" : "Confirm");
        lv_obj_set_style_text_font(okl, fa ? &ui_font_naz24 : &lv_font_montserrat_16,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    lv_bar_set_value(exit_q_bar, 1000, LV_ANIM_OFF);  // دقت بیشتر، بدون انیمیشن/تایمر اضافه
    exit_q_dur_ms = 3000;
    exit_q_start_ms = lv_tick_get();
    exit_q_paused = false;
    exit_q_paused_elapsed = 0;
    GUI.lock_screen = true;
    GUI.buzzer_cmd_long = true;
    lv_obj_clear_flag(exit_q_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(exit_q_cont);
  }

  if (!exit_q_cont || lv_obj_has_flag(exit_q_cont, LV_OBJ_FLAG_HIDDEN)) return;
  const bool pressed = lv_obj_has_state(exit_q_cont, LV_STATE_PRESSED);
  if (pressed && !exit_q_paused) {
    exit_q_paused = true;
    exit_q_paused_elapsed = lv_tick_elaps(exit_q_start_ms);
  } else if (!pressed && exit_q_paused) {
    exit_q_paused = false;
    exit_q_start_ms = lv_tick_get() - exit_q_paused_elapsed;
  }
  if (exit_q_paused) return;
  uint32_t elapsed = lv_tick_elaps(exit_q_start_ms);
  if (elapsed >= exit_q_dur_ms) {
    lv_bar_set_value(exit_q_bar, 0, LV_ANIM_OFF);
    exit_q_close(true);
    return;
  }
  int remain = (int)(1000UL - (elapsed * 1000UL / exit_q_dur_ms));
  if (remain < 0) remain = 0;
  if (remain > 1000) remain = 1000;
  lv_bar_set_value(exit_q_bar, remain, LV_ANIM_OFF);
}

//==================== نامپد لمسی ====================
// ctx: 1=صفحهٔ اصلی  2=لیست  3=طول شاخه (F1)  4=عرض تیغه (F1، اعشار)
static void settings_sync_stock_kerf_labels();  // forward — در بخش F1 تعریف می‌شود
static bool f1_general_settings_active();       // forward
static bool settings_soft_kb_tab_ok();          // F1/F2 + soft-KB
void f2_hide_line_kb();
static void f2_ensure_line_kb();
extern void f4_kb_hide(void);
void ui_dismiss_all_soft_keyboards(void);
static void opt_invalidate_cache();
void opt_main_bar_lbl_hide();
void opt_main_bar_lbl_sync();
static void opt_main_bar_click_cb(lv_event_t *e);  // fwd — لمس کادر شاخه/ردیف → پنل مربوط
static void opt_open_panel();  // fwd
void jog_manual_badge_hide();
void jog_manual_badge_sync();
void jog_manual_badge_invalidate();
void blade_err_badge_sync();
void blade_err_disarm_restore_destination();
static void numpad_build();
static void numpad_style_f1();
static void list_reflow_compact();
static int list_max_top();
static void list_page_refresh_flags();
static lv_obj_t *list_row_panel(int i);  // forward — قبل از list_start_scroll_motion
static lv_obj_t *numpad_cont = NULL;
static lv_obj_t *f1_ok_overlay = NULL;
static lv_obj_t *list_bs_overlay = NULL;
static lv_obj_t *list_ok_overlay = NULL;
static lv_obj_t *main_numpad_catcher = NULL;  // لمس بیرون نامپد مقصد → بستن
static int numpad_ctx = 0;
static String settings_num_buf;  // بافر ورودی F1 (stock/kerf)
static bool settings_clear_on_type = false;  // مثل clear_buffer_sp_flag صفحهٔ اصلی
static lv_obj_t *box_kerf = NULL;
static lv_obj_t *box_stock = NULL;

// ---- ذخیرهٔ F1..F5: سبز = تغییر ذخیره‌نشده؛ ترک تب/صفحه بدون Save = برگشت ----
static bool settings_dirty = false;
static void f1_sync_estop_visual();
static void f2_motor_fields_reflow();
static void f1_sync_print_chip_visual();
void f2_apply_jog_slider_from_value(int ramp_ms);
int f2_read_jog_ramp_ms();
void f2_end_edit();
void f2_commit_start_value();
void f2_commit_end_value();
void f2_begin_edit_start_line();
void f2_begin_edit_end_line();
void f2_on_digit_typed();
bool f2_try_append_char(char d);
void f2_on_backspace();
static float f2_ppmm();
static float f2_start_mm_from_pulses();
static void f2_format_size_buf(String &buf, float v, bool keep_frac);
static void f2_load_start_buf_from_pulses();
static void f1_caret_hide();

struct SettingsSnap {
  int stock_len_mm;
  float kerf_mm;
  bool virtual_kb_enable;
  uint8_t vkb_mode;
  bool buzzer_enable;
  bool joystick_enable;
  bool stopstate;
  bool printer_enabled;
  uint8_t printer_model;
  bool diagnostics_overlay_enabled;
  bool blade_err_enable;
  float blade_err_mm;
  int system_language;
  int L1_GUI;
  int L2_GUI;
  int initial_Position_p;
  char final_mm[20];
  int final_position_p;
  int global_speed;
  int global_init_speed;
  int cut_prcs_timer;
  int system_base_dir;
  int visible_raws;
  int pend_cut_prcs_timer;
  int pend_system_base_dir;
  int pend_global_speed;
  int pend_global_init_speed;
  int pend_system_language;
  bool pend_buzzer;
  bool pend_vkb;
  bool pend_joystick;
  int pend_visible_raws;
  int jog_speed_interval;
  int jog_init_speed;
  int auto_ramp_ms;
  int jog_ramp_ms;
  int f3_stepper_test_pulses;
  int f3_stepper_test_period_ms;
} settings_snap;

static void settings_paint_save_btn(bool dirty) {
  if (!objects.btn_save) return;
  const uint32_t bg = dirty ? 0x34C759u : 0x0a84ffu;
  const uint32_t pressed = dirty ? 0x248a3du : 0x0066ccu;
  lv_obj_set_style_bg_color(objects.btn_save, lv_color_hex(bg), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(objects.btn_save, lv_color_hex(pressed), LV_PART_MAIN | LV_STATE_PRESSED);
}

static void settings_take_snap() {
  settings_snap.stock_len_mm = stock_len_mm;
  settings_snap.kerf_mm = kerf_mm;
  settings_snap.virtual_kb_enable = GUI.virtual_kb_enable;
  settings_snap.vkb_mode = GUI.vkb_mode;
  settings_snap.buzzer_enable = GUI.buzzer_enable;
  settings_snap.joystick_enable = GUI.joystick_enable;
  settings_snap.stopstate = stopstate;
  settings_snap.printer_enabled = printer_enabled;
  settings_snap.printer_model = printer_model;
  settings_snap.diagnostics_overlay_enabled = diagnostics_overlay_enabled;
  settings_snap.blade_err_enable = blade_err_enable;
  settings_snap.blade_err_mm = blade_err_mm;
  settings_snap.system_language = GUI.system_language;
  settings_snap.L1_GUI = GUI.L1_GUI;
  settings_snap.L2_GUI = GUI.L2_GUI;
  settings_snap.initial_Position_p = STP.initial_Position_p;
  strncpy(settings_snap.final_mm, STP.final_position_mm.c_str(), sizeof(settings_snap.final_mm) - 1);
  settings_snap.final_mm[sizeof(settings_snap.final_mm) - 1] = '\0';
  settings_snap.final_position_p = STP.final_position_p;
  settings_snap.global_speed = STP.global_speed;
  settings_snap.global_init_speed = STP.global_init_speed;
  settings_snap.cut_prcs_timer = IOS.cut_prcs_timer;
  settings_snap.system_base_dir = STP.system_base_dir;
  settings_snap.visible_raws = GUI.visible_raws;
  settings_snap.pend_cut_prcs_timer = PEND.cut_prcs_timer;
  settings_snap.pend_system_base_dir = PEND.system_base_dir;
  settings_snap.pend_global_speed = PEND.global_speed;
  settings_snap.pend_global_init_speed = PEND.global_init_speed;
  settings_snap.pend_system_language = PEND.system_language;
  settings_snap.pend_buzzer = PEND.buzzer_enable;
  settings_snap.pend_vkb = PEND.virtual_kb_enable;
  settings_snap.pend_joystick = PEND.joystick_enable;
  settings_snap.pend_visible_raws = PEND.visible_raws;
  settings_snap.jog_speed_interval = jog_speed_interval;
  settings_snap.jog_init_speed = jog_init_speed;
  settings_snap.auto_ramp_ms = auto_ramp_ms;
  settings_snap.jog_ramp_ms = jog_ramp_ms;
  settings_snap.f3_stepper_test_pulses = f3_stepper_test_pulses;
  settings_snap.f3_stepper_test_period_ms = f3_stepper_test_period_ms;
}

static bool settings_values_match_snap() {
  return stock_len_mm == settings_snap.stock_len_mm &&
         fabsf(kerf_mm - settings_snap.kerf_mm) < 0.0001f &&
         GUI.virtual_kb_enable == settings_snap.virtual_kb_enable &&
         GUI.vkb_mode == settings_snap.vkb_mode &&
         GUI.buzzer_enable == settings_snap.buzzer_enable &&
         GUI.joystick_enable == settings_snap.joystick_enable &&
         stopstate == settings_snap.stopstate &&
         printer_enabled == settings_snap.printer_enabled &&
         printer_model == settings_snap.printer_model &&
         diagnostics_overlay_enabled == settings_snap.diagnostics_overlay_enabled &&
         blade_err_enable == settings_snap.blade_err_enable &&
         fabsf(blade_err_mm - settings_snap.blade_err_mm) < 0.0001f &&
         GUI.system_language == settings_snap.system_language &&
         GUI.L1_GUI == settings_snap.L1_GUI &&
         GUI.L2_GUI == settings_snap.L2_GUI &&
         STP.initial_Position_p == settings_snap.initial_Position_p &&
         STP.final_position_mm == settings_snap.final_mm &&
         STP.final_position_p == settings_snap.final_position_p &&
         STP.global_speed == settings_snap.global_speed &&
         STP.global_init_speed == settings_snap.global_init_speed &&
         IOS.cut_prcs_timer == settings_snap.cut_prcs_timer &&
         STP.system_base_dir == settings_snap.system_base_dir &&
         GUI.visible_raws == settings_snap.visible_raws &&
         PEND.cut_prcs_timer == settings_snap.pend_cut_prcs_timer &&
         PEND.system_base_dir == settings_snap.pend_system_base_dir &&
         PEND.global_speed == settings_snap.pend_global_speed &&
         PEND.global_init_speed == settings_snap.pend_global_init_speed &&
         PEND.system_language == settings_snap.pend_system_language &&
         PEND.buzzer_enable == settings_snap.pend_buzzer &&
         PEND.virtual_kb_enable == settings_snap.pend_vkb &&
         PEND.joystick_enable == settings_snap.pend_joystick &&
         PEND.visible_raws == settings_snap.pend_visible_raws &&
         (!objects.slider2_1 ||
          map(lv_slider_get_value(objects.slider2_1), 0, 1000, 100, 8) == settings_snap.pend_global_speed) &&
         jog_speed_interval == settings_snap.jog_speed_interval &&
         jog_init_speed == settings_snap.jog_init_speed &&
         auto_ramp_ms == settings_snap.auto_ramp_ms &&
         jog_ramp_ms == settings_snap.jog_ramp_ms &&
         f3_stepper_test_pulses == settings_snap.f3_stepper_test_pulses &&
         f3_stepper_test_period_ms == settings_snap.f3_stepper_test_period_ms &&
         !f4_has_pending_header_save_edit();
}

static void settings_sync_dirty_state() {
  const bool dirty = !settings_values_match_snap() || pinmap_pending_has_changes();
  settings_dirty = dirty;
  settings_paint_save_btn(dirty);
}

void settings_mark_dirty() {
  if (GUI.scr_id != 2) return;
  settings_sync_dirty_state();
}

// Save هدر: اگر هیچ تغییری نبود NVS ننویس
bool settings_has_unsaved_changes() {
  settings_sync_dirty_state();
  return settings_dirty;
}

static void settings_restore_from_snap(bool sync_ui) {
  stock_len_mm = settings_snap.stock_len_mm;
  kerf_mm = settings_snap.kerf_mm;
  GUI.virtual_kb_enable = settings_snap.virtual_kb_enable;
  GUI.vkb_mode = settings_snap.vkb_mode;
  GUI.buzzer_enable = settings_snap.buzzer_enable;
  GUI.joystick_enable = settings_snap.joystick_enable;
  stopstate = settings_snap.stopstate;
  printer_enabled = settings_snap.printer_enabled;
  printer_model = settings_snap.printer_model;
  diagnostics_overlay_enabled = settings_snap.diagnostics_overlay_enabled;
  if (!printer_enabled) printertest = 0;
  blade_err_enable = settings_snap.blade_err_enable;
  blade_err_mm = settings_snap.blade_err_mm;
  PEND.blade_err_enable = blade_err_enable;
  PEND.blade_err_mm = blade_err_mm;
  GUI.L1_GUI = settings_snap.L1_GUI;
  GUI.L2_GUI = settings_snap.L2_GUI;
  STP.initial_Position_p = settings_snap.initial_Position_p;
  STP.final_position_mm = String(settings_snap.final_mm);
  STP.final_position_p = settings_snap.final_position_p;
  STP.global_speed = settings_snap.global_speed;
  STP.global_init_speed = settings_snap.global_init_speed;
  IOS.cut_prcs_timer = settings_snap.cut_prcs_timer;
  STP.system_base_dir = settings_snap.system_base_dir;
  GUI.visible_raws = settings_snap.visible_raws;
  PEND.cut_prcs_timer = settings_snap.pend_cut_prcs_timer;
  PEND.system_base_dir = settings_snap.pend_system_base_dir;
  PEND.global_speed = settings_snap.pend_global_speed;
  PEND.global_init_speed = settings_snap.pend_global_init_speed;
  PEND.system_language = settings_snap.pend_system_language;
  PEND.buzzer_enable = settings_snap.pend_buzzer;
  PEND.virtual_kb_enable = settings_snap.pend_vkb;
  PEND.joystick_enable = settings_snap.pend_joystick;
  PEND.visible_raws = settings_snap.pend_visible_raws;
  jog_speed_interval = settings_snap.jog_speed_interval;
  jog_init_speed = settings_snap.jog_init_speed;
  auto_ramp_ms = settings_snap.auto_ramp_ms;
  jog_ramp_ms = settings_snap.jog_ramp_ms;
  f3_stepper_test_pulses = settings_snap.f3_stepper_test_pulses;
  f3_stepper_test_period_ms = settings_snap.f3_stepper_test_period_ms;
  STP.global_lowSpeedTime = auto_ramp_ms;
  GUI.enable_edit_lable2_3 = false;
  GUI.enable_edit_lable2_5 = false;
  f2_load_start_buf_from_pulses();
  GUI.inputBuffer_max_L = STP.final_position_mm;
  numpad_close();
  f2_end_edit();
  if (GUI.system_language != settings_snap.system_language) {
    GUI.system_language = settings_snap.system_language;
    PEND.system_language = settings_snap.system_language;
    GUI.pending_lang_selected = (settings_snap.system_language == 1) ? 0 : 1;
    GUI.flag_apply_language = true;
  } else {
    GUI.system_language = settings_snap.system_language;
  }
  if (!sync_ui) {
    f2_apply_jog_slider_from_value(jog_ramp_ms);
    return;
  }
  f2_apply_jog_slider_from_value(jog_ramp_ms);
  settings_sync_stock_kerf_labels();
  f1_sync_estop_visual();
  f1_sync_print_chip_visual();
  f2_motor_fields_reflow();
  GUI.flag_reinit_settings = true;
}

void settings_on_tab_changed() {
  if (GUI.scr_id != 2) return;
  settings_sync_dirty_state();
  // تمام کیبوردهای تب قبلی حتی یک فریم روی تب تازه دیده نشود.
  ui_dismiss_all_soft_keyboards();
  // فوری: کروم قدیمی EEZ را پنهان/بازنشسته کن تا قبل از رندر بعدی فلش نزند
  settings_hide_eez_clutter();
  if (settings_dirty) {
    settings_restore_from_snap(true);
    settings_dirty = false;
  }
  load_pending_from_current();
  pinmap_pending_discard();
  f4_chain_pending_discard();
  settings_take_snap();
  settings_paint_save_btn(false);
  admin_tabs_reflow();
  settings_hide_eez_clutter();  // بعد از reflow زبان/لیبل‌های عنوان قدیمی دوباره hide
  vkb_apply_effective(true);
  ui_resume_checkpoint();
}

void settings_on_leave_settings() {
  extern void f4_session_lock();
  f4_session_lock();  // برگشت به صفحه اصلی → دوباره رمز F4
  ui_dismiss_all_soft_keyboards();
  if (objects.tabview2_1 && lv_obj_is_valid(objects.tabview2_1)) {
    lv_tabview_set_active(objects.tabview2_1, 0, LV_ANIM_OFF);
  }
  settings_sync_dirty_state();
  if (!settings_dirty) {
    settings_paint_save_btn(false);
  } else {
    settings_restore_from_snap(false);
    settings_dirty = false;
    settings_paint_save_btn(false);
    load_pending_from_current();
  }
  pinmap_pending_discard();
  f4_chain_pending_discard();
  // اگر استاپ سخت‌افزاری هنوز فعال است، بعد از خروج از تنظیمات اسپلش را نشان بده
  if (IOS.stop_button_state) GUI.flag_cast_emg = true;
  else GUI.flag_del_emg = true;
}

void settings_on_enter_settings() {
  load_pending_from_current();
  pinmap_pending_begin();
  f4_chain_pending_begin();
  settings_take_snap();
  settings_dirty = false;
  settings_paint_save_btn(false);
}

void settings_after_save_all() {
  load_pending_from_current();
  pinmap_pending_begin();
  f4_chain_pending_begin();
  settings_take_snap();
  settings_dirty = false;
  settings_paint_save_btn(false);
}

// قبل از Save: بافر ویرایش و اسلایدرها را به RAM بریز تا NVS عدد روی صفحه را بگیرد
void settings_flush_pending_edits() {
  if (numpad_ctx == 3) {
    long v = settings_num_buf.toInt();
    int old_stock = stock_len_mm;
    stock_len_mm = (int)constrain(v, 10L, 100000L);
    if (stock_len_mm != old_stock) opt_invalidate_cache();
    settings_sync_stock_kerf_labels();
    numpad_ctx = 0;
    settings_num_buf = "";
    settings_clear_on_type = false;
    f1_caret_hide();
    if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
    if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  } else if (numpad_ctx == 4) {
    float v = roundf(settings_num_buf.toFloat() * 10.0f) / 10.0f;
    float old_kerf = kerf_mm;
    kerf_mm = constrain(v, 0.0f, 50.0f);
    if (fabsf(kerf_mm - old_kerf) > 0.001f) opt_invalidate_cache();
    settings_sync_stock_kerf_labels();
    numpad_ctx = 0;
    settings_num_buf = "";
    settings_clear_on_type = false;
    f1_caret_hide();
    if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
    if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  }

  if (GUI.scr_id == 2 && GUI.enable_edit_lable2_3) {
    float vf = GUI.inputBuffer_initial_L2.toFloat();
    float endv = STP.final_position_mm.toFloat();
    float mx = 9999.0f;
    if (endv > 0.0f && endv < mx) mx = endv;
    if (mx < 30.0f) mx = 30.0f;
    if (vf < 30.0f) vf = 30.0f;
    if (vf > mx) vf = mx;
    const bool keep_frac = GUI.inputBuffer_initial_L2.indexOf('.') >= 0;
    f2_format_size_buf(GUI.inputBuffer_initial_L2, vf, keep_frac);
    vf = GUI.inputBuffer_initial_L2.toFloat();
    STP.initial_Position_p = (int)lroundf(f2_ppmm() * vf);
    GUI.L2_GUI = (int)lroundf(vf);
    if (objects.lable2_2) {
      lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
      lv_obj_set_style_text_color(objects.lable2_2, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
  if (GUI.scr_id == 2 && GUI.enable_edit_lable2_5) {
    STP.final_position_mm = GUI.inputBuffer_max_L;
    STP.final_position_p = round((float)STP.final_position_mm.toFloat() * (STP.drv_ppr / STP.Axis_Pitch));
    if (objects.lable2_3) {
      lv_label_set_text(objects.lable2_3, STP.final_position_mm.c_str());
      lv_obj_set_style_text_color(objects.lable2_3, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
  f2_end_edit();

  if (objects.slider2_1)
    PEND.global_speed = map(lv_slider_get_value(objects.slider2_1), 0, 1000, 100, 8);
  if (objects.slider2_2) {
    auto_ramp_ms = (int)map(lv_slider_get_value(objects.slider2_2), 0, 1000, 0, 5000);
    STP.global_lowSpeedTime = auto_ramp_ms;
    PEND.global_init_speed = (auto_ramp_ms > 0) ? 400 : PEND.global_speed;
  }
  jog_ramp_ms = f2_read_jog_ramp_ms();
  jog_init_speed = (jog_ramp_ms > 0) ? 400 : 0;

  // F4: commit کادرهای باز + ساعت/تاریخ با همان Save
  if (GUI.scr_id == 2) {
    f4_flush_open_edits_for_save();
    f4_clock_commit_from_ui();
  }
}

void settings_refresh_after_reinit() {
  if (!settings_dirty) settings_on_enter_settings();
  else settings_paint_save_btn(true);
}

// نشانگر چشمک‌زن ورودی F1 (طول شاخه / عرض تیغه)
static lv_obj_t *f1_edit_caret = NULL;
static uint32_t f1_caret_blink_ms = 0;
static bool f1_caret_lit = true;

static void f1_caret_hide() {
  if (f1_edit_caret) lv_obj_add_flag(f1_edit_caret, LV_OBJ_FLAG_HIDDEN);
}

static void f1_caret_refresh() {
  if (numpad_ctx != 3 && numpad_ctx != 4) {
    f1_caret_hide();
    return;
  }
  lv_obj_t *box = (numpad_ctx == 3) ? box_stock : box_kerf;
  if (!box) { f1_caret_hide(); return; }
  if (!f1_edit_caret) {
    f1_edit_caret = lv_obj_create(box);
    lv_obj_set_size(f1_edit_caret, 2, 22);
    lv_obj_clear_flag(f1_edit_caret, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_style_bg_color(f1_edit_caret, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(f1_edit_caret, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f1_edit_caret, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f1_edit_caret, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(f1_edit_caret, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_margin_left(f1_edit_caret, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if (lv_obj_get_parent(f1_edit_caret) != box) {
    lv_obj_set_parent(f1_edit_caret, box);
  }
  lv_obj_clear_flag(f1_edit_caret, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f1_edit_caret);
  lv_obj_set_style_bg_opa(f1_edit_caret, f1_caret_lit ? LV_OPA_COVER : LV_OPA_TRANSP,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
}

void f1_caret_tick() {
  if (numpad_ctx != 3 && numpad_ctx != 4) {
    f1_caret_hide();
    return;
  }
  uint32_t now = millis();
  if ((int32_t)(now - f1_caret_blink_ms) >= 450) {
    f1_caret_blink_ms = now;
    f1_caret_lit = !f1_caret_lit;
  }
  f1_caret_refresh();
}

// CL-MAIN-02 / CL-LIST-01: یک ممیز، بدون صفر پیشرو اضافه، ترتیب LTR
static void numeric_buf_append(String &buf, char d, int maxlen, bool allow_dot) {
  if (d == '.') {
    if (!allow_dot) return;
    if (buf.indexOf('.') >= 0) return;
    if (buf.length() == 0) buf = "0";
    if ((int)buf.length() < maxlen) buf += '.';
    return;
  }
  if (d < '0' || d > '9') return;
  if (buf == "0") {
    buf = String(d);
    return;
  }
  if ((int)buf.length() < maxlen) buf += d;
}

// سایز لیست برش: رقم صحیح ≤ رقم‌های طول شاخه؛ حداکثر یک ممیز؛ مقدار ≤ طول شاخه
static int list_size_max_int_digits() {
  int n = stock_len_mm;
  if (n < 1) n = 1;
  int d = 0;
  while (n > 0) {
    d++;
    n /= 10;
  }
  return d < 1 ? 1 : d;
}

bool list_try_size_digit(String &buf, char d) {
  if (d == ',') d = '.';
  const int max_int = list_size_max_int_digits();
  if (d == '.') {
    if (buf.indexOf('.') >= 0) {
      GUI.buzzer_cmd_long = true;
      ui_toast("فقط یک ممیز مجاز است", "Only one decimal point allowed", 2500);
      return false;
    }
    int int_len = (int)buf.length();
    if (buf.length() == 0) {
      buf = "0.";
      return true;
    }
    if (int_len > max_int) {
      GUI.buzzer_cmd_long = true;
      ui_toast("محدوده عدد رعایت نشده", "Number out of range", 2800);
      return false;
    }
    buf += '.';
    return true;
  }
  if (d < '0' || d > '9') return false;
  int dot = buf.indexOf('.');
  if (dot >= 0) {
    if ((int)buf.length() - dot - 1 >= 1) {
      GUI.buzzer_cmd_long = true;
      return false;
    }
    String trial = buf + d;
    float v = trial.toFloat();
    if (v > (float)stock_len_mm + 0.05f) {
      GUI.buzzer_cmd_long = true;
      char msg[64];
      snprintf(msg, sizeof(msg), "حداکثر %d mm", stock_len_mm);
      ui_toast(msg, msg, 2800);
      return false;
    }
    buf += d;
    return true;
  }
  String trial = (buf == "0") ? String(d) : (buf + d);
  if ((int)trial.length() > max_int) {
    GUI.buzzer_cmd_long = true;
    char msg[72];
    snprintf(msg, sizeof(msg), "حداکثر %d رقم (طول شاخه %d)", max_int, stock_len_mm);
    ui_toast(msg, msg, 3000);
    return false;
  }
  float v = trial.toFloat();
  if (v > (float)stock_len_mm + 0.05f) {
    GUI.buzzer_cmd_long = true;
    char msg[64];
    snprintf(msg, sizeof(msg), "حداکثر %d mm", stock_len_mm);
    ui_toast(msg, msg, 2800);
    return false;
  }
  buf = trial;
  return true;
}

// هدف صفحه اصلی: حداکثر ۴ رقم، با اعشار حداکثر ۵ رقم (یک رقم بعد ممیز)
static bool append_main_target_char(String &buf, char d) {
  if (d == ',') d = '.';  // ممیز فارسی/اروپایی روی بعضی کیبوردها
  if (d == '.') {
    if (buf.indexOf('.') >= 0) return false;
    if ((int)buf.length() > 4) return false;
    if (buf.length() == 0) buf = "0";
    buf += '.';
    return true;
  }
  if (d < '0' || d > '9') return false;
  int dot = buf.indexOf('.');
  if (dot >= 0) {
    if ((int)buf.length() - dot - 1 >= 1) return false;
    buf += d;
    return true;
  }
  if (buf == "0") { buf = String(d); return true; }
  if ((int)buf.length() >= 4) return false;
  buf += d;
  return true;
}

static void entry_add_digit(char d) {
  if (numpad_ctx == 1) {
    if (STP.motorIs_moving || STP.moveStepperCMD) {
      GUI.buzzer_cmd_long = true;
      ui_toast("موتور در حال حرکت است. الان نمیتوانید وارد لیست شوید",
               "Motor is moving. Cannot open the list now", 3500);
      return;
    }
    // مثل USB: در جاگ / لیست / بهینه اول خروج از مد، بعد تایپ
    if (GUI.NumLock) {
      request_exit_queue_mode_prompt(3);
      return;
    }
    if (main_queue_mode_active()) {
      request_exit_queue_mode_prompt(1);
      return;
    }
    if (clear_buffer_sp_flag) { GUI.inputBuffer_sp = ""; clear_buffer_sp_flag = false; }
    if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)2) == pdTRUE) {
      if (!append_main_target_char(GUI.inputBuffer_sp, d)) GUI.buzzer_cmd_long = true;
      xSemaphoreGive(lable1_1_Semaphore);
    } else if (!append_main_target_char(GUI.inputBuffer_sp, d)) {
      GUI.buzzer_cmd_long = true;
    }
    GUI.update_lable1_1 = true;
    return;
  }
  if (numpad_ctx == 2) {
    // بدون انتخاب متراژ/تعداد → اولین متراژ صفحه
    bool any = false;
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_sp[j] || AUT.list_edit_mode_count[j]) { any = true; break; }
    }
    if (!any) {
      int first = GUI.list_page;
      if (first < 0) first = 0;
      if (first >= LIST_MAX) first = LIST_MAX - 1;
      if (first >= GUI.visible_raws && GUI.visible_raws > 0) first = GUI.visible_raws - 1;
      list_show_row(first);
      begin_screen3_sp_edit(first);
    }
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_sp[j]) {
        if (GUI.clear_buffer_aut_flag) GUI.inputBuffer_L_3_1_x[j] = "";
        GUI.clear_buffer_aut_flag = false;
        list_try_size_digit(GUI.inputBuffer_L_3_1_x[j], d);
        GUI.flag_update_labels_3_1 = true;
        return;
      }
      if (AUT.list_edit_mode_count[j]) {
        if (d == '.') return;
        if (GUI.clear_buffer_aut_flag) GUI.inputBuffer_L_3_2_x[j] = "";
        GUI.clear_buffer_aut_flag = false;
        numeric_buf_append(GUI.inputBuffer_L_3_2_x[j], d, 3, false);
        GUI.flag_update_labels_3_2 = true;
        return;
      }
    }
    return;
  }
  if (numpad_ctx == 3 || numpad_ctx == 4) {
    // مثل صفحهٔ اصلی: اولین رقم جایگزین مقدار قبلی می‌شود
    if (settings_clear_on_type) {
      settings_num_buf = "";
      settings_clear_on_type = false;
    }
    int lim = (numpad_ctx == 3) ? 6 : 5;  // 100000 یا 50.0
    numeric_buf_append(settings_num_buf, d, lim, numpad_ctx == 4);
    settings_sync_stock_kerf_labels();  // پیش‌نمایش در حین تایپ
    f1_caret_lit = true;
    f1_caret_blink_ms = millis();
    f1_caret_refresh();
    return;
  }
  if (numpad_ctx == 5) {
    // اصلاح خطای برش: محدوده -9.9 تا +9.9 میلیمتر
    if (settings_clear_on_type) {
      settings_num_buf = "";
      settings_clear_on_type = false;
    }
    if (d == '-' || d == '+') {
      if (settings_num_buf.startsWith("-")) {
        settings_num_buf = settings_num_buf.substring(1);
      } else {
        settings_num_buf = "-" + settings_num_buf;
      }
    } else {
      numeric_buf_append(settings_num_buf, d, 5, true);
    }
    blade_err_badge_sync();
    return;
  }
}

static void entry_backspace() {
  if (numpad_ctx == 1) {
    if (STP.motorIs_moving || STP.moveStepperCMD) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (!GUI.update_lable1_1 && GUI.inputBuffer_sp.length()) {
      GUI.inputBuffer_sp.remove(GUI.inputBuffer_sp.length() - 1);
      GUI.update_lable1_1 = true;
    }
    return;
  }
  if (numpad_ctx == 3 || numpad_ctx == 4) {
    // بعد از بک‌اسپیس دیگر جایگزین‌کامل با رقم بعدی نشود
    settings_clear_on_type = false;
    if (settings_num_buf.length()) {
      settings_num_buf.remove(settings_num_buf.length() - 1);
      settings_sync_stock_kerf_labels();
      f1_caret_lit = true;
      f1_caret_blink_ms = millis();
      f1_caret_refresh();
    }
    return;
  }
  if (numpad_ctx == 5) {
    settings_clear_on_type = false;
    if (settings_num_buf.length()) {
      settings_num_buf.remove(settings_num_buf.length() - 1);
      blade_err_badge_sync();
    }
    return;
  }
  for (int j = 0; j < LIST_MAX; j++) {
    if (AUT.list_edit_mode_sp[j] && GUI.inputBuffer_L_3_1_x[j].length()) {
      GUI.inputBuffer_L_3_1_x[j].remove(GUI.inputBuffer_L_3_1_x[j].length() - 1);
      GUI.flag_update_labels_3_1 = true;
      return;
    }
    if (AUT.list_edit_mode_count[j] && GUI.inputBuffer_L_3_2_x[j].length()) {
      GUI.inputBuffer_L_3_2_x[j].remove(GUI.inputBuffer_L_3_2_x[j].length() - 1);
      GUI.flag_update_labels_3_2 = true;
      return;
    }
  }
}

// فقط ویجت نامپد لمسی را مخفی کن — فوکوس USB روی F1 (طول شاخه/تیغه) را پاک نکن
static void numpad_hide_soft_widgets() {
  if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
  if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  if (list_bs_overlay) lv_obj_add_flag(list_bs_overlay, LV_OBJ_FLAG_HIDDEN);
  if (list_ok_overlay) lv_obj_add_flag(list_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  if (main_numpad_catcher) lv_obj_add_flag(main_numpad_catcher, LV_OBJ_FLAG_HIDDEN);
}

void numpad_close() {
  const int prev_ctx = numpad_ctx;
  numpad_hide_soft_widgets();
  numpad_ctx = 0;
  settings_num_buf = "";
  settings_clear_on_type = false;
  f1_caret_hide();
  // بستن کیبورد لیست → ۵ ردیف و پر کردن دوباره کادر سفید
  if (prev_ctx == 2 && GUI.scr_id == 3 && objects.panel3_1) {
    int max_top = list_max_top();
    if (GUI.list_page > max_top) GUI.list_page = max_top;
    if (GUI.list_page < 0) GUI.list_page = 0;
    list_reflow_compact();
    list_page_refresh_flags();
  }
}

void ui_dismiss_all_soft_keyboards(void) {
  numpad_close();
  f2_hide_line_kb();
  f4_kb_hide();
}

static void numpad_commit() {
  if (numpad_ctx == 1) {
    commit_main_sp(true);  // با فرمان حرکت (مثل Enter کیبورد)
    numpad_close();
    return;
  }
  if (numpad_ctx == 3) {
    long v = settings_num_buf.toInt();
    int old_stock = stock_len_mm;
    stock_len_mm = (int)constrain(v, 10L, 100000L);
    if (stock_len_mm != old_stock) opt_invalidate_cache();
    GUI.flag_save_stock_kerf = false;  // فقط با دکمه Save
    settings_mark_dirty();
    // Enter / تأیید روی طول شاخه → پرش به عرض تیغه (نشانگر هم می‌رود)
    numpad_ctx = 4;
    {
      char tmp[16];
      snprintf(tmp, sizeof(tmp), "%.1f", (double)kerf_mm);
      settings_num_buf = tmp;
    }
    settings_clear_on_type = true;
    settings_sync_stock_kerf_labels();
    f1_caret_lit = true;
    f1_caret_blink_ms = millis();
    f1_caret_refresh();
    if (GUI.virtual_kb_enable && f1_ok_overlay) {
      lv_obj_clear_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(f1_ok_overlay);
    }
    return;
  }
  if (numpad_ctx == 4) {
    float v = settings_num_buf.toFloat();
    // گرد کردن به ۰.۱
    v = roundf(v * 10.0f) / 10.0f;
    float old_kerf = kerf_mm;
    kerf_mm = constrain(v, 0.0f, 50.0f);
    if (fabsf(kerf_mm - old_kerf) > 0.001f) opt_invalidate_cache();
    GUI.flag_save_stock_kerf = false;  // فقط با دکمه Save
    settings_mark_dirty();
    // تأیید عرض تیغه → کامل خارج از ویرایش (بدون نشانگر چشمک‌زن)
    numpad_ctx = 0;
    settings_num_buf = "";
    settings_clear_on_type = false;
    f1_caret_hide();
    settings_sync_stock_kerf_labels();
    if (!GUI.virtual_kb_enable) {
      if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
      if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
      // نامپد می‌تواند بماند؛ فوکوس خاموش
      if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
  if (numpad_ctx == 5) {
    if (settings_num_buf.length() == 0 || settings_num_buf == "-" || settings_num_buf == "+") {
      ui_toast("خطا: مقدار وارد شده نامعتبر است", "Invalid value entered", 2400);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      char tmp[16];
      snprintf(tmp, sizeof(tmp), "%.1f", (double)blade_err_mm);
      settings_num_buf = tmp;
      blade_err_badge_sync();
      return;
    }
    float v = settings_num_buf.toFloat();
    v = roundf(v * 10.0f) / 10.0f;
    if (v < -9.9f || v > 9.9f) {
      ui_toast("خطا: محدوده مجاز ۹.۹- تا ۹.۹+ میلی‌متر است", "Allowed range: -9.9 to +9.9 mm", 2800);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      char tmp[16];
      snprintf(tmp, sizeof(tmp), "%.1f", (double)blade_err_mm);
      settings_num_buf = tmp;
      blade_err_badge_sync();
      return;
    }
    blade_err_mm = v;
    PEND.blade_err_mm = v;
    pref.begin("params", false);
    pref_put_float_if_changed(pref, "bld_err", blade_err_mm);
    pref.end();
    if (blade_err_armed) {
      STP.position_Setpoint_p = blade_err_motor_target_p(STP.Man_Sp_mm);
      AUT.in_position_flag = false;
      STP.in_position_flag = false;
    }
    char toast_txt[48];
    snprintf(toast_txt, sizeof(toast_txt), "اصلاح خطای برش: %.1f mm", (double)v);
    ui_toast(toast_txt, "Blade offset saved", 1800);
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    numpad_close();
    blade_err_badge_sync();
    return;
  }
  for (int j = 0; j < LIST_MAX; j++) {
    if (AUT.list_edit_mode_sp[j]) { commit_sp_entry(j); return; }  // نامپد باز می‌ماند برای تعداد
    if (AUT.list_edit_mode_count[j]) { commit_count_entry(j); return; }
  }
  numpad_close();
}

// برای Task4 (کیبورد USB) — روی F1، حتی اگر کیبورد مجازی خاموش باشد
bool f1_num_entry_active() {
  if (GUI.scr_id != 2) return false;
  if (!f1_general_settings_active()) return false;
  return (numpad_ctx == 3 || numpad_ctx == 4);
}

// آماده‌سازی فوکوس فیلد (بدون اجبار به نامپد لمسی)
static void f1_edit_focus(int ctx, bool clear_on_first) {
  if (ctx != 3 && ctx != 4) return;
  if (!f1_general_settings_active()) return;
  // گارد: اگر کاربر در همین کادر در حال تایپ است، بافر را ریست نکن
  const bool already_editing = (numpad_ctx == ctx) && !settings_clear_on_type;
  numpad_ctx = ctx;
  if (!already_editing) {
    if (ctx == 3) settings_num_buf = String(stock_len_mm);
    else {
      char tmp[16];
      snprintf(tmp, sizeof(tmp), "%.1f", (double)kerf_mm);
      settings_num_buf = tmp;
    }
    settings_clear_on_type = clear_on_first;
  }
  // already_editing → بافر و settings_clear_on_type دست نخورده می‌مانند
  if (GUI.virtual_kb_enable) {
    numpad_build();
    numpad_style_f1();
    if (numpad_cont) {
      lv_obj_clear_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(numpad_cont);
    }
    if (f1_ok_overlay) {
      lv_obj_clear_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(f1_ok_overlay);
    }
  }
  f1_caret_lit = true;
  f1_caret_blink_ms = millis();
  f1_caret_refresh();
  settings_sync_stock_kerf_labels();
}

void f1_num_entry_digit(char d) {
  if (GUI.scr_id != 2 || !f1_general_settings_active()) return;
  // اولین رقم بدون فوکوس قبلی → طول شاخه + پاک‌کردن با رقم اول
  if (numpad_ctx != 3 && numpad_ctx != 4)
    f1_edit_focus(3, true);
  entry_add_digit(d);
}

// Task4: اولین رقم فیزیکی بدون فوکوس — حتی با کیبورد مجازی خاموش
bool f1_try_num_digit(char d) {
  // فقط تب F1 (طول شاخه/تیغه) — روی F2 نباید رقم را ببلعد
  if (GUI.scr_id != 2 || !f1_general_settings_active()) return false;
  f1_num_entry_digit(d);
  return true;
}
void f1_num_entry_backspace() {
  if (!f1_num_entry_active()) return;
  entry_backspace();
}
void f1_num_entry_commit() {
  if (!f1_num_entry_active()) return;
  numpad_commit();
}

void prcs_keyboard_ui_requests() {
  if (GUI.flag_sync_stock_kerf) {
    GUI.flag_sync_stock_kerf = false;
    settings_sync_stock_kerf_labels();
    f1_caret_refresh();
  }
  if (GUI.flag_close_numpad) {
    GUI.flag_close_numpad = false;
    numpad_close();
  }
  f1_caret_tick();
  f2_caret_tick();
}

static void numpad_event_cb(lv_event_t *e) {
  lv_obj_t *bm = (lv_obj_t *)lv_event_get_target(e);
  uint32_t id = lv_buttonmatrix_get_selected_button(bm);
  const char *txt = lv_buttonmatrix_get_button_text(bm, id);
  if (!txt) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);

  // F2: ارقام / ممیز به بافر اول/آخر خط (بدون پرش به هندلرهای دیگر)
  bool on_f2 = (GUI.scr_id == 2 && objects.tabview2_1 &&
                (int)lv_tabview_get_tab_act(objects.tabview2_1) == 1);
  if (on_f2) {
    if ((txt[0] >= '0' && txt[0] <= '9' && txt[1] == 0) || (txt[0] == '.' && txt[1] == 0)) {
      if (!GUI.enable_edit_lable2_3 && !GUI.enable_edit_lable2_5)
        f2_begin_edit_start_line();
      f2_try_append_char(txt[0]);
      return;
    }
    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "<<") == 0) {
      if (GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5) f2_on_backspace();
      return;
    }
    if (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "OK") == 0 ||
        (txt[0] == ' ' && txt[1] == 0) || id == 12) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
      if (GUI.enable_edit_lable2_3) f2_commit_start_value();
      else if (GUI.enable_edit_lable2_5) f2_commit_end_value();
      return;
    }
    return;
  }

  // صفحهٔ اصلی (ctx=1) و لیست (ctx=2): رقم / ممیز / بک‌اسپیس / تأیید
  if (numpad_ctx == 1 || numpad_ctx == 2) {
    if (txt[0] >= '0' && txt[0] <= '9' && txt[1] == 0) {
      entry_add_digit(txt[0]);
    } else if (txt[0] == '.' && txt[1] == 0) {
      entry_add_digit('.');
    } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "<<") == 0) {
      entry_backspace();
    } else if (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "OK") == 0 ||
               (txt[0] == ' ' && txt[1] == 0) || id == 12) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
      numpad_commit();
    } else if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
      if (numpad_ctx == 2) cancel_screen3_pending_edit();
      numpad_close();
    }
    return;
  }

  if (txt[0] >= '0' && txt[0] <= '9' && txt[1] == 0) {
    if (numpad_ctx != 3 && numpad_ctx != 4) f1_edit_focus(3, true);
    entry_add_digit(txt[0]);
  } else if (txt[0] == '.' && txt[1] == 0) {
    if (numpad_ctx != 3 && numpad_ctx != 4) f1_edit_focus(4, true);
    entry_add_digit('.');
  } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "<<") == 0) {
    if (numpad_ctx == 3 || numpad_ctx == 4) entry_backspace();
  } else if (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "OK") == 0 ||
             strcmp(txt, "تأیید") == 0 || id == 12) {
    if (numpad_ctx == 3 || numpad_ctx == 4) numpad_commit();
  } else if (txt[0] == ' ' && txt[1] == 0 && id == 12) {
    if (numpad_ctx == 3 || numpad_ctx == 4) numpad_commit();
  } else if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
    numpad_close();
    settings_sync_stock_kerf_labels();
  }
}

// نامپد لیست/اصلی (کنار صفحه)
static const char *numpad_map_side[] = { "1", "2", "3", "\n", "4", "5", "6", "\n",
                                    "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE, "0", ".", "\n",
                                    LV_SYMBOL_OK, LV_SYMBOL_CLOSE, "" };

// لیست: بدون ضربدر؛ ردیف آخر ۴تایی تا کلیدها بلندتر شوند (اندازهٔ بیرونی ثابت)
static const char *numpad_map_list[] = { "1", "2", "3", "\n", "4", "5", "6", "\n",
                                    "7", "8", "9", "\n",
                                    LV_SYMBOL_BACKSPACE, "0", ".", LV_SYMBOL_OK, "" };

// نامپد F1: رقم لاتین (montserrat) + تأیید فارسی با لیبل روی دکمهٔ آخر
static const char *numpad_map_f1[] = { "1", "2", "3", "\n", "4", "5", "6", "\n",
                                    "7", "8", "9", "\n", ".", "0", LV_SYMBOL_BACKSPACE, "\n",
                                    " ", "" };

static void entry_add_digit(char d);
static void entry_backspace();
static void list_bs_overlay_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (numpad_ctx != 1 && numpad_ctx != 2) return;
  entry_backspace();
}

static void list_ok_overlay_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (numpad_ctx != 1 && numpad_ctx != 2) return;
  numpad_commit();
}

static void main_numpad_catcher_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (numpad_ctx != 1) return;
  numpad_close();
}

static void main_numpad_catcher_sync(bool show) {
  if (!show) {
    if (main_numpad_catcher) lv_obj_add_flag(main_numpad_catcher, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (!main_numpad_catcher) {
    main_numpad_catcher = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(main_numpad_catcher, 0, 0);
    lv_obj_set_size(main_numpad_catcher, 480, 320);
    lv_obj_set_style_bg_opa(main_numpad_catcher, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(main_numpad_catcher, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(main_numpad_catcher, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(main_numpad_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(main_numpad_catcher, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(main_numpad_catcher, main_numpad_catcher_cb, LV_EVENT_CLICKED, NULL);
  }
  lv_obj_set_parent(main_numpad_catcher, lv_layer_top());
  lv_obj_clear_flag(main_numpad_catcher, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(main_numpad_catcher);
}

// هندسهٔ کیبرد پهن لیست/مقصد — فقط مقصد اصلی +۷۰px به بالا
static void list_style_numpad_geom(int *np_x, int *np_y, int *np_w, int *np_h) {
  if (!np_x || !np_y || !np_w || !np_h) return;
  *np_x = 10;
  *np_w = 460;
  if (numpad_ctx == 1 && GUI.scr_id == 1) {
    // پایهٔ قبلی (110,10,460,210) +۱۱ بالا +۴ چپ +۴ راست +۴ دیگر از بالا
    *np_x = 6;
    *np_y = 95;
    *np_w = 468;
    *np_h = 225;
  } else {
    *np_x = 10;
    *np_y = 180;
    *np_w = 460;
    *np_h = 140;
  }
}

static void list_bs_overlay_sync(bool show) {
  if (!show) {
    if (list_bs_overlay) lv_obj_add_flag(list_bs_overlay, LV_OBJ_FLAG_HIDDEN);
    if (list_ok_overlay) lv_obj_add_flag(list_ok_overlay, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  int np_x = 10, np_y = 180, np_w = 460, np_h = 140;
  list_style_numpad_geom(&np_x, &np_y, &np_w, &np_h);
  const int pad = 4, gap_c = 6, gap_r = 4, cols = 4, rows = 4;
  const int bw = (np_w - 2 * pad - (cols - 1) * gap_c) / cols;
  const int bh = (np_h - 2 * pad - (rows - 1) * gap_r) / rows;
  const int bx = np_x + pad;
  const int by = np_y + pad + 3 * (bh + gap_r);
  if (!list_bs_overlay) {
    list_bs_overlay = lv_button_create(lv_layer_top());
    lv_obj_set_style_bg_color(list_bs_overlay, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(list_bs_overlay, lv_color_hex(0xD70015), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(list_bs_overlay, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(list_bs_overlay, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list_bs_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(list_bs_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(list_bs_overlay, list_bs_overlay_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *lb = lv_label_create(list_bs_overlay);
    lv_label_set_text(lb, LV_SYMBOL_BACKSPACE);
    lv_obj_set_style_text_color(lb, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lb);
  }
  lv_obj_set_parent(list_bs_overlay, lv_layer_top());
  lv_obj_set_pos(list_bs_overlay, bx, by);
  lv_obj_set_size(list_bs_overlay, bw, bh);
  lv_obj_clear_flag(list_bs_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(list_bs_overlay);

  const int ok_x = np_x + pad + 3 * (bw + gap_c);
  if (!list_ok_overlay) {
    list_ok_overlay = lv_button_create(lv_layer_top());
    lv_obj_set_style_bg_color(list_ok_overlay, lv_color_hex(0x34C759), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(list_ok_overlay, lv_color_hex(0x248A3D), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(list_ok_overlay, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(list_ok_overlay, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list_ok_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(list_ok_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(list_ok_overlay, list_ok_overlay_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *lb = lv_label_create(list_ok_overlay);
    lv_label_set_text(lb, "تأیید");
    lv_obj_set_style_text_color(lb, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lb, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lb);
  }
  lv_obj_set_parent(list_ok_overlay, lv_layer_top());
  lv_obj_set_pos(list_ok_overlay, ok_x, by);
  lv_obj_set_size(list_ok_overlay, bw, bh);
  lv_obj_clear_flag(list_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(list_ok_overlay);
}

static void numpad_style_side() {
  if (!numpad_cont) return;
  // لیست و مقصد اصلی: همان کیبرد پهن پایین صفحه
  const bool list_kb = (numpad_ctx == 2 && GUI.scr_id == 3) ||
                       (numpad_ctx == 1 && GUI.scr_id == 1);
  lv_buttonmatrix_set_map(numpad_cont, list_kb ? numpad_map_list : numpad_map_side);
  lv_obj_set_parent(numpad_cont, lv_layer_top());
  lv_obj_set_align(numpad_cont, LV_ALIGN_TOP_LEFT);
  if (list_kb) {
    int np_x = 10, np_y = 180, np_w = 460, np_h = 140;
    list_style_numpad_geom(&np_x, &np_y, &np_w, &np_h);
    // لیست: y=180 h=140 | مقصد اصلی: +۷۰px به بالا (y=110 h=210)؛ لبه پایین y=320
    lv_obj_set_pos(numpad_cont, np_x, np_y);
    lv_obj_set_size(numpad_cont, np_w, np_h);
    lv_obj_set_style_text_font(numpad_cont, &lv_font_montserrat_22, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_buttonmatrix_clear_button_ctrl_all(numpad_cont, (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_DISABLED));
    // بک‌اسپیس و تأیید با دکمه‌های واضح جدا روی خانه‌های ۹ و ۱۲ می‌نشینند.
    lv_buttonmatrix_set_button_ctrl(numpad_cont, 9, LV_BUTTONMATRIX_CTRL_HIDDEN);
    lv_buttonmatrix_set_button_ctrl(numpad_cont, 12, LV_BUTTONMATRIX_CTRL_HIDDEN);
    lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0x34C759), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(numpad_cont, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_CHECKED);
    bool count_edit = false;
    if (numpad_ctx == 2) {
      for (int j = 0; j < LIST_MAX; j++) {
        if (AUT.list_edit_mode_count[j]) { count_edit = true; break; }
      }
    }
    if (count_edit) lv_buttonmatrix_set_button_ctrl(numpad_cont, 11, LV_BUTTONMATRIX_CTRL_DISABLED);
  } else {
    lv_obj_set_size(numpad_cont, 176, 236);
    lv_obj_align(numpad_cont, LV_ALIGN_RIGHT_MID, -6, 18);
    lv_obj_set_style_text_font(numpad_cont, &lv_font_montserrat_22, LV_PART_ITEMS | LV_STATE_DEFAULT);
  }
  lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0x243447), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(numpad_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(numpad_cont, lv_color_hex(0x17384d), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0x1976d2), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_radius(numpad_cont, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(numpad_cont, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(numpad_cont, list_kb ? 4 : 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(numpad_cont, list_kb ? 4 : 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(numpad_cont, list_kb ? 6 : 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(numpad_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  list_bs_overlay_sync(list_kb);
}

static void f1_ok_overlay_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (numpad_ctx != 3 && numpad_ctx != 4) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  numpad_commit();
}

static void numpad_style_f1() {
  if (!numpad_cont) return;
  // ---------- کیبورد عددی F1 — +۲۴ راست، +۲۷ پایین نسبت به کارت ----------
  const int np_x = 110, np_y = 144, np_w = 300, np_h = 166;
  const int pad = 8, row_gap = 6, rows = 5;
  const int inner_h = np_h - 2 * pad;
  const int row_h = (inner_h - (rows - 1) * row_gap) / rows;
  const int ok_x = np_x + pad;
  const int ok_y = np_y + pad + (rows - 1) * (row_h + row_gap);
  const int ok_w = np_w - 2 * pad;

  lv_buttonmatrix_set_map(numpad_cont, numpad_map_f1);
  lv_obj_set_size(numpad_cont, np_w, np_h);
  lv_obj_set_parent(numpad_cont, lv_layer_top());
  lv_obj_set_align(numpad_cont, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(numpad_cont, np_x, np_y);
  lv_obj_set_style_text_font(numpad_cont, &lv_font_montserrat_20, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0x1a3144), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(numpad_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(numpad_cont, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(numpad_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(numpad_cont, pad, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(numpad_cont, row_gap, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(numpad_cont, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(numpad_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(numpad_cont, lv_color_hex(0x17384d), LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(numpad_cont, 10, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(numpad_cont, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
  // ردیف آخر ماتریس بی‌رنگ (دکمهٔ واقعی تأیید جداست و روی آن می‌نشیند)
  lv_buttonmatrix_clear_button_ctrl_all(numpad_cont, (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_DISABLED | LV_BUTTONMATRIX_CTRL_HIDDEN));
  // Backspace is destructive: keep its red state permanently visible.
  lv_buttonmatrix_set_button_ctrl(numpad_cont, 11, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_buttonmatrix_set_button_ctrl(numpad_cont, 12, LV_BUTTONMATRIX_CTRL_HIDDEN);
  lv_obj_set_style_bg_color(numpad_cont, lv_color_hex(0xff3b30), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(numpad_cont, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_CHECKED);

  // دکمهٔ سبز جدا با لیبل «تأیید» — مختصات مطلق روی همان ردیف آخر
  if (f1_ok_overlay && !lv_obj_has_class(f1_ok_overlay, &lv_button_class)) {
    lv_obj_delete(f1_ok_overlay);
    f1_ok_overlay = NULL;
  }
  if (!f1_ok_overlay) {
    f1_ok_overlay = lv_button_create(lv_layer_top());
    lv_obj_set_style_bg_color(f1_ok_overlay, lv_color_hex(0x1c9b67), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(f1_ok_overlay, lv_color_hex(0x178a5a), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(f1_ok_overlay, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f1_ok_overlay, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f1_ok_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(f1_ok_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(f1_ok_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(f1_ok_overlay, f1_ok_overlay_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *okl = lv_label_create(f1_ok_overlay);
    lv_obj_set_style_text_font(okl, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(okl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(okl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(okl, -12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(okl, "تأیید");
    lv_obj_center(okl);
  } else {
    lv_obj_set_parent(f1_ok_overlay, lv_layer_top());
  }
  lv_obj_set_pos(f1_ok_overlay, ok_x, ok_y);
  lv_obj_set_size(f1_ok_overlay, ok_w, row_h);
  if (lv_obj_get_child_count(f1_ok_overlay) > 0)
    lv_obj_center(lv_obj_get_child(f1_ok_overlay, 0));
  lv_obj_clear_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(numpad_cont);
  lv_obj_move_foreground(f1_ok_overlay);
}

static void numpad_build() {
  if (numpad_cont) return;
  numpad_cont = lv_buttonmatrix_create(lv_layer_top());
  lv_obj_add_event_cb(numpad_cont, numpad_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
  numpad_style_side();
}

// فقط صفحهٔ تنظیمات + تب F1 (تنظیمات عمومی)
static bool f1_general_settings_active() {
  if (GUI.scr_id != 2) return false;
  if (!objects.tabview2_1) return true;
  return lv_tabview_get_tab_act(objects.tabview2_1) == 0;
}

static bool list_soft_kb_allowed() {
  // تصمیم فقط از تنظیم مؤثر کیبورد تاچ می‌آید؛ مسیر USB و تشخیص آن دست‌نخورده است.
  // حالت AUTO هنگام نبود USB همین virtual_kb_enable را روشن می‌کند.
  return GUI.scr_id == 3 && !OPT.active && !GUI.lock_screen && GUI.virtual_kb_enable;
}

static bool list_edit_active() {
  for (int i = 0; i < LIST_MAX; i++) {
    if (AUT.list_edit_mode_sp[i] || AUT.list_edit_mode_count[i]) return true;
  }
  return false;
}

static void list_soft_kb_ensure_open() {
  if (!list_soft_kb_allowed()) return;
  if (!list_edit_active()) {
    // GUI.list_page = top-row index (not page×ROWS)
    int first = GUI.list_page;
    if (first < 0) first = 0;
    if (first >= LIST_MAX) first = LIST_MAX - 1;
    if (first >= GUI.visible_raws && GUI.visible_raws > 0) first = GUI.visible_raws - 1;
    list_show_row(first);
    begin_screen3_sp_edit(first);
  }
  numpad_open(2);
}

void numpad_sync_screen() {
  if (!numpad_cont || lv_obj_has_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN)) return;
  if (numpad_ctx == 1 && GUI.scr_id == 1 && GUI.virtual_kb_enable) return;
  if (numpad_ctx == 2 && list_soft_kb_allowed()) return;
  if (GUI.virtual_kb_enable && settings_soft_kb_tab_ok()) return;
  // USB/تب دیگر: نامپد لمسی را ببند؛ فوکوس F1 برای USB را نگه دار
  if (numpad_ctx == 3 || numpad_ctx == 4) {
    numpad_hide_soft_widgets();
    return;
  }
  numpad_close();
}

static void list_reflow_compact();  // fwd — برای بازشدن نامپد لیست

void numpad_open(int ctx) {
  f4_kb_hide();
  f2_hide_line_kb();
  // صفحه اصلی: عدد مقصد — لمس → همان کیبرد لیست + تأیید = حرکت
  if (ctx == 1) {
    if (GUI.scr_id != 1) return;
    if (STP.motorIs_moving || STP.moveStepperCMD) {
      GUI.buzzer_cmd_long = true;
      ui_toast("موتور در حال حرکت است", "Motor is moving", 2500);
      return;
    }
    // مثل کیبرد USB: در جاگ / لیست برش / بهینه اول پیغام خروج
    if (GUI.NumLock) {
      request_exit_queue_mode_prompt(3);
      return;
    }
    if (main_queue_mode_active()) {
      request_exit_queue_mode_prompt(1);
      return;
    }
    numpad_ctx = 1;
    clear_buffer_sp_flag = true;
    GUI.edit_mode_lable1_1 = true;
    if (!GUI.virtual_kb_enable) return;  // فقط USB؛ ctx برای تایپ آماده است
    main_numpad_catcher_sync(true);
    numpad_build();
    numpad_style_side();
    lv_obj_clear_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(numpad_cont);
    list_bs_overlay_sync(true);
    return;
  }
  if (ctx == 2) {
    if (!list_soft_kb_allowed()) return;
    numpad_build();
    numpad_ctx = 2;
    numpad_style_side();
    lv_obj_clear_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(numpad_cont);
    // بعد از ctx=2 قوانین ۲ردیفه اعمال شود
    list_reflow_compact();
    list_bs_overlay_sync(true);
    return;
  }
  if (!GUI.virtual_kb_enable) return;
  if (ctx != 3 && ctx != 4) return;
  if (!f1_general_settings_active()) return;
  numpad_build();
  numpad_ctx = ctx;
  numpad_style_f1();
  lv_obj_clear_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(numpad_cont);
  // تأیید باید بالای ماتریس بماند (قبلاً move_foreground نامپد روی OK می‌افتاد)
  if (f1_ok_overlay) {
    lv_obj_clear_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(f1_ok_overlay);
  }
  f1_caret_lit = true;
  f1_caret_blink_ms = millis();
  f1_caret_refresh();
}

//==================== صفحه‌بندی لیست 50تایی ====================
static lv_obj_t *page_lbl = NULL;
static lv_obj_t *btn_page_prev = NULL, *btn_page_next = NULL, *btn_optimize = NULL;
static lv_obj_t *btn_list_clear = NULL, *btn_list_help = NULL, *list_scroll_track = NULL, *list_scroll_thumb = NULL;
static lv_obj_t *list_meter_unit_lbl = NULL, *list_header_line = NULL;
static lv_obj_t *list_initial_count_header_lbl = NULL;
static lv_obj_t *list_help_book_parts[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static lv_obj_t *btn_list_add_round = NULL, *btn_list_remove_round = NULL;
static uint32_t list_clear_next_ms = 0;
static bool list_clear_current_zeroed = false;

int list_rows_on_screen() {
  // فقط وقتی نامپد لیست واقعاً دیده می‌شود → ۲ ردیف؛ ctx گیرکرده/مخفی اسکرول را خراب نکند
  if (GUI.scr_id == 3 && numpad_ctx == 2 && GUI.virtual_kb_enable &&
      numpad_cont && !lv_obj_has_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN)) return 2;
  return LIST_ROWS_PER_PAGE;
}
static int list_max_top() {
  int rows = list_rows_on_screen();
  if (GUI.visible_raws <= rows) return 0;
  return GUI.visible_raws - rows;
}

static void list_page_refresh_flags() {
  GUI.flag_update_labels_3_1 = true;
  GUI.flag_update_labels_3_2 = true;
  GUI.flag_panel3_edit_labels_txt = true;
  GUI.flag_update_panel3_2_x_clr = true;
  GUI.flag_update_page = true;
  GUI.flag_refresh_visible = true;
}

void list_zero_row(int idx) {
  if (idx < 0 || idx >= LIST_MAX) return;
  AUT.list_sp_Val_p[idx] = 0;
  AUT.list_count_Val[idx] = 0;
  AUT.list_count_base_Val[idx] = 0;
  GUI.inputBuffer_L_3_1_x[idx] = "0";
  GUI.inputBuffer_L_3_2_x[idx] = "0";
  // زاویهٔ ردیف هم «خام» شود — ردیف تازه با درجهٔ پیش‌فرض ۴۵ باز شود
  if (AUT.list_angle_Val[idx] != 45) {
    AUT.list_angle_Val[idx] = 45;
    save_permanently_preferences_angle_val(idx, 45);
  }
  AUT.list_edit_mode_sp[idx] = false;
  AUT.list_edit_mode_count[idx] = false;
  opt_notify_list_edited();
}

static void list_start_scroll_motion(int dir) {
  // موج A: انیم lv_anim روی ۱۰ ردیف حذف — فقط snap (صفحه عوض می‌شود؛ بدون هزینهٔ رندر)
  (void)dir;
  for (int i = 0; i < LIST_ROWS_PER_PAGE; i++) {
    lv_obj_t *row = list_row_panel(i);
    if (!row || lv_obj_has_flag(row, LV_OBJ_FLAG_HIDDEN)) continue;
    lv_obj_set_style_translate_y(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

void list_page_change(int dir) {
  int np = GUI.list_page + dir;
  int max_top = list_max_top();
  if (np < 0 || np > max_top) { GUI.buzzer_cmd_long = true; return; }
  cancel_screen3_pending_edit();
  numpad_close();
  GUI.list_page = np;
  list_page_refresh_flags();
  list_start_scroll_motion(dir);
  GUI.buzzer_cmd_short = true;
  ui_resume_checkpoint();
}

void list_show_row(int idx) {
  if (idx < 0) idx = 0;
  if (idx >= LIST_MAX) idx = LIST_MAX - 1;
  int top = GUI.list_page;
  if (idx < top) top = idx;
  {
  int rows = list_rows_on_screen();
  if (idx >= top + rows) top = idx - rows + 1;
  }
  if (top < 0) top = 0;
  int max_top = list_max_top();
  if (top > max_top) top = max_top;
  int old_top = GUI.list_page;
  GUI.list_page = top;
  list_page_refresh_flags();
  if (top != old_top) list_start_scroll_motion((top > old_top) ? +1 : -1);
}

// فقط اگر ردیف زیر کیبورد/پنجره دیده نمی‌شود جابه‌جا کن — کلیک روی عدد مرئی اسکرول نکند
void list_ensure_row_visible(int idx) {
  if (idx < 0) idx = 0;
  if (idx >= LIST_MAX) idx = LIST_MAX - 1;
  int top = GUI.list_page;
  int rows = list_rows_on_screen();
  if (rows < 1) rows = 1;
  if (idx >= top && idx < top + rows) return;
  list_show_row(idx);
}

static void page_btn_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  int dir = (int)(intptr_t)lv_event_get_user_data(e);
  list_page_change(dir);
}

static void list_add_row_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  int next = GUI.visible_raws + 1;
  if (next > LIST_MAX) {
    char fa[80], en[64];
    snprintf(fa, sizeof(fa), "ظرفیت لیست کامل شد؛ بیشتر از %d ردیف نمی‌شود", LIST_MAX);
    snprintf(en, sizeof(en), "List limit reached: %d rows", LIST_MAX);
    ui_toast(fa, en, 3500);
    return;
  }
  int idx = next - 1;
  GUI.buzzer_cmd_short = true;
  list_zero_row(idx);
  GUI.pending_zero_bits[idx >> 3] |= (uint8_t)(1u << (idx & 7));
  GUI.visible_raws = next;
  PEND.visible_raws = next;
  GUI.flag_save_visible_raws = true;
  list_show_row(idx);
}

static void list_remove_row_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  if (GUI.visible_raws <= 0) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  int idx = GUI.visible_raws - 1;
  list_zero_row(idx);
  GUI.pending_zero_bits[idx >> 3] |= (uint8_t)(1u << (idx & 7));
  GUI.visible_raws--;
  PEND.visible_raws = GUI.visible_raws;
  int max_top = list_max_top();
  if (GUI.list_page > max_top) GUI.list_page = max_top;
  GUI.flag_save_visible_raws = true;
  if (GUI.visible_raws > 0) list_show_row(GUI.visible_raws - 1);
  list_page_refresh_flags();
  list_start_scroll_motion(+1);
}

// سوایپ عمدی با آستانهٔ بالا — سوایپ کوچک/اسکرول تصادفی صفحه را عوض نکند
// موس: فقط وقتی دکمه چپ پایین است PRESSING می‌آید → درگ=اسکرول؛ حرکت آزاد نشانگر اسکرول نمی‌کند
static lv_point_t list_press_pt;
static bool list_press_valid = false;
static bool list_press_moved = false;
static bool list_edge_bounce = false;
static bool ui_help_dragged = false;  // درگ انگشت/موس → راهنمای ۲ثانیه باطل

bool list_ui_was_dragged(void) {
  return list_press_moved;
}

static void list_bounce_reset() {
  if (!list_edge_bounce || !objects.panel3_1) return;
  lv_obj_set_y(objects.panel3_1, 88);  // هم‌تراز list_reflow_compact panel y
  list_edge_bounce = false;
}
static void list_swipe_cb(lv_event_t *e) {
  if (GUI.lock_screen || OPT.active || GUI.scr_id != 3) return;
  lv_event_code_t code = lv_event_get_code(e);
  // بعد از USB، lv_indev_active() ممکن است موس/کیبورد بماند → نقطهٔ تاچ غلط → اسکرول می‌میرد
  lv_indev_t *indev = lv_event_get_indev(e);
  if (!indev) indev = lv_indev_active();
  if (!indev) return;
  lv_point_t p;
  lv_indev_get_point(indev, &p);
  if (code == LV_EVENT_PRESSED) {
    list_press_pt = p;
    list_press_valid = true;
    list_press_moved = false;
    ui_help_dragged = false;
    return;
  }
  if (code == LV_EVENT_PRESSING && list_press_valid) {
    int dx = (int)p.x - (int)list_press_pt.x;
    int dy = (int)p.y - (int)list_press_pt.y;
  if (LV_ABS(dy) >= 14 && LV_ABS(dy) >= LV_ABS(dx)) {
      // FORCE_ON+USB: نامپد باز ناحیهٔ سوایپ را می‌گیرد → اول ببند تا ۵ ردیف برگردد
      if (numpad_ctx == 2) numpad_close();
      int steps = LV_ABS(dy) / 14;
      int dir = (dy < 0) ? +1 : -1;
      int top = GUI.list_page + (dir * steps);
      int max_top = list_max_top();
      if (top < 0) top = 0;
      if (top > max_top) top = max_top;
      if (top != GUI.list_page) {
        list_bounce_reset();
        int old_top = GUI.list_page;
        GUI.list_page = top;
        list_press_moved = true;
        ui_help_dragged = true;  // درگ اسکرول → راهنمای ۲ثانیه نیاید
        if (indev) lv_indev_reset_long_press(indev);
        GUI.flag_update_page = true;
        list_page_refresh_flags();
        list_start_scroll_motion((top > old_top) ? +1 : -1);
      } else if (objects.panel3_1 && max_top > 0) {
        if (GUI.list_page == max_top && dy < 0) {
          lv_obj_set_y(objects.panel3_1, 82);   // base 88 − 6
          list_edge_bounce = true;
          list_press_moved = true;
          ui_help_dragged = true;
          if (indev) lv_indev_reset_long_press(indev);
        } else if (GUI.list_page == 0 && dy > 0) {
          lv_obj_set_y(objects.panel3_1, 94);  // base 88 + 6
          list_edge_bounce = true;
          list_press_moved = true;
          ui_help_dragged = true;
          if (indev) lv_indev_reset_long_press(indev);
        }
      }
      list_press_pt = p;
    }
    return;
  }
  if (code != LV_EVENT_RELEASED || !list_press_valid) return;
  list_press_valid = false;
  list_bounce_reset();
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  if (list_press_moved) return;
  int dx = (int)p.x - (int)list_press_pt.x;
  int dy = (int)p.y - (int)list_press_pt.y;
  // لمس کوتاه روی اعداد/ردیف: کیبورد را اینجا نبند (با CLICK روی اعداد می‌جنگد)
  // بستن کیبورد فقط با list_blank_dismiss_kb_cb
  if (LV_ABS(dy) >= 14 && LV_ABS(dy) >= LV_ABS(dx)) {
    list_page_change((dy < 0) ? +1 : -1);
  }
}

void prcs_update_page() {
  if (!GUI.flag_update_page) return;
  GUI.flag_update_page = false;
  if (page_lbl) lv_obj_add_flag(page_lbl, LV_OBJ_FLAG_HIDDEN);
  if (btn_page_prev) lv_obj_add_flag(btn_page_prev, LV_OBJ_FLAG_HIDDEN);
  if (btn_page_next) lv_obj_add_flag(btn_page_next, LV_OBJ_FLAG_HIDDEN);
  if (list_scroll_track && list_scroll_thumb) {
    int max_top = list_max_top();
    if (max_top <= 0) {
      lv_obj_add_flag(list_scroll_track, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    lv_obj_clear_flag(list_scroll_track, LV_OBJ_FLAG_HIDDEN);
    int track_h = 202;
    int thumb_h = (LIST_ROWS_PER_PAGE * track_h) / GUI.visible_raws;
    if (thumb_h < 36) thumb_h = 36;
    if (thumb_h > track_h) thumb_h = track_h;
    int y = (GUI.list_page * (track_h - thumb_h)) / max_top;
    lv_obj_set_size(list_scroll_thumb, 4, thumb_h);
    lv_obj_set_pos(list_scroll_thumb, 0, y);
  }
}

//==================== موتور بهینه‌سازی برش (FFD — First Fit Decreasing) ====================
static lv_obj_t *opt_cont = NULL;
static lv_obj_t *opt_lbl_summary = NULL;
static lv_obj_t *opt_lbl_bar_title = NULL;
static lv_obj_t *opt_lbl_bar_value = NULL;
static lv_obj_t *opt_lbl_cut_title = NULL;
static lv_obj_t *opt_lbl_cut_value = NULL;
static lv_obj_t *opt_lbl_current = NULL;
static lv_obj_t *opt_list_body = NULL;
static lv_obj_t *opt_bar_rows[OPT_RENDER_ROWS];
static lv_obj_t *opt_badge_angle[OPT_RENDER_ROWS];    // بج زاویه سمت چپ شاخه (45° / 90°)
static lv_obj_t *opt_badge_bar[OPT_RENDER_ROWS];
static lv_obj_t *opt_badge_waste[OPT_RENDER_ROWS];
static lv_obj_t *opt_badge_waste_num[OPT_RENDER_ROWS];  // عدد پرت = Montserrat (نه naz → مربع)
static lv_obj_t *opt_badge_bar_num[OPT_RENDER_ROWS];    // رقم شاخه = Montserrat
static lv_obj_t *opt_badge_bar_xmark[OPT_RENDER_ROWS];  // ضربدر خاکستری روی شاخهٔ تمام‌شده
static lv_obj_t *opt_piece_done_lbl[OPT_RENDER_ROWS];
static lv_obj_t *opt_piece_current_lbl[OPT_RENDER_ROWS];
static lv_obj_t *opt_piece_lbl[OPT_RENDER_ROWS];
static lv_obj_t *opt_done_box = NULL;   // کادر ابعاد برش‌خورده (اول لیست)
static lv_obj_t *opt_done_lbl = NULL;
static lv_obj_t *opt_done_body = NULL;  // بدنهٔ گروهی شاخه/اعداد
static uint32_t opt_saved_list_fp = 0;
static bool opt_plan_ready = false;
static volatile bool opt_rebuild_req = false;  // تغییر شاخه/kerf از روی پنل → بازچینی در Task2
static volatile bool opt_list_edit_dirty = false;  // ویرایش سایز/تعداد لیست → بازچینی از نو (لاگ برش می‌ماند)
static volatile bool opt_home_preserve_active = false;  // خانه از بهینه: مستقیم main، بدون خاموش‌کردن مود
static volatile bool opt_remote_open_req = false;
static int opt_bar_page_start = 0;  // شروع صفحهٔ مجازی (۰، ۲۰، ۴۰، …)
static lv_obj_t *opt_bar_page_lbl = NULL;
static lv_obj_t *opt_bar_page_prev_btn = NULL;
static lv_obj_t *opt_bar_page_next_btn = NULL;
static uint32_t opt_bar_page_guard_ms = 0;  // جلوگیری از حلقهٔ SCROLL_END بعد از عوض‌صفحه
static bool opt_skip_next_scroll_end = false;  // بعد از scroll برنامه‌ای یک‌بار نادیده بگیر
// کش رندر ردیف‌ها: بار/cur/لاگ/نسل یکسان → رندر قبلی معتبر (کاهش لگ اسکرول)
static int16_t opt_row_last_bar[OPT_RENDER_ROWS] = { -1 };
static int16_t opt_row_last_cur[OPT_RENDER_ROWS] = { -1 };
static int16_t opt_row_last_log[OPT_RENDER_ROWS] = { -1 };
static uint8_t opt_render_gen = 0;  // بازچینی برنامه = محتوای شاخه‌ها عوض شده
static uint8_t opt_row_last_gen[OPT_RENDER_ROWS] = { 0 };
// لمس ثابت روی «شاخه» — اسکرول/کشیدن نباید تأیید شود
static bool opt_tap_armed = false;
static int32_t opt_tap_scroll_y0 = 0;
static lv_point_t opt_tap_pt0 = {0, 0};
static int opt_last_loaded_bar = -1;
static lv_obj_t *opt_main_bar_lbl = NULL;  // کانتینر سبز شاخه
static lv_obj_t *opt_main_bar_title = NULL;
static lv_obj_t *opt_main_bar_num = NULL;

struct OptCalcPiece { float len; uint8_t entry; uint8_t angle; };
static_assert(sizeof(OptCalcPiece) == 8, "Unexpected OptCalcPiece layout");
struct OptCalcScratch {
  OptCalcPiece tmp[OPT_MAX_PIECES];
  float bar_used[OPT_MAX_BARS];
  uint8_t bar_angle[OPT_MAX_BARS];
  uint16_t bar_of_tmp[OPT_MAX_PIECES];
  int32_t bar_cursor[OPT_MAX_BARS];
  float input_len[LIST_MAX];
  uint16_t input_count[LIST_MAX];
  uint8_t input_angle[LIST_MAX];
  // خروجی تا پایان محاسبه داخل PSRAM می‌ماند؛ لغو/timeout نباید برنامهٔ
  // فعال قبلی را با آرایه‌های نیمه‌نوشته مخلوط کند.
  float final_len[OPT_MAX_PIECES];
  uint8_t final_entry[OPT_MAX_PIECES];
  uint16_t final_bar[OPT_MAX_PIECES];
};
static OptCalcScratch *opt_calc_scratch = NULL;
#define opt_calc_tmp         (opt_calc_scratch->tmp)
#define opt_calc_bar_used    (opt_calc_scratch->bar_used)
#define opt_calc_bar_angle   (opt_calc_scratch->bar_angle)
#define opt_calc_bar_of_tmp  (opt_calc_scratch->bar_of_tmp)
#define opt_calc_bar_cursor  (opt_calc_scratch->bar_cursor)
#define opt_calc_input_len   (opt_calc_scratch->input_len)
#define opt_calc_input_count (opt_calc_scratch->input_count)
#define opt_calc_input_angle (opt_calc_scratch->input_angle)
#define opt_calc_final_len   (opt_calc_scratch->final_len)
#define opt_calc_final_entry (opt_calc_scratch->final_entry)
#define opt_calc_final_bar   (opt_calc_scratch->final_bar)
#define opt_piece_done       (OPT.piece_done)
static lv_obj_t *opt_calc_box = NULL;
static lv_obj_t *opt_calc_status = NULL;
static lv_obj_t *opt_calc_bar = NULL;   // نوار پیشرفت زیر درصد (درخواست مالک)
static bool opt_calc_active = false;
static uint8_t opt_calc_stage = 0;
static int opt_calc_limit = 0;
static int opt_calc_row = 0;
static int opt_calc_row_remaining = 0;
static float opt_calc_row_len = 0;
static uint8_t opt_calc_row_entry = 0;
static uint8_t opt_calc_row_ang = 45;
static int opt_calc_n = 0;
static int opt_calc_total = 0;
static int opt_calc_place_i = 0;
static int opt_calc_scan_b = 0;
static int opt_calc_bars = 0;
static int opt_calc_group_b = 0;
static int opt_calc_group_i = 0;
static int opt_calc_k = 0;
static bool opt_calc_overflow = false;
static float opt_calc_usable = 0;
static float opt_calc_kerf = 0;
static uint32_t opt_calc_input_fp = 0;
static uint32_t opt_calc_start_ms = 0;
static uint32_t opt_calc_last_ui_ms = 0;
static bool opt_calc_stage5_prep = false;
// بازهٔ پیوستهٔ قطعات هر شاخه — جلوگیری از اسکن O(bars×pieces) هنگام رندر ۲۰ ردیف
static int16_t opt_bar_pi0[OPT_MAX_BARS];
static int16_t opt_bar_pn[OPT_MAX_BARS];

static void opt_rebuild_bar_ranges() {
  const int bc = OPT.bar_count;
  for (int b = 0; b < bc && b < OPT_MAX_BARS; b++) {
    opt_bar_pi0[b] = -1;
    opt_bar_pn[b] = 0;
  }
  for (int i = 0; i < OPT.piece_count; i++) {
    int b = (int)OPT.piece_bar[i];
    if (b < 0 || b >= OPT_MAX_BARS || b >= bc) continue;
    if (opt_bar_pi0[b] < 0) opt_bar_pi0[b] = (int16_t)i;
    opt_bar_pn[b]++;
  }

  // چیدمان زیک‌زاک صرفه‌جویی زمان: شاخه زوج صعودی، شاخه فرد نزولی
  for (int b = 0; b < bc && b < OPT_MAX_BARS; b++) {
    int pi0 = opt_bar_pi0[b];
    int pn = opt_bar_pn[b];
    if (pi0 < 0 || pn <= 1) continue;
    const bool ascending = (b % 2 == 0);
    for (int i = pi0; i < pi0 + pn - 1; i++) {
      for (int j = i + 1; j < pi0 + pn; j++) {
        bool should_swap = ascending ? (OPT.piece_len[i] > OPT.piece_len[j])
                                     : (OPT.piece_len[i] < OPT.piece_len[j]);
        if (should_swap) {
          float tmp_len = OPT.piece_len[i];
          OPT.piece_len[i] = OPT.piece_len[j];
          OPT.piece_len[j] = tmp_len;

          uint16_t tmp_entry = OPT.piece_entry[i];
          OPT.piece_entry[i] = OPT.piece_entry[j];
          OPT.piece_entry[j] = tmp_entry;
        }
      }
    }
  }
}

static void opt_calc_close_modal() {
  if (opt_calc_box && lv_obj_is_valid(opt_calc_box)) lv_obj_delete(opt_calc_box);
  opt_calc_box = NULL;
  opt_calc_status = NULL;
}

static void opt_calc_open_modal();   // fwd
static void opt_calc_update_modal(int percent);

static bool opt_calc_alloc_scratch() {
  if (opt_calc_scratch) return true;
  opt_calc_scratch = (OptCalcScratch *)heap_caps_malloc(
      sizeof(OptCalcScratch), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!opt_calc_scratch) {
    Serial.printf("[OPT] PSRAM scratch alloc failed bytes=%u free_psram=%u\n",
                  (unsigned)sizeof(OptCalcScratch), (unsigned)ESP.getFreePsram());
    return false;
  }
  Serial.printf("[OPT] scratch=%uB PSRAM free=%u\n",
                (unsigned)sizeof(OptCalcScratch), (unsigned)ESP.getFreePsram());
  return true;
}

static void opt_calc_free_scratch() {
  if (!opt_calc_scratch) return;
  heap_caps_free(opt_calc_scratch);
  opt_calc_scratch = NULL;
}

static void opt_calc_reset() {
  opt_calc_active = false;
  opt_calc_stage = 0;
  opt_calc_row = 0;
  opt_calc_row_remaining = 0;
  opt_calc_n = 0;
  opt_calc_total = 0;
  opt_calc_place_i = 0;
  opt_calc_scan_b = 0;
  opt_calc_bars = 0;
  opt_calc_group_b = 0;
  opt_calc_group_i = 0;
  opt_calc_k = 0;
  opt_calc_overflow = false;
  opt_calc_kerf = 0;
  opt_calc_input_fp = 0;
  opt_calc_stage5_prep = false;
  opt_calc_free_scratch();
  opt_calc_close_modal();
}

static bool opt_calc_reset_cut_log = true;
static uint32_t opt_list_fingerprint();

static void opt_calc_fail(const char *fa, const char *en) {
  opt_calc_reset();
  GUI.lock_screen = false;
  if (fa && en) ui_toast(fa, en, 4000);
}

static bool opt_calc_begin(bool reset_cut_log) {
  if (opt_calc_active) return true;
  int limit = 0;
  for (int i = 0; i < LIST_MAX; i++) {
    if (AUT.list_sp_Val_p[i] > 0.05f && AUT.list_count_Val[i] > 0) limit = i + 1;
  }
  if (GUI.visible_raws > limit) limit = (GUI.visible_raws < LIST_MAX) ? GUI.visible_raws : LIST_MAX;
  int64_t requested = 0;
  for (int i = 0; i < limit; i++) {
    if (AUT.list_sp_Val_p[i] > 0.05f && AUT.list_count_Val[i] > 0)
      requested += (int64_t)AUT.list_count_Val[i];
  }
  if (requested <= 0) {
    ui_toast("لیست خالی است. متراژ و تعداد وارد کنید", "List is empty. Enter length and quantity", 3500);
    return false;
  }
  if (requested > (int64_t)OPT_USER_MAX_PIECES) {
    char fa[96], en[88];
    snprintf(fa, sizeof(fa), "حداکثر %d قطعه مجاز است (مجموع تعداد)", OPT_USER_MAX_PIECES);
    snprintf(en, sizeof(en), "Max %d pieces allowed (total qty)", OPT_USER_MAX_PIECES);
    ui_toast(fa, en, 4000);
    return false;
  }
  float usable = (float)(stock_len_mm - trim_mm);
  if (usable < 100) {
    ui_toast("طول مفید شاخه خیلی کم است (شاخه - پیش برش)", "Usable stock too small (stock - trim)", 3500);
    return false;
  }
  for (int i = 0; i < limit; i++) {
    float len = AUT.list_sp_Val_p[i];
    int cnt = AUT.list_count_Val[i];
    if (len <= 0 || cnt <= 0) continue;
    if (len + kerf_mm > usable) {
      char fa[80], en[80];
      snprintf(fa, sizeof(fa), "طول ردیف %d از شاخه بزرگتر است", i + 1);
      snprintf(en, sizeof(en), "Row %d longer than stock bar", i + 1);
      ui_toast(fa, en, 4000);
      return false;
    }
  }
  opt_calc_reset();
  if (!opt_calc_alloc_scratch()) {
    ui_toast("حافظه بهینه ساز کافی نیست",
             "Not enough optimizer memory", 4000);
    return false;
  }
  // Immutable input snapshot: web/serial/UI changes during a long 5000-piece
  // calculation cannot mix old lengths with new counts or a new kerf.
  for (int i = 0; i < LIST_MAX; i++) {
    opt_calc_input_len[i] = AUT.list_sp_Val_p[i];
    opt_calc_input_count[i] =
        (uint16_t)constrain(AUT.list_count_Val[i], 0, 999);
    uint8_t a = AUT.list_angle_Val[i];
    if (a != 90) a = 45;
    opt_calc_input_angle[i] = a;
  }
  opt_calc_kerf = kerf_mm;
  opt_calc_input_fp = opt_list_fingerprint();
  opt_calc_reset_cut_log = reset_cut_log;
  opt_calc_limit = limit;
  opt_calc_usable = usable;
  opt_calc_total = (int)requested;
  opt_calc_n = 0;
  opt_calc_row = 0;
  opt_calc_row_remaining = 0;
  opt_calc_place_i = 0;
  opt_calc_scan_b = 0;
  opt_calc_bars = 0;
  opt_calc_group_b = 0;
  opt_calc_group_i = 0;
  opt_calc_k = 0;
  opt_calc_overflow = false;
  opt_calc_active = true;
  opt_calc_stage = 1;
  opt_calc_start_ms = millis();
  opt_calc_last_ui_ms = 0;
  GUI.lock_screen = true;
  opt_calc_open_modal();
  opt_calc_update_modal(2);
  return true;
}

static lv_obj_t *opt_main_ang_lbl = NULL;
static lv_obj_t *opt_main_ang_title = NULL;
static lv_obj_t *opt_main_ang_val = NULL;

static void opt_main_ang_click_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    if (opt_main_ang_lbl && lv_obj_is_valid(opt_main_ang_lbl)) {
      lv_obj_set_style_transform_scale(opt_main_ang_lbl, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  } else if (code == LV_EVENT_RELEASED) {
    if (opt_main_ang_lbl && lv_obj_is_valid(opt_main_ang_lbl)) {
      lv_obj_set_style_transform_scale(opt_main_ang_lbl, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    GUI.buzzer_cmd_short = true;
    if (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX && !OPT.active) {
      int gidx = AUT.selected_count_val;
      uint8_t new_ang = (AUT.list_angle_Val[gidx] == 90) ? 45 : 90;
      AUT.list_angle_Val[gidx] = new_ang;
      save_permanently_preferences_angle_val(gidx, new_ang);
      opt_main_bar_lbl_sync();
      if (new_ang == 90) ui_toast("زاویه: ۹۰ درجه", "Angle: 90 deg", 1000);
      else ui_toast("زاویه: ۴۵ درجه", "Angle: 45 deg", 1000);
    }
  }
}

void opt_main_bar_lbl_hide() {
  if (opt_main_bar_lbl && lv_obj_is_valid(opt_main_bar_lbl))
    lv_obj_add_flag(opt_main_bar_lbl, LV_OBJ_FLAG_HIDDEN);
  if (opt_main_ang_lbl && lv_obj_is_valid(opt_main_ang_lbl))
    lv_obj_add_flag(opt_main_ang_lbl, LV_OBJ_FLAG_HIDDEN);
  // کادر درجه مخفی → برچسب mm به پیش‌فرضِ واقعی UI تولیدی (364,56) برگردد
  if (objects.obj4 && lv_obj_is_valid(objects.obj4))
    lv_obj_set_pos(objects.obj4, 364, 56);
}

// کادر قرمز «حالت / دستی» — جای کادر شاخه، در جاگ کیبرد یا حرکت جویستیک
static lv_obj_t *jog_manual_badge = NULL;
static lv_obj_t *jog_manual_title = NULL;
static lv_obj_t *jog_manual_sub = NULL;
static bool jog_manual_badge_last_show = false;
// مثلث آبی «کم»: قاعده پایین، ضلع راست عمودی، وتر چپ صاف — بالای کادر قرمز دستی
// روی layer_top تا وسط جاگ/جزئی‌رندر گم نشود
// مالک: فیدبک بصری باید با سرعت ÷۶ یکی باشد؛ وتر پله‌ای قبول نیست؛ لیبل «کم» پایین کادر (offset y=+1 ≈ ۲px پایین‌تر از قبل)
// پروفایل: docs/OWNER_PROFILE_FA.md — جاگ+کلید۲
static lv_obj_t *jog_fine_badge = NULL;
static lv_obj_t *jog_fine_lbl = NULL;
static bool jog_fine_badge_last_show = false;
static int8_t jog_fine_badge_last_lang = -1;

static void jog_fine_badge_hide() {
  jog_fine_badge_last_show = false;
  if (jog_fine_badge && lv_obj_is_valid(jog_fine_badge))
    lv_obj_add_flag(jog_fine_badge, LV_OBJ_FLAG_HIDDEN);
}

static void jog_fine_tri_draw_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  if (!obj || !layer) return;
  lv_area_t a;
  lv_obj_get_coords(obj, &a);
  // گوشهٔ راست‌بالا، راست‌پایین، چپ‌پایین → وتر صاف سمت چپ
  lv_draw_triangle_dsc_t dsc;
  lv_draw_triangle_dsc_init(&dsc);
  dsc.p[0].x = a.x2;
  dsc.p[0].y = a.y1;
  dsc.p[1].x = a.x2;
  dsc.p[1].y = a.y2;
  dsc.p[2].x = a.x1;
  dsc.p[2].y = a.y2;
  dsc.color = lv_color_hex(0x0A84FF);
  dsc.opa = LV_OPA_COVER;
  lv_draw_triangle(layer, &dsc);
}

static void jog_fine_badge_sync(bool show) {
  if (!show) {
    if (jog_fine_badge_last_show) jog_fine_badge_hide();
    return;
  }
  if (GUI.scr_id != 1) {
    jog_fine_badge_hide();
    return;
  }
  // هم‌تراز لبهٔ راست کادر قرمز (x=64 w=43 → راست=107) — ۴px از چپ جمع‌شده
  const int bar_w = 39;
  const int bar_x = 68;
  const int tri_h = 24;
  const int tri_y = 45 - tri_h - 2;
  lv_obj_t *parent = lv_layer_top();
  if (!parent) return;
  const int8_t lang = (GUI.system_language == 1) ? 1 : 0;
  if (jog_fine_badge_last_show &&
      jog_fine_badge_last_lang == lang &&
      jog_fine_badge && lv_obj_is_valid(jog_fine_badge) &&
      lv_obj_get_parent(jog_fine_badge) == parent &&
      !lv_obj_has_flag(jog_fine_badge, LV_OBJ_FLAG_HIDDEN)) {
    return;
  }
  if (!jog_fine_badge || !lv_obj_is_valid(jog_fine_badge) ||
      lv_obj_get_parent(jog_fine_badge) != parent) {
    jog_fine_lbl = NULL;
    jog_fine_badge = lv_obj_create(parent);
    lv_obj_set_size(jog_fine_badge, bar_w, tri_h);
    lv_obj_set_style_bg_opa(jog_fine_badge, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(jog_fine_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(jog_fine_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(jog_fine_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(jog_fine_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(jog_fine_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(jog_fine_badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(jog_fine_badge, jog_fine_tri_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    jog_fine_lbl = lv_label_create(jog_fine_badge);
    lv_obj_set_width(jog_fine_lbl, bar_w - 4);
    lv_obj_set_style_text_color(jog_fine_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(jog_fine_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_obj_set_pos(jog_fine_badge, bar_x, tri_y);
  lv_obj_set_size(jog_fine_badge, bar_w, tri_h);
  const bool fa = (lang == 1);
  if (jog_fine_lbl) {
    lv_obj_set_style_text_font(jog_fine_lbl, fa ? &ui_font_naz18b : &lv_font_montserrat_14,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(jog_fine_lbl, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(jog_fine_lbl, fa ? "کم" : "LO");
    // ۲px پایین‌تر از قبل (قبلاً y=-1)
    lv_obj_align(jog_fine_lbl, LV_ALIGN_BOTTOM_RIGHT, -1, 1);
  }
  lv_obj_clear_flag(jog_fine_badge, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(jog_fine_badge);
  lv_obj_invalidate(jog_fine_badge);
  jog_fine_badge_last_show = true;
  jog_fine_badge_last_lang = lang;
}

void jog_manual_badge_hide() {
  jog_manual_badge_last_show = false;
  if (jog_manual_badge && lv_obj_is_valid(jog_manual_badge))
    lv_obj_add_flag(jog_manual_badge, LV_OBJ_FLAG_HIDDEN);
  jog_fine_badge_hide();
}

static lv_obj_t *blade_err_badge = NULL;
static lv_obj_t *blade_err_badge_lbl = NULL;
static bool blade_err_long_pressed = false;

static void blade_err_badge_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_LONG_PRESSED) {
    if (!blade_err_enable || GUI.scr_id != 1 || GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    blade_err_long_pressed = true;
    numpad_ctx = 5;
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%.1f", (double)blade_err_mm);
    settings_num_buf = tmp;
    settings_clear_on_type = true;
    if (GUI.virtual_kb_enable) {
      numpad_build();
      numpad_style_side();
      if (numpad_cont) {
        lv_obj_clear_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(numpad_cont);
      }
    }
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    blade_err_badge_sync();
    return;
  }
  if (code != LV_EVENT_RELEASED) return;
  if (blade_err_long_pressed) {
    blade_err_long_pressed = false;
    return;
  }
  if (numpad_ctx == 5) {
    numpad_commit();
    return;
  }
  if (!blade_err_enable || GUI.scr_id != 1 || GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (STP.motorIs_moving || STP.moveStepperCMD || STP.flag_is_cutting) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  blade_err_armed = !blade_err_armed;
  STP.position_Setpoint_p = blade_err_motor_target_p(STP.Man_Sp_mm);
  AUT.in_position_flag = false;
  STP.in_position_flag = false;
  GUI.buzzer_cmd_short = true;
  blade_err_badge_sync();
}

// خاموش‌کردن اصلاح فقط آفست موتور را صفر می‌کند؛ مقصد UI هرگز تغییر نمی‌کند.
void blade_err_disarm_restore_destination() {
  blade_err_armed = false;
  STP.position_Setpoint_p = blade_err_motor_target_p(STP.Man_Sp_mm);
  AUT.in_position_flag = false;
  STP.in_position_flag = false;
  blade_err_badge_sync();
}

void blade_err_badge_sync() {
  if (!objects.main_1) return;
  if (!blade_err_enable || GUI.scr_id != 1) {
    if (blade_err_badge && lv_obj_is_valid(blade_err_badge))
      lv_obj_add_flag(blade_err_badge, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (!blade_err_badge || !lv_obj_is_valid(blade_err_badge) ||
      lv_obj_get_parent(blade_err_badge) != objects.main_1) {
    if (blade_err_badge && lv_obj_is_valid(blade_err_badge)) lv_obj_delete(blade_err_badge);
    blade_err_badge = lv_obj_create(objects.main_1);
    lv_obj_set_size(blade_err_badge, 44, 44);
    lv_obj_set_style_radius(blade_err_badge, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(blade_err_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(blade_err_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(blade_err_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(blade_err_badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(blade_err_badge, blade_err_badge_cb, LV_EVENT_ALL, NULL);
    blade_err_badge_lbl = lv_label_create(blade_err_badge);
    lv_obj_set_style_text_align(blade_err_badge_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(blade_err_badge_lbl, 44);
    lv_obj_clear_flag(blade_err_badge_lbl, LV_OBJ_FLAG_CLICKABLE);
  }
  // کمی پایین‌تر و راست‌تر
  lv_obj_set_pos(blade_err_badge, 430, 272);
  lv_obj_set_size(blade_err_badge, 44, 44);
  if (numpad_ctx == 5) {
    // در حال ویرایش با کیبورد / نشانگر فعال
    lv_obj_set_style_bg_color(blade_err_badge, lv_color_hex(0x003A70), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(blade_err_badge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(blade_err_badge, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(blade_err_badge, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if (blade_err_armed) {
    // فعال: آبی روشن + حلقه سفید
    lv_obj_set_style_bg_color(blade_err_badge, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(blade_err_badge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(blade_err_badge, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(blade_err_badge, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    // خاموشِ اعمال: خاکستری تیره
    lv_obj_set_style_bg_color(blade_err_badge, lv_color_hex(0x3a3a3c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(blade_err_badge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(blade_err_badge, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(blade_err_badge, lv_color_hex(0x636366), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (blade_err_badge_lbl) {
    if (numpad_ctx == 5) {
      const char *txt = settings_num_buf.length() ? settings_num_buf.c_str() : "0";
      lv_label_set_text(blade_err_badge_lbl, txt);
      lv_obj_set_style_text_font(blade_err_badge_lbl, &lv_font_montserrat_18, 0);
      lv_obj_set_style_text_color(blade_err_badge_lbl, lv_color_hex(0xFFD60A), 0);
      lv_obj_set_style_text_opa(blade_err_badge_lbl, LV_OPA_COVER, 0);
      lv_obj_center(blade_err_badge_lbl);
    } else {
      char b[8];
      float r = roundf(blade_err_mm * 10.0f) / 10.0f;
      if (r > 9.9f) r = 9.9f;
      if (r < -9.9f) r = -9.9f;
      if (fabsf(r) < 0.1f) r = (r < 0.0f) ? -0.1f : 0.1f;
      if (fabsf(r - roundf(r)) < 0.05f)
        snprintf(b, sizeof(b), "%d", (int)lroundf(r));
      else
        snprintf(b, sizeof(b), "%.1f", (double)r);
      lv_label_set_text(blade_err_badge_lbl, b);
      lv_obj_set_style_text_font(blade_err_badge_lbl, &lv_font_montserrat_18, 0);
      lv_obj_set_style_text_color(blade_err_badge_lbl, lv_color_hex(0xffffff), 0);
      lv_obj_set_style_text_opa(blade_err_badge_lbl,
                                blade_err_armed ? LV_OPA_COVER : LV_OPA_70, 0);
      lv_obj_center(blade_err_badge_lbl);
    }
  }
  lv_obj_clear_flag(blade_err_badge, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(blade_err_badge);
}

void jog_manual_badge_invalidate() {
  // بعد از loadScreen اشاره‌گر قبلی باطل است
  jog_manual_badge = NULL;
  jog_manual_title = NULL;
  jog_manual_sub = NULL;
  jog_manual_badge_last_show = false;
  if (jog_fine_badge && lv_obj_is_valid(jog_fine_badge))
    lv_obj_add_flag(jog_fine_badge, LV_OBJ_FLAG_HIDDEN);
  jog_fine_badge = NULL;
  jog_fine_lbl = NULL;
  jog_fine_badge_last_show = false;
  jog_fine_badge_last_lang = -1;
}

void jog_manual_badge_sync() {
  if (!objects.main_1 || GUI.scr_id != 1) {
    jog_manual_badge_hide();
    return;
  }
  const bool show = GUI.NumLock;
  if (!show) {
    if (jog_manual_badge_last_show) jog_manual_badge_hide();
    else jog_fine_badge_hide();
    return;
  }
  // اگر صفحه عوض شده، آبجکت قدیمی را دور بینداز
  if (jog_manual_badge && lv_obj_is_valid(jog_manual_badge) &&
      lv_obj_get_parent(jog_manual_badge) != objects.main_1) {
    jog_manual_badge_invalidate();
  }
  if (jog_manual_badge && !lv_obj_is_valid(jog_manual_badge)) {
    jog_manual_badge_invalidate();
  }
  const bool need_rebuild = !(jog_manual_badge_last_show && jog_manual_badge &&
                              lv_obj_is_valid(jog_manual_badge) &&
                              !lv_obj_has_flag(jog_manual_badge, LV_OBJ_FLAG_HIDDEN));
  if (need_rebuild) {
  jog_manual_badge_last_show = true;
  opt_main_bar_lbl_hide();  // با کادر شاخه هم‌جا نشود
  // دقیقاً همان ابعاد/جای کادر «شاخه»
  const int bar_w = 43;
  const int bar_x = 64;
  if (!jog_manual_badge || !lv_obj_is_valid(jog_manual_badge)) {
    jog_manual_badge = lv_obj_create(objects.main_1);
    lv_obj_set_size(jog_manual_badge, bar_w, 44);
    lv_obj_set_style_radius(jog_manual_badge, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(jog_manual_badge, lv_color_hex(0xB90000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(jog_manual_badge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(jog_manual_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(jog_manual_badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(jog_manual_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(jog_manual_badge, LV_OBJ_FLAG_CLICKABLE);

    jog_manual_title = lv_label_create(jog_manual_badge);
    lv_obj_set_width(jog_manual_title, bar_w);
    lv_obj_set_style_text_color(jog_manual_title, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(jog_manual_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    jog_manual_sub = lv_label_create(jog_manual_badge);
    lv_obj_set_width(jog_manual_sub, bar_w);
    lv_obj_set_style_text_color(jog_manual_sub, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(jog_manual_sub, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_obj_set_pos(jog_manual_badge, bar_x, 42);  // 45 → 42: دقیقاً ۳px بالا
  lv_obj_set_size(jog_manual_badge, bar_w, 44);
  lv_obj_set_style_bg_color(jog_manual_badge, lv_color_hex(0xB90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  const bool fa = (GUI.system_language == 1);
  if (jog_manual_title) {
    lv_obj_set_style_text_font(jog_manual_title, fa ? &ui_font_naz18b : &lv_font_montserrat_14,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(jog_manual_title, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(jog_manual_title, fa ? "حالت" : "Mode");
    lv_obj_set_pos(jog_manual_title, 0, -1);
  }
  if (jog_manual_sub) {
    // EN: Manual — montserrat_12 (۸ خیلی کوچک بود)
    lv_obj_set_style_text_font(jog_manual_sub, fa ? &ui_font_naz18b : &lv_font_montserrat_12,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(jog_manual_sub, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(jog_manual_sub, fa ? "دستی" : "Manual");
    lv_obj_set_pos(jog_manual_sub, 0, fa ? 17 : 18);
  }
  lv_obj_clear_flag(jog_manual_badge, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(jog_manual_badge);
  }
  // نشان «کم» — همیشه با وضعیت کلید ۲ همگام
  jog_fine_badge_sync(jog_fine_held);
}

void opt_finish_to_manual(bool show_toast) {
  OPT.active = false;
  OPT.waiting_move = false;
  OPT.flag_refresh = false;
  AUT.selected_count_val = -1;
  AUT.temp_list_count_Val = 0;
  AUT.in_position_flag = true;
  STP.in_position_flag = true;
  GUI.update_lable1_3 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  opt_main_bar_lbl_hide();
  if (objects.lable1_3) lv_label_set_text(objects.lable1_3, "---");
  main_restore_destination_to_current_pos();
  if (show_toast)
    ui_toast("بهينه سازي تمام شد", "Optimization finished", 4000);
}

void opt_main_bar_lbl_sync() {
  if (!objects.main_1) return;
  // جاگ/دستی اولویت دارد — کادر شاخه را نشان نده
  if (GUI.NumLock ||
      (STP.motorIs_moving && !STP.moveStepperCMD &&
       (keyboardJogActive || STP.jogEnabled || IOS.joystick_state_L || IOS.joystick_state_R))) {
    opt_main_bar_lbl_hide();
    jog_manual_badge_sync();
    return;
  }
  jog_manual_badge_hide();
  const bool opt_mode = OPT.active && OPT.cur < OPT.piece_count;
  const bool list_mode = !opt_mode && AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX;
  if (!opt_mode && !list_mode) {
    opt_main_bar_lbl_hide();
    return;
  }
  const int bar_w = 43;
  const int bar_x = 64;
  // مهاجرت از لیبل تکی قدیمی به کانتینر دوخطی
  if (opt_main_bar_lbl && lv_obj_is_valid(opt_main_bar_lbl) && !opt_main_bar_title) {
    lv_obj_delete(opt_main_bar_lbl);
    opt_main_bar_lbl = NULL;
    opt_main_bar_num = NULL;
  }
  if (!opt_main_bar_lbl || !lv_obj_is_valid(opt_main_bar_lbl)) {
    opt_main_bar_lbl = lv_obj_create(objects.main_1);
    lv_obj_set_size(opt_main_bar_lbl, bar_w, 44);
    lv_obj_set_style_radius(opt_main_bar_lbl, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(opt_main_bar_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(opt_main_bar_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(opt_main_bar_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(opt_main_bar_lbl, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(opt_main_bar_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(opt_main_bar_lbl, opt_main_bar_click_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(opt_main_bar_lbl, opt_main_bar_click_cb, LV_EVENT_PRESSED, NULL);

    opt_main_bar_title = lv_label_create(opt_main_bar_lbl);
    lv_obj_set_width(opt_main_bar_title, bar_w);
    lv_obj_set_style_text_color(opt_main_bar_title, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(opt_main_bar_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    opt_main_bar_num = lv_label_create(opt_main_bar_lbl);
    lv_obj_set_width(opt_main_bar_num, bar_w);
    lv_obj_set_style_text_color(opt_main_bar_num, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(opt_main_bar_num, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  // سبز = شاخه بهینه؛ آبی = ردیف لیست برش
  lv_obj_set_style_bg_color(opt_main_bar_lbl,
                            lv_color_hex(opt_mode ? 0x00572b : 0x0A84FF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(opt_main_bar_lbl, bar_x, 42);  // 45 → 42: دقیقاً ۳px بالا
  lv_obj_set_size(opt_main_bar_lbl, bar_w, 44);
  const bool fa = (GUI.system_language == 1);
  if (opt_main_bar_title) {
    lv_obj_set_width(opt_main_bar_title, bar_w);
    lv_obj_set_style_text_font(opt_main_bar_title, fa ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(opt_main_bar_title, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (opt_mode) lv_label_set_text(opt_main_bar_title, fa ? "شاخه" : "Bar");
    else lv_label_set_text(opt_main_bar_title, fa ? "ردیف" : "Row");
    lv_obj_set_pos(opt_main_bar_title, 0, -1);
  }
  if (opt_main_bar_num) {
    lv_obj_set_width(opt_main_bar_num, bar_w);
    // رقم بزرگ و خوانا با Montserrat 18
    lv_obj_set_style_text_font(opt_main_bar_num, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (opt_mode && OPT.badge_wait_pick) lv_label_set_text(opt_main_bar_num, "--");
    else if (opt_mode) lv_label_set_text_fmt(opt_main_bar_num, "%d", OPT.piece_bar[OPT.cur] + 1);
    else lv_label_set_text_fmt(opt_main_bar_num, "%d", AUT.selected_count_val + 1);
    lv_obj_set_pos(opt_main_bar_num, 0, 20);
  }
  lv_obj_clear_flag(opt_main_bar_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(opt_main_bar_lbl);

  // کادر زاویه (۴۵° / ۹۰°) — ۲۶ پیکسل بالاتر و ۴ پیکسل به چپ (X=368, Y=12)
  if (opt_mode && OPT.badge_wait_pick) {
    if (opt_main_ang_lbl && lv_obj_is_valid(opt_main_ang_lbl))
      lv_obj_add_flag(opt_main_ang_lbl, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  uint8_t cur_angle = 45;
  if (list_mode) {
    cur_angle = AUT.list_angle_Val[AUT.selected_count_val];
  } else if (opt_mode) {
    int b = OPT.piece_bar[OPT.cur];
    // OPT.bar_angle نه scratch — scratch بعد از موفقیت آزاد می‌شود (خواندنش هنگ بود)
    if (b >= 0 && b < OPT_MAX_BARS && OPT.bar_angle[b] != 0) cur_angle = OPT.bar_angle[b];
  }
  if (cur_angle != 90) cur_angle = 45;

  const int ang_w = 32;
  const int ang_h = 26;
  if (!opt_main_ang_lbl || !lv_obj_is_valid(opt_main_ang_lbl)) {
    opt_main_ang_lbl = lv_obj_create(objects.main_1);
    lv_obj_set_size(opt_main_ang_lbl, ang_w, ang_h);
    lv_obj_set_style_radius(opt_main_ang_lbl, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(opt_main_ang_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(opt_main_ang_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(opt_main_ang_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(opt_main_ang_lbl, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(opt_main_ang_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(opt_main_ang_lbl, opt_main_ang_click_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(opt_main_ang_lbl, opt_main_ang_click_cb, LV_EVENT_PRESSED, NULL);

    opt_main_ang_title = lv_label_create(opt_main_ang_lbl);
    lv_obj_add_flag(opt_main_ang_title, LV_OBJ_FLAG_HIDDEN);

    opt_main_ang_val = lv_label_create(opt_main_ang_lbl);
    lv_obj_set_style_text_color(opt_main_ang_val, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(opt_main_ang_val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_set_style_bg_color(opt_main_ang_lbl,
                            lv_color_hex(cur_angle == 90 ? 0xd97706 : 0x0284c7),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  // پنل درجه: هم‌تراز عمودی با وسط کادر عدد مقصد (X=368, Y=28)
  lv_obj_set_pos(opt_main_ang_lbl, 368, 28);
  lv_obj_set_size(opt_main_ang_lbl, ang_w, ang_h);

  if (opt_main_ang_title) {
    lv_obj_add_flag(opt_main_ang_title, LV_OBJ_FLAG_HIDDEN);
  }
  if (opt_main_ang_val) {
    lv_obj_set_style_text_font(opt_main_ang_val, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(opt_main_ang_val, cur_angle == 90 ? "90°" : "45°");
    lv_obj_center(opt_main_ang_val);
  }
  lv_obj_clear_flag(opt_main_ang_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(opt_main_ang_lbl);
  // برچسب mm در پیش‌فرض تولیدی (364,56) می‌ماند — زیر کادر درجه، بدون هم‌پوشانی
}

static void opt_main_bar_click_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    if (opt_main_bar_lbl && lv_obj_is_valid(opt_main_bar_lbl)) {
      lv_obj_set_style_transform_scale(opt_main_bar_lbl, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  } else if (code == LV_EVENT_RELEASED) {
    if (opt_main_bar_lbl && lv_obj_is_valid(opt_main_bar_lbl)) {
      lv_obj_set_style_transform_scale(opt_main_bar_lbl, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    GUI.buzzer_cmd_short = true;
    if (OPT.active) {
      opt_open_panel();
    } else if (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX) {
      request_goto_screen(3);
      list_show_row(AUT.selected_count_val);
    }
  }
}

static void opt_invalidate_cache() {
  opt_calc_reset();
  OPT.active = false;
  OPT.waiting_move = false;
  OPT.flag_refresh = false;
  OPT.cur = 0;
  OPT.badge_wait_pick = true;
  OPT.piece_count = 0;
  OPT.bar_count = 0;
  // cut_log نمی‌پاکد — فقط با تأیید روی کادر «برش خورده»
  opt_plan_ready = false;
  opt_saved_list_fp = 0;
  opt_list_edit_dirty = false;
  opt_last_loaded_bar = -1;
  opt_rebuild_req = false;
  opt_main_bar_lbl_hide();
  if (opt_cont) {
    lv_obj_delete_async(opt_cont);
    opt_cont = NULL;
  }
  GUI.lock_screen = false;
}

static void f1_hdr_set_book_icon(lv_obj_t *btn);
static void list_hdr_button_icon(lv_obj_t *btn, const char *txt, const lv_font_t *font, int yoff);

static String opt_fmt_len(float v) {
  char b[16];
  float r = roundf(v * 10.0f) / 10.0f;
  if (fabsf(r - roundf(r)) < 0.05f) snprintf(b, sizeof(b), "%d", (int)lroundf(r));
  else snprintf(b, sizeof(b), "%.1f", (double)r);
  return String(b);
}

// عرض ستون قطعات: badge پرت x=352، فاصلهٔ حداقل ۵px → right=347؛ +۸ بودجهٔ wrap
static const int OPT_PIECE_LINE_W = 265;   // red@352 −۵ − piece_x82؛ CHAR_W=12 ≈ +۸ جا نسبت به ۱۴
static const int OPT_PIECE_CHAR_W = 12;    // montserrat_24 واقعی باریک‌تر از ۱۴ بود

static void opt_append_piece_token(String &out, bool &first, int &line_w, int max_w,
                                   const String &token, int state) {
  if (!token.length()) return;
  int tw = (int)token.length() * OPT_PIECE_CHAR_W;
  const int sep_w = 3 * OPT_PIECE_CHAR_W;  // " - " نارنجی
  if (!first && line_w + sep_w + tw > max_w) {
    out += "\n";
    first = true;
    line_w = 0;
  }
  if (!first) {
    out += " #FF9500 -# ";
    line_w += sep_w;
  }
  if (state == 0) {
    // برش‌خورده: کم‌رنگ (خط‌خوردگی کل لیبل وقتی همهٔ شاخه تمام است)
    out += "#6f7780 ";
    out += token;
    out += "#";
  } else if (state == 1) {
    out += "#c47f00 ";
    out += token;
    out += "#";
  } else {
    out += token;
  }
  line_w += tw;
  first = false;
}

static void opt_flush_run_token(String &out, bool &first, int &line_w, int max_w,
                                int run_tenths, int run_count, int state) {
  if (run_count <= 0 || run_tenths < 0) return;
  String token = opt_fmt_len((float)run_tenths / 10.0f);
  if (run_count > 1) {
    token += "x";
    token += String(run_count);
  }
  opt_append_piece_token(out, first, line_w, max_w, token, state);
}

// یک جریان ثابت: برش‌خورده‌ها جایشان می‌مانند؛ باقی پایین کشیده نمی‌شوند
static void opt_build_bar_flows(int bar, String &cut_out, String &live_out) {
  cut_out = "";
  live_out = "";
  if (bar < 0 || bar >= OPT.bar_count || bar >= OPT_MAX_BARS) return;
  const int i0 = (int)opt_bar_pi0[bar];
  const int pn = (int)opt_bar_pn[bar];
  if (i0 < 0 || pn <= 0) return;
  const int i1 = i0 + pn;
  bool first_c = true, first_l = true;
  int line_w_c = 0, line_w_l = 0;
  const int max_w = OPT_PIECE_LINE_W;
  int run_tenths = -1;
  int run_count = 0;
  int run_state = 2;
  auto flush = [&]() {
    if (run_count <= 0 || run_tenths < 0) return;
    if (run_state == 0)
      opt_flush_run_token(cut_out, first_c, line_w_c, max_w, run_tenths, run_count, 0);
    else
      opt_flush_run_token(live_out, first_l, line_w_l, max_w, run_tenths, run_count, run_state);
    run_tenths = -1;
    run_count = 0;
  };
  int cur_end = OPT.cur - 1;
  if (OPT.cur >= i0 && OPT.cur < i1) {
    int len_t = (int)lroundf(OPT.piece_len[OPT.cur] * 10.0f);
    cur_end = OPT.cur;
    for (int i = OPT.cur; i < i1; i++) {
      if ((int)lroundf(OPT.piece_len[i] * 10.0f) != len_t) break;
      cur_end = i;
    }
  }
  for (int i = i0; i < i1; i++) {
    int state = 2;
    if (opt_piece_done[i]) {
      state = 0;
    } else if (OPT.cur >= i0 && OPT.cur < i1 && i >= OPT.cur && i <= cur_end) {
      state = 1;
    }
    int tenths = (int)lroundf(OPT.piece_len[i] * 10.0f);
    if (run_count > 0 && tenths == run_tenths && state == run_state) {
      run_count++;
    } else {
      flush();
      run_tenths = tenths;
      run_count = 1;
      run_state = state;
    }
  }
  flush();
}

static void opt_build_bar_flow_text(int bar, String &out) {
  String cut, live;
  opt_build_bar_flows(bar, cut, live);
  out = cut;
  if (cut.length() && live.length()) out += "\n";
  out += live;
}

// تعداد پشت‌سرهم همان متراژ در همان شاخه از ایندکس start (برنامهٔ بهینه، نه کل لیست برش)
int opt_run_qty_from(int start) {
  if (start < 0 || start >= OPT.piece_count) return 0;
  const int bar = (int)OPT.piece_bar[start];
  const int tenths = (int)lroundf(OPT.piece_len[start] * 10.0f);
  int n = 1;
  for (int i = start + 1; i < OPT.piece_count; i++) {
    if ((int)OPT.piece_bar[i] != bar) break;
    if ((int)lroundf(OPT.piece_len[i] * 10.0f) != tenths) break;
    n++;
  }
  return n;
}

int opt_current_run_qty() { return opt_run_qty_from(OPT.cur); }

static int opt_count_lines(const String &s) {
  if (!s.length()) return 0;
  int n = 1;
  for (unsigned i = 0; i < s.length(); i++) if (s[i] == '\n') n++;
  return n;
}

static bool opt_bar_is_finished(int bar) {
  if (bar < 0 || bar >= OPT.bar_count || bar >= OPT_MAX_BARS) return false;
  const int i0 = (int)opt_bar_pi0[bar];
  const int pn = (int)opt_bar_pn[bar];
  if (i0 < 0 || pn <= 0) return false;
  for (int i = i0; i < i0 + pn; i++) {
    if (!opt_piece_done[i]) return false;
  }
  return true;
}

static int opt_bar_for_slot(int slot) {
  int bar = opt_bar_page_start + slot;
  return (slot >= 0 && slot < OPT_RENDER_ROWS && bar >= 0 && bar < OPT.bar_count) ? bar : -1;
}

static void opt_bar_purge_show(int bar);  // fwd

static void opt_tap_arm_from_event(lv_event_t *e) {
  (void)e;
  opt_tap_armed = true;
  opt_tap_scroll_y0 = 0;
  if (opt_list_body && lv_obj_is_valid(opt_list_body))
    opt_tap_scroll_y0 = lv_obj_get_scroll_y(opt_list_body);
  lv_indev_t *indev = lv_indev_active();
  if (indev) lv_indev_get_point(indev, &opt_tap_pt0);
  else opt_tap_armed = false;
}

static bool opt_tap_was_stationary() {
  if (!opt_tap_armed) return false;
  opt_tap_armed = false;
  if (opt_list_body && lv_obj_is_valid(opt_list_body)) {
    int32_t dy = lv_obj_get_scroll_y(opt_list_body) - opt_tap_scroll_y0;
    if (dy < 0) dy = -dy;
    if (dy > 8) return false;
  }
  lv_indev_t *indev = lv_indev_active();
  if (indev) {
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    int32_t dx = p.x - opt_tap_pt0.x;
    int32_t dy = p.y - opt_tap_pt0.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx > 12 || dy > 12) return false;
  }
  return true;
}

static void opt_bar_finished_tap_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (!opt_tap_was_stationary()) return;
  int bar = opt_bar_for_slot((int)(intptr_t)lv_event_get_user_data(e));
  if (bar < 0) return;
  if (!opt_bar_is_finished(bar)) return;
  opt_bar_purge_show(bar);
}

static lv_obj_t *opt_done_history_lbl = NULL;
static lv_obj_t *opt_done_history_parent = NULL;
static int opt_done_history_n = -1;
static int8_t opt_done_history_lang = -1;
static lv_obj_t *opt_lbl_branch_title = NULL;
static lv_obj_t *opt_lbl_branch_value = NULL;

static void opt_clear_panel_ptrs() {
  opt_done_box = NULL;
  opt_done_body = NULL;
  opt_done_lbl = NULL;
  opt_done_history_lbl = NULL;
  opt_done_history_parent = NULL;
  opt_done_history_n = -1;
  opt_done_history_lang = -1;
  opt_list_body = NULL;
  opt_lbl_branch_title = NULL;
  opt_lbl_branch_value = NULL;
  opt_lbl_cut_title = NULL;
  opt_lbl_cut_value = NULL;
  opt_lbl_current = NULL;
  opt_bar_page_lbl = NULL;
  opt_bar_page_prev_btn = NULL;
  opt_bar_page_next_btn = NULL;
  for (int i = 0; i < OPT_RENDER_ROWS; i++) {
    opt_bar_rows[i] = NULL;
    opt_badge_angle[i] = NULL;
    opt_badge_bar[i] = NULL;
    opt_badge_waste[i] = NULL;
    opt_badge_waste_num[i] = NULL;
    opt_badge_bar_num[i] = NULL;
    opt_badge_bar_xmark[i] = NULL;
    opt_piece_done_lbl[i] = NULL;
    opt_piece_current_lbl[i] = NULL;
    opt_piece_lbl[i] = NULL;
  }
}

// هدر پنل: شاخه/برش — عنوان دوزبانه + رقم Montserrat؛ دیگر هرگز "Text" نمی‌ماند
static void opt_stats_labels_sync() {
  if (!opt_cont || !lv_obj_is_valid(opt_cont)) return;
  const bool fa = (GUI.system_language == 1);
  if (opt_lbl_bar_title) lv_label_set_text(opt_lbl_bar_title, fa ? "شاخه" : "Bar");
  if (opt_lbl_bar_value) lv_label_set_text_fmt(opt_lbl_bar_value, "%d", OPT.bar_count);
  if (opt_lbl_cut_title) lv_label_set_text(opt_lbl_cut_title, fa ? "برش" : "Cut");
  if (opt_lbl_cut_value) lv_label_set_text_fmt(opt_lbl_cut_value, "%d", OPT.piece_count);
}

static void opt_apply_piece_labels() {
  if (!opt_cont || !lv_obj_is_valid(opt_cont) || !opt_list_body || !lv_obj_is_valid(opt_list_body)) return;
  for (int slot = 0; slot < OPT_RENDER_ROWS; slot++) {
    int b = opt_bar_for_slot(slot);
    // کش رندر: شاخه/cur/لاگ/نسل این ردیف عوض نشده → رندر قبلی معتبر است (تأخیر اسکرول صفحه)
    if (b == opt_row_last_bar[slot] && OPT.cur == opt_row_last_cur[slot] &&
        OPT.cut_log_n == opt_row_last_log[slot] && opt_render_gen == opt_row_last_gen[slot]) continue;
    opt_row_last_bar[slot] = (int16_t)b;
    opt_row_last_cur[slot] = (int16_t)OPT.cur;
    opt_row_last_log[slot] = (int16_t)OPT.cut_log_n;
    opt_row_last_gen[slot] = opt_render_gen;
    if (b < 0) {
      if (opt_bar_rows[slot]) lv_obj_add_flag(opt_bar_rows[slot], LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    if (opt_bar_rows[slot]) lv_obj_clear_flag(opt_bar_rows[slot], LV_OBJ_FLAG_HIDDEN);
    String cut_flow = "", live_flow = "";
    opt_build_bar_flows(b, cut_flow, live_flow);
    String flow = cut_flow;
    if (cut_flow.length() && live_flow.length()) flow += "\n";
    flow += live_flow;
    const int piece_x = 86;
    const int piece_right = 347;  // red@352 − ۵px
    const int piece_w = piece_right - piece_x;  // 261
    const bool fin = opt_bar_is_finished(b);
    if (opt_piece_current_lbl[slot]) lv_obj_add_flag(opt_piece_current_lbl[slot], LV_OBJ_FLAG_HIDDEN);
    int cut_lines = cut_flow.length() ? opt_count_lines(cut_flow) : 0;
    int live_y = 6 + (cut_lines > 0 ? cut_lines * 28 : 0);
    // اعداد برش‌خورده (تمام یا بخشی از شاخه) با خط‌خوردگی
    // استایل ثابت در زمان ساخت ست می‌شود — اینجا فقط متن/پرچم دینامیک
    if (opt_piece_done_lbl[slot]) {
      if (cut_flow.length()) {
        lv_label_set_text(opt_piece_done_lbl[slot], cut_flow.c_str());
        lv_obj_clear_flag(opt_piece_done_lbl[slot], LV_OBJ_FLAG_HIDDEN);
        if (fin) lv_obj_add_flag(opt_piece_done_lbl[slot], LV_OBJ_FLAG_CLICKABLE);
        else lv_obj_clear_flag(opt_piece_done_lbl[slot], LV_OBJ_FLAG_CLICKABLE);
      } else {
        lv_obj_add_flag(opt_piece_done_lbl[slot], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(opt_piece_done_lbl[slot], LV_OBJ_FLAG_CLICKABLE);
      }
    }
    if (opt_piece_lbl[slot]) {
      if (!fin && live_flow.length()) {
        lv_obj_set_pos(opt_piece_lbl[slot], piece_x, live_y);  // y دینامیک زیر خط‌های برش‌خورده
        lv_label_set_text(opt_piece_lbl[slot], live_flow.c_str());
        lv_obj_clear_flag(opt_piece_lbl[slot], LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(opt_piece_lbl[slot], LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (opt_badge_bar_num[slot]) lv_label_set_text_fmt(opt_badge_bar_num[slot], "%d", b + 1);
    if (opt_badge_bar[slot]) {
      // شاخهٔ تمام‌شده = زرد؛ فعال = سبز
      lv_obj_set_style_bg_color(opt_badge_bar[slot],
                                lv_color_hex(fin ? 0xFFCC00 : 0x00572b),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(opt_badge_bar[slot],
                                  lv_color_hex(fin ? 0x3a3a3c : 0xffffff),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      if (opt_badge_bar_num[slot])
        lv_obj_set_style_text_color(opt_badge_bar_num[slot],
                                    lv_color_hex(fin ? 0x3a3a3c : 0xffffff),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (opt_bar_rows[slot]) {
      // ردیف تمام‌شده: پس‌زمینه زرد (نه سبز)
      lv_obj_set_style_bg_color(opt_bar_rows[slot],
                                lv_color_hex(fin ? 0xFFF3B0 : ((slot & 1) ? 0xf7f8fb : 0xffffff)),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (opt_badge_waste_num[slot]) {
      float wr = roundf(OPT.bar_waste[b] * 10.0f) / 10.0f;
      char wb[24];
      if (fabsf(wr - roundf(wr)) < 0.05f) snprintf(wb, sizeof(wb), "%d", (int)lroundf(wr));
      else snprintf(wb, sizeof(wb), "%.1f", (double)wr);
      lv_label_set_text(opt_badge_waste_num[slot], wb);
    }
    if (opt_bar_rows[slot] && opt_badge_bar[slot] && opt_badge_waste[slot]) {
      int lines = opt_count_lines(flow);
      if (lines < 1) lines = 1;
      // ارتفاع واقعی محتوا (متن از y=6) + پد ردیف ۶+۶
      int inner_h = 6 + lines * 28;
      if (inner_h < 56) inner_h = 56;
      const int pad = 6;
      int row_h = inner_h + pad * 2;
      lv_obj_set_height(opt_bar_rows[slot], row_h);
      const int badge_h = 48;
      // وسط واقعی کمی پایین می‌آمد → ۲px جبران به بالا (قبلاً −۴؛ مالک: ۲px پایین‌تر)
      int badge_y = (inner_h - badge_h) / 2 - 2;
      if (badge_y < 0) badge_y = 0;
      lv_obj_set_y(opt_badge_bar[slot], badge_y);
      lv_obj_set_y(opt_badge_waste[slot], badge_y);
      if (opt_badge_bar_xmark[slot] && lv_obj_is_valid(opt_badge_bar_xmark[slot])) {
        if (fin) {
          lv_obj_clear_flag(opt_badge_bar_xmark[slot], LV_OBJ_FLAG_HIDDEN);
          lv_obj_move_foreground(opt_badge_bar_xmark[slot]);
        } else {
          lv_obj_add_flag(opt_badge_bar_xmark[slot], LV_OBJ_FLAG_HIDDEN);
        }
      }
    }
  }
}

static int opt_cmp_desc(const void *a, const void *b) {
  float fa = *(const float *)a, fb = *(const float *)b;
  return (fb > fa) - (fb < fa);
}

static int opt_cmp_piece_desc(const void *a, const void *b) {
  const OptCalcPiece *pa = (const OptCalcPiece *)a;
  const OptCalcPiece *pb = (const OptCalcPiece *)b;
  if (pa->angle != pb->angle) {
    return (int)pa->angle - (int)pb->angle;
  }
  return (pb->len > pa->len) - (pb->len < pa->len);
}
void opt_refresh_labels() {
  opt_stats_labels_sync();
  opt_apply_piece_labels();
}

bool opt_load_current_to_main(bool move_machine, bool sync_badge) {
  if (!OPT.active || OPT.cur >= OPT.piece_count) return false;
  float len = OPT.piece_len[OPT.cur];
  STP.Man_Sp_mm = len;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(len));
  GUI.inputBuffer_sp = String(buf);
  GUI.update_lable1_1 = true;
  GUI.edit_mode_lable1_1 = true;

  // محاسبهٔ تعداد باقی‌مانده از این سایز در شاخهٔ جاری
  int count = 0;
  int cur_bar = (OPT.cur < OPT.piece_count) ? (int)OPT.piece_bar[OPT.cur] : -1;
  for (int i = OPT.cur; i < OPT.piece_count; i++) {
    if ((int)OPT.piece_bar[i] != cur_bar) break;
    if (!opt_piece_done[i] && fabsf(OPT.piece_len[i] - len) < 0.2f) {
      count++;
    }
  }
  if (count <= 0) count = 1;
  AUT.temp_list_count_Val = count;
  AUT.selected_count_val = 0;  // اطمینان از معتبر بودن ایندکس جهت نمایش در lable1_3
  GUI.update_lable1_3 = true;

  if (sync_badge) opt_main_bar_lbl_sync();
  if (move_machine && STP.referenced) {
    STP.position_Setpoint_p = (int)round((len * (STP.drv_ppr / STP.Axis_Pitch)));
    STP.moveStepperCMD = true;
  }
  return true;
}

static void opt_calc_cancel_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  GUI.flag_abort_opt_calc = true;  // حلقهٔ Task2 در همان تیک بعد لغو/پاک‌سازی می‌کند
}

static void opt_calc_open_modal() {
  opt_calc_close_modal();
  opt_calc_box = lv_obj_create(lv_layer_top());
  lv_obj_set_size(opt_calc_box, 380, 168);
  lv_obj_align(opt_calc_box, LV_ALIGN_CENTER, 0, 5);
  lv_obj_clear_flag(opt_calc_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(opt_calc_box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(opt_calc_box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_calc_box, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_calc_box, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(opt_calc_box, lv_color_hex(0xd7dde2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_calc_box, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(opt_calc_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // خواستهٔ مالک: داخل کادر فقط درصد، دقیقاً در مرکز؛ بدون عنوان/زمان/متن.
  opt_calc_status = lv_label_create(opt_calc_box);
  lv_obj_set_width(opt_calc_status, 350);
  lv_obj_align(opt_calc_status, LV_ALIGN_CENTER, 0, -12);
  lv_obj_set_style_text_font(opt_calc_status, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_calc_status, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(opt_calc_status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_calc_status, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(opt_calc_status, "0%");

  // نوار پیشرفت زیر درصد — از ۱ تا ۱۰۰ پر شود (درخواست مالک)
  opt_calc_bar = lv_bar_create(opt_calc_box);
  lv_obj_set_size(opt_calc_bar, 350, 14);
  lv_obj_align_to(opt_calc_bar, opt_calc_status, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);
  lv_bar_set_range(opt_calc_bar, 0, 100);
  lv_bar_set_value(opt_calc_bar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(opt_calc_bar, lv_color_hex(0xE5E5EA), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(opt_calc_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_calc_bar, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_calc_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_calc_bar, lv_color_hex(0x0A84FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(opt_calc_bar, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_calc_bar, 7, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_calc_bar, LV_OBJ_FLAG_CLICKABLE);

  // ضربدر لغو — خواستهٔ مالک: اگر بهینه نمی‌خواست، بتواند قطع کند
  lv_obj_t *x_btn = lv_obj_create(opt_calc_box);
  lv_obj_set_size(x_btn, 36, 36);
  lv_obj_align(x_btn, LV_ALIGN_TOP_RIGHT, 6, -6);
  lv_obj_set_style_radius(x_btn, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(x_btn, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(x_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(x_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(x_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(x_btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(x_btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(x_btn, opt_calc_cancel_click_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *x_lbl = lv_label_create(x_btn);
  lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
  lv_obj_center(x_lbl);

  lv_obj_move_foreground(opt_calc_box);
}

static void opt_calc_update_modal(int percent) {
  if (!opt_calc_box) return;
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  if (opt_calc_status) {
    char msg[16];
    snprintf(msg, sizeof(msg), "%d%%", percent);
    lv_label_set_text(opt_calc_status, msg);
  }
  if (opt_calc_bar) lv_bar_set_value(opt_calc_bar, percent, LV_ANIM_OFF);
}

// اثرانگشت: اندازه + تعداد + شاخه/kerf/پیش‌برش.
// بعد از برش، fp را sync می‌کنیم تا کاهش تعداد برش باعث بازچینی نشود.
// ویرایش دستی لیست → opt_list_edit_dirty یا mismatch → بازچینی با حفظ cut_log.
static uint32_t opt_list_fingerprint() {
  uint32_t h = 2166136261u;
  auto mix = [&](uint32_t v) {
    h ^= v;
    h *= 16777619u;
  };
  mix((uint32_t)stock_len_mm);
  mix((uint32_t)lroundf(kerf_mm * 100.0f));
  mix((uint32_t)trim_mm);
  for (int i = 0; i < LIST_MAX; i++) {
    int tenths = (int)lroundf(AUT.list_sp_Val_p[i] * 10.0f);
    int cnt = AUT.list_count_Val[i];
    if (tenths <= 0 && cnt <= 0) continue;
    mix((uint32_t)(i + 1));
    mix((uint32_t)tenths);
    mix((uint32_t)(cnt < 0 ? 0 : cnt));
  }
  return h;
}

void opt_notify_list_edited() {
  opt_list_edit_dirty = true;
}

// برنامهٔ فعلی هنوز با موجودی لیست قابل ادامه است؟
static bool opt_plan_still_valid() {
  if (!opt_plan_ready || OPT.piece_count <= 0) return false;
  int need[LIST_MAX];
  for (int i = 0; i < LIST_MAX; i++) need[i] = 0;
  for (int i = OPT.cur; i < OPT.piece_count; i++) {
    int e = (int)OPT.piece_entry[i];
    if (e < 0 || e >= LIST_MAX) return false;
    int tenths_plan = (int)lroundf(OPT.piece_len[i] * 10.0f);
    int tenths_list = (int)lroundf(AUT.list_sp_Val_p[e] * 10.0f);
    if (tenths_plan != tenths_list) return false;
    need[e]++;
  }
  for (int i = 0; i < LIST_MAX; i++) {
    if (need[i] > AUT.list_count_Val[i]) return false;
  }
  return true;
}

static bool opt_open_from_list() {
  uint32_t fp = opt_list_fingerprint();
  // لیست دست‌نخورده و قطعات باقی هنوز موجود → برنامه/cur/خط‌خوردگی بمان
  if (!opt_list_edit_dirty && opt_plan_ready && OPT.piece_count > 0 &&
      fp == opt_saved_list_fp && opt_plan_still_valid()) {
    opt_rebuild_bar_ranges();
    opt_open_panel();
    return true;
  }
  // سایز/تعداد زیاد/عوض‌شده → محاسبه غیرمسدود؛ لاگ برش‌خورده‌ها حفظ می‌شود
  return opt_calc_begin(false);
}

//---------- پنل بهینه‌سازی ----------

static void opt_done_box_clear_body() {
  if (!opt_done_body || !lv_obj_is_valid(opt_done_body)) return;
  while (lv_obj_get_child_count(opt_done_body) > 0) {
    lv_obj_t *ch = lv_obj_get_child(opt_done_body, 0);
    if (ch) lv_obj_delete(ch);
    else break;
  }
  opt_done_history_lbl = NULL;
  opt_done_history_parent = NULL;
  opt_done_history_n = -1;
  opt_done_history_lang = -1;
}

static void opt_done_box_sync() {
  if (!opt_cont || !lv_obj_is_valid(opt_cont)) return;
  const bool fa = (GUI.system_language == 1);
  // کادر «برش خورده» فقط صفحهٔ اول شاخه‌ها
  if (opt_done_box && lv_obj_is_valid(opt_done_box)) {
    if (opt_bar_page_start == 0) lv_obj_clear_flag(opt_done_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(opt_done_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (opt_bar_page_start != 0) {
    opt_done_box_clear_body();
    return;
  }
  int total_done = OPT.cut_log_n;
  if (total_done <= 0) {
    for (int p = 0; p < OPT.piece_count; p++) {
      if (OPT.piece_done[p]) total_done++;
    }
  }
  if (opt_done_lbl && lv_obj_is_valid(opt_done_lbl)) {
    if (total_done <= 0) {
      lv_obj_clear_flag(opt_done_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_font(opt_done_lbl, fa ? &ui_font_naz18b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_decor(opt_done_lbl, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_opa(opt_done_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(opt_done_lbl, lv_color_hex(0x6f7780), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(opt_done_lbl, fa ? "هنوز برشی ثبت نشده" : "No cuts logged yet");
    } else {
      lv_obj_add_flag(opt_done_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (opt_done_box && lv_obj_is_valid(opt_done_box)) {
    lv_obj_set_style_bg_opa(opt_done_box, total_done > 0 ? LV_OPA_COVER : LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(opt_done_box, total_done > 0 ? LV_OPA_COVER : LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (!opt_done_body || !lv_obj_is_valid(opt_done_body)) return;
  if (total_done <= 0) {
    if (lv_obj_get_child_count(opt_done_body) > 0) opt_done_box_clear_body();
    return;
  }

  if (opt_done_history_lbl && lv_obj_is_valid(opt_done_history_lbl) &&
      opt_done_history_parent == opt_done_body &&
      opt_done_history_n == total_done &&
      opt_done_history_lang == (int8_t)GUI.system_language) {
    return;
  }

  // یک label با بازنمایی واضح و خوانای قطعات برش‌خورده با فونت درشت
  // قطعه تازه برش‌خورده در ابتدای کادر به‌صورت برجسته نمایش داده می‌شود
  opt_done_box_clear_body();
  String history;
  history.reserve((size_t)total_done * 24U + 64U);

  if (OPT.cut_log_n > 0) {
    // نمایش برجسته آخرین متراژ برش‌خورده در سر کادر
    float last_mm = OPT.cut_log_mm[OPT.cut_log_n - 1];
    int16_t last_bar = OPT.cut_log_bar[OPT.cut_log_n - 1];
    if (fa) {
      history += "آخرین برش: " + opt_fmt_len(last_mm) + " mm";
      if (last_bar >= 0) history += " (شاخه " + String(last_bar + 1) + ")";
      history += "\n--------------------------------\n";
    } else {
      history += "Last Cut: " + opt_fmt_len(last_mm) + " mm";
      if (last_bar >= 0) history += " (Bar " + String(last_bar + 1) + ")";
      history += "\n--------------------------------\n";
    }

    int i = 0;
    while (i < OPT.cut_log_n) {
      int16_t bar = OPT.cut_log_bar[i];
      int j = i + 1;
      while (j < OPT.cut_log_n && OPT.cut_log_bar[j] == bar) j++;

      if (bar >= 0) {
        if (fa) {
          history += "شاخه " + String(bar + 1) + ": ";
        } else {
          history += "Bar " + String(bar + 1) + ": ";
        }
      } else {
        history += (fa ? "برش دستی: " : "Manual: ");
      }

      // فهرست قطعات این شاخه
      String line = "";
      int run_t = -1, run_n = 0;
      auto flush = [&]() {
        if (run_n <= 0) return;
        if (line.length()) line += " - ";
        line += opt_fmt_len((float)run_t / 10.0f);
        if (run_n > 1) {
          line += fa ? " (تعداد " : " (x";
          line += String(run_n);
          line += fa ? ")" : ")";
        }
        run_n = 0;
        run_t = -1;
      };
      for (int k = i; k < j; k++) {
        int t = (int)lroundf(OPT.cut_log_mm[k] * 10.0f);
        if (run_n > 0 && t == run_t) run_n++;
        else {
          flush();
          run_t = t;
          run_n = 1;
        }
      }
      flush();
      history += line + "\n";
      i = j;
    }
  } else {
    // بازنمایی مستقیم از روی piece_done
    for (int b = 0; b < OPT.bar_count && b < OPT_MAX_BARS; b++) {
      String line = "";
      for (int p = 0; p < OPT.piece_count; p++) {
        if (OPT.piece_bar[p] == (uint16_t)b && OPT.piece_done[p]) {
          if (line.length()) line += " - ";
          line += opt_fmt_len(OPT.piece_len[p]);
        }
      }
      if (line.length()) {
        history += (fa ? "شاخه " : "Bar ") + String(b + 1) + ": " + line + "\n";
      }
    }
  }

  opt_done_history_lbl = lv_label_create(opt_done_body);
  opt_done_history_parent = opt_done_body;
  opt_done_history_n = OPT.cut_log_n;
  opt_done_history_lang = (int8_t)GUI.system_language;
  lv_obj_set_width(opt_done_history_lbl, 410);
  lv_obj_set_style_text_font(
      opt_done_history_lbl, fa ? &ui_font_naz20 : &lv_font_montserrat_16,
      LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_done_history_lbl, lv_color_hex(0x1c2b3a),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(opt_done_history_lbl, LV_OPA_COVER,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_decor(opt_done_history_lbl, LV_TEXT_DECOR_NONE,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(opt_done_history_lbl, history.c_str());
}

void prcs_opt_calc_progress() {
  if (GUI.flag_abort_opt_calc) {
    GUI.flag_abort_opt_calc = false;
    // استوپ/کانسل وقتی هیچ محاسبه‌ای فعال نیست: فقط پاک‌سازی خاموش پرچم —
    // بدون توست، بدون دست‌زدن به اسکرچ/مودال/قفل (توست بی‌ربط «لغو شد»)
    if (!opt_calc_active && !(opt_calc_box && lv_obj_is_valid(opt_calc_box))) return;
    opt_calc_free_scratch();
    opt_calc_active = false;
    opt_calc_stage = 0;
    GUI.lock_screen = false;
    opt_calc_close_modal();
    ui_toast("بهینه‌سازی لغو شد", "Optimization cancelled", 2000);
    return;
  }
  if (!opt_calc_active) return;
  if (!opt_calc_scratch) {
    opt_calc_fail("حافظه بهینه ساز آزاد شد", "Optimizer workspace was lost");
    return;
  }
  // تایم‌اوت امن — قفل استارت/استوپ نماند
  if ((millis() - opt_calc_start_ms) > 180000UL) {
    opt_calc_fail("زمان بهینه‌سازی تمام شد — دوباره امتحان کنید",
                  "Optimize timeout — please retry");
    return;
  }
  // بودجهٔ زمانی: روی هر فریم حداکثر حدود ۱.۱ms کار کن. سقف ثابت ۲۲۰
  // برای ۵۰۰۰ قطعه تا حدود یک دقیقه طول می‌کشید و شبیه هنگ دیده می‌شد.
  const uint32_t slice_start_us = micros();
  const uint32_t SLICE_US = 16000U; // محاسبه بسیار سریع و روان بدون تاخیر اضافی
  const int WORK_MAX = 32768;
  int work = 0;
  auto budget_left = [&]() {
    if (work >= WORK_MAX) return false;
    // هزینهٔ micros را برای هر مقایسه نپرداز؛ هر ۶۴ کار یک بار زمان را بسنج.
    if (work < 128 || (work & 0x3f) != 0) return true;
    return (uint32_t)(micros() - slice_start_us) < SLICE_US;
  };
  auto ui_pct = [&](int base, int span, int cur, int tot) {
    int p = base;
    if (tot > 0) p = base + (span * cur) / tot;
    if (p > 99) p = 99;
    if (millis() - opt_calc_last_ui_ms >= 40) {
      opt_calc_last_ui_ms = millis();
      opt_calc_update_modal(p);
    }
  };

  if (opt_calc_stage == 1) {
    opt_calc_stage = 2;
    return;
  }

  if (opt_calc_stage == 2) {
    while (budget_left() && opt_calc_n < OPT_MAX_PIECES) {
      if (opt_calc_row_remaining <= 0) {
        while (opt_calc_row < opt_calc_limit) {
          float len = opt_calc_input_len[opt_calc_row];
          int cnt = (int)opt_calc_input_count[opt_calc_row];
          if (len > 0.05f && cnt > 0) {
            opt_calc_row_len = len;
            opt_calc_row_entry = (uint8_t)opt_calc_row;
            opt_calc_row_ang = opt_calc_input_angle[opt_calc_row];
            opt_calc_row_remaining = cnt;
            opt_calc_row++;
            break;
          }
          opt_calc_row++;
        }
        if (opt_calc_row_remaining <= 0) break;
      }
      opt_calc_tmp[opt_calc_n].len = opt_calc_row_len;
      opt_calc_tmp[opt_calc_n].entry = opt_calc_row_entry;
      opt_calc_tmp[opt_calc_n].angle = opt_calc_row_ang;
      opt_calc_n++;
      opt_calc_row_remaining--;
      work++;
    }
    if (opt_calc_n >= OPT_MAX_PIECES &&
        (opt_calc_row_remaining > 0 || opt_calc_row < opt_calc_limit)) {
      opt_calc_overflow = true;
    }
    ui_pct(5, 30, opt_calc_n, opt_calc_total > 0 ? opt_calc_total : 1);
    if (opt_calc_row_remaining > 0 || opt_calc_row < opt_calc_limit) {
      if (!opt_calc_overflow) return;
    }
    if (opt_calc_n <= 0) {
      opt_calc_fail("لیست خالی است. متراژ و تعداد وارد کنید",
                    "List is empty. Enter length and quantity");
      return;
    }
    opt_calc_stage = 3;
    return;
  }

  if (opt_calc_stage == 3) {
    qsort(opt_calc_tmp, (size_t)opt_calc_n, sizeof(OptCalcPiece), opt_cmp_piece_desc);
    opt_calc_place_i = 0;
    opt_calc_scan_b = 0;
    opt_calc_bars = 0;
    for (int b = 0; b < OPT_MAX_BARS; b++) opt_calc_bar_angle[b] = 0;
    opt_calc_stage = 4;
    opt_calc_update_modal(40);
    return;
  }

  if (opt_calc_stage == 4) {
    // BFD با تفکیک زاویه: قطعات ۴۵ در شاخه‌های ۴۵ و ۹۰ در شاخه‌های ۹۰
    // قطعات از قبل نزولی مرتب‌اند → FFD: اولین شاخهٔ خوانا جا می‌دهد؛ اسکن کامل ممنوع
    while (budget_left() && opt_calc_place_i < opt_calc_n) {
      int i = opt_calc_place_i;
      const float need = opt_calc_tmp[i].len + opt_calc_kerf;
      const uint8_t ang = opt_calc_tmp[i].angle;
      int placed = -1;
      while (opt_calc_scan_b < opt_calc_bars && budget_left()) {
        const int b = opt_calc_scan_b++;
        work++;
        if (opt_calc_bar_angle[b] != 0 && opt_calc_bar_angle[b] != ang) continue;
        float rem = opt_calc_usable - opt_calc_bar_used[b];
        if (need <= rem + 1e-4f) {
          placed = b;
          break;
        }
      }
      if (placed < 0 && opt_calc_scan_b < opt_calc_bars) {
        break;  // بودجهٔ فریم وسط اسکن تمام شد → ادامهٔ اسکن در فریم بعد
      }
      if (placed < 0) {
        if (opt_calc_bars >= OPT_MAX_BARS) {
          char fa[96], en[80];
          snprintf(fa, sizeof(fa), "بیش از ظرفیت شاخه است؛ ظرفیت %d شاخه", OPT_MAX_BARS);
          snprintf(en, sizeof(en), "Bar capacity exceeded: %d bars", OPT_MAX_BARS);
          opt_calc_fail(fa, en);
          return;
        }
        placed = opt_calc_bars++;
        opt_calc_bar_used[placed] = 0;
        opt_calc_bar_angle[placed] = ang;
        work++;
      }
      opt_calc_bar_used[placed] += need;
      opt_calc_bar_of_tmp[i] = (uint16_t)placed;
      opt_calc_place_i++;
      opt_calc_scan_b = 0;
      work++;
    }
    ui_pct(40, 45, opt_calc_place_i, opt_calc_n);
    if (opt_calc_place_i < opt_calc_n) return;
    opt_calc_group_b = 0;
    opt_calc_group_i = 0;
    opt_calc_k = 0;
    opt_calc_stage5_prep = false;
    opt_calc_stage = 5;
    return;
  }

  if (opt_calc_stage == 5) {
    if (!opt_calc_stage5_prep) {
      for (int b = 0; b < opt_calc_bars; b++) {
        opt_calc_bar_cursor[b] = 0;
      }
      for (int i = 0; i < opt_calc_n; i++)
        opt_calc_bar_cursor[opt_calc_bar_of_tmp[i]]++;
      int pos = 0;
      for (int b = 0; b < opt_calc_bars; b++) {
        int c = opt_calc_bar_cursor[b];
        opt_calc_bar_cursor[b] = pos;
        pos += c;
      }
      opt_calc_place_i = 0;
      opt_calc_stage5_prep = true;
      opt_calc_update_modal(88);
      return;
    }
    while (budget_left() && opt_calc_place_i < opt_calc_n) {
      int i = opt_calc_place_i++;
      int b = (int)opt_calc_bar_of_tmp[i];
      int k = opt_calc_bar_cursor[b]++;
      opt_calc_final_len[k] = opt_calc_tmp[i].len;
      opt_calc_final_entry[k] = opt_calc_tmp[i].entry;
      opt_calc_final_bar[k] = (uint16_t)b;
      work++;
    }
    ui_pct(88, 11, opt_calc_place_i, opt_calc_n > 0 ? opt_calc_n : 1);
    if (opt_calc_place_i < opt_calc_n) return;

    opt_calc_k = opt_calc_place_i;
    if (opt_calc_k != opt_calc_n) {
      opt_calc_fail("خطای چیدمان بهینه — دوباره امتحان کنید",
                    "Optimizer packing mismatch — retry");
      return;
    }
    if (opt_calc_input_fp != opt_list_fingerprint()) {
      opt_calc_fail("لیست هنگام محاسبه تغییر کرد؛ دوباره تلاش کنید",
                    "List changed during optimization — retry");
      return;
    }
    // commit اتمیِ منطقی
    memcpy(OPT.piece_len, opt_calc_final_len,
           (size_t)opt_calc_n * sizeof(OPT.piece_len[0]));
    memcpy(OPT.piece_entry, opt_calc_final_entry,
           (size_t)opt_calc_n * sizeof(OPT.piece_entry[0]));
    memcpy(OPT.piece_bar, opt_calc_final_bar,
           (size_t)opt_calc_n * sizeof(OPT.piece_bar[0]));
    for (int b = 0; b < opt_calc_bars; b++) {
      OPT.bar_waste[b] = opt_calc_usable - opt_calc_bar_used[b];
      OPT.bar_angle[b] = opt_calc_bar_angle[b];  // scratch آزاد می‌شود → نسخهٔ پایدار در OPT
    }
    OPT.piece_count = opt_calc_n;
    OPT.bar_count = opt_calc_bars;
    OPT.cur = 0;
    OPT.badge_wait_pick = true;  // برنامهٔ جدید: تا انتخاب صریح شاخه، عدد شاخه «—» بماند
    memset(opt_piece_done, 0, sizeof(opt_piece_done));
    if (opt_calc_reset_cut_log) OPT.cut_log_n = 0;
    OPT.waiting_move = false;
    opt_plan_ready = true;
    opt_render_gen++;  // برنامهٔ جدید → کش رندر ردیف‌ها باطل
    opt_list_edit_dirty = false;
    opt_saved_list_fp = opt_calc_input_fp;
    opt_rebuild_bar_ranges();
    if (opt_calc_overflow) {
      char fa[96], en[88];
      snprintf(fa, sizeof(fa), "بیش از ظرفیت قطعه است؛ ظرفیت %d قطعه", OPT_MAX_PIECES);
      snprintf(en, sizeof(en), "Piece capacity exceeded: %d pieces", OPT_MAX_PIECES);
      ui_toast(fa, en, 4000);
    }
    opt_calc_update_modal(100);
    lv_timer_t *ocs = lv_timer_create([](lv_timer_t *t) { lv_timer_delete(t); opt_calc_close_modal(); }, 300, NULL);
    (void)ocs;
    opt_calc_free_scratch();
    opt_calc_active = false;
    opt_calc_stage = 0;
    GUI.lock_screen = false;
    // ریکاوری بعد از هنگ: اگر روی اصلی در مود بهینه بود، پنل را باز نکن
    if (ui_resume_opt_to_main) {
      ui_resume_opt_to_main = false;
      OPT.active = true;
      if (ui_resume_apply_opt_cur >= 0 && ui_resume_apply_opt_cur < OPT.piece_count)
        OPT.cur = (int)ui_resume_apply_opt_cur;
      (void)opt_load_current_to_main(false, false);
      ui_resume_opt_pending = false;
      ui_resume_opt_want_help = false;
    } else {
      opt_open_panel();
    }
  }
}

void prcs_opt_refresh() {  // در Task2 — بعد از هر برشِ تأییدشده (فلگ از Task8)
  if (opt_rebuild_req) {   // تغییر شاخه/kerf از روی پنل → بازچینی کامل
    opt_rebuild_req = false;
    if (!opt_calc_begin(false)) {
      if (opt_cont) {
        lv_obj_delete(opt_cont);
        opt_cont = NULL;
        OPT.active = false;
        GUI.lock_screen = false;
      }
    }
    return;
  }
  if (!OPT.flag_refresh) return;
  OPT.flag_refresh = false;
  // کاهش تعداد بعد از برش = همان برنامه؛ fp را با لیست هم‌تراز کن تا بازچینی نشود
  if (opt_plan_ready) {
    if (!opt_plan_still_valid()) {
      // لیست وسط کار خراب شده → از نو با حفظ لاگ (غیرمسدود)
      (void)opt_calc_begin(false);
    } else {
      opt_saved_list_fp = opt_list_fingerprint();
    }
  }
  if (OPT.cur >= OPT.piece_count) {
    // حتی اگر OPT.active قبلاً false شده باشد، از مود لیست بیرون بیا
    opt_finish_to_manual(true);
  } else if (OPT.active && !opt_cont) {
    opt_load_current_to_main(false, true);
  }
  if (opt_cont && lv_obj_is_valid(opt_cont)) {
    opt_refresh_labels();
    opt_done_box_sync();  // برش/پاک‌کردن جدید → فهرست «برش خورده» به‌روز
  }
  if (OPT.active || AUT.selected_count_val >= 0) opt_main_bar_lbl_sync();
  else opt_main_bar_lbl_hide();
}

static lv_obj_t *opt_help_modal = NULL;
static lv_obj_t *opt_help_cont = NULL;
static lv_obj_t *opt_help_body_box = NULL;

static bool opt_help_is_open() {
  return opt_help_cont && lv_obj_is_valid(opt_help_cont) && !lv_obj_has_flag(opt_help_cont, LV_OBJ_FLAG_HIDDEN);
}

static void opt_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  // برنامهٔ ناتمام را رها نکن — فقط پنل را ببند؛ کادر سبز شاخه بماند
  OPT.waiting_move = false;
  if (!(OPT.active && OPT.cur < OPT.piece_count)) {
    OPT.active = false;
    opt_main_bar_lbl_hide();
  } else {
    opt_main_bar_lbl_sync();
  }
  GUI.lock_screen = false;
  if (opt_cont) { lv_obj_delete_async(opt_cont); opt_cont = NULL; opt_clear_panel_ptrs(); }
  ui_resume_checkpoint();
}

static void opt_home_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (opt_help_is_open()) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  OPT.waiting_move = false;
  if (!(OPT.active && OPT.cur < OPT.piece_count)) {
    OPT.active = false;
    opt_main_bar_lbl_hide();
  }
  GUI.lock_screen = false;
  // اگر همین الان روی اصلی هستیم فقط پنل را ببند
  if (GUI.scr_id == 1) {
    if (opt_cont) { lv_obj_delete_async(opt_cont); opt_cont = NULL; opt_clear_panel_ptrs(); }
    if (OPT.active) opt_main_bar_lbl_sync();
    return;
  }
  // پنل را نگه دار تا صفحهٔ اصلی لود شود — وگرنه لحظه‌ای لیست برش دیده می‌شود
  opt_home_preserve_active = (OPT.active && OPT.cur < OPT.piece_count);
  GUI.pending_goto_scr = 1;
}

bool opt_consume_home_preserve() {
  bool v = opt_home_preserve_active;
  opt_home_preserve_active = false;
  return v;
}

static void opt_move_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (opt_help_is_open()) { GUI.buzzer_cmd_long = true; return; }
  OPT.active = true;
  OPT.badge_wait_pick = false;  // حرکت صریح از پنل = پذیرش قطعهٔ جاری → عدد نمایش داده شود
  opt_load_current_to_main(true, true);
}

static void opt_bar_press_feedback_cb(lv_event_t *e) {
  lv_obj_t *bid = (lv_obj_t *)lv_event_get_target(e);
  if (!bid) return;
  lv_event_code_t code = lv_event_get_code(e);
  int slot = -1;
  for (int i = 0; i < OPT_RENDER_ROWS; i++) {
    if (opt_badge_bar[i] == bid) { slot = i; break; }
  }
  int bar = (slot >= 0) ? opt_bar_for_slot(slot) : -1;
  const bool fin = (bar >= 0) && opt_bar_is_finished(bar);
  if (code == LV_EVENT_PRESSED) {
    opt_tap_arm_from_event(e);
    lv_obj_set_style_translate_y(bid, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bid, lv_color_hex(fin ? 0xE6B800 : 0x1C9B67), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    lv_obj_set_style_translate_y(bid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bid, lv_color_hex(fin ? 0xFFCC00 : 0x00572b), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void opt_bar_select_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (opt_help_is_open()) { GUI.buzzer_cmd_long = true; return; }
  if (!opt_tap_was_stationary()) return;
  int bar = opt_bar_for_slot((int)(intptr_t)lv_event_get_user_data(e));
  if (bar < 0) return;
  if (opt_bar_is_finished(bar)) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    ui_toast("این شاخه برش خورده است و نمی توانید این شاخه را انتخاب کنید",
             "This bar is already cut and cannot be selected", 3000);
    return;
  }
  int target_idx = 0;
  int pi0 = (bar >= 0 && bar < OPT_MAX_BARS) ? opt_bar_pi0[bar] : -1;
  int pn = (bar >= 0 && bar < OPT_MAX_BARS) ? opt_bar_pn[bar] : 0;
  if (pi0 >= 0 && pn > 0) {
    target_idx = pi0;
    for (int i = pi0; i < pi0 + pn; i++) {
      int en = OPT.piece_entry[i];
      if (en >= 0 && en < LIST_MAX && AUT.list_count_Val[en] > 0) {
        target_idx = i;
        break;
      }
    }
  } else {
    for (int i = 0; i < OPT.piece_count; i++) {
      if (OPT.piece_bar[i] == (uint16_t)bar) {
        target_idx = i;
        break;
      }
    }
  }
  OPT.active = true;            // انتخاب صریح شاخه توسط اپراتور
  OPT.cur = target_idx;
  OPT.badge_wait_pick = false;  // انتخاب صریح شاخه → عدد در صفحهٔ اصلی ظاهر شود
  opt_load_current_to_main(false, true);  // فقط بارگذاری عدد بدون حرکت موتور
  if (opt_cont) { lv_obj_delete_async(opt_cont); opt_cont = NULL; opt_clear_panel_ptrs(); }
  GUI.pending_goto_scr = 1;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
}

static void opt_bar_page_ui_sync() {
  // نوار انتخاب صفحه برداشته شد — فقط اسکرول
  if (opt_bar_page_prev_btn && lv_obj_is_valid(opt_bar_page_prev_btn))
    lv_obj_add_flag(opt_bar_page_prev_btn, LV_OBJ_FLAG_HIDDEN);
  if (opt_bar_page_next_btn && lv_obj_is_valid(opt_bar_page_next_btn))
    lv_obj_add_flag(opt_bar_page_next_btn, LV_OBJ_FLAG_HIDDEN);
  if (opt_bar_page_lbl && lv_obj_is_valid(opt_bar_page_lbl))
    lv_obj_add_flag(opt_bar_page_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void opt_bar_page_change(int delta_pages) {
  // هر صفحه = حداکثر OPT_RENDER_ROWS شاخه؛ تعویض نرم با ادامهٔ موقعیت اسکرول
  if (OPT.bar_count <= OPT_RENDER_ROWS) return;
  int last = ((OPT.bar_count - 1) / OPT_RENDER_ROWS) * OPT_RENDER_ROWS;
  if (last < 0) last = 0;
  int next = opt_bar_page_start + delta_pages * OPT_RENDER_ROWS;
  if (next < 0) next = 0;
  if (next > last) next = last;
  if (next == opt_bar_page_start) return;
  opt_bar_page_start = next;
  opt_bar_page_guard_ms = millis();
  opt_skip_next_scroll_end = true;
  opt_refresh_labels();
  opt_bar_page_ui_sync();
  ui_resume_checkpoint();
  if (opt_list_body && lv_obj_is_valid(opt_list_body)) {
    if (delta_pages < 0) {
      // برگشت به صفحهٔ قبل → از پایین همان صفحه ادامه بده (حس پیوستگی)
      lv_obj_scroll_to_y(opt_list_body, LV_COORD_MAX, LV_ANIM_OFF);
    } else {
      lv_obj_scroll_to_y(opt_list_body, 0, LV_ANIM_OFF);
    }
  }
}

static void opt_bar_page_prev_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  opt_bar_page_change(-1);
}
static void opt_bar_page_next_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  opt_bar_page_change(+1);
}

static void opt_title_jump_home_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  bool changed = (opt_bar_page_start != 0);
  if (changed) {
    opt_bar_page_start = 0;
    opt_bar_page_guard_ms = millis();
    opt_skip_next_scroll_end = true;
    opt_refresh_labels();
    opt_bar_page_ui_sync();
  }
  if (opt_list_body && lv_obj_is_valid(opt_list_body))
    lv_obj_scroll_to_y(opt_list_body, 0, LV_ANIM_ON);
  GUI.buzzer_cmd_short = true;
}

static void opt_bar_page_gesture_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
  lv_indev_t *indev = lv_event_get_indev(e);
  if (!indev) indev = lv_indev_active();
  if (!indev) return;
  if (OPT.bar_count <= OPT_RENDER_ROWS) return;
  lv_dir_t dir = lv_indev_get_gesture_dir(indev);
  if (dir == LV_DIR_TOP || dir == LV_DIR_LEFT) opt_bar_page_change(+1);
  else if (dir == LV_DIR_BOTTOM || dir == LV_DIR_RIGHT) opt_bar_page_change(-1);
}

static void opt_list_scroll_end_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_SCROLL_END) return;
  if (OPT.bar_count <= OPT_RENDER_ROWS) return;
  if (opt_skip_next_scroll_end) {
    opt_skip_next_scroll_end = false;
    return;
  }
  if ((uint32_t)(millis() - opt_bar_page_guard_ms) < 700) return;
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  if (!obj) return;
  if (lv_obj_get_scroll_bottom(obj) <= 0) {
    opt_bar_page_change(+1);
  } else if (lv_obj_get_scroll_top(obj) <= 0) {
    opt_bar_page_change(-1);
  }
}

static void opt_help_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (opt_help_modal && lv_obj_is_valid(opt_help_modal)) lv_obj_delete_async(opt_help_modal);
  opt_help_modal = NULL;
  opt_help_cont = NULL;
  opt_help_body_box = NULL;
  GUI.lock_screen = false;
  GUI.buzzer_cmd_short = true;
  ui_resume_checkpoint();
}

static void list_help_add_section(lv_obj_t *box, bool en, const char *head, const char *desc);

static void opt_help_show() {
  GUI.buzzer_cmd_short = true;
  if (opt_help_modal && lv_obj_is_valid(opt_help_modal)) {
    lv_obj_delete(opt_help_modal);
    opt_help_modal = NULL;
    opt_help_cont = NULL;
    opt_help_body_box = NULL;
  }

  const bool en = (GUI.system_language == 2);
  opt_help_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(opt_help_modal, 0, 0);
  lv_obj_set_size(opt_help_modal, 480, 320);
  lv_obj_set_style_bg_opa(opt_help_modal, LV_OPA_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_help_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_help_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_help_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_help_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(opt_help_modal, LV_OBJ_FLAG_CLICKABLE);

  opt_help_cont = lv_obj_create(opt_help_modal);
  lv_obj_set_size(opt_help_cont, 440, 254);
  lv_obj_align(opt_help_cont, LV_ALIGN_CENTER, 0, 23);
  lv_obj_set_style_bg_color(opt_help_cont, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(opt_help_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_help_cont, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_help_cont, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(opt_help_cont, lv_color_hex(0xd7dde2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_help_cont, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_help_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *close = lv_button_create(opt_help_cont);
  lv_obj_set_size(close, 42, 30);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(close, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(close, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, opt_help_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *xl = lv_label_create(close);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(xl, "X");
  lv_obj_center(xl);

  lv_obj_t *title = lv_label_create(opt_help_cont);
  lv_obj_set_pos(title, 54, 0);
  lv_obj_set_size(title, 350, 34);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(title, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(title, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(title, en ? "Cut Optimization Help" : "راهنمای بهینه برش");

  lv_obj_t *body_box = lv_obj_create(opt_help_cont);
  opt_help_body_box = body_box;
  lv_obj_set_pos(body_box, 8, 42);
  lv_obj_set_size(body_box, 404, 180);
  lv_obj_set_style_bg_opa(body_box, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(body_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(body_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(body_box, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scroll_dir(body_box, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(body_box, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(body_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(body_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(body_box, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

  if (en) {
    list_help_add_section(body_box, en, "Purpose", "Packs saved cut-list lengths into stock bars to reduce scrap.");
    list_help_add_section(body_box, en, "Green badge", "Stock bar number. Tap only this badge to load that bar.");
    list_help_add_section(body_box, en, "Red badge", "Waste left on that bar after packing.");
    list_help_add_section(body_box, en, "Yellow size", "Next cut sent to the main screen as the active target.");
    list_help_add_section(body_box, en, "Grey struck sizes", "Cuts already confirmed. Each size shows as length times quantity on one line.");
    list_help_add_section(body_box, en, "Confirm / Skip / Home", "Confirm advances to the next cut and reduces list quantity. Skip passes the active cut. Home closes this panel.");
    list_help_add_section(body_box, en, "Print", "Prints the cut plan for the bars shown here.");
  } else {
    list_help_add_section(body_box, en, "هدف صفحه", "متراژهای لیست برش را داخل شاخه ها می چیند تا پرت کمتر شود.");
    list_help_add_section(body_box, en, "کادر سبز", "شماره شاخه است. فقط روی همین کادر بزنید تا همان شاخه انتخاب شود.");
    list_help_add_section(body_box, en, "کادر قرمز", "مقدار پرت باقی مانده همان شاخه را نشان می دهد.");
    list_help_add_section(body_box, en, "عدد زرد", "برش بعدی است که به صفحه اصلی فرستاده می شود.");
    list_help_add_section(body_box, en, "عددهای خاکستری", "برش های تاییدشده هستند. هر سایز به شکل متراژ ضربدر تعداد در یک خط نوشته می شود.");
    list_help_add_section(body_box, en, "تایید و رد و خانه", "با تایید برش بعدی فعال می شود و تعداد در لیست کم می شود. رد فقط از برش فعال می گذرد. خانه پنل را می بندد.");
    list_help_add_section(body_box, en, "چاپ", "برنامه برش همین شاخه ها را برای چاپ می فرستد.");
  }

  GUI.lock_screen = true;
  lv_obj_clear_flag(opt_help_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(opt_help_modal);
  ui_resume_checkpoint();
}

static void opt_help_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  opt_help_show();
}

static void opt_skip_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (opt_help_is_open()) { GUI.buzzer_cmd_long = true; return; }
  if (OPT.cur >= OPT.piece_count) return;
  GUI.buzzer_cmd_short = true;
  if (OPT.cut_log_n < OPT_CUT_LOG_MAX) {
    OPT.cut_log_mm[OPT.cut_log_n] = OPT.piece_len[OPT.cur];
    OPT.cut_log_bar[OPT.cut_log_n] = (int16_t)OPT.piece_bar[OPT.cur];
    OPT.cut_log_n++;
  }
  OPT.piece_done[OPT.cur] = true;
  OPT.cur = OPT.cur + 1;
  OPT.waiting_move = false;
  OPT.flag_refresh = true;
  opt_done_box_sync();
}

// ---- پاک کردن لاگ برش‌خورده / انتخاب پرینت ----
static lv_obj_t *opt_done_purge_modal = NULL;
static lv_obj_t *opt_done_purge_bar = NULL;
static uint32_t opt_done_purge_start_ms = 0;
static const uint32_t OPT_DONE_PURGE_MS = 4500;
static lv_obj_t *opt_print_choice_modal = NULL;
static lv_obj_t *opt_print_choice_bar = NULL;

static void opt_done_purge_close(bool do_clear) {
  // async: کال‌بک روی همین مودال ایستاده است — حذف هم‌زمان UAF می‌شود
  if (opt_done_purge_modal && lv_obj_is_valid(opt_done_purge_modal)) lv_obj_delete_async(opt_done_purge_modal);
  opt_done_purge_modal = NULL;
  opt_done_purge_bar = NULL;
  GUI.lock_screen = false;
  ui_help_suppress_release = true;
  if (do_clear) {
    OPT.cut_log_n = 0;
    opt_done_box_sync();
    GUI.buzzer_cmd_short = true;
  }
}

// ---- حذف برنامهٔ یک شاخهٔ تمام‌شده ----
static lv_obj_t *opt_bar_purge_modal = NULL;
static lv_obj_t *opt_bar_purge_barfill = NULL;
static uint32_t opt_bar_purge_start_ms = 0;
static int opt_bar_purge_target = -1;
static const uint32_t OPT_BAR_PURGE_MS = 4500;

static void opt_delete_finished_bar(int bar) {
  if (bar < 0 || bar >= OPT.bar_count) return;
  // لاگ برش‌خوردهٔ همین شاخه
  int w = 0;
  for (int i = 0; i < OPT.cut_log_n; i++) {
    if (OPT.cut_log_bar[i] == (int16_t)bar) continue;
    OPT.cut_log_mm[w] = OPT.cut_log_mm[i];
    int16_t b = OPT.cut_log_bar[i];
    if (b > bar) b--;
    OPT.cut_log_bar[w] = b;
    w++;
  }
  OPT.cut_log_n = w;
  // قطعات برنامه
  int pc = 0;
  int new_cur = 0;
  for (int i = 0; i < OPT.piece_count; i++) {
    if (OPT.piece_bar[i] == (uint16_t)bar) continue;
    if (i < OPT.cur) new_cur++;
    OPT.piece_len[pc] = OPT.piece_len[i];
    OPT.piece_entry[pc] = OPT.piece_entry[i];
    uint16_t pb = OPT.piece_bar[i];
    if (pb > (uint16_t)bar) pb--;
    OPT.piece_bar[pc] = pb;
    pc++;
  }
  OPT.cur = new_cur;
  OPT.piece_count = pc;
  for (int b = bar; b < OPT.bar_count - 1; b++) OPT.bar_waste[b] = OPT.bar_waste[b + 1];
  if (OPT.bar_count > 0) OPT.bar_count--;
  opt_plan_ready = (OPT.piece_count > 0);
  opt_saved_list_fp = opt_list_fingerprint();
  opt_rebuild_bar_ranges();
  if (opt_cont) {
    opt_open_panel();
  } else {
    opt_done_box_sync();
  }
}

static void opt_bar_purge_close(bool do_del) {
  int bar = opt_bar_purge_target;
  // async: کال‌بک روی همین مودال ایستاده است — حذف هم‌زمان UAF می‌شود
  if (opt_bar_purge_modal && lv_obj_is_valid(opt_bar_purge_modal)) lv_obj_delete_async(opt_bar_purge_modal);
  opt_bar_purge_modal = NULL;
  opt_bar_purge_barfill = NULL;
  opt_bar_purge_target = -1;
  GUI.lock_screen = false;
  ui_help_suppress_release = true;
  if (do_del && bar >= 0) {
    GUI.buzzer_cmd_short = true;
    opt_delete_finished_bar(bar);
  }
}

static void opt_bar_purge_yes_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  opt_bar_purge_close(true);
}
static void opt_bar_purge_no_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  opt_bar_purge_close(false);
}
static void opt_bar_purge_card_eat_cb(lv_event_t *e) { lv_event_stop_bubbling(e); }

static void opt_bar_purge_show(int bar) {
  if (!opt_bar_is_finished(bar)) { GUI.buzzer_cmd_long = true; return; }
  if (opt_bar_purge_modal && lv_obj_is_valid(opt_bar_purge_modal)) opt_bar_purge_close(false);
  const bool fa = (GUI.system_language != 2);
  GUI.buzzer_cmd_short = true;
  opt_bar_purge_target = bar;

  opt_bar_purge_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(opt_bar_purge_modal, 0, 0);
  lv_obj_set_size(opt_bar_purge_modal, 480, 320);
  lv_obj_set_style_bg_opa(opt_bar_purge_modal, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_bar_purge_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_bar_purge_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_bar_purge_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_bar_purge_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(opt_bar_purge_modal, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(opt_bar_purge_modal, opt_bar_purge_no_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(opt_bar_purge_modal);
  lv_obj_set_size(card, 400, 168);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, -8);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, opt_bar_purge_card_eat_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *close = lv_button_create(card);
  lv_obj_set_size(close, 36, 32);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, opt_bar_purge_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(close);
  lv_label_set_text(cl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cl);

  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 340);
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 10, 8);
  lv_obj_set_style_text_font(msg, fa ? &ui_font_naz20b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(msg, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(msg, fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(msg, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  lv_label_set_text(msg, fa ? "میخواهید این لیست برش را حذف کنید؟"
                             : "Delete this finished bar cut list?");

  lv_obj_t *yes = lv_button_create(card);
  lv_obj_set_size(yes, 120, 44);
  lv_obj_align(yes, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
  lv_obj_set_style_bg_color(yes, lv_color_hex(0x34C759), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(yes, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(yes, opt_bar_purge_yes_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *yl = lv_label_create(yes);
  lv_obj_set_style_text_font(yl, fa ? &ui_font_naz20b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(yl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(yl, fa ? "بله" : "Yes");
  lv_obj_center(yl);

  lv_obj_t *no = lv_button_create(card);
  lv_obj_set_size(no, 120, 44);
  lv_obj_align(no, LV_ALIGN_BOTTOM_LEFT, 8, -8);
  lv_obj_set_style_bg_color(no, lv_color_hex(0x8E8E93), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(no, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(no, opt_bar_purge_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *nl = lv_label_create(no);
  lv_obj_set_style_text_font(nl, fa ? &ui_font_naz20b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(nl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(nl, fa ? "خیر" : "No");
  lv_obj_center(nl);

  opt_bar_purge_barfill = lv_bar_create(card);
  lv_obj_set_size(opt_bar_purge_barfill, 360, 8);
  lv_obj_align(opt_bar_purge_barfill, LV_ALIGN_BOTTOM_MID, 0, -58);
  lv_bar_set_range(opt_bar_purge_barfill, 0, 100);
  lv_bar_set_value(opt_bar_purge_barfill, 100, LV_ANIM_OFF);
  opt_bar_purge_start_ms = lv_tick_get();
  GUI.lock_screen = true;
}

static void opt_done_purge_yes_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  opt_done_purge_close(true);
}
static void opt_done_purge_no_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  opt_done_purge_close(false);
}
static void opt_done_purge_card_eat_cb(lv_event_t *e) { lv_event_stop_bubbling(e); }

static void opt_done_purge_show() {
  if (OPT.cut_log_n <= 0) { GUI.buzzer_cmd_long = true; return; }
  if (opt_done_purge_modal && lv_obj_is_valid(opt_done_purge_modal)) opt_done_purge_close(false);
  const bool fa = (GUI.system_language != 2);
  GUI.buzzer_cmd_short = true;

  opt_done_purge_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(opt_done_purge_modal, 0, 0);
  lv_obj_set_size(opt_done_purge_modal, 480, 320);
  lv_obj_set_style_bg_opa(opt_done_purge_modal, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_done_purge_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_done_purge_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_done_purge_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_done_purge_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(opt_done_purge_modal, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(opt_done_purge_modal, opt_done_purge_no_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(opt_done_purge_modal);
  lv_obj_set_size(card, 400, 168);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, -8);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, opt_done_purge_card_eat_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *close = lv_button_create(card);
  lv_obj_set_size(close, 36, 32);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, opt_done_purge_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(close);
  lv_label_set_text(cl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cl);

  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 340);
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 10, 8);
  lv_obj_set_style_text_font(msg, fa ? &ui_font_naz20b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(msg, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(msg, fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(msg, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  lv_label_set_text(msg, fa ? "این ها برش خورده اند. نیاز به حذف دارند؟"
                            : "These sizes are already cut. Delete the log?");

  lv_obj_t *yes = lv_button_create(card);
  lv_obj_set_size(yes, 140, 40);
  lv_obj_align(yes, LV_ALIGN_BOTTOM_RIGHT, -4, -28);
  lv_obj_set_style_bg_color(yes, lv_color_hex(0xB90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(yes, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(yes, opt_done_purge_yes_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *yl = lv_label_create(yes);
  lv_label_set_text(yl, fa ? "حذف" : "Delete");
  lv_obj_set_style_text_font(yl, fa ? &ui_font_naz18b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(yl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(yl);

  opt_done_purge_bar = lv_bar_create(card);
  lv_obj_set_size(opt_done_purge_bar, 360, 8);
  lv_obj_align(opt_done_purge_bar, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_bar_set_range(opt_done_purge_bar, 0, 100);
  lv_bar_set_value(opt_done_purge_bar, 100, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(opt_done_purge_bar, lv_color_hex(0xE5E5EA), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_done_purge_bar, lv_color_hex(0x0A84FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);

  opt_done_purge_start_ms = lv_tick_get();
  GUI.lock_screen = true;
  lv_obj_move_foreground(opt_done_purge_modal);
}

static void opt_done_box_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  if (opt_help_is_open()) { GUI.buzzer_cmd_long = true; return; }
  opt_done_purge_show();
}

static void opt_print_choice_close() {
  // async: کال‌بک روی همین مودال ایستاده است — حذف هم‌زمان UAF می‌شود
  if (opt_print_choice_modal && lv_obj_is_valid(opt_print_choice_modal)) lv_obj_delete_async(opt_print_choice_modal);
  opt_print_choice_modal = NULL;
  opt_print_choice_bar = NULL;
  GUI.lock_screen = false;
  ui_help_suppress_release = true;
}
static void opt_print_choice_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  opt_print_choice_close();
}
static void opt_print_choice_plan_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  opt_print_choice_close();
  IOS.opt_print_request = true;  // سایزهای باقی‌مانده (هنوز نبریده)
  GUI.buzzer_cmd_short = true;
  ui_toast("چاپ سایزهای باقی‌مانده", "Printing remaining sizes", 1400);
}
static void opt_print_choice_done_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  opt_print_choice_close();
  IOS.opt_print_done_request = true;
  GUI.buzzer_cmd_short = true;
  ui_toast("چاپ سایزهای بریده‌شده", "Printing cut sizes", 1400);
}
static void opt_print_choice_card_eat_cb(lv_event_t *e) { lv_event_stop_bubbling(e); }

static void opt_print_choice_show() {
  if (opt_print_choice_modal && lv_obj_is_valid(opt_print_choice_modal)) opt_print_choice_close();
  const bool fa = (GUI.system_language != 2);
  GUI.buzzer_cmd_short = true;

  opt_print_choice_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(opt_print_choice_modal, 0, 0);
  lv_obj_set_size(opt_print_choice_modal, 480, 320);
  lv_obj_set_style_bg_opa(opt_print_choice_modal, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_print_choice_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_print_choice_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_print_choice_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_print_choice_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(opt_print_choice_modal, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(opt_print_choice_modal, opt_print_choice_cancel_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(opt_print_choice_modal);
  lv_obj_set_size(card, 428, 218);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, -4);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, opt_print_choice_card_eat_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *close = lv_button_create(card);
  lv_obj_set_size(close, 36, 32);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, opt_print_choice_cancel_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(close);
  lv_label_set_text(cl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cl);

  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 380);
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_set_style_text_font(msg, fa ? &ui_font_naz20 : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(msg, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(msg, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  lv_label_set_text(msg, fa
      ? "انتخاب گزارش چاپ:\nمایل به چاپ کدام لیست هستید؟"
      : "Print Report Choice:\nWhich list would you like to print?");

  lv_obj_t *btn_plan = lv_button_create(card);
  lv_obj_set_size(btn_plan, 196, 52);
  lv_obj_align(btn_plan, LV_ALIGN_BOTTOM_LEFT, 4, -20);
  lv_obj_set_style_bg_color(btn_plan, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_plan, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_plan, opt_print_choice_plan_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lp = lv_label_create(btn_plan);
  lv_label_set_text(lp, fa ? "قطعات باقی‌مانده" : "Remaining Plan");
  lv_obj_set_style_text_font(lp, fa ? &ui_font_naz20 : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lp, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(lp);

  lv_obj_t *btn_done = lv_button_create(card);
  lv_obj_set_size(btn_done, 196, 52);
  lv_obj_align(btn_done, LV_ALIGN_BOTTOM_RIGHT, -4, -20);
  lv_obj_set_style_bg_color(btn_done, lv_color_hex(0x1C9B67), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_done, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_done, opt_print_choice_done_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *ld = lv_label_create(btn_done);
  lv_label_set_text(ld, fa ? "قطعات برش‌خورده" : "Cut History");
  lv_obj_set_style_text_font(ld, fa ? &ui_font_naz20 : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ld, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ld);

  opt_print_choice_bar = lv_bar_create(card);
  lv_obj_set_size(opt_print_choice_bar, 380, 8);
  lv_obj_align(opt_print_choice_bar, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_bar_set_range(opt_print_choice_bar, 0, 100);
  lv_bar_set_value(opt_print_choice_bar, 100, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(opt_print_choice_bar, lv_color_hex(0xE5E5EA), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_print_choice_bar, lv_color_hex(0x0A84FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_flag(opt_print_choice_bar, LV_OBJ_FLAG_HIDDEN);  // نوار شمارش معکوس حذف شد

  GUI.lock_screen = true;
  lv_obj_move_foreground(opt_print_choice_modal);
}

void prcs_opt_done_modals() {
  if (opt_done_purge_modal && lv_obj_is_valid(opt_done_purge_modal)) {
    uint32_t elapsed = lv_tick_elaps(opt_done_purge_start_ms);
    if (elapsed >= OPT_DONE_PURGE_MS) {
      opt_done_purge_close(false);
    } else if (opt_done_purge_bar) {
      int remain = (int)(100UL - (elapsed * 100UL / OPT_DONE_PURGE_MS));
      if (remain < 0) remain = 0;
      lv_bar_set_value(opt_done_purge_bar, remain, LV_ANIM_OFF);
    }
  }
  if (opt_bar_purge_modal && lv_obj_is_valid(opt_bar_purge_modal)) {
    uint32_t elapsed = lv_tick_elaps(opt_bar_purge_start_ms);
    if (elapsed >= OPT_BAR_PURGE_MS) {
      opt_bar_purge_close(false);
    } else if (opt_bar_purge_barfill) {
      int remain = (int)(100UL - (elapsed * 100UL / OPT_BAR_PURGE_MS));
      if (remain < 0) remain = 0;
      lv_bar_set_value(opt_bar_purge_barfill, remain, LV_ANIM_OFF);
    }
  }
  // انتخاب پرینت دیگر poll/حذف خودکار ندارد — فقط دکمه یا لمس پس‌زمینه می‌بندد
}

static void opt_print_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (opt_help_is_open()) { GUI.buzzer_cmd_long = true; return; }
  if (OPT.bar_count <= 0 || OPT.piece_count <= 0) {
    GUI.buzzer_cmd_long = true;
    ui_toast("چیزی برای پرینت نیست", "Nothing to print", 1500);
    return;
  }
  if (OPT.cut_log_n <= 0) {
    // هنوز برشی انجام نشده → چاپ مستقیم برنامه برش بدون پرسش اضافی
    IOS.opt_print_request = true;
    GUI.buzzer_cmd_short = true;
    ui_toast("در حال چاپ لیست برش بهینه...", "Printing cut plan...", 2000);
    return;
  }
  opt_print_choice_show();
}

static lv_obj_t *opt_mk_btn(lv_obj_t *parent, const char *txt, uint32_t color, lv_event_cb_t cb, int x) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_size(btn, 140, 44);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, x, -6);
  lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *l = lv_label_create(btn);
  lv_obj_set_style_text_font(l, (GUI.system_language == 1) ? &ui_font_naz24 : &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(l);
  if (x == 4) lv_label_set_text(l, (GUI.system_language == 1) ? "حرکت" : "Move");
  else if (x == 154) lv_label_set_text(l, (GUI.system_language == 1) ? "رد" : "Skip");
  else if (x == 304) lv_label_set_text(l, (GUI.system_language == 1) ? "خروج" : "Exit");
  else lv_label_set_text(l, txt);
  return btn;
}

static void opt_open_panel_build() {
  if (opt_cont) { lv_obj_delete_async(opt_cont); opt_cont = NULL; }
  opt_cont = lv_obj_create(lv_layer_top());
  lv_obj_set_size(opt_cont, 480, 320);
  lv_obj_set_pos(opt_cont, 0, 0);
  lv_obj_set_style_bg_color(opt_cont, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *home = lv_button_create(opt_cont);
  // خانه/راهنما/عنوان/چاپ: −۸x −۹y
  lv_obj_set_pos(home, -8, -9);
  lv_obj_set_size(home, 44, 39);
  lv_obj_set_style_bg_color(home, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(home, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(home, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(home, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(home, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(home, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_event_cb(home, opt_home_cb, LV_EVENT_RELEASED, NULL);
  list_hdr_button_icon(home, LV_SYMBOL_HOME, &lv_font_montserrat_26, 0);

  lv_obj_t *help = lv_button_create(opt_cont);
  lv_obj_set_pos(help, 42, -9);
  lv_obj_set_size(help, 44, 39);
  lv_obj_set_style_bg_color(help, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(help, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(help, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(help, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(help, opt_help_cb, LV_EVENT_RELEASED, NULL);
  f1_hdr_set_book_icon(help);

  lv_obj_t *title_box = lv_obj_create(opt_cont);
  lv_obj_set_pos(title_box, 93, -7);
  lv_obj_set_size(title_box, 198, 36);
  lv_obj_clear_flag(title_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(title_box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(title_box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(title_box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(title_box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(title_box, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_event_cb(title_box, opt_title_jump_home_cb, LV_EVENT_RELEASED, NULL);

  // عنوان
  lv_obj_t *title = lv_label_create(title_box);
  lv_obj_set_style_text_font(title, (GUI.system_language == 1) ? &ui_font_naz30 : &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_x(title, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_y(title, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_x(title, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_y(title, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_size(title, 198, 36);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -6);  // وسط کادر سفید
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(title, (GUI.system_language == 1) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
  lv_label_set_text(title, (GUI.system_language == 1) ? "بهینه برش" : "Cut Plan");
  // خلاصه
  opt_lbl_summary = lv_label_create(opt_cont);
  lv_obj_set_style_text_font(opt_lbl_summary, (GUI.system_language == 1) ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(opt_lbl_summary, 352, 0);
  lv_obj_set_size(opt_lbl_summary, 110, 32);
  lv_obj_set_style_text_align(opt_lbl_summary, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_lbl_summary, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(opt_lbl_summary, LV_LABEL_LONG_CLIP);
  lv_label_set_text(opt_lbl_summary, "");  // مخفی — متن پیش‌فرض "Text" نداشته باشد
  lv_obj_add_flag(opt_lbl_summary, LV_OBJ_FLAG_HIDDEN);
  // شاخه/برش داخل پنل هدر مات — ارتفاع کافی برای naz (نصف پایین نبرد)
  lv_obj_t *opt_stats = lv_obj_create(opt_cont);
  lv_obj_set_pos(opt_stats, 342, -6);
  lv_obj_set_size(opt_stats, 136, 64);
  lv_obj_clear_flag(opt_stats, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(opt_stats, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(opt_stats, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_stats, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_stats, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_stats, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(opt_stats, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(opt_stats, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

  opt_lbl_bar_title = lv_label_create(opt_stats);
  lv_obj_set_pos(opt_lbl_bar_title, -4, -6);  // ۴px چپ
  lv_obj_set_size(opt_lbl_bar_title, 58, 26);
  lv_obj_set_style_text_font(opt_lbl_bar_title, (GUI.system_language == 1) ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_lbl_bar_title, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(opt_lbl_bar_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_lbl_bar_title, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(opt_lbl_bar_title, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_label_set_long_mode(opt_lbl_bar_title, LV_LABEL_LONG_CLIP);

  opt_lbl_bar_value = lv_label_create(opt_stats);
  lv_obj_set_pos(opt_lbl_bar_value, -4, 16);  // ۲px پایین‌تر به خواسته مالک (۱۴ -> ۱۶)
  lv_obj_set_size(opt_lbl_bar_value, 58, 30);
  lv_obj_set_style_text_font(opt_lbl_bar_value, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_lbl_bar_value, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(opt_lbl_bar_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_lbl_bar_value, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_lbl_bar_value, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_lbl_bar_value, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(opt_lbl_bar_value, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_label_set_long_mode(opt_lbl_bar_value, LV_LABEL_LONG_CLIP);

  opt_lbl_cut_title = lv_label_create(opt_stats);
  lv_obj_set_pos(opt_lbl_cut_title, 41, -8);  // ۴px چپ نسبت به ۴۵
  lv_obj_set_size(opt_lbl_cut_title, 84, 26);
  lv_obj_set_style_text_font(opt_lbl_cut_title, (GUI.system_language == 1) ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_lbl_cut_title, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(opt_lbl_cut_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_lbl_cut_title, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(opt_lbl_cut_title, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_label_set_long_mode(opt_lbl_cut_title, LV_LABEL_LONG_CLIP);

  opt_lbl_cut_value = lv_label_create(opt_stats);
  lv_obj_set_pos(opt_lbl_cut_value, 41, 16);  // ۲px پایین‌تر به خواسته مالک (۱۴ -> ۱۶)
  lv_obj_set_size(opt_lbl_cut_value, 84, 30);
  lv_obj_set_style_text_font(opt_lbl_cut_value, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_lbl_cut_value, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(opt_lbl_cut_value, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(opt_lbl_cut_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_lbl_cut_value, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(opt_lbl_cut_value, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(opt_lbl_cut_value, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(opt_lbl_cut_value, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_label_set_long_mode(opt_lbl_cut_value, LV_LABEL_LONG_CLIP);
  // قطعهٔ جاری (درشت)
  opt_lbl_current = lv_label_create(opt_cont);
  lv_obj_set_style_text_font(opt_lbl_current, (GUI.system_language == 1) ? &ui_font_naz24 : &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(opt_lbl_current, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(opt_lbl_current, 54, 41);
  lv_obj_set_size(opt_lbl_current, 370, 30);
  lv_obj_set_style_text_align(opt_lbl_current, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(opt_lbl_current, (GUI.system_language == 1) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(opt_lbl_current, LV_LABEL_LONG_CLIP);
  lv_label_set_text(opt_lbl_current, "");  // مخفی — متن پیش‌فرض "Text" نداشته باشد
  lv_obj_add_flag(opt_lbl_current, LV_OBJ_FLAG_HIDDEN);
  // بدنهٔ لیست شاخه‌ها — اسکرول عمودی؛ صفحه با ته/سر اسکرول عوض می‌شود
  opt_list_body = lv_obj_create(opt_cont);
  lv_obj_set_size(opt_list_body, 460, 250);  // تمام ارتفاع تا کف (نوار صفحه حذف شد)
  lv_obj_align(opt_list_body, LV_ALIGN_TOP_MID, 0, 62);
  lv_obj_set_flex_flow(opt_list_body, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_top(opt_list_body, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(opt_list_body, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(opt_list_body, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(opt_list_body, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(opt_list_body, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(opt_list_body, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(opt_list_body, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(opt_list_body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(opt_list_body, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(opt_list_body, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scrollbar_mode(opt_list_body, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_scroll_dir(opt_list_body, LV_DIR_VER);
  lv_obj_add_flag(opt_list_body, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(opt_list_body, LV_OBJ_FLAG_SCROLL_ELASTIC); // بدون لگ انیمیشن فنری
  lv_obj_clear_flag(opt_list_body, LV_OBJ_FLAG_SCROLL_MOMENTUM); // اسکرول کاملاً سبک و سریع
  lv_obj_add_event_cb(opt_list_body, opt_bar_page_gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(opt_list_body, opt_list_scroll_end_cb, LV_EVENT_SCROLL_END, NULL);
  // دکمه‌های صفحه مخفی (فقط اسکرول)
  opt_bar_page_prev_btn = lv_button_create(opt_cont);
  lv_obj_set_pos(opt_bar_page_prev_btn, 12, 292);
  lv_obj_set_size(opt_bar_page_prev_btn, 44, 28);
  lv_obj_add_flag(opt_bar_page_prev_btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(opt_bar_page_prev_btn, opt_bar_page_prev_cb, LV_EVENT_RELEASED, NULL);
  opt_bar_page_next_btn = lv_button_create(opt_cont);
  lv_obj_set_pos(opt_bar_page_next_btn, 200, 292);
  lv_obj_set_size(opt_bar_page_next_btn, 44, 28);
  lv_obj_add_flag(opt_bar_page_next_btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(opt_bar_page_next_btn, opt_bar_page_next_cb, LV_EVENT_RELEASED, NULL);
  opt_bar_page_lbl = lv_label_create(opt_cont);
  lv_obj_set_pos(opt_bar_page_lbl, 58, 294);
  lv_obj_set_size(opt_bar_page_lbl, 140, 24);
  lv_obj_add_flag(opt_bar_page_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(opt_bar_page_lbl, "");
  lv_obj_t *print_btn = lv_button_create(opt_cont);
  lv_obj_set_pos(print_btn, 297, -8);
  lv_obj_set_size(print_btn, 38, 38);
  lv_obj_set_style_bg_color(print_btn, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(print_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(print_btn, 9, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(print_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(print_btn, opt_print_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *print_icon = lv_image_create(print_btn);
  lv_image_set_src(print_icon, &img_printer);
  lv_obj_set_size(print_icon, 26, 26);
  lv_obj_center(print_icon);
  opt_done_box = NULL;
  opt_done_lbl = NULL;
  opt_done_body = NULL;
  for (int b = 0; b < OPT_RENDER_ROWS; b++) {
    opt_bar_rows[b] = NULL;
    opt_badge_bar[b] = NULL;
    opt_badge_waste[b] = NULL;
    opt_badge_waste_num[b] = NULL;
    opt_badge_bar_num[b] = NULL;
    opt_badge_bar_xmark[b] = NULL;
    opt_piece_done_lbl[b] = NULL;
    opt_piece_current_lbl[b] = NULL;
    opt_piece_lbl[b] = NULL;
  }
  // کادر اول لیست: ابعاد برش‌خورده یکی‌یکی (بدون تعداد)
  {
    const bool fa = (GUI.system_language == 1);
    opt_done_box = lv_obj_create(opt_list_body);
    lv_obj_set_width(opt_done_box, 441);
    lv_obj_set_height(opt_done_box, LV_SIZE_CONTENT);
    lv_obj_clear_flag(opt_done_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(opt_done_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(opt_done_box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(opt_done_box, lv_color_hex(0xf0f4f8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(opt_done_box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(opt_done_box, lv_color_hex(0x8E8E93), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(opt_done_box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(opt_done_box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(opt_done_box, opt_done_box_click_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *tit = lv_label_create(opt_done_box);
    lv_obj_set_width(tit, 420);
    lv_obj_set_style_text_font(tit, fa ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(tit, lv_color_hex(0x6f7780), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(tit, fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(tit, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(tit, fa ? "برش خورده ها" : "Already cut");

    opt_done_lbl = lv_label_create(opt_done_box);
    lv_obj_set_width(opt_done_lbl, 420);
    lv_obj_set_style_text_font(opt_done_lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(opt_done_lbl, lv_color_hex(0x3a3a3c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(opt_done_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(opt_done_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(opt_done_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_align(opt_done_lbl, LV_ALIGN_TOP_LEFT, 0, 22);

    opt_done_body = lv_obj_create(opt_done_box);
    lv_obj_set_width(opt_done_body, 420);
    lv_obj_set_height(opt_done_body, LV_SIZE_CONTENT);
    lv_obj_align(opt_done_body, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_obj_set_style_bg_opa(opt_done_body, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(opt_done_body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(opt_done_body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(opt_done_body, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(opt_done_body, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(opt_done_body, LV_OBJ_FLAG_SCROLLABLE);
  }
  opt_bar_page_start = 0;
  int render_count = OPT.bar_count;
  if (render_count > OPT_RENDER_ROWS) render_count = OPT_RENDER_ROWS;
  for (int b = 0; b < render_count; b++) {
    lv_obj_t *row = lv_obj_create(opt_list_body);
    if (!row) break;
    opt_bar_rows[b] = row;
    // بدون translate — translate ضلع راست/پایین را از کلیپ اسکرول می‌بُرید
    lv_obj_set_width(row, 441);  // +۵ راست
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(row, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(row, lv_color_hex((b & 1) ? 0xf7f8fb : 0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(row, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(row, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(row, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    // بج زاویه سمت چپ شاخه (۴۵° یا ۹۰°) — افزایش ۱۰٪ محور X
    lv_obj_t *ang_badge = lv_label_create(row);
    opt_badge_angle[b] = ang_badge;
    lv_obj_set_pos(ang_badge, 1, 1);
    lv_obj_set_size(ang_badge, 33, 48);
    lv_obj_set_style_radius(ang_badge, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ang_badge, lv_color_hex((b < OPT_MAX_BARS && OPT.bar_angle[b] == 90) ? 0xd97706 : 0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ang_badge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ang_badge, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ang_badge, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ang_badge, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ang_badge, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ang_badge, (b < OPT_MAX_BARS && OPT.bar_angle[b] == 90) ? "90°" : "45°");

    lv_obj_t *bid = lv_label_create(row);
    opt_badge_bar[b] = bid;
    lv_obj_set_pos(bid, 38, 1);  // ۳px به سمت راست (فاصله مناسب از بج زاویه)
    lv_obj_set_size(bid, 44, 48);
    lv_obj_set_style_radius(bid, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bid, lv_color_hex(0x00572b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bid, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bid, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bid, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(bid, 2, LV_PART_MAIN | LV_STATE_DEFAULT);  // نوشته شاخه ۲px بالا
    lv_obj_set_style_pad_bottom(bid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(bid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(bid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bid, (GUI.system_language == 1) ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(bid, (GUI.system_language == 1) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    // عنوان فارسی جدا از رقم (رقم روی naz → مربع)
    lv_label_set_text(bid, (GUI.system_language == 1) ? "شاخه" : "Bar");
    {
      lv_obj_t *bn = lv_label_create(bid);
      opt_badge_bar_num[b] = bn;
      lv_obj_set_style_text_font(bn, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(bn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(bn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text_fmt(bn, "%d", b + 1);
      lv_obj_align(bn, LV_ALIGN_BOTTOM_MID, 0, -2);
    }
    {
      lv_obj_t *xm = lv_label_create(bid);
      opt_badge_bar_xmark[b] = xm;
      lv_obj_set_style_text_font(xm, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(xm, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_opa(xm, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(xm, LV_SYMBOL_CLOSE);
      lv_obj_center(xm);
      lv_obj_add_flag(xm, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(xm, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_add_flag(bid, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bid, opt_bar_press_feedback_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(bid, opt_bar_press_feedback_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(bid, opt_bar_press_feedback_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(bid, opt_bar_select_cb, LV_EVENT_RELEASED, (void *)(intptr_t)b);
    lv_obj_t *wid = lv_label_create(row);
    opt_badge_waste[b] = wid;
    lv_obj_set_pos(wid, 352, 1);  // +۵ راست؛ y در opt_apply وسط‌چین
    lv_obj_set_size(wid, 72, 48);
    lv_obj_set_style_radius(wid, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(wid, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(wid, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(wid, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(wid, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(wid, -3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(wid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(wid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(wid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(wid, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(wid, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(wid, "پرت");  // فقط فارسی — عدد جدا با Montserrat
    {
      float wval = OPT.bar_waste[b];
      char wb[24];
      float wr = roundf(wval * 10.0f) / 10.0f;
      bool has_dec = fabsf(wr - roundf(wr)) >= 0.05f;
      if (!has_dec) snprintf(wb, sizeof(wb), "%d", (int)lroundf(wr));
      else snprintf(wb, sizeof(wb), "%.1f", (double)wr);
      lv_obj_t *wn = lv_label_create(wid);
      opt_badge_waste_num[b] = wn;
      lv_obj_set_style_text_font(wn, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(wn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(wn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(wn, wb);
      lv_obj_align(wn, LV_ALIGN_BOTTOM_MID, 0, -4);  // عدد پرت ۳px بالا نسبت به −۱
    }
    lv_obj_t *done_lbl = lv_label_create(row);
    opt_piece_done_lbl[b] = done_lbl;
    lv_obj_set_pos(done_lbl, 86, 6);  // شروع اعداد ۴px به راست
    lv_obj_set_width(done_lbl, 260);
    lv_label_set_recolor(done_lbl, true);  // متن با مارک‌آپ رنگ رندر می‌شود
    lv_obj_set_style_text_font(done_lbl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(done_lbl, lv_color_hex(0x6f7780), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(done_lbl, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_decor(done_lbl, LV_TEXT_DECOR_STRIKETHROUGH, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(done_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(done_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(done_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(done_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(done_lbl, opt_tap_arm_from_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(done_lbl, opt_bar_finished_tap_cb, LV_EVENT_RELEASED, (void *)(intptr_t)b);
    lv_obj_t *cur_lbl = lv_label_create(row);
    opt_piece_current_lbl[b] = cur_lbl;
    lv_obj_set_size(cur_lbl, 124, 34);
    lv_obj_set_style_radius(cur_lbl, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cur_lbl, lv_color_hex(0xffd54f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cur_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cur_lbl, lv_color_hex(0x2b2410), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cur_lbl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(cur_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(cur_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(cur_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(cur_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_add_flag(cur_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *lbl = lv_label_create(row);
    opt_piece_lbl[b] = lbl;
    lv_obj_set_pos(lbl, 86, 2);  // شروع اعداد ۴px به راست
    lv_obj_set_width(lbl, 260);
    lv_label_set_recolor(lbl, true);  // متن با مارک‌آپ رنگ رندر می‌شود
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
  }
  opt_apply_piece_labels();
  opt_last_loaded_bar = -1;
  OPT.flag_refresh = false;
  for (int s = 0; s < OPT_RENDER_ROWS; s++) {  // ویجت‌ها تازه‌ساخته‌اند → کش رندر باطل
    opt_row_last_bar[s] = -1;
    opt_row_last_cur[s] = -1;
    opt_row_last_log[s] = -1;
  }
  opt_refresh_labels();
  opt_done_box_sync();  // کادر «برش خورده» از ابتدا محتوا/حالت درست — نه "Text" خام
  opt_bar_page_ui_sync();
  // پنل آمار هدر + چاپ همیشه روی لیست
  if (opt_lbl_bar_title) {
    lv_obj_t *st = lv_obj_get_parent(opt_lbl_bar_title);
    if (st) lv_obj_move_foreground(st);
  }
  if (print_btn) lv_obj_move_foreground(print_btn);
  ui_resume_checkpoint();
}

static bool opt_open_pending = false;  // تایمر ساخت پنل فعال است

static void opt_open_panel_timer_cb(lv_timer_t *t) {
  lv_timer_delete(t);
  opt_open_pending = false;
  opt_open_panel_build();
}

static void opt_open_panel() {
  // پنل داخل کال‌بک لمسی حذف/ساخت نشود (hang) → async + ساخت با تایمر یک‌باره
  if (opt_cont) { lv_obj_delete_async(opt_cont); opt_cont = NULL; }
  if (opt_open_pending) return;
  opt_open_pending = true;
  lv_timer_t *t = lv_timer_create(opt_open_panel_timer_cb, 1, NULL);
  (void)t;
}

static void btn_optimize_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  if (STP.flag_is_cutting || STP.motorIs_moving) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  cancel_screen3_pending_edit();
  numpad_close();
  opt_open_from_list();
}

//==================== پاک‌کردن لیست (صفرکردن همهٔ تعدادها) ====================
void opt_request_remote_open() {
  opt_remote_open_req = true;
}

void prcs_opt_remote_open() {
  if (!opt_remote_open_req) return;
  opt_remote_open_req = false;
  if (GUI.lock_screen && !STP.goref_cmd && !STP.motorIs_moving) {
    if (objects.panel0_1) lv_obj_add_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
    pending_reference_confirm = false;
    GUI.lock_screen = false;
  }
  if (GUI.scr_id != 3 || GUI.lock_screen || STP.flag_is_cutting || STP.motorIs_moving) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  cancel_screen3_pending_edit();
  numpad_close();
  opt_open_from_list();
}

static lv_obj_t *clr_mbox = NULL;
static lv_obj_t *list_purge_modal = NULL;
static lv_obj_t *list_purge_bar = NULL;
static int list_purge_idx = -1;
static uint32_t list_purge_start_ms = 0;
static const uint32_t LIST_PURGE_DUR_MS = 4500;

static void list_purge_close(bool do_delete) {
  if (list_purge_modal && lv_obj_is_valid(list_purge_modal)) {
    lv_obj_delete(list_purge_modal);
  }
  list_purge_modal = NULL;
  list_purge_bar = NULL;
  GUI.lock_screen = false;
  ui_help_suppress_release = true;
  if (do_delete && list_purge_idx >= 0 && list_purge_idx < LIST_MAX) {
    int idx = list_purge_idx;
    list_zero_row(idx);
    save_permanently_preferences_sp_val(idx, 0);
    save_permanently_preferences_count_val(idx, 0);
    save_permanently_preferences_count_base(idx, 0);
    if (AUT.selected_count_val == idx) {
      AUT.selected_count_val = -1;
      AUT.temp_list_count_Val = 0;
      opt_main_bar_lbl_hide();
    }
    GUI.flag_update_labels_3_1 = true;
    GUI.flag_update_labels_3_2 = true;
    GUI.flag_panel3_edit_labels_txt = true;
    GUI.flag_refresh_visible = true;
    GUI.buzzer_cmd_short = true;
  }
  list_purge_idx = -1;
}

static void list_purge_yes_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  list_purge_close(true);
}

static void list_purge_no_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  list_purge_close(false);
}

static void list_purge_card_eat_cb(lv_event_t *e) {
  lv_event_stop_bubbling(e);
}

void list_purge_confirm_show(int idx) {
  if (idx < 0 || idx >= LIST_MAX) return;
  if (GUI.lock_screen && list_purge_modal && lv_obj_is_valid(list_purge_modal)) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (list_purge_modal && lv_obj_is_valid(list_purge_modal)) list_purge_close(false);
  list_purge_idx = idx;
  GUI.buzzer_cmd_short = true;
  const bool fa = (GUI.system_language != 2);

  list_purge_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(list_purge_modal, 0, 0);
  lv_obj_set_size(list_purge_modal, 480, 320);
  lv_obj_set_style_bg_opa(list_purge_modal, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(list_purge_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(list_purge_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(list_purge_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(list_purge_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(list_purge_modal, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(list_purge_modal, list_purge_no_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(list_purge_modal);
  lv_obj_set_size(card, 400, 168);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, -8);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);  // کلیک روی کارت بیرون نبندد
  lv_obj_add_event_cb(card, list_purge_card_eat_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *close = lv_button_create(card);
  lv_obj_set_size(close, 36, 32);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, list_purge_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(close);
  lv_label_set_text(cl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cl);

  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 340);
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 10, 8);
  lv_obj_set_style_text_font(msg, fa ? &ui_font_naz20b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(msg, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(msg, fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(msg, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  if (fa) {
    char buf[96];
    snprintf(buf, sizeof(buf), "ردیف %d خط خورده است. پاک شود؟", idx + 1);
    lv_label_set_text(msg, buf);
  } else {
    char buf[96];
    snprintf(buf, sizeof(buf), "Row %d is finished. Delete it?", idx + 1);
    lv_label_set_text(msg, buf);
  }

  lv_obj_t *yes = lv_button_create(card);
  lv_obj_set_size(yes, 140, 40);
  lv_obj_align(yes, LV_ALIGN_BOTTOM_RIGHT, -4, -28);
  lv_obj_set_style_bg_color(yes, lv_color_hex(0xB90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(yes, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(yes, list_purge_yes_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *yl = lv_label_create(yes);
  lv_label_set_text(yl, fa ? "پاک کن" : "Delete");
  lv_obj_set_style_text_font(yl, fa ? &ui_font_naz18b : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(yl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(yl);

  list_purge_bar = lv_bar_create(card);
  lv_obj_set_size(list_purge_bar, 360, 8);
  lv_obj_align(list_purge_bar, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_bar_set_range(list_purge_bar, 0, 100);
  lv_bar_set_value(list_purge_bar, 100, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(list_purge_bar, lv_color_hex(0xE5E5EA), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(list_purge_bar, lv_color_hex(0x0A84FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);

  list_purge_start_ms = lv_tick_get();
  GUI.lock_screen = true;
  lv_obj_move_foreground(list_purge_modal);
}

// ---- تأیید کنار گذاشتن شاخه/ردیف ناتمام ----
static lv_obj_t *queue_sw_modal = NULL;
static lv_obj_t *queue_sw_bar = NULL;
static uint32_t queue_sw_start_ms = 0;
static int queue_sw_pending_idx = -1;
static const uint32_t QUEUE_SW_MS = 5500;

static void queue_sw_close(bool do_switch) {
  int idx = queue_sw_pending_idx;
  if (queue_sw_modal && lv_obj_is_valid(queue_sw_modal)) lv_obj_delete(queue_sw_modal);
  queue_sw_modal = NULL;
  queue_sw_bar = NULL;
  queue_sw_pending_idx = -1;
  GUI.lock_screen = false;
  ui_help_suppress_release = true;
  if (do_switch && idx >= 0) {
    GUI.buzzer_cmd_short = true;
    select_list_entry_apply(idx);
  }
}

static void queue_sw_yes_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  queue_sw_close(true);
}
static void queue_sw_no_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  GUI.buzzer_cmd_short = true;
  queue_sw_close(false);
}
static void queue_sw_card_eat_cb(lv_event_t *e) { lv_event_stop_bubbling(e); }

void queue_switch_confirm_show(int new_idx) {
  if (new_idx < 0 || new_idx >= LIST_MAX) return;
  if (queue_sw_modal && lv_obj_is_valid(queue_sw_modal)) queue_sw_close(false);
  queue_sw_pending_idx = new_idx;
  GUI.buzzer_cmd_short = true;
  const bool fa = (GUI.system_language != 2);
  const bool opt_busy = OPT.active && OPT.cur < OPT.piece_count;

  queue_sw_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(queue_sw_modal, 0, 0);
  lv_obj_set_size(queue_sw_modal, 480, 320);
  lv_obj_set_style_bg_opa(queue_sw_modal, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(queue_sw_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(queue_sw_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(queue_sw_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(queue_sw_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(queue_sw_modal, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(queue_sw_modal, queue_sw_no_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(queue_sw_modal);
  lv_obj_set_size(card, 420, 188);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, -8);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, queue_sw_card_eat_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *close = lv_button_create(card);
  lv_obj_set_size(close, 36, 32);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xFF3B30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, queue_sw_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(close);
  lv_label_set_text(cl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cl);

  lv_obj_t *msg = lv_label_create(card);
  lv_obj_set_width(msg, 360);
  lv_obj_align(msg, LV_ALIGN_TOP_MID, 10, 8);
  lv_obj_set_style_text_font(msg, fa ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(msg, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(msg, fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(msg, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  // بدون رقم لاتین داخل naz
  if (fa) {
    lv_label_set_text(msg, opt_busy
      ? "الان روی یک شاخه ناتمام هستید. میخواهید کنارش بگذارید و این ردیف را جایگزین کنید؟"
      : "الان روی یک ردیف ناتمام هستید. میخواهید کنارش بگذارید و این ردیف را جایگزین کنید؟");
  } else {
    char buf[160];
    if (opt_busy)
      snprintf(buf, sizeof(buf), "Bar work is unfinished. Set it aside and load row %d?", new_idx + 1);
    else
      snprintf(buf, sizeof(buf), "Row work is unfinished. Set it aside and load row %d?", new_idx + 1);
    lv_label_set_text(msg, buf);
  }

  lv_obj_t *yes = lv_button_create(card);
  lv_obj_set_size(yes, 150, 44);
  lv_obj_align(yes, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
  lv_obj_set_style_bg_color(yes, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(yes, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(yes, queue_sw_yes_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *yl = lv_label_create(yes);
  lv_obj_set_style_text_font(yl, fa ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(yl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(yl, fa ? "جایگزین کن" : "Replace");
  lv_obj_center(yl);

  lv_obj_t *no = lv_button_create(card);
  lv_obj_set_size(no, 150, 44);
  lv_obj_align(no, LV_ALIGN_BOTTOM_LEFT, 8, -8);
  lv_obj_set_style_bg_color(no, lv_color_hex(0x8E8E93), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(no, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(no, queue_sw_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *nl = lv_label_create(no);
  lv_obj_set_style_text_font(nl, fa ? &ui_font_naz18b : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(nl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(nl, fa ? "بمان" : "Stay");
  lv_obj_center(nl);

  queue_sw_bar = lv_bar_create(card);
  lv_obj_set_size(queue_sw_bar, 380, 8);
  lv_obj_align(queue_sw_bar, LV_ALIGN_BOTTOM_MID, 0, -58);
  lv_bar_set_range(queue_sw_bar, 0, 100);
  lv_bar_set_value(queue_sw_bar, 100, LV_ANIM_OFF);
  queue_sw_start_ms = lv_tick_get();
  GUI.lock_screen = true;
  lv_obj_move_foreground(queue_sw_modal);
}

void prcs_list_purge_confirm() {
  if (queue_sw_modal && lv_obj_is_valid(queue_sw_modal)) {
    uint32_t elapsed = lv_tick_elaps(queue_sw_start_ms);
    if (elapsed >= QUEUE_SW_MS) {
      queue_sw_close(false);
    } else if (queue_sw_bar) {
      int remain = (int)(100UL - (elapsed * 100UL / QUEUE_SW_MS));
      if (remain < 0) remain = 0;
      lv_bar_set_value(queue_sw_bar, remain, LV_ANIM_OFF);
    }
  }
  if (!list_purge_modal || !lv_obj_is_valid(list_purge_modal)) return;
  uint32_t elapsed = lv_tick_elaps(list_purge_start_ms);
  if (elapsed >= LIST_PURGE_DUR_MS) {
    list_purge_close(false);
    return;
  }
  if (list_purge_bar) {
    int remain = (int)(100UL - (elapsed * 100UL / LIST_PURGE_DUR_MS));
    if (remain < 0) remain = 0;
    lv_bar_set_value(list_purge_bar, remain, LV_ANIM_OFF);
  }
}

static void clr_close_mbox() {
  if (clr_mbox && lv_obj_is_valid(clr_mbox)) {
    lv_obj_delete(clr_mbox);  // sync — async گاهی lock/UI را به‌هم می‌ریخت
  }
  clr_mbox = NULL;
  GUI.lock_screen = false;
}
static void clr_yes_cb(lv_event_t *e) {
  GUI.buzzer_cmd_short = true;
  clr_close_mbox();
  GUI.flag_clear_list_counts = true;
}
static void clr_no_cb(lv_event_t *e) {
  GUI.buzzer_cmd_short = true;
  clr_close_mbox();
}
static void btn_clear_list_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen || STP.flag_is_cutting || OPT.active) { GUI.buzzer_cmd_long = true; return; }
  if (clr_mbox) return;  // دیالوگ باز است
  GUI.buzzer_cmd_short = true;
  cancel_screen3_pending_edit();
  numpad_close();
  // ساخت msgbox را به تیک بعد موکول نکن — ولی قفل را فقط وقتی دیالوگ زنده است نگه دار
  clr_mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_title(clr_mbox, "Clear List");
  lv_msgbox_add_text(clr_mbox, "Set ALL counts to zero?\n(lengths are kept)");
  lv_obj_t *by = lv_msgbox_add_footer_button(clr_mbox, "YES - Clear");
  lv_obj_t *bn = lv_msgbox_add_footer_button(clr_mbox, "NO");
  lv_obj_set_style_bg_color(by, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(bn, lv_color_hex(0x008306), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(by, clr_yes_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(bn, clr_no_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_center(clr_mbox);
  GUI.lock_screen = true;
}

//==================== ساخت ویجت‌های runtime صفحهٔ لیست ====================
static lv_obj_t *mk_hdr_btn(lv_obj_t *parent, const char *sym, int x, lv_event_cb_t cb, void *ud) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_pos(btn, x, -13);
  lv_obj_set_size(btn, 44, 50);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0xff1976d2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_RELEASED, ud);
  lv_obj_t *l = lv_label_create(btn);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(l);
  lv_label_set_text(l, sym);
  return btn;
}

//==================== اسپلش بوت + سلف‌تست ====================
// دو عکس ۴۸۰×۳۲۰ در SPIFFS (/splash0.rgb565 ، /splash1.rgb565) — هر بوت یکی به‌نوبت ~۳ثانیه
// گوشهٔ چپ‌بالا: Haffler + خط + ورژن: x.y.z  |  خطای سخت‌افزار همان پایین می‌ماند
static lv_obj_t *splash_cont = NULL;
static lv_obj_t *splash_img_obj = NULL;
static uint8_t splash_phase = 0;           // فاز اسپلش: ۱ = عکس اول، ۲ = عکس دوم
static bool splash_hold_for_goto = false;  // پوشش سیاه تا لود صفحهٔ resume
static bool splash_hold_is_resume = false; // resume هنگ — boot_main_ready نزن
static int splash_resume_target = -1;      // مقصد بازگشت، بعد از نمایش اسپلش
static bool splash_resume_applied = false; // resume اجرا شد؛ مسیر boot→main دیگر راه نیفتد
static uint32_t splash_hide_at = 0;
static uint8_t *splash_rgb_buf = NULL;
static lv_image_dsc_t splash_img_dsc;

static void splash_free_img_buf() {
  if (splash_rgb_buf) {
    heap_caps_free(splash_rgb_buf);
    splash_rgb_buf = NULL;
  }
  memset(&splash_img_dsc, 0, sizeof(splash_img_dsc));
}

static int splash_next_index() {
  int idx = 0;
  if (pref.begin("ui", false)) {
    idx = pref.getInt("splash", 0) & 1;
    pref.putInt("splash", idx ^ 1);
    pref.end();
  }
  return idx;
}

static bool splash_try_load_rgb565(int idx) {
  splash_free_img_buf();
  // بدون PSRAM عکس اسپلش ممنوع — alloc روی DRAM داخلی (~300KB) بعد از LVGL = هنگ/کرش
  if (!psramFound()) return false;
  if (!SPIFFS.begin(false)) return false;
  char path[24];
  snprintf(path, sizeof(path), "/splash%d.rgb565", idx & 1);
  File f = SPIFFS.open(path, "r");
  if (!f) {
    snprintf(path, sizeof(path), "/splash%d.rgb565", (idx ^ 1) & 1);
    f = SPIFFS.open(path, "r");
  }
  if (!f) return false;
  const size_t need = (size_t)480 * 320 * 2;
  if ((size_t)f.size() < need) { f.close(); return false; }
  // فقط SPIRAM — هرگز fallback به heap داخلی
  splash_rgb_buf = (uint8_t *)heap_caps_malloc(need, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!splash_rgb_buf) { f.close(); return false; }
  size_t n = f.read(splash_rgb_buf, need);
  f.close();
  if (n < need) { splash_free_img_buf(); return false; }
  splash_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  splash_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
  splash_img_dsc.header.flags = 0;
  splash_img_dsc.header.w = 480;
  splash_img_dsc.header.h = 320;
  splash_img_dsc.header.stride = 480 * 2;
  splash_img_dsc.data_size = need;
  splash_img_dsc.data = splash_rgb_buf;
  return true;
}

void splash_close_manual() {
  if (splash_cont) {
    lv_obj_delete(splash_cont);
    splash_cont = NULL;
    splash_img_obj = NULL;
    splash_free_img_buf();
    device_lock_ui_hold_for_splash(false);
  }
}

void splash_manual_view(int idx) {
  splash_close_manual();
  splash_cont = lv_obj_create(lv_layer_top());
  lv_obj_set_size(splash_cont, 480, 320);
  lv_obj_set_pos(splash_cont, 0, 0);
  lv_obj_set_style_bg_color(splash_cont, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(splash_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(splash_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(splash_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(splash_cont, LV_OBJ_FLAG_SCROLLABLE);

  splash_phase = 99; // حالت نمایش دستی
  splash_hide_at = lv_tick_get() + 60000UL;
  const bool have_photo = splash_try_load_rgb565(idx);
  if (have_photo) {
    splash_img_obj = lv_image_create(splash_cont);
    lv_image_set_src(splash_img_obj, &splash_img_dsc);
    lv_obj_set_pos(splash_img_obj, 0, 0);
    lv_obj_clear_flag(splash_img_obj, LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_invalidate(lv_layer_top());
}

void splash_show() {
  // صفحه فوراً بازیابی شود؛ نمایش AUTO تا پایان شناسایی اولیه USB عقب بماند.
  GUI.soft_kb_auto_ready_ms = millis() + 4500UL;
  device_lock_ui_hold_for_splash(true);
  // روزشمار: اگر فعال است، تطبیق انجام می‌شود و اسپلش با روزمانده نمایش داده می‌شود
  if (day_lock_boot_reconcile()) save_admin_svc_state();
  // هرگز در بوت اسپلش دور زده نمی‌شود؛ هر ۲ عکس همیشه کامل دیده می‌شوند.
  int resume_scr = -1;
  const bool resume_ok = ui_resume_boot_get_latched(&resume_scr);
  splash_resume_target = -1;
  splash_resume_applied = false;
  if (resume_ok) {
    splash_resume_target = resume_scr;
    STP.referenced = false;
    STP.goref_cmd = false;
    STP.moveStepperCMD = false;
    STP.jogEnabled = false;
    STP.mvm_interupt = false;
    GUI.flag_start_key = false;
    ref_failed_flag = false;
    ref_wrong_dir_flag = false;
    ref_fail_reason = 0;
    ref_error_ui_active = false;
  }

  splash_cont = lv_obj_create(lv_layer_top());
  lv_obj_set_size(splash_cont, 480, 320);
  lv_obj_set_pos(splash_cont, 0, 0);
  lv_obj_set_style_bg_color(splash_cont, lv_color_hex(0x0d2b4d),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(splash_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(splash_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(splash_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(splash_cont, LV_OBJ_FLAG_SCROLLABLE);

  splash_phase = 1;
  const bool have_photo = splash_try_load_rgb565(0);
  if (have_photo) {
    splash_img_obj = lv_image_create(splash_cont);
    lv_image_set_src(splash_img_obj, &splash_img_dsc);
    lv_obj_set_pos(splash_img_obj, 0, 0);
    lv_obj_clear_flag(splash_img_obj, LV_OBJ_FLAG_CLICKABLE);
  } else {
    splash_img_obj = NULL;
  }

  // چپ‌بالا بدون کادر: Haffler ضخیم + خط + ورژن Bold
  lv_obj_t *t_name_sh = lv_label_create(splash_cont);
  lv_obj_set_style_text_font(t_name_sh, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(t_name_sh, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(t_name_sh, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(t_name_sh, 14, 10);
  lv_label_set_text(t_name_sh, "Haffler");

  lv_obj_t *t_name = lv_label_create(splash_cont);
  lv_obj_set_style_text_font(t_name, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(t_name, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(t_name, 12, 8);
  lv_label_set_text(t_name, "Haffler");

  lv_obj_t *line = lv_obj_create(splash_cont);
  lv_obj_set_pos(line, 12, 48);
  lv_obj_set_size(line, 130, 2);
  lv_obj_set_style_bg_color(line, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(line, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(line, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

  // زیر HAFFLER: ورژن + روزمانده قفل (اگر فعال)
  // روزشمار: فقط در بوت تطبیق؛ ذخیره فقط وقتی روز کم شود
  lv_obj_t *t_ver = lv_label_create(splash_cont);
  lv_obj_set_style_text_font(t_ver, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(t_ver, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(t_ver, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(t_ver, 16, 54);
  {
    char ver_buf[32];
    snprintf(ver_buf, sizeof(ver_buf), "V %s", PROJECT_VERSION_STRING);
    lv_label_set_text(t_ver, ver_buf);
  }
  if (day_lock_remain > 0) {
    // روزشمار روی خود عکس اسپلش؛ هیچ کادر آبی یا سایه‌ای عکس را نپوشاند.
    lv_obj_t *pill = lv_obj_create(splash_cont);
    lv_obj_set_size(pill, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_pad_hor(pill, 10, 0);
    lv_obj_set_style_pad_ver(pill, 2, 0);
    lv_obj_set_style_radius(pill, 14, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pill, 0, 0);
    lv_obj_set_style_shadow_width(pill, 0, 0);
    lv_obj_clear_flag(pill, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pill, 4, 0);

    lv_obj_t *num = lv_label_create(pill);
    lv_obj_set_style_text_font(num, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(num, lv_color_hex(0x0A84FF), 0);
    lv_obj_set_style_translate_y(num, 0, 0);
    {
      char nb[8];
      snprintf(nb, sizeof(nb), "%d", day_lock_remain);
      lv_label_set_text(num, nb);
    }
    lv_obj_t *unit = lv_label_create(pill);
    lv_obj_set_style_text_font(unit, (GUI.system_language == 2) ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(unit, lv_color_hex(0x0A84FF), 0);
    lv_obj_set_style_base_dir(unit, (GUI.system_language == 2) ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
    // هر دو زبان ۳px پایین‌تر؛ واژهٔ انگلیسی days سه پیکسل دیگر پایین‌تر.
    lv_obj_set_style_translate_y(unit, (GUI.system_language == 2) ? 1 : -2, 0);
    lv_label_set_text(unit, (GUI.system_language == 2) ? "days" : "روز");

    lv_obj_align_to(pill, t_ver, LV_ALIGN_OUT_RIGHT_MID, 12, 0);
  }

  if (!have_photo) {
    lv_obj_t *t2 = lv_label_create(splash_cont);
    lv_obj_set_style_text_font(t2, &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(t2, lv_color_hex(0x90caf9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(t2, LV_ALIGN_TOP_MID, 0, 120);
    lv_label_set_text(t2, "دستگاه برش و متر دیجیتال");
  }

  // علت ری‌استارت قبلی — تشخیص brownout (تغذیهٔ ضعیف) / کرش / واچ‌داگ در محل مشتری
  esp_reset_reason_t rr = esp_reset_reason();
  bool abnormal_reset = (rr == ESP_RST_BROWNOUT || rr == ESP_RST_PANIC ||
                         rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT);
  const char *rr_txt = (rr == ESP_RST_POWERON) ? "PowerOn" : (rr == ESP_RST_SW) ? "SW-Restart"
                       : (rr == ESP_RST_BROWNOUT) ? "BROWNOUT!" : (rr == ESP_RST_PANIC) ? "CRASH!"
                       : (rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT) ? "WATCHDOG!"
                       : (rr == ESP_RST_DEEPSLEEP) ? "DeepSleep" : "Other";
  const uint32_t psram_bytes = ESP.getPsramSize();
  // برد واقعی ۲MiB PSRAM دارد؛ «پیدا شد» به‌تنهایی برای پذیرش کافی نیست.
  bool psram_ok = psramFound() && psram_bytes >= (2UL * 1024UL * 1024UL);
  uint32_t heap_kb = ESP.getFreeHeap() / 1024;
  bool heap_ok = heap_kb >= 8;
  bool nvs_ok = pref.begin("params", true);
  if (nvs_ok) pref.end();
  bool all_ok = touch_ready && psram_ok && heap_ok && nvs_ok;

  if (!all_ok || abnormal_reset) {
    lv_obj_t *t4 = lv_label_create(splash_cont);
    lv_obj_set_style_text_font(t4, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(t4, lv_color_hex(all_ok ? 0xffcc80 : 0xff8a80), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(t4, LV_ALIGN_BOTTOM_MID, 0, -48);
    char chk[160];
    snprintf(chk, sizeof(chk), "Touch:%s  PSRAM:%luKB/%s  NVS:%s  Heap:%luKB  Reset:%s",
             touch_ready ? "OK" : "FAIL", (unsigned long)(psram_bytes / 1024UL),
             psram_ok ? "OK" : "FAIL", nvs_ok ? "OK" : "FAIL",
             (unsigned long)heap_kb, rr_txt);
    lv_label_set_text(t4, chk);
  }
  if (abnormal_reset) {
    lv_obj_t *t6 = lv_label_create(splash_cont);
    lv_obj_set_style_text_font(t6, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(t6, lv_color_hex(0xffcc80), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(t6, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_label_set_text(t6, (rr == ESP_RST_BROWNOUT) ? "خاموشی قبلی از افت ولتاژ تغذیه بود - منبع تغذیه را چک کنید"
                                                   : "ری استارت غیرعادی - در صورت تکرار به پشتیبانی اطلاع دهید");
    GUI.buzzer_cmd_long = true;
  }
  if (!all_ok) {
    lv_obj_t *t5 = lv_label_create(splash_cont);
    lv_obj_set_style_text_font(t5, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(t5, lv_color_hex(0xff8a80), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(t5, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_label_set_text(t5, "اشکال سخت افزاری - با پشتیبانی تماس بگیرید");
    GUI.buzzer_cmd_long = true;
  }
  // عکس اسپلش اول ۲ ثانیه، سپس عکس دوم ۲ ثانیه
  splash_hide_at = lv_tick_get() + 2000;
}

extern float cpu_temp_alarm_c;
//==================== مانیتور دمای داخلی تراشه ====================
// دمای بالا = خرابی فن/تهویهٔ تابلو — هشدار با هیسترزیس (75°C هشدار، زیر 65°C ری‌آرم)
static unsigned long temp_last_check_ms = 0;
static bool temp_warned = false;
void prcs_temp_monitor() {
  if (millis() - temp_last_check_ms < 10000) return;  // هر 10 ثانیه
  temp_last_check_ms = millis();
  float t = temperatureRead();
  float alarm = cpu_temp_alarm_c;
  if (alarm < 45.0f) alarm = 45.0f;
  if (alarm > 105.0f) alarm = 105.0f;
  if (!temp_warned && t >= alarm) {
    temp_warned = true;
    char fa[128], en[80];
    snprintf(fa, sizeof(fa), "دمای پردازنده بالا: %d درجه\nتهویه را بررسی کنید", (int)t);
    snprintf(en, sizeof(en), "CPU temp high: %dC\nCheck cooling", (int)t);
    ui_toast(fa, en, 6000);
    Serial.printf("TEMP WARNING: %.1fC (alarm %.0f)\n", t, alarm);
  } else if (temp_warned && t < (alarm - 10.0f)) {
    temp_warned = false;  // خنک شد — هشدار دوباره مسلح می‌شود
  }
}

void prcs_clear_list_counts() {
  // فاز 1: پاک RAM + فلگ UI (سبک) — قفل را فوری آزاد کن تا تاچ زنده بماند
  if (GUI.flag_clear_list_counts) {
    GUI.flag_clear_list_counts = false;
    GUI.lock_screen = false;
    opt_notify_list_edited();
    int visible_limit = GUI.visible_raws;
    if (visible_limit < 0) visible_limit = 0;
    if (visible_limit > LIST_MAX) visible_limit = LIST_MAX;
    for (int i = visible_limit; i < LIST_MAX; i++) {
      AUT.list_count_Val[i] = 0;
      GUI.inputBuffer_L_3_2_x[i] = "0";
    }
    AUT.selected_count_val = -1;
    AUT.temp_list_count_Val = 0;
    GUI.clear_visual_idx = visible_limit - 1;
    list_clear_current_zeroed = false;
    list_clear_next_ms = 0;
    if (GUI.clear_visual_idx >= 0) {
      list_show_row(GUI.clear_visual_idx);
      GUI.update_lable1_3 = true;
      GUI.flag_update_labels_3_2 = true;
      GUI.flag_panel3_edit_labels_txt = true;
      GUI.flag_update_panel3_2_x_clr = true;
      return;
    }
    GUI.clear_nvs_idx = 0;
    return;
  }
  // فاز 2: هر تیک حداکثر ۳ کلید NVS (سبک‌تر از قبل)
  if (GUI.clear_visual_idx >= 0) {
    uint32_t now = millis();
    if (list_clear_next_ms != 0 && (int32_t)(now - list_clear_next_ms) < 0) return;
    int i = GUI.clear_visual_idx;
    if (!list_clear_current_zeroed) {
      AUT.list_count_Val[i] = 0;
      GUI.inputBuffer_L_3_2_x[i] = "0";
      list_clear_current_zeroed = true;
    } else {
      GUI.clear_visual_idx--;
      list_clear_current_zeroed = false;
      i = GUI.clear_visual_idx;
    }
    if (i >= 0) list_show_row(i);
    GUI.update_lable1_3 = true;
    GUI.flag_update_labels_3_2 = true;
    GUI.flag_panel3_edit_labels_txt = true;
    GUI.flag_update_panel3_2_x_clr = true;
    list_clear_next_ms = now + 55;
    if (GUI.clear_visual_idx < 0) {
      GUI.clear_nvs_idx = 0;
      GUI.flag_panel3_edit_labels_txt = true;
    }
    return;
  }
  if (GUI.clear_nvs_idx < 0) return;
  pref.begin("params", false);
  int end = GUI.clear_nvs_idx + 3;
  if (end > LIST_MAX) end = LIST_MAX;
  for (int i = GUI.clear_nvs_idx; i < end; i++) {
    char key[20];
    snprintf(key, sizeof(key), "count_val_%d", i);
    pref.putInt(key, 0);
  }
  pref.end();
  GUI.clear_nvs_idx = end;
  if (GUI.clear_nvs_idx >= LIST_MAX) {
    GUI.clear_nvs_idx = -1;
    GUI.flag_update_labels_3_2 = true;
    ui_toast("تعداد همه ردیف ها صفر شد", "All counts cleared", 2500);
  }
}

// سوئیچ کیبرد مجازی + عرض تیغه + دکمهٔ وب‌اپ در تنظیمات عمومی (F1)
extern "C" void action_printertest(lv_event_t *e);
extern "C" void action_stopstate(lv_event_t *e);
static lv_obj_t *sw_vkb = NULL;
static lv_obj_t *lbl_set_kerf = NULL;
static lv_obj_t *lbl_set_stock = NULL;
static lv_obj_t *lbl_unit_kerf = NULL;
static lv_obj_t *lbl_unit_stock = NULL;
static lv_obj_t *lbl_kerf_title = NULL;
static lv_obj_t *lbl_stock_title = NULL;
static lv_obj_t *f1_center_card = NULL;
static lv_obj_t *f1_factory_reset_btn = NULL;
static lv_obj_t *f1_factory_reset_lbl = NULL;
static lv_obj_t *f1_factory_reserved_btn = NULL;
static lv_obj_t *f1_factory_reserved_lbl = NULL;
static lv_obj_t *f1_factory_res_btn = NULL;
static lv_obj_t *f1_factory_res_lbl = NULL;
static lv_obj_t *f1_web_btn = NULL;
static lv_obj_t *f1_kb_chip = NULL;
static lv_obj_t *f1_ncno_desc_bg = NULL;  // کادر قرمز ثابت؛ متن جدا بالا می‌رود
static lv_obj_t *f1_ncno_desc_lbl = NULL;
bool buzzer_request_pattern(uint8_t pattern);


// ---- Smart soft-KB vs USB keyboard ----
// vkb_mode: 0=AUTO 1=FORCE_ON 2=FORCE_OFF
static bool settings_soft_kb_tab_ok() {
  if (GUI.scr_id != 2) return false;
  if (!objects.tabview2_1) return true;
  int tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
  return (tab == 0);  // فقط نامپد بزرگ F1 — F2 کیبورد خط خودش را دارد
}

static void vkb_sync_chip_visual() {
  if (sw_vkb) {
    if (GUI.virtual_kb_enable) lv_obj_add_state(sw_vkb, LV_STATE_CHECKED);
    else lv_obj_clear_state(sw_vkb, LV_STATE_CHECKED);
  }
  if (f1_kb_chip) {
    lv_obj_set_style_bg_color(f1_kb_chip,
      lv_color_hex(GUI.virtual_kb_enable ? 0x0a84ff : 0xc5ccd3),
      LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static bool vkb_compute_effective() {
  if (GUI.vkb_mode == 1) return true;
  if (GUI.vkb_mode == 2) return false;
  // AUTO
  return !usb_keyboard_present();
}

static bool vkb_auto_open_guard_active() {
  return GUI.vkb_mode == 0 && GUI.soft_kb_auto_ready_ms != 0 &&
         (int32_t)(millis() - GUI.soft_kb_auto_ready_ms) < 0;
}

void vkb_apply_effective(bool allow_open) {
  bool on = vkb_compute_effective();
  GUI.virtual_kb_enable = on;
  PEND.virtual_kb_enable = on;
  vkb_sync_chip_visual();
  if (vkb_auto_open_guard_active()) {
    GUI.pending_f1_numpad_open = false;
    numpad_hide_soft_widgets();
    f2_hide_line_kb();
    return;
  }
  if (!on) {
    GUI.pending_f1_numpad_open = false;
    // USB/خاموش: نامپد لمسی را بردار؛ فوکوس F1 (ctx 3/4) برای تایپ USB بماند
    if (numpad_ctx == 1 || numpad_ctx == 2) numpad_close();
    else numpad_hide_soft_widgets();
    f2_hide_line_kb();
  } else if (allow_open && settings_soft_kb_tab_ok()) {
    GUI.pending_f1_numpad_open = true;
    f2_hide_line_kb();
  } else if (allow_open) {
    // تب F2: کیبورد خط خودش
    f2_ensure_line_kb();
  } else {
    f2_ensure_line_kb();
  }
}

void vkb_auto_open_guard_tick() {
  if (GUI.soft_kb_auto_ready_ms == 0) return;
  if ((int32_t)(millis() - GUI.soft_kb_auto_ready_ms) < 0) return;
  GUI.soft_kb_auto_ready_ms = 0;
  vkb_apply_effective(true);
}

void vkb_apply_user_toggle(bool want_on) {
  bool auto_on = !usb_keyboard_present();
  if (want_on == auto_on) GUI.vkb_mode = 0;      // back to AUTO
  else GUI.vkb_mode = want_on ? 1 : 2;            // FORCE_ON / FORCE_OFF
  vkb_apply_effective(true);
  settings_mark_dirty();
}

void vkb_poll_usb() {
  // فقط هماهنگی soft-KB با حضور USB — بدون LED / mouse / reflow / Task4
  // خط وحشتناک قرمز: اینجا را برای «درست کردن USB» عوض نکن؛ Task4/EspUsbHost ممنوع.
  static bool inited = false;
  static bool prev = false;
  bool now = usb_keyboard_present();
  if (!inited) {
    inited = true;
    prev = now;
    vkb_apply_effective(false);
    if (!now && list_edit_active()) list_soft_kb_ensure_open();
    return;
  }
  if (now == prev) {
    // بدون لبه: FORCE_ON با USB وصل نامپد لیست/مقصد را زنده نگه می‌دارد
    if (GUI.vkb_mode != 0) vkb_sync_chip_visual();
    return;
  }
  const bool plugged = now && !prev;
  const bool unplugged = !now && prev;
  prev = now;

  if (plugged) {
    // اتصال USB انتخاب دستی touch-KB را عوض نمی‌کند. AUTO بسته می‌شود،
    // FORCE_ON باز می‌ماند و FORCE_OFF نیز محترم است.
    vkb_apply_effective(GUI.vkb_mode == 1);
    if (GUI.vkb_mode != 1 && (numpad_ctx == 1 || numpad_ctx == 2)) numpad_close();
    usb_keyboard_set_numlock_led_force(!GUI.NumLock);
    // وضعیت سوایپ لیست را پاک کن تا بعد از وصل دوباره اسکرول کار کند
    list_press_valid = false;
    list_press_moved = false;
    list_bounce_reset();
    return;
  }
  if (unplugged) {
    // USB رفت: AUTO خودش روشن می‌شود؛ انتخاب صریح FORCE_ON/OFF تغییر نمی‌کند.
    vkb_apply_effective(GUI.scr_id == 2);
    list_press_valid = false;
    list_press_moved = false;
    list_bounce_reset();
    if (GUI.scr_id == 1 && GUI.virtual_kb_enable) numpad_open(1);
    else if (list_edit_active()) list_soft_kb_ensure_open();
  }
}

static void sw_vkb_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  bool on = lv_obj_has_state(sw_vkb, LV_STATE_CHECKED);
  vkb_apply_user_toggle(on);
}

static void settings_sync_stock_kerf_labels() {
  // طراح: عدد درشت آبی + واحد mm خاکستری جدا
  if (lbl_set_kerf) {
    if (numpad_ctx == 4) {
      if (settings_num_buf.length())
        lv_label_set_text(lbl_set_kerf, settings_num_buf.c_str());
      else
        lv_label_set_text(lbl_set_kerf, "");
    } else {
      lv_label_set_text_fmt(lbl_set_kerf, "%.1f", (double)kerf_mm);
    }
  }
  if (lbl_set_stock) {
    if (numpad_ctx == 3) {
      if (settings_num_buf.length())
        lv_label_set_text(lbl_set_stock, settings_num_buf.c_str());
      else
        lv_label_set_text(lbl_set_stock, "");
    } else {
      lv_label_set_text_fmt(lbl_set_stock, "%d", stock_len_mm);
    }
  }
  // هنگام ویرایش، mm را مخفی کن تا فقط عدد دیده شود (مثل صفحهٔ اصلی)
  if (lbl_unit_kerf) {
    if (numpad_ctx == 4) lv_obj_add_flag(lbl_unit_kerf, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(lbl_unit_kerf, LV_OBJ_FLAG_HIDDEN);
  }
  if (lbl_unit_stock) {
    if (numpad_ctx == 3) lv_obj_add_flag(lbl_unit_stock, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(lbl_unit_stock, LV_OBJ_FLAG_HIDDEN);
  }
  if (box_kerf) {
    const bool oobe_req = !g_oobe_done && (g_oobe_stage == OOBE_STAGE_F1_ESSENTIALS);
    const uint32_t clr = (numpad_ctx == 4) ? 0x0a84ff : (oobe_req ? 0xff3b30 : 0xd0e4f2);
    lv_obj_set_style_border_color(box_kerf, lv_color_hex(clr), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_kerf, (numpad_ctx == 4 || oobe_req) ? 2 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (box_stock) {
    const bool oobe_req = !g_oobe_done && (g_oobe_stage == OOBE_STAGE_F1_ESSENTIALS);
    const uint32_t clr = (numpad_ctx == 3) ? 0x0a84ff : (oobe_req ? 0xff3b30 : 0xd0e4f2);
    lv_obj_set_style_border_color(box_stock, lv_color_hex(clr), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_stock, (numpad_ctx == 3 || oobe_req) ? 2 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void settings_kerf_adj_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  int code = (int)(intptr_t)lv_event_get_user_data(e);
  GUI.buzzer_cmd_short = true;
  int old_stock = stock_len_mm;
  float old_kerf = kerf_mm;
  if (code == 1) kerf_mm = constrain(kerf_mm + 0.1f, 0.0f, 50.0f);
  else if (code == 2) kerf_mm = constrain(kerf_mm - 0.1f, 0.0f, 50.0f);
  else if (code == 3) stock_len_mm = constrain(stock_len_mm + 10, 10, 100000);
  else if (code == 4) stock_len_mm = constrain(stock_len_mm - 10, 10, 100000);
  kerf_mm = roundf(kerf_mm * 10.0f) / 10.0f;
  if (stock_len_mm != old_stock || fabsf(kerf_mm - old_kerf) > 0.001f) opt_invalidate_cache();
  settings_mark_dirty();
  settings_sync_stock_kerf_labels();
}

static void settings_open_num_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_CLICKED && code != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  // جلوگیری از دوبار فوکوس روی CLICKED+RELEASED یک لمس
  static uint32_t guard_ms = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - guard_ms) < 120) return;
  guard_ms = now;
  int which = (int)(intptr_t)lv_event_get_user_data(e);
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  int target_ctx = (which == 3) ? 3 : 4;
  if (numpad_ctx == target_ctx && !settings_clear_on_type) {
    f1_caret_lit = true;
    f1_caret_blink_ms = millis();
    GUI.flag_sync_stock_kerf = true;
    return;
  }
  // لمس کادر = فوکوس؛ نامپد فقط اگر مجازی روشن باشد
  f1_edit_focus(target_ctx, true);
}

static void settings_web_btn_cb(lv_event_t *e);
void f1_sync_wifi_chip() {
  if (!f1_web_btn || !lv_obj_is_valid(f1_web_btn)) return;
  const bool on = web_portal_running;
  lv_obj_set_style_bg_color(f1_web_btn, lv_color_hex(on ? 0xdcfce7 : 0xf1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(f1_web_btn, lv_color_hex(on ? 0x16a34a : 0xc5ccd3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(f1_web_btn, lv_color_hex(on ? 0x16a34a : 0x94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_t *wi = lv_obj_get_child(f1_web_btn, 0);
  if (wi) lv_obj_set_style_text_color(wi, lv_color_hex(on ? 0x16a34a : 0x94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void f1_wifi_hold_cb(lv_event_t *e);
static void f1_wifi_qr_open_cb(lv_event_t *e);
bool web_portal_request_direct(void);
void web_portal_request_sta_setup(void);
bool web_portal_is_sta(void);
void web_portal_sta_ip(char *out, size_t out_sz);
static lv_obj_t *f1_wifi_mode_box = NULL;
static lv_obj_t *f1_wifi_info_box = NULL;
static bool f1_wifi_hold_handled = false;

static lv_obj_t *f1_wifi_qr_box; // تعریف اصلی همراه مودال QR است؛ اینجا برای گاردِ بستن لازم است
static void f1_wifi_info_close_cb(lv_event_t *e) {
  (void)e;
  // اگر مودال QR باز مانده باشد، همراه پنل بسته شود تا یتیم نماند
  if (f1_wifi_qr_box && lv_obj_is_valid(f1_wifi_qr_box)) lv_obj_delete(f1_wifi_qr_box);
  f1_wifi_qr_box = NULL;
  if (f1_wifi_info_box && lv_obj_is_valid(f1_wifi_info_box)) {
    lv_obj_delete(f1_wifi_info_box);
  }
  f1_wifi_info_box = NULL;
  GUI.lock_screen = false;
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
}

static void f1_wifi_info_toggle_off_cb(lv_event_t *e) {
  (void)e;
  if (f1_wifi_info_box && lv_obj_is_valid(f1_wifi_info_box)) {
    lv_obj_delete(f1_wifi_info_box);
  }
  f1_wifi_info_box = NULL;
  GUI.lock_screen = false;
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  GUI.flag_web_portal_toggle = true;
}

static void f1_wifi_add_info_row(lv_obj_t *parent, int y, const char *lbl_fa, const char *lbl_en, const char *val, uint32_t val_color, bool val_ltr, bool en) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_pos(row, 8, y);
  lv_obj_set_size(row, 400, 30);
  lv_obj_set_style_bg_color(row, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(row, 6, 0);
  lv_obj_set_style_border_width(row, 1, 0);
  lv_obj_set_style_border_color(row, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *k = lv_label_create(row);
  lv_obj_set_style_text_font(k, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(k, lv_color_hex(0x475569), 0);
  lv_obj_set_style_base_dir(k, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(k, en ? lbl_en : lbl_fa);
  if (en) lv_obj_align(k, LV_ALIGN_LEFT_MID, 10, 0);
  else lv_obj_align(k, LV_ALIGN_RIGHT_MID, -10, 0);

  lv_obj_t *v = lv_label_create(row);
  lv_obj_set_style_text_font(v, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(v, lv_color_hex(val_color), 0);
  lv_obj_set_style_base_dir(v, val_ltr ? LV_BASE_DIR_LTR : (en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL), 0);
  lv_label_set_text(v, val);
  if (en) lv_obj_align(v, LV_ALIGN_RIGHT_MID, -10, 0);
  else lv_obj_align(v, LV_ALIGN_LEFT_MID, 10, 0);
}

static lv_draw_buf_t *f1_wifi_qr_draw_buf = NULL;

static void f1_wifi_show_info_modal() {
  if (f1_wifi_info_box && lv_obj_is_valid(f1_wifi_info_box)) return;
  const bool en = (GUI.system_language != 1);
  const bool is_sta = web_portal_is_sta();

  // Root modal backdrop (centered nicely on 480x320 screen)
  f1_wifi_info_box = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f1_wifi_info_box, 460, 290);
  lv_obj_center(f1_wifi_info_box);
  lv_obj_set_style_bg_color(f1_wifi_info_box, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(f1_wifi_info_box, 16, 0);
  lv_obj_set_style_border_width(f1_wifi_info_box, 2, 0);
  lv_obj_set_style_border_color(f1_wifi_info_box, lv_color_hex(is_sta ? 0x0284c7 : 0x16a34a), 0);
  lv_obj_set_style_shadow_width(f1_wifi_info_box, 0, 0);
  lv_obj_clear_flag(f1_wifi_info_box, LV_OBJ_FLAG_SCROLLABLE);
  GUI.lock_screen = true;

  // Title
  lv_obj_t *title = lv_label_create(f1_wifi_info_box);
  lv_obj_set_width(title, 370);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(is_sta ? 0x0284c7 : 0x16a34a), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
  lv_label_set_text(title, is_sta ? (en ? "Wi-Fi Network Info" : "اطلاعات شبکه وای‌فای")
                                  : (en ? "Direct SoftAP Info" : "اطلاعات اتصال مستقیم وای‌فای"));

  // Red Close button (top-right)
  lv_obj_t *x_btn = lv_button_create(f1_wifi_info_box);
  lv_obj_set_size(x_btn, 34, 34);
  lv_obj_align(x_btn, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_set_style_bg_color(x_btn, lv_color_hex(0xef4444), 0);
  lv_obj_set_style_radius(x_btn, 10, 0);
  lv_obj_add_event_cb(x_btn, f1_wifi_info_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *xl = lv_label_create(x_btn);
  lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_18, 0);
  lv_obj_center(xl);

  // Info card Container
  lv_obj_t *info_card = lv_obj_create(f1_wifi_info_box);
  lv_obj_set_pos(info_card, 12, 44);
  lv_obj_set_size(info_card, 436, 160);
  lv_obj_set_style_bg_color(info_card, lv_color_hex(0xf8fafc), 0);
  lv_obj_set_style_radius(info_card, 10, 0);
  lv_obj_set_style_border_width(info_card, 1, 0);
  lv_obj_set_style_border_color(info_card, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_pad_all(info_card, 0, 0);
  lv_obj_clear_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  if (!ds3231_read_time(&y, &m, &d, &hh, &mi, &ss)) {
    const int64_t ep = soft_now_epoch();
    if (ep > 0) epoch_to_civil(ep, &y, &m, &d, &hh, &mi, &ss);
  }
  char time_str[24];
  if (y > 0) snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hh, mi, ss);
  else snprintf(time_str, sizeof(time_str), "--:--:--");

  if (!is_sta) {
    f1_wifi_add_info_row(info_card, 6, "نام شبکه (SSID):", "Network (SSID):", ap_ssid, 0x0284c7, true, en);
    f1_wifi_add_info_row(info_card, 42, "رمز اتصال:", "Password:", ap_pass, 0x0f172a, true, en);
    f1_wifi_add_info_row(info_card, 78, "آدرس مرورگر:", "Browser URL:", "http://192.168.4.1", 0x0284c7, true, en);
    f1_wifi_add_info_row(info_card, 114, "ساعت سیستم:", "Device Clock:", time_str, 0x475569, true, en);
  } else {
    String ip_str = WiFi.localIP().toString();
    char ip_buf[48];
    snprintf(ip_buf, sizeof(ip_buf), "http://%s", ip_str.c_str());
    f1_wifi_add_info_row(info_card, 6, "شبکه متصل:", "Connected To:", WiFi.SSID().c_str(), 0x0284c7, true, en);
    f1_wifi_add_info_row(info_card, 42, "آدرس در مرورگر (IP):", "Browser URL (IP):", ip_buf, 0x0f172a, true, en);
    f1_wifi_add_info_row(info_card, 78, "لینک مستقیم:", "Direct Link:", "http://haffler.local", 0x0284c7, true, en);
    f1_wifi_add_info_row(info_card, 114, "ساعت سیستم:", "Device Clock:", time_str, 0x475569, true, en);
  }

  // Bottom action buttons — سه دکمهٔ متناسب درون کارت: 12 + 138 + 11 + 138 + 11 + 138 + 12 = 460
  // چیدمان طبیعی RTL: در فارسی [ورود با کیوآرکد (راست)] [قطع اتصال وای‌فای (وسط)] [بستن (چپ)]
  // در انگلیسی: [Login with QR (Left)] [Turn Off Wi-Fi (Mid)] [Close (Right)]
  const int pos_right = 310;
  const int pos_mid   = 161;
  const int pos_left  = 12;
  const int btn_y     = 222;
  const int btn_w     = 138;
  const int btn_h     = 44;

  // 1. دکمه ورود با کیوآرکد (عملیات اصلی / آبی)
  lv_obj_t *qr_btn = lv_button_create(f1_wifi_info_box);
  lv_obj_set_pos(qr_btn, en ? pos_left : pos_right, btn_y);
  lv_obj_set_size(qr_btn, btn_w, btn_h);
  lv_obj_set_style_bg_color(qr_btn, lv_color_hex(0x0284c7), 0);
  lv_obj_set_style_radius(qr_btn, 10, 0);
  lv_obj_add_event_cb(qr_btn, f1_wifi_qr_open_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *qr_lbl = lv_label_create(qr_btn);
  lv_obj_set_style_text_font(qr_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(qr_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_base_dir(qr_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(qr_lbl, en ? "Login with QR" : "ورود با کیوآرکد");
  lv_obj_center(qr_lbl);

  // 2. دکمه قطع اتصال وای‌فای (عملیات مهم / قرمز)
  lv_obj_t *off_btn = lv_button_create(f1_wifi_info_box);
  lv_obj_set_pos(off_btn, pos_mid, btn_y);
  lv_obj_set_size(off_btn, btn_w, btn_h);
  lv_obj_set_style_bg_color(off_btn, lv_color_hex(0xef4444), 0);
  lv_obj_set_style_radius(off_btn, 10, 0);
  lv_obj_add_event_cb(off_btn, f1_wifi_info_toggle_off_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *off_lbl = lv_label_create(off_btn);
  lv_obj_set_style_text_font(off_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(off_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_base_dir(off_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(off_lbl, en ? "Turn Off Wi-Fi" : "قطع اتصال وای‌فای");
  lv_obj_center(off_lbl);

  // 3. دکمه بستن (عملیات خنثی / خاکستری)
  lv_obj_t *close_btn = lv_button_create(f1_wifi_info_box);
  lv_obj_set_pos(close_btn, en ? pos_right : pos_left, btn_y);
  lv_obj_set_size(close_btn, btn_w, btn_h);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x64748b), 0);
  lv_obj_set_style_radius(close_btn, 10, 0);
  lv_obj_add_event_cb(close_btn, f1_wifi_info_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *close_lbl = lv_label_create(close_btn);
  lv_obj_set_style_text_font(close_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_base_dir(close_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(close_lbl, en ? "Close" : "بستن");
  lv_obj_center(close_lbl);
}

enum qrcodegen_Ecc {
  qrcodegen_Ecc_LOW = 0,
  qrcodegen_Ecc_MEDIUM,
  qrcodegen_Ecc_QUARTILE,
  qrcodegen_Ecc_HIGH,
};
enum qrcodegen_Mask {
  qrcodegen_Mask_AUTO = -1,
  qrcodegen_Mask_0 = 0,
  qrcodegen_Mask_1,
  qrcodegen_Mask_2,
  qrcodegen_Mask_3,
  qrcodegen_Mask_4,
  qrcodegen_Mask_5,
  qrcodegen_Mask_6,
  qrcodegen_Mask_7,
};
extern "C" {
  bool qrcodegen_encodeText(const char *text, uint8_t tempBuffer[], uint8_t qrcode[],
                            enum qrcodegen_Ecc ecl, int minVersion, int maxVersion,
                            enum qrcodegen_Mask mask, bool boostEcl);
  int qrcodegen_getSize(const uint8_t qrcode[]);
  bool qrcodegen_getModule(const uint8_t qrcode[], int x, int y);
}

// ---------- پنجرهٔ ورود با کیوآرکد (روی پنجرهٔ اطلاعات وای‌فای) ----------
static void f1_wifi_qr_close_cb(lv_event_t *e) {
  (void)e;
  if (f1_wifi_qr_box && lv_obj_is_valid(f1_wifi_qr_box)) {
    lv_obj_delete(f1_wifi_qr_box);
  }
  f1_wifi_qr_box = NULL;
  if (f1_wifi_qr_draw_buf) {
    lv_draw_buf_destroy(f1_wifi_qr_draw_buf);
    f1_wifi_qr_draw_buf = NULL;
  }
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
}

static void f1_wifi_qr_open_cb(lv_event_t *e) {
  (void)e;
  if (f1_wifi_qr_box && lv_obj_is_valid(f1_wifi_qr_box)) return;
  const bool en = (GUI.system_language != 1);

  char qr_url[64];
  if (web_portal_is_sta()) {
    snprintf(qr_url, sizeof(qr_url), "http://%s", WiFi.localIP().toString().c_str());
  } else {
    snprintf(qr_url, sizeof(qr_url), "http://192.168.4.1");
  }

  // ریشهٔ تمام‌صفحه کلیک‌خور → لمس پس‌زمینه بلاک می‌شود
  f1_wifi_qr_box = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f1_wifi_qr_box, 480, 320);
  lv_obj_set_pos(f1_wifi_qr_box, 0, 0);
  lv_obj_set_style_bg_color(f1_wifi_qr_box, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(f1_wifi_qr_box, LV_OPA_70, 0);
  lv_obj_set_style_border_width(f1_wifi_qr_box, 0, 0);
  lv_obj_set_style_pad_all(f1_wifi_qr_box, 0, 0);
  lv_obj_clear_flag(f1_wifi_qr_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(f1_wifi_qr_box, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(f1_wifi_qr_box);
  lv_obj_set_size(card, 272, 308);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x0284c7), 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // دکمهٔ بستن — هم‌سبک X پنجرهٔ اطلاعات
  lv_obj_t *x_btn = lv_button_create(card);
  lv_obj_set_size(x_btn, 34, 34);
  lv_obj_align(x_btn, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_set_style_bg_color(x_btn, lv_color_hex(0xef4444), 0);
  lv_obj_set_style_radius(x_btn, 10, 0);
  lv_obj_add_event_cb(x_btn, f1_wifi_qr_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *xl = lv_label_create(x_btn);
  lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_18, 0);
  lv_obj_center(xl);

  // رندر تضمینی کیوآرکد روی RGB565 Canvas با qrcodegen
  if (f1_wifi_qr_draw_buf) {
    lv_draw_buf_destroy(f1_wifi_qr_draw_buf);
    f1_wifi_qr_draw_buf = NULL;
  }
  uint8_t qrcode[256];
  uint8_t tempBuffer[256];
  bool ok = qrcodegen_encodeText(qr_url, tempBuffer, qrcode, qrcodegen_Ecc_LOW, 1, 4, qrcodegen_Mask_AUTO, true);
  if (ok) {
    int qr_dim = qrcodegen_getSize(qrcode);
    int scale = 180 / qr_dim;
    if (scale < 1) scale = 1;
    int canvas_sz = qr_dim * scale;
    f1_wifi_qr_draw_buf = lv_draw_buf_create(canvas_sz, canvas_sz, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
    if (f1_wifi_qr_draw_buf) {
      lv_obj_t *canvas = lv_canvas_create(card);
      lv_canvas_set_draw_buf(canvas, f1_wifi_qr_draw_buf);
      lv_canvas_fill_bg(canvas, lv_color_hex(0xffffff), LV_OPA_COVER);
      for (int y = 0; y < qr_dim; y++) {
        for (int x = 0; x < qr_dim; x++) {
          if (qrcodegen_getModule(qrcode, x, y)) {
            for (int sy = 0; sy < scale; sy++) {
              for (int sx = 0; sx < scale; sx++) {
                lv_canvas_set_px(canvas, x * scale + sx, y * scale + sy, lv_color_hex(0x000000), LV_OPA_COVER);
              }
            }
          }
        }
      }
      lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, 44);
    }
  }

  // توضیح دو خطی زیر کیوآرکد
  lv_obj_t *cap = lv_label_create(card);
  lv_obj_set_width(cap, 248);
  lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(cap, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(cap, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_line_space(cap, en ? -2 : -10, 0);
  lv_obj_set_style_text_color(cap, lv_color_hex(0x475569), 0);
  lv_label_set_text(cap, en ? "Scan with phone camera\nto open web app"
                            : "با دوربین گوشی اسکن کنید\nصفحه وب‌اپ باز می‌شود");
  lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 234);

  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
}

static void settings_web_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (f1_wifi_hold_handled) { f1_wifi_hold_handled = false; return; }
  if (GUI.lock_screen) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    ui_toast("ابتدا پنجره باز را ببندید", "Close the open window first", 2200);
    return;
  }
  if (STP.motorIs_moving || STP.flag_is_cutting) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    ui_toast("هنگام حرکت یا برش، وای فای تغییر نمی‌کند", "WiFi cannot change during motion/cutting", 3000);
    return;
  }
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  if (web_portal_running) {
    f1_wifi_show_info_modal();
    return;
  }
  WebWifiMode mode;
  if (web_wifi_saved_mode(&mode)) {
    if (mode == WEB_WIFI_STA) web_portal_request_sta_setup();
    else if (!web_portal_request_direct()) ui_toast("حافظه پورتال کافی نیست", "Not enough memory for portal", 3500);
    return;
  }
  f1_wifi_hold_cb(NULL);
}

static void f1_wifi_mode_close(void) {
  if (f1_wifi_mode_box) { lv_obj_delete(f1_wifi_mode_box); f1_wifi_mode_box = NULL; }
  GUI.lock_screen = false;
}
static void f1_wifi_mode_cb(lv_event_t *e) {
  const int pick = (int)(intptr_t)lv_event_get_user_data(e);
  f1_wifi_mode_close();
  if (pick == 1) web_portal_request_sta_setup();
  else if (pick == 0 && !web_portal_request_direct()) ui_toast("حافظه پورتال کافی نیست", "Not enough memory for portal", 3500);
}
static void f1_wifi_mode_x_cb(lv_event_t *e) { (void)e; f1_wifi_mode_close(); }

static void f1_wifi_hold_cb(lv_event_t *e) {
  if (e) f1_wifi_hold_handled = true;
  if (f1_wifi_mode_box || f1_wifi_info_box || GUI.lock_screen) return;
  if (web_portal_running) {
    f1_wifi_show_info_modal();
    return;
  }
  f1_wifi_mode_box = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f1_wifi_mode_box, 380, 200);
  lv_obj_center(f1_wifi_mode_box);
  lv_obj_set_style_bg_color(f1_wifi_mode_box, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(f1_wifi_mode_box, 16, 0);
  lv_obj_set_style_border_width(f1_wifi_mode_box, 1, 0);
  lv_obj_set_style_border_color(f1_wifi_mode_box, lv_color_hex(0xd7dde2), 0);
  lv_obj_set_style_shadow_width(f1_wifi_mode_box, 0, 0);
  lv_obj_clear_flag(f1_wifi_mode_box, LV_OBJ_FLAG_SCROLLABLE);
  GUI.lock_screen = true;

  const bool en = (GUI.system_language != 1);
  lv_obj_t *t = lv_label_create(f1_wifi_mode_box);
  lv_label_set_text(t, en ? "Select Wi-Fi Mode" : "روش اتصال وای فای");
  lv_obj_set_style_text_font(t, en ? &lv_font_montserrat_20 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x1c1c1e), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *x_btn = lv_button_create(f1_wifi_mode_box);
  lv_obj_set_size(x_btn, 32, 32);
  lv_obj_align(x_btn, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_set_style_bg_color(x_btn, lv_color_hex(0xd92d20), 0);
  lv_obj_set_style_radius(x_btn, 8, 0);
  lv_obj_add_event_cb(x_btn, f1_wifi_mode_x_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *xl = lv_label_create(x_btn);
  lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(xl, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_18, 0);
  lv_obj_center(xl);

  const char *txt_fa[2] = {"اتصال مستقیم (SoftAP)", "هات اسپات یا مودم (STA)"};
  const char *txt_en[2] = {"Direct Connection (SoftAP)", "Modem / Hotspot (STA)"};
  const int y[2] = {58, 114};
  for (int i = 0; i < 2; ++i) {
    lv_obj_t *b = lv_button_create(f1_wifi_mode_box);
    lv_obj_set_size(b, 320, 44);
    lv_obj_set_pos(b, 30, y[i]);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(i ? 0x0a84ff : 0x1b9e4a), 0);
    lv_obj_add_event_cb(b, f1_wifi_mode_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, en ? txt_en[i] : txt_fa[i]);
    lv_obj_set_style_text_font(l, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), 0);
    lv_obj_center(l);
  }
}

static lv_obj_t *mk_tiny_adj(lv_obj_t *parent, const char *sym, int x, int y, int code) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, 30, 30);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x1976d2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn, settings_kerf_adj_cb, LV_EVENT_RELEASED, (void *)(intptr_t)code);
  lv_obj_t *l = lv_label_create(btn);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(l);
  lv_label_set_text(l, sym);
  return btn;
}

static lv_obj_t *mk_value_box(lv_obj_t *parent, int x, int y, int w, int h, int which) {
  // طراح id19/21: bg #f4f8fc، radius 12، عدد+mm افقی
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_set_pos(box, x, y);
  lv_obj_set_size(box, w, h);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(box, lv_color_hex(0xf4f8fc), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(box, lv_color_hex(0xd0e4f2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(box, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(box, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
  // CLICKED + RELEASED: بعد از USB گاهی فقط یکی می‌آید؛ لیبل‌ها کلیک را ندزدند
  lv_obj_add_event_cb(box, settings_open_num_cb, LV_EVENT_CLICKED, (void *)(intptr_t)which);
  lv_obj_add_event_cb(box, settings_open_num_cb, LV_EVENT_RELEASED, (void *)(intptr_t)which);
  return box;
}

// user_data: 1=icon label, 2=caption — بچه‌های EEZ را مخفی می‌کنیم تا ذخیره/خانه خالی نماند
static void f1_fill_hdr_btn(lv_obj_t *btn, const char *sym, const char *caption) {
  if (!btn) return;
  lv_obj_t *ic = NULL, *cap = NULL;
  uint32_t n = lv_obj_get_child_count(btn);
  for (uint32_t i = 0; i < n; i++) {
    lv_obj_t *ch = lv_obj_get_child(btn, i);
    if (!ch) continue;
    intptr_t tag = (intptr_t)lv_obj_get_user_data(ch);
    if (tag == 1) ic = ch;
    else if (tag == 2) cap = ch;
    else lv_obj_add_flag(ch, LV_OBJ_FLAG_HIDDEN);
  }
  const bool has_sym = sym && sym[0];
  const bool has_cap = caption && caption[0];
  // naz18b line_height≈29 — بدون clip تا متن دیده شود؛ scale برای جا شدن در ۳۲px
  lv_obj_set_style_clip_corner(btn, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  // absolute children — نه flex (flex + ارتفاع فونت فارسی = برش کامل)
  lv_obj_set_layout(btn, LV_LAYOUT_NONE);

  if (has_sym) {
    if (!ic) {
      ic = lv_label_create(btn);
      lv_obj_set_user_data(ic, (void *)(intptr_t)1);
      lv_obj_set_style_text_color(ic, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_set_style_text_font(ic, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ic, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ic, sym);
    if (has_cap) lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 1);
    else lv_obj_center(ic);
  } else if (ic) {
    lv_obj_add_flag(ic, LV_OBJ_FLAG_HIDDEN);
  }

  if (has_cap) {
    if (!cap) {
      cap = lv_label_create(btn);
      lv_obj_set_user_data(cap, (void *)(intptr_t)2);
      lv_obj_set_style_text_color(cap, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_clear_flag(cap, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(cap, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(cap, -14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(cap, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(cap, 40);
    lv_obj_set_height(cap, LV_SIZE_CONTENT);
    // ~55٪ تا در دکمهٔ ۳۲px جا شود (naz18b خط ۲۹)
    lv_obj_set_style_transform_scale_x(cap, 140, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(cap, 140, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_x(cap, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(cap, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (has_sym) lv_obj_align(cap, LV_ALIGN_BOTTOM_MID, 0, 2);
    else lv_obj_center(cap);
    lv_label_set_text(cap, caption);
  } else if (cap) {
    lv_obj_add_flag(cap, LV_OBJ_FLAG_HIDDEN);
  }
}

// نوار خطر زرد/مشکی (برای استاپ طراح)
static void f1_draw_hazard_bar(lv_obj_t *parent, int y, int w) {
  const int stripe = 8;
  for (int x = 0; x < w; x += stripe) {
    lv_obj_t *s = lv_obj_create(parent);
    lv_obj_set_pos(s, x, y);
    lv_obj_set_size(s, stripe, 10);
    lv_obj_clear_flag(s, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    bool yel = ((x / stripe) & 1) == 0;
    lv_obj_set_style_bg_color(s, lv_color_hex(yel ? 0xffeb3b : 0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static lv_obj_t *f1_buz_chip = NULL;
static lv_obj_t *f1_estop_art = NULL;
static lv_obj_t *f1_ncno = NULL;
static lv_obj_t *f1_ncno_bg = NULL;
static lv_obj_t *f1_ncno_desc = NULL;

static void f1_sync_estop_visual() {
  // کم‌رنگ نشود — فقط برچسب N.C / N.O و توضیح زبان عوض شود
  if (f1_estop_art)
    lv_obj_set_style_opa(f1_estop_art, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (objects.st_bt_co)
    lv_obj_set_style_opa(objects.st_bt_co, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (f1_ncno_bg)
    lv_obj_set_style_opa(f1_ncno_bg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (f1_ncno) {
    lv_label_set_text(f1_ncno, stopstate ? "N.C" : "N.O");
    lv_obj_set_style_opa(f1_ncno, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (f1_ncno_desc_lbl) {
    const bool en = (GUI.system_language == 2);
    if (en) {
      lv_obj_set_style_text_font(f1_ncno_desc_lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(f1_ncno_desc_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(f1_ncno_desc_lbl, stopstate ? "N.Closed" : "N.Open");
    } else {
      lv_obj_set_style_text_font(f1_ncno_desc_lbl, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(f1_ncno_desc_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(f1_ncno_desc_lbl, stopstate ? "عادی بسته" : "عادی باز");
    }
    lv_obj_set_style_opa(f1_ncno_desc_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if (f1_ncno_desc) {
    const bool en = (GUI.system_language == 2);
    lv_label_set_text(f1_ncno_desc, en
      ? (stopstate ? "N.Closed" : "N.Open")
      : (stopstate ? "عادی بسته" : "عادی باز"));
  }
}

static lv_obj_t *s_vol_modal_backdrop = NULL;
static lv_obj_t *s_vol_slider_sys = NULL;
static lv_obj_t *s_vol_slider_alrm = NULL;
static lv_obj_t *s_lbl_sys_vol_pct = NULL;
static lv_obj_t *s_lbl_alrm_vol_pct = NULL;
static lv_timer_t *s_vol_dismiss_timer = NULL;

static void ui_close_volume_modal() {
  if (s_vol_dismiss_timer) {
    lv_timer_delete(s_vol_dismiss_timer);
    s_vol_dismiss_timer = NULL;
  }
  if (s_vol_modal_backdrop && lv_obj_is_valid(s_vol_modal_backdrop)) {
    lv_obj_delete(s_vol_modal_backdrop);
    s_vol_modal_backdrop = NULL;
    s_vol_slider_sys = NULL;
    s_vol_slider_alrm = NULL;
    s_lbl_sys_vol_pct = NULL;
    s_lbl_alrm_vol_pct = NULL;
  }
  save_volumes();
  save_buzzer_state();
  if (f1_buz_chip && lv_obj_is_valid(f1_buz_chip)) {
    bool active = GUI.buzzer_enable && (sys_volume > 0 || alarm_volume > 0);
    lv_obj_set_style_bg_color(f1_buz_chip, lv_color_hex(active ? 0x0a84ff : 0xc5ccd3),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *ic = lv_obj_get_child(f1_buz_chip, 0);
    if (ic) lv_label_set_text(ic, active ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
  }
}

static void vol_dismiss_timer_cb(lv_timer_t *t) {
  (void)t;
  ui_close_volume_modal();
}

static void on_vol_slider_sys_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;
  if (s_vol_dismiss_timer) lv_timer_reset(s_vol_dismiss_timer);
  int32_t val = lv_slider_get_value(s_vol_slider_sys);
  if (val < 0) val = 0; if (val > 100) val = 100;
  sys_volume = (uint8_t)val;
  if (s_lbl_sys_vol_pct) lv_label_set_text_fmt(s_lbl_sys_vol_pct, "%d%%", (int)val);
  if (code == LV_EVENT_RELEASED) {
    if (sys_volume > 0) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  }
}

static void on_vol_slider_alrm_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;
  if (s_vol_dismiss_timer) lv_timer_reset(s_vol_dismiss_timer);
  int32_t val = lv_slider_get_value(s_vol_slider_alrm);
  if (val < 0) val = 0; if (val > 100) val = 100;
  alarm_volume = (uint8_t)val;
  if (s_lbl_alrm_vol_pct) lv_label_set_text_fmt(s_lbl_alrm_vol_pct, "%d%%", (int)val);
  if (code == LV_EVENT_RELEASED) {
    if (alarm_volume > 0) (void)buzzer_request_pattern(BUZZER_PATTERN_WARN);
  }
}

static void ui_open_volume_modal() {
  if (s_vol_modal_backdrop && lv_obj_is_valid(s_vol_modal_backdrop)) {
    ui_close_volume_modal();
    return;
  }
  s_vol_modal_backdrop = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_vol_modal_backdrop, 480, 320);
  lv_obj_set_pos(s_vol_modal_backdrop, 0, 0);
  lv_obj_set_style_bg_opa(s_vol_modal_backdrop, LV_OPA_30, 0);
  lv_obj_set_style_bg_color(s_vol_modal_backdrop, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_width(s_vol_modal_backdrop, 0, 0);
  lv_obj_set_style_pad_all(s_vol_modal_backdrop, 0, 0);
  lv_obj_clear_flag(s_vol_modal_backdrop, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_vol_modal_backdrop, [](lv_event_t *e) {
    if (lv_event_get_target(e) == s_vol_modal_backdrop) {
      ui_close_volume_modal();
    }
  }, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(s_vol_modal_backdrop);
  lv_obj_set_size(card, 280, 218);
  lv_obj_set_pos(card, 100, 48);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_bg_opa(card, 250, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x38bdf8), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_pad_all(card, 8, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *ttl = lv_label_create(card);
  lv_label_set_text(ttl, "تنظیم صدای دستگاه");
  lv_obj_set_style_text_font(ttl, &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(ttl, lv_color_hex(0xf8fafc), 0);
  lv_obj_align(ttl, LV_ALIGN_TOP_MID, 0, 2);

  lv_obj_t *btn_x = lv_button_create(card);
  lv_obj_set_size(btn_x, 28, 28);
  lv_obj_set_pos(btn_x, 4, 2);
  lv_obj_set_style_bg_color(btn_x, lv_color_hex(0x334155), 0);
  lv_obj_set_style_radius(btn_x, 14, 0);
  lv_obj_set_style_shadow_width(btn_x, 0, 0);
  lv_obj_add_event_cb(btn_x, [](lv_event_t *e) {
    (void)e;
    ui_close_volume_modal();
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_x = lv_label_create(btn_x);
  lv_label_set_text(lbl_x, "X");
  lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xffffff), 0);
  lv_obj_center(lbl_x);

  lv_obj_t *col1 = lv_obj_create(card);
  lv_obj_set_size(col1, 124, 162);
  lv_obj_set_pos(col1, 8, 38);
  lv_obj_set_style_bg_color(col1, lv_color_hex(0x1e293b), 0);
  lv_obj_set_style_radius(col1, 12, 0);
  lv_obj_set_style_border_color(col1, lv_color_hex(0x334155), 0);
  lv_obj_set_style_border_width(col1, 1, 0);
  lv_obj_set_style_shadow_width(col1, 0, 0);
  lv_obj_set_style_pad_all(col1, 4, 0);
  lv_obj_clear_flag(col1, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_sys = lv_label_create(col1);
  lv_label_set_text(lbl_sys, "صدای سیستم");
  lv_obj_set_style_text_font(lbl_sys, &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lbl_sys, lv_color_hex(0x94a3b8), 0);
  lv_obj_align(lbl_sys, LV_ALIGN_TOP_MID, 0, -3);

  s_lbl_sys_vol_pct = lv_label_create(col1);
  lv_label_set_text_fmt(s_lbl_sys_vol_pct, "%d%%", (int)sys_volume);
  lv_obj_set_style_text_font(s_lbl_sys_vol_pct, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_lbl_sys_vol_pct, lv_color_hex(0x38bdf8), 0);
  lv_obj_align(s_lbl_sys_vol_pct, LV_ALIGN_TOP_MID, 0, 26);

  s_vol_slider_sys = lv_slider_create(col1);
  lv_obj_set_size(s_vol_slider_sys, 28, 92);
  lv_obj_align(s_vol_slider_sys, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_slider_set_range(s_vol_slider_sys, 0, 100);
  lv_slider_set_value(s_vol_slider_sys, sys_volume, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(s_vol_slider_sys, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_vol_slider_sys, lv_color_hex(0x0284c7), LV_PART_INDICATOR);
  lv_obj_set_style_radius(s_vol_slider_sys, 12, LV_PART_MAIN);
  lv_obj_set_style_radius(s_vol_slider_sys, 12, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(s_vol_slider_sys, 0, LV_PART_KNOB);
  lv_obj_add_event_cb(s_vol_slider_sys, on_vol_slider_sys_event, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(s_vol_slider_sys, on_vol_slider_sys_event, LV_EVENT_RELEASED, NULL);

  lv_obj_t *col2 = lv_obj_create(card);
  lv_obj_set_size(col2, 124, 162);
  lv_obj_set_pos(col2, 138, 38);
  lv_obj_set_style_bg_color(col2, lv_color_hex(0x1e293b), 0);
  lv_obj_set_style_radius(col2, 12, 0);
  lv_obj_set_style_border_color(col2, lv_color_hex(0x334155), 0);
  lv_obj_set_style_border_width(col2, 1, 0);
  lv_obj_set_style_shadow_width(col2, 0, 0);
  lv_obj_set_style_pad_all(col2, 4, 0);
  lv_obj_clear_flag(col2, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_alrm = lv_label_create(col2);
  lv_label_set_text(lbl_alrm, "صدای هشدار");
  lv_obj_set_style_text_font(lbl_alrm, &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lbl_alrm, lv_color_hex(0x94a3b8), 0);
  lv_obj_align(lbl_alrm, LV_ALIGN_TOP_MID, 0, -3);

  s_lbl_alrm_vol_pct = lv_label_create(col2);
  lv_label_set_text_fmt(s_lbl_alrm_vol_pct, "%d%%", (int)alarm_volume);
  lv_obj_set_style_text_font(s_lbl_alrm_vol_pct, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_lbl_alrm_vol_pct, lv_color_hex(0xf59e0b), 0);
  lv_obj_align(s_lbl_alrm_vol_pct, LV_ALIGN_TOP_MID, 0, 26);

  s_vol_slider_alrm = lv_slider_create(col2);
  lv_obj_set_size(s_vol_slider_alrm, 28, 92);
  lv_obj_align(s_vol_slider_alrm, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_slider_set_range(s_vol_slider_alrm, 0, 100);
  lv_slider_set_value(s_vol_slider_alrm, alarm_volume, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(s_vol_slider_alrm, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_vol_slider_alrm, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
  lv_obj_set_style_radius(s_vol_slider_alrm, 12, LV_PART_MAIN);
  lv_obj_set_style_radius(s_vol_slider_alrm, 12, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(s_vol_slider_alrm, 0, LV_PART_KNOB);
  lv_obj_add_event_cb(s_vol_slider_alrm, on_vol_slider_alrm_event, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(s_vol_slider_alrm, on_vol_slider_alrm_event, LV_EVENT_RELEASED, NULL);

  s_vol_dismiss_timer = lv_timer_create(vol_dismiss_timer_cb, 6000, NULL);
}

static void f1_buz_chip_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_enable = true;
  PEND.buzzer_enable = true;
  ui_open_volume_modal();
}

static void f1_estop_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  // اگر لوگوی استاپ سخت‌افزاری به‌خاطر تغییر قطبیت صفحه را قفل کرده، آزاد کن
  if (GUI.lock_screen && objects.emg && lv_obj_is_valid(objects.emg)) {
    GUI.flag_del_emg = true;
    GUI.lock_screen = false;
  }
  if (GUI.lock_screen) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    return;
  }
  // فقط جابه‌جایی N.O / N.C — بدون ledc روی GPIO45 (با بک‌لایت/بوق مشترک بود)
  // بدون hide_eez سنگین و بدون toast سنگین (علت هنگ لمس استاپ)
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  stopstate = !stopstate;
  // NVS فقط با Save
  f1_sync_estop_visual();
  settings_mark_dirty();
}

// حذف امن ویجت EEZ جایگزین‌شده (یک‌بار) — اشاره‌گر objects را NULL می‌کند
static void settings_retire_eez_obj(lv_obj_t **slot) {
  if (!slot || !*slot) return;
  lv_obj_t *o = *slot;
  *slot = NULL;
  if (lv_obj_is_valid(o)) lv_obj_delete(o);
}

// مخفی/حذف ویجت‌های قدیمی EEZ که با طرح طراح تداخل دارند
// باید هم‌زمان با تعویض تب F صدا زده شود تا یک فریم فلش تصویر قدیمی نماند
void settings_hide_eez_clutter() {
  // یک‌بار: تصاویر بازر / N.C-N.O / دکمهٔ تست چاپ قدیمی را واقعاً پاک کن
  static bool retired_imgs = false;
  if (!retired_imgs) {
    retired_imgs = true;
    settings_retire_eez_obj(&objects.img2_1);
    settings_retire_eez_obj(&objects.img2_2);
    settings_retire_eez_obj(&objects.normally_closed);
    settings_retire_eez_obj(&objects.normally_open);
    // دکمهٔ قدیمی «تست چاپ» (پدربزرگ لیبل)
    if (objects.lablef_print) {
      lv_obj_t *p = lv_obj_get_parent(objects.lablef_print);
      if (p) p = lv_obj_get_parent(p);
      objects.lablef_print = NULL;
      objects.lablee_print = NULL;
      if (p && lv_obj_is_valid(p)) lv_obj_delete(p);
    } else {
      objects.lablee_print = NULL;
    }
    // آیکن استاپ قدیمی EEZ (والد st_bt_co)
    if (objects.st_bt_co) {
      lv_obj_t *old = lv_obj_get_parent(objects.st_bt_co);
      objects.st_bt_co = NULL;
      if (old && lv_obj_is_valid(old)) lv_obj_delete(old);
    }
  } else {
    if (objects.img2_1) lv_obj_add_flag(objects.img2_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.img2_2) lv_obj_add_flag(objects.img2_2, LV_OBJ_FLAG_HIDDEN);
    if (objects.normally_closed) lv_obj_add_flag(objects.normally_closed, LV_OBJ_FLAG_HIDDEN);
    if (objects.normally_open) lv_obj_add_flag(objects.normally_open, LV_OBJ_FLAG_HIDDEN);
  }
  // کادر سفید عنوان صفحه در F1..F5 — عنوان در هدر مشترک است
  // (زبان‌سوییچ ممکن است دوباره clear_flag کند → هر بار hide)
  lv_obj_t *tab_titles_f[] = {
    objects.lablef2_1, objects.lablef2_2, objects.lablef2_3,
    objects.lablef2_4, objects.lablef2_5
  };
  lv_obj_t *tab_titles_e[] = {
    objects.lablee2_1, objects.lablee2_2, objects.lablee2_3,
    objects.lablee2_4, objects.lablee2_5
  };
  for (unsigned i = 0; i < sizeof(tab_titles_f) / sizeof(tab_titles_f[0]); i++) {
    if (!tab_titles_f[i]) continue;
    lv_obj_t *box = lv_obj_get_parent(tab_titles_f[i]);
    if (box) {
      lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_add_flag(tab_titles_f[i], LV_OBJ_FLAG_HIDDEN);
  }
  for (unsigned i = 0; i < sizeof(tab_titles_e) / sizeof(tab_titles_e[0]); i++) {
    if (tab_titles_e[i]) lv_obj_add_flag(tab_titles_e[i], LV_OBJ_FLAG_HIDDEN);
  }
  // پنل‌های toast استاپ را هم کوچک/مخفی نگه دار تا وسط صفحه را نگیرند
  if (objects.panel_no) lv_obj_add_flag(objects.panel_no, LV_OBJ_FLAG_HIDDEN);
  if (objects.panel_nc) lv_obj_add_flag(objects.panel_nc, LV_OBJ_FLAG_HIDDEN);
}

static inline void f1_hide_eez_clutter() { settings_hide_eez_clutter(); }

static lv_obj_t *f1_help_btn = NULL;
static lv_obj_t *f1_lang_btn = NULL;
static lv_obj_t *f1_title_bar = NULL;

/*==============================================================================
 * صفحه تنظیمات عمومی (F1) — نقشهٔ مختصات
 * مرجع: simulator/settings-f1-designer.html → initial[]
 *
 * مهم:
 *   - اعداد پایین = پیکسل صفحهٔ ۴۸۰×۳۲۰ (مثل طراح).
 *   - هدر (خانه/راهنما/عنوان/FA/ذخیره) روی parent دکمهٔ EEZ است → همان x,y صفحه.
 *   - بقیه روی objects.obj5 است که در EEZ روی (-16,-16) است →
 *     برای جابه‌جایی از f1_lx(x) / f1_ly(y) استفاده کن (یعنی +۱۶).
 *   - برای جابه‌جا کردن یک آیتم: فقط عدد x/y همان کامنت را عوض کن.
 *============================================================================*/
enum {
  // --- هدر: تراز کامل تمام ۵ آیتم در Y=6 با ارتفاع یکدست ۳۸px ---
  F1_HDR_Y = 6, F1_HDR_H = 38, F1_HDR_W = 44, F1_HDR_GAP = 6,
  F1_HOME_X = 6,
  F1_HELP_X = 56,                                      // فاصله استاندارد ۶px بعد از خانه (۶+۴۴+۶)
  F1_TITLE_X = 106, F1_TITLE_W = 204, F1_TITLE_H = 38, // هدر وسط دقیقاً هم‌تراز با بقیه در Y=6 و H=38
  F1_TITLE_Y = 6,
  F1_LANG_X = 316,                                     // ۶px بعد از باکس عنوان (۱۰۶+۲۰۴+۶)
  F1_SAVE_X = 366,                                     // ۶px بعد از دکمه زبان (۳۱۶+۴۴+۶)

  // --- پنل ابزار سمت چپ: نوار سفید در بالا (Y=20) و کلیدها ۲px بالاتر ---
  F1_TOOLS_X = -24, F1_TOOLS_Y = 20, F1_TOOLS_W = 92, F1_TOOLS_H = 286,
  F1_WIFI_X = -18, F1_WIFI_Y = 26, F1_WIFI_W = 80, F1_WIFI_H = 28,
  F1_KB_X = -18, F1_KB_Y = 61, F1_KB_W = 80, F1_KB_H = 26,
  F1_BUZ_X = -18, F1_BUZ_Y = 94, F1_BUZ_W = 80, F1_BUZ_H = 26,
  F1_PRINT_X = -18, F1_PRINT_Y = 127, F1_PRINT_W = 80, F1_PRINT_H = 28,
  F1_ESTOP_X = -20, F1_ESTOP_Y = 162, F1_ESTOP_W = 80, F1_ESTOP_H = 92,

  // --- کارت مقادیر: طول شاخه / عرض تیغه ---
  F1_CARD_X = 76, F1_CARD_Y = 20, F1_CARD_W = 300, F1_CARD_H = 82,
  F1_STOCK_LBL_X = 84, F1_STOCK_LBL_Y = 19, F1_STOCK_LBL_W = 136, F1_STOCK_LBL_H = 28,
  F1_STOCK_BOX_X = 84, F1_STOCK_BOX_Y = 47, F1_STOCK_BOX_W = 136, F1_STOCK_BOX_H = 46,
  F1_KERF_LBL_X = 232, F1_KERF_LBL_Y = 19, F1_KERF_LBL_W = 132, F1_KERF_LBL_H = 28,
  F1_KERF_BOX_X = 232, F1_KERF_BOX_Y = 47, F1_KERF_BOX_W = 132, F1_KERF_BOX_H = 46,
  // --- کارت سرویس زیر ابزار پرینتر در پنل ابزار چپ (فواصل ۷px، ارتفاع کلیدها ۳۵px) ---
  F1_FACTORY_CARD_X = -18, F1_FACTORY_CARD_Y = 162, F1_FACTORY_CARD_W = 80, F1_FACTORY_CARD_H = 121,
  F1_FACTORY_X = 0, F1_FACTORY_Y = 0, F1_FACTORY_W = 80, F1_FACTORY_H = 35,
  F1_FACTORY_RESERVED_X = 0, F1_FACTORY_RESERVED_Y = 42, F1_FACTORY_RESERVED_W = 80, F1_FACTORY_RESERVED_H = 35,
  F1_FACTORY_RES_X = 0, F1_FACTORY_RES_Y = 84, F1_FACTORY_RES_W = 80, F1_FACTORY_RES_H = 35,
  F1_NUMPAD_X = 110, F1_NUMPAD_Y = 144, F1_NUMPAD_W = 300, F1_NUMPAD_H = 166,
};

// تبدیل مختصات صفحه → مختصات محلی داخل obj5 (چون obj5 در (-16,-16) است)
static inline int f1_lx(int sx) { return sx + 16; }
static inline int f1_ly(int sy) { return sy + 16; }
// موقعیت نسبی داخل کارت ابزار (tools در صفحه روی (8,48) است)
static inline int f1_tx(int sx) { return sx - F1_TOOLS_X; }
static inline int f1_ty(int sy) { return sy - F1_TOOLS_Y; }

static lv_obj_t *f1_hdr_root() {
  if (objects.btn2_1) return lv_obj_get_parent(objects.btn2_1);
  return objects.main_2;
}

static void f1_style_hdr_btn(lv_obj_t *btn, uint32_t bg) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x0066cc), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_radius(btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(btn, false, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// فوروارد — تعریف بعد از f1_book_rect
static void f1_hdr_set_home_icon(lv_obj_t *btn);
static void f1_hdr_set_save_icon(lv_obj_t *btn);

// آیکون هدر پایدار: سمبل LVGL فونت ۲۶ وسط — بدون transform و بدون هندسهٔ شکننده
static void f1_hdr_icon_only(lv_obj_t *btn, const char *sym) {
  if (!btn || !sym) return;
  // سمبل‌های قدیمی هندسی را پاک کن (قبلاً خانه/ذخیره را خراب می‌کرد)
  for (int i = (int)lv_obj_get_child_count(btn) - 1; i >= 0; i--) {
    lv_obj_t *ch = lv_obj_get_child(btn, (uint32_t)i);
    if (!ch) continue;
    intptr_t t = (intptr_t)lv_obj_get_user_data(ch);
    if (t == 43 || t == 44) lv_obj_delete(ch);
  }
  f1_fill_hdr_btn(btn, sym, "");
  lv_obj_t *ic = NULL;
  uint32_t n = lv_obj_get_child_count(btn);
  for (uint32_t i = 0; i < n; i++) {
    lv_obj_t *ch = lv_obj_get_child(btn, i);
    if (ch && (intptr_t)lv_obj_get_user_data(ch) == 1) { ic = ch; break; }
  }
  if (ic) {
    lv_obj_set_style_text_font(ic, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ic, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ic, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ic, LV_OBJ_FLAG_HIDDEN);
    lv_obj_center(ic);
    lv_obj_move_foreground(ic);
  }
  lv_obj_set_style_clip_corner(btn, false, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// پنهان‌کردن لیبل‌های متن/سمبل داخل دکمهٔ هدر (برای آیکون سفارشی)
static void f1_hdr_hide_text_kids(lv_obj_t *btn) {
  if (!btn) return;
  uint32_t n = lv_obj_get_child_count(btn);
  for (uint32_t i = 0; i < n; i++) {
    lv_obj_t *ch = lv_obj_get_child(btn, i);
    if (!ch) continue;
    intptr_t tag = (intptr_t)lv_obj_get_user_data(ch);
    if (tag == 1 || tag == 2) lv_obj_add_flag(ch, LV_OBJ_FLAG_HIDDEN);
  }
}

// پرچم هدر — ~۱۰٪ بزرگ‌تر (۲۹×۱۹) وسط دکمه — بدون transform
static lv_obj_t *f1_mk_mini_flag(lv_obj_t *parent, int kind) {
  const int FW = 29, FH = 19;
  lv_obj_t *flag = lv_obj_create(parent);
  lv_obj_set_size(flag, FW, FH);
  lv_obj_set_user_data(flag, (void *)(intptr_t)3);
  lv_obj_clear_flag(flag, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_radius(flag, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(flag, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(flag, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(flag, lv_color_hex(0xcccccc), LV_PART_MAIN | LV_STATE_DEFAULT);
  if (kind == 0) {
    lv_obj_set_style_bg_opa(flag, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    const int ys[3] = {0, 7, 13};
    const int hs[3] = {7, 6, 6};
    const uint32_t cs[3] = {0x239f40, 0xffffff, 0xda0000};
    for (int i = 0; i < 3; i++) {
      lv_obj_t *o = lv_obj_create(flag);
      lv_obj_set_pos(o, 0, ys[i]);
      lv_obj_set_size(o, FW, hs[i]);
      lv_obj_clear_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
      lv_obj_set_style_bg_color(o, lv_color_hex(cs[i]), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  } else {
    lv_obj_set_style_bg_color(flag, lv_color_hex(0x012169), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(flag, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    static lv_point_precise_t d1[2] = {{0, 0}, {29, 19}};
    static lv_point_precise_t d2[2] = {{29, 0}, {0, 19}};
    for (int pass = 0; pass < 4; pass++) {
      lv_obj_t *o = lv_line_create(flag);
      lv_obj_set_pos(o, 0, 0);
      lv_obj_clear_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
      uint32_t c = (pass < 2) ? 0xffffff : 0xc8102e;
      int w = (pass < 2) ? 5 : 2;
      lv_obj_set_style_line_color(o, lv_color_hex(c), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_line_width(o, w, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_line_set_points(o, (pass % 2) ? d2 : d1, 2);
    }
    const int bx[4] = {0, 0, 12, 13};
    const int by[4] = {7, 8, 0, 0};
    const int bw[4] = {29, 29, 5, 3};
    const int bh[4] = {5, 3, 19, 19};
    const uint32_t bc[4] = {0xffffff, 0xc8102e, 0xffffff, 0xc8102e};
    for (int i = 0; i < 4; i++) {
      lv_obj_t *o = lv_obj_create(flag);
      lv_obj_set_pos(o, bx[i], by[i]);
      lv_obj_set_size(o, bw[i], bh[i]);
      lv_obj_clear_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
      lv_obj_set_style_bg_color(o, lv_color_hex(bc[i]), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
  return flag;
}

// کتاب بسته + نشانک قرمز (گزینه ۴ — جزئیات بیشتر / کیفیت بالاتر)
static lv_obj_t *f1_book_rect(lv_obj_t *parent, int x, int y, int w, int h, uint32_t c, int radius) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_clear_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_color(o, lv_color_hex(c), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(o, radius, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  return o;
}

static inline int f1_book_s(int v) { return (v * 29 + 13) / 26; }
static lv_obj_t *f1_book_rect_s(lv_obj_t *parent, int x, int y, int w, int h, uint32_t c, int radius) {
  return f1_book_rect(parent, f1_book_s(x), f1_book_s(y), f1_book_s(w), f1_book_s(h), c,
                      radius ? f1_book_s(radius) : 0);
}

static void f1_hdr_set_home_icon(lv_obj_t *btn) {
  f1_hdr_icon_only(btn, LV_SYMBOL_HOME);
}

static void f1_hdr_set_save_icon(lv_obj_t *btn) {
  f1_hdr_icon_only(btn, LV_SYMBOL_SAVE);
}

static void f1_hdr_set_book_icon(lv_obj_t *btn) {
  if (!btn) return;
  f1_hdr_hide_text_kids(btn);
  lv_obj_set_style_clip_corner(btn, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  const intptr_t BOOK_TAG = 42;  // کتاب ۲۶×۲۶ — نسخهٔ پایدار قبلی
  lv_obj_t *book = NULL;
  for (int i = (int)lv_obj_get_child_count(btn) - 1; i >= 0; i--) {
    lv_obj_t *ch = lv_obj_get_child(btn, (uint32_t)i);
    if (!ch) continue;
    intptr_t tag = (intptr_t)lv_obj_get_user_data(ch);
    if (tag == BOOK_TAG) book = ch;
    else if (tag == 4 || tag == 41) lv_obj_delete(ch);
  }
  if (book && lv_obj_get_width(book) != 29) {
    lv_obj_delete(book);
    book = NULL;
  }
  if (!book) {
    book = lv_obj_create(btn);
    lv_obj_set_user_data(book, (void *)BOOK_TAG);
    lv_obj_set_size(book, 29, 29);
    lv_obj_clear_flag(book, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_style_bg_opa(book, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(book, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(book, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    f1_book_rect_s(book, 5, 5, 20, 17, 0xb5d0ea, 3);
    f1_book_rect_s(book, 3, 3, 20, 18, 0xa8c8e6, 3);
    lv_obj_t *cover = f1_book_rect_s(book, 2, 2, 20, 18, 0xffffff, 3);
    f1_book_rect_s(cover, 4, 1, 14, 1, 0xeef5fc, 0);
    f1_book_rect_s(cover, 0, 0, 4, 18, 0x9ec4e8, 2);
    f1_book_rect_s(cover, 1, 1, 2, 16, 0xd4e6f7, 1);
    f1_book_rect_s(cover, 1, 3, 1, 2, 0xffffff, 0);
    f1_book_rect_s(cover, 1, 7, 1, 2, 0xffffff, 0);
    f1_book_rect_s(cover, 1, 11, 1, 2, 0xffffff, 0);
    f1_book_rect_s(cover, 1, 15, 1, 2, 0xffffff, 0);
    f1_book_rect_s(cover, 6, 4, 11, 2, 0x6fb0ff, 1);
    f1_book_rect_s(cover, 6, 8, 11, 2, 0x93c4ff, 1);
    f1_book_rect_s(cover, 6, 12, 8, 2, 0xb7d6ff, 1);
    f1_book_rect_s(book, 4, 19, 17, 2, 0xc9dff3, 1);
    f1_book_rect_s(book, 5, 20, 15, 1, 0xe8f2fb, 0);
    f1_book_rect_s(book, 17, 0, 4, 11, 0xff3b30, 1);
    f1_book_rect_s(book, 18, 1, 2, 8, 0xff6b63, 0);
    f1_book_rect_s(book, 17, 10, 2, 2, 0xff3b30, 0);
    f1_book_rect_s(book, 19, 10, 2, 2, 0xff3b30, 0);
    f1_book_rect_s(book, 18, 12, 2, 2, 0xe0352b, 0);
  }
  lv_obj_clear_flag(book, LV_OBJ_FLAG_HIDDEN);
  lv_obj_center(book);
  lv_obj_set_style_translate_x(book, -1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_move_foreground(book);
}

static lv_obj_t *f1_lang_flag_ir = NULL;
static lv_obj_t *f1_lang_flag_uk = NULL;

void f1_lang_btn_sync() {
  if (!f1_lang_btn) return;
  const bool en = (GUI.system_language == 2);
  if (f1_lang_flag_ir) {
    if (en) lv_obj_add_flag(f1_lang_flag_ir, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(f1_lang_flag_ir, LV_OBJ_FLAG_HIDDEN);
  }
  if (f1_lang_flag_uk) {
    if (en) lv_obj_clear_flag(f1_lang_flag_uk, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(f1_lang_flag_uk, LV_OBJ_FLAG_HIDDEN);
  }
}

static void f1_hdr_set_lang_flags(lv_obj_t *btn) {
  if (!btn) return;
  f1_hdr_hide_text_kids(btn);
  lv_obj_set_style_clip_corner(btn, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  // اگر پرچم قدیمی/کوچک مانده، دوباره بساز (~۲۹×۱۹، وسط کادر)
  if (f1_lang_flag_ir && (lv_obj_get_parent(f1_lang_flag_ir) != btn ||
                          lv_obj_get_width(f1_lang_flag_ir) != 29)) {
    lv_obj_delete(f1_lang_flag_ir);
    f1_lang_flag_ir = NULL;
  }
  if (f1_lang_flag_uk && (lv_obj_get_parent(f1_lang_flag_uk) != btn ||
                          lv_obj_get_width(f1_lang_flag_uk) != 29)) {
    lv_obj_delete(f1_lang_flag_uk);
    f1_lang_flag_uk = NULL;
  }
  if (!f1_lang_flag_ir) {
    f1_lang_flag_ir = f1_mk_mini_flag(btn, 0);
    lv_obj_center(f1_lang_flag_ir);
  }
  if (!f1_lang_flag_uk) {
    f1_lang_flag_uk = f1_mk_mini_flag(btn, 1);
    lv_obj_center(f1_lang_flag_uk);
  }
  if (f1_lang_flag_ir) { lv_obj_center(f1_lang_flag_ir); lv_obj_move_foreground(f1_lang_flag_ir); }
  if (f1_lang_flag_uk) { lv_obj_center(f1_lang_flag_uk); lv_obj_move_foreground(f1_lang_flag_uk); }
  f1_lang_btn_sync();
}

// تمام فرزندان را غیرقابل‌کلیک کن تا لمس به ردیف والد برسد
static void f1_make_children_pass_clicks(lv_obj_t *parent) {
  if (!parent) return;
  uint32_t n = lv_obj_get_child_count(parent);
  for (uint32_t i = 0; i < n; i++) {
    lv_obj_t *ch = lv_obj_get_child(parent, i);
    if (!ch) continue;
    lv_obj_clear_flag(ch, LV_OBJ_FLAG_CLICKABLE);
    f1_make_children_pass_clicks(ch);
  }
}

// لیست زبان EEZ را زیر دکمهٔ پرچم هدر می‌آورد
static void f1_place_lang_dropdown() {
  if (!objects.roller2_1) return;
  lv_obj_set_parent(objects.roller2_1, lv_layer_top());
  lv_obj_clear_flag(objects.roller2_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_pos(objects.roller2_1, F1_LANG_X + F1_HDR_W - 120, F1_HDR_Y + F1_HDR_H + 2);
  lv_obj_set_size(objects.roller2_1, 120, 100);
  lv_obj_set_style_bg_opa(objects.roller2_1, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_t *compact = lv_obj_get_child(objects.roller2_1, 0);
  if (compact) lv_obj_add_flag(compact, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *panel = lv_obj_get_child(objects.roller2_1, 1);
  if (panel) {
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, 120, 92);
    // ردیف‌ها: کل سطح قابل لمس (نه فقط دایرهٔ رادیو)
    lv_obj_t *row_fa = lv_obj_get_child(panel, 0);
    lv_obj_t *row_en = lv_obj_get_child(panel, 2);
    if (row_fa) { lv_obj_add_flag(row_fa, LV_OBJ_FLAG_CLICKABLE); f1_make_children_pass_clicks(row_fa); }
    if (row_en) { lv_obj_add_flag(row_en, LV_OBJ_FLAG_CLICKABLE); f1_make_children_pass_clicks(row_en); }
  }
  lv_obj_move_foreground(objects.roller2_1);
}

static lv_obj_t *f1_help_dlg = NULL;
// 0=هیچ، 1=تب/settle، 2=نمایش راهنما — جلوگیری از باز شدن نامپد روی راهنما
static uint8_t pending_f1_help_phase = 0;
static void f1_help_show();
static void ui_resume_deferred_help_tick();  // بعد از تعریف list_help

bool f1_help_blocks_numpad() {
  if (pending_f1_help_phase != 0) return true;
  return f1_help_dlg && lv_obj_is_valid(f1_help_dlg) &&
         !lv_obj_has_flag(f1_help_dlg, LV_OBJ_FLAG_HIDDEN);
}

static void f1_help_request_deferred() {
  pending_f1_help_phase = 1;
  GUI.pending_f1_numpad_open = false;
  if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
  if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
}

void f1_help_deferred_tick() {
  ui_resume_deferred_help_tick();
  if (pending_f1_help_phase == 1) {
    pending_f1_help_phase = 2;
    GUI.pending_f1_numpad_open = false;
    if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
    if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (pending_f1_help_phase == 2) {
    pending_f1_help_phase = 0;
    GUI.pending_f1_numpad_open = false;
    numpad_close();
    f1_help_show();
    // در بازیابی هنگ، پوشش فقط بعد از ساخته‌شدن خود راهنما کنار برود.
    splash_release_hold_cover();
  }
}

static void f1_help_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  pending_f1_help_phase = 0;
  if (f1_help_dlg) {
    lv_obj_delete(f1_help_dlg);
    f1_help_dlg = NULL;
  }
  GUI.lock_screen = false;
  ui_resume_checkpoint();
}

// تیتر جمله: آبی Primary؛ سایز کوچک‌تر از naz24 قبلی (naz20 / m16)
static void f1_help_add_item(lv_obj_t *box, bool en, const char *head, const char *desc) {
  if (!box || !head || !desc) return;
  lv_obj_t *h = lv_label_create(box);
  lv_obj_set_width(h, 410);
  lv_obj_set_style_text_font(h, en ? &lv_font_montserrat_16 : &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(h, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(h, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(h, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(h, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (!en) lv_obj_set_style_text_line_space(h, -4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
  lv_label_set_text(h, head);

  lv_obj_t *d = lv_label_create(box);
  lv_obj_set_width(d, 410);
  lv_obj_set_style_text_font(d, en ? &lv_font_montserrat_14 : &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(d, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(d, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(d, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(d, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (!en) lv_obj_set_style_text_line_space(d, -4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(d, LV_LABEL_LONG_WRAP);
  // فونت فارسی حروف لاتینِ نام تراشه را ندارد؛ نام سخت‌افزار را با شرح فارسیِ خوانا جایگزین کن.
  const char *shown_desc = desc;
  if (!en && strstr(desc, "DS3231")) shown_desc = "دمای تراشه ساعت داخلی";
  lv_label_set_text(d, shown_desc);
}

static void f1_help_show() {
  if (f1_help_dlg) {
    lv_obj_delete(f1_help_dlg);
    f1_help_dlg = NULL;
  }
  // کیبورد مجازی روی راهنما نماند
  if (numpad_cont) lv_obj_add_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
  if (f1_ok_overlay) lv_obj_add_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
  GUI.pending_f1_numpad_open = false;

  int tab = 0;
  if (objects.tabview2_1) tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
  if (tab < 0) tab = 0;
  if (tab > 4) tab = 4;

  const bool en = (GUI.system_language == 2);

  f1_help_dlg = lv_obj_create(lv_layer_top());
  lv_obj_set_size(f1_help_dlg, 448, 292);
  lv_obj_align(f1_help_dlg, LV_ALIGN_CENTER, 0, 4);
  lv_obj_set_style_bg_color(f1_help_dlg, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_help_dlg, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(f1_help_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_help_dlg, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(f1_help_dlg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_opa(f1_help_dlg, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(f1_help_dlg, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *close = lv_button_create(f1_help_dlg);
  lv_obj_set_size(close, 36, 36);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xff3b30), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(close, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, f1_help_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cx = lv_label_create(close);
  lv_label_set_text(cx, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cx, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cx);

  // عنوان بدون خط تیره بلند (—) که در فونت فارسی مربع می‌شود
  static const char *fa_title[] = {
    "راهنما: تنظیمات عمومی",
    "راهنما: تنظیمات موتور",
    "راهنما: تست دستی",
    "راهنما: تنظیمات مدیریت",
    "راهنما: توسعه دهنده"
  };
  static const char *en_title[] = {
    "Help: General",
    "Help: Motor",
    "Help: Manual Test",
    "Help: Admin",
    "Help: Developer"
  };

  {
    lv_obj_t *t = lv_label_create(f1_help_dlg);
    lv_obj_set_style_text_color(t, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (en) {
      lv_obj_set_style_text_font(t, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(t, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(t, en_title[tab]);
      lv_obj_align(t, LV_ALIGN_TOP_LEFT, 42, 4);
    } else {
      lv_obj_set_style_text_font(t, &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(t, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(t, fa_title[tab]);
      lv_obj_align(t, LV_ALIGN_TOP_RIGHT, -2, -4);
    }
  }

  // بدنه اسکرول‌پذیر — تیتر جدا از توضیح
  lv_obj_t *body_box = lv_obj_create(f1_help_dlg);
  lv_obj_set_pos(body_box, 0, 38);
  lv_obj_set_size(body_box, 432, 240);
  lv_obj_set_style_bg_opa(body_box, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(body_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(body_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(body_box, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(body_box, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scroll_dir(body_box, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(body_box, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_add_flag(body_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(body_box, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_flex_flow(body_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(body_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  if (tab == 0) {
    if (en) {
      f1_help_add_item(body_box, en, "Home", "Return to the main machine operation screen");
      f1_help_add_item(body_box, en, "Help", "Comprehensive guide for all system settings and tools");
      f1_help_add_item(body_box, en, "Language", "Switch UI language between Persian and English");
      f1_help_add_item(body_box, en, "Save", "Commit changes to non-volatile memory (green = pending changes)");
      f1_help_add_item(body_box, en, "Wi-Fi", "Toggle Direct Hotspot / Router mode for web app & wireless OTA update");
      f1_help_add_item(body_box, en, "Keyboard", "Enable/disable virtual on-screen touch keyboard");
      f1_help_add_item(body_box, en, "Buzzer", "Enable or mute acoustic feedback with multi-tone frequencies");
      f1_help_add_item(body_box, en, "Printer", "Auto-print profile labels after cut (hold to select printer model)");
      f1_help_add_item(body_box, en, "Factory Reset", "Restore all device parameters to factory default values");
      f1_help_add_item(body_box, en, "Auto Cut", "Configure automatic cutting cycles and stock optimization");
      f1_help_add_item(body_box, en, "Stock Length", "Raw stock bar length for scrap and pattern calculation");
      f1_help_add_item(body_box, en, "Blade Kerf", "Saw blade thickness compensation for exact cut dimensions");
    } else {
      f1_help_add_item(body_box, en, "خانه", "بازگشت به صفحه اصلی کارکرد دستگاه");
      f1_help_add_item(body_box, en, "راهنما", "راهنمای جامع و فنی تنظیمات و ابزارهای دستگاه");
      f1_help_add_item(body_box, en, "زبان", "تعویض زبان رابط کاربری بین فارسی و انگلیسی");
      f1_help_add_item(body_box, en, "ذخیره", "ثبت دائمی تغییرات در حافظه دستگاه (رنگ سبز = تغییرات ذخیره نشده)");
      f1_help_add_item(body_box, en, "وای فای", "روشن کردن هات اسپات مستقیم یا شبکه روتر جهت وب اپ و اپدیت بیسیم");
      f1_help_add_item(body_box, en, "کیبورد", "فعال یا غیرفعال سازی کیبورد لمسی هوشمند روی نمایشگر");
      f1_help_add_item(body_box, en, "بازر صوتی", "فعال یا بیصدا کردن بوق سیستم با تن ها و ملودی های تفکیک شده");
      f1_help_add_item(body_box, en, "پرینتر حرارتی", "چاپ خودکار برچسب ابعاد با هر برش (نگه داشتن برای انتخاب مدل پرینتر)");
      f1_help_add_item(body_box, en, "بازنشانی کارخانه", "بازگردانی تمامی پارامترها و کالیبراسیون به مقادیر پیش فرض کارخانه");
      f1_help_add_item(body_box, en, "برش اتوماتیک", "تنظیمات سیکل برش خودکار و بهینه سازی شاخه");
      f1_help_add_item(body_box, en, "طول شاخه", "متراژ شاخه خام ورودی جهت محاسبه دقیق پرت و الگو");
      f1_help_add_item(body_box, en, "عرض تیغه", "ضخامت براده برداری تیغ اره برای جبران دقیق طول برش");
    }
  } else if (tab == 1) {
    if (en) {
      f1_help_add_item(body_box, en, "Speed", "Cutting and automatic move speed");
      f1_help_add_item(body_box, en, "Init speed", "Motor acceleration ramp");
      f1_help_add_item(body_box, en, "Manual jog init", "Soft start and stop for jog");
      f1_help_add_item(body_box, en, "Start size", "Initial length box");
      f1_help_add_item(body_box, en, "End size", "Axis max box");
      f1_help_add_item(body_box, en, "Save", "Store motor settings");
    } else {
      f1_help_add_item(body_box, en, "سرعت", "سرعت حرکت برش و اتومات");
      f1_help_add_item(body_box, en, "سرعت راه اندازی", "شیب زمانی صفر تا سرعت نامی (0%=انی، 100%=5ثانیه، نمایی)");
      f1_help_add_item(body_box, en, "سرعت راه اندازی حرکت دستی", "همان شیب برای جاگ");
      f1_help_add_item(body_box, en, "سایز اول خط", "طول اولیه کادر آبی");
      f1_help_add_item(body_box, en, "سایز آخر خط", "حداکثر محور کادر آبی");
      f1_help_add_item(body_box, en, "ذخیره", "ثبت تغییرات موتور در حافظه");
    }
  } else if (tab == 2) {
    if (en) {
      f1_help_add_item(body_box, en, "Outputs", "Manual test of clamps spray saw lamp lift");
      f1_help_add_item(body_box, en, "LEDs", "Live status of sensors and keys");
      f1_help_add_item(body_box, en, "Caution", "For diagnostics only not production");
      f1_help_add_item(body_box, en, "Leave tab", "Leaving turns all test outputs OFF");
    } else {
      f1_help_add_item(body_box, en, "خروجی ها", "تست دستی گیره اسپری اره چراغ بالابر");
      f1_help_add_item(body_box, en, "ال ای دی ها", "وضعیت زنده سنسورها و کلیدها");
      f1_help_add_item(body_box, en, "احتیاط", "فقط برای تست در تولید استفاده نکنید");
      f1_help_add_item(body_box, en, "خروج از تب", "با ترک این تب همه خروجی های تست خاموش می شوند");
    }
  } else if (tab == 3) {
    if (en) {
      f1_help_add_item(body_box, en, "Saw timer", "How long the saw stays on");
      f1_help_add_item(body_box, en, "Base direction", "Axis base direction");
      f1_help_add_item(body_box, en, "Language", "UI language from header");
      f1_help_add_item(body_box, en, "Save", "Apply and store admin settings");
    } else {
      f1_help_add_item(body_box, en, "تایمر اره", "مدت روشن بودن اره در سیکل برش");
      f1_help_add_item(body_box, en, "جهت پایه", "جهت مرجع محور راست یا چپ");
      f1_help_add_item(body_box, en, "زبان", "زبان رابط از هدر یا این صفحه");
      f1_help_add_item(body_box, en, "ذخیره", "اعمال و ذخیره تنظیمات مدیریت");
    }
  } else {
    if (en) {
      f1_help_add_item(body_box, en, "Cut count", "Tap to reset the shift cut counter after confirm");
      f1_help_add_item(body_box, en, "Total cuts", "Lifetime cut count. Cannot be reset here");
      f1_help_add_item(body_box, en, "Firmware", "Installed software version");
      f1_help_add_item(body_box, en, "Maker", "Device manufacturer name");
      f1_help_add_item(body_box, en, "System clock", "Live clock from admin settings. Set it on the admin page first");
      f1_help_add_item(body_box, en, "CPU temp", "ESP32 processor temperature");
      f1_help_add_item(body_box, en, "Build date", "Date and time this firmware was built");
      f1_help_add_item(body_box, en, "Free RAM", "Free internal memory and PSRAM");
      f1_help_add_item(body_box, en, "Update", "Blue bar inside the white card. Starts Wi-Fi firmware update");
      f1_help_add_item(body_box, en, "Warranty", "Activate once. Saves that day forever and the button disappears");
      f1_help_add_item(body_box, en, "Caution", "Developer changes need care");
    } else {
      f1_help_add_item(body_box, en, "تعداد برش", "با لمس و تایید شمارنده شیفت صفر می شود");
      f1_help_add_item(body_box, en, "تعداد برش کل", "شمارنده عمری. اینجا صفر نمی شود");
      f1_help_add_item(body_box, en, "نسخه برنامه", "نسخه نرم افزار نصب شده روی دستگاه");
      f1_help_add_item(body_box, en, "سازنده", "نام سازنده دستگاه");
      f1_help_add_item(body_box, en, "ساعت و تاریخ سیستم", "ساعت زنده از تنظیمات مدیریت. اول آنجا ساعت را تنظیم کنید");
      f1_help_add_item(body_box, en, "دما پردازنده", "دمای پردازنده دستگاه");
      f1_help_add_item(body_box, en, "تاریخ برنامه", "تاریخ ساخت نسخه فعلی؛ لمس برای دیدن سوابق نصب روی دستگاه");
      f1_help_add_item(body_box, en, "دما داخلی", "دمای تراشه ساعت DS3231");
      f1_help_add_item(body_box, en, "بروزرسانی نرم افزار", "نوار آبی داخل کادر سفید. شروع به روزرسانی از وای فای");
      f1_help_add_item(body_box, en, "فعال سازی گارانتی", "یک بار بزنید. تاریخ همان روز ذخیره می شود و کلید دیگر نمی آید");
      f1_help_add_item(body_box, en, "احتیاط", "تغییرات این صفحه فقط با آگاهی فنی");
    }
  }

  if (en) {
    f1_help_add_item(body_box, en, "Five panel keys",
                     "Mode moves the marker. Start confirms or increases a slider. Stop decreases a slider; elsewhere it explains the focused item.");
    f1_help_add_item(body_box, en, "Cancel",
                     "Returns one level from a row or keypad. At the page level it closes the marker.");
    f1_help_add_item(body_box, en, "Panel Save",
                     "Stores pending changes and briefly shows Saved.");
    f1_help_add_item(body_box, en, "Marker timeout",
                     "The marker closes after ten seconds without a panel-key action.");
  } else {
    f1_help_add_item(body_box, en, "پنج کلید پنل",
                     "مد نشانگر را جابه جا می کند. استارت تایید می کند یا اسلایدر را زیاد می کند. استپ اسلایدر را کم می کند و در بقیه قسمت ها توضیح همان قسمت را نشان می دهد.");
    f1_help_add_item(body_box, en, "کنسل",
                     "از ردیف یا کیبورد یک مرحله عقب می آید. در سطح صفحه نشانگر را می بندد.");
    f1_help_add_item(body_box, en, "ذخیره پنل",
                     "تغییرات انجام شده را ثبت می کند و پیام ذخیره شد می آید.");
    f1_help_add_item(body_box, en, "مهلت نشانگر",
                     "اگر ده ثانیه هیچ کلید پنل زده نشود نشانگر بسته می شود.");
  }

  lv_obj_clear_flag(f1_help_dlg, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f1_help_dlg);
  GUI.lock_screen = true;
  ui_resume_checkpoint();
}

// عنوان هدر مشترک F1..F5 مطابق تب فعال
static void f1_header_title_sync() {
  if (!f1_title_bar || lv_obj_get_child_count(f1_title_bar) == 0) return;
  lv_obj_t *tl = lv_obj_get_child(f1_title_bar, 0);
  if (!tl) return;
  const bool en = (GUI.system_language == 2);
  int tab = 0;
  if (objects.tabview2_1) tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
  const char *fa[] = {
    "تنظیمات عمومی",
    "تنظیمات موتور",
    "تست دستی المان ها",
    "تنظیمات مدیریت",
    "تنظیمات توسعه دهنده"
  };
  const char *eng[] = {
    "General Settings",
    "Motor Settings",
    "Manual Test",
    "Admin Settings",
    "Developer"
  };
  if (tab < 0) tab = 0;
  if (tab > 4) tab = 4;
  if (en) {
    // سایز انگلیسی ≈ فارسی هر تب (F5=naz20 → montserrat_20)
    const lv_font_t *f = (tab == 2 || tab == 4) ? &lv_font_montserrat_20 : &lv_font_montserrat_22;
    lv_obj_set_style_text_font(tl, f, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(tl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(tl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(tl, eng[tab]);
  } else {
    // عنوان‌های بلند کمی کوچک‌تر تا در کادر جا شوند
    const lv_font_t *f = (tab == 4) ? &ui_font_naz20b :
                         (tab == 2) ? &ui_font_naz24 : &ui_font_naz30;
    lv_obj_set_style_text_font(tl, f, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(tl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(tl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(tl, fa[tab]);
  }
  lv_obj_center(tl);
}

// لی‌اوت F2: اول/آخر خط به‌صورت ورودی + اسلایدرها + جوگ
static lv_obj_t *f2_mm_start = NULL;
static lv_obj_t *f2_mm_end = NULL;
static lv_obj_t *slider_jog = NULL;
static lv_obj_t *lbl_spd_jog = NULL;
static lv_obj_t *lbl_fa_jog = NULL;
static lv_obj_t *lbl_en_jog = NULL;
static lv_obj_t *f2_edit_caret = NULL;
static uint32_t f2_caret_blink_ms = 0;
static bool f2_caret_lit = true;
static int f2_edit_ctx = 0;  // 1=اول خط  2=آخر خط

static void f2_caret_hide() {
  if (f2_edit_caret) lv_obj_add_flag(f2_edit_caret, LV_OBJ_FLAG_HIDDEN);
  f2_edit_ctx = 0;
}

static void f2_caret_refresh() {
  if (f2_edit_ctx != 1 && f2_edit_ctx != 2) { f2_caret_hide(); return; }
  lv_obj_t *box = (f2_edit_ctx == 1) ? objects.obj9 : objects.obj10;
  lv_obj_t *num = (f2_edit_ctx == 1) ? objects.lable2_2 : objects.lable2_3;
  if (!box || !num) { f2_caret_hide(); return; }
  if (!f2_edit_caret) {
    f2_edit_caret = lv_obj_create(box);
    lv_obj_set_size(f2_edit_caret, 2, 28);
    lv_obj_clear_flag(f2_edit_caret, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_set_style_bg_color(f2_edit_caret, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f2_edit_caret, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f2_edit_caret, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(f2_edit_caret, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_obj_set_parent(f2_edit_caret, box);
  // دقیقاً بعد از آخرین رقم (نه لبهٔ راست ویجت خالی)
  const char *txt = lv_label_get_text(num);
  if (!txt) txt = "";
  lv_point_t letter_pos = {0, 0};
  lv_label_get_letter_pos(num, (uint32_t)strlen(txt), &letter_pos);
  int nx = lv_obj_get_x(num);
  int ny = lv_obj_get_y(num);
  int caret_h = 22;
  lv_obj_set_size(f2_edit_caret, 2, caret_h);
  // montserrat_22 داخل کادر؛ caret هم‌ارتفاع متن
  lv_obj_set_pos(f2_edit_caret, nx + letter_pos.x + 1, ny + 4);
  lv_obj_set_style_bg_opa(f2_edit_caret, f2_caret_lit ? LV_OPA_COVER : LV_OPA_TRANSP,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(f2_edit_caret, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f2_edit_caret);
}

void f2_caret_tick() {
  if (GUI.scr_id != 2 || (f2_edit_ctx != 1 && f2_edit_ctx != 2)) {
    if (f2_edit_caret && !lv_obj_has_flag(f2_edit_caret, LV_OBJ_FLAG_HIDDEN))
      f2_caret_hide();
    return;
  }
  uint32_t now = millis();
  if ((int32_t)(now - f2_caret_blink_ms) >= 450) {
    f2_caret_blink_ms = now;
    f2_caret_lit = !f2_caret_lit;
  }
  f2_caret_refresh();
}

static float f2_ppmm() {
  float p = (float)STP.drv_ppr / (float)STP.Axis_Pitch;
  return (p > 0.01f) ? p : 1.0f;
}

static float f2_start_mm_from_pulses() {
  return (float)STP.initial_Position_p / f2_ppmm();
}

static void f2_format_size_buf(String &buf, float v, bool keep_frac) {
  char b[16];
  if (keep_frac || fabsf(v - roundf(v)) > 0.05f)
    snprintf(b, sizeof(b), "%.1f", (double)v);
  else
    snprintf(b, sizeof(b), "%d", (int)lroundf(v));
  buf = b;
}

static void f2_load_start_buf_from_pulses() {
  float v = f2_start_mm_from_pulses();
  f2_format_size_buf(GUI.inputBuffer_initial_L2, v, fabsf(v - roundf(v)) > 0.05f);
  GUI.L2_GUI = (int)lroundf(v);
}

void f2_begin_edit_start_line() {
  if (GUI.enable_edit_lable2_5) f2_commit_end_value();
  // گارد: لمس مجدد همین کادر در حال تایپ → بافر را نریز
  const bool was_editing = GUI.enable_edit_lable2_3 && !GUI.clear_buffer_aut_flag;
  GUI.enable_edit_lable2_3 = true;
  GUI.enable_edit_lable2_5 = false;
  if (!was_editing) {
    f2_load_start_buf_from_pulses();
    GUI.clear_buffer_aut_flag = true;
  }
  GUI.flag_update_lables_2 = true;
  if (objects.lable2_2) {
    lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
    lv_obj_set_style_text_color(objects.lable2_2, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  f2_edit_ctx = 1;
  f2_caret_lit = true;
  f2_caret_blink_ms = millis();
  f2_caret_refresh();
}

void f2_begin_edit_end_line() {
  if (GUI.enable_edit_lable2_3) f2_commit_start_value();
  // گارد: لمس مجدد همین کادر در حال تایپ → بافر را نریز
  const bool was_editing = GUI.enable_edit_lable2_5 && !GUI.clear_buffer_aut_flag;
  GUI.enable_edit_lable2_5 = true;
  GUI.enable_edit_lable2_3 = false;
  if (!was_editing) {
    GUI.inputBuffer_max_L = STP.final_position_mm;
    GUI.clear_buffer_aut_flag = true;
  }
  GUI.flag_update_lable2_5 = true;
  if (objects.lable2_3) {
    lv_label_set_text(objects.lable2_3, GUI.inputBuffer_max_L.c_str());
    lv_obj_set_style_text_color(objects.lable2_3, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  f2_edit_ctx = 2;
  f2_caret_lit = true;
  f2_caret_blink_ms = millis();
  f2_caret_refresh();
}

void f2_on_digit_typed() {
  f2_caret_lit = true;
  f2_caret_blink_ms = millis();
  f2_caret_refresh();
}

// F2 اول/آخر خط (+ کالیبر مرحله ۴): ۴ رقم + حداکثر یک رقم اعشار؛ بدون پرش به کلید دیگر
bool f2_try_append_char(char d) {
  if (GUI.scr_id != 2) return false;
  const bool calib4 = STP.begin_calibration && STP.calib_stage == 4;
  if (!calib4 && !(GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5)) return false;
  if (d == ',') d = '.';
  String *buf = (!calib4 && GUI.enable_edit_lable2_5) ? &GUI.inputBuffer_max_L : &GUI.inputBuffer_initial_L2;
  if (GUI.clear_buffer_aut_flag) {
    *buf = "";
    GUI.clear_buffer_aut_flag = false;
  }
  if (!append_main_target_char(*buf, d)) {
    GUI.buzzer_cmd_long = true;
    return false;
  }
  if (!calib4 && GUI.enable_edit_lable2_5) {
    GUI.flag_update_lable2_5 = true;
    if (objects.lable2_3) lv_label_set_text(objects.lable2_3, buf->c_str());
  } else {
    GUI.flag_update_lables_2 = true;
    if (objects.lable2_2) lv_label_set_text(objects.lable2_2, buf->c_str());
  }
  f2_on_digit_typed();
  return true;
}

void f2_on_backspace() {
  // ریشهٔ «پاک نمی‌شود»: بعضی مسیرها فقط caret را تازه می‌کردند و کاراکتر را حذف نمی‌کردند
  GUI.clear_buffer_aut_flag = false;
  const bool calib4 = STP.begin_calibration && STP.calib_stage == 4;
  String *buf = nullptr;
  lv_obj_t *lab = nullptr;
  if (!calib4 && GUI.enable_edit_lable2_5) {
    buf = &GUI.inputBuffer_max_L;
    lab = objects.lable2_3;
    GUI.flag_update_lable2_5 = true;
  } else if (calib4 || GUI.enable_edit_lable2_3) {
    buf = &GUI.inputBuffer_initial_L2;
    lab = objects.lable2_2;
    GUI.flag_update_lables_2 = true;
  }
  if (buf && buf->length()) buf->remove(buf->length() - 1);
  if (lab) lv_label_set_text(lab, buf ? buf->c_str() : "");
  f2_caret_lit = true;
  f2_caret_blink_ms = millis();
  f2_caret_refresh();
}

void f2_end_edit() {
  f2_caret_hide();
  GUI.enable_edit_lable2_3 = false;
  GUI.enable_edit_lable2_5 = false;
}

void f2_commit_start_value() {
  float endv = STP.final_position_mm.toFloat();
  float vf = GUI.inputBuffer_initial_L2.toFloat();
  float mx = 9999.0f;
  if (endv > 0.0f && endv < mx) mx = endv;
  if (mx < 30.0f) mx = 30.0f;
  if (vf < 30.0f) vf = 30.0f;
  if (vf > mx) vf = mx;
  const bool keep_frac = GUI.inputBuffer_initial_L2.indexOf('.') >= 0;
  f2_format_size_buf(GUI.inputBuffer_initial_L2, vf, keep_frac);
  vf = GUI.inputBuffer_initial_L2.toFloat();
  GUI.L2_GUI = (int)lroundf(vf);
  STP.initial_Position_p = (int)lroundf(f2_ppmm() * vf);
  settings_mark_dirty();
  if (objects.lable2_2) {
    lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
    lv_obj_set_style_text_color(objects.lable2_2, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  GUI.enable_edit_lable2_3 = false;
  f2_caret_hide();
  // فقط همان کادر را commit کن — پرش خودکار به کادر بعدی باعث قفل کیبورد می‌شد
}

void f2_commit_end_value() {
  float startv = f2_start_mm_from_pulses();
  if (GUI.inputBuffer_initial_L2.length()) startv = GUI.inputBuffer_initial_L2.toFloat();
  float vf = GUI.inputBuffer_max_L.toFloat();
  float mn = 100.0f;
  if (startv > mn) mn = startv;
  if (vf < mn) vf = mn;
  if (vf > 9999.0f) vf = 9999.0f;
  const bool keep_frac = GUI.inputBuffer_max_L.indexOf('.') >= 0;
  f2_format_size_buf(GUI.inputBuffer_max_L, vf, keep_frac);
  vf = GUI.inputBuffer_max_L.toFloat();
  STP.final_position_mm = GUI.inputBuffer_max_L;
  STP.final_position_p = (int)lroundf(vf * f2_ppmm());
  settings_mark_dirty();
  GUI.flag_update_lable2_5 = true;
  if (objects.lable2_3) {
    lv_label_set_text(objects.lable2_3, STP.final_position_mm.c_str());
    lv_obj_set_style_text_color(objects.lable2_3, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.lable2_5) lv_label_set_text(objects.lable2_5, STP.final_position_mm.c_str());
  GUI.enable_edit_lable2_5 = false;
  f2_caret_hide();
  GUI.clear_buffer_aut_flag = true;
}

int f2_read_jog_ramp_ms() {
  // اسلایدر ۰٪ = بدون شیب؛ ۱۰۰٪ = ۵۰۰۰ms
  if (!slider_jog) return jog_ramp_ms;
  int v = (int)lv_slider_get_value(slider_jog);
  return (int)map(constrain(v, 0, 1000), 0, 1000, 0, 5000);
}

void f2_apply_jog_slider_from_value(int ramp_ms) {
  jog_ramp_ms = constrain(ramp_ms, 0, 5000);
  jog_init_speed = (jog_ramp_ms > 0) ? 400 : 0;
  if (slider_jog)
    lv_slider_set_value(slider_jog, (int)map(jog_ramp_ms, 0, 5000, 0, 1000), LV_ANIM_OFF);
  if (lbl_spd_jog && slider_jog) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider_jog) / 10);
    lv_label_set_text(lbl_spd_jog, buf);
  }
}

static void f2_style_line_input_box(lv_obj_t *box, bool sensor_hit) {
  if (!box) return;
  const uint32_t blue = 0x0a84ffu;
  const uint32_t green = 0x34C759u;
  const uint32_t red = 0xff3b30u;
  const bool oobe_req = !g_oobe_done && (g_oobe_stage == OOBE_STAGE_F2_LIMITS);
  const uint32_t c = sensor_hit ? green : (oobe_req ? red : blue);
  lv_obj_set_style_bg_color(box, lv_color_hex(0xf2f2f7), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(box, lv_color_hex(c), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void f2_sync_line_box_colors() {
  if (GUI.scr_id != 2) return;
  // سنسور اول خط = ref ، سنسور/کلید آخر خط = up_lim
  f2_style_line_input_box(objects.obj9, IOS.ref_sensor_state);
  f2_style_line_input_box(objects.obj10, IOS.up_lim_sensor_state);
}

static void f2_box_start_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen || STP.begin_calibration || GUI.cast_message) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    return;
  }
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  if (GUI.enable_edit_lable2_3 && !GUI.enable_edit_lable2_5) {
    f2_caret_lit = true;
    f2_caret_blink_ms = millis();
    f2_caret_refresh();
    return;
  }
  f2_begin_edit_start_line();
}
static void f2_box_end_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen || STP.begin_calibration || GUI.cast_message) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    return;
  }
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  if (GUI.enable_edit_lable2_5 && !GUI.enable_edit_lable2_3) {
    f2_caret_lit = true;
    f2_caret_blink_ms = millis();
    f2_caret_refresh();
    return;
  }
  f2_begin_edit_end_line();
}

static void f2_wire_input_box_events() {
  static bool wired = false;
  if (wired) return;
  if (objects.obj9) {
    lv_obj_add_flag(objects.obj9, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.obj9, f2_box_start_cb, LV_EVENT_RELEASED, NULL);
  }
  if (objects.obj10) {
    lv_obj_add_flag(objects.obj10, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.obj10, f2_box_end_cb, LV_EVENT_RELEASED, NULL);
  }
  wired = true;
}

static void f2_make_side_label_not_button(lv_obj_t *btn) {
  if (!btn) return;
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void f2_find_mm_labels() {
  if (objects.obj9 && !f2_mm_start) {
    uint32_t n = lv_obj_get_child_count(objects.obj9);
    for (uint32_t i = 0; i < n; i++) {
      lv_obj_t *ch = lv_obj_get_child(objects.obj9, i);
      if (ch && ch != objects.lable2_2) { f2_mm_start = ch; break; }
    }
  }
  if (objects.obj10 && !f2_mm_end) {
    uint32_t n = lv_obj_get_child_count(objects.obj10);
    for (uint32_t i = 0; i < n; i++) {
      lv_obj_t *ch = lv_obj_get_child(objects.obj10, i);
      if (ch && ch != objects.lable2_3) { f2_mm_end = ch; break; }
    }
  }
}

static void f2_jog_slider_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  if (!slider_jog || !lbl_spd_jog) return;
  char buf[8];
  int v = (int)lv_slider_get_value(slider_jog);
  snprintf(buf, sizeof(buf), "%d%%", v / 10);
  lv_label_set_text(lbl_spd_jog, buf);
  jog_ramp_ms = (int)map(constrain(v, 0, 1000), 0, 1000, 0, 5000);
  jog_init_speed = (jog_ramp_ms > 0) ? 400 : 0;
  settings_mark_dirty();
}

static void f2_ensure_jog_slider() {
  if (!objects.obj7 || slider_jog) return;
  const bool en = (GUI.system_language == 2);
  slider_jog = lv_slider_create(objects.obj7);
  lv_obj_set_size(slider_jog, 360, 12);
  lv_obj_set_style_pad_ver(slider_jog, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_width(slider_jog, 14, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_height(slider_jog, 14, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_slider_set_range(slider_jog, 0, 1000);
  f2_apply_jog_slider_from_value(jog_ramp_ms);
  lv_obj_add_event_cb(slider_jog, f2_jog_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lbl_spd_jog = lv_label_create(objects.obj7);
  lv_obj_set_style_text_font(lbl_spd_jog, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(lbl_spd_jog, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  f2_apply_jog_slider_from_value(jog_ramp_ms);

  lbl_fa_jog = lv_label_create(objects.obj7);
  lv_obj_set_style_text_font(lbl_fa_jog, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_fa_jog, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(lbl_fa_jog, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_fa_jog, "سرعت راه اندازی دستی");

  lbl_en_jog = lv_label_create(objects.obj7);
  lv_obj_set_style_text_font(lbl_en_jog, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_en_jog, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_en_jog, "Manual Jog Ramp");
  if (en) {
    lv_obj_add_flag(lbl_fa_jog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lbl_en_jog, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(lbl_fa_jog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lbl_en_jog, LV_OBJ_FLAG_HIDDEN);
  }
}

static void f2_style_thin_slider(lv_obj_t *sl) {
  if (!sl) return;
  lv_obj_set_size(sl, 360, 12);
  lv_obj_set_style_pad_ver(sl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_width(sl, 14, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_height(sl, 14, LV_PART_KNOB | LV_STATE_DEFAULT);
}

// عنوان و درصد: عنوان وسط؛ درصد سمت راست (+۲۳px نسبت به قبل)
static void f2_place_spd_row(lv_obj_t *title_fa, lv_obj_t *title_en, lv_obj_t *pct, int title_y, int pct_y) {
  if (title_fa) {
    lv_obj_set_style_text_align(title_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    // تیتر اسلایدر: کوچک‌تر + مشکی
    lv_obj_set_style_text_font(title_fa, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title_fa, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(title_fa, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(title_fa, -6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(title_fa, LV_LABEL_LONG_CLIP);
    lv_obj_align(title_fa, LV_ALIGN_TOP_MID, -8, title_y);  // −۴px چپ (اسلایدر)
  }
  if (title_en) {
    lv_obj_set_style_text_align(title_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title_en, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title_en, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(title_en, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title_en, LV_ALIGN_TOP_MID, -8, title_y + 10);
  }
  if (pct) {
    lv_obj_set_style_text_align(pct, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pct, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(pct, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(pct, LV_ALIGN_TOP_MID, 162, pct_y);  // −۴px چپ
  }
}


static lv_obj_t *f2_line_kb = NULL;
static lv_obj_t *f2_line_kb_del = NULL;
static lv_obj_t *f2_line_kb_ok = NULL;

void f2_hide_line_kb() {
  if (f2_line_kb && !lv_obj_has_flag(f2_line_kb, LV_OBJ_FLAG_HIDDEN))
    lv_obj_add_flag(f2_line_kb, LV_OBJ_FLAG_HIDDEN);
  if (f2_line_kb_del && !lv_obj_has_flag(f2_line_kb_del, LV_OBJ_FLAG_HIDDEN))
    lv_obj_add_flag(f2_line_kb_del, LV_OBJ_FLAG_HIDDEN);
  if (f2_line_kb_ok && !lv_obj_has_flag(f2_line_kb_ok, LV_OBJ_FLAG_HIDDEN))
    lv_obj_add_flag(f2_line_kb_ok, LV_OBJ_FLAG_HIDDEN);
}

static void f2_line_kb_event(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  lv_obj_t *bm = (lv_obj_t *)lv_event_get_target(e);
  uint32_t id = lv_buttonmatrix_get_selected_button(bm);
  if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
  // OK مخفی است — فقط overlay سبز؛ BS هم از overlay قرمز
  if (id == 9 || id == 12) return;
  const char *txt = lv_buttonmatrix_get_button_text(bm, id);
  if (!txt) return;
  if (GUI.lock_screen || STP.begin_calibration) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  if (!(GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5))
    f2_begin_edit_start_line();
  if ((txt[0] >= '0' && txt[0] <= '9' && txt[1] == 0) ||
      (txt[0] == '.' && txt[1] == 0) || (txt[0] == ',' && txt[1] == 0)) {
    f2_try_append_char(txt[0]);
  }
}

static void f2_line_kb_del_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen || STP.begin_calibration) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  if (!(GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5)) f2_begin_edit_start_line();
  f2_on_backspace();
}

static void f2_line_kb_ok_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen || STP.begin_calibration) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  if (GUI.enable_edit_lable2_5) f2_commit_end_value();
  else f2_commit_start_value();
}

static lv_obj_t *f2_make_kb_overlay_btn(lv_obj_t *parent, lv_event_cb_t cb, const char *txt, uint32_t color) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl, txt);
  lv_obj_center(lbl);
  return btn;
}

static void f2_ensure_line_kb() {
  if (!objects.obj7) return;
  // فقط تب F2 + کیبورد نرم‌افزاری
  bool on_f2 = (GUI.scr_id == 2 && objects.tabview2_1 &&
                (int)lv_tabview_get_tab_act(objects.tabview2_1) == 1);
  if (!on_f2 || !GUI.virtual_kb_enable) {
    f2_hide_line_kb();
    return;
  }
  numpad_close();
  f4_kb_hide();
  // ردیف آخر: پاک | 0 | ممیز | تأیید — بدون overlay روی ممیز (ریشهٔ «اعشار نمی‌گیرد»)
  static const char *map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", ".", LV_SYMBOL_OK, ""
  };
  if (!f2_line_kb) {
    f2_line_kb = lv_buttonmatrix_create(objects.obj7);
    lv_buttonmatrix_set_map(f2_line_kb, map);
    lv_obj_add_event_cb(f2_line_kb, f2_line_kb_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_bg_color(f2_line_kb, lv_color_hex(0x1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(f2_line_kb, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f2_line_kb, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(f2_line_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(f2_line_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(f2_line_kb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f2_line_kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(f2_line_kb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(f2_line_kb, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(f2_line_kb, lv_color_hex(0x1c1c1e), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(f2_line_kb, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f2_line_kb, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(f2_line_kb, lv_color_hex(0x1c9b67), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(f2_line_kb, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_CHECKED);
  }
  lv_buttonmatrix_set_map(f2_line_kb, map);
  // مثل لیست برش: OK ماتریس مخفی + دکمهٔ سبز جدا؛ بدون CHECKED (ریشهٔ قفل کیبورد)
  lv_buttonmatrix_clear_button_ctrl_all(f2_line_kb, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_buttonmatrix_set_button_ctrl(f2_line_kb, 9, LV_BUTTONMATRIX_CTRL_HIDDEN);   // BS
  lv_buttonmatrix_set_button_ctrl(f2_line_kb, 12, LV_BUTTONMATRIX_CTRL_HIDDEN);  // OK
  if (!f2_line_kb_del) f2_line_kb_del = f2_make_kb_overlay_btn(objects.obj7, f2_line_kb_del_cb, LV_SYMBOL_BACKSPACE, 0xd92d20);
  if (!f2_line_kb_ok) f2_line_kb_ok = f2_make_kb_overlay_btn(objects.obj7, f2_line_kb_ok_cb, LV_SYMBOL_OK, 0x1c9b67);
  lv_obj_set_pos(f2_line_kb, 225, 188);
  lv_obj_set_size(f2_line_kb, 168, 110);
  lv_obj_clear_flag(f2_line_kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f2_line_kb);
  // خانهٔ BS / OK: pad4 + col ≈ (168-8-9)/4
  lv_obj_set_pos(f2_line_kb_del, 229, 271);
  lv_obj_set_size(f2_line_kb_del, 38, 24);
  lv_obj_clear_flag(f2_line_kb_del, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f2_line_kb_del);
  lv_obj_set_pos(f2_line_kb_ok, 351, 271);
  lv_obj_set_size(f2_line_kb_ok, 38, 24);
  lv_obj_clear_flag(f2_line_kb_ok, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f2_line_kb_ok);
}

static void f2_motor_fields_reflow() {
  const bool en = (GUI.system_language == 2);
  f2_ensure_jog_slider();
  f2_find_mm_labels();

  f2_style_thin_slider(objects.slider2_1);
  f2_style_thin_slider(objects.slider2_2);
  f2_style_thin_slider(slider_jog);
  if (objects.slider2_1) lv_obj_set_pos(objects.slider2_1, 8, 59);
  if (objects.slider2_2) lv_obj_set_pos(objects.slider2_2, 8, 112);
  if (slider_jog) lv_obj_set_pos(slider_jog, 8, 166);

  f2_place_spd_row(objects.lablef2_6, objects.lablee2_6, objects.lbl_spd2_1, 25, 33);
  f2_place_spd_row(objects.lablef2_7, objects.lablee2_7, objects.lbl_spd2_2, 78, 86);
  f2_place_spd_row(lbl_fa_jog, lbl_en_jog, lbl_spd_jog, 132, 140);

  // سه عنوان اسلایدر: naz18b مشکی
  if (objects.lablef2_6) {
    lv_obj_set_style_text_font(objects.lablef2_6, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.lablef2_6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablef2_6, "سرعت");
    if (en) lv_obj_add_flag(objects.lablef2_6, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(objects.lablef2_6, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablee2_6) {
    lv_obj_set_style_text_font(objects.lablee2_6, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.lablee2_6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablee2_6, "Speed");
    if (en) lv_obj_clear_flag(objects.lablee2_6, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(objects.lablee2_6, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablef2_7) {
    lv_obj_set_style_text_font(objects.lablef2_7, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.lablef2_7, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    // «اندازی» با ی فارسی (U+06CC) — بدون لاتین تا مربع نماند
    lv_label_set_text(objects.lablef2_7, "سرعت راه اندازی");
    if (en) lv_obj_add_flag(objects.lablef2_7, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(objects.lablef2_7, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablee2_7) {
    lv_obj_set_style_text_font(objects.lablee2_7, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.lablee2_7, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablee2_7, "Ramp Speed");
    if (en) lv_obj_clear_flag(objects.lablee2_7, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(objects.lablee2_7, LV_OBJ_FLAG_HIDDEN);
  }
  if (lbl_fa_jog) {
    lv_obj_set_style_text_font(lbl_fa_jog, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_fa_jog, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lbl_fa_jog, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl_fa_jog, "سرعت راه اندازی دستی");
    if (en) lv_obj_add_flag(lbl_fa_jog, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(lbl_fa_jog, LV_OBJ_FLAG_HIDDEN);
  }
  if (lbl_en_jog) {
    lv_obj_set_style_text_font(lbl_en_jog, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_en_jog, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl_en_jog, "Manual Jog Ramp");
    if (en) lv_obj_clear_flag(lbl_en_jog, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(lbl_en_jog, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lbl_spd2_1)
    lv_obj_set_style_text_font(objects.lbl_spd2_1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (objects.lbl_spd2_2)
    lv_obj_set_style_text_font(objects.lbl_spd2_2, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (lbl_spd_jog)
    lv_obj_set_style_text_font(lbl_spd_jog, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  f2_apply_jog_slider_from_value(jog_ramp_ms);

  // کادر طوسی اول/آخر خط — شبکهٔ ثابت داخل پنل ۲۲۸px
  // پد۴ + لیبل۸۸ + فاصله۲ + کادر سفید۱۱۰ + فاصله۲ + رزرو۲۰ + پد۲ = ۲۲۸
  // mm داخل راست کادر سفید، ۱۰px فاصله از لبهٔ راست — عرض کافی برای «mm» کامل (نه فقط m)
  const int f2g_w = 228;
  const int f2g_pad = 4;
  const int f2g_lbl_w = 88;
  const int f2g_gap = 2;
  const int f2g_box_w = 110;
  const int f2g_box_x = f2g_pad + f2g_lbl_w + f2g_gap + 8;  // 94→102 (+۸ راست)
  const int f2g_box_y0 = 8;   // قبل: -10 → +۱۸ پایین
  const int f2g_box_y1 = 55;  // قبل: 37 → +۱۸ پایین
  const int f2g_mm_w = 28;    // montserrat_12: «mm» کامل
  const int f2g_mm_inset = 7;   // ۱۰→۷: mm سه پیکسل راست
  const int f2g_mm_x = f2g_box_w - f2g_mm_w - f2g_mm_inset;
  const int f2g_num_x = 4 - 6 + 3 + 4;  // وسط‌چین → ۴px راست دیگر
  const int f2g_num_w = f2g_mm_x - 4 - 2;  // عرض ناحیهٔ عدد نسبت به لبهٔ mm (نه x شیفت‌شده)
  if (objects.obj8) {
    lv_obj_set_pos(objects.obj8, -9, 188);
    lv_obj_set_size(objects.obj8, f2g_w, 105);  // فقط لبهٔ پایین ۵px کوتاه‌تر
    lv_obj_set_style_bg_color(objects.obj8, lv_color_hex(0xffffff),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.obj8, LV_OPA_COVER,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(objects.obj8, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(objects.obj8, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(objects.obj8, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(objects.obj8, LV_OBJ_FLAG_SCROLLABLE);
  }

  auto f2_style_side_lbl = [&](lv_obj_t *btn, lv_obj_t *fa, lv_obj_t *en_lbl,
                               const char *fa_txt, const char *en_txt, int y) {
    if (btn) {
      lv_obj_set_pos(btn, f2g_pad, y);
      lv_obj_set_size(btn, f2g_lbl_w, 42);
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x0066cc), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (fa) {
      lv_obj_set_size(fa, f2g_lbl_w, 36);
      lv_obj_set_style_text_font(fa, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(fa, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(fa, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(fa, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_long_mode(fa, LV_LABEL_LONG_CLIP);
      String txt(fa_txt);
      if (!g_oobe_done && g_oobe_stage == 2) txt += " *";
      lv_label_set_text(fa, txt.c_str());
      lv_obj_align(fa, LV_ALIGN_TOP_MID, 0, 4);  // ۴ پیکسل پایین‌تر در فارسی
      if (en) lv_obj_add_flag(fa, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(fa, LV_OBJ_FLAG_HIDDEN);
    }
    if (en_lbl) {
      lv_obj_set_size(en_lbl, f2g_lbl_w, 24);
      lv_obj_set_style_text_font(en_lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(en_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(en_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(en_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(en_lbl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_long_mode(en_lbl, LV_LABEL_LONG_CLIP);
      String txt(en_txt);
      if (!g_oobe_done && g_oobe_stage == 2) txt += " *";
      lv_label_set_text(en_lbl, txt.c_str());
      lv_obj_align(en_lbl, LV_ALIGN_TOP_MID, 0, 13);  // وسط‌چین عمودی در انگلیسی (۱۳ پیکسل از بالا)
      if (en) lv_obj_clear_flag(en_lbl, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(en_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  };
  f2_style_side_lbl(objects.btn2_3, objects.lablef2_8, objects.lablee2_8,
                    "سایز اول خط", "Start size", f2g_box_y0);
  f2_style_side_lbl(objects.btn2_4, objects.lablef2_9, objects.lablee2_9,
                    "سایز آخر خط", "End size", f2g_box_y1);

  // پنل راست قدیمی مخفی
  if (objects.obj11) lv_obj_add_flag(objects.obj11, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn2_8) lv_obj_add_flag(objects.btn2_8, LV_OBJ_FLAG_HIDDEN);
  if (objects.lable2_5) lv_obj_add_flag(objects.lable2_5, LV_OBJ_FLAG_HIDDEN);

  // کادر سفید +۱۸ پایین؛ mm داخل راست با inset ۱۰؛ عدد تا قبل از mm
  auto f2_prep_size_box = [&](lv_obj_t *box, int y) {
    if (!box) return;
    lv_obj_set_pos(box, f2g_box_x, y);
    lv_obj_set_size(box, f2g_box_w, 42);
    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xf2f2f7),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(box, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(box, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  };
  f2_prep_size_box(objects.obj9, f2g_box_y0);
  f2_prep_size_box(objects.obj10, f2g_box_y1);
  auto f2_style_mm = [&](lv_obj_t *mm, lv_obj_t *box) {
    if (!mm || !box) return;
    // داخل کادر سفید، ۱۰px از راست؛ متن دقیق «mm» (عرض کافی تا m دوم بریده نشود)
    lv_obj_set_parent(mm, box);
    lv_obj_set_pos(mm, f2g_mm_x, 12);
    lv_obj_set_size(mm, f2g_mm_w, 18);
    lv_obj_set_style_text_font(mm, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mm, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(mm, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(mm, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(mm, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(mm, LV_LABEL_LONG_CLIP);
    lv_label_set_text(mm, "mm");
    lv_obj_clear_flag(mm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(mm);
  };
  f2_style_mm(f2_mm_start, objects.obj9);
  f2_style_mm(f2_mm_end, objects.obj10);
  auto f2_style_size_num = [&](lv_obj_t *lab) {
    if (!lab) return;
    lv_obj_set_pos(lab, f2g_num_x, 6);
    lv_obj_set_size(lab, f2g_num_w, 30);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lab, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(lab, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(lab, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(lab, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_CLIP);
    lv_obj_clear_flag(lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(lab);
  };
  f2_style_size_num(objects.lable2_2);
  f2_style_size_num(objects.lable2_3);
  // عدد روی mm نیاید؛ mm سمت راست داخل کادر بماند
  if (f2_mm_start) lv_obj_move_foreground(f2_mm_start);
  if (f2_mm_end) lv_obj_move_foreground(f2_mm_end);

  f2_wire_input_box_events();
  f2_ensure_line_kb();
  f2_sync_line_box_colors();

  if (!STP.begin_calibration) {
    // حین ویرایش هیچ‌کدام از دو کادر: بافر را از پالس ری‌لود نکن (ریشهٔ برگشت عدد بعد از پاک)
    const bool editing = GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5;
    if (objects.lable2_2) {
      if (editing) {
        if (GUI.enable_edit_lable2_3)
          lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
      } else {
        f2_load_start_buf_from_pulses();
        lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
      }
    }
    if (objects.lable2_3) {
      if (GUI.enable_edit_lable2_5)
        lv_label_set_text(objects.lable2_3, GUI.inputBuffer_max_L.c_str());
      else if (!editing)
        lv_label_set_text(objects.lable2_3, STP.final_position_mm.c_str());
    }
  }
}

void f1_apply_language_texts() {
  const bool en = (GUI.system_language == 2);
  f1_header_title_sync();
  f2_motor_fields_reflow();
  admin_tabs_reflow();
  const bool f1_req = (!g_oobe_done && g_oobe_stage == 1);
  if (lbl_stock_title) {
    // موقعیت ثابت مثل فارسی؛ فقط پد برای متریک فونت انگلیسی
    lv_obj_set_pos(lbl_stock_title, f1_lx(F1_STOCK_LBL_X), f1_ly(F1_STOCK_LBL_Y));
    lv_obj_set_size(lbl_stock_title, F1_STOCK_LBL_W, F1_STOCK_LBL_H);
    lv_obj_set_style_text_align(lbl_stock_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (en) {
      lv_obj_set_style_text_font(lbl_stock_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(lbl_stock_title, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_top(lbl_stock_title, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_line_space(lbl_stock_title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(lbl_stock_title, f1_req ? "Stock length *" : "Stock length");
    } else {
      lv_obj_set_style_text_font(lbl_stock_title, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(lbl_stock_title, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_top(lbl_stock_title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_line_space(lbl_stock_title, -6, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(lbl_stock_title, f1_req ? "طول شاخه *" : "طول شاخه");
    }
  }
  if (lbl_kerf_title) {
    lv_obj_set_pos(lbl_kerf_title, f1_lx(F1_KERF_LBL_X), f1_ly(F1_KERF_LBL_Y));
    lv_obj_set_size(lbl_kerf_title, F1_KERF_LBL_W, F1_KERF_LBL_H);
    lv_obj_set_style_text_align(lbl_kerf_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (en) {
      lv_obj_set_style_text_font(lbl_kerf_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(lbl_kerf_title, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_top(lbl_kerf_title, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_line_space(lbl_kerf_title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(lbl_kerf_title, f1_req ? "Blade / Kerf *" : "Blade / Kerf");
    } else {
      lv_obj_set_style_text_font(lbl_kerf_title, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_base_dir(lbl_kerf_title, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_top(lbl_kerf_title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_line_space(lbl_kerf_title, -6, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(lbl_kerf_title, f1_req ? "عرض تیغه / پرت *" : "عرض تیغه / پرت");
    }
  }
  if (f1_factory_reset_lbl) {
    lv_obj_set_style_text_font(f1_factory_reset_lbl,
                               en ? &lv_font_montserrat_14 : &ui_font_naz18b,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(f1_factory_reset_lbl,
                              en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(f1_factory_reset_lbl, en ? -2 : -12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(f1_factory_reset_lbl, en ? "Factory\nReset" : "بازنشانی\nکارخانه");
    lv_obj_center(f1_factory_reset_lbl);
  }
  if (f1_factory_reserved_lbl) {
    lv_obj_set_style_text_font(f1_factory_reserved_lbl,
                               en ? &lv_font_montserrat_14 : &ui_font_naz18b,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(f1_factory_reserved_lbl,
                              en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(f1_factory_reserved_lbl, en ? -2 : -12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(f1_factory_reserved_lbl, en ? "Auto\nCut" : "برش\nاتوماتیک");
    lv_obj_center(f1_factory_reserved_lbl);
  }
  if (f1_factory_res_lbl) {
    lv_obj_set_style_text_font(f1_factory_res_lbl,
                               en ? &lv_font_montserrat_14 : &ui_font_naz18b,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(f1_factory_res_lbl,
                              en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(f1_factory_res_lbl, en ? "Reserve" : "رزرو");
    lv_obj_center(f1_factory_res_lbl);
  }
  f1_sync_estop_visual();
  f1_lang_btn_sync();
  // راهنمای باز را اینجا حذف نکن — reinit بعد از resume راهنما را می‌کشت
  settings_hide_eez_clutter();  // apply_language ممکن است عنوان/چاپ قدیمی را دوباره روشن کند
}

static void f1_help_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen && (!f1_help_dlg || lv_obj_has_flag(f1_help_dlg, LV_OBJ_FLAG_HIDDEN))) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  f1_help_request_deferred();
}

static void f1_lang_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  // لمس دکمه زبان = تعویض فوری FA/EN (بدون نیاز به زدن دایرهٔ کوچک)
  int sel = (GUI.system_language == 1) ? 1 : 0;  // 0=FA 1=EN — برو به زبان دیگر
  if (objects.roller2_1) {
    lv_obj_send_event(objects.roller2_1, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)sel);
    // پنل را بسته نگه دار
    lv_obj_t *panel = lv_obj_get_child(objects.roller2_1, 1);
    if (panel) lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
  } else {
    GUI.system_language = (sel == 0) ? 1 : 2;
    PEND.system_language = GUI.system_language;
    GUI.pending_lang_selected = sel;
    GUI.flag_apply_language = true;
    f1_lang_btn_sync();
  }
  ui_toast(sel == 0 ? "زبان فارسی" : "زبان انگلیسی",
           sel == 0 ? "Persian" : "English", 900);
  settings_mark_dirty();
}

// مختصات عین simulator/settings-f1-designer.html → initial[] + حس Apple
static void f1_reflow_eez_chrome() {
  if (!objects.obj5) return;
  f1_hide_eez_clutter();

  // پس‌زمینه هم‌رنگ صفحهٔ اصلی
  if (objects.main_2) {
    lv_obj_set_style_bg_color(objects.main_2, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.main_2, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.main_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(objects.main_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_obj_set_style_bg_color(objects.obj5, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(objects.obj5, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(objects.obj5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(objects.obj5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // تنظیم اندازه tabview2_1 و obj5 به ارتفاع کامل ۳۲۰ پیکسل جهت پوشش کل لبه پایین و حذف خط افقی درز
  if (objects.tabview2_1) {
    lv_obj_set_size(objects.tabview2_1, 480, 320);
    lv_obj_set_style_border_width(objects.tabview2_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(objects.tabview2_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *tv_cont = lv_tabview_get_content(objects.tabview2_1);
    if (tv_cont) {
      lv_obj_set_size(tv_cont, 420, 320);
      lv_obj_set_style_border_width(tv_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_outline_width(tv_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(tv_cont, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
  if (objects.obj5) {
    lv_obj_set_size(objects.obj5, 420, 336);
  }

  lv_obj_t *hdr = f1_hdr_root();

  // ---------- آیکون خانه (دکمه برگشت به صفحهٔ اصلی) — طراح: x=8 y=8 w=42 h=32 ----------
  if (objects.btn2_1) {
    lv_obj_set_pos(objects.btn2_1, F1_HOME_X, F1_HDR_Y);
    lv_obj_set_size(objects.btn2_1, F1_HDR_W, F1_HDR_H);
    f1_style_hdr_btn(objects.btn2_1, 0x0a84ff);
    f1_hdr_icon_only(objects.btn2_1, LV_SYMBOL_HOME);
    lv_obj_move_foreground(objects.btn2_1);
  }

  // ---------- دکمه راهنما — طراح: x=54 y=8 w=42 h=32 — فقط آیکون کتاب باز ----------
  if (hdr && !f1_help_btn) {
    f1_help_btn = lv_button_create(hdr);
    lv_obj_add_event_cb(f1_help_btn, f1_help_cb, LV_EVENT_RELEASED, NULL);
  }
  if (f1_help_btn) {
    lv_obj_set_pos(f1_help_btn, F1_HELP_X, F1_HDR_Y);
    lv_obj_set_size(f1_help_btn, F1_HDR_W, F1_HDR_H);
    f1_style_hdr_btn(f1_help_btn, 0x0a84ff);
    f1_hdr_set_book_icon(f1_help_btn);
    lv_obj_move_foreground(f1_help_btn);
  }

  // ---------- باکس عنوان «تنظیمات عمومی» — طراح: x=100 y=8 w=216 h=32 ----------
  if (hdr && !f1_title_bar) {
    f1_title_bar = lv_obj_create(hdr);
    lv_obj_clear_flag(f1_title_bar, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_t *tl = lv_label_create(f1_title_bar);
    lv_label_set_text(tl, "تنظیمات عمومی");
    lv_obj_set_style_text_font(tl, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(tl, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(tl);
  }
  if (f1_title_bar) {
    lv_obj_set_pos(f1_title_bar, F1_TITLE_X, F1_TITLE_Y);
    lv_obj_set_size(f1_title_bar, F1_TITLE_W, F1_TITLE_H);
    lv_obj_set_style_bg_color(f1_title_bar, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(f1_title_bar, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(f1_title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(f1_title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(f1_title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(f1_title_bar);
  }

  // ---------- دکمه زبان — طراح: x=320 y=8 w=42 h=32 — فقط پرچم؛ لمس → لیست EEZ ----------
  f1_place_lang_dropdown();
  if (objects.roller2_1) {
    lv_obj_t *panel = lv_obj_get_child(objects.roller2_1, 1);
    if (panel) lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);  // پیش‌فرض بسته
  }
  if (hdr && !f1_lang_btn) {
    f1_lang_btn = lv_button_create(hdr);
    lv_obj_add_event_cb(f1_lang_btn, f1_lang_cb, LV_EVENT_RELEASED, NULL);
  }
  if (f1_lang_btn) {
    lv_obj_set_pos(f1_lang_btn, F1_LANG_X, F1_HDR_Y);
    lv_obj_set_size(f1_lang_btn, F1_HDR_W, F1_HDR_H);
    f1_style_hdr_btn(f1_lang_btn, 0x0a84ff);
    f1_hdr_set_lang_flags(f1_lang_btn);
    lv_obj_move_foreground(f1_lang_btn);
  }

  // ---------- دکمه ذخیره — فقط آیکون؛ لیبل Save/ذخیره همیشه مخفی ----------
  if (objects.btn_save) {
    lv_obj_set_pos(objects.btn_save, F1_SAVE_X, F1_HDR_Y);
    lv_obj_set_size(objects.btn_save, F1_HDR_W, F1_HDR_H);
    f1_style_hdr_btn(objects.btn_save, settings_dirty ? 0x34C759 : 0x0a84ff);
    if (settings_dirty) {
      lv_obj_set_style_bg_color(objects.btn_save, lv_color_hex(0x248a3d), LV_PART_MAIN | LV_STATE_PRESSED);
    }
    if (objects.lablef_save) {
      lv_label_set_text(objects.lablef_save, "");
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.lablee_save) {
      lv_label_set_text(objects.lablee_save, "");
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
    }
    f1_hdr_icon_only(objects.btn_save, LV_SYMBOL_SAVE);
    lv_obj_move_foreground(objects.btn_save);
  }

  // ---------- تب‌های F1..F5 سمت راست — تب فعال زرد کامل تا لبهٔ سفید ----------
  if (objects.tabview2_1) {
    lv_obj_t *bar = lv_tabview_get_tab_bar(objects.tabview2_1);
    if (bar) {
      lv_obj_set_style_pad_top(bar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_bottom(bar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_left(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_right(bar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_row(bar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      uint32_t n = lv_obj_get_child_count(bar);
      for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *tb = lv_obj_get_child(bar, i);
        if (!tb) continue;
        lv_obj_set_style_radius(tb, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tb, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tb, 2, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(tb, lv_color_hex(0xd0d5db), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(tb, lv_color_hex(0xe6a800), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_outline_width(tb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(tb, 0, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(tb, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(tb, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(tb, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(tb, lv_color_hex(0xffd60a), LV_PART_MAIN | LV_STATE_CHECKED);  // زرد کامل
        lv_obj_set_style_text_color(tb, lv_color_hex(0x4a5d6e), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(tb, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_shadow_width(tb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(tb, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(tb, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_pad_left(tb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(tb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
  }
}

static void f1_kb_chip_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    return;
  }
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  bool on = !GUI.virtual_kb_enable;
  if (on) {
    numpad_ctx = 0;
    settings_num_buf = "";
    settings_clear_on_type = false;
    f1_caret_hide();
  }
  vkb_apply_user_toggle(on);
  settings_sync_stock_kerf_labels();
}

static lv_obj_t *f1_print_btn = NULL;
static lv_obj_t *f1_printer_model_overlay = NULL;
static lv_obj_t *f1_printer_model_title = NULL;
static lv_obj_t *f1_printer_model_note = NULL;
static lv_obj_t *f1_printer_model_opt[2] = { NULL, NULL };
static lv_obj_t *f1_printer_model_lbl[2] = { NULL, NULL };
static bool f1_print_long_fired = false;
static bool f1_printer_model_owns_lock = false;

static void f1_printer_model_close() {
  if (f1_printer_model_overlay)
    lv_obj_add_flag(f1_printer_model_overlay, LV_OBJ_FLAG_HIDDEN);
  if (f1_printer_model_owns_lock && !IOS.stop_button_state &&
      !IOS.motor_fault_state && !sensor_alert_motion_lock && !device_locked)
    GUI.lock_screen = false;
  f1_printer_model_owns_lock = false;
}

static void f1_printer_model_sync() {
  const bool en = (GUI.system_language == 2);
  if (f1_printer_model_title) {
    lv_label_set_text(f1_printer_model_title, en ? "Printer Hardware Model" : "انتخاب مدل پرینتر حرارتی");
    lv_obj_set_style_text_font(f1_printer_model_title,
                               en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  }
  if (f1_printer_model_note) {
    lv_label_set_text(f1_printer_model_note,
                      en ? "Select standard 58mm POS or TTL module:"
                         : "مدل سازگار: پرینتر حرارتی ۵۸mm استاندارد یا ماژول TTL:");
    lv_obj_set_style_text_font(f1_printer_model_note,
                               en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  }
  const char *names_en[2] = { "ESC/POS Thermal (58mm)", "CSN-A2 Thermal (TTL)" };
  const char *names_fa[2] = { "ESC/POS حرارتی (۵۸ میلی‌متر)", "CSN-A2 حرارتی (TTL)" };
  for (uint8_t i = 0; i < 2; ++i) {
    if (!f1_printer_model_opt[i]) continue;
    lv_obj_clear_flag(f1_printer_model_opt[i], LV_OBJ_FLAG_HIDDEN);
    const bool selected = (printer_model == i);
    lv_obj_set_style_bg_color(f1_printer_model_opt[i],
                              lv_color_hex(selected ? 0xE8F3FF : 0xF2F2F7), 0);
    lv_obj_set_style_border_color(f1_printer_model_opt[i],
                                  lv_color_hex(selected ? 0x0A84FF : 0xC7C7CC), 0);
    lv_obj_set_style_border_width(f1_printer_model_opt[i], selected ? 2 : 1, 0);
    lv_label_set_text(f1_printer_model_lbl[i], en ? names_en[i] : names_fa[i]);
    lv_obj_set_style_text_font(f1_printer_model_lbl[i], en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(f1_printer_model_lbl[i], lv_color_hex(selected ? 0x0A84FF : 0x334155), 0);
  }
}

static void f1_printer_model_option_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  const uintptr_t raw = (uintptr_t)lv_event_get_user_data(e);
  if (raw > 1) return;
  printer_model = (uint8_t)raw;
  settings_mark_dirty();
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  f1_printer_model_sync();
  f1_printer_model_close();
  ui_toast(printer_model == 1 ? "پرینتر: CSN-A2" : "پرینتر: ESC/POS",
           printer_model == 1 ? "Printer: CSN-A2" : "Printer: ESC/POS", 1500);
}

static void f1_printer_model_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  f1_printer_model_close();
}

static void f1_printer_model_open() {
  if (!f1_printer_model_overlay) {
    f1_printer_model_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(f1_printer_model_overlay, 0, 0);
    lv_obj_set_size(f1_printer_model_overlay, 480, 320);
    lv_obj_set_style_bg_color(f1_printer_model_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(f1_printer_model_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(f1_printer_model_overlay, 0, 0);
    lv_obj_set_style_pad_all(f1_printer_model_overlay, 0, 0);
    lv_obj_clear_flag(f1_printer_model_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(f1_printer_model_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *card = lv_obj_create(f1_printer_model_overlay);
    lv_obj_set_pos(card, 60, 48);
    lv_obj_set_size(card, 360, 224);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    f1_printer_model_title = lv_label_create(card);
    lv_obj_set_width(f1_printer_model_title, 270);
    lv_obj_set_style_text_align(f1_printer_model_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(f1_printer_model_title, LV_ALIGN_TOP_MID, -12, 10);
    lv_obj_t *x = lv_button_create(card);
    lv_obj_set_pos(x, 308, 6);
    lv_obj_set_size(x, 44, 44);
    lv_obj_set_style_bg_color(x, lv_color_hex(0xE5E5EA), 0);
    lv_obj_set_style_radius(x, 10, 0);
    lv_obj_add_event_cb(x, f1_printer_model_close_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *xl = lv_label_create(x);
    lv_label_set_text(xl, LV_SYMBOL_CLOSE);
    lv_obj_center(xl);
    f1_printer_model_note = lv_label_create(card);
    lv_obj_set_pos(f1_printer_model_note, 16, 55);
    lv_obj_set_width(f1_printer_model_note, 328);
    lv_label_set_long_mode(f1_printer_model_note, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(f1_printer_model_note, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(f1_printer_model_note, lv_color_hex(0x636366), 0);
    for (uint8_t i = 0; i < 2; ++i) {
      f1_printer_model_opt[i] = lv_button_create(card);
      lv_obj_set_pos(f1_printer_model_opt[i], 16, 112 + i * 56);
      lv_obj_set_size(f1_printer_model_opt[i], 328, 48);
      lv_obj_set_style_radius(f1_printer_model_opt[i], 10, 0);
      lv_obj_add_event_cb(f1_printer_model_opt[i], f1_printer_model_option_cb,
                          LV_EVENT_RELEASED, (void *)(uintptr_t)i);
      f1_printer_model_lbl[i] = lv_label_create(f1_printer_model_opt[i]);
      lv_obj_center(f1_printer_model_lbl[i]);
    }
    lv_obj_add_flag(f1_printer_model_overlay, LV_OBJ_FLAG_HIDDEN);
  }
  f1_printer_model_owns_lock = !GUI.lock_screen;
  if (f1_printer_model_owns_lock) GUI.lock_screen = true;
  f1_printer_model_sync();
  lv_obj_clear_flag(f1_printer_model_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(f1_printer_model_overlay);
}

static void f1_sync_print_chip_visual() {
  if (!f1_print_btn) return;
  lv_obj_set_style_bg_color(f1_print_btn, lv_color_hex(printer_enabled ? 0x1c9b67 : 0xc5ccd3),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void f1_print_chip_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) { f1_print_long_fired = false; return; }
  if (code == LV_EVENT_LONG_PRESSED) {
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    f1_print_long_fired = true;
    GUI.buzzer_cmd_short = true;
    f1_printer_model_open();
    return;
  }
  if (code != LV_EVENT_RELEASED) return;
  if (f1_print_long_fired) { f1_print_long_fired = false; return; }
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  printer_enabled = !printer_enabled;
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (!btn) return;
  // خاموش: خاکستری مثل بلندگو؛ روشن: سبز کم‌رنگ
  lv_obj_set_style_bg_color(btn, lv_color_hex(printer_enabled ? 0x1c9b67 : 0xc5ccd3),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (printer_enabled) {
    printertest = 1;
    ui_toast("تست چاپ", "Printer test", 1000);
  } else {
    ui_toast("پرینتر خاموش", "Printer off", 900);
  }
  settings_mark_dirty();
}

static lv_obj_t *f1_mk_chip(lv_obj_t *parent, int x, int y, int w, int h, uint32_t bg, lv_event_cb_t cb) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_RELEASED, NULL);
  return btn;
}

static void f1_factory_reset_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (!btn) return;
  if (code == LV_EVENT_PRESSED) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffdada), LV_PART_MAIN | LV_STATE_DEFAULT);
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff2f2), LV_PART_MAIN | LV_STATE_DEFAULT);
    return;
  }
  if (code == LV_EVENT_LONG_PRESSED) {
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    GUI.flag_factory_reset_dialog = true;
    GUI.buzzer_cmd_short = true;
    return;
  }
}

static lv_obj_t *autocut_trim_modal = NULL;
static lv_obj_t *autocut_trim_ta = NULL;
static char autocut_trim_buf[16] = {0};

static void autocut_trim_close_cb(lv_event_t *e) {
  (void)e;
  if (autocut_trim_modal && lv_obj_is_valid(autocut_trim_modal)) {
    lv_obj_delete(autocut_trim_modal);
  }
  autocut_trim_modal = NULL;
  autocut_trim_ta = NULL;
  GUI.lock_screen = false;
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
}

static void autocut_trim_adj_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  int delta = (int)(intptr_t)lv_event_get_user_data(e);
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  trim_mm = (int)constrain(trim_mm + delta, 0, 500);
  if (autocut_trim_ta && lv_obj_is_valid(autocut_trim_ta)) {
    snprintf(autocut_trim_buf, sizeof(autocut_trim_buf), "%d", trim_mm);
    lv_textarea_set_text(autocut_trim_ta, autocut_trim_buf);
  }
}

static void autocut_trim_save_cb(lv_event_t *e) {
  (void)e;
  if (autocut_trim_ta && lv_obj_is_valid(autocut_trim_ta)) {
    const char *txt = lv_textarea_get_text(autocut_trim_ta);
    if (txt && strlen(txt) > 0) {
      trim_mm = (int)constrain(atoi(txt), 0, 500);
    }
  }
  save_stock_kerf();
  if (autocut_trim_modal && lv_obj_is_valid(autocut_trim_modal)) {
    lv_obj_delete(autocut_trim_modal);
  }
  autocut_trim_modal = NULL;
  autocut_trim_ta = NULL;
  GUI.lock_screen = false;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  ui_toast("سربرش ذخیره شد", "Trim length saved", 2000);
}

static bool autocut_enabled = false;

void ui_show_autocut_trim_modal() {
  if (autocut_trim_modal && lv_obj_is_valid(autocut_trim_modal)) return;
  const bool en = (GUI.system_language != 1);
  GUI.lock_screen = true;
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;

  // کادر اصلی پاپ‌آپ با پس‌زمینه سفید و لبه‌های نرم
  autocut_trim_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_size(autocut_trim_modal, 464, 296);
  lv_obj_center(autocut_trim_modal);
  lv_obj_set_style_bg_color(autocut_trim_modal, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(autocut_trim_modal, 16, 0);
  lv_obj_set_style_border_width(autocut_trim_modal, 2, 0);
  lv_obj_set_style_border_color(autocut_trim_modal, lv_color_hex(0x007aff), 0);
  lv_obj_set_style_shadow_width(autocut_trim_modal, 0, 0);
  lv_obj_clear_flag(autocut_trim_modal, LV_OBJ_FLAG_SCROLLABLE);

  // نوار عنوان وسط‌چین مدرن
  lv_obj_t *title = lv_label_create(autocut_trim_modal);
  lv_obj_set_width(title, 360);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x007aff), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
  lv_label_set_text(title, en ? "Auto-Cut & Trim Settings" : "تنظیمات برش اتوماتیک و سربرش");

  // دکمه خروج دایره‌ای شیک
  lv_obj_t *x_btn = lv_button_create(autocut_trim_modal);
  lv_obj_set_size(x_btn, 30, 30);
  lv_obj_align(x_btn, en ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, en ? -8 : 8, 7);
  lv_obj_set_style_bg_color(x_btn, lv_color_hex(0xf1f5f9), 0);
  lv_obj_set_style_radius(x_btn, 15, 0);
  lv_obj_set_style_border_width(x_btn, 1, 0);
  lv_obj_set_style_border_color(x_btn, lv_color_hex(0xcbd5e1), 0);
  lv_obj_add_event_cb(x_btn, autocut_trim_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *xl = lv_label_create(x_btn);
  lv_label_set_text(xl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(xl, lv_color_hex(0x475569), 0);
  lv_obj_set_style_text_font(xl, &lv_font_montserrat_14, 0);
  lv_obj_center(xl);

  // ================= 1. کارت تنظیم اندازه سربرش (Stepper Card) =================
  lv_obj_t *stepper_card = lv_obj_create(autocut_trim_modal);
  lv_obj_set_pos(stepper_card, 14, 44);
  lv_obj_set_size(stepper_card, 436, 56);
  lv_obj_set_style_bg_color(stepper_card, lv_color_hex(0xf8fafc), 0);
  lv_obj_set_style_radius(stepper_card, 12, 0);
  lv_obj_set_style_border_width(stepper_card, 1, 0);
  lv_obj_set_style_border_color(stepper_card, lv_color_hex(0xe2e8f0), 0);
  lv_obj_clear_flag(stepper_card, LV_OBJ_FLAG_SCROLLABLE);

  // عنوان سربرش داخل کارت
  lv_obj_t *lbl = lv_label_create(stepper_card);
  lv_obj_set_style_text_font(lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_base_dir(lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(lbl, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 10 : -10, 0);
  lv_label_set_text(lbl, en ? "Start Trim Length (mm):" : "میزان سربرش ابتدای شاخه:");

  // دکمه‌های کاهش و افزایش و کادر عدد (چیدمان متقارن)
  const int step_base_x = en ? 240 : 10;
  lv_obj_t *b_m10 = lv_button_create(stepper_card);
  lv_obj_set_pos(b_m10, step_base_x, 7);
  lv_obj_set_size(b_m10, 48, 40);
  lv_obj_set_style_bg_color(b_m10, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_radius(b_m10, 8, 0);
  lv_obj_add_event_cb(b_m10, autocut_trim_adj_cb, LV_EVENT_RELEASED, (void *)(intptr_t)-10);
  lv_obj_t *lm10 = lv_label_create(b_m10);
  lv_obj_set_style_text_font(lm10, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lm10, lv_color_hex(0x0f172a), 0);
  lv_label_set_text(lm10, "-10");
  lv_obj_center(lm10);

  autocut_trim_ta = lv_textarea_create(stepper_card);
  lv_textarea_set_one_line(autocut_trim_ta, true);
  lv_obj_set_pos(autocut_trim_ta, step_base_x + 54, 7);
  lv_obj_set_size(autocut_trim_ta, 74, 40);
  lv_obj_set_style_bg_color(autocut_trim_ta, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_border_width(autocut_trim_ta, 1, 0);
  lv_obj_set_style_border_color(autocut_trim_ta, lv_color_hex(0x007aff), 0);
  lv_obj_set_style_radius(autocut_trim_ta, 8, 0);
  lv_obj_set_style_text_font(autocut_trim_ta, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(autocut_trim_ta, lv_color_hex(0x007aff), 0);
  lv_obj_set_style_text_align(autocut_trim_ta, LV_TEXT_ALIGN_CENTER, 0);
  snprintf(autocut_trim_buf, sizeof(autocut_trim_buf), "%d", trim_mm);
  lv_textarea_set_text(autocut_trim_ta, autocut_trim_buf);

  lv_obj_t *b_p10 = lv_button_create(stepper_card);
  lv_obj_set_pos(b_p10, step_base_x + 134, 7);
  lv_obj_set_size(b_p10, 48, 40);
  lv_obj_set_style_bg_color(b_p10, lv_color_hex(0x007aff), 0);
  lv_obj_set_style_radius(b_p10, 8, 0);
  lv_obj_add_event_cb(b_p10, autocut_trim_adj_cb, LV_EVENT_RELEASED, (void *)(intptr_t)10);
  lv_obj_t *lp10 = lv_label_create(b_p10);
  lv_obj_set_style_text_font(lp10, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lp10, lv_color_hex(0xffffff), 0);
  lv_label_set_text(lp10, "+10");
  lv_obj_center(lp10);

  // ================= 2. چیپ‌های سه‌گانه اطلاعاتی (۳ ردیف تفکیک‌شده شکیل) =================
  // چیپ ۱: زاویه برش و سربرش
  lv_obj_t *chip1 = lv_obj_create(autocut_trim_modal);
  lv_obj_set_pos(chip1, 14, 106);
  lv_obj_set_size(chip1, 436, 38);
  lv_obj_set_style_bg_color(chip1, lv_color_hex(0xf1f5f9), 0);
  lv_obj_set_style_radius(chip1, 8, 0);
  lv_obj_set_style_border_width(chip1, 1, 0);
  lv_obj_set_style_border_color(chip1, lv_color_hex(0xe2e8f0), 0);
  lv_obj_clear_flag(chip1, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *badge1 = lv_label_create(chip1);
  lv_obj_set_style_text_font(badge1, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(badge1, lv_color_hex(0x0284c7), 0);
  lv_obj_set_style_base_dir(badge1, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(badge1, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 8 : -8, 0);
  lv_label_set_text(badge1, en ? "[ Angle Mode ]" : "[ زاویه برش ]");

  lv_obj_t *txt1 = lv_label_create(chip1);
  lv_obj_set_style_text_font(txt1, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(txt1, lv_color_hex(0x334155), 0);
  lv_obj_set_style_base_dir(txt1, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(txt1, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 115 : -115, 0);
  lv_label_set_text(txt1, en ? "45 deg with auto trim  |  90 deg without trim" : "زاویه ۴۵ با سربرش خودکار  |  زاویه ۹۰ بدون سربرش");

  // چیپ ۲: سرعت هدایت فیدر
  lv_obj_t *chip2 = lv_obj_create(autocut_trim_modal);
  lv_obj_set_pos(chip2, 14, 148);
  lv_obj_set_size(chip2, 436, 38);
  lv_obj_set_style_bg_color(chip2, lv_color_hex(0xf1f5f9), 0);
  lv_obj_set_style_radius(chip2, 8, 0);
  lv_obj_set_style_border_width(chip2, 1, 0);
  lv_obj_set_style_border_color(chip2, lv_color_hex(0xe2e8f0), 0);
  lv_obj_clear_flag(chip2, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *badge2 = lv_label_create(chip2);
  lv_obj_set_style_text_font(badge2, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(badge2, lv_color_hex(0xd97706), 0);
  lv_obj_set_style_base_dir(badge2, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(badge2, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 8 : -8, 0);
  lv_label_set_text(badge2, en ? "[ Feed Speed ]" : "[ سرعت فیدر ]");

  lv_obj_t *txt2 = lv_label_create(chip2);
  lv_obj_set_style_text_font(txt2, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(txt2, lv_color_hex(0x334155), 0);
  lv_obj_set_style_base_dir(txt2, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(txt2, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 115 : -115, 0);
  lv_label_set_text(txt2, en ? "Slow feed at 1/3 speed for smooth profile push" : "کاهش به یک‌سوم سرعت جهت هل دادن آرام پروفیل");

  // چیپ ۳: محدودیت کورس
  lv_obj_t *chip3 = lv_obj_create(autocut_trim_modal);
  lv_obj_set_pos(chip3, 14, 190);
  lv_obj_set_size(chip3, 436, 38);
  lv_obj_set_style_bg_color(chip3, lv_color_hex(0xf1f5f9), 0);
  lv_obj_set_style_radius(chip3, 8, 0);
  lv_obj_set_style_border_width(chip3, 1, 0);
  lv_obj_set_style_border_color(chip3, lv_color_hex(0xe2e8f0), 0);
  lv_obj_clear_flag(chip3, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *badge3 = lv_label_create(chip3);
  lv_obj_set_style_text_font(badge3, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(badge3, lv_color_hex(0x475569), 0);
  lv_obj_set_style_base_dir(badge3, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(badge3, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 8 : -8, 0);
  lv_label_set_text(badge3, en ? "[ Travel Limit ]" : "[ کنترل کورس ]");

  lv_obj_t *txt3 = lv_label_create(chip3);
  lv_obj_set_style_text_font(txt3, en ? &lv_font_montserrat_12 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(txt3, lv_color_hex(0x334155), 0);
  lv_obj_set_style_base_dir(txt3, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_align(txt3, en ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID, en ? 115 : -115, 0);
  lv_label_set_text(txt3, en ? "Auto stroke limiter based on maximum machine travel" : "کنترل خودکار موقعیت بر اساس حداکثر کورس دستگاه");

  // ================= 3. دکمه‌های اقدام زیرین (متوازن و متقارن) =================
  // دکمه تایید / ذخیره
  lv_obj_t *save_btn = lv_button_create(autocut_trim_modal);
  lv_obj_set_pos(save_btn, en ? 14 : 234, 238);
  lv_obj_set_size(save_btn, 216, 44);
  lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x16a34a), 0);
  lv_obj_set_style_radius(save_btn, 12, 0);
  lv_obj_add_event_cb(save_btn, autocut_trim_save_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *s_lbl = lv_label_create(save_btn);
  lv_obj_set_style_text_font(s_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(s_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(s_lbl, en ? "Save Settings" : "ذخیره تنظیمات");
  lv_obj_center(s_lbl);

  // دکمه انصراف
  lv_obj_t *close_btn = lv_button_create(autocut_trim_modal);
  lv_obj_set_pos(close_btn, en ? 240 : 14, 238);
  lv_obj_set_size(close_btn, 210, 44);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x64748b), 0);
  lv_obj_set_style_radius(close_btn, 12, 0);
  lv_obj_add_event_cb(close_btn, autocut_trim_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *c_lbl = lv_label_create(close_btn);
  lv_obj_set_style_text_font(c_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(c_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(c_lbl, en ? "Cancel" : "انصراف");
  lv_obj_center(c_lbl);
}

static void f1_sync_autocut_btn_visual() {
  if (!f1_factory_reserved_btn || !lv_obj_is_valid(f1_factory_reserved_btn)) return;
  if (!f1_factory_reserved_lbl || !lv_obj_is_valid(f1_factory_reserved_lbl)) return;
  if (autocut_enabled) {
    lv_obj_set_style_bg_color(f1_factory_reserved_btn, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(f1_factory_reserved_btn, lv_color_hex(0x0369a1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(f1_factory_reserved_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_obj_set_style_bg_color(f1_factory_reserved_btn, lv_color_hex(0xf1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(f1_factory_reserved_btn, lv_color_hex(0xc5ccd3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(f1_factory_reserved_lbl, lv_color_hex(0x64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void f1_autocut_chip_cb(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (!btn) return;
  if (code == LV_EVENT_LONG_PRESSED) {
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    ui_show_autocut_trim_modal();
    return;
  }
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    autocut_enabled = !autocut_enabled;
    f1_sync_autocut_btn_visual();
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    ui_toast(autocut_enabled ? "برش اتوماتیک: فعال" : "برش اتوماتیک: غیرفعال",
             autocut_enabled ? "Auto Cut: Enabled" : "Auto Cut: Disabled", 1500);
  }
}

static void f1_reserve_chip_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  ui_toast("رزرو", "Reserved", 1200);
}

static void vkb_settings_build() {
  if (sw_vkb || !objects.obj5) return;
  lv_obj_t *parent = objects.obj5;  // محتوا روی obj5 → از f1_lx/f1_ly استفاده کن
  f1_reflow_eez_chrome();

  // ---------- کارت سفید پنل ابزار چپ — گوشهٔ چپ زیر خانه/راهنما (F1_TOOLS_*) ----------
  lv_obj_t *tools = lv_obj_create(parent);
  lv_obj_set_pos(tools, f1_lx(F1_TOOLS_X), f1_ly(F1_TOOLS_Y));
  lv_obj_set_size(tools, F1_TOOLS_W, F1_TOOLS_H);
  lv_obj_clear_flag(tools, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_color(tools, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(tools, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(tools, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(tools, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(tools, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(tools, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  // clip_corner باعث برش وای‌فای از بالا و «عادی باز» از پایین می‌شد
  lv_obj_set_style_clip_corner(tools, false, LV_PART_MAIN | LV_STATE_DEFAULT);

  // ---------- دکمه وای‌فای (وب‌پورتال) — طراح: x=14 y=52 w=80 h=32 — فقط آیکون ~۶۰٪ بزرگ‌تر، وسط ----------
  lv_obj_t *wifi = f1_mk_chip(tools, f1_tx(F1_WIFI_X), f1_ty(F1_WIFI_Y),
                              F1_WIFI_W, F1_WIFI_H, 0xd8f5e0, settings_web_btn_cb);
  f1_web_btn = wifi;
  lv_obj_set_style_radius(wifi, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(wifi, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(wifi, lv_color_hex(0x1b9e4a), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(wifi, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_t *wi = lv_label_create(wifi);
  // قبلاً ۱۴ بود؛ ~۶۰٪ بزرگ‌تر → ۲۲
  lv_obj_set_style_text_font(wi, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(wi, lv_color_hex(0x1b9e4a), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(wi, LV_SYMBOL_WIFI);
  lv_obj_center(wi);
  lv_obj_add_event_cb(wifi, f1_wifi_hold_cb, LV_EVENT_LONG_PRESSED, NULL);
  f1_sync_wifi_chip();

  // سوئیچ مخفی کیبورد مجازی (منطق؛ ظاهرش همان چیپ کیبورد است)
  sw_vkb = lv_switch_create(tools);
  lv_obj_add_flag(sw_vkb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(sw_vkb, sw_vkb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  if (GUI.virtual_kb_enable) lv_obj_add_state(sw_vkb, LV_STATE_CHECKED);
  else lv_obj_clear_state(sw_vkb, LV_STATE_CHECKED);

  // ---------- دکمه کیبورد مجازی روشن/خاموش — طراح: x=14 y=88 w=80 h=28 ----------
  f1_kb_chip = f1_mk_chip(tools, f1_tx(F1_KB_X), f1_ty(F1_KB_Y),
                          F1_KB_W, F1_KB_H,
                          GUI.virtual_kb_enable ? 0x0a84ff : 0xc5ccd3, f1_kb_chip_cb);
  lv_obj_set_style_radius(f1_kb_chip, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_t *kbi = lv_label_create(f1_kb_chip);
  lv_obj_set_style_text_font(kbi, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);  // ~۱۰٪ بزرگ‌تر
  lv_obj_set_style_text_color(kbi, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(kbi);
  lv_label_set_text(kbi, LV_SYMBOL_KEYBOARD);

  // ---------- دکمه بازر (بلندگو) — طراح: x=14 y=120 w=80 h=28 ----------
  f1_buz_chip = f1_mk_chip(tools, f1_tx(F1_BUZ_X), f1_ty(F1_BUZ_Y),
                           F1_BUZ_W, F1_BUZ_H,
                           GUI.buzzer_enable ? 0x0a84ff : 0xc5ccd3, f1_buz_chip_cb);
  lv_obj_set_style_radius(f1_buz_chip, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_t *bzi = lv_label_create(f1_buz_chip);
  lv_obj_set_style_text_font(bzi, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(bzi, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(bzi);
  lv_label_set_text(bzi, GUI.buzzer_enable ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);

  // ---------- دکمه پرینتر — فعال/غیرفعال + تست چاپ هنگام روشن ----------
  f1_print_btn = f1_mk_chip(tools, f1_tx(F1_PRINT_X), f1_ty(F1_PRINT_Y),
                            F1_PRINT_W, F1_PRINT_H,
                            printer_enabled ? 0x1c9b67 : 0xc5ccd3, NULL);
  lv_obj_set_style_radius(f1_print_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(f1_print_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(f1_print_btn, f1_print_chip_cb, LV_EVENT_ALL, NULL);
  {
    // آیکون بدون پس‌زمینهٔ سفید عکس (alpha شفاف شده) + ~۱۰٪ بزرگ‌تر
    lv_obj_t *pi = lv_image_create(f1_print_btn);
    lv_image_set_src(pi, &img_printer);
    lv_image_set_scale(pi, 165);  // قبلاً ۱۵۰
    lv_obj_set_style_bg_opa(pi, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(pi);
  }

  // ---------- بلوک E-STOP — جمع‌وجور تا «عادی بسته» کامل دیده شود ----------
  lv_obj_t *estop = lv_obj_create(tools);
  lv_obj_set_pos(estop, f1_tx(F1_ESTOP_X), f1_ty(F1_ESTOP_Y));
  lv_obj_set_size(estop, F1_ESTOP_W, F1_ESTOP_H);
  lv_obj_clear_flag(estop, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(estop, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(estop, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(estop, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(estop, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(estop, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(estop, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(estop, f1_estop_cb, LV_EVENT_RELEASED, NULL);

  lv_obj_t *emg = lv_image_create(estop);
  f1_estop_art = emg;
  lv_image_set_src(emg, &img_emg_stop);  // پس‌زمینهٔ سفید عکس شفاف شده
  lv_obj_set_pos(emg, 8, 0);
  lv_obj_set_size(emg, 64, 64);
  lv_image_set_scale(emg, 228);
  lv_image_set_inner_align(emg, LV_IMAGE_ALIGN_CENTER);
  lv_obj_set_style_bg_opa(emg, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(emg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(emg, f1_estop_cb, LV_EVENT_RELEASED, NULL);
  if (objects.st_bt_co) lv_obj_add_flag(objects.st_bt_co, LV_OBJ_FLAG_HIDDEN);

  // کادر N.C/N.O: ارتفاع +۵٪، متن وسط‌چین
  f1_ncno_bg = lv_obj_create(estop);
  lv_obj_set_pos(f1_ncno_bg, 15, 61);
  lv_obj_set_size(f1_ncno_bg, 52, 14);  // h 13→14 (+~5%)
  lv_obj_clear_flag(f1_ncno_bg, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_color(f1_ncno_bg, lv_color_hex(0xe53935), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(f1_ncno_bg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_ncno_bg, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(f1_ncno_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_ncno_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  f1_ncno = lv_label_create(f1_ncno_bg);
  lv_obj_set_style_text_font(f1_ncno, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(f1_ncno, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(f1_ncno, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(f1_ncno, "N.C");
  lv_obj_align(f1_ncno, LV_ALIGN_CENTER, 0, 0);

  // کادر «عادی باز/بسته» — عرض +۸٪ روی محور X، متن وسط‌چین
  f1_ncno_desc_bg = lv_obj_create(estop);
  lv_obj_set_pos(f1_ncno_desc_bg, 3, 80);
  lv_obj_set_size(f1_ncno_desc_bg, 74, 23);  // w 68→74 (+~8%)
  lv_obj_clear_flag(f1_ncno_desc_bg, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_color(f1_ncno_desc_bg, lv_color_hex(0xe53935), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(f1_ncno_desc_bg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_ncno_desc_bg, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(f1_ncno_desc_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_ncno_desc_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  f1_ncno_desc_lbl = lv_label_create(f1_ncno_desc_bg);
  lv_obj_set_style_text_color(f1_ncno_desc_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(f1_ncno_desc_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(f1_ncno_desc_lbl, -6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(f1_ncno_desc_lbl, LV_LABEL_LONG_CLIP);
  lv_label_set_text(f1_ncno_desc_lbl, "عادی بسته");
  lv_obj_set_width(f1_ncno_desc_lbl, 70);
  lv_obj_align(f1_ncno_desc_lbl, LV_ALIGN_CENTER, 0, 0);
  f1_ncno_desc = f1_ncno_desc_lbl;

  f1_sync_estop_visual();
  f1_estop_art = f1_ncno = f1_ncno_bg = f1_ncno_desc = NULL;
  f1_ncno_desc_bg = f1_ncno_desc_lbl = NULL;
  lv_obj_delete(estop);  // کنترل قطبیت استاپ به F4 منتقل شد

  // ---------- کارت سفید مقادیر (طول شاخه / عرض تیغه) — طراح: x=108 y=48 w=300 h=86 ----------
  f1_center_card = lv_obj_create(parent);
  lv_obj_set_pos(f1_center_card, f1_lx(F1_CARD_X), f1_ly(F1_CARD_Y));
  lv_obj_set_size(f1_center_card, F1_CARD_W, F1_CARD_H);
  lv_obj_clear_flag(f1_center_card, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_color(f1_center_card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(f1_center_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_center_card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_center_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(f1_center_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // ---------- برچسب «طول شاخه» — ارتفاع کافی برای naz (نه برش نصف) ----------
  lv_obj_t *ls = lv_label_create(parent);
  lbl_stock_title = ls;
  lv_obj_set_pos(ls, f1_lx(F1_STOCK_LBL_X), f1_ly(F1_STOCK_LBL_Y));
  lv_obj_set_size(ls, F1_STOCK_LBL_W, F1_STOCK_LBL_H);
  lv_obj_set_style_text_font(ls, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ls, lv_color_hex(0x5b6b7c), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(ls, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(ls, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(ls, -6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(ls, LV_LABEL_LONG_CLIP);
  lv_label_set_text(ls, "طول شاخه");
  lv_obj_move_foreground(ls);

  // ---------- کادر عدد طول شاخه (قابل لمس → نامپد) — طراح: x=116 y=74 w=136 h=46 ----------
  box_stock = mk_value_box(parent, f1_lx(F1_STOCK_BOX_X), f1_ly(F1_STOCK_BOX_Y),
                           F1_STOCK_BOX_W, F1_STOCK_BOX_H, 3);
  lbl_set_stock = lv_label_create(box_stock);
  lv_obj_clear_flag(lbl_set_stock, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(lbl_set_stock, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_set_stock, lv_color_hex(0x0a66c2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_set_stock, "6000");
  lbl_unit_stock = lv_label_create(box_stock);
  lv_obj_clear_flag(lbl_unit_stock, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(lbl_unit_stock, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_unit_stock, lv_color_hex(0x8e8e93), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_unit_stock, "mm");

  // ---------- برچسب «عرض تیغه / پرت» ----------
  lv_obj_t *kl = lv_label_create(parent);
  lbl_kerf_title = kl;
  lv_obj_set_pos(kl, f1_lx(F1_KERF_LBL_X), f1_ly(F1_KERF_LBL_Y));
  lv_obj_set_size(kl, F1_KERF_LBL_W, F1_KERF_LBL_H);
  lv_obj_set_style_text_font(kl, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(kl, lv_color_hex(0x5b6b7c), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(kl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(kl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(kl, -6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(kl, LV_LABEL_LONG_CLIP);
  lv_label_set_text(kl, "عرض تیغه / پرت");
  lv_obj_move_foreground(kl);

  // ---------- کادر عدد عرض تیغه (قابل لمس → نامپد) — طراح: x=264 y=74 w=132 h=46 ----------
  box_kerf = mk_value_box(parent, f1_lx(F1_KERF_BOX_X), f1_ly(F1_KERF_BOX_Y),
                          F1_KERF_BOX_W, F1_KERF_BOX_H, 4);
  lbl_set_kerf = lv_label_create(box_kerf);
  lv_obj_clear_flag(lbl_set_kerf, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(lbl_set_kerf, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_set_kerf, lv_color_hex(0x0a66c2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_set_kerf, "3.0");
  lbl_unit_kerf = lv_label_create(box_kerf);
  lv_obj_clear_flag(lbl_unit_kerf, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(lbl_unit_kerf, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_unit_kerf, lv_color_hex(0x8e8e93), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_unit_kerf, "mm");

  // ورودی امن به سناریوی موجود بازنشانی؛ لمس کوتاه هیچ داده ای را پاک نمی کند.
  // --- کارت سرویس زیر ابزار پرینتر در پنل چپ (F1_FACTORY_*) ---
  lv_obj_t *fcard = lv_obj_create(tools);
  lv_obj_set_pos(fcard, f1_tx(F1_FACTORY_CARD_X), f1_ty(F1_FACTORY_CARD_Y));
  lv_obj_set_size(fcard, F1_FACTORY_CARD_W, F1_FACTORY_CARD_H);
  lv_obj_clear_flag(fcard, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_bg_opa(fcard, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(fcard, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(fcard, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(fcard, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(fcard, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(fcard, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(fcard, false, LV_PART_MAIN | LV_STATE_DEFAULT);

  f1_factory_reset_btn = lv_button_create(fcard);
  lv_obj_set_pos(f1_factory_reset_btn, F1_FACTORY_X, F1_FACTORY_Y);
  lv_obj_set_size(f1_factory_reset_btn, F1_FACTORY_W, F1_FACTORY_H);
  lv_obj_set_style_bg_color(f1_factory_reset_btn, lv_color_hex(0xd32f2f), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(f1_factory_reset_btn, lv_color_hex(0xb71c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(f1_factory_reset_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_factory_reset_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(f1_factory_reset_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_factory_reset_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(f1_factory_reset_btn, f1_factory_reset_cb, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(f1_factory_reset_btn, (lv_obj_flag_t)(LV_OBJ_FLAG_USER_1 | LV_OBJ_FLAG_USER_2));
  f1_factory_reset_lbl = lv_label_create(f1_factory_reset_btn);
  lv_obj_set_width(f1_factory_reset_lbl, 76);
  lv_obj_set_style_text_font(f1_factory_reset_lbl, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(f1_factory_reset_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(f1_factory_reset_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(f1_factory_reset_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(f1_factory_reset_lbl, -12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(f1_factory_reset_lbl, "بازنشانی\nکارخانه");
  lv_obj_center(f1_factory_reset_lbl);

  // دکمه برش اتوماتیک؛ کلیک راهنمایی می‌دهد، نگه داشتن منوی تنظیمات سربرش را باز می‌کند
  f1_factory_reserved_btn = lv_button_create(fcard);
  lv_obj_set_pos(f1_factory_reserved_btn, F1_FACTORY_RESERVED_X, F1_FACTORY_RESERVED_Y);
  lv_obj_set_size(f1_factory_reserved_btn, F1_FACTORY_RESERVED_W, F1_FACTORY_RESERVED_H);
  lv_obj_set_style_border_width(f1_factory_reserved_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_factory_reserved_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(f1_factory_reserved_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_factory_reserved_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(f1_factory_reserved_btn, f1_autocut_chip_cb, LV_EVENT_ALL, NULL);
  f1_factory_reserved_lbl = lv_label_create(f1_factory_reserved_btn);
  lv_obj_set_width(f1_factory_reserved_lbl, 76);
  lv_obj_set_style_text_font(f1_factory_reserved_lbl, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(f1_factory_reserved_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(f1_factory_reserved_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(f1_factory_reserved_lbl, -12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(f1_factory_reserved_lbl, "برش\nاتوماتیک");
  lv_obj_center(f1_factory_reserved_lbl);
  f1_sync_autocut_btn_visual();

  // دکمه رزرو زیر دکمه برش اتوماتیک
  f1_factory_res_btn = lv_button_create(fcard);
  lv_obj_set_pos(f1_factory_res_btn, F1_FACTORY_RES_X, F1_FACTORY_RES_Y);
  lv_obj_set_size(f1_factory_res_btn, F1_FACTORY_RES_W, F1_FACTORY_RES_H);
  lv_obj_set_style_bg_color(f1_factory_res_btn, lv_color_hex(0xf1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(f1_factory_res_btn, lv_color_hex(0xc5ccd3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(f1_factory_res_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(f1_factory_res_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(f1_factory_res_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(f1_factory_res_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(f1_factory_res_btn, f1_reserve_chip_cb, LV_EVENT_ALL, NULL);
  f1_factory_res_lbl = lv_label_create(f1_factory_res_btn);
  lv_obj_set_width(f1_factory_res_lbl, 76);
  lv_obj_set_style_text_font(f1_factory_res_lbl, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(f1_factory_res_lbl, lv_color_hex(0x64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(f1_factory_res_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(f1_factory_res_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(f1_factory_res_lbl, "رزرو");
  lv_obj_center(f1_factory_res_lbl);

  f1_sync_wifi_chip();
  settings_sync_stock_kerf_labels();
  f1_apply_language_texts();
  // نامپد جداست؛ مختصاتش در numpad_style_f1: F1_NUMPAD_X/Y
}

void vkb_settings_sync() {
  if (!sw_vkb) return;
  // ارتفاع نوار ابزار سفید (اگر قبلاً ساخته شده)
  if (f1_kb_chip) {
    lv_obj_t *tools = lv_obj_get_parent(f1_kb_chip);
    if (tools) lv_obj_set_size(tools, F1_TOOLS_W, F1_TOOLS_H);
  }
  // فقط وضعیت چیپ‌ها — بدون f1_reflow و بدون باز کردن خودکار نامپد/نشانگر
  f1_hide_eez_clutter();
  if (GUI.virtual_kb_enable) lv_obj_add_state(sw_vkb, LV_STATE_CHECKED);
  else lv_obj_clear_state(sw_vkb, LV_STATE_CHECKED);
  if (f1_kb_chip) {
    lv_obj_set_style_bg_color(f1_kb_chip, lv_color_hex(GUI.virtual_kb_enable ? 0x0a84ff : 0xc5ccd3),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (f1_buz_chip) {
    lv_obj_set_style_bg_color(f1_buz_chip, lv_color_hex(GUI.buzzer_enable ? 0x0a84ff : 0xc5ccd3),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *ic = lv_obj_get_child(f1_buz_chip, 0);
    if (ic) lv_label_set_text(ic, GUI.buzzer_enable ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
  }
  f1_sync_wifi_chip();
  f1_sync_estop_visual();
  settings_sync_stock_kerf_labels();
  // نامپد لمسی را مطابق حالت مؤثر هم‌تراز کن — فوکوس USB روی F1 را پاک نکن
  if (vkb_auto_open_guard_active() || !settings_soft_kb_tab_ok() || !GUI.virtual_kb_enable) {
    GUI.pending_f1_numpad_open = false;
    if (numpad_ctx == 1 || numpad_ctx == 2) numpad_close();
    else numpad_hide_soft_widgets();
  } else {
    GUI.pending_f1_numpad_open = true;
  }
}

void splash_release_hold_cover() {
  if (!splash_hold_for_goto) return;
  if (ui_resume_apply_pending) return;
  // راهنمای F1 دو تیک deferred دارد؛ تا خود پنجره ساخته نشده صفحهٔ میانی دیده نشود.
  if (splash_hold_is_resume && pending_f1_help_phase != 0) return;
  const bool was_resume = splash_hold_is_resume;
  splash_hold_for_goto = false;
  splash_hold_is_resume = false;
  if (splash_cont) {
    lv_obj_delete(splash_cont);
    splash_cont = NULL;
    splash_free_img_buf();
  }
  device_lock_ui_hold_for_splash(false);
  GUI.ui_nav_ready_ms = millis();
  // resume هنگ: هرگز مسیر «boot → آماده‌سازی اصلی» را راه نینداز
  if (!was_resume)
    GUI.pending_boot_main_ready = true;
}

void prcs_splash() {  // در Task2 — بعد از مهلت، اسپلش حذف می‌شود
  if (!splash_cont) return;

  if (splash_phase == 99) {
    if ((int32_t)(lv_tick_get() - splash_hide_at) >= 0) {
      splash_close_manual();
    }
    return;
  }

  // ۱) چرخه نمایش ۲ عکس متوالی بوت (عکس اول ۲ ثانیه، سپس عکس دوم ۲ ثانیه)
  if (splash_phase == 1 && (int32_t)(lv_tick_get() - splash_hide_at) >= 0) {
    splash_phase = 2;
    const bool have_photo2 = splash_try_load_rgb565(1);
    if (have_photo2) {
      if (splash_img_obj && lv_obj_is_valid(splash_img_obj)) {
        lv_image_set_src(splash_img_obj, &splash_img_dsc);
        lv_obj_invalidate(splash_img_obj);
      } else {
        splash_img_obj = lv_image_create(splash_cont);
        lv_image_set_src(splash_img_obj, &splash_img_dsc);
        lv_obj_set_pos(splash_img_obj, 0, 0);
        lv_obj_move_to_index(splash_img_obj, 0);
        lv_obj_clear_flag(splash_img_obj, LV_OBJ_FLAG_CLICKABLE);
      }
    }
    splash_hide_at = lv_tick_get() + 2000;
    return;
  }

  // تا زمانی که فاز ۱ یا فاز ۲ در حال نمایش است، هیچ پرش یا رزومی اسپلش را قطع نمی‌کند
  if (splash_phase == 1 || splash_phase == 2) {
    if ((int32_t)(lv_tick_get() - splash_hide_at) < 0) {
      return;  // مهلت ۲ ثانیه‌ای عکس هنوز تمام نشده است
    }
  }

  // ۲) صف goto ریکاوری در صورتی که دکمه‌ای زده شده باشد
  if (dma_ui_resume_scr >= 0 && dma_ui_resume_scr <= 3) {
    int s = dma_ui_resume_scr;
    dma_ui_resume_scr = -1;
    ui_resume_clear();  // فقط بعد از صف‌کردن goto
    STP.referenced = false;
    STP.goref_cmd = false;
    STP.moveStepperCMD = false;
    STP.jogEnabled = false;
    STP.mvm_interupt = false;
    GUI.flag_start_key = false;
    ref_failed_flag = false;
    ref_wrong_dir_flag = false;
    ref_fail_reason = 0;
    ref_error_ui_active = false;
    GUI.lock_screen = true;  // تا apply نهایی، هیچ فرمان حرکتی از صفحهٔ بازیابی‌شده نپذیر
    if (s == 0) s = 1;
    ui_resume_apply_scr = s;
    ui_resume_apply_pending = true;  // اگر boot قبلاً ست کرده بود، محکم بمان
    GUI.pending_goto_scr = s;
    GUI.ui_nav_ready_ms = millis();
    splash_hold_for_goto = true;
    splash_hold_is_resume = true;
    splash_hide_at = lv_tick_get();  // تایم‌اوت از همین لحظه
    lv_obj_set_style_bg_color(splash_cont, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    ets_printf("[UIRESUME] queue goto scr=%d\n", s);
    return;
  }

  // ۳) در بازگشت امن، مقصد قبلی صف شود.
  if (splash_resume_target >= 0 && (int32_t)(lv_tick_get() - splash_hide_at) >= 0) {
    dma_ui_resume_scr = splash_resume_target;
    splash_resume_target = -1;
    splash_resume_applied = true;
    return;
  }

  // ۴) تایم‌اوت فقط وقتی هنوز goto/apply زنده نیست — resume را نکش
  if (splash_hold_for_goto &&
      !ui_resume_apply_pending &&
      GUI.pending_goto_scr < 0 &&
      (int32_t)(lv_tick_get() - splash_hide_at) > 5000) {
    splash_release_hold_cover();
    if (!splash_cont) return;
  }

  // ۵) پایان کامل اسپلش (هر ۲ عکس کامل دیده شدند)
  if ((splash_phase == 2 || splash_phase == 0) && (int32_t)(lv_tick_get() - splash_hide_at) >= 0) {
    if (splash_hold_for_goto) return;  // منتظر لود صفحهٔ هدف / apply
    splash_phase = 0;
    lv_obj_delete(splash_cont);
    splash_cont = NULL;
    splash_img_obj = NULL;
    splash_free_img_buf();
    device_lock_ui_hold_for_splash(false);
    GUI.ui_nav_ready_ms = millis() + 1000;

    // بررسی ویزارد راه‌اندازی اولیه سازنده
    extern void oobe_wizard_check_start();
    oobe_wizard_check_start();

    // resume هنگ: صف «boot → صفحهٔ اول → اصلی» راه نیفتد؛ کاربر نبیهد هنگ کرده
    if (!splash_resume_applied && !oobe_is_active()) GUI.pending_boot_main_ready = true;
  }
}

//==================== صفحه اصلی: دایره سفید به‌جای قوس آبی ====================
static lv_obj_t *main_pos_disk = NULL;
static lv_obj_t *main_pos_digit_lbl[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
static char main_pos_digit_cache[7] = "";
static int main_pos_digit_len = -1;
static uint32_t main_pos_digit_color = 0xb90000;

static bool main_position_digits_ready() {
  return false;
}

static void main_position_digits_set_color(uint32_t color) {
  main_pos_digit_color = color;
}

static void main_position_digits_layout(int len) {
  if (!main_position_digits_ready()) return;
  const int slot_w = 88;
  const int slot_h = 155;
  const int y = 84;
  int total_w = slot_w * len;
  int x0 = (480 - total_w) / 2;
  if (x0 < 8) x0 = 8;
  for (int i = 0; i < 6; i++) {
    lv_obj_set_pos(main_pos_digit_lbl[i], x0 + i * slot_w, y);
    lv_obj_set_size(main_pos_digit_lbl[i], slot_w, slot_h);
  }
}

static bool main_position_digits_set_text(const char *txt) {
  (void)txt;
  return false;
}

static void main_reflow_position_disk() {
  if (!objects.lable1_2) return;
  if (!main1_safe_mode) {
    if (objects.obj1) lv_obj_add_flag(objects.obj1, LV_OBJ_FLAG_HIDDEN);
    if (objects.arc1_1) {
      lv_obj_add_flag(objects.arc1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.arc1_1, LV_OBJ_FLAG_CLICKABLE);
    }
    return;
  }
  if (main_pos_disk) {
    lv_obj_delete(main_pos_disk);
    main_pos_disk = NULL;
  }
  // قوس آبی گیج را بردار
  if (objects.arc1_1) {
    lv_obj_add_flag(objects.arc1_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.arc1_1, LV_OBJ_FLAG_CLICKABLE);
  }
  if (objects.panel1_2) {
    lv_obj_set_pos(objects.panel1_2, 111, 3);  // مقصد ۲px دیگر بالا (مالک: قبلی پایین آمده بود)
    lv_obj_set_size(objects.panel1_2, 250, 83);
    lv_obj_set_style_radius(objects.panel1_2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.panel1_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(objects.panel1_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.lable1_1) {
    // مقصد پاک‌شدهٔ عمدی (اتمام صف): لیبل پنهان بماند؛ حین تایپ خالی هرگز
    const bool dest_cleared =
        (GUI.inputBuffer_sp.length() == 0 && !GUI.edit_mode_lable1_1 && !GUI.NumLock &&
         objects.lable1_1 && lv_obj_has_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN));
    if (!dest_cleared) lv_obj_clear_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(objects.lable1_1, 72, 2);  // مقصد بالا (Y کمتر)
    lv_obj_set_size(objects.lable1_1, 307, 82);
    lv_obj_set_style_transform_scale_x(objects.lable1_1, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(objects.lable1_1, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.lable1_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(objects.lable1_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(objects.lable1_1, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(objects.lable1_1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(objects.lable1_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(objects.lable1_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(objects.lable1_1, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(objects.lable1_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(objects.lable1_1, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  }
  lv_obj_set_pos(objects.lable1_2, 5, 78);
  lv_obj_set_size(objects.lable1_2, 470, 167);
  lv_obj_set_style_pad_right(objects.lable1_2, 35, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(objects.lable1_2, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(objects.lable1_2, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_label_set_long_mode(objects.lable1_2, LV_LABEL_LONG_MODE_CLIP);
  lv_obj_set_style_text_align(objects.lable1_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  main_pos_digit_len = -1;
  main_pos_digit_cache[0] = '\0';
  if (objects.obj4) {
    lv_obj_set_pos(objects.obj4, 364, 56);
    lv_obj_set_size(objects.obj4, 86, 34);
    lv_obj_set_style_text_font(objects.obj4, (GUI.system_language == 1) ? &ui_font_naz24 : &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.obj4, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.obj4, (GUI.system_language == 1) ? "\xD9\x85\xDB\x8C\xD9\x84\xDB\x8C \xD9\x85\xD8\xAA\xD8\xB1" : "mm");
  }
  if (objects.obj3) {
    lv_obj_set_pos(objects.obj3, 311, (AUT.selected_count_val == -1) ? 260 : 266);  // −۵ چپ
    lv_obj_set_size(objects.obj3, 78, 34);
    lv_obj_add_flag(objects.obj3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_align(objects.obj3, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (GUI.system_language == 1) {
      lv_obj_set_style_text_font(objects.obj3, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(objects.obj3, "\xD8\xAA\xD8\xB9\xD8\xAF\xD8\xA7\xD8\xAF");
    } else {
      lv_obj_set_style_text_font(objects.obj3, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(objects.obj3, "PCS");
    }
  }
  if (objects.lable1_3) {
    lv_obj_set_pos(objects.lable1_3, 112, (AUT.selected_count_val == -1) ? 236 : 239);
    lv_obj_add_flag(objects.lable1_3, LV_OBJ_FLAG_CLICKABLE);
  }
  if (objects.obj2) {
    lv_obj_set_pos(objects.obj2, 141, (AUT.selected_count_val == -1) ? 253 : 259);
    lv_obj_add_flag(objects.obj2, LV_OBJ_FLAG_CLICKABLE);
  }
  // عدد موقعیت روی دیسک بماند
  lv_obj_move_foreground(objects.lable1_2);
  for (int i = 0; i < 6; i++) {
    if (main_pos_digit_lbl[i]) lv_obj_move_foreground(main_pos_digit_lbl[i]);
  }
  if (objects.panel1_2) lv_obj_move_foreground(objects.panel1_2);
  if (objects.lable1_1) lv_obj_move_foreground(objects.lable1_1);
  if (objects.lable1_3) lv_obj_move_foreground(objects.lable1_3);
  if (objects.obj4) lv_obj_move_foreground(objects.obj4);
  if (objects.obj3) lv_obj_move_foreground(objects.obj3);
  if (objects.btn1_1) lv_obj_move_foreground(objects.btn1_1);
  if (objects.btn1_2) lv_obj_move_foreground(objects.btn1_2);
  // مثبت: دیسک سبز ثابت؛ آیکن + یک پیکسل راست‌تر و چرخش با تاچ
  {
    static lv_obj_t *plus_disc = NULL;
    if ((!plus_disc || !lv_obj_is_valid(plus_disc)) && objects.main_1) {
      plus_disc = lv_obj_create(objects.main_1);
      lv_obj_set_size(plus_disc, 46, 46);
      lv_obj_set_pos(plus_disc, 3, 2);  // ثابت
      lv_obj_clear_flag(plus_disc, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
      lv_obj_set_style_radius(plus_disc, 23, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(plus_disc, lv_color_hex(0x1C9B67), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(plus_disc, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(plus_disc, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(plus_disc, lv_color_hex(0xA7F3D0), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(plus_disc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (plus_disc && lv_obj_is_valid(plus_disc)) {
      lv_obj_set_pos(plus_disc, 3, 2);
      lv_obj_move_foreground(plus_disc);
      if (objects.btn1_1) lv_obj_move_foreground(objects.btn1_1);
    }
  }
  if (objects.btn1_1) {
    lv_obj_set_pos(objects.btn1_1, 1, 0);  // +۱px راست؛ دیسک ثابت
    lv_obj_set_style_image_recolor(objects.btn1_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(objects.btn1_1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(objects.btn1_1, lv_color_hex(0xe8ffe8), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_image_recolor_opa(objects.btn1_1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_pivot_x(objects.btn1_1, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(objects.btn1_1, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(objects.btn1_1, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(objects.btn1_1, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(objects.btn1_1, 275, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(objects.btn1_1, 275, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  // تنظیمات: چرخش فقط با انیم تاچ (زاویه را اینجا ریست نکن)
  if (objects.btn1_2) {
    lv_obj_set_style_image_recolor(objects.btn1_2, lv_color_hex(0x26384d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(objects.btn1_2, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(objects.btn1_2, lv_color_hex(0x1f2f42), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_image_recolor_opa(objects.btn1_2, LV_OPA_60, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_pivot_x(objects.btn1_2, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(objects.btn1_2, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(objects.btn1_2, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(objects.btn1_2, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(objects.btn1_2, 270, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(objects.btn1_2, 270, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  if (objects.btn1_3) lv_obj_move_foreground(objects.btn1_3);
  if (objects.btn1_4) lv_obj_move_foreground(objects.btn1_4);
  if (objects.btn1_5) lv_obj_move_foreground(objects.btn1_5);
  // فلش حرکت: ساده بدون scale — همان موقعیت prcs_dir_anim
  if (objects.anim1_2) lv_obj_set_pos(objects.anim1_2, 71, 237);
  if (objects.anim1_1) lv_obj_set_pos(objects.anim1_1, 371, 237);
  opt_main_bar_lbl_sync();
  jog_manual_badge_sync();
  blade_err_badge_sync();
  // دیالوگ اتمام/خطا باید بالای ارقام و کروم بماند
  if (objects.panel1_3 && !lv_obj_has_flag(objects.panel1_3, LV_OBJ_FLAG_HIDDEN))
    lv_obj_move_foreground(objects.panel1_3);
  if (objects.panel1_1 && !lv_obj_has_flag(objects.panel1_1, LV_OBJ_FLAG_HIDDEN))
    lv_obj_move_foreground(objects.panel1_1);
}

//==================== لیست: ۱۰ ردیف داخل پنل + هدر بدون هم‌پوشانی ====================
static lv_obj_t *list_row_panel(int i) {
  switch (i) {
    case 0: return objects.panel3_1_1; case 1: return objects.panel3_1_2;
    case 2: return objects.panel3_1_3; case 3: return objects.panel3_1_4;
    case 4: return objects.panel3_1_5; case 5: return objects.panel3_1_6;
    case 6: return objects.panel3_1_7; case 7: return objects.panel3_1_8;
    case 8: return objects.panel3_1_9; case 9: return objects.panel3_1_10;
    default: return NULL;
  }
}
static lv_obj_t *list_idx_panel(int i) {
  switch (i) {
    case 0: return objects.panel3_2_1; case 1: return objects.panel3_2_2;
    case 2: return objects.panel3_2_3; case 3: return objects.panel3_2_4;
    case 4: return objects.panel3_2_5; case 5: return objects.panel3_2_6;
    case 6: return objects.panel3_2_7; case 7: return objects.panel3_2_8;
    case 8: return objects.panel3_2_9; case 9: return objects.panel3_2_10;
    default: return NULL;
  }
}
static lv_obj_t *list_sp_label(int i) {
  switch (i) {
    case 0: return objects.lable3_1_1; case 1: return objects.lable3_1_2;
    case 2: return objects.lable3_1_3; case 3: return objects.lable3_1_4;
    case 4: return objects.lable3_1_5; case 5: return objects.lable3_1_6;
    case 6: return objects.lable3_1_7; case 7: return objects.lable3_1_8;
    case 8: return objects.lable3_1_9; case 9: return objects.lable3_1_10;
    default: return NULL;
  }
}
static lv_obj_t *list_cnt_label(int i);  // fwd
static void list_blank_dismiss_kb_cb(lv_event_t *e) {
  // فقط CLICKED — با RELEASED سوایپ/اعداد تداخل نکند
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  // درگ اسکرول: CLICK بعد از درگ معمولاً نمی‌آید؛ اگر آمد نبند
  if (list_press_moved) return;
  if (GUI.scr_id != 3) return;
  bool kb_open = (numpad_ctx == 2 && numpad_cont && !lv_obj_has_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN));
  if (!kb_open && !list_edit_active()) return;

  lv_obj_t *t = (lv_obj_t *)lv_event_get_target(e);
  if (!t) return;
  if (numpad_cont) {
    for (lv_obj_t *c = t; c; c = lv_obj_get_parent(c)) {
      if (c == numpad_cont) return;
    }
  }
  if (list_bs_overlay) {
    for (lv_obj_t *c = t; c; c = lv_obj_get_parent(c)) {
      if (c == list_bs_overlay) return;
    }
  }
  for (int i = 0; i < 10; i++) {
    lv_obj_t *sp = list_sp_label(i);
    lv_obj_t *cn = list_cnt_label(i);
    for (lv_obj_t *c = t; c; c = lv_obj_get_parent(c)) {
      if ((sp && c == sp) || (cn && c == cn)) return;
    }
  }
  cancel_screen3_pending_edit();
  numpad_close();
  list_reflow_compact();
}

static void list_bind_swipe_target(lv_obj_t *o) {
  if (!o) return;
  lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
  // هر سه رویداد لازم است؛ فقط PRESSED اسکرول را جلو نمی‌برد
  lv_obj_add_event_cb(o, list_swipe_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(o, list_swipe_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(o, list_swipe_cb, LV_EVENT_RELEASED, NULL);
}

static void list_bind_dismiss_target(lv_obj_t *o) {
  if (!o) return;
  lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(o, list_blank_dismiss_kb_cb, LV_EVENT_CLICKED, NULL);
  list_bind_swipe_target(o);
}

static lv_obj_t *list_cnt_label(int i) {
  switch (i) {
    case 0: return objects.lable3_2_1; case 1: return objects.lable3_2_2;
    case 2: return objects.lable3_2_3; case 3: return objects.lable3_2_4;
    case 4: return objects.lable3_2_5; case 5: return objects.lable3_2_6;
    case 6: return objects.lable3_2_7; case 7: return objects.lable3_2_8;
    case 8: return objects.lable3_2_9; case 9: return objects.lable3_2_10;
    default: return NULL;
  }
}
static lv_obj_t *list_edit_btn(int i) {
  switch (i) {
    case 0: return objects.btn3_1_1; case 1: return objects.btn3_1_2;
    case 2: return objects.btn3_1_3; case 3: return objects.btn3_1_4;
    case 4: return objects.btn3_1_5; case 5: return objects.btn3_1_6;
    case 6: return objects.btn3_1_7; case 7: return objects.btn3_1_8;
    case 8: return objects.btn3_1_9; case 9: return objects.btn3_1_10;
    default: return NULL;
  }
}
static lv_obj_t *list_x_label(int i) {
  switch (i) {
    case 0: return objects.obj24; case 1: return objects.obj26;
    case 2: return objects.obj28; case 3: return objects.obj30;
    case 4: return objects.obj32; case 5: return objects.obj34;
    case 6: return objects.obj36; case 7: return objects.obj38;
    case 8: return objects.obj40; case 9: return objects.obj42;
    default: return NULL;
  }
}

static void list_hdr_label_icon(lv_obj_t *lbl, const char *txt, const lv_font_t *font, int yoff) {
  if (!lbl || !txt) return;
  lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_x(lbl, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_y(lbl, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_x(lbl, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_y(lbl, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl, txt);
  lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(lbl, LV_ALIGN_CENTER, 0, yoff);
  lv_obj_move_foreground(lbl);
}

static void list_hdr_button_icon(lv_obj_t *btn, const char *txt, const lv_font_t *font, int yoff) {
  if (!btn) return;
  lv_obj_t *lbl = NULL;
  if (lv_obj_get_child_count(btn) > 0) lbl = lv_obj_get_child(btn, 0);
  if (!lbl) lbl = lv_label_create(btn);
  list_hdr_label_icon(lbl, txt, font, yoff);
}

static lv_obj_t *list_ang_btn[LIST_ROWS_PER_PAGE] = { NULL };

static void list_ang_btn_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  int row_slot = (int)(intptr_t)lv_event_get_user_data(e);
  int gidx = page_base() + row_slot;
  if (gidx < 0 || gidx >= LIST_MAX) return;
  uint8_t cur_ang = AUT.list_angle_Val[gidx];
  uint8_t new_ang = (cur_ang == 90) ? 45 : 90;
  AUT.list_angle_Val[gidx] = new_ang;
  save_permanently_preferences_angle_val(gidx, new_ang);
  if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  opt_notify_list_edited();
  list_apply_visible_values_now();
}

static lv_obj_t *list_get_ang_button(int slot) {
  if (slot < 0 || slot >= LIST_ROWS_PER_PAGE) return NULL;
  lv_obj_t *row = list_row_panel(slot);
  if (!row) return NULL;
  if (!list_ang_btn[slot] || !lv_obj_is_valid(list_ang_btn[slot]) ||
      lv_obj_get_parent(list_ang_btn[slot]) != row) {
    lv_obj_t *b = lv_button_create(row);
    lv_obj_set_pos(b, 49, 3);
    lv_obj_set_size(b, 46, 36);
    lv_obj_set_style_radius(b, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(b, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x007AFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(b, list_ang_btn_click_cb, LV_EVENT_RELEASED, (void *)(intptr_t)slot);

    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(l, "45°");
    list_ang_btn[slot] = b;
  }
  return list_ang_btn[slot];
}

void list_sync_index_labels() {
  for (int i = 0; i < LIST_ROWS_PER_PAGE; i++) {
    lv_obj_t *idxp = list_idx_panel(i);
    if (!idxp || lv_obj_get_child_count(idxp) == 0) continue;
    lv_obj_t *il = lv_obj_get_child(idxp, 0);
    int gidx = GUI.list_page + i;
    if (gidx < 0 || gidx >= LIST_MAX) {
      lv_label_set_text(il, "");
    } else {
      lv_label_set_text_fmt(il, "%02d", gidx + 1);
    }
  }
}

static void list_ensure_xmark(lv_obj_t *anchor, lv_obj_t **out_mark) {
  if (!anchor || !out_mark) return;
  lv_obj_t *mk = *out_mark;
  if (mk && lv_obj_is_valid(mk)) return;
  lv_obj_t *par = lv_obj_get_parent(anchor);
  if (!par) return;
  mk = lv_label_create(par);
  // کوچک و خیلی کم‌رنگ — عدد زیر باید خوانا بماند
  lv_obj_set_style_text_font(mk, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(mk, lv_color_hex(0xAEAEB2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(mk, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(mk, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(mk, LV_SYMBOL_CLOSE);
  lv_obj_add_flag(mk, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(mk, LV_OBJ_FLAG_FLOATING);
  *out_mark = mk;
}

static void list_place_xmark(lv_obj_t *mk, lv_obj_t *anchor) {
  if (!mk || !anchor || !lv_obj_is_valid(mk) || !lv_obj_is_valid(anchor)) return;
  lv_obj_set_size(mk, 22, 22);
  lv_obj_set_style_text_font(mk, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(mk, lv_color_hex(0xAEAEB2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(mk, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align_to(mk, anchor, LV_ALIGN_CENTER, 0, -3);  // ۳px بالا
  lv_obj_clear_flag(mk, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(mk);
}

// نمایش تعداد/اندازه + خط‌خوردگی/ضربدر ردیف تمام — از Task2 هم صدا زده می‌شود
void list_apply_visible_values_now() {
  static lv_obj_t *list_sp_xmark[LIST_ROWS_PER_PAGE] = {};
  static lv_obj_t *list_cn_xmark[LIST_ROWS_PER_PAGE] = {};
  static lv_obj_t *list_was_xmark[LIST_ROWS_PER_PAGE] = {};
  static lv_obj_t *list_full_cut_lbl[LIST_ROWS_PER_PAGE] = {};
  const bool fa = (GUI.system_language == 1);
  for (int i = 0; i < LIST_ROWS_PER_PAGE; i++) {
    int gidx = page_base() + i;
    lv_obj_t *sp = list_sp_label(i);
    lv_obj_t *cn = list_cnt_label(i);
    lv_obj_t *row = list_row_panel(i);
    const bool valid = (gidx >= 0 && gidx < LIST_MAX);
    const float spv = valid ? AUT.list_sp_Val_p[gidx] : 0.0f;
    const int cur = valid ? AUT.list_count_Val[gidx] : 0;
    int base = valid ? AUT.list_count_base_Val[gidx] : 0;
    if (base < cur) base = cur;
    const bool editing = valid &&
        (AUT.list_edit_mode_sp[gidx] || AUT.list_edit_mode_count[gidx]);
    // صفرِ تازه‌واردشده «تمام‌شده» نیست؛ مبنا باید واقعاً از برش بالاتر از جاری مانده باشد.
    const bool depleted = valid && !editing && spv > 0.05f &&
                          base > 0 && cur == 0 && base > cur;
    const bool show_was = valid && !editing && !depleted &&
                          base > cur && base > 0;

    if (sp) {
      list_ensure_xmark(sp, &list_sp_xmark[i]);
      if (valid) {
        String &txt = GUI.inputBuffer_L_3_1_x[gidx];
        lv_label_set_text(sp, (txt.length() > 0) ? txt.c_str() : "0");
      } else {
        lv_label_set_text(sp, "0");
      }
      // عدد خوانا بماند؛ فقط خط نازک + ضربدر خیلی کم‌رنگ
      lv_obj_set_style_text_opa(sp, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(sp, lv_color_hex(depleted ? 0x636366 : 0x212121),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_decor(sp, depleted ? LV_TEXT_DECOR_STRIKETHROUGH : LV_TEXT_DECOR_NONE,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      if (depleted && list_sp_xmark[i]) list_place_xmark(list_sp_xmark[i], sp);
      else if (list_sp_xmark[i]) lv_obj_add_flag(list_sp_xmark[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (cn) {
      // لیبل تعداد اولیهٔ برش‌خورده (کنار باقی‌مانده)
      lv_obj_t *was = (lv_obj_t *)lv_obj_get_user_data(cn);
      if (!was || !lv_obj_is_valid(was)) {
        lv_obj_t *par = lv_obj_get_parent(cn);
        if (par) {
          was = lv_label_create(par);
          lv_obj_set_style_text_font(was, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_set_style_text_color(was, lv_color_hex(0x8E8E93), LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_set_style_text_decor(was, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_set_style_text_opa(was, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_set_style_text_align(was, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_add_flag(was, LV_OBJ_FLAG_HIDDEN);
          lv_obj_set_user_data(cn, was);
        }
      }
      list_ensure_xmark(cn, &list_cn_xmark[i]);
      if (was) list_ensure_xmark(was, &list_was_xmark[i]);
      if (depleted) {
        lv_obj_set_pos(cn, 268, 5);
        lv_obj_set_size(cn, 138, 30);
        char b[12];
        snprintf(b, sizeof(b), "%d", base > 0 ? base : 0);
        lv_label_set_text(cn, b);
        lv_obj_set_style_text_decor(cn, LV_TEXT_DECOR_STRIKETHROUGH, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(cn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(cn, lv_color_hex(0x636366), LV_PART_MAIN | LV_STATE_DEFAULT);
        if (was) lv_obj_add_flag(was, LV_OBJ_FLAG_HIDDEN);
        if (list_cn_xmark[i]) list_place_xmark(list_cn_xmark[i], cn);
        if (list_was_xmark[i]) lv_obj_add_flag(list_was_xmark[i], LV_OBJ_FLAG_HIDDEN);
      } else if (valid) {
        // وقتی تعداد اولیه هم نمایش داده می‌شود، دو عدد ناحیهٔ جدا داشته باشند.
        // جاری زیر ستون تعداد؛ اولیهٔ خط‌خورده در ستون راست آن.
        lv_obj_set_pos(cn, show_was ? 297 : 268, 5);
        lv_obj_set_size(cn, show_was ? 82 : 138, 30);
        if (editing && AUT.list_edit_mode_count[gidx]) {
          lv_label_set_text(cn, GUI.inputBuffer_L_3_2_x[gidx].c_str());
        } else {
          char b[12];
          snprintf(b, sizeof(b), "%d", cur);
          lv_label_set_text(cn, b);
        }
        lv_obj_set_style_text_decor(cn, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(cn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(cn, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
        if (list_cn_xmark[i]) lv_obj_add_flag(list_cn_xmark[i], LV_OBJ_FLAG_HIDDEN);
        if (was && show_was) {
          lv_obj_set_size(was, 56, 28);
          lv_obj_set_pos(was, 380, 7);  // ستون مقدار اولیه در راست — هم‌راستا با رقم جاری
          lv_label_set_text_fmt(was, "%d", base);
          lv_obj_set_style_text_decor(was, LV_TEXT_DECOR_STRIKETHROUGH, LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_set_style_text_opa(was, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
          lv_obj_set_style_text_color(was, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT); // مشکی
          lv_obj_clear_flag(was, LV_OBJ_FLAG_HIDDEN);
          if (list_was_xmark[i]) list_place_xmark(list_was_xmark[i], was);
        } else if (was) {
          lv_obj_add_flag(was, LV_OBJ_FLAG_HIDDEN);
          if (list_was_xmark[i]) lv_obj_add_flag(list_was_xmark[i], LV_OBJ_FLAG_HIDDEN);
        }
      } else {
        lv_obj_set_pos(cn, 268, 5);
        lv_obj_set_size(cn, 138, 30);
        lv_label_set_text(cn, "0");
        if (was) lv_obj_add_flag(was, LV_OBJ_FLAG_HIDDEN);
        if (list_cn_xmark[i]) lv_obj_add_flag(list_cn_xmark[i], LV_OBJ_FLAG_HIDDEN);
        if (list_was_xmark[i]) lv_obj_add_flag(list_was_xmark[i], LV_OBJ_FLAG_HIDDEN);
      }
    }
    const bool is_active_row = (AUT.selected_count_val >= 0 && gidx == AUT.selected_count_val);
    if (row && lv_obj_is_valid(row)) {
      uint32_t row_c = is_active_row ? 0xFFF9C4u : (depleted ? 0xF2F2F7u : ((i & 1) ? 0xf7f8fbu : 0xffffffu));
      lv_obj_set_style_bg_color(row, lv_color_hex(row_c), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      // گوشهٔ راست: «تعداد اولیه» — مشکی و هم‌راستا با نوشته سمت چپ
      if (!list_full_cut_lbl[i] || !lv_obj_is_valid(list_full_cut_lbl[i]) ||
          lv_obj_get_parent(list_full_cut_lbl[i]) != row) {
        list_full_cut_lbl[i] = lv_label_create(row);
        lv_obj_set_style_text_font(list_full_cut_lbl[i], fa ? &ui_font_naz18b : &lv_font_montserrat_12,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(list_full_cut_lbl[i], lv_color_hex(0x000000), // رنگ مشکی
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(list_full_cut_lbl[i], LV_TEXT_ALIGN_RIGHT,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_base_dir(list_full_cut_lbl[i], fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(list_full_cut_lbl[i], fa ? "تعداد اولیه" : "Initial Qty");
        lv_obj_add_flag(list_full_cut_lbl[i], LV_OBJ_FLAG_HIDDEN);
      }
      if (depleted && !is_active_row) {
        lv_obj_set_style_text_font(list_full_cut_lbl[i], fa ? &ui_font_naz18b : &lv_font_montserrat_12,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(list_full_cut_lbl[i], lv_color_hex(0x000000), // رنگ مشکی
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(list_full_cut_lbl[i], fa ? "تعداد اولیه" : "Initial Qty");
        lv_obj_align(list_full_cut_lbl[i], LV_ALIGN_RIGHT_MID, -9, 2); // هم‌راستا با خط زیرین متراژ
        lv_obj_clear_flag(list_full_cut_lbl[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(list_full_cut_lbl[i]);
      } else {
        lv_obj_add_flag(list_full_cut_lbl[i], LV_OBJ_FLAG_HIDDEN);
      }
    }
    lv_obj_t *idxp = list_idx_panel(i);
    if (idxp && lv_obj_is_valid(idxp)) {
      // ردیف فعال انتخابی = زرد؛ تمام‌شده = خاکستری؛ عادی = سبز
      uint32_t idx_c = is_active_row ? 0xFFD600u : (depleted ? 0x8E8E93u : 0x34C759u);
      lv_obj_set_style_bg_color(idxp, lv_color_hex(idx_c), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (lv_obj_get_child_count(idxp) > 0) {
        lv_obj_t *il = lv_obj_get_child(idxp, 0);
        lv_obj_set_style_text_color(il, lv_color_hex(is_active_row ? 0x1C1C1E : 0xffffff),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    lv_obj_t *ang_b = list_get_ang_button(i);
    if (ang_b && lv_obj_is_valid(ang_b)) {
      uint8_t ang = valid ? AUT.list_angle_Val[gidx] : 45;
      if (ang != 90) ang = 45;
      uint32_t ang_c = depleted ? 0x8E8E93u : (ang == 90 ? 0x5856D6u : 0x007AFFu);
      lv_obj_set_style_bg_color(ang_b, lv_color_hex(ang_c), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (lv_obj_get_child_count(ang_b) > 0) {
        lv_obj_t *al = lv_obj_get_child(ang_b, 0);
        lv_label_set_text(al, ang == 90 ? "90°" : "45°");
      }
      lv_obj_clear_flag(ang_b, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // بررسی وجود حداقل یک برش در صفحهٔ جاری جهت نمایش عنوان «تعداد اولیه»
  bool has_was = false;
  for (int i = 0; i < LIST_ROWS_PER_PAGE; i++) {
    int gidx = page_base() + i;
    if (gidx >= 0 && gidx < LIST_MAX) {
      const bool editing = (AUT.list_edit_mode_sp[gidx] || AUT.list_edit_mode_count[gidx]);
      const int cur = AUT.list_count_Val[gidx];
      int base = AUT.list_count_base_Val[gidx];
      if (base < cur) base = cur;
      const bool depleted = !editing && AUT.list_sp_Val_p[gidx] > 0.05f && base > 0 && cur == 0 && base > cur;
      const bool show_was = !editing && !depleted && base > cur && base > 0;
      if (show_was) {
        has_was = true;
        break;
      }
    }
  }
  const bool en = (GUI.system_language != 1);
  if (list_initial_count_header_lbl && lv_obj_is_valid(list_initial_count_header_lbl)) {
    if (has_was) {
      lv_obj_clear_flag(list_initial_count_header_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(list_initial_count_header_lbl);
      if (objects.lablef3_2 && lv_obj_is_valid(objects.lablef3_2)) {
        lv_obj_set_pos(objects.lablef3_2, 290, en ? 54 : 52);
        lv_obj_set_size(objects.lablef3_2, 90, 30);
      }
    } else {
      lv_obj_add_flag(list_initial_count_header_lbl, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef3_2 && lv_obj_is_valid(objects.lablef3_2)) {
        lv_obj_set_pos(objects.lablef3_2, 268, en ? 54 : 52);
        lv_obj_set_size(objects.lablef3_2, 138, 30);
      }
    }
  }
}

static void list_reflow_compact() {
  if (!objects.panel3_1 || !objects.obj22) return;

  // هندسهٔ والد: تا لبهٔ پایین صفحه بپوشاند (شکاف خاکستری زرد)
  lv_obj_set_style_pad_all(objects.obj22, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(objects.obj22, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(objects.obj22, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(objects.obj22, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_clip_corner(objects.obj22, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(objects.obj22, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  // EEZ: y=-13 h=311 → پایین صفحه خالی می‌ماند؛ تا y=320 پر کن
  lv_obj_set_pos(objects.obj22, -13, -13);
  lv_obj_set_size(objects.obj22, 471, 333);  // -13+333=320

  if (objects.lablef3_0) {
    lv_obj_t *title_box = lv_obj_get_parent(objects.lablef3_0);
    const bool en = (GUI.system_language != 1);
    if (title_box) {
      // هدر: خانه | راهنما | عنوان | +/− | بهینه
      lv_obj_set_pos(title_box, 112, 10);
      lv_obj_set_size(title_box, 206, 34);
      lv_obj_set_style_pad_all(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      // پوسته شفاف: متن naz بیرون می‌زند؛ گردی روی لایهٔ سفید جدا (مثل مربع سبز ردیف)
      lv_obj_set_style_bg_opa(title_box, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_clip_corner(title_box, false, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_outline_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_clear_flag(title_box, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(title_box, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
      lv_obj_add_flag(title_box, LV_OBJ_FLAG_CLICKABLE);

      static lv_obj_t *list_title_bg = NULL;
      static bool list_title_bg_styled = false;
      if (!list_title_bg || !lv_obj_is_valid(list_title_bg) || lv_obj_get_parent(list_title_bg) != title_box) {
        list_title_bg = lv_obj_create(title_box);
        list_title_bg_styled = false;
        lv_obj_clear_flag(list_title_bg, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(list_title_bg, LV_OBJ_FLAG_SCROLLABLE);
      }
      if (!list_title_bg_styled) {
        // تم پیش‌فرض LVGL گوشه را تیز نگه می‌دارد — استایل را کامل پاک کن
        lv_obj_remove_style_all(list_title_bg);
        list_title_bg_styled = true;
      }
      lv_obj_set_pos(list_title_bg, 0, 0);
      lv_obj_set_size(list_title_bg, 206, 34);
      lv_obj_set_style_pad_all(list_title_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(list_title_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_outline_width(list_title_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(list_title_bg, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(list_title_bg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      // گوشهٔ بالا چپ/راست — همان گردی مربع سبز ردیف (radius ۱۰)
      lv_obj_set_style_radius(list_title_bg, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_clip_corner(list_title_bg, true, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(list_title_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_move_to_index(list_title_bg, 0);  // پشت نوشته
    }
    // FA: y=-5 (naz30). EN montserrat_26 در کادر ۳۴px: وسط ≈ y=4
    lv_obj_set_pos(objects.lablef3_0, 0, en ? 4 : -5);
    lv_obj_set_size(objects.lablef3_0, 206, en ? 34 : 45);
    lv_obj_set_style_text_font(objects.lablef3_0, en ? &lv_font_montserrat_26 : &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(objects.lablef3_0, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(objects.lablef3_0, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_x(objects.lablef3_0, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(objects.lablef3_0, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.lablef3_0, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(objects.lablef3_0, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablef3_0, en ? "Cut List" : "لیست برش");
    lv_obj_clear_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.lablef3_0);
    if (objects.lablee3_0) lv_obj_add_flag(objects.lablee3_0, LV_OBJ_FLAG_HIDDEN);
  }

  if (objects.btn3_1) {
    lv_obj_set_pos(objects.btn3_1, 3, 2);
    lv_obj_set_size(objects.btn3_1, 44, 39);
    lv_obj_set_style_clip_corner(objects.btn3_1, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(objects.btn3_1, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    f1_style_hdr_btn(objects.btn3_1, 0x0a84ff);
    lv_obj_set_style_bg_color(objects.btn3_1, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.btn3_1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    list_hdr_button_icon(objects.btn3_1, LV_SYMBOL_HOME, &lv_font_montserrat_26, 0);
  }
  if (btn_list_help) {
    lv_obj_set_pos(btn_list_help, 55, 2);
    lv_obj_set_size(btn_list_help, 44, 39);
    f1_style_hdr_btn(btn_list_help, 0x0a84ff);
    if (lv_obj_get_child_count(btn_list_help) > 0) {
      lv_obj_t *lh = lv_obj_get_child(btn_list_help, 0);
      lv_obj_set_style_text_color(lh, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(lh, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(lh, "");
      lv_obj_center(lh);
    }
    f1_hdr_set_book_icon(btn_list_help);
  }
  if (btn_page_prev) lv_obj_add_flag(btn_page_prev, LV_OBJ_FLAG_HIDDEN);
  if (page_lbl) lv_obj_add_flag(page_lbl, LV_OBJ_FLAG_HIDDEN);
  if (btn_page_next) lv_obj_add_flag(btn_page_next, LV_OBJ_FLAG_HIDDEN);
  if (list_header_line) {
    lv_obj_set_pos(list_header_line, 0, 49);
    lv_obj_set_size(list_header_line, 480, 2);
    lv_obj_set_style_bg_color(list_header_line, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(list_header_line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(list_header_line);
  }

  if (btn_optimize) {
    lv_obj_set_pos(btn_optimize, 425, 2);
    lv_obj_set_size(btn_optimize, 44, 39);
    lv_obj_set_style_clip_corner(btn_optimize, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_optimize, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    f1_style_hdr_btn(btn_optimize, 0xef6c00);
    list_hdr_button_icon(btn_optimize, LV_SYMBOL_SHUFFLE, &lv_font_montserrat_26, 0);
  }
  if (btn_list_clear) lv_obj_add_flag(btn_list_clear, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn3_2) lv_obj_add_flag(objects.btn3_2, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn3_3) lv_obj_add_flag(objects.btn3_3, LV_OBJ_FLAG_HIDDEN);
  if (btn_list_add_round) {
    lv_obj_set_pos(btn_list_add_round, 321, 2);
    lv_obj_set_size(btn_list_add_round, 44, 39);
    lv_obj_set_style_clip_corner(btn_list_add_round, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_list_add_round, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_radius(btn_list_add_round, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    // ظاهر سه‌بعدی نرم: هایلایت بالا + سایه پایین
    lv_obj_set_style_bg_color(btn_list_add_round, lv_color_hex(0x12b35a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btn_list_add_round, lv_color_hex(0x007a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btn_list_add_round, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_list_add_round, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn_list_add_round, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn_list_add_round, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_list_add_round, lv_color_hex(0x007a3a), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(btn_list_add_round, lv_color_hex(0x005528), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_pivot_x(btn_list_add_round, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(btn_list_add_round, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(btn_list_add_round, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(btn_list_add_round, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_x(btn_list_add_round, 288, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_list_add_round, 288, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_flag(btn_list_add_round, LV_OBJ_FLAG_CLICKABLE);
    list_hdr_button_icon(btn_list_add_round, "+", &lv_font_montserrat_38, -2);
    if (lv_obj_get_child_count(btn_list_add_round) > 0) {
      lv_obj_t *pl = lv_obj_get_child(btn_list_add_round, 0);
      lv_obj_set_style_text_color(pl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_opa(pl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_move_foreground(btn_list_add_round);
  }
  if (btn_list_remove_round) {
    lv_obj_set_pos(btn_list_remove_round, 373, 2);
    lv_obj_set_size(btn_list_remove_round, 44, 39);
    lv_obj_set_style_clip_corner(btn_list_remove_round, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_list_remove_round, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_radius(btn_list_remove_round, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_list_remove_round, lv_color_hex(0xff3b30), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_list_remove_round, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn_list_remove_round, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_list_remove_round, LV_OBJ_FLAG_CLICKABLE);
    list_hdr_button_icon(btn_list_remove_round, "-", &lv_font_montserrat_38, -3);
    lv_obj_move_foreground(btn_list_remove_round);
  }

  {
    static lv_obj_t *list_row_header_lbl = NULL;
    const bool en = (GUI.system_language != 1);
    if (!list_row_header_lbl || !lv_obj_is_valid(list_row_header_lbl) ||
        lv_obj_get_parent(list_row_header_lbl) != objects.obj22) {
      list_row_header_lbl = lv_label_create(objects.obj22);
      lv_obj_add_flag(list_row_header_lbl, LV_OBJ_FLAG_CLICKABLE);
      list_bind_dismiss_target(list_row_header_lbl);
    }
    lv_obj_set_pos(list_row_header_lbl, 4, en ? 54 : 50);
    lv_obj_set_size(list_row_header_lbl, 40, 30);
    lv_obj_set_style_text_font(list_row_header_lbl, en ? &lv_font_montserrat_18 : &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(list_row_header_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(list_row_header_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(list_row_header_lbl, en ? "Row" : "ردیف");
    lv_obj_clear_flag(list_row_header_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(list_row_header_lbl);
  }

  {
    static lv_obj_t *list_ang_header_lbl = NULL;
    const bool en = (GUI.system_language != 1);
    if (!list_ang_header_lbl || !lv_obj_is_valid(list_ang_header_lbl) ||
        lv_obj_get_parent(list_ang_header_lbl) != objects.obj22) {
      list_ang_header_lbl = lv_label_create(objects.obj22);
      lv_obj_add_flag(list_ang_header_lbl, LV_OBJ_FLAG_CLICKABLE);
      list_bind_dismiss_target(list_ang_header_lbl);
    }
    lv_obj_set_pos(list_ang_header_lbl, 49, en ? 54 : 50);
    lv_obj_set_size(list_ang_header_lbl, 46, 30);
    lv_obj_set_style_text_font(list_ang_header_lbl, en ? &lv_font_montserrat_18 : &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(list_ang_header_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(list_ang_header_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(list_ang_header_lbl, en ? "Angle" : "زاویه");
    lv_obj_clear_flag(list_ang_header_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(list_ang_header_lbl);
  }

  if (objects.lablef3_1) {
    const bool en = (GUI.system_language != 1);
    lv_obj_set_pos(objects.lablef3_1, 96, en ? 54 : 50);
    lv_obj_set_size(objects.lablef3_1, 144, 30);
    lv_obj_set_style_text_font(objects.lablef3_1, en ? &lv_font_montserrat_18 : &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.lablef3_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(objects.lablef3_1, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablef3_1, en ? "Length (mm)" : "متراژ (میلی متر)");
    lv_obj_add_flag(objects.lablef3_1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablee3_1) lv_obj_add_flag(objects.lablee3_1, LV_OBJ_FLAG_HIDDEN);
  if (list_meter_unit_lbl) {
    lv_obj_add_flag(list_meter_unit_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablef3_2) {
    const bool en = (GUI.system_language != 1);
    lv_obj_set_pos(objects.lablef3_2, 268, en ? 54 : 52);
    lv_obj_set_size(objects.lablef3_2, 138, 30);
    lv_obj_set_style_text_font(objects.lablef3_2, en ? &lv_font_montserrat_18 : &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.lablef3_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(objects.lablef3_2, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablef3_2, en ? "Quantity" : "تعداد");
    lv_obj_add_flag(objects.lablef3_2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablee3_2) lv_obj_add_flag(objects.lablee3_2, LV_OBJ_FLAG_HIDDEN);

  // هدر ستون تعداد اولیه (فقط وقتی برش انجام شده ظاهر می‌شود)
  if (!list_initial_count_header_lbl || !lv_obj_is_valid(list_initial_count_header_lbl) ||
      lv_obj_get_parent(list_initial_count_header_lbl) != objects.obj22) {
    list_initial_count_header_lbl = lv_label_create(objects.obj22);
    lv_obj_clear_flag(list_initial_count_header_lbl, LV_OBJ_FLAG_SCROLLABLE);
  }
  {
    const bool en = (GUI.system_language != 1);
    lv_obj_set_pos(list_initial_count_header_lbl, 376, en ? 54 : 50);
    lv_obj_set_size(list_initial_count_header_lbl, 66, 30);
    lv_obj_set_style_text_font(list_initial_count_header_lbl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(list_initial_count_header_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(list_initial_count_header_lbl, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(list_initial_count_header_lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(list_initial_count_header_lbl, en ? "Initial" : "تعداد اولیه");
    lv_obj_add_flag(list_initial_count_header_lbl, LV_OBJ_FLAG_HIDDEN);
  }

  if (objects.lablef3_1) lv_obj_move_foreground(objects.lablef3_1);
  if (objects.lablef3_2) lv_obj_move_foreground(objects.lablef3_2);
  if (list_initial_count_header_lbl) lv_obj_move_foreground(list_initial_count_header_lbl);

  // پنل لیست: پوسته بدون کلیپ (مربع سبز ردیف ۱ بریده نشود)
  // گردی گوش بالا/پایین فقط روی لایهٔ سفید پشت + ردیف اول/آخر بدون clip_corner
  // با نامپد باز فقط ۲ ردیف بالای کیبرد (تا لمس زیر نامپد اسکرول را نبلعد)
  const int list_vis_rows = list_rows_on_screen();
  const int list_panel_h = (list_vis_rows <= 2) ? (2 * 44) : 245;  // 88 یا 245
  // catcher مقصد اصلی اگر گیر کرده باشد کل لیست را قفل می‌کند
  if (main_numpad_catcher) lv_obj_add_flag(main_numpad_catcher, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_pos(objects.panel3_1, 3, 88);
  lv_obj_set_size(objects.panel3_1, 464, list_panel_h);  // 88+245=333 = کف obj22
  lv_obj_set_style_pad_all(objects.panel3_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(objects.panel3_1, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(objects.panel3_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(objects.panel3_1, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(objects.panel3_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(objects.panel3_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(objects.panel3_1, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(objects.panel3_1, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_flag(objects.panel3_1, LV_OBJ_FLAG_CLICKABLE);

  {
    static lv_obj_t *list_panel_bg = NULL;
    static bool list_panel_bg_styled = false;
    if (!list_panel_bg || !lv_obj_is_valid(list_panel_bg) ||
        lv_obj_get_parent(list_panel_bg) != objects.panel3_1) {
      list_panel_bg = lv_obj_create(objects.panel3_1);
      list_panel_bg_styled = false;
      // کلیک‌پذیر تا فضای خالی زیر ردیف‌ها هم اسکرول بگیرد
      lv_obj_add_flag(list_panel_bg, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(list_panel_bg, LV_OBJ_FLAG_SCROLLABLE);
      list_bind_swipe_target(list_panel_bg);
    }
    if (!list_panel_bg_styled) {
      lv_obj_remove_style_all(list_panel_bg);
      list_panel_bg_styled = true;
      // بعد از remove_style_all فلگ‌ها را برگردان
      lv_obj_add_flag(list_panel_bg, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(list_panel_bg, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_set_pos(list_panel_bg, 0, 0);
    lv_obj_set_size(list_panel_bg, 464, list_panel_h);
    lv_obj_set_style_pad_all(list_panel_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list_panel_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(list_panel_bg, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(list_panel_bg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(list_panel_bg, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(list_panel_bg, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_to_index(list_panel_bg, 0);
  }
  static bool list_blank_cb_added = false;
  if (!list_blank_cb_added) {
    // لمس بیرون از اعداد → بستن کیبورد نرم
    list_bind_dismiss_target(objects.panel3_1);
    list_bind_dismiss_target(objects.obj22);
    list_bind_dismiss_target(objects.main_3);
    list_bind_dismiss_target(objects.lablef3_1);
    list_bind_dismiss_target(objects.lablef3_2);
    if (objects.lablef3_0) {
      list_bind_dismiss_target(lv_obj_get_parent(objects.lablef3_0));
      list_bind_dismiss_target(objects.lablef3_0);
    }
    for (int ri = 0; ri < 10; ri++) {
      list_bind_dismiss_target(list_row_panel(ri));
      list_bind_dismiss_target(list_idx_panel(ri));
    }
    list_blank_cb_added = true;
  }

  if (list_scroll_track) {
    lv_obj_set_parent(list_scroll_track, objects.panel3_1);
    lv_obj_set_pos(list_scroll_track, 456, 8);
    lv_obj_set_size(list_scroll_track, 4, 229);  // هم‌تراز ارتفاع جدید پنل
    lv_obj_set_style_radius(list_scroll_track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(list_scroll_track, lv_color_hex(0xe3e7ea), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (list_max_top() <= 0) lv_obj_add_flag(list_scroll_track, LV_OBJ_FLAG_HIDDEN);
  }
  if (list_scroll_thumb) {
    lv_obj_set_style_radius(list_scroll_thumb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(list_scroll_thumb, lv_color_hex(0x94a3ad), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  const int row_h = 44;
  const int gap = 0;
  const int vis_rows = list_rows_on_screen();
  for (int i = 0; i < 10; i++) {
    lv_obj_t *row = list_row_panel(i);
    if (!row) continue;
    if (i >= vis_rows) {
      lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_obj_set_pos(row, 0, i * (row_h + gap));
    lv_obj_set_size(row, 464, row_h);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    // ردیف اول/آخر: گوش گرد روی پس‌زمینه — بدون clip تا مربع سبز بریده نشود
    const bool edge_row = (i == 0) || (i == vis_rows - 1);
    lv_obj_set_style_radius(row, edge_row ? 10 : 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(row, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_border_width(row, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(row, lv_color_hex(0xe4e5e9), LV_PART_MAIN | LV_STATE_DEFAULT);
    // رنگ ردیف/شاخص فقط در list_apply (زرد تمام‌برش) — اینجا سبز اجباری نگذار
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *idxp = list_idx_panel(i);
    if (idxp) {
      lv_obj_set_pos(idxp, 4, 3);
      lv_obj_set_size(idxp, 40, 36);
      lv_obj_set_style_radius(idxp, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(idxp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(idxp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(idxp, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
      if (lv_obj_get_child_count(idxp) > 0) {
        lv_obj_t *il = lv_obj_get_child(idxp, 0);
        lv_obj_set_style_text_font(il, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(il, 40, 36);
        lv_obj_set_style_text_align(il, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(il, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(il, LV_ALIGN_CENTER, 0, 4);
      }
    }
    lv_obj_t *ang_b = list_get_ang_button(i);
    if (ang_b) {
      lv_obj_set_pos(ang_b, 46, 3);
      lv_obj_set_size(ang_b, 46, 36);
      lv_obj_clear_flag(ang_b, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *sp = list_sp_label(i);
    if (sp) {
      lv_obj_set_pos(sp, 96, 5);
      lv_obj_set_size(sp, 144, 30);
      lv_obj_set_style_text_font(sp, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(sp, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(sp, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(sp, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(sp, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_bg_opa(sp, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(sp, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_text_color(sp, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_t *xx = list_x_label(i);
    if (xx) {
      lv_obj_set_pos(xx, 244, 11);
      lv_obj_set_size(xx, 20, 22);
      lv_obj_set_style_text_font(xx, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(xx, lv_color_hex(0x66727d), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(xx, "x");
    }
    lv_obj_t *cn = list_cnt_label(i);
    if (cn) {
      lv_obj_set_pos(cn, 268, 5);
      lv_obj_set_size(cn, 138, 30);
      lv_obj_set_style_text_font(cn, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_align(cn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_radius(cn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(cn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(cn, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_bg_opa(cn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(cn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_text_color(cn, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_t *ed = list_edit_btn(i);
    if (ed) lv_obj_add_flag(ed, LV_OBJ_FLAG_HIDDEN);
  }
  list_sync_index_labels();
  list_apply_visible_values_now();
  // reflow استایل کادر عدد را ریست می‌کند → Task2 دوباره فوکوس آبی را بکشد
  GUI.flag_panel3_edit_labels_txt = true;
}

static lv_obj_t *list_help_modal = NULL;
static lv_obj_t *list_help_cont = NULL;
static lv_obj_t *list_help_body_box = NULL;

static void list_help_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (list_help_modal && lv_obj_is_valid(list_help_modal)) lv_obj_delete_async(list_help_modal);
  list_help_modal = NULL;
  list_help_cont = NULL;
  list_help_body_box = NULL;
  GUI.lock_screen = false;
  GUI.buzzer_cmd_short = true;
  ui_resume_checkpoint();
}


static void list_help_add_section(lv_obj_t *box, bool en, const char *head, const char *desc) {
  if (!box || !head || !desc) return;
  lv_obj_t *h = lv_label_create(box);
  lv_obj_set_width(h, 392);
  lv_obj_set_style_text_font(h, en ? &lv_font_montserrat_18 : &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(h, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(h, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(h, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(h, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
  lv_label_set_text(h, head);
  lv_obj_t *d = lv_label_create(box);
  lv_obj_set_width(d, 392);
  lv_obj_set_style_text_font(d, en ? &lv_font_montserrat_14 : &ui_font_naz20b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(d, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(d, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_line_space(d, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(d, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(d, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(d, LV_LABEL_LONG_WRAP);
  lv_label_set_text(d, desc);
}

static void list_help_show() {
  if (GUI.lock_screen && (!list_help_modal || !lv_obj_is_valid(list_help_modal))) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (list_help_modal && lv_obj_is_valid(list_help_modal)) {
    lv_obj_delete(list_help_modal);
    list_help_modal = NULL;
    list_help_cont = NULL;
    list_help_body_box = NULL;
  }
  const bool en = (GUI.system_language == 2);

  list_help_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(list_help_modal, 0, 0);
  lv_obj_set_size(list_help_modal, 480, 320);
  lv_obj_set_style_bg_opa(list_help_modal, LV_OPA_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(list_help_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(list_help_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(list_help_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(list_help_modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(list_help_modal, LV_OBJ_FLAG_CLICKABLE);

  list_help_cont = lv_obj_create(list_help_modal);
  lv_obj_set_size(list_help_cont, 440, 244);
  lv_obj_align(list_help_cont, LV_ALIGN_CENTER, 0, 18);
  lv_obj_set_style_bg_color(list_help_cont, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(list_help_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(list_help_cont, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(list_help_cont, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(list_help_cont, lv_color_hex(0xd7dde2), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(list_help_cont, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(list_help_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *close = lv_button_create(list_help_cont);
  lv_obj_set_size(close, 34, 30);
  lv_obj_align(close, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_radius(close, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(close, lv_color_hex(0xd34235), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(close, list_help_close_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(close);
  lv_label_set_text(cl, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(cl);

  lv_obj_t *title = lv_label_create(list_help_cont);
  lv_obj_set_pos(title, 72, 0);
  lv_obj_set_size(title, 320, 36);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_20 : &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(title, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(title, en ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(title, en ? "Cut List Guide" : "راهنمای لیست برش");

  lv_obj_t *body_box = lv_obj_create(list_help_cont);
  list_help_body_box = body_box;
  lv_obj_set_pos(body_box, 8, 38);
  lv_obj_set_size(body_box, 404, 176);
  lv_obj_set_style_bg_opa(body_box, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(body_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(body_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(body_box, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scroll_dir(body_box, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(body_box, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(body_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(body_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(body_box, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

  if (en) {
    list_help_add_section(body_box, en, "Length", "Tap the left number box, type the size in millimeters, then confirm.");
    list_help_add_section(body_box, en, "Quantity", "After confirming length, the quantity box opens. Enter how many pieces you need.");
    list_help_add_section(body_box, en, "Next row", "Confirming quantity moves to the next row. A new row is created if needed.");
    list_help_add_section(body_box, en, "+ / -", "Add a new empty row or remove the last visible row.");
    list_help_add_section(body_box, en, "Scroll", "Drag inside the white list area to see more rows. Soft keyboard opens on number tap when no external keyboard is connected.");
    list_help_add_section(body_box, en, "Optimize", "Builds a cutting layout from saved lengths and quantities. Review before real cutting.");
  } else {
    list_help_add_section(body_box, en, "متراژ", "روی کادر عدد سمت چپ بزنید، سایز را بر حسب میلی متر وارد کنید و تایید را بزنید.");
    list_help_add_section(body_box, en, "تعداد", "بعد از تایید متراژ، کادر تعداد همان ردیف خودکار فعال می شود. تعداد قطعه های مورد نیاز را وارد کنید.");
    list_help_add_section(body_box, en, "رفتن به ردیف بعد", "با تایید تعداد، ردیف بعدی آماده ورود می شود و در صورت نیاز ردیف جدید ساخته می شود.");
    list_help_add_section(body_box, en, "+ و -", "ردیف خالی اضافه می کند یا آخرین ردیف قابل نمایش را کم می کند.");
    list_help_add_section(body_box, en, "اسکرول و کیبورد", "داخل کادر سفید بکشید تا ردیف های بیشتر دیده شوند. اگر کیبورد خارجی وصل نباشد، لمس عدد کیبورد نرم را باز می کند و لمس جای دیگر آن را می بندد.");
    list_help_add_section(body_box, en, "بهینه سازی", "از متراژها و تعدادهای ذخیره شده برنامه چیدمان برش می سازد؛ قبل از اجرای واقعی، خروجی را بررسی کنید.");
  }

  GUI.lock_screen = true;
  lv_obj_clear_flag(list_help_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(list_help_modal);
  GUI.buzzer_cmd_short = true;
  ui_resume_checkpoint();
}

static void list_help_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_suppress_release) { ui_help_suppress_release = false; return; }
  list_help_show();
}

bool ui_resume_help_f1_open() {
  if (pending_f1_help_phase != 0) return true;
  return f1_help_dlg && lv_obj_is_valid(f1_help_dlg) &&
         !lv_obj_has_flag(f1_help_dlg, LV_OBJ_FLAG_HIDDEN);
}

bool ui_resume_help_list_open() {
  return list_help_modal && lv_obj_is_valid(list_help_modal) &&
         !lv_obj_has_flag(list_help_modal, LV_OBJ_FLAG_HIDDEN);
}

bool ui_resume_help_opt_open() {
  return opt_help_is_open();
}

// راهنما را بعد از reinit/تب اعمال کن تا دوباره پاک نشود
static bool ui_resume_want_help_f1 = false;
static bool ui_resume_want_help_list = false;
static uint8_t ui_resume_help_delay = 0;
static bool ui_resume_goto_requeued = false;

static void ui_resume_deferred_help_tick() {
  if (ui_resume_help_delay > 0) {
    if (GUI.ui_screen_settle > 0) return;
    if (GUI.flag_reinit_settings || GUI.pending_reinit_settings) return;
    ui_resume_help_delay--;
    if (ui_resume_help_delay > 0) return;
    // آخرین تیک تأخیر: تب را دوباره قفل کن روی مقدار resume
    if (GUI.scr_id == 2 && objects.tabview2_1) {
      int tab = (int)ui_resume_apply_settings_tab;
      if (tab < 0) tab = 0;
      if (tab > 4) tab = 4;
      if ((int)lv_tabview_get_tab_act(objects.tabview2_1) != tab) {
        lv_tabview_set_active(objects.tabview2_1, tab, LV_ANIM_OFF);
        admin_tabs_reflow();
        f1_header_title_sync();
      }
    }
  }
  if (ui_resume_want_help_f1 && GUI.scr_id == 2) {
    ui_resume_want_help_f1 = false;
    f1_help_request_deferred();
  }
  if (ui_resume_want_help_list && GUI.scr_id == 3) {
    ui_resume_want_help_list = false;
    list_help_show();
  }
}

bool ui_resume_opt_panel_open() {
  return opt_cont && lv_obj_is_valid(opt_cont);
}

int ui_resume_query_opt_bar_page() { return opt_bar_page_start; }

void ui_resume_restore_opt_bar_page(int page) {
  if (page < 0) page = 0;
  int last = 0;
  if (OPT.bar_count > OPT_RENDER_ROWS)
    last = ((OPT.bar_count - 1) / OPT_RENDER_ROWS) * OPT_RENDER_ROWS;
  if (page > last) page = last;
  opt_bar_page_start = page;
}

void ui_resume_apply_pending_state() {
  if (!ui_resume_apply_pending) return;
  // تا صف goto و لود صفحهٔ هدف تمام نشده، اسنپ‌شات را نسوزان
  if (dma_ui_resume_scr >= 0) return;
  if (GUI.ui_screen_settle > 0) return;
  if (GUI.pending_goto_scr >= 0) return;
  if (opt_calc_active) return;
  if (ui_resume_apply_scr >= 0 && GUI.scr_id != ui_resume_apply_scr) {
    // یک‌بار صف مجدد؛ اگر باز هم نرسید رها کن تا صفحه سیاه قفل نماند
    if (!ui_resume_goto_requeued) {
      ui_resume_goto_requeued = true;
      GUI.pending_goto_scr = ui_resume_apply_scr;
      return;
    }
    ui_resume_goto_requeued = false;
    ui_resume_apply_pending = false;
    ui_resume_apply_scr = -1;
    STP.referenced = false;
    GUI.lock_screen = false;
    splash_release_hold_cover();
    return;
  }
  ui_resume_goto_requeued = false;

  const uint32_t f = ui_resume_apply_flags;
  ui_resume_apply_pending = false;
  ui_resume_apply_scr = -1;
  // مسیر عمومی ورود به Main referenced را true می‌کند؛ بازیابی hang باید آن را
  // پیش از برداشتن قفل/پوشش دوباره نامعتبر کند.
  STP.referenced = false;
  GUI.lock_screen = false;

  if (GUI.scr_id == 2 && objects.tabview2_1) {
    int tab = (int)ui_resume_apply_settings_tab;
    if (tab < 0) tab = 0;
    if (tab > 4) tab = 4;
    // همیشه تب ذخیره‌شده را بزن (F2..F5) — فقط «اگر فرق داشت» باعث می‌شد گاهی روی F1 بماند
    lv_tabview_set_active(objects.tabview2_1, tab, LV_ANIM_OFF);
    settings_hide_eez_clutter();
    admin_tabs_reflow();
    f1_header_title_sync();
    GUI.pending_reinit_settings = true;
    if ((f & UI_RESUME_F_F4_UNLOCKED) && tab == 3)
      ui_resume_restore_f4_unlocked();
    if ((f & UI_RESUME_F_F5_OV) && tab == 4)
      ui_resume_restore_f5_ov((int)ui_resume_apply_f5_ov);
  }

  if (GUI.scr_id == 3) {
    int lp = (int)ui_resume_apply_list_page;
    int max_top = list_max_top();
    if (lp < 0) lp = 0;
    if (lp > max_top) lp = max_top;
    GUI.list_page = lp;
    GUI.flag_refresh_visible = true;
    GUI.flag_update_labels_3_1 = true;
    GUI.flag_update_labels_3_2 = true;
    list_reflow_compact();
  }

  if (GUI.scr_id == 1) {
    if (f & UI_RESUME_F_NUMLOCK) {
      GUI.NumLock = true;
      OPT.active = false;
      OPT.waiting_move = false;
      AUT.selected_count_val = -1;
      AUT.temp_list_count_Val = 0;
      opt_main_bar_lbl_hide();
      jog_manual_badge_sync();
    } else if (f & UI_RESUME_F_OPT) {
      ui_resume_restore_opt_bar_page((int)ui_resume_apply_opt_bar_page);
      ui_resume_opt_pending = true;
      ui_resume_opt_started = false;
      ui_resume_opt_want_panel = (f & UI_RESUME_F_OPT_PANEL) != 0;
      ui_resume_opt_want_help = (f & UI_RESUME_F_HELP_OPT) != 0;
      ui_resume_opt_to_main = !ui_resume_opt_want_panel;
    } else if (f & UI_RESUME_F_LIST) {
      int idx = (int)ui_resume_apply_list_idx;
      if (idx >= 0 && idx < LIST_MAX && AUT.list_count_Val[idx] > 0) {
        if (OPT.active) {
          OPT.active = false;
          OPT.waiting_move = false;
          OPT.flag_refresh = false;
        }
        STP.Man_Sp_mm = AUT.list_sp_Val_p[idx];
        GUI.inputBuffer_sp = GUI.inputBuffer_L_3_1_x[idx];
        AUT.selected_count_val = idx;
        int qty = (int)ui_resume_apply_list_qty;
        if (qty < 1) qty = AUT.list_count_Val[idx];
        if (qty > AUT.list_count_Val[idx]) qty = AUT.list_count_Val[idx];
        AUT.temp_list_count_Val = qty;
        AUT.in_position_flag = false;
        GUI.NumLock = false;
        GUI.update_lable1_1 = true;
        GUI.update_lable1_3 = true;
        GUI.edit_mode_lable1_1 = true;
        opt_main_bar_lbl_sync();
      }
    }
  } else if ((f & UI_RESUME_F_OPT) && (f & UI_RESUME_F_OPT_PANEL)) {
    ui_resume_restore_opt_bar_page((int)ui_resume_apply_opt_bar_page);
    ui_resume_opt_pending = true;
    ui_resume_opt_started = false;
    ui_resume_opt_want_panel = true;
    ui_resume_opt_want_help = (f & UI_RESUME_F_HELP_OPT) != 0;
    ui_resume_opt_to_main = false;
  }

  if (GUI.scr_id == 2) {
    if ((f & UI_RESUME_F_F3_PINMAP) && ui_resume_apply_f3_pinmap >= 0) {
      const int pack = (int)ui_resume_apply_f3_pinmap;
      ui_resume_restore_f3_pinmap((pack & 0x100) != 0, pack & 0xFF);
    } else if ((f & UI_RESUME_F_F3_PINHELP) && ui_resume_apply_f3_help_id > 0) {
      ui_resume_restore_f3_pinhelp((int)ui_resume_apply_f3_help_id);
    }
    if (f & UI_RESUME_F_HELP_F1) f1_help_request_deferred();
  } else if (GUI.scr_id == 3 && (f & UI_RESUME_F_HELP_LIST)) {
    list_help_show();
  }
  // اسنپ‌شات تازه و حذف پوشش فقط بعد از ساخت تب/راهنما/پنجرهٔ نهایی.
  ui_resume_checkpoint();
  splash_release_hold_cover();
}

void prcs_ui_resume_opt() {
  // [UI/Task2] پس از پایان splash/apply، همان مسیر toggle موجود را برای AP صف کن.
  // درخواست RTC در early boot مصرف شده است؛ اگر همین start نیمه‌کاره reset شود،
  // تا یک start موفق رکورد تازه‌ای برای بازیابی وجود ندارد.
  if (web_portal_resume_boot_pending && !splash_cont && dma_ui_resume_scr < 0 &&
      (!ui_resume_apply_pending || device_locked) &&
      !g_update_mode && !update_flow_network_busy() &&
      !web_portal_transitioning && !GUI.flag_web_portal_toggle) {
    if (web_portal_running) {
      (void)web_portal_resume_take_boot_request();
    } else if (web_portal_resume_take_boot_request()) {
      WebWifiMode mode;
      if (web_wifi_saved_mode(&mode) && mode == WEB_WIFI_STA) web_portal_request_sta_setup();
      else GUI.flag_web_portal_toggle = true;
    }
  }
  if (!ui_resume_opt_pending) return;
  STP.referenced = false;
  if (opt_calc_active) return;
  if (GUI.pending_goto_scr >= 0) return;

  if (!ui_resume_opt_started) {
    ui_resume_opt_started = true;
    if (!opt_open_from_list()) {
      STP.referenced = false;
      ui_resume_opt_pending = false;
      ui_resume_opt_to_main = false;
      ui_resume_opt_want_help = false;
      return;
    }
    STP.referenced = false;
    if (opt_calc_active) return;  // اتمام calc مسیر to_main/panel را تمام می‌کند
  }

  if (OPT.piece_count <= 0) {
    STP.referenced = false;
    ui_resume_opt_pending = false;
    ui_resume_opt_to_main = false;
    ui_resume_opt_want_help = false;
    return;
  }

  if (ui_resume_apply_opt_cur >= 0 && ui_resume_apply_opt_cur < OPT.piece_count)
    OPT.cur = (int)ui_resume_apply_opt_cur;

  if (ui_resume_opt_want_panel) {
    if (!opt_cont) opt_open_panel();
    else {
      OPT.active = true;
      opt_refresh_labels();
      opt_bar_page_ui_sync();
    }
    if (ui_resume_opt_want_help) opt_help_show();
  } else {
    if (opt_cont) {
      lv_obj_delete(opt_cont);
      opt_cont = NULL;
    }
    (void)opt_load_current_to_main(false, false);
  }

  STP.referenced = false;
  ui_resume_opt_pending = false;
  ui_resume_opt_to_main = false;
  ui_resume_opt_want_help = false;
}

static lv_obj_t *main_ctrl_help_modal = NULL;

static void main_ctrl_help_close() {
  if (main_ctrl_help_modal && lv_obj_is_valid(main_ctrl_help_modal)) {
    lv_obj_delete(main_ctrl_help_modal);
  }
  main_ctrl_help_modal = NULL;
  GUI.lock_screen = false;
}

static void main_ctrl_help_close_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  main_ctrl_help_close();
  GUI.buzzer_cmd_short = true;
}

static void main_ctrl_help_show(const char *fa, const char *en) {
  // راهنمای کامل با همان توست قابل‌نگه‌داشتن (تایمر با لمس می‌ایستد)
  toast_build();
  const bool english = (GUI.system_language == 2);
  lv_label_set_text(toast_lbl_fa, english ? en : fa);
  lv_obj_set_style_text_font(toast_lbl_fa, english ? &lv_font_montserrat_16 : &ui_font_naz18b,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(toast_lbl_fa, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_base_dir(toast_lbl_fa, english ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_width(toast_lbl_fa, 408);
  lv_label_set_long_mode(toast_lbl_fa, LV_LABEL_LONG_WRAP);
  lv_obj_align(toast_lbl_fa, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_add_flag(toast_lbl_en, LV_OBJ_FLAG_HIDDEN);
  toast_holdable = true;
  toast_modal_active = true;
  toast_dismiss_armed = false;
  toast_paused = false;
  toast_paused_elapsed = 0;
  toast_duration_ms = 5200;
  toast_start_ms = lv_tick_get();
  toast_hide_at = toast_start_ms + toast_duration_ms;
  // بلندتر از بالا (کف ثابت پایین) تا متن بلند داخل کادر بماند
  toast_base_w = 448;
  toast_base_h = 168;
  lv_obj_set_size(toast_cont, toast_base_w, toast_base_h);
  lv_obj_align(toast_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
  if (toast_bar) {
    lv_obj_set_width(toast_bar, 408);
    lv_obj_align(toast_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_bar_set_value(toast_bar, 100, LV_ANIM_OFF);
    lv_obj_clear_flag(toast_bar, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_flag(toast_cont, LV_OBJ_FLAG_CLICKABLE);
  toast_set_held_style(false);
  if (toast_catcher) {
    lv_obj_clear_flag(toast_catcher, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast_catcher);
  }
  lv_obj_clear_flag(toast_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(toast_cont);
  GUI.lock_screen = true;  // استارت/جاگ/تاچ زیرین تا بستن پیام
}

bool settings_control_help_try(lv_obj_t *obj);

static void ui_control_help_toast(lv_obj_t *obj) {
  if (GUI.scr_id == 2 && settings_control_help_try(obj)) return;
  // ---- راهنمای کامل: فقط حروف فونت naz + رقم؛ بدون لاتین ----
  if (GUI.scr_id == 1 && obj && objects.main_1) {
    lv_obj_t *cur = obj;
    for (int d = 0; d < 8 && cur; d++, cur = lv_obj_get_parent(cur)) {
      if (objects.lable1_1 && cur == objects.lable1_1) {
        main_ctrl_help_show(
          "عدد مقصد حرکت. انگشت را افقی بکشید تا عدد عوض شود بدون حرکت موتور. دو ضربه ثابت پشت سر هم موتور را به همین عدد میبرد.",
          "Target size. Drag horizontally to change without moving. Steady double-tap moves the axis there.");
        return;
      }
      if (objects.panel1_2 && cur == objects.panel1_2) {
        main_ctrl_help_show(
          "عدد مقصد حرکت. انگشت را افقی بکشید تا عدد عوض شود بدون حرکت موتور. دو ضربه ثابت پشت سر هم موتور را به همین عدد میبرد.",
          "Target size. Drag horizontally to change without moving. Steady double-tap moves the axis there.");
        return;
      }
      if (objects.lable1_2 && cur == objects.lable1_2) {
        main_ctrl_help_show(
          "موقعیت فعلی محور به میلی متر. اگر موتور در حال حرکت است یک لمس یعنی توقف فوری.",
          "Live axis position in mm. One tap while moving stops immediately.");
        return;
      }
      if (objects.lable1_3 && cur == objects.lable1_3) {
        if (AUT.selected_count_val < 0) {
          main_ctrl_help_show(
            "تعداد قطعه. وقتی ردیفی از لیست انتخاب نشده اینجا خط چین است. از لیست یک ردیف را انتخاب کنید تا تعداد باقی مانده دیده شود.",
            "Piece count. Dashes mean no active row. Pick a list row to see remaining quantity.");
        } else {
          main_ctrl_help_show(
            "تعداد باقی مانده از ردیف فعال یا صف برش. با اتمام هر برش کم میشود.",
            "Remaining quantity for the active row or cut queue.");
        }
        return;
      }
      if (objects.obj2 && cur == objects.obj2) {
        main_ctrl_help_show(
          "علامت ضربدر کنار تعداد؛ یعنی چند قطعه از این اندازه لازم است.",
          "Times mark next to quantity.");
        return;
      }
      if (objects.obj3 && cur == objects.obj3) {
        if (AUT.selected_count_val < 0) {
          main_ctrl_help_show(
            "برچسب تعداد. با انتخاب ردیف از لیست برش، عدد تعداد اینجا یا کنار خط چین دیده میشود.",
            "Quantity label. Select a cut-list row to show the count.");
        } else {
          main_ctrl_help_show(
            "برچسب تعداد قطعه های باقی مانده.",
            "Remaining pieces label.");
        }
        return;
      }
      if (objects.obj4 && cur == objects.obj4) {
        main_ctrl_help_show(
          "واحد اندازه گیری میلی متر برای عدد مقصد و موقعیت.",
          "Millimeter unit for target and position.");
        return;
      }
      if (objects.btn1_1 && cur == objects.btn1_1) {
        main_ctrl_help_show(
          "ورود به صفحه لیست برش برای ثبت اندازه و تعداد.",
          "Open the cut list to enter sizes and counts.");
        return;
      }
      if (objects.btn1_2 && cur == objects.btn1_2) {
        main_ctrl_help_show(
          "ورود به تنظیمات ماشین سرعت زبان و موارد دیگر.",
          "Open machine settings.");
        return;
      }
      if (objects.btn1_5 && cur == objects.btn1_5) {
        main_ctrl_help_show(
          "کلید حالت جاگ یا عدد. در جاگ کادر قرمز دستی روشن میشود و با کلیدهای شش و چهار و دو حرکت میکنید.",
          "Toggle jog/number mode. In jog use keys 6 4 2.");
        return;
      }
      if (jog_fine_badge && cur == jog_fine_badge) {
        main_ctrl_help_show(
          "نشان کم. وقتی کلید دو را نگه دارید سرعت جاگ آرام میشود. رها کنید تا سرعت عادی برگردد.",
          "Fine badge. Hold key 2 for slow jog; release for normal speed.");
        return;
      }
      if (jog_fine_lbl && cur == jog_fine_lbl) {
        main_ctrl_help_show(
          "نشان کم. وقتی کلید دو را نگه دارید سرعت جاگ آرام میشود. رها کنید تا سرعت عادی برگردد.",
          "Fine badge. Hold key 2 for slow jog; release for normal speed.");
        return;
      }
      if ((jog_manual_badge && cur == jog_manual_badge) ||
          (jog_manual_title && cur == jog_manual_title) ||
          (jog_manual_sub && cur == jog_manual_sub)) {
        main_ctrl_help_show(
          "حالت دستی جاگ فعال است. کلید شش حرکت به راست. کلید چهار حرکت به چپ. کلید دو سرعت کم. رها کردن کلید یعنی توقف. جویستیک هم همان چپ و راست را انجام میدهد.",
          "Manual jog. Key 6 right, key 4 left, key 2 slow. Release stops. Joystick does left/right too.");
        return;
      }
      if (opt_main_bar_lbl && cur == opt_main_bar_lbl) {
        main_ctrl_help_show(
          "شماره شاخه در بهینه سازی یا شماره ردیف در لیست برش که الان فعال است.",
          "Active bar number in optimize, or active cut-list row.");
        return;
      }
    }
    lv_obj_t *probe = obj;
    for (int d = 0; d < 3 && probe; d++, probe = lv_obj_get_parent(probe)) {
      if (!objects.main_1 || lv_obj_get_parent(probe) != objects.main_1) continue;
      lv_area_t a;
      lv_obj_get_coords(probe, &a);
      const int w = (int)a.x2 - (int)a.x1;
      const int h = (int)a.y2 - (int)a.y1;
      if (w >= 300 && h >= 100 && a.y1 >= 60 && a.y2 <= 260) {
        main_ctrl_help_show(
          "موقعیت فعلی محور به میلی متر. اگر موتور در حال حرکت است یک لمس یعنی توقف فوری.",
          "Live axis position in mm. One tap while moving stops immediately.");
        return;
      }
    }
  }

  if (GUI.scr_id == 3 && obj) {
    lv_obj_t *cur = obj;
    for (int d = 0; d < 10 && cur; d++, cur = lv_obj_get_parent(cur)) {
      if (objects.btn3_1 && cur == objects.btn3_1) {
        main_ctrl_help_show(
          "خانه. با یک لمس کوتاه به صفحه اصلی برمیگردید. اگر نگه دارید فقط همین راهنما نشان داده میشود و صفحه عوض نمیشود.",
          "Home. Short tap returns to main. Hold shows this help without leaving.");
        return;
      }
      if (btn_list_help && cur == btn_list_help) {
        main_ctrl_help_show(
          "راهنمای کامل صفحه. با لمس کوتاه برگه راهنما باز میشود. نگه داشتن فقط این توضیح کوتاه را نشان میدهد.",
          "Page help. Short tap opens the full guide. Hold shows this tip only.");
        return;
      }
      if (objects.lablef3_0 && (cur == objects.lablef3_0 || cur == lv_obj_get_parent(objects.lablef3_0))) {
        main_ctrl_help_show(
          "عنوان صفحه لیست برش. اینجا اندازه و تعداد هر سایز را برای برش ثبت میکنید.",
          "Cut List title. Enter each size and quantity here.");
        return;
      }
      if (btn_list_add_round && cur == btn_list_add_round) {
        main_ctrl_help_show(
          "افزودن ردیف. لمس کوتاه یک ردیف خالی جدید در انتهای لیست میسازد. ظرفیت کل صد ردیف است.",
          "Add row. Short tap appends an empty row. Capacity is 100 rows.");
        return;
      }
      if (btn_list_remove_round && cur == btn_list_remove_round) {
        main_ctrl_help_show(
          "کم کردن ردیف. لمس کوتاه آخرین ردیف قابل نمایش را برمیدارد و مقادیرش را صفر میکند.",
          "Remove row. Short tap clears and drops the last visible row.");
        return;
      }
      if (btn_optimize && cur == btn_optimize) {
        main_ctrl_help_show(
          "بهینه برش. از اندازه ها و تعدادهای ذخیره شده برنامه چیدن روی شاخه میسازد تا پرت کمتر شود.",
          "Optimize. Builds a stock-bar plan from saved sizes to reduce scrap.");
        return;
      }
      if (objects.lablef3_1 && cur == objects.lablef3_1) {
        main_ctrl_help_show(
          "ستون متراژ به میلی متر. روی عدد هر ردیف بزنید تا اندازه را وارد کنید. ممیز فقط در همین ستون مجاز است.",
          "Length column in mm. Tap a row value to edit. Decimal only here.");
        return;
      }
      if (objects.lablef3_2 && cur == objects.lablef3_2) {
        main_ctrl_help_show(
          "ستون تعداد قطعه. بعد از تایید متراژ خودکار فعال میشود. فقط عدد صحیح وارد کنید.",
          "Quantity column. Opens after length confirm. Integers only.");
        return;
      }
      if (list_header_line && cur == list_header_line) {
        main_ctrl_help_show("خط جداکننده هدر از جدول ردیف ها.", "Header separator line.");
        return;
      }
      if (list_scroll_track && (cur == list_scroll_track || (list_scroll_thumb && cur == list_scroll_thumb))) {
        main_ctrl_help_show(
          "نوار پیمایش. برای دیدن ردیف های بیشتر داخل جدول سفید انگشت را بالا یا پایین بکشید.",
          "Scroll bar. Drag inside the white table to see more rows.");
        return;
      }
      if (numpad_cont && cur == numpad_cont) {
        main_ctrl_help_show(
          "صفحه کلید عددی نرم. برای متراژ ممیز مجاز است. برای تعداد فقط رقم صحیح.",
          "Soft number pad. Decimal for length only.");
        return;
      }
      if (objects.panel3_1 && cur == objects.panel3_1) {
        main_ctrl_help_show(
          "جدول لیست. روی متراژ یا تعداد بزنید تا ویرایش شود. برای ردیف بیشتر انگشت را روی همین کادر بکشید.",
          "Cut table. Tap length or quantity to edit. Drag to scroll.");
        return;
      }
    }
    for (int i = 0; i < 10; i++) {
      lv_obj_t *row = list_row_panel(i);
      lv_obj_t *idxp = list_idx_panel(i);
      lv_obj_t *sp = list_sp_label(i);
      lv_obj_t *cn = list_cnt_label(i);
      lv_obj_t *cur2 = obj;
      for (int d = 0; d < 6 && cur2; d++, cur2 = lv_obj_get_parent(cur2)) {
        if (idxp && cur2 == idxp) {
          main_ctrl_help_show("شماره ردیف. لمس کوتاه همان ردیف را انتخاب میکند.", "Row number. Short tap selects it.");
          return;
        }
        if (sp && cur2 == sp) {
          main_ctrl_help_show(
            "متراژ این ردیف به میلی متر. لمس کوتاه ویرایش را باز میکند و بعد به تعداد میرود.",
            "Row length in mm. Short tap edits, then quantity opens.");
          return;
        }
        if (cn && cur2 == cn) {
          main_ctrl_help_show(
            "تعداد قطعه برای همین متراژ. با تایید به ردیف بعدی میروید.",
            "Piece count for this length. Confirm goes to next row.");
          return;
        }
        if (row && cur2 == row) {
          main_ctrl_help_show(
            "یک ردیف لیست برش. شماره سبز، متراژ به میلی متر، سپس تعداد.",
            "One cut-list row: index, length mm, quantity.");
          return;
        }
      }
    }
    main_ctrl_help_show(
      "صفحه لیست برش. دو ثانیه نگه دارید تا راهنمای هر بخش بیاید بدون اجرای کلید.",
      "Cut List. Hold two seconds for help without running the control.");
    return;
  }

  const char *fa = "این کنترل را نگه دارید تا راهنمایش را ببینید.";
  const char *en = "Hold this control to see its help.";
  if (obj == btn_optimize) {
    fa = "بهینه برش. قطعات را داخل شاخه ها میچیند تا دورریز کمتر شود.";
    en = "Optimize: packs pieces into bars to reduce scrap.";
  } else if (obj == btn_list_add_round) {
    fa = "افزودن یک ردیف جدید به لیست برش.";
    en = "Add a new cut-list row.";
  } else if (obj == btn_list_remove_round) {
    fa = "حذف آخرین ردیف از لیست برش.";
    en = "Remove the last cut-list row.";
  } else if (obj == btn_list_clear) {
    fa = "صفر کردن تعداد همه ردیف های لیست برش.";
    en = "Clear quantity on every cut-list row.";
  } else if (obj == btn_list_help) {
    fa = "راهنمای کامل همین صفحه لیست برش.";
    en = "Full help for the cut-list page.";
  } else if (objects.btn3_1 && obj == objects.btn3_1) {
    fa = "برگشت به صفحه اصلی.";
    en = "Return to the main screen.";
  } else if (obj == numpad_cont) {
    fa = "ورود عدد. ممیز فقط برای اندازه به میلی متر مجاز است.";
    en = "Number pad. Decimal only for length in mm.";
  }
  main_ctrl_help_show(fa, en);
}

static lv_point_t ui_help_press_pt;
static bool ui_help_press_valid = false;
static const int UI_HELP_MOVE_CANCEL_PX = 12;

// اعلام پیش‌رو — تعریف در admin_ui.h (همان TU)
bool f3_try_pin_help(lv_obj_t *obj);

static void ui_control_help_event_cb(lv_event_t *e);

static void ui_control_help_unbind(lv_obj_t *obj) {
  if (!obj) return;
  lv_obj_remove_event_cb(obj, ui_control_help_event_cb);
}

static void ui_control_help_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t *indev = lv_indev_active();
  lv_point_t p;
  if (indev) lv_indev_get_point(indev, &p);

  // تب F3: لرزش تاچ نباید تایمر ۲ثانیهٔ راهنمای پایه را بکشد
  const bool on_f3 = (GUI.scr_id == 2 && objects.tabview2_1 &&
                      (int)lv_tabview_get_tab_act(objects.tabview2_1) == 2);

  if (code == LV_EVENT_PRESSED) {
    ui_help_press_pt = p;
    ui_help_press_valid = true;
    ui_help_dragged = false;
    return;
  }
  if (code == LV_EVENT_PRESSING && ui_help_press_valid) {
    if (on_f3) return;
    int dx = (int)p.x - (int)ui_help_press_pt.x;
    int dy = (int)p.y - (int)ui_help_press_pt.y;
    if (!ui_help_dragged && (LV_ABS(dx) >= UI_HELP_MOVE_CANCEL_PX || LV_ABS(dy) >= UI_HELP_MOVE_CANCEL_PX)) {
      ui_help_dragged = true;
      if (indev) lv_indev_reset_long_press(indev);  // تایمر ۲ثانیه راهنما را قطع کن
    }
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    ui_help_press_valid = false;
    ui_help_dragged = false;  // نچسبد — وگرنه long-press مقصد بعداً می‌میرد
    return;
  }
  if (code != LV_EVENT_LONG_PRESSED) return;
  if (GUI.lock_screen || (lv_layer_top() && lv_obj_get_child_count(lv_layer_top()) > 0)) return;
  lv_obj_t *tgt = (lv_obj_t *)lv_event_get_target(e);
  if (on_f3 && f3_try_pin_help(tgt)) {
    ui_help_suppress_release = true;
    return;
  }
  const bool is_dest =
      tgt && ((objects.lable1_1 && tgt == objects.lable1_1) ||
              (objects.panel1_2 && tgt == objects.panel1_2));
  if (is_dest) {
    // مقصد: فقط اسکراب واقعی عدد؛ ui_help_dragged اینجا معنی ندارد (PRESSING راهنما بسته است)
    if (main_destination_scrub_moved()) return;
  } else {
    if (ui_help_dragged) return;
    if (!ui_help_press_valid) return;
  }
  // نگه‌داشتن ذخیرهٔ Fها → دیالوگ بازگردانی تنظیمات کارخانه (نه ذخیره)
  if (GUI.scr_id == 2 && objects.btn_save && tgt) {
    for (lv_obj_t *cur = tgt; cur; cur = lv_obj_get_parent(cur)) {
      if (cur == objects.btn_save) {
        ui_help_suppress_release = true;
        GUI.flag_factory_reset_dialog = true;
        GUI.buzzer_cmd_short = true;
        return;
      }
    }
  }
  ui_help_suppress_release = true;
  ui_control_help_toast(tgt);
  GUI.buzzer_cmd_short = true;
}

// بعد از recreate مقصد در ensure_safe_main1 — راهنما را فوری روی ویجت جدید ببند
void ui_rebind_destination_help() {
  lv_obj_t *objs[2] = { objects.lable1_1, objects.panel1_2 };
  for (int i = 0; i < 2; i++) {
    lv_obj_t *root = objs[i];
    if (!root) continue;
    if (!lv_obj_has_flag(root, LV_OBJ_FLAG_USER_1)) {
      // PRESSED فقط برای پاک‌کردن فلگ؛ بدون PRESSING تا با اسکراب/تایمر تداخل نکند
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESSED, NULL);
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_RELEASED, NULL);
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESS_LOST, NULL);
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_LONG_PRESSED, NULL);
      lv_obj_add_flag(root, LV_OBJ_FLAG_USER_1);
    }
    lv_obj_add_flag(root, LV_OBJ_FLAG_USER_2);
  }
}

static void ui_bind_help_tree(lv_obj_t *root) {
  if (!root) return;
  // مقصد: بدون PRESSING راهنما (reset_long_press با اسکراب می‌جنگد)
  if ((objects.lable1_1 && root == objects.lable1_1) ||
      (objects.panel1_2 && root == objects.panel1_2)) {
    if (!lv_obj_has_flag(root, LV_OBJ_FLAG_USER_1)) {
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESSED, NULL);
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_RELEASED, NULL);
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESS_LOST, NULL);
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_LONG_PRESSED, NULL);
      lv_obj_add_flag(root, LV_OBJ_FLAG_USER_1);
    }
    lv_obj_add_flag(root, LV_OBJ_FLAG_USER_2);
    return;
  }
  if (lv_obj_has_flag(root, LV_OBJ_FLAG_CLICKABLE) && !lv_obj_has_flag(root, LV_OBJ_FLAG_USER_2)) {
    // USER_2 = bind کامل (pressed/pressing برای لغو راهنما هنگام درگ)
    lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_PRESS_LOST, NULL);
    if (!lv_obj_has_flag(root, LV_OBJ_FLAG_USER_1)) {
      lv_obj_add_event_cb(root, ui_control_help_event_cb, LV_EVENT_LONG_PRESSED, NULL);
      lv_obj_add_flag(root, LV_OBJ_FLAG_USER_1);
    }
    lv_obj_add_flag(root, LV_OBJ_FLAG_USER_2);
  }
  uint32_t child_count = lv_obj_get_child_count(root);
  for (uint32_t i = 0; i < child_count; i++) ui_bind_help_tree(lv_obj_get_child(root, i));
}

static void ui_bind_visible_help_callbacks() {
  if (GUI.ui_screen_settle > 0) return;
  static lv_obj_t *last_scr = NULL;
  static lv_obj_t *last_top = NULL;
  static uint32_t last_scr_children = 0;
  static uint32_t last_top_children = 0;
  static uint32_t last_bind_ms = 0;
  uint32_t now = millis();
  lv_display_t *disp = lv_display_get_default();
  if (!disp) return;
  lv_obj_t *scr = lv_screen_active();
  lv_obj_t *top = lv_display_get_layer_top(disp);
  uint32_t scr_children = scr ? lv_obj_get_child_count(scr) : 0;
  uint32_t top_children = top ? lv_obj_get_child_count(top) : 0;
  bool changed = (scr != last_scr) || (top != last_top) ||
                 (scr_children != last_scr_children) ||
                 (top_children != last_top_children);
  if (!changed && (uint32_t)(now - last_bind_ms) < 5000) return;
  last_scr = scr;
  last_top = top;
  last_scr_children = scr_children;
  last_top_children = top_children;
  last_bind_ms = now;
  ui_bind_help_tree(scr);
  ui_bind_help_tree(top);
}

static lv_obj_t *ui_mouse_obj_at_point() {
  lv_display_t *disp = lv_display_get_default();
  if (!disp) return NULL;
  lv_point_t p;
  usb_mouse_get_point(&p);
  lv_obj_t *obj = lv_indev_search_obj(lv_display_get_layer_top(disp), &p);
  if (obj) return obj;
  obj = lv_indev_search_obj(lv_screen_active(), &p);
  return obj;
}

static bool ui_scroll_obj_by_wheel(lv_obj_t *obj, int wheel) {
  if (!obj || !lv_obj_is_valid(obj) || wheel == 0) return false;
  int dy = (wheel < 0) ? -42 : 42;
  lv_obj_t *cur = obj;
  while (cur && lv_obj_is_valid(cur)) {
    if (lv_obj_has_flag(cur, LV_OBJ_FLAG_SCROLLABLE) &&
        (lv_obj_get_scroll_top(cur) > 0 || lv_obj_get_scroll_bottom(cur) > 0)) {
      lv_obj_scroll_by_bounded(cur, 0, dy, LV_ANIM_OFF);
      return true;
    }
    cur = lv_obj_get_parent(cur);
  }
  return false;
}

void prcs_usb_mouse_extras() {
  if (GUI.ui_screen_settle > 0) return;
  ui_bind_visible_help_callbacks();

  if (usb_mouse_take_back_click()) {
    keyboard_nav_page_back();
    return;
  }

  if (usb_mouse_take_right_click()) {
    ui_control_help_toast(ui_mouse_obj_at_point());
  }

  int wheel = usb_mouse_take_wheel();
  if (wheel == 0 || GUI.scr_id == 1) return;
  if (list_help_body_box && lv_obj_is_valid(list_help_body_box) && list_help_modal &&
      lv_obj_is_valid(list_help_modal) && !lv_obj_has_flag(list_help_modal, LV_OBJ_FLAG_HIDDEN)) {
    ui_scroll_obj_by_wheel(list_help_body_box, wheel);
    return;
  }
  if (opt_help_body_box && lv_obj_is_valid(opt_help_body_box) && opt_help_is_open()) {
    ui_scroll_obj_by_wheel(opt_help_body_box, wheel);
    return;
  }
  if (OPT.active && opt_list_body && lv_obj_is_valid(opt_list_body)) {
    opt_bar_page_change((wheel < 0) ? +1 : -1);
    return;
  }
  if (GUI.scr_id == 3) {
    // چرخ وسط موس: هر تیک یک ردیف؛ چند تیک = چند ردیف
    int steps = LV_ABS(wheel);
    if (steps < 1) steps = 1;
    if (steps > 5) steps = 5;
    int dir = (wheel < 0) ? +1 : -1;
    for (int s = 0; s < steps; s++) {
      int max_top = list_max_top();
      int np = GUI.list_page + dir;
      if (np < 0 || np > max_top) break;
      list_page_change(dir);
    }
    return;
  }
  ui_scroll_obj_by_wheel(ui_mouse_obj_at_point(), wheel);
}

void qa_process_ui_action() {
  int action = qa_ui_action;
  if (!action) return;
  qa_ui_action = 0;
  pending_f1_help_phase = 0;
  if (f1_help_dlg && lv_obj_is_valid(f1_help_dlg)) { lv_obj_delete(f1_help_dlg); f1_help_dlg = NULL; }
  if (list_help_modal && lv_obj_is_valid(list_help_modal)) { lv_obj_delete(list_help_modal); list_help_modal = NULL; list_help_cont = NULL; list_help_body_box = NULL; }
  if (opt_help_modal && lv_obj_is_valid(opt_help_modal)) { lv_obj_delete(opt_help_modal); opt_help_modal = NULL; opt_help_cont = NULL; opt_help_body_box = NULL; }
  if (main_ctrl_help_modal && lv_obj_is_valid(main_ctrl_help_modal)) { lv_obj_delete(main_ctrl_help_modal); main_ctrl_help_modal = NULL; }
  GUI.pending_f1_numpad_open = false;
  numpad_close();
  GUI.lock_screen = false;
  pending_reference_confirm = false;
  if (objects.panel0_1) lv_obj_add_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
  if (action >= 1 && action <= 5) {
    if (objects.tabview2_1) lv_tabview_set_active(objects.tabview2_1, action - 1, LV_ANIM_OFF);
    GUI.tab_view_select = action;
    f1_help_request_deferred();
  } else if (action == 6) {
    list_help_show();
  } else if (action == 7) {
    opt_help_show();
  } else if (action == 8) {
    if (objects.tabview2_1) lv_tabview_set_active(objects.tabview2_1, 1, LV_ANIM_OFF);
    GUI.tab_view_select = 2;
    GUI.virtual_kb_enable = true;
    f2_begin_edit_start_line();
    f2_ensure_line_kb();
  } else if (action == 9 || action == 10) {
    int n = (action == 9) ? 5 : 20;
    const int demo_len[5] = { 2000, 1500, 580, 860, 1200 };
    const int demo_cnt[5] = { 6, 10, 20, 8, 6 };
    for (int i = 0; i < LIST_MAX; i++) {
      int len = 0, cnt = 0;
      if (i < n) {
        if (action == 9) {
          len = demo_len[i];
          cnt = demo_cnt[i];
        } else {
          len = 180 + ((i * 137) % 2720);
          cnt = 200;
        }
      }
      AUT.list_sp_Val_p[i] = (float)len;
      AUT.list_count_Val[i] = cnt;
      GUI.inputBuffer_L_3_1_x[i] = String(len);
      GUI.inputBuffer_L_3_2_x[i] = String(cnt);
      save_permanently_preferences_sp_val(i, AUT.list_sp_Val_p[i]);
      save_permanently_preferences_count_val(i, AUT.list_count_Val[i]);
    }
    GUI.visible_raws = n;
    PEND.visible_raws = n;
    save_visible_raws();
    GUI.list_page = 0;
    GUI.flag_update_labels_3_1 = true;
    GUI.flag_update_labels_3_2 = true;
    GUI.flag_refresh_visible = true;
    GUI.flag_update_page = true;
  }
}

void ui_extras_init() {
  // دکمه‌های صفحه‌بندی + بهینه‌سازی روی هدر صفحهٔ لیست (parent: objects.obj22)
  lv_obj_t *parent = objects.obj22;
  if (!parent) return;
  btn_page_prev = mk_hdr_btn(parent, LV_SYMBOL_LEFT, 42, page_btn_event_cb, (void *)(intptr_t)-1);
  lv_obj_set_size(btn_page_prev, 34, 36);
  lv_obj_set_pos(btn_page_prev, 42, -8);
  page_lbl = lv_label_create(parent);
  lv_obj_set_pos(page_lbl, 78, 4);
  lv_obj_set_style_text_font(page_lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(page_lbl, "1/5");
  btn_page_next = mk_hdr_btn(parent, LV_SYMBOL_RIGHT, 118, page_btn_event_cb, (void *)(intptr_t)1);
  lv_obj_set_size(btn_page_next, 34, 36);
  lv_obj_set_pos(btn_page_next, 118, -8);
  lv_obj_add_flag(btn_page_prev, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(btn_page_next, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(page_lbl, LV_OBJ_FLAG_HIDDEN);
  btn_list_help = lv_button_create(parent);
  lv_obj_set_pos(btn_list_help, 56, 6);
  lv_obj_set_size(btn_list_help, 48, 36);
  lv_obj_set_style_bg_color(btn_list_help, lv_color_hex(0xeef3f7), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_list_help, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn_list_help, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(btn_list_help, lv_color_hex(0xcfd8df), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_list_help, list_help_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lh = lv_label_create(btn_list_help);
  lv_obj_set_style_text_font(lh, &ui_font_naz18b, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lh, lv_color_hex(0x20313d), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lh, "راهنما");
  lv_obj_center(lh);
  list_meter_unit_lbl = lv_label_create(parent);
  lv_label_set_text(list_meter_unit_lbl, "(mm)");
  list_header_line = lv_obj_create(parent);
  lv_obj_set_pos(list_header_line, 0, 55);
  lv_obj_set_size(list_header_line, 480, 2);
  lv_obj_set_style_bg_color(list_header_line, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(list_header_line, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(list_header_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(list_header_line, LV_OBJ_FLAG_SCROLLABLE);
  // دکمهٔ بهینه‌سازی (نارنجی، متمایز)
  btn_optimize = lv_button_create(parent);
  lv_obj_set_pos(btn_optimize, 280, -8);
  lv_obj_set_size(btn_optimize, 72, 36);
  lv_obj_set_style_bg_color(btn_optimize, lv_color_hex(0xef6c00), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_optimize, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_optimize, btn_optimize_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *l = lv_label_create(btn_optimize);
  lv_obj_set_style_text_font(l, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(l);
  lv_label_set_text(l, "بهینه");
  // دکمهٔ پاک‌کردن لیست (سطل)
  btn_list_clear = lv_button_create(parent);
  lv_obj_set_pos(btn_list_clear, 354, -8);
  lv_obj_set_size(btn_list_clear, 36, 36);
  lv_obj_set_style_bg_color(btn_list_clear, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn_list_clear, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_list_clear, btn_clear_list_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *lc = lv_label_create(btn_list_clear);
  lv_obj_set_style_text_font(lc, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lc, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(lc);
  lv_label_set_text(lc, LV_SYMBOL_TRASH);
  lv_obj_add_flag(btn_list_clear, LV_OBJ_FLAG_HIDDEN);

  if (objects.btn3_2) lv_obj_add_flag(objects.btn3_2, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn3_3) lv_obj_add_flag(objects.btn3_3, LV_OBJ_FLAG_HIDDEN);

  btn_list_add_round = lv_button_create(parent);
  lv_obj_set_pos(btn_list_add_round, 388, 6);
  lv_obj_set_size(btn_list_add_round, 36, 36);
  lv_obj_set_style_radius(btn_list_add_round, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_list_add_round, lv_color_hex(0x1c9b67), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn_list_add_round, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_list_add_round, list_add_row_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *add_l = lv_label_create(btn_list_add_round);
  lv_label_set_text(add_l, "+");
  lv_obj_set_style_text_font(add_l, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(add_l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(add_l);

  btn_list_remove_round = lv_button_create(parent);
  lv_obj_set_pos(btn_list_remove_round, 436, 6);
  lv_obj_set_size(btn_list_remove_round, 36, 36);
  lv_obj_set_style_radius(btn_list_remove_round, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_list_remove_round, lv_color_hex(0xd34235), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn_list_remove_round, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_list_remove_round, list_remove_row_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *remove_l = lv_label_create(btn_list_remove_round);
  lv_label_set_text(remove_l, "-");
  lv_obj_set_style_text_font(remove_l, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(remove_l, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(remove_l);

  if (objects.panel3_1) {
    list_scroll_track = lv_obj_create(objects.panel3_1);
    lv_obj_clear_flag(list_scroll_track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(list_scroll_track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    list_scroll_thumb = lv_obj_create(list_scroll_track);
    lv_obj_clear_flag(list_scroll_thumb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(list_scroll_thumb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  // سوایپ کنترل‌شده + غیرفعال‌کردن اسکرول بومی LVGL (علت کادرهای پنهان/ظاهرشونده)
  // ردیف/پنل از list_bind_dismiss_target سوایپ کامل دارند؛ اینجا فقط لیبل‌های عدد
  if (objects.panel3_1) {
    lv_obj_clear_flag(objects.panel3_1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(objects.panel3_1, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(objects.panel3_1, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    static bool list_label_swipe_bound = false;
    for (int i = 0; i < 10; i++) {
      lv_obj_t *p = list_row_panel(i);
      if (p) lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
      if (list_label_swipe_bound) continue;
      list_bind_swipe_target(list_sp_label(i));
      list_bind_swipe_target(list_cnt_label(i));
    }
    list_label_swipe_bound = true;
  }
  list_reflow_compact();
  main_reflow_position_disk();
  jog_manual_badge_sync();
  blade_err_badge_sync();
  toast_build();
  numpad_build();
  vkb_settings_build();
}
