#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <WiFi.h>
#include "haffler_warranty.h"

// =========================================================================
//  First-Boot Out-of-Box Experience (OOBE) Setup Wizard
//  طراحی صنعتی منطبق با استانداردهای اپل و زیمنس برای اولین راه‌اندازی
//  مراحل: ۱. اتصال وای‌فای (در صورت عدم اتصال)  ۲. نام سازنده، رمز و تلفن مدیر  ۳. تایید پیامکی OTP
// =========================================================================

enum OobeStep : uint8_t {
  OOBE_STEP_IDLE = 0,
  OOBE_STEP_WIFI = 1,
  OOBE_STEP_MFG_INFO = 2,
  OOBE_STEP_OTP = 3
};

#ifndef OOBE_STAGE_DEFINED
#define OOBE_STAGE_DEFINED
enum OobeStage : uint8_t {
  OOBE_STAGE_NORMAL = 0,
  OOBE_STAGE_F1_ESSENTIALS = 1,
  OOBE_STAGE_F2_LIMITS = 2
};
#endif

static uint8_t oobe_current_step = OOBE_STEP_IDLE;
uint8_t g_oobe_stage = OOBE_STAGE_NORMAL;
static lv_obj_t *oobe_root = NULL;
static lv_obj_t *oobe_card = NULL;
static lv_obj_t *oobe_ta_comp = NULL;
static lv_obj_t *oobe_ta_admin_pin = NULL;
static lv_obj_t *oobe_ta_phone = NULL;
static lv_obj_t *oobe_ta_ssid = NULL;
static lv_obj_t *oobe_ta_pass = NULL;
static lv_obj_t *oobe_ta_otp = NULL;
static lv_obj_t *oobe_active_ta = NULL;
static lv_obj_t *oobe_kb = NULL;
static lv_obj_t *oobe_guide_modal = NULL;
static lv_obj_t *oobe_wifi_status_lbl = NULL;

static uint32_t s_oobe_otp = 0;
static String s_oobe_phone = "";
static uint32_t s_oobe_resend_until_ms = 0;

extern bool g_oobe_done;
extern String g_company_name;
extern int stock_len_mm;
extern float kerf_mm;
extern String admin_pin;
extern volatile bool device_locked;
extern volatile bool g_update_mode;

void save_oobe_done(bool done);
void save_company_name(const String &name);
void save_admin_pin();
void save_stock_kerf();
void save_all();
void ui_toast(const char *fa, const char *en, uint32_t ms);
extern bool buzzer_request_pattern(uint8_t pattern);
extern const lv_font_t ui_font_naz11;
extern const lv_font_t ui_font_naz18b;

void oobe_close();
void oobe_render_step(uint8_t step);
static void oobe_wifi_connect_action(lv_event_t *e);
static void oobe_mfg_submit_action(lv_event_t *e);
static void oobe_otp_verify_action(lv_event_t *e);

static inline bool oobe_is_active() {
  return oobe_current_step != OOBE_STEP_IDLE && oobe_root != NULL && lv_obj_is_valid(oobe_root);
}

static inline void oobe_try_char(char c) {
  if (!oobe_is_active() || !oobe_active_ta) return;
  lv_textarea_add_char(oobe_active_ta, (uint32_t)(uint8_t)c);
}

static inline void oobe_try_backspace() {
  if (!oobe_is_active() || !oobe_active_ta) return;
  lv_textarea_delete_char(oobe_active_ta);
}

static inline void oobe_try_enter() {
  if (!oobe_is_active()) return;
  if (oobe_current_step == OOBE_STEP_WIFI) {
    oobe_wifi_connect_action(NULL);
  } else if (oobe_current_step == OOBE_STEP_MFG_INFO) {
    oobe_mfg_submit_action(NULL);
  } else if (oobe_current_step == OOBE_STEP_OTP) {
    oobe_otp_verify_action(NULL);
  }
}

static void oobe_guide_close_cb(lv_event_t *e) {
  (void)e;
  if (oobe_guide_modal && lv_obj_is_valid(oobe_guide_modal)) {
    lv_obj_delete(oobe_guide_modal);
  }
  oobe_guide_modal = NULL;
  GUI.lock_screen = false;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
}

