#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NAV_STATE_IDLE = 0,       /* نشانگر مخفی؛ عملکرد تاچ عادی و فرمان استارت/استاپ به ماشین */
    NAV_STATE_ACTIVE = 1,     /* نشانگر فوکوس فعال؛ پیمایش و تنظیم ویجت‌های صفحه */
    NAV_STATE_KEYBOARD = 2    /* نشانگر روی ماتریس کلیدهای کیبرد یا نام‌پد صنعتی */
} PanelNavState;

typedef struct {
    PanelNavState state;
    lv_obj_t *focus_obj;
    lv_obj_t *items[64];
    int item_count;
    int current_index;
    uint32_t last_activity_ms;
    int current_screen_id;
    uint32_t last_confirm_ms;    /* زمان میلی‌ثانیه آخرین تایید */
    int saved_confirm_index;     /* ایندکس آخرین آیتم تایید شده */
    int saved_screen_id;         /* شناسه صفحه هنگام تایید */
} PanelNavManager;

/* Core Navigation APIs */
void panel_nav_init(void);
void panel_nav_queue_key(uint8_t row, uint8_t key);
void panel_nav_poll_tick(void);
void panel_nav_exit_mode(void);
bool panel_nav_is_active(void);
bool panel_nav_focus_is_repeatable(void);
void panel_nav_notify_touch(void);
void panel_nav_notify_touch_point(int16_t x, int16_t y);
void panel_nav_on_screen_load(int screen_id);

/* Toast Banner APIs (Apple HIG with Draining Progress Bar & Touch-Freeze) */
void panel_nav_show_toast(const char *msg, lv_color_t accent_color);
void panel_nav_toast_info(const char *msg);
void panel_nav_toast_success(const char *msg);
void panel_nav_toast_warning(const char *msg);
void panel_nav_toast_error(const char *msg);

/* Modal Quarantine Helpers */
bool panel_nav_has_active_modal(void);
bool panel_nav_dismiss_active_modal(void);

#ifdef __cplusplus
}
#endif
