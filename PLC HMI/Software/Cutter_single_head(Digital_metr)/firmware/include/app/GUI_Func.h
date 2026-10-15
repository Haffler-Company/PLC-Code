#include <bb_captouch.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include "fonts.h"
#include "cut_stats.h"

extern bool printer_enabled;
extern uint8_t printer_model;
extern volatile bool cut_start_ui_modal_active;

enum BuzzerPattern : uint8_t {
  BUZZER_PATTERN_NONE = 0,
  BUZZER_PATTERN_SHORT = 1,        // 30ms crisp touch tap
  BUZZER_PATTERN_SUCCESS = 2,      // 2 x 45ms with 35ms pause (confirm/success)
  BUZZER_PATTERN_WARN = 3,         // 150ms medium warning tone
  BUZZER_PATTERN_LONG = 4,         // 280ms error/reject tone
  BUZZER_PATTERN_SINGLE_CLICK = 5, // 35ms single beep for speaker toggle
  BUZZER_PATTERN_BOOT = 6,         // 2 x 35ms cheerful startup chime
  BUZZER_PATTERN_COMPLETE = 7,     // 3 x 40ms with 35ms pause (cut done / homing done)
  BUZZER_PATTERN_STEP_UP = 8,      // 20ms tick for + increment
  BUZZER_PATTERN_STEP_DOWN = 9     // 20ms tick for - decrement
};
bool buzzer_request_pattern(uint8_t pattern);

// ---- تعریف‌های ui_extras (پیاده‌سازی بعد از این فایل include می‌شود) ----
void ui_toast(const char *fa, const char *en, uint32_t ms);
void usb_keyboard_set_numlock_led(bool on);
void usb_keyboard_set_numlock_led_force(bool on);
void numpad_open(int ctx);
void numpad_close();
void numpad_sync_screen();
bool f1_num_entry_active();
bool f1_try_num_digit(char d);
void f1_num_entry_digit(char d);
void f1_num_entry_backspace();
void f1_num_entry_commit();
bool main_queue_mode_active();
void request_exit_queue_mode_prompt(uint8_t why);
void prcs_exit_queue_mode_prompt();
void cut_start_wrong_screen_ui_tick();
bool cut_start_wrong_screen_modal_active();
void opt_main_bar_lbl_sync();
void opt_main_bar_lbl_hide();
void opt_notify_list_edited();
bool opt_consume_home_preserve();
void jog_manual_badge_hide();
void jog_manual_badge_sync();
void jog_manual_badge_invalidate();
void main_clear_destination_sp();
void main_restore_destination_to_current_pos();
void main_bind_destination_scrub_handlers();
bool main_destination_scrub_moved();  // true اگر اسکراب واقعاً عدد را عوض کرده — راهنما نیاید
void opt_finish_to_manual(bool show_toast);
int opt_current_run_qty();
int opt_run_qty_from(int start);
void vkb_settings_sync();
void vkb_poll_usb();
void vkb_apply_user_toggle(bool want_on);
void vkb_apply_effective(bool allow_open);
bool is_toast_object(lv_obj_t *obj);
bool usb_keyboard_present();
void usb_keyboard_ui_drain();
void prcs_usb_mouse_extras();
void prcs_opt_calc_progress();
void prcs_opt_refresh();
void f1_lang_btn_sync();
void f1_apply_language_texts();
void settings_hide_eez_clutter();  // ویجت/تصویر قدیمی EEZ تنظیمات — همیشه پنهان/بازنشسته
void blade_err_badge_sync();      // دایره اصلاح خطا صفحه اصلی
void settings_mark_dirty();
void settings_on_tab_changed();
void settings_on_leave_settings();
void settings_on_enter_settings();
void settings_after_save_all();
void settings_flush_pending_edits();
#ifndef OOBE_STAGE_DEFINED
#define OOBE_STAGE_DEFINED
enum OobeStage : uint8_t {
  OOBE_STAGE_NORMAL = 0,
  OOBE_STAGE_F1_ESSENTIALS = 1,
  OOBE_STAGE_F2_LIMITS = 2
};
#endif
bool oobe_guard_tab_change(int target_tab);
extern uint8_t g_oobe_stage;
extern bool g_oobe_done;
void admin_tabs_reflow();
void f4_dir_buttons_sync();
void f4_clock_commit_from_ui();  // ساعت/تاریخ F4 با Save هدر
void f4_clock_live_tick();       // [UI/Task2] ساعت/دقیقهٔ F4 تا پیش از ویرایش دستی
void f4_flush_open_edits_for_save();
bool f4_has_pending_header_save_edit();
void f4_chain_pending_begin();
void f4_chain_pending_discard();
bool f4_chain_pending_has_changes();
bool f4_chain_commit_pending_on_header_save();
bool f4_panel_adjust_numeric(lv_obj_t *obj, int dir);
void f3_q012_sync_active_green();
void device_lock_ui_tick();
void f5_dev_reflow();
void f5_dev_runtime_tick();
void update_flow_start_from_ui();
bool factory_snapshot_exists();
void f2_caret_tick();
void f2_sync_line_box_colors();
void f2_begin_edit_start_line();
void f2_begin_edit_end_line();
void f2_on_digit_typed();
bool f2_try_append_char(char d);
void f2_commit_start_value();
void f2_commit_end_value();
void f2_on_backspace();
void f2_end_edit();
void f2_apply_jog_slider_from_value(int ramp_ms);
int f2_read_jog_ramp_ms();
static void cancel_screen3_pending_edit();
bool commit_main_sp(bool do_move);
static bool main_is_at_destination();
static inline int page_base() { return GUI.list_page; }

extern volatile bool ui_help_suppress_release;
static inline bool ui_help_take_suppress(void) {
  if (!ui_help_suppress_release) return false;
  ui_help_suppress_release = false;
  return true;
}

// تعویض امن صفحه: هرگز lv_screen_load را داخل callback لمس صدا نزن
void ui_resume_mark_current(int scr);  // پایین‌تر — RTC اسنپ‌شات
static inline void request_goto_screen(int scr) {
  numpad_close();
  cancel_screen3_pending_edit();
  GUI.pending_goto_scr = scr;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  if (scr >= 0 && scr <= 3) ui_resume_mark_current(scr);
}

void list_page_change(int dir);
void list_show_row(int idx);
void list_ensure_row_visible(int idx);
void list_zero_row(int idx);
bool list_ui_was_dragged(void);
int list_rows_on_screen(void);

#define TOUCH_ORIENTATION 270

#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_INT -1
#define TOUCH_RST -1
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);
BBCapTouch bbct;
TwoWire touchWire = TwoWire(0);
extern SemaphoreHandle_t touchWireMutex;
bool touch_ready = false;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;
static bool display_dma_ready = false;
static bool display_dma_attempted = false;
static bool display_dma_pending = false;
static bool display_dma_transaction_open = false;
static uint32_t display_dma_enable_at_ms = 0;
static uint32_t display_dma_transfer_started_us = 0;
static uint32_t display_dma_wait_total_us = 0;
static uint32_t display_dma_transfer_count = 0;
static uint32_t display_dma_max_transfer_us = 0;
static constexpr uint32_t DISPLAY_DMA_TIMEOUT_US = 50000UL;
static constexpr uint32_t DISPLAY_DMA_MIN_PIXELS = 64UL;
static constexpr uint32_t DISPLAY_DMA_RECOVERY_MAGIC = 0xD4A61E5AUL;
static constexpr uint32_t WEB_PORTAL_RESUME_MAGIC = 0x57A90F31UL;
RTC_NOINIT_ATTR static uint32_t display_dma_recovery_magic;
RTC_NOINIT_ATTR static uint32_t display_dma_recovery_magic_inv;
// ریست امن / هنگ: همان صفحه + مود/راهنما را بعد از بوت کوتاه برگردان
RTC_NOINIT_ATTR static uint32_t ui_resume_scr_magic;
RTC_NOINIT_ATTR static uint32_t ui_resume_scr_magic_inv;
RTC_NOINIT_ATTR static uint32_t ui_resume_view_code;
RTC_NOINIT_ATTR static volatile uint32_t web_portal_resume_magic;
RTC_NOINIT_ATTR static volatile uint32_t web_portal_resume_magic_inv;
RTC_NOINIT_ATTR static volatile uint32_t web_portal_resume_committed_on;
static int dma_ui_resume_scr = -1;  // -1 = عادی؛ 0..3 بعد از recovery

enum {
  UI_RESUME_F_NUMLOCK = 1u << 0,
  UI_RESUME_F_LIST = 1u << 1,
  UI_RESUME_F_OPT = 1u << 2,
  UI_RESUME_F_OPT_PANEL = 1u << 3,
  UI_RESUME_F_HELP_F1 = 1u << 4,
  UI_RESUME_F_HELP_LIST = 1u << 5,
  UI_RESUME_F_HELP_OPT = 1u << 6,
  UI_RESUME_F_F4_UNLOCKED = 1u << 7,
  UI_RESUME_F_F5_OV = 1u << 8,
  UI_RESUME_F_F3_PINMAP = 1u << 9,
  UI_RESUME_F_F3_PINHELP = 1u << 10,
};

enum {
  UI_RESUME_VIEW_NONE = 0,
  UI_RESUME_VIEW_F1_HELP = 1,
  UI_RESUME_VIEW_LIST_HELP = 2,
  UI_RESUME_VIEW_OPT_PANEL = 3,
  UI_RESUME_VIEW_OPT_HELP = 4,
  UI_RESUME_VIEW_F5_OVERLAY = 5,
  UI_RESUME_VIEW_F3_PINMAP = 6,
  UI_RESUME_VIEW_F3_PINHELP = 7,
};

// اسنپ‌شات RAM برای اعمال بعد از loadScreen (قبل از ui_resume_clear)
static uint32_t ui_resume_apply_flags = 0;
static int32_t ui_resume_apply_list_idx = -1;
static int32_t ui_resume_apply_list_qty = 0;
static int32_t ui_resume_apply_opt_cur = 0;
static int32_t ui_resume_apply_settings_tab = 0;
static int32_t ui_resume_apply_list_page = 0;
static int32_t ui_resume_apply_opt_bar_page = 0;
static int32_t ui_resume_apply_f5_ov = 0;
static int32_t ui_resume_apply_f3_pinmap = -1;
static int32_t ui_resume_apply_f3_help_id = 0;
static int ui_resume_apply_scr = -1;  // صفحهٔ هدف resume (بعد از 0→1)
static bool ui_resume_apply_pending = false;
static bool ui_resume_boot_latched = false;
static bool ui_resume_boot_valid = false;
static int ui_resume_boot_scr = -1;
static bool web_portal_resume_boot_pending = false;
static bool ui_resume_opt_pending = false;
static bool ui_resume_opt_started = false;
static bool ui_resume_opt_want_panel = false;
static bool ui_resume_opt_want_help = false;
static bool ui_resume_opt_to_main = false;  // بعد از calc: پنل نساز؛ مستقیم main

// ui_extras / admin / f5 — تشخیص باز بودن بخش‌ها
bool ui_resume_help_f1_open();
bool ui_resume_help_list_open();
bool ui_resume_help_opt_open();
bool ui_resume_opt_panel_open();
int ui_resume_query_opt_bar_page();
int ui_resume_query_f5_ov_kind();
bool ui_resume_f4_is_unlocked();
void ui_resume_restore_f5_ov(int kind);
void ui_resume_restore_f4_unlocked();
void ui_resume_restore_opt_bar_page(int page);
bool ui_resume_f3_pinmap_is_open();
int ui_resume_f3_pinmap_pack();
int ui_resume_f3_pinhelp_id();
void ui_resume_restore_f3_pinmap(bool is_output, int role);
void ui_resume_restore_f3_pinhelp(int id);
void ui_resume_apply_pending_state();
void prcs_ui_resume_opt();
static inline bool ui_resume_has_pending_apply(void) { return ui_resume_apply_pending; }

// فقط یک کد ۳۲ بیتی در RTC: صفحه، تب، نوع زیرصفحه و شناسهٔ آن.
// هیچ مقدار فرم، ردیف، تعداد، موقعیت یا دادهٔ کاری ذخیره نمی‌شود.
void ui_resume_mark_current(int scr) {
  // تا اعمال resume تمام نشده، اسنپ‌شات بوت را با scr=0 خراب نکن
  if (ui_resume_apply_pending) return;
  if (dma_ui_resume_scr >= 0) return;
  if (scr < 0 || scr > 3) return;
  int tab = 0;
  if (scr == 2 && objects.tabview2_1) {
    tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
    if (tab < 0) tab = 0;
    if (tab > 4) tab = 4;
  }
  int view = UI_RESUME_VIEW_NONE;
  int arg = 0;
  if (ui_resume_help_f1_open()) {
    view = UI_RESUME_VIEW_F1_HELP;
  } else if (ui_resume_help_list_open()) {
    view = UI_RESUME_VIEW_LIST_HELP;
  } else if (ui_resume_help_opt_open()) {
    view = UI_RESUME_VIEW_OPT_HELP;
  } else if (ui_resume_opt_panel_open()) {
    view = UI_RESUME_VIEW_OPT_PANEL;
  }
  const int f5_kind = ui_resume_query_f5_ov_kind();
  if (f5_kind > 0) {
    view = UI_RESUME_VIEW_F5_OVERLAY;
    arg = f5_kind;
  }
  if (ui_resume_f3_pinmap_is_open()) {
    view = UI_RESUME_VIEW_F3_PINMAP;
    arg = ui_resume_f3_pinmap_pack();
  } else {
    const int hid = ui_resume_f3_pinhelp_id();
    if (hid > 0) {
      view = UI_RESUME_VIEW_F3_PINHELP;
      arg = hid;
    }
  }
  if (arg < 0) arg = 0;
  if (arg > 0x1FF) arg = 0x1FF;
  const uint32_t code = 0xA7000000u |
      ((uint32_t)scr & 0x3u) |
      (((uint32_t)tab & 0x7u) << 2) |
      (((uint32_t)view & 0xFu) << 5) |
      (((uint32_t)arg & 0x1FFu) << 9);
  if (ui_resume_scr_magic == DISPLAY_DMA_RECOVERY_MAGIC &&
      ui_resume_scr_magic_inv == ~DISPLAY_DMA_RECOVERY_MAGIC &&
      ui_resume_view_code == code)
    return;
  ui_resume_view_code = code;
  ui_resume_scr_magic = DISPLAY_DMA_RECOVERY_MAGIC;
  ui_resume_scr_magic_inv = ~DISPLAY_DMA_RECOVERY_MAGIC;
  // فقط RTC برای هنگ؛ NVS/Flash عمداً نوشته نمی‌شود.
}

// هر تغییر صفحه/تب/راهنما/بخش → فوری در RTC
void ui_resume_checkpoint(void) {
  ui_resume_mark_current(GUI.scr_id);
}

static void ui_resume_clear() {
  ui_resume_scr_magic = 0;
  ui_resume_scr_magic_inv = 0;
  ui_resume_view_code = 0;
  // id/اسنپ‌شات apply_* را پاک نکن — بدون magic دیگر از RTC خوانده نمی‌شود
}

// فقط علت‌های «هنگ» — نه برق، نه دکمه ریست سخت‌افزاری
static bool ui_resume_reset_is_hang(esp_reset_reason_t rr) {
  switch (rr) {
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
    case ESP_RST_SW:  // esp_restart بعد از تایم‌اوت DMA / ریکاوری
#ifdef ESP_RST_CPU_LOCKUP
    case ESP_RST_CPU_LOCKUP:
#endif
      return true;
    default:
      return false;
  }
}

// [RT/WebPortalTask + early boot] Portal state is transient hang-recovery state.
// A valid record means the AP completed startup. Clearing the record represents
// an accepted stop and also consumes a boot restore before its asynchronous
// restart, so a reset during that transition cannot replay a half-start.
static void web_portal_resume_commit(bool committed_on) {
  if (committed_on) {
    web_portal_resume_committed_on = 1;
    web_portal_resume_magic = WEB_PORTAL_RESUME_MAGIC;
    web_portal_resume_magic_inv = ~WEB_PORTAL_RESUME_MAGIC;
  } else {
    web_portal_resume_magic = 0;
    web_portal_resume_magic_inv = 0;
    web_portal_resume_committed_on = 0;
  }
}

static bool web_portal_resume_take_boot_request() {
  if (!web_portal_resume_boot_pending) return false;
  web_portal_resume_boot_pending = false;
  return true;
}

// فقط بعد از هنگ: RTC سالم + (دلیل هنگ یا مارکر DMA) → همان صفحه+بخش
static bool ui_resume_boot_should_apply(int *out_scr) {
  if (out_scr) *out_scr = -1;

  const esp_reset_reason_t rr = esp_reset_reason();
  const bool dma_hang =
      (display_dma_recovery_magic == DISPLAY_DMA_RECOVERY_MAGIC &&
       display_dma_recovery_magic_inv == ~DISPLAY_DMA_RECOVERY_MAGIC);
  const bool hang = ui_resume_reset_is_hang(rr) || dma_hang;

  if (!hang) {
    // خاموش/روشن یا ریست سخت‌افزاری: صفحهٔ قبلی نیاید
    ui_resume_clear();
    ui_resume_apply_pending = false;
    ui_resume_apply_scr = -1;
    ets_printf("[UIRESUME] skip rr=%d (not hang)\n", (int)rr);
    return false;
  }

  bool ok = (ui_resume_scr_magic == DISPLAY_DMA_RECOVERY_MAGIC &&
             ui_resume_scr_magic_inv == ~DISPLAY_DMA_RECOVERY_MAGIC &&
             (ui_resume_view_code & 0xFF000000u) == 0xA7000000u);
  if (!ok) {
    ui_resume_clear();
    ui_resume_apply_pending = false;
    ui_resume_apply_scr = -1;
    ets_printf("[UIRESUME] skip rr=%d dma=%d (no rtc snap)\n", (int)rr, dma_hang ? 1 : 0);
    return false;
  }

  int scr = (int)(ui_resume_view_code & 0x3u);
  if (scr < 0 || scr > 3) {
    ui_resume_clear();
    ui_resume_apply_scr = -1;
    return false;
  }
  // صفحهٔ سبز بوت → اصلی (همان تبدیل prcs_splash)
  if (scr == 0) scr = 1;
  const uint32_t code = ui_resume_view_code;
  const int view = (int)((code >> 5) & 0xFu);
  const int arg = (int)((code >> 9) & 0x1FFu);
  ui_resume_apply_flags = 0;
  ui_resume_apply_list_idx = -1;
  ui_resume_apply_list_qty = 0;
  ui_resume_apply_opt_cur = 0;
  ui_resume_apply_settings_tab = (int32_t)((code >> 2) & 0x7u);
  ui_resume_apply_list_page = 0;
  ui_resume_apply_opt_bar_page = 0;
  ui_resume_apply_f5_ov = 0;
  ui_resume_apply_f3_pinmap = -1;
  ui_resume_apply_f3_help_id = 0;
  if (view == UI_RESUME_VIEW_F1_HELP) ui_resume_apply_flags |= UI_RESUME_F_HELP_F1;
  else if (view == UI_RESUME_VIEW_LIST_HELP) ui_resume_apply_flags |= UI_RESUME_F_HELP_LIST;
  else if (view == UI_RESUME_VIEW_OPT_PANEL)
    ui_resume_apply_flags |= UI_RESUME_F_OPT | UI_RESUME_F_OPT_PANEL;
  else if (view == UI_RESUME_VIEW_OPT_HELP)
    ui_resume_apply_flags |= UI_RESUME_F_OPT | UI_RESUME_F_OPT_PANEL | UI_RESUME_F_HELP_OPT;
  else if (view == UI_RESUME_VIEW_F5_OVERLAY) {
    ui_resume_apply_flags |= UI_RESUME_F_F5_OV;
    ui_resume_apply_f5_ov = arg;
  } else if (view == UI_RESUME_VIEW_F3_PINMAP) {
    ui_resume_apply_flags |= UI_RESUME_F_F3_PINMAP;
    ui_resume_apply_f3_pinmap = arg;
  } else if (view == UI_RESUME_VIEW_F3_PINHELP) {
    ui_resume_apply_flags |= UI_RESUME_F_F3_PINHELP;
    ui_resume_apply_f3_help_id = arg;
  }
  ui_resume_apply_scr = scr;
  ui_resume_apply_pending = true;
  ui_resume_opt_pending = false;
  ui_resume_opt_started = false;
  if (out_scr) *out_scr = scr;
  ets_printf("[UIRESUME] apply scr=%d tab=%d flg=0x%lx rr=%d dma=%d\n",
             scr, (int)ui_resume_apply_settings_tab, (unsigned long)ui_resume_apply_flags,
             (int)rr, dma_hang ? 1 : 0);
  return true;
}

// باید اولین کار setup باشد: قبل از Serial/Preferences/LVGL/widgetها.
// پس از این latch، checkpointهای راه‌اندازی به‌خاطر apply_pending نمی‌توانند
// صفحهٔ واقعی قبل از هنگ را با GUI.scr_id پیش‌فرض صفر overwrite کنند.
void ui_resume_boot_latch_early() {
  if (ui_resume_boot_latched) return;
  ui_resume_boot_latched = true;
  const esp_reset_reason_t rr = esp_reset_reason();
  const bool dma_hang =
      (display_dma_recovery_magic == DISPLAY_DMA_RECOVERY_MAGIC &&
       display_dma_recovery_magic_inv == ~DISPLAY_DMA_RECOVERY_MAGIC);
  const bool hang = ui_resume_reset_is_hang(rr) || dma_hang;
  const bool portal_committed_on =
      (web_portal_resume_magic == WEB_PORTAL_RESUME_MAGIC &&
       web_portal_resume_magic_inv == ~WEB_PORTAL_RESUME_MAGIC &&
       web_portal_resume_committed_on == 1);
  web_portal_resume_boot_pending = hang && portal_committed_on;
  web_portal_resume_commit(false);  // consume before the asynchronous restart
  int scr = -1;
  ui_resume_boot_valid = ui_resume_boot_should_apply(&scr);
  ui_resume_boot_scr = ui_resume_boot_valid ? scr : -1;
  ets_printf("[UIRESUME] early latch valid=%d scr=%d\n",
             ui_resume_boot_valid ? 1 : 0, ui_resume_boot_scr);
}

bool ui_resume_boot_get_latched(int *out_scr) {
  if (out_scr) *out_scr = ui_resume_boot_valid ? ui_resume_boot_scr : -1;
  return ui_resume_boot_latched && ui_resume_boot_valid;
}
static volatile int usb_mouse_x = SCREEN_WIDTH / 2;
static volatile int usb_mouse_y = SCREEN_HEIGHT / 2;
static volatile bool usb_mouse_left_down = false;
static volatile bool usb_mouse_seen = false;
static volatile int usb_mouse_wheel_delta = 0;
static volatile bool usb_mouse_right_click_pending = false;
static volatile bool usb_mouse_back_pending = false;
static lv_obj_t *usb_mouse_cursor = NULL;
static lv_group_t *keyboard_nav_group = NULL;
static volatile uint32_t keyboard_nav_key = 0;
static volatile bool keyboard_nav_key_pending = false;
static volatile bool keyboard_nav_key_down = false;
static int keyboard_nav_registered_scr = -99;
static volatile bool panel_five_nav_active = false;
static lv_obj_t *panel_five_nav_focus = NULL;
static lv_obj_t *panel_five_nav_marker = NULL;
static lv_obj_t *panel_five_nav_scroll = NULL;
static bool panel_five_nav_keypad_engaged = false;
static bool panel_five_nav_settings_page_engaged = false;
static int panel_five_nav_list_row = -1;
static uint32_t panel_five_nav_last_action_ms = 0;
static bool panel_save_feedback_pending = false;
bool f3_pinmap_panel_adjust(lv_obj_t *obj, int dir);
bool f3_try_pin_help(lv_obj_t *obj);
bool f3_pinmap_dialog_is_open();
bool f3_pinmap_panel_cancel();
static int panel_five_nav_list_row_of(lv_obj_t *obj);
static bool panel_five_nav_is_f_tab(lv_obj_t *obj);
static void panel_five_nav_adjust(int dir);
static void panel_five_nav_start_action();
static void panel_five_nav_stop_action();

