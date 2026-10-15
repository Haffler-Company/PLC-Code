#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "lang.h"
#include "defines.h"
#include "app_sound.h"
#include "hwtest.h"
#include "fw_changelog.h"   // fw_changelog[] + FW_BUILD_DATE/TIME برای پاپ‌آپ اطلاعات ورژن
#include "drivers/keyboard_style.h"

extern void action_btn_back_to_password(lv_event_t *e);
extern void reset_config_wifi_flow(bool keepConnection);
extern void config_start_wifi_scan(void);
extern bool isSearching;
extern lv_obj_t *tick_value_change_obj;

// پیامِ موفقیتِ اتصالِ وای‌فای: app_task پرچم + SSID را پر می‌کند؛ اعمال/پاک‌سازی در
// نخِ LVGL (tick صفحه‌ی وای‌فای) — الگوی g_card_ver_ready
extern volatile bool g_wifi_ok_msg;
extern char g_wifi_ok_ssid[65];

static void event_handler_wifi_back_to_password(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    reset_config_wifi_flow(true);
    action_btn_back_to_password(e);
}

static void async_open_firmware_from_wifi(void *user_data) {
    (void)user_data;
    eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
}

// طولِ نویسه‌ی UTF-8 از بایتِ آغازینِ آن (۰ = بایتِ ادامه/نامعتبر — نویسه‌ی خراب)
static int wifi_utf8_seq_len(unsigned char c) {
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 0;
}

static void update_wifi_dropdown_options_direct(void) {
    if (!objects.drop_down_wifi_list) return;

    // کپیِ UTF-8-امن وکران‌دار: بافرِ ۱۰۲۴تایی، حداکثر ۱۲ شبکه، هر SSID حداکثر ۳۲ بایت،
    // و هیچ نویسه‌ی چندبایتی در مرزِ برش نصف نمی‌شود — SSIDهای فارسی/ایموجی دیگر
    // مربع نمایش داده نمی‌شدند (فونت + برشِ وسطِ دنباله هر دو علتِ گلیفِ خراب بودند).
    // ترتیبِ گزینه‌ها عیناً ترتیبِ wifis[] در .ino می‌ماند (برش فقط از انتها).
    static char opts[1024];
    const char *src = get_var_available_wifi_networks();

    size_t j = 0;
    if (src && src[0]) {
        const int MAX_NETS = 12;   // سقفِ شبکه‌های لیست
        const int MAX_SSID = 32;   // سقفِ بایتِ هر SSID
        int nets = 0;
        const unsigned char *p = (const unsigned char *)src;
        while (*p && nets < MAX_NETS && j + 2 < sizeof(opts)) {
            if (j > 0) opts[j++] = '\n';
            nets++;
            int ssidBytes = 0;
            while (*p && *p != '#' && ssidBytes < MAX_SSID && j + 5 < sizeof(opts)) {
                int clen = wifi_utf8_seq_len(*p);
                if (clen == 0) { p++; continue; }          // بایتِ ناقص/نامعتبر → حذف
                if (ssidBytes + clen > MAX_SSID) break;    // نویسه‌ی بعدی در مرزِ ۳۲ نصف می‌شد → پایانِ SSID
                int k = 0;
                for (; k < clen && p[k]; k++) opts[j++] = (char)*p++;
                if (k < clen) { j -= k; break; }           // رشته وسطِ نویسه تمام شد → کلِ نویسه کنار
                ssidBytes += clen;
            }
            while (*p && *p != '#') p++;                   // باقیِ SSID بلند رد شود
            if (*p == '#') p++;
        }
        while (j > 0 && opts[j - 1] == '\n') j--;          // جداکننده‌ی آویخته نه
    }
    opts[j] = 0;

    if (opts[0] == 0) {
        strncpy(opts, "Scanning...", sizeof(opts) - 1);
        opts[sizeof(opts) - 1] = 0;
    }

    const char *cur = lv_dropdown_get_options(objects.drop_down_wifi_list);
    if (strcmp(opts, cur) != 0) {
        tick_value_change_obj = objects.drop_down_wifi_list;
        lv_dropdown_set_options(objects.drop_down_wifi_list, opts);
        tick_value_change_obj = NULL;
    }
}

// پرچم‌های زبان (تولیدشده در img_flag_*.c)
extern const lv_image_dsc_t img_flag_en;
extern const lv_image_dsc_t img_flag_tr;
extern const lv_image_dsc_t img_flag_fa;
extern const lv_image_dsc_t img_flag_de;
extern const lv_image_dsc_t img_flag_ru;

// تابع تغییر زبان (در .ino تعریف شده)
#ifdef __cplusplus
extern "C" void app_set_language(int lang);
#else
extern void app_set_language(int lang);
#endif

// اشاره‌گر پنل انتخاب زبان روی صفحه‌ی رمز ادمین (برای toggle)
static lv_obj_t *g_lang_select_panel = NULL;

#include <string.h>
#include <stdio.h>   // snprintf برای پیامِ «متصل شد»

objects_t objects;
lv_obj_t *tick_value_change_obj;

static const lv_point_precise_t delete_icon_outline[] = {
    { 34, 4 }, { 16, 4 }, { 5, 14 }, { 16, 24 }, { 34, 24 }, { 34, 4 }
};
static const lv_point_precise_t delete_icon_x1[] = { { 20, 10 }, { 28, 18 } };
static const lv_point_precise_t delete_icon_x2[] = { { 28, 10 }, { 20, 18 } };

static void add_delete_icon_line(lv_obj_t *parent, const lv_point_precise_t *points, uint32_t count, int32_t width)
{
    lv_obj_t *line = lv_line_create(parent);
    lv_obj_set_pos(line, 63, 127);
    lv_obj_set_size(line, 40, 28);
    lv_line_set_points(line, points, count);
    lv_obj_set_style_line_width(line, width, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(line, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

static void add_password_delete_icon(lv_obj_t *kb)
{
    add_delete_icon_line(kb, delete_icon_outline, 6, 3);
    add_delete_icon_line(kb, delete_icon_x1, 2, 4);
    add_delete_icon_line(kb, delete_icon_x2, 2, 4);
}

// ---- دکمه چشم برای نمایش/پنهان‌سازی رمز (سفارشی، خارج از تولید EEZ) ----
static void password_eye_hold_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    if (!ta) return;
    if (code == LV_EVENT_PRESSED) app_sound_play(SOUND_TAP);
    bool show = (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
        code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
    lv_textarea_set_password_mode(ta, !show);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl) lv_label_set_text(lbl, show ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
}

// ایجاد دکمه چشم کنار یک textarea رمز
static void add_password_eye(lv_obj_t *parent, lv_obj_t *ta, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 44, 42);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, password_eye_hold_cb, LV_EVENT_ALL, ta);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_EYE_CLOSE);  // پیش‌فرض: رمز پنهان است
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT); // ۳۵ درصد بزرگتر (از ۲۰ به ۲۶)
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl);
}

// ---- انتخاب‌گر زبان (دکمه‌ی گرد پرچم + بسته‌شدن با لمس بیرون) ----
static lv_obj_t *g_lang_backdrop = NULL;
static lv_obj_t *g_lang_btn_img = NULL;

static void lang_backdrop_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_lang_select_panel && lv_obj_is_valid(g_lang_select_panel))
        lv_obj_add_flag(g_lang_select_panel, LV_OBJ_FLAG_HIDDEN);
    if (g_lang_backdrop && lv_obj_is_valid(g_lang_backdrop))
        lv_obj_add_flag(g_lang_backdrop, LV_OBJ_FLAG_HIDDEN);
}

static void lang_btn_toggle_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    if (g_lang_select_panel && lv_obj_is_valid(g_lang_select_panel)) {
        if (lv_obj_has_flag(g_lang_select_panel, LV_OBJ_FLAG_HIDDEN)) {
            if (g_lang_backdrop && lv_obj_is_valid(g_lang_backdrop)) {
                lv_obj_clear_flag(g_lang_backdrop, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(g_lang_backdrop);
            }
            lv_obj_clear_flag(g_lang_select_panel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(g_lang_select_panel);
        } else {
            lv_obj_add_flag(g_lang_select_panel, LV_OBJ_FLAG_HIDDEN);
            if (g_lang_backdrop && lv_obj_is_valid(g_lang_backdrop))
                lv_obj_add_flag(g_lang_backdrop, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

extern void app_save_cards_scroll_x(int16_t x);
extern int16_t app_get_cards_scroll_x(void);

static bool g_cards_scroll_dirty = false;
static uint32_t g_cards_scroll_dirty_ms = 0;

static void event_handler_cards_row_scroll(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCROLL) {
        g_cards_scroll_dirty = true;
        g_cards_scroll_dirty_ms = lv_tick_get();
    }
}

static lv_obj_t *g_pm_panel = NULL;
lv_obj_t *get_pinmap_panel(void) { return g_pm_panel; }
static int s_current_pm_idx = -1;
static void show_pinmap(int idx);
static void update_guide_modal_lang(void);

static void lang_row_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    int lang = (int)(intptr_t)lv_event_get_user_data(e);
    app_set_language(lang);   // تنظیم + ذخیره + اعمال زنده
    if (g_lang_btn_img && lv_obj_is_valid(g_lang_btn_img)) {
        const lv_image_dsc_t *flags[LANG_COUNT] = { &img_flag_en, &img_flag_tr, &img_flag_fa, &img_flag_de, &img_flag_ru };
        if (lang >= 0 && lang < LANG_COUNT) {
            lv_image_set_src(g_lang_btn_img, flags[lang]);
        }
    }
    if (g_lang_select_panel && lv_obj_is_valid(g_lang_select_panel))
        lv_obj_add_flag(g_lang_select_panel, LV_OBJ_FLAG_HIDDEN);
    if (g_lang_backdrop && lv_obj_is_valid(g_lang_backdrop))
        lv_obj_add_flag(g_lang_backdrop, LV_OBJ_FLAG_HIDDEN);
    if (g_pm_panel && lv_obj_is_valid(g_pm_panel) && !lv_obj_has_flag(g_pm_panel, LV_OBJ_FLAG_HIDDEN)) {
        show_pinmap(s_current_pm_idx);
    }
    update_guide_modal_lang();
}

// ---- دکمه ضامن صدا/بلندگو (بالا سمت چپ، متقارن با پرچم زبان ۵۸x۵۸) ----
static lv_obj_t *g_sound_btn_lbl = NULL;
static void sound_btn_toggle_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    bool cur = app_sound_is_enabled();
    app_sound_set_enabled(!cur);
    bool new_state = app_sound_is_enabled();
    if (g_sound_btn_lbl && lv_obj_is_valid(g_sound_btn_lbl)) {
        lv_label_set_text(g_sound_btn_lbl, new_state ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
        lv_obj_set_style_text_color(g_sound_btn_lbl, lv_color_hex(new_state ? 0xff0f172a : 0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (new_state) {
        app_sound_play(SOUND_SUCCESS);
    }
}

static void add_sound_toggle_button(lv_obj_t *parent) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, 4, 4); // ۲ پیکسل بالا و ۴ پیکسل چپ
    lv_obj_set_size(btn, 58, 58);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(btn, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, sound_btn_toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    g_sound_btn_lbl = lbl;
    bool en = app_sound_is_enabled();
    lv_label_set_text(lbl, en ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(en ? 0xff0f172a : 0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl);
}

// ---- مودال راهنما و دکمه کتاب (سمت چپ کادر رمز، ۴۴x۴۲) ----
static lv_obj_t *g_guide_modal = NULL;
static lv_obj_t *g_guide_backdrop = NULL;
static lv_obj_t *g_guide_title = NULL;
static lv_obj_t *g_guide_items[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t *g_guide_close_lbl = NULL;

static void update_guide_modal_lang(void) {
    if (!g_guide_modal || !lv_obj_is_valid(g_guide_modal)) return;

    Lang l = g_lang;
    if (l < 0 || l >= LANG_COUNT) l = LANG_FA;
    bool is_fa = (l == LANG_FA);

    static const char *const titles[LANG_COUNT] = {
        "System Guide & Access Codes",
        "Sistem Kilavuzu ve Erisim Kodlari",
        "راهنمای سیستم و کدهای دسترسی",
        "Systemhandbuch & Zugriffscodes",
        "Rukovodstvo i kody dostupa"
    };

    static const char *const items[4][LANG_COUNT] = {
        {
            "0000 / 9999 : Full Hardware Test & QC Suite",
            "0000 / 9999 : Tam Donanim Testi ve Kalite Kontrol (QC)",
            "0000 / 9999 : تست جامع سخت افزار و کنترل کیفیت (QC)",
            "0000 / 9999 : Umfassender Hardwaretest & QC",
            "0000 / 9999 : Polnyy test zheleza i kontrol kachestva (QC)"
        },
        {
            "123 : UPVC & Aluminum Machines (Welder, Cutter...)",
            "123 : UPVC ve Aluminyum Makineleri (Kaynak, Kesim...)",
            "123 : ماشین آلات UPVC و آلومینیوم (جوش، برش و...)",
            "123 : UPVC- & Aluminiummaschinen (Schweißen, Sägen...)",
            "123 : UPVC i alyuminievye stanki (svarka, pilenie...)"
        },
        {
            "321 : Other Industrial Machines (Bakery, Ice Cream...)",
            "321 : Diger Sanayi Makineleri (Firin, Dondurma...)",
            "321 : سایر ماشین آلات صنعتی (نانوایی، بستنی و...)",
            "321 : Andere Industriemaschinen (Bäckerei, Eismaschine...)",
            "321 : Drugie promyshlennye stanki (pech, morozhenoe...)"
        },
        {
            "Direct Link : Orange button to download from server",
            "Link Girisi : Sunucudan dogrudan indirme butonu",
            "ورود لینک : دکمه نارنجی برای دانلود مستقیم از مخزن",
            "Direkt-Link : Orange Schaltfläche zum Download",
            "Vvod ssylki : Oranzhevaya knopka dlya skachivaniya s servera"
        }
    };

    static const char *const close_texts[LANG_COUNT] = {
        "Got it",
        "Anladim",
        "متوجه شدم",
        "Verstanden",
        "Ponyatno"
    };

    if (g_guide_title && lv_obj_is_valid(g_guide_title)) {
        lv_label_set_text(g_guide_title, titles[l]);
        lv_obj_set_style_text_font(g_guide_title, is_fa ? &ui_font_kalameh_bold_18 : &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    for (int i = 0; i < 4; i++) {
        if (g_guide_items[i] && lv_obj_is_valid(g_guide_items[i])) {
            lv_label_set_text(g_guide_items[i], items[i][l]);
            lv_obj_set_style_text_font(g_guide_items[i], is_fa ? &ui_font_kalame_bold_14 : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_base_dir(g_guide_items[i], is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(g_guide_items[i], is_fa ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, is_fa ? -6 : 6, 0);
        }
    }

    if (g_guide_close_lbl && lv_obj_is_valid(g_guide_close_lbl)) {
        lv_label_set_text(g_guide_close_lbl, close_texts[l]);
        lv_obj_set_style_text_font(g_guide_close_lbl, is_fa ? &ui_font_kalame_bold_14 : &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void guide_close_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    if (g_guide_modal && lv_obj_is_valid(g_guide_modal))
        lv_obj_add_flag(g_guide_modal, LV_OBJ_FLAG_HIDDEN);
    if (g_guide_backdrop && lv_obj_is_valid(g_guide_backdrop))
        lv_obj_add_flag(g_guide_backdrop, LV_OBJ_FLAG_HIDDEN);
}

static void guide_open_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    update_guide_modal_lang();
    if (g_guide_backdrop && lv_obj_is_valid(g_guide_backdrop)) {
        lv_obj_clear_flag(g_guide_backdrop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_guide_backdrop);
    }
    if (g_guide_modal && lv_obj_is_valid(g_guide_modal)) {
        lv_obj_clear_flag(g_guide_modal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_guide_modal);
    }
}

static void add_guide_button(lv_obj_t *parent) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, 69, 37);
    lv_obj_set_size(btn, 44, 42);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, guide_open_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, &img_book_open);
    lv_obj_center(img);
}

static void create_guide_modal(lv_obj_t *parent) {
    lv_obj_t *bd = lv_obj_create(parent);
    g_guide_backdrop = bd;
    lv_obj_set_pos(bd, 0, 0);
    lv_obj_set_size(bd, 480, 320);
    lv_obj_set_style_bg_color(bd, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bd, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bd, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bd, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(bd, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bd, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(bd, guide_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *card = lv_obj_create(parent);
    g_guide_modal = card;
    lv_obj_set_size(card, 430, 276);
    lv_obj_center(card);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *title = lv_label_create(card);
    g_guide_title = title;
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *box = lv_obj_create(card);
    lv_obj_set_pos(box, 0, 36);
    lv_obj_set_size(box, 410, 172);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = lv_obj_create(box);
        lv_obj_set_size(row, 396, 36);
        lv_obj_set_pos(row, 0, i * 40);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(row, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(row, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl = lv_label_create(row);
        g_guide_items[i] = lbl;
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xff1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_t *close_btn = lv_btn_create(card);
    lv_obj_set_size(close_btn, 160, 40);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(close_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(close_btn, guide_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clbl = lv_label_create(close_btn);
    g_guide_close_lbl = clbl;
    lv_obj_center(clbl);

    update_guide_modal_lang();
}

static void add_language_selector(lv_obj_t *parent) {
    const lv_image_dsc_t *flags[LANG_COUNT] = { &img_flag_en, &img_flag_tr, &img_flag_fa, &img_flag_de, &img_flag_ru };

    // لایه شفاف برای تشخیص لمس بیرون و بستن خودکار
    lv_obj_t *bd = lv_obj_create(parent);
    g_lang_backdrop = bd;
    lv_obj_set_pos(bd, 0, 0);
    lv_obj_set_size(bd, 480, 320);
    lv_obj_set_style_bg_opa(bd, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bd, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bd, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(bd, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bd, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(bd, lang_backdrop_cb, LV_EVENT_CLICKED, NULL);

    // دکمه‌ی گرد پرچم زبان، گوشه بالا سمت راست (۲ پیکسل بالا و ۴ پیکسل راست: 418, 4)
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, 418, 4);
    lv_obj_set_size(btn, 58, 58);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(btn, 1, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(btn, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, lang_btn_toggle_cb, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *bi = lv_image_create(btn);
        g_lang_btn_img = bi;
        if (g_lang >= 0 && g_lang < LANG_COUNT) {
            lv_image_set_src(bi, flags[g_lang]);
        } else {
            lv_image_set_src(bi, &img_flag_fa);
        }
        lv_obj_center(bi);
    }

    // پنل انتخاب زبان (پیش‌فرض پنهان با تم روشن و سایز ارگونومیک)
    lv_obj_t *panel = lv_obj_create(parent);
    g_lang_select_panel = panel;
    lv_obj_set_size(panel, 224, 286);
    lv_obj_set_pos(panel, 246, 26);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < LANG_COUNT; i++) {
        lv_obj_t *row = lv_button_create(panel);
        lv_obj_set_size(row, 210, 52);
        lv_obj_set_pos(row, 0, i * 55);
        lv_obj_set_style_radius(row, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(row, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(row, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(row, lang_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        {
            lv_obj_t *fl = lv_image_create(row);
            lv_image_set_src(fl, flags[i]);
            lv_obj_align(fl, LV_ALIGN_LEFT_MID, 6, 0);
        }
        {
            lv_obj_t *nm = lv_label_create(row);
            lv_label_set_text(nm, lang_native_name((Lang)i));
            lv_obj_set_style_text_color(nm, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(nm, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(nm, LV_ALIGN_LEFT_MID, 58, 0);
        }
    }
}

static void event_handler_cb_screen_splash_screen_splash(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_SCREEN_LOADED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_screen_password_panle_wifi_password_1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_SCREEN_LOADED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

// کیبردِ عددیِ رمزِ ورود: فقط عدد + پاک‌کردن + تایید (سه‌زبانه). دکمهٔ id=9 پاک، id=11 تایید.
static const char *cfg_pass_kb_map_fa[] = {
    "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const char *cfg_pass_kb_map_en[] = {
    "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const char *cfg_pass_kb_map_tr[] = {
    "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const char *cfg_pass_kb_map_de[] = {
    "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const char *cfg_pass_kb_map_ru[] = {
    "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t cfg_pass_kb_ctrl[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static const char *const *cfg_pass_kb_map(void) {
    if (g_lang == LANG_EN) return cfg_pass_kb_map_en;
    if (g_lang == LANG_TR) return cfg_pass_kb_map_tr;
    if (g_lang == LANG_DE) return cfg_pass_kb_map_de;
    if (g_lang == LANG_RU) return cfg_pass_kb_map_ru;
    return cfg_pass_kb_map_fa;
}

// کیبرد اختصاصی ورود لینک و URL با کلید حروف بزرگ (Shift/Caps) و بدون لگ
static const char * const cfg_url_kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "/", "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", ".", "-", "\n",
    "https://", ":", "%", "=", "_", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t cfg_url_kb_ctrl_lc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 2, 2
};

static const char * const cfg_url_kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "/", "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", ".", "-", "\n",
    "https://", ":", "%", "=", "_", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t cfg_url_kb_ctrl_uc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 2, 2
};

// کیبرد صنعتی ارگونومیک وای‌فای: ۱۰ رقم (۰-۹) در ردیف نخست با دسترسی مستقیم بدون تغییر مود
static const char * const cfg_wifi_kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_BACKSPACE, "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", "1#", " ", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t cfg_wifi_kb_ctrl_lc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2), 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2), 3, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 3)
};

static const char * const cfg_wifi_kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_BACKSPACE, "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", "1#", " ", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t cfg_wifi_kb_ctrl_uc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2), 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2), 3, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 3)
};

static const char * const cfg_wifi_kb_map_spec[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "\n",
    "-", "_", "=", "+", "[", "]", "{", "}", "\\", LV_SYMBOL_BACKSPACE, "\n",
    "abc", ":", ";", "\"", "'", "<", ">", ",", ".", "/", " ", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t cfg_wifi_kb_ctrl_spec[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2), 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, (LV_KEYBOARD_CTRL_BUTTON_FLAGS | 3)
};

static void event_handler_cb_screen_password_frameware_password_keybord(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);

    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *kb = lv_event_get_target(e);
        uint32_t id = lv_buttonmatrix_get_selected_button(kb);   // 0..8=ارقام، 9=پاک، 10="0"، 11=تایید
        lv_obj_t *ta = objects.text_area_framware_password;
        if (!ta) return;
        app_sound_play(SOUND_TAP);
        const char *txt = lv_buttonmatrix_get_button_text(kb, id);
        if (id == 11 || (txt && strcmp(txt, LV_SYMBOL_OK) == 0)) {
            const char *t = lv_textarea_get_text(ta);
            if (t) set_var_admin_password(t);
            action_admin_password_redy(e);
        }
        else if (id == 9 || (txt && strcmp(txt, LV_SYMBOL_BACKSPACE) == 0)) {
            lv_textarea_delete_char(ta);
        }
        else {
            if (txt) lv_textarea_add_text(ta, txt);
        }
    } else if (event == LV_EVENT_READY) {
        // نادیده‌گرفتن جهت جلوگیری از ارسال مجدد با رشته‌ی خالی (دکمه ۱۱ قبلاً در VALUE_CHANGED پردازش شد)
    }
}

static void event_handler_cb_screen_password_text_area_framware_password(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            if (tick_value_change_obj != ta) {
                assignStringProperty(flowState, 4, 3, value, "Failed to assign Text in Textarea widget");
            }
        }
    }
}

static void event_handler_cb_screen_password_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 6, 0, e);
    }
}

static void event_handler_cb_screen_wifi_drop_down_wifi_list(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    lv_obj_t *ta = lv_event_get_target(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        app_sound_play(SOUND_TAP);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            set_var_selected_wifi_index(value);
            if (tick_value_change_obj != ta) {
                assignIntegerProperty(flowState, 3, 4, value, "Failed to assign Selected in Dropdown widget");
            }
        }
    }
}

static void event_handler_cb_screen_wifi_btn_wifi_connect(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_CLICKED || event == LV_EVENT_SHORT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_btn_connect(e);
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 4, 0, e);
    }
}

static void event_handler_cb_screen_wifi_btn_wifi_redresh(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_CLICKED || event == LV_EVENT_SHORT_CLICKED) {
        app_sound_play(SOUND_TAP);
        e->user_data = (void *)0;
        action_btn_wifi_search_event(e);
    }
}

static void event_handler_cb_screen_wifi_panle_wifi_password(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_SCREEN_LOADED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 9, 0, e);
    }
}

static void event_handler_cb_screen_wifi_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_CLICKED || event == LV_EVENT_SHORT_CLICKED) {
        if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password)) {
            lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        }
        app_sound_play(SOUND_TAP);
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 10, 0, e);
    }
}

