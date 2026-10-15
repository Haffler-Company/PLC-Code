#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_arc1_e1;
extern const lv_img_dsc_t img_icon_setting;
extern const lv_img_dsc_t img_big_alert;
extern const lv_img_dsc_t img_splash_a;
extern const lv_img_dsc_t img_splash_b;
extern const lv_img_dsc_t img_icon_jack;
extern const lv_img_dsc_t img_icon_admin;
extern const lv_img_dsc_t img_icon_update;
extern const lv_img_dsc_t img_icon_exit;
extern const lv_img_dsc_t img_icon_close;
extern const lv_img_dsc_t img_icon_up_28;
extern const lv_img_dsc_t img_icon_down_22;
extern const lv_img_dsc_t img_bitmap_status;
extern const lv_img_dsc_t img_bitmap_chone_gir;
extern const lv_img_dsc_t img_bitmap_download_icon;
extern const lv_img_dsc_t img_bitmap_language;
extern const lv_img_dsc_t img_bitmap_package;
extern const lv_img_dsc_t img_bitmap_wifi;
extern const lv_img_dsc_t img_bitmap_close;
extern const lv_img_dsc_t img_bitmap_refresh;
extern const lv_img_dsc_t img_bitmap_test;
extern const lv_img_dsc_t img_bitmap_single_head;
extern const lv_img_dsc_t img_bitmap_cutter_single;
extern const lv_img_dsc_t img_bitmap_cutter_double;
extern const lv_img_dsc_t img_bitmap_welder_double;
extern const lv_img_dsc_t img_bitmap_gas;
extern const lv_img_dsc_t img_bitmap_welder_single;
extern const lv_img_dsc_t img_bitmap_corner;
extern const lv_img_dsc_t img_bitmap_copy;
extern const lv_img_dsc_t img_bitmap_punch;
extern const lv_img_dsc_t img_bitmap_config;
extern const lv_img_dsc_t img_emergency;
extern const lv_img_dsc_t img_bitmap_haffler_logo;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[32];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/
