#ifndef GASFILLING_OTA_MANAGER_H
#define GASFILLING_OTA_MANAGER_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_CONNECTING_WIFI,
    OTA_STATUS_WIFI_CONNECTED,
    OTA_STATUS_CHECKING_SERVER,
    OTA_STATUS_UP_TO_DATE,
    OTA_STATUS_UPDATE_AVAILABLE,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_VERIFYING_SHA,
    OTA_STATUS_SUCCESS_REBOOTING,
    OTA_STATUS_ERROR
} ota_status_t;

#define OTA_DESC_MAX_LINES 12
#define OTA_DESC_LINE_LEN  160
#define OTA_PINMAP_MAX_LEN 1200

typedef struct {
    char latest_version[32];
    char latest_build_date[32];
    char latest_build_time[16];
    char prog_name[80];
    char company_name[80];
    char company_website[64];
    char expected_sha256[65];
    char changelog[512];
    char desc_fa[OTA_DESC_MAX_LINES][OTA_DESC_LINE_LEN];
    int  desc_fa_count;
    char desc_en[OTA_DESC_MAX_LINES][OTA_DESC_LINE_LEN];
    int  desc_en_count;
    char desc_tr[OTA_DESC_MAX_LINES][OTA_DESC_LINE_LEN];
    int  desc_tr_count;
    char raw_pinmap[OTA_PINMAP_MAX_LEN];
    bool raw_pinmap_ok;
    uint32_t file_size;
    bool update_available;
    bool scan_done;
    bool scan_error;
} ota_info_t;

typedef void (*ota_ui_progress_fn)(uint32_t downloaded, uint32_t total, uint8_t percent, float speed_kbps);
typedef void (*ota_ui_status_fn)(ota_status_t status, const char *message);

#define OTA_PROG_GAS    0
#define OTA_PROG_CONFIG 1
#define OTA_PROG_COUNT  2

void ota_manager_init(void);
void ota_manager_set_callbacks(ota_ui_progress_fn progress_cb, ota_ui_status_fn status_cb);
ota_status_t ota_manager_get_status(void);
uint8_t      ota_manager_get_percent(void);
const char*  ota_manager_get_status_message(void);
const ota_info_t* ota_manager_get_info(void);
const ota_info_t* ota_manager_get_prog_info(int prog_idx);

const char* ota_info_get_prog_name(int prog_idx);
const char* ota_info_get_company_name(int prog_idx);
const char* ota_info_get_company_website(int prog_idx);
const char* ota_info_get_version(int prog_idx);
const char* ota_info_get_build_date(int prog_idx);
const char* ota_info_get_build_time(int prog_idx);
const char* ota_info_get_sha256(int prog_idx);
const char* ota_info_get_pinmap_text(int prog_idx);
int         ota_info_get_desc_count_lang(int prog_idx, int lang);
const char* ota_info_get_desc_line_lang(int prog_idx, int lang, int line);
bool        ota_info_is_available(int prog_idx);
bool        ota_info_is_scan_done(void);
bool        ota_info_is_scan_busy(void);
void        ota_manager_request_rescan(void);

/**
 * @brief Connect to Wi-Fi and query lastfile for both programs from server asynchronously
 * @param ssid Wi-Fi network SSID (NULL uses stored/default)
 * @param pass Wi-Fi network password (NULL uses stored/default)
 */
bool ota_manager_start_check(const char *ssid, const char *pass);

/**
 * @brief Download and stream firmware.bin with SHA-256 verification
 */
bool ota_manager_start_install(void);

/**
 * @brief Download and stream firmware.bin for specific program (0=Gas, 1=Config)
 */
bool ota_manager_start_install_prog(int prog_idx);
bool ota_manager_start_install_url(const char *url);

bool ota_manager_is_wifi_connected(void);
bool ota_manager_connect_wifi_async(const char *ssid, const char *pass);

void ota_manager_save_wifi_credentials(const char *ssid, const char *pass);
void ota_manager_load_wifi_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len);
bool ota_manager_get_saved_wifi_pass(const char *ssid, char *out_pass, size_t max_len);
int ota_manager_scan_wifi(char out_ssids[][33], int max_count);
void ota_manager_start_scan_async(void);
bool ota_manager_is_scan_running(void);
int ota_manager_get_scan_results(char out_ssids[][33], int max_count);

/**
 * @brief Cancel ongoing check or download
 */
void ota_manager_cancel(void);

/**
 * @brief Background tick to process UI callbacks from FreeRTOS task
 */
void ota_manager_poll(void);

/* Automatic Wi-Fi connection and NTP time sync */
void ota_manager_auto_connect(void);
bool ota_manager_sync_ntp_now(void);

/* Link validation for custom URL firmware installation */
bool ota_manager_validate_url(const char *url, size_t *out_size_bytes);

/* Background 1-minute update checker and 5-stage snooze persistence */
uint8_t ota_manager_get_snooze_count(void);
void    ota_manager_increment_snooze(void);
void    ota_manager_reset_snooze(void);
void    ota_manager_dismiss_for_boot(void);
bool    ota_manager_is_dismissed_this_boot(void);
bool    ota_manager_is_update_prompt_pending(char *out_ver, size_t max_len);
void    ota_manager_clear_update_prompt_pending(void);

#ifdef __cplusplus
}
#endif

#endif /* GASFILLING_OTA_MANAGER_H */
