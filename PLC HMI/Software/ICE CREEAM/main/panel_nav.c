#include "panel_nav.h"
#include "day_lock_ui.h"
#include "day_lock_core.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "screens.h"
#include "ui/ui.h"
#include "actions.h"
#include "buzzer.h"
#include "storage.h"
#include "project_config.h"
#include "fonts.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "wifi.h"
#include "ui_lang.h"
#include "warranty.h"
#include "ui_refresh.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "panel_nav";

#define NAV_MAX_ITEMS       64
#define KEY_QUEUE_CAPACITY  16
#define INACTIVITY_TIMEOUT_MS 10000U
#define TOAST_DURATION_MS   3500
/* B10: از 50ms به 100ms — نمایش progress bar همچنان روان است،
 * اما تعداد lv_obj_set_width (که dirty mark می‌کند) نصف می‌شود */
#define TOAST_TICK_INTERVAL_MS 100
#define TOAST_WIDTH         456
#define TOAST_MIN_HEIGHT    48
#define TOAST_MAX_HEIGHT    220

static PanelNavManager g_nav = {0};

/* صف کلیدها بین Core 1 (actions_task) و Core 0 (LVGL) */
static volatile uint16_t s_key_queue[KEY_QUEUE_CAPACITY];
static volatile uint8_t s_q_head = 0;
static volatile uint8_t s_q_tail = 0;

/* ویجت‌ها و متغیرهای بنر توست با قابلیت Touch-Freeze */
static lv_obj_t *s_toast_card = NULL;
static lv_obj_t *s_toast_bar = NULL;
static lv_timer_t *s_toast_timer = NULL;
static bool s_toast_paused = false;
static int s_toast_remaining_ms = 0;
static int s_toast_total_ms = TOAST_DURATION_MS;

/* نشانگر متحرک لایه بالا به سبک Apple HIG */
static lv_obj_t *s_nav_marker = NULL;

/* تایمر پریودیک ناوبری روی LVGL */
static lv_timer_t *s_nav_timer = NULL;

/* -------------------------------------------------------------------------- */
/*                               TOAST SYSTEM                                 */
/* -------------------------------------------------------------------------- */

static void panel_nav_dismiss_toast(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
    if (s_toast_timer) {
        lv_timer_delete(s_toast_timer);
        s_toast_timer = NULL;
    }
    if (s_toast_card && lv_obj_is_valid(s_toast_card)) {
        lv_obj_delete(s_toast_card);
    }
    s_toast_card = NULL;
    s_toast_bar = NULL;
    s_toast_paused = false;
    s_toast_remaining_ms = 0;
    lvgl_unlock();
}

static void toast_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_toast_paused) return;

    s_toast_remaining_ms -= TOAST_TICK_INTERVAL_MS;
    if (s_toast_remaining_ms <= 0) {
        panel_nav_dismiss_toast();
        return;
    }

    if (s_toast_bar && lv_obj_is_valid(s_toast_bar)) {
        int32_t w = (int32_t)((TOAST_WIDTH * s_toast_remaining_ms) / (s_toast_total_ms > 0 ? s_toast_total_ms : TOAST_DURATION_MS));
        if (w < 0) w = 0;
        lv_obj_set_width(s_toast_bar, w);
    }
}

static void toast_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_toast_paused = true; /* Touch-and-Hold Freeze */
    } else if (code == LV_EVENT_RELEASED) {
        s_toast_paused = false; /* ادامه شمارش پس از رها شدن انگشت */
    } else if (code == LV_EVENT_CLICKED) {
        panel_nav_dismiss_toast(); /* لمس کوتاه = بستن فوری */
    }
}

