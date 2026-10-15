#include "ui_ProjectInfoScreen.h"
#include "ui_WiFiScreen.h"
#include "ui.h"
#include "lang.h"
#include "ota_manager.h"
#include "sound.h"
#include <WiFi.h>

lv_obj_t *ui_ProjectInfoScreen = NULL;

// Header
static lv_obj_t *s_info_header = NULL;
static lv_obj_t *s_info_title_lbl = NULL;
static lv_obj_t *s_info_back_btn = NULL;
static lv_obj_t *s_info_wifi_btn = NULL;

// Scrollable Cards Container (Horizontal Carousel)
static lv_obj_t *s_scroll_container = NULL;

// Card 0: HAFFLER Brand & Direct Link Entry
static lv_obj_t *s_card_brand = NULL;
static lv_obj_t *s_lbl_brand_ver = NULL;
static lv_obj_t *s_btn_link_entry = NULL;
static lv_obj_t *s_btn_brand_back = NULL;

// Card 1: Bakery Dough Divider (نان چانه‌گیر)
static lv_obj_t *s_card_bakery = NULL;
static lv_obj_t *s_lbl_bakery_title = NULL;
static lv_obj_t *s_img_bakery = NULL;
static lv_obj_t *s_lbl_bakery_ver = NULL;
static lv_obj_t *s_btn_bakery_info = NULL;
static lv_obj_t *s_btn_bakery_install = NULL;
static lv_obj_t *s_lbl_bakery_install = NULL;

// Card 2: Hardware Config & Diagnostic Test (کانفیگ و تست سخت‌افزار)
static lv_obj_t *s_card_config = NULL;
static lv_obj_t *s_lbl_config_title = NULL;
static lv_obj_t *s_img_config = NULL;
static lv_obj_t *s_lbl_config_ver = NULL;
static lv_obj_t *s_btn_config_info = NULL;
static lv_obj_t *s_btn_config_install = NULL;
static lv_obj_t *s_lbl_config_install = NULL;

// Modal 1: Custom URL Entry ("ورود لینک")
static lv_obj_t *s_modal_link = NULL;
static lv_obj_t *s_btn_link_back = NULL;
static lv_obj_t *s_btn_link_clear = NULL;
static lv_obj_t *s_btn_link_default = NULL;
static lv_obj_t *s_btn_link_confirm = NULL;
static lv_obj_t *s_ta_link = NULL;
static lv_obj_t *s_kb_link = NULL;
static lv_obj_t *s_lbl_link_err = NULL;

// Modal 2: Specs & Hardware Pinout ("مشخصات و نقشه پایه‌ها")
static lv_obj_t *s_modal_specs = NULL;
static lv_obj_t *s_lbl_specs_title = NULL;
static lv_obj_t *s_lbl_specs_ver = NULL;
static lv_obj_t *s_lbl_specs_chg = NULL;
static lv_obj_t *s_lbl_specs_pins_in = NULL;
static lv_obj_t *s_lbl_specs_pins_out = NULL;
static lv_obj_t *s_btn_specs_close = NULL;
static lv_obj_t *s_btn_specs_install = NULL;

// Modal 3: Confirmation Modal
static lv_obj_t *s_modal_confirm = NULL;
static lv_obj_t *s_lbl_confirm_msg = NULL;
static lv_obj_t *s_btn_confirm_ok = NULL;
static lv_obj_t *s_btn_confirm_cancel = NULL;

// Modal 4: Download & Flash Progress
static lv_obj_t *s_modal_progress = NULL;
static lv_obj_t *s_bar_progress = NULL;
static lv_obj_t *s_lbl_progress_pct = NULL;
static lv_obj_t *s_lbl_progress_sub = NULL;

// State Tracking
static lv_obj_t *s_info_prev_scr = NULL;
static int s_target_prog = 0; // 0 = Bakery, 1 = Config-Test, 2 = Custom URL
static char s_custom_url_buf[256] = "";

// Keyboard Maps for Custom URL Entry
static const char * s_url_kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "/", "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", ".", "-", "\n",
    "https://", ":", "%", "=", "_", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t s_url_kb_ctrl_lc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 2, 2
};

static const char * s_url_kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "/", "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", ".", "-", "\n",
    "https://", ":", "%", "=", "_", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t s_url_kb_ctrl_uc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 2, 2
};

// ── Callbacks ────────────────────────────────────────────────────────────────

static void info_back_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_info_prev_scr && lv_obj_is_valid(s_info_prev_scr)) {
        lv_scr_load(s_info_prev_scr);
    } else if (ui_SettingPage && lv_obj_is_valid(ui_SettingPage)) {
        lv_scr_load(ui_SettingPage);
    } else if (ui_Menu && lv_obj_is_valid(ui_Menu)) {
        lv_scr_load(ui_Menu);
    }
}

static void info_wifi_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    ui_WiFiScreen_open();
}