static lv_obj_t *panel_five_nav_active_settings_page() {
  if (GUI.scr_id != 2 || !objects.tabview2_1) return NULL;
  lv_obj_t *content = lv_tabview_get_content(objects.tabview2_1);
  if (!content) return NULL;
  const int tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
  if (tab < 0 || (uint32_t)tab >= lv_obj_get_child_count(content)) return NULL;
  return lv_obj_get_child(content, (uint32_t)tab);
}

static bool panel_five_nav_is_active() { return panel_five_nav_active; }

static bool panel_five_nav_visible(lv_obj_t *obj) {
  if (!obj || !lv_obj_is_valid(obj) || !lv_obj_is_visible(obj) ||
      lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) || lv_obj_has_state(obj, LV_STATE_DISABLED))
    return false;
  lv_area_t a;
  lv_obj_get_coords(obj, &a);
  return a.x2 >= 0 && a.y2 >= 0 && a.x1 < SCREEN_WIDTH && a.y1 < SCREEN_HEIGHT;
}

static bool panel_five_nav_available(lv_obj_t *obj) {
  if (!obj || !lv_obj_is_valid(obj)) return false;
  for (lv_obj_t *p = obj; p; p = lv_obj_get_parent(p)) {
    if (!lv_obj_is_valid(p) || lv_obj_has_flag(p, LV_OBJ_FLAG_HIDDEN) ||
        lv_obj_has_state(p, LV_STATE_DISABLED))
      return false;
  }
  return true;
}

static bool panel_five_nav_text_scroll(lv_obj_t *obj) {
  if (!obj || !lv_obj_has_flag(obj, LV_OBJ_FLAG_SCROLLABLE) ||
      (lv_obj_get_scroll_top(obj) <= 0 && lv_obj_get_scroll_bottom(obj) <= 0))
    return false;
  const uint32_t n = lv_obj_get_child_count(obj);
  for (uint32_t i = 0; i < n; ++i) {
    lv_obj_t *child = lv_obj_get_child(obj, i);
    if (lv_obj_check_type(child, &lv_button_class) ||
        lv_obj_check_type(child, &lv_buttonmatrix_class) ||
        lv_obj_check_type(child, &lv_keyboard_class) ||
        lv_obj_check_type(child, &lv_slider_class) ||
        lv_obj_check_type(child, &lv_switch_class))
      return false;
  }
  return true;
}

static bool panel_five_nav_candidate(lv_obj_t *obj) {
  const bool allow_offscreen = f3_pinmap_dialog_is_open() ||
      (GUI.scr_id == 2 && panel_five_nav_settings_page_engaged);
  if (allow_offscreen ? !panel_five_nav_available(obj) : !panel_five_nav_visible(obj))
    return false;
  if (GUI.scr_id == 2 && objects.tabview2_1 &&
      obj == lv_tabview_get_tab_bar(objects.tabview2_1))
    return false;  // خود نوار مقصد نیست؛ فقط پنج کلید داخل آن مقصدند.
  if (lv_obj_check_type(obj, &lv_label_class)) {
    if (GUI.scr_id == 2 && !f3_pinmap_dialog_is_open()) return false;
    if (lv_obj_get_event_count(obj) == 0) return false;
  }
  const bool readable_scroll = panel_five_nav_text_scroll(obj);
  const bool adjustable =
      lv_obj_check_type(obj, &lv_slider_class) ||
      lv_obj_check_type(obj, &lv_arc_class) ||
      lv_obj_check_type(obj, &lv_roller_class) ||
      lv_obj_check_type(obj, &lv_dropdown_class) ||
      lv_obj_check_type(obj, &lv_switch_class) ||
      lv_obj_check_type(obj, &lv_textarea_class) ||
      lv_obj_check_type(obj, &lv_keyboard_class) ||
      lv_obj_check_type(obj, &lv_buttonmatrix_class);
  if (readable_scroll || adjustable) return true;
  if (is_toast_object(obj)) return false;
  if (lv_obj_check_type(obj, &lv_button_class)) return lv_obj_get_event_count(obj) > 0;
  if (!lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) return false;

  // catcher و پس‌زمینهٔ تمام‌صفحه مقصد عملیاتی نیستند؛ کنترل‌های داخلشان جدا جمع می‌شوند.
  if (lv_obj_get_class(obj) == &lv_obj_class) {
    if (lv_obj_get_event_count(obj) == 0) return false;
    const int w = lv_obj_get_width(obj);
    const int h = lv_obj_get_height(obj);
    if (w >= SCREEN_WIDTH - 8 && h >= SCREEN_HEIGHT / 2) return false;
    if (w > 240 && h > 160) return false;
    if (w < 32 || h < 28) return false;
  }
  return true;
}

static bool panel_five_nav_is_keypad(lv_obj_t *obj) {
  return obj && (lv_obj_check_type(obj, &lv_keyboard_class) ||
                 lv_obj_check_type(obj, &lv_buttonmatrix_class));
}

static void panel_five_nav_push(lv_obj_t **items, int *count, int max_count, lv_obj_t *obj) {
  if (!obj || *count >= max_count) return;
  for (int i = 0; i < *count; ++i) if (items[i] == obj) return;
  items[(*count)++] = obj;
}

static void panel_five_nav_collect(lv_obj_t *root, lv_obj_t **items, int *count,
                                   int max_count, bool keypad_only = false) {
  const bool allow_offscreen = f3_pinmap_dialog_is_open() ||
      (GUI.scr_id == 2 && panel_five_nav_settings_page_engaged);
  if (!root ||
      (allow_offscreen ? !panel_five_nav_available(root) : !panel_five_nav_visible(root)) ||
      *count >= max_count)
    return;
  // در سطح کلی لیست، هر ردیف یک مقصد است؛ طول و تعداد فقط بعد از تأیید ردیف وارد چرخه می‌شوند.
  if (!keypad_only && GUI.scr_id == 3 && panel_five_nav_list_row < 0 &&
      panel_five_nav_list_row_of(root) >= 0)
    return;
  const uint32_t n = lv_obj_get_child_count(root);
  for (uint32_t i = 0; i < n && *count < max_count; ++i) {
    lv_obj_t *child = lv_obj_get_child(root, i);
    if (GUI.scr_id == 2 && panel_five_nav_settings_page_engaged &&
        panel_five_nav_is_f_tab(child))
      continue;
    if (panel_five_nav_candidate(child) && (!keypad_only || panel_five_nav_is_keypad(child)))
      panel_five_nav_push(items, count, max_count, child);
    panel_five_nav_collect(child, items, count, max_count, keypad_only);
  }
}

static bool panel_five_nav_is_f_tab(lv_obj_t *obj) {
  if (!obj || GUI.scr_id != 2 || !objects.tabview2_1) return false;
  lv_obj_t *bar = lv_tabview_get_tab_bar(objects.tabview2_1);
  return bar && lv_obj_get_parent(obj) == bar;
}

static int panel_five_nav_list_row_of(lv_obj_t *obj) {
  lv_obj_t *rows[10] = {
    objects.btn3_1_1, objects.btn3_1_2, objects.btn3_1_3, objects.btn3_1_4, objects.btn3_1_5,
    objects.btn3_1_6, objects.btn3_1_7, objects.btn3_1_8, objects.btn3_1_9, objects.btn3_1_10
  };
  for (int i = 0; i < 10; ++i) if (obj == rows[i]) return i;
  return -1;
}

static int panel_five_nav_list_field_kind(lv_obj_t *obj) {
  lv_obj_t *lengths[10] = {
    objects.lable3_1_1, objects.lable3_1_2, objects.lable3_1_3, objects.lable3_1_4, objects.lable3_1_5,
    objects.lable3_1_6, objects.lable3_1_7, objects.lable3_1_8, objects.lable3_1_9, objects.lable3_1_10
  };
  lv_obj_t *counts[10] = {
    objects.lable3_2_1, objects.lable3_2_2, objects.lable3_2_3, objects.lable3_2_4, objects.lable3_2_5,
    objects.lable3_2_6, objects.lable3_2_7, objects.lable3_2_8, objects.lable3_2_9, objects.lable3_2_10
  };
  for (int i = 0; i < 10; ++i) {
    if (obj == lengths[i]) return 1;
    if (obj == counts[i]) return 2;
  }
  return 0;
}

static void panel_five_nav_list_row_items(int row, lv_obj_t **items, int *count, int max_count) {
  lv_obj_t *lengths[10] = {
    objects.lable3_1_1, objects.lable3_1_2, objects.lable3_1_3, objects.lable3_1_4, objects.lable3_1_5,
    objects.lable3_1_6, objects.lable3_1_7, objects.lable3_1_8, objects.lable3_1_9, objects.lable3_1_10
  };
  lv_obj_t *counts[10] = {
    objects.lable3_2_1, objects.lable3_2_2, objects.lable3_2_3, objects.lable3_2_4, objects.lable3_2_5,
    objects.lable3_2_6, objects.lable3_2_7, objects.lable3_2_8, objects.lable3_2_9, objects.lable3_2_10
  };
  lv_obj_t *rows[10] = {
    objects.btn3_1_1, objects.btn3_1_2, objects.btn3_1_3, objects.btn3_1_4, objects.btn3_1_5,
    objects.btn3_1_6, objects.btn3_1_7, objects.btn3_1_8, objects.btn3_1_9, objects.btn3_1_10
  };
  if (row < 0 || row >= 10) return;
  panel_five_nav_push(items, count, max_count, lengths[row]);
  panel_five_nav_push(items, count, max_count, counts[row]);
  panel_five_nav_push(items, count, max_count, rows[row]);
}

static void panel_five_nav_marker_hide() {
  if (panel_five_nav_marker && lv_obj_is_valid(panel_five_nav_marker))
    lv_obj_add_flag(panel_five_nav_marker, LV_OBJ_FLAG_HIDDEN);
}

static void panel_five_nav_mark(lv_obj_t *obj) {
  if (panel_five_nav_focus && lv_obj_is_valid(panel_five_nav_focus)) {
    lv_obj_clear_state(panel_five_nav_focus, LV_STATE_FOCUSED);
    if (panel_five_nav_is_keypad(panel_five_nav_focus)) {
      lv_buttonmatrix_set_selected_button(panel_five_nav_focus, LV_BUTTONMATRIX_BUTTON_NONE);
    }
  }
  panel_five_nav_focus = obj;
  if (!obj || !lv_obj_is_valid(obj)) {
    panel_five_nav_marker_hide();
    return;
  }
  lv_obj_scroll_to_view_recursive(obj, LV_ANIM_OFF);
  lv_obj_update_layout(obj);
  if (panel_five_nav_is_keypad(obj) && panel_five_nav_keypad_engaged) {
    panel_five_nav_marker_hide();
    return;
  }

  if (!panel_five_nav_marker || !lv_obj_is_valid(panel_five_nav_marker)) {
    panel_five_nav_marker = lv_obj_create(lv_layer_top());
    lv_obj_clear_flag(panel_five_nav_marker,
                      (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_set_style_bg_opa(panel_five_nav_marker, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel_five_nav_marker, lv_color_hex(0x0ea5e9),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_five_nav_marker, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_five_nav_marker, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_five_nav_marker, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_area_t a;
  lv_obj_get_coords(obj, &a);
  const int x1 = max(1, (int)a.x1 - 3);
  const int y1 = max(1, (int)a.y1 - 3);
  const int x2 = min(SCREEN_WIDTH - 2, (int)a.x2 + 3);
  const int y2 = min(SCREEN_HEIGHT - 2, (int)a.y2 + 3);
  lv_obj_set_pos(panel_five_nav_marker, x1, y1);
  lv_obj_set_size(panel_five_nav_marker, max(2, x2 - x1 + 1), max(2, y2 - y1 + 1));
  lv_obj_clear_flag(panel_five_nav_marker, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(panel_five_nav_marker);
  lv_obj_add_state(obj, LV_STATE_FOCUSED);
}

static void panel_five_nav_exit() {
  if (panel_five_nav_scroll && lv_obj_is_valid(panel_five_nav_scroll))
    lv_obj_scroll_to_y(panel_five_nav_scroll, 0, LV_ANIM_OFF);
  panel_five_nav_active = false;
  panel_five_nav_keypad_engaged = false;
  panel_five_nav_settings_page_engaged = false;
  panel_five_nav_list_row = -1;
  panel_five_nav_scroll = NULL;
  panel_five_nav_mark(NULL);
}

static void panel_five_nav_note_activity() {
  panel_five_nav_last_action_ms = millis();
}

static void panel_five_nav_tick() {
  if (panel_five_nav_active &&
      (uint32_t)(millis() - panel_five_nav_last_action_ms) >= 10000U)
    panel_five_nav_exit();
}

static int panel_five_nav_build(lv_obj_t **items, int max_count) {
  int count = 0;
  if (GUI.scr_id == 3 && panel_five_nav_list_row >= 0) {
    panel_five_nav_list_row_items(panel_five_nav_list_row, items, &count, max_count);
    return count;
  }
  lv_obj_t *top_items[128];
  int top_count = 0;
  panel_five_nav_collect(lv_layer_top(), top_items, &top_count, 128);
  if (top_count > 0) {
    // مودال، ویزارد یا دیالوگ روی لایهٔ top باز است: فقط کنترل‌های همان لایه پیمایش می‌شوند (قرنطینه کامل پس‌زمینه)
    for (int i = 0; i < top_count; ++i) panel_five_nav_push(items, &count, max_count, top_items[i]);
  } else {
    if (GUI.scr_id == 2 && objects.tabview2_1 && !panel_five_nav_settings_page_engaged) {
      lv_obj_t *bar = lv_tabview_get_tab_bar(objects.tabview2_1);
      const uint32_t tabs = bar ? lv_obj_get_child_count(bar) : 0;
      for (uint32_t i = 0; i < tabs; ++i)
        panel_five_nav_push(items, &count, max_count, lv_obj_get_child(bar, i));
      panel_five_nav_push(items, &count, max_count, panel_five_nav_active_settings_page());
    } else if (GUI.scr_id == 2 && objects.tabview2_1 &&
               panel_five_nav_settings_page_engaged) {
      // داخل صفحه فقط کنترل‌های تب فعال مقصدند؛ خود tabview دوباره وارد چرخه نشود.
      panel_five_nav_collect(panel_five_nav_active_settings_page(),
                             items, &count, max_count);
    } else {
      panel_five_nav_collect(lv_screen_active(), items, &count, max_count);
    }
  }

  // The F3 pin picker is a scroll modal. Preserve its natural construction
  // order; geometric sort mixes off-screen choices with footer buttons.
  if (!f3_pinmap_dialog_is_open()) {
    for (int i = 0; i < count - 1; ++i) {
      for (int j = i + 1; j < count; ++j) {
      lv_area_t a, b;
      lv_obj_get_coords(items[i], &a);
      lv_obj_get_coords(items[j], &b);
      const bool a_keypad = panel_five_nav_is_keypad(items[i]);
      const bool b_keypad = panel_five_nav_is_keypad(items[j]);
      const bool a_tab = panel_five_nav_is_f_tab(items[i]);
      const bool b_tab = panel_five_nav_is_f_tab(items[j]);
      const bool a_scroll = panel_five_nav_text_scroll(items[i]);
      const bool b_scroll = panel_five_nav_text_scroll(items[j]);
      const int a_priority = a_tab ? 0 : (a_scroll ? 1 : (a_keypad ? 3 : 2));
      const int b_priority = b_tab ? 0 : (b_scroll ? 1 : (b_keypad ? 3 : 2));
      if (b_priority < a_priority ||
          (b_priority == a_priority && (b.y1 < a.y1 || (b.y1 == a.y1 && b.x1 < a.x1)))) {
        lv_obj_t *t = items[i]; items[i] = items[j]; items[j] = t;
      }
      }
    }
  }
  return count;
}

static bool panel_five_nav_modal_available() {
  lv_obj_t *items[16];
  int count = 0;
  panel_five_nav_collect(lv_layer_top(), items, &count, 16);
  return count > 0;
}

static bool panel_five_nav_digit_text(const char *txt) {
  return txt && txt[0] >= '0' && txt[0] <= '9' && txt[1] == '\0';
}

static int panel_five_nav_keypad_digit_count(lv_obj_t *keypad, int *first_digit) {
  const char * const *map = lv_buttonmatrix_get_map(keypad);
  int button_id = 0, count = 0;
  if (first_digit) *first_digit = -1;
  if (!map) return 0;
  for (int i = 0; map[i] && map[i][0] != '\0'; ++i) {
    if (strcmp(map[i], "\n") == 0) continue;
    if (panel_five_nav_digit_text(map[i]) &&
        !lv_buttonmatrix_has_button_ctrl(keypad, button_id, LV_BUTTONMATRIX_CTRL_HIDDEN)) {
      if (first_digit && *first_digit < 0) *first_digit = button_id;
      count++;
    }
    button_id++;
  }
  return count;
}

static void panel_five_nav_keypad_next(lv_obj_t *keypad, bool initialize) {
  if (!panel_five_nav_is_keypad(keypad)) return;
  lv_obj_add_state(keypad, LV_STATE_FOCUSED);
  const uint32_t selected = lv_buttonmatrix_get_selected_button(keypad);
  if (initialize || selected == LV_BUTTONMATRIX_BUTTON_NONE) {
    lv_buttonmatrix_set_selected_button(keypad, 0);
    return;
  }
  uint32_t key = LV_KEY_RIGHT;
  lv_obj_send_event(keypad, LV_EVENT_KEY, &key);
}

static void panel_five_nav_keypad_prev(lv_obj_t *keypad) {
  if (!panel_five_nav_is_keypad(keypad)) return;
  lv_obj_add_state(keypad, LV_STATE_FOCUSED);
  uint32_t key = LV_KEY_LEFT;
  lv_obj_send_event(keypad, LV_EVENT_KEY, &key);
}

static void panel_five_nav_mode_next() {
  panel_five_nav_note_activity();
  if (panel_five_nav_active && panel_five_nav_focus && lv_obj_is_valid(panel_five_nav_focus) &&
      panel_five_nav_is_keypad(panel_five_nav_focus) && panel_five_nav_keypad_engaged) {
    panel_five_nav_keypad_next(panel_five_nav_focus, false);
    return;
  }
  if (panel_five_nav_active && panel_five_nav_focus && lv_obj_is_valid(panel_five_nav_focus) &&
      panel_five_nav_text_scroll(panel_five_nav_focus)) {
    const int bottom = lv_obj_get_scroll_bottom(panel_five_nav_focus);
    if (bottom > 0) {
      panel_five_nav_scroll = panel_five_nav_focus;
      lv_obj_scroll_by(panel_five_nav_focus, 0, -min(bottom, 44), LV_ANIM_OFF);
      panel_five_nav_mark(panel_five_nav_focus);
      return;
    }
  }
  lv_obj_t *items[128];
  const int count = panel_five_nav_build(items, 128);
  if (count <= 0) { panel_five_nav_exit(); return; }
  int index = -1;
  for (int i = 0; i < count; ++i) if (items[i] == panel_five_nav_focus) { index = i; break; }
  if (panel_five_nav_active && index >= count - 1) { panel_five_nav_exit(); return; }
  panel_five_nav_active = true;
  panel_five_nav_mark(items[index + 1]);
}

static void panel_five_nav_next() {
  panel_five_nav_mode_next();
}

static void panel_five_nav_prev() {
  panel_five_nav_note_activity();
  lv_obj_t *items[128];
  const int count = panel_five_nav_build(items, 128);
  if (count <= 0) { panel_five_nav_exit(); return; }
  int index = -1;
  for (int i = 0; i < count; ++i) if (items[i] == panel_five_nav_focus) { index = i; break; }
  if (index <= 0) {
    if (panel_five_nav_active) panel_five_nav_exit();
    return;
  }
  panel_five_nav_active = true;
  panel_five_nav_mark(items[index - 1]);
}

static bool panel_five_nav_activate(lv_obj_t *obj) {
  if (!obj || !lv_obj_is_valid(obj)) return false;
  if (lv_obj_send_event(obj, LV_EVENT_PRESSED, NULL) != LV_RESULT_OK || !lv_obj_is_valid(obj))
    return false;
  if (lv_obj_send_event(obj, LV_EVENT_RELEASED, NULL) != LV_RESULT_OK || !lv_obj_is_valid(obj))
    return false;
  return lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL) == LV_RESULT_OK && lv_obj_is_valid(obj);
}

static const char *panel_five_nav_first_text(lv_obj_t *obj) {
  if (!obj || !lv_obj_is_valid(obj)) return NULL;
  if (lv_obj_check_type(obj, &lv_label_class)) {
    const char *txt = lv_label_get_text(obj);
    if (txt && txt[0]) return txt;
  }
  const uint32_t n = lv_obj_get_child_count(obj);
  for (uint32_t i = 0; i < n; ++i) {
    const char *txt = panel_five_nav_first_text(lv_obj_get_child(obj, i));
    if (txt && txt[0]) return txt;
  }
  return NULL;
}

static void panel_five_nav_describe() {
  lv_obj_t *obj = panel_five_nav_focus;
  if (!panel_five_nav_active || !panel_five_nav_visible(obj)) return;
  panel_five_nav_note_activity();
  if (f3_try_pin_help(obj)) return;
  if (obj == objects.btn1_2) {
    ui_toast("تنظیمات: سرعت، موتور، پایه ها و امکانات دستگاه",
             "Settings: speed, motor, pins and device options", 3600);
    return;
  }
  if (obj == objects.btn_save) {
    ui_toast("ذخیره: تغییرات سبز را در حافظه ثبت می کند",
             "Save: stores the green pending changes", 3400);
    return;
  }
  if (GUI.scr_id == 2 && obj == panel_five_nav_active_settings_page()) {
    ui_toast("محتوای صفحه: استارت بزنید تا کنترل های این بخش انتخاب شوند",
             "Page content: press Start to enter this section", 3600);
    return;
  }
  if (panel_five_nav_is_f_tab(obj)) {
    ui_toast("کلیدهای اف: جابه جایی بین پنج بخش تنظیمات",
             "F keys: move between the five settings sections", 3200);
    return;
  }
  if (panel_five_nav_is_keypad(obj)) {
    ui_toast("کیبورد عددی: استارت ورود، مد حرکت، کنسل بازگشت",
             "Keypad: Start enters, Mode moves, Cancel returns", 3600);
    return;
  }
  const int row = panel_five_nav_list_row_of(obj);
  if (row >= 0) {
    ui_toast("ردیف برش: استارت برای انتخاب متراژ، تعداد یا خود ردیف",
             "Cut row: Start opens length, quantity and row selection", 3800);
    return;
  }
  const int field_kind = panel_five_nav_list_field_kind(obj);
  if (field_kind == 1) {
    ui_toast("متراژ ردیف: استارت برای باز کردن و ویرایش اندازه",
             "Row length: press Start to edit the size", 3400);
    return;
  }
  if (field_kind == 2) {
    ui_toast("تعداد ردیف: استارت برای باز کردن و ویرایش تعداد",
             "Row quantity: press Start to edit the quantity", 3400);
    return;
  }
  const char *txt = panel_five_nav_first_text(obj);
  char fa[150] = "این قسمت یک کنترل قابل انتخاب است";
  char en[150] = "This is a selectable control";
  if (txt && txt[0]) {
    const uint8_t lead = (uint8_t)txt[0];
    if (lead == 0xd8 || lead == 0xd9)
      snprintf(fa, sizeof(fa), "این قسمت برای %s است", txt);
    else if (lead >= 0x20 && lead < 0x7f)
      snprintf(en, sizeof(en), "This control is for %s", txt);
  }
  ui_toast(fa, en, 3000);
}

static void panel_five_nav_confirm() {
  lv_obj_t *obj = panel_five_nav_focus;
  if (!panel_five_nav_active || !panel_five_nav_visible(obj)) {
    panel_five_nav_exit();
    return;
  }
  panel_five_nav_note_activity();
  if (panel_five_nav_is_keypad(obj)) {
    if (!panel_five_nav_keypad_engaged) {
      panel_five_nav_keypad_engaged = true;
      panel_five_nav_keypad_next(obj, true);
      panel_five_nav_mark(obj);
    } else {
      lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
    return;
  }
  if (GUI.scr_id == 2 && !panel_five_nav_settings_page_engaged &&
      obj == panel_five_nav_active_settings_page()) {
    panel_five_nav_settings_page_engaged = true;
    lv_obj_t *items[128];
    const int count = panel_five_nav_build(items, 128);
    panel_five_nav_mark(count > 0 ? items[0] : obj);
    return;
  }
  const int row = panel_five_nav_list_row_of(obj);
  if (GUI.scr_id == 3 && panel_five_nav_list_row < 0 && row >= 0) {
    panel_five_nav_list_row = row;
    lv_obj_t *items[3];
    int count = 0;
    panel_five_nav_list_row_items(row, items, &count, 3);
    panel_five_nav_mark(count > 0 ? items[0] : obj);
    return;
  }
  panel_five_nav_activate(obj);
}

static bool panel_five_nav_cancel_level() {
  if (!panel_five_nav_active) return false;
  panel_five_nav_note_activity();
  if (panel_five_nav_keypad_engaged && panel_five_nav_is_keypad(panel_five_nav_focus)) {
    panel_five_nav_keypad_engaged = false;
    lv_buttonmatrix_set_selected_button(panel_five_nav_focus, LV_BUTTONMATRIX_BUTTON_NONE);
    panel_five_nav_mark(panel_five_nav_focus);
    return true;
  }
  if (panel_five_nav_modal_available()) {
    panel_five_nav_exit();
    return true;
  }
  if (GUI.scr_id == 3 && panel_five_nav_list_row >= 0) {
    lv_obj_t *rows[10] = {
      objects.btn3_1_1, objects.btn3_1_2, objects.btn3_1_3, objects.btn3_1_4, objects.btn3_1_5,
      objects.btn3_1_6, objects.btn3_1_7, objects.btn3_1_8, objects.btn3_1_9, objects.btn3_1_10
    };
    const int row = panel_five_nav_list_row;
    panel_five_nav_list_row = -1;
    panel_five_nav_mark(rows[row]);
    return true;
  }
  if (GUI.scr_id == 2 && panel_five_nav_settings_page_engaged) {
    panel_five_nav_settings_page_engaged = false;
    panel_five_nav_mark(panel_five_nav_active_settings_page());
    return true;
  }
  panel_five_nav_exit();
  return true;
}

static bool panel_five_nav_keyboard_step(int dir) {
  panel_five_nav_note_activity();
  if (panel_five_nav_active && panel_five_nav_focus &&
      lv_obj_is_valid(panel_five_nav_focus) &&
      panel_five_nav_is_keypad(panel_five_nav_focus) &&
      panel_five_nav_keypad_engaged) {
    uint32_t key = (dir >= 0) ? LV_KEY_RIGHT : LV_KEY_LEFT;
    lv_obj_send_event(panel_five_nav_focus, LV_EVENT_KEY, &key);
    return true;
  }

  lv_obj_t *items[128];
  const int count = panel_five_nav_build(items, 128);
  if (count <= 0) return false;
  int index = -1;
  for (int i = 0; i < count; ++i) {
    if (items[i] == panel_five_nav_focus) {
      index = i;
      break;
    }
  }
  if (!panel_five_nav_active || index < 0)
    index = (dir >= 0) ? 0 : count - 1;
  else
    index = (index + (dir >= 0 ? 1 : -1) + count) % count;
  panel_five_nav_active = true;
  panel_five_nav_mark(items[index]);
  return true;
}

static bool panel_five_nav_keyboard_confirm() {
  if (!panel_five_nav_active) return false;
  panel_five_nav_confirm();
  return true;
}

static bool panel_five_nav_keyboard_cancel() {
  if (f3_pinmap_dialog_is_open()) {
    const bool closed = f3_pinmap_panel_cancel();
    panel_five_nav_exit();
    return closed;
  }
  return panel_five_nav_cancel_level();
}

static bool panel_five_nav_keypad_delete(lv_obj_t *obj) {
  if (!obj || !lv_obj_is_valid(obj)) return false;
  if (lv_obj_check_type(obj, &lv_keyboard_class)) {
    lv_obj_t *ta = lv_keyboard_get_textarea(obj);
    if (ta) {
      lv_textarea_delete_char(ta);
      return true;
    }
  }
  const char * const *map = lv_buttonmatrix_get_map(obj);
  const uint32_t old_selected = lv_buttonmatrix_get_selected_button(obj);
  int button_id = 0;
  for (int i = 0; map && map[i] && map[i][0] != '\0'; ++i) {
    if (strcmp(map[i], "\n") == 0) continue;
    if (strcmp(map[i], LV_SYMBOL_BACKSPACE) == 0) {
      lv_buttonmatrix_set_selected_button(obj, button_id);
      lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
      if (lv_obj_is_valid(obj)) lv_buttonmatrix_set_selected_button(obj, old_selected);
      return true;
    }
    button_id++;
  }
  return false;
}

static void panel_five_nav_adjust(int dir) {
  lv_obj_t *obj = panel_five_nav_focus;
  if (!panel_five_nav_active || !panel_five_nav_visible(obj)) { panel_five_nav_exit(); return; }
  panel_five_nav_note_activity();
  if (f3_pinmap_panel_adjust(obj, dir)) return;
  if (panel_five_nav_is_keypad(obj)) {
    if (dir > 0) panel_five_nav_confirm();
    else panel_five_nav_keypad_delete(obj);
    return;
  }
  if (lv_obj_check_type(obj, &lv_slider_class)) {
    const int v = constrain((int)lv_slider_get_value(obj) + dir,
                            (int)lv_slider_get_min_value(obj), (int)lv_slider_get_max_value(obj));
    lv_slider_set_value(obj, v, LV_ANIM_OFF);
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    return;
  }
  if (lv_obj_check_type(obj, &lv_arc_class)) {
    const int v = constrain((int)lv_arc_get_value(obj) + dir,
                            (int)lv_arc_get_min_value(obj), (int)lv_arc_get_max_value(obj));
    lv_arc_set_value(obj, v);
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    return;
  }
  if (lv_obj_check_type(obj, &lv_roller_class)) {
    int v = (int)lv_roller_get_selected(obj) + dir;
    const int n = (int)lv_roller_get_option_count(obj);
    lv_roller_set_selected(obj, constrain(v, 0, n > 0 ? n - 1 : 0), LV_ANIM_OFF);
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    return;
  }
  if (lv_obj_check_type(obj, &lv_dropdown_class)) {
    int v = (int)lv_dropdown_get_selected(obj) + dir;
    const int n = (int)lv_dropdown_get_option_count(obj);
    lv_dropdown_set_selected(obj, constrain(v, 0, n > 0 ? n - 1 : 0));
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    return;
  }
  if (f4_panel_adjust_numeric(obj, dir)) return;
  if (lv_obj_check_type(obj, &lv_textarea_class)) {
    if (dir < 0) lv_textarea_delete_char(obj);
    else panel_five_nav_activate(obj);
    return;
  }
  if (lv_obj_check_type(obj, &lv_switch_class) ||
      lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE)) {
    if (dir > 0) panel_five_nav_activate(obj);
    return;
  }
  if (dir > 0) panel_five_nav_confirm();
}

static bool panel_five_nav_is_adjustable(lv_obj_t *obj) {
  if (!obj || !lv_obj_is_valid(obj)) return false;
  if (lv_obj_check_type(obj, &lv_slider_class) ||
      lv_obj_check_type(obj, &lv_arc_class) ||
      lv_obj_check_type(obj, &lv_roller_class) ||
      lv_obj_check_type(obj, &lv_dropdown_class))
    return true;
  return false;
}

static void panel_five_nav_start_action() {
  lv_obj_t *obj = panel_five_nav_focus;
  if (!panel_five_nav_active || !panel_five_nav_visible(obj)) {
    panel_five_nav_exit();
    return;
  }
  panel_five_nav_note_activity();

  // 1. کیبورد یا نامپد مجازی درگیر
  if (panel_five_nav_is_keypad(obj)) {
    panel_five_nav_confirm();
    return;
  }

  // 2. ویجت‌های پیوسته و مقادیر عددی قابل تنظیم (+1)
  if (panel_five_nav_is_adjustable(obj)) {
    panel_five_nav_adjust(+1);
    return;
  }
  if (f4_panel_adjust_numeric(obj, +1)) return;

  // 3. کلید دووضعیتی یا چک‌باکس
  if (lv_obj_check_type(obj, &lv_switch_class) ||
      lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE)) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      lv_obj_clear_state(obj, LV_STATE_CHECKED);
    } else {
      lv_obj_add_state(obj, LV_STATE_CHECKED);
    }
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    return;
  }

  // 4. دکمه‌های فشاری، باز کردن زیربخش‌ها و تأیید پیش‌فرض
  panel_five_nav_confirm();
}

static void panel_five_nav_stop_action() {
  lv_obj_t *obj = panel_five_nav_focus;
  if (!panel_five_nav_active || !panel_five_nav_visible(obj)) {
    panel_five_nav_exit();
    return;
  }
  panel_five_nav_note_activity();

  // 1. دیالوگ پین‌مپ در صورت فعال بودن
  if (f3_pinmap_panel_adjust(obj, -1)) return;

  // 2. کیبورد یا نامپد مجازی: بک‌اسپیس و پاک کردن کاراکتر
  if (panel_five_nav_is_keypad(obj)) {
    panel_five_nav_keypad_delete(obj);
    return;
  }

  // 3. ویجت‌های پیوسته و مقادیر عددی قابل تنظیم (-1)
  if (panel_five_nav_is_adjustable(obj)) {
    panel_five_nav_adjust(-1);
    return;
  }
  if (f4_panel_adjust_numeric(obj, -1)) return;

  // 4. کادر متنی: حذف کاراکتر
  if (lv_obj_check_type(obj, &lv_textarea_class)) {
    lv_textarea_delete_char(obj);
    return;
  }

  // 5. زیربخش‌های تودرتو: بازگشت به سطح بالاتر
  if ((GUI.scr_id == 3 && panel_five_nav_list_row >= 0) ||
      (GUI.scr_id == 2 && panel_five_nav_settings_page_engaged)) {
    panel_five_nav_cancel_level();
    return;
  }

  // 6. دکمه‌های فشاری: عدم انجام عمل مثبت (اصل ایمنی زیمنس)
}

static inline int clamp_screen_x(int x) {
  if (x < 0) return 0;
  if (x >= SCREEN_WIDTH) return SCREEN_WIDTH - 1;
  return x;
}

static inline int clamp_screen_y(int y) {
  if (y < 0) return 0;
  if (y >= SCREEN_HEIGHT) return SCREEN_HEIGHT - 1;
  return y;
}

void usb_mouse_move_pointer(int dx, int dy) {
  usb_mouse_x = clamp_screen_x((int)usb_mouse_x + dx);
  usb_mouse_y = clamp_screen_y((int)usb_mouse_y + dy);
  usb_mouse_seen = true;
}

void usb_mouse_set_left_button(bool pressed) {
  usb_mouse_left_down = pressed;
  usb_mouse_seen = true;
}

void usb_mouse_report_buttons(bool left_down, bool right_clicked, bool back_clicked) {
  usb_mouse_left_down = left_down;
  if (right_clicked) usb_mouse_right_click_pending = true;
  if (back_clicked) usb_mouse_back_pending = true;
  usb_mouse_seen = true;
}

void usb_mouse_add_wheel(int delta) {
  if (delta == 0) return;
  int v = usb_mouse_wheel_delta + delta;
  if (v > 8) v = 8;
  if (v < -8) v = -8;
  usb_mouse_wheel_delta = v;
  usb_mouse_seen = true;
}

int usb_mouse_take_wheel() {
  int v = usb_mouse_wheel_delta;
  usb_mouse_wheel_delta = 0;
  return v;
}

bool usb_mouse_take_right_click() {
  bool v = usb_mouse_right_click_pending;
  usb_mouse_right_click_pending = false;
  return v;
}

bool usb_mouse_take_back_click() {
  bool v = usb_mouse_back_pending;
  usb_mouse_back_pending = false;
  return v;
}

void usb_mouse_get_point(lv_point_t *p) {
  if (!p) return;
  p->x = usb_mouse_x;
  p->y = usb_mouse_y;
}

void usb_mouse_reset_state() {
  usb_mouse_left_down = false;
  usb_mouse_seen = false;
  usb_mouse_wheel_delta = 0;
  usb_mouse_right_click_pending = false;
  usb_mouse_back_pending = false;
}

void keyboard_nav_queue_key(uint32_t key) {
  if (keyboard_nav_key_pending) return;
  keyboard_nav_key = key;
  keyboard_nav_key_pending = true;
  keyboard_nav_key_down = true;
}

void keyboard_nav_page_back() {
  if (GUI.scr_id == 2 || GUI.scr_id == 3 || GUI.scr_id == 0) {
    numpad_close();
    cancel_screen3_pending_edit();
    GUI.pending_goto_scr = 1;
  }
}

static bool io_test_snapshot_valid = false;
static bool io_test_output_state[6] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };

static void capture_io_test_outputs() {
  io_test_output_state[0] = pinmap_out_read(IOS.spray_pin);
  io_test_output_state[1] = pinmap_out_read(IOS.u_clip_pin);
  io_test_output_state[2] = pinmap_out_read(IOS.saw_pin);
  io_test_output_state[3] = pinmap_out_read(IOS.f_clip_pin);
  io_test_output_state[4] = pinmap_out_read(IOS.sawl_ift_pin);
  io_test_output_state[5] = pinmap_out_read(IOS.lamp_pin);
  io_test_snapshot_valid = true;
}

static void restore_io_test_outputs() {
  if (!io_test_snapshot_valid) return;
  pinmap_out_write(IOS.spray_pin, io_test_output_state[0]);
  pinmap_out_write(IOS.u_clip_pin, io_test_output_state[1]);
  pinmap_out_write(IOS.saw_pin, io_test_output_state[2]);
  pinmap_out_write(IOS.f_clip_pin, io_test_output_state[3]);
  pinmap_out_write(IOS.sawl_ift_pin, io_test_output_state[4]);
  pinmap_out_write(IOS.lamp_pin, io_test_output_state[5]);
  io_test_snapshot_valid = false;
}
static void turn_off_test_outputs() {
  // turn all test outputs OFF and uncheck buttons visually
  pinmap_out_write(IOS.spray_pin,    HIGH);
  pinmap_out_write(IOS.u_clip_pin,   HIGH);
  pinmap_out_write(IOS.saw_pin,      HIGH);
  pinmap_out_write(IOS.f_clip_pin,   HIGH);
  pinmap_out_write(IOS.sawl_ift_pin, HIGH);
  pinmap_out_write(IOS.lamp_pin,     HIGH);
  lv_obj_clear_state(objects.btn2_3_1, LV_STATE_CHECKED);
  lv_obj_clear_state(objects.btn2_3_2, LV_STATE_CHECKED);
  lv_obj_clear_state(objects.btn2_3_3, LV_STATE_CHECKED);
  lv_obj_clear_state(objects.btn2_3_4, LV_STATE_CHECKED);
  lv_obj_clear_state(objects.btn2_3_5, LV_STATE_CHECKED);
  lv_obj_clear_state(objects.btn2_3_6, LV_STATE_CHECKED);
}
//-----------------------homing error UI (main_0)--------------------------
// کارت سفید وسط؛ لمس → مستطیل سبز. پروفایل: docs/OWNER_PROFILE_FA.md
static lv_obj_t *ref_green_hint = NULL;
static lv_obj_t *ref_error_confirm_btn = NULL;
static lv_obj_t *ref_error_confirm_label = NULL;

