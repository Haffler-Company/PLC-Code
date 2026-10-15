#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_gear;
extern const lv_img_dsc_t img_plus_silver;
extern const lv_img_dsc_t img_b;
extern const lv_img_dsc_t img_bb;
extern const lv_img_dsc_t img_f;
extern const lv_img_dsc_t img_ff;
extern const lv_img_dsc_t img_keypad;
extern const lv_img_dsc_t img_ret;
extern const lv_img_dsc_t img_unmute;
extern const lv_img_dsc_t img_mute;
extern const lv_img_dsc_t img_insert_row;
extern const lv_img_dsc_t img_delete_row;
extern const lv_img_dsc_t img_edit_pen;
extern const lv_img_dsc_t img_add;
extern const lv_img_dsc_t img_remove;
extern const lv_img_dsc_t img_newf;
extern const lv_img_dsc_t img_newff;
extern const lv_img_dsc_t img_newb;
extern const lv_img_dsc_t img_newbb;
extern const lv_img_dsc_t img_emg;
extern const lv_img_dsc_t img_emg_stop;
extern const lv_img_dsc_t img_normally_open;
extern const lv_img_dsc_t img_normally_closed;
extern const lv_img_dsc_t img_printer;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[23];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/