static void open_confirmation_modal(int prog_type) {
    s_target_prog = prog_type;
    char buf[256];
    if (prog_type == 0) {
        snprintf(buf, sizeof(buf), "آیا از دانلود و نصب فریم‌ور نان چانه‌گیر (%s) اطمینان دارید؟", 
                 otaManager.release_info.available ? otaManager.release_info.version : "آخرین نسخه");
    } else if (prog_type == 1) {
        snprintf(buf, sizeof(buf), "آیا از دانلود و نصب نرم‌افزار کانفیگ و تست سخت‌افزار (%s) اطمینان دارید؟",
                 otaManager.release_info_config.available ? otaManager.release_info_config.version : "آخرین نسخه");
    } else {
        snprintf(buf, sizeof(buf), "آیا از دانلود و نصب فریم‌ور از آدرس دلخواه اطمینان دارید؟\n%.60s...", s_custom_url_buf);
    }
    if (s_lbl_confirm_msg) lv_label_set_text(s_lbl_confirm_msg, buf);
    if (s_modal_confirm) lv_obj_clear_flag(s_modal_confirm, LV_OBJ_FLAG_HIDDEN);
}

static void bakery_install_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_notification(NOTIF_WARNING, "ارتقا سرور", "دستگاه به اینترنت متصل نیست");
        return;
    }
    open_confirmation_modal(0);
}

static void config_install_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_notification(NOTIF_WARNING, "ارتقا سرور", "دستگاه به اینترنت متصل نیست");
        return;
    }
    open_confirmation_modal(1);
}

static void open_specs_modal(int prog_type) {
    s_target_prog = prog_type;
    FwReleaseInfo &rel = (prog_type == 1) ? otaManager.release_info_config : otaManager.release_info;
    int lang_idx = (int)g_lang;

    if (s_lbl_specs_title) {
        lv_label_set_text(s_lbl_specs_title, (prog_type == 1) ? tr(STR_CONFIG_TEST_TITLE) : tr(STR_CHONE_GIR_TITLE));
    }

    if (s_lbl_specs_ver) {
        char vbuf[128];
        String curVer = storage.getVersion();
        if (rel.available) {
            snprintf(vbuf, sizeof(vbuf), "نسخه سرور: %s  |  نسخه دستگاه: %s  |  تاریخ: %s",
                     rel.version, curVer.c_str(), rel.build_date[0] ? rel.build_date : "--");
        } else {
            snprintf(vbuf, sizeof(vbuf), "نسخه دستگاه: %s  (استعلام آنلاین از سرور انجام نشده)", curVer.c_str());
        }
        lv_label_set_text(s_lbl_specs_ver, vbuf);
    }

    // Changelog Rollup
    if (s_lbl_specs_chg) {
        String chg = "";
        int cnt = 0;
        char (*lines)[128] = NULL;
        if (lang_idx == 0) { cnt = rel.desc_fa_count; lines = rel.desc_fa; }
        else if (lang_idx == 1) { cnt = rel.desc_en_count; lines = rel.desc_en; }
        else if (lang_idx == 2) { cnt = rel.desc_tr_count; lines = rel.desc_tr; }
        else { cnt = (rel.desc_en_count > 0) ? rel.desc_en_count : rel.desc_fa_count; lines = (rel.desc_en_count > 0) ? rel.desc_en : rel.desc_fa; }

        if (cnt > 0 && lines) {
            for (int i = 0; i < cnt; i++) {
                char clean[256];
                OtaManager::sanitizeText(clean, sizeof(clean), lines[i], lang_idx);
                if (clean[0]) {
                    if (chg.length() > 0) chg += "\n";
                    chg += "• ";
                    chg += clean;
                }
            }
        } else {
            chg = (lang_idx == 0) ? "• پایداری قطعی رندر و زمان‌بندی موتورها\n• هماهنگ‌سازی خودکار ساعت اینترنت NTP\n• کیبورد و دکمه بازگشت یکپارچه صنعتی"
                                  : "• Rock-solid motor timing & FreeRTOS stability\n• Automatic NTP Internet clock synchronization\n• Unified industrial keyboard & red back buttons";
        }
        lv_label_set_text(s_lbl_specs_chg, chg.c_str());
    }

    // Inputs (Physical Bakery Hardware Only - Zero M pins!)
    if (s_lbl_specs_pins_in) {
        String in_str = (lang_idx == 0) ? "• سنسور هوم پیستون (GPIO 7)\n• سنسور هوم قیف (GPIO 3)\n• صفحه کلید پنل F (GPIO 1)\n• صفحه کلید پنل M (GPIO 2)"
                                        : "• Piston Home Sensor (GPIO 7)\n• Funnel Home Sensor (GPIO 3)\n• Keypad Panel F (GPIO 1)\n• Keypad Panel M (GPIO 2)";
        lv_label_set_text(s_lbl_specs_pins_in, in_str.c_str());
    }

    // Outputs (Physical Bakery Hardware Only - Zero M pins!)
    if (s_lbl_specs_pins_out) {
        String out_str = (lang_idx == 0) ? "• رله موتور پیستون (GPIO 36)\n• رله موتور قیف (GPIO 42)\n• اینورتر جلو FWD (GPIO 39)\n• اینورتر معکوس REV (GPIO 48)"
                                         : "• Piston Motor Relay (GPIO 36)\n• Funnel Motor Relay (GPIO 42)\n• Inverter FWD (GPIO 39)\n• Inverter REV (GPIO 48)";
        lv_label_set_text(s_lbl_specs_pins_out, out_str.c_str());
    }

    if (s_modal_specs) lv_obj_clear_flag(s_modal_specs, LV_OBJ_FLAG_HIDDEN);
}

static void bakery_info_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    open_specs_modal(0);
}

static void config_info_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    open_specs_modal(1);
}

static void specs_close_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_modal_specs) lv_obj_add_flag(s_modal_specs, LV_OBJ_FLAG_HIDDEN);
}

