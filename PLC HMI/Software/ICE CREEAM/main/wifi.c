#include "wifi.h"
#include "lvgl_port.h"
#include "screens.h"
#include "ui/ui.h"
#include "fonts.h"
#include "ui/ui_lang.h"
#include "ui/ui_image_icons.h"
#include "webportal.h"
#include "cloud_client.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_ota_ops.h"
#include "ui_resume.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sntp.h"
#include "ds3231.h"
#include "panel_nav.h"
#include "ui/actions.h"
#include "day_lock_core.h"
#include "day_lock_ui.h"
#include "storage.h"
#include "esp_timer.h"
#include <time.h>
#include <sys/time.h>
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include <arpa/inet.h>

extern jalali_time_t jt;
extern int32_t ds_year, ds_month, ds_day, ds_hour, ds_minute;

static const char *TAG = "wifi_mod";

/* Scan results */
static wifi_ap_record_t *ap_records = NULL;
static uint16_t ap_count = 0;

/* Selection & state (Static buffers to prevent heap fragmentation & use-after-free) */
static char s_selected_ssid[33] = {0};
static char s_connected_ssid[33] = {0};
static bool s_is_connected = false;
static char s_last_ip[32] = {0};
static bool wifi_inited = false;
static int s_retry_cnt = 0;
static const int s_retry_max = 5;
static bool s_sntp_inited = false;
static bool s_sntp_synced = false;
static bool s_user_initiated_connect = false;
static volatile bool s_preparing_connect = false;
static volatile bool s_kb_modal_open = false;
static bool s_wifi_power_enabled = true;
static bool s_is_scanning = false;
static bool s_is_connecting = false;
static int s_last_disc_reason = 0;
static char s_ap_ssid[33] = ASTIAJ_SOFTAP_SSID;
static char s_ap_pass[65] = ASTIAJ_SOFTAP_PASS;
static bool s_ap_creds_loaded = false;
static uint8_t s_ap_clients_count = 0;

uint8_t wifi_get_ap_client_count(void) {
    return s_ap_clients_count;
}

static void wifi_load_ap_credentials(void) {
    if (s_ap_creds_loaded) return;
    char stored_ssid[33] = {0};
    char stored_pass[65] = {0};
    if (storage_get_str("wifi_ap_ssid", "", stored_ssid, sizeof(stored_ssid)) && stored_ssid[0] != '\0') {
        strncpy(s_ap_ssid, stored_ssid, sizeof(s_ap_ssid) - 1);
        s_ap_ssid[sizeof(s_ap_ssid) - 1] = '\0';
    } else {
        strncpy(s_ap_ssid, ASTIAJ_SOFTAP_SSID, sizeof(s_ap_ssid) - 1);
        s_ap_ssid[sizeof(s_ap_ssid) - 1] = '\0';
    }
    if (storage_get_str("wifi_ap_pass", "", stored_pass, sizeof(stored_pass)) && strlen(stored_pass) >= 8) {
        strncpy(s_ap_pass, stored_pass, sizeof(s_ap_pass) - 1);
        s_ap_pass[sizeof(s_ap_pass) - 1] = '\0';
    } else {
        strncpy(s_ap_pass, ASTIAJ_SOFTAP_PASS, sizeof(s_ap_pass) - 1);
        s_ap_pass[sizeof(s_ap_pass) - 1] = '\0';
    }
    s_ap_creds_loaded = true;
}

const char *wifi_get_ap_ssid(void) {
    if (!s_ap_creds_loaded) wifi_load_ap_credentials();
    return s_ap_ssid;
}

const char *wifi_get_ap_pass(void) {
    if (!s_ap_creds_loaded) wifi_load_ap_credentials();
    return s_ap_pass;
}

bool wifi_set_ap_credentials(const char *ssid, const char *pass) {
    if (!pass || strlen(pass) < 8 || strlen(pass) > 63) {
        return false;
    }
    if (ssid && strlen(ssid) > 0 && strlen(ssid) <= 32) {
        strncpy(s_ap_ssid, ssid, sizeof(s_ap_ssid) - 1);
        s_ap_ssid[sizeof(s_ap_ssid) - 1] = '\0';
        storage_set_str("wifi_ap_ssid", s_ap_ssid);
    }
    strncpy(s_ap_pass, pass, sizeof(s_ap_pass) - 1);
    s_ap_pass[sizeof(s_ap_pass) - 1] = '\0';
    storage_set_str("wifi_ap_pass", s_ap_pass);
    s_ap_creds_loaded = true;

    if (wifi_inited) {
        wifi_config_t cur_ap = {0};
        esp_wifi_get_config(WIFI_IF_AP, &cur_ap);
        strncpy((char *)cur_ap.ap.ssid, s_ap_ssid, sizeof(cur_ap.ap.ssid) - 1);
        cur_ap.ap.ssid_len = strlen(s_ap_ssid);
        strncpy((char *)cur_ap.ap.password, s_ap_pass, sizeof(cur_ap.ap.password) - 1);
        cur_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        cur_ap.ap.pairwise_cipher = WIFI_CIPHER_TYPE_UNKNOWN;
        cur_ap.ap.pmf_cfg.required = false;
        cur_ap.ap.pmf_cfg.capable = true;
        esp_wifi_set_config(WIFI_IF_AP, &cur_ap);
        ESP_LOGI(TAG, "SoftAP credentials updated: SSID='%s', Pass='%s'", s_ap_ssid, s_ap_pass);
    }
    return true;
}

bool wifi_is_connecting(void) {
    return s_is_connecting;
}

int wifi_get_last_error_reason(void) {
    return s_last_disc_reason;
}

/* ---------------- Credentials & Persistent Storage ---------------- */
#define MAX_SAVED_WIFI_NETS 8
typedef struct {
    char ssid[33];
    char pass[65];
} wifi_saved_entry_t;

typedef struct {
    uint8_t count;
    wifi_saved_entry_t entries[MAX_SAVED_WIFI_NETS];
} wifi_saved_table_t;

static wifi_saved_table_t s_saved_tbl = {0};
static bool s_saved_tbl_loaded = false;

#define CHONE_NVS_NS "chone"
#define CHONE_WIFI_MAX 16

/* همگام‌سازی و خواندن شبکه‌های ذخیره‌شده از فضای نام chone (برنامه config-test) */
static void wifi_sync_from_chone_nvs(void) {
    nvs_handle_t chone_h = 0;
    esp_err_t err = nvs_open(CHONE_NVS_NS, NVS_READONLY, &chone_h);
    if (err != ESP_OK) {
        return;
    }

    int32_t wc = 0;
    if (nvs_get_i32(chone_h, "wc", &wc) != ESP_OK) {
        uint32_t uwc = 0;
        if (nvs_get_u32(chone_h, "wc", &uwc) == ESP_OK) {
            wc = (int32_t)uwc;
        } else {
            uint8_t u8wc = 0;
            if (nvs_get_u8(chone_h, "wc", &u8wc) == ESP_OK) {
                wc = (int32_t)u8wc;
            } else {
                wc = CHONE_WIFI_MAX;
            }
        }
    }
    if (wc <= 0 || wc > CHONE_WIFI_MAX) {
        wc = CHONE_WIFI_MAX;
    }

    int imported_count = 0;
    char last_chone_ssid[33] = {0};

    for (int i = 0; i < wc; i++) {
        char key_s[16], key_p[16];
        snprintf(key_s, sizeof(key_s), "ws%d", i);
        snprintf(key_p, sizeof(key_p), "wp%d", i);

        char s_buf[64] = {0};
        char p_buf[65] = {0};
        size_t s_len = sizeof(s_buf);
        size_t p_len = sizeof(p_buf);

        if (nvs_get_str(chone_h, key_s, s_buf, &s_len) == ESP_OK && s_buf[0] != '\0') {
            nvs_get_str(chone_h, key_p, p_buf, &p_len);
            strncpy(last_chone_ssid, s_buf, sizeof(last_chone_ssid) - 1);

            int found_idx = -1;
            for (uint8_t k = 0; k < s_saved_tbl.count; k++) {
                if (strncmp(s_saved_tbl.entries[k].ssid, s_buf, 32) == 0) {
                    found_idx = (int)k;
                    break;
                }
            }

            if (found_idx >= 0) {
                if (p_buf[0] != '\0') {
                    strncpy(s_saved_tbl.entries[found_idx].pass, p_buf, sizeof(s_saved_tbl.entries[found_idx].pass) - 1);
                    s_saved_tbl.entries[found_idx].pass[sizeof(s_saved_tbl.entries[found_idx].pass) - 1] = '\0';
                }
            } else {
                if (s_saved_tbl.count < MAX_SAVED_WIFI_NETS) {
                    found_idx = s_saved_tbl.count++;
                } else {
                    for (int k = 0; k < MAX_SAVED_WIFI_NETS - 1; k++) {
                        s_saved_tbl.entries[k] = s_saved_tbl.entries[k + 1];
                    }
                    found_idx = MAX_SAVED_WIFI_NETS - 1;
                }
                strncpy(s_saved_tbl.entries[found_idx].ssid, s_buf, sizeof(s_saved_tbl.entries[found_idx].ssid) - 1);
                s_saved_tbl.entries[found_idx].ssid[sizeof(s_saved_tbl.entries[found_idx].ssid) - 1] = '\0';
                strncpy(s_saved_tbl.entries[found_idx].pass, p_buf, sizeof(s_saved_tbl.entries[found_idx].pass) - 1);
                s_saved_tbl.entries[found_idx].pass[sizeof(s_saved_tbl.entries[found_idx].pass) - 1] = '\0';
                imported_count++;
            }
        }
    }
    nvs_close(chone_h);

    if (imported_count > 0) {
        storage_set_blob("wifi_saved_tbl", &s_saved_tbl, sizeof(s_saved_tbl));
        ESP_LOGI(TAG, "Imported %d WiFi credential(s) from config-test ('chone'). Latest: '%s'",
                 imported_count, last_chone_ssid);
    }
}

/* بازیابی آخرین و جدیدترین شبکه ذخیره‌شده در برنامه تست (chone) */
static bool wifi_get_latest_chone_credentials(char *out_ssid, size_t max_ssid_len, char *out_pass, size_t max_pass_len) {
    if (!out_ssid || max_ssid_len == 0) return false;
    out_ssid[0] = '\0';
    if (out_pass && max_pass_len > 0) out_pass[0] = '\0';

    nvs_handle_t chone_h = 0;
    if (nvs_open(CHONE_NVS_NS, NVS_READONLY, &chone_h) != ESP_OK) {
        return false;
    }

    int32_t wc = 0;
    if (nvs_get_i32(chone_h, "wc", &wc) != ESP_OK) {
        uint32_t uwc = 0;
        if (nvs_get_u32(chone_h, "wc", &uwc) == ESP_OK) wc = (int32_t)uwc;
        else wc = CHONE_WIFI_MAX;
    }
    if (wc <= 0 || wc > CHONE_WIFI_MAX) wc = CHONE_WIFI_MAX;

    for (int i = wc - 1; i >= 0; i--) {
        char key_s[16], key_p[16];
        snprintf(key_s, sizeof(key_s), "ws%d", i);
        snprintf(key_p, sizeof(key_p), "wp%d", i);

        char s_buf[64] = {0};
        size_t s_req = sizeof(s_buf);
        if (nvs_get_str(chone_h, key_s, s_buf, &s_req) == ESP_OK && s_buf[0] != '\0') {
            strncpy(out_ssid, s_buf, max_ssid_len - 1);
            out_ssid[max_ssid_len - 1] = '\0';
            if (out_pass && max_pass_len > 0) {
                size_t p_req = max_pass_len;
                nvs_get_str(chone_h, key_p, out_pass, &p_req);
            }
            nvs_close(chone_h);
            return true;
        }
    }
    nvs_close(chone_h);
    return false;
}

