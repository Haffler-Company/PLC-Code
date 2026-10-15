#ifndef BAKERY_DAY_LOCK_STORAGE_H
#define BAKERY_DAY_LOCK_STORAGE_H

#include "day_lock_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Computes CRC32 checksum for a DayLockRecord_t struct.
 */
uint32_t day_lock_storage_calc_crc(const DayLockRecord_t *rec);

/**
 * @brief Loads the active DayLockRecord from mirrored NVS slots (Slot A / Slot B).
 * Automatically recovers and promotes the backup slot if one is corrupted.
 * @param out_rec Pointer to destination record buffer
 * @return true if a valid CRC-verified record was loaded; false on cold uninitialized boot.
 */
bool day_lock_storage_load(DayLockRecord_t *out_rec);

/**
 * @brief Persists the DayLockRecord to both mirrored NVS slots with CRC32.
 * @param rec Pointer to record to save
 */
void day_lock_storage_save(DayLockRecord_t *rec);

#ifdef __cplusplus
}
#endif

#endif /* BAKERY_DAY_LOCK_STORAGE_H */