static void ref_homing_apply_green_caption() {
  const bool en = (GUI.system_language == 2);
  // عنوان بزرگ‌تر؛ راهنما پایین با فاصلهٔ واضح
  if (objects.lablef0_1) {
    lv_obj_set_size(objects.lablef0_1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_long_mode(objects.lablef0_1, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(objects.lablef0_1, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.lablef0_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(objects.lablef0_1, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.lablef0_1, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablef0_1, "موقعیت یابی");
    lv_obj_align(objects.lablef0_1, LV_ALIGN_CENTER, 0, 0);
    if (en) lv_obj_add_flag(objects.lablef0_1, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(objects.lablef0_1, LV_OBJ_FLAG_HIDDEN);
  }
  if (objects.lablee0_1) {
    lv_obj_set_size(objects.lablee0_1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_long_mode(objects.lablee0_1, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(objects.lablee0_1, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(objects.lablee0_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(objects.lablee0_1, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.lablee0_1, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablee0_1, "Reference\n     Zero");
    lv_obj_align(objects.lablee0_1, LV_ALIGN_CENTER, 0, 0);
    if (en) lv_obj_clear_flag(objects.lablee0_1, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(objects.lablee0_1, LV_OBJ_FLAG_HIDDEN);
  }
  if (ref_green_hint && lv_obj_is_valid(ref_green_hint))
    lv_obj_add_flag(ref_green_hint, LV_OBJ_FLAG_HIDDEN);
}

static void ref_homing_restore_ready_ui() {
  ref_error_ui_active = false;
  ref_failed_flag = false;
  ref_wrong_dir_flag = false;
  ref_wrong_dir_sensor = 0;
  ref_fail_reason = 0;
  if (objects.spinner0_1) lv_obj_add_flag(objects.spinner0_1, LV_OBJ_FLAG_HIDDEN);
  // پنجرهٔ لغو روی لایهٔ بالا ساخته می‌شود؛ قبل از آشکارشدن ضربدر حتماً پنهانش کن.
  if (objects.panel0_1) lv_obj_add_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
  if (ref_error_confirm_btn && lv_obj_is_valid(ref_error_confirm_btn))
    lv_obj_add_flag(ref_error_confirm_btn, LV_OBJ_FLAG_HIDDEN);
  if (!objects.btn0_1) return;
  // کارت خطا LV_ALIGN_CENTER را روی همین شیء نگه می‌دارد؛ پیش از مختصات اصلی پاکش کن.
  lv_obj_set_style_align(objects.btn0_1, LV_ALIGN_DEFAULT,
                         LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_pos(objects.btn0_1, 84, 70);
  lv_obj_set_size(objects.btn0_1, 276, 144);
  lv_obj_set_style_bg_color(objects.btn0_1, lv_color_hex(0xff006805), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(objects.btn0_1, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(objects.btn0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(objects.btn0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(objects.btn0_1, LV_OBJ_FLAG_HIDDEN);
  ref_homing_apply_green_caption();
  if (objects.btn0_2) {
    // پنجرهٔ تایید روی لایهٔ بالا می‌رود؛ بازگشت باید ضربدر اصلی را کامل بازسازی کند.
    lv_obj_set_pos(objects.btn0_2, 295, 70);
    lv_obj_set_size(objects.btn0_2, 65, 55);
    lv_obj_set_style_bg_color(objects.btn0_2, lv_color_hex(0xffa60d0d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.btn0_2, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(objects.btn0_2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.btn0_2);
  }
}

static void ref_homing_error_confirm_event(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  lv_event_stop_bubbling(e);
  ref_homing_restore_ready_ui();
  GUI.buzzer_cmd_short = true;
}

static void ref_homing_show_error_ui(bool wrong_dir, int reason) {
  if (objects.spinner0_1) lv_obj_add_flag(objects.spinner0_1, LV_OBJ_FLAG_HIDDEN);
  if (ref_green_hint && lv_obj_is_valid(ref_green_hint))
    lv_obj_add_flag(ref_green_hint, LV_OBJ_FLAG_HIDDEN);
  if (!objects.btn0_1) return;
  const int card_w = 400;
  const int card_h = 220;
  lv_obj_set_size(objects.btn0_1, card_w, card_h);
  lv_obj_align(objects.btn0_1, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_radius(objects.btn0_1, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_hor(objects.btn0_1, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_ver(objects.btn0_1, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(objects.btn0_1, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(objects.btn0_1, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(objects.btn0_1,
                                lv_color_hex(wrong_dir ? 0xffE65100u : 0xffFF3B30u),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  const uint32_t txt_col = 0xff1C1C1Eu;
  auto prep_msg_label = [&](lv_obj_t *lab) {
    if (!lab) return;
    lv_obj_set_width(lab, card_w - 56);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    const bool english = (lab == objects.lablee0_1);
    lv_obj_set_style_text_font(lab, english ? &lv_font_montserrat_22 : &ui_font_naz24,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lab, english ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lab, lv_color_hex(txt_col), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lab, LV_ALIGN_TOP_MID, 0, 27);
  };
  prep_msg_label(objects.lablef0_1);
  prep_msg_label(objects.lablee0_1);
  if (wrong_dir) {
    if (objects.lablef0_1)
      lv_label_set_text(objects.lablef0_1,
                        "به سنسور آخر خط برخورد کرد\n"
                        "جای سنسور یا جهت حرکت اشتباه است\n"
                        "در تنظیمات جهت مبنا را عوض کنید\n"
                        "سپس دوباره موقعیت‌یابی کنید\n\n"
                        "برای بازگشت لمس کنید");
    if (objects.lablee0_1)
      lv_label_set_text(objects.lablee0_1,
                        "Hit end-of-line sensor\n"
                        "Wrong sensor place or direction\n"
                        "Fix base direction in settings\n"
                        "Then home again\n\n"
                        "Tap to go back");
  } else if (reason == 2) {
    if (objects.lablef0_1)
      lv_label_set_text(objects.lablef0_1,
                        "موقعیت‌یابی متوقف شد\n"
                        "استاپ را آزاد کنید\n"
                        "سپس دوباره بزنید\n\n"
                        "برای بازگشت لمس کنید");
    if (objects.lablee0_1)
      lv_label_set_text(objects.lablee0_1,
                        "Homing stopped\n"
                        "Release E-STOP\n"
                        "Then try again\n\n"
                        "Tap to go back");
  } else {
    if (objects.lablef0_1)
      lv_label_set_text(objects.lablef0_1,
                        "موقعیت‌یابی انجام نشد\n"
                        "سنسور اول خط یا اتصال آن را بررسی کنید\n"
                        "پس از رفع اشکال، دوباره تلاش کنید");
    if (objects.lablee0_1)
      lv_label_set_text(objects.lablee0_1,
                        "Homing was not completed\n"
                        "Check the start sensor and its connection\n"
                        "Then try again");
  }
  if (!ref_error_confirm_btn || !lv_obj_is_valid(ref_error_confirm_btn)) {
    ref_error_confirm_btn = lv_button_create(objects.btn0_1);
    lv_obj_set_size(ref_error_confirm_btn, 144, 46);
    lv_obj_set_style_radius(ref_error_confirm_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ref_error_confirm_btn, lv_color_hex(0xff087CF0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ref_error_confirm_btn, lv_color_hex(0xff0066CC), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(ref_error_confirm_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ref_error_confirm_btn, ref_homing_error_confirm_event, LV_EVENT_RELEASED, NULL);
    ref_error_confirm_label = lv_label_create(ref_error_confirm_btn);
    lv_obj_clear_flag(ref_error_confirm_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_color(ref_error_confirm_label, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (ref_error_confirm_btn && lv_obj_is_valid(ref_error_confirm_btn) &&
      ref_error_confirm_label && lv_obj_is_valid(ref_error_confirm_label)) {
    const bool en = (GUI.system_language == 2);
    lv_obj_set_style_text_font(ref_error_confirm_label, en ? &lv_font_montserrat_18 : &ui_font_naz24,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(ref_error_confirm_label, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ref_error_confirm_label, en ? "Confirm" : "تایید");
    lv_obj_align(ref_error_confirm_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(ref_error_confirm_btn, LV_ALIGN_BOTTOM_MID, 0, 13);
    lv_obj_clear_flag(ref_error_confirm_btn, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_clear_flag(objects.btn0_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn0_2) lv_obj_add_flag(objects.btn0_2, LV_OBJ_FLAG_HIDDEN);
  ref_error_ui_active = true;
}

// فلگ‌های شکست رفرنس فقط روی صفحهٔ موقعیت‌یابی (scr0) — نه صفحهٔ اصلی
static void prcs_ref_homing_result() {
  if (GUI.scr_id != 0) return;  // روی main، پیام سنسور با prcs_wrong_dir / limit_recal است
  if (ref_wrong_dir_flag) {
    ref_wrong_dir_flag = false;
    ref_failed_flag = false;
    ref_fail_reason = 0;
    ref_homing_show_error_ui(true, 0);
    return;
  }
  if (ref_failed_flag) {
    int reason = ref_fail_reason;
    ref_failed_flag = false;
    ref_fail_reason = 0;
    // استپ پنل / امرجنسی: بدون کارت سفید، همان کلید سبز
    if (reason == 2) {
      ref_homing_restore_ready_ui();
      return;
    }
    ref_homing_show_error_ui(false, reason);
  }
}

// نبود سنسور اول: جایابی خودکار ممنوع است؛ اپراتور فقط موقعیت فعلی را وارد می‌کند.
static lv_obj_t *ref_manual_pos_root = NULL;
static lv_obj_t *ref_manual_pos_value = NULL;
static char ref_manual_pos_buf[16] = "";
static const char * const ref_manual_pos_map[] = {
  "1", "2", "3", "\n",
  "4", "5", "6", "\n",
  "7", "8", "9", "\n",
  ".", "0", LV_SYMBOL_BACKSPACE, ""
};

static bool ref_manual_pos_prompt_is_open() {
  return ref_manual_pos_root && lv_obj_is_valid(ref_manual_pos_root);
}

static void ref_manual_pos_sync_value() {
  if (!ref_manual_pos_value || !lv_obj_is_valid(ref_manual_pos_value)) return;
  lv_label_set_text(ref_manual_pos_value, ref_manual_pos_buf[0] ? ref_manual_pos_buf : "0");
}

static void ref_manual_pos_close_async(void *) {
  if (ref_manual_pos_root && lv_obj_is_valid(ref_manual_pos_root))
    lv_obj_delete(ref_manual_pos_root);
  ref_manual_pos_root = NULL;
  ref_manual_pos_value = NULL;
  GUI.lock_screen = false;
}

static void ref_manual_pos_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  lv_async_call(ref_manual_pos_close_async, NULL);
}

static void ref_manual_pos_key_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  lv_obj_t *kb = (lv_obj_t *)lv_event_get_target(e);
  const uint32_t id = lv_buttonmatrix_get_selected_button(kb);
  const char *txt = lv_buttonmatrix_get_button_text(kb, id);
  if (!txt) return;
  const size_t len = strlen(ref_manual_pos_buf);
  if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    if (len > 0) ref_manual_pos_buf[len - 1] = '\0';
  } else if (strcmp(txt, ".") == 0) {
    if (!strchr(ref_manual_pos_buf, '.') && len > 0 && len + 1 < sizeof(ref_manual_pos_buf)) {
      ref_manual_pos_buf[len] = '.';
      ref_manual_pos_buf[len + 1] = '\0';
    }
  } else if (txt[0] >= '0' && txt[0] <= '9' && txt[1] == '\0' &&
             len + 1 < sizeof(ref_manual_pos_buf)) {
    ref_manual_pos_buf[len] = txt[0];
    ref_manual_pos_buf[len + 1] = '\0';
  }
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  ref_manual_pos_sync_value();
}

static void ref_manual_pos_confirm_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (!ref_manual_pos_buf[0]) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    ui_toast("عدد فعلی دستگاه را وارد کنید", "Enter the current machine position", 2800);
    return;
  }
  const float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm <= 0.0f) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    ui_toast("تنظیمات موتور معتبر نیست", "Motor settings are invalid", 3000);
    return;
  }
  const float value_mm = strtof(ref_manual_pos_buf, NULL);
  const float min_mm = (float)STP.initial_Position_p / ppmm;
  const float max_mm = (float)STP.final_position_p / ppmm;
  if (!isfinite(value_mm) || value_mm < min_mm || value_mm > max_mm) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    ui_toast("عدد باید داخل محدوده اول و آخر خط باشد",
             "Value must be inside the configured axis limits", 3500);
    return;
  }

  const int32_t pulse = (int32_t)lroundf(value_mm * ppmm);
  STP.currentPosition_p = pulse;
  STP.position_Setpoint_p = pulse;
  STP.Man_Sp_mm = value_mm;
  const float rounded = roundf(value_mm);
  GUI.inputBuffer_sp = (fabsf(value_mm - rounded) < 0.01f)
    ? String((int)rounded)
    : String(value_mm, 1);
  GUI.flag_update_lable1_2 = true;
  GUI.update_lable1_1 = true;
  STP.referenced = true;
  pending_reference_confirm = true;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  GUI.lock_screen = false;
  GUI.pending_goto_scr = 1;
  lv_async_call(ref_manual_pos_close_async, NULL);
}

static void ref_manual_pos_prompt_open() {
  if (ref_manual_pos_prompt_is_open()) return;
  ref_manual_pos_buf[0] = '\0';
  const bool en = (GUI.system_language == 2);

  ref_manual_pos_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ref_manual_pos_root, 480, 320);
  lv_obj_set_pos(ref_manual_pos_root, 0, 0);
  lv_obj_set_style_bg_color(ref_manual_pos_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ref_manual_pos_root, LV_OPA_60, 0);
  lv_obj_set_style_border_width(ref_manual_pos_root, 0, 0);
  lv_obj_set_style_pad_all(ref_manual_pos_root, 0, 0);
  lv_obj_clear_flag(ref_manual_pos_root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *card = lv_obj_create(ref_manual_pos_root);
  lv_obj_set_pos(card, 12, 8);
  lv_obj_set_size(card, 456, 304);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0xff9f0a), 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_width(title, 420);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x1c1c1e), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(title, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(title, en ? "First sensor is disconnected"
                              : "سنسور اول خط وصل نیست");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

  lv_obj_t *hint = lv_label_create(card);
  lv_obj_set_width(hint, 420);
  lv_obj_set_style_text_font(hint, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x8a4b00), 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_base_dir(hint, en ? LV_BASE_DIR_LTR : LV_BASE_DIR_RTL, 0);
  lv_label_set_text(hint, en ? "Automatic homing is unavailable. Enter the current position."
                             : "موقعیت یابی خودکار انجام نمی شود؛ عدد فعلی دستگاه را وارد کنید");
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 36);

  ref_manual_pos_value = lv_label_create(card);
  lv_obj_set_width(ref_manual_pos_value, 150);
  lv_obj_set_style_text_font(ref_manual_pos_value, &ui_font_digit40, 0);
  lv_obj_set_style_text_color(ref_manual_pos_value, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_text_align(ref_manual_pos_value, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ref_manual_pos_value, LV_ALIGN_TOP_MID, 0, 62);
  ref_manual_pos_sync_value();

  lv_obj_t *kb = lv_buttonmatrix_create(card);
  lv_obj_set_pos(kb, 16, 112);
  lv_obj_set_size(kb, 424, 132);
  lv_buttonmatrix_set_map(kb, ref_manual_pos_map);
  lv_obj_set_style_text_font(kb, &lv_font_montserrat_26, LV_PART_ITEMS);
  lv_obj_set_style_radius(kb, 8, LV_PART_ITEMS);
  lv_obj_add_event_cb(kb, ref_manual_pos_key_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *cancel = lv_button_create(card);
  lv_obj_set_pos(cancel, 16, 252);
  lv_obj_set_size(cancel, 196, 44);
  lv_obj_set_style_bg_color(cancel, lv_color_hex(0x636366), 0);
  lv_obj_set_style_radius(cancel, 10, 0);
  lv_obj_add_event_cb(cancel, ref_manual_pos_cancel_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *cancel_lbl = lv_label_create(cancel);
  lv_obj_set_style_text_font(cancel_lbl, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(cancel_lbl, en ? "Cancel" : "انصراف");
  lv_obj_center(cancel_lbl);

  lv_obj_t *confirm = lv_button_create(card);
  lv_obj_set_pos(confirm, 228, 252);
  lv_obj_set_size(confirm, 212, 44);
  lv_obj_set_style_bg_color(confirm, lv_color_hex(0x34c759), 0);
  lv_obj_set_style_radius(confirm, 10, 0);
  lv_obj_add_event_cb(confirm, ref_manual_pos_confirm_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *confirm_lbl = lv_label_create(confirm);
  lv_obj_set_style_text_font(confirm_lbl, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(confirm_lbl, lv_color_hex(0xffffff), 0);
  lv_label_set_text(confirm_lbl, en ? "Confirm position" : "تایید موقعیت");
  lv_obj_center(confirm_lbl);

  GUI.lock_screen = true;
  GUI.buzzer_cmd_long = true;
  lv_obj_move_foreground(ref_manual_pos_root);
}

// شروع موقعیت‌یابی = تاچ سبز / استارت پنل / Enter
static void ref_homing_try_start() {
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    ui_toast("صفحه قفل است", "Screen locked", 2500);
    return;
  }
  if (ref_error_ui_active) {
    GUI.buzzer_cmd_short = true;
    ref_homing_restore_ready_ui();
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    ui_toast("استاپ فعال است — آزاد کنید", "E-STOP is active — release it", 3500);
    return;
  }
  // اگر اسپینر در حال اجراست دوباره شروع نکن
  if (objects.spinner0_1 && !lv_obj_has_flag(objects.spinner0_1, LV_OBJ_FLAG_HIDDEN)) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (STP.motorIs_moving) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (!pinmap_in_role_connected(0)) {
    ref_manual_pos_prompt_open();
    return;
  }
  GUI.buzzer_cmd_short = true;
  ref_failed_flag = false;
  ref_wrong_dir_flag = false;
  ref_fail_reason = 0;
  ref_error_ui_active = false;
  STP.mvm_interupt = false;
  GUI.pending_goto_scr = -1;
  STP.goref_cmd = true;
  if (objects.spinner0_1) lv_obj_clear_flag(objects.spinner0_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn0_1) lv_obj_add_flag(objects.btn0_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn0_2) lv_obj_add_flag(objects.btn0_2, LV_OBJ_FLAG_HIDDEN);
}

// ضربدر قرمز / استپ پنل وقتی جایابی در جریان نیست
static void ref_homing_apply_skip_panel_texts() {
  const bool en = (GUI.system_language == 2);
  if (objects.panel0_1) {
    // فقط ویژگی‌های سطح پنجره را override کن. پاک‌کردن تمام styleها در
    // یک شیء تولیدشده، chain چیدمان/تم آن را از بین می‌برد و پیام را می‌ریزد.
    // پنل تولیدشده باید بالای تمام اجزای MAIN_0 باشد؛ در غیر این صورت بخش
    // پایینیِ صفحه از زیر آن دیده می‌شود و پیام نامرتب به نظر می‌رسد.
    if (lv_obj_get_parent(objects.panel0_1) != lv_layer_top())
      lv_obj_set_parent(objects.panel0_1, lv_layer_top());
    lv_obj_set_pos(objects.panel0_1, 24, 34);  // 432×252: دقیقاً وسط 480×320
    lv_obj_set_size(objects.panel0_1, 432, 252);
    lv_obj_set_style_bg_color(objects.panel0_1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.panel0_1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(objects.panel0_1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(objects.panel0_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.panel0_1, lv_color_hex(0xD1D1D6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.panel0_1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.panel0_1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(objects.panel0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(objects.panel0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(objects.panel0_1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(objects.panel0_1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(objects.panel0_1);
  }
  // سطح پنجره سفید است؛ «بله» خطرناک قرمز و «انصراف» آبیِ استاندارد است.
  if (objects.btn0_4) {
    lv_obj_set_pos(objects.btn0_4, 32, 188);
    lv_obj_set_size(objects.btn0_4, 100, 48);
    lv_obj_set_style_bg_color(objects.btn0_4, lv_color_hex(0xD64545), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.btn0_4, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.btn0_4, lv_color_hex(0xB83333), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(objects.btn0_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.btn0_4, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.btn0_5) {
    lv_obj_set_pos(objects.btn0_5, 300, 188);
    lv_obj_set_size(objects.btn0_5, 100, 48);
    lv_obj_set_style_bg_color(objects.btn0_5, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.btn0_5, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.btn0_5, lv_color_hex(0x0077ED), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(objects.btn0_5, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(objects.btn0_5, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.btn0_5, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.btn0_5, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(objects.btn0_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.btn0_3) {
    lv_obj_set_pos(objects.btn0_3, 386, 8);
    lv_obj_set_size(objects.btn0_3, 36, 32);
  }
  if (objects.lablef0_5) {
    lv_obj_set_style_text_color(objects.lablef0_5, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.lablee0_5) {
    lv_obj_set_style_text_color(objects.lablee0_5, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.lablef0_3) {
    lv_obj_set_pos(objects.lablef0_3, 16, 30);
    lv_obj_set_size(objects.lablef0_3, 400, 144);
    lv_obj_set_style_text_font(objects.lablef0_3, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(objects.lablef0_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(objects.lablef0_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablef0_3,
                      "ورود بدون موقعیت‌یابی\n"
                      "احتمال خارج‌شدن اندازه‌گیری سایز برش\n"
                      "از کالیبره زیاد است.\n"
                      "آیا اطمینان دارید؟");
  }
  if (objects.lablee0_3) {
    lv_obj_set_pos(objects.lablee0_3, 16, 32);
    lv_obj_set_size(objects.lablee0_3, 400, 140);
    lv_obj_set_style_text_font(objects.lablee0_3, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(objects.lablee0_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(objects.lablee0_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(objects.lablee0_3,
                      "Enter without homing?\n"
                      "Cut-size measurement may be\n"
                      "out of calibration.\n"
                      "Are you sure?");
  }
  if (objects.lablef0_5) {
    lv_label_set_text(objects.lablef0_5, "انصراف");
    lv_obj_set_style_text_align(objects.lablef0_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(objects.lablef0_5, LV_ALIGN_CENTER, 0, 0);
  }
  if (objects.lablee0_5) {
    lv_label_set_text(objects.lablee0_5, "Cancel");
    lv_obj_set_style_text_align(objects.lablee0_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(objects.lablee0_5, LV_ALIGN_CENTER, 0, 0);
  }
  if (en) {
    if (objects.lablef0_3) lv_obj_add_flag(objects.lablef0_3, LV_OBJ_FLAG_HIDDEN);
    if (objects.lablee0_3) lv_obj_clear_flag(objects.lablee0_3, LV_OBJ_FLAG_HIDDEN);
    if (objects.lablef0_5) lv_obj_add_flag(objects.lablef0_5, LV_OBJ_FLAG_HIDDEN);
    if (objects.lablee0_5) lv_obj_clear_flag(objects.lablee0_5, LV_OBJ_FLAG_HIDDEN);
  } else {
    if (objects.lablee0_3) lv_obj_add_flag(objects.lablee0_3, LV_OBJ_FLAG_HIDDEN);
    if (objects.lablef0_3) lv_obj_clear_flag(objects.lablef0_3, LV_OBJ_FLAG_HIDDEN);
    if (objects.lablee0_5) lv_obj_add_flag(objects.lablee0_5, LV_OBJ_FLAG_HIDDEN);
    if (objects.lablef0_5) lv_obj_clear_flag(objects.lablef0_5, LV_OBJ_FLAG_HIDDEN);
  }
}

static bool ref_skip_panel_is_open() {
  return objects.panel0_1 && lv_obj_is_valid(objects.panel0_1) &&
         !lv_obj_has_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
}

static void ref_homing_confirm_skip_yes() {
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  if (objects.panel0_1) lv_obj_add_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
  STP.referenced = true;
  if (IOS.ref_sensor_state) limit_escape_sensor = 1;
  else if (IOS.up_lim_sensor_state) limit_escape_sensor = 2;
  pending_reference_confirm = true;
  GUI.pending_goto_scr = 1;
}

static void ref_homing_confirm_skip_no() {
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  if (objects.panel0_1) lv_obj_add_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
  ref_homing_restore_ready_ui();
}

static void ref_homing_open_skip_panel() {
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  ref_homing_apply_skip_panel_texts();
  if (objects.panel0_1) lv_obj_clear_flag(objects.panel0_1, LV_OBJ_FLAG_HIDDEN);
}

//-----------------------btn_handlers-------------------------------------
static void btn0_1_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  (void)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) ref_homing_try_start();
}
static void btn0_2_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_PRESSED) {
    // بازخورد واضح برای ضربدر: فقط ظاهر، بدون اثر در منطق موقعیت‌یابی.
    lv_obj_set_style_transform_scale(btn, 282, LV_PART_MAIN | LV_STATE_PRESSED);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    lv_obj_set_style_transform_scale(btn, 256, LV_PART_MAIN | LV_STATE_PRESSED);
    if (code == LV_EVENT_RELEASED) ref_homing_open_skip_panel();
  }
}
static void btn0_3_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  (void)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) ref_homing_confirm_skip_no();
}
static void btn0_4_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  (void)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) ref_homing_confirm_skip_yes();
}
static void btn0_5_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  (void)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) ref_homing_confirm_skip_no();
}
static void btn1_1_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn1_1 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (STP.flag_is_cutting) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (!GUI.lock_screen) {
      if (!STP.motorIs_moving) {
        GUI.buzzer_cmd_short = true;
        GUI.flag_update_labels_3_1 = true;
        GUI.flag_update_labels_3_2 = true;
        GUI.flag_refresh_visible = true;
        request_goto_screen(3);
      } else GUI.buzzer_cmd_long = true;
    } else GUI.buzzer_cmd_long = true;
  }
}
static void icon_spin_anim_cb(void *obj, int32_t v) {
  lv_obj_set_style_transform_rotation((lv_obj_t *)obj, (int32_t)v, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void icon_spin_ready_cb(lv_anim_t *a) {
  if (!a || !a->var) return;
  lv_obj_set_style_transform_rotation((lv_obj_t *)a->var, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static bool settings_gear_entering = false;

static void settings_gear_spin_to(lv_obj_t *btn, int32_t to_angle, uint32_t ms, lv_anim_completed_cb_t done_cb) {
  if (!btn) return;
  lv_obj_update_layout(btn);
  const int32_t pw = lv_obj_get_width(btn);
  const int32_t ph = lv_obj_get_height(btn);
  lv_obj_set_style_transform_pivot_x(btn, pw / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_y(btn, ph / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_anim_delete(btn, icon_spin_anim_cb);
  int32_t cur = lv_obj_get_style_transform_rotation(btn, LV_PART_MAIN);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, btn);
  lv_anim_set_values(&a, cur, to_angle);
  lv_anim_set_time(&a, ms);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_exec_cb(&a, icon_spin_anim_cb);
  if (done_cb) lv_anim_set_completed_cb(&a, done_cb);
  lv_anim_start(&a);
}

static void settings_gear_enter_ready_cb(lv_anim_t *a) {
  settings_gear_entering = false;
  if (a && a->var) {
    lv_obj_set_style_transform_rotation((lv_obj_t *)a->var, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  // فقط فلگ — loadScreen اینجا ممنوع؛ Task2 مصرف می‌کند
  request_goto_screen(2);
}

// چرخش کوتاه مثبت/تنظیمات فقط موقع تاچ؛ بعد از رها شدن به ۰
// تنظیمات (btn1_2): روی RELEASED اینجا برنگرد — ورود با چرخش در click handler است
static void main_icon_press_spin(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  if (!btn) return;
  if (btn == objects.btn1_2 && code == LV_EVENT_RELEASED) return;
  if (btn == objects.btn1_2 && settings_gear_entering) return;
  // محور چرخش = مرکز کلید (وگرنه چرخش دیده نمی‌شود/کج می‌شود)
  lv_obj_update_layout(btn);
  const int32_t pw = lv_obj_get_width(btn);
  const int32_t ph = lv_obj_get_height(btn);
  lv_obj_set_style_transform_pivot_x(btn, pw / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_y(btn, ph / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    settings_gear_spin_to(btn, 0, 160, icon_spin_ready_cb);
    return;
  }
  if (code != LV_EVENT_PRESSED) return;
  settings_gear_spin_to(btn, 450, 160, NULL);  // ~45° — بازخورد لمس
}

static void btn1_2_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn1_2 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code != LV_EVENT_RELEASED) return;
  if (settings_gear_entering) return;
  // گارد کامل: حین برش، ارهٔ روشن، گیرهٔ درگیر یا برشِ در انتظار تأیید → ورود به تنظیمات ممنوع
  if (STP.flag_is_cutting || IOS.saw_is_on || IOS.clamp_engaged || IOS.cut_confirm_pending) {
    GUI.buzzer_cmd_long = true;
    settings_gear_spin_to(btn1_2, 0, 160, icon_spin_ready_cb);
    return;
  }
  if (GUI.lock_screen || STP.motorIs_moving) {
    GUI.buzzer_cmd_long = true;
    settings_gear_spin_to(btn1_2, 0, 160, icon_spin_ready_cb);
    return;
  }
  GUI.buzzer_cmd_short = true;
  load_pending_from_current();
  capture_io_test_outputs();
  GUI.flag_update_lables_2 = true;
  GUI.pending_reinit_settings = true;
  // یک دور چرخش (~۳۶۰°) بعد ورود — بدون loadScreen در تاچ
  settings_gear_entering = true;
  int32_t cur = lv_obj_get_style_transform_rotation(btn1_2, LV_PART_MAIN);
  settings_gear_spin_to(btn1_2, cur + 3600, 300, settings_gear_enter_ready_cb);
}
static void btn1_3_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn1_3 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    AUT.selected_count_val = -1;
    GUI.update_lable1_3 = true;
    GUI.flag_remove_error1_1 = true;
  }
}
static void btn1_4_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn1_4 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    // فقط وقتی واقعاً در مود لیست هستیم جابه‌جا شو — selected==-1 را به ۰ برنگردان
    if (AUT.selected_count_val < 0) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (AUT.selected_count_val < GUI.visible_raws && AUT.selected_count_val < (LIST_MAX - 1)) AUT.selected_count_val++;
    else {
      AUT.selected_count_val = -1;
      AUT.temp_list_count_Val = 0;
      GUI.flag_cast_error1_3 = true;
      GUI.update_lable1_3 = true;
      return;
    }
    STP.Man_Sp_mm = AUT.list_sp_Val_p[AUT.selected_count_val];
    GUI.inputBuffer_sp = GUI.inputBuffer_L_3_1_x[AUT.selected_count_val];
    AUT.temp_list_count_Val = AUT.list_count_Val[AUT.selected_count_val];
    GUI.NumLock = false;
    jog_forward_resume_dest_valid = false;
    GUI.update_lable1_1 = true;
    GUI.update_lable1_3 = true;
    GUI.edit_mode_lable1_1 = true;
    GUI.update_lable1_3 = true;
    GUI.flag_remove_error1_1 = true;
    GUI.flag_update_panel3_2_x_clr = true;
    AUT.in_position_flag = false;
    if (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX) {
      uint8_t sel_ang = AUT.list_angle_Val[AUT.selected_count_val];
      if (sel_ang == 90) {
        pinmap_out_write(IOS.lamp_pin, LOW);
        if (objects.btn2_3_6 && lv_obj_is_valid(objects.btn2_3_6)) {
          lv_obj_add_state(objects.btn2_3_6, LV_STATE_CHECKED);
        }
        if (!cut_angle_90_confirmed) cut_angle_90_prompt_req = true;
      } else {
        pinmap_out_write(IOS.lamp_pin, HIGH);
        if (objects.btn2_3_6 && lv_obj_is_valid(objects.btn2_3_6)) {
          lv_obj_clear_state(objects.btn2_3_6, LV_STATE_CHECKED);
        }
        cut_angle_90_confirmed = false;
      }
    }
    if (AUT.selected_count_val >= GUI.visible_raws) {
      AUT.selected_count_val = -1;
      GUI.flag_cast_error1_3 = true;
    }
  }
}
static void btn1_5_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  (void)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (!STP.referenced) STP.referenced = true;
    if (STP.flag_is_cutting || STP.motorIs_moving) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (main_queue_mode_active() && !GUI.NumLock) {
      request_exit_queue_mode_prompt(2);
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.NumLock = !GUI.NumLock;
    GUI.edit_mode_lable1_1 = !GUI.NumLock;
    AUT.selected_count_val = -1;
    GUI.update_lable1_3 = true;
    GUI.flag_remove_error1_1 = true;
    GUI.flag_update_panel3_2_x_clr = true;
    GUI.buzzer_cmd_short = true;
    ui_toast(GUI.NumLock ? "حالت جاگ فعال" : "حالت جاگ خاموش",
             GUI.NumLock ? "Jog mode ON" : "Jog mode OFF", 1500);
    // حالت عدد = LED روشن؛ جاگ = خاموش
    usb_keyboard_set_numlock_led(!GUI.NumLock);
    if (!GUI.NumLock) {
      STP.jogEnabled = false;
      keyboardJogActive = false;
      jog_forward_resume_dest_valid = false;
    }
    // LVGL فقط از همین تسک؛ کادر با prcs_panel1_2_clr همگام می‌شود
  }
}
static void btn1_6_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn1_6 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  (void)btn1_6;
  if (code == LV_EVENT_RELEASED) {
    GUI.NumLock = false;  // حالت عدد — نه جاگ اجباری
    jog_forward_resume_dest_valid = false;
    GUI.flag_remove_error1_3 = true;
    GUI.flag_update_panel3_2_x_clr = true;
    AUT.selected_count_val = -1;
    GUI.update_lable1_3 = true;
    GUI.buzzer_cmd_short = true;
  }
}
static void btn1_7_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED) {
    AUT.selected_count_val = -1;
    AUT.temp_list_count_Val = 0;
    GUI.update_lable1_3 = true;
    GUI.flag_remove_error1_3 = true;
  }
}

static String format_list_length_mm(float v);  // تعریف پایین‌تر

// اسکراب افقی عدد مقصد (بدون حرکت موتور). جابه‌جایی ≥۱۰px = اسکراب نه دابل‌تاچ
static bool sp_drag_active = false;
static bool sp_drag_moved = false;       // جابه‌جایی انگشت — برای لغو کلیک/دابل‌تاچ
static bool sp_drag_did_scrub = false;   // واقعاً عدد عوض شده — فقط این راهنما را ببندد
static lv_point_t sp_drag_last = {0, 0};
static lv_point_t sp_drag_origin = {0, 0};
static uint32_t sp_drag_last_ms = 0;
static float sp_drag_accum = 0.0f;
static const int SP_SCRUB_TRAVEL_PX = 18;
static const int SP_DOUBLE_TAP_DISTANCE_PX = 32;

static void main_sp_apply_scrub_mm(float mm) {
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  float initial_mm = (float)STP.initial_Position_p / ppmm;
  float final_mm = STP.final_position_mm.toFloat();
  if (final_mm < initial_mm) final_mm = initial_mm;
  if (mm < initial_mm) mm = initial_mm;
  if (mm > final_mm) mm = final_mm;
  if (mm > 9999.9f) mm = 9999.9f;
  mm = roundf(mm * 10.0f) / 10.0f;
  if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)2) == pdTRUE) {
    GUI.inputBuffer_sp = format_list_length_mm(mm);
    xSemaphoreGive(lable1_1_Semaphore);
  } else {
    GUI.inputBuffer_sp = format_list_length_mm(mm);
  }
  STP.Man_Sp_mm = mm;
  AUT.in_position_flag = false;
  STP.in_position_flag = false;
  GUI.edit_mode_lable1_1 = true;
  GUI.update_lable1_1 = true;
  clear_buffer_sp_flag = false;
}

static void lable1_1_drag_event_handler(lv_event_t *e);
static void lable1_1_click_event_handler(lv_event_t *e);

// بعد از ensure_safe_main1 لیبل مقصد عوض می‌شود — باید دوباره bind شود
void main_bind_destination_scrub_handlers() {
  if (objects.lable1_1) {
    lv_obj_remove_event_cb(objects.lable1_1, lable1_1_drag_event_handler);
    lv_obj_remove_event_cb(objects.lable1_1, lable1_1_click_event_handler);
    lv_obj_add_flag(objects.lable1_1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(objects.lable1_1, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(objects.lable1_1, lable1_1_drag_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(objects.lable1_1, lable1_1_drag_event_handler, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(objects.lable1_1, lable1_1_drag_event_handler, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(objects.lable1_1, lable1_1_drag_event_handler, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(objects.lable1_1, lable1_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  }
  if (objects.panel1_2) {
    lv_obj_remove_event_cb(objects.panel1_2, lable1_1_drag_event_handler);
    lv_obj_remove_event_cb(objects.panel1_2, lable1_1_click_event_handler);
    lv_obj_add_flag(objects.panel1_2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(objects.panel1_2, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(objects.panel1_2, lable1_1_drag_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(objects.panel1_2, lable1_1_drag_event_handler, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(objects.panel1_2, lable1_1_drag_event_handler, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(objects.panel1_2, lable1_1_drag_event_handler, LV_EVENT_PRESS_LOST, NULL);
    // کلیک فقط روی لیبل مقصد — پنل همپوشان دابل‌تاچ را دوبار نمی‌زند
  }
}

bool main_destination_scrub_moved() {
  // راهنما: فقط اگر اسکراب واقعاً عدد را عوض کرده — نویز ۱۰px نگهداشتن ۲ثانیه کافی نیست
  return sp_drag_did_scrub;
}

static void lable1_1_drag_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  const bool jog_held = keyboardJogActive || IOS.joystick_state_L || IOS.joystick_state_R;
  // مسدود فقط وقتی واقعاً خطرناک است — نه به‌خاطر referenced/NumLock خالی
  if (GUI.lock_screen || STP.flag_is_cutting || STP.motorIs_moving || STP.moveStepperCMD ||
      STP.begin_calibration || (GUI.NumLock && jog_held)) {
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
      sp_drag_active = false;
      sp_drag_moved = false;
      sp_drag_did_scrub = false;
    }
    return;
  }

  lv_indev_t *indev = lv_indev_active();
  lv_point_t p = {0, 0};
  if (indev) lv_indev_get_point(indev, &p);
  uint32_t now = millis();

  if (code == LV_EVENT_PRESSED) {
    sp_drag_active = true;
    sp_drag_moved = false;
    sp_drag_did_scrub = false;
    sp_drag_last = p;
    sp_drag_origin = p;
    sp_drag_last_ms = now;
    sp_drag_accum = 0.0f;
    return;
  }

  if (code == LV_EVENT_PRESSING && sp_drag_active) {
    int dx = (int)p.x - (int)sp_drag_last.x;
    int ox = (int)p.x - (int)sp_drag_origin.x;
    int oy = (int)p.y - (int)sp_drag_origin.y;
    if (!sp_drag_moved && (LV_ABS(ox) >= SP_SCRUB_TRAVEL_PX || LV_ABS(oy) >= SP_SCRUB_TRAVEL_PX)) {
      sp_drag_moved = true;
    }
    // تا افقی واضح نشده اسکراب نکن؛ نویز عمودی نمونه‌به‌نمونه را نادیده بگیر
    if (LV_ABS(ox) < SP_SCRUB_TRAVEL_PX) {
      sp_drag_last = p;
      sp_drag_last_ms = now;
      return;
    }
    if (LV_ABS(oy) > LV_ABS(ox) + 24) {
      sp_drag_last = p;
      sp_drag_last_ms = now;
      return;
    }
    if (dx == 0) {
      sp_drag_last = p;
      sp_drag_last_ms = now;
      return;
    }
    uint32_t dt = now - sp_drag_last_ms;
    if (dt < 1) dt = 1;
    float speed = (float)LV_ABS(dx) / (float)dt;
    float mm_per_px = 0.22f + speed * 2.5f;
    if (mm_per_px > 8.0f) mm_per_px = 8.0f;
    // چپ→راست = زیاد، راست→چپ = کم
    sp_drag_accum += (float)dx * mm_per_px;
    sp_drag_last = p;
    sp_drag_last_ms = now;
    if (fabsf(sp_drag_accum) >= 0.1f) {
      float cur = GUI.inputBuffer_sp.length() ? GUI.inputBuffer_sp.toFloat() : STP.Man_Sp_mm;
      float next = cur + sp_drag_accum;
      sp_drag_accum = 0.0f;
      main_sp_apply_scrub_mm(next);
      sp_drag_did_scrub = true;
    }
    return;
  }

  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sp_drag_active = false;
  }
}

static void lable1_1_click_event_handler(lv_event_t *e) {
  // دابل‌تاچ روی عدد مقصد: حرکت / در لیست‌بهینه اگر روی مقصد هستیم → عدد بعدی
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  // اگر اسکراب بوده، دابل‌تاچ/کلیک حرکت ندهد
  if (sp_drag_moved) {
    sp_drag_moved = false;
    return;
  }
  if (sp_drag_did_scrub) {
    sp_drag_did_scrub = false;
    return;
  }
  if (GUI.lock_screen || STP.flag_is_cutting) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (!STP.referenced) STP.referenced = true;
  if (GUI.NumLock) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  static uint32_t sp_last_tap_ms = 0;
  static uint32_t sp_dbl_ready_ms = 0;  // تا این زمان دابل‌تاچ بعدی قبول نشود
  static lv_point_t sp_last_tap_point = {0, 0};
  static bool sp_last_tap_point_valid = false;
  uint32_t now = millis();
  if ((int32_t)(now - sp_dbl_ready_ms) < 0) {
    // فاصلهٔ اجباری ۱ثانیه بعد از دابل‌تاچ قبلی
    sp_last_tap_ms = 0;
    sp_last_tap_point_valid = false;
    GUI.buzzer_cmd_long = true;
    return;
  }

  lv_point_t tap_point = {0, 0};
  lv_indev_t *indev = lv_indev_active();
  const bool tap_point_valid = indev != NULL;
  if (indev) lv_indev_get_point(indev, &tap_point);
  bool close_enough = true;
  if (tap_point_valid && sp_last_tap_point_valid) {
    const int dx = (int)tap_point.x - (int)sp_last_tap_point.x;
    const int dy = (int)tap_point.y - (int)sp_last_tap_point.y;
    close_enough = (dx * dx + dy * dy) <=
                   (SP_DOUBLE_TAP_DISTANCE_PX * SP_DOUBLE_TAP_DISTANCE_PX);
  }
  bool dbl = (sp_last_tap_ms != 0) && ((now - sp_last_tap_ms) <= 700u) && close_enough;
  sp_last_tap_ms = now;
  sp_last_tap_point = tap_point;
  sp_last_tap_point_valid = tap_point_valid;
  if (!dbl) {
    GUI.buzzer_cmd_short = true;
    // USB نبود یا کلید کیبرد مجازی روشن → همان کیبرد لیست زیر عدد مقصد
    // (در جاگ/لیست/بهینه numpad_open همان پیغام خروج USB را می‌آورد)
    if (GUI.virtual_kb_enable && !STP.motorIs_moving && !STP.moveStepperCMD) {
      numpad_open(1);
      sp_last_tap_ms = 0;  // دابل‌تاچ با نامپد باز قاطی نشود
      sp_last_tap_point_valid = false;
    } else if (!GUI.virtual_kb_enable && (GUI.NumLock || main_queue_mode_active()) &&
               !STP.motorIs_moving && !STP.moveStepperCMD) {
      // USB وصل است ولی لمس مقصد برای تایپ — همان پیغام خروج مد
      if (GUI.NumLock) request_exit_queue_mode_prompt(3);
      else request_exit_queue_mode_prompt(1);
      sp_last_tap_ms = 0;
      sp_last_tap_point_valid = false;
    }
    return;
  }
  GUI.buzzer_cmd_short = true;
  if (STP.motorIs_moving || STP.moveStepperCMD) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  // روی مقصد در لیست/بهینه، دابل‌تاچ برش محسوب نمی‌شود.
  if (main_queue_mode_active() && main_is_at_destination()) {
    sp_dbl_ready_ms = now + 1000u;
    sp_last_tap_ms = 0;
    sp_last_tap_point_valid = false;
    return;
  }
  // دابل‌تاچ = حرکت به مقصد فعلی
  commit_main_sp(true);
  sp_dbl_ready_ms = now + 1000u;
  sp_last_tap_ms = 0;
  sp_last_tap_point_valid = false;
}
static void lable1_2_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *lable1_2 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  (void)lable1_2;
  if (code == LV_EVENT_CLICKED) {
    // moveStepperCMD هم بین دو پای جبران‌لقی true است — فقط motorIs_moving کافی نبود
    if (STP.motorIs_moving || STP.moveStepperCMD) {
      STP.mvm_interupt = true;
      GUI.buzzer_cmd_long = true;
    }
  }
}
static void slider2_1_val_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    char buf[8];
    int v = (int)lv_slider_get_value(objects.slider2_1);
    static int last_v1 = -1;
    if (abs(v - last_v1) >= 50) {
      last_v1 = v;
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_STEP_UP);
    }
    snprintf(buf, sizeof(buf), "%d%%", v / 10);
    lv_label_set_text(objects.lbl_spd2_1, buf);
    settings_mark_dirty();
  }
}
static void slider2_2_val_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    char buf[8];
    int v = (int)lv_slider_get_value(objects.slider2_2);
    static int last_v2 = -1;
    if (abs(v - last_v2) >= 50) {
      last_v2 = v;
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_STEP_UP);
    }
    snprintf(buf, sizeof(buf), "%d%%", v / 10);
    lv_label_set_text(objects.lbl_spd2_2, buf);
    // اعمال فوری شیب (۰٪=۰ms … ۱۰۰٪=۵۰۰۰ms)
    auto_ramp_ms = (int)map(constrain(v, 0, 1000), 0, 1000, 0, 5000);
    STP.global_lowSpeedTime = auto_ramp_ms;
    settings_mark_dirty();
  }
}
static void btn_save_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    // NVS و apply فقط در Task2 — جلوگیری از هنگ لمس
    GUI.flag_save_all = true;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  }
}
static bool f2_seeking_limit1 = false;
static bool f2_seeking_limit2 = false;

static void btn2_1_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn2_1 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (f2_seeking_limit1 || f2_seeking_limit2 || STP.motorIs_moving) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      ui_toast("موتور درحال حرکت است. صبر کنید یا کلید زرد را برای توقف لمس کنید",
               "Motor is moving. Wait or press yellow button to stop", 3000);
      return;
    }
    if (!STP.begin_calibration) {
      if (!GUI.cast_message) {
        GUI.buzzer_cmd_short = true;
        turn_off_test_outputs();
        restore_io_test_outputs();
        request_goto_screen(1);
      } else GUI.buzzer_cmd_long = true;
    } else GUI.buzzer_cmd_long = true;
  }
}
static void img2_1_click_event_handler(lv_event_t *e) {
  (void)e;
  // تصاویر بازر EEZ بازنشسته شدند — بازر فقط از چیپ F1
}
static void img2_2_click_event_handler(lv_event_t *e) {
  (void)e;
}

static void btn2_3_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn2_3 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen || IOS.stop_button_state) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      return;
    }
    if (f2_seeking_limit1) {
      extern void panel_request_stop_motion(void);
      panel_request_stop_motion();
      f2_seeking_limit1 = false;
      lv_obj_set_style_bg_color(btn2_3, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (objects.lablef2_8) lv_obj_set_style_text_color(objects.lablef2_8, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (objects.lablee2_8) lv_obj_set_style_text_color(objects.lablee2_8, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
      ui_toast("توقف حرکت موتور", "Motor motion stopped", 1800);
      return;
    }
    if (STP.motorIs_moving) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      ui_toast("موتور درحال حرکت است. صبر کنید یا کلید زرد را برای توقف لمس کنید",
               "Motor is moving. Wait or press yellow button to stop", 3000);
      return;
    }
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    f2_seeking_limit1 = true;
    lv_obj_set_style_bg_color(btn2_3, lv_color_hex(0xffd60a), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.lablef2_8) lv_obj_set_style_text_color(objects.lablef2_8, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.lablee2_8) lv_obj_set_style_text_color(objects.lablee2_8, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    STP.referenced = false;
    STP.goref_cmd = true;
    ui_toast("حرکت به سمت سنسور اول خط... (برای توقف کلید زرد را بزنید)",
             "Seeking Start Sensor... (Press yellow button to stop)", 3000);
  }
}
static void btn2_4_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn2_4 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen || IOS.stop_button_state) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      return;
    }
    if (f2_seeking_limit2) {
      extern void panel_request_stop_motion(void);
      panel_request_stop_motion();
      f2_seeking_limit2 = false;
      lv_obj_set_style_bg_color(btn2_4, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (objects.lablef2_9) lv_obj_set_style_text_color(objects.lablef2_9, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (objects.lablee2_9) lv_obj_set_style_text_color(objects.lablee2_9, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
      ui_toast("توقف حرکت موتور", "Motor motion stopped", 1800);
      return;
    }
    if (STP.motorIs_moving) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
      ui_toast("موتور درحال حرکت است. صبر کنید یا کلید زرد را برای توقف لمس کنید",
               "Motor is moving. Wait or press yellow button to stop", 3000);
      return;
    }
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    f2_seeking_limit2 = true;
    lv_obj_set_style_bg_color(btn2_4, lv_color_hex(0xffd60a), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.lablef2_9) lv_obj_set_style_text_color(objects.lablef2_9, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.lablee2_9) lv_obj_set_style_text_color(objects.lablee2_9, lv_color_hex(0x1c2b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    STP.position_Setpoint_p = STP.final_position_p;
    STP.moveStepperCMD = true;
    ui_toast("حرکت به سمت سنسور آخر خط... (برای توقف کلید زرد را بزنید)",
             "Seeking End Sensor... (Press yellow button to stop)", 3000);
  }
}
static void btn2_5_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn2_5 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    STP.begin_calibration = true;
    STP.calib_stage = 1;
    STP.go_sampling_CMD = true;
    GUI.cast_message = false;
    GUI.clear_buffer_aut_flag = true;
    lv_obj_add_flag(objects.panel2_1, LV_OBJ_FLAG_HIDDEN);
  }
}
static void btn2_6_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn2_6 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    GUI.cast_message = false;
    lv_obj_add_flag(objects.panel2_1, LV_OBJ_FLAG_HIDDEN);
  }
}
static void btn2_7_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn2_7 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    GUI.cast_message = false;
    lv_obj_add_flag(objects.panel2_1, LV_OBJ_FLAG_HIDDEN);
  }
}
static void btn2_8_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn2_8 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    GUI.enable_edit_lable2_5 = true;
    GUI.inputBuffer_max_L = "0";
    GUI.flag_update_lable2_5 = true;
    lv_obj_set_style_text_color(objects.lable2_5, lv_color_hex(0x0011d1), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.lable2_3)
      lv_obj_set_style_text_color(objects.lable2_3, lv_color_hex(0x0011d1), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}
// کادر بالا (اول خط) — فوکوس ورودی + نشانگر؛ رقم اول جایگزین می‌کند
static void lable2_2_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (STP.begin_calibration || GUI.cast_message) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    f2_begin_edit_start_line();
  }
}
// کادر پایین (آخر خط) — فوکوس ورودی + نشانگر
static void lable2_3_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (STP.begin_calibration || GUI.cast_message) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    f2_begin_edit_end_line();
  }
}

static bool screen3_edit_snapshot_valid = false;
static bool screen3_edit_snapshot_is_count = false;
static int screen3_edit_snapshot_index = -1;
static String screen3_edit_snapshot_value = "0";

static void cancel_screen3_pending_edit();

static void begin_screen3_sp_edit(int index) {
  if (index < 0 || index >= LIST_MAX) return;
  // همان سلول از قبل انتخاب است → فقط آبی را نگه دار (دوبار کلیک مقدار را پاک نکند)
  if (AUT.list_edit_mode_sp[index] && screen3_edit_snapshot_valid &&
      !screen3_edit_snapshot_is_count && screen3_edit_snapshot_index == index) {
    GUI.flag_panel3_edit_labels_txt = true;
    return;
  }
  // همیشه فقط همین سلول — بدون toggle خاموش (دوبار کلیک/btn+label انتخاب را می‌کشت)
  if (screen3_edit_snapshot_valid) cancel_screen3_pending_edit();
  for (int i = 0; i < LIST_MAX; i++) {
    AUT.list_edit_mode_sp[i] = false;
    AUT.list_edit_mode_count[i] = false;
  }
  screen3_edit_snapshot_valid = true;
  screen3_edit_snapshot_is_count = false;
  screen3_edit_snapshot_index = index;
  screen3_edit_snapshot_value = GUI.inputBuffer_L_3_1_x[index];
  AUT.list_edit_mode_sp[index] = true;
  GUI.clear_buffer_aut_flag = true;
  GUI.flag_update_labels_3_1 = true;
  GUI.flag_panel3_edit_labels_txt = true;
}

static void begin_screen3_count_edit(int index) {
  if (index < 0 || index >= LIST_MAX) return;
  if (AUT.list_edit_mode_count[index] && screen3_edit_snapshot_valid &&
      screen3_edit_snapshot_is_count && screen3_edit_snapshot_index == index) {
    GUI.flag_panel3_edit_labels_txt = true;
    return;
  }
  // مثل sp: فقط یک سلول آبی — پاک‌کردن بقیه قبل از فعال‌کردن
  if (screen3_edit_snapshot_valid) cancel_screen3_pending_edit();
  for (int i = 0; i < LIST_MAX; i++) {
    AUT.list_edit_mode_sp[i] = false;
    AUT.list_edit_mode_count[i] = false;
  }
  screen3_edit_snapshot_valid = true;
  screen3_edit_snapshot_is_count = true;
  screen3_edit_snapshot_index = index;
  screen3_edit_snapshot_value = GUI.inputBuffer_L_3_2_x[index];
  AUT.list_edit_mode_count[index] = true;
  GUI.clear_buffer_aut_flag = true;
  GUI.flag_update_labels_3_2 = true;
  GUI.flag_panel3_edit_labels_txt = true;
}

static void commit_screen3_pending_edit() {
  screen3_edit_snapshot_valid = false;
  screen3_edit_snapshot_index = -1;
}

static void cancel_screen3_pending_edit() {
  if (screen3_edit_snapshot_valid && screen3_edit_snapshot_index >= 0 && screen3_edit_snapshot_index < LIST_MAX) {
    if (screen3_edit_snapshot_is_count) {
      GUI.inputBuffer_L_3_2_x[screen3_edit_snapshot_index] = screen3_edit_snapshot_value;
      AUT.list_count_Val[screen3_edit_snapshot_index] = screen3_edit_snapshot_value.toInt();
      save_permanently_preferences_count_val(screen3_edit_snapshot_index, AUT.list_count_Val[screen3_edit_snapshot_index]);
      GUI.flag_update_labels_3_2 = true;
    } else {
      GUI.inputBuffer_L_3_1_x[screen3_edit_snapshot_index] = screen3_edit_snapshot_value;
      AUT.list_sp_Val_p[screen3_edit_snapshot_index] = screen3_edit_snapshot_value.toFloat();
      save_permanently_preferences_sp_val(screen3_edit_snapshot_index, AUT.list_sp_Val_p[screen3_edit_snapshot_index]);
      GUI.flag_update_labels_3_1 = true;
    }
  }
  for (int i = 0; i < LIST_MAX; i++) {
    AUT.list_edit_mode_sp[i] = false;
    AUT.list_edit_mode_count[i] = false;
  }
  screen3_edit_snapshot_valid = false;
  screen3_edit_snapshot_index = -1;
  GUI.clear_buffer_aut_flag = true;
  GUI.flag_panel3_edit_labels_txt = true;
}

//---------- commit مشترک ورودی‌ها (کیبورد USB + نامپد لمسی) با پیام خطای محدوده ----------
static void format_fa_uint(uint32_t value, char *out, size_t out_size) {
  if (!out || out_size < 3) return;
  char ascii[12];
  snprintf(ascii, sizeof(ascii), "%lu", (unsigned long)value);
  size_t pos = 0;
  for (const char *p = ascii; *p && (pos + 2) < out_size; ++p) {
    out[pos++] = (char)0xD9;
    out[pos++] = (char)(0xA0 + (*p - '0'));
  }
  out[pos] = '\0';
}

static void range_toast(int lo, int hi) {
  char fa[96], en[64], fa_lo[24], fa_hi[24];
  format_fa_uint((uint32_t)lo, fa_lo, sizeof(fa_lo));
  format_fa_uint((uint32_t)hi, fa_hi, sizeof(fa_hi));
  snprintf(fa, sizeof(fa), "عدد خارج از محدوده مجاز است: %s تا %s", fa_lo, fa_hi);
  snprintf(en, sizeof(en), "Allowed size: %d to %d mm", lo, hi);
  ui_toast(fa, en, 3000);
}

// نمایش متراژ لیست با دقت ۰.۱mm (بدون دور ریختن ممیز روی تأیید)
static String format_list_length_mm(float v) {
  char b[16];
  float r = roundf(v * 10.0f) / 10.0f;
  if (fabsf(r - roundf(r)) < 0.05f)
    snprintf(b, sizeof(b), "%d", (int)lroundf(r));
  else
    snprintf(b, sizeof(b), "%.1f", (double)r);
  return String(b);
}

// اگر ردیفی خالی شد (متراژ ۰ و تعداد ۰)، ردیف‌های دارای مقدار یا تمام‌شده (خط‌خورده) بعد از آن به بالا شیفت پیدا کنند
void list_compact_after_zero(int empty_idx) {
  if (empty_idx < 0 || empty_idx >= LIST_MAX) return;
  // بررسی آیا ردیف واقعاً خالی است (ردیف تمام‌شده دارای base>0 و sp>0 خالی محسوب نمی‌شود)
  const bool is_depleted = (AUT.list_sp_Val_p[empty_idx] > 0.05f &&
                            AUT.list_count_base_Val[empty_idx] > 0 &&
                            AUT.list_count_Val[empty_idx] == 0);
  if (is_depleted || AUT.list_sp_Val_p[empty_idx] > 0.05f || AUT.list_count_Val[empty_idx] > 0) {
    return; // خالی نیست یا برش‌خورده/دارای مقدار است
  }
  // پیدا کردن آخرین ردیف معتبر بعد از empty_idx
  int last_valid = -1;
  for (int i = LIST_MAX - 1; i > empty_idx; i--) {
    bool has_data = (AUT.list_sp_Val_p[i] > 0.05f || AUT.list_count_Val[i] > 0 ||
                     (AUT.list_count_base_Val[i] > 0 && AUT.list_sp_Val_p[i] > 0.05f));
    if (has_data) { last_valid = i; break; }
  }
  if (last_valid <= empty_idx) return;

  for (int i = empty_idx; i < last_valid; i++) {
    AUT.list_sp_Val_p[i] = AUT.list_sp_Val_p[i + 1];
    AUT.list_count_Val[i] = AUT.list_count_Val[i + 1];
    AUT.list_count_base_Val[i] = AUT.list_count_base_Val[i + 1];
    AUT.list_angle_Val[i] = AUT.list_angle_Val[i + 1];
    GUI.inputBuffer_L_3_1_x[i] = GUI.inputBuffer_L_3_1_x[i + 1];
    GUI.inputBuffer_L_3_2_x[i] = GUI.inputBuffer_L_3_2_x[i + 1];
    save_permanently_preferences_sp_val(i, AUT.list_sp_Val_p[i]);
    save_permanently_preferences_count_val(i, AUT.list_count_Val[i]);
    save_permanently_preferences_count_base(i, AUT.list_count_base_Val[i]);
    save_permanently_preferences_angle_val(i, AUT.list_angle_Val[i]);
  }
  // خالی کردن آخرین ردیف
  AUT.list_sp_Val_p[last_valid] = 0.0f;
  AUT.list_count_Val[last_valid] = 0;
  AUT.list_count_base_Val[last_valid] = 0;
  AUT.list_angle_Val[last_valid] = 45;
  GUI.inputBuffer_L_3_1_x[last_valid] = "0";
  GUI.inputBuffer_L_3_2_x[last_valid] = "0";
  save_permanently_preferences_sp_val(last_valid, 0.0f);
  save_permanently_preferences_count_val(last_valid, 0);
  save_permanently_preferences_count_base(last_valid, 0);
  save_permanently_preferences_angle_val(last_valid, 45);

  GUI.flag_update_labels_3_1 = true;
  GUI.flag_update_labels_3_2 = true;
  GUI.flag_panel3_edit_labels_txt = true;
  GUI.flag_update_panel3_2_x_clr = true;
  opt_notify_list_edited();
}

void commit_sp_entry(int j) {
  if (j < 0 || j >= LIST_MAX) return;
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  int initial_mm = (int)round((float)STP.initial_Position_p / ppmm);
  int final_mm = STP.final_position_mm.toInt();
  int max_mm = (stock_len_mm > 0) ? stock_len_mm : final_mm;
  if (final_mm > 0 && max_mm > final_mm) max_mm = final_mm;
  if (max_mm < initial_mm) max_mm = initial_mm;
  float v = GUI.inputBuffer_L_3_1_x[j].toFloat();
  if (GUI.clear_buffer_aut_flag) v = AUT.list_sp_Val_p[j];
  GUI.clear_buffer_aut_flag = true;
  v = roundf(v * 10.0f) / 10.0f;
  if (v == 0.0f) {
    AUT.list_sp_Val_p[j] = 0.0f;
    AUT.list_count_Val[j] = 0;
    AUT.list_count_base_Val[j] = 0;
    GUI.inputBuffer_L_3_1_x[j] = "0";
    GUI.inputBuffer_L_3_2_x[j] = "0";
    save_permanently_preferences_sp_val(j, 0.0f);
    save_permanently_preferences_count_val(j, 0);
    save_permanently_preferences_count_base(j, 0);
    GUI.pending_sp_save_bits[j >> 3] |= (uint8_t)(1u << (j & 7));
    GUI.pending_count_save_bits[j >> 3] |= (uint8_t)(1u << (j & 7));
    opt_notify_list_edited();
    commit_screen3_pending_edit();
    AUT.list_edit_mode_sp[j] = false;
    AUT.list_edit_mode_count[j] = false;
    list_compact_after_zero(j);
    GUI.flag_panel3_edit_labels_txt = true;
    GUI.flag_update_labels_3_1 = true;
    GUI.flag_update_labels_3_2 = true;
    numpad_close();
    return;
  }
  bool out_of_range = false;
  if (v > (float)max_mm) {
    v = (float)max_mm;
    out_of_range = true;
    range_toast(initial_mm, max_mm);
  } else if (v < (float)initial_mm) {
    v = (float)initial_mm;
    out_of_range = true;
    range_toast(initial_mm, max_mm);
  }
  AUT.list_sp_Val_p[j] = v;
  GUI.inputBuffer_L_3_1_x[j] = format_list_length_mm(v);
  save_permanently_preferences_sp_val(j, AUT.list_sp_Val_p[j]);
  GUI.pending_sp_save_bits[j >> 3] |= (uint8_t)(1u << (j & 7));  // NVS در Task2
  // متراژ با تعداد صفر = ردیف تازه؛ خط‌خوردگی «برش‌خورده» نشان نده
  if (AUT.list_count_Val[j] <= 0) {
    AUT.list_count_base_Val[j] = 0;
    save_permanently_preferences_count_base(j, 0);
  }
  GUI.flag_update_labels_3_1 = true;
  opt_notify_list_edited();
  if (out_of_range) {
    AUT.list_edit_mode_sp[j] = true;
    AUT.list_edit_mode_count[j] = false;
    screen3_edit_snapshot_valid = true;
    screen3_edit_snapshot_is_count = false;
    screen3_edit_snapshot_index = j;
    screen3_edit_snapshot_value = GUI.inputBuffer_L_3_1_x[j];
    GUI.flag_panel3_edit_labels_txt = true;
    return;
  }
  commit_screen3_pending_edit();
  AUT.list_edit_mode_sp[j] = false;
  begin_screen3_count_edit(j);  // بعد از متراژ، مستقیم تعداد ادیت می‌شود
}

void commit_count_entry(int j) {
  if (j < 0 || j >= LIST_MAX) return;
  int raw_count = GUI.inputBuffer_L_3_2_x[j].toInt();
  if (GUI.clear_buffer_aut_flag) raw_count = AUT.list_count_Val[j];
  int c = constrain(raw_count, 0, 999);
  if (raw_count != c) ui_toast("حداکثر تعداد هر ردیف ٩٩٩ است", "Row quantity capacity is 999", 3500);
  AUT.list_count_Val[j] = c;
  // ورود دستی، مبنای تازه است؛ حتی صفر نباید «برش‌خورده» تلقی شود.
  // فقط مسیر تأیید برش مقدار جاری را کم می‌کند و مبنا را نگه می‌دارد.
  AUT.list_count_base_Val[j] = c;
  save_permanently_preferences_count_base(j, c);
  GUI.inputBuffer_L_3_2_x[j] = String(c);
  save_permanently_preferences_count_val(j, AUT.list_count_Val[j]);
  GUI.pending_count_save_bits[j >> 3] |= (uint8_t)(1u << (j & 7));  // NVS در Task2
  if (c == 0 && AUT.list_sp_Val_p[j] <= 0.05f) {
    list_compact_after_zero(j);
  }
  opt_notify_list_edited();
  commit_screen3_pending_edit();
  AUT.list_edit_mode_count[j] = false;
  GUI.flag_panel3_edit_labels_txt = true;
  GUI.flag_update_labels_3_2 = true;
  int next = j + 1;
  if (next < GUI.visible_raws && next < LIST_MAX) {
    list_show_row(next);
    begin_screen3_sp_edit(next);
    numpad_open(2);
  } else if (next == GUI.visible_raws && next < LIST_MAX) {
    list_zero_row(next);
    GUI.pending_zero_bits[next >> 3] |= (uint8_t)(1u << (next & 7));
    GUI.visible_raws = next + 1;
    PEND.visible_raws = GUI.visible_raws;
    GUI.flag_save_visible_raws = true;
    list_show_row(next);
    begin_screen3_sp_edit(next);
    numpad_open(2);
  } else if (next >= LIST_MAX) {
    char fa[80], en[64];
    snprintf(fa, sizeof(fa), "ظرفیت لیست کامل شد؛ بیشتر از %d ردیف نمی‌شود", LIST_MAX);
    snprintf(en, sizeof(en), "List limit reached: %d rows", LIST_MAX);
    ui_toast(fa, en, 3500);
  }
}

// آیا محور روی همان عدد مقصد ایستاده؟ (با تلورانس ۰٫۲mm)
// فقط فاصلهٔ واقعی — in_position_flag به‌تنهایی کافی نیست (اسکراب/کیبرد عدد را عوض می‌کند)
static bool main_is_at_destination() {
  if (STP.motorIs_moving || STP.flag_is_cutting) return false;
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  float dest = STP.Man_Sp_mm;
  if (xSemaphoreTake(lable1_1_Semaphore, pdMS_TO_TICKS(5)) == pdTRUE) {
    if (GUI.inputBuffer_sp.length()) dest = GUI.inputBuffer_sp.toFloat();
    xSemaphoreGive(lable1_1_Semaphore);
  }
  const int target_p = blade_err_motor_target_p(dest);
  // مقصد UI ثابت می‌ماند؛ فاصله با هدف واقعی اصلاح‌شده سنجیده می‌شود.
  return abs(STP.currentPosition_p - target_p) <= (int)lroundf(ppmm * 0.2f);
}

void main_consume_active_queue_piece(bool record_physical_cut) {
  if (OPT.active) {
    if (OPT.cur < OPT.piece_count) {
      if (record_physical_cut && OPT.cut_log_n < OPT_CUT_LOG_MAX) {
        OPT.cut_log_mm[OPT.cut_log_n] = OPT.piece_len[OPT.cur];
        OPT.cut_log_bar[OPT.cut_log_n] = (int16_t)OPT.piece_bar[OPT.cur];
        OPT.cut_log_n++;
      }
      OPT.piece_done[OPT.cur] = true;
      int en = OPT.piece_entry[OPT.cur];
      if (en >= 0 && en < LIST_MAX && AUT.list_count_Val[en] > 0) {
        if (AUT.list_count_base_Val[en] < AUT.list_count_Val[en])
          AUT.list_count_base_Val[en] = AUT.list_count_Val[en];
        AUT.list_count_Val[en]--;
        GUI.inputBuffer_L_3_2_x[en] = String(AUT.list_count_Val[en]);
        save_permanently_preferences_count_val(en, AUT.list_count_Val[en]);
        save_permanently_preferences_count_base(en, AUT.list_count_base_Val[en]);
        GUI.flag_update_labels_3_2 = true;
        GUI.flag_flash_count_dec = true;
      }
      OPT.cur = OPT.cur + 1;
    }
    if (OPT.cur < OPT.piece_count) {
      float target = OPT.piece_len[OPT.cur];
      int en = OPT.piece_entry[OPT.cur];
      if (en < 0 || en >= LIST_MAX) en = 0;
      int rem_list = AUT.list_count_Val[en];
      if (rem_list < 1) rem_list = 1;
      int run_q = opt_current_run_qty();
      if (run_q < 1) run_q = 1;
      if (run_q > rem_list) run_q = rem_list;
      AUT.selected_count_val = en;
      AUT.temp_list_count_Val = run_q;
      STP.Man_Sp_mm = target;
      {
        char b[16];
        float r1 = roundf(target * 10.0f) / 10.0f;
        if (fabsf(r1 - roundf(r1)) >= 0.05f) snprintf(b, sizeof(b), "%.1f", (double)r1);
        else snprintf(b, sizeof(b), "%d", (int)lroundf(r1));
        if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)10) == pdTRUE) {
          GUI.inputBuffer_sp = b;
          xSemaphoreGive(lable1_1_Semaphore);
        } else {
          GUI.inputBuffer_sp = b;
        }
      }
      AUT.in_position_flag = false;
      STP.in_position_flag = false;
      GUI.update_lable1_1 = true;
      GUI.update_lable1_3 = true;
      GUI.edit_mode_lable1_1 = true;
      OPT.waiting_move = true;
    }
    OPT.flag_refresh = true;
  } else if (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX &&
             AUT.temp_list_count_Val > 0) {
    int sel = AUT.selected_count_val;
    if (record_physical_cut && OPT.cut_log_n < OPT_CUT_LOG_MAX) {
      OPT.cut_log_mm[OPT.cut_log_n] = AUT.list_sp_Val_p[sel];
      OPT.cut_log_bar[OPT.cut_log_n] = -1;
      OPT.cut_log_n++;
    }
    if (AUT.list_count_base_Val[sel] < AUT.temp_list_count_Val)
      AUT.list_count_base_Val[sel] = AUT.temp_list_count_Val;
    AUT.temp_list_count_Val--;
    AUT.list_count_Val[sel] = AUT.temp_list_count_Val;
    GUI.inputBuffer_L_3_2_x[sel] = String(AUT.temp_list_count_Val);
    save_permanently_preferences_count_val(sel, AUT.temp_list_count_Val);
    save_permanently_preferences_count_base(sel, AUT.list_count_base_Val[sel]);
    GUI.update_lable1_3 = true;
    GUI.flag_update_labels_3_2 = true;
    GUI.flag_flash_count_dec = true;
    GUI.flag_update_panel3_2_x_clr = true;

    if (AUT.temp_list_count_Val == 0) {
      AUT.list_count_Val[AUT.selected_count_val] = 0;
      GUI.inputBuffer_L_3_2_x[AUT.selected_count_val] = "0";

      int next = -1;
      for (int r = AUT.selected_count_val + 1; r < GUI.visible_raws && r < LIST_MAX; r++) {
        if (AUT.list_count_Val[r] > 0) { next = r; break; }
      }
      if (next != -1) {
        AUT.selected_count_val = next;
        AUT.temp_list_count_Val = AUT.list_count_Val[next];
        STP.Man_Sp_mm = AUT.list_sp_Val_p[next];
        if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)10) == pdTRUE) {
          GUI.inputBuffer_sp = GUI.inputBuffer_L_3_1_x[next];
          xSemaphoreGive(lable1_1_Semaphore);
        }
        uint8_t next_ang = AUT.list_angle_Val[next];
        if (next_ang == 90) {
          pinmap_out_write(IOS.lamp_pin, LOW);
          if (objects.btn2_3_6 && lv_obj_is_valid(objects.btn2_3_6)) {
            lv_obj_add_state(objects.btn2_3_6, LV_STATE_CHECKED);
          }
          if (!cut_angle_90_confirmed) cut_angle_90_prompt_req = true;
        } else {
          pinmap_out_write(IOS.lamp_pin, HIGH);
          if (objects.btn2_3_6 && lv_obj_is_valid(objects.btn2_3_6)) {
            lv_obj_clear_state(objects.btn2_3_6, LV_STATE_CHECKED);
          }
          cut_angle_90_confirmed = false;
        }
        AUT.in_position_flag = false;
        GUI.update_lable1_1 = true;
        GUI.flag_update_panel3_2_x_clr = true;
        GUI.buzzer_cmd_short = true;
      } else {
        AUT.selected_count_val = -1;
        AUT.temp_list_count_Val = 0;
        OPT.active = false;
        OPT.waiting_move = false;
        GUI.update_lable1_3 = true;
        GUI.flag_update_panel3_2_x_clr = true;
        GUI.flag_cast_error1_3 = true;
      }
    }
  }
}

bool commit_main_sp(bool do_move) {
  clear_buffer_sp_flag = true;
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  const float active_offset = blade_err_active_offset_mm();
  const int configured_initial_mm = (int)ceilf((float)STP.initial_Position_p / ppmm);
  const int configured_final_mm = (int)floorf((float)STP.final_position_p / ppmm);
  const int motor_safe_initial_mm =
      (int)ceilf((float)STP.initial_Position_p / ppmm - active_offset);
  const int motor_safe_final_mm =
      (int)floorf((float)STP.final_position_p / ppmm - active_offset);
  int initial_mm = max(configured_initial_mm, motor_safe_initial_mm);
  int final_mm = min(configured_final_mm, motor_safe_final_mm);
  bool clamped = false;
  // بافر خالی را صفر فرض نکن — جلوی «خارج از محدوده» و پریدن به ابتدای خط
  if (GUI.inputBuffer_sp.length() == 0) {
    if (do_move) {
      GUI.buzzer_cmd_long = true;
      ui_toast("ابتدا عدد مقصد را وارد كنيد", "Enter a destination first", 2500);
    }
    return false;
  }
  // سقف ورودی هدف: 9999.9
  if (GUI.inputBuffer_sp.toFloat() > 9999.9f) {
    GUI.inputBuffer_sp = "9999.9";
    GUI.update_lable1_1 = true;
    clamped = true;
  }
  if (blade_err_motor_target_p(GUI.inputBuffer_sp.toFloat()) < STP.initial_Position_p) {
    GUI.inputBuffer_sp = String(initial_mm);
    GUI.update_lable1_1 = true;
    range_toast(initial_mm, final_mm);
    clamped = true;
  } else if (blade_err_motor_target_p(GUI.inputBuffer_sp.toFloat()) > STP.final_position_p) {
    GUI.inputBuffer_sp = String(final_mm);
    GUI.update_lable1_1 = true;
    range_toast(initial_mm, final_mm);
    clamped = true;
  }
  STP.Man_Sp_mm = GUI.inputBuffer_sp.toFloat();
  // خارج محدوده: فقط اصلاح عدد — حرکت فقط با Enter بعدی
  if (clamped && do_move) {
    GUI.buzzer_cmd_long = true;
    return false;
  }
  if (do_move) {
    // روی صفحه اصلی: هرگز به‌خاطر referenced حرکت را نکش.
    // این فلگ گاهی بعد از مودال/رفرنس/ورود ناقص false می‌ماند و ماشین می‌میرد.
    if (!STP.referenced) STP.referenced = true;
    // قفل مودال سنسور اگر UI باز نباشد ولی فلگ گیر کرده باشد
    if (sensor_alert_motion_lock && !GUI.lock_screen) {
      sensor_alert_motion_lock = false;
      STP.mvm_interupt = false;
    }
    if (IOS.stop_button_state) {
      GUI.buzzer_cmd_long = true;
      ui_toast("استاپ فعال است — آزاد کنید", "E-STOP is active — release it", 3500);
      return false;
    }
    if (OPT.active && STP.motorIs_moving) {
      GUI.buzzer_cmd_long = true;
      return false;
    }
    if (STP.flag_is_cutting) {  // protect if cutting dont move the axis
      GUI.buzzer_cmd_long = true;
      return false;
    }
    // رسیدن به مقصد، برش فیزیکی نیست؛ Enter/START پنل فقط یک قطعه از صف مصرف می‌کند.
    if ((OPT.active || AUT.selected_count_val >= 0) && main_is_at_destination()) {
      main_consume_active_queue_piece(false);
      GUI.buzzer_cmd_short = true;
      return true;
    }
    // همان تابع گیت: فقط فرمان حرکت — جاگ را خاموش نکن اگر اپراتور در NumLock است
    if (!GUI.NumLock) {
      STP.jogEnabled = false;
    }
    // جهت لقی با هدف واقعی اصلاح‌شده سنجیده می‌شود.
    if (STP.currentPosition_p > blade_err_motor_target_p(STP.Man_Sp_mm))
      force_left_backlash_settle = true;
    STP.moveStepperCMD = true;
    STP.in_position_flag = false;
    AUT.in_position_flag = false;
    if (OPT.active) OPT.waiting_move = false;
    // شمارش سایز Enter (نه جاگ) — debounce با confirm_cut در note_cut_event
    if (!keyboardJogActive && !STP.jogEnabled) {
      note_cut_event(cut_stats_detect_src());
    }
  }
  return true;
}

//---------- انتخاب ردیف لیست (مشترک ده پنل) ----------
static bool list_row_is_depleted(int idx) {
  if (idx < 0 || idx >= LIST_MAX) return false;
  if (AUT.list_edit_mode_sp[idx] || AUT.list_edit_mode_count[idx]) return false;
  const int base = AUT.list_count_base_Val[idx];
  const int cur = AUT.list_count_Val[idx];
  return AUT.list_sp_Val_p[idx] > 0.05f && base > 0 && cur <= 0 && cur < base;
}

void list_purge_confirm_show(int idx);  // ui_extras.h
void queue_switch_confirm_show(int new_idx);
void select_list_entry_apply(int idx);  // اعمال بدون تأیید

static void select_list_entry(int idx) {
  if (idx < 0 || idx >= LIST_MAX) return;
  if (AUT.list_sp_Val_p[idx] <= 0.05f) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    ui_toast("متراژ این ردیف صفر است و قابل انتخاب نیست",
             "Row length is zero; cannot select", 3500);
    return;
  }
  if (AUT.list_count_Val[idx] <= 0 && !list_row_is_depleted(idx)) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    ui_toast("تعداد این ردیف صفر است و قابل برش نیست",
             "Row quantity is zero; cannot cut", 3500);
    return;
  }
  if (list_row_is_depleted(idx)) {
    list_purge_confirm_show(idx);
    return;
  }
  if (STP.motorIs_moving || STP.moveStepperCMD) {
    GUI.buzzer_cmd_long = true;
    ui_toast("موتور در حال حرکت است. الان نمیتوانید وارد لیست شوید",
             "Motor is moving. Cannot open the list now", 3500);
    return;
  }
  const bool opt_busy = OPT.active && OPT.cur < OPT.piece_count;
  const bool row_busy = !OPT.active && AUT.selected_count_val >= 0 &&
                        AUT.selected_count_val < LIST_MAX &&
                        AUT.temp_list_count_Val > 0 &&
                        AUT.selected_count_val != idx;
  if (opt_busy || row_busy) {
    queue_switch_confirm_show(idx);
    return;
  }
  select_list_entry_apply(idx);
}

void select_list_entry_apply(int idx) {
  if (idx < 0 || idx >= LIST_MAX) return;
  if (AUT.list_sp_Val_p[idx] <= 0.05f) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    ui_toast("متراژ این ردیف صفر است و قابل انتخاب نیست",
             "Row length is zero; cannot select", 3500);
    return;
  }
  if (AUT.list_count_Val[idx] <= 0 && !list_row_is_depleted(idx)) {
    if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
    ui_toast("تعداد این ردیف صفر است و قابل برش نیست",
             "Row quantity is zero; cannot cut", 3500);
    return;
  }
  if (OPT.active) {
    OPT.active = false;
    OPT.waiting_move = false;
    OPT.flag_refresh = false;
  }
  numpad_close();
  STP.Man_Sp_mm = AUT.list_sp_Val_p[idx];
  GUI.inputBuffer_sp = GUI.inputBuffer_L_3_1_x[idx];
  AUT.selected_count_val = idx;
  AUT.temp_list_count_Val = AUT.list_count_Val[idx];
  AUT.in_position_flag = false;
  GUI.NumLock = false;
  jog_forward_resume_dest_valid = false;
  GUI.update_lable1_1 = true;
  GUI.update_lable1_3 = true;
  GUI.edit_mode_lable1_1 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  request_goto_screen(1);
  opt_main_bar_lbl_sync();
}

static void btn3_1_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);            //--> Get the event code.
  lv_obj_t *btn3_1 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    request_goto_screen(1);
  }
}
static void btn3_2_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn3_2 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_STEP_UP);
    // ادیت بازِ سلول قبل از تغییر ساختار لیست بسته شود تا اسنپ‌شاتش برنگردد
    cancel_screen3_pending_edit();
    // PEND ممکن است با GUI هم‌خوان نباشد (فقط موقع ورود به تنظیمات sync می‌شود)
    int next = GUI.visible_raws + 1;
    if (next > LIST_MAX) {
      char fa[80], en[64];
      snprintf(fa, sizeof(fa), "ظرفیت لیست کامل شد؛ بیشتر از %d ردیف نمی‌شود", LIST_MAX);
      snprintf(en, sizeof(en), "List limit reached: %d rows", LIST_MAX);
      ui_toast(fa, en, 3500);
      return;
    }
    int idx = next - 1;
    list_zero_row(idx);
    GUI.pending_zero_bits[idx >> 3] |= (uint8_t)(1u << (idx & 7));
    GUI.visible_raws = next;
    PEND.visible_raws = next;
    GUI.flag_save_visible_raws = true;
    list_show_row(idx);                 // اگر از صفحه بیرون زد، خودکار برو صفحهٔ بعد
  }
}
static void btn3_3_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn3_3 = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_STEP_DOWN);
    if (GUI.visible_raws <= 0) return;
    // ادیت بازِ سلول قبل از حذف ردیف بسته شود تا اسنپ‌شاتش برنگردد
    cancel_screen3_pending_edit();
    int idx = GUI.visible_raws - 1;
    list_zero_row(idx);
    GUI.pending_zero_bits[idx >> 3] |= (uint8_t)(1u << (idx & 7));
    GUI.visible_raws--;
    PEND.visible_raws = GUI.visible_raws;
    GUI.flag_save_visible_raws = true;
    // اگر صفحهٔ فعلی خالی شد، برگرد صفحهٔ قبل
    if (GUI.visible_raws > 0) list_show_row(GUI.visible_raws - 1);
    else GUI.list_page = 0;
    GUI.flag_refresh_visible = true;
    GUI.flag_update_labels_3_1 = true;
    GUI.flag_update_labels_3_2 = true;
    GUI.flag_update_page = true;
    GUI.flag_save_visible_raws = true;
  }
}
static bool f3_q_output_touch_blocked(lv_obj_t *btn) {
  if (pinmap_pending_has_changes()) {
    if (lv_obj_has_state(btn, LV_STATE_CHECKED)) lv_obj_clear_state(btn, LV_STATE_CHECKED);
    GUI.buzzer_cmd_long = true;
    ui_toast("تغییر پایه ها هنوز ذخیره نشده؛ اول ذخیره را بزنید",
             "Pin changes are not saved; press Save first", 3200);
    return true;
  }
  if (!(f3_stepper_test_busy || f3_stepper_test_cmd ||
        STP.flag_is_cutting || STP.motorIs_moving || STP.jogEnabled || STP.moveStepperCMD)) {
    return false;
  }
  // LVGL قبلاً CHECKED را عوض کرده — برگردان تا خروجی تاگل نشود
  if (lv_obj_has_state(btn, LV_STATE_CHECKED)) lv_obj_clear_state(btn, LV_STATE_CHECKED);
  else lv_obj_add_state(btn, LV_STATE_CHECKED);
  GUI.buzzer_cmd_long = true;
  return true;
}

static bool f3_q_output_disconnected(lv_obj_t *btn, int role,
                                     const char *fa, const char *en) {
  if (pinmap_pending_has_changes()) return false;
  if (pinmap_out_get_source(role) != PINMAP_OUT_NC) return false;
  if (btn) lv_obj_clear_state(btn, LV_STATE_CHECKED);
  GUI.buzzer_cmd_long = true;
  ui_toast(fa, en, 3200);
  return true;
}

static void btn2_3_1_click_event_handler(lv_event_t *e) {
  lv_obj_t *btn2_3_1 = (lv_obj_t *)lv_event_get_target(e);
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (f3_q_output_disconnected(btn2_3_1, 5,
        "اسپری به هیچ پایه ای وصل نیست",
        "Spray output has no assigned pin")) return;
  if (f3_q_output_touch_blocked(btn2_3_1)) return;
  if (lv_obj_has_state(btn2_3_1, LV_STATE_CHECKED)) {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.spray_pin, LOW);
  } else {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.spray_pin, HIGH);
  }
}
static void btn2_3_2_click_event_handler(lv_event_t *e) {
  lv_obj_t *btn2_3_2 = (lv_obj_t *)lv_event_get_target(e);
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (f3_q_output_disconnected(btn2_3_2, 3,
        "جک نگهدارنده به هیچ پایه ای وصل نیست",
        "Holder jack has no assigned pin")) return;
  if (f3_q_output_touch_blocked(btn2_3_2)) return;
  if (lv_obj_has_state(btn2_3_2, LV_STATE_CHECKED)) {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.u_clip_pin, LOW);
  } else {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.u_clip_pin, HIGH);
  }
}
static void btn2_3_3_click_event_handler(lv_event_t *e) {
  lv_obj_t *btn2_3_3 = (lv_obj_t *)lv_event_get_target(e);
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (f3_q_output_disconnected(btn2_3_3, 6,
        "موتور اره به هیچ پایه ای وصل نیست",
        "Saw motor has no assigned pin")) return;
  if (f3_q_output_touch_blocked(btn2_3_3)) return;
  if (lv_obj_has_state(btn2_3_3, LV_STATE_CHECKED)) {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.saw_pin, LOW);
  } else {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.saw_pin, HIGH);
  }
}
static void btn2_3_4_click_event_handler(lv_event_t *e) {
  lv_obj_t *btn2_3_4 = (lv_obj_t *)lv_event_get_target(e);
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (f3_q_output_disconnected(btn2_3_4, 4,
        "جک محافظ به هیچ پایه ای وصل نیست",
        "Guard jack has no assigned pin")) return;
  if (f3_q_output_touch_blocked(btn2_3_4)) return;
  if (lv_obj_has_state(btn2_3_4, LV_STATE_CHECKED)) {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.f_clip_pin, LOW);
  } else {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.f_clip_pin, HIGH);
  }
}
static void btn2_3_5_click_event_handler(lv_event_t *e) {
  lv_obj_t *btn2_3_5 = (lv_obj_t *)lv_event_get_target(e);
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (f3_q_output_disconnected(btn2_3_5, 8,
        "جک تیغه به هیچ پایه ای وصل نیست",
        "Saw lift has no assigned pin")) return;
  if (f3_q_output_touch_blocked(btn2_3_5)) return;
  if (lv_obj_has_state(btn2_3_5, LV_STATE_CHECKED)) {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.sawl_ift_pin, LOW);
  } else {
    GUI.buzzer_cmd_short = true;
    pinmap_out_write(IOS.sawl_ift_pin, HIGH);
  }
}
static uint8_t f3_blade_angle_mode = 0; // 0: 45°, 1: -45°, 2: 90°

