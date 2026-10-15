#include "ui_WiFiScreen.h"
#include "ui.h"
#include "lang.h"
#include "ota_manager.h"
#include "sound.h"

lv_obj_t *ui_WiFiScreen = NULL;

static lv_obj_t *s_wifi_header = NULL;
static lv_obj_t *s_wifi_title_lbl = NULL;
static lv_obj_t *s_wifi_back_btn = NULL;

static lv_obj_t *s_wifi_status_card = NULL;
static lv_obj_t *s_wifi_status_lbl = NULL;
static lv_obj_t *s_wifi_scan_btn = NULL;
static lv_obj_t *s_wifi_scan_lbl = NULL;
static lv_obj_t *s_wifi_scan_ico = NULL;
static lv_obj_t *s_wifi_spinner = NULL;

static lv_obj_t *s_wifi_dd_card = NULL;
static lv_obj_t *s_wifi_dd_lbl = NULL;
static lv_obj_t *s_wifi_dd = NULL;
static lv_obj_t *s_wifi_connect_btn = NULL;
static lv_obj_t *s_wifi_connect_lbl = NULL;

// Password Modal
static lv_obj_t *s_wifi_pass_modal = NULL;
static lv_obj_t *s_wifi_pass_card = NULL;
static lv_obj_t *s_wifi_pass_title = NULL;
static lv_obj_t *s_wifi_pass_ta = NULL;
static lv_obj_t *s_wifi_pass_eye_btn = NULL;
static lv_obj_t *s_wifi_pass_eye_lbl = NULL;
static lv_obj_t *s_wifi_pass_kb = NULL;
static lv_obj_t *s_wifi_pass_close_btn = NULL;

static bool s_pass_visible = false;
static char s_selected_ssid[33] = {0};
static lv_obj_t *s_wifi_prev_scr = NULL;

// ── Ergonomic Industrial WiFi Keypad Maps (Apple HIG & Siemens Zero-Defect) ──
// Row 1: 10 Digits (0-9) directly accessible without mode switching
// Row 2: 10 Letters (q-p / Q-P)
// Row 3: 9 Letters (a-l / A-L) + Backspace
// Row 4: Mode switch + 7 Letters (z-m / Z-M) + Symbol switch + Space + OK

static const char * const wifi_kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_BACKSPACE, "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", "1#", " ", LV_SYMBOL_OK, ""
};

static const lv_btnmatrix_ctrl_t wifi_kb_ctrl_lc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BTN_FLAGS | 2),
    (LV_KEYBOARD_CTRL_BTN_FLAGS | 2), 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BTN_FLAGS | 2), 3, (LV_KEYBOARD_CTRL_BTN_FLAGS | 3)
};

static const char * const wifi_kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_BACKSPACE, "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", "1#", " ", LV_SYMBOL_OK, ""
};

static const lv_btnmatrix_ctrl_t wifi_kb_ctrl_uc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BTN_FLAGS | 2),
    (LV_KEYBOARD_CTRL_BTN_FLAGS | 2), 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BTN_FLAGS | 2), 3, (LV_KEYBOARD_CTRL_BTN_FLAGS | 3)
};

static const char * const wifi_kb_map_spec[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "\n",
    "-", "_", "=", "+", "[", "]", "{", "}", "\\", LV_SYMBOL_BACKSPACE, "\n",
    "abc", ":", ";", "\"", "'", "<", ">", ",", ".", "/", " ", LV_SYMBOL_OK, ""
};

static const lv_btnmatrix_ctrl_t wifi_kb_ctrl_spec[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BTN_FLAGS | 2),
    (LV_KEYBOARD_CTRL_BTN_FLAGS | 2), 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, (LV_KEYBOARD_CTRL_BTN_FLAGS | 3)
};

