#include "day_lock_storage.h"
#include <Arduino.h>
#include <Preferences.h>
#include <esp_rom_crc.h>
#include <string.h>

static Preferences s_prefs;
static const char *NVS_NAMESPACE = "haffler_lock";
static const char *KEY_SLOT_A    = "meta_a";
static const char *KEY_SLOT_B    = "meta_b";

uint32_t day_lock_storage_calc_crc(const DayLockRecord_t *rec) {
    if (!rec) return 0;
    return esp_rom_crc32_le(0, (const uint8_t*)rec, sizeof(DayLockRecord_t) - sizeof(uint32_t));
}

bool day_lock_storage_load(DayLockRecord_t *out_rec) {
    if (!out_rec) return false;

    s_prefs.begin(NVS_NAMESPACE, false);

    DayLockRecord_t recA;
    DayLockRecord_t recB;
    memset(&recA, 0, sizeof(DayLockRecord_t));
    memset(&recB, 0, sizeof(DayLockRecord_t));

    size_t szA = s_prefs.getBytes(KEY_SLOT_A, &recA, sizeof(DayLockRecord_t));
    size_t szB = s_prefs.getBytes(KEY_SLOT_B, &recB, sizeof(DayLockRecord_t));

    bool validA = (szA == sizeof(DayLockRecord_t)) && 
                  (recA.magic == DAY_LOCK_MAGIC) && 
                  (day_lock_storage_calc_crc(&recA) == recA.crc32);

    bool validB = (szB == sizeof(DayLockRecord_t)) && 
                  (recB.magic == DAY_LOCK_MAGIC) && 
                  (day_lock_storage_calc_crc(&recB) == recB.crc32);

    if (validA && validB) {
        // Both valid; select the one with more recent state or higher sec_accumulator / last_known_epoch
        if (recB.last_known_epoch > recA.last_known_epoch || 
           (recB.last_known_epoch == recA.last_known_epoch && recB.sec_accumulator > recA.sec_accumulator)) {
            *out_rec = recB;
        } else {
            *out_rec = recA;
        }
        s_prefs.end();
        return true;
    } else if (validA) {
        // Slot A valid, heal Slot B
        *out_rec = recA;
        s_prefs.putBytes(KEY_SLOT_B, &recA, sizeof(DayLockRecord_t));
        s_prefs.end();
        return true;
    } else if (validB) {
        // Slot B valid, heal Slot A
        *out_rec = recB;
        s_prefs.putBytes(KEY_SLOT_A, &recB, sizeof(DayLockRecord_t));
        s_prefs.end();
        return true;
    }

    // Check legacy migration fallback (if prior single-key preferences existed)
    if (s_prefs.isKey("remain") || s_prefs.isKey("stage")) {
        memset(out_rec, 0, sizeof(DayLockRecord_t));
        out_rec->magic = DAY_LOCK_MAGIC;
        out_rec->remain_days = (int16_t)s_prefs.getInt("remain", 0);
        out_rec->active_stage = s_prefs.getUChar("stage", 0);
        out_rec->stage_days[0] = (int16_t)s_prefs.getInt("stg0", 0);
        out_rec->stage_days[1] = (int16_t)s_prefs.getInt("stg1", 0);
        out_rec->stage_days[2] = (int16_t)s_prefs.getInt("stg2", 0);
        out_rec->stage_days[3] = (int16_t)s_prefs.getInt("stg3", 0);
        out_rec->stage_days[4] = (int16_t)s_prefs.getInt("stg4", 0);
        out_rec->grace10_used = s_prefs.getBool("grace", false);
        out_rec->sec_accumulator = s_prefs.getUInt("sec_acc", 0);
        out_rec->crc32 = day_lock_storage_calc_crc(out_rec);

        s_prefs.putBytes(KEY_SLOT_A, out_rec, sizeof(DayLockRecord_t));
        s_prefs.putBytes(KEY_SLOT_B, out_rec, sizeof(DayLockRecord_t));
        s_prefs.end();
        return true;
    }

    s_prefs.end();
    return false; // First-time initialization required
}

void day_lock_storage_save(DayLockRecord_t *rec) {
    if (!rec) return;
    rec->magic = DAY_LOCK_MAGIC;
    rec->crc32 = day_lock_storage_calc_crc(rec);

    s_prefs.begin(NVS_NAMESPACE, false);
    // Atomic mirrored update: Slot A first, then Slot B
    s_prefs.putBytes(KEY_SLOT_A, rec, sizeof(DayLockRecord_t));
    s_prefs.putBytes(KEY_SLOT_B, rec, sizeof(DayLockRecord_t));
    s_prefs.end();
}