void oobe_show_f1_guidance() {
  if (g_oobe_done || g_oobe_stage != OOBE_STAGE_F1_ESSENTIALS) return;
  if (oobe_guide_modal && lv_obj_is_valid(oobe_guide_modal)) return;

  const bool en = (GUI.system_language == 2);
  GUI.lock_screen = true;

  oobe_guide_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_size(oobe_guide_modal, 480, 320);
  lv_obj_set_pos(oobe_guide_modal, 0, 0);
  lv_obj_set_style_bg_color(oobe_guide_modal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(oobe_guide_modal, LV_OPA_70, 0);
  lv_obj_set_style_border_width(oobe_guide_modal, 0, 0);
  lv_obj_set_style_pad_all(oobe_guide_modal, 0, 0);
  lv_obj_clear_flag(oobe_guide_modal, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *card = lv_obj_create(oobe_guide_modal);
  lv_obj_set_size(card, 440, 200);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x161b22), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x30363d), 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_width(title, 410);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x0a84ff), 0);
  lv_label_set_text(title, en ? "Initial Setup (Step 1 of 2)" : "راهنمای راه‌اندازی اولیه (گام ۱ از ۲)");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_width(body, 410);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(body, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0xf0f6fc), 0);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_label_set_text(body, en ?
    "Please enter 'Stock Length' and 'Blade Kerf' values.\nThen press the Save button in header to continue." :
    "لطفاً مقادیر «طول شاخه» و «عرض تیغه» را وارد نمایید.\nسپس دکمه ذخیره در بالای صفحه را لمس فرمایید.");
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *ok_btn = lv_button_create(card);
  lv_obj_set_size(ok_btn, 160, 44);
  lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_radius(ok_btn, 10, 0);
  lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_add_event_cb(ok_btn, oobe_guide_close_cb, LV_EVENT_RELEASED, NULL);

  lv_obj_t *ok_lbl = lv_label_create(ok_btn);
  lv_obj_set_style_text_font(ok_lbl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(ok_lbl, en ? "Got it" : "متوجه شدم");
  lv_obj_center(ok_lbl);
}

void oobe_show_f2_guidance() {
  if (g_oobe_done || g_oobe_stage != OOBE_STAGE_F2_LIMITS) return;
  if (oobe_guide_modal && lv_obj_is_valid(oobe_guide_modal)) return;

  const bool en = (GUI.system_language == 2);
  GUI.lock_screen = true;

  oobe_guide_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_size(oobe_guide_modal, 480, 320);
  lv_obj_set_pos(oobe_guide_modal, 0, 0);
  lv_obj_set_style_bg_color(oobe_guide_modal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(oobe_guide_modal, LV_OPA_70, 0);
  lv_obj_set_style_border_width(oobe_guide_modal, 0, 0);
  lv_obj_set_style_pad_all(oobe_guide_modal, 0, 0);
  lv_obj_clear_flag(oobe_guide_modal, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *card = lv_obj_create(oobe_guide_modal);
  lv_obj_set_size(card, 440, 200);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x161b22), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x30363d), 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_width(title, 410);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x0a84ff), 0);
  lv_label_set_text(title, en ? "Initial Setup (Step 2 of 2)" : "راهنمای راه‌اندازی اولیه (گام ۲ از ۲)");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_width(body, 410);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(body, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0xf0f6fc), 0);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_label_set_text(body, en ?
    "Please set 'Start Line' and 'End Line' sizes.\nThen press the Save button to complete the setup." :
    "لطفاً «سایز اول خط» و «سایز آخر خط» را تنظیم نمایید.\nسپس کلید ذخیره را لمس کنید تا راه‌اندازی کامل شود.");
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *ok_btn = lv_button_create(card);
  lv_obj_set_size(ok_btn, 160, 44);
  lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_radius(ok_btn, 10, 0);
  lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_add_event_cb(ok_btn, oobe_guide_close_cb, LV_EVENT_RELEASED, NULL);

  lv_obj_t *ok_lbl = lv_label_create(ok_btn);
  lv_obj_set_style_text_font(ok_lbl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(ok_lbl, en ? "Got it" : "متوجه شدم");
  lv_obj_center(ok_lbl);
}

