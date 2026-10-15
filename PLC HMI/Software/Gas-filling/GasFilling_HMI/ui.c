#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"
#include "ui_resume.h"

#include <string.h>

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    /* Screen-transition animations can lock the LVGL task on this ESP32-S3.
       A direct load keeps every touch path responsive. */
    lv_scr_load(screen);
    ui_resume_checkpoint();
    if (screenId == SCREEN_ID_WIFI) {
        ui_wifi_on_screen_enter();
    } else if (screenId == SCREEN_ID_FRAMWARE) {
        ui_framware_on_screen_enter();
    }
}

void ui_init() {
    create_screens();
    if (ui_resume_has_target()) {
        loadScreen((enum ScreensEnum)ui_resume_get_target_screen());
    } else {
        loadScreen(SCREEN_ID_MAIN);
    }
}

void ui_tick() {
    tick_screen(currentScreen);
}

int16_t ui_current_screen_id(void) {
    return currentScreen < 0 ? -1 : currentScreen + 1;
}
