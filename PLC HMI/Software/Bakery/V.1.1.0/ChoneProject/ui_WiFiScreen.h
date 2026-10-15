#ifndef UI_WIFI_SCREEN_H
#define UI_WIFI_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

void ui_WiFiScreen_screen_init(void);
void ui_WiFiScreen_open(void);
void ui_WiFiScreen_update_tick(void);

extern lv_obj_t *ui_WiFiScreen;

#ifdef __cplusplus
}
#endif

#endif // UI_WIFI_SCREEN_H
