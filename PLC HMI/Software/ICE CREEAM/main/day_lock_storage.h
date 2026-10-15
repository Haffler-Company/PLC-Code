#pragma once

#include "day_lock_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief محاسبه چک‌سام استاندارد CRC32
 */
uint32_t day_lock_crc32(const uint8_t *data, size_t len);

/**
 * @brief بارگذاری رکورد روزشمار از اسلات‌های آینه‌ای A یا B در NVS
 * @return true در صورت موفقیت و صحت CRC، در غیر اینصورت false
 */
bool day_lock_storage_load(DayLockRecord_t *out_rec);

/**
 * @brief ذخیره همزمان رکورد در هر دو اسلات آینه‌ای A و B با محاسبه CRC
 */
bool day_lock_storage_save(DayLockRecord_t *rec);

#ifdef __cplusplus
}
#endif