static void f3_update_blade_angle_ui() {
  const char *ang_str = "45°";
  if (f3_blade_angle_mode == 0) {
    ang_str = "45°";
    pinmap_out_write(IOS.sawl_ift_pin, LOW);    // 45° -> Q8 خاموش
    pinmap_out_write(IOS.q9_pin, LOW);
  } else if (f3_blade_angle_mode == 1) {
    ang_str = "-45°";
    pinmap_out_write(IOS.sawl_ift_pin, HIGH);   // -45° -> Q8 روشن
    pinmap_out_write(IOS.q9_pin, LOW);
  } else {
    ang_str = "90°";
    pinmap_out_write(IOS.sawl_ift_pin, LOW);
    pinmap_out_write(IOS.q9_pin, HIGH);         // 90° -> Q9 روشن
  }
  if (objects.lablef2_17 && lv_obj_is_valid(objects.lablef2_17)) lv_label_set_text(objects.lablef2_17, "زاویه تیغه");
  if (objects.lablee2_17 && lv_obj_is_valid(objects.lablee2_17)) lv_label_set_text(objects.lablee2_17, "Blade Angle");
  extern lv_obj_t *f3_angle_val_lbl;
  if (f3_angle_val_lbl && lv_obj_is_valid(f3_angle_val_lbl)) {
    lv_label_set_text(f3_angle_val_lbl, ang_str);
  }
}

