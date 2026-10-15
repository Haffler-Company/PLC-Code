#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes and displays the standard Haffler Startup Splash & Loading Screen.
 * 
 * Features:
 * - Clean Apple HIG Dark Slate canvas (#0F172A)
 * - Prominent "HAFFLER" typography with industrial subtitle
 * - Machine-specific icon / graphic placeholder
 * - Animated 3px loading bar with progress callback
 * - Firmware version tag (e.g. V.1.0.0) & Hardware MCU indicator
 * 
 * @param machine_name Machine title in English/Local (e.g. "Single Head Cutting Saw")
 * @param version_str Firmware version string (e.g. "V.1.0.0")
 * @param on_complete_cb Callback function executed when loading hits 100%
 */
void splash_screen_show(const char *machine_name, const char *version_str, void (*on_complete_cb)(void));

/**
 * @brief Updates the splash progress (0 to 100).
 */
void splash_screen_set_progress(uint8_t percent, const char *status_msg);

#ifdef __cplusplus
}
#endif