bool oobe_guard_tab_change(int target_tab) {
  if (g_oobe_done) return true;
  if (g_oobe_stage == OOBE_STAGE_F1_ESSENTIALS) {
    if (target_tab != 0) {
      ui_toast("ابتدا طول شاخه و عرض تیغه را وارد و ذخیره کنید",
               "Please enter and save Stock Length and Kerf first", 3500);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      return false;
    }
  } else if (g_oobe_stage == OOBE_STAGE_F2_LIMITS) {
    if (target_tab != 1) {
      ui_toast("ابتدا سایز اول و آخر خط را تنظیم و ذخیره کنید",
               "Please set and save Start and End sizes first", 3500);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      return false;
    }
  }
  return true;
}

bool oobe_guard_leave_screen() {
  if (g_oobe_done) return true;
  if (g_oobe_stage == OOBE_STAGE_F1_ESSENTIALS) {
    ui_toast("ابتدا طول شاخه و عرض تیغه را وارد و ذخیره کنید",
             "Please enter and save Stock Length and Kerf first", 3500);
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    return false;
  } else if (g_oobe_stage == OOBE_STAGE_F2_LIMITS) {
    ui_toast("ابتدا سایز اول و آخر خط را تنظیم و ذخیره کنید",
             "Please set and save Start and End sizes first", 3500);
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    return false;
  }
  return true;
}

void oobe_close() {
  oobe_current_step = OOBE_STEP_IDLE;
  if (oobe_root && lv_obj_is_valid(oobe_root)) {
    lv_obj_delete(oobe_root);
    oobe_root = NULL;
    oobe_card = NULL;
    oobe_ta_comp = NULL;
    oobe_ta_admin_pin = NULL;
    oobe_ta_phone = NULL;
    oobe_ta_ssid = NULL;
    oobe_ta_pass = NULL;
    oobe_ta_otp = NULL;
    oobe_active_ta = NULL;
    oobe_kb = NULL;
    oobe_wifi_status_lbl = NULL;
  }
}

// ---------------------- کیبورد استاندارد اپل ----------------------
static const char * const oobe_kb_map_lc[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
  "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
  "a", "s", "d", "f", "g", "h", "j", "k", "l", "-", "\n",
  "ABC", "z", "x", "c", "v", "b", "n", "m", "Space", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""
};

#define O_W1 ((lv_buttonmatrix_ctrl_t)1)
#define O_W2 ((lv_buttonmatrix_ctrl_t)2)
#define O_W3 ((lv_buttonmatrix_ctrl_t)3)
#define O_DEL ((lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED | 2))
#define O_CAPS ((lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_NO_REPEAT | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | 2))

static const lv_buttonmatrix_ctrl_t oobe_kb_ctrl_lc[] = {
  O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_DEL,
  O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1,
  O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1,
  O_CAPS, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W1, O_W3, O_W1, O_W1
};

static const char * const oobe_kb_map_uc[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
  "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
  "A", "S", "D", "F", "G", "H", "J", "K", "L", "-", "\n",
  "abc", "Z", "X", "C", "V", "B", "N", "M", "Space", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""
};

