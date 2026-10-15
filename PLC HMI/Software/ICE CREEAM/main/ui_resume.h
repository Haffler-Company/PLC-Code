#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_system.h"
#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Early boot latch: inspects reset reason and RTC memory before UI is created.
 * If the boot is due to a crash, task WDT, or software panic, it returns true
 * and sets out_screen and out_tab to the last screen the user was on.
 */
bool ui_resume_check_boot_recovery(enum ScreensEnum *out_screen, int *out_tab);

/**
 * Updates the persistent checkpoint in RTC memory (0 flash wear).
 * Call whenever the user transitions to a new screen or switches tabs.
 */
void ui_resume_checkpoint(enum ScreensEnum screenId, int tab_idx);

/**
 * Marks the system as stable after running for >10 seconds, resetting the crash counter.
 */
void ui_resume_mark_stable(void);

/**
 * Returns true if the current boot was recovered from a crash/hang.
 */
bool ui_resume_is_recovered(void);

/**
 * Clears the recovery checkpoint (e.g. on clean user logout or factory reset).
 */
void ui_resume_clear(void);

/**
 * Enable or disable checkpoint recording. Must be kept false during boot/screen creation.
 */
void ui_resume_set_ready(bool ready);
bool ui_resume_is_ready(void);

/**
 * Marks that a clean post-OTA reboot is about to occur, saving version info in NVS.
 */
void ui_resume_mark_post_ota(const char *prev_ver, const char *new_ver);

/**
 * Returns true if the current boot is immediately after an OTA update.
 */
bool ui_resume_is_post_ota_boot(void);

int ui_resume_get_pending_tab(void);
void ui_resume_clear_pending_tab(void);
void ui_resume_set_pending_tab(int tab);

#ifdef __cplusplus
}
#endif
