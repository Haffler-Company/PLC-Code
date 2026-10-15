#include "ui_toast.h"
#include "ui_vazir_fonts.h"
#include "fonts.h"
#include "a2_eeprom.h"
#include <Arduino.h>

static lv_obj_t *s_toast_backdrop = NULL;
static lv_obj_t *s_toast_container = NULL;
static lv_obj_t *s_toast_bar = NULL;
static lv_obj_t *s_toast_title_lbl = NULL;
static lv_obj_t *s_toast_msg_lbl = NULL;

static uint32_t s_total_duration_ms = 4000;
static uint32_t s_remaining_ms = 0;
static uint32_t s_last_tick_ms = 0;
static bool s_is_paused = false;

static void toast_backdrop_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    /* Tapping outside the dialog on the backdrop dismisses the message immediately */
    if (target == s_toast_backdrop) {
        if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
            ui_toast_dismiss();
        }
    }
}

static void toast_card_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        s_is_paused = true;
    } else if (code == LV_EVENT_RELEASED) {
        s_is_paused = false;
        s_last_tick_ms = millis();
    }
}

static void toast_close_btn_cb(lv_event_t *e) {
    (void)e;
    ui_toast_dismiss();
}

static int s_last_toast_w = -1;

void ui_toast_dismiss(void) {
    if (s_toast_container) {
        lv_obj_delete(s_toast_container);
        s_toast_container = NULL;
        s_toast_bar = NULL;
        s_toast_title_lbl = NULL;
        s_toast_msg_lbl = NULL;
    }
    if (s_toast_backdrop) {
        lv_obj_delete(s_toast_backdrop);
        s_toast_backdrop = NULL;
    }
    s_is_paused = false;
    s_remaining_ms = 0;
    s_last_toast_w = -1;
}

lv_obj_t *ui_toast_get_container(void) {
    return s_toast_container ? s_toast_container : s_toast_backdrop;
}

bool ui_toast_is_container(const lv_obj_t *obj) {
    if (!obj) return false;
    return (obj == s_toast_container || obj == s_toast_backdrop);
}