static const char * const oobe_numpad_map[] = {
  "1", "2", "3", "\n",
  "4", "5", "6", "\n",
  "7", "8", "9", "\n",
  LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t oobe_numpad_ctrl[] = {
  O_W1, O_W1, O_W1,
  O_W1, O_W1, O_W1,
  O_W1, O_W1, O_W1,
  O_DEL, O_W1, (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED | 2)
};

static void oobe_focus_ta_cb(lv_event_t *e) {
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  oobe_active_ta = ta;
  if (oobe_kb && lv_obj_is_valid(oobe_kb)) {
    lv_keyboard_set_textarea(oobe_kb, ta);
    // اگر فیلد شماره همراه یا رمز یا OTP است -> نام‌پد عددی
    if (ta == oobe_ta_phone || ta == oobe_ta_admin_pin || ta == oobe_ta_otp) {
      lv_keyboard_set_map(oobe_kb, LV_KEYBOARD_MODE_USER_1, oobe_numpad_map, oobe_numpad_ctrl);
      lv_keyboard_set_mode(oobe_kb, LV_KEYBOARD_MODE_USER_1);
    } else {
      lv_keyboard_set_map(oobe_kb, LV_KEYBOARD_MODE_TEXT_LOWER, oobe_kb_map_lc, oobe_kb_ctrl_lc);
      lv_keyboard_set_mode(oobe_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    }
  }
}

// ---------------------- مرحله ۱: اتصال به وای‌فای ----------------------
static void oobe_wifi_connect_action(lv_event_t *e) {
  (void)e;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  if (!oobe_ta_ssid || !oobe_ta_pass) return;

  const char *ss = lv_textarea_get_text(oobe_ta_ssid);
  const char *pw = lv_textarea_get_text(oobe_ta_pass);
  String s_ss(ss); s_ss.trim();
  String s_pw(pw); s_pw.trim();

  if (s_ss.length() == 0) {
    ui_toast("نام شبکه وای‌فای را وارد نمایید", "Enter Wi-Fi SSID", 3000);
    return;
  }

  if (oobe_wifi_status_lbl && lv_obj_is_valid(oobe_wifi_status_lbl)) {
    lv_label_set_text(oobe_wifi_status_lbl, "در حال اتصال به مودم...");
    lv_obj_set_style_text_color(oobe_wifi_status_lbl, lv_color_hex(0x0a84ff), 0);
  }

  // ذخیره در NVS برای اتصال‌های بعدی
  Preferences p;
  p.begin("params", false);
  p.putString("wifi_ssid", s_ss);
  p.putString("wifi_pass", s_pw);
  p.end();

  WiFi.mode(WIFI_STA);
  WiFi.begin(s_ss.c_str(), s_pw.length() ? s_pw.c_str() : nullptr);

  // بررسی سریع ۲ ثانیه‌ای وضعیت اتصال
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0 < 3500)) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    configTime(3.5 * 3600, 0, "pool.ntp.org", "time.google.com", "ir.pool.ntp.org");
    g_flag_time_synced_toast = true;
    ui_toast("وای‌فای با موفقیت متصل شد", "Wi-Fi Connected Successfully", 3000);
    oobe_render_step(OOBE_STEP_MFG_INFO);
  } else {
    if (oobe_wifi_status_lbl && lv_obj_is_valid(oobe_wifi_status_lbl)) {
      lv_label_set_text(oobe_wifi_status_lbl, "اتصال ناموفق! رمز را بررسی کنید یا آفلاین ادامه دهید.");
      lv_obj_set_style_text_color(oobe_wifi_status_lbl, lv_color_hex(0xdc2626), 0);
    }
    ui_toast("عدم برقراری ارتباط با وای‌فای", "Wi-Fi connection failed", 3500);
  }
}

static void oobe_wifi_skip_action(lv_event_t *e) {
  (void)e;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  oobe_render_step(OOBE_STEP_MFG_INFO);
}

// ---------------------- مرحله ۲: اطلاعات سازنده و مدیر ----------------------
static void oobe_mfg_submit_action(lv_event_t *e) {
  (void)e;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);

  if (oobe_ta_comp) {
    const char *c = lv_textarea_get_text(oobe_ta_comp);
    String sc(c); sc.trim();
    if (sc.length() == 0) sc = "Haffler";
    save_company_name(sc);
  }

  if (oobe_ta_admin_pin) {
    const char *ap = lv_textarea_get_text(oobe_ta_admin_pin);
    String sap(ap); sap.trim();
    if (sap.length() >= 4) {
      admin_pin = sap;
      save_admin_pin();
    }
  }

  if (oobe_ta_phone) {
    const char *ph = lv_textarea_get_text(oobe_ta_phone);
    String sph(ph); sph.trim();
    char clean_ph[20] = {0};
    warranty_sanitize_phone(sph.c_str(), clean_ph, sizeof(clean_ph));
    if (clean_ph[0] != '0' || clean_ph[1] != '9' || strlen(clean_ph) != 11) {
      ui_toast("شماره موبایل مدیر باید ۱۱ رقمی و با 09 باشد", "Valid 11-digit mobile required", 3500);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      return;
    }
    s_oobe_phone = String(clean_ph);
  } else {
    s_oobe_phone = "09120000000";
  }

  // تولید کد ۵ رقمی تصادفی OTP و ارسال پیامک
  s_oobe_otp = 10000 + (esp_random() % 90000);
  s_oobe_resend_until_ms = millis() + 60000UL;

  ESP_LOGI("OOBE", "Generated Manager OTP: %u for phone %s", (unsigned)s_oobe_otp, s_oobe_phone.c_str());
  haffler_send_mfg_otp_sms(s_oobe_phone.c_str(), s_oobe_otp);

  ui_toast("کد تایید به شماره مدیر پیامک شد", "OTP code sent via SMS", 3500);
  oobe_render_step(OOBE_STEP_OTP);
}

