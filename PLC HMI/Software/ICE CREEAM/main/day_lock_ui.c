#include "day_lock_ui.h"
#include "day_lock_core.h"
#include "panel_nav.h"
#include "lvgl_port.h"
#include "fonts.h"
#include "buzzer.h"
#include "wifi.h"
#include "ui/actions.h"
#include "ui/ui_lang.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "day_lock_ui";

static lv_obj_t *s_modal_mask = NULL;
static lv_obj_t *s_card = NULL;
static lv_obj_t *s_pin_label = NULL;
static lv_obj_t *s_chal_lbl = NULL;
static lv_obj_t *s_err_banner = NULL;
static lv_obj_t *s_btnm = NULL;
static lv_obj_t *s_help_dialog = NULL;
static lv_obj_t *s_btn_eye_lbl = NULL;

static bool s_pin_visible = false;
static char s_entered_pin[16] = {0};
static uint8_t s_pin_len = 0;
static uint8_t s_nav_btn_idx = 0;

/* ارجاع به تابع رنگ‌آمیزی کلیدهای کیبورد */
extern void keyboard_color_draw_task_cb(lv_event_t *e);

static void update_pin_display(void)
{
    if (!s_pin_label || !lv_obj_is_valid(s_pin_label)) return;

    if (s_pin_len == 0) {
        lv_label_set_text(s_pin_label, ui_lang_str(STR_PIN_PROMPT));
        lv_obj_set_style_text_color(s_pin_label, lv_color_hex(0x94A3B8), 0);
        lv_obj_set_style_text_font(s_pin_label, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(s_pin_label, ui_lang_base_dir(), 0);
        lv_obj_center(s_pin_label);
    } else {
        char buf[32] = {0};
        size_t pos = 0;
        for (uint8_t i = 0; i < s_pin_len && pos + 2 < sizeof(buf); i++) {
            buf[pos++] = s_pin_visible ? s_entered_pin[i] : '*';
            if (s_pin_len <= 5) {
                buf[pos++] = ' ';
            }
        }
        buf[pos] = '\0';
        lv_label_set_text(s_pin_label, buf);
        lv_obj_set_style_text_color(s_pin_label, lv_color_hex(0x0F172A), 0);
        lv_obj_set_style_text_font(s_pin_label, (s_pin_len > 6) ? &lv_font_montserrat_16 : &lv_font_montserrat_24, 0);
        lv_obj_set_style_base_dir(s_pin_label, LV_BASE_DIR_LTR, 0);
        lv_obj_align(s_pin_label, LV_ALIGN_CENTER, 0, 3);
    }
}

static lv_timer_t *s_sync_status_timer = NULL;
static lv_timer_t *s_lockout_timer = NULL;

static void sync_status_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
        lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_sync_status_timer) {
        lv_timer_del(s_sync_status_timer);
        s_sync_status_timer = NULL;
    }
}

static void lockout_timer_cb(lv_timer_t *t)
{
    (void)t;
    uint32_t rem_sec = 0;
    if (day_lock_is_brute_locked(&rem_sec)) {
        if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
            char freeze_msg[96];
            if (ui_lang_get() == UI_LANG_FA) {
                if (rem_sec >= 60) {
                    snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است\n(%u دقیقه صبر کنید)", (unsigned)((rem_sec + 59) / 60));
                } else {
                    snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است\n(%u ثانیه صبر کنید)", (unsigned)rem_sec);
                }
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(freeze_msg, sizeof(freeze_msg), "Giris engellendi\n(%u dk bekleyin)", (unsigned)((rem_sec + 59) / 60));
            } else {
                snprintf(freeze_msg, sizeof(freeze_msg), "Input locked\n(wait %u min)", (unsigned)((rem_sec + 59) / 60));
            }
            lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
            lv_label_set_text(s_err_banner, freeze_msg);
            lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
            lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_lockout_timer) {
            lv_timer_del(s_lockout_timer);
            s_lockout_timer = NULL;
        }
    }
}

void day_lock_ui_notify_time_sync(bool success)
{
    if (!lvgl_lock(pdMS_TO_TICKS(200))) return;

    if (!s_err_banner || !lv_obj_is_valid(s_err_banner)) {
        lvgl_unlock();
        return;
    }

    if (s_sync_status_timer) {
        lv_timer_del(s_sync_status_timer);
        s_sync_status_timer = NULL;
    }

    if (success) {
        lv_label_set_text(s_err_banner, (ui_lang_get() == UI_LANG_FA) ? "ساعت با اینترنت تنظیم شد" :
                                        ((ui_lang_get() == UI_LANG_TR) ? "Saat esitlendi" : "Time synchronized"));
        lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0x16A34A), 0);
        lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        s_sync_status_timer = lv_timer_create(sync_status_timer_cb, 3000, NULL);
    } else {
        lv_label_set_text(s_err_banner, (ui_lang_get() == UI_LANG_FA) ? "خطا در دریافت ساعت" :
                                        ((ui_lang_get() == UI_LANG_TR) ? "Saat esitleme hatasi" : "Time sync failed"));
        lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
        lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        s_sync_status_timer = lv_timer_create(sync_status_timer_cb, 4000, NULL);
    }
    lvgl_unlock();
}

static void eye_btn_cb(lv_event_t *e)
{
    (void)e;
    s_pin_visible = !s_pin_visible;
    buzzer_beep_click();
    if (s_btn_eye_lbl && lv_obj_is_valid(s_btn_eye_lbl)) {
        lv_label_set_text(s_btn_eye_lbl, s_pin_visible ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
    }
    update_pin_display();
}

static void wifi_sync_btn_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();

    if (s_sync_status_timer) {
        lv_timer_del(s_sync_status_timer);
        s_sync_status_timer = NULL;
    }

    if (wifi_is_connected()) {
        if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
            lv_label_set_text(s_err_banner, ui_lang_str(STR_DL_SYNCING_TIME));
            lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0x0284C7), 0);
            lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
            s_sync_status_timer = lv_timer_create(sync_status_timer_cb, 8000, NULL);
        }
        wifi_sntp_sync_start();
        panel_nav_toast_info(ui_lang_str(STR_DL_SYNCING_TIME));
    } else {
        if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
            lv_label_set_text(s_err_banner, ui_lang_str(STR_DL_NO_WIFI));
            lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEA580C), 0);
            lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
            s_sync_status_timer = lv_timer_create(sync_status_timer_cb, 4000, NULL);
        }
        if (!day_lock_is_locked()) {
            day_lock_ui_hide();
            open_settings_tab(3);
            wifi_start_scan();
        }
        panel_nav_toast_warning(ui_lang_str(STR_DL_NO_WIFI));
    }
}

static void day_lock_close_btn_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    if (!day_lock_is_locked()) {
        day_lock_ui_hide();
    }
}

static void help_dialog_close_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    if (s_help_dialog && lv_obj_is_valid(s_help_dialog)) {
        lv_obj_delete(s_help_dialog);
    }
    s_help_dialog = NULL;
}

