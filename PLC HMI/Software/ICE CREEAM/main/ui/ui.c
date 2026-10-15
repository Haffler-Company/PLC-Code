#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"
#include "ui_resume.h"
#include "panel_nav.h"
#include "boot_diag.h"
#include "storage.h"
#include "ui_lang.h"
#include "lvgl_port.h"
#include "adc.h"
#include "wifi.h"
#include "ota_update.h"

static lv_timer_t *s_splash_timer = NULL;

void ui_cancel_splash_timer(void) {
    if (s_splash_timer) {
        lv_timer_delete(s_splash_timer);
        s_splash_timer = NULL;
    }
}

static void splash_to_main_cb(lv_timer_t *t) {
    (void)t;
    s_splash_timer = NULL;
    ui_resume_set_ready(true);
    if (g_boot_f5_service_mode || g_boot_f5_ota_requested) {
        /* در حالت سرویس بوت F5 ورود به صفحه اصلی اکیداً ممنوع است */
        return;
    }
    /* بررسی نهایی کلید F5 روی هر دو خط IO1 و IO2 قبل از ورود به صفحه اصلی */
    int v1 = adc_get_io1();
    int v2 = adc_get_io2();
    if ((v1 >= 350 && v1 <= 1850) || (v2 >= 350 && v2 <= 1850)) {
        g_boot_f5_service_mode = true;
        s_pin_for_boot_wifi = true;
        loadScreen(SCREEN_ID_BOOT_WIFI);
        show_manager_pin_dialog();
        return;
    }
    loadScreen(SCREEN_ID_MAIN_PG);
    ui_resume_checkpoint(SCREEN_ID_MAIN_PG, 0);
    if (boot_diag_has_error() && !boot_diag_is_dismissed()) {
        show_boot_timed_alert();
    }
    extern bool warranty_is_expired_and_locked(void);
    extern void open_warranty_expired_dialog(void);
    if (warranty_is_expired_and_locked()) {
        open_warranty_expired_dialog();
    }
}


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
    switch (index) {
        case 0: return objects.splash_screen;
        case 1: return objects.main_pg;
        case 2: return objects.settings_pg;
        case 3: return objects.screen_framware;
        case 4: return objects.boot_wifi_pg;
        case 5: return objects.screen_warranty;
        default: return NULL;
    }
}

void loadScreenWithTab(enum ScreensEnum screenId, int tab_idx) {
    if (screenId != SCREEN_ID_SETTINGS_PG || tab_idx != 4) {
        io_test_turn_off_all_relays();
    }
    int16_t target = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(target);
    if (!screen) return;
    currentScreen = target;
    panel_nav_on_screen_load(screenId);
    if (screenId == SCREEN_ID_SCREEN_FRAMWARE) {
        screens_framware_on_load();
    }
    lv_screen_load(screen);

    /* پس از بارگذاری صفحه تنظیمات، تب مورد نظر را فعال و پندینگ تب را مقداردهی می‌کنیم */
    if (screenId == SCREEN_ID_SETTINGS_PG) {
        extern void settings_take_snapshot(void);
        settings_take_snapshot();
        ui_resume_set_pending_tab(tab_idx);
        if (objects.obj6 && lv_obj_is_valid(objects.obj6)) {
            lv_obj_update_layout(objects.obj6);
            lv_tabview_set_active(objects.obj6, (uint32_t)tab_idx, LV_ANIM_OFF);
        }
    }

    ui_resume_checkpoint(screenId, tab_idx);
    lvgl_port_wake();
}

void loadScreen(enum ScreensEnum screenId) {
    loadScreenWithTab(screenId, 0);
}

enum ScreensEnum ui_get_screen_id(void) {
    if (currentScreen < 0) {
        return SCREEN_ID_SPLASH_SCREEN;
    }
    return (enum ScreensEnum)(currentScreen + 1);
}

static volatile int s_cached_settings_tab = 0;

void ui_set_cached_settings_tab(int tab) {
    s_cached_settings_tab = tab;
}