// ---------------------- مرحله ۳: تایید OTP مدیر ----------------------
static void oobe_otp_verify_action(lv_event_t *e) {
  (void)e;
  if (!oobe_ta_otp) return;
  const char *txt = lv_textarea_get_text(oobe_ta_otp);
  String s(txt); s.trim();
  uint32_t val = (uint32_t)s.toInt();

  // کد ارسالی یا کد مستر آفلاین کارخانه ۷۴۱۲۵
  if (val == s_oobe_otp || val == 74125UL) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);

    // ذخیره شماره مدیر در تنظیمات NVS
    Preferences p;
    p.begin("haff_warr", false);
    p.putString("phone", s_oobe_phone);
    p.putString("mfg_phone", s_oobe_phone);
    p.putBool("mfg_onboarded", true);
    p.end();

    save_oobe_done(true);
    ui_toast("دستگاه با موفقیت فعال و تایید گردید", "Device verified and activated successfully", 4000);

    g_oobe_stage = OOBE_STAGE_F1_ESSENTIALS;
    oobe_close();
    GUI.tab_view_select = 0;
    GUI.pending_goto_scr = 2; // هدایت به راهنمای طول شاخه و عرض تیغه
  } else {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    ui_toast("کد تایید اشتباه است (کد مستر: 74125)", "Incorrect code (Master: 74125)", 3500);
    lv_textarea_set_text(oobe_ta_otp, "");
  }
}

void oobe_verify_master_pin() {
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  Preferences p;
  p.begin("haff_warr", false);
  p.putString("phone", "09131697476");
  p.putString("mfg_phone", "09131697476");
  p.putBool("mfg_onboarded", true);
  p.end();

  save_oobe_done(true);
  ui_toast("دستگاه با موفقیت فعال و تایید گردید", "Device verified and activated successfully", 4000);
  g_oobe_stage = OOBE_STAGE_NORMAL;
  oobe_close();
  GUI.pending_goto_scr = 1;
}

static void oobe_otp_resend_action(lv_event_t *e) {
  (void)e;
  if (millis() < s_oobe_resend_until_ms) {
    char w[64];
    snprintf(w, sizeof(w), "%lu ثانیه دیگر مجاز به ارسال مجدد هستید", (unsigned long)((s_oobe_resend_until_ms - millis()) / 1000));
    ui_toast(w, "Please wait before resend", 2500);
    return;
  }
  s_oobe_otp = 10000 + (esp_random() % 90000);
  s_oobe_resend_until_ms = millis() + 60000UL;
  haffler_send_mfg_otp_sms(s_oobe_phone.c_str(), s_oobe_otp);
  ui_toast("کد جدید مجدداً ارسال گردید", "New code resent", 3000);
}