static void btn2_3_6_click_event_handler(lv_event_t *e) {
  (void)e;
  if (GUI.lock_screen || IOS.stop_button_state) {
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    return;
  }
  f3_blade_angle_mode = (f3_blade_angle_mode + 1) % 3;
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
  f3_update_blade_angle_ui();
}
// =================== انتخاب‌گر زبان — بروزرسانی ظاهر dropdown ===================
static void lang_selector_update_visual(int sel) {
  if (!objects.roller2_1) return;
  lv_obj_set_user_data(objects.roller2_1, (void *)(intptr_t)sel);

  // --- دکمه compact (child 0 از wrapper) ---
  lv_obj_t *compact_btn = lv_obj_get_child(objects.roller2_1, 0);
  if (compact_btn) {
    lv_obj_t *flag_ir_c = lv_obj_get_child(compact_btn, 0);  // child 0 = پرچم ایران
    lv_obj_t *flag_uk_c = lv_obj_get_child(compact_btn, 1);  // child 1 = پرچم UK
    lv_obj_t *lbl_cur   = lv_obj_get_child(compact_btn, 2);  // child 2 = برچسب زبان
    if (flag_ir_c) { if (sel == 0) lv_obj_clear_flag(flag_ir_c, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(flag_ir_c, LV_OBJ_FLAG_HIDDEN); }
    if (flag_uk_c) { if (sel == 1) lv_obj_clear_flag(flag_uk_c, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(flag_uk_c, LV_OBJ_FLAG_HIDDEN); }
    if (lbl_cur) {
      if (sel == 0) {
        lv_label_set_text(lbl_cur, "فارسی");
        lv_obj_set_style_text_font(lbl_cur, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl_cur, lv_color_hex(0x1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
      } else {
        lv_label_set_text(lbl_cur, "English");
        lv_obj_set_style_text_font(lbl_cur, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl_cur, lv_color_hex(0x1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
  }

  // --- پنل dropdown (child 1 از wrapper) ---
  lv_obj_t *panel = lv_obj_get_child(objects.roller2_1, 1);
  if (panel) {
    lv_obj_t *row_fa = lv_obj_get_child(panel, 0);  // ردیف فارسی
    lv_obj_t *row_en = lv_obj_get_child(panel, 2);  // ردیف انگلیسی
    if (row_fa) {
      lv_obj_set_style_bg_color(row_fa, (sel == 0) ? lv_color_hex(0xeef8fd) : lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_t *radio_fa = lv_obj_get_child(row_fa, 2);
      if (radio_fa) {
        lv_obj_set_style_bg_opa(radio_fa, (sel == 0) ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_t *dot_fa = lv_obj_get_child(radio_fa, 0);
        if (dot_fa) { if (sel == 0) lv_obj_clear_flag(dot_fa, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(dot_fa, LV_OBJ_FLAG_HIDDEN); }
      }
    }
    if (row_en) {
      lv_obj_set_style_bg_color(row_en, (sel == 1) ? lv_color_hex(0xeef8fd) : lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_t *radio_en = lv_obj_get_child(row_en, 2);
      if (radio_en) {
        lv_obj_set_style_bg_opa(radio_en, (sel == 1) ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_t *dot_en = lv_obj_get_child(radio_en, 0);
        if (dot_en) { if (sel == 1) lv_obj_clear_flag(dot_en, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(dot_en, LV_OBJ_FLAG_HIDDEN); }
      }
    }
    // بستن پنل بعد از انتخاب
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
  }
  f1_lang_btn_sync();  // پرچم روی دکمهٔ هدر F1
}
// ================================================================================

static void apply_system_language_ui(int selected);

static void roller2_1_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *roller2_1 = (lv_obj_t *)lv_event_get_target(e);
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (code == LV_EVENT_VALUE_CHANGED) {
    GUI.buzzer_cmd_short = true;
    int selected = (int)(intptr_t)lv_event_get_param(e);  // 0=FA 1=EN
    lv_obj_set_user_data(roller2_1, (void *)(intptr_t)selected);
    lang_selector_update_visual(selected);
    if (selected == 0) { PEND.system_language = 1; GUI.system_language = 1; }
    else { PEND.system_language = 2; GUI.system_language = 2; }
    GUI.pending_lang_selected = selected;
    GUI.flag_apply_language = true;  // لیبل‌ها در Task2 — جلوگیری از هنگ لمس
    settings_mark_dirty();
  }
}

static void apply_system_language_ui(int selected) {
{                                                  // hide all lables
      lv_obj_add_flag(objects.lablef0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_29, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_29, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef_print) lv_obj_add_flag(objects.lablef_print, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee_print) lv_obj_add_flag(objects.lablee_print, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef_dir_chap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_dir_chap, LV_OBJ_FLAG_HIDDEN);
    }
    if (selected == 0) {
      PEND.system_language = 1;
      GUI.system_language = 1;
      lv_obj_clear_flag(objects.lablef0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_6, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_1) lv_obj_add_flag(objects.lablef2_1, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_2) lv_obj_add_flag(objects.lablef2_2, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_3) lv_obj_add_flag(objects.lablef2_3, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_4) lv_obj_add_flag(objects.lablef2_4, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_5) lv_obj_add_flag(objects.lablef2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_29, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
      // F1: ذخیره فقط آیکون — لیبل متنی نشان داده نشود
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef_print) lv_obj_clear_flag(objects.lablef_print, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef_dir_chap, LV_OBJ_FLAG_HIDDEN);
    } else if (selected == 1) {
      PEND.system_language = 2;
      GUI.system_language = 2;
      lv_obj_clear_flag(objects.lablee0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_6, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_1) lv_obj_add_flag(objects.lablee2_1, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_2) lv_obj_add_flag(objects.lablee2_2, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_3) lv_obj_add_flag(objects.lablee2_3, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_4) lv_obj_add_flag(objects.lablee2_4, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_5) lv_obj_add_flag(objects.lablee2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_29, LV_OBJ_FLAG_HIDDEN);
      // لیست برش: همان lablef3_* — متن EN/FA با list_reflow_compact در Task2
      lv_obj_clear_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_2, LV_OBJ_FLAG_HIDDEN);
      // F1: ذخیره فقط آیکون
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee_print) lv_obj_clear_flag(objects.lablee_print, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee_dir_chap, LV_OBJ_FLAG_HIDDEN);
    }
    
  lv_obj_invalidate(lv_scr_act());
  settings_hide_eez_clutter();  // عنوان/چاپ/تصاویر قدیمی تنظیمات را دوباره روشن نکن
}

static void roller2_2_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);               // Get the event code
  lv_obj_t *roller2_2 = (lv_obj_t *)lv_event_get_target(e);  // roller that triggered the event
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (code == LV_EVENT_VALUE_CHANGED) {
    GUI.buzzer_cmd_short = true;
    int selected = lv_roller_get_selected(roller2_2);  // Use the roller object from the event
    if (selected < 0) selected = 0;
    if (selected > 4) selected = 4;  // سقف ۵ ثانیه
    PEND.cut_prcs_timer = (selected + 1) * 1000;
    settings_mark_dirty();
    Serial.println(selected);
    Serial.println(PEND.cut_prcs_timer);
  }
}
static void sw2_1_event_handler(lv_event_t *e) {
  lv_obj_t *sw2_1 = (lv_obj_t *)lv_event_get_target(e);  // get the target object of the event
  if (GUI.lock_screen) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  GUI.buzzer_cmd_short = true;
  if (lv_obj_has_state(sw2_1, LV_STATE_CHECKED)) {
    PEND.system_base_dir = 1;
    Serial.println(PEND.system_base_dir);
  } else {
    PEND.system_base_dir = -1;
    Serial.println(PEND.system_base_dir);
  }
  f4_dir_buttons_sync();
  settings_mark_dirty();
}
static void btn_dir_rast_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    GUI.buzzer_cmd_short = true;
    PEND.system_base_dir = 1;
    lv_obj_add_state(objects.sw2_1, LV_STATE_CHECKED);
    f4_dir_buttons_sync();
    settings_mark_dirty();
  }
}
static void btn_dir_chap_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED) {
    if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
    GUI.buzzer_cmd_short = true;
    PEND.system_base_dir = -1;
    lv_obj_clear_state(objects.sw2_1, LV_STATE_CHECKED);
    f4_dir_buttons_sync();
    settings_mark_dirty();
  }
}
static void tabview2_1_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (objects.tabview2_1 && lv_obj_is_valid(objects.tabview2_1)) {
      int next_tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
      if (!oobe_guard_tab_change(next_tab)) {
        int cur_tab = (g_oobe_stage == OOBE_STAGE_F1_ESSENTIALS) ? 0 : 1;
        lv_tabview_set_active(objects.tabview2_1, cur_tab, LV_ANIM_OFF);
        return;
      }
      ui_active_settings_tab = next_tab;
    }
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    pinmap_out_write(IOS.lamp_pin, LOW);
    settings_on_tab_changed();  // برگشت تغییرات تب قبلی + snap تب جدید
    GUI.flag_reinit_settings = true;  // عنوان هدر + sync در Task2
  }
}
static void panel3_2_1_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 0);
}
static void panel3_2_2_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 1);
}
static void panel3_2_3_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 2);
}
static void panel3_2_4_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 3);
}
static void panel3_2_5_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 4);
}
static void panel3_2_6_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 5);
}
static void panel3_2_7_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 6);
}
static void panel3_2_8_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 7);
}
static void panel3_2_9_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 8);
}
static void panel3_2_10_click_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  select_list_entry(page_base() + 9);
}
static void btn3_1_1_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_1 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 0);
    numpad_open(2);
    list_show_row(page_base() + 0);
  }
}
static void btn3_1_2_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_2 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 1);
    numpad_open(2);
    list_show_row(page_base() + 1);
  }
}
static void btn3_1_3_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_3 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 2);
    numpad_open(2);
    list_show_row(page_base() + 2);
  }
}
static void btn3_1_4_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_4 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 3);
    numpad_open(2);
    list_show_row(page_base() + 3);
  }
}
static void btn3_1_5_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_5 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 4);
    numpad_open(2);
    list_show_row(page_base() + 4);
  }
}
static void btn3_1_6_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_6 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 5);
    numpad_open(2);
    list_show_row(page_base() + 5);
  }
}
static void btn3_1_7_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_7 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 6);
    numpad_open(2);
    list_show_row(page_base() + 6);
  }
}
static void btn3_1_8_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_8 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 7);
    numpad_open(2);
    list_show_row(page_base() + 7);
  }
}
static void btn3_1_9_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);              //--> Get the event code.
  lv_obj_t *btn3_1_9 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 8);
    numpad_open(2);
    list_show_row(page_base() + 8);
  }
}
static void btn3_1_10_click_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);               //--> Get the event code.
  lv_obj_t *btn3_1_10 = (lv_obj_t *)lv_event_get_target(e);  //--> Button that generated the event.
  if (code == LV_EVENT_RELEASED) {
    if (ui_help_take_suppress()) return;
    if (list_ui_was_dragged()) return;
    if (GUI.lock_screen) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.buzzer_cmd_short = true;
    begin_screen3_sp_edit(page_base() + 9);
    numpad_open(2);
    list_show_row(page_base() + 9);
  }
}

