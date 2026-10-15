#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_wifi_types.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SoftAP Defaults */
#define ASTIAJ_SOFTAP_SSID    "ASTIAJ-IceCream"
#define ASTIAJ_SOFTAP_PASS    "12345678"

/* WiFi init & scan */
void wifi_init_once(void);
void wifi_start_scan(void);
void wifi_scan_stop_now(void);
void wifi_trigger_f4_auto_scan(void);

/* Credentials & Persistent Storage */
bool wifi_get_saved_password(const char *ssid, char *out_pass, size_t max_len);
void wifi_save_credentials(const char *ssid, const char *password);
const char *wifi_get_primary_saved_ssid(void);
bool wifi_has_saved_credentials(void);

/* Connection */
void wifi_connect_begin(const char *password);  // شروع اتصال به selected_ssid با پسورد
const char *wifi_get_selected_ssid(void);

/* UI helpers */
void wifi_ap_selected_cb(lv_event_t * e);   // کلیک روی SSID در لیست
void wifi_key_act_show(void);
void wifi_key_act_hide(void);

/* MessageBox helpers */
void wifi_status_msgbox_show(const char *text);
void wifi_status_msgbox_hide(void);
void wifi_status_msgbox_show_ok(const char *text);
void wifi_status_msgbox_show_error(const char *text);

/* --- ADD: disconnect + scan (برای دکمه Scan) --- */
void wifi_disconnect_and_scan(void);

/* Connection status query */
bool wifi_is_connected(void);
bool wifi_is_scanning(void);
const char *wifi_get_connected_ssid(void);
const char *wifi_get_ip_str(void);

/* Radio Power Enable/Disable (Operator Toggle) */
bool wifi_is_enabled(void);
void wifi_set_enabled(bool enable);
void wifi_toggle_enabled(void);

/* SNTP Internet Time Synchronization */
void wifi_sntp_sync_start(void);
bool wifi_sntp_is_synced(void);

/* Programmatic connection from WebPortal */
void wifi_connect_to_ap(const char *ssid, const char *password);
bool wifi_is_connecting(void);
int  wifi_get_last_error_reason(void);

/* SoftAP customizable SSID & Password (persisted in NVS) */
const char *wifi_get_ap_ssid(void);
const char *wifi_get_ap_pass(void);
bool wifi_set_ap_credentials(const char *ssid, const char *pass);
uint8_t wifi_get_ap_client_count(void);

/* Get cached or current AP records for Web & UI */
uint16_t wifi_get_scan_records(wifi_ap_record_t **out_records);
uint16_t wifi_get_scan_records_copy(wifi_ap_record_t *out_records, uint16_t max_count);

/* Scan done event callback hook for custom UI screens */
typedef void (*wifi_scan_done_cb_t)(uint16_t count, wifi_ap_record_t *records);
void wifi_register_scan_done_cb(wifi_scan_done_cb_t cb);
void wifi_unregister_scan_done_cb(wifi_scan_done_cb_t cb);

#ifdef __cplusplus
}
#endif