static void wifi_keyboard_draw_part_event_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc && dsc->part == LV_PART_ITEMS) {
        lv_obj_t * kb = lv_event_get_target(e);
        const char * txt = lv_btnmatrix_get_btn_text(kb, dsc->id);
        if(txt) {
            uint16_t sel = lv_btnmatrix_get_selected_btn(kb);
            bool is_pressed = (sel == dsc->id) && (lv_obj_get_state(kb) & LV_STATE_PRESSED);

            if(strcmp(txt, LV_SYMBOL_OK) == 0) {
                // Confirm button: Vivid Emerald Green background, white symbol
                dsc->rect_dsc->bg_color = is_pressed ? lv_color_hex(0x059669) : lv_color_hex(0x10B981);
                dsc->rect_dsc->bg_opa = LV_OPA_COVER;
                dsc->label_dsc->color = lv_color_hex(0xFFFFFF);
            }
            else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                // Backspace button: Vivid Red background, white symbol
                dsc->rect_dsc->bg_color = is_pressed ? lv_color_hex(0xDC2626) : lv_color_hex(0xEF4444);
                dsc->rect_dsc->bg_opa = LV_OPA_COVER;
                dsc->label_dsc->color = lv_color_hex(0xFFFFFF);
            }
            else if(strcmp(txt, "ABC") == 0 || strcmp(txt, "abc") == 0 || strcmp(txt, "1#") == 0) {
                // Mode switch keys: Apple Blue tint
                dsc->rect_dsc->bg_color = is_pressed ? lv_color_hex(0x0284C7) : lv_color_hex(0x2563EB);
                dsc->rect_dsc->bg_opa = LV_OPA_COVER;
                dsc->label_dsc->color = lv_color_hex(0xFFFFFF);
            }
            else if(is_pressed) {
                dsc->rect_dsc->bg_color = lv_color_hex(0x3F3F46);
            }
        }
    }
}

static void wifi_back_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_wifi_prev_scr && lv_obj_is_valid(s_wifi_prev_scr)) {
        lv_scr_load(s_wifi_prev_scr);
    } else if (ui_SettingPage && lv_obj_is_valid(ui_SettingPage)) {
        lv_scr_load(ui_SettingPage);
    } else if (ui_Menu && lv_obj_is_valid(ui_Menu)) {
        lv_scr_load(ui_Menu);
    }
}

