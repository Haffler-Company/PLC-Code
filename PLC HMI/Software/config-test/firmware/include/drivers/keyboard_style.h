#pragma once
#include <lvgl.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void keyboard_action_keys_draw_cb(lv_event_t *e) {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    if (!draw_task) return;

    lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
    if (!base_dsc) return;

    if (base_dsc->part == LV_PART_ITEMS) {
        uint32_t btn_id = base_dsc->id1;
        const char *txt = lv_buttonmatrix_get_button_text(obj, btn_id);
        if (!txt) return;

        bool is_ok = (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "✓") == 0 || strcmp(txt, "OK") == 0 ||
                      strcmp(txt, "تایید") == 0 || strcmp(txt, "ثبت") == 0 || strcmp(txt, "Enter") == 0);
        bool is_backspace = (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0 || strcmp(txt, "DEL") == 0 || strcmp(txt, "⌫") == 0 ||
                             strcmp(txt, "پاک") == 0 || strcmp(txt, "حذف") == 0 || strcmp(txt, "Clear") == 0 || strcmp(txt, "BS") == 0);

        if (is_ok || is_backspace) {
            bool pressed = (lv_buttonmatrix_get_selected_button(obj) == btn_id && lv_obj_has_state(obj, LV_STATE_PRESSED));

            lv_draw_fill_dsc_t *fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_dsc) {
                if (is_ok) {
                    fill_dsc->color = pressed ? lv_color_hex(0xff059669) : lv_color_hex(0xff10b981); // Emerald Green
                } else {
                    fill_dsc->color = pressed ? lv_color_hex(0xffdc2626) : lv_color_hex(0xffef4444); // Crimson Red
                }
            }
            lv_draw_border_dsc_t *border_dsc = lv_draw_task_get_border_dsc(draw_task);
            if (border_dsc) {
                border_dsc->color = is_ok ? (pressed ? lv_color_hex(0xff047857) : lv_color_hex(0xff059669))
                                          : (pressed ? lv_color_hex(0xffb91c1c) : lv_color_hex(0xffdc2626));
            }
            lv_draw_label_dsc_t *label_dsc = lv_draw_task_get_label_dsc(draw_task);
            if (label_dsc) {
                label_dsc->color = lv_color_white();
            }
        }

        // نشانگر تک و انحصاری فوکوس روی کلید فعال کیپد (Apple HIG Neon Cyan #0ea5e9)
        bool is_focused = (lv_obj_has_state(obj, LV_STATE_FOCUSED) && lv_buttonmatrix_get_selected_button(obj) == btn_id);
        if (is_focused) {
            lv_draw_border_dsc_t *border_dsc = lv_draw_task_get_border_dsc(draw_task);
            if (border_dsc) {
                border_dsc->color = lv_color_hex(0x0ea5e9); // Apple Neon Cyan
                border_dsc->width = 3;
                border_dsc->opa = LV_OPA_COVER;
            }
            if (!is_ok && !is_backspace) {
                lv_draw_fill_dsc_t *fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
                if (fill_dsc) {
                    fill_dsc->color = lv_color_hex(0xe0f2fe); // Soft cyan background
                }
            }
        }
    }
}

static inline void style_keyboard_action_keys(lv_obj_t *kb) {
    if (!kb) return;
    lv_obj_add_event_cb(kb, keyboard_action_keys_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
}

#ifdef __cplusplus
}
#endif
