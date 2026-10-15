#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "lvgl8_compat.h"
#include "project_config.h"
#include "buzzer.h"
#include "extra_modes.h"
#include "fault_log.h"
#include "ds3231.h"
#include "storage.h"
#include "wifi.h"
#include "max6675.h"
#include "lvgl_port.h"
#include "ui_resume.h"
#include "ota_update.h"
#include "ui_lang.h"
#include "panel_nav.h"
#include "day_lock_core.h"
#include "day_lock_ui.h"
#include "boot_diag.h"
#include "cloud_client.h"
#include "analog_inputs.h"
#include "machine_advisor.h"
#include "warranty.h"
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

extern jalali_time_t jt;
extern int32_t ds_year, ds_month, ds_day;
extern bool plc_is_ready(void);
extern bool plc_is_dispense(void);
extern void nvs_request_lazy_flush(void);


bool g_boot_f5_service_mode = false;
bool s_pin_for_boot_wifi = false;
bool s_pin_for_warranty_offline = false;
bool s_pin_for_warranty_edit = false;
char s_warr_temp_name[64] = {0};
char s_warr_temp_city[64] = {0};  /* Persian UTF-8: هر حرف ۲ بایت — ۳۲ بایت فقط ۱۵ حرف = ناکافی */
char s_warr_temp_phone[16] = {0};
static lv_obj_t *s_warr_status_box = NULL;
static lv_obj_t *s_lbl_warr_title = NULL;
static lv_obj_t *s_lbl_warr_name = NULL;
static lv_obj_t *s_lbl_warr_city = NULL;
static lv_obj_t *s_lbl_warr_phone = NULL;
static lv_obj_t *s_lbl_warr_submit = NULL;
static lv_obj_t *s_lbl_warr_status = NULL;
bool s_warr_opened_from_main = false;
static lv_obj_t *s_ta_warr_name = NULL;
static lv_obj_t *s_ta_warr_city = NULL;
static lv_obj_t *s_ta_warr_phone = NULL;
static lv_obj_t *s_warr_details_backdrop = NULL;
static lv_obj_t *s_val_fixed_cd = NULL;
static lv_obj_t *s_val_fixed_rst = NULL;
static lv_obj_t *s_val_fixed_deb = NULL;
static lv_obj_t *s_val_fixed_cyl = NULL;
static void action_open_warranty_details(lv_event_t *e);

objects_t objects;
lv_obj_t *tick_value_change_obj;

static int compare_semantic_versions(const char *v1, const char *v2);

static void splash_bar_anim_cb(void *var, int32_t val)
{
    lv_bar_set_value((lv_obj_t *)var, val, LV_ANIM_OFF);
}

void create_screen_splash_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.splash_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0a0c10), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* تصویر تمام‌صفحه اختصاصی اسپلش (بستنی آرتیسان + لوگوی آستیاژ) */
    lv_obj_t *splash_bg = lv_image_create(obj);
    lv_image_set_src(splash_bg, &img_splash_screen_img);
    lv_obj_set_pos(splash_bg, 0, 0);
    lv_obj_set_size(splash_bg, 480, 320);
    lv_obj_clear_flag(splash_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* کانتینر برندینگ و عناوین روی پس‌زمینه (x=24, y=144, w=240 - 20px پایین‌تر) */
    lv_obj_t *brand_cnt = lv_obj_create(obj);
    lv_obj_remove_style_all(brand_cnt);
    lv_obj_set_pos(brand_cnt, 24, 144);
    lv_obj_set_size(brand_cnt, 240, 80);
    lv_obj_set_layout(brand_cnt, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(brand_cnt, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(brand_cnt, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(brand_cnt, 4, 0);
    lv_obj_clear_flag(brand_cnt, LV_OBJ_FLAG_SCROLLABLE);

    /* مدل دستگاه: ASTIAJ CM-700 */
    lv_obj_t *model_lbl = lv_label_create(brand_cnt);
    lv_label_set_text(model_lbl, FIRMWARE_MODEL_STR);
    lv_obj_set_style_text_font(model_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(model_lbl, lv_color_hex(0x38bdf8), 0); // Ice Blue

    /* زیرعنوان صنعتی */
    lv_obj_t *sub_lbl = lv_label_create(brand_cnt);
    lv_label_set_text(sub_lbl, FIRMWARE_SYSTEM_DESC);
    lv_obj_set_style_text_font(sub_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x94a3b8), 0); // Slate 400

    /* نسخه فریمور */
    objects.splash_ver_lbl = lv_label_create(brand_cnt);
    lv_label_set_text(objects.splash_ver_lbl, "FIRMWARE " FIRMWARE_VERSION_STR);
    lv_obj_set_style_text_font(objects.splash_ver_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(objects.splash_ver_lbl, lv_color_hex(0xe2e8f0), 0);

    /* برچسب وضعیت لودینگ بالای نوار پیشرفت در ستون چپ (y=242, w=180) */
    objects.splash_status_lbl = lv_label_create(obj);
    lv_label_set_text(objects.splash_status_lbl, "در حال راه‌اندازی...");
    lv_obj_set_style_text_font(objects.splash_status_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(objects.splash_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_base_dir(objects.splash_status_lbl, LV_BASE_DIR_RTL, 0);
    lv_obj_set_pos(objects.splash_status_lbl, 24, 242);
    lv_obj_set_width(objects.splash_status_lbl, 180);
    lv_label_set_long_mode(objects.splash_status_lbl, LV_LABEL_LONG_WRAP);

    /* نوار لودر انیمیشنی اپل در ستون چپ — بدون تداخل با تصویر بستنی (x=24, y=272, w=180, h=6) */
    lv_obj_t *bar = lv_bar_create(obj);
    objects.splash_loader_bar = bar;
    lv_obj_set_pos(bar, 24, 272);
    lv_obj_set_size(bar, 180, 6);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x1e293b), LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);

    /* رنگ اندیکاتور نوار پیشرفت (طیف نیلی به فیروزه‌ای اپل) */
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0284c7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0x38bdf8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    /* انیمیشن پر شدن آرام و پیوسته طی ۲.۲ ثانیه */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, 2200);
    lv_anim_set_exec_cb(&a, splash_bar_anim_cb);
    lv_anim_start(&a);

    tick_screen_splash_screen();
}

void tick_screen_splash_screen() {
    extern int adc_get_io1(void);
    extern int adc_get_io2(void);
    int v1 = adc_get_io1();
    int v2 = adc_get_io2();
    if ((v1 >= 350 && v1 <= 1850) || (v2 >= 350 && v2 <= 1850)) {
        /* کلید F5 در حین اسپلش فشرده شد -> ابطال تایمر اسپلش و ورود بلادرنگ به سرویس و گیت رمز مدیر */
        ui_cancel_splash_timer();
        g_boot_f5_service_mode = true;
        s_pin_for_boot_wifi = true;
        loadScreen(SCREEN_ID_BOOT_WIFI);
        show_manager_pin_dialog();
    }
}

static void style_mode_btn(lv_obj_t *obj)
{
    /* حالت عادی (خاموش) - زمینه سفید با حاشیه خاکستری ملایم */
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x9e9e9e), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* حالت فعال (روشن / Checked) - استایل تمیز اپل با حاشیه سبز زمردی ۲ پیکسلی و پس‌زمینه ملایم */
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xf0fdf4), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x16a34a), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_CHECKED);

    /* حالت فشرده (Pressed) */
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xf1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x64748b), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_PRESSED);

    /* حالت روشن + فشرده (Checked + Pressed) */
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xdcfce7), LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x15803d), LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);

    /* حذف کادرهای ناخواسته فوکوس تم پیش‌فرض */
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *make_status_box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, 24);
    lv_obj_set_style_radius(box, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(box);
    lv_obj_set_style_text_font(lbl, &ui_font_yekan_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl, "خاموش");
    lv_obj_center(lbl);

    ui_style_status_label(box, false);
    return box;
}

__attribute__((unused)) static lv_obj_t *make_mode_label(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, const char *txt)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, w);
    lv_obj_set_height(obj, 27);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(obj, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, txt);
    lv_obj_set_style_text_font(obj, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    /* بدون فونت Bold واقعی: outline برای ضخیم‌تر شدن متن */
    lv_obj_set_style_text_outline_stroke_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_outline_stroke_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_outline_stroke_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    return obj;
}

/* آیکون‌ها ۱۰۰x۱۰۰ هستند؛ کمی کوچک‌تر از قاب تا از کادر بیرون نزنند */
__attribute__((unused)) static lv_obj_t *make_mode_btn(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                               const void *off_img, const void *on_img,
                               lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_clear_flag(btn, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_GESTURE_BUBBLE));
    lv_obj_add_flag(btn, LV_OBJ_FLAG_PRESS_LOCK);
    style_mode_btn(btn);
    if (cb) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    }

    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, off_img);
    lv_image_set_scale(img, 256); /* 100% native resolution for crisp clarity */
    lv_obj_center(img);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    (void)on_img;
    return btn;
}

/* ---------- رهگیری زمان فشرده‌سازی ۶ کلید اصلی جهت جلوگیری قطعی از استارت تصادفی هنگام انصراف از لانگ‌پرس ---------- */
static uint32_t s_mode_btn_press_tick[6] = {0};

static void mode_btn_press_tracker_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    intptr_t id = (intptr_t)lv_event_get_user_data(e);
    if (id < 0 || id >= 6) return;

    if (code == LV_EVENT_PRESSED) {
        s_mode_btn_press_tick[id] = lv_tick_get();
    }
}

uint32_t ui_get_mode_btn_hold_duration_ms(uint8_t btn_id)
{
    if (btn_id >= 6 || s_mode_btn_press_tick[btn_id] == 0) return 0;
    return lv_tick_elaps(s_mode_btn_press_tick[btn_id]);
}

void ui_reset_mode_btn_hold_duration(uint8_t btn_id)
{
    if (btn_id < 6) {
        s_mode_btn_press_tick[btn_id] = 0;
    }
}



static lv_obj_t *s_flag_btns[16];
static uint8_t s_flag_btn_count = 0;

/* =========================================================================
 * پنجره انتخاب زبان سیستم (Apple HIG Language Selector Modal)
 * ========================================================================= */
static lv_obj_t *s_lang_modal_backdrop = NULL;

void hide_language_selector_modal(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    if (s_lang_modal_backdrop && lv_obj_is_valid(s_lang_modal_backdrop)) {
        lv_obj_delete(s_lang_modal_backdrop);
    }
    s_lang_modal_backdrop = NULL;
    lvgl_unlock();
}

static void on_lang_card_clicked(lv_event_t *e)
{
    ui_lang_t target_lang = (ui_lang_t)(intptr_t)lv_event_get_user_data(e);
    buzzer_beep_confirm();
    ui_lang_set(target_lang);
    hide_language_selector_modal();
    ui_mark_settings_modified();
    ui_set_save_btn_state(true);
    if (target_lang == UI_LANG_FA) {
        panel_nav_toast_success("زبان به فارسی تنظیم شد");
    } else if (target_lang == UI_LANG_TR) {
        panel_nav_toast_success("Dil Turkce olarak ayarlandi");
    } else {
        panel_nav_toast_success("Language set to English");
    }
}

static void on_lang_modal_backdrop_click(lv_event_t *e)
{
    if (lv_event_get_target(e) == s_lang_modal_backdrop) {
        buzzer_beep_click();
        hide_language_selector_modal();
    }
}

static void on_lang_modal_close_btn(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    hide_language_selector_modal();
}

void show_language_selector_modal(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    hide_language_selector_modal();

    ui_lang_t cur_lang = ui_lang_get();

    /* 1. Backdrop */
    s_lang_modal_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_lang_modal_backdrop, 0, 0);
    lv_obj_set_size(s_lang_modal_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_lang_modal_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_lang_modal_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_lang_modal_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_lang_modal_backdrop, 0, 0);
    lv_obj_clear_flag(s_lang_modal_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_lang_modal_backdrop, on_lang_modal_backdrop_click, LV_EVENT_CLICKED, NULL);

    /* 2. Main Dialog Card */
    lv_obj_t *dialog = lv_obj_create(s_lang_modal_backdrop);
    lv_obj_set_pos(dialog, 50, 48);
    lv_obj_set_size(dialog, 380, 224);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(dialog, 14, 0);
    lv_obj_set_style_clip_corner(dialog, true, 0);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(dialog, 0, 0);
    lv_obj_set_style_pad_all(dialog, 0, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    /* 3. Header Bar */
    lv_obj_t *hdr = lv_obj_create(dialog);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, 380, 40);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_radius(hdr, 14, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t_lbl = lv_label_create(hdr);
    const char *hdr_txt = (cur_lang == UI_LANG_FA) ? "انتخاب زبان سیستم" : ((cur_lang == UI_LANG_TR) ? "Dil Secimi" : "System Language Selection");
    lv_label_set_text(t_lbl, hdr_txt);
    lv_obj_set_style_text_font(t_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(t_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(t_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(t_lbl);

    /* Close button [✕] */
    lv_obj_t *c_btn = lv_btn_create(hdr);
    lv_obj_set_pos(c_btn, 345, 5);
    lv_obj_set_size(c_btn, 30, 30);
    lv_obj_set_style_radius(c_btn, 8, 0);
    lv_obj_set_style_bg_color(c_btn, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(c_btn, 0, 0);
    lv_obj_set_ext_click_area(c_btn, 6);
    lv_obj_add_event_cb(c_btn, on_lang_modal_close_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *x_lbl = lv_label_create(c_btn);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(x_lbl);

    /* 4. 3 Language Cards: Persian, English, Turkish */
    static const struct {
        ui_lang_t lang;
        const char *badge;
        const char *name;
        uint32_t active_border;
        uint32_t active_bg;
        uint32_t badge_bg;
        uint32_t badge_fg;
    } s_langs[3] = {
        {UI_LANG_FA, "FA", "فارسی",  0x22c55e, 0xf0fdf4, 0xdcfce7, 0x16a34a},
        {UI_LANG_EN, "EN", "English", 0x0284c7, 0xf0f9ff, 0xe0f2fe, 0x0284c7},
        {UI_LANG_TR, "TR", "Turkce",  0xef4444, 0xfef2f2, 0xfee2e2, 0xdc2626}
    };

    for (int i = 0; i < 3; i++) {
        bool is_active = (cur_lang == s_langs[i].lang);
        lv_obj_t *card = lv_obj_create(dialog);
        lv_obj_set_pos(card, 16 + (i * 120), 48);
        lv_obj_set_size(card, 108, 162);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(is_active ? s_langs[i].active_bg : 0xf8fafc), 0);
        lv_obj_set_style_border_width(card, is_active ? 2 : 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(is_active ? s_langs[i].active_border : 0xcbd5e1), 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, on_lang_card_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)s_langs[i].lang);

        /* 1. Circular Flag Image (44x44) centered on top */
        const lv_image_dsc_t *flag_dsc = (s_langs[i].lang == UI_LANG_FA) ? &img_flag_fa :
                                         ((s_langs[i].lang == UI_LANG_TR) ? &img_flag_tr : &img_flag_en);
        lv_obj_t *flag_img = lv_image_create(card);
        lv_image_set_src(flag_img, flag_dsc);
        lv_obj_set_pos(flag_img, 32, 10);
        lv_obj_clear_flag(flag_img, LV_OBJ_FLAG_CLICKABLE);

        /* Main Language Name */
        lv_obj_t *nlbl = lv_label_create(card);
        lv_label_set_text(nlbl, s_langs[i].name);
        lv_obj_set_style_text_font(nlbl, (s_langs[i].lang == UI_LANG_FA) ? &ui_font_yekan_20 : &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(nlbl, lv_color_hex(0x0f172a), 0);
        lv_obj_set_style_base_dir(nlbl, (s_langs[i].lang == UI_LANG_FA) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        lv_obj_align(nlbl, LV_ALIGN_TOP_MID, 0, 58);

        /* Subtitle dynamically translated based on currently active system language */
        const char *sub_txt = "";
        const lv_font_t *sub_font = &lv_font_montserrat_14;
        lv_base_dir_t sub_dir = LV_BASE_DIR_LTR;
        if (cur_lang == UI_LANG_FA) {
            sub_font = &ui_font_yekan_16;
            sub_dir = LV_BASE_DIR_RTL;
            if (s_langs[i].lang == UI_LANG_FA) sub_txt = "فارسی";
            else if (s_langs[i].lang == UI_LANG_EN) sub_txt = "انگلیسی";
            else sub_txt = "ترکی";
        } else if (cur_lang == UI_LANG_TR) {
            sub_font = &lv_font_montserrat_14;
            sub_dir = LV_BASE_DIR_LTR;
            if (s_langs[i].lang == UI_LANG_FA) sub_txt = "Farsca";
            else if (s_langs[i].lang == UI_LANG_EN) sub_txt = "Ingilizce";
            else sub_txt = "Turkce";
        } else { /* UI_LANG_EN */
            sub_font = &lv_font_montserrat_14;
            sub_dir = LV_BASE_DIR_LTR;
            if (s_langs[i].lang == UI_LANG_FA) sub_txt = "Persian";
            else if (s_langs[i].lang == UI_LANG_EN) sub_txt = "English";
            else sub_txt = "Turkish";
        }

        lv_obj_t *slbl = lv_label_create(card);
        lv_label_set_text(slbl, sub_txt);
        lv_obj_set_style_text_font(slbl, sub_font, 0);
        lv_obj_set_style_text_color(slbl, lv_color_hex(0x64748b), 0);
        lv_obj_set_style_base_dir(slbl, sub_dir, 0);
        lv_obj_align(slbl, LV_ALIGN_TOP_MID, 0, 84);

        /* Status Badge at bottom */
        if (is_active) {
            lv_obj_t *ok_badge = lv_obj_create(card);
            lv_obj_set_pos(ok_badge, 22, 120);
            lv_obj_set_size(ok_badge, 64, 24);
            lv_obj_set_style_radius(ok_badge, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(ok_badge, lv_color_hex(s_langs[i].active_border), 0);
            lv_obj_set_style_border_width(ok_badge, 0, 0);
            lv_obj_set_style_pad_all(ok_badge, 0, 0);
            lv_obj_clear_flag(ok_badge, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *ok_lbl = lv_label_create(ok_badge);
            lv_label_set_text(ok_lbl, LV_SYMBOL_OK);
            lv_obj_set_style_text_font(ok_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
            lv_obj_center(ok_lbl);
        } else {
            lv_obj_t *sel_lbl = lv_label_create(card);
            const char *sel_txt = (cur_lang == UI_LANG_FA) ? "انتخاب" : ((cur_lang == UI_LANG_TR) ? "Sec" : "Select");
            lv_label_set_text(sel_lbl, sel_txt);
            lv_obj_set_style_text_font(sel_lbl, ui_lang_font_16(), 0);
            lv_obj_set_style_text_color(sel_lbl, lv_color_hex(0x94a3b8), 0);
            lv_obj_align(sel_lbl, LV_ALIGN_BOTTOM_MID, 0, -14);
        }
    }

    lvgl_unlock();
}

static void ui_update_flag_button(lv_obj_t *btn)
{
    if (!btn || !lv_obj_is_valid(btn)) return;
    lv_obj_clean(btn);

    ui_lang_t lang = ui_lang_get();
    lv_obj_set_size(btn, 52, 24);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_opa(btn, 245, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_ext_click_area(btn, 6);

    const lv_image_dsc_t *flag_dsc = &img_flag_fa_mini;
    uint32_t border_col = 0x22c55e;

    if (lang == UI_LANG_EN) {
        flag_dsc = &img_flag_en_mini;
        border_col = 0x0284c7;
    } else if (lang == UI_LANG_TR) {
        flag_dsc = &img_flag_tr_mini;
        border_col = 0xef4444;
    }

    lv_obj_set_style_border_color(btn, lv_color_hex(border_col), 0);

    /* 1. پرچم مینیاتوری گرد و باکیفیت بدون هیچ متن اضافه */
    lv_obj_t *flag_img = lv_image_create(btn);
    lv_image_set_src(flag_img, flag_dsc);
    lv_obj_set_pos(flag_img, 3, 2);
    lv_obj_clear_flag(flag_img, LV_OBJ_FLAG_CLICKABLE);

    /* 2. فلش کوچک انتخاب */
    lv_obj_t *arr = lv_label_create(btn);
    lv_label_set_text(arr, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_font(arr, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(arr, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(arr, 28, 5);
    lv_obj_clear_flag(arr, LV_OBJ_FLAG_CLICKABLE);
}

static lv_obj_t *create_circular_flag_button(lv_obj_t *parent, int32_t x, int32_t y)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 52, 24);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_clip_corner(btn, true, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x38bdf8), 0);
    lv_obj_add_event_cb(btn, action_lang_cycle, LV_EVENT_CLICKED, NULL);

    if (s_flag_btn_count < (uint8_t)(sizeof(s_flag_btns) / sizeof(s_flag_btns[0]))) {
        s_flag_btns[s_flag_btn_count++] = btn;
    }
    ui_update_flag_button(btn);
    return btn;
}

static lv_obj_t *s_volume_backdrop = NULL;
static lv_timer_t *s_volume_timer = NULL;

static void volume_timer_dismiss_cb(lv_timer_t *t)
{
    (void)t;
    ui_volume_popup_hide();
}

static void volume_backdrop_clicked_cb(lv_event_t *e)
{
    if (lv_event_get_target(e) == s_volume_backdrop) {
        ui_volume_popup_hide();
    }
}

static void on_volume_slider_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    if (val < 0) val = 0;
    if (val > 100) val = 100;

    if (val == 0) {
        buzzer_set_enabled(false);
        buzzer_set_volume(0);
    } else {
        buzzer_set_volume((uint8_t)val);
        if (!buzzer_is_enabled()) {
            buzzer_set_enabled(true);
        }
    }

    /* به‌روزرسانی درصد بالای اسلایدر */
    if (objects.volume_pct_lbl && lv_obj_is_valid(objects.volume_pct_lbl)) {
        lv_label_set_text_fmt(objects.volume_pct_lbl, "%ld%%", (long)val);
    }
    /* به‌روزرسانی آیکون پایین اسلایدر */
    if (objects.volume_icon_lbl && lv_obj_is_valid(objects.volume_icon_lbl)) {
        lv_label_set_text(objects.volume_icon_lbl, (val > 0) ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
    }
    /* به‌روزرسانی کلید اسپیکر در تولبار پایین */
    ui_update_toolbar_buttons();

    /* فیدبک صوتی زنده شبیه ویندوز: هنگام تغییر اسلایدر یا رها کردن آن، شدت صدای انتخاب‌شده پخش می‌شود */
    static uint32_t s_last_buzz_tick = 0;
    uint32_t now = lv_tick_get();
    if (code == LV_EVENT_RELEASED || (now - s_last_buzz_tick > 150)) {
        s_last_buzz_tick = now;
        if (val > 0 && buzzer_is_enabled()) {
            buzz(40);
        }
    }

    if (s_volume_timer) {
        lv_timer_reset(s_volume_timer);
    }
}

void ui_volume_popup_show(void)
{
    if (!s_volume_backdrop || !lv_obj_is_valid(s_volume_backdrop)) return;
    uint8_t vol = buzzer_is_enabled() ? buzzer_get_volume() : 0;
    if (objects.volume_slider && lv_obj_is_valid(objects.volume_slider)) {
        lv_slider_set_value(objects.volume_slider, vol, LV_ANIM_OFF);
    }
    if (objects.volume_pct_lbl && lv_obj_is_valid(objects.volume_pct_lbl)) {
        lv_label_set_text_fmt(objects.volume_pct_lbl, "%d%%", (int)vol);
    }
    if (objects.volume_icon_lbl && lv_obj_is_valid(objects.volume_icon_lbl)) {
        lv_label_set_text(objects.volume_icon_lbl, (vol > 0) ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
    }
    lv_obj_clear_flag(s_volume_backdrop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_volume_backdrop);

    if (s_volume_timer == NULL) {
        s_volume_timer = lv_timer_create(volume_timer_dismiss_cb, 5000, NULL);
    } else {
        lv_timer_reset(s_volume_timer);
        lv_timer_resume(s_volume_timer);
    }
}

void ui_volume_popup_hide(void)
{
    if (s_volume_backdrop && lv_obj_is_valid(s_volume_backdrop)) {
        lv_obj_add_flag(s_volume_backdrop, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_volume_timer) {
        lv_timer_pause(s_volume_timer);
    }
}

void ui_volume_popup_toggle(void)
{
    if (!s_volume_backdrop || !lv_obj_is_valid(s_volume_backdrop)) return;
    if (lv_obj_has_flag(s_volume_backdrop, LV_OBJ_FLAG_HIDDEN)) {
        ui_volume_popup_show();
    } else {
        ui_volume_popup_hide();
    }
}

static void ui_create_volume_popup(void)
{
    lv_obj_t *top_layer = lv_layer_top();

    /* بک‌دراپ شفاف تمام‌صفحه جهت بستن پاپ‌آپ با لمس هر نقطه از صفحه (رفتار استاندارد ویندوز و مک) */
    s_volume_backdrop = lv_obj_create(top_layer);
    lv_obj_set_pos(s_volume_backdrop, 0, 0);
    lv_obj_set_size(s_volume_backdrop, 480, 320);
    lv_obj_set_style_bg_opa(s_volume_backdrop, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_volume_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_volume_backdrop, 0, 0);
    lv_obj_clear_flag(s_volume_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_volume_backdrop, volume_backdrop_clicked_cb, LV_EVENT_CLICKED, NULL);

    /* کپسول عمودی تنظیم صدا استایل iOS Control Center بالای کلید اسپیکر */
    lv_obj_t *card = lv_obj_create(s_volume_backdrop);
    objects.volume_popup_card = card;
    lv_obj_set_pos(card, 290, 100);
    lv_obj_set_size(card, 58, 162);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(card, 248, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* ۱. نمایش درصد بالای اسلایدر */
    lv_obj_t *pct_lbl = lv_label_create(card);
    objects.volume_pct_lbl = pct_lbl;
    lv_obj_set_pos(pct_lbl, 0, 6);
    lv_obj_set_size(pct_lbl, 58, 18);
    lv_obj_set_style_text_align(pct_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(pct_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pct_lbl, lv_color_hex(0x38bdf8), 0);
    lv_label_set_text_fmt(pct_lbl, "%d%%", (int)buzzer_get_volume());

    /* ۲. اسلایدر عمودی کپسولی اپل */
    lv_obj_t *slider = lv_slider_create(card);
    objects.volume_slider = slider;
    lv_obj_set_pos(slider, 15, 26);
    lv_obj_set_size(slider, 28, 104);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, buzzer_is_enabled() ? buzzer_get_volume() : 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(slider, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_MAIN);

    lv_obj_set_style_bg_color(slider, lv_color_hex(0x0284c7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, 255, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 14, LV_PART_INDICATOR);

    lv_obj_set_style_bg_opa(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, on_volume_slider_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(slider, on_volume_slider_event, LV_EVENT_RELEASED, NULL);

    /* ۳. آیکون وضعیت پایین اسلایدر */
    lv_obj_t *bot_icon = lv_label_create(card);
    objects.volume_icon_lbl = bot_icon;
    lv_obj_set_pos(bot_icon, 0, 136);
    lv_obj_set_size(bot_icon, 58, 20);
    lv_obj_set_style_text_align(bot_icon, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(bot_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(bot_icon, lv_color_hex(0xffffff), 0);
    lv_label_set_text(bot_icon, buzzer_is_enabled() ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);

    lv_obj_add_flag(s_volume_backdrop, LV_OBJ_FLAG_HIDDEN);
}

typedef struct {
    lv_obj_t *card;
    lv_obj_t *lbl_title;
    ui_str_key_t key;
    int32_t lbl_x;
    lv_obj_t *u_box;
    uint8_t param_id;
} stepper_card_entry_t;

static stepper_card_entry_t s_stepper_cards[40];
static uint8_t s_stepper_card_count = 0;

static ui_str_key_t stepper_param_to_key(uint8_t param_id) {
    switch (param_id) {
        case 1:  return STR_TMR_PADDLE;
        case 3:  return STR_TMR_COMP_RST;
        case 4:  return STR_TMR_EYE;
        case 5:  return STR_YEAR;
        case 6:  return STR_MONTH;
        case 7:  return STR_DAY;
        case 8:  return STR_HOUR;
        case 9:  return STR_MINUTE;
        case 10: return STR_MODBUS_DELAY;
        case 11: return STR_COMP_CYCLE;
        case 12: return STR_HOLD_DELAY;
        case 17: return STR_F5_HOLD_TIME;
        case 18: return STR_F5_PROD_TIME;
        case 20: return STR_F5_MAIN_COMP;
        case 21: return STR_F5_HOLD_COMP;
        case 22: return STR_TMR_DSP_RUN;
        case 23: return STR_TMR_FRT_HOLD;
        case 24: return STR_CNT_SCP_COMP;
        case 25: return STR_TMR_FRZ_TO;
        case 26: return STR_TMR_LEV_MAX;
        case 27: return STR_TEMP_T1_ON;
        case 28: return STR_TEMP_T1_OFF;
        case 29: return STR_T1_FALLBACK_ON;
        case 30: return STR_T1_FALLBACK_OFF;
        case 31: return STR_COMP_FALLBACK_ON;
        case 32: return STR_COMP_FALLBACK_OFF;
        default: return STR_COUNT;
    }
}

const char *get_ss_timeout_text(int32_t s)
{
    ui_lang_t lang = ui_lang_get();
    if (lang == UI_LANG_TR) {
        if (s == 0) return "Kapali";
        if (s == 900) return "15 dk";
        if (s == 1800) return "30 dk";
        if (s == 3600) return "1 saat";
        if (s == 7200) return "2 saat";
        return "Kapali";
    } else if (lang == UI_LANG_EN) {
        if (s == 0) return "Off";
        if (s == 900) return "15 min";
        if (s == 1800) return "30 min";
        if (s == 3600) return "1 Hour";
        if (s == 7200) return "2 Hours";
        return "Off";
    } else {
        if (s == 0) return "خاموش";
        if (s == 900) return "یک ربع";
        if (s == 1800) return "نیم ساعت";
        if (s == 3600) return "یک ساعت";
        if (s == 7200) return "دو ساعت";
        return "خاموش";
    }
}

void update_ss_timeout_ui(void)
{
    if (objects.lbl_ss_title && lv_obj_is_valid(objects.lbl_ss_title)) {
        lv_label_set_text(objects.lbl_ss_title, ui_lang_str(STR_STANDBY_PREFIX));
        lv_obj_set_style_text_font(objects.lbl_ss_title, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_ss_title, ui_lang_base_dir(), 0);
        lv_obj_align(objects.lbl_ss_title, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -4 : 4);
    }
    if (objects.lbl_ss_val && lv_obj_is_valid(objects.lbl_ss_val)) {
        lv_label_set_text(objects.lbl_ss_val, get_ss_timeout_text(screensaver_timeout_s));
        lv_obj_set_style_text_font(objects.lbl_ss_val, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_ss_val, ui_lang_base_dir(), 0);
        lv_obj_align(objects.lbl_ss_val, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *pill = lv_obj_get_parent(objects.lbl_ss_val);
        if (pill && lv_obj_is_valid(pill)) {
            lv_color_t clr = (screensaver_timeout_s > 0) ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b);
            lv_obj_set_style_bg_color(pill, clr, 0);
        }
    }
}

void update_pulley_en_ui(void)
{
    if (objects.lbl_pulley_title && lv_obj_is_valid(objects.lbl_pulley_title)) {
        lv_label_set_text(objects.lbl_pulley_title, ui_lang_str(STR_PULLEY_SENSOR));
        lv_obj_set_style_text_font(objects.lbl_pulley_title, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_pulley_title, ui_lang_base_dir(), 0);
        lv_obj_align(objects.lbl_pulley_title, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -4 : 4);
    }
    if (objects.lbl_pulley_val && lv_obj_is_valid(objects.lbl_pulley_val)) {
        lv_label_set_text(objects.lbl_pulley_val, s_pulley_en ? ui_lang_str(STR_PULLEY_ENABLED) : ui_lang_str(STR_PULLEY_DISABLED));
        lv_obj_set_style_text_font(objects.lbl_pulley_val, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_pulley_val, ui_lang_base_dir(), 0);
        lv_obj_align(objects.lbl_pulley_val, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *pill = lv_obj_get_parent(objects.lbl_pulley_val);
        if (pill && lv_obj_is_valid(pill)) {
            lv_color_t clr = s_pulley_en ? lv_color_hex(0x16a34a) : lv_color_hex(0x64748b);
            lv_obj_set_style_bg_color(pill, clr, 0);
        }
    }
}

static void format_lease_remain_str(char *buf, size_t buf_sz, uint8_t cur_stg, uint8_t tot_stg, ui_lang_t lang)
{
    if (!buf || buf_sz == 0) return;

    int32_t val = 0;
    DayLockTimeUnit_t unit = day_lock_get_remain_time_unit(&val);

    const char *unit_fa = (unit == DAY_LOCK_UNIT_DAYS) ? "روز" :
                          ((unit == DAY_LOCK_UNIT_HOURS) ? "ساعت" : "دقیقه");
    const char *unit_tr = (unit == DAY_LOCK_UNIT_DAYS) ? "gun" :
                          ((unit == DAY_LOCK_UNIT_HOURS) ? "saat" : "dakika");
    const char *unit_en = (unit == DAY_LOCK_UNIT_DAYS) ? "days" :
                          ((unit == DAY_LOCK_UNIT_HOURS) ? "hours" : "mins");

    if (cur_stg >= 1 && cur_stg <= DAY_LOCK_MAX_STAGES) {
        if (lang == UI_LANG_FA) {
            snprintf(buf, buf_sz, "قسط %u از %u: %ld %s", (unsigned)cur_stg, (unsigned)tot_stg, (long)val, unit_fa);
        } else if (lang == UI_LANG_TR) {
            snprintf(buf, buf_sz, "%u/%u Taksit: %ld %s", (unsigned)cur_stg, (unsigned)tot_stg, (long)val, unit_tr);
        } else {
            snprintf(buf, buf_sz, "Stage %u/%u: %ld %s", (unsigned)cur_stg, (unsigned)tot_stg, (long)val, unit_en);
        }
    } else {
        if (lang == UI_LANG_FA) {
            snprintf(buf, buf_sz, "%ld %s باقیمانده", (long)val, unit_fa);
        } else if (lang == UI_LANG_TR) {
            snprintf(buf, buf_sz, "%ld %s kaldi", (long)val, unit_tr);
        } else {
            snprintf(buf, buf_sz, "%ld %s left", (long)val, unit_en);
        }
    }
}


void ui_screens_refresh_language(void)
{
    /* ۱. به‌روزرسانی تمام دکمه‌های پرچم/زبان ثبت‌شده */
    for (uint8_t i = 0; i < s_flag_btn_count; i++) {
        ui_update_flag_button(s_flag_btns[i]);
    }

    /* ۲. به‌روزرسانی تمام کارت‌های استپر تنظیمات (F1, F2, F3, F5) */
    for (uint8_t i = 0; i < s_stepper_card_count; i++) {
        if (s_stepper_cards[i].lbl_title && lv_obj_is_valid(s_stepper_cards[i].lbl_title)) {
            if (s_stepper_cards[i].key < STR_COUNT) {
                lv_label_set_text(s_stepper_cards[i].lbl_title, ui_lang_str(s_stepper_cards[i].key));
            }
            lv_obj_set_style_text_font(s_stepper_cards[i].lbl_title, ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(s_stepper_cards[i].lbl_title, ui_lang_base_dir(), 0);
            if (ui_lang_get() == UI_LANG_FA) {
                int32_t cw = (s_stepper_cards[i].card && lv_obj_is_valid(s_stepper_cards[i].card)) ? lv_obj_get_width(s_stepper_cards[i].card) : 0;
                int32_t right_ofs = (cw <= 300 && cw >= 238) ? -2 : -6;
                lv_obj_align(s_stepper_cards[i].lbl_title, LV_ALIGN_RIGHT_MID, right_ofs, 0);
                lv_obj_set_style_text_align(s_stepper_cards[i].lbl_title, LV_TEXT_ALIGN_RIGHT, 0);
            } else {
                lv_obj_align(s_stepper_cards[i].lbl_title, LV_ALIGN_LEFT_MID, s_stepper_cards[i].lbl_x, 2);
                lv_obj_set_style_text_align(s_stepper_cards[i].lbl_title, LV_TEXT_ALIGN_LEFT, 0);
            }
        }
    }

    /* ۳. به‌روزرسانی تیترهای تب‌های تنظیمات با وسط‌چین‌سازی کامل */
    if (objects.obj8 && lv_obj_is_valid(objects.obj8)) {
        lv_label_set_text(objects.obj8, ui_lang_str(STR_TAB_F1_HDR));
        lv_obj_set_style_text_font(objects.obj8, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.obj8, ui_lang_base_dir(), 0);
        lv_obj_align(objects.obj8, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.obj10 && lv_obj_is_valid(objects.obj10)) {
        lv_label_set_text(objects.obj10, ui_lang_str(STR_TAB_F2_HDR));
        lv_obj_set_style_text_font(objects.obj10, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.obj10, ui_lang_base_dir(), 0);
        lv_obj_align(objects.obj10, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.obj12 && lv_obj_is_valid(objects.obj12)) {
        lv_label_set_text(objects.obj12, ui_lang_str(STR_TAB_F3_HDR));
        lv_obj_set_style_text_font(objects.obj12, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.obj12, ui_lang_base_dir(), 0);
        lv_obj_align(objects.obj12, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.obj14 && lv_obj_is_valid(objects.obj14)) {
        lv_label_set_text(objects.obj14, ui_lang_str(STR_TAB_F4_HDR));
        lv_obj_set_style_text_font(objects.obj14, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.obj14, ui_lang_base_dir(), 0);
        lv_obj_align(objects.obj14, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.obj17 && lv_obj_is_valid(objects.obj17)) {
        lv_label_set_text(objects.obj17, ui_lang_str(STR_TAB_F5_HDR));
        lv_obj_set_style_text_font(objects.obj17, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.obj17, ui_lang_base_dir(), 0);
        lv_obj_align(objects.obj17, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.hdr_f6_lbl && lv_obj_is_valid(objects.hdr_f6_lbl)) {
        lv_label_set_text(objects.hdr_f6_lbl, ui_lang_str(STR_TAB_F6_HDR));
        lv_obj_set_style_text_font(objects.hdr_f6_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.hdr_f6_lbl, ui_lang_base_dir(), 0);
        lv_obj_align(objects.hdr_f6_lbl, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.hdr_f7_lbl && lv_obj_is_valid(objects.hdr_f7_lbl)) {
        lv_label_set_text(objects.hdr_f7_lbl, ui_lang_str(STR_TAB_F7_HDR));
        lv_obj_set_style_text_font(objects.hdr_f7_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.hdr_f7_lbl, ui_lang_base_dir(), 0);
        lv_obj_align(objects.hdr_f7_lbl, LV_ALIGN_CENTER, 18, 0);
    }
    if (objects.lbl_locked_hdr && lv_obj_is_valid(objects.lbl_locked_hdr)) {
        lv_label_set_text(objects.lbl_locked_hdr, ui_lang_str(STR_LOCKED_TIMERS_HDR));
        lv_obj_set_style_text_font(objects.lbl_locked_hdr, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_locked_hdr, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_align(objects.lbl_locked_hdr, LV_TEXT_ALIGN_CENTER, 0);
    }
    if (objects.lbl_locked_badge && lv_obj_is_valid(objects.lbl_locked_badge)) {
        lv_label_set_text(objects.lbl_locked_badge, ui_lang_str(STR_LOCKED_BADGE));
        lv_obj_set_style_text_font(objects.lbl_locked_badge, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_locked_badge, ui_lang_base_dir(), 0);
        lv_obj_center(objects.lbl_locked_badge);
    }
    if (objects.lbl_fixed_cd && lv_obj_is_valid(objects.lbl_fixed_cd)) {
        lv_label_set_text(objects.lbl_fixed_cd, ui_lang_str(STR_FIXED_COOLDOWN));
        lv_obj_set_style_text_font(objects.lbl_fixed_cd, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_fixed_cd, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.lbl_fixed_cd, LV_ALIGN_RIGHT_MID, -6, 0);
            if (s_val_fixed_cd && lv_obj_is_valid(s_val_fixed_cd)) {
                lv_obj_align(s_val_fixed_cd, LV_ALIGN_LEFT_MID, 6, 0);
            }
        } else {
            lv_obj_align(objects.lbl_fixed_cd, LV_ALIGN_LEFT_MID, 6, 0);
            if (s_val_fixed_cd && lv_obj_is_valid(s_val_fixed_cd)) {
                lv_obj_align(s_val_fixed_cd, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    if (objects.lbl_fixed_rst && lv_obj_is_valid(objects.lbl_fixed_rst)) {
        lv_label_set_text(objects.lbl_fixed_rst, ui_lang_str(STR_FIXED_RESET_PULSE));
        lv_obj_set_style_text_font(objects.lbl_fixed_rst, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_fixed_rst, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.lbl_fixed_rst, LV_ALIGN_RIGHT_MID, -6, 0);
            if (s_val_fixed_rst && lv_obj_is_valid(s_val_fixed_rst)) {
                lv_obj_align(s_val_fixed_rst, LV_ALIGN_LEFT_MID, 6, 0);
            }
        } else {
            lv_obj_align(objects.lbl_fixed_rst, LV_ALIGN_LEFT_MID, 6, 0);
            if (s_val_fixed_rst && lv_obj_is_valid(s_val_fixed_rst)) {
                lv_obj_align(s_val_fixed_rst, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    if (objects.lbl_fixed_deb && lv_obj_is_valid(objects.lbl_fixed_deb)) {
        lv_label_set_text(objects.lbl_fixed_deb, ui_lang_str(STR_FIXED_DEBOUNCE));
        lv_obj_set_style_text_font(objects.lbl_fixed_deb, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_fixed_deb, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.lbl_fixed_deb, LV_ALIGN_RIGHT_MID, -6, 0);
            if (s_val_fixed_deb && lv_obj_is_valid(s_val_fixed_deb)) {
                lv_obj_align(s_val_fixed_deb, LV_ALIGN_LEFT_MID, 6, 0);
            }
        } else {
            lv_obj_align(objects.lbl_fixed_deb, LV_ALIGN_LEFT_MID, 6, 0);
            if (s_val_fixed_deb && lv_obj_is_valid(s_val_fixed_deb)) {
                lv_obj_align(s_val_fixed_deb, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    if (objects.lbl_fixed_cyl && lv_obj_is_valid(objects.lbl_fixed_cyl)) {
        lv_label_set_text(objects.lbl_fixed_cyl, ui_lang_str(STR_FIXED_CYL_LIMIT));
        lv_obj_set_style_text_font(objects.lbl_fixed_cyl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_fixed_cyl, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.lbl_fixed_cyl, LV_ALIGN_RIGHT_MID, -6, 0);
            if (s_val_fixed_cyl && lv_obj_is_valid(s_val_fixed_cyl)) {
                lv_obj_align(s_val_fixed_cyl, LV_ALIGN_LEFT_MID, 6, 0);
            }
        } else {
            lv_obj_align(objects.lbl_fixed_cyl, LV_ALIGN_LEFT_MID, 6, 0);
            if (s_val_fixed_cyl && lv_obj_is_valid(s_val_fixed_cyl)) {
                lv_obj_align(s_val_fixed_cyl, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    if (objects.lbl_fixed_eye && lv_obj_is_valid(objects.lbl_fixed_eye)) {
        lv_label_set_text(objects.lbl_fixed_eye, "");
        lv_obj_set_style_text_font(objects.lbl_fixed_eye, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_fixed_eye, ui_lang_base_dir(), 0);
        lv_obj_align(objects.lbl_fixed_eye, (ui_lang_get() == UI_LANG_FA) ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, (ui_lang_get() == UI_LANG_FA) ? -6 : 42, 0);
    }
    if (objects.lbl_warr_activate_btn && lv_obj_is_valid(objects.lbl_warr_activate_btn)) {
        lv_label_set_text(objects.lbl_warr_activate_btn, ui_lang_str(STR_WARR_ACTIVATE_BTN));
        lv_obj_set_style_text_font(objects.lbl_warr_activate_btn, ui_lang_font_16(), 0);
        lv_obj_center(objects.lbl_warr_activate_btn);
    }
    if (s_lbl_warr_title && lv_obj_is_valid(s_lbl_warr_title)) {
        lv_label_set_text(s_lbl_warr_title, ui_lang_str(STR_WARR_TITLE));
        lv_obj_set_style_text_font(s_lbl_warr_title, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(s_lbl_warr_title, ui_lang_base_dir(), 0);
        lv_obj_center(s_lbl_warr_title);
    }
    if (s_lbl_warr_name && lv_obj_is_valid(s_lbl_warr_name)) {
        lv_label_set_text(s_lbl_warr_name, ui_lang_str(STR_WARR_BUYER_NAME));
        lv_obj_set_style_text_font(s_lbl_warr_name, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(s_lbl_warr_name, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_align(s_lbl_warr_name, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    }
    if (s_lbl_warr_city && lv_obj_is_valid(s_lbl_warr_city)) {
        lv_label_set_text(s_lbl_warr_city, ui_lang_str(STR_WARR_CITY));
        lv_obj_set_style_text_font(s_lbl_warr_city, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(s_lbl_warr_city, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_align(s_lbl_warr_city, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    }
    if (s_lbl_warr_phone && lv_obj_is_valid(s_lbl_warr_phone)) {
        lv_label_set_text(s_lbl_warr_phone, ui_lang_str(STR_WARR_PHONE));
        lv_obj_set_style_text_font(s_lbl_warr_phone, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(s_lbl_warr_phone, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_align(s_lbl_warr_phone, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    }
    if (s_lbl_warr_submit && lv_obj_is_valid(s_lbl_warr_submit)) {
        lv_label_set_text(s_lbl_warr_submit, ui_lang_str(STR_WARR_SUBMIT));
        lv_obj_set_style_text_font(s_lbl_warr_submit, ui_lang_font_16(), 0);
        lv_obj_center(s_lbl_warr_submit);
    }
    if (s_lbl_warr_status && lv_obj_is_valid(s_lbl_warr_status)) {
        lv_label_set_text(s_lbl_warr_status, (ui_lang_get() == UI_LANG_FA) ? "وضعیت: فعال نشده" :
                                             ((ui_lang_get() == UI_LANG_TR) ? "Durum: Etkin degil" :
                                              "Status: Inactive"));
        lv_obj_set_style_base_dir(s_lbl_warr_status, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_align(s_lbl_warr_status, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    }
    const char *ph_name = (ui_lang_get() == UI_LANG_FA) ? "" : ui_lang_str(STR_WARR_PH_NAME);
    const char *ph_city = (ui_lang_get() == UI_LANG_FA) ? "" : ui_lang_str(STR_WARR_PH_CITY);
    const char *ph_phone = (ui_lang_get() == UI_LANG_FA) ? "" : ui_lang_str(STR_WARR_PH_PHONE);
    if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) {
        lv_textarea_set_placeholder_text(s_ta_warr_name, ph_name);
    }
    if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
        lv_textarea_set_placeholder_text(s_ta_warr_city, ph_city);
    }
    if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) {
        lv_textarea_set_placeholder_text(s_ta_warr_phone, ph_phone);
    }
    ui_screens_update_warranty_f6();
    if (s_warr_details_backdrop && lv_obj_is_valid(s_warr_details_backdrop)) {
        open_warranty_details_dialog();
    }
    day_lock_ui_refresh_5timers();

    /* به‌روزرسانی تاریخ تب F2 (تبدیل خودکار شمسی <-> میلادی هنگام تغییر زبان) */
    if (objects.year_lbl && lv_obj_is_valid(objects.year_lbl)) {
        if (ui_lang_get() == UI_LANG_FA) {
            if (ds_year >= 2000 && ds_year <= 2099) {
                int jy = 0, jm = 0, jd = 0;
                gregorian_to_jalali((int)ds_year, (int)ds_month, (int)ds_day, &jy, &jm, &jd);
                ds_year = jy;
                ds_month = jm;
                ds_day = jd;
            } else if (ds_year < 1400 || ds_year > 1499) {
                ds_year = (jt.jy > 0) ? jt.jy : 1405;
                ds_month = (jt.jm > 0) ? jt.jm : 1;
                ds_day = (jt.jd > 0) ? jt.jd : 1;
            }
        } else {
            if (ds_year >= 1300 && ds_year < 1600) {
                int gy = 0, gm = 0, gd = 0;
                jalali_to_gregorian((int)ds_year, (int)ds_month, (int)ds_day, &gy, &gm, &gd);
                ds_year = gy;
                ds_month = gm;
                ds_day = gd;
            } else if (ds_year < 2020 || ds_year > 2099) {
                ds_year = 2026;
                ds_month = 1;
                ds_day = 1;
            }
        }
        lv_label_set_text_fmt(objects.year_lbl, "%04ld", (long)ds_year);
        if (objects.month_lbl && lv_obj_is_valid(objects.month_lbl)) {
            lv_label_set_text_fmt(objects.month_lbl, "%02ld", (long)ds_month);
        }
        if (objects.day_lbl && lv_obj_is_valid(objects.day_lbl)) {
            lv_label_set_text_fmt(objects.day_lbl, "%02ld", (long)ds_day);
        }
    }

    /* ۴. کلیدهای نوار ابزار پایین */
    if (objects.save_lbl && lv_obj_is_valid(objects.save_lbl)) {
        lv_label_set_text(objects.save_lbl, ui_lang_str(STR_BTN_SAVE));
        lv_obj_set_style_text_font(objects.save_lbl, ui_lang_font_16(), 0);
        lv_obj_center(objects.save_lbl);
        ui_set_save_btn_state(settings_is_dirty());
    }
    if (objects.help_lbl && lv_obj_is_valid(objects.help_lbl)) {
        lv_label_set_text(objects.help_lbl, ui_lang_str(STR_BTN_HELP));
        lv_obj_set_style_text_font(objects.help_lbl, ui_lang_font_16(), 0);
        lv_obj_center(objects.help_lbl);
    }
    if (objects.fault_lbl && lv_obj_is_valid(objects.fault_lbl)) {
        lv_label_set_text(objects.fault_lbl, ui_lang_str(STR_BTN_FAULTS));
        lv_obj_set_style_text_font(objects.fault_lbl, ui_lang_font_16(), 0);
        lv_obj_center(objects.fault_lbl);
    }

    /* کپسول یکپارچه مانیتورینگ زنده صفحه اصلی */
    if (objects.live_monitor_status_badge && lv_obj_is_valid(objects.live_monitor_status_badge)) {
        lv_obj_set_pos(objects.live_monitor_status_badge, 8, 7);
        lv_obj_set_size(objects.live_monitor_status_badge, 448, 26);
    }
    if (objects.live_monitor_status_lbl && lv_obj_is_valid(objects.live_monitor_status_lbl)) {
        lv_obj_set_style_text_font(objects.live_monitor_status_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.live_monitor_status_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(objects.live_monitor_status_lbl);
    }
    if (objects.live_monitor_next_lbl && lv_obj_is_valid(objects.live_monitor_next_lbl)) {
        lv_obj_set_style_text_font(objects.live_monitor_next_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.live_monitor_next_lbl, ui_lang_base_dir(), 0);
    }

    /* ۴-الف. المان‌های چندزبانه تب F1 (شمارنده و استندبای و سنسور پولی) */
    if (objects.lbl_cnt_title && lv_obj_is_valid(objects.lbl_cnt_title)) {
        lv_label_set_text(objects.lbl_cnt_title, ui_lang_str(STR_TOTAL));
        lv_obj_set_style_text_font(objects.lbl_cnt_title, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_cnt_title, ui_lang_base_dir(), 0);
        if (objects.total_counter_lbl && lv_obj_is_valid(objects.total_counter_lbl)) {
            if (ui_lang_get() == UI_LANG_FA) {
                lv_obj_align(objects.lbl_cnt_title, LV_ALIGN_RIGHT_MID, -6, 0);
                lv_obj_align(objects.total_counter_lbl, LV_ALIGN_LEFT_MID, 6, 0);
            } else {
                lv_obj_align(objects.lbl_cnt_title, LV_ALIGN_LEFT_MID, 6, 0);
                lv_obj_align(objects.total_counter_lbl, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    if (objects.lbl_cnt_rst && lv_obj_is_valid(objects.lbl_cnt_rst)) {
        lv_label_set_text(objects.lbl_cnt_rst, ui_lang_str(STR_COUNTER_RESET));
        lv_obj_set_style_text_font(objects.lbl_cnt_rst, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_cnt_rst, ui_lang_base_dir(), 0);
    }
    update_ss_timeout_ui();
    update_pulley_en_ui();

    /* ۵. برچسب‌های کپسول دما و حالت‌های صفحه اصلی */
    if (objects.temp_hopper_lbl && lv_obj_is_valid(objects.temp_hopper_lbl)) {
        lv_label_set_text(objects.temp_hopper_lbl, ui_lang_str(STR_HOPPER_LABEL));
        lv_obj_set_style_text_font(objects.temp_hopper_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.temp_hopper_lbl, ui_lang_base_dir(), 0);
        if (objects.temp_hopper_box && lv_obj_is_valid(objects.temp_hopper_box)) {
            lv_obj_set_style_base_dir(objects.temp_hopper_box, ui_lang_base_dir(), 0);
        }
        if (objects.temp_hopper_cont && lv_obj_is_valid(objects.temp_hopper_cont)) {
            lv_obj_set_style_base_dir(objects.temp_hopper_cont, LV_BASE_DIR_LTR, 0);
        }
        if (objects.temp_hopper_val && lv_obj_is_valid(objects.temp_hopper_val) && !max6675_is_top_valid()) {
            lv_label_set_text(objects.temp_hopper_val, ui_lang_str(STR_SENSOR_NC));
            lv_obj_set_style_text_font(objects.temp_hopper_val, ui_lang_font_16(), 0);
        }
    }
    if (objects.temp_cyl_lbl && lv_obj_is_valid(objects.temp_cyl_lbl)) {
        lv_label_set_text(objects.temp_cyl_lbl, ui_lang_str(STR_CYL_LABEL));
        lv_obj_set_style_text_font(objects.temp_cyl_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.temp_cyl_lbl, ui_lang_base_dir(), 0);
        if (objects.temp_cyl_box && lv_obj_is_valid(objects.temp_cyl_box)) {
            lv_obj_set_style_base_dir(objects.temp_cyl_box, ui_lang_base_dir(), 0);
        }
        if (objects.temp_cyl_cont && lv_obj_is_valid(objects.temp_cyl_cont)) {
            lv_obj_set_style_base_dir(objects.temp_cyl_cont, LV_BASE_DIR_LTR, 0);
        }
        if (objects.temp_cyl_val && lv_obj_is_valid(objects.temp_cyl_val) && !max6675_is_cylinder_valid()) {
            lv_label_set_text(objects.temp_cyl_val, "--.-");
            lv_obj_set_style_text_color(objects.temp_cyl_val, lv_color_hex(0x64748b), 0);
            lv_obj_set_style_text_font(objects.temp_cyl_val, &ui_font_yekan_16, 0);
        }
    }
    if (objects.diag_temp_hopper_val && lv_obj_is_valid(objects.diag_temp_hopper_val) && !max6675_is_top_valid()) {
        lv_label_set_text(objects.diag_temp_hopper_val, ui_lang_str(STR_SENSOR_NC));
        lv_obj_set_style_text_font(objects.diag_temp_hopper_val, ui_lang_font_16(), 0);
    }
    if (objects.diag_temp_cyl_val && lv_obj_is_valid(objects.diag_temp_cyl_val) && !max6675_is_cylinder_valid()) {
        lv_label_set_text(objects.diag_temp_cyl_val, ui_lang_str(STR_SENSOR_NC));
        lv_obj_set_style_text_font(objects.diag_temp_cyl_val, ui_lang_font_16(), 0);
    }
    const int32_t mode_lbl_y = 224;
    if (objects.lbl_mode_make && lv_obj_is_valid(objects.lbl_mode_make)) {
        lv_label_set_text(objects.lbl_mode_make, ui_lang_str(STR_MODE_PROD));
        lv_obj_set_style_text_font(objects.lbl_mode_make, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_mode_make, ui_lang_base_dir(), 0);
        lv_obj_set_y(objects.lbl_mode_make, mode_lbl_y);
    }
    if (objects.lbl_mode_hold && lv_obj_is_valid(objects.lbl_mode_hold)) {
        lv_label_set_text(objects.lbl_mode_hold, ui_lang_str(STR_MODE_HOLD));
        lv_obj_set_style_text_font(objects.lbl_mode_hold, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_mode_hold, ui_lang_base_dir(), 0);
        lv_obj_set_y(objects.lbl_mode_hold, mode_lbl_y);
    }
    if (objects.lbl_mode_pdl && lv_obj_is_valid(objects.lbl_mode_pdl)) {
        lv_label_set_text(objects.lbl_mode_pdl, ui_lang_str(STR_MODE_PADDLE_BEATER));
        lv_obj_set_style_text_font(objects.lbl_mode_pdl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_mode_pdl, ui_lang_base_dir(), 0);
        lv_obj_set_y(objects.lbl_mode_pdl, mode_lbl_y);
    }
    if (objects.lbl_mode_edge && lv_obj_is_valid(objects.lbl_mode_edge)) {
        lv_label_set_text(objects.lbl_mode_edge, ui_lang_str(STR_EDGE_INJECT));
        lv_obj_set_style_text_font(objects.lbl_mode_edge, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_mode_edge, ui_lang_base_dir(), 0);
        lv_obj_set_y(objects.lbl_mode_edge, mode_lbl_y);
    }
    if (objects.lbl_mode_rb && lv_obj_is_valid(objects.lbl_mode_rb)) {
        lv_label_set_text(objects.lbl_mode_rb, ui_lang_str(STR_RAINBOW));
        lv_obj_set_style_text_font(objects.lbl_mode_rb, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_mode_rb, ui_lang_base_dir(), 0);
        lv_obj_set_y(objects.lbl_mode_rb, mode_lbl_y);
    }
    if (objects.lbl_mode_wash && lv_obj_is_valid(objects.lbl_mode_wash)) {
        lv_label_set_text(objects.lbl_mode_wash, ui_lang_str(STR_MODE_WASH));
        lv_obj_set_style_text_font(objects.lbl_mode_wash, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_mode_wash, ui_lang_base_dir(), 0);
        lv_obj_set_y(objects.lbl_mode_wash, mode_lbl_y);
    }

    /* ۵-ب. به‌روزرسانی بج‌های وضعیت ۶ گانه صفحه اصلی */
    lv_obj_t *statuses[6] = {
        objects.comp_status, objects.fridge_status, objects.paddle_status,
        objects.comp_status_b, objects.fridge_status_b, objects.paddle_status_b
    };
    for (int i = 0; i < 6; i++) {
        if (statuses[i] && lv_obj_is_valid(statuses[i])) {
            lv_obj_t *lbl = lv_obj_get_child(statuses[i], 0);
            if (lbl && lv_obj_is_valid(lbl)) {
                lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
                lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);
                lv_label_set_text(lbl, ui_lang_str(STR_STATUS_OFF));
                lv_obj_center(lbl);
            }
        }
    }

    /* ۵-الف. به‌روزرسانی کارت روزشمار و قفل اقساط در تب F2 */
    if (objects.lbl_dl_hdr && lv_obj_is_valid(objects.lbl_dl_hdr)) {
        lv_label_set_text(objects.lbl_dl_hdr, ui_lang_str(STR_DL_TITLE));
        lv_obj_set_style_text_font(objects.lbl_dl_hdr, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_dl_hdr, ui_lang_base_dir(), 0);
    }
    if (objects.lease_days_lbl && lv_obj_is_valid(objects.lease_days_lbl)) {
        DayLockState_t l_st = day_lock_get_state();
        bool is_locked = (l_st == LOCK_STAGE_HARD_LOCK || l_st == LOCK_STAGE_TAMPER);
        uint8_t init_stg = day_lock_get_active_stage();
        uint8_t tot_stg = day_lock_get_total_stages();
        if (tot_stg < init_stg) tot_stg = init_stg;
        if (tot_stg < 1) tot_stg = 1;
        ui_lang_t init_lang = ui_lang_get();
        if (is_locked) {
            if (init_stg >= 1 && init_stg <= DAY_LOCK_MAX_STAGES) {
                char lk_buf[48];
                if (init_lang == UI_LANG_FA) {
                    snprintf(lk_buf, sizeof(lk_buf), "قفل: پایان قسط %u از %u", (unsigned)init_stg, (unsigned)tot_stg);
                } else if (init_lang == UI_LANG_TR) {
                    snprintf(lk_buf, sizeof(lk_buf), "Kilitli: %u/%u Taksit", (unsigned)init_stg, (unsigned)tot_stg);
                } else {
                    snprintf(lk_buf, sizeof(lk_buf), "Locked: Stage %u/%u", (unsigned)init_stg, (unsigned)tot_stg);
                }
                lv_label_set_text(objects.lease_days_lbl, lk_buf);
            } else {
                lv_label_set_text(objects.lease_days_lbl, ui_lang_str(STR_DL_STATUS_LOCKED));
            }
        } else if (l_st == LOCK_STAGE_ACTIVE_LEASE || l_st == LOCK_STAGE_GRACE_10) {
            char act_buf[48];
            format_lease_remain_str(act_buf, sizeof(act_buf), init_stg, tot_stg, init_lang);
            lv_label_set_text(objects.lease_days_lbl, act_buf);
        } else {
            lv_label_set_text(objects.lease_days_lbl, ui_lang_str(STR_DL_STATUS_UNLIMITED));
        }
        lv_obj_set_style_text_font(objects.lease_days_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lease_days_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(objects.lease_days_lbl);
    }
    if (objects.lbl_ch_title && lv_obj_is_valid(objects.lbl_ch_title)) {
        lv_label_set_text(objects.lbl_ch_title, ui_lang_str(STR_DL_CHALLENGE));
        lv_obj_set_style_text_font(objects.lbl_ch_title, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_ch_title, ui_lang_base_dir(), 0);
    }
    if (objects.lease_btn_extend && lv_obj_is_valid(objects.lease_btn_extend)) {
        lv_obj_t *lbl = lv_obj_get_child(objects.lease_btn_extend, 0);
        if (lbl && lv_obj_is_valid(lbl)) {
            lv_label_set_text(lbl, ui_lang_str(STR_DL_EXTEND_BTN));
            lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);
            lv_obj_center(lbl);
        }
    }
    if (objects.lease_btn_5timers && lv_obj_is_valid(objects.lease_btn_5timers)) {
        lv_obj_t *lbl = lv_obj_get_child(objects.lease_btn_5timers, 0);
        if (lbl && lv_obj_is_valid(lbl)) {
            lv_label_set_text(lbl, ui_lang_str(STR_DL_5TIMERS_BTN));
            lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);
            lv_obj_center(lbl);
        }
    }
    if (objects.lbl_dl_sup && lv_obj_is_valid(objects.lbl_dl_sup)) {
        lv_label_set_text(objects.lbl_dl_sup, ui_lang_str(STR_DL_SUPPORT));
        lv_obj_set_style_text_font(objects.lbl_dl_sup, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_dl_sup, ui_lang_base_dir(), 0);
    }
    if (objects.lbl_dl_emg && lv_obj_is_valid(objects.lbl_dl_emg)) {
        lv_label_set_text(objects.lbl_dl_emg, ui_lang_str(STR_DL_EMERGENCY));
        lv_obj_set_style_text_font(objects.lbl_dl_emg, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_dl_emg, ui_lang_base_dir(), 0);
    }
    if (objects.lbl_factory_reset && lv_obj_is_valid(objects.lbl_factory_reset)) {
        char rst_txt[80];
        if (ui_lang_get() == UI_LANG_FA) {
            snprintf(rst_txt, sizeof(rst_txt), "%s بازنشانی کارخانه\n(تنظیمات اولیه)", LV_SYMBOL_REFRESH);
        } else if (ui_lang_get() == UI_LANG_TR) {
            snprintf(rst_txt, sizeof(rst_txt), "%s Fabrika Ayarlari\n(Varsayilana Don)", LV_SYMBOL_REFRESH);
        } else {
            snprintf(rst_txt, sizeof(rst_txt), "%s Factory Reset\n(Restore Defaults)", LV_SYMBOL_REFRESH);
        }
        lv_label_set_text(objects.lbl_factory_reset, rst_txt);
        lv_obj_set_style_text_font(objects.lbl_factory_reset, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_factory_reset, ui_lang_base_dir(), 0);
        lv_obj_center(objects.lbl_factory_reset);
    }

    if (objects.lbl_ntp_sync && lv_obj_is_valid(objects.lbl_ntp_sync)) {
        lv_label_set_text(objects.lbl_ntp_sync, ui_lang_str(STR_NTP_SYNC_BTN));
        lv_obj_set_style_text_font(objects.lbl_ntp_sync, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lbl_ntp_sync, ui_lang_base_dir(), 0);
        lv_obj_center(objects.lbl_ntp_sync);
    }

    /* ۶. کلیدهای تب وای‌فای F4 */
    if (objects.wifi_scan_lbl && lv_obj_is_valid(objects.wifi_scan_lbl)) {
        lv_label_set_text(objects.wifi_scan_lbl, ui_lang_str(STR_WIFI_SCAN));
        lv_obj_set_style_text_font(objects.wifi_scan_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.wifi_scan_lbl, ui_lang_base_dir(), 0);
    }
    if (objects.wifi_connect_lbl && lv_obj_is_valid(objects.wifi_connect_lbl)) {
        lv_label_set_text(objects.wifi_connect_lbl, ui_lang_str(STR_WIFI_CONNECT));
        lv_obj_set_style_text_font(objects.wifi_connect_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.wifi_connect_lbl, ui_lang_base_dir(), 0);
    }
    if (objects.wifi_ota_lbl && lv_obj_is_valid(objects.wifi_ota_lbl)) {
        lv_label_set_text(objects.wifi_ota_lbl, ui_lang_str(STR_OTA_UPDATE_BTN));
        lv_obj_set_style_text_font(objects.wifi_ota_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.wifi_ota_lbl, ui_lang_base_dir(), 0);
    }
    if (objects.wifi_pass_title_lbl && lv_obj_is_valid(objects.wifi_pass_title_lbl)) {
        lv_label_set_text(objects.wifi_pass_title_lbl, ui_lang_str(STR_WIFI_PASS_TITLE));
        lv_obj_set_style_text_font(objects.wifi_pass_title_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.wifi_pass_title_lbl, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.wifi_pass_title_lbl, LV_ALIGN_TOP_RIGHT, -4, 2);
        } else {
            lv_obj_align(objects.wifi_pass_title_lbl, LV_ALIGN_TOP_LEFT, 4, 2);
        }
    }
    if (objects.wifi_pass_txtarea && lv_obj_is_valid(objects.wifi_pass_txtarea)) {
        lv_textarea_set_placeholder_text(objects.wifi_pass_txtarea, ui_lang_str(STR_WIFI_PASS_PLACEHOLDER));
    }
    if (objects.ui_scan_list && lv_obj_is_valid(objects.ui_scan_list)) {
        if (lv_obj_get_child_count(objects.ui_scan_list) == 1) {
            lv_obj_t *first_child = lv_obj_get_child(objects.ui_scan_list, 0);
            if (first_child && lv_obj_check_type(first_child, &lv_label_class)) {
                if (wifi_is_scanning()) {
                    lv_label_set_text(first_child, ui_lang_str(STR_WIFI_SCANNING));
                } else {
                    lv_label_set_text(first_child, (ui_lang_get() == UI_LANG_FA) ?
                        "جهت جستجو و مشاهده شبکه‌های وای‌فای،\nدکمه [جستجو] را لمس کنید." :
                        ((ui_lang_get() == UI_LANG_TR) ?
                        "Wi-Fi aglarini aramak icin\n[Ara] butonuna dokunun." :
                        "To scan and view Wi-Fi networks,\ntap [Scan] button."));
                }
                lv_obj_set_style_text_font(first_child, ui_lang_font_16(), 0);
                lv_obj_set_style_base_dir(first_child, ui_lang_base_dir(), 0);
                lv_obj_set_style_pad_ver(first_child, 14, 0);
            }
        }
    }

    /* ۷. برچسب‌های تب دیاگ F5 */
    if (objects.f6_in_title_lbl && lv_obj_is_valid(objects.f6_in_title_lbl)) {
        lv_label_set_text(objects.f6_in_title_lbl, ui_lang_str(STR_F6_INPUTS_HDR));
        lv_obj_set_style_text_font(objects.f6_in_title_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.f6_in_title_lbl, ui_lang_base_dir(), 0);
        lv_obj_align(objects.f6_in_title_lbl, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -3 : 3);
    }
    if (objects.f6_out_title_lbl && lv_obj_is_valid(objects.f6_out_title_lbl)) {
        lv_label_set_text(objects.f6_out_title_lbl, ui_lang_str(STR_F6_OUTPUTS_HDR));
        lv_obj_set_style_text_font(objects.f6_out_title_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.f6_out_title_lbl, ui_lang_base_dir(), 0);
        lv_obj_align(objects.f6_out_title_lbl, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -3 : 3);
    }
    static const ui_str_key_t s_f6_in_keys[7] = {
        STR_F6_IN_EYE, STR_F6_IN_REAR_SW, STR_F6_IN_FRONT_SW,
        STR_F6_IN_STOP, STR_F6_IN_START, STR_F6_IN_MTR_OVL, STR_F6_IN_COMP_OVL
    };
    for (int i = 0; i < 7; i++) {
        if (objects.f6_in_lbls[i] && lv_obj_is_valid(objects.f6_in_lbls[i])) {
            lv_label_set_text(objects.f6_in_lbls[i], ui_lang_str(s_f6_in_keys[i]));
            lv_obj_set_style_text_font(objects.f6_in_lbls[i], ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(objects.f6_in_lbls[i], ui_lang_base_dir(), 0);
            int32_t iy = 26 + (i * 22);
            if (ui_lang_get() == UI_LANG_FA) {
                lv_obj_align(objects.f6_in_lbls[i], LV_ALIGN_TOP_RIGHT, -6, iy - 3);
            } else {
                lv_obj_align(objects.f6_in_lbls[i], LV_ALIGN_TOP_LEFT, 42, iy - 3);
            }
        }
    }
    static const ui_str_key_t s_f6_out_keys[8] = {
        STR_F6_OUT_COMP, STR_F6_OUT_PROD, STR_F6_OUT_MIXER,
        STR_F6_OUT_EDGE, STR_F6_OUT_RAINBOW, STR_F6_OUT_AIR,
        STR_F6_OUT_RESET, STR_F6_OUT_WASH
    };
    for (int i = 0; i < 8; i++) {
        if (objects.f6_out_lbls[i] && lv_obj_is_valid(objects.f6_out_lbls[i])) {
            lv_label_set_text(objects.f6_out_lbls[i], ui_lang_str(s_f6_out_keys[i]));
            lv_obj_set_style_text_font(objects.f6_out_lbls[i], ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(objects.f6_out_lbls[i], ui_lang_base_dir(), 0);
        }
    }
    for (int i = 0; i < 8; i++) {
        if (objects.lbl_in[i] && lv_obj_is_valid(objects.lbl_in[i])) {
            int8_t r = pinmap_get_role_for_terminal((uint8_t)i);
            if (r >= 0) {
                const char *rname = pinmap_get_role_name((pinmap_role_t)r);
                lv_label_set_text(objects.lbl_in[i], rname);
                lv_obj_set_style_text_color(objects.lbl_in[i], lv_color_hex(0x0f172a), 0);
            } else {
                lv_label_set_text(objects.lbl_in[i], "");
            }
            lv_obj_set_style_text_font(objects.lbl_in[i], ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(objects.lbl_in[i], ui_lang_base_dir(), 0);
            lv_obj_set_style_bg_color(objects.lbl_in[i], lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_opa(objects.lbl_in[i], LV_OPA_COVER, 0);
            lv_obj_set_style_pad_hor(objects.lbl_in[i], 4, 0);
            lv_obj_set_width(objects.lbl_in[i], LV_SIZE_CONTENT);
            lv_obj_set_style_max_width(objects.lbl_in[i], 138, 0);
            lv_obj_update_layout(objects.lbl_in[i]);
            lv_obj_align(objects.lbl_in[i], LV_ALIGN_RIGHT_MID, -4, 0);
        }
    }
    if (objects.diag_t1_lbl && lv_obj_is_valid(objects.diag_t1_lbl)) {
        lv_label_set_text(objects.diag_t1_lbl, ui_lang_str(STR_F6_TEMP_T1));
        lv_obj_set_style_text_font(objects.diag_t1_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.diag_t1_lbl, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.diag_t1_lbl, LV_ALIGN_RIGHT_MID, -6, 0);
            if (objects.diag_temp_hopper_val && lv_obj_is_valid(objects.diag_temp_hopper_val)) {
                lv_obj_align(objects.diag_temp_hopper_val, LV_ALIGN_LEFT_MID, 6, 0);
            }
        } else {
            lv_obj_align(objects.diag_t1_lbl, LV_ALIGN_LEFT_MID, 6, 0);
            if (objects.diag_temp_hopper_val && lv_obj_is_valid(objects.diag_temp_hopper_val)) {
                lv_obj_align(objects.diag_temp_hopper_val, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    if (objects.diag_t2_lbl && lv_obj_is_valid(objects.diag_t2_lbl)) {
        lv_label_set_text(objects.diag_t2_lbl, ui_lang_str(STR_F6_TEMP_T2));
        lv_obj_set_style_text_font(objects.diag_t2_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.diag_t2_lbl, ui_lang_base_dir(), 0);
        if (ui_lang_get() == UI_LANG_FA) {
            lv_obj_align(objects.diag_t2_lbl, LV_ALIGN_RIGHT_MID, -6, 0);
            if (objects.diag_temp_cyl_val && lv_obj_is_valid(objects.diag_temp_cyl_val)) {
                lv_obj_align(objects.diag_temp_cyl_val, LV_ALIGN_LEFT_MID, 6, 0);
            }
        } else {
            lv_obj_align(objects.diag_t2_lbl, LV_ALIGN_LEFT_MID, 6, 0);
            if (objects.diag_temp_cyl_val && lv_obj_is_valid(objects.diag_temp_cyl_val)) {
                lv_obj_align(objects.diag_temp_cyl_val, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
    }
    ui_update_toolbar_buttons();
}

void ui_update_toolbar_buttons(void)
{
    /* 1. دکمه اختصاصی وای‌فای: پس‌زمینه تیره متالیک (#334155)، بدون بوردر دندانه‌دار، کرو ۱۲px */
    if (objects.wifi_sw_btn && lv_obj_is_valid(objects.wifi_sw_btn)) {
        bool w_en = wifi_is_enabled();
        lv_obj_set_style_bg_color(objects.wifi_sw_btn, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(objects.wifi_sw_btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(objects.wifi_sw_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (objects.wifi_sw_lbl && lv_obj_is_valid(objects.wifi_sw_lbl)) {
            lv_label_set_text(objects.wifi_sw_lbl, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(objects.wifi_sw_lbl, lv_color_hex(w_en ? 0x22c55e : 0x64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    /* 2. دکمه بلندگوی پنل: پس‌زمینه تیره متالیک (#334155)، بدون بوردر دندانه‌دار، کرو ۱۲px */
    if (objects.buzzer_sw && lv_obj_is_valid(objects.buzzer_sw)) {
        bool b_en = buzzer_is_enabled() && (buzzer_get_volume() > 0);
        lv_obj_t *lbl_buz = objects.buzzer_sw_lbl ? objects.buzzer_sw_lbl : lv_obj_get_child(objects.buzzer_sw, 0);
        lv_obj_set_style_bg_color(objects.buzzer_sw, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(objects.buzzer_sw, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(objects.buzzer_sw, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (lbl_buz && lv_obj_is_valid(lbl_buz)) {
            lv_label_set_text(lbl_buz, b_en ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
            lv_obj_set_style_text_color(lbl_buz, lv_color_hex(b_en ? 0x38bdf8 : 0x64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    /* 3. دکمه مشاهده خطاها: بدون بوردر دندانه‌دار، کرو ۱۲px (قرمز فقط در خطای فعال لحظه‌ای) */
    if (objects.fault_btn && lv_obj_is_valid(objects.fault_btn)) {
        bool active_fault = plc_is_fault();
        lv_obj_set_style_radius(objects.fault_btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(objects.fault_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (active_fault) {
            lv_obj_set_style_bg_color(objects.fault_btn, lv_color_hex(0xdc2626), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(objects.fault_btn, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.fault_lbl && lv_obj_is_valid(objects.fault_lbl)) {
            lv_obj_set_style_text_color(objects.fault_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    ui_screens_update_warranty_f6();
}

static lv_obj_t *make_tab_header_bar(lv_obj_t *parent, const char *title, const char *icon_symbol, uint32_t icon_color)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_pos(bar, 8, 4);
    lv_obj_set_size(bar, 398, 30);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(bar, 255, 0);
    lv_obj_set_style_radius(bar, 6, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    create_circular_flag_button(bar, 6, 3);

    /* آیکون مدرن اپل سمت راست هدر متناسب با موضوع تب */
    if (icon_symbol && icon_symbol[0] != '\0') {
        lv_obj_t *icon_box = lv_obj_create(bar);
        lv_obj_set_pos(icon_box, 362, 3);
        lv_obj_set_size(icon_box, 30, 24);
        lv_obj_set_style_bg_color(icon_box, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_bg_opa(icon_box, 255, 0);
        lv_obj_set_style_radius(icon_box, 6, 0);
        lv_obj_set_style_border_width(icon_box, 1, 0);
        lv_obj_set_style_border_color(icon_box, lv_color_hex(icon_color), 0);
        lv_obj_set_style_pad_all(icon_box, 0, 0);
        lv_obj_set_style_shadow_width(icon_box, 0, 0);
        lv_obj_clear_flag(icon_box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *icon_lbl = lv_label_create(icon_box);
        lv_label_set_text(icon_lbl, icon_symbol);
        lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(icon_lbl, lv_color_hex(icon_color), 0);
        lv_obj_center(icon_lbl);
    }

    lv_obj_t *lbl = lv_label_create(bar);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_width(lbl, 290);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 18, 0);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    return lbl;
}

static void action_stepper_title_clicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    uint8_t param_id = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    ui_show_single_param_help(param_id, stepper_param_to_key(param_id));
}

static void action_stepper_card_bg_clicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *cur = lv_event_get_current_target(e);
    if (target != cur) return; /* User tapped child button/control */

    uint8_t param_id = (uint8_t)(intptr_t)lv_event_get_user_data(e);

    /* هدایت هوشمند تاچ در حواشی دکمه‌های استپر (راست/چپ/بین مثبت و منفی) */
    lv_indev_t *indev = lv_indev_active();
    if (!indev) indev = lv_indev_get_act();
    if (indev && lv_obj_get_child_count(cur) >= 3) {
        lv_point_t p;
        lv_indev_get_point(indev, &p);

        lv_obj_t *btn_m = lv_obj_get_child(cur, 0);
        lv_obj_t *btn_p = lv_obj_get_child(cur, 2);
        lv_area_t minus_area, plus_area;
        lv_obj_get_coords(btn_m, &minus_area);
        lv_obj_get_coords(btn_p, &plus_area);

        /* ۱. لمس در سمت چپ یا روی دکمه منفی تا مرز کارت */
        if (p.x <= minus_area.x2) {
            ui_stepper_do_step(param_id, -1);
            return;
        }

        /* ۲. لمس بین دکمه‌های منفی و مثبت (روی کادر مقدار) */
        if (p.x > minus_area.x2 && p.x < plus_area.x1) {
            lv_coord_t mid = (minus_area.x2 + plus_area.x1) / 2;
            if (p.x <= mid) {
                ui_stepper_do_step(param_id, -1);
            } else {
                ui_stepper_do_step(param_id, 1);
            }
            return;
        }

        /* ۳. لمس تا ۱۰ پیکسل سمت راست دکمه مثبت */
        if (p.x >= plus_area.x1 && p.x <= plus_area.x2 + 10) {
            ui_stepper_do_step(param_id, 1);
            return;
        }
    }

    /* ۴. لمس در سایر بخش‌های کارت (متن‌ها و فضای خالی راست): باز کردن راهنما */
    ui_show_single_param_help(param_id, stepper_param_to_key(param_id));
}

static void action_locked_item_clicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_str_key_t key = (ui_str_key_t)(intptr_t)lv_event_get_user_data(e);
    ui_show_single_param_help(0, key);
}

static lv_obj_t *ui_create_stepper_card(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                                       uint8_t param_id, const char *title_txt, const char *initial_val,
                                       lv_obj_t **out_val_obj)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    int32_t btn_w;
    int32_t btn_minus_x = 4;
    int32_t val_w;
    int32_t gap = 2;
    if (w <= 180) {
        /* تب F2 ساعت و تاریخ: کادر ۲۸px با دکمه‌های ۲۸×۲۸ متقارن */
        btn_w = 28;
        btn_minus_x = 4;
        val_w = 42;
    } else if (w > 300) {
        /* تب F3 نگهداری و برودت: کادر ۳۴px با دکمه‌های ۳۴×۳۴ متقارن */
        btn_w = 34;
        btn_minus_x = 6;
        val_w = 54;
        gap = 3;
    } else if (w == 238) {
        /* تب F6 ثابت‌های زمانی دیاگ: کادر ۲۶px با دکمه‌های ۲۶×۳۲ و فضای باز برای عناوین */
        btn_w = 26;
        btn_minus_x = 4;
        val_w = 30;
    } else {
        /* تب F1 تایمرهای تولید: کادر ۲۴px با دکمه‌های متقارن و فاصله باز و دلنشین تا پرانتزها */
        btn_minus_x = 2;
        btn_w = 24;
        val_w = 24;
    }
    int32_t val_x = btn_minus_x + btn_w + gap;
    int32_t btn_plus_x = val_x + val_w + gap;
    int32_t ctrl_end_x = btn_plus_x + btn_w;
    int32_t lbl_x = ctrl_end_x + 5;

    int32_t btn_h = (h > 46) ? (h - 14) : ((h <= 30) ? (h - 5) : (h - 6));
    int32_t btn_y = (h - btn_h) / 2 - 1;
    if (btn_y < 1) btn_y = 1;

    const lv_font_t *sym_font = (h >= 38 && w > 300) ? &lv_font_montserrat_30 : &lv_font_montserrat_24;

    /* 1. Decrement Button [-] (Apple Slate 700 with border and click extension) */
    lv_obj_t *btn_minus = lv_btn_create(card);
    lv_obj_set_pos(btn_minus, btn_minus_x, btn_y);
    lv_obj_set_size(btn_minus, btn_w, btn_h);
    lv_obj_set_style_radius(btn_minus, 6, 0);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_minus, 1, 0);
    lv_obj_set_style_border_color(btn_minus, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_minus, 0, 0);
    lv_obj_set_style_pad_all(btn_minus, 0, 0);
    lv_obj_set_ext_click_area(btn_minus, 8);
    lv_obj_add_flag(btn_minus, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(btn_minus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_minus, action_universal_stepper, LV_EVENT_ALL, (void *)(intptr_t)(param_id & 0xFF));

    lv_obj_t *l_minus = lv_label_create(btn_minus);
    lv_label_set_text(l_minus, "-");
    lv_obj_set_style_text_font(l_minus, sym_font, 0);
    lv_obj_set_style_text_color(l_minus, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(l_minus, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(l_minus, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l_minus, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(l_minus, LV_OBJ_FLAG_CLICKABLE);

    /* 2. Value Box Container (Apple Inset Card with pure mathematical center) */
    lv_obj_t *val_container = lv_obj_create(card);
    lv_obj_set_pos(val_container, val_x, btn_y);
    lv_obj_set_size(val_container, val_w, btn_h);
    ui_style_cyan_value(val_container);
    lv_obj_clear_flag(val_container, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(val_container, 0, 0);

    /* Inner Value Label — automatically centered vertically and horizontally */
    lv_obj_t *val_lbl = lv_label_create(val_container);
    lv_label_set_text(val_lbl, initial_val ? initial_val : "0");
    lv_obj_set_style_text_font(val_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_base_dir(val_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(val_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(val_lbl);
    lv_obj_clear_flag(val_lbl, LV_OBJ_FLAG_CLICKABLE);
    if (out_val_obj) *out_val_obj = val_lbl;

    /* 3. Increment Button [+] (Apple System Blue with border and click extension) */
    lv_obj_t *btn_plus = lv_btn_create(card);
    lv_obj_set_pos(btn_plus, btn_plus_x, btn_y);
    lv_obj_set_size(btn_plus, btn_w, btn_h);
    lv_obj_set_style_radius(btn_plus, 6, 0);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x0369a1), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_plus, 1, 0);
    lv_obj_set_style_border_color(btn_plus, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_shadow_width(btn_plus, 0, 0);
    lv_obj_set_style_pad_all(btn_plus, 0, 0);
    lv_obj_set_ext_click_area(btn_plus, 8);
    lv_obj_add_flag(btn_plus, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(btn_plus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_plus, action_universal_stepper, LV_EVENT_ALL, (void *)(intptr_t)((param_id & 0xFF) | 0x100));

    lv_obj_t *l_plus = lv_label_create(btn_plus);
    lv_label_set_text(l_plus, "+");
    lv_obj_set_style_text_font(l_plus, sym_font, 0);
    lv_obj_set_style_text_color(l_plus, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(l_plus, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(l_plus, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l_plus, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(l_plus, LV_OBJ_FLAG_CLICKABLE);

    /* 4. Title Label (Vertically centered with bounded width and ample breathing room from +) */
    int32_t max_title_w = (w <= 300 && w >= 238) ? (w - ctrl_end_x - 3) : ((w <= 300 && w > 180) ? (w - lbl_x - 3) : (w - lbl_x - 6));
    if (max_title_w < 50) max_title_w = 50;

    lv_obj_t *lbl = lv_label_create(card);
    ui_str_key_t key = stepper_param_to_key(param_id);
    const char *disp_text = (key < STR_COUNT) ? ui_lang_str(key) : (title_txt ? title_txt : "");
    lv_label_set_text(lbl, disp_text);
    lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x0f172a), 0);
    lv_obj_set_width(lbl, max_title_w);
    lv_obj_set_height(lbl, 22);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);

    if (ui_lang_get() == UI_LANG_FA) {
        lv_obj_align(lbl, LV_ALIGN_RIGHT_MID, (w <= 300 && w >= 238) ? -2 : -6, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_RIGHT, 0);
    } else {
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, lbl_x, 2);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
    }

    /* Make title label interactive and open explanation on touch */
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(lbl, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(lbl, action_stepper_title_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)param_id);

    /* Also allow tapping the card background around the title */
    lv_obj_set_ext_click_area(card, 8);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, action_stepper_card_bg_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)param_id);

    lv_obj_t *u_box = NULL;

    if (s_stepper_card_count < (uint8_t)(sizeof(s_stepper_cards) / sizeof(s_stepper_cards[0]))) {
        s_stepper_cards[s_stepper_card_count].card = card;
        s_stepper_cards[s_stepper_card_count].lbl_title = lbl;
        s_stepper_cards[s_stepper_card_count].key = key;
        s_stepper_cards[s_stepper_card_count].lbl_x = lbl_x;
        s_stepper_cards[s_stepper_card_count].u_box = u_box;
        s_stepper_cards[s_stepper_card_count].param_id = param_id;
        s_stepper_card_count++;
    }

    return card;
}

static lv_obj_t *s_help_backdrop = NULL;
static lv_obj_t *s_lease_backdrop = NULL;

void hide_modal_help_dialog(void)
{
    if (s_help_backdrop && lv_obj_is_valid(s_help_backdrop)) {
        lv_obj_delete(s_help_backdrop);
        extern void lvgl_port_suppress_touch_until_release(void);
        lvgl_port_suppress_touch_until_release();
    }
    s_help_backdrop = NULL;
    objects.help_modal = NULL;
    ui_hide_tooltip();
}

void show_modal_help_dialog(const char *title, const char *content)
{
    hide_modal_help_dialog();

    /* 1. Backdrop */
    s_help_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_help_backdrop, 0, 0);
    lv_obj_set_size(s_help_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_help_backdrop, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_help_backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_help_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_help_backdrop, 0, 0);
    lv_obj_clear_flag(s_help_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_help_backdrop, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_help_backdrop, action_close_help_dialog, LV_EVENT_CLICKED, NULL);

    /* 2. Centered Card */
    lv_obj_t *card = lv_obj_create(s_help_backdrop);
    objects.help_modal = card;
    lv_obj_set_pos(card, 20, 16);
    lv_obj_set_size(card, 440, 288);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, false, 0); /* Zero clipping mask overhead */
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

    /* 3. Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 440, 42);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(header, 255, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(title_lbl);

    lv_obj_t *btn_close = lv_btn_create(header);
    lv_obj_set_pos(btn_close, 399, 4);
    lv_obj_set_size(btn_close, 34, 34);
    lv_obj_set_ext_click_area(btn_close, 6);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, action_close_help_dialog, LV_EVENT_CLICKED, NULL);
    lv_obj_t *x_lbl = lv_label_create(btn_close);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(x_lbl);

    /* 4. Scrollable Body with modern Apple cards */
    lv_obj_t *scroll = lv_obj_create(card);
    lv_obj_set_pos(scroll, 10, 48);
    lv_obj_set_size(scroll, 420, 184);
    lv_obj_set_style_bg_color(scroll, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(scroll, 255, 0);
    lv_obj_set_style_border_width(scroll, 0, 0);
    lv_obj_set_style_pad_all(scroll, 2, 0);
    lv_obj_set_style_pad_bottom(scroll, 20, 0);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(scroll, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_anim_duration(scroll, 0, 0);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scroll, 8, 0);

    const char *p = content;
    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;

        const char *block_start = p;
        const char *block_end = NULL;

        const char *scan = block_start;
        while (*scan) {
            if (*scan == '\n') {
                const char *next = scan + 1;
                while (*next == ' ' || *next == '\t') next++;
                if (*next == '\n' || *next == '\r') {
                    block_end = scan;
                    break;
                }
                /* Check for bullet: • (\xE2\x80\xA2) or - or numbered list */
                if ((uint8_t)next[0] == 0xE2 && (uint8_t)next[1] == 0x80 && (uint8_t)next[2] == 0xA2) {
                    block_end = scan;
                    break;
                }
                if (*next == '-' && (next[1] == ' ' || (uint8_t)next[1] >= 0x80)) {
                    block_end = scan;
                    break;
                }
                if (*next >= '0' && *next <= '9') {
                    const char *d = next;
                    while (*d >= '0' && *d <= '9') d++;
                    if (*d == '.' || *d == ')') {
                        block_end = scan;
                        break;
                    }
                }
            }
            scan++;
        }
        if (!block_end) block_end = scan;

        int block_len = (int)(block_end - block_start);
        if (block_len > 0) {
            char buf[1024];
            if (block_len >= (int)sizeof(buf)) block_len = (int)sizeof(buf) - 1;
            memcpy(buf, block_start, block_len);
            buf[block_len] = '\0';

            char *colon = strchr(buf, ':');
            if (colon) {
                char *title_part = buf;
                char *desc_part = colon + 1;
                while (*desc_part == ' ' || *desc_part == '\t' || *desc_part == '\r' || *desc_part == '\n') {
                    desc_part++;
                }
                *colon = '\0'; /* Terminate title_part cleanly without truncating desc_part */

                lv_obj_t *item_card = lv_obj_create(scroll);
                lv_obj_set_width(item_card, 400);
                lv_obj_set_height(item_card, LV_SIZE_CONTENT);
                lv_obj_set_style_bg_color(item_card, lv_color_hex(0xf8fafc), 0);
                lv_obj_set_style_bg_opa(item_card, 255, 0);
                lv_obj_set_style_radius(item_card, 8, 0);
                lv_obj_set_style_border_width(item_card, 1, 0);
                lv_obj_set_style_border_color(item_card, lv_color_hex(0xe2e8f0), 0);
                lv_obj_set_style_pad_all(item_card, 8, 0);
                lv_obj_clear_flag(item_card, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_flex_flow(item_card, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_style_pad_row(item_card, 4, 0);

                /* عنوان با رنگ نیلی اپل و با فونت خوانا */
                lv_obj_t *t_lbl = lv_label_create(item_card);
                lv_obj_set_width(t_lbl, 384);
                lv_label_set_long_mode(t_lbl, LV_LABEL_LONG_WRAP);
                lv_label_set_text_fmt(t_lbl, "%s:", title_part);
                lv_obj_set_style_text_font(t_lbl, (ui_lang_get() == UI_LANG_FA) ? &ui_font_yekan_16 : &lv_font_montserrat_16, 0);
                lv_obj_set_style_text_color(t_lbl, lv_color_hex(0x0284c7), 0); /* Apple Cobalt Cyan */
                lv_obj_set_style_base_dir(t_lbl, ui_lang_base_dir(), 0);
                lv_obj_set_style_text_align(t_lbl, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);

                /* متن توضیح با رنگ تیره با کنتراست بالا */
                if (*desc_part) {
                    lv_obj_t *d_lbl = lv_label_create(item_card);
                    lv_obj_set_width(d_lbl, 384);
                    lv_label_set_long_mode(d_lbl, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(d_lbl, desc_part);
                    lv_obj_set_style_text_font(d_lbl, ui_lang_font_16(), 0);
                    lv_obj_set_style_text_color(d_lbl, lv_color_hex(0x1e293b), 0); /* Slate Dark Gray */
                    lv_obj_set_style_base_dir(d_lbl, ui_lang_base_dir(), 0);
                    lv_obj_set_style_text_align(d_lbl, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
                }
            } else {
                lv_obj_t *g_lbl = lv_label_create(scroll);
                lv_obj_set_width(g_lbl, 400);
                lv_label_set_text(g_lbl, buf);
                lv_label_set_long_mode(g_lbl, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_font(g_lbl, ui_lang_font_16(), 0);
                lv_obj_set_style_text_color(g_lbl, lv_color_hex(0x334155), 0);
                lv_obj_set_style_base_dir(g_lbl, ui_lang_base_dir(), 0);
                lv_obj_set_style_text_align(g_lbl, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
            }
        }
        p = block_end;
    }

    /* 5. Bottom action button */
    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_pos(btn_ok, 150, 238);
    lv_obj_set_size(btn_ok, 140, 40);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(btn_ok, 0, 0);
    lv_obj_add_event_cb(btn_ok, action_close_help_dialog, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(btn_ok);
    lv_label_set_text(ok_lbl, ui_lang_str(STR_HELP_GOT_IT));
    lv_obj_set_style_text_font(ok_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(ok_lbl);
}

void ui_show_single_param_help(uint8_t param_id, ui_str_key_t key)
{
    buzzer_beep_click();
    if (key >= STR_COUNT && param_id > 0) {
        key = stepper_param_to_key(param_id);
    }

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    const char *title = NULL;
    const char *content = NULL;

    switch (key) {
    /* ==================== F1: TIMERS ==================== */
    case STR_TMR_PADDLE:
        title = is_fa ? "تنظیم کارکرد پارو سیلندر" : (is_tr ? "Silindir Palet Calisma Suresi" : "Cylinder Paddle Run Time");
        content = is_fa ?
            "• وظیفه و نحوه عملکرد پارو:\n"
            "مدت زمان چرخش الکتروموتور گیربکس پارو درون سیلندر در فرآیند تولید بستنی قیفی. تیغه‌های پارو با تراشیدن مداوم لایه منجمد شده از روی جداره استیل سیلندر و تزریق هوای فشرده (Overrun)، باعث تشکیل بافت خامه‌ای، لطیف و بدون کریستال یخ می‌شوند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۱۵ دقیقه است (بازه مجاز: ۱۰ الی ۲۰ دقیقه). در مایه با چربی بالا عدد کمتر و برای مایه رقیق‌تر عدد بیشتر مناسب است.\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۱۰ دقیقه باعث آبکی و شل ماندن بستنی شده و قوام قیف گرفتن ایجاد نمی‌شود؛ تنظیم بالای ۲۰ دقیقه موجب اصطکاک بیهوده، گرم شدن مایه و استهلاک زودهنگام تیغه‌ها و گیربکس می‌گردد.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "توقف اصلی کمپرسور در تولید بر اساس سفتی بستنی با سنسور کلاچ عقب انجام می‌شود؛ این تایمر سقف زمانی مجاز چرخش پارو است."
            : (is_tr ?
            "• Fonksiyon ve Calisma Prensibi:\nSilindir palet motorunun dondurma uretimindeki calisma suresidir. Karisimi havalandirip krema kivami verir.\n\n• Fabrika Standardi:\nOnerilen deger 15 dakikadir (10-20 dk arasi).\n\n• Hatali Ayar Sonuclari:\n10 dk alti sulu birakir, 20 dk ustu surtunme ile karisimi isitir.\n\n• Teknisyen Notu:\nKompresor arka debriyaj switchi ile durur; bu zamanlayici azami palet suresidir."
            :
            "• Function & Operation:\nCylinder paddle motor runtime during ice cream production. Scrapes walls and incorporates air for smooth texture.\n\n• Factory Default:\nRecommended is 15 minutes (range: 10 to 20 min).\n\n• Incorrect Setting Impact:\nBelow 10 min leaves mix too soft; above 20 min creates unnecessary friction and gearbox wear.\n\n• Technician Tip:\nMain cooling stop is controlled by rear clutch switch; this timer is the paddle runtime safety window.");
        break;

    case STR_TMR_COMP_RST:
        title = is_fa ? "تاخیر استارت نرم کمپرسور بزرگ" : (is_tr ? "Ana Kompresor Baslama Gecikmesi" : "Compressor Soft-Start Delay");
        content = is_fa ?
            "• هدف ایمنی و عملکرد تاخیر:\n"
            "فاصله زمانی بین روشن شدن موتور پارو تا راه‌اندازی کمپرسور برودت سیلندر. با زدن کلید تولید، ابتدا پارو روشن می‌شود تا مایه در سیلندر جریان یافته و نرم شود؛ سپس کمپرسور بزرگ استارت می‌زند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۴ الی ۵ ثانیه است (بازه مجاز: ۳ الی ۶۰ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۳ ثانیه باعث استارت همزمان دو موتور سنگین، افت ولتاژ شدید شبکه برق و پریدن فیوز مینیاتوری مغازه می‌شود؛ تنظیم بالای ۳۰ ثانیه زمان آماده‌سازی اولیه را بی‌مورد طولانی می‌کند.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "اگر برق کارگاه دارای نوسان یا ژنراتور کم‌توان است، این زمان را روی ۸ الی ۱۰ ثانیه قرار دهید تا استارت پلکانی و بدون شوک جریانی انجام شود."
            : (is_tr ?
            "• Emniyet ve Calisma Gecikmesi:\nPalet motoru ile buyuk kompresor arasindaki baslama gecikmesidir. Ani elektrik yuklenmesini onler.\n\n• Fabrika Standardi:\nOnerilen deger 4-5 saniyedir (3-60 sn arasi).\n\n• Hatali Ayar Sonuclari:\n3 sn alti sigorta attirabilir; 30 sn ustu uretimi gereksiz geciktirir.\n\n• Teknisyen Notu:\nZayif sebekelerde motorlarin sirayla devreye girmesi icin 8-10 saniye onerilir."
            :
            "• Safety Purpose & Function:\nStartup delay between paddle motor and main cylinder compressor, preventing high inrush current.\n\n• Factory Default:\nRecommended is 4 to 5 seconds (range: 3 to 60 sec).\n\n• Incorrect Setting Impact:\nUnder 3s may trip circuit breakers due to dual motor surge; over 30s causes unnecessary delay.\n\n• Technician Tip:\nIncrease to 8-10s if operating on weak workshop wiring or backup generators.");
        break;

    case STR_TMR_EYE:
        title = is_fa ? "مهلت سنسور دور پولی گیربکس" : (is_tr ? "Kasnak Sensoru Zaman Asimi" : "Pulley Sensor Stall Timeout");
        content = is_fa ?
            "• عملکرد حفاظت ضد سوختن موتور:\n"
            "سنسور القایی چرخش پولی گیربکس پارو را لحظه به لحظه رصد می‌کند. اگر موتور پارو روشن باشد اما در طول این مدت پالسی از سنسور نرسد، کنترلر فورا فرمان قطع صادر کرده و خطای توقف پولی (E01) ثبت می‌کند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۴ ثانیه است (بازه مجاز: ۳ الی ۶ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۳ ثانیه در لحظه شتاب‌گیری اولیه پارو خطای کاذب می‌دهد؛ تنظیم بالای ۶ ثانیه در صورت پاره شدن تسمه یا قفل گیربکس باعث داغ شدن شدید و سوختن سیم‌پیچ الکتروموتور می‌شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "در صورت بروز خطای ۰۱، فاصله نوک سنسور القایی تا زائده فلزی پولی (حدود ۲ میلی‌متر) و سلامت و کشش تسمه انتقال قدرت را بررسی فرمایید."
            : (is_tr ?
            "• Motor Yanma Korumasi:\nKasnak enduktif sensorunden sinyal gelmezse motoru durdurarak E01 arizasi verir.\n\n• Fabrika Standardi:\nOnerilen deger 4 saniyedir (3-6 sn arasi).\n\n• Hatali Ayar Sonuclari:\n3 sn alti kalkista yalanci hata verebilir; 6 sn ustu kilitlenmede motoru yakabilir.\n\n• Teknisyen Notu:\nE01 hatasinda kayis gerginligini ve 2 mm sensor boslugunu kontrol edin."
            :
            "• Motor Stall Protection:\nInductive sensor continuously monitors gearbox pulley rotation. Trips fault E01 if pulses stop while powered.\n\n• Factory Default:\nRecommended is 4 seconds (range: 3 to 6 sec).\n\n• Incorrect Setting Impact:\nBelow 3s causes false trips during spin-up; above 6s risks motor burnout if gearbox jams.\n\n• Technician Tip:\nInspect 2mm sensor gap and belt tension if code E01 appears.");
        break;

    case STR_T1_FALLBACK_ON:
        title = is_fa ? "زمان کارکرد همزن مخزن شیر" : (is_tr ? "Sut Haznesi Palet Calisma Suresi" : "Hopper Paddle Run Duration");
        content = is_fa ?
            "• عملکرد و یکنواختی مایه شیر:\n"
            "مدت زمان چرخش پروانه آرام بالای مخزن شیر در هر نوبت. این همزن مایه شیر را مخلوط می‌کند تا دمای بن‌ماری یکدست مانده و چربی، خامه و شیره شکر ته‌نشین یا دوفاز نشوند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۲۰ ثانیه است (بازه مجاز: ۱۰ الی ۶۰ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۱۰ ثانیه باعث ته‌نشینی چربی در کف مخزن و ایجاد لایه یخ روی شیر می‌شود؛ تنظیم بالای ۶۰ ثانیه باعث کف کردن مایه بستنی و گرم شدن شیر می‌گردد.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "اختلاط متناوب بهترین کیفیت شیر را تضمین کرده و از استهلاک موتور همزن جلوگیری می‌نماید."
            : (is_tr ?
            "• Sut Karisimi Homojenligi:\nUst hazne paletinin calisma suresidir. Yag ve sekerin dibe cokmesini onler.\n\n• Fabrika Standardi:\nOnerilen deger 20 saniyedir (10-60 sn arasi).\n\n• Hatali Ayar Sonuclari:\n10 sn alti yag ayrismasina, 60 sn ustu sutun kopurmesine yol acar.\n\n• Teknisyen Notu:\nPeriyodik karistirma sut kalitesini en ust seviyede tutar."
            :
            "• Mix Homogeneity & Preservation:\nGentle agitation cycle for the top hopper mix, preventing fat and sugar stratification.\n\n• Factory Default:\nRecommended is 20 seconds (range: 10 to 60 sec).\n\n• Incorrect Setting Impact:\nUnder 10s causes cream separation; over 60s generates excessive foam and heat.\n\n• Technician Tip:\nIntermittent cycling maintains ideal mix consistency and protects the small stirrer motor.");
        break;

    case STR_T1_FALLBACK_OFF:
        title = is_fa ? "زمان استراحت همزن مخزن شیر" : (is_tr ? "Sut Haznesi Palet Bekleme Suresi" : "Hopper Paddle Rest Duration");
        content = is_fa ?
            "• عملکرد و جلوگیری از گرم شدن شیر:\n"
            "مدت زمان خاموش ماندن همزن مخزن بن‌ماری بین دو نوبت اختلاط شیر. این وقفه مانع از گرم شدن مایه بستنی در اثر اصطکاک مداوم پروانه می‌شود.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۵ دقیقه است (بازه مجاز: ۲ الی ۱۵ دقیقه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۲ دقیقه همزن را مکرراً روشن کرده و دمای شیر را بالا می‌برد؛ تنظیم بالای ۱۵ دقیقه باعث دوفاز شدن خامه و افت کیفیت مایه ورودی به سیلندر می‌شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "برای مایه‌های پرچرب یا بستنی سنتی زعفرانی، عدد ۳ الی ۵ دقیقه بهترین کارایی را دارد."
            : (is_tr ?
            "• Hazne Palet Dinlenme:\nPalet calismalari arasindaki bekleme suresidir.\n\n• Fabrika Standardi:\nOnerilen deger 5 dakikadir (2-15 dk arasi).\n\n• Hatali Ayar Sonuclari:\n2 dk alti sutu isitir, 15 dk ustu karisimi bozar.\n\n• Teknisyen Notu:\nYagli karisimlar icin 3-5 dakika idealdir."
            :
            "• Rest Interval Between Agitations:\nPause duration between hopper mixing cycles to prevent thermal heating from mechanical friction.\n\n• Factory Default:\nRecommended is 5 minutes (range: 2 to 15 min).\n\n• Incorrect Setting Impact:\nBelow 2 min warms mix; above 15 min causes uneven viscosity entering the cylinder.\n\n• Technician Tip:\nUse 3 to 5 minutes for high-fat artisanal ice cream recipes.");
        break;

    case STR_COMP_FALLBACK_ON:
        title = is_fa ? "زمان سرمایش اضطراری مخزن بدون سنسور" : (is_tr ? "Sensorsiz Acil Hazne Sogutma Suresi" : "Emergency Hopper Cooling Run Time");
        content = is_fa ?
            "• عملکرد نجات شیر در خرابی سنسور:\n"
            "اگر سنسور دمای مخزن T1 قطع شود (خطای E04)، دستگاه به جای توقف کامل، طبق این تایمر نگهدارنده را روشن می‌کند تا مایه شیر فاسد و ترش نشود.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۳ دقیقه است (بازه مجاز: ۱ الی ۱۰ دقیقه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۲ دقیقه برودت ناکافی ایجاد کرده و شیر ترش می‌شود؛ تنظیم بالای ۱۰ دقیقه باعث یخ زدن کامل شیر و مسدود شدن مجرای تغذیه سیلندر می‌گردد.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "این حالت فقط هنگام خرابی پراب دما فعال است؛ در حالت عادی ترموستات هوشمند مخزن دما را کنترل می‌کند."
            : (is_tr ?
            "• Acil Durum Sogutma:\nT1 sensoru arizalandiginda sutun bozulmamasi icin calisma suresidir.\n\n• Fabrika Standardi:\nOnerilen deger 3 dakikadir (1-10 dk arasi).\n\n• Hatali Ayar Sonuclari:\n2 dk alti sutu eksitir, 10 dk ustu sutu dondurur.\n\n• Teknisyen Notu:\nSadece sensor kopukken calisir; normalde termostat kontrol eder."
            :
            "• Sensorless Failsafe Hopper Cooling:\nRuns small compressor periodically if hopper sensor T1 fails (fault E04) so milk never spoils.\n\n• Factory Default:\nRecommended is 3 minutes (range: 1 to 10 min).\n\n• Incorrect Setting Impact:\nUnder 2 min risks sour milk; over 10 min causes hopper mix to freeze solid.\n\n• Technician Tip:\nOnly active when probe is disconnected. Replace probe T1 to resume thermostat mode.");
        break;

    case STR_COMP_FALLBACK_OFF:
        title = is_fa ? "زمان استراحت اضطراری مخزن بدون سنسور" : (is_tr ? "Sensorsiz Acil Hazne Dinlenme Suresi" : "Emergency Hopper Cooling Rest Time");
        content = is_fa ?
            "• عملکرد تعدیل برودت بدون سنسور:\n"
            "مدت زمان خاموش ماندن کمپرسور نگهدارنده در وضعیت قطعی سنسور دما بین هر سیکل خنک‌کاری تا سرمای مخزن تعدیل شده و لوله‌ها قندیل نبندند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۵ الی ۱۰ دقیقه است (بازه مجاز: ۳ الی ۳۰ دقیقه - پیش‌فرض: ۵ دقیقه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۳ دقیقه باعث یخ زدن شیر در کف بن‌ماری می‌شود؛ تنظیم بالای ۱۵ دقیقه در گرمای تابستان شیر را گرم و فاسد می‌کند.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "در اولین فرصت سوکت و کابل سنسور دمای T1 را بررسی یا تعویض کنید تا دستگاه به حالت خودکار بازگردد."
            : (is_tr ?
            "• Acil Durum Bekleme:\nSensorsiz modda sogutma durma araligidir.\n\n• Fabrika Standardi:\nOnerilen deger 5 dakikadir (3-30 dk arasi).\n\n• Hatali Ayar Sonuclari:\n3 dk alti dondurur, 15 dk ustu sutu bozar.\n\n• Teknisyen Notu:\nEn kisa surede T1 sensor kablosunu kontrol edin."
            :
            "• Sensorless Failsafe Rest Interval:\nPause between emergency cooling cycles to prevent evaporator coil freezing.\n\n• Factory Default:\nRecommended is 5 minutes (range: 3 to 30 min).\n\n• Incorrect Setting Impact:\nUnder 3 min freezes mix; over 15 min leads to mix warming in high ambient temperatures.\n\n• Technician Tip:\nCheck and restore sensor T1 probe cable at the first opportunity.");
        break;

    /* ==================== F3: HOLDING & COOLING ==================== */
    case STR_COMP_CYCLE:
        title = is_fa ? "دوره استراحت بازرسی خودکار سیلندر" : (is_tr ? "Silindir Otomatik Kontrol Dinlenme Suresi" : "Cylinder Auto-Check Rest Cycle");
        content = is_fa ?
            "• عملکرد و پایش خودکار قوام بستنی:\n"
            "در حالت آماده‌باش یا نگهداری شبانه، این تایمر وقفه زمانی بین بررسی‌های خودکار بافت بستنی است. پس از پایان این زمان، دستگاه سفتی بستنی را تست می‌کند: اگر میکروسوئیچ کلاچ عقب فشرده باشد (بستنی سفت است)، فقط موتور پارو ۱۰ ثانیه می‌چرخد تا بافت بستنی تازه بماند و کمپرسور خاموش می‌ماند؛ در صورت شل شدن بستنی، کمپرسور بزرگ روشن می‌شود تا برودت را احیا کند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۱۲۰ دقیقه (۲ ساعت) است (بازه مجاز: ۳۰ الی ۲۴۰ دقیقه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۳۰ دقیقه باعث روشن/خاموش شدن مکرر کمپرسور در ساعات خلوتی و افزایش شدید مصرف برق می‌شود؛ تنظیم بالای ۱۸۰ دقیقه باعث آب شدن بستنی داخل سیلندر و شل بودن اولین قیف مشتری می‌گردد.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "در تابستان‌های بسیار گرم این عدد را روی ۶۰ الی ۹۰ دقیقه بگذارید؛ در زمستان روی ۱۲۰ الی ۱۵۰ دقیقه ایده‌آل است."
            : (is_tr ?
            "• Otomatik Sertlik Kontrol Araligi:\nBekleme modunda dondurma kivamini periyodik kontrol eder. Sertse sadece 10 sn palet calisir; yumusamissa kompresor devreye girer.\n\n• Fabrika Standardi:\nOnerilen deger 120 dakikadir (30-240 dk arasi).\n\n• Hatali Ayar Sonuclari:\n30 dk alti elektrik tuketimini artirir; 180 dk ustu dondurmayi eritir.\n\n• Teknisyen Notu:\nYazin 60-90 dk, kisin 120-150 dk idealdir."
            :
            "• Standby Consistency Auto-Check:\nInterval between automatic firmness tests in Ready mode. If firm, only paddle runs for 10s; if soft, cooling restarts.\n\n• Factory Default:\nRecommended is 120 minutes (range: 30 to 240 min).\n\n• Incorrect Setting Impact:\nUnder 30 min causes excessive compressor starts; over 180 min softens mix in quiet hours.\n\n• Technician Tip:\nSet to 60-90 min during hot summer seasons; 120-150 min in winter.");
        break;

    case STR_F5_MAIN_COMP:
        title = is_fa ? "سقف زمان کارکرد کمپرسور در نگهداری" : (is_tr ? "Bekleme Modu Azami Kompresor Suresi" : "Max Comp Run Time in Holding Mode");
        content = is_fa ?
            "• عملکرد و کنترل هوشمند کلاچ:\n"
            "حداکثر زمان مجاز روشن ماندن کمپرسور سیلندر در فاز نگهداری یا استندبای. اگر روی 0 تنظیم شود، توقف کمپرسور کاملاً هوشمندانه بر عهده میکروسوئیچ کلاچ عقب (سفت شدن بستنی) یا ترموکوپل سیلندر خواهد بود.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۰ (کنترل هوشمند خودکار با کلاچ) است (بازه مجاز: ۰ الی ۱۵ دقیقه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم روی عدد نامناسب بدون نیاز ممکن است قبل از سفت شدن بستنی کمپرسور را خاموش کند یا موجب برودت اضافی شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "بهترین گزینه عدد 0 است تا سنسور کلاچ مکانیکی سفتی واقعی بستنی را بسنجد و کمترین استهلاک به سیستم وارد شود."
            : (is_tr ?
            "• Bekleme Azami Sogutma Suresi:\n0 yapildiginda durdurma kontrolu arka debriyaj switchine birakilir.\n\n• Fabrika Standardi:\nOnerilen deger 0 dir (otomatik kontrol).\n\n• Hatali Ayar Sonuclari:\nYanlis deger dondurmayi tam kivam almadan durdurabilir.\n\n• Teknisyen Notu:\n0 degeri mekanik debriyaja gore en dengeli calismayi saglar."
            :
            "• Holding Mode Compressor Run Ceiling:\nSafety run limit per holding cooling cycle. Setting to 0 uses automatic clutch microswitch control.\n\n• Factory Default:\nRecommended is 0 (automatic clutch cutoff).\n\n• Incorrect Setting Impact:\nNon-zero limits may shut off refrigeration before ideal firmness is achieved.\n\n• Technician Tip:\nKeep at 0 for automatic adaptive mechanical clutch operation.");
        break;

    case STR_HOLD_DELAY:
        title = is_fa ? "تاخیر استارت سرمایش در حالت نگهداری" : (is_tr ? "Bekleme Modu Yumusak Baslama Gecikmesi" : "Holding Mode Soft-Start Delay");
        content = is_fa ?
            "• عملکرد و راه‌اندازی نرم در نگهداری:\n"
            "فاصله زمانی چرخش اولیه پارو پیش از روشن شدن کمپرسور سیلندر هنگام شارژ سرمای دوره‌ای در حالت نگهداری. این تاخیر بستنی را نرم کرده و مانع از استارت کمپرسور روی بار سنگین می‌شود.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۳۰ ثانیه است (بازه مجاز: ۱۰ الی ۶۰ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم زیر ۱۰ ثانیه شوک ضربه‌ای به گیربکس وارد می‌کند؛ تنظیم بالای ۶۰ ثانیه چرخش بی‌مورد پارو در حالت نگهداری ایجاد می‌کند.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "این تاخیر برای سلامت گیربکس و کاهش نوسان برق کارگاه ضروری است و تغییر آن به ندرت نیاز می‌شود."
            : (is_tr ?
            "• Beklemede Yumusak Baslama:\nBekleme modunda kompresor baslamadan once paletin donus suresidir.\n\n• Fabrika Standardi:\nOnerilen deger 30 saniyedir (10-60 sn arasi).\n\n• Hatali Ayar Sonuclari:\n10 sn alti disli kutusuna darbe bindirir; 60 sn ustu gereksiz calismadir.\n\n• Teknisyen Notu:\nMekanik omru korumak icin fabrika ayarinda birakin."
            :
            "• Holding Cycle Paddle Pre-Run:\nPre-rotation delay softening ice cream before compressor engages during maintenance cycles.\n\n• Factory Default:\nRecommended is 30 seconds (range: 10 to 60 sec).\n\n• Incorrect Setting Impact:\nBelow 10s subjects gearbox to high torque shock; above 60s is redundant runtime.\n\n• Technician Tip:\nCritical for mechanical protection; changing factory value is rarely required.");
        break;

    case STR_TEMP_T1_ON:
        title = is_fa ? "دمای استارت سرمایش مخزن شیر" : (is_tr ? "Sut Tanki Sogutma Baslatma Sicakligi" : "Milk Tank Cooling Start Temp");
        content = is_fa ?
            "• عملکرد ترموستات بهداشتی مخزن شیر:\n"
            "حد بالای دمای مجاز مخزن بن‌ماری. هرگاه دمای مایه شیر به این عدد برسد، کمپرسور کوچک فوراً روشن می‌شود تا از بالا رفتن دمای شیر و رشد باکتری جلوگیری کند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۴.۰+ درجه سانتی‌گراد است (بازه مجاز: ۳+ الی ۸+ درجه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم بالاتر از ۶ درجه خط قرمز بهداشت است و شیر ترش و فاسد می‌شود؛ تنظیم کمتر از ۳ درجه فاصله قطع و وصل ترموستات را بسیار کم کرده و کمپرسور مکرراً استارت می‌زند.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "استاندارد جهانی سلامت شیر زیر ۴ درجه سانتی‌گراد است؛ در فصول گرم سال این تنظیم را تغییر ندهید."
            : (is_tr ?
            "• Hazne Termostat Baslama Sicakligi:\nSut haznesi bu sicakliga ulastiginda sogutma baslar.\n\n• Fabrika Standardi:\nOnerilen deger +4.0°C dir (+3 ile +8 arasi).\n\n• Hatali Ayar Sonuclari:\n+6°C ustu sutu bozar; +3°C alti kompresoru sik sik calistirir.\n\n• Teknisyen Notu:\nHijyen standartlari icin daima 4 derecenin altinda tutun."
            :
            "• Hopper Thermostat Upper Setpoint:\nRefrigeration starts automatically when hopper mix reaches this temperature to preserve freshness.\n\n• Factory Default:\nRecommended is +4.0°C (range: +3 to +8°C).\n\n• Incorrect Setting Impact:\nAbove +6°C promotes bacterial growth and spoils milk; below +3°C causes short-cycling.\n\n• Technician Tip:\nKeep at +4.0°C to comply with sanitary dairy preservation standards.");
        break;

    case STR_TEMP_T1_OFF:
        title = is_fa ? "دمای استپ سرمایش مخزن شیر" : (is_tr ? "Sut Tanki Sogutma Durdurma Sicakligi" : "Milk Tank Cooling Stop Temp");
        content = is_fa ?
            "• عملکرد ضد یخ‌زدگی مخزن شیر:\n"
            "حد پایین دمای مخزن بن‌ماری. با رسیدن دمای شیر به این عدد، سرمایش مخزن قطع می‌شود تا از یخ بستن شیر در کف مخزن جلوگیری شود.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۲.۰+ درجه سانتی‌گراد است (بازه مجاز: ۰ الی ۳+ درجه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم کمتر از ۰ درجه باعث قندیل بستن شیر، گیر کردن پروانه همزن و مسدود شدن مجرای ورود شیر به سیلندر می‌شود؛ تنظیم بیشتر از ۴ درجه شیر را در معرض فساد قرار می‌دهد.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "اختلاف ۲ درجه بین استارت (۴°) و استپ (۲°) هیسترزیس ایده‌آل برای جلوگیری از استهلاک کمپرسور است."
            : (is_tr ?
            "• Hazne Termostat Durma Sicakligi:\nSogutma bu sicakliga inince durur.\n\n• Fabrika Standardi:\nOnerilen deger +2.0°C dir (0 ile +3 arasi).\n\n• Hatali Ayar Sonuclari:\n0°C alti sutu dondurur ve boruyu tikar; +4°C ustu sogutmayi yetersiz birakir.\n\n• Teknisyen Notu:\n2 derecelik histerezis kompresor sagligi icin idealdir."
            :
            "• Hopper Thermostat Lower Cut-Out:\nRefrigeration stops when hopper mix cools down to this setpoint to avoid mix freezing.\n\n• Factory Default:\nRecommended is +2.0°C (range: 0 to +3°C).\n\n• Incorrect Setting Impact:\nBelow 0°C freezes mix, blocks feed tube, and stalls impeller; above +4°C reduces chilling.\n\n• Technician Tip:\nA 2°C hysteresis between start (+4°C) and stop (+2°C) optimizes compressor longevity.");
        break;

    /* ==================== F6: DISPENSE & SAFETY ==================== */
    case STR_TMR_DSP_RUN:
        title = is_fa ? "زمان شارژ برودت پس از کشیدن قیف" : (is_tr ? "Servis Sonrasi Hizli Sogutma Suresi" : "Post-Dispense Rechill Run Time");
        content = is_fa ?
            "• عملکرد و بازیابی قوام بستنی:\n"
            "مدت زمانی که پس از رها شدن اهرم و پایان کشیدن بستنی، کمپرسور بزرگ روشن می‌ماند تا مایه گرم جدیدی که از مخزن بالا وارد سیلندر شده را سریعاً خنک و منجمد کند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۴۵ ثانیه است (بازه مجاز: ۱۰ الی ۶۰ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم کمتر از ۲۰ ثانیه در ساعات شلوغی با فروش پشت سر هم باعث شل و آبکی شدن بستنی‌های بعدی می‌شود؛ تنظیم بیشتر از ۶۰ ثانیه بستنی را بیش از حد سفت کرده و کشیدن اهرم را دشوار می‌سازد.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "در روزهای شلوغ و داغ تابستان اگر بستنی پس از چند قیف شل شد، این عدد را تا ۵۵ یا ۶۰ ثانیه افزایش دهید."
            : (is_tr ?
            "• Servis Sonrasi Kivam Koruma:\nKulah cekildikten sonra hazneye giren ilik sutu dondurmak icin kompresorun calisma suresidir.\n\n• Fabrika Standardi:\nOnerilen deger 45 saniyedir (10-60 sn arasi).\n\n• Hatali Ayar Sonuclari:\n20 sn alti seri satista dondurmayi ciritir; 60 sn ustu kolu asiri sertlestirir.\n\n• Teknisyen Notu:\nYazin cok yogun saatlerde 55-60 saniyeye cikarabilirsiniz."
            :
            "• Post-Dispense Quick Rechill:\nCylinder compressor runtime after lever release, rapidly freezing incoming warm mix from the hopper.\n\n• Factory Default:\nRecommended is 45 seconds (range: 10 to 60 sec).\n\n• Incorrect Setting Impact:\nBelow 20s leads to watery cones during rushes; above 60s over-hardens texture.\n\n• Technician Tip:\nIncrease to 55-60s on peak summer days to maintain firmness across back-to-back dispenses.");
        break;

    case STR_TMR_FRT_HOLD:
        title = is_fa ? "زمان نگهداشت اهرم در فروش حجیم" : (is_tr ? "Toptan Satis Kol Basili Tutma Suresi" : "Bulk Dispense Lever Hold Duration");
        content = is_fa ?
            "• عملکرد شارژ سریع حین تخلیه مداوم:\n"
            "اگر کاربر اهرم را به طور پیوسته برای پر کردن ظروف بزرگ یا چند قیف متوالی پایین نگه دارد، با عبور از این زمان کمپرسور بزرگ سریعاً حین خروج بستنی وارد مدار می‌شود تا سیلندر دچار افت سرما نشود.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۶ ثانیه است (بازه مجاز: ۲ الی ۱۵ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم کمتر از ۳ ثانیه برای هر قیف معمولی کمپرسور را روشن می‌کند و استهلاک بوبین را بالا می‌برد؛ تنظیم بیشتر از ۱۲ ثانیه باعث آبکی شدن انتهای بستنی ظرف بزرگ می‌شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "مقدار ۶ ثانیه زمان استاندارد برای یک قیف است؛ اگر بسته‌بندی کیلویی زیاد دارید، روی ۵ ثانیه قرار دهید."
            : (is_tr ?
            "• Kilo ile Satista Hizli Sogutma:\nKol uzun sure basili tutulursa sogutmanin aninda devreye girme suresidir.\n\n• Fabrika Standardi:\nOnerilen deger 6 saniyedir (2-15 sn arasi).\n\n• Hatali Ayar Sonuclari:\n3 sn alti tek kulahlarda bile kompresoru yorar; 12 sn ustu kabi sulu doldurur.\n\n• Teknisyen Notu:\nKilo ile satis yogunsa 5 saniyeye ayarlayin."
            :
            "• Bulk Container Dispense Protection:\nContinuous lever pull duration that triggers immediate cooling during bulk takeout dispenses.\n\n• Factory Default:\nRecommended is 6 seconds (range: 2 to 15 sec).\n\n• Incorrect Setting Impact:\nUnder 3s short-cycles on single cones; over 12s softens mix in bulk containers.\n\n• Technician Tip:\nSetting to 5-6s provides prompt temperature recovery when filling large tubs.");
        break;

    case STR_CNT_SCP_COMP:
        title = is_fa ? "تعداد قیف کشیده شده تا شارژ سرما" : (is_tr ? "Sogutma Icin Kulah Sayisi Limiti" : "Scoop Count Trigger for Rechill");
        content = is_fa ?
            "• عملکرد کاهش استهلاک کنتاکتورها:\n"
            "تعداد قیف‌های فروخته شده پیش از استارت مجدد کمپرسور بزرگ. به جای روشن شدن مکرر کمپرسور با هر یک قیف، سیستم پس از این تعداد قیف، یک سیکل شارژ سرما اجرا می‌کند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۳ قیف است (بازه مجاز: ۱ الی ۱۰ قیف).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم روی ۱ باعث استهلاک شدید رله‌ها و کنتاکتور برق کمپرسور می‌شود؛ بیشتر از ۵ قیف باعث افت قوام بستنی در قیف‌های بعدی می‌شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "عدد ۳ قیف تعادل بی‌نظیری بین حفظ بافت بستنی و طول عمر قطعات برقی ایجاد می‌کند."
            : (is_tr ?
            "• Kontak Omru Koruma Sayaci:\nKac kulah satisindan sonra kompresorun devreye girecegini belirler.\n\n• Fabrika Standardi:\nOnerilen deger 3 kulahtir (1-10 arasi).\n\n• Hatali Ayar Sonuclari:\n1 ayari kontaktorleri erken yipratir; 5 ustu son kulahlari yumusatir.\n\n• Teknisyen Notu:\n3 kulah ayari makine omru ve kalite acisindan en ideal dengedir."
            :
            "• Dispense Counter Cooling Threshold:\nNumber of dispensed cones before scheduling a cooling cycle, avoiding excessive compressor on/off cycling.\n\n• Factory Default:\nRecommended is 3 cones (range: 1 to 10 cones).\n\n• Incorrect Setting Impact:\nSetting to 1 causes excessive wear on contactors; above 5 softens consecutive cones.\n\n• Technician Tip:\n3 scoops strikes the perfect balance between motor health and serving texture.");
        break;

    case STR_TMR_FRZ_TO:
        title = is_fa ? "حداکثر زمان مجاز انجماد سیلندر" : (is_tr ? "Azami Dondurma Guvenlik Sure Siniri" : "Maximum Freeze Safety Cutoff");
        content = is_fa ?
            "• عملکرد حفاظت اضطراری کمپرسور:\n"
            "سقف ایمنی روشن ماندن مداوم کمپرسور بزرگ در یک سیکل تولید. اگر به دلیل نشت گاز، خرابی کلاچ عقب یا نقص فنی بستنی سفت نشود، کنترلر برای نجات موتور پس از این زمان برق را قطع کرده و آلارم می‌دهد.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۱۵ دقیقه است (بازه مجاز: ۵ الی ۳۰ دقیقه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم کمتر از ۸ دقیقه در بار اول شیر گرم بیهوده آلارم می‌دهد؛ تنظیم بیشتر از ۲۰ دقیقه در صورت کمبود گاز باعث داغ شدن شدید و سوختن سیم‌پیچ کمپرسور می‌شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "اگر دستگاه پس از ۱۵ دقیقه این خطا را داد، گاز دستگاه، سلامت فن خنک‌کننده رادیاتور و سفتی تسمه را بررسی فرمایید."
            : (is_tr ?
            "• Kompresor Guvenlik Sure Siniri:\nGaz eksikligi veya mekanik arizada kompresorun durmaksizin calismasini onler.\n\n• Fabrika Standardi:\nOnerilen deger 15 dakikadir (5-30 dk arasi).\n\n• Hatali Ayar Sonuclari:\n8 dk alti ilk dolumda erken hata verir; 20 dk ustu motoru yakabilir.\n\n• Teknisyen Notu:\n15 dk da uyari aliyorsaniz gazi, radyator fanini ve kayisi kontrol edin."
            :
            "• Maximum Freezing Duration Cutoff:\nEmergency ceiling shutting off big compressor if ice cream does not harden within this window.\n\n• Factory Default:\nRecommended is 15 minutes (range: 5 to 30 min).\n\n• Incorrect Setting Impact:\nBelow 8 min trips prematurely on warm mix; above 20 min risks burning out compressor if refrigerant is low.\n\n• Technician Tip:\nIf freeze timeout trips, check refrigerant charge, condenser fan, and drive belt.");
        break;

    case STR_TMR_LEV_MAX:
        title = is_fa ? "سقف زمان هشدار باز ماندن اهرم جلو" : (is_tr ? "Kol Acik Kalma Alarm Zaman Asimi" : "Stuck Dispense Lever Alarm Timeout");
        content = is_fa ?
            "• عملکرد جلوگیری از هدررفت بستنی:\n"
            "حداکثر زمان مجاز باز ماندن دسته تخلیه پیش از فعال شدن بوق هشدار ممتد و ثبت خطای E08. از خالی شدن مایه مخزن روی زمین بر اثر گیر کردن دسته یا بی‌دقتی اپراتور جلوگیری می‌کند.\n\n"
            "• تنظیم استاندارد کارخانه و بازه مجاز:\n"
            "مقدار پیشنهادی کارخانه ۲۵ ثانیه است (بازه مجاز: ۱۵ الی ۱۲۰ ثانیه).\n\n"
            "• اثر تنظیم نادرست و عیب‌یابی:\n"
            "تنظیم کمتر از ۱۵ ثانیه هنگام شستشو یا کشیدن ظروف بزرگ آلارم کاذب می‌دهد؛ تنظیم بیشتر از ۶۰ ثانیه در صورت گیر کردن اهرم باعث هدر رفتن حجم زیادی از مایه بستنی می‌شود.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "۲۵ ثانیه زمان کافی برای کشیدن هر ظرف بزرگی است و ایمنی کامل دستگاه را تضمین می‌نماید."
            : (is_tr ?
            "• Kol Acik Kalma Alarmi:\nServis kolu takili kalirsa sesli uyari verip urunun bosa akmasini onler.\n\n• Fabrika Standardi:\nOnerilen deger 25 saniyedir (15-120 sn arasi).\n\n• Hatali Ayar Sonuclari:\n15 sn alti temizlikte erken calar; 60 sn ustu cok urun ziyan eder.\n\n• Teknisyen Notu:\n25 saniye her turlu servis icin guvenli ve yeterlidir."
            :
            "• Dispensing Lever Jam Alarm:\nAudible buzzer and fault E08 trigger if lever remains unclosed, preventing hopper drainage onto the floor.\n\n• Factory Default:\nRecommended is 25 seconds (range: 15 to 120 sec).\n\n• Incorrect Setting Impact:\nUnder 15s alarms prematurely during washing; over 60s wastes product if lever binds.\n\n• Technician Tip:\n25 seconds accommodates large dishes while safeguarding against unattended spills.");
        break;

    /* ==================== F6: FIXED HARDWARE-LOCKED TIMERS ==================== */
    case STR_FIXED_COOLDOWN:
        title = is_fa ? "شروع مجدد کمپرسور (ضد استارت زیر بار)" : (is_tr ? "Kompresor Yeniden Baslatma" : "Compressor Restart Delay");
        content = is_fa ?
            "• عملکرد حفاظت تعادل فشار گاز کمپرسور:\n"
            "مدت زمان لازم پس از خاموش شدن کمپرسور بزرگ تا برقراری تعادل فشار گاز مبرد (Anti-Short Cycle). کنترلر تا سپری شدن این زمان، اجازه استارت مجدد به کمپرسور نمی‌دهد.\n\n"
            "• مقدار استاندارد قفل‌شده صنعتی:\n"
            "مقدار ثابت سخت‌افزاری ۱۰ ثانیه (حفاظت قطعی از استارت زیر فشار).\n\n"
            "• چرا این پارامتر قفل سخت‌افزار است؟\n"
            "استارت زودهنگام کمپرسور در فشار نامتعادل، آمپر هجومی شدید ایجاد کرده و به رله استارت، خازن و سیم‌پیچ موتور آسیب قطعی می‌زند.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "این محافظت سخت‌افزاری ضامن سلامت دائم کمپرسور اصلی دستگاه است."
            : (is_tr ?
            "• Kompresor Gaz Dengeleme:\nKompresor durduktan sonra gaz basincinin dengelenmesi icin gereken sabit suredir.\n\n• Kilitli Standart Deger:\n10 saniye.\n\n• Neden Donanim Kilitli?:\nYuk altinda kalkisi onleyerek motor omrunu korur."
            :
            "• Compressor Anti-Short Cycle Delay:\nMinimum delay required after compressor shutdown to allow refrigerant pressure equalization before restart.\n\n• Factory Locked Value:\n10 seconds fixed hardware standard.\n\n• Why is this Locked?:\nPrevents locked-rotor starting under high head pressure, protecting motor windings and capacitors.\n\n• Technician Tip:\nEnsures maximum compressor lifespan and prevents premature contactor wear.");
        break;

    case STR_FIXED_RESET_PULSE:
        title = is_fa ? "پالس ریست سخت‌افزاری کنترل‌بار" : (is_tr ? "Donanim Yuk Sifirlama Palsi" : "Hardware Load Reset Pulse");
        content = is_fa ?
            "• عملکرد و بازنشانی الکترونیکی رله‌ها:\n"
            "مدت زمان ارسال پالس ولتاژ به بوبین رله ریست (خروجی رله Q6) برای بازنشانی الکترونیکی کنترل‌بار موتور و کمپرسور پس از رفع خطا یا در لحظه روشن شدن دستگاه.\n\n"
            "• مقدار استاندارد قفل‌شده صنعتی:\n"
            "مقدار ثابت سخت‌افزاری ۲ ثانیه (پالس ۵۰۰ میلی‌ثانیه‌ای استاندارد).\n\n"
            "• چرا این پارامتر قفل سخت‌افزار است؟\n"
            "پالس کمتر از ۲۰۰ میلی‌ثانیه ممکن است بوبین ریست مکانیکی را تحریک نکند و پالس بالای ۳ ثانیه سیم‌پیچ رله ریست را داغ می‌کند؛ به همین دلیل در فریمور قفل صنعتی شده است.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "این پالس خودکار به شما امکان می‌دهد بدون نیاز به باز کردن درب تابلو برق، رله‌های اورلود را از روی نمایشگر ریست کنید."
            : (is_tr ?
            "• Role Sifirlama Palsi:\nTermik ve asiri yuk koruma rolelerini uzaktan sifirlayan elektrik palsidir.\n\n• Kilitli Standart Deger:\n2 saniye (500ms aktif pals).\n\n• Neden Donanim Kilitli?:\nBobin omrunu korumak ve guvenli tetikleme saglamak icin degistirilemez.\n\n• Teknisyen Notu:\nPano kapagini acmadan ekran uzerinden hatayi gidermenizi saglar."
            :
            "• Overload Relay Remote Reset Pulse:\nEnergizes the reset relay coil (Output Relay Q6) to electronically clear thermal overload trips.\n\n• Factory Locked Value:\n2 seconds total cycle (500ms active pulse).\n\n• Why is this Locked?:\nProtects reset coil from thermal burnout while ensuring reliable contact clearing.\n\n• Technician Tip:\nEnables one-touch overload clearing directly from screen without opening electrical panel.");
        break;

    case STR_FIXED_DEBOUNCE:
        title = is_fa ? "فیلتر حذف نویز دیجیتال ورودی‌ها" : (is_tr ? "Giris Gurultu ve Titresim Filtresi" : "Input Noise Filter & Debounce");
        content = is_fa ?
            "• عملکرد ضد نویز و تثبیت سیگنال:\n"
            "فیلتر دیجیتال صنعتی زیمنس روی تمام ورودی‌های سنسورها (میکروسوئیچ اهرم، کلاچ عقب، استپ قارچی و کنترل‌بار). هر سیگنال باید حداقل ۷۰ میلی‌ثانیه پیوسته و پایدار بماند تا کنترلر آن را بپذیرد.\n\n"
            "• مقدار استاندارد قفل‌شده صنعتی:\n"
            "زمان قطعی زیمنس ۷۰ میلی‌ثانیه (Deterministic 70ms).\n\n"
            "• چرا این پارامتر قفل سخت‌افزار است؟\n"
            "جرقه‌های کنتاکتورهای قدرت و نویز الکتروموتورها ممکن است پالس‌های ولتاژ کاذب میلی‌ثانیه‌ای روی کابل‌ها القا کنند. این فیلتر از دستورات اشتباه و خطاهای کاذب ناشی از نویز محیط کارگاه ۱۰۰٪ جلوگیری می‌کند.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "این فیلتر بالاترین ضریب اطمینان را به عملکرد میکروسوئیچ‌ها و سنسورهای دستگاه می‌بخشد."
            : (is_tr ?
            "• Sayisal Gurultu Filtresi:\nGirislerin kararli calismasi icin Siemens standardinda filtre suresidir.\n\n• Kilitli Standart Deger:\n70 milisaniye.\n\n• Neden Donanim Kilitli?:\nKontaktor parazitlerinin sahte tetikleme yapmasini kesin olarak onler.\n\n• Teknisyen Notu:\nEndustriyel gurultulu ortamlarda %100 kararli calisma saglar."
            :
            "• Deterministic Input Noise Debounce:\nFilters mechanical contact bounce and electrical EMI on all limit switches and sensor inputs.\n\n• Factory Locked Value:\n70 milliseconds (Siemens deterministic standard).\n\n• Why is this Locked?:\nPrevents false fault trips caused by electromagnetic noise from contactors and high-power motors.\n\n• Technician Tip:\nGuarantees maximum operational reliability in electrically noisy workshops.");
        break;

    case STR_FIXED_CYL_LIMIT:
        title = is_fa ? "حد برودت انجماد بحرانی سیلندر" : (is_tr ? "Silindir Donma Emniyet Limiti" : "Cylinder Freeze Safety Limit");
        content = is_fa ?
            "• عملکرد حفاظت مکانیکی سیلندر و پارو:\n"
            "اینترلاک سخت‌افزاری محافظت از سیلندر با دماسنج ترموکوپل T2. اگر دمای سیلندر به زیر ۲۲.۰- درجه سانتی‌گراد برسد، کنترلر فوراً کمپرسور بزرگ را قطع مطلق می‌کند و خطای دمایی صادر می‌کند.\n\n"
            "• مقدار استاندارد قفل‌شده صنعتی:\n"
            "آستانه بحرانی ایمنی -۲۲.۰°C.\n\n"
            "• چرا این پارامتر قفل سخت‌افزار است؟\n"
            "سرمایش زیر ۲۲- درجه باعث انجماد سنگ مانند بستنی و قفل شدن پارو در بدنه استیل سیلندر می‌شود که نتیجه آن شکستن تیغه‌های همزن، تاب برداشتن شفت و سوختن الکتروموتور است.\n\n"
            "• نکته طلایی تکنسین آستیاج:\n"
            "این قفل ایمنی ضامن سلامت قطعات مکانیکی گران‌قیمت دستگاه شماست و تحت هیچ شرایطی نباید دور زده شود."
            : (is_tr ?
            "• Silindir Asiri Donma Emniyeti:\nSilindir sicakligi -22°C altina inerse kompresor kilitlenerek mekanik parcalar korunur.\n\n• Kilitli Standart Deger:\n-22.0°C kritik sinir.\n\n• Neden Donanim Kilitli?:\nAsiri donmada palet bicaklarinin kirilmasini ve motorun yanmasini kesin olarak engeller.\n\n• Teknisyen Notu:\nPahali mekanik parcalarin garantisidir, asla devre disi birakilamaz."
            :
            "• Critical Cylinder Low-Temperature Cutoff:\nHardware interlock monitoring probe T2. Immediately shuts off cooling if cylinder temp drops below -22.0°C.\n\n• Factory Locked Value:\n-22.0°C absolute floor limit.\n\n• Why is this Locked?:\nPrevents rock-hard ice lockups that fracture scraper blades and twist drive shafts.\n\n• Technician Tip:\nProtects expensive mechanical parts against thermal over-freezing accidents.");
        break;

    default:
        title = is_fa ? "راهنمای تنظیمات سیستم" : (is_tr ? "Sistem Ayari Rehberi" : "System Setting Guide");
        content = is_fa ?
            "• نحوه تغییر مقدار:\nبا استفاده از کلیدهای منفی (-) و مثبت (+) مقدار این پارامتر را کم یا زیاد کنید.\n\n"
            "• ذخیره تنظیمات:\nپس از اعمال تغییرات، برای ثبت نهایی در حافظه دائم دستگاه، کلید سبز 'ذخیره' را لمس فرمایید."
            : (is_tr ?
            "• Degeri Degistirme:\n(-) ve (+) butonlari ile degeri ayarlayin.\n\n• Kaydetme:\nDegisiklikleri kaydetmek icin Yesil Kaydet tusuna basin."
            :
            "• Adjusting Value:\nUse (-) and (+) buttons to decrease or increase this setting.\n\n• Saving:\nPress the green Save button to persist changes in non-volatile memory.");
        break;
    }

    if (title && content) {
        show_modal_help_dialog(title, content);
    }
}

void hide_lease_unlock_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_lease_backdrop && lv_obj_is_valid(s_lease_backdrop)) {
        lv_obj_delete(s_lease_backdrop);
    }
    s_lease_backdrop = NULL;
    objects.lease_modal = NULL;
    objects.lease_ta_code = NULL;
    lvgl_unlock();
}

void ui_setup_industrial_numpad(lv_obj_t *kb)
{
    if (!kb) return;
    static const char * const numpad_map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
    };
    static const lv_buttonmatrix_ctrl_t numpad_ctrl[] = {
        1, 1, 1,
        1, 1, 1,
        1, 1, 1,
        1, 1, 1
    };
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, numpad_map, numpad_ctrl);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_24, LV_PART_ITEMS);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
}

void show_lease_unlock_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;

    hide_lease_unlock_dialog();

    s_lease_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_lease_backdrop, 0, 0);
    lv_obj_set_size(s_lease_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_lease_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_lease_backdrop, 160, 0);
    lv_obj_set_style_border_width(s_lease_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_lease_backdrop, 0, 0);
    lv_obj_clear_flag(s_lease_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_lease_backdrop);
    objects.lease_modal = card;
    lv_obj_set_pos(card, 24, 12);
    lv_obj_set_size(card, 432, 296);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 432, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(header, 255, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "تمدید روزشمار و ورود کد فعال‌سازی");
    lv_obj_set_style_text_font(title, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, 0);
    lv_obj_center(title);

    lv_obj_t *btn_close = lv_btn_create(header);
    lv_obj_set_pos(btn_close, 396, 3);
    lv_obj_set_size(btn_close, 32, 32);
    lv_obj_set_ext_click_area(btn_close, 8);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, action_close_lease_dialog, LV_EVENT_CLICKED, NULL);
    lv_obj_t *x_lbl = lv_label_create(btn_close);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(x_lbl);

    /* Body info */
    int32_t left_days = lease_days_left();
    int32_t ch_code = lease_get_challenge_code();
    char info_buf[128];
    snprintf(info_buf, sizeof(info_buf), "اعتبار: %ld روز | کد چالش دستگاه: %ld", (long)left_days, (long)ch_code);

    lv_obj_t *info_lbl = lv_label_create(card);
    lv_obj_set_pos(info_lbl, 10, 42);
    lv_obj_set_size(info_lbl, 412, 22);
    lv_label_set_text(info_lbl, info_buf);
    lv_obj_set_style_text_font(info_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(info_lbl, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_text_align(info_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(info_lbl, LV_BASE_DIR_RTL, 0);

    lv_obj_t *hint_lbl = lv_label_create(card);
    lv_obj_set_pos(hint_lbl, 10, 66);
    lv_obj_set_size(hint_lbl, 412, 20);
    lv_label_set_text(hint_lbl, "کد تمدید، رمز چالش یا کد اضطراری (112233) را وارد کنید:");
    lv_obj_set_style_text_font(hint_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(hint_lbl, lv_color_hex(0x475569), 0);
    lv_obj_set_style_text_align(hint_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(hint_lbl, LV_BASE_DIR_RTL, 0);

    /* Textarea for code */
    lv_obj_t *ta = lv_textarea_create(card);
    objects.lease_ta_code = ta;
    lv_obj_set_pos(ta, 96, 90);
    lv_obj_set_size(ta, 240, 42);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "کد تمدید عددی...");
    lv_obj_set_style_text_font(ta, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_ver(ta, 6, 0);
    lv_obj_set_style_pad_hor(ta, 8, 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_width(ta, 2, 0);
    lv_obj_set_style_radius(ta, 8, 0);

    /* Keypad for entering code */
    lv_obj_t *kb = lv_keyboard_create(card);
    lv_obj_align(kb, LV_ALIGN_TOP_MID, 0, 138);
    lv_obj_set_size(kb, 380, 108);
    ui_setup_industrial_numpad(kb);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);

    /* Bottom Buttons */
    lv_obj_t *btn_submit = lv_btn_create(card);
    lv_obj_set_pos(btn_submit, 226, 252);
    lv_obj_set_size(btn_submit, 140, 36);
    lv_obj_set_style_radius(btn_submit, 8, 0);
    lv_obj_set_style_bg_color(btn_submit, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_shadow_width(btn_submit, 0, 0);
    lv_obj_add_event_cb(btn_submit, action_submit_lease_code, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sub_lbl = lv_label_create(btn_submit);
    lv_label_set_text(sub_lbl, "ثبت و تمدید");
    lv_obj_set_style_text_font(sub_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(sub_lbl);

    lv_obj_t *btn_cancel = lv_btn_create(card);
    lv_obj_set_pos(btn_cancel, 66, 252);
    lv_obj_set_size(btn_cancel, 140, 36);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_add_event_cb(btn_cancel, action_close_lease_dialog, LV_EVENT_CLICKED, NULL);
    lv_obj_t *can_lbl = lv_label_create(btn_cancel);
    lv_label_set_text(can_lbl, "انصراف");
    lv_obj_set_style_text_font(can_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(can_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(can_lbl);
    lvgl_unlock();
}

/* =========================================================================
 * کیبورد هوشمند وای‌فای به سبک اپل با پشتیبانی کامل نمادها و کاراکترهای خاص
 * ========================================================================= */
static const char * const s_wifi_kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "@", "_", "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", ".", "1#", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t s_wifi_kb_ctrl_lc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};

static const char * const s_wifi_kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "@", "_", "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", ".", "1#", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t s_wifi_kb_ctrl_uc[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};

static const char * const s_wifi_kb_map_sym[] = {
    "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
    "-", "_", "=", "+", "[", "]", "{", "}", "\\", "|", "/", "\n",
    ":", ";", "\'", "\"", "<", ">", ",", ".", "?", "~", "\n",
    "abc", " ", "ABC", "1#", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t s_wifi_kb_ctrl_sym[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 6, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};

static void wifi_keyboard_custom_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        action_wifi_kb_ready(e);
    } else if (code == LV_EVENT_CANCEL) {
        wifi_key_act_hide();
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        buzzer_beep_click();
    }
}

static void wifi_cancel_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    wifi_key_act_hide();
}

/* =========================================================================
 * پنل مدیریت امنیتی و سناریوی اقساط روزشمار دستگاه (Manager / Admin Flow)
 * ========================================================================= */
static lv_obj_t *s_mgr_pin_backdrop = NULL;
static lv_obj_t *s_mgr_dash_backdrop = NULL;
static lv_obj_t *s_factory_cfm_backdrop = NULL;
static lv_obj_t *s_stage_val_lbls[5] = {NULL, NULL, NULL, NULL, NULL};
static lv_obj_t *s_lease_dash_status_lbl = NULL;

extern lv_obj_t *g_mgr_pin_display;
extern lv_obj_t *g_mgr_pin_err_lbl;
extern lv_obj_t *g_mgr_pin_eye_lbl;

void hide_manager_pin_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_mgr_pin_backdrop && lv_obj_is_valid(s_mgr_pin_backdrop)) {
        lv_obj_delete(s_mgr_pin_backdrop);
    }
    s_mgr_pin_backdrop = NULL;
    g_mgr_pin_display = NULL;
    g_mgr_pin_err_lbl = NULL;
    g_mgr_pin_eye_lbl = NULL;
    lvgl_unlock();
}

void show_manager_pin_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;

    hide_manager_pin_dialog();
    action_manager_pin_reset();

    s_mgr_pin_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_mgr_pin_backdrop, 0, 0);
    lv_obj_set_size(s_mgr_pin_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_mgr_pin_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_mgr_pin_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_mgr_pin_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_mgr_pin_backdrop, 0, 0);
    lv_obj_clear_flag(s_mgr_pin_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_mgr_pin_backdrop);
    lv_obj_set_pos(card, 55, 10);
    lv_obj_set_size(card, 370, 300);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 370, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(header, 255, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    if (s_pin_for_boot_wifi) {
        ui_lang_t lang = ui_lang_get();
        lv_label_set_text(title, (lang == UI_LANG_FA) ? "ورود مدیر — سرویس وای‌فای و بروزرسانی" :
                                 ((lang == UI_LANG_TR) ? "Yonetici Girisi — Servis ve Guncelleme" :
                                  "Manager PIN — Service & Update"));
    } else if (s_pin_for_warranty_edit) {
        ui_lang_t lang = ui_lang_get();
        lv_label_set_text(title, (lang == UI_LANG_FA) ? "ورود مدیر — ویرایش اطلاعات گارانتی" :
                                 ((lang == UI_LANG_TR) ? "Yonetici Girisi — Garanti Duzenleme" :
                                  "Manager PIN — Edit Warranty Info"));
    } else {
        lv_label_set_text(title, ui_lang_str(STR_PIN_TITLE));
    }
    lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_center(title);

    /* Close Button (Red Industrial Styling) */
    lv_obj_t *btn_close = lv_btn_create(header);
    lv_obj_set_pos(btn_close, 334, 4);
    lv_obj_set_size(btn_close, 32, 32);
    lv_obj_set_ext_click_area(btn_close, 8);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_border_color(btn_close, lv_color_hex(0xb91c1c), 0);
    lv_obj_set_style_border_width(btn_close, 1, 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, action_manager_pin_cancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *x_lbl = lv_label_create(btn_close);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(x_lbl);

    /* PIN Display Box */
    lv_obj_t *disp_box = lv_obj_create(card);
    lv_obj_set_pos(disp_box, 25, 48);
    lv_obj_set_size(disp_box, 260, 42);
    lv_obj_set_style_bg_color(disp_box, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_width(disp_box, 1, 0);
    lv_obj_set_style_border_color(disp_box, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_radius(disp_box, 8, 0);
    lv_obj_set_style_pad_all(disp_box, 2, 0);
    lv_obj_clear_flag(disp_box, LV_OBJ_FLAG_SCROLLABLE);

    g_mgr_pin_display = lv_label_create(disp_box);
    lv_label_set_text(g_mgr_pin_display, ui_lang_str(STR_PIN_PROMPT));
    lv_obj_set_style_text_font(g_mgr_pin_display, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(g_mgr_pin_display, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(g_mgr_pin_display, ui_lang_base_dir(), 0);
    lv_obj_center(g_mgr_pin_display);

    /* Eye Toggle Button */
    lv_obj_t *btn_eye = lv_btn_create(card);
    lv_obj_set_pos(btn_eye, 295, 48);
    lv_obj_set_size(btn_eye, 50, 42);
    lv_obj_set_ext_click_area(btn_eye, 6);
    lv_obj_set_style_radius(btn_eye, 8, 0);
    lv_obj_set_style_bg_color(btn_eye, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_color(btn_eye, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_border_width(btn_eye, 1, 0);
    lv_obj_set_style_shadow_width(btn_eye, 0, 0);
    lv_obj_add_event_cb(btn_eye, action_manager_pin_eye_toggle, LV_EVENT_CLICKED, NULL);

    g_mgr_pin_eye_lbl = lv_label_create(btn_eye);
    lv_label_set_text(g_mgr_pin_eye_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_font(g_mgr_pin_eye_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(g_mgr_pin_eye_lbl, lv_color_hex(0x334155), 0);
    lv_obj_center(g_mgr_pin_eye_lbl);

    /* Error Label - Shifted 6px UP away from keypad (y=86) */
    g_mgr_pin_err_lbl = lv_label_create(card);
    lv_obj_set_pos(g_mgr_pin_err_lbl, 25, 86);
    lv_obj_set_size(g_mgr_pin_err_lbl, 320, 20);
    lv_label_set_text(g_mgr_pin_err_lbl, "");
    lv_obj_set_style_text_font(g_mgr_pin_err_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(g_mgr_pin_err_lbl, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_text_align(g_mgr_pin_err_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(g_mgr_pin_err_lbl, ui_lang_base_dir(), 0);
    lv_obj_add_flag(g_mgr_pin_err_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Keypad (1..9, پاک, 0, ورود) - 3 Columns, 4 Rows with ext_click_area for responsive touch */
    static const int btn_y_coords[4] = {112, 158, 204, 250};
    static const int btn_x_coords[3] = {25, 136, 247};

    int digit = 1;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            lv_obj_t *btn = lv_btn_create(card);
            lv_obj_set_pos(btn, btn_x_coords[c], btn_y_coords[r]);
            lv_obj_set_size(btn, 98, 40);
            lv_obj_set_ext_click_area(btn, 6);
            lv_obj_set_style_radius(btn, 8, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284c7), 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x0369a1), 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            lv_obj_add_event_cb(btn, action_manager_pin_num, LV_EVENT_CLICKED, (void *)(intptr_t)digit);

            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text_fmt(lbl, "%d", digit);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
            lv_obj_center(lbl);
            digit++;
        }
    }

    /* Row 4: پاک (Slate) | 0 (Blue) | ورود (Green) */
    lv_obj_t *btn_del = lv_btn_create(card);
    lv_obj_set_pos(btn_del, btn_x_coords[0], btn_y_coords[3]);
    lv_obj_set_size(btn_del, 98, 40);
    lv_obj_set_ext_click_area(btn_del, 6);
    lv_obj_set_style_radius(btn_del, 8, 0);
    lv_obj_set_style_bg_color(btn_del, lv_color_hex(0x475569), 0);
    lv_obj_set_style_bg_color(btn_del, lv_color_hex(0x334155), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_del, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_border_width(btn_del, 1, 0);
    lv_obj_set_style_shadow_width(btn_del, 0, 0);
    lv_obj_add_event_cb(btn_del, action_manager_pin_del, LV_EVENT_CLICKED, NULL);
    lv_obj_t *del_lbl = lv_label_create(btn_del);
    lv_label_set_text(del_lbl, ui_lang_str(STR_PIN_CLEAR));
    lv_obj_set_style_text_font(del_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(del_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(del_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(del_lbl);

    lv_obj_t *btn_0 = lv_btn_create(card);
    lv_obj_set_pos(btn_0, btn_x_coords[1], btn_y_coords[3]);
    lv_obj_set_size(btn_0, 98, 40);
    lv_obj_set_ext_click_area(btn_0, 6);
    lv_obj_set_style_radius(btn_0, 8, 0);
    lv_obj_set_style_bg_color(btn_0, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_color(btn_0, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_border_width(btn_0, 1, 0);
    lv_obj_set_style_shadow_width(btn_0, 0, 0);
    lv_obj_add_event_cb(btn_0, action_manager_pin_num, LV_EVENT_CLICKED, (void *)(intptr_t)0);
    lv_obj_t *zero_lbl = lv_label_create(btn_0);
    lv_label_set_text(zero_lbl, "0");
    lv_obj_set_style_text_font(zero_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(zero_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(zero_lbl);

    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_pos(btn_ok, btn_x_coords[2], btn_y_coords[3]);
    lv_obj_set_size(btn_ok, 98, 40);
    lv_obj_set_ext_click_area(btn_ok, 6);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_border_color(btn_ok, lv_color_hex(0x15803d), 0);
    lv_obj_set_style_border_width(btn_ok, 1, 0);
    lv_obj_set_style_shadow_width(btn_ok, 0, 0);
    lv_obj_add_event_cb(btn_ok, action_manager_pin_ok, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(btn_ok);
    lv_label_set_text(ok_lbl, ui_lang_str(STR_PIN_ENTER));
    lv_obj_set_style_text_font(ok_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(ok_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(ok_lbl);
    lvgl_unlock();
}

void hide_manager_dashboard_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_mgr_dash_backdrop && lv_obj_is_valid(s_mgr_dash_backdrop)) {
        lv_obj_delete(s_mgr_dash_backdrop);
    }
    s_mgr_dash_backdrop = NULL;
    s_lease_dash_status_lbl = NULL;
    for (int i = 0; i < 5; i++) s_stage_val_lbls[i] = NULL;
    lvgl_unlock();
}

void show_manager_dashboard_dialog(bool is_master)
{
    if (!lvgl_lock(pdMS_TO_TICKS(500))) return;

    hide_manager_dashboard_dialog();

    s_mgr_dash_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_mgr_dash_backdrop, 0, 0);
    lv_obj_set_size(s_mgr_dash_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_mgr_dash_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_mgr_dash_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_mgr_dash_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_mgr_dash_backdrop, 0, 0);
    lv_obj_clear_flag(s_mgr_dash_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_mgr_dash_backdrop);
    lv_obj_set_pos(card, 10, 8);
    lv_obj_set_size(card, 460, 304);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 460, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(header, 255, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, is_master ? "پنل ارشد ادمین و مدیریت اقساط" : "پنل اختصاصی مدیر دستگاه");
    lv_obj_set_style_text_font(title, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, -30, 0);

    lv_obj_t *btn_exit = lv_btn_create(header);
    lv_obj_set_pos(btn_exit, 394, 4);
    lv_obj_set_size(btn_exit, 60, 30);
    lv_obj_set_ext_click_area(btn_exit, 6);
    lv_obj_set_style_radius(btn_exit, 6, 0);
    lv_obj_set_style_bg_color(btn_exit, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(btn_exit, 0, 0);
    lv_obj_add_event_cb(btn_exit, (lv_event_cb_t)hide_manager_dashboard_dialog, LV_EVENT_CLICKED, NULL);
    lv_obj_t *exit_lbl = lv_label_create(btn_exit);
    lv_label_set_text(exit_lbl, "خروج");
    lv_obj_set_style_text_font(exit_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(exit_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(exit_lbl);

    /* Tabview inside Dashboard */
    lv_obj_t *tv = lv_tabview_create(card, LV_DIR_TOP, 36);
    lv_obj_set_pos(tv, 0, 38);
    lv_obj_set_size(tv, 460, 266);
    lv_tabview_set_tab_bar_size(tv, 36);
    lv_obj_t *tv_content = lv_tabview_get_content(tv);
    if (tv_content) {
        lv_obj_set_style_anim_duration(tv_content, 0, 0);
    }

    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tv);
    if (tab_bar) {
        lv_obj_set_style_text_font(tab_bar, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(tab_bar, LV_BASE_DIR_RTL, 0);
    }

    lv_obj_t *tab1 = lv_tabview_add_tab(tv, "روزشمار و اقساط چک‌ها");
    lv_obj_t *tab2 = lv_tabview_add_tab(tv, "امنیت و کارخانه");
    lv_obj_clear_flag(tab1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn0 = lv_tabview_get_tab_button(tv, 0);
    if (btn0) {
        lv_obj_set_style_text_font(btn0, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(btn0, LV_BASE_DIR_RTL, 0);
    }
    lv_obj_t *btn1 = lv_tabview_get_tab_button(tv, 1);
    if (btn1) {
        lv_obj_set_style_text_font(btn1, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(btn1, LV_BASE_DIR_RTL, 0);
    }

    /* --- Tab 1: Installment & Check Timers --- */
    {
        /* Status Banner */
        lv_obj_t *sbox = lv_obj_create(tab1);
        lv_obj_set_pos(sbox, 10, 4);
        lv_obj_set_size(sbox, 420, 34);
        lv_obj_set_style_bg_color(sbox, lv_color_hex(0xe0f2fe), 0);
        lv_obj_set_style_border_color(sbox, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_border_width(sbox, 1, 0);
        lv_obj_set_style_radius(sbox, 6, 0);
        lv_obj_set_style_pad_all(sbox, 2, 0);
        lv_obj_clear_flag(sbox, LV_OBJ_FLAG_SCROLLABLE);

        s_lease_dash_status_lbl = lv_label_create(sbox);
        uint8_t stg = lease_active_stage_get();
        int32_t rem = lease_days_left();
        if (stg > 0) {
            lv_label_set_text_fmt(s_lease_dash_status_lbl, "وضعیت: چک مرحله %d فعال — %ld روز مانده", stg, (long)rem);
        } else {
            lv_label_set_text(s_lease_dash_status_lbl, "وضعیت: اعتبار دستگاه فعال و بدون محدودیت است");
        }
        lv_obj_set_style_text_font(s_lease_dash_status_lbl, &ui_font_yekan_16, 0);
        lv_obj_set_style_text_color(s_lease_dash_status_lbl, lv_color_hex(0x0369a1), 0);
        lv_obj_set_style_base_dir(s_lease_dash_status_lbl, LV_BASE_DIR_RTL, 0);
        lv_obj_center(s_lease_dash_status_lbl);

        /* 5 Stage Rows */
        static const char *const stage_names[5] = {"قسط اول", "قسط دوم", "قسط سوم", "قسط چهارم", "قسط پنجم"};
        for (uint8_t i = 0; i < 5; i++) {
            int y = 42 + (i * 28);
            lv_obj_t *lbl_name = lv_label_create(tab1);
            lv_obj_set_pos(lbl_name, 10, y + 2);
            lv_obj_set_size(lbl_name, 110, 24);
            lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_CLIP);
            lv_label_set_text(lbl_name, stage_names[i]);
            lv_obj_set_style_text_font(lbl_name, &ui_font_yekan_16, 0);
            lv_obj_set_style_base_dir(lbl_name, LV_BASE_DIR_RTL, 0);
            lv_obj_set_style_text_color(lbl_name, lv_color_hex(0x1e293b), 0);

            /* [-5] */
            lv_obj_t *btn_m = lv_btn_create(tab1);
            lv_obj_set_pos(btn_m, 125, y);
            lv_obj_set_size(btn_m, 36, 24);
            lv_obj_set_ext_click_area(btn_m, 8);
            lv_obj_add_flag(btn_m, LV_OBJ_FLAG_PRESS_LOCK);
            lv_obj_clear_flag(btn_m, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(btn_m, 4, 0);
            lv_obj_set_style_bg_color(btn_m, lv_color_hex(0x64748b), 0);
            lv_obj_set_style_shadow_width(btn_m, 0, 0);
            lv_obj_add_event_cb(btn_m, action_manager_stage_adj, LV_EVENT_CLICKED, (void *)(intptr_t)(i + 1));
            lv_obj_t *lm = lv_label_create(btn_m);
            lv_label_set_text(lm, "-");
            lv_obj_center(lm);

            /* Value */
            s_stage_val_lbls[i] = lv_label_create(tab1);
            lv_obj_set_pos(s_stage_val_lbls[i], 165, y + 2);
            lv_obj_set_size(s_stage_val_lbls[i], 50, 24);
            lv_label_set_text_fmt(s_stage_val_lbls[i], "%ld روز", (long)lease_stage_days_get(i + 1));
            lv_obj_set_style_text_font(s_stage_val_lbls[i], &ui_font_yekan_16, 0);
            lv_obj_set_style_base_dir(s_stage_val_lbls[i], LV_BASE_DIR_RTL, 0);
            lv_obj_set_style_text_color(s_stage_val_lbls[i], lv_color_hex(0x0284c7), 0);
            lv_obj_set_style_text_align(s_stage_val_lbls[i], LV_TEXT_ALIGN_CENTER, 0);

            /* [+5] */
            lv_obj_t *btn_p = lv_btn_create(tab1);
            lv_obj_set_pos(btn_p, 220, y);
            lv_obj_set_size(btn_p, 36, 24);
            lv_obj_set_ext_click_area(btn_p, 8);
            lv_obj_add_flag(btn_p, LV_OBJ_FLAG_PRESS_LOCK);
            lv_obj_clear_flag(btn_p, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(btn_p, 4, 0);
            lv_obj_set_style_bg_color(btn_p, lv_color_hex(0x0284c7), 0);
            lv_obj_set_style_shadow_width(btn_p, 0, 0);
            lv_obj_add_event_cb(btn_p, action_manager_stage_adj, LV_EVENT_CLICKED, (void *)(intptr_t)((i + 1) | 0x100));
            lv_obj_t *lp = lv_label_create(btn_p);
            lv_label_set_text(lp, "+");
            lv_obj_center(lp);
        }

        /* Right Column: Start Chain & Unlock */
        lv_obj_t *btn_start_chain = lv_btn_create(tab1);
        lv_obj_set_pos(btn_start_chain, 270, 44);
        lv_obj_set_size(btn_start_chain, 160, 42);
        lv_obj_set_style_radius(btn_start_chain, 8, 0);
        lv_obj_set_style_bg_color(btn_start_chain, lv_color_hex(0x2563eb), 0);
        lv_obj_set_style_shadow_width(btn_start_chain, 0, 0);
        lv_obj_add_event_cb(btn_start_chain, action_manager_chain_start, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_sc = lv_label_create(btn_start_chain);
        lv_label_set_text(lbl_sc, "فعال‌سازی روزشمار");
        lv_obj_set_style_text_font(lbl_sc, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(lbl_sc, LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_color(lbl_sc, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_sc);

        /* Unlock Input Box */
        lv_obj_t *ta_code = lv_textarea_create(tab1);
        lv_obj_set_pos(ta_code, 270, 96);
        lv_obj_set_size(ta_code, 160, 36);
        lv_textarea_set_one_line(ta_code, true);
        lv_textarea_set_placeholder_text(ta_code, "کد بازگشایی / چک");
        lv_obj_set_style_text_font(ta_code, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(ta_code, LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_font(ta_code, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_base_dir(ta_code, LV_BASE_DIR_RTL, LV_PART_TEXTAREA_PLACEHOLDER);

        lv_obj_t *btn_submit_code = lv_btn_create(tab1);
        lv_obj_set_pos(btn_submit_code, 270, 138);
        lv_obj_set_size(btn_submit_code, 160, 38);
        lv_obj_set_style_radius(btn_submit_code, 8, 0);
        lv_obj_set_style_bg_color(btn_submit_code, lv_color_hex(0x16a34a), 0);
        lv_obj_set_style_shadow_width(btn_submit_code, 0, 0);
        lv_obj_add_event_cb(btn_submit_code, action_manager_unlock_code, LV_EVENT_CLICKED, (void *)ta_code);
        lv_obj_t *lbl_sub = lv_label_create(btn_submit_code);
        lv_label_set_text(lbl_sub, "ثبت کد تایید چک");
        lv_obj_set_style_text_font(lbl_sub, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(lbl_sub, LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_sub);
    }

    /* --- Tab 2: Security & Factory Reset --- */
    {
        /* Check if Manager PIN is still default */
        char current_pin[16] = {0};
        storage_get_str(KEY_MGR_PIN, DEFAULT_MGR_PIN, current_pin, sizeof(current_pin));
        bool is_default = (strcmp(current_pin, DEFAULT_MGR_PIN) == 0);

        /* Banner Box */
        lv_obj_t *banner = lv_obj_create(tab2);
        lv_obj_set_pos(banner, 10, 4);
        lv_obj_set_size(banner, 420, 38);
        lv_obj_set_style_radius(banner, 6, 0);
        lv_obj_set_style_pad_all(banner, 2, 0);
        lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *banner_lbl = lv_label_create(banner);
        if (is_default) {
            lv_obj_set_style_bg_color(banner, lv_color_hex(0xfef3c7), 0);
            lv_obj_set_style_border_color(banner, lv_color_hex(0xf59e0b), 0);
            lv_label_set_text(banner_lbl, "هشدار: رمز پیش‌فرض (123456) فعال است! لطفاً آن را تغییر دهید.");
            lv_obj_set_style_text_color(banner_lbl, lv_color_hex(0xb45309), 0);
        } else {
            lv_obj_set_style_bg_color(banner, lv_color_hex(0xdcfce7), 0);
            lv_obj_set_style_border_color(banner, lv_color_hex(0x16a34a), 0);
            lv_label_set_text(banner_lbl, "وضعیت امنیت: رمز مدیر با موفقیت تغییر یافته و ایمن است.");
            lv_obj_set_style_text_color(banner_lbl, lv_color_hex(0x15803d), 0);
        }
        lv_obj_set_style_text_font(banner_lbl, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(banner_lbl, LV_BASE_DIR_RTL, 0);
        lv_obj_center(banner_lbl);

        /* Change PIN Section */
        lv_obj_t *ta_new_pin = lv_textarea_create(tab2);
        lv_obj_set_pos(ta_new_pin, 10, 50);
        lv_obj_set_size(ta_new_pin, 200, 38);
        lv_textarea_set_one_line(ta_new_pin, true);
        lv_textarea_set_placeholder_text(ta_new_pin, "رمز عبور جدید مدیر...");
        lv_obj_set_style_text_font(ta_new_pin, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(ta_new_pin, LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_font(ta_new_pin, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_base_dir(ta_new_pin, LV_BASE_DIR_RTL, LV_PART_TEXTAREA_PLACEHOLDER);

        lv_obj_t *btn_save_pin = lv_btn_create(tab2);
        lv_obj_set_pos(btn_save_pin, 220, 50);
        lv_obj_set_size(btn_save_pin, 210, 38);
        lv_obj_set_style_radius(btn_save_pin, 8, 0);
        lv_obj_set_style_bg_color(btn_save_pin, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_shadow_width(btn_save_pin, 0, 0);
        lv_obj_add_event_cb(btn_save_pin, action_manager_change_pin, LV_EVENT_CLICKED, (void *)ta_new_pin);
        lv_obj_t *lbl_sp = lv_label_create(btn_save_pin);
        lv_label_set_text(lbl_sp, "ثبت و ذخیره رمز جدید");
        lv_obj_set_style_text_font(lbl_sp, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(lbl_sp, LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_color(lbl_sp, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_sp);

        /* Factory Reset Button */
        lv_obj_t *btn_fac = lv_btn_create(tab2);
        lv_obj_set_pos(btn_fac, 10, 96);
        lv_obj_set_size(btn_fac, 420, 40);
        lv_obj_set_style_radius(btn_fac, 8, 0);
        lv_obj_set_style_bg_color(btn_fac, lv_color_hex(0xb91c1c), 0);
        lv_obj_set_style_shadow_width(btn_fac, 0, 0);
        lv_obj_add_event_cb(btn_fac, action_manager_factory_reset, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_fac = lv_label_create(btn_fac);
        lv_label_set_text(lbl_fac, "بازنشانی به مقادیر کارخانه (Factory Reset)");
        lv_obj_set_style_text_font(lbl_fac, &ui_font_yekan_16, 0);
        lv_obj_set_style_base_dir(lbl_fac, LV_BASE_DIR_RTL, 0);
        lv_obj_set_style_text_color(lbl_fac, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_fac);

        /* Quick Links: Fault history & Help */
        lv_obj_t *btn_flt = lv_btn_create(tab2);
        lv_obj_set_pos(btn_flt, 10, 144);
        lv_obj_set_size(btn_flt, 205, 38);
        lv_obj_set_style_radius(btn_flt, 8, 0);
        lv_obj_set_style_bg_color(btn_flt, lv_color_hex(0x475569), 0);
        lv_obj_set_style_shadow_width(btn_flt, 0, 0);
        lv_obj_add_event_cb(btn_flt, action_open_fault_history, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_flt = lv_label_create(btn_flt);
        lv_label_set_text(lbl_flt, "تاریخچه خطاهای سیستم");
        lv_obj_set_style_text_font(lbl_flt, &ui_font_yekan_16, 0);
        lv_obj_set_style_text_color(lbl_flt, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_flt);

        lv_obj_t *btn_hlp = lv_btn_create(tab2);
        lv_obj_set_pos(btn_hlp, 225, 144);
        lv_obj_set_size(btn_hlp, 205, 38);
        lv_obj_set_style_radius(btn_hlp, 8, 0);
        lv_obj_set_style_bg_color(btn_hlp, lv_color_hex(0x0369a1), 0);
        lv_obj_set_style_shadow_width(btn_hlp, 0, 0);
        lv_obj_add_event_cb(btn_hlp, action_help_dialog, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_hlp = lv_label_create(btn_hlp);
        lv_label_set_text(lbl_hlp, "راهنمای جامع دستگاه");
        lv_obj_set_style_text_font(lbl_hlp, &ui_font_yekan_16, 0);
        lv_obj_set_style_text_color(lbl_hlp, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_hlp);
    }
    lvgl_unlock();
}

/* =========================================================================
 * دیالوگ تایید بازنشانی تنظیمات کارخانه (Apple HIG با نوار تخلیه و تاچ پاز)
 * ========================================================================= */
static lv_obj_t *s_factory_countdown_bar = NULL;
static lv_obj_t *s_factory_countdown_lbl = NULL;
static lv_timer_t *s_factory_countdown_timer = NULL;
static int32_t s_factory_remaining_ms = 0;
static int32_t s_factory_total_ms = 12000;
static bool s_factory_timer_paused = false;

#define FACTORY_RESET_TIMEOUT_MS 12000
#define FACTORY_RESET_TICK_MS    100

void hide_factory_reset_confirm_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
    if (s_factory_countdown_timer) {
        lv_timer_delete(s_factory_countdown_timer);
        s_factory_countdown_timer = NULL;
    }
    s_factory_timer_paused = false;
    s_factory_remaining_ms = 0;
    if (s_factory_cfm_backdrop && lv_obj_is_valid(s_factory_cfm_backdrop)) {
        lv_obj_delete(s_factory_cfm_backdrop);
    }
    s_factory_cfm_backdrop = NULL;
    s_factory_countdown_bar = NULL;
    s_factory_countdown_lbl = NULL;
    lvgl_unlock();
}

static void factory_reset_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_factory_timer_paused) {
        return; /* Touch-and-Hold Freeze */
    }

    s_factory_remaining_ms -= FACTORY_RESET_TICK_MS;
    if (s_factory_remaining_ms <= 0) {
        hide_factory_reset_confirm_dialog();
        return;
    }

    if (s_factory_countdown_bar && lv_obj_is_valid(s_factory_countdown_bar)) {
        int32_t pct = (s_factory_remaining_ms * 100) / (s_factory_total_ms > 0 ? s_factory_total_ms : FACTORY_RESET_TIMEOUT_MS);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        lv_bar_set_value(s_factory_countdown_bar, pct, LV_ANIM_OFF);
    }

    if (s_factory_countdown_lbl && lv_obj_is_valid(s_factory_countdown_lbl)) {
        int sec = (s_factory_remaining_ms + 999) / 1000;
        char buf[96];
        snprintf(buf, sizeof(buf), ui_lang_str(STR_FACTORY_RST_AUTO_CLOSE), sec);
        lv_label_set_text(s_factory_countdown_lbl, buf);
    }
}

static void factory_dialog_touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_factory_timer_paused = true;
        if (s_factory_countdown_bar && lv_obj_is_valid(s_factory_countdown_bar)) {
            lv_obj_set_style_bg_color(s_factory_countdown_bar, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
        }
        if (s_factory_countdown_lbl && lv_obj_is_valid(s_factory_countdown_lbl)) {
            int sec = (s_factory_remaining_ms + 999) / 1000;
            char buf[96];
            snprintf(buf, sizeof(buf), ui_lang_str(STR_FACTORY_RST_AUTO_PAUSED), sec);
            lv_label_set_text(s_factory_countdown_lbl, buf);
            lv_obj_set_style_text_color(s_factory_countdown_lbl, lv_color_hex(0xfbbf24), 0);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED) {
        s_factory_timer_paused = false;
        if (s_factory_countdown_bar && lv_obj_is_valid(s_factory_countdown_bar)) {
            lv_obj_set_style_bg_color(s_factory_countdown_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
        }
        if (s_factory_countdown_lbl && lv_obj_is_valid(s_factory_countdown_lbl)) {
            int sec = (s_factory_remaining_ms + 999) / 1000;
            char buf[96];
            snprintf(buf, sizeof(buf), ui_lang_str(STR_FACTORY_RST_AUTO_CLOSE), sec);
            lv_label_set_text(s_factory_countdown_lbl, buf);
            lv_obj_set_style_text_color(s_factory_countdown_lbl, lv_color_hex(0x94a3b8), 0);
        }
    }
}

void show_factory_reset_confirm_dialog(void)
{
    hide_factory_reset_confirm_dialog();

    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;

    s_factory_cfm_backdrop = lv_obj_create(lv_layer_top());
    if (!s_factory_cfm_backdrop) {
        lvgl_unlock();
        return;
    }
    lv_obj_set_pos(s_factory_cfm_backdrop, 0, 0);
    lv_obj_set_size(s_factory_cfm_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_factory_cfm_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_factory_cfm_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_factory_cfm_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_factory_cfm_backdrop, 0, 0);
    lv_obj_clear_flag(s_factory_cfm_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_factory_cfm_backdrop, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_factory_cfm_backdrop, factory_dialog_touch_cb, LV_EVENT_ALL, NULL);

    /* کادر Apple HIG دیالوگ هشدار */
    lv_obj_t *card = lv_obj_create(s_factory_cfm_backdrop);
    lv_obj_set_pos(card, 30, 38);
    lv_obj_set_size(card, 420, 244);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x18181b), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, factory_dialog_touch_cb, LV_EVENT_ALL, NULL);

    /* ۱. هدر کارت: آیکون اخطار + عنوان */
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_width(hdr, lv_pct(100));
    lv_obj_set_height(hdr, 38);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_bg_opa(hdr, 255, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 8, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hdr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(hdr, factory_dialog_touch_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *icon_lbl = lv_label_create(hdr);
    lv_label_set_text(icon_lbl, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_24, 0);

    lv_obj_t *title_lbl = lv_label_create(hdr);
    lv_label_set_text(title_lbl, ui_lang_str(STR_FACTORY_RST_TITLE));
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);

    /* ۲. کادر متن توضیحات: شرح دقیق عملکرد بازنشانی */
    lv_obj_t *body_box = lv_obj_create(card);
    lv_obj_set_pos(body_box, 16, 42);
    lv_obj_set_size(body_box, 388, 86);
    lv_obj_set_style_bg_color(body_box, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_radius(body_box, 10, 0);
    lv_obj_set_style_border_width(body_box, 1, 0);
    lv_obj_set_style_border_color(body_box, lv_color_hex(0x334155), 0);
    lv_obj_set_style_pad_left(body_box, 6, 0);
    lv_obj_set_style_pad_right(body_box, 6, 0);
    lv_obj_set_style_pad_top(body_box, 3, 0);
    lv_obj_set_style_pad_bottom(body_box, 6, 0);
    lv_obj_clear_flag(body_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(body_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(body_box, factory_dialog_touch_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *txt = lv_label_create(body_box);
    lv_obj_set_width(txt, 376);
    lv_label_set_text(txt, ui_lang_str(STR_FACTORY_RST_DESC));
    lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(txt, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(txt, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_text_line_space(txt, 2, 0);
    lv_obj_set_style_base_dir(txt, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(txt, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(txt, LV_ALIGN_TOP_MID, 0, -1);
    lv_obj_clear_flag(txt, LV_OBJ_FLAG_CLICKABLE);

    /* ۳. نوار پیشرفت تخلیه زمان‌سنج (Draining Progress Bar) */
    s_factory_countdown_bar = lv_bar_create(card);
    lv_obj_set_pos(s_factory_countdown_bar, 16, 134);
    lv_obj_set_size(s_factory_countdown_bar, 388, 5);
    lv_obj_set_style_bg_color(s_factory_countdown_bar, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(s_factory_countdown_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_factory_countdown_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_factory_countdown_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(s_factory_countdown_bar, 0, 100);
    lv_bar_set_value(s_factory_countdown_bar, 100, LV_ANIM_OFF);

    /* برچسب ثانیه‌شمار معکوس */
    s_factory_countdown_lbl = lv_label_create(card);
    char buf[96];
    snprintf(buf, sizeof(buf), ui_lang_str(STR_FACTORY_RST_AUTO_CLOSE), FACTORY_RESET_TIMEOUT_MS / 1000);
    lv_label_set_text(s_factory_countdown_lbl, buf);
    lv_obj_set_pos(s_factory_countdown_lbl, 16, 143);
    lv_obj_set_size(s_factory_countdown_lbl, 388, 22);
    lv_obj_set_style_text_align(s_factory_countdown_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_factory_countdown_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(s_factory_countdown_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_base_dir(s_factory_countdown_lbl, ui_lang_base_dir(), 0);
    lv_obj_clear_flag(s_factory_countdown_lbl, LV_OBJ_FLAG_CLICKABLE);

    /* ۴. دکمه‌های تعاملی ۴۴px: انصراف | بله، بازنشانی شود */
    const int32_t btn_w = 186;
    const int32_t btn_h = 46;
    const int32_t btn_y = 172;

    lv_obj_t *btn_no = lv_btn_create(card);
    lv_obj_set_pos(btn_no, 16, btn_y);
    lv_obj_set_size(btn_no, btn_w, btn_h);
    lv_obj_set_style_radius(btn_no, 10, 0);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(btn_no, 1, 0);
    lv_obj_set_style_border_color(btn_no, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_no, 0, 0);
    lv_obj_add_event_cb(btn_no, action_manager_factory_cancel, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_no, factory_dialog_touch_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *no_lbl = lv_label_create(btn_no);
    lv_label_set_text(no_lbl, ui_lang_str(STR_FACTORY_RST_CANCEL));
    lv_obj_set_style_text_font(no_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(no_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(no_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(no_lbl);

    lv_obj_t *btn_yes = lv_btn_create(card);
    lv_obj_set_pos(btn_yes, 218, btn_y);
    lv_obj_set_size(btn_yes, btn_w, btn_h);
    lv_obj_set_style_radius(btn_yes, 10, 0);
    lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_border_width(btn_yes, 1, 0);
    lv_obj_set_style_border_color(btn_yes, lv_color_hex(0xb91c1c), 0);
    lv_obj_set_style_shadow_width(btn_yes, 0, 0);
    lv_obj_add_event_cb(btn_yes, action_manager_factory_confirm, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_yes, factory_dialog_touch_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *yes_lbl = lv_label_create(btn_yes);
    lv_label_set_text(yes_lbl, ui_lang_str(STR_FACTORY_RST_CONFIRM));
    lv_obj_set_style_text_font(yes_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(yes_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(yes_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(yes_lbl);

    /* ۵. راه‌اندازی تایمر معکوس */
    s_factory_total_ms = FACTORY_RESET_TIMEOUT_MS;
    s_factory_remaining_ms = FACTORY_RESET_TIMEOUT_MS;
    s_factory_timer_paused = false;
    s_factory_countdown_timer = lv_timer_create(factory_reset_timer_cb, FACTORY_RESET_TICK_MS, NULL);

    buzzer_beep_click();
    lvgl_unlock();
}

/* =========================================================================
 * دیالوگ تایید صفر کردن شمارنده تولید
 * ========================================================================= */
static lv_obj_t *s_reset_cnt_backdrop = NULL;

void hide_reset_counter_confirm_dialog(void)
{
    if (s_reset_cnt_backdrop && lv_obj_is_valid(s_reset_cnt_backdrop)) {
        lv_obj_delete(s_reset_cnt_backdrop);
    }
    s_reset_cnt_backdrop = NULL;
}

void show_reset_counter_confirm_dialog(void)
{
    hide_reset_counter_confirm_dialog();

    s_reset_cnt_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_reset_cnt_backdrop, 0, 0);
    lv_obj_set_size(s_reset_cnt_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_reset_cnt_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_reset_cnt_backdrop, 190, 0);
    lv_obj_set_style_border_width(s_reset_cnt_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_reset_cnt_backdrop, 0, 0);
    lv_obj_clear_flag(s_reset_cnt_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_reset_cnt_backdrop);
    lv_obj_set_pos(card, 50, 60);
    lv_obj_set_size(card, 380, 200);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xea580c), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 36);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xea580c), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t_lbl = lv_label_create(header);
    lv_label_set_text(t_lbl, ui_lang_str(STR_RST_CNT_TITLE));
    lv_obj_set_style_text_font(t_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(t_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(t_lbl);

    /* Question text */
    lv_obj_t *txt = lv_label_create(card);
    lv_obj_set_pos(txt, 20, 52);
    lv_obj_set_size(txt, 340, 66);
    lv_label_set_text(txt, ui_lang_str(STR_RST_CNT_MSG));
    lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(txt, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(txt, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_base_dir(txt, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(txt, LV_TEXT_ALIGN_CENTER, 0);

    /* Buttons: [انصراف]  [بله، صفر شود] */
    lv_obj_t *btn_no = lv_btn_create(card);
    lv_obj_set_pos(btn_no, 40, 136);
    lv_obj_set_size(btn_no, 130, 42);
    lv_obj_set_style_radius(btn_no, 8, 0);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_shadow_width(btn_no, 0, 0);
    lv_obj_add_event_cb(btn_no, action_reset_counter_cancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *no_lbl = lv_label_create(btn_no);
    lv_label_set_text(no_lbl, ui_lang_str(STR_UNSAVED_CANCEL));
    lv_obj_set_style_text_font(no_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(no_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(no_lbl);

    lv_obj_t *btn_yes = lv_btn_create(card);
    lv_obj_set_pos(btn_yes, 210, 136);
    lv_obj_set_size(btn_yes, 130, 42);
    lv_obj_set_style_radius(btn_yes, 8, 0);
    lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0xea580c), 0);
    lv_obj_set_style_shadow_width(btn_yes, 0, 0);
    lv_obj_add_event_cb(btn_yes, action_reset_counter_confirm, LV_EVENT_CLICKED, NULL);
    lv_obj_t *yes_lbl = lv_label_create(btn_yes);
    lv_label_set_text(yes_lbl, ui_lang_str(STR_RST_CNT_CONFIRM));
    lv_obj_set_style_text_font(yes_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(yes_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(yes_lbl);
}

/* =========================================================================
 * دیالوگ یادآوری اتصال به وای‌فای در زمان راه‌اندازی (Boot Wi-Fi Reminder)
 * ========================================================================= */
static lv_obj_t *s_boot_wifi_backdrop = NULL;

void hide_boot_wifi_prompt_dialog(void)
{
    if (s_boot_wifi_backdrop && lv_obj_is_valid(s_boot_wifi_backdrop)) {
        lv_obj_delete(s_boot_wifi_backdrop);
    }
    s_boot_wifi_backdrop = NULL;
}

void show_boot_wifi_prompt_dialog(void)
{
    hide_boot_wifi_prompt_dialog();

    s_boot_wifi_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_boot_wifi_backdrop, 0, 0);
    lv_obj_set_size(s_boot_wifi_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_boot_wifi_backdrop, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_boot_wifi_backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_boot_wifi_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_boot_wifi_backdrop, 0, 0);
    lv_obj_clear_flag(s_boot_wifi_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_boot_wifi_backdrop);
    lv_obj_set_pos(card, 45, 55);
    lv_obj_set_size(card, 390, 210);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t_lbl = lv_label_create(header);
    lv_label_set_text(t_lbl, ui_lang_str(STR_BOOT_WIFI_PROMPT_TITLE));
    lv_obj_set_style_text_font(t_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(t_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(t_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(t_lbl);

    /* Message */
    lv_obj_t *txt = lv_label_create(card);
    lv_obj_set_pos(txt, 20, 50);
    lv_obj_set_size(txt, 350, 75);
    lv_label_set_text(txt, ui_lang_str(STR_BOOT_WIFI_PROMPT_MSG));
    lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(txt, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(txt, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_base_dir(txt, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(txt, LV_TEXT_ALIGN_CENTER, 0);

    /* Buttons: [انصراف]  [تنظیمات وای‌فای] */
    lv_obj_t *btn_no = lv_btn_create(card);
    lv_obj_set_pos(btn_no, 30, 145);
    lv_obj_set_size(btn_no, 150, 44);
    lv_obj_set_style_radius(btn_no, 8, 0);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_shadow_width(btn_no, 0, 0);
    lv_obj_add_event_cb(btn_no, action_boot_wifi_cancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *no_lbl = lv_label_create(btn_no);
    lv_label_set_text(no_lbl, ui_lang_str(STR_BOOT_WIFI_BTN_LATER));
    lv_obj_set_style_text_font(no_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(no_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(no_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(no_lbl);

    lv_obj_t *btn_yes = lv_btn_create(card);
    lv_obj_set_pos(btn_yes, 210, 145);
    lv_obj_set_size(btn_yes, 150, 44);
    lv_obj_set_style_radius(btn_yes, 8, 0);
    lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(btn_yes, 0, 0);
    lv_obj_add_event_cb(btn_yes, action_boot_wifi_goto, LV_EVENT_CLICKED, NULL);
    lv_obj_t *yes_lbl = lv_label_create(btn_yes);
    lv_label_set_text(yes_lbl, ui_lang_str(STR_BOOT_WIFI_BTN_GOTO));
    lv_obj_set_style_text_font(yes_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(yes_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(yes_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(yes_lbl);

    lv_obj_add_event_cb(s_boot_wifi_backdrop, action_boot_wifi_cancel, LV_EVENT_CLICKED, NULL);
}

/* =========================================================================
 * دیالوگ اعلان به‌روزرسانی فریمور در زمان بوت (صفحه اصلی)
 * ========================================================================= */
static lv_obj_t *s_boot_ota_backdrop = NULL;

void hide_boot_ota_prompt_dialog(void)
{
    if (s_boot_ota_backdrop && lv_obj_is_valid(s_boot_ota_backdrop)) {
        lv_obj_del(s_boot_ota_backdrop);
    }
    s_boot_ota_backdrop = NULL;
}

void show_boot_ota_prompt_dialog(const char *new_ver)
{
    hide_boot_ota_prompt_dialog();

    s_boot_ota_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_boot_ota_backdrop, 0, 0);
    lv_obj_set_size(s_boot_ota_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_boot_ota_backdrop, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_boot_ota_backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_boot_ota_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_boot_ota_backdrop, 0, 0);
    lv_obj_clear_flag(s_boot_ota_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_boot_ota_backdrop);
    lv_obj_set_pos(card, 40, 50);
    lv_obj_set_size(card, 400, 220);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t_lbl = lv_label_create(header);
    lv_label_set_text(t_lbl, ui_lang_str(STR_BOOT_OTA_PROMPT_TITLE));
    lv_obj_set_style_text_font(t_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(t_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(t_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(t_lbl);

    /* Message */
    lv_obj_t *txt = lv_label_create(card);
    lv_obj_set_pos(txt, 15, 48);
    lv_obj_set_size(txt, 370, 85);
    char msg_buf[192];
    snprintf(msg_buf, sizeof(msg_buf), ui_lang_str(STR_BOOT_OTA_PROMPT_MSG), new_ver ? new_ver : "جدید");
    lv_label_set_text(txt, msg_buf);
    lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(txt, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(txt, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_base_dir(txt, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(txt, LV_TEXT_ALIGN_CENTER, 0);

    /* Buttons: [انصراف]  [دریافت و نصب] */
    lv_obj_t *btn_no = lv_btn_create(card);
    lv_obj_set_pos(btn_no, 25, 150);
    lv_obj_set_size(btn_no, 160, 44);
    lv_obj_set_style_radius(btn_no, 8, 0);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_shadow_width(btn_no, 0, 0);
    lv_obj_add_event_cb(btn_no, action_boot_ota_cancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *no_lbl = lv_label_create(btn_no);
    lv_label_set_text(no_lbl, ui_lang_str(STR_BOOT_OTA_BTN_LATER));
    lv_obj_set_style_text_font(no_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(no_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(no_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(no_lbl);

    lv_obj_t *btn_yes = lv_btn_create(card);
    lv_obj_set_pos(btn_yes, 215, 150);
    lv_obj_set_size(btn_yes, 160, 44);
    lv_obj_set_style_radius(btn_yes, 8, 0);
    lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_shadow_width(btn_yes, 0, 0);
    lv_obj_add_event_cb(btn_yes, action_boot_ota_install, LV_EVENT_CLICKED, NULL);
    lv_obj_t *yes_lbl = lv_label_create(btn_yes);
    lv_label_set_text(yes_lbl, ui_lang_str(STR_BOOT_OTA_BTN_INSTALL));
    lv_obj_set_style_text_font(yes_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(yes_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(yes_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(yes_lbl);

    lv_obj_add_event_cb(s_boot_ota_backdrop, action_boot_ota_cancel, LV_EVENT_CLICKED, NULL);
}

/* =========================================================================
 * دیالوگ مشاهده تاریخچه ۱۰ خطای اخیر سیستم
 * ========================================================================= */
static lv_obj_t *s_fault_backdrop = NULL;

void hide_fault_history_dialog(void)
{
    if (s_fault_backdrop && lv_obj_is_valid(s_fault_backdrop)) {
        lv_obj_delete(s_fault_backdrop);
        extern void lvgl_port_suppress_touch_until_release(void);
        lvgl_port_suppress_touch_until_release();
    }
    s_fault_backdrop = NULL;
}

static void action_fault_history_row_click(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_indev_t *indev = lv_indev_get_act();
    if (indev && lv_indev_get_scroll_obj(indev) != NULL) return; /* در حین اسکرول رویداد کلیک باز نشود */

    uint8_t code = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    hide_fault_history_dialog();
    hide_interactive_fault_dialog();
    extern void plc_trigger_fault_interactive(uint8_t code);
    plc_trigger_fault_interactive(code);
}

void show_fault_history_dialog(void)
{
    fault_log_mark_read();
    ui_update_toolbar_buttons();
    hide_fault_history_dialog();

    s_fault_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_fault_backdrop, 0, 0);
    lv_obj_set_size(s_fault_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_fault_backdrop, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_fault_backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_fault_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_fault_backdrop, 0, 0);
    lv_obj_clear_flag(s_fault_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    /* Card */
    lv_obj_t *card = lv_obj_create(s_fault_backdrop);
    lv_obj_set_pos(card, 16, 12);
    lv_obj_set_size(card, 448, 296);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(header, 255, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, ui_lang_str(STR_FAULT_HIST_TITLE));
    lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_center(title);

    lv_obj_t *btn_close = lv_btn_create(header);
    lv_obj_align(btn_close, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_size(btn_close, 32, 32);
    lv_obj_set_ext_click_area(btn_close, 8);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_set_style_bg_opa(btn_close, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(btn_close, 80, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, action_close_fault_history, LV_EVENT_CLICKED, NULL);
    lv_obj_t *x_lbl = lv_label_create(btn_close);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(x_lbl);

    /* Body - List container */
    lv_obj_t *list = lv_obj_create(card);
    lv_obj_set_pos(list, 10, 42);
    lv_obj_set_size(list, 428, 204);
    lv_obj_set_style_bg_color(list, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_width(list, 1, 0);
    lv_obj_set_style_border_color(list, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_set_style_pad_all(list, 4, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_anim_duration(list, 0, 0);

    int count = fault_log_get_count();
    if (count == 0) {
        lv_obj_t *empty_lbl = lv_label_create(list);
        lv_label_set_text(empty_lbl, ui_lang_str(STR_FAULT_HIST_EMPTY));
        lv_obj_set_style_text_font(empty_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(empty_lbl, lv_color_hex(0x16a34a), 0);
        lv_obj_set_style_base_dir(empty_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(empty_lbl);
    } else {
        for (int i = 0; i < count; i++) {
            fault_entry_t entry;
            if (!fault_log_get_entry(i, &entry)) continue;

            lv_obj_t *row = lv_obj_create(list);
            lv_obj_set_size(row, 400, 34);
            lv_obj_set_pos(row, 0, i * 36);
            lv_obj_set_style_bg_color(row, (i % 2 == 0) ? lv_color_hex(0xffffff) : lv_color_hex(0xf8fafc), 0);
            lv_obj_set_style_border_width(row, 1, 0);
            lv_obj_set_style_border_color(row, lv_color_hex(0xe2e8f0), 0);
            lv_obj_set_style_radius(row, 6, 0);
            lv_obj_set_style_pad_all(row, 2, 0);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE); /* خود سطر کلیک‌پذیر نیست تا اسکرول کاملاً روان و بدون تداخل باشد */
            lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

            /* Index */
            lv_obj_t *idx_lbl = lv_label_create(row);
            lv_label_set_text_fmt(idx_lbl, "#%d", i + 1);
            lv_obj_set_style_text_font(idx_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(idx_lbl, lv_color_hex(0x64748b), 0);
            lv_obj_set_pos(idx_lbl, 6, 7);

            /* Fault Title */
            lv_obj_t *name_lbl = lv_label_create(row);
            lv_label_set_text(name_lbl, fault_code_to_str((fault_code_t)entry.code));
            lv_obj_set_style_text_font(name_lbl, ui_lang_font_16(), 0);
            lv_obj_set_style_text_color(name_lbl, lv_color_hex(0x0f172a), 0);
            lv_obj_set_style_base_dir(name_lbl, ui_lang_base_dir(), 0);
            lv_obj_set_size(name_lbl, 235, 24);
            lv_obj_set_style_text_align(name_lbl, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_set_pos(name_lbl, 45, 5);

            /* Timestamp */
            time_t t_stamp = (time_t)entry.timestamp;
            struct tm tm_info;
            localtime_r(&t_stamp, &tm_info);
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);

            lv_obj_t *time_lbl = lv_label_create(row);
            lv_label_set_text(time_lbl, time_str);
            lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(time_lbl, lv_color_hex(0xd97706), 0);
            lv_obj_set_pos(time_lbl, 285, 7);

            /* دکمه اختصاصی جزئیات و فلش در سمت راست سطر */
            lv_obj_t *btn_detail = lv_btn_create(row);
            lv_obj_set_size(btn_detail, 40, 28);
            lv_obj_set_pos(btn_detail, 355, 1);
            lv_obj_set_style_bg_opa(btn_detail, LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_opa(btn_detail, 60, LV_STATE_PRESSED);
            lv_obj_set_style_radius(btn_detail, 6, 0);
            lv_obj_set_style_pad_all(btn_detail, 0, 0);
            lv_obj_set_style_shadow_width(btn_detail, 0, 0);
            lv_obj_add_event_cb(btn_detail, action_fault_history_row_click, LV_EVENT_CLICKED, (void *)(intptr_t)entry.code);

            lv_obj_t *arr_lbl = lv_label_create(btn_detail);
            lv_label_set_text(arr_lbl, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_font(arr_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(arr_lbl, lv_color_hex(0x0284c7), 0);
            lv_obj_center(arr_lbl);
        }
    }

    /* Bottom Buttons */
    lv_obj_t *btn_clr = lv_btn_create(card);
    lv_obj_set_pos(btn_clr, 16, 252);
    lv_obj_set_size(btn_clr, 140, 36);
    lv_obj_set_style_radius(btn_clr, 8, 0);
    lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_shadow_width(btn_clr, 0, 0);
    lv_obj_add_event_cb(btn_clr, action_clear_fault_history, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clr_lbl = lv_label_create(btn_clr);
    lv_label_set_text(clr_lbl, ui_lang_str(STR_FAULT_HIST_CLEAR));
    lv_obj_set_style_text_font(clr_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(clr_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(clr_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(clr_lbl);

    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_pos(btn_ok, 292, 252);
    lv_obj_set_size(btn_ok, 124, 36);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(btn_ok, 0, 0);
    lv_obj_add_event_cb(btn_ok, action_close_fault_history, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(btn_ok);
    lv_label_set_text(ok_lbl, ui_lang_str(STR_FAULT_HIST_CLOSE));
    lv_obj_set_style_text_font(ok_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(ok_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(ok_lbl);
}

/* =========================================================================
 * دیالوگ تعاملی و آموزشی خطاهای صنعتی (Interactive Industrial Fault Modal)
 * ========================================================================= */
static lv_obj_t *s_fault_interactive_backdrop = NULL;

void hide_interactive_fault_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    if (s_fault_interactive_backdrop && lv_obj_is_valid(s_fault_interactive_backdrop)) {
        lv_obj_delete(s_fault_interactive_backdrop);
        extern void lvgl_port_suppress_touch_until_release(void);
        lvgl_port_suppress_touch_until_release();
    }
    s_fault_interactive_backdrop = NULL;
    objects.interactive_fault_modal = NULL;
    lvgl_unlock();
}

static void __attribute__((unused)) action_fault_disable_pulley(lv_event_t *e)
{
    (void)e;
    buzzer_beep_confirm();
    s_pulley_en = false;
    storage_set_bool(KEY_PULLEY_EN, false);
    nvs_request_lazy_flush();
    update_pulley_en_ui();
    plc_dismiss_current_fault();
    hide_interactive_fault_dialog();
    panel_nav_toast_info(ui_lang_str(STR_PULLEY_DISABLED));
}

static void action_fault_bypass_pulley_4h(lv_event_t *e)
{
    (void)e;
    buzzer_beep_confirm();
    machine_advisor_enable_pulley_bypass(PULLEY_BYPASS_DEFAULT_SEC);
    plc_dismiss_current_fault();
    hide_interactive_fault_dialog();
    panel_nav_toast_info("سنسور پولی به مدت ۴ ساعت بای‌پس شد");
}

static void action_fault_enable_thermo_fallback(lv_event_t *e)
{
    (void)e;
    buzzer_beep_confirm();
    machine_advisor_set_thermo_fallback(true);
    plc_dismiss_current_fault();
    hide_interactive_fault_dialog();
    panel_nav_toast_info("سرمایش اضطراری زمانی مخزن فعال شد");
}

void show_interactive_fault_dialog(uint8_t code, const char *title, const char *cause, const char *solution)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    hide_interactive_fault_dialog();

    s_fault_interactive_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_fault_interactive_backdrop, 0, 0);
    lv_obj_set_size(s_fault_interactive_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_fault_interactive_backdrop, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_fault_interactive_backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_fault_interactive_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_fault_interactive_backdrop, 0, 0);
    lv_obj_clear_flag(s_fault_interactive_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    /* Main Modal Card */
    lv_obj_t *card = lv_obj_create(s_fault_interactive_backdrop);
    objects.interactive_fault_modal = card;
    lv_obj_set_pos(card, 16, 10);
    lv_obj_set_size(card, 448, 300);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 42);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_bg_opa(header, 255, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, title ? title : "خطای سیستم");
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_width(title_lbl, 432);
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(title_lbl, 4, 0);
    lv_obj_center(title_lbl);

    /* Body - Scrollable content with flex column layout */
    lv_obj_t *body = lv_obj_create(card);
    lv_obj_set_pos(body, 10, 46);
    lv_obj_set_size(body, 428, 198);
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_width(body, 1, 0);
    lv_obj_set_style_border_color(body, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_radius(body, 8, 0);
    lv_obj_set_style_pad_all(body, 6, 0);
    lv_obj_set_style_pad_row(body, 8, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_anim_duration(body, 0, 0);

    /* 1. Cause Container Card (Soft red tint) */
    lv_obj_t *cause_card = lv_obj_create(body);
    lv_obj_set_width(cause_card, 412);
    lv_obj_set_height(cause_card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(cause_card, lv_color_hex(0xfef2f2), 0);
    lv_obj_set_style_border_width(cause_card, 1, 0);
    lv_obj_set_style_border_color(cause_card, lv_color_hex(0xfecaca), 0);
    lv_obj_set_style_radius(cause_card, 8, 0);
    lv_obj_set_style_pad_all(cause_card, 6, 0);
    lv_obj_set_style_pad_row(cause_card, 4, 0);
    lv_obj_set_flex_flow(cause_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(cause_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cause_card, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    /* Cause Header */
    lv_obj_t *cause_hdr = lv_label_create(cause_card);
    lv_label_set_text(cause_hdr, ui_lang_str(STR_FAULT_CAUSE_HDR));
    lv_obj_set_style_text_font(cause_hdr, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(cause_hdr, lv_color_hex(0x991b1b), 0);
    lv_obj_set_style_base_dir(cause_hdr, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(cause_hdr, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(cause_hdr, 396);

    /* Cause Text */
    lv_obj_t *cause_lbl = lv_label_create(cause_card);
    lv_label_set_text(cause_lbl, cause ? cause : "---");
    lv_label_set_long_mode(cause_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(cause_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(cause_lbl, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_text_line_space(cause_lbl, 3, 0);
    lv_obj_set_style_base_dir(cause_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(cause_lbl, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(cause_lbl, 396);

    /* 2. Solution Container Card (Soft sky blue tint) */
    lv_obj_t *sol_card = lv_obj_create(body);
    lv_obj_set_width(sol_card, 412);
    lv_obj_set_height(sol_card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sol_card, lv_color_hex(0xf0f9ff), 0);
    lv_obj_set_style_border_width(sol_card, 1, 0);
    lv_obj_set_style_border_color(sol_card, lv_color_hex(0xbae6fd), 0);
    lv_obj_set_style_radius(sol_card, 8, 0);
    lv_obj_set_style_pad_all(sol_card, 4, 0);
    lv_obj_set_style_pad_row(sol_card, 2, 0);
    lv_obj_set_flex_flow(sol_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(sol_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(sol_card, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    /* Solution Header */
    lv_obj_t *sol_hdr = lv_label_create(sol_card);
    lv_label_set_text(sol_hdr, ui_lang_str(STR_FAULT_SOL_HDR));
    lv_obj_set_style_text_font(sol_hdr, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(sol_hdr, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_base_dir(sol_hdr, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(sol_hdr, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(sol_hdr, 396);

    /* Solution Text */
    lv_obj_t *sol_lbl = lv_label_create(sol_card);
    lv_label_set_text(sol_lbl, solution ? solution : "---");
    lv_label_set_long_mode(sol_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(sol_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(sol_lbl, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_text_line_space(sol_lbl, 3, 0);
    lv_obj_set_style_base_dir(sol_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(sol_lbl, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(sol_lbl, 396);

    /* Bottom Action Buttons */
    if (code == FAULT_INDUCTIVE_STALL) {
        /* ۱. دکمه تکرار مجدد (سبز) */
        lv_obj_t *btn_retry = lv_btn_create(card);
        lv_obj_set_pos(btn_retry, 300, 252);
        lv_obj_set_size(btn_retry, 134, 38);
        lv_obj_set_style_radius(btn_retry, 8, 0);
        lv_obj_set_style_bg_color(btn_retry, lv_color_hex(0x16a34a), 0);
        lv_obj_set_style_shadow_width(btn_retry, 0, 0);
        lv_obj_add_event_cb(btn_retry, action_fault_retry, LV_EVENT_CLICKED, (void *)(intptr_t)code);
        lv_obj_t *retry_lbl = lv_label_create(btn_retry);
        lv_label_set_text(retry_lbl, ui_lang_str(STR_FAULT_RETRY));
        lv_obj_set_style_text_font(retry_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(retry_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(retry_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(retry_lbl);

        /* ۲. دکمه بای‌پس موقت سنسور پولی (نارنجی صنعتی) */
        lv_obj_t *btn_disable = lv_btn_create(card);
        lv_obj_set_pos(btn_disable, 154, 252);
        lv_obj_set_size(btn_disable, 140, 38);
        lv_obj_set_style_radius(btn_disable, 8, 0);
        lv_obj_set_style_bg_color(btn_disable, lv_color_hex(0xea580c), 0);
        lv_obj_set_style_pad_hor(btn_disable, 2, 0);
        lv_obj_set_style_shadow_width(btn_disable, 0, 0);
        lv_obj_add_event_cb(btn_disable, action_fault_bypass_pulley_4h, LV_EVENT_CLICKED, NULL);
        lv_obj_t *dis_pulley_lbl = lv_label_create(btn_disable);
        bool is_fa_e01 = (ui_lang_get() == UI_LANG_FA);
        bool is_tr_e01 = (ui_lang_get() == UI_LANG_TR);
        lv_label_set_text(dis_pulley_lbl, is_fa_e01 ? "بای‌پس ۴ ساعته" : (is_tr_e01 ? "4s Baypas" : "4h Bypass"));
        lv_obj_set_style_text_font(dis_pulley_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(dis_pulley_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(dis_pulley_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(dis_pulley_lbl);

        /* ۳. دکمه رد خطا (طوسی) */
        lv_obj_t *btn_dismiss = lv_btn_create(card);
        lv_obj_set_pos(btn_dismiss, 14, 252);
        lv_obj_set_size(btn_dismiss, 134, 38);
        lv_obj_set_style_radius(btn_dismiss, 8, 0);
        lv_obj_set_style_bg_color(btn_dismiss, lv_color_hex(0x475569), 0);
        lv_obj_set_style_shadow_width(btn_dismiss, 0, 0);
        lv_obj_add_event_cb(btn_dismiss, action_fault_dismiss, LV_EVENT_CLICKED, NULL);
        lv_obj_t *dis_lbl = lv_label_create(btn_dismiss);
        lv_label_set_text(dis_lbl, ui_lang_str(STR_FAULT_DISMISS));
        lv_obj_set_style_text_font(dis_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(dis_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(dis_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(dis_lbl);
    } else if (code == FAULT_THERMO_TOP_DISCONNECT) {
        /* ۱. دکمه تکرار مجدد (سبز) */
        lv_obj_t *btn_retry = lv_btn_create(card);
        lv_obj_set_pos(btn_retry, 300, 252);
        lv_obj_set_size(btn_retry, 134, 38);
        lv_obj_set_style_radius(btn_retry, 8, 0);
        lv_obj_set_style_bg_color(btn_retry, lv_color_hex(0x16a34a), 0);
        lv_obj_set_style_shadow_width(btn_retry, 0, 0);
        lv_obj_add_event_cb(btn_retry, action_fault_retry, LV_EVENT_CLICKED, (void *)(intptr_t)code);
        lv_obj_t *retry_lbl = lv_label_create(btn_retry);
        lv_label_set_text(retry_lbl, ui_lang_str(STR_FAULT_RETRY));
        lv_obj_set_style_text_font(retry_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(retry_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(retry_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(retry_lbl);

        /* ۲. دکمه سرمایش ایمن زمانی (آبی نیلی اپل) */
        lv_obj_t *btn_fallback = lv_btn_create(card);
        lv_obj_set_pos(btn_fallback, 156, 252);
        lv_obj_set_size(btn_fallback, 138, 38);
        lv_obj_set_style_radius(btn_fallback, 8, 0);
        lv_obj_set_style_bg_color(btn_fallback, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_shadow_width(btn_fallback, 0, 0);
        lv_obj_add_event_cb(btn_fallback, action_fault_enable_thermo_fallback, LV_EVENT_CLICKED, NULL);
        lv_obj_t *fallback_lbl = lv_label_create(btn_fallback);
        lv_label_set_text(fallback_lbl, "سرمایش زمانی ایمن");
        lv_obj_set_style_text_font(fallback_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(fallback_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(fallback_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(fallback_lbl);

        /* ۳. دکمه رد خطا (طوسی) */
        lv_obj_t *btn_dismiss = lv_btn_create(card);
        lv_obj_set_pos(btn_dismiss, 14, 252);
        lv_obj_set_size(btn_dismiss, 134, 38);
        lv_obj_set_style_radius(btn_dismiss, 8, 0);
        lv_obj_set_style_bg_color(btn_dismiss, lv_color_hex(0x475569), 0);
        lv_obj_set_style_shadow_width(btn_dismiss, 0, 0);
        lv_obj_add_event_cb(btn_dismiss, action_fault_dismiss, LV_EVENT_CLICKED, NULL);
        lv_obj_t *dis_lbl = lv_label_create(btn_dismiss);
        lv_label_set_text(dis_lbl, ui_lang_str(STR_FAULT_DISMISS));
        lv_obj_set_style_text_font(dis_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(dis_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(dis_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(dis_lbl);
    } else {
        /* 1. Retry Button (Green) */
        lv_obj_t *btn_retry = lv_btn_create(card);
        lv_obj_set_pos(btn_retry, 240, 252);
        lv_obj_set_size(btn_retry, 180, 38);
        lv_obj_set_style_radius(btn_retry, 8, 0);
        lv_obj_set_style_bg_color(btn_retry, lv_color_hex(0x16a34a), 0);
        lv_obj_set_style_shadow_width(btn_retry, 0, 0);
        lv_obj_add_event_cb(btn_retry, action_fault_retry, LV_EVENT_CLICKED, (void *)(intptr_t)code);
        lv_obj_t *retry_lbl = lv_label_create(btn_retry);
        lv_label_set_text(retry_lbl, ui_lang_str(STR_FAULT_RETRY));
        lv_obj_set_style_text_font(retry_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(retry_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(retry_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(retry_lbl);

        /* 2. Dismiss Button (Slate/Red) */
        lv_obj_t *btn_dismiss = lv_btn_create(card);
        lv_obj_set_pos(btn_dismiss, 28, 252);
        lv_obj_set_size(btn_dismiss, 180, 38);
        lv_obj_set_style_radius(btn_dismiss, 8, 0);
        lv_obj_set_style_bg_color(btn_dismiss, lv_color_hex(0x475569), 0);
        lv_obj_set_style_shadow_width(btn_dismiss, 0, 0);
        lv_obj_add_event_cb(btn_dismiss, action_fault_dismiss, LV_EVENT_CLICKED, NULL);
        lv_obj_t *dis_lbl = lv_label_create(btn_dismiss);
        lv_label_set_text(dis_lbl, ui_lang_str(STR_FAULT_DISMISS));
        lv_obj_set_style_text_font(dis_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(dis_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(dis_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(dis_lbl);
    }
    lvgl_unlock();
}

/* =========================================================================
 * پنجره اعلان زمان‌دار هشدار سلامت سخت‌افزار در صفحه اصلی (Timed POST Alert)
 * ========================================================================= */
#define BOOT_ALERT_TICK_MS     50
#define BOOT_ALERT_TOTAL_MS    10000

static lv_obj_t *s_boot_alert_backdrop = NULL;
static lv_timer_t *s_boot_alert_timer = NULL;
static int32_t s_boot_alert_ms_left = BOOT_ALERT_TOTAL_MS;
static bool s_boot_alert_touch_held = false;
static lv_obj_t *s_boot_alert_countdown_bar = NULL;
static lv_obj_t *s_boot_alert_countdown_lbl = NULL;
static int s_boot_alert_last_sec = -1;

void hide_boot_timed_alert(void) {
    s_boot_alert_touch_held = false;
    s_boot_alert_last_sec = -1;
    if (s_boot_alert_timer) {
        lv_timer_delete(s_boot_alert_timer);
        s_boot_alert_timer = NULL;
    }
    if (s_boot_alert_backdrop && lv_obj_is_valid(s_boot_alert_backdrop)) {
        lv_obj_delete(s_boot_alert_backdrop);
    }
    s_boot_alert_backdrop = NULL;
    s_boot_alert_countdown_bar = NULL;
    s_boot_alert_countdown_lbl = NULL;
    objects.boot_alert_modal = NULL;
}

static void boot_alert_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_boot_alert_touch_held = true;
        if (s_boot_alert_countdown_bar && lv_obj_is_valid(s_boot_alert_countdown_bar)) {
            lv_obj_set_style_bg_color(s_boot_alert_countdown_bar, lv_color_hex(0x38bdf8), LV_PART_INDICATOR);
        }
        if (s_boot_alert_countdown_lbl && lv_obj_is_valid(s_boot_alert_countdown_lbl)) {
            int sec = (s_boot_alert_ms_left + 999) / 1000;
            char buf[96];
            bool is_fa = (ui_lang_get() == UI_LANG_FA);
            bool is_tr = (ui_lang_get() == UI_LANG_TR);
            if (is_fa) {
                snprintf(buf, sizeof(buf), "توقف تایمر حین لمس (%d ثانیه)", sec);
            } else if (is_tr) {
                snprintf(buf, sizeof(buf), "Dokunurken duraklatildi (%d sn)", sec);
            } else {
                snprintf(buf, sizeof(buf), "Timer paused while touching (%d s)", sec);
            }
            lv_label_set_text(s_boot_alert_countdown_lbl, buf);
            lv_obj_set_style_text_color(s_boot_alert_countdown_lbl, lv_color_hex(0x38bdf8), 0);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED) {
        s_boot_alert_touch_held = false;
        /* پس از رها کردن تاچ، حداقل ۵ ثانیه فرصت مطالعه یا انتخاب دکمه به اپراتور داده می‌شود */
        if (s_boot_alert_ms_left < 5000) {
            s_boot_alert_ms_left = 5000;
        }
        if (s_boot_alert_countdown_bar && lv_obj_is_valid(s_boot_alert_countdown_bar)) {
            const boot_diag_result_t *res = boot_diag_get_result();
            lv_color_t border_color = res->user_resolvable ? lv_color_hex(0xf59e0b) : lv_color_hex(0xef4444);
            lv_obj_set_style_bg_color(s_boot_alert_countdown_bar, border_color, LV_PART_INDICATOR);
        }
        if (s_boot_alert_countdown_lbl && lv_obj_is_valid(s_boot_alert_countdown_lbl)) {
            lv_obj_set_style_text_color(s_boot_alert_countdown_lbl, lv_color_hex(0xa1a1aa), 0);
            int sec = (s_boot_alert_ms_left + 999) / 1000;
            char buf[64];
            snprintf(buf, sizeof(buf), ui_lang_str(STR_BOOT_ALERT_AUTO), sec);
            lv_label_set_text(s_boot_alert_countdown_lbl, buf);
        }
    }
}

static void boot_alert_timer_cb(lv_timer_t *t) {
    (void)t;
    if (s_boot_alert_touch_held) {
        return; /* توقف کامل شمارش معکوس حین لمس اپراتور */
    }
    s_boot_alert_ms_left -= BOOT_ALERT_TICK_MS;
    if (s_boot_alert_ms_left <= 0) {
        hide_boot_timed_alert();
        return;
    }
    if (s_boot_alert_countdown_bar && lv_obj_is_valid(s_boot_alert_countdown_bar)) {
        /* تخلیه فوق‌العاده نرم با دقت ۵۰ میلی‌ثانیه و ۱۰۰۰ واحد وضوح */
        int32_t val = (s_boot_alert_ms_left * 1000) / BOOT_ALERT_TOTAL_MS;
        if (val < 0) val = 0;
        if (val > 1000) val = 1000;
        lv_bar_set_value(s_boot_alert_countdown_bar, val, LV_ANIM_OFF);
    }
    if (s_boot_alert_countdown_lbl && lv_obj_is_valid(s_boot_alert_countdown_lbl)) {
        int sec = (s_boot_alert_ms_left + 999) / 1000;
        if (sec != s_boot_alert_last_sec) {
            s_boot_alert_last_sec = sec;
            char buf[64];
            snprintf(buf, sizeof(buf), ui_lang_str(STR_BOOT_ALERT_AUTO), sec);
            lv_label_set_text(s_boot_alert_countdown_lbl, buf);
        }
    }
}

static void boot_alert_dismiss_btn_cb(lv_event_t *e) {
    (void)e;
    buzzer_beep_click();
    boot_diag_dismiss();
    hide_boot_timed_alert();
}

static void boot_alert_view_btn_cb(lv_event_t *e) {
    (void)e;
    buzzer_beep_click();
    hide_boot_timed_alert();
    boot_diag_dismiss();

    const boot_diag_result_t *res = boot_diag_get_result();
    ui_lang_t lang = ui_lang_get();
    const char *title = ui_lang_str(STR_BOOT_ALERT_TITLE);
    const char *cause = (lang == UI_LANG_EN) ? res->cause_en : ((lang == UI_LANG_TR) ? res->cause_tr : res->cause_fa);
    const char *sol = (lang == UI_LANG_EN) ? res->solution_en : ((lang == UI_LANG_TR) ? res->solution_tr : res->solution_fa);

    show_interactive_fault_dialog(15, title, cause, sol);
}

void show_boot_timed_alert(void) {
    if (s_boot_alert_backdrop && lv_obj_is_valid(s_boot_alert_backdrop)) return;
    if (!boot_diag_has_error() || boot_diag_is_dismissed()) return;

    hide_boot_timed_alert();

    s_boot_alert_ms_left = BOOT_ALERT_TOTAL_MS;
    s_boot_alert_touch_held = false;
    s_boot_alert_last_sec = -1;

    // لایه مات پس‌زمینه (Modal Backdrop)
    s_boot_alert_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_boot_alert_backdrop, 0, 0);
    lv_obj_set_size(s_boot_alert_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_boot_alert_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_boot_alert_backdrop, 160, 0);
    lv_obj_set_style_border_width(s_boot_alert_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_boot_alert_backdrop, 0, 0);
    lv_obj_clear_flag(s_boot_alert_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_boot_alert_backdrop, boot_alert_touch_cb, LV_EVENT_ALL, NULL);

    // کارت هشدار Apple HIG با بوردر اخطار
    lv_obj_t *card = lv_obj_create(s_boot_alert_backdrop);
    objects.boot_alert_modal = card;
    lv_obj_set_pos(card, 24, 16);
    lv_obj_set_size(card, 432, 284);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x18181b), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_add_event_cb(card, boot_alert_touch_cb, LV_EVENT_ALL, NULL);

    const boot_diag_result_t *res = boot_diag_get_result();
    lv_color_t border_color = res->user_resolvable ? lv_color_hex(0xf59e0b) : lv_color_hex(0xef4444);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // ۱. هدر کارت: آیکون اخطار + عنوان
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, 408, 32);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 8, 0);

    lv_obj_t *icon_lbl = lv_label_create(hdr);
    lv_label_set_text(icon_lbl, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon_lbl, border_color, 0);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_24, 0);

    lv_obj_t *title_lbl = lv_label_create(hdr);
    lv_label_set_text(title_lbl, ui_lang_str(STR_BOOT_ALERT_TITLE));
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_20(), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);

    // خط جداکننده
    lv_obj_t *div = lv_obj_create(card);
    lv_obj_remove_style_all(div);
    lv_obj_set_pos(div, 0, 36);
    lv_obj_set_size(div, 408, 1);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x27272a), 0);
    lv_obj_set_style_bg_opa(div, 255, 0);

    // ۲. کادر متن خلاصه موارد شناسایی‌شده
    lv_obj_t *body_box = lv_obj_create(card);
    lv_obj_set_pos(body_box, 0, 42);
    lv_obj_set_size(body_box, 408, 120);
    lv_obj_set_style_bg_color(body_box, lv_color_hex(0x27272a), 0);
    lv_obj_set_style_radius(body_box, 8, 0);
    lv_obj_set_style_border_width(body_box, 0, 0);
    lv_obj_set_style_pad_all(body_box, 8, 0);
    lv_obj_set_scroll_dir(body_box, LV_DIR_VER);
    lv_obj_clear_flag(body_box, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_anim_duration(body_box, 0, 0);
    lv_obj_add_event_cb(body_box, boot_alert_touch_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *sum_lbl = lv_label_create(body_box);
    ui_lang_t lang = ui_lang_get();
    const char *sum_text = (lang == UI_LANG_EN) ? res->summary_en : ((lang == UI_LANG_TR) ? res->summary_tr : res->summary_fa);
    lv_label_set_text(sum_lbl, sum_text);
    lv_obj_set_width(sum_lbl, 384);
    lv_obj_set_style_text_font(sum_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(sum_lbl, lv_color_hex(0xfef08a), 0);
    lv_obj_set_style_base_dir(sum_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(sum_lbl, (lang == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_add_event_cb(sum_lbl, boot_alert_touch_cb, LV_EVENT_ALL, NULL);

    // ۳. نوار پیشرفت تخلیه زمان‌سنج (y=168)
    s_boot_alert_countdown_bar = lv_bar_create(card);
    lv_obj_set_pos(s_boot_alert_countdown_bar, 0, 168);
    lv_obj_set_size(s_boot_alert_countdown_bar, 408, 5);
    lv_obj_set_style_bg_color(s_boot_alert_countdown_bar, lv_color_hex(0x3f3f46), LV_PART_MAIN);
    lv_obj_set_style_radius(s_boot_alert_countdown_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_boot_alert_countdown_bar, border_color, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_boot_alert_countdown_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(s_boot_alert_countdown_bar, 0, 1000);
    lv_bar_set_value(s_boot_alert_countdown_bar, 1000, LV_ANIM_OFF);

    // برچسب زمان معکوس (y=176) — فونت سه‌زبانه هماهنگ با Zero Tofu
    s_boot_alert_countdown_lbl = lv_label_create(card);
    char buf[64];
    snprintf(buf, sizeof(buf), ui_lang_str(STR_BOOT_ALERT_AUTO), (s_boot_alert_ms_left + 999) / 1000);
    lv_label_set_text(s_boot_alert_countdown_lbl, buf);
    lv_obj_set_pos(s_boot_alert_countdown_lbl, 0, 176);
    lv_obj_set_size(s_boot_alert_countdown_lbl, 408, 20);
    lv_obj_set_style_text_align(s_boot_alert_countdown_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_boot_alert_countdown_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_base_dir(s_boot_alert_countdown_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_color(s_boot_alert_countdown_lbl, lv_color_hex(0xa1a1aa), 0);

    // ۴. دکمه‌های تعاملی پایین (y=208, ارتفاع ۴۴ پیکسل طبق Apple HIG)
    // دکمه مشاهده و راهکار
    lv_obj_t *btn_view = lv_btn_create(card);
    lv_obj_set_pos(btn_view, 212, 208);
    lv_obj_set_size(btn_view, 196, 44);
    lv_obj_set_style_radius(btn_view, 10, 0);
    lv_obj_set_style_bg_color(btn_view, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(btn_view, 0, 0);
    lv_obj_add_event_cb(btn_view, boot_alert_view_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *view_lbl = lv_label_create(btn_view);
    lv_label_set_text(view_lbl, ui_lang_str(STR_BOOT_ALERT_VIEW));
    lv_obj_set_style_text_font(view_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(view_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(view_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(view_lbl);

    // دکمه بستن
    lv_obj_t *btn_close = lv_btn_create(card);
    lv_obj_set_pos(btn_close, 0, 208);
    lv_obj_set_size(btn_close, 196, 44);
    lv_obj_set_style_radius(btn_close, 10, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, boot_alert_dismiss_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_lbl = lv_label_create(btn_close);
    lv_label_set_text(close_lbl, ui_lang_str(STR_BOOT_ALERT_DISMISS));
    lv_obj_set_style_text_font(close_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(close_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(close_lbl);

    // تایمر ۵۰ میلی‌ثانیه‌ای جهت تخلیه فوق‌روان نوار زمان‌سنج (Smooth Glide)
    s_boot_alert_timer = lv_timer_create(boot_alert_timer_cb, BOOT_ALERT_TICK_MS, NULL);
    buzzer_beep_click();
}

/* =========================================================================
 * پنجره تنظیم پولاریته کنترل‌بار (Overload Polarity Modal: N.O. / N.C.)
 * ========================================================================= */
static lv_obj_t *s_overload_polarity_backdrop = NULL;
static lv_timer_t *s_overload_hold_timer = NULL;
static bool s_overload_hold_is_motor = false;
static lv_obj_t *s_overload_hold_row = NULL;

void hide_overload_polarity_modal(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    if (s_overload_polarity_backdrop && lv_obj_is_valid(s_overload_polarity_backdrop)) {
        lv_obj_delete(s_overload_polarity_backdrop);
    }
    s_overload_polarity_backdrop = NULL;
    lvgl_unlock();
}

static void overload_polarity_select_cb(lv_event_t *e)
{
    intptr_t ud = (intptr_t)lv_event_get_user_data(e);
    bool is_motor = (ud & 0x01);
    bool is_no = ((ud & 0x02) != 0);

    buzzer_beep_confirm();
    analog_inputs_set_overload_polarity(is_motor, is_no);
    hide_overload_polarity_modal();
    panel_nav_toast_success(ui_lang_str(STR_SETTINGS_SAVED));
}

static void overload_polarity_close_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    hide_overload_polarity_modal();
}

void show_overload_polarity_modal(bool is_motor)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    hide_overload_polarity_modal();

    s_overload_polarity_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_overload_polarity_backdrop, 0, 0);
    lv_obj_set_size(s_overload_polarity_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_overload_polarity_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overload_polarity_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_overload_polarity_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_overload_polarity_backdrop, 0, 0);
    lv_obj_clear_flag(s_overload_polarity_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    /* Modal Card */
    lv_obj_t *card = lv_obj_create(s_overload_polarity_backdrop);
    lv_obj_set_pos(card, 55, 45);
    lv_obj_set_size(card, 370, 230);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 42);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, ui_lang_str(STR_OVERLOAD_POLARITY));
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(title_lbl);

    /* کپسول عنوان ورودی انتخاب شده با متن زرد درخشان جهت نمایش ۱۰۰٪ قطعی انتخاب کاربر */
    lv_obj_t *chan_badge = lv_obj_create(card);
    lv_obj_set_pos(chan_badge, 25, 46);
    lv_obj_set_size(chan_badge, 320, 28);
    lv_obj_set_style_bg_color(chan_badge, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(chan_badge, 6, 0);
    lv_obj_set_style_border_width(chan_badge, 1, 0);
    lv_obj_set_style_border_color(chan_badge, lv_color_hex(0xeab308), 0);
    lv_obj_set_style_pad_all(chan_badge, 0, 0);
    lv_obj_clear_flag(chan_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sub_lbl = lv_label_create(chan_badge);
    ui_lang_t lang = ui_lang_get();
    const char *chan_name;
    if (lang == UI_LANG_FA) {
        chan_name = is_motor ? "کنترل‌بار موتور پارو" : "کنترل‌بار کمپرسور بزرگ";
    } else if (lang == UI_LANG_TR) {
        chan_name = is_motor ? "Karistirici Motor Asiri Yuk (Giris I3)" : "Kompresor Asiri Yuk (Giris I5)";
    } else {
        chan_name = is_motor ? "Paddle Motor Overload (Input I3)" : "Compressor Overload (Input I5)";
    }
    lv_label_set_text(sub_lbl, chan_name);
    lv_obj_set_style_text_font(sub_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0xfacc15), 0);
    lv_obj_set_style_base_dir(sub_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(sub_lbl);

    bool cur_no = analog_inputs_get_overload_polarity(is_motor);

    /* Button N.O. */
    lv_obj_t *btn_no = lv_btn_create(card);
    lv_obj_set_pos(btn_no, 25, 80);
    lv_obj_set_size(btn_no, 320, 42);
    lv_obj_set_style_radius(btn_no, 8, 0);
    lv_obj_set_style_bg_color(btn_no, cur_no ? lv_color_hex(0xeff6ff) : lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_width(btn_no, cur_no ? 2 : 1, 0);
    lv_obj_set_style_border_color(btn_no, cur_no ? lv_color_hex(0x0284c7) : lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(btn_no, 0, 0);
    intptr_t ud_no = (is_motor ? 1 : 0) | 2; /* is_motor + is_no=1 */
    lv_obj_add_event_cb(btn_no, overload_polarity_select_cb, LV_EVENT_CLICKED, (void *)ud_no);

    lv_obj_t *lbl_no = lv_label_create(btn_no);
    lv_label_set_text(lbl_no, ui_lang_str(STR_POLARITY_NO));
    lv_obj_set_style_text_font(lbl_no, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_no, cur_no ? lv_color_hex(0x0284c7) : lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_base_dir(lbl_no, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_no);

    /* Button N.C. */
    lv_obj_t *btn_nc = lv_btn_create(card);
    lv_obj_set_pos(btn_nc, 25, 128);
    lv_obj_set_size(btn_nc, 320, 42);
    lv_obj_set_style_radius(btn_nc, 8, 0);
    lv_obj_set_style_bg_color(btn_nc, !cur_no ? lv_color_hex(0xeff6ff) : lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_width(btn_nc, !cur_no ? 2 : 1, 0);
    lv_obj_set_style_border_color(btn_nc, !cur_no ? lv_color_hex(0x0284c7) : lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(btn_nc, 0, 0);
    intptr_t ud_nc = (is_motor ? 1 : 0); /* is_motor + is_no=0 */
    lv_obj_add_event_cb(btn_nc, overload_polarity_select_cb, LV_EVENT_CLICKED, (void *)ud_nc);

    lv_obj_t *lbl_nc = lv_label_create(btn_nc);
    lv_label_set_text(lbl_nc, ui_lang_str(STR_POLARITY_NC));
    lv_obj_set_style_text_font(lbl_nc, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_nc, !cur_no ? lv_color_hex(0x0284c7) : lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_base_dir(lbl_nc, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_nc);

    /* Close Button */
    lv_obj_t *btn_close = lv_btn_create(card);
    lv_obj_set_pos(btn_close, 115, 180);
    lv_obj_set_size(btn_close, 140, 36);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, overload_polarity_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, ui_lang_str(STR_CLOSE_DIALOG));
    lv_obj_set_style_text_font(lbl_close, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_close, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(lbl_close, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_close);

    lvgl_unlock();
}

static void overload_hold_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_overload_hold_timer) {
        lv_timer_delete(s_overload_hold_timer);
        s_overload_hold_timer = NULL;
    }
    int idx = s_overload_hold_is_motor ? 3 : 5;
    lv_obj_t *lbl = (idx < 8) ? objects.lbl_in[idx] : NULL;
    if (s_overload_hold_row && lv_obj_is_valid(s_overload_hold_row)) {
        lv_obj_set_style_bg_opa(s_overload_hold_row, 0, 0);
    }
    if (lbl && lv_obj_is_valid(lbl)) {
        lv_obj_set_style_bg_color(lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x0f172a), 0);
    }
    buzzer_beep_click();
    show_overload_polarity_modal(s_overload_hold_is_motor);
}

static void overload_row_press_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *row = lv_event_get_target(e);
    bool is_motor = (bool)(intptr_t)lv_event_get_user_data(e);
    int idx = is_motor ? 3 : 5;
    lv_obj_t *lbl = (idx < 8) ? objects.lbl_in[idx] : NULL;

    if (code == LV_EVENT_PRESSED) {
        s_overload_hold_row = row;
        s_overload_hold_is_motor = is_motor;

        /* افکت لمسی هوشمند: پس‌زمینه دارک اسلیت با متن زرد طلایی درخشان */
        lv_obj_set_style_bg_color(row, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 4, 0);

        if (lbl && lv_obj_is_valid(lbl)) {
            lv_obj_set_style_bg_color(lbl, lv_color_hex(0x1e293b), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xfacc15), 0); /* متن زرد طلایی */
        }

        if (s_overload_hold_timer) {
            lv_timer_delete(s_overload_hold_timer);
        }
        s_overload_hold_timer = lv_timer_create(overload_hold_timer_cb, 1200, NULL);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_overload_hold_timer) {
            lv_timer_delete(s_overload_hold_timer);
            s_overload_hold_timer = NULL;
        }
        if (row && lv_obj_is_valid(row)) {
            lv_obj_set_style_bg_opa(row, 0, 0);
        }
        if (lbl && lv_obj_is_valid(lbl)) {
            lv_obj_set_style_bg_color(lbl, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x0f172a), 0); /* بازگشت به مشکی عادی */
        }
    }
}

/* =========================================================================
 * پنجره کالیبراسیون و تصحیح دمای T1 و T2 (Temperature Calibration Modal)
 * ========================================================================= */
static lv_obj_t *s_temp_cal_backdrop = NULL;
static bool s_cal_is_hopper = false;
static float s_cal_working_offset = 0.0f;
static lv_obj_t *s_cal_offset_lbl = NULL;
static lv_obj_t *s_cal_final_lbl = NULL;
static lv_obj_t *s_cal_raw_lbl = NULL;

static void update_temp_cal_previews(void)
{
    float raw = s_cal_is_hopper ? max6675_get_raw_temp_top() : max6675_get_raw_temp_cyl();
    bool valid = s_cal_is_hopper ? max6675_is_top_valid() : max6675_is_cylinder_valid();

    if (s_cal_offset_lbl && lv_obj_is_valid(s_cal_offset_lbl)) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%+.1f°C", s_cal_working_offset);
        lv_label_set_text(s_cal_offset_lbl, buf);
    }

    if (s_cal_raw_lbl && lv_obj_is_valid(s_cal_raw_lbl)) {
        char buf[32];
        if (valid) {
            snprintf(buf, sizeof(buf), "%+.1f°C", raw);
        } else {
            snprintf(buf, sizeof(buf), "--.-°C");
        }
        lv_label_set_text(s_cal_raw_lbl, buf);
    }

    if (s_cal_final_lbl && lv_obj_is_valid(s_cal_final_lbl)) {
        char buf[32];
        if (valid) {
            snprintf(buf, sizeof(buf), "%+.1f°C", raw + s_cal_working_offset);
        } else {
            snprintf(buf, sizeof(buf), "--.-°C");
        }
        lv_label_set_text(s_cal_final_lbl, buf);
    }
}

void hide_temp_calibration_modal(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    if (s_temp_cal_backdrop && lv_obj_is_valid(s_temp_cal_backdrop)) {
        lv_obj_delete(s_temp_cal_backdrop);
    }
    s_temp_cal_backdrop = NULL;
    s_cal_offset_lbl = NULL;
    s_cal_final_lbl = NULL;
    s_cal_raw_lbl = NULL;
    lvgl_unlock();
}

static void temp_cal_step_cb(lv_event_t *e)
{
    int delta_x10 = (intptr_t)lv_event_get_user_data(e);
    s_cal_working_offset += ((float)delta_x10 / 10.0f);
    if (s_cal_working_offset < -15.0f) s_cal_working_offset = -15.0f;
    if (s_cal_working_offset > 15.0f)  s_cal_working_offset = 15.0f;
    buzzer_beep_click();
    update_temp_cal_previews();
}

static void temp_cal_reset_cb(lv_event_t *e)
{
    (void)e;
    s_cal_working_offset = 0.0f;
    buzzer_beep_click();
    update_temp_cal_previews();
}

static void temp_cal_save_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_confirm();
    if (s_cal_is_hopper) {
        max6675_set_cal_offset_top(s_cal_working_offset);
    } else {
        max6675_set_cal_offset_cyl(s_cal_working_offset);
    }
    hide_temp_calibration_modal();
    panel_nav_toast_success(ui_lang_str(STR_SETTINGS_SAVED));
}

static void temp_cal_close_cb(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    hide_temp_calibration_modal();
}

void show_temp_calibration_modal(bool is_hopper)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    hide_temp_calibration_modal();

    s_cal_is_hopper = is_hopper;
    s_cal_working_offset = is_hopper ? max6675_get_cal_offset_top() : max6675_get_cal_offset_cyl();

    s_temp_cal_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_temp_cal_backdrop, 0, 0);
    lv_obj_set_size(s_temp_cal_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_temp_cal_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_temp_cal_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_temp_cal_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_temp_cal_backdrop, 0, 0);
    lv_obj_clear_flag(s_temp_cal_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    /* Modal Card */
    lv_obj_t *card = lv_obj_create(s_temp_cal_backdrop);
    lv_obj_set_pos(card, 45, 25);
    lv_obj_set_size(card, 390, 270);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_lbl = lv_label_create(header);
    ui_lang_t lang = ui_lang_get();
    const char *hdr_title;
    if (lang == UI_LANG_FA) {
        hdr_title = is_hopper ? "کالیبراسیون دمای مخزن شیر" : "کالیبراسیون دمای سیلندر انجماد";
    } else if (lang == UI_LANG_TR) {
        hdr_title = is_hopper ? "Sut Tanki Sicaklik Kalibrasyonu (T1)" : "Silindir Sicaklik Kalibrasyonu (T2)";
    } else {
        hdr_title = is_hopper ? "Milk Tank Temp Calibration (T1)" : "Cylinder Temp Calibration (T2)";
    }
    lv_label_set_text(title_lbl, hdr_title);
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xfacc15), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(title_lbl);

    /* 3 Info Boxes (Raw, Offset, Final) */
    /* 1. Raw Box (Left) */
    lv_obj_t *box_raw = lv_obj_create(card);
    lv_obj_set_pos(box_raw, 18, 48);
    lv_obj_set_size(box_raw, 110, 56);
    lv_obj_set_style_bg_color(box_raw, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_width(box_raw, 1, 0);
    lv_obj_set_style_border_color(box_raw, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_radius(box_raw, 8, 0);
    lv_obj_set_style_pad_all(box_raw, 2, 0);
    lv_obj_clear_flag(box_raw, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_r_t = lv_label_create(box_raw);
    lv_label_set_text(lbl_r_t, ui_lang_str(STR_RAW_TEMP));
    lv_obj_set_style_text_font(lbl_r_t, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_r_t, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(lbl_r_t, ui_lang_base_dir(), 0);
    lv_obj_align(lbl_r_t, LV_ALIGN_TOP_MID, 0, 2);

    s_cal_raw_lbl = lv_label_create(box_raw);
    lv_obj_set_style_text_font(s_cal_raw_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_cal_raw_lbl, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_base_dir(s_cal_raw_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_align(s_cal_raw_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* 2. Offset Box (Center) */
    lv_obj_t *box_off = lv_obj_create(card);
    lv_obj_set_pos(box_off, 140, 48);
    lv_obj_set_size(box_off, 110, 56);
    lv_obj_set_style_bg_color(box_off, lv_color_hex(0xf0fdf4), 0);
    lv_obj_set_style_border_width(box_off, 1, 0);
    lv_obj_set_style_border_color(box_off, lv_color_hex(0xbbf7d0), 0);
    lv_obj_set_style_radius(box_off, 8, 0);
    lv_obj_set_style_pad_all(box_off, 2, 0);
    lv_obj_clear_flag(box_off, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_o_t = lv_label_create(box_off);
    lv_label_set_text(lbl_o_t, ui_lang_str(STR_CAL_OFFSET));
    lv_obj_set_style_text_font(lbl_o_t, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_o_t, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_base_dir(lbl_o_t, ui_lang_base_dir(), 0);
    lv_obj_align(lbl_o_t, LV_ALIGN_TOP_MID, 0, 2);

    s_cal_offset_lbl = lv_label_create(box_off);
    lv_obj_set_style_text_font(s_cal_offset_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_cal_offset_lbl, lv_color_hex(0x15803d), 0);
    lv_obj_set_style_base_dir(s_cal_offset_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_align(s_cal_offset_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* 3. Final Box (Right) */
    lv_obj_t *box_fin = lv_obj_create(card);
    lv_obj_set_pos(box_fin, 262, 48);
    lv_obj_set_size(box_fin, 110, 56);
    lv_obj_set_style_bg_color(box_fin, lv_color_hex(0xf0f9ff), 0);
    lv_obj_set_style_border_width(box_fin, 1, 0);
    lv_obj_set_style_border_color(box_fin, lv_color_hex(0xbae6fd), 0);
    lv_obj_set_style_radius(box_fin, 8, 0);
    lv_obj_set_style_pad_all(box_fin, 2, 0);
    lv_obj_clear_flag(box_fin, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_f_t = lv_label_create(box_fin);
    lv_label_set_text(lbl_f_t, ui_lang_str(STR_FINAL_TEMP));
    lv_obj_set_style_text_font(lbl_f_t, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_f_t, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_base_dir(lbl_f_t, ui_lang_base_dir(), 0);
    lv_obj_align(lbl_f_t, LV_ALIGN_TOP_MID, 0, 2);

    s_cal_final_lbl = lv_label_create(box_fin);
    lv_obj_set_style_text_font(s_cal_final_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_cal_final_lbl, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_base_dir(s_cal_final_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_align(s_cal_final_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Stepper Controls: [-] [Reset to 0] [+] */
    /* [-] Button */
    lv_obj_t *btn_minus = lv_btn_create(card);
    lv_obj_set_pos(btn_minus, 40, 120);
    lv_obj_set_size(btn_minus, 70, 44);
    lv_obj_set_style_radius(btn_minus, 8, 0);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(btn_minus, 0, 0);
    lv_obj_set_ext_click_area(btn_minus, 8);
    lv_obj_add_flag(btn_minus, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(btn_minus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_minus, temp_cal_step_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-5);

    lv_obj_t *lbl_m = lv_label_create(btn_minus);
    lv_label_set_text(lbl_m, "-0.5");
    lv_obj_set_style_text_font(lbl_m, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_m, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_m);
    lv_obj_clear_flag(lbl_m, LV_OBJ_FLAG_CLICKABLE);

    /* Reset to Zero Button */
    lv_obj_t *btn_rst = lv_btn_create(card);
    lv_obj_set_pos(btn_rst, 130, 120);
    lv_obj_set_size(btn_rst, 130, 44);
    lv_obj_set_style_radius(btn_rst, 8, 0);
    lv_obj_set_style_bg_color(btn_rst, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_width(btn_rst, 1, 0);
    lv_obj_set_style_border_color(btn_rst, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(btn_rst, 0, 0);
    lv_obj_add_event_cb(btn_rst, temp_cal_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_rst = lv_label_create(btn_rst);
    lv_label_set_text(lbl_rst, ui_lang_str(STR_CAL_RESET_ZERO));
    lv_obj_set_style_text_font(lbl_rst, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_rst, lv_color_hex(0x334155), 0);
    lv_obj_set_style_base_dir(lbl_rst, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_rst);
    lv_obj_clear_flag(lbl_rst, LV_OBJ_FLAG_CLICKABLE);

    /* [+] Button */
    lv_obj_t *btn_plus = lv_btn_create(card);
    lv_obj_set_pos(btn_plus, 280, 120);
    lv_obj_set_size(btn_plus, 70, 44);
    lv_obj_set_style_radius(btn_plus, 8, 0);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(btn_plus, 0, 0);
    lv_obj_set_ext_click_area(btn_plus, 8);
    lv_obj_add_flag(btn_plus, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(btn_plus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_plus, temp_cal_step_cb, LV_EVENT_CLICKED, (void *)(intptr_t)5);

    lv_obj_t *lbl_p = lv_label_create(btn_plus);
    lv_label_set_text(lbl_p, "+0.5");
    lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_p, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_p);
    lv_obj_clear_flag(lbl_p, LV_OBJ_FLAG_CLICKABLE);

    /* Bottom Action Buttons: Save & Cancel */
    /* Save Button (Green) */
    lv_obj_t *btn_save = lv_btn_create(card);
    lv_obj_set_pos(btn_save, 210, 185);
    lv_obj_set_size(btn_save, 140, 44);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_shadow_width(btn_save, 0, 0);
    lv_obj_add_event_cb(btn_save, temp_cal_save_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, ui_lang_str(STR_CAL_SAVE));
    lv_obj_set_style_text_font(lbl_save, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_save, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(lbl_save, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_save);

    /* Cancel Button (Slate) */
    lv_obj_t *btn_cancel = lv_btn_create(card);
    lv_obj_set_pos(btn_cancel, 40, 185);
    lv_obj_set_size(btn_cancel, 140, 44);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_add_event_cb(btn_cancel, temp_cal_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, ui_lang_str(STR_CLOSE_DIALOG));
    lv_obj_set_style_text_font(lbl_cancel, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(lbl_cancel, ui_lang_base_dir(), 0);
    lv_obj_center(lbl_cancel);

    update_temp_cal_previews();
    lvgl_unlock();
}

static void temp_card_press_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *card = lv_event_get_target(e);
    bool is_hopper = ((intptr_t)lv_event_get_user_data(e) == 1);
    lv_obj_t *title = is_hopper ? objects.diag_t1_lbl : objects.diag_t2_lbl;
    lv_obj_t *val = is_hopper ? objects.diag_temp_hopper_val : objects.diag_temp_cyl_val;

    if (code == LV_EVENT_PRESSED) {
        /* افکت لمسی هوشمند: پس‌زمینه دارک اسلیت با کادر و متن زرد طلایی درخشان */
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0xeab308), 0);
        if (title && lv_obj_is_valid(title)) {
            lv_obj_set_style_text_color(title, lv_color_hex(0xfacc15), 0);
        }
        if (val && lv_obj_is_valid(val)) {
            lv_obj_set_style_text_color(val, lv_color_hex(0xfacc15), 0);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
        if (title && lv_obj_is_valid(title)) {
            lv_obj_set_style_text_color(title, lv_color_hex(0x1e3a8a), 0);
        }
        if (val && lv_obj_is_valid(val)) {
            lv_obj_set_style_text_color(val, lv_color_hex(is_hopper ? 0x0284c7 : 0x0369a1), 0);
        }
    } else if (code == LV_EVENT_LONG_PRESSED) {
        lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
        if (title && lv_obj_is_valid(title)) {
            lv_obj_set_style_text_color(title, lv_color_hex(0x1e3a8a), 0);
        }
        if (val && lv_obj_is_valid(val)) {
            lv_obj_set_style_text_color(val, lv_color_hex(is_hopper ? 0x0284c7 : 0x0369a1), 0);
        }
        buzzer_beep_click();
        show_temp_calibration_modal(is_hopper);
    }
}


/* =========================================================================
 * سیستم راهنمای لمسی و سناریوی هوشمند کلیدها (Smart Guide Modal - Apple HIG)
 * ========================================================================= */
typedef struct {
    const char *title;
    const char *desc;
} tooltip_data_t;

static lv_obj_t *s_tooltip_backdrop = NULL;
static lv_obj_t *s_guide_card = NULL;
static lv_obj_t *s_guide_title_lbl = NULL;
static lv_obj_t *s_guide_mode_bar = NULL;
static lv_obj_t *s_guide_mode_btns[6] = {NULL};
static lv_obj_t *s_guide_mode_lbls[6] = {NULL};
static lv_obj_t *s_guide_scroll = NULL;
static bool s_smart_guide_active = false;
static uint32_t s_last_guide_long_press_tick = 0;
static guide_id_t s_current_guide_id = GUIDE_ID_MAKE;

bool ui_is_smart_guide_active(void)
{
    return s_smart_guide_active && (s_tooltip_backdrop != NULL);
}

uint32_t ui_get_last_guide_long_press_tick(void)
{
    return s_last_guide_long_press_tick;
}

void ui_hide_tooltip(void)
{
    if (s_tooltip_backdrop && lv_obj_is_valid(s_tooltip_backdrop)) {
        lv_obj_delete(s_tooltip_backdrop);
    }
    s_tooltip_backdrop = NULL;
    s_guide_card = NULL;
    s_guide_title_lbl = NULL;
    s_guide_mode_bar = NULL;
    for (int i = 0; i < 6; i++) {
        s_guide_mode_btns[i] = NULL;
        s_guide_mode_lbls[i] = NULL;
    }
    s_guide_scroll = NULL;
    s_smart_guide_active = false;
}

static void tooltip_close_cb(lv_event_t *e)
{
    (void)e;
    ui_hide_tooltip();
}

static void guide_mode_pill_click_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_target(e);
    intptr_t gid = (intptr_t)lv_obj_get_user_data(btn);
    if ((gid >= 1 && gid <= 6) || (gid >= (intptr_t)GUIDE_ID_TAB_F1 && gid <= (intptr_t)GUIDE_ID_TAB_F6)) {
        buzzer_beep_click();
        ui_show_smart_guide((guide_id_t)gid);
    }
}

typedef struct {
    const char *badge;
    uint32_t badge_color;
    const char *title;
    const char *desc;
    const char *tip;
} guide_card_item_t;

static void sanitize_text_no_emoji(const char *src, char *dst, size_t dst_sz)
{
    if (!dst || dst_sz == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t d = 0;
    while (*src && d < dst_sz - 1) {
        unsigned char c = (unsigned char)*src;
        /* 4-byte UTF-8 emojis (e.g. 💡 0xF0 0x9F 0x92 0xA1) */
        if (c >= 0xF0 && c <= 0xF4) {
            src++;
            for (int k = 0; k < 3 && *src && ((unsigned char)*src & 0xC0) == 0x80; k++) {
                src++;
            }
            continue;
        }
        /* 3-byte warning emoji ⚠️ 0xE2 0x9A 0xA0 */
        if (c == 0xE2 && (unsigned char)*(src+1) == 0x9A && (unsigned char)*(src+2) == 0xA0) {
            src += 3;
            if ((unsigned char)*src == 0xEF && (unsigned char)*(src+1) == 0xB8 && (unsigned char)*(src+2) == 0x8F) {
                src += 3;
            }
            continue;
        }
        dst[d++] = *src++;
    }
    dst[d] = '\0';
}

static void populate_rich_guide(lv_obj_t *scroll, const char *objective,
                                const guide_card_item_t *items, int item_count,
                                const char *status_line)
{
    if (!scroll || !lv_obj_is_valid(scroll)) return;
    lv_obj_clean(scroll);
    lv_obj_scroll_to_y(scroll, 0, LV_ANIM_OFF);

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    /* ۱. کارت معرفی و هدف کلی فرآیند (Apple Sky Banner) */
    if (objective && objective[0]) {
        lv_obj_t *obj_card = lv_obj_create(scroll);
        lv_obj_set_width(obj_card, lv_pct(100));
        lv_obj_set_height(obj_card, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(obj_card, lv_color_hex(0xf0f9ff), 0);
        lv_obj_set_style_border_color(obj_card, lv_color_hex(0xbae6fd), 0);
        lv_obj_set_style_border_width(obj_card, 1, 0);
        lv_obj_set_style_radius(obj_card, 8, 0);
        lv_obj_set_style_pad_all(obj_card, 6, 0);
        lv_obj_set_style_pad_row(obj_card, 4, 0);
        lv_obj_set_flex_flow(obj_card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_base_dir(obj_card, ui_lang_base_dir(), 0);
        lv_obj_clear_flag(obj_card, LV_OBJ_FLAG_SCROLLABLE);

        /* هدر ردیف: هم‌راستایی کامل بج در جهت RTL */
        lv_obj_t *hdr_row = lv_obj_create(obj_card);
        lv_obj_set_width(hdr_row, lv_pct(100));
        lv_obj_set_height(hdr_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(hdr_row, 0, 0);
        lv_obj_set_style_border_width(hdr_row, 0, 0);
        lv_obj_set_style_pad_all(hdr_row, 0, 0);
        lv_obj_set_flex_flow(hdr_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(hdr_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_base_dir(hdr_row, ui_lang_base_dir(), 0);
        lv_obj_clear_flag(hdr_row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *badge = lv_obj_create(hdr_row);
        lv_obj_set_size(badge, LV_SIZE_CONTENT, 20);
        lv_obj_set_style_bg_color(badge, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_radius(badge, 4, 0);
        lv_obj_set_style_pad_hor(badge, 6, 0);
        lv_obj_set_style_pad_ver(badge, 0, 0);
        lv_obj_set_style_border_width(badge, 0, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *badge_lbl = lv_label_create(badge);
        lv_label_set_text(badge_lbl, is_fa ? "هدف عملکرد" : (is_tr ? "Hedef" : "Target"));
        lv_obj_set_style_text_font(badge_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_center(badge_lbl);

        lv_obj_t *obj_lbl = lv_label_create(obj_card);
        lv_label_set_long_mode(obj_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(obj_lbl, lv_pct(100));
        char safe_obj[384];
        sanitize_text_no_emoji(objective, safe_obj, sizeof(safe_obj));
        lv_label_set_text(obj_lbl, safe_obj);
        lv_obj_set_style_text_font(obj_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(obj_lbl, lv_color_hex(0x0369a1), 0);
        lv_obj_set_style_base_dir(obj_lbl, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_line_space(obj_lbl, 2, 0);
    }

    /* ۲. کارت‌های تفصیلی و شیک آیتم‌ها و پارامترها */
    for (int i = 0; i < item_count; i++) {
        const guide_card_item_t *item = &items[i];
        if (!item->title && !item->desc) continue;

        lv_obj_t *item_card = lv_obj_create(scroll);
        lv_obj_set_width(item_card, lv_pct(100));
        lv_obj_set_height(item_card, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(item_card, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_border_color(item_card, lv_color_hex(0xcbd5e1), 0);
        lv_obj_set_style_border_width(item_card, 1, 0);
        lv_obj_set_style_radius(item_card, 8, 0);
        lv_obj_set_style_pad_all(item_card, 6, 0);
        lv_obj_set_style_pad_row(item_card, 4, 0);
        lv_obj_set_flex_flow(item_card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_base_dir(item_card, ui_lang_base_dir(), 0);
        lv_obj_clear_flag(item_card, LV_OBJ_FLAG_SCROLLABLE);

        /* هدر ردیف: بج رنگی + عنوان */
        lv_obj_t *hdr_row = lv_obj_create(item_card);
        lv_obj_set_width(hdr_row, lv_pct(100));
        lv_obj_set_height(hdr_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(hdr_row, 0, 0);
        lv_obj_set_style_border_width(hdr_row, 0, 0);
        lv_obj_set_style_pad_all(hdr_row, 0, 0);
        lv_obj_set_style_pad_column(hdr_row, 6, 0);
        lv_obj_set_flex_flow(hdr_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(hdr_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_base_dir(hdr_row, ui_lang_base_dir(), 0);
        lv_obj_clear_flag(hdr_row, LV_OBJ_FLAG_SCROLLABLE);

        if (item->badge && item->badge[0]) {
            lv_obj_t *b = lv_obj_create(hdr_row);
            lv_obj_set_size(b, LV_SIZE_CONTENT, 20);
            lv_obj_set_style_bg_color(b, lv_color_hex(item->badge_color ? item->badge_color : 0x0284c7), 0);
            lv_obj_set_style_radius(b, 4, 0);
            lv_obj_set_style_pad_hor(b, 6, 0);
            lv_obj_set_style_pad_ver(b, 0, 0);
            lv_obj_set_style_border_width(b, 0, 0);
            lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *bl = lv_label_create(b);
            lv_label_set_text(bl, item->badge);
            lv_obj_set_style_text_font(bl, ui_lang_font_16(), 0);
            lv_obj_set_style_text_color(bl, lv_color_hex(0xffffff), 0);
            lv_obj_center(bl);
        }

        if (item->title && item->title[0]) {
            lv_obj_t *tl = lv_label_create(hdr_row);
            lv_label_set_long_mode(tl, LV_LABEL_LONG_WRAP);
            lv_obj_set_flex_grow(tl, 1);
            char safe_title[160];
            sanitize_text_no_emoji(item->title, safe_title, sizeof(safe_title));
            lv_label_set_text(tl, safe_title);
            lv_obj_set_style_text_font(tl, ui_lang_font_16(), 0);
            lv_obj_set_style_text_color(tl, lv_color_hex(0x0f172a), 0);
            lv_obj_set_style_base_dir(tl, ui_lang_base_dir(), 0);
        }

        /* توضیحات کامل و روان */
        if (item->desc && item->desc[0]) {
            lv_obj_t *dl = lv_label_create(item_card);
            lv_label_set_long_mode(dl, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(dl, lv_pct(100));
            char safe_desc[512];
            sanitize_text_no_emoji(item->desc, safe_desc, sizeof(safe_desc));
            lv_label_set_text(dl, safe_desc);
            lv_obj_set_style_text_font(dl, ui_lang_font_16(), 0);
            lv_obj_set_style_text_color(dl, lv_color_hex(0x334155), 0);
            lv_obj_set_style_base_dir(dl, ui_lang_base_dir(), 0);
            lv_obj_set_style_text_line_space(dl, 2, 0);
        }

        /* کادر راهکار کارخانه یا نکته عیب‌یابی (اختیاری) */
        if (item->tip && item->tip[0]) {
            lv_obj_t *tip_box = lv_obj_create(item_card);
            lv_obj_set_width(tip_box, lv_pct(100));
            lv_obj_set_height(tip_box, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(tip_box, lv_color_hex(0xfffbeb), 0);
            lv_obj_set_style_border_color(tip_box, lv_color_hex(0xfde68a), 0);
            lv_obj_set_style_border_width(tip_box, 1, 0);
            lv_obj_set_style_radius(tip_box, 6, 0);
            lv_obj_set_style_pad_all(tip_box, 5, 0);
            lv_obj_clear_flag(tip_box, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *tipl = lv_label_create(tip_box);
            lv_label_set_long_mode(tipl, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(tipl, lv_pct(100));
            char safe_tip[384];
            sanitize_text_no_emoji(item->tip, safe_tip, sizeof(safe_tip));
            lv_label_set_text(tipl, safe_tip);
            lv_obj_set_style_text_font(tipl, ui_lang_font_16(), 0);
            lv_obj_set_style_text_color(tipl, lv_color_hex(0x92400e), 0);
            lv_obj_set_style_base_dir(tipl, ui_lang_base_dir(), 0);
            lv_obj_set_style_text_line_space(tipl, 2, 0);
        }
    }

    /* ۳. کارت وضعیت زنده سیستم (در صورت وجود) */
    if (status_line && status_line[0]) {
        lv_obj_t *stat_card = lv_obj_create(scroll);
        lv_obj_set_width(stat_card, lv_pct(100));
        lv_obj_set_height(stat_card, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(stat_card, lv_color_hex(0xf0fdf4), 0);
        lv_obj_set_style_border_color(stat_card, lv_color_hex(0xbbf7d0), 0);
        lv_obj_set_style_border_width(stat_card, 1, 0);
        lv_obj_set_style_radius(stat_card, 8, 0);
        lv_obj_set_style_pad_all(stat_card, 6, 0);
        lv_obj_set_style_flex_flow(stat_card, LV_FLEX_FLOW_COLUMN, 0);
        lv_obj_set_style_base_dir(stat_card, ui_lang_base_dir(), 0);
        lv_obj_clear_flag(stat_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *stat_lbl = lv_label_create(stat_card);
        lv_label_set_long_mode(stat_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(stat_lbl, lv_pct(100));
        char safe_stat[192];
        sanitize_text_no_emoji(status_line, safe_stat, sizeof(safe_stat));
        lv_label_set_text(stat_lbl, safe_stat);
        lv_obj_set_style_text_font(stat_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(stat_lbl, lv_color_hex(0x166534), 0);
        lv_obj_set_style_base_dir(stat_lbl, ui_lang_base_dir(), 0);
    }
}

static void populate_structured_guide(lv_obj_t *scroll, const char *objective,
                                     const char **steps, int step_count,
                                     const char *status_line)
{
    guide_card_item_t items[12];
    int cnt = 0;
    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);
    for (int i = 0; i < step_count && i < 12; i++) {
        if (!steps[i] || !steps[i][0]) continue;
        items[cnt].badge = is_fa ? "مرحله" : (is_tr ? "Adim" : "Step");
        items[cnt].badge_color = 0x0284c7;
        items[cnt].title = NULL;
        items[cnt].desc = steps[i];
        items[cnt].tip = NULL;
        cnt++;
    }
    populate_rich_guide(scroll, objective, items, cnt, status_line);
}

void ui_show_smart_guide(guide_id_t id)
{
    s_smart_guide_active = true;
    s_last_guide_long_press_tick = lv_tick_get();
    s_current_guide_id = id;

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    if (!s_tooltip_backdrop || !lv_obj_is_valid(s_tooltip_backdrop)) {
        buzzer_beep_click();

        s_tooltip_backdrop = lv_obj_create(lv_layer_top());
        lv_obj_set_pos(s_tooltip_backdrop, 0, 0);
        lv_obj_set_size(s_tooltip_backdrop, 480, 320);
        lv_obj_set_style_bg_color(s_tooltip_backdrop, lv_color_hex(0x0f172a), 0);
        lv_obj_set_style_bg_opa(s_tooltip_backdrop, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_tooltip_backdrop, 0, 0);
        lv_obj_set_style_pad_all(s_tooltip_backdrop, 0, 0);
        lv_obj_clear_flag(s_tooltip_backdrop, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(s_tooltip_backdrop, tooltip_close_cb, LV_EVENT_CLICKED, NULL);

        /* Modal Card - 460x296 Apple High-Contrast Slate */
        s_guide_card = lv_obj_create(s_tooltip_backdrop);
        lv_obj_set_size(s_guide_card, 460, 296);
        lv_obj_center(s_guide_card);
        lv_obj_set_style_bg_color(s_guide_card, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_radius(s_guide_card, 12, 0);
        lv_obj_set_style_clip_corner(s_guide_card, false, 0); /* حذف ماسک هندسی گوشه‌ها جهت رهایی ۱۰۰٪ CPU در اسکرول */
        lv_obj_set_style_border_width(s_guide_card, 1, 0);
        lv_obj_set_style_border_color(s_guide_card, lv_color_hex(0x94a3b8), 0);
        lv_obj_set_style_shadow_width(s_guide_card, 0, 0);
        lv_obj_set_style_pad_all(s_guide_card, 0, 0);
        lv_obj_clear_flag(s_guide_card, LV_OBJ_FLAG_SCROLLABLE);

        /* Header Bar */
        lv_obj_t *hdr = lv_obj_create(s_guide_card);
        lv_obj_set_pos(hdr, 0, 0);
        lv_obj_set_width(hdr, lv_pct(100));
        lv_obj_set_height(hdr, 34);
        lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_radius(hdr, 0, 0);
        lv_obj_set_style_border_width(hdr, 0, 0);
        lv_obj_set_style_pad_all(hdr, 0, 0);
        lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

        s_guide_title_lbl = lv_label_create(hdr);
        lv_label_set_text(s_guide_title_lbl, is_fa ? "راهنمای هوشمند صنعتی آستیاج" : "ASTIAJ Industrial Smart Guide");
        lv_obj_set_style_text_font(s_guide_title_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(s_guide_title_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_base_dir(s_guide_title_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(s_guide_title_lbl);

        lv_obj_t *btn_x = lv_btn_create(hdr);
        lv_obj_align(btn_x, LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_set_size(btn_x, 30, 30);
        lv_obj_set_ext_click_area(btn_x, 8);
        lv_obj_set_style_radius(btn_x, 6, 0);
        lv_obj_set_style_bg_opa(btn_x, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(btn_x, 80, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(btn_x, 0, 0);
        lv_obj_add_event_cb(btn_x, tooltip_close_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *xlbl = lv_label_create(btn_x);
        lv_label_set_text(xlbl, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_font(xlbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(xlbl, lv_color_hex(0xffffff), 0);
        lv_obj_center(xlbl);

        /* نوار سلکتور چندحالته (سوئیچ بین مودهای اصلی یا تب‌های تنظیمات) */
        s_guide_mode_bar = lv_obj_create(s_guide_card);
        lv_obj_set_pos(s_guide_mode_bar, 6, 36);
        lv_obj_set_size(s_guide_mode_bar, 448, 26);
        lv_obj_set_style_bg_opa(s_guide_mode_bar, 0, 0);
        lv_obj_set_style_border_width(s_guide_mode_bar, 0, 0);
        lv_obj_set_style_pad_all(s_guide_mode_bar, 0, 0);
        lv_obj_set_style_pad_column(s_guide_mode_bar, 4, 0);
        lv_obj_set_flex_flow(s_guide_mode_bar, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_base_dir(s_guide_mode_bar, ui_lang_base_dir(), 0);
        lv_obj_clear_flag(s_guide_mode_bar, LV_OBJ_FLAG_SCROLLABLE);

        for (int i = 0; i < 6; i++) {
            lv_obj_t *mbtn = lv_btn_create(s_guide_mode_bar);
            s_guide_mode_btns[i] = mbtn;
            lv_obj_set_size(mbtn, 71, 24);
            lv_obj_set_style_radius(mbtn, 12, 0);
            lv_obj_set_style_pad_all(mbtn, 0, 0);
            lv_obj_set_style_shadow_width(mbtn, 0, 0);
            lv_obj_set_style_border_width(mbtn, 1, 0);
            lv_obj_set_ext_click_area(mbtn, 6);
            lv_obj_add_event_cb(mbtn, guide_mode_pill_click_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *mlbl = lv_label_create(mbtn);
            s_guide_mode_lbls[i] = mlbl;
            lv_obj_set_style_text_font(mlbl, ui_lang_font_16(), 0);
            lv_obj_center(mlbl);
        }

        /* کانتینر اسکرول محتوای راهنما */
        s_guide_scroll = lv_obj_create(s_guide_card);
        lv_obj_set_pos(s_guide_scroll, 6, 65);
        lv_obj_set_size(s_guide_scroll, 448, 188);
        lv_obj_set_style_bg_color(s_guide_scroll, lv_color_hex(0xf8fafc), 0);
        lv_obj_set_style_bg_opa(s_guide_scroll, 255, 0);
        lv_obj_set_style_radius(s_guide_scroll, 8, 0);
        lv_obj_set_style_border_width(s_guide_scroll, 1, 0);
        lv_obj_set_style_border_color(s_guide_scroll, lv_color_hex(0xe2e8f0), 0);
        lv_obj_set_style_pad_left(s_guide_scroll, 6, 0);
        lv_obj_set_style_pad_right(s_guide_scroll, 14, 0);
        lv_obj_set_style_pad_top(s_guide_scroll, 6, 0);
        lv_obj_set_style_pad_row(s_guide_scroll, 6, 0);
        lv_obj_set_style_pad_bottom(s_guide_scroll, 14, 0);
        lv_obj_set_flex_flow(s_guide_scroll, LV_FLEX_FLOW_COLUMN);
        lv_obj_add_flag(s_guide_scroll, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(s_guide_scroll, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN);
        lv_obj_set_scroll_dir(s_guide_scroll, LV_DIR_VER);
        lv_obj_set_style_anim_duration(s_guide_scroll, 0, 0);

        /* دکمه تایید و بستن پایین */
        lv_obj_t *btn_ok = lv_btn_create(s_guide_card);
        lv_obj_set_pos(btn_ok, 160, 258);
        lv_obj_set_size(btn_ok, 140, 32);
        lv_obj_set_style_radius(btn_ok, 8, 0);
        lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x16a34a), 0);
        lv_obj_set_style_shadow_width(btn_ok, 0, 0);
        lv_obj_add_event_cb(btn_ok, tooltip_close_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *ok_lbl = lv_label_create(btn_ok);
        lv_label_set_text(ok_lbl, is_fa ? "متوجه شدم" : (is_tr ? "Anladim" : "Got It"));
        lv_obj_set_style_text_font(ok_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
        lv_obj_center(ok_lbl);
    }

    /* نوار سلکتور بالا: سوئیچ هوشمند بین مودها یا تب‌ها */
    if (s_guide_mode_bar && lv_obj_is_valid(s_guide_mode_bar)) {
        if (id >= 1 && id <= 6) {
            lv_obj_clear_flag(s_guide_mode_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_guide_scroll, 6, 65);
            lv_obj_set_size(s_guide_scroll, 448, 188);
            static const char *s_pills_fa[6] = {"تولید", "نگهداری", "پارو", "لبه", "شیر", "شستشو"};
            static const char *s_pills_en[6] = {"Prod", "Hold", "Paddle", "Edge", "Mixer", "Wash"};
            static const char *s_pills_tr[6] = {"Uretim", "Bekleme", "Palet", "Kenar", "Karistirici", "Yikama"};
            for (int i = 0; i < 6; i++) {
                if (!s_guide_mode_btns[i] || !lv_obj_is_valid(s_guide_mode_btns[i])) continue;
                lv_obj_set_user_data(s_guide_mode_btns[i], (void *)(intptr_t)(i + 1));
                const char *ptext = is_fa ? s_pills_fa[i] : (is_tr ? s_pills_tr[i] : s_pills_en[i]);
                if (s_guide_mode_lbls[i] && lv_obj_is_valid(s_guide_mode_lbls[i])) {
                    lv_label_set_text(s_guide_mode_lbls[i], ptext);
                }
                if ((i + 1) == (int)id) {
                    lv_obj_set_style_bg_color(s_guide_mode_btns[i], lv_color_hex(0x0284c7), 0);
                    lv_obj_set_style_border_color(s_guide_mode_btns[i], lv_color_hex(0x38bdf8), 0);
                    if (s_guide_mode_lbls[i]) lv_obj_set_style_text_color(s_guide_mode_lbls[i], lv_color_hex(0xffffff), 0);
                } else {
                    lv_obj_set_style_bg_color(s_guide_mode_btns[i], lv_color_hex(0xf1f5f9), 0);
                    lv_obj_set_style_border_color(s_guide_mode_btns[i], lv_color_hex(0xcbd5e1), 0);
                    if (s_guide_mode_lbls[i]) lv_obj_set_style_text_color(s_guide_mode_lbls[i], lv_color_hex(0x475569), 0);
                }
            }
        } else if (id >= GUIDE_ID_TAB_F1 && id <= GUIDE_ID_TAB_F6) {
            lv_obj_clear_flag(s_guide_mode_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_guide_scroll, 6, 65);
            lv_obj_set_size(s_guide_scroll, 448, 188);
            static const char *s_tab_pills_fa[6] = {"تایمرها", "ساعت", "برودت", "وای‌فای", "تست دیاگ", "ایمنی"};
            static const char *s_tab_pills_en[6] = {"F1 Timers", "F2 Clock", "F3 Cold", "F4 WiFi", "F5 Test", "F6 Safety"};
            static const char *s_tab_pills_tr[6] = {"F1 Zaman", "F2 Saat", "F3 Sogut", "F4 WiFi", "F5 Test", "F6 Guven"};
            for (int i = 0; i < 6; i++) {
                if (!s_guide_mode_btns[i] || !lv_obj_is_valid(s_guide_mode_btns[i])) continue;
                guide_id_t target_gid = (guide_id_t)(GUIDE_ID_TAB_F1 + i);
                lv_obj_set_user_data(s_guide_mode_btns[i], (void *)(intptr_t)target_gid);
                const char *ptext = is_fa ? s_tab_pills_fa[i] : (is_tr ? s_tab_pills_tr[i] : s_tab_pills_en[i]);
                if (s_guide_mode_lbls[i] && lv_obj_is_valid(s_guide_mode_lbls[i])) {
                    lv_label_set_text(s_guide_mode_lbls[i], ptext);
                }
                if (target_gid == id) {
                    lv_obj_set_style_bg_color(s_guide_mode_btns[i], lv_color_hex(0x0284c7), 0);
                    lv_obj_set_style_border_color(s_guide_mode_btns[i], lv_color_hex(0x38bdf8), 0);
                    if (s_guide_mode_lbls[i]) lv_obj_set_style_text_color(s_guide_mode_lbls[i], lv_color_hex(0xffffff), 0);
                } else {
                    lv_obj_set_style_bg_color(s_guide_mode_btns[i], lv_color_hex(0xf1f5f9), 0);
                    lv_obj_set_style_border_color(s_guide_mode_btns[i], lv_color_hex(0xcbd5e1), 0);
                    if (s_guide_mode_lbls[i]) lv_obj_set_style_text_color(s_guide_mode_lbls[i], lv_color_hex(0x475569), 0);
                }
            }
        } else {
            lv_obj_add_flag(s_guide_mode_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_guide_scroll, 6, 38);
            lv_obj_set_size(s_guide_scroll, 448, 215);
        }
    }

    static char title[128];
    static char objective[384];
    static char status_line[160];
    static guide_card_item_t cards[10];
    int card_cnt = 0;
    title[0] = '\0';
    objective[0] = '\0';
    status_line[0] = '\0';

#define G_TXT(fa_s, en_s, tr_s) (is_fa ? (fa_s) : (is_tr ? (tr_s) : (en_s)))

    switch (id) {
    case GUIDE_ID_MAKE:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای حالت تولید بستنی", "Ice Cream Production Guide", "Dondurma Uretim Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("فرآیند کامل آماده‌سازی، انجماد، توقف خودکار کمپرسور و توزیع بستنی قیفی.", "Complete cycle of mix freezing, automatic consistency stop, and dispensing.", "Karisim, dondurma, otomatik sertlik durdurma ve servis sureci."));
        cards[0] = (guide_card_item_t){G_TXT("مرحله ۱", "Step 1", "Adim 1"), 0x0284c7, G_TXT("استارت پارو و تاخیر کمپرسور", "Paddle Start & Delay", "Palet Baslatma ve Gecikme"),
            G_TXT("با فشردن کلید تولید، ابتدا موتور پارو روشن می‌شود تا مایه بستنی در سیلندر جریان یابد و بار اولیه نرم شود. پس از ۵ ثانیه تاخیر، کمپرسور بزرگ روشن می‌شود تا از پریدن فیوز برق مغازه بر اثر استارت همزمان جلوگیری گردد.", "Paddle starts first to agitate mix. Compressor starts after 5s delay to prevent power surge.", "Uretim basladiginda once palet calisir. 5 saniye sonra buyuk kompresor devreye girer boylece sebeke asiri yuklenmez."),
            G_TXT("نکته: تاخیر استارت در بخش تنظیمات تایمرها قابل برنامه‌ریزی است (پیش‌فرض: ۵ ثانیه).", "Note: Startup delay is configurable in Tab F1 (default: 5s).", "Not: Baslatma gecikmesi F1 sekmesinde ayarlanabilir (varsayilan: 5 sn).")};
        cards[1] = (guide_card_item_t){G_TXT("مرحله ۲", "Step 2", "Adim 2"), 0x0284c7, G_TXT("انجماد و تراشیدن سیلندر", "Freezing & Scraping", "Dondurma ve Kazima"),
            G_TXT("کمپرسور بزرگ برودت سیلندر را تامین می‌کند و تیغه‌های پارو مایه بستنی را از دیواره سیلندر تراشیده و به مایه هوادهی می‌کنند تا پف و قوام ایده‌آل ایجاد شود.", "Big compressor cools the cylinder while paddle scrapes walls and incorporates air.", "Buyuk kompresor sogutma yaparken palet bicaklari silindir duvarini kazir ve hacim kazandirir."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("مرحله ۳", "Step 3", "Adim 3"), 0x16a34a, G_TXT("توقف خودکار با سنسور سفتی کلاچ", "Auto Stop (Clutch Sensor)", "Otomatik Sertlik Durdurma"),
            G_TXT("با سفت شدن بستنی به حد مجاز، فشار مکانیکی شفت را به عقب می‌راند و میکروسوئیچ کلاچ عقب تحریک می‌شود. با اولین تحریک، کمپرسور بزرگ فورا خاموش می‌شود تا بستنی سفت‌تر از حد نشود.", "When ice cream hardens, mechanical pressure trips the rear clutch microswitch and shuts off compressor.", "Dondurma istenen sertlige ulasinca arka kavrama switchi tetiklenir ve kompresor aninda durur."),
            G_TXT("نکته: سقف ایمنی انجماد ۱۵ دقیقه است؛ اگر کلاچ عمل نکند، سیستم خودکار کمپرسور را قطع می‌کند.", "Note: Safety freeze limit is 15 minutes.", "Not: Guvenli dondurma siniri 15 dakikadir.")};
        cards[3] = (guide_card_item_t){G_TXT("مرحله ۴", "Step 4", "Adim 4"), 0x16a34a, G_TXT("توزیع بستنی با کشیدن اهرم", "Dispensing via Lever", "Kol ile Dondurma Servisi"),
            G_TXT("با پایین کشیدن اهرم، پارو شروع به چرخش می‌کند تا بستنی تخلیه شود و ۱ عدد به شمارنده قیف اضافه می‌گردد. با سبک شدن بار، کمپرسور مجدداً جهت انجماد استارت می‌زند.", "Pulling the lever spins the paddle to dispense ice cream and increments the cone counter.", "Kolu asagi cektiginizde palet donerek dondurmayi verir ve kulah sayaci bir artar."),
            NULL};
        cards[4] = (guide_card_item_t){G_TXT("عیب‌یابی", "Troubleshoot", "Ariza Cozum"), 0xea580c, G_TXT("تنظیم سفتی یا شلی بستنی", "Adjusting Ice Cream Texture", "Kivam Ayari"),
            G_TXT("اگر بستنی خیلی شل تخلیه می‌شود، پیچ رگلاژ فنر کلاچ عقب پشت دستگاه را ۱ دور سفت‌تر کنید. اگر بستنی خیلی سفت است و موتور به سختی کار می‌کند، پیچ فنر را نیم دور شل کنید.", "Tighten rear clutch spring 1 turn if too soft. Loosen 1/2 turn if too stiff.", "Dondurma cok yumusaksa arka kavrama yayini 1 tur sikin. Cok sertse yarim tur gevsetin."),
            G_TXT("نکته: درصد چربی و شکر مایه بستنی تاثیر مستقیم در زمان سفت شدن دارد.", "Note: Mix fat and sugar ratio directly affects hardening time.", "Not: Sut karisimindaki yag ve seker orani kivam alma suresini dogrudan etkiler.")};
        card_cnt = 5;
        if (max6675_is_cylinder_valid() && max6675_is_top_valid()) {
            snprintf(status_line, sizeof(status_line), "%s", G_TXT("وضعیت زنده: دمای سیلندر %.1f° | مخزن شیر: %.1f°", "Live: Cylinder %.1f°C | Hopper %.1f°C", "Canli: Silindir %.1f°C | Hazne %.1f°C"));
            /* format with values */
            char temp_buf[160];
            snprintf(temp_buf, sizeof(temp_buf), status_line, max6675_get_temp_cylinder(), max6675_get_temp_top());
            strncpy(status_line, temp_buf, sizeof(status_line) - 1);
        } else {
            snprintf(status_line, sizeof(status_line), "%s", G_TXT("سیستم آماده استارت فرآیند تولید بستنی", "System ready for ice cream production", "Sistem dondurma uretimi icin hazir"));
        }
        break;

    case GUIDE_ID_HOLD:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای حالت نگهداری شبانه", "Night Holding Mode Guide", "Gece Bekleme Modu Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("حفاظت برودتی و نگهداری شبانه مایه بستنی در مخزن بن‌ماری بدون نیاز به شستشو و تخلیه.", "Overnight refrigerated preservation of milk mix without dumping.", "Sut karisimini bosaltmadan gece boyunca hijyenik sogutma ile koruma."));
        cards[0] = (guide_card_item_t){G_TXT("عملکرد", "Function", "Fonksiyon"), 0x0284c7, G_TXT("پایش پیوسته دمای مخزن شیر", "Continuous Hopper Temp Monitoring", "Surekli Hazne Sicaklik Takibi"),
            G_TXT("در پایان شیفت نیازی به تخلیه شیر نیست. دستگاه با روشن کردن نگهدارنده بن‌ماری، دمای شیر را بین ۳+ تا ۶+ درجه نگه می‌دارد تا از رشد باکتری و ترشیدن شیر جلوگیری شود.", "Holding cooling cycles to keep hopper temperature between +3°C and +6°C.", "Gece sutu bosaltmaya gerek yoktur. Bekleme sogutmasi calisarak hazne sicakligini +3 ile +6 derece arasinda tutar."),
            G_TXT("نکته: مصرف برق دستگاه در حالت نگهداری شبانه بسیار کم و اقتصادی است.", "Note: Energy saving mode for overnight preservation.", "Not: Gece bekleme modu yuksek enerji tasarrufu saglar.")};
        cards[1] = (guide_card_item_t){G_TXT("ترموستات", "Thermostat", "Termostat"), 0x16a34a, G_TXT("دمای قطع و وصل سرمایش", "Cut-in / Cut-out Setpoints", "Sogutma Acma Kapama Degerleri"),
            G_TXT("نگهدارنده تا رسیدن به ۳+ درجه خنک می‌کند و سپس خاموش می‌شود. با بالا رفتن دما به ۶+ درجه مجدداً استارت می‌زند (هیسترزیس ۳ درجه جهت حفاظت نگهدارنده).", "Holding cools down to +3°C and restarts at +6°C to protect equipment life.", "Bekleme +3 derecede durur ve +6 derecede tekrar baslar."),
            G_TXT("نکته: این دماها در بخش تنظیمات برودت قابل برنامه‌ریزی هستند.", "Note: Configurable in Tab F3.", "Not: Bu sicakliklar F3 sekmesinde ayarlanabilir.")};
        cards[2] = (guide_card_item_t){G_TXT("استندبای", "Standby", "Bekleme"), 0x7c3aed, G_TXT("تازه نگه‌داشتن بستنی سیلندر", "Cylinder Refresh", "Silindir Tazeleme"),
            G_TXT("کمپرسور بزرگ در طول شب به صورت دوره‌ای و کوتاه روشن می‌شود تا مایه داخل سیلندر آب نشود و برای صبح فردا کاملاً تازه و آماده کار بماند.", "Big compressor pulses periodically to prevent cylinder mix from melting.", "Buyuk kompresor gece boyunca periyodik calisarak silindirdeki dondurmanin erimesini onler."),
            NULL};
        cards[3] = (guide_card_item_t){G_TXT("بهداشت", "Hygiene", "Hijyen"), 0xea580c, G_TXT("شرایط بهداشتی نگهداری", "Hygiene Conditions", "Hijyen Sartlari"),
            G_TXT("درب استیل مخزن بالای دستگاه را همیشه بسته نگه دارید. در صورت قطعی طولانی برق شهری، دمای مخزن شیر را کنترل نمایید.", "Keep hopper lid closed. Check temperature probe T1 if power was lost.", "Hazne kapagini daima kapali tutun. Elektrik kesintisi olursa T1 sicakligini kontrol edin."),
            NULL};
        card_cnt = 4;
        snprintf(status_line, sizeof(status_line), "%s: %s",
                 G_TXT("وضعیت پراب مخزن شیر", "Hopper Sensor", "Hazne Sensoru"),
                 max6675_is_top_valid() ? G_TXT("متصل و کالیبره", "Connected", "Bagli ve Kalibre") : G_TXT("قطع یا عدم اتصال", "Disconnected", "Baglanti Yok"));
        break;

    case GUIDE_ID_PADDLE:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای کارکرد پارو", "Beater Paddle Guide", "Palet Karistirici Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("چرخش مستقل موتور پارو برای یکنواختی مایه بستنی یا تخلیه بدون برودت.", "Independent beater rotation for mixing or emptying cylinder without refrigeration.", "Sogutma olmadan karistirma veya silindiri bosaltma icin bagimsiz palet calismasi."));
        cards[0] = (guide_card_item_t){G_TXT("کاربرد", "Usage", "Kullanim"), 0x0284c7, G_TXT("مخلوط کردن مایه و تخلیه سیلندر", "Mixing & Emptying", "Karisim ve Bosaltma"),
            G_TXT("این کلید موتور پارو را بدون روشن شدن کمپرسورها به گردش درمی‌آورد. برای هم زدن مایه تازه ریخته شده یا تخلیه کامل بستنی در پایان کار استفاده می‌شود.", "Rotates beater without compressor. Used for stirring fresh mix or cylinder evacuation.", "Kompresor calismadan sadece paleti dondurur. Yeni konulan karisimi homojenlestirmek veya bosaltmak icin kullanilir."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("حفاظت تسمه", "Belt Guard", "Kayis Korumasi"), 0xea580c, G_TXT("پایش سنسور پولی دور موتور", "Pulley Sensor Stall Guard", "Kasnak Sensoru Takibi"),
            G_TXT("هنگام چرخش پارو، پالس‌های سنسور القایی پولی به طور مداوم چک می‌شود. در صورت عدم دریافت پالس ظرف ۴ ثانیه (پاره شدن تسمه یا قفل گیربکس)، فورا خطای توقف پولی (کد ۰۱) صادر شده و موتور خاموش می‌شود.", "Inductive sensor monitors pulley speed. Lack of pulses within 4s triggers E01 stall fault.", "Palet donerken kasnak sensorunden sinyal beklenir. 4 saniye icinde sinyal gelmezse E01 arizasi verilir ve motor durdurulur."),
            G_TXT("نکته: در صورت بروز خطای ایست پولی، سلامت تسمه و فاصله ۲ میلی‌متری سنسور را بررسی کنید.", "Note: Check belt condition and 2mm sensor gap if E01 occurs.", "Not: E01 arizasinda kayisi ve 2 mm sensor mesafesini kontrol edin.")};
        cards[2] = (guide_card_item_t){G_TXT("تایمر", "Timers", "Zamanlayici"), 0x16a34a, G_TXT("کارکرد دوره‌ای جهت ممانعت از گرم شدن", "Intermittent Duty Cycle", "Periyodik Calisma"),
            G_TXT("چرخش مداوم تیغه‌ها باعث اصطکاک و گرم شدن مایه می‌شود؛ بنابراین چرخش پارو به صورت متناوب طبق تنظیمات زمان‌بندی تایمرها انجام می‌گیرد.", "Intermittent cycle prevents friction heat buildup according to F1 timers.", "Surekli donus surtunme ile isinmaya yol acar; bu yuzden palet F1 zamanlayicilarina gore aralikli calisir."),
            NULL};
        card_cnt = 3;
        snprintf(status_line, sizeof(status_line), "%s: %s",
                 G_TXT("وضعیت سنسور پولی", "Pulley Sensor", "Kasnak Sensoru"),
                 machine_advisor_is_pulley_bypassed() ? G_TXT("بای‌پس موقت فعال", "Bypassed", "Gecici Devre Disi") : G_TXT("در حال پایش فعال دور موتور", "Actively Monitoring", "Aktif Izleniyor"));
        break;

    case GUIDE_ID_EDGE:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تزریق لبه رنگی", "Edge Rainbow Flavor Guide", "Kenar Sos Pompasi Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("تزیین و تزریق خودکار شربت و ژله طعم‌دار روی لبه بستنی قیفی.", "Automated syrup and jelly decoration along the cone rim.", "Kulah kenarina otomatik sos ve jole enjeksiyonu."));
        cards[0] = (guide_card_item_t){G_TXT("عملکرد", "Function", "Fonksiyon"), 0x0284c7, G_TXT("تزریق همگام با کشیدن دسته", "Synchronized Injection", "Senkronize Enjeksiyon"),
            G_TXT("با فعال بودن این کلید، هر زمان اهرم بستنی‌زنی را به پایین بکشید، پمپ کرم و شیر برقی باد فعال شده و شربت روی لبه قیف تزریق می‌شود.", "Pulling the lever triggers flavor pump and air solenoid simultaneously.", "Bu mod aktifken servis kolu indirildiginde sos pompasi ve hava valfi ayni anda devreye girer."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("قطع آنی", "Auto Cutoff", "Aninda Kesme"), 0x16a34a, G_TXT("قطع بلادرنگ با رها کردن دسته", "Instant Release Cutoff", "Kol Birakilinca Durma"),
            G_TXT("به محض رها کردن دسته، تزریق شربت و باد فوراً قطع می‌شود تا قطره‌ای روی بدنه دستگاه یا زمین نریزد.", "Injection halts immediately when lever returns to idle.", "Kol birakildigi anda sos ve hava kesilerek damlama yapilmasi onlenir."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("عیب‌یابی", "Troubleshoot", "Ariza Cozum"), 0xea580c, G_TXT("اگر شربت پاشیده نمی‌شود چه کنیم؟", "No Flavor Spray?", "Sos Gelmiyorsa"),
            G_TXT("مخزن ژله را چک کنید تا شربت تمام نشده باشد. فشار هوای فشرده (شیر باد) را بررسی کنید. از شربت‌های بسیار غلیظ که باعث گرفتگی نازل می‌شوند پرهیز نمایید.", "Check syrup level, air pressure, and clean nozzles with warm water daily.", "Sos seviyesini ve hava basincini kontrol edin. Asiri koyu soslar nozulu tikayabilir."),
            G_TXT("نکته: نازل تزریق را در پایان هر روز کاری با آب گرم بشویید.", "Note: Flush nozzles with warm water daily.", "Not: Sos nozulunu her gun sicak suyla temizleyin.")};
        card_cnt = 3;
        snprintf(status_line, sizeof(status_line), "%s: %s",
                 G_TXT("وضعیت سیستم تزریق", "Flavor Pump", "Sos Pompasi"),
                 edge_is_active() ? G_TXT("فعال و در حال پمپاژ", "Active Pumping", "Aktif Pompalaniyor") : G_TXT("آماده کار", "Ready", "Hazir"));
        break;

    case GUIDE_ID_RAINBOW:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای پارو مخزن شیر", "Hopper Milk Paddle Guide", "Hazne Sut Paleti Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("موتور پارو مخزن بن‌ماری جهت یکنواختی دمای شیر و جلوگیری از ته‌نشینی چربی.", "Slow hopper paddle preventing fat separation and ensuring uniform temperature.", "Yag ayrismasini onleyen ve homojen sicaklik saglayan hazne paleti."));
        cards[0] = (guide_card_item_t){G_TXT("عملکرد", "Function", "Fonksiyon"), 0x0284c7, G_TXT("یکنواختی دما و کیفیت مایه شیر", "Uniform Mix Temperature", "Homojen Karisim Sicakligi"),
            G_TXT("این پروانه پارو در کف مخزن، شیر را آرام به گردش درمی‌آورد تا دمای کل مخزن یکنواخت بماند و چربی و شکر مایه دوفاز یا ته‌نشین نشوند.", "Gentle agitation maintains uniform temperature and prevents cream separation.", "Hazne dibindeki pervane sutu yavasca dondurerek yag ve sekerin cokmesini engeller."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("تایمر", "Timers", "Zamanlayici"), 0x16a34a, G_TXT("کارکرد دوره‌ای و استراحت خودکار", "Periodic Duty Cycle", "Otomatik Dinlenme Dongusu"),
            G_TXT("پارو شیر به صورت خودکار کار می‌کند (پیش‌فرض: ۲۰ ثانیه روشن، ۱۵ دقیقه استراحت). این زمان‌ها در بخش تنظیمات تایمرها قابل برنامه‌ریزی هستند.", "Automatic cycling: 20s run, 15min rest (configurable in Tab F1).", "Otomatik calisma: 20 sn calisma, 15 dk bekleme (F1 sekmesinde ayarlanabilir)."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("نظافت", "Cleaning", "Temizlik"), 0xea580c, G_TXT("نظافت پروانه پارو مخزن", "Paddle Impeller Hygiene", "Palet Pervane Hijyeni"),
            G_TXT("هنگام شستشوی مخزن، پروانه پارو را با آب ولرم بشویید و مطمئن شوید پروانه به جایی گیر نمی‌کند.", "Rinse impeller with warm water during hopper cleaning.", "Hazne temizliginde pervaneyi ilik suyla yikayin ve serbest dondugunden emin olun."),
            NULL};
        card_cnt = 3;
        snprintf(status_line, sizeof(status_line), "%s: %s",
                 G_TXT("موتور پارو مخزن شیر", "Paddle Motor", "Palet Motoru"),
                 rainbow_is_active() ? G_TXT("روشن و فعال", "Running", "Calisiyor") : G_TXT("خاموش و استراحت", "Resting", "Dinlenmede"));
        break;

    case GUIDE_ID_WASH:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای شستشو و پمپ شستشو", "Wash Pump Guide", "Yikama Pompasi Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("شستشو و ضدعفونی مسیر با پمپ شستشو به صورت کاملاً مستقل از موتور پارو.", "Washing circuit using wash pump completely independent of beater motor.", "Palet motorundan bagimsiz olarak yikama pompasi ile yikama devresi."));
        cards[0] = (guide_card_item_t){G_TXT("مرحله ۱", "Step 1", "Adim 1"), 0x0284c7, G_TXT("تخلیه مایه باقی‌مانده", "Drain Remaining Mix", "Kalan Karisimi Bosaltma"),
            G_TXT("با زدن کلید پارو و پایین نگه داشتن اهرم، تمام مایه بستنی باقی‌مانده در سیلندر را به طور کامل در ظرف تمیز تخلیه کنید.", "Drain all leftover mix into a clean container by running beater and opening lever.", "Palet butonuna basip kolu acarak silindirdeki kalan karisimi temiz bir kaba bosaltin."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("مرحله ۲", "Step 2", "Adim 2"), 0x0284c7, G_TXT("ریختن آب ولرم و شوینده مجاز", "Warm Water & Sanitizer", "Ilik Su ve Dezenfektan"),
            G_TXT("مخزن بالا را با آب ولرم (حداکثر ۴۰ درجه) و محلول ضدعفونی مخصوص بستنی‌ساز پر کنید تا وارد سیلندر شود.", "Fill hopper with lukewarm water (max 40°C) and food-grade sanitizer.", "Ust hazneyi ilik su (en fazla 40 derece) ve gida tipi dezenfektan ile doldurun."),
            G_TXT("هشدار: هرگز آب داغ بالای ۵۰ درجه داخل دستگاه نریزید تا اورینگ‌ها و قطعات پلاستیکی آسیب نبینند.", "Warning: Never use water above 50°C to protect seals and gaskets.", "Uyari: Contalara zarar vermemek icin kesinlikle 50 dereceden sicak su koymayin.")};
        cards[2] = (guide_card_item_t){G_TXT("مرحله ۳", "Step 3", "Adim 3"), 0x16a34a, G_TXT("فعال‌سازی پمپ شستشو", "Run Wash Pump", "Yikama Pompasini Calistirma"),
            G_TXT("کلید شستشو را بزنید. پمپ شستشو فعال می‌شود تا مسیر را شستشو دهد. در این حالت موتور پارو و کمپرسورها قطع مطلق هستند.", "Press Wash button. Wash pump energizes. Beater motor and compressors remain safely OFF.", "Yikama butonuna basin. Yikama pompasi calisir. Palet ve kompresorler kapali kalir."),
            NULL};
        cards[3] = (guide_card_item_t){G_TXT("مرحله ۴", "Step 4", "Adim 4"), 0x16a34a, G_TXT("تخلیه کامل آب و خشک شدن", "Full Drain & Dry", "Tam Bosaltma ve Kurulama"),
            G_TXT("پس از ۳ تا ۵ دقیقه چرخش، اهرم تخلیه را پایین بکشید تا آب کف‌آلود کاملاً خارج شود. سپس با آب ولرم تمیز آبکشی کنید تا هیچ آبی در سیلندر باقی نماند.", "Open lever to drain sanitizer. Rinse thoroughly with clean water until completely dry.", "3-5 dakika sonra kolu acip suyu bosaltin ve temiz su ile durulayin."),
            G_TXT("نکته: قبل از ریختن مایه بستنی جدید، مطمئن شوید قطره آبی در سیلندر نمانده باشد.", "Note: Ensure cylinder is completely drained before adding new mix.", "Not: Yeni karisim eklemeden once silindirde su kalmadigindan emin olun.")};
        card_cnt = 4;
        snprintf(status_line, sizeof(status_line), "%s: %s",
                 G_TXT("مدار شستشو", "Wash Circuit", "Yikama Devresi"),
                 wash_is_active() ? G_TXT("پمپ شستشو", "Wash Pump", "Yikama Pompasi") : G_TXT("متوقف و آماده", "Idle", "Hazir"));
        break;

    case GUIDE_ID_SETTINGS:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای منوی تنظیمات مهندسی", "Engineering Settings Guide", "Muhendislik Ayarlari Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("راهنمای جامع تب‌های ۶گانه تنظیمات و پارامترهای صنعتی دستگاه.", "Complete overview of the 6 engineering parameter tabs.", "Cihazin 6 muhendislik ayar sekmesinin ozeti."));
        cards[0] = (guide_card_item_t){G_TXT("تایمرها", "F1", "F1"), 0x0284c7, G_TXT("تایمرهای فرآیند و پارو", "Process & Beater Timers", "Surec ve Palet Zamanlayicilari"),
            G_TXT("تنظیم تاخیر استارت کمپرسور، زمان چرخش و استراحت پارو، سنسور پولی، تاخیر ایمنی کمپرسورها و پارو بن‌ماری شیر.", "Compressor startup delay, paddle duty timers, pulley timeout, and hopper paddle.", "Kompresor baslama gecikmesi, palet calisma sureleri ve hazne paleti."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("ساعت", "F2", "F2"), 0x0284c7, G_TXT("ساعت، تاریخ و کالیبراسیون", "Clock & Calibration", "Saat, Tarih ve Kalibrasyon"),
            G_TXT("تنظیم ساعت رسمی، باتری پشتیبان حافظه، همگام‌سازی اینترنتی و کالیبراسیون دماسنج‌های مخزن و سیلندر.", "Hardware RTC, backup battery, internet NTP sync, and thermocouple offsets.", "Donanim saati, pil durumu, internet senkronizasyonu ve sensor kalibrasyonu."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("برودت", "F3", "F3"), 0x16a34a, G_TXT("کنترل برودت و نگهداری", "Cooling & Night Holding", "Sogutma ve Gece Bekleme"),
            G_TXT("تنظیم دمای قطع و وصل ترموستات مخزن، استراحت شبانه و کارکرد اضطراری نجات مایه در صورت خرابی سنسور.", "Hopper thermostat thresholds, standby refresh, and probe failure fallback duty.", "Hazne termostat esikleri, gece koruma ve acil durum calismasi."),
            NULL};
        cards[3] = (guide_card_item_t){G_TXT("وای‌فای", "F4", "F4"), 0x16a34a, G_TXT("ارتباط وای‌فای و وب‌پورتال", "WiFi & Web Portal", "WiFi ve Web Portal"),
            G_TXT("اتصال به گوشی بدون اینترنت، اتصال به مودم مغازه، مانیتورینگ پارامترها و آپدیت آنلاین ابری فریمور.", "Direct phone hotspot, shop WiFi router connection, live web dashboard, and OTA updates.", "Telefona direkt baglanti, dukkan modemi, canli gosterge ve bulut guncelleme."),
            NULL};
        cards[4] = (guide_card_item_t){G_TXT("تست دیاگ", "F5", "F5"), 0xea580c, G_TXT("تست سخت‌افزار و دیاگ", "Hardware Diagnostics", "Donanim Testi ve Ariza Tespit"),
            G_TXT("تست و عیب‌یابی ورودی‌های دیجیتال (اهرم، کلاچ، استپ، سنسور پولی) و تست دستی تک‌تک رله‌های خروجی.", "Live monitoring of all digital inputs and manual relay actuation tests.", "Dijital girislerin izlenmesi ve cikis rolelerinin elle test edilmesi."),
            NULL};
        cards[5] = (guide_card_item_t){G_TXT("ایمنی", "F6", "F6"), 0x7c3aed, G_TXT("ایمنی فرآیند و روزشمار", "Safety & Day-Lock", "Guvenlik ve Kiralama"),
            G_TXT("سقف زمان انجماد، آلارم باز ماندن اهرم، وضعیت قفل اقساط روزشمار و بازنشانی به تنظیمات کارخانه.", "Maximum freeze limit, lever stuck timeout, leasing lock status, and factory reset.", "Dondurma sure siniri, kol uyarisi, taksit durumu ve fabrika sifirlamasi."),
            NULL};
        card_cnt = 6;
        break;

    case GUIDE_ID_TAB_F1:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تنظیمات تایمرهای فرآیند و پارو", "Tab F1: Process & Beater Timers", "F1 Sekmesi: Surec ve Palet Zamanlayicilari"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("تنظیم دقیق زمان‌بندی موتور پارو، کمپرسورهای برودت و پارو مخزن شیر جهت تضمین کیفیت و طول عمر قطعات.", "Configuring paddle timing, compressor delays, and hopper paddle parameters.", "Palet, kompresor ve hazne paleti zamanlamalarini hassas ayarlama."));
        cards[0] = (guide_card_item_t){G_TXT("ایمنی استارت", "Safety Delay", "Gecikme"), 0x0284c7, G_TXT("تاخیر استارت کمپرسور سیلندر", "Compressor Startup Delay", "Silindir Kompresor Baslama Gecikmesi"),
            G_TXT("با زدن کلید تولید، اول پارو روشن می‌شود تا مایه در سیلندر روان شود؛ سپس پس از سپری شدن این زمان، کمپرسور سیلندر روشن می‌شود تا از پریدن فیوز برق مغازه بر اثر استارت همزمان جلوگیری شود.", "Paddle starts first. Cylinder compressor starts after this delay to prevent simultaneous high startup currents.", "Once palet baslar, ardindan bu sure sonunda kompresor devreye girerek ani akim cekilmesini onler."),
            G_TXT("نکته: مقدار پیشنهادی کارخانه ۵ ثانیه است (بازه مجاز: ۳ تا ۶۰ ثانیه).", "Note: Factory default is 5 seconds (range: 3-60s).", "Not: Fabrika degeri 5 saniyedir (aralik: 3-60 sn).")};
        cards[1] = (guide_card_item_t){G_TXT("قوام بستنی", "Consistency", "Kivam"), 0x16a34a, G_TXT("کارکرد دوره‌ای پارو", "Paddle Duty Timer", "Palet Calisma Zamanlayicisi"),
            G_TXT("تایمر کارکرد پارو مدت زمان چرخش مداوم موتور پارو در فاز تولید و خروج بستنی را تعیین می‌کند تا بافت بستنی یکدست و لطیف شود.", "Sets active rotation time for the cylinder beater motor to ensure uniform texture.", "Silindir palet motorunun calisma suresini belirler."),
            G_TXT("نکته: تنظیم پیشنهادی کارخانه ۱۵ دقیقه است (بازه مجاز: ۱۰ الی ۲۰ دقیقه).", "Note: Recommended: 15 minutes (range: 10-20 min).", "Not: Onerilen ayar: 15 dakika (aralik: 10-20 dk).")};
        cards[2] = (guide_card_item_t){G_TXT("حفاظت تسمه", "Stall Guard", "Kayis Korumasi"), 0xea580c, G_TXT("مهلت سنسور دور پولی گیربکس", "Pulley Sensor Stall Timeout", "Kasnak Sensoru Zaman Asimi"),
            G_TXT("سنسور القایی چرخش پولی گیربکس را می‌شمارد. اگر تسمه شل یا پاره شود یا بار سنگین گیربکس را قفل کند، در صورت نرسیدن پالس طی این زمان، فورا خطای توقف پولی (کد ۰۱) صادر شده و تمام سیستم خاموش می‌شود تا موتور نسوزد.", "Inductive sensor monitors gearbox pulley. If no pulses arrive within this timeout, E01 fault trips the system.", "Kasnaktan bu sure boyunca sinyal alinmazsa E01 arizasi verilir ve motor korunur."),
            G_TXT("نکته: مقدار استاندارد ۴ ثانیه است. اگر خطای کاذب می‌دهد، فاصله نوک سنسور تا پولی را روی ۲ میلی‌متر تنظیم نمایید.", "Note: Default: 4s. Verify 2mm physical gap between sensor and target.", "Not: Standart 4 saniyedir. Sensor mesafesini 2 mm olarak ayarlayin.")};
        cards[3] = (guide_card_item_t){G_TXT("کنترل بار و کلاچ", "Load & Clutch", "Asiri Yuk Korumasi"), 0x0284c7, G_TXT("حفاظت هوشمند جریان و میکروسوئیچ کلاچ", "Current Sensor & Mechanical Clutch Guard", "Akim Sensoru ve Mekanik Debriyaj Korumasi"),
            G_TXT("حفاظت هوشمند دستگاه با سنسور جریان و میکروسوئیچ شافت پولی انجام می‌شود. در صورت سفت شدن بیش از حد بستنی یا فشار مکانیکی، سنسور جریان و میکروسوئیچ بلافاصله واکنش داده و مانع استارت یا آسیب به موتور و کمپرسور می‌شوند.", "The controller protects equipment via precision current sensing and rear shaft clutch microswitch, preventing high-pressure motor damage.", "Sistem akim sensoru ve mekanik switch ile korunur; asiri yuk veya basincta motor ve kompresor otomatik durdurulur."),
            G_TXT("نکته: در صورت احساس سفتی، قوام بستنی و فاصله میکروسوئیچ اهرم را بررسی نمایید.", "Note: Check ice cream firmness and clutch switch adjustment if triggered.", "Not: Yuk algilanirsa dondurma kivamini ve switch mesafesini kontrol edin.")};
        cards[4] = (guide_card_item_t){G_TXT("بهداشت شیر", "Hygiene", "Sut Hijyeni"), 0x7c3aed, G_TXT("پارو آرام مخزن بن‌ماری شیر", "Hopper Milk Paddle Duty", "Hazne Palet Periyodu"),
            G_TXT("موتور پارو بالای مخزن با چرخش دوره‌ای (ثانیه کار / دقیقه استراحت)، مانع ته‌نشینی چربی و خامه و ترشیدن شیر در مخزن می‌شود.", "Cycles the small hopper paddle (seconds on / minutes off) to prevent fat stratification.", "Kucuk hazne motoru aralikli calisarak yag tabakalasmasini ve sutun bozulmasini onler."),
            G_TXT("نکته: تنظیم پیشنهادی کارخانه ۲۰ ثانیه کارکرد و ۱۵ دقیقه استراحت است.", "Note: Recommended: 20s run, 15min rest.", "Not: Onerilen ayar: 20 sn calisma, 15 dk bekleme.")};
        card_cnt = 5;
        snprintf(status_line, sizeof(status_line), "%s: %ld s | %s: %ld/%ld s | %s: %ld s",
                 G_TXT("تاخیر", "Delay", "Gecikme"), (long)startup_timer,
                 G_TXT("پارو", "Paddle", "Palet"), (long)paddle_timer_1, (long)paddle_timer_2,
                 G_TXT("پولی", "Pulley", "Kasnak"), (long)mid_eye_timer);
        break;

    case GUIDE_ID_TAB_F2:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تنظیمات ساعت، تاریخ و کالیبراسیون", "Tab F2: Clock, Date & Calibration", "F2 Sekmesi: Saat, Tarih ve Kalibrasyon"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("ساعت رسمی دستگاه، باتری پشتیبان و کالیبراسیون دقیق سنسورهای دمای مخزن و سیلندر.", "Real-time clock DS3231, coin cell battery, and thermocouple temperature offsets.", "Gercek zamanli saat, pil durumu ve sicaklik sensor kalibrasyonu."));
        cards[0] = (guide_card_item_t){G_TXT("تنظیم زمان", "Time Set", "Zaman Ayari"), 0x0284c7, G_TXT("تنظیم ساعت و تاریخ رسمی", "Clock & Calendar Configuration", "Saat ve Tarih Ayari"),
            G_TXT("با کلیدهای مثبت و منفی مقادیر سال، ماه، روز، ساعت و دقیقه را تنظیم کرده و سپس کلید سبز ذخیره تنظیمات را بزنید. ساعت دقیق برای ثبت تاریخچه خطاها و روزشمار اقساط الزامی است.", "Use + and - to set Year, Month, Day, Hour, and Minute, then press green Save button. Accurate time is required for logs and day-lock leasing.", "+ ve - tuslari ile yili, ayi, gunu, saati ve dakikayi ayarlayip Yesil Kaydet tusuna basin."),
            G_TXT("نکته: پس از تغییر مقادیر، حتماً کلید ذخیره را لمس کنید.", "Note: Always press Save after altering time values.", "Not: Degerleri degistirdikten sonra mutlaka Kaydet butonuna basin.")};
        cards[1] = (guide_card_item_t){G_TXT("نگهداری برد", "Hardware", "Donanim"), 0xea580c, G_TXT("باتری پشتیبان ساعت حافظه", "RTC Backup Coin Cell", "Hafiza Saati Pili"),
            G_TXT("با قطع برق شهر، ساعت دستگاه به لطف باتری سکه‌ای روی برد اصلی حفظ می‌شود. اگر پس از خاموش بودن طولانی، تاریخ دستگاه به سال ۲۰۰۰ بازگشت، باتری سکه‌ای باید تعویض گردد.", "Maintains clock across power cuts. If date resets to 2000 after extended shutdown, replace CR2032 battery.", "Elektrik kesildiginde saatin calismasi CR2032 dugme pil ile saglanir. Tarih 2000 yilina donerse pili degistirin."),
            G_TXT("نکته: عمر مفید باتری سکه‌ای بین ۳ تا ۵ سال است.", "Note: Typical battery lifespan is 3 to 5 years.", "Not: Pil omru 3 ila 5 yildir.")};
        cards[2] = (guide_card_item_t){G_TXT("هوشمند", "Auto Sync", "Otomatik Senkron"), 0x16a34a, G_TXT("همگام‌سازی اینترنتی ساعت", "Internet Clock Sync (NTP)", "Internet Saat Senkronu"),
            G_TXT("با اتصال دستگاه به مودم مغازه در منوی وای‌فای، ساعت دستگاه به صورت کاملاً خودکار با سرورهای زمانی اینترنت هماهنگ می‌شود و نیازی به تنظیم دستی نخواهد بود.", "When connected to WiFi with internet in Tab F4, the controller syncs time automatically.", "Cihaz F4 sekmesinde internete baglandiginda saat otomatik olarak esitlenir."),
            NULL};
        cards[3] = (guide_card_item_t){G_TXT("کالیبراسیون", "Offsets", "Kalibrasyon"), 0x7c3aed, G_TXT("کالیبراسیون دماسنج مخزن و سیلندر", "Temperature Sensor Offsets", "Sicaklik Sensor Ofsetleri"),
            G_TXT("اگر دماسنج مرجع کالیبره دارید و دمای واقعی با عدد نمایشگر تفاوت دارد، می‌توانید با افست کالیبراسیون عدد را تا دهم درجه اصلاح کنید.", "Fine-tune display readings by tenths of a degree if comparing against calibrated lab gauge.", "Referans termometre ile ekrandaki deger arasinda fark varsa ofset vererek hassas ayar yapabilirsiniz."),
            G_TXT("نکته: سنسورها در کارخانه کالیبره شده‌اند؛ فقط در صورت تعویض پراب به آن دست بزنید.", "Note: Factory calibrated. Alter only when replacing physical probes.", "Not: Sensorler fabrikada kalibre edilmistir; yalnizca sensor degisiminde degistirin.")};
        card_cnt = 4;
        snprintf(status_line, sizeof(status_line), "%s: %+.1f°C | %s: %+.1f°C",
                 G_TXT("افست مخزن", "Hopper Offset", "Hazne Ofset"), max6675_get_cal_offset_top(),
                 G_TXT("افست سیلندر", "Cylinder Offset", "Silindir Ofset"), max6675_get_cal_offset_cyl());
        break;

    case GUIDE_ID_TAB_F3:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تنظیمات کنترل برودت و نگهداری شبانه", "Tab F3: Cooling & Night Preservation", "F3 Sekmesi: Sogutma ve Gece Bekleme"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("تنظیم دمای بهداشتی مخزن شیر، استراحت شبانه، تازه نگه‌داشتن بستنی در استندبای و کارکرد نجات اضطراری.", "Hopper thermostat setpoints, night holding cycles, and emergency fallback cooling.", "Hazne termostat degerleri, gece koruma ve acil durum calismasi."));
        cards[0] = (guide_card_item_t){G_TXT("ترموستات", "Thermostat", "Termostat"), 0x0284c7, G_TXT("دمای قطع و وصل ترموستات مخزن شیر", "Hopper Cut-in & Cut-out Setpoints", "Hazne Termostat Limitleri"),
            G_TXT("نگهدارنده مخزن تا رسیدن به دمای قطع (پیش‌فرض: ۳+ درجه) خنک می‌کند و با گرم شدن مایه تا دمای وصل (پیش‌فرض: ۶+ درجه) مجدداً روشن می‌شود.", "Holding stops at cut-out (+3°C) and restarts at cut-in (+6°C).", "Bekleme +3 derecede durur ve +6 derecede tekrar devreye girer."),
            G_TXT("نکته: بازه ایده‌آل نگهداری شیر بین ۳+ تا ۵+ درجه سانتی‌گراد است تا از رشد میکروب جلوگیری شود.", "Note: Safe health standard: between +3°C and +5°C.", "Not: Bakteri olusumunu onlemek icin ideal sut sicakligi +3 ila +5 derecedir.")};
        cards[1] = (guide_card_item_t){G_TXT("استراحت شب", "Preservation", "Gece Koruma"), 0x16a34a, G_TXT("حالت نگهداری شبانه شیر", "Night Holding Mode", "Gece Bekleme Modu"),
            G_TXT("در پایان شیفت کاری نیازی به تخلیه مایه نیست. کافیست کلید نگهداری را بزنید تا دستگاه با حداقل مصرف برق، مایه را در دمای خنک نگه دارد.", "Maintains mix safely overnight with minimal electrical consumption.", "Vardiya sonunda karisimi bosaltmaya gerek yoktur. Bekleme modu minimum elektrik ile karisimi korur."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("کیفیت بستنی", "Standby", "Dondurma Kalitesi"), 0x7c3aed, G_TXT("تازه نگه‌داشتن بستنی در استندبای", "Standby Cylinder Refresh", "Beklemede Silindir Tazeleme"),
            G_TXT("در ساعات خلوتی که بستنی کشیده نمی‌شود، کمپرسور سیلندر هر چند ساعت به مدت کوتاهی روشن می‌شود تا بستنی داخل سیلندر آب یا دو فاز نشود.", "Runs cylinder compressor periodically during quiet hours to preserve cylinder texture.", "Satis olmayan durgun saatlerde silindir kompresor kisa sureli calisarak silindirdeki dondurmanin erimesini onler."),
            G_TXT("نکته: زمان استراحت و کارکرد استندبای در این تب قابل تغییر است.", "Note: Configurable standby duty in this tab.", "Not: Bekleme calisma ve dinlenme sureleri bu sekmede ayarlanabilir.")};
        cards[3] = (guide_card_item_t){G_TXT("نجات فرآیند", "Fallback", "Acil Durum"), 0xea580c, G_TXT("کارکرد خنک‌کاری اضطراری", "Emergency Fallback Cooling", "Sensor Arizasi Acil Sogutma"),
            G_TXT("اگر سیم سنسور دما قطع شود (کد خطای ۰۴ یا ۰۵)، سیستم کار را متوقف نمی‌کند، بلکه طبق تایمرهای اضطراری به خنک‌کاری ادامه می‌دهد تا بستنی و شیر خراب نشوند تا سرویس‌کار برسد.", "If sensor disconnects, controller runs safety time-proportional cooling so mix does not spoil.", "Sensor kablosu kopsa bile cihaz acil durum zamanlayicisiyla sogutmaya devam ederek sutun bozulmasini engeller."),
            G_TXT("نکته: تنظیم کارخانه ۵ دقیقه کار و ۱۰ دقیقه استراحت نگهدارنده است.", "Note: Factory default: 5min run, 10min rest.", "Not: Fabrika ayari: 5 dk calisma, 10 dk bekleme.")};
        card_cnt = 4;
        snprintf(status_line, sizeof(status_line), "%s: %ld°C | %s: %ld°C",
                 G_TXT("قطع سرمایش", "Cut-out", "Kesme"), (long)hold_temp_off_c(),
                 G_TXT("وصل مجدد", "Cut-in", "Baslama"), (long)hold_temp_on_c());
        break;

    case GUIDE_ID_TAB_F4:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تنظیمات ارتباط وای‌فای و وب‌پورتال", "Tab F4: WiFi & Web Portal", "F4 Sekmesi: WiFi ve Web Portal"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("ارتباط بی‌سیم با موبایل، مانیتورینگ بدون نیاز به نصب برنامه و آپدیت آنلاین ابری.", "Direct hotspot, shop router connection, smartphone dashboard, and cloud OTA updates.", "Telefona kablosuz baglanti, programsiz izleme ve bulut guncelleme."));
        cards[0] = (guide_card_item_t){G_TXT("اتصال مستقیم", "Direct AP", "Direkt Baglanti"), 0x0284c7, G_TXT("اتصال مستقیم بی‌سیم به دستگاه", "Direct Machine Hotspot", "Cihazin Kendi WiFi Agi"),
            G_TXT("وای‌فای گوشی خود را روشن کرده و به شبکه دستگاه وصل شوید (رمز پیش‌فرض: 12345678). سپس در مرورگر آدرس 192.168.4.1 را وارد کنید تا پنل کاربری موبایل باز شود.", "Connect smartphone to machine WiFi (Password: 12345678), then open 192.168.4.1 in browser.", "Telefonunuzun WiFi agindan cihaza baglanin (Sifre: 12345678). Tarayicida 192.168.4.1 adresini acin."),
            G_TXT("نکته: در وب‌پورتال می‌توانید دماها، شمارنده فروش و پارامترها را روی گوشی ببینید.", "Note: Access live temperatures, dispenser statistics, and controls on phone.", "Not: Web portalda sicakliklari, satis sayacini ve parametreleri gorebilirsiniz.")};
        cards[1] = (guide_card_item_t){G_TXT("شبکه مغازه", "Shop Router", "Dukkan Modemi"), 0x16a34a, G_TXT("اتصال به مودم وای‌فای مغازه", "Shop WiFi Router Connection", "Modem Uzerinden Baglanti"),
            G_TXT("در منوی تنظیمات وای‌فای دکمه اسکن شبکه‌ها را بزنید، نام مودم مغازه را انتخاب و رمز آن را وارد کنید. پس از اتصال، دستگاه یک IP محلی دریافت کرده و همیشه در دسترس خواهد بود.", "Scan networks, select shop SSID, enter password. Machine gets a local IP for permanent access.", "Ag tara butonuna basip modeminizi secin ve sifreyi girin. Cihaz yerel IP alacaktir."),
            G_TXT("نکته: با اتصال به اینترنت، ساعت دستگاه تنظیم و آماده آپدیت ابری می‌شود.", "Note: Internet connection enables NTP clock sync and cloud OTA.", "Not: Internet baglantisi ile saat otomatik esitlenir ve guncelleme hazir olur.")};
        cards[2] = (guide_card_item_t){G_TXT("آپدیت ابری", "Cloud OTA", "Bulut Guncelleme"), 0x7c3aed, G_TXT("بروزرسانی آنلاین ابری نرم‌افزار", "Over-The-Air Cloud Updates", "Cevrimici Yazilim Guncelleme"),
            G_TXT("بدون نیاز به کامپیوتر یا کابل پروگرم! با زدن دکمه بررسی بروزرسانی، دستگاه به سرور رسمی آستیاج متصل شده و ظرف ۳۰ ثانیه دستگاه شما را به جدیدترین نسخه آپدیت می‌کند.", "Press Check for Update to download latest firmware directly from ASTIAJ cloud server.", "Kabloya gerek olmadan, Guncelleme Kontrol butonuna basarak cihazi 30 saniyede en son surume yukseltebilirsiniz."),
            G_TXT("نکته: در صورت وجود آپدیت، متن تغییرات به زبان شما نمایش داده خواهد شد.", "Note: Changelog is presented in your active language.", "Not: Guncelleme varsa degisiklik listesi kendi dilinizde gosterilir.")};
        card_cnt = 3;
        snprintf(status_line, sizeof(status_line), "%s: %s (%s)",
                 G_TXT("وضعیت وای‌فای", "WiFi", "WiFi Durumu"),
                 wifi_is_connected() ? G_TXT("متصل به مودم", "Connected", "Modeme Bagli") : G_TXT("هات‌اسپات مستقیم", "Direct Hotspot", "Direkt Ag"),
                 wifi_is_connected() ? wifi_get_ip_str() : "192.168.4.1");
        break;

    case GUIDE_ID_TAB_F5:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تست و دیاگ سخت‌افزار (ورودی و خروجی‌ها)", "Tab F5: Hardware Diagnostics & I/O", "F5 Sekmesi: Donanim Testi ve Giris/Cikislar"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("پایش بلادرنگ سنسورها و کلیدها، تست دستی رله‌های قدرت و بررسی دماهای لحظه‌ای دستگاه.", "Live digital inputs status, 5-second manual relay actuation tests, and thermocouple readouts.", "Canli sensor durumlari, elle 5 saniyelik role testleri ve anlik sicaklik izleme."));
        cards[0] = (guide_card_item_t){G_TXT("ورودی‌ها", "Inputs", "Girisler"), 0x0284c7, G_TXT("وضعیت سنسورها و ورودی‌های دیجیتال (I0 تا I7)", "Digital Inputs Status (I0 - I7)", "Dijital Giris Durumlari (I0 - I7)"),
            G_TXT("با تحریک هر سنسور چراغ نئونی سبز آن فورا روشن می‌شود:\n• سنسور دور پولی (I0)\n• فلوتر مخزن شیر (I1)\n• کلاچ عقب سیلندر (I2)\n• کنترل‌بار موتور (I3)\n• اهرم تخلیه جلو (I4)\n• کنترل‌بار کمپرسور (I5)\n• شستی استپ پنل (I6)\n• شستی استارت پنل (I7)", "Indicators turn green when physical inputs trigger (Pulley I0, Hopper I1, Clutch I2, Motor overload I3, Lever I4, Comp overload I5, Stop I6, Start I7).", "Girisler tetiklendiginde yesil yanar: Kasnak I0, Sut haznesi I1, Kivam switchi I2, Motor yuk I3, Servis kolu I4, Kompr yuk I5, Stop I6, Start I7."),
            G_TXT("نکته: در صورت عمل نکردن هر سنسور، چراغ سبز مربوطه را جهت تست سیم‌کشی بررسی کنید.", "Note: Use indicator LEDs to verify field wiring without multimeters.", "Not: Saha kablo baglantisini test etmek icin gosterge LED'lerini kullanin.")};
        cards[1] = (guide_card_item_t){G_TXT("خروجی‌ها", "Outputs", "Cikislar"), 0x16a34a, G_TXT("تست دستی تک‌تک رله‌های خروجی قدرت", "Manual Output Relay Actuation", "Cikis Rolelerini Elle Test Etme"),
            G_TXT("در وضعیت توقف، با لمس هر کلید، رله مربوطه ۵ ثانیه فعال می‌شود تا سلامت کنتاکتور، بوبین و سیم‌کشی بررسی شود (نگهدارنده Q0، کمپرسور بزرگ Q1، موتور پارو Q2، پمپ کرم Q3، پمپ رنگ Q4، شیر باد Q5، پارو مخزن شیر، رله ریست).", "Tap any output button (holding, main compressor, beater, pumps, air valve, reset relay) to pulse it for 5s.", "Bakim sirasinda herhangi bir roleye dokunarak 5 saniye calistirabilir ve tesisati test edebilirsiniz."),
            G_TXT("هشدار: هنگام تست خروجی‌ها از قطعات متحرک، تسمه و پروانه‌ها فاصله بگیرید.", "Warning: Keep hands away from moving belts and fans during relay tests.", "Uyari: Roleleri test ederken ellerinizi kayis ve fanlardan uzak tutun.")};
        cards[2] = (guide_card_item_t){G_TXT("دماسنج‌ها", "Sensors", "Sicaklik"), 0x7c3aed, G_TXT("پایش زنده سنسورهای دمای T1 و T2", "Live Thermocouple Temperature Readouts", "Canli Termokupl Sicaklik Takibi"),
            G_TXT("نمایش دمای لحظه‌ای مخزن شیر (ترموکوپل T1) و دمای بدنه سیلندر (ترموکوپل T2) بر حسب دهم درجه سانتی‌گراد همراه با تشخیص خودکار قطعی پراب یا اتصال کوتاه.", "Live readouts of hopper mix temperature (T1) and cylinder wall (T2) with probe disconnection guard.", "Sut haznesi (T1) ve silindir gozdesi (T2) anlik sicakliklari ile sensor arizasi tespiti."),
            NULL};
        card_cnt = 3;
        snprintf(status_line, sizeof(status_line), "%s: %.1f°C | %s: %.1f°C",
                 G_TXT("مخزن شیر", "Hopper", "Hazne"), max6675_get_temp_top(),
                 G_TXT("سیلندر", "Cylinder", "Silindir"), max6675_get_temp_cylinder());
        break;

    case GUIDE_ID_TAB_F6:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای تنظیمات فروش، شارژ سرما و حفاظت", "Tab F6: Dispense, Rechill & Safeties", "F6 Sekmesi: Servis, Sogutma ve Koruma"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("تنظیم هوشمند شارژ برودت پس از کشیدن قیف، فروش عمده، سقف زمان انجماد و اینترلاک‌های ایمنی.", "Configuring post-dispense rechill, bulk hold, freeze ceiling, and factory safeguards.", "Servis sonrasi sogutma, toptan satis, donma limiti ve guvenlik ayarlari."));
        cards[0] = (guide_card_item_t){G_TXT("شارژ سرما", "Rechill Run", "Sogutma"), 0x0284c7, G_TXT("شارژ برودت پس از فروش (ثانیه)", "Post-Dispense Rechill Run", "Servis Sonrasi Sogutma"),
            G_TXT("مدت زمان روشن ماندن کمپرسور بزرگ پس از خروج بستنی و رها شدن اهرم. با ورود مایه گرم از مخزن، این شارژ سریع از افت قوام بستنی‌های بعدی پیشگیری می‌کند.", "Main compressor run time after lever release to restore cylinder temperature after warm mix feeds in.", "Kulah servisinden sonra silindirin soguklugunu korumak icin buyuk kompresorun calisma suresidir."),
            G_TXT("نکته: تنظیم کارخانه ۴۵ ثانیه است (بازه مجاز: ۱۰ تا ۶۰ ثانیه).", "Note: Factory default is 45s (range: 10-60s).", "Not: Fabrika ayari 45 sn (aralik: 10-60 sn).")};
        cards[1] = (guide_card_item_t){G_TXT("فروش عمده", "Bulk Hold", "Toptan Satis"), 0x16a34a, G_TXT("نگهداشت اهرم برای فروش عمده (ثانیه)", "Bulk Dispense Lever Hold", "Kilo ile Satis Kol Suresi"),
            G_TXT("اگر کاربر اهرم را به طور مداوم برای پر کردن ظروف حجیم باز نگه دارد، با عبور از این زمان کمپرسور بزرگ فورا وارد مدار می‌شود تا دمای سیلندر سقوط نکند.", "Continuous lever pull duration for bulk container sales triggering immediate cooling.", "Kolu basili tutarak buyuk kap doldururken kompresorun hemen devreye girme suresi."),
            G_TXT("نکته: تنظیم کارخانه ۶ ثانیه است (بازه مجاز: ۲ تا ۱۵ ثانیه).", "Note: Factory default is 6s (range: 2-15s).", "Not: Fabrika ayari 6 sn (aralik: 2-15 sn).")};
        cards[2] = (guide_card_item_t){G_TXT("کاهش استهلاک", "Scoops Count", "Kulah Sayisi"), 0x7c3aed, G_TXT("تعداد قیف فروش تا شارژ سرمایش مجدد", "Scoop Count Trigger for Rechill", "Sogutma Kulah Sayisi"),
            G_TXT("تعداد قیف‌های فروخته شده پیش از استارت مجدد کمپرسور بزرگ. در ساعات شلوغی با تنظیم عدد روی ۲ یا ۳، تعداد استارت/استپ‌های کمپرسور کاهش یافته و عمر بوبین زیاد می‌شود.", "Number of dispensed cones triggering a cooling cycle to reduce motor cycling wear.", "Kompresorun devreye girmesi icin gereken kulah sayisi. Motor omrunu uzatir."),
            G_TXT("نکته: تنظیم کارخانه ۳ قیف است (بازه مجاز: ۱ تا ۱۰ قیف).", "Note: Factory default is 3 scoops (range: 1-10).", "Not: Fabrika degeri 3 kulahtir (aralik: 1-10).")};
        cards[3] = (guide_card_item_t){G_TXT("حفاظت کمپرسور", "Freeze Limit", "Sure Siniri"), 0xea580c, G_TXT("سقف زمان مجاز انجماد سیلندر (دقیقه)", "Maximum Freeze Timeout", "Maksimum Dondurma Suresi"),
            G_TXT("حداکثر زمان مجاز روشن ماندن مداوم کمپرسور بزرگ در یک سیکل. اگر سنسور کلاچ خراب شود یا گاز دستگاه کم باشد، کنترلر پس از این زمان کمپرسور را برای نجات موتور خاموش و آلارم می‌دهد.", "Safety cutoff shutting off big compressor if ice cream does not harden within this limit.", "Gaz eksikligi veya switch arizasinda motorun yanmasini onleyen azami calisma suresidir."),
            G_TXT("نکته: تنظیم کارخانه ۱۵ دقیقه است (بازه مجاز: ۵ تا ۳۰ دقیقه).", "Note: Factory default is 15 min (range: 5-30 min).", "Not: Fabrika ayari 15 dakikadir (aralik: 5-30 dk).")};
        cards[4] = (guide_card_item_t){G_TXT("جلوگیری هدررفت", "Lever Alarm", "Kol Uyarisi"), 0xdc2626, G_TXT("سقف هشدار باز ماندن اهرم (ثانیه)", "Stuck Lever Audible Alarm", "Kol Acik Kalma Alarm Suresi"),
            G_TXT("حداکثر زمان مجاز باز ماندن دسته پیش از فعال شدن بوق هشدار ممتد و ثبت خطای ۰۸. از هدر رفتن مایه مخزن روی زمین بر اثر گیر کردن دسته جلوگیری می‌کند.", "Timer limit before triggering buzzer and fault E08 for unclosed or jammed dispensing lever.", "Kol takili kalirsa urunun bosa akmasini onleyen sesli ve gorsel alarm suresidir."),
            G_TXT("نکته: تنظیم کارخانه ۲۵ ثانیه است (بازه مجاز: ۱۵ تا ۱۲۰ ثانیه).", "Note: Factory default is 25s (range: 15-120s).", "Not: Fabrika degeri 25 sn (aralik: 15-120 sn).")};
        card_cnt = 5;
        snprintf(status_line, sizeof(status_line), "%s: %ld s | %s: %ld s | %s: %ld | %s: %ld min",
                 G_TXT("شارژ", "Run", "Calisma"), (long)dispense_comp_run_s,
                 G_TXT("اهرم", "Hold", "Kol"), (long)front_hold_comp_s,
                 G_TXT("قیف", "Scoops", "Kulah"), (long)scoops_for_comp,
                 G_TXT("سقف", "Limit", "Sinir"), (long)max_freeze_timeout_m);
        break;

    case GUIDE_ID_COUNTER:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای شمارنده دیجیتال قیف", "Dispenser Cone Counter Guide", "Kulah Sayaci Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("آمار دقیق بستنی‌های توزیع‌شده، کنترل دخل شیفت و کیلومترشمار کل دستگاه.", "Dispense statistics, shift sales audit, and non-resettable odometer.", "Dagitilan dondurma sayisi, vardiya kasa kontrolu ve toplam cihaz omru sayaci."));
        cards[0] = (guide_card_item_t){G_TXT("شیفت جاری", "Shift", "Vardiya"), 0x0284c7, G_TXT("شمارنده شیفت کاری (قابل صفر کردن)", "Resettable Shift Counter", "Sifirlanabilir Vardiya Sayaci"),
            G_TXT("تعداد بستنی‌های کشیده شده در شیفت کاری جاری را می‌شمارد. در پایان هر شیفت، مدیر می‌تواند با لمس کادر شمارنده و وارد کردن رمز ۴ رقمی، آن را برای شیفت بعد صفر نماید تا دخل مغازه با آمار فروش تطبیق یابد.", "Counts cones pulled during current shift. Resettable via manager PIN at end of shift.", "Mevcut vardiyada satilan kulahlari sayar. Yonetici 4 haneli PIN ile vardiya sonunda sifirlayabilir."),
            G_TXT("نکته: رمز پیش‌فرض مدیر 1234 است (قابل تغییر در منوی امنیت).", "Note: Default PIN: 1234 (changeable in security menu).", "Not: Varsayilan PIN: 1234 (guvenlik menusunden degistirilebilir).")};
        cards[1] = (guide_card_item_t){G_TXT("آمار روزانه", "Daily", "Gunluk"), 0x16a34a, G_TXT("شمارنده فروش امروز (ریست خودکار)", "Daily Auto Counter", "Gunluk Otomatik Sayac"),
            G_TXT("تعداد کل قیف‌های فروخته شده از ساعت ۱۲ شب گذشته تا کنون را نشان می‌دهد و هر شب در ساعت ۰۰:۰۰ به طور خودکار ریست شده و آمار در حافظه ثبت می‌گردد.", "Tracks cones sold today. Automatically resets at midnight and saves to history.", "Gece 00:00 dan itibaren satilan kulahlari sayar ve her gece otomatik sifirlanarak hafizaya kaydeder."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("کیلومترشمار", "Odometer", "Toplam Omur"), 0x7c3aed, G_TXT("شمارنده کل مادام‌العمر (غیرقابل تغییر)", "Lifetime Machine Odometer", "Silinemez Toplam Sayac"),
            G_TXT("مشابه کیلومترشمار اتومبیل، تعداد کل بستنی‌هایی که دستگاه از روز اول تولید کرده در حافظه ماندگار برد ذخیره می‌شود و تحت هیچ شرایطی پاک نمی‌شود.", "Permanent lifetime counter stored in non-volatile memory; cannot be reset.", "Arac kilometresi gibi, cihaz satin alindigi gunden beri uretilen toplam dondurmayi gosterir ve silinemez."),
            G_TXT("نکته: هنگام خرید یا فروش دستگاه دست دوم، با این عدد کارکرد واقعی دستگاه را بسنجید.", "Note: Essential metric when evaluating machine usage for resale.", "Not: Ikinci el alim satiminda makinenin gercek yipranmasini bu sayac belirler.")};
        card_cnt = 3;
        snprintf(status_line, sizeof(status_line), "%s: %ld | %s: %ld | %s: %ld",
                 G_TXT("شیفت", "Shift", "Vardiya"), (long)icecream_counter,
                 G_TXT("امروز", "Today", "Bugun"), (long)icecream_counter_daily,
                 G_TXT("کل مادام‌العمر", "Lifetime", "Toplam"), (long)icecream_counter_total);
        break;

    case GUIDE_ID_TEMP_HOPPER:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای دماسنج مخزن بن‌ماری شیر", "Thermocouple T1: Milk Hopper Guide", "T1 Termokupl Hazne Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("سنسور صنعتی دمای مخزن شیر بالای دستگاه و حفاظت از کیفیت بهداشتی مایه.", "Industrial sanitary hopper thermocouple for mix preservation.", "Ust hazne sut sicakligi ve hijyen koruma kilavuzu."));
        cards[0] = (guide_card_item_t){G_TXT("دماسنج دیجیتال", "Probe", "Sensor"), 0x0284c7, G_TXT("اندازه‌گیری لحظه‌ای دمای شیر", "Real-Time Milk Temperature", "Anlik Sut Sicakligi Olcumu"),
            G_TXT("پراب استیل صنعتی در تماس با مایه شیر قرار دارد و دمای دقیق را به مدار کنترل ارسال می‌کند تا نگهدارنده در دمای تنظیم‌شده کار کند.", "Stainless steel food probe immersed in milk transmits precision temperature to PLC.", "Paslanmaz celik sensor sut ile temas halinde olup beklemenin dogru calismasini saglar."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("مفهوم رنگ‌ها", "Color Code", "Renk Anlamlari"), 0x16a34a, G_TXT("راهنمای رنگ وضعیت دمای مخزن شیر", "T1 Capsule Color Meaning", "T1 Kapsul Renk Rehberi"),
            G_TXT("رنگ آبی: نگهدارنده روشن است و مخزن در حال خنک شدن است.\nرنگ سبز: دمای مخزن در بازه بهداشتی مطلوب است (بین ۳+ تا ۶+ درجه).\nرنگ قرمز: دمای شیر بالای ۸ درجه است یا سیم سنسور قطع شده است.", "Blue: Cooling active.\nGreen: Safe +3°C to +6°C.\nRed: Warning temp above +8°C or disconnected.", "Mavi: Sogutma devrede.\nYesil: Guvenli aralik (+3 ile +6 derece arasi).\nKirmizi: Sicaklik +8 dereceden yuksek veya sensor kopuk."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("عیب‌یابی", "Troubleshoot", "Ariza Cozum"), 0xea580c, G_TXT("اگر خطای قطعی سنسور مخزن (کد ۰۴) نمایش داده شد چه کنیم؟", "Fault E04: Sensor Disconnect", "E04 Sensor Arizasi"),
            G_TXT("کد خطای ۰۴ نشانه قطعی سیم یا شل شدن سوکت سنسور مخزن است. در این حالت دستگاه موقتاً طبق تایمر اضطراری خنک می‌کند تا شیر فاسد نشود. سوکت پراب پشت مخزن را چک کنید.", "E04 indicates disconnected probe. Controller maintains emergency cooling. Inspect socket wiring.", "E04 sensor kablosunun koptugunu gosterir. Cihaz acil sogutmaya gecer. Soketi kontrol edin."),
            NULL};
        card_cnt = 3;
        if (max6675_is_top_valid()) {
            snprintf(status_line, sizeof(status_line), "%s: %.1f°C | %s",
                     G_TXT("دمای لحظه‌ای مخزن", "Hopper Temp", "Hazne Sicakligi"), max6675_get_temp_top(),
                     G_TXT("پراب: سالم و کالیبره", "Probe: OK", "Sensor: Saglam"));
        } else {
            snprintf(status_line, sizeof(status_line), "%s: %s | %s",
                     G_TXT("دمای لحظه‌ای مخزن", "Hopper Temp", "Hazne Sicakligi"),
                     G_TXT("سنسور وصل نیست", "Disconnected", "Bagli Degil"),
                     G_TXT("پراب: قطع", "Probe: Fault", "Sensor: Ariza"));
        }
        break;

    case GUIDE_ID_TEMP_CYL:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای دماسنج سیلندر انجماد اصلی", "Thermocouple T2: Cylinder Guide", "T2 Termokupl Silindir Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("پایش دمای انجماد سیلندر و مدار حفاظت ضد یخ‌زدگی شفت پارو.", "Cylinder freezing temperature monitoring and anti-freeze shaft protection.", "Silindir dondurma sicakligi ve mil donma korumasi."));
        cards[0] = (guide_card_item_t){G_TXT("دمای انجماد", "Freezing", "Dondurma"), 0x0284c7, G_TXT("پایش دمای بستنی داخل سیلندر", "Core Ice Cream Temperature", "Silindir Ici Sicaklik Takibi"),
            G_TXT("دمای دقیق بستنی در حال تشکیل در سیلندر را نشان می‌دهد. بستنی معمولاً در دمای منفی ۵ تا منفی ۹ درجه سانتی‌گراد آماده کشیدن است.", "Indicates real-time core mix temperature. Dispense readiness is typically -5°C to -9°C.", "Silindir icindeki dondurmanin anlik sicakligini gosterir. Dondurma genellikle -5 ile -9 derece arasinda servise hazirdir."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("ایمنی مطلق", "Anti-Freeze", "Tam Guvenlik"), 0xdc2626, G_TXT("حفاظت ضد انجماد شفت و گیربکس", "Anti-Freeze Shaft Protection", "Mil ve Sanziman Donma Korumasi"),
            G_TXT("اگر دمای سیلندر به منفی ۲۲ درجه یا کمتر برسد، کمپرسورها فوراً قطع اضطراری می‌شوند تا شفت و گیربکس آسیب نبینند و تیغه‌های پارو نشکنند.", "Trips compressors instantly if cylinder drops to -22°C to prevent mechanical gearbox damage.", "Silindir sicakligi -22 dereceye duserse mil ve dislilerin kirilmamasi icin kompresorler aninda durdurulur."),
            G_TXT("هشدار: در این حالت تا بالا آمدن دما به منفی ۱۰ درجه، کمپرسورها اجازه روشن شدن ندارند.", "Warning: Compressors remain locked out until temp rises above -10°C.", "Uyari: Sicaklik -10 dereceye yukselene kadar kompresorler calismaz.")};
        cards[2] = (guide_card_item_t){G_TXT("عیب‌یابی", "Troubleshoot", "Ariza Cozum"), 0xea580c, G_TXT("اگر خطای قطعی سنسور سیلندر (کد ۰۵) نمایش داده شد چه کنیم؟", "Fault E05: Probe Disconnect", "E05 Sensor Arizasi"),
            G_TXT("کد خطای ۰۵ نشانه قطعی سیم سنسور سیلندر است. دستگاه به صورت خودکار به مد زمانی اضطراری سوئیچ می‌کند تا بستنی شل نشود. اتصالات سنسور سیلندر را بررسی فرمایید.", "E05 indicates probe disconnect. Emergency timer duty is engaged. Check T2 wiring.", "E05 silindir sensorunun koptugunu gosterir. Sistem acil sogutmaya gecer. T2 baglantisini kontrol edin."),
            NULL};
        card_cnt = 3;
        if (max6675_is_cylinder_valid()) {
            snprintf(status_line, sizeof(status_line), "%s: %.1f°C | %s",
                     G_TXT("دمای لحظه‌ای سیلندر", "Cylinder Temp", "Silindir Sicakligi"), max6675_get_temp_cylinder(),
                     G_TXT("پراب: سالم و فعال", "Probe: OK", "Sensor: Saglam"));
        } else {
            snprintf(status_line, sizeof(status_line), "%s: %s | %s",
                     G_TXT("دمای لحظه‌ای سیلندر", "Cylinder Temp", "Silindir Sicakligi"),
                     G_TXT("سنسور وصل نیست", "Disconnected", "Bagli Degil"),
                     G_TXT("پراب: قطع", "Probe: Fault", "Sensor: Ariza"));
        }
        break;

    case GUIDE_ID_WARRANTY:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای ثبت و فعالسازی گارانتی", "Warranty Registration Guide", "Garanti Kayit ve Aktivasyon Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("راهنمای فعال‌سازی گارانتی رسمی، ثبت اطلاعات مالک و خدمات پس از فروش آستیاج.", "Activating official ASTIAJ warranty and after-sales support.", "Resmi ASTIAJ garantisini baslatma ve servis kaydi."));
        cards[0] = (guide_card_item_t){G_TXT("ثبت فرم", "Form", "Form"), 0x0284c7, G_TXT("نحوه ثبت اطلاعات در سامانه", "Registering Owner Details", "Bilgileri Kaydetme"),
            G_TXT("نام خریدار، شهر محل نصب و شماره همراه معتبر (با ۰۹) را با کیبورد لمسی وارد نموده و کلید سبز تایید و فعالسازی را لمس فرمایید.", "Enter buyer name, installation city, and mobile phone (starting 09), then press Confirm.", "Alici adini, sehri ve telefon numarasini girip Yesil Onay butonuna basin."),
            G_TXT("نکته: شماره همراه مالک برای دریافت پیامک‌های سرویس دوره‌ای و کدهای تمدید ضروری است.", "Note: Mobile phone is required for service notifications.", "Not: Periyodik bakim ve sifre smsleri icin gecerli bir cep telefonu giriniz.")};
        cards[1] = (guide_card_item_t){G_TXT("فعالسازی آنلاین", "Online", "Cevrimici"), 0x16a34a, G_TXT("ارسال مستقیم اطلاعات با وای‌فای", "Direct Cloud Registration", "Bulut Uzerinden Aktivasyon"),
            G_TXT("در صورت اتصال دستگاه به اینترنت، اطلاعات فوراً به سرور ابری آستیاج ارسال شده و گارانتی رسمی ۱ ساله دستگاه از همان لحظه فعال می‌گردد.", "If WiFi has internet, warranty activates instantly on ASTIAJ cloud server.", "Cihaz internete bagli ise bilgiler ASTIAJ bulut sunucusuna iletilir ve 1 yillik garanti aninda baslar."),
            NULL};
        cards[2] = (guide_card_item_t){G_TXT("فعالسازی آفلاین", "Offline", "Cevrimdisi"), 0x7c3aed, G_TXT("فعالسازی بدون دسترسی به اینترنت", "Offline Registration via SMS", "SMS ile Aktivasyon"),
            G_TXT("اگر دستگاه به وای‌فای متصل نیست، پس از فشردن دکمه تایید، کد ۵ رقمی و شماره سریال دستگاه به شما نمایش داده می‌شود که می‌توانید آن را با پیامک به سامانه پشتیبانی آستیاج ارسال فرمایید.", "If offline, send the 5-digit challenge code displayed on screen via SMS to factory support.", "Internet yoksa ekranda cikan 5 haneli kodu ve seri numarasini SMS ile servis hattina iletebilirsiniz."),
            NULL};
        card_cnt = 3;
        break;

    case GUIDE_ID_FAULTS:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای جامع رفع خطاهای دستگاه", "Fault Troubleshooting Guide", "Ariza Giderme Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("راهنمای جامع کدهای خطای صنعتی و راه‌حل‌های فوری رفع مشکل بدون نیاز به تماس.", "Quick-resolution guide for all ISA-18.2 machine fault codes.", "Ariza kodlari ve aninda cozum rehberi."));
        cards[0] = (guide_card_item_t){G_TXT("کد ۰۰", "E00", "E00"), 0xdc2626, G_TXT("کلید توقف اضطراری قارچی", "Emergency Stop Tripped", "Acil Stop Butonu"),
            G_TXT("شستی قارچی قرمز روی پنل فشرده شده است. شستی را در جهت عقربه‌های ساعت بچرخانید تا به سمت بالا بپرد و مدار فرمان وصل شود.", "Red E-Stop button is depressed. Twist clockwise to release.", "Kirmizi acil stop butonu basilmis. Saatin donus yonunde cevirerek yukari birakin."),
            NULL};
        cards[1] = (guide_card_item_t){G_TXT("کد ۰۱", "E01", "E01"), 0xdc2626, G_TXT("توقف گردش پولی پارو", "Pulley Stall Fault", "Kasnak Donus Durmasi"),
            G_TXT("تسمه موتور پارو شل یا پاره شده یا بار بستنی بیش از حد سنگین شده است. همچنین تمیزی و فاصله ۲ میلی‌متری سنسور القایی با پولی را چک کنید.", "Beater belt loose/broken or heavy load. Inspect 2mm gap of inductive sensor.", "Palet kayisi kopmus/gevsemis veya yuk cok agir. Sensorun 2 mm mesafesini kontrol edin."),
            G_TXT("نکته: پس از رفع مشکل، دستگاه را یک‌بار خاموش و روشن کنید.", "Note: Cycle power after resolving belt issue.", "Not: Sorunu giderdikten sonra cihazi kapatip acin.")};
        cards[2] = (guide_card_item_t){G_TXT("کد ۰۲", "E02", "E02"), 0xea580c, G_TXT("اضافه‌بار موتور پارو", "Motor Overload Tripped", "Karistirici Motor Asiri Yuku"),
            G_TXT("رله کنترل‌بار موتور پارو در تابلو برق تریپ کرده است. بستنی بیش از حد سفت شده یا موتور داغ کرده است. دکمه آبی ریست کنترل‌بار را فشار دهید.", "Motor overload relay tripped. Press blue reset button on overload relay in electrical cabinet.", "Termik role atti. Dondurma cok sert veya motor isinmis. Panodaki mavi reset butonuna basin."),
            NULL};
        cards[3] = (guide_card_item_t){G_TXT("کد ۰۳", "E03", "E03"), 0xea580c, G_TXT("اضافه‌بار کمپرسور برودت", "Compressor Overload Tripped", "Kompresor Asiri Yuku"),
            G_TXT("رله کنترل‌بار کمپرسور تریپ کرده است. کندانسور پشت دستگاه کثیف است یا فن خنک‌کننده کار نمی‌کند یا ولتاژ برق مغازه افت کرده است.", "Clean condenser fins, check cooling fan, and verify mains voltage.", "Kompresor termigi atti. Kondenser peteklerini temizleyin, fani ve sebeke voltajini kontrol edin."),
            G_TXT("نکته: استفاده از استابلایزر ولتاژ در بستنی‌فروشی اکیداً توصیه می‌شود.", "Note: Voltage stabilizer is strongly recommended.", "Not: Voltaj regulatoru kullanilmasi onemle tavsiye edilir.")};
        cards[4] = (guide_card_item_t){G_TXT("کد ۰۴/۰۵", "E04/E05", "E04/E05"), 0x0284c7, G_TXT("قطعی سنسور دمای مخزن یا سیلندر", "Thermocouple Disconnected", "Sicaklik Sensoru Baglanti Kopuklugu"),
            G_TXT("سیم یا سوکت پراب دما قطع است. دستگاه موقتاً طبق تایمر اضطراری به خنک‌کاری ادامه می‌دهد. فیش پراب را بررسی و محکم نمایید.", "Probe wiring loose. Emergency fallback cooling is active. Reseat probe connector.", "Sensor fisini ve kablosunu kontrol edip yerine sikica takin."),
            NULL};
        card_cnt = 5;
        break;

    default:
        snprintf(title, sizeof(title), "%s", G_TXT("راهنمای سیستم صنعتی", "System Guide", "Sistem Kilavuzu"));
        snprintf(objective, sizeof(objective), "%s", G_TXT("راهنمای کاربری و پارامترهای صنعتی کنترلر آستیاج.", "ASTIAJ Industrial Controller operator guide.", "ASTIAJ Endustriyel Kontrolor kullanim rehberi."));
        card_cnt = 0;
        break;
    }

#undef G_TXT

    if (s_guide_title_lbl && lv_obj_is_valid(s_guide_title_lbl)) {
        lv_label_set_text(s_guide_title_lbl, title);
    }

    populate_rich_guide(s_guide_scroll, objective, cards, card_cnt, status_line[0] ? status_line : NULL);
}

void ui_show_tooltip(const char *title, const char *desc)
{
    ui_show_smart_guide(GUIDE_ID_SETTINGS);
    if (s_guide_title_lbl && lv_obj_is_valid(s_guide_title_lbl) && title) {
        lv_label_set_text(s_guide_title_lbl, title);
    }
    if (s_guide_scroll && lv_obj_is_valid(s_guide_scroll) && desc) {
        const char *st[1] = {desc};
        populate_structured_guide(s_guide_scroll, title, st, 1, NULL);
    }
}

static void tooltip_click_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    intptr_t ud = (intptr_t)lv_event_get_user_data(e);
    if (ud >= 1 && ud <= 20) {
        ui_show_smart_guide((guide_id_t)ud);
        return;
    }
}

static void tooltip_long_press_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) return;
    s_last_guide_long_press_tick = lv_tick_get();

    /* مهار قطعی تاچ جهت جلوگیری از ارسال رویداد کلیک ناخواسته پس از رها کردن انگشت */
    lv_indev_t *indev = lv_indev_active();
    if (indev) {
        lv_indev_wait_release(indev);
    }

    intptr_t ud = (intptr_t)lv_event_get_user_data(e);
    if (ud >= 1 && ud <= 20) {
        ui_show_smart_guide((guide_id_t)ud);
        return;
    }
    tooltip_data_t *d = (tooltip_data_t *)ud;
    if (d && d->title) {
        ui_show_tooltip(d->title, d->desc);
    }
}

/* =========================================================================
 * پنجره‌های هوشمند حفاظت حرارتی و قفل اورلود (Machine Advisor Overload UI)
 * ========================================================================= */
static lv_obj_t *s_overload_modal_backdrop = NULL;
static lv_obj_t *s_ovl_pin_ta = NULL;

void ui_hide_overload_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    if (s_overload_modal_backdrop && lv_obj_is_valid(s_overload_modal_backdrop)) {
        lv_obj_delete(s_overload_modal_backdrop);
    }
    s_overload_modal_backdrop = NULL;
    s_ovl_pin_ta = NULL;
    lvgl_unlock();
}

static void overload_dialog_close_cb(lv_event_t *e)
{
    (void)e;
    ui_hide_overload_dialog();
}

void ui_show_overload_cooldown_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    ui_hide_overload_dialog();
    buzzer_beep_alarm();

    s_overload_modal_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_overload_modal_backdrop, 0, 0);
    lv_obj_set_size(s_overload_modal_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_overload_modal_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overload_modal_backdrop, 190, 0);
    lv_obj_set_style_border_width(s_overload_modal_backdrop, 0, 0);
    lv_obj_clear_flag(s_overload_modal_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_overload_modal_backdrop);
    lv_obj_set_size(card, 430, 270);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xea580c), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_width(hdr, lv_pct(100));
    lv_obj_set_height(hdr, 42);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xea580c), 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_lbl = lv_label_create(hdr);
    lv_label_set_text(title_lbl, "محافظت حرارتی سیم‌پیچ (اضافه‌بار ۲)");
    lv_obj_set_style_text_font(title_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title_lbl, LV_BASE_DIR_RTL, 0);
    lv_obj_center(title_lbl);

    /* Body scroll */
    lv_obj_t *scroll = lv_obj_create(card);
    lv_obj_set_pos(scroll, 10, 48);
    lv_obj_set_size(scroll, 410, 165);
    lv_obj_set_style_bg_color(scroll, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_opa(scroll, 255, 0);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_clear_flag(scroll, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_anim_duration(scroll, 0, 0);

    uint32_t rem = machine_advisor_get_cooldown_remain_s();
    uint32_t min = rem / 60;
    uint32_t sec = rem % 60;

    char msg[1024];
    snprintf(msg, sizeof(msg),
        "هشدار حفاظتی زیمنس:\n"
        "دستگاه برای دومین مرتبه دچار تریپ کنترل‌بار اضافه جریان شده است.\n\n"
        "به جهت خنک‌سازی کامل سیم‌پیچ الکتروموتور و کمپرسور و ممانعت از سوختگی، سیستم به مدت ۳۰ دقیقه قفل حرارتی شده است.\n\n"
        "زمان خنک‌سازی باقی‌مانده: %lu دقیقه و %lu ثانیه\n\n"
        "توجه: خاموش و روشن کردن دستگاه این تایمر را دور نمی‌زند و مجدداً از ۳۰ دقیقه آغاز خواهد شد.",
        (unsigned long)min, (unsigned long)sec);

    lv_obj_t *lbl = lv_label_create(scroll);
    lv_obj_set_width(lbl, 390);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, msg);
    lv_obj_set_style_text_font(lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_line_space(lbl, 5, 0);

    /* Close button */
    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_pos(btn_ok, 145, 222);
    lv_obj_set_size(btn_ok, 140, 36);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_ok, 0, 0);
    lv_obj_add_event_cb(btn_ok, overload_dialog_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn_ok);
    lv_label_set_text(btn_lbl, "متوجه شدم");
    lv_obj_set_style_text_font(btn_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(btn_lbl);
    lvgl_unlock();
}

static void overload_pin_btn_cb(lv_event_t *e)
{
    const char *digit = (const char *)lv_event_get_user_data(e);
    if (!digit || !s_ovl_pin_ta) return;
    buzzer_beep_click();
    if (strcmp(digit, "DEL") == 0) {
        lv_textarea_delete_char(s_ovl_pin_ta);
    } else if (strcmp(digit, "CLR") == 0) {
        lv_textarea_set_text(s_ovl_pin_ta, "");
    } else {
        lv_textarea_add_text(s_ovl_pin_ta, digit);
    }
}

static void overload_pin_submit_cb(lv_event_t *e)
{
    (void)e;
    if (!s_ovl_pin_ta) return;
    const char *code = lv_textarea_get_text(s_ovl_pin_ta);
    if (machine_advisor_verify_unlock_code(code)) {
        buzzer_beep_confirm();
        ui_hide_overload_dialog();
        panel_nav_toast_success("قفل اورلود با موفقیت بازگشایی شد");
    } else {
        buzzer_beep_alarm();
        panel_nav_toast_error("کد بازگشایی نامعتبر است!");
        lv_textarea_set_text(s_ovl_pin_ta, "");
    }
}

void ui_show_overload_hard_lock_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;
    ui_hide_overload_dialog();
    buzzer_beep_alarm();

    s_overload_modal_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_overload_modal_backdrop, 0, 0);
    lv_obj_set_size(s_overload_modal_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_overload_modal_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overload_modal_backdrop, 200, 0);
    lv_obj_set_style_border_width(s_overload_modal_backdrop, 0, 0);
    lv_obj_clear_flag(s_overload_modal_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_overload_modal_backdrop);
    lv_obj_set_size(card, 440, 290);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_width(hdr, lv_pct(100));
    lv_obj_set_height(hdr, 42);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_lbl = lv_label_create(hdr);
    lv_label_set_text(title_lbl, "قفل بحرانی اورلود (۳ تریپ در ۲ ساعت)");
    lv_obj_set_style_text_font(title_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title_lbl, LV_BASE_DIR_RTL, 0);
    lv_obj_center(title_lbl);

    lv_obj_t *btn_close = lv_btn_create(hdr);
    lv_obj_set_pos(btn_close, 8, 5);
    lv_obj_set_size(btn_close, 32, 32);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0xb91c1c), 0);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_set_style_border_width(btn_close, 0, 0);
    lv_obj_set_style_pad_all(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, overload_dialog_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_close = lv_label_create(btn_close);
    lv_label_set_text(l_close, LV_SYMBOL_CLOSE);
    lv_obj_center(l_close);

    uint32_t chal = machine_advisor_get_overload_challenge();
    char msg[512];
    snprintf(msg, sizeof(msg),
        "هشدار: ۳ مرتبه تریپ اورلود در بازه ۲ ساعت رخ داده است!\n"
        "جهت جلوگیری از آسیب به کمپرسور/موتور، سیستم متوقف شد.\n"
        "کد چالش: %lu | پشتیبانی آستیاج: ۰۹۱۲۷۷۳۷۰۴۲",
        (unsigned long)chal);

    lv_obj_t *info_lbl = lv_label_create(card);
    lv_obj_set_pos(info_lbl, 12, 48);
    lv_obj_set_width(info_lbl, 416);
    lv_label_set_long_mode(info_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(info_lbl, msg);
    lv_obj_set_style_text_font(info_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(info_lbl, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_base_dir(info_lbl, LV_BASE_DIR_RTL, 0);

    /* Text Area */
    s_ovl_pin_ta = lv_textarea_create(card);
    lv_obj_set_pos(s_ovl_pin_ta, 120, 118);
    lv_obj_set_size(s_ovl_pin_ta, 200, 36);
    lv_textarea_set_password_mode(s_ovl_pin_ta, false);
    lv_textarea_set_one_line(s_ovl_pin_ta, true);
    lv_textarea_set_max_length(s_ovl_pin_ta, 10);
    lv_textarea_set_placeholder_text(s_ovl_pin_ta, "کد بازگشایی...");
    lv_obj_set_style_text_font(s_ovl_pin_ta, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_font(s_ovl_pin_ta, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_align(s_ovl_pin_ta, LV_TEXT_ALIGN_CENTER, 0);

    /* Compact Keypad row 1: 1 2 3 4 5 */
    const char *row1[] = {"1", "2", "3", "4", "5"};
    for (int i = 0; i < 5; i++) {
        lv_obj_t *b = lv_btn_create(card);
        lv_obj_set_pos(b, 20 + (i * 80), 162);
        lv_obj_set_size(b, 72, 34);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x334155), 0);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_add_event_cb(b, overload_pin_btn_cb, LV_EVENT_CLICKED, (void *)row1[i]);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, row1[i]);
        lv_obj_center(l);
    }

    /* Compact Keypad row 2: 6 7 8 9 0 */
    const char *row2[] = {"6", "7", "8", "9", "0"};
    for (int i = 0; i < 5; i++) {
        lv_obj_t *b = lv_btn_create(card);
        lv_obj_set_pos(b, 20 + (i * 80), 202);
        lv_obj_set_size(b, 72, 34);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x334155), 0);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_add_event_cb(b, overload_pin_btn_cb, LV_EVENT_CLICKED, (void *)row2[i]);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, row2[i]);
        lv_obj_center(l);
    }

    /* Row 3: CLR, Submit, DEL */
    lv_obj_t *btn_clr = lv_btn_create(card);
    lv_obj_set_pos(btn_clr, 20, 242);
    lv_obj_set_size(btn_clr, 90, 36);
    lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_radius(btn_clr, 6, 0);
    lv_obj_add_event_cb(btn_clr, overload_pin_btn_cb, LV_EVENT_CLICKED, (void *)"CLR");
    lv_obj_t *l_clr = lv_label_create(btn_clr);
    lv_label_set_text(l_clr, "پاک");
    lv_obj_set_style_text_font(l_clr, &ui_font_yekan_16, 0);
    lv_obj_center(l_clr);

    lv_obj_t *btn_sub = lv_btn_create(card);
    lv_obj_set_pos(btn_sub, 120, 242);
    lv_obj_set_size(btn_sub, 200, 36);
    lv_obj_set_style_bg_color(btn_sub, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_radius(btn_sub, 6, 0);
    lv_obj_add_event_cb(btn_sub, overload_pin_submit_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_sub = lv_label_create(btn_sub);
    lv_label_set_text(l_sub, "تایید و بازگشایی");
    lv_obj_set_style_text_font(l_sub, &ui_font_yekan_16, 0);
    lv_obj_center(l_sub);

    lv_obj_t *btn_del = lv_btn_create(card);
    lv_obj_set_pos(btn_del, 330, 242);
    lv_obj_set_size(btn_del, 90, 36);
    lv_obj_set_style_bg_color(btn_del, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_radius(btn_del, 6, 0);
    lv_obj_add_event_cb(btn_del, overload_pin_btn_cb, LV_EVENT_CLICKED, (void *)"DEL");
    lv_obj_t *l_del = lv_label_create(btn_del);
    lv_label_set_text(l_del, "حذف");
    lv_obj_set_style_text_font(l_del, &ui_font_yekan_16, 0);
    lv_obj_center(l_del);

    lvgl_unlock();
}

/* =========================================================================
 * حالت استندبای و محافظ صفحه هوشمند (Apple Standby Mode)
 * ========================================================================= */
static lv_obj_t *s_screensaver_layer = NULL;
static lv_obj_t *s_ss_time_lbl = NULL;
static lv_obj_t *s_ss_date_lbl = NULL;
static lv_obj_t *s_ss_status_lbl = NULL;
static lv_obj_t *s_ss_status_led = NULL;
static uint32_t s_inactivity_sec = 0;

bool screensaver_is_active(void)
{
    return (s_screensaver_layer != NULL && lv_obj_is_valid(s_screensaver_layer));
}

void screensaver_notify_activity(void)
{
    s_inactivity_sec = 0;
}

void hide_screensaver(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
    if (s_screensaver_layer && lv_obj_is_valid(s_screensaver_layer)) {
        lv_obj_delete(s_screensaver_layer);
    }
    s_screensaver_layer = NULL;
    s_ss_time_lbl = NULL;
    s_ss_date_lbl = NULL;
    s_ss_status_lbl = NULL;
    s_ss_status_led = NULL;
    s_inactivity_sec = 0;
    lvgl_unlock();
}

void show_screensaver(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
    if (s_screensaver_layer && lv_obj_is_valid(s_screensaver_layer)) {
        lvgl_unlock();
        return;
    }

    s_screensaver_layer = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_screensaver_layer, 0, 0);
    lv_obj_set_size(s_screensaver_layer, 480, 320);
    lv_obj_set_style_bg_color(s_screensaver_layer, lv_color_hex(0x020617), 0);
    lv_obj_set_style_bg_opa(s_screensaver_layer, 255, 0);
    lv_obj_set_style_border_width(s_screensaver_layer, 0, 0);
    lv_obj_set_style_pad_all(s_screensaver_layer, 0, 0);
    lv_obj_clear_flag(s_screensaver_layer, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Header: 60% Larger HD Mountain Emblem + Brand Typography */
    lv_obj_t *logo = lv_image_create(s_screensaver_layer);
    lv_image_set_src(logo, &img_screensaver_logo);
    lv_image_set_scale(logo, 256);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *brand = lv_label_create(s_screensaver_layer);
    lv_label_set_text(brand, "ASTIAJ INDUSTRIAL MACHINERY");
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(brand, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_letter_space(brand, 2, 0);
    lv_obj_align(brand, LV_ALIGN_TOP_MID, 0, 64);

    /* 2. Apple Standby Glass Card */
    lv_obj_t *card = lv_obj_create(s_screensaver_layer);
    lv_obj_set_pos(card, 24, 84);
    lv_obj_set_size(card, 432, 194);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(card, 250, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Big Digital Clock */
    s_ss_time_lbl = lv_label_create(card);
    lv_label_set_text(s_ss_time_lbl, "00:00");
    lv_obj_set_style_text_font(s_ss_time_lbl, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_ss_time_lbl, lv_color_hex(0xf8fafc), 0);
    lv_obj_align(s_ss_time_lbl, LV_ALIGN_TOP_MID, 0, 8);

    /* Date label - Left to Right (Jalali for FA, Gregorian for EN/TR) */
    s_ss_date_lbl = lv_label_create(card);
    lv_label_set_text(s_ss_date_lbl, "---");
    lv_obj_set_style_text_font(s_ss_date_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(s_ss_date_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_base_dir(s_ss_date_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_align(s_ss_date_lbl, LV_ALIGN_TOP_MID, 0, 74);

    /* Left Ice Cream Icon (Luxury HD Soft-Serve Cone) */
    lv_obj_t *ice_l = lv_image_create(card);
    lv_image_set_src(ice_l, &img_icecream_left);
    lv_obj_align(ice_l, LV_ALIGN_LEFT_MID, 12, -12);

    /* Right Ice Cream Icon (Symmetrically Mirrored Luxury HD Soft-Serve Cone) */
    lv_obj_t *ice_r = lv_image_create(card);
    lv_image_set_src(ice_r, &img_icecream_right);
    lv_obj_align(ice_r, LV_ALIGN_RIGHT_MID, -12, -12);

    /* Status Pill: Centered Flex Row with LED dot & Persian text */
    lv_obj_t *pill = lv_obj_create(card);
    lv_obj_set_size(pill, 270, 36);
    lv_obj_align(pill, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_bg_color(pill, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_opa(pill, 255, 0);
    lv_obj_set_style_radius(pill, 18, 0);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_border_color(pill, lv_color_hex(0x334155), 0);
    lv_obj_set_style_pad_all(pill, 0, 0);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(pill, 10, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    /* LED Circle */
    s_ss_status_led = lv_obj_create(pill);
    lv_obj_set_size(s_ss_status_led, 10, 10);
    lv_obj_set_style_radius(s_ss_status_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ss_status_led, lv_color_hex(0x22c55e), 0);
    lv_obj_set_style_bg_opa(s_ss_status_led, 255, 0);
    lv_obj_set_style_border_width(s_ss_status_led, 0, 0);
    lv_obj_clear_flag(s_ss_status_led, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* Status Label */
    s_ss_status_lbl = lv_label_create(pill);
    lv_label_set_text(s_ss_status_lbl, ui_lang_str(STR_SS_STANDBY));
    lv_obj_set_style_text_font(s_ss_status_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(s_ss_status_lbl, lv_color_hex(0x22c55e), 0);
    lv_obj_set_style_base_dir(s_ss_status_lbl, ui_lang_base_dir(), 0);

    /* 3. Bottom Wake Hint */
    lv_obj_t *hint = lv_label_create(s_screensaver_layer);
    lv_label_set_text(hint, ui_lang_str(STR_SS_WAKE_HINT));
    lv_obj_set_style_text_font(hint, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(hint, ui_lang_base_dir(), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);

    screensaver_tick_1s();
    lvgl_unlock();
}

void screensaver_tick_1s(void)
{
    if (screensaver_is_active()) {
        if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
        if (s_ss_time_lbl && lv_obj_is_valid(s_ss_time_lbl)) {
            static bool blink = false;
            blink = !blink;
            lv_label_set_text_fmt(s_ss_time_lbl, "%02d%c%02d", jt.hour, blink ? ':' : ' ', jt.minute);
        }
        if (s_ss_date_lbl && lv_obj_is_valid(s_ss_date_lbl)) {
            static const ui_str_key_t s_wday_keys[7] = {
                STR_WDAY_SUN, STR_WDAY_MON, STR_WDAY_TUE, STR_WDAY_WED,
                STR_WDAY_THU, STR_WDAY_FRI, STR_WDAY_SAT
            };
            int wday_idx = (jt.wday >= 1 && jt.wday <= 7) ? (jt.wday - 1) : 0;
            bool is_fa = (ui_lang_get() == UI_LANG_FA);
            if (is_fa) {
                lv_label_set_text_fmt(s_ss_date_lbl, "%s  %04d/%02d/%02d",
                                      ui_lang_str(s_wday_keys[wday_idx]), jt.jy, jt.jm, jt.jd);
            } else {
                int gy = 0, gm = 0, gd = 0;
                jalali_to_gregorian((jt.jy > 0) ? jt.jy : 1404,
                                    (jt.jm > 0) ? jt.jm : 1,
                                    (jt.jd > 0) ? jt.jd : 1,
                                    &gy, &gm, &gd);
                lv_label_set_text_fmt(s_ss_date_lbl, "%s  %04d/%02d/%02d",
                                      ui_lang_str(s_wday_keys[wday_idx]), gy, gm, gd);
            }
            /* فقط در صورت تغییر واقعی زبان، فونت و جهت را به‌روز کن */
            static ui_lang_t s_ss_last_lang = (ui_lang_t)-1;
            if (ui_lang_get() != s_ss_last_lang) {
                s_ss_last_lang = ui_lang_get();
                lv_obj_set_style_text_font(s_ss_date_lbl, ui_lang_font_16(), 0);
                lv_obj_set_style_base_dir(s_ss_date_lbl, LV_BASE_DIR_LTR, 0);
                if (s_ss_status_lbl && lv_obj_is_valid(s_ss_status_lbl)) {
                    lv_obj_set_style_text_font(s_ss_status_lbl, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ss_status_lbl, ui_lang_base_dir(), 0);
                }
            }
        }
        if (s_ss_status_lbl && lv_obj_is_valid(s_ss_status_lbl)) {
            uint32_t col = 0x22c55e;
            const char *st_txt = ui_lang_str(STR_SS_STANDBY);
            if (plc_is_ready()) {
                st_txt = ui_lang_str(STR_SS_READY_DISPENSE);
                col = 0x22c55e;
            } else if (plc_is_dispense()) {
                st_txt = ui_lang_str(STR_SS_DISPENSING);
                col = 0x06b6d4;
            } else if (make_start) {
                st_txt = ui_lang_str(STR_SS_PROD_CYCLE);
                col = 0x3b82f6;
            } else if (paddle_start) {
                st_txt = ui_lang_str(STR_SS_PADDLE_ACTIVE);
                col = 0xf59e0b;
            } else {
                st_txt = ui_lang_str(STR_SS_STANDBY);
                col = 0x94a3b8;
            }
            lv_label_set_text(s_ss_status_lbl, st_txt);
            if (s_ss_status_led && lv_obj_is_valid(s_ss_status_led)) {
                lv_obj_set_style_bg_color(s_ss_status_led, lv_color_hex(col), 0);
            }
            lv_obj_set_style_text_color(s_ss_status_lbl, lv_color_hex(col), 0);
        }
        lvgl_unlock();
    } else {
        if (screensaver_timeout_s > 0) {
            s_inactivity_sec++;
            if (s_inactivity_sec >= (uint32_t)screensaver_timeout_s) {
                show_screensaver();
            }
        }
    }
}

/* =========================================================================
 * دیالوگ آمار تفصیلی و شیفت تولید (با لمس کادر شمارنده باز می‌شود)
 * ========================================================================= */
static lv_obj_t *s_counter_dialog_layer = NULL;

void hide_counter_details_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
    if (s_counter_dialog_layer && lv_obj_is_valid(s_counter_dialog_layer)) {
        lv_obj_delete(s_counter_dialog_layer);
    }
    s_counter_dialog_layer = NULL;
    lvgl_unlock();
}

static void action_reset_daily_stats(lv_event_t *e)
{
    (void)e;
    if (warranty_is_expired_and_locked()) {
        open_warranty_expired_dialog();
        buzzer_beep_alarm();
        return;
    }
    icecream_counter_daily = 0;
    icecream_counter = 0;
    nvs_request_lazy_flush();
    ui_update_icecream_counter();
    buzzer_beep_confirm();
    hide_counter_details_dialog();
    show_counter_details_dialog();
}

static void action_toggle_drain_dialog(lv_event_t *e)
{
    (void)e;
    if (warranty_is_expired_and_locked()) {
        open_warranty_expired_dialog();
        buzzer_beep_alarm();
        return;
    }
    if (drain_mode_is_active()) {
        drain_mode_stop();
    } else {
        drain_mode_start();
    }
    hide_counter_details_dialog();
}

void show_counter_details_dialog(void)
{
    hide_counter_details_dialog();

    s_counter_dialog_layer = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_counter_dialog_layer, 0, 0);
    lv_obj_set_size(s_counter_dialog_layer, 480, 320);
    lv_obj_set_style_bg_color(s_counter_dialog_layer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_counter_dialog_layer, 180, 0);
    lv_obj_set_style_border_width(s_counter_dialog_layer, 0, 0);
    lv_obj_set_style_pad_all(s_counter_dialog_layer, 0, 0);
    lv_obj_clear_flag(s_counter_dialog_layer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_counter_dialog_layer);
    lv_obj_set_pos(card, 30, 16);
    lv_obj_set_size(card, 420, 288);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 420, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_radius(header, 14, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, ui_lang_str(STR_CNT_STATS_TITLE));
    lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_center(title);

    int64_t daily_rev = (int64_t)icecream_counter_daily * cone_price;
    int64_t total_rev = (int64_t)icecream_counter_total * cone_price;

    /* Card 1: Today Stats & Revenue */
    lv_obj_t *c1 = lv_obj_create(card);
    lv_obj_set_pos(c1, 16, 48);
    lv_obj_set_size(c1, 188, 142);
    lv_obj_set_style_bg_color(c1, lv_color_hex(0xf0fdf4), 0);
    lv_obj_set_style_border_color(c1, lv_color_hex(0x86efac), 0);
    lv_obj_set_style_radius(c1, 10, 0);
    lv_obj_set_style_pad_all(c1, 0, 0);
    lv_obj_clear_flag(c1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *l1_title = lv_label_create(c1);
    lv_label_set_text(l1_title, ui_lang_str(STR_CNT_SHIFT_CONE));
    lv_obj_set_style_text_font(l1_title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l1_title, lv_color_hex(0x166534), 0);
    lv_obj_set_style_base_dir(l1_title, ui_lang_base_dir(), 0);
    lv_obj_align(l1_title, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *l1_val = lv_label_create(c1);
    lv_label_set_text_fmt(l1_val, "%ld", (long)icecream_counter_daily);
    lv_obj_set_style_text_font(l1_val, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(l1_val, lv_color_hex(0x15803d), 0);
    lv_obj_align(l1_val, LV_ALIGN_CENTER, 0, -14);

    lv_obj_t *l1_rev = lv_label_create(c1);
    lv_label_set_text_fmt(l1_rev, "%s %lld", ui_lang_str(STR_CNT_SHIFT_REV), (long long)daily_rev);
    lv_obj_set_style_text_font(l1_rev, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l1_rev, lv_color_hex(0x166534), 0);
    lv_obj_set_style_base_dir(l1_rev, ui_lang_base_dir(), 0);
    lv_obj_align(l1_rev, LV_ALIGN_CENTER, 0, 14);

    lv_obj_t *btn_rst_daily = lv_btn_create(c1);
    lv_obj_set_size(btn_rst_daily, 160, 30);
    lv_obj_align(btn_rst_daily, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(btn_rst_daily, lv_color_hex(0xdcfce7), 0);
    lv_obj_set_style_border_color(btn_rst_daily, lv_color_hex(0x22c55e), 0);
    lv_obj_set_style_border_width(btn_rst_daily, 1, 0);
    lv_obj_set_style_radius(btn_rst_daily, 6, 0);
    lv_obj_set_style_shadow_width(btn_rst_daily, 0, 0);
    lv_obj_add_event_cb(btn_rst_daily, action_reset_daily_stats, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_rst_lbl = lv_label_create(btn_rst_daily);
    lv_label_set_text(btn_rst_lbl, ui_lang_str(STR_CNT_SHIFT_RST));
    lv_obj_set_style_text_font(btn_rst_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(btn_rst_lbl, lv_color_hex(0x166534), 0);
    lv_obj_set_style_base_dir(btn_rst_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(btn_rst_lbl);

    /* Card 2: Total Odometer & Total Sales */
    lv_obj_t *c2 = lv_obj_create(card);
    lv_obj_set_pos(c2, 216, 48);
    lv_obj_set_size(c2, 188, 142);
    lv_obj_set_style_bg_color(c2, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_color(c2, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_radius(c2, 10, 0);
    lv_obj_set_style_pad_all(c2, 0, 0);
    lv_obj_clear_flag(c2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *l2_title = lv_label_create(c2);
    lv_label_set_text(l2_title, ui_lang_str(STR_CNT_TOTAL_CONE));
    lv_obj_set_style_text_font(l2_title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l2_title, lv_color_hex(0x334155), 0);
    lv_obj_set_style_base_dir(l2_title, ui_lang_base_dir(), 0);
    lv_obj_align(l2_title, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *l2_val = lv_label_create(c2);
    lv_label_set_text_fmt(l2_val, "%ld", (long)icecream_counter_total);
    lv_obj_set_style_text_font(l2_val, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(l2_val, lv_color_hex(0x0f172a), 0);
    lv_obj_align(l2_val, LV_ALIGN_CENTER, 0, -14);

    lv_obj_t *l2_rev = lv_label_create(c2);
    lv_label_set_text_fmt(l2_rev, "%s %lld", ui_lang_str(STR_CNT_TOTAL_REV), (long long)total_rev);
    lv_obj_set_style_text_font(l2_rev, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l2_rev, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(l2_rev, ui_lang_base_dir(), 0);
    lv_obj_align(l2_rev, LV_ALIGN_CENTER, 0, 14);

    lv_obj_t *l2_sub = lv_label_create(c2);
    lv_label_set_text(l2_sub, ui_lang_str(STR_CNT_ODOMETER));
    lv_obj_set_style_text_font(l2_sub, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l2_sub, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(l2_sub, ui_lang_base_dir(), 0);
    lv_obj_align(l2_sub, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* Unit price footnote */
    lv_obj_t *l_price = lv_label_create(card);
    lv_label_set_text_fmt(l_price, "%s: %ld", ui_lang_str(STR_CNT_PRICE_NOTE), (long)cone_price);
    lv_obj_set_style_text_font(l_price, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l_price, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_base_dir(l_price, ui_lang_base_dir(), 0);
    lv_obj_align(l_price, LV_ALIGN_TOP_MID, 0, 196);

    /* Bottom Action Buttons: Drain Mode + Close */
    lv_obj_t *btn_drain = lv_btn_create(card);
    lv_obj_set_pos(btn_drain, 16, 226);
    lv_obj_set_size(btn_drain, 188, 44);
    if (drain_mode_is_active()) {
        lv_obj_set_style_bg_color(btn_drain, lv_color_hex(0xef4444), 0);
    } else {
        lv_obj_set_style_bg_color(btn_drain, lv_color_hex(0x0284c7), 0);
    }
    lv_obj_set_style_radius(btn_drain, 8, 0);
    lv_obj_add_event_cb(btn_drain, action_toggle_drain_dialog, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_drain_lbl = lv_label_create(btn_drain);
    if (drain_mode_is_active()) {
        lv_label_set_text(btn_drain_lbl, ui_lang_str(STR_CNT_DRAIN_STOP));
    } else {
        lv_label_set_text(btn_drain_lbl, ui_lang_str(STR_CNT_DRAIN_START));
    }
    lv_obj_set_style_text_font(btn_drain_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(btn_drain_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(btn_drain_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(btn_drain_lbl);

    lv_obj_t *btn_close = lv_btn_create(card);
    lv_obj_set_pos(btn_close, 216, 226);
    lv_obj_set_size(btn_close, 188, 44);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_add_event_cb(btn_close, (lv_event_cb_t)hide_counter_details_dialog, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_close_lbl = lv_label_create(btn_close);
    lv_label_set_text(btn_close_lbl, ui_lang_str(STR_CLOSE_DIALOG));
    lv_obj_set_style_text_font(btn_close_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(btn_close_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(btn_close_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(btn_close_lbl);
}

void action_toggle_ss_timeout(lv_event_t *e)
{
    (void)e;
    if (warranty_is_expired_and_locked()) {
        open_warranty_expired_dialog();
        buzzer_beep_alarm();
        return;
    }
    buzz(80);
    if (screensaver_timeout_s == 0) {
        screensaver_timeout_s = 900;
    } else if (screensaver_timeout_s == 900) {
        screensaver_timeout_s = 1800;
    } else if (screensaver_timeout_s == 1800) {
        screensaver_timeout_s = 3600;
    } else if (screensaver_timeout_s == 3600) {
        screensaver_timeout_s = 7200;
    } else {
        screensaver_timeout_s = 0;
    }

    update_ss_timeout_ui();
    ui_mark_settings_modified();
}

void action_toggle_pulley_en(lv_event_t *e)
{
    (void)e;
    if (warranty_is_expired_and_locked()) {
        open_warranty_expired_dialog();
        buzzer_beep_alarm();
        return;
    }
    buzz(80);
    s_pulley_en = !s_pulley_en;
    storage_set_bool(KEY_PULLEY_EN, s_pulley_en);
    nvs_request_lazy_flush();
    if (!s_pulley_en) {
        plc_try_clear_fault();
    }
    update_pulley_en_ui();
    ui_mark_settings_modified();
}

/* =========================================================================
 * دیالوگ تایید خروج در صورت وجود تغییرات ذخیره‌نشده (Apple HIG Alert Sheet)
 * ========================================================================= */
static lv_obj_t *s_unsaved_dialog_layer = NULL;

void hide_unsaved_changes_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(50))) return;
    if (s_unsaved_dialog_layer && lv_obj_is_valid(s_unsaved_dialog_layer)) {
        lv_obj_delete(s_unsaved_dialog_layer);
    }
    s_unsaved_dialog_layer = NULL;
    lvgl_unlock();
}

void show_unsaved_changes_dialog(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(100))) return;

    if (s_unsaved_dialog_layer && lv_obj_is_valid(s_unsaved_dialog_layer)) {
        lv_obj_delete(s_unsaved_dialog_layer);
        s_unsaved_dialog_layer = NULL;
    }

    setting_diff_item_t diffs[32];
    int count = ui_get_settings_diff_list(diffs, 32);
    if (count <= 0) {
        hide_unsaved_changes_dialog();
        io_test_turn_off_all_relays();
        loadScreen(SCREEN_ID_MAIN_PG);
        lvgl_unlock();
        return;
    }
    ui_lang_t lang = ui_lang_get();

    s_unsaved_dialog_layer = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_unsaved_dialog_layer, 0, 0);
    lv_obj_set_size(s_unsaved_dialog_layer, 480, 320);
    lv_obj_set_style_bg_color(s_unsaved_dialog_layer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_unsaved_dialog_layer, 180, 0);
    lv_obj_set_style_border_width(s_unsaved_dialog_layer, 0, 0);
    lv_obj_set_style_pad_all(s_unsaved_dialog_layer, 0, 0);
    lv_obj_clear_flag(s_unsaved_dialog_layer, LV_OBJ_FLAG_SCROLLABLE);

    /* ابعاد پویا و متناسب بر اساس تعداد تغییرات */
    int row_h = 42;
    int list_h = (count <= 3) ? (count * row_h + 8) : 156;
    int card_h = 40 + 26 + list_h + 10 + 42 + 14;
    if (card_h > 304) card_h = 304;
    int card_y = (320 - card_h) / 2;
    int btn_y = card_h - 52;

    /* کادر سفید اصلی به سبک اپل */
    lv_obj_t *card = lv_obj_create(s_unsaved_dialog_layer);
    lv_obj_set_pos(card, 15, card_y);
    lv_obj_set_size(card, 450, card_h);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* نوار تیتر تیره با نماد هشدار */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 450, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    char title_buf[80];
    snprintf(title_buf, sizeof(title_buf),
             (lang == UI_LANG_FA) ? "تغییرات ذخیره‌نشده (%d مورد)" :
             ((lang == UI_LANG_TR) ? "Kaydedilmemis Degisiklikler (%d)" : "Unsaved Changes (%d items)"),
             count);
    lv_label_set_text(title, title_buf);
    lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_center(title);

    /* پیام راهنما با تایپوگرافی تمیز */
    lv_obj_t *hint_lbl = lv_label_create(card);
    lv_obj_set_pos(hint_lbl, 20, 46);
    lv_obj_set_size(hint_lbl, 410, 22);
    lv_label_set_text(hint_lbl, (lang == UI_LANG_FA) ? "جهت بازگردانی هر تغییر، کلید مقابل آن را لمس نمایید:" :
                                ((lang == UI_LANG_TR) ? "Geri almak icin yanindaki butona dokunun:" :
                                                        "Tap the revert button next to any item to undo:"));
    lv_obj_set_style_text_font(hint_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(hint_lbl, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(hint_lbl, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(hint_lbl, (lang == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);

    /* کانتینر اسکرول‌شونده لیست تغییرات */
    lv_obj_t *list_box = lv_obj_create(card);
    lv_obj_set_pos(list_box, 10, 70);
    lv_obj_set_size(list_box, 430, list_h);
    lv_obj_set_style_bg_color(list_box, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_border_color(list_box, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_border_width(list_box, 1, 0);
    lv_obj_set_style_radius(list_box, 8, 0);
    lv_obj_set_style_pad_all(list_box, 4, 0);
    lv_obj_set_scroll_dir(list_box, LV_DIR_VER);

    for (int i = 0; i < count; i++) {
        lv_obj_t *row = lv_obj_create(list_box);
        lv_obj_set_pos(row, 0, i * row_h);
        lv_obj_set_size(row, 402, 38);
        lv_obj_set_style_bg_color(row, lv_color_hex((i % 2 == 0) ? 0xffffff : 0xf1f5f9), 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0xe2e8f0), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        /* دکمه لغو تکی ↺ */
        lv_obj_t *btn_rev = lv_btn_create(row);
        lv_obj_set_pos(btn_rev, 4, 4);
        lv_obj_set_size(btn_rev, 34, 30);
        lv_obj_set_style_bg_color(btn_rev, lv_color_hex(0xe0e7ff), 0);
        lv_obj_set_style_border_color(btn_rev, lv_color_hex(0x818cf8), 0);
        lv_obj_set_style_border_width(btn_rev, 1, 0);
        lv_obj_set_style_radius(btn_rev, 6, 0);
        lv_obj_set_style_shadow_width(btn_rev, 0, 0);
        lv_obj_add_event_cb(btn_rev, action_unsaved_revert_single_cb, LV_EVENT_CLICKED, (void *)(intptr_t)diffs[i].id);

        lv_obj_t *l_rev = lv_label_create(btn_rev);
        lv_label_set_text(l_rev, LV_SYMBOL_REFRESH);
        lv_obj_set_style_text_font(l_rev, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(l_rev, lv_color_hex(0x4338ca), 0);
        lv_obj_center(l_rev);

        /* کانتینر مقادیر قبلی و جدید */
        lv_obj_t *val_cont = lv_obj_create(row);
        lv_obj_remove_style_all(val_cont);
        lv_obj_set_pos(val_cont, 42, 4);
        lv_obj_set_size(val_cont, 180, 30);
        lv_obj_set_layout(val_cont, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(val_cont, (lang == UI_LANG_FA) ? LV_FLEX_FLOW_ROW_REVERSE : LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(val_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(val_cont, 6, 0);
        lv_obj_clear_flag(val_cont, LV_OBJ_FLAG_SCROLLABLE);

        /* بج مقدار قبلی (خاکستری ملایم) */
        lv_obj_t *old_badge = lv_obj_create(val_cont);
        lv_obj_set_size(old_badge, LV_SIZE_CONTENT, 26);
        lv_obj_set_style_bg_color(old_badge, lv_color_hex(0xf1f5f9), 0);
        lv_obj_set_style_border_color(old_badge, lv_color_hex(0xcbd5e1), 0);
        lv_obj_set_style_border_width(old_badge, 1, 0);
        lv_obj_set_style_radius(old_badge, 5, 0);
        lv_obj_set_style_pad_hor(old_badge, 6, 0);
        lv_obj_set_style_pad_ver(old_badge, 0, 0);
        lv_obj_clear_flag(old_badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *old_lbl = lv_label_create(old_badge);
        lv_label_set_text(old_lbl, diffs[i].old_val);
        lv_obj_set_style_text_font(old_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(old_lbl, lv_color_hex(0x64748b), 0);
        lv_obj_center(old_lbl);

        /* فلش جهت تغییر */
        lv_obj_t *arr_lbl = lv_label_create(val_cont);
        lv_label_set_text(arr_lbl, (lang == UI_LANG_FA) ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_font(arr_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(arr_lbl, lv_color_hex(0x94a3b8), 0);

        /* بج مقدار جدید (آبی آسمانی شفاف) */
        lv_obj_t *new_badge = lv_obj_create(val_cont);
        lv_obj_set_size(new_badge, LV_SIZE_CONTENT, 26);
        lv_obj_set_style_bg_color(new_badge, lv_color_hex(0xe0f2fe), 0);
        lv_obj_set_style_border_color(new_badge, lv_color_hex(0x7dd3fc), 0);
        lv_obj_set_style_border_width(new_badge, 1, 0);
        lv_obj_set_style_radius(new_badge, 5, 0);
        lv_obj_set_style_pad_hor(new_badge, 6, 0);
        lv_obj_set_style_pad_ver(new_badge, 0, 0);
        lv_obj_clear_flag(new_badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *new_lbl = lv_label_create(new_badge);
        lv_label_set_text(new_lbl, diffs[i].new_val);
        lv_obj_set_style_text_font(new_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(new_lbl, lv_color_hex(0x0284c7), 0);
        lv_obj_center(new_lbl);

        /* نام پارامتر */
        lv_obj_t *name_lbl = lv_label_create(row);
        lv_obj_set_pos(name_lbl, 226, 6);
        lv_obj_set_size(name_lbl, 170, 26);
        lv_label_set_text(name_lbl, diffs[i].name);
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0x0f172a), 0);
        lv_obj_set_style_base_dir(name_lbl, ui_lang_base_dir(), 0);
        lv_obj_set_style_text_font(name_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_align(name_lbl, (lang == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    }

    /* ردیف ۳ دکمه تعاملی با چینش منطقی و ارگونومیک RTL */
    const int32_t btn_w = 136;
    const int32_t btn_h = 40;
    const int32_t start_x = 10;
    const int32_t gap = 11;

    int32_t x_save, x_discard, x_cancel;
    if (lang == UI_LANG_FA) {
        x_save    = start_x + (btn_w + gap) * 2; /* سمت راست (موقعیت اصلی در فارسی) */
        x_discard = start_x + btn_w + gap;       /* وسط */
        x_cancel  = start_x;                      /* سمت چپ */
    } else {
        x_save    = start_x;                      /* سمت چپ */
        x_discard = start_x + btn_w + gap;       /* وسط */
        x_cancel  = start_x + (btn_w + gap) * 2; /* سمت راست */
    }

    /* ۱. ذخیره و خروج (سبز) */
    lv_obj_t *btn_save = lv_btn_create(card);
    lv_obj_set_pos(btn_save, x_save, btn_y);
    lv_obj_set_size(btn_save, btn_w, btn_h);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_border_color(btn_save, lv_color_hex(0x15803d), 0);
    lv_obj_set_style_border_width(btn_save, 1, 0);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_set_style_shadow_width(btn_save, 0, 0);
    lv_obj_add_event_cb(btn_save, action_unsaved_save_and_exit_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l_save = lv_label_create(btn_save);
    lv_label_set_text(l_save, ui_lang_str(STR_UNSAVED_SAVE_EXIT));
    lv_obj_set_style_text_font(l_save, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l_save, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(l_save, ui_lang_base_dir(), 0);
    lv_obj_center(l_save);

    /* ۲. خروج بدون ذخیره (قرمز) */
    lv_obj_t *btn_discard = lv_btn_create(card);
    lv_obj_set_pos(btn_discard, x_discard, btn_y);
    lv_obj_set_size(btn_discard, btn_w, btn_h);
    lv_obj_set_style_bg_color(btn_discard, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_border_color(btn_discard, lv_color_hex(0xb91c1c), 0);
    lv_obj_set_style_border_width(btn_discard, 1, 0);
    lv_obj_set_style_radius(btn_discard, 8, 0);
    lv_obj_set_style_shadow_width(btn_discard, 0, 0);
    lv_obj_add_event_cb(btn_discard, action_unsaved_discard_and_exit_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l_discard = lv_label_create(btn_discard);
    lv_label_set_text(l_discard, ui_lang_str(STR_UNSAVED_DISCARD_EXIT));
    lv_obj_set_style_text_font(l_discard, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l_discard, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(l_discard, ui_lang_base_dir(), 0);
    lv_obj_center(l_discard);

    /* ۳. ادامه ویرایش (اسلیت تیره) */
    lv_obj_t *btn_cancel = lv_btn_create(card);
    lv_obj_set_pos(btn_cancel, x_cancel, btn_y);
    lv_obj_set_size(btn_cancel, btn_w, btn_h);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(btn_cancel, 1, 0);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_add_event_cb(btn_cancel, action_unsaved_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(l_cancel, (lang == UI_LANG_FA) ? "ادامه ویرایش" :
                                ((lang == UI_LANG_TR) ? "Duzenlemeye Don" : "Continue Editing"));
    lv_obj_set_style_text_font(l_cancel, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(l_cancel, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(l_cancel, ui_lang_base_dir(), 0);
    lv_obj_center(l_cancel);

    lvgl_unlock();
}

static void on_temp_hopper_toggle_click(lv_event_t *e)
{
    (void)e;
    bool cur = max6675_is_top_enabled();
    bool next = !cur;
    max6675_set_top_enabled(next);
    if (next) {
        panel_nav_toast_info(ui_lang_str(STR_HOPPER_SENSOR_EN));
    } else {
        panel_nav_toast_warning(ui_lang_str(STR_HOPPER_SENSOR_DIS));
    }
}

void create_screen_main_pg() {

    /* عرض امن داخل ۴۸۰: آخرین ستون باید x+w <= 472 */
    const int32_t btn_w = 70;
    const int32_t btn_h = 78; /* ارتفاع متناسب جهت تخصیص فضای کافی به برچسب‌های فارسی */
    /* وضعیت نشانگر: قرار گرفتن بالای هر دکمه به صورت جداگانه در کپسول‌های اختصاصی */
    const int32_t status_y = 117;
    const int32_t btn_y = 144;
    const int32_t label_y = 224; /* تراز عمودی دقیق جهت نمایش کامل خط ۲۵ پیکسلی و نقاط حروف */
    const int32_t ax[3] = {8, 84, 160};   /* ستون چپ */
    const int32_t bx[3] = {250, 326, 402}; /* ستون راست */

    lv_obj_t *obj = lv_obj_create(0);
    objects.main_pg = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffececec), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffececec), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;

                /* ساعت — بدون کادر سفید، مستقیم روی زمینه روشن */
                {
                    lv_obj_t *clock_cont = lv_obj_create(parent_obj);
                    objects.clock_cont = clock_cont;
                    lv_obj_set_pos(clock_cont, 9, 4);
                    lv_obj_set_size(clock_cont, 84, 30);
                    lv_obj_set_style_bg_opa(clock_cont, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(clock_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(clock_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(clock_cont, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(clock_cont, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(clock_cont, action_clock_long_press, LV_EVENT_ALL, NULL);

                    lv_obj_t *lbl_d = lv_label_create(clock_cont);
                    objects.date_lbl = lbl_d;
                    lv_obj_add_flag(lbl_d, LV_OBJ_FLAG_HIDDEN);
                    lv_label_set_text(lbl_d, "");

                    lv_obj_t *lbl = lv_label_create(clock_cont);
                    objects.time_lbl = lbl;
                    lv_label_set_text(lbl, "00:00");
                    lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0x0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(lbl, action_clock_long_press, LV_EVENT_ALL, NULL);
                    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
                }

                /* لوگو وسط بالا (۳۰٪ بزرگتر با وضوح وکتور شارپ) */
                {
                    lv_obj_t *logo = lv_image_create(parent_obj);
                    objects.astiaj_title_lbl = logo;
                    lv_image_set_src(logo, &img_astiaj_logo_img);
                    lv_image_set_scale(logo, 256);
                    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);
                }

                /* کپسول دمای مخزن بالا (شیر T1) — سمت چپ شمارنده (تراز متقارن در x=13) */
                {
                    lv_obj_t *box = lv_obj_create(parent_obj);
                    objects.temp_hopper_box = box;
                    lv_obj_set_pos(box, 13, 76);
                    lv_obj_set_size(box, 136, 34);
                    lv_obj_set_style_bg_color(box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(box, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(box, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_layout(box, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
                    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                    lv_obj_set_style_pad_column(box, 5, 0);
                    lv_obj_set_style_base_dir(box, ui_lang_base_dir(), 0);
                    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(box, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TEMP_HOPPER);

                    lv_obj_t *lbl_title = lv_label_create(box);
                    objects.temp_hopper_lbl = lbl_title;
                    lv_label_set_text(lbl_title, ui_lang_str(STR_HOPPER_LABEL));
                    lv_obj_set_style_text_font(lbl_title, ui_lang_font_16(), 0);
                    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x0f172a), 0);
                    lv_obj_set_style_base_dir(lbl_title, ui_lang_base_dir(), 0);

                    lv_obj_t *val_cont = lv_obj_create(box);
                    objects.temp_hopper_cont = val_cont;
                    lv_obj_set_size(val_cont, LV_SIZE_CONTENT, 30);
                    lv_obj_set_style_bg_opa(val_cont, 0, 0);
                    lv_obj_set_style_border_width(val_cont, 0, 0);
                    lv_obj_set_style_pad_all(val_cont, 0, 0);
                    lv_obj_set_style_pad_column(val_cont, 1, 0);
                    lv_obj_set_flex_flow(val_cont, LV_FLEX_FLOW_ROW);
                    lv_obj_set_flex_align(val_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                    lv_obj_set_style_base_dir(val_cont, LV_BASE_DIR_LTR, 0);
                    lv_obj_clear_flag(val_cont, LV_OBJ_FLAG_SCROLLABLE);

                    lv_obj_t *lbl_val = lv_label_create(val_cont);
                    objects.temp_hopper_val = lbl_val;
                    lv_label_set_text(lbl_val, "--.-");
                    lv_obj_set_style_text_color(lbl_val, max6675_is_top_valid() ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b), 0);
                    lv_obj_set_style_text_font(lbl_val, &ui_font_yekan_16, 0);
                    lv_obj_set_style_base_dir(lbl_val, LV_BASE_DIR_LTR, 0);

                    lv_obj_t *lbl_unit = lv_label_create(val_cont);
                    objects.temp_hopper_unit = lbl_unit;
                    lv_label_set_text(lbl_unit, "°C");
                    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
                    lv_obj_set_style_text_color(lbl_unit, max6675_is_top_valid() ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b), 0);
                    lv_obj_set_style_base_dir(lbl_unit, LV_BASE_DIR_LTR, 0);
                    lv_obj_set_style_translate_y(lbl_unit, 2, 0);
                }

                /* کپسول شمارنده قیف در وسط (۱۰٪ عریض‌تر بر محور X: از ۱۴۰ به ۱۵۴ با تراز متقارن در مرکز) */
                {
                    lv_obj_t *cnt_box = lv_obj_create(parent_obj);
                    lv_obj_set_pos(cnt_box, 163, 76);
                    lv_obj_set_size(cnt_box, 154, 34);
                    lv_obj_set_style_bg_color(cnt_box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(cnt_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(cnt_box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(cnt_box, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(cnt_box, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(cnt_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(cnt_box, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(cnt_box, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(cnt_box, (lv_event_cb_t)show_counter_details_dialog, LV_EVENT_CLICKED, NULL);
                    lv_obj_add_event_cb(cnt_box, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_COUNTER);

                    lv_obj_t *cnt_lbl = lv_label_create(cnt_box);
                    objects.make_icecream_counter_1 = cnt_lbl;
                    lv_label_set_text_fmt(cnt_lbl, "%ld", (long)icecream_counter);
                    lv_label_set_long_mode(cnt_lbl, LV_LABEL_LONG_CLIP);
                    const lv_font_t *main_cnt_font = (icecream_counter >= 1000000) ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
                    lv_obj_set_style_text_font(cnt_lbl, main_cnt_font, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(cnt_lbl, lv_color_hex(0x0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(cnt_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_base_dir(cnt_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_center(cnt_lbl);
                }

                /* کپسول دمای سیلندر انجماد (T2) — سمت راست شمارنده (تراز متقارن در x=331) */
                {
                    lv_obj_t *box = lv_obj_create(parent_obj);
                    objects.temp_cyl_box = box;
                    lv_obj_set_pos(box, 331, 76);
                    lv_obj_set_size(box, 136, 34);
                    lv_obj_set_style_bg_color(box, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(box, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(box, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_layout(box, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
                    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                    lv_obj_set_style_pad_column(box, 5, 0);
                    lv_obj_set_style_base_dir(box, ui_lang_base_dir(), 0);
                    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(box, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TEMP_CYL);

                    lv_obj_t *lbl_title = lv_label_create(box);
                    objects.temp_cyl_lbl = lbl_title;
                    lv_label_set_text(lbl_title, ui_lang_str(STR_CYL_LABEL));
                    lv_obj_set_style_text_font(lbl_title, ui_lang_font_16(), 0);
                    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x0f172a), 0);
                    lv_obj_set_style_base_dir(lbl_title, ui_lang_base_dir(), 0);

                    lv_obj_t *val_cont2 = lv_obj_create(box);
                    objects.temp_cyl_cont = val_cont2;
                    lv_obj_set_size(val_cont2, LV_SIZE_CONTENT, 30);
                    lv_obj_set_style_bg_opa(val_cont2, 0, 0);
                    lv_obj_set_style_border_width(val_cont2, 0, 0);
                    lv_obj_set_style_pad_all(val_cont2, 0, 0);
                    lv_obj_set_style_pad_column(val_cont2, 1, 0);
                    lv_obj_set_flex_flow(val_cont2, LV_FLEX_FLOW_ROW);
                    lv_obj_set_flex_align(val_cont2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                    lv_obj_set_style_base_dir(val_cont2, LV_BASE_DIR_LTR, 0);
                    lv_obj_clear_flag(val_cont2, LV_OBJ_FLAG_SCROLLABLE);

                    lv_obj_t *lbl_val = lv_label_create(val_cont2);
                    objects.temp_cyl_val = lbl_val;
                    lv_label_set_text(lbl_val, "--.-");
                    lv_obj_set_style_text_color(lbl_val, max6675_is_cylinder_valid() ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b), 0);
                    lv_obj_set_style_text_font(lbl_val, &ui_font_yekan_16, 0);
                    lv_obj_set_style_base_dir(lbl_val, LV_BASE_DIR_LTR, 0);

                    lv_obj_t *lbl_unit = lv_label_create(val_cont2);
                    objects.temp_cyl_unit = lbl_unit;
                    lv_label_set_text(lbl_unit, "°C");
                    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
                    lv_obj_set_style_text_color(lbl_unit, max6675_is_cylinder_valid() ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b), 0);
                    lv_obj_set_style_base_dir(lbl_unit, LV_BASE_DIR_LTR, 0);
                    lv_obj_set_style_translate_y(lbl_unit, 2, 0);
                }

                /* دکمه چرخ‌دنده تنظیمات — بدون کادر سفید، مستقیم روی زمینه */
                {
                    lv_obj_t *btn = lv_btn_create(parent_obj);
                    objects.settings_btn = btn;
                    lv_obj_set_pos(btn, 433, 2);
                    lv_obj_set_size(btn, 44, 44);
                    lv_obj_set_style_bg_opa(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

                    lv_obj_t *img = lv_image_create(btn);
                    lv_image_set_src(img, &img_icon_settings_gear);
                    lv_obj_center(img);
                    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);

                    lv_obj_add_event_cb(btn, action_go_settings, LV_EVENT_CLICKED, (void *)1);
                    lv_obj_add_event_cb(btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_SETTINGS);
                }

                /* خط تفکیک عمودی دو بخش */
                {
                    lv_obj_t *div = lv_obj_create(parent_obj);
                    lv_obj_set_pos(div, 239, 117);
                    lv_obj_set_size(div, 2, 134);
                    lv_obj_set_style_bg_color(div, lv_color_hex(0x90a4ae), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                }


                /* --- Left A: Make | Paddle | Hold --- */
                objects.comp_status = make_status_box(parent_obj, ax[0], status_y, btn_w);
                objects.compressor_btn = make_mode_btn(parent_obj, ax[0], btn_y, btn_w, btn_h,
                    &img_icon_make_icecream, NULL, action_go_make_icecream_on, (void *)3);
                lv_obj_add_event_cb(objects.compressor_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_MAKE);
                lv_obj_add_event_cb(objects.compressor_btn, mode_btn_press_tracker_cb, LV_EVENT_PRESSED, (void *)(intptr_t)1);
                objects.obj2 = make_mode_label(parent_obj, ax[0] - 1, label_y, btn_w + 2, ui_lang_str(STR_MODE_PROD));
                objects.lbl_mode_make = objects.obj2;

                objects.paddle_status = make_status_box(parent_obj, ax[1], status_y, btn_w);
                objects.icecreem_btn = make_mode_btn(parent_obj, ax[1], btn_y, btn_w, btn_h,
                    &img_icon_paddle_beater, NULL, action_go_paddle_on, (void *)4);
                lv_obj_add_event_cb(objects.icecreem_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_PADDLE);
                lv_obj_add_event_cb(objects.icecreem_btn, mode_btn_press_tracker_cb, LV_EVENT_PRESSED, (void *)(intptr_t)2);
                objects.obj3 = make_mode_label(parent_obj, ax[1] - 1, label_y, btn_w + 2, ui_lang_str(STR_MODE_PADDLE_BEATER));
                objects.lbl_mode_pdl = objects.obj3;

                objects.fridge_status = make_status_box(parent_obj, ax[2], status_y, btn_w);
                objects.fridge_btn = make_mode_btn(parent_obj, ax[2], btn_y, btn_w, btn_h,
                    &img_icon_hold_storage, NULL, action_go_compressor_on, (void *)2);
                lv_obj_add_event_cb(objects.fridge_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_HOLD);
                lv_obj_add_event_cb(objects.fridge_btn, mode_btn_press_tracker_cb, LV_EVENT_PRESSED, (void *)(intptr_t)0);
                objects.obj1 = make_mode_label(parent_obj, ax[2] - 1, label_y, btn_w + 2, ui_lang_str(STR_MODE_HOLD));
                objects.lbl_mode_hold = objects.obj1;

                /* --- Right B: لبه رنگی | رینبو | شستشو --- */
                objects.comp_status_b = make_status_box(parent_obj, bx[0], status_y, btn_w);
                objects.compressor_btn_b = make_mode_btn(parent_obj, bx[0], btn_y, btn_w, btn_h,
                    &img_icon_color_edge, NULL, action_go_make_b_on, NULL);
                lv_obj_add_event_cb(objects.compressor_btn_b, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_EDGE);
                lv_obj_add_event_cb(objects.compressor_btn_b, mode_btn_press_tracker_cb, LV_EVENT_PRESSED, (void *)(intptr_t)3);
                objects.lbl_mode_edge = make_mode_label(parent_obj, bx[0] - 1, label_y, btn_w + 2, ui_lang_str(STR_EDGE_INJECT));

                objects.fridge_status_b = make_status_box(parent_obj, bx[1], status_y, btn_w);
                objects.fridge_btn_b = make_mode_btn(parent_obj, bx[1], btn_y, btn_w, btn_h,
                    &img_icon_rainbow_swirl, NULL, action_go_hold_b_on, NULL);
                lv_obj_add_event_cb(objects.fridge_btn_b, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_RAINBOW);
                lv_obj_add_event_cb(objects.fridge_btn_b, mode_btn_press_tracker_cb, LV_EVENT_PRESSED, (void *)(intptr_t)4);
                objects.lbl_mode_rb = make_mode_label(parent_obj, bx[1] - 1, label_y, btn_w + 2, ui_lang_str(STR_RAINBOW));

                objects.paddle_status_b = make_status_box(parent_obj, bx[2], status_y, btn_w);
                objects.icecreem_btn_b = make_mode_btn(parent_obj, bx[2], btn_y, btn_w, btn_h,
                    &img_icon_wash_clean, NULL, action_go_paddle_b_on, NULL);
                lv_obj_add_event_cb(objects.icecreem_btn_b, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_WASH);
                lv_obj_add_event_cb(objects.icecreem_btn_b, mode_btn_press_tracker_cb, LV_EVENT_PRESSED, (void *)(intptr_t)5);
                objects.lbl_mode_wash = make_mode_label(parent_obj, bx[2] - 1, label_y, btn_w + 2, ui_lang_str(STR_MODE_WASH));

                /* --- کادر مانیتورینگ زنده فرآیند و وضعیت دستگاه (طراحی مدرن اپل و زیمنس) --- */
                objects.viscosity_bar = NULL;
                objects.viscosity_lbl = NULL;
                objects.daily_counter_lbl = NULL;
                objects.total_counter_lbl = NULL;
                objects.comp_cooldown_badge = NULL;
                objects.comp_cooldown_lbl = NULL;
                objects.wash_timer_badge = NULL;
                objects.wash_timer_lbl = NULL;

                {
                    lv_obj_t *m_card = lv_obj_create(parent_obj);
                    objects.live_monitor_card = m_card;
                    lv_obj_set_pos(m_card, 8, 248);
                    lv_obj_set_size(m_card, 464, 68);
                    lv_obj_set_style_bg_color(m_card, lv_color_hex(0xffffffff), 0);
                    lv_obj_set_style_bg_opa(m_card, 255, 0);
                    lv_obj_set_style_radius(m_card, 10, 0);
                    lv_obj_set_style_border_width(m_card, 1, 0);
                    lv_obj_set_style_border_color(m_card, lv_color_hex(0xcbd5e1), 0);
                    lv_obj_set_style_pad_all(m_card, 0, 0);
                    lv_obj_clear_flag(m_card, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(m_card, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(m_card, action_live_monitor_click, LV_EVENT_CLICKED, NULL);

                    /* ۱. کپسول یکپارچه وضعیت فرآیند — یک تکه کامل بدون تقسیم به دو بخش */
                    lv_obj_t *badge = lv_obj_create(m_card);
                    objects.live_monitor_status_badge = badge;
                    lv_obj_set_pos(badge, 8, 6);
                    lv_obj_set_size(badge, 448, 26);
                    lv_obj_set_style_bg_color(badge, lv_color_hex(0x334155), 0);
                    lv_obj_set_style_bg_opa(badge, 255, 0);
                    lv_obj_set_style_radius(badge, 13, 0);
                    lv_obj_set_style_border_width(badge, 0, 0);
                    lv_obj_set_style_pad_all(badge, 0, 0);
                    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(badge, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(badge, action_live_monitor_click, LV_EVENT_CLICKED, NULL);

                    lv_obj_t *s_lbl = lv_label_create(badge);
                    objects.live_monitor_status_lbl = s_lbl;
                    lv_label_set_text(s_lbl, (ui_lang_get() == UI_LANG_FA) ? "وضعیت: آماده‌به‌کار | سیستم آماده فرمان" :
                                             ((ui_lang_get() == UI_LANG_TR) ? "Durum: Hazir | Sistem Komut Bekliyor" :
                                              "Status: Standby | System Ready"));
                    lv_obj_set_style_text_font(s_lbl, ui_lang_font_16(), 0);
                    lv_obj_set_style_text_color(s_lbl, lv_color_hex(0xffffff), 0);
                    lv_obj_set_style_base_dir(s_lbl, ui_lang_base_dir(), 0);
                    lv_obj_set_width(s_lbl, 440);
                    lv_label_set_long_mode(s_lbl, LV_LABEL_LONG_CLIP);
                    lv_obj_set_style_text_align(s_lbl, LV_TEXT_ALIGN_CENTER, 0);
                    lv_obj_center(s_lbl);

                    objects.live_monitor_timer_badge = NULL;
                    objects.live_monitor_timer_lbl = NULL;

                    /* ۲. برچسب راهنمای فرآیند (خط دوم) - تمام‌عرض ۴۴۸ پیکسل با تراز دقیق وسط و رنگ با کنتراست بالا */
                    lv_obj_t *n_lbl = lv_label_create(m_card);
                    objects.live_monitor_next_lbl = n_lbl;
                    lv_obj_set_pos(n_lbl, 8, 36);
                    lv_obj_set_size(n_lbl, 448, 26);
                    lv_label_set_long_mode(n_lbl, LV_LABEL_LONG_CLIP);
                    lv_label_set_text(n_lbl, ui_lang_str(STR_MON_NEXT_SELECT));
                    lv_obj_set_style_text_font(n_lbl, ui_lang_font_16(), 0);
                    lv_obj_set_style_text_color(n_lbl, lv_color_hex(0x1e293b), 0);
                    lv_obj_set_style_base_dir(n_lbl, ui_lang_base_dir(), 0);
                    lv_obj_set_style_text_align(n_lbl, LV_TEXT_ALIGN_CENTER, 0);
                }
            }
        }
    }

    tick_screen_main_pg();
}

void tick_screen_main_pg() {
    /* اگر دیالوگ اعلان به‌روزرسانی یا پنجره‌های مودال فعال هستند، به‌روزرسانی صفحه اصلی متوقف شود تا صفر درصد لگ باشد */
    if ((s_boot_ota_backdrop && lv_obj_is_valid(s_boot_ota_backdrop)) ||
        (s_boot_wifi_backdrop && lv_obj_is_valid(s_boot_wifi_backdrop))) {
        return;
    }

    /* B9: یک بار اثبات validity آبجکت اصلی — پس از آن dirty-check کافی است.
     * lv_obj_is_valid هر بار یک linked-list scan انجام می‌دهد — گران‌قیمت است.
     * با این flag هر tick فقط یک pointer-check انجام می‌شود. */
    static bool s_main_alive = false;
    if (!s_main_alive) {
        if (!objects.live_monitor_card || !lv_obj_is_valid(objects.live_monitor_card)) return;
        s_main_alive = true;
    }

    /* اعلان آنی و سبک موفقیت ارتقا بدون تاخیر و بدون پاپ‌آپ مسدودکننده (حذف کامل لگ جاروب ۴ ثانیه‌ای) */
    static bool s_ota_congrats_shown = false;
    if (!s_ota_congrats_shown && ui_resume_is_post_ota_boot()) {
        s_ota_congrats_shown = true;
        storage_set_str("ota_prev_ver", FIRMWARE_VERSION_STR);
        panel_nav_toast_success((ui_lang_get() == UI_LANG_FA) ? "ارتقای فریم‌ور با موفقیت انجام شد" :
                                ((ui_lang_get() == UI_LANG_TR) ? "Guncelleme basariyla tamamlandi" :
                                "Firmware updated successfully"));
    }

    #define PIN_IS_ON(pin)   (gpio_get_level(pin) == 0)
    #define PIN_IS_OFF(pin)  (gpio_get_level(pin) != 0)

    /* 1. به‌روزرسانی کادر مانیتورینگ زنده فرآیند و وضعیت دستگاه (با لاجیک Dirty-Check) */
    {
        static int s_last_mode_code = -999;
        static int s_last_mode_val = -999;
        static ui_lang_t s_last_mon_lang = (ui_lang_t)-1;
        /* B1: throttle snprintf — فقط وقتی make_seq_counter یا وضعیت GPIO تغییر کرده */
        static int32_t s_last_make_seq = -1;
        static bool s_last_pdl = false, s_last_big = false, s_last_sml = false;

        int cur_mode_code = 0;
        int cur_mode_val = 0;
        uint32_t badge_bg = 0x334155;
        const char *next_text = ui_lang_str(STR_MON_NEXT_SELECT);

        bool drain_active = (s_drain_mode_active && s_drain_timer_s > 0);
        bool wash_active = wash_is_active() || (s_wash_countdown_s > 0) || drain_active;

        if (plc_is_fault()) {
            cur_mode_code = 99;
            cur_mode_val = 0;
            badge_bg = 0xdc2626; /* Apple Red */
            if (ui_lang_get() == UI_LANG_FA) {
                next_text = "خروجی‌ها قطع شدند | برای ریست کلید STOP را لمس کنید";
            } else if (ui_lang_get() == UI_LANG_TR) {
                next_text = "Tum cikislar durduruldu | Sifirlamak icin STOP tusuna basin";
            } else {
                next_text = "All outputs stopped | Press STOP or tap here to clear";
            }
        } else if (drain_active) {
            cur_mode_code = 2;
            cur_mode_val = s_drain_timer_s;
            badge_bg = 0x0891b2; /* Cyan */
            static char s_drain_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(s_drain_buf, sizeof(s_drain_buf),
                         "تخلیه سیلندر | شیر تخلیه باز | پارو در حال تخلیه مواد");
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_drain_buf, sizeof(s_drain_buf),
                         "Silindir Tahliyesi | On vana acik | Palet bosaltiyor");
            } else {
                snprintf(s_drain_buf, sizeof(s_drain_buf),
                         "Cylinder Drain | Front drain open | Paddle emptying cylinder");
            }
            next_text = s_drain_buf;
        } else if (wash_active) {
            cur_mode_code = 3;
            cur_mode_val = s_wash_countdown_s;
            badge_bg = 0x0891b2; /* Cyan */
            static char s_wash_monitor_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(s_wash_monitor_buf, sizeof(s_wash_monitor_buf),
                         "پمپ شستشو فعال | سایر خروجی‌ها خاموش | جهت توقف شستشو را لمس کنید");
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_wash_monitor_buf, sizeof(s_wash_monitor_buf),
                         "Yikama Pompasi Aktif | Diger Cikislar Kapali | Durdurmak icin dokunun");
            } else {
                snprintf(s_wash_monitor_buf, sizeof(s_wash_monitor_buf),
                         "Wash Pump Active | Other Outputs OFF | Tap wash button to stop");
            }
            next_text = s_wash_monitor_buf;
        } else if (plc_is_ready()) {
            cur_mode_code = 5;
            int32_t cur_scoops = plc_get_session_scoops();
            int32_t target_scoops = plc_get_scoops_target();
            int32_t rem_scoops = target_scoops - cur_scoops;
            if (rem_scoops < 0) rem_scoops = 0;
            int32_t hold_target_s = (front_hold_comp_s >= 2 && front_hold_comp_s <= 15) ? front_hold_comp_s : 6;
            cur_mode_val = (int)cur_scoops;
            badge_bg = 0x16a34a; /* Emerald */
            static char s_ready_monitor_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                if (cur_scoops == 0) {
                    snprintf(s_ready_monitor_buf, sizeof(s_ready_monitor_buf),
                             "کمپرسورها خاموش (بستنی سفت است) | استارت با %ld قیف یا %ld ثانیه اهرم",
                             (long)target_scoops, (long)hold_target_s);
                } else {
                    snprintf(s_ready_monitor_buf, sizeof(s_ready_monitor_buf),
                             "قیف %ld از %ld تحویل شد | استارت مجدد با %ld قیف یا %ld ثانیه اهرم",
                             (long)cur_scoops, (long)target_scoops, (long)rem_scoops, (long)hold_target_s);
                }
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_ready_monitor_buf, sizeof(s_ready_monitor_buf),
                         "Kompresorler kapali | Kolu cekin | Kompresor %ld koni veya %ld sn sonra",
                         (long)rem_scoops, (long)hold_target_s);
            } else {
                snprintf(s_ready_monitor_buf, sizeof(s_ready_monitor_buf),
                         "Compressors OFF | Ready to serve | Comp restarts in %ld cones or %ld s lever",
                         (long)rem_scoops, (long)hold_target_s);
            }
            next_text = s_ready_monitor_buf;
        } else if (plc_is_ending()) {
            cur_mode_code = 55;
            int32_t rem_ending_s = plc_get_ending_rem_s();
            cur_mode_val = (int)rem_ending_s;
            badge_bg = 0x16a34a; /* Emerald */
            static char s_ending_monitor_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(s_ending_monitor_buf, sizeof(s_ending_monitor_buf),
                         "کمپرسورها خاموش (میکروسوئیچ عقب) | آماده توزیع (%ld ثانیه)",
                         (long)rem_ending_s);
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_ending_monitor_buf, sizeof(s_ending_monitor_buf),
                         "Kapali: Kompresorler (Arka sviç) | Sonraki: Servise hazir (%ld sn)",
                         (long)rem_ending_s);
            } else {
                snprintf(s_ending_monitor_buf, sizeof(s_ending_monitor_buf),
                         "OFF: Compressors (Rear micro triggered) | Next: Ready to serve in %ld s",
                         (long)rem_ending_s);
            }
            next_text = s_ending_monitor_buf;
        } else if (plc_is_rechill()) {
            cur_mode_code = 56;
            int32_t rechill_sec = plc_get_rechill_timer();
            cur_mode_val = (int)(((rechill_sec & 0xFFFF)) | (((uint32_t)(s_comp_cooldown_left_s & 0xFF)) << 16));
            badge_bg = 0x0284c7; /* Apple Blue */
            const int32_t big_at = make_comp_delay_s();
            static char s_rechill_monitor_buf[160] = {0};
            plc_trigger_t trig = plc_get_start_trigger();
            const char *cause_fa = (trig == TRIGGER_FRONT_HELD) ? "۶ ثانیه نگه داشتن اهرم جلو" :
                                   ((trig == TRIGGER_SCOOPS_REACHED) ? "۳ بار کشیدن اهرم جلو" : "تحریک اهرم جلو");
            const char *cause_tr = (trig == TRIGGER_FRONT_HELD) ? "6 sn kol cekilmesi" : "3 koni verilmesi";
            const char *cause_en = (trig == TRIGGER_FRONT_HELD) ? "6s lever hold" : "3 cones dispensed";

            if (ui_lang_get() == UI_LANG_FA) {
                if (rechill_sec <= big_at) {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | پارو | کمپرسور در %ld ثانیه",
                             cause_fa, (long)(big_at - rechill_sec));
                } else if (s_comp_cooldown_left_s > 0) {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | پارو فعال | تعادل گاز کمپرسور: %ld ثانیه مانده",
                             cause_fa, (long)s_comp_cooldown_left_s);
                } else {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | پارو و کمپرسور بزرگ | قطع با کلاچ عقب",
                             cause_fa);
                }
            } else if (ui_lang_get() == UI_LANG_TR) {
                if (rechill_sec <= big_at) {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | Palet | Kompresor %ld sn sonra",
                             cause_tr, (long)(big_at - rechill_sec));
                } else if (s_comp_cooldown_left_s > 0) {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | Palet aktif | Gaz dengesi: %ld sn kaldi",
                             cause_tr, (long)s_comp_cooldown_left_s);
                } else {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | Palet ve Kompresor | Arka sviç durusu",
                             cause_tr);
                }
            } else {
                if (rechill_sec <= big_at) {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | Paddle | Compressor in %ld s",
                             cause_en, (long)(big_at - rechill_sec));
                } else if (s_comp_cooldown_left_s > 0) {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | Paddle active | Gas cooldown: %ld s remaining",
                             cause_en, (long)s_comp_cooldown_left_s);
                } else {
                    snprintf(s_rechill_monitor_buf, sizeof(s_rechill_monitor_buf),
                             "%s | Paddle & Compressor | Rear switch shutoff",
                             cause_en);
                }
            }
            next_text = s_rechill_monitor_buf;
        } else if (plc_is_dispense()) {
            cur_mode_code = 4;
            int32_t cur_scoops = plc_get_session_scoops();
            cur_mode_val = (int)cur_scoops;
            badge_bg = 0x16a34a; /* Emerald */
            static char s_disp_monitor_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(s_disp_monitor_buf, sizeof(s_disp_monitor_buf),
                         "پارو (اهرم جلو) | قیف %ld | رهاسازی: توقف پارو",
                         (long)cur_scoops);
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_disp_monitor_buf, sizeof(s_disp_monitor_buf),
                         "Palet (On kol) | Koni #%ld | Kolu birakinca durur",
                         (long)cur_scoops);
            } else {
                snprintf(s_disp_monitor_buf, sizeof(s_disp_monitor_buf),
                         "Paddle (Front lever) | Cone #%ld | Stops on release",
                         (long)cur_scoops);
            }
            next_text = s_disp_monitor_buf;
        } else if (make_start) {
            int pct = s_freeze_progress_pct;
            if (pct > 100) pct = 100;
            if (pct < 0) pct = 0;
            const int32_t big_at = make_comp_delay_s();
            const int32_t small_at = big_at + hold_small_delay_s();
            cur_mode_code = 6;
            cur_mode_val = (pct << 16) | (make_seq_counter & 0xFFFF);
            badge_bg = 0x0284c7;

            static char s_prod_monitor_buf[192] = {0};
            bool pdl_on = PIN_IS_ON(Paddle);
            bool big_on = PIN_IS_ON(Make_Icecream);
            bool sml_on = PIN_IS_ON(Compressor);

            static int32_t s_last_cd_s = -1;
            bool seq_changed = (make_seq_counter != s_last_make_seq || s_comp_cooldown_left_s != s_last_cd_s);
            bool gpio_changed = (pdl_on != s_last_pdl || big_on != s_last_big || sml_on != s_last_sml);
            if (seq_changed || gpio_changed) {
                s_last_make_seq = make_seq_counter;
                s_last_cd_s = s_comp_cooldown_left_s;
                s_last_pdl = pdl_on; s_last_big = big_on; s_last_sml = sml_on;

                if (ui_lang_get() == UI_LANG_FA) {
                    if (make_seq_counter < big_at && !big_on) {
                        int t_b = big_at - make_seq_counter;
                        if (t_b < 1) t_b = 1;
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "پارو | کمپرسور بزرگ در %d ثانیه", t_b);
                    } else if (!big_on && s_comp_cooldown_left_s > 0) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "پارو فعال | تعادل گاز کمپرسور: %ld ثانیه مانده", (long)s_comp_cooldown_left_s);
                    } else if (make_seq_counter < small_at && !sml_on) {
                        int rem_s = small_at - make_seq_counter;
                        if (rem_s < 1) rem_s = 1;
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "پارو و کمپرسور بزرگ | نگهدارنده در %d ثانیه", rem_s);
                    } else {
                        bool is_fb = hold_is_fallback_mode();
                        if (is_fb) {
                            bool is_cool = false;
                            int32_t rem_s = hold_get_fallback_rem_s(&is_cool);
                            int rm = rem_s / 60;
                            int rs = rem_s % 60;
                            if (sml_on) {
                                if (rm > 0) {
                                    snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                             "برودت سیلندر فعال | نگهدارنده شیر: روشن (%d دقیقه و %02d ثانیه تا استراحت)", rm, rs);
                                } else {
                                    snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                             "برودت سیلندر فعال | نگهدارنده شیر: روشن (%d ثانیه تا استراحت)", rs);
                                }
                            } else {
                                if (rm > 0) {
                                    snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                             "برودت سیلندر فعال | نگهدارنده شیر: استراحت (%d دقیقه و %02d ثانیه تا استارت)", rm, rs);
                                } else {
                                    snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                             "برودت سیلندر فعال | نگهدارنده شیر: استراحت (%d ثانیه تا استارت)", rs);
                                }
                            }
                        } else {
                            float top_t = max6675_get_temp_top();
                            int32_t t_off = hold_temp_off_c();
                            int32_t t_on = hold_temp_on_c();
                            if (sml_on) {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "برودت سیلندر فعال | نگهدارنده شیر: روشن (دما: %.1f°C - هدف: %ld°C)", top_t, (long)t_off);
                            } else {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "برودت سیلندر فعال | نگهدارنده شیر: استراحت (دما: %.1f°C - استارت: %ld°C)", top_t, (long)t_on);
                            }
                        }
                    }
                } else if (ui_lang_get() == UI_LANG_TR) {
                    if (make_seq_counter < big_at && !big_on) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "Palet | Ana Kompresor %ld sn",
                                 (long)(big_at - make_seq_counter));
                    } else if (!big_on && s_comp_cooldown_left_s > 0) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "Palet aktif | Gaz dengesi: %ld sn kaldi", (long)s_comp_cooldown_left_s);
                    } else if (make_seq_counter < small_at && !sml_on) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "Palet ve Ana Kompresor | Bekleme %ld sn",
                                 (long)(small_at - make_seq_counter));
                    } else {
                        bool is_fb = hold_is_fallback_mode();
                        if (is_fb) {
                            bool is_cool = false;
                            int32_t rem_s = hold_get_fallback_rem_s(&is_cool);
                            int rm = rem_s / 60;
                            int rs = rem_s % 60;
                            if (sml_on) {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Silindir Sogutma | Sut Bekleme: Acik (%d dk %02d sn)", rm, rs);
                            } else {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Silindir Sogutma | Sut Bekleme: Dinlenmede (%d dk %02d sn)", rm, rs);
                            }
                        } else {
                            float top_t = max6675_get_temp_top();
                            int32_t t_off = hold_temp_off_c();
                            int32_t t_on = hold_temp_on_c();
                            if (sml_on) {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Silindir Sogutma | Sut: Acik (%.1f°C - Hedef: %ld°C)", top_t, (long)t_off);
                            } else {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Silindir Sogutma | Sut: Dinlenmede (%.1f°C - Baslama: %ld°C)", top_t, (long)t_on);
                            }
                        }
                    }
                } else {
                    if (make_seq_counter < big_at && !big_on) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "Paddle | Big Compressor in %ld s",
                                 (long)(big_at - make_seq_counter));
                    } else if (!big_on && s_comp_cooldown_left_s > 0) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "Paddle active | Gas cooldown: %ld s remaining", (long)s_comp_cooldown_left_s);
                    } else if (make_seq_counter < small_at && !sml_on) {
                        snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                 "Paddle & Big Comp | Holding in %ld s",
                                 (long)(small_at - make_seq_counter));
                    } else {
                        bool is_fb = hold_is_fallback_mode();
                        if (is_fb) {
                            bool is_cool = false;
                            int32_t rem_s = hold_get_fallback_rem_s(&is_cool);
                            int rm = rem_s / 60;
                            int rs = rem_s % 60;
                            if (sml_on) {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Cylinder Cooling | Milk Holding: ON (%d min %02d s left)", rm, rs);
                            } else {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Cylinder Cooling | Milk Holding: Resting (%d min %02d s)", rm, rs);
                            }
                        } else {
                            float top_t = max6675_get_temp_top();
                            int32_t t_off = hold_temp_off_c();
                            int32_t t_on = hold_temp_on_c();
                            if (sml_on) {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Cylinder Cooling | Milk Holding: ON (%.1f°C - Target: %ld°C)", top_t, (long)t_off);
                            } else {
                                snprintf(s_prod_monitor_buf, sizeof(s_prod_monitor_buf),
                                         "Cylinder Cooling | Milk Holding: Resting (%.1f°C - Start: %ld°C)", top_t, (long)t_on);
                            }
                        }
                    }
                }
            }
            next_text = s_prod_monitor_buf;
        } else if (ice_start) {
            bool is_fb = hold_is_fallback_mode();
            bool sml_on = (gpio_get_level(Compressor) == 0);
            cur_mode_code = 7;
            static char s_hold_monitor_buf[192] = {0};

            if (!is_fb) {
                float t = max6675_get_temp_top();
                int32_t t_off = hold_temp_off_c();
                int32_t t_on = hold_temp_on_c();
                cur_mode_val = (int)(t * 10.0f) + (sml_on ? 10000 : 0);
                badge_bg = sml_on ? 0x16a34a : 0x0284c7;

                if (ui_lang_get() == UI_LANG_FA) {
                    if (sml_on) {
                        float diff = t - (float)t_off;
                        if (diff < 0.0f) diff = 0.0f;
                        snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                 "دمای شیر: %.1f°C | %.1f°C تا خاموش شدن کمپرسور (هدف: %ld°C)",
                                 t, diff, (long)t_off);
                    } else {
                        float diff = (float)t_on - t;
                        if (diff < 0.0f) diff = 0.0f;
                        snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                 "دمای شیر: %.1f°C | %.1f°C تا روشن شدن کمپرسور (هدف: %ld°C)",
                                 t, diff, (long)t_on);
                    }
                } else if (ui_lang_get() == UI_LANG_TR) {
                    if (sml_on) {
                        float diff = t - (float)t_off;
                        if (diff < 0.0f) diff = 0.0f;
                        snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                 "Sut: %.1f°C | Kapanmaya %.1f°C (Hedef: %ld°C)",
                                 t, diff, (long)t_off);
                    } else {
                        float diff = (float)t_on - t;
                        if (diff < 0.0f) diff = 0.0f;
                        snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                 "Sut: %.1f°C | Calismaya %.1f°C (Hedef: %ld°C)",
                                 t, diff, (long)t_on);
                    }
                } else {
                    if (sml_on) {
                        float diff = t - (float)t_off;
                        if (diff < 0.0f) diff = 0.0f;
                        snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                 "Milk: %.1f°C | %.1f°C until Comp OFF (Target: %ld°C)",
                                 t, diff, (long)t_off);
                    } else {
                        float diff = (float)t_on - t;
                        if (diff < 0.0f) diff = 0.0f;
                        snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                 "Milk: %.1f°C | %.1f°C until Comp ON (Target: %ld°C)",
                                 t, diff, (long)t_on);
                    }
                }
            } else {
                bool is_cooling = false;
                int32_t rem_s = hold_get_fallback_rem_s(&is_cooling);
                cur_mode_val = rem_s + (sml_on ? 100000 : 0);
                badge_bg = sml_on ? 0x16a34a : 0xd97706;
                int m = rem_s / 60;
                int s = rem_s % 60;

                if (ui_lang_get() == UI_LANG_FA) {
                    if (sml_on) {
                        if (m > 0) {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d دقیقه و %02d ثانیه تا خاموش شدن کمپرسور مخزن", m, s);
                        } else {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d ثانیه تا خاموش شدن موتور کمپرسور مخزن", s);
                        }
                    } else {
                        if (m > 0) {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d دقیقه و %02d ثانیه تا روشن شدن کمپرسور مخزن", m, s);
                        } else {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d ثانیه تا روشن شدن موتور کمپرسور مخزن", s);
                        }
                    }
                } else if (ui_lang_get() == UI_LANG_TR) {
                    if (sml_on) {
                        if (m > 0) {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "Kompresor kapanmasina: %d dk %02d sn", m, s);
                        } else {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "Kompresor kapanmasina: %d sn", s);
                        }
                    } else {
                        if (m > 0) {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "Kompresor calismasina: %d dk %02d sn", m, s);
                        } else {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "Kompresor calismasina: %d sn", s);
                        }
                    }
                } else {
                    if (sml_on) {
                        if (m > 0) {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d min %02d s until Compressor OFF", m, s);
                        } else {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d s until Compressor OFF", s);
                        }
                    } else {
                        if (m > 0) {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d min %02d s until Compressor ON", m, s);
                        } else {
                            snprintf(s_hold_monitor_buf, sizeof(s_hold_monitor_buf),
                                     "%d s until Compressor ON", s);
                        }
                    }
                }
            }
            next_text = s_hold_monitor_buf;
        } else if (paddle_start) {
            cur_mode_code = 8;
            int32_t limit = (paddle_time > 0) ? paddle_time : 900;
            int32_t rem = limit - paddle_counter;
            if (rem < 0) rem = 0;
            cur_mode_val = rem;
            badge_bg = 0x6366f1; /* Indigo */
            static char s_pdl_monitor_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(s_pdl_monitor_buf, sizeof(s_pdl_monitor_buf),
                         "چرخش همزن سیلندر | جهت توقف دستی کلید پارو را لمس کنید");
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_pdl_monitor_buf, sizeof(s_pdl_monitor_buf),
                         "Silindir paleti calisiyor | Manuel durdurmak icin dokunun");
            } else {
                snprintf(s_pdl_monitor_buf, sizeof(s_pdl_monitor_buf),
                         "Cylinder paddle running | Tap paddle button to stop manually");
            }
            next_text = s_pdl_monitor_buf;
        } else if (rainbow_is_active()) {
            cur_mode_code = 12;
            bool is_running = false;
            int32_t rem_s = rainbow_get_cycle_rem_s(&is_running);
            cur_mode_val = rem_s + (is_running ? 10000 : 0);
            badge_bg = is_running ? 0x16a34a : 0x6366f1; /* Green or Indigo */
            static char s_rb_monitor_buf[160] = {0};
            if (ui_lang_get() == UI_LANG_FA) {
                snprintf(s_rb_monitor_buf, sizeof(s_rb_monitor_buf),
                         "چرخه خودکار اختلاط شیر مایع | %ld ثانیه کار و %ld دقیقه استراحت",
                         (long)milk_mixer_on_time_s(), (long)milk_mixer_off_time_m());
            } else if (ui_lang_get() == UI_LANG_TR) {
                snprintf(s_rb_monitor_buf, sizeof(s_rb_monitor_buf),
                         "Otomatik sut karistirma | %ld sn calisma / %ld dk bekleme",
                         (long)milk_mixer_on_time_s(), (long)milk_mixer_off_time_m());
            } else {
                snprintf(s_rb_monitor_buf, sizeof(s_rb_monitor_buf),
                         "Auto milk mixing cycle | %ld s work / %ld min rest",
                         (long)milk_mixer_on_time_s(), (long)milk_mixer_off_time_m());
            }
            next_text = s_rb_monitor_buf;
        } else if (edge_is_active()) {
            cur_mode_code = 13;
            bool crm_phys = (gpio_get_level(CREAM_PUMP_GPIO) == 0);
            bool air_phys = (gpio_get_level(AIR_SOLENOID_GPIO) == 0);
            cur_mode_val = (crm_phys ? 1 : 0) | (air_phys ? 2 : 0);
            badge_bg = (crm_phys || air_phys) ? 0x16a34a : 0x0891b2; /* Cyan */
            static char s_edge_monitor_buf[160] = {0};
            if (crm_phys || air_phys) {
                if (ui_lang_get() == UI_LANG_FA) {
                    snprintf(s_edge_monitor_buf, sizeof(s_edge_monitor_buf),
                             "در حال تزریق رنگی روی بستنی | با رهاسازی اهرم، تزریق متوقف می‌شود");
                } else if (ui_lang_get() == UI_LANG_TR) {
                    snprintf(s_edge_monitor_buf, sizeof(s_edge_monitor_buf),
                             "Dondurmaya renk enjekte ediliyor | Kolu birakinca durur");
                } else {
                    snprintf(s_edge_monitor_buf, sizeof(s_edge_monitor_buf),
                             "Injecting color onto ice cream | Stops on lever release");
                }
            } else {
                if (ui_lang_get() == UI_LANG_FA) {
                    snprintf(s_edge_monitor_buf, sizeof(s_edge_monitor_buf),
                             "پمپ کرم و شیر باد آماده فرمان | با کشیدن اهرم جلو تزریق انجام می‌شود");
                } else if (ui_lang_get() == UI_LANG_TR) {
                    snprintf(s_edge_monitor_buf, sizeof(s_edge_monitor_buf),
                             "Krema pompasi ve hava vanasi hazir | Kol cekilince enjekte eder");
                } else {
                    snprintf(s_edge_monitor_buf, sizeof(s_edge_monitor_buf),
                             "Cream pump & air solenoid ready | Injects when front lever pulled");
                }
            }
            next_text = s_edge_monitor_buf;
        } else {
            /* بررسی فیزیکی جامع کلیه ۸ خروجی دستگاه (Q0..Q7) جهت نمایش دقیق هر خروجی روشن */
            bool q0_sml = (gpio_get_level(Compressor) == 0);
            bool q1_big = (gpio_get_level(Make_Icecream) == 0);
            bool q2_pdl = (gpio_get_level(Paddle) == 0);
            bool q3_crm = (gpio_get_level(CREAM_PUMP_GPIO) == 0);
            bool q4_rb  = (gpio_get_level(RAINBOW_GPIO) == 0);
            bool q5_air = (gpio_get_level(AIR_SOLENOID_GPIO) == 0);
            bool q6_rst = (gpio_get_level(RESET_RELAY_GPIO) == 0);
            bool q7_wsh = (gpio_get_level(WASH_PUMP_GPIO) == 0);

            uint8_t out_mask = (q0_sml ? 1 : 0) |
                               (q1_big ? 2 : 0) |
                               (q2_pdl ? 4 : 0) |
                               (q3_crm ? 8 : 0) |
                               (q4_rb  ? 16 : 0) |
                               (q5_air ? 32 : 0) |
                               (q6_rst ? 64 : 0) |
                               (q7_wsh ? 128 : 0);

            if (out_mask != 0) {
                cur_mode_code = 14;
                cur_mode_val = (int)out_mask;
                badge_bg = 0xd97706; /* Amber */
                static char s_act_out_buf[256] = {0};
                int pos = 0;
                bool first = true;
                if (ui_lang_get() == UI_LANG_FA) {
                    if (q2_pdl && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sپارو", first ? "" : " | "); first = false; }
                    if (q1_big && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sکمپرسور بزرگ", first ? "" : " | "); first = false; }
                    if (q0_sml && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sنگهدارنده", first ? "" : " | "); first = false; }
                    if (q7_wsh && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sپمپ شستشو", first ? "" : " | "); first = false; }
                    if (q4_rb  && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sپارو مخزن", first ? "" : " | "); first = false; }
                    if (q3_crm && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sپمپ کرم", first ? "" : " | "); first = false; }
                    if (q5_air && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sشیر باد", first ? "" : " | "); first = false; }
                    if (q6_rst && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sرله ریست", first ? "" : " | "); first = false; }
                } else if (ui_lang_get() == UI_LANG_TR) {
                    if (q2_pdl && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sPalet", first ? "" : " | "); first = false; }
                    if (q1_big && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sAna Kompresor", first ? "" : " | "); first = false; }
                    if (q0_sml && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sBekleme", first ? "" : " | "); first = false; }
                    if (q7_wsh && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sYikama", first ? "" : " | "); first = false; }
                    if (q4_rb  && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sHazne Paleti", first ? "" : " | "); first = false; }
                    if (q3_crm && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sKrema Pompasi", first ? "" : " | "); first = false; }
                    if (q5_air && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sHava Vanasi", first ? "" : " | "); first = false; }
                    if (q6_rst && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sReset Role", first ? "" : " | "); first = false; }
                } else {
                    if (q2_pdl && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sPaddle", first ? "" : " | "); first = false; }
                    if (q1_big && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sBig Compressor", first ? "" : " | "); first = false; }
                    if (q0_sml && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sHolding", first ? "" : " | "); first = false; }
                    if (q7_wsh && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sWash Pump", first ? "" : " | "); first = false; }
                    if (q4_rb  && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sHopper Paddle", first ? "" : " | "); first = false; }
                    if (q3_crm && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sCream Pump", first ? "" : " | "); first = false; }
                    if (q5_air && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sAir Solenoid", first ? "" : " | "); first = false; }
                    if (q6_rst && pos < sizeof(s_act_out_buf)) { pos += snprintf(s_act_out_buf + pos, sizeof(s_act_out_buf) - pos, "%sReset Relay", first ? "" : " | "); first = false; }
                }
                next_text = s_act_out_buf;
            } else {
                cur_mode_code = 0;
                int32_t idle_scoops = plc_get_session_scoops();
                int32_t idle_target = plc_get_scoops_target();
                int32_t idle_rem = idle_target - idle_scoops;
                if (idle_rem < 0) idle_rem = 0;
                int32_t hold_target_s = (front_hold_comp_s >= 2 && front_hold_comp_s <= 15) ? front_hold_comp_s : 6;
                cur_mode_val = (int)idle_scoops;
                badge_bg = (idle_scoops > 0) ? 0x16a34a : 0x0284c7;
                static char s_idle_monitor_buf[160] = {0};
                if (idle_scoops > 0) {
                    if (ui_lang_get() == UI_LANG_FA) {
                        snprintf(s_idle_monitor_buf, sizeof(s_idle_monitor_buf),
                                 "قیف %ld از %ld شمرده شد | استارت کمپرسور با %ld قیف یا %ld ثانیه اهرم جلو",
                                 (long)idle_scoops, (long)idle_target, (long)idle_rem, (long)hold_target_s);
                    } else if (ui_lang_get() == UI_LANG_TR) {
                        snprintf(s_idle_monitor_buf, sizeof(s_idle_monitor_buf),
                                 "Koni: %ld/%ld | Kompresore %ld Koni veya %ld sn Kaldi",
                                 (long)idle_scoops, (long)idle_target, (long)idle_rem, (long)hold_target_s);
                    } else {
                        snprintf(s_idle_monitor_buf, sizeof(s_idle_monitor_buf),
                                 "Cone: %ld/%ld | %ld Left or %ld s lever until Comp Start",
                                 (long)idle_scoops, (long)idle_target, (long)idle_rem, (long)hold_target_s);
                    }
                } else {
                    if (ui_lang_get() == UI_LANG_FA) {
                        snprintf(s_idle_monitor_buf, sizeof(s_idle_monitor_buf),
                                 "خروجی‌ها خاموش | برای شروع، یک حالت کاری را لمس کنید");
                    } else if (ui_lang_get() == UI_LANG_TR) {
                        snprintf(s_idle_monitor_buf, sizeof(s_idle_monitor_buf),
                                 "Tum cikislar kapali | Baslatmak icin bir mod secin");
                    } else {
                        snprintf(s_idle_monitor_buf, sizeof(s_idle_monitor_buf),
                                 "All outputs stopped | Tap any process to start");
                    }
                }
                next_text = s_idle_monitor_buf;
            }
        }


        if (cur_mode_code != s_last_mode_code || cur_mode_val != s_last_mode_val || ui_lang_get() != s_last_mon_lang) {
            s_last_mode_code = cur_mode_code;
            s_last_mode_val = cur_mode_val;
            s_last_mon_lang = ui_lang_get();

            if (objects.live_monitor_status_badge) {
                lv_obj_set_pos(objects.live_monitor_status_badge, 8, 7);
                lv_obj_set_size(objects.live_monitor_status_badge, 448, 26);
                lv_obj_set_style_bg_color(objects.live_monitor_status_badge, lv_color_hex(badge_bg), 0);
            }

            char status_buf[160] = {0};

            if (objects.live_monitor_status_lbl) {
                lv_obj_set_style_text_font(objects.live_monitor_status_lbl, ui_lang_font_16(), 0);
                lv_obj_set_style_base_dir(objects.live_monitor_status_lbl, ui_lang_base_dir(), 0);
                lv_obj_set_style_text_align(objects.live_monitor_status_lbl, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_set_width(objects.live_monitor_status_lbl, 440);

                switch (cur_mode_code) {
                    case 2: { /* تخلیه سیلندر */
                        int s = cur_mode_val;
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "تخلیه سیلندر | مدت %d ثانیه", s);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Silindir Bosaltma | Sure: %d sn", s);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Cylinder Drain | Time: %d s", s);
                        }
                        break;
                    }
                    case 3: { /* شستشو با پمپ Q7 */
                        int m = cur_mode_val / 60;
                        int s = cur_mode_val % 60;
                        if (ui_lang_get() == UI_LANG_FA) {
                            if (m > 0) {
                                snprintf(status_buf, sizeof(status_buf), "شستشو با پمپ | %d دقیقه و %d ثانیه مانده", m, s);
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "شستشو با پمپ | %d ثانیه مانده", s);
                            }
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Yikama Pompasi | \xE2\x80\xAD%02d:%02d\xE2\x80\xAC", m, s);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Wash Pump | \xE2\x80\xAD%02d:%02d\xE2\x80\xAC", m, s);
                        }
                        break;
                    }
                    case 4: { /* خروج بستنی / کشیدن اهرم */
                        int32_t cur = cur_mode_val;
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "در حال خروج بستنی | قیف شماره %ld", (long)cur);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Dondurma Veriliyor | %ld. Koni", (long)cur);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Dispensing Ice Cream | Cone #%ld", (long)cur);
                        }
                        break;
                    }
                    case 5: { /* بستنی آماده تحویل */
                        int32_t cur = cur_mode_val;
                        int32_t tgt = plc_get_scoops_target();
                        int32_t rem = tgt - cur;
                        if (rem < 0) rem = 0;
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "بستنی آماده تحویل | باقی‌مانده: %ld قیف", (long)rem);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Dondurma Hazir | Kalan: %ld Koni", (long)rem);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Ice Cream Ready | %ld Cones Left", (long)rem);
                        }
                        break;
                    }
                    case 6: { /* تولید بستنی و درصد انجماد */
                        int pct = (cur_mode_val >> 16) & 0xFFFF;
                        if (pct > 100) pct = 100;
                        if (pct < 0) pct = 0;
                        int el_m = make_seq_counter / 60;
                        int el_s = make_seq_counter % 60;
                        bool sml_on = PIN_IS_ON(Compressor);
                        bool top_ok = max6675_is_top_valid();
                        float top_t = max6675_get_temp_top();
                        char milk_buf[48] = {0};
                        if (ui_lang_get() == UI_LANG_FA) {
                            if (top_ok) {
                                snprintf(milk_buf, sizeof(milk_buf), "مخزن شیر: %s (%.1f°C)", sml_on ? "روشن" : "استراحت", top_t);
                            } else {
                                snprintf(milk_buf, sizeof(milk_buf), "مخزن شیر: %s", sml_on ? "روشن" : "استراحت");
                            }
                            snprintf(status_buf, sizeof(status_buf), "تولید بستنی %d٪ (%02d:%02d) | %s", pct, el_s, el_m, milk_buf);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            if (top_ok) {
                                snprintf(milk_buf, sizeof(milk_buf), "Hazne Sut: %s (%.1f°C)", sml_on ? "Acik" : "Beklemede", top_t);
                            } else {
                                snprintf(milk_buf, sizeof(milk_buf), "Hazne Sut: %s", sml_on ? "Acik" : "Beklemede");
                            }
                            snprintf(status_buf, sizeof(status_buf), "Dondurma %%%d (%02d:%02d) | %s", pct, el_m, el_s, milk_buf);
                        } else {
                            if (top_ok) {
                                snprintf(milk_buf, sizeof(milk_buf), "Milk Hopper: %s (%.1f°C)", sml_on ? "ON" : "Rest", top_t);
                            } else {
                                snprintf(milk_buf, sizeof(milk_buf), "Milk Hopper: %s", sml_on ? "ON" : "Rest");
                            }
                            snprintf(status_buf, sizeof(status_buf), "Making %d%% (%02d:%02d) | %s", pct, el_m, el_s, milk_buf);
                        }
                        break;
                    }
                    case 7: { /* نگهداری مخزن شیر */
                        bool sml_on = (gpio_get_level(Compressor) == 0);
                        bool is_fb = hold_is_fallback_mode();
                        if (ui_lang_get() == UI_LANG_FA) {
                            if (is_fb) {
                                snprintf(status_buf, sizeof(status_buf), "نگهداری مخزن (کنترل تایمری) | کمپرسور: %s", sml_on ? "روشن" : "استراحت");
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "نگهداری مخزن شیر | کمپرسور مخزن: %s", sml_on ? "روشن" : "استراحت");
                            }
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Sut Koruma%s | Kompresor: %s", is_fb ? " (Zaman)" : "", sml_on ? "Acik" : "Beklemede");
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Milk Holding%s | Compressor: %s", is_fb ? " (Timer)" : "", sml_on ? "ON" : "Resting");
                        }
                        break;
                    }
                    case 8: { /* پارو */
                        int32_t rem = cur_mode_val;
                        int rm = rem / 60;
                        int rs = rem % 60;
                        if (ui_lang_get() == UI_LANG_FA) {
                            if (rm > 0) {
                                snprintf(status_buf, sizeof(status_buf), "موتور پارو فعال | باقیمانده: %d دقیقه و %02d ثانیه", rm, rs);
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "موتور پارو فعال | باقیمانده: %d ثانیه تا توقف", rs);
                            }
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Palet Motoru Aktif | Kalan: %d dk %02d sn", rm, rs);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Paddle Motor Active | Remaining: %d min %02d s", rm, rs);
                        }
                        break;
                    }
                    case 10: { /* استارت کمپرسور بزرگ */
                        int s = cur_mode_val;
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "تاخیر کمپرسور بزرگ | استارت در %d ثانیه", s);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Ana Komp Gecikmesi | %d sn", s);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Big Comp Delay | In %d s", s);
                        }
                        break;
                    }
                    case 11: { /* استارت نگهدارنده */
                        int s = cur_mode_val;
                        if (s < 1) s = 1;
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "تاخیر نگهدارنده | استارت در %d ثانیه", s);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Bekleme Gecikmesi | %d sn", s);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Holding Delay | In %d s", s);
                        }
                        break;
                    }
                    case 12: { /* پارو مخزن شیر */
                        bool is_running = (cur_mode_val >= 10000);
                        int rem_s = cur_mode_val % 10000;
                        int rm = rem_s / 60;
                        int rs = rem_s % 60;
                        if (ui_lang_get() == UI_LANG_FA) {
                            if (is_running) {
                                snprintf(status_buf, sizeof(status_buf), "پارو مخزن شیر روشن | %d ثانیه تا پایان اختلاط", rem_s);
                            } else {
                                if (rm > 0) snprintf(status_buf, sizeof(status_buf), "پارو مخزن در استراحت | %d دقیقه و %02d ثانیه تا اختلاط بعدی", rm, rs);
                                else snprintf(status_buf, sizeof(status_buf), "پارو مخزن در استراحت | %d ثانیه تا شروع اختلاط", rs);
                            }
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            if (is_running) snprintf(status_buf, sizeof(status_buf), "Hazne Paleti Acik | %d sn kaldi", rem_s);
                            else snprintf(status_buf, sizeof(status_buf), "Hazne Paleti Beklemede | %d dk %02d sn sonra", rm, rs);
                        } else {
                            if (is_running) snprintf(status_buf, sizeof(status_buf), "Hopper Paddle ON | %d s left", rem_s);
                            else snprintf(status_buf, sizeof(status_buf), "Hopper Paddle Idle | In %d min %02d s", rm, rs);
                        }
                        break;
                    }
                    case 13: { /* سیستم لبه رنگی */
                        bool crm_phys = (cur_mode_val & 1) != 0;
                        bool air_phys = (cur_mode_val & 2) != 0;
                        if (ui_lang_get() == UI_LANG_FA) {
                            if (crm_phys || air_phys) {
                                snprintf(status_buf, sizeof(status_buf), "تزریق لبه رنگی فعال | %s%s%s",
                                         crm_phys ? "پمپ کرم" : "",
                                         (crm_phys && air_phys) ? " و " : "",
                                         air_phys ? "شیر باد" : "");
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "سیستم لبه رنگی | آماده به کار");
                            }
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            if (crm_phys || air_phys) {
                                snprintf(status_buf, sizeof(status_buf), "Renkli Kenar Aktif | %s%s%s",
                                         crm_phys ? "Krema" : "",
                                         (crm_phys && air_phys) ? " ve " : "",
                                         air_phys ? "Hava" : "");
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "Renkli Kenar | Hazir");
                            }
                        } else {
                            if (crm_phys || air_phys) {
                                snprintf(status_buf, sizeof(status_buf), "Color Edge Active | %s%s%s",
                                         crm_phys ? "Cream" : "",
                                         (crm_phys && air_phys) ? " & " : "",
                                         air_phys ? "Air" : "");
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "Color Edge | Ready");
                            }
                        }
                        break;
                    }
                    case 14: { /* خروجی فعال در مدار */
                        int cnt = 0;
                        for (int bi = 0; bi < 8; bi++) {
                            if (cur_mode_val & (1 << bi)) cnt++;
                        }
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "خروجی‌های فعال (%d خروجی در حال کار)", cnt);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Aktif Cikislar (%d Cikis Calisiyor)", cnt);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Active Outputs (%d Outputs Running)", cnt);
                        }
                        break;
                    }
                    case 55: { /* پرداخت نهایی */
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "قوام بستنی حاصل شد | پرداخت نهایی: %d ثانیه", cur_mode_val);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Kivam Tamamlandi | Palet: %d sn", cur_mode_val);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Firmness Achieved | Paddle: %d s", cur_mode_val);
                        }
                        break;
                    }
                    case 56: { /* شارژ تکمیلی */
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "شارژ قوام برودت | کارکرد: %d ثانیه", cur_mode_val);
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Kivam Tazeleme | Sure: %d sn", cur_mode_val);
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Rechill Phase | Time: %d s", cur_mode_val);
                        }
                        break;
                    }
                    case 99: { /* خطا */
                        fault_entry_t fe;
                        if (fault_log_get_entry(0, &fe)) {
                            const char *f_desc = fault_code_to_str((fault_code_t)fe.code);
                            if (ui_lang_get() == UI_LANG_FA) {
                                snprintf(status_buf, sizeof(status_buf), "خطای حفاظتی: %s", f_desc ? f_desc : "خطای سیستم");
                            } else if (ui_lang_get() == UI_LANG_TR) {
                                snprintf(status_buf, sizeof(status_buf), "Guvenlik Hatasi: %s", f_desc ? f_desc : "Hata");
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "Safety Fault: %s", f_desc ? f_desc : "Fault");
                            }
                        } else {
                            if (ui_lang_get() == UI_LANG_FA) {
                                snprintf(status_buf, sizeof(status_buf), "خطای حفاظتی در سیستم");
                            } else if (ui_lang_get() == UI_LANG_TR) {
                                snprintf(status_buf, sizeof(status_buf), "Sistem Guvenlik Hatasi");
                            } else {
                                snprintf(status_buf, sizeof(status_buf), "System Safety Fault");
                            }
                        }
                        break;
                    }
                    default: { /* آماده‌به‌کار */
                        if (ui_lang_get() == UI_LANG_FA) {
                            snprintf(status_buf, sizeof(status_buf), "وضعیت: آماده‌به‌کار | سیستم آماده فرمان");
                        } else if (ui_lang_get() == UI_LANG_TR) {
                            snprintf(status_buf, sizeof(status_buf), "Durum: Hazir | Sistem Komut Bekliyor");
                        } else {
                            snprintf(status_buf, sizeof(status_buf), "Status: Standby | System Ready");
                        }
                        break;
                    }
                }
                lv_label_set_text(objects.live_monitor_status_lbl, status_buf);
                size_t s_chars = 0;
                for (const unsigned char *p = (const unsigned char *)status_buf; *p; p++) {
                    if ((*p & 0xC0) != 0x80) s_chars++;
                }
                const lv_font_t *s_font = (s_chars > 32) ? ui_lang_font_14() : ui_lang_font_16();
                lv_obj_set_style_text_font(objects.live_monitor_status_lbl, s_font, 0);
                lv_obj_center(objects.live_monitor_status_lbl);
            }
        }

        static char s_last_rendered_next_text[256] = "";
        static ui_lang_t s_last_rendered_next_lang = (ui_lang_t)-1;
        if (objects.live_monitor_next_lbl && next_text) {
            if (strcmp(next_text, s_last_rendered_next_text) != 0 || ui_lang_get() != s_last_rendered_next_lang) {
                s_last_rendered_next_lang = ui_lang_get();
                strncpy(s_last_rendered_next_text, next_text, sizeof(s_last_rendered_next_text) - 1);
                s_last_rendered_next_text[sizeof(s_last_rendered_next_text) - 1] = '\0';
                lv_label_set_text(objects.live_monitor_next_lbl, s_last_rendered_next_text);

                size_t utf8_chars = 0;
                for (const unsigned char *p = (const unsigned char *)s_last_rendered_next_text; *p; p++) {
                    if ((*p & 0xC0) != 0x80) utf8_chars++;
                }
                const lv_font_t *dyn_font = (utf8_chars > 32) ? ui_lang_font_14() : ui_lang_font_16();
                lv_obj_set_style_text_font(objects.live_monitor_next_lbl, dyn_font, 0);
                lv_obj_set_style_base_dir(objects.live_monitor_next_lbl, ui_lang_base_dir(), 0);
                lv_obj_set_style_text_align(objects.live_monitor_next_lbl, LV_TEXT_ALIGN_CENTER, 0);
            }
        }
    }

    /* 4. تفکیک شمارنده مرکزی، امروز و کل (با Dirty-Check) */
    static int32_t s_last_cnt_main = -999;
    static int32_t s_last_cnt_daily = -999;
    static int32_t s_last_cnt_total = -999;
    if (objects.make_icecream_counter_1) {
        if (icecream_counter != s_last_cnt_main) {
            s_last_cnt_main = icecream_counter;
            lv_label_set_text_fmt(objects.make_icecream_counter_1, "%ld", (long)icecream_counter);
            const lv_font_t *main_cnt_font = (icecream_counter >= 1000000) ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
            lv_obj_set_style_text_font(objects.make_icecream_counter_1, main_cnt_font, 0);
            lv_obj_center(objects.make_icecream_counter_1);
        }
    }
    if (objects.daily_counter_lbl) {
        if (icecream_counter_daily != s_last_cnt_daily) {
            s_last_cnt_daily = icecream_counter_daily;
            lv_label_set_text_fmt(objects.daily_counter_lbl, "%s %ld", ui_lang_str(STR_TODAY), (long)icecream_counter_daily);
            lv_obj_set_style_text_font(objects.daily_counter_lbl, ui_lang_font_16(), 0);
            lv_obj_set_style_base_dir(objects.daily_counter_lbl, ui_lang_base_dir(), 0);
        }
    }
    if (objects.total_counter_lbl) {
        if (icecream_counter_total != s_last_cnt_total) {
            s_last_cnt_total = icecream_counter_total;
            lv_label_set_text_fmt(objects.total_counter_lbl, "%ld", (long)icecream_counter_total);
            lv_obj_set_style_text_font(objects.total_counter_lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_style_base_dir(objects.total_counter_lbl, LV_BASE_DIR_LTR, 0);
        }
    }

    /* 5. به‌روزرسانی آرام و بدون پرش دمای مخزن شیر (T1) و سیلندر انجماد (T2)
     * آهنگ به‌روزرسانی ۱۰ ثانیه‌ای با باند مرده ±0.25°C جهت حذف کامل پرش ارقام */
    uint32_t now_temp_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    static uint8_t s_last_temp_lang = 255;
    uint8_t cur_lang = (uint8_t)ui_lang_get();
    bool lang_changed = (cur_lang != s_last_temp_lang);
    if (lang_changed) {
        s_last_temp_lang = cur_lang;
    }

    if (objects.temp_hopper_val) {
        static float s_disp_t1 = -999.0f;
        static int8_t s_disp_t1_ok = -1;
        static uint32_t s_last_t1_disp_ms = 0;
        bool ok = max6675_is_top_valid();
        int8_t ok_val = ok ? 1 : 0;
        float t1 = max6675_get_temp_top();

        bool validity_changed = (ok_val != s_disp_t1_ok);
        bool time_elapsed = (now_temp_ms - s_last_t1_disp_ms >= 2000);
        bool large_step = (fabsf(t1 - s_disp_t1) >= 1.0f);
        bool deadband_passed = (fabsf(t1 - s_disp_t1) >= 0.10f);

        if (validity_changed || lang_changed || (ok && (s_disp_t1 < -900.0f || ((time_elapsed || large_step) && deadband_passed)))) {
            s_disp_t1_ok = ok_val;
            s_disp_t1 = t1;
            s_last_t1_disp_ms = now_temp_ms;
            if (ok) {
                char tbuf1[32];
                snprintf(tbuf1, sizeof(tbuf1), "%+.1f", t1);
                lv_label_set_text(objects.temp_hopper_val, tbuf1);
                lv_obj_set_style_text_color(objects.temp_hopper_val, lv_color_hex(0x0284c7), 0);
                if (objects.temp_hopper_unit) {
                    lv_obj_clear_flag(objects.temp_hopper_unit, LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                lv_label_set_text(objects.temp_hopper_val, "--.-");
                lv_obj_set_style_text_color(objects.temp_hopper_val, lv_color_hex(0x64748b), 0);
                if (objects.temp_hopper_unit) {
                    lv_obj_clear_flag(objects.temp_hopper_unit, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_text_color(objects.temp_hopper_unit, lv_color_hex(0x64748b), 0);
                }
            }
        }
    }

    if (objects.temp_cyl_val) {
        static float s_disp_t2 = -999.0f;
        static int8_t s_disp_t2_ok = -1;
        static uint32_t s_last_t2_disp_ms = 0;
        bool ok = max6675_is_cylinder_valid();
        int8_t ok_val = ok ? 1 : 0;
        float t2 = max6675_get_temp_cylinder();

        bool validity_changed = (ok_val != s_disp_t2_ok);
        bool time_elapsed = (now_temp_ms - s_last_t2_disp_ms >= 2000);
        bool large_step = (fabsf(t2 - s_disp_t2) >= 1.0f);
        bool deadband_passed = (fabsf(t2 - s_disp_t2) >= 0.10f);

        if (validity_changed || lang_changed || (ok && (s_disp_t2 < -900.0f || ((time_elapsed || large_step) && deadband_passed)))) {
            s_disp_t2_ok = ok_val;
            s_disp_t2 = t2;
            s_last_t2_disp_ms = now_temp_ms;
            if (ok) {
                char tbuf2[32];
                snprintf(tbuf2, sizeof(tbuf2), "%+.1f", t2);
                lv_label_set_text(objects.temp_cyl_val, tbuf2);
                lv_obj_set_style_text_color(objects.temp_cyl_val, lv_color_hex(0x0284c7), 0);
                if (objects.temp_cyl_unit) {
                    lv_obj_clear_flag(objects.temp_cyl_unit, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_text_color(objects.temp_cyl_unit, lv_color_hex(0x0284c7), 0);
                }
            } else {
                lv_label_set_text(objects.temp_cyl_val, "--.-");
                lv_obj_set_style_text_color(objects.temp_cyl_val, lv_color_hex(0x64748b), 0);
                if (objects.temp_cyl_unit) {
                    lv_obj_clear_flag(objects.temp_cyl_unit, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_text_color(objects.temp_cyl_unit, lv_color_hex(0x64748b), 0);
                }
            }
        }
    }

    /* ۶. بررسی یادآوری اتصال به وای‌فای در زمان راه‌اندازی (عدم نمایش در صورت وجود تنظیمات ذخیره‌شده) */
    static bool s_boot_wifi_prompt_checked = false;
    static uint32_t s_boot_start_ms = 0;
    if (!s_boot_wifi_prompt_checked) {
        if (s_boot_start_ms == 0) {
            s_boot_start_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        }
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        /* اگر نام و رمز قبلاً در دستگاه ذخیره شده است، هرگز یادآوری نمایش داده نشود */
        if (wifi_has_saved_credentials()) {
            s_boot_wifi_prompt_checked = true;
        } else if (now_ms - s_boot_start_ms > 15000) {
            s_boot_wifi_prompt_checked = true;
            if (wifi_is_enabled() && !wifi_is_connected()) {
                int32_t skip_cnt = storage_get_i32("wifi_skip_cnt", 0);
                if (skip_cnt < 3) {
                    show_boot_wifi_prompt_dialog();
                }
            }
        }
    } else {
        /* در صورت اتصال به وای‌فای در هر لحظه، اگر دیالوگ باز است فوراً بسته شود */
        if (wifi_is_connected()) {
            hide_boot_wifi_prompt_dialog();
        }
    }

    /* ۷. بررسی انتشار نسخه جدید فریمور پس از برقراری اینترنت و اسکن متادیتا */
    static bool s_boot_ota_prompt_checked = false;
    if (!s_boot_ota_prompt_checked && ota_is_scan_done()) {
        const lastfile_info_t *info = ota_get_info(0);
        if (info && info->ver[0] != '\0' && !info->scan_error) {
            s_boot_ota_prompt_checked = true;
            if (compare_semantic_versions(info->ver, FIRMWARE_VERSION_STR) > 0) {
                if (!machine_advisor_is_ota_snoozed()) {
                    show_boot_ota_prompt_dialog(info->ver);
                }
            }
        }
    }
}


static lv_obj_t *ui_create_terminal_tag(lv_obj_t *parent, int32_t x, int32_t y, const char *tag)
{
    lv_obj_t *tag_cont = lv_obj_create(parent);
    lv_obj_set_pos(tag_cont, x, y);
    lv_obj_set_size(tag_cont, 24, 18);
    lv_obj_set_style_pad_all(tag_cont, 0, 0);
    lv_obj_set_style_border_width(tag_cont, 0, 0);
    lv_obj_set_style_bg_opa(tag_cont, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(tag_cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (tag && tag[0] == 'I' && tag[1] >= '0' && tag[1] <= '9') {
        /* خط افقی بالای حرف I */
        lv_obj_t *bar_top = lv_obj_create(tag_cont);
        lv_obj_set_pos(bar_top, 1, 2);
        lv_obj_set_size(bar_top, 8, 2);
        lv_obj_set_style_bg_color(bar_top, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_bg_opa(bar_top, 255, 0);
        lv_obj_set_style_border_width(bar_top, 0, 0);
        lv_obj_set_style_radius(bar_top, 0, 0);
        lv_obj_set_style_pad_all(bar_top, 0, 0);
        lv_obj_clear_flag(bar_top, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        /* ساقه عمودی وسط حرف I */
        lv_obj_t *stem = lv_obj_create(tag_cont);
        lv_obj_set_pos(stem, 4, 3);
        lv_obj_set_size(stem, 2, 10);
        lv_obj_set_style_bg_color(stem, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_bg_opa(stem, 255, 0);
        lv_obj_set_style_border_width(stem, 0, 0);
        lv_obj_set_style_radius(stem, 0, 0);
        lv_obj_set_style_pad_all(stem, 0, 0);
        lv_obj_clear_flag(stem, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        /* خط افقی پایین حرف I */
        lv_obj_t *bar_bot = lv_obj_create(tag_cont);
        lv_obj_set_pos(bar_bot, 1, 12);
        lv_obj_set_size(bar_bot, 8, 2);
        lv_obj_set_style_bg_color(bar_bot, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_bg_opa(bar_bot, 255, 0);
        lv_obj_set_style_border_width(bar_bot, 0, 0);
        lv_obj_set_style_radius(bar_bot, 0, 0);
        lv_obj_set_style_pad_all(bar_bot, 0, 0);
        lv_obj_clear_flag(bar_bot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        /* عدد ایندکس کنار I */
        lv_obj_t *num_lbl = lv_label_create(tag_cont);
        lv_obj_set_pos(num_lbl, 11, 0);
        char buf[4];
        buf[0] = tag[1];
        buf[1] = '\0';
        lv_label_set_text(num_lbl, buf);
        lv_obj_set_style_text_font(num_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(num_lbl, lv_color_hex(0x0284c7), 0);
    } else if (tag) {
        /* برچسب ورودی‌های غیرایندکسی مانند LM و LC */
        lv_obj_t *txt_lbl = lv_label_create(tag_cont);
        lv_obj_set_pos(txt_lbl, 2, 0);
        lv_label_set_text(txt_lbl, tag);
        lv_obj_set_style_text_font(txt_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(txt_lbl, lv_color_hex(0x0284c7), 0);
    }

    return tag_cont;
}

static inline lv_obj_t *ui_create_serif_i_tag(lv_obj_t *parent, int32_t x, int32_t y, int index)
{
    char buf[4];
    buf[0] = 'I';
    buf[1] = (char)('0' + index);
    buf[2] = '\0';
    return ui_create_terminal_tag(parent, x, y, buf);
}

void create_screen_settings_pg() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_pg = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_set_style_bg_img_opa(obj, 70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_recolor(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffcecece), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0a0ebf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 476, 316);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_clip_corner(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_tabview_create(parent_obj, LV_DIR_RIGHT, 60);
                    objects.obj6 = obj;
                    lv_obj_set_pos(obj, 1, 1);
                    lv_obj_set_size(obj, 474, 314);
                    lv_obj_add_event_cb(obj, action_settings_tab_changed, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *tbtns = lv_tabview_get_tab_btns(obj);
                        if (tbtns) {
                            lv_obj_set_style_pad_all(tbtns, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_gap(tbtns, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(tbtns, 5, LV_PART_ITEMS | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(tbtns, 2, LV_PART_ITEMS | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(tbtns, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
                        }
                    }
                    {
                        lv_obj_t *tcontent = lv_tabview_get_content(obj);
                        if (tcontent) {
                            lv_obj_set_style_pad_all(tcontent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(tcontent, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);
                            lv_obj_set_scrollbar_mode(tcontent, LV_SCROLLBAR_MODE_OFF);
                            lv_obj_set_style_anim_duration(tcontent, 0, 0);
                        }
                    }
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // tab_f1_btn — Timers (نمونه Delta)
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F1");
                            objects.tab_f1_btn = obj;
                            lv_obj_add_event_cb(obj, action_f1_act, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj7 = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, 414, 264);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe8eef2), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                                    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        objects.obj8 = make_tab_header_bar(parent_obj, ui_lang_str(STR_TAB_F1_HDR), LV_SYMBOL_SETTINGS, 0x38bdf8);
                                        lv_obj_add_event_cb(objects.obj8, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F1);
                                        lv_obj_add_event_cb(objects.tab_f1_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F1);

                                         /* ۱. کارت‌های ۷ گانه تایمرها (ستون چپ - عرض ۲۵۲، ارتفاع ۲۹px با گپ ۲px، از ۴۱ تا ۲۵۶) */
                                         ui_create_stepper_card(parent_obj, 8, 41, 252, 29, 1, ui_lang_str(STR_TMR_PADDLE), "15", &objects.val_paddle_t1);
                                         ui_create_stepper_card(parent_obj, 8, 72, 252, 29, 3, ui_lang_str(STR_TMR_COMP_RST), "4", &objects.val_startup_t);
                                         ui_create_stepper_card(parent_obj, 8, 103, 252, 29, 4, ui_lang_str(STR_TMR_EYE), "4", &objects.val_eye_t);
                                         ui_create_stepper_card(parent_obj, 8, 134, 252, 29, 29, ui_lang_str(STR_T1_FALLBACK_ON), "20", &objects.val_fallback_on);
                                         ui_create_stepper_card(parent_obj, 8, 165, 252, 29, 30, ui_lang_str(STR_T1_FALLBACK_OFF), "5", &objects.val_fallback_off);
                                         ui_create_stepper_card(parent_obj, 8, 196, 252, 29, 31, ui_lang_str(STR_COMP_FALLBACK_ON), "3", &objects.val_comp_fb_on);
                                         ui_create_stepper_card(parent_obj, 8, 227, 252, 29, 32, ui_lang_str(STR_COMP_FALLBACK_OFF), "5", &objects.val_comp_fb_off);
                                         objects.val_paddle_t2 = NULL;

                                         /* ۲. ستون راست: کارت شمارنده تولید + کلید اسلیپ (x=264، عرض ۱۴۲، تراز متقارن به ۴۰۶ و ۲۵۰) */
                                         {
                                              /* کارت یکپارچه شمارش بستنی (y=41, ارتفاع ۹۴px) */
                                              lv_obj_t *card_cnt = lv_obj_create(parent_obj);
                                              lv_obj_set_pos(card_cnt, 264, 41);
                                              lv_obj_set_size(card_cnt, 142, 94);
                                              lv_obj_set_style_pad_all(card_cnt, 0, 0);
                                              lv_obj_set_style_radius(card_cnt, 8, 0);
                                              lv_obj_set_style_bg_color(card_cnt, lv_color_hex(0xffffff), 0);
                                              lv_obj_set_style_border_width(card_cnt, 1, 0);
                                              lv_obj_set_style_border_color(card_cnt, lv_color_hex(0xcbd5e1), 0);
                                              lv_obj_set_style_shadow_width(card_cnt, 0, 0);
                                              lv_obj_clear_flag(card_cnt, LV_OBJ_FLAG_SCROLLABLE);

                                              /* ۱. ردیف بالا: شمارنده کل (غیرقابل پاک شدن، ظرفیت تا ۸ رقم 99999999) */
                                              lv_obj_t *box_total = lv_obj_create(card_cnt);
                                              lv_obj_set_pos(box_total, 8, 4);
                                              lv_obj_set_size(box_total, 126, 22);
                                              ui_style_cyan_value(box_total);
                                              lv_obj_clear_flag(box_total, LV_OBJ_FLAG_SCROLLABLE);
                                              lv_obj_set_style_pad_all(box_total, 0, 0);

                                              lv_obj_t *lbl_tot_title = lv_label_create(box_total);
                                              objects.lbl_cnt_title = lbl_tot_title;
                                              lv_label_set_text(lbl_tot_title, ui_lang_str(STR_TOTAL));
                                              lv_obj_set_style_text_font(lbl_tot_title, ui_lang_font_16(), 0);
                                              lv_obj_set_style_text_color(lbl_tot_title, lv_color_hex(0x64748b), 0);
                                              lv_obj_set_style_base_dir(lbl_tot_title, ui_lang_base_dir(), 0);

                                              objects.total_counter_lbl = lv_label_create(box_total);
                                              lv_label_set_text_fmt(objects.total_counter_lbl, "%ld", (long)icecream_counter_total);
                                              lv_label_set_long_mode(objects.total_counter_lbl, LV_LABEL_LONG_CLIP);
                                              const lv_font_t *tot_font = (icecream_counter_total >= 1000000) ? &lv_font_montserrat_14 : &lv_font_montserrat_16;
                                              lv_obj_set_style_text_font(objects.total_counter_lbl, tot_font, 0);
                                              lv_obj_set_style_text_color(objects.total_counter_lbl, lv_color_hex(0x0f172a), 0);
                                              lv_obj_set_style_base_dir(objects.total_counter_lbl, LV_BASE_DIR_LTR, 0);

                                              if (ui_lang_get() == UI_LANG_FA) {
                                                  lv_obj_align(lbl_tot_title, LV_ALIGN_RIGHT_MID, -6, 0);
                                                  lv_obj_align(objects.total_counter_lbl, LV_ALIGN_LEFT_MID, 6, 0);
                                              } else {
                                                  lv_obj_align(lbl_tot_title, LV_ALIGN_LEFT_MID, 6, 0);
                                                  lv_obj_align(objects.total_counter_lbl, LV_ALIGN_RIGHT_MID, -6, 0);
                                              }

                                              /* ۲. ردیف میانی: شمارنده جاری (تعداد قابل ریست با فونت متناسب) */
                                              lv_obj_t *cnt_box = lv_obj_create(card_cnt);
                                              lv_obj_set_pos(cnt_box, 8, 30);
                                              lv_obj_set_size(cnt_box, 126, 32);
                                              ui_style_cyan_value(cnt_box);
                                              lv_obj_clear_flag(cnt_box, LV_OBJ_FLAG_SCROLLABLE);
                                              lv_obj_set_style_pad_all(cnt_box, 0, 0);

                                              objects.make_icecream_counter_2 = lv_label_create(cnt_box);
                                              lv_label_set_text_fmt(objects.make_icecream_counter_2, "%ld", (long)icecream_counter);
                                              lv_label_set_long_mode(objects.make_icecream_counter_2, LV_LABEL_LONG_CLIP);
                                              const lv_font_t *cnt_font = (icecream_counter >= 1000000) ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
                                              lv_obj_set_style_text_font(objects.make_icecream_counter_2, cnt_font, 0);
                                              lv_obj_set_style_text_color(objects.make_icecream_counter_2, lv_color_hex(0x0f172a), 0);
                                              lv_obj_center(objects.make_icecream_counter_2);

                                              /* ۳. ردیف پایین: کلید صفر کردن (طراحی متناسب و ارگونومیک اپل h=24px) */
                                              objects.reset_counter_icecream = lv_btn_create(card_cnt);
                                              lv_obj_set_pos(objects.reset_counter_icecream, 8, 66);
                                              lv_obj_set_size(objects.reset_counter_icecream, 126, 24);
                                              lv_obj_set_style_radius(objects.reset_counter_icecream, 6, 0);
                                              lv_obj_set_style_bg_color(objects.reset_counter_icecream, lv_color_hex(0xea580c), 0);
                                              lv_obj_set_style_shadow_width(objects.reset_counter_icecream, 0, 0);
                                              lv_obj_add_event_cb(objects.reset_counter_icecream, action_reset_counter_icecream, LV_EVENT_CLICKED, NULL);

                                              lv_obj_t *lbl_btn_rst = lv_label_create(objects.reset_counter_icecream);
                                              objects.lbl_cnt_rst = lbl_btn_rst;
                                              lv_label_set_text(lbl_btn_rst, ui_lang_str(STR_COUNTER_RESET));
                                              lv_obj_set_style_text_font(lbl_btn_rst, ui_lang_font_16(), 0);
                                              lv_obj_set_style_text_color(lbl_btn_rst, lv_color_hex(0xffffff), 0);
                                              lv_obj_set_style_base_dir(lbl_btn_rst, ui_lang_base_dir(), 0);
                                              lv_obj_center(lbl_btn_rst);

                                               /* ۱. کارت اسلیپ صفحه نمایش استایل اپل دو سطحی (فاصله سخاوتمندانه متن و بج) */
                                               objects.btn_ss_timeout = lv_btn_create(parent_obj);
                                               lv_obj_set_pos(objects.btn_ss_timeout, 264, 138);
                                               lv_obj_set_size(objects.btn_ss_timeout, 142, 55);
                                               lv_obj_set_style_radius(objects.btn_ss_timeout, 8, 0);
                                               lv_obj_set_style_bg_color(objects.btn_ss_timeout, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_bg_opa(objects.btn_ss_timeout, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_border_width(objects.btn_ss_timeout, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_border_color(objects.btn_ss_timeout, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_shadow_width(objects.btn_ss_timeout, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_pad_all(objects.btn_ss_timeout, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_bg_color(objects.btn_ss_timeout, lv_color_hex(0xf1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
                                               lv_obj_set_style_translate_y(objects.btn_ss_timeout, 1, LV_PART_MAIN | LV_STATE_PRESSED);
                                               lv_obj_clear_flag(objects.btn_ss_timeout, LV_OBJ_FLAG_SCROLLABLE);
                                               lv_obj_add_event_cb(objects.btn_ss_timeout, action_toggle_ss_timeout, LV_EVENT_CLICKED, NULL);

                                               lv_obj_t *lbl_title = lv_label_create(objects.btn_ss_timeout);
                                               objects.lbl_ss_title = lbl_title;
                                               lv_label_set_text(lbl_title, ui_lang_str(STR_STANDBY_PREFIX));
                                               lv_obj_set_style_text_font(lbl_title, ui_lang_font_16(), 0);
                                               lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x475569), 0);
                                               lv_obj_set_style_base_dir(lbl_title, ui_lang_base_dir(), 0);
                                               lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -4 : 4);

                                               lv_obj_t *pill = lv_obj_create(objects.btn_ss_timeout);
                                               lv_obj_set_size(pill, 82, 20);
                                               lv_obj_align(pill, LV_ALIGN_BOTTOM_MID, 0, -6);
                                               lv_obj_set_style_radius(pill, 10, 0);
                                               lv_color_t clr_init = (screensaver_timeout_s > 0) ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b);
                                               lv_obj_set_style_bg_color(pill, clr_init, 0);
                                               lv_obj_set_style_bg_opa(pill, 255, 0);
                                               lv_obj_set_style_border_width(pill, 0, 0);
                                               lv_obj_set_style_pad_all(pill, 0, 0);
                                               lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

                                               lv_obj_t *lbl_val = lv_label_create(pill);
                                               objects.lbl_ss_val = lbl_val;
                                               lv_label_set_text(lbl_val, get_ss_timeout_text(screensaver_timeout_s));
                                               lv_obj_set_style_text_font(lbl_val, ui_lang_font_16(), 0);
                                               lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xffffff), 0);
                                               lv_obj_set_style_base_dir(lbl_val, ui_lang_base_dir(), 0);
                                               lv_obj_align(lbl_val, LV_ALIGN_CENTER, 0, -1);

                                               /* ۲. کارت سنسور پولی استایل اپل دو سطحی (فاصله سخاوتمندانه متن و بج) */
                                               objects.btn_pulley_en = lv_btn_create(parent_obj);
                                               lv_obj_set_pos(objects.btn_pulley_en, 264, 197);
                                               lv_obj_set_size(objects.btn_pulley_en, 142, 55);
                                               lv_obj_set_style_radius(objects.btn_pulley_en, 8, 0);
                                               lv_obj_set_style_bg_color(objects.btn_pulley_en, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_bg_opa(objects.btn_pulley_en, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_border_width(objects.btn_pulley_en, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_border_color(objects.btn_pulley_en, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_shadow_width(objects.btn_pulley_en, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_pad_all(objects.btn_pulley_en, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                               lv_obj_set_style_bg_color(objects.btn_pulley_en, lv_color_hex(0xf1f5f9), LV_PART_MAIN | LV_STATE_PRESSED);
                                               lv_obj_set_style_translate_y(objects.btn_pulley_en, 1, LV_PART_MAIN | LV_STATE_PRESSED);
                                               lv_obj_clear_flag(objects.btn_pulley_en, LV_OBJ_FLAG_SCROLLABLE);
                                               lv_obj_add_event_cb(objects.btn_pulley_en, action_toggle_pulley_en, LV_EVENT_CLICKED, NULL);

                                               lv_obj_t *lbl_p_title = lv_label_create(objects.btn_pulley_en);
                                               objects.lbl_pulley_title = lbl_p_title;
                                               lv_label_set_text(lbl_p_title, ui_lang_str(STR_PULLEY_SENSOR));
                                               lv_obj_set_style_text_font(lbl_p_title, ui_lang_font_16(), 0);
                                               lv_obj_set_style_text_color(lbl_p_title, lv_color_hex(0x475569), 0);
                                               lv_obj_set_style_base_dir(lbl_p_title, ui_lang_base_dir(), 0);
                                               lv_obj_align(lbl_p_title, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -4 : 4);

                                               lv_obj_t *p_pill = lv_obj_create(objects.btn_pulley_en);
                                               lv_obj_set_size(p_pill, 82, 20);
                                               lv_obj_align(p_pill, LV_ALIGN_BOTTOM_MID, 0, -6);
                                               lv_obj_set_style_radius(p_pill, 10, 0);
                                               lv_color_t p_clr = s_pulley_en ? lv_color_hex(0x16a34a) : lv_color_hex(0x64748b);
                                               lv_obj_set_style_bg_color(p_pill, p_clr, 0);
                                               lv_obj_set_style_bg_opa(p_pill, 255, 0);
                                               lv_obj_set_style_border_width(p_pill, 0, 0);
                                               lv_obj_set_style_pad_all(p_pill, 0, 0);
                                               lv_obj_clear_flag(p_pill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

                                               lv_obj_t *lbl_p_val = lv_label_create(p_pill);
                                               objects.lbl_pulley_val = lbl_p_val;
                                               lv_label_set_text(lbl_p_val, s_pulley_en ? ui_lang_str(STR_PULLEY_ENABLED) : ui_lang_str(STR_PULLEY_DISABLED));
                                               lv_obj_set_style_text_font(lbl_p_val, ui_lang_font_16(), 0);
                                               lv_obj_set_style_text_color(lbl_p_val, lv_color_hex(0xffffff), 0);
                                               lv_obj_set_style_base_dir(lbl_p_val, ui_lang_base_dir(), 0);
                                               lv_obj_align(lbl_p_val, LV_ALIGN_CENTER, 0, -1);
                                         }
                                    }
                                }
                            }
                        }
                        {
                            // tab_f2_btn
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F2");
                            objects.tab_f2_btn = obj;
                            lv_obj_add_event_cb(obj, action_f2_act, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj9 = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, 414, 264);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd2eafc), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                                    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        objects.obj10 = make_tab_header_bar(parent_obj, ui_lang_str(STR_TAB_F2_HDR), LV_SYMBOL_REFRESH, 0xf59e0b);
                                        lv_obj_add_event_cb(objects.obj10, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F2);
                                        lv_obj_add_event_cb(objects.tab_f2_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F2);

                                         /* مقادیر اولیه زنده از ساعت سخت‌افزاری دستگاه */
                                         char init_y[8], init_m[4], init_d[4], init_h[4], init_mi[4];
                                         if (ui_lang_get() == UI_LANG_FA) {
                                             snprintf(init_y, sizeof(init_y), "%04d", (jt.jy > 0) ? jt.jy : 1404);
                                             snprintf(init_m, sizeof(init_m), "%02d", (jt.jm > 0) ? jt.jm : 1);
                                             snprintf(init_d, sizeof(init_d), "%02d", (jt.jd > 0) ? jt.jd : 1);
                                         } else {
                                             int gy = 2026, gm = 1, gd = 1;
                                             jalali_to_gregorian((jt.jy > 0) ? jt.jy : 1404,
                                                                 (jt.jm > 0) ? jt.jm : 1,
                                                                 (jt.jd > 0) ? jt.jd : 1,
                                                                 &gy, &gm, &gd);
                                             snprintf(init_y, sizeof(init_y), "%04d", gy);
                                             snprintf(init_m, sizeof(init_m), "%02d", gm);
                                             snprintf(init_d, sizeof(init_d), "%02d", gd);
                                         }
                                         snprintf(init_h, sizeof(init_h), "%02d", jt.hour);
                                         snprintf(init_mi, sizeof(init_mi), "%02d", jt.minute);

                                         /* ۱. ستون چپ: کارت‌های ۵ گانه تنظیم ساعت و تاریخ + کلید NTP (تراز دقیق با لبه ۲۶۰) */
                                         ui_create_stepper_card(parent_obj, 8, 38, 176, 34, 5, ui_lang_str(STR_YEAR), init_y, &objects.year_lbl);
                                         ui_create_stepper_card(parent_obj, 8, 75, 176, 34, 6, ui_lang_str(STR_MONTH), init_m, &objects.month_lbl);
                                         ui_create_stepper_card(parent_obj, 8, 112, 176, 34, 7, ui_lang_str(STR_DAY), init_d, &objects.day_lbl);
                                         ui_create_stepper_card(parent_obj, 8, 149, 176, 34, 8, ui_lang_str(STR_HOUR), init_h, &objects.hour_lbl);
                                         ui_create_stepper_card(parent_obj, 8, 186, 176, 34, 9, ui_lang_str(STR_MINUTE), init_mi, &objects.min_lbl);

                                         /* دکمه همگام‌سازی زمان با سرور NTP اینترنت (تراز دقیق با لبه ۲۶۰) */
                                         objects.btn_ntp_sync = lv_btn_create(parent_obj);
                                         lv_obj_set_pos(objects.btn_ntp_sync, 8, 223);
                                         lv_obj_set_size(objects.btn_ntp_sync, 176, 37);
                                         lv_obj_set_style_radius(objects.btn_ntp_sync, 8, 0);
                                         bool net_online = wifi_is_connected();
                                         lv_color_t ntp_col = net_online ? lv_color_hex(0x0284c7) : lv_color_hex(0x64748b);
                                         lv_color_t ntp_border = net_online ? lv_color_hex(0x0369a1) : lv_color_hex(0x475569);
                                         lv_obj_set_style_bg_color(objects.btn_ntp_sync, ntp_col, 0);
                                         lv_obj_set_style_border_color(objects.btn_ntp_sync, ntp_border, 0);
                                         lv_obj_set_style_border_width(objects.btn_ntp_sync, 1, 0);
                                         lv_obj_set_style_shadow_width(objects.btn_ntp_sync, 0, 0);
                                         lv_obj_add_event_cb(objects.btn_ntp_sync, action_ntp_sync_cb, LV_EVENT_CLICKED, NULL);

                                         lv_obj_t *lbl_ntp = lv_label_create(objects.btn_ntp_sync);
                                         objects.lbl_ntp_sync = lbl_ntp;
                                         lv_label_set_text(lbl_ntp, ui_lang_str(STR_NTP_SYNC_BTN));
                                         lv_obj_set_style_text_font(lbl_ntp, ui_lang_font_16(), 0);
                                         lv_obj_set_style_text_color(lbl_ntp, lv_color_hex(0xffffff), 0);
                                         lv_obj_set_style_base_dir(lbl_ntp, ui_lang_base_dir(), 0);
                                         lv_obj_center(lbl_ntp);

                                          /* ۲. ستون راست: کارت روزشمار و قفل اعتباری آستیاج (x=190, عرض ۲۱۶ قفل به ۴۰۶) */
                                          lv_obj_t *card_dl = lv_obj_create(parent_obj);
                                          objects.lease_card = card_dl;
                                          lv_obj_set_pos(card_dl, 190, 38);
                                          lv_obj_set_size(card_dl, 216, 222);
                                          lv_obj_set_style_pad_all(card_dl, 0, 0);
                                          lv_obj_set_style_radius(card_dl, 10, 0);
                                          lv_obj_set_style_bg_color(card_dl, lv_color_hex(0xffffff), 0);
                                          lv_obj_set_style_border_width(card_dl, 1, 0);
                                          lv_obj_set_style_border_color(card_dl, lv_color_hex(0xcbd5e1), 0);
                                          lv_obj_set_style_shadow_width(card_dl, 0, 0);
                                          lv_obj_clear_flag(card_dl, LV_OBJ_FLAG_SCROLLABLE);

                                          /* عنوان کارت */
                                          lv_obj_t *lbl_dl_hdr = lv_label_create(card_dl);
                                          objects.lbl_dl_hdr = lbl_dl_hdr;
                                          lv_label_set_text(lbl_dl_hdr, ui_lang_str(STR_DL_TITLE));
                                          lv_obj_set_style_text_font(lbl_dl_hdr, ui_lang_font_16(), 0);
                                          lv_obj_set_style_text_color(lbl_dl_hdr, lv_color_hex(0x0f172a), 0);
                                          lv_obj_set_style_base_dir(lbl_dl_hdr, ui_lang_base_dir(), 0);
                                          lv_obj_align(lbl_dl_hdr, LV_ALIGN_TOP_MID, 0, 3);

                                           /* نشانگر وضعیت اعتبار / روزهای باقیمانده */
                                           lv_obj_t *badge_days = lv_obj_create(card_dl);
                                           objects.lease_status_badge = badge_days;
                                           lv_obj_set_pos(badge_days, 11, 26);
                                           lv_obj_set_size(badge_days, 194, 28);
                                           lv_obj_set_style_radius(badge_days, 8, 0);
                                           lv_obj_set_style_pad_all(badge_days, 0, 0);

                                           DayLockState_t l_st = day_lock_get_state();
                                           bool is_locked = (l_st == LOCK_STAGE_HARD_LOCK || l_st == LOCK_STAGE_TAMPER);

                                           if (is_locked) {
                                               lv_obj_set_style_bg_color(badge_days, lv_color_hex(0xfef2f2), 0);
                                               lv_obj_set_style_border_color(badge_days, lv_color_hex(0xf87171), 0);
                                               lv_obj_set_style_border_width(badge_days, 1, 0);
                                           } else if (l_st == LOCK_STAGE_ACTIVE_LEASE || l_st == LOCK_STAGE_GRACE_10) {
                                               lv_obj_set_style_bg_color(badge_days, lv_color_hex(0xf0fdf4), 0);
                                               lv_obj_set_style_border_color(badge_days, lv_color_hex(0x86efac), 0);
                                               lv_obj_set_style_border_width(badge_days, 1, 0);
                                           } else {
                                               lv_obj_set_style_bg_color(badge_days, lv_color_hex(0xf0f9ff), 0);
                                               lv_obj_set_style_border_color(badge_days, lv_color_hex(0x7dd3fc), 0);
                                               lv_obj_set_style_border_width(badge_days, 1, 0);
                                           }

                                           lv_obj_t *lbl_days_txt = lv_label_create(badge_days);
                                           objects.lease_days_lbl = lbl_days_txt;
                                           uint8_t init_stg = day_lock_get_active_stage();
                                           uint8_t tot_stg = day_lock_get_total_stages();
                                           if (tot_stg < init_stg) tot_stg = init_stg;
                                           if (tot_stg < 1) tot_stg = 1;
                                           ui_lang_t init_lang = ui_lang_get();
                                           if (is_locked) {
                                               if (init_stg >= 1 && init_stg <= DAY_LOCK_MAX_STAGES) {
                                                   char lk_buf[48];
                                                   if (init_lang == UI_LANG_FA) {
                                                       snprintf(lk_buf, sizeof(lk_buf), "قفل: پایان قسط %u از %u", (unsigned)init_stg, (unsigned)tot_stg);
                                                   } else if (init_lang == UI_LANG_TR) {
                                                       snprintf(lk_buf, sizeof(lk_buf), "Kilitli: %u/%u Taksit", (unsigned)init_stg, (unsigned)tot_stg);
                                                   } else {
                                                       snprintf(lk_buf, sizeof(lk_buf), "Locked: Stage %u/%u", (unsigned)init_stg, (unsigned)tot_stg);
                                                   }
                                                   lv_label_set_text(lbl_days_txt, lk_buf);
                                               } else {
                                                   lv_label_set_text(lbl_days_txt, ui_lang_str(STR_DL_STATUS_LOCKED));
                                               }
                                               lv_obj_set_style_text_color(lbl_days_txt, lv_color_hex(0xdc2626), 0);
                                           } else if (l_st == LOCK_STAGE_ACTIVE_LEASE || l_st == LOCK_STAGE_GRACE_10) {
                                               char act_buf[48];
                                               format_lease_remain_str(act_buf, sizeof(act_buf), init_stg, tot_stg, init_lang);
                                               lv_label_set_text(lbl_days_txt, act_buf);
                                               lv_obj_set_style_text_color(lbl_days_txt, lv_color_hex(0x16a34a), 0);
                                           } else {
                                               lv_label_set_text(lbl_days_txt, ui_lang_str(STR_DL_STATUS_UNLIMITED));
                                               lv_obj_set_style_text_color(lbl_days_txt, lv_color_hex(0x0284c7), 0);
                                           }
                                           lv_obj_set_style_text_font(lbl_days_txt, ui_lang_font_16(), 0);
                                           lv_obj_set_style_base_dir(lbl_days_txt, ui_lang_base_dir(), 0);
                                           lv_obj_center(lbl_days_txt);

                                           objects.lease_btn_extend = NULL;

                                           /* دکمه تنظیم زنجیره ۵ قسط (نیازمند رمز عبور) */
                                           objects.lease_btn_5timers = lv_btn_create(card_dl);
                                           lv_obj_set_pos(objects.lease_btn_5timers, 11, 56);
                                           lv_obj_set_size(objects.lease_btn_5timers, 194, 30);
                                           lv_obj_set_style_radius(objects.lease_btn_5timers, 8, 0);
                                           lv_obj_set_style_bg_color(objects.lease_btn_5timers, lv_color_hex(0x0284c7), 0);
                                           lv_obj_set_style_border_color(objects.lease_btn_5timers, lv_color_hex(0x0369a1), 0);
                                           lv_obj_set_style_border_width(objects.lease_btn_5timers, 1, 0);
                                           lv_obj_set_style_shadow_width(objects.lease_btn_5timers, 0, 0);
                                           lv_obj_add_event_cb(objects.lease_btn_5timers, action_open_5timers_dialog, LV_EVENT_CLICKED, NULL);

                                           lv_obj_t *lbl_btn_5t = lv_label_create(objects.lease_btn_5timers);
                                           lv_label_set_text(lbl_btn_5t, ui_lang_str(STR_DL_5TIMERS_BTN));
                                            lv_obj_set_style_text_font(lbl_btn_5t, ui_lang_font_16(), 0);
                                           lv_obj_set_style_text_color(lbl_btn_5t, lv_color_hex(0xffffff), 0);
                                           lv_obj_set_style_base_dir(lbl_btn_5t, ui_lang_base_dir(), 0);
                                           lv_obj_center(lbl_btn_5t);

                                            /* کادر راهنما و پشتیبانی کارخانه آستیاج استایل کارت توکار اپل */
                                            lv_obj_t *sup_card = lv_obj_create(card_dl);
                                            lv_obj_set_pos(sup_card, 11, 90);
                                            lv_obj_set_size(sup_card, 194, 56);
                                            lv_obj_set_style_radius(sup_card, 8, 0);
                                            lv_obj_set_style_bg_color(sup_card, lv_color_hex(0xf8fafc), 0);
                                            lv_obj_set_style_border_color(sup_card, lv_color_hex(0xe2e8f0), 0);
                                            lv_obj_set_style_border_width(sup_card, 1, 0);
                                            lv_obj_set_style_pad_all(sup_card, 2, 0);
                                            lv_obj_clear_flag(sup_card, LV_OBJ_FLAG_SCROLLABLE);

                                            lv_obj_t *lbl_sup = lv_label_create(sup_card);
                                            objects.lbl_dl_sup = lbl_sup;
                                            lv_label_set_text(lbl_sup, ui_lang_str(STR_DL_SUPPORT));
                                            lv_obj_set_style_text_font(lbl_sup, ui_lang_font_16(), 0);
                                            lv_obj_set_style_text_color(lbl_sup, lv_color_hex(0x334155), 0);
                                            lv_obj_set_style_text_line_space(lbl_sup, 0, 0);
                                            lv_obj_set_style_base_dir(lbl_sup, ui_lang_base_dir(), 0);
                                            lv_obj_set_style_text_align(lbl_sup, LV_TEXT_ALIGN_CENTER, 0);
                                            lv_obj_center(lbl_sup);
                                           objects.lbl_dl_emg = NULL;

                                           /* دکمه بازنشانی تنظیمات کارخانه (Factory Reset) زیر آدرس سایت آستیاج */
                                           objects.btn_factory_reset = lv_btn_create(card_dl);
                                           lv_obj_set_pos(objects.btn_factory_reset, 11, 150);
                                           lv_obj_set_size(objects.btn_factory_reset, 194, 60);
                                           lv_obj_set_style_radius(objects.btn_factory_reset, 10, 0);
                                           lv_obj_set_style_bg_color(objects.btn_factory_reset, lv_color_hex(0xdc2626), 0);
                                           lv_obj_set_style_bg_color(objects.btn_factory_reset, lv_color_hex(0xb91c1c), LV_STATE_PRESSED);
                                           lv_obj_set_style_border_color(objects.btn_factory_reset, lv_color_hex(0xef4444), 0);
                                           lv_obj_set_style_border_width(objects.btn_factory_reset, 1, 0);
                                           lv_obj_set_style_shadow_width(objects.btn_factory_reset, 0, 0);
                                           lv_obj_set_style_pad_all(objects.btn_factory_reset, 4, 0);
                                           lv_obj_add_event_cb(objects.btn_factory_reset, action_manager_factory_reset, LV_EVENT_CLICKED, NULL);

                                           objects.lbl_factory_reset = lv_label_create(objects.btn_factory_reset);
                                           char rst_init_txt[80];
                                           if (ui_lang_get() == UI_LANG_FA) {
                                               snprintf(rst_init_txt, sizeof(rst_init_txt), "%s بازنشانی کارخانه\n(تنظیمات اولیه)", LV_SYMBOL_REFRESH);
                                           } else if (ui_lang_get() == UI_LANG_TR) {
                                               snprintf(rst_init_txt, sizeof(rst_init_txt), "%s Fabrika Ayarlari\n(Varsayilana Don)", LV_SYMBOL_REFRESH);
                                           } else {
                                               snprintf(rst_init_txt, sizeof(rst_init_txt), "%s Factory Reset\n(Restore Defaults)", LV_SYMBOL_REFRESH);
                                           }
                                           lv_label_set_text(objects.lbl_factory_reset, rst_init_txt);
                                           lv_obj_set_style_text_font(objects.lbl_factory_reset, ui_lang_font_16(), 0);
                                           lv_obj_set_style_text_color(objects.lbl_factory_reset, lv_color_hex(0xffffff), 0);
                                           lv_obj_set_style_text_line_space(objects.lbl_factory_reset, 3, 0);
                                           lv_obj_set_style_text_align(objects.lbl_factory_reset, LV_TEXT_ALIGN_CENTER, 0);
                                           lv_obj_set_style_base_dir(objects.lbl_factory_reset, ui_lang_base_dir(), 0);
                                           lv_obj_center(objects.lbl_factory_reset);
                                     }
                                }
                            }
                        }
                        {
                            // tab_f3_btn — Current / Restart (نمونه Delta)
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F3");
                            objects.tab_f3_btn = obj;
                            lv_obj_add_event_cb(obj, action_f3_act, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj11 = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, 414, 264);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe8eef2), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                                    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                         objects.obj12 = make_tab_header_bar(parent_obj, ui_lang_str(STR_TAB_F3_HDR), LV_SYMBOL_TINT, 0x06b6d4);
                                         lv_obj_add_event_cb(objects.obj12, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F3);
                                         lv_obj_add_event_cb(objects.tab_f3_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F3);
                                         /* کارت‌های ۵ گانه تنظیم حالت نگهداری و برودت (عرض ۳۹۸px قفل به مرز ۴۰۶ با گپ استاندارد ۵ پیکسلی متقارن) */
                                         ui_create_stepper_card(parent_obj, 8, 39, 398, 40, 11, ui_lang_str(STR_COMP_CYCLE), "120", &objects.val_rst_t1);
                                         ui_create_stepper_card(parent_obj, 8, 84, 398, 40, 20, ui_lang_str(STR_F5_MAIN_COMP), "0", &objects.comp3phas_act_time_lbl);
                                         ui_create_stepper_card(parent_obj, 8, 129, 398, 40, 12, ui_lang_str(STR_HOLD_DELAY), "30", &objects.val_rst_t2);
                                         ui_create_stepper_card(parent_obj, 8, 174, 398, 40, 27, ui_lang_str(STR_TEMP_T1_ON), "4", &objects.val_t1_on);
                                         ui_create_stepper_card(parent_obj, 8, 219, 398, 40, 28, ui_lang_str(STR_TEMP_T1_OFF), "2", &objects.val_t1_off);
                                        objects.paddle_time_lbl = NULL;
                                        objects.val_modbus = NULL;
                                        objects.val_cur1a = NULL;
                                        objects.val_cur2a = NULL;
                                        objects.current_set_1 = NULL;
                                        objects.current_set_2 = NULL;
                                        objects.current_set_1_key = NULL;
                                        objects.current_set_2_key = NULL;
                                    }
                                }
                            }
                        }
                        {
                            // tab_f4_btn
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F4");
                            objects.tab_f4_btn = obj;
                            lv_obj_add_event_cb(obj, action_f4_act, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj13 = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, 414, 264);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe8eef2), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                                    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        objects.obj14 = make_tab_header_bar(parent_obj, ui_lang_str(STR_TAB_F4_HDR), LV_SYMBOL_WIFI, 0x60a5fa);
                                        lv_obj_add_event_cb(objects.obj14, tooltip_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)GUIDE_ID_TAB_F4);
                                        lv_obj_add_event_cb(objects.obj14, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F4);
                                        lv_obj_add_event_cb(objects.tab_f4_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F4);
                                        {
                                            // ui_scan_list — Apple Inset Grouped Card
                                            lv_obj_t *obj = lv_list_create(parent_obj);
                                            objects.ui_scan_list = obj;
                                            lv_obj_set_pos(obj, 8, 38);
                                            lv_obj_set_size(obj, 398, 126);
                                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_all(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_row(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
                                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);

                                            lv_obj_t *init_lbl = lv_label_create(obj);
                                            lv_obj_set_width(init_lbl, LV_PCT(100));
                                            lv_label_set_long_mode(init_lbl, LV_LABEL_LONG_WRAP);
                                            lv_label_set_text(init_lbl, "جهت جستجو و مشاهده شبکه‌های وای‌فای،\nدکمه [جستجو] را لمس کنید.");
                                            lv_obj_set_style_text_align(init_lbl, LV_TEXT_ALIGN_CENTER, 0);
                                            lv_obj_set_style_text_font(init_lbl, ui_lang_font_16(), 0);
                                            lv_obj_set_style_text_color(init_lbl, lv_color_hex(0x64748b), 0);
                                            lv_obj_set_style_base_dir(init_lbl, ui_lang_base_dir(), 0);
                                            lv_obj_set_style_pad_ver(init_lbl, 14, 0);
                                        }
                                        {
                                            // scan_wifi_btn — Apple Secondary Pill Button (h=40px)
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.scan_wifi_btn = obj;
                                            lv_obj_set_pos(obj, 8, 170);
                                            lv_obj_set_size(obj, 194, 40);
                                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, action_scan_wifi_start, LV_EVENT_CLICKED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.wifi_scan_lbl = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_label_set_text(obj, ui_lang_str(STR_WIFI_SCAN));
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_base_dir(obj, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                            }
                                        }
                                        {
                                            // connect_wifi_btn — Apple Primary Action Button (h=40px)
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.connect_wifi_btn = obj;
                                            lv_obj_set_pos(obj, 212, 170);
                                            lv_obj_set_size(obj, 194, 40);
                                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, action_connect_btn_act, LV_EVENT_CLICKED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.wifi_connect_lbl = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_label_set_text(obj, ui_lang_str(STR_WIFI_CONNECT));
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_base_dir(obj, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                            }
                                        }
                                        {
                                            // btn_goto_framware — Apple Dark Banner (Zero Tofu, h=42px)
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.btn_goto_framware = obj;
                                            lv_obj_set_pos(obj, 8, 216);
                                            lv_obj_set_size(obj, 398, 42);
                                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, action_go_framware, LV_EVENT_CLICKED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.wifi_ota_lbl = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_label_set_text(obj, ui_lang_str(STR_OTA_UPDATE_BTN));
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_base_dir(obj, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                            }
                                        }
                                        /* ساخت پنل مودال کیبورد و پسورد وای‌فای به سبک اپل با پس‌زمینه دارک مات */
                                        {
                                            lv_obj_t *overlay = lv_obj_create(lv_layer_top());
                                            objects.wifi_kb_overlay = overlay;
                                            lv_obj_set_pos(overlay, 0, 0);
                                            lv_obj_set_size(overlay, 480, 320);
                                            lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
                                            lv_obj_set_style_bg_opa(overlay, 180, 0);
                                            lv_obj_set_style_border_width(overlay, 0, 0);
                                            lv_obj_set_style_pad_all(overlay, 0, 0);
                                            lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
                                            lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);

                                            /* باکس ورودی پسورد در بالای صفحه */
                                            lv_obj_t *top_card = lv_obj_create(overlay);
                                            lv_obj_set_pos(top_card, 16, 10);
                                            lv_obj_set_size(top_card, 448, 86);
                                            lv_obj_set_style_bg_color(top_card, lv_color_hex(0xffffff), 0);
                                            lv_obj_set_style_radius(top_card, 10, 0);
                                            lv_obj_set_style_border_width(top_card, 1, 0);
                                            lv_obj_set_style_border_color(top_card, lv_color_hex(0xcbd5e1), 0);
                                            lv_obj_set_style_pad_all(top_card, 6, 0);
                                            lv_obj_clear_flag(top_card, LV_OBJ_FLAG_SCROLLABLE);

                                            lv_obj_t *title = lv_label_create(top_card);
                                            objects.wifi_pass_title_lbl = title;
                                            lv_label_set_text(title, ui_lang_str(STR_WIFI_PASS_TITLE));
                                            lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
                                            lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);
                                            lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
                                            if (ui_lang_get() == UI_LANG_FA) {
                                                lv_obj_align(title, LV_ALIGN_TOP_RIGHT, -4, 2);
                                            } else {
                                                lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 2);
                                            }

                                             /* فیلد ورودی متن رمز */
                                             objects.wifi_pass_txtarea = lv_textarea_create(top_card);
                                             lv_obj_set_pos(objects.wifi_pass_txtarea, 6, 30);
                                             lv_obj_set_size(objects.wifi_pass_txtarea, 248, 44);
                                             lv_textarea_set_max_length(objects.wifi_pass_txtarea, 64);
                                             lv_textarea_set_one_line(objects.wifi_pass_txtarea, true);
                                             lv_textarea_set_password_mode(objects.wifi_pass_txtarea, false);
                                             lv_textarea_set_placeholder_text(objects.wifi_pass_txtarea, ui_lang_str(STR_WIFI_PASS_PLACEHOLDER));
                                             lv_obj_set_style_text_font(objects.wifi_pass_txtarea, &lv_font_montserrat_16, 0);
                                             lv_obj_set_style_text_font(objects.wifi_pass_txtarea, &lv_font_montserrat_16, LV_PART_TEXTAREA_PLACEHOLDER);
                                             lv_obj_set_style_base_dir(objects.wifi_pass_txtarea, LV_BASE_DIR_LTR, LV_PART_TEXTAREA_PLACEHOLDER);
                                             lv_obj_set_style_base_dir(objects.wifi_pass_txtarea, LV_BASE_DIR_LTR, 0);
                                             lv_obj_set_style_bg_color(objects.wifi_pass_txtarea, lv_color_hex(0xf8fafc), 0);
                                             lv_obj_set_style_border_color(objects.wifi_pass_txtarea, lv_color_hex(0x0284c7), 0);
                                             lv_obj_set_style_border_width(objects.wifi_pass_txtarea, 2, 0);
                                             lv_obj_set_style_radius(objects.wifi_pass_txtarea, 6, 0);

                                             /* کلید چشم جهت نمایش/مخفی‌سازی رمز عبور در محیط کارگاهی */
                                             objects.wifi_pass_eye_btn = lv_btn_create(top_card);
                                             lv_obj_set_pos(objects.wifi_pass_eye_btn, 260, 30);
                                             lv_obj_set_size(objects.wifi_pass_eye_btn, 44, 44);
                                             lv_obj_set_ext_click_area(objects.wifi_pass_eye_btn, 6);
                                             lv_obj_set_style_radius(objects.wifi_pass_eye_btn, 6, 0);
                                             lv_obj_set_style_bg_color(objects.wifi_pass_eye_btn, lv_color_hex(0xf1f5f9), 0);
                                             lv_obj_set_style_border_color(objects.wifi_pass_eye_btn, lv_color_hex(0xcbd5e1), 0);
                                             lv_obj_set_style_border_width(objects.wifi_pass_eye_btn, 1, 0);
                                             lv_obj_set_style_shadow_width(objects.wifi_pass_eye_btn, 0, 0);
                                             lv_obj_add_event_cb(objects.wifi_pass_eye_btn, action_wifi_eye_toggle_cb, LV_EVENT_CLICKED, NULL);
                                             lv_obj_t *lbl_eye = lv_label_create(objects.wifi_pass_eye_btn);
                                             objects.wifi_pass_eye_lbl = lbl_eye;
                                             lv_label_set_text(lbl_eye, LV_SYMBOL_EYE_OPEN);
                                             lv_obj_set_style_text_font(lbl_eye, &lv_font_montserrat_16, 0);
                                             lv_obj_set_style_text_color(lbl_eye, lv_color_hex(0x0f172a), 0);
                                             lv_obj_center(lbl_eye);

                                             /* کلید اتصال مستقیم در بالای مودال */
                                             objects.wifi_pass_connect_btn = lv_btn_create(top_card);
                                             lv_obj_set_pos(objects.wifi_pass_connect_btn, 310, 30);
                                             lv_obj_set_size(objects.wifi_pass_connect_btn, 80, 44);
                                             lv_obj_set_ext_click_area(objects.wifi_pass_connect_btn, 6);
                                             lv_obj_set_style_radius(objects.wifi_pass_connect_btn, 6, 0);
                                             lv_obj_set_style_bg_color(objects.wifi_pass_connect_btn, lv_color_hex(0x0284c7), 0);
                                             lv_obj_set_style_shadow_width(objects.wifi_pass_connect_btn, 0, 0);
                                             lv_obj_add_event_cb(objects.wifi_pass_connect_btn, action_connect_btn_act, LV_EVENT_CLICKED, NULL);
                                             lv_obj_t *lbl_conn = lv_label_create(objects.wifi_pass_connect_btn);
                                             lv_label_set_text(lbl_conn, ui_lang_str(STR_WIFI_CONNECT));
                                             lv_obj_set_style_text_font(lbl_conn, ui_lang_font_16(), 0);
                                             lv_obj_set_style_text_color(lbl_conn, lv_color_hex(0xffffff), 0);
                                             lv_obj_set_style_base_dir(lbl_conn, ui_lang_base_dir(), 0);
                                             lv_obj_center(lbl_conn);

                                             /* دکمه انصراف و بستن */
                                             objects.wifi_pass_cancel_btn = lv_btn_create(top_card);
                                             lv_obj_set_pos(objects.wifi_pass_cancel_btn, 396, 30);
                                             lv_obj_set_size(objects.wifi_pass_cancel_btn, 42, 44);
                                             lv_obj_set_ext_click_area(objects.wifi_pass_cancel_btn, 6);
                                             lv_obj_set_style_radius(objects.wifi_pass_cancel_btn, 6, 0);
                                             lv_obj_set_style_bg_color(objects.wifi_pass_cancel_btn, lv_color_hex(0xdc2626), 0);
                                             lv_obj_set_style_shadow_width(objects.wifi_pass_cancel_btn, 0, 0);
                                             lv_obj_add_event_cb(objects.wifi_pass_cancel_btn, wifi_cancel_btn_cb, LV_EVENT_CLICKED, NULL);
                                             lv_obj_t *lbl_x = lv_label_create(objects.wifi_pass_cancel_btn);
                                             lv_label_set_text(lbl_x, LV_SYMBOL_CLOSE);
                                             lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_16, 0);
                                             lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xffffff), 0);
                                             lv_obj_center(lbl_x);

                                            /* کیبورد کامل در پایین مودال با سایز استاندارد ۲۱۰ پیکسل */
                                            objects.wifi_pass_key = lv_keyboard_create(overlay);
                                            lv_obj_align(objects.wifi_pass_key, LV_ALIGN_BOTTOM_MID, 0, -6);
                                            lv_obj_set_size(objects.wifi_pass_key, 464, 210);
                                            lv_keyboard_set_map(objects.wifi_pass_key, LV_KEYBOARD_MODE_TEXT_LOWER, s_wifi_kb_map_lc, s_wifi_kb_ctrl_lc);
                                            lv_keyboard_set_map(objects.wifi_pass_key, LV_KEYBOARD_MODE_TEXT_UPPER, s_wifi_kb_map_uc, s_wifi_kb_ctrl_uc);
                                            lv_keyboard_set_map(objects.wifi_pass_key, LV_KEYBOARD_MODE_SPECIAL, s_wifi_kb_map_sym, s_wifi_kb_ctrl_sym);
                                            lv_keyboard_set_mode(objects.wifi_pass_key, LV_KEYBOARD_MODE_TEXT_LOWER);
                                            lv_obj_add_event_cb(objects.wifi_pass_key, wifi_keyboard_custom_cb, LV_EVENT_ALL, NULL);
                                            lv_obj_add_event_cb(objects.wifi_pass_key, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
                                            lv_obj_add_flag(objects.wifi_pass_key, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
                                            lv_obj_clear_flag(objects.wifi_pass_key, LV_OBJ_FLAG_SCROLLABLE);
                                            lv_obj_set_style_radius(objects.wifi_pass_key, 8, 0);
                                        }
                                        {
                                            // wifi_status_messagebox (overlay on lv_layer_top for global alert visibility)
                                            lv_obj_t *obj = lv_obj_create(lv_layer_top());
                                            objects.wifi_status_messagebox = obj;
                                            lv_obj_set_pos(obj, 90, 80);
                                            lv_obj_set_size(obj, 300, LV_SIZE_CONTENT);
                                            lv_obj_add_event_cb(obj, action_wifi_status_act, LV_EVENT_CLICKED, (void *)0);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, 245, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        }
                                    }
                                }
                            }
                        }
                        {
                            // tab_f5_btn — تست و دیاگ ورودی و خروجی‌ها (Tab Index 4)
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F5");
                            objects.tab_f5_btn = obj;
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *card = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(card, 0, 0);
                                    lv_obj_set_size(card, 414, 264);
                                    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(card, lv_color_hex(0xffe8eef2), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_scroll_dir(card, LV_DIR_NONE);
                                    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

                                    lv_obj_t *hdr_f5 = make_tab_header_bar(card, ui_lang_str(STR_TAB_F5_HDR), LV_SYMBOL_CHARGE, 0xeab308);
                                    objects.obj17 = hdr_f5;
                                    lv_obj_add_event_cb(hdr_f5, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F5);
                                    lv_obj_add_event_cb(objects.tab_f5_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F5);

                                     /* سمت چپ: نمایشگر زنده ورودی‌ها (سنسورها و ترمینال‌های ۱۰ گانه) */
                                     lv_obj_t *in_panel = lv_obj_create(card);
                                     lv_obj_set_pos(in_panel, 8, 36);
                                     lv_obj_set_size(in_panel, 196, 188);
                                     lv_obj_set_style_pad_all(in_panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                     lv_obj_set_style_border_width(in_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                     lv_obj_set_style_border_color(in_panel, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                     lv_obj_set_style_radius(in_panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                     lv_obj_set_style_bg_color(in_panel, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                     lv_obj_clear_flag(in_panel, LV_OBJ_FLAG_SCROLLABLE);

                                      /* تیتر ورودی‌ها */
                                      lv_obj_t *in_title = lv_label_create(in_panel);
                                      objects.f6_in_title_lbl = in_title;
                                      lv_label_set_text(in_title, ui_lang_str(STR_F6_INPUTS_HDR));
                                      lv_obj_set_style_text_font(in_title, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                      lv_obj_set_style_text_color(in_title, lv_color_hex(0x1e3a8a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                      lv_obj_set_style_base_dir(in_title, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                      lv_obj_set_style_text_align(in_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                      lv_obj_align(in_title, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -3 : 3);

                                         static const char *s_term_disp_names[8] = {
                                             "I0", "I1", "I2", "I3", "I4", "I5", "I6", "I7"
                                         };

                                         for (int i = 0; i < 8; i++) {
                                             int32_t iy = 23 + (i * 20);
                                              lv_obj_t *row = lv_obj_create(in_panel);
                                              objects.row_in[i] = row;
                                              lv_obj_set_pos(row, 2, iy);
                                              lv_obj_set_size(row, 190, 18);
                                              lv_obj_set_style_bg_opa(row, 0, 0);
                                              lv_obj_set_style_border_width(row, 0, 0);
                                              lv_obj_set_style_pad_all(row, 0, 0);
                                              if (i == 3 || i == 5) {
                                                  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
                                                  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
                                                  bool is_m = (i == 3);
                                                  lv_obj_add_event_cb(row, overload_row_press_cb, LV_EVENT_ALL, (void *)(intptr_t)is_m);
                                              } else {
                                                  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                              }

                                             lv_obj_t *led = lv_led_create(row);
                                             objects.led_in[i] = led;
                                             lv_obj_set_pos(led, 6, 4);
                                             lv_obj_set_size(led, 10, 10);
                                             lv_led_set_color(led, lv_color_hex(0x22c55e));
                                             lv_led_off(led);
                                             lv_obj_set_style_border_color(led, lv_color_hex(0x64748b), 0);
                                             lv_obj_set_style_border_width(led, 1, 0);
                                             lv_obj_clear_flag(led, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

                                             /* نماد ترمینال */
                                             ui_create_terminal_tag(row, 22, 0, s_term_disp_names[i]);

                                             /* خط‌چین راهنما میان تگ و عنوان سنسور */
                                             lv_obj_t *dots = lv_label_create(row);
                                             lv_label_set_text(dots, ". . . . . . . . . . . . . . . . . .");
                                             lv_label_set_long_mode(dots, LV_LABEL_LONG_CLIP);
                                             lv_obj_set_style_text_font(dots, &lv_font_montserrat_14, 0);
                                             lv_obj_set_style_text_color(dots, lv_color_hex(0x94a3b8), 0);
                                             lv_obj_set_style_base_dir(dots, LV_BASE_DIR_LTR, 0);
                                             lv_obj_set_style_text_align(dots, LV_TEXT_ALIGN_LEFT, 0);
                                             lv_obj_set_pos(dots, 44, -2);
                                             lv_obj_set_size(dots, 142, 16);
                                             lv_obj_clear_flag(dots, LV_OBJ_FLAG_CLICKABLE);

                                             /* برچسب نام نقش - با پس‌زمینه سفید مات جهت ماسک کامل خط‌چین در زیر حروف */
                                              lv_obj_t *lbl = lv_label_create(row);
                                              objects.lbl_in[i] = lbl;
                                              lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_width(lbl, LV_SIZE_CONTENT);
                                             lv_obj_set_style_max_width(lbl, 142, 0);
                                             lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
                                             lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
                                             lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);
                                             lv_obj_set_style_bg_color(lbl, lv_color_hex(0xffffff), 0);
                                             lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
                                             lv_obj_set_style_pad_hor(lbl, 4, 0);
                                             lv_obj_set_style_pad_ver(lbl, 0, 0);

                                             int8_t r = pinmap_get_role_for_terminal((uint8_t)i);
                                             if (r >= 0) {
                                                 lv_label_set_text(lbl, pinmap_get_role_name((pinmap_role_t)r));
                                                 lv_obj_set_style_text_color(lbl, lv_color_hex(0x0f172a), 0);
                                             } else {
                                                 lv_label_set_text(lbl, "");
                                             }
                                             lv_obj_update_layout(lbl);
                                             lv_obj_align(lbl, LV_ALIGN_RIGHT_MID, -4, 0);
                                         }

                                        objects.led_in_eye   = objects.led_in[0];
                                        objects.led_in_rear  = objects.led_in[2];
                                        objects.led_in_front = objects.led_in[4];
                                        objects.led_in_loadm = objects.led_in[3];
                                        objects.led_in_loadc = objects.led_in[5];
                                        objects.led_in_stop  = objects.led_in[6];
                                        objects.led_in_start = objects.led_in[7];

                                       /* سمت راست: کلیدهای تست دستی رله‌ها (خروجی) */
                                       lv_obj_t *out_panel = lv_obj_create(card);
                                       lv_obj_set_pos(out_panel, 210, 36);
                                       lv_obj_set_size(out_panel, 196, 188);
                                       lv_obj_set_style_pad_all(out_panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_border_width(out_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_border_color(out_panel, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_radius(out_panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_bg_color(out_panel, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_clear_flag(out_panel, LV_OBJ_FLAG_SCROLLABLE);

                                       lv_obj_t *out_title = lv_label_create(out_panel);
                                       objects.f6_out_title_lbl = out_title;
                                       lv_label_set_text(out_title, ui_lang_str(STR_F6_OUTPUTS_HDR));
                                       lv_obj_set_style_text_font(out_title, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_text_color(out_title, lv_color_hex(0x1e3a8a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_base_dir(out_title, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_set_style_text_align(out_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                       lv_obj_align(out_title, LV_ALIGN_TOP_MID, 0, (ui_lang_get() == UI_LANG_FA) ? -3 : 3);

                                         static const struct {
                                             ui_str_key_t key;
                                             const char *tag;
                                             gpio_num_t pin;
                                             lv_obj_t **btn_ptr;
                                         } relay_tests[8] = {
                                             {STR_F6_OUT_COMP,    "Q0", GPIO_NUM_16,       &objects.btn_test_comp},
                                             {STR_F6_OUT_PROD,    "Q1", GPIO_NUM_42,       &objects.btn_test_make},
                                             {STR_F6_OUT_MIXER,   "Q2", GPIO_NUM_39,       &objects.btn_test_pdl},
                                             {STR_F6_OUT_EDGE,    "Q3", CREAM_PUMP_GPIO,   &objects.btn_test_cream},
                                             {STR_F6_OUT_RAINBOW, "Q4", RAINBOW_GPIO,      &objects.btn_test_rb},
                                             {STR_F6_OUT_AIR,     "Q5", AIR_SOLENOID_GPIO, &objects.btn_test_air},
                                             {STR_F6_OUT_RESET,   "Q6", RESET_RELAY_GPIO,  &objects.btn_test_rst},
                                             {STR_F6_OUT_WASH,    "Q7", WASH_PUMP_GPIO,    &objects.btn_test_wash}
                                         };

                                        for (int i = 0; i < 8; i++) {
                                            int32_t oy = 20 + (i * 20);
                                            lv_obj_t *rbtn = lv_btn_create(out_panel);
                                            *relay_tests[i].btn_ptr = rbtn;
                                            lv_obj_set_pos(rbtn, 6, oy);
                                            lv_obj_set_size(rbtn, 182, 18);
                                          lv_obj_set_ext_click_area(rbtn, 4);
                                          lv_obj_set_style_radius(rbtn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

                                          bool is_on = (gpio_get_level(relay_tests[i].pin) == 0);
                                          if (is_on) {
                                              lv_obj_set_style_bg_color(rbtn, lv_color_hex(0x16a34a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                              lv_obj_set_style_border_color(rbtn, lv_color_hex(0x86efac), LV_PART_MAIN | LV_STATE_DEFAULT);
                                              lv_obj_set_style_border_width(rbtn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                          } else {
                                              lv_obj_set_style_bg_color(rbtn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                              lv_obj_set_style_border_color(rbtn, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                              lv_obj_set_style_border_width(rbtn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                          }
                                          lv_obj_set_style_shadow_width(rbtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_set_style_pad_all(rbtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_add_event_cb(rbtn, action_io_relay_toggle, LV_EVENT_CLICKED, (void *)(intptr_t)relay_tests[i].pin);

                                          /* برچسب کد PLC در سمت چپ با فونت انگلیسی مینی‌مال */
                                          lv_obj_t *tag_lbl = lv_label_create(rbtn);
                                          lv_label_set_text(tag_lbl, relay_tests[i].tag);
                                          lv_obj_set_style_text_font(tag_lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_set_style_text_color(tag_lbl, is_on ? lv_color_hex(0xffffff) : lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_align(tag_lbl, LV_ALIGN_LEFT_MID, 6, 0);

                                          /* برچسب نام رله در سمت راست */
                                          lv_obj_t *rlbl = lv_label_create(rbtn);
                                          objects.f6_out_lbls[i] = rlbl;
                                          lv_label_set_text(rlbl, ui_lang_str(relay_tests[i].key));
                                          lv_obj_set_style_text_font(rlbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_set_style_text_color(rlbl, is_on ? lv_color_hex(0xffffff) : lv_color_hex(0x0f172a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_set_style_base_dir(rlbl, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                          lv_obj_align(rlbl, LV_ALIGN_RIGHT_MID, -6, 0);
                                      }

                                    /* نمایشگر زنده سنسورهای دمای MAX6675 در انتهای تب دیاگ F6 - کاملاً بالای تولبار */
                                    lv_obj_t *t1_card = lv_obj_create(card);
                                    lv_obj_set_pos(t1_card, 8, 228);
                                    lv_obj_set_size(t1_card, 196, 34);
                                    lv_obj_set_style_pad_all(t1_card, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(t1_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(t1_card, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(t1_card, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(t1_card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(t1_card, LV_OBJ_FLAG_SCROLLABLE);
                                    lv_obj_add_flag(t1_card, LV_OBJ_FLAG_CLICKABLE);
                                    lv_obj_add_event_cb(t1_card, on_temp_hopper_toggle_click, LV_EVENT_CLICKED, NULL);
                                    lv_obj_add_event_cb(t1_card, temp_card_press_cb, LV_EVENT_ALL, (void *)(intptr_t)1);

                                    lv_obj_t *t1_title = lv_label_create(t1_card);
                                    objects.diag_t1_lbl = t1_title;
                                    lv_label_set_text(t1_title, ui_lang_str(STR_F6_TEMP_T1));
                                    lv_obj_set_style_text_font(t1_title, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(t1_title, lv_color_hex(0x1e3a8a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_base_dir(t1_title, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);

                                    objects.diag_temp_hopper_val = lv_label_create(t1_card);
                                    lv_label_set_text(objects.diag_temp_hopper_val, "--.-°C");
                                    lv_obj_set_style_text_font(objects.diag_temp_hopper_val, &ui_font_yekan_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(objects.diag_temp_hopper_val, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_base_dir(objects.diag_temp_hopper_val, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);

                                    if (ui_lang_get() == UI_LANG_FA) {
                                        lv_obj_align(t1_title, LV_ALIGN_RIGHT_MID, -6, 0);
                                        lv_obj_align(objects.diag_temp_hopper_val, LV_ALIGN_LEFT_MID, 6, 0);
                                    } else {
                                        lv_obj_align(t1_title, LV_ALIGN_LEFT_MID, 6, 0);
                                        lv_obj_align(objects.diag_temp_hopper_val, LV_ALIGN_RIGHT_MID, -6, 0);
                                    }

                                    lv_obj_t *t2_card = lv_obj_create(card);
                                    lv_obj_set_pos(t2_card, 210, 228);
                                    lv_obj_set_size(t2_card, 196, 34);
                                    lv_obj_set_style_pad_all(t2_card, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(t2_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(t2_card, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(t2_card, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(t2_card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(t2_card, LV_OBJ_FLAG_SCROLLABLE);
                                    lv_obj_add_flag(t2_card, LV_OBJ_FLAG_CLICKABLE);
                                    lv_obj_add_event_cb(t2_card, temp_card_press_cb, LV_EVENT_ALL, (void *)(intptr_t)2);

                                    lv_obj_t *t2_title = lv_label_create(t2_card);
                                    objects.diag_t2_lbl = t2_title;
                                    lv_label_set_text(t2_title, ui_lang_str(STR_F6_TEMP_T2));
                                    lv_obj_set_style_text_font(t2_title, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(t2_title, lv_color_hex(0x1e3a8a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_base_dir(t2_title, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);

                                    objects.diag_temp_cyl_val = lv_label_create(t2_card);
                                    lv_label_set_text(objects.diag_temp_cyl_val, "--.-°C");
                                    lv_obj_set_style_text_font(objects.diag_temp_cyl_val, &ui_font_yekan_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(objects.diag_temp_cyl_val, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_base_dir(objects.diag_temp_cyl_val, LV_BASE_DIR_LTR, LV_PART_MAIN | LV_STATE_DEFAULT);

                                    if (ui_lang_get() == UI_LANG_FA) {
                                        lv_obj_align(t2_title, LV_ALIGN_RIGHT_MID, -6, 0);
                                        lv_obj_align(objects.diag_temp_cyl_val, LV_ALIGN_LEFT_MID, 6, 0);
                                    } else {
                                        lv_obj_align(t2_title, LV_ALIGN_LEFT_MID, 6, 0);
                                        lv_obj_align(objects.diag_temp_cyl_val, LV_ALIGN_RIGHT_MID, -6, 0);
                                    }
                                }
                            }
                        }
                        {
                            // tab_f6_btn — فرآیند، فروش و ایمنی (Tab Index 5)
                            lv_obj_t *tab_f6 = lv_tabview_add_tab(parent_obj, "F6");
                            objects.tab_f6_btn = tab_f6;
                            objects.tab_f7_btn = tab_f6;
                            lv_obj_add_event_cb(tab_f6, action_f6_act, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_pad_all(tab_f6, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(tab_f6, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_scroll_dir(tab_f6, LV_DIR_NONE);
                            lv_obj_set_scrollbar_mode(tab_f6, LV_SCROLLBAR_MODE_OFF);
                            {
                                lv_obj_t *parent_obj = tab_f6;
                                {
                                    lv_obj_t *card = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(card, 0, 0);
                                    lv_obj_set_size(card, 414, 264);
                                    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(card, lv_color_hex(0xffe8eef2), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_clear_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_scroll_dir(card, LV_DIR_NONE);
                                    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

                                    lv_obj_t *hdr_f6 = make_tab_header_bar(card, ui_lang_str(STR_TAB_F6_HDR), LV_SYMBOL_WARNING, 0xf97316);
                                    objects.hdr_f6_lbl = hdr_f6;
                                    objects.hdr_f7_lbl = hdr_f6;
                                    lv_obj_add_event_cb(hdr_f6, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F6);
                                    lv_obj_add_event_cb(objects.tab_f6_btn, tooltip_long_press_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)GUIDE_ID_TAB_F6);

                                     /* ستون چپ: کارت‌های ۵ گانه تنظیمات فرآیند و فروش (x=8, w=238 با تراز متقارن ۷ پیکسلی بالا و پایین) */
                                     ui_create_stepper_card(card, 8, 41, 238, 40, 22, ui_lang_str(STR_TMR_DSP_RUN), "45", &objects.val_dsp_run);
                                     ui_create_stepper_card(card, 8, 85, 238, 40, 23, ui_lang_str(STR_TMR_FRT_HOLD), "6", &objects.val_frt_hold);
                                     ui_create_stepper_card(card, 8, 129, 238, 40, 24, ui_lang_str(STR_CNT_SCP_COMP), "3", &objects.val_scp_comp);
                                     ui_create_stepper_card(card, 8, 173, 238, 40, 25, ui_lang_str(STR_TMR_FRZ_TO), "15", &objects.val_frz_to);
                                     ui_create_stepper_card(card, 8, 217, 238, 40, 26, ui_lang_str(STR_TMR_LEV_MAX), "25", &objects.val_lev_max);

                                     /* ستون راست: کارت لایت تایمرهای ثابت قفل سخت‌افزار (x=248, w=158 تراز متقارن به ۴۱ و ۲۵۷) */
                                     {
                                         lv_obj_t *locked_card = lv_obj_create(card);
                                         objects.card_locked_timers = locked_card;
                                         lv_obj_set_pos(locked_card, 248, 41);
                                         lv_obj_set_size(locked_card, 158, 216);
                                         lv_obj_set_style_bg_color(locked_card, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_bg_opa(locked_card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_border_width(locked_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_border_color(locked_card, lv_color_hex(0xcbd5e1), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_radius(locked_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_pad_all(locked_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_clear_flag(locked_card, LV_OBJ_FLAG_SCROLLABLE);

                                         /* ۱. بج عنوان قفل سخت‌افزار (سبک لایت اپل) با تراز دقیق ریاضی و بدون بریدگی */
                                         lv_obj_t *badge = lv_obj_create(locked_card);
                                         lv_obj_set_pos(badge, 8, 6);
                                         lv_obj_set_size(badge, 142, 24);
                                         lv_obj_set_style_bg_color(badge, lv_color_hex(0xfef2f2), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_border_color(badge, lv_color_hex(0xfca5a5), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_radius(badge, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

                                         lv_obj_t *badge_lbl = lv_label_create(badge);
                                         objects.lbl_locked_badge = badge_lbl;
                                         lv_label_set_text(badge_lbl, ui_lang_str(STR_LOCKED_BADGE));
                                         lv_obj_set_style_text_font(badge_lbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0xdc2626), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_base_dir(badge_lbl, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_center(badge_lbl);

                                         /* تیتر توضیحی */
                                         lv_obj_t *l_hdr = lv_label_create(locked_card);
                                         objects.lbl_locked_hdr = l_hdr;
                                         lv_obj_set_pos(l_hdr, 6, 31);
                                         lv_obj_set_size(l_hdr, 146, LV_SIZE_CONTENT);
                                         lv_label_set_text(l_hdr, ui_lang_str(STR_LOCKED_TIMERS_HDR));
                                         lv_obj_set_style_text_font(l_hdr, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_text_color(l_hdr, lv_color_hex(0x64748b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_base_dir(l_hdr, ui_lang_base_dir(), LV_PART_MAIN | LV_STATE_DEFAULT);
                                         lv_obj_set_style_text_align(l_hdr, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

                                         /* ۴ ردیف آیتم‌های قفل‌شده ایمنی با تراز متقارن و حاشیه تنفس از لبه راست */
                                         // ردیف ۰: تاخیر کمپرسور (10s)
                                         {
                                             lv_obj_t *row0 = lv_obj_create(locked_card);
                                             lv_obj_set_pos(row0, 3, 55);
                                             lv_obj_set_size(row0, 152, 21);
                                             lv_obj_set_style_bg_opa(row0, 0, 0);
                                             lv_obj_set_style_border_width(row0, 0, 0);
                                             lv_obj_set_style_pad_all(row0, 0, 0);
                                             lv_obj_clear_flag(row0, LV_OBJ_FLAG_SCROLLABLE);
                                             lv_obj_add_flag(row0, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_style_radius(row0, 6, 0);
                                             lv_obj_set_style_bg_color(row0, lv_color_hex(0xf1f5f9), LV_STATE_PRESSED);
                                             lv_obj_set_style_bg_opa(row0, 255, LV_STATE_PRESSED);
                                             lv_obj_add_event_cb(row0, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_COOLDOWN);

                                             lv_obj_t *val0 = lv_label_create(row0);
                                             s_val_fixed_cd = val0;
                                             lv_label_set_text(val0, "10s");
                                             lv_obj_set_style_text_font(val0, &lv_font_montserrat_14, 0);
                                             lv_obj_set_style_text_color(val0, lv_color_hex(0x0284c7), 0);

                                             lv_obj_t *it0 = lv_label_create(row0);
                                             objects.lbl_fixed_cd = it0;
                                             lv_label_set_text(it0, ui_lang_str(STR_FIXED_COOLDOWN));
                                             lv_obj_set_style_text_font(it0, ui_lang_font_16(), 0);
                                             lv_obj_set_style_text_color(it0, lv_color_hex(0x1e293b), 0);
                                             lv_obj_set_style_text_color(it0, lv_color_hex(0x0284c7), LV_STATE_PRESSED);
                                             lv_obj_set_style_base_dir(it0, ui_lang_base_dir(), 0);
                                             if (ui_lang_get() == UI_LANG_FA) {
                                                 lv_obj_align(it0, LV_ALIGN_RIGHT_MID, -6, 0);
                                                 lv_obj_align(val0, LV_ALIGN_LEFT_MID, 6, 0);
                                             } else {
                                                 lv_obj_align(it0, LV_ALIGN_LEFT_MID, 6, 0);
                                                 lv_obj_align(val0, LV_ALIGN_RIGHT_MID, -6, 0);
                                             }
                                             lv_obj_add_flag(it0, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_ext_click_area(it0, 6);
                                             lv_obj_add_event_cb(it0, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_COOLDOWN);
                                         }

                                         // ردیف ۱: پالس ریست مدار (2s)
                                         {
                                             lv_obj_t *row1 = lv_obj_create(locked_card);
                                             lv_obj_set_pos(row1, 3, 78);
                                             lv_obj_set_size(row1, 152, 21);
                                             lv_obj_set_style_bg_opa(row1, 0, 0);
                                             lv_obj_set_style_border_width(row1, 0, 0);
                                             lv_obj_set_style_pad_all(row1, 0, 0);
                                             lv_obj_clear_flag(row1, LV_OBJ_FLAG_SCROLLABLE);
                                             lv_obj_add_flag(row1, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_style_radius(row1, 6, 0);
                                             lv_obj_set_style_bg_color(row1, lv_color_hex(0xf1f5f9), LV_STATE_PRESSED);
                                             lv_obj_set_style_bg_opa(row1, 255, LV_STATE_PRESSED);
                                             lv_obj_add_event_cb(row1, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_RESET_PULSE);

                                             lv_obj_t *val1 = lv_label_create(row1);
                                             s_val_fixed_rst = val1;
                                             lv_label_set_text(val1, "2s");
                                             lv_obj_set_style_text_font(val1, &lv_font_montserrat_14, 0);
                                             lv_obj_set_style_text_color(val1, lv_color_hex(0x475569), 0);

                                             lv_obj_t *it1 = lv_label_create(row1);
                                             objects.lbl_fixed_rst = it1;
                                             lv_label_set_text(it1, ui_lang_str(STR_FIXED_RESET_PULSE));
                                             lv_obj_set_style_text_font(it1, ui_lang_font_16(), 0);
                                             lv_obj_set_style_text_color(it1, lv_color_hex(0x1e293b), 0);
                                             lv_obj_set_style_text_color(it1, lv_color_hex(0x0284c7), LV_STATE_PRESSED);
                                             lv_obj_set_style_base_dir(it1, ui_lang_base_dir(), 0);
                                             if (ui_lang_get() == UI_LANG_FA) {
                                                 lv_obj_align(it1, LV_ALIGN_RIGHT_MID, -6, 0);
                                                 lv_obj_align(val1, LV_ALIGN_LEFT_MID, 6, 0);
                                             } else {
                                                 lv_obj_align(it1, LV_ALIGN_LEFT_MID, 6, 0);
                                                 lv_obj_align(val1, LV_ALIGN_RIGHT_MID, -6, 0);
                                             }
                                             lv_obj_add_flag(it1, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_ext_click_area(it1, 6);
                                             lv_obj_add_event_cb(it1, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_RESET_PULSE);
                                         }

                                         // ردیف ۲: فیلتر نویز ورودی (70ms)
                                         {
                                             lv_obj_t *row2 = lv_obj_create(locked_card);
                                             lv_obj_set_pos(row2, 3, 101);
                                             lv_obj_set_size(row2, 152, 21);
                                             lv_obj_set_style_bg_opa(row2, 0, 0);
                                             lv_obj_set_style_border_width(row2, 0, 0);
                                             lv_obj_set_style_pad_all(row2, 0, 0);
                                             lv_obj_clear_flag(row2, LV_OBJ_FLAG_SCROLLABLE);
                                             lv_obj_add_flag(row2, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_style_radius(row2, 6, 0);
                                             lv_obj_set_style_bg_color(row2, lv_color_hex(0xf1f5f9), LV_STATE_PRESSED);
                                             lv_obj_set_style_bg_opa(row2, 255, LV_STATE_PRESSED);
                                             lv_obj_add_event_cb(row2, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_DEBOUNCE);

                                             lv_obj_t *val2 = lv_label_create(row2);
                                             s_val_fixed_deb = val2;
                                             lv_label_set_text(val2, "70ms");
                                             lv_obj_set_style_text_font(val2, &lv_font_montserrat_14, 0);
                                             lv_obj_set_style_text_color(val2, lv_color_hex(0x475569), 0);

                                             lv_obj_t *it2 = lv_label_create(row2);
                                             objects.lbl_fixed_deb = it2;
                                             lv_label_set_text(it2, ui_lang_str(STR_FIXED_DEBOUNCE));
                                             lv_obj_set_style_text_font(it2, ui_lang_font_16(), 0);
                                             lv_obj_set_style_text_color(it2, lv_color_hex(0x1e293b), 0);
                                             lv_obj_set_style_text_color(it2, lv_color_hex(0x0284c7), LV_STATE_PRESSED);
                                             lv_obj_set_style_base_dir(it2, ui_lang_base_dir(), 0);
                                             if (ui_lang_get() == UI_LANG_FA) {
                                                 lv_obj_align(it2, LV_ALIGN_RIGHT_MID, -6, 0);
                                                 lv_obj_align(val2, LV_ALIGN_LEFT_MID, 6, 0);
                                             } else {
                                                 lv_obj_align(it2, LV_ALIGN_LEFT_MID, 6, 0);
                                                 lv_obj_align(val2, LV_ALIGN_RIGHT_MID, -6, 0);
                                             }
                                             lv_obj_add_flag(it2, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_ext_click_area(it2, 6);
                                             lv_obj_add_event_cb(it2, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_DEBOUNCE);
                                         }

                                         // ردیف ۳: حد برودت انجماد سیلندر (-22°C)
                                         {
                                             lv_obj_t *row3 = lv_obj_create(locked_card);
                                             lv_obj_set_pos(row3, 3, 124);
                                             lv_obj_set_size(row3, 152, 21);
                                             lv_obj_set_style_bg_opa(row3, 0, 0);
                                             lv_obj_set_style_border_width(row3, 0, 0);
                                             lv_obj_set_style_pad_all(row3, 0, 0);
                                             lv_obj_clear_flag(row3, LV_OBJ_FLAG_SCROLLABLE);
                                             lv_obj_add_flag(row3, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_style_radius(row3, 6, 0);
                                             lv_obj_set_style_bg_color(row3, lv_color_hex(0xf1f5f9), LV_STATE_PRESSED);
                                             lv_obj_set_style_bg_opa(row3, 255, LV_STATE_PRESSED);
                                             lv_obj_add_event_cb(row3, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_CYL_LIMIT);

                                             lv_obj_t *val3 = lv_label_create(row3);
                                             s_val_fixed_cyl = val3;
                                             lv_label_set_text(val3, "-22°C");
                                             lv_obj_set_style_text_font(val3, &lv_font_montserrat_14, 0);
                                             lv_obj_set_style_text_color(val3, lv_color_hex(0xd97706), 0);

                                             lv_obj_t *it3 = lv_label_create(row3);
                                             objects.lbl_fixed_cyl = it3;
                                             lv_label_set_text(it3, ui_lang_str(STR_FIXED_CYL_LIMIT));
                                             lv_obj_set_style_text_font(it3, ui_lang_font_16(), 0);
                                             lv_obj_set_style_text_color(it3, lv_color_hex(0x1e293b), 0);
                                             lv_obj_set_style_text_color(it3, lv_color_hex(0x0284c7), LV_STATE_PRESSED);
                                             lv_obj_set_style_base_dir(it3, ui_lang_base_dir(), 0);
                                             if (ui_lang_get() == UI_LANG_FA) {
                                                 lv_obj_align(it3, LV_ALIGN_RIGHT_MID, -6, 0);
                                                 lv_obj_align(val3, LV_ALIGN_LEFT_MID, 6, 0);
                                             } else {
                                                 lv_obj_align(it3, LV_ALIGN_LEFT_MID, 6, 0);
                                                 lv_obj_align(val3, LV_ALIGN_RIGHT_MID, -6, 0);
                                             }
                                             lv_obj_add_flag(it3, LV_OBJ_FLAG_CLICKABLE);
                                             lv_obj_set_ext_click_area(it3, 6);
                                             lv_obj_add_event_cb(it3, action_locked_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)STR_FIXED_CYL_LIMIT);
                                         }

                                          // ردیف ۴: فعال‌سازی یا وضعیت گارانتی آستیاج
                                          {
                                              lv_obj_t *row4 = lv_obj_create(locked_card);
                                              lv_obj_set_pos(row4, 3, 149);
                                              lv_obj_set_size(row4, 152, 55);
                                              lv_obj_set_style_bg_opa(row4, 0, 0);
                                              lv_obj_set_style_border_width(row4, 0, 0);
                                              lv_obj_set_style_pad_all(row4, 0, 0);
                                              lv_obj_clear_flag(row4, LV_OBJ_FLAG_SCROLLABLE);

                                              /* کلید فعال‌سازی گارانتی */
                                              lv_obj_t *w_btn = lv_btn_create(row4);
                                              objects.btn_warr_activate = w_btn;
                                              lv_obj_set_pos(w_btn, 0, 4);
                                              lv_obj_set_size(w_btn, 152, 46);
                                              lv_obj_set_style_radius(w_btn, 8, 0);
                                              lv_obj_set_style_bg_color(w_btn, lv_color_hex(0x0284c7), 0);
                                              lv_obj_set_style_shadow_width(w_btn, 0, 0);
                                              lv_obj_add_event_cb(w_btn, action_open_warranty_details, LV_EVENT_CLICKED, NULL);

                                              lv_obj_t *w_lbl = lv_label_create(w_btn);
                                              objects.lbl_warr_activate_btn = w_lbl;
                                              lv_label_set_text(w_lbl, ui_lang_str(STR_WARR_ACTIVATE_BTN));
                                              lv_obj_set_style_text_font(w_lbl, ui_lang_font_16(), 0);
                                              lv_obj_set_style_text_color(w_lbl, lv_color_hex(0xffffff), 0);
                                              lv_obj_center(w_lbl);

                                              /* کادر وضعیت فعال بودن گارانتی */
                                              lv_obj_t *w_box = lv_obj_create(row4);
                                              s_warr_status_box = w_box;
                                              lv_obj_set_pos(w_box, 0, 0);
                                              lv_obj_set_size(w_box, 152, 54);
                                              lv_obj_set_style_bg_color(w_box, lv_color_hex(0xf0fdf4), 0);
                                              lv_obj_set_style_border_color(w_box, lv_color_hex(0x86efac), 0);
                                              lv_obj_set_style_border_width(w_box, 1, 0);
                                              lv_obj_set_style_radius(w_box, 8, 0);
                                              lv_obj_set_style_pad_all(w_box, 0, 0);
                                              lv_obj_clear_flag(w_box, LV_OBJ_FLAG_SCROLLABLE);
                                               lv_obj_add_flag(w_box, LV_OBJ_FLAG_CLICKABLE);
                                               lv_obj_add_event_cb(w_box, action_open_warranty_details, LV_EVENT_CLICKED, NULL);

                                              objects.lbl_warr_status_date = lv_label_create(w_box);
                                              lv_obj_set_style_text_font(objects.lbl_warr_status_date, &ui_font_yekan_16, 0);
                                              lv_obj_set_style_text_color(objects.lbl_warr_status_date, lv_color_hex(0x166534), 0);
                                              lv_obj_set_style_base_dir(objects.lbl_warr_status_date, LV_BASE_DIR_LTR, 0);
                                              lv_obj_align(objects.lbl_warr_status_date, LV_ALIGN_TOP_MID, 0, 6);

                                              objects.lbl_warr_status_code = lv_label_create(w_box);
                                              lv_obj_set_style_text_font(objects.lbl_warr_status_code, &lv_font_montserrat_14, 0);
                                              lv_obj_set_style_text_color(objects.lbl_warr_status_code, lv_color_hex(0x0f172a), 0);
                                              lv_obj_set_style_base_dir(objects.lbl_warr_status_code, LV_BASE_DIR_LTR, 0);
                                              lv_obj_align(objects.lbl_warr_status_code, LV_ALIGN_BOTTOM_MID, 0, -6);
                                              lv_obj_add_flag(objects.lbl_warr_status_date, LV_OBJ_FLAG_CLICKABLE);
                                              lv_obj_add_event_cb(objects.lbl_warr_status_date, action_open_warranty_details, LV_EVENT_CLICKED, NULL);
                                              lv_obj_add_flag(objects.lbl_warr_status_code, LV_OBJ_FLAG_CLICKABLE);
                                              lv_obj_add_event_cb(objects.lbl_warr_status_code, action_open_warranty_details, LV_EVENT_CLICKED, NULL);

                                              ui_screens_update_warranty_f6();
                                          }
                                     }
                                 }
                            }
                        }
                    }
                }
            }
        }
        {
            /* Toolbar bottom (چیدمان فشرده بیرون از کادر زبانه‌های F - تراز دقیق ۸ تا ۴۰۶) */
            /* 1. دکمه بازگشت به خانه (Home) */
            lv_obj_t *btn_back = lv_btn_create(parent_obj);
            objects.obj16 = btn_back;
            lv_obj_set_pos(btn_back, 8, 268);
            lv_obj_set_size(btn_back, 42, 42);
            lv_obj_set_ext_click_area(btn_back, 6);
            lv_obj_add_event_cb(btn_back, action_go_menu, LV_EVENT_CLICKED, (void *)5);
            lv_obj_set_style_radius(btn_back, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn_back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl = lv_label_create(btn_back);
                lv_label_set_text(lbl, LV_SYMBOL_HOME);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(lbl);
            }

            /* 2. دکمه ذخیره تنظیمات */
            lv_obj_t *btn_save = lv_btn_create(parent_obj);
            objects.save_btn = btn_save;
            lv_obj_set_pos(btn_save, 55, 268);
            lv_obj_set_size(btn_save, 78, 42);
            lv_obj_set_ext_click_area(btn_save, 6);
            lv_obj_add_event_cb(btn_save, action_save_btn_event_cb, LV_EVENT_ALL, NULL);
            lv_obj_set_style_radius(btn_save, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn_save, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_save, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_save, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x0284c7), LV_PART_MAIN | LV_STATE_PRESSED);
            {
                lv_obj_t *lbl = lv_label_create(btn_save);
                objects.save_lbl = lbl;
                lv_label_set_text(lbl, ui_lang_str(STR_BTN_SAVE));
                lv_obj_set_style_text_font(lbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(lbl);
            }

            /* 3. دکمه راهنمای تب فعال با آیکون و استایل حرفه‌ای */
            lv_obj_t *btn_help = lv_btn_create(parent_obj);
            objects.help_btn = btn_help;
            lv_obj_set_pos(btn_help, 138, 268);
            lv_obj_set_size(btn_help, 78, 42);
            lv_obj_set_ext_click_area(btn_help, 6);
            lv_obj_add_event_cb(btn_help, action_help_dialog, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_radius(btn_help, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_help, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn_help, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_help, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_help, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl = lv_label_create(btn_help);
                objects.help_lbl = lbl;
                lv_label_set_text(lbl, ui_lang_str(STR_BTN_HELP));
                lv_obj_set_style_text_font(lbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(lbl);
            }

            /* 4. دکمه مشاهده تاریخچه خطاها */
            lv_obj_t *btn_fault = lv_btn_create(parent_obj);
            objects.fault_btn = btn_fault;
            lv_obj_set_pos(btn_fault, 221, 268);
            lv_obj_set_size(btn_fault, 85, 42);
            lv_obj_set_ext_click_area(btn_fault, 6);
            lv_obj_add_event_cb(btn_fault, action_open_fault_history, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_radius(btn_fault, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn_fault, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn_fault, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_fault, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_fault, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl = lv_label_create(btn_fault);
                objects.fault_lbl = lbl;
                lv_label_set_text(lbl, ui_lang_str(STR_BTN_FAULTS));
                lv_obj_set_style_text_font(lbl, ui_lang_font_16(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(lbl);
            }

            /* 5. کلید قطع/وصل بوق پنل با آیکون اسپیکر */
            lv_obj_t *btn_buz = lv_btn_create(parent_obj);
            objects.buzzer_sw = btn_buz;
            lv_obj_set_pos(btn_buz, 311, 268);
            lv_obj_set_size(btn_buz, 45, 42);
            lv_obj_set_ext_click_area(btn_buz, 6);
            lv_obj_set_style_radius(btn_buz, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_buz, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_buz, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn_buz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl_buz = lv_label_create(btn_buz);
                objects.buzzer_sw_lbl = lbl_buz;
                lv_label_set_text(lbl_buz, LV_SYMBOL_VOLUME_MAX);
                lv_obj_set_style_text_font(lbl_buz, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl_buz, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(lbl_buz);
            }
            lv_obj_add_event_cb(btn_buz, action_buzzer_sw_changed, LV_EVENT_CLICKED, NULL);

            /* 6. کلید اختصاصی روشن/خاموش کردن وای‌فای کنار کلید بلندگو */
            lv_obj_t *btn_wifi = lv_btn_create(parent_obj);
            objects.wifi_sw_btn = btn_wifi;
            lv_obj_set_pos(btn_wifi, 361, 268);
            lv_obj_set_size(btn_wifi, 45, 42);
            lv_obj_set_ext_click_area(btn_wifi, 6);
            lv_obj_set_style_radius(btn_wifi, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(btn_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn_wifi, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *lbl_w = lv_label_create(btn_wifi);
                objects.wifi_sw_lbl = lbl_w;
                lv_label_set_text(lbl_w, LV_SYMBOL_WIFI);
                lv_obj_set_style_text_font(lbl_w, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl_w, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(lbl_w);
            }
            lv_obj_add_event_cb(btn_wifi, action_wifi_sw_changed, LV_EVENT_CLICKED, NULL);

            /* همگام‌سازی فوری وضعیت بصری دکمه‌های نوار ابزار بر پایه وضعیت واقعی سخت‌افزار */
            ui_update_toolbar_buttons();
        }
    }
    /* کیبورد عددی مشترک روی ریشه صفحه تنظیمات */
    {
        lv_obj_t *kb = lv_keyboard_create(objects.settings_pg);
        objects.sample_kb = kb;
        objects.current_set_1_key = kb;
        objects.current_set_2_key = kb;
        lv_obj_set_size(kb, 460, 160);
        ui_setup_industrial_numpad(kb);
        lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.current_set_1 && objects.current_set_1_key) {
        lv_keyboard_set_textarea(objects.current_set_1_key, objects.current_set_1);
    }
    if (objects.current_set_2 && objects.current_set_2_key) {
        lv_keyboard_set_textarea(objects.current_set_2_key, objects.current_set_2);
    }
    if (objects.wifi_pass_key && objects.wifi_pass_txtarea) {
        lv_keyboard_set_textarea(objects.wifi_pass_key, objects.wifi_pass_txtarea);
    }

    /* پر کردن مقادیر نمونه Delta از NVS */
    {
        char buf[16];
        extern int32_t paddle_timer_1, paddle_timer_2, startup_timer, mid_eye_timer;
        extern int32_t restart_time_1, restart_time_2, modbus_delay;
        extern int32_t current_set_1a, current_set_1b, current_set_2a, current_set_2b;
        extern int32_t icecream_counter;
#define SET_TA(obj, v) do { \
    if (obj) { \
        snprintf(buf, sizeof(buf), "%ld", (long)(v)); \
        if (lv_obj_check_type(obj, &lv_textarea_class)) { \
            lv_textarea_set_text(obj, buf); \
        } else { \
            lv_label_set_text(obj, buf); \
        } \
    } \
} while (0)
        SET_TA(objects.val_paddle_t1, paddle_timer_1);
        SET_TA(objects.val_paddle_t2, paddle_timer_2);
        SET_TA(objects.val_startup_t, startup_timer);
        SET_TA(objects.val_eye_t, mid_eye_timer);
        SET_TA(objects.val_fallback_on, milk_mixer_on_time_s());
        SET_TA(objects.val_fallback_off, milk_mixer_off_time_m());
        SET_TA(objects.val_comp_fb_on, fallback_on_time_m());
        SET_TA(objects.val_comp_fb_off, fallback_off_time_m());
        SET_TA(objects.val_modbus, modbus_delay);
        SET_TA(objects.val_rst_t1, restart_time_1 / 60);
        SET_TA(objects.val_rst_t2, restart_time_2);
        SET_TA(objects.val_cur1a, current_set_1a);
        SET_TA(objects.val_cur1b, current_set_1b);
        SET_TA(objects.val_cur2a, current_set_2a);
        SET_TA(objects.val_cur2b, current_set_2b);
#undef SET_TA
        if (objects.make_icecream_counter_2) {
            lv_label_set_text_fmt(objects.make_icecream_counter_2, "%ld", (long)icecream_counter);
        }
        settings_refresh_labels();
    }

    tick_screen_settings_pg();
}

void tick_screen_settings_pg() {
    static bool s_settings_alive = false;
    if (!s_settings_alive) {
        if (!objects.obj6 || !lv_obj_is_valid(objects.obj6)) return;
        s_settings_alive = true;
    }

    static int s_last_settings_tab = -1;
    int pend = ui_resume_get_pending_tab();
    if (pend >= 0) {
        if (s_last_settings_tab == 4 && pend != 4) {
            io_test_turn_off_all_relays();
        }
        if (objects.obj6 && lv_obj_is_valid(objects.obj6)) {
            lv_obj_update_layout(objects.obj6);
            lv_tabview_set_active(objects.obj6, (uint32_t)pend, LV_ANIM_OFF);
        }
        s_last_settings_tab = pend;
        ui_resume_clear_pending_tab();
    }
    /* B11: فقط یک‌بار خواندن تب فعال و کش کردن آن برای کل تیک */
    int cur_tab = (int)lv_tabview_get_tab_active(objects.obj6);
    ui_set_cached_settings_tab(cur_tab);
    if (cur_tab != s_last_settings_tab) {
        if (s_last_settings_tab == 4 && cur_tab != 4) {
            io_test_turn_off_all_relays();
        }
        s_last_settings_tab = cur_tab;
        ui_resume_checkpoint(SCREEN_ID_SETTINGS_PG, cur_tab);
        if (cur_tab == 3) {
            wifi_trigger_f4_auto_scan();
        }
    }

    /* پایش وضعیت دکمه بروزرسانی فریمور بر اساس اتصال به اینترنت و وای‌فای */
    if (objects.btn_goto_framware && lv_obj_is_valid(objects.btn_goto_framware)) {
        static int8_t s_last_wifi_conn = -1;
        bool conn = wifi_is_connected();
        int8_t conn_val = conn ? 1 : 0;
        if (conn_val != s_last_wifi_conn) {
            s_last_wifi_conn = conn_val;
            if (conn) {
                lv_obj_clear_state(objects.btn_goto_framware, LV_STATE_DISABLED);
                lv_obj_set_style_bg_color(objects.btn_goto_framware, lv_color_hex(0x0f172a), LV_PART_MAIN);
                lv_obj_set_style_border_color(objects.btn_goto_framware, lv_color_hex(0x0284c7), LV_PART_MAIN);
                if (objects.wifi_ota_lbl && lv_obj_is_valid(objects.wifi_ota_lbl)) {
                    lv_obj_set_style_text_color(objects.wifi_ota_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
                }
            } else {
                lv_obj_add_state(objects.btn_goto_framware, LV_STATE_DISABLED);
                lv_obj_set_style_bg_color(objects.btn_goto_framware, lv_color_hex(0xe2e8f0), LV_PART_MAIN);
                lv_obj_set_style_border_color(objects.btn_goto_framware, lv_color_hex(0xcbd5e1), LV_PART_MAIN);
                if (objects.wifi_ota_lbl && lv_obj_is_valid(objects.wifi_ota_lbl)) {
                    lv_obj_set_style_text_color(objects.wifi_ota_lbl, lv_color_hex(0x94a3b8), LV_PART_MAIN);
                }
            }
        }
    }

    /* B11: اگر تب F5 فعال است (ایندکس ۴)، وضعیت زنده سنسورها را بخوان و رنگ ال‌ای‌دی را به‌روز کن */
    if (cur_tab == 4) {
        static int8_t s_last_in_state[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
        static int s_last_diag_tab = -1;
        static int8_t s_last_diag_t1_ok = -1;
        static float s_last_diag_t1 = -999.0f;
        static int8_t s_last_diag_t2_ok = -1;
        static float s_last_diag_t2 = -999.0f;

        if (cur_tab != s_last_diag_tab) {
            s_last_diag_tab = cur_tab;
            for (int k = 0; k < 8; k++) s_last_in_state[k] = -1;
            s_last_diag_t1_ok = -1;
            s_last_diag_t1 = -999.0f;
            s_last_diag_t2_ok = -1;
            s_last_diag_t2 = -999.0f;
        }

        for (int i = 0; i < 8; i++) {
            if (objects.led_in[i] && lv_obj_is_valid(objects.led_in[i])) {
                bool act = pinmap_terminal_is_active((uint8_t)i);
                int8_t act_val = act ? 1 : 0;
                if (act_val != s_last_in_state[i]) {
                    s_last_in_state[i] = act_val;
                    if (act) {
                        lv_led_on(objects.led_in[i]);
                    } else {
                        lv_led_off(objects.led_in[i]);
                    }
                }
            }
        }

        /* به‌روزرسانی زنده دمای مخزن و سیلندر در تب دیاگ با Dirty-Check */
        if (objects.diag_temp_hopper_val && lv_obj_is_valid(objects.diag_temp_hopper_val)) {
            bool ok = max6675_is_top_valid();
            int8_t ok_val = ok ? 1 : 0;
            float t1 = max6675_get_temp_top();
            if (ok_val != s_last_diag_t1_ok || (ok && (int)(t1 * 10.0f) != (int)(s_last_diag_t1 * 10.0f))) {
                s_last_diag_t1_ok = ok_val;
                s_last_diag_t1 = t1;
                if (ok) {
                    char tbuf1[32];
                    snprintf(tbuf1, sizeof(tbuf1), "%+.1f°C", t1);
                    lv_label_set_text(objects.diag_temp_hopper_val, tbuf1);
                    lv_obj_set_style_text_color(objects.diag_temp_hopper_val, lv_color_hex(0x0284c7), 0);
                } else {
                    lv_label_set_text(objects.diag_temp_hopper_val, ui_lang_str(STR_SENSOR_NC));
                    lv_obj_set_style_text_color(objects.diag_temp_hopper_val, lv_color_hex(0xef4444), 0);
                }
            }
        }

        if (objects.diag_temp_cyl_val && lv_obj_is_valid(objects.diag_temp_cyl_val)) {
            bool ok = max6675_is_cylinder_valid();
            int8_t ok_val = ok ? 1 : 0;
            float t2 = max6675_get_temp_cylinder();
            if (ok_val != s_last_diag_t2_ok || (ok && (int)(t2 * 10.0f) != (int)(s_last_diag_t2 * 10.0f))) {
                s_last_diag_t2_ok = ok_val;
                s_last_diag_t2 = t2;
                if (ok) {
                    char tbuf2[32];
                    snprintf(tbuf2, sizeof(tbuf2), "%+.1f°C", t2);
                    lv_label_set_text(objects.diag_temp_cyl_val, tbuf2);
                    lv_obj_set_style_text_color(objects.diag_temp_cyl_val, lv_color_hex(0x0284c7), 0);
                } else {
                    lv_label_set_text(objects.diag_temp_cyl_val, ui_lang_str(STR_SENSOR_NC));
                    lv_obj_set_style_text_color(objects.diag_temp_cyl_val, lv_color_hex(0xef4444), 0);
                }
            }
        }
    }

    /* اگر تب F1 فعال است، شمارنده جاری را رفرش کن */
    if (cur_tab == 0) {
        static int32_t s_last_f1_cnt = -999;
        if (objects.make_icecream_counter_2 && lv_obj_is_valid(objects.make_icecream_counter_2)) {
            if (icecream_counter != s_last_f1_cnt) {
                s_last_f1_cnt = icecream_counter;
                lv_label_set_text_fmt(objects.make_icecream_counter_2, "%ld", (long)icecream_counter);
                const lv_font_t *cnt_font = (icecream_counter >= 1000000) ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
                lv_obj_set_style_text_font(objects.make_icecream_counter_2, cnt_font, 0);
                lv_obj_center(objects.make_icecream_counter_2);
            }
        }
    }

    /* B11: اگر تب F2 فعال است، روزشمار قفل دستگاه را رفرش کن */
    if (cur_tab == 1) {
        ui_update_lease_display();
    }

    /* نظارت Dirty-driven بر دکمه‌های نوار ابزار پایین (وای‌فای، بلندگو، خطاها) */
    static int8_t s_last_w_en = -1;
    static int8_t s_last_b_en = -1;
    static int8_t s_last_unread = -1;

    int8_t cur_w_en = wifi_is_enabled() ? 1 : 0;
    int8_t cur_b_en = (buzzer_is_enabled() && buzzer_get_volume() > 0) ? 1 : 0;
    int8_t cur_unread = fault_log_has_unread() ? 1 : 0;

    if (cur_w_en != s_last_w_en || cur_b_en != s_last_b_en || cur_unread != s_last_unread) {
        s_last_w_en = cur_w_en;
        s_last_b_en = cur_b_en;
        s_last_unread = cur_unread;
        ui_update_toolbar_buttons();
    }
}

void ui_update_lease_display(void)
{
    if (objects.lease_days_lbl && lv_obj_is_valid(objects.lease_days_lbl)) {
        static int32_t s_last_time_val = -999;
        static DayLockTimeUnit_t s_last_time_unit = (DayLockTimeUnit_t)-1;
        static DayLockState_t s_last_l_st = (DayLockState_t)-1;
        static ui_lang_t s_last_dl_lang = (ui_lang_t)-1;
        static uint8_t s_last_stg = 255;
        static uint8_t s_last_tot_stg = 255;

        int32_t cur_time_val = 0;
        DayLockTimeUnit_t cur_time_unit = day_lock_get_remain_time_unit(&cur_time_val);
        DayLockState_t l_st = day_lock_get_state();
        ui_lang_t cur_lang = ui_lang_get();
        uint8_t cur_stg = day_lock_get_active_stage();
        uint8_t tot_stg = day_lock_get_total_stages();
        if (tot_stg < cur_stg) tot_stg = cur_stg;
        if (tot_stg < 1) tot_stg = 1;

        if (cur_time_val == s_last_time_val && cur_time_unit == s_last_time_unit &&
            l_st == s_last_l_st && cur_lang == s_last_dl_lang &&
            cur_stg == s_last_stg && tot_stg == s_last_tot_stg) {
            return;
        }
        s_last_time_val = cur_time_val;
        s_last_time_unit = cur_time_unit;
        s_last_l_st = l_st;
        s_last_dl_lang = cur_lang;
        s_last_stg = cur_stg;
        s_last_tot_stg = tot_stg;

        bool is_locked = (l_st == LOCK_STAGE_HARD_LOCK || l_st == LOCK_STAGE_TAMPER);
        if (objects.lease_status_badge && lv_obj_is_valid(objects.lease_status_badge)) {
            if (is_locked) {
                lv_obj_set_style_bg_color(objects.lease_status_badge, lv_color_hex(0xfef2f2), 0);
                lv_obj_set_style_border_color(objects.lease_status_badge, lv_color_hex(0xf87171), 0);
                lv_obj_set_style_border_width(objects.lease_status_badge, 1, 0);
            } else if (l_st == LOCK_STAGE_ACTIVE_LEASE || l_st == LOCK_STAGE_GRACE_10) {
                lv_obj_set_style_bg_color(objects.lease_status_badge, lv_color_hex(0xf0fdf4), 0);
                lv_obj_set_style_border_color(objects.lease_status_badge, lv_color_hex(0x86efac), 0);
                lv_obj_set_style_border_width(objects.lease_status_badge, 1, 0);
            } else {
                lv_obj_set_style_bg_color(objects.lease_status_badge, lv_color_hex(0xf0f9ff), 0);
                lv_obj_set_style_border_color(objects.lease_status_badge, lv_color_hex(0x7dd3fc), 0);
                lv_obj_set_style_border_width(objects.lease_status_badge, 1, 0);
            }
        }

        if (is_locked) {
            if (cur_stg >= 1 && cur_stg <= DAY_LOCK_MAX_STAGES) {
                char lk_buf[48];
                if (cur_lang == UI_LANG_FA) {
                    snprintf(lk_buf, sizeof(lk_buf), "قفل: پایان قسط %u از %u", (unsigned)cur_stg, (unsigned)tot_stg);
                } else if (cur_lang == UI_LANG_TR) {
                    snprintf(lk_buf, sizeof(lk_buf), "Kilitli: %u/%u Taksit", (unsigned)cur_stg, (unsigned)tot_stg);
                } else {
                    snprintf(lk_buf, sizeof(lk_buf), "Locked: Stage %u/%u", (unsigned)cur_stg, (unsigned)tot_stg);
                }
                lv_label_set_text(objects.lease_days_lbl, lk_buf);
            } else {
                lv_label_set_text(objects.lease_days_lbl, ui_lang_str(STR_DL_STATUS_LOCKED));
            }
            lv_obj_set_style_text_color(objects.lease_days_lbl, lv_color_hex(0xdc2626), 0);
        } else if (l_st == LOCK_STAGE_ACTIVE_LEASE || l_st == LOCK_STAGE_GRACE_10) {
            char act_buf[48];
            format_lease_remain_str(act_buf, sizeof(act_buf), cur_stg, tot_stg, cur_lang);
            lv_label_set_text(objects.lease_days_lbl, act_buf);
            lv_obj_set_style_text_color(objects.lease_days_lbl, lv_color_hex(0x16a34a), 0);
        } else {
            lv_label_set_text(objects.lease_days_lbl, ui_lang_str(STR_DL_STATUS_UNLIMITED));
            lv_obj_set_style_text_color(objects.lease_days_lbl, lv_color_hex(0x0284c7), 0);
        }
        lv_obj_set_style_text_font(objects.lease_days_lbl, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(objects.lease_days_lbl, ui_lang_base_dir(), 0);
        lv_obj_center(objects.lease_days_lbl);
    }
}




/* =========================================================================
 *                   FIRMWARE OTA & SYSTEM UPDATE SCREEN
 * ========================================================================= */
static lv_obj_t *s_fw_card_obj[OTA_MODEL_COUNT] = {NULL, NULL};
static lv_obj_t *s_fw_card_ver_lbl[OTA_MODEL_COUNT] = {NULL, NULL};
static lv_obj_t *s_fw_card_btn_inst[OTA_MODEL_COUNT] = {NULL, NULL};
static lv_obj_t *s_fw_card_btn_inst_lbl[OTA_MODEL_COUNT] = {NULL, NULL};

static float s_ota_smooth_eta_sec = 0.0f;
static uint32_t s_ota_eta_last_sec_tick = 0;
static uint32_t s_last_rendered_scan_rev = 0xFFFFFFFF;

static int compare_semantic_versions(const char *v1, const char *v2)
{
    if (!v1 && !v2) return 0;
    if (!v1) return -1;
    if (!v2) return 1;

    while (*v1 == 'v' || *v1 == 'V' || *v1 == ' ') v1++;
    while (*v2 == 'v' || *v2 == 'V' || *v2 == ' ') v2++;

    int maj1 = 0, min1 = 0, pat1 = 0;
    int maj2 = 0, min2 = 0, pat2 = 0;

    sscanf(v1, "%d.%d.%d", &maj1, &min1, &pat1);
    sscanf(v2, "%d.%d.%d", &maj2, &min2, &pat2);

    if (maj1 != maj2) return maj1 - maj2;
    if (min1 != min2) return min1 - min2;
    return pat1 - pat2;
}

static lv_obj_t *s_ota_pinmap_popup = NULL;
static lv_obj_t *s_ota_pinmap_scroll = NULL;
static lv_obj_t *s_ota_pinmap_content = NULL;
static int s_ota_pinmap_idx = 0;

static lv_obj_t *s_ota_progress_modal = NULL;
static lv_obj_t *s_ota_progress_bar = NULL;
static lv_obj_t *s_ota_progress_lbl = NULL;
static lv_obj_t *s_ota_progress_status = NULL;
static lv_obj_t *s_ota_progress_circle = NULL;
static lv_obj_t *s_ota_progress_icon = NULL;
static lv_obj_t *s_ota_progress_close_btn = NULL;
static lv_obj_t *s_ota_progress_close_lbl = NULL;
static lv_obj_t *s_ota_main_pct_lbl = NULL;
static lv_obj_t *s_ota_card_rem_pct_val = NULL;
static lv_obj_t *s_ota_card_size_val = NULL;
static lv_obj_t *s_ota_card_speed_val = NULL;
static lv_obj_t *s_ota_card_eta_val = NULL;
static ota_install_state_t s_last_rendered_inst_state = OTA_INST_IDLE;

static lv_obj_t *s_custom_url_modal = NULL;
static lv_obj_t *s_custom_url_ta = NULL;
static lv_obj_t *s_custom_url_kb = NULL;
static lv_obj_t *s_url_msgbox = NULL;
static lv_obj_t *s_url_msgbox_title = NULL;
static lv_obj_t *s_url_msgbox_sub = NULL;
static lv_obj_t *s_url_msgbox_bar = NULL;
static lv_timer_t *s_url_msgbox_timer = NULL;
static int32_t s_url_msgbox_ms_left = 0;
static bool s_url_msgbox_touch_held = false;
static char s_last_checked_valid_url[256] = {0};

static bool s_ota_last_scan_done = false;
static uint32_t s_ota_last_tick_scan = 0;

static volatile uint32_t s_ota_dl_bytes = 0;
static volatile uint32_t s_ota_dl_total = 0;
static volatile uint8_t s_ota_dl_pct = 0;
static volatile bool s_ota_progress_dirty = false;
static uint32_t s_ota_speed_last_time = 0;
static uint32_t s_ota_speed_last_bytes = 0;
static float s_ota_speed_kbps = 0.0f;
static int s_ota_install_active_model = -1;

static void ota_progress_close_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_beep_click();
    ota_cancel_install();
    if (s_ota_progress_modal && lv_obj_is_valid(s_ota_progress_modal)) {
        lv_obj_del(s_ota_progress_modal);
        s_ota_progress_modal = NULL;
        s_ota_progress_bar = NULL;
        s_ota_progress_lbl = NULL;
        s_ota_progress_status = NULL;
        s_ota_progress_circle = NULL;
        s_ota_progress_icon = NULL;
        s_ota_progress_close_btn = NULL;
        s_ota_progress_close_lbl = NULL;
        s_ota_main_pct_lbl = NULL;
        s_ota_card_rem_pct_val = NULL;
        s_ota_card_size_val = NULL;
        s_ota_card_speed_val = NULL;
        s_ota_card_eta_val = NULL;
        s_last_rendered_inst_state = OTA_INST_IDLE;
        s_ota_smooth_eta_sec = 0.0f;
        s_ota_eta_last_sec_tick = 0;
        s_ota_speed_kbps = 0.0f;
        s_ota_speed_last_time = 0;
        s_ota_speed_last_bytes = 0;
    }
}

static void ota_ui_progress_cb(uint32_t bytes_dl, uint32_t total_bytes, uint8_t pct)
{
    s_ota_dl_bytes = bytes_dl;
    s_ota_dl_total = total_bytes;
    s_ota_dl_pct = pct;
    s_ota_progress_dirty = true;
}

static void start_ota_install_for_model(int idx)
{
    if (!wifi_is_connected()) {
        wifi_status_msgbox_show_error(ui_lang_str(STR_WIFI_CONNECT_FIRST));
        return;
    }

    if (s_ota_progress_modal && lv_obj_is_valid(s_ota_progress_modal)) {
        lv_obj_del(s_ota_progress_modal);
        s_ota_progress_modal = NULL;
        s_ota_progress_bar = NULL;
        s_ota_progress_lbl = NULL;
        s_ota_progress_status = NULL;
        s_ota_progress_circle = NULL;
        s_ota_progress_icon = NULL;
        s_ota_progress_close_btn = NULL;
        s_ota_progress_close_lbl = NULL;
        s_ota_main_pct_lbl = NULL;
        s_ota_card_rem_pct_val = NULL;
        s_ota_card_size_val = NULL;
        s_ota_card_speed_val = NULL;
        s_ota_card_eta_val = NULL;
        s_last_rendered_inst_state = OTA_INST_IDLE;
    }

    // لایه مات تمام‌صفحه سبک اپل (Backdrop Solid Dark Slate)
    s_ota_progress_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_ota_progress_modal, 480, 320);
    lv_obj_set_pos(s_ota_progress_modal, 0, 0);
    lv_obj_set_style_bg_color(s_ota_progress_modal, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_ota_progress_modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ota_progress_modal, 0, 0);
    lv_obj_set_style_pad_all(s_ota_progress_modal, 0, 0);
    lv_obj_clear_flag(s_ota_progress_modal, LV_OBJ_FLAG_SCROLLABLE);

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    // عنوان متمرکز در بالای صفحه با تراز ریاضی و ایزولاسیون نسخه LRO
    lv_obj_t *title = lv_label_create(s_ota_progress_modal);
    lv_obj_set_width(title, 460);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_text_font(title, ui_lang_font_20(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);

    const lastfile_info_t *target_info = (idx >= 0 && idx < OTA_MODEL_COUNT) ? ota_get_info(idx) : NULL;
    const char *target_ver = (target_info && target_info->ver[0]) ? target_info->ver : "";

    const char *s_ver = (target_ver[0] == 'v' || target_ver[0] == 'V') ? target_ver + 1 : target_ver;

    char t_str[128];
    if (idx == 0) {
        if (target_ver[0]) {
            snprintf(t_str, sizeof(t_str), is_fa ? "دریافت کد بستنی‌ساز \xE2\x80\xAA(V%s)\xE2\x80\xAC" : (is_tr ? "Dondurma Kodu Indiriliyor (V%s)" : "Downloading Ice Cream Code (V%s)"), s_ver);
        } else {
            snprintf(t_str, sizeof(t_str), "%s", is_fa ? "دریافت کد بستنی‌ساز..." : (is_tr ? "Dondurma Kodu Indiriliyor..." : "Downloading Ice Cream Code..."));
        }
    } else if (idx == 1) {
        if (target_ver[0]) {
            snprintf(t_str, sizeof(t_str), is_fa ? "در حال دریافت تست و کانفیگ \xE2\x80\xAA(V%s)\xE2\x80\xAC" : (is_tr ? "Test ve Yapilandirma (V%s)" : "Downloading Config App (V%s)"), s_ver);
        } else {
            snprintf(t_str, sizeof(t_str), "%s", is_fa ? "در حال دریافت تست و کانفیگ..." : (is_tr ? "Test Yazilimi Indiriliyor..." : "Downloading Config & Test App..."));
        }
    } else {
        snprintf(t_str, sizeof(t_str), "%s", is_fa ? "دریافت کد فریمور مستقیم..." : (is_tr ? "Dogrudan Yazilim Indiriliyor..." : "Downloading Direct Firmware..."));
    }
    lv_label_set_text(title, t_str);

    // دایره پیشرفت درصدی سبک اپل (y=36, 50x50) با فضای تنفس کامل
    lv_obj_t *circle = lv_arc_create(s_ota_progress_modal);
    s_ota_progress_circle = circle;
    lv_obj_set_size(circle, 50, 50);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, 36);
    lv_arc_set_rotation(circle, 270);
    lv_arc_set_bg_angles(circle, 0, 360);
    lv_arc_set_angles(circle, 0, 0);
    lv_arc_set_range(circle, 0, 100);
    lv_arc_set_value(circle, 0);
    lv_obj_remove_style(circle, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_arc_width(circle, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(circle, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_arc_width(circle, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(circle, lv_color_hex(0x0284c7), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(circle, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_pad_all(circle, 0, 0);

    s_ota_main_pct_lbl = lv_label_create(circle);
    lv_label_set_text(s_ota_main_pct_lbl, "%0");
    lv_obj_set_style_text_font(s_ota_main_pct_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(s_ota_main_pct_lbl, lv_color_hex(0x38bdf8), 0);
    lv_obj_center(s_ota_main_pct_lbl);

    s_ota_progress_icon = lv_label_create(circle);
    lv_label_set_text(s_ota_progress_icon, "");
    lv_obj_set_style_text_font(s_ota_progress_icon, &lv_font_montserrat_24, 0);
    lv_obj_center(s_ota_progress_icon);
    lv_obj_add_flag(s_ota_progress_icon, LV_OBJ_FLAG_HIDDEN);

    // نوار پیشرفت مدرن اپل متقارن (y=100, 400x10) با حاشیه و قاب متمایز و فضای تنفس
    s_ota_progress_bar = lv_bar_create(s_ota_progress_modal);
    lv_obj_set_size(s_ota_progress_bar, 400, 10);
    lv_obj_align(s_ota_progress_bar, LV_ALIGN_TOP_MID, 0, 100);
    lv_bar_set_range(s_ota_progress_bar, 0, 100);
    lv_bar_set_value(s_ota_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_base_dir(s_ota_progress_bar, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_bg_color(s_ota_progress_bar, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_opa(s_ota_progress_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ota_progress_bar, 1, 0);
    lv_obj_set_style_border_color(s_ota_progress_bar, lv_color_hex(0x334155), 0);
    lv_obj_set_style_radius(s_ota_progress_bar, 5, 0);
    lv_obj_set_style_bg_color(s_ota_progress_bar, lv_color_hex(0x0284c7), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_ota_progress_bar, 5, LV_PART_INDICATOR);

    // ۴ کارت مشخصات صنعتی آستیاج با چینش استاندارد و تراز راست‌به‌چپ در فارسی (y=120, h=62, w=96)
    // در زبان فارسی: راست: ۱. پیشرفت | ۲. حجم دانلود | ۳. سرعت | چپ: ۴. زمان مانده
    const char *card_titles[4] = {
        is_fa ? "پیشرفت" : (is_tr ? "Ilerleme" : "Progress"),
        is_fa ? "حجم دانلود" : (is_tr ? "Boyut" : "Size"),
        is_fa ? "سرعت" : (is_tr ? "Hiz" : "Speed"),
        is_fa ? "زمان مانده" : (is_tr ? "Kalan Sure" : "Est. Time")
    };
    /* تراز متقارن ۴ کارت در صفحه ۴۸۰ پیکسل (عرض هر کارت ۹۶ + فاصله ۱۴ = ۴۲۶ پیکسل -> حاشیه طرفین ۲۷ پیکسل) */
    int card_xs[4];
    if (is_fa) {
        card_xs[0] = 357; /* پیشرفت: سمت راست */
        card_xs[1] = 247; /* حجم دانلود */
        card_xs[2] = 137; /* سرعت */
        card_xs[3] = 27;  /* زمان مانده: سمت چپ */
    } else {
        card_xs[0] = 27;  /* Progress: Left */
        card_xs[1] = 137; /* Size */
        card_xs[2] = 247; /* Speed */
        card_xs[3] = 357; /* Est. Time: Right */
    }

    for (int i = 0; i < 4; i++) {
        lv_obj_t *card = lv_obj_create(s_ota_progress_modal);
        lv_obj_set_pos(card, card_xs[i], 120);
        lv_obj_set_size(card, 96, 62);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x334155), 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        // تیتر کارت
        lv_obj_t *tl = lv_label_create(card);
        lv_label_set_text(tl, card_titles[i]);
        lv_obj_set_style_text_font(tl, ui_lang_font_14(), 0);
        lv_obj_set_style_text_color(tl, lv_color_hex(0x94a3b8), 0);
        lv_obj_set_style_base_dir(tl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 4);

        // مقدار اصلی مستقیم در کارت
        lv_obj_t *val = lv_label_create(card);
        lv_obj_set_style_base_dir(val, LV_BASE_DIR_LTR, 0);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(val, LV_ALIGN_BOTTOM_MID, 0, -6);

        if (i == 0) {
            s_ota_card_rem_pct_val = val;
            lv_label_set_text(val, "0%");
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(val, lv_color_hex(0x38bdf8), 0);
        } else if (i == 1) {
            s_ota_card_size_val = val;
            uint32_t init_total = 0;
            if (idx >= 0 && idx < OTA_MODEL_COUNT) {
                init_total = ota_get_info(idx)->size_bytes;
            }
            if (init_total == 0) {
                init_total = 3190912; // 3.1 MB fallback for Ice Cream
            }
            char init_sz_buf[32];
            uint32_t tot_x10 = (init_total * 10) / (1024 * 1024);
            snprintf(init_sz_buf, sizeof(init_sz_buf), "0.0/%u.%u MB",
                     (unsigned)(tot_x10 / 10), (unsigned)(tot_x10 % 10));
            lv_label_set_text(val, init_sz_buf);
            lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(val, lv_color_hex(0xf8fafc), 0);
            lv_obj_set_style_base_dir(val, LV_BASE_DIR_LTR, 0);
        } else if (i == 2) {
            s_ota_card_speed_val = val;
            lv_label_set_text(val, "0 KB/s");
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(val, lv_color_hex(0x22c55e), 0);
            lv_obj_set_style_base_dir(val, LV_BASE_DIR_LTR, 0);
        } else {
            s_ota_card_eta_val = val;
            lv_label_set_text(val, "--:--");
            lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(val, lv_color_hex(0xfbbf24), 0);
            lv_obj_set_style_base_dir(val, LV_BASE_DIR_LTR, 0);
        }
    }

    // خط وضعیت / پیام خطا در پایین کارت‌ها (y=190)
    s_ota_progress_status = lv_label_create(s_ota_progress_modal);
    lv_obj_set_width(s_ota_progress_status, 460);
    lv_label_set_long_mode(s_ota_progress_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_ota_progress_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_ota_progress_status, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0xf59e0b), 0);
    lv_obj_set_style_base_dir(s_ota_progress_status, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_label_set_text(s_ota_progress_status, is_fa ? "در حال آماده‌سازی حافظه دستگاه..." : (is_tr ? "Cihaz hafizasi hazirlaniyor..." : "Preparing flash memory..."));

    // کلید قرمز کپسولی انصراف (y=224, 130x34)
    s_ota_progress_close_btn = lv_btn_create(s_ota_progress_modal);
    lv_obj_set_size(s_ota_progress_close_btn, 130, 34);
    lv_obj_align(s_ota_progress_close_btn, LV_ALIGN_TOP_MID, 0, 224);
    lv_obj_set_style_radius(s_ota_progress_close_btn, 17, 0);
    lv_obj_set_style_bg_color(s_ota_progress_close_btn, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_bg_color(s_ota_progress_close_btn, lv_color_hex(0xb91c1c), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(s_ota_progress_close_btn, 0, 0);
    lv_obj_add_event_cb(s_ota_progress_close_btn, ota_progress_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_c = lv_label_create(s_ota_progress_close_btn);
    s_ota_progress_close_lbl = lbl_c;
    lv_label_set_text(lbl_c, is_fa ? "انصراف" : (is_tr ? "Iptal" : "Cancel"));
    lv_obj_set_style_text_font(lbl_c, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_c, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_c);

    // کادر کپسولی آدرس لینک سرور در پایین صفحه (y=268)
    const char *target_bin_url = ota_get_bin_url(idx);
    if (target_bin_url && target_bin_url[0]) {
        lv_obj_t *url_cont = lv_obj_create(s_ota_progress_modal);
        lv_obj_set_size(url_cont, 430, 26);
        lv_obj_align(url_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_bg_color(url_cont, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_bg_opa(url_cont, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(url_cont, 1, 0);
        lv_obj_set_style_border_color(url_cont, lv_color_hex(0x334155), 0);
        lv_obj_set_style_radius(url_cont, 6, 0);
        lv_obj_set_style_pad_all(url_cont, 0, 0);
        lv_obj_clear_flag(url_cont, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *url_lbl = lv_label_create(url_cont);
        char display_url[256];
        if (idx == 0 || strstr(target_bin_url, "ice-cream")) {
            snprintf(display_url, sizeof(display_url), "https://ota.haffler-infi.ir/other/ice-cream");
        } else {
            char temp[256];
            snprintf(temp, sizeof(temp), "%.240s", target_bin_url);
            char *last_slash = strrchr(temp, '/');
            if (last_slash && last_slash > temp + 8) {
                *last_slash = '\0';
            }
            if (strncmp(temp, "http://", 7) == 0) {
                snprintf(display_url, sizeof(display_url), "https://%.200s", temp + 7);
            } else {
                snprintf(display_url, sizeof(display_url), "%.240s", temp);
            }
        }
        lv_label_set_text(url_lbl, display_url);
        lv_obj_set_style_text_font(url_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(url_lbl, lv_color_hex(0x94a3b8), 0);
        lv_obj_set_style_base_dir(url_lbl, LV_BASE_DIR_LTR, 0);
        lv_obj_center(url_lbl);
    }

    s_ota_install_active_model = idx;
    s_ota_speed_last_time = 0;
    s_ota_speed_last_bytes = 0;
    s_ota_speed_kbps = 0.0f;
    s_ota_smooth_eta_sec = 0.0f;
    s_ota_eta_last_sec_tick = 0;
    s_ota_dl_bytes = 0;
    s_ota_dl_total = 0;
    s_ota_dl_pct = 0;
    s_ota_progress_dirty = false;
    s_last_rendered_inst_state = OTA_INST_IDLE;

    ota_start_install(idx, ota_ui_progress_cb);
}

static void framware_install_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    buzz(100);
    start_ota_install_for_model(idx);
}

void ota_ui_start_install_model(int idx)
{
    start_ota_install_for_model(idx);
}

static lv_obj_t *s_pm_title = NULL;
static lv_obj_t *s_pm_in = NULL;
static lv_obj_t *s_pm_out = NULL;
static lv_obj_t *s_pm_meta_ver_build = NULL;
static lv_obj_t *s_pm_meta_status = NULL;
static lv_obj_t *s_pm_meta_url = NULL;
static lv_obj_t *s_pm_desc_title = NULL;
static lv_obj_t *s_pm_meta_desc = NULL;

void hide_ota_pinmap_dialog(void)
{
    if (s_ota_pinmap_popup && lv_obj_is_valid(s_ota_pinmap_popup)) {
        lv_obj_del(s_ota_pinmap_popup);
        s_ota_pinmap_popup = NULL;
        s_ota_pinmap_scroll = NULL;
        s_ota_pinmap_content = NULL;
        s_pm_title = NULL;
        s_pm_in = NULL;
        s_pm_out = NULL;
        s_pm_meta_ver_build = NULL;
        s_pm_meta_status = NULL;
        s_pm_meta_url = NULL;
        s_pm_desc_title = NULL;
        s_pm_meta_desc = NULL;
    }
}

static void fw_pinmap_close_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzz(100);
    hide_ota_pinmap_dialog();
}

static void pinmap_install_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzz(100);
    int idx = s_ota_pinmap_idx;
    hide_ota_pinmap_dialog();
    start_ota_install_for_model(idx);
}

static void fw_pinmap_card_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    buzz(100);
    show_ota_pinmap_dialog(idx);
}

static void fw_pinmap_update_content(int idx)
{
    if (!s_ota_pinmap_popup || !lv_obj_is_valid(s_ota_pinmap_popup)) return;
    const lastfile_info_t *info = ota_get_info(idx);
    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    // ۱. هدر عنوان
    if (s_pm_title && lv_obj_is_valid(s_pm_title)) {
        const char *t_str = (idx == 1) ?
            (is_fa ? "برنامه تست و کانفیگ سخت‌افزار" : (is_tr ? "Test ve Yapilandirma Yazilimi" : "Hardware Test & Config Firmware")) :
            (is_fa ? "برنامه اصلی کنترلر بستنی آستیاج" : (is_tr ? "ASTIAJ CM-700 Dondurma Ana Yazilimi" : "ASTIAJ CM-700 Ice Cream Main Firmware"));
        lv_label_set_text(s_pm_title, t_str);
    }

    // ۲. ورودی‌ها (Inputs)
    if (s_pm_in && lv_obj_is_valid(s_pm_in)) {
        if (info && info->pinmap_inputs[0]) {
            char clean_in[1200];
            ota_sanitize_server_text(clean_in, sizeof(clean_in), info->pinmap_inputs);
            lv_label_set_text(s_pm_in, clean_in);
        } else {
            if (idx == 1) {
                lv_label_set_text(s_pm_in, is_fa ?
                    "• ورودی‌های ایزوله دیاگ (۰ تا ۷)\n"
                    "• کنترل‌بار موتور پارو\n"
                    "• کنترل‌بار کمپرسور بزرگ\n"
                    "• سنسور دمای مخزن شیر\n"
                    "• سنسور دمای سیلندر برودت\n"
                    "• خطوط کمکی دوطرفه (M0 تا M7)" :
                    "I0..I7: Isolated Diagnostic Inputs\n"
                    "LM: Mixer Motor Overload (Terminal I3)\n"
                    "LC: Compressor Overload (Terminal I5)\n"
                    "T1: Hopper Milk Temp (Terminal T1)\n"
                    "T2: Cylinder Cooling Temp (Terminal T2)\n"
                    "M0..M7: Bidirectional Auxiliary Lines");
            } else {
                lv_label_set_text(s_pm_in, is_fa ?
                    "• ورودی ۰: سنسور دور پولی پارو\n"
                    "• ورودی ۱: کلاچ عقب (سفتی بستنی)\n"
                    "• ورودی ۲: اهرم جلو (شمارنده قیف)\n"
                    "• ورودی ۳ و ۴: کنترل‌بار پارو و کمپرسور\n"
                    "• ورودی ۵: ورودی کمکی رزرو\n"
                    "• ورودی ۶: سنسور خالی بودن مخزن شیر\n"
                    "• ورودی ۷: شستی استارت چرخه\n"
                    "• سنسورهای دمای مخزن و سیلندر\n"
                    "• خطوط کمکی دوطرفه (M0 تا M7)" :
                    "I0: Pulley Inductive Prox Sensor\n"
                    "I1: Rear Clutch (Ice Cream Hardness)\n"
                    "I2: Front Lever (Cone Dispense Counter)\n"
                    "I3/I4: Mixer & Compressor Overloads\n"
                    "I5: Reserved Auxiliary Input\n"
                    "I6: Milk Low Level Hopper Sensor\n"
                    "I7: Panel Production Start Button\n"
                    "TC1/2: Hopper & Cylinder Thermocouples\n"
                    "M0..M7: Bidirectional Auxiliary Lines");
            }
        }
    }

    // ۳. خروجی‌ها (Outputs)
    if (s_pm_out && lv_obj_is_valid(s_pm_out)) {
        if (info && info->pinmap_outputs[0]) {
            char clean_out[1200];
            ota_sanitize_server_text(clean_out, sizeof(clean_out), info->pinmap_outputs);
            lv_label_set_text(s_pm_out, clean_out);
        } else {
            if (idx == 1) {
                lv_label_set_text(s_pm_out, is_fa ?
                    "• خروجی‌های رله تست (۰ تا ۶)\n"
                    "• خطوط کمکی دوطرفه (M0 تا M7)" :
                    "Q0..Q6: Power Relay Test Outputs\n"
                    "M0..M7: Bidirectional Auxiliary Lines");
            } else {
                lv_label_set_text(s_pm_out, is_fa ?
                    "• خروجی ۰: نگهدارنده\n"
                    "• خروجی ۱: کمپرسور بزرگ سیلندر\n"
                    "• خروجی ۲: کنتاکتور موتور پارو\n"
                    "• خروجی ۳: پمپ کرم لبه رنگی\n"
                    "• خروجی ۴: موتور پارو مخزن شیر\n"
                    "• خروجی ۵: شیر باد لبه رنگی\n"
                    "• خروجی ۶: رله ریست مدار فرمان\n"
                    "• خطوط کمکی دوطرفه (M0 تا M7)" :
                    "Q0: Holding\n"
                    "Q1: Cylinder Main Compressor\n"
                    "Q2: Cylinder Paddle Contactor\n"
                    "Q3: Edge Cream Syrup Pump\n"
                    "Q4: Hopper Milk Paddle Motor\n"
                    "Q5: Rainbow Edge Air Solenoid\n"
                    "Q6: Control Circuit Reset Pulse\n"
                    "M0..M7: Bidirectional Auxiliary Lines");
            }
        }
    }

    // ۴. مشخصات سرور و بیلد
    if (s_pm_meta_ver_build && lv_obj_is_valid(s_pm_meta_ver_build)) {
        if (info && info->ver[0]) {
            const char *c_ver = (FIRMWARE_VERSION_STR[0] == 'v' || FIRMWARE_VERSION_STR[0] == 'V') ? FIRMWARE_VERSION_STR + 1 : FIRMWARE_VERSION_STR;
            const char *s_ver = (info->ver[0] == 'v' || info->ver[0] == 'V') ? info->ver + 1 : info->ver;
            lv_label_set_text_fmt(s_pm_meta_ver_build,
                is_fa ? "مسیر نسخه: V%s -> V%s  |  بیلد: %s %s" : "Upgrade: V%s -> V%s  |  Build: %s %s",
                c_ver, s_ver,
                info->bdate[0] ? info->bdate : "-",
                info->btime[0] ? info->btime : "");
        } else {
            lv_label_set_text(s_pm_meta_ver_build,
                is_fa ? "نسخه سرور: در حال دریافت...  |  دستگاه: " FIRMWARE_VERSION_STR : "Server Version: Fetching...  |  Device: " FIRMWARE_VERSION_STR);
        }
    }

    // ۵. وضعیت سرور
    if (s_pm_meta_status && lv_obj_is_valid(s_pm_meta_status)) {
        if (info && info->ver[0]) {
            lv_label_set_text(s_pm_meta_status, is_fa ? "وضعیت: آماده دریافت و نصب از سرور آروان‌کلود" : "Status: Ready to install from ArvanCloud");
            lv_obj_set_style_text_color(s_pm_meta_status, lv_color_hex(0x4ade80), 0);
        } else if (info && info->scan_error) {
            lv_label_set_text(s_pm_meta_status, is_fa ? "وضعیت: عدم دسترسی به سرور آروان‌کلود" : "Status: Server unreachable");
            lv_obj_set_style_text_color(s_pm_meta_status, lv_color_hex(0xef4444), 0);
        } else {
            lv_label_set_text(s_pm_meta_status, is_fa ? "وضعیت: در حال برقراری ارتباط با سرور..." : "Status: Connecting to server...");
            lv_obj_set_style_text_color(s_pm_meta_status, lv_color_hex(0xf59e0b), 0);
        }
    }

    // ۶. آدرس URL سرور (بدون افشای نام فایل سورس)
    if (s_pm_meta_url && lv_obj_is_valid(s_pm_meta_url)) {
        const char *u = ota_get_lastfile_url(idx);
        if (u && u[0]) {
            char disp_u[128];
            snprintf(disp_u, sizeof(disp_u), "%s", u);
            char *last_sl = strrchr(disp_u, '/');
            if (last_sl && last_sl > disp_u + 8) {
                *last_sl = '\0';
            }
            lv_label_set_text(s_pm_meta_url, disp_u);
        } else {
            lv_label_set_text(s_pm_meta_url, "");
        }
    }

    // ۷. تیتر چنج‌لاگ
    if (s_pm_desc_title && lv_obj_is_valid(s_pm_desc_title)) {
        lv_label_set_text(s_pm_desc_title, is_fa ? "ویژگی‌ها و قابلیت‌های این نسخه:" : (is_tr ? "Bu Surumdeki Yenilikler:" : "Features & Release Notes:"));
    }

    // ۸. توضیحات چنج‌لاگ تجمعی
    if (s_pm_meta_desc && lv_obj_is_valid(s_pm_meta_desc)) {
        char desc_buf[800];
        desc_buf[0] = '\0';
        int d_cnt = 0;
        const char *desc_arr[OTA_DESC_MAX_LINES] = {0};
        int cnt = 0;
        if (ui_lang_get() == UI_LANG_EN && info && info->desc_en_cnt > 0) {
            cnt = info->desc_en_cnt;
            for (int d = 0; d < cnt; d++) desc_arr[d] = info->desc_en[d];
        } else if (ui_lang_get() == UI_LANG_TR && info && info->desc_tr_cnt > 0) {
            cnt = info->desc_tr_cnt;
            for (int d = 0; d < cnt; d++) desc_arr[d] = info->desc_tr[d];
        } else if (info && info->desc_fa_cnt > 0) {
            cnt = info->desc_fa_cnt;
            for (int d = 0; d < cnt; d++) desc_arr[d] = info->desc_fa[d];
        }
        if (cnt > 0) {
            for (int d = 0; d < cnt; d++) {
                const char *line = desc_arr[d];
                if (!line || !line[0]) continue;
                char clean[160];
                ota_sanitize_server_text(clean, sizeof(clean), line);
                const char *p = clean;
                while (*p == ' ' || *p == '\t') p++;
                if (*p == '-' || *p == '*') p++;
                while (*p == ' ' || *p == '\t') p++;
                if (*p) {
                    if (d_cnt > 0) strncat(desc_buf, "\n- ", sizeof(desc_buf) - strlen(desc_buf) - 1);
                    else strncat(desc_buf, "- ", sizeof(desc_buf) - strlen(desc_buf) - 1);
                    strncat(desc_buf, p, sizeof(desc_buf) - strlen(desc_buf) - 1);
                    d_cnt++;
                }
            }
        }
        if (d_cnt == 0) {
            snprintf(desc_buf, sizeof(desc_buf), "%s",
                is_fa ? "توضیحاتی برای این نسخه در سرور ثبت نشده است." :
                (is_tr ? "Bu surum icin degisiklik notu bulunmuyor." : "No changelog notes available for this release."));
        }
        lv_label_set_text(s_pm_meta_desc, desc_buf);
    }
}

void show_ota_pinmap_dialog(int model_idx)
{
    hide_ota_pinmap_dialog();
    s_ota_pinmap_idx = model_idx;
    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    s_ota_pinmap_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_ota_pinmap_popup, 476, 316);
    lv_obj_center(s_ota_pinmap_popup);
    lv_obj_set_style_bg_color(s_ota_pinmap_popup, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(s_ota_pinmap_popup, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(s_ota_pinmap_popup, 2, 0);
    lv_obj_set_style_radius(s_ota_pinmap_popup, 12, 0);
    lv_obj_set_style_clip_corner(s_ota_pinmap_popup, true, 0);
    lv_obj_set_style_pad_all(s_ota_pinmap_popup, 4, 0);
    lv_obj_clear_flag(s_ota_pinmap_popup, LV_OBJ_FLAG_SCROLLABLE);

    // هدر عنوان مودال
    s_pm_title = lv_label_create(s_ota_pinmap_popup);
    lv_obj_set_style_text_font(s_pm_title, ui_lang_font_20(), 0);
    lv_obj_set_style_text_color(s_pm_title, lv_color_hex(0x38bdf8), 0);
    lv_obj_align(s_pm_title, LV_ALIGN_TOP_MID, 0, 4);

    // کانتینر اسکرول‌پذیر عمودی
    s_ota_pinmap_scroll = lv_obj_create(s_ota_pinmap_popup);
    lv_obj_set_size(s_ota_pinmap_scroll, 468, 234);
    lv_obj_set_pos(s_ota_pinmap_scroll, 4, 28);
    lv_obj_set_style_bg_color(s_ota_pinmap_scroll, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_opa(s_ota_pinmap_scroll, 255, 0);
    lv_obj_set_style_border_width(s_ota_pinmap_scroll, 0, 0);
    lv_obj_set_style_pad_all(s_ota_pinmap_scroll, 2, 0);
    lv_obj_set_style_pad_bottom(s_ota_pinmap_scroll, 14, 0);
    lv_obj_set_style_pad_row(s_ota_pinmap_scroll, 8, 0);
    lv_obj_set_style_clip_corner(s_ota_pinmap_scroll, false, 0);
    lv_obj_set_flex_flow(s_ota_pinmap_scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_ota_pinmap_scroll, LV_DIR_VER);
    lv_obj_clear_flag(s_ota_pinmap_scroll, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_anim_duration(s_ota_pinmap_scroll, 0, 0);
    lv_obj_set_scrollbar_mode(s_ota_pinmap_scroll, LV_SCROLLBAR_MODE_OFF);

    // ۱. ردیف کارت‌های ورودی و خروجی (پایه‌ها)
    lv_obj_t *pins_row = lv_obj_create(s_ota_pinmap_scroll);
    lv_obj_set_size(pins_row, 460, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pins_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pins_row, 0, 0);
    lv_obj_set_style_pad_all(pins_row, 0, 0);
    lv_obj_set_style_pad_column(pins_row, 8, 0);
    lv_obj_set_flex_flow(pins_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(pins_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pins_row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    // کارت ستون ورودی‌ها (I)
    lv_obj_t *card_in = lv_obj_create(pins_row);
    lv_obj_set_size(card_in, 226, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card_in, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_color(card_in, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_in, 1, 0);
    lv_obj_set_style_radius(card_in, 10, 0);
    lv_obj_set_style_pad_all(card_in, 5, 0);
    lv_obj_clear_flag(card_in, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *lbl_hdr = lv_label_create(card_in);
        lv_label_set_text(lbl_hdr, is_fa ? "ترمینال‌های ورودی دستگاه (I0 تا I7)" : (is_tr ? "Giris Klemensleri (I0..I7)" : "Input Terminals (I0..I7)"));
        lv_obj_set_style_text_font(lbl_hdr, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0x22c55e), 0);
        lv_obj_set_style_base_dir(lbl_hdr, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        lv_obj_align(lbl_hdr, LV_ALIGN_TOP_MID, 0, 0);

        s_pm_in = lv_label_create(card_in);
        lv_obj_set_width(s_pm_in, 216);
        lv_label_set_long_mode(s_pm_in, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(s_pm_in, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(s_pm_in, ui_lang_font_16(), 0);
        lv_obj_set_style_text_line_space(s_pm_in, -1, 0);
        lv_obj_set_style_base_dir(s_pm_in, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        lv_obj_set_style_text_color(s_pm_in, lv_color_hex(0xf8fafc), 0);
        lv_obj_align(s_pm_in, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 20);
    }

    // کارت ستون خروجی‌ها (Q)
    lv_obj_t *card_out = lv_obj_create(pins_row);
    lv_obj_set_size(card_out, 226, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card_out, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_color(card_out, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_out, 1, 0);
    lv_obj_set_style_radius(card_out, 10, 0);
    lv_obj_set_style_pad_all(card_out, 5, 0);
    lv_obj_clear_flag(card_out, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *lbl_hdr = lv_label_create(card_out);
        lv_label_set_text(lbl_hdr, is_fa ? "ترمینال‌های خروجی رله (Q0 تا Q6)" : (is_tr ? "Role Cikis Klemensleri (Q0..Q6)" : "Relay Output Terminals (Q0..Q6)"));
        lv_obj_set_style_text_font(lbl_hdr, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0xf59e0b), 0);
        lv_obj_set_style_base_dir(lbl_hdr, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        lv_obj_align(lbl_hdr, LV_ALIGN_TOP_MID, 0, 0);

        s_pm_out = lv_label_create(card_out);
        lv_obj_set_width(s_pm_out, 216);
        lv_label_set_long_mode(s_pm_out, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(s_pm_out, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(s_pm_out, ui_lang_font_16(), 0);
        lv_obj_set_style_text_line_space(s_pm_out, -1, 0);
        lv_obj_set_style_base_dir(s_pm_out, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        lv_obj_set_style_text_color(s_pm_out, lv_color_hex(0xf8fafc), 0);
        lv_obj_align(s_pm_out, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 20);
    }

    // ۲. کارت مشخصات و توضیحات سرور (lastfile / فریمور)
    lv_obj_t *card_meta = lv_obj_create(s_ota_pinmap_scroll);
    lv_obj_set_size(card_meta, 460, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card_meta, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_color(card_meta, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_meta, 1, 0);
    lv_obj_set_style_radius(card_meta, 10, 0);
    lv_obj_set_style_pad_all(card_meta, 8, 0);
    lv_obj_clear_flag(card_meta, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *meta_hdr = lv_label_create(card_meta);
        lv_label_set_text(meta_hdr, is_fa ? "مشخصات و تغییرات نسخه (Changelog)" : "Release Specs & Rollup");
        lv_obj_set_style_text_font(meta_hdr, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(meta_hdr, lv_color_hex(0x38bdf8), 0);
        lv_obj_align(meta_hdr, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 0);

        s_pm_meta_ver_build = lv_label_create(card_meta);
        lv_obj_set_style_text_font(s_pm_meta_ver_build, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(s_pm_meta_ver_build, lv_color_hex(0xe2e8f0), 0);
        lv_obj_align(s_pm_meta_ver_build, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 22);

        s_pm_meta_status = lv_label_create(card_meta);
        lv_obj_set_style_text_font(s_pm_meta_status, ui_lang_font_16(), 0);
        lv_obj_align(s_pm_meta_status, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 44);

        s_pm_meta_url = lv_label_create(card_meta);
        lv_obj_set_style_text_font(s_pm_meta_url, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_pm_meta_url, lv_color_hex(0x94a3b8), 0);
        lv_label_set_long_mode(s_pm_meta_url, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(s_pm_meta_url, 440);
        lv_obj_align(s_pm_meta_url, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 66);

        s_pm_desc_title = lv_label_create(card_meta);
        lv_label_set_text(s_pm_desc_title, is_fa ? "ویژگی‌ها و قابلیت‌های این نسخه:" : "Features & Release Notes:");
        lv_obj_set_style_text_font(s_pm_desc_title, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(s_pm_desc_title, lv_color_hex(0xfbbf24), 0);
        lv_obj_align(s_pm_desc_title, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 92);

        s_pm_meta_desc = lv_label_create(card_meta);
        lv_obj_set_width(s_pm_meta_desc, 440);
        lv_label_set_long_mode(s_pm_meta_desc, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(s_pm_meta_desc, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(s_pm_meta_desc, lv_color_hex(0xcbd5e1), 0);
        lv_obj_align(s_pm_meta_desc, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 116);
    }

    // نوار پایین برای کلیدهای عملیاتی با پس‌زمینه سالید ضد همپوشانی (Solid Bottom Footer Guard)
    lv_obj_t *footer_bar = lv_obj_create(s_ota_pinmap_popup);
    lv_obj_set_size(footer_bar, 476, 52);
    lv_obj_align(footer_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer_bar, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(footer_bar, 255, 0);
    lv_obj_set_style_border_width(footer_bar, 0, 0);
    lv_obj_set_style_pad_all(footer_bar, 0, 0);
    lv_obj_clear_flag(footer_bar, LV_OBJ_FLAG_SCROLLABLE);

    // کلید بازگشت (سمت چپ پایین - خاکستری تیره اپل)
    lv_obj_t *bk = lv_btn_create(footer_bar);
    lv_obj_set_size(bk, 140, 36);
    lv_obj_align(bk, LV_ALIGN_BOTTOM_LEFT, 16, -8);
    lv_obj_set_style_bg_color(bk, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(bk, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(bk, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(bk, 1, 0);
    lv_obj_set_style_radius(bk, 10, 0);
    lv_obj_set_style_shadow_width(bk, 0, 0);
    lv_obj_add_event_cb(bk, fw_pinmap_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(bk);
    lv_label_set_text(bl, is_fa ? "بازگشت" : "Back");
    lv_obj_set_style_text_font(bl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(bl, lv_color_hex(0xf8fafc), 0);
    lv_obj_center(bl);

    // کلید نصب مستقیم از داخل پنجره مشخصات (سبز زمردی اپل)
    lv_obj_t *inst = lv_btn_create(footer_bar);
    lv_obj_set_size(inst, 230, 36);
    lv_obj_align(inst, LV_ALIGN_BOTTOM_RIGHT, -16, -8);
    lv_obj_set_style_bg_color(inst, lv_color_hex(0x059669), 0);
    lv_obj_set_style_bg_color(inst, lv_color_hex(0x047857), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(inst, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_border_width(inst, 1, 0);
    lv_obj_set_style_radius(inst, 10, 0);
    lv_obj_set_style_shadow_width(inst, 0, 0);
    lv_obj_add_event_cb(inst, pinmap_install_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *il = lv_label_create(inst);
    lv_label_set_text(il, is_fa ? "نصب این برنامه" : "Install This Firmware");
    lv_obj_set_style_text_font(il, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(il, lv_color_hex(0xffffffff), 0);
    lv_obj_center(il);

    fw_pinmap_update_content(model_idx);
}

/* ---------------- پنجره تبریک نسخه جدید به سبک اپل با لاگ تجمعی (What's New) ---------------- */
static lv_obj_t *s_ota_congrats_modal = NULL;

void hide_ota_update_congrats_dialog(void)
{
    if (s_ota_congrats_modal && lv_obj_is_valid(s_ota_congrats_modal)) {
        lv_obj_del(s_ota_congrats_modal);
        s_ota_congrats_modal = NULL;
        extern void lvgl_port_suppress_touch_until_release(void);
        lvgl_port_suppress_touch_until_release();
    }
}

static void ota_congrats_close_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzz(100);
    hide_ota_update_congrats_dialog();
}

static void add_whats_new_item(lv_obj_t *parent, const char *tag, uint32_t tag_color, const char *text, bool is_fa, bool is_tr)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_remove_style_all(item);
    lv_obj_set_size(item, 406, LV_SIZE_CONTENT);
    lv_obj_set_layout(item, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, is_fa ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(item, 2, 0);
    lv_obj_set_style_pad_bottom(item, 8, 0);
    lv_obj_set_style_border_side(item, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(item, 1, 0);
    lv_obj_set_style_border_color(item, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_base_dir(item, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    // عنوان دسته‌بندی درشت در سمت راست (RTL) بدون کادر رنگی
    lv_obj_t *tag_lbl = lv_label_create(item);
    lv_obj_set_width(tag_lbl, 406);
    lv_label_set_text(tag_lbl, tag);
    lv_obj_set_style_text_font(tag_lbl, is_fa ? ui_lang_font_20() : &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(tag_lbl, lv_color_hex(tag_color), 0);
    lv_obj_set_style_text_align(tag_lbl, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_base_dir(tag_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);

    // متن توضیحات ویژگی با فونت ۱۶ خوانا در تمام عرض با تراز راست در فارسی
    lv_obj_t *txt_lbl = lv_label_create(item);
    lv_obj_set_width(txt_lbl, 406);
    lv_label_set_long_mode(txt_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(txt_lbl, text);
    lv_obj_set_style_text_font(txt_lbl, is_fa ? ui_lang_font_16() : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(txt_lbl, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_text_align(txt_lbl, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_base_dir(txt_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
}

void show_ota_update_congrats_dialog(const char *prev_ver, const char *new_ver)
{
    hide_ota_update_congrats_dialog();

    const char *p_v = (prev_ver && prev_ver[0]) ? prev_ver : "";
    const char *n_v = (new_ver && new_ver[0]) ? new_ver : FIRMWARE_VERSION_STR;
    while (*p_v == 'v' || *p_v == 'V') p_v++;
    while (*n_v == 'v' || *n_v == 'V') n_v++;

    /* تضمین قطعی: اگر نسخه قبلی و جدید یکسان باشد (نصب مجدد همان نسخه)، هرگز این پنجره باز نشود */
    if (p_v[0] == '\0' || n_v[0] == '\0' || strcmp(p_v, n_v) == 0) {
        return;
    }

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    // پس‌زمینه کدر و سریع تمام صفحه (پوشش کامل جهت ممانعت از رندر اضافه و حذف آلفا-بلندینگ سنگین)
    s_ota_congrats_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_ota_congrats_modal, 480, 320);
    lv_obj_set_pos(s_ota_congrats_modal, 0, 0);
    lv_obj_set_style_bg_color(s_ota_congrats_modal, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(s_ota_congrats_modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ota_congrats_modal, 0, 0);
    lv_obj_set_style_pad_all(s_ota_congrats_modal, 0, 0);
    lv_obj_clear_flag(s_ota_congrats_modal, LV_OBJ_FLAG_SCROLLABLE);

    // کارت اصلی دیالوگ تبریک اپل (ابعاد بهینه‌سازی‌شده ۴۶۴x۳۰۴ جهت بیشترین فضای متن)
    lv_obj_t *card = lv_obj_create(s_ota_congrats_modal);
    lv_obj_set_size(card, 464, 304);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // نوار تیتر بالای کارت در یک سطر افقی فشرده (x=8, y=6, w=448, h=28)
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_pos(hdr, 8, 6);
    lv_obj_set_size(hdr, 448, 28);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // بج قرصی موفقیت در سمت راست (یا چپ در LTR)
    lv_obj_t *badge = lv_obj_create(hdr);
    lv_obj_set_size(badge, 270, 26);
    lv_obj_align(badge, is_fa ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_TOP_LEFT, 0, 1);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_radius(badge, 13, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_set_style_shadow_width(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge_lbl = lv_label_create(badge);
    lv_obj_set_style_base_dir(badge_lbl, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_label_set_text(badge_lbl, is_fa ? "ارتقای نسخه با موفقیت انجام شد" : (is_tr ? "Guncelleme Basariyla Tamamlandi" : "Update Completed Successfully"));
    lv_obj_set_style_text_font(badge_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(badge_lbl);

    // بج مسیر ارتقای نسخه در سمت مخالف
    lv_obj_t *ver_badge = lv_obj_create(hdr);
    lv_obj_set_size(ver_badge, 160, 26);
    lv_obj_align(ver_badge, is_fa ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, 1);
    lv_obj_set_style_bg_color(ver_badge, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_color(ver_badge, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(ver_badge, 1, 0);
    lv_obj_set_style_radius(badge, 13, 0);
    lv_obj_set_style_pad_all(ver_badge, 0, 0);
    lv_obj_set_style_shadow_width(ver_badge, 0, 0);
    lv_obj_clear_flag(ver_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ver_lbl = lv_label_create(ver_badge);
    lv_label_set_text_fmt(ver_lbl, "V%s  ->  V%s", p_v, n_v);
    lv_obj_set_style_text_font(ver_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ver_lbl, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_base_dir(ver_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_center(ver_lbl);

    // کادر اسکرول‌دار نکات نسخه جدید (What's New) — حاشیه ۲۴ پیکسلی از راست جهت تفکیک ۱۰۰٪ اسکرول‌بار از متن
    lv_obj_t *scroll = lv_obj_create(card);
    lv_obj_set_pos(scroll, 8, 38);
    lv_obj_set_size(scroll, 448, 216);
    lv_obj_set_style_bg_color(scroll, lv_color_hex(0x0b1120), 0);
    lv_obj_set_style_border_color(scroll, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_width(scroll, 1, 0);
    lv_obj_set_style_radius(scroll, 10, 0);
    lv_obj_set_style_pad_left(scroll, 10, 0);
    lv_obj_set_style_pad_right(scroll, 24, 0);
    lv_obj_set_style_pad_top(scroll, 8, 0);
    lv_obj_set_style_pad_bottom(scroll, 8, 0);
    lv_obj_set_style_shadow_width(scroll, 0, 0);
    lv_obj_set_layout(scroll, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scroll, 8, 0);
    lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(scroll, lv_color_hex(0x475569), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(scroll, 180, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(scroll, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(scroll, 2, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_right(scroll, 3, LV_PART_SCROLLBAR);

    #define COLOR_TOPIC_FIX    0xEF4444  /* برطرف کردن مشکلات فنی (قرمز اپل) */
    #define COLOR_TOPIC_OPT    0x10B981  /* آپشن و امکانات جدید (سبز زمردی) */
    #define COLOR_TOPIC_SPEED  0xF59E0B  /* بهینه‌سازی و سرعت (کهربایی) */
    #define COLOR_TOPIC_STABLE 0x8B5CF6  /* پایداری سیستم (بنفش نیلی) */

    if (is_fa) {
        add_whats_new_item(scroll, "• سرعت و بهینه‌سازی ورود", COLOR_TOPIC_SPEED, "حذف کامل تاخیر ورود به صفحه بروزرسانی و توقف آنی اسکن شبکه", is_fa, is_tr);
        add_whats_new_item(scroll, "• دانلود پرسرعت و پیوسته", COLOR_TOPIC_FIX, "افزایش چشمگیر سرعت دریافت فایل و حذف کامل مکث در میانه دانلود", is_fa, is_tr);
        add_whats_new_item(scroll, "• نمایش دقیق سرعت و زمان", COLOR_TOPIC_OPT, "نمایش کاملاً نرم، خطی و پایدار سرعت دانلود و زمان باقیمانده", is_fa, is_tr);
        add_whats_new_item(scroll, "• هوشمندسازی اعلان ارتقا", COLOR_TOPIC_STABLE, "نمایش پنجره تغییرات فقط در صورت ارتقای واقعی به نسخه جدیدتر", is_fa, is_tr);
    } else if (is_tr) {
        add_whats_new_item(scroll, "• OTA Hizlandirma", COLOR_TOPIC_SPEED, "Guncelleme ekranina giristeki 7 saniyelik gecikme tamamen kaldirildi", is_fa, is_tr);
        add_whats_new_item(scroll, "• Kesintisiz Indirme", COLOR_TOPIC_FIX, "16KB akis parcalari ile yuzde 65 seviyesindeki donma sorunu cozuldu", is_fa, is_tr);
        add_whats_new_item(scroll, "• Akici Hiz Gostergesi", COLOR_TOPIC_OPT, "Indirme hizi ve kalan sure ekranda dalgalanmasiz gosteriliyor", is_fa, is_tr);
        add_whats_new_item(scroll, "• Sistem Kararliligi", COLOR_TOPIC_STABLE, "Guncelleme bildirimi yalnizca yeni surume gecildiginde gosterilir", is_fa, is_tr);
    } else {
        add_whats_new_item(scroll, "• Instant Screen Navigation", COLOR_TOPIC_SPEED, "Eliminated 7-second navigation lag with instant Wi-Fi scan cancelation", is_fa, is_tr);
        add_whats_new_item(scroll, "• Zero-Stall Stream Engine", COLOR_TOPIC_FIX, "16KB chunk size & 32KB socket buffer eliminated 4-second pause at 65%", is_fa, is_tr);
        add_whats_new_item(scroll, "• Smooth Speed Indicator", COLOR_TOPIC_OPT, "Linear EMA-filtered download speed and countdown without jitter", is_fa, is_tr);
        add_whats_new_item(scroll, "• Smart Upgrade Notification", COLOR_TOPIC_STABLE, "Release notes dialog only appears on genuine version upgrades", is_fa, is_tr);
    }

    // کلید شیک "متوجه شدم" در پایین کارت با فاصله اختصاصی ۸ پیکسل از کادر اسکرول
    lv_obj_t *close_btn = lv_btn_create(card);
    lv_obj_set_size(close_btn, 180, 34);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x0369a1), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_border_width(close_btn, 1, 0);
    lv_obj_set_style_radius(close_btn, 8, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_add_event_cb(close_btn, ota_congrats_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(close_btn);
    lv_label_set_text(btn_lbl, is_fa ? "متوجه شدم" : (is_tr ? "Anladim" : "Got it"));
    lv_obj_set_style_text_font(btn_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(btn_lbl);
}

// کیبورد تخصصی ۵ ردیفه آدرس اینترنتی (URL) به سبک آستیاج / اپل با کلیدهای میانبر
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

static void close_url_msgbox(void)
{
    if (s_url_msgbox_timer) {
        lv_timer_del(s_url_msgbox_timer);
        s_url_msgbox_timer = NULL;
    }
    s_url_msgbox_touch_held = false;
    s_url_msgbox_ms_left = 0;
    if (s_url_msgbox && lv_obj_is_valid(s_url_msgbox)) {
        lv_obj_del(s_url_msgbox);
    }
    s_url_msgbox = NULL;
    s_url_msgbox_title = NULL;
    s_url_msgbox_sub = NULL;
    s_url_msgbox_bar = NULL;
}

static void close_custom_url_dialog(void)
{
    close_url_msgbox();
    if (s_custom_url_modal && lv_obj_is_valid(s_custom_url_modal)) {
        lv_obj_del(s_custom_url_modal);
        s_custom_url_modal = NULL;
        s_custom_url_ta = NULL;
        s_custom_url_kb = NULL;
    }
}

static void custom_url_close_cb(lv_event_t *e)
{
    if (e && lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_CANCEL) return;
    buzzer_beep_click();
    close_custom_url_dialog();
}

static void custom_url_clear_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_beep_click();
    if (s_custom_url_ta && lv_obj_is_valid(s_custom_url_ta)) {
        lv_textarea_set_text(s_custom_url_ta, "");
    }
}

static void custom_url_default_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_beep_click();
    if (s_custom_url_ta && lv_obj_is_valid(s_custom_url_ta)) {
        const char *preset_prefix = "http://ota.haffler-infi.ir/other/test/";
        lv_textarea_set_text(s_custom_url_ta, preset_prefix);
        lv_textarea_set_cursor_pos(s_custom_url_ta, LV_TEXTAREA_CURSOR_LAST);
    }
}

// نرمال‌ساز و تکمیل‌کننده هوشمند آدرس لینک طبق استاندارد کارفرما:
// اگر .bin داشت حفظ شود، اگر نداشت منحصراً .bin اضافه شود (بدون افزودن firmware.bin)
static void complete_and_normalize_url(const char *in_url, char *out_url, size_t out_max)
{
    if (!in_url || !out_url || out_max < 16) return;

    char temp[384];
    size_t len = 0;
    while (*in_url && len < 240) {
        temp[len++] = *in_url++;
    }
    temp[len] = '\0';

    // حذف فاصله‌های خالی ابتدا و انتها
    while (len > 0 && isspace((unsigned char)temp[len - 1])) {
        temp[--len] = '\0';
    }

    if (len == 0) {
        out_url[0] = '\0';
        return;
    }

    // افزودن خودکار پروتکل http:// در صورت فقدان http:// یا https://
    if (strncmp(temp, "http://", 7) != 0 && strncmp(temp, "https://", 8) != 0) {
        char with_pfx[384];
        const char pfx[] = "http://";
        size_t pfx_len = sizeof(pfx) - 1;
        memcpy(with_pfx, pfx, pfx_len);
        size_t copy_len = len;
        if (copy_len > sizeof(with_pfx) - pfx_len - 1) {
            copy_len = sizeof(with_pfx) - pfx_len - 1;
        }
        memcpy(with_pfx + pfx_len, temp, copy_len);
        with_pfx[pfx_len + copy_len] = '\0';
        memcpy(temp, with_pfx, pfx_len + copy_len + 1);
        len = pfx_len + copy_len;
    }

    // بررسی وجود پسوند .bin
    bool has_bin = false;
    if (len >= 4) {
        const char *end = temp + len - 4;
        if (strcasecmp(end, ".bin") == 0) {
            has_bin = true;
        }
    }

    // اگر پسوند .bin نداشت: اسلش پایانی (در صورت وجود) حذف و فقط .bin اضافه شود
    if (!has_bin) {
        while (len > 0 && temp[len - 1] == '/') {
            temp[--len] = '\0';
        }
        if (len + 4 < sizeof(temp)) {
            strncat(temp, ".bin", sizeof(temp) - len - 1u);
            len = strlen(temp);
        }
    }

    snprintf(out_url, out_max, "%s", temp);
}

static void url_msgbox_touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_url_msgbox_touch_held = true;
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_url_msgbox_touch_held = false;
        if (s_url_msgbox_ms_left < 2500) {
            s_url_msgbox_ms_left = 2500;
        }
    }
}

static void url_msgbox_timer_cb(lv_timer_t *tmr)
{
    (void)tmr;
    if (s_url_msgbox_touch_held) {
        return; // لمس ممتد مانع از بسته شدن خودکار پیام می‌شود
    }
    s_url_msgbox_ms_left -= 50;
    if (s_url_msgbox_bar && lv_obj_is_valid(s_url_msgbox_bar)) {
        int pct = (s_url_msgbox_ms_left * 100) / 6000;
        if (pct < 0) pct = 0;
        lv_bar_set_value(s_url_msgbox_bar, pct, LV_ANIM_OFF);
    }
    if (s_url_msgbox_ms_left <= 0) {
        close_url_msgbox();
    }
}

static void url_msgbox_close_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_beep_click();
    close_url_msgbox();
}

static void url_msgbox_download_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_beep_confirm();
    if (!wifi_is_connected()) {
        wifi_status_msgbox_show_error(ui_lang_str(STR_WIFI_CONNECT_FIRST));
        return;
    }
    char final_url[256] = {0};
    if (s_last_checked_valid_url[0]) {
        snprintf(final_url, sizeof(final_url), "%s", s_last_checked_valid_url);
    } else if (s_custom_url_ta && lv_obj_is_valid(s_custom_url_ta)) {
        complete_and_normalize_url(lv_textarea_get_text(s_custom_url_ta), final_url, sizeof(final_url));
    }
    if (final_url[0]) {
        ota_set_custom_url(final_url);
    }
    close_url_msgbox();
    close_custom_url_dialog();
    start_ota_install_for_model(OTA_MODEL_CUSTOM);
}

static void show_url_msgbox_checking(const char *url)
{
    close_url_msgbox();
    if (!s_custom_url_modal || !lv_obj_is_valid(s_custom_url_modal)) return;

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    s_url_msgbox = lv_obj_create(s_custom_url_modal);
    lv_obj_set_size(s_url_msgbox, 420, 150);
    lv_obj_center(s_url_msgbox);
    lv_obj_set_style_bg_color(s_url_msgbox, lv_color_hex(0x181e28), 0);
    lv_obj_set_style_bg_opa(s_url_msgbox, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_url_msgbox, 14, 0);
    lv_obj_set_style_border_width(s_url_msgbox, 2, 0);
    lv_obj_set_style_border_color(s_url_msgbox, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(s_url_msgbox, 0, 0);
    lv_obj_clear_flag(s_url_msgbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_url_msgbox, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_url_msgbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_url_msgbox, 10, 0);
    lv_obj_set_style_pad_row(s_url_msgbox, 6, 0);

    lv_obj_add_event_cb(s_url_msgbox, url_msgbox_touch_cb, LV_EVENT_ALL, NULL);

    s_url_msgbox_title = lv_label_create(s_url_msgbox);
    lv_obj_set_style_text_font(s_url_msgbox_title, ui_lang_font_20(), 0);
    lv_obj_set_style_text_color(s_url_msgbox_title, lv_color_hex(0x38bdf8), 0);
    lv_label_set_text(s_url_msgbox_title, is_fa ? LV_SYMBOL_REFRESH " در حال بررسی لینک..." : (is_tr ? LV_SYMBOL_REFRESH " Link kontrol ediliyor..." : LV_SYMBOL_REFRESH " Checking URL..."));
    lv_obj_set_style_base_dir(s_url_msgbox_title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);

    s_url_msgbox_sub = lv_label_create(s_url_msgbox);
    lv_obj_set_style_text_font(s_url_msgbox_sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_url_msgbox_sub, lv_color_hex(0x94a3b8), 0);
    lv_label_set_long_mode(s_url_msgbox_sub, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_url_msgbox_sub, 380);
    lv_obj_set_style_text_align(s_url_msgbox_sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_url_msgbox_sub, url ? url : "");

    lv_obj_t *btn_cancel = lv_btn_create(s_url_msgbox);
    lv_obj_set_size(btn_cancel, 90, 32);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(btn_cancel, 1, 0);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_add_event_cb(btn_cancel, url_msgbox_close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_c = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_c, is_fa ? "انصراف" : (is_tr ? "Iptal" : "Cancel"));
    lv_obj_set_style_text_font(lbl_c, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_c, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_c);

    lv_obj_move_foreground(s_url_msgbox);
}

static void show_url_msgbox_result(bool is_valid, int status_code, uint32_t content_length, const char *err_msg, const char *url)
{
    close_url_msgbox();
    if (!s_custom_url_modal || !lv_obj_is_valid(s_custom_url_modal)) return;

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    s_url_msgbox = lv_obj_create(s_custom_url_modal);
    lv_obj_set_size(s_url_msgbox, 420, 160);
    lv_obj_center(s_url_msgbox);
    lv_obj_set_style_bg_color(s_url_msgbox, lv_color_hex(0x181e28), 0);
    lv_obj_set_style_bg_opa(s_url_msgbox, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_url_msgbox, 14, 0);
    lv_obj_set_style_border_width(s_url_msgbox, 2, 0);
    lv_obj_set_style_border_color(s_url_msgbox, is_valid ? lv_color_hex(0x10b981) : lv_color_hex(0xef4444), 0);
    lv_obj_set_style_shadow_width(s_url_msgbox, 0, 0);
    lv_obj_clear_flag(s_url_msgbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_url_msgbox, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_url_msgbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_url_msgbox, 10, 0);
    lv_obj_set_style_pad_row(s_url_msgbox, 6, 0);

    lv_obj_add_event_cb(s_url_msgbox, url_msgbox_touch_cb, LV_EVENT_ALL, NULL);

    if (is_valid) {
        buzzer_beep_confirm();
        if (url) {
            snprintf(s_last_checked_valid_url, sizeof(s_last_checked_valid_url), "%s", url);
        }

        s_url_msgbox_title = lv_label_create(s_url_msgbox);
        lv_obj_set_style_text_font(s_url_msgbox_title, ui_lang_font_20(), 0);
        lv_obj_set_style_text_color(s_url_msgbox_title, lv_color_hex(0x10b981), 0);
        lv_label_set_text(s_url_msgbox_title, is_fa ? LV_SYMBOL_OK " لینک درسته و معتبر است" : (is_tr ? LV_SYMBOL_OK " Link gecerli" : LV_SYMBOL_OK " Valid Link"));
        lv_obj_set_style_base_dir(s_url_msgbox_title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);

        s_url_msgbox_sub = lv_label_create(s_url_msgbox);
        lv_obj_set_style_text_font(s_url_msgbox_sub, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(s_url_msgbox_sub, lv_color_hex(0xe2e8f0), 0);
        lv_obj_set_width(s_url_msgbox_sub, 380);
        lv_obj_set_style_text_align(s_url_msgbox_sub, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_base_dir(s_url_msgbox_sub, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        if (content_length > 0) {
            float mb = (float)content_length / (1024.0f * 1024.0f);
            if (is_fa) {
                lv_label_set_text_fmt(s_url_msgbox_sub, "حجم فایل: %.2f مگابایت (آماده دریافت)", mb);
            } else if (is_tr) {
                lv_label_set_text_fmt(s_url_msgbox_sub, "Boyut: %.2f MB (Indirmeye hazir)", mb);
            } else {
                lv_label_set_text_fmt(s_url_msgbox_sub, "Size: %.2f MB (Ready to download)", mb);
            }
        } else {
            lv_label_set_text(s_url_msgbox_sub, is_fa ? "فایل روی سرور تایید شد" : (is_tr ? "Dosya hazir" : "File is ready"));
        }

        lv_obj_t *row = lv_obj_create(s_url_msgbox);
        lv_obj_set_size(row, 380, 40);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(row, 0, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);

        lv_obj_t *btn_dl = lv_btn_create(row);
        lv_obj_set_size(btn_dl, 130, 34);
        lv_obj_set_style_radius(btn_dl, 8, 0);
        lv_obj_set_style_bg_color(btn_dl, lv_color_hex(0x059669), 0);
        lv_obj_set_style_bg_color(btn_dl, lv_color_hex(0x047857), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn_dl, 1, 0);
        lv_obj_set_style_border_color(btn_dl, lv_color_hex(0x10b981), 0);
        lv_obj_set_style_shadow_width(btn_dl, 0, 0);
        lv_obj_add_event_cb(btn_dl, url_msgbox_download_btn_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_dl = lv_label_create(btn_dl);
        lv_label_set_text(lbl_dl, is_fa ? "دانلود و نصب" : (is_tr ? "Indir ve Yukle" : "Download"));
        lv_obj_set_style_text_font(lbl_dl, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_dl, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_dl);

        lv_obj_t *btn_cls = lv_btn_create(row);
        lv_obj_set_size(btn_cls, 80, 34);
        lv_obj_set_style_radius(btn_cls, 8, 0);
        lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0x334155), 0);
        lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn_cls, 1, 0);
        lv_obj_set_style_border_color(btn_cls, lv_color_hex(0x475569), 0);
        lv_obj_set_style_shadow_width(btn_cls, 0, 0);
        lv_obj_add_event_cb(btn_cls, url_msgbox_close_btn_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_cls = lv_label_create(btn_cls);
        lv_label_set_text(lbl_cls, is_fa ? "بستن" : (is_tr ? "Kapat" : "Close"));
        lv_obj_set_style_text_font(lbl_cls, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_cls, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_cls);

    } else {
        buzzer_beep_error();
        s_url_msgbox_title = lv_label_create(s_url_msgbox);
        lv_obj_set_style_text_font(s_url_msgbox_title, ui_lang_font_20(), 0);
        lv_obj_set_style_text_color(s_url_msgbox_title, lv_color_hex(0xef4444), 0);
        lv_label_set_text(s_url_msgbox_title, is_fa ? LV_SYMBOL_WARNING " لینک درست نیست" : (is_tr ? LV_SYMBOL_WARNING " Link gecersiz" : LV_SYMBOL_WARNING " Invalid URL"));
        lv_obj_set_style_base_dir(s_url_msgbox_title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);

        s_url_msgbox_sub = lv_label_create(s_url_msgbox);
        lv_obj_set_style_text_font(s_url_msgbox_sub, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(s_url_msgbox_sub, lv_color_hex(0xfca5a5), 0);
        lv_obj_set_width(s_url_msgbox_sub, 380);
        lv_obj_set_style_text_align(s_url_msgbox_sub, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_base_dir(s_url_msgbox_sub, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
        if (status_code == 404) {
            lv_label_set_text(s_url_msgbox_sub, is_fa ? "فایل روی سرور یافت نشد (خطای ۴۰۴)" : (is_tr ? "Dosya bulunamadi (404)" : "File not found on server (404)"));
        } else if (err_msg && err_msg[0]) {
            lv_label_set_text(s_url_msgbox_sub, err_msg);
        } else {
            lv_label_set_text(s_url_msgbox_sub, is_fa ? "پاسخی از سرور دریافت نشد" : (is_tr ? "Sunucu yanit vermedi" : "No response from server"));
        }

        lv_obj_t *btn_cls = lv_btn_create(s_url_msgbox);
        lv_obj_set_size(btn_cls, 100, 34);
        lv_obj_set_style_radius(btn_cls, 8, 0);
        lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0x334155), 0);
        lv_obj_set_style_bg_color(btn_cls, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn_cls, 1, 0);
        lv_obj_set_style_border_color(btn_cls, lv_color_hex(0x475569), 0);
        lv_obj_set_style_shadow_width(btn_cls, 0, 0);
        lv_obj_add_event_cb(btn_cls, url_msgbox_close_btn_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_cls = lv_label_create(btn_cls);
        lv_label_set_text(lbl_cls, is_fa ? "بستن" : (is_tr ? "Kapat" : "Close"));
        lv_obj_set_style_text_font(lbl_cls, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_cls, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_cls);
    }

    s_url_msgbox_bar = lv_bar_create(s_url_msgbox);
    lv_obj_set_size(s_url_msgbox_bar, 380, 4);
    lv_bar_set_range(s_url_msgbox_bar, 0, 100);
    lv_bar_set_value(s_url_msgbox_bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_url_msgbox_bar, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_url_msgbox_bar, is_valid ? lv_color_hex(0x10b981) : lv_color_hex(0xef4444), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_url_msgbox_bar, 2, 0);

    s_url_msgbox_ms_left = 6000;
    s_url_msgbox_touch_held = false;
    s_url_msgbox_timer = lv_timer_create(url_msgbox_timer_cb, 50, NULL);

    lv_obj_move_foreground(s_url_msgbox);
}

static void on_url_check_result_ui_cb(bool is_valid, int status_code, uint32_t content_length, const char *err_msg)
{
    if (!s_custom_url_modal || !lv_obj_is_valid(s_custom_url_modal)) return;

    char curr_url[256] = {0};
    if (s_custom_url_ta && lv_obj_is_valid(s_custom_url_ta)) {
        const char *t = lv_textarea_get_text(s_custom_url_ta);
        if (t) snprintf(curr_url, sizeof(curr_url), "%s", t);
    }

    show_url_msgbox_result(is_valid, status_code, content_length, err_msg, curr_url);
}

static void custom_url_check_cb(lv_event_t *e)
{
    if (e && lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_READY) return;
    buzzer_beep_click();

    char norm_url[256] = {0};
    if (s_custom_url_ta && lv_obj_is_valid(s_custom_url_ta)) {
        const char *txt = lv_textarea_get_text(s_custom_url_ta);
        if (txt && strlen(txt) > 0) {
            complete_and_normalize_url(txt, norm_url, sizeof(norm_url));
            if (norm_url[0]) {
                lv_textarea_set_text(s_custom_url_ta, norm_url);
                lv_textarea_set_cursor_pos(s_custom_url_ta, LV_TEXTAREA_CURSOR_LAST);
            }
        }
    }

    if (!norm_url[0]) return;

    show_url_msgbox_checking(norm_url);
    ota_check_url_async(norm_url, on_url_check_result_ui_cb);
}

static void custom_url_install_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED && lv_event_get_code(e) != LV_EVENT_READY) return;
    buzzer_beep_click();
    if (!wifi_is_connected()) {
        wifi_status_msgbox_show_error(ui_lang_str(STR_WIFI_CONNECT_FIRST));
        return;
    }
    const char *final_url = "http://ota.haffler-infi.ir/other/ice-cream/firmware.bin";
    char norm_url[256];
    if (s_last_checked_valid_url[0]) {
        final_url = s_last_checked_valid_url;
    } else if (s_custom_url_ta && lv_obj_is_valid(s_custom_url_ta)) {
        const char *txt = lv_textarea_get_text(s_custom_url_ta);
        if (txt && strlen(txt) > 0) {
            complete_and_normalize_url(txt, norm_url, sizeof(norm_url));
            if (norm_url[0]) {
                final_url = norm_url;
            }
        }
    }
    ota_set_custom_url(final_url);
    close_url_msgbox();
    close_custom_url_dialog();
    start_ota_install_for_model(OTA_MODEL_CUSTOM);
}

static void event_handler_link_entry_keyboard(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_READY && code != LV_EVENT_CANCEL && code != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *kb = lv_event_get_target(e);
    if (code == LV_EVENT_READY) {
        // فشردن دکمه تایید/اینتر روی کیبورد: بررسی لینک بدون خروج از صفحه
        custom_url_check_cb(e);
    } else if (code == LV_EVENT_CANCEL) {
        buzzer_beep_click();
        custom_url_close_cb(e);
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
        const char *txt = lv_buttonmatrix_get_button_text(kb, btn_id);
        if (txt) {
            if (strcmp(txt, "ABC") == 0) {
                lv_buttonmatrix_set_map(kb, cfg_url_kb_map_uc);
                lv_buttonmatrix_set_ctrl_map(kb, cfg_url_kb_ctrl_uc);
            } else if (strcmp(txt, "abc") == 0) {
                lv_buttonmatrix_set_map(kb, cfg_url_kb_map_lc);
                lv_buttonmatrix_set_ctrl_map(kb, cfg_url_kb_ctrl_lc);
            }
        }
    }
}

void open_custom_url_dialog(void);

void open_custom_url_dialog_cb(lv_event_t *e)
{
    if (e != NULL && lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_beep_click();

    close_custom_url_dialog();

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    s_custom_url_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_custom_url_modal, 480, 320);
    lv_obj_set_pos(s_custom_url_modal, 0, 0);
    lv_obj_set_style_bg_color(s_custom_url_modal, lv_color_hex(0x1e2024), 0);
    lv_obj_set_style_border_width(s_custom_url_modal, 0, 0);
    lv_obj_set_style_radius(s_custom_url_modal, 0, 0);
    lv_obj_set_style_pad_all(s_custom_url_modal, 0, 0);
    lv_obj_clear_flag(s_custom_url_modal, LV_OBJ_FLAG_SCROLLABLE);

    // سطر بالای کلیدهای عملیاتی (y=6)
    // ۱. کلید برگشت (Apple Slate)
    lv_obj_t *btn_bk = lv_btn_create(s_custom_url_modal);
    lv_obj_set_pos(btn_bk, 6, 6);
    lv_obj_set_size(btn_bk, 76, 32);
    lv_obj_set_style_radius(btn_bk, 8, 0);
    lv_obj_set_style_bg_color(btn_bk, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(btn_bk, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_bk, 1, 0);
    lv_obj_set_style_border_color(btn_bk, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_bk, 0, 0);
    lv_obj_add_event_cb(btn_bk, custom_url_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_bk = lv_label_create(btn_bk);
    lv_label_set_text(lbl_bk, is_fa ? LV_SYMBOL_LEFT " برگشت" : (is_tr ? LV_SYMBOL_LEFT " Geri" : LV_SYMBOL_LEFT " Back"));
    lv_obj_set_style_text_font(lbl_bk, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_bk, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_bk);

    // ۲. کلید سطل زباله (پاک‌کردن متن) — قرمز استاندارد آستیاج
    lv_obj_t *btn_clr = lv_btn_create(s_custom_url_modal);
    lv_obj_set_pos(btn_clr, 86, 6);
    lv_obj_set_size(btn_clr, 34, 32);
    lv_obj_set_style_radius(btn_clr, 8, 0);
    lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0xb91c1c), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_clr, 1, 0);
    lv_obj_set_style_border_color(btn_clr, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_shadow_width(btn_clr, 0, 0);
    lv_obj_add_event_cb(btn_clr, custom_url_clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, LV_SYMBOL_TRASH);
    lv_obj_set_style_text_font(lbl_clr, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_clr, lv_color_hex(0xffffffff), 0);
    lv_obj_center(lbl_clr);

    // ۳. درج لینک پیش‌فرض سرور (other/test)
    lv_obj_t *btn_def = lv_btn_create(s_custom_url_modal);
    lv_obj_set_pos(btn_def, 124, 6);
    lv_obj_set_size(btn_def, 118, 32);
    lv_obj_set_style_radius(btn_def, 8, 0);
    lv_obj_set_style_bg_color(btn_def, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(btn_def, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_width(btn_def, 1, 0);
    lv_obj_set_style_shadow_width(btn_def, 0, 0);
    lv_obj_add_event_cb(btn_def, custom_url_default_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_def = lv_label_create(btn_def);
    lv_label_set_text(lbl_def, is_fa ? "لینک پیش‌فرض" : (is_tr ? "Varsayilan" : "Default URL"));
    lv_obj_set_style_text_font(lbl_def, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_def, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_base_dir(lbl_def, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_obj_center(lbl_def);

    // ۴. کلید تایید و بررسی صحت لینک (سبز زمردی اپل)
    lv_obj_t *btn_chk = lv_btn_create(s_custom_url_modal);
    lv_obj_set_pos(btn_chk, 246, 6);
    lv_obj_set_size(btn_chk, 94, 32);
    lv_obj_set_style_radius(btn_chk, 8, 0);
    lv_obj_set_style_bg_color(btn_chk, lv_color_hex(0x16a34a), 0);
    lv_obj_set_style_bg_color(btn_chk, lv_color_hex(0x15803d), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_chk, 1, 0);
    lv_obj_set_style_border_color(btn_chk, lv_color_hex(0x22c55e), 0);
    lv_obj_set_style_shadow_width(btn_chk, 0, 0);
    lv_obj_add_event_cb(btn_chk, custom_url_check_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_chk = lv_label_create(btn_chk);
    lv_label_set_text(lbl_chk, is_fa ? "تایید" : (is_tr ? "Kontrol" : "Check"));
    lv_obj_set_style_text_font(lbl_chk, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_chk, lv_color_hex(0xffffffff), 0);
    lv_obj_center(lbl_chk);

    // ۵. کلید دانلود و نصب مستقیم
    lv_obj_t *btn_ok = lv_btn_create(s_custom_url_modal);
    lv_obj_set_pos(btn_ok, 344, 6);
    lv_obj_set_size(btn_ok, 130, 32);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x059669), 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x047857), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_ok, 1, 0);
    lv_obj_set_style_border_color(btn_ok, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_shadow_width(btn_ok, 0, 0);
    lv_obj_add_event_cb(btn_ok, custom_url_install_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, is_fa ? "دانلود و نصب" : (is_tr ? "Indir ve Yukle" : "Download"));
    lv_obj_set_style_text_font(lbl_ok, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_ok, lv_color_hex(0xffffffff), 0);
    lv_obj_center(lbl_ok);

    // کادر ورودی آدرس لینک (y=42)
    s_custom_url_ta = lv_textarea_create(s_custom_url_modal);
    lv_obj_set_pos(s_custom_url_ta, 8, 42);
    lv_obj_set_size(s_custom_url_ta, 464, 34);
    lv_textarea_set_one_line(s_custom_url_ta, true);
    lv_textarea_set_max_length(s_custom_url_ta, 256);
    lv_obj_set_style_radius(s_custom_url_ta, 8, 0);
    lv_obj_set_style_bg_color(s_custom_url_ta, lv_color_hex(0x09090b), 0);
    lv_obj_set_style_border_color(s_custom_url_ta, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_width(s_custom_url_ta, 1, 0);
    lv_obj_set_style_pad_left(s_custom_url_ta, 8, 0);
    lv_obj_set_style_pad_right(s_custom_url_ta, 8, 0);
    lv_obj_set_style_pad_top(s_custom_url_ta, 6, 0);
    lv_obj_set_style_pad_bottom(s_custom_url_ta, 6, 0);
    lv_obj_set_style_text_font(s_custom_url_ta, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_custom_url_ta, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_base_dir(s_custom_url_ta, LV_BASE_DIR_LTR, 0);

    const char *curr_url = ota_get_custom_url();
    if (curr_url && curr_url[0]) {
        lv_textarea_set_text(s_custom_url_ta, curr_url);
    } else {
        lv_textarea_set_text(s_custom_url_ta, "http://ota.haffler-infi.ir/other/test/");
    }
    lv_textarea_set_cursor_pos(s_custom_url_ta, LV_TEXTAREA_CURSOR_LAST);

    // کیبورد مجازی ۵ ردیفه اختصاصی وب (y=80)
    s_custom_url_kb = lv_keyboard_create(s_custom_url_modal);
    lv_obj_set_align(s_custom_url_kb, LV_ALIGN_DEFAULT);
    lv_obj_set_pos(s_custom_url_kb, 0, 80);
    lv_obj_set_size(s_custom_url_kb, 480, 238);
    lv_obj_clear_flag(s_custom_url_kb, LV_OBJ_FLAG_SCROLLABLE);
    lv_keyboard_set_map(s_custom_url_kb, LV_KEYBOARD_MODE_TEXT_LOWER, cfg_url_kb_map_lc, cfg_url_kb_ctrl_lc);
    lv_keyboard_set_map(s_custom_url_kb, LV_KEYBOARD_MODE_TEXT_UPPER, cfg_url_kb_map_uc, cfg_url_kb_ctrl_uc);
    lv_keyboard_set_mode(s_custom_url_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_custom_url_kb, s_custom_url_ta);
    lv_obj_add_event_cb(s_custom_url_kb, event_handler_link_entry_keyboard, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(s_custom_url_kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(s_custom_url_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_set_style_bg_color(s_custom_url_kb, lv_color_hex(0x1e2024), 0);
    lv_obj_set_style_bg_opa(s_custom_url_kb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_custom_url_kb, 0, 0);
    lv_obj_set_style_pad_all(s_custom_url_kb, 2, 0);
    lv_obj_set_style_pad_row(s_custom_url_kb, 3, 0);
    lv_obj_set_style_pad_column(s_custom_url_kb, 3, 0);
    lv_obj_set_style_bg_color(s_custom_url_kb, lv_color_hex(0xffffffff), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_custom_url_kb, lv_color_hex(0x38bdf8), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(s_custom_url_kb, lv_color_hex(0x0f172a), LV_PART_ITEMS);
    lv_obj_set_style_radius(s_custom_url_kb, 6, LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_custom_url_kb, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_custom_url_kb, lv_color_hex(0xcbd5e1), LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(s_custom_url_kb, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_custom_url_kb, &lv_font_montserrat_16, LV_PART_ITEMS);
}

void open_custom_url_dialog(void)
{
    open_custom_url_dialog_cb(NULL);
}

void create_screen_screen_framware(void)
{
    lv_obj_t *scr = lv_obj_create(0);
    objects.screen_framware = scr;
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_size(scr, 480, 320);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x181a1f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    lv_obj_t *cards_row = lv_obj_create(scr);
    lv_obj_set_pos(cards_row, 4, 2);
    lv_obj_set_size(cards_row, 472, 316);
    lv_obj_set_scrollbar_mode(cards_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(cards_row, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(cards_row, LV_SCROLL_SNAP_START);
    if (g_boot_f5_service_mode) {
        lv_obj_add_flag(cards_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    } else {
        lv_obj_clear_flag(cards_row, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_clear_flag(cards_row, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_anim_duration(cards_row, 180, 0);
    lv_obj_set_flex_flow(cards_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_base_dir(cards_row, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_flex_main_place(cards_row, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_style_flex_cross_place(cards_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(cards_row, 8, 0);
    lv_obj_set_style_pad_all(cards_row, 6, 0);
    lv_obj_set_style_bg_color(cards_row, lv_color_hex(0x181a1f), 0);
    lv_obj_set_style_bg_opa(cards_row, 255, 0);
    lv_obj_set_style_border_width(cards_row, 0, 0);

    // ===== کارت ۰: برند ASTIAJ + نسخه + لوگو + بازگشت =====
    lv_obj_t *c0 = lv_obj_create(cards_row);
    lv_obj_set_size(c0, 226, 304);
    lv_obj_clear_flag(c0, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(c0, lv_color_hex(0x23272e), 0);
    lv_obj_set_style_radius(c0, 12, 0);
    lv_obj_set_style_pad_all(c0, 0, 0);
    lv_obj_set_style_border_width(c0, 1, 0);
    lv_obj_set_style_border_color(c0, lv_color_hex(0x374151), 0);
    lv_obj_set_style_shadow_width(c0, 0, 0);
    {
        // عنوان ASTIAG با رنگ سفید
        lv_obj_t *hdr = lv_label_create(c0);
        lv_label_set_text(hdr, "ASTIAG");
        lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 12);
        lv_obj_set_style_text_color(hdr, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(hdr, &lv_font_montserrat_24, 0);

        // خط افقی فیروزه‌ای اپل
        lv_obj_t *line = lv_obj_create(c0);
        lv_obj_set_size(line, 180, 2);
        lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 44);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(line, lv_color_hex(0x38bdf8), 0);
        lv_obj_set_style_border_width(line, 0, 0);

        // نسخه‌ی جاری فریمور
        lv_obj_t *ver = lv_label_create(c0);
        lv_label_set_text(ver, FIRMWARE_VERSION_STR);
        lv_obj_align(ver, LV_ALIGN_TOP_MID, 0, 54);
        lv_obj_set_style_text_color(ver, lv_color_hex(0x94a3b8), 0);
        lv_obj_set_style_text_font(ver, &lv_font_montserrat_16, 0);

        // کلید ورود با لینک
        lv_obj_t *btn_link = lv_btn_create(c0);
        lv_obj_set_size(btn_link, 190, 36);
        lv_obj_align(btn_link, LV_ALIGN_TOP_MID, 0, 84);
        lv_obj_set_style_radius(btn_link, 8, 0);
        lv_obj_set_style_bg_color(btn_link, lv_color_hex(0x10b981), 0);
        lv_obj_set_style_bg_color(btn_link, lv_color_hex(0x059669), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_link, lv_color_hex(0x22c55e), 0);
        lv_obj_set_style_border_width(btn_link, 1, 0);
        lv_obj_set_style_shadow_width(btn_link, 0, 0);
        lv_obj_add_event_cb(btn_link, open_custom_url_dialog_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_link = lv_label_create(btn_link);
        lv_label_set_text(lbl_link, is_fa ? "ورود لینک" : (is_tr ? "Baglanti Girisi" : "Direct URL"));
        lv_obj_set_style_text_font(lbl_link, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_link, lv_color_hex(0xffffffff), 0);
        lv_obj_center(lbl_link);

        // لوگوی رسمی شارپ و زیبای آستیاژ با نوشته سفید در زیر برای تم دارک
        lv_obj_t *logo = lv_image_create(c0);
        lv_image_set_src(logo, &img_astiaj_logo_white_img);
        lv_image_set_scale(logo, 256); // مقیاس ۱:۱ کاملاً واضح بدون تاری
        lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 138);

        // کلید قرمز بازگشت اپل
        lv_obj_t *btn_back = lv_btn_create(c0);
        lv_obj_set_size(btn_back, 190, 42);
        lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -14);
        lv_obj_set_style_radius(btn_back, 8, 0);
        lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xdc2626), 0);
        lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xb91c1c), LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(btn_back, 0, 0);
        lv_obj_add_event_cb(btn_back, action_framware_back, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_back = lv_label_create(btn_back);
        lv_label_set_text(lbl_back, is_fa ? "بازگشت" : (is_tr ? "Geri" : "Back"));
        lv_obj_set_style_text_font(lbl_back, ui_lang_font_20(), 0);
        lv_obj_set_style_text_color(lbl_back, lv_color_hex(0xffffffff), 0);
        lv_obj_center(lbl_back);
    }

    // ===== کارت‌های ۱ و ۲: برنامه‌ها =====
    const char *card_titles[OTA_MODEL_COUNT] = {
        is_fa ? "برنامه تولید بستنی" : (is_tr ? "Dondurma Uretim Yazilimi" : "Ice Cream Main App"),
        is_fa ? "برنامه تست و کانفیگ" : (is_tr ? "Test ve Yapilandirma" : "Test & Config App")
    };
    const lv_img_dsc_t *card_icons[OTA_MODEL_COUNT] = {
        (const lv_image_dsc_t *)&img_bitmap_ice_cream,
        (const lv_image_dsc_t *)&img_bitmap_config
    };

    /* ایجاد هر دو برنامه (تولید بستنی و تست/کانفیگ) جهت حفظ پایداری حافظه و پوینترها */
    for (int i = 0; i < OTA_MODEL_COUNT; i++) {
        lv_obj_t *card = lv_obj_create(cards_row);
        s_fw_card_obj[i] = card;
        lv_obj_set_size(card, 226, 304);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x23272e), 0);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x374151), 0);
        lv_obj_set_style_shadow_width(card, 0, 0);

        // عنوان برنامه
        lv_obj_t *title = lv_label_create(card);
        lv_label_set_text(title, card_titles[i]);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_set_style_text_color(title, lv_color_hex(0xffffffff), 0);
        lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
        lv_obj_set_style_base_dir(title, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);

        // تصویر محصول به صورت شناور با کیفیت بالا (210x210)
        lv_obj_t *im = lv_image_create(card);
        lv_image_set_src(im, card_icons[i]);
        lv_obj_align(im, LV_ALIGN_TOP_MID, 0, 30);

        // وضعیت نسخه سرور به سبک مینیمال V... بدون باگ BiDi
        s_fw_card_ver_lbl[i] = lv_label_create(card);
        lv_obj_set_size(s_fw_card_ver_lbl[i], 218, LV_SIZE_CONTENT);
        lv_obj_align(s_fw_card_ver_lbl[i], LV_ALIGN_TOP_MID, 0, 240);
        lv_obj_set_style_text_align(s_fw_card_ver_lbl[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(s_fw_card_ver_lbl[i], lv_color_hex(0x94a3b8), 0);
        lv_obj_set_style_text_font(s_fw_card_ver_lbl[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_base_dir(s_fw_card_ver_lbl[i], LV_BASE_DIR_LTR, 0);

        const lastfile_info_t *cached_info = ota_get_info(i);
        if (cached_info && cached_info->ver[0]) {
            const char *c_ver = (FIRMWARE_VERSION_STR[0] == 'v' || FIRMWARE_VERSION_STR[0] == 'V') ? FIRMWARE_VERSION_STR + 1 : FIRMWARE_VERSION_STR;
            const char *s_ver = (cached_info->ver[0] == 'v' || cached_info->ver[0] == 'V') ? cached_info->ver + 1 : cached_info->ver;
            lv_label_set_text_fmt(s_fw_card_ver_lbl[i], "V%s -> V%s", c_ver, s_ver);
            if (i == 0) {
                int cmp = compare_semantic_versions(cached_info->ver, FIRMWARE_VERSION_STR);
                if (cmp > 0) lv_obj_set_style_text_color(s_fw_card_ver_lbl[0], lv_color_hex(0x38bdf8), 0);
                else if (cmp == 0) lv_obj_set_style_text_color(s_fw_card_ver_lbl[0], lv_color_hex(0x4ade80), 0);
                else lv_obj_set_style_text_color(s_fw_card_ver_lbl[0], lv_color_hex(0xf59e0b), 0);
            }
        } else {
            lv_label_set_text(s_fw_card_ver_lbl[i], "...");
        }

        // دکمه اکشن اصلی: نصب (سبز زمردی اپل)
        lv_obj_t *btn_inst = lv_btn_create(card);
        s_fw_card_btn_inst[i] = btn_inst;
        lv_obj_set_size(btn_inst, 104, 30);
        lv_obj_align(btn_inst, LV_ALIGN_BOTTOM_LEFT, 8, -8);
        lv_obj_set_style_radius(btn_inst, 8, 0);
        lv_obj_set_style_bg_color(btn_inst, lv_color_hex(0x10b981), 0);
        lv_obj_set_style_bg_color(btn_inst, lv_color_hex(0x059669), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_inst, lv_color_hex(0x22c55e), 0);
        lv_obj_set_style_border_width(btn_inst, 1, 0);
        lv_obj_set_style_shadow_width(btn_inst, 0, 0);
        lv_obj_add_event_cb(btn_inst, framware_install_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *lbl_inst = lv_label_create(btn_inst);
        s_fw_card_btn_inst_lbl[i] = lbl_inst;
        lv_label_set_text(lbl_inst, is_fa ? "نصب" : (is_tr ? "Yukle" : "Install"));
        lv_label_set_long_mode(lbl_inst, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(lbl_inst, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_inst, lv_color_hex(0xffffffff), 0);
        lv_obj_center(lbl_inst);

        // دکمه اکشن ثانویه: مشخصات (Dark Slate متین اپل)
        lv_obj_t *btn_info = lv_btn_create(card);
        lv_obj_set_size(btn_info, 102, 30);
        lv_obj_align(btn_info, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
        lv_obj_set_style_radius(btn_info, 8, 0);
        lv_obj_set_style_bg_color(btn_info, lv_color_hex(0x334155), 0);
        lv_obj_set_style_bg_color(btn_info, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn_info, lv_color_hex(0x475569), 0);
        lv_obj_set_style_border_width(btn_info, 1, 0);
        lv_obj_set_style_shadow_width(btn_info, 0, 0);
        lv_obj_add_event_cb(btn_info, fw_pinmap_card_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *lbl_info = lv_label_create(btn_info);
        lv_label_set_text(lbl_info, is_fa ? "مشخصات" : (is_tr ? "Ozellikler" : "Specs"));
        lv_obj_set_style_text_font(lbl_info, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xcbd5e1), 0);
        lv_obj_center(lbl_info);

        /* پنهان‌سازی آنی کارت برنامه تست در لحظه ساخت در مد عادی F4 جهت ممانعت از ری‌فلو و لرزش UI */
        if (i == 1 && !g_boot_f5_service_mode) {
            lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* پایش نهایی وضعیت نمایش برنامه کانفیگ */
    if (s_fw_card_obj[1] && lv_obj_is_valid(s_fw_card_obj[1])) {
        if (g_boot_f5_service_mode) {
            lv_obj_clear_flag(s_fw_card_obj[1], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_fw_card_obj[1], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void delete_screen_screen_framware(void)
{
    hide_ota_pinmap_dialog();
    close_custom_url_dialog();
    ota_cancel_install();
    if (s_ota_progress_modal && lv_obj_is_valid(s_ota_progress_modal)) {
        lv_obj_del(s_ota_progress_modal);
        s_ota_progress_modal = NULL;
        s_ota_progress_bar = NULL;
        s_ota_progress_lbl = NULL;
        s_ota_progress_status = NULL;
        s_ota_progress_circle = NULL;
        s_ota_progress_icon = NULL;
        s_ota_progress_close_btn = NULL;
        s_ota_progress_close_lbl = NULL;
        s_last_rendered_inst_state = OTA_INST_IDLE;
    }
    s_last_rendered_scan_rev = 0xFFFFFFFF;
    for (int i = 0; i < OTA_MODEL_COUNT; i++) {
        s_fw_card_obj[i] = NULL;
        s_fw_card_ver_lbl[i] = NULL;
        s_fw_card_btn_inst[i] = NULL;
        s_fw_card_btn_inst_lbl[i] = NULL;
    }
    s_ota_card_rem_pct_val = NULL;
    s_ota_card_size_val = NULL;
    s_ota_card_speed_val = NULL;
    s_ota_card_eta_val = NULL;
    s_ota_main_pct_lbl = NULL;
    s_ota_pinmap_popup = NULL;
    s_ota_pinmap_scroll = NULL;
    s_ota_pinmap_content = NULL;
    s_pm_title = NULL;
    s_pm_in = NULL;
    s_pm_out = NULL;
    s_pm_meta_ver_build = NULL;
    s_pm_meta_status = NULL;
    s_pm_meta_url = NULL;
    s_ota_smooth_eta_sec = 0.0f;
    s_ota_eta_last_sec_tick = 0;
    s_ota_speed_kbps = 0.0f;
    s_ota_speed_last_time = 0;
    s_ota_speed_last_bytes = 0;
    s_ota_last_scan_done = false;
    s_ota_last_tick_scan = 0;

    if (objects.screen_framware && lv_obj_is_valid(objects.screen_framware)) {
        lv_obj_del(objects.screen_framware);
        objects.screen_framware = NULL;
    }
}

static bool s_last_rendered_scan_done = false;
static int s_last_rendered_lang = -1;

void screens_framware_on_load(void)
{
    s_last_rendered_scan_rev = 0xFFFFFFFF;
    s_last_rendered_scan_done = false;
    s_last_rendered_lang = -1;
}

void tick_screen_screen_framware(void)
{
    /* پایش پیوسته وضعیت نمایش برنامه کانفیگ بر اساس مد ورود (F5 بوت در برابر F4 عادی) */
    if (s_fw_card_obj[1] && lv_obj_is_valid(s_fw_card_obj[1])) {
        if (g_boot_f5_service_mode) {
            if (lv_obj_has_flag(s_fw_card_obj[1], LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_clear_flag(s_fw_card_obj[1], LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            if (!lv_obj_has_flag(s_fw_card_obj[1], LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(s_fw_card_obj[1], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    bool scan_done = ota_is_scan_done();
    uint32_t scan_rev = ota_get_scan_revision();
    uint32_t now = lv_tick_get();
    bool is_fa = (ui_lang_get() == UI_LANG_FA);
    bool is_tr = (ui_lang_get() == UI_LANG_TR);

    int cur_lang = (int)ui_lang_get();
    bool lang_changed = (cur_lang != s_last_rendered_lang);
    if (lang_changed) s_last_rendered_lang = cur_lang;

    if (scan_rev != s_last_rendered_scan_rev || scan_done != s_last_rendered_scan_done || lang_changed) {
        s_last_rendered_scan_rev = scan_rev;
        s_last_rendered_scan_done = scan_done;
        s_ota_last_scan_done = scan_done;
        s_ota_last_tick_scan = now;

        for (int i = 0; i < OTA_MODEL_COUNT; i++) {
            if (!s_fw_card_ver_lbl[i] || !lv_obj_is_valid(s_fw_card_ver_lbl[i])) continue;
            const lastfile_info_t *info = ota_get_info(i);
            if (info && info->ver[0]) {
                const char *c_ver = (FIRMWARE_VERSION_STR[0] == 'v' || FIRMWARE_VERSION_STR[0] == 'V') ? FIRMWARE_VERSION_STR + 1 : FIRMWARE_VERSION_STR;
                const char *s_ver = (info->ver[0] == 'v' || info->ver[0] == 'V') ? info->ver + 1 : info->ver;
                lv_label_set_text_fmt(s_fw_card_ver_lbl[i], "V%s -> V%s", c_ver, s_ver);
                lv_obj_set_style_text_font(s_fw_card_ver_lbl[i], &lv_font_montserrat_16, 0);
                lv_obj_set_style_base_dir(s_fw_card_ver_lbl[i], LV_BASE_DIR_LTR, 0);

                if (i == 0 && s_fw_card_btn_inst_lbl[0] && lv_obj_is_valid(s_fw_card_btn_inst_lbl[0])) {
                    int cmp = compare_semantic_versions(info->ver, FIRMWARE_VERSION_STR);
                    if (cmp > 0) {
                        lv_obj_set_style_text_color(s_fw_card_ver_lbl[0], lv_color_hex(0x38bdf8), 0);
                        lv_label_set_text(s_fw_card_btn_inst_lbl[0], ui_lang_str(STR_OTA_CARD_UPDATE_BTN));
                        if (s_fw_card_btn_inst[0] && lv_obj_is_valid(s_fw_card_btn_inst[0])) {
                            lv_obj_set_style_bg_color(s_fw_card_btn_inst[0], lv_color_hex(0x10b981), 0);
                            lv_obj_set_style_border_color(s_fw_card_btn_inst[0], lv_color_hex(0x22c55e), 0);
                        }
                    } else if (cmp == 0) {
                        lv_obj_set_style_text_color(s_fw_card_ver_lbl[0], lv_color_hex(0x4ade80), 0);
                        lv_label_set_text(s_fw_card_btn_inst_lbl[0], is_fa ? "نصب مجدد" : (is_tr ? "Tekrar Yukle" : "Reinstall"));
                        if (s_fw_card_btn_inst[0] && lv_obj_is_valid(s_fw_card_btn_inst[0])) {
                            lv_obj_set_style_bg_color(s_fw_card_btn_inst[0], lv_color_hex(0x0284c7), 0);
                            lv_obj_set_style_border_color(s_fw_card_btn_inst[0], lv_color_hex(0x38bdf8), 0);
                        }
                    } else {
                        lv_obj_set_style_text_color(s_fw_card_ver_lbl[0], lv_color_hex(0xf59e0b), 0);
                        lv_label_set_text(s_fw_card_btn_inst_lbl[0], is_fa ? "دانگرید" : (is_tr ? "Eski Surum" : "Downgrade"));
                        if (s_fw_card_btn_inst[0] && lv_obj_is_valid(s_fw_card_btn_inst[0])) {
                            lv_obj_set_style_bg_color(s_fw_card_btn_inst[0], lv_color_hex(0xd97706), 0);
                            lv_obj_set_style_border_color(s_fw_card_btn_inst[0], lv_color_hex(0xf59e0b), 0);
                        }
                    }
                    lv_obj_center(s_fw_card_btn_inst_lbl[0]);
                }
            } else if (info && info->scan_error) {
                lv_label_set_text(s_fw_card_ver_lbl[i], is_fa ? "عدم دسترسی به سرور" : (is_tr ? "Sunucuya Baglanilamadi" : "Server Offline"));
                lv_obj_set_style_text_color(s_fw_card_ver_lbl[i], lv_color_hex(0xef4444), 0);
                lv_obj_set_style_text_font(s_fw_card_ver_lbl[i], ui_lang_font_16(), 0);
                lv_obj_set_style_base_dir(s_fw_card_ver_lbl[i], is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
            } else {
                lv_label_set_text(s_fw_card_ver_lbl[i], "...");
                lv_obj_set_style_text_color(s_fw_card_ver_lbl[i], lv_color_hex(0x64748b), 0);
                lv_obj_set_style_text_font(s_fw_card_ver_lbl[i], &lv_font_montserrat_16, 0);
            }
        }

        if (s_ota_pinmap_popup && lv_obj_is_valid(s_ota_pinmap_popup)) {
            fw_pinmap_update_content(s_ota_pinmap_idx);
        }
    }

    ota_install_state_t st = ota_get_install_state();

    if (s_ota_progress_dirty) {
        s_ota_progress_dirty = false;
        uint8_t pct = s_ota_dl_pct;
        uint32_t b_dl = s_ota_dl_bytes;
        uint32_t b_tot = s_ota_dl_total;
        if (s_ota_progress_bar && lv_obj_is_valid(s_ota_progress_bar)) {
            lv_bar_set_value(s_ota_progress_bar, pct, LV_ANIM_OFF);
        }
        if (s_ota_progress_circle && lv_obj_is_valid(s_ota_progress_circle)) {
            lv_arc_set_value(s_ota_progress_circle, pct);
        }
        if (s_ota_main_pct_lbl && lv_obj_is_valid(s_ota_main_pct_lbl)) {
            lv_label_set_text_fmt(s_ota_main_pct_lbl, "%%%u", (unsigned)pct);
        }

        if (s_ota_speed_last_time == 0) {
            s_ota_speed_last_time = now;
            s_ota_speed_last_bytes = b_dl;
            s_ota_speed_kbps = 0.0f;
            s_ota_eta_last_sec_tick = now;
        } else {
            uint32_t dt = now - s_ota_speed_last_time;
            if (dt >= 400) {
                uint32_t db = (b_dl >= s_ota_speed_last_bytes) ? (b_dl - s_ota_speed_last_bytes) : 0;
                if (db > 0 || dt >= 1500) {
                    float inst_kbps = ((float)db / (float)dt) * 1000.0f / 1024.0f;
                    if (inst_kbps > 1.0f) {
                        if (s_ota_speed_kbps <= 1.0f) {
                            s_ota_speed_kbps = inst_kbps;
                        } else {
                            s_ota_speed_kbps = (s_ota_speed_kbps * 0.65f) + (inst_kbps * 0.35f);
                        }
                    } else if (s_ota_speed_kbps > 5.0f) {
                        s_ota_speed_kbps = s_ota_speed_kbps * 0.90f;
                    }
                    s_ota_speed_last_time = now;
                    s_ota_speed_last_bytes = b_dl;

                    // محاسبه زمان باقیمانده پایدار
                    uint32_t b_rem = (b_tot > b_dl) ? (b_tot - b_dl) : 0;
                    if (s_ota_speed_kbps > 2.0f && b_rem > 0) {
                        float spd_bytes = s_ota_speed_kbps * 1024.0f;
                        float calc_eta = (float)b_rem / spd_bytes;
                        if (s_ota_smooth_eta_sec <= 0.5f) {
                            s_ota_smooth_eta_sec = calc_eta;
                        } else {
                            s_ota_smooth_eta_sec = (s_ota_smooth_eta_sec * 0.80f) + (calc_eta * 0.20f);
                        }
                    }
                }
            }
        }

            // ۱. درصد پیشرفت
            if (s_ota_card_rem_pct_val && lv_obj_is_valid(s_ota_card_rem_pct_val)) {
                lv_label_set_text_fmt(s_ota_card_rem_pct_val, "%u%%", (unsigned)pct);
            }

            // ۲. حجم (انجام شده / کل)
            if (b_tot == 0) {
                b_tot = ota_get_install_total();
                if (b_tot == 0 && s_ota_install_active_model >= 0 && s_ota_install_active_model < OTA_MODEL_COUNT) {
                    b_tot = ota_get_info(s_ota_install_active_model)->size_bytes;
                }
                if (b_tot == 0) {
                    b_tot = 3346864; // Fallback
                }
            }
            if (s_ota_card_size_val && lv_obj_is_valid(s_ota_card_size_val)) {
                char sz_buf[32];
                uint32_t cur_x10 = (b_dl * 10) / (1024 * 1024);
                uint32_t tot_x10 = (b_tot * 10) / (1024 * 1024);
                snprintf(sz_buf, sizeof(sz_buf), "%u.%u/%u.%u MB",
                         (unsigned)(cur_x10 / 10), (unsigned)(cur_x10 % 10),
                         (unsigned)(tot_x10 / 10), (unsigned)(tot_x10 % 10));
                lv_label_set_text(s_ota_card_size_val, sz_buf);
            }

            // ۳. سرعت لحظه‌ای و میانگین فیلترشده
            if (s_ota_card_speed_val && lv_obj_is_valid(s_ota_card_speed_val)) {
                if (st == OTA_INST_ERASING) {
                    lv_label_set_text(s_ota_card_speed_val, "-- KB/s");
                } else {
                    char spd_buf[32];
                    snprintf(spd_buf, sizeof(spd_buf), "%u KB/s", (unsigned)s_ota_speed_kbps);
                    lv_label_set_text(s_ota_card_speed_val, spd_buf);
                }
            }

            // ۴. زمان تقریبی باقیمانده با شمارش معکوس ملایم
            if (s_ota_card_eta_val && lv_obj_is_valid(s_ota_card_eta_val)) {
                if (st == OTA_INST_ERASING) {
                    uint32_t eta_erase = (pct < 100) ? (((100 - pct) * 4) / 100) : 0;
                    char eta_buf[16];
                    snprintf(eta_buf, sizeof(eta_buf), "00:%02u", (unsigned)eta_erase);
                    lv_label_set_text(s_ota_card_eta_val, eta_buf);
                } else if (s_ota_speed_kbps > 2.0f && s_ota_smooth_eta_sec > 0.5f) {
                    if (now - s_ota_eta_last_sec_tick >= 1000) {
                        if (s_ota_smooth_eta_sec > 1.0f) s_ota_smooth_eta_sec -= 1.0f;
                        s_ota_eta_last_sec_tick = now;
                    }

                    uint32_t eta_display = (uint32_t)(s_ota_smooth_eta_sec + 0.5f);
                    uint32_t m = eta_display / 60;
                    uint32_t s = eta_display % 60;
                    if (m > 99) {
                        lv_label_set_text(s_ota_card_eta_val, ">99m");
                    } else {
                        char eta_buf[16];
                        snprintf(eta_buf, sizeof(eta_buf), "%02u:%02u", (unsigned)m, (unsigned)s);
                        lv_label_set_text(s_ota_card_eta_val, eta_buf);
                    }
                } else {
                    lv_label_set_text(s_ota_card_eta_val, "--:--");
                }
            }
    }

    if (st != s_last_rendered_inst_state) {
        s_last_rendered_inst_state = st;

        if (st == OTA_INST_CONNECTING) {
            s_ota_speed_last_time = 0;
            s_ota_speed_last_bytes = 0;
            s_ota_speed_kbps = 0.0f;
            if (s_ota_progress_status && lv_obj_is_valid(s_ota_progress_status)) {
                if (is_fa) {
                    lv_label_set_text(s_ota_progress_status, "در حال برقراری ارتباط با سرور...");
                    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_RTL, 0);
                } else if (is_tr) {
                    lv_label_set_text(s_ota_progress_status, "Sunucuya baglaniliyor...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                } else {
                    lv_label_set_text(s_ota_progress_status, "Connecting to server...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                }
                lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0x38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        } else if (st == OTA_INST_ERASING) {
            s_ota_speed_last_time = 0;
            s_ota_speed_last_bytes = 0;
            s_ota_speed_kbps = 0.0f;
            if (s_ota_progress_status && lv_obj_is_valid(s_ota_progress_status)) {
                if (is_fa) {
                    lv_label_set_text(s_ota_progress_status, "در حال آماده‌سازی حافظه فلش...");
                    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_RTL, 0);
                } else if (is_tr) {
                    lv_label_set_text(s_ota_progress_status, "Flash bellegi siliniyor...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                } else {
                    lv_label_set_text(s_ota_progress_status, "Preparing flash partition...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                }
                lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0xf59e0b), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        } else if (st == OTA_INST_DOWNLOADING) {
            s_ota_speed_last_time = 0;
            s_ota_speed_last_bytes = 0;
            s_ota_speed_kbps = 0.0f;
            s_ota_smooth_eta_sec = 0.0f;
            if (s_ota_progress_status && lv_obj_is_valid(s_ota_progress_status)) {
                if (is_fa) {
                    lv_label_set_text(s_ota_progress_status, "در حال دریافت و رایت فریمور...");
                    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_RTL, 0);
                } else if (is_tr) {
                    lv_label_set_text(s_ota_progress_status, "Yazilim paketi indiriliyor...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                } else {
                    lv_label_set_text(s_ota_progress_status, "Downloading & writing firmware...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                }
                lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0x22c55e), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        } else if (st == OTA_INST_VERIFYING) {
            if (s_ota_progress_status && lv_obj_is_valid(s_ota_progress_status)) {
                if (is_fa) {
                    lv_label_set_text(s_ota_progress_status, "در حال اعتبارسنجی فریمور...");
                    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_RTL, 0);
                } else if (is_tr) {
                    lv_label_set_text(s_ota_progress_status, "Yazilim dogrulaniyor...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                } else {
                    lv_label_set_text(s_ota_progress_status, "Verifying firmware...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                }
                lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0x38bdf8), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            if (s_ota_progress_bar && lv_obj_is_valid(s_ota_progress_bar)) {
                lv_bar_set_value(s_ota_progress_bar, 100, LV_ANIM_OFF);
            }
            if (s_ota_progress_circle && lv_obj_is_valid(s_ota_progress_circle)) {
                lv_arc_set_value(s_ota_progress_circle, 100);
            }
            if (s_ota_main_pct_lbl && lv_obj_is_valid(s_ota_main_pct_lbl)) {
                lv_label_set_text(s_ota_main_pct_lbl, "%100");
            }
            if (s_ota_card_rem_pct_val && lv_obj_is_valid(s_ota_card_rem_pct_val)) {
                lv_label_set_text(s_ota_card_rem_pct_val, "0%");
            }
            if (s_ota_card_eta_val && lv_obj_is_valid(s_ota_card_eta_val)) {
                lv_label_set_text(s_ota_card_eta_val, "00:00");
            }
            if (s_ota_progress_close_btn && lv_obj_is_valid(s_ota_progress_close_btn)) {
                lv_obj_add_flag(s_ota_progress_close_btn, LV_OBJ_FLAG_HIDDEN);
            }
        } else if (st == OTA_INST_FAILED) {
            if (s_ota_progress_status && lv_obj_is_valid(s_ota_progress_status)) {
                const char *err = ota_get_install_error();
                if (is_fa) {
                    lv_label_set_text_fmt(s_ota_progress_status, "خطا: %s", err);
                    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_RTL, 0);
                } else if (is_tr) {
                    lv_label_set_text_fmt(s_ota_progress_status, "Hata: %s", err);
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                } else {
                    lv_label_set_text_fmt(s_ota_progress_status, "Error: %s", err);
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                }
                lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0xef4444), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            if (s_ota_main_pct_lbl && lv_obj_is_valid(s_ota_main_pct_lbl)) {
                lv_obj_add_flag(s_ota_main_pct_lbl, LV_OBJ_FLAG_HIDDEN);
            }
            if (s_ota_progress_circle && lv_obj_is_valid(s_ota_progress_circle)) {
                lv_obj_set_style_arc_color(s_ota_progress_circle, lv_color_hex(0xef4444), LV_PART_INDICATOR);
            }
            if (s_ota_progress_icon && lv_obj_is_valid(s_ota_progress_icon)) {
                lv_obj_clear_flag(s_ota_progress_icon, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(s_ota_progress_icon, LV_SYMBOL_CLOSE);
                lv_obj_set_style_text_color(s_ota_progress_icon, lv_color_hex(0xef4444), 0);
            }
            if (s_ota_progress_close_lbl && lv_obj_is_valid(s_ota_progress_close_lbl)) {
                lv_label_set_text(s_ota_progress_close_lbl, is_fa ? "بستن" : (is_tr ? "Kapat" : "Close"));
            }
        } else if (st == OTA_INST_SUCCESS) {
            if (s_ota_progress_status && lv_obj_is_valid(s_ota_progress_status)) {
                if (is_fa) {
                    lv_label_set_text(s_ota_progress_status, "بروزرسانی موفق! در حال راه‌اندازی مجدد...");
                    lv_obj_set_style_text_font(s_ota_progress_status, ui_lang_font_16(), 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_RTL, 0);
                } else if (is_tr) {
                    lv_label_set_text(s_ota_progress_status, "Guncelleme basarili! Yeniden baslatiliyor...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                } else {
                    lv_label_set_text(s_ota_progress_status, "Update successful! Rebooting...");
                    lv_obj_set_style_text_font(s_ota_progress_status, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_base_dir(s_ota_progress_status, LV_BASE_DIR_LTR, 0);
                }
                lv_obj_set_style_text_color(s_ota_progress_status, lv_color_hex(0x22c55e), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            if (s_ota_progress_bar && lv_obj_is_valid(s_ota_progress_bar)) {
                lv_bar_set_value(s_ota_progress_bar, 100, LV_ANIM_OFF);
            }
            if (s_ota_main_pct_lbl && lv_obj_is_valid(s_ota_main_pct_lbl)) {
                lv_obj_add_flag(s_ota_main_pct_lbl, LV_OBJ_FLAG_HIDDEN);
            }
            if (s_ota_progress_circle && lv_obj_is_valid(s_ota_progress_circle)) {
                lv_arc_set_value(s_ota_progress_circle, 100);
                lv_obj_set_style_arc_color(s_ota_progress_circle, lv_color_hex(0x22c55e), LV_PART_INDICATOR);
            }
            if (s_ota_progress_icon && lv_obj_is_valid(s_ota_progress_icon)) {
                lv_obj_clear_flag(s_ota_progress_icon, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(s_ota_progress_icon, LV_SYMBOL_OK);
                lv_obj_set_style_text_color(s_ota_progress_icon, lv_color_hex(0x22c55e), 0);
            }
            if (s_ota_card_rem_pct_val && lv_obj_is_valid(s_ota_card_rem_pct_val)) {
                lv_label_set_text(s_ota_card_rem_pct_val, "0%");
            }
            if (s_ota_card_eta_val && lv_obj_is_valid(s_ota_card_eta_val)) {
                lv_label_set_text(s_ota_card_eta_val, "00:00");
            }
            if (s_ota_card_speed_val && lv_obj_is_valid(s_ota_card_speed_val)) {
                lv_label_set_text(s_ota_card_speed_val, "-");
            }
            if (s_ota_progress_close_btn && lv_obj_is_valid(s_ota_progress_close_btn)) {
                lv_obj_add_flag(s_ota_progress_close_btn, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

/* ========================================================================= */
/*                   SCREEN_ID_BOOT_WIFI (Service Mode)                      */
/* ========================================================================= */
static lv_obj_t *s_boot_wifi_ssid_dd = NULL;
static lv_obj_t *s_boot_wifi_pass_ta = NULL;
static lv_obj_t *s_boot_wifi_eye_btn = NULL;
static lv_obj_t *s_boot_wifi_eye_lbl = NULL;
static lv_obj_t *s_boot_wifi_connect_btn = NULL;
static lv_obj_t *s_boot_wifi_connect_lbl = NULL;
static lv_obj_t *s_boot_wifi_scan_btn = NULL;
static lv_obj_t *s_boot_wifi_status_lbl = NULL;
static lv_obj_t *s_boot_wifi_ip_val_lbl = NULL;
static lv_obj_t *s_boot_wifi_hdr_pill = NULL;
static lv_obj_t *s_boot_wifi_hdr_pill_lbl = NULL;
static lv_obj_t *s_boot_wifi_ota_btn = NULL;
static lv_obj_t *s_boot_wifi_kb = NULL;

static bool s_boot_wifi_pass_masked = true;
static bool s_boot_wifi_auto_advance = false;
static uint32_t s_boot_wifi_last_status_check = 0;
static uint16_t s_boot_wifi_last_scan_count = 0;

static void boot_wifi_connect_btn_cb(lv_event_t *e);
static void boot_wifi_scan_btn_cb(lv_event_t *e);
static void boot_wifi_eye_btn_cb(lv_event_t *e);
static void boot_wifi_dd_changed_cb(lv_event_t *e);
static void boot_wifi_ta_event_cb(lv_event_t *e);
static void boot_wifi_kb_event_cb(lv_event_t *e);
static void boot_wifi_return_to_main_cb(lv_event_t *e);
static void boot_wifi_ota_btn_cb(lv_event_t *e);
static void boot_wifi_on_scan_done_cb(uint16_t count, wifi_ap_record_t *records);

static void boot_wifi_return_to_main_cb(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_MAIN_PG);
}

void boot_wifi_enable_scan_btn(void)
{
    if (s_boot_wifi_scan_btn && lv_obj_is_valid(s_boot_wifi_scan_btn)) {
        lv_obj_clear_state(s_boot_wifi_scan_btn, LV_STATE_DISABLED);
    }
}

static void boot_wifi_on_scan_done_cb(uint16_t count, wifi_ap_record_t *records)
{
    if (!objects.boot_wifi_pg || !lv_obj_is_valid(objects.boot_wifi_pg)) return;
    s_boot_wifi_last_scan_count = count;

    if (s_boot_wifi_scan_btn && lv_obj_is_valid(s_boot_wifi_scan_btn)) {
        lv_obj_clear_state(s_boot_wifi_scan_btn, LV_STATE_DISABLED);
    }

    if (s_boot_wifi_ssid_dd && lv_obj_is_valid(s_boot_wifi_ssid_dd)) {
        if (count == 0 || !records) {
            lv_dropdown_set_options(s_boot_wifi_ssid_dd, "شبکه یافت نشد");
            if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
                lv_label_set_text(s_boot_wifi_status_lbl, "شبکه‌ای یافت نشد\nمجدداً اسکن کنید");
                lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0xf87171), 0);
            }
            return;
        }

        char prev_sel[64] = {0};
        lv_dropdown_get_selected_str(s_boot_wifi_ssid_dd, prev_sel, sizeof(prev_sel));

        char opts[1536] = {0};
        size_t offset = 0;
        uint16_t item_idx = 0;
        int32_t restore_idx = -1;
        const char *primary = wifi_get_primary_saved_ssid();
        if (primary && *primary) {
            size_t plen = strlen(primary);
            if (plen < sizeof(opts) - 2) {
                memcpy(opts, primary, plen);
                offset = plen;
                if (prev_sel[0] && strcmp(prev_sel, primary) == 0) {
                    restore_idx = 0;
                }
                item_idx++;
            }
        }

        for (uint16_t i = 0; i < count && i < 25; i++) {
            const char *s = (const char *)records[i].ssid;
            if (!s || !*s) continue;
            if (primary && strcmp(s, primary) == 0) continue;
            if (offset > 0 && offset < sizeof(opts) - 1) {
                opts[offset++] = '\n';
            }
            size_t slen = strlen(s);
            if (offset + slen < sizeof(opts) - 1) {
                memcpy(&opts[offset], s, slen);
                offset += slen;
                opts[offset] = '\0';
                if (prev_sel[0] && strcmp(prev_sel, s) == 0) {
                    restore_idx = (int32_t)item_idx;
                }
                item_idx++;
            }
        }

        if (offset > 0) {
            lv_dropdown_set_options(s_boot_wifi_ssid_dd, opts);
            lv_dropdown_set_selected(s_boot_wifi_ssid_dd, (restore_idx >= 0) ? (uint32_t)restore_idx : 0);
        }

        if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%d شبکه وای‌فای یافت شد. شبکه مورد نظر را انتخاب کنید.", (int)count);
            lv_label_set_text(s_boot_wifi_status_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x22c55e), 0);
        }
    }
}

static void boot_wifi_dd_changed_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    if (!dd) return;
    char sel_ssid[64] = {0};
    lv_dropdown_get_selected_str(dd, sel_ssid, sizeof(sel_ssid));
    if (sel_ssid[0]) {
        char saved_pass[64] = {0};
        if (wifi_get_saved_password(sel_ssid, saved_pass, sizeof(saved_pass))) {
            if (s_boot_wifi_pass_ta && lv_obj_is_valid(s_boot_wifi_pass_ta)) {
                lv_textarea_set_text(s_boot_wifi_pass_ta, saved_pass);
            }
            if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
                lv_label_set_text(s_boot_wifi_status_lbl, "رمز عبور این شبکه بازیابی شد");
                lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x10b981), 0);
            }
        } else {
            if (s_boot_wifi_pass_ta && lv_obj_is_valid(s_boot_wifi_pass_ta)) {
                lv_textarea_set_text(s_boot_wifi_pass_ta, "");
            }
        }
    }
}

static void boot_wifi_eye_btn_cb(lv_event_t *e)
{
    (void)e;
    s_boot_wifi_pass_masked = !s_boot_wifi_pass_masked;
    if (s_boot_wifi_pass_ta && lv_obj_is_valid(s_boot_wifi_pass_ta)) {
        lv_textarea_set_password_mode(s_boot_wifi_pass_ta, s_boot_wifi_pass_masked);
    }
    if (s_boot_wifi_eye_lbl && lv_obj_is_valid(s_boot_wifi_eye_lbl)) {
        lv_obj_set_style_text_font(s_boot_wifi_eye_lbl, &lv_font_montserrat_16, 0);
        lv_label_set_text(s_boot_wifi_eye_lbl, s_boot_wifi_pass_masked ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
    }
}

static void boot_wifi_ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED) {
        if (s_boot_wifi_kb && lv_obj_is_valid(s_boot_wifi_kb)) {
            lv_keyboard_set_textarea(s_boot_wifi_kb, s_boot_wifi_pass_ta);
            lv_obj_clear_flag(s_boot_wifi_kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(s_boot_wifi_kb);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        if (s_boot_wifi_kb && lv_obj_is_valid(s_boot_wifi_kb)) {
            lv_obj_add_flag(s_boot_wifi_kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void boot_wifi_kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        if (s_boot_wifi_kb && lv_obj_is_valid(s_boot_wifi_kb)) {
            lv_obj_add_flag(s_boot_wifi_kb, LV_OBJ_FLAG_HIDDEN);
        }
        if (code == LV_EVENT_READY) {
            boot_wifi_connect_btn_cb(NULL);
        }
    }
}

static void boot_wifi_scr_click_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target != s_boot_wifi_pass_ta && target != s_boot_wifi_eye_btn && target != s_boot_wifi_kb) {
        if (s_boot_wifi_kb && lv_obj_is_valid(s_boot_wifi_kb) && !lv_obj_has_flag(s_boot_wifi_kb, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(s_boot_wifi_kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void boot_wifi_connect_btn_cb(lv_event_t *e)
{
    (void)e;
    char sel_ssid[64] = {0};
    if (s_boot_wifi_ssid_dd && lv_obj_is_valid(s_boot_wifi_ssid_dd)) {
        lv_dropdown_get_selected_str(s_boot_wifi_ssid_dd, sel_ssid, sizeof(sel_ssid));
    }
    const char *pass = "";
    if (s_boot_wifi_pass_ta && lv_obj_is_valid(s_boot_wifi_pass_ta)) {
        pass = lv_textarea_get_text(s_boot_wifi_pass_ta);
    }
    if (!sel_ssid[0] || strcmp(sel_ssid, "لیست شبکه‌ها...") == 0 || strcmp(sel_ssid, "جستجوی شبکه‌ها...") == 0 || strcmp(sel_ssid, "هیچ شبکه‌ای یافت نشد") == 0 || strcmp(sel_ssid, "شبکه‌ای یافت نشد") == 0) {
        if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
            lv_label_set_text(s_boot_wifi_status_lbl, "ابتدا یک شبکه معتبر را انتخاب کنید");
            lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0xf87171), 0);
        }
        return;
    }
    if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "در حال اتصال به شبکه %s", sel_ssid);
        lv_label_set_text(s_boot_wifi_status_lbl, buf);
        lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x38bdf8), 0);
    }

    s_boot_wifi_auto_advance = false;
    wifi_connect_to_ap(sel_ssid, pass ? pass : "");
}

static void boot_wifi_scan_btn_cb(lv_event_t *e)
{
    (void)e;
    s_boot_wifi_last_scan_count = 0;
    if (s_boot_wifi_scan_btn && lv_obj_is_valid(s_boot_wifi_scan_btn)) {
        lv_obj_add_state(s_boot_wifi_scan_btn, LV_STATE_DISABLED);
    }
    if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
        lv_label_set_text(s_boot_wifi_status_lbl, "در حال جستجو و اسکن شبکه‌های اطراف...");
        lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x38bdf8), 0);
    }
    wifi_start_scan();
}

static void boot_wifi_ota_btn_cb(lv_event_t *e)
{
    (void)e;
    framware_set_return_target(SCREEN_ID_BOOT_WIFI, 0);
    loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
    ota_request_scan();
}

void create_screen_boot_wifi(void)
{
    s_boot_wifi_auto_advance = false;
    wifi_register_scan_done_cb(boot_wifi_on_scan_done_cb);

    lv_obj_t *scr = lv_obj_create(0);
    objects.boot_wifi_pg = scr;
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_size(scr, 480, 320);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0b0f17), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);

    /* Header Bar (480x36) */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, 480, 36);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);

    /* Back Button on Left */
    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_pos(btn_back, 6, 3);
    lv_obj_set_size(btn_back, 76, 30);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_radius(btn_back, 6, 0);
    lv_obj_add_event_cb(btn_back, boot_wifi_return_to_main_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " بازگشت");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl_back, &ui_font_yekan_16, 0);
    lv_obj_center(lbl_back);

    /* Wi-Fi Status Pill in Center (Centered at x=240, w=226 for full breathing room) */
    s_boot_wifi_hdr_pill = lv_obj_create(hdr);
    lv_obj_set_pos(s_boot_wifi_hdr_pill, 127, 4);
    lv_obj_set_size(s_boot_wifi_hdr_pill, 226, 28);
    lv_obj_clear_flag(s_boot_wifi_hdr_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_boot_wifi_hdr_pill, lv_color_hex(0x0b0f17), 0);
    lv_obj_set_style_border_color(s_boot_wifi_hdr_pill, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(s_boot_wifi_hdr_pill, 1, 0);
    lv_obj_set_style_radius(s_boot_wifi_hdr_pill, 14, 0);
    lv_obj_set_style_pad_all(s_boot_wifi_hdr_pill, 0, 0);

    s_boot_wifi_hdr_pill_lbl = lv_label_create(s_boot_wifi_hdr_pill);
    lv_label_set_text(s_boot_wifi_hdr_pill_lbl, "وای‌فای در حال بررسی...");
    lv_obj_align(s_boot_wifi_hdr_pill_lbl, LV_ALIGN_CENTER, 0, -2);
    lv_obj_set_style_text_color(s_boot_wifi_hdr_pill_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(s_boot_wifi_hdr_pill_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(s_boot_wifi_hdr_pill_lbl, LV_BASE_DIR_RTL, 0);

    /* Title on Right */
    lv_obj_t *title_mode = lv_label_create(hdr);
    lv_label_set_text(title_mode, "سرویس وای‌فای");
    lv_obj_align(title_mode, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_text_color(title_mode, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_font(title_mode, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(title_mode, LV_BASE_DIR_RTL, 0);

    /* ===== Card 1: Left - Wi-Fi Configuration (246x274) ===== */
    lv_obj_t *c1 = lv_obj_create(scr);
    lv_obj_set_pos(c1, 6, 40);
    lv_obj_set_size(c1, 246, 274);
    lv_obj_clear_flag(c1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(c1, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_radius(c1, 10, 0);
    lv_obj_set_style_border_width(c1, 1, 0);
    lv_obj_set_style_border_color(c1, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_pad_all(c1, 0, 0);

    /* Card 1 Title - Full width centered */
    lv_obj_t *c1_title = lv_label_create(c1);
    lv_label_set_text(c1_title, "تنظیمات اتصال وای‌فای");
    lv_obj_set_pos(c1_title, 0, 10);
    lv_obj_set_size(c1_title, 246, 22);
    lv_obj_set_style_text_color(c1_title, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_font(c1_title, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(c1_title, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(c1_title, LV_TEXT_ALIGN_CENTER, 0);

    /* 1. Password Section Label - Exact width and X of password input box (x=9, w=178) -> Dead-center, y=38! */
    lv_obj_t *lbl_pass = lv_label_create(c1);
    lv_label_set_text(lbl_pass, "رمز عبور");
    lv_obj_set_pos(lbl_pass, 9, 38);
    lv_obj_set_size(lbl_pass, 178, 22);
    lv_obj_set_style_text_color(lbl_pass, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(lbl_pass, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(lbl_pass, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(lbl_pass, LV_TEXT_ALIGN_CENTER, 0);

    /* Password Input Row: Textarea (w=178) + Eye Button (w=44) -> Total 228, centered at x=9, y=66 */
    s_boot_wifi_pass_ta = lv_textarea_create(c1);
    lv_obj_set_pos(s_boot_wifi_pass_ta, 9, 66);
    lv_obj_set_size(s_boot_wifi_pass_ta, 178, 32);
    lv_textarea_set_password_mode(s_boot_wifi_pass_ta, true);
    lv_textarea_set_one_line(s_boot_wifi_pass_ta, true);
    lv_textarea_set_placeholder_text(s_boot_wifi_pass_ta, "Password...");
    lv_obj_set_style_bg_color(s_boot_wifi_pass_ta, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(s_boot_wifi_pass_ta, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(s_boot_wifi_pass_ta, 1, 0);
    lv_obj_set_style_text_color(s_boot_wifi_pass_ta, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(s_boot_wifi_pass_ta, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_hor(s_boot_wifi_pass_ta, 10, 0);
    lv_obj_set_style_pad_ver(s_boot_wifi_pass_ta, 4, 0);
    lv_obj_add_event_cb(s_boot_wifi_pass_ta, boot_wifi_ta_event_cb, LV_EVENT_ALL, NULL);

    s_boot_wifi_eye_btn = lv_btn_create(c1);
    lv_obj_set_pos(s_boot_wifi_eye_btn, 193, 66);
    lv_obj_set_size(s_boot_wifi_eye_btn, 44, 32);
    lv_obj_set_style_bg_color(s_boot_wifi_eye_btn, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_color(s_boot_wifi_eye_btn, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(s_boot_wifi_eye_btn, 1, 0);
    lv_obj_set_style_radius(s_boot_wifi_eye_btn, 8, 0);
    lv_obj_add_event_cb(s_boot_wifi_eye_btn, boot_wifi_eye_btn_cb, LV_EVENT_CLICKED, NULL);

    s_boot_wifi_eye_lbl = lv_label_create(s_boot_wifi_eye_btn);
    lv_obj_set_style_text_font(s_boot_wifi_eye_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_boot_wifi_eye_lbl, lv_color_hex(0xffffff), 0);
    lv_label_set_text(s_boot_wifi_eye_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_center(s_boot_wifi_eye_lbl);

    /* 2. SSID Section Label - Exact width and X of dropdown box (x=9, w=186) -> Dead-center, y=108! */
    lv_obj_t *lbl_ssid = lv_label_create(c1);
    lv_label_set_text(lbl_ssid, "نام شبکه (SSID)");
    lv_obj_set_pos(lbl_ssid, 9, 108);
    lv_obj_set_size(lbl_ssid, 186, 22);
    lv_obj_set_style_text_color(lbl_ssid, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(lbl_ssid, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(lbl_ssid, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(lbl_ssid, LV_TEXT_ALIGN_CENTER, 0);

    /* SSID Row: Dropdown expanded to w=186 + Scan Button compact w=36 -> Total 228, centered at x=9, y=136 */
    s_boot_wifi_ssid_dd = lv_dropdown_create(c1);
    lv_obj_set_pos(s_boot_wifi_ssid_dd, 9, 136);
    lv_obj_set_size(s_boot_wifi_ssid_dd, 186, 32);
    lv_obj_set_style_bg_color(s_boot_wifi_ssid_dd, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(s_boot_wifi_ssid_dd, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(s_boot_wifi_ssid_dd, 1, 0);
    lv_obj_set_style_text_color(s_boot_wifi_ssid_dd, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(s_boot_wifi_ssid_dd, &ui_font_yekan_16, 0);
    lv_obj_set_style_pad_hor(s_boot_wifi_ssid_dd, 10, 0);
    lv_obj_set_style_pad_ver(s_boot_wifi_ssid_dd, 4, 0);
    lv_dropdown_set_options(s_boot_wifi_ssid_dd, "انتخاب شبکه...");
    lv_obj_t *dd_list = lv_dropdown_get_list(s_boot_wifi_ssid_dd);
    if (dd_list) lv_obj_set_style_text_font(dd_list, &ui_font_yekan_16, 0);
    lv_obj_add_event_cb(s_boot_wifi_ssid_dd, boot_wifi_dd_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_boot_wifi_scan_btn = lv_btn_create(c1);
    lv_obj_set_pos(s_boot_wifi_scan_btn, 201, 136);
    lv_obj_set_size(s_boot_wifi_scan_btn, 36, 32);
    lv_obj_set_style_bg_color(s_boot_wifi_scan_btn, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(s_boot_wifi_scan_btn, lv_color_hex(0x1e293b), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(s_boot_wifi_scan_btn, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(s_boot_wifi_scan_btn, 1, 0);
    lv_obj_set_style_radius(s_boot_wifi_scan_btn, 8, 0);
    lv_obj_add_event_cb(s_boot_wifi_scan_btn, boot_wifi_scan_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_scan = lv_label_create(s_boot_wifi_scan_btn);
    lv_label_set_text(lbl_scan, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(lbl_scan, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_scan, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_scan);

    /* 3. Connect Button - Centered at x=9, w=228, h=36, y=176 */
    s_boot_wifi_connect_btn = lv_btn_create(c1);
    lv_obj_set_pos(s_boot_wifi_connect_btn, 9, 176);
    lv_obj_set_size(s_boot_wifi_connect_btn, 228, 36);
    lv_obj_set_style_bg_color(s_boot_wifi_connect_btn, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_bg_color(s_boot_wifi_connect_btn, lv_color_hex(0x0369a1), LV_STATE_PRESSED);
    lv_obj_set_style_pad_ver(s_boot_wifi_connect_btn, 0, 0);
    lv_obj_set_style_radius(s_boot_wifi_connect_btn, 8, 0);
    lv_obj_add_event_cb(s_boot_wifi_connect_btn, boot_wifi_connect_btn_cb, LV_EVENT_CLICKED, NULL);

    s_boot_wifi_connect_lbl = lv_label_create(s_boot_wifi_connect_btn);
    lv_label_set_text(s_boot_wifi_connect_lbl, LV_SYMBOL_WIFI " اتصال به شبکه");
    lv_obj_set_style_text_color(s_boot_wifi_connect_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(s_boot_wifi_connect_lbl, &ui_font_yekan_16, 0);
    lv_obj_center(s_boot_wifi_connect_lbl);

    /* 4. Connection Status Subtext - Tall 52px box to completely prevent bottom descender clipping, y=216 */
    s_boot_wifi_status_lbl = lv_label_create(c1);
    lv_label_set_text(s_boot_wifi_status_lbl, "آماده اتصال به شبکه...");
    lv_obj_set_pos(s_boot_wifi_status_lbl, 8, 216);
    lv_obj_set_size(s_boot_wifi_status_lbl, 230, 52);
    lv_label_set_long_mode(s_boot_wifi_status_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_boot_wifi_status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(s_boot_wifi_status_lbl, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(s_boot_wifi_status_lbl, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_line_space(s_boot_wifi_status_lbl, 0, 0);
    lv_obj_clear_flag(s_boot_wifi_status_lbl, LV_OBJ_FLAG_SCROLLABLE);

    /* ===== Card 2: Right - OTA Gateway & System (216x274) ===== */
    lv_obj_t *c2 = lv_obj_create(scr);
    lv_obj_set_pos(c2, 258, 40);
    lv_obj_set_size(c2, 216, 274);
    lv_obj_clear_flag(c2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(c2, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_radius(c2, 10, 0);
    lv_obj_set_style_border_width(c2, 1, 0);
    lv_obj_set_style_border_color(c2, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_pad_all(c2, 0, 0);

    /* Card 2 Title - Full width centered */
    lv_obj_t *c2_title = lv_label_create(c2);
    lv_label_set_text(c2_title, "درگاه سیستم و بروزرسانی");
    lv_obj_set_pos(c2_title, 0, 10);
    lv_obj_set_size(c2_title, 216, 22);
    lv_obj_set_style_text_color(c2_title, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_text_font(c2_title, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(c2_title, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(c2_title, LV_TEXT_ALIGN_CENTER, 0);

    /* 1. Unified Version & IP Box - Uniform 11px gap, h=54, y=43 */
    lv_obj_t *box_sys = lv_obj_create(c2);
    lv_obj_set_pos(box_sys, 8, 43);
    lv_obj_set_size(box_sys, 200, 54);
    lv_obj_clear_flag(box_sys, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(box_sys, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(box_sys, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(box_sys, 1, 0);
    lv_obj_set_style_radius(box_sys, 8, 0);
    lv_obj_set_style_pad_all(box_sys, 0, 0);

    /* Full 22px label heights to eliminate any descender clipping */
    lv_obj_t *lbl_ver = lv_label_create(box_sys);
    lv_label_set_text(lbl_ver, "نسخه فریمور: " FIRMWARE_VERSION_STR);
    lv_obj_set_pos(lbl_ver, 0, 4);
    lv_obj_set_size(lbl_ver, 200, 22);
    lv_obj_set_style_text_color(lbl_ver, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_font(lbl_ver, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(lbl_ver, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(lbl_ver, LV_TEXT_ALIGN_CENTER, 0);

    s_boot_wifi_ip_val_lbl = lv_label_create(box_sys);
    lv_label_set_text(s_boot_wifi_ip_val_lbl, "IP: Disconnected");
    lv_obj_set_pos(s_boot_wifi_ip_val_lbl, 0, 28);
    lv_obj_set_size(s_boot_wifi_ip_val_lbl, 200, 22);
    lv_obj_set_style_text_color(s_boot_wifi_ip_val_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(s_boot_wifi_ip_val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_base_dir(s_boot_wifi_ip_val_lbl, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(s_boot_wifi_ip_val_lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* 2. Big OTA Gateway Button - Uniform 11px gap from box_sys (y=108, h=38) */
    s_boot_wifi_ota_btn = lv_btn_create(c2);
    lv_obj_set_pos(s_boot_wifi_ota_btn, 8, 108);
    lv_obj_set_size(s_boot_wifi_ota_btn, 200, 38);
    lv_obj_set_style_bg_color(s_boot_wifi_ota_btn, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_bg_color(s_boot_wifi_ota_btn, lv_color_hex(0x059669), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(s_boot_wifi_ota_btn, lv_color_hex(0x34d399), 0);
    lv_obj_set_style_border_width(s_boot_wifi_ota_btn, 1, 0);
    lv_obj_set_style_pad_ver(s_boot_wifi_ota_btn, 0, 0);
    lv_obj_set_style_radius(s_boot_wifi_ota_btn, 8, 0);
    lv_obj_add_event_cb(s_boot_wifi_ota_btn, boot_wifi_ota_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_ota = lv_label_create(s_boot_wifi_ota_btn);
    lv_label_set_text(lbl_ota, LV_SYMBOL_DOWNLOAD " بروزرسانی ابری OTA");
    lv_obj_center(lbl_ota);
    lv_obj_set_style_text_color(lbl_ota, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl_ota, &ui_font_yekan_16, 0);

    /* 3. SoftAP Direct Connection Info (y=150, h=64) */
    lv_obj_t *ap_box = lv_obj_create(c2);
    lv_obj_set_pos(ap_box, 8, 150);
    lv_obj_set_size(ap_box, 200, 64);
    lv_obj_clear_flag(ap_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ap_box, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(ap_box, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_width(ap_box, 1, 0);
    lv_obj_set_style_radius(ap_box, 8, 0);
    lv_obj_set_style_pad_all(ap_box, 0, 0);

    lv_obj_t *lbl_ap_t = lv_label_create(ap_box);
    lv_label_set_text(lbl_ap_t, "اتصال مستقیم وای‌فای ماشین");
    lv_obj_set_pos(lbl_ap_t, 0, 3);
    lv_obj_set_size(lbl_ap_t, 200, 18);
    lv_obj_set_style_text_color(lbl_ap_t, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(lbl_ap_t, &ui_font_yekan_16, 0);
    lv_obj_set_style_base_dir(lbl_ap_t, LV_BASE_DIR_RTL, 0);
    lv_obj_set_style_text_align(lbl_ap_t, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *lbl_ap_ssid = lv_label_create(ap_box);
    lv_label_set_text(lbl_ap_ssid, ASTIAJ_SOFTAP_SSID);
    lv_obj_set_pos(lbl_ap_ssid, 0, 23);
    lv_obj_set_size(lbl_ap_ssid, 200, 18);
    lv_obj_set_style_text_color(lbl_ap_ssid, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_font(lbl_ap_ssid, &lv_font_montserrat_14, 0);
    lv_obj_set_style_base_dir(lbl_ap_ssid, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(lbl_ap_ssid, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *lbl_ap_ip = lv_label_create(ap_box);
    lv_label_set_text(lbl_ap_ip, "Pass: 12345678 | 192.168.4.1");
    lv_obj_set_pos(lbl_ap_ip, 0, 43);
    lv_obj_set_size(lbl_ap_ip, 200, 18);
    lv_obj_set_style_text_color(lbl_ap_ip, lv_color_hex(0x6ee7b7), 0);
    lv_obj_set_style_text_font(lbl_ap_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_base_dir(lbl_ap_ip, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(lbl_ap_ip, LV_TEXT_ALIGN_CENTER, 0);

    /* 4. Return to Machine Program Button - Uniform 11px gap from ap_box (y=222, h=42) */
    lv_obj_t *btn_home = lv_btn_create(c2);
    lv_obj_set_pos(btn_home, 8, 222);
    lv_obj_set_size(btn_home, 200, 42);
    lv_obj_set_style_bg_color(btn_home, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_color(btn_home, lv_color_hex(0x0f172a), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_home, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_border_width(btn_home, 1, 0);
    lv_obj_set_style_pad_ver(btn_home, 0, 0);
    lv_obj_set_style_radius(btn_home, 8, 0);
    lv_obj_add_event_cb(btn_home, boot_wifi_return_to_main_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_home = lv_label_create(btn_home);
    lv_label_set_text(lbl_home, LV_SYMBOL_HOME " صفحه اصلی ماشین");
    lv_obj_center(lbl_home);
    lv_obj_set_style_text_color(lbl_home, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl_home, &ui_font_yekan_16, 0);

    /* Virtual Keyboard - TALLER (height 164px for comfortable typing!) */
    s_boot_wifi_kb = lv_keyboard_create(scr);
    lv_obj_set_size(s_boot_wifi_kb, 476, 164);
    lv_obj_align(s_boot_wifi_kb, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_keyboard_set_map(s_boot_wifi_kb, LV_KEYBOARD_MODE_TEXT_LOWER, s_wifi_kb_map_lc, s_wifi_kb_ctrl_lc);
    lv_keyboard_set_map(s_boot_wifi_kb, LV_KEYBOARD_MODE_TEXT_UPPER, s_wifi_kb_map_uc, s_wifi_kb_ctrl_uc);
    lv_keyboard_set_map(s_boot_wifi_kb, LV_KEYBOARD_MODE_SPECIAL, s_wifi_kb_map_sym, s_wifi_kb_ctrl_sym);
    lv_keyboard_set_mode(s_boot_wifi_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_boot_wifi_kb, s_boot_wifi_pass_ta);
    lv_obj_add_flag(s_boot_wifi_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_boot_wifi_kb, boot_wifi_kb_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(s_boot_wifi_kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(s_boot_wifi_kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    lv_obj_add_event_cb(scr, boot_wifi_scr_click_cb, LV_EVENT_CLICKED, NULL);

    /* بارگذاری بلادرنگ وضعیت اتصال یا وای‌فای ذخیره‌شده بدون تاخیر تیک */
    if (wifi_is_connected()) {
        const char *ssid = wifi_get_connected_ssid();
        const char *ip_str = wifi_get_ip_str();
        if (s_boot_wifi_hdr_pill_lbl && lv_obj_is_valid(s_boot_wifi_hdr_pill_lbl)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "متصل به %s", (ssid && *ssid) ? ssid : "شبکه");
            lv_label_set_text(s_boot_wifi_hdr_pill_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_hdr_pill_lbl, lv_color_hex(0x22c55e), 0);
        }
        if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "دستگاه به شبکه %s متصل است.", (ssid && *ssid) ? ssid : "");
            lv_label_set_text(s_boot_wifi_status_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x22c55e), 0);
        }
        if (s_boot_wifi_ip_val_lbl && lv_obj_is_valid(s_boot_wifi_ip_val_lbl)) {
            char buf[64];
            snprintf(buf, sizeof(buf), "IP: %s", (ip_str && *ip_str) ? ip_str : "Disconnected");
            lv_label_set_text(s_boot_wifi_ip_val_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_ip_val_lbl, lv_color_hex(0x38bdf8), 0);
        }
        if (s_boot_wifi_ssid_dd && lv_obj_is_valid(s_boot_wifi_ssid_dd) && ssid && *ssid) {
            lv_dropdown_set_options(s_boot_wifi_ssid_dd, ssid);
            lv_dropdown_set_selected(s_boot_wifi_ssid_dd, 0);
        }
    } else if (wifi_has_saved_credentials()) {
        const char *saved_ssid = wifi_get_primary_saved_ssid();
        char saved_pass[64] = {0};
        if (saved_ssid && *saved_ssid) {
            wifi_get_saved_password(saved_ssid, saved_pass, sizeof(saved_pass));
            if (s_boot_wifi_ssid_dd && lv_obj_is_valid(s_boot_wifi_ssid_dd)) {
                lv_dropdown_set_options(s_boot_wifi_ssid_dd, saved_ssid);
                lv_dropdown_set_selected(s_boot_wifi_ssid_dd, 0);
            }
            if (s_boot_wifi_pass_ta && lv_obj_is_valid(s_boot_wifi_pass_ta)) {
                lv_textarea_set_text(s_boot_wifi_pass_ta, saved_pass);
            }
        }
    }

    /* راه‌اندازی خودکار اسکن در پس‌زمینه برای یافتن شبکه‌های مجاور */
    wifi_start_scan();
}

void delete_screen_boot_wifi(void)
{
    s_boot_wifi_auto_advance = false;
    wifi_unregister_scan_done_cb(boot_wifi_on_scan_done_cb);

    s_boot_wifi_ssid_dd = NULL;
    s_boot_wifi_pass_ta = NULL;
    s_boot_wifi_eye_btn = NULL;
    s_boot_wifi_eye_lbl = NULL;
    s_boot_wifi_connect_btn = NULL;
    s_boot_wifi_connect_lbl = NULL;
    s_boot_wifi_scan_btn = NULL;
    s_boot_wifi_status_lbl = NULL;
    s_boot_wifi_ip_val_lbl = NULL;
    s_boot_wifi_hdr_pill = NULL;
    s_boot_wifi_hdr_pill_lbl = NULL;
    s_boot_wifi_ota_btn = NULL;
    s_boot_wifi_kb = NULL;
    s_boot_wifi_last_scan_count = 0;

    if (objects.boot_wifi_pg && lv_obj_is_valid(objects.boot_wifi_pg)) {
        lv_obj_del(objects.boot_wifi_pg);
        objects.boot_wifi_pg = NULL;
    }
}

void boot_wifi_on_pin_passed(void)
{
    if (wifi_is_connected()) {
        s_boot_wifi_auto_advance = false;
        framware_set_return_target(SCREEN_ID_BOOT_WIFI, 0);
        loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
        ota_request_scan();
        return;
    }

    if (wifi_has_saved_credentials()) {
        const char *saved_ssid = wifi_get_primary_saved_ssid();
        char saved_pass[64] = {0};
        if (saved_ssid && *saved_ssid) {
            wifi_get_saved_password(saved_ssid, saved_pass, sizeof(saved_pass));
        }

        if (s_boot_wifi_ssid_dd && lv_obj_is_valid(s_boot_wifi_ssid_dd) && saved_ssid && *saved_ssid) {
            lv_dropdown_set_options(s_boot_wifi_ssid_dd, saved_ssid);
            lv_dropdown_set_selected(s_boot_wifi_ssid_dd, 0);
        }
        if (s_boot_wifi_pass_ta && lv_obj_is_valid(s_boot_wifi_pass_ta)) {
            lv_textarea_set_text(s_boot_wifi_pass_ta, saved_pass);
        }
        if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "در حال اتصال به شبکه %s", saved_ssid ? saved_ssid : "وای‌فای");
            lv_label_set_text(s_boot_wifi_status_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0x38bdf8), 0);
        }

        s_boot_wifi_auto_advance = false;
        if (saved_ssid && *saved_ssid) {
            wifi_connect_to_ap(saved_ssid, saved_pass);
        }
    } else {
        s_boot_wifi_auto_advance = false;
        if (s_boot_wifi_status_lbl && lv_obj_is_valid(s_boot_wifi_status_lbl)) {
            lv_label_set_text(s_boot_wifi_status_lbl, "وای‌فای ذخیره نشده؛ در حال جستجو...");
            lv_obj_set_style_text_color(s_boot_wifi_status_lbl, lv_color_hex(0xf59e0b), 0);
        }
        wifi_start_scan();
    }
}

void tick_screen_boot_wifi(void)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    if (now - s_boot_wifi_last_status_check < 400) {
        return;
    }
    s_boot_wifi_last_status_check = now;

    bool connected = wifi_is_connected();
    if (connected) {
        const char *ip_str = wifi_get_ip_str();
        const char *ssid = wifi_get_connected_ssid();

        if (s_boot_wifi_hdr_pill_lbl && lv_obj_is_valid(s_boot_wifi_hdr_pill_lbl)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "متصل به %s", (ssid && *ssid) ? ssid : "شبکه");
            lv_label_set_text(s_boot_wifi_hdr_pill_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_hdr_pill_lbl, lv_color_hex(0x22c55e), 0);
        }
        if (s_boot_wifi_ip_val_lbl && lv_obj_is_valid(s_boot_wifi_ip_val_lbl)) {
            char buf[64];
            snprintf(buf, sizeof(buf), "IP: %s", (ip_str && *ip_str) ? ip_str : "192.168.x.x");
            lv_label_set_text(s_boot_wifi_ip_val_lbl, buf);
            lv_obj_set_style_text_color(s_boot_wifi_ip_val_lbl, lv_color_hex(0x38bdf8), 0);
        }
    } else {
        if (s_boot_wifi_hdr_pill_lbl && lv_obj_is_valid(s_boot_wifi_hdr_pill_lbl)) {
            lv_label_set_text(s_boot_wifi_hdr_pill_lbl, "وای‌فای قطع");
            lv_obj_set_style_text_color(s_boot_wifi_hdr_pill_lbl, lv_color_hex(0x94a3b8), 0);
        }
        if (s_boot_wifi_ip_val_lbl && lv_obj_is_valid(s_boot_wifi_ip_val_lbl)) {
            lv_label_set_text(s_boot_wifi_ip_val_lbl, "IP: Disconnected");
            lv_obj_set_style_text_color(s_boot_wifi_ip_val_lbl, lv_color_hex(0x94a3b8), 0);
        }
    }

    wifi_ap_record_t *records = NULL;
    uint16_t count = wifi_get_scan_records(&records);
    if (count > 0 && count != s_boot_wifi_last_scan_count && records) {
        s_boot_wifi_last_scan_count = count;
        if (s_boot_wifi_ssid_dd && lv_obj_is_valid(s_boot_wifi_ssid_dd)) {
            char opts[1536] = {0};
            size_t offset = 0;
            const char *primary = wifi_get_primary_saved_ssid();
            if (primary && *primary) {
                size_t plen = strlen(primary);
                if (plen < sizeof(opts) - 2) {
                    memcpy(opts, primary, plen);
                    offset = plen;
                }
            }
            for (uint16_t i = 0; i < count && i < 25; i++) {
                const char *s = (const char *)records[i].ssid;
                if (!s || !*s) continue;
                if (primary && strcmp(s, primary) == 0) continue;
                if (offset > 0 && offset < sizeof(opts) - 1) {
                    opts[offset++] = '\n';
                }
                size_t slen = strlen(s);
                if (offset + slen < sizeof(opts) - 1) {
                    memcpy(&opts[offset], s, slen);
                    offset += slen;
                    opts[offset] = '\0';
                }
            }
            if (offset > 0) {
                lv_dropdown_set_options(s_boot_wifi_ssid_dd, opts);
            }
        }
    }
}

/* =========================================================================
 * SUB-SYSTEM: WARRANTY ACTIVATION SCREEN & DIALOGS (480x320 APPLE HIG)
 * ========================================================================= */

static lv_obj_t *s_warr_kb = NULL;
static lv_obj_t *s_warr_active_ta = NULL;
static lv_obj_t *s_warr_expired_backdrop = NULL;
static lv_obj_t *s_warr_no_net_backdrop = NULL;


/* =========================================================================
 * SUB-SYSTEM: APPLE HIG WARRANTY & AFTER-SALES CERTIFICATE DIALOG (480x320)
 * ========================================================================= */

static void get_warranty_dates_for_lang(char *start_out, size_t start_sz,
                                        char *warr_end_out, size_t warr_sz,
                                        char *serv_end_out, size_t serv_sz)
{
    const warranty_info_t *wi = warranty_get_info();
    ui_lang_t lang = ui_lang_get();
    bool is_fa = (lang == UI_LANG_FA);

    int y = 0, m = 0, d = 0;
    int matched = 0;
    if (wi->date_str[0]) {
        matched = sscanf(wi->date_str, "%d/%d/%d", &y, &m, &d);
        if (matched != 3) {
            matched = sscanf(wi->date_str, "%d-%d-%d", &y, &m, &d);
        }
    }

    if (matched == 3 && y > 0) {
        if (is_fa) {
            /* تاریخ شمسی برای زبان فارسی */
            int jy = y, jm = m, jd = d;
            if (y >= 2000) {
                gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
            }
            snprintf(start_out, start_sz, "%04d/%02d/%02d", jy, jm, jd);
            snprintf(warr_end_out, warr_sz, "%04d/%02d/%02d", jy + 1, jm, jd);
            snprintf(serv_end_out, serv_sz, "%04d/%02d/%02d", jy + 10, jm, jd);
        } else {
            /* تاریخ میلادی برای انگلیسی و ترکی */
            int gy = y, gm = m, gd = d;
            if (y < 2000) {
                jalali_to_gregorian(y, m, d, &gy, &gm, &gd);
            }
            snprintf(start_out, start_sz, "%04d/%02d/%02d", gy, gm, gd);
            snprintf(warr_end_out, warr_sz, "%04d/%02d/%02d", gy + 1, gm, gd);
            snprintf(serv_end_out, serv_sz, "%04d/%02d/%02d", gy + 10, gm, gd);
        }
    } else {
        /* در صورت عدم ثبت، محاسبه پویا بر اساس تاریخ امروز RTC دستگاه */
        if (is_fa) {
            jalali_time_t jt;
            if (ds3231_get_time_jalali(&jt) == ESP_OK && jt.jy >= 1400) {
                snprintf(start_out, start_sz, "%04d/%02d/%02d", jt.jy, jt.jm, jt.jd);
                snprintf(warr_end_out, warr_sz, "%04d/%02d/%02d", jt.jy + 1, jt.jm, jt.jd);
                snprintf(serv_end_out, serv_sz, "%04d/%02d/%02d", jt.jy + 10, jt.jm, jt.jd);
            } else {
                snprintf(start_out, start_sz, "در انتظار ثبت");
                snprintf(warr_end_out, warr_sz, "۱ سال پس از ثبت");
                snprintf(serv_end_out, serv_sz, "۱۰ سال پس از ثبت");
            }
        } else if (lang == UI_LANG_TR) {
            ds3231_time_t dt;
            if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24) {
                snprintf(start_out, start_sz, "20%02d/%02d/%02d", dt.year, dt.month, dt.date);
                snprintf(warr_end_out, warr_sz, "20%02d/%02d/%02d", dt.year + 1, dt.month, dt.date);
                snprintf(serv_end_out, serv_sz, "20%02d/%02d/%02d", dt.year + 10, dt.month, dt.date);
            } else {
                snprintf(start_out, start_sz, "Kayit Bekleniyor");
                snprintf(warr_end_out, warr_sz, "Kayittan 1 Yil Sonra");
                snprintf(serv_end_out, serv_sz, "Kayittan 10 Yil Sonra");
            }
        } else {
            ds3231_time_t dt;
            if (ds3231_get_time(&dt) == ESP_OK && dt.year >= 24) {
                snprintf(start_out, start_sz, "20%02d/%02d/%02d", dt.year, dt.month, dt.date);
                snprintf(warr_end_out, warr_sz, "20%02d/%02d/%02d", dt.year + 1, dt.month, dt.date);
                snprintf(serv_end_out, serv_sz, "20%02d/%02d/%02d", dt.year + 10, dt.month, dt.date);
            } else {
                snprintf(start_out, start_sz, "Awaiting Registration");
                snprintf(warr_end_out, warr_sz, "1 Year After Reg");
                snprintf(serv_end_out, serv_sz, "10 Years After Reg");
            }
        }
    }
}

void hide_warranty_details_dialog(void)
{
    if (s_warr_details_backdrop && lv_obj_is_valid(s_warr_details_backdrop)) {
        lv_obj_del(s_warr_details_backdrop);
        s_warr_details_backdrop = NULL;
    }
}

static void close_warranty_details_dialog_cb(lv_event_t *e)
{
    (void)e;
    hide_warranty_details_dialog();
}

static void action_open_warranty_registration_from_details(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    if (warranty_is_active()) {
        s_pin_for_warranty_edit = true;
        show_manager_pin_dialog();
    } else {
        hide_warranty_details_dialog();
        s_warr_opened_from_main = false;
        open_warranty_screen();
    }
}

void open_warranty_details_dialog(void)
{
    hide_warranty_details_dialog();

    const warranty_info_t *wi = warranty_get_info();
    ui_lang_t lang = ui_lang_get();
    bool is_fa = (lang == UI_LANG_FA);
    bool is_tr = (lang == UI_LANG_TR);
    bool is_active = warranty_is_active();

    char start_date[48] = {0};
    char warr_end[48] = {0};
    char serv_end[48] = {0};
    get_warranty_dates_for_lang(start_date, sizeof(start_date),
                                warr_end, sizeof(warr_end),
                                serv_end, sizeof(serv_end));

    /* لایه تیره پس‌زمینه اپل HIG (Backdrop) */
    s_warr_details_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_warr_details_backdrop, 0, 0);
    lv_obj_set_size(s_warr_details_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_warr_details_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_warr_details_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_warr_details_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_warr_details_backdrop, 0, 0);
    lv_obj_clear_flag(s_warr_details_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    /* کادر اصلی پنجره مدال با گوشه‌های گرد و کادر ظریف */
    lv_obj_t *card = lv_obj_create(s_warr_details_backdrop);
    lv_obj_set_pos(card, 13, 13);
    lv_obj_set_size(card, 454, 294);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* هدر مشکی مات شیک */
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 454, 38);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_obj_set_style_text_font(title, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    if (is_fa) {
        lv_label_set_text(title, "شناسنامه گارانتی و خدمات پس از فروش آستیاج");
    } else if (is_tr) {
        lv_label_set_text(title, "ASTIAJ Altin Garanti ve Servis Belgesi");
    } else {
        lv_label_set_text(title, "ASTIAJ Golden Warranty & Service Certificate");
    }
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* دکمه خروج Header */
    lv_obj_t *btn_x = lv_btn_create(header);
    lv_obj_set_size(btn_x, 32, 28);
    lv_obj_set_pos(btn_x, is_fa ? 6 : 416, 5);
    lv_obj_set_style_bg_color(btn_x, lv_color_hex(0x334155), 0);
    lv_obj_set_style_radius(btn_x, 6, 0);
    lv_obj_set_style_shadow_width(btn_x, 0, 0);
    lv_obj_add_event_cb(btn_x, close_warranty_details_dialog_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_x = lv_label_create(btn_x);
    lv_label_set_text(lbl_x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_x);

    /* چینش متقارن ستون‌ها بر اساس جهت نوشتاری RTL/LTR */
    int col_cust_x = is_fa ? 232 : 10;
    int col_warr_x = is_fa ? 10 : 232;

    /* --- کارت ۱: مشخصات مالک و دستگاه --- */
    lv_obj_t *card_cust = lv_obj_create(card);
    lv_obj_set_pos(card_cust, col_cust_x, 44);
    lv_obj_set_size(card_cust, 212, 198);
    lv_obj_set_style_bg_color(card_cust, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_radius(card_cust, 8, 0);
    lv_obj_set_style_border_color(card_cust, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_border_width(card_cust, 1, 0);
    lv_obj_set_style_pad_all(card_cust, 6, 0);
    lv_obj_clear_flag(card_cust, LV_OBJ_FLAG_SCROLLABLE);

    /* نشانگر سربرگ کارت مشتری */
    lv_obj_t *badge_cust = lv_obj_create(card_cust);
    lv_obj_set_pos(badge_cust, 0, 0);
    lv_obj_set_size(badge_cust, 198, 24);
    lv_obj_set_style_bg_color(badge_cust, lv_color_hex(0xe0f2fe), 0);
    lv_obj_set_style_radius(badge_cust, 5, 0);
    lv_obj_set_style_border_width(badge_cust, 0, 0);
    lv_obj_set_style_pad_all(badge_cust, 0, 0);
    lv_obj_clear_flag(badge_cust, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_b_c = lv_label_create(badge_cust);
    lv_obj_set_style_text_font(lbl_b_c, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_b_c, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_base_dir(lbl_b_c, ui_lang_base_dir(), 0);
    lv_label_set_text(lbl_b_c, is_fa ? "مشخصات مالک و دستگاه" : (is_tr ? "Musteri ve Cihaz Bilgisi" : "Customer & Machine Info"));
    lv_obj_center(lbl_b_c);

    /* نام خریدار (فونت یکان برای جلوگیری از نمایش مربعی اسامی فارسی در تمام زبان‌ها) */
    lv_obj_t *l_name = lv_label_create(card_cust);
    lv_obj_set_pos(l_name, 2, 28);
    lv_obj_set_size(l_name, 196, 22);
    lv_obj_set_style_text_font(l_name, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(l_name, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_base_dir(l_name, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(l_name, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    if (is_fa) {
        lv_label_set_text_fmt(l_name, "نام خریدار: %s", wi->buyer_name[0] ? wi->buyer_name : "ثبت‌نشده");
    } else if (is_tr) {
        lv_label_set_text_fmt(l_name, "Alici: %s", wi->buyer_name[0] ? wi->buyer_name : "Belirtilmedi");
    } else {
        lv_label_set_text_fmt(l_name, "Buyer: %s", wi->buyer_name[0] ? wi->buyer_name : "Not Registered");
    }

    /* شهر / محل نصب (فونت یکان جهت صفر درصد مربع) */
    lv_obj_t *l_city = lv_label_create(card_cust);
    lv_obj_set_pos(l_city, 2, 52);
    lv_obj_set_size(l_city, 196, 22);
    lv_obj_set_style_text_font(l_city, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_color(l_city, lv_color_hex(0x334155), 0);
    lv_obj_set_style_base_dir(l_city, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(l_city, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    if (is_fa) {
        lv_label_set_text_fmt(l_city, "شهر / استان: %s", wi->city[0] ? wi->city : "---");
    } else if (is_tr) {
        lv_label_set_text_fmt(l_city, "Sehir: %s", wi->city[0] ? wi->city : "---");
    } else {
        lv_label_set_text_fmt(l_city, "City: %s", wi->city[0] ? wi->city : "---");
    }

    /* شماره همراه */
    lv_obj_t *l_phone = lv_label_create(card_cust);
    lv_obj_set_pos(l_phone, 2, 76);
    lv_obj_set_size(l_phone, 196, 22);
    lv_obj_set_style_text_font(l_phone, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l_phone, lv_color_hex(0x334155), 0);
    lv_obj_set_style_base_dir(l_phone, is_fa ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_text_align(l_phone, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    if (is_fa) {
        lv_label_set_text_fmt(l_phone, "شماره همراه: \xE2\x80\xAD%s\xE2\x80\xAC", wi->phone[0] ? wi->phone : "---");
    } else if (is_tr) {
        lv_label_set_text_fmt(l_phone, "Telefon: %s", wi->phone[0] ? wi->phone : "---");
    } else {
        lv_label_set_text_fmt(l_phone, "Phone: %s", wi->phone[0] ? wi->phone : "---");
    }

    /* باکس اختصاصی کد مشتری آستیاج */
    lv_obj_t *box_code = lv_obj_create(card_cust);
    lv_obj_set_pos(box_code, 2, 102);
    lv_obj_set_size(box_code, 196, 48);
    lv_obj_set_style_bg_color(box_code, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_radius(box_code, 6, 0);
    lv_obj_set_style_border_color(box_code, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(box_code, 1, 0);
    lv_obj_set_style_pad_all(box_code, 2, 0);
    lv_obj_clear_flag(box_code, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_code_title = lv_label_create(box_code);
    lv_obj_set_style_text_font(lbl_code_title, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code_title, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(lbl_code_title, ui_lang_base_dir(), 0);
    lv_label_set_text(lbl_code_title, is_fa ? "کد اختصاصی مشتری:" : (is_tr ? "Musteri Kodu:" : "Customer Code:"));
    lv_obj_align(lbl_code_title, LV_ALIGN_TOP_MID, 0, 1);

    char cust_code[20] = {0};
    if (wi->customer_code[0]) {
        snprintf(cust_code, sizeof(cust_code), "%s", wi->customer_code);
    } else {
        warranty_get_customer_code(cust_code, sizeof(cust_code));
    }
    lv_obj_t *lbl_code_val = lv_label_create(box_code);
    lv_obj_set_style_text_font(lbl_code_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code_val, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_base_dir(lbl_code_val, LV_BASE_DIR_LTR, 0);
    lv_label_set_text(lbl_code_val, cust_code);
    lv_obj_align(lbl_code_val, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* مدل ماشین */
    lv_obj_t *l_model = lv_label_create(card_cust);
    lv_obj_set_pos(l_model, 2, 156);
    lv_obj_set_size(l_model, 196, 22);
    lv_obj_set_style_text_font(l_model, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l_model, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(l_model, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(l_model, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(l_model, is_fa ? "مدل دستگاه: \xE2\x80\xAD" "CM-700\xE2\x80\xAC" : "Model: CM-700");

    /* --- کارت ۲: وضعیت گارانتی، اتمام و پشتیبانی آستیاج --- */
    lv_obj_t *card_warr = lv_obj_create(card);
    lv_obj_set_pos(card_warr, col_warr_x, 44);
    lv_obj_set_size(card_warr, 212, 198);
    lv_obj_set_style_bg_color(card_warr, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_radius(card_warr, 8, 0);
    lv_obj_set_style_border_color(card_warr, is_active ? lv_color_hex(0x86efac) : lv_color_hex(0xfde68a), 0);
    lv_obj_set_style_border_width(card_warr, 1, 0);
    lv_obj_set_style_pad_all(card_warr, 6, 0);
    lv_obj_clear_flag(card_warr, LV_OBJ_FLAG_SCROLLABLE);

    /* وضعیت گارانتی (Badge) */
    lv_obj_t *badge_warr = lv_obj_create(card_warr);
    lv_obj_set_pos(badge_warr, 0, 0);
    lv_obj_set_size(badge_warr, 198, 24);
    lv_obj_set_style_bg_color(badge_warr, is_active ? lv_color_hex(0xdcfce7) : lv_color_hex(0xfef3c7), 0);
    lv_obj_set_style_radius(badge_warr, 5, 0);
    lv_obj_set_style_border_width(badge_warr, 0, 0);
    lv_obj_set_style_pad_all(badge_warr, 0, 0);
    lv_obj_clear_flag(badge_warr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_b_w = lv_label_create(badge_warr);
    lv_obj_set_style_text_font(lbl_b_w, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_b_w, is_active ? lv_color_hex(0x166534) : lv_color_hex(0x92400e), 0);
    lv_obj_set_style_base_dir(lbl_b_w, ui_lang_base_dir(), 0);
    if (is_active) {
        lv_label_set_text(lbl_b_w, is_fa ? "گارانتی طلایی آستیاج (فعال)" : (is_tr ? "Altin Garanti (Aktif)" : "Golden Warranty (Active)"));
    } else {
        lv_label_set_text(lbl_b_w, is_fa ? "در انتظار ثبت و فعال‌سازی" : (is_tr ? "Kayit Bekleniyor" : "Awaiting Activation"));
    }
    lv_obj_center(lbl_b_w);

    const lv_font_t *date_font = is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14;

    /* تاریخ شروع */
    lv_obj_t *l_sdate = lv_label_create(card_warr);
    lv_obj_set_pos(l_sdate, 2, 28);
    lv_obj_set_size(l_sdate, 196, 22);
    lv_obj_set_style_text_font(l_sdate, date_font, 0);
    lv_obj_set_style_text_color(l_sdate, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_base_dir(l_sdate, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(l_sdate, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    if (is_fa) {
        lv_label_set_text_fmt(l_sdate, "تاریخ شروع: \xE2\x80\xAD%s\xE2\x80\xAC", start_date);
    } else if (is_tr) {
        lv_label_set_text_fmt(l_sdate, "Baslangic: %s", start_date);
    } else {
        lv_label_set_text_fmt(l_sdate, "Start: %s", start_date);
    }

    /* تاریخ اتمام گارانتی (۱ سال طلایی) */
    lv_obj_t *l_wdate = lv_label_create(card_warr);
    lv_obj_set_pos(l_wdate, 2, 52);
    lv_obj_set_size(l_wdate, 198, 22);
    lv_label_set_long_mode(l_wdate, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(l_wdate, date_font, 0);
    lv_obj_set_style_text_color(l_wdate, lv_color_hex(0x166534), 0);
    lv_obj_set_style_base_dir(l_wdate, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(l_wdate, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    if (is_fa) {
        lv_label_set_text_fmt(l_wdate, "اتمام گارانتی: \xE2\x80\xAD%s\xE2\x80\xAC", warr_end);
    } else if (is_tr) {
        lv_label_set_text_fmt(l_wdate, "Garanti (1 Yil): %s", warr_end);
    } else {
        lv_label_set_text_fmt(l_wdate, "Warranty (1Y): %s", warr_end);
    }

    /* پایان خدمات پس از فروش و تامین قطعات (۱۰ سال) */
    lv_obj_t *l_sends = lv_label_create(card_warr);
    lv_obj_set_pos(l_sends, 2, 76);
    lv_obj_set_size(l_sends, 198, 22);
    lv_label_set_long_mode(l_sends, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(l_sends, date_font, 0);
    lv_obj_set_style_text_color(l_sends, lv_color_hex(0x0369a1), 0);
    lv_obj_set_style_base_dir(l_sends, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(l_sends, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    if (is_fa) {
        lv_label_set_text_fmt(l_sends, "خدمات \xE2\x80\xAD۱۰\xE2\x80\xAC ساله: \xE2\x80\xAD%s\xE2\x80\xAC", serv_end);
    } else if (is_tr) {
        lv_label_set_text_fmt(l_sends, "Destek (10 Yil): %s", serv_end);
    } else {
        lv_label_set_text_fmt(l_sends, "Support (10Y): %s", serv_end);
    }

    /* باکس سبز اختصاصی پشتیبانی و خدمات کارخانه (09102367062) */
    lv_obj_t *box_phone = lv_obj_create(card_warr);
    lv_obj_set_pos(box_phone, 2, 102);
    lv_obj_set_size(box_phone, 196, 48);
    lv_obj_set_style_bg_color(box_phone, lv_color_hex(0xf0fdf4), 0);
    lv_obj_set_style_radius(box_phone, 6, 0);
    lv_obj_set_style_border_color(box_phone, lv_color_hex(0x86efac), 0);
    lv_obj_set_style_border_width(box_phone, 1, 0);
    lv_obj_set_style_pad_all(box_phone, 2, 0);
    lv_obj_clear_flag(box_phone, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_ph_title = lv_label_create(box_phone);
    lv_obj_set_style_text_font(lbl_ph_title, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_ph_title, lv_color_hex(0x166534), 0);
    lv_obj_set_style_base_dir(lbl_ph_title, ui_lang_base_dir(), 0);
    lv_label_set_text(lbl_ph_title, is_fa ? "پشتیبانی و خدمات کارخانه:" : (is_tr ? "Fabrika Destek Hatti:" : "Factory Support Line:"));
    lv_obj_align(lbl_ph_title, LV_ALIGN_TOP_MID, 0, 1);

    lv_obj_t *lbl_ph_val = lv_label_create(box_phone);
    lv_obj_set_style_text_font(lbl_ph_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_ph_val, lv_color_hex(0x15803d), 0);
    lv_obj_set_style_base_dir(lbl_ph_val, LV_BASE_DIR_LTR, 0);
    lv_label_set_text(lbl_ph_val, "09102367062");
    lv_obj_align(lbl_ph_val, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* وب‌سایت رسمی آستیاج */
    lv_obj_t *l_web = lv_label_create(card_warr);
    lv_obj_set_pos(l_web, 2, 156);
    lv_obj_set_size(l_web, 196, 22);
    lv_obj_set_style_text_font(l_web, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l_web, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(l_web, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(l_web, is_fa ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(l_web, is_fa ? "وبسایت رسمی: astiaj.ir" : "Web: astiaj.ir");

    /* --- دکمه‌های اقدام پایین کارت --- */
    lv_obj_t *btn_reg = lv_btn_create(card);
    lv_obj_set_size(btn_reg, 212, 36);
    lv_obj_set_pos(btn_reg, is_fa ? 232 : 10, 248);
    lv_obj_set_style_radius(btn_reg, 8, 0);
    lv_obj_set_style_bg_color(btn_reg, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(btn_reg, 0, 0);
    lv_obj_add_event_cb(btn_reg, action_open_warranty_registration_from_details, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_reg = lv_label_create(btn_reg);
    lv_obj_set_style_text_font(lbl_reg, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_reg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(lbl_reg, ui_lang_base_dir(), 0);
    if (is_active) {
        lv_label_set_text(lbl_reg, is_fa ? "ویرایش اطلاعات" : (is_tr ? "Bilgileri Duzenle" : "Edit Information"));
    } else {
        lv_label_set_text(lbl_reg, is_fa ? "ثبت اطلاعات و فعال‌سازی" : (is_tr ? "Kayit ve Etkinlestir" : "Register & Activate"));
    }
    lv_obj_center(lbl_reg);

    lv_obj_t *btn_close = lv_btn_create(card);
    lv_obj_set_size(btn_close, 140, 36);
    lv_obj_set_pos(btn_close, is_fa ? 45 : 304, 248);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x475569), 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, close_warranty_details_dialog_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_obj_set_style_text_font(lbl_close, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_close, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_base_dir(lbl_close, ui_lang_base_dir(), 0);
    lv_label_set_text(lbl_close, is_fa ? "بستن پنجره" : (is_tr ? "Kapat" : "Close"));
    lv_obj_center(lbl_close);
}

static void action_open_warranty_details(lv_event_t *e)
{
    (void)e;
    open_warranty_details_dialog();
}

void ui_screens_update_warranty_f6(void)
{
    if (!objects.btn_warr_activate || !lv_obj_is_valid(objects.btn_warr_activate)) return;
    if (!s_warr_status_box || !lv_obj_is_valid(s_warr_status_box)) return;

    if (warranty_is_active()) {
        lv_obj_add_flag(objects.btn_warr_activate, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_warr_status_box, LV_OBJ_FLAG_HIDDEN);

        const warranty_info_t *wi = warranty_get_info();
        if (objects.lbl_warr_status_date && lv_obj_is_valid(objects.lbl_warr_status_date)) {
            ui_lang_t lang = ui_lang_get();
            bool is_fa = (lang == UI_LANG_FA);
            char disp_date[24];
            warranty_get_display_date(disp_date, sizeof(disp_date), is_fa);

            /* نمایش تاریخ کاملاً چپ به راست (LTR) بدون وارونگی سال و روز */
            lv_obj_set_style_base_dir(objects.lbl_warr_status_date, LV_BASE_DIR_LTR, 0);
            lv_obj_set_style_text_font(objects.lbl_warr_status_date, is_fa ? &ui_font_yekan_16 : &lv_font_montserrat_14, 0);

            if (is_fa) {
                lv_label_set_text_fmt(objects.lbl_warr_status_date, "ثبت: %s", disp_date);
            } else if (lang == UI_LANG_TR) {
                lv_label_set_text_fmt(objects.lbl_warr_status_date, "Kayit: %s", disp_date);
            } else {
                lv_label_set_text_fmt(objects.lbl_warr_status_date, "Reg: %s", disp_date);
            }
            lv_obj_align(objects.lbl_warr_status_date, LV_ALIGN_TOP_MID, 0, 6);
        }
        if (objects.lbl_warr_status_code && lv_obj_is_valid(objects.lbl_warr_status_code)) {
            lv_obj_set_style_base_dir(objects.lbl_warr_status_code, LV_BASE_DIR_LTR, 0);
            lv_label_set_text_fmt(objects.lbl_warr_status_code, "%s",
                                 wi->customer_code[0] ? wi->customer_code : "---");
            lv_obj_align(objects.lbl_warr_status_code, LV_ALIGN_BOTTOM_MID, 0, -6);
        }
    } else {
        lv_obj_clear_flag(objects.btn_warr_activate, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_warr_status_box, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Keyboard maps for Persian, English, and Numeric */
static const char * const kb_map_fa[] = {
    "ض", "ص", "ث", "ق", "ف", "غ", "ع", "ه", "خ", "ح", "ج", "چ", "\n",
    "ش", "س", "ی", "ب", "ل", "ا", "ت", "ن", "م", "ک", "گ", "پ", "\n",
    "ظ", "ط", "ز", "ر", "ذ", "د", "و", "ژ", "آ", LV_SYMBOL_BACKSPACE, "\n",
    "[EN]", "[123]", "فاصله", "[بعدی]", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t kb_ctrl_fa[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 2, 4, 2, 2
};

static const char * const kb_map_en[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
    "[فا]", "[123]", "Space", "[Next]", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t kb_ctrl_en[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2,
    2, 2, 4, 2, 2
};

static const char * const kb_map_num[] = {
    "1", "2", "3", "4", "5", "\n",
    "6", "7", "8", "9", "0", "\n",
    "-", "+", ".", "/", LV_SYMBOL_BACKSPACE, "\n",
    "[فا]", "[EN]", "Space", "[Next]", LV_SYMBOL_OK, ""
};
static const lv_buttonmatrix_ctrl_t kb_ctrl_num[] = {
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 2,
    2, 2, 2, 2, 2
};

static void action_warr_submit(void);

static void warranty_kb_event_cb(lv_event_t *e)
{
    lv_obj_t *kb = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
        if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;
        const char *txt = lv_buttonmatrix_get_button_text(kb, btn_id);
        if (!txt) return;

        buzzer_beep_click();

        if (strcmp(txt, "[EN]") == 0) {
            lv_buttonmatrix_set_map(kb, kb_map_en);
            lv_buttonmatrix_set_ctrl_map(kb, kb_ctrl_en);
            return;
        } else if (strcmp(txt, "[فا]") == 0) {
            lv_buttonmatrix_set_map(kb, kb_map_fa);
            lv_buttonmatrix_set_ctrl_map(kb, kb_ctrl_fa);
            return;
        } else if (strcmp(txt, "[123]") == 0) {
            lv_buttonmatrix_set_map(kb, kb_map_num);
            lv_buttonmatrix_set_ctrl_map(kb, kb_ctrl_num);
            return;
        }

        if (!s_warr_active_ta || !lv_obj_is_valid(s_warr_active_ta)) return;

        if (strcmp(txt, "فاصله") == 0 || strcmp(txt, "Space") == 0) {
            lv_textarea_add_char(s_warr_active_ta, ' ');
        } else if (strcmp(txt, "[بعدی]") == 0 || strcmp(txt, "[Next]") == 0) {
            if (s_warr_active_ta == s_ta_warr_name) {
                s_warr_active_ta = s_ta_warr_city;
                if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) {
                    lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex(0xcbd5e1), 0);
                    lv_obj_set_style_border_width(s_ta_warr_name, 1, 0);
                }
                if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
                    lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0x0284c7), 0);
                    lv_obj_set_style_border_width(s_ta_warr_city, 2, 0);
                }
            } else if (s_warr_active_ta == s_ta_warr_city) {
                s_warr_active_ta = s_ta_warr_phone;
                if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
                    lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0xcbd5e1), 0);
                    lv_obj_set_style_border_width(s_ta_warr_city, 1, 0);
                }
                if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) {
                    lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex(0x0284c7), 0);
                    lv_obj_set_style_border_width(s_ta_warr_phone, 2, 0);
                }
                lv_buttonmatrix_set_map(kb, kb_map_num);
                lv_buttonmatrix_set_ctrl_map(kb, kb_ctrl_num);
            } else {
                action_warr_submit();
            }
        } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            lv_textarea_delete_char(s_warr_active_ta);
        } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
            action_warr_submit();
        } else {
            /* Normal character (letters, numbers, symbols) */
            lv_textarea_add_text(s_warr_active_ta, txt);
        }
    }
}

static void warranty_ta_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        s_warr_active_ta = ta;

        /* Highlight active textarea with Apple Blue border */
        if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) {
            lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex((ta == s_ta_warr_name) ? 0x0284c7 : 0xcbd5e1), 0);
            lv_obj_set_style_border_width(s_ta_warr_name, (ta == s_ta_warr_name) ? 2 : 1, 0);
        }
        if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
            lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex((ta == s_ta_warr_city) ? 0x0284c7 : 0xcbd5e1), 0);
            lv_obj_set_style_border_width(s_ta_warr_city, (ta == s_ta_warr_city) ? 2 : 1, 0);
        }
        if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) {
            lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex((ta == s_ta_warr_phone) ? 0x0284c7 : 0xcbd5e1), 0);
            lv_obj_set_style_border_width(s_ta_warr_phone, (ta == s_ta_warr_phone) ? 2 : 1, 0);
        }

        if (s_warr_kb && lv_obj_is_valid(s_warr_kb)) {
            if (ta == s_ta_warr_phone) {
                lv_buttonmatrix_set_map(s_warr_kb, kb_map_num);
                lv_buttonmatrix_set_ctrl_map(s_warr_kb, kb_ctrl_num);
            } else {
                if (ui_lang_get() == UI_LANG_FA) {
                    lv_buttonmatrix_set_map(s_warr_kb, kb_map_fa);
                    lv_buttonmatrix_set_ctrl_map(s_warr_kb, kb_ctrl_fa);
                } else {
                    lv_buttonmatrix_set_map(s_warr_kb, kb_map_en);
                    lv_buttonmatrix_set_ctrl_map(s_warr_kb, kb_ctrl_en);
                }
            }
        }
    }
}

static void action_warr_back_clicked(lv_event_t *e)
{
    (void)e;
    if (s_warr_opened_from_main) {
        s_warr_opened_from_main = false;
        loadScreen(SCREEN_ID_MAIN_PG);
    } else {
        loadScreenWithTab(SCREEN_ID_SETTINGS_PG, 5);
    }
}

static void action_warr_help_clicked(lv_event_t *e)
{
    (void)e;
    buzzer_beep_click();
    ui_show_smart_guide(GUIDE_ID_WARRANTY);
}

static void action_warr_submit(void)
{
    if (!s_ta_warr_name || !s_ta_warr_city || !s_ta_warr_phone) return;

    const char *name = lv_textarea_get_text(s_ta_warr_name);
    const char *city = lv_textarea_get_text(s_ta_warr_city);
    const char *phone = lv_textarea_get_text(s_ta_warr_phone);

    if (!name || strlen(name) < 2 || !city || strlen(city) < 2 || !phone || strlen(phone) < 10) {
        panel_nav_toast_error(ui_lang_str(STR_WARR_INVALID_INPUT));
        buzzer_beep_error();
        return;
    }

    char clean_phone[32] = {0};
    warranty_sanitize_phone(phone, clean_phone, sizeof(clean_phone));
    if (strlen(clean_phone) != 11 || clean_phone[0] != '0' || clean_phone[1] != '9') {
        ui_lang_t lang = ui_lang_get();
        panel_nav_toast_error((lang == UI_LANG_FA) ? "شماره همراه باید ۱۱ رقم و با ۰۹ شروع شود" :
                              ((lang == UI_LANG_TR) ? "Telefon numarasi 09 ile baslamalidir" :
                               "Phone number must be 11 digits starting with 09"));
        buzzer_beep_error();
        return;
    }

    strncpy(s_warr_temp_name, name, sizeof(s_warr_temp_name) - 1);
    strncpy(s_warr_temp_city, city, sizeof(s_warr_temp_city) - 1);
    strncpy(s_warr_temp_phone, clean_phone, sizeof(s_warr_temp_phone) - 1);

    if (wifi_is_connected()) {
        warranty_activate(s_warr_temp_name, s_warr_temp_city, s_warr_temp_phone, false);
        buzzer_beep_confirm();
        panel_nav_toast_success(ui_lang_str(STR_WARR_ACTIVATED_ONLINE));
        if (s_warr_opened_from_main) {
            s_warr_opened_from_main = false;
            loadScreen(SCREEN_ID_MAIN_PG);
        } else {
            loadScreenWithTab(SCREEN_ID_SETTINGS_PG, 5);
        }
    } else {
        open_warranty_no_net_dialog();
    }
}

static void action_warr_submit_btn_cb(lv_event_t *e)
{
    (void)e;
    action_warr_submit();
}

lv_obj_t *ui_get_warranty_kb(void)
{
    return s_warr_kb;
}

void open_warranty_screen(void)
{
    const warranty_info_t *wi = warranty_get_info();
    if (wi && wi->active) {
        if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) lv_textarea_set_text(s_ta_warr_name, wi->buyer_name);
        if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) lv_textarea_set_text(s_ta_warr_city, wi->city);
        if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) lv_textarea_set_text(s_ta_warr_phone, wi->phone);
    }
    s_warr_active_ta = s_ta_warr_name;
    if (s_ta_warr_name && lv_obj_is_valid(s_ta_warr_name)) {
        lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_border_width(s_ta_warr_name, 2, 0);
    }
    if (s_ta_warr_city && lv_obj_is_valid(s_ta_warr_city)) {
        lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0xcbd5e1), 0);
        lv_obj_set_style_border_width(s_ta_warr_city, 1, 0);
    }
    if (s_ta_warr_phone && lv_obj_is_valid(s_ta_warr_phone)) {
        lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex(0xcbd5e1), 0);
        lv_obj_set_style_border_width(s_ta_warr_phone, 1, 0);
    }
    if (s_warr_kb && lv_obj_is_valid(s_warr_kb)) {
        if (ui_lang_get() == UI_LANG_FA) {
            lv_buttonmatrix_set_map(s_warr_kb, kb_map_fa);
            lv_buttonmatrix_set_ctrl_map(s_warr_kb, kb_ctrl_fa);
        } else {
            lv_buttonmatrix_set_map(s_warr_kb, kb_map_en);
            lv_buttonmatrix_set_ctrl_map(s_warr_kb, kb_ctrl_en);
        }
    }
    loadScreen(SCREEN_ID_WARRANTY);
}

void create_screen_warranty(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    objects.screen_warranty = scr;
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_size(scr, 480, 320);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf1f5f9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Header Bar (Apple Light Clean y=0, h=34) */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, 480, 34);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(hdr, 255, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    /* Back Button */
    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_pos(btn_back, 6, 2);
    lv_obj_set_size(btn_back, 30, 30);
    lv_obj_set_ext_click_area(btn_back, 6);
    lv_obj_set_style_radius(btn_back, 6, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_shadow_width(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, action_warr_back_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_label_create(btn_back);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_icon, lv_color_hex(0x0f172a), 0);
    lv_obj_center(back_icon);

    /* Title */
    lv_obj_t *title_lbl = lv_label_create(hdr);
    s_lbl_warr_title = title_lbl;
    lv_label_set_text(title_lbl, ui_lang_str(STR_WARR_TITLE));
    lv_obj_set_style_text_font(title_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_base_dir(title_lbl, ui_lang_base_dir(), 0);
    lv_obj_center(title_lbl);

    /* Help Button */
    lv_obj_t *btn_help = lv_btn_create(hdr);
    lv_obj_set_pos(btn_help, 444, 2);
    lv_obj_set_size(btn_help, 30, 30);
    lv_obj_set_ext_click_area(btn_help, 6);
    lv_obj_set_style_radius(btn_help, 6, 0);
    lv_obj_set_style_bg_color(btn_help, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_color(btn_help, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(btn_help, 1, 0);
    lv_obj_set_style_shadow_width(btn_help, 0, 0);
    lv_obj_add_event_cb(btn_help, action_warr_help_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *help_icon = lv_label_create(btn_help);
    lv_label_set_text(help_icon, LV_SYMBOL_FILE);
    lv_obj_set_style_text_color(help_icon, lv_color_hex(0x0284c7), 0);
    lv_obj_center(help_icon);

    /* 2. Form Container (White Card y=36, h=126, w=468, x=6) */
    lv_obj_t *form = lv_obj_create(scr);
    lv_obj_set_pos(form, 6, 36);
    lv_obj_set_size(form, 468, 126);
    lv_obj_set_style_bg_color(form, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(form, 255, 0);
    lv_obj_set_style_border_color(form, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(form, 1, 0);
    lv_obj_set_style_radius(form, 8, 0);
    lv_obj_set_style_pad_all(form, 0, 0);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_SCROLLABLE);

    const char *ph_name = (ui_lang_get() == UI_LANG_FA) ? "" : ui_lang_str(STR_WARR_PH_NAME);
    const char *ph_city = (ui_lang_get() == UI_LANG_FA) ? "" : ui_lang_str(STR_WARR_PH_CITY);
    const char *ph_phone = (ui_lang_get() == UI_LANG_FA) ? "" : ui_lang_str(STR_WARR_PH_PHONE);

    /* Column Right (RTL): Name & Phone (x=242, w=218) */
    lv_obj_t *lbl_name = lv_label_create(form);
    s_lbl_warr_name = lbl_name;
    lv_obj_set_pos(lbl_name, 242, 2);
    lv_obj_set_size(lbl_name, 218, 22);
    lv_label_set_text(lbl_name, ui_lang_str(STR_WARR_BUYER_NAME));
    lv_obj_set_style_text_font(lbl_name, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(lbl_name, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(lbl_name, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);

    s_ta_warr_name = lv_textarea_create(form);
    lv_obj_set_pos(s_ta_warr_name, 242, 24);
    lv_obj_set_size(s_ta_warr_name, 218, 33);
    lv_textarea_set_one_line(s_ta_warr_name, true);
    lv_textarea_set_max_length(s_ta_warr_name, 48);
    lv_textarea_set_placeholder_text(s_ta_warr_name, ph_name);
    lv_obj_set_style_text_font(s_ta_warr_name, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_font(s_ta_warr_name, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(s_ta_warr_name, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_text_color(s_ta_warr_name, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(s_ta_warr_name, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_border_width(s_ta_warr_name, 2, 0);
    lv_obj_set_style_radius(s_ta_warr_name, 6, 0);
    lv_obj_set_style_pad_top(s_ta_warr_name, 2, 0);
    lv_obj_set_style_pad_bottom(s_ta_warr_name, 2, 0);
    lv_obj_set_style_pad_left(s_ta_warr_name, 6, 0);
    lv_obj_set_style_pad_right(s_ta_warr_name, 6, 0);
    lv_obj_set_style_base_dir(s_ta_warr_name, ui_lang_base_dir(), 0);
    lv_obj_add_event_cb(s_ta_warr_name, warranty_ta_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl_phone = lv_label_create(form);
    s_lbl_warr_phone = lbl_phone;
    lv_obj_set_pos(lbl_phone, 242, 59);
    lv_obj_set_size(lbl_phone, 218, 22);
    lv_label_set_text(lbl_phone, ui_lang_str(STR_WARR_PHONE));
    lv_obj_set_style_text_font(lbl_phone, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_phone, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(lbl_phone, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(lbl_phone, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);

    s_ta_warr_phone = lv_textarea_create(form);
    lv_obj_set_pos(s_ta_warr_phone, 242, 81);
    lv_obj_set_size(s_ta_warr_phone, 218, 38);
    lv_textarea_set_one_line(s_ta_warr_phone, true);
    lv_textarea_set_max_length(s_ta_warr_phone, 14);
    lv_textarea_set_placeholder_text(s_ta_warr_phone, ph_phone);
    lv_obj_set_style_text_font(s_ta_warr_phone, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_font(s_ta_warr_phone, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(s_ta_warr_phone, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_text_color(s_ta_warr_phone, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(s_ta_warr_phone, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(s_ta_warr_phone, 1, 0);
    lv_obj_set_style_radius(s_ta_warr_phone, 6, 0);
    lv_obj_set_style_pad_top(s_ta_warr_phone, 4, 0);
    lv_obj_set_style_pad_bottom(s_ta_warr_phone, 4, 0);
    lv_obj_set_style_pad_left(s_ta_warr_phone, 6, 0);
    lv_obj_set_style_pad_right(s_ta_warr_phone, 6, 0);
    lv_obj_set_style_base_dir(s_ta_warr_phone, LV_BASE_DIR_LTR, 0);
    lv_obj_add_event_cb(s_ta_warr_phone, warranty_ta_event_cb, LV_EVENT_ALL, NULL);

    /* Column Left (RTL): City & Submit Button with Single-Line Status (x=8, w=218) */
    lv_obj_t *lbl_city = lv_label_create(form);
    s_lbl_warr_city = lbl_city;
    lv_obj_set_pos(lbl_city, 8, 2);
    lv_obj_set_size(lbl_city, 218, 22);
    lv_label_set_text(lbl_city, ui_lang_str(STR_WARR_CITY));
    lv_obj_set_style_text_font(lbl_city, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_city, lv_color_hex(0x475569), 0);
    lv_obj_set_style_base_dir(lbl_city, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(lbl_city, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);

    s_ta_warr_city = lv_textarea_create(form);
    lv_obj_set_pos(s_ta_warr_city, 8, 24);
    lv_obj_set_size(s_ta_warr_city, 218, 33);
    lv_textarea_set_one_line(s_ta_warr_city, true);
    lv_textarea_set_max_length(s_ta_warr_city, 24);
    lv_textarea_set_placeholder_text(s_ta_warr_city, ph_city);
    lv_obj_set_style_text_font(s_ta_warr_city, &ui_font_yekan_16, 0);
    lv_obj_set_style_text_font(s_ta_warr_city, &ui_font_yekan_16, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(s_ta_warr_city, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_text_color(s_ta_warr_city, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(s_ta_warr_city, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(s_ta_warr_city, 1, 0);
    lv_obj_set_style_radius(s_ta_warr_city, 6, 0);
    lv_obj_set_style_pad_top(s_ta_warr_city, 2, 0);
    lv_obj_set_style_pad_bottom(s_ta_warr_city, 2, 0);
    lv_obj_set_style_pad_left(s_ta_warr_city, 6, 0);
    lv_obj_set_style_pad_right(s_ta_warr_city, 6, 0);
    lv_obj_set_style_base_dir(s_ta_warr_city, ui_lang_base_dir(), 0);
    lv_obj_add_event_cb(s_ta_warr_city, warranty_ta_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl_status = lv_label_create(form);
    s_lbl_warr_status = lbl_status;
    lv_obj_set_pos(lbl_status, 8, 59);
    lv_obj_set_size(lbl_status, 218, 22);
    lv_label_set_text(lbl_status, (ui_lang_get() == UI_LANG_FA) ? "وضعیت: فعال نشده" :
                                  ((ui_lang_get() == UI_LANG_TR) ? "Durum: Etkin degil" :
                                   "Status: Inactive"));
    lv_obj_set_style_text_font(lbl_status, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_base_dir(lbl_status, ui_lang_base_dir(), 0);
    lv_obj_set_style_text_align(lbl_status, (ui_lang_get() == UI_LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(lbl_status, LV_LABEL_LONG_CLIP);

    lv_obj_t *btn_submit = lv_btn_create(form);
    lv_obj_set_pos(btn_submit, 8, 81);
    lv_obj_set_size(btn_submit, 218, 38);
    lv_obj_set_style_bg_color(btn_submit, lv_color_hex(0x059669), 0);
    lv_obj_set_style_bg_color(btn_submit, lv_color_hex(0x047857), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_submit, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_border_width(btn_submit, 1, 0);
    lv_obj_set_style_radius(btn_submit, 8, 0);
    lv_obj_set_style_shadow_width(btn_submit, 0, 0);
    lv_obj_add_event_cb(btn_submit, action_warr_submit_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *sub_lbl = lv_label_create(btn_submit);
    s_lbl_warr_submit = sub_lbl;
    lv_label_set_text(sub_lbl, ui_lang_str(STR_WARR_SUBMIT));
    lv_obj_set_style_text_font(sub_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(sub_lbl);

    /* 3. Bilingual Virtual Keypad (Buttonmatrix) docked at bottom with Apple Clean Light Style */
    lv_obj_t *kb = lv_buttonmatrix_create(scr);
    s_warr_kb = kb;
    lv_obj_set_pos(kb, 6, 166);
    lv_obj_set_size(kb, 468, 150);
    lv_obj_set_style_base_dir(kb, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xf1f5f9), 0);
    lv_obj_set_style_border_color(kb, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_style_border_width(kb, 1, 0);
    lv_obj_set_style_radius(kb, 8, 0);
    lv_obj_set_style_pad_all(kb, 3, 0);
    lv_obj_set_style_pad_gap(kb, 3, 0);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_SCROLLABLE);

    /* Apple HIG Clean White buttons */
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xffffff), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(kb, 255, LV_PART_ITEMS);
    lv_obj_set_style_text_color(kb, lv_color_hex(0x0f172a), LV_PART_ITEMS);
    lv_obj_set_style_border_color(kb, lv_color_hex(0xcbd5e1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(kb, 6, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(kb, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xe2e8f0), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(kb, lv_color_hex(0x0284c7), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb, lv_color_hex(0x0f172a), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_font(kb, &ui_font_yekan_16, LV_PART_ITEMS);

    /* Set default map (Persian or English) */
    if (ui_lang_get() == UI_LANG_FA) {
        lv_buttonmatrix_set_map(kb, kb_map_fa);
        lv_buttonmatrix_set_ctrl_map(kb, kb_ctrl_fa);
    } else {
        lv_buttonmatrix_set_map(kb, kb_map_en);
        lv_buttonmatrix_set_ctrl_map(kb, kb_ctrl_en);
    }

    lv_obj_add_event_cb(kb, warranty_kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(kb, keyboard_color_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
}

void delete_screen_warranty(void)
{
    if (objects.screen_warranty && lv_obj_is_valid(objects.screen_warranty)) {
        lv_obj_del(objects.screen_warranty);
        objects.screen_warranty = NULL;
    }
    s_lbl_warr_title = NULL;
    s_lbl_warr_name = NULL;
    s_lbl_warr_city = NULL;
    s_lbl_warr_phone = NULL;
    s_lbl_warr_submit = NULL;
    s_lbl_warr_status = NULL;
    s_ta_warr_name = NULL;
    s_ta_warr_city = NULL;
    s_ta_warr_phone = NULL;
    s_warr_kb = NULL;
    s_warr_active_ta = NULL;
}

void tick_screen_warranty(void)
{
    if (s_warr_kb && lv_obj_is_valid(s_warr_kb)) {
        if (lv_obj_has_flag(s_warr_kb, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(s_warr_kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void hide_warranty_expired_dialog(void)
{
    if (s_warr_expired_backdrop && lv_obj_is_valid(s_warr_expired_backdrop)) {
        lv_obj_del(s_warr_expired_backdrop);
        s_warr_expired_backdrop = NULL;
    }
}

static void action_warr_expired_dismiss(lv_event_t *e)
{
    (void)e;
    warranty_dismiss_expired_lock();
    hide_warranty_expired_dialog();
}

static void action_warr_expired_activate(lv_event_t *e)
{
    (void)e;
    hide_warranty_expired_dialog();
    s_warr_opened_from_main = true;
    open_warranty_screen();
}

void open_warranty_expired_dialog(void)
{
    hide_warranty_expired_dialog();

    s_warr_expired_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_warr_expired_backdrop, 0, 0);
    lv_obj_set_size(s_warr_expired_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_warr_expired_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_warr_expired_backdrop, 200, 0);
    lv_obj_set_style_border_width(s_warr_expired_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_warr_expired_backdrop, 0, 0);
    lv_obj_clear_flag(s_warr_expired_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_warr_expired_backdrop);
    lv_obj_set_pos(card, 30, 30);
    lv_obj_set_size(card, 420, 260);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, 416, 44);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xfef2f2), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 12, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, ui_lang_str(STR_WARR_EXPIRED_TITLE));
    lv_obj_set_style_text_font(title, ui_lang_font_20(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xdc2626), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, -14, 0);

    lv_obj_t *msg = lv_label_create(card);
    lv_obj_set_pos(msg, 2, 56);
    lv_obj_set_size(msg, 384, 110);
    lv_label_set_text(msg, ui_lang_str(STR_WARR_EXPIRED_MSG));
    lv_obj_set_style_text_font(msg, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x334155), 0);
    lv_obj_set_style_text_line_space(msg, 3, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(msg, ui_lang_base_dir(), 0);

    lv_obj_t *btn_act = lv_btn_create(card);
    lv_obj_set_pos(btn_act, 201, 180);
    lv_obj_set_size(btn_act, 180, 48);
    lv_obj_set_style_bg_color(btn_act, lv_color_hex(0x059669), 0);
    lv_obj_set_style_radius(btn_act, 10, 0);
    lv_obj_set_style_shadow_width(btn_act, 0, 0);
    lv_obj_add_event_cb(btn_act, action_warr_expired_activate, LV_EVENT_CLICKED, NULL);

    lv_obj_t *act_lbl = lv_label_create(btn_act);
    lv_label_set_text(act_lbl, ui_lang_str(STR_WARR_ACTIVATE_BTN));
    lv_obj_set_style_text_font(act_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(act_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(act_lbl);

    lv_obj_t *btn_dis = lv_btn_create(card);
    lv_obj_set_pos(btn_dis, 1, 180);
    lv_obj_set_size(btn_dis, 180, 48);
    lv_obj_set_style_bg_color(btn_dis, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_radius(btn_dis, 10, 0);
    lv_obj_set_style_shadow_width(btn_dis, 0, 0);
    lv_obj_add_event_cb(btn_dis, action_warr_expired_dismiss, LV_EVENT_CLICKED, NULL);

    lv_obj_t *dis_lbl = lv_label_create(btn_dis);
    lv_label_set_text(dis_lbl, ui_lang_str(STR_WARR_DISMISS_BTN));
    lv_obj_set_style_text_font(dis_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(dis_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(dis_lbl);
}

void hide_warranty_no_net_dialog(void)
{
    if (s_warr_no_net_backdrop && lv_obj_is_valid(s_warr_no_net_backdrop)) {
        lv_obj_del(s_warr_no_net_backdrop);
        s_warr_no_net_backdrop = NULL;
    }
}

static void action_warr_no_net_goto_wifi(lv_event_t *e)
{
    (void)e;
    hide_warranty_no_net_dialog();
    loadScreenWithTab(SCREEN_ID_SETTINGS_PG, 3);
}

static void action_warr_no_net_admin_pin(lv_event_t *e)
{
    (void)e;
    hide_warranty_no_net_dialog();
    s_pin_for_warranty_offline = true;
    show_manager_pin_dialog();
}

void open_warranty_no_net_dialog(void)
{
    hide_warranty_no_net_dialog();

    s_warr_no_net_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_warr_no_net_backdrop, 0, 0);
    lv_obj_set_size(s_warr_no_net_backdrop, 480, 320);
    lv_obj_set_style_bg_color(s_warr_no_net_backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_warr_no_net_backdrop, 180, 0);
    lv_obj_set_style_border_width(s_warr_no_net_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_warr_no_net_backdrop, 0, 0);
    lv_obj_clear_flag(s_warr_no_net_backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_warr_no_net_backdrop);
    lv_obj_set_pos(card, 30, 30);
    lv_obj_set_size(card, 420, 260);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xf59e0b), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, 416, 44);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0xfef3c7), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 12, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, ui_lang_str(STR_WARR_NO_NET_TITLE));
    lv_obj_set_style_text_font(title, ui_lang_font_20(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xb45309), 0);
    lv_obj_set_style_base_dir(title, ui_lang_base_dir(), 0);
    lv_obj_center(title);

    lv_obj_t *msg = lv_label_create(card);
    lv_obj_set_pos(msg, 16, 56);
    lv_obj_set_size(msg, 384, 110);
    lv_label_set_text(msg, ui_lang_str(STR_WARR_NO_NET_MSG));
    lv_obj_set_style_text_font(msg, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x334155), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_base_dir(msg, ui_lang_base_dir(), 0);

    lv_obj_t *btn_wifi = lv_btn_create(card);
    lv_obj_set_pos(btn_wifi, 220, 180);
    lv_obj_set_size(btn_wifi, 180, 48);
    lv_obj_set_style_bg_color(btn_wifi, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_radius(btn_wifi, 10, 0);
    lv_obj_set_style_shadow_width(btn_wifi, 0, 0);
    lv_obj_add_event_cb(btn_wifi, action_warr_no_net_goto_wifi, LV_EVENT_CLICKED, NULL);

    lv_obj_t *wifi_lbl = lv_label_create(btn_wifi);
    lv_label_set_text(wifi_lbl, ui_lang_str(STR_WARR_GOTO_WIFI));
    lv_obj_set_style_text_font(wifi_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(wifi_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(wifi_lbl);

    lv_obj_t *btn_pin = lv_btn_create(card);
    lv_obj_set_pos(btn_pin, 20, 180);
    lv_obj_set_size(btn_pin, 180, 48);
    lv_obj_set_style_bg_color(btn_pin, lv_color_hex(0xd97706), 0);
    lv_obj_set_style_radius(btn_pin, 10, 0);
    lv_obj_set_style_shadow_width(btn_pin, 0, 0);
    lv_obj_add_event_cb(btn_pin, action_warr_no_net_admin_pin, LV_EVENT_CLICKED, NULL);

    lv_obj_t *pin_lbl = lv_label_create(btn_pin);
    lv_label_set_text(pin_lbl, ui_lang_str(STR_WARR_ADMIN_OVERRIDE));
    lv_obj_set_style_text_font(pin_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(pin_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(pin_lbl);
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_splash_screen,
    tick_screen_main_pg,
    tick_screen_settings_pg,
    tick_screen_screen_framware,
    tick_screen_boot_wifi,
    tick_screen_warranty,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < (int)(sizeof(tick_screen_funcs) / sizeof(tick_screen_funcs[0]))) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

void create_screens() {
    const void *volatile p_dummy_font = &ui_font_yekan_24;
    (void)p_dummy_font;
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    create_screen_splash_screen();
    create_screen_main_pg();
    create_screen_settings_pg();
    create_screen_screen_framware();
    create_screen_boot_wifi();
    create_screen_warranty();

    /* ایجاد کپسول پاپ‌آپ ولوم استایل iOS Control Center */
    ui_create_volume_popup();

    /* ثبت کالبک به‌روزرسانی زنده متون و آیکون‌ها هنگام تغییر زبان */
    ui_lang_register_refresh_cb(ui_screens_refresh_language);
}

void screens_notify_touch_point(int16_t x, int16_t y)
{
    if (s_url_msgbox && lv_obj_is_valid(s_url_msgbox) && !lv_obj_has_flag(s_url_msgbox, LV_OBJ_FLAG_HIDDEN)) {
        if (x >= 0 && y >= 0) {
            lv_area_t coords;
            lv_obj_get_coords(s_url_msgbox, &coords);
            if (x < coords.x1 || x > coords.x2 || y < coords.y1 || y > coords.y2) {
                if (s_url_msgbox_ms_left > 100) {
                    s_url_msgbox_ms_left = 100;
                    s_url_msgbox_touch_held = false;
                }
            }
        }
    }
}
