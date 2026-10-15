#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_splash_screen_img;
extern const lv_img_dsc_t img_compressor0icon;
extern const lv_img_dsc_t img_compressor1icon;
extern const lv_img_dsc_t img_setting0icon;
extern const lv_img_dsc_t img_make0icon;
extern const lv_img_dsc_t img_make1icon;
extern const lv_img_dsc_t img_paddle0icon;
extern const lv_img_dsc_t img_paddle1icon;
extern const lv_img_dsc_t img_astiaj_logo_img;
extern const lv_image_dsc_t img_astiaj_logo_white_img;
extern const lv_image_dsc_t img_screensaver_logo;
extern const lv_image_dsc_t img_icecream_left;
extern const lv_image_dsc_t img_icecream_right;
extern const lv_image_dsc_t img_bitmap_ice_cream;
extern const lv_image_dsc_t img_bitmap_config;
extern const lv_image_dsc_t img_flag_fa;
extern const lv_image_dsc_t img_flag_en;
extern const lv_image_dsc_t img_flag_tr;
extern const lv_image_dsc_t img_flag_fa_mini;
extern const lv_image_dsc_t img_flag_en_mini;
extern const lv_image_dsc_t img_flag_tr_mini;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[10];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/