static void wifi_scan_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    otaManager.scanAsync();
    if (s_wifi_spinner) lv_obj_clear_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_scan_ico) lv_obj_add_flag(s_wifi_scan_ico, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_status_lbl) {
        lv_label_set_text(s_wifi_status_lbl, "در حال جستجوی شبکه‌های وای‌فای...");
        lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void wifi_eye_toggle_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    s_pass_visible = !s_pass_visible;
    if (s_wifi_pass_ta) {
        lv_textarea_set_password_mode(s_wifi_pass_ta, !s_pass_visible);
    }
    if (s_wifi_pass_eye_lbl) {
        lv_label_set_text(s_wifi_pass_eye_lbl, s_pass_visible ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
    }
}

static void wifi_pass_close_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_wifi_pass_modal) {
        lv_obj_add_flag(s_wifi_pass_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void wifi_start_connection(void) {
    const char *pass = lv_textarea_get_text(s_wifi_pass_ta);
    otaManager.connectAsync(s_selected_ssid, pass ? pass : "");
}

static void wifi_kb_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        sound_play_c(SOUND_CLICK);
        wifi_start_connection();
    } else if (code == LV_EVENT_CANCEL) {
        sound_play_c(SOUND_CLICK);
        if (s_wifi_pass_modal) lv_obj_add_flag(s_wifi_pass_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void wifi_connect_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (!s_wifi_dd) return;

    char sel_str[64] = {0};
    lv_dropdown_get_selected_str(s_wifi_dd, sel_str, sizeof(sel_str));
    if (strlen(sel_str) == 0 || strcmp(sel_str, "---") == 0 || strstr(sel_str, "...") != NULL) return;

    strncpy(s_selected_ssid, sel_str, sizeof(s_selected_ssid) - 1);
    s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';

    // Show Password Modal
    if (s_wifi_pass_modal) {
        if (s_wifi_pass_title) {
            char tbuf[80];
            snprintf(tbuf, sizeof(tbuf), "%s: %s", tr(STR_WIFI_PASS_PROMPT), s_selected_ssid);
            lv_label_set_text(s_wifi_pass_title, tbuf);
            lv_obj_set_style_text_color(s_wifi_pass_title, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (s_wifi_pass_ta) {
            String saved_pass = storage.getWifiPassword();
            if (storage.getWifiSsid() == String(s_selected_ssid)) {
                lv_textarea_set_text(s_wifi_pass_ta, saved_pass.c_str());
            } else {
                lv_textarea_set_text(s_wifi_pass_ta, "");
            }
        }
        if (s_wifi_pass_kb) {
            lv_keyboard_set_mode(s_wifi_pass_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        }
        lv_obj_clear_flag(s_wifi_pass_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_WiFiScreen_screen_init(void) {
    if (ui_WiFiScreen) return;

    ui_WiFiScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_WiFiScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_WiFiScreen, lv_color_hex(0x18181B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_WiFiScreen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Header (48px) ──
    s_wifi_header = lv_obj_create(ui_WiFiScreen);
    lv_obj_set_size(s_wifi_header, 480, 48);
    lv_obj_set_pos(s_wifi_header, 0, 0);
    lv_obj_set_style_bg_color(s_wifi_header, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_wifi_header, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_wifi_header, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(s_wifi_header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_wifi_header, LV_OBJ_FLAG_SCROLLABLE);

    s_wifi_title_lbl = lv_label_create(s_wifi_header);
    lv_obj_align(s_wifi_title_lbl, LV_ALIGN_CENTER, 14, 0);
    lv_label_set_text(s_wifi_title_lbl, tr(STR_WIFI_NETWORKS));
    lv_obj_set_style_text_color(s_wifi_title_lbl, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_title_lbl, &ui_font_Dana_Bold18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *s_wifi_title_ico = lv_label_create(s_wifi_header);
    lv_label_set_text(s_wifi_title_ico, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(s_wifi_title_ico, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_wifi_title_ico, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align_to(s_wifi_title_ico, s_wifi_title_lbl, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    s_wifi_back_btn = lv_btn_create(s_wifi_header);
    lv_obj_set_size(s_wifi_back_btn, 92, 36);
    lv_obj_align(s_wifi_back_btn, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_color(s_wifi_back_btn, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_wifi_back_btn, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_wifi_back_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_wifi_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_wifi_back_btn, wifi_back_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_lbl = lv_label_create(s_wifi_back_btn);
    lv_label_set_text(back_lbl, tr(STR_BACK));
    lv_obj_center(back_lbl);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(back_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Status Card (Row 1: Y=54, H=64) ──
    s_wifi_status_card = lv_obj_create(ui_WiFiScreen);
    lv_obj_set_size(s_wifi_status_card, 460, 64);
    lv_obj_set_pos(s_wifi_status_card, 10, 54);
    lv_obj_set_style_bg_color(s_wifi_status_card, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_status_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_wifi_status_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_wifi_status_card, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_wifi_status_card, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_wifi_status_card, LV_OBJ_FLAG_SCROLLABLE);

    s_wifi_status_lbl = lv_label_create(s_wifi_status_card);
    lv_obj_set_width(s_wifi_status_lbl, 310);
    lv_label_set_long_mode(s_wifi_status_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_base_dir(s_wifi_status_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(s_wifi_status_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_wifi_status_lbl, LV_ALIGN_LEFT_MID, 8, 0);
    lv_label_set_text(s_wifi_status_lbl, "در حال جستجوی شبکه‌های وای‌فای...");
    lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_status_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Search button with integrated spinner and static text (no wrapping or collision)
    s_wifi_scan_btn = lv_btn_create(s_wifi_status_card);
    lv_obj_set_size(s_wifi_scan_btn, 118, 44);
    lv_obj_align(s_wifi_scan_btn, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(s_wifi_scan_btn, lv_color_hex(0x0284C7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_wifi_scan_btn, lv_color_hex(0x0369A1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_wifi_scan_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_wifi_scan_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_wifi_scan_btn, wifi_scan_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_wifi_scan_ico = lv_label_create(s_wifi_scan_btn);
    lv_label_set_text(s_wifi_scan_ico, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(s_wifi_scan_ico, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_wifi_scan_ico, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_wifi_scan_ico, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_flag(s_wifi_scan_ico, LV_OBJ_FLAG_HIDDEN); // Initially hidden while auto-scanning

    s_wifi_spinner = lv_spinner_create(s_wifi_scan_btn, 1000, 60);
    lv_obj_set_size(s_wifi_spinner, 20, 20);
    lv_obj_align(s_wifi_spinner, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_arc_color(s_wifi_spinner, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_wifi_spinner, lv_color_hex(0x0369A1), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_wifi_spinner, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_wifi_spinner, 3, LV_PART_MAIN);

    s_wifi_scan_lbl = lv_label_create(s_wifi_scan_btn);
    lv_label_set_text(s_wifi_scan_lbl, tr(STR_WIFI_SCAN));
    lv_obj_align(s_wifi_scan_lbl, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(s_wifi_scan_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_scan_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Network Selection Card (Row 2: Y=124, H=188) ──
    s_wifi_dd_card = lv_obj_create(ui_WiFiScreen);
    lv_obj_set_size(s_wifi_dd_card, 460, 188);
    lv_obj_set_pos(s_wifi_dd_card, 10, 124);
    lv_obj_set_style_bg_color(s_wifi_dd_card, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_dd_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_wifi_dd_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_wifi_dd_card, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_wifi_dd_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_wifi_dd_card, LV_OBJ_FLAG_SCROLLABLE);

    s_wifi_dd_lbl = lv_label_create(s_wifi_dd_card);
    lv_obj_align(s_wifi_dd_lbl, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_label_set_text(s_wifi_dd_lbl, "انتخاب شبکه وای فای کارگاه:");
    lv_obj_set_style_text_color(s_wifi_dd_lbl, lv_color_hex(0xA1A1AA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_dd_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(s_wifi_dd_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_wifi_dd = lv_dropdown_create(s_wifi_dd_card);
    lv_obj_set_size(s_wifi_dd, 440, 46);
    lv_obj_align(s_wifi_dd, LV_ALIGN_TOP_MID, 0, 26);
    lv_dropdown_set_options(s_wifi_dd, "در حال جستجوی شبکه‌ها...");
    lv_dropdown_set_symbol(s_wifi_dd, NULL); // ZERO TOFU: Eliminates missing glyph box
    lv_obj_set_style_bg_color(s_wifi_dd, lv_color_hex(0x18181B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_wifi_dd, lv_color_hex(0x52525B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_wifi_dd, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_dd, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_wifi_dd, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_dd, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(s_wifi_dd, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *dd_list = lv_dropdown_get_list(s_wifi_dd);
    if (dd_list) {
        lv_obj_set_style_text_font(dd_list, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(dd_list, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(dd_list, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(dd_list, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_max_height(dd_list, 160, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(dd_list, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    s_wifi_connect_btn = lv_btn_create(s_wifi_dd_card);
    lv_obj_set_size(s_wifi_connect_btn, 440, 50);
    lv_obj_align(s_wifi_connect_btn, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(s_wifi_connect_btn, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_wifi_connect_btn, lv_color_hex(0x059669), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_wifi_connect_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_wifi_connect_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_wifi_connect_btn, wifi_connect_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cico = lv_label_create(s_wifi_connect_btn);
    lv_label_set_text(cico, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(cico, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cico, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(cico, LV_ALIGN_LEFT_MID, 20, 0);

    s_wifi_connect_lbl = lv_label_create(s_wifi_connect_btn);
    lv_label_set_text(s_wifi_connect_lbl, tr(STR_WIFI_CONNECT));
    lv_obj_center(s_wifi_connect_lbl);
    lv_obj_set_style_text_color(s_wifi_connect_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_connect_lbl, &ui_font_Dana_Bold18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Password Modal Overlay ──
    s_wifi_pass_modal = lv_obj_create(ui_WiFiScreen);
    lv_obj_set_size(s_wifi_pass_modal, 480, 320);
    lv_obj_set_pos(s_wifi_pass_modal, 0, 0);
    lv_obj_set_style_bg_color(s_wifi_pass_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_wifi_pass_modal, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_wifi_pass_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_wifi_pass_modal, LV_OBJ_FLAG_HIDDEN);

    s_wifi_pass_card = lv_obj_create(s_wifi_pass_modal);
    lv_obj_set_size(s_wifi_pass_card, 474, 312);
    lv_obj_align(s_wifi_pass_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_wifi_pass_card, lv_color_hex(0x18181B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_pass_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_wifi_pass_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_wifi_pass_card, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_wifi_pass_card, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_wifi_pass_card, LV_OBJ_FLAG_SCROLLABLE);

    s_wifi_pass_title = lv_label_create(s_wifi_pass_card);
    lv_obj_align(s_wifi_pass_title, LV_ALIGN_TOP_LEFT, 10, 8);
    lv_label_set_text(s_wifi_pass_title, tr(STR_WIFI_PASS_PROMPT));
    lv_obj_set_style_text_color(s_wifi_pass_title, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_pass_title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Circular Red Close Button with white 'X'
    s_wifi_pass_close_btn = lv_btn_create(s_wifi_pass_card);
    lv_obj_set_size(s_wifi_pass_close_btn, 36, 36);
    lv_obj_align(s_wifi_pass_close_btn, LV_ALIGN_TOP_RIGHT, -4, 2);
    lv_obj_set_style_bg_color(s_wifi_pass_close_btn, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_wifi_pass_close_btn, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_wifi_pass_close_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_wifi_pass_close_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_wifi_pass_close_btn, wifi_pass_close_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clbl = lv_label_create(s_wifi_pass_close_btn);
    lv_label_set_text(clbl, LV_SYMBOL_CLOSE);
    lv_obj_center(clbl);
    lv_obj_set_style_text_font(clbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(clbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Textarea + Eye button
    s_wifi_pass_ta = lv_textarea_create(s_wifi_pass_card);
    lv_obj_set_size(s_wifi_pass_ta, 404, 40);
    lv_obj_align(s_wifi_pass_ta, LV_ALIGN_TOP_LEFT, 8, 42);
    lv_textarea_set_password_mode(s_wifi_pass_ta, true);
    lv_textarea_set_one_line(s_wifi_pass_ta, true);
    lv_obj_set_style_bg_color(s_wifi_pass_ta, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_wifi_pass_ta, lv_color_hex(0xF9FAFB), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_wifi_pass_ta, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_wifi_pass_ta, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_pass_ta, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_wifi_pass_eye_btn = lv_btn_create(s_wifi_pass_card);
    lv_obj_set_size(s_wifi_pass_eye_btn, 44, 40);
    lv_obj_align(s_wifi_pass_eye_btn, LV_ALIGN_TOP_RIGHT, -4, 42);
    lv_obj_set_style_bg_color(s_wifi_pass_eye_btn, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_wifi_pass_eye_btn, lv_color_hex(0x52525B), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_wifi_pass_eye_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_wifi_pass_eye_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_wifi_pass_eye_btn, wifi_eye_toggle_cb, LV_EVENT_CLICKED, NULL);

    s_wifi_pass_eye_lbl = lv_label_create(s_wifi_pass_eye_btn);
    lv_label_set_text(s_wifi_pass_eye_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_center(s_wifi_pass_eye_lbl);
    lv_obj_set_style_text_font(s_wifi_pass_eye_lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_wifi_pass_eye_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // On-screen Virtual Keyboard: Generous 464x216px
    s_wifi_pass_kb = lv_keyboard_create(s_wifi_pass_card);
    lv_obj_set_size(s_wifi_pass_kb, 464, 216);
    lv_obj_align(s_wifi_pass_kb, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_keyboard_set_textarea(s_wifi_pass_kb, s_wifi_pass_ta);
    lv_keyboard_set_map(s_wifi_pass_kb, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)wifi_kb_map_lc, wifi_kb_ctrl_lc);
    lv_keyboard_set_map(s_wifi_pass_kb, LV_KEYBOARD_MODE_TEXT_UPPER, (const char **)wifi_kb_map_uc, wifi_kb_ctrl_uc);
    lv_keyboard_set_map(s_wifi_pass_kb, LV_KEYBOARD_MODE_SPECIAL, (const char **)wifi_kb_map_spec, wifi_kb_ctrl_spec);
    lv_keyboard_set_mode(s_wifi_pass_kb, LV_KEYBOARD_MODE_TEXT_LOWER);

    lv_obj_set_style_text_font(s_wifi_pass_kb, &lv_font_montserrat_18, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_pass_kb, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_wifi_pass_kb, 6, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_wifi_pass_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(s_wifi_pass_kb, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(s_wifi_pass_kb, wifi_keyboard_draw_part_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_obj_add_event_cb(s_wifi_pass_kb, wifi_kb_event_cb, LV_EVENT_ALL, NULL);
}

void ui_WiFiScreen_open(void) {
    s_wifi_prev_scr = lv_scr_act();
    ui_WiFiScreen_screen_init();
    lv_scr_load(ui_WiFiScreen);

    // Automatically trigger a scan whenever the screen is opened
    otaManager.scanAsync();
    if (s_wifi_spinner) lv_obj_clear_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_scan_ico) lv_obj_add_flag(s_wifi_scan_ico, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_status_lbl) {
        lv_label_set_text(s_wifi_status_lbl, "در حال جستجوی شبکه‌های وای‌فای...");
        lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_WiFiScreen_update_tick(void) {
    if (!ui_WiFiScreen || lv_scr_act() != ui_WiFiScreen) return;

    // 1. Update Connection Status
    if (s_wifi_status_lbl) {
        if (otaManager.is_connecting) {
            lv_label_set_text(s_wifi_status_lbl, tr(STR_WIFI_CONNECTING));
            lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xFBBF24), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (otaManager.is_connected && WiFi.status() == WL_CONNECTED) {
            char sbuf[80];
            snprintf(sbuf, sizeof(sbuf), "متصل به: %s (%s)", otaManager.current_ssid, otaManager.connected_ip);
            lv_label_set_text(s_wifi_status_lbl, sbuf);
            lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (otaManager.is_scanning) {
            lv_label_set_text(s_wifi_status_lbl, "در حال جستجوی شبکه‌های وای‌فای...");
            lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_label_set_text(s_wifi_status_lbl, "وضعیت: قطع ارتباط شبکه");
            lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0x9CA3AF), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // 2. Update Scan Results
    if (otaManager.scan_done) {
        otaManager.scan_done = false;
        if (s_wifi_spinner) lv_obj_add_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
        if (s_wifi_scan_ico) lv_obj_clear_flag(s_wifi_scan_ico, LV_OBJ_FLAG_HIDDEN);
        if (s_wifi_scan_lbl) lv_label_set_text(s_wifi_scan_lbl, tr(STR_WIFI_SCAN));

        if (s_wifi_dd) {
            String opts = "";
            for (int i = 0; i < otaManager.scanned_count; i++) {
                if (strlen(otaManager.scanned_ssids[i]) > 0) {
                    if (opts.length() > 0) opts += "\n";
                    opts += otaManager.scanned_ssids[i];
                }
            }
            if (opts.length() == 0) {
                opts = "هیچ شبکه‌ای یافت نشد";
            }
            lv_dropdown_set_options(s_wifi_dd, opts.c_str());
        }
    }

    // 3. Handle Wrong Password Error Notification
    if (otaManager.wrong_password_detected) {
        otaManager.wrong_password_detected = false;
        if (s_wifi_pass_title) {
            lv_label_set_text(s_wifi_pass_title, "رمز اشتباه است! مجدداً وارد کنید");
            lv_obj_set_style_text_color(s_wifi_pass_title, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        sound_play_c(SOUND_ALARM_FAULT);
    }
}
