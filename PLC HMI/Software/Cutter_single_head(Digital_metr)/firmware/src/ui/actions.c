#include "actions.h"
#include "ui.h"
#include "vars.h"
#include <Arduino.h>



static void hide_panel_no_cb(lv_timer_t * timer)
{
    lv_obj_add_flag(objects.panel_no, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(timer);   // تایمر بعد از اجرا حذف شود
}

static void hide_panel_nc_cb(lv_timer_t * timer)
{
    lv_obj_add_flag(objects.panel_nc, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(timer);   // تایمر بعد از اجرا حذف شود
}




extern void action_stopstate(lv_event_t * e){
    (void)e;
    // پنل‌های قدیمی وسط صفحه تاچ را مسدود می‌کردند — هرگز نشان نده
    if (objects.panel_no) lv_obj_add_flag(objects.panel_no, LV_OBJ_FLAG_HIDDEN);
    if (objects.panel_nc) lv_obj_add_flag(objects.panel_nc, LV_OBJ_FLAG_HIDDEN);
    // روی F1، فریم جدید f1_estop_cb مسئول جابه‌جایی است؛ استاپ EEZ را بی‌اثر کن
    if (objects.st_bt_co) lv_obj_clear_flag(objects.st_bt_co, LV_OBJ_FLAG_CLICKABLE);
}

extern void action_printertest(lv_event_t * e){
 printertest = 1;
}

extern void action_lang_farsi(lv_event_t * e) {
    // ارسال رویداد VALUE_CHANGED با param=0 (فارسی) به container انتخاب زبان
    lv_obj_send_event(objects.roller2_1, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)0);
}

extern void action_lang_english(lv_event_t * e) {
    // ارسال رویداد VALUE_CHANGED با param=1 (انگلیسی) به container انتخاب زبان
    lv_obj_send_event(objects.roller2_1, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)1);
}

extern void action_toggle_dropdown(lv_event_t * e) {
    // باز/بسته کردن پنل dropdown
    if (!objects.roller2_1) return;
    lv_obj_t *panel = lv_obj_get_child(objects.roller2_1, 1);
    if (!panel) return;
    if (lv_obj_has_flag(panel, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    }
}


