#ifndef GASFILLING_UI_RESUME_H
#define GASFILLING_UI_RESUME_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_system.h>
#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Early boot latch to capture crash/hang reset reason and target screen.
 *        MUST be called at the very beginning of setup() before serial/UI init.
 */
void ui_resume_boot_latch_early(void);

/**
 * @brief Returns true if system recovered from a hang and has a valid target screen.
 */
bool ui_resume_has_target(void);

/**
 * @brief Get the restored target screen ID (1..4)
 */
int ui_resume_get_target_screen(void);

/**
 * @brief Get the restored target tab index in Settings (0..4)
 */
int ui_resume_get_target_tab(void);

/**
 * @brief Apply the restored screen/tab if pending.
 *        Called in setup() right after ui_init() and settings are loaded.
 */
void ui_resume_apply_if_pending(void);

/**
 * @brief Checkpoint active screen and tab into RTC memory (Zero Flash Wear).
 *        Called immediately whenever a screen or tab is entered or exited.
 */
void ui_resume_checkpoint(void);

/**
 * @brief Manually mark a specific screen and tab into RTC memory.
 */
void ui_resume_mark_screen(int screen_id, int tab_idx);

/**
 * @brief Periodic background check in loop() to catch any unhooked navigation changes.
 */
void ui_resume_poll(void);

/**
 * @brief Clear the RTC resume record (used on normal power down or factory reset).
 */
void ui_resume_clear(void);

/**
 * @brief Diagnostic functions for reset telemetry.
 */
const char* ui_resume_get_reset_reason_str(void);
esp_reset_reason_t ui_resume_get_reset_reason(void);
bool ui_resume_is_last_boot_hang(void);

#ifdef __cplusplus
}
#endif

#endif /* GASFILLING_UI_RESUME_H */