static void screen3_sp_label_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  int rel = (int)(intptr_t)lv_event_get_user_data(e);
  int idx = page_base() + rel;
  if (idx < 0 || idx >= GUI.visible_raws || idx >= LIST_MAX) { GUI.buzzer_cmd_long = true; return; }
  if (list_row_is_depleted(idx)) {
    list_purge_confirm_show(idx);
    return;
  }
  GUI.buzzer_cmd_short = true;
  begin_screen3_sp_edit(idx);
  numpad_open(2);   // اول ctx تا قوانین ۲ردیفه فعال شود
  list_ensure_row_visible(idx);  // فقط اگر ردیف دیده نمی‌شود جابه‌جا کن
}

static void screen3_count_label_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (ui_help_take_suppress()) return;
  if (list_ui_was_dragged()) return;
  if (GUI.lock_screen) { GUI.buzzer_cmd_long = true; return; }
  int rel = (int)(intptr_t)lv_event_get_user_data(e);
  int idx = page_base() + rel;
  if (idx < 0 || idx >= GUI.visible_raws || idx >= LIST_MAX) { GUI.buzzer_cmd_long = true; return; }
  if (list_row_is_depleted(idx)) {
    list_purge_confirm_show(idx);
    return;
  }
  GUI.buzzer_cmd_short = true;
  begin_screen3_count_edit(idx);
  numpad_open(2);
  list_ensure_row_visible(idx);
}