static void help_dialog_open_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    if (s_help_dialog && lv_obj_is_valid(s_help_dialog)) return;

    /* ۱. پرده تاریک‌کننده پس‌زمینه (Dark Dimmer Backdrop) تمام‌صفحه */
    s_help_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_help_dialog, 480, 320);
    lv_obj_set_pos(s_help_dialog, 0, 0);
    lv_obj_set_style_bg_color(s_help_dialog, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(s_help_dialog, 200, 0); /* تاریکی ۷۸٪ برای محو هدر و کی‌پد پشت */
    lv_obj_set_style_border_width(s_help_dialog, 0, 0);
    lv_obj_set_style_pad_all(s_help_dialog, 0, 0);
    lv_obj_clear_flag(s_help_dialog, LV_OBJ_FLAG_SCROLLABLE);

    /* ۲. کارت سفید دیالوگ راهنما - ابعاد دقیق و بدون برش متن با فاصله متوازن از عنوان */
    lv_obj_t *card = lv_obj_create(s_help_dialog);
    lv_obj_set_size(card, 440, 246);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(card);
    lv_label_set_text(t, ui_lang_str(STR_DL_HELP_TITLE));
    lv_obj_set_style_text_font(t, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_base_dir(t, ui_lang_base_dir(), 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);

    /* محتوای متنی - فاصله ارگونومیک ۱۲ پیکسلی از عنوان و عدم برش کاسه حروف */
    lv_obj_t *msg = lv_label_create(card);
    lv_obj_set_size(msg, 404, 116);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 38);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_label_set_text(msg, ui_lang_str(STR_DL_HELP_MSG));
    lv_obj_set_style_text_font(msg, ui_lang_font_16(), 0);
    lv_obj_set_style_text_line_space(msg, 3, 0);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x334155), 0);
    lv_obj_set_style_base_dir(msg, ui_lang_base_dir(), 0);

    /* وب‌سایت رسمی - کاملاً در وسط و با فاصله متوازن زیر متن */
    lv_obj_t *site = lv_label_create(card);
    lv_label_set_text(site, ui_lang_str(STR_DL_HELP_SITE));
    lv_obj_set_style_text_font(site, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(site, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_base_dir(site, ui_lang_base_dir(), 0);
    lv_obj_align(site, LV_ALIGN_TOP_MID, 0, 160);

    lv_obj_t *btn_close = lv_btn_create(card);
    lv_obj_set_size(btn_close, 140, 36);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, help_dialog_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_c = lv_label_create(btn_close);
    lv_label_set_text(lbl_c, ui_lang_str(STR_HELP_GOT_IT));
    lv_obj_set_style_text_font(lbl_c, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_c, lv_color_white(), 0);
    lv_obj_center(lbl_c);
}

static void keypad_click_cb(lv_event_t *e)
{
    uint32_t rem_sec = 0;
    if (day_lock_is_brute_locked(&rem_sec)) {
        char freeze_msg[96];
        if (ui_lang_get() == UI_LANG_FA) {
            if (rem_sec >= 60) {
                snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است\n(%u دقیقه صبر کنید)", (unsigned)((rem_sec + 59) / 60));
            } else {
                snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است\n(%u ثانیه صبر کنید)", (unsigned)rem_sec);
            }
        } else if (ui_lang_get() == UI_LANG_TR) {
            snprintf(freeze_msg, sizeof(freeze_msg), "Giris engellendi\n(%u dk bekleyin)", (unsigned)((rem_sec + 59) / 60));
        } else {
            snprintf(freeze_msg, sizeof(freeze_msg), "Input locked\n(wait %u min)", (unsigned)((rem_sec + 59) / 60));
        }
        if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
            lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
            lv_label_set_text(s_err_banner, freeze_msg);
            lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        }
        if (!s_lockout_timer) {
            s_lockout_timer = lv_timer_create(lockout_timer_cb, 1000, NULL);
        }
        buzzer_beep_error();
        return;
    }

    lv_obj_t *btnm = (lv_obj_t *)lv_event_get_target(e);
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(btnm);
    const char *txt = lv_buttonmatrix_get_button_text(btnm, btn_id);
    if (!txt) return;

    if (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "OK") == 0) {
        if (s_pin_len > 0) {
            int grace_idx = -1;
            for (int gi = 0; gi < EMERGENCY_GRACE_COUNT; gi++) {
                if (strcmp(s_entered_pin, EMERGENCY_GRACE_PINS[gi]) == 0) {
                    grace_idx = gi;
                    break;
                }
            }

            if (strcmp(s_entered_pin, DEFAULT_ADMIN_PIN) == 0) {
                day_lock_clear_after_unlock();
                buzzer_beep_confirm();
                day_lock_ui_hide();
                day_lock_ui_show_5timers_dialog();
                return;
            } else if (day_lock_validate_pin(s_entered_pin)) {
                day_lock_ui_hide();
                if (grace_idx >= 0) {
                    panel_nav_toast_success((ui_lang_get() == UI_LANG_FA) ? "کد اضطراری تایید شد: ۱۰ روز به اعتبار دستگاه افزوده شد" :
                                            ((ui_lang_get() == UI_LANG_TR) ? "Acil kod onaylandi: 10 gun eklendi" : "Emergency code verified: 10 days added"));
                }
            } else {
                bool was_used_grace = (grace_idx >= 0 && day_lock_is_grace_indexed_used((uint8_t)grace_idx));
                s_pin_len = 0;
                s_entered_pin[0] = '\0';
                update_pin_display();
                if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
                    lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
                    lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
                    if (was_used_grace) {
                        lv_label_set_text(s_err_banner, ui_lang_str(STR_DL_GRACE_USED));
                        panel_nav_toast_warning(ui_lang_str(STR_DL_GRACE_USED));
                    } else {
                        uint32_t b_rem = 0;
                        if (day_lock_is_brute_locked(&b_rem)) {
                            char lk_msg[96];
                            if (ui_lang_get() == UI_LANG_FA) {
                                snprintf(lk_msg, sizeof(lk_msg), "ورود مسدود شد\n(۱۰ دقیقه صبر کنید)");
                            } else if (ui_lang_get() == UI_LANG_TR) {
                                snprintf(lk_msg, sizeof(lk_msg), "Giris engellendi\n(10 dakika bekleyin)");
                            } else {
                                snprintf(lk_msg, sizeof(lk_msg), "Entry locked\n(Wait 10 minutes)");
                            }
                            lv_label_set_text(s_err_banner, lk_msg);
                            if (!s_lockout_timer) {
                                s_lockout_timer = lv_timer_create(lockout_timer_cb, 1000, NULL);
                            }
                        } else {
                            uint8_t rem_att = day_lock_get_remaining_attempts();
                            char err_msg[96];
                            if (ui_lang_get() == UI_LANG_FA) {
                                snprintf(err_msg, sizeof(err_msg), "رمز اشتباه است\n(%u فرصت دیگر)", (unsigned)rem_att);
                            } else if (ui_lang_get() == UI_LANG_TR) {
                                snprintf(err_msg, sizeof(err_msg), "Hatali PIN\n(%u deneme kaldi)", (unsigned)rem_att);
                            } else {
                                snprintf(err_msg, sizeof(err_msg), "Invalid PIN\n(%u attempts left)", (unsigned)rem_att);
                            }
                            lv_label_set_text(s_err_banner, err_msg);
                        }
                    }
                }
                if (s_chal_lbl && lv_obj_is_valid(s_chal_lbl)) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%04lu", (unsigned long)day_lock_get_active_challenge());
                    lv_label_set_text(s_chal_lbl, buf);
                }
            }
        }
    } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "DEL") == 0) {
        if (s_pin_len > 0) {
            s_pin_len--;
            s_entered_pin[s_pin_len] = '\0';
            update_pin_display();
            buzzer_beep_click();
        }
    } else if (txt[0] >= '0' && txt[0] <= '9') {
        if (s_pin_len < 8) {
            s_entered_pin[s_pin_len++] = txt[0];
            s_entered_pin[s_pin_len] = '\0';
            update_pin_display();
            if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
                lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
            }
            buzzer_beep_click();
        }
    }
}