/* ذخیره دوطرفه در فضای نام chone جهت سازگاری کامل در صورت بازگشت به config-test */
static void wifi_sync_to_chone_nvs(const char *ssid, const char *password) {
    if (!ssid || !*ssid) return;
    nvs_handle_t chone_h = 0;
    if (nvs_open(CHONE_NVS_NS, NVS_READWRITE, &chone_h) != ESP_OK) {
        return;
    }

    int32_t wc = 0;
    if (nvs_get_i32(chone_h, "wc", &wc) != ESP_OK) {
        uint32_t uwc = 0;
        if (nvs_get_u32(chone_h, "wc", &uwc) == ESP_OK) wc = (int32_t)uwc;
        else wc = 0;
    }
    if (wc < 0) wc = 0;
    if (wc > CHONE_WIFI_MAX) wc = CHONE_WIFI_MAX;

    int found_idx = -1;
    for (int i = 0; i < wc; i++) {
        char key_s[16];
        snprintf(key_s, sizeof(key_s), "ws%d", i);
        char s_buf[64] = {0};
        size_t s_len = sizeof(s_buf);
        if (nvs_get_str(chone_h, key_s, s_buf, &s_len) == ESP_OK && strcmp(s_buf, ssid) == 0) {
            found_idx = i;
            break;
        }
    }

    const char *pwd = password ? password : "";
    if (found_idx >= 0) {
        char key_p[16];
        snprintf(key_p, sizeof(key_p), "wp%d", found_idx);
        nvs_set_str(chone_h, key_p, pwd);
    } else {
        if (wc < CHONE_WIFI_MAX) {
            char key_s[16], key_p[16];
            snprintf(key_s, sizeof(key_s), "ws%d", (int)wc);
            snprintf(key_p, sizeof(key_p), "wp%d", (int)wc);
            nvs_set_str(chone_h, key_s, ssid);
            nvs_set_str(chone_h, key_p, pwd);
            wc++;
            nvs_set_i32(chone_h, "wc", wc);
        } else {
            for (int i = 1; i < CHONE_WIFI_MAX; i++) {
                char k_s_prev[16], k_p_prev[16], k_s_curr[16], k_p_curr[16];
                snprintf(k_s_prev, sizeof(k_s_prev), "ws%d", i - 1);
                snprintf(k_p_prev, sizeof(k_p_prev), "wp%d", i - 1);
                snprintf(k_s_curr, sizeof(k_s_curr), "ws%d", i);
                snprintf(k_p_curr, sizeof(k_p_curr), "wp%d", i);

                char buf_s[64] = {0}, buf_p[65] = {0};
                size_t ls = sizeof(buf_s), lp = sizeof(buf_p);
                if (nvs_get_str(chone_h, k_s_curr, buf_s, &ls) == ESP_OK) {
                    nvs_set_str(chone_h, k_s_prev, buf_s);
                }
                if (nvs_get_str(chone_h, k_p_curr, buf_p, &lp) == ESP_OK) {
                    nvs_set_str(chone_h, k_p_prev, buf_p);
                }
            }
            char key_s[16], key_p[16];
            snprintf(key_s, sizeof(key_s), "ws%d", CHONE_WIFI_MAX - 1);
            snprintf(key_p, sizeof(key_p), "wp%d", CHONE_WIFI_MAX - 1);
            nvs_set_str(chone_h, key_s, ssid);
            nvs_set_str(chone_h, key_p, pwd);
            nvs_set_i32(chone_h, "wc", CHONE_WIFI_MAX);
        }
    }
    nvs_commit(chone_h);
    nvs_close(chone_h);
}

static void wifi_load_saved_table(void) {
    if (s_saved_tbl_loaded) return;
    memset(&s_saved_tbl, 0, sizeof(s_saved_tbl));
    int res = storage_get_blob("wifi_saved_tbl", &s_saved_tbl, sizeof(s_saved_tbl));
    if (res != (int)sizeof(s_saved_tbl) || s_saved_tbl.count > MAX_SAVED_WIFI_NETS) {
        memset(&s_saved_tbl, 0, sizeof(s_saved_tbl));
        char old_ssid[33] = {0};
        char old_pass[65] = {0};
        if (storage_get_str("wifi_sta_ssid", "", old_ssid, sizeof(old_ssid)) && old_ssid[0] != '\0') {
            storage_get_str("wifi_sta_pass", "", old_pass, sizeof(old_pass));
            strncpy(s_saved_tbl.entries[0].ssid, old_ssid, sizeof(s_saved_tbl.entries[0].ssid) - 1);
            strncpy(s_saved_tbl.entries[0].pass, old_pass, sizeof(s_saved_tbl.entries[0].pass) - 1);
            s_saved_tbl.count = 1;
        }
    }
    /* واردسازی و هماهنگی تمامی شبکه‌های ذخیره‌شده از config-test */
    wifi_sync_from_chone_nvs();
    s_saved_tbl_loaded = true;
}

bool wifi_get_saved_password(const char *ssid, char *out_pass, size_t max_len) {
    if (!ssid || !*ssid || !out_pass || max_len == 0) return false;
    out_pass[0] = '\0';
    wifi_load_saved_table();

    for (uint8_t i = 0; i < s_saved_tbl.count; i++) {
        if (strncmp(s_saved_tbl.entries[i].ssid, ssid, 32) == 0) {
            strncpy(out_pass, s_saved_tbl.entries[i].pass, max_len - 1);
            out_pass[max_len - 1] = '\0';
            return (out_pass[0] != '\0');
        }
    }

    char prim_ssid[33] = {0};
    if (storage_get_str("wifi_sta_ssid", "", prim_ssid, sizeof(prim_ssid)) &&
        strncmp(prim_ssid, ssid, 32) == 0) {
        storage_get_str("wifi_sta_pass", "", out_pass, max_len);
        return (out_pass[0] != '\0');
    }

    /* جستجوی مستقیم در فضای نام chone به عنوان مسیر نجات تکمیلی */
    nvs_handle_t chone_h = 0;
    if (nvs_open(CHONE_NVS_NS, NVS_READONLY, &chone_h) == ESP_OK) {
        int32_t wc = 0;
        if (nvs_get_i32(chone_h, "wc", &wc) != ESP_OK) {
            uint32_t uwc = 0;
            if (nvs_get_u32(chone_h, "wc", &uwc) == ESP_OK) wc = (int32_t)uwc;
            else wc = CHONE_WIFI_MAX;
        }
        if (wc <= 0 || wc > CHONE_WIFI_MAX) wc = CHONE_WIFI_MAX;
        for (int i = 0; i < wc; i++) {
            char key_s[16], key_p[16];
            snprintf(key_s, sizeof(key_s), "ws%d", i);
            snprintf(key_p, sizeof(key_p), "wp%d", i);
            char cur_s[64] = {0};
            size_t req_s = sizeof(cur_s);
            if (nvs_get_str(chone_h, key_s, cur_s, &req_s) == ESP_OK && strcmp(cur_s, ssid) == 0) {
                size_t req_p = max_len;
                if (nvs_get_str(chone_h, key_p, out_pass, &req_p) == ESP_OK && out_pass[0] != '\0') {
                    nvs_close(chone_h);
                    return true;
                }
            }
        }
        nvs_close(chone_h);
    }

    wifi_config_t cfg = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &cfg) == ESP_OK &&
        strncmp((char*)cfg.sta.ssid, ssid, 32) == 0 && cfg.sta.password[0] != '\0') {
        strncpy(out_pass, (char*)cfg.sta.password, max_len - 1);
        out_pass[max_len - 1] = '\0';
        return true;
    }

    return false;
}

void wifi_save_credentials(const char *ssid, const char *password) {
    if (!ssid || !*ssid) return;
    wifi_load_saved_table();

    /* محافظت حیاتی: در صورت ارسال رشته خالی، پسورد معتبر قبلی را هرگز پاک نکن */
    char existing_pwd[65] = {0};
    if ((!password || !*password) && wifi_get_saved_password(ssid, existing_pwd, sizeof(existing_pwd))) {
        password = existing_pwd;
    }
    const char *pwd = password ? password : "";

    int found_idx = -1;
    for (uint8_t i = 0; i < s_saved_tbl.count; i++) {
        if (strncmp(s_saved_tbl.entries[i].ssid, ssid, 32) == 0) {
            found_idx = (int)i;
            break;
        }
    }

    if (found_idx >= 0) {
        if (pwd && *pwd) {
            strncpy(s_saved_tbl.entries[found_idx].pass, pwd, sizeof(s_saved_tbl.entries[found_idx].pass) - 1);
            s_saved_tbl.entries[found_idx].pass[sizeof(s_saved_tbl.entries[found_idx].pass) - 1] = '\0';
        }
    } else {
        if (s_saved_tbl.count < MAX_SAVED_WIFI_NETS) {
            found_idx = s_saved_tbl.count++;
        } else {
            for (int i = 0; i < MAX_SAVED_WIFI_NETS - 1; i++) {
                s_saved_tbl.entries[i] = s_saved_tbl.entries[i + 1];
            }
            found_idx = MAX_SAVED_WIFI_NETS - 1;
        }
        strncpy(s_saved_tbl.entries[found_idx].ssid, ssid, sizeof(s_saved_tbl.entries[found_idx].ssid) - 1);
        s_saved_tbl.entries[found_idx].ssid[sizeof(s_saved_tbl.entries[found_idx].ssid) - 1] = '\0';
        strncpy(s_saved_tbl.entries[found_idx].pass, pwd, sizeof(s_saved_tbl.entries[found_idx].pass) - 1);
        s_saved_tbl.entries[found_idx].pass[sizeof(s_saved_tbl.entries[found_idx].pass) - 1] = '\0';
    }

    storage_set_blob("wifi_saved_tbl", &s_saved_tbl, sizeof(s_saved_tbl));
    storage_set_str("wifi_sta_ssid", ssid);
    storage_set_str("wifi_sta_pass", pwd);
    wifi_sync_to_chone_nvs(ssid, pwd);
    ESP_LOGI(TAG, "WiFi credentials saved permanently for SSID: '%s' (synced to 'app' & 'chone')", ssid);
}

const char *wifi_get_primary_saved_ssid(void) {
    static char s_prim_buf[33] = {0};
    if (storage_get_str("wifi_sta_ssid", "", s_prim_buf, sizeof(s_prim_buf)) && s_prim_buf[0] != '\0') {
        return s_prim_buf;
    }
    return NULL;
}

bool wifi_has_saved_credentials(void) {
    wifi_load_saved_table();
    if (s_saved_tbl.count > 0) {
        for (uint8_t i = 0; i < s_saved_tbl.count; i++) {
            if (s_saved_tbl.entries[i].ssid[0] != '\0') {
                return true;
            }
        }
    }
    char prim_buf[33] = {0};
    if (storage_get_str("wifi_sta_ssid", "", prim_buf, sizeof(prim_buf)) && prim_buf[0] != '\0') {
        return true;
    }
    wifi_config_t cfg = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &cfg) == ESP_OK && cfg.sta.ssid[0] != '\0') {
        return true;
    }
    return false;
}

/* ---------------- helpers ---------------- */
__attribute__((unused)) static const char* authmode_str(wifi_auth_mode_t m){
    switch(m){
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        default: return "UNKNOWN";
    }
}
const char *wifi_get_selected_ssid(void){ return s_selected_ssid[0] ? s_selected_ssid : NULL; }

