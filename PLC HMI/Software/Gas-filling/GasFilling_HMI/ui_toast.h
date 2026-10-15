#ifndef UI_TOAST_H
#define UI_TOAST_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOAST_TYPE_INFO,
    TOAST_TYPE_SUCCESS,
    TOAST_TYPE_WARNING,
    TOAST_TYPE_ERROR
} ui_toast_type_t;

/**
 * Show a non-blocking industrial toast notification on top layer.
 * Auto-dismisses after duration_ms.
 * Touching and holding (long-press) pauses the timer until release.
 *
 * @param title Short title or category (e.g. "خطا", "تایید", "هشدار")
 * @param msg Message body
 * @param duration_ms Visible duration in milliseconds (default 3500ms)
 * @param type Color theme and icon
 */
void ui_toast_show(const char *title, const char *msg, uint32_t duration_ms, ui_toast_type_t type);

/**
 * Periodic poll called from main loop to handle countdown and hold-pause state.
 */
void ui_toast_poll(void);

/**
 * Dismiss active toast immediately.
 */
void ui_toast_dismiss(void);

/**
 * Returns active toast container, or NULL if none.
 */
lv_obj_t *ui_toast_get_container(void);

/**
 * Returns true if the given object is the active toast container.
 */
bool ui_toast_is_container(const lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif // UI_TOAST_H
