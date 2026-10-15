#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"







#if defined(EEZ_FOR_LVGL)

void ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&objects, sizeof(objects), images, sizeof(images), actions);
}

void ui_tick() {
    eez_flow_tick();
    tick_screen(g_currentScreen);
}

#else

#include <string.h>

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

static void ensureScreenCreated(enum ScreensEnum screenId) {
    switch (screenId) {
        case SCREEN_ID_MAIN_2:
            if (!objects.main_2) create_screen_main_2();
            break;
        case SCREEN_ID_MAIN_3:
            if (!objects.main_3) create_screen_main_3();
            break;
        default:
            break;
    }
}

void loadScreen(enum ScreensEnum screenId) {
    ensureScreenCreated(screenId);
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    if (!screen) return;
    // بدون انیمیشن — FADE باعث می‌شد تصویر صفحهٔ قبلی روی صفحهٔ جدید بیفتد و حس هنگ بدهد
    lv_screen_load(screen);
}

void ui_init() {
    create_screens();
    loadScreen(SCREEN_ID_MAIN_0);
}

void ui_tick() {
    tick_screen(currentScreen);
}

#endif
