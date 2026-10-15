#ifndef T_EEZ_LVGL_UI_GUI_H
#define T_EEZ_LVGL_UI_GUI_H

#include <lvgl.h>



#if defined(EEZ_FOR_LVGL)
#include <eez/flow/lvgl_api.h>
#endif

#if !defined(EEZ_FOR_LVGL)
#include "t_screens.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif



void t_ui_init();
void t_ui_tick();

#if !defined(EEZ_FOR_LVGL)
void t_loadScreen(enum T_ScreensEnum screenId);
#endif

#ifdef __cplusplus
}
#endif

#endif // T_EEZ_LVGL_UI_GUI_H