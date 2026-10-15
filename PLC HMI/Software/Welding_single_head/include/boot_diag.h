/**
 * @file boot_diag.h
 * @brief Industrial Power-On Self-Test (POST) & Hardware Diagnostics Subsystem
 * @author Haffler Automation Systems
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool has_error;
    bool rtc_fail;
    bool touch_fail;
    bool thermo_fail;
    bool psram_fail;
    bool heap_low;
    bool brownout_detected;
    bool watchdog_detected;
    float current_temp_c;
    uint32_t free_heap_bytes;
    uint32_t free_psram_bytes;
} boot_diag_result_t;

void boot_diag_run_self_test(void);
const boot_diag_result_t* boot_diag_get_result(void);
bool boot_diag_has_error(void);

#ifdef __cplusplus
}
#endif
