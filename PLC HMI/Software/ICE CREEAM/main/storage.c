#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

#define STORAGE_NS "app"           // فضای نام NVS (تا 15 کاراکتر)
static nvs_handle_t s_nvs = 0;
static bool s_inited = false;
static SemaphoreHandle_t s_mutex = NULL;

static inline bool lock(void)   {
    if (!s_mutex) return false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGE("storage", "NVS mutex timeout — skipping NVS op to prevent data corruption");
        return false;
    }
    return true;
}
static inline void unlock(void) { if (s_mutex) xSemaphoreGive(s_mutex); }

bool storage_init(void) {
    if (s_inited) return true;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return false;

    err = nvs_open(STORAGE_NS, NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) return false;

    s_mutex = xSemaphoreCreateMutex();
    s_inited = true;
    return true;
}

// ---------- i32 ----------
bool storage_set_i32(const char *key, int32_t v) {
    if (!lock()) return false;
    int32_t curr = 0;
    if (nvs_get_i32(s_nvs, key, &curr) == ESP_OK && curr == v) {
        unlock();
        return true; /* مقدار بدون تغییر: جلوگیری از استهلاک فلش و مسدودی I/O */
    }
    esp_err_t err = nvs_set_i32(s_nvs, key, v);
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    unlock();
    return err == ESP_OK;
}
int32_t storage_get_i32(const char *key, int32_t defval) {
    int32_t v = defval;
    if (!lock()) return defval;
    nvs_get_i32(s_nvs, key, &v); // اگر نبود، v دست‌نخورده می‌مونه
    unlock();
    return v;
}

// ---------- u32 ----------
bool storage_set_u32(const char *key, uint32_t v) {
    if (!lock()) return false;
    uint32_t curr = 0;
    if (nvs_get_u32(s_nvs, key, &curr) == ESP_OK && curr == v) {
        unlock();
        return true;
    }
    esp_err_t err = nvs_set_u32(s_nvs, key, v);
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    unlock();
    return err == ESP_OK;
}
uint32_t storage_get_u32(const char *key, uint32_t defval) {
    uint32_t v = defval;
    if (!lock()) return defval;
    nvs_get_u32(s_nvs, key, &v);
    unlock();
    return v;
}

// ---------- bool ----------
bool storage_set_bool(const char *key, bool v) {
    return storage_set_u32(key, v ? 1u : 0u);
}
bool storage_get_bool(const char *key, bool defval) {
    return storage_get_u32(key, defval ? 1u : 0u) ? true : false;
}

// ---------- string ----------
bool storage_set_str(const char *key, const char *str) {
    const char *new_str = str ? str : "";
    if (!lock()) return false;
    size_t req = 0;
    if (nvs_get_str(s_nvs, key, NULL, &req) == ESP_OK && req == (strlen(new_str) + 1)) {
        char *buf = (char *)malloc(req);
        if (buf) {
            if (nvs_get_str(s_nvs, key, buf, &req) == ESP_OK && strcmp(buf, new_str) == 0) {
                free(buf);
                unlock();
                return true;
            }
            free(buf);
        }
    }
    esp_err_t err = nvs_set_str(s_nvs, key, new_str);
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    unlock();
    return err == ESP_OK;
}
bool storage_get_str(const char *key, const char *defval, char *out, size_t out_size) {
    if (!out || out_size == 0) return false;

    size_t req = 0;
    if (!lock()) {
        const char *d = defval ? defval : "";
        strlcpy(out, d, out_size);
        return false;
    }
    esp_err_t err = nvs_get_str(s_nvs, key, NULL, &req);
    if (err == ESP_OK && req <= out_size) {
        err = nvs_get_str(s_nvs, key, out, &req);
        unlock();
        return err == ESP_OK;
    } else {
        // key نبود یا بافر کوچیک بود: defval بریز
        unlock();
        const char *d = defval ? defval : "";
        strlcpy(out, d, out_size);
        return false;
    }
}

// ---------- blob ----------
bool storage_set_blob(const char *key, const void *data, size_t len) {
    if (!lock()) return false;
    esp_err_t err = nvs_set_blob(s_nvs, key, data, len);
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    unlock();
    return err == ESP_OK;
}
int storage_get_blob(const char *key, void *out, size_t out_size) {
    size_t req = 0;
    if (!lock()) return -1;
    esp_err_t err = nvs_get_blob(s_nvs, key, NULL, &req);
    if (err == ESP_OK) {
        if (out == NULL || req > out_size) {
            unlock();
            return -2; // بافر کوچک یا out=NULL
        }
        err = nvs_get_blob(s_nvs, key, out, &req);
        unlock();
        return (err == ESP_OK) ? (int)req : -1;
    }
    unlock();
    return -1; // پیدا نشد
}

// ---------- erase ----------
bool storage_erase_key(const char *key) {
    if (!lock()) return false;
    esp_err_t err = nvs_erase_key(s_nvs, key);
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    unlock();
    return err == ESP_OK;
}
bool storage_erase_all(void) {
    if (!lock()) return false;
    esp_err_t err = nvs_erase_all(s_nvs);
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    unlock();
    return err == ESP_OK;
}
