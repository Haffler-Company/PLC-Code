#include "images.h"

const ext_img_desc_t images[10] = {
    { "splashScreenIMG", &img_splash_screen_img },
    { "compressor0icon", &img_compressor0icon },
    { "compressor1icon", &img_compressor1icon },
    { "setting0icon", &img_setting0icon },
    { "make0icon", &img_make0icon },
    { "make1icon", &img_make1icon },
    { "paddle0icon", &img_paddle0icon },
    { "paddle1icon", &img_paddle1icon },
    { "astiaj_logo_img", &img_astiaj_logo_img },
    { "astiaj_logo_white_img", (const lv_img_dsc_t *)&img_astiaj_logo_white_img },
};
