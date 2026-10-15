/**
 * @file ota_update.h
 * @brief ASTIAJ ASTIAJ Industrial OTA Update Subsystem & Server Metadata Client
 * @author ASTIAJ Company
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_MODEL_CUSTOM       -1
#define OTA_MODEL_ICE_CREAM    0
#define OTA_MODEL_CONFIG_TEST  1
#define OTA_MODEL_COUNT        2

#define OTA_DESC_MAX_LINES     12
#define OTA_LINE_MAX_LEN       256
#define OTA_PINMAP_MAX_LEN     1200

typedef struct {
    char name[96];
    char ver[32];
    char bdate[24];
    char btime[16];
    char sha256[72];
    uint32_t size_bytes;
    char desc_fa[OTA_DESC_MAX_LINES][OTA_LINE_MAX_LEN];
    int desc_fa_cnt;
    char desc_en[OTA_DESC_MAX_LINES][OTA_LINE_MAX_LEN];
    int desc_en_cnt;
    char desc_tr[OTA_DESC_MAX_LINES][OTA_LINE_MAX_LEN];
    int desc_tr_cnt;
    char pinmap_inputs[OTA_PINMAP_MAX_LEN];
    char pinmap_outputs[OTA_PINMAP_MAX_LEN];
    bool is_valid;
    bool scan_error;
} lastfile_info_t;

typedef enum {
    OTA_INST_IDLE = 0,
    OTA_INST_CONNECTING,
    OTA_INST_ERASING,
    OTA_INST_DOWNLOADING,
    OTA_INST_VERIFYING,
    OTA_INST_SUCCESS,
    OTA_INST_FAILED
} ota_install_state_t;

typedef void (*ota_progress_cb_t)(uint32_t bytes_dl, uint32_t total_bytes, uint8_t pct);

/**
 * @brief Initialize OTA subsystem & mutexes
 */
esp_err_t ota_update_init(void);

/**
 * @brief Request background scan of server lastfile metadata for all models
 */
void ota_request_scan(void);
bool ota_is_scan_busy(void);
bool ota_is_scan_done(void);
uint32_t ota_get_scan_revision(void);
void ota_pre_erase_start(void);
void ota_pre_erase_stop(void);
bool ota_is_pre_erase_done(void);

/**
 * @brief Get parsed metadata for a model
 */
const lastfile_info_t *ota_get_info(int model_idx);
const char *ota_get_bin_url(int model_idx);
const char *ota_get_lastfile_url(int model_idx);

/**
 * @brief Custom URL management ("ورود با لینک")
 */
void ota_set_custom_url(const char *url);
const char *ota_get_custom_url(void);
bool ota_is_custom_active(void);

/**
 * @brief Universal Zero-Tofu Sanitizer
 * Strips ZWNJ (\xE2\x80\x8C -> ' '), BiDi marks, special unicode bullets/arrows,
 * and accented glyphs, guaranteeing 0% square characters on ILI9488 displays.
 */
void ota_sanitize_server_text(char *dst, size_t dst_sz, const char *src);

/**
 * @brief Start background HTTPS/HTTP firmware download & flash
 */
esp_err_t ota_start_install(int model_idx, ota_progress_cb_t cb);
void ota_cancel_install(void);
ota_install_state_t ota_get_install_state(void);
const char *ota_get_install_error(void);
uint8_t ota_get_install_pct(void);
uint32_t ota_get_install_downloaded(void);
uint32_t ota_get_install_total(void);
bool ota_is_active(void);

typedef void (*ota_url_check_cb_t)(bool is_valid, int status_code, uint32_t content_length, const char *err_msg);

/**
 * @brief Asynchronously verify if a URL is reachable and valid
 */
esp_err_t ota_check_url_async(const char *url, ota_url_check_cb_t cb);

#ifdef __cplusplus
}
#endif