/* ---------------- keyboard & textarea visibility ---------------- */
void wifi_key_act_hide(void) {
    s_kb_modal_open = false;
    if (objects.wifi_pass_key) {
        lv_keyboard_set_textarea(objects.wifi_pass_key, NULL);
        lv_obj_add_flag(objects.wifi_pass_key, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.wifi_pass_txtarea) {
        lv_obj_add_flag(objects.wifi_pass_txtarea, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.wifi_pass_cancel_btn) {
        lv_obj_add_flag(objects.wifi_pass_cancel_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.wifi_kb_overlay) {
        lv_obj_add_flag(objects.wifi_kb_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}
void wifi_key_act_show(void) {
    s_kb_modal_open = true;
    /* توقف تلاش‌های پس‌زمینه هنگام باز شدن کیبورد رمز تا رادیو در لحظه زدن دکمه اتصال کاملاً آماده و آزاد باشد */
    if (wifi_inited && !s_is_connected && !s_user_initiated_connect) {
        s_preparing_connect = true;
        esp_wifi_disconnect();
        s_preparing_connect = false;
    }
    if (objects.wifi_kb_overlay) {
        lv_obj_clear_flag(objects.wifi_kb_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.wifi_kb_overlay);
    }
    if (objects.wifi_pass_txtarea) {
        char saved_pwd[65] = {0};
        if (s_selected_ssid[0] && wifi_get_saved_password(s_selected_ssid, saved_pwd, sizeof(saved_pwd))) {
            lv_textarea_set_text(objects.wifi_pass_txtarea, saved_pwd);
            lv_textarea_set_cursor_pos(objects.wifi_pass_txtarea, LV_TEXTAREA_CURSOR_LAST);
        } else {
            lv_textarea_set_text(objects.wifi_pass_txtarea, "");
        }
        lv_obj_clear_flag(objects.wifi_pass_txtarea, LV_OBJ_FLAG_HIDDEN);
        lv_group_t *g = lv_group_get_default();
        if (g) lv_group_add_obj(g, objects.wifi_pass_txtarea);
    }
    if (objects.wifi_pass_cancel_btn) {
        lv_obj_clear_flag(objects.wifi_pass_cancel_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.wifi_pass_key) {
        lv_obj_clear_flag(objects.wifi_pass_key, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(objects.wifi_pass_key, objects.wifi_pass_txtarea);
        lv_keyboard_set_mode(objects.wifi_pass_key, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_move_foreground(objects.wifi_pass_key);
    }
}

/* ---------------- MessageBox (async-safe) ---------------- */
typedef enum {
    MSG_KIND_NEUTRAL = 0,
    MSG_KIND_OK,
    MSG_KIND_ERR
} msg_kind_t;

typedef struct {
    char *text;
    msg_kind_t kind;
} msgbox_payload_t;

static void wifi_status_msgbox_hide_cb(void *arg) {
    (void)arg;
    s_user_initiated_connect = false;
    if (objects.wifi_status_messagebox) {
        lv_obj_add_flag(objects.wifi_status_messagebox, LV_OBJ_FLAG_HIDDEN);
    }
}

static void apply_msgbox_bg_by_kind(lv_obj_t *mbox, msg_kind_t kind) {
    lv_obj_set_style_bg_color(mbox, lv_color_hex(0x1e293b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(mbox, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(mbox, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_color_t border_clr;
    switch (kind) {
        case MSG_KIND_OK:
            border_clr = lv_color_hex(0x10b981); // Emerald Green Accent
            break;
        case MSG_KIND_ERR:
            border_clr = lv_color_hex(0xef4444); // Crimson Red Accent
            break;
        default:
            border_clr = lv_color_hex(0x475569); // Slate Gray
            break;
    }
    lv_obj_set_style_border_color(mbox, border_clr, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(mbox, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
}


static void wifi_mbox_ok_event_cb(lv_event_t *e) {
    (void)e;
    s_user_initiated_connect = false;
    wifi_status_msgbox_hide();
}

static void wifi_status_msgbox_show_cb(void *arg) {
    msgbox_payload_t *pl = (msgbox_payload_t *)arg;
    if (!objects.wifi_status_messagebox) { if (pl){ free(pl->text); free(pl);} return; }

    lv_obj_t *mbox = objects.wifi_status_messagebox;

    lv_obj_move_foreground(mbox);

    lv_obj_clear_flag(mbox, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(mbox);

    lv_obj_set_width(mbox, 300);
    lv_obj_set_height(mbox, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(mbox, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(mbox, 16, 0);
    lv_obj_set_style_pad_row(mbox, 8, 0);
    lv_obj_set_style_min_height(mbox, 120, 0);
    apply_msgbox_bg_by_kind(mbox, pl ? pl->kind : MSG_KIND_NEUTRAL);

    /* Status Symbol (Apple HIG Iconography) */
    lv_obj_t *icon = lv_label_create(mbox);
    if (pl && pl->kind == MSG_KIND_ERR) {
        lv_label_set_text(icon, LV_SYMBOL_WARNING);
        lv_obj_set_style_text_color(icon, lv_color_hex(0xef4444), 0);
    } else if (pl && pl->kind == MSG_KIND_OK) {
        lv_label_set_text(icon, LV_SYMBOL_OK);
        lv_obj_set_style_text_color(icon, lv_color_hex(0x10b981), 0);
    } else {
        lv_label_set_text(icon, LV_SYMBOL_BELL);
        lv_obj_set_style_text_color(icon, lv_color_hex(0x38bdf8), 0);
    }
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);

    /* Text message */
    const char *txt = pl && pl->text ? pl->text : "";
    lv_obj_t *lbl = lv_label_create(mbox);
    lv_label_set_text(lbl, txt);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 260);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_base_dir(lbl, ui_lang_base_dir(), 0);

    /* OK Button (Apple System Blue) */
    lv_obj_t *ok = lv_btn_create(mbox);
    lv_obj_set_size(ok, 100, 36);
    lv_obj_set_style_radius(ok, 8, 0);
    lv_obj_set_style_bg_color(ok, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_shadow_width(ok, 0, 0);
    lv_obj_set_ext_click_area(ok, 6);
    lv_obj_t *ok_lbl = lv_label_create(ok);
    const char *btn_txt = (ui_lang_get() == UI_LANG_FA) ? "تأیید" : ((ui_lang_get() == UI_LANG_TR) ? "Tamam" : "OK");
    lv_label_set_text(ok_lbl, btn_txt);
    lv_obj_set_style_text_font(ok_lbl, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(ok_lbl);
    lv_obj_add_event_cb(ok, wifi_mbox_ok_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_center(mbox);

    if (pl) { free(pl->text); free(pl); }
}


/* بسته ایمن برای انتقال نتایج اسکن از Core 0 به Core 1 بدون race condition */
typedef struct {
    wifi_ap_record_t *records;
    uint16_t count;
} ap_snapshot_t;

static void lvgl_update_list_cb(void *arg);

static inline void safe_async_call(lv_async_cb_t async_xcb, void *user_data) {
    if (lvgl_lock(pdMS_TO_TICKS(100))) {
        lv_async_call(async_xcb, user_data);
        lvgl_unlock();
    } else {
        /* در صورت عدم دسترسی به قفل LVGL، جلوگیری از نشت حافظه اسنپ‌شات اسکن */
        if (async_xcb == lvgl_update_list_cb && user_data) {
            ap_snapshot_t *snap = (ap_snapshot_t *)user_data;
            if (snap->records) free(snap->records);
            free(snap);
        }
    }
}

/* public APIs (async-safe) */
void wifi_status_msgbox_hide(void) {
    safe_async_call(wifi_status_msgbox_hide_cb, NULL);
}
static void wifi_status_msgbox_show_kind(const char *text, msg_kind_t kind) {
    msgbox_payload_t *pl = (msgbox_payload_t *)malloc(sizeof(msgbox_payload_t));
    if (!pl) return;
    pl->kind = kind;
    if (text) {
        size_t n = strlen(text) + 1;
        pl->text = (char *)malloc(n);
        if (!pl->text) { free(pl); return; }
        memcpy(pl->text, text, n);
    } else {
        pl->text = NULL;
    }
    if (lvgl_lock(pdMS_TO_TICKS(100))) {
        lv_async_call(wifi_status_msgbox_show_cb, pl);
        lvgl_unlock();
    } else {
        if (pl->text) free(pl->text);
        free(pl);
    }
}
void wifi_status_msgbox_show(const char *text) { wifi_status_msgbox_show_kind(text, MSG_KIND_NEUTRAL); }
void wifi_status_msgbox_show_ok(const char *text) { wifi_status_msgbox_show_kind(text, MSG_KIND_OK); }
void wifi_status_msgbox_show_error(const char *text) { wifi_status_msgbox_show_kind(text, MSG_KIND_ERR); }

/* ---------------- List scanning banner ---------------- */
static void lvgl_show_scanning_cb(void *arg) {
    (void)arg;
    if (!objects.ui_scan_list) return;

    /* آزادسازی حافظه نام‌های قبلی قبل از پاک‌سازی لیست */
    uint32_t child_cnt = lv_obj_get_child_count(objects.ui_scan_list);
    for (uint32_t c = 0; c < child_cnt; c++) {
        lv_obj_t *child = lv_obj_get_child(objects.ui_scan_list, c);
        if (child) {
            char *old_ssid = (char*)lv_obj_get_user_data(child);
            if (old_ssid) {
                free(old_ssid);
                lv_obj_set_user_data(child, NULL);
            }
        }
    }
    lv_obj_clean(objects.ui_scan_list);

    lv_obj_t *lbl_scan = lv_label_create(objects.ui_scan_list);
    lv_label_set_text(lbl_scan, ui_lang_str(STR_WIFI_SCANNING));
    lv_obj_set_width(lbl_scan, LV_PCT(100));
    lv_label_set_long_mode(lbl_scan, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl_scan, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_scan, ui_lang_font_16(), 0);
    lv_obj_set_style_text_color(lbl_scan, lv_color_hex(0x0284c7), 0);
    lv_obj_set_style_base_dir(lbl_scan, ui_lang_base_dir(), 0);
    lv_obj_set_style_pad_ver(lbl_scan, 16, 0);
    if (objects.scan_wifi_btn) lv_obj_add_state(objects.scan_wifi_btn, LV_STATE_DISABLED);
}

/* پالایش و ضدعفونی نام وای‌فای از هرگونه استیکر، ایموجی یا نمادهای ناسازگار جهت تضمین صفر درصد مربع (Zero Tofu) */
void wifi_sanitize_ssid_for_display(const char *src, char *dst, size_t dst_max)
{
    if (!dst || dst_max == 0) return;
    dst[0] = '\0';
    if (!src || !*src) {
        strncpy(dst, (ui_lang_get() == UI_LANG_FA) ? "شبکه بی‌نام" : "Wi-Fi AP", dst_max - 1);
        dst[dst_max - 1] = '\0';
        return;
    }

    size_t in_len = strlen(src);
    const uint8_t *p = (const uint8_t *)src;
    const uint8_t *end = p + in_len;
    size_t out_idx = 0;
    bool last_was_space = true; /* مهار فاصله‌های زائد ابتدایی */

    while (p < end && out_idx + 4 < dst_max) {
        uint8_t c = *p;
        uint32_t cp = 0;
        size_t step = 0;

        if (c < 0x80) {
            cp = c;
            step = 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (p + 1 >= end) break;
            cp = ((c & 0x1F) << 6) | (p[1] & 0x3F);
            step = 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (p + 2 >= end) break;
            cp = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            step = 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (p + 3 >= end) break;
            cp = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
            step = 4;
        } else {
            p++;
            continue;
        }

        p += step;

        /* حذف بی‌صدای Variation Selector-16 و موارد مشابه (مانند U+FE0F در ⚙️) */
        if (cp >= 0xFE00 && cp <= 0xFE0F) {
            continue;
        }

        /* نویسه‌های استاندارد قابل چاپ اسکی */
        if (cp >= 0x20 && cp <= 0x7E) {
            if (cp == ' ') {
                if (!last_was_space) {
                    dst[out_idx++] = ' ';
                    last_was_space = true;
                }
            } else {
                dst[out_idx++] = (char)cp;
                last_was_space = false;
            }
            continue;
        }

        /* نویسه‌های فارسی و عربی استاندارد */
        if ((cp >= 0x0600 && cp <= 0x06FF) || (cp >= 0xFB50 && cp <= 0xFDFF) || (cp >= 0xFE70 && cp <= 0xFEFF)) {
            if (cp <= 0x7FF) {
                dst[out_idx++] = (char)(0xC0 | ((cp >> 6) & 0x1F));
                dst[out_idx++] = (char)(0x80 | (cp & 0x3F));
            } else {
                dst[out_idx++] = (char)(0xE0 | ((cp >> 12) & 0x0F));
                dst[out_idx++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                dst[out_idx++] = (char)(0x80 | (cp & 0x3F));
            }
            last_was_space = false;
            continue;
        }

        /* تبدیل خط تیره‌ها و نشانه‌های یونیکد به معادل‌های تک‌بایتی استاندارد */
        if (cp == 0x2013 || cp == 0x2014) { /* en-dash / em-dash */
            dst[out_idx++] = '-';
            last_was_space = false;
            continue;
        }
        if (cp == 0x2022) { /* bullet */
            dst[out_idx++] = '*';
            last_was_space = false;
            continue;
        }
        if (cp >= 0x2018 && cp <= 0x201D) { /* smart quotes */
            dst[out_idx++] = '\'';
            last_was_space = false;
            continue;
        }

        /* نویسه‌های ترکی استانبولی بر پایه استاندارد AGENTS.md (حذف مربع‌های ترکی) */
        if (cp == 0x00E7 || cp == 0x00C7) { dst[out_idx++] = (cp == 0x00E7) ? 'c' : 'C'; last_was_space = false; continue; }
        if (cp == 0x00F6 || cp == 0x00D6) { dst[out_idx++] = (cp == 0x00F6) ? 'o' : 'O'; last_was_space = false; continue; }
        if (cp == 0x00FC || cp == 0x00DC) { dst[out_idx++] = (cp == 0x00FC) ? 'u' : 'U'; last_was_space = false; continue; }
        if (cp == 0x011F || cp == 0x011E) { dst[out_idx++] = (cp == 0x011F) ? 'g' : 'G'; last_was_space = false; continue; }
        if (cp == 0x0131 || cp == 0x0130) { dst[out_idx++] = (cp == 0x0131) ? 'i' : 'I'; last_was_space = false; continue; }
        if (cp == 0x015F || cp == 0x015E) { dst[out_idx++] = (cp == 0x015F) ? 's' : 'S'; last_was_space = false; continue; }

        /* کلیه استیکرها، ایموجی‌ها و نمادهای غیرقابل رندر (شامل چرخ‌دنده ⚙ U+2699 و ایموجی‌های مدرن) */
        if (!last_was_space) {
            dst[out_idx++] = ' ';
            last_was_space = true;
        }
    }

    /* حذف فاصله زائد انتهای نام */
    while (out_idx > 0 && dst[out_idx - 1] == ' ') {
        out_idx--;
    }
    dst[out_idx] = '\0';

    if (out_idx == 0) {
        strncpy(dst, (ui_lang_get() == UI_LANG_FA) ? "شبکه بی‌نام" : "Wi-Fi AP", dst_max - 1);
        dst[dst_max - 1] = '\0';
    }
}

/* ---------------- Smart Touch vs. Scroll Discriminator for Wi-Fi List ---------------- */
static lv_coord_t s_wifi_touch_start_x = 0;
static lv_coord_t s_wifi_touch_start_y = 0;
static lv_coord_t s_wifi_touch_max_move = 0;
static lv_coord_t s_wifi_touch_start_scroll_y = 0;
static int64_t    s_wifi_last_scroll_us = 0;
static bool       s_wifi_list_scroll_cb_bound = false;

static void wifi_scan_list_scroll_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCROLL_BEGIN || code == LV_EVENT_SCROLL || code == LV_EVENT_SCROLL_END) {
        s_wifi_last_scroll_us = esp_timer_get_time();
        s_wifi_touch_max_move = 100; /* علامت‌گذاری قطعی به عنوان حرکت اسکرول */
    }
}

static void wifi_ap_row_touch_track_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t pt = {0, 0};
    if (indev) {
        lv_indev_get_point(indev, &pt);
    }

    if (code == LV_EVENT_PRESSED) {
        s_wifi_touch_start_x = pt.x;
        s_wifi_touch_start_y = pt.y;
        s_wifi_touch_max_move = 0;
        s_wifi_touch_start_scroll_y = (objects.ui_scan_list && lv_obj_is_valid(objects.ui_scan_list))
                                      ? lv_obj_get_scroll_y(objects.ui_scan_list) : 0;
    } else if (code == LV_EVENT_PRESSING) {
        lv_coord_t dx = LV_ABS(pt.x - s_wifi_touch_start_x);
        lv_coord_t dy = LV_ABS(pt.y - s_wifi_touch_start_y);
        lv_coord_t move = dx + dy;
        if (move > s_wifi_touch_max_move) {
            s_wifi_touch_max_move = move;
        }
        if (objects.ui_scan_list && lv_obj_is_valid(objects.ui_scan_list)) {
            lv_coord_t d_scr = LV_ABS(lv_obj_get_scroll_y(objects.ui_scan_list) - s_wifi_touch_start_scroll_y);
            if (d_scr > 3) {
                s_wifi_last_scroll_us = esp_timer_get_time();
                if (s_wifi_touch_max_move < 50) s_wifi_touch_max_move = 50;
            }
        }
    }
}

static void wifi_ip_banner_click_cb(lv_event_t *e)
{
    (void)e;
    if (!s_is_connected || s_last_ip[0] == '\0') return;
    char clean_ssid[48] = {0};
    wifi_sanitize_ssid_for_display(s_connected_ssid[0] ? s_connected_ssid : "مودم", clean_ssid, sizeof(clean_ssid));
    char msg[220];
    if (ui_lang_get() == UI_LANG_FA) {
        snprintf(msg, sizeof(msg),
                 "آدرس پنل وب:\nhttp://%s\nگوشی را به وای‌فای «%s» (بدون فیلترشکن) وصل کنید.",
                 s_last_ip, clean_ssid);
    } else if (ui_lang_get() == UI_LANG_TR) {
        snprintf(msg, sizeof(msg),
                 "Web Panel Adresi:\nhttp://%s\nTelefonu '%s' agina (VPN kapali) baglayin.",
                 s_last_ip, clean_ssid);
    } else {
        snprintf(msg, sizeof(msg),
                 "Web Portal URL:\nhttp://%s\nConnect phone to '%s' (VPN OFF).",
                 s_last_ip, clean_ssid);
    }
    wifi_status_msgbox_show_ok(msg);
}

/* ---------------- List refresh (with connected mark) ---------------- */
static void lvgl_update_list_cb(void *arg) {
    ap_snapshot_t *snap = (ap_snapshot_t *)arg;

    /* اگر کاربر در صفحه یا تب وای‌فای حضور ندارد، لیست را بازسازی نکن تا مانع لگ سایر صفحات نشود */
    enum ScreensEnum cur_scr = ui_get_screen_id();
    if (cur_scr != SCREEN_ID_BOOT_WIFI && (cur_scr != SCREEN_ID_SETTINGS_PG || ui_get_settings_tab() != 3)) {
        if (snap) { free(snap->records); free(snap); }
        return;
    }

    if (!objects.ui_scan_list) {
        if (snap) { free(snap->records); free(snap); }
        return;
    }

    if (!s_wifi_list_scroll_cb_bound) {
        lv_obj_add_event_cb(objects.ui_scan_list, wifi_scan_list_scroll_event_cb, LV_EVENT_ALL, NULL);
        s_wifi_list_scroll_cb_bound = true;
    }

    /* آزادسازی حافظه نام‌های قبلی قبل از پاک‌سازی لیست برای جلوگیری از Memory Leak */
    uint32_t child_cnt = lv_obj_get_child_count(objects.ui_scan_list);
    for (uint32_t c = 0; c < child_cnt; c++) {
        lv_obj_t *child = lv_obj_get_child(objects.ui_scan_list, c);
        if (child) {
            char *old_ssid = (char*)lv_obj_get_user_data(child);
            if (old_ssid) {
                free(old_ssid);
                lv_obj_set_user_data(child, NULL);
            }
        }
    }
    lv_obj_clean(objects.ui_scan_list);

    const wifi_ap_record_t *records = (snap && snap->records) ? snap->records : ap_records;
    uint16_t count = (snap && snap->records) ? snap->count : ap_count;

    if (s_is_connected && s_last_ip[0] != '\0') {
        lv_obj_t *top_banner = lv_obj_create(objects.ui_scan_list);
        lv_obj_set_size(top_banner, LV_PCT(100), 30);
        lv_obj_set_style_radius(top_banner, 8, 0);
        lv_obj_set_style_bg_color(top_banner, lv_color_hex(0x0284c7), 0);
        lv_obj_set_style_border_width(top_banner, 0, 0);
        lv_obj_set_style_pad_all(top_banner, 2, 0);
        lv_obj_clear_flag(top_banner, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_add_flag(top_banner, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(top_banner, wifi_ip_banner_click_cb, LV_EVENT_SHORT_CLICKED, NULL);

        lv_obj_t *lbl_ip = lv_label_create(top_banner);
        char ip_text[96];
        snprintf(ip_text, sizeof(ip_text), "http://%s", s_last_ip);
        lv_label_set_text(lbl_ip, ip_text);
        lv_obj_set_style_base_dir(lbl_ip, LV_BASE_DIR_LTR, 0);
        lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_ip, lv_color_hex(0xffffff), 0);
        lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(lbl_ip, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLL_CHAIN_VER);
        lv_obj_center(lbl_ip);
    }

    if (count == 0) {
        lv_obj_t *lbl_empty = lv_label_create(objects.ui_scan_list);
        if (s_is_connected && s_last_ip[0] != '\0') {
            lv_label_set_text_fmt(lbl_empty, "دستگاه به «%s» متصل است.\nجهت جستجوی مجدد شبکه‌ها، دکمه [جستجو] را بزنید.", s_connected_ssid[0] ? s_connected_ssid : "وای‌فای");
        } else {
            lv_label_set_text(lbl_empty, "شبکه‌ای یافت نشد یا جستجو نشده است.\nجهت جستجو، دکمه [جستجو] را لمس کنید.");
        }
        lv_obj_set_width(lbl_empty, LV_PCT(100));
        lv_label_set_long_mode(lbl_empty, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(lbl_empty, ui_lang_font_16(), 0);
        lv_obj_set_style_text_color(lbl_empty, lv_color_hex(0x64748b), 0);
        lv_obj_set_style_base_dir(lbl_empty, ui_lang_base_dir(), 0);
        lv_obj_set_style_pad_ver(lbl_empty, 14, 0);
    }

    for (uint16_t i = 0; i < count; ++i) {
        char ssid[33] = {0};
        strncpy(ssid, (const char*)records[i].ssid, sizeof(ssid)-1);

        bool is_connected = (s_connected_ssid[0] && strcmp(s_connected_ssid, ssid) == 0);

        /* Apple HIG Inset Grouped Row (38px so 2 rows + 30px IP banner fit cleanly in 126px) */
        lv_obj_t *btn = lv_list_add_btn(objects.ui_scan_list, NULL, NULL);
        lv_obj_set_size(btn, LV_PCT(100), 38);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_bg_color(btn, is_connected ? lv_color_hex(0xf0fdf4) : lv_color_hex(0xffffff), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, is_connected ? lv_color_hex(0x86efac) : lv_color_hex(0xe2e8f0), 0);
        lv_obj_set_style_pad_hor(btn, 10, 0);
        lv_obj_set_style_pad_ver(btn, 2, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_base_dir(btn, LV_BASE_DIR_LTR, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS | LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_GESTURE_BUBBLE);

        /* 1. Left: Checkmark for connected network */
        lv_obj_t *lbl_chk = lv_label_create(btn);
        if (is_connected) {
            lv_label_set_text(lbl_chk, LV_SYMBOL_OK);
            lv_obj_set_style_text_color(lbl_chk, lv_color_hex(0x16a34a), 0);
        } else {
            lv_label_set_text(lbl_chk, " ");
        }
        lv_obj_set_style_text_font(lbl_chk, &lv_font_montserrat_16, 0);
        lv_obj_set_width(lbl_chk, 22);
        lv_obj_clear_flag(lbl_chk, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(lbl_chk, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

        /* 2. Middle: SSID name (use Montserrat for pure ASCII so digits in X28-ASTIAJ stay Latin) */
        char clean_ssid[48] = {0};
        wifi_sanitize_ssid_for_display(ssid, clean_ssid, sizeof(clean_ssid));
        bool has_non_ascii = false;
        for (const char *p = clean_ssid; *p; ++p) {
            if ((unsigned char)*p >= 0x80) {
                has_non_ascii = true;
                break;
            }
        }

        lv_obj_t *lbl_ssid = lv_label_create(btn);
        lv_label_set_text(lbl_ssid, clean_ssid);
        lv_obj_set_style_text_font(lbl_ssid, has_non_ascii ? &ui_font_yekan_16 : &lv_font_montserrat_16, 0);
        lv_obj_set_style_base_dir(lbl_ssid, has_non_ascii ? LV_BASE_DIR_AUTO : LV_BASE_DIR_LTR, 0);
        lv_obj_set_style_text_color(lbl_ssid, is_connected ? lv_color_hex(0x16a34a) : lv_color_hex(0x0f172a), 0);
        lv_label_set_long_mode(lbl_ssid, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_flex_grow(lbl_ssid, 1);
        lv_obj_clear_flag(lbl_ssid, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(lbl_ssid, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

        /* 3. Right side: Lock icon (if secured network) */
        if (records[i].authmode != WIFI_AUTH_OPEN) {
            lv_obj_t *img_lock = lv_image_create(btn);
            lv_image_set_src(img_lock, &img_icon_wifi_lock);
            lv_obj_set_style_pad_right(img_lock, 8, 0);
            lv_obj_clear_flag(img_lock, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(img_lock, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLL_CHAIN_VER);
        }

        /* 4. Right side: Dynamic Wi-Fi signal icon */
        const lv_image_dsc_t *sig_img = &img_icon_wifi_sig_1;
        int8_t rssi = records[i].rssi;
        if (rssi >= -60) {
            sig_img = &img_icon_wifi_sig_4;
        } else if (rssi >= -70) {
            sig_img = &img_icon_wifi_sig_3;
        } else if (rssi >= -82) {
            sig_img = &img_icon_wifi_sig_2;
        }
        lv_obj_t *img_sig = lv_image_create(btn);
        lv_image_set_src(img_sig, sig_img);
        lv_obj_clear_flag(img_sig, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(img_sig, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

        char *ssid_copy = strdup(ssid);
        lv_obj_set_user_data(btn, ssid_copy);
        lv_obj_add_event_cb(btn, wifi_ap_row_touch_track_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(btn, wifi_ap_row_touch_track_cb, LV_EVENT_PRESSING, NULL);
        lv_obj_add_event_cb(btn, wifi_ap_selected_cb, LV_EVENT_SHORT_CLICKED, NULL);
    }

    /* آزاد کردن snapshot بعد از استفاده */
    if (snap) { free(snap->records); free(snap); }

    if (objects.scan_wifi_btn) lv_obj_clear_state(objects.scan_wifi_btn, LV_STATE_DISABLED);
    extern void boot_wifi_enable_scan_btn(void);
    boot_wifi_enable_scan_btn();
}

/* State tracking getters */
bool wifi_is_connected(void) {
    return s_is_connected;
}

bool wifi_is_scanning(void) {
    return s_is_scanning;
}

const char *wifi_get_connected_ssid(void) {
    return s_connected_ssid[0] ? s_connected_ssid : NULL;
}

const char *wifi_get_ip_str(void) {
    if (s_is_connected && s_last_ip[0] != '\0') {
        return s_last_ip;
    }
    return "192.168.4.1";
}

/* Helper to hide boot wifi prompt dialog when IP connection succeeds */
static void async_hide_boot_wifi_prompt_cb(void *user_data)
{
    (void)user_data;
    hide_boot_wifi_prompt_dialog();
}



static int ap_rssi_compare(const void *a, const void *b) {
    const wifi_ap_record_t *ra = (const wifi_ap_record_t *)a;
    const wifi_ap_record_t *rb = (const wifi_ap_record_t *)b;
    return (int)rb->rssi - (int)ra->rssi;
}

static wifi_scan_done_cb_t s_custom_scan_done_cb = NULL;

void wifi_register_scan_done_cb(wifi_scan_done_cb_t cb) {
    s_custom_scan_done_cb = cb;
}

void wifi_unregister_scan_done_cb(wifi_scan_done_cb_t cb) {
    if (s_custom_scan_done_cb == cb) {
        s_custom_scan_done_cb = NULL;
    }
}

static void wifi_custom_scan_done_async_cb(void *arg) {
    (void)arg;
    if (s_custom_scan_done_cb) {
        s_custom_scan_done_cb(ap_count, ap_records);
    }
}

/* ---------------- Event handlers ---------------- */
static void wifi_event_handler(void* arg, esp_event_base_t base,
                               int32_t id, void* data) {
    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_SCAN_DONE: {
            s_is_scanning = false;
            uint16_t raw_count = 0;
            esp_wifi_scan_get_ap_num(&raw_count);
            if (ap_records) { free(ap_records); ap_records = NULL; }
            ap_count = 0;

            ap_snapshot_t *snap = (ap_snapshot_t *)calloc(1, sizeof(ap_snapshot_t));

            if (raw_count > 0) {
                wifi_ap_record_t *raw_records = calloc(raw_count, sizeof(wifi_ap_record_t));
                if (raw_records) {
                    esp_wifi_scan_get_ap_records(&raw_count, raw_records);
                    /* مرتب‌سازی بر اساس قدرت سیگنال (قوی‌ترین در ابتدای لیست) */
                    qsort(raw_records, raw_count, sizeof(wifi_ap_record_t), ap_rssi_compare);

                    /* پالایش و حذف موارد تکراری یا خالی */
                    wifi_ap_record_t *clean_records = calloc(raw_count, sizeof(wifi_ap_record_t));
                    if (clean_records) {
                        uint16_t dedup_cnt = 0;
                        for (uint16_t i = 0; i < raw_count; i++) {
                            const char *s = (const char *)raw_records[i].ssid;
                            if (!s || !*s) continue;
                            bool dup = false;
                            for (uint16_t j = 0; j < dedup_cnt; j++) {
                                if (strncmp((const char *)clean_records[j].ssid, s, 32) == 0) {
                                    dup = true;
                                    break;
                                }
                            }
                            if (!dup) {
                                memcpy(&clean_records[dedup_cnt], &raw_records[i], sizeof(wifi_ap_record_t));
                                /* پالایش کاراکترهای خارج از محدوده اسکی و ایموجی‌ها جهت تضمین ۱۰۰٪ Zero Tofu */
                                for (int c = 0; c < 32 && clean_records[dedup_cnt].ssid[c] != '\0'; c++) {
                                    unsigned char ch = (unsigned char)clean_records[dedup_cnt].ssid[c];
                                    if (ch < 32 || ch > 126) {
                                        clean_records[dedup_cnt].ssid[c] = ' ';
                                    }
                                }
                                dedup_cnt++;
                            }
                        }
                        /* گلوبال برای wifi_ap_selected_cb */
                        ap_records = clean_records;
                        ap_count = dedup_cnt;

                        /* snapshot مستقل برای UI (بدون race condition) */
                        if (snap) {
                            snap->records = calloc(dedup_cnt, sizeof(wifi_ap_record_t));
                            if (snap->records) {
                                memcpy(snap->records, clean_records, dedup_cnt * sizeof(wifi_ap_record_t));
                                snap->count = dedup_cnt;
                            }
                        }
                    }
                    free(raw_records);
                }
            }
            safe_async_call(lvgl_update_list_cb, snap);
            if (s_custom_scan_done_cb) {
                safe_async_call(wifi_custom_scan_done_async_cb, NULL);
            }
            break;
        }
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)data;
            if (event) {
                ESP_LOGI(TAG, "SoftAP: mobile client joined, AID=%d, MAC=%02x:%02x:%02x:%02x:%02x:%02x",
                         event->aid, event->mac[0], event->mac[1], event->mac[2],
                         event->mac[3], event->mac[4], event->mac[5]);
            }
            if (s_ap_clients_count < 255) s_ap_clients_count++;
            /* اگر هنوز به مودم وصل نیستیم و اپراتور به هات‌اسپات دستگاه وصل شده، تلاش پس‌زمینه STA را متوقف کن تا کانال رادیو پرش نکند */
            if (!s_is_connected && !s_user_initiated_connect) {
                esp_wifi_disconnect();
            }
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)data;
            if (event) {
                ESP_LOGI(TAG, "SoftAP: mobile client left, AID=%d, MAC=%02x:%02x:%02x:%02x:%02x:%02x",
                         event->aid, event->mac[0], event->mac[1], event->mac[2],
                         event->mac[3], event->mac[4], event->mac[5]);
            }
            if (s_ap_clients_count > 0) s_ap_clients_count--;
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *disc = (wifi_event_sta_disconnected_t *)data;
            int reason = disc ? disc->reason : -1;

            /* نادیده گرفتن قطع اتصال عمدی در حین آماده‌سازی اتصال جدید (جلوگیری از مصرف کاذب ۳ تلاش و برخورد وضعیت‌ها) */
            if (s_preparing_connect || reason == WIFI_REASON_ASSOC_LEAVE) {
                ESP_LOGI(TAG, "Ignoring intentional STA disconnect during connect setup (reason=%d)", reason);
                s_is_connected = false;
                s_last_ip[0] = '\0';
                break;
            }

            s_last_disc_reason = reason;
            ESP_LOGW(TAG, "Disconnected, reason=%d", reason);
            s_is_connected = false;
            s_last_ip[0] = '\0';
            cloud_client_on_wifi_disconnected();

            /* خطای قطعی رمز اشتباه صریح در WPA/WPA2 */
            if (reason == WIFI_REASON_AUTH_FAIL) {
                s_retry_cnt = s_retry_max;
                s_connected_ssid[0] = '\0';
                s_is_connecting = false;
                if (s_user_initiated_connect) {
                    const char *pw_err = (ui_lang_get() == UI_LANG_FA) ? "رمز عبور اشتباه است!\nلطفاً رمز را مجدداً وارد فرمایید." : ((ui_lang_get() == UI_LANG_TR) ? "Sifre yanlis!\nLutfen sifreyi tekrar girin." : "Incorrect password!\nPlease re-enter password.");
                    wifi_status_msgbox_show_error(pw_err);
                }
                safe_async_call(lvgl_update_list_cb, NULL);
            } else if (s_user_initiated_connect && !s_is_scanning && s_retry_cnt < 3) {
                /* تلاش مجدد با مکث استاندارد جهت پاک‌سازی نشست ناقص مودم و پایداری کانال رادیویی */
                s_retry_cnt++;
                if (s_user_initiated_connect) {
                    char msg[96];
                    if (ui_lang_get() == UI_LANG_FA) {
                        snprintf(msg, sizeof(msg), "در حال تلاش مجدد... (%d/3)", s_retry_cnt);
                    } else if (ui_lang_get() == UI_LANG_TR) {
                        snprintf(msg, sizeof(msg), "Tekrar deneniyor... (%d/3)", s_retry_cnt);
                    } else {
                        snprintf(msg, sizeof(msg), "Retrying... (%d/3)", s_retry_cnt);
                    }
                    wifi_status_msgbox_show(msg);
                }
                vTaskDelay(pdMS_TO_TICKS(180));
                if (!s_preparing_connect && !s_is_scanning) {
                    esp_wifi_connect();
                }
            } else {
                /* اتمام تلاش‌های مرحله‌ای اولیه و واگذاری به تسک پس‌زمینه */
                s_connected_ssid[0] = '\0';
                s_is_connecting = false;
                const char *reason_str = (ui_lang_get() == UI_LANG_FA) ? "ارتباط با مودم برقرار نشد" : ((ui_lang_get() == UI_LANG_TR) ? "Modem baglantisi saglanamadi" : "Could not connect to AP");
                if (disc) {
                    switch (disc->reason) {
                        case WIFI_REASON_NO_AP_FOUND:
                            reason_str = (ui_lang_get() == UI_LANG_FA) ? "مودم انتخابی یافت نشد" : ((ui_lang_get() == UI_LANG_TR) ? "Secilen modem bulunamadi" : "AP not found");
                            break;
                        case WIFI_REASON_ASSOC_FAIL:
                            reason_str = (ui_lang_get() == UI_LANG_FA) ? "خطای همگام‌سازی با مودم" : ((ui_lang_get() == UI_LANG_TR) ? "Modem senkronizasyon hatasi" : "Association failed");
                            break;
                        case WIFI_REASON_HANDSHAKE_TIMEOUT:
                        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                            reason_str = (ui_lang_get() == UI_LANG_FA) ? "رمز عبور نادرست است یا سیگنال مودم ضعیف است" : ((ui_lang_get() == UI_LANG_TR) ? "Sifre yanlis veya sinyal zayif" : "Incorrect password or weak signal");
                            break;
                        default: break;
                    }
                }
                if (s_user_initiated_connect) {
                    char msg[128];
                    if (ui_lang_get() == UI_LANG_FA) {
                        snprintf(msg, sizeof(msg), "خطا در اتصال: %s", reason_str);
                    } else if (ui_lang_get() == UI_LANG_TR) {
                        snprintf(msg, sizeof(msg), "Baglanti hatasi: %s", reason_str);
                    } else {
                        snprintf(msg, sizeof(msg), "Connection error: %s", reason_str);
                    }
                    wifi_status_msgbox_show_error(msg);
                }
                safe_async_call(lvgl_update_list_cb, NULL);
            }
            break;
        }
        default:
            break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        s_retry_cnt = 0;
        s_is_connected = true;
        s_is_connecting = false;
        s_last_disc_reason = 0;
        storage_set_i32("wifi_skip_cnt", 0);

        /* ثبت نام واقعی شبکه متصل‌شده از درایور رادیو */
        wifi_ap_record_t ap_info = {0};
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK && ap_info.ssid[0] != '\0') {
            strncpy(s_connected_ssid, (const char *)ap_info.ssid, sizeof(s_connected_ssid) - 1);
            s_connected_ssid[sizeof(s_connected_ssid) - 1] = '\0';
        } else {
            strncpy(s_connected_ssid, s_selected_ssid, sizeof(s_connected_ssid) - 1);
            s_connected_ssid[sizeof(s_connected_ssid) - 1] = '\0';
        }

        snprintf(s_last_ip, sizeof(s_last_ip), IPSTR, IP2STR(&event->ip_info.ip));
        if (s_user_initiated_connect) {
            char clean_ssid[48] = {0};
            wifi_sanitize_ssid_for_display(s_connected_ssid[0] ? s_connected_ssid : "مودم", clean_ssid, sizeof(clean_ssid));
            char msg[160];
            snprintf(msg, sizeof(msg), "اتصال موفق به %s\nآی‌پی دستگاه: %s",
                     clean_ssid, s_last_ip);
            wifi_status_msgbox_show_ok(msg);
        }
        safe_async_call(lvgl_update_list_cb, NULL);
        safe_async_call(async_hide_boot_wifi_prompt_cb, NULL);


        /* تنظیم DNS پشتیبان پایدار (Cloudflare 1.1.1.1 و Google 8.8.8.8) */
        esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (sta_netif) {
            esp_netif_dns_info_t dns_fb;
            dns_fb.ip.type = ESP_IPADDR_TYPE_V4;
            dns_fb.ip.u_addr.ip4.addr = ipaddr_addr("1.1.1.1");
            esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_FALLBACK, &dns_fb);

            esp_netif_dns_info_t dns_bk;
            dns_bk.ip.type = ESP_IPADDR_TYPE_V4;
            dns_bk.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
            esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_BACKUP, &dns_bk);
            ESP_LOGI(TAG, "DNS Fallbacks installed (1.1.1.1 & 8.8.8.8)");
        }

        ESP_LOGI(TAG, "Connected, IP: %s -> requesting OTA scan & SNTP time sync", s_last_ip);
        extern void ota_request_scan(void);
        ota_request_scan();
        extern void ota_pre_erase_start(void);
        ota_pre_erase_start();
        wifi_sntp_sync_start();
        cloud_client_on_wifi_connected();
        extern void warranty_on_wifi_connected(void);
        warranty_on_wifi_connected();
    }
}

/* -------------- همگام‌سازی فرکانس SoftAP با کانال مودم هدف -------------- */
/* در حالت APSTA رادیوی ESP32 تک‌کاناله است؛ اگر کانال SoftAP با کانال مودم یکی نباشد،
 * در تبادل ۴ مرحله‌ای کلید WPA2 پکت‌ها گم شده و خطای Reason 15 (Handshake Timeout) رخ می‌دهد */
static void wifi_sync_softap_channel_to_target(const char *ssid)
{
    if (!ssid || !*ssid || !ap_records || ap_count == 0) return;
    uint8_t target_chan = 0;
    for (uint16_t i = 0; i < ap_count; i++) {
        if (strncmp((const char *)ap_records[i].ssid, ssid, 32) == 0) {
            target_chan = ap_records[i].primary;
            break;
        }
    }
    if (target_chan >= 1 && target_chan <= 13) {
        wifi_config_t cur_ap_cfg = {0};
        if (esp_wifi_get_config(WIFI_IF_AP, &cur_ap_cfg) == ESP_OK) {
            if (cur_ap_cfg.ap.channel != target_chan) {
                ESP_LOGI(TAG, "Syncing SoftAP channel %d -> %d to match target AP '%s' (Zero Handshake Drop)",
                         cur_ap_cfg.ap.channel, target_chan, ssid);
                cur_ap_cfg.ap.channel = target_chan;
                esp_wifi_set_config(WIFI_IF_AP, &cur_ap_cfg);
                /* مکث تثبیت فرکانس رادیو پس از تغییر کانال SoftAP تا اولین تلاش اتصال STA مختل نشود */
                vTaskDelay(pdMS_TO_TICKS(80));
            }
        }
    }
}

/* -------------- Background Auto-Reconnect Task (Multi-Network Failover) -------------- */
/* چرخش هوشمند میان تمام شبکه‌های ذخیره‌شده (تا ۸ شبکه) جهت اتصال مجدد و مصونیت از قطعی مودم کارگاه */
static void wifi_bg_reconnect_task(void *pv)
{
    (void)pv;
    static uint8_t s_reconnect_cycle_idx = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); /* بررسی هر ۳۰ ثانیه */
        /* اگر گوشی اپراتور به SoftAP وصل است، یا در حال ورود رمز است، یا OTA در حال اجراست، تداخل ایجاد نکن */
        extern bool ota_is_active(void);
        if (s_ap_clients_count > 0 || s_kb_modal_open || s_preparing_connect || s_is_connecting || ota_is_active()) {
            continue;
        }
        if (!s_is_connected && wifi_inited && s_wifi_power_enabled && !s_is_scanning && !s_user_initiated_connect) {
            wifi_load_saved_table();

            if (s_saved_tbl.count > 0) {
                uint8_t idx = s_reconnect_cycle_idx % s_saved_tbl.count;
                s_reconnect_cycle_idx++;

                const char *target_ssid = s_saved_tbl.entries[idx].ssid;
                const char *target_pass = s_saved_tbl.entries[idx].pass;

                if (target_ssid && target_ssid[0] != '\0') {
                    if (!s_kb_modal_open && s_selected_ssid[0] == '\0') {
                        strncpy(s_selected_ssid, target_ssid, sizeof(s_selected_ssid) - 1);
                        s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';
                    }

                    wifi_sync_softap_channel_to_target(target_ssid);

                    wifi_config_t cfg = {0};
                    strncpy((char*)cfg.sta.ssid, target_ssid, sizeof(cfg.sta.ssid) - 1);
                    if (target_pass && target_pass[0] != '\0') {
                        strncpy((char*)cfg.sta.password, target_pass, sizeof(cfg.sta.password) - 1);
                    }
                    cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
                    cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
                    cfg.sta.failure_retry_cnt = 2;
                    cfg.sta.pmf_cfg.capable = true;
                    cfg.sta.pmf_cfg.required = false;
                    wifi_mode_t cur_mode = WIFI_MODE_NULL;
                    if (esp_wifi_get_mode(&cur_mode) == ESP_OK && cur_mode != WIFI_MODE_APSTA) {
                        esp_wifi_set_mode(WIFI_MODE_APSTA);
                    }
                    esp_wifi_set_config(WIFI_IF_STA, &cfg);
                    s_retry_cnt = 0;
                    s_user_initiated_connect = false;
                    ESP_LOGI(TAG, "BG Reconnect (%d/%d): attempting saved SSID '%s'", idx + 1, s_saved_tbl.count, target_ssid);
                    esp_err_t conn_err = esp_wifi_connect();
                    if (conn_err != ESP_OK) {
                        ESP_LOGW(TAG, "BG Reconnect esp_wifi_connect error: %s", esp_err_to_name(conn_err));
                    }
                }
            } else {
                char saved_ssid[33] = {0};
                char saved_pass[65] = {0};
                if (storage_get_str("wifi_sta_ssid", "", saved_ssid, sizeof(saved_ssid)) && saved_ssid[0] != '\0') {
                    storage_get_str("wifi_sta_pass", "", saved_pass, sizeof(saved_pass));
                    wifi_sync_softap_channel_to_target(saved_ssid);

                    wifi_config_t cfg = {0};
                    strncpy((char*)cfg.sta.ssid, saved_ssid, sizeof(cfg.sta.ssid) - 1);
                    if (saved_pass[0] != '\0') {
                        strncpy((char*)cfg.sta.password, saved_pass, sizeof(cfg.sta.password) - 1);
                    }
                    cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
                    cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
                    cfg.sta.failure_retry_cnt = 2;
                    cfg.sta.pmf_cfg.capable = true;
                    cfg.sta.pmf_cfg.required = false;
                    esp_wifi_set_storage(WIFI_STORAGE_RAM);
                    {
                        wifi_mode_t cur_mode = WIFI_MODE_NULL;
                        if (esp_wifi_get_mode(&cur_mode) == ESP_OK && cur_mode != WIFI_MODE_APSTA) {
                            esp_wifi_set_mode(WIFI_MODE_APSTA);
                        }
                    }
                    esp_wifi_set_config(WIFI_IF_STA, &cfg);
                    s_retry_cnt = 0;
                    s_user_initiated_connect = false;
                    ESP_LOGI(TAG, "BG Reconnect: attempting single saved SSID '%s'", saved_ssid);
                    esp_wifi_connect();
                }
            }
        }
    }
}

void wifi_init_once(void) {
    if (wifi_inited) return;

    /* بازیابی وضعیت رادیوی وای‌فای از حافظه NVS */
    s_wifi_power_enabled = storage_get_bool("wifi_en", true);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.static_rx_buf_num = 10;
    cfg.dynamic_rx_buf_num = 32;
    cfg.static_tx_buf_num = 16;
    cfg.cache_tx_buf_num = 32;
    cfg.mgmt_sbuf_num = 32;
    esp_err_t wifi_init_err = esp_wifi_init(&cfg);
    if (wifi_init_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed (%s), retrying with minimal buffers", esp_err_to_name(wifi_init_err));
        cfg.static_rx_buf_num = 4;
        cfg.dynamic_rx_buf_num = 12;
        cfg.static_tx_buf_num = 4;
        cfg.cache_tx_buf_num = 8;
        cfg.mgmt_sbuf_num = 12;
        wifi_init_err = esp_wifi_init(&cfg);
        if (wifi_init_err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_init fatal error: %s", esp_err_to_name(wifi_init_err));
            return;
        }
    }

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    /* تنظیم دامنه فرکانسی ایران (کانال‌های ۱ تا ۱۳) با توان حداکثری ۲۰dBm */
    wifi_country_t country = {
        .cc = "IR",
        .schan = 1,
        .nchan = 13,
        .max_tx_power = 80,
        .policy = WIFI_COUNTRY_POLICY_MANUAL
    };
    esp_wifi_set_country(&country);

    wifi_load_ap_credentials();
    wifi_config_t ap_config = {
        .ap = {
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .beacon_interval = 100,
            .dtim_period = 1,
            .pairwise_cipher = WIFI_CIPHER_TYPE_CCMP,
            .pmf_cfg = {
                .required = false,
                .capable = false,
            },
        },
    };
    strncpy((char*)ap_config.ap.ssid, s_ap_ssid, sizeof(ap_config.ap.ssid) - 1);
    ap_config.ap.ssid_len = strlen(s_ap_ssid);
    strncpy((char*)ap_config.ap.password, s_ap_pass, sizeof(ap_config.ap.password) - 1);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    if (!s_wifi_power_enabled) {
        ESP_LOGW(TAG, "WiFi power was disabled by operator in NVS. Radio stays OFF on boot.");
        wifi_inited = true;
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_max_tx_power(80);
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    wifi_inited = true;
    webportal_init();
    ESP_LOGI(TAG, "wifi APSTA initialized, SoftAP: %s (pass: %s, 20dBm, PS_NONE)", s_ap_ssid, s_ap_pass);

    /* اتصال خودکار از NVS در صورت فعال بودن رادیو و وجود تنظیمات ذخیره‌شده قبلی */
    wifi_load_saved_table();

    char saved_ssid[33] = {0};
    char saved_pass[65] = {0};
    storage_get_str("wifi_sta_ssid", "", saved_ssid, sizeof(saved_ssid));
    storage_get_str("wifi_sta_pass", "", saved_pass, sizeof(saved_pass));

    char chone_ssid[33] = {0};
    char chone_pass[65] = {0};
    bool has_chone = wifi_get_latest_chone_credentials(chone_ssid, sizeof(chone_ssid), chone_pass, sizeof(chone_pass));

    const esp_partition_t *running_part = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    bool is_ota_trial = (running_part && esp_ota_get_state_partition(running_part, &ota_state) == ESP_OK &&
                         (ota_state == ESP_OTA_IMG_PENDING_VERIFY || ota_state == ESP_OTA_IMG_NEW));
    bool is_post_ota = ui_resume_is_post_ota_boot() || is_ota_trial || (storage_get_u32("ota_upd_ok", 0) == 1);

    if (has_chone) {
        if (saved_ssid[0] == '\0' || is_post_ota) {
            ESP_LOGI(TAG, "Adopting config-test WiFi '%s' (is_post_ota=%d, prev_empty=%d)",
                     chone_ssid, (int)is_post_ota, (int)(saved_ssid[0] == '\0'));
            strncpy(saved_ssid, chone_ssid, sizeof(saved_ssid) - 1);
            saved_ssid[sizeof(saved_ssid) - 1] = '\0';
            strncpy(saved_pass, chone_pass, sizeof(saved_pass) - 1);
            saved_pass[sizeof(saved_pass) - 1] = '\0';
            wifi_save_credentials(saved_ssid, saved_pass);
        }
    } else if (saved_ssid[0] == '\0') {
        wifi_config_t saved_sta_cfg = {0};
        if (esp_wifi_get_config(WIFI_IF_STA, &saved_sta_cfg) == ESP_OK && saved_sta_cfg.sta.ssid[0] != '\0') {
            strncpy(saved_ssid, (char*)saved_sta_cfg.sta.ssid, sizeof(saved_ssid) - 1);
            strncpy(saved_pass, (char*)saved_sta_cfg.sta.password, sizeof(saved_pass) - 1);
            wifi_save_credentials(saved_ssid, saved_pass);
        }
    }

    if (saved_ssid[0] != '\0') {
        ESP_LOGI(TAG, "Found saved STA credentials for SSID: '%s' -> Auto-connecting on boot...", saved_ssid);
        strncpy(s_selected_ssid, saved_ssid, sizeof(s_selected_ssid) - 1);
        s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';

        wifi_sync_softap_channel_to_target(saved_ssid);

        wifi_config_t sta_cfg = {0};
        strncpy((char*)sta_cfg.sta.ssid, saved_ssid, sizeof(sta_cfg.sta.ssid) - 1);
        if (saved_pass[0] != '\0') {
            strncpy((char*)sta_cfg.sta.password, saved_pass, sizeof(sta_cfg.sta.password) - 1);
        }
        sta_cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        sta_cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        sta_cfg.sta.failure_retry_cnt = 2;
        sta_cfg.sta.pmf_cfg.capable = true;
        sta_cfg.sta.pmf_cfg.required = false;
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);

        s_retry_cnt = 0;
        s_user_initiated_connect = false;
        esp_wifi_connect();
    }

    /* راه‌اندازی تسک پس‌زمینه برای اتصال مجدد خودکار */
    static bool s_bg_task_created = false;
    if (!s_bg_task_created) {
        s_bg_task_created = true;
        xTaskCreatePinnedToCore(wifi_bg_reconnect_task, "wifi_bg_rc", 5120, NULL, 2, NULL, 0);
    }
}

static int64_t s_scan_start_us = 0;

void wifi_start_scan(void) {
    if (!wifi_inited) wifi_init_once();
    if (!s_wifi_power_enabled) {
        wifi_set_enabled(true);
    }

    int64_t now_us = esp_timer_get_time();
    if (s_is_scanning) {
        if (now_us - s_scan_start_us < 8000000LL) {
            ESP_LOGW(TAG, "WiFi scan already in progress (%lld ms), ignoring duplicate call.",
                     (long long)((now_us - s_scan_start_us) / 1000LL));
            return;
        }
        ESP_LOGW(TAG, "WiFi scan watchdog timeout -> resetting scanning state");
        s_is_scanning = false;
        esp_wifi_scan_stop();
    }

    s_is_scanning = true;
    s_scan_start_us = now_us;

    /* نمایش حالت در حال جستجو روی لیست */
    if (objects.ui_scan_list) {
        safe_async_call(lvgl_show_scanning_cb, NULL);
    }

    /* پیکربندی استاندارد اسکن سریع کانال‌های ۱ تا ۱۳ بدون قفل رادیویی (~650ms به جای ۳.۲ ثانیه) */
    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 30,
        .scan_time.active.max = 70,
    };
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_scan_start error: %s", esp_err_to_name(err));
        s_is_scanning = false;
        if (objects.scan_wifi_btn)
            lv_obj_clear_state(objects.scan_wifi_btn, LV_STATE_DISABLED);
        extern void boot_wifi_enable_scan_btn(void);
        boot_wifi_enable_scan_btn();
        safe_async_call(lvgl_update_list_cb, NULL);
        if (s_custom_scan_done_cb) {
            safe_async_call(wifi_custom_scan_done_async_cb, NULL);
        }
    }
}

void wifi_scan_stop_now(void) {
    if (s_is_scanning) {
        s_is_scanning = false;
        esp_wifi_scan_stop();
        if (objects.scan_wifi_btn && lv_obj_is_valid(objects.scan_wifi_btn)) {
            lv_obj_clear_state(objects.scan_wifi_btn, LV_STATE_DISABLED);
        }
        extern void boot_wifi_enable_scan_btn(void);
        boot_wifi_enable_scan_btn();
        ESP_LOGI(TAG, "WiFi scan stopped immediately for OTA / network operations");
    }
}

void wifi_trigger_f4_auto_scan(void) {
    if (!s_wifi_power_enabled) {
        wifi_set_enabled(true);
    }
    if (ap_count == 0 && !s_is_scanning) {
        wifi_start_scan();
        return;
    }
    safe_async_call(lvgl_update_list_cb, NULL);
}

/* ---------------- Public: connect (permanent) ---------------- */
void wifi_connect_begin(const char *password) {
    if (!wifi_inited) wifi_init_once();
    if (!s_selected_ssid[0]) {
        const char *prim = wifi_get_primary_saved_ssid();
        if (prim && *prim) {
            strncpy(s_selected_ssid, prim, sizeof(s_selected_ssid) - 1);
            s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';
        } else {
            const char *no_net = (ui_lang_get() == UI_LANG_FA) ? "هیچ شبکه‌ای انتخاب نشده است" : ((ui_lang_get() == UI_LANG_TR) ? "Hicbir ag secilmedi" : "No network selected");
            wifi_status_msgbox_show_error(no_net);
            return;
        }
    }

    /* فعال‌سازی گارد آماده‌سازی پیش از قطع ارتباط قبلی تا رویدادهای معوقه باعث خطای کاذب ۳ تلاش نشوند */
    s_preparing_connect = true;
    s_user_initiated_connect = false;
    s_is_connecting = true;
    s_last_disc_reason = 0;

    /* قطع اسکن احتمالی و قطع ارتباط پیشین در ابتدای کار */
    if (s_is_scanning) {
        esp_wifi_scan_stop();
        s_is_scanning = false;
    }
    esp_wifi_disconnect();

    /* بازیابی پسورد ذخیره‌شده در صورت خالی بودن ورودی */
    char pass_buf[65] = {0};
    const char *act_pass = password;
    if ((!act_pass || !*act_pass) && wifi_get_saved_password(s_selected_ssid, pass_buf, sizeof(pass_buf))) {
        act_pass = pass_buf;
    }

    if (act_pass && *act_pass) {
        wifi_save_credentials(s_selected_ssid, act_pass);
    }
    vTaskDelay(pdMS_TO_TICKS(80));

    wifi_mode_t cur_mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&cur_mode) == ESP_OK && cur_mode != WIFI_MODE_APSTA) {
        esp_wifi_set_mode(WIFI_MODE_APSTA);
    }

    /* همگام‌سازی کانال هات‌اسپات پیش از پیکربندی STA تا تغییر کانال در لحظه هندشیک رخ ندهد */
    wifi_sync_softap_channel_to_target(s_selected_ssid);

    wifi_config_t cfg = {0};
    strncpy((char*)cfg.sta.ssid, s_selected_ssid, sizeof(cfg.sta.ssid) - 1);
    if (act_pass && *act_pass) {
        strncpy((char*)cfg.sta.password, act_pass, sizeof(cfg.sta.password) - 1);
    }
    cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    cfg.sta.failure_retry_cnt = 2;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;

    esp_err_t cfg_err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if (cfg_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(cfg_err));
    }

    char clean_ssid[48] = {0};
    wifi_sanitize_ssid_for_display(s_selected_ssid, clean_ssid, sizeof(clean_ssid));
    char connect_msg[128];
    if (ui_lang_get() == UI_LANG_FA) {
        snprintf(connect_msg, sizeof(connect_msg), "در حال اتصال به:\n%s...", clean_ssid);
    } else if (ui_lang_get() == UI_LANG_TR) {
        snprintf(connect_msg, sizeof(connect_msg), "Baglaniyor:\n%s...", clean_ssid);
    } else {
        snprintf(connect_msg, sizeof(connect_msg), "Connecting to:\n%s...", clean_ssid);
    }
    wifi_status_msgbox_show(connect_msg);

    s_retry_cnt = 0;
    s_user_initiated_connect = true;
    s_preparing_connect = false;

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        char msg[96];
        if (ui_lang_get() == UI_LANG_FA) {
            snprintf(msg, sizeof(msg), "خطا در آغاز اتصال: %s", esp_err_to_name(err));
        } else if (ui_lang_get() == UI_LANG_TR) {
            snprintf(msg, sizeof(msg), "Baglanti baslatma hatasi: %s", esp_err_to_name(err));
        } else {
            snprintf(msg, sizeof(msg), "Connect start error: %s", esp_err_to_name(err));
        }
        wifi_status_msgbox_show_error(msg);
    }
}

/* ---------------- Public: programmatic connect from Web ---------------- */
void wifi_connect_to_ap(const char *ssid, const char *password) {
    if (!ssid || !*ssid) return;
    if (!wifi_inited) wifi_init_once();

    s_preparing_connect = true;
    s_user_initiated_connect = false;
    s_is_connecting = true;
    s_last_disc_reason = 0;

    strncpy(s_selected_ssid, ssid, sizeof(s_selected_ssid) - 1);
    s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';

    if (s_is_scanning) {
        esp_wifi_scan_stop();
        s_is_scanning = false;
    }
    esp_wifi_disconnect();

    char saved_pass_buf[65] = {0};
    const char *pwd = password;
    if ((!pwd || !*pwd) && wifi_get_saved_password(ssid, saved_pass_buf, sizeof(saved_pass_buf))) {
        pwd = saved_pass_buf;
    }

    if (pwd && *pwd) {
        wifi_save_credentials(ssid, pwd);
    }
    vTaskDelay(pdMS_TO_TICKS(80));

    wifi_mode_t cur_mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&cur_mode) == ESP_OK && cur_mode != WIFI_MODE_APSTA) {
        esp_wifi_set_mode(WIFI_MODE_APSTA);
    }

    wifi_sync_softap_channel_to_target(ssid);

    wifi_config_t cfg = {0};
    strncpy((char*)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid) - 1);
    if (pwd && *pwd) {
        strncpy((char*)cfg.sta.password, pwd, sizeof(cfg.sta.password) - 1);
    }
    cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    cfg.sta.failure_retry_cnt = 2;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;

    esp_wifi_set_config(WIFI_IF_STA, &cfg);

    s_retry_cnt = 0;
    s_user_initiated_connect = true;
    s_preparing_connect = false;
    esp_wifi_connect();
}

uint16_t wifi_get_scan_records(wifi_ap_record_t **out_records) {
    if (out_records) *out_records = ap_records;
    return ap_count;
}

uint16_t wifi_get_scan_records_copy(wifi_ap_record_t *out_records, uint16_t max_count) {
    if (!out_records || max_count == 0 || ap_count == 0 || !ap_records) return 0;
    uint16_t cnt = (ap_count < max_count) ? ap_count : max_count;
    memcpy(out_records, ap_records, cnt * sizeof(wifi_ap_record_t));
    return cnt;
}

/* ---------------- Public: disconnect + rescan (for Scan button) ---------------- */
void wifi_disconnect_and_scan(void) {
    s_preparing_connect = true;
    s_user_initiated_connect = false;
    if (wifi_inited) {
        esp_wifi_disconnect();
    }
    s_preparing_connect = false;
    s_retry_cnt = 0;

    s_selected_ssid[0] = '\0';
    s_connected_ssid[0] = '\0';

    wifi_key_act_hide();       // hide keyboard & textarea
    wifi_start_scan();         // start fresh scan
}

/* ---------------- List item click (select SSID only on steady tap, never on scroll) ---------------- */
void wifi_ap_selected_cb(lv_event_t * e) {
    lv_indev_t *indev = lv_indev_get_act();
    if (indev) {
        lv_point_t pt = {0, 0};
        lv_indev_get_point(indev, &pt);
        lv_coord_t dx = LV_ABS(pt.x - s_wifi_touch_start_x);
        lv_coord_t dy = LV_ABS(pt.y - s_wifi_touch_start_y);
        if ((dx + dy) > s_wifi_touch_max_move) {
            s_wifi_touch_max_move = dx + dy;
        }
        if (dy > 7) {
            ESP_LOGD(TAG, "Ignored WiFi item tap due to vertical drag (dy=%d)", (int)dy);
            return;
        }
    }

    if (objects.ui_scan_list && lv_obj_is_valid(objects.ui_scan_list)) {
        lv_coord_t d_scr = LV_ABS(lv_obj_get_scroll_y(objects.ui_scan_list) - s_wifi_touch_start_scroll_y);
        if (d_scr > 3 || lv_obj_is_scrolling(objects.ui_scan_list)) {
            ESP_LOGD(TAG, "Ignored WiFi item tap during list scroll (d_scr=%d)", (int)d_scr);
            return;
        }
    }

    if (s_wifi_touch_max_move > 10 || (esp_timer_get_time() - s_wifi_last_scroll_us) < 250000LL) {
        ESP_LOGD(TAG, "Ignored WiFi item tap: hand moving/scrolling (max_move=%d)", (int)s_wifi_touch_max_move);
        return;
    }

    lv_obj_t *btn = lv_event_get_target(e);
    const char *ssid_copy = (const char*)lv_obj_get_user_data(btn);
    if (!ssid_copy) return;

    strncpy(s_selected_ssid, ssid_copy, sizeof(s_selected_ssid) - 1);
    s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';
    ESP_LOGI(TAG, "Selected SSID: %s", s_selected_ssid);

    if (objects.wifi_pass_title_lbl && lv_obj_is_valid(objects.wifi_pass_title_lbl)) {
        char clean_ssid[48] = {0};
        wifi_sanitize_ssid_for_display(s_selected_ssid, clean_ssid, sizeof(clean_ssid));
        char title_buf[96];
        snprintf(title_buf, sizeof(title_buf), "%s %s", ui_lang_str(STR_WIFI_PASS_TITLE), clean_ssid);
        lv_label_set_text(objects.wifi_pass_title_lbl, title_buf);
    }

    wifi_key_act_show(); // show keyboard & textarea for password input
}

/* ---------------- WiFi Radio Power Enable/Disable ---------------- */
bool wifi_is_enabled(void)
{
    return s_wifi_power_enabled;
}

void wifi_set_enabled(bool enable)
{
    storage_set_bool("wifi_en", enable);
    if (enable == s_wifi_power_enabled) return;
    s_wifi_power_enabled = enable;

    if (enable) {
        if (!wifi_inited) {
            wifi_init_once();
        } else {
            wifi_load_ap_credentials();
            esp_wifi_set_storage(WIFI_STORAGE_RAM);
            esp_wifi_set_mode(WIFI_MODE_APSTA);
            wifi_config_t ap_config = {0};
            strncpy((char*)ap_config.ap.ssid, s_ap_ssid, sizeof(ap_config.ap.ssid) - 1);
            ap_config.ap.ssid_len = strlen(s_ap_ssid);
            ap_config.ap.channel = 1;
            strncpy((char*)ap_config.ap.password, s_ap_pass, sizeof(ap_config.ap.password) - 1);
            ap_config.ap.max_connection = 4;
            ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
            ap_config.ap.ssid_hidden = 0;
            ap_config.ap.beacon_interval = 100;
            ap_config.ap.dtim_period = 1;
            ap_config.ap.pairwise_cipher = WIFI_CIPHER_TYPE_CCMP;
            ap_config.ap.pmf_cfg.required = false;
            ap_config.ap.pmf_cfg.capable = false;
            esp_wifi_set_config(WIFI_IF_AP, &ap_config);
            esp_wifi_start();
            esp_wifi_set_ps(WIFI_PS_NONE);
            esp_wifi_set_max_tx_power(80);
            esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
            esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
            webportal_init();
        }
        ESP_LOGI(TAG, "WiFi radio ENABLED (AP+STA active, Web Portal started)");

        /* در صورت وجود شبکه ذخیره‌شده قبلی، تلاش برای اتصال مجدد در پس‌زمینه */
        char saved_ssid[33] = {0};
        char saved_pass[65] = {0};
        if (storage_get_str("wifi_sta_ssid", "", saved_ssid, sizeof(saved_ssid)) && saved_ssid[0] != '\0') {
            storage_get_str("wifi_sta_pass", "", saved_pass, sizeof(saved_pass));
            strncpy(s_selected_ssid, saved_ssid, sizeof(s_selected_ssid) - 1);
            s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';
            wifi_sync_softap_channel_to_target(saved_ssid);
            wifi_config_t sta_cfg = {0};
            strncpy((char*)sta_cfg.sta.ssid, saved_ssid, sizeof(sta_cfg.sta.ssid) - 1);
            if (saved_pass[0] != '\0') {
                strncpy((char*)sta_cfg.sta.password, saved_pass, sizeof(sta_cfg.sta.password) - 1);
            }
            sta_cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
            sta_cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
            sta_cfg.sta.failure_retry_cnt = 2;
            sta_cfg.sta.pmf_cfg.capable = true;
            sta_cfg.sta.pmf_cfg.required = false;
            esp_wifi_set_storage(WIFI_STORAGE_RAM);
            esp_wifi_set_mode(WIFI_MODE_APSTA);
            esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
            s_retry_cnt = 0;
            s_user_initiated_connect = false;
            esp_wifi_connect();
        }
    } else {
        if (s_sntp_inited) {
            esp_sntp_stop();
            s_sntp_inited = false;
        }
        cloud_client_stop();
        webportal_stop();
        esp_wifi_stop();
        ESP_LOGI(TAG, "WiFi radio DISABLED (AP+STA stopped, Web Portal stopped)");
    }
}

void wifi_toggle_enabled(void)
{
    wifi_set_enabled(!s_wifi_power_enabled);
}

/* ---------------- SNTP Internet Time Synchronization ---------------- */
/* ساختار انتقال نتیجه همگام‌سازی از Core 0 به Core 1 */
typedef struct {
    jalali_time_t jt_now;
    bool success;
    int64_t drift_s;
    bool day_lock_recovered;
} sntp_sync_result_t;

static sntp_sync_result_t s_sntp_sync_res;

static void sntp_ui_apply_cb(void *arg)
{
    /* این callback روی Core 1 (LVGL task) اجرا می‌شود — فقط UI update، بدون I2C */
    sntp_sync_result_t *res = (sntp_sync_result_t *)arg;
    if (!res) return;

    if (res->success) {
        jt = res->jt_now;
        ds_year  = jt.jy;
        ds_month = jt.jm;
        ds_day   = jt.jd;
        ds_hour  = jt.hour;
        ds_minute = jt.minute;

        if (objects.time_lbl) {
            lv_obj_set_style_base_dir(objects.time_lbl, LV_BASE_DIR_LTR, 0);
            lv_label_set_text_fmt(objects.time_lbl, "\xE2\x80\xAD%02d:%02d\xE2\x80\xAC", jt.hour, jt.minute);
            lv_obj_align(objects.time_lbl, LV_ALIGN_LEFT_MID, 0, 0);
        }
        if (objects.date_lbl) {
            lv_obj_set_style_base_dir(objects.date_lbl, LV_BASE_DIR_LTR, 0);
            if (ui_lang_get() == UI_LANG_FA) {
                static const char *const WDAY_NAME[7] = {
                    "یک شنبه", "دو شنبه", "سه شنبه", "چهار شنبه", "پنج شنبه", "جمعه", "شنبه"};
                int wday_idx = (jt.wday >= 1 && jt.wday <= 7) ? (jt.wday - 1) : 0;
                lv_label_set_text_fmt(objects.date_lbl, "%04d/%02d/%02d %s",
                                      jt.jy, jt.jm, jt.jd, WDAY_NAME[wday_idx]);
            } else {
                int gy = 0, gm = 0, gd = 0;
                jalali_to_gregorian((jt.jy > 0) ? jt.jy : 1404,
                                    (jt.jm > 0) ? jt.jm : 1,
                                    (jt.jd > 0) ? jt.jd : 1,
                                    &gy, &gm, &gd);
                static const char *const WDAY_NAME_EN[7] = {
                    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
                static const char *const WDAY_NAME_TR[7] = {
                    "Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};
                int wday_idx = (jt.wday >= 1 && jt.wday <= 7) ? (jt.wday - 1) : 0;
                const char *wname = (ui_lang_get() == UI_LANG_TR) ? WDAY_NAME_TR[wday_idx] : WDAY_NAME_EN[wday_idx];
                lv_label_set_text_fmt(objects.date_lbl, "%04d/%02d/%02d %s", gy, gm, gd, wname);
            }
        }
        settings_refresh_labels();
        panel_nav_toast_success("ساعت با اینترنت همگام شد");
        if (res->day_lock_recovered) {
            panel_nav_toast_success("ساعت تنظیم شد و قفل امنیتی رفع گردید");
        }
        day_lock_ui_notify_time_sync(res->success);
    } else {
        day_lock_ui_notify_time_sync(false);
    }
}

static void sntp_rtc_sync_task(void *arg)
{
    /* این task روی Core 0 اجرا می‌شود — همه I2C اینجاست، هیچ LVGL نیست */
    time_t now = time(NULL);
    struct tm timeinfo = {0};
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year >= (2025 - 1900)) {
        time_t rtc_epoch = ds3231_get_epoch();
        int64_t drift_s = (int64_t)now - (int64_t)rtc_epoch;
        if (drift_s < 0) drift_s = -drift_s;

        if (rtc_epoch == 0 || drift_s >= 60) {
            uint8_t wday = (uint8_t)(timeinfo.tm_wday + 1);
            esp_err_t err = rtc_set((uint16_t)(timeinfo.tm_year + 1900),
                                    (uint8_t)(timeinfo.tm_mon + 1),
                                    (uint8_t)timeinfo.tm_mday,
                                    (uint8_t)timeinfo.tm_hour,
                                    (uint8_t)timeinfo.tm_min,
                                    (uint8_t)timeinfo.tm_sec,
                                    wday);
            if (err == ESP_OK) {
                memset(&s_sntp_sync_res, 0, sizeof(s_sntp_sync_res));
                s_sntp_sync_res.success  = (ds3231_get_time_jalali(&s_sntp_sync_res.jt_now) == ESP_OK);
                s_sntp_sync_res.drift_s  = drift_s;
                extern void day_lock_notify_time_adjusted(uint32_t new_epoch);
                day_lock_notify_time_adjusted(0);
                s_sntp_sync_res.day_lock_recovered = day_lock_check_rtc_recovery();
                ESP_LOGI(TAG, "RTC DS3231 synced (drift %lld s): %04d/%02d/%02d %02d:%02d:%02d",
                         (long long)drift_s,
                         s_sntp_sync_res.jt_now.jy, s_sntp_sync_res.jt_now.jm, s_sntp_sync_res.jt_now.jd,
                         s_sntp_sync_res.jt_now.hour, s_sntp_sync_res.jt_now.minute, s_sntp_sync_res.jt_now.second);
                safe_async_call(sntp_ui_apply_cb, &s_sntp_sync_res);
            } else {
                ESP_LOGE(TAG, "Failed to write synced time to DS3231: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGI(TAG, "RTC already accurate (drift %lld s < 60s); notifying success.", (long long)drift_s);
            memset(&s_sntp_sync_res, 0, sizeof(s_sntp_sync_res));
            s_sntp_sync_res.success  = (ds3231_get_time_jalali(&s_sntp_sync_res.jt_now) == ESP_OK);
            s_sntp_sync_res.drift_s  = drift_s;
            s_sntp_sync_res.day_lock_recovered = day_lock_check_rtc_recovery();
            safe_async_call(sntp_ui_apply_cb, &s_sntp_sync_res);
        }
    }
    vTaskDelete(NULL);
}

static void time_sync_notification_cb(struct timeval *tv) {
    (void)tv;
    s_sntp_synced = true;
    ESP_LOGI(TAG, "SNTP packet received! Dispatching RTC & UI update on Core 0...");
    xTaskCreatePinnedToCore(sntp_rtc_sync_task, "sntp_rtc", 4096, NULL, 3, NULL, 0);
}

void wifi_sntp_sync_start(void) {
    setenv("TZ", "IRST-3:30", 1);
    tzset();

    if (!s_sntp_inited) {
        ESP_LOGI(TAG, "Starting SNTP service (pool.ntp.org, time.google.com, time.windows.com)...");
        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_setservername(1, "time.google.com");
        esp_sntp_setservername(2, "time.windows.com");
        esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
        esp_sntp_init();
        s_sntp_inited = true;
    } else {
        ESP_LOGI(TAG, "Restarting SNTP sync on active WiFi connection...");
        esp_sntp_restart();
    }
}

bool wifi_sntp_is_synced(void) {
    return s_sntp_synced;
}

