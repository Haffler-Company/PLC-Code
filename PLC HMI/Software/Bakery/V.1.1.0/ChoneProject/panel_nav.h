#ifndef PANEL_NAV_H
#define PANEL_NAV_H

#include <stdbool.h>
#include <stdint.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

//? ============================================================================
//? PANEL NAVIGATION 3-STATE FINITE STATE MACHINE (Siemens / Apple HIG)
//? ============================================================================
typedef enum {
    NAV_STATE_IDLE = 0,       // Cursor hidden; normal touch & physical machine operation
    NAV_STATE_ACTIVE = 1,     // Cursor active; navigating screen actionable widgets
    NAV_STATE_KEYBOARD = 2    // Cursor navigating on-screen virtual keyboard / numpad
} PanelNavState;

typedef struct {
    PanelNavState state;
    lv_obj_t *focus_obj;
    lv_obj_t *items[64];
    int item_count;
    int current_index;
    uint32_t last_activity_ms;
} PanelNavManager;

//? ============================================================================
//? CORE INTERFACES & THREAD-SAFE APIs
//? ============================================================================
void panel_nav_init(void);
void panel_nav_queue_key(uint8_t col, uint8_t key); // Thread-safe queue entry from Core 0
void panel_nav_poll_tick(void);                     // Executed on Core 1 under lv_lock()
void panel_nav_exit_mode(void);                     // Dismisses focus cursor & restores IDLE
bool panel_nav_is_active(void);                     // Returns true if in NAV_STATE_ACTIVE or KEYBOARD
void panel_nav_notify_touch(void);                  // Instant touch override: restores IDLE
void panel_nav_on_screen_loaded(void);              // Screen purge to prevent lingering visual ghosts

#ifdef __cplusplus
}
#endif

#endif // PANEL_NAV_H
