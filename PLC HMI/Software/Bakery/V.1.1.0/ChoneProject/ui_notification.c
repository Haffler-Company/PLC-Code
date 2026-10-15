#include "ui_notification.h"
#include <stdio.h>
#include <string.h>
#include "lang.h"

extern const lv_font_t ui_font_Dana_Bold18;
extern const lv_font_t ui_font_Dana_Bold13;
extern const lv_font_t ui_font_Dana_Bold10;
extern void sound_play_c(uint8_t sound_type);
extern bool lv_lock_c(uint32_t timeout_ms);
extern void lv_unlock_c(void);

#define NOTIF_SND_CLICK        1
#define NOTIF_SND_PORTION_DONE 2
#define NOTIF_SND_ALARM_FAULT  5
#define NOTIF_SND_WARNING      7

static lv_obj_t* s_notif_banner = NULL;
static lv_obj_t* s_lbl_title = NULL;
static lv_obj_t* s_lbl_msg = NULL;
static lv_timer_t* s_dismiss_timer = NULL;

static lv_obj_t* s_progress_bar = NULL;
static uint32_t s_elapsed_ms = 0;
static uint32_t s_total_ms = 3500;
static bool s_touch_frozen = false;

static void notif_drain_timer_cb(lv_timer_t* timer)
{
    (void)timer;
    if (s_touch_frozen) return; // Touch-and-hold freeze!

    s_elapsed_ms += 50;
    if (s_elapsed_ms >= s_total_ms)
    {
        ui_notification_dismiss();
        return;
    }

    if (s_progress_bar != NULL && lv_obj_is_valid(s_progress_bar))
    {
        int32_t remaining_pct = (int32_t)(100 - ((s_elapsed_ms * 100) / s_total_ms));
        if (remaining_pct < 0) remaining_pct = 0;
        lv_bar_set_value(s_progress_bar, remaining_pct, LV_ANIM_OFF);
    }
}

static void notif_touch_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        s_touch_frozen = true; // Freeze draining countdown while held
    }
    else if (code == LV_EVENT_RELEASED)
    {
        s_touch_frozen = false; // Resume countdown
    }
    else if (code == LV_EVENT_CLICKED)
    {
        ui_notification_dismiss(); // Tap dismisses immediately
    }
}

static lv_obj_t* s_lbl_icon = NULL;

void ui_notification_init(void)
{
    s_notif_banner = NULL;
    s_progress_bar = NULL;
    s_lbl_icon = NULL;
    s_lbl_title = NULL;
    s_lbl_msg = NULL;
    s_dismiss_timer = NULL;
    s_touch_frozen = false;
    s_elapsed_ms = 0;
}

void ui_notification_dismiss(void)
{
    if (!lv_lock_c(100)) return;

    if (s_dismiss_timer != NULL)
    {
        lv_timer_del(s_dismiss_timer);
        s_dismiss_timer = NULL;
    }

    if (s_notif_banner != NULL && lv_obj_is_valid(s_notif_banner))
    {
        lv_obj_del(s_notif_banner);
    }
    s_notif_banner = NULL;
    s_progress_bar = NULL;
    s_lbl_icon = NULL;
    s_lbl_title = NULL;
    s_lbl_msg = NULL;
    s_touch_frozen = false;
    s_elapsed_ms = 0;

    lv_unlock_c();
}

