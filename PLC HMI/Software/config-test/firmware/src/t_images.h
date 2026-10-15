#ifndef T_EEZ_LVGL_UI_IMAGES_H
#define T_EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_temps;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t t_imageslist[1];


#ifdef __cplusplus
}
#endif

#endif /*T_EEZ_LVGL_UI_IMAGES_H*/