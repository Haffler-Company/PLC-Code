#pragma once
#include "pitch_cal.h"
// F3 pin map / F4 admin / F5 developer chrome + device lock overlay
#include "admin_svc.h"
#include "version.h"
#include "fw_changelog.h"
#include "f5_report_ui.h"
#include <Preferences.h>
#include <esp_task_wdt.h>

// پنجرۀ WDT رندر (۱ث) را از نو باز می‌کند — کارِ سنگینِ قانونیِ یک‌تکه پنیک نکند؛
// هنگِ واقعی همچنان همان ۱ ثانیه ریست می‌شود (سیاست مالک).
static inline void f4_wdt_feed() { (void)esp_task_wdt_reset(); }

// یک خط مدرک برای دیاگ ریست/هنگ مدیر: heap، کمینۀ تاریخی، استخر LVGL و استک Task2
static void f4_diag_log(const char *tag) {
  lv_mem_monitor_t mon;
  lv_mem_monitor(&mon);
  ets_printf("[F4] %s heap=%u min=%u lvfree=%u lvbig=%u stk=%u\n",
             tag,
             (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
             (unsigned)mon.free_size, (unsigned)mon.free_biggest_size,
             h_Task2 ? (unsigned)uxTaskGetStackHighWaterMark(h_Task2) : 0u);
}

void save_factory_snapshot();
bool factory_snapshot_exists();
void factory_reset();
void save_admin_svc_state();
void save_admin_pin();
void save_user_cuts();
extern uint32_t total_cuts;
extern uint32_t user_cuts;
extern bool warranty_active;
extern char warranty_date[24];
void save_warranty_state();
void ui_toast(const char *fa, const char *en, uint32_t ms);
void update_flow_start_from_ui();
void settings_mark_dirty();
void f4_session_lock();  // خروج از تنظیمات → دوباره رمز
bool usb_keyboard_present();

static void f4_gate_usb_drain();
void f4_kb_hide();
static void f4_kb_show_for(lv_obj_t *ta);
static void f4_clear_focus_style();
void f4_admin_reflow();
static void dev_lock_usb_drain();
static void f4_chain_close();
void f5_dev_reflow();

// ===================== Modular Subheaders =====================
#include "admin/dev_lock.h"
#include "admin/tab_f3_pinmap.h"
#include "admin/tab_f5_dev.h"
#include "admin/tab_f4_admin.h"
