#ifndef UI_PROJECT_INFO_SCREEN_H
#define UI_PROJECT_INFO_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

void ui_ProjectInfoScreen_screen_init(void);
void ui_ProjectInfoScreen_open(void);
void ui_ProjectInfoScreen_update_tick(void);

extern lv_obj_t *ui_ProjectInfoScreen;

#ifdef __cplusplus
}
#endif

#endif // UI_PROJECT_INFO_SCREEN_H
