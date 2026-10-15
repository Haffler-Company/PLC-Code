#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_setting_icon;
extern const lv_img_dsc_t img_setting_icon_pressed;
extern const lv_img_dsc_t img_home;
extern const lv_img_dsc_t img_home_pressed;
extern const lv_img_dsc_t img_lowequation;
extern const lv_img_dsc_t img_setting1;
extern const lv_img_dsc_t img_back;
extern const lv_img_dsc_t img_haffler_logo;
extern const lv_img_dsc_t img_bitmap_wifi;
extern const lv_img_dsc_t img_bitmap_close;
extern const lv_img_dsc_t img_bitmap_refresh;
extern const lv_img_dsc_t img_bitmap_gas;
extern const lv_img_dsc_t img_bitmap_config;
extern const lv_img_dsc_t img_bitmap_haffler_logo;
extern const lv_img_dsc_t img_bitmap_book;
extern const lv_img_dsc_t img_flag_ir;
extern const lv_img_dsc_t img_flag_en;
extern const lv_img_dsc_t img_flag_tr;
extern const lv_img_dsc_t img_flag_ru;
extern const lv_img_dsc_t img_bitmap_stop_mushroom;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[13];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/