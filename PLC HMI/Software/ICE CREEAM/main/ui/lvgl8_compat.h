#pragma once
/* Extra LVGL 8 → 9 shims not covered by lv_api_map_v8.h */

#include "lvgl.h"

#ifndef lv_obj_set_style_bg_img_opa
#define lv_obj_set_style_bg_img_opa lv_obj_set_style_bg_image_opa
#endif
#ifndef lv_obj_set_style_bg_img_tiled
#define lv_obj_set_style_bg_img_tiled lv_obj_set_style_bg_image_tiled
#endif

static inline lv_obj_t *lv_tabview_create_v8(lv_obj_t *parent, lv_dir_t tab_pos, int32_t tab_size)
{
    lv_obj_t *tv = lv_tabview_create(parent);
    if (tv) {
        lv_tabview_set_tab_bar_position(tv, tab_pos);
        lv_tabview_set_tab_bar_size(tv, tab_size);
    }
    return tv;
}
#undef lv_tabview_create
#define lv_tabview_create(parent, tab_pos, tab_size) lv_tabview_create_v8((parent), (tab_pos), (tab_size))

static inline lv_obj_t *lv_msgbox_create_v8(lv_obj_t *parent, const char *title, const char *txt,
                                            const char *const *btn_txts, bool add_close_btn)
{
    (void)btn_txts;
    lv_obj_t *mbox = lv_msgbox_create(parent);
    if (!mbox) {
        return NULL;
    }
    if (title && title[0]) {
        lv_msgbox_add_title(mbox, title);
    }
    if (txt && txt[0]) {
        lv_msgbox_add_text(mbox, txt);
    }
    if (add_close_btn) {
        lv_msgbox_add_close_button(mbox);
    }
    return mbox;
}
#undef lv_msgbox_create
#define lv_msgbox_create(parent, title, txt, btn_txts, add_close) \
    lv_msgbox_create_v8((parent), (title), (txt), (btn_txts), (add_close))