static void specs_install_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_modal_specs) lv_obj_add_flag(s_modal_specs, LV_OBJ_FLAG_HIDDEN);
    open_confirmation_modal(s_target_prog);
}

// ── Custom Link Modal Callbacks ──────────────────────────────────────────────

static void link_entry_open_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_lbl_link_err) lv_obj_add_flag(s_lbl_link_err, LV_OBJ_FLAG_HIDDEN);
    if (s_modal_link) lv_obj_clear_flag(s_modal_link, LV_OBJ_FLAG_HIDDEN);
}

static void link_entry_cancel_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_modal_link) lv_obj_add_flag(s_modal_link, LV_OBJ_FLAG_HIDDEN);
}

static void link_entry_clear_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_ta_link) lv_textarea_set_text(s_ta_link, "");
}

static void link_entry_default_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_ta_link) {
        lv_textarea_set_text(s_ta_link, OTA_BAKERY_BIN_HTTPS);
    }
}

static void link_entry_confirm_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (!s_ta_link) return;
    const char *txt = lv_textarea_get_text(s_ta_link);
    if (!txt || strlen(txt) < 8 || (!strstr(txt, "http://") && !strstr(txt, "https://"))) {
        if (s_lbl_link_err) {
            lv_label_set_text(s_lbl_link_err, "لینک نامعتبر است! آدرس باید با http:// یا https:// شروع شود.");
            lv_obj_clear_flag(s_lbl_link_err, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    strncpy(s_custom_url_buf, txt, sizeof(s_custom_url_buf) - 1);
    if (s_modal_link) lv_obj_add_flag(s_modal_link, LV_OBJ_FLAG_HIDDEN);
    open_confirmation_modal(2);
}

static void link_keyboard_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        link_entry_confirm_cb(e);
    } else if (code == LV_EVENT_CANCEL) {
        link_entry_cancel_cb(e);
    }
}

// ── Confirmation Modal Callbacks ─────────────────────────────────────────────

static void confirm_cancel_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (s_modal_confirm) lv_obj_add_flag(s_modal_confirm, LV_OBJ_FLAG_HIDDEN);
}

static void confirm_ok_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_START_CYCLE);
    if (s_modal_confirm) lv_obj_add_flag(s_modal_confirm, LV_OBJ_FLAG_HIDDEN);
    if (s_modal_progress) lv_obj_clear_flag(s_modal_progress, LV_OBJ_FLAG_HIDDEN);

    if (s_target_prog == 2 && strlen(s_custom_url_buf) > 0) {
        otaManager.startUpdateAsync(2, s_custom_url_buf);
    } else {
        otaManager.startUpdateAsync(s_target_prog);
    }
}

// ── Populate Card Versions ───────────────────────────────────────────────────

