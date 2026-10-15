#ifndef PANEL_NAV_H
#define PANEL_NAV_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NAV_STATE_IDLE = 0,       // Cursor hidden; normal touch & machine operation
    NAV_STATE_ACTIVE = 1,     // Cursor active; navigating screen widgets
    NAV_STATE_KEYBOARD = 2    // Cursor navigating on-screen virtual keyboard/numpad
} PanelNavState;

typedef struct {
    PanelNavState state;
    lv_obj_t *focus_obj;
    lv_obj_t *items[64];
    int item_count;
    int current_index;
    uint32_t last_activity_ms;
    int current_screen_id;
} PanelNavManager;

// Core API
void panel_nav_init(void);
void panel_nav_queue_key(uint8_t row, uint8_t key); // Thread-safe queue entry
void panel_nav_poll_tick(void);                     // Called from LVGL periodic timer / loop
void panel_nav_exit_mode(void);
bool panel_nav_is_active(void);
PanelNavState panel_nav_get_state(void);
void panel_nav_notify_touch(void);                  // Call on any digitizer touch
void panel_nav_auto_repeat(int dir);                // Auto-repeat on held START (+1) or STOP (-1)

#ifdef __cplusplus
}
#endif

#endif // PANEL_NAV_H