void ui_show_notification(notification_type_t type, const char* title, const char* message)
{
    if (title == NULL && message == NULL) return;
    if (!lv_lock_c(100)) return;

    // Determine Apple HIG semantic colors and icon badges
    lv_color_t border_col;
    uint32_t duration_ms = 2500;
    const char* symbol = LV_SYMBOL_BELL;

    switch (type)
    {
        case NOTIF_SUCCESS:
            border_col = lv_color_hex(0x30D158); // Apple Green
            duration_ms = 2500;
            symbol = LV_SYMBOL_OK;
            sound_play_c(NOTIF_SND_PORTION_DONE);
            break;
        case NOTIF_INFO:
            border_col = lv_color_hex(0x0A84FF); // Apple Blue
            duration_ms = 2500;
            symbol = LV_SYMBOL_BELL;
            sound_play_c(NOTIF_SND_CLICK);
            break;
        case NOTIF_WARNING:
            border_col = lv_color_hex(0xFF9F0A); // Apple Amber/Orange
            duration_ms = 3500;
            symbol = LV_SYMBOL_WARNING;
            sound_play_c(NOTIF_SND_WARNING);
            break;
        case NOTIF_ERROR:
        default:
            border_col = lv_color_hex(0xFF453A); // Apple Red
            duration_ms = 4500;
            symbol = LV_SYMBOL_CLOSE;
            sound_play_c(NOTIF_SND_ALARM_FAULT);
            break;
    }

    // Reset previous timer if active
    if (s_dismiss_timer != NULL)
    {
        lv_timer_del(s_dismiss_timer);
        s_dismiss_timer = NULL;
    }

    // Text direction and alignment based on active language
    const bool is_rtl = (g_lang == LANG_FA);
    lv_base_dir_t base_dir = is_rtl ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR;
    lv_text_align_t text_align = is_rtl ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT;

    // Create banner on top-most layer if not already existing
    if (s_notif_banner == NULL || !lv_obj_is_valid(s_notif_banner))
    {
        s_notif_banner = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(s_notif_banner);
        lv_obj_set_size(s_notif_banner, 464, 68);
        lv_obj_set_align(s_notif_banner, LV_ALIGN_TOP_MID);
        lv_obj_set_y(s_notif_banner, 6);
        lv_obj_clear_flag(s_notif_banner, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_notif_banner, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_radius(s_notif_banner, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_notif_banner, lv_color_hex(0x161B22), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(s_notif_banner, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_notif_banner, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(s_notif_banner, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(s_notif_banner, notif_touch_cb, LV_EVENT_ALL, NULL);

        // Dedicated icon label using Montserrat font (guaranteed valid LV_SYMBOL glyphs)
        s_lbl_icon = lv_label_create(s_notif_banner);
        lv_obj_set_style_text_font(s_lbl_icon, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Text labels using Dana font
        s_lbl_title = lv_label_create(s_notif_banner);
        lv_obj_set_width(s_lbl_title, 410);
        lv_obj_set_style_text_font(s_lbl_title, &ui_font_Dana_Bold18, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_lbl_msg = lv_label_create(s_notif_banner);
        lv_obj_set_width(s_lbl_msg, 446);
        lv_obj_set_style_text_font(s_lbl_msg, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(s_lbl_msg, lv_color_hex(0xE4E4E7), LV_PART_MAIN | LV_STATE_DEFAULT);

        // Apple-style slim 4px draining progress bar at bottom edge
        s_progress_bar = lv_bar_create(s_notif_banner);
        lv_obj_remove_style_all(s_progress_bar);
        lv_obj_set_size(s_progress_bar, 460, 4);
        lv_obj_align(s_progress_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_radius(s_progress_bar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_progress_bar, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_progress_bar, lv_color_hex(0x2A2E39), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(s_progress_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // Dynamic RTL/LTR realignment
    lv_obj_set_style_base_dir(s_lbl_title, base_dir, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(s_lbl_title, text_align, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(s_lbl_msg, base_dir, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(s_lbl_msg, text_align, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (is_rtl)
    {
        lv_obj_align(s_lbl_icon, LV_ALIGN_TOP_RIGHT, -4, 2);
        lv_obj_align(s_lbl_title, LV_ALIGN_TOP_RIGHT, -32, 0);
        lv_obj_align(s_lbl_msg, LV_ALIGN_BOTTOM_RIGHT, -4, -6);
    }
    else
    {
        lv_obj_align(s_lbl_icon, LV_ALIGN_TOP_LEFT, 4, 2);
        lv_obj_align(s_lbl_title, LV_ALIGN_TOP_LEFT, 32, 0);
        lv_obj_align(s_lbl_msg, LV_ALIGN_BOTTOM_LEFT, 4, -6);
    }

    // Apply color styling
    lv_obj_set_style_border_color(s_notif_banner, border_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_lbl_icon, border_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_lbl_title, border_col, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (s_progress_bar != NULL && lv_obj_is_valid(s_progress_bar))
    {
        lv_obj_set_style_bg_color(s_progress_bar, border_col, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(s_progress_bar, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_bar_set_range(s_progress_bar, 0, 100);
        lv_bar_set_value(s_progress_bar, 100, LV_ANIM_OFF);
    }

    // Set texts cleanly without concatenation or tofu characters
    lv_label_set_text(s_lbl_icon, symbol);
    lv_label_set_text(s_lbl_title, title ? title : "");
    lv_label_set_text(s_lbl_msg, message ? message : "");

    // Bring to front
    lv_obj_move_foreground(s_notif_banner);

    // Schedule auto-dismiss with 50ms draining animation ticks
    s_elapsed_ms = 0;
    s_total_ms = duration_ms;
    s_touch_frozen = false;
    s_dismiss_timer = lv_timer_create(notif_drain_timer_cb, 50, NULL);

    lv_unlock_c();
}