static void populate_release_ui(void) {
    bool wifi_ok = (WiFi.status() == WL_CONNECTED);
    String curVer = storage.getVersion();

    // 1. Card 0: Brand
    if (s_lbl_brand_ver) {
        char bbuf[64];
        snprintf(bbuf, sizeof(bbuf), "نسخه دستگاه: %s", curVer.c_str());
        lv_label_set_text(s_lbl_brand_ver, bbuf);
    }

    // 2. Card 1: Bakery
    if (s_lbl_bakery_ver) {
        FwReleaseInfo &rel = otaManager.release_info;
        if (!wifi_ok) {
            lv_label_set_text(s_lbl_bakery_ver, "وای‌فای متصل نیست");
            lv_obj_set_style_text_color(s_lbl_bakery_ver, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (!rel.available) {
            lv_label_set_text(s_lbl_bakery_ver, "در حال اتصال به سرور...");
            lv_obj_set_style_text_color(s_lbl_bakery_ver, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            char vbuf[64];
            snprintf(vbuf, sizeof(vbuf), "نسخه سرور: %s (%s)", rel.version, rel.is_newer ? "جدیدتر" : "پایدار");
            lv_label_set_text(s_lbl_bakery_ver, vbuf);
            lv_obj_set_style_text_color(s_lbl_bakery_ver, lv_color_hex(rel.is_newer ? 0x10B981 : 0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // 3. Card 2: Config-Test
    if (s_lbl_config_ver) {
        FwReleaseInfo &cfg_rel = otaManager.release_info_config;
        if (!wifi_ok) {
            lv_label_set_text(s_lbl_config_ver, "وای‌فای متصل نیست");
            lv_obj_set_style_text_color(s_lbl_config_ver, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (!cfg_rel.available) {
            lv_label_set_text(s_lbl_config_ver, "در حال اتصال به سرور...");
            lv_obj_set_style_text_color(s_lbl_config_ver, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            char vbuf[64];
            snprintf(vbuf, sizeof(vbuf), "نسخه سرور: %s", cfg_rel.version);
            lv_label_set_text(s_lbl_config_ver, vbuf);
            lv_obj_set_style_text_color(s_lbl_config_ver, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

// ── Screen Initialization ────────────────────────────────────────────────────

void ui_ProjectInfoScreen_screen_init(void) {
    if (ui_ProjectInfoScreen) return;

    ui_ProjectInfoScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ProjectInfoScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_ProjectInfoScreen, lv_color_hex(0x18181B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ProjectInfoScreen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Header (48px) ──
    s_info_header = lv_obj_create(ui_ProjectInfoScreen);
    lv_obj_set_size(s_info_header, 480, 48);
    lv_obj_set_pos(s_info_header, 0, 0);
    lv_obj_set_style_bg_color(s_info_header, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_info_header, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_info_header, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(s_info_header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_info_header, LV_OBJ_FLAG_SCROLLABLE);

    s_info_title_lbl = lv_label_create(s_info_header);
    lv_obj_align(s_info_title_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(s_info_title_lbl, "مدیریت بروزرسانی و دریافت نرم‌افزار");
    lv_obj_set_style_text_color(s_info_title_lbl, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_info_title_lbl, &ui_font_Dana_Bold18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Unified Signature Crimson Red Back Button
    s_info_back_btn = lv_btn_create(s_info_header);
    lv_obj_set_size(s_info_back_btn, 76, 36);
    lv_obj_align(s_info_back_btn, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_color(s_info_back_btn, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_info_back_btn, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_info_back_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_info_back_btn, info_back_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *blbl = lv_label_create(s_info_back_btn);
    lv_label_set_text(blbl, tr(STR_BACK));
    lv_obj_center(blbl);
    lv_obj_set_style_text_color(blbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(blbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // WiFi Quick Access Button
    s_info_wifi_btn = lv_btn_create(s_info_header);
    lv_obj_set_size(s_info_wifi_btn, 105, 36);
    lv_obj_align(s_info_wifi_btn, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_style_bg_color(s_info_wifi_btn, lv_color_hex(0x0284C7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_info_wifi_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_info_wifi_btn, info_wifi_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *wlbl = lv_label_create(s_info_wifi_btn);
    lv_label_set_text(wlbl, "وای‌فای");
    lv_obj_set_style_base_dir(wlbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(wlbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(wlbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(wlbl, LV_ALIGN_CENTER, 10, 0);

    lv_obj_t *wico = lv_label_create(s_info_wifi_btn);
    lv_label_set_text(wico, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wico, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(wico, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align_to(wico, wlbl, LV_ALIGN_OUT_LEFT_MID, -6, 0);

    // ── Horizontal Carousel Container (H: 272px) ──
    s_scroll_container = lv_obj_create(ui_ProjectInfoScreen);
    lv_obj_set_size(s_scroll_container, 480, 272);
    lv_obj_set_pos(s_scroll_container, 0, 48);
    lv_obj_set_style_bg_opa(s_scroll_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_scroll_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_scroll_container, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(s_scroll_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_scroll_container, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_snap_x(s_scroll_container, LV_SCROLL_SNAP_CENTER);

    // =========================================================================
    // CARD 0: HAFFLER BRAND & CUSTOM LINK ENTRY
    // =========================================================================
    s_card_brand = lv_obj_create(s_scroll_container);
    lv_obj_set_size(s_card_brand, 226, 256);
    lv_obj_clear_flag(s_card_brand, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_card_brand, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_card_brand, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_card_brand, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_card_brand, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_card_brand, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *b_title = lv_label_create(s_card_brand);
    lv_label_set_text(b_title, "HAFFLER");
    lv_obj_align(b_title, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_color(b_title, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(b_title, &ui_font_Dana_Bold18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *b_sub = lv_label_create(s_card_brand);
    lv_label_set_text(b_sub, "سامانه هوشمند بروزرسانی");
    lv_obj_align(b_sub, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_text_color(b_sub, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(b_sub, &ui_font_Dana_Bold10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *b_line = lv_obj_create(s_card_brand);
    lv_obj_set_size(b_line, 180, 2);
    lv_obj_align(b_line, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_set_style_bg_color(b_line, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(b_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_brand_ver = lv_label_create(s_card_brand);
    lv_label_set_text(s_lbl_brand_ver, "نسخه دستگاه: V1.1.0");
    lv_obj_align(s_lbl_brand_ver, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_color(s_lbl_brand_ver, lv_color_hex(0xA1A1AA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_brand_ver, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_link_entry = lv_btn_create(s_card_brand);
    lv_obj_set_size(s_btn_link_entry, 196, 36);
    lv_obj_align(s_btn_link_entry, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_set_style_bg_color(s_btn_link_entry, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_btn_link_entry, lv_color_hex(0x059669), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_link_entry, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_link_entry, link_entry_open_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *le_lbl = lv_label_create(s_btn_link_entry);
    lv_label_set_text(le_lbl, tr(STR_LINK_ENTRY));
    lv_obj_center(le_lbl);
    lv_obj_set_style_text_font(le_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(le_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Center gear decoration
    lv_obj_t *gear_ico = lv_label_create(s_card_brand);
    lv_label_set_text(gear_ico, LV_SYMBOL_SETTINGS);
    lv_obj_align(gear_ico, LV_ALIGN_TOP_MID, 0, 142);
    lv_obj_set_style_text_font(gear_ico, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(gear_ico, lv_color_hex(0x475569), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_brand_back = lv_btn_create(s_card_brand);
    lv_obj_set_size(s_btn_brand_back, 196, 38);
    lv_obj_align(s_btn_brand_back, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(s_btn_brand_back, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_btn_brand_back, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_brand_back, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_brand_back, info_back_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *bb_lbl = lv_label_create(s_btn_brand_back);
    lv_label_set_text(bb_lbl, tr(STR_BACK));
    lv_obj_center(bb_lbl);
    lv_obj_set_style_text_font(bb_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bb_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // =========================================================================
    // CARD 1: BAKERY DOUGH DIVIDER (نان چانه‌گیر)
    // =========================================================================
    s_card_bakery = lv_obj_create(s_scroll_container);
    lv_obj_set_size(s_card_bakery, 226, 256);
    lv_obj_clear_flag(s_card_bakery, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_card_bakery, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_card_bakery, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_card_bakery, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_card_bakery, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_card_bakery, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_bakery_title = lv_label_create(s_card_bakery);
    lv_label_set_text(s_lbl_bakery_title, tr(STR_CHONE_GIR_TITLE));
    lv_obj_align(s_lbl_bakery_title, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_text_color(s_lbl_bakery_title, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_bakery_title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_img_bakery = lv_img_create(s_card_bakery);
    lv_img_set_src(s_img_bakery, &ui_img_chone_gir);
    lv_obj_align(s_img_bakery, LV_ALIGN_TOP_MID, 0, 32);

    s_lbl_bakery_ver = lv_label_create(s_card_bakery);
    lv_label_set_text(s_lbl_bakery_ver, "در حال اتصال به سرور...");
    lv_obj_align(s_lbl_bakery_ver, LV_ALIGN_TOP_MID, 0, 168);
    lv_obj_set_style_text_color(s_lbl_bakery_ver, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_bakery_ver, &ui_font_Dana_Bold10, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_bakery_info = lv_btn_create(s_card_bakery);
    lv_obj_set_size(s_btn_bakery_info, 94, 34);
    lv_obj_align(s_btn_bakery_info, LV_ALIGN_BOTTOM_LEFT, 4, -8);
    lv_obj_set_style_bg_color(s_btn_bakery_info, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_btn_bakery_info, lv_color_hex(0x475569), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_btn_bakery_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_bakery_info, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_bakery_info, bakery_info_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *bki_lbl = lv_label_create(s_btn_bakery_info);
    lv_label_set_text(bki_lbl, "مشخصات");
    lv_obj_center(bki_lbl);
    lv_obj_set_style_text_font(bki_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bki_lbl, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_bakery_install = lv_btn_create(s_card_bakery);
    lv_obj_set_size(s_btn_bakery_install, 108, 34);
    lv_obj_align(s_btn_bakery_install, LV_ALIGN_BOTTOM_RIGHT, -4, -8);
    lv_obj_set_style_bg_color(s_btn_bakery_install, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_btn_bakery_install, lv_color_hex(0x059669), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_bakery_install, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_bakery_install, bakery_install_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_lbl_bakery_install = lv_label_create(s_btn_bakery_install);
    lv_label_set_text(s_lbl_bakery_install, "نصب / ارتقا");
    lv_obj_center(s_lbl_bakery_install);
    lv_obj_set_style_text_font(s_lbl_bakery_install, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_lbl_bakery_install, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // =========================================================================
    // CARD 2: HARDWARE CONFIG & TEST (کانفیگ و تست)
    // =========================================================================
    s_card_config = lv_obj_create(s_scroll_container);
    lv_obj_set_size(s_card_config, 226, 256);
    lv_obj_clear_flag(s_card_config, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_card_config, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_card_config, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_card_config, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_card_config, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_card_config, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_config_title = lv_label_create(s_card_config);
    lv_label_set_text(s_lbl_config_title, tr(STR_CONFIG_TEST_TITLE));
    lv_obj_align(s_lbl_config_title, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_text_color(s_lbl_config_title, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_config_title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Diagnostics Badge Center
    lv_obj_t *cfg_badge = lv_obj_create(s_card_config);
    lv_obj_set_size(cfg_badge, 86, 86);
    lv_obj_align(cfg_badge, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_set_style_bg_color(cfg_badge, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cfg_badge, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cfg_badge, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cfg_badge, 43, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(cfg_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cfg_ico = lv_label_create(cfg_badge);
    lv_label_set_text(cfg_ico, LV_SYMBOL_WIFI);
    lv_obj_center(cfg_ico);
    lv_obj_set_style_text_font(cfg_ico, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cfg_ico, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_config_ver = lv_label_create(s_card_config);
    lv_label_set_text(s_lbl_config_ver, "در حال اتصال به سرور...");
    lv_obj_align(s_lbl_config_ver, LV_ALIGN_TOP_MID, 0, 168);
    lv_obj_set_style_text_color(s_lbl_config_ver, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_config_ver, &ui_font_Dana_Bold10, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_config_info = lv_btn_create(s_card_config);
    lv_obj_set_size(s_btn_config_info, 94, 34);
    lv_obj_align(s_btn_config_info, LV_ALIGN_BOTTOM_LEFT, 4, -8);
    lv_obj_set_style_bg_color(s_btn_config_info, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_btn_config_info, lv_color_hex(0x475569), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_btn_config_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_config_info, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_config_info, config_info_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cfgi_lbl = lv_label_create(s_btn_config_info);
    lv_label_set_text(cfgi_lbl, "مشخصات");
    lv_obj_center(cfgi_lbl);
    lv_obj_set_style_text_font(cfgi_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cfgi_lbl, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_config_install = lv_btn_create(s_card_config);
    lv_obj_set_size(s_btn_config_install, 108, 34);
    lv_obj_align(s_btn_config_install, LV_ALIGN_BOTTOM_RIGHT, -4, -8);
    lv_obj_set_style_bg_color(s_btn_config_install, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_btn_config_install, lv_color_hex(0xD97706), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_config_install, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_config_install, config_install_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_lbl_config_install = lv_label_create(s_btn_config_install);
    lv_label_set_text(s_lbl_config_install, "نصب و اجرا");
    lv_obj_center(s_lbl_config_install);
    lv_obj_set_style_text_font(s_lbl_config_install, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_lbl_config_install, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // =========================================================================
    // MODAL 1: CUSTOM LINK ENTRY ("ورود لینک")
    // =========================================================================
    s_modal_link = lv_obj_create(ui_ProjectInfoScreen);
    lv_obj_set_size(s_modal_link, 480, 320);
    lv_obj_set_pos(s_modal_link, 0, 0);
    lv_obj_set_style_bg_color(s_modal_link, lv_color_hex(0x18181B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_modal_link, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_modal_link, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_modal_link, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_modal_link, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_modal_link, LV_OBJ_FLAG_HIDDEN);

    // Top action bar
    s_btn_link_back = lv_btn_create(s_modal_link);
    lv_obj_set_size(s_btn_link_back, 74, 36);
    lv_obj_set_pos(s_btn_link_back, 8, 6);
    lv_obj_set_style_bg_color(s_btn_link_back, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_link_back, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_link_back, link_entry_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lk_b_lbl = lv_label_create(s_btn_link_back);
    lv_label_set_text(lk_b_lbl, tr(STR_BACK));
    lv_obj_center(lk_b_lbl);
    lv_obj_set_style_text_font(lk_b_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_link_clear = lv_btn_create(s_modal_link);
    lv_obj_set_size(s_btn_link_clear, 42, 36);
    lv_obj_set_pos(s_btn_link_clear, 88, 6);
    lv_obj_set_style_bg_color(s_btn_link_clear, lv_color_hex(0x991B1B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_link_clear, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_link_clear, link_entry_clear_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *tr_ico = lv_label_create(s_btn_link_clear);
    lv_label_set_text(tr_ico, LV_SYMBOL_TRASH);
    lv_obj_center(tr_ico);
    lv_obj_set_style_text_font(tr_ico, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(tr_ico, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_link_default = lv_btn_create(s_modal_link);
    lv_obj_set_size(s_btn_link_default, 160, 36);
    lv_obj_set_pos(s_btn_link_default, 136, 6);
    lv_obj_set_style_bg_color(s_btn_link_default, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_btn_link_default, lv_color_hex(0x3B82F6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_btn_link_default, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_link_default, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_link_default, link_entry_default_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *def_lbl = lv_label_create(s_btn_link_default);
    lv_label_set_text(def_lbl, tr(STR_DEFAULT_LINK));
    lv_obj_center(def_lbl);
    lv_obj_set_style_text_font(def_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(def_lbl, lv_color_hex(0x93C5FD), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_link_confirm = lv_btn_create(s_modal_link);
    lv_obj_set_size(s_btn_link_confirm, 164, 36);
    lv_obj_set_pos(s_btn_link_confirm, 304, 6);
    lv_obj_set_style_bg_color(s_btn_link_confirm, lv_color_hex(0x059669), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_link_confirm, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_link_confirm, link_entry_confirm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cnf_lbl = lv_label_create(s_btn_link_confirm);
    lv_label_set_text(cnf_lbl, tr(STR_DOWNLOAD_AND_INSTALL));
    lv_obj_center(cnf_lbl);
    lv_obj_set_style_text_font(cnf_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cnf_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Text Area
    s_ta_link = lv_textarea_create(s_modal_link);
    lv_obj_set_pos(s_ta_link, 8, 48);
    lv_obj_set_size(s_ta_link, 464, 42);
    lv_textarea_set_one_line(s_ta_link, true);
    lv_textarea_set_max_length(s_ta_link, 256);
    lv_textarea_set_placeholder_text(s_ta_link, "https://... or http://...");
    lv_obj_set_style_radius(s_ta_link, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_ta_link, lv_color_hex(0x09090B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_ta_link, lv_color_hex(0x3B82F6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_ta_link, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_ta_link, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_ta_link, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_ta_link, lv_color_hex(0x38BDF8), LV_PART_CURSOR);
    lv_obj_set_style_border_color(s_ta_link, lv_color_hex(0x38BDF8), LV_PART_CURSOR);
    lv_obj_set_style_border_width(s_ta_link, 2, LV_PART_CURSOR);

    // Keyboard (480 x 220)
    s_kb_link = lv_keyboard_create(s_modal_link);
    lv_obj_set_pos(s_kb_link, 0, 96);
    lv_obj_set_size(s_kb_link, 480, 224);
    lv_keyboard_set_map(s_kb_link, LV_KEYBOARD_MODE_TEXT_LOWER, s_url_kb_map_lc, s_url_kb_ctrl_lc);
    lv_keyboard_set_map(s_kb_link, LV_KEYBOARD_MODE_TEXT_UPPER, s_url_kb_map_uc, s_url_kb_ctrl_uc);
    lv_keyboard_set_mode(s_kb_link, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_kb_link, s_ta_link);
    lv_obj_add_event_cb(s_kb_link, link_keyboard_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(s_kb_link, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_kb_link, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Error banner
    s_lbl_link_err = lv_label_create(s_modal_link);
    lv_obj_set_size(s_lbl_link_err, 440, 48);
    lv_obj_align(s_lbl_link_err, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_long_mode(s_lbl_link_err, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_bg_color(s_lbl_link_err, lv_color_hex(0x7F1D1D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_lbl_link_err, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_lbl_link_err, lv_color_hex(0xFECACA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_link_err, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(s_lbl_link_err, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_lbl_link_err, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_lbl_link_err, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(s_lbl_link_err, LV_OBJ_FLAG_HIDDEN);

    // =========================================================================
    // MODAL 2: SPECS & HARDWARE PINOUT MAP ("مشخصات و پایه‌ها")
    // =========================================================================
    s_modal_specs = lv_obj_create(ui_ProjectInfoScreen);
    lv_obj_set_size(s_modal_specs, 480, 320);
    lv_obj_set_pos(s_modal_specs, 0, 0);
    lv_obj_set_style_bg_color(s_modal_specs, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_modal_specs, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_modal_specs, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_modal_specs, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *spec_card = lv_obj_create(s_modal_specs);
    lv_obj_set_size(spec_card, 456, 290);
    lv_obj_align(spec_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(spec_card, lv_color_hex(0x1F2937), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(spec_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(spec_card, lv_color_hex(0x3B82F6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(spec_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(spec_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(spec_card, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_specs_title = lv_label_create(spec_card);
    lv_obj_align(s_lbl_specs_title, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_color(s_lbl_specs_title, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_specs_title, &ui_font_Dana_Bold18, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_specs_ver = lv_label_create(spec_card);
    lv_obj_align(s_lbl_specs_ver, LV_ALIGN_TOP_RIGHT, 0, 26);
    lv_obj_set_style_text_color(s_lbl_specs_ver, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_specs_ver, &ui_font_Dana_Bold10, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Scroll box for changelog and pinout
    lv_obj_t *spec_scroller = lv_obj_create(spec_card);
    lv_obj_set_size(spec_scroller, 434, 175);
    lv_obj_align(spec_scroller, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_set_style_bg_color(spec_scroller, lv_color_hex(0x111827), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(spec_scroller, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(spec_scroller, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(spec_scroller, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(spec_scroller, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(spec_scroller, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *lbl_chg_h = lv_label_create(spec_scroller);
    lv_label_set_text(lbl_chg_h, tr(STR_CHANGELOG_TITLE));
    lv_obj_set_style_text_color(lbl_chg_h, lv_color_hex(0xFBBF24), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_chg_h, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_specs_chg = lv_label_create(spec_scroller);
    lv_obj_set_width(s_lbl_specs_chg, 410);
    lv_label_set_long_mode(s_lbl_specs_chg, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_lbl_specs_chg, lv_color_hex(0xE4E4E7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_specs_chg, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *lbl_pin_h = lv_label_create(spec_scroller);
    lv_label_set_text(lbl_pin_h, tr(STR_PINMAP_TITLE));
    lv_obj_set_style_text_color(lbl_pin_h, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_pin_h, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_specs_pins_in = lv_label_create(spec_scroller);
    lv_obj_set_width(s_lbl_specs_pins_in, 410);
    lv_label_set_long_mode(s_lbl_specs_pins_in, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_lbl_specs_pins_in, lv_color_hex(0xD4D4D8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_specs_pins_in, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_lbl_specs_pins_out = lv_label_create(spec_scroller);
    lv_obj_set_width(s_lbl_specs_pins_out, 410);
    lv_label_set_long_mode(s_lbl_specs_pins_out, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_lbl_specs_pins_out, lv_color_hex(0xD4D4D8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_specs_pins_out, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Bottom modal buttons
    s_btn_specs_close = lv_btn_create(spec_card);
    lv_obj_set_size(s_btn_specs_close, 110, 38);
    lv_obj_align(s_btn_specs_close, LV_ALIGN_BOTTOM_LEFT, 6, -2);
    lv_obj_set_style_bg_color(s_btn_specs_close, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_btn_specs_close, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_specs_close, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_specs_close, specs_close_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *spc_c_lbl = lv_label_create(s_btn_specs_close);
    lv_label_set_text(spc_c_lbl, tr(STR_CLOSE));
    lv_obj_center(spc_c_lbl);
    lv_obj_set_style_text_font(spc_c_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_specs_install = lv_btn_create(spec_card);
    lv_obj_set_size(s_btn_specs_install, 150, 38);
    lv_obj_align(s_btn_specs_install, LV_ALIGN_BOTTOM_RIGHT, -6, -2);
    lv_obj_set_style_bg_color(s_btn_specs_install, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_btn_specs_install, lv_color_hex(0x059669), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_specs_install, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_specs_install, specs_install_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *spc_i_lbl = lv_label_create(s_btn_specs_install);
    lv_label_set_text(spc_i_lbl, tr(STR_INSTALL_UPDATE));
    lv_obj_center(spc_i_lbl);
    lv_obj_set_style_text_font(spc_i_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // =========================================================================
    // MODAL 3: CONFIRMATION MODAL
    // =========================================================================
    s_modal_confirm = lv_obj_create(ui_ProjectInfoScreen);
    lv_obj_set_size(s_modal_confirm, 480, 320);
    lv_obj_set_pos(s_modal_confirm, 0, 0);
    lv_obj_set_style_bg_color(s_modal_confirm, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_modal_confirm, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_modal_confirm, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_modal_confirm, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *card_confirm = lv_obj_create(s_modal_confirm);
    lv_obj_set_size(card_confirm, 410, 175);
    lv_obj_align(card_confirm, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card_confirm, lv_color_hex(0x1F2937), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card_confirm, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card_confirm, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card_confirm, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(card_confirm, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_confirm_msg = lv_label_create(card_confirm);
    lv_obj_set_width(s_lbl_confirm_msg, 370);
    lv_label_set_long_mode(s_lbl_confirm_msg, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_confirm_msg, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(s_lbl_confirm_msg, tr(STR_INSTALL_CONFIRM));
    lv_obj_set_style_text_color(s_lbl_confirm_msg, lv_color_hex(0xF9FAFB), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_confirm_msg, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(s_lbl_confirm_msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_confirm_cancel = lv_btn_create(card_confirm);
    lv_obj_set_size(s_btn_confirm_cancel, 150, 42);
    lv_obj_align(s_btn_confirm_cancel, LV_ALIGN_BOTTOM_LEFT, 10, -6);
    lv_obj_set_style_bg_color(s_btn_confirm_cancel, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_confirm_cancel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_confirm_cancel, confirm_cancel_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cncl_lbl = lv_label_create(s_btn_confirm_cancel);
    lv_label_set_text(cncl_lbl, "انصراف");
    lv_obj_center(cncl_lbl);
    lv_obj_set_style_text_font(cncl_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_btn_confirm_ok = lv_btn_create(card_confirm);
    lv_obj_set_size(s_btn_confirm_ok, 160, 42);
    lv_obj_align(s_btn_confirm_ok, LV_ALIGN_BOTTOM_RIGHT, -10, -6);
    lv_obj_set_style_bg_color(s_btn_confirm_ok, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_btn_confirm_ok, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_btn_confirm_ok, confirm_ok_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ok_lbl = lv_label_create(s_btn_confirm_ok);
    lv_label_set_text(ok_lbl, "تایید و شروع دانلود");
    lv_obj_center(ok_lbl);
    lv_obj_set_style_text_font(ok_lbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // =========================================================================
    // MODAL 4: PROGRESS OVERLAY
    // =========================================================================
    s_modal_progress = lv_obj_create(ui_ProjectInfoScreen);
    lv_obj_set_size(s_modal_progress, 480, 320);
    lv_obj_set_pos(s_modal_progress, 0, 0);
    lv_obj_set_style_bg_color(s_modal_progress, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_modal_progress, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_modal_progress, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_modal_progress, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *prog_card = lv_obj_create(s_modal_progress);
    lv_obj_set_size(prog_card, 410, 180);
    lv_obj_align(prog_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(prog_card, lv_color_hex(0x1F2937), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(prog_card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(prog_card, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_progress_sub = lv_label_create(prog_card);
    lv_obj_align(s_lbl_progress_sub, LV_ALIGN_TOP_MID, 0, 14);
    lv_label_set_text(s_lbl_progress_sub, tr(STR_INSTALLING));
    lv_obj_set_style_text_color(s_lbl_progress_sub, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_progress_sub, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_bar_progress = lv_bar_create(prog_card);
    lv_obj_set_size(s_bar_progress, 370, 24);
    lv_obj_align(s_bar_progress, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(s_bar_progress, 0, 100);
    lv_bar_set_value(s_bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_progress, lv_color_hex(0x374151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_bar_progress, lv_color_hex(0x10B981), LV_PART_INDICATOR | LV_STATE_DEFAULT);

    s_lbl_progress_pct = lv_label_create(prog_card);
    lv_obj_align(s_lbl_progress_pct, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_label_set_text(s_lbl_progress_pct, "0 % - لطفاً دستگاه را خاموش نکنید");
    lv_obj_set_style_text_color(s_lbl_progress_pct, lv_color_hex(0xF9FAFB), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_lbl_progress_pct, &ui_font_Dana_Bold10, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_ProjectInfoScreen_open(void) {
    s_info_prev_scr = lv_scr_act();
    ui_ProjectInfoScreen_screen_init();
    lv_scr_load(ui_ProjectInfoScreen);

    // If connected to WiFi, trigger fetch for both programs if not already fetched
    if (WiFi.status() == WL_CONNECTED) {
        if (!otaManager.release_info.available && !otaManager.is_fetching_info) {
            otaManager.fetchReleaseInfoAsync(0);
        }
        if (!otaManager.release_info_config.available && !otaManager.is_fetching_info) {
            otaManager.fetchReleaseInfoAsync(1);
        }
    }
    populate_release_ui();
}

void ui_ProjectInfoScreen_update_tick(void) {
    if (!ui_ProjectInfoScreen || lv_scr_act() != ui_ProjectInfoScreen) return;

    if (otaManager.info_fetch_done) {
        otaManager.info_fetch_done = false;
        populate_release_ui();
    }

    if (otaManager.is_downloading) {
        if (s_bar_progress) {
            lv_bar_set_value(s_bar_progress, otaManager.download_percent, LV_ANIM_OFF);
        }
        if (s_lbl_progress_pct) {
            char pbuf[64];
            snprintf(pbuf, sizeof(pbuf), "%d %% - در حال ذخیره‌سازی در فلش...", otaManager.download_percent);
            lv_label_set_text(s_lbl_progress_pct, pbuf);
        }
    }

    if (otaManager.download_success) {
        if (s_lbl_progress_pct) {
            lv_label_set_text(s_lbl_progress_pct, "100 % - نصب با موفقیت پایان یافت. ریبوت سیستم...");
            lv_obj_set_style_text_color(s_lbl_progress_pct, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    } else if (otaManager.download_error) {
        otaManager.download_error = false;
        if (s_lbl_progress_pct) {
            lv_label_set_text(s_lbl_progress_pct, "خطا در دانلود یا نوشتن پارتیشن. لغو عملیات.");
            lv_obj_set_style_text_color(s_lbl_progress_pct, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}
