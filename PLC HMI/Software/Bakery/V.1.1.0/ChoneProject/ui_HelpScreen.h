#ifndef UI_HELP_SCREEN_H
#define UI_HELP_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

void ui_HelpScreen_screen_init(void);
void ui_HelpScreen_open(void);

extern lv_obj_t *ui_HelpScreen;

#ifdef __cplusplus
}
#endif

#endif // UI_HELP_SCREEN_H
