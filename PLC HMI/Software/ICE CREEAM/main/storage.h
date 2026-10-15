#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// حتماً در CMake: REQUIRES nvs_flash

#ifdef __cplusplus
extern "C" {
#endif

// باید یک بار در شروع برنامه صدا زده شود (خیلی زود، قبل از استفاده از set/get)
bool storage_init(void);   // true: ok

// اعداد 32 بیتی
bool     storage_set_i32(const char *key, int32_t v);
int32_t  storage_get_i32(const char *key, int32_t defval);

bool     storage_set_u32(const char *key, uint32_t v);
uint32_t storage_get_u32(const char *key, uint32_t defval);

// بولین
bool storage_set_bool(const char *key, bool v);
bool storage_get_bool(const char *key, bool defval);

// رشته (null-terminated)
// set: مستقیماً همون رشته ذخیره می‌شه
bool storage_set_str(const char *key, const char *str);

// get: اگر موجود بود، توی out کپی می‌کنه و true برمی‌گردونه.
// اگر نبود، defval رو توی out می‌نویسه و false برمی‌گردونه.
bool storage_get_str(const char *key, const char *defval, char *out, size_t out_size);

// باینری (blob)
bool storage_set_blob(const char *key, const void *data, size_t len);
// برمی‌گرداند: طول واقعی خوانده‌شده؛ اگر نبود: -1 ؛ اگر بافر کوچک بود: -2
int  storage_get_blob(const char *key, void *out, size_t out_size);

// پاک‌کردن
bool storage_erase_key(const char *key);
bool storage_erase_all(void);   // کل فضای نام "app"

#ifdef __cplusplus
}
#endif
