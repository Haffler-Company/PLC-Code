#ifndef UI_NOTIFICATION_H
#define UI_NOTIFICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

typedef enum {
    NOTIF_SUCCESS = 0,
    NOTIF_INFO,
    NOTIF_WARNING,
    NOTIF_ERROR
} notification_type_t;

/**
 * @brief Initialize notification system structures
 */
void ui_notification_init(void);

/**
 * @brief Show an Apple HIG floating notification pill on top of current screen
 * @param type Severity level (SUCCESS, INFO, WARNING, ERROR)
 * @param title Short headline text (Persian/English)
 * @param message Detailed description text
 */
void ui_show_notification(notification_type_t type, const char* title, const char* message);

/**
 * @brief Dismiss active notification immediately
 */
void ui_notification_dismiss(void);

#ifdef __cplusplus
}
#endif

#endif // UI_NOTIFICATION_H