void panel_nav_show_toast(const char *msg, lv_color_t accent_color)
{
    if (!msg || !msg[0]) return;

    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;

    panel_nav_dismiss_toast();

    /* ۱. کارت شناور مات مشکی اپل با ابعاد منعطف و بدون بریدگی متن */
    s_toast_card = lv_obj_create(lv_layer_top());
    if (!s_toast_card) {
        lvgl_unlock();
        return;
    }
    lv_obj_set_width(s_toast_card, TOAST_WIDTH);
    lv_obj_set_height(s_toast_card, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(s_toast_card, TOAST_MIN_HEIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_max_height(s_toast_card, TOAST_MAX_HEIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_toast_card, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(s_toast_card, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_toast_card, 245, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_toast_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_toast_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_toast_card, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_toast_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(s_toast_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(s_toast_card, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(s_toast_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_toast_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_toast_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_toast_card, toast_event_cb, LV_EVENT_ALL, NULL);

    ui_lang_t lang = ui_lang_get();

    /* ۲. نشانگر کوچک رنگی در گوشه */
    lv_obj_t *dot = lv_obj_create(s_toast_card);
    if (dot) {
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, accent_color, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        if (lang == UI_LANG_FA) {
            lv_obj_align(dot, LV_ALIGN_TOP_RIGHT, 0, 4);
        } else {
            lv_obj_align(dot, LV_ALIGN_TOP_LEFT, 0, 4);
        }
    }

    /* ۳. متن پیام با قابلیت Wrap چندخطی و بدون بریدگی */
    lv_obj_t *lbl = lv_label_create(s_toast_card);
    if (lbl) {
        lv_label_set_text(lbl, msg);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, TOAST_WIDTH - 44);
        if (lang == UI_LANG_FA) {
            lv_obj_set_style_text_font(lbl, &ui_font_yekan_16, 0);
            lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_RTL, 0);
            lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        } else {
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_LTR, 0);
            lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        }
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_line_space(lbl, 4, 0);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    }

    /* ۴. نوار باریک ۳ پیکسلی تخلیه شونده در لبه پایین */
    s_toast_bar = lv_obj_create(s_toast_card);
    if (s_toast_bar) {
        lv_obj_set_size(s_toast_bar, TOAST_WIDTH, 3);
        lv_obj_align(s_toast_bar, LV_ALIGN_BOTTOM_LEFT, -12, 12);
        lv_obj_set_style_bg_color(s_toast_bar, accent_color, 0);
        lv_obj_set_style_bg_opa(s_toast_bar, 255, 0);
        lv_obj_set_style_border_width(s_toast_bar, 0, 0);
        lv_obj_set_style_radius(s_toast_bar, 2, 0);
        lv_obj_set_style_pad_all(s_toast_bar, 0, 0);
        lv_obj_clear_flag(s_toast_bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    int total_duration = TOAST_DURATION_MS;
    size_t msg_len = strlen(msg);
    if (msg_len > 30) {
        total_duration += (int)(msg_len - 30) * 50;
        if (total_duration > 7500) total_duration = 7500;
    }
    s_toast_total_ms = total_duration;
    s_toast_remaining_ms = total_duration;
    s_toast_paused = false;
    s_toast_timer = lv_timer_create(toast_timer_cb, TOAST_TICK_INTERVAL_MS, NULL);
    lvgl_unlock();
}

void panel_nav_toast_info(const char *msg)
{
    panel_nav_show_toast(msg, lv_color_hex(0x0EA5E9)); /* Neon Cyan */
}

void panel_nav_toast_success(const char *msg)
{
    panel_nav_show_toast(msg, lv_color_hex(0x10B981)); /* Green */
}

void panel_nav_toast_warning(const char *msg)
{
    panel_nav_show_toast(msg, lv_color_hex(0xF59E0B)); /* Amber */
}

void panel_nav_toast_error(const char *msg)
{
    panel_nav_show_toast(msg, lv_color_hex(0xEF4444)); /* Red */
}

/* -------------------------------------------------------------------------- */
/*                               QUEUE SYSTEM                                 */
/* -------------------------------------------------------------------------- */

void panel_nav_queue_key(uint8_t row, uint8_t key)
{
    uint8_t next_head = (uint8_t)((s_q_head + 1) % KEY_QUEUE_CAPACITY);
    if (next_head != s_q_tail) {
        s_key_queue[s_q_head] = (uint16_t)((row << 8) | key);
        s_q_head = next_head;
    }
}

static bool dequeue_key(uint8_t *row, uint8_t *key)
{
    if (s_q_head == s_q_tail) return false;
    uint16_t evt = s_key_queue[s_q_tail];
    s_q_tail = (uint8_t)((s_q_tail + 1) % KEY_QUEUE_CAPACITY);
    *row = (uint8_t)((evt >> 8) & 0xFF);
    *key = (uint8_t)(evt & 0xFF);
    return true;
}

/* -------------------------------------------------------------------------- */
/*                       MODAL & WIDGET FILTERING                             */
/* -------------------------------------------------------------------------- */

bool panel_nav_has_active_modal(void)
{
    if (screensaver_is_active()) return false;
    if (objects.wifi_kb_overlay && lv_obj_is_valid(objects.wifi_kb_overlay) && !lv_obj_has_flag(objects.wifi_kb_overlay, LV_OBJ_FLAG_HIDDEN)) {
        return true;
    }
    lv_obj_t *top = lv_layer_top();
    if (!top) return false;

    uint32_t cnt = lv_obj_get_child_count(top);
    for (int32_t i = (int32_t)cnt - 1; i >= 0; --i) {
        lv_obj_t *child = lv_obj_get_child(top, (uint32_t)i);
        if (!child || !lv_obj_is_valid(child)) continue;
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
        if (child == s_toast_card || child == s_nav_marker) continue;
        return true;
    }
    return false;
}

static lv_obj_t *get_active_modal_root(void)
{
    if (screensaver_is_active()) return NULL;
    if (objects.wifi_kb_overlay && lv_obj_is_valid(objects.wifi_kb_overlay) && !lv_obj_has_flag(objects.wifi_kb_overlay, LV_OBJ_FLAG_HIDDEN)) {
        return objects.wifi_kb_overlay;
    }
    lv_obj_t *top = lv_layer_top();
    if (!top) return NULL;

    uint32_t cnt = lv_obj_get_child_count(top);
    for (int32_t i = (int32_t)cnt - 1; i >= 0; --i) {
        lv_obj_t *child = lv_obj_get_child(top, (uint32_t)i);
        if (!child || !lv_obj_is_valid(child)) continue;
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
        if (child == s_toast_card || child == s_nav_marker) continue;
        return child;
    }
    return NULL;
}

bool panel_nav_dismiss_active_modal(void)
{
    bool dismissed = false;

    if (day_lock_ui_is_visible()) {
        if (!day_lock_is_locked()) {
            day_lock_ui_hide();
            dismissed = true;
        }
    }
    if (!day_lock_is_locked()) {
        day_lock_ui_hide_5timers_dialog();
    }

    if (objects.wifi_kb_overlay && lv_obj_is_valid(objects.wifi_kb_overlay) && !lv_obj_has_flag(objects.wifi_kb_overlay, LV_OBJ_FLAG_HIDDEN)) {
        wifi_key_act_hide();
        dismissed = true;
    }

    if (panel_nav_has_active_modal()) {
        hide_language_selector_modal();
        hide_modal_help_dialog();
        hide_lease_unlock_dialog();
        hide_manager_pin_dialog();
        hide_manager_dashboard_dialog();
        hide_factory_reset_confirm_dialog();
        hide_warranty_expired_dialog();
        hide_warranty_no_net_dialog();
        hide_fault_history_dialog();
        hide_interactive_fault_dialog();
        hide_counter_details_dialog();
        hide_ota_pinmap_dialog();
        hide_boot_wifi_prompt_dialog();
        wifi_status_msgbox_hide();
        extern void hide_ota_update_congrats_dialog(void);
        hide_ota_update_congrats_dialog();
        dismissed = true;
    }

    if (objects.sample_kb && lv_obj_is_valid(objects.sample_kb)) {
        if (!lv_obj_has_flag(objects.sample_kb, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(objects.sample_kb, LV_OBJ_FLAG_HIDDEN);
            dismissed = true;
        }
    }

    if (dismissed) {
        g_nav.saved_confirm_index = -1;
        g_nav.saved_screen_id = -1;
        g_nav.last_confirm_ms = 0;
    }

    return dismissed;
}

static bool is_in_inactive_tab(lv_obj_t *obj)
{
    if (!objects.obj6 || !lv_obj_is_valid(objects.obj6)) {
        return false;
    }

    uint32_t active_tab = lv_tabview_get_tab_active(objects.obj6);
    lv_obj_t *active_tab_obj = NULL;
    switch (active_tab) {
        case 0: active_tab_obj = objects.tab_f1_btn; break;
        case 1: active_tab_obj = objects.tab_f2_btn; break;
        case 2: active_tab_obj = objects.tab_f3_btn; break;
        case 3: active_tab_obj = objects.tab_f4_btn; break;
        case 4: active_tab_obj = objects.tab_f5_btn; break;
        case 5: active_tab_obj = objects.tab_f6_btn; break;
        default: break;
    }

    lv_obj_t *all_tabs[6] = {
        objects.tab_f1_btn, objects.tab_f2_btn, objects.tab_f3_btn,
        objects.tab_f4_btn, objects.tab_f5_btn, objects.tab_f6_btn
    };

    lv_obj_t *p = obj;
    while (p) {
        for (int i = 0; i < 6; i++) {
            if (all_tabs[i] && lv_obj_is_valid(all_tabs[i]) && p == all_tabs[i]) {
                return (p != active_tab_obj);
            }
        }
        p = lv_obj_get_parent(p);
    }
    return false;
}

static bool is_actionable(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;

    /* اگر شیء یا اجداد آن درون کارت توست باشند رد کن */
    if (s_toast_card && lv_obj_is_valid(s_toast_card)) {
        lv_obj_t *p = obj;
        while (p) {
            if (p == s_toast_card) return false;
            p = lv_obj_get_parent(p);
        }
    }

    /* قرنطینه تب‌های غیرفعال صفحه تنظیمات: جلوگیری از فوکوس روی ویجت‌های پنهان */
    if (is_in_inactive_tab(obj)) return false;

    /* بررسی پنهان بودن اجداد */
    lv_obj_t *immediate_parent = lv_obj_get_parent(obj);
    lv_obj_t *anc = immediate_parent;
    while (anc) {
        if (lv_obj_has_flag(anc, LV_OBJ_FLAG_HIDDEN)) return false;
        anc = lv_obj_get_parent(anc);
    }

    /* خط قرمز: ساعت، کپسول‌های دما، و بنر مانیتورینگ هرگز فوکوس نمی‌گیرند */
    lv_obj_t *chk = obj;
    while (chk) {
        if (chk == objects.clock_cont || chk == objects.time_lbl || chk == objects.date_lbl) return false;
        if (chk == objects.temp_hopper_box || chk == objects.temp_cyl_box) return false;
        if (chk == objects.live_monitor_card || chk == objects.live_monitor_status_badge) return false;
        if (objects.time_lbl && chk == lv_obj_get_parent(objects.time_lbl)) return false;
        chk = lv_obj_get_parent(chk);
    }

    /* خط قرمز: کانتینرهای تب‌ها، هدر بارها، و کادرهای ساختاری هرگز فوکوس نمی‌گیرند */
    if (obj == objects.tab_f1_btn || obj == objects.tab_f2_btn || obj == objects.tab_f3_btn ||
        obj == objects.tab_f4_btn || obj == objects.tab_f5_btn || obj == objects.tab_f6_btn) {
        return false;
    }
    if (obj == objects.obj5 || obj == objects.obj6 || obj == objects.obj7 || obj == objects.obj8 ||
        obj == objects.obj10 || obj == objects.obj12 || obj == objects.obj14) {
        return false;
    }

    /* حذف کلیه عناصر هدر بار تب‌ها (عنوان، پرچم زبان، آیکون تب) از چرخه ناوبری پارامترها */
    lv_obj_t *hdr_chk = obj;
    while (hdr_chk) {
        if (hdr_chk == objects.obj8 || hdr_chk == objects.obj10 || hdr_chk == objects.obj12 || hdr_chk == objects.obj14) {
            return false;
        }
        if (lv_obj_get_y(hdr_chk) <= 10 && lv_obj_get_height(hdr_chk) <= 34 && lv_obj_get_width(hdr_chk) >= 300) {
            return false;
        }
        hdr_chk = lv_obj_get_parent(hdr_chk);
    }

    /* خط قرمز: لیبل‌ها و تصاویر ایستا هرگز فوکوس نمی‌گیرند */
    if (lv_obj_check_type(obj, &lv_label_class)) return false;
    if (lv_obj_check_type(obj, &lv_image_class) && !lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) return false;

    /* استپرهای عددی: کارت والد دارای ۴ فرزند است (c0: minus, c1: val_box, c2: plus, c3: title).
     * دکمه منفی [c0] و دکمه مثبت [c2] هر دو کلیدهای مستقل هستند و فوکوس می‌گیرند.
     * کادر عدد [c1] و عنوان فارسی [c3] رد می‌شوند تا نشانگر منحصراً روی خود کلیدها بنشیند. */
    if (immediate_parent && lv_obj_get_child_count(immediate_parent) >= 3) {
        lv_obj_t *c0 = lv_obj_get_child(immediate_parent, 0);
        lv_obj_t *c2 = lv_obj_get_child(immediate_parent, 2);
        if (lv_obj_check_type(c0, &lv_button_class) && lv_obj_check_type(c2, &lv_button_class)) {
            if (obj != c0 && obj != c2) {
                return false;
            }
        }
    }

    /* حذف نوار دکمه‌های تب از چرخه ناوبری MODE تا فوکوس روی پارامترهای فعال متمرکز بماند */
    if (objects.obj6 && lv_obj_is_valid(objects.obj6)) {
        lv_obj_t *tb = lv_tabview_get_tab_bar(objects.obj6);
        if (obj == tb || immediate_parent == tb) return false;
    }

    /* کلاس‌های ویجت تعاملی مجاز */
    if (lv_obj_check_type(obj, &lv_button_class)) return true;
    if (lv_obj_check_type(obj, &lv_switch_class)) return true;
    if (lv_obj_check_type(obj, &lv_slider_class)) return true;
    if (lv_obj_check_type(obj, &lv_arc_class)) return true;
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
    if (lv_obj_check_type(obj, &lv_roller_class)) return true;
    if (lv_obj_check_type(obj, &lv_textarea_class)) return true;
    if (lv_obj_check_type(obj, &lv_keyboard_class)) return true;
    if (lv_obj_check_type(obj, &lv_buttonmatrix_class)) return true;

    /* کانتینرهای چیدمان و کادرهای نمایش عدد (مانند باکس شمارنده کل و جاری) هرگز نباید فوکوس بگیرند */
    return false;
}

static void collect_actionable_widgets(lv_obj_t *parent)
{
    if (!parent || !lv_obj_is_valid(parent)) return;

    uint32_t cnt = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < cnt && g_nav.item_count < NAV_MAX_ITEMS; ++i) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (!child || !lv_obj_is_valid(child)) continue;

        if (is_actionable(child)) {
            /* جلوگیری از درج تکراری */
            bool already = false;
            for (int k = 0; k < g_nav.item_count; ++k) {
                if (g_nav.items[k] == child) {
                    already = true;
                    break;
                }
            }
            if (!already) {
                g_nav.items[g_nav.item_count++] = child;
            }
        }
        collect_actionable_widgets(child);
    }
}

/* مرتب‌سازی ویجت‌ها بر اساس موقعیت بصری:
 * در صفحه تنظیمات با ساختار دو ستونی: ابتدا کل ستون چپ (از بالا به پایین)، سپس کل ستون راست (از بالا به پایین)
 * در سایر صفحات: بالا به پایین، چپ به راست */
static void sort_items_by_position(void)
{
    for (int i = 0; i < g_nav.item_count - 1; ++i) {
        for (int j = i + 1; j < g_nav.item_count; ++j) {
            lv_area_t a, b;
            lv_obj_get_coords(g_nav.items[i], &a);
            lv_obj_get_coords(g_nav.items[j], &b);

            bool swap = false;
            if (ui_get_screen_id() == SCREEN_ID_SETTINGS_PG) {
                /* تفکیک دو ستونی در تب‌های تنظیمات: ستون چپ (x < 180) قبل از ستون راست (x >= 180) */
                bool a_left = (a.x1 < 180);
                bool b_left = (b.x1 < 180);
                if (a_left != b_left) {
                    swap = b_left; /* اگر b در ستون چپ و a در ستون راست است، جابجا شوند */
                } else {
                    /* در همان ستون: اگر تفاوت سطری بیش از ۱۲ پیکسل است بر اساس Y، در غیر این صورت بر اساس X */
                    if (abs(b.y1 - a.y1) > 12) {
                        swap = (b.y1 < a.y1);
                    } else {
                        swap = (b.x1 < a.x1);
                    }
                }
            } else {
                /* در صفحه اصلی یا سایر صفحات: مقایسه ابتدا y، سپس x */
                if (abs(b.y1 - a.y1) > 12) {
                    swap = (b.y1 < a.y1);
                } else {
                    swap = (b.x1 < a.x1);
                }
            }

            if (swap) {
                lv_obj_t *tmp = g_nav.items[i];
                g_nav.items[i] = g_nav.items[j];
                g_nav.items[j] = tmp;
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/*                           FOCUS STYLES & ACTIONS                           */
/* -------------------------------------------------------------------------- */

static bool panel_activate_widget(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return false;
    lv_obj_send_event(obj, LV_EVENT_PRESSED, NULL);
    if (!lv_obj_is_valid(obj)) return true;
    lv_obj_send_event(obj, LV_EVENT_RELEASED, NULL);
    if (!lv_obj_is_valid(obj)) return true;
    lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL);
    return true;
}

static void clear_focus_ring(lv_obj_t *obj)
{
    if (s_nav_marker && lv_obj_is_valid(s_nav_marker)) {
        lv_obj_add_flag(s_nav_marker, LV_OBJ_FLAG_HIDDEN);
    }
    if (!obj || !lv_obj_is_valid(obj)) return;

    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_clear_state(obj, LV_STATE_FOCUSED);

    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_buttonmatrix_class)) {
        lv_buttonmatrix_set_selected_button(obj, LV_BUTTONMATRIX_BUTTON_NONE);
    }
}

static void apply_focus_ring(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return;

    lv_obj_update_layout(obj);

    /* کادر هایلایت بصری: منحصراً روی خود کلید تعاملی قرار می‌گیرد و هرگز کل کارت والد را نمی‌پوشاند */
    lv_obj_t *target_frame = obj;
    if ((obj == objects.btn_ss_timeout || obj == objects.btn_pulley_en) && lv_obj_get_child_count(obj) >= 2) {
        target_frame = lv_obj_get_child(obj, 1);
    }

    int32_t r = lv_obj_get_style_radius(target_frame, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (r <= 0) r = 4;
    else if (r > 12) r = 10;

    if (!s_nav_marker || !lv_obj_is_valid(s_nav_marker)) {
        s_nav_marker = lv_obj_create(lv_layer_top());
        if (s_nav_marker) {
            lv_obj_clear_flag(s_nav_marker, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
            lv_obj_set_style_bg_opa(s_nav_marker, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(s_nav_marker, lv_color_hex(0x00F0FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(s_nav_marker, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(s_nav_marker, r, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(s_nav_marker, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(s_nav_marker, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    } else {
        lv_obj_set_style_radius(s_nav_marker, r, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_nav_marker, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (s_nav_marker && lv_obj_is_valid(s_nav_marker)) {
        lv_area_t a;
        lv_obj_get_coords(target_frame, &a);
        int32_t x1 = (a.x1 >= 3) ? (a.x1 - 3) : 0;
        int32_t y1 = (a.y1 >= 3) ? (a.y1 - 3) : 0;
        int32_t x2 = (a.x2 <= (480 - 4)) ? (a.x2 + 3) : (480 - 1);
        int32_t y2 = (a.y2 <= (320 - 4)) ? (a.y2 + 3) : (320 - 1);
        int32_t mw = x2 - x1 + 1;
        int32_t mh = y2 - y1 + 1;
        if (mw < 10) mw = 10;
        if (mh < 10) mh = 10;
        lv_obj_set_pos(s_nav_marker, x1, y1);
        lv_obj_set_size(s_nav_marker, mw, mh);
        lv_obj_clear_flag(s_nav_marker, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_nav_marker);
    }

    lv_obj_add_state(obj, LV_STATE_FOCUSED);

    bool is_tab_bar = (objects.obj6 && lv_obj_is_valid(objects.obj6) && obj == lv_tabview_get_tab_btns(objects.obj6));

    /* کیبرد/بات‌ماتریس: در اولین فوکوس، نشانگر روی کل کیبرد (ACTIVE) → START برای ورود تک‌تک کلیدها (KEYBOARD) */
    if ((lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_buttonmatrix_class)) && !is_tab_bar) {
        lv_buttonmatrix_set_selected_button(obj, LV_BUTTONMATRIX_BUTTON_NONE); /* هیچ کلیدی انتخاب نشود */
        g_nav.state = NAV_STATE_ACTIVE;  /* کل کیبرد به عنوان یک آیتم */
    } else {
        g_nav.state = NAV_STATE_ACTIVE;
    }

    /* اسکرول نرم خودکار جهت هدایت ویجت یا کادر استپر به دید اپراتور */
    lv_obj_scroll_to_view_recursive(target_frame, LV_ANIM_ON);
}

void panel_nav_exit_mode(void)
{
    if (s_nav_marker && lv_obj_is_valid(s_nav_marker)) {
        lv_obj_add_flag(s_nav_marker, LV_OBJ_FLAG_HIDDEN);
    }
    if (g_nav.focus_obj && lv_obj_is_valid(g_nav.focus_obj)) {
        clear_focus_ring(g_nav.focus_obj);
    }
    g_nav.focus_obj = NULL;
    g_nav.state = NAV_STATE_IDLE;
    g_nav.current_index = -1;
    g_nav.item_count = 0;
}

bool panel_nav_is_active(void)
{
    return (g_nav.state != NAV_STATE_IDLE);
}

bool panel_nav_focus_is_repeatable(void)
{
    if (g_nav.state == NAV_STATE_IDLE || !g_nav.focus_obj || !lv_obj_is_valid(g_nav.focus_obj)) {
        return false;
    }
    lv_obj_t *o = g_nav.focus_obj;
    if (lv_obj_check_type(o, &lv_slider_class) ||
        lv_obj_check_type(o, &lv_arc_class) ||
        lv_obj_check_type(o, &lv_dropdown_class) ||
        lv_obj_check_type(o, &lv_roller_class) ||
        o == objects.btn_ss_timeout ||
        o == objects.btn_pulley_en ||
        (objects.obj6 && lv_obj_is_valid(objects.obj6) && o == lv_tabview_get_tab_btns(objects.obj6))) {
        return true;
    }
    /* استپرهای عددی تنظیمات نیز با نگه‌داشتن ممتد START/STOP تکرار شوند */
    lv_obj_t *parent = lv_obj_get_parent(o);
    if (parent && lv_obj_get_child_count(parent) >= 3) {
        lv_obj_t *c0 = lv_obj_get_child(parent, 0);
        lv_obj_t *c2 = lv_obj_get_child(parent, 2);
        if (o == c0 || o == c2) {
            return true;
        }
    }
    return false;
}

void panel_nav_notify_touch(void)
{
    panel_nav_notify_touch_point(-1, -1);
}

void panel_nav_notify_touch_point(int16_t x, int16_t y)
{
    if (g_nav.state != NAV_STATE_IDLE) {
        panel_nav_exit_mode();
    }
    g_nav.saved_confirm_index = -1;
    g_nav.saved_screen_id = -1;
    g_nav.last_confirm_ms = 0;

    /* تسریع بسته شدن پیغام زمان‌دار (Toast) در صورت لمس خارج از کادر آن روی صفحه */
    if (s_toast_card && lv_obj_is_valid(s_toast_card) && !lv_obj_has_flag(s_toast_card, LV_OBJ_FLAG_HIDDEN)) {
        if (x >= 0 && y >= 0) {
            lv_area_t coords;
            lv_obj_get_coords(s_toast_card, &coords);
            if (x < coords.x1 || x > coords.x2 || y < coords.y1 || y > coords.y2) {
                if (s_toast_remaining_ms > 50) {
                    s_toast_remaining_ms = 50;
                    s_toast_paused = false;
                    if (s_toast_bar && lv_obj_is_valid(s_toast_bar)) {
                        int32_t w = (int32_t)((TOAST_WIDTH * 50) / TOAST_DURATION_MS);
                        lv_obj_set_width(s_toast_bar, w > 0 ? w : 0);
                    }
                }
            }
        }
    }
}

void panel_nav_on_screen_load(int screen_id)
{
    panel_nav_exit_mode();
    g_nav.current_screen_id = screen_id;
    g_nav.saved_confirm_index = -1;
    g_nav.saved_screen_id = -1;
    g_nav.last_confirm_ms = 0;
}

static void adjust_focused_widget(int dir)
{
    lv_obj_t *obj = g_nav.focus_obj;
    if (!obj || !lv_obj_is_valid(obj)) return;

    /* ۰. نوار تب‌های صفحه تنظیمات (تغییر تب بدون نیاز به تاچ اسکرین) */
    if (objects.obj6 && lv_obj_is_valid(objects.obj6) && obj == lv_tabview_get_tab_btns(objects.obj6)) {
        uint32_t cur = lv_tabview_get_tab_active(objects.obj6);
        uint32_t target_tab = (dir > 0) ? ((cur + 1) % 6) : ((cur + 5) % 6);
        open_settings_tab(target_tab);
        buzzer_beep_click();

        /* به‌روزرسانی سریع کش ناوبری با ویجت‌های تب جدید */
        g_nav.item_count = 0;
        collect_actionable_widgets(lv_tabview_get_content(objects.obj6));
        g_nav.current_index = 0;
        if (g_nav.item_count > 0) {
            g_nav.focus_obj = g_nav.items[0];
            apply_focus_ring(g_nav.focus_obj);
        }
        return;
    }

    /* ۱. اسلایدرها */
    if (lv_obj_check_type(obj, &lv_slider_class)) {
        int32_t val = lv_slider_get_value(obj);
        int32_t min = lv_slider_get_min_value(obj);
        int32_t max = lv_slider_get_max_value(obj);

        if (dir > 0) {
            if (val >= max) {
                panel_nav_toast_warning((ui_lang_get() == UI_LANG_FA) ? "حداکثر مقدار مجاز" :
                                        ((ui_lang_get() == UI_LANG_TR) ? "Maksimum degere ulasildi" : "Maximum limit reached"));
                buzzer_beep_click();
                return;
            }
            val += 1;
        } else {
            if (val <= min) {
                panel_nav_toast_warning((ui_lang_get() == UI_LANG_FA) ? "حداقل مقدار مجاز" :
                                        ((ui_lang_get() == UI_LANG_TR) ? "Minimum degere ulasildi" : "Minimum limit reached"));
                buzzer_beep_click();
                return;
            }
            val -= 1;
        }

        lv_slider_set_value(obj, val, LV_ANIM_ON);
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        buzzer_beep_click();
        return;
    }

    /* ۲. سوییچ‌ها */
    if (lv_obj_check_type(obj, &lv_switch_class)) {
        if (dir > 0) {
            if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
                lv_obj_clear_state(obj, LV_STATE_CHECKED);
            } else {
                lv_obj_add_state(obj, LV_STATE_CHECKED);
            }
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
            buzzer_beep_click();
        }
        return;
    }

    /* ۳. آرک‌ها */
    if (lv_obj_check_type(obj, &lv_arc_class)) {
        int32_t val = lv_arc_get_value(obj);
        int32_t min = lv_arc_get_min_value(obj);
        int32_t max = lv_arc_get_max_value(obj);

        if (dir > 0) {
            if (val < max) val += 1;
            else panel_nav_toast_warning((ui_lang_get() == UI_LANG_FA) ? "حداکثر مقدار مجاز" :
                                         ((ui_lang_get() == UI_LANG_TR) ? "Maksimum degere ulasildi" : "Maximum limit reached"));
        } else {
            if (val > min) val -= 1;
            else panel_nav_toast_warning((ui_lang_get() == UI_LANG_FA) ? "حداقل مقدار مجاز" :
                                         ((ui_lang_get() == UI_LANG_TR) ? "Minimum degere ulasildi" : "Minimum limit reached"));
        }
        lv_arc_set_value(obj, val);
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        buzzer_beep_click();
        return;
    }

    /* ۳.۵. دراپ‌داون‌ها (تغییر گزینه با START و STOP) */
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        uint32_t count = lv_dropdown_get_option_count(obj);
        if (count > 1) {
            uint32_t sel = lv_dropdown_get_selected(obj);
            if (dir > 0) {
                sel = (sel + 1) % count;
            } else {
                sel = (sel + count - 1) % count;
            }
            lv_dropdown_set_selected(obj, sel);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
            buzzer_beep_click();
            return;
        }
    }

    /* ۴. کیبرد مجازی — دو فاز:
     *    فاز ۱ (ACTIVE  + dir>0): ورود به کیبرد → انتخاب کلید اول
     *    فاز ۲ (KEYBOARD + dir>0): تأیید کلید انتخاب‌شده
     *    STOP در KEYBOARD: حذف آخرین کاراکتر؛ در ACTIVE: بدون اقدام */
    if (lv_obj_check_type(obj, &lv_keyboard_class) || lv_obj_check_type(obj, &lv_buttonmatrix_class)) {
        if (dir > 0) {
            if (g_nav.state != NAV_STATE_KEYBOARD) {
                /* فاز ۱: START روی کل کیبرد → وارد حالت کلید‌به‌کلید */
                g_nav.state = NAV_STATE_KEYBOARD;
                lv_buttonmatrix_set_selected_button(obj, 0);
                lv_obj_add_state(obj, LV_STATE_FOCUSED);
                /* نشانگر کل کیبرد پنهان می‌شود — هایلایت تک‌کلید جایگزین */
                if (s_nav_marker && lv_obj_is_valid(s_nav_marker))
                    lv_obj_add_flag(s_nav_marker, LV_OBJ_FLAG_HIDDEN);
                buzzer_beep_click();
            } else {
                /* فاز ۲: START روی کلید انتخاب‌شده → ارسال رویداد تأیید */
                lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
                buzzer_beep_click();
            }
        } else {
            /* STOP فقط در حالت KEYBOARD حذف می‌کند */
            if (g_nav.state == NAV_STATE_KEYBOARD) {
                lv_obj_t *ta = lv_keyboard_get_textarea(obj);
                if (ta && lv_obj_is_valid(ta)) {
                    lv_textarea_delete_char(ta);
                }
                buzzer_beep_click();
            }
        }
        return;
    }

    /* ۵. جعبه متن */
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        if (dir > 0) {
            lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL);
        } else {
            lv_textarea_delete_char(obj);
        }
        buzzer_beep_click();
        return;
    }

    /* ۶. دکمه‌ها و اشیای قابل کلیک */
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        if (dir > 0) {
            panel_activate_widget(obj);
            buzzer_beep_click();
        } else {
            /* اگر فوکوس روی دکمه افزایشی [+] استپر باشد و STOP فشرده شود، دکمه کاهشی [-] معادل فعال شود */
            lv_obj_t *parent = lv_obj_get_parent(obj);
            if (parent && lv_obj_get_child_count(parent) >= 3) {
                lv_obj_t *c0 = lv_obj_get_child(parent, 0);
                lv_obj_t *c2 = lv_obj_get_child(parent, 2);
                if (obj == c2 && c0 && lv_obj_has_flag(c0, LV_OBJ_FLAG_CLICKABLE)) {
                    panel_activate_widget(c0);
                    buzzer_beep_click();
                    return;
                }
            }
        }
        /* قانون ایمنی صنعتی: STOP هرگز دکمه‌های تایید را کلیک نمی‌کند اما بوق بازخورد می‌زند */
        buzzer_beep_click();
        return;
    }
}

/* -------------------------------------------------------------------------- */
/*                               POLL TICK FSM                                */
/* -------------------------------------------------------------------------- */

void panel_nav_poll_tick(void)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

    /* نگهبان ۱۰ ثانیه‌ای عدم فعالیت */
    if (g_nav.state != NAV_STATE_IDLE) {
        if ((now - g_nav.last_activity_ms) >= INACTIVITY_TIMEOUT_MS) {
            panel_nav_exit_mode();
            g_nav.saved_confirm_index = -1;
            g_nav.saved_screen_id = -1;
            g_nav.last_confirm_ms = 0;
            panel_nav_toast_info((ui_lang_get() == UI_LANG_FA) ? "خروج خودکار از ناوبری (عدم فعالیت)" :
                                 ((ui_lang_get() == UI_LANG_TR) ? "Gezinmeden cikildi (Hareketsizlik)" : "Navigation exited (Inactivity)"));
            return;
        }
    }

    uint8_t row = 0, key = 0;
    if (!dequeue_key(&row, &key)) return;

    /* DEBUG موقت: تأیید دریافت کلید */
    ESP_LOGI("NAV", "KEY row=%d key=%d state=%d ss=%d", row, key, g_nav.state, (int)screensaver_is_active());

    /* اسکرین‌سیور فعال است: هر کلیدی صرفاً صفحه را بیدار کند */
    if (screensaver_is_active()) {
        hide_screensaver();
        screensaver_notify_activity();
        return;
    }
    screensaver_notify_activity();

    if (day_lock_is_locked()) {
        if (!day_lock_ui_is_visible()) {
            day_lock_ui_show();
        }
        day_lock_ui_handle_physical_key(row, key);
        return;
    }

    if (day_lock_ui_is_visible()) {
        day_lock_ui_handle_physical_key(row, key);
        return;
    }

    g_nav.last_activity_ms = now;

    /* بازخورد صوتی بلادرنگ بازر برای ۵ کلید جلوی پنل و F1..F6 */
    if (!(row == 2 && key == 1 && ui_get_screen_id() == SCREEN_ID_SETTINGS_PG)) {
        buzzer_beep_click();
    }

    /* ------------------- کلید CANCEL (ردیف ۲، کلید ۲) ------------------- */
    if (row == 2 && key == 2) {
        /* ۰. اگر در حالت ناوبری کلیدهای کیبورد هستیم، برگشت به ناوبری خود ویجت‌ها */
        if (g_nav.state == NAV_STATE_KEYBOARD) {
            g_nav.state = NAV_STATE_ACTIVE;
            if (g_nav.focus_obj && lv_obj_is_valid(g_nav.focus_obj)) {
                lv_buttonmatrix_set_selected_button(g_nav.focus_obj, LV_BUTTONMATRIX_BUTTON_NONE);
                apply_focus_ring(g_nav.focus_obj);
            }
            buzzer_beep_click();
            return;
        }

        /* ۱. اگر مودال فعال است: اول ببند، سپس nav را کلاً پاک کن */
        if (panel_nav_dismiss_active_modal()) {
            panel_nav_exit_mode();
            g_nav.saved_confirm_index = -1;
            g_nav.saved_screen_id = -1;
            g_nav.last_confirm_ms = 0;
            buzzer_beep_click();
            return;
        }

        /* ۲. در هر حالت nav (ACTIVE/KEYBOARD/SETTINGS): خروج کامل + نشانگر پاک */
        if (g_nav.state != NAV_STATE_IDLE) {
            panel_nav_exit_mode();
            g_nav.saved_confirm_index = -1;
            g_nav.saved_screen_id = -1;
            g_nav.last_confirm_ms = 0;
            panel_nav_toast_info((ui_lang_get() == UI_LANG_FA) ? "خروج از ناوبری" :
                                 ((ui_lang_get() == UI_LANG_TR) ? "Gezinmeden cikildi" : "Navigation exited"));
            buzzer_beep_click();
            return;
        }

        /* ۳. IDLE در صفحه تنظیمات: ذخیره و برگشت به خانه */
        if (ui_get_screen_id() == SCREEN_ID_SETTINGS_PG) {
            hwkey_save_and_home();
            buzzer_beep_click();
            return;
        }
        return;
    }

    /* ------------------- کلید SAVE (ردیف ۲، کلید ۱) ------------------- */
    if (row == 2 && key == 1) {
        if (ui_get_screen_id() == SCREEN_ID_SETTINGS_PG) {
            /* فقط تنظیمات همان صفحه ذخیره می‌شود */
            hwkey_save_and_home();
            buzzer_beep_confirm();
            panel_nav_toast_success(ui_lang_str(STR_SETTINGS_SAVED));
        } else {
            /* در صفحات دیگر: SAVE بدون اقدام — راهنما نمایش داده می‌شود */
            panel_nav_toast_info((ui_lang_get() == UI_LANG_FA) ? "برای ذخیره، وارد تنظیمات شوید" :
                                 ((ui_lang_get() == UI_LANG_TR) ? "Kaydetmek icin ayarlara gidin" :
                                  "Enter settings to save"));
            buzzer_beep_click();
        }
        if (g_nav.state != NAV_STATE_IDLE) {
            panel_nav_exit_mode();
            g_nav.saved_confirm_index = -1;
            g_nav.saved_screen_id = -1;
            g_nav.last_confirm_ms = 0;
        }
        return;
    }

    /* ------------------- کلید MODE (ردیف ۲، کلید ۳) ------------------- */
    if (row == 2 && key == 3) {
        if (warranty_is_expired_and_locked() && ui_get_screen_id() == SCREEN_ID_MAIN_PG && !panel_nav_has_active_modal()) {
            buzzer_beep_alarm();
            extern void open_warranty_expired_dialog(void);
            open_warranty_expired_dialog();
            return;
        }

        /* در حالت KEYBOARD (بعد از START روی کیبرد): MODE بین کلیدهای کیبرد حرکت می‌کند */
        if (g_nav.state == NAV_STATE_KEYBOARD) {
            uint32_t right = LV_KEY_RIGHT;
            lv_obj_send_event(g_nav.focus_obj, LV_EVENT_KEY, &right);
            buzzer_beep_click();
            return;
        }

        /* ورود به ناوبری از حالت IDLE */
        if (g_nav.state == NAV_STATE_IDLE) {
            g_nav.item_count = 0;
            lv_obj_t *root = get_active_modal_root();
            if (!root) {
                if (ui_get_screen_id() == SCREEN_ID_MAIN_PG) {
                    /* صفحه اصلی منحصراً دارای ۷ سناریوی کنترلی فعال است (تولید، نگهداری، پارو، لبه، پارو مخزن شیر، شستشو، تنظیمات) */
                    lv_obj_t *main_btns[] = {
                        objects.compressor_btn, /* Make */
                        objects.icecreem_btn,   /* Paddle */
                        objects.fridge_btn,     /* Hold */
                        objects.compressor_btn_b,
                        objects.fridge_btn_b,
                        objects.icecreem_btn_b,
                        objects.settings_btn
                    };
                    for (size_t b = 0; b < sizeof(main_btns) / sizeof(main_btns[0]); b++) {
                        /* دکمه‌های هاردکد شده صفحه اصلی: دستی انتخاب شدند → بایپس is_actionable */
                        if (main_btns[b] && lv_obj_is_valid(main_btns[b]) &&
                            !lv_obj_has_flag(main_btns[b], LV_OBJ_FLAG_HIDDEN) &&
                            !lv_obj_has_state(main_btns[b], LV_STATE_DISABLED)) {
                            g_nav.items[g_nav.item_count++] = main_btns[b];
                        }
                    }
                } else if (ui_get_screen_id() == SCREEN_ID_SETTINGS_PG && objects.obj6 && lv_obj_is_valid(objects.obj6)) {
                    /* در صفحه تنظیمات: پیمایش ترتیبی پارامترهای فعال تب جاری */
                    collect_actionable_widgets(lv_tabview_get_content(objects.obj6));
                } else {
                    root = lv_screen_active();
                    collect_actionable_widgets(root);
                }
            } else {
                collect_actionable_widgets(root);
            }

            /* مرتب‌سازی بصری: بالا→پایین، چپ→راست */
            sort_items_by_position();

            if (g_nav.item_count == 0) {
                panel_nav_toast_info((ui_lang_get() == UI_LANG_FA) ? "هیچ گزینه فعالی یافت نشد" :
                                     ((ui_lang_get() == UI_LANG_TR) ? "Etkin oge bulunamadi" : "No actionable items found"));
                return;
            }

            int cur_screen = ui_get_screen_id();
            int target_index = 0;

            /* بررسی قانون ۱۰ ثانیه: اگر در همان صفحه و زیر ۱۰ ثانیه تایید انجام شده بود، ادامه از آیتم بعدی */
            if (g_nav.saved_confirm_index >= 0 &&
                g_nav.saved_screen_id == cur_screen &&
                (now - g_nav.last_confirm_ms) < 10000U) {
                target_index = g_nav.saved_confirm_index + 1;
                if (target_index >= g_nav.item_count) {
                    target_index = 0; /* دور زدن حلقه به ابتدای گزینه‌ها */
                }
            } else {
                target_index = 0;
            }

            g_nav.current_index = target_index;
            g_nav.focus_obj = g_nav.items[target_index];
            apply_focus_ring(g_nav.focus_obj);
            buzzer_beep_click();
            return;
        }

        /* پیمایش ترتیبی در حالت ACTIVE با قابلیت دور زدن کامل حلقه (Wrap-around) */
        if (g_nav.state == NAV_STATE_ACTIVE) {
            clear_focus_ring(g_nav.focus_obj);
            /* پیمایش ترتیبی: رسیدن به آخرین آیتم → دور زدن حلقه به اولین آیتم (Wrap-around) */
            int next_index = g_nav.current_index + 1;
            if (next_index >= g_nav.item_count) {
                next_index = 0;
            }
            g_nav.current_index = next_index;
            g_nav.focus_obj = g_nav.items[g_nav.current_index];
            apply_focus_ring(g_nav.focus_obj);
            buzzer_beep_click();
            return;
        }
        return;
    }

    /* ------------------- کلید START (ردیف ۲، کلید ۴) ------------------- */
    if (row == 2 && key == 4) {
        if (warranty_is_expired_and_locked() && (g_nav.state == NAV_STATE_IDLE || ui_get_screen_id() == SCREEN_ID_MAIN_PG)) {
            ui_lang_t lang = ui_lang_get();
            panel_nav_toast_error((lang == UI_LANG_FA) ? "گارانتی فعال نشده است — کلیدها قفل می‌باشند" :
                                  ((lang == UI_LANG_TR) ? "Garanti etkinlestirilmedi - Tuslar kilitli" :
                                   "Warranty not activated - Keys locked"));
            buzzer_beep_alarm();
            extern void open_warranty_expired_dialog(void);
            open_warranty_expired_dialog();
            return;
        }

        if (g_nav.state == NAV_STATE_IDLE) {
            /* اینترلاک ایمنی صنعتی زیمنس: استارت فیزیکی منحصراً در صفحه اصلی و بدون مودال فعال مجاز است */
            if (ui_get_screen_id() == SCREEN_ID_MAIN_PG && !panel_nav_has_active_modal()) {
                plc_hw_start_make();
                buzzer_beep_click();
            } else {
                panel_nav_toast_warning((ui_lang_get() == UI_LANG_FA) ? "جهت استارت، به صفحه اصلی بروید" :
                                        ((ui_lang_get() == UI_LANG_TR) ? "Baslatmak icin ana sayfaya gidin" :
                                         "Go to main screen to start"));
                buzzer_beep_click();
            }
            return;
        } else {
            /* تنظیم مثبت ویجت / تایید */
            int confirmed_idx = g_nav.current_index;
            int cur_screen = ui_get_screen_id();
            bool is_repeatable = panel_nav_focus_is_repeatable();
            adjust_focused_widget(+1);

            int post_screen = ui_get_screen_id();
            if (post_screen != cur_screen || panel_nav_has_active_modal()) {
                /* اگر صفحه تغییر کرد یا پنجره جدیدی باز شد، حافظه قبلی را پاک کن تا از ابتدای پنجره جدید شروع کند */
                panel_nav_exit_mode();
                g_nav.saved_confirm_index = -1;
                g_nav.saved_screen_id = -1;
                g_nav.last_confirm_ms = 0;
            } else if (!is_repeatable || cur_screen == SCREEN_ID_MAIN_PG) {
                /* در همان صفحه تایید شد: نشانگر محو شود و موقعیت برای ادامه حرکت زیر ۱۰ ثانیه ذخیره گردد */
                panel_nav_exit_mode();
                g_nav.saved_confirm_index = confirmed_idx;
                g_nav.saved_screen_id = cur_screen;
                g_nav.last_confirm_ms = now;
            }
            return;
        }
    }

    /* ------------------- کلید STOP (ردیف ۲، کلید ۵) ------------------- */
    if (row == 2 && key == 5) {
        if (g_nav.state != NAV_STATE_IDLE) {
            /* در ناوبری: STOP = کاهش مقدار / Backspace روی ویجت فوکوس‌دار */
            adjust_focused_widget(-1);
            return;
        }
        /* اگر خطایی در سیستم فعال است، شستی STOP تلاش برای پاک‌سازی خطا می‌کند */
        if (plc_is_fault()) {
            if (plc_try_clear_fault()) {
                hide_interactive_fault_dialog();
                ui_lang_t lang = ui_lang_get();
                panel_nav_toast_success((lang == UI_LANG_FA) ? "خطای حفاظتی برطرف شد — سیستم آماده به کار" :
                                        ((lang == UI_LANG_TR) ? "Hata temizlendi — Sistem hazir" :
                                         "Fault cleared — System ready"));
                buzzer_beep_confirm();
                ui_request_apply();
                return;
            } else {
                ui_lang_t lang = ui_lang_get();
                panel_nav_toast_warning((lang == UI_LANG_FA) ? "خطا همچنان فعال است — علت فیزیکی را رفع کنید" :
                                        ((lang == UI_LANG_TR) ? "Hata hala aktif — Fiziksel nedeni giderin" :
                                         "Fault still active — Resolve physical cause"));
                buzzer_beep_alarm();
                return;
            }
        }
        /* خارج از ناوبری: STOP همیشه = قطع سناریوی فعال و همه خروجی‌ها */
        plc_stop_all();
        buzzer_beep_click();
        return;
    }

    /* ------------------- کلیدهای F1..F6 (ردیف ۱) ------------------- */
    if (row == 1 && key >= 1 && key <= 6) {
        open_settings_tab((uint32_t)(key - 1));
        buzzer_beep_click();

        /* در صورت فعال بودن ناوبری، جمع‌آوری مجدد گزینه‌های تب جدید */
        if (g_nav.state != NAV_STATE_IDLE) {
            g_nav.saved_confirm_index = -1;
            g_nav.saved_screen_id = -1;
            g_nav.last_confirm_ms = 0;
            g_nav.item_count = 0;
            if (objects.obj6 && lv_obj_is_valid(objects.obj6)) {
                collect_actionable_widgets(lv_tabview_get_content(objects.obj6));
            } else {
                collect_actionable_widgets(lv_screen_active());
            }
            sort_items_by_position();
            if (g_nav.item_count > 0) {
                g_nav.current_index = 0;
                g_nav.focus_obj = g_nav.items[0];
                apply_focus_ring(g_nav.focus_obj);
            } else {
                panel_nav_exit_mode();
            }
        }
        return;
    }
}

/* -------------------------------------------------------------------------- */
/*                               INITIALIZATION                               */
/* -------------------------------------------------------------------------- */

static void panel_nav_timer_cb(lv_timer_t *t)
{
    (void)t;
    panel_nav_poll_tick();
}

void panel_nav_init(void)
{
    memset(&g_nav, 0, sizeof(g_nav));
    g_nav.current_index = -1;
    g_nav.saved_confirm_index = -1;
    g_nav.saved_screen_id = -1;
    g_nav.last_confirm_ms = 0;
    s_q_head = 0;
    s_q_tail = 0;

    if (!s_nav_timer) {
        s_nav_timer = lv_timer_create(panel_nav_timer_cb, 5, NULL);
    }
    ESP_LOGI(TAG, "Touchless panel navigation engine initialized (5ms high-speed tick)");
}