void day_lock_ui_show(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }

    if (s_modal_mask && lv_obj_is_valid(s_modal_mask)) {
        /* به‌روزرسانی کد چالش و وضعیت در صورت باز بودن */
        if (s_chal_lbl && lv_obj_is_valid(s_chal_lbl)) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%05lu", (unsigned long)day_lock_get_active_challenge());
            lv_label_set_text(s_chal_lbl, buf);
        }
        lvgl_unlock();
        return;
    }

    s_pin_len = 0;
    s_entered_pin[0] = '\0';
    s_nav_btn_idx = 0;

    /* ۱. کفی تمام‌صفحه لایه بالا (Apple HIG Slate Neutral #F8FAFC) */
    s_modal_mask = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_modal_mask, 0, 0);
    lv_obj_set_size(s_modal_mask, 480, 320);
    lv_obj_set_style_bg_color(s_modal_mask, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(s_modal_mask, 245, 0);
    lv_obj_set_style_border_width(s_modal_mask, 0, 0);
    lv_obj_set_style_pad_all(s_modal_mask, 0, 0);
    lv_obj_clear_flag(s_modal_mask, LV_OBJ_FLAG_SCROLLABLE);

    /* ۲. کارت شناور سفید خالص (Apple Pure White Card) */
    s_card = lv_obj_create(s_modal_mask);
    lv_obj_set_pos(s_card, 10, 10);
    lv_obj_set_size(s_card, 460, 300);
    lv_obj_set_style_bg_color(s_card, lv_color_white(), 0);
    lv_obj_set_style_radius(s_card, 14, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(s_card, 1, 0);
    lv_obj_set_style_shadow_width(s_card, 0, 0); /* Zero-lag mandate */
    lv_obj_set_style_pad_all(s_card, 8, 0);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);

    /* ۳. نوار هدر بالا */
    DayLockState_t st = day_lock_get_state();
    bool is_tamper = (st == LOCK_STAGE_TAMPER);

    lv_obj_t *title = lv_label_create(s_card);
    if (is_tamper) {
        lv_label_set_text(title, ui_lang_str(STR_DL_TAMPER_ALERT));
        lv_obj_set_style_text_color(title, lv_color_hex(0xEF4444), 0);
    } else {
        uint8_t cur_stg = day_lock_get_active_stage();
        if (cur_stg >= 1 && cur_stg <= DAY_LOCK_MAX_STAGES) {
            char title_buf[64];
            if (ui_lang_get() == UI_LANG_FA) {
                static const char *const stg_names_fa[] = {"", "اول", "دوم", "سوم", "چهارم", "پنجم"};
                const char *stg_str = (cur_stg <= 5) ? stg_names_fa[cur_stg] : "اول";
                snprintf(title_buf, sizeof(title_buf), "قفل پایان مهلت قسط %s آستیاج", stg_str);
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(title_buf, sizeof(title_buf), "ASTIAJ %u. Taksit Kilit Ekrani", (unsigned)cur_stg);
            } else {
                snprintf(title_buf, sizeof(title_buf), "ASTIAJ Stage %u Installment Lock", (unsigned)cur_stg);
            }
            lv_label_set_text(title, title_buf);
        } else {
            lv_label_set_text(title, ui_lang_str(STR_DL_LOCKED_TITLE));
        }
        lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);
    }
    lv_obj_set_style_text_font(title, ui_lang_font_20(), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 12, 1);

    /* کلید راهنما (آیکون کتاب) در گوشه هدر - ۲ پیکسل به راست منتقل شد */
    lv_obj_t *btn_help = lv_btn_create(s_card);
    lv_obj_set_pos(btn_help, 8, 2);
    lv_obj_set_size(btn_help, 32, 28);
    lv_obj_set_style_bg_color(btn_help, lv_color_hex(0xEFF6FF), 0);
    lv_obj_set_style_border_color(btn_help, lv_color_hex(0xBFDBFE), 0);
    lv_obj_set_style_border_width(btn_help, 1, 0);
    lv_obj_set_style_radius(btn_help, 6, 0);
    lv_obj_set_style_shadow_width(btn_help, 0, 0);
    lv_obj_add_event_cb(btn_help, help_dialog_open_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_q = lv_label_create(btn_help);
    lv_label_set_text(lbl_q, LV_SYMBOL_FILE);
    lv_obj_set_style_text_color(lbl_q, lv_color_hex(0x007AFF), 0);
    lv_obj_center(lbl_q);

    /* کلید بستن و بازگشت (Return / Close) */
    lv_obj_t *btn_close_card = lv_btn_create(s_card);
    lv_obj_set_pos(btn_close_card, 410, 4);
    lv_obj_set_size(btn_close_card, 34, 28);
    lv_obj_set_style_bg_color(btn_close_card, lv_color_hex(0xFEE2E2), 0);
    lv_obj_set_style_border_color(btn_close_card, lv_color_hex(0xFECACA), 0);
    lv_obj_set_style_border_width(btn_close_card, 1, 0);
    lv_obj_set_style_radius(btn_close_card, 6, 0);
    lv_obj_set_style_shadow_width(btn_close_card, 0, 0);
    lv_obj_add_event_cb(btn_close_card, day_lock_close_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cls = lv_label_create(btn_close_card);
    lv_label_set_text(lbl_cls, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(lbl_cls, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_cls, lv_color_hex(0xDC2626), 0);
    lv_obj_center(lbl_cls);

    if (is_tamper || (st == LOCK_STAGE_HARD_LOCK)) {
        lv_obj_add_flag(btn_close_card, LV_OBJ_FLAG_HIDDEN);
    }

    /* ۴. ستون سمت چپ: کارت چالش، کادر رمز، کلید وای‌فای، بنر خطا و پاورقی */
    /* ۴.۱ کارت چالش (Ice Blue Pill Badge) */
    lv_obj_t *chal_badge = lv_obj_create(s_card);
    lv_obj_set_pos(chal_badge, 8, 38);
    lv_obj_set_size(chal_badge, 180, 60);
    lv_obj_set_style_bg_color(chal_badge, lv_color_hex(0xEFF6FF), 0);
    lv_obj_set_style_border_color(chal_badge, lv_color_hex(0xBFDBFE), 0);
    lv_obj_set_style_border_width(chal_badge, 1, 0);
    lv_obj_set_style_radius(chal_badge, 10, 0);
    lv_obj_set_style_shadow_width(chal_badge, 0, 0);
    lv_obj_set_style_pad_all(chal_badge, 2, 0);
    lv_obj_clear_flag(chal_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_c_title = lv_label_create(chal_badge);
    lv_label_set_text(lbl_c_title, ui_lang_str(STR_DL_CHALLENGE));
    lv_obj_set_style_text_font(lbl_c_title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_c_title, lv_color_hex(0x1E40AF), 0);
    lv_obj_set_style_base_dir(lbl_c_title, ui_lang_base_dir(), 0);
    lv_obj_align(lbl_c_title, LV_ALIGN_TOP_MID, 0, 2);

    s_chal_lbl = lv_label_create(chal_badge);
    char buf[16];
    snprintf(buf, sizeof(buf), "%05lu", (unsigned long)day_lock_get_active_challenge());
    lv_label_set_text(s_chal_lbl, buf);
    lv_obj_set_style_text_font(s_chal_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_chal_lbl, lv_color_hex(0x007AFF), 0);
    lv_obj_align(s_chal_lbl, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* ۴.۲ کادر نمایش گلوله‌ای رمز + کلید چشم */
    lv_obj_t *pin_box = lv_obj_create(s_card);
    lv_obj_set_pos(pin_box, 8, 104);
    lv_obj_set_size(pin_box, 138, 38);
    lv_obj_set_style_bg_color(pin_box, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_border_color(pin_box, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(pin_box, 1, 0);
    lv_obj_set_style_radius(pin_box, 8, 0);
    lv_obj_set_style_shadow_width(pin_box, 0, 0);
    lv_obj_set_style_pad_all(pin_box, 0, 0);
    lv_obj_clear_flag(pin_box, LV_OBJ_FLAG_SCROLLABLE);

    s_pin_label = lv_label_create(pin_box);
    lv_obj_set_style_base_dir(s_pin_label, ui_lang_base_dir(), 0);
    lv_obj_center(s_pin_label);

    lv_obj_t *btn_eye = lv_btn_create(s_card);
    lv_obj_set_pos(btn_eye, 150, 104);
    lv_obj_set_size(btn_eye, 38, 38);
    lv_obj_set_style_bg_color(btn_eye, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_color(btn_eye, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(btn_eye, 1, 0);
    lv_obj_set_style_radius(btn_eye, 8, 0);
    lv_obj_set_style_shadow_width(btn_eye, 0, 0);
    lv_obj_add_event_cb(btn_eye, eye_btn_cb, LV_EVENT_CLICKED, NULL);

    s_btn_eye_lbl = lv_label_create(btn_eye);
    lv_label_set_text(s_btn_eye_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_font(s_btn_eye_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_btn_eye_lbl, lv_color_hex(0x0284C7), 0);
    lv_obj_center(s_btn_eye_lbl);

    s_pin_visible = false;
    update_pin_display();

    /* ۴.۳ کلید تنظیم خودکار ساعت با اینترنت (Apple HIG Sky Blue) */
    lv_obj_t *btn_wifi_sync = lv_btn_create(s_card);
    lv_obj_set_pos(btn_wifi_sync, 8, 146);
    lv_obj_set_size(btn_wifi_sync, 180, 36);
    lv_obj_set_style_bg_color(btn_wifi_sync, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_border_color(btn_wifi_sync, lv_color_hex(0x0369A1), 0);
    lv_obj_set_style_border_width(btn_wifi_sync, 1, 0);
    lv_obj_set_style_radius(btn_wifi_sync, 8, 0);
    lv_obj_set_style_shadow_width(btn_wifi_sync, 0, 0);
    lv_obj_add_event_cb(btn_wifi_sync, wifi_sync_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_wifi = lv_label_create(btn_wifi_sync);
    lv_label_set_text(lbl_wifi, (ui_lang_get() == UI_LANG_FA) ? "تنظیم ساعت با اینترنت" :
                                 ((ui_lang_get() == UI_LANG_TR) ? "Internette Saati Ayarla" : "Sync Time via Internet"));
    lv_obj_set_style_text_font(lbl_wifi, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_wifi, lv_color_white(), 0);
    lv_obj_set_style_base_dir(lbl_wifi, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_wifi);

    /* ۴.۴ بنر خطا / وضعیت */
    s_err_banner = lv_label_create(s_card);
    lv_obj_set_pos(s_err_banner, 8, 184);
    lv_obj_set_size(s_err_banner, 180, 46);
    lv_label_set_long_mode(s_err_banner, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_err_banner, "");
    lv_obj_set_style_text_font(s_err_banner, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
    lv_obj_set_style_text_align(s_err_banner, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(s_err_banner, ui_lang_base_dir(), 0);
    lv_obj_set_style_pad_all(s_err_banner, 0, 0);
    lv_obj_set_style_text_line_space(s_err_banner, 2, 0);
    lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);

    /* ۴.۵ راهنمای تماس پاورقی */
    lv_obj_t *footer_lbl = lv_label_create(s_card);
    lv_obj_set_pos(footer_lbl, 8, 236);
    lv_obj_set_size(footer_lbl, 180, 48);
    lv_label_set_long_mode(footer_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(footer_lbl, ui_lang_str(STR_DL_SUPPORT));
    lv_obj_set_style_text_font(footer_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(footer_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_align(footer_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(footer_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_pad_all(footer_lbl, 0, 0);
    lv_obj_set_style_text_line_space(footer_lbl, 2, 0);

    /* ۵. ستون سمت راست: کی‌پد ماتریسی لمسی ۴x۳ (Apple HIG Tactile Matrix) */
    static const char *const kmap[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
    };
    static const lv_buttonmatrix_ctrl_t kctrl[] = {
        1, 1, 1,
        1, 1, 1,
        1, 1, 1,
        1, 1, 1
    };

    s_btnm = lv_buttonmatrix_create(s_card);
    lv_buttonmatrix_set_map(s_btnm, kmap);
    lv_buttonmatrix_set_ctrl_map(s_btnm, kctrl);
    lv_obj_set_pos(s_btnm, 196, 38);
    lv_obj_set_size(s_btnm, 250, 246);
    lv_obj_set_style_bg_opa(s_btnm, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_btnm, 0, 0);
    lv_obj_set_style_pad_all(s_btnm, 2, 0);
    lv_obj_set_style_pad_row(s_btnm, 6, 0);
    lv_obj_set_style_pad_column(s_btnm, 6, 0);

    /* استایل کلیدها: کاشی‌های روشن #F1F5F9 روی کارت سفید */
    lv_obj_set_style_bg_color(s_btnm, lv_color_hex(0xF1F5F9), LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_btnm, lv_color_hex(0xCBD5E1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_btnm, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_btnm, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_btnm, lv_color_hex(0x0F172A), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_btnm, &lv_font_montserrat_24, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(s_btnm, 0, LV_PART_ITEMS);

    lv_obj_add_event_cb(s_btnm, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(s_btnm, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(s_btnm, keypad_click_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* بررسی وضعیت قفل بروت‌فورس در هنگام نمایش پنجره */
    uint32_t init_b_rem = 0;
    if (day_lock_is_brute_locked(&init_b_rem)) {
        char init_lk_msg[96];
        if (ui_lang_get() == UI_LANG_FA) {
            if (init_b_rem >= 60) {
                snprintf(init_lk_msg, sizeof(init_lk_msg), "ورود مسدود است\n(%u دقیقه صبر کنید)", (unsigned)((init_b_rem + 59) / 60));
            } else {
                snprintf(init_lk_msg, sizeof(init_lk_msg), "ورود مسدود است\n(%u ثانیه صبر کنید)", (unsigned)init_b_rem);
            }
        } else if (ui_lang_get() == UI_LANG_TR) {
            snprintf(init_lk_msg, sizeof(init_lk_msg), "Giris engellendi\n(%u dk bekleyin)", (unsigned)((init_b_rem + 59) / 60));
        } else {
            snprintf(init_lk_msg, sizeof(init_lk_msg), "Input locked\n(wait %u min)", (unsigned)((init_b_rem + 59) / 60));
        }
        lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
        lv_label_set_text(s_err_banner, init_lk_msg);
        lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        if (!s_lockout_timer) {
            s_lockout_timer = lv_timer_create(lockout_timer_cb, 1000, NULL);
        }
    }

    lvgl_unlock();
    ESP_LOGI(TAG, "Apple HIG Pure White Day Lock Modal displayed");
}

void day_lock_ui_hide(void)
{
    if (day_lock_is_locked()) {
        ESP_LOGW(TAG, "day_lock_ui_hide blocked: machine is locked");
        return;
    }

    if (!lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }

    if (!s_modal_mask || !lv_obj_is_valid(s_modal_mask)) {
        lvgl_unlock();
        return;
    }

    if (s_help_dialog && lv_obj_is_valid(s_help_dialog)) {
        lv_obj_delete(s_help_dialog);
        s_help_dialog = NULL;
    }

    if (s_sync_status_timer) {
        lv_timer_del(s_sync_status_timer);
        s_sync_status_timer = NULL;
    }

    if (s_lockout_timer) {
        lv_timer_del(s_lockout_timer);
        s_lockout_timer = NULL;
    }

    lv_obj_delete(s_modal_mask);
    s_modal_mask = NULL;
    s_card = NULL;
    s_pin_label = NULL;
    s_chal_lbl = NULL;
    s_err_banner = NULL;
    s_btnm = NULL;
    s_pin_len = 0;
    s_entered_pin[0] = '\0';

    lvgl_unlock();
    ESP_LOGI(TAG, "Day Lock Modal dismissed");
}

bool day_lock_ui_is_visible(void)
{
    return (s_modal_mask != NULL && lv_obj_is_valid(s_modal_mask));
}

void day_lock_ui_handle_physical_key(uint8_t row, uint8_t key)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;

    if (!day_lock_ui_is_visible() || !s_btnm || !lv_obj_is_valid(s_btnm)) {
        lvgl_unlock();
        return;
    }

    /* کلیدهای ردیف ۲ پنل ناوبری آستیاج:
       SAVE   (ردیف ۲، کلید ۱): تایید نهایی و ثبت کد
       CANCEL (ردیف ۲، کلید ۲): خروج و بستن دیالوگ
       MODE   (ردیف ۲، کلید ۳): تغییر کلید فوکوس شده روی نام‌پد (۰ تا ۱۱)
       START  (ردیف ۲، کلید ۴): تایید / فشردن کلید انتخاب شده
       STOP   (ردیف ۲، کلید ۵): پاک‌کردن (DEL)
    */
    if (row == 2 && key == 3) {
        /* کلید MODE = چرخش فوکوس روی کلیدهای نام‌پد */
        s_nav_btn_idx = (s_nav_btn_idx + 1) % 12;
        lv_buttonmatrix_set_selected_button(s_btnm, s_nav_btn_idx);
        buzzer_beep_click();
    } else if (row == 2 && key == 2) {
        /* کلید CANCEL = خروج و بستن دیالوگ فقط در صورت عدم قفل سخت */
        if (!day_lock_is_locked()) {
            day_lock_ui_hide();
            buzzer_beep_click();
        } else {
            buzzer_beep_error();
        }
    } else if (row == 2 && key == 1) {
        /* کلید SAVE = ارسال مستقیم تایید پین */
        lv_buttonmatrix_set_selected_button(s_btnm, 11); /* دکمه تایید OK */
        lv_obj_send_event(s_btnm, LV_EVENT_VALUE_CHANGED, NULL);
        buzzer_beep_confirm();
    } else if (row == 2 && key == 4) {
        /* کلید START = ارسال رویداد فشردن کلید فوکوس شده */
        lv_obj_send_event(s_btnm, LV_EVENT_VALUE_CHANGED, NULL);
        buzzer_beep_click();
    } else if (row == 2 && key == 5) {
        /* کلید STOP = پاک کردن آخرین رقم */
        if (s_pin_len > 0) {
            s_pin_len--;
            s_entered_pin[s_pin_len] = '\0';
            update_pin_display();
            buzzer_beep_click();
        }
    } else if (row == 1 && key >= 1 && key <= 5) {
        /* کلیدهای F1 تا F5 (ردیف ۱، کلیدهای ۱ تا ۵) مستقیماً ارقام ۱ تا ۵ */
        if (s_pin_len < 8) {
            s_entered_pin[s_pin_len++] = (char)('0' + key);
            s_entered_pin[s_pin_len] = '\0';
            update_pin_display();
            if (s_err_banner && lv_obj_is_valid(s_err_banner)) {
                lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
            }
            buzzer_beep_click();
        }
    }

    lvgl_unlock();
}

/* =========================================================================
 * ساب‌سیستم دیالوگ تنظیم ۵ تایمر اقساط آستیاج (Apple HIG & Jalali Calendar)
 * ========================================================================= */

static lv_obj_t *s_5t_mask = NULL;
static int16_t s_5t_days[5] = {30, 30, 30, 30, 30};
static lv_obj_t *s_5t_rows[5] = {NULL};
static lv_obj_t *s_5t_name_lbls[5] = {NULL};
static lv_obj_t *s_5t_val_lbls[5] = {NULL};
static lv_obj_t *s_5t_date_lbls[5] = {NULL};
static lv_obj_t *s_5t_date_boxes[5] = {NULL};
static bool s_5t_settled[5] = {false};
static char s_5t_settled_dates[5][24] = {{0}};

static void update_5timers_display(void)
{
    ui_lang_t lang = ui_lang_get();
    uint8_t act_stg = day_lock_get_active_stage();
    int16_t cur_remain = day_lock_get_remain_days();
    if (cur_remain < 0) cur_remain = 0;

    int16_t running_offset = 0;
    bool found_active = false;

    for (int i = 0; i < 5; i++) {
        if (s_5t_settled[i]) {
            /* قسط تایید و کامل شده: کم‌رنگ، کادر سبز ملایم، نمایش تاریخ تایید */
            if (s_5t_rows[i] && lv_obj_is_valid(s_5t_rows[i])) {
                lv_obj_set_style_opa(s_5t_rows[i], 160, 0);
            }
            if (s_5t_name_lbls[i] && lv_obj_is_valid(s_5t_name_lbls[i])) {
                lv_obj_set_style_text_color(s_5t_name_lbls[i], lv_color_hex(0x64748B), 0);
            }
            if (s_5t_val_lbls[i] && lv_obj_is_valid(s_5t_val_lbls[i])) {
                lv_label_set_text(s_5t_val_lbls[i], (lang == UI_LANG_FA) ? "تایید" : ((lang == UI_LANG_TR) ? "Onay" : "Paid"));
                lv_obj_set_style_text_color(s_5t_val_lbls[i], lv_color_hex(0x16A34A), 0);
            }
            if (s_5t_date_boxes[i] && lv_obj_is_valid(s_5t_date_boxes[i])) {
                lv_obj_set_style_bg_color(s_5t_date_boxes[i], lv_color_hex(0xF0FDF4), 0);
                lv_obj_set_style_border_color(s_5t_date_boxes[i], lv_color_hex(0xBBF7D0), 0);
            }
            if (s_5t_date_lbls[i] && lv_obj_is_valid(s_5t_date_lbls[i])) {
                lv_obj_set_style_base_dir(s_5t_date_lbls[i], LV_BASE_DIR_LTR, 0);
                if (s_5t_settled_dates[i][0] == '\0') {
                    day_lock_get_stage_settled_date(i + 1, s_5t_settled_dates[i], sizeof(s_5t_settled_dates[i]));
                }
                char full_str[160];
                const char *d_str = s_5t_settled_dates[i][0] ? s_5t_settled_dates[i] : "---";
                snprintf(full_str, sizeof(full_str), (lang == UI_LANG_FA) ? "تایید شد در %.20s" :
                                                     ((lang == UI_LANG_TR) ? "%.20s onaylandi" : "Approved %.20s"),
                         d_str);
                lv_label_set_text(s_5t_date_lbls[i], full_str);
                lv_obj_set_style_text_color(s_5t_date_lbls[i], lv_color_hex(0x15803D), 0);
            }
        } else {
            /* قسط در جریان یا فعال: شفافیت کامل و نمایش موعد بعدی */
            if (s_5t_rows[i] && lv_obj_is_valid(s_5t_rows[i])) {
                lv_obj_set_style_opa(s_5t_rows[i], 255, 0);
            }
            if (s_5t_name_lbls[i] && lv_obj_is_valid(s_5t_name_lbls[i])) {
                lv_obj_set_style_text_color(s_5t_name_lbls[i], lv_color_hex(0x1E293B), 0);
            }
            if (s_5t_val_lbls[i] && lv_obj_is_valid(s_5t_val_lbls[i])) {
                char d_str[16];
                snprintf(d_str, sizeof(d_str), (lang == UI_LANG_FA) ? "%d روز" : ((lang == UI_LANG_TR) ? "%d gun" : "%d days"), s_5t_days[i]);
                lv_label_set_text(s_5t_val_lbls[i], d_str);
                lv_obj_set_style_text_color(s_5t_val_lbls[i], lv_color_hex(0x0F172A), 0);
            }
            if (s_5t_date_boxes[i] && lv_obj_is_valid(s_5t_date_boxes[i])) {
                lv_obj_set_style_bg_color(s_5t_date_boxes[i], lv_color_hex(0xEFF6FF), 0);
                lv_obj_set_style_border_color(s_5t_date_boxes[i], lv_color_hex(0xBFDBFE), 0);
            }
            if (s_5t_date_lbls[i] && lv_obj_is_valid(s_5t_date_lbls[i])) {
                lv_obj_set_style_base_dir(s_5t_date_lbls[i], LV_BASE_DIR_LTR, 0);
                if (s_5t_days[i] <= 0) {
                    lv_label_set_text(s_5t_date_lbls[i], (lang == UI_LANG_FA) ? "غیرفعال (خاموش)" : ((lang == UI_LANG_TR) ? "Devre Disi (Kapali)" : "Disabled (Off)"));
                    lv_obj_set_style_text_color(s_5t_date_lbls[i], lv_color_hex(0x94A3B8), 0);
                } else {
                    if (!found_active) {
                        found_active = true;
                        if (act_stg == (uint8_t)(i + 1)) {
                            running_offset = cur_remain;
                        } else {
                            running_offset = s_5t_days[i];
                        }
                    } else {
                        running_offset += s_5t_days[i];
                    }
                    char j_date[32];
                    day_lock_get_future_date_str(running_offset, j_date, sizeof(j_date), lang == UI_LANG_FA);
                    char full_str[96];
                    snprintf(full_str, sizeof(full_str), (lang == UI_LANG_FA) ? "موعد: %s" : ((lang == UI_LANG_TR) ? "Vade: %s" : "Due: %s"), j_date);
                    lv_label_set_text(s_5t_date_lbls[i], full_str);
                    lv_obj_set_style_text_color(s_5t_date_lbls[i], lv_color_hex(0x0284C7), 0);
                }
            }
        }
    }
}

void day_lock_ui_refresh_5timers(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    update_5timers_display();
    lvgl_unlock();
}

static void timer_stepper_minus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < 5) {
        if (s_5t_settled[idx]) {
            s_5t_settled[idx] = false;
            s_5t_days[idx] = 0;
        } else {
            int step = (code == LV_EVENT_LONG_PRESSED_REPEAT) ? 5 : 1;
            if (s_5t_days[idx] >= step) {
                s_5t_days[idx] -= step;
            } else {
                s_5t_days[idx] = 0;
            }
        }
        update_5timers_display();
        buzzer_beep_click();
    }
}

static void timer_stepper_plus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < 5) {
        if (s_5t_settled[idx]) {
            s_5t_settled[idx] = false;
            if (s_5t_days[idx] <= 0) s_5t_days[idx] = 30;
        } else {
            int step = (code == LV_EVENT_LONG_PRESSED_REPEAT) ? 5 : 1;
            if (s_5t_days[idx] + step <= 365) {
                s_5t_days[idx] += step;
            } else {
                s_5t_days[idx] = 365;
            }
        }
        update_5timers_display();
        buzzer_beep_click();
    }
}

static void val_box_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < 5) {
        if (s_5t_settled[idx]) {
            s_5t_settled[idx] = false;
            s_5t_days[idx] = 30;
        } else {
            /* چرخش میان دوره‌های استاندارد اقساط آستیاج (۰، ۳۰، ۶۰، ۹۰، ۱۲۰) */
            if (s_5t_days[idx] == 0) s_5t_days[idx] = 30;
            else if (s_5t_days[idx] == 30) s_5t_days[idx] = 60;
            else if (s_5t_days[idx] == 60) s_5t_days[idx] = 90;
            else if (s_5t_days[idx] == 90) s_5t_days[idx] = 120;
            else if (s_5t_days[idx] >= 120) s_5t_days[idx] = 0;
            else s_5t_days[idx] = ((s_5t_days[idx] / 10) + 1) * 10;
            if (s_5t_days[idx] > 365) s_5t_days[idx] = 0;
        }
        update_5timers_display();
        buzzer_beep_click();
    }
}

static void btn_5timers_save_cb(lv_event_t *e)
{
    (void)e;
    for (int i = 0; i < 5; i++) {
        if (!s_5t_settled[i]) {
            day_lock_clear_stage_settled(i + 1);
        }
    }
    day_lock_arm_5stages(s_5t_days);
    buzzer_beep_confirm();
    uint8_t armed_count = 0;
    for (int i = 0; i < 5; i++) {
        if (s_5t_days[i] > 0 || s_5t_settled[i]) armed_count++;
    }
    if (armed_count == 0) armed_count = 1;
    char tst_buf[80];
    if (ui_lang_get() == UI_LANG_FA) {
        snprintf(tst_buf, sizeof(tst_buf), "تنظیمات %u قسط با موفقیت ذخیره و فعال شد", (unsigned)armed_count);
    } else if (ui_lang_get() == UI_LANG_TR) {
        snprintf(tst_buf, sizeof(tst_buf), "%u taksit basariyla kaydedildi", (unsigned)armed_count);
    } else {
        snprintf(tst_buf, sizeof(tst_buf), "%u-stage installments saved successfully", (unsigned)armed_count);
    }
    panel_nav_toast_success(tst_buf);
    day_lock_ui_hide_5timers_dialog();
}

static void btn_5timers_clear_cb(lv_event_t *e)
{
    (void)e;
    for (int i = 0; i < 5; i++) {
        day_lock_clear_stage_settled(i + 1);
    }
    day_lock_clear_after_unlock();
    buzzer_beep_confirm();
    panel_nav_toast_info((ui_lang_get() == UI_LANG_FA) ? "کلیه تایمرها غیرفعال و دستگاه آزاد شد" :
                         ((ui_lang_get() == UI_LANG_TR) ? "Tum zamanlayicilar devre disi" : "All timers disabled, machine unlocked"));
    day_lock_ui_hide_5timers_dialog();
}

static void btn_5timers_cancel_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    day_lock_ui_hide_5timers_dialog();
}

void day_lock_ui_hide_5timers_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (!s_5t_mask || !lv_obj_is_valid(s_5t_mask)) {
        lvgl_unlock();
        return;
    }

    lv_obj_delete(s_5t_mask);
    s_5t_mask = NULL;
    for (int i = 0; i < 5; i++) {
        s_5t_rows[i] = NULL;
        s_5t_name_lbls[i] = NULL;
        s_5t_val_lbls[i] = NULL;
        s_5t_date_lbls[i] = NULL;
        s_5t_date_boxes[i] = NULL;
    }
    lvgl_unlock();
}

void day_lock_ui_show_5timers_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_5t_mask && lv_obj_is_valid(s_5t_mask)) {
        lvgl_unlock();
        return;
    }

    for (int i = 0; i < 5; i++) {
        int16_t d = day_lock_get_stage_days(i + 1);
        s_5t_days[i] = (d > 0) ? d : 0;
        s_5t_settled[i] = day_lock_is_stage_settled(i + 1);
        if (s_5t_settled[i]) {
            day_lock_get_stage_settled_date(i + 1, s_5t_settled_dates[i], sizeof(s_5t_settled_dates[i]));
        } else {
            s_5t_settled_dates[i][0] = '\0';
        }
    }

    s_5t_mask = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_5t_mask, 0, 0);
    lv_obj_set_size(s_5t_mask, 480, 320);
    lv_obj_set_style_bg_color(s_5t_mask, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(s_5t_mask, 235, 0);
    lv_obj_set_style_border_width(s_5t_mask, 0, 0);
    lv_obj_set_style_pad_all(s_5t_mask, 0, 0);
    lv_obj_clear_flag(s_5t_mask, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_5t_mask);
    lv_obj_set_pos(card, 10, 10);
    lv_obj_set_size(card, 460, 300);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    ui_lang_t cur_lang = ui_lang_get();
    const lv_font_t *font_title = ui_lang_font_16();
    lv_base_dir_t base_dir = ui_lang_base_dir();

    /* تیتر هدر */
    lv_obj_t *title = lv_label_create(card);
    const char *title_text = (cur_lang == UI_LANG_FA) ? "تنظیمات زنجیره ۵ تایمر اقساط آستیاج" :
                             ((cur_lang == UI_LANG_TR) ? "ASTIAJ 5 Taksit Zinciri Ayarlari" :
                                                         "ASTIAJ 5-Stage Installment Settings");
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, font_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_base_dir(title, base_dir, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* دکمه بستن X */
    lv_obj_t *btn_cls = lv_btn_create(card);
    lv_obj_set_pos(btn_cls, 412, 2);
    lv_obj_set_size(btn_cls, 34, 28);
    lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0xFEE2E2), 0);
    lv_obj_set_style_border_color(btn_cls, lv_color_hex(0xFECACA), 0);
    lv_obj_set_style_border_width(btn_cls, 1, 0);
    lv_obj_set_style_radius(btn_cls, 6, 0);
    lv_obj_set_style_shadow_width(btn_cls, 0, 0);
    lv_obj_add_event_cb(btn_cls, btn_5timers_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_x = lv_label_create(btn_cls);
    lv_label_set_text(lbl_x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xDC2626), 0);
    lv_obj_center(lbl_x);

    static const char *const s_stage_names_fa[5] = {
        "قسط اول (T1)",
        "قسط دوم (T2)",
        "قسط سوم (T3)",
        "قسط چهارم (T4)",
        "قسط پنجم (T5)"
    };
    static const char *const s_stage_names_en[5] = {
        "Stage 1 (T1)",
        "Stage 2 (T2)",
        "Stage 3 (T3)",
        "Stage 4 (T4)",
        "Stage 5 (T5)"
    };
    static const char *const s_stage_names_tr[5] = {
        "Taksit 1 (T1)",
        "Taksit 2 (T2)",
        "Taksit 3 (T3)",
        "Taksit 4 (T4)",
        "Taksit 5 (T5)"
    };
    const char *const *s_stage_names = (cur_lang == UI_LANG_FA) ? s_stage_names_fa :
                                       ((cur_lang == UI_LANG_TR) ? s_stage_names_tr : s_stage_names_en);

    for (int i = 0; i < 5; i++) {
        int row_y = 36 + (i * 42);
        lv_obj_t *row = lv_obj_create(card);
        s_5t_rows[i] = row;
        lv_obj_set_pos(row, 8, row_y);
        lv_obj_set_size(row, 444, 38);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xF8FAFC), 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0xE2E8F0), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_shadow_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        /* ۱. نام قسط (سمت راست - با فاصله و پدینگ مناسب جهت عدم چسبندگی به کادر) */
        lv_obj_t *lbl_name = lv_label_create(row);
        s_5t_name_lbls[i] = lbl_name;
        lv_obj_set_pos(lbl_name, 304, 7);
        lv_obj_set_size(lbl_name, 120, 24);
        lv_label_set_text(lbl_name, s_stage_names[i]);
        lv_obj_set_style_text_font(lbl_name, font_title, 0);
        lv_obj_set_style_text_color(lbl_name, lv_color_hex(0x1E293B), 0);
        lv_obj_set_style_base_dir(lbl_name, base_dir, 0);
        lv_obj_set_style_pad_right(lbl_name, 8, 0);

        /* ۲. دکمه منفی */
        lv_obj_t *btn_m = lv_btn_create(row);
        lv_obj_set_pos(btn_m, 172, 4);
        lv_obj_set_size(btn_m, 30, 28);
        lv_obj_set_style_bg_color(btn_m, lv_color_hex(0x334155), 0);
        lv_obj_set_style_border_color(btn_m, lv_color_hex(0x475569), 0);
        lv_obj_set_style_border_width(btn_m, 1, 0);
        lv_obj_set_style_radius(btn_m, 6, 0);
        lv_obj_set_style_shadow_width(btn_m, 0, 0);
        lv_obj_set_ext_click_area(btn_m, 8);
        lv_obj_add_flag(btn_m, LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_clear_flag(btn_m, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn_m, timer_stepper_minus_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn_m, timer_stepper_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)i);

        lv_obj_t *lbl_m = lv_label_create(btn_m);
        lv_label_set_text(lbl_m, LV_SYMBOL_MINUS);
        lv_obj_set_style_text_font(lbl_m, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_m, lv_color_white(), 0);
        lv_obj_center(lbl_m);
        lv_obj_clear_flag(lbl_m, LV_OBJ_FLAG_CLICKABLE);

        /* ۳. کادر نمایش روزها */
        lv_obj_t *val_box = lv_obj_create(row);
        lv_obj_set_pos(val_box, 206, 4);
        lv_obj_set_size(val_box, 56, 28);
        lv_obj_set_style_bg_color(val_box, lv_color_white(), 0);
        lv_obj_set_style_border_color(val_box, lv_color_hex(0xCBD5E1), 0);
        lv_obj_set_style_border_width(val_box, 1, 0);
        lv_obj_set_style_radius(val_box, 6, 0);
        lv_obj_set_style_shadow_width(val_box, 0, 0);
        lv_obj_set_style_pad_all(val_box, 0, 0);
        lv_obj_add_flag(val_box, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(val_box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(val_box, val_box_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        s_5t_val_lbls[i] = lv_label_create(val_box);
        lv_obj_set_style_text_font(s_5t_val_lbls[i], font_title, 0);
        lv_obj_set_style_text_color(s_5t_val_lbls[i], lv_color_hex(0x0F172A), 0);
        lv_obj_set_style_base_dir(s_5t_val_lbls[i], base_dir, 0);
        lv_obj_center(s_5t_val_lbls[i]);
        lv_obj_clear_flag(s_5t_val_lbls[i], LV_OBJ_FLAG_CLICKABLE);

        /* ۴. دکمه مثبت */
        lv_obj_t *btn_p = lv_btn_create(row);
        lv_obj_set_pos(btn_p, 266, 4);
        lv_obj_set_size(btn_p, 30, 28);
        lv_obj_set_style_bg_color(btn_p, lv_color_hex(0x0284C7), 0);
        lv_obj_set_style_border_color(btn_p, lv_color_hex(0x0369A1), 0);
        lv_obj_set_style_border_width(btn_p, 1, 0);
        lv_obj_set_style_radius(btn_p, 6, 0);
        lv_obj_set_style_shadow_width(btn_p, 0, 0);
        lv_obj_set_ext_click_area(btn_p, 8);
        lv_obj_add_flag(btn_p, LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_clear_flag(btn_p, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn_p, timer_stepper_plus_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn_p, timer_stepper_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)i);

        lv_obj_t *lbl_p = lv_label_create(btn_p);
        lv_label_set_text(lbl_p, LV_SYMBOL_PLUS);
        lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_p, lv_color_white(), 0);
        lv_obj_center(lbl_p);

        /* ۵. بج تاریخ جلالی / شمسی موعد قفل یا تایید */
        lv_obj_t *date_box = lv_obj_create(row);
        s_5t_date_boxes[i] = date_box;
        lv_obj_set_pos(date_box, 5, 4);
        lv_obj_set_size(date_box, 164, 28);
        lv_obj_set_style_bg_color(date_box, lv_color_hex(0xEFF6FF), 0);
        lv_obj_set_style_border_color(date_box, lv_color_hex(0xBFDBFE), 0);
        lv_obj_set_style_border_width(date_box, 1, 0);
        lv_obj_set_style_radius(date_box, 6, 0);
        lv_obj_set_style_shadow_width(date_box, 0, 0);
        lv_obj_set_style_pad_all(date_box, 0, 0);
        lv_obj_clear_flag(date_box, LV_OBJ_FLAG_SCROLLABLE);

        s_5t_date_lbls[i] = lv_label_create(date_box);
        lv_obj_set_style_text_font(s_5t_date_lbls[i], ui_lang_font_14(), 0);
        lv_obj_set_style_base_dir(s_5t_date_lbls[i], LV_BASE_DIR_LTR, 0);
        lv_obj_center(s_5t_date_lbls[i]);
    }

    update_5timers_display();

    /* دکمه‌های فوتر پایین */
    /* دکمه ۱: ذخیره و فعال‌سازی (سبز زمردی) */
    lv_obj_t *btn_save = lv_btn_create(card);
    lv_obj_set_pos(btn_save, 16, 252);
    lv_obj_set_size(btn_save, 140, 36);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x16A34A), 0);
    lv_obj_set_style_border_color(btn_save, lv_color_hex(0x15803D), 0);
    lv_obj_set_style_border_width(btn_save, 1, 0);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_set_style_shadow_width(btn_save, 0, 0);
    lv_obj_add_event_cb(btn_save, btn_5timers_save_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_save = lv_label_create(btn_save);
    const char *save_text = (cur_lang == UI_LANG_FA) ? "ذخیره و فعال‌سازی" :
                            ((cur_lang == UI_LANG_TR) ? "Kaydet ve Etkinlestir" : "Save & Activate");
    lv_label_set_text(lbl_save, save_text);
    lv_obj_set_style_text_font(lbl_save, font_title, 0);
    lv_obj_set_style_text_color(lbl_save, lv_color_white(), 0);
    lv_obj_set_style_base_dir(lbl_save, base_dir, 0);
    lv_obj_center(lbl_save);

    /* دکمه ۲: غیرفعال‌سازی همه / آزاد (قرمز) */
    lv_obj_t *btn_free = lv_btn_create(card);
    lv_obj_set_pos(btn_free, 164, 252);
    lv_obj_set_size(btn_free, 160, 36);
    lv_obj_set_style_bg_color(btn_free, lv_color_hex(0xDC2626), 0);
    lv_obj_set_style_border_color(btn_free, lv_color_hex(0xB91C1C), 0);
    lv_obj_set_style_border_width(btn_free, 1, 0);
    lv_obj_set_style_radius(btn_free, 8, 0);
    lv_obj_set_style_shadow_width(btn_free, 0, 0);
    lv_obj_add_event_cb(btn_free, btn_5timers_clear_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_free = lv_label_create(btn_free);
    const char *free_text = (cur_lang == UI_LANG_FA) ? "غیرفعال‌سازی همه (آزاد)" :
                            ((cur_lang == UI_LANG_TR) ? "Hepsini Devre Disi Birak" : "Disable All (Unlock)");
    lv_label_set_text(lbl_free, free_text);
    lv_obj_set_style_text_font(lbl_free, font_title, 0);
    lv_obj_set_style_text_color(lbl_free, lv_color_white(), 0);
    lv_obj_set_style_base_dir(lbl_free, base_dir, 0);
    lv_obj_center(lbl_free);

    /* دکمه ۳: انصراف (خاکستری) */
    lv_obj_t *btn_cancel = lv_btn_create(card);
    lv_obj_set_pos(btn_cancel, 332, 252);
    lv_obj_set_size(btn_cancel, 104, 36);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(btn_cancel, 1, 0);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_add_event_cb(btn_cancel, btn_5timers_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    const char *cancel_text = (cur_lang == UI_LANG_FA) ? "انصراف" :
                              ((cur_lang == UI_LANG_TR) ? "Iptal" : "Cancel");
    lv_label_set_text(lbl_cancel, cancel_text);
    lv_obj_set_style_text_font(lbl_cancel, font_title, 0);
    lv_obj_set_style_text_color(lbl_cancel, lv_color_white(), 0);
    lv_obj_set_style_base_dir(lbl_cancel, base_dir, 0);
    lv_obj_center(lbl_cancel);

    lvgl_unlock();
}