int ui_get_settings_tab(void) {
    if (ui_get_screen_id() != SCREEN_ID_SETTINGS_PG) {
        return 0;
    }
    return s_cached_settings_tab;
}

/*
 * B3: تایمر UI Adaptive — به جای tick ثابت ۵۰ms در همه حالات:
 *   - در حالت idle (دستگاه خاموش): 150ms — صرفه‌جویی ۶۶% CPU UI
 *   - در حالت active (make_start, ice_start, wash, drain): 50ms — پاسخ‌گویی کافی
 * ui_adaptive_tick_rate() وضعیت PLC را بدون lock می‌خواند.
 */
extern volatile bool make_start;
extern volatile bool ice_start;
extern volatile bool paddle_start;
extern int32_t s_comp_cooldown_left_s;
extern bool s_drain_mode_active;
extern int32_t s_wash_countdown_s;

extern bool lvgl_port_is_touch_active(void);

static uint32_t ui_adaptive_tick_rate(void)
{
    /* پاسخ‌گویی فوق‌روان و بلادرنگ تاچ خازنی (۱۰۰ هرتز) */
    if (lvgl_port_is_touch_active()) {
        return 10;
    }
    /* رندرینگ فوق‌روان ۶۰ هرتز استاندارد اپل (~۱۶ms) برای کلیه صفحات و فرآیندها بدون لگ */
    return 16;
}

static lv_timer_t *s_ui_tick_timer = NULL;

static void ui_periodic_tick_cb(lv_timer_t *t) {
    ui_tick();
    /* B3: تنظیم پریود بعدی بر اساس وضعیت جاری (فقط در صورت تغییر واقعی پریود) */
    uint32_t next_period = ui_adaptive_tick_rate();
    static uint32_t s_last_period = 0;
    if (next_period != s_last_period && s_ui_tick_timer) {
        s_last_period = next_period;
        lv_timer_set_period(s_ui_tick_timer, next_period);
    }
}

void ui_tick() {
    if (ui_is_smart_guide_active()) {
        return; /* پنجره راهنما فعال است — نیازی به رفرش صفحه پس‌زمینه نیست تا ۱۰۰٪ بار CPU آزاد باشد */
    }
    tick_screen(currentScreen);
}

void ui_init(void) {
    ui_resume_set_ready(false);
    create_screens();

    /* B3: تایمر adaptive — شروع بلادرنگ با ۱۰ms و تنظیم خودکار */
    s_ui_tick_timer = lv_timer_create(ui_periodic_tick_cb, 10, NULL);

    /* اولویت مطلق ۱: نگه‌داشتن کلید F5 در زمان بوت */
    if (g_boot_f5_ota_requested) {
        ui_resume_clear();
        ui_resume_set_ready(true);
        g_boot_f5_service_mode = true;
        s_pin_for_boot_wifi = true;
        loadScreen(SCREEN_ID_BOOT_WIFI);
        show_manager_pin_dialog();
        return;
    }

    enum ScreensEnum resume_scr = SCREEN_ID_SPLASH_SCREEN;
    int resume_tab = 0;
    bool is_recovery = ui_resume_check_boot_recovery(&resume_scr, &resume_tab);

    if (is_recovery && resume_scr > SCREEN_ID_SPLASH_SCREEN) {
        ui_resume_set_ready(true);
        if (resume_scr == SCREEN_ID_SETTINGS_PG) {
            open_settings_tab((uint32_t)resume_tab);
        } else {
            loadScreenWithTab(resume_scr, resume_tab);
        }
        ui_resume_checkpoint(resume_scr, resume_tab);
        panel_nav_toast_info((ui_lang_get() == UI_LANG_FA) ? "بازیابی هوشمند صفحه" :
                             ((ui_lang_get() == UI_LANG_TR) ? "Ekran Kurtarildi" : "Screen Auto-Recovered"));
    } else {
        loadScreen(SCREEN_ID_SPLASH_SCREEN);
        s_splash_timer = lv_timer_create(splash_to_main_cb, 2200, NULL);
        if (s_splash_timer) {
            lv_timer_set_repeat_count(s_splash_timer, 1);
        }
    }
}

#endif