//---------------register widgets clb-----------------------------------------------------
void register_clb() {
  if (!objects.main_2) create_screen_main_2();
  if (!objects.main_3) create_screen_main_3();
  lv_obj_add_event_cb(objects.btn0_1, btn0_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn0_2, btn0_2_click_event_handler, LV_EVENT_ALL, NULL);
  ref_homing_apply_green_caption();  // راهنما زیر «موقعیت یابی»
  lv_obj_add_event_cb(objects.btn0_3, btn0_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn0_4, btn0_4_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn0_5, btn0_5_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_1, main_icon_press_spin, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(objects.btn1_1, main_icon_press_spin, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_1, main_icon_press_spin, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_add_event_cb(objects.btn1_1, btn1_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_2, main_icon_press_spin, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(objects.btn1_2, main_icon_press_spin, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_2, main_icon_press_spin, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_add_event_cb(objects.btn1_2, btn1_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_3, btn1_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_4, btn1_4_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_5, btn1_5_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_6, btn1_6_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_7, btn1_7_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.lable1_2, lable1_2_click_event_handler, LV_EVENT_CLICKED, NULL);
  main_bind_destination_scrub_handlers();
  lv_obj_add_event_cb(objects.btn2_1, btn2_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn_save, btn_save_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.slider2_1, slider2_1_val_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(objects.slider2_2, slider2_2_val_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(objects.btn2_3, btn2_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  if (objects.img2_1) lv_obj_add_event_cb(objects.img2_1, img2_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  if (objects.img2_2) lv_obj_add_event_cb(objects.img2_2, img2_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  // B5 fix: ثبت تکراری btn2_3 حذف شد (بالاتر یک‌بار ثبت شده — دوبار اجراشدن هندلر)
  lv_obj_add_event_cb(objects.btn2_4, btn2_4_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_5, btn2_5_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_6, btn2_6_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_7, btn2_7_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_8, btn2_8_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_flag(objects.lable2_2, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(objects.lable2_2, lable2_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_flag(objects.lable2_3, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(objects.lable2_3, lable2_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_3_1, btn2_3_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_3_2, btn2_3_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_3_3, btn2_3_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_3_4, btn2_3_4_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_3_5, btn2_3_5_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn2_3_6, btn2_3_6_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1, btn3_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_2, btn3_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_3, btn3_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.roller2_1, roller2_1_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(objects.roller2_2, roller2_2_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(objects.sw2_1, sw2_1_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(objects.btn_dir_rast, btn_dir_rast_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn_dir_chap, btn_dir_chap_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.tabview2_1, tabview2_1_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_1, panel3_2_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_2, panel3_2_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_3, panel3_2_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_4, panel3_2_4_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_5, panel3_2_5_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_6, panel3_2_6_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_7, panel3_2_7_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_8, panel3_2_8_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_9, panel3_2_9_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.panel3_2_10, panel3_2_10_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_t *list_row_tiles[10] = {
    objects.panel3_2_1, objects.panel3_2_2, objects.panel3_2_3, objects.panel3_2_4, objects.panel3_2_5,
    objects.panel3_2_6, objects.panel3_2_7, objects.panel3_2_8, objects.panel3_2_9, objects.panel3_2_10
  };
  for (int i = 0; i < 10; i++) {
    if (!list_row_tiles[i]) continue;
    lv_obj_set_style_bg_color(list_row_tiles[i], lv_color_hex(0x218C45), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale(list_row_tiles[i], 246, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  lv_obj_add_event_cb(objects.btn3_1_1, btn3_1_1_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_2, btn3_1_2_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_3, btn3_1_3_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_4, btn3_1_4_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_5, btn3_1_5_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_6, btn3_1_6_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_7, btn3_1_7_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_8, btn3_1_8_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_9, btn3_1_9_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn3_1_10, btn3_1_10_click_event_handler, LV_EVENT_RELEASED, NULL);
  lv_obj_t *sp_labels[10] = {
    objects.lable3_1_1, objects.lable3_1_2, objects.lable3_1_3, objects.lable3_1_4, objects.lable3_1_5,
    objects.lable3_1_6, objects.lable3_1_7, objects.lable3_1_8, objects.lable3_1_9, objects.lable3_1_10
  };
  lv_obj_t *count_labels[10] = {
    objects.lable3_2_1, objects.lable3_2_2, objects.lable3_2_3, objects.lable3_2_4, objects.lable3_2_5,
    objects.lable3_2_6, objects.lable3_2_7, objects.lable3_2_8, objects.lable3_2_9, objects.lable3_2_10
  };
  for (int i = 0; i < 10; i++) {
    if (sp_labels[i]) {
      lv_obj_add_flag(sp_labels[i], LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(sp_labels[i], LV_OBJ_FLAG_PRESS_LOCK);
      lv_obj_add_event_cb(sp_labels[i], screen3_sp_label_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
    }
    if (count_labels[i]) {
      lv_obj_add_flag(count_labels[i], LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(count_labels[i], LV_OBJ_FLAG_PRESS_LOCK);
      lv_obj_add_event_cb(count_labels[i], screen3_count_label_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
    }
  }
}
//----------------touchRead Function-------------------------------
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  if (!touch_ready) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }
  static uint32_t last_read_ms = 0;
  static lv_indev_data_t last_data = {};
  static bool panel_nav_touch_suppressed = false;
  const uint32_t now = millis();
  const uint32_t period_ms = (last_data.state == LV_INDEV_STATE_PRESSED) ? 8U : 12U;
  if ((uint32_t)(now - last_read_ms) < period_ms) {
    *data = last_data;
    return;
  }
  last_read_ms = now;
  TOUCHINFO ti;
  memset(&ti, 0, sizeof(ti));
  bool got_samples = false;
  if (touchWireMutex) {
    if (xSemaphoreTakeRecursive(touchWireMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
      got_samples = bbct.getSamples(&ti);
      xSemaphoreGiveRecursive(touchWireMutex);
    }
  } else {
    got_samples = bbct.getSamples(&ti);
  }

  if (!got_samples || ti.count <= 0) {
    data->state = LV_INDEV_STATE_RELEASED;
    panel_nav_touch_suppressed = false;
  } else {
    if (!panel_nav_touch_suppressed && panel_five_nav_is_active()) {
      panel_five_nav_exit();
      panel_nav_touch_suppressed = true;
    }
    data->state = panel_nav_touch_suppressed ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
    data->point.x = ti.x[0];
    data->point.y = ti.y[0];
    if (data->point.x < 0) data->point.x = 0;
    if (data->point.x >= SCREEN_WIDTH) data->point.x = SCREEN_WIDTH - 1;
    if (data->point.y < 0) data->point.y = 0;
    if (data->point.y >= SCREEN_HEIGHT) data->point.y = SCREEN_HEIGHT - 1;
  }
  last_data = *data;
}

static void my_mouse_read(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;
  data->point.x = clamp_screen_x((int)usb_mouse_x);
  data->point.y = clamp_screen_y((int)usb_mouse_y);
  data->state = usb_mouse_left_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  if (usb_mouse_cursor) {
    if (usb_mouse_seen) lv_obj_clear_flag(usb_mouse_cursor, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(usb_mouse_cursor, LV_OBJ_FLAG_HIDDEN);
  }
}

static void keyboard_nav_register_objects();

static void my_keyboard_read(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;
  if (keyboard_nav_key_pending) {
    if (keyboard_nav_registered_scr != GUI.scr_id) keyboard_nav_register_objects();
    data->key = keyboard_nav_key;
    data->state = keyboard_nav_key_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if (keyboard_nav_key_down) keyboard_nav_key_down = false;
    else keyboard_nav_key_pending = false;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void keyboard_nav_add(lv_obj_t *obj) {
  if (!keyboard_nav_group || !obj) return;
  lv_group_add_obj(keyboard_nav_group, obj);
}

static void keyboard_nav_register_objects() {
  if (!keyboard_nav_group) return;
  lv_group_remove_all_objs(keyboard_nav_group);
  keyboard_nav_registered_scr = GUI.scr_id;

  if (GUI.scr_id == 0) {
    keyboard_nav_add(objects.btn0_1);
    keyboard_nav_add(objects.btn0_2);
    keyboard_nav_add(objects.btn0_3);
    keyboard_nav_add(objects.btn0_4);
    keyboard_nav_add(objects.btn0_5);
  } else if (GUI.scr_id == 1) {
    keyboard_nav_add(objects.btn1_1);
    keyboard_nav_add(objects.btn1_2);
    keyboard_nav_add(objects.btn1_3);
    keyboard_nav_add(objects.btn1_4);
    keyboard_nav_add(objects.btn1_5);
    keyboard_nav_add(objects.btn1_6);
    keyboard_nav_add(objects.btn1_7);
    keyboard_nav_add(objects.lable1_1);
    keyboard_nav_add(objects.lable1_2);
  } else if (GUI.scr_id == 2) {
    keyboard_nav_add(objects.btn2_1);
    keyboard_nav_add(objects.btn2_3);
    keyboard_nav_add(objects.btn2_4);
    keyboard_nav_add(objects.btn2_5);
    keyboard_nav_add(objects.btn2_6);
    keyboard_nav_add(objects.btn2_7);
    keyboard_nav_add(objects.btn2_8);
    keyboard_nav_add(objects.slider2_1);
    keyboard_nav_add(objects.slider2_2);
    keyboard_nav_add(objects.roller2_1);
    keyboard_nav_add(objects.roller2_2);
    keyboard_nav_add(objects.sw2_1);
    keyboard_nav_add(objects.btn2_3_1);
    keyboard_nav_add(objects.btn2_3_2);
    keyboard_nav_add(objects.btn2_3_3);
    keyboard_nav_add(objects.btn2_3_4);
    keyboard_nav_add(objects.btn2_3_5);
    keyboard_nav_add(objects.btn2_3_6);
  } else if (GUI.scr_id == 3) {
    keyboard_nav_add(objects.btn3_1);
    keyboard_nav_add(objects.lable3_1_1);
    keyboard_nav_add(objects.lable3_2_1);
    keyboard_nav_add(objects.lable3_1_2);
    keyboard_nav_add(objects.lable3_2_2);
    keyboard_nav_add(objects.lable3_1_3);
    keyboard_nav_add(objects.lable3_2_3);
    keyboard_nav_add(objects.lable3_1_4);
    keyboard_nav_add(objects.lable3_2_4);
    keyboard_nav_add(objects.lable3_1_5);
    keyboard_nav_add(objects.lable3_2_5);
    keyboard_nav_add(objects.lable3_1_6);
    keyboard_nav_add(objects.lable3_2_6);
    keyboard_nav_add(objects.lable3_1_7);
    keyboard_nav_add(objects.lable3_2_7);
    keyboard_nav_add(objects.lable3_1_8);
    keyboard_nav_add(objects.lable3_2_8);
    keyboard_nav_add(objects.lable3_1_9);
    keyboard_nav_add(objects.lable3_2_9);
    keyboard_nav_add(objects.lable3_1_10);
    keyboard_nav_add(objects.lable3_2_10);
  }
}
static bool initTouchController() {
  touch_ready = false;
  int touchInitResult = bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT, 400000, &touchWire);
  touchWire.setTimeOut(25);
  int touchType = bbct.sensorType();
  Serial.printf("Touch init result=%d type=%d\n", touchInitResult, touchType);
  if (touchInitResult == CT_SUCCESS && touchType != CT_TYPE_UNKNOWN) {
    bbct.setOrientation(TOUCH_ORIENTATION, SCREEN_HEIGHT, SCREEN_WIDTH);
    touch_ready = true;
  }
  return touch_ready;
}
//----------------display Flush Function-------------------------------
static void display_dma_log_state(const char *tag) {
  // Keep diagnostics on validated software state. Raw SPI register reads are
  // not portable across Arduino/IDF host numbering and must never risk boot.
  ets_printf("[LCDDMA] %s core=%d ready=%d pending=%d count=%lu max_us=%lu\n",
             tag, xPortGetCoreID(), display_dma_ready ? 1 : 0,
             display_dma_pending ? 1 : 0,
             (unsigned long)display_dma_transfer_count,
             (unsigned long)display_dma_max_transfer_us);
}

static void display_dma_schedule_enable() {
  if (display_dma_attempted || display_dma_enable_at_ms != 0) return;
  // The splash removal and first main-screen paint stay on proven blocking SPI.
  display_dma_enable_at_ms = millis() + 500UL;
}

static bool display_dma_init_on_render_core() {
  if (display_dma_attempted) return display_dma_ready;
  display_dma_attempted = true;

  if (display_dma_recovery_magic == DISPLAY_DMA_RECOVERY_MAGIC &&
      display_dma_recovery_magic_inv == ~DISPLAY_DMA_RECOVERY_MAGIC) {
    // یک بوت با SPI مسدود؛ مارکر DMA را پاک کن تا بوت بعدی دوباره DMA را امتحان کند
    display_dma_recovery_magic = 0;
    display_dma_recovery_magic_inv = 0;
    // صفحهٔ هدف را پاک نکن — splash/prcs_splash مسئول اعمال و clear هستند
    if (dma_ui_resume_scr < 0) {
      int s = -1;
      if (ui_resume_boot_get_latched(&s)) {
        dma_ui_resume_scr = s;
        STP.goref_cmd = false;
      }
    }
    ets_printf("[LCDDMA] recovery boot: blocking fallback resume_scr=%d\n", dma_ui_resume_scr);
    return false;
  }

  // LVGL swaps RGB565 in my_disp_flush; DMA sends that wire-order buffer as-is.
  tft.setSwapBytes(false);
  display_dma_ready = tft.DMA_Enabled;
  // TFT_eSPI's documented path keeps manual CS low from RAMWR through the DMA
  // payload. Hardware CS would split those into separate panel transactions.
  if (!display_dma_ready) display_dma_ready = tft.initDMA(false);
  ets_printf("[LCDDMA] init=%s core=%d\n", display_dma_ready ? "ready" : "fallback", xPortGetCoreID());
  display_dma_log_state(display_dma_ready ? "ready" : "init-failed");
  return display_dma_ready;
}

static void display_dma_try_enable_scheduled() {
  if (display_dma_attempted || display_dma_enable_at_ms == 0) return;
  if ((int32_t)(millis() - display_dma_enable_at_ms) < 0) return;
  display_dma_enable_at_ms = 0;
  display_dma_init_on_render_core();
}

static inline uint32_t display_dma_wait_counter() {
  return display_dma_wait_total_us;
}

static void display_dma_finish_pending() {
  if (!display_dma_pending) return;

  const uint32_t wait_start = micros();
  while (tft.dmaBusy()) {
    const uint32_t transfer_us = (uint32_t)(micros() - display_dma_transfer_started_us);
    if (transfer_us > DISPLAY_DMA_TIMEOUT_US) {
      display_dma_log_state("timeout");
      ets_printf("[LCDDMA] timeout transfer_us=%lu count=%lu; safe restart\n",
                 (unsigned long)transfer_us, (unsigned long)display_dma_transfer_count);
      // صفحهٔ فعلی را دقیق ذخیره کن تا بعد از ریست همان‌جا باز شود
      ui_resume_mark_current(GUI.scr_id);
      display_dma_recovery_magic = DISPLAY_DMA_RECOVERY_MAGIC;
      display_dma_recovery_magic_inv = ~DISPLAY_DMA_RECOVERY_MAGIC;
      vTaskDelay(pdMS_TO_TICKS(20));
      esp_restart();
      while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    taskYIELD();
  }

  const uint32_t wait_us = (uint32_t)(micros() - wait_start);
  const uint32_t transfer_us = (uint32_t)(micros() - display_dma_transfer_started_us);
  // dmaBusy() consumed the completed IDF transaction, so endWrite cannot enter
  // TFT_eSPI's unbounded dmaWait(). It only releases manual CS and the SPI lock.
  if (display_dma_transaction_open) {
    tft.endWrite();
    display_dma_transaction_open = false;
  }
  display_dma_wait_total_us += wait_us;
  if (transfer_us > display_dma_max_transfer_us) display_dma_max_transfer_us = transfer_us;
  display_dma_pending = false;
}

static void my_disp_flush_wait(lv_display_t *disp) {
  if (display_dma_pending) {
    display_dma_finish_pending();
    lv_display_flush_ready(disp);
  }
}

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  uint32_t px_count = w * h;
  uint16_t *buf16 = (uint16_t *)color_p;
  bool last_flush = lv_display_flush_is_last(disp);
  shot_on_flush(area, buf16, last_flush);
  perf_overlay_note_flush(last_flush, px_count);
  // LVGL 9 ignores the legacy LV_COLOR_16_SWAP option. Swap exactly once here
  // so blocking SPI and DMA both receive RGB565 in panel byte order.
  lv_draw_sw_rgb565_swap(buf16, px_count);

  if (display_dma_ready && px_count >= DISPLAY_DMA_MIN_PIXELS) {
    if (display_dma_pending) {
      display_dma_finish_pending();
      lv_display_flush_ready(disp);
    }
    // One continuous panel transaction: CASET/PASET/RAMWR and pixel payload
    // share manual CS. This is the supported TFT_eSPI ESP32-S3 DMA contract.
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    display_dma_transaction_open = true;
    display_dma_transfer_started_us = micros();
    bool dma_queued = false;
    if (px_count <= 0x4000UL) {
      dma_queued = tft.pushPixelsDMAQueued(buf16, px_count);
    } else {
      // TFT_eSPI splits the leading payload into bounded blocking chunks and
      // queues the final <=0x4000-pixel block. This permits a taller LVGL
      // render stripe without violating the ESP32-S3 DMA transaction limit.
      tft.pushPixelsDMA(buf16, px_count);
      dma_queued = true;
    }
    if (dma_queued) {
      display_dma_transfer_count++;
      display_dma_pending = true;
      return;  // LVGL waits through my_disp_flush_wait before reusing this buffer.
    }

    // A full queue or invalid DMA state must never block the UI. Close the
    // command transaction, disable runtime DMA, and resend this frame below.
    tft.endWrite();
    display_dma_transaction_open = false;
    display_dma_ready = false;
    display_dma_log_state("queue-fallback");
  }

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(buf16, px_count, false);
  tft.endWrite();
  lv_display_flush_ready(disp);
}
//----------------Setup Gui----------------------------------------------
void Setup_Gui() {
  static lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!disp) {
    Serial.println("Failed to create LVGL display");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
  }
  // Keep the existing 30,720-byte budget, but prefer two 16-line DMA buffers.
  // On list/overlay redraws LVGL can render the next stripe while the previous
  // stripe is on SPI DMA; a single 32-line buffer makes every stripe wait.
  const int dual_fast_line_try[] = { 32, 24, 16, 12, 8 };
  const int dual_line_try[] = { 8, 6 };
  const int single_line_try[] = { 32, 24, 16, 12, 8 };
  size_t buf_px_cnt = 0;
  buf1 = buf2 = nullptr;
  int selected_lines = 0;
  for (int lines : dual_fast_line_try) {
    buf_px_cnt = SCREEN_WIDTH * (size_t)lines;
    buf1 = (lv_color_t *)heap_caps_malloc(buf_px_cnt * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = (lv_color_t *)heap_caps_malloc(buf_px_cnt * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf1 && buf2) {
      selected_lines = lines;
      break;
    }
    if (buf1) { heap_caps_free(buf1); buf1 = nullptr; }
    if (buf2) { heap_caps_free(buf2); buf2 = nullptr; }
  }
  for (int lines : dual_line_try) {
    if (buf1) break;
    buf_px_cnt = SCREEN_WIDTH * (size_t)lines;
    buf1 = (lv_color_t *)heap_caps_malloc(buf_px_cnt * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = (lv_color_t *)heap_caps_malloc(buf_px_cnt * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf1 && buf2) {
      selected_lines = lines;
      break;
    }
    if (buf1) { heap_caps_free(buf1); buf1 = nullptr; }
    if (buf2) { heap_caps_free(buf2); buf2 = nullptr; }
  }
  if (!buf1) {
    if (buf1) { heap_caps_free(buf1); buf1 = nullptr; }
    if (buf2) { heap_caps_free(buf2); buf2 = nullptr; }
    for (int lines : single_line_try) {
      buf_px_cnt = SCREEN_WIDTH * (size_t)lines;
      buf1 = (lv_color_t *)heap_caps_malloc(buf_px_cnt * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
      if (buf1) {
        selected_lines = lines;
        break;
      }
    }
  }
  if (!buf1) {
    Serial.println("FAIL TO ALLOC BUFF");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
  }
  Serial.printf("LVGL %s buffer lines=%d bytes=%u\n",
                buf2 ? "double" : "single", selected_lines,
                (unsigned)(buf_px_cnt * sizeof(lv_color_t) * (buf2 ? 2U : 1U)));
  lv_display_render_mode_t mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
  lv_display_set_buffers(disp, buf1, buf2, buf_px_cnt * sizeof(lv_color_t), mode);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_flush_wait_cb(disp, my_disp_flush_wait);
  static lv_indev_t *indev_touch = lv_indev_create();
  lv_indev_set_type(indev_touch, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev_touch, my_touchpad_read);
  lv_indev_set_long_press_time(indev_touch, 2000);
  lv_indev_set_scroll_limit(indev_touch, 12);  // حرکت → اسکرول/درگ؛ long-press کنسل
  static lv_indev_t *indev_mouse = lv_indev_create();
  lv_indev_set_type(indev_mouse, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev_mouse, my_mouse_read);
  lv_indev_set_long_press_time(indev_mouse, 2000);
  lv_indev_set_scroll_limit(indev_mouse, 12);
  usb_mouse_cursor = lv_obj_create(lv_layer_top());
  lv_obj_set_size(usb_mouse_cursor, 13, 13);
  lv_obj_set_style_radius(usb_mouse_cursor, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(usb_mouse_cursor, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_opa(usb_mouse_cursor, LV_OPA_80, 0);
  lv_obj_set_style_border_width(usb_mouse_cursor, 2, 0);
  lv_obj_set_style_border_color(usb_mouse_cursor, lv_color_hex(0x0a84ff), 0);
  lv_obj_set_style_shadow_width(usb_mouse_cursor, 0, 0);
  lv_obj_clear_flag(usb_mouse_cursor, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(usb_mouse_cursor, LV_OBJ_FLAG_HIDDEN);
  lv_indev_set_cursor(indev_mouse, usb_mouse_cursor);

  keyboard_nav_group = lv_group_create();
  lv_group_set_wrap(keyboard_nav_group, true);
  static lv_indev_t *indev_keyboard = lv_indev_create();
  lv_indev_set_type(indev_keyboard, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev_keyboard, my_keyboard_read);
  lv_indev_set_group(indev_keyboard, keyboard_nav_group);
  ui_init();
  delay(10);
}
//------------------------initial widget states upon startup---------------------------//
void init_widgets() {
  load_pending_from_current();
  lv_slider_set_value(objects.slider2_1, (int)map(STP.global_speed, 8, 100, 1000, 0), LV_ANIM_OFF);
  lv_slider_set_value(objects.slider2_2, (int)map(auto_ramp_ms, 0, 5000, 0, 1000), LV_ANIM_OFF);
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(objects.slider2_1) / 10);
    lv_label_set_text(objects.lbl_spd2_1, buf);
    snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(objects.slider2_2) / 10);
    lv_label_set_text(objects.lbl_spd2_2, buf);
  }
  //----------
  lang_selector_update_visual((GUI.system_language == 1) ? 0 : 1);
  lv_roller_set_selected(objects.roller2_2, (int)constrain(IOS.cut_prcs_timer / 1000 - 1, 0, 4), LV_ANIM_OFF);

  if (STP.system_base_dir == 1) lv_obj_add_state(objects.sw2_1, LV_STATE_CHECKED);
  else lv_obj_clear_state(objects.sw2_1, LV_STATE_CHECKED);
  if (objects.normally_closed) lv_obj_add_flag(objects.normally_closed, LV_OBJ_FLAG_HIDDEN);
  if (objects.normally_open) lv_obj_add_flag(objects.normally_open, LV_OBJ_FLAG_HIDDEN);
  //----------
  {  // init lables in scr 2 — بالا اول خط(L2)، پایین آخر خط(max) — با اعشار از پالس
    float ppmm = (float)STP.drv_ppr / (float)STP.Axis_Pitch;
    if (ppmm < 0.01f) ppmm = 1.0f;
    float start_mm = (float)STP.initial_Position_p / ppmm;
    char buff_L2[16];
    if (fabsf(start_mm - roundf(start_mm)) > 0.05f)
      snprintf(buff_L2, sizeof(buff_L2), "%.1f", (double)start_mm);
    else
      snprintf(buff_L2, sizeof(buff_L2), "%d", (int)lroundf(start_mm));
    GUI.inputBuffer_initial_L2 = buff_L2;
    GUI.L2_GUI = (int)lroundf(start_mm);
    lv_label_set_text(objects.lable2_2, buff_L2);
    GUI.inputBuffer_max_L = STP.final_position_mm;
    lv_label_set_text(objects.lable2_3, GUI.inputBuffer_max_L.c_str());
    lv_label_set_text(objects.lable2_5, GUI.inputBuffer_max_L.c_str());
  }
  {  //hide alarm panels
    lv_obj_add_flag(objects.panel2_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.panel1_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.panel1_3) lv_obj_add_flag(objects.panel1_3, LV_OBJ_FLAG_HIDDEN);
  }
  {  // مقصد بوت = موتور فعلی — نه طول اولیه/۶۰ (قبل از اولین paint)
    main_restore_destination_to_current_pos();
  }
  {  //mute-unmute img — EEZ قدیمی؛ فقط پنهان (بازر روی چیپ F1)
    if (objects.img2_1) lv_obj_add_flag(objects.img2_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.img2_2) lv_obj_add_flag(objects.img2_2, LV_OBJ_FLAG_HIDDEN);
  }
  {    //language
    {  // hide all lables
      lv_obj_add_flag(objects.lablee0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee1_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee2_29, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef_print) lv_obj_add_flag(objects.lablef_print, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee_print) lv_obj_add_flag(objects.lablee_print, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef_dir_chap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_dir_chap, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef1_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef2_29, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
    }
    if (GUI.system_language == 1) {
      lv_obj_clear_flag(objects.lablef0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef1_6, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_1) lv_obj_add_flag(objects.lablef2_1, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_2) lv_obj_add_flag(objects.lablef2_2, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_3) lv_obj_add_flag(objects.lablef2_3, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_4) lv_obj_add_flag(objects.lablef2_4, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef2_5) lv_obj_add_flag(objects.lablef2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef2_29, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
      // F1: ذخیره فقط آیکون
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablef_print) lv_obj_clear_flag(objects.lablef_print, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef_dir_chap, LV_OBJ_FLAG_HIDDEN);
    } else if (GUI.system_language == 2) {
      lv_obj_clear_flag(objects.lablee0_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee0_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee0_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee0_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee1_6, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_1) lv_obj_add_flag(objects.lablee2_1, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_2) lv_obj_add_flag(objects.lablee2_2, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_3) lv_obj_add_flag(objects.lablee2_3, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_4) lv_obj_add_flag(objects.lablee2_4, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee2_5) lv_obj_add_flag(objects.lablee2_5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_6, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_7, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_8, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_9, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_10, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_11, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_12, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_13, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_14, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_15, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_16, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_17, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_18, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_19, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_20, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_21, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_22, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_23, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_24, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_25, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_26, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_27, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_28, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee2_29, LV_OBJ_FLAG_HIDDEN);
      // لیست برش: همان lablef3_* در جای فارسی — نه لیبل‌های EEZ انگلیسی
      lv_obj_clear_flag(objects.lablef3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablef3_2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_1, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee3_2, LV_OBJ_FLAG_HIDDEN);
      // F1: ذخیره فقط آیکون
      lv_obj_add_flag(objects.lablef_save, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(objects.lablee_save, LV_OBJ_FLAG_HIDDEN);
      if (objects.lablee_print) lv_obj_clear_flag(objects.lablee_print, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee_dir_rast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(objects.lablee_dir_chap, LV_OBJ_FLAG_HIDDEN);
    }
  }
  // راهنمای سبز موقعیت‌یابی را با زبان تازه هم‌تراز کن (اگر کارت خطا باز نباشد)
  if (!ref_error_ui_active && objects.btn0_1 &&
      !lv_obj_has_flag(objects.btn0_1, LV_OBJ_FLAG_HIDDEN)) {
    ref_homing_apply_green_caption();
  }
  keyboard_nav_register_objects();
  settings_hide_eez_clutter();
}
