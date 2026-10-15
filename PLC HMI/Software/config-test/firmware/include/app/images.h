#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_bitmap_splash;
extern const lv_img_dsc_t img_splash_a;
extern const lv_img_dsc_t img_splash_b;
extern const lv_img_dsc_t img_bitmap_wifi;
extern const lv_img_dsc_t img_bitmap_refresh;
extern const lv_img_dsc_t img_bitmap_close;
extern const lv_img_dsc_t img_bitmap_language;
extern const lv_img_dsc_t img_bitmap_download_icon;
extern const lv_img_dsc_t img_bitmap_cutter_single;
extern const lv_img_dsc_t img_bitmap_cutter_double;
extern const lv_img_dsc_t img_bitmap_welder_double;
extern const lv_img_dsc_t img_bitmap_gas;
extern const lv_img_dsc_t img_bitmap_welder_single;
extern const lv_img_dsc_t img_bitmap_corner;
extern const lv_img_dsc_t img_bitmap_copy;
extern const lv_img_dsc_t img_bitmap_punch;
extern const lv_img_dsc_t img_bitmap_config;
extern const lv_img_dsc_t img_bitmap_haffler_logo;
extern const lv_img_dsc_t img_bitmap_ice_cream;
extern const lv_img_dsc_t img_bitmap_bakery_changir;
extern const lv_img_dsc_t img_bitmap_bakery_mixer;
extern const lv_image_dsc_t img_flag_en;
extern const lv_image_dsc_t img_flag_tr;
extern const lv_image_dsc_t img_flag_fa;
extern const lv_image_dsc_t img_flag_de;
extern const lv_image_dsc_t img_flag_ru;
extern const lv_image_dsc_t img_book_open;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[6];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/
