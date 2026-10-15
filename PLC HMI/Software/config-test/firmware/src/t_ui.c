#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "t_ui.h"
#include "t_screens.h"
#include "t_images.h"
#include "t_actions.h"
#include "t_vars.h"







#if defined(EEZ_FOR_LVGL)

void t_ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&t_objects, sizeof(t_objects), t_imageslist, sizeof(t_imageslist), actions);
}

void t_ui_tick() {
    eez_flow_tick();
    t_tick_screen(g_currentScreen);
}

#else

#include <string.h>

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index < 0 || index >= 15) {
        return 0;
    }
    return ((lv_obj_t **)&t_objects)[index];
}

void t_loadScreen(enum T_ScreensEnum screenId) {
    if ((int)screenId < 1 || (int)screenId > 15) return;
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    if (screen && lv_obj_is_valid(screen)) {
        lv_scr_load(screen);
    }
}

void t_ui_init() {
    t_create_screens();
    t_loadScreen(T_SCREEN_ID_MAIN);

}

void t_ui_tick() {
    t_tick_screen(currentScreen);
}

#endif