static void event_handler_cb_screen_wifi_keybord(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_READY) {
        if (objects.text_area_wifi_password && lv_obj_is_valid(objects.text_area_wifi_password)) {
            const char *pw = lv_textarea_get_text(objects.text_area_wifi_password);
            set_var_wifi_password(pw ? pw : "");
        }
        if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password)) {
            lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        }
        app_sound_play(SOUND_TAP);
        e->user_data = (void *)0;
        action_btn_wifi_connect_event(e);
        flowPropagateValueLVGLEvent(flowState, 13, 0, e);
    } else if (event == LV_EVENT_CANCEL) {
        if (objects.panle_wifi_password && lv_obj_is_valid(objects.panle_wifi_password)) {
            lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        }
        app_sound_play(SOUND_TAP);
    } else if (event == LV_EVENT_VALUE_CHANGED) {
        app_sound_play(SOUND_TAP);
    }
}

static void event_handler_cb_screen_wifi_text_area_wifi_password(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            if (tick_value_change_obj != ta) {
                assignStringProperty(flowState, 14, 3, value, "Failed to assign Text in Textarea widget");
            }
        }
    }
}

static void event_handler_cb_screen_wifi_language_panle(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 19, 0, e);
    }
}

void create_screen_screen_splash() {
    void *flowState = getFlowState(0, 0);
    lv_obj_t *obj = lv_obj_create(0);
    objects.screen_splash = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, event_handler_cb_screen_splash_screen_splash, LV_EVENT_ALL, flowState);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_img_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, g_splash_variant ? &img_splash_b : &img_splash_a);
        }
        {
            // نام شرکت HAFFLER در گوشه بالا-چپ
            lv_obj_t *lbl = lv_label_create(parent_obj);
            lv_obj_set_pos(lbl, 10, 9);
            lv_obj_set_size(lbl, 200, LV_SIZE_CONTENT);
            lv_label_set_text(lbl, "HAFFLER");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // خط زیر نام شرکت
            lv_obj_t *line = lv_obj_create(parent_obj);
            lv_obj_remove_style_all(line);
            lv_obj_set_pos(line, 10, 40);
            lv_obj_set_size(line, 120, 2);
            lv_obj_set_style_bg_color(line, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(line, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // نسخه برنامه زیر خط
            lv_obj_t *lbl = lv_label_create(parent_obj);
            lv_obj_set_pos(lbl, 10, 44);
            lv_obj_set_size(lbl, 200, LV_SIZE_CONTENT);
            lv_label_set_text(lbl, "Version: " VERSION);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 0, 37);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); // حذف نمایش نسخه در وسط صفحه اسپلش
        }
        if (g_boot_diag.has_error) {
            // کپسول هشدار ظریف در پایین صفحه اسپلش در صورت بروز خطا در تست اولیه
            lv_obj_t *pill = lv_obj_create(parent_obj);
            lv_obj_set_size(pill, 260, 30);
            lv_obj_align(pill, LV_ALIGN_BOTTOM_MID, 0, -12);
            lv_obj_set_style_bg_color(pill, lv_color_hex(0x18181b), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(pill, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(pill, lv_color_hex(0xef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(pill, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(pill, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(pill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *lbl = lv_label_create(pill);
            lv_label_set_text(lbl, LV_SYMBOL_WARNING " خطای سخت‌افزار شناسایی شد");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xf87171), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(lbl);
        }
    }
    
    tick_screen_screen_splash();
}

void delete_screen_screen_splash() {
    lv_obj_del(objects.screen_splash);
    objects.screen_splash = 0;
    objects.obj4 = 0;
    deletePageFlowState(0);
}

void tick_screen_screen_splash() {
    if (!objects.screen_splash || !lv_obj_is_valid(objects.screen_splash)) return;
    void *flowState = getFlowState(0, 0);
    if (!flowState) return;
    if (objects.obj4 && lv_obj_is_valid(objects.obj4)) {
        const char *new_val = evalTextProperty(flowState, 2, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj4);
        if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj4;
            lv_label_set_text(objects.obj4, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_screen_password() {
    i18n_reset();
    set_var_admin_password("");
    set_var_wrong_password_atemp("");
    set_var_admin_waiting_password_stutus(false);
    set_var_admin_waiting_stutus_lable("");
    // اسکن پیش‌دستانه وای‌فای در پس‌زمینه تا زمان ورود به صفحه وای‌فای آماده باشد
    if (!get_var_is_wifi_connected() && !isSearching) {
        config_start_wifi_scan();
    }
    void *flowState = getFlowState(0, 1);
    lv_obj_t *obj = lv_obj_create(0);
    objects.screen_password = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // Panle_WifiPassword_1 (پس‌زمینه کاملاً سفید تم لایت)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.panle_wifi_password_1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_add_event_cb(obj, event_handler_cb_screen_password_panle_wifi_password_1, LV_EVENT_ALL, flowState);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // عنوان صفحه رمز (تراز دقیق در وسط بالا، ۶ پیکسل بالاتر در y=0)
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj5 = obj;
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
                    lv_label_set_text(obj, tr(STR_ADMIN_ENTER));
                    i18n_register(obj, STR_ADMIN_ENTER);
                    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                // دکمه راهنمای سیستم و رمزها (کتاب) در سمت چپ کادر رمز (۴۴x۴۲)
                add_guide_button(parent_obj);
                {
                    // کادر ورودی رمز (تراز کاملاً متقارن در مرکز صفحه در x=120..360، عرض ۲۴۰، ارتفاع ۴۲ در y=34)
                    lv_obj_t *obj = lv_textarea_create(parent_obj);
                    objects.text_area_framware_password = obj;
                    lv_obj_set_pos(obj, 120, 34);
                    lv_obj_set_size(obj, 240, 42);
                    lv_textarea_set_max_length(obj, 12);
                    lv_textarea_set_one_line(obj, true);
                    lv_textarea_set_password_mode(obj, true);
                    lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_cb_screen_password_text_area_framware_password, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                // دکمه چشم رمز ادمین در سمت راست کادر رمز (x=367, y=37, 44x42)
                add_password_eye(parent_obj, objects.text_area_framware_password, 367, 37);
                {
                    // کیپد عددی استاندارد اپل در تم سفید (مارجین کاملاً متقارن ۱۲ پیکسل از دو طرف صفحه، ارتفاع ۲۰۲ پیکسل)
                    lv_obj_t *obj = lv_keyboard_create(parent_obj);
                    objects.frameware_password_keybord = obj;
                    lv_obj_set_pos(obj, 12, 106);
                    lv_obj_set_size(obj, 456, 202);
                    lv_keyboard_set_map(obj, LV_KEYBOARD_MODE_USER_1, cfg_pass_kb_map(), cfg_pass_kb_ctrl);
                    lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_USER_1);
                    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

                    // استایل کلیدهای کیپد در تم سفید اپل (حذف کامل سایه نرم‌افزاری برای تاچ آنی ۶۰FPS)
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe2e8f0), LV_PART_ITEMS | LV_STATE_PRESSED);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_cb_screen_password_frameware_password_keybord, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    style_keyboard_action_keys(obj);
                }
            }
        }
        {
            // پیام خطا در صورت رمز اشتباه (تراز دقیق در وسط بالا در y=0 با فونت کلمه و رنگ قرمز)
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 10, 8);
            lv_obj_set_size(obj, 40, 40);
            lv_obj_add_event_cb(obj, event_handler_cb_screen_password_obj0, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff4e4e4e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, -8, -2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_bitmap_close);
                }
            }
        }
        {
            // WaitingPanle_2 (لودینگ بدون تداخل لودر با متن)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.waiting_panle_2 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // لودر چرخان با تراز دقیق در مرکز
                    lv_obj_t *sp = lv_spinner_create(parent_obj);
                    lv_spinner_set_anim_params(sp, 1000, 60);
                    objects.obj8 = sp;
                    lv_obj_set_size(sp, 76, 76);
                    lv_obj_align(sp, LV_ALIGN_CENTER, 0, -32);
                    lv_obj_set_style_arc_color(sp, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(sp, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(sp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(sp, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(sp, lv_color_hex(0xff0284c7), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // متن وضعیت زیر لودر با فاصله کافی و عدم تداخل
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.obj7 = lbl;
                    lv_obj_set_size(lbl, 460, LV_SIZE_CONTENT);
                    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 48);
                    lv_label_set_text(lbl, "");
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        // انتخاب‌گر زبان (سفارشی) — گوشه بالا سمت راست (58x58 در x=414, y=6)
        add_language_selector(parent_obj);
        // کلید صدا / بلندگو (سفارشی) — گوشه بالا سمت چپ (متقارن با پرچم: 58x58 در x=8, y=6)
        add_sound_toggle_button(parent_obj);
        // ساخت مودال راهنمای سیستم و رمزها (پیش‌فرض مخفی)
        create_guide_modal(parent_obj);
    }
    // اتصالِ خودکار قطع شد؛ ورودِ ارقام/پاک/تایید دستی در event_handler مدیریت می‌شود
    lv_keyboard_set_textarea(objects.frameware_password_keybord, NULL);

    // نمایش کارت هشدار سلامت سخت‌افزار در صورت شناسایی خطا در تست اولیه
    hwtest_show_boot_alert_if_needed(objects.screen_password);

    tick_screen_screen_password();
}

void delete_screen_screen_password() {
    lv_obj_del(objects.screen_password);
    // پنلِ انتخابِ زبان و پس‌زمینه لمس فرزندِ همین صفحه است؛ اشاره‌گرها معلق نشود
    g_lang_select_panel = NULL;
    g_lang_backdrop = NULL;
    g_lang_btn_img = NULL;
    g_sound_btn_lbl = NULL;
    g_guide_modal = NULL;
    g_guide_backdrop = NULL;
    g_guide_title = NULL;
    for (int i = 0; i < 4; i++) g_guide_items[i] = NULL;
    g_guide_close_lbl = NULL;
    objects.screen_password = 0;
    objects.panle_wifi_password_1 = 0;
    objects.obj5 = 0;
    objects.frameware_password_keybord = 0;
    objects.text_area_framware_password = 0;
    objects.obj6 = 0;
    objects.obj0 = 0;
    objects.waiting_panle_2 = 0;
    objects.obj7 = 0;
    objects.obj8 = 0;
    deletePageFlowState(1);
}

void tick_screen_screen_password() {
    if (!objects.screen_password || !lv_obj_is_valid(objects.screen_password)) return;
    static Lang s_last_guide_lang = (Lang)-1;
    if (s_last_guide_lang != g_lang) {
        s_last_guide_lang = g_lang;
        update_guide_modal_lang();
    }
    void *flowState = getFlowState(0, 1);
    if (!flowState) return;
    if (objects.text_area_framware_password && lv_obj_is_valid(objects.text_area_framware_password)) {
        const char *new_val = evalTextProperty(flowState, 4, 3, "Failed to evaluate Text in Textarea widget");
        const char *cur_val = lv_textarea_get_text(objects.text_area_framware_password);
        if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.text_area_framware_password;
            lv_textarea_set_text(objects.text_area_framware_password, new_val);
            tick_value_change_obj = NULL;
        }
    }
    if (objects.obj6 && lv_obj_is_valid(objects.obj6)) {
        bool new_val = evalBooleanProperty(flowState, 5, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.obj6, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj6;
            if (new_val) {
                lv_obj_add_flag(objects.obj6, LV_OBJ_FLAG_HIDDEN);
                if (objects.obj5 && lv_obj_is_valid(objects.obj5)) lv_obj_clear_flag(objects.obj5, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_flag(objects.obj6, LV_OBJ_FLAG_HIDDEN);
                if (objects.obj5 && lv_obj_is_valid(objects.obj5)) lv_obj_add_flag(objects.obj5, LV_OBJ_FLAG_HIDDEN);
            }
            tick_value_change_obj = NULL;
        }
        const char *new_val_lbl = evalTextProperty(flowState, 5, 4, "Failed to evaluate Text in Label widget");
        const char *cur_val_lbl = lv_label_get_text(objects.obj6);
        if (new_val_lbl && cur_val_lbl && strcmp(new_val_lbl, cur_val_lbl) != 0) {
            tick_value_change_obj = objects.obj6;
            lv_label_set_text(objects.obj6, new_val_lbl);
            tick_value_change_obj = NULL;
        }
    }
    if (objects.waiting_panle_2 && lv_obj_is_valid(objects.waiting_panle_2)) {
        bool is_waiting = get_var_admin_waiting_password_stutus();
        bool is_hidden = lv_obj_has_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
        if (is_waiting && is_hidden) {
            tick_value_change_obj = objects.waiting_panle_2;
            lv_obj_clear_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        } else if (!is_waiting && !is_hidden) {
            tick_value_change_obj = objects.waiting_panle_2;
            lv_obj_add_flag(objects.waiting_panle_2, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    if (objects.obj7 && lv_obj_is_valid(objects.obj7)) {
        const char *new_val = evalTextProperty(flowState, 9, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj7);
        if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj7;
            lv_label_set_text(objects.obj7, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_screen_wifi() {
    i18n_reset();
    config_start_wifi_scan();
    void *flowState = getFlowState(0, 2);
    lv_obj_t *obj = lv_obj_create(0);
    objects.screen_wifi = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // دکمه بازگشت به صفحه رمز (بالا سمت چپ)
            lv_obj_t *btn_back = lv_btn_create(parent_obj);
            lv_obj_set_pos(btn_back, 8, 8);
            lv_obj_set_size(btn_back, 44, 40);
            lv_obj_add_event_cb(btn_back, event_handler_wifi_back_to_password, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(btn_back, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_back, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(btn_back, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl = lv_label_create(btn_back);
                lv_label_set_text(lbl, LV_SYMBOL_LEFT);
                lv_obj_center(lbl);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
        {
            // آیکون وای‌فای در هدر
            lv_obj_t *wi = lv_img_create(parent_obj);
            lv_obj_set_pos(wi, 226, 12);
            lv_obj_set_size(wi, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(wi, &img_bitmap_wifi);
        }
        {
            // عنوان صفحه (با مارجین چپ و راست برای عدم برخورد با دکمه بازگشت)
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, 56, 54);
            lv_obj_set_size(obj, 368, LV_SIZE_CONTENT);
            lv_label_set_text(obj, tr(STR_SELECT_WIFI));
            i18n_register(obj, STR_SELECT_WIFI);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // دکمه تازه‌سازی/اسکن شبکه‌ها (آیکون و متن واضح «جستجو»)
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.btn_wifi_redresh = obj;
            lv_obj_set_pos(obj, 12, 94);
            lv_obj_set_size(obj, 96, 46);
            lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_btn_wifi_redresh, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl = lv_label_create(obj);
                lv_label_set_text(lbl, LV_SYMBOL_REFRESH " جستجو");
                lv_obj_center(lbl);
                lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
        {
            // دراپ‌داون لیست شبکه‌های وای‌فای
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.drop_down_wifi_list = obj;
            lv_obj_set_pos(obj, 114, 94);
            lv_obj_set_size(obj, 354, 46);
            lv_dropdown_set_options(obj, "Searching...");
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_drop_down_wifi_list, LV_EVENT_ALL, flowState);
        }
        {
            // دکمه شیک اتصال به وای‌فای
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.btn_wifi_connect = obj;
            lv_obj_set_pos(obj, 130, 160);
            lv_obj_set_size(obj, 220, 48);
            lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_btn_wifi_connect, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DISABLED);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, tr(STR_CONNECT));
                    i18n_register(obj, STR_CONNECT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // پیام وضعیت یا خطای اتصال
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj16 = obj;
            lv_obj_set_pos(obj, 0, 226);
            lv_obj_set_size(obj, 480, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc2626), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // پنجره ورود رمز شبکه وای‌فای (Modal Dialog با جابجایی ۷ پیکسل بالا و ۱۵ پیکسل چپ)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.panle_wifi_password = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_panle_wifi_password, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_opa(obj, 245, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // دکمه بازگشت پنجره رمز (بالا سمت چپ با آیکون فلش)
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.obj1 = obj;
                    lv_obj_set_pos(obj, 8, 8);
                    lv_obj_set_size(obj, 44, 40);
                    lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_obj1, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *lbl = lv_label_create(obj);
                        lv_label_set_text(lbl, LV_SYMBOL_LEFT);
                        lv_obj_center(lbl);
                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                {
                    // عنوان پنجره رمز (شیفت ۲۳ پیکسل چپ جهت تراز دقیق در وسط کادر رمز)
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj10 = obj;
                    lv_obj_set_pos(obj, 33, 12);
                    lv_obj_set_size(obj, 368, LV_SIZE_CONTENT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
                    lv_label_set_text(obj, "");
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // کادر ورود رمز وای‌فای (شیفت ۷ پیکسل بالاتر به y=40)
                    lv_obj_t *obj = lv_textarea_create(parent_obj);
                    objects.text_area_wifi_password = obj;
                    lv_obj_set_pos(obj, 65, 40);
                    lv_obj_set_size(obj, 270, 46);
                    lv_textarea_set_max_length(obj, 32);
                    lv_textarea_set_one_line(obj, true);
                    lv_textarea_set_password_mode(obj, true);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_text_area_wifi_password, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                // دکمه چشم رمز وای‌فای (شیفت ۷ پیکسل بالاتر به y=40)
                add_password_eye(parent_obj, objects.text_area_wifi_password, 341, 40);
                {
                    // کیبرد کامل حروف و اعداد با تم لایت سریع و روان بدون لگ (شیفت ۷ پیکسل بالاتر به y=96)
                    lv_obj_t *obj = lv_keyboard_create(parent_obj);
                    objects.keybord = obj;
                    lv_obj_set_pos(obj, -11, 96);
                    lv_obj_set_size(obj, 472, 206);
                    lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_TEXT_LOWER);
                    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

                    // استایل کلیدها (حذف سایه نرم‌افزاری برای ارتقای رندر به ۶۰ فریم بر ثانیه)
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe2e8f0), LV_PART_ITEMS | LV_STATE_PRESSED);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_cb_screen_wifi_keybord, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    style_keyboard_action_keys(obj);
                }
            }
        }
        {
            // پنجره وضعیت انتظار اتصال (Waiting Dialog با تفکیک کامل لودر از متن)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.waiting_panle = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // لودر چرخان بدون تداخل
                    lv_obj_t *sp = lv_spinner_create(parent_obj);
                    lv_spinner_set_anim_params(sp, 1000, 60);
                    objects.obj12 = sp;
                    lv_obj_set_size(sp, 76, 76);
                    lv_obj_align(sp, LV_ALIGN_CENTER, 0, -32);
                    lv_obj_set_style_arc_color(sp, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(sp, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(sp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(sp, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(sp, lv_color_hex(0xff0284c7), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // متن وضعیت زیر لودر با فاصله کافی
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.obj11 = lbl;
                    lv_obj_set_size(lbl, 460, LV_SIZE_CONTENT);
                    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 48);
                    lv_label_set_text(lbl, "");
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        // دکمه انتخاب زبان گوشه بالا سمت راست
        add_language_selector(parent_obj);
    }
    lv_keyboard_set_textarea(objects.keybord, objects.text_area_wifi_password);
    if (get_var_is_admin_password_verifyed() && get_var_is_wifi_connected()) {   // تا وصل نشده وارد بروزرسانی نشو
        lv_async_call(async_open_firmware_from_wifi, NULL);
    }
    
    tick_screen_screen_wifi();
}

void delete_screen_screen_wifi() {
    lv_obj_del(objects.screen_wifi);
    objects.screen_wifi = 0;
    objects.obj9 = 0;
    objects.drop_down_wifi_list = 0;
    objects.btn_wifi_connect = 0;
    objects.obj16 = 0;
    objects.btn_wifi_redresh = 0;
    objects.panle_wifi_password = 0;
    objects.obj1 = 0;
    objects.obj10 = 0;
    objects.keybord = 0;
    objects.text_area_wifi_password = 0;
    objects.waiting_panle = 0;
    objects.obj11 = 0;
    objects.obj12 = 0;
    objects.language_panle = 0;
    objects.obj13 = 0;
    objects.obj14 = 0;
    objects.obj15 = 0;
    deletePageFlowState(2);
}

void tick_screen_screen_wifi() {
    if (!objects.screen_wifi || !lv_obj_is_valid(objects.screen_wifi)) return;

    // ==== پیامِ سبزِ «متصل شد: …» — پرچم از app_task؛ اعمال/پاک‌سازی در همین نخِ LVGL (~۳ ثانیه) ====
    static uint32_t wifiOkUntil = 0;
    bool wifiOkActive = false;
    if (g_wifi_ok_msg) {
        g_wifi_ok_msg = false;
        wifiOkUntil = lv_tick_get() + 3000;
        if (objects.obj16 && lv_obj_is_valid(objects.obj16)) {
            char okmsg[96];
            snprintf(okmsg, sizeof(okmsg), "متصل شد: %s", g_wifi_ok_ssid);
            lv_label_set_text(objects.obj16, okmsg);
            lv_obj_set_style_text_color(objects.obj16, lv_color_hex(0xff4caf50), LV_PART_MAIN | LV_STATE_DEFAULT);   // سبز
        }
    }
    if (wifiOkUntil) {
        if ((int32_t)(lv_tick_get() - wifiOkUntil) >= 0) {
            wifiOkUntil = 0;
            if (objects.obj16 && lv_obj_is_valid(objects.obj16)) {
                // برگشت به رنگِ زردِ استایلِ خطا + متنِ متغیر (معمولاً خالی)
                lv_obj_set_style_text_color(objects.obj16, lv_color_hex(0xffffc800), LV_PART_MAIN | LV_STATE_DEFAULT);
                const char *err_lbl = get_var_connection_error_label();
                lv_label_set_text(objects.obj16, err_lbl ? err_lbl : "");
            }
        } else {
            wifiOkActive = true;   // پیام هنوز در زمانِ خودش است
        }
    }

    if (get_var_is_admin_password_verifyed() && get_var_is_wifi_connected() && !wifiOkActive) {   // فقط با اتصالِ وای‌فای وارد بروزرسانی شو (بعد از نمایشِ پیامِ موفقیت)
        eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
        return;
    }
    void *flowState = getFlowState(0, 2);
    if (!flowState) return;
    {
        update_wifi_dropdown_options_direct();
    }
    {
        if (objects.drop_down_wifi_list && lv_obj_is_valid(objects.drop_down_wifi_list)) {
            if (!(lv_obj_get_state(objects.drop_down_wifi_list) & LV_STATE_EDITED)) {
                int32_t new_val = evalIntegerProperty(flowState, 3, 4, "Failed to evaluate Selected in Dropdown widget");
                int32_t cur_val = lv_dropdown_get_selected(objects.drop_down_wifi_list);
                if (new_val != cur_val) {
                    tick_value_change_obj = objects.drop_down_wifi_list;
                    lv_dropdown_set_selected(objects.drop_down_wifi_list, new_val);
                    tick_value_change_obj = NULL;
                }
            }
        }
    }
    {
        if (objects.btn_wifi_connect && lv_obj_is_valid(objects.btn_wifi_connect)) {
            bool new_val = evalBooleanProperty(flowState, 4, 3, "Failed to evaluate Disabled state");
            bool cur_val = lv_obj_has_state(objects.btn_wifi_connect, LV_STATE_DISABLED);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.btn_wifi_connect;
                if (new_val) lv_obj_add_state(objects.btn_wifi_connect, LV_STATE_DISABLED);
                else lv_obj_clear_state(objects.btn_wifi_connect, LV_STATE_DISABLED);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.obj16 && lv_obj_is_valid(objects.obj16)) {
            const char *new_val = evalTextProperty(flowState, 6, 3, "Failed to evaluate Text in Label widget");
            const char *cur_val = lv_label_get_text(objects.obj16);
            if (!wifiOkActive && new_val && cur_val && strcmp(new_val, cur_val) != 0) {   // در زمانِ پیامِ «متصل شد» بازنویسی نشود
                tick_value_change_obj = objects.obj16;
                lv_label_set_text(objects.obj16, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.obj10 && lv_obj_is_valid(objects.obj10)) {
            const char *new_val = evalTextProperty(flowState, 12, 3, "Failed to evaluate Text in Label widget");
            const char *cur_val = lv_label_get_text(objects.obj10);
            if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
                tick_value_change_obj = objects.obj10;
                lv_label_set_text(objects.obj10, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.text_area_wifi_password && lv_obj_is_valid(objects.text_area_wifi_password)) {
            const char *new_val = evalTextProperty(flowState, 14, 3, "Failed to evaluate Text in Textarea widget");
            const char *cur_val = lv_textarea_get_text(objects.text_area_wifi_password);
            if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
                tick_value_change_obj = objects.text_area_wifi_password;
                lv_textarea_set_text(objects.text_area_wifi_password, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.waiting_panle && lv_obj_is_valid(objects.waiting_panle)) {
            bool new_val = evalBooleanProperty(flowState, 15, 3, "Failed to evaluate Hidden flag");
            bool cur_val = lv_obj_has_flag(objects.waiting_panle, LV_OBJ_FLAG_HIDDEN);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.waiting_panle;
                if (new_val) lv_obj_add_flag(objects.waiting_panle, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_clear_flag(objects.waiting_panle, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.obj11 && lv_obj_is_valid(objects.obj11)) {
            const char *new_val = evalTextProperty(flowState, 16, 3, "Failed to evaluate Text in Label widget");
            const char *cur_val = lv_label_get_text(objects.obj11);
            if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
                tick_value_change_obj = objects.obj11;
                lv_label_set_text(objects.obj11, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    if (objects.keybord && lv_obj_is_valid(objects.keybord)) {
        float timeline_position = getTimelinePosition(flowState);
        
        static struct {
            float last_timeline_position;
            int32_t obj_screen_wifi_y_init_value;
            int32_t obj_keybord_y_init_value;
        } anim_state = { -1 };
        
        if (anim_state.last_timeline_position == -1) {
            anim_state.last_timeline_position = 0;
            anim_state.obj_screen_wifi_y_init_value = lv_obj_get_style_prop(objects.screen_wifi, LV_PART_MAIN, LV_STYLE_Y).num;
            anim_state.obj_keybord_y_init_value = lv_obj_get_style_prop(objects.keybord, LV_PART_MAIN, LV_STYLE_Y).num;
        }
        
        if (timeline_position != anim_state.last_timeline_position) {
            anim_state.last_timeline_position = timeline_position;
            
            {
                lv_obj_t *obj = objects.screen_wifi;
                
                float y_value = anim_state.obj_screen_wifi_y_init_value;
                
                while (1) {
                    // keyframe #1
                    if (timeline_position < 0) {
                        break;
                    }
                    y_value = 150;
                    
                    break;
                }
                
                lv_style_value_t value;
                
                value.num = (int32_t)roundf(y_value);
                lv_obj_set_local_style_prop(obj, LV_STYLE_Y, value, LV_PART_MAIN);
            }
            {
                lv_obj_t *obj = objects.keybord;
                
                float y_value = anim_state.obj_keybord_y_init_value;
                
                while (1) {
                    // keyframe #1
                    if (timeline_position < 0) {
                        break;
                    }
                    y_value = 320;
                    
                    // keyframe #2
                    if (timeline_position >= 0 && timeline_position < 1) {
                        float t = (timeline_position - 0) / 1;
                        // y
                        {
                            float t2 = eez_linear(t);
                            y_value = (1 - t2) * y_value + t2 * 160;
                        }
                        break;
                    }
                    y_value = 160;
                    
                    break;
                }
                
                lv_style_value_t value;
                
                value.num = (int32_t)roundf(y_value);
                lv_obj_set_local_style_prop(obj, LV_STYLE_Y, value, LV_PART_MAIN);
            }
        }
    }
}

// ===== چیدمان جدید: هندلرها + سازنده‌ی کارتِ عمودی =====
extern void action_card_install(lv_event_t *e);
extern void action_confirm_yes(lv_event_t *e);
extern void action_confirm_no(lv_event_t *e);
extern void action_link_entry_open(lv_event_t *e);
extern void action_link_confirm(lv_event_t *e);
extern void action_link_check(lv_event_t *e);
extern const char* link_check_get_result(void);
extern int link_check_get_status(void);
extern bool link_check_is_done(void);
extern void link_check_clear_done(void);
extern void action_link_cancel(lv_event_t *e);
extern void action_version_open(lv_event_t *e);
extern void action_version_close(lv_event_t *e);
extern void action_download_cancel(lv_event_t *e);

static void event_handler_card_install(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_card_install(e);
    }
}
static void event_handler_download_cancel(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_download_cancel(e);
    }
}
static void event_handler_confirm_yes(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_confirm_yes(e);
    }
}
static void event_handler_confirm_no(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_confirm_no(e);
    }
}
static void event_handler_link_entry_open(lv_event_t *e) {
    lv_event_code_t c = lv_event_get_code(e);
    if (c == LV_EVENT_SHORT_CLICKED || c == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_link_entry_open(e);
    }
}
static void event_handler_link_confirm(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_link_confirm(e);
    }
}
static uint32_t s_link_msg_hide_ms = 0;

void link_entry_show_message(const char *msg, uint32_t bg_hex, uint32_t text_hex, uint32_t border_hex, uint32_t duration_ms) {
    if (!objects.link_entry_error || !lv_obj_is_valid(objects.link_entry_error)) return;
    lv_label_set_text(objects.link_entry_error, msg ? msg : "");
    lv_obj_set_style_bg_color(objects.link_entry_error, lv_color_hex(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.link_entry_error, lv_color_hex(text_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.link_entry_error, lv_color_hex(border_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(objects.link_entry_error, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.link_entry_error);
    if (duration_ms > 0) {
        s_link_msg_hide_ms = lv_tick_get() + duration_ms;
    } else {
        s_link_msg_hide_ms = 0;
    }
}

void link_entry_hide_message(void) {
    s_link_msg_hide_ms = 0;
    if (objects.link_entry_error && lv_obj_is_valid(objects.link_entry_error)) {
        lv_obj_add_flag(objects.link_entry_error, LV_OBJ_FLAG_HIDDEN);
    }
}

static void event_handler_link_cancel(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_link_cancel(e);
    }
}
static void event_handler_link_default(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        if (objects.link_entry_textarea && lv_obj_is_valid(objects.link_entry_textarea)) {
            lv_textarea_set_text(objects.link_entry_textarea, "http://ota.haffler-infi.ir/other/test/");
            lv_textarea_set_cursor_pos(objects.link_entry_textarea, LV_TEXTAREA_CURSOR_LAST);
            link_entry_hide_message();
        }
    }
}
static void event_handler_link_clear(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        if (objects.link_entry_textarea && lv_obj_is_valid(objects.link_entry_textarea)) {
            lv_textarea_set_text(objects.link_entry_textarea, "");
            link_entry_hide_message();
        }
    }
}
static void event_handler_link_textarea(lv_event_t *e) {
    link_entry_hide_message();
}
static void event_handler_link_error_click(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        link_entry_hide_message();
    }
}
static void event_handler_link_entry_keyboard(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if (code == LV_EVENT_READY) {
        app_sound_play(SOUND_TAP);
        action_link_check(e);
    } else if (code == LV_EVENT_CANCEL) {
        app_sound_play(SOUND_TAP);
        action_link_cancel(e);
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        app_sound_play(SOUND_TAP);
        uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
        const char *txt = lv_buttonmatrix_get_button_text(kb, btn_id);
        if (txt) {
            if (strcmp(txt, "ABC") == 0) {
                lv_buttonmatrix_set_map(kb, cfg_url_kb_map_uc);
                lv_buttonmatrix_set_ctrl_map(kb, cfg_url_kb_ctrl_uc);
                style_keyboard_action_keys(kb);
            } else if (strcmp(txt, "abc") == 0) {
                lv_buttonmatrix_set_map(kb, cfg_url_kb_map_lc);
                lv_buttonmatrix_set_ctrl_map(kb, cfg_url_kb_ctrl_lc);
                style_keyboard_action_keys(kb);
            }
        }
    }
}
static void event_handler_version_open(lv_event_t *e) {
    // فقط SHORT_CLICKED ثبت شده است: اسکرول/اسنپِ cards_row معمولاً CLICKED را می‌بلعد
    // ولی SHORT_CLICKED صادر می‌شود؛ ثبتِ هر دو، شورت‌تپ را دوبار اجرا می‌کرد
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_version_open(e);
    }
}
static void event_handler_version_close(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_version_close(e);
    }
}

static lv_obj_t *s_confirm_cancel_bar = NULL;
static lv_timer_t *s_confirm_cancel_timer = NULL;
static int32_t s_confirm_cancel_remaining_ms = 5000;
static bool s_confirm_cancel_paused = false;

static void confirm_touch_pause_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_confirm_cancel_paused = true;
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_confirm_cancel_paused = false;
    }
}

extern lv_indev_t *g_touch_indev;

static void confirm_cancel_timer_cb(lv_timer_t *timer) {
    (void)timer;
    // بررسی مستقیم سخت‌افزار تاچ‌پد: با کوچکترین تماس انگشت، تایمر ۱۰۰٪ ثابت می‌ماند
    lv_indev_t * indev = lv_indev_get_act();
    if (!indev) indev = g_touch_indev;
    if (indev) {
        lv_indev_state_t state = lv_indev_get_state(indev);
        if (state == LV_INDEV_STATE_PRESSED) {
            return; // تاچ نگه داشته شده -> توقف کامل شمارش
        }
    }
    if (s_confirm_cancel_paused) return; // توقف هنگام رویداد فشار

    s_confirm_cancel_remaining_ms -= 50;
    if (s_confirm_cancel_remaining_ms < 0) s_confirm_cancel_remaining_ms = 0;

    // نمایش زمان ثانیه به صورت عددی و شکیل در دکمه انصراف (حذف نوار ناهمگون دور دکمه)
    int sec = (s_confirm_cancel_remaining_ms + 999) / 1000;
    if (objects.confirm_no && lv_obj_is_valid(objects.confirm_no)) {
        lv_obj_t *lbl = lv_obj_get_child(objects.confirm_no, 0);
        if (lbl && lv_obj_is_valid(lbl)) {
            char b[32];
            snprintf(b, sizeof(b), "%s (%d)", tr(STR_NO), sec);
            lv_label_set_text(lbl, b);
        }
    }

    if (s_confirm_cancel_remaining_ms <= 0) {
        action_confirm_no(NULL);
    }
}

void firmware_confirm_countdown_start(void) {
    s_confirm_cancel_remaining_ms = 5000;
    s_confirm_cancel_paused = false;
    if (objects.confirm_no && lv_obj_is_valid(objects.confirm_no)) {
        lv_obj_t *lbl = lv_obj_get_child(objects.confirm_no, 0);
        if (lbl && lv_obj_is_valid(lbl)) {
            char b[32];
            snprintf(b, sizeof(b), "%s (5)", tr(STR_NO));
            lv_label_set_text(lbl, b);
        }
    }
    if (s_confirm_cancel_bar && lv_obj_is_valid(s_confirm_cancel_bar)) {
        lv_obj_add_flag(s_confirm_cancel_bar, LV_OBJ_FLAG_HIDDEN);
    }
    if (!s_confirm_cancel_timer) {
        s_confirm_cancel_timer = lv_timer_create(confirm_cancel_timer_cb, 50, NULL);
    }
    lv_timer_reset(s_confirm_cancel_timer);
    lv_timer_resume(s_confirm_cancel_timer);
}

void firmware_confirm_countdown_stop(void) {
    s_confirm_cancel_paused = false;
    if (s_confirm_cancel_timer) lv_timer_pause(s_confirm_cancel_timer);
    if (objects.confirm_no && lv_obj_is_valid(objects.confirm_no)) {
        lv_obj_t *lbl = lv_obj_get_child(objects.confirm_no, 0);
        if (lbl && lv_obj_is_valid(lbl)) {
            lv_label_set_text(lbl, tr(STR_NO));
        }
    }
    if (s_confirm_cancel_bar && lv_obj_is_valid(s_confirm_cancel_bar)) {
        lv_obj_add_flag(s_confirm_cancel_bar, LV_OBJ_FLAG_HIDDEN);
    }
}

// ===== نقشه‌ی پایه‌های ورودی/خروجی هر دستگاه به تفکیک ۵ زبان (EN, TR, FA, DE, RU) =====
// LANG_EN = 0, LANG_TR = 1, LANG_FA = 2, LANG_DE = 3, LANG_RU = 4
static const char *g_pinmap_in[LANG_COUNT][9] = {
    // LANG_EN (0)
    {
        "I0: Heater open\nI1: Heater closed\nI2: Clamp open\nI3: Clamp closed\nI4: Table open\nI5: Return\nI6: Emergency\nI7: Foot pedal",
        "I0: Left head ready\nI1: Right head ready\nI2: Clamp open\nI3: Clamp closed\nI4: 2-stage pedal\nI5: Emergency\nI6: Air pressure\nI7: Start switch",
        "I0: Origin sensor (Ref)\nI1: End sensor (Max)\nI2: Encoder Phase A\nI3: Encoder Phase B\nI4: Blade return\nI5: Motor fault\nI6: Emergency\nI7: Cut pedal",
        "I0: Left ref sensor\nI1: Right sensor\nI2: Driver fault\nI3: Blade 1 return\nI4: Blade 2 return\nI5: Air pressure\nI6: Emergency\nI7: Cut start",
        "I0: Glass entry sensor\nI1: Glass exit sensor\nI2: Oxygen analyzer\nI3: Gas pressure sw\nI4: Start pedal\nI5: Emergency\nI6: Sensor error\nI7: Reset",
        "I0: Position sensor\nI1: Fixture sensor\nI2: Top knife sensor\nI3: Bottom knife sensor\nI4: Clamp pedal\nI5: Cycle start\nI6: Emergency\nI7: Air pressure",
        "I0: Motor switch\nI1: Z-stroke sensor\nI2: X-stroke sensor\nI3: Left clamp\nI4: Right clamp\nI5: Foot pedal\nI6: Emergency\nI7: Speed switch",
        "I0: Upper die sensor\nI1: Lower die sensor\nI2: Hydraulic pressure sw\nI3: Oil level sensor\nI4: 2-step foot pedal\nI5: E-stop mushroom\nI6: Safety light curtain\nI7: Reset switch",
        "I0..I7: Opto-isolated inputs\nQ0..Q7: Industrial relay outputs\nRS485: Pins 17 & 18\nI2C: MCP23017 expander"
    },
    // LANG_TR (1)
    {
        "I0: Rezistans acik\nI1: Rezistans kapali\nI2: Sablon acik\nI3: Sablon kapali\nI4: Tabla acik\nI5: Geri donus\nI6: Acil stop\nI7: Ayak pedali",
        "I0: Sol kafa hazir\nI1: Sag kafa hazir\nI2: Sablon acik\nI3: Sablon kapali\nI4: 2 Kademeli pedal\nI5: Acil stop\nI6: Hava basinci\nI7: Baslat butonu",
        "I0: Baslangic sensoru\nI1: Bitis sensoru (Max)\nI2: Enkoder Faz A\nI3: Enkoder Faz B\nI4: Testere geri donus\nI5: Motor arizasi\nI6: Acil stop\nI7: Kesim pedali",
        "I0: Sol ref sensoru\nI1: Sag sensor\nI2: Surucu arizasi\nI3: Testere 1 donus\nI4: Testere 2 donus\nI5: Hava basinci\nI6: Acil stop\nI7: Kesim baslat",
        "I0: Cam giris sensoru\nI1: Cam cikis sensoru\nI2: Oksijen analizoru\nI3: Gaz basinc salteri\nI4: Baslatma pedali\nI5: Acil stop\nI6: Sensor hatasi\nI7: Sifirla",
        "I0: Konum sensoru\nI1: Fikstur sensoru\nI2: Ust bicak sensoru\nI3: Alt bicak sensoru\nI4: Baski pedali\nI5: Cevrim baslat\nI6: Acil stop\nI7: Hava basinci",
        "I0: Motor salteri\nI1: Z-kurs sensoru\nI2: X-kurs sensoru\nI3: Sol baski\nI4: Sag baski\nI5: Ayak pedali\nI6: Acil stop\nI7: Devir salteri",
        "I0: Ust kalip sensoru\nI1: Alt kalip sensoru\nI2: Hidrolik basinc\nI3: Yag seviyesi\nI4: Cift kademe pedal\nI5: Acil stop butonu\nI6: Isik bariyeri\nI7: Sifirlama",
        "I0..I7: Opto-izole PLC girisleri\nQ0..Q7: Endustriyel role cikislari\nRS485: Pin 17 & 18\nI2C: MCP23017 genisletme"
    },
    // LANG_FA (2)
    {
        "I0: المنت باز\nI1: المنت بسته\nI2: شابلون باز\nI3: شابلون بسته\nI4: میز باز\nI5: برگشت\nI6: اضطراری\nI7: پدال",
        "I0: سر چپ آماده\nI1: سر راست آماده\nI2: شابلون باز\nI3: شابلون بسته\nI4: پدال دو مرحله\nI5: اضطراری\nI6: فشار باد\nI7: کلید استارت",
        "I0: سنسور ابتدا (Ref)\nI1: سنسور انتها (Max)\nI2: انکودر فاز A\nI3: انکودر فاز B\nI4: برگشت تیغه\nI5: خطای موتور\nI6: اضطراری\nI7: پدال برش",
        "I0: سنسور چپ (Ref)\nI1: سنسور راست\nI2: خطای درایور\nI3: برگشت تیغه ۱\nI4: برگشت تیغه ۲\nI5: فشار باد\nI6: اضطراری\nI7: استارت برش",
        "I0: سنسور ورودی شیشه\nI1: سنسور خروجی\nI2: آنالایزر اکسیژن\nI3: پرشر سوییچ گاز\nI4: پدال استارت\nI5: اضطراری\nI6: خطای سنسور\nI7: ریست",
        "I0: سنسور موقعیت\nI1: سنسور فیکسچر\nI2: تیغ بالا\nI3: تیغ پایین\nI4: پدال گیره\nI5: استارت\nI6: اضطراری\nI7: فشار باد",
        "I0: میکروسوییچ موتور\nI1: سنسور کورس Z\nI2: سنسور کورس X\nI3: گیره چپ\nI4: گیره راست\nI5: پدال\nI6: اضطراری\nI7: کلید سرعت",
        "I0: سنسور قالب بالا\nI1: سنسور قالب پایین\nI2: پرشر هیدرولیک\nI3: سطح روغن\nI4: پدال دوپله\nI5: استپ قارچی\nI6: حفاظ ایمنی\nI7: ریست",
        "I0..I7: ورودی های ایزوله PLC\nQ0..Q7: رله های خروجی صنعتی\nRS485: پین ۱۷ و ۱۸\nI2C: توسعه MCP23017"
    },
    // LANG_DE (3)
    {
        "I0: Heizelement auf\nI1: Heizelement zu\nI2: Schablone auf\nI3: Schablone zu\nI4: Tisch auf\nI5: Ruckhub\nI6: Not-Halt\nI7: Fusspedal",
        "I0: Linker Kopf bereit\nI1: Rechter Kopf bereit\nI2: Schablone auf\nI3: Schablone zu\nI4: 2-Stufen-Pedal\nI5: Not-Halt\nI6: Luftdruck\nI7: Starttaster",
        "I0: Referenzsensor (Ref)\nI1: Endschalter (Max)\nI2: Encoder Phase A\nI3: Encoder Phase B\nI4: Sageblatt Ruckhub\nI5: Motorstorung\nI6: Not-Halt\nI7: Schnittpedal",
        "I0: Sensor links (Ref)\nI1: Sensor rechts\nI2: Antriebsfehler\nI3: Sageblatt 1 Ruckhub\nI4: Sageblatt 2 Ruckhub\nI5: Luftdruck\nI6: Not-Halt\nI7: Schnittstart",
        "I0: Glaseinlauf-Sensor\nI1: Glasauslauf-Sensor\nI2: Sauerstoffanalysator\nI3: Gasdruckschalter\nI4: Startpedal\nI5: Not-Halt\nI6: Sensorfehler\nI7: Reset",
        "I0: Positionssensor\nI1: Vorrichtungssensor\nI2: Messer oben Sensor\nI3: Messer unten Sensor\nI4: Spannpedal\nI5: Starttaster\nI6: Not-Halt\nI7: Druckluft",
        "I0: Motorschalter\nI1: Hubsensor Z\nI2: Hubsensor X\nI3: Spanner links\nI4: Spanner rechts\nI5: Fusspedal\nI6: Not-Halt\nI7: Drehzahlschalter",
        "I0: Werkzeug oben Sensor\nI1: Werkzeug unten Sensor\nI2: Hydraulikdruckschalter\nI3: Olstandsensor\nI4: 2-Stufen-Fussschalter\nI5: Not-Aus-Taster\nI6: Sicherheitslichtschranke\nI7: Reset-Taster",
        "I0..I7: Opto-entkoppelte Eingange\nQ0..Q7: Industrierelais-Ausgange\nRS485: Pins 17 & 18\nI2C: MCP23017-Erweiterung"
    },
    // LANG_RU (4)
    {
        "I0: Nagrevatel otkr\nI1: Nagrevatel zakr\nI2: Shablon otkr\nI3: Shablon zakr\nI4: Stol otkr\nI5: Vozvrat\nI6: Avariyniy stop\nI7: Pedal",
        "I0: Lev. golovka gotova\nI1: Prav. golovka gotova\nI2: Shablon otkr\nI3: Shablon zakr\nI4: 2-stup. pedal\nI5: Avariyniy stop\nI6: Davlenie vozduha\nI7: Pusk",
        "I0: Datchik nulya (Ref)\nI1: Kontsevoy datchik\nI2: Enkoder Faza A\nI3: Enkoder Faza B\nI4: Vozvrat pily\nI5: Oshibka dvigatelya\nI6: Avariyniy stop\nI7: Pedal reza",
        "I0: Lev. datchik (Ref)\nI1: Prav. datchik\nI2: Sboy privoda\nI3: Vozvrat pily 1\nI4: Vozvrat pily 2\nI5: Davlenie vozduha\nI6: Avariyniy stop\nI7: Pusk reza",
        "I0: Datchik vhoda stekla\nI1: Datchik vyhoda\nI2: Gazoanalizator O2\nI3: Datchik davleniya gaza\nI4: Pedal starta\nI5: Avariyniy stop\nI6: Oshibka datchika\nI7: Sbros",
        "I0: Datchik pozitsii\nI1: Datchik prizhima\nI2: Verkhniy nozh\nI3: Nizhniy nozh\nI4: Pedal zazhima\nI5: Zapusk tsikla\nI6: Avariyniy stop\nI7: Davlenie vozduha",
        "I0: Kontsevik motora\nI1: Khod Z\nI2: Khod X\nI3: Prizhim levy\nI4: Prizhim pravy\nI5: Pedal\nI6: Avariyniy stop\nI7: Pereklyuchatel skorosti",
        "I0: Shtamp vverhu\nI1: Shtamp vnizu\nI2: Davlenie gidravliki\nI3: Uroven masla\nI4: 2-stup. pedal\nI5: Gribok avarii\nI6: Baryer bezopasnosti\nI7: Sbros",
        "I0..I7: Optoizolirovannye vhody\nQ0..Q7: Releynye vyhody\nRS485: Vyvody 17 i 18\nI2C: Modul MCP23017"
    }
};

static const char *g_pinmap_out[LANG_COUNT][9] = {
    // LANG_EN (0)
    {
        "Q0: Table cylinder\nQ1: Left clamp\nQ2: Right clamp\nQ3: Heater cylinder\nQ4: Profile clamp\nQ5: Heater SSR\nQ6: Pulse 1\nQ7: Pulse 2",
        "Q0: Left clamp\nQ1: Right clamp\nQ2: Left heater\nQ3: Right heater\nQ4: Heater SSR\nQ5: Moving head cyl\nQ6: Pneumatic brake\nQ7: Alarm buzzer",
        "Q0: Horizontal clamp\nQ1: Vertical clamp\nQ2: Saw blade feed\nQ3: Auto lube\nQ4: Dust vacuum\nQ5: Motor brake\nQ6: Buzzer\nQ7: Status lamp",
        "Q0: Left clamp\nQ1: Right clamp\nQ2: Left saw blade\nQ3: Right saw blade\nQ4: Dust collector\nQ5: Pneumatic lock\nQ6: Angle selector\nQ7: Fault alarm",
        "Q0: Gas inject valve\nQ1: Gas exhaust valve\nQ2: Vacuum pump\nQ3: Glass clamp\nQ4: Sensor valve\nQ5: Completion chime\nQ6: Safety relay\nQ7: Signal lamp",
        "Q0: Profile clamp\nQ1: Top knife motor\nQ2: Bottom knife motor\nQ3: Water-slot drill\nQ4: Air blow nozzle\nQ5: Carriage feed\nQ6: Buzzer\nQ7: Work lamp",
        "Q0: Main router spindle\nQ1: Pneumatic clamps\nQ2: Mist coolant spray\nQ3: Template lock\nQ4: Chip collector\nQ5: Electric brake\nQ6: Buzzer\nQ7: Work light",
        "Q0: Down hydraulic valve\nQ1: Up hydraulic valve\nQ2: Hydraulic motor pump\nQ3: Oil cooler fan\nQ4: Workpiece clamp\nQ5: Positioning pin\nQ6: Warning siren\nQ7: Signal tower",
        "Q0..Q7: High-current relays\nStepper pulse: High freq\nBuzzer: PWM Pin 45\nStatus LED: Onboard indicator"
    },
    // LANG_TR (1)
    {
        "Q0: Tabla pnomatik\nQ1: Sol baski\nQ2: Sag baski\nQ3: Rezistans pistonu\nQ4: Profil sablonu\nQ5: Isitici SSR\nQ6: Darbe 1\nQ7: Darbe 2",
        "Q0: Sol baski\nQ1: Sag baski\nQ2: Sol rezistans\nQ3: Sag rezistans\nQ4: Isitici SSR\nQ5: Hareketli kafa\nQ6: Havali fren\nQ7: Uyari zili",
        "Q0: Yatay baski\nQ1: Dikey baski\nQ2: Testere hareketi\nQ3: Otomatik yaglama\nQ4: Talas emici\nQ5: Motor freni\nQ6: Sesli ikaz\nQ7: Durum lambasi",
        "Q0: Sol baski\nQ1: Sag baski\nQ2: Sol testere\nQ3: Sag testere\nQ4: Talas emici\nQ5: Havali kilit\nQ6: Aci ayari\nQ7: Hata alarmi",
        "Q0: Gaz dolum valfi\nQ1: Gaz tahliye valfi\nQ2: Vakum pompasi\nQ3: Cam tutucu\nQ4: Sensor valfi\nQ5: Bitis sinyali\nQ6: Emniyet rolesi\nQ7: Sinyal lambasi",
        "Q0: Profil baskisi\nQ1: Ust bicak motoru\nQ2: Alt bicak motoru\nQ3: Su tahliye matkabi\nQ4: Hava nozulu\nQ5: Tasiyici hareket\nQ6: Sesli ikaz\nQ7: Calisma lambasi",
        "Q0: Freze motoru\nQ1: Pnomatik baski\nQ2: Sogutma spreyi\nQ3: Sablon kilidi\nQ4: Talas emici\nQ5: Elektrikli fren\nQ6: Ikaz zili\nQ7: Aydinlatma",
        "Q0: Asagi hidrolik valfi\nQ1: Yukari hidrolik valfi\nQ2: Hidrolik pompa\nQ3: Yag sogutucu fan\nQ4: Parca baskisi\nQ5: Konum pimi\nQ6: Siren\nQ7: Tepe lambasi",
        "Q0..Q7: Role cikislari\nStep pals: Yuksek frekans\nBuzzer: PWM Pin 45\nDurum LEDi: Kart uzeri"
    },
    // LANG_FA (2)
    {
        "Q0: میز\nQ1: پرس چپ\nQ2: پرس راست\nQ3: المنت\nQ4: شابلون\nQ5: SSR المنت\nQ6: ضربه ۱\nQ7: ضربه ۲",
        "Q0: گیره چپ\nQ1: گیره راست\nQ2: المنت چپ\nQ3: المنت راست\nQ4: هیتر SSR\nQ5: جک سر متحرک\nQ6: ترمز بادی\nQ7: بازر اخطار",
        "Q0: گیره افقی\nQ1: گیره عمودی\nQ2: حرکت تیغه اره\nQ3: روغن کاری اتومات\nQ4: مکنده براده\nQ5: ترمز موتور\nQ6: بازر\nQ7: چراغ وضعیت",
        "Q0: گیره چپ\nQ1: گیره راست\nQ2: تیغه چپ\nQ3: تیغه راست\nQ4: مکنده براده\nQ5: قفل بادی\nQ6: تنظیم زاویه\nQ7: آلارم خطا",
        "Q0: شیر برقی تزریق\nQ1: شیر برقی تخلیه\nQ2: پمپ وکیوم\nQ3: گیره شیشه\nQ4: شیر سنسور\nQ5: بوق اتمام کار\nQ6: رله ایمنی\nQ7: چراغ سیگنال",
        "Q0: گیره پروفیل\nQ1: موتور تیغ بالا\nQ2: موتور تیغ پایین\nQ3: مته سوراخ کاری\nQ4: نازل باد\nQ5: حرکت کالسکه\nQ6: بازر\nQ7: چراغ کار",
        "Q0: موتور فرز اصلی\nQ1: گیره پنوماتیک\nQ2: اسپری آب صابون\nQ3: قفل شابلون\nQ4: مکنده براده\nQ5: ترمز الکتریکی\nQ6: بازر\nQ7: روشنایی",
        "Q0: شیر هیدرولیک پایین\nQ1: شیر هیدرولیک بالا\nQ2: پمپ هیدرولیک\nQ3: فن خنک کن\nQ4: گیره قطعه کار\nQ5: پین موقعیت\nQ6: آلارم\nQ7: برجک نوری",
        "Q0..Q7: خروجی رله ها\nپالس استپر: فرکانس بالا\nبازر: خروجی PWM پین ۴۵\nLED وضعیت: پین روی برد"
    },
    // LANG_DE (3)
    {
        "Q0: Tischzylinder\nQ1: Spanner links\nQ2: Spanner rechts\nQ3: Heizelement Hub\nQ4: Schablonenhalter\nQ5: Heizung SSR\nQ6: Impuls 1\nQ7: Impuls 2",
        "Q0: Spanner links\nQ1: Spanner rechts\nQ2: Heizelement links\nQ3: Heizelement rechts\nQ4: Heizung SSR\nQ5: Fahrkopfzylinder\nQ6: Pneumatikbremse\nQ7: Alarmbuzzer",
        "Q0: Horizontalspanner\nQ1: Vertikalspanner\nQ2: Sageblattvorschub\nQ3: Automatische Schmierung\nQ4: Spaneabsaugung\nQ5: Motorbremse\nQ6: Summer\nQ7: Statusleuchte",
        "Q0: Spanner links\nQ1: Spanner rechts\nQ2: Sageblatt links\nQ3: Sageblatt rechts\nQ4: Spaneabsaugung\nQ5: Druckluftbremse\nQ6: Winkelverstellung\nQ7: Storalarm",
        "Q0: Gasfullventil\nQ1: Entluftungsventil\nQ2: Vakuumpumpe\nQ3: Glasspanner\nQ4: Sensorventil\nQ5: Signalton Fertig\nQ6: Sicherheitsrelais\nQ7: Signallampe",
        "Q0: Profilspanner\nQ1: Obermesser-Motor\nQ2: Untermesser-Motor\nQ3: Wasserschlitzbohrer\nQ4: Ausblasduse\nQ5: Schlittenvorschub\nQ6: Summer\nQ7: Arbeitsleuchte",
        "Q0: Frasmotor\nQ1: Pneumatikspanner\nQ2: Kuhlspruhung\nQ3: Kopierschablonen-Arretierung\nQ4: Spaneabsaugung\nQ5: Motorbremse\nQ6: Summer\nQ7: Beleuchtung",
        "Q0: Hydraulikventil ab\nQ1: Hydraulikventil auf\nQ2: Hydraulikpumpe\nQ3: Olkuhlerlufter\nQ4: Werkstuckspanner\nQ5: Zentrierstift\nQ6: Alarmsirene\nQ7: Signalsaule",
        "Q0..Q7: Relaisausgange\nSchrittmotor-Puls: Hochfrequenz\nSummer: PWM-Pin 45\nStatus-LED: Onboard-Anzeige"
    },
    // LANG_RU (4)
    {
        "Q0: Pnevmostol\nQ1: Prizhim levy\nQ2: Prizhim pravy\nQ3: Nagrevatel\nQ4: Prizhim profilya\nQ5: SSR nagrevatelya\nQ6: Impuls 1\nQ7: Impuls 2",
        "Q0: Prizhim levy\nQ1: Prizhim pravy\nQ2: Nagrevatel lev\nQ3: Nagrevatel prav\nQ4: SSR nagrevatelya\nQ5: Tsilindr golovki\nQ6: Pnevmotormoz\nQ7: Zummer avarii",
        "Q0: Goriz. prizhim\nQ1: Vert. prizhim\nQ2: Podacha diska pily\nQ3: Avtosmazka\nQ4: Struzhkootdos\nQ5: Tormoz motora\nQ6: Zummer\nQ7: Lampa sostoyaniya",
        "Q0: Prizhim levy\nQ1: Prizhim pravy\nQ2: Disk pily lev\nQ3: Disk pily prav\nQ4: Struzhkootdos\nQ5: Pnevmozamok\nQ6: Povorot ugla\nQ7: Avariya",
        "Q0: Klapan vpryska gaza\nQ1: Klapan sbrosa\nQ2: Vakuumny nasos\nQ3: Prizhim stekla\nQ4: Klapan datchika\nQ5: Zummer gotovnosti\nQ6: Rele zashchity\nQ7: Svetovoy mayak",
        "Q0: Zazhim profilya\nQ1: Motor verhnego nozha\nQ2: Motor nizhnego nozha\nQ3: Freza drenazha\nQ4: Obduv struzhki\nQ5: Khod karetki\nQ6: Zummer\nQ7: Podsvetka",
        "Q0: Glavniy shpindel\nQ1: Pnevmoprizhimy\nQ2: Podacha SOZh\nQ3: Fiksator kopira\nQ4: Struzhkootdos\nQ5: Elektrotormoz\nQ6: Zummer\nQ7: Osveshchenie",
        "Q0: Klapan vniz\nQ1: Klapan vverh\nQ2: Gidronasos\nQ3: Ventilyator ohladitelya\nQ4: Zazhim zagotovki\nQ5: Fiksiruyushchiy palets\nQ6: Sirena\nQ7: Svetovaya kolonna",
        "Q0..Q7: Promyshlennye rele\nImpuls shagovika: Vysokaya chastota\nZummer: SHIM Pin 45\nLED statusa: Na plate"
    }
};

static const char *g_default_machine_desc[LANG_COUNT][9] = {
    // LANG_EN (0)
    {
        "- MAX6675 precision thermocouple temperature control\n- Automatic pneumatic clamping & weld table cycles\n- Fully configurable weld & cool timers",
        "- Dual independent temperature controllers (L & R)\n- Synchronized moving head pneumatic positioning & brake\n- Solid-State Relay (SSR) digital regulation",
        "- High-resolution optical encoder & digital scale\n- Hydro-pneumatic smooth saw feed mechanism\n- Dual-axis horizontal & vertical profile clamps",
        "- Synchronized dual-motor twin saw operation\n- Air-clamped movable arm with 45/90 deg index\n- Integrated chip exhaust & safety trip system",
        "- Real-time Argon gas purity analyzer with O2 probe\n- Smart vacuum evacuation & dual-stage injection\n- Acoustic completion alarm & safety lockout",
        "- Automated weld bead corner flash removal cycle\n- Dual top/bottom trimming blades & water-slot drill\n- Programmable automatic air-blow swarf purge",
        "- 3-axis copy milling for locks and handles\n- Mist coolant lubrication & rapid spindle brake\n- Ergonomic pneumatic template clamp",
        "- Real-time hydraulic pressure & fluid monitoring\n- High-precision proportional directional valves\n- Two-stage safety foot pedal & interlock shield",
        "- Central OTA firmware download & update manager\n- Peripheral diagnostics & thermal QC printer interface\n- Anti-rollback modular architecture"
    },
    // LANG_TR (1)
    {
        "- MAX6675 ile hassas rezistans sicaklik kontrolu\n- Pnomatik baski, tabla ve sablon cevrimleri\n- Ayarlanabilir kaynak ve soguma sureleri",
        "- Bagimsiz cift kafa sicaklik kontrolu (Sol/Sag)\n- Hareketli kafa pnomatik konumlandirma ve fren\n- Yari iletken roleler (SSR) ile hassas kontrol",
        "- Optik enkoder ve dijital cetvel entegrasyonu\n- Hidropnomatik hassas testere besleme hareketi\n- Yatay ve dikey profil sabitleme mengeleri",
        "- Eszamanli cift testere motor calismasi\n- Havali kilitli hareketli kol ve 45/90 derece ayari\n- Entegre talas emme ve guvenlik alarmlari",
        "- O2 sensorlu gercek zamanli Argon saflik analizoru\n- Akilli vakum tahliye ve hassas gaz enjeksiyonu\n- Sesli dolum uyarisi ve emniyet kilitlemesi",
        "- Otomatik kaynak capagi temizleme cevrimi\n- Ust/Alt bicak kontrolu ve su tahliye frezesi\n- Otomatik hava ufleme ve talas supurme sistemi",
        "- Kilit ve kol yuvalari icin 3 eksenli kopya freze\n- Puskurtmeli sogutma ve hizli motor freni\n- Ergonomik pnomatik sablon kilidi",
        "- Hidrolik basinc ve yag seviye izleme\n- Hassas oransal yon kontrol valfleri\n- Cift kademeli emniyet pedali ve koruma perdesi",
        "- Kapsamli OTA bellenim indirme ve guncelleme yoneticisi\n- Donanim testleri ve termal QC yazici destegi\n- Geri donus onlemeli moduler mimari"
    },
    // LANG_FA (2)
    {
        "- کنترل حرارت المنت با سنسور MAX6675\n- جک های پنوماتیک پرس، میز و شابلون\n- تنظیم زمان بندی جوش و خنک کاری",
        "- کنترل دمای مستقل المنت چپ و راست\n- جک سر متحرک و ترمز بادی هماهنگ\n- کنترل پیشرفته با رله های حالت جامد SSR",
        "- انکودر نوری فاز A/B و خط کش دیجیتال\n- حرکت هیدروپنوماتیک دیسک برش اره\n- گیره های مهاربندی افقی و عمودی",
        "- چرخه همزمان دو موتور اره چپ و راست\n- قفل بادی بازوی متحرک و تنظیم زاویه ۴۵ و ۹۰\n- سیستم مکنده براده و آلارم ایمنی",
        "- آنالایزر خلوص گاز آرگون با سنسور اکسیژن\n- شیرهای برقی وکیوم و تزریق هوشمند\n- هشدار صوتی و رله های ایمنی اتمام تزریق",
        "- چرخه خودکار تمیزکاری گوشه های جوش خورده\n- کنترل تیغ بالا/پایین و مته آبریز\n- سیستم بادپاش پاک سازی خودکار",
        "- فرزکاری سه محوره شیارهای دستگیره و قفل\n- اسپری روان کننده آب صابون و ترمز موتور\n- قفل شابلون پنوماتیک ارگونومیک",
        "- مانیتورینگ فشار هیدرولیک و مخزن روغن\n- شیرهای پروپوشنال بالا و پایین\n- پدال ایمنی دوپله و حفاظ اضطراری",
        "- مدیریت جامع دانلود و بروزرسانی OTA فریمورها\n- آزمون و عیب یابی پریفرال ها و چاپگر حرارتی QC\n- معماری ماژولار با سیستم ضدبازگشت"
    },
    // LANG_DE (3)
    {
        "- MAX6675 prazise Heizelement-Temperaturregelung\n- Automatische Pneumatikspanner & Schweisstisch-Zyklen\n- Einstellbare Schweiss- und Kuhlzeitgeber",
        "- Unabhangige Temperaturregelung fur linken/rechten Kopf\n- Synchronisierte Verfahrkopf-Pneumatik und Bremse\n- Halbleiterrelais (SSR) fur exakte Heizungsregelung",
        "- Optischer Drehgeber & digitale Langenmessung\n- Hydro-pneumatischer praziser Sageblattvorschub\n- Horizontal- und Vertikal-Profilspanner",
        "- Synchronisierter Betrieb beider Sagemotoren\n- Druckluftverriegelter Fahrkopf mit 45/90 Grad Rastung\n- Integrierte Spaneabsaugung & Sicherheitsuberwachung",
        "- O2-Echtzeit-Gasanalyse fur Argon-Reinheit\n- Intelligente Vakuumevakuierung & 2-Stufen-Gasinjektion\n- Akustischer Fertigmelder & Sicherheitsabschaltung",
        "- Automatischer Schweissnaht-Eckenputzzyklus\n- Ober-/Untermesser & Wasserschlitz-Bohreinheit\n- Automatische Druckluft-Spaneabblasung",
        "- 3-Achsen-Kopierfrasen fur Schloss- und Griffausklinkungen\n- Minimalmengenspruhung & elektronische Motorbremse\n- Ergonomische pneumatische Schablonenklemmung",
        "- Hydraulikdruck- und Fullstandsuberwachung in Echtzeit\n- Prazise Proportionalventile fur Heben/Senken\n- 2-Stufen-Sicherheitspedal & Schutzgitteruberwachung",
        "- Zentraler OTA-Firmware-Manager & Download-Portal\n- Peripheriediagnose & Thermodrucker-QC-Schnittstelle\n- Modulare Architektur mit Rollback-Schutz"
    },
    // LANG_RU (4)
    {
        "- Kontrol temperatury MAX6675\n- Pnevmaticheskie prizhimy i stol svarki\n- Nastroyka taymerov svarki i ohlazhdeniya",
        "- Nezavisimiy termokontrol levoy i pravoy golovki\n- Sinhronizirovanniy pnevmotormoz i hod golovki\n- Tverdotelnye rele SSR dlya nagrevateley",
        "- Opticheskiy enkoder i tsifrovaya lineyka\n- Gidropnevmaticheskaya plavnaya podacha diska\n- Gorizontalnye i vertikalnye prizhimy profilya",
        "- Sinhronnaya rabota dvuh pilnyh motorov\n- Pnevmozamok podvizhnoy golovki, ugly 45 i 90\n- Struzhkootdos i sistema avariynoy zashchity",
        "- Analizator chistoty argona s datchikom O2\n- Umnaya vakuuimatsiya i dvuhstupenchaty vprysk gaza\n- Zvukovoe opoveshchenie i blokirovka bezopasnosti",
        "- Avtomaticheskaya zachistka svarnogo shva\n- Verhniy i nizhniy nozhi, sverlenie drenazha\n- Avtomaticheskiy obduv struzhki vozduhom",
        "- Trehosevoe kopirovalnoe frezerovanie pazov i zamkov\n- Ohlazhdenie raspyleniem SOZh i tormoz shpindelya\n- Ergonomichniy pnevmozazhim shablona",
        "- Monitoring davleniya gidravliki i urovnya masla\n- Proportsionalnye klapany podema i opuskaniya\n- Dvuhstupenchataya pedal i svetovoy baryer",
        "- Tsentralizovanniy menedzher OTA-obnovleniy proshivok\n- Diagnostika oborudovaniya i termopechat QC\n- Modulnaya arhitektura s zashchitoy ot otkata"
    }
};

// ===== نقشه‌ی پایه‌های ورودی/خروجی برنامه‌های سایر شرکت‌ها (۴ برنامه) =====
static const char *g_other_pinmap_in[LANG_COUNT][4] = {
    // LANG_EN (0)
    {
        "I0: Tank upper level\nI1: Tank lower level\nI2: Cylinder temp sensor\nI3: Hopper temp sensor\nI4: Dispense handle sw\nI5: High pressure sw\nI6: Emergency stop\nI7: Wash/Auto switch",
        "I0: Dough hopper sensor\nI1: Piston front sensor\nI2: Piston rear sensor\nI3: Conveyor speed pulse\nI4: Safety hopper lid\nI5: Emergency stop\nI6: Overload trip\nI7: Auto/Manual cycle",
        "I0: Safety bowl guard switch\nI1: Bowl lock in position\nI2: Spiral motor high thermal\nI3: Spiral motor low thermal\nI4: Bowl reverse switch\nI5: Emergency mushroom\nI6: Water batch meter\nI7: Cycle start push button",
        "I0..I7: Opto-isolated inputs\nQ0..Q7: Industrial relay outputs\nRS485: Pins 17 & 18\nI2C: MCP23017 expander"
    },
    // LANG_TR (1)
    {
        "I0: Tank ust seviye\nI1: Tank alt seviye\nI2: Silindir sicaklik\nI3: Hazne sicaklik\nI4: Kol mikrosalteri\nI5: Yuksek basinc\nI6: Acil stop\nI7: Yikama/Oto salter",
        "I0: Hamur hazne sensoru\nI1: Piston on sensor\nI2: Piston arka sensor\nI3: Konveyor hiz palsi\nI4: Emniyet kapak switch\nI5: Acil stop\nI6: Asiri yuk koruma\nI7: Oto/Manuel cevrim",
        "I0: Guvenlik kapak switchi\nI1: Kazan kilit sensoru\nI2: Spiral motor yuksek termik\nI3: Spiral motor dusuk termik\nI4: Kazan geri salteri\nI5: Acil stop butonu\nI6: Su sayaci palsi\nI7: Baslatma butonu",
        "I0..I7: Opto-izole PLC girisleri\nQ0..Q7: Endustriyel role cikislari\nRS485: Pin 17 & 18\nI2C: MCP23017 genisletme"
    },
    // LANG_FA (2)
    {
        "I0: سنسور سطح بالای مخزن\nI1: سنسور سطح پایین مخزن\nI2: سنسور دمای سیلندر\nI3: سنسور دمای مخزن\nI4: میکروسوییچ دسته توزیع\nI5: پرشر سوییچ فشار بالا\nI6: استپ اضطراری\nI7: کلید شستشو / خودکار",
        "I0: سنسور مخزن خمیر\nI1: سنسور موقعیت جلو پیستون\nI2: سنسور موقعیت عقب پیستون\nI3: پالس سرعت کانوایر خروجی\nI4: میکروسوییچ درب ایمنی قیف\nI5: استپ قارچی اضطراری\nI6: خطای اضافه بار موتور\nI7: کلید انتخاب حالت خودکار / دستی",
        "I0: میکروسوییچ حفاظ ایمنی دیگ\nI1: سنسور قفل شدن دیگ در موقعیت\nI2: ترمیک دور تند موتور اسپیرال\nI3: ترمیک دور کند موتور اسپیرال\nI4: کلید چرخش معکوس دیگ\nI5: استپ قارچی اضطراری\nI6: ورودی پالس کنتور دوزینگ آب\nI7: شستی استارت چرخه میکس",
        "I0..I7: ورودی های ایزوله PLC\nQ0..Q7: رله های خروجی صنعتی\nRS485: پین ۱۷ و ۱۸\nI2C: توسعه MCP23017"
    },
    // LANG_DE (3)
    {
        "I0: Tank Fullstand oben\nI1: Tank Fullstand unten\nI2: Zylinder-Temperatursensor\nI3: Behalter-Temperatursensor\nI4: Zapfhebel-Mikroschalter\nI5: Hochdruckschalter\nI6: Not-Halt-Taster\nI7: Spul-/Automatikschalter",
        "I0: Teigtrichter-Sensor\nI1: Kolben-Vordersensor\nI2: Kolben-Rucksensor\nI3: Forderband-Geschwindigkeitspuls\nI4: Sicherheitsschalter Trichterdeckel\nI5: Not-Aus-Taster\nI6: Motoruberlastung\nI7: Auto-/Manuell-Wahlschalter",
        "I0: Schutzhauben-Sicherheitsschalter\nI1: Kesselverriegelungssensor\nI2: Spirale Schnellgang-Thermoschutz\nI3: Spirale Langsamgang-Thermoschutz\nI4: Kessel-Reversierschalter\nI5: Not-Aus-Schlagtaster\nI6: Wasserdosierer-Impulseingang\nI7: Start-Drucktaster",
        "I0..I7: Opto-entkoppelte Eingange\nQ0..Q7: Industrierelais-Ausgange\nRS485: Pins 17 & 18\nI2C: MCP23017-Erweiterung"
    },
    // LANG_RU (4)
    {
        "I0: Verhniy uroven baka\nI1: Nizhniy uroven baka\nI2: Datchik temp. tsilindra\nI3: Datchik temp. bunkera\nI4: Mikropereklyuchatel rychaga\nI5: Rele vysokogo davleniya\nI6: Avariyniy stop\nI7: Pereklyuchatel moyka/avto",
        "I0: Datchik bunkera testa\nI1: Datchik perednego polozheniya\nI2: Datchik zadnego polozheniya\nI3: Impulsy skorosti transportera\nI4: Kontsevik kryshki bunkera\nI5: Avariynaya knopka stop\nI6: Peregruzka dvigatelya\nI7: Pereklyuchatel avto/ruchnoy",
        "I0: Zashchitnaya reshetka dezhi\nI1: Datchik fiksatsii dezhi\nI2: Teplovoe rele bystroy skorosti\nI3: Teplovoe rele medlennoy skorosti\nI4: Revers vrascheniya dezhi\nI5: Avariyniy gribok stop\nI6: Schetchik dozirovaniya vody\nI7: Knopka puska smeshivaniya",
        "I0..I7: Optoizolirovannye vhody\nQ0..Q7: Releynye vyhody\nRS485: Vyvody 17 i 18\nI2C: Modul MCP23017"
    }
};

static const char *g_other_pinmap_out[LANG_COUNT][4] = {
    // LANG_EN (0)
    {
        "Q0: Compressor motor\nQ1: Beater motor\nQ2: Air pump motor\nQ3: Hot gas defrost valve\nQ4: Hopper cooling solenoid\nQ5: Dispense valve\nQ6: Low mix buzzer\nQ7: Ready indicator lamp",
        "Q0: Main divider motor\nQ1: Dough cutter knife\nQ2: Flour duster feeder\nQ3: Output conveyor motor\nQ4: Rounding cone motor\nQ5: Central lubrication\nQ6: Cycle completion buzzer\nQ7: System run lamp",
        "Q0: Spiral motor Low speed\nQ1: Spiral motor High speed\nQ2: Bowl rotation motor (FWD)\nQ3: Bowl rotation motor (REV)\nQ4: Auto water dosing valve\nQ5: Hydraulic head lift\nQ6: End of kneading chime\nQ7: Work basin illumination",
        "Q0..Q7: High-current relays\nStepper pulse: High freq\nBuzzer: PWM Pin 45\nStatus LED: Onboard indicator"
    },
    // LANG_TR (1)
    {
        "Q0: Kompresor motoru\nQ1: Cirpici mikser\nQ2: Hava pompasi\nQ3: Sicak gaz defrost valfi\nQ4: Hazne sogutma valfi\nQ5: Dagitim valfi\nQ6: Karisim az uyari zili\nQ7: Hazir gosterge lambasi",
        "Q0: Ana kesme motoru\nQ1: Hamur kesme bicagi\nQ2: Un serpme motoru\nQ3: Cikis konveyoru\nQ4: Yuvarlama motoru\nQ5: Otomatik yaglama\nQ6: Islem sonu zili\nQ7: Calisma lambasi",
        "Q0: Spiral motor dusuk devir\nQ1: Spiral motor yuksek devir\nQ2: Kazan donus (Ileri)\nQ3: Kazan donus (Geri)\nQ4: Otomatik su dozaj valfi\nQ5: Hidrolik kafa kaldirma\nQ6: Hamur hazir sesli ikaz\nQ7: Kazan aydinlatma lambasi",
        "Q0..Q7: Role cikislari\nStep pals: Yuksek frekans\nBuzzer: PWM Pin 45\nDurum LEDi: Kart uzeri"
    },
    // LANG_FA (2)
    {
        "Q0: موتور کمپرسور برودتی\nQ1: موتور همزن (میکسر)\nQ2: موتور پمپ تزریق هوا\nQ3: شیر برقی دیفراست گاز داغ\nQ4: شیر برقی خنک کننده مخزن\nQ5: شیر توزیع بستنی\nQ6: بازر هشدار اتمام مواد\nQ7: چراغ سیگنال آماده توزیع",
        "Q0: الکتروموتور اصلی چانه‌گیر\nQ1: تیغه برش و تقسیم خمیر\nQ2: مکانیزم آردپاش اتوماتیک\nQ3: نوار نقاله خروجی چانه\nQ4: موتور مخروطی گردکن خمیر\nQ5: سیستم روغن‌کاری مرکزی\nQ6: بوق هشدار اتمام چرخه خمیر\nQ7: چراغ سیگنال کارکرد دستگاه",
        "Q0: کنتاکتور دور کند موتور چنگک\nQ1: کنتاکتور دور تند موتور چنگک\nQ2: موتور چرخش دیگ (راست‌گرد)\nQ3: موتور چرخش دیگ (چپ‌گرد)\nQ4: شیر برقی تزریق دوزینگ آب\nQ5: جک هیدرولیک بالابر کله دستگاه\nQ6: بازر اتمام تایمر آماده‌سازی خمیر\nQ7: چراغ روشنایی داخل دیگ",
        "Q0..Q7: خروجی رله ها\nپالس استپر: فرکانس بالا\nبازر: خروجی PWM پین ۴۵\nLED وضعیت: پین روی برد"
    },
    // LANG_DE (3)
    {
        "Q0: Kaltemittelkompressor\nQ1: Ruhrwerk-Motor\nQ2: Luftpumpen-Motor\nQ3: Heissgas-Abtauventil\nQ4: Behalterkuhlung-Magnetventil\nQ5: Zapfventil\nQ6: Warnsummer Materialmangel\nQ7: Bereitschafts-Signalleuchte",
        "Q0: Hauptantriebsmotor\nQ1: Teigabschneidemesser\nQ2: Automatischer Bemehler\nQ3: Abtransport-Forderband\nQ4: Rundwirker-Motor\nQ5: Zentralschmierung\nQ6: Signalton Arbeitsende\nQ7: Betriebslampe",
        "Q0: Spirale Schleichgang-Schutz\nQ1: Spirale Schnellgang-Schutz\nQ2: Kesselantrieb (Vorwarts)\nQ3: Kesselantrieb (Ruckwarts)\nQ4: Automatisches Wasserventil\nQ5: Hydraulischer Kopflifter\nQ6: Knetende-Signalton\nQ7: Kesselbeleuchtung",
        "Q0..Q7: Relaisausgange\nSchrittmotor-Puls: Hochfrequenz\nSummer: PWM-Pin 45\nStatus-LED: Onboard-Anzeige"
    },
    // LANG_RU (4)
    {
        "Q0: Kompressor ohlazhdeniya\nQ1: Motor meshalki\nQ2: Motor vozdushnogo nasosa\nQ3: Klapan ottayki goryachim gazom\nQ4: Klapan ohlazhdeniya bunkera\nQ5: Klapan vydachi\nQ6: Zummer urovnya smesi\nQ7: Signalnaya lampa gotovnosti",
        "Q0: Glavniy privod delitelya\nQ1: Nozh rezki testa\nQ2: Dozator posypki mukoy\nQ3: Transporter vydachi zagotovok\nQ4: Okruglyayushchiy konussny motor\nQ5: Tsentralnaya smazka\nQ6: Zummer zaversheniya tsikla\nQ7: Lampa raboty ustroystva",
        "Q0: Pusk medlennoy skorosti spirali\nQ1: Pusk bystroy skorosti spirali\nQ2: Vraschenie dezhi vpered\nQ3: Vraschenie dezhi nazad\nQ4: Klapan dozirovaniya vody\nQ5: Gidropodemnik golovki\nQ6: Zvukovoy signal gotovnosti\nQ7: Podsvetka zony dezhi",
        "Q0..Q7: Promyshlennye rele\nImpuls shagovika: Vysokaya chastota\nZummer: SHIM Pin 45\nLED statusa: Na plate"
    }
};

static const char *g_other_machine_desc[LANG_COUNT][4] = {
    // LANG_EN (0)
    {
        "- Dual independent refrigeration & cylinder cooling\n- Smart air injection & overrun texture control\n- Automatic night conservation & pasteurization cycle",
        "- Precision volumetric dough dividing & portioning\n- Integrated flour dusting & belt synchronization\n- Optical dough detection with safety interlocks",
        "- Dual-speed heavy-duty spiral hook kneading\n- Reversible stainless steel bowl rotation\n- Programmable two-stage digital dough timers",
        "- Central OTA firmware download & update manager\n- Peripheral diagnostics & thermal QC printer interface\n- Anti-rollback modular architecture"
    },
    // LANG_TR (1)
    {
        "- Bagimsiz cift sogutma ve silindir kontrolu\n- Akilli hava enjeksiyonu ve kivam denetimi\n- Otomatik gece koruma ve pastorizasyon cevrimi",
        "- Hassas hacimsel hamur kesme ve gramaj denetimi\n- Entegre un serpme ve konveyor senkronizasyonu\n- Guvenlik kilitli optik hamur algilama sistemi",
        "- Cift hizli agir hizmet tipi spiral kanca yogurma\n- Paslanmaz celik kazan ileri/geri hareket kontrolu\n- Programlanabilir cift asamali dijital sure denetimi",
        "- Kapsamli OTA bellenim indirme ve guncelleme yoneticisi\n- Donanim testleri ve termal QC yazici destegi\n- Geri donus onlemeli moduler mimari"
    },
    // LANG_FA (2)
    {
        "- کنترل دوگانه سیستم برودتی سیلندر و مخزن نگهداری\n- سیستم تزریق هوای هوشمند جهت تنظیم پف و بافت بستنی\n- چرخه خودکار نگهداری شبانه و شستشوی سیستم",
        "- تقسیم حجمی دقیق خمیر با کنترل گرم و وزن دلخواه\n- سیستم آردپاش خودکار و هماهنگ‌سازی نوار نقاله\n- سنسورهای اپتیکال ردیابی خمیر با قفل‌های ایمنی صنعتی",
        "- موتور دو دور پرقدرت قلاب اسپیرال برای اختلاط کامل\n- چرخش دوطرفه دیگ استیل با سیستم کلاچ ایمن\n- تایمر دیجیتال دو مرحله‌ای قابل برنامه‌ریزی دور کند و تند",
        "- مدیریت جامع دانلود و بروزرسانی OTA فریمورها\n- آزمون و عیب یابی پریفرال ها و چاپگر حرارتی QC\n- معماری ماژولار با سیستم ضدبازگشت"
    },
    // LANG_DE (3)
    {
        "- Getrennte Kuhlkreislaufe fur Zylinder und Vorratsbehalter\n- Intelligente Luftinjektion fur optimale Konsistenz\n- Automatischer Nachtspar- und Reinigungslauf",
        "- Prazise volumetrische Teigteilung mit Grammatur-Einstellung\n- Integrierte Bemehlung und Bandtaktung\n- Optische Teigerkennung mit Schutzhaubenverriegelung",
        "- 2-stufiger Hochleistungs-Spiralkneter fur schwere Teige\n- Reversierbare Edelstahl-Bottichdrehung\n- Programmierbare 2-Phasen-Digitalzeitsteuerung",
        "- Zentraler OTA-Firmware-Manager & Download-Portal\n- Peripheriediagnose & Thermodrucker-QC-Schnittstelle\n- Modulare Architektur mit Rollback-Schutz"
    },
    // LANG_RU (4)
    {
        "- Razdelnoe ohlazhdenie tsilindra i emkosti smesi\n- Umnaya inzhektsiya vozduha dlya konsistentsii\n- Avtomaticheskiy nochnoy rezhim i promyvka",
        "- Tochnoe obemnoe delenie testa s nastroykoy vesa\n- Vstroennaya podsypka muki i sinhronizatsiya lent\n- Opticheskoe obnaruzhenie testa s blokirovkoy",
        "- Dvuhskorostnoy spiralniy mesilniy organ\n- Reversivnoe vraschenie dezhi iz nerzhaveyushchey stali\n- Dvuhstupenchatye tsifrovye taymery zamesa",
        "- Tsentralizovanniy menedzher OTA-obnovleniy proshivok\n- Diagnostika oborudovaniya i termopechat QC\n- Modulnaya arhitektura s zashchitoy ot otkata"
    }
};

extern const char* get_card_version_str(int idx);
extern const char* get_card_bdate_str(int idx);
extern const char* get_card_btime_str(int idx);
extern const char* get_card_desc_line(int idx, int line);
extern const char* get_card_inputs_str(int idx);
extern const char* get_card_outputs_str(int idx);
extern bool get_card_exists_flag(int idx);
extern uint8_t get_card_hwreq_val(int idx);
extern const char* get_card_lastfile_url(int idx);

static StrId s_card_title[9];

static lv_obj_t *g_pm_title = NULL;
static lv_obj_t *g_pm_scroll_cont = NULL;
static lv_obj_t *g_pm_in = NULL;
static lv_obj_t *g_pm_out = NULL;
static lv_obj_t *g_pm_meta_ver_build = NULL;
static lv_obj_t *g_pm_meta_status = NULL;
static lv_obj_t *g_pm_meta_url = NULL;
static lv_obj_t *g_pm_meta_hw = NULL;
static lv_obj_t *g_pm_meta_desc = NULL;
static lv_obj_t *g_pm_desc_title = NULL;

static void pinmap_back_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    if (g_pm_panel) lv_obj_add_flag(g_pm_panel, LV_OBJ_FLAG_HIDDEN);
}

static void pinmap_install_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    if (g_pm_panel) lv_obj_add_flag(g_pm_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_current_pm_idx >= 0 && s_current_pm_idx < 9) {
        action_install_card_by_index(s_current_pm_idx);
    }
}

static void show_pinmap(int idx) {
    if (idx < 0 || idx >= 9) return;
    s_current_pm_idx = idx;
    int lang = (g_lang >= 0 && g_lang < LANG_COUNT) ? g_lang : LANG_FA;

    if (!g_pm_panel) {
        g_pm_panel = lv_obj_create(lv_layer_top());
        lv_obj_set_size(g_pm_panel, 476, 316);
        lv_obj_center(g_pm_panel);
        lv_obj_set_style_bg_color(g_pm_panel, lv_color_hex(0xff0f172a), 0);
        lv_obj_set_style_border_color(g_pm_panel, lv_color_hex(0xff334155), 0);
        lv_obj_set_style_border_width(g_pm_panel, 2, 0);
        lv_obj_set_style_radius(g_pm_panel, 12, 0);
        lv_obj_set_style_pad_all(g_pm_panel, 4, 0);
        lv_obj_clear_flag(g_pm_panel, LV_OBJ_FLAG_SCROLLABLE);

        // هدر عنوان مودال
        g_pm_title = lv_label_create(g_pm_panel);
        lv_obj_set_style_text_font(g_pm_title, &ui_font_kalameh_bold_18, 0);
        lv_obj_set_style_text_color(g_pm_title, lv_color_hex(0xff38bdf8), 0);
        lv_obj_align(g_pm_title, LV_ALIGN_TOP_MID, 0, 4);

        // کانتینر اسکرول‌پذیر عمودی با فضای باز و عدم تداخل با دکمه‌های پایین
        g_pm_scroll_cont = lv_obj_create(g_pm_panel);
        lv_obj_set_size(g_pm_scroll_cont, 468, 226);
        lv_obj_set_pos(g_pm_scroll_cont, 0, 28);
        lv_obj_set_style_bg_opa(g_pm_scroll_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_pm_scroll_cont, 0, 0);
        lv_obj_set_style_pad_all(g_pm_scroll_cont, 2, 0);
        lv_obj_set_style_pad_bottom(g_pm_scroll_cont, 14, 0);
        lv_obj_set_style_pad_row(g_pm_scroll_cont, 8, 0);
        lv_obj_set_flex_flow(g_pm_scroll_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(g_pm_scroll_cont, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(g_pm_scroll_cont, LV_SCROLLBAR_MODE_AUTO);

        // ۱. ردیف کارت‌های ورودی و خروجی (پایه‌ها)
        lv_obj_t *pins_row = lv_obj_create(g_pm_scroll_cont);
        lv_obj_set_size(pins_row, 460, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(pins_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(pins_row, 0, 0);
        lv_obj_set_style_pad_all(pins_row, 0, 0);
        lv_obj_set_style_pad_column(pins_row, 8, 0);
        lv_obj_set_flex_flow(pins_row, LV_FLEX_FLOW_ROW);
        lv_obj_clear_flag(pins_row, LV_OBJ_FLAG_SCROLLABLE);

        // کارت ستون ورودی‌ها (I)
        lv_obj_t *card_in = lv_obj_create(pins_row);
        lv_obj_set_size(card_in, 226, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card_in, lv_color_hex(0xff1e293b), 0);
        lv_obj_set_style_border_color(card_in, lv_color_hex(0xff334155), 0);
        lv_obj_set_style_border_width(card_in, 1, 0);
        lv_obj_set_style_radius(card_in, 10, 0);
        lv_obj_set_style_pad_all(card_in, 8, 0);
        lv_obj_clear_flag(card_in, LV_OBJ_FLAG_SCROLLABLE);
        {
            lv_obj_t *lbl_hdr = lv_label_create(card_in);
            lv_label_set_text(lbl_hdr, tr(STR_INPUTS_PINS));
            i18n_register(lbl_hdr, STR_INPUTS_PINS);
            lv_obj_set_style_text_font(lbl_hdr, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0xff22c55e), 0);
            lv_obj_align(lbl_hdr, LV_ALIGN_TOP_MID, 0, 0);

            g_pm_in = lv_label_create(card_in);
            lv_obj_set_width(g_pm_in, 210);
            lv_label_set_long_mode(g_pm_in, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(g_pm_in, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_set_style_text_font(g_pm_in, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(g_pm_in, lv_color_hex(0xfff8fafc), 0);
            lv_obj_align(g_pm_in, LV_ALIGN_TOP_RIGHT, 0, 24);
        }

        // کارت ستون خروجی‌ها (Q)
        lv_obj_t *card_out = lv_obj_create(pins_row);
        lv_obj_set_size(card_out, 226, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card_out, lv_color_hex(0xff1e293b), 0);
        lv_obj_set_style_border_color(card_out, lv_color_hex(0xff334155), 0);
        lv_obj_set_style_border_width(card_out, 1, 0);
        lv_obj_set_style_radius(card_out, 10, 0);
        lv_obj_set_style_pad_all(card_out, 8, 0);
        lv_obj_clear_flag(card_out, LV_OBJ_FLAG_SCROLLABLE);
        {
            lv_obj_t *lbl_hdr = lv_label_create(card_out);
            lv_label_set_text(lbl_hdr, tr(STR_OUTPUTS_PINS));
            i18n_register(lbl_hdr, STR_OUTPUTS_PINS);
            lv_obj_set_style_text_font(lbl_hdr, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0xfff59e0b), 0);
            lv_obj_align(lbl_hdr, LV_ALIGN_TOP_MID, 0, 0);

            g_pm_out = lv_label_create(card_out);
            lv_obj_set_width(g_pm_out, 210);
            lv_label_set_long_mode(g_pm_out, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(g_pm_out, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_set_style_text_font(g_pm_out, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(g_pm_out, lv_color_hex(0xfff8fafc), 0);
            lv_obj_align(g_pm_out, LV_ALIGN_TOP_RIGHT, 0, 24);
        }

        // ۲. کارت مشخصات و توضیحات سرور (lastfile / فریمور) که با اسکرول پایین می‌آید
        lv_obj_t *card_meta = lv_obj_create(g_pm_scroll_cont);
        lv_obj_set_size(card_meta, 460, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card_meta, lv_color_hex(0xff1e293b), 0);
        lv_obj_set_style_border_color(card_meta, lv_color_hex(0xff334155), 0);
        lv_obj_set_style_border_width(card_meta, 1, 0);
        lv_obj_set_style_radius(card_meta, 10, 0);
        lv_obj_set_style_pad_all(card_meta, 8, 0);
        lv_obj_clear_flag(card_meta, LV_OBJ_FLAG_SCROLLABLE);
        {
            // تیتر بخش مشخصات
            lv_obj_t *meta_hdr = lv_label_create(card_meta);
            lv_label_set_text(meta_hdr, tr(STR_FW_DETAILS));
            i18n_register(meta_hdr, STR_FW_DETAILS);
            lv_obj_set_style_text_font(meta_hdr, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(meta_hdr, lv_color_hex(0xff38bdf8), 0);
            lv_obj_align(meta_hdr, LV_ALIGN_TOP_RIGHT, 0, 0);

            // سطر نسخه و تاریخ بیلد
            g_pm_meta_ver_build = lv_label_create(card_meta);
            lv_obj_set_style_text_font(g_pm_meta_ver_build, &lv_font_dejavu_16_persian_hebrew, 0);
            lv_obj_set_style_text_color(g_pm_meta_ver_build, lv_color_hex(0xffe2e8f0), 0);
            lv_obj_align(g_pm_meta_ver_build, LV_ALIGN_TOP_RIGHT, 0, 22);

            // سطر وضعیت سرور
            g_pm_meta_status = lv_label_create(card_meta);
            lv_obj_set_style_text_font(g_pm_meta_status, &lv_font_dejavu_16_persian_hebrew, 0);
            lv_obj_align(g_pm_meta_status, LV_ALIGN_TOP_RIGHT, 0, 42);

            // سطر آدرس فایل متنی lastfile روی سرور
            g_pm_meta_url = lv_label_create(card_meta);
            lv_obj_set_style_text_font(g_pm_meta_url, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(g_pm_meta_url, lv_color_hex(0xff94a3b8), 0);
            lv_label_set_long_mode(g_pm_meta_url, LV_LABEL_LONG_DOT);
            lv_obj_set_width(g_pm_meta_url, 440);
            lv_obj_align(g_pm_meta_url, LV_ALIGN_TOP_RIGHT, 0, 62);

            // سطر نیازمندی سخت‌افزار
            g_pm_meta_hw = lv_label_create(card_meta);
            lv_obj_set_style_text_font(g_pm_meta_hw, &lv_font_dejavu_16_persian_hebrew, 0);
            lv_obj_set_style_text_color(g_pm_meta_hw, lv_color_hex(0xff94a3b8), 0);
            lv_obj_align(g_pm_meta_hw, LV_ALIGN_TOP_RIGHT, 0, 80);

            // تیتر توضیحات و تغییرات
            g_pm_desc_title = lv_label_create(card_meta);
            lv_label_set_text(g_pm_desc_title, tr(STR_FEATURES_SPECS));
            i18n_register(g_pm_desc_title, STR_FEATURES_SPECS);
            lv_obj_set_style_text_font(g_pm_desc_title, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(g_pm_desc_title, lv_color_hex(0xfffbbf24), 0);
            lv_obj_align(g_pm_desc_title, LV_ALIGN_TOP_RIGHT, 0, 102);

            // متن تغییرات و فیچرهای برنامه
            g_pm_meta_desc = lv_label_create(card_meta);
            lv_obj_set_width(g_pm_meta_desc, 440);
            lv_label_set_long_mode(g_pm_meta_desc, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(g_pm_meta_desc, &ui_font_kalame_bold_14, 0);
            lv_obj_set_style_text_color(g_pm_meta_desc, lv_color_hex(0xffcbd5e1), 0);
            lv_obj_align(g_pm_meta_desc, LV_ALIGN_TOP_RIGHT, 0, 124);
        }

        // کلید بازگشت (سمت چپ پایین - خاکستری تیره اپل)
        lv_obj_t *bk = lv_btn_create(g_pm_panel);
        lv_obj_set_size(bk, 140, 36);
        lv_obj_align(bk, LV_ALIGN_BOTTOM_LEFT, 16, -10);
        lv_obj_set_style_bg_color(bk, lv_color_hex(0xff334155), 0);
        lv_obj_set_style_bg_color(bk, lv_color_hex(0xff1e293b), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(bk, lv_color_hex(0xff475569), 0);
        lv_obj_set_style_border_width(bk, 1, 0);
        lv_obj_set_style_radius(bk, 10, 0);
        lv_obj_set_style_shadow_width(bk, 0, 0);
        lv_obj_add_event_cb(bk, pinmap_back_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *bl = lv_label_create(bk);
        lv_label_set_text(bl, tr(STR_BACK));
        i18n_register(bl, STR_BACK);
        lv_obj_set_style_text_font(bl, &ui_font_kalame_bold_14, 0);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xfff8fafc), 0);
        lv_obj_center(bl);

        // کلید نصب مستقیم از داخل پنجره مشخصات (سبز زمردی اپل)
        lv_obj_t *inst = lv_btn_create(g_pm_panel);
        lv_obj_set_size(inst, 230, 36);
        lv_obj_align(inst, LV_ALIGN_BOTTOM_RIGHT, -16, -10);
        lv_obj_set_style_bg_color(inst, lv_color_hex(0xff059669), 0);
        lv_obj_set_style_bg_color(inst, lv_color_hex(0xff047857), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(inst, lv_color_hex(0xff10b981), 0);
        lv_obj_set_style_border_width(inst, 1, 0);
        lv_obj_set_style_radius(inst, 10, 0);
        lv_obj_set_style_shadow_width(inst, 0, 0);
        lv_obj_add_event_cb(inst, pinmap_install_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *il = lv_label_create(inst);
        lv_label_set_text(il, tr(STR_INSTALL_THIS));
        i18n_register(il, STR_INSTALL_THIS);
        lv_obj_set_style_text_font(il, &ui_font_kalame_bold_14, 0);
        lv_obj_set_style_text_color(il, lv_color_hex(0xffffffff), 0);
        lv_obj_center(il);
    }

    // بازنشانی موقعیت اسکرول به ابتدای بالا در هر بار باز شدن
    lv_obj_scroll_to_y(g_pm_scroll_cont, 0, LV_ANIM_OFF);

    bool isOther = (g_firmware_suite == SUITE_OTHER);
    bool isConfigCard = (idx == 8 || (isOther && idx == 3));

    // ۱. متون پایه‌ها به زبان انتخابی (برگرفته از سرور)
    lv_label_set_text(g_pm_title, tr(s_card_title[idx]));
    const char *srv_in = get_card_inputs_str(idx);
    const char *srv_out = get_card_outputs_str(idx);
    if (srv_in && srv_in[0]) {
        lv_label_set_text(g_pm_in, srv_in);
    } else {
        lv_label_set_text(g_pm_in, tr(STR_NOT_DEFINED_ON_SERVER));
    }
    if (srv_out && srv_out[0]) {
        lv_label_set_text(g_pm_out, srv_out);
    } else {
        lv_label_set_text(g_pm_out, tr(STR_NOT_DEFINED_ON_SERVER));
    }

    // تنظیم جهت و تراز متن متناسب با زبان (راست‌چین برای فارسی، چپ‌چین برای سایر زبان‌ها)
    lv_base_dir_t bdir = (lang == LANG_FA) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR;
    lv_text_align_t talign = (lang == LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT;
    lv_obj_set_style_base_dir(g_pm_in, bdir, 0);
    lv_obj_set_style_text_align(g_pm_in, talign, 0);
    lv_obj_set_style_base_dir(g_pm_out, bdir, 0);
    lv_obj_set_style_text_align(g_pm_out, talign, 0);
    if (g_pm_meta_desc) {
        lv_obj_set_style_base_dir(g_pm_meta_desc, bdir, 0);
        lv_obj_set_style_text_align(g_pm_meta_desc, talign, 0);
    }

    // ۲. متون فراداده، نسخه و بیلد (کاملاً سرورمحور و بدون مقادیر ماک پیش‌فرض)
    const char *ver = get_card_version_str(idx);
    const char *bdate = get_card_bdate_str(idx);
    const char *btime = get_card_btime_str(idx);
    bool exists = get_card_exists_flag(idx);
    uint8_t hwreq = get_card_hwreq_val(idx);

    static char verBuf[128];
    const char *verPrefix = (lang == LANG_FA) ? "نسخه: " : ((lang == LANG_TR) ? "Versiyon: " : ((lang == LANG_RU) ? "Versiya: " : "Version: "));
    const char *bldPrefix = (lang == LANG_FA) ? "  |  بیلد: " : ((lang == LANG_TR) ? "  |  Yapim: " : ((lang == LANG_RU) ? "  |  Sborka: " : "  |  Build: "));

    if (ver && ver[0]) {
        snprintf(verBuf, sizeof(verBuf), "%s%s%s%s%s %s",
                 verPrefix,
                 (ver[0] == 'v' || ver[0] == 'V') ? "" : "V", ver,
                 bldPrefix,
                 (bdate && bdate[0]) ? bdate : "-",
                 (btime && btime[0]) ? btime : "");
    } else {
        if (isConfigCard) {
            snprintf(verBuf, sizeof(verBuf), "%s%s%s" FW_BUILD_DATE " " FW_BUILD_TIME, verPrefix, VERSION, bldPrefix);
        } else {
            snprintf(verBuf, sizeof(verBuf), "%s%s", verPrefix, tr(STR_NOT_RECEIVED_FROM_SERVER));
        }
    }
    lv_label_set_text(g_pm_meta_ver_build, verBuf);

    // وضعیت سرور
    if (exists || isConfigCard) {
        lv_label_set_text(g_pm_meta_status, tr(STR_STATUS_READY));
        lv_obj_set_style_text_color(g_pm_meta_status, lv_color_hex(0xff4ade80), 0);
    } else {
        lv_label_set_text(g_pm_meta_status, tr(STR_STATUS_OFFLINE));
        lv_obj_set_style_text_color(g_pm_meta_status, lv_color_hex(0xffef4444), 0);
    }

    // آدرس فایل متنی lastfile روی سرور آروان‌کلود
    if (g_pm_meta_url) {
        const char *u = get_card_lastfile_url(idx);
        lv_label_set_text(g_pm_meta_url, (u && u[0]) ? u : "");
    }

    // نیازمندی سخت‌افزار
    StrId hwId = STR_HW_BASE;
    if (hwreq == 1) hwId = STR_HW_REQ1;
    else if (hwreq == 2) hwId = STR_HW_REQ2;
    else if (hwreq == 3) hwId = STR_HW_REQ3;
    lv_label_set_text(g_pm_meta_hw, tr(hwId));

    if (g_pm_desc_title) lv_label_set_text(g_pm_desc_title, tr(STR_FEATURES_SPECS));

    // توضیحات و قابلیت‌های برنامه برگرفته مستقیم از lastfile سرور (بدون متن ماک)
    static char descBuf[768];
    descBuf[0] = '\0';
    int d_cnt = 0;
    for (int d = 0; d < 8; d++) {
        const char *dl = get_card_desc_line(idx, d);
        if (dl && dl[0]) {
            const char *p = dl;
            while (*p == ' ' || *p == '\t') p++;
            while (*p == '-' || *p == '*' || *p == '=') {
                p++;
                while (*p == ' ' || *p == '\t') p++;
            }
            if (*p) {
                if (d_cnt > 0) strncat(descBuf, "\n- ", sizeof(descBuf) - strlen(descBuf) - 1);
                else strncat(descBuf, "- ", sizeof(descBuf) - strlen(descBuf) - 1);
                strncat(descBuf, p, sizeof(descBuf) - strlen(descBuf) - 1);
                d_cnt++;
            }
        }
    }
    if (d_cnt == 0) {
        snprintf(descBuf, sizeof(descBuf), "%s", tr(STR_DETAILS_NOT_RECEIVED));
    }
    lv_label_set_text(g_pm_meta_desc, descBuf);

    lv_obj_clear_flag(g_pm_panel, LV_OBJ_FLAG_HIDDEN);
}

static void card_click_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    app_sound_play(SOUND_TAP);
    show_pinmap((int)(intptr_t)lv_event_get_user_data(e));
}

// ===== کارتِ HAFFLER (اولین کارت): برند + نسخه‌ی Config + لوگو + دکمه‌ی برگشت به رمز ادمین =====
static lv_obj_t *s_brand_version_label = NULL;
extern void action_btn_back_to_password(lv_event_t *e);
static void event_handler_back_to_password(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        app_sound_play(SOUND_TAP);
        action_btn_back_to_password(e);
    }
}

static void build_haffler_card(lv_obj_t *parent) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 226, 304);   // عرض بهینه ۲۲۶ پیکسل جهت نمایش همزمان دو کارت کامل
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xff2b2b2b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0xff3f3f46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        // عنوانِ بالا: HAFFLER
        lv_obj_t *o = lv_label_create(card);
        lv_label_set_text(o, "HAFFLER");
        lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_set_style_text_color(o, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(o, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // عنوان مجموعه برنامه‌ها بر اساس رمز واردشده (UPVC یا سایر شرکت‌ها)
        lv_obj_t *suite_lbl = lv_label_create(card);
        StrId s_id = (g_firmware_suite == SUITE_OTHER) ? STR_SUITE_OTHER : STR_SUITE_UPVC;
        lv_label_set_text(suite_lbl, tr(s_id));
        i18n_register(suite_lbl, s_id);
        lv_obj_set_width(suite_lbl, 214);
        lv_label_set_long_mode(suite_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_base_dir(suite_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(suite_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(suite_lbl, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(suite_lbl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(suite_lbl, LV_ALIGN_TOP_MID, 0, 32);
    }
    {
        lv_obj_t *line = lv_obj_create(card);
        lv_obj_set_size(line, 180, 2);
        lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 72);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(line, lv_color_hex(0xff38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // نسخه‌ی فریم‌ور دستگاه (نمایش دائم در بالای کلید ورود لینک در هر دو صفحه UPVC و OTHER)
        lv_obj_t *o = lv_label_create(card);
        s_brand_version_label = o;
        const char *ver_str = get_var_app_version();
        if (!ver_str || ver_str[0] == '\0') {
            ver_str = VERSION;
        }
        lv_label_set_text(o, ver_str);
        lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 80);
        lv_obj_set_style_text_color(o, lv_color_hex(0xff94a3b8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(o, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // دکمه‌ی «ورود لینک»: نصب فریمور از URL دلخواه (زیر برند/نسخه)
        lv_obj_t *btn = lv_btn_create(card);
        objects.link_entry_btn = btn;
        lv_obj_set_size(btn, 190, 34);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 106);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(btn, event_handler_link_entry_open, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(btn, event_handler_link_entry_open, LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, tr(STR_LINK_ENTRY));
        i18n_register(bl, STR_LINK_ENTRY);
        lv_obj_center(bl);
        lv_obj_set_style_text_font(bl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // لوگوی رسمی HAFFLER بین کلید ورود لینک و کلید بازگشت (شیفت به y=160)
        lv_obj_t *logo = lv_img_create(card);
        lv_img_set_src(logo, &img_bitmap_haffler_logo);
        lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 160);
    }
    {
        // دکمه‌ی برگشت به منوی رمز ادمین (پایینِ کارت با قرمز استاندارد اپل)
        lv_obj_t *btn = lv_btn_create(card);
        lv_obj_set_size(btn, 204, 46);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -12);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xffef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xffdc2626), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn, event_handler_back_to_password, LV_EVENT_CLICKED, NULL);
        lv_obj_t *bl = lv_label_create(btn);
        lv_obj_set_width(bl, 190);
        lv_label_set_long_mode(bl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(bl, tr(STR_BACK_TO_ADMIN));
        i18n_register(bl, STR_BACK_TO_ADMIN);
        lv_obj_set_style_text_align(bl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(bl);
        lv_obj_set_style_text_font(bl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void build_prog_card(lv_obj_t *parent, int idx, StrId titleId, const lv_img_dsc_t *img) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 226, 304);   // عرض ۲۲۶ پیکسل جهت جاگیری بی‌نقص در صفحه ۴۸۰px
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    s_card_title[idx] = titleId;
    lv_obj_set_style_bg_color(card, lv_color_hex(0xff2b2b2b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0xff3f3f46), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        // عنوان دستگاه (فونت زیبای کلمه بولد)
        lv_obj_t *o = lv_label_create(card);
        lv_obj_set_pos(o, 4, 6);
        lv_obj_set_size(o, 218, LV_SIZE_CONTENT);
        lv_label_set_long_mode(o, LV_LABEL_LONG_WRAP);
        lv_label_set_text(o, tr(titleId));
        i18n_register(o, titleId);
        lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(o, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(o, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // تصویر دستگاه (۱۰ پیکسل بالاتر)
        lv_obj_t *o = lv_img_create(card);
        lv_img_set_src(o, img);
        lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 32);
    }
    {
        // وضعیت نسخه آنلاین (پیش‌فرض: در حال اتصال به سرور بدون متن ماک)
        lv_obj_t *o = lv_label_create(card);
        objects.card_ver[idx] = o;
        lv_obj_set_size(o, 218, LV_SIZE_CONTENT);
        lv_label_set_text(o, tr(STR_CONNECTING_TO_SERVER));
        lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 239);
        lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(o, lv_color_hex(0xfff59e0b), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(o, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (idx == 8) {
            lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(o, event_handler_version_open, LV_EVENT_SHORT_CLICKED, NULL);
        }
    }
    {
        lv_obj_t *o = lv_label_create(card);
        objects.card_incompat[idx] = o;
        lv_obj_set_size(o, 218, LV_SIZE_CONTENT);
        lv_label_set_long_mode(o, LV_LABEL_LONG_WRAP);
        lv_label_set_text(o, "");
        lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 239);
        lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(o, lv_color_hex(0xffef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(o, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // دکمه نصب (سمت چپ پایین - پیش‌فرض سبز «نصب»)
        lv_obj_t *btn = lv_btn_create(card);
        objects.card_btn[idx] = btn;
        lv_obj_set_size(btn, 102, 28);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 6, -8);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        bool isInstalled = (idx == 8 || (g_firmware_suite == SUITE_OTHER && idx == 3));
        if (isInstalled) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xffd97706), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xffb45309), LV_PART_MAIN | LV_STATE_PRESSED);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xff059669), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xff047857), LV_PART_MAIN | LV_STATE_PRESSED);
        }
        lv_obj_add_event_cb(btn, event_handler_card_install, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, isInstalled ? tr(STR_REINSTALL) : tr(STR_INSTALL));
        lv_obj_center(bl);
        lv_obj_set_style_text_font(bl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // دکمه مشخصات و پایه‌ها (سمت راست پایین - خاکستری تیره اپل)
        lv_obj_t *btn_info = lv_btn_create(card);
        lv_obj_set_size(btn_info, 102, 28);
        lv_obj_align(btn_info, LV_ALIGN_BOTTOM_RIGHT, -6, -8);
        lv_obj_set_style_radius(btn_info, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn_info, lv_color_hex(0xff334155), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn_info, lv_color_hex(0xff1e293b), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_info, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(btn_info, card_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
        lv_obj_t *bl = lv_label_create(btn_info);
        lv_label_set_text(bl, tr(STR_INFO));
        i18n_register(bl, STR_INFO);
        lv_obj_center(bl);
        lv_obj_set_style_text_font(bl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// ===== پاپ‌آپ اطلاعات ورژن: آپلود (زمان بیلد) + تاریخچه‌ی fw_changelog (جدید → قدیم) =====
static void build_version_panel(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    objects.version_panel = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    {
        lv_obj_t *parent_obj = obj;
        // کارتِ مرکزی 420x290 (تیره، گردگوشه، مثل بقیه‌ی کارت‌ها)
        lv_obj_t *card = lv_obj_create(parent_obj);
        lv_obj_set_size(card, 420, 290);
        lv_obj_set_align(card, LV_ALIGN_CENTER);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(0xff2b2b2b), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(card, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        {
            // عنوان
            lv_obj_t *t = lv_label_create(card);
            lv_label_set_text(t, "اطلاعات ورژن");
            lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);
            lv_obj_set_style_text_color(t, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(t, &ui_font_kalameh_bold_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // خطِ «آپلود» — همیشه دیده می‌شود، بیرون از ناحیه‌ی اسکرول
            lv_obj_t *u = lv_label_create(card);
            objects.version_upload = u;
            lv_label_set_text(u, "آپلود: " FW_BUILD_DATE " " FW_BUILD_TIME);
            lv_obj_align(u, LV_ALIGN_TOP_MID, 0, 32);
            lv_obj_set_style_text_color(u, lv_color_hex(0xffb0b0b0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(u, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ناحیه‌ی اسکرولِ تاریخچه (ستونی؛ در صورت سرریز اسکرول می‌شود)
            lv_obj_t *sc = lv_obj_create(card);
            objects.version_scroll = sc;
            lv_obj_set_pos(sc, 8, 56);
            lv_obj_set_size(sc, 404, 186);
            lv_obj_set_scroll_dir(sc, LV_DIR_VER);
            lv_obj_set_scrollbar_mode(sc, LV_SCROLLBAR_MODE_AUTO);
            lv_obj_set_style_bg_opa(sc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(sc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(sc, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(sc, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_flex_flow(sc, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_row(sc, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            for (int i = (int)FW_CHANGELOG_COUNT - 1; i >= 0; i--) {
                const FwLogEntry *entry = &fw_changelog[i];
                {
                    // نسخه + تاریخ (لاتین)
                    lv_obj_t *vd = lv_label_create(sc);
                    lv_label_set_text_fmt(vd, "%s  (%s)", entry->ver, entry->date);
                    lv_obj_set_width(vd, lv_pct(100));
                    lv_obj_set_style_text_align(vd, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(vd, lv_color_hex(0xff4db1ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(vd, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // خلاصه‌ی تغییرات (فارسی، ممکن است دو خط شود)
                    lv_obj_t *nt = lv_label_create(sc);
                    lv_label_set_text(nt, entry->note);
                    lv_obj_set_width(nt, lv_pct(100));
                    lv_label_set_long_mode(nt, LV_LABEL_LONG_WRAP);
                    lv_obj_set_style_text_align(nt, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(nt, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(nt, &ui_font_kalameh_regular_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // دکمه‌ی بستن
            lv_obj_t *btn = lv_btn_create(card);
            objects.version_close = btn;
            lv_obj_set_size(btn, 120, 38);
            lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -6);
            lv_obj_set_style_radius(btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xff555555), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(btn, event_handler_version_close, LV_EVENT_CLICKED, (void *)0);
            lv_obj_t *bl = lv_label_create(btn);
            lv_label_set_text(bl, "بستن");
            lv_obj_center(bl);
            lv_obj_set_style_text_font(bl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

extern uint32_t g_card_query_start_ms;

void create_screen_screen_framware() {
    if (g_card_query_start_ms == 0) {
        g_card_query_start_ms = (uint32_t)millis();
    }
    i18n_reset();
    void *flowState = getFlowState(0, 3);
    lv_obj_t *obj = lv_obj_create(0);
    objects.screen_framware = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ONE|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(obj, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // ===== ردیف افقیِ کارت‌ها (نمایش کامل و همزمان دو کارت اول در ۴۸۰px بدون هیچ برشی) =====
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.cards_row = obj;
            lv_obj_set_pos(obj, 4, 2);
            lv_obj_set_size(obj, 472, 316);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_HOR);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_NONE);   // حذف محاسبات سنگین اسنپ در هر پیکسل لمس برای روانی کامل
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ELASTIC); // اینرسی پرتابی روان و خاصیت کشسانی نرم لبه‌ها به سبک اپل
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN);
            lv_obj_add_event_cb(obj, event_handler_cards_row_scroll, LV_EVENT_SCROLL, NULL);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                build_haffler_card(parent_obj);   // کارتِ اول: برند HAFFLER + نسخه + برگشت به رمز ادمین
                if (g_firmware_suite == SUITE_OTHER) {
                    build_prog_card(parent_obj, 0, STR_PROD_ICE_CREAM,      &img_bitmap_ice_cream);
                    build_prog_card(parent_obj, 1, STR_PROD_BAKERY_CHANGIR,  &img_bitmap_bakery_changir);
                    build_prog_card(parent_obj, 2, STR_PROD_BAKERY_MIXER,    &img_bitmap_bakery_mixer);
                    build_prog_card(parent_obj, 3, STR_PROD_CONFIG,          &img_bitmap_config);
                } else {
                    build_prog_card(parent_obj, 0, STR_PROD_SINGLE,        &img_bitmap_welder_single);
                    build_prog_card(parent_obj, 1, STR_PROD_WELDER_DOUBLE, &img_bitmap_welder_double);
                    build_prog_card(parent_obj, 2, STR_PROD_CUTTER_SINGLE, &img_bitmap_cutter_single);
                    build_prog_card(parent_obj, 3, STR_PROD_CUTTER_DOUBLE, &img_bitmap_cutter_double);
                    build_prog_card(parent_obj, 4, STR_PROD_GAS,           &img_bitmap_gas);
                    build_prog_card(parent_obj, 5, STR_PROD_CORNER,        &img_bitmap_corner);
                    build_prog_card(parent_obj, 6, STR_PROD_COPY,          &img_bitmap_copy);
                    build_prog_card(parent_obj, 7, STR_PROD_PUNCH,         &img_bitmap_punch);
                    build_prog_card(parent_obj, 8, STR_PROD_CONFIG,        &img_bitmap_config);
                }
            }
        }
        {
            // WaitingPanle_1
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.waiting_panle_1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_bg_color(obj, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 230, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj28 = obj;
                    lv_obj_set_pos(obj, 0, 22);
                    lv_obj_set_size(obj, 460, LV_SIZE_CONTENT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.obj29 = obj;
                    lv_obj_set_pos(obj, 200, 60);
                    lv_obj_set_size(obj, 80, 78);
                    lv_arc_set_rotation(obj, 270);
                    lv_arc_set_bg_angles(obj, 0, 360);
                    lv_arc_set_range(obj, 0, 100);
                    lv_arc_set_value(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff4a4a4a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff29ac6b), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            lv_obj_set_pos(obj, 24, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_bitmap_download_icon);
                        }
                    }
                }
                {
                    // برچسب متریک‌های لحظه‌ای دانلود (درصد، حجم دانلود شده، سرعت شبکه)
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.download_metrics_label = lbl;
                    lv_obj_set_pos(lbl, 10, 148);
                    lv_obj_set_size(lbl, 460, 24);
                    lv_label_set_text(lbl, "%0   •   0.00 / 0.00 MB   •   0 KB/s   •   --s");
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xff4db1ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // دکمه انصراف و لغو دانلود
                    lv_obj_t *cbtn = lv_btn_create(parent_obj);
                    objects.download_cancel_btn = cbtn;
                    lv_obj_set_size(cbtn, 170, 44);
                    lv_obj_set_pos(cbtn, 155, 184);
                    lv_obj_set_style_radius(cbtn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(cbtn, lv_color_hex(0xffd32f2f), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(cbtn, lv_color_hex(0xffb71c1c), LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_shadow_width(cbtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(cbtn, event_handler_download_cancel, LV_EVENT_CLICKED, NULL);

                    lv_obj_t *clbl = lv_label_create(cbtn);
                    objects.download_cancel_label = clbl;
                    lv_label_set_text(clbl, tr(STR_CANCEL_DL));
                    i18n_register(clbl, STR_CANCEL_DL);
                    lv_obj_center(clbl);
                    lv_obj_set_style_text_color(clbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(clbl, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // ===== پنل تأیید نصب (طراحی استاندارد اپل HIG با توقف تایمر در لمس) =====
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.confirm_panel = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, confirm_touch_pause_cb, LV_EVENT_ALL, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // کارت پنجره مودال
                    lv_obj_t *card = lv_obj_create(parent_obj);
                    lv_obj_set_size(card, 440, 252);
                    lv_obj_set_align(card, LV_ALIGN_CENTER);
                    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_bg_color(card, lv_color_hex(0xff1c1c1e), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(card, lv_color_hex(0xff3a3a3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(card, confirm_touch_pause_cb, LV_EVENT_ALL, NULL);
                    {
                        lv_obj_t *parent_obj = card;
                        {
                            // کانتینر اسکرول‌شونده متن (جلوگیری قطعی از سرریز روی کلیدها)
                            lv_obj_t *desc_box = lv_obj_create(parent_obj);
                            lv_obj_set_size(desc_box, 416, 166);
                            lv_obj_align(desc_box, LV_ALIGN_TOP_MID, 0, 4);
                            lv_obj_set_scroll_dir(desc_box, LV_DIR_VER);
                            lv_obj_set_scrollbar_mode(desc_box, LV_SCROLLBAR_MODE_AUTO);
                            lv_obj_set_style_bg_opa(desc_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(desc_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(desc_box, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(desc_box, confirm_touch_pause_cb, LV_EVENT_ALL, NULL);

                            lv_obj_t *lbl = lv_label_create(desc_box);
                            objects.confirm_label = lbl;
                            lv_obj_set_width(lbl, 404);
                            lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
                            lv_label_set_text(lbl, "");
                            lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(lbl, lv_color_hex(0xfff5f5f7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(lbl, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(lbl, confirm_touch_pause_cb, LV_EVENT_ALL, NULL);
                        }
                        {
                            // کلید تایید (Yes) با استایل مدرن اپل
                            lv_obj_t *btn_yes = lv_btn_create(parent_obj);
                            objects.confirm_yes = btn_yes;
                            lv_obj_set_size(btn_yes, 140, 44);
                            lv_obj_align(btn_yes, LV_ALIGN_BOTTOM_LEFT, 20, -14);
                            lv_obj_set_style_radius(btn_yes, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0xff10b981), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0xff059669), LV_PART_MAIN | LV_STATE_PRESSED);
                            lv_obj_add_event_cb(btn_yes, event_handler_confirm_yes, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl_yes = lv_label_create(btn_yes);
                                lv_label_set_text(lbl_yes, tr(STR_YES));
                                i18n_register(lbl_yes, STR_YES);
                                lv_obj_center(lbl_yes);
                                lv_obj_set_style_text_font(lbl_yes, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);
                            }
                        }
                        {
                            // کلید انصراف (No) با نوار پیشرفت باریک ۳ پیکسلی در لبه پایینی دکمه (اپل HIG)
                            lv_obj_t *btn_no = lv_btn_create(parent_obj);
                            objects.confirm_no = btn_no;
                            lv_obj_set_size(btn_no, 140, 44);
                            lv_obj_align(btn_no, LV_ALIGN_BOTTOM_RIGHT, -20, -14);
                            lv_obj_set_style_radius(btn_no, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_clip_corner(btn_no, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(btn_no, lv_color_hex(0xff3a3a3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(btn_no, lv_color_hex(0xff2c2c2e), LV_PART_MAIN | LV_STATE_PRESSED);
                            lv_obj_set_style_pad_all(btn_no, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(btn_no, event_handler_confirm_no, LV_EVENT_CLICKED, (void *)0);

                            lv_obj_t *lbl_no = lv_label_create(btn_no);
                            lv_label_set_text(lbl_no, tr(STR_NO));
                            i18n_register(lbl_no, STR_NO);
                            lv_obj_center(lbl_no);
                            lv_obj_set_style_text_font(lbl_no, &lv_font_dejavu_16_persian_hebrew, LV_PART_MAIN | LV_STATE_DEFAULT);

                            // نوار پیشرفت مینیمال تایمر در لبه پایین
                            lv_obj_t *bar = lv_obj_create(btn_no);
                            s_confirm_cancel_bar = bar;
                            lv_obj_set_size(bar, 0, 3);
                            lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
                            lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                            lv_obj_set_style_bg_color(bar, lv_color_hex(0xfffbbf24), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(bar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
        {
            // ===== پنل ورود لینک — تمام‌صفحه (مات؛ حاشیه/گردگوشه مثل کارت‌های تیره) =====
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.link_entry_panel = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2b2b2b), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff4e4e4e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // برگشت (بالا سمت چپ)
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.link_entry_cancel = obj;
                    lv_obj_set_pos(obj, 8, 6);
                    lv_obj_set_size(obj, 76, 34);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_link_cancel, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *obj2 = lv_label_create(obj);
                        lv_label_set_text(obj2, tr(STR_BACK));
                        i18n_register(obj2, STR_BACK);
                        lv_obj_center(obj2);
                        lv_obj_set_style_text_font(obj2, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                {
                    // دکمه پاک‌کردن متن (آیکون سطل زباله)
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    lv_obj_set_pos(obj, 88, 6);
                    lv_obj_set_size(obj, 40, 34);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff991b1b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_link_clear, LV_EVENT_CLICKED, NULL);
                    {
                        lv_obj_t *obj2 = lv_label_create(obj);
                        lv_label_set_text(obj2, LV_SYMBOL_TRASH);
                        lv_obj_center(obj2);
                        lv_obj_set_style_text_font(obj2, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_color(obj2, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                {
                    // کلید درج لینک پیش‌فرض سرور در کادر اصلی
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    lv_obj_set_pos(obj, 132, 6);
                    lv_obj_set_size(obj, 176, 34);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3b82f6), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_link_default, LV_EVENT_CLICKED, NULL);
                    {
                        lv_obj_t *obj2 = lv_label_create(obj);
                        lv_label_set_text(obj2, tr(STR_DEFAULT_LINK));
                        i18n_register(obj2, STR_DEFAULT_LINK);
                        lv_obj_center(obj2);
                        lv_obj_set_style_base_dir(obj2, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(obj2, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_color(obj2, lv_color_hex(0xff93c5fd), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                {
                    // تایید و دانلود
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.link_entry_confirm = obj;
                    lv_obj_set_pos(obj, 312, 6);
                    lv_obj_set_size(obj, 160, 34);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff059669), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, event_handler_link_confirm, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *obj2 = lv_label_create(obj);
                        lv_label_set_text(obj2, tr(STR_DOWNLOAD_AND_INSTALL));
                        i18n_register(obj2, STR_DOWNLOAD_AND_INSTALL);
                        lv_obj_center(obj2);
                        lv_obj_set_style_text_font(obj2, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_color(obj2, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                {
                    // کادر اصلی ورود لینک — تمام‌عرض و تک‌خطی با نشانگر کنتراست بالای آبی آسمانی اپل (y=44 تا y=78)
                    lv_obj_t *obj = lv_textarea_create(parent_obj);
                    objects.link_entry_textarea = obj;
                    lv_obj_set_pos(obj, 8, 44);
                    lv_obj_set_size(obj, 464, 34);
                    lv_textarea_set_one_line(obj, true);
                    lv_textarea_set_max_length(obj, 256);
                    lv_textarea_set_placeholder_text(obj, "http://... or filename.bin");
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff09090b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3b82f6), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_hor(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_ver(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    // نشان‌گر متمایز و درخشان (غیر از مشکی و سفید: آبی آسمانی اپل)
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff38bdf8), LV_PART_CURSOR);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff38bdf8), LV_PART_CURSOR);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_CURSOR);
                    lv_obj_set_style_anim_time(obj, 500, LV_PART_CURSOR);
                    lv_obj_add_event_cb(obj, event_handler_link_textarea, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_obj_add_event_cb(obj, event_handler_link_textarea, LV_EVENT_CLICKED, NULL);
                }
                {
                    // بنر اختصاصی وضعیت و پیام بررسی لینک — سطر مستقل، بدون تداخل با کادر متن یا کیبورد (y=82 تا y=116)
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.link_entry_error = obj;
                    lv_obj_set_pos(obj, 8, 82);
                    lv_obj_set_size(obj, 464, 34);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, tr(STR_ENTER_LINK_OR_FILE));
                    i18n_register(obj, STR_ENTER_LINK_OR_FILE);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfffca5a5), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff090d16), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 252, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_hor(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_kalame_bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(obj, event_handler_link_error_click, LV_EVENT_CLICKED, NULL);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // کیبرد اختصاصی URL با کلید حروف بزرگ (Shift/Caps) — 480x200 از y=118
                    lv_obj_t *obj = lv_keyboard_create(parent_obj);
                    objects.link_entry_keyboard = obj;
                    lv_obj_set_align(obj, LV_ALIGN_DEFAULT);
                    lv_obj_set_pos(obj, 0, 118);
                    lv_obj_set_size(obj, 480, 200);
                    lv_keyboard_set_map(obj, LV_KEYBOARD_MODE_TEXT_LOWER, cfg_url_kb_map_lc, cfg_url_kb_ctrl_lc);
                    lv_keyboard_set_map(obj, LV_KEYBOARD_MODE_TEXT_UPPER, cfg_url_kb_map_uc, cfg_url_kb_ctrl_uc);
                    lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_TEXT_LOWER);
                    lv_keyboard_set_textarea(obj, objects.link_entry_textarea);
                    lv_obj_add_event_cb(obj, event_handler_link_entry_keyboard, LV_EVENT_ALL, NULL);
                    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_PRESSED);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 6, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    style_keyboard_action_keys(obj);
                }
            }
        }
        build_version_panel(parent_obj);   // پاپ‌آپ اطلاعات ورژن (کلیک روی نسخه‌ی کارت Config)
    }
    
    i18n_apply();
    if (objects.cards_row && lv_obj_is_valid(objects.cards_row)) {
        int16_t saved_sx = app_get_cards_scroll_x();
        if (saved_sx > 0) {
            lv_obj_update_layout(objects.cards_row);
            lv_obj_scroll_to_x(objects.cards_row, saved_sx, LV_ANIM_OFF);
        }
    }
    tick_screen_screen_framware();
}

static bool s_cards_scroll_restored = false;

void delete_screen_screen_framware() {
    s_cards_scroll_restored = false;
    firmware_confirm_countdown_stop();
    s_confirm_cancel_bar = NULL;
    // پنلِ نقشه‌ی پایه‌ها روی lv_layer_top ساخته می‌شود؛ با حذفِ صفحه باید پاک شود
    // وگرنه روی صفحه‌های بعدی می‌ماند (اگر باز باشد) و اشاره‌گرهای ایستایش معلق می‌شوند
    if (g_pm_panel && lv_obj_is_valid(g_pm_panel)) lv_obj_del(g_pm_panel);
    g_pm_panel = NULL;
    g_pm_title = NULL;
    g_pm_in = NULL;
    g_pm_out = NULL;
    g_pm_scroll_cont = NULL;
    g_pm_meta_ver_build = NULL;
    g_pm_meta_status = NULL;
    g_pm_meta_url = NULL;
    g_pm_meta_hw = NULL;
    g_pm_desc_title = NULL;
    g_pm_meta_desc = NULL;
    s_current_pm_idx = -1;
    s_brand_version_label = NULL;
    lv_obj_del(objects.screen_framware);
    objects.screen_framware = 0;
    objects.info_panle = 0;
    objects.obj17 = 0;
    objects.obj18 = 0;
    objects.obj19 = 0;
    objects.seriall_number = 0;
    objects.obj20 = 0;
    objects.obj21 = 0;
    objects.obj22 = 0;
    objects.obj23 = 0;
    objects.obj24 = 0;
    objects.obj25 = 0;
    objects.obj26 = 0;
    objects.obj27 = 0;
    objects.chane_gir = 0;
    objects.obj2 = 0;
    objects.dubble_head_button = 0;
    objects.btn_single_dubble_install = 0;
    objects.corner_cleaner_button = 0;
    objects.btn_single_head_install = 0;
    objects.single_head_button = 0;
    objects.obj3 = 0;
    objects.waiting_panle_1 = 0;
    objects.obj28 = 0;
    objects.obj29 = 0;
    objects.download_metrics_label = 0;
    objects.download_cancel_btn = 0;
    objects.download_cancel_label = 0;
    objects.cutter_single_button = 0;
    objects.btn_cutter_single_install = 0;
    objects.ver_cutter_single = 0;
    objects.cutter_double_button = 0;
    objects.btn_cutter_double_install = 0;
    objects.ver_cutter_double = 0;
    objects.welder_double_button = 0;
    objects.btn_welder_double_install = 0;
    objects.ver_welder_double = 0;
    objects.gas_button = 0;
    objects.btn_gas_install = 0;
    objects.ver_gas = 0;
    objects.ver_dough = 0;
    objects.ver_welder_single = 0;
    objects.ver_test = 0;
    objects.ver_sw1 = 0;
    objects.incompat_welder_single = 0;
    objects.incompat_welder_double = 0;
    objects.incompat_cutter_double = 0;
    objects.cards_row = 0;
    for (int i = 0; i < 9; i++) { objects.card_btn[i] = 0; objects.card_ver[i] = 0; objects.card_incompat[i] = 0; }
    objects.confirm_panel = 0;
    objects.confirm_label = 0;
    objects.confirm_yes = 0;
    objects.confirm_no = 0;
    objects.link_entry_btn = 0;
    objects.link_entry_panel = 0;
    objects.link_entry_textarea = 0;
    objects.link_entry_keyboard = 0;
    objects.link_entry_confirm = 0;
    objects.link_entry_cancel = 0;
    objects.link_entry_error = 0;
    objects.version_panel = 0;
    objects.version_upload = 0;
    objects.version_scroll = 0;
    objects.version_close = 0;
    deletePageFlowState(3);
}

void tick_screen_screen_framware() {
    if (!objects.screen_framware || !lv_obj_is_valid(objects.screen_framware)) return;

    // تضمین نمایش قطعی نسخه بالای کلید ورود لینک در هر دو صفحه UPVC و OTHER
    if (s_brand_version_label && lv_obj_is_valid(s_brand_version_label)) {
        const char *cur = lv_label_get_text(s_brand_version_label);
        const char *v = get_var_app_version();
        if (!v || v[0] == '\0') v = VERSION;
        if (!cur || strcmp(cur, v) != 0) {
            lv_label_set_text(s_brand_version_label, v);
        }
    }

    // بازیابی قطعی اسکرول در اولین تیک پس از آماده‌سازی کامل لایوت LVGL
    if (!s_cards_scroll_restored && objects.cards_row && lv_obj_is_valid(objects.cards_row)) {
        int16_t saved_sx = app_get_cards_scroll_x();
        if (saved_sx > 0) {
            lv_obj_update_layout(objects.cards_row);
            lv_obj_scroll_to_x(objects.cards_row, saved_sx, LV_ANIM_OFF);
        }
        s_cards_scroll_restored = true;
    }

    // زمان‌بندی محو خودکار پیام وضعیت لینک (زمان‌دار)
    if (s_link_msg_hide_ms > 0 && lv_tick_get() >= s_link_msg_hide_ms) {
        link_entry_hide_message();
    }

    // به‌روزرسانی نتیجه‌ی استعلام سرور برای لینک سفارشی
    if (link_check_is_done()) {
        link_check_clear_done();
        int st = link_check_get_status();
        if (st == 2 || st == 3) {
            // LINK_CHECK_OK_INFO (2) یا LINK_CHECK_OK_BIN (3): سبز زمردی شیک اپل
            link_entry_show_message(link_check_get_result(), 0x064e3b, 0xecfdf5, 0x10b981, 4000);
            app_sound_play(SOUND_SUCCESS);
        } else {
            // خطا یا عدم وجود فایل در سرور: زرشکی تیره شیک اپل
            link_entry_show_message(link_check_get_result(), 0x3b0712, 0xfca5a5, 0xef4444, 4000);
            app_sound_play(SOUND_ERROR);
        }
    }

    // ذخیره‌سازی موقعیت اسکرول کارت‌ها با تاخیر ۱ ثانیه پس از توقف کامل حرکت (محافظت از حافظه فلش)
    if (g_cards_scroll_dirty && (lv_tick_get() - g_cards_scroll_dirty_ms >= 1000)) {
        g_cards_scroll_dirty = false;
        if (objects.cards_row && lv_obj_is_valid(objects.cards_row)) {
            int16_t sx = (int16_t)lv_obj_get_scroll_x(objects.cards_row);
            if (sx < 0) sx = 0;
            app_save_cards_scroll_x(sx);
        }
    }

    void *flowState = getFlowState(0, 3);
    if (!flowState) return;
    if (objects.waiting_panle_1 && lv_obj_is_valid(objects.waiting_panle_1)) {
        bool new_val = evalBooleanProperty(flowState, 35, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.waiting_panle_1, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.waiting_panle_1;
            if (new_val) lv_obj_add_flag(objects.waiting_panle_1, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.waiting_panle_1, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    if (objects.obj28 && lv_obj_is_valid(objects.obj28)) {
        const char *new_val = evalTextProperty(flowState, 36, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj28);
        if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj28;
            lv_label_set_text(objects.obj28, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void change_color_theme(uint32_t theme_index) {
    if (theme_index >= 2) return;
    if (objects.obj0 && lv_obj_is_valid(objects.obj0)) lv_obj_set_style_bg_color(objects.obj0, lv_color_hex(theme_colors[theme_index][1]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.waiting_panle_2 && lv_obj_is_valid(objects.waiting_panle_2)) lv_obj_set_style_bg_color(objects.waiting_panle_2, lv_color_hex(theme_colors[theme_index][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.btn_wifi_connect && lv_obj_is_valid(objects.btn_wifi_connect)) lv_obj_set_style_bg_color(objects.btn_wifi_connect, lv_color_hex(theme_colors[theme_index][1]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.btn_wifi_redresh && lv_obj_is_valid(objects.btn_wifi_redresh)) lv_obj_set_style_bg_color(objects.btn_wifi_redresh, lv_color_hex(theme_colors[theme_index][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.obj1 && lv_obj_is_valid(objects.obj1)) lv_obj_set_style_bg_color(objects.obj1, lv_color_hex(theme_colors[theme_index][1]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.waiting_panle && lv_obj_is_valid(objects.waiting_panle)) lv_obj_set_style_bg_color(objects.waiting_panle, lv_color_hex(theme_colors[theme_index][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)) lv_obj_set_style_bg_color(objects.screen_framware, lv_color_hex(theme_colors[theme_index][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.obj2 && lv_obj_is_valid(objects.obj2)) lv_obj_set_style_bg_color(objects.obj2, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.btn_single_dubble_install && lv_obj_is_valid(objects.btn_single_dubble_install)) lv_obj_set_style_bg_color(objects.btn_single_dubble_install, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.btn_single_head_install && lv_obj_is_valid(objects.btn_single_head_install)) lv_obj_set_style_bg_color(objects.btn_single_head_install, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.obj3 && lv_obj_is_valid(objects.obj3)) lv_obj_set_style_bg_color(objects.obj3, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);

    if (objects.btn_cutter_single_install && lv_obj_is_valid(objects.btn_cutter_single_install)) lv_obj_set_style_bg_color(objects.btn_cutter_single_install, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.btn_cutter_double_install && lv_obj_is_valid(objects.btn_cutter_double_install)) lv_obj_set_style_bg_color(objects.btn_cutter_double_install, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.btn_welder_double_install && lv_obj_is_valid(objects.btn_welder_double_install)) lv_obj_set_style_bg_color(objects.btn_welder_double_install, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.btn_gas_install && lv_obj_is_valid(objects.btn_gas_install)) lv_obj_set_style_bg_color(objects.btn_gas_install, lv_color_hex(theme_colors[theme_index][2]), LV_PART_MAIN | LV_STATE_DEFAULT);

    if (objects.waiting_panle_1 && lv_obj_is_valid(objects.waiting_panle_1)) lv_obj_set_style_bg_color(objects.waiting_panle_1, lv_color_hex(theme_colors[theme_index][0]), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    if (objects.screen_splash && lv_obj_is_valid(objects.screen_splash)) lv_obj_invalidate(objects.screen_splash);
    if (objects.screen_password && lv_obj_is_valid(objects.screen_password)) lv_obj_invalidate(objects.screen_password);
    if (objects.screen_wifi && lv_obj_is_valid(objects.screen_wifi)) lv_obj_invalidate(objects.screen_wifi);
    if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)) lv_obj_invalidate(objects.screen_framware);
}

extern void add_style(lv_obj_t *obj, int32_t styleIndex);
extern void remove_style(lv_obj_t *obj, int32_t styleIndex);

static const char *screen_names[] = { "Screen_Splash", "Screen_Password", "Screen_Wifi", "Screen_Framware" };
static const char *object_names[] = { "screen_splash", "screen_password", "screen_wifi", "screen_framware", "panle_wifi_password_1", "frameware_password_keybord", "text_area_framware_password", "obj0", "drop_down_wifi_list", "btn_wifi_connect", "btn_wifi_redresh", "panle_wifi_password", "obj1", "keybord", "text_area_wifi_password", "language_panle", "obj2", "btn_single_dubble_install", "btn_single_head_install", "obj3", "obj4", "obj5", "obj6", "waiting_panle_2", "obj7", "obj8", "obj9", "obj10", "waiting_panle", "obj11", "obj12", "obj13", "obj14", "obj15", "obj16", "info_panle", "obj17", "obj18", "obj19", "seriall_number", "obj20", "obj21", "obj22", "obj23", "obj24", "obj25", "obj26", "obj27", "chane_gir", "dubble_head_button", "corner_cleaner_button", "single_head_button", "waiting_panle_1", "obj28", "obj29" };
static const char *style_names[] = { "Error_Style" };
static const char *theme_names[] = { "Dark", "Light" };

uint32_t theme_colors[2][3] = {
    { 0xff404040, 0xff2196f3, 0xff29ac6b },
    { 0xff3161f2, 0xff000000, 0xff000000 },
};


typedef void (*create_screen_func_t)();
create_screen_func_t create_screen_funcs[] = {
    create_screen_screen_splash,
    create_screen_screen_password,
    create_screen_screen_wifi,
    create_screen_screen_framware,
};
void create_screen(int screen_index) {
    if (screen_index < 0 || screen_index >= 4) return;
    create_screen_funcs[screen_index]();
}
void create_screen_by_id(enum ScreensEnum screenId) {
    int idx = (int)screenId - 1;
    if (idx < 0 || idx >= 4) return;
    create_screen_funcs[idx]();
}

typedef void (*delete_screen_func_t)();
delete_screen_func_t delete_screen_funcs[] = {
    delete_screen_screen_splash,
    delete_screen_screen_password,
    delete_screen_screen_wifi,
    delete_screen_screen_framware,
};
void delete_screen(int screen_index) {
    if (screen_index < 0 || screen_index >= 4) return;
    delete_screen_funcs[screen_index]();
}
void delete_screen_by_id(enum ScreensEnum screenId) {
    int idx = (int)screenId - 1;
    if (idx < 0 || idx >= 4) return;
    delete_screen_funcs[idx]();
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_screen_splash,
    tick_screen_screen_password,
    tick_screen_screen_wifi,
    tick_screen_screen_framware,
};
void tick_screen(int screen_index) {
    if (screen_index < 0 || screen_index >= 4) return;
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    int idx = (int)screenId - 1;
    if (idx < 0 || idx >= 4) return;
    tick_screen_funcs[idx]();
}

void create_screens() {
    eez_flow_init_styles(add_style, remove_style);
    
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    eez_flow_init_style_names(style_names, sizeof(style_names) / sizeof(const char *));
    eez_flow_init_themes(theme_names, sizeof(theme_names) / sizeof(const char *), change_color_theme);
    
    eez_flow_set_create_screen_func(create_screen);
    eez_flow_set_delete_screen_func(delete_screen);
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_screen_splash();
}

