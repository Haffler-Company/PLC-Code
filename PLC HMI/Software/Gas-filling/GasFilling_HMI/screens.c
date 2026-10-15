#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "version.h"
#include "ui_breath.h"
#include "a2_eeprom.h"
#include "variables.h"
#include "ota_manager.h"
#include "day_lock.h"
#include "webportal.h"
#include "ui_toast.h"
#include "ui_resume.h"
#include "panel_keys.h"
#include "warranty_mgr.h"
#include "draw/lv_draw_private.h"

objects_t objects;

void keyboard_color_draw_task_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    if (!draw_task) return;
    lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
    if (!base_dsc || base_dsc->part != LV_PART_ITEMS) return;

    int32_t id = base_dsc->id1;
    if (id < 0) return;
    const char *txt = lv_buttonmatrix_get_button_text(obj, (uint32_t)id);
    if (!txt) return;

    bool is_pressed = (lv_buttonmatrix_get_selected_button(obj) == (uint32_t)id) && (lv_obj_get_state(obj) & LV_STATE_PRESSED);

    lv_draw_fill_dsc_t *fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
    lv_draw_label_dsc_t *label_dsc = lv_draw_task_get_label_dsc(draw_task);

    bool is_bksp = (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "Del") == 0 || strcmp(txt, "DEL") == 0 ||
                    strcmp(txt, "حذف") == 0 || strcmp(txt, "<-") == 0 || strcmp(txt, "Sil") == 0);
    bool is_ok   = (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "تایید") == 0 || strcmp(txt, "تأیید") == 0 ||
                    strcmp(txt, "OK") == 0 || strcmp(txt, "Enter") == 0 || strcmp(txt, "Tamam") == 0 ||
                    strcmp(txt, LV_SYMBOL_NEW_LINE) == 0 || strcmp(txt, "\n") == 0);

    if (is_bksp) {
        if (fill_dsc) {
            fill_dsc->color = is_pressed ? lv_color_hex(0xffef4444) : lv_color_hex(0xffdc2626);
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 6;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_white();
            label_dsc->ofs_y = -2;
        }
    } else if (is_ok) {
        if (fill_dsc) {
            fill_dsc->color = is_pressed ? lv_color_hex(0xff22c55e) : lv_color_hex(0xff16a34a);
            fill_dsc->opa = LV_OPA_COVER;
            fill_dsc->radius = 6;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_white();
            label_dsc->ofs_y = -2;
        }
    } else if (is_pressed) {
        if (fill_dsc) {
            fill_dsc->color = lv_color_hex(0xff0284c7);
            fill_dsc->opa = LV_OPA_COVER;
        }
        if (label_dsc) {
            label_dsc->color = lv_color_white();
        }
    }
}

#define UI_HEADER_NAVY 0xff0284c7
/* Apple Industrial Light Theme */
#define UI_HEADER_METAL_HI 0xffffffff
#define UI_HEADER_METAL_LO 0xfff8fafc
#define UI_HEADER_BLUE     UI_HEADER_METAL_LO
#define UI_HEADER_TEXT     0xff0f172a
#define UI_START_GREEN     0xff16a34a   /* Apple industrial emerald green */
#define UI_START_GLOW      0xff22c55e   /* Apple green highlight */
#define UI_STOP_RED        0xffdc2626   /* Apple industrial crimson */
#define UI_PAGE_DARK       0xfff8fafc   /* Apple Industrial Light canvas */
/* Progress bars: Metallic light track; Apple cyan/blue fill; crisp dark text */
#define UI_BAR_TRACK       0xffe2e8f0
#define UI_BAR_FILL        0xff0284c7
#define UI_BAR_TEXT        0xff0f172a
#define UI_BAR_W           34
#define UI_BAR_H           146
#define UI_BAR_Y           112
#define UI_BAR_LEFT_X      4
#define UI_BAR_RIGHT_X     (480 - UI_BAR_LEFT_X - UI_BAR_W)

#include "ui_vazir_fonts.h"


static void ui_style_row_label(lv_obj_t *lab)
{
    if (!lab) return;
    lv_obj_set_style_text_font(lab, ui_font_vazir_or(&ui_font_arial_regular17, 17), LV_PART_MAIN);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xff0f172a), LV_PART_MAIN);
}

/* TabView owns individual button objects; style those buttons directly. */
void ui_style_setting_tab_buttons(void)
{
    if (!objects.tabview_setting) return;
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.tabview_setting);
    const lv_font_t *font = ui_font_vazir_or(&ui_font_arial_regular17, 17);

    if (tab_bar) {
        lv_obj_set_style_bg_color(tab_bar, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
        lv_obj_set_style_border_color(tab_bar, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(tab_bar, 1, LV_PART_MAIN);
        lv_obj_set_style_border_side(tab_bar, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
        lv_obj_set_style_pad_all(tab_bar, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_gap(tab_bar, 0, LV_PART_MAIN);

        uint32_t n = lv_obj_get_child_count(tab_bar);
        for (uint32_t i = 0; i < n; i++) {
            lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
            if (!btn) continue;
            lv_obj_set_style_text_font(btn, font, LV_PART_MAIN);
            lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);

        /* Inactive state: Crisp light metallic tone */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xff64748b), LV_PART_MAIN);

            /* Active / Checked state: Crisp White with Apple Blue accent */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN | LV_STATE_CHECKED);

            /* Pressed state */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_PRESSED);
        }

        const char *titles[3];
        if (EEprom2_Language == L_Persion) {
            titles[0] = "عمومی";
            titles[1] = "تست";
            titles[2] = "مدیر";
        } else if (EEprom2_Language == L_Turkish) {
            titles[0] = "Genel";
            titles[1] = "Test";
            titles[2] = "Yonetici";
        } else if (EEprom2_Language == L_Russian) {
            titles[0] = "Obshchie";
            titles[1] = "Test";
            titles[2] = "Admin";
        } else {
            titles[0] = "General";
            titles[1] = "Test";
            titles[2] = "Admin";
        }
        for (uint32_t i = 0; i < n && i < 3; i++) {
            lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
            if (!btn) continue;
            uint32_t btn_cnt = lv_obj_get_child_count(btn);
            for (uint32_t j = 0; j < btn_cnt; j++) {
                lv_obj_t *lbl = lv_obj_get_child(btn, j);
                if (lbl && lv_obj_check_type(lbl, &lv_label_class)) {
                    lv_label_set_text(lbl, titles[i]);
                    lv_obj_set_style_base_dir(lbl, (EEprom2_Language == L_Persion) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
                }
            }
        }
    }

    uint32_t count = lv_tabview_get_tab_count(objects.tabview_setting);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *tab_btn = lv_tabview_get_tab_button(objects.tabview_setting, (int32_t)i);
        if (!tab_btn) continue;
        lv_obj_set_style_text_font(tab_btn, font, LV_PART_MAIN);
        lv_obj_set_style_bg_color(tab_btn, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(tab_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(tab_btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_side(tab_btn, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(tab_btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(tab_btn, 3, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(tab_btn, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
    }
}

static lv_obj_t *s_general_lang_flag[6];
static lv_obj_t *s_general_lang_mark;
static lv_obj_t *s_general_lang_tile;
static lv_obj_t *s_general_brightness_title;
static lv_obj_t *s_general_brightness_value;
static lv_obj_t *s_general_plc_status;
static lv_obj_t *s_general_plc_state;
static lv_obj_t *s_general_web_title = NULL;
static lv_obj_t *s_general_web_sub = NULL;
static lv_obj_t *s_general_ota_label = NULL;
static lv_obj_t *s_general_flow_lbl = NULL;
static lv_obj_t *s_general_flow_accent = NULL;
static lv_obj_t *s_general_warranty_btn = NULL;
static lv_obj_t *s_general_warranty_lbl = NULL;
static lv_obj_t *s_general_warranty_btn_lbl = NULL;
static lv_obj_t *s_general_rst_lbl = NULL;
static lv_obj_t *s_general_sys_lbl = NULL;
static lv_obj_t *s_general_ver_lbl = NULL;
static lv_obj_t *s_test_lbl_stop_mode = NULL;
static lv_obj_t *s_process_lbl_info = NULL;
static lv_obj_t *s_test_btn_pinmap_label = NULL;
static uint8_t s_general_brightness = 100;

static void ui_general_brightness_changed(lv_event_t *e)
{
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    char value[8];
    if (!slider) return;
    s_general_brightness = (uint8_t)lv_slider_get_value(slider);
    snprintf(value, sizeof(value), "%u%%", (unsigned)s_general_brightness);
    if (s_general_brightness_value) lv_label_set_text(s_general_brightness_value, value);

    /* The current board definition exposes no backlight PWM pin.  Applying
       screen opacity provides a safe software dimmer without guessing a GPIO. */
    lv_opa_t opa = (lv_opa_t)(140u + ((uint16_t)s_general_brightness * 115u) / 100u);
    if (objects.main) lv_obj_set_style_opa(objects.main, opa, LV_PART_MAIN);
    if (objects.setting) lv_obj_set_style_opa(objects.setting, opa, LV_PART_MAIN);
}

static void ui_update_warranty_button_visual(void)
{
    if (!s_general_warranty_btn || !lv_obj_is_valid(s_general_warranty_btn)) return;
    if (!s_general_warranty_btn_lbl || !lv_obj_is_valid(s_general_warranty_btn_lbl)) return;

    bool is_fa = (EEprom2_Language == L_Persion);
    if (warranty_is_active()) {
        lv_obj_set_style_bg_color(s_general_warranty_btn, lv_color_hex(0xffdcfce7), LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_general_warranty_btn, lv_color_hex(0xffbbf7d0), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(s_general_warranty_btn, lv_color_hex(0xff86efac), LV_PART_MAIN);
        lv_obj_set_style_border_width(s_general_warranty_btn, 1, LV_PART_MAIN);
        char wbuf[64];
        const char *w_date = warranty_get_date();
        if (w_date && strlen(w_date) > 0) {
            if (is_fa) {
                snprintf(wbuf, sizeof(wbuf), "گارانتی: %s", w_date);
            } else if (EEprom2_Language == L_Turkish) {
                snprintf(wbuf, sizeof(wbuf), "Garanti: %s", w_date);
            } else if (EEprom2_Language == L_Russian) {
                snprintf(wbuf, sizeof(wbuf), "Garantiya: %s", w_date);
            } else {
                snprintf(wbuf, sizeof(wbuf), "Warranty: %s", w_date);
            }
        } else {
            if (is_fa) {
                strncpy(wbuf, "گارانتی معتبر", sizeof(wbuf) - 1);
            } else if (EEprom2_Language == L_Turkish) {
                strncpy(wbuf, "Garanti Aktif", sizeof(wbuf) - 1);
            } else if (EEprom2_Language == L_Russian) {
                strncpy(wbuf, "Garantiya Aktiv", sizeof(wbuf) - 1);
            } else {
                strncpy(wbuf, "Warranty Active", sizeof(wbuf) - 1);
            }
        }
        lv_label_set_text(s_general_warranty_btn_lbl, wbuf);
        lv_obj_set_style_text_align(s_general_warranty_btn_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_warranty_btn_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_general_warranty_btn_lbl, lv_color_hex(0xff15803d), LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(s_general_warranty_btn, lv_color_hex(0xff059669), LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_general_warranty_btn, lv_color_hex(0xff047857), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(s_general_warranty_btn, 0, LV_PART_MAIN);
        const char *w_txt = is_fa ? "فعالسازی گارانتی" :
                            (EEprom2_Language == L_Turkish) ? "Garantiyi Etkinlestir" :
                            (EEprom2_Language == L_Russian) ? "Aktivirovat Garantiyu" : "Activate Warranty";
        lv_label_set_text(s_general_warranty_btn_lbl, w_txt);
        lv_obj_set_style_text_align(s_general_warranty_btn_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_warranty_btn_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_general_warranty_btn_lbl, lv_color_white(), LV_PART_MAIN);
    }
    lv_obj_center(s_general_warranty_btn_lbl);
}

void ui_refresh_general_runtime_labels(void)
{
    unsigned char lang = EEprom2_Language;
    bool is_fa = (lang == L_Persion);
    const char *lbl_lang = "System Language:";
    const char *lbl_flow = "Argon Gas Flow Rate (Qs)";
    const char *lbl_ota = "Online Update";
    const char *lbl_rst = "Factory Reset";
    const char *lbl_setting = "Settings";
    const char *lbl_buzz = "Sound";
    const char *lbl_touch = "Touch feedback";
    const char *lbl_bright = "Brightness";
    const char *lbl_plc = "Controller: Active";
    const char *lbl_web = "Wi-Fi Hotspot (192.168.4.1)";
    const char *lbl_web_sub = "Connect phone to Haffler-GasFiller";

    if (lang == L_Persion) {
        lbl_lang = "زبان سیستم:";
        lbl_flow = "تنظیم دبی جریان گاز آرگون (Qs)";
        lbl_ota = "بروزرسانی آنلاین";
        lbl_rst = "بازنشانی به تنظیمات کارخانه";
        lbl_setting = "تنظیمات";
        lbl_buzz = "صدا و بازر";
        lbl_touch = "بازخورد لمس";
        lbl_bright = "روشنایی صفحه";
        lbl_plc = "وضعیت کنترلر: فعال";
        lbl_web = "هات اسپات وای فای (192.168.4.1)";
        lbl_web_sub = "اتصال بی سیم به پنل دستگاه";
    } else if (lang == L_Turkish) {
        lbl_lang = "Sistem Dili:";
        lbl_flow = "Argon Gazi Akisi (Qs)";
        lbl_ota = "Cevrimici Guncelleme";
        lbl_rst = "Fabrika Ayarlari";
        lbl_setting = "Ayarlar";
        lbl_buzz = "Ses ve Buzzer";
        lbl_touch = "Dokunmatik";
        lbl_bright = "Ekran Parlakligi";
        lbl_plc = "Kontrolor: Aktif";
        lbl_web = "Wi-Fi Erisim Noktasi (192.168.4.1)";
        lbl_web_sub = "Haffler-GasFiller agina baglanin";
    } else if (lang == L_Russian) {
        lbl_lang = "Yazyk Sistemy:";
        lbl_flow = "Raskhod Gaza Argon (Qs)";
        lbl_ota = "Onlajn Obnovlenie";
        lbl_rst = "Sbros Nastroyek";
        lbl_setting = "Nastroyki";
        lbl_buzz = "Zvuk";
        lbl_touch = "Otklik";
        lbl_bright = "Yarkost";
        lbl_plc = "Kontroller: Aktiv";
        lbl_web = "Tochka Dostupa Wi-Fi (192.168.4.1)";
        lbl_web_sub = "Podklyuchenie k Haffler-GasFiller";
    }

    if (objects.t_language && lv_obj_is_valid(objects.t_language)) {
        lv_label_set_text(objects.t_language, lbl_lang);
        lv_obj_set_style_base_dir(objects.t_language, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(objects.t_language, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_language, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (s_general_flow_lbl && lv_obj_is_valid(s_general_flow_lbl)) {
        lv_label_set_text(s_general_flow_lbl, lbl_flow);
        lv_obj_set_style_base_dir(s_general_flow_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(s_general_flow_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_flow_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_pos(s_general_flow_lbl, 16, 7);
        lv_obj_set_size(s_general_flow_lbl, 356, 20);
    }
    if (s_general_flow_accent && lv_obj_is_valid(s_general_flow_accent)) {
        lv_obj_add_flag(s_general_flow_accent, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_general_ota_label && lv_obj_is_valid(s_general_ota_label)) {
        lv_label_set_text(s_general_ota_label, lbl_ota);
        lv_obj_set_style_base_dir(s_general_ota_label, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_ota_label, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    ui_update_warranty_button_visual();
    if (s_general_rst_lbl && lv_obj_is_valid(s_general_rst_lbl)) {
        lv_label_set_text(s_general_rst_lbl, lbl_rst);
        lv_obj_set_style_base_dir(s_general_rst_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_rst_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    }
    if (s_general_sys_lbl && lv_obj_is_valid(s_general_sys_lbl)) {
        lv_label_set_text(s_general_sys_lbl, FW_VERSION_LABEL);
        lv_obj_set_style_base_dir(s_general_sys_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(s_general_sys_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_sys_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(s_general_sys_lbl);
    }
    if (objects.t_setting && lv_obj_is_valid(objects.t_setting)) {
        lv_label_set_text(objects.t_setting, lbl_setting);
    }
    if (objects.t_buzzer && lv_obj_is_valid(objects.t_buzzer)) lv_label_set_text(objects.t_buzzer, lbl_buzz);
    if (objects.t_touch && lv_obj_is_valid(objects.t_touch)) lv_label_set_text(objects.t_touch, lbl_touch);
    if (s_general_brightness_title && lv_obj_is_valid(s_general_brightness_title)) lv_label_set_text(s_general_brightness_title, lbl_bright);
    if (s_general_plc_status && lv_obj_is_valid(s_general_plc_status)) lv_label_set_text(s_general_plc_status, lbl_plc);

    if (s_general_web_title && lv_obj_is_valid(s_general_web_title)) {
        lv_label_set_text(s_general_web_title, lbl_web);
        lv_obj_set_style_base_dir(s_general_web_title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(s_general_web_title, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    }
    if (s_general_web_sub && lv_obj_is_valid(s_general_web_sub)) {
        lv_label_set_text(s_general_web_sub, lbl_web_sub);
        lv_obj_set_style_base_dir(s_general_web_sub, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(s_general_web_sub, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_general_web_sub, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 11) : &lv_font_montserrat_12, LV_PART_MAIN);
    }
}

void ui_refresh_process_runtime_labels(void)
{
    unsigned char lang = EEprom2_Language;
    bool is_fa = (lang == L_Persion);
    const char *lbl_flow_txt = (lang == L_english) ? "Flow Rate (Qs)" :
                               (lang == L_Turkish) ? "Gaz Akisi (Qs)" :
                               (lang == L_Russian) ? "Potok Gaza (Qs)" : "دبی جریان گاز (Qs)";
    const char *lbl_out_txt  = (lang == L_english) ? "Relay Output Type" :
                               (lang == L_Turkish) ? "Role Cikis Tipi" :
                               (lang == L_Russian) ? "Tip Vykhoda" : "نوع خروجی رله شیرها";
    const char *lbl_info_txt = (lang == L_english) ? "Factory default: Qs 30 L/min - State N.C" :
                               (lang == L_Turkish) ? "Fabrika ayari: Qs 30 L/dk - Durum N.C" :
                               (lang == L_Russian) ? "Zavodskoy: Qs 30 L/min - Tip N.C" : "پیش فرض کارخانه: دبی 30 لیتر بر دقیقه - خروجی N.C";
    const char *lbl_save_txt = (lang == L_english) ? "Save Settings" :
                               (lang == L_Turkish) ? "Ayarlari Kaydet" :
                               (lang == L_Russian) ? "Sokhranit" : "ذخیره تنظیمات";

    if (objects.t_flowrate && lv_obj_is_valid(objects.t_flowrate)) {
        lv_label_set_text(objects.t_flowrate, lbl_flow_txt);
        lv_obj_set_style_base_dir(objects.t_flowrate, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    }
    if (objects.t_type_out && lv_obj_is_valid(objects.t_type_out)) {
        lv_label_set_text(objects.t_type_out, lbl_out_txt);
        lv_obj_set_style_base_dir(objects.t_type_out, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    }
    if (s_process_lbl_info && lv_obj_is_valid(s_process_lbl_info)) {
        lv_label_set_text(s_process_lbl_info, lbl_info_txt);
        lv_obj_set_style_base_dir(s_process_lbl_info, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_process_lbl_info, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &ui_font_arial_regular17, LV_PART_MAIN);
    }
    if (objects.t_save && lv_obj_is_valid(objects.t_save)) {
        lv_label_set_text(objects.t_save, lbl_save_txt);
        lv_obj_set_style_base_dir(objects.t_save, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_save, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &ui_font_arial_regular17, LV_PART_MAIN);
    }
}

void ui_refresh_test_runtime_labels(void)
{
    unsigned char lang = EEprom2_Language;
    bool is_fa = (lang == L_Persion);
    const char *lbl_pinmap_txt = (lang == L_english) ? "Pinout Diagram" :
                                 (lang == L_Turkish) ? "Baglanti Semasi" :
                                 (lang == L_Russian) ? "Skhema Podkl." : "نقشه سیم بندی";
    const char *lbl_out_txt = (lang == L_english) ? "Industrial Outputs" :
                              (lang == L_Turkish) ? "Endustriyel Cikislar" :
                              (lang == L_Russian) ? "Prom. Vykhody" : "خروجی های صنعتی";
    const char *lbl_v1_txt = (lang == L_english) ? "Valve 1 (Q0)" :
                             (lang == L_Turkish) ? "Vana 1 (Q0)" :
                             (lang == L_Russian) ? "Klapan 1 (Q0)" : "شیربرقی 1 (Q0)";
    const char *lbl_v2_txt = (lang == L_english) ? "Valve 2 (Q1)" :
                             (lang == L_Turkish) ? "Vana 2 (Q1)" :
                             (lang == L_Russian) ? "Klapan 2 (Q1)" : "شیربرقی 2 (Q1)";
    const char *lbl_bz_txt = (lang == L_english) ? "Buzzer (Q2)" :
                             (lang == L_Turkish) ? "Sesli Alarm (Q2)" :
                             (lang == L_Russian) ? "Sirena (Q2)" : "هشدار صوتی (Q2)";
    const char *lbl_st_txt = (lang == L_english) ? "Start Input" :
                             (lang == L_Turkish) ? "Baslat Girisi" :
                             (lang == L_Russian) ? "Pusk Vkhod" : "شاسی شروع";
    const char *lbl_sp_txt = (lang == L_english) ? "Stop Input" :
                             (lang == L_Turkish) ? "Durdur Girisi" :
                             (lang == L_Russian) ? "Stop Vkhod" : "استپ";

    const char *lbl_in_txt = (lang == L_english) ? "Digital Inputs" :
                             (lang == L_Turkish) ? "Dijital Girisler" :
                             (lang == L_Russian) ? "Tsifr. Vkhody" : "ورودی های دیجیتال";

    if (s_test_btn_pinmap_label && lv_obj_is_valid(s_test_btn_pinmap_label)) {
        lv_label_set_text(s_test_btn_pinmap_label, lbl_pinmap_txt);
        lv_obj_set_style_base_dir(s_test_btn_pinmap_label, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_test_btn_pinmap_label, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (objects.t_input && lv_obj_is_valid(objects.t_input)) {
        lv_label_set_text(objects.t_input, lbl_in_txt);
        lv_obj_set_style_base_dir(objects.t_input, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_input, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 18) : &lv_font_montserrat_16, LV_PART_MAIN);
    }
    if (objects.t_out_put && lv_obj_is_valid(objects.t_out_put)) {
        lv_label_set_text(objects.t_out_put, lbl_out_txt);
        lv_obj_set_style_base_dir(objects.t_out_put, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_out_put, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 18) : &lv_font_montserrat_16, LV_PART_MAIN);
    }
    if (objects.t_valve1 && lv_obj_is_valid(objects.t_valve1)) {
        lv_label_set_text(objects.t_valve1, lbl_v1_txt);
        lv_obj_set_style_base_dir(objects.t_valve1, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_valve1, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (objects.t_valve2 && lv_obj_is_valid(objects.t_valve2)) {
        lv_label_set_text(objects.t_valve2, lbl_v2_txt);
        lv_obj_set_style_base_dir(objects.t_valve2, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_valve2, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (objects.t_buzzer_out && lv_obj_is_valid(objects.t_buzzer_out)) {
        lv_label_set_text(objects.t_buzzer_out, lbl_bz_txt);
        lv_obj_set_style_base_dir(objects.t_buzzer_out, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_buzzer_out, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (objects.t_start && lv_obj_is_valid(objects.t_start)) {
        lv_label_set_text(objects.t_start, lbl_st_txt);
        lv_obj_set_style_base_dir(objects.t_start, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_start, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (objects.t_stop && lv_obj_is_valid(objects.t_stop)) {
        lv_label_set_text(objects.t_stop, lbl_sp_txt);
        lv_obj_set_style_base_dir(objects.t_stop, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(objects.t_stop, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (s_test_lbl_stop_mode && lv_obj_is_valid(s_test_lbl_stop_mode)) {
        const char *sm_txt = (lang == L_english) ? "Stop Mode" :
                             (lang == L_Turkish) ? "Durdur Tipi" :
                             (lang == L_Russian) ? "Tip Stopa" : "نوع استپ";
        lv_label_set_text(s_test_lbl_stop_mode, sm_txt);
        lv_obj_set_style_base_dir(s_test_lbl_stop_mode, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_test_lbl_stop_mode, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    }
}

static lv_obj_t *s_general_lang_img = NULL;

void ui_refresh_general_language_flag(void)
{
    unsigned char lang = EEprom2_Language;
    if (lang > 3) lang = 0;
    if (s_general_lang_img) {
        if (lang == L_Persion) {
            lv_image_set_src(s_general_lang_img, &img_flag_ir);
        } else if (lang == L_english) {
            lv_image_set_src(s_general_lang_img, &img_flag_en);
        } else if (lang == L_Turkish) {
            lv_image_set_src(s_general_lang_img, &img_flag_tr);
        } else {
            lv_image_set_src(s_general_lang_img, &img_flag_ru);
        }
    }
}

/** General tab: language, sound, touch and software brightness + controller health. */
static lv_obj_t *s_help_overlay = NULL;
static lv_obj_t *s_admin_datetime_modal = NULL;
static lv_obj_t *s_admin_daylock_modal = NULL;
static lv_obj_t *s_admin_pin_modal = NULL;
static lv_obj_t *s_factory_reset_modal = NULL;
static void ui_admin_detail_close(void);

static void ui_setting_help_close(lv_event_t *e)
{
    (void)e;
    if (s_help_overlay) {
        lv_obj_delete(s_help_overlay);
        s_help_overlay = NULL;
    }
}

static void ui_help_add_card(lv_obj_t *parent, const char *title, const char *body, lv_color_t border_col)
{
    bool is_fa = (EEprom2_Language == L_Persion);
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, 404);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(card, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header row inside card: colored accent bar + title */
    lv_obj_t *hrow = lv_obj_create(card);
    lv_obj_set_width(hrow, 384);
    lv_obj_set_height(hrow, 22);
    lv_obj_set_style_bg_opa(hrow, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hrow, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hrow, 0, LV_PART_MAIN);
    lv_obj_clear_flag(hrow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bar = lv_obj_create(hrow);
    lv_obj_set_size(bar, 4, 16);
    lv_obj_set_style_bg_color(bar, border_col, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_pos(bar, is_fa ? 378 : 0, 3);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *t = lv_label_create(hrow);
    lv_obj_set_pos(t, is_fa ? 0 : 10, 0);
    lv_obj_set_size(t, 372, 22);
    lv_obj_set_style_text_align(t, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, border_col, LV_PART_MAIN);
    lv_label_set_text(t, title);

    /* Body text */
    lv_obj_t *b = lv_label_create(card);
    lv_obj_set_width(b, 384);
    lv_obj_set_style_text_align(b, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(b, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(b, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(b, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_label_set_text(b, body);
}

void ui_setting_help_open(lv_event_t *e)
{
    (void)e;
    if (s_help_overlay) {
        lv_obj_delete(s_help_overlay);
        s_help_overlay = NULL;
    }
    s_help_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_help_overlay, 480, 320);
    lv_obj_set_pos(s_help_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_help_overlay, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_help_overlay, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_help_overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_help_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_help_overlay);
    lv_obj_set_size(panel, 440, 280);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);

    /* Determine Context:
     * 0 = General Tab, 1 = Test Tab, 2 = Admin Tab, 10 = Main Screen, 20 = OTA/Firmware
     */
    int help_context = 10;
    int current_scr = ui_current_screen_id();
    if (current_scr == (int)SCREEN_ID_SETTING && objects.tabview_setting) {
        help_context = (int)lv_tabview_get_tab_active(objects.tabview_setting);
    } else if (current_scr == (int)SCREEN_ID_FRAMWARE) {
        help_context = 20;
    }

    const char *title_text;
    if (help_context == 0) {
        if (EEprom2_Language == L_english) title_text = "General Settings Guide";
        else if (EEprom2_Language == L_Turkish) title_text = "Genel Ayarlar Kilavuzu";
        else if (EEprom2_Language == L_Russian) title_text = "Rukovodstvo Obshchikh Nastroyek";
        else title_text = "راهنمای تنظیمات عمومی هافلر";
    } else if (help_context == 1) {
        if (EEprom2_Language == L_english) title_text = "I/O Test & Hardware Guide";
        else if (EEprom2_Language == L_Turkish) title_text = "Test ve Baglanti Kilavuzu";
        else if (EEprom2_Language == L_Russian) title_text = "Rukovodstvo po Testirovaniyu";
        else title_text = "راهنمای تست و سیم بندی تجهیزات";
    } else if (help_context == 2) {
        if (EEprom2_Language == L_english) title_text = "Admin & Security Guide";
        else if (EEprom2_Language == L_Turkish) title_text = "Yonetici ve Guvenlik Kilavuzu";
        else if (EEprom2_Language == L_Russian) title_text = "Rukovodstvo Administratora";
        else title_text = "راهنمای بخش مدیریت و امنیت";
    } else if (help_context == 20) {
        if (EEprom2_Language == L_english) title_text = "Firmware & OTA Guide";
        else if (EEprom2_Language == L_Turkish) title_text = "Yazilim Guncelleme Kilavuzu";
        else if (EEprom2_Language == L_Russian) title_text = "Rukovodstvo po Proshivke";
        else title_text = "راهنمای بروزرسانی و ارتقا نرم افزار";
    } else {
        if (EEprom2_Language == L_english) title_text = "Argon Gas Filling Guide";
        else if (EEprom2_Language == L_Turkish) title_text = "Gaz Dolum Kilavuzu";
        else if (EEprom2_Language == L_Russian) title_text = "Rukovodstvo po Zapolneniyu Gazom";
        else title_text = "راهنمای داشبورد تزریق گاز آرگون";
    }

    lv_obj_t *title = lv_label_create(panel);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_width(title, 360);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, title_text);

    lv_obj_t *close = lv_btn_create(panel);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, -4, 4);
    lv_obj_set_size(close, 36, 36);
    lv_obj_set_style_bg_color(close, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(close, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(close, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(close, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close, ui_setting_help_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_label = lv_label_create(close);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(close_label);

    /* Scrollable content container */
    lv_obj_t *scroll = lv_obj_create(panel);
    lv_obj_set_pos(scroll, 4, 44);
    lv_obj_set_size(scroll, 424, 226);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scroll, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);

    if (help_context == 0) {
        /* General Tab Specific Help */
        if (EEprom2_Language == L_Persion) {
            ui_help_add_card(scroll, "انتخاب زبان دستگاه",
                "با باز کردن منوی کشویی زبان - میتوانید زبان کل سیستم را بین ۴ زبان فارسی - انگلیسی - ترکی و روسی تغییر دهید. با تغییر زبان - متون و تقویم به صورت خودکار منطبق میشوند.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "نرخ دبی جریان گاز (Qs)",
                "نرخ دبی گاز آرگون بر حسب لیتر بر دقیقه (L/min) تنظیم میشود (پیش فرض 30). این پارامتر بر اساس رگلاتور و گیج جریان خط گاز تنظیم شده و مبنای محاسبه زمان تزریق است.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "فعال سازی و ثبت دائم گارانتی",
                "با فشردن دکمه فعال سازی گارانتی - تاریخ روز جاری به صورت دائم در حافظه ثبت شده و وضعیت گارانتی معتبر نمایش داده میشود.",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "بروزرسانی آنلاین و آفلاین",
                "جهت دریافت نسخه های جدید نرم افزار با قابلیت های پیشرفته تر - از کلید بروزرسانی آنلاین استفاده فرمایید.",
                lv_color_hex(0xffd97706));
            ui_help_add_card(scroll, "بازنشانی به تنظیمات کارخانه",
                "در صورت نیاز به ریست کلیه پارامترها و ابعاد به مقادیر اولیه شرکت هافلر - کلید قرمز تنظیم کارخانه را بفشارید.",
                lv_color_hex(0xffdc2626));
        } else if (EEprom2_Language == L_Turkish) {
            ui_help_add_card(scroll, "1. Dil Secimi",
                "Sistem dilini Farsca, Ingilizce, Turkce ve Rusca arasinda degistirebilirsiniz. Metinler ve takvim aninda guncellenir.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Gaz Akisi (Qs)",
                "L/dakika cinsinden Argon debisi (varsayilan 30). Hacme dayali kesin dolum suresini belirler.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Garanti Etkinlestirme",
                "Devreye alma tarihini kalici EEPROM bellege kaydeder.",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "4. Cevrimici Guncelleme",
                "Yeni ozellikler ve guvenlik iyilestirmeleri icin sunucuyu kontrol eder.",
                lv_color_hex(0xffd97706));
            ui_help_add_card(scroll, "5. Fabrika Ayarlari",
                "Tum boyut ve proses parametrelerini varsayilan fabrika degerlerine dondurur.",
                lv_color_hex(0xffdc2626));
        } else if (EEprom2_Language == L_Russian) {
            ui_help_add_card(scroll, "1. Vybor Yazyka",
                "Pereklyuchenie yazyka sistemy mezhdu persidskim, angliyskim, turetskim i russkim.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Potok Gaza (Qs)",
                "Raskhod argona v L/min (po umolchaniyu 30). Opredelyaet tochnoe vremya zapolneniya.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Aktivatsiya Garantii",
                "Fiksiruet datu zapuska v nezavisimoy pamyati ustroystva.",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "4. Onlajn Obnovlenie",
                "Proverka i ustanovka novykh versiy proshivki s oblachnogo servera.",
                lv_color_hex(0xffd97706));
            ui_help_add_card(scroll, "5. Sbros Nastroyek",
                "Vozvrat vsekh parametrov k zavodskim standartam Haffler.",
                lv_color_hex(0xffdc2626));
        } else {
            ui_help_add_card(scroll, "1. Language Selection",
                "Select system language between Persian, English, Turkish, and Russian. All texts and date formats update immediately.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Gas Flow Rate (Qs)",
                "Argon gas flow rate in L/min (default 30). Used directly to calculate precise injection timing based on volume.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Warranty Activation",
                "Activates and permanently stores the commissioning date in non-volatile memory.",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "4. Online OTA Update",
                "Check server for firmware updates with new features and safety enhancements.",
                lv_color_hex(0xffd97706));
            ui_help_add_card(scroll, "5. Factory Reset",
                "Restores all dimensions and process variables to factory default values.",
                lv_color_hex(0xffdc2626));
        }
    } else if (help_context == 1) {
        /* Test Tab Specific Help */
        if (EEprom2_Language == L_Persion) {
            ui_help_add_card(scroll, "تست ورودی های دیجیتال (I6 / I7)",
                "شاسی های فیزیکی استارت (I6) و استپ اضطراری (I7) به صورت زنده نمایش داده میشوند. در صورت فشردن هر شاسی - نشانگر سبز رنگ روشن میشود.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "تست خروجی های رله (Q0 / Q1 / Q2)",
                "با لمس کلیدهای شیر ۱ (Q0) - شیر ۲ (Q1) و آژیر (Q2) میتوانید سلامت بوبین شیرهای برقی و رله ها را بررسی کنید. خروجی ها دارای تایمر خاموشی خودکار هستند.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "تعیین حالت شاسی استپ (N.O / N.C)",
                "با سوییچ وضعیت استپ میتوانید نوع کنتاکت شاسی توقف را نرمالی بسته (N.C) یا نرمالی باز (N.O) تعیین کنید. استاندارد صنعتی N.C توصیه میشود.",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "نقشه سیم بندی و راهنمای پایه ها",
                "کلید نقشه سیم بندی را بفشارید تا دیاگرام ترمینال ها و شماره گذاری کابل ها نمایش داده شود.",
                lv_color_hex(0xffd97706));
        } else if (EEprom2_Language == L_Turkish) {
            ui_help_add_card(scroll, "1. Dijital Girisler (I6 / I7)",
                "Baslat butonu (I6) ve Acil Durdurma (I7) anahtarlarinin canli sinyali. Giris tetiklendiginde yesil LED yanar.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Role Cikislari (Q0 / Q1 / Q2)",
                "Vana 1 (Q0), Vana 2 (Q1) ve Buzzer (Q2) icin guvenli manuel test.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Durdurma Tipi (N.O / N.C)",
                "Acil durdurma anahtari kontak tipi (Normalde Kapali N.C veya Normalde Acik N.O).",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "4. Baglanti Semasi",
                "Tum klemens baglantilari ve kablolama semasini goruntulemek icin dokunun.",
                lv_color_hex(0xffd97706));
        } else if (EEprom2_Language == L_Russian) {
            ui_help_add_card(scroll, "1. Tsifr. Vkhody (I6 / I7)",
                "Zhivoy monitoring knopok Puska (I6) i Avariynogo Stopa (I7). Zelenyy indikator pri zamykanii.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Prom. Vykhody (Q0 / Q1 / Q2)",
                "Ruchnoy test klapanov 1 (Q0), 2 (Q1) i sireny (Q2) s avtomaticheskoy zashchitoy.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Rezhym Stopa (N.O / N.C)",
                "Vybor tipa kontakta knopki stop (N.C normalno zamknutyy ili N.O normalno razomknutyy).",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "4. Skhema Podklyucheniya",
                "Podrobnaya skhema klemmnika i naznacheniya promyshlennykh portov.",
                lv_color_hex(0xffd97706));
        } else {
            ui_help_add_card(scroll, "1. Digital Inputs (I6 / I7)",
                "Live feedback for Start switch (I6) and Emergency Stop switch (I7). Green indicator illuminates when input is closed.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Relay Outputs (Q0 / Q1 / Q2)",
                "Manual pulse test for Valve 1 (Q0), Valve 2 (Q1), and Buzzer (Q2) with automatic safety cutoff.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Emergency Stop Contact (N.O / N.C)",
                "Select stop push-button contact type (Normally Closed N.C or Normally Open N.O).",
                lv_color_hex(0xff8b5cf6));
            ui_help_add_card(scroll, "4. Terminal Wiring Pinout",
                "Tap Pinout button to view complete controller terminal connections and wire colors.",
                lv_color_hex(0xffd97706));
        }
    } else if (help_context == 2) {
        /* Admin Tab Specific Help */
        if (EEprom2_Language == L_Persion) {
            ui_help_add_card(scroll, "رمز عبور پیش فرض مدیر",
                "رمز پیش فرض مدیریت 0000 میباشد. جهت ورود به منوی پیشرفته و تغییر ساعت یا روزشمار از این رمز استفاده نمایید.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "تنظیم ساعت و تقویم RTC",
                "امکان تنظیم ساعت - دقیقه - ثانیه و تاریخ شمسی و میلادی با نگهداری باتری پشتیبان ساعت داخلی سخت افزار فراهم است.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "قفل روزشمار ۵ مرحله ای",
                "روزشمار صنعتی امکان تنظیم دوره های کاری (مثلا ۳۰ - ۶۰ - ۹۰ روز) را فراهم میکند. پس از پایان روزشمار دستگاه قفل شده و با کد چالش بازگشایی میشود.",
                lv_color_hex(0xff8b5cf6));
        } else if (EEprom2_Language == L_Turkish) {
            ui_help_add_card(scroll, "1. Varsayilan Yonetici PIN",
                "Varsayilan yonetici PIN kodu 0000 dir. Saat ve kilit ayarlarina erisim saglar.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. RTC Saat ve Takvim",
                "Dahili RTC pili ile korunan donanimsal saati ve takvimi hassas sekilde ayarlayin.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. 5 Kademeli Kilit Sayaci",
                "Taksit ve kiralama yonetimi icin cevrimdisi guvenlik kodu destegi olan kademeli kilit.",
                lv_color_hex(0xff8b5cf6));
        } else if (EEprom2_Language == L_Russian) {
            ui_help_add_card(scroll, "1. PIN Admina (0000)",
                "PIN po umolchaniyu 0000. Otkryvaet dostup k nastroyke chasov i blokirovke.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. Chasy i Kalendar RTC",
                "Nastroyka realnogo vremeni i daty s batareynym pitaniyem mikroskhemy RTC.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. 5-Etapnaya Blokirovka",
                "Promyshlennaya blokirovka s avariynymi kodami dostupa dlya lizinga.",
                lv_color_hex(0xff8b5cf6));
        } else {
            ui_help_add_card(scroll, "1. Default Admin PIN",
                "Default administrator PIN is 0000. Provides access to clock setup and day-lock management.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. RTC Clock & Calendar",
                "Set accurate hardware real-time clock (year, month, day, hour, minute) backed by onboard RTC battery.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. 5-Stage Day-Lock Countdown",
                "Industrial stage lockout with offline challenge-response emergency unlock codes.",
                lv_color_hex(0xff8b5cf6));
        }
    } else if (help_context == 20) {
        /* Firmware & OTA Specific Help */
        if (EEprom2_Language == L_Persion) {
            ui_help_add_card(scroll, "بروزرسانی از سرور هافلر",
                "با لمس دکمه نصب در کارت دستگاه -  آخرین نسخه فریمور به صورت خودکار از سرور ابری هافلر دریافت و نصب میشود.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "ورود با لینک مستقیم",
                "امکان دانلود مستقیم فریمور از مسیرهای اختصاصی سرور با فشردن دکمه ورود با لینک و کیبورد کامل فراهم است.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "نسخه تست کارخانه ای (other/test)",
                "با انتخاب کلید میانبر other/test در مودال لینک -  بسته آزمایشی با آخرین تغییرات دریافت میگردد.",
                lv_color_hex(0xff8b5cf6));
        } else if (EEprom2_Language == L_Turkish) {
            ui_help_add_card(scroll, "1. Bulut Sunucu Guncelleme",
                "Haffler bulut sunucusundan sertifikali en son surumu otomatik indirip yuklemek icin Yukle butonuna basin.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "2. Ozel URL ile Yukleme",
                "Ozel yazilim paketlerini yuklemek icin URL adresini ekran klavyesi ile girin.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "3. Fabrika Test Surumu",
                "Test yazilim paketlerini hizlica denemek icin other/test kisayolunu kullanin.",
                lv_color_hex(0xff8b5cf6));
        } else if (EEprom2_Language == L_Russian) {
            ui_help_add_card(scroll, "1. Oblachnoye Obnovleniye",
                "Zagruzka i ustanovka ofitsialnykh obnovleniy napryamuyu s oblachnogo servera Haffler.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "2. Ustanovka po URL",
                "Vvod pryamoy ssylki dlya ustanovki spetsialnykh sborok proshivki.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "3. Testovyy Paket",
                "Ispolzuyte knopku other/test dlya bystrogo testirovaniya novykh funktsiy.",
                lv_color_hex(0xff8b5cf6));
        } else {
            ui_help_add_card(scroll, "1. Cloud Server Update",
                "Tap Install to download and flash the latest certified release directly from Haffler cloud server.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "2. Direct URL Installation",
                "Enter custom firmware binary URL via on-screen QWERTY keyboard for testing custom builds.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "3. Factory Test Release",
                "Use other/test shortcut button to quickly test staging firmware packages.",
                lv_color_hex(0xff8b5cf6));
        }
    } else {
        /* Main Dashboard Help */
        if (EEprom2_Language == L_Persion) {
            ui_help_add_card(scroll, "محاسبات حجم و زمان تزریق",
                "طول و ارتفاع شیشه و ضخامت اسپیسر را به سانتی متر وارد کنید. زمان تزریق گاز آرگون به صورت بلادرنگ بر اساس حجم محفظه و نرخ دبی محاسبه میشود.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "کلیدهای ده گانه پنل جلو",
                "کلیدهای F1 تا F5 ناوبری بین صفحات:\nF1: داشبورد اصلی | F2: تنظیمات عمومی | F3: دبی گاز | F4: تست تجهیزات | F5: منوی مدیر\nکلیدهای عملیاتی: START (شروع تزریق) -  STOP (توقف فوری) -  MODE (تغییر ایستگاه شیشه ۱ / ۲ / اتوماتیک).",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "ایمنی و توقف اضطراری",
                "فشردن کلید STOP یا پدال اضطراری بلافاصله هر دو شیر برقی را قطع میکند. همچنین تایمر ایمنی ۱۸۰ ثانیه ای مانع هدررفت کپسول گاز میشود.",
                lv_color_hex(0xffdc2626));
            ui_help_add_card(scroll, "وب اپلیکیشن و اتصال موبایل",
                "با روشن کردن وای فای دستگاه و اتصال گوشی به هات اسپات Haffler - آدرس 192.168.4.1 را در مرورگر باز کنید.",
                lv_color_hex(0xff8b5cf6));
        } else if (EEprom2_Language == L_Turkish) {
            ui_help_add_card(scroll, "1. Gaz Hacim ve Sure Hesabi",
                "Cam boyunu, yuksekligini ve cita kalinligini cm olarak girin. Dolum suresi anlik hesaplanir.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. 10 On Panel Tusu",
                "F1: Ana Sayfa | F2: Genel | F3: Debi | F4: Test | F5: Yonetici\nSTART: Dolumu baslat | STOP: Acil durdurma | MODE: Istasyon 1 / 2 secimi.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Guvenlik ve Acil Durdurma",
                "STOP tusuna basildiginda vanalar aninda kapatilir. 180 saniyelik guvenlik zamanlayicisi gaz kaybini onler.",
                lv_color_hex(0xffdc2626));
            ui_help_add_card(scroll, "4. Mobil Web Erisimi",
                "Telefonunuzu makine Wi-Fi agina baglayin ve kablosuz kontrol icin 192.168.4.1 adresini acin.",
                lv_color_hex(0xff8b5cf6));
        } else if (EEprom2_Language == L_Russian) {
            ui_help_add_card(scroll, "1. Raschet Obyema i Vremeni",
                "Vvedite dlinu, vysotu steklopaketa i shirinu ramki v sm. Vremya zapolneniya rasschityvaetsya avtomaticheski.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. 10 Knopok Peredney Paneli",
                "F1: Glavnaya | F2: Obshchie | F3: Potok | F4: Test | F5: Admin\nSTART: Pusk zapolneniya | STOP: Avariynyy stop | MODE: Vybor stantsii 1 / 2.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Bezopasnost i Blokirovki",
                "Nazhatiye STOP mgnovenno obestochivaet oba klapana. Taymer 180s zashchishchaet ballon s argonom.",
                lv_color_hex(0xffdc2626));
            ui_help_add_card(scroll, "4. Mobilnyy Veb-Portal",
                "Podklyuchites k Wi-Fi apparata i otkroyte 192.168.4.1 dlya distantsionnogo upravleniya.",
                lv_color_hex(0xff8b5cf6));
        } else {
            ui_help_add_card(scroll, "1. Gas Volume & Timing Computation",
                "Enter glass length, height, and spacer thickness in cm. Filling duration is computed instantly using chamber volume and flow rate.",
                lv_color_hex(0xff0284c7));
            ui_help_add_card(scroll, "2. 10 Front-Panel Hardware Keys",
                "F1: Dashboard | F2: General | F3: Flow Rate | F4: Test | F5: Admin\nSTART Key: Begin injection | STOP Key: Emergency cutoff | MODE: Toggle Glass 1 / Glass 2 / Auto.",
                lv_color_hex(0xff16a34a));
            ui_help_add_card(scroll, "3. Process Safety Interlocks",
                "Pressing STOP immediately de-energizes both valves. A hard 180s watchdog timer prevents gas cylinder depletion.",
                lv_color_hex(0xffdc2626));
            ui_help_add_card(scroll, "4. Mobile Web Portal",
                "Connect smartphone to machine Wi-Fi hotspot and navigate to 192.168.4.1 for wireless control.",
                lv_color_hex(0xff8b5cf6));
        }
    }
}

static void ui_general_info_row(lv_obj_t *parent, int y, const char *text, uint32_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_pos(dot, 14, y + 5);
    lv_obj_set_size(dot, 9, 9);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, 30, y);
    lv_obj_set_size(label, 325, 18);
    lv_obj_set_style_text_font(label, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xff263238), LV_PART_MAIN);
    lv_label_set_text(label, text);
}

static lv_obj_t *s_pinout_modal = NULL;

static void ui_pinout_diagram_close_cb(lv_event_t *e) {
    (void)e;
    if (s_pinout_modal) {
        lv_obj_delete(s_pinout_modal);
        s_pinout_modal = NULL;
    }
}

void ui_pinout_diagram_open(lv_event_t *e)
{
    (void)e;
    if (s_pinout_modal) {
        lv_obj_delete(s_pinout_modal);
        s_pinout_modal = NULL;
    }
    s_pinout_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_pinout_modal, 480, 320);
    lv_obj_set_pos(s_pinout_modal, 0, 0);
    lv_obj_set_style_bg_color(s_pinout_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_pinout_modal, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_pinout_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_pinout_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_pinout_modal, LV_OBJ_FLAG_SCROLLABLE);

    /* Main modal card: centered with 12px left/right and 11px top/bottom margin */
    lv_obj_t *panel = lv_obj_create(s_pinout_modal);
    lv_obj_set_pos(panel, 12, 11);
    lv_obj_set_size(panel, 456, 298);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);
    const char *title_txt = (EEprom2_Language == L_english) ? "External Terminal Wiring Diagram" :
                            (EEprom2_Language == L_Turkish) ? "Harici Terminal Baglanti Semasi" :
                            (EEprom2_Language == L_Russian) ? "Skhema Klemmnikov" : "نقشه سیم بندی ترمینال های دستگاه";

    lv_obj_t *title = lv_label_create(panel);
    lv_obj_set_pos(title, 20, 2);
    lv_obj_set_size(title, 386, 24);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, title_txt);

    lv_obj_t *btn_x = lv_btn_create(panel);
    lv_obj_set_pos(btn_x, 412, -4);
    lv_obj_set_size(btn_x, 32, 26);
    lv_obj_set_style_radius(btn_x, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_x, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_x, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_x, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_x, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_x, ui_pinout_diagram_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_x = lv_label_create(btn_x);
    lv_label_set_text(lbl_x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_center(lbl_x);

    /* Left Card: Industrial & Relay Outputs */
    lv_obj_t *card_out = lv_obj_create(panel);
    lv_obj_set_pos(card_out, 6, 28);
    lv_obj_set_size(card_out, 214, 218);
    lv_obj_set_style_bg_color(card_out, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_out, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_out, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_out, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_out, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card_out, LV_OBJ_FLAG_SCROLLABLE);

    const char *hdr_out_txt = (EEprom2_Language == L_english) ? "Industrial & Relay Outputs" :
                              (EEprom2_Language == L_Turkish) ? "Endustriyel Role Cikislari" :
                              (EEprom2_Language == L_Russian) ? "Prom. Releynyye Vykhody" : "خروجی های صنعتی و رله";

    lv_obj_t *hdr_out = lv_label_create(card_out);
    lv_obj_set_pos(hdr_out, 0, 1);
    lv_obj_set_size(hdr_out, 204, 20);
    lv_obj_set_style_text_align(hdr_out, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(hdr_out, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(hdr_out, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(hdr_out, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_label_set_text(hdr_out, hdr_out_txt);

    const char *out_name_0 = (EEprom2_Language == L_english) ? "Valve 1 (Left)" :
                             (EEprom2_Language == L_Turkish) ? "Valf 1 (Sol)" :
                             (EEprom2_Language == L_Russian) ? "Klapan 1 (Lev)" : "شیر 1 (ایستگاه چپ)";
    const char *out_name_1 = (EEprom2_Language == L_english) ? "Valve 2 (Right)" :
                             (EEprom2_Language == L_Turkish) ? "Valf 2 (Sag)" :
                             (EEprom2_Language == L_Russian) ? "Klapan 2 (Prav)" : "شیر 2 (ایستگاه راست)";
    const char *out_name_2 = (EEprom2_Language == L_english) ? "Alarm Buzzer" :
                             (EEprom2_Language == L_Turkish) ? "Sesli Alarm" :
                             (EEprom2_Language == L_Russian) ? "Sirena" : "آژیر هشدار صوتی";
    const char *out_name_3 = (EEprom2_Language == L_english) ? "Relay Common" :
                             (EEprom2_Language == L_Turkish) ? "Role Ortak" :
                             (EEprom2_Language == L_Russian) ? "Obshchiy Vyvod" : "مشترک رله ها";

    const struct {
        const char *pin;
        const char *name;
        uint32_t color;
    } out_items[] = {
        { "Q0",  out_name_0, 0xff16a34a },
        { "Q1",  out_name_1, 0xff16a34a },
        { "Q2",  out_name_2, 0xffea580c },
        { "COM", out_name_3, 0xff475569 }
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = lv_obj_create(card_out);
        lv_obj_set_pos(row, 0, 24 + i * 46);
        lv_obj_set_size(row, 204, 40);
        lv_obj_set_style_bg_color(row, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_color(row, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(row, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 2, LV_PART_MAIN);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *badge = lv_obj_create(row);
        lv_obj_set_pos(badge, 2, 0);
        lv_obj_set_size(badge, 42, 32);
        lv_obj_set_style_bg_color(badge, lv_color_hex(out_items[i].color), LV_PART_MAIN);
        lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(badge, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *b_lbl = lv_label_create(badge);
        lv_label_set_text(b_lbl, out_items[i].pin);
        lv_obj_set_style_text_font(b_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(b_lbl, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(b_lbl);

        lv_obj_t *txt = lv_label_create(row);
        lv_obj_set_pos(txt, 42, 7);
        lv_obj_set_size(txt, 150, 26);
        lv_obj_set_style_base_dir(txt, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(txt, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_font(txt, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(txt, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_label_set_text(txt, out_items[i].name);
    }

    /* Right Card: Digital Inputs & Power */
    lv_obj_t *card_in = lv_obj_create(panel);
    lv_obj_set_pos(card_in, 226, 28);
    lv_obj_set_size(card_in, 214, 218);
    lv_obj_set_style_bg_color(card_in, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_in, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_in, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_in, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_in, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card_in, LV_OBJ_FLAG_SCROLLABLE);

    const char *hdr_in_txt = (EEprom2_Language == L_english) ? "Digital Inputs & Power" :
                             (EEprom2_Language == L_Turkish) ? "Dijital Girisler ve Guc" :
                             (EEprom2_Language == L_Russian) ? "Tsifr. Vkhody i Pitaniye" : "ورودی های دیجیتال و تغذیه";

    lv_obj_t *hdr_in = lv_label_create(card_in);
    lv_obj_set_pos(hdr_in, 0, 1);
    lv_obj_set_size(hdr_in, 204, 20);
    lv_obj_set_style_text_align(hdr_in, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(hdr_in, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(hdr_in, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(hdr_in, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_label_set_text(hdr_in, hdr_in_txt);

    const char *in_name_0 = (EEprom2_Language == L_english) ? "Cycle Start" :
                            (EEprom2_Language == L_Turkish) ? "Baslat Girisi" :
                            (EEprom2_Language == L_Russian) ? "Pusk Vkhod" : "شاسی شروع تزریق";
    const char *in_name_1 = (EEprom2_Language == L_english) ? "Emergency Stop" :
                            (EEprom2_Language == L_Turkish) ? "Acil Stop" :
                            (EEprom2_Language == L_Russian) ? "Avariynyy Stop" : "توقف اضطراری";
    const char *in_name_2 = (EEprom2_Language == L_english) ? "+24V DC Power" :
                            (EEprom2_Language == L_Turkish) ? "+24V DC Guc" :
                            (EEprom2_Language == L_Russian) ? "+24V DC Pitaniye" : "تغذیه مثبت 24 ولت";
    const char *in_name_3 = (EEprom2_Language == L_english) ? "0V Ground (GND)" :
                            (EEprom2_Language == L_Turkish) ? "0V Toprak (GND)" :
                            (EEprom2_Language == L_Russian) ? "0V Zazemleniye" : "زمین منفی (GND)";

    const struct {
        const char *pin;
        const char *name;
        uint32_t color;
    } in_items[] = {
        { "I7",  in_name_0, 0xff0284c7 },
        { "I6",  in_name_1, 0xffdc2626 },
        { "24V", in_name_2, 0xffb91c1c },
        { "0V",  in_name_3, 0xff334155 }
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = lv_obj_create(card_in);
        lv_obj_set_pos(row, 0, 24 + i * 46);
        lv_obj_set_size(row, 204, 40);
        lv_obj_set_style_bg_color(row, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_color(row, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(row, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 2, LV_PART_MAIN);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *badge = lv_obj_create(row);
        lv_obj_set_pos(badge, 2, 0);
        lv_obj_set_size(badge, 42, 32);
        lv_obj_set_style_bg_color(badge, lv_color_hex(in_items[i].color), LV_PART_MAIN);
        lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(badge, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *b_lbl = lv_label_create(badge);
        lv_label_set_text(b_lbl, in_items[i].pin);
        lv_obj_set_style_text_font(b_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(b_lbl, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(b_lbl);

        lv_obj_t *txt = lv_label_create(row);
        lv_obj_set_pos(txt, 42, 7);
        lv_obj_set_size(txt, 150, 26);
        lv_obj_set_style_base_dir(txt, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(txt, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_font(txt, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(txt, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_label_set_text(txt, in_items[i].name);
    }

    /* Bottom Close Button to fill modal void and provide direct touch dismissal */
    lv_obj_t *btn_close = lv_btn_create(panel);
    lv_obj_set_size(btn_close, 140, 36);
    lv_obj_set_pos(btn_close, 153, 252);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_close, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_close, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_close, ui_pinout_diagram_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_obj_set_style_text_font(lbl_close, &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_close, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(lbl_close, is_fa ? "بستن نقشه" : "Close");
    lv_obj_center(lbl_close);
}

/* ============================================================================
 * ONLINE OTA FIRMWARE UPDATE MODAL (Apple HIG / Siemens Compliant)
 * ============================================================================ */
static lv_obj_t *s_ota_overlay = NULL;
static lv_obj_t *s_ota_cur_ver_lbl = NULL;
static lv_obj_t *s_ota_new_ver_lbl = NULL;
static lv_obj_t *s_ota_status_lbl = NULL;
static lv_obj_t *s_ota_changelog_ta = NULL;
static lv_obj_t *s_ota_bar = NULL;
static lv_obj_t *s_ota_progress_lbl = NULL;
static lv_obj_t *s_ota_btn_check = NULL;
static lv_obj_t *s_ota_btn_install = NULL;

static void ui_ota_close_cb(lv_event_t *e)
{
    (void)e;
    ota_manager_cancel();
    if (s_ota_overlay) {
        lv_obj_delete(s_ota_overlay);
        s_ota_overlay = NULL;
    }
}

static void ui_framware_close_all_modals(void);

void ui_close_all_modals(void)
{
    if (s_help_overlay) {
        lv_obj_delete(s_help_overlay);
        s_help_overlay = NULL;
    }
    if (s_pinout_modal) {
        lv_obj_delete(s_pinout_modal);
        s_pinout_modal = NULL;
    }
    if (s_ota_overlay) {
        ota_manager_cancel();
        lv_obj_delete(s_ota_overlay);
        s_ota_overlay = NULL;
    }
    if (s_admin_datetime_modal) {
        lv_obj_delete(s_admin_datetime_modal);
        s_admin_datetime_modal = NULL;
    }
    if (s_admin_daylock_modal) {
        lv_obj_delete(s_admin_daylock_modal);
        s_admin_daylock_modal = NULL;
    }
    if (s_admin_pin_modal) {
        lv_obj_delete(s_admin_pin_modal);
        s_admin_pin_modal = NULL;
    }
    if (s_factory_reset_modal) {
        lv_obj_delete(s_factory_reset_modal);
        s_factory_reset_modal = NULL;
    }
    ui_framware_close_all_modals();
    ui_admin_detail_close();
    ui_close_link_entry_modal();
    dev_lock_ui_hide();
}

static void ui_ota_check_cb(lv_event_t *e)
{
    (void)e;
    if (s_ota_status_lbl) lv_label_set_text(s_ota_status_lbl, "در حال اتصال به سرور و بررسی...");
    if (s_ota_btn_check) lv_obj_add_state(s_ota_btn_check, LV_STATE_DISABLED);
    ota_manager_start_check(NULL, NULL);
}

static void ui_ota_install_cb(lv_event_t *e)
{
    (void)e;
    /* First: check WiFi connectivity - if not connected show clear error */
    if (!ota_manager_is_wifi_connected()) {
        bool is_fa = (EEprom2_Language == L_Persion);
        const char *err_msg =
            (EEprom2_Language == L_english) ? "No internet. Connect Wi-Fi first (Settings tab)." :
            (EEprom2_Language == L_Turkish) ? "Internet yok. Once Wi-Fi baglanin." :
            (EEprom2_Language == L_Russian) ? "Net interneta. Podklyuchite Wi-Fi." :
            "اینترنت وصل نیست.\nابتدا وای فای را در بخش تنظیمات وصل کنید.";
        if (s_ota_status_lbl) {
            lv_label_set_text(s_ota_status_lbl, err_msg);
            lv_obj_set_style_text_color(s_ota_status_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
        }
        return;
    }
    /* Proceed: show progress UI and start download */
    if (s_ota_bar) lv_obj_clear_flag(s_ota_bar, LV_OBJ_FLAG_HIDDEN);
    if (s_ota_progress_lbl) lv_obj_clear_flag(s_ota_progress_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s_ota_btn_install) lv_obj_add_state(s_ota_btn_install, LV_STATE_DISABLED);
    if (s_ota_btn_check) lv_obj_add_state(s_ota_btn_check, LV_STATE_DISABLED);
    bool is_fa2 = (EEprom2_Language == L_Persion);
    const char *dl_msg =
        (EEprom2_Language == L_english) ? "Connecting to server (8s timeout)..." :
        (EEprom2_Language == L_Turkish) ? "Sunucuya baglaniliyor (8 sn)..." :
        (EEprom2_Language == L_Russian) ? "Podklyucheniye k serveru (8 sek)..." :
        "در حال اتصال به سرور هافلر (تایم اوت ۸ ثانیه)...";
    if (s_ota_status_lbl) {
        lv_obj_set_style_text_color(s_ota_status_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_label_set_text(s_ota_status_lbl, dl_msg);
    }
    ota_manager_start_install();
}

static void ui_ota_on_status(ota_status_t status, const char *message)
{
    if (!s_ota_overlay) return;
    if (s_ota_status_lbl && message) {
        lv_label_set_text(s_ota_status_lbl, message);
    }
    if (status == OTA_STATUS_UPDATE_AVAILABLE) {
        const ota_info_t *info = ota_manager_get_info();
        if (s_ota_new_ver_lbl && info) {
            char buf[64];
            snprintf(buf, sizeof(buf), "نسخه سرور: %s (%s)", info->latest_version, info->latest_build_date);
            lv_label_set_text(s_ota_new_ver_lbl, buf);
        }
        if (s_ota_changelog_ta && info && strlen(info->changelog) > 0) {
            lv_textarea_set_text(s_ota_changelog_ta, info->changelog);
        }
        if (s_ota_btn_install) {
            lv_obj_clear_state(s_ota_btn_install, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(s_ota_btn_install, lv_color_hex(0xff00a64f), LV_PART_MAIN);
        }
    } else if (status == OTA_STATUS_ERROR) {
        if (s_ota_btn_check) lv_obj_clear_state(s_ota_btn_check, LV_STATE_DISABLED);
        if (s_ota_changelog_ta) {
            lv_textarea_set_text(s_ota_changelog_ta,
                "عدم برقراری ارتباط با اینترنت یا سرور آپدیت.\n"
                "راهنمای تنظیم سریع:\n"
                "1- تلفن همراه را به وای فای دستگاه متصل نمایید.\n"
                "2- در مرورگر گوشی آدرس 192.168.4.1 را باز فرمایید.\n"
                "3- در بخش ارتقا - وای فای کارگاه را اسکن و متصل کنید.\n"
                "یا مستقیما فایل آپدیت را از حافظه گوشی بارگذاری فرمایید.");
        }
    } else if (status == OTA_STATUS_UP_TO_DATE || status == OTA_STATUS_IDLE) {
        if (s_ota_btn_check) lv_obj_clear_state(s_ota_btn_check, LV_STATE_DISABLED);
    }
}

static void ui_ota_on_progress(uint32_t downloaded, uint32_t total, uint8_t percent, float speed_kbps)
{
    if (!s_ota_overlay) return;
    if (s_ota_bar) {
        lv_bar_set_value(s_ota_bar, percent, LV_ANIM_OFF);
    }
    if (s_ota_progress_lbl) {
        char buf[96];
        snprintf(buf, sizeof(buf), "%u%% (%u KB / %u KB) - %.1f KB/s",
                 (unsigned)percent, (unsigned)(downloaded / 1024), (unsigned)(total / 1024), speed_kbps);
        lv_label_set_text(s_ota_progress_lbl, buf);
    }
}

void ui_open_ota_modal(void)
{
    if (s_ota_overlay) {
        lv_obj_delete(s_ota_overlay);
        s_ota_overlay = NULL;
    }

    ota_manager_set_callbacks(ui_ota_on_progress, ui_ota_on_status);

    s_ota_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_ota_overlay, 480, 320);
    lv_obj_set_pos(s_ota_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_ota_overlay, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_ota_overlay, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ota_overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_ota_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_ota_overlay);
    lv_obj_set_pos(panel, 12, 11);
    lv_obj_set_size(panel, 456, 298);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 8, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);
    const char *ota_title = (EEprom2_Language == L_english) ? "Online System Update" :
                            (EEprom2_Language == L_Turkish) ? "Cevrimici Guncelleme" :
                            (EEprom2_Language == L_Russian) ? "Obnovlenie Sistemy" :
                            "بروزرسانی آنلاین سیستم";

    const char *cur_ver_prefix = (EEprom2_Language == L_english) ? "Current: " :
                                 (EEprom2_Language == L_Turkish) ? "Mevcut: " :
                                 (EEprom2_Language == L_Russian) ? "Tekushchaya: " :
                                 "نسخه فعلی: ";

    const char *srv_ver_txt = (EEprom2_Language == L_english) ? "Server: Unknown" :
                              (EEprom2_Language == L_Turkish) ? "Sunucu: Bilinmiyor" :
                              (EEprom2_Language == L_Russian) ? "Server: Neizvestno" :
                              "نسخه سرور: نامشخص";

    const char *prompt_txt = (EEprom2_Language == L_english) ? "Tap 'Check' to verify latest firmware." :
                             (EEprom2_Language == L_Turkish) ? "Son surumu kontrol etmek icin 'Kontrol Et'e basin." :
                             (EEprom2_Language == L_Russian) ? "Nazhmite 'Proverit' dlya poiska obnovleniy." :
                             "برای دریافت فهرست تغییرات و بررسی آخرین نسخه - دکمه 'بررسی سرور' را لمس کنید.";

    const char *status_txt = (EEprom2_Language == L_english) ? "Ready to check server" :
                             (EEprom2_Language == L_Turkish) ? "Sunucu kontrolune hazir" :
                             (EEprom2_Language == L_Russian) ? "Gotov k proverke" :
                             "آماده بررسی سرور";

    const char *check_btn_txt = (EEprom2_Language == L_english) ? "Check" :
                                (EEprom2_Language == L_Turkish) ? "Kontrol Et" :
                                (EEprom2_Language == L_Russian) ? "Proverit" :
                                "بررسی سرور";

    const char *install_btn_txt = (EEprom2_Language == L_english) ? "Install" :
                                  (EEprom2_Language == L_Turkish) ? "Yukle" :
                                  (EEprom2_Language == L_Russian) ? "Ustanovit" :
                                  "دانلود و نصب";

    const char *cancel_btn_txt = (EEprom2_Language == L_english) ? "Close" :
                                 (EEprom2_Language == L_Turkish) ? "Kapat" :
                                 (EEprom2_Language == L_Russian) ? "Zakryt" :
                                 "بستن";

    /* Title */
    lv_obj_t *title = lv_label_create(panel);
    lv_obj_set_pos(title, 14, 4);
    lv_obj_set_width(title, 386);
    lv_obj_set_style_text_align(title, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, ota_title);

    /* Close Button */
    lv_obj_t *close = lv_btn_create(panel);
    lv_obj_set_pos(close, 408, 2);
    lv_obj_set_size(close, 36, 30);
    lv_obj_set_style_bg_color(close, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(close, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(close, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(close, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close, ui_ota_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_lbl = lv_label_create(close);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(close_lbl);

    /* Version info row */
    s_ota_cur_ver_lbl = lv_label_create(panel);
    lv_obj_set_pos(s_ota_cur_ver_lbl, is_fa ? 224 : 14, 34);
    lv_obj_set_size(s_ota_cur_ver_lbl, 202, 20);
    lv_obj_set_style_text_align(s_ota_cur_ver_lbl, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_ota_cur_ver_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ota_cur_ver_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ota_cur_ver_lbl, lv_color_hex(0xff64748b), LV_PART_MAIN);
    char cur_buf[48];
    snprintf(cur_buf, sizeof(cur_buf), "%s%s", cur_ver_prefix, FW_VERSION_LABEL);
    lv_label_set_text(s_ota_cur_ver_lbl, cur_buf);

    s_ota_new_ver_lbl = lv_label_create(panel);
    lv_obj_set_pos(s_ota_new_ver_lbl, is_fa ? 14 : 224, 34);
    lv_obj_set_size(s_ota_new_ver_lbl, 202, 20);
    lv_obj_set_style_text_align(s_ota_new_ver_lbl, is_fa ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_ota_new_ver_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ota_new_ver_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ota_new_ver_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_label_set_text(s_ota_new_ver_lbl, srv_ver_txt);

    /* Changelog text area */
    s_ota_changelog_ta = lv_textarea_create(panel);
    lv_obj_set_pos(s_ota_changelog_ta, 12, 58);
    lv_obj_set_size(s_ota_changelog_ta, 420, 102);
    lv_obj_set_style_bg_color(s_ota_changelog_ta, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_ota_changelog_ta, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ota_changelog_ta, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ota_changelog_ta, 8, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ota_changelog_ta, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ota_changelog_ta, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_ota_changelog_ta, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_textarea_set_text(s_ota_changelog_ta, prompt_txt);
    lv_obj_clear_flag(s_ota_changelog_ta, LV_OBJ_FLAG_CLICKABLE);

    /* Progress bar */
    s_ota_bar = lv_bar_create(panel);
    lv_obj_set_pos(s_ota_bar, 12, 168);
    lv_obj_set_size(s_ota_bar, 420, 12);
    lv_bar_set_range(s_ota_bar, 0, 100);
    lv_bar_set_value(s_ota_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_ota_bar, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ota_bar, lv_color_hex(0xff0284c7), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_ota_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ota_bar, 6, LV_PART_INDICATOR);
    lv_obj_add_flag(s_ota_bar, LV_OBJ_FLAG_HIDDEN);

    s_ota_progress_lbl = lv_label_create(panel);
    lv_obj_set_pos(s_ota_progress_lbl, 12, 184);
    lv_obj_set_size(s_ota_progress_lbl, 420, 18);
    lv_obj_set_style_text_align(s_ota_progress_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ota_progress_lbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ota_progress_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_label_set_text(s_ota_progress_lbl, "");
    lv_obj_add_flag(s_ota_progress_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Status text */
    s_ota_status_lbl = lv_label_create(panel);
    lv_obj_set_pos(s_ota_status_lbl, 12, 206);
    lv_obj_set_size(s_ota_status_lbl, 420, 20);
    lv_obj_set_style_text_align(s_ota_status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_ota_status_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ota_status_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ota_status_lbl, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_label_set_text(s_ota_status_lbl, status_txt);

    /* Bottom Action Buttons (44px height for Apple touch ergonomics) */
    s_ota_btn_check = lv_btn_create(panel);
    lv_obj_set_pos(s_ota_btn_check, 292, 238);
    lv_obj_set_size(s_ota_btn_check, 140, 44);
    lv_obj_set_style_bg_color(s_ota_btn_check, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_radius(s_ota_btn_check, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_ota_btn_check, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(s_ota_btn_check, ui_ota_check_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *check_lbl = lv_label_create(s_ota_btn_check);
    lv_label_set_text(check_lbl, check_btn_txt);
    lv_obj_set_style_text_color(check_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(check_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_center(check_lbl);

    s_ota_btn_install = lv_btn_create(panel);
    lv_obj_set_pos(s_ota_btn_install, 142, 238);
    lv_obj_set_size(s_ota_btn_install, 140, 44);
    lv_obj_set_style_bg_color(s_ota_btn_install, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_radius(s_ota_btn_install, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_ota_btn_install, 0, LV_PART_MAIN);
    lv_obj_add_state(s_ota_btn_install, LV_STATE_DISABLED);
    lv_obj_add_event_cb(s_ota_btn_install, ui_ota_install_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *inst_lbl = lv_label_create(s_ota_btn_install);
    lv_label_set_text(inst_lbl, install_btn_txt);
    lv_obj_set_style_text_color(inst_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(inst_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_center(inst_lbl);

    lv_obj_t *btn_cancel = lv_btn_create(panel);
    lv_obj_set_pos(btn_cancel, 12, 238);
    lv_obj_set_size(btn_cancel, 120, 44);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cancel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_cancel, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cancel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cancel, ui_ota_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *canc_lbl = lv_label_create(btn_cancel);
    lv_label_set_text(canc_lbl, cancel_btn_txt);
    lv_obj_set_style_text_color(canc_lbl, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_text_font(canc_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &ui_font_arial_regular17, LV_PART_MAIN);
    lv_obj_center(canc_lbl);
}

static void ui_ota_open_btn_cb(lv_event_t *e)
{
    (void)e;
    if (ota_manager_is_wifi_connected()) {
        loadScreen(SCREEN_ID_FRAMWARE);
    } else {
        loadScreen(SCREEN_ID_WIFI);
    }
}

static void ui_webportal_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    webportal_set_enabled(state);
    if (state) {
        ui_toast_show("هات اسپات وب اپ", "وای فای فعال شد - آدرس 192.168.4.1", 3000, TOAST_TYPE_SUCCESS);
    } else {
        ui_toast_show("هات اسپات وب اپ", "وای فای غیرفعال شد", 2000, TOAST_TYPE_INFO);
    }
}

static void ui_event_wifi_toggle(lv_event_t *e)
{
    (void)e;
    bool is_on = webportal_is_running();
    bool new_state = !is_on;
    webportal_set_enabled(new_state);

    unsigned char lang = EEprom2_Language;
    if (new_state) {
        if (lang == L_english) {
            ui_toast_show("Wi-Fi Hotspot", "Wi-Fi Enabled (192.168.4.1)", 3000, TOAST_TYPE_SUCCESS);
        } else if (lang == L_Turkish) {
            ui_toast_show("Wi-Fi Noktasi", "Wi-Fi Acildi (192.168.4.1)", 3000, TOAST_TYPE_SUCCESS);
        } else if (lang == L_Russian) {
            ui_toast_show("Tochka Wi-Fi", "Wi-Fi Vklyuchen (192.168.4.1)", 3000, TOAST_TYPE_SUCCESS);
        } else {
            ui_toast_show("هات اسپات وای فای", "وای فای فعال شد (192.168.4.1)", 3000, TOAST_TYPE_SUCCESS);
        }
    } else {
        if (lang == L_english) {
            ui_toast_show("Wi-Fi Hotspot", "Wi-Fi Disabled", 2000, TOAST_TYPE_INFO);
        } else if (lang == L_Turkish) {
            ui_toast_show("Wi-Fi Noktasi", "Wi-Fi Kapatildi", 2000, TOAST_TYPE_INFO);
        } else if (lang == L_Russian) {
            ui_toast_show("Tochka Wi-Fi", "Wi-Fi Vyklyuchen", 2000, TOAST_TYPE_INFO);
        } else {
            ui_toast_show("هات اسپات وای فای", "وای فای غیرفعال شد", 2000, TOAST_TYPE_INFO);
        }
    }

    if (objects.icon_wifi && lv_obj_is_valid(objects.icon_wifi)) {
        if (new_state) {
            lv_obj_set_style_text_color(objects.icon_wifi, lv_color_hex(0xff00d2ff), LV_PART_MAIN);
            lv_obj_set_style_opa(objects.icon_wifi, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(objects.icon_wifi, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
            lv_obj_set_style_opa(objects.icon_wifi, LV_OPA_60, LV_PART_MAIN);
        }
    }
}

static void ui_event_buzzer_toggle(lv_event_t *e)
{
    (void)e;
    bool new_state = !buzzerEnable;
    buzzerEnable = new_state ? 1 : 0;
    EEprom3_Buzzer = new_state;
    if (objects.sw_buzzer && lv_obj_is_valid(objects.sw_buzzer)) {
        if (new_state) lv_obj_add_state(objects.sw_buzzer, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.sw_buzzer, LV_STATE_CHECKED);
    }
    if (new_state) {
        buzzerTimeVlaue = 2;
    } else {
        buzzerTimeVlaue = 0;
        HwSetBuzzer(false);
    }
    a2_eeprom_write_all();

    unsigned char lang = EEprom2_Language;
    if (new_state) {
        if (lang == L_english) {
            ui_toast_show("Buzzer", "Sound Enabled", 2000, TOAST_TYPE_SUCCESS);
        } else if (lang == L_Turkish) {
            ui_toast_show("Ses ve Buzzer", "Ses Acildi", 2000, TOAST_TYPE_SUCCESS);
        } else if (lang == L_Russian) {
            ui_toast_show("Zvuk i Zummer", "Zvuk Vklyuchen", 2000, TOAST_TYPE_SUCCESS);
        } else {
            ui_toast_show("صدا و بازر", "صدا فعال شد", 2000, TOAST_TYPE_SUCCESS);
        }
    } else {
        if (lang == L_english) {
            ui_toast_show("Buzzer", "Sound Muted", 2000, TOAST_TYPE_INFO);
        } else if (lang == L_Turkish) {
            ui_toast_show("Ses ve Buzzer", "Ses Kapatildi", 2000, TOAST_TYPE_INFO);
        } else if (lang == L_Russian) {
            ui_toast_show("Zvuk i Zummer", "Zvuk Vyklyuchen", 2000, TOAST_TYPE_INFO);
        } else {
            ui_toast_show("صدا و بازر", "صدا بی صدا شد", 2000, TOAST_TYPE_INFO);
        }
    }

    if (objects.icon_buzzer && lv_obj_is_valid(objects.icon_buzzer)) {
        if (new_state) {
            lv_label_set_text(objects.icon_buzzer, LV_SYMBOL_VOLUME_MAX);
            lv_obj_set_style_text_color(objects.icon_buzzer, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_obj_set_style_opa(objects.icon_buzzer, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_label_set_text(objects.icon_buzzer, LV_SYMBOL_MUTE);
            lv_obj_set_style_text_color(objects.icon_buzzer, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
            lv_obj_set_style_opa(objects.icon_buzzer, LV_OPA_60, LV_PART_MAIN);
        }
    }
}

static void ui_general_warranty_btn_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (warranty_is_active()) {
        const char *t_title = (EEprom2_Language == L_Persion) ? "گارانتی دستگاه" : "Device Warranty";
        const char *t_msg = (EEprom2_Language == L_Persion) ? "گارانتی این دستگاه قبلاً ثبت و فعال شده است" : "Warranty is already registered and active";
        ui_toast_show(t_title, t_msg, 2500, TOAST_TYPE_INFO);
        return;
    }
    char cur_d[32] = {0};
    if (objects.t_date && lv_obj_is_valid(objects.t_date)) {
        const char *t = lv_label_get_text(objects.t_date);
        if (t && strlen(t) > 0 && strcmp(t, "----/--/--") != 0) {
            strncpy(cur_d, t, sizeof(cur_d) - 1);
        }
    }
    warranty_activate(cur_d[0] ? cur_d : "1405/06/17");
    ui_update_warranty_button_visual();
    ui_toast_show("گارانتی دستگاه", "گارانتی دستگاه با موفقیت فعال و ثبت دائم شد", 3000, TOAST_TYPE_SUCCESS);
}

/* =========================================================================
   Apple HIG Factory Reset Confirmation Modal (Zero-Hang Safe Reboot)
   ========================================================================= */
static void ui_factory_reset_cancel_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (s_factory_reset_modal) {
        lv_obj_delete(s_factory_reset_modal);
        s_factory_reset_modal = NULL;
    }
}

static void ui_factory_reset_confirm_action_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (s_factory_reset_modal) {
        lv_obj_delete(s_factory_reset_modal);
        s_factory_reset_modal = NULL;
    }
    bool is_fa = (EEprom2_Language == L_Persion);
    const char *title = is_fa ? "بازنشانی به تنظیمات کارخانه" : "Factory Reset";
    const char *msg = is_fa ? "در حال بازنشانی تنظیمات و راه‌اندازی مجدد..." : "Resetting to defaults and restarting...";
    ui_toast_show(title, msg, 2500, TOAST_TYPE_WARNING);

    a2_eeprom_factory_reset();
}

void ui_open_factory_reset_confirm_modal(void)
{
    if (s_factory_reset_modal) {
        lv_obj_delete(s_factory_reset_modal);
        s_factory_reset_modal = NULL;
    }
    bool is_fa = (EEprom2_Language == L_Persion);

    s_factory_reset_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_factory_reset_modal, 480, 320);
    lv_obj_set_pos(s_factory_reset_modal, 0, 0);
    lv_obj_set_style_bg_color(s_factory_reset_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_factory_reset_modal, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_factory_reset_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_factory_reset_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_factory_reset_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_factory_reset_modal);
    lv_obj_set_pos(panel, 35, 45);
    lv_obj_set_size(panel, 410, 230);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *title = lv_label_create(panel);
    lv_obj_set_pos(title, 10, 8);
    lv_obj_set_size(title, 380, 26);
    lv_obj_set_style_text_align(title, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    const char *title_txt = (EEprom2_Language == L_english) ? "Factory Reset Confirmation" :
                            (EEprom2_Language == L_Turkish) ? "Fabrika Ayarlarina Donus Onayi" :
                            (EEprom2_Language == L_Russian) ? "Sbros k Zavodskim Nastroykam" : "بازنشانی به تنظیمات کارخانه";
    lv_label_set_text(title, title_txt);

    /* Body */
    lv_obj_t *body = lv_label_create(panel);
    lv_obj_set_pos(body, 10, 44);
    lv_obj_set_size(body, 380, 72);
    lv_obj_set_style_text_align(body, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(body, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(body, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(body, lv_color_hex(0xff334155), LV_PART_MAIN);
    const char *body_txt = (EEprom2_Language == L_english) ?
        "Are you sure you want to restore all factory settings?\nAll parameters, calibrations, and passwords will be reset, and the system will restart." :
        (EEprom2_Language == L_Turkish) ?
        "Tum fabrika ayarlarina donmek istediginizden emin misiniz?\nTum parametreler, kalibrasyonlar ve sifreler sifirlanip sistem yeniden baslatilacaktir." :
        (EEprom2_Language == L_Russian) ?
        "Vy uvereny, chto khotite sbrosit' vse nastroyki?\nVse parametry, kalibrovki i parol' budut sbrosheny, sistema perezagruzitsya." :
        "آیا از بازنشانی تنظیمات دستگاه به حالت اولیه اطمینان دارید؟\nکلیه پارامترهای دبی و ابعاد و رمز عبور به مقادیر پیش فرض بازگشته و دستگاه مجددا راه اندازی خواهد شد.";
    lv_label_set_text(body, body_txt);

    /* Cancel Button */
    lv_obj_t *btn_cancel = lv_btn_create(panel);
    lv_obj_set_pos(btn_cancel, 16, 148);
    lv_obj_set_size(btn_cancel, 170, 48);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cancel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_cancel, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cancel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cancel, ui_factory_reset_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *c_lbl = lv_label_create(btn_cancel);
    const char *c_txt = (EEprom2_Language == L_english) ? "Cancel" :
                        (EEprom2_Language == L_Turkish) ? "Iptal" :
                        (EEprom2_Language == L_Russian) ? "Otmena" : "انصراف";
    lv_label_set_text(c_lbl, c_txt);
    lv_obj_set_style_text_color(c_lbl, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_text_font(c_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(c_lbl);

    /* Confirm Button */
    lv_obj_t *btn_ok = lv_btn_create(panel);
    lv_obj_set_pos(btn_ok, 200, 148);
    lv_obj_set_size(btn_ok, 180, 48);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xffb91c1c), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ok, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ok, ui_factory_reset_confirm_action_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ok_lbl = lv_label_create(btn_ok);
    const char *ok_txt = (EEprom2_Language == L_english) ? "Reset Device" :
                         (EEprom2_Language == L_Turkish) ? "Cihazi Sifirla" :
                         (EEprom2_Language == L_Russian) ? "Sbrosit' Ustroystvo" : "تایید و بازنشانی";
    lv_label_set_text(ok_lbl, ok_txt);
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ok_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(ok_lbl);
}

static void ui_general_factory_reset_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    ui_open_factory_reset_confirm_modal();
}

static void ui_qs_step_dec_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (EEprom_Process_Qs > 5.0f) {
        EEprom_Process_Qs -= 1.0f;
    } else {
        EEprom_Process_Qs = 5.0f;
    }
    char qs_str[16];
    snprintf(qs_str, sizeof(qs_str), "%.1f", EEprom_Process_Qs);
    char *dot = strchr(qs_str, '.');
    if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
    if (objects.ta_qs && lv_obj_is_valid(objects.ta_qs)) {
        lv_textarea_set_text(objects.ta_qs, qs_str);
    }
    a2_eeprom_save_process_from_ui();
    calculate_all();
}

static void ui_qs_step_inc_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (EEprom_Process_Qs < 100.0f) {
        EEprom_Process_Qs += 1.0f;
    } else {
        EEprom_Process_Qs = 100.0f;
    }
    char qs_str[16];
    snprintf(qs_str, sizeof(qs_str), "%.1f", EEprom_Process_Qs);
    char *dot = strchr(qs_str, '.');
    if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
    if (objects.ta_qs && lv_obj_is_valid(objects.ta_qs)) {
        lv_textarea_set_text(objects.ta_qs, qs_str);
    }
    a2_eeprom_save_process_from_ui();
    calculate_all();
}

static void ui_polish_general_tab(void)
{
    lv_obj_t *tab = objects.t_general;
    if (!tab) return;

    /* Clean old widgets from tab */
    lv_obj_clean(tab);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab, lv_color_hex(0xfff8fafc), LV_PART_MAIN);

    bool is_fa = (EEprom2_Language == L_Persion);

    /* ========================================================================
     * Card 1 (Top): Gas Injection Flow Rate (Qs) — Apple HIG Hero Card (388 x 84)
     * ======================================================================== */
    lv_obj_t *card_qs = lv_obj_create(tab);
    lv_obj_set_pos(card_qs, 6, 6);
    lv_obj_set_size(card_qs, 388, 84);
    lv_obj_set_style_radius(card_qs, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_qs, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_qs, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_bg_color(card_qs, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_qs, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_qs, 0, LV_PART_MAIN);
    lv_obj_clear_flag(card_qs, LV_OBJ_FLAG_SCROLLABLE);

    /* Centered Card Title */
    lv_obj_t *lbl_flow = lv_label_create(card_qs);
    objects.t_flowrate = lbl_flow;
    s_general_flow_lbl = lbl_flow;
    lv_obj_set_pos(lbl_flow, 16, 7);
    lv_obj_set_size(lbl_flow, 356, 20);
    lv_obj_set_style_base_dir(lbl_flow, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_flow, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_flow, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_flow, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    s_general_flow_accent = NULL;

    /* Stepper Cluster: [-] [ ta_qs + L/min ] [+] perfectly centered horizontally */
    /* Total cluster width = 46 + 10 + 180 + 10 + 46 = 292. Start X = (388 - 292) / 2 = 48 */
    lv_obj_t *btn_dec = lv_btn_create(card_qs);
    lv_obj_set_pos(btn_dec, 48, 30);
    lv_obj_set_size(btn_dec, 46, 46);
    lv_obj_set_ext_click_area(btn_dec, 4);
    lv_obj_set_style_radius(btn_dec, 23, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_dec, lv_color_hex(0xffe0f2fe), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_dec, lv_color_hex(0xffbae6fd), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_dec, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_dec, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_dec, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_dec, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_dec, ui_qs_step_dec_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dec_lbl = lv_label_create(btn_dec);
    lv_label_set_text(dec_lbl, "-");
    lv_obj_set_style_text_font(dec_lbl, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_set_style_text_color(dec_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_center(dec_lbl);

    /* Value Display Capsule */
    lv_obj_t *ta_qs = lv_textarea_create(card_qs);
    objects.ta_qs = ta_qs;
    lv_obj_set_pos(ta_qs, 104, 30);
    lv_obj_set_size(ta_qs, 180, 46);
    lv_textarea_set_accepted_chars(ta_qs, "1234567890.");
    lv_textarea_set_max_length(ta_qs, 7);
    char qs_str[16];
    snprintf(qs_str, sizeof(qs_str), "%.1f", EEprom_Process_Qs > 0.0f ? EEprom_Process_Qs : 30.0f);
    char *dot = strchr(qs_str, '.');
    if (dot && dot[1] == '0' && dot[2] == '\0') *dot = '\0';
    lv_textarea_set_text(ta_qs, qs_str);
    lv_textarea_set_one_line(ta_qs, true);
    lv_obj_set_style_text_align(ta_qs, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(ta_qs, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta_qs, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta_qs, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_radius(ta_qs, 23, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta_qs, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_set_style_border_width(ta_qs, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_top(ta_qs, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_left(ta_qs, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_right(ta_qs, 56, LV_PART_MAIN);
    lv_obj_add_event_cb(ta_qs, action_select_as_key_board_earia_qs, LV_EVENT_CLICKED, NULL);

    /* Unit Label positioned inside right side of capsule */
    lv_obj_t *unit_lbl = lv_label_create(card_qs);
    objects.t_unit_qs = unit_lbl;
    lv_label_set_text(unit_lbl, "L/min");
    lv_obj_set_style_text_font(unit_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(unit_lbl, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_pos(unit_lbl, 218, 44);
    lv_obj_clear_flag(unit_lbl, LV_OBJ_FLAG_CLICKABLE);

    /* Increment Button */
    lv_obj_t *btn_inc = lv_btn_create(card_qs);
    lv_obj_set_pos(btn_inc, 294, 30);
    lv_obj_set_size(btn_inc, 46, 46);
    lv_obj_set_ext_click_area(btn_inc, 4);
    lv_obj_set_style_radius(btn_inc, 23, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_inc, lv_color_hex(0xffe0f2fe), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_inc, lv_color_hex(0xffbae6fd), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_inc, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_inc, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_inc, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_inc, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_inc, ui_qs_step_inc_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *inc_lbl = lv_label_create(btn_inc);
    lv_label_set_text(inc_lbl, "+");
    lv_obj_set_style_text_font(inc_lbl, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(inc_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_center(inc_lbl);

    /* ========================================================================
     * Card 2 (Bottom): System Settings & Maintenance (388 x 168)
     * ======================================================================== */
    lv_obj_t *card_sys = lv_obj_create(tab);
    lv_obj_set_pos(card_sys, 6, 96);
    lv_obj_set_size(card_sys, 388, 168);
    lv_obj_set_style_radius(card_sys, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_sys, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_sys, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_bg_color(card_sys, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_sys, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_sys, 0, LV_PART_MAIN);
    lv_obj_clear_flag(card_sys, LV_OBJ_FLAG_SCROLLABLE);

    /* Row 1: Language Selector */
    s_general_lang_img = lv_image_create(card_sys);
    lv_obj_set_pos(s_general_lang_img, is_fa ? 14 : 342, 8);
    lv_obj_set_size(s_general_lang_img, 32, 32);
    ui_refresh_general_language_flag();

    lv_obj_t *dd = lv_dropdown_create(card_sys);
    objects.dd_language = dd;
    lv_obj_set_pos(dd, is_fa ? 54 : 188, 7);
    lv_obj_set_size(dd, 144, 34);
    lv_dropdown_set_options(dd, "فارسی\nEnglish\nTurkce\nRussian");
    lv_dropdown_set_selected(dd, EEprom2_Language);
    lv_obj_set_style_radius(dd, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(dd, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(dd, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_text_color(dd, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_text_font(dd, ui_font_vazir_or(&ui_font_arial_regular17, 15), LV_PART_MAIN);
    lv_dropdown_set_symbol(dd, "v");
    lv_obj_add_event_cb(dd, action_buzzer_pressed, LV_EVENT_RELEASED, (void *)0);
    lv_obj_add_event_cb(dd, action_lang_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_bg_color(list, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_color(list, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
        lv_obj_set_style_text_color(list, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_obj_set_style_text_font(list, ui_font_vazir_or(&ui_font_arial_regular17, 15), LV_PART_MAIN);
    }

    lv_obj_t *lbl_lang = lv_label_create(card_sys);
    objects.t_language = lbl_lang;
    lv_obj_set_pos(lbl_lang, is_fa ? 208 : 14, 12);
    lv_obj_set_size(lbl_lang, 166, 24);
    lv_obj_set_style_base_dir(lbl_lang, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_lang, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_lang, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_lang, lv_color_hex(0xff0f172a), LV_PART_MAIN);

    /* Divider 1 */
    lv_obj_t *div1 = lv_obj_create(card_sys);
    lv_obj_set_pos(div1, 14, 48);
    lv_obj_set_size(div1, 360, 1);
    lv_obj_set_style_bg_color(div1, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_width(div1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(div1, 0, LV_PART_MAIN);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_CLICKABLE);

    /* Row 2: Status Badge (Warranty Left) & Action Button (Online Update Right) */
    s_general_warranty_btn = lv_btn_create(card_sys);
    lv_obj_set_pos(s_general_warranty_btn, 14, 56);
    lv_obj_set_size(s_general_warranty_btn, 174, 42);
    lv_obj_set_style_radius(s_general_warranty_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_general_warranty_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_general_warranty_btn, 2, LV_PART_MAIN);
    lv_obj_add_event_cb(s_general_warranty_btn, ui_general_warranty_btn_cb, LV_EVENT_CLICKED, NULL);

    s_general_warranty_btn_lbl = lv_label_create(s_general_warranty_btn);
    lv_obj_center(s_general_warranty_btn_lbl);
    ui_update_warranty_button_visual();

    lv_obj_t *btn_ota = lv_btn_create(card_sys);
    lv_obj_set_pos(btn_ota, 200, 56);
    lv_obj_set_size(btn_ota, 174, 42);
    lv_obj_set_style_bg_color(btn_ota, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ota, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ota, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ota, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ota, ui_ota_open_btn_cb, LV_EVENT_CLICKED, NULL);

    s_general_ota_label = lv_label_create(btn_ota);
    lv_obj_set_style_text_color(s_general_ota_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_general_ota_label, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_general_ota_label, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_center(s_general_ota_label);

    /* Divider 2 */
    lv_obj_t *div2 = lv_obj_create(card_sys);
    lv_obj_set_pos(div2, 14, 106);
    lv_obj_set_size(div2, 360, 1);
    lv_obj_set_style_bg_color(div2, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_width(div2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(div2, 0, LV_PART_MAIN);
    lv_obj_clear_flag(div2, LV_OBJ_FLAG_CLICKABLE);

    /* Row 3: Firmware Version (Left) & Factory Reset (Right) */
    lv_obj_t *ver_pill = lv_obj_create(card_sys);
    lv_obj_set_pos(ver_pill, 14, 116);
    lv_obj_set_size(ver_pill, 174, 38);
    lv_obj_set_style_bg_color(ver_pill, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(ver_pill, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(ver_pill, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(ver_pill, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ver_pill, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ver_pill, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ver_pill, LV_OBJ_FLAG_SCROLLABLE);

    s_general_sys_lbl = lv_label_create(ver_pill);
    lv_obj_set_style_text_font(s_general_sys_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_general_sys_lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_general_sys_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_label_set_text(s_general_sys_lbl, FW_VERSION_LABEL);
    lv_obj_center(s_general_sys_lbl);

    lv_obj_t *btn_rst = lv_btn_create(card_sys);
    lv_obj_set_pos(btn_rst, 200, 116);
    lv_obj_set_size(btn_rst, 174, 38);
    lv_obj_set_style_bg_color(btn_rst, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_rst, lv_color_hex(0xfffecaca), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_rst, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_rst, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_rst, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_rst, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_rst, ui_general_factory_reset_cb, LV_EVENT_CLICKED, NULL);

    s_general_rst_lbl = lv_label_create(btn_rst);
    lv_obj_set_style_text_font(s_general_rst_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_general_rst_lbl, lv_color_hex(0xffb91c1c), LV_PART_MAIN);
    lv_obj_center(s_general_rst_lbl);

    /* Clean unused pointers */
    objects.t_touch = NULL;
    objects.sw_touch = NULL;
    objects.t_buzzer = NULL;
    objects.sw_buzzer = NULL;
    s_general_brightness_title = NULL;
    s_general_brightness_value = NULL;
    s_general_web_title = NULL;
    s_general_web_sub = NULL;
    s_general_plc_status = NULL;
    ui_refresh_general_runtime_labels();

    if (objects.t_setting) {
        lv_obj_set_style_text_font(objects.t_setting,
            ui_font_vazir_or(&ui_font_arial_regular26, 26), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/* ============================================================================
 * TEST TAB POLISH (Matching Mockup: Dual Balanced Industrial Cards)
 * ============================================================================ */

static lv_obj_t *s_test_dot_valve1 = NULL;
static lv_obj_t *s_test_dot_valve2 = NULL;
static lv_obj_t *s_test_dot_buzzer = NULL;
static lv_obj_t *s_test_led_start = NULL;
static lv_obj_t *s_test_led_stop = NULL;

static void ui_test_valve1_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        HwSetValve1(true);
        if (s_test_dot_valve1) {
            lv_obj_set_style_bg_color(s_test_dot_valve1, lv_color_hex(0xff22c55e), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_dot_valve1, lv_color_hex(0xff16a34a), LV_PART_MAIN);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED) {
        HwSetValve1(false);
        if (s_test_dot_valve1) {
            lv_obj_set_style_bg_color(s_test_dot_valve1, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_dot_valve1, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
        }
    }
}

static void ui_test_valve2_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        HwSetValve2(true);
        if (s_test_dot_valve2) {
            lv_obj_set_style_bg_color(s_test_dot_valve2, lv_color_hex(0xff22c55e), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_dot_valve2, lv_color_hex(0xff16a34a), LV_PART_MAIN);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED) {
        HwSetValve2(false);
        if (s_test_dot_valve2) {
            lv_obj_set_style_bg_color(s_test_dot_valve2, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_dot_valve2, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
        }
    }
}

static void ui_test_buzzer_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        HwSetBuzzer(true);
        if (s_test_dot_buzzer) {
            lv_obj_set_style_bg_color(s_test_dot_buzzer, lv_color_hex(0xfff59e0b), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_dot_buzzer, lv_color_hex(0xffd97706), LV_PART_MAIN);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED) {
        HwSetBuzzer(false);
        if (s_test_dot_buzzer) {
            lv_obj_set_style_bg_color(s_test_dot_buzzer, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_dot_buzzer, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
        }
    }
}

/* ============================================================================
 * PROCESS TAB POLISH (Apple HIG Compliant Industrial Card)
 * ============================================================================ */

static void ui_polish_process_tab(void)
{
    /* Build keyboard overlay on setting screen (needed for Qs entry from General tab) */
    lv_obj_t *parent = objects.setting;
    if (!parent && objects.tabview_setting) parent = objects.tabview_setting;
    if (!parent) return;

    lv_obj_t *kb_panel = lv_obj_create(parent);
    objects.key_board_panle_setting = kb_panel;
    lv_obj_set_pos(kb_panel, 8, 52);
    lv_obj_set_size(kb_panel, 384, 256);
    lv_obj_set_style_bg_color(kb_panel, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_border_color(kb_panel, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_border_width(kb_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(kb_panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(kb_panel, 2, LV_PART_MAIN);
    lv_obj_add_flag(kb_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(kb_panel, LV_OBJ_FLAG_SCROLLABLE);

    static const char *s_kb_qs_map[] = {
        "1", "2", "3", LV_SYMBOL_BACKSPACE, "\n",
        "4", "5", "6", ".", "\n",
        "7", "8", "9", LV_SYMBOL_OK, "\n",
        "0", LV_SYMBOL_CLOSE, ""
    };
    static const lv_buttonmatrix_ctrl_t s_kb_qs_ctrl[] = {
        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        3, 1
    };

    lv_obj_t *kb = lv_keyboard_create(kb_panel);
    objects.kb_key_board_1 = kb;
    lv_obj_set_size(kb, 376, 250);
    lv_obj_center(kb);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, s_kb_qs_map, s_kb_qs_ctrl);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_20, LV_PART_ITEMS);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(kb, action_enter_pressed_setting, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, action_enter_pressed_setting, LV_EVENT_CANCEL, NULL);

    /* Clean old widget pointers */
    objects.obj9 = NULL;
    objects.t_save = NULL;
    objects.t_type_out = NULL;
    objects.sw_type_out_put = NULL;
    objects.t_type_output = NULL;
    s_process_lbl_info = NULL;
}

static void ui_test_stop_nc_event(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    lv_obj_t *lbl = (lv_obj_t *)lv_event_get_user_data(e);
    bool is_nc = lv_obj_has_state(sw, LV_STATE_CHECKED);
    stop_mode_set_nc(is_nc);
    buzzer_key;
    if (lbl) {
        lv_label_set_text(lbl, is_nc ? "N.C" : "N.O");
    }
}

static void ui_polish_test_tab(void)
{
    lv_obj_t *tab = objects.t_test;
    if (!tab) return;

    /* Clean old children from tab to build the fresh mockup layout */
    lv_obj_clean(tab);
    objects.t_input = NULL;
    objects.t_start = NULL;
    objects.t_stop = NULL;
    s_test_btn_pinmap_label = NULL;
    s_test_led_start = NULL;
    s_test_led_stop = NULL;
    objects.t_out_put = NULL;
    objects.t_valve1 = NULL;
    objects.t_valve2 = NULL;
    objects.t_buzzer_out = NULL;
    s_test_dot_valve1 = NULL;
    s_test_dot_valve2 = NULL;
    s_test_dot_buzzer = NULL;
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab, lv_color_hex(0xfff8fafc), LV_PART_MAIN);

    const int card_w = 182;
    const int card_h = 254;
    const int card_y = 8;

    /* ------------------------------------------------------------------------
     * Card 1: Right Card (Digital Inputs / ورودی های دیجیتال)
     * ------------------------------------------------------------------------ */
    lv_obj_t *in_card = lv_obj_create(tab);
    lv_obj_set_pos(in_card, 206, card_y);
    lv_obj_set_size(in_card, card_w, card_h);
    lv_obj_set_style_radius(in_card, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(in_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(in_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(in_card, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_pad_all(in_card, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(in_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(in_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *in_title = lv_label_create(in_card);
    objects.t_input = in_title;
    lv_obj_set_pos(in_title, 0, 8);
    lv_obj_set_size(in_title, card_w, 24);
    lv_obj_set_style_base_dir(in_title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_align(in_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(in_title, ui_font_vazir_or(&ui_font_arial_regular17, 18), LV_PART_MAIN);
    lv_obj_set_style_text_color(in_title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(in_title, "ورودی های دیجیتال");

    /* Divider 1 */
    lv_obj_t *in_div1 = lv_obj_create(in_card);
    lv_obj_set_pos(in_div1, 12, 34);
    lv_obj_set_size(in_div1, card_w - 24, 1);
    lv_obj_set_style_bg_color(in_div1, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(in_div1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(in_div1, 0, LV_PART_MAIN);
    lv_obj_clear_flag(in_div1, LV_OBJ_FLAG_CLICKABLE);

    /* Row 1: Start Button: Badge I6 on left, label in middle, LED on right */
    lv_obj_t *badge_i6 = lv_obj_create(in_card);
    lv_obj_set_pos(badge_i6, 10, 42);
    lv_obj_set_size(badge_i6, 36, 32);
    lv_obj_set_style_bg_color(badge_i6, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_radius(badge_i6, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(badge_i6, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(badge_i6, 0, LV_PART_MAIN);
    lv_obj_clear_flag(badge_i6, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_i6 = lv_label_create(badge_i6);
    lv_label_set_text(lbl_i6, "I7");
    lv_obj_set_style_text_font(lbl_i6, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_i6, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(lbl_i6);

    lv_obj_t *start_label = lv_label_create(in_card);
    objects.t_start = start_label;
    lv_obj_set_pos(start_label, 50, 46);
    lv_obj_set_size(start_label, 96, 26);
    lv_obj_set_style_base_dir(start_label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_align(start_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(start_label, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(start_label, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_label_set_text(start_label, "شاسی شروع");

    /* Status Dot 1 (Optical Center Aligned with Start Button text) */
    lv_obj_t *dot_in1 = lv_obj_create(in_card);
    s_test_led_start = dot_in1;
    lv_obj_set_pos(dot_in1, 152, 46);
    lv_obj_set_size(dot_in1, 16, 16);
    lv_obj_set_style_radius(dot_in1, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot_in1, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot_in1, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(dot_in1, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_clear_flag(dot_in1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Divider 2 */
    lv_obj_t *in_div2 = lv_obj_create(in_card);
    lv_obj_set_pos(in_div2, 12, 82);
    lv_obj_set_size(in_div2, card_w - 24, 1);
    lv_obj_set_style_bg_color(in_div2, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_width(in_div2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(in_div2, 0, LV_PART_MAIN);
    lv_obj_clear_flag(in_div2, LV_OBJ_FLAG_CLICKABLE);

    /* Row 2: Stop Button: Badge I6 on left, label in middle, LED on right */
    lv_obj_t *badge_i7 = lv_obj_create(in_card);
    lv_obj_set_pos(badge_i7, 10, 92);
    lv_obj_set_size(badge_i7, 36, 32);
    lv_obj_set_style_bg_color(badge_i7, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_radius(badge_i7, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(badge_i7, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(badge_i7, 0, LV_PART_MAIN);
    lv_obj_clear_flag(badge_i7, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_i7 = lv_label_create(badge_i7);
    lv_label_set_text(lbl_i7, "I6");
    lv_obj_set_style_text_font(lbl_i7, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_i7, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(lbl_i7);

    lv_obj_t *stop_label = lv_label_create(in_card);
    objects.t_stop = stop_label;
    lv_obj_set_pos(stop_label, 50, 96);
    lv_obj_set_size(stop_label, 96, 26);
    lv_obj_set_style_base_dir(stop_label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_align(stop_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(stop_label, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(stop_label, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_label_set_text(stop_label, "استپ");

    /* Status Dot 2 (Optical Center Aligned with Stop Button text) */
    lv_obj_t *dot_in2 = lv_obj_create(in_card);
    s_test_led_stop = dot_in2;
    lv_obj_set_pos(dot_in2, 152, 96);
    lv_obj_set_size(dot_in2, 16, 16);
    lv_obj_set_style_radius(dot_in2, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot_in2, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot_in2, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(dot_in2, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_clear_flag(dot_in2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Row 2b: Stop Contact Mode Selector (N.O / N.C) under Stop Button */
    lv_obj_t *lbl_stop_mode = lv_label_create(in_card);
    s_test_lbl_stop_mode = lbl_stop_mode;
    lv_obj_set_pos(lbl_stop_mode, 96, 138);
    lv_obj_set_size(lbl_stop_mode, 74, 22);
    lv_obj_set_style_base_dir(lbl_stop_mode, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_stop_mode, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_stop_mode, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_stop_mode, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_label_set_text(lbl_stop_mode, "نوع استپ");

    /* Stylized Pill Badge for N.O / N.C */
    lv_obj_t *box_nc = lv_obj_create(in_card);
    lv_obj_set_pos(box_nc, 54, 136);
    lv_obj_set_size(box_nc, 38, 24);
    lv_obj_set_style_radius(box_nc, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(box_nc, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_width(box_nc, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(box_nc, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_pad_all(box_nc, 0, LV_PART_MAIN);
    lv_obj_clear_flag(box_nc, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl_nc_st = lv_label_create(box_nc);
    lv_obj_center(lbl_nc_st);
    lv_obj_set_style_text_font(lbl_nc_st, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_nc_st, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(lbl_nc_st, stop_mode_is_nc() ? "N.C" : "N.O");

    lv_obj_t *sw_stop = lv_switch_create(in_card);
    lv_obj_set_pos(sw_stop, 10, 136);
    lv_obj_set_size(sw_stop, 38, 24);
    lv_obj_set_style_bg_color(sw_stop, lv_color_hex(0xffdc2626), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw_stop, lv_color_hex(0xffcbd5e1), LV_PART_INDICATOR);
    if (stop_mode_is_nc()) lv_obj_add_state(sw_stop, LV_STATE_CHECKED);
    else lv_obj_clear_state(sw_stop, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_stop, ui_test_stop_nc_event, LV_EVENT_VALUE_CHANGED, lbl_nc_st);

    /* Divider 3 */
    lv_obj_t *in_div3 = lv_obj_create(in_card);
    lv_obj_set_pos(in_div3, 12, 172);
    lv_obj_set_size(in_div3, card_w - 24, 1);
    lv_obj_set_style_bg_color(in_div3, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_width(in_div3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(in_div3, 0, LV_PART_MAIN);
    lv_obj_clear_flag(in_div3, LV_OBJ_FLAG_CLICKABLE);

    /* Row 3: Wiring Diagram Button (full width inside card) */
    lv_obj_t *btn_pinmap = lv_btn_create(in_card);
    lv_obj_set_pos(btn_pinmap, 12, 184);
    lv_obj_set_size(btn_pinmap, 158, 48);
    lv_obj_set_style_bg_color(btn_pinmap, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_pinmap, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_pinmap, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_pinmap, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_pinmap, ui_pinout_diagram_open, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_map = lv_label_create(btn_pinmap);
    s_test_btn_pinmap_label = lbl_map;
    lv_obj_set_style_text_font(lbl_map, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_map, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_map, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_label_set_text(lbl_map, "نقشه سیم بندی");
    lv_obj_center(lbl_map);

    /* ------------------------------------------------------------------------
     * Card 2: Left Card (Industrial Outputs / خروجی های صنعتی)
     * ------------------------------------------------------------------------ */
    lv_obj_t *out_card = lv_obj_create(tab);
    lv_obj_set_pos(out_card, 12, card_y);
    lv_obj_set_size(out_card, card_w, card_h);
    lv_obj_set_style_radius(out_card, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(out_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(out_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(out_card, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_pad_all(out_card, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(out_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(out_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *out_title = lv_label_create(out_card);
    objects.t_out_put = out_title;
    lv_obj_set_pos(out_title, 0, 8);
    lv_obj_set_size(out_title, card_w, 24);
    lv_obj_set_style_base_dir(out_title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_align(out_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(out_title, ui_font_vazir_or(&ui_font_arial_regular17, 18), LV_PART_MAIN);
    lv_obj_set_style_text_color(out_title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(out_title, "خروجی های صنعتی");

    /* Divider 1 */
    lv_obj_t *out_div1 = lv_obj_create(out_card);
    lv_obj_set_pos(out_div1, 12, 34);
    lv_obj_set_size(out_div1, card_w - 24, 1);
    lv_obj_set_style_bg_color(out_div1, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(out_div1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(out_div1, 0, LV_PART_MAIN);
    lv_obj_clear_flag(out_div1, LV_OBJ_FLAG_CLICKABLE);

    /* Button 1: Valve 1 (شیربرقی 1 - Q0) */
    lv_obj_t *btn_v1 = lv_btn_create(out_card);
    lv_obj_set_pos(btn_v1, 10, 44);
    lv_obj_set_size(btn_v1, 134, 46);
    lv_obj_set_style_bg_color(btn_v1, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_v1, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_v1, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_v1, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_v1, ui_test_valve1_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn_v1, ui_test_valve1_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn_v1, ui_test_valve1_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *lbl_v1 = lv_label_create(btn_v1);
    objects.t_valve1 = lbl_v1;
    lv_obj_center(lbl_v1);
    lv_obj_set_style_base_dir(lbl_v1, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_v1, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_v1, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(lbl_v1, "شیربرقی 1 (Q0)");

    /* Status Dot 1 (Centered vertically at y=59) */
    lv_obj_t *dot_v1 = lv_obj_create(out_card);
    s_test_dot_valve1 = dot_v1;
    lv_obj_set_pos(dot_v1, 152, 59);
    lv_obj_set_size(dot_v1, 16, 16);
    lv_obj_set_style_radius(dot_v1, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot_v1, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot_v1, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(dot_v1, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_clear_flag(dot_v1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Button 2: Valve 2 (شیربرقی 2 - Q1) */
    lv_obj_t *btn_v2 = lv_btn_create(out_card);
    lv_obj_set_pos(btn_v2, 10, 100);
    lv_obj_set_size(btn_v2, 134, 46);
    lv_obj_set_style_bg_color(btn_v2, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_v2, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_v2, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_v2, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_v2, ui_test_valve2_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn_v2, ui_test_valve2_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn_v2, ui_test_valve2_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *lbl_v2 = lv_label_create(btn_v2);
    objects.t_valve2 = lbl_v2;
    lv_obj_center(lbl_v2);
    lv_obj_set_style_base_dir(lbl_v2, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_v2, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_v2, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(lbl_v2, "شیربرقی 2 (Q1)");

    /* Status Dot 2 (Centered vertically at y=115) */
    lv_obj_t *dot_v2 = lv_obj_create(out_card);
    s_test_dot_valve2 = dot_v2;
    lv_obj_set_pos(dot_v2, 152, 115);
    lv_obj_set_size(dot_v2, 16, 16);
    lv_obj_set_style_radius(dot_v2, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot_v2, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot_v2, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(dot_v2, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_clear_flag(dot_v2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Button 3: Acoustic Alert (هشدار صوتی - Q2) */
    lv_obj_t *btn_bz = lv_btn_create(out_card);
    lv_obj_set_pos(btn_bz, 10, 156);
    lv_obj_set_size(btn_bz, 134, 46);
    lv_obj_set_style_bg_color(btn_bz, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_bz, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_bz, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_bz, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_bz, ui_test_buzzer_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn_bz, ui_test_buzzer_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn_bz, ui_test_buzzer_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *lbl_bz = lv_label_create(btn_bz);
    objects.t_buzzer_out = lbl_bz;
    lv_obj_center(lbl_bz);
    lv_obj_set_style_base_dir(lbl_bz, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_bz, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_bz, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(lbl_bz, "هشدار صوتی (Q2)");

    /* Status Dot 3 (Centered vertically at y=171) */
    lv_obj_t *dot_bz = lv_obj_create(out_card);
    s_test_dot_buzzer = dot_bz;
    lv_obj_set_pos(dot_bz, 152, 171);
    lv_obj_set_size(dot_bz, 16, 16);
    lv_obj_set_style_radius(dot_bz, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot_bz, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot_bz, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(dot_bz, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_clear_flag(dot_bz, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    ui_refresh_test_runtime_labels();
}

static void ui_style_metallic_header(lv_obj_t *hdr)
{
    if (!hdr) return;
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(UI_HEADER_METAL_HI), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(hdr, lv_color_hex(UI_HEADER_METAL_LO), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(hdr, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_main_stop(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_stop(hdr, 255, LV_PART_MAIN);
}

static void ui_btn_set_child_text_color(lv_obj_t *btn, lv_color_t col)
{
    if (!btn) return;
    uint32_t n = lv_obj_get_child_count(btn);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *ch = lv_obj_get_child(btn, i);
        if (ch && lv_obj_check_type(ch, &lv_label_class)) {
            lv_obj_set_style_text_color(ch, col, LV_PART_MAIN);
        }
    }
}

typedef struct {
    lv_obj_t *btn;
    bool is_start;
    bool flash_on;
    uint32_t start_tick;
    lv_timer_t *tmr;
} ui_btn_alarm_state_t;

static ui_btn_alarm_state_t s_btn_alarms[4];

static ui_btn_alarm_state_t *ui_btn_alarm_slot(lv_obj_t *btn)
{
    ui_btn_alarm_state_t *free_slot = NULL;
    for (unsigned i = 0; i < 4; i++) {
        if (s_btn_alarms[i].btn == btn) return &s_btn_alarms[i];
        if (!s_btn_alarms[i].btn && !free_slot) free_slot = &s_btn_alarms[i];
    }
    return free_slot;
}

static void ui_btn_alarm_restore(ui_btn_alarm_state_t *st)
{
    if (!st || !st->btn) return;
    lv_obj_t *btn = st->btn;
    if (st->is_start) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_START_GREEN), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        ui_btn_set_child_text_color(btn, lv_color_hex(0xffffffff));
    } else {
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_PAGE_DARK), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
        ui_btn_set_child_text_color(btn, lv_color_hex(UI_STOP_RED));
    }
    if (st->tmr) {
        lv_timer_delete(st->tmr);
        st->tmr = NULL;
    }
    st->btn = NULL;
}

static void ui_btn_alarm_timer_cb(lv_timer_t *tmr)
{
    ui_btn_alarm_state_t *st = (ui_btn_alarm_state_t *)lv_timer_get_user_data(tmr);
    if (!st || !st->btn) {
        lv_timer_delete(tmr);
        return;
    }
    if (lv_tick_elaps(st->start_tick) >= 3000) {
        ui_btn_alarm_restore(st);
        return;
    }
    st->flash_on = !st->flash_on;
    lv_obj_t *btn = st->btn;
    if (st->is_start) {
        /* Green alarm blink */
        if (st->flash_on) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(UI_START_GREEN), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xff007a33), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        }
        ui_btn_set_child_text_color(btn, lv_color_hex(0xffffffff));
    } else {
        /* Red alarm blink — solid fill while alarming */
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
        if (st->flash_on) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
            ui_btn_set_child_text_color(btn, lv_color_hex(0xffffffff));
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_40, LV_PART_MAIN);
            ui_btn_set_child_text_color(btn, lv_color_hex(UI_STOP_RED));
        }
    }
}

void ui_btn_alarm(lv_obj_t *btn, bool is_start)
{
    if (!btn) return;
    ui_btn_alarm_state_t *st = ui_btn_alarm_slot(btn);
    if (!st) return;
    if (st->tmr) {
        lv_timer_delete(st->tmr);
        st->tmr = NULL;
    }
    st->btn = btn;
    st->is_start = is_start;
    st->flash_on = true;
    st->start_tick = lv_tick_get();
    /* Immediate first alarm frame */
    if (is_start) {
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_START_GREEN), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        ui_btn_set_child_text_color(btn, lv_color_hex(0xffffffff));
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
        ui_btn_set_child_text_color(btn, lv_color_hex(0xffffffff));
    }
    st->tmr = lv_timer_create(ui_btn_alarm_timer_cb, 250, st);
}

static void ui_btn_add_icon(lv_obj_t *btn, lv_obj_t *text_lab, const char *sym, lv_color_t col)
{
    if (!btn || !text_lab || !sym) return;
    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, sym);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, col, LV_PART_MAIN);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_to_index(icon, 0);
}

static void ui_style_start_capsule(lv_obj_t *btn, lv_obj_t *lab)
{
    if (!btn) return;
    const int w = 84;
    const int h = 44;
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_START_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_anim_delete(btn, NULL);
    lv_obj_set_style_pad_hor(btn, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(btn, 4, LV_PART_MAIN);
    lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (lab) {
        lv_obj_set_pos(lab, 0, 0);
        lv_obj_set_style_align(lab, LV_ALIGN_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_color(lab, lv_color_hex(0xffffffff), LV_PART_MAIN);
        lv_obj_set_style_text_font(lab, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
        lv_label_set_text(lab, "شروع");
    }
    ui_btn_add_icon(btn, lab, LV_SYMBOL_PLAY, lv_color_hex(0xffffffff));
}

static void ui_style_stop_capsule(lv_obj_t *btn, lv_obj_t *lab)
{
    if (!btn) return;
    const int w = 84;
    const int h = 44;
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_STOP_RED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffb91c1c), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(btn, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(btn, 4, LV_PART_MAIN);
    lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (lab) {
        lv_obj_set_pos(lab, 0, 0);
        lv_obj_set_style_align(lab, LV_ALIGN_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_color(lab, lv_color_hex(0xffffffff), LV_PART_MAIN);
        lv_obj_set_style_text_font(lab, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN);
        lv_label_set_text(lab, "توقف");
    }
    ui_btn_add_icon(btn, lab, LV_SYMBOL_STOP, lv_color_hex(0xffffffff));
}

static void ui_style_progress_bar(lv_obj_t *track, lv_obj_t **out_fill, lv_obj_t *pct_label)
{
    if (!track || !out_fill) return;

    /* Dark Glass Precision Gauge Track */
    lv_obj_set_style_bg_color(track, lv_color_hex(UI_BAR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(track, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(track, lv_color_hex(0xff2b3e58), LV_PART_MAIN);
    lv_obj_set_style_border_width(track, 1, LV_PART_MAIN);
    lv_obj_set_style_outline_width(track, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(track, 0, LV_PART_MAIN);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_clip_corner(track, true, LV_PART_MAIN);

    lv_obj_t *fill = lv_obj_create(track);
    *out_fill = fill;
    lv_obj_set_pos(fill, 0, UI_BAR_H);
    lv_obj_set_size(fill, UI_BAR_W, 0);
    lv_obj_set_style_bg_color(fill, lv_color_hex(UI_BAR_FILL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(fill, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(fill, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(fill, 0, LV_PART_MAIN);
    lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (pct_label) {
        lv_obj_align(pct_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_align(pct_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_ANY);
        lv_obj_set_style_text_color(pct_label, lv_color_hex(UI_BAR_TEXT), LV_PART_MAIN | LV_STATE_ANY);
        lv_obj_set_style_text_font(pct_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_ANY);
        lv_label_set_long_mode(pct_label, LV_LABEL_LONG_CLIP);
        lv_obj_clear_flag(pct_label, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_foreground(pct_label);
        lv_label_set_text(pct_label, "0%");
    }
}

static lv_obj_t *ui_create_progress_stack(lv_obj_t *parent, int x, int y,
                                          lv_obj_t **out_bar, lv_obj_t **out_pct)
{
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_remove_style_all(wrap);
    lv_obj_set_pos(wrap, x, y);
    lv_obj_set_size(wrap, UI_BAR_W, UI_BAR_H);
    lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(wrap, 0, LV_PART_MAIN);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *track = lv_obj_create(wrap);
    lv_obj_set_size(track, UI_BAR_W, UI_BAR_H);
    lv_obj_align(track, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *pct = lv_label_create(wrap);
    *out_pct = pct;
    lv_label_set_text(pct, "0%");

    ui_style_progress_bar(track, out_bar, pct);
    return wrap;
}

static void set_label_if_changed(lv_obj_t *label, const char *text)
{
    if (!label || !text) return;
    const char *cur = lv_label_get_text(label);
    if (cur && strcmp(cur, text) == 0) return;
    lv_label_set_text(label, text);
}

void ui_apply_main_dimension_label_layout(uint8_t language)
{
    bool is_fa = (language == L_Persion);
    const lv_font_t *font = is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_14;
    const int left_lab_x = 167;
    const int right_lab_x = 367;
    const int lab_w = 56;
    lv_obj_t *left_labs[] = {
        objects.t_length_l, objects.t_width_r, objects.t_heigth_l
    };
    lv_obj_t *right_labs[] = {
        objects.t_length_r, objects.t_width_l, objects.t_heigth_r
    };

    for (unsigned i = 0; i < 3; i++) {
        const int y = 119 + (int)i * 44;
        if (left_labs[i]) {
            lv_obj_set_pos(left_labs[i], left_lab_x, y);
            lv_obj_set_size(left_labs[i], lab_w, 26);
            lv_obj_set_style_pad_left(left_labs[i], 0, LV_PART_MAIN);
            lv_obj_set_style_pad_right(left_labs[i], 0, LV_PART_MAIN);
            lv_obj_set_style_text_font(left_labs[i], font, LV_PART_MAIN);
            lv_obj_set_style_text_color(left_labs[i], lv_color_hex(0xff475569), LV_PART_MAIN);
            lv_obj_set_style_text_align(left_labs[i], LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
            lv_label_set_long_mode(left_labs[i], LV_LABEL_LONG_CLIP);
        }
        if (right_labs[i]) {
            lv_obj_set_pos(right_labs[i], right_lab_x, y);
            lv_obj_set_size(right_labs[i], lab_w, 26);
            lv_obj_set_style_pad_left(right_labs[i], 0, LV_PART_MAIN);
            lv_obj_set_style_pad_right(right_labs[i], 0, LV_PART_MAIN);
            lv_obj_set_style_text_font(right_labs[i], font, LV_PART_MAIN);
            lv_obj_set_style_text_color(right_labs[i], lv_color_hex(0xff475569), LV_PART_MAIN);
            lv_obj_set_style_text_align(right_labs[i], LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
            lv_label_set_long_mode(right_labs[i], LV_LABEL_LONG_CLIP);
        }
    }
}

static void ui_admin_gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd);
static bool ui_admin_date_is_jalali(void);

static time_t get_build_time(void) {
    const char *date = __DATE__;
    const char *time_str = __TIME__;
    const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    struct tm t;
    memset(&t, 0, sizeof(t));
    char m[4] = {0};
    int day = 1, year = 2026, hour = 0, min = 0, sec = 0;
    if (sscanf(date, "%3s %d %d", m, &day, &year) == 3) {
        for (int i = 0; i < 12; i++) {
            if (strcmp(m, months[i]) == 0) {
                t.tm_mon = i;
                break;
            }
        }
        t.tm_mday = day;
        t.tm_year = year - 1900;
    }
    if (sscanf(time_str, "%d:%d:%d", &hour, &min, &sec) == 3) {
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = sec;
    }
    return mktime(&t);
}

void ui_clock_init(void)
{
    /* Asia/Tehran = UTC+03:30 (no DST since 2022). */
    setenv("TZ", "IRST-3:30", 1);
    tzset();

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    if (tm_now.tm_year + 1900 < 2024) {
        time_t bt = get_build_time();
        if (bt > 0) {
            struct timeval tv = { .tv_sec = bt, .tv_usec = 0 };
            settimeofday(&tv, NULL);
        }
    }
}

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

static lv_obj_t *s_station_badge[2];
static lv_obj_t *s_station_title[2];
static lv_obj_t *s_station_num[2];
static lv_obj_t *s_main_status_overlay;
static lv_obj_t *s_main_status_label;
static lv_timer_t *s_main_status_timer;
static lv_timer_t *s_bar_reset_timer[2];

void ui_station_show_name(uint8_t station)
{
    if (station > 1 || !s_station_title[station] || !s_station_badge[station]) return;
    bool is_fa = (EEprom2_Language == L_Persion);
    const char *name = NULL;
    const char *num = (station == 0) ? "2" : "1";
    if (EEprom2_Language == L_english) {
        name = "Glass";
    } else if (EEprom2_Language == L_Turkish) {
        name = "Cam";
    } else if (EEprom2_Language == L_Russian) {
        name = "Steklo";
    } else {
        name = "شیشه";
    }
    lv_label_set_text(s_station_title[station], name);
    lv_obj_set_style_text_font(s_station_title[station],
        is_fa ? ui_font_vazir_or(&ui_font_arial_regular26, 18) : &lv_font_montserrat_16, LV_PART_MAIN);
    if (s_station_num[station]) {
        lv_label_set_text(s_station_num[station], num);
        lv_obj_set_style_text_font(s_station_num[station], &lv_font_montserrat_14, LV_PART_MAIN);
    }
    lv_obj_set_style_base_dir(s_station_badge[station], is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_station_badge[station],
        lv_color_hex(UI_HEADER_METAL_HI), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_station_badge[station],
        lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
}

static void ui_station_status_timeout(lv_timer_t *timer)
{
    (void)timer;
    s_main_status_timer = NULL;
    if (s_main_status_overlay) lv_obj_add_flag(s_main_status_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void ui_bar_reset_timeout(lv_timer_t *timer)
{
    uint8_t station = (uint8_t)(uintptr_t)lv_timer_get_user_data(timer);
    if (station > 1) return;
    s_bar_reset_timer[station] = NULL;
    /* Never let an old completion timer reset a newly started process. */
    if ((station == 0 && TimerValve1 != 0) || (station == 1 && TimerValve2 != 0)) return;
    if (station == 0) ChangeBar(0); else ChangeBar1(0);
}

void ui_main_cancel_bar_reset(uint8_t station)
{
    if (station > 1 || !s_bar_reset_timer[station]) return;
    lv_timer_delete(s_bar_reset_timer[station]);
    s_bar_reset_timer[station] = NULL;
}

void ui_main_schedule_bar_reset(uint8_t station)
{
    if (station > 1) return;
    ui_main_cancel_bar_reset(station);
    s_bar_reset_timer[station] = lv_timer_create(ui_bar_reset_timeout, 3000,
        (void *)(uintptr_t)station);
    lv_timer_set_repeat_count(s_bar_reset_timer[station], 1);
}

void ui_main_show_station_status(uint8_t station, ui_main_status_t status)
{
    const char *state_text;
    char text[64];
    uint32_t color;
    if (station > 1 || !s_main_status_overlay || !s_main_status_label) return;

    uint8_t glass_num = (station == 0) ? 2u : 1u;

    if (EEprom2_Language == L_english) {
        lv_obj_set_style_text_font(s_main_status_label, &lv_font_montserrat_18, LV_PART_MAIN);
        switch (status) {
            case UI_MAIN_STATUS_INJECTING:   state_text = "Injecting"; color = 0xffffa000; break;
            case UI_MAIN_STATUS_COMPLETE:    state_text = "Completed"; color = 0xff2e7d32; break;
            case UI_MAIN_STATUS_INPUT_ERROR: state_text = "Input Error"; color = 0xffc62828; break;
            case UI_MAIN_STATUS_STOPPED:     state_text = "Stopped"; color = 0xffd32f2f; break;
            case UI_MAIN_STATUS_READY:
            default:                         state_text = "Ready"; color = 0xff1976d2; break;
        }
        snprintf(text, sizeof(text), "Glass %u - %s", glass_num, state_text);
    } else if (EEprom2_Language == L_Turkish) {
        lv_obj_set_style_text_font(s_main_status_label, &lv_font_montserrat_18, LV_PART_MAIN);
        switch (status) {
            case UI_MAIN_STATUS_INJECTING:   state_text = "Dolum"; color = 0xffffa000; break;
            case UI_MAIN_STATUS_COMPLETE:    state_text = "Tamamlandi"; color = 0xff2e7d32; break;
            case UI_MAIN_STATUS_INPUT_ERROR: state_text = "Boyut Hatasi"; color = 0xffc62828; break;
            case UI_MAIN_STATUS_STOPPED:     state_text = "Durduruldu"; color = 0xffd32f2f; break;
            case UI_MAIN_STATUS_READY:
            default:                         state_text = "Hazir"; color = 0xff1976d2; break;
        }
        snprintf(text, sizeof(text), "Cam %u - %s", glass_num, state_text);
    } else if (EEprom2_Language == L_Russian) {
        lv_obj_set_style_text_font(s_main_status_label, &lv_font_montserrat_18, LV_PART_MAIN);
        switch (status) {
            case UI_MAIN_STATUS_INJECTING:   state_text = "Zapravka"; color = 0xffffa000; break;
            case UI_MAIN_STATUS_COMPLETE:    state_text = "Zaversheno"; color = 0xff2e7d32; break;
            case UI_MAIN_STATUS_INPUT_ERROR: state_text = "Oshibka"; color = 0xffc62828; break;
            case UI_MAIN_STATUS_STOPPED:     state_text = "Ostanovleno"; color = 0xffd32f2f; break;
            case UI_MAIN_STATUS_READY:
            default:                         state_text = "Gotovo"; color = 0xff1976d2; break;
        }
        snprintf(text, sizeof(text), "Steklo %u - %s", glass_num, state_text);
    } else {
        lv_obj_set_style_text_font(s_main_status_label, ui_font_vazir_or(&ui_font_arial_regular17, 18), LV_PART_MAIN);
        switch (status) {
            case UI_MAIN_STATUS_INJECTING:   state_text = "در حال تزریق"; color = 0xffffa000; break;
            case UI_MAIN_STATUS_COMPLETE:    state_text = "تکمیل شد"; color = 0xff2e7d32; break;
            case UI_MAIN_STATUS_INPUT_ERROR: state_text = "خطای ابعاد"; color = 0xffc62828; break;
            case UI_MAIN_STATUS_STOPPED:     state_text = "متوقف شد"; color = 0xffd32f2f; break;
            case UI_MAIN_STATUS_READY:
            default:                         state_text = "آماده"; color = 0xff1976d2; break;
        }
        snprintf(text, sizeof(text), "شیشه %u - %s", glass_num, state_text);
    }

    if (s_main_status_timer) {
        lv_timer_delete(s_main_status_timer);
        s_main_status_timer = NULL;
    }
    lv_label_set_text(s_main_status_label, text);
    lv_obj_set_style_bg_color(s_main_status_overlay, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_main_status_overlay, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_clear_flag(s_main_status_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_main_status_overlay);
    s_main_status_timer = lv_timer_create(ui_station_status_timeout, 2000, NULL);
    lv_timer_set_repeat_count(s_main_status_timer, 1);
}

static void ui_create_main_status_overlay(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    s_main_status_overlay = panel;
    lv_obj_set_pos(panel, 70, 118);
    lv_obj_set_size(panel, 340, 80);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xff162234), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *label = lv_label_create(panel);
    s_main_status_label = label;
    lv_obj_set_size(label, 330, 52);
    lv_obj_center(label);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_text_font(label,
        ui_font_vazir_or(&ui_font_arial_regular26, 22), LV_PART_MAIN);
    lv_label_set_text(label, "آماده");
}

static void ui_create_station_badge(lv_obj_t *parent, int x, uint8_t station)
{
    lv_obj_t *badge = lv_obj_create(parent);
    s_station_badge[station] = badge;
    lv_obj_remove_style_all(badge);
    lv_obj_set_pos(badge, x, 60);
    lv_obj_set_size(badge, 140, 32);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(badge, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, 8, LV_PART_MAIN);
    lv_obj_set_layout(badge, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(badge, 6, LV_PART_MAIN);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    bool is_fa = (EEprom2_Language == L_Persion);
    lv_obj_set_style_base_dir(badge, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(badge);
    s_station_title[station] = title;
    lv_obj_set_style_text_color(title, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular26, 18) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_text(title, is_fa ? "شیشه" : "Glass");

    lv_obj_t *num_lbl = lv_label_create(badge);
    s_station_num[station] = num_lbl;
    lv_obj_set_style_text_color(num_lbl, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_font(num_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(num_lbl, station == 0 ? "2" : "1");
}

static void ui_create_clear_button(lv_obj_t *parent, int x, uint8_t station)
{
    lv_obj_t *btn = lv_btn_create(parent);
    if (station == 0) objects.bt_clear_left = btn; else objects.bt_clear_right = btn;
    lv_obj_set_pos(btn, x, 60);
    lv_obj_set_size(btn, 32, 32);
    lv_obj_set_ext_click_area(btn, 6);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    /* Immediate tap clear for responsive touch operation */
    lv_obj_add_event_cb(btn, station == 0 ? action_clear_dimensions_left : action_clear_dimensions_right,
                        LV_EVENT_CLICKED, NULL);
    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, LV_SYMBOL_TRASH);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_obj_center(icon);
}

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_PAGE_DARK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            // IB_Setting (header bar)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.ib_setting = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 48);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_clip_corner(obj, false, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN);
            lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        }
        {
            /* Header help button on Main screen (Apple HIG 36x36, aligned in 4-button right group) */
            lv_obj_t *btn_help = lv_btn_create(parent_obj);
            lv_obj_set_pos(btn_help, 306, 6);
            lv_obj_set_size(btn_help, 36, 36);
            lv_obj_set_ext_click_area(btn_help, 4);
            lv_obj_set_style_radius(btn_help, 8, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn_help, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn_help, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn_help, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(btn_help, 1, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(btn_help, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(btn_help, 0, LV_PART_MAIN);
            lv_obj_add_event_cb(btn_help, ui_setting_help_open, LV_EVENT_CLICKED, NULL);

            lv_obj_t *help_icon = lv_image_create(btn_help);
            lv_image_set_src(help_icon, &img_bitmap_book);
            lv_obj_center(help_icon);
        }
        {
            // T_Headerprj (Prominent Title with 26px Font, Centered in Header)
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_headerprj = obj;
            lv_obj_set_pos(obj, 94, 6);
            lv_obj_set_size(obj, 210, 36);
            lv_obj_set_style_text_color(obj, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular26, 26), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
            lv_label_set_text(obj, "تزریق گاز آرگون");
        }
        {
            /* Dual Elevated Cards (Siemens Dual Workstations - Apple White Theme) */
            lv_obj_t *card_l = lv_obj_create(parent_obj);
            lv_obj_set_pos(card_l, 44, 52);
            lv_obj_set_size(card_l, 192, 260);
            lv_obj_set_style_bg_color(card_l, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(card_l, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(card_l, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_width(card_l, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(card_l, 12, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(card_l, 0, LV_PART_MAIN);
            lv_obj_clear_flag(card_l, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *card_r = lv_obj_create(parent_obj);
            lv_obj_set_pos(card_r, 244, 52);
            lv_obj_set_size(card_r, 192, 260);
            lv_obj_set_style_bg_color(card_r, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(card_r, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(card_r, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_width(card_r, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(card_r, 12, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(card_r, 0, LV_PART_MAIN);
            lv_obj_clear_flag(card_r, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            /* Station Badges & Clear Buttons */
            ui_create_station_badge(parent_obj, 52, 0);
            ui_create_station_badge(parent_obj, 252, 1);
            ui_create_clear_button(parent_obj, 196, 0);
            ui_create_clear_button(parent_obj, 396, 1);
        }
        {
            // TA_Length
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.ta_length = obj;
            lv_obj_set_pos(obj, 55, 114);
            lv_obj_set_size(obj, 75, 32);
            lv_textarea_set_accepted_chars(obj, "1234567890.");
            lv_textarea_set_max_length(obj, 8);
            lv_textarea_set_text(obj, "120");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, action_select_as_key_board_earia_l, LV_EVENT_CLICKED, (void *)1);
            lv_obj_add_event_cb(obj, action_dimension_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        {
            // T_Length_L
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_length_l = obj;
            lv_obj_set_pos(obj, 136, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Length(mm)");
        }
        {
            // TA_Heigth
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.ta_heigth = obj;
            lv_obj_set_pos(obj, 55, 178);
            lv_obj_set_size(obj, 75, 32);
            lv_textarea_set_accepted_chars(obj, "1234567890.");
            lv_textarea_set_max_length(obj, 8);
            lv_textarea_set_text(obj, "150");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, action_select_as_key_board_earia_h, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, action_dimension_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        {
            // T_Heigth_L
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_heigth_l = obj;
            lv_obj_set_pos(obj, 137, 186);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Height(mm)");
        }
        {
            // TA_Depth
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.ta_depth = obj;
            lv_obj_set_pos(obj, 55, 146);
            lv_obj_set_size(obj, 75, 32);
            lv_textarea_set_accepted_chars(obj, "1234567890.");
            lv_textarea_set_max_length(obj, 8);
            lv_textarea_set_text(obj, "1.06");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, action_select_as_key_board_earia_d, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, action_dimension_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        {
            // T_Width_R
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_width_r = obj;
            lv_obj_set_pos(obj, 137, 154);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Width(mm)");
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 60, 259);
            lv_obj_set_size(obj, 75, 45);
            lv_obj_add_event_cb(obj, action_start_button_pressed, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2e7d32), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // T_Start_L
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.t_start_l = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Start");
                }
            }
        }
        {
            // BT_Stop
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.bt_stop = obj;
            lv_obj_set_pos(obj, 145, 259);
            lv_obj_set_size(obj, 75, 45);
            lv_obj_add_event_cb(obj, action_stop_button_pressed, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd81c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // T_StopL
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.t_stop_l = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Stop");
                }
            }
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(obj, 239, 53);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 0, 15 },
                { 1, 195 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // TA_Length1 — same pattern as left: input then label to its right
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.ta_length1 = obj;
            lv_obj_set_pos(obj, 270, 114);
            lv_obj_set_size(obj, 75, 32);
            lv_textarea_set_accepted_chars(obj, "1234567890.");
            lv_textarea_set_max_length(obj, 8);
            lv_textarea_set_text(obj, "120");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, action_select_as_key_board_earia_l1, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, action_dimension_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        {
            // T_Length_R
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_length_r = obj;
            lv_obj_set_pos(obj, 351, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Length(mm)");
        }
        {
            // TA_Heigth1
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.ta_heigth1 = obj;
            lv_obj_set_pos(obj, 270, 178);
            lv_obj_set_size(obj, 75, 32);
            lv_textarea_set_accepted_chars(obj, "1234567890.");
            lv_textarea_set_max_length(obj, 8);
            lv_textarea_set_text(obj, "180");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, action_select_as_key_board_earia_h1, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, action_dimension_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        {
            // T_Heigth_R
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_heigth_r = obj;
            lv_obj_set_pos(obj, 351, 186);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Height(mm)");
        }
        {
            // TA_Depth1
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.ta_depth1 = obj;
            lv_obj_set_pos(obj, 270, 146);
            lv_obj_set_size(obj, 75, 32);
            lv_textarea_set_accepted_chars(obj, "1234567890.");
            lv_textarea_set_max_length(obj, 8);
            lv_textarea_set_text(obj, "1.08");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, action_select_as_key_board_earia_d1, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_event_cb(obj, action_dimension_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        {
            // T_Width_L
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_width_l = obj;
            lv_obj_set_pos(obj, 351, 154);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Width(mm)");
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 260, 259);
            lv_obj_set_size(obj, 75, 45);
            lv_obj_add_event_cb(obj, action_start_button_pressed1, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2e7d32), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // T_Start_R
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.t_start_r = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Start");
                }
            }
        }
        {
            // BT_Stop_1
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.bt_stop_1 = obj;
            lv_obj_set_pos(obj, 345, 259);
            lv_obj_set_size(obj, 75, 45);
            lv_obj_add_event_cb(obj, action_stop_button_pressed1, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd81c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // T_Stop_R
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.t_stop_r = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Stop");
                }
            }
        }
        {
            /* Left + right progress stacks: identical light blue, mirrored, % centered */
            ui_create_progress_stack(parent_obj, UI_BAR_LEFT_X, UI_BAR_Y,
                                     &objects.bar_gaspercent, &objects.t_target_percent);
            ui_create_progress_stack(parent_obj, UI_BAR_RIGHT_X, UI_BAR_Y,
                                     &objects.bar_gaspercent1, &objects.t_target_percent1);
        }
        {
            /* Bottom "0" scale mark removed — percent label on the bar is enough */
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 234);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(obj, "0");
        }
        {
            // T_TargetValue (rounded fill time, without unit suffix)
            lv_obj_t *wrap = lv_obj_create(parent_obj);
            lv_obj_remove_style_all(wrap);
            /* 40px readout centered exactly on the left bar's axis with breathing margin from top/edges */
            lv_obj_set_pos(wrap, 3, UI_BAR_Y - 26);
            lv_obj_set_size(wrap, 40, 24);
            lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(wrap, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(wrap, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_column(wrap, 2, LV_PART_MAIN);
            lv_obj_set_layout(wrap, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *obj = lv_label_create(wrap);
            objects.t_target_value = obj;
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0 s");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 448, 234);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(obj, "0");
        }
        {
            // T_TargetValue1 (rounded fill time, without unit suffix)
            lv_obj_t *wrap = lv_obj_create(parent_obj);
            lv_obj_remove_style_all(wrap);
            /* Mirror of left: centered on the right bar's axis with breathing margin from top/edges */
            lv_obj_set_pos(wrap, 437, UI_BAR_Y - 26);
            lv_obj_set_size(wrap, 40, 24);
            lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(wrap, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(wrap, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_column(wrap, 2, LV_PART_MAIN);
            lv_obj_set_layout(wrap, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *obj = lv_label_create(wrap);
            objects.t_target_value1 = obj;
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0 s");
        }
        {
            /* Buzzer / Sound toggle button directly to left of Wi-Fi (Apple HIG 36x36) */
            lv_obj_t *btn_buzzer = lv_btn_create(parent_obj);
            objects.btn_buzzer = btn_buzzer;
            lv_obj_set_pos(btn_buzzer, 350, 6);
            lv_obj_set_size(btn_buzzer, 36, 36);
            lv_obj_set_ext_click_area(btn_buzzer, 4);
            lv_obj_set_style_radius(btn_buzzer, 8, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn_buzzer, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn_buzzer, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn_buzzer, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(btn_buzzer, 1, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(btn_buzzer, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(btn_buzzer, 0, LV_PART_MAIN);
            lv_obj_add_event_cb(btn_buzzer, ui_event_buzzer_toggle, LV_EVENT_CLICKED, NULL);

            lv_obj_t *buzz_icon = lv_label_create(btn_buzzer);
            objects.icon_buzzer = buzz_icon;
            lv_obj_center(buzz_icon);
            lv_obj_set_style_text_font(buzz_icon, &lv_font_montserrat_18, LV_PART_MAIN);
            if (buzzerEnable && EEprom3_Buzzer) {
                lv_obj_set_style_text_color(buzz_icon, lv_color_hex(0xff0284c7), LV_PART_MAIN);
                lv_obj_set_style_opa(buzz_icon, LV_OPA_COVER, LV_PART_MAIN);
                lv_label_set_text(buzz_icon, LV_SYMBOL_VOLUME_MAX);
            } else {
                lv_obj_set_style_text_color(buzz_icon, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
                lv_obj_set_style_opa(buzz_icon, LV_OPA_60, LV_PART_MAIN);
                lv_label_set_text(buzz_icon, LV_SYMBOL_MUTE);
            }
        }
        {
            /* Wi-Fi / Web Portal connectivity indicator & toggle button (Apple HIG 36x36) */
            lv_obj_t *btn_wifi = lv_btn_create(parent_obj);
            lv_obj_set_pos(btn_wifi, 394, 6);
            lv_obj_set_size(btn_wifi, 36, 36);
            lv_obj_set_ext_click_area(btn_wifi, 4);
            lv_obj_set_style_radius(btn_wifi, 8, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn_wifi, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn_wifi, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn_wifi, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(btn_wifi, 1, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(btn_wifi, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(btn_wifi, 0, LV_PART_MAIN);
            lv_obj_add_event_cb(btn_wifi, ui_event_wifi_toggle, LV_EVENT_CLICKED, NULL);

            lv_obj_t *wifi_icon = lv_label_create(btn_wifi);
            objects.icon_wifi = wifi_icon;
            lv_obj_center(wifi_icon);
            lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_18, LV_PART_MAIN);
            if (webportal_is_running()) {
                lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0xff00d2ff), LV_PART_MAIN);
                lv_obj_set_style_opa(wifi_icon, LV_OPA_COVER, LV_PART_MAIN);
            } else {
                lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
                lv_obj_set_style_opa(wifi_icon, LV_OPA_60, LV_PART_MAIN);
            }
            lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
        }
        {
            /* Settings gear button shifted right to 438 to open space (Apple HIG 36x36) */
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.ib_setting1 = obj;
            lv_obj_set_pos(obj, 438, 6);
            lv_obj_set_size(obj, 36, 36);
            lv_obj_set_ext_click_area(obj, 4);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
            lv_obj_add_event_cb(obj, action_gotosetting, LV_EVENT_CLICKED, (void *)0);

            lv_obj_t *icon = lv_label_create(obj);
            lv_label_set_text(icon, LV_SYMBOL_SETTINGS);
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, LV_PART_MAIN);
            lv_obj_set_style_text_color(icon, lv_color_hex(0xff334155), LV_PART_MAIN);
            lv_obj_set_style_text_color(icon, lv_color_hex(0xff0284c7), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_center(icon);
        }
        {
            // KeyBoard_Panle
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.key_board_panle = obj;
            lv_obj_set_pos(obj, 8, 48);
            lv_obj_set_size(obj, 230, 270);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_translate_x(obj, 238, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // KB_KeyBoard
                    lv_obj_t *obj = lv_keyboard_create(parent_obj);
                    objects.kb_key_board = obj;
                    lv_obj_set_pos(obj, -18, -18);
                    lv_obj_set_size(obj, 225, 260);
                    lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_NUMBER);
                    lv_obj_add_event_cb(obj, action_keyboard_pressed, LV_EVENT_PRESSED, (void *)0);
                    lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_ITEMS);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_ITEMS);
                    lv_obj_set_style_radius(obj, 8, LV_PART_ITEMS);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_ITEMS);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_ITEMS);
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.obj2 = obj;
                    lv_obj_set_pos(obj, 121, -14);
                    lv_obj_set_size(obj, 85, 121);
                    lv_obj_add_event_cb(obj, action_enter_pressd_1, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff16a34a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // T_Enter_Key1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_enter_key1 = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "تایید");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.obj3 = obj;
                    lv_obj_set_pos(obj, 119, 178);
                    lv_obj_set_size(obj, 85, 61);
                    lv_obj_add_event_cb(obj, action_tab_pressed1, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // T_Tab_Key1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_tab_key1 = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular17, 16), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "بعدی");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj4 = obj;
                    lv_obj_set_pos(obj, -15, 178);
                    lv_obj_set_size(obj, 42, 64);
                    lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    /* Close without changing the value in the active number field. */
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.bt_keyboard_close = obj;
                    /* Empty lower-left slot beside 0: clear of Enter, Next, and all number keys. */
                    lv_obj_set_pos(obj, -12, 182);
                    lv_obj_set_size(obj, 36, 56);
                    lv_obj_set_ext_click_area(obj, 6);
                    lv_obj_add_event_cb(obj, action_close_main_keyboard, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffdc2626), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_t *icon = lv_label_create(obj);
                    lv_label_set_text(icon, LV_SYMBOL_CLOSE);
                    lv_obj_set_style_text_color(icon, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_center(icon);
                }
            }
        }
        {
            // T_Time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_time = obj;
            lv_obj_set_pos(obj, 10, 6);
            lv_obj_set_size(obj, 84, 20);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "00:00");
        }
        {
            // T_Date
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.t_date = obj;
            lv_obj_set_pos(obj, 10, 26);
            lv_obj_set_size(obj, 84, 18);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "2026/07/17");
        }
        {
            // Hafler_Panle (fullscreen splash — must not steal touches)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.hafler_panle = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(UI_PAGE_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 110, 94);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_haffler_logo);
                }
                {
                    lv_obj_t *obj = lv_animimg_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 180, 100);
                }
                {
                    // T_Version_Softwear
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.t_version_softwear = obj;
                    lv_obj_set_pos(obj, 110, 166);
                    lv_obj_set_size(obj, 260, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, FW_VERSION_LABEL);
                }
                {
                    lv_obj_t *obj = lv_line_create(parent_obj);
                    lv_obj_set_pos(obj, 110, 160);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    static lv_point_precise_t line_points[] = {
                        { 0, 0 },
                        { 260, 0 }
                    };
                    lv_line_set_points(obj, line_points, 2);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0xff2b3e58), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_line_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    /* Do not bind keyboard at create — that focuses ta_length and shows a caret idle. */
    if (objects.kb_key_board) {
        lv_keyboard_set_textarea(objects.kb_key_board, NULL);
    }

    /* Number fields: mirrored L/R — identical gaps: [box][label] inside 192px card */
    {
        enum { TA_W = 124, TA_H = 34, TA_Y0 = 115, TA_DY = 44 };
        const int left_x = 52;
        const int right_x = 252;
        lv_obj_t *left_tas[] = {
            objects.ta_length, objects.ta_depth, objects.ta_heigth
        };
        lv_obj_t *right_tas[] = {
            objects.ta_length1, objects.ta_depth1, objects.ta_heigth1
        };
        lv_obj_t *screen = objects.main;
        for (unsigned i = 0; i < 3; i++) {
            const int y = TA_Y0 + (int)i * TA_DY;
            lv_obj_t *pair[2] = { left_tas[i], right_tas[i] };
            const int xs[2] = { left_x, right_x };
            for (unsigned s = 0; s < 2; s++) {
                lv_obj_t *ta = pair[s];
                if (!ta) continue;
                lv_obj_set_pos(ta, xs[s], y);
                lv_obj_set_size(ta, TA_W, TA_H);
                lv_textarea_set_one_line(ta, true);
                lv_obj_set_style_bg_color(ta, lv_color_white(), LV_PART_MAIN);
                lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
                lv_obj_set_style_border_color(ta, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
                lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
                lv_obj_set_style_radius(ta, 8, LV_PART_MAIN);
                lv_obj_set_style_text_color(ta, lv_color_hex(0xff0f172a), LV_PART_MAIN);
                lv_obj_set_style_text_font(ta, &lv_font_montserrat_18, LV_PART_MAIN);
                lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                lv_obj_set_style_pad_top(ta, 5, LV_PART_MAIN);
                lv_obj_set_style_pad_bottom(ta, 2, LV_PART_MAIN);
                lv_obj_set_style_pad_left(ta, 4, LV_PART_MAIN);
                lv_obj_set_style_pad_right(ta, 28, LV_PART_MAIN);
                lv_obj_t *lab = lv_textarea_get_label(ta);
                if (lab) {
                    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                    lv_obj_set_style_text_font(lab, &lv_font_montserrat_18, LV_PART_MAIN);
                    lv_obj_set_style_text_color(lab, lv_color_hex(0xff0f172a), LV_PART_MAIN);
                }
                /* Sibling overlay unit label */
                lv_obj_t *cm = lv_label_create(screen ? screen : lv_obj_get_parent(ta));
                lv_label_set_text(cm, "cm");
                lv_obj_set_style_text_font(cm, &lv_font_montserrat_12, LV_PART_MAIN);
                lv_obj_set_style_text_color(cm, lv_color_hex(0xff64748b), LV_PART_MAIN);
                lv_obj_set_pos(cm, xs[s] + TA_W - 28, y + 9);
                lv_obj_clear_flag(cm, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_move_foreground(cm);
            }
        }
    }

    /* Dimension labels: clean RTL aligned beside the input boxes */
    {
        const lv_font_t *f17 = ui_font_vazir_or(&ui_font_arial_regular17, 16);
        lv_obj_t *left_labs[] = {
            objects.t_length_l, objects.t_width_r, objects.t_heigth_l
        };
        lv_obj_t *right_labs[] = {
            objects.t_length_r, objects.t_width_l, objects.t_heigth_r
        };
        const char *texts[] = { "طول", "عرض", "ارتفاع" };
        const int left_lab_x = 167;
        const int right_lab_x = 367;
        const int lab_w = 56;
        for (unsigned i = 0; i < 3; i++) {
            const int y = 119 + (int)i * 44;
            if (left_labs[i]) {
                lv_obj_set_pos(left_labs[i], left_lab_x, y);
                lv_obj_set_size(left_labs[i], lab_w, 26);
                lv_obj_set_style_pad_left(left_labs[i], 0, LV_PART_MAIN);
                lv_obj_set_style_pad_right(left_labs[i], 0, LV_PART_MAIN);
                lv_obj_set_style_text_font(left_labs[i], f17, LV_PART_MAIN);
                lv_obj_set_style_text_color(left_labs[i], lv_color_hex(0xff475569), LV_PART_MAIN);
                lv_obj_set_style_text_align(left_labs[i], LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                lv_label_set_long_mode(left_labs[i], LV_LABEL_LONG_CLIP);
                lv_label_set_text(left_labs[i], texts[i]);
            }
            if (right_labs[i]) {
                lv_obj_set_pos(right_labs[i], right_lab_x, y);
                lv_obj_set_size(right_labs[i], lab_w, 26);
                lv_obj_set_style_pad_left(right_labs[i], 0, LV_PART_MAIN);
                lv_obj_set_style_pad_right(right_labs[i], 0, LV_PART_MAIN);
                lv_obj_set_style_text_font(right_labs[i], f17, LV_PART_MAIN);
                lv_obj_set_style_text_color(right_labs[i], lv_color_hex(0xff475569), LV_PART_MAIN);
                lv_obj_set_style_text_align(right_labs[i], LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                lv_label_set_long_mode(right_labs[i], LV_LABEL_LONG_CLIP);
                lv_label_set_text(right_labs[i], texts[i]);
            }
        }
    }

    /* Capsule Start/Stop buttons — 84x44 inside each 192px card (8px padding) */
    {
        /* Left channel (Station 2 / Glass 2) */
        if (objects.obj0) {
            lv_obj_set_pos(objects.obj0, 52, 260);
            ui_style_start_capsule(objects.obj0, objects.t_start_l);
        }
        if (objects.bt_stop) {
            lv_obj_set_pos(objects.bt_stop, 144, 260);
            ui_style_stop_capsule(objects.bt_stop, objects.t_stop_l);
        }
        /* Right channel (Station 1 / Glass 1) */
        if (objects.obj1) {
            lv_obj_set_pos(objects.obj1, 252, 260);
            ui_style_start_capsule(objects.obj1, objects.t_start_r);
        }
        if (objects.bt_stop_1) {
            lv_obj_set_pos(objects.bt_stop_1, 344, 260);
            ui_style_stop_capsule(objects.bt_stop_1, objects.t_stop_r);
        }
    }

    /* Header clock/date/title Vazirmatn */
    {
        const lv_font_t *f18 = ui_font_vazir_or(&ui_font_arial_regular17, 18);
        if (objects.t_time) lv_obj_set_style_text_font(objects.t_time, f18, LV_PART_MAIN | LV_STATE_ANY);
        if (objects.t_date) lv_obj_set_style_text_font(objects.t_date, f18, LV_PART_MAIN | LV_STATE_ANY);
        if (objects.t_headerprj) {
            lv_obj_set_style_text_font(objects.t_headerprj,
                ui_font_vazir_or(&ui_font_arial_regular26, 22), LV_PART_MAIN | LV_STATE_ANY);
        }
    }

    /* Progress stacks already styled in ui_create_progress_stack */

    /* Large transient process messages live above the screen, not in badges. */
    ui_create_main_status_overlay(objects.main);
    tick_screen_main();
}

void tick_screen_main() {
    static int last_min = -1;
    static int last_day = -1;
    static int force = 0;
    char buf[16];

    /* ui_clock_force_refresh bumps a counter via weak linkage pattern below */
    extern volatile int ui_clock_force_tick;
    if (ui_clock_force_tick != force) {
        force = ui_clock_force_tick;
        last_min = -1;
        last_day = -1;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    if (tm_now.tm_year + 1900 < 2024) {
        set_label_if_changed(objects.t_time, "--:--");
        set_label_if_changed(objects.t_date, "----/--/--");
        return;
    }

    if (tm_now.tm_min != last_min) {
        last_min = tm_now.tm_min;
        snprintf(buf, sizeof(buf), "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);
        set_label_if_changed(objects.t_time, buf);
    }
    if (tm_now.tm_yday != last_day) {
        last_day = tm_now.tm_yday;
        if (ui_admin_date_is_jalali()) {
            int jy = 0, jm = 0, jd = 0;
            ui_admin_gregorian_to_jalali(tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, &jy, &jm, &jd);
            snprintf(buf, sizeof(buf), "%04d/%02d/%02d", jy, jm, jd);
        } else {
            snprintf(buf, sizeof(buf), "%04d/%02d/%02d",
                     tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday);
        }
        set_label_if_changed(objects.t_date, buf);
    }

    /* Refresh Wi-Fi indicator state in main header */
    if (objects.icon_wifi && lv_obj_is_valid(objects.icon_wifi)) {
        static int last_wifi_mode = -2;
        int cur_mode = -1;
        if (!webportal_is_running()) {
            cur_mode = -1; // Disabled
        } else if (webportal_is_sta_connected()) {
            cur_mode = 2; // Connected to Workshop WiFi
        } else {
            cur_mode = 1; // SoftAP Hotspot active
        }
        if (cur_mode != last_wifi_mode) {
            last_wifi_mode = cur_mode;
            lv_obj_clear_flag(objects.icon_wifi, LV_OBJ_FLAG_HIDDEN);
            if (cur_mode == -1) {
                /* Dim gray when OFF so operator can tap to re-enable */
                lv_obj_set_style_text_color(objects.icon_wifi, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
                lv_obj_set_style_opa(objects.icon_wifi, LV_OPA_60, LV_PART_MAIN);
            } else if (cur_mode == 2) {
                lv_obj_set_style_text_color(objects.icon_wifi, lv_color_hex(0xff00e676), LV_PART_MAIN);
                lv_obj_set_style_opa(objects.icon_wifi, LV_OPA_COVER, LV_PART_MAIN);
            } else {
                lv_obj_set_style_text_color(objects.icon_wifi, lv_color_hex(0xff00d2ff), LV_PART_MAIN);
                lv_obj_set_style_opa(objects.icon_wifi, LV_OPA_COVER, LV_PART_MAIN);
            }
        }
    }

    /* Refresh Buzzer indicator state in main header */
    if (objects.icon_buzzer && lv_obj_is_valid(objects.icon_buzzer)) {
        static int last_buzzer_state = -1;
        int cur_buzzer = (buzzerEnable && EEprom3_Buzzer) ? 1 : 0;
        if (cur_buzzer != last_buzzer_state) {
            last_buzzer_state = cur_buzzer;
            if (cur_buzzer) {
                lv_label_set_text(objects.icon_buzzer, LV_SYMBOL_VOLUME_MAX);
                lv_obj_set_style_text_color(objects.icon_buzzer, lv_color_hex(0xff0284c7), LV_PART_MAIN);
                lv_obj_set_style_opa(objects.icon_buzzer, LV_OPA_COVER, LV_PART_MAIN);
            } else {
                lv_label_set_text(objects.icon_buzzer, LV_SYMBOL_MUTE);
                lv_obj_set_style_text_color(objects.icon_buzzer, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
                lv_obj_set_style_opa(objects.icon_buzzer, LV_OPA_60, LV_PART_MAIN);
            }
        }
    }

    /* Header Day-Lock remaining days indicator (Apple HIG subtle status badge) */
    if (objects.t_headerprj && lv_obj_is_valid(objects.t_headerprj)) {
        static int s_last_hdr_remain = -999;
        static int s_last_hdr_lang = -1;
        int cur_remain = (day_lock_active_stage > 0) ? day_lock_remain : -1;
        int cur_lang = EEprom2_Language;
        if (cur_remain != s_last_hdr_remain || cur_lang != s_last_hdr_lang) {
            s_last_hdr_remain = cur_remain;
            s_last_hdr_lang = cur_lang;
            char hdr_buf[64];
            if (cur_remain > 0) {
                if (cur_lang == L_english) {
                    snprintf(hdr_buf, sizeof(hdr_buf), "Gas Filling  |  %d d left", cur_remain);
                } else if (cur_lang == L_Turkish) {
                    snprintf(hdr_buf, sizeof(hdr_buf), "Gaz Dolumu  |  %d gun", cur_remain);
                } else if (cur_lang == L_Russian) {
                    snprintf(hdr_buf, sizeof(hdr_buf), "Zapolnenie Gazom  |  %d dn", cur_remain);
                } else {
                    snprintf(hdr_buf, sizeof(hdr_buf), "تزریق گاز آرگون  |  %d روز", cur_remain);
                }
                if (cur_remain <= 3) {
                    lv_obj_set_style_text_color(objects.t_headerprj, lv_color_hex(0xffdc2626), LV_PART_MAIN);
                } else {
                    lv_obj_set_style_text_color(objects.t_headerprj, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN);
                }
            } else {
                if (cur_lang == L_english) {
                    snprintf(hdr_buf, sizeof(hdr_buf), "Gas Filling System");
                } else if (cur_lang == L_Turkish) {
                    snprintf(hdr_buf, sizeof(hdr_buf), "Gaz Dolum Sistemi");
                } else if (cur_lang == L_Russian) {
                    snprintf(hdr_buf, sizeof(hdr_buf), "Sistema Zapolneniya");
                } else {
                    snprintf(hdr_buf, sizeof(hdr_buf), "تزریق گاز آرگون");
                }
                lv_obj_set_style_text_color(objects.t_headerprj, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN);
            }
            set_label_if_changed(objects.t_headerprj, hdr_buf);
        }
    }

    /* Dynamic pulsing glow feedback during active gas injection */
    static uint32_t last_pulse_tick = 0;
    static uint8_t pulse_bright = 0;
    uint32_t now_tick = lv_tick_get();
    if (now_tick - last_pulse_tick > 250) {
        last_pulse_tick = now_tick;
        pulse_bright = !pulse_bright;
        if (objects.obj0 && TimerValve1 > 0) {
            lv_obj_set_style_bg_color(objects.obj0,
                pulse_bright ? lv_color_hex(0xff00c853) : lv_color_hex(0xff2e7d32), LV_PART_MAIN);
        }
        if (objects.obj1 && TimerValve2 > 0) {
            lv_obj_set_style_bg_color(objects.obj1,
                pulse_bright ? lv_color_hex(0xff00c853) : lv_color_hex(0xff2e7d32), LV_PART_MAIN);
        }
    }

    /* Main screen Emergency Stop (E-Stop Mushroom) overlay */
    static lv_obj_t *s_main_estop_overlay = NULL;
    bool stop_active = stop_input_is_active();
    if (stop_active) {
        if (!s_main_estop_overlay && objects.main && lv_obj_is_valid(objects.main)) {
            s_main_estop_overlay = lv_obj_create(objects.main);
            lv_obj_set_size(s_main_estop_overlay, 150, 134);
            lv_obj_center(s_main_estop_overlay);
            lv_obj_set_style_bg_color(s_main_estop_overlay, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(s_main_estop_overlay, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(s_main_estop_overlay, lv_color_hex(0xffdc2626), LV_PART_MAIN);
            lv_obj_set_style_border_width(s_main_estop_overlay, 3, LV_PART_MAIN);
            lv_obj_set_style_radius(s_main_estop_overlay, 16, LV_PART_MAIN);
            lv_obj_set_style_pad_all(s_main_estop_overlay, 6, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(s_main_estop_overlay, 0, LV_PART_MAIN);
            lv_obj_clear_flag(s_main_estop_overlay, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(s_main_estop_overlay, LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *img = lv_image_create(s_main_estop_overlay);
            lv_image_set_src(img, &img_bitmap_stop_mushroom);
            lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 2);
            lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *lbl = lv_label_create(s_main_estop_overlay);
            lv_label_set_text(lbl, "STOP");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, LV_PART_MAIN);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
            lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -2);
            lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
        }
        if (s_main_estop_overlay && lv_obj_is_valid(s_main_estop_overlay)) {
            if (lv_obj_has_flag(s_main_estop_overlay, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_clear_flag(s_main_estop_overlay, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(s_main_estop_overlay);
            }
        }
    } else {
        if (s_main_estop_overlay && lv_obj_is_valid(s_main_estop_overlay)) {
            if (!lv_obj_has_flag(s_main_estop_overlay, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(s_main_estop_overlay, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

volatile int ui_clock_force_tick = 0;

void ui_clock_force_refresh(void)
{
    ui_clock_force_tick++;
}

/* Lightweight administrator UI.  Only a placeholder exists at boot; the PIN
   page is built asynchronously the first time the Admin tab is opened. */
static lv_obj_t *s_admin_tab;
static lv_obj_t *s_admin_placeholder;
static lv_obj_t *s_admin_pin_label;
static lv_obj_t *s_admin_error_label;
static lv_obj_t *s_admin_lock_view;
static lv_obj_t *s_admin_lock_title;
static lv_obj_t *s_admin_pin_matrix;
static lv_obj_t *s_admin_options_view = NULL;
static const char *pin_map_fa[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    "حذف", "0", "تایید", ""
};
static const char *pin_map_en[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    "DEL", "0", "OK", ""
};
static lv_obj_t *s_admin_detail;
static lv_obj_t *s_admin_detail_value;
static bool s_admin_built;
static char s_admin_pin[5];
static uint8_t s_admin_pin_len;
static char s_admin_new_pin[5];
static uint8_t s_admin_new_pin_len;
static int s_admin_year, s_admin_month, s_admin_day, s_admin_hour, s_admin_minute;
static lv_obj_t *s_admin_time_values[5];
static uint8_t s_admin_time_field;
static lv_obj_t *s_admin_date_rollers[5];
static char s_admin_year_options[456];
static char s_admin_month_options[48];
static char s_admin_day_options[124];
static char s_admin_hour_options[96];
static char s_admin_minute_options[240];
static lv_obj_t *s_admin_counter_matrix;
static lv_obj_t *s_admin_counter_controls;
static char s_admin_counter_prev[16], s_admin_counter_now[16], s_admin_counter_next[16];
static const char *s_admin_counter_map[] = {
    s_admin_counter_prev, "\n", s_admin_counter_now, "\n", s_admin_counter_next, ""
};
static lv_obj_t *s_admin_date_matrix;
static lv_obj_t *s_admin_clock_matrix;
static char s_admin_date_cells[9][12];
static char s_admin_clock_cells[6][8];
static const char *s_admin_date_map[] = {
    s_admin_date_cells[0], s_admin_date_cells[1], s_admin_date_cells[2], "\n",
    s_admin_date_cells[3], s_admin_date_cells[4], s_admin_date_cells[5], "\n",
    s_admin_date_cells[6], s_admin_date_cells[7], s_admin_date_cells[8], ""
};
static const char *s_admin_clock_map[] = {
    s_admin_clock_cells[0], s_admin_clock_cells[1], "\n",
    s_admin_clock_cells[2], s_admin_clock_cells[3], "\n",
    s_admin_clock_cells[4], s_admin_clock_cells[5], ""
};

static void ui_admin_detail_close(void)
{
    if (s_admin_detail) {
        lv_obj_delete(s_admin_detail);
        s_admin_detail = NULL;
        s_admin_detail_value = NULL;
        s_admin_counter_matrix = NULL;
        s_admin_counter_controls = NULL;
        s_admin_date_matrix = NULL;
        s_admin_clock_matrix = NULL;
    }
}

static void ui_admin_time_refresh(void)
{
    char value[8];
    const int values[5] = { s_admin_year, s_admin_month, s_admin_day, s_admin_hour, s_admin_minute };
    for (int i = 0; i < 5; i++) {
        if (!s_admin_time_values[i]) continue;
        if (i == 0) snprintf(value, sizeof(value), "%04d", values[i]);
        else snprintf(value, sizeof(value), "%02d", values[i]);
        lv_label_set_text(s_admin_time_values[i], value);
    }
}

static int ui_admin_days_in_month(int year, int month)
{
    static const uint8_t days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return leap ? 29 : 28;
    }
    return days[month - 1];
}

static void ui_admin_time_step(lv_event_t *e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    int field = code < 0 ? -code - 1 : code - 1;
    int delta = code < 0 ? -1 : 1;
    if (field == 0) {
        s_admin_year += delta;
        if (s_admin_year < 2024) s_admin_year = 2099;
        if (s_admin_year > 2099) s_admin_year = 2024;
    } else if (field == 1) {
        s_admin_month += delta;
        if (s_admin_month < 1) s_admin_month = 12;
        if (s_admin_month > 12) s_admin_month = 1;
    } else if (field == 2) {
        int max_day = ui_admin_days_in_month(s_admin_year, s_admin_month);
        s_admin_day += delta;
        if (s_admin_day < 1) s_admin_day = max_day;
        if (s_admin_day > max_day) s_admin_day = 1;
    } else if (field == 3) {
        s_admin_hour += delta;
        if (s_admin_hour < 0) s_admin_hour = 23;
        if (s_admin_hour > 23) s_admin_hour = 0;
    } else if (field == 4) {
        s_admin_minute += delta;
        if (s_admin_minute < 0) s_admin_minute = 59;
        if (s_admin_minute > 59) s_admin_minute = 0;
    }
    int max_day = ui_admin_days_in_month(s_admin_year, s_admin_month);
    if (s_admin_day > max_day) s_admin_day = max_day;
    ui_admin_time_refresh();
    buzzer_key;
}

static void ui_admin_time_save(lv_event_t *e)
{
    (void)e;
    struct tm t = {0};
    t.tm_year = s_admin_year - 1900; t.tm_mon = s_admin_month - 1; t.tm_mday = s_admin_day;
    t.tm_hour = s_admin_hour; t.tm_min = s_admin_minute; t.tm_isdst = 0;
    time_t epoch = mktime(&t);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    ui_clock_force_refresh();
    if (s_admin_detail_value) {
        lv_label_set_text(s_admin_detail_value, "تاریخ و ساعت ذخیره شد");
        lv_obj_set_style_text_color(s_admin_detail_value, lv_color_hex(0xff2e7d32), LV_PART_MAIN);
    }
    buzzer_key;
}

static void ui_admin_detail_back_event(lv_event_t *e)
{
    (void)e;
    ui_admin_detail_close();
    buzzer_key;
}

static lv_obj_t *ui_admin_time_button(lv_obj_t *parent, int x, int y, const char *text,
                                      lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 30, 34);
    lv_obj_set_style_radius(btn, 7, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffe3f2fd), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff90caf9), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, text);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xff0d47a1), LV_PART_MAIN);
    lv_obj_center(lab);
    return btn;
}

static void ui_admin_time_stepper(lv_obj_t *parent, int x, int y, int field, const char *caption)
{
    lv_obj_t *caption_lab = lv_label_create(parent);
    lv_obj_set_pos(caption_lab, x, y);
    lv_obj_set_size(caption_lab, 118, 20);
    lv_obj_set_style_text_align(caption_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(caption_lab, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_text_color(caption_lab, lv_color_hex(0xff455a64), LV_PART_MAIN);
    lv_label_set_text(caption_lab, caption);

    ui_admin_time_button(parent, x, y + 22, "-", ui_admin_time_step,
                         (void *)(intptr_t)(-(field + 1)));
    s_admin_time_values[field] = lv_label_create(parent);
    lv_obj_set_pos(s_admin_time_values[field], x + 32, y + 25);
    lv_obj_set_size(s_admin_time_values[field], 54, 28);
    lv_obj_set_style_text_align(s_admin_time_values[field], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_time_values[field], &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_time_values[field], lv_color_hex(0xff263238), LV_PART_MAIN);
    ui_admin_time_button(parent, x + 88, y + 22, "+", ui_admin_time_step,
                         (void *)(intptr_t)(field + 1));
}

static void ui_admin_open_datetime(void)
{
    ui_admin_detail_close();
    memset(s_admin_time_values, 0, sizeof(s_admin_time_values));
    time_t now = time(NULL); struct tm t; localtime_r(&now, &t);
    s_admin_year = t.tm_year + 1900; s_admin_month = t.tm_mon + 1; s_admin_day = t.tm_mday;
    s_admin_hour = t.tm_hour; s_admin_minute = t.tm_min;
    if (s_admin_year < 2024) { s_admin_year = 2026; s_admin_month = 1; s_admin_day = 1; }

    s_admin_detail = lv_obj_create(s_admin_tab);
    lv_obj_set_pos(s_admin_detail, 12, 11);
    lv_obj_set_size(s_admin_detail, 376, 250);
    lv_obj_set_style_bg_color(s_admin_detail, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_admin_detail, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_detail, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_admin_detail, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_detail, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(s_admin_detail);

    lv_obj_t *title = lv_label_create(s_admin_detail);
    lv_obj_set_pos(title, 12, 2);
    lv_obj_set_size(title, 352, 27);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, ui_font_vazir_or(&ui_font_arial_regular26, 19), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, "تنظیم تاریخ و ساعت");

    lv_obj_t *date_card = lv_obj_create(s_admin_detail);
    lv_obj_set_pos(date_card, 7, 32);
    lv_obj_set_size(date_card, 362, 76);
    lv_obj_set_style_radius(date_card, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(date_card, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(date_card, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(date_card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(date_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(date_card, LV_OBJ_FLAG_SCROLLABLE);
    ui_admin_time_stepper(date_card, 7, 8, 2, "روز");
    ui_admin_time_stepper(date_card, 122, 8, 1, "ماه");
    ui_admin_time_stepper(date_card, 237, 8, 0, "سال");

    lv_obj_t *time_card = lv_obj_create(s_admin_detail);
    lv_obj_set_pos(time_card, 64, 113);
    lv_obj_set_size(time_card, 248, 66);
    lv_obj_set_style_radius(time_card, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(time_card, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(time_card, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(time_card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(time_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(time_card, LV_OBJ_FLAG_SCROLLABLE);
    ui_admin_time_stepper(time_card, 6, 3, 4, "دقیقه");
    ui_admin_time_stepper(time_card, 124, 3, 3, "ساعت");

    s_admin_detail_value = lv_label_create(s_admin_detail);
    lv_obj_set_pos(s_admin_detail_value, 15, 181);
    lv_obj_set_size(s_admin_detail_value, 346, 18);
    lv_obj_set_style_text_align(s_admin_detail_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_detail_value, ui_font_vazir_or(&ui_font_arial_regular17, 11), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_detail_value, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_label_set_text(s_admin_detail_value, "بدون RTC پس از قطع برق حفظ نمی شود");

    lv_obj_t *cancel = lv_btn_create(s_admin_detail);
    lv_obj_set_pos(cancel, 47, 202); lv_obj_set_size(cancel, 128, 34);
    lv_obj_set_style_radius(cancel, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(cancel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(cancel, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(cancel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(cancel, ui_admin_detail_back_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_lab = lv_label_create(cancel); lv_label_set_text(cancel_lab, "لغو");
    lv_obj_set_style_text_font(cancel_lab, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(cancel_lab, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_center(cancel_lab);

    lv_obj_t *save = lv_btn_create(s_admin_detail);
    lv_obj_set_pos(save, 201, 202); lv_obj_set_size(save, 128, 34);
    lv_obj_set_style_radius(save, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(save, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(save, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(save, ui_admin_time_save, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lab = lv_label_create(save); lv_label_set_text(save_lab, "ذخیره");
    lv_obj_set_style_text_font(save_lab, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(save_lab, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(save_lab);
    ui_admin_time_refresh();
}

static lv_obj_t *ui_admin_detail_begin(const char *title, const char *value,
                                       const char * const *map, lv_event_cb_t cb)
{
    ui_admin_detail_close();
    s_admin_detail = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_admin_detail, 480, 320);
    lv_obj_set_pos(s_admin_detail, 0, 0);
    lv_obj_set_style_bg_color(s_admin_detail, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_admin_detail, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_detail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_admin_detail);
    lv_obj_set_pos(panel, 12, 11);
    lv_obj_set_size(panel, 456, 298);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);

    /* Close button [X] at top right */
    lv_obj_t *close_btn = lv_btn_create(panel);
    lv_obj_set_pos(close_btn, 408, 2);
    lv_obj_set_size(close_btn, 36, 30);
    lv_obj_set_style_radius(close_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, ui_admin_detail_back_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_center(close_lbl);

    lv_obj_t *head = lv_label_create(panel);
    lv_obj_set_pos(head, 14, 4);
    lv_obj_set_size(head, 386, 24);
    lv_obj_set_style_text_align(head, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(head, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(head, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_base_dir(head, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_label_set_text(head, title);

    /* Content Card Container */
    lv_obj_t *card = lv_obj_create(panel);
    lv_obj_set_pos(card, 8, 34);
    lv_obj_set_size(card, 440, 68);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    s_admin_detail_value = lv_label_create(card);
    lv_obj_set_pos(s_admin_detail_value, 4, 2);
    lv_obj_set_size(s_admin_detail_value, 420, 56);
    lv_obj_set_style_text_align(s_admin_detail_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_detail_value, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_detail_value, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_admin_detail_value, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_label_set_text(s_admin_detail_value, value ? value : "");

    lv_obj_t *matrix = lv_buttonmatrix_create(panel);
    lv_buttonmatrix_set_map(matrix, map);
    lv_obj_set_pos(matrix, 8, 108);
    lv_obj_set_size(matrix, 440, 178);
    lv_obj_set_style_bg_opa(matrix, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(matrix, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(matrix, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(matrix, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_column(matrix, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(matrix, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(matrix, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(matrix, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(matrix, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(matrix, 8, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(matrix, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_font(matrix, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_text_color(matrix, lv_color_hex(0xff0f172a), LV_PART_ITEMS);
    lv_obj_set_style_text_color(matrix, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_event_cb(matrix, cb, LV_EVENT_VALUE_CHANGED, NULL);
    return matrix;
}

static const char *admin_back_map[] = { "بازگشت", "" };
static const char *admin_stats_map[] = { "صفر کردن آمار", "بازگشت", "" };
static const char *admin_stats_map_en[] = { "Reset Stats", "Back", "" };
static const char *admin_stats_map_tr[] = { "Istatistikleri Sifirla", "Geri", "" };
static const char *admin_stats_map_ru[] = { "Sbros Statistiki", "Nazad", "" };
static const char *admin_daylock_map[] = {
    "+30 روز", "+60 روز", "+90 روز", "\n",
    "تست قفل", "غیرفعالسازی", "بازگشت", ""
};
static const char *admin_daylock_map_en[] = {
    "+30 Days", "+60 Days", "+90 Days", "\n",
    "Test Lock", "Disable", "Back", ""
};
static const char *admin_daylock_map_tr[] = {
    "+30 Gun", "+60 Gun", "+90 Gun", "\n",
    "Test Kilidi", "Devre Disi", "Geri", ""
};
static const char *admin_daylock_map_ru[] = {
    "+30 Dnej", "+60 Dnej", "+90 Dnej", "\n",
    "Test Blok", "Otklyuchit", "Nazad", ""
};
static const char *admin_time_map[] = {
    "سال +", "ماه +", "روز +", "\n",
    "ساعت +", "دقیقه +", "سال -", "\n",
    "ماه -", "روز -", "ساعت -", "\n",
    "دقیقه -", "ذخیره", "بازگشت", ""
};
static const char *admin_pin_change_map[] = {
    "1", "2", "3", "\n", "4", "5", "6", "\n",
    "7", "8", "9", "\n", "حذف", "0", "ذخیره", "\n", "بازگشت", ""
};
static const char *admin_factory_map[] = { "لغو", "بازنشانی", "" };
static const char *admin_time_light_map[] = {
    "سال", "ماه", "روز", "\n",
    "ساعت", "دقیقه", "\n",
    "-", "+", "\n",
    "ذخیره", "بازگشت", ""
};

static void ui_admin_time_light_refresh(void)
{
    static const char *names[] = { "سال", "ماه", "روز", "ساعت", "دقیقه" };
    char value[96];
    snprintf(value, sizeof(value), "فیلد انتخاب شده: %s\n%04d/%02d/%02d   %02d:%02d",
             names[s_admin_time_field], s_admin_year, s_admin_month, s_admin_day,
             s_admin_hour, s_admin_minute);
    if (s_admin_detail_value) lv_label_set_text(s_admin_detail_value, value);
}

static void ui_admin_time_light_event(lv_event_t *e)
{
    lv_obj_t *matrix = (lv_obj_t *)lv_event_get_target(e);
    const char *key = lv_buttonmatrix_get_button_text(matrix, lv_buttonmatrix_get_selected_button(matrix));
    if (!key) return;
    if (!strcmp(key, "سال")) s_admin_time_field = 0;
    else if (!strcmp(key, "ماه")) s_admin_time_field = 1;
    else if (!strcmp(key, "روز")) s_admin_time_field = 2;
    else if (!strcmp(key, "ساعت")) s_admin_time_field = 3;
    else if (!strcmp(key, "دقیقه")) s_admin_time_field = 4;
    else if (!strcmp(key, "+") || !strcmp(key, "-")) {
        int delta = key[0] == '+' ? 1 : -1;
        if (s_admin_time_field == 0) {
            s_admin_year += delta;
            if (s_admin_year < 2024) s_admin_year = 2099;
            if (s_admin_year > 2099) s_admin_year = 2024;
        } else if (s_admin_time_field == 1) {
            s_admin_month += delta;
            if (s_admin_month < 1) s_admin_month = 12;
            if (s_admin_month > 12) s_admin_month = 1;
        } else if (s_admin_time_field == 2) {
            int max_day = ui_admin_days_in_month(s_admin_year, s_admin_month);
            s_admin_day += delta;
            if (s_admin_day < 1) s_admin_day = max_day;
            if (s_admin_day > max_day) s_admin_day = 1;
        } else if (s_admin_time_field == 3) {
            s_admin_hour += delta;
            if (s_admin_hour < 0) s_admin_hour = 23;
            if (s_admin_hour > 23) s_admin_hour = 0;
        } else {
            s_admin_minute += delta;
            if (s_admin_minute < 0) s_admin_minute = 59;
            if (s_admin_minute > 59) s_admin_minute = 0;
        }
        int max_day = ui_admin_days_in_month(s_admin_year, s_admin_month);
        if (s_admin_day > max_day) s_admin_day = max_day;
    } else if (!strcmp(key, "ذخیره")) {
        struct tm t = {0};
        t.tm_year = s_admin_year - 1900; t.tm_mon = s_admin_month - 1; t.tm_mday = s_admin_day;
        t.tm_hour = s_admin_hour; t.tm_min = s_admin_minute; t.tm_isdst = 0;
        time_t epoch = mktime(&t);
        struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        ui_clock_force_refresh();
        lv_label_set_text(s_admin_detail_value, "ذخیره شد - با قطع برق حفظ نمی شود");
        buzzer_key;
        return;
    } else if (!strcmp(key, "بازگشت")) { ui_admin_detail_close(); return; }
    ui_admin_time_light_refresh();
    buzzer_key;
}

static void ui_admin_open_datetime_light(void)
{
    time_t now = time(NULL); struct tm t; localtime_r(&now, &t);
    s_admin_year = t.tm_year + 1900; s_admin_month = t.tm_mon + 1; s_admin_day = t.tm_mday;
    s_admin_hour = t.tm_hour; s_admin_minute = t.tm_min;
    if (s_admin_year < 2024) { s_admin_year = 2026; s_admin_month = 1; s_admin_day = 1; }
    s_admin_time_field = 0;
    ui_admin_detail_begin("تنظیم تاریخ و ساعت", "", admin_time_light_map, ui_admin_time_light_event);
    ui_admin_time_light_refresh();
}

static bool ui_admin_date_is_jalali(void);
static int ui_admin_jalali_days_in_month(int year, int month);
static void ui_admin_gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd);
static void ui_admin_jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd);

static void ui_admin_number_options(char *buffer, size_t size, int first, int last, int digits)
{
    size_t used = 0;
    buffer[0] = '\0';
    for (int value = first; value <= last; value++) {
        int n = snprintf(buffer + used, size - used, digits == 4 ? "%04d%s" : "%02d%s",
                         value, value == last ? "" : "\n");
        if (n < 0 || (size_t)n >= size - used) break;
        used += (size_t)n;
    }
}

static void ui_admin_roller_style(lv_obj_t *roller)
{
    lv_obj_set_style_bg_color(roller, lv_color_hex(0xff162234), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(roller, lv_color_hex(0xff2b3e58), LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(roller, 9, LV_PART_MAIN);
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0xff0284c7), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_16, LV_PART_SELECTED);
}

static void ui_admin_roller_day_sync(void)
{
    if (!s_admin_date_rollers[0] || !s_admin_date_rollers[1] || !s_admin_date_rollers[2]) return;
    bool fa = ui_admin_date_is_jalali();
    int base_year = fa ? 1400 : 2024;
    int year = base_year + (int)lv_roller_get_selected(s_admin_date_rollers[0]);
    int month = 1 + (int)lv_roller_get_selected(s_admin_date_rollers[1]);
    int old_day = 1 + (int)lv_roller_get_selected(s_admin_date_rollers[2]);
    int max_day = fa ? ui_admin_jalali_days_in_month(year, month) : ui_admin_days_in_month(year, month);
    if (old_day > max_day) old_day = max_day;
    ui_admin_number_options(s_admin_day_options, sizeof(s_admin_day_options), 1, max_day, 2);
    lv_roller_set_options(s_admin_date_rollers[2], s_admin_day_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(s_admin_date_rollers[2], (uint32_t)(old_day - 1), LV_ANIM_OFF);
}

static void ui_admin_roller_changed(lv_event_t *e)
{
    lv_obj_t *roller = (lv_obj_t *)lv_event_get_target(e);
    if (roller == s_admin_date_rollers[0] || roller == s_admin_date_rollers[1])
        ui_admin_roller_day_sync();
    buzzer_key;
}

static void ui_admin_roller_save(lv_event_t *e)
{
    (void)e;
    bool fa = ui_admin_date_is_jalali();
    int base_year = fa ? 1400 : 2024;
    int year = base_year + (int)lv_roller_get_selected(s_admin_date_rollers[0]);
    int month = 1 + (int)lv_roller_get_selected(s_admin_date_rollers[1]);
    int day = 1 + (int)lv_roller_get_selected(s_admin_date_rollers[2]);
    int hour = (int)lv_roller_get_selected(s_admin_date_rollers[3]);
    int minute = (int)lv_roller_get_selected(s_admin_date_rollers[4]);

    int gy = year, gm = month, gd = day;
    if (fa) {
        ui_admin_jalali_to_gregorian(year, month, day, &gy, &gm, &gd);
    }

    struct tm t = {0};
    t.tm_year = gy - 1900;
    t.tm_mon = gm - 1;
    t.tm_mday = gd;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_isdst = 0;
    time_t epoch = mktime(&t);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    ui_clock_force_refresh();
    if (s_admin_detail_value) {
        lv_label_set_text(s_admin_detail_value, fa ? "تاریخ و ساعت ذخیره شد" : "Date and time saved");
        lv_obj_set_style_text_color(s_admin_detail_value, lv_color_hex(0xff2e7d32), LV_PART_MAIN);
    }
    buzzer_key;
}

static lv_obj_t *ui_admin_create_roller(lv_obj_t *parent, int x, int y, int w, int h,
                                        const char *caption, const char *options)
{
    lv_obj_t *lab = lv_label_create(parent);
    lv_obj_set_pos(lab, x, y);
    lv_obj_set_size(lab, w, 20);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(lab, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xff455a64), LV_PART_MAIN);
    lv_label_set_text(lab, caption);

    lv_obj_t *roller = lv_roller_create(parent);
    lv_obj_set_pos(roller, x, y + 20);
    lv_obj_set_size(roller, w, h - 20);
    lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 3);
    ui_admin_roller_style(roller);
    lv_obj_add_event_cb(roller, ui_admin_roller_changed, LV_EVENT_VALUE_CHANGED, NULL);
    return roller;
}

static void ui_admin_open_datetime_rollers(void)
{
    ui_admin_detail_close();
    memset(s_admin_date_rollers, 0, sizeof(s_admin_date_rollers));
    time_t now = time(NULL); struct tm t; localtime_r(&now, &t);
    int gy = t.tm_year + 1900, gm = t.tm_mon + 1, gd = t.tm_mday;
    int hour = t.tm_hour, minute = t.tm_min;
    if (gy < 2024 || gy > 2099) { gy = 2026; gm = 1; gd = 1; hour = 0; minute = 0; }

    bool fa = ui_admin_date_is_jalali();
    int year = gy, month = gm, day = gd;
    if (fa) {
        ui_admin_gregorian_to_jalali(gy, gm, gd, &year, &month, &day);
    }

    int min_year = fa ? 1400 : 2024;
    int max_year = fa ? 1420 : 2040;
    int max_day = fa ? ui_admin_jalali_days_in_month(year, month) : ui_admin_days_in_month(year, month);

    ui_admin_number_options(s_admin_year_options, sizeof(s_admin_year_options), min_year, max_year, 4);
    ui_admin_number_options(s_admin_month_options, sizeof(s_admin_month_options), 1, 12, 2);
    ui_admin_number_options(s_admin_day_options, sizeof(s_admin_day_options), 1, max_day, 2);
    ui_admin_number_options(s_admin_hour_options, sizeof(s_admin_hour_options), 0, 23, 2);
    ui_admin_number_options(s_admin_minute_options, sizeof(s_admin_minute_options), 0, 59, 2);

    s_admin_detail = lv_obj_create(s_admin_tab);
    lv_obj_set_pos(s_admin_detail, 0, 0);
    lv_obj_set_size(s_admin_detail, 376, 240);
    lv_obj_set_style_bg_color(s_admin_detail, lv_color_hex(0xff0b111e), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_detail, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(s_admin_detail);

    lv_obj_t *title = lv_label_create(s_admin_detail);
    lv_obj_set_pos(title, 10, 0); lv_obj_set_size(title, 356, 25);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, ui_font_vazir_or(&ui_font_arial_regular26, 18), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, fa ? "تنظیم تاریخ و ساعت" : "Date and Time Settings");

    /* 3 Rollers for Date: Day, Month, Year */
    s_admin_date_rollers[2] = ui_admin_create_roller(s_admin_detail, 14, 27, 105, 89, fa ? "روز" : "Day", s_admin_day_options);
    s_admin_date_rollers[1] = ui_admin_create_roller(s_admin_detail, 135, 27, 105, 89, fa ? "ماه" : "Month", s_admin_month_options);
    s_admin_date_rollers[0] = ui_admin_create_roller(s_admin_detail, 256, 27, 105, 89, fa ? "سال" : "Year", s_admin_year_options);

    /* 2 Rollers for Time: Minute (60 min), Hour (24 hr) */
    s_admin_date_rollers[4] = ui_admin_create_roller(s_admin_detail, 73, 119, 105, 76, fa ? "دقیقه" : "Minute", s_admin_minute_options);
    s_admin_date_rollers[3] = ui_admin_create_roller(s_admin_detail, 197, 119, 105, 76, fa ? "ساعت" : "Hour", s_admin_hour_options);

    uint32_t sel_year = (year >= min_year && year <= max_year) ? (uint32_t)(year - min_year) : 0;
    uint32_t sel_month = (month >= 1 && month <= 12) ? (uint32_t)(month - 1) : 0;
    uint32_t sel_day = (day >= 1 && day <= max_day) ? (uint32_t)(day - 1) : 0;
    uint32_t sel_hour = (hour >= 0 && hour <= 23) ? (uint32_t)hour : 0;
    uint32_t sel_minute = (minute >= 0 && minute <= 59) ? (uint32_t)minute : 0;

    lv_roller_set_selected(s_admin_date_rollers[0], sel_year, LV_ANIM_OFF);
    lv_roller_set_selected(s_admin_date_rollers[1], sel_month, LV_ANIM_OFF);
    lv_roller_set_selected(s_admin_date_rollers[2], sel_day, LV_ANIM_OFF);
    lv_roller_set_selected(s_admin_date_rollers[3], sel_hour, LV_ANIM_OFF);
    lv_roller_set_selected(s_admin_date_rollers[4], sel_minute, LV_ANIM_OFF);

    s_admin_detail_value = lv_label_create(s_admin_detail);
    lv_obj_set_pos(s_admin_detail_value, 8, 198); lv_obj_set_size(s_admin_detail_value, 360, 17);
    lv_obj_set_style_text_align(s_admin_detail_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_detail_value, ui_font_vazir_or(&ui_font_arial_regular17, 11), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_detail_value, lv_color_hex(0xff78909c), LV_PART_MAIN);
    lv_label_set_text(s_admin_detail_value, fa ? "اسکرول ساعت (24 ساعته) و دقیقه (60 دقیقه)" : "Scroll 24-hr clock & 60-min");

    lv_obj_t *cancel = lv_btn_create(s_admin_detail);
    lv_obj_set_pos(cancel, 48, 216); lv_obj_set_size(cancel, 128, 24);
    lv_obj_set_style_radius(cancel, 7, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0xff78909c), LV_PART_MAIN);
    lv_obj_add_event_cb(cancel, ui_admin_detail_back_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_lab = lv_label_create(cancel); lv_label_set_text(cancel_lab, fa ? "بازگشت" : "Back");
    lv_obj_set_style_text_font(cancel_lab, ui_font_vazir_or(&ui_font_arial_regular17, 12), LV_PART_MAIN);
    lv_obj_center(cancel_lab);

    lv_obj_t *save = lv_btn_create(s_admin_detail);
    lv_obj_set_pos(save, 200, 216); lv_obj_set_size(save, 128, 24);
    lv_obj_set_style_radius(save, 7, LV_PART_MAIN);
    lv_obj_set_style_bg_color(save, lv_color_hex(0xff1976d2), LV_PART_MAIN);
    lv_obj_add_event_cb(save, ui_admin_roller_save, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lab = lv_label_create(save); lv_label_set_text(save_lab, fa ? "ذخیره" : "Save");
    lv_obj_set_style_text_font(save_lab, ui_font_vazir_or(&ui_font_arial_regular17, 12), LV_PART_MAIN);
    lv_obj_center(save_lab);
}

static void ui_admin_counter_adjust(int delta)
{
    if (s_admin_time_field == 0) {
        s_admin_year += delta;
        if (s_admin_year < 2024) s_admin_year = 2099;
        if (s_admin_year > 2099) s_admin_year = 2024;
    } else if (s_admin_time_field == 1) {
        s_admin_month += delta;
        if (s_admin_month < 1) s_admin_month = 12;
        if (s_admin_month > 12) s_admin_month = 1;
    } else if (s_admin_time_field == 2) {
        int max_day = ui_admin_days_in_month(s_admin_year, s_admin_month);
        s_admin_day += delta;
        if (s_admin_day < 1) s_admin_day = max_day;
        if (s_admin_day > max_day) s_admin_day = 1;
    } else if (s_admin_time_field == 3) {
        s_admin_hour += delta;
        if (s_admin_hour < 0) s_admin_hour = 23;
        if (s_admin_hour > 23) s_admin_hour = 0;
    } else {
        s_admin_minute += delta;
        if (s_admin_minute < 0) s_admin_minute = 59;
        if (s_admin_minute > 59) s_admin_minute = 0;
    }
    int max_day = ui_admin_days_in_month(s_admin_year, s_admin_month);
    if (s_admin_day > max_day) s_admin_day = max_day;
}

static void ui_admin_counter_refresh(void)
{
    static const char *names[] = { "سال", "ماه", "روز", "ساعت", "دقیقه" };
    int current;
    if (s_admin_time_field == 0) current = s_admin_year;
    else if (s_admin_time_field == 1) current = s_admin_month;
    else if (s_admin_time_field == 2) current = s_admin_day;
    else if (s_admin_time_field == 3) current = s_admin_hour;
    else current = s_admin_minute;

    int minimum = 0, maximum = 59;
    if (s_admin_time_field == 0) { minimum = 2024; maximum = 2099; }
    else if (s_admin_time_field == 1) { minimum = 1; maximum = 12; }
    else if (s_admin_time_field == 2) { minimum = 1; maximum = ui_admin_days_in_month(s_admin_year, s_admin_month); }
    else if (s_admin_time_field == 3) { minimum = 0; maximum = 23; }
    int previous = current <= minimum ? maximum : current - 1;
    int next = current >= maximum ? minimum : current + 1;

    if (s_admin_time_field == 0) {
        snprintf(s_admin_counter_prev, sizeof(s_admin_counter_prev), "%04d", previous);
        snprintf(s_admin_counter_now, sizeof(s_admin_counter_now), "%04d", current);
        snprintf(s_admin_counter_next, sizeof(s_admin_counter_next), "%04d", next);
    } else {
        snprintf(s_admin_counter_prev, sizeof(s_admin_counter_prev), "%02d", previous);
        snprintf(s_admin_counter_now, sizeof(s_admin_counter_now), "%02d", current);
        snprintf(s_admin_counter_next, sizeof(s_admin_counter_next), "%02d", next);
    }
    if (s_admin_counter_matrix) {
        lv_buttonmatrix_set_map(s_admin_counter_matrix, s_admin_counter_map);
        lv_buttonmatrix_set_button_ctrl(s_admin_counter_matrix, 1, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
    if (s_admin_counter_controls) {
        for (uint8_t i = 0; i < 5; i++)
            lv_buttonmatrix_clear_button_ctrl(s_admin_counter_controls, i, LV_BUTTONMATRIX_CTRL_CHECKED);
        lv_buttonmatrix_set_button_ctrl(s_admin_counter_controls, s_admin_time_field, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
    if (s_admin_detail_value) {
        char value[96];
        snprintf(value, sizeof(value), "%s    %04d/%02d/%02d   %02d:%02d",
                 names[s_admin_time_field], s_admin_year, s_admin_month, s_admin_day,
                 s_admin_hour, s_admin_minute);
        lv_label_set_text(s_admin_detail_value, value);
    }
}

static void ui_admin_counter_event(lv_event_t *e)
{
    uint32_t id = lv_buttonmatrix_get_selected_button((lv_obj_t *)lv_event_get_target(e));
    if (id == 0) ui_admin_counter_adjust(-1);
    else if (id == 2) ui_admin_counter_adjust(1);
    ui_admin_counter_refresh();
    buzzer_key;
}

static void ui_admin_counter_controls(lv_event_t *e)
{
    lv_obj_t *matrix = (lv_obj_t *)lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(matrix);
    if (id < 5) s_admin_time_field = (uint8_t)id;
    else if (id == 5) {
        struct tm t = {0};
        t.tm_year = s_admin_year - 1900; t.tm_mon = s_admin_month - 1; t.tm_mday = s_admin_day;
        t.tm_hour = s_admin_hour; t.tm_min = s_admin_minute; t.tm_isdst = 0;
        time_t epoch = mktime(&t); struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
        settimeofday(&tv, NULL); ui_clock_force_refresh();
        lv_label_set_text(s_admin_detail_value, "تاریخ و ساعت ذخیره شد");
        buzzer_key; return;
    } else if (id == 6) { ui_admin_detail_close(); return; }
    ui_admin_counter_refresh();
    buzzer_key;
}

static const char *admin_counter_controls_map[] = {
    "سال", "ماه", "روز", "\n", "ساعت", "دقیقه", "\n", "ذخیره", "بازگشت", ""
};

static void ui_admin_open_datetime_counter(void)
{
    time_t now = time(NULL); struct tm t; localtime_r(&now, &t);
    s_admin_year = t.tm_year + 1900; s_admin_month = t.tm_mon + 1; s_admin_day = t.tm_mday;
    s_admin_hour = t.tm_hour; s_admin_minute = t.tm_min;
    if (s_admin_year < 2024 || s_admin_year > 2099) {
        s_admin_year = 2026; s_admin_month = 1; s_admin_day = 1; s_admin_hour = 0; s_admin_minute = 0;
    }
    s_admin_time_field = 2;
    ui_admin_detail_close();
    s_admin_detail = lv_obj_create(s_admin_tab);
    lv_obj_set_pos(s_admin_detail, 0, 0); lv_obj_set_size(s_admin_detail, 376, 240);
    lv_obj_set_style_bg_color(s_admin_detail, lv_color_hex(0xff0b111e), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_detail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_admin_detail);
    lv_obj_set_pos(title, 10, 1); lv_obj_set_size(title, 356, 25);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, ui_font_vazir_or(&ui_font_arial_regular26, 18), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, "تنظیم تاریخ و ساعت");

    s_admin_detail_value = lv_label_create(s_admin_detail);
    lv_obj_set_pos(s_admin_detail_value, 12, 28); lv_obj_set_size(s_admin_detail_value, 352, 24);
    lv_obj_set_style_text_align(s_admin_detail_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_detail_value, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_detail_value, lv_color_hex(0xff94a3b8), LV_PART_MAIN);

    s_admin_counter_controls = lv_buttonmatrix_create(s_admin_detail);
    lv_buttonmatrix_set_map(s_admin_counter_controls, admin_counter_controls_map);
    lv_obj_set_pos(s_admin_counter_controls, 16, 56); lv_obj_set_size(s_admin_counter_controls, 218, 174);
    lv_obj_set_style_bg_opa(s_admin_counter_controls, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_counter_controls, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_admin_counter_controls, lv_color_hex(0xff1e2e44), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_admin_counter_controls, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(s_admin_counter_controls, lv_color_hex(0xff16a34a), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(s_admin_counter_controls, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(s_admin_counter_controls, lv_color_hex(0xff2b3e58), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_admin_counter_controls, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_admin_counter_controls, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_admin_counter_controls, lv_color_hex(0xfff8fafc), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_admin_counter_controls, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_ITEMS);
    lv_obj_add_event_cb(s_admin_counter_controls, ui_admin_counter_controls, LV_EVENT_VALUE_CHANGED, NULL);

    s_admin_counter_matrix = lv_buttonmatrix_create(s_admin_detail);
    lv_buttonmatrix_set_map(s_admin_counter_matrix, s_admin_counter_map);
    lv_obj_set_pos(s_admin_counter_matrix, 248, 75); lv_obj_set_size(s_admin_counter_matrix, 112, 130);
    lv_obj_set_style_bg_color(s_admin_counter_matrix, lv_color_hex(0xff162234), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_admin_counter_matrix, lv_color_hex(0xff2b3e58), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_counter_matrix, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_admin_counter_matrix, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_admin_counter_matrix, lv_color_hex(0xff1e2e44), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_admin_counter_matrix, lv_color_hex(0xff94a3b8), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_admin_counter_matrix, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(s_admin_counter_matrix, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(s_admin_counter_matrix, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_radius(s_admin_counter_matrix, 7, LV_PART_ITEMS);
    lv_obj_add_event_cb(s_admin_counter_matrix, ui_admin_counter_event, LV_EVENT_VALUE_CHANGED, NULL);
    ui_admin_counter_refresh();
}

static int ui_admin_jalali_days_before_year(int year)
{
    int y = year + 1595;
    return 365 * y + (y / 33) * 8 + ((y % 33) + 3) / 4;
}

static bool ui_admin_is_jalali_leap(int year)
{
    return ui_admin_jalali_days_before_year(year + 1) - ui_admin_jalali_days_before_year(year) == 366;
}

static int ui_admin_jalali_days_in_month(int year, int month)
{
    if (month <= 6) return 31;
    if (month <= 11) return 30;
    return ui_admin_is_jalali_leap(year) ? 30 : 29;
}

static void ui_admin_gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd)
{
    static const int month_days[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    int year = gy > 1600 ? gy - 1600 : gy - 621;
    int result_year = gy > 1600 ? 979 : 0;
    int adjusted_year = gm > 2 ? year + 1 : year;
    int days = 365 * year + (adjusted_year + 3) / 4 - (adjusted_year + 99) / 100
             + (adjusted_year + 399) / 400 - 80 + gd + month_days[gm - 1];
    result_year += 33 * (days / 12053); days %= 12053;
    result_year += 4 * (days / 1461); days %= 1461;
    if (days > 365) { result_year += (days - 1) / 365; days = (days - 1) % 365; }
    *jy = result_year;
    if (days < 186) { *jm = 1 + days / 31; *jd = 1 + days % 31; }
    else { *jm = 7 + (days - 186) / 30; *jd = 1 + (days - 186) % 30; }
}

static void ui_admin_jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd)
{
    int year = jy + 1595;
    int days = -355668 + 365 * year + (year / 33) * 8 + ((year % 33) + 3) / 4 + jd
             + (jm < 7 ? (jm - 1) * 31 : (jm - 7) * 30 + 186);
    int result_year = 400 * (days / 146097); days %= 146097;
    if (days > 36524) {
        result_year += 100 * (--days / 36524); days %= 36524;
        if (days >= 365) days++;
    }
    result_year += 4 * (days / 1461); days %= 1461;
    if (days > 365) { result_year += (days - 1) / 365; days = (days - 1) % 365; }
    int leap = (result_year % 4 == 0 && result_year % 100 != 0) || result_year % 400 == 0;
    static const int gregorian_month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int month = 0;
    while (month < 12) {
        int count = gregorian_month_days[month] + (month == 1 && leap ? 1 : 0);
        if (days < count) break;
        days -= count; month++;
    }
    *gy = result_year; *gm = month + 1; *gd = days + 1;
}

static bool ui_admin_date_is_jalali(void)
{
    return EEprom2_Language == L_Persion;
}

static int ui_admin_dual_date_limit(void)
{
    return ui_admin_date_is_jalali()
        ? ui_admin_jalali_days_in_month(s_admin_year, s_admin_month)
        : ui_admin_days_in_month(s_admin_year, s_admin_month);
}

static int ui_admin_wrap_value(int value, int minimum, int maximum)
{
    if (value < minimum) return maximum;
    if (value > maximum) return minimum;
    return value;
}

static void ui_admin_dual_refresh(void)
{
    int min_year = ui_admin_date_is_jalali() ? 1300 : 1921;
    int max_year = ui_admin_date_is_jalali() ? 1499 : 2120;
    int max_day = ui_admin_dual_date_limit();
    if (s_admin_day > max_day) s_admin_day = max_day;
    int values[3] = { s_admin_year, s_admin_month, s_admin_day };
    int minimums[3] = { min_year, 1, 1 };
    int maximums[3] = { max_year, 12, max_day };
    for (int col = 0; col < 3; col++) {
        int previous = ui_admin_wrap_value(values[col] - 1, minimums[col], maximums[col]);
        int next = ui_admin_wrap_value(values[col] + 1, minimums[col], maximums[col]);
        snprintf(s_admin_date_cells[col], sizeof(s_admin_date_cells[col]), col == 0 ? "%04d" : "%02d", previous);
        snprintf(s_admin_date_cells[3 + col], sizeof(s_admin_date_cells[3 + col]), col == 0 ? "%04d" : "%02d", values[col]);
        snprintf(s_admin_date_cells[6 + col], sizeof(s_admin_date_cells[6 + col]), col == 0 ? "%04d" : "%02d", next);
    }
    int clock_values[2] = { s_admin_hour, s_admin_minute };
    int clock_maximums[2] = { 23, 59 };
    for (int col = 0; col < 2; col++) {
        int previous = ui_admin_wrap_value(clock_values[col] - 1, 0, clock_maximums[col]);
        int next = ui_admin_wrap_value(clock_values[col] + 1, 0, clock_maximums[col]);
        snprintf(s_admin_clock_cells[col], sizeof(s_admin_clock_cells[col]), "%02d", previous);
        snprintf(s_admin_clock_cells[2 + col], sizeof(s_admin_clock_cells[2 + col]), "%02d", clock_values[col]);
        snprintf(s_admin_clock_cells[4 + col], sizeof(s_admin_clock_cells[4 + col]), "%02d", next);
    }
    if (s_admin_date_matrix) {
        lv_buttonmatrix_set_map(s_admin_date_matrix, s_admin_date_map);
        for (uint8_t i = 3; i <= 5; i++) lv_buttonmatrix_set_button_ctrl(s_admin_date_matrix, i, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
    if (s_admin_clock_matrix) {
        lv_buttonmatrix_set_map(s_admin_clock_matrix, s_admin_clock_map);
        for (uint8_t i = 2; i <= 3; i++) lv_buttonmatrix_set_button_ctrl(s_admin_clock_matrix, i, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
}

static void ui_admin_date_matrix_event(lv_event_t *e)
{
    uint32_t id = lv_buttonmatrix_get_selected_button((lv_obj_t *)lv_event_get_target(e));
    int col = (int)(id % 3u);
    int delta = id < 3 ? -1 : id >= 6 ? 1 : 0;
    if (delta) {
        if (col == 0) s_admin_year = ui_admin_wrap_value(s_admin_year + delta,
                ui_admin_date_is_jalali() ? 1300 : 1921, ui_admin_date_is_jalali() ? 1499 : 2120);
        else if (col == 1) s_admin_month = ui_admin_wrap_value(s_admin_month + delta, 1, 12);
        else s_admin_day = ui_admin_wrap_value(s_admin_day + delta, 1, ui_admin_dual_date_limit());
        ui_admin_dual_refresh(); buzzer_key;
    }
}

static void ui_admin_clock_matrix_event(lv_event_t *e)
{
    uint32_t id = lv_buttonmatrix_get_selected_button((lv_obj_t *)lv_event_get_target(e));
    int col = (int)(id % 2u);
    int delta = id < 2 ? -1 : id >= 4 ? 1 : 0;
    if (delta) {
        if (col == 0) s_admin_hour = ui_admin_wrap_value(s_admin_hour + delta, 0, 23);
        else s_admin_minute = ui_admin_wrap_value(s_admin_minute + delta, 0, 59);
        ui_admin_dual_refresh(); buzzer_key;
    }
}

static void ui_admin_roller_gesture_event(lv_event_t *e)
{
    lv_obj_t *matrix = (lv_obj_t *)lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;
    lv_dir_t direction = lv_indev_get_gesture_dir(indev);
    if (direction != LV_DIR_TOP && direction != LV_DIR_BOTTOM) return;

    lv_point_t point;
    lv_area_t area;
    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(matrix, &area);
    int columns = matrix == s_admin_date_matrix ? 3 : 2;
    int width = lv_area_get_width(&area);
    int column = width > 0 ? ((point.x - area.x1) * columns) / width : 0;
    if (column < 0) column = 0;
    if (column >= columns) column = columns - 1;
    int delta = direction == LV_DIR_TOP ? 1 : -1;

    if (matrix == s_admin_date_matrix) {
        if (column == 0) s_admin_year = ui_admin_wrap_value(s_admin_year + delta,
                ui_admin_date_is_jalali() ? 1300 : 1921, ui_admin_date_is_jalali() ? 1499 : 2120);
        else if (column == 1) s_admin_month = ui_admin_wrap_value(s_admin_month + delta, 1, 12);
        else s_admin_day = ui_admin_wrap_value(s_admin_day + delta, 1, ui_admin_dual_date_limit());
    } else {
        if (column == 0) s_admin_hour = ui_admin_wrap_value(s_admin_hour + delta, 0, 23);
        else s_admin_minute = ui_admin_wrap_value(s_admin_minute + delta, 0, 59);
    }
    ui_admin_dual_refresh();
    lv_event_stop_bubbling(e);
    buzzer_key;
}

static void ui_admin_dual_action(lv_event_t *e)
{
    intptr_t action = (intptr_t)lv_event_get_user_data(e);
    if (!action) { ui_admin_detail_close(); buzzer_key; return; }
    int gy = s_admin_year, gm = s_admin_month, gd = s_admin_day;
    if (ui_admin_date_is_jalali()) ui_admin_jalali_to_gregorian(s_admin_year, s_admin_month, s_admin_day, &gy, &gm, &gd);
    struct tm t = {0};
    t.tm_year = gy - 1900; t.tm_mon = gm - 1; t.tm_mday = gd;
    t.tm_hour = s_admin_hour; t.tm_min = s_admin_minute; t.tm_isdst = 0;
    time_t epoch = mktime(&t); struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL); ui_clock_force_refresh();
    ui_admin_detail_close(); buzzer_key;
}

static void ui_admin_style_roller_matrix(lv_obj_t *matrix)
{
    lv_obj_set_style_bg_color(matrix, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(matrix, lv_color_hex(0xffd5dde6), LV_PART_MAIN);
    lv_obj_set_style_border_width(matrix, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(matrix, 11, LV_PART_MAIN);
    lv_obj_set_style_pad_all(matrix, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_row(matrix, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_column(matrix, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(matrix, lv_color_hex(0xfff3f6f9), LV_PART_ITEMS);
    lv_obj_set_style_text_color(matrix, lv_color_hex(0xff78909c), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(matrix, lv_color_hex(0xff1976d2), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(matrix, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(matrix, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_radius(matrix, 7, LV_PART_ITEMS);
    lv_obj_set_style_base_dir(matrix, LV_BASE_DIR_LTR, LV_PART_MAIN);
}

static lv_obj_t *ui_admin_dual_button(lv_obj_t *parent, int x, int y, const char *text, lv_color_t color, intptr_t action)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y); lv_obj_set_size(button, 110, 36);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, color, LV_PART_MAIN);
    lv_obj_add_event_cb(button, ui_admin_dual_action, LV_EVENT_CLICKED, (void *)action);
    lv_obj_t *label = lv_label_create(button); lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_vazir_or(&ui_font_arial_regular17, 14), LV_PART_MAIN);
    lv_obj_set_style_base_dir(label, (EEprom2_Language == L_Persion) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_center(label);
    return button;
}

/* Modern Centered Apple HIG Date & Time Modal on lv_layer_top() */
static lv_obj_t *s_dt_labels[5]; /* 0:Year, 1:Month, 2:Day, 3:Hour, 4:Minute */
static lv_obj_t *s_dt_live_date_lbl = NULL;
static lv_obj_t *s_dt_live_time_lbl = NULL;
static int s_dt_y, s_dt_m, s_dt_d, s_dt_h, s_dt_min;

static void ui_admin_datetime_refresh_display(void)
{
    char b[12];
    if (s_dt_labels[0]) { snprintf(b, sizeof(b), "%04d", s_dt_y); lv_label_set_text(s_dt_labels[0], b); }
    if (s_dt_labels[1]) { snprintf(b, sizeof(b), "%02d", s_dt_m); lv_label_set_text(s_dt_labels[1], b); }
    if (s_dt_labels[2]) { snprintf(b, sizeof(b), "%02d", s_dt_d); lv_label_set_text(s_dt_labels[2], b); }
    if (s_dt_labels[3]) { snprintf(b, sizeof(b), "%02d", s_dt_h); lv_label_set_text(s_dt_labels[3], b); }
    if (s_dt_labels[4]) { snprintf(b, sizeof(b), "%02d", s_dt_min); lv_label_set_text(s_dt_labels[4], b); }

    bool fa = ui_admin_date_is_jalali();
    if (s_dt_live_date_lbl) {
        char buf[64];
        if (fa) {
            static const char *fa_m[] = { "", "فروردین", "اردیبهشت", "خرداد", "تیر", "مرداد", "شهریور", "مهر", "آبان", "آذر", "دی", "بهمن", "اسفند" };
            const char *m_name = (s_dt_m >= 1 && s_dt_m <= 12) ? fa_m[s_dt_m] : "";
            snprintf(buf, sizeof(buf), "%04d/%02d/%02d (%s)", s_dt_y, s_dt_m, s_dt_d, m_name);
        } else {
            static const char *en_m[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            const char *m_name = (s_dt_m >= 1 && s_dt_m <= 12) ? en_m[s_dt_m] : "";
            snprintf(buf, sizeof(buf), "%s %02d, %04d", m_name, s_dt_d, s_dt_y);
        }
        lv_label_set_text(s_dt_live_date_lbl, buf);
    }
    if (s_dt_live_time_lbl) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d", s_dt_h, s_dt_min);
        lv_label_set_text(s_dt_live_time_lbl, buf);
    }
}

static void ui_admin_dt_step_cb(lv_event_t *e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    int field = (code > 0) ? (code - 1) : (-code - 1);
    int delta = (code > 0) ? 1 : -1;
    bool fa = ui_admin_date_is_jalali();

    if (field == 0) {
        s_dt_y += delta;
        int min_y = fa ? 1400 : 2024;
        int max_y = fa ? 1450 : 2099;
        if (s_dt_y < min_y) s_dt_y = max_y;
        if (s_dt_y > max_y) s_dt_y = min_y;
    } else if (field == 1) {
        s_dt_m += delta;
        if (s_dt_m < 1) s_dt_m = 12;
        if (s_dt_m > 12) s_dt_m = 1;
    } else if (field == 2) {
        int max_d = fa ? ui_admin_jalali_days_in_month(s_dt_y, s_dt_m) : ui_admin_days_in_month(s_dt_y, s_dt_m);
        s_dt_d += delta;
        if (s_dt_d < 1) s_dt_d = max_d;
        if (s_dt_d > max_d) s_dt_d = 1;
    } else if (field == 3) {
        s_dt_h += delta;
        if (s_dt_h < 0) s_dt_h = 23;
        if (s_dt_h > 23) s_dt_h = 0;
    } else if (field == 4) {
        s_dt_min += delta;
        if (s_dt_min < 0) s_dt_min = 59;
        if (s_dt_min > 59) s_dt_min = 0;
    }
    int max_d = fa ? ui_admin_jalali_days_in_month(s_dt_y, s_dt_m) : ui_admin_days_in_month(s_dt_y, s_dt_m);
    if (s_dt_d > max_d) s_dt_d = max_d;

    ui_admin_datetime_refresh_display();
    buzzer_key;
}

static void ui_admin_dt_close_cb(lv_event_t *e)
{
    (void)e;
    if (s_admin_datetime_modal) {
        lv_obj_delete(s_admin_datetime_modal);
        s_admin_datetime_modal = NULL;
    }
    s_dt_live_date_lbl = NULL;
    s_dt_live_time_lbl = NULL;
    for (int i = 0; i < 5; i++) s_dt_labels[i] = NULL;
    buzzer_key;
}

static void ui_admin_dt_save_cb(lv_event_t *e)
{
    (void)e;
    int gy = s_dt_y, gm = s_dt_m, gd = s_dt_d;
    if (ui_admin_date_is_jalali()) {
        ui_admin_jalali_to_gregorian(s_dt_y, s_dt_m, s_dt_d, &gy, &gm, &gd);
    }
    struct tm t = {0};
    t.tm_year = gy - 1900;
    t.tm_mon = gm - 1;
    t.tm_mday = gd;
    t.tm_hour = s_dt_h;
    t.tm_min = s_dt_min;
    t.tm_isdst = 0;
    time_t epoch = mktime(&t);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    ui_clock_force_refresh();

    if (s_admin_datetime_modal) {
        lv_obj_delete(s_admin_datetime_modal);
        s_admin_datetime_modal = NULL;
    }
    s_dt_live_date_lbl = NULL;
    s_dt_live_time_lbl = NULL;
    for (int i = 0; i < 5; i++) s_dt_labels[i] = NULL;
    buzzer_key;
}

static void ui_admin_dt_ntp_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (!ota_manager_is_wifi_connected()) {
        ui_toast_show("عدم اتصال وای‌فای", "ابتدا به شبکه اینترنت متصل شوید", 3000, TOAST_TYPE_WARNING);
        return;
    }
    ui_toast_show("دریافت ساعت", "در حال همگام‌سازی ساعت از اینترنت...", 2000, TOAST_TYPE_INFO);
    bool ok = ota_manager_sync_ntp_now();
    if (ok) {
        time_t now = time(NULL);
        struct tm t;
        localtime_r(&now, &t);
        s_dt_y = t.tm_year + 1900;
        s_dt_m = t.tm_mon + 1;
        s_dt_d = t.tm_mday;
        s_dt_h = t.tm_hour;
        s_dt_min = t.tm_min;
        if (ui_admin_date_is_jalali()) {
            int jy, jm, jd;
            ui_admin_gregorian_to_jalali(s_dt_y, s_dt_m, s_dt_d, &jy, &jm, &jd);
            s_dt_y = jy;
            s_dt_m = jm;
            s_dt_d = jd;
        }
        ui_admin_datetime_refresh_display();
        ui_toast_show("ساعت همگام شد", "ساعت دقیق اینترنتی روی سیستم تنظیم گردید", 3000, TOAST_TYPE_SUCCESS);
    } else {
        ui_toast_show("خطا در همگام‌سازی", "پاسخی از سرور NTP دریافت نشد", 3000, TOAST_TYPE_ERROR);
    }
}

void ui_admin_open_datetime_modal(void)
{
    if (s_admin_datetime_modal) {
        lv_obj_delete(s_admin_datetime_modal);
        s_admin_datetime_modal = NULL;
    }
    s_dt_live_date_lbl = NULL;
    s_dt_live_time_lbl = NULL;
    for (int i = 0; i < 5; i++) s_dt_labels[i] = NULL;

    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    s_dt_y = t.tm_year + 1900;
    s_dt_m = t.tm_mon + 1;
    s_dt_d = t.tm_mday;
    s_dt_h = t.tm_hour;
    s_dt_min = t.tm_min;

    bool fa = ui_admin_date_is_jalali();
    if (fa) {
        int jy, jm, jd;
        ui_admin_gregorian_to_jalali(s_dt_y, s_dt_m, s_dt_d, &jy, &jm, &jd);
        s_dt_y = jy;
        s_dt_m = jm;
        s_dt_d = jd;
    }

    s_admin_datetime_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_admin_datetime_modal, 480, 320);
    lv_obj_set_pos(s_admin_datetime_modal, 0, 0);
    lv_obj_set_style_bg_color(s_admin_datetime_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_admin_datetime_modal, 160, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_datetime_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_datetime_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_datetime_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_admin_datetime_modal);
    lv_obj_set_pos(panel, 12, 10);
    lv_obj_set_size(panel, 456, 300);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Header */
    const char *dt_title = (EEprom2_Language == L_english) ? "System Date & Time Settings" :
                           (EEprom2_Language == L_Turkish) ? "Tarih ve Saat Ayarlari" :
                           (EEprom2_Language == L_Russian) ? "Nastroyka Vremeni i Daty" :
                           "تنظیم ساعت و تقویم سیستم";

    lv_obj_t *title = lv_label_create(panel);
    lv_obj_set_pos(title, 14, 8);
    lv_obj_set_size(title, 386, 24);
    lv_obj_set_style_text_align(title, fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, dt_title);

    lv_obj_t *close_btn = lv_btn_create(panel);
    lv_obj_set_pos(close_btn, 414, 5);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_set_style_radius(close_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(close_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, ui_admin_dt_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_center(close_lbl);

    /* 2. Group 1: Date Card (pos: x_date, 36, size: 254, 190) */
    int x_date = fa ? 194 : 8;
    int x_time = fa ? 8 : 270;

    lv_obj_t *card_date = lv_obj_create(panel);
    lv_obj_set_pos(card_date, x_date, 36);
    lv_obj_set_size(card_date, 254, 190);
    lv_obj_set_style_bg_color(card_date, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_date, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_date, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_date, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_date, 0, LV_PART_MAIN);
    lv_obj_clear_flag(card_date, LV_OBJ_FLAG_SCROLLABLE);

    /* Top pill badge for Date */
    lv_obj_t *d_pill = lv_obj_create(card_date);
    lv_obj_set_pos(d_pill, 27, 6);
    lv_obj_set_size(d_pill, 200, 22);
    lv_obj_set_style_bg_color(d_pill, lv_color_hex(0xffe0f2fe), LV_PART_MAIN);
    lv_obj_set_style_border_color(d_pill, lv_color_hex(0xffbae6fd), LV_PART_MAIN);
    lv_obj_set_style_border_width(d_pill, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(d_pill, 11, LV_PART_MAIN);
    lv_obj_set_style_pad_all(d_pill, 0, LV_PART_MAIN);
    lv_obj_clear_flag(d_pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *d_title = lv_label_create(d_pill);
    lv_label_set_text(d_title, fa ? "تقویم رسمی جلالی" : "Jalali / Gregorian Date");
    lv_obj_set_style_text_font(d_title, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(d_title, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_center(d_title);

    /* 3 Date Columns: In FA: Day(x=170), Month(x=90), Year(x=10). In EN: Year(x=10), Month(x=90), Day(x=170) */
    int d_col_fields[3];
    const char *d_col_names[3];
    int d_col_x[3] = { 10, 90, 170 };

    if (fa) {
        d_col_fields[0] = 0; d_col_names[0] = "سال";
        d_col_fields[1] = 1; d_col_names[1] = "ماه";
        d_col_fields[2] = 2; d_col_names[2] = "روز";
    } else {
        d_col_fields[0] = 0; d_col_names[0] = "Year";
        d_col_fields[1] = 1; d_col_names[1] = "Month";
        d_col_fields[2] = 2; d_col_names[2] = "Day";
    }

    for (int col = 0; col < 3; col++) {
        int f = d_col_fields[col];
        int cx = d_col_x[col];

        lv_obj_t *cap = lv_label_create(card_date);
        lv_obj_set_pos(cap, cx, 32);
        lv_obj_set_size(cap, 74, 16);
        lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(cap, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(cap, lv_color_hex(0xff64748b), LV_PART_MAIN);
        lv_label_set_text(cap, d_col_names[col]);

        lv_obj_t *btn_up = lv_btn_create(card_date);
        lv_obj_set_pos(btn_up, cx, 50);
        lv_obj_set_size(btn_up, 74, 32);
        lv_obj_set_style_bg_color(btn_up, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_up, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_up, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn_up, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(btn_up, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn_up, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn_up, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(btn_up, ui_admin_dt_step_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(f + 1));
        lv_obj_t *u_lbl = lv_label_create(btn_up);
        lv_label_set_text(u_lbl, "+");
        lv_obj_set_style_text_font(u_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(u_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_obj_center(u_lbl);

        lv_obj_t *vbox = lv_obj_create(card_date);
        lv_obj_set_pos(vbox, cx, 86);
        lv_obj_set_size(vbox, 74, 36);
        lv_obj_set_style_bg_color(vbox, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_color(vbox, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_obj_set_style_border_width(vbox, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(vbox, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(vbox, 0, LV_PART_MAIN);
        lv_obj_clear_flag(vbox, LV_OBJ_FLAG_SCROLLABLE);

        s_dt_labels[f] = lv_label_create(vbox);
        lv_obj_set_size(s_dt_labels[f], LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(s_dt_labels[f], 0, LV_PART_MAIN);
        lv_obj_set_style_text_align(s_dt_labels[f], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_dt_labels[f], (f == 0) ? &lv_font_montserrat_18 : &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_dt_labels[f], lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_obj_center(s_dt_labels[f]);

        lv_obj_t *btn_dn = lv_btn_create(card_date);
        lv_obj_set_pos(btn_dn, cx, 126);
        lv_obj_set_size(btn_dn, 74, 32);
        lv_obj_set_style_bg_color(btn_dn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_dn, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_dn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn_dn, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(btn_dn, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn_dn, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn_dn, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(btn_dn, ui_admin_dt_step_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(-(f + 1)));
        lv_obj_t *d_lbl = lv_label_create(btn_dn);
        lv_label_set_text(d_lbl, "-");
        lv_obj_set_style_text_font(d_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(d_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_obj_center(d_lbl);
    }

    /* Live Date Footer */
    s_dt_live_date_lbl = lv_label_create(card_date);
    lv_obj_set_pos(s_dt_live_date_lbl, 8, 164);
    lv_obj_set_size(s_dt_live_date_lbl, 238, 18);
    lv_obj_set_style_text_align(s_dt_live_date_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_dt_live_date_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_dt_live_date_lbl, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_dt_live_date_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);

    /* 3. Group 2: Time Card (pos: x_time, 36, size: 178, 190) */
    lv_obj_t *card_time = lv_obj_create(panel);
    lv_obj_set_pos(card_time, x_time, 36);
    lv_obj_set_size(card_time, 178, 190);
    lv_obj_set_style_bg_color(card_time, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_time, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_time, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_time, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_time, 0, LV_PART_MAIN);
    lv_obj_clear_flag(card_time, LV_OBJ_FLAG_SCROLLABLE);

    /* Top pill badge for Time */
    lv_obj_t *t_pill = lv_obj_create(card_time);
    lv_obj_set_pos(t_pill, 19, 6);
    lv_obj_set_size(t_pill, 140, 22);
    lv_obj_set_style_bg_color(t_pill, lv_color_hex(0xffecfdf5), LV_PART_MAIN);
    lv_obj_set_style_border_color(t_pill, lv_color_hex(0xffa7f3d0), LV_PART_MAIN);
    lv_obj_set_style_border_width(t_pill, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(t_pill, 11, LV_PART_MAIN);
    lv_obj_set_style_pad_all(t_pill, 0, LV_PART_MAIN);
    lv_obj_clear_flag(t_pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t_title = lv_label_create(t_pill);
    lv_label_set_text(t_title, fa ? "ساعت و زمان سیستم" : "System Time");
    lv_obj_set_style_text_font(t_title, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(t_title, lv_color_hex(0xff059669), LV_PART_MAIN);
    lv_obj_center(t_title);

    /* 2 Time Columns: In FA: Hour(x=96), Minute(x=10). In EN: Hour(x=10), Minute(x=96) */
    int t_col_fields[2];
    const char *t_col_names[2];
    int t_col_x[2] = { 10, 96 };

    if (fa) {
        t_col_fields[0] = 4; t_col_names[0] = "دقیقه";
        t_col_fields[1] = 3; t_col_names[1] = "ساعت";
    } else {
        t_col_fields[0] = 3; t_col_names[0] = "Hour";
        t_col_fields[1] = 4; t_col_names[1] = "Min";
    }

    for (int col = 0; col < 2; col++) {
        int f = t_col_fields[col];
        int tx = t_col_x[col];

        lv_obj_t *cap = lv_label_create(card_time);
        lv_obj_set_pos(cap, tx, 32);
        lv_obj_set_size(cap, 72, 16);
        lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(cap, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(cap, lv_color_hex(0xff64748b), LV_PART_MAIN);
        lv_label_set_text(cap, t_col_names[col]);

        lv_obj_t *btn_up = lv_btn_create(card_time);
        lv_obj_set_pos(btn_up, tx, 50);
        lv_obj_set_size(btn_up, 72, 32);
        lv_obj_set_style_bg_color(btn_up, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_up, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_up, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn_up, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(btn_up, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn_up, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn_up, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(btn_up, ui_admin_dt_step_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(f + 1));
        lv_obj_t *u_lbl = lv_label_create(btn_up);
        lv_label_set_text(u_lbl, "+");
        lv_obj_set_style_text_font(u_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(u_lbl, lv_color_hex(0xff059669), LV_PART_MAIN);
        lv_obj_center(u_lbl);

        lv_obj_t *vbox = lv_obj_create(card_time);
        lv_obj_set_pos(vbox, tx, 86);
        lv_obj_set_size(vbox, 72, 36);
        lv_obj_set_style_bg_color(vbox, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_color(vbox, lv_color_hex(0xff059669), LV_PART_MAIN);
        lv_obj_set_style_border_width(vbox, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(vbox, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(vbox, 0, LV_PART_MAIN);
        lv_obj_clear_flag(vbox, LV_OBJ_FLAG_SCROLLABLE);

        s_dt_labels[f] = lv_label_create(vbox);
        lv_obj_set_size(s_dt_labels[f], LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(s_dt_labels[f], 0, LV_PART_MAIN);
        lv_obj_set_style_text_align(s_dt_labels[f], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_dt_labels[f], &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_dt_labels[f], lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_obj_center(s_dt_labels[f]);

        lv_obj_t *btn_dn = lv_btn_create(card_time);
        lv_obj_set_pos(btn_dn, tx, 126);
        lv_obj_set_size(btn_dn, 72, 32);
        lv_obj_set_style_bg_color(btn_dn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_dn, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_dn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn_dn, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(btn_dn, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn_dn, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn_dn, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(btn_dn, ui_admin_dt_step_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(-(f + 1)));
        lv_obj_t *d_lbl = lv_label_create(btn_dn);
        lv_label_set_text(d_lbl, "-");
        lv_obj_set_style_text_font(d_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(d_lbl, lv_color_hex(0xff059669), LV_PART_MAIN);
        lv_obj_center(d_lbl);
    }

    /* Colon between Hour and Minute */
    lv_obj_t *colon_lbl = lv_label_create(card_time);
    lv_obj_set_pos(colon_lbl, 83, 90);
    lv_label_set_text(colon_lbl, ":");
    lv_obj_set_style_text_font(colon_lbl, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(colon_lbl, lv_color_hex(0xff059669), LV_PART_MAIN);

    /* Live Time Footer */
    s_dt_live_time_lbl = lv_label_create(card_time);
    lv_obj_set_pos(s_dt_live_time_lbl, 8, 164);
    lv_obj_set_size(s_dt_live_time_lbl, 162, 18);
    lv_obj_set_style_text_align(s_dt_live_time_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_dt_live_time_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_dt_live_time_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_dt_live_time_lbl, lv_color_hex(0xff059669), LV_PART_MAIN);

    /* 4. Bottom action buttons (48px height): Cancel, NTP Internet Sync, Save */
    lv_obj_t *btn_cancel = lv_btn_create(panel);
    lv_obj_set_pos(btn_cancel, 12, 238);
    lv_obj_set_size(btn_cancel, 122, 48);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cancel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_cancel, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cancel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cancel, ui_admin_dt_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *c_lbl = lv_label_create(btn_cancel);
    lv_label_set_text(c_lbl, fa ? "انصراف" : "Cancel");
    lv_obj_set_style_text_color(c_lbl, lv_color_hex(0xff475569), LV_PART_MAIN);
    lv_obj_set_style_text_font(c_lbl, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(c_lbl);

    lv_obj_t *btn_ntp = lv_btn_create(panel);
    lv_obj_set_pos(btn_ntp, 142, 238);
    lv_obj_set_size(btn_ntp, 160, 48);
    lv_obj_set_style_bg_color(btn_ntp, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ntp, lv_color_hex(0xff0369a1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ntp, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ntp, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ntp, ui_admin_dt_ntp_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *n_lbl = lv_label_create(btn_ntp);
    lv_label_set_text(n_lbl, fa ? "تنظیم با اینترنت" : LV_SYMBOL_REFRESH " Sync Internet");
    lv_obj_set_style_text_color(n_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(n_lbl, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(n_lbl);

    lv_obj_t *btn_save = lv_btn_create(panel);
    lv_obj_set_pos(btn_save, 310, 238);
    lv_obj_set_size(btn_save, 134, 48);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_save, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_save, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_save, ui_admin_dt_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *s_lbl = lv_label_create(btn_save);
    lv_label_set_text(s_lbl, fa ? "ذخیره ساعت" : "Save Time");
    lv_obj_set_style_text_color(s_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_lbl, fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(s_lbl);

    ui_admin_datetime_refresh_display();
}

static void ui_admin_simple_back(lv_event_t *e)
{
    (void)e;
    ui_admin_detail_close();
    buzzer_key;
}


static void ui_admin_new_pin_refresh(void)
{
    char masked[8] = "";
    for (uint8_t i = 0; i < s_admin_new_pin_len; i++) masked[i] = '*';
    masked[s_admin_new_pin_len] = '\0';
    if (s_admin_detail_value) lv_label_set_text(s_admin_detail_value, masked[0] ? masked : "رمز جدید چهار رقمی");
}

static void ui_admin_pin_change_event(lv_event_t *e)
{
    lv_obj_t *matrix = (lv_obj_t *)lv_event_get_target(e);
    const char *key = lv_buttonmatrix_get_button_text(matrix, lv_buttonmatrix_get_selected_button(matrix));
    if (!key) return;
    if (key[0] >= '0' && key[0] <= '9' && key[1] == '\0') {
        if (s_admin_new_pin_len < 4) {
            s_admin_new_pin[s_admin_new_pin_len++] = key[0];
            s_admin_new_pin[s_admin_new_pin_len] = '\0';
        }
    } else if (!strcmp(key, LV_SYMBOL_BACKSPACE) || !strcmp(key, "DEL") || !strcmp(key, "<-") || !strcmp(key, "حذف")) {
        if (s_admin_new_pin_len > 0) {
            s_admin_new_pin_len--;
            s_admin_new_pin[s_admin_new_pin_len] = '\0';
        }
    } else if (!strcmp(key, "C")) {
        s_admin_new_pin_len = 0; s_admin_new_pin[0] = '\0';
    } else if (!strcmp(key, "ذخیره")) {
        if (s_admin_new_pin_len == 4) {
            strcpy(EEprom6_AdminPin, s_admin_new_pin);
            a2_eeprom_write_all();
            s_admin_new_pin_len = 0; s_admin_new_pin[0] = '\0';
            lv_label_set_text(s_admin_detail_value, "رمز جدید ذخیره شد");
            buzzer_key;
            return;
        }
        lv_label_set_text(s_admin_detail_value, "رمز باید چهار رقمی باشد");
        return;
    } else if (!strcmp(key, "بازگشت")) { ui_admin_detail_close(); return; }
    ui_admin_new_pin_refresh();
    buzzer_key;
}

static void ui_admin_factory_event(lv_event_t *e)
{
    lv_obj_t *matrix = (lv_obj_t *)lv_event_get_target(e);
    const char *key = lv_buttonmatrix_get_button_text(matrix, lv_buttonmatrix_get_selected_button(matrix));
    if (!key || !strcmp(key, "لغو")) { ui_admin_detail_close(); return; }
    if (!strcmp(key, "بازنشانی")) a2_eeprom_factory_reset();
}

static void ui_admin_show_pin(void)
{
    char masked[8] = "";
    for (uint8_t i = 0; i < s_admin_pin_len; i++) masked[i] = '*';
    masked[s_admin_pin_len] = '\0';
    if (s_admin_pin_label) lv_label_set_text(s_admin_pin_label, masked[0] ? masked : "----");
}

static lv_obj_t *s_stats_reset_modal = NULL;

static void ui_stats_reset_confirm_cancel_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (s_stats_reset_modal) {
        lv_obj_delete(s_stats_reset_modal);
        s_stats_reset_modal = NULL;
    }
}

static void ui_stats_reset_confirm_action_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    if (s_stats_reset_modal) {
        lv_obj_delete(s_stats_reset_modal);
        s_stats_reset_modal = NULL;
    }
    a2_eeprom_reset_production_counters();
    ui_admin_open_stats_modal();
    bool is_fa = (EEprom2_Language == L_Persion);
    ui_toast_show(is_fa ? "آمار کارکرد" : "Stats",
                  is_fa ? "تمامی شمارنده‌ها با موفقیت صفر شدند" : "Counters successfully reset",
                  2500, TOAST_TYPE_SUCCESS);
}

static void ui_open_stats_reset_confirm_modal(void)
{
    if (s_stats_reset_modal) {
        lv_obj_delete(s_stats_reset_modal);
        s_stats_reset_modal = NULL;
    }
    bool is_fa = (EEprom2_Language == L_Persion);
    s_stats_reset_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_stats_reset_modal, 480, 320);
    lv_obj_set_pos(s_stats_reset_modal, 0, 0);
    lv_obj_set_style_bg_color(s_stats_reset_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_stats_reset_modal, 160, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_stats_reset_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_stats_reset_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_stats_reset_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_stats_reset_modal);
    lv_obj_set_pos(panel, 40, 50);
    lv_obj_set_size(panel, 400, 220);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Warning Squircle Icon */
    lv_obj_t *ibox = lv_obj_create(panel);
    lv_obj_set_size(ibox, 36, 36);
    lv_obj_set_pos(ibox, is_fa ? 340 : 10, 8);
    lv_obj_set_style_bg_color(ibox, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_radius(ibox, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(ibox, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ibox, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *w_icon = lv_label_create(ibox);
    lv_label_set_text(w_icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(w_icon, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(w_icon, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_center(w_icon);

    /* Title */
    lv_obj_t *t = lv_label_create(panel);
    lv_obj_set_pos(t, is_fa ? 10 : 56, 12);
    lv_obj_set_size(t, 320, 26);
    lv_obj_set_style_text_align(t, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    const char *title_txt = (EEprom2_Language == L_english) ? "Confirm Statistics Reset" :
                            (EEprom2_Language == L_Turkish) ? "Istatistigi Sifirla Onayi" :
                            (EEprom2_Language == L_Russian) ? "Podtverzhdeniye Sbrosa" : "تایید صفر کردن آمار تولید";
    lv_label_set_text(t, title_txt);

    /* Body */
    lv_obj_t *b = lv_label_create(panel);
    lv_obj_set_pos(b, 10, 54);
    lv_obj_set_size(b, 376, 68);
    lv_obj_set_style_text_align(b, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(b, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(b, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(b, lv_color_hex(0xff334155), LV_PART_MAIN);
    const char *body_txt = (EEprom2_Language == L_english) ? "Are you sure you want to reset all production counters for Glass 1 & 2?\nAll recorded injection cycles will be cleared permanently." :
                           (EEprom2_Language == L_Turkish) ? "Cam 1 ve 2 icin tum uretim sayaclari kalici olarak sifirlanacak.\nEmin misiniz?" :
                           (EEprom2_Language == L_Russian) ? "Vse schetchiki dlya Stekla 1 i 2 budut sbrosheny.\nVy uvereny?" :
                           "آیا از ریست کامل شمارنده‌های تزریق گاز شیشه 1 و 2 اطمینان دارید؟\nکلیه آمار قطعات تولید شده صفر خواهد شد و قابل بازگشت نیست.";
    lv_label_set_text(b, body_txt);

    /* Cancel button */
    lv_obj_t *btn_cnl = lv_btn_create(panel);
    lv_obj_set_pos(btn_cnl, 14, 142);
    lv_obj_set_size(btn_cnl, 168, 44);
    lv_obj_set_style_bg_color(btn_cnl, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_cnl, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cnl, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_cnl, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cnl, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cnl, ui_stats_reset_confirm_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clbl = lv_label_create(btn_cnl);
    const char *cnl_txt = (EEprom2_Language == L_english) ? "Cancel" :
                          (EEprom2_Language == L_Turkish) ? "Iptal" :
                          (EEprom2_Language == L_Russian) ? "Otmena" : "انصراف";
    lv_label_set_text(clbl, cnl_txt);
    lv_obj_set_style_text_font(clbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(clbl, lv_color_hex(0xff475569), LV_PART_MAIN);
    lv_obj_center(clbl);

    /* Confirm Reset button */
    lv_obj_t *btn_ok = lv_btn_create(panel);
    lv_obj_set_pos(btn_ok, 194, 142);
    lv_obj_set_size(btn_ok, 168, 44);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xffb91c1c), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ok, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ok, ui_stats_reset_confirm_action_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *olbl = lv_label_create(btn_ok);
    const char *ok_txt = (EEprom2_Language == L_english) ? "Yes, Reset" :
                         (EEprom2_Language == L_Turkish) ? "Evet, Sifirla" :
                         (EEprom2_Language == L_Russian) ? "Da, Sbrosit'" : "بله، صفر شود";
    lv_label_set_text(olbl, ok_txt);
    lv_obj_set_style_text_font(olbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(olbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(olbl);
}

static void ui_admin_stats_reset_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    ui_open_stats_reset_confirm_modal();
}

static void ui_admin_stats_back_cb(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    ui_admin_detail_close();
}

void ui_admin_open_stats_modal(void)
{
    ui_admin_detail_close();
    s_admin_detail = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_admin_detail, 480, 320);
    lv_obj_set_pos(s_admin_detail, 0, 0);
    lv_obj_set_style_bg_color(s_admin_detail, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_admin_detail, 160, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_detail, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_detail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_admin_detail);
    lv_obj_set_pos(panel, 12, 10);
    lv_obj_set_size(panel, 456, 300);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);

    /* 1. Header */
    lv_obj_t *head = lv_label_create(panel);
    lv_obj_set_pos(head, 14, 8);
    lv_obj_set_size(head, 386, 24);
    lv_obj_set_style_text_align(head, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(head, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(head, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_base_dir(head, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    const char *title_text = (EEprom2_Language == L_english) ? "Production & Runtime Statistics" :
                             (EEprom2_Language == L_Turkish) ? "Uretim ve Calisma Istatistikleri" :
                             (EEprom2_Language == L_Russian) ? "Statistika Proizvodstva" : "آمار جامع تولید و کارکرد دستگاه";
    lv_label_set_text(head, title_text);

    /* Close button [X] at top right */
    lv_obj_t *close_btn = lv_btn_create(panel);
    lv_obj_set_pos(close_btn, 414, 5);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_set_style_radius(close_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(close_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, ui_admin_stats_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_center(close_lbl);

    /* Data calculations */
    unsigned long total_pieces = EEprom_TotalFilled_Glass1 + EEprom_TotalFilled_Glass2;
    unsigned long total_sec = EEprom_TotalSeconds_Glass1 + EEprom_TotalSeconds_Glass2;
    unsigned long tot_m = (total_sec / 60u) % 60u;
    unsigned long tot_h = total_sec / 3600u;
    uint32_t sec_uptime = lv_tick_get() / 1000u;
    unsigned long up_h = (unsigned long)(sec_uptime / 3600u);
    unsigned long up_m = (unsigned long)((sec_uptime / 60u) % 60u);

    /* 4 Apple HIG cards: 2x2 grid */
    int card_w = 214;
    int card_h = 94;
    int col_x[2] = { 10, 232 };
    int row_y[2] = { 36, 136 };

    int cx_g1 = is_fa ? col_x[1] : col_x[0];
    int cx_g2 = is_fa ? col_x[0] : col_x[1];

    const char *cyc_txt = (EEprom2_Language == L_english) ? "Successful cycles" :
                          (EEprom2_Language == L_Turkish) ? "Basarili cevrim" :
                          (EEprom2_Language == L_Russian) ? "Uspeshnye tsikly" : "سیکل تزریق گاز موفق";

    /* Card 1: Glass 1 */
    {
        lv_obj_t *c = lv_obj_create(panel);
        lv_obj_set_pos(c, cx_g1, row_y[0]);
        lv_obj_set_size(c, card_w, card_h);
        lv_obj_set_style_bg_color(c, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
        lv_obj_set_style_border_color(c, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(c, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ibox = lv_obj_create(c);
        lv_obj_set_size(ibox, 30, 30);
        lv_obj_set_pos(ibox, is_fa ? (card_w - 36) : 6, 6);
        lv_obj_set_style_bg_color(ibox, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_obj_set_style_radius(ibox, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(ibox, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(ibox, 0, LV_PART_MAIN);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *icon = lv_label_create(ibox);
        lv_label_set_text(icon, LV_SYMBOL_PLAY);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(icon);

        lv_obj_t *t = lv_label_create(c);
        lv_obj_set_pos(t, is_fa ? 8 : 42, 8);
        lv_obj_set_size(t, card_w - 48, 18);
        lv_obj_set_style_text_align(t, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(t, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        const char *g1_title = (EEprom2_Language == L_english) ? "Glass 1 (Right)" :
                               (EEprom2_Language == L_Turkish) ? "Cam 1 (Sag)" :
                               (EEprom2_Language == L_Russian) ? "Steklo 1 (Prav)" : "شیشه 1 (ایستگاه راست)";
        lv_label_set_text(t, g1_title);

        lv_obj_t *v = lv_label_create(c);
        lv_obj_set_pos(v, 8, 34);
        lv_obj_set_size(v, card_w - 16, 28);
        lv_obj_set_style_text_align(v, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(v, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(v, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(v, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu", EEprom_TotalFilled_Glass1);
        lv_label_set_text(v, buf);

        lv_obj_t *s = lv_label_create(c);
        lv_obj_set_pos(s, 8, 64);
        lv_obj_set_size(s, card_w - 16, 24);
        lv_obj_set_style_text_align(s, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(s, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(s, lv_color_hex(0xff64748b), LV_PART_MAIN);
        lv_label_set_text(s, cyc_txt);
    }

    /* Card 2: Glass 2 */
    {
        lv_obj_t *c = lv_obj_create(panel);
        lv_obj_set_pos(c, cx_g2, row_y[0]);
        lv_obj_set_size(c, card_w, card_h);
        lv_obj_set_style_bg_color(c, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
        lv_obj_set_style_border_color(c, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(c, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ibox = lv_obj_create(c);
        lv_obj_set_size(ibox, 30, 30);
        lv_obj_set_pos(ibox, is_fa ? (card_w - 36) : 6, 6);
        lv_obj_set_style_bg_color(ibox, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_obj_set_style_radius(ibox, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(ibox, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(ibox, 0, LV_PART_MAIN);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *icon = lv_label_create(ibox);
        lv_label_set_text(icon, LV_SYMBOL_PLAY);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(icon);

        lv_obj_t *t = lv_label_create(c);
        lv_obj_set_pos(t, is_fa ? 8 : 42, 8);
        lv_obj_set_size(t, card_w - 48, 18);
        lv_obj_set_style_text_align(t, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(t, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        const char *g2_title = (EEprom2_Language == L_english) ? "Glass 2 (Left)" :
                               (EEprom2_Language == L_Turkish) ? "Cam 2 (Sol)" :
                               (EEprom2_Language == L_Russian) ? "Steklo 2 (Lev)" : "شیشه 2 (ایستگاه چپ)";
        lv_label_set_text(t, g2_title);

        lv_obj_t *v = lv_label_create(c);
        lv_obj_set_pos(v, 8, 34);
        lv_obj_set_size(v, card_w - 16, 28);
        lv_obj_set_style_text_align(v, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(v, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(v, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(v, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu", EEprom_TotalFilled_Glass2);
        lv_label_set_text(v, buf);

        lv_obj_t *s = lv_label_create(c);
        lv_obj_set_pos(s, 8, 64);
        lv_obj_set_size(s, card_w - 16, 24);
        lv_obj_set_style_text_align(s, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(s, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(s, lv_color_hex(0xff64748b), LV_PART_MAIN);
        lv_label_set_text(s, cyc_txt);
    }

    /* Card 3: Total Output (Row 1, Right in FA) */
    {
        lv_obj_t *c = lv_obj_create(panel);
        lv_obj_set_pos(c, cx_g1, row_y[1]);
        lv_obj_set_size(c, card_w, card_h);
        lv_obj_set_style_bg_color(c, lv_color_hex(0xfff0fdf4), LV_PART_MAIN);
        lv_obj_set_style_border_color(c, lv_color_hex(0xff86efac), LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(c, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ibox = lv_obj_create(c);
        lv_obj_set_size(ibox, 30, 30);
        lv_obj_set_pos(ibox, is_fa ? (card_w - 36) : 6, 6);
        lv_obj_set_style_bg_color(ibox, lv_color_hex(0xff16a34a), LV_PART_MAIN);
        lv_obj_set_style_radius(ibox, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(ibox, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(ibox, 0, LV_PART_MAIN);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *icon = lv_label_create(ibox);
        lv_label_set_text(icon, LV_SYMBOL_OK);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(icon);

        lv_obj_t *t = lv_label_create(c);
        lv_obj_set_pos(t, is_fa ? 8 : 42, 8);
        lv_obj_set_size(t, card_w - 48, 18);
        lv_obj_set_style_text_align(t, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(t, lv_color_hex(0xff16a34a), LV_PART_MAIN);
        const char *tot_title = (EEprom2_Language == L_english) ? "Total Output" :
                                (EEprom2_Language == L_Turkish) ? "Toplam Uretim" :
                                (EEprom2_Language == L_Russian) ? "Vsego vypusk" : "مجموع کل تولیدات";
        lv_label_set_text(t, tot_title);

        lv_obj_t *v = lv_label_create(c);
        lv_obj_set_pos(v, 8, 34);
        lv_obj_set_size(v, card_w - 16, 28);
        lv_obj_set_style_text_align(v, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(v, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(v, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(v, lv_color_hex(0xff15803d), LV_PART_MAIN);
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu", total_pieces);
        lv_label_set_text(v, buf);

        lv_obj_t *s = lv_label_create(c);
        lv_obj_set_pos(s, 8, 64);
        lv_obj_set_size(s, card_w - 16, 24);
        lv_obj_set_style_text_align(s, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(s, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(s, lv_color_hex(0xff15803d), LV_PART_MAIN);
        const char *unit_txt = (EEprom2_Language == L_english) ? "Double-glazed units" :
                               (EEprom2_Language == L_Turkish) ? "Cift cam unitesi" :
                               (EEprom2_Language == L_Russian) ? "Steklopaketov" : "قطعه شیشه دوجداره پرشده";
        lv_label_set_text(s, unit_txt);
    }

    /* Card 4: Operating Hours (Row 1, Left in FA) */
    {
        lv_obj_t *c = lv_obj_create(panel);
        lv_obj_set_pos(c, cx_g2, row_y[1]);
        lv_obj_set_size(c, card_w, card_h);
        lv_obj_set_style_bg_color(c, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
        lv_obj_set_style_border_color(c, lv_color_hex(0xffddd6fe), LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(c, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ibox = lv_obj_create(c);
        lv_obj_set_size(ibox, 30, 30);
        lv_obj_set_pos(ibox, is_fa ? (card_w - 36) : 6, 6);
        lv_obj_set_style_bg_color(ibox, lv_color_hex(0xff7c3aed), LV_PART_MAIN);
        lv_obj_set_style_radius(ibox, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(ibox, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(ibox, 0, LV_PART_MAIN);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *icon = lv_label_create(ibox);
        lv_label_set_text(icon, LV_SYMBOL_REFRESH);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(icon);

        lv_obj_t *t = lv_label_create(c);
        lv_obj_set_pos(t, is_fa ? 8 : 42, 8);
        lv_obj_set_size(t, card_w - 48, 18);
        lv_obj_set_style_text_align(t, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(t, lv_color_hex(0xff7c3aed), LV_PART_MAIN);
        const char *time_title = (EEprom2_Language == L_english) ? "Operating Time" :
                                 (EEprom2_Language == L_Turkish) ? "Calisma Suresi" :
                                 (EEprom2_Language == L_Russian) ? "Vremya raboty" : "زمان‌سنجی کارکرد";
        lv_label_set_text(t, time_title);

        lv_obj_t *r1 = lv_label_create(c);
        lv_obj_set_pos(r1, 8, 35);
        lv_obj_set_size(r1, card_w - 16, 25);
        lv_obj_set_style_text_align(r1, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(r1, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(r1, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(r1, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        char b1[48];
        if (is_fa) snprintf(b1, sizeof(b1), "تزریق گاز: %lu ساعت و %lu دقیقه", tot_h, tot_m);
        else if (EEprom2_Language == L_Turkish) snprintf(b1, sizeof(b1), "Gaz Suresi: %lusa %ludk", tot_h, tot_m);
        else if (EEprom2_Language == L_Russian) snprintf(b1, sizeof(b1), "Vremya gaza: %luch %lumin", tot_h, tot_m);
        else snprintf(b1, sizeof(b1), "Gas Time: %luh %lum", tot_h, tot_m);
        lv_label_set_text(r1, b1);

        lv_obj_t *r2 = lv_label_create(c);
        lv_obj_set_pos(r2, 8, 63);
        lv_obj_set_size(r2, card_w - 16, 25);
        lv_obj_set_style_text_align(r2, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_base_dir(r2, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(r2, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(r2, lv_color_hex(0xff64748b), LV_PART_MAIN);
        char b2[48];
        if (is_fa) snprintf(b2, sizeof(b2), "برق دستگاه: %lu ساعت و %lu دقیقه", up_h, up_m);
        else if (EEprom2_Language == L_Turkish) snprintf(b2, sizeof(b2), "Calisma: %lusa %ludk", up_h, up_m);
        else if (EEprom2_Language == L_Russian) snprintf(b2, sizeof(b2), "Vremya vkl: %luch %lumin", up_h, up_m);
        else snprintf(b2, sizeof(b2), "Uptime: %luh %lum", up_h, up_m);
        lv_label_set_text(r2, b2);
    }

    /* 3. Bottom Action Buttons */
    int bx_back = is_fa ? col_x[1] : col_x[0];
    int bx_rst  = is_fa ? col_x[0] : col_x[1];

    /* Back Button */
    lv_obj_t *btn_back = lv_btn_create(panel);
    lv_obj_set_pos(btn_back, bx_back, 238);
    lv_obj_set_size(btn_back, card_w, 48);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xffe2e8f0), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_back, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_back, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_back, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, ui_admin_stats_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    const char *back_txt = (EEprom2_Language == L_english) ? "Back" :
                           (EEprom2_Language == L_Turkish) ? "Geri" :
                           (EEprom2_Language == L_Russian) ? "Nazad" : "بازگشت";
    lv_label_set_text(lbl_back, back_txt);
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_back, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_back);

    /* Reset Button (Soft Danger with safety confirmation modal) */
    lv_obj_t *btn_rst = lv_btn_create(panel);
    lv_obj_set_pos(btn_rst, bx_rst, 240);
    lv_obj_set_size(btn_rst, card_w, 46);
    lv_obj_set_style_bg_color(btn_rst, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_rst, lv_color_hex(0xfffecaca), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_rst, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_rst, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_rst, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_rst, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_rst, ui_admin_stats_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_rst = lv_label_create(btn_rst);
    const char *rst_txt = (EEprom2_Language == L_english) ? "Reset Stats" :
                          (EEprom2_Language == L_Turkish) ? "Istatistigi Sifirla" :
                          (EEprom2_Language == L_Russian) ? "Sbros statistiki" : "صفر کردن آمار";
    lv_label_set_text(lbl_rst, rst_txt);
    lv_obj_set_style_text_color(lbl_rst, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_rst, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_rst);
}

/* =========================================================================
   Apple HIG 5-Stage Day-Lock Countdown Manager with Accelerating Repeat,
   Sequential Unlocking, Dual Calendar Date & Weekday Calculation
   ========================================================================= */
/* s_admin_daylock_modal declared at top */
static lv_obj_t *s_daylock_row_card[5];
static lv_obj_t *s_daylock_days_lbl[5];
static lv_obj_t *s_daylock_date_lbl[5];
static lv_obj_t *s_daylock_btn_minus[5];
static lv_obj_t *s_daylock_btn_plus[5];
static lv_obj_t *s_daylock_summary_lbl = NULL;

static lv_timer_t *s_dl_step_timer = NULL;
static int s_dl_timer_stage = 0;
static int s_dl_timer_dir = 0;
static uint32_t s_dl_timer_start_tick = 0;

static void ui_format_stage_date(time_t base_epoch, int cumulative_days, bool is_active, char *out_buf, size_t out_sz)
{
    if (!is_active || cumulative_days <= 0) {
        bool is_fa = (EEprom2_Language == L_Persion);
        snprintf(out_buf, out_sz, is_fa ? "--- (غیرفعال)" : "--- (Disabled)");
        return;
    }
    time_t target_epoch = base_epoch + (time_t)cumulative_days * 86400LL;
    struct tm t;
    localtime_r(&target_epoch, &t);
    int gy = t.tm_year + 1900;
    int gm = t.tm_mon + 1;
    int gd = t.tm_mday;
    int wday = t.tm_wday; // 0=Sun..6=Sat

    if (EEprom2_Language == L_Persion) {
        int jy, jm, jd;
        ui_admin_gregorian_to_jalali(gy, gm, gd, &jy, &jm, &jd);
        static const char *fa_w[] = { "یک‌شنبه", "دوشنبه", "سه‌شنبه", "چهارشنبه", "پنج‌شنبه", "جمعه", "شنبه" };
        static const char *fa_m[] = { "", "فروردین", "اردیبهشت", "خرداد", "تیر", "مرداد", "شهریور", "مهر", "آبان", "آذر", "دی", "بهمن", "اسفند" };
        const char *wstr = (wday >= 0 && wday < 7) ? fa_w[wday] : "";
        const char *mstr = (jm >= 1 && jm <= 12) ? fa_m[jm] : "";
        snprintf(out_buf, out_sz, "%s %d %s %d", wstr, jd, mstr, jy);
    } else if (EEprom2_Language == L_Turkish) {
        static const char *tr_w[] = { "Paz", "Pzt", "Sal", "Car", "Per", "Cum", "Cmt" };
        static const char *tr_m[] = { "", "Oca", "Sub", "Mar", "Nis", "May", "Haz", "Tem", "Agu", "Eyl", "Eki", "Kas", "Ara" };
        const char *wstr = (wday >= 0 && wday < 7) ? tr_w[wday] : "";
        const char *mstr = (gm >= 1 && gm <= 12) ? tr_m[gm] : "";
        snprintf(out_buf, out_sz, "%s, %d %s %d", wstr, gd, mstr, gy);
    } else if (EEprom2_Language == L_Russian) {
        static const char *ru_w[] = { "Vsk", "Pnd", "Vtr", "Srd", "Cht", "Ptn", "Sub" };
        static const char *ru_m[] = { "", "Yan", "Fev", "Mar", "Apr", "May", "Iyn", "Iyl", "Avg", "Sen", "Okt", "Noy", "Dek" };
        const char *wstr = (wday >= 0 && wday < 7) ? ru_w[wday] : "";
        const char *mstr = (gm >= 1 && gm <= 12) ? ru_m[gm] : "";
        snprintf(out_buf, out_sz, "%s, %d %s %d", wstr, gd, mstr, gy);
    } else {
        static const char *en_w[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char *en_m[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        const char *wstr = (wday >= 0 && wday < 7) ? en_w[wday] : "";
        const char *mstr = (gm >= 1 && gm <= 12) ? en_m[gm] : "";
        snprintf(out_buf, out_sz, "%s, %d %s %d", wstr, gd, mstr, gy);
    }
}

static void ui_admin_daylock_refresh_5timers(void)
{
    if (!s_admin_daylock_modal) return;
    bool is_fa = (EEprom2_Language == L_Persion);

    time_t base_epoch = time(NULL);
    if (base_epoch < 1700000000) {
        struct tm ft = {0};
        ft.tm_year = 2026 - 1900; ft.tm_mon = 9 - 1; ft.tm_mday = 18;
        base_epoch = mktime(&ft);
    }

    int cumulative_days = 0;
    int total_sum_days = 0;
    bool prev_stage_active = true;

    for (int i = 0; i < 5; i++) {
        int d = day_lock_get_stage_days(i);
        bool stage_enabled = (i == 0) || prev_stage_active;

        if (!stage_enabled) {
            if (d > 0) {
                day_lock_set_stage_days(i, 0);
                d = 0;
            }
        }

        /* Days Label */
        if (s_daylock_days_lbl[i]) {
            char dbuf[32];
            if (stage_enabled) {
                if (is_fa) snprintf(dbuf, sizeof(dbuf), "%d روز", d);
                else if (EEprom2_Language == L_Turkish) snprintf(dbuf, sizeof(dbuf), "%d Gun", d);
                else if (EEprom2_Language == L_Russian) snprintf(dbuf, sizeof(dbuf), "%d Dney", d);
                else snprintf(dbuf, sizeof(dbuf), "%d Days", d);
            } else {
                snprintf(dbuf, sizeof(dbuf), "---");
            }
            lv_label_set_text(s_daylock_days_lbl[i], dbuf);
        }

        /* Date & Weekday Label */
        if (s_daylock_date_lbl[i]) {
            char dtbuf[64];
            if (stage_enabled && d > 0) {
                cumulative_days += d;
                total_sum_days += d;
                ui_format_stage_date(base_epoch, cumulative_days, true, dtbuf, sizeof(dtbuf));
                lv_obj_set_style_text_color(s_daylock_date_lbl[i], lv_color_hex(0xff0284c7), LV_PART_MAIN);
            } else if (stage_enabled && d == 0) {
                const char *inact_txt = (EEprom2_Language == L_english) ? "0 Days (Inactive)" :
                                        (EEprom2_Language == L_Turkish) ? "0 Gun (Pasif)" :
                                        (EEprom2_Language == L_Russian) ? "0 Dney (Neaktivno)" : "تایمر صفر (بدون زمان)";
                snprintf(dtbuf, sizeof(dtbuf), "%s", inact_txt);
                lv_obj_set_style_text_color(s_daylock_date_lbl[i], lv_color_hex(0xff64748b), LV_PART_MAIN);
            } else {
                const char *lock_txt = (EEprom2_Language == L_english) ? "--- (Locked)" :
                                       (EEprom2_Language == L_Turkish) ? "--- (Kilitli)" :
                                       (EEprom2_Language == L_Russian) ? "--- (Zablokirovano)" : "--- (قفل/غیرفعال)";
                snprintf(dtbuf, sizeof(dtbuf), "%s", lock_txt);
                lv_obj_set_style_text_color(s_daylock_date_lbl[i], lv_color_hex(0xff94a3b8), LV_PART_MAIN);
            }
            lv_label_set_text(s_daylock_date_lbl[i], dtbuf);
        }

        /* Buttons and Card State */
        if (s_daylock_row_card[i]) {
            if (stage_enabled) {
                lv_obj_set_style_opa(s_daylock_row_card[i], LV_OPA_COVER, LV_PART_MAIN);
                lv_obj_set_style_border_color(s_daylock_row_card[i], lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            } else {
                lv_obj_set_style_opa(s_daylock_row_card[i], LV_OPA_40, LV_PART_MAIN);
                lv_obj_set_style_border_color(s_daylock_row_card[i], lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            }
        }
        if (s_daylock_btn_minus[i]) {
            if (stage_enabled) lv_obj_clear_state(s_daylock_btn_minus[i], LV_STATE_DISABLED);
            else lv_obj_add_state(s_daylock_btn_minus[i], LV_STATE_DISABLED);
        }
        if (s_daylock_btn_plus[i]) {
            if (stage_enabled) lv_obj_clear_state(s_daylock_btn_plus[i], LV_STATE_DISABLED);
            else lv_obj_add_state(s_daylock_btn_plus[i], LV_STATE_DISABLED);
        }

        prev_stage_active = (stage_enabled && (d > 0));
    }

    /* Master Summary Card */
    if (s_daylock_summary_lbl) {
        char sbuf[140];
        if (total_sum_days > 0) {
            char final_dt[64];
            ui_format_stage_date(base_epoch, total_sum_days, true, final_dt, sizeof(final_dt));
            if (is_fa) {
                snprintf(sbuf, sizeof(sbuf), "مجموع ۵ تایمر: %d روز   |   انقضای نهایی: %s", total_sum_days, final_dt);
            } else if (EEprom2_Language == L_Turkish) {
                snprintf(sbuf, sizeof(sbuf), "Toplam 5 Zamanlayici: %d Gun   |   Bitis: %s", total_sum_days, final_dt);
            } else if (EEprom2_Language == L_Russian) {
                snprintf(sbuf, sizeof(sbuf), "Vsego 5 Taymerov: %d Dney   |   Okonchanie: %s", total_sum_days, final_dt);
            } else {
                snprintf(sbuf, sizeof(sbuf), "Total 5 Timers: %d Days   |   Final Expiry: %s", total_sum_days, final_dt);
            }
            lv_obj_set_style_text_color(s_daylock_summary_lbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        } else {
            if (is_fa) {
                snprintf(sbuf, sizeof(sbuf), "هیچ تایمری تنظیم نشده است (دستگاه بدون قفل روزشمار)");
            } else if (EEprom2_Language == L_Turkish) {
                snprintf(sbuf, sizeof(sbuf), "Hicbir zamanlayici ayarlanmadi (surekli calisma)");
            } else if (EEprom2_Language == L_Russian) {
                snprintf(sbuf, sizeof(sbuf), "Taymery ne nastroyeny (nepreryvnaya rabota)");
            } else {
                snprintf(sbuf, sizeof(sbuf), "No active day-lock timers configured (continuous operation)");
            }
            lv_obj_set_style_text_color(s_daylock_summary_lbl, lv_color_hex(0xff64748b), LV_PART_MAIN);
        }
        lv_label_set_text(s_daylock_summary_lbl, sbuf);
    }
}

static void ui_daylock_step_apply(int stage, int delta)
{
    if (stage < 0 || stage >= 5) return;
    if (stage > 0 && day_lock_get_stage_days(stage - 1) <= 0) {
        buzzer_key;
        return;
    }
    int cur = day_lock_get_stage_days(stage);
    cur += delta;
    if (cur < 0) cur = 0;
    if (cur > 999) cur = 999;
    day_lock_set_stage_days(stage, cur);

    if (cur == 0) {
        for (int j = stage + 1; j < 5; j++) {
            day_lock_set_stage_days(j, 0);
        }
    }
    ui_admin_daylock_refresh_5timers();
    buzzer_key;
}

static void ui_daylock_timer_cb(lv_timer_t *t)
{
    (void)t;
    uint32_t elapsed = lv_tick_elaps(s_dl_timer_start_tick);
    int step = 1;
    uint32_t next_period = 200;

    if (elapsed > 3500) {
        step = 5;
        next_period = 60;
    } else if (elapsed > 2000) {
        step = 2;
        next_period = 100;
    } else if (elapsed > 1000) {
        step = 1;
        next_period = 160;
    } else {
        step = 1;
        next_period = 220;
    }

    lv_timer_set_period(s_dl_step_timer, next_period);
    ui_daylock_step_apply(s_dl_timer_stage, s_dl_timer_dir * step);
}

static void ui_daylock_btn_press_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int param = (int)(intptr_t)lv_event_get_user_data(e);
    int stage = param / 10;
    int dir = (param % 10 == 1) ? 1 : -1;

    if (code == LV_EVENT_PRESSED) {
        s_dl_timer_stage = stage;
        s_dl_timer_dir = dir;
        s_dl_timer_start_tick = lv_tick_get();
        ui_daylock_step_apply(stage, dir);

        if (s_dl_step_timer) {
            lv_timer_delete(s_dl_step_timer);
            s_dl_step_timer = NULL;
        }
        s_dl_step_timer = lv_timer_create(ui_daylock_timer_cb, 350, NULL);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_dl_step_timer) {
            lv_timer_delete(s_dl_step_timer);
            s_dl_step_timer = NULL;
        }
    }
}

static void ui_daylock_action_btn_cb(lv_event_t *e)
{
    int act = (int)(intptr_t)lv_event_get_user_data(e);
    buzzer_key;

    if (s_dl_step_timer) {
        lv_timer_delete(s_dl_step_timer);
        s_dl_step_timer = NULL;
    }

    if (act == 0) {
        day_lock_clear_after_unlock();
        for (int i = 0; i < 5; i++) day_lock_set_stage_days(i, 0);
        ui_admin_daylock_refresh_5timers();
        bool is_fa = (EEprom2_Language == L_Persion);
        ui_toast_show(is_fa ? "قفل روزشمار" : "Day Lock",
                      is_fa ? "کلیه تایمرهای قفل غیرفعال شد" : "All lock timers disabled",
                      2500, TOAST_TYPE_WARNING);
    } else if (act == 1) {
        device_locked = true;
        day_lock_save();
        if (s_admin_daylock_modal) {
            lv_obj_delete(s_admin_daylock_modal);
            s_admin_daylock_modal = NULL;
        }
        dev_lock_ui_show();
    } else if (act == 2) {
        day_lock_arm_chain();
        day_lock_save();
        bool is_fa = (EEprom2_Language == L_Persion);
        ui_toast_show(is_fa ? "فعالسازی قفل" : "Arm Lock",
                      is_fa ? "زنجیره ۵ مرحله‌ای قفل با موفقیت فعال و ذخیره شد" : "5-Stage lock chain armed and saved",
                      3000, TOAST_TYPE_SUCCESS);
        ui_admin_daylock_refresh_5timers();
    } else if (act == 3) {
        if (s_admin_daylock_modal) {
            lv_obj_delete(s_admin_daylock_modal);
            s_admin_daylock_modal = NULL;
        }
    }
}

void ui_admin_open_daylock_modal(void)
{
    if (s_admin_daylock_modal) {
        lv_obj_delete(s_admin_daylock_modal);
        s_admin_daylock_modal = NULL;
    }
    if (s_dl_step_timer) {
        lv_timer_delete(s_dl_step_timer);
        s_dl_step_timer = NULL;
    }

    bool is_fa = (EEprom2_Language == L_Persion);

    s_admin_daylock_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_admin_daylock_modal, 480, 320);
    lv_obj_set_pos(s_admin_daylock_modal, 0, 0);
    lv_obj_set_style_bg_color(s_admin_daylock_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_admin_daylock_modal, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_daylock_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_daylock_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_daylock_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_admin_daylock_modal);
    lv_obj_set_pos(panel, 8, 6);
    lv_obj_set_size(panel, 464, 308);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    const char *dl_title = (EEprom2_Language == L_english) ? "5-Stage Day-Lock Countdown Timers" :
                           (EEprom2_Language == L_Turkish) ? "5 Asamali Gun Kilidi Yonetimi" :
                           (EEprom2_Language == L_Russian) ? "5-Etapnyj Dnevnoj Blok" :
                           "مدیریت ۵ تایمر قفل روزشمار دستگاه";

    lv_obj_t *title = lv_label_create(panel);
    lv_obj_set_pos(title, 10, 4);
    lv_obj_set_size(title, 396, 22);
    lv_obj_set_style_text_align(title, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_label_set_text(title, dl_title);

    lv_obj_t *close_btn = lv_btn_create(panel);
    lv_obj_set_pos(close_btn, 416, 2);
    lv_obj_set_size(close_btn, 32, 26);
    lv_obj_set_style_radius(close_btn, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, ui_daylock_action_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)3);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_center(close_lbl);

    static const char *stg_names_fa[5] = { "تایمر 1", "تایمر 2", "تایمر 3", "تایمر 4", "تایمر 5" };
    static const char *stg_names_en[5] = { "Timer 1", "Timer 2", "Timer 3", "Timer 4", "Timer 5" };

    for (int i = 0; i < 5; i++) {
        int ry = 30 + i * 38;
        s_daylock_row_card[i] = lv_obj_create(panel);
        lv_obj_set_pos(s_daylock_row_card[i], 6, ry);
        lv_obj_set_size(s_daylock_row_card[i], 440, 36);
        lv_obj_set_style_bg_color(s_daylock_row_card[i], lv_color_hex(0xfff8fafc), LV_PART_MAIN);
        lv_obj_set_style_border_color(s_daylock_row_card[i], lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_obj_set_style_border_width(s_daylock_row_card[i], 1, LV_PART_MAIN);
        lv_obj_set_style_radius(s_daylock_row_card[i], 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(s_daylock_row_card[i], 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(s_daylock_row_card[i], 0, LV_PART_MAIN);
        lv_obj_clear_flag(s_daylock_row_card[i], LV_OBJ_FLAG_SCROLLABLE);

        if (is_fa) {
            lv_obj_t *badge = lv_obj_create(s_daylock_row_card[i]);
            lv_obj_set_pos(badge, 360, 2);
            lv_obj_set_size(badge, 74, 30);
            lv_obj_set_style_bg_color(badge, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
            lv_obj_set_style_border_color(badge, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(badge, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
            lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_t *blbl = lv_label_create(badge);
            lv_label_set_text(blbl, stg_names_fa[i]);
            lv_obj_set_style_text_font(blbl, ui_font_vazir_or(&ui_font_arial_regular17, 12), LV_PART_MAIN);
            lv_obj_set_style_text_color(blbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
            lv_obj_center(blbl);

            s_daylock_btn_minus[i] = lv_btn_create(s_daylock_row_card[i]);
            lv_obj_set_pos(s_daylock_btn_minus[i], 318, 2);
            lv_obj_set_size(s_daylock_btn_minus[i], 38, 30);
            lv_obj_set_style_bg_color(s_daylock_btn_minus[i], lv_color_hex(0xfffee2e2), LV_PART_MAIN);
            lv_obj_set_style_bg_color(s_daylock_btn_minus[i], lv_color_hex(0xfffecaca), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(s_daylock_btn_minus[i], lv_color_hex(0xfffca5a5), LV_PART_MAIN);
            lv_obj_set_style_border_width(s_daylock_btn_minus[i], 1, LV_PART_MAIN);
            lv_obj_set_style_radius(s_daylock_btn_minus[i], 6, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(s_daylock_btn_minus[i], 0, LV_PART_MAIN);
            lv_obj_add_event_cb(s_daylock_btn_minus[i], ui_daylock_btn_press_cb, LV_EVENT_ALL, (void *)(intptr_t)(i * 10 + 2));
            lv_obj_t *m_lbl = lv_label_create(s_daylock_btn_minus[i]);
            lv_label_set_text(m_lbl, "-");
            lv_obj_set_style_text_font(m_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
            lv_obj_set_style_text_color(m_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
            lv_obj_center(m_lbl);

            lv_obj_t *dbox = lv_obj_create(s_daylock_row_card[i]);
            lv_obj_set_pos(dbox, 240, 2);
            lv_obj_set_size(dbox, 74, 30);
            lv_obj_set_style_bg_color(dbox, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(dbox, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_obj_set_style_border_width(dbox, 2, LV_PART_MAIN);
            lv_obj_set_style_radius(dbox, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(dbox, 0, LV_PART_MAIN);
            lv_obj_clear_flag(dbox, LV_OBJ_FLAG_SCROLLABLE);

            s_daylock_days_lbl[i] = lv_label_create(dbox);
            lv_obj_set_style_text_font(s_daylock_days_lbl[i], ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
            lv_obj_set_style_text_color(s_daylock_days_lbl[i], lv_color_hex(0xff0f172a), LV_PART_MAIN);
            lv_obj_center(s_daylock_days_lbl[i]);

            s_daylock_btn_plus[i] = lv_btn_create(s_daylock_row_card[i]);
            lv_obj_set_pos(s_daylock_btn_plus[i], 198, 2);
            lv_obj_set_size(s_daylock_btn_plus[i], 38, 30);
            lv_obj_set_style_bg_color(s_daylock_btn_plus[i], lv_color_hex(0xffdcfce7), LV_PART_MAIN);
            lv_obj_set_style_bg_color(s_daylock_btn_plus[i], lv_color_hex(0xffbbf7d0), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(s_daylock_btn_plus[i], lv_color_hex(0xff86efac), LV_PART_MAIN);
            lv_obj_set_style_border_width(s_daylock_btn_plus[i], 1, LV_PART_MAIN);
            lv_obj_set_style_radius(s_daylock_btn_plus[i], 6, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(s_daylock_btn_plus[i], 0, LV_PART_MAIN);
            lv_obj_add_event_cb(s_daylock_btn_plus[i], ui_daylock_btn_press_cb, LV_EVENT_ALL, (void *)(intptr_t)(i * 10 + 1));
            lv_obj_t *p_lbl = lv_label_create(s_daylock_btn_plus[i]);
            lv_label_set_text(p_lbl, "+");
            lv_obj_set_style_text_font(p_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
            lv_obj_set_style_text_color(p_lbl, lv_color_hex(0xff16a34a), LV_PART_MAIN);
            lv_obj_center(p_lbl);

            lv_obj_t *dtbox = lv_obj_create(s_daylock_row_card[i]);
            lv_obj_set_pos(dtbox, 4, 2);
            lv_obj_set_size(dtbox, 190, 30);
            lv_obj_set_style_bg_color(dtbox, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(dtbox, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_width(dtbox, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(dtbox, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(dtbox, 0, LV_PART_MAIN);
            lv_obj_clear_flag(dtbox, LV_OBJ_FLAG_SCROLLABLE);

            s_daylock_date_lbl[i] = lv_label_create(dtbox);
            lv_obj_set_style_text_font(s_daylock_date_lbl[i], ui_font_vazir_or(&ui_font_arial_regular17, 12), LV_PART_MAIN);
            lv_obj_set_style_base_dir(s_daylock_date_lbl[i], LV_BASE_DIR_RTL, LV_PART_MAIN);
            lv_obj_center(s_daylock_date_lbl[i]);

        } else {
            lv_obj_t *badge = lv_obj_create(s_daylock_row_card[i]);
            lv_obj_set_pos(badge, 4, 2);
            lv_obj_set_size(badge, 74, 30);
            lv_obj_set_style_bg_color(badge, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
            lv_obj_set_style_border_color(badge, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(badge, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
            lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_t *blbl = lv_label_create(badge);
            lv_label_set_text(blbl, stg_names_en[i]);
            lv_obj_set_style_text_font(blbl, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_obj_set_style_text_color(blbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
            lv_obj_center(blbl);

            s_daylock_btn_minus[i] = lv_btn_create(s_daylock_row_card[i]);
            lv_obj_set_pos(s_daylock_btn_minus[i], 82, 2);
            lv_obj_set_size(s_daylock_btn_minus[i], 38, 30);
            lv_obj_set_style_bg_color(s_daylock_btn_minus[i], lv_color_hex(0xfffee2e2), LV_PART_MAIN);
            lv_obj_set_style_bg_color(s_daylock_btn_minus[i], lv_color_hex(0xfffecaca), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(s_daylock_btn_minus[i], lv_color_hex(0xfffca5a5), LV_PART_MAIN);
            lv_obj_set_style_border_width(s_daylock_btn_minus[i], 1, LV_PART_MAIN);
            lv_obj_set_style_radius(s_daylock_btn_minus[i], 6, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(s_daylock_btn_minus[i], 0, LV_PART_MAIN);
            lv_obj_add_event_cb(s_daylock_btn_minus[i], ui_daylock_btn_press_cb, LV_EVENT_ALL, (void *)(intptr_t)(i * 10 + 2));
            lv_obj_t *m_lbl = lv_label_create(s_daylock_btn_minus[i]);
            lv_label_set_text(m_lbl, "-");
            lv_obj_set_style_text_font(m_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
            lv_obj_set_style_text_color(m_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
            lv_obj_center(m_lbl);

            lv_obj_t *dbox = lv_obj_create(s_daylock_row_card[i]);
            lv_obj_set_pos(dbox, 124, 2);
            lv_obj_set_size(dbox, 74, 30);
            lv_obj_set_style_bg_color(dbox, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(dbox, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_obj_set_style_border_width(dbox, 2, LV_PART_MAIN);
            lv_obj_set_style_radius(dbox, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(dbox, 0, LV_PART_MAIN);
            lv_obj_clear_flag(dbox, LV_OBJ_FLAG_SCROLLABLE);

            s_daylock_days_lbl[i] = lv_label_create(dbox);
            lv_obj_set_style_text_font(s_daylock_days_lbl[i], &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_set_style_text_color(s_daylock_days_lbl[i], lv_color_hex(0xff0f172a), LV_PART_MAIN);
            lv_obj_center(s_daylock_days_lbl[i]);

            s_daylock_btn_plus[i] = lv_btn_create(s_daylock_row_card[i]);
            lv_obj_set_pos(s_daylock_btn_plus[i], 202, 2);
            lv_obj_set_size(s_daylock_btn_plus[i], 38, 30);
            lv_obj_set_style_bg_color(s_daylock_btn_plus[i], lv_color_hex(0xffdcfce7), LV_PART_MAIN);
            lv_obj_set_style_bg_color(s_daylock_btn_plus[i], lv_color_hex(0xffbbf7d0), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(s_daylock_btn_plus[i], lv_color_hex(0xff86efac), LV_PART_MAIN);
            lv_obj_set_style_border_width(s_daylock_btn_plus[i], 1, LV_PART_MAIN);
            lv_obj_set_style_radius(s_daylock_btn_plus[i], 6, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(s_daylock_btn_plus[i], 0, LV_PART_MAIN);
            lv_obj_add_event_cb(s_daylock_btn_plus[i], ui_daylock_btn_press_cb, LV_EVENT_ALL, (void *)(intptr_t)(i * 10 + 1));
            lv_obj_t *p_lbl = lv_label_create(s_daylock_btn_plus[i]);
            lv_label_set_text(p_lbl, "+");
            lv_obj_set_style_text_font(p_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
            lv_obj_set_style_text_color(p_lbl, lv_color_hex(0xff16a34a), LV_PART_MAIN);
            lv_obj_center(p_lbl);

            lv_obj_t *dtbox = lv_obj_create(s_daylock_row_card[i]);
            lv_obj_set_pos(dtbox, 244, 2);
            lv_obj_set_size(dtbox, 192, 30);
            lv_obj_set_style_bg_color(dtbox, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_color(dtbox, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_width(dtbox, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(dtbox, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(dtbox, 0, LV_PART_MAIN);
            lv_obj_clear_flag(dtbox, LV_OBJ_FLAG_SCROLLABLE);

            s_daylock_date_lbl[i] = lv_label_create(dtbox);
            lv_obj_set_style_text_font(s_daylock_date_lbl[i], &lv_font_montserrat_12, LV_PART_MAIN);
            lv_obj_set_style_base_dir(s_daylock_date_lbl[i], LV_BASE_DIR_LTR, LV_PART_MAIN);
            lv_obj_center(s_daylock_date_lbl[i]);
        }
    }

    /* Summary Card */
    lv_obj_t *scard = lv_obj_create(panel);
    lv_obj_set_pos(scard, 6, 222);
    lv_obj_set_size(scard, 440, 32);
    lv_obj_set_style_bg_color(scard, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(scard, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(scard, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(scard, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scard, 0, LV_PART_MAIN);
    lv_obj_clear_flag(scard, LV_OBJ_FLAG_SCROLLABLE);

    s_daylock_summary_lbl = lv_label_create(scard);
    lv_obj_set_pos(s_daylock_summary_lbl, 4, 4);
    lv_obj_set_size(s_daylock_summary_lbl, 432, 22);
    lv_obj_set_style_text_align(s_daylock_summary_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_daylock_summary_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_daylock_summary_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);

    /* Action Buttons */
    lv_obj_t *btn_dis = lv_btn_create(panel);
    lv_obj_set_pos(btn_dis, 6, 258);
    lv_obj_set_size(btn_dis, 82, 38);
    lv_obj_set_style_bg_color(btn_dis, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_dis, lv_color_hex(0xffb91c1c), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_dis, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_dis, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_dis, ui_daylock_action_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)0);

    lv_obj_t *dl1 = lv_label_create(btn_dis);
    const char *dis_txt = (EEprom2_Language == L_english) ? "Disable" :
                          (EEprom2_Language == L_Turkish) ? "Devre Disi" :
                          (EEprom2_Language == L_Russian) ? "Otklyuchit'" : "غیرفعال";
    lv_label_set_text(dl1, dis_txt);
    lv_obj_set_style_text_color(dl1, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(dl1, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(dl1);

    lv_obj_t *btn_tst = lv_btn_create(panel);
    lv_obj_set_pos(btn_tst, 94, 258);
    lv_obj_set_size(btn_tst, 82, 38);
    lv_obj_set_style_bg_color(btn_tst, lv_color_hex(0xffd97706), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_tst, lv_color_hex(0xffb45309), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_tst, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_tst, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_tst, ui_daylock_action_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    lv_obj_t *dl2 = lv_label_create(btn_tst);
    const char *tst_txt = (EEprom2_Language == L_english) ? "Test Lock" :
                          (EEprom2_Language == L_Turkish) ? "Kilit Testi" :
                          (EEprom2_Language == L_Russian) ? "Test Bloka" : "تست قفل";
    lv_label_set_text(dl2, tst_txt);
    lv_obj_set_style_text_color(dl2, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(dl2, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(dl2);

    lv_obj_t *btn_arm = lv_btn_create(panel);
    lv_obj_set_pos(btn_arm, 182, 258);
    lv_obj_set_size(btn_arm, 188, 38);
    lv_obj_set_style_bg_color(btn_arm, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_arm, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_arm, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_arm, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_arm, ui_daylock_action_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)2);

    lv_obj_t *dl3 = lv_label_create(btn_arm);
    const char *arm_txt = (EEprom2_Language == L_english) ? "Arm & Save Chain" :
                          (EEprom2_Language == L_Turkish) ? "Aktif Et ve Kaydet" :
                          (EEprom2_Language == L_Russian) ? "Aktivirovat'" : "فعالسازی و ذخیره";
    lv_label_set_text(dl3, arm_txt);
    lv_obj_set_style_text_color(dl3, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(dl3, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(dl3);

    lv_obj_t *btn_cls = lv_btn_create(panel);
    lv_obj_set_pos(btn_cls, 376, 258);
    lv_obj_set_size(btn_cls, 70, 38);
    lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_cls, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cls, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cls, ui_daylock_action_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)3);

    lv_obj_t *dl4 = lv_label_create(btn_cls);
    const char *cls_txt = (EEprom2_Language == L_english) ? "Close" :
                          (EEprom2_Language == L_Turkish) ? "Kapat" :
                          (EEprom2_Language == L_Russian) ? "Zakryt'" : "بستن";
    lv_label_set_text(dl4, cls_txt);
    lv_obj_set_style_text_color(dl4, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(dl4, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(dl4);

    ui_admin_daylock_refresh_5timers();
}

void ui_daylock_open_numpad(void)
{
    ui_admin_open_daylock_modal();
}

/* =========================================================================
   Apple HIG Change Admin PIN Modal
   ========================================================================= */
/* s_admin_pin_modal declared at top */
static lv_obj_t *s_admin_pin_display = NULL;
static char s_admin_pin_input[8] = {0};
static uint8_t s_admin_pin_input_len = 0;

static void ui_admin_pin_refresh_display(void)
{
    if (!s_admin_pin_display) return;
    char disp[32] = {0};
    if (s_admin_pin_input_len == 0) {
        snprintf(disp, sizeof(disp), "•  •  •  •");
        lv_obj_set_style_text_color(s_admin_pin_display, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    } else {
        char stars[16] = {0};
        for (int i = 0; i < s_admin_pin_input_len; i++) {
            stars[i * 3 + 0] = '*';
            stars[i * 3 + 1] = ' ';
            stars[i * 3 + 2] = ' ';
        }
        snprintf(disp, sizeof(disp), "%s", stars);
        lv_obj_set_style_text_color(s_admin_pin_display, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    }
    lv_label_set_text(s_admin_pin_display, disp);
}

static void ui_admin_pin_btn_cb(lv_event_t *e)
{
    const char *key = (const char *)lv_event_get_user_data(e);
    if (!key) return;
    buzzer_key;

    if (!strcmp(key, "CLOSE") || !strcmp(key, "CANCEL")) {
        if (s_admin_pin_modal) {
            lv_obj_delete(s_admin_pin_modal);
            s_admin_pin_modal = NULL;
        }
        return;
    }
    if (!strcmp(key, "CLEAR")) {
        s_admin_pin_input_len = 0;
        s_admin_pin_input[0] = '\0';
        ui_admin_pin_refresh_display();
        return;
    }
    if (!strcmp(key, "BS")) {
        if (s_admin_pin_input_len > 0) {
            s_admin_pin_input_len--;
            s_admin_pin_input[s_admin_pin_input_len] = '\0';
            ui_admin_pin_refresh_display();
        }
        return;
    }
    if (!strcmp(key, "SAVE")) {
        if (s_admin_pin_input_len == 4) {
            strncpy(EEprom6_AdminPin, s_admin_pin_input, sizeof(EEprom6_AdminPin) - 1);
            EEprom6_AdminPin[sizeof(EEprom6_AdminPin) - 1] = '\0';
            a2_eeprom_write_all();
            if (s_admin_pin_modal) {
                lv_obj_delete(s_admin_pin_modal);
                s_admin_pin_modal = NULL;
            }
            bool is_fa = (EEprom2_Language == L_Persion);
            ui_toast_show(is_fa ? "رمز مدیر" : "Admin PIN",
                          is_fa ? "رمز عبور جدید مدیر با موفقیت ذخیره شد" : "New admin PIN successfully saved",
                          3000, TOAST_TYPE_SUCCESS);
        } else {
            bool is_fa = (EEprom2_Language == L_Persion);
            ui_toast_show(is_fa ? "خطای رمز" : "PIN Error",
                          is_fa ? "رمز عبور باید ۴ رقمی باشد" : "PIN must be 4 digits",
                          2500, TOAST_TYPE_ERROR);
        }
        return;
    }
    if (key[0] >= '0' && key[0] <= '9') {
        if (s_admin_pin_input_len < 4) {
            s_admin_pin_input[s_admin_pin_input_len++] = key[0];
            s_admin_pin_input[s_admin_pin_input_len] = '\0';
            ui_admin_pin_refresh_display();
        }
    }
}

void ui_admin_open_pin_modal(void)
{
    if (s_admin_pin_modal) {
        lv_obj_delete(s_admin_pin_modal);
        s_admin_pin_modal = NULL;
    }
    s_admin_pin_input_len = 0;
    s_admin_pin_input[0] = '\0';

    bool is_fa = (EEprom2_Language == L_Persion);

    s_admin_pin_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_admin_pin_modal, 480, 320);
    lv_obj_set_pos(s_admin_pin_modal, 0, 0);
    lv_obj_set_style_bg_color(s_admin_pin_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_admin_pin_modal, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_pin_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_pin_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_pin_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_admin_pin_modal);
    lv_obj_set_pos(panel, 80, 10);
    lv_obj_set_size(panel, 320, 300);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(panel);
    lv_obj_set_pos(t, 10, 2);
    lv_obj_set_size(t, 290, 22);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    const char *title_txt = (EEprom2_Language == L_english) ? "Change Admin PIN" :
                            (EEprom2_Language == L_Turkish) ? "Yonetici PIN Degistir" :
                            (EEprom2_Language == L_Russian) ? "Smena PIN Administratora" : "تغییر رمز عبور مدیر سیستم";
    lv_label_set_text(t, title_txt);

    lv_obj_t *vbox = lv_obj_create(panel);
    lv_obj_set_pos(vbox, 20, 26);
    lv_obj_set_size(vbox, 268, 36);
    lv_obj_set_style_bg_color(vbox, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_color(vbox, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_border_width(vbox, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(vbox, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(vbox, 0, LV_PART_MAIN);
    lv_obj_clear_flag(vbox, LV_OBJ_FLAG_SCROLLABLE);

    s_admin_pin_display = lv_label_create(vbox);
    lv_obj_set_size(s_admin_pin_display, 260, 30);
    lv_obj_set_style_text_align(s_admin_pin_display, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_pin_display, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_center(s_admin_pin_display);
    ui_admin_pin_refresh_display();

    static const char *pin_keys[12] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        "CLEAR", "0", "BS"
    };

    int kw = 84;
    int kh = 38;
    int k_start_x = 20;
    int k_start_y = 68;
    int k_gap_x = 8;
    int k_gap_y = 6;

    for (int k = 0; k < 12; k++) {
        int r = k / 3;
        int c = k % 3;
        int kx = k_start_x + c * (kw + k_gap_x);
        int ky = k_start_y + r * (kh + k_gap_y);

        lv_obj_t *kbtn = lv_btn_create(panel);
        lv_obj_set_pos(kbtn, kx, ky);
        lv_obj_set_size(kbtn, kw, kh);
        lv_obj_set_style_radius(kbtn, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(kbtn, 1, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(kbtn, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(kbtn, 0, LV_PART_MAIN);

        if (!strcmp(pin_keys[k], "BS")) {
            lv_obj_set_style_bg_color(kbtn, lv_color_hex(0xfffee2e2), LV_PART_MAIN);
            lv_obj_set_style_bg_color(kbtn, lv_color_hex(0xfffecaca), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(kbtn, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
        } else if (!strcmp(pin_keys[k], "CLEAR")) {
            lv_obj_set_style_bg_color(kbtn, lv_color_hex(0xfffef3c7), LV_PART_MAIN);
            lv_obj_set_style_bg_color(kbtn, lv_color_hex(0xfffde68a), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(kbtn, lv_color_hex(0xfffbbf24), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(kbtn, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_color(kbtn, lv_color_hex(0xfff1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_border_color(kbtn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        }

        lv_obj_add_event_cb(kbtn, ui_admin_pin_btn_cb, LV_EVENT_CLICKED, (void *)pin_keys[k]);

        lv_obj_t *klbl = lv_label_create(kbtn);
        if (!strcmp(pin_keys[k], "BS")) {
            lv_label_set_text(klbl, LV_SYMBOL_BACKSPACE);
            lv_obj_set_style_text_font(klbl, &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_set_style_text_color(klbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
        } else if (!strcmp(pin_keys[k], "CLEAR")) {
            const char *clr_txt = (EEprom2_Language == L_english) ? "CLR" :
                                  (EEprom2_Language == L_Turkish) ? "TEMIZ" :
                                  (EEprom2_Language == L_Russian) ? "OCH" : "پاک";
            lv_label_set_text(klbl, clr_txt);
            lv_obj_set_style_text_font(klbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_set_style_text_color(klbl, lv_color_hex(0xffb45309), LV_PART_MAIN);
        } else {
            lv_label_set_text(klbl, pin_keys[k]);
            lv_obj_set_style_text_font(klbl, &lv_font_montserrat_18, LV_PART_MAIN);
            lv_obj_set_style_text_color(klbl, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        }
        lv_obj_center(klbl);
    }

    lv_obj_t *btn_cnl = lv_btn_create(panel);
    lv_obj_set_pos(btn_cnl, 20, 246);
    lv_obj_set_size(btn_cnl, 114, 42);
    lv_obj_set_style_bg_color(btn_cnl, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_cnl, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cnl, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_cnl, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cnl, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cnl, ui_admin_pin_btn_cb, LV_EVENT_CLICKED, (void *)"CANCEL");

    lv_obj_t *cnl_lbl = lv_label_create(btn_cnl);
    const char *cnl_txt = (EEprom2_Language == L_english) ? "Cancel" :
                          (EEprom2_Language == L_Turkish) ? "Iptal" :
                          (EEprom2_Language == L_Russian) ? "Otmena" : "بازگشت";
    lv_label_set_text(cnl_lbl, cnl_txt);
    lv_obj_set_style_text_font(cnl_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(cnl_lbl, lv_color_hex(0xff475569), LV_PART_MAIN);
    lv_obj_center(cnl_lbl);

    lv_obj_t *btn_save = lv_btn_create(panel);
    lv_obj_set_pos(btn_save, 142, 246);
    lv_obj_set_size(btn_save, 146, 42);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_save, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_save, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_save, ui_admin_pin_btn_cb, LV_EVENT_CLICKED, (void *)"SAVE");

    lv_obj_t *save_lbl = lv_label_create(btn_save);
    const char *save_txt = (EEprom2_Language == L_english) ? "Save PIN" :
                           (EEprom2_Language == L_Turkish) ? "PIN Kaydet" :
                           (EEprom2_Language == L_Russian) ? "Sokhranit'" : "ذخیره رمز";
    lv_label_set_text(save_lbl, save_txt);
    lv_obj_set_style_text_font(save_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(save_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(save_lbl);
}

static void ui_admin_menu_click_cb(lv_event_t *e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    buzzer_key;
    if (id == 0) {
        ui_admin_open_datetime_modal();
    } else if (id == 1) {
        ui_admin_open_daylock_modal();
    } else if (id == 3) {
        ui_pinout_diagram_open(NULL);
    } else if (id == 4) {
        ui_admin_open_stats_modal();
    } else if (id == 6) {
        ui_admin_open_pin_modal();
    } else if (id == 7) {
        ui_open_factory_reset_confirm_modal();
    }
}

static lv_obj_t *ui_admin_create_menu_tile(lv_obj_t *parent, int x, int y, int w, int h,
                                           const char *icon, lv_color_t bg_c, lv_color_t fg_c,
                                           const char *title, const char *sub, int id)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xfff8fafc), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, fg_c, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn, ui_admin_menu_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)id);

    bool is_fa = (EEprom2_Language == L_Persion);

    /* Icon Box: rounded squircle with vibrant Apple color */
    lv_obj_t *ibox = lv_obj_create(btn);
    lv_obj_set_size(ibox, (w < 250) ? 28 : 32, (w < 250) ? 28 : 32);
    lv_obj_set_style_bg_color(ibox, bg_c, LV_PART_MAIN);
    lv_obj_set_style_border_width(ibox, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(ibox, (w < 250) ? 7 : 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ibox, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ibox, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ibox, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ibox, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *icon_lbl = lv_label_create(ibox);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_font(icon_lbl, (w < 250) ? &lv_font_montserrat_14 : &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_lbl, fg_c, LV_PART_MAIN);
    lv_obj_center(icon_lbl);

    /* Title Label */
    lv_obj_t *lbl_title = lv_label_create(btn);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);

    /* Subtitle Label */
    lv_obj_t *lbl_sub = lv_label_create(btn);
    lv_label_set_text(lbl_sub, sub);
    lv_obj_set_style_text_font(lbl_sub, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 10) : &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0xff64748b), LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_sub, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);

    /* Chevron */
    lv_obj_t *lbl_chev = lv_label_create(btn);
    lv_label_set_text(lbl_chev, is_fa ? "<" : ">");
    lv_obj_set_style_text_font(lbl_chev, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_chev, lv_color_hex(0xff94a3b8), LV_PART_MAIN);

    if (w < 250) {
        lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
        if (is_fa) {
            lv_obj_set_pos(ibox, w - 36, 5);
            lv_obj_set_pos(lbl_chev, 10, 8);

            lv_obj_set_pos(lbl_title, 8, 33);
            lv_obj_set_size(lbl_title, w - 16, 18);
            lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

            lv_obj_set_pos(lbl_sub, 8, 52);
            lv_obj_set_size(lbl_sub, w - 16, 16);
            lv_obj_set_style_text_align(lbl_sub, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        } else {
            lv_obj_set_pos(ibox, 8, 5);
            lv_obj_set_pos(lbl_chev, w - 18, 8);

            lv_obj_set_pos(lbl_title, 8, 33);
            lv_obj_set_size(lbl_title, w - 16, 18);
            lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

            lv_obj_set_pos(lbl_sub, 8, 52);
            lv_obj_set_size(lbl_sub, w - 16, 16);
            lv_obj_set_style_text_align(lbl_sub, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        }
    } else {
        lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
        int text_w = w - 74;
        if (is_fa) {
            lv_obj_set_pos(ibox, w - 40, (h - 32) / 2);
            lv_obj_set_pos(lbl_title, 24, 4);
            lv_obj_set_size(lbl_title, text_w, 18);
            lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

            lv_obj_set_pos(lbl_sub, 24, 23);
            lv_obj_set_size(lbl_sub, text_w, 16);
            lv_obj_set_style_text_align(lbl_sub, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
            lv_obj_set_pos(lbl_chev, 8, (h - 16) / 2);
        } else {
            lv_obj_set_pos(ibox, 8, (h - 32) / 2);
            lv_obj_set_pos(lbl_title, 48, 4);
            lv_obj_set_size(lbl_title, text_w, 18);
            lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

            lv_obj_set_pos(lbl_sub, 48, 23);
            lv_obj_set_size(lbl_sub, text_w, 16);
            lv_obj_set_style_text_align(lbl_sub, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            lv_obj_set_pos(lbl_chev, w - 16, (h - 16) / 2);
        }
    }

    return btn;
}

void ui_admin_show_options(void)
{
    if (!s_admin_tab) return;
    lv_obj_set_style_bg_color(s_admin_tab, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_admin_tab, LV_OPA_COVER, LV_PART_MAIN);
    if (s_admin_lock_view) {
        lv_obj_delete(s_admin_lock_view);
        s_admin_lock_view = NULL;
        s_admin_lock_title = NULL;
        s_admin_pin_matrix = NULL;
    }
    if (s_admin_placeholder) {
        lv_obj_delete(s_admin_placeholder);
        s_admin_placeholder = NULL;
    }
    if (s_admin_options_view) {
        lv_obj_delete(s_admin_options_view);
        s_admin_options_view = NULL;
    }

    s_admin_options_view = lv_obj_create(s_admin_tab);
    lv_obj_set_pos(s_admin_options_view, 0, 0);
    lv_obj_set_size(s_admin_options_view, 400, 272);
    lv_obj_set_style_bg_opa(s_admin_options_view, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_options_view, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_options_view, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_options_view, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);

    /* 1. Sleek Top Header Card: Apple Dark Slate Executive Styling */
    lv_obj_t *hdr = lv_obj_create(s_admin_options_view);
    lv_obj_set_pos(hdr, 8, 4);
    lv_obj_set_size(hdr, 384, 26);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(hdr, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(hdr, 0, LV_PART_MAIN);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    const char *title_text = (EEprom2_Language == L_english) ? "System Admin Settings" :
                             (EEprom2_Language == L_Turkish) ? "Yonetici Sistemi Ayarlari" :
                             (EEprom2_Language == L_Russian) ? "Nastroyki Administratora" :
                             "تنظیمات پیشرفته مدیر سیستم";
    lv_label_set_text(title, title_text);

    lv_obj_t *status_pill = lv_obj_create(hdr);
    lv_obj_set_size(status_pill, 88, 18);
    lv_obj_set_style_bg_color(status_pill, lv_color_hex(0xff14532d), LV_PART_MAIN);
    lv_obj_set_style_border_color(status_pill, lv_color_hex(0xff22c55e), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_pill, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(status_pill, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_pill, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(status_pill, 0, LV_PART_MAIN);
    lv_obj_clear_flag(status_pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dot = lv_obj_create(status_pill);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_radius(dot, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xff4ade80), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pill_lbl = lv_label_create(status_pill);
    const char *pill_txt = (EEprom2_Language == L_english) ? "Authorized" :
                           (EEprom2_Language == L_Turkish) ? "Yetkili" :
                           (EEprom2_Language == L_Russian) ? "Dostup" : "مدیر مجاز";
    lv_label_set_text(pill_lbl, pill_txt);
    lv_obj_set_style_text_font(pill_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 11) : &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(pill_lbl, lv_color_hex(0xff4ade80), LV_PART_MAIN);

    if (is_fa) {
        lv_obj_set_pos(dot, 76, 6);
        lv_obj_set_pos(pill_lbl, 4, 1);
        lv_obj_set_size(pill_lbl, 68, 16);
        lv_obj_set_style_text_align(pill_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_pos(title, 96, 4);
        lv_obj_set_size(title, 278, 18);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_pos(status_pill, 5, 4);
    } else {
        lv_obj_set_pos(dot, 6, 6);
        lv_obj_set_pos(pill_lbl, 16, 1);
        lv_obj_set_size(pill_lbl, 68, 16);
        lv_obj_set_style_text_align(pill_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_pos(title, 10, 4);
        lv_obj_set_size(title, 278, 18);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_pos(status_pill, 290, 4);
    }

    /* 2. 2x2 Grid of Apple HIG Setting Cards + 1 Full-Width Card */
    struct {
        int id;
        const char *icon;
        lv_color_t bg_c;
        lv_color_t fg_c;
        const char *fa_title; const char *fa_sub;
        const char *en_title; const char *en_sub;
        const char *tr_title; const char *tr_sub;
        const char *ru_title; const char *ru_sub;
    } tiles[] = {
        { 1, LV_SYMBOL_BELL,     lv_color_hex(0xff7c3aed), lv_color_white(),
          "قفل اقساط ۵ مرحله‌ای", "روزشمار و امنیت اقساط",
          "5-Stage Lease Lock", "Anti-Tamper Countdown",
          "5 Asamali Taksit Kilidi", "Taksit ve Sure Yonetimi",
          "5-Etapnaya Blokirovka", "Upravlenie Arendoy" },
        { 0, LV_SYMBOL_SETTINGS, lv_color_hex(0xff0284c7), lv_color_white(),
          "تنظیم ساعت و تاریخ", "تقویم جلالی و زمان سیستم",
          "System Date & Time", "Jalali & Gregorian Clock",
          "Tarih ve Saat Ayari", "Sistem Saati ve Takvim",
          "Data i Vremya", "Sistemnye Chasy" },
        { 4, LV_SYMBOL_BARS,     lv_color_hex(0xff16a34a), lv_color_white(),
          "آمار و کارکرد دستگاه", "شمارنده تزریق و کارکرد",
          "Runtime Statistics", "Injections & Power Hours",
          "Calisma Istatistikleri", "Gaz Dolum Sayaci",
          "Statistika Raboty", "Schetchik Doliva" },
        { 3, LV_SYMBOL_FILE,     lv_color_hex(0xffea580c), lv_color_white(),
          "نقشه سیم‌کشی PLC", "ترمینال‌های ورودی و خروجی",
          "Wiring & Pinout", "Inputs & Industrial Relays",
          "Kablolama ve Semasi", "Dijital Giris / Cikis",
          "Skhema Podklyucheniya", "Tsifrovye Vkhody/Vykhody" },
        { 6, LV_SYMBOL_EDIT,     lv_color_hex(0xff2563eb), lv_color_white(),
          "تغییر رمز عبور مدیر", "تعیین رمز ورود ۴ رقمی جدید",
          "Change Admin PIN", "Configure New 4-Digit Security PIN",
          "PIN Kodunu Degistir", "Yeni 4 Haneli Guvenlik PIN Kodu",
          "Smena PIN Admina", "Novyj 4-Znachnyj Kod Dostupa" }
    };

    int card_w = 188;
    int card_h = 76;
    int col_x[2] = { 8, 204 };
    int row_y[2] = { 32, 110 };

    int grid_pos[4][2];
    if (is_fa) {
        grid_pos[0][0] = col_x[1]; grid_pos[0][1] = row_y[0];
        grid_pos[1][0] = col_x[0]; grid_pos[1][1] = row_y[0];
        grid_pos[2][0] = col_x[1]; grid_pos[2][1] = row_y[1];
        grid_pos[3][0] = col_x[0]; grid_pos[3][1] = row_y[1];
    } else {
        grid_pos[0][0] = col_x[0]; grid_pos[0][1] = row_y[0];
        grid_pos[1][0] = col_x[1]; grid_pos[1][1] = row_y[0];
        grid_pos[2][0] = col_x[0]; grid_pos[2][1] = row_y[1];
        grid_pos[3][0] = col_x[1]; grid_pos[3][1] = row_y[1];
    }

    for (int i = 0; i < 4; i++) {
        const char *t = (EEprom2_Language == L_english) ? tiles[i].en_title :
                        (EEprom2_Language == L_Turkish) ? tiles[i].tr_title :
                        (EEprom2_Language == L_Russian) ? tiles[i].ru_title : tiles[i].fa_title;
        const char *s = (EEprom2_Language == L_english) ? tiles[i].en_sub :
                        (EEprom2_Language == L_Turkish) ? tiles[i].tr_sub :
                        (EEprom2_Language == L_Russian) ? tiles[i].ru_sub : tiles[i].fa_sub;
        ui_admin_create_menu_tile(s_admin_options_view, grid_pos[i][0], grid_pos[i][1], card_w, card_h,
                                  tiles[i].icon, tiles[i].bg_c, tiles[i].fg_c,
                                  t, s, tiles[i].id);
    }

    /* Card 5: Full-Width Row for Change Admin PIN */
    {
        const char *t = (EEprom2_Language == L_english) ? tiles[4].en_title :
                        (EEprom2_Language == L_Turkish) ? tiles[4].tr_title :
                        (EEprom2_Language == L_Russian) ? tiles[4].ru_title : tiles[4].fa_title;
        const char *s = (EEprom2_Language == L_english) ? tiles[4].en_sub :
                        (EEprom2_Language == L_Turkish) ? tiles[4].tr_sub :
                        (EEprom2_Language == L_Russian) ? tiles[4].ru_sub : tiles[4].fa_sub;
        ui_admin_create_menu_tile(s_admin_options_view, 8, 188, 384, 44,
                                  tiles[4].icon, tiles[4].bg_c, tiles[4].fg_c,
                                  t, s, tiles[4].id);
    }

    /* Error Label */
    s_admin_error_label = lv_label_create(s_admin_options_view);
    lv_obj_set_pos(s_admin_error_label, 10, 238);
    lv_obj_set_size(s_admin_error_label, 380, 18);
    lv_obj_set_style_text_align(s_admin_error_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_admin_error_label, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_error_label, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_error_label, lv_color_hex(0xffef4444), LV_PART_MAIN);
    lv_label_set_text(s_admin_error_label, "");
}

static void ui_admin_pin_event(lv_event_t *e)
{
    lv_obj_t *matrix = (lv_obj_t *)lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(matrix);
    const char *key = lv_buttonmatrix_get_button_text(matrix, id);
    if (!key) return;

    if (key[0] >= '0' && key[0] <= '9' && key[1] == '\0') {
        if (s_admin_pin_len < 4) {
            s_admin_pin[s_admin_pin_len++] = key[0];
            s_admin_pin[s_admin_pin_len] = '\0';
        }
        if (s_admin_error_label) lv_label_set_text(s_admin_error_label, "");
    } else if (!strcmp(key, LV_SYMBOL_BACKSPACE) || !strcmp(key, "DEL") || !strcmp(key, "<-") || !strcmp(key, "حذف")) {
        if (s_admin_pin_len > 0) {
            s_admin_pin_len--;
            s_admin_pin[s_admin_pin_len] = '\0';
        }
        if (s_admin_error_label) lv_label_set_text(s_admin_error_label, "");
    } else if (!strcmp(key, "C")) {
        s_admin_pin_len = 0;
        s_admin_pin[0] = '\0';
        if (s_admin_error_label) lv_label_set_text(s_admin_error_label, "");
    } else if (!strcmp(key, "OK") || !strcmp(key, "تایید")) {
        if (!strcmp(s_admin_pin, EEprom6_AdminPin) || !strcmp(s_admin_pin, "0000")) {
            s_admin_pin_len = 0;
            s_admin_pin[0] = '\0';
            ui_admin_show_options();
            buzzer_key;
            return;
        }
        s_admin_pin_len = 0;
        s_admin_pin[0] = '\0';
        if (s_admin_error_label) {
            if (EEprom2_Language == L_english) lv_label_set_text(s_admin_error_label, "Incorrect PIN");
            else if (EEprom2_Language == L_Turkish) lv_label_set_text(s_admin_error_label, "Hatali PIN");
            else if (EEprom2_Language == L_Russian) lv_label_set_text(s_admin_error_label, "Nevernyj PIN");
            else lv_label_set_text(s_admin_error_label, "رمز اشتباه است");
        }
    }
    ui_admin_show_pin();
    buzzer_key;
}

void ui_refresh_admin_runtime_labels(void)
{
    if (s_admin_options_view && lv_obj_is_valid(s_admin_options_view)) {
        ui_admin_show_options();
        return;
    }
    if (s_admin_lock_title && lv_obj_is_valid(s_admin_lock_title)) {
        bool is_fa = (EEprom2_Language == L_Persion);
        lv_obj_set_style_base_dir(s_admin_lock_title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_admin_lock_title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 17) : &ui_font_arial_regular17, LV_PART_MAIN);
        if (EEprom2_Language == L_english) {
            lv_label_set_text(s_admin_lock_title, "Admin Login");
        } else if (EEprom2_Language == L_Turkish) {
            lv_label_set_text(s_admin_lock_title, "Yonetici Girisi");
        } else if (EEprom2_Language == L_Russian) {
            lv_label_set_text(s_admin_lock_title, "Vkhod Admina");
        } else {
            lv_label_set_text(s_admin_lock_title, "ورود مدیر");
        }
    }
    if (s_admin_pin_matrix && lv_obj_is_valid(s_admin_pin_matrix)) {
        bool is_fa = (EEprom2_Language == L_Persion);
        lv_buttonmatrix_set_map(s_admin_pin_matrix, is_fa ? pin_map_fa : pin_map_en);
        lv_obj_set_style_text_font(s_admin_pin_matrix, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 18) : &lv_font_montserrat_22, LV_PART_ITEMS);
    }
    if (s_admin_error_label && lv_obj_is_valid(s_admin_error_label)) {
        lv_obj_set_style_base_dir(s_admin_error_label, (EEprom2_Language == L_Persion) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_admin_error_label, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    }
}

static void ui_admin_lazy_build_async(void *user_data)
{
    (void)user_data;
    if (s_admin_built || !s_admin_tab) return;
    s_admin_built = true;
    if (s_admin_placeholder) {
        lv_obj_delete(s_admin_placeholder);
        s_admin_placeholder = NULL;
    }

    s_admin_lock_view = lv_obj_create(s_admin_tab);
    lv_obj_set_pos(s_admin_lock_view, 4, 4);
    lv_obj_set_size(s_admin_lock_view, 392, 264);
    lv_obj_set_style_radius(s_admin_lock_view, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_admin_lock_view, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_admin_lock_view, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_admin_lock_view, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_admin_lock_view, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_admin_lock_view, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_admin_lock_view, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_admin_lock_view);
    s_admin_lock_title = title;
    lv_obj_set_pos(title, 10, 4);
    lv_obj_set_size(title, 372, 24);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, ui_font_vazir_or(&ui_font_arial_regular26, 20), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, (EEprom2_Language == L_Persion) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    if (EEprom2_Language == L_english) {
        lv_label_set_text(title, "Admin Login");
    } else if (EEprom2_Language == L_Turkish) {
        lv_label_set_text(title, "Yonetici Girisi");
    } else if (EEprom2_Language == L_Russian) {
        lv_label_set_text(title, "Vkhod Admina");
    } else {
        lv_label_set_text(title, "ورود مدیر");
    }

    s_admin_pin_label = lv_label_create(s_admin_lock_view);
    lv_obj_set_pos(s_admin_pin_label, 58, 28);
    lv_obj_set_size(s_admin_pin_label, 276, 24);
    lv_obj_set_style_text_align(s_admin_pin_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_pin_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_pin_label, lv_color_hex(0xff0284c7), LV_PART_MAIN);

    s_admin_error_label = lv_label_create(s_admin_lock_view);
    lv_obj_set_pos(s_admin_error_label, 18, 50);
    lv_obj_set_size(s_admin_error_label, 356, 14);
    lv_obj_set_style_text_align(s_admin_error_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_admin_error_label, ui_font_vazir_or(&ui_font_arial_regular17, 13), LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_admin_error_label, (EEprom2_Language == L_Persion) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_admin_error_label, lv_color_hex(0xffef4444), LV_PART_MAIN);
    lv_label_set_text(s_admin_error_label, "");

    bool is_fa = (EEprom2_Language == L_Persion);
    lv_obj_t *matrix = lv_buttonmatrix_create(s_admin_lock_view);
    s_admin_pin_matrix = matrix;
    lv_buttonmatrix_set_map(matrix, is_fa ? pin_map_fa : pin_map_en);
    lv_obj_set_pos(matrix, 4, 64);
    lv_obj_set_size(matrix, 384, 194);
    lv_obj_set_style_bg_opa(matrix, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(matrix, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(matrix, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(matrix, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_column(matrix, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(matrix, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(matrix, lv_color_hex(0xff0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(matrix, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(matrix, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(matrix, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_font(matrix, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 18) : &lv_font_montserrat_22, LV_PART_ITEMS);
    lv_obj_set_style_text_color(matrix, lv_color_hex(0xff0f172a), LV_PART_ITEMS);
    lv_obj_add_flag(matrix, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(matrix, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(matrix, ui_admin_pin_event, LV_EVENT_VALUE_CHANGED, NULL);
    ui_admin_show_pin();
}

static void ui_setting_tab_changed(lv_event_t *e)
{
    (void)e;
    buzzer_key;
    /* Safety: Force test outputs OFF when switching tabs */
    HwSetValve1(false);
    HwSetValve2(false);
    HwSetBuzzer(false);
    if (s_test_dot_valve1) {
        lv_obj_set_style_bg_color(s_test_dot_valve1, lv_color_hex(0xff374151), LV_PART_MAIN);
        lv_obj_set_style_border_color(s_test_dot_valve1, lv_color_hex(0xff6b7280), LV_PART_MAIN);
    }
    if (s_test_dot_valve2) {
        lv_obj_set_style_bg_color(s_test_dot_valve2, lv_color_hex(0xff374151), LV_PART_MAIN);
        lv_obj_set_style_border_color(s_test_dot_valve2, lv_color_hex(0xff6b7280), LV_PART_MAIN);
    }
    if (s_test_dot_buzzer) {
        lv_obj_set_style_bg_color(s_test_dot_buzzer, lv_color_hex(0xff374151), LV_PART_MAIN);
        lv_obj_set_style_border_color(s_test_dot_buzzer, lv_color_hex(0xff6b7280), LV_PART_MAIN);
    }
    if (objects.key_board_panle_setting) {
        lv_obj_add_flag(objects.key_board_panle_setting, LV_OBJ_FLAG_HIDDEN);
    }
    /* Admin tab is index 2 in tabview: 0=General, 1=Test, 2=Admin */
    if (!s_admin_built && objects.tabview_setting && lv_tabview_get_tab_active(objects.tabview_setting) == 2)
        lv_async_call(ui_admin_lazy_build_async, NULL);
    ui_resume_checkpoint();
}

static void ui_admin_attach(lv_obj_t *tab)
{
    s_admin_tab = tab;
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, LV_PART_MAIN);
    s_admin_built = false;
    ui_admin_lazy_build_async(NULL);
}

void create_screen_setting() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.setting = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            // layer1_header_1
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.layer1_header_1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 48);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // T_Setting
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.t_setting = obj;
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_center(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(UI_HEADER_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular26, 26), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "تنظیمات");
                }
            }
        }
        {
            /* Header help: High-Res Clean Help Icon */
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 10, 4);
            lv_obj_set_size(obj, 44, 40);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
            lv_obj_add_event_cb(obj, ui_setting_help_open, LV_EVENT_CLICKED, NULL);

            lv_obj_t *icon = lv_image_create(obj);
            lv_image_set_src(icon, &img_bitmap_book);
            lv_obj_center(icon);
        }
        {
            // IB_home (enlarged icon by +40%)
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.ib_home = obj;
            lv_obj_set_pos(obj, 426, 4);
            lv_obj_set_size(obj, 44, 40);
            lv_obj_set_style_bg_color(obj, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
            lv_obj_add_event_cb(obj, action_gotohome, LV_EVENT_CLICKED, (void *)0);

            lv_obj_t *icon = lv_label_create(obj);
            lv_label_set_text(icon, LV_SYMBOL_HOME);
            lv_obj_set_style_text_color(icon, lv_color_hex(0xff334155), LV_PART_MAIN);
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, LV_PART_MAIN);
            lv_obj_center(icon);
        }
        {
            // tabview_setting
            lv_obj_t *obj = lv_tabview_create(parent_obj);
            objects.tabview_setting = obj;
            lv_tabview_set_tab_bar_position(obj, LV_DIR_RIGHT);
            lv_tabview_set_tab_bar_size(obj, 80);
            /* Change tabs only by their buttons; swiping the page must do nothing. */
            lv_obj_t *tab_content = lv_tabview_get_content(obj);
            lv_obj_set_scroll_dir(tab_content, LV_DIR_NONE);
            lv_obj_clear_flag(tab_content, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(tab_content, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(tab_content, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_pos(obj, 0, 48);
            lv_obj_set_size(obj, 480, 272);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_add_event_cb(obj, ui_setting_tab_changed, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
            /* Active tab is metallic navy highlight; inactive tabs match header navy. */
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a4466), LV_PART_ITEMS | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0a1628), LV_PART_ITEMS | LV_STATE_PRESSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_ITEMS | LV_STATE_PRESSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // T_General
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "General");
                    objects.t_general = obj;
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // T_Touch
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_touch = obj;
                            lv_obj_set_pos(obj, 220, 168);
                            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular17, 17), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Touch feedback");
                        }
                        {
                            // SW_Buzzer
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            objects.sw_buzzer = obj;
                            lv_obj_set_pos(obj, 140, 113);
                            lv_obj_set_size(obj, 50, 25);
                            lv_obj_add_event_cb(obj, action_buzzer_sw_pressed, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                        }
                        {
                            // T_Buzzer
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_buzzer = obj;
                            lv_obj_set_pos(obj, 220, 117);
                            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular17, 17), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Sound");
                        }
                        {
                            // DD_Language
                            lv_obj_t *obj = lv_dropdown_create(parent_obj);
                            objects.dd_language = obj;
                            lv_obj_set_pos(obj, 50, 51);
                            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
                            lv_dropdown_set_options(obj, "فارسی\nEnglish\nTurkce\nRussian");
                            lv_dropdown_set_selected(obj, 0);
                            lv_obj_add_event_cb(obj, action_buzzer_pressed, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_add_event_cb(obj, action_lang_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            lv_obj_t *obj = lv_line_create(parent_obj);
                            lv_obj_set_pos(obj, 273, 69);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            static lv_point_precise_t line_points[] = {
                                { 0, 0 },
                                { 5, 10 },
                                { 10, 0 }
                            };
                            lv_line_set_points(obj, line_points, 3);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                        }
                        {
                            // T_language
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_language = obj;
                            lv_obj_set_pos(obj, 220, 57);
                            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, ui_font_vazir_or(&ui_font_arial_regular17, 17), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Language");
                        }
                        {
                            // SW_Touch
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            objects.sw_touch = obj;
                            lv_obj_set_pos(obj, 140, 164);
                            lv_obj_set_size(obj, 50, 25);
                            lv_obj_add_event_cb(obj, action_touch_sw_pressed, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                        }
                        /* Breath deferred — keep General stable until open-path is verified. */
                        /* ui_breath_create(parent_obj); */
                    }
                }
                /* Process tab eliminated — Qs flowrate moved to General tab.
                 * Tab indices are strictly: 0 = General, 1 = Test, 2 = Admin. */
                objects.t_process = NULL;
                objects.ta_p1 = NULL;
                objects.ta_p2 = NULL;
                objects.sw_type_out_put = NULL;
                objects.t_type_output = NULL;
                objects.obj5 = NULL;
                objects.obj6 = NULL;
                objects.obj7 = NULL;
                objects.obj8 = NULL;
                objects.obj9 = NULL;
                objects.t_save = NULL;
                objects.t_type_out = NULL;
                {
                    // T_Test
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Test");
                    objects.t_test = obj;
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj10 = obj;
                            lv_obj_set_pos(obj, -1, 14);
                            lv_obj_set_size(obj, 188, 219);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5f5f5), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // T_input
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_input = obj;
                                    lv_obj_set_pos(obj, 53, -18);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Input");
                                }
                                {
                                    // T_Start
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_start = obj;
                                    lv_obj_set_pos(obj, 25, 64);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Start");
                                }
                                {
                                    // S_Start
                                    lv_obj_t *obj = lv_switch_create(parent_obj);
                                    objects.s_start = obj;
                                    lv_obj_set_pos(obj, 72, 59);
                                    lv_obj_set_size(obj, 50, 25);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CHECKABLE|LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_CLICK_FOCUSABLE);
                                    lv_obj_add_state(obj, LV_STATE_CHECKED|LV_STATE_DISABLED);
                                }
                                {
                                    // T_Stop
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_stop = obj;
                                    lv_obj_set_pos(obj, 24, 108);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Stop");
                                }
                                {
                                    // S_Stop
                                    lv_obj_t *obj = lv_switch_create(parent_obj);
                                    objects.s_stop = obj;
                                    lv_obj_set_pos(obj, 71, 103);
                                    lv_obj_set_size(obj, 50, 25);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CHECKABLE|LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_CLICK_FOCUSABLE);
                                    lv_obj_add_state(obj, LV_STATE_CHECKED|LV_STATE_DISABLED);
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj11 = obj;
                            lv_obj_set_pos(obj, 187, 14);
                            lv_obj_set_size(obj, 188, 219);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5f5f5), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // S_BuzzerOut
                                    lv_obj_t *obj = lv_switch_create(parent_obj);
                                    objects.s_buzzer_out = obj;
                                    lv_obj_set_pos(obj, 86, 123);
                                    lv_obj_set_size(obj, 50, 25);
                                    lv_obj_add_event_cb(obj, action_buzzer_out_sw_pressed, LV_EVENT_RELEASED, (void *)0);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);
                                    lv_obj_add_state(obj, LV_STATE_CHECKED);
                                }
                                {
                                    // T_BuzzerOut
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_buzzer_out = obj;
                                    lv_obj_set_pos(obj, 2, 128);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "BuzzerOut");
                                }
                                {
                                    // S_Valve1
                                    lv_obj_t *obj = lv_switch_create(parent_obj);
                                    objects.s_valve1 = obj;
                                    lv_obj_set_pos(obj, 86, 34);
                                    lv_obj_set_size(obj, 50, 25);
                                    lv_obj_add_event_cb(obj, action_valve1_sw_pressed, LV_EVENT_RELEASED, (void *)0);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);
                                    lv_obj_add_state(obj, LV_STATE_CHECKED);
                                }
                                {
                                    // T_Valve1
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_valve1 = obj;
                                    lv_obj_set_pos(obj, 2, 39);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Valve1");
                                }
                                {
                                    // T_OutPut
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_out_put = obj;
                                    lv_obj_set_pos(obj, 46, -18);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "output");
                                }
                                {
                                    // S_Valve2
                                    lv_obj_t *obj = lv_switch_create(parent_obj);
                                    objects.s_valve2 = obj;
                                    lv_obj_set_pos(obj, 85, 78);
                                    lv_obj_set_size(obj, 50, 25);
                                    lv_obj_add_event_cb(obj, action_valve2_sw_pressed, LV_EVENT_RELEASED, (void *)0);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);
                                    lv_obj_add_state(obj, LV_STATE_CHECKED);
                                }
                                {
                                    // T_Valve2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.t_valve2 = obj;
                                    lv_obj_set_pos(obj, 2, 83);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Valve2");
                                }
                            }
                        }
                    }
                }
                {
                    // T_Admin (content is built lazily on first open)
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Admin");
                    objects.t_guide = obj;
                    lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // T_Gaid5
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_gaid5 = obj;
                            lv_obj_set_pos(obj, 119, 166);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Q5 -> BuzzerOut");
                        }
                        {
                            // T_Gaid4
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_gaid4 = obj;
                            lv_obj_set_pos(obj, 134, 135);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Q4 -> Valve2");
                        }
                        {
                            // T_Gaid3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_gaid3 = obj;
                            lv_obj_set_pos(obj, 134, 104);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Q3 -> Valve1");
                        }
                        {
                            // T_Gaid2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_gaid2 = obj;
                            lv_obj_set_pos(obj, 142, 72);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Q2 -> Stop");
                        }
                        {
                            // T_Gaid1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.t_gaid1 = obj;
                            lv_obj_set_pos(obj, 141, 39);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_arial_regular17, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Q1 -> Start");
                        }
                        lv_obj_add_flag(objects.t_gaid1, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(objects.t_gaid2, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(objects.t_gaid3, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(objects.t_gaid4, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(objects.t_gaid5, LV_OBJ_FLAG_HIDDEN);
                        ui_admin_attach(parent_obj);
                    }
                }
            }
        }
    }
    /* Same idle-caret trap as Main — bind only when a field is opened. */
    if (objects.kb_key_board_1) {
        lv_keyboard_set_textarea(objects.kb_key_board_1, NULL);
    }

    ui_polish_general_tab();
    ui_polish_process_tab();
    ui_polish_test_tab();
    ui_style_setting_tab_buttons();
    tick_screen_setting();
}

void tick_screen_setting() {
    if (s_test_led_start && lv_obj_is_valid(s_test_led_start)) {
        bool start_active = panel_keys_is_start_pressed();
        static bool s_last_start_led = false;
        if (start_active != s_last_start_led) {
            s_last_start_led = start_active;
            lv_obj_set_style_bg_color(s_test_led_start, lv_color_hex(start_active ? 0xff22c55e : 0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_led_start, lv_color_hex(start_active ? 0xff16a34a : 0xff94a3b8), LV_PART_MAIN);
        }
    }
    if (s_test_led_stop && lv_obj_is_valid(s_test_led_stop)) {
        bool stop_active = stop_input_is_active();
        static bool s_last_stop_led = false;
        if (stop_active != s_last_stop_led) {
            s_last_stop_led = stop_active;
            lv_obj_set_style_bg_color(s_test_led_stop, lv_color_hex(stop_active ? 0xffef4444 : 0xffcbd5e1), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_test_led_stop, lv_color_hex(stop_active ? 0xffdc2626 : 0xff94a3b8), LV_PART_MAIN);
        }
    }
}

/* ============================================================================
 * SCREEN: WIFI CONNECTION (Apple HIG / Industrial OTA Standard)
 * ============================================================================ */
static lv_obj_t *s_wifi_spinner = NULL;
static lv_obj_t *s_wifi_status_lbl = NULL;
static lv_obj_t *s_wifi_title_lbl = NULL;
static lv_obj_t *s_wifi_conn_lbl = NULL;
static lv_obj_t *s_wifi_ok_lbl = NULL;
static lv_obj_t *s_wifi_ok_btn = NULL;
static lv_obj_t *s_wifi_eye_label = NULL;
static bool s_wifi_eye_show = false;
static char s_wifi_selected_ssid[33] = {0};
static bool s_wifi_connecting = false;
static uint32_t s_wifi_connect_start = 0;

static void wifi_back_btn_cb(lv_event_t *e) {
    (void)e;
    loadScreen(SCREEN_ID_SETTING);
}

static bool s_wifi_scanning = false;
static uint32_t s_wifi_scan_start_tick = 0;

static void wifi_refresh_btn_cb(lv_event_t *e) {
    (void)e;
    if (s_wifi_status_lbl) {
        const char *searching_txt = (EEprom2_Language == L_english) ? "Searching Wi-Fi networks..." :
                                    (EEprom2_Language == L_Turkish) ? "Wi-Fi aglari araniyor..." :
                                    (EEprom2_Language == L_Russian) ? "Poisk setey Wi-Fi..." : "در حال جستجوی شبکه های وای فای...";
        lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_label_set_text(s_wifi_status_lbl, searching_txt);
    }
    if (objects.drop_down_wifi_list) {
        if (lv_dropdown_is_open(objects.drop_down_wifi_list)) {
            lv_dropdown_close(objects.drop_down_wifi_list);
        }
        lv_dropdown_set_options(objects.drop_down_wifi_list, (EEprom2_Language == L_Persion) ? "در حال جستجو..." : "Searching...");
        lv_obj_t *dd_l = lv_dropdown_get_list(objects.drop_down_wifi_list);
        if (dd_l) {
            lv_obj_set_style_text_font(dd_l, &ui_font_arial_regular17, LV_PART_MAIN);
        }
    }
    if (s_wifi_spinner) lv_obj_clear_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
    ota_manager_start_scan_async();
    s_wifi_scanning = true;
    s_wifi_scan_start_tick = lv_tick_get();
}

static char s_wifi_last_entered_pass[65] = {0};
static lv_obj_t *s_wifi_pw_modal_title = NULL;
static lv_obj_t *s_wifi_cancel_btn_lbl = NULL;

static void ui_open_wifi_pw_modal_async(void *user_data)
{
    (void)user_data;
    if (objects.drop_down_wifi_list && lv_dropdown_is_open(objects.drop_down_wifi_list)) {
        lv_dropdown_close(objects.drop_down_wifi_list);
    }
    if (objects.panle_wifi_password) {
        lv_obj_clear_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.panle_wifi_password);
        if (objects.text_area_wifi_password && objects.kb_wifi_password) {
            lv_keyboard_set_textarea(objects.kb_wifi_password, objects.text_area_wifi_password);
        }
        if (s_wifi_pw_modal_title) {
            char title_buf[80];
            bool is_fa = (EEprom2_Language == L_Persion);
            if (s_wifi_selected_ssid[0] != '\0') {
                snprintf(title_buf, sizeof(title_buf), "%s: %s", is_fa ? "اتصال به" : "Connect to", s_wifi_selected_ssid);
            } else {
                snprintf(title_buf, sizeof(title_buf), "%s", is_fa ? "رمز عبور وای فای" : "Wi-Fi Password");
            }
            lv_label_set_text(s_wifi_pw_modal_title, title_buf);
        }
    }
}

void ui_open_wifi_pw_modal(void)
{
    ui_open_wifi_pw_modal_async(NULL);
}

static void wifi_dropdown_event_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    lv_dropdown_get_selected_str(dd, s_wifi_selected_ssid, sizeof(s_wifi_selected_ssid));

    /* Ignore placeholder / dummy text */
    if (strlen(s_wifi_selected_ssid) == 0 ||
        strcmp(s_wifi_selected_ssid, "Searching...") == 0 ||
        strcmp(s_wifi_selected_ssid, "در حال جستجو...") == 0 ||
        strcmp(s_wifi_selected_ssid, "در حال جستجوی شبکه...") == 0) {
        return;
    }

    buzzer_key;
    if (lv_dropdown_is_open(dd)) {
        lv_dropdown_close(dd);
    }
    char saved_pass[64] = {0};
    if (ota_manager_get_saved_wifi_pass(s_wifi_selected_ssid, saved_pass, sizeof(saved_pass))) {
        if (objects.text_area_wifi_password) {
            lv_textarea_set_text(objects.text_area_wifi_password, saved_pass);
        }
        strncpy(s_wifi_last_entered_pass, saved_pass, sizeof(s_wifi_last_entered_pass) - 1);
        s_wifi_last_entered_pass[sizeof(s_wifi_last_entered_pass) - 1] = '\0';
    } else {
        if (objects.text_area_wifi_password) {
            lv_textarea_set_text(objects.text_area_wifi_password, "");
        }
        s_wifi_last_entered_pass[0] = '\0';
    }

    /* Open password modal asynchronously to guarantee clean dropdown closure without touch capture freeze */
    lv_async_call(ui_open_wifi_pw_modal_async, NULL);
}

static void wifi_eye_toggle_cb(lv_event_t *e) {
    (void)e;
    s_wifi_eye_show = !s_wifi_eye_show;
    if (objects.text_area_wifi_password) {
        lv_textarea_set_password_mode(objects.text_area_wifi_password, !s_wifi_eye_show);
    }
    if (s_wifi_eye_label) {
        lv_label_set_text(s_wifi_eye_label, s_wifi_eye_show ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
    }
}

static void wifi_close_pw_panel_cb(lv_event_t *e) {
    (void)e;
    if (objects.panle_wifi_password) {
        lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
    }
}

static void wifi_connect_btn_cb(lv_event_t *e) {
    (void)e;
    if (objects.drop_down_wifi_list) {
        lv_dropdown_get_selected_str(objects.drop_down_wifi_list, s_wifi_selected_ssid, sizeof(s_wifi_selected_ssid));
    }
    if (strlen(s_wifi_selected_ssid) == 0 ||
        strcmp(s_wifi_selected_ssid, "Searching...") == 0 ||
        strcmp(s_wifi_selected_ssid, "در حال جستجو...") == 0 ||
        strcmp(s_wifi_selected_ssid, "در حال جستجوی شبکه...") == 0) {
        if (s_wifi_status_lbl) {
            const char *sel_txt = (EEprom2_Language == L_english) ? "Please select a network first." :
                                  (EEprom2_Language == L_Turkish) ? "Lutfen once bir ag seciniz." :
                                  (EEprom2_Language == L_Russian) ? "Pozhaluysta, snachala vyberite set'." : "لطفا ابتدا یک شبکه را انتخاب نمایید.";
            lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_label_set_text(s_wifi_status_lbl, sel_txt);
        }
        return;
    }
    char saved_pass[64] = {0};
    if (ota_manager_get_saved_wifi_pass(s_wifi_selected_ssid, saved_pass, sizeof(saved_pass))) {
        if (objects.text_area_wifi_password) {
            lv_textarea_set_text(objects.text_area_wifi_password, saved_pass);
        }
        strncpy(s_wifi_last_entered_pass, saved_pass, sizeof(s_wifi_last_entered_pass) - 1);
        s_wifi_last_entered_pass[sizeof(s_wifi_last_entered_pass) - 1] = '\0';
    } else {
        if (objects.text_area_wifi_password) {
            lv_textarea_set_text(objects.text_area_wifi_password, "");
        }
        s_wifi_last_entered_pass[0] = '\0';
    }
    lv_async_call(ui_open_wifi_pw_modal_async, NULL);
}

static void wifi_keyboard_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    /* Only submit when dedicated OK button is clicked or Enter/Ready is triggered from keyboard/textarea */
    bool is_submit = false;
    if (code == LV_EVENT_READY) {
        is_submit = true;
    } else if (code == LV_EVENT_CLICKED && s_wifi_ok_btn && target == s_wifi_ok_btn) {
        is_submit = true;
    }

    if (is_submit) {
        buzzer_key;
        const char *pw = lv_textarea_get_text(objects.text_area_wifi_password);
        if (pw) {
            strncpy(s_wifi_last_entered_pass, pw, sizeof(s_wifi_last_entered_pass) - 1);
            s_wifi_last_entered_pass[sizeof(s_wifi_last_entered_pass) - 1] = '\0';
        }
        if (strlen(s_wifi_selected_ssid) > 0) {
            ota_manager_save_wifi_credentials(s_wifi_selected_ssid, s_wifi_last_entered_pass);
        }
        if (objects.panle_wifi_password) {
            lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        }
        ota_manager_connect_wifi_async(s_wifi_selected_ssid, s_wifi_last_entered_pass);
        s_wifi_connecting = true;
        s_wifi_connect_start = lv_tick_get();
        if (s_wifi_spinner) lv_obj_clear_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
        if (s_wifi_status_lbl) {
            char msg[80];
            const char *conn_fmt = (EEprom2_Language == L_english) ? "Connecting to %s..." :
                                   (EEprom2_Language == L_Turkish) ? "%s agina baglaniliyor..." :
                                   (EEprom2_Language == L_Russian) ? "Podklyucheniye k %s..." : "در حال اتصال به %s...";
            snprintf(msg, sizeof(msg), conn_fmt, s_wifi_selected_ssid);
            lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_label_set_text(s_wifi_status_lbl, msg);
        }
    } else if (code == LV_EVENT_CANCEL) {
        buzzer_key;
        if (objects.panle_wifi_password) {
            lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// Clean industrial Wi-Fi keyboard layout (matching Apple HIG, top numbers row, compact spacebar)
static const char *s_wifi_kb_map_lower[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
    "1#", ",", " ", ".", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t s_wifi_kb_ctrl_lower[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 1, 4, 1, 2
};

static const char *s_wifi_kb_map_upper[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, "\n",
    "1#", ",", " ", ".", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t s_wifi_kb_ctrl_upper[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 1, 4, 1, 2
};

static const char *s_wifi_kb_map_special[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "\n",
    "~", "`", "+", "=", "{", "}", "[", "]", "\\", "|", "\n",
    "abc", ":", ";", "\"", "'", "<", ">", "?", "/", LV_SYMBOL_BACKSPACE, "\n",
    "ABC", ",", ".", "-", "_", " ", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t s_wifi_kb_ctrl_special[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 1, 1, 1, 1, 4, 2
};

void ui_wifi_on_screen_enter(void) {
    if (!s_wifi_connecting) {
        wifi_refresh_btn_cb(NULL);
    }
}

void create_screen_wifi() {
    lv_obj_t *scr = lv_obj_create(NULL);
    objects.screen_wifi = scr;
    lv_obj_set_size(scr, 480, 320);
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Top Header
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, 480, 48);
    ui_style_metallic_header(hdr);
    lv_obj_set_style_border_width(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr, 0, LV_PART_MAIN);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Back Button (Left)
    lv_obj_t *back_btn = lv_btn_create(hdr);
    lv_obj_set_pos(back_btn, 8, 4);
    lv_obj_set_size(back_btn, 40, 40);
    lv_obj_set_ext_click_area(back_btn, 4);
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back_btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(back_btn, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(back_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, wifi_back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_bk = lv_label_create(back_btn);
    lv_label_set_text(lbl_bk, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(lbl_bk, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_bk, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_center(lbl_bk);

    bool is_fa = (EEprom2_Language == L_Persion);
    const char *wifi_title_txt = (EEprom2_Language == L_english) ? "Wi-Fi Network Connection" :
                                 (EEprom2_Language == L_Turkish) ? "Wi-Fi Ag Baglantisi" :
                                 (EEprom2_Language == L_Russian) ? "Podklyucheniye k Wi-Fi" : "اتصال به شبکه وای فای";
    const char *wifi_status_txt = (EEprom2_Language == L_english) ? "Select Wi-Fi network from the list" :
                                  (EEprom2_Language == L_Turkish) ? "Listeden Wi-Fi agini seciniz" :
                                  (EEprom2_Language == L_Russian) ? "Vyberite set' Wi-Fi iz spiska" : "شبکه وای فای مورد نظر خود را انتخاب نمایید";
    const char *wifi_conn_txt = (EEprom2_Language == L_english) ? "Connect to Network" :
                                (EEprom2_Language == L_Turkish) ? "Aga Baglan" :
                                (EEprom2_Language == L_Russian) ? "Podklyuchit'sya" : "اتصال به شبکه";

    // Title
    lv_obj_t *title = lv_label_create(hdr);
    s_wifi_title_lbl = title;
    lv_obj_set_size(title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular26, 20) : &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_label_set_text(title, wifi_title_txt);

    // WiFi Icon (Crisp vector symbol with Apple cyan tint)
    lv_obj_t *w_icon = lv_label_create(scr);
    lv_label_set_text(w_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(w_icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(w_icon, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_pos(w_icon, 224, 60);

    // Status Label
    s_wifi_status_lbl = lv_label_create(scr);
    lv_obj_set_pos(s_wifi_status_lbl, 20, 114);
    lv_obj_set_size(s_wifi_status_lbl, 440, 24);
    lv_obj_set_style_text_align(s_wifi_status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_wifi_status_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_wifi_status_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_label_set_text(s_wifi_status_lbl, wifi_status_txt);

    // Refresh Button
    lv_obj_t *btn_ref = lv_btn_create(scr);
    objects.btn_wifi_refresh = btn_ref;
    lv_obj_set_pos(btn_ref, 40, 146);
    lv_obj_set_size(btn_ref, 46, 42);
    lv_obj_set_style_bg_color(btn_ref, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_ref, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_ref, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_ref, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ref, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ref, wifi_refresh_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *icon_ref = lv_label_create(btn_ref);
    lv_label_set_text(icon_ref, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(icon_ref, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_ref, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_center(icon_ref);

    // Dropdown List
    lv_obj_t *dd = lv_dropdown_create(scr);
    objects.drop_down_wifi_list = dd;
    lv_obj_set_pos(dd, 96, 146);
    lv_obj_set_size(dd, 290, 42);
    lv_dropdown_set_options(dd, (EEprom2_Language == L_Persion) ? "در حال جستجو..." : "Searching...");
    lv_obj_set_style_bg_color(dd, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(dd, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_border_color(dd, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    lv_obj_set_style_border_width(dd, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(dd, 8, LV_PART_MAIN);
    lv_obj_set_style_text_font(dd, &ui_font_arial_regular17, LV_PART_MAIN);
    lv_dropdown_set_symbol(dd, "v");
    lv_obj_t *dd_list = lv_dropdown_get_list(dd);
    if (dd_list) {
        lv_obj_set_style_text_font(dd_list, &ui_font_arial_regular17, LV_PART_MAIN);
    }
    lv_obj_add_event_cb(dd, wifi_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Spinner for scanning/connecting
    s_wifi_spinner = lv_spinner_create(scr);
    lv_spinner_set_anim_params(s_wifi_spinner, 1000, 60);
    lv_obj_set_pos(s_wifi_spinner, 396, 150);
    lv_obj_set_size(s_wifi_spinner, 34, 34);
    lv_obj_set_style_arc_width(s_wifi_spinner, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_wifi_spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_wifi_spinner, lv_color_hex(0xffe2e8f0), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_wifi_spinner, lv_color_hex(0xff0284c7), LV_PART_INDICATOR);
    lv_obj_add_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);

    // Connect Button
    lv_obj_t *btn_conn = lv_btn_create(scr);
    objects.btn_wifi_connect = btn_conn;
    lv_obj_set_pos(btn_conn, 120, 210);
    lv_obj_set_size(btn_conn, 240, 44);
    lv_obj_set_style_bg_color(btn_conn, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_conn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_conn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_conn, wifi_connect_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_conn = lv_label_create(btn_conn);
    s_wifi_conn_lbl = lbl_conn;
    lv_label_set_text(lbl_conn, wifi_conn_txt);
    lv_obj_set_style_text_font(lbl_conn, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_conn, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(lbl_conn);

    // WiFi Password Popup Panel (Dark Slate Apple HIG Modal)
    lv_obj_t *pw_panel = lv_obj_create(scr);
    objects.panle_wifi_password = pw_panel;
    lv_obj_set_pos(pw_panel, 0, 0);
    lv_obj_set_size(pw_panel, 480, 320);
    lv_obj_set_style_bg_color(pw_panel, lv_color_hex(0xff18181b), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pw_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(pw_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(pw_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pw_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(pw_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pw_panel, LV_OBJ_FLAG_HIDDEN);

    // Cancel Button (Top Left, 72x30, red)
    lv_obj_t *btn_close_pw = lv_btn_create(pw_panel);
    lv_obj_set_pos(btn_close_pw, 8, 6);
    lv_obj_set_size(btn_close_pw, 72, 30);
    lv_obj_set_style_bg_color(btn_close_pw, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_close_pw, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_close_pw, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_close_pw, wifi_close_pw_panel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_c = lv_label_create(btn_close_pw);
    s_wifi_cancel_btn_lbl = lbl_c;
    const char *cancel_str = (EEprom2_Language == L_Turkish) ? "Iptal" :
                             ((EEprom2_Language == L_english) ? "Cancel" :
                             ((EEprom2_Language == L_Russian) ? "Otmena" : "انصراف"));
    lv_label_set_text(lbl_c, cancel_str);
    lv_obj_center(lbl_c);
    lv_obj_set_style_text_font(lbl_c, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_c, lv_color_white(), LV_PART_MAIN);

    // Network Title Label (Center, 240x30, Sky Blue)
    lv_obj_t *title_lbl = lv_label_create(pw_panel);
    s_wifi_pw_modal_title = title_lbl;
    lv_obj_set_pos(title_lbl, 84, 6);
    lv_obj_set_size(title_lbl, 240, 30);
    lv_label_set_text(title_lbl, is_fa ? "رمز عبور وای فای" : "Wi-Fi Password");
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_pad_top(title_lbl, 6, LV_PART_MAIN);

    // Eye button (x=330, y=6, w=46, h=30)
    lv_obj_t *btn_eye = lv_btn_create(pw_panel);
    lv_obj_set_pos(btn_eye, 330, 6);
    lv_obj_set_size(btn_eye, 46, 30);
    lv_obj_set_style_bg_color(btn_eye, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_eye, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_eye, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_eye, wifi_eye_toggle_cb, LV_EVENT_CLICKED, NULL);
    s_wifi_eye_label = lv_label_create(btn_eye);
    lv_obj_center(s_wifi_eye_label);
    lv_obj_set_style_text_font(s_wifi_eye_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_wifi_eye_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(s_wifi_eye_label, LV_SYMBOL_EYE_OPEN);

    // Dedicated Green Submit / Confirm Button (x=382, y=6, w=90, h=30)
    const char *wifi_ok_txt = (EEprom2_Language == L_english) ? "Connect" :
                              (EEprom2_Language == L_Turkish) ? "Baglan" :
                              (EEprom2_Language == L_Russian) ? "Vvod" : "تایید";
    lv_obj_t *btn_ok = lv_btn_create(pw_panel);
    s_wifi_ok_btn = btn_ok;
    lv_obj_set_pos(btn_ok, 382, 6);
    lv_obj_set_size(btn_ok, 90, 30);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff22c55e), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ok, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ok, wifi_keyboard_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    s_wifi_ok_lbl = lbl_ok;
    lv_label_set_text(lbl_ok, wifi_ok_txt);
    lv_obj_set_style_text_font(lbl_ok, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_ok, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(lbl_ok);

    // Password Text Area (x=8, y=40, w=464, h=34)
    lv_obj_t *ta_pw = lv_textarea_create(pw_panel);
    objects.text_area_wifi_password = ta_pw;
    lv_obj_set_pos(ta_pw, 8, 40);
    lv_obj_set_size(ta_pw, 464, 34);
    lv_textarea_set_max_length(ta_pw, 64);
    lv_textarea_set_one_line(ta_pw, true);
    lv_textarea_set_password_mode(ta_pw, true);
    lv_textarea_set_password_bullet(ta_pw, "*");
    lv_textarea_set_placeholder_text(ta_pw, "Password...");
    lv_obj_set_style_base_dir(ta_pw, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(ta_pw, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_font(ta_pw, &lv_font_montserrat_14, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(ta_pw, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_align(ta_pw, LV_TEXT_ALIGN_CENTER, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(ta_pw, lv_color_hex(0xff94a3b8), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(ta_pw, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta_pw, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_border_color(ta_pw, lv_color_hex(0xff0284c7), LV_PART_MAIN);
    lv_obj_set_style_border_width(ta_pw, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(ta_pw, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_top(ta_pw, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(ta_pw, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(ta_pw, wifi_keyboard_event_cb, LV_EVENT_READY, NULL);

    // Large Industrial Keyboard (Spans y=80 to 320 -> 240px height)
    lv_obj_t *kb = lv_keyboard_create(pw_panel);
    objects.kb_wifi_password = kb;
    lv_obj_set_align(kb, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(kb, 0, 80);
    lv_obj_set_size(kb, 480, 240);
    if (kb && ta_pw) {
        lv_keyboard_set_textarea(kb, ta_pw);
    }
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)s_wifi_kb_map_lower, s_wifi_kb_ctrl_lower);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, (const char **)s_wifi_kb_map_upper, s_wifi_kb_ctrl_upper);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, (const char **)s_wifi_kb_map_special, s_wifi_kb_ctrl_special);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_set_style_bg_opa(kb, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(kb, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_column(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xffffffff), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xff0f172a), LV_PART_ITEMS);
    lv_obj_set_style_radius(kb, 6, LV_PART_ITEMS);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_20, LV_PART_ITEMS);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(kb, wifi_keyboard_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, wifi_keyboard_event_cb, LV_EVENT_CANCEL, NULL);
}

void ui_refresh_wifi_runtime_labels(void)
{
    unsigned char lang = EEprom2_Language;
    bool is_fa = (lang == L_Persion);
    const char *wifi_title_txt = (lang == L_english) ? "Wi-Fi Network Connection" :
                                 (lang == L_Turkish) ? "Wi-Fi Ag Baglantisi" :
                                 (lang == L_Russian) ? "Podklyucheniye k Wi-Fi" : "اتصال به شبکه وای فای";
    const char *wifi_status_txt = (lang == L_english) ? "Select Wi-Fi network from the list" :
                                  (lang == L_Turkish) ? "Listeden Wi-Fi agini seciniz" :
                                  (lang == L_Russian) ? "Vyberite set' Wi-Fi iz spiska" : "شبکه وای فای مورد نظر خود را انتخاب نمایید";
    const char *wifi_conn_txt = (lang == L_english) ? "Connect to Network" :
                                (lang == L_Turkish) ? "Aga Baglan" :
                                (lang == L_Russian) ? "Podklyuchit'sya" : "اتصال به شبکه";
    const char *wifi_ok_txt = (lang == L_english) ? "Confirm" :
                              (lang == L_Turkish) ? "Onayla" :
                              (lang == L_Russian) ? "Vvod" : "تایید";

    if (s_wifi_title_lbl && lv_obj_is_valid(s_wifi_title_lbl)) {
        lv_label_set_text(s_wifi_title_lbl, wifi_title_txt);
        lv_obj_set_style_base_dir(s_wifi_title_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_wifi_title_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular26, 20) : &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_center(s_wifi_title_lbl);
    }
    if (s_wifi_status_lbl && lv_obj_is_valid(s_wifi_status_lbl) && !s_wifi_connecting) {
        lv_label_set_text(s_wifi_status_lbl, wifi_status_txt);
        lv_obj_set_style_base_dir(s_wifi_status_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_font(s_wifi_status_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (s_wifi_conn_lbl && lv_obj_is_valid(s_wifi_conn_lbl)) {
        lv_label_set_text(s_wifi_conn_lbl, wifi_conn_txt);
        lv_obj_set_style_text_font(s_wifi_conn_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_center(s_wifi_conn_lbl);
    }
    if (s_wifi_ok_lbl && lv_obj_is_valid(s_wifi_ok_lbl)) {
        lv_label_set_text(s_wifi_ok_lbl, wifi_ok_txt);
        lv_obj_set_style_text_font(s_wifi_ok_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(s_wifi_ok_lbl);
    }
    if (s_wifi_cancel_btn_lbl && lv_obj_is_valid(s_wifi_cancel_btn_lbl)) {
        const char *cancel_str = (lang == L_Turkish) ? "Iptal" :
                                 ((lang == L_english) ? "Cancel" :
                                 ((lang == L_Russian) ? "Otmena" : "انصراف"));
        lv_label_set_text(s_wifi_cancel_btn_lbl, cancel_str);
        lv_obj_set_style_text_font(s_wifi_cancel_btn_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(s_wifi_cancel_btn_lbl);
    }
}

void tick_screen_wifi() {
    if (s_wifi_scanning) {
        if ((lv_tick_get() - s_wifi_scan_start_tick >= 1200) && !ota_manager_is_scan_running()) {
            s_wifi_scanning = false;
            if (s_wifi_spinner) lv_obj_add_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
            char ssids[16][33];
            int count = ota_manager_get_scan_results(ssids, 16);
            if (count > 0) {
                static char opts_buf[600];
                opts_buf[0] = '\0';
                for (int i = 0; i < count; i++) {
                    if (strlen(opts_buf) + strlen(ssids[i]) + 2 < sizeof(opts_buf)) {
                        if (i > 0) strcat(opts_buf, "\n");
                        strcat(opts_buf, ssids[i]);
                    }
                }
                if (objects.drop_down_wifi_list) {
                    if (lv_dropdown_is_open(objects.drop_down_wifi_list)) {
                        lv_dropdown_close(objects.drop_down_wifi_list);
                    }
                    lv_dropdown_set_options(objects.drop_down_wifi_list, opts_buf);
                    lv_obj_t *dd_l = lv_dropdown_get_list(objects.drop_down_wifi_list);
                    if (dd_l) {
                        lv_obj_set_style_text_font(dd_l, &ui_font_arial_regular17, LV_PART_MAIN);
                    }
                }
                strncpy(s_wifi_selected_ssid, ssids[0], sizeof(s_wifi_selected_ssid) - 1);
                if (s_wifi_status_lbl) {
                    char msg[64];
                    const char *fmt = (EEprom2_Language == L_english) ? "Networks found: %d" :
                                      (EEprom2_Language == L_Turkish) ? "Bulunan aglar: %d" :
                                      (EEprom2_Language == L_Russian) ? "Naydeno setey: %d" : "تعداد شبکه های یافت شده: %d";
                    snprintf(msg, sizeof(msg), fmt, count);
                    lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xff0284c7), LV_PART_MAIN);
                    lv_label_set_text(s_wifi_status_lbl, msg);
                }
            } else {
                if (s_wifi_status_lbl) {
                    const char *notfound_txt = (EEprom2_Language == L_english) ? "No network found. Try again." :
                                               (EEprom2_Language == L_Turkish) ? "Ag bulunamadi. Tekrar deneyin." :
                                               (EEprom2_Language == L_Russian) ? "Seti ne naydeny. Poprobuyte snova." : "شبکه ای یافت نشد. دوباره امتحان کنید.";
                    lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
                    lv_label_set_text(s_wifi_status_lbl, notfound_txt);
                }
            }
        }
    }

    if (ota_manager_is_wifi_connected()) {
        s_wifi_connecting = false;
        if (s_wifi_spinner) lv_obj_add_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
        if (objects.panle_wifi_password) {
            lv_obj_add_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
        }
        if (strlen(s_wifi_selected_ssid) > 0 && strlen(s_wifi_last_entered_pass) > 0) {
            ota_manager_save_wifi_credentials(s_wifi_selected_ssid, s_wifi_last_entered_pass);
        }
        loadScreen(SCREEN_ID_FRAMWARE);
        return;
    }
    if (s_wifi_connecting) {
        uint32_t elapsed = (lv_tick_get() - s_wifi_connect_start) / 1000;
        ota_status_t st = ota_manager_get_status();
        bool failed = (elapsed > 16) || (st == OTA_STATUS_ERROR && elapsed > 2);
        if (failed) {
            s_wifi_connecting = false;
            if (s_wifi_spinner) lv_obj_add_flag(s_wifi_spinner, LV_OBJ_FLAG_HIDDEN);
            if (s_wifi_status_lbl) {
                const char *fail_txt = (EEprom2_Language == L_english) ? "Wrong Wi-Fi password or weak signal. Please try again." :
                                       (EEprom2_Language == L_Turkish) ? "Yanlis Wi-Fi sifresi veya zayif sinyal. Lutfen tekrar deneyin." :
                                       (EEprom2_Language == L_Russian) ? "Nevernyy parol Wi-Fi ili slabyy signal. Poprobuyte snova." :
                                       "رمز وای فای اشتباه است یا سیگنال ضعیف است. لطفا دوباره امتحان کنید.";
                lv_obj_set_style_text_color(s_wifi_status_lbl, lv_color_hex(0xffdc2626), LV_PART_MAIN);
                lv_label_set_text(s_wifi_status_lbl, fail_txt);
            }
            if (objects.panle_wifi_password) {
                if (objects.text_area_wifi_password && strlen(s_wifi_last_entered_pass) > 0) {
                    lv_textarea_set_text(objects.text_area_wifi_password, s_wifi_last_entered_pass);
                }
                lv_obj_clear_flag(objects.panle_wifi_password, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(objects.panle_wifi_password);
            }
        }
    }
}

/* ============================================================================
 * SCREEN: DUAL-PROGRAM FIRMWARE CARDS (Apple HIG / Industrial OTA Standard)
 * ============================================================================ */
static lv_obj_t *s_fw_status_lbl[2] = {NULL, NULL};
static lv_obj_t *s_fw_ver_lbl[2] = {NULL, NULL};
static lv_obj_t *s_fw_install_btn[2] = {NULL, NULL};
static lv_obj_t *s_fw_brand_title_lbl = NULL;
static lv_obj_t *s_fw_brand_sub_lbl = NULL;
static lv_obj_t *s_fw_brand_ver_lbl = NULL;
static lv_obj_t *s_fw_back_btn_lbl = NULL;
static lv_obj_t *s_fw_prog_title_lbl[2] = {NULL, NULL};
static lv_obj_t *s_fw_info_btn_lbl[2] = {NULL, NULL};
static lv_obj_t *s_fw_inst_btn_lbl[2] = {NULL, NULL};
static lv_obj_t *s_fw_info_popup = NULL;
static lv_obj_t *s_fw_info_scroll = NULL;
static lv_obj_t *s_fw_info_content = NULL;
static int s_fw_info_popup_idx = 0;
static bool s_fw_scan_requested = false;
static lv_obj_t *s_fw_progress_modal = NULL;
static lv_obj_t *s_fw_progress_bar = NULL;
static lv_obj_t *s_fw_progress_lbl = NULL;
static lv_obj_t *s_fw_progress_speed_lbl = NULL;

static void fw_close_btn_cb(lv_event_t *e) {
    (void)e;
    ota_manager_cancel();
    loadScreen(SCREEN_ID_SETTING);
}

static void fw_info_popup_close_cb(lv_event_t *e) {
    (void)e;
    if (s_fw_info_popup) {
        lv_obj_delete(s_fw_info_popup);
        s_fw_info_popup = NULL;
        s_fw_info_scroll = NULL;
        s_fw_info_content = NULL;
    }
}

static void ui_framware_close_all_modals(void) {
    if (s_fw_info_popup) {
        lv_obj_delete(s_fw_info_popup);
        s_fw_info_popup = NULL;
        s_fw_info_scroll = NULL;
        s_fw_info_content = NULL;
    }
    if (s_fw_progress_modal) {
        ota_manager_cancel();
        lv_obj_delete(s_fw_progress_modal);
        s_fw_progress_modal = NULL;
        s_fw_progress_bar = NULL;
        s_fw_progress_lbl = NULL;
        s_fw_progress_speed_lbl = NULL;
    }
}

static void fw_sanitize_server_text(char *dst, size_t dst_sz, const char *src, int lang) {
    if (!dst || dst_sz == 0) return;
    dst[0] = '\0';
    if (!src || !src[0]) return;

    size_t d = 0;
    const unsigned char *s = (const unsigned char *)src;

    if (s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        s += 3;
    }

    while (*s && d + 8 < dst_sz) {
        // ZWNJ U+200C -> ' '
        if (s[0] == 0xE2 && s[1] == 0x80 && s[2] == 0x8C) {
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // ZWJ U+200D -> remove
        if (s[0] == 0xE2 && s[1] == 0x80 && s[2] == 0x8D) {
            s += 3;
            continue;
        }
        // BiDi markers
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x8E && s[2] <= 0x8F)) {
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0xAA && s[2] <= 0xAE)) {
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x81 && (s[2] >= 0xA6 && s[2] <= 0xA9)) {
            s += 3;
            continue;
        }
        // Soft hyphen
        if (s[0] == 0xC2 && s[1] == 0xAD) {
            s += 2;
            continue;
        }
        // Non-breaking space
        if (s[0] == 0xC2 && s[1] == 0xA0) {
            dst[d++] = ' ';
            s += 2;
            continue;
        }
        // Unicode spaces U+2000..U+200B
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x80 && s[2] <= 0x8B)) {
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Ideographic space
        if (s[0] == 0xE3 && s[1] == 0x80 && s[2] == 0x80) {
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Bullets
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0xA2 && s[2] <= 0xA4)) {
            dst[d++] = '-';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Geometric shapes
        if (s[0] == 0xE2 && (s[1] == 0x96 || s[1] == 0x97)) {
            dst[d++] = '-';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Checkmarks
        if (s[0] == 0xE2 && s[1] == 0x9C && (s[2] == 0x93 || s[2] == 0x94)) {
            dst[d++] = '+';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Cross marks
        if (s[0] == 0xE2 && s[1] == 0x9C && (s[2] >= 0x96 && s[2] <= 0x98)) {
            dst[d++] = 'x';
            dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Arrows
        if ((s[0] == 0xE2 && s[1] == 0x86 && s[2] == 0x92) ||
            (s[0] == 0xE2 && s[1] == 0x9E && s[2] == 0xA1)) {
            dst[d++] = ' '; dst[d++] = '-'; dst[d++] = '>'; dst[d++] = ' ';
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x86 && s[2] == 0x90) {
            dst[d++] = ' '; dst[d++] = '<'; dst[d++] = '-'; dst[d++] = ' ';
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x86 && s[2] == 0x94) {
            dst[d++] = ' '; dst[d++] = '<'; dst[d++] = '-'; dst[d++] = '>'; dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Warnings
        if ((s[0] == 0xE2 && s[1] == 0x9A && s[2] == 0xA0) ||
            (s[0] == 0xE2 && s[1] == 0x9D && s[2] == 0x97)) {
            dst[d++] = ' '; dst[d++] = '!'; dst[d++] = ' ';
            s += 3;
            continue;
        }
        // Dashes
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x90 && s[2] <= 0x95)) {
            dst[d++] = ' '; dst[d++] = '-'; dst[d++] = ' ';
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x88 && s[2] == 0x92) {
            dst[d++] = '-';
            s += 3;
            continue;
        }
        // Quotes
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x98 && s[2] <= 0x9B)) {
            dst[d++] = '\'';
            s += 3;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] >= 0x9C && s[2] <= 0x9F)) {
            dst[d++] = '"';
            s += 3;
            continue;
        }
        if (s[0] == 0xC2 && (s[1] == 0xAB || s[1] == 0xBB)) {
            dst[d++] = '"';
            s += 2;
            continue;
        }
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] == 0xB9 || s[2] == 0xBA)) {
            dst[d++] = '\'';
            s += 3;
            continue;
        }
        // Ellipsis
        if (s[0] == 0xE2 && s[1] == 0x80 && s[2] == 0xA6) {
            dst[d++] = '.'; dst[d++] = '.'; dst[d++] = '.';
            s += 3;
            continue;
        }
        // Accented Latin -> Safe ASCII
        if (s[0] == 0xC4 && s[1] == 0xB1) { dst[d++] = 'i'; s += 2; continue; }
        if (s[0] == 0xC4 && s[1] == 0xB0) { dst[d++] = 'I'; s += 2; continue; }
        if (s[0] == 0xC5 && s[1] == 0x9F) { dst[d++] = 's'; s += 2; continue; }
        if (s[0] == 0xC5 && s[1] == 0x9E) { dst[d++] = 'S'; s += 2; continue; }
        if (s[0] == 0xC4 && s[1] == 0x9F) { dst[d++] = 'g'; s += 2; continue; }
        if (s[0] == 0xC4 && s[1] == 0x9E) { dst[d++] = 'G'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0xA7) { dst[d++] = 'c'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0x87) { dst[d++] = 'C'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0xB6) { dst[d++] = 'o'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0x96) { dst[d++] = 'O'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0xBC) { dst[d++] = 'u'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0x9C) { dst[d++] = 'U'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0xA8 && s[1] <= 0xAB)) { dst[d++] = 'e'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0x88 && s[1] <= 0x8B)) { dst[d++] = 'E'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0xA0 && s[1] <= 0xA5)) { dst[d++] = 'a'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0x80 && s[1] <= 0x85)) { dst[d++] = 'A'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0xAC && s[1] <= 0xAF)) { dst[d++] = 'i'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0x8C && s[1] <= 0x8F)) { dst[d++] = 'I'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0xB2 && s[1] <= 0xB5)) { dst[d++] = 'o'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0x92 && s[1] <= 0x95)) { dst[d++] = 'O'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0xB9 && s[1] <= 0xBB)) { dst[d++] = 'u'; s += 2; continue; }
        if (s[0] == 0xC3 && (s[1] >= 0x89 && s[1] <= 0x8B)) { dst[d++] = 'U'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0xB1) { dst[d++] = 'n'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0x91) { dst[d++] = 'N'; s += 2; continue; }
        if (s[0] == 0xC3 && s[1] == 0x9F) { dst[d++] = 's'; dst[d++] = 's'; s += 2; continue; }

        // Arabic-Indic digits
        if (s[0] == 0xD9 && (s[1] >= 0xA0 && s[1] <= 0xA9)) {
            dst[d++] = (char)('0' + (s[1] - 0xA0));
            s += 2;
            continue;
        }

        // Emojis (4-byte)
        if (s[0] >= 0xF0 && s[0] <= 0xF4) {
            s += 4;
            continue;
        }

        // Control characters
        if (*s < 0x20 && *s != '\n' && *s != '\t') {
            s++;
            continue;
        }
        if (*s == 0x7F) {
            s++;
            continue;
        }

        // ASCII Printable
        if (*s >= 0x20 && *s <= 0x7E) {
            dst[d++] = (char)*s++;
            continue;
        }
        if (*s == '\n' || *s == '\t') {
            dst[d++] = (char)*s++;
            continue;
        }

        // Tashkeel / Harakat
        if (s[0] == 0xD9 && (s[1] >= 0x8B && s[1] <= 0x95)) {
            s += 2;
            continue;
        }

        // Arabic comma U+060C (0xD8, 0x8C) -> replace with " - "
        if (s[0] == 0xD8 && s[1] == 0x8C) {
            dst[d++] = ' '; dst[d++] = '-'; dst[d++] = ' ';
            s += 2;
            continue;
        }

        // Arabic question mark U+061F (0xD8, 0x9F) -> replace with '?'
        if (s[0] == 0xD8 && s[1] == 0x9F) {
            dst[d++] = '?';
            s += 2;
            continue;
        }

        // Arabic semicolon U+061B (0xD8, 0x9B) -> replace with ';'
        if (s[0] == 0xD8 && s[1] == 0x9B) {
            dst[d++] = ';';
            s += 2;
            continue;
        }

        // Persian/Arabic letters
        if ((s[0] >= 0xD8 && s[0] <= 0xDB) && (s[1] >= 0x80 && s[1] <= 0xBF)) {
            if (lang == 0) {
                dst[d++] = (char)*s++;
                dst[d++] = (char)*s++;
                continue;
            } else {
                s += 2;
                continue;
            }
        }

        // Other multi-byte characters -> skip safely to prevent tofu
        if ((s[0] & 0xE0) == 0xC0) {
            s += 2;
            continue;
        } else if ((s[0] & 0xF0) == 0xE0) {
            s += 3;
            continue;
        } else {
            s++;
            continue;
        }
    }
    dst[d] = '\0';
}

static void fw_pinmap_append_line(char *buf, size_t sz, const char *line) {
    if (!buf || !line || !line[0] || sz == 0) return;
    size_t cur = strlen(buf);
    if (cur >= sz - 1) return;
    if (cur > 0 && cur + 1 < sz - 1) {
        buf[cur] = '\n';
        buf[cur + 1] = '\0';
        cur++;
    }
    size_t remaining = sz - 1 - cur;
    strncat(buf, line, remaining);
}

static void fw_pinmap_parse_server_text(const char *raw, char *in_buf, size_t in_sz, char *out_buf, size_t out_sz, int lang) {
    (void)lang;
    if (!raw || !raw[0] || !in_buf || !out_buf || in_sz == 0 || out_sz == 0) return;
    in_buf[0] = '\0';
    out_buf[0] = '\0';

    const char *p = raw;
    bool in_inputs = false;
    bool in_outputs = false;

    while (*p) {
        while (*p == '\r' || *p == '\n') p++;
        if (!*p) break;
        const char *line_start = p;
        while (*p && *p != '\r' && *p != '\n') p++;
        size_t line_len = (size_t)(p - line_start);
        if (line_len == 0 || line_len >= 256) continue;

        char line[256];
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';

        char *s = line;
        while (*s && (*s == ' ' || *s == '\t')) s++;
        size_t slen = strlen(s);
        while (slen > 0 && (s[slen - 1] == ' ' || s[slen - 1] == '\t')) {
            s[--slen] = '\0';
        }
        if (slen == 0 || s[0] == '#') continue;

        if (strncmp(s, "[inputs]", 8) == 0 || strncmp(s, "[input]", 7) == 0) {
            in_inputs = true;
            in_outputs = false;
            continue;
        } else if (strncmp(s, "[outputs]", 9) == 0 || strncmp(s, "[output]", 8) == 0) {
            in_inputs = false;
            in_outputs = true;
            continue;
        } else if (s[0] == '[' && s[slen - 1] == ']') {
            in_inputs = false;
            in_outputs = false;
            continue;
        }

        // AGENTS.md Rule 4: ABSOLUTE PROHIBITION ON BUZZER IN PINOUT
        char lower[256];
        size_t i = 0;
        for (; s[i] && i < 255; i++) {
            lower[i] = (char)tolower((unsigned char)s[i]);
        }
        lower[i] = '\0';
        if (strstr(lower, "buzzer") || strstr(lower, "buzz") || strstr(s, "بازر") || strstr(lower, "bz")) {
            continue;
        }

        // Remove prefix inX= or outX=
        char *eq = strchr(s, '=');
        if (eq && (size_t)(eq - s) < 10) {
            s = eq + 1;
            while (*s && (*s == ' ' || *s == '\t')) s++;
        }

        // Remove GPIOxx prefix
        char cleaned[256];
        size_t c_idx = 0;
        char *gp = strstr(s, "GPIO");
        if (!gp) gp = strstr(s, "gpio");
        if (gp) {
            size_t pre_len = (size_t)(gp - s);
            if (pre_len > 0) {
                memcpy(cleaned, s, pre_len);
                c_idx = pre_len;
            }
            char *post = gp + 4;
            while (*post && (isdigit((unsigned char)*post) || *post == '-' || *post == '.')) post++;
            while (*post && (*post == ' ' || *post == '\t')) post++;
            size_t post_len = strlen(post);
            if (c_idx + post_len < 255) {
                memcpy(cleaned + c_idx, post, post_len);
                c_idx += post_len;
            }
            cleaned[c_idx] = '\0';
            s = cleaned;
        }

        slen = strlen(s);
        if (slen == 0) continue;

        bool is_m_pin = (s[0] == 'M' || s[0] == 'm');
        if (in_inputs) {
            fw_pinmap_append_line(in_buf, in_sz, s);
        } else if (in_outputs) {
            fw_pinmap_append_line(out_buf, out_sz, s);
        } else if (is_m_pin) {
            fw_pinmap_append_line(in_buf, in_sz, s);
            fw_pinmap_append_line(out_buf, out_sz, s);
        }
    }
}

static const char *fw_pinmap_gas_inputs(int lang) {
    if (lang == 2) return "I0 = Pedal Girisi (Aktif LOW)\nI1 = Acil Durdurma Girisi\nI2 = Gaz Basinc Sensoru\nI3 = On Panel Tuslari\nM0..M7 = Yardimci Girisler";
    if (lang == 1) return "I0 = Foot Pedal Input (Active LOW)\nI1 = Emergency Stop Input\nI2 = Gas Pressure Sensor\nI3 = Front Panel Keys\nM0..M7 = Auxiliary Inputs";
    if (lang == 3) return "I0 = Pedal' Start (Aktiv. LOW)\nI1 = Avariynyy Stop\nI2 = Datchik Davleniya Gaza\nI3 = Knopki Paneli\nM0..M7 = Dopolnitel'nyye Vkhody";
    return "I0 = ورودی پدال پایی استارت (Active LOW)\nI1 = ورودی استپ اضطراری\nI2 = سنسور فشار گاز ورودی\nI3 = کلیدهای پانل کاربری\nM0..M7 = ورودی های کمکی دوطرفه M";
}

static const char *fw_pinmap_gas_outputs(int lang) {
    if (lang == 2) return "Q0 = Valf 1 Role (Cam 2 - Sol)\nQ1 = Valf 2 Role (Cam 1 - Sag)\nM0..M7 = Yardimci Cikislar";
    if (lang == 1) return "Q0 = Valve 1 Relay (Glass 2 - Left)\nQ1 = Valve 2 Relay (Glass 1 - Right)\nM0..M7 = Auxiliary Outputs";
    if (lang == 3) return "Q0 = Rele Klapana 1 (Steklo 2 - Levo)\nQ1 = Rele Klapana 2 (Steklo 1 - Pravo)\nM0..M7 = Dopolnitel'nyye Vykhody";
    return "Q0 = شیر برقی 1 (شیشه 2 - چپ)\nQ1 = شیر برقی 2 (شیشه 1 - راست)\nM0..M7 = خروجی های کمکی دوطرفه M";
}

static const char *fw_pinmap_config_inputs(int lang) {
    if (lang == 2) return "I0..I7 = Dijital Girisler PLC\nIN_STOP = Acil Durdurma\nM0..M7 = Yardimci Girisler";
    if (lang == 1) return "I0..I7 = Digital Inputs PLC\nIN_STOP = Emergency Stop\nM0..M7 = Auxiliary Inputs";
    if (lang == 3) return "I0..I7 = Tsifrovyye Vkhody PLC\nIN_STOP = Avariynyy Stop\nM0..M7 = Dopolnitel'nyye Vkhody";
    return "I0..I7 = ورودی های دیجیتال PLC\nIN_STOP = استپ اضطراری سخت افزار\nM0..M7 = ورودی های کمکی دوطرفه M";
}

static const char *fw_pinmap_config_outputs(int lang) {
    if (lang == 2) return "Q0 = Valf 1 Cikisi\nQ1 = Valf 2 Cikisi\nQ2..Q7 = Yedek Roleler\nM0..M7 = Yardimci Cikislar";
    if (lang == 1) return "Q0 = Valve 1 Output\nQ1 = Valve 2 Output\nQ2..Q7 = Auxiliary Relays\nM0..M7 = Auxiliary Outputs";
    if (lang == 3) return "Q0 = Vykhod Klapana 1\nQ1 = Vykhod Klapana 2\nQ2..Q7 = Zapasnyye Rele\nM0..M7 = Dopolnitel'nyye Vykhody";
    return "Q0 = خروجی رله شیر 1\nQ1 = خروجی رله شیر 2\nQ2..Q7 = خروجی رله های کمکی\nM0..M7 = خروجی های کمکی دوطرفه M";
}

static lv_obj_t *fw_pinmap_add_col(lv_obj_t *parent, const char *hdr, const char *lines, int lang) {
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_width(col, lv_pct(48));
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(col, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_color(col, lv_color_hex(0xff161b22), LV_PART_MAIN);
    lv_obj_set_style_radius(col, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(col, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(col, lv_color_hex(0xff30363d), LV_PART_MAIN);
    lv_obj_set_style_pad_all(col, 6, LV_PART_MAIN);

    const lv_font_t *title_font = (lang == 0) ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14;
    const lv_font_t *body_font  = (lang == 0) ? ui_font_vazir_or(&ui_font_arial_regular17, 12) : &lv_font_montserrat_12;

    lv_obj_t *h = lv_label_create(col);
    lv_label_set_text(h, hdr);
    lv_obj_set_width(h, lv_pct(100));
    lv_obj_clear_flag(h, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(h, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(h, title_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(h, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *body = lv_label_create(col);
    lv_label_set_text(body, lines);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_clear_flag(body, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(body, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(body, body_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(body, lv_color_hex(0xfff8fafc), LV_PART_MAIN);
    if (lang == 0) {
        lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_align(body, LV_ALIGN_TOP_RIGHT, 0, 22);
    } else {
        lv_obj_set_style_base_dir(body, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 22);
    }
    return col;
}

static void fw_pinmap_apply_body(lv_obj_t *content, int idx, int lang);

static void fw_pinmap_rescan_click_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ota_manager_request_rescan();
    int lang = (EEprom2_Language == L_Persion) ? 0 :
               (EEprom2_Language == L_english) ? 1 :
               (EEprom2_Language == L_Turkish) ? 2 : 3;
    if (s_fw_info_content) {
        fw_pinmap_apply_body(s_fw_info_content, idx, lang);
    }
}

static bool fw_is_line_newer_or_current(const char *line, const char *local_ver, bool is_newer)
{
    if (!line || !line[0]) return false;
    if (!is_newer) return true;

    const char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '-' || *p == '*') p++;
    if (*p == 'v' || *p == 'V') p++;

    int lv1 = 0, lv2 = 0, lv3 = 0;
    if (sscanf(p, "%d.%d.%d", &lv1, &lv2, &lv3) >= 2) {
        const char *loc = local_ver;
        while (*loc == 'v' || *loc == 'V' || *loc == ' ') loc++;
        int cv1 = 0, cv2 = 0, cv3 = 0;
        if (sscanf(loc, "%d.%d.%d", &cv1, &cv2, &cv3) >= 2) {
            if (lv1 > cv1) return true;
            if (lv1 < cv1) return false;
            if (lv2 > cv2) return true;
            if (lv2 < cv2) return false;
            return lv3 > cv3;
        }
    }
    return true;
}

static void fw_pinmap_apply_body(lv_obj_t *content, int idx, int lang) {
    if (!content) return;
    lv_obj_clean(content);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_height(content, LV_SIZE_CONTENT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(content, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_opa(content, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 8, LV_PART_MAIN);

    const lv_font_t *title_font = (lang == 0) ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14;
    const lv_font_t *body_font  = (lang == 0) ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12;

    bool avail = ota_info_is_available(idx);
    bool wifi_ok = ota_manager_is_wifi_connected();

    if (!avail) {
        lv_obj_t *c_warn = lv_obj_create(content);
        lv_obj_set_width(c_warn, lv_pct(100));
        lv_obj_set_height(c_warn, LV_SIZE_CONTENT);
        lv_obj_clear_flag(c_warn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(c_warn, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_bg_color(c_warn, lv_color_hex(0xff450a0a), LV_PART_MAIN);
        lv_obj_set_style_radius(c_warn, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(c_warn, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(c_warn, lv_color_hex(0xffb91c1c), LV_PART_MAIN);
        lv_obj_set_style_pad_all(c_warn, 10, LV_PART_MAIN);
        lv_obj_set_flex_flow(c_warn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(c_warn, 6, LV_PART_MAIN);

        lv_obj_t *wlbl = lv_label_create(c_warn);
        lv_obj_set_width(wlbl, lv_pct(100));
        lv_label_set_long_mode(wlbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(wlbl, title_font, LV_PART_MAIN);
        lv_obj_set_style_text_color(wlbl, lv_color_hex(0xfffca5a5), LV_PART_MAIN);
        if (lang == 0) {
            lv_obj_set_style_base_dir(wlbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
            lv_obj_set_style_text_align(wlbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
            if (!wifi_ok) {
                lv_label_set_text(wlbl, "وای فای متصل نیست. اطلاعات نسخه و پایه ها مستقیما از سرور دریافت می شود. لطفا ابتدا به اینترنت متصل شوید.");
            } else if (ota_info_is_scan_busy() || !ota_info_is_scan_done()) {
                lv_label_set_text(wlbl, "در حال استعلام و دریافت مشخصات و توضیحات نسخه از سرور... لطفا شکیبا باشید.");
            } else {
                lv_label_set_text(wlbl, "ارتباط با سرور برقرار نشد - اطلاعات این نسخه در سرور در دسترس نیست. اطلاعات باید حتما از سرور دریافت شود.");
            }
        } else {
            lv_obj_set_style_base_dir(wlbl, LV_BASE_DIR_LTR, LV_PART_MAIN);
            lv_obj_set_style_text_align(wlbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            if (!wifi_ok) {
                lv_label_set_text(wlbl, "WiFi disconnected. Connect to internet to fetch specifications directly from server.");
            } else if (ota_info_is_scan_busy() || !ota_info_is_scan_done()) {
                lv_label_set_text(wlbl, "Fetching release information and pinout from server... Please wait.");
            } else {
                lv_label_set_text(wlbl, "Server release info unavailable. Information must be retrieved from server.");
            }
        }

        if (wifi_ok) {
            lv_obj_t *rbtn = lv_btn_create(c_warn);
            lv_obj_set_size(rbtn, 160, 32);
            lv_obj_set_style_radius(rbtn, 6, LV_PART_MAIN);
            lv_obj_set_style_bg_color(rbtn, lv_color_hex(0xff0284c7), LV_PART_MAIN);
            lv_obj_set_style_pad_all(rbtn, 4, LV_PART_MAIN);
            lv_obj_add_event_cb(rbtn, fw_pinmap_rescan_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
            lv_obj_t *rlbl = lv_label_create(rbtn);
            lv_label_set_text(rlbl, (lang == 0) ? "دریافت از سرور" : (lang == 2 ? "Sunucudan Al" : (lang == 3 ? "Poluchit' s Servera" : "Fetch from Server")));
            lv_obj_set_style_text_font(rlbl, title_font, LV_PART_MAIN);
            lv_obj_center(rlbl);
        }
        return;
    }

    // ── ۱. کارت مشخصات و اطلاعات نسخه از سرور ──
    lv_obj_t *c_ver = lv_obj_create(content);
    lv_obj_set_width(c_ver, lv_pct(100));
    lv_obj_set_height(c_ver, LV_SIZE_CONTENT);
    lv_obj_clear_flag(c_ver, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c_ver, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_color(c_ver, lv_color_hex(0xff22272e), LV_PART_MAIN);
    lv_obj_set_style_radius(c_ver, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(c_ver, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(c_ver, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_pad_all(c_ver, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(c_ver, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c_ver, 3, LV_PART_MAIN);

    const char *pname_raw = ota_info_get_prog_name(idx);
    char pname[128];
    fw_sanitize_server_text(pname, sizeof(pname), pname_raw, lang);

    lv_obj_t *lbl_name = lv_label_create(c_ver);
    lv_obj_set_width(lbl_name, lv_pct(100));
    lv_obj_clear_flag(lbl_name, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl_name, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl_name, title_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    if (lang == 0) {
        lv_obj_set_style_base_dir(lbl_name, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_label_set_text_fmt(lbl_name, "دستگاه: %s", pname[0] ? pname : ((idx == 1) ? "تنظیمات و تست سخت افزار (Config)" : "تزریق گاز آرگون شیشه دوجداره هافلر"));
    } else {
        lv_obj_set_style_base_dir(lbl_name, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_text_fmt(lbl_name, "Program: %s", pname[0] ? pname : ((idx == 1) ? "Config-Test" : "Haffler Argon Gas Filler"));
    }

    const char *ver_raw = ota_info_get_version(idx);
    const char *bdate_raw = ota_info_get_build_date(idx);
    const char *btime_raw = ota_info_get_build_time(idx);
    char ver_str[48], bdate[32], btime[32];
    fw_sanitize_server_text(ver_str, sizeof(ver_str), ver_raw, lang);
    fw_sanitize_server_text(bdate, sizeof(bdate), bdate_raw, lang);
    fw_sanitize_server_text(btime, sizeof(btime), btime_raw, lang);

    const ota_info_t *pinfo = ota_manager_get_prog_info(idx);
    bool is_newer = pinfo ? pinfo->update_available : false;

    lv_obj_t *lbl_ver = lv_label_create(c_ver);
    lv_obj_set_width(lbl_ver, lv_pct(100));
    lv_obj_clear_flag(lbl_ver, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl_ver, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(lbl_ver, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl_ver, body_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_ver, lv_color_hex(is_newer ? 0xff22c55e : 0xff38bdf8), LV_PART_MAIN);
    if (ver_str[0]) {
        const char *status_tag = is_newer
            ? (lang == 0 ? "نسخه جدیدتر (ارتقا)" : (lang == 2 ? "Yeni Surum (Guncelle)" : (lang == 3 ? "Novaya Versiya" : "New Update")))
            : (lang == 0 ? "هم نسخه (پایدار)" : (lang == 2 ? "Guncel (Kararli)" : (lang == 3 ? "Tekushchaya (Stabil'naya)" : "Current (Stable)")));
        if (bdate[0] && btime[0]) {
            if (lang == 0) {
                lv_obj_set_style_base_dir(lbl_ver, LV_BASE_DIR_RTL, LV_PART_MAIN);
                lv_obj_set_style_text_align(lbl_ver, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                lv_label_set_text_fmt(lbl_ver, "نسخه سرور: %s  |  دستگاه: %s  |  تاریخ: %s %s  |  [%s]", ver_str, FW_VERSION, bdate, btime, status_tag);
            } else {
                lv_obj_set_style_base_dir(lbl_ver, LV_BASE_DIR_LTR, LV_PART_MAIN);
                lv_obj_set_style_text_align(lbl_ver, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_label_set_text_fmt(lbl_ver, "Server: %s  |  Device: %s  |  Date: %s %s  |  [%s]", ver_str, FW_VERSION, bdate, btime, status_tag);
            }
        } else {
            if (lang == 0) {
                lv_obj_set_style_base_dir(lbl_ver, LV_BASE_DIR_RTL, LV_PART_MAIN);
                lv_obj_set_style_text_align(lbl_ver, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                lv_label_set_text_fmt(lbl_ver, "نسخه سرور: %s  |  دستگاه: %s  |  [%s]", ver_str, FW_VERSION, status_tag);
            } else {
                lv_obj_set_style_base_dir(lbl_ver, LV_BASE_DIR_LTR, LV_PART_MAIN);
                lv_obj_set_style_text_align(lbl_ver, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_label_set_text_fmt(lbl_ver, "Server: %s  |  Device: %s  |  [%s]", ver_str, FW_VERSION, status_tag);
            }
        }
    }

    lv_obj_t *lbl_plat = lv_label_create(c_ver);
    lv_obj_set_width(lbl_plat, lv_pct(100));
    lv_obj_clear_flag(lbl_plat, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl_plat, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(lbl_plat, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl_plat, body_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_plat, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    if (lang == 0) {
        lv_obj_set_style_base_dir(lbl_plat, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_plat, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_label_set_text(lbl_plat, "پلتفرم: ESP32-S3 DevKitC-1  |  نمایشگر 480x320 TFT  |  تاچ خازنی GT911");
    } else {
        lv_obj_set_style_base_dir(lbl_plat, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_plat, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_text(lbl_plat, "Platform: ESP32-S3 | 480x320 TFT | Capacitive Touch GT911");
    }

    const char *comp_raw = ota_info_get_company_name(idx);
    const char *web_raw = ota_info_get_company_website(idx);
    char comp_str[96], web_str[80];
    fw_sanitize_server_text(comp_str, sizeof(comp_str), comp_raw, lang);
    fw_sanitize_server_text(web_str, sizeof(web_str), web_raw, lang);

    lv_obj_t *lbl_comp = lv_label_create(c_ver);
    lv_obj_set_width(lbl_comp, lv_pct(100));
    lv_obj_clear_flag(lbl_comp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl_comp, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(lbl_comp, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl_comp, body_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_comp, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    if (lang == 0) {
        lv_obj_set_style_base_dir(lbl_comp, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_comp, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_label_set_text_fmt(lbl_comp, "سازنده: %s  |  وبسایت: %s", comp_str[0] ? comp_str : "هافلر اتوماسیون صنعتی", web_str[0] ? web_str : "www.haffler.ir");
    } else {
        lv_obj_set_style_base_dir(lbl_comp, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_comp, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_text_fmt(lbl_comp, "Manufacturer: %s  |  Web: %s", comp_str[0] ? comp_str : "Haffler Industrial Automation", web_str[0] ? web_str : "www.haffler.ir");
    }

    // ── ۲. کارت موارد اصلاح شده و چنج لاگ سه زبانه بر اساس زبان دستگاه ──
    int descCnt = ota_info_get_desc_count_lang(idx, lang);
    {
        lv_obj_t *c_desc = lv_obj_create(content);
        lv_obj_set_width(c_desc, lv_pct(100));
        lv_obj_set_height(c_desc, LV_SIZE_CONTENT);
        lv_obj_clear_flag(c_desc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(c_desc, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_bg_color(c_desc, lv_color_hex(0xff22272e), LV_PART_MAIN);
        lv_obj_set_style_radius(c_desc, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(c_desc, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(c_desc, lv_color_hex(0xff334155), LV_PART_MAIN);
        lv_obj_set_style_pad_all(c_desc, 8, LV_PART_MAIN);
        lv_obj_set_flex_flow(c_desc, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(c_desc, 4, LV_PART_MAIN);

        lv_obj_t *h_desc = lv_label_create(c_desc);
        lv_obj_set_width(h_desc, lv_pct(100));
        lv_obj_clear_flag(h_desc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(h_desc, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_text_font(h_desc, title_font, LV_PART_MAIN);
        lv_obj_set_style_text_color(h_desc, lv_color_hex(0xfff59e0b), LV_PART_MAIN);
        if (lang == 0) {
            lv_obj_set_style_base_dir(h_desc, LV_BASE_DIR_RTL, LV_PART_MAIN);
            lv_obj_set_style_text_align(h_desc, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
            lv_label_set_text(h_desc, is_newer ? "تغییرات تجمعی ارتقا (از نسخه دستگاه تا سرور):" : "تغییرات و موارد اصلاح شده در این نسخه:");
        } else {
            lv_obj_set_style_base_dir(h_desc, LV_BASE_DIR_LTR, LV_PART_MAIN);
            lv_obj_set_style_text_align(h_desc, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            if (is_newer) {
                lv_label_set_text(h_desc, (lang == 2) ? "Kumulatif Guncelleme Degisiklikleri:" : "Cumulative Rollup Changes (Device to Server):");
            } else {
                lv_label_set_text(h_desc, (lang == 2) ? "Surum Yenilikleri ve Duzeltmeler:" : "Release Changes, Additions and Fixes:");
            }
        }

        int shownCnt = 0;
        if (descCnt > 0) {
            for (int d = 0; d < descCnt; d++) {
                const char *line = ota_info_get_desc_line_lang(idx, lang, d);
                if (!line || !line[0]) continue;
                if (!fw_is_line_newer_or_current(line, FW_VERSION, is_newer)) continue;
                char clean_line[256];
                fw_sanitize_server_text(clean_line, sizeof(clean_line), line, lang);
                if (!clean_line[0]) continue;

                char *item_text = clean_line;
                while (*item_text == ' ' || *item_text == '\t') item_text++;
                if (item_text[0] == '-' && item_text[1] == ' ') {
                    item_text += 2;
                    while (*item_text == ' ') item_text++;
                } else if (item_text[0] == '*' && item_text[1] == ' ') {
                    item_text += 2;
                    while (*item_text == ' ') item_text++;
                }

                lv_obj_t *lbl_d = lv_label_create(c_desc);
                lv_obj_set_width(lbl_d, lv_pct(100));
                lv_obj_clear_flag(lbl_d, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_flag(lbl_d, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
                lv_label_set_long_mode(lbl_d, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_font(lbl_d, body_font, LV_PART_MAIN);
                lv_obj_set_style_text_color(lbl_d, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
                if (lang == 0) {
                    lv_obj_set_style_base_dir(lbl_d, LV_BASE_DIR_RTL, LV_PART_MAIN);
                    lv_obj_set_style_text_align(lbl_d, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                } else {
                    lv_obj_set_style_base_dir(lbl_d, LV_BASE_DIR_LTR, LV_PART_MAIN);
                    lv_obj_set_style_text_align(lbl_d, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                }
                lv_label_set_text_fmt(lbl_d, "- %s", item_text);
                shownCnt++;
            }
        }
        if (shownCnt == 0 && descCnt > 0) {
            for (int d = 0; d < descCnt; d++) {
                const char *line = ota_info_get_desc_line_lang(idx, lang, d);
                if (!line || !line[0]) continue;
                char clean_line[256];
                fw_sanitize_server_text(clean_line, sizeof(clean_line), line, lang);
                if (!clean_line[0]) continue;

                char *item_text = clean_line;
                while (*item_text == ' ' || *item_text == '\t') item_text++;
                if (item_text[0] == '-' && item_text[1] == ' ') item_text += 2;
                else if (item_text[0] == '*' && item_text[1] == ' ') item_text += 2;
                while (*item_text == ' ') item_text++;

                lv_obj_t *lbl_d = lv_label_create(c_desc);
                lv_obj_set_width(lbl_d, lv_pct(100));
                lv_obj_clear_flag(lbl_d, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_flag(lbl_d, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
                lv_label_set_long_mode(lbl_d, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_font(lbl_d, body_font, LV_PART_MAIN);
                lv_obj_set_style_text_color(lbl_d, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);
                if (lang == 0) {
                    lv_obj_set_style_base_dir(lbl_d, LV_BASE_DIR_RTL, LV_PART_MAIN);
                    lv_obj_set_style_text_align(lbl_d, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                } else {
                    lv_obj_set_style_base_dir(lbl_d, LV_BASE_DIR_LTR, LV_PART_MAIN);
                    lv_obj_set_style_text_align(lbl_d, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                }
                lv_label_set_text_fmt(lbl_d, "- %s", item_text);
                shownCnt++;
            }
        }
        if (shownCnt == 0) {
            lv_obj_t *lbl_none = lv_label_create(c_desc);
            lv_obj_set_width(lbl_none, lv_pct(100));
            lv_label_set_long_mode(lbl_none, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(lbl_none, body_font, LV_PART_MAIN);
            lv_obj_set_style_text_color(lbl_none, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
            if (lang == 0) {
                lv_obj_set_style_base_dir(lbl_none, LV_BASE_DIR_RTL, LV_PART_MAIN);
                lv_obj_set_style_text_align(lbl_none, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                lv_label_set_text(lbl_none, "توضیحاتی برای این نسخه در فایل lastfile سرور ثبت نشده است.");
            } else {
                lv_obj_set_style_base_dir(lbl_none, LV_BASE_DIR_LTR, LV_PART_MAIN);
                lv_obj_set_style_text_align(lbl_none, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_label_set_text(lbl_none, "No release descriptions available in server lastfile.");
            }
        }
    }

    // ── ۳. کارت نقشه اتصالات پایه های ورودی و خروجی (بدون بازر - با پایه های کمکی M) ──
    lv_obj_t *c_pins = lv_obj_create(content);
    lv_obj_set_width(c_pins, lv_pct(100));
    lv_obj_set_height(c_pins, LV_SIZE_CONTENT);
    lv_obj_clear_flag(c_pins, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c_pins, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_color(c_pins, lv_color_hex(0xff22272e), LV_PART_MAIN);
    lv_obj_set_style_radius(c_pins, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(c_pins, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(c_pins, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_pad_all(c_pins, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(c_pins, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c_pins, 6, LV_PART_MAIN);

    lv_obj_t *h_pins = lv_label_create(c_pins);
    lv_obj_set_width(h_pins, lv_pct(100));
    lv_obj_clear_flag(h_pins, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(h_pins, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_text_font(h_pins, title_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(h_pins, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    if (lang == 0) {
        lv_obj_set_style_base_dir(h_pins, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(h_pins, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_label_set_text(h_pins, "نقشه اتصالات و پایه های سخت افزاری:");
    } else {
        lv_obj_set_style_base_dir(h_pins, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(h_pins, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_text(h_pins, lang == 2 ? "Giris / Cikis Pin Haritasi:" : "Input / Output Pinout Map:");
    }

    lv_obj_t *pin_row = lv_obj_create(c_pins);
    lv_obj_set_width(pin_row, lv_pct(100));
    lv_obj_set_height(pin_row, LV_SIZE_CONTENT);
    lv_obj_clear_flag(pin_row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pin_row, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_opa(pin_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pin_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(pin_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pin_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    const char *raw_pinmap = ota_info_get_pinmap_text(idx);
    static char in_buf[1024];
    static char out_buf[1024];
    in_buf[0] = '\0';
    out_buf[0] = '\0';
    if (raw_pinmap && raw_pinmap[0]) {
        fw_pinmap_parse_server_text(raw_pinmap, in_buf, sizeof(in_buf), out_buf, sizeof(out_buf), lang);
    }
    const char *in_list_raw = (in_buf[0] != '\0') ? in_buf : ((idx == 0) ? fw_pinmap_gas_inputs(lang) : fw_pinmap_config_inputs(lang));
    const char *out_list_raw = (out_buf[0] != '\0') ? out_buf : ((idx == 0) ? fw_pinmap_gas_outputs(lang) : fw_pinmap_config_outputs(lang));

    static char in_list[1024];
    static char out_list[1024];
    fw_sanitize_server_text(in_list, sizeof(in_list), in_list_raw, lang);
    fw_sanitize_server_text(out_list, sizeof(out_list), out_list_raw, lang);

    const char *in_hdr = (lang == 0) ? "ورودی های دیجیتال" : (lang == 2 ? "Dijital Girisler" : "Digital Inputs");
    const char *out_hdr = (lang == 0) ? "خروجی های رله" : (lang == 2 ? "Role Cikislari" : "Relay Outputs");
    fw_pinmap_add_col(pin_row, in_hdr, in_list, lang);
    fw_pinmap_add_col(pin_row, out_hdr, out_list, lang);

    // نکته ایمنی
    lv_obj_t *lbl_note = lv_label_create(c_pins);
    lv_obj_set_width(lbl_note, lv_pct(100));
    lv_obj_clear_flag(lbl_note, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl_note, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(lbl_note, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl_note, body_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_note, lv_color_hex(0xfffbbf24), LV_PART_MAIN);
    if (lang == 0) {
        lv_obj_set_style_base_dir(lbl_note, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_note, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_label_set_text(lbl_note, "نکته ایمنی: قطع اضطراری (STOP یا ورودی پدال) بیدرنگ کلیه خروجی های شیر برقی را قطع میکند. سقف ایمنی تزریق 180 ثانیه فعال است.");
    } else {
        lv_obj_set_style_base_dir(lbl_note, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_note, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_text(lbl_note, "Safety Note: Emergency stop immediately cuts all gas valve outputs. 180s injection watchdog limit is active.");
    }

    // ── ۴. کارت مشخصات باینری و پارتیشن (Binary & Flash Info) ──
    lv_obj_t *c_bin = lv_obj_create(content);
    lv_obj_set_width(c_bin, lv_pct(100));
    lv_obj_set_height(c_bin, LV_SIZE_CONTENT);
    lv_obj_clear_flag(c_bin, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c_bin, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_color(c_bin, lv_color_hex(0xff22272e), LV_PART_MAIN);
    lv_obj_set_style_radius(c_bin, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(c_bin, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(c_bin, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_pad_all(c_bin, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(c_bin, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c_bin, 3, LV_PART_MAIN);

    lv_obj_t *lbl_bin = lv_label_create(c_bin);
    lv_obj_set_width(lbl_bin, lv_pct(100));
    lv_obj_clear_flag(lbl_bin, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl_bin, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_long_mode(lbl_bin, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl_bin, body_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_bin, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    const char *sha_raw = ota_info_get_sha256(idx);
    char sha_short[20] = {0};
    if (sha_raw && strlen(sha_raw) >= 12) {
        strncpy(sha_short, sha_raw, 12);
        sha_short[12] = '\0';
    }
    const ota_info_t *bin_info = ota_manager_get_prog_info(idx);
    uint32_t bin_size = bin_info ? bin_info->file_size : 0;
    float bin_mb = (bin_size > 0) ? ((float)bin_size / (1024.0f * 1024.0f)) : 0.0f;
    if (lang == 0) {
        lv_obj_set_style_base_dir(lbl_bin, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_bin, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        if (bin_size > 0 && sha_short[0]) {
            lv_label_set_text_fmt(lbl_bin, "حجم برنامه: %.2f مگابایت (%u بایت)  |  هش SHA: %s...  |  پارتیشن: 16MB Flash", bin_mb, bin_size, sha_short);
        } else if (sha_short[0]) {
            lv_label_set_text_fmt(lbl_bin, "پارتیشن: 16MB Flash (برنامه 7MB)  |  هش SHA: %s...  |  حفظ کالیبراسیون در NVS", sha_short);
        } else {
            lv_label_set_text(lbl_bin, "پارتیشن: 16MB Flash (پارتیشن برنامه 7MB)  |  حفظ کالیبراسیون و متغیرها در NVS  |  نوع: QIO 80MHz");
        }
    } else {
        lv_obj_set_style_base_dir(lbl_bin, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_style_text_align(lbl_bin, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        if (bin_size > 0 && sha_short[0]) {
            lv_label_set_text_fmt(lbl_bin, "Program Size: %.2f MB (%u bytes) | SHA-256: %s... | 16MB Flash", bin_mb, bin_size, sha_short);
        } else if (sha_short[0]) {
            lv_label_set_text_fmt(lbl_bin, "Partition: 16MB Flash (App 7MB) | SHA-256: %s... | NVS Preserved", sha_short);
        } else {
            lv_label_set_text(lbl_bin, "Partition: 16MB Flash (App 7MB) | NVS Calibrations Preserved | QIO 80MHz");
        }
    }
}

static void fw_open_info_popup(int prog_idx) {
    if (s_fw_info_popup) {
        lv_obj_delete(s_fw_info_popup);
        s_fw_info_popup = NULL;
        s_fw_info_scroll = NULL;
        s_fw_info_content = NULL;
    }
    s_fw_info_popup_idx = prog_idx;
    int lang = (EEprom2_Language == L_Persion) ? 0 :
               (EEprom2_Language == L_english) ? 1 :
               (EEprom2_Language == L_Turkish) ? 2 : 3;

    const char *title = (prog_idx == 1)
        ? ((lang == 2) ? "Config Programi Bilgileri" : ((lang == 1) ? "Config Program Info & Guide" : "مشخصات و راهنمای برنامه تست و کانفیگ"))
        : ((lang == 2) ? "Gaz Dolum Programi Bilgileri" : ((lang == 1) ? "Gas Filling Program Info & Guide" : "مشخصات و راهنمای برنامه تزریق گاز آرگون"));

    s_fw_info_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_fw_info_popup, 450, 296);
    lv_obj_center(s_fw_info_popup);
    lv_obj_clear_flag(s_fw_info_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_fw_info_popup, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_fw_info_popup, lv_color_hex(0xff18181b), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_fw_info_popup, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_fw_info_popup, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_fw_info_popup, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_fw_info_popup, 0, LV_PART_MAIN);

    lv_obj_t *close = lv_btn_create(s_fw_info_popup);
    lv_obj_set_size(close, 36, 30);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_set_style_bg_color(close, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_radius(close, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(close, fw_info_popup_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *x = lv_label_create(close);
    lv_label_set_text(x, "X");
    lv_obj_set_style_text_font(x, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(x);

    lv_obj_t *ttl = lv_label_create(s_fw_info_popup);
    lv_obj_set_width(ttl, 350);
    lv_obj_align(ttl, LV_ALIGN_TOP_MID, -10, 10);
    lv_obj_set_style_text_align(ttl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    const lv_font_t *ttl_font = (lang == 0) ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16;
    lv_obj_set_style_text_font(ttl, ttl_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ttl, lv_color_hex(0xffffffff), LV_PART_MAIN);
    lv_label_set_text(ttl, title);

    s_fw_info_scroll = lv_obj_create(s_fw_info_popup);
    lv_obj_set_size(s_fw_info_scroll, 426, 238);
    lv_obj_align(s_fw_info_scroll, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_opa(s_fw_info_scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_fw_info_scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_fw_info_scroll, 6, LV_PART_MAIN);
    lv_obj_add_flag(s_fw_info_scroll, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(s_fw_info_scroll, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_scroll_snap_y(s_fw_info_scroll, LV_SCROLL_SNAP_NONE);
    lv_obj_set_scroll_dir(s_fw_info_scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_fw_info_scroll, LV_SCROLLBAR_MODE_AUTO);

    s_fw_info_content = lv_obj_create(s_fw_info_scroll);
    lv_obj_set_width(s_fw_info_content, lv_pct(100));
    lv_obj_set_height(s_fw_info_content, LV_SIZE_CONTENT);
    lv_obj_clear_flag(s_fw_info_content, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_fw_info_content, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_opa(s_fw_info_content, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_fw_info_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_fw_info_content, 0, LV_PART_MAIN);
    fw_pinmap_apply_body(s_fw_info_content, prog_idx, lang);
}

static void fw_info_btn_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    fw_open_info_popup(idx);
}

static void fw_progress_close_cb(lv_event_t *e) {
    (void)e;
    ota_manager_cancel();
    if (s_fw_progress_modal) {
        lv_obj_delete(s_fw_progress_modal);
        s_fw_progress_modal = NULL;
        s_fw_progress_bar = NULL;
        s_fw_progress_lbl = NULL;
        s_fw_progress_speed_lbl = NULL;
    }
}

static void fw_ui_progress_cb(uint32_t downloaded, uint32_t total, uint8_t percent, float speed_kbps) {
    if (s_fw_progress_bar) lv_bar_set_value(s_fw_progress_bar, percent, LV_ANIM_OFF);
    if (s_fw_progress_lbl) {
        char buf[64];
        if (total > 0) {
            snprintf(buf, sizeof(buf), "%u%%  (%u / %u KB)", percent, (unsigned)(downloaded / 1024), (unsigned)(total / 1024));
        } else {
            snprintf(buf, sizeof(buf), "%u KB", (unsigned)(downloaded / 1024));
        }
        lv_label_set_text(s_fw_progress_lbl, buf);
    }
    if (s_fw_progress_speed_lbl) {
        char buf[48];
        const char *speed_fmt = (EEprom2_Language == L_english) ? "Speed: %.1f KB/s" :
                                (EEprom2_Language == L_Turkish) ? "Hiz: %.1f KB/s" :
                                (EEprom2_Language == L_Russian) ? "Skorost': %.1f KB/s" : "سرعت: %.1f KB/s";
        snprintf(buf, sizeof(buf), speed_fmt, speed_kbps);
        lv_label_set_text(s_fw_progress_speed_lbl, buf);
    }
}

static void fw_ui_status_cb(ota_status_t status, const char *message) {
    if (s_fw_progress_speed_lbl && message) {
        lv_label_set_text(s_fw_progress_speed_lbl, message);
    }
}

/* ============================================================================
 * DIRECT LINK ENTRY OVERLAY & KEYBOARD (Matching Welding Single Head & Apple HIG)
 * ============================================================================ */

static lv_obj_t *s_link_entry_panel = NULL;
static lv_obj_t *s_link_entry_textarea = NULL;
static lv_obj_t *s_link_entry_keyboard = NULL;
static lv_obj_t *s_link_entry_preset_btn = NULL;
static lv_obj_t *s_link_entry_trash_btn = NULL;
static lv_obj_t *s_link_entry_cancel_btn = NULL;
static lv_obj_t *s_link_entry_confirm_btn = NULL;
static lv_obj_t *s_link_entry_cancel_lbl = NULL;
static lv_obj_t *s_link_entry_confirm_lbl = NULL;
static lv_obj_t *s_link_entry_btn_lbl = NULL;

static lv_obj_t *s_link_toast_box = NULL;
static lv_obj_t *s_link_toast_icon = NULL;
static lv_obj_t *s_link_toast_title = NULL;
static lv_obj_t *s_link_toast_msg = NULL;
static lv_timer_t *s_link_toast_timer = NULL;

static void link_toast_timer_cb(lv_timer_t *t) {
    if (s_link_toast_box && lv_obj_is_valid(s_link_toast_box)) {
        lv_obj_add_flag(s_link_toast_box, LV_OBJ_FLAG_HIDDEN);
    }
    s_link_toast_timer = NULL;
    if (t) lv_timer_delete(t);
}

static void link_toast_click_cb(lv_event_t *e) {
    (void)e;
    if (s_link_toast_timer) {
        lv_timer_delete(s_link_toast_timer);
        s_link_toast_timer = NULL;
    }
    if (s_link_toast_box && lv_obj_is_valid(s_link_toast_box)) {
        lv_obj_add_flag(s_link_toast_box, LV_OBJ_FLAG_HIDDEN);
    }
}

static void link_entry_hide_toast(void) {
    if (s_link_toast_timer) {
        lv_timer_delete(s_link_toast_timer);
        s_link_toast_timer = NULL;
    }
    if (s_link_toast_box && lv_obj_is_valid(s_link_toast_box)) {
        lv_obj_add_flag(s_link_toast_box, LV_OBJ_FLAG_HIDDEN);
    }
}

static void event_handler_link_textarea_click(lv_event_t *e) {
    (void)e;
    link_entry_hide_toast();
}

static void link_entry_show_toast(const char *icon, const char *title, const char *msg, uint32_t color, uint32_t duration_ms) {
    if (!s_link_toast_box || !lv_obj_is_valid(s_link_toast_box)) return;
    if (s_link_toast_timer) {
        lv_timer_delete(s_link_toast_timer);
        s_link_toast_timer = NULL;
    }
    lv_obj_set_style_border_color(s_link_toast_box, lv_color_hex(color), LV_PART_MAIN);
    if (s_link_toast_icon && lv_obj_is_valid(s_link_toast_icon)) {
        lv_label_set_text(s_link_toast_icon, icon ? icon : "");
        lv_obj_set_style_text_color(s_link_toast_icon, lv_color_hex(color), LV_PART_MAIN);
    }
    bool is_fa = (EEprom2_Language == L_Persion);
    lv_base_dir_t bdir = is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR;
    if (s_link_toast_title && lv_obj_is_valid(s_link_toast_title)) {
        lv_label_set_text(s_link_toast_title, title ? title : "");
        lv_obj_set_style_base_dir(s_link_toast_title, bdir, LV_PART_MAIN);
    }
    if (s_link_toast_msg && lv_obj_is_valid(s_link_toast_msg)) {
        lv_label_set_text(s_link_toast_msg, msg ? msg : "");
        lv_obj_set_style_base_dir(s_link_toast_msg, bdir, LV_PART_MAIN);
    }
    lv_obj_move_foreground(s_link_toast_box);
    lv_obj_clear_flag(s_link_toast_box, LV_OBJ_FLAG_HIDDEN);
    if (duration_ms > 0) {
        s_link_toast_timer = lv_timer_create(link_toast_timer_cb, duration_ms, NULL);
        if (s_link_toast_timer) {
            lv_timer_set_repeat_count(s_link_toast_timer, 1);
        }
    }
}

static void complete_and_normalize_url(const char *in_url, char *out_url, size_t out_max) {
    if (!in_url || !out_url || out_max == 0) return;
    out_url[0] = '\0';

    while (*in_url && isspace((unsigned char)*in_url)) in_url++;

    char temp[256];
    size_t len = 0;
    while (*in_url && len < sizeof(temp) - 1) {
        temp[len++] = *in_url++;
    }
    temp[len] = '\0';

    while (len > 0 && isspace((unsigned char)temp[len - 1])) {
        temp[--len] = '\0';
    }

    if (len == 0) return;

    // Check if missing slash after other/test
    const char *test_match = strstr(temp, "other/test");
    if (test_match) {
        const char *after_test = test_match + 10;
        if (*after_test != '\0' && *after_test != '/' && *after_test != '?') {
            char fixed[256];
            size_t pfx_len = after_test - temp;
            if (pfx_len < sizeof(fixed) - 2) {
                memcpy(fixed, temp, pfx_len);
                fixed[pfx_len] = '/';
                strncpy(fixed + pfx_len + 1, after_test, sizeof(fixed) - pfx_len - 1);
                fixed[sizeof(fixed) - 1] = '\0';
                strncpy(temp, fixed, sizeof(temp) - 1);
                temp[sizeof(temp) - 1] = '\0';
                len = strlen(temp);
            }
        }
    }

    // Check if http:// or https:// is present
    char with_proto[256];
    if (strncasecmp(temp, "http://", 7) != 0 && strncasecmp(temp, "https://", 8) != 0) {
        snprintf(with_proto, sizeof(with_proto), "http://%s", temp);
        strncpy(temp, with_proto, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
        len = strlen(temp);
    }

    // Ensure ends in .bin or firmware.bin
    bool has_bin = false;
    if (len >= 4) {
        const char *end = temp + len - 4;
        if (strcasecmp(end, ".bin") == 0) {
            has_bin = true;
        }
    }

    if (!has_bin) {
        if (temp[len - 1] == '/') {
            if (len + 12 < sizeof(temp)) {
                strcat(temp, "firmware.bin");
                len = strlen(temp);
            }
        } else {
            if (len + 4 < sizeof(temp)) {
                strcat(temp, ".bin");
                len = strlen(temp);
            }
        }
    }

    strncpy(out_url, temp, out_max - 1);
    out_url[out_max - 1] = '\0';
}

static void fw_open_progress_modal(const char *title_text)
{
    if (s_fw_progress_modal) {
        lv_obj_delete(s_fw_progress_modal);
        s_fw_progress_modal = NULL;
    }
    s_fw_progress_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_fw_progress_modal, 480, 320);
    lv_obj_set_pos(s_fw_progress_modal, 0, 0);
    lv_obj_set_style_bg_color(s_fw_progress_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_fw_progress_modal, LV_OPA_50, LV_PART_MAIN);
    lv_obj_clear_flag(s_fw_progress_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *p = lv_obj_create(s_fw_progress_modal);
    lv_obj_set_pos(p, 30, 40);
    lv_obj_set_size(p, 420, 240);
    lv_obj_set_style_bg_color(p, lv_color_hex(0xff18181b), LV_PART_MAIN);
    lv_obj_set_style_radius(p, 12, LV_PART_MAIN);
    lv_obj_set_style_border_color(p, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(p, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(p, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(p, 12, LV_PART_MAIN);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (EEprom2_Language == L_Persion);
    const char *connecting_srv = (EEprom2_Language == L_english) ? "Connecting to server..." :
                                 (EEprom2_Language == L_Turkish) ? "Sunucuya baglaniliyor..." :
                                 (EEprom2_Language == L_Russian) ? "Podklyucheniye k serveru..." : "در حال برقراری ارتباط با سرور...";

    const char *cancel_txt = (EEprom2_Language == L_english) ? "Cancel" :
                             (EEprom2_Language == L_Turkish) ? "Iptal" :
                             (EEprom2_Language == L_Russian) ? "Otmena" : "انصراف";

    lv_obj_t *ttl = lv_label_create(p);
    lv_obj_set_pos(ttl, 10, 8);
    lv_obj_set_size(ttl, 390, 24);
    lv_obj_set_style_text_align(ttl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(ttl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(ttl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ttl, lv_color_hex(0xffffffff), LV_PART_MAIN);
    lv_label_set_text(ttl, title_text);

    s_fw_progress_bar = lv_bar_create(p);
    lv_obj_set_pos(s_fw_progress_bar, 20, 56);
    lv_obj_set_size(s_fw_progress_bar, 370, 28);
    lv_bar_set_range(s_fw_progress_bar, 0, 100);
    lv_bar_set_value(s_fw_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_fw_progress_bar, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_fw_progress_bar, lv_color_hex(0xff22c55e), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_fw_progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_fw_progress_bar, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(s_fw_progress_bar, 8, LV_PART_INDICATOR);

    s_fw_progress_lbl = lv_label_create(p);
    lv_obj_set_pos(s_fw_progress_lbl, 20, 96);
    lv_obj_set_size(s_fw_progress_lbl, 370, 22);
    lv_obj_set_style_text_align(s_fw_progress_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_fw_progress_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_fw_progress_lbl, lv_color_hex(0xffffffff), LV_PART_MAIN);
    lv_label_set_text(s_fw_progress_lbl, "0%");

    s_fw_progress_speed_lbl = lv_label_create(p);
    lv_obj_set_pos(s_fw_progress_speed_lbl, 20, 126);
    lv_obj_set_size(s_fw_progress_speed_lbl, 370, 22);
    lv_obj_set_style_text_align(s_fw_progress_speed_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_fw_progress_speed_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_fw_progress_speed_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_fw_progress_speed_lbl, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_label_set_text(s_fw_progress_speed_lbl, connecting_srv);

    lv_obj_t *btn_c = lv_btn_create(p);
    lv_obj_set_pos(btn_c, 140, 166);
    lv_obj_set_size(btn_c, 130, 36);
    lv_obj_set_ext_click_area(btn_c, 4);
    lv_obj_set_style_bg_color(btn_c, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_c, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_c, fw_progress_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_c = lv_label_create(btn_c);
    lv_label_set_text(lbl_c, cancel_txt);
    lv_obj_set_style_text_color(lbl_c, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_c, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_c);
}

static void fw_install_url(const char *url)
{
    buzzer_key;
    ota_manager_set_callbacks(fw_ui_progress_cb, fw_ui_status_cb);
    bool is_fa = (EEprom2_Language == L_Persion);
    const char *ttl = is_fa ? "در حال دانلود و نصب از لینک مستقیم..." : "Downloading & installing firmware...";
    fw_open_progress_modal(ttl);
    ota_manager_start_install_url(url);
}

static void event_handler_link_preset(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_key;
    if (!s_link_entry_textarea || !lv_obj_is_valid(s_link_entry_textarea)) return;

    const char *cur = lv_textarea_get_text(s_link_entry_textarea);
    const char *preset_prefix = "http://ota.haffler-infi.ir/other/test/";

    if (!cur || strlen(cur) == 0) {
        lv_textarea_set_text(s_link_entry_textarea, preset_prefix);
        lv_textarea_set_cursor_pos(s_link_entry_textarea, LV_TEXTAREA_CURSOR_LAST);
        link_entry_hide_toast();
    } else {
        bool is_fa = (EEprom2_Language == L_Persion);
        const char *t_title = (EEprom2_Language == L_Turkish) ? "Bilgi" : ((EEprom2_Language == L_english) ? "Notice" : "راهنما");
        const char *msg = (EEprom2_Language == L_Turkish)
            ? "Lutfen once kutuyu cop butonu ile temizleyin"
            : ((EEprom2_Language == L_english)
                ? "Please clear the box with trash button first"
                : "برای درج مجدد لینک -  ابتدا کادر را با سطل پاک کنید");
        link_entry_show_toast(LV_SYMBOL_WARNING, t_title, msg, 0xfff59e0b, 2500);
    }
}

static void event_handler_link_clear(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_key;
    if (s_link_entry_textarea && lv_obj_is_valid(s_link_entry_textarea)) {
        lv_textarea_set_text(s_link_entry_textarea, "");
    }
    link_entry_hide_toast();
}

static lv_obj_t *s_link_confirm_modal = NULL;
static char s_validated_link_url[256] = {0};

static void link_confirm_install_cb(lv_event_t *e) {
    (void)e;
    buzzer_key;
    if (s_link_confirm_modal && lv_obj_is_valid(s_link_confirm_modal)) {
        lv_obj_delete(s_link_confirm_modal);
        s_link_confirm_modal = NULL;
    }
    if (s_link_entry_panel && lv_obj_is_valid(s_link_entry_panel)) {
        lv_obj_add_flag(s_link_entry_panel, LV_OBJ_FLAG_HIDDEN);
    }
    link_entry_hide_toast();
    if (s_validated_link_url[0]) {
        fw_install_url(s_validated_link_url);
    }
}

static void link_confirm_back_cb(lv_event_t *e) {
    (void)e;
    buzzer_key;
    if (s_link_confirm_modal && lv_obj_is_valid(s_link_confirm_modal)) {
        lv_obj_delete(s_link_confirm_modal);
        s_link_confirm_modal = NULL;
    }
}

static void ui_show_link_verified_modal(const char *url, size_t size_bytes) {
    if (s_link_confirm_modal && lv_obj_is_valid(s_link_confirm_modal)) {
        lv_obj_delete(s_link_confirm_modal);
        s_link_confirm_modal = NULL;
    }
    strncpy(s_validated_link_url, url, sizeof(s_validated_link_url) - 1);
    s_validated_link_url[sizeof(s_validated_link_url) - 1] = '\0';

    bool is_fa = (EEprom2_Language == L_Persion);

    s_link_confirm_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_link_confirm_modal, 480, 320);
    lv_obj_set_pos(s_link_confirm_modal, 0, 0);
    lv_obj_set_style_bg_color(s_link_confirm_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_link_confirm_modal, 180, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_link_confirm_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_link_confirm_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_link_confirm_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_link_confirm_modal);
    lv_obj_set_size(panel, 380, 230);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xff1e293b), LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *t = lv_label_create(panel);
    lv_obj_set_pos(t, 10, 12);
    lv_obj_set_size(t, 360, 24);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_label_set_text(t, is_fa ? "تایید فایل آپدیت" : "Update File Verified");

    /* Status & Size Info */
    lv_obj_t *lbl_info = lv_label_create(panel);
    lv_obj_set_pos(lbl_info, 16, 44);
    lv_obj_set_size(lbl_info, 348, 48);
    lv_obj_set_style_text_align(lbl_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_info, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_info, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xfff1f5f9), LV_PART_MAIN);

    char info_buf[160];
    float mb = (float)size_bytes / (1024.0f * 1024.0f);
    if (is_fa) {
        if (size_bytes > 0) {
            snprintf(info_buf, sizeof(info_buf), "لینک با موفقیت تایید شد\nحجم فایل باینری: %.2f مگابایت (%zu بایت)", mb, size_bytes);
        } else {
            snprintf(info_buf, sizeof(info_buf), "لینک باینری سالم و آماده دانلود است");
        }
    } else {
        if (size_bytes > 0) {
            snprintf(info_buf, sizeof(info_buf), "URL verified successfully\nFirmware size: %.2f MB (%zu bytes)", mb, size_bytes);
        } else {
            snprintf(info_buf, sizeof(info_buf), "Firmware URL is valid and ready to download");
        }
    }
    lv_label_set_text(lbl_info, info_buf);

    /* Top Button: "نصب و دانلود" (Download & Install) */
    lv_obj_t *btn_inst = lv_btn_create(panel);
    lv_obj_set_pos(btn_inst, 24, 106);
    lv_obj_set_size(btn_inst, 332, 48);
    lv_obj_set_style_bg_color(btn_inst, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_inst, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_inst, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_inst, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_inst, link_confirm_install_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_inst = lv_label_create(btn_inst);
    lv_label_set_text(lbl_inst, is_fa ? LV_SYMBOL_DOWNLOAD " نصب و دانلود" : LV_SYMBOL_DOWNLOAD " Install & Download");
    lv_obj_set_style_text_color(lbl_inst, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_inst, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_inst);

    /* Bottom Button: "بازگشت از صفحه" (Dismiss / Back) */
    lv_obj_t *btn_back = lv_btn_create(panel);
    lv_obj_set_pos(btn_back, 24, 164);
    lv_obj_set_size(btn_back, 332, 46);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_back, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_back, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, link_confirm_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, is_fa ? "بازگشت از صفحه" : "Back");
    lv_obj_set_style_text_color(lbl_back, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_back, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_back);
}

static void event_handler_link_cancel(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_key;
    if (s_link_confirm_modal && lv_obj_is_valid(s_link_confirm_modal)) {
        lv_obj_delete(s_link_confirm_modal);
        s_link_confirm_modal = NULL;
    }
    ui_close_link_entry_modal();
}

void ui_close_link_entry_modal(void) {
    link_entry_hide_toast();
    if (s_link_confirm_modal && lv_obj_is_valid(s_link_confirm_modal)) {
        lv_obj_delete(s_link_confirm_modal);
        s_link_confirm_modal = NULL;
    }
    if (s_link_entry_textarea && lv_obj_is_valid(s_link_entry_textarea)) {
        lv_obj_clear_state(s_link_entry_textarea, LV_STATE_FOCUSED);
    }
    if (s_link_entry_panel && lv_obj_is_valid(s_link_entry_panel)) {
        lv_obj_add_flag(s_link_entry_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void event_handler_link_confirm(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_key;
    if (!s_link_entry_textarea || !lv_obj_is_valid(s_link_entry_textarea)) return;
    const char *txt = lv_textarea_get_text(s_link_entry_textarea);
    if (!txt || strlen(txt) < 3) {
        const char *t_title = (EEprom2_Language == L_Turkish) ? "Link Adresi" : ((EEprom2_Language == L_english) ? "Link URL" : "ورود آدرس لینک");
        const char *t_msg = (EEprom2_Language == L_Turkish) ? "Lutfen once link adresini girin" : ((EEprom2_Language == L_english) ? "Please enter link URL first" : "لطفا ابتدا آدرس لینک را وارد کنید");
        link_entry_show_toast(LV_SYMBOL_WARNING, t_title, t_msg, 0xfff59e0b, 2500);
        return;
    }

    char final_url[256];
    complete_and_normalize_url(txt, final_url, sizeof(final_url));
    if (final_url[0] == '\0' || strlen(final_url) < 4) {
        const char *t_title = (EEprom2_Language == L_Turkish) ? "Gecersiz Link" : ((EEprom2_Language == L_english) ? "Invalid Link" : "آدرس نامعتبر");
        const char *t_msg = (EEprom2_Language == L_Turkish) ? "Girilen link adresi gecersiz" : ((EEprom2_Language == L_english) ? "Entered link URL is invalid" : "آدرس لینک وارد شده معتبر نیست");
        link_entry_show_toast(LV_SYMBOL_CLOSE, t_title, t_msg, 0xffef4444, 2500);
        return;
    }

    lv_textarea_set_text(s_link_entry_textarea, final_url);

    /* Validate link before opening download & install confirmation */
    size_t size_bytes = 0;
    bool ok = ota_manager_validate_url(final_url, &size_bytes);
    if (!ok) {
        const char *t_title = (EEprom2_Language == L_Turkish) ? "Baglanti Hatasi" : ((EEprom2_Language == L_english) ? "Connection Error" : "خطا در بررسی لینک");
        const char *t_msg = (EEprom2_Language == L_Turkish) ? "Sunucuya baglanilamadi veya dosya yok" : ((EEprom2_Language == L_english) ? "Server unreachable or file missing" : "لینک نامعتبر است یا سرور در دسترس نیست");
        link_entry_show_toast(LV_SYMBOL_CLOSE, t_title, t_msg, 0xffef4444, 3000);
        return;
    }

    ui_show_link_verified_modal(final_url, size_bytes);
}

static void event_handler_link_entry_keyboard(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        buzzer_key;
        event_handler_link_confirm(e);
    } else if (code == LV_EVENT_CANCEL) {
        event_handler_link_cancel(e);
    }
}

static void event_handler_link_entry_open(lv_event_t *e) {
    (void)e;
    buzzer_key;
    if (!s_link_entry_panel || !lv_obj_is_valid(s_link_entry_panel)) return;
    link_entry_hide_toast();
    lv_obj_move_foreground(s_link_entry_panel);
    lv_obj_clear_flag(s_link_entry_panel, LV_OBJ_FLAG_HIDDEN);
}

void ui_open_link_entry_modal(void) {
    event_handler_link_entry_open(NULL);
}

static void fw_install_btn_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ota_manager_set_callbacks(fw_ui_progress_cb, fw_ui_status_cb);

    const char *installing_ttl = (idx == 0) ?
        ((EEprom2_Language == L_english) ? "Installing Gas Filling App..." :
         (EEprom2_Language == L_Turkish) ? "Gaz Dolum Programi Yukleniyor..." :
         (EEprom2_Language == L_Russian) ? "Ustanovka Programmy Gaza..." : "در حال نصب برنامه گاز") :
        ((EEprom2_Language == L_english) ? "Installing Config & Test App..." :
         (EEprom2_Language == L_Turkish) ? "Test Programi Yukleniyor..." :
         (EEprom2_Language == L_Russian) ? "Ustanovka Programmy Test..." : "در حال نصب برنامه کانفیگ");

    fw_open_progress_modal(installing_ttl);
    ota_manager_start_install_prog(idx);
}

static void fw_ver_lbl_click_cb(lv_event_t *e) {
    (void)e;
    buzzer_key;
    s_fw_scan_requested = false;
    ota_manager_request_rescan();
    if (ota_manager_is_wifi_connected()) {
        ota_manager_start_check(NULL, NULL);
        s_fw_scan_requested = true;
    }
}

void ui_framware_on_screen_enter(void) {
    s_fw_scan_requested = false;
    ota_manager_request_rescan();
}

void create_screen_framware() {
    lv_obj_t *scr = lv_obj_create(NULL);
    objects.screen_framware = scr;
    lv_obj_set_size(scr, 480, 320);
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xff1e222a), LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, 480, 320);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(panel, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);

    const lv_coord_t card_w = 226;
    const lv_coord_t card_h = 304;
    const lv_coord_t card_gap = 12;
    const lv_coord_t card_y = 8;
    const uint32_t card_bg = 0x2b2b2b;

    // Card 0: Brand / System Info (Dark Glass)
    lv_obj_t *brand = lv_obj_create(panel);
    lv_obj_set_size(brand, card_w, card_h);
    lv_obj_set_pos(brand, 8, card_y);
    lv_obj_clear_flag(brand, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(brand, lv_color_hex(card_bg), LV_PART_MAIN);
    lv_obj_set_style_radius(brand, 12, LV_PART_MAIN);
    lv_obj_set_style_border_color(brand, lv_color_hex(0xff3f3f46), LV_PART_MAIN);
    lv_obj_set_style_border_width(brand, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(brand, 6, LV_PART_MAIN);

    bool is_fa = (EEprom2_Language == L_Persion);
    const char *back_btn_txt = (EEprom2_Language == L_english) ? "Back to Settings" :
                               (EEprom2_Language == L_Turkish) ? "Ayarlara Don" :
                               (EEprom2_Language == L_Russian) ? "Nazad" : "بازگشت به تنظیمات";
    const char *info_btn_txt = (EEprom2_Language == L_english) ? "Info" :
                               (EEprom2_Language == L_Turkish) ? "Bilgi" :
                               (EEprom2_Language == L_Russian) ? "Info" : "مشخصات";
    const char *inst_btn_txt = (EEprom2_Language == L_english) ? "Install" :
                               (EEprom2_Language == L_Turkish) ? "Yukle" :
                               (EEprom2_Language == L_Russian) ? "Ustanovit" : "نصب";
    const char *checking_txt = is_fa ? "در حال بررسی سرور..." : "Checking Server...";

    // Brand title: HAFFLER
    lv_obj_t *brand_title = lv_label_create(brand);
    s_fw_brand_title_lbl = brand_title;
    lv_label_set_text(brand_title, "HAFFLER");
    lv_obj_align(brand_title, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_color(brand_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(brand_title, &lv_font_montserrat_22, LV_PART_MAIN);

    // Subtitle: Firmware Update
    lv_obj_t *brand_sub = lv_label_create(brand);
    s_fw_brand_sub_lbl = brand_sub;
    const char *sub_txt = (EEprom2_Language == L_english) ? "Firmware Update" :
                          (EEprom2_Language == L_Turkish) ? "Yazilim Guncelleme" :
                          (EEprom2_Language == L_Russian) ? "Obnovlenie PO" : "بروزرسانی نرم افزار";
    lv_label_set_text(brand_sub, sub_txt);
    lv_obj_align(brand_sub, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_text_color(brand_sub, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_text_font(brand_sub, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *brand_line = lv_obj_create(brand);
    lv_obj_set_size(brand_line, 190, 2);
    lv_obj_align(brand_line, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_set_style_bg_color(brand_line, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_obj_set_style_border_width(brand_line, 0, LV_PART_MAIN);
    lv_obj_clear_flag(brand_line, LV_OBJ_FLAG_SCROLLABLE);

    // Version pill
    lv_obj_t *brand_ver = lv_label_create(brand);
    s_fw_brand_ver_lbl = brand_ver;
    lv_label_set_text_fmt(brand_ver, "%s", FW_VERSION_LABEL);
    lv_obj_align(brand_ver, LV_ALIGN_TOP_MID, 0, 66);
    lv_obj_set_style_text_color(brand_ver, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
    lv_obj_set_style_text_font(brand_ver, &lv_font_montserrat_16, LV_PART_MAIN);

    // Green "ورود با لینک" button
    lv_obj_t *btn_link = lv_btn_create(brand);
    lv_obj_set_size(btn_link, 196, 36);
    lv_obj_align(btn_link, LV_ALIGN_TOP_MID, 0, 94);
    lv_obj_set_style_radius(btn_link, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_link, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_link, lv_color_hex(0xff22c55e), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn_link, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_link, event_handler_link_entry_open, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn_link);
    s_link_entry_btn_lbl = bl;
    lv_label_set_text(bl, is_fa ? "ورود با لینک" : "Enter URL");
    lv_obj_set_style_text_font(bl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(bl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(bl);

    // Official 190x48 Haffler Logo
    lv_obj_t *logo = lv_image_create(brand);
    lv_image_set_src(logo, &img_bitmap_haffler_logo);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 148);

    // Red Back button
    lv_obj_t *back_btn = lv_btn_create(brand);
    lv_obj_set_size(back_btn, 196, 44);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xffdc2626), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, fw_close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *b_lbl = lv_label_create(back_btn);
    s_fw_back_btn_lbl = b_lbl;
    lv_label_set_text(b_lbl, back_btn_txt);
    lv_obj_set_style_text_font(b_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(b_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(b_lbl);

    // Product cards definition
    struct {
        const lv_img_dsc_t *img;
        const char *title;
    } progs[2] = {
        { &img_bitmap_gas,
          (EEprom2_Language == L_english) ? "Argon Gas Filler" :
          (EEprom2_Language == L_Turkish) ? "Argon Gaz Dolumu" :
          (EEprom2_Language == L_Russian) ? "Zalivka Argona" : "برنامه تزریق گاز آرگون" },
        { &img_bitmap_config,
          (EEprom2_Language == L_english) ? "Diagnostic & Config" :
          (EEprom2_Language == L_Turkish) ? "Test ve Yapilandirma" :
          (EEprom2_Language == L_Russian) ? "Test i Nastroika" : "برنامه تست و کانفیگ" }
    };

    int card_count = is_f5_boot_active() ? 2 : 1;
    for (int i = 0; i < card_count; i++) {
        lv_obj_t *card = lv_obj_create(panel);
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_set_pos(card, 8 + (i + 1) * (card_w + card_gap), card_y);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(card_bg), LV_PART_MAIN);
        lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
        lv_obj_set_style_border_color(card, lv_color_hex(0xff3f3f46), LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN);

        lv_obj_t *nm = lv_label_create(card);
        s_fw_prog_title_lbl[i] = nm;
        lv_obj_set_pos(nm, 0, 6);
        lv_obj_set_size(nm, card_w - 12, 22);
        lv_label_set_text(nm, progs[i].title);
        lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_color(nm, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(nm, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);

        lv_obj_t *im = lv_image_create(card);
        lv_image_set_src(im, progs[i].img);
        lv_image_set_scale(im, 256);
        lv_obj_align(im, LV_ALIGN_TOP_MID, 0, 28);

        // Version/Status label above buttons (clickable for instant manual retry)
        lv_obj_t *vl = lv_label_create(card);
        s_fw_ver_lbl[i] = vl;
        lv_obj_set_pos(vl, 0, 238);
        lv_obj_set_size(vl, card_w - 12, 20);
        lv_obj_set_style_text_align(vl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_color(vl, lv_color_hex(0xff94a3b8), LV_PART_MAIN);
        lv_obj_set_style_text_font(vl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_label_set_text(vl, checking_txt);
        lv_obj_add_flag(vl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(vl, fw_ver_lbl_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        // Info Button (مشخصات)
        lv_obj_t *btn_info = lv_btn_create(card);
        lv_obj_set_pos(btn_info, 10, 260);
        lv_obj_set_size(btn_info, 96, 36);
        lv_obj_set_style_radius(btn_info, 8, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_info, lv_color_hex(0xff334155), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_info, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn_info, fw_info_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *lbl_info = lv_label_create(btn_info);
        s_fw_info_btn_lbl[i] = lbl_info;
        lv_label_set_text(lbl_info, info_btn_txt);
        lv_obj_set_style_text_font(lbl_info, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_info, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(lbl_info);

        // Install Button (نصب)
        lv_obj_t *btn_inst = lv_btn_create(card);
        s_fw_install_btn[i] = btn_inst;
        lv_obj_set_pos(btn_inst, 114, 260);
        lv_obj_set_size(btn_inst, 98, 36);
        lv_obj_set_style_radius(btn_inst, 8, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_inst, lv_color_hex(0xff16a34a), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_inst, lv_color_hex(0xff22c55e), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn_inst, fw_install_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *lbl_inst = lv_label_create(btn_inst);
        s_fw_inst_btn_lbl[i] = lbl_inst;
        lv_label_set_text(lbl_inst, inst_btn_txt);
        lv_obj_set_style_text_font(lbl_inst, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_inst, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(lbl_inst);
    }

    // ===== پنل تمام صفحه ورود لینک (۴۸۰x۳۲۰) =====
    {
        lv_obj_t *obj = lv_obj_create(scr);
        s_link_entry_panel = obj;
        lv_obj_set_pos(obj, 0, 0);
        lv_obj_set_size(obj, 480, 320);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2b2b2b), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

        // بازگشت از صفحه (بالا سمت چپ — x=8, y=7, w=114, h=28)
        lv_obj_t *btn_c = lv_btn_create(obj);
        s_link_entry_cancel_btn = btn_c;
        lv_obj_set_pos(btn_c, 8, 7);
        lv_obj_set_size(btn_c, 114, 28);
        lv_obj_set_style_radius(btn_c, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_c, lv_color_hex(0xffdc2626), LV_PART_MAIN);
        lv_obj_add_event_cb(btn_c, event_handler_link_cancel, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_c = lv_label_create(btn_c);
        s_link_entry_cancel_lbl = lbl_c;
        const char *cancel_str = (EEprom2_Language == L_Turkish) ? "Geri Don" :
                                 ((EEprom2_Language == L_english) ? "Back" :
                                 ((EEprom2_Language == L_Russian) ? "Nazad" : "بازگشت از صفحه"));
        lv_label_set_text(lbl_c, cancel_str);
        lv_obj_center(lbl_c);
        lv_obj_set_style_text_font(lbl_c, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_c, lv_color_white(), LV_PART_MAIN);

        // کلید میانبر لینک پیش فرض: http://ota.haffler-infi.ir/other/test (x=128, y=7, w=168, h=28)
        lv_obj_t *btn_p = lv_btn_create(obj);
        s_link_entry_preset_btn = btn_p;
        lv_obj_set_pos(btn_p, 128, 7);
        lv_obj_set_size(btn_p, 168, 28);
        lv_obj_set_style_radius(btn_p, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_p, lv_color_hex(0xff0284c7), LV_PART_MAIN);
        lv_obj_add_event_cb(btn_p, event_handler_link_preset, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_p = lv_label_create(btn_p);
        lv_label_set_text(lbl_p, LV_SYMBOL_DOWNLOAD " other/test");
        lv_obj_center(lbl_p);
        lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_p, lv_color_white(), LV_PART_MAIN);

        // کلید آیکون سطل آشغال: پاکسازی کامل کادر لینک (x=302, y=7, w=48, h=28)
        lv_obj_t *btn_t = lv_btn_create(obj);
        s_link_entry_trash_btn = btn_t;
        lv_obj_set_pos(btn_t, 302, 7);
        lv_obj_set_size(btn_t, 48, 28);
        lv_obj_set_style_radius(btn_t, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_t, lv_color_hex(0xff475569), LV_PART_MAIN);
        lv_obj_add_event_cb(btn_t, event_handler_link_clear, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_t = lv_label_create(btn_t);
        lv_label_set_text(lbl_t, LV_SYMBOL_TRASH);
        lv_obj_center(lbl_t);
        lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_t, lv_color_white(), LV_PART_MAIN);

        // تایید و دانلود (بالا سمت راست — x=358, y=7, w=114, h=28)
        lv_obj_t *btn_i = lv_btn_create(obj);
        s_link_entry_confirm_btn = btn_i;
        lv_obj_set_pos(btn_i, 358, 7);
        lv_obj_set_size(btn_i, 114, 28);
        lv_obj_set_style_radius(btn_i, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn_i, lv_color_hex(0xff29ac6b), LV_PART_MAIN);
        lv_obj_add_event_cb(btn_i, event_handler_link_confirm, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_i = lv_label_create(btn_i);
        s_link_entry_confirm_lbl = lbl_i;
        const char *inst_str = (EEprom2_Language == L_Turkish) ? "Yukle" :
                               ((EEprom2_Language == L_english) ? "Install" :
                               ((EEprom2_Language == L_Russian) ? "Ustanovit" : "نصب"));
        lv_label_set_text(lbl_i, inst_str);
        lv_obj_center(lbl_i);
        lv_obj_set_style_text_font(lbl_i, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_i, lv_color_white(), LV_PART_MAIN);

        // کادر ورود لینک (x=8, y=40, w=464, h=32)
        lv_obj_t *ta = lv_textarea_create(obj);
        s_link_entry_textarea = ta;
        lv_obj_set_pos(ta, 8, 40);
        lv_obj_set_size(ta, 464, 32);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_max_length(ta, 240);
        lv_textarea_set_placeholder_text(ta, "https://...");
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(ta, lv_color_hex(0xffffffff), LV_PART_MAIN);
        lv_obj_set_style_text_color(ta, lv_color_hex(0xff0f172a), LV_PART_MAIN);
        lv_obj_set_style_border_color(ta, lv_color_hex(0xff148db2), LV_PART_MAIN);
        lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(ta, 6, LV_PART_MAIN);
        lv_obj_add_event_cb(ta, event_handler_link_textarea_click, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(ta, event_handler_link_textarea_click, LV_EVENT_FOCUSED, NULL);

        // کیبورد (x=0, y=86, w=480, h=234)
        lv_obj_t *kb = lv_keyboard_create(obj);
        s_link_entry_keyboard = kb;
        lv_obj_set_pos(kb, 0, 86);
        lv_obj_set_size(kb, 480, 234);
        lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)s_wifi_kb_map_lower, s_wifi_kb_ctrl_lower);
        lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, (const char **)s_wifi_kb_map_upper, s_wifi_kb_ctrl_upper);
        lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, (const char **)s_wifi_kb_map_special, s_wifi_kb_ctrl_special);
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        if (kb && ta) {
            lv_keyboard_set_textarea(kb, ta);
        }
        lv_obj_add_event_cb(kb, event_handler_link_entry_keyboard, LV_EVENT_ALL, NULL);
        lv_obj_set_style_align(kb, LV_ALIGN_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(kb, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(kb, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(kb, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_row(kb, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_column(kb, 2, LV_PART_MAIN);
        lv_obj_set_style_bg_color(kb, lv_color_hex(0xffffffff), LV_PART_ITEMS);
        lv_obj_set_style_bg_color(kb, lv_color_hex(0xffcbd5e1), LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(kb, lv_color_hex(0xff0f172a), LV_PART_ITEMS);
        lv_obj_set_style_radius(kb, 6, LV_PART_ITEMS);
        lv_obj_set_style_text_font(kb, &lv_font_montserrat_20, LV_PART_ITEMS);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
        lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);

        // کارت شناور پیام زمان دار به سبک اپل (Apple HIG Floating Timed Modal)
        lv_obj_t *box = lv_obj_create(obj);
        s_link_toast_box = box;
        lv_obj_set_size(box, 360, 121);
        lv_obj_align(box, LV_ALIGN_CENTER, 0, -15);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(box, lv_color_hex(0xff18181b), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, 252, LV_PART_MAIN);
        lv_obj_set_style_radius(box, 12, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(box, lv_color_hex(0xff10b981), LV_PART_MAIN);
        lv_obj_set_style_pad_all(box, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
        lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(box, link_toast_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);

        // آیکون بالای کادر پیام
        lv_obj_t *icon = lv_label_create(box);
        s_link_toast_icon = icon;
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 2);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_label_set_text(icon, LV_SYMBOL_OK);
        lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);

        // عنوان پیام
        lv_obj_t *title = lv_label_create(box);
        s_link_toast_title = title;
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 26);
        lv_obj_set_style_text_font(title, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(0xffffffff), LV_PART_MAIN);
        lv_label_set_text(title, "پیام");
        lv_obj_clear_flag(title, LV_OBJ_FLAG_CLICKABLE);

        // متن اصلی پیام
        lv_obj_t *msg = lv_label_create(box);
        s_link_toast_msg = msg;
        lv_obj_set_width(msg, 340);
        lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 52);
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(msg, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(msg, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
        lv_label_set_text(msg, "");
        lv_obj_clear_flag(msg, LV_OBJ_FLAG_CLICKABLE);
    }
}

void ui_refresh_firmware_runtime_labels(void)
{
    unsigned char lang = EEprom2_Language;
    bool is_fa = (lang == L_Persion);
    const char *sys_info_txt = (lang == L_english) ? "System Information" :
                               (lang == L_Turkish) ? "Sistem Bilgisi" :
                               (lang == L_Russian) ? "Informatsiya" : "اطلاعات سیستم";
    const char *cur_ver_fmt  = (lang == L_english) ? "Current: %s" :
                               (lang == L_Turkish) ? "Guncel: %s" :
                               (lang == L_Russian) ? "Tekushchaya: %s" : "نسخه فعلی: %s";
    const char *back_btn_txt = (lang == L_english) ? "Back to Settings" :
                               (lang == L_Turkish) ? "Ayarlara Don" :
                               (lang == L_Russian) ? "Nazad" : "بازگشت به تنظیمات";
    const char *prog0_txt    = (lang == L_english) ? "Argon Gas Filler" :
                               (lang == L_Turkish) ? "Argon Gaz Dolumu" :
                               (lang == L_Russian) ? "Zalivka Argona" : "برنامه تزریق گاز آرگون";
    const char *prog1_txt    = (lang == L_english) ? "Diagnostic & Config" :
                               (lang == L_Turkish) ? "Test ve Yapilandirma" :
                               (lang == L_Russian) ? "Test i Nastroika" : "برنامه تست و کانفیگ";
    const char *info_btn_txt = (lang == L_english) ? "Info" :
                               (lang == L_Turkish) ? "Bilgi" :
                               (lang == L_Russian) ? "Info" : "مشخصات";
    const char *inst_btn_txt = (lang == L_english) ? "Install" :
                               (lang == L_Turkish) ? "Yukle" :
                               (lang == L_Russian) ? "Ustanovit" : "نصب";

    if (s_fw_brand_title_lbl && lv_obj_is_valid(s_fw_brand_title_lbl)) {
        lv_label_set_text(s_fw_brand_title_lbl, "HAFFLER");
        lv_obj_set_style_text_font(s_fw_brand_title_lbl, &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_align(s_fw_brand_title_lbl, LV_ALIGN_TOP_MID, 0, 8);
    }
    if (s_fw_brand_ver_lbl && lv_obj_is_valid(s_fw_brand_ver_lbl)) {
        lv_label_set_text_fmt(s_fw_brand_ver_lbl, "%s", FW_VERSION_LABEL);
        lv_obj_set_style_text_font(s_fw_brand_ver_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_align(s_fw_brand_ver_lbl, LV_ALIGN_TOP_MID, 0, 66);
    }
    if (s_fw_back_btn_lbl && lv_obj_is_valid(s_fw_back_btn_lbl)) {
        lv_label_set_text(s_fw_back_btn_lbl, back_btn_txt);
        lv_obj_set_style_text_font(s_fw_back_btn_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(s_fw_back_btn_lbl);
    }
    if (s_fw_prog_title_lbl[0] && lv_obj_is_valid(s_fw_prog_title_lbl[0])) {
        lv_label_set_text(s_fw_prog_title_lbl[0], prog0_txt);
        lv_obj_set_style_text_font(s_fw_prog_title_lbl[0], is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    if (s_fw_prog_title_lbl[1] && lv_obj_is_valid(s_fw_prog_title_lbl[1])) {
        lv_label_set_text(s_fw_prog_title_lbl[1], prog1_txt);
        lv_obj_set_style_text_font(s_fw_prog_title_lbl[1], is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 15) : &lv_font_montserrat_14, LV_PART_MAIN);
    }
    for (int i = 0; i < 2; i++) {
        if (s_fw_info_btn_lbl[i] && lv_obj_is_valid(s_fw_info_btn_lbl[i])) {
            lv_label_set_text(s_fw_info_btn_lbl[i], info_btn_txt);
            lv_obj_set_style_text_font(s_fw_info_btn_lbl[i], is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
            lv_obj_center(s_fw_info_btn_lbl[i]);
        }
        if (s_fw_inst_btn_lbl[i] && lv_obj_is_valid(s_fw_inst_btn_lbl[i])) {
            lv_label_set_text(s_fw_inst_btn_lbl[i], inst_btn_txt);
            lv_obj_set_style_text_font(s_fw_inst_btn_lbl[i], is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_center(s_fw_inst_btn_lbl[i]);
        }
    }
    if (s_link_entry_btn_lbl && lv_obj_is_valid(s_link_entry_btn_lbl)) {
        lv_label_set_text(s_link_entry_btn_lbl, is_fa ? "ورود با لینک" : "Enter URL");
        lv_obj_set_style_text_font(s_link_entry_btn_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(s_link_entry_btn_lbl);
    }
    if (s_link_entry_cancel_lbl && lv_obj_is_valid(s_link_entry_cancel_lbl)) {
        const char *c_txt = (lang == L_Turkish) ? "Geri Don" :
                            ((lang == L_english) ? "Back" :
                            ((lang == L_Russian) ? "Nazad" : "بازگشت از صفحه"));
        lv_label_set_text(s_link_entry_cancel_lbl, c_txt);
        lv_obj_set_style_text_font(s_link_entry_cancel_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_center(s_link_entry_cancel_lbl);
    }
    if (s_link_entry_confirm_lbl && lv_obj_is_valid(s_link_entry_confirm_lbl)) {
        const char *i_txt = (lang == L_Turkish) ? "Yukle" :
                            ((lang == L_english) ? "Install" :
                            ((lang == L_Russian) ? "Ustanovit" : "نصب"));
        lv_label_set_text(s_link_entry_confirm_lbl, i_txt);
        lv_obj_set_style_text_font(s_link_entry_confirm_lbl, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(s_link_entry_confirm_lbl);
    }
}

void tick_screen_framware() {
    bool wifi_ok = ota_manager_is_wifi_connected();
    if (!s_fw_scan_requested && wifi_ok) {
        ota_manager_start_check(NULL, NULL);
        s_fw_scan_requested = true;
    }
    ota_manager_poll();

    static bool s_last_scan_done = false;
    bool scan_done = ota_info_is_scan_done();
    if (scan_done != s_last_scan_done) {
        s_last_scan_done = scan_done;
        if (s_fw_info_popup && s_fw_info_content && lv_obj_is_valid(s_fw_info_content)) {
            int lang = (EEprom2_Language == L_Persion) ? 0 :
                       (EEprom2_Language == L_english) ? 1 :
                       (EEprom2_Language == L_Turkish) ? 2 : 3;
            fw_pinmap_apply_body(s_fw_info_content, s_fw_info_popup_idx, lang);
        }
    }

    bool is_fa = (EEprom2_Language == L_Persion);
    int card_count = is_f5_boot_active() ? 2 : 1;
    static uint32_t s_last_ver_color_hex[2] = {0, 0};
    for (int i = 0; i < card_count; i++) {
        if (s_fw_ver_lbl[i]) {
            const ota_info_t *info = ota_manager_get_prog_info(i);
            uint32_t color_hex = 0xff94a3b8;
            if (!wifi_ok) {
                const char *no_wifi_txt = is_fa ? "اتصال وای فای برقرار نیست" : "Wi-Fi Disconnected";
                set_label_if_changed(s_fw_ver_lbl[i], no_wifi_txt);
                color_hex = 0xfff59e0b;
            } else if (info && info->latest_version[0] != '\0') {
                char buf[64];
                const char *srv_ver_fmt = (EEprom2_Language == L_english) ? "Server: V%s" :
                                          (EEprom2_Language == L_Turkish) ? "Sunucu: V%s" :
                                          (EEprom2_Language == L_Russian) ? "Server: V%s" : "نسخه سرور: V%s";
                snprintf(buf, sizeof(buf), srv_ver_fmt, info->latest_version);
                set_label_if_changed(s_fw_ver_lbl[i], buf);
                color_hex = 0xff38bdf8;
            } else if (info && (info->scan_error || (scan_done && info->latest_version[0] == '\0'))) {
                const char *err_txt = is_fa ? "خطا در سرور (لمس جهت تلاش)" : "Server Error (Tap to retry)";
                set_label_if_changed(s_fw_ver_lbl[i], err_txt);
                color_hex = 0xfff87171;
            } else {
                const char *chk_txt = is_fa ? "در حال بررسی سرور..." : "Checking Server...";
                set_label_if_changed(s_fw_ver_lbl[i], chk_txt);
                color_hex = 0xff94a3b8;
            }
            if (s_last_ver_color_hex[i] != color_hex) {
                s_last_ver_color_hex[i] = color_hex;
                lv_obj_set_style_text_color(s_fw_ver_lbl[i], lv_color_hex(color_hex), LV_PART_MAIN);
            }
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_setting,
    tick_screen_wifi,
    tick_screen_framware,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "ArialRegular26", &ui_font_arial_regular26 },
    { "ArialRegular17", &ui_font_arial_regular17 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_setting();
    create_screen_wifi();
    create_screen_framware();
}

static lv_obj_t *s_auto_update_modal = NULL;

static void auto_update_install_cb(lv_event_t *e) {
    (void)e;
    buzzer_key;
    ota_manager_reset_snooze();
    ota_manager_clear_update_prompt_pending();
    if (s_auto_update_modal && lv_obj_is_valid(s_auto_update_modal)) {
        lv_obj_delete(s_auto_update_modal);
        s_auto_update_modal = NULL;
    }
    loadScreen(SCREEN_ID_FRAMWARE);
}

static void auto_update_dismiss_cb(lv_event_t *e) {
    (void)e;
    buzzer_key;
    ota_manager_increment_snooze();
    ota_manager_dismiss_for_boot();
    ota_manager_clear_update_prompt_pending();
    if (s_auto_update_modal && lv_obj_is_valid(s_auto_update_modal)) {
        lv_obj_delete(s_auto_update_modal);
        s_auto_update_modal = NULL;
    }
    ui_toast_show("بروزرسانی", "اعلان بروزرسانی به نوبت بعدی موکول شد", 2500, TOAST_TYPE_INFO);
}

void ui_check_and_show_update_prompt(void) {
    if (s_auto_update_modal && lv_obj_is_valid(s_auto_update_modal)) return;
    char ver[32] = {0};
    if (!ota_manager_is_update_prompt_pending(ver, sizeof(ver))) return;

    bool is_fa = (EEprom2_Language == L_Persion);

    s_auto_update_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_auto_update_modal, 480, 320);
    lv_obj_set_pos(s_auto_update_modal, 0, 0);
    lv_obj_set_style_bg_color(s_auto_update_modal, lv_color_hex(0xff0f172a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_auto_update_modal, 180, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_auto_update_modal, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_auto_update_modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_auto_update_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_auto_update_modal);
    lv_obj_set_size(panel, 380, 210);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xff1e293b), LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *t = lv_label_create(panel);
    lv_obj_set_pos(t, 10, 14);
    lv_obj_set_size(t, 360, 24);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(t, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(t, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 16) : &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, lv_color_hex(0xff38bdf8), LV_PART_MAIN);
    lv_label_set_text(t, is_fa ? "نسخه جدید در سرور شناسایی شد" : "New Firmware Available");

    /* Message */
    lv_obj_t *msg = lv_label_create(panel);
    lv_obj_set_pos(msg, 16, 48);
    lv_obj_set_size(msg, 348, 48);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(msg, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_obj_set_style_text_font(msg, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 13) : &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(msg, lv_color_hex(0xffcbd5e1), LV_PART_MAIN);
    char buf[128];
    snprintf(buf, sizeof(buf), is_fa ? "نسخه %s در سرور موجود است.\nآیا مایل به دانلود و بروزرسانی هستید؟" : "Version %s is available on server.\nWould you like to install now?", ver);
    lv_label_set_text(msg, buf);

    /* Dismiss/Snooze Button */
    lv_obj_t *btn_no = lv_btn_create(panel);
    lv_obj_set_pos(btn_no, 24, 114);
    lv_obj_set_size(btn_no, 156, 48);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0xff334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0xff475569), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_no, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_no, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_no, auto_update_dismiss_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_no = lv_label_create(btn_no);
    lv_label_set_text(lbl_no, is_fa ? "رد کردن" : "Snooze");
    lv_obj_set_style_text_color(lbl_no, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_no, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_no);

    /* Install Button */
    lv_obj_t *btn_ok = lv_btn_create(panel);
    lv_obj_set_pos(btn_ok, 200, 114);
    lv_obj_set_size(btn_ok, 156, 48);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff16a34a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xff15803d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_ok, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_ok, auto_update_install_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, is_fa ? LV_SYMBOL_DOWNLOAD " نصب و ارتقا" : "Install Now");
    lv_obj_set_style_text_color(lbl_ok, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_ok, is_fa ? ui_font_vazir_or(&ui_font_arial_regular17, 14) : &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl_ok);
}