void ui_toast_show(const char *title, const char *msg, uint32_t duration_ms, ui_toast_type_t type) {
    if (duration_ms < 2500) duration_ms = 3500;
    if (duration_ms > 20000) duration_ms = 20000;

    ui_toast_dismiss();

    lv_obj_t *top = lv_layer_top();
    if (!top) return;

    bool is_fa = (EEprom2_Language == 0);
    unsigned char lang = EEprom2_Language;

    /* Theme colors, icons, and category titles */
    lv_color_t border_col;
    lv_color_t badge_col;
    lv_color_t header_bg_col;
    const char *category_txt = "";
    const char *icon_symbol = LV_SYMBOL_BELL;

    switch (type) {
        case TOAST_TYPE_ERROR:
            border_col    = lv_color_hex(0xffdc2626); // red-600
            badge_col     = lv_color_hex(0xffef4444); // red-500
            header_bg_col = lv_color_hex(0xfffef2f2); // red-50
            icon_symbol   = LV_SYMBOL_CLOSE;
            category_txt  = (lang == 1) ? "Error" :
                            (lang == 2) ? "Hata" :
                            (lang == 3) ? "Oshibka" : "اخطار";
            break;
        case TOAST_TYPE_WARNING:
            border_col    = lv_color_hex(0xffd97706); // amber-600
            badge_col     = lv_color_hex(0xfff59e0b); // amber-500
            header_bg_col = lv_color_hex(0xfffffbeb); // amber-50
            icon_symbol   = LV_SYMBOL_WARNING;
            category_txt  = (lang == 1) ? "Warning" :
                            (lang == 2) ? "Uyari" :
                            (lang == 3) ? "Preduprezhdeniye" : "هشدار";
            break;
        case TOAST_TYPE_SUCCESS:
            border_col    = lv_color_hex(0xff16a34a); // emerald-600
            badge_col     = lv_color_hex(0xff22c55e); // emerald-500
            header_bg_col = lv_color_hex(0xfff0fdf4); // emerald-50
            icon_symbol   = LV_SYMBOL_OK;
            category_txt  = (lang == 1) ? "Confirmation" :
                            (lang == 2) ? "Onay Mesaji" :
                            (lang == 3) ? "Podtverzhdeniye" : "پیغام تایید";
            break;
        case TOAST_TYPE_INFO:
        default:
            border_col    = lv_color_hex(0xff0284c7); // sky-600
            badge_col     = lv_color_hex(0xff0ea5e9); // sky-500
            header_bg_col = lv_color_hex(0xfff0f9ff); // sky-50
            icon_symbol   = LV_SYMBOL_BELL;
            category_txt  = (lang == 1) ? "Notice" :
                            (lang == 2) ? "Bilgi" :
                            (lang == 3) ? "Soobshcheniye" : "پیغام";
            break;
    }

    /* 1. Full-screen Dim Backdrop (480x320) - tap outside to dismiss faster */
    s_toast_backdrop = lv_obj_create(top);
    lv_obj_set_pos(s_toast_backdrop, 0, 0);
    lv_obj_set_size(s_toast_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_toast_backdrop, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_toast_backdrop, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_toast_backdrop, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_toast_backdrop, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_toast_backdrop, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_toast_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_toast_backdrop, toast_backdrop_event_cb, LV_EVENT_ALL, NULL);

    /* 2. Windows-like Centered Dialog Card (390x186) */
    const int w = 390;
    const int h = 186;
    const int x = (480 - w) / 2;
    const int y = (320 - h) / 2;

    s_toast_container = lv_obj_create(top);
    lv_obj_set_pos(s_toast_container, x, y);
    lv_obj_set_size(s_toast_container, w, h);
    lv_obj_set_style_bg_color(s_toast_container, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_toast_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_toast_container, border_col, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_toast_container, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(s_toast_container, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_toast_container, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_toast_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_toast_container, LV_OBJ_FLAG_SCROLLABLE);

    /* Touch & hold listener on card to pause depleting countdown */
    lv_obj_add_event_cb(s_toast_container, toast_card_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_toast_container, toast_card_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_toast_container, toast_card_event_cb, LV_EVENT_RELEASED, NULL);

    /* 3. Header Bar with category title ("هشدار - اخطار - پیغام") */
    lv_obj_t *header = lv_obj_create(s_toast_container);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, w, 42);
    lv_obj_set_style_bg_color(header, header_bg_col, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(header, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(header, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* Icon badge */
    lv_obj_t *badge = lv_obj_create(header);
    lv_obj_set_size(badge, 28, 28);
    lv_obj_set_style_bg_color(badge, badge_col, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(badge, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *icon_lbl = lv_label_create(badge);
    lv_label_set_text(icon_lbl, icon_symbol);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(icon_lbl);

    /* Category title */
    lv_obj_t *cat_lbl = lv_label_create(header);
    lv_label_set_text(cat_lbl, category_txt);
    lv_obj_set_style_text_font(cat_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(cat_lbl, border_col, LV_PART_MAIN);
    lv_obj_set_style_base_dir(cat_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_clear_flag(cat_lbl, LV_OBJ_FLAG_CLICKABLE);

    /* Close 'X' button */
    lv_obj_t *close_btn = lv_btn_create(header);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_set_style_bg_color(close_btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(close_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, toast_close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *x_lbl = lv_label_create(close_btn);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_obj_center(x_lbl);

    /* BiDi Header Alignment */
    if (is_fa) {
        lv_obj_set_pos(badge, w - 38, 7);
        lv_obj_set_pos(cat_lbl, 45, 9);
        lv_obj_set_size(cat_lbl, w - 90, 24);
        lv_obj_set_style_text_align(cat_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_pos(close_btn, 8, 6);
    } else {
        lv_obj_set_pos(badge, 10, 7);
        lv_obj_set_pos(cat_lbl, 45, 9);
        lv_obj_set_size(cat_lbl, w - 90, 24);
        lv_obj_set_style_text_align(cat_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_pos(close_btn, w - 38, 6);
    }

    /* 4. Draining Countdown Progress Bar (Under header, starts full, empties to 0) */
    lv_obj_t *track = lv_obj_create(s_toast_container);
    lv_obj_set_pos(track, 0, 42);
    lv_obj_set_size(track, w, 4);
    lv_obj_set_style_bg_color(track, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(track, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(track, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);

    s_toast_bar = lv_obj_create(s_toast_container);
    lv_obj_set_pos(s_toast_bar, 0, 42);
    lv_obj_set_size(s_toast_bar, w, 4);
    lv_obj_set_style_bg_color(s_toast_bar, badge_col, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_toast_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_toast_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_toast_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_toast_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_toast_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 5. Subject Title (e.g. "توقف اضطراری", "تنظیم کارخانه", "هات اسپات") */
    s_toast_title_lbl = lv_label_create(s_toast_container);
    lv_obj_set_pos(s_toast_title_lbl, 16, 50);
    lv_obj_set_size(s_toast_title_lbl, w - 32, 24);
    lv_obj_set_style_text_font(s_toast_title_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_toast_title_lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_toast_title_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_toast_title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(s_toast_title_lbl, title ? title : "");
    lv_obj_add_flag(s_toast_title_lbl, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* 6. Explanation Body ("توضیح که بزار") */
    s_toast_msg_lbl = lv_label_create(s_toast_container);
    lv_obj_set_pos(s_toast_msg_lbl, 16, 76);
    lv_obj_set_size(s_toast_msg_lbl, w - 32, 52);
    lv_obj_set_style_text_font(s_toast_msg_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_toast_msg_lbl, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_toast_msg_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_toast_msg_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(s_toast_msg_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_toast_msg_lbl, msg ? msg : "");
    lv_obj_add_flag(s_toast_msg_lbl, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* 7. Bottom Confirmation & Dismiss Button */
    const char *btn_ok_text = (lang == 1) ? "OK / Close" :
                              (lang == 2) ? "Tamam / Kapat" :
                              (lang == 3) ? "Zakryt'" : "تایید و بستن";
    lv_obj_t *ok_btn = lv_btn_create(s_toast_container);
    lv_obj_set_pos(ok_btn, (w - 140) / 2, 136);
    lv_obj_set_size(ok_btn, 140, 36);
    lv_obj_set_style_bg_color(ok_btn, badge_col, LV_PART_MAIN);
    lv_obj_set_style_radius(ok_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ok_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(ok_btn, toast_close_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, btn_ok_text);
    lv_obj_set_style_text_font(ok_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ok_lbl);

    /* Bring full dialog forward */
    lv_obj_move_foreground(s_toast_backdrop);
    lv_obj_move_foreground(s_toast_container);

    s_total_duration_ms = duration_ms;
    s_remaining_ms = duration_ms;
    s_last_tick_ms = millis();
    s_is_paused = false;
}

void ui_toast_poll(void) {
    if (!s_toast_container) return;

    uint32_t now = millis();
    uint32_t dt = now - s_last_tick_ms;
    s_last_tick_ms = now;

    if (s_is_paused) {
        /* User holds finger on message: timer is frozen, bar does not shrink */
        return;
    }

    if (dt >= s_remaining_ms) {
        ui_toast_dismiss();
        return;
    }
    s_remaining_ms -= dt;

    /* Draining progress bar: shrinks smoothly from full width (390px) to 0 */
    if (s_toast_bar) {
        int current_w = (int)((390ULL * s_remaining_ms) / s_total_duration_ms);
        if (current_w < 0) current_w = 0;
        if (current_w != s_last_toast_w) {
            s_last_toast_w = current_w;
            lv_obj_set_width(s_toast_bar, current_w);
        }
    }
}
