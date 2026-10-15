#include "day_lock_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "day_lock_storage";
static const char *NVS_NAMESPACE = "haf_lock";
static const char *KEY_SLOT_A    = "meta_a";
static const char *KEY_SLOT_B    = "meta_b";

/* محاسبه استاندارد CRC32 (IEEE 802.3) بدون وابستگی */
uint32_t day_lock_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    if (!data) return 0;

    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint32_t)data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

static uint32_t calc_record_crc(const DayLockRecord_t *rec)
{
    if (!rec) return 0;
    return day_lock_crc32((const uint8_t *)rec, sizeof(DayLockRecord_t) - sizeof(uint32_t));
}

static void migrate_legacy_record(const DayLockLegacyRecord_t *leg, DayLockRecord_t *out)
{
    memset(out, 0, sizeof(DayLockRecord_t));
    out->magic = leg->magic;
    if (leg->active_stage == 0) {
        out->lock_state = LOCK_STAGE_FACTORY_FREE;
        out->active_stage = 0;
    } else if (leg->active_stage >= 1 && leg->active_stage <= DAY_LOCK_MAX_STAGES) {
        out->active_stage = leg->active_stage;
        if (leg->remain_days <= 0) {
            out->lock_state = LOCK_STAGE_HARD_LOCK;
        } else {
            out->lock_state = LOCK_STAGE_ACTIVE_LEASE;
        }
    } else {
        out->lock_state = leg->active_stage;
        out->active_stage = 1;
    }
    out->remain_days = leg->remain_days;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) {
        out->stage_days[i] = leg->stage_days[i];
    }
    out->grace10_used = leg->grace10_used;
    out->last_known_epoch = leg->last_known_epoch;
    out->sec_accumulator = leg->sec_accumulator;
    out->failed_attempts = leg->failed_attempts;
    out->lockout_until_sec = leg->lockout_until_sec;
    out->boot_counter = leg->boot_counter;
    for (int i = 0; i < 3; i++) {
        out->grace_used[i] = leg->grace_used[i];
    }
}

bool day_lock_storage_load(DayLockRecord_t *out_rec)
{
    if (!out_rec) return false;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open '%s' failed (%s)", NVS_NAMESPACE, esp_err_to_name(err));
        return false;
    }

    DayLockRecord_t recA, recB;
    size_t szA = sizeof(DayLockRecord_t);
    size_t szB = sizeof(DayLockRecord_t);

    esp_err_t errA = nvs_get_blob(h, KEY_SLOT_A, &recA, &szA);
    esp_err_t errB = nvs_get_blob(h, KEY_SLOT_B, &recB, &szB);

    bool validA = (errA == ESP_OK) && (szA == sizeof(DayLockRecord_t)) &&
                  (recA.magic == DAY_LOCK_MAGIC) &&
                  (calc_record_crc(&recA) == recA.crc32);

    bool validB = (errB == ESP_OK) && (szB == sizeof(DayLockRecord_t)) &&
                  (recB.magic == DAY_LOCK_MAGIC) &&
                  (calc_record_crc(&recB) == recB.crc32);

    if (validA) {
        *out_rec = recA;
        if (!validB) {
            ESP_LOGW(TAG, "Slot B corrupted or missing! Restoring from Slot A");
            (void)nvs_set_blob(h, KEY_SLOT_B, &recA, sizeof(DayLockRecord_t));
            (void)nvs_commit(h);
        }
        nvs_close(h);
        return true;
    } else if (validB) {
        ESP_LOGW(TAG, "Slot A corrupted or missing! Restoring from Slot B");
        *out_rec = recB;
        (void)nvs_set_blob(h, KEY_SLOT_A, &recB, sizeof(DayLockRecord_t));
        (void)nvs_commit(h);
        nvs_close(h);
        return true;
    }

    /* بررسی مهاجرت از ساختار نسخه پیشین NVS */
    DayLockLegacyRecord_t legA, legB;
    size_t leg_szA = sizeof(DayLockLegacyRecord_t);
    size_t leg_szB = sizeof(DayLockLegacyRecord_t);
    esp_err_t lerrA = nvs_get_blob(h, KEY_SLOT_A, &legA, &leg_szA);
    esp_err_t lerrB = nvs_get_blob(h, KEY_SLOT_B, &legB, &leg_szB);

    bool lvalidA = (lerrA == ESP_OK) && (leg_szA == sizeof(DayLockLegacyRecord_t)) &&
                   (legA.magic == DAY_LOCK_MAGIC) &&
                   (day_lock_crc32((const uint8_t *)&legA, sizeof(DayLockLegacyRecord_t) - sizeof(uint32_t)) == legA.crc32);
    bool lvalidB = (lerrB == ESP_OK) && (leg_szB == sizeof(DayLockLegacyRecord_t)) &&
                   (legB.magic == DAY_LOCK_MAGIC) &&
                   (day_lock_crc32((const uint8_t *)&legB, sizeof(DayLockLegacyRecord_t) - sizeof(uint32_t)) == legB.crc32);

    if (lvalidA || lvalidB) {
        migrate_legacy_record(lvalidA ? &legA : &legB, out_rec);
        out_rec->magic = DAY_LOCK_MAGIC;
        out_rec->crc32 = calc_record_crc(out_rec);
        (void)nvs_set_blob(h, KEY_SLOT_A, out_rec, sizeof(DayLockRecord_t));
        (void)nvs_set_blob(h, KEY_SLOT_B, out_rec, sizeof(DayLockRecord_t));
        (void)nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "Legacy day-lock record successfully migrated to 5-stage architecture");
        return true;
    }

    nvs_close(h);
    ESP_LOGI(TAG, "No valid day-lock record found (first boot or wiped)");
    return false;
}

bool day_lock_storage_save(DayLockRecord_t *rec)
{
    if (!rec) return false;

    rec->magic = DAY_LOCK_MAGIC;
    rec->crc32 = calc_record_crc(rec);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open write failed (%s)", esp_err_to_name(err));
        return false;
    }

    esp_err_t eA = nvs_set_blob(h, KEY_SLOT_A, rec, sizeof(DayLockRecord_t));
    if (eA == ESP_OK) {
        (void)nvs_commit(h);
    }
    esp_err_t eB = nvs_set_blob(h, KEY_SLOT_B, rec, sizeof(DayLockRecord_t));
    if (eB == ESP_OK) {
        (void)nvs_commit(h);
    }

    nvs_close(h);
    return (eA == ESP_OK && eB == ESP_OK);
}