// ---------------------- رندرینگ رابط کاربری مراحل ویزارد ----------------------
void oobe_render_step(uint8_t step) {
  oobe_current_step = step;
  if (!oobe_root || !lv_obj_is_valid(oobe_root)) {
    oobe_root = lv_obj_create(lv_layer_top());
    lv_obj_set_size(oobe_root, 480, 320);
    lv_obj_set_pos(oobe_root, 0, 0);
    lv_obj_set_style_bg_color(oobe_root, lv_color_hex(0x0f172a), 0); // پس‌زمینه دارک اپل
    lv_obj_set_style_bg_opa(oobe_root, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(oobe_root, 0, 0);
    lv_obj_set_style_border_width(oobe_root, 0, 0);
    lv_obj_set_style_pad_all(oobe_root, 0, 0);
    lv_obj_clear_flag(oobe_root, LV_OBJ_FLAG_SCROLLABLE);
  } else {
    lv_obj_clean(oobe_root);
  }

  const bool en = (GUI.system_language == 2);

  // کیبورد سراسری پایین صفحه
  oobe_kb = lv_keyboard_create(oobe_root);
  lv_obj_set_size(oobe_kb, 480, 165);
  lv_obj_align(oobe_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(oobe_kb, lv_color_hex(0x1e293b), 0);
  lv_obj_set_style_radius(oobe_kb, 0, 0);
  lv_obj_set_style_border_width(oobe_kb, 0, 0);
  lv_obj_set_style_pad_all(oobe_kb, 4, 0);
  lv_obj_set_style_pad_gap(oobe_kb, 4, 0);

  lv_obj_set_style_bg_color(oobe_kb, lv_color_hex(0x334155), LV_PART_ITEMS);
  lv_obj_set_style_text_color(oobe_kb, lv_color_hex(0xffffff), LV_PART_ITEMS);
  lv_obj_set_style_radius(oobe_kb, 8, LV_PART_ITEMS);

  // دکمه پاک کردن قرمز
  lv_obj_set_style_bg_color(oobe_kb, lv_color_hex(0xdc2626), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(oobe_kb, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_CHECKED);

  // ========================== مرحله ۱: تنظیمات وای‌فای ==========================
  if (step == OOBE_STEP_WIFI) {
    lv_obj_t *card = lv_obj_create(oobe_root);
    lv_obj_set_size(card, 460, 145);
    lv_obj_set_pos(card, 10, 8);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x334155), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_set_width(title, 430);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x38bdf8), 0);
    lv_label_set_text(title, en ? "Step 1: Connect to Wi-Fi" : "گام اول: اتصال به وای فای و اینترنت");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, -4);

    oobe_ta_ssid = lv_textarea_create(card);
    lv_obj_set_size(oobe_ta_ssid, 210, 40);
    lv_obj_set_pos(oobe_ta_ssid, 10, 26);
    lv_textarea_set_placeholder_text(oobe_ta_ssid, "Wi-Fi SSID");
    lv_textarea_set_one_line(oobe_ta_ssid, true);
    lv_obj_set_style_text_font(oobe_ta_ssid, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(oobe_ta_ssid, oobe_focus_ta_cb, LV_EVENT_FOCUSED, NULL);

    String saved_ss, saved_pw;
    Preferences p;
    p.begin("params", true);
    saved_ss = p.getString("wifi_ssid", "");
    saved_pw = p.getString("wifi_pass", "");
    p.end();
    if (saved_ss.length() == 0) {
      p.begin("chone", true);
      saved_ss = p.getString("ws0", "");
      saved_pw = p.getString("wp0", "");
      p.end();
    }
    if (saved_ss.length()) lv_textarea_set_text(oobe_ta_ssid, saved_ss.c_str());

    oobe_ta_pass = lv_textarea_create(card);
    lv_obj_set_size(oobe_ta_pass, 210, 40);
    lv_obj_set_pos(oobe_ta_pass, 230, 26);
    lv_textarea_set_placeholder_text(oobe_ta_pass, "Password");
    lv_textarea_set_one_line(oobe_ta_pass, true);
    lv_textarea_set_password_mode(oobe_ta_pass, true);
    lv_obj_set_style_text_font(oobe_ta_pass, &lv_font_montserrat_16, 0);
    if (saved_pw.length()) lv_textarea_set_text(oobe_ta_pass, saved_pw.c_str());
    lv_obj_add_event_cb(oobe_ta_pass, oobe_focus_ta_cb, LV_EVENT_FOCUSED, NULL);

    lv_obj_t *btn_conn = lv_button_create(card);
    lv_obj_set_size(btn_conn, 210, 38);
    lv_obj_set_pos(btn_conn, 10, 74);
    lv_obj_set_style_bg_color(btn_conn, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_radius(btn_conn, 8, 0);
    lv_obj_add_event_cb(btn_conn, oobe_wifi_connect_action, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_conn = lv_label_create(btn_conn);
    lv_label_set_text(l_conn, en ? "Connect" : "اتصال به مودم");
    lv_obj_set_style_text_font(l_conn, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_center(l_conn);

    lv_obj_t *btn_skip = lv_button_create(card);
    lv_obj_set_size(btn_skip, 210, 38);
    lv_obj_set_pos(btn_skip, 230, 74);
    lv_obj_set_style_bg_color(btn_skip, lv_color_hex(0x475569), 0);
    lv_obj_set_style_radius(btn_skip, 8, 0);
    lv_obj_add_event_cb(btn_skip, oobe_wifi_skip_action, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_skip = lv_label_create(btn_skip);
    lv_label_set_text(l_skip, en ? "Skip (Offline)" : "ادامه بدون اینترنت (آفلاین)");
    lv_obj_set_style_text_font(l_skip, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_center(l_skip);

    oobe_active_ta = oobe_ta_ssid;
    lv_keyboard_set_textarea(oobe_kb, oobe_ta_ssid);
    lv_keyboard_set_map(oobe_kb, LV_KEYBOARD_MODE_TEXT_LOWER, oobe_kb_map_lc, oobe_kb_ctrl_lc);
    lv_keyboard_set_mode(oobe_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
  }

  // ========================== مرحله ۲: نام شرکت، رمز و شماره مدیر ==========================
  else if (step == OOBE_STEP_MFG_INFO) {
    lv_obj_t *card = lv_obj_create(oobe_root);
    lv_obj_set_size(card, 460, 145);
    lv_obj_set_pos(card, 10, 8);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x334155), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_set_width(title, 430);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x38bdf8), 0);
    lv_label_set_text(title, en ? "Step 2: Manufacturer & Manager Info" : "گام دوم: اطلاعات سازنده و تلفن مدیر");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, -4);

    // فیلد ۱: نام شرکت / کارگاه
    oobe_ta_comp = lv_textarea_create(card);
    lv_obj_set_size(oobe_ta_comp, 140, 40);
    lv_obj_set_pos(oobe_ta_comp, 5, 26);
    lv_textarea_set_placeholder_text(oobe_ta_comp, "Company");
    lv_textarea_set_text(oobe_ta_comp, (g_company_name.length() > 0 && g_company_name != "HAFFLER") ? g_company_name.c_str() : "Haffler");
    lv_textarea_set_one_line(oobe_ta_comp, true);
    lv_obj_set_style_text_font(oobe_ta_comp, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(oobe_ta_comp, oobe_focus_ta_cb, LV_EVENT_FOCUSED, NULL);

    // فیلد ۲: رمز مدیریت (پیش‌فرض 123456)
    oobe_ta_admin_pin = lv_textarea_create(card);
    lv_obj_set_size(oobe_ta_admin_pin, 120, 40);
    lv_obj_set_pos(oobe_ta_admin_pin, 155, 26);
    lv_textarea_set_placeholder_text(oobe_ta_admin_pin, "Admin PIN");
    lv_textarea_set_text(oobe_ta_admin_pin, admin_pin.length() ? admin_pin.c_str() : "123456");
    lv_textarea_set_one_line(oobe_ta_admin_pin, true);
    lv_obj_set_style_text_font(oobe_ta_admin_pin, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(oobe_ta_admin_pin, oobe_focus_ta_cb, LV_EVENT_FOCUSED, NULL);

    // فیلد ۳: شماره موبایل مدیر
    oobe_ta_phone = lv_textarea_create(card);
    lv_obj_set_size(oobe_ta_phone, 150, 40);
    lv_obj_set_pos(oobe_ta_phone, 285, 26);
    lv_textarea_set_placeholder_text(oobe_ta_phone, "09xxxxxxxxx");
    lv_textarea_set_text(oobe_ta_phone, s_oobe_phone.length() ? s_oobe_phone.c_str() : "09");
    lv_textarea_set_one_line(oobe_ta_phone, true);
    lv_textarea_set_max_length(oobe_ta_phone, 11);
    lv_obj_set_style_text_font(oobe_ta_phone, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(oobe_ta_phone, oobe_focus_ta_cb, LV_EVENT_FOCUSED, NULL);

    // دکمه ارسال پیامک و ادامه
    lv_obj_t *btn_next = lv_button_create(card);
    lv_obj_set_size(btn_next, 430, 40);
    lv_obj_set_pos(btn_next, 5, 74);
    lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_radius(btn_next, 8, 0);
    lv_obj_add_event_cb(btn_next, oobe_mfg_submit_action, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l_next = lv_label_create(btn_next);
    lv_label_set_text(l_next, en ? "Send OTP Code via SMS" : "ارسال پیامک کد تایید و ادامه");
    lv_obj_set_style_text_font(l_next, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
    lv_obj_center(l_next);

    oobe_active_ta = oobe_ta_phone;
    lv_keyboard_set_textarea(oobe_kb, oobe_ta_phone);
    lv_keyboard_set_map(oobe_kb, LV_KEYBOARD_MODE_USER_1, oobe_numpad_map, oobe_numpad_ctrl);
    lv_keyboard_set_mode(oobe_kb, LV_KEYBOARD_MODE_USER_1);
  }

  // ========================== مرحله ۳: ورود کد OTP ==========================
  else if (step == OOBE_STEP_OTP) {
    lv_obj_t *card = lv_obj_create(oobe_root);
    lv_obj_set_size(card, 460, 145);
    lv_obj_set_pos(card, 10, 8);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x334155), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_set_width(title, 430);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x38bdf8), 0);
    lv_label_set_text(title, en ? "Step 3: Enter 5-Digit Verification Code" : "گام سوم: تایید پیامکی کد فعال سازی");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, -4);

    oobe_ta_otp = lv_textarea_create(card);
    lv_obj_set_size(oobe_ta_otp, 180, 44);
    lv_obj_set_pos(oobe_ta_otp, 20, 26);
    lv_textarea_set_placeholder_text(oobe_ta_otp, "-----");
    lv_textarea_set_one_line(oobe_ta_otp, true);
    lv_textarea_set_max_length(oobe_ta_otp, 5);
    lv_obj_set_style_text_align(oobe_ta_otp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(oobe_ta_otp, &lv_font_montserrat_24, 0);
    lv_obj_add_event_cb(oobe_ta_otp, oobe_otp_verify_action, LV_EVENT_READY, NULL);

    lv_obj_t *btn_ver = lv_button_create(card);
    lv_obj_set_size(btn_ver, 105, 44);
    lv_obj_set_pos(btn_ver, 215, 26);
    lv_obj_set_style_bg_color(btn_ver, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_radius(btn_ver, 8, 0);
    lv_obj_add_event_cb(btn_ver, oobe_otp_verify_action, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_ver = lv_label_create(btn_ver);
    lv_label_set_text(l_ver, en ? "Verify" : "تایید");
    lv_obj_set_style_text_font(l_ver, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
    lv_obj_center(l_ver);

    lv_obj_t *btn_resend = lv_button_create(card);
    lv_obj_set_size(btn_resend, 115, 44);
    lv_obj_set_pos(btn_resend, 325, 26);
    lv_obj_set_style_bg_color(btn_resend, lv_color_hex(0x475569), 0);
    lv_obj_set_style_radius(btn_resend, 8, 0);
    lv_obj_add_event_cb(btn_resend, oobe_otp_resend_action, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_resend = lv_label_create(btn_resend);
    lv_label_set_text(l_resend, en ? "Resend" : "ارسال مجدد");
    lv_obj_set_style_text_font(l_resend, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
    lv_obj_center(l_resend);

    lv_obj_t *hint = lv_label_create(card);
    lv_obj_set_width(hint, 430);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x94a3b8), 0);
    lv_label_set_text(hint, "Offline Master PIN: 74125");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);

    oobe_active_ta = oobe_ta_otp;
    lv_keyboard_set_textarea(oobe_kb, oobe_ta_otp);
    lv_keyboard_set_map(oobe_kb, LV_KEYBOARD_MODE_USER_1, oobe_numpad_map, oobe_numpad_ctrl);
    lv_keyboard_set_mode(oobe_kb, LV_KEYBOARD_MODE_USER_1);
  }
}

static inline void oobe_wizard_start() {
  if (WiFi.status() == WL_CONNECTED) {
    oobe_render_step(OOBE_STEP_MFG_INFO);
  } else {
    oobe_render_step(OOBE_STEP_WIFI);
  }
}

static inline void oobe_wizard_tick() {
  if (!oobe_is_active()) return;
  if (oobe_current_step == OOBE_STEP_WIFI && WiFi.status() == WL_CONNECTED) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    configTime(3.5 * 3600, 0, "pool.ntp.org", "time.google.com", "ir.pool.ntp.org");
    g_flag_time_synced_toast = true;
    ui_toast("وای‌فای با موفقیت متصل شد", "Wi-Fi Connected Successfully", 2500);
    oobe_render_step(OOBE_STEP_MFG_INFO);
  }
}

static inline void oobe_wizard_check_start() {
  Preferences pw;
  pw.begin("haff_warr", true);
  bool mfg_onboarded = pw.getBool("mfg_onboarded", false);
  pw.end();

  if (!mfg_onboarded) {
    g_oobe_done = false;
  }

  if (!g_oobe_done && !g_update_mode && oobe_current_step == OOBE_STEP_IDLE) {
    if (g_oobe_stage == OOBE_STAGE_NORMAL) {
      oobe_wizard_start();
    }
  }
}
