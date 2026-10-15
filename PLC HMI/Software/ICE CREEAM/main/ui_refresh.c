#include "ui_refresh.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "screens.h"
#include "ui/images.h"
#include "project_config.h"
#include "styles.h"
#include "extra_modes.h"

extern int32_t icecream_counter;

static volatile bool s_apply_pending = false;

static void set_mode_btn_state(lv_obj_t *btn, bool on)
{
    if (!btn) {
        return;
    }
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    bool is_checked = lv_obj_has_state(btn, LV_STATE_CHECKED);
    if (is_checked == on) {
        return; /* قبلاً در همین حالت بوده و نیازی به تغییر و رندر مجدد ندارد */
    }
    if (on) {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    }
}

static void apply_bank(lv_obj_t *make_btn, lv_obj_t *hold_btn, lv_obj_t *paddle_btn,
                       lv_obj_t *make_st, lv_obj_t *hold_st, lv_obj_t *paddle_st,
                       bool make_on, bool hold_on, bool paddle_on)
{
    ui_style_status_label(make_st, make_on);
    ui_style_status_label(hold_st, hold_on);
    ui_style_status_label(paddle_st, paddle_on);

    set_mode_btn_state(make_btn, make_on);
    set_mode_btn_state(hold_btn, hold_on);
    set_mode_btn_state(paddle_btn, paddle_on);
}

static void ui_apply_async(void *p)
{
    (void)p;
    s_apply_pending = false;
    ui_apply_state();
}

static void ui_full_redraw_async(void *p)
{
    (void)p;
    lv_obj_invalidate(lv_scr_act());
}

void ui_request_apply(void)
{
    if (s_apply_pending) return;
    if (lvgl_lock(pdMS_TO_TICKS(30))) {
        if (!s_apply_pending) {
            s_apply_pending = true;
            lv_async_call(ui_apply_async, NULL);
        }
        lvgl_unlock();
        lvgl_port_wake();
    }
}

void ui_request_full_redraw(void)
{
    if (lvgl_lock(pdMS_TO_TICKS(30))) {
        lv_async_call(ui_full_redraw_async, NULL);
        lvgl_unlock();
        lvgl_port_wake();
    }
}


void ui_apply_state(void)
{
    if (!lvgl_lock(pdMS_TO_TICKS(40))) {
        ui_request_apply();
        return;
    }

    extern bool drain_mode_is_active(void);
    const bool ready = plc_is_ready();
    const bool ending = plc_is_ending();
    const bool dispense = plc_is_dispense();
    const bool make_ui = make_start || ending || ready || dispense;
    const bool paddle_ui = paddle_start || drain_mode_is_active();

    apply_bank(objects.compressor_btn, objects.fridge_btn, objects.icecreem_btn,
               objects.comp_status, objects.fridge_status, objects.paddle_status,
               make_ui, ice_start, paddle_ui);

    if (ready && objects.comp_status) {
        ui_style_status_ready(objects.comp_status);
    }

    /* بانک B: تزریق لبه | رینبو | شستشو کاملاً مستقل و تفکیک‌شده */
    ui_style_status_label(objects.comp_status_b, edge_is_active());
    ui_style_status_label(objects.fridge_status_b, rainbow_is_active());
    ui_style_status_label(objects.paddle_status_b, wash_is_active());

    set_mode_btn_state(objects.compressor_btn_b, edge_is_active());
    set_mode_btn_state(objects.fridge_btn_b, rainbow_is_active());
    set_mode_btn_state(objects.icecreem_btn_b, wash_is_active());

    make_start_b = edge_is_active();
    ice_start_b = rainbow_is_active();
    paddle_start_b = wash_is_active();

    static int32_t s_last_counter = -1;
    if (icecream_counter != s_last_counter) {
        s_last_counter = icecream_counter;
        if (objects.make_icecream_counter_1) {
            lv_obj_t *target_lbl = objects.make_icecream_counter_1;
            if (lv_obj_get_child_count(objects.make_icecream_counter_1) > 0) {
                target_lbl = lv_obj_get_child(objects.make_icecream_counter_1, 0);
            }
            const lv_font_t *f1 = (icecream_counter >= 1000000) ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
            lv_obj_set_style_text_font(target_lbl, f1, 0);
            lv_label_set_text_fmt(target_lbl, "%ld", (long)icecream_counter);
            lv_obj_center(target_lbl);
        }
        if (objects.make_icecream_counter_2) {
            const lv_font_t *f2 = (icecream_counter >= 1000000) ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
            lv_obj_set_style_text_font(objects.make_icecream_counter_2, f2, 0);
            lv_label_set_text_fmt(objects.make_icecream_counter_2, "%ld", (long)icecream_counter);
            lv_obj_center(objects.make_icecream_counter_2);
        }
    }

    /* BUG-FIX: total_counter_lbl guard مستقل از daily counter.
     * قبلاً total فقط داخل بلوک (icecream_counter != s_last_counter) رندر می‌شد.
     * اگر دستگاه بوت کند و NVS مقدار total را بازیابی کند در حالی که daily=0
     * (تغییر نکرده)، total_counter_lbl هرگز آپدیت نمی‌شد. */
    static int32_t s_last_total = -1;
    if (objects.total_counter_lbl && icecream_counter_total != s_last_total) {
        s_last_total = icecream_counter_total;
        const lv_font_t *ftot = (icecream_counter_total >= 1000000) ? &lv_font_montserrat_14 : &lv_font_montserrat_16;
        lv_obj_set_style_text_font(objects.total_counter_lbl, ftot, 0);
        lv_label_set_text_fmt(objects.total_counter_lbl, "%ld", (long)icecream_counter_total);
    }


    lvgl_unlock();
}
