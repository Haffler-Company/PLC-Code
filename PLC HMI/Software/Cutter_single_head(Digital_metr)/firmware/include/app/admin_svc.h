#pragma once
#include "jalali.h"
#include <Wire.h>
#include <sys/time.h>
#include <time.h>

#define MASTER_UNLOCK_PIN "12773"
#define DEFAULT_ADMIN_PIN "123456"
#define DS3231_I2C_ADDR 0x68

extern String admin_pin;
extern uint32_t user_cuts;
extern int day_lock_remain;
extern int day_lock_stage_days[5];
extern uint8_t day_lock_active_stage;
extern volatile bool device_locked;
extern int64_t soft_epoch_base;
extern uint32_t soft_millis_base;
extern int64_t day_lock_last_midnight;
extern bool soft_clock_valid;
extern volatile bool flag_start_update_flow;
extern TwoWire touchWire;
extern SemaphoreHandle_t touchWireMutex;

static inline int64_t soft_now_epoch() {
  if (!soft_clock_valid || soft_epoch_base <= 0) return 0;
  return soft_epoch_base + (int64_t)((millis() - soft_millis_base) / 1000UL);
}

static inline void soft_clock_set_epoch(int64_t epoch) {
  soft_epoch_base = epoch;
  soft_millis_base = millis();
  soft_clock_valid = (epoch > 0);
}

static inline void soft_clock_set_civil(int y, int m, int d, int hh, int mm, int ss) {
  soft_clock_set_epoch(civil_to_epoch(y, m, d, hh, mm, ss));
}

// DS3231 روی همان I2C تاچ (SDA8/SCL9). اگر نباشد false برمی‌گردد.
static inline uint8_t ds3231_bcd2bin(uint8_t v) { return (uint8_t)(((v >> 4) * 10) + (v & 0x0f)); }
static inline uint8_t ds3231_bin2bcd(uint8_t v) { return (uint8_t)(((v / 10U) << 4) | (v % 10U)); }

static inline bool ds3231_read_time(int *y, int *m, int *d, int *hh, int *mi, int *ss) {
  if (!y || !m || !d || !hh || !mi || !ss) return false;
  bool ok = false;
  if (touchWireMutex) {
    if (xSemaphoreTakeRecursive(touchWireMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  }
  touchWire.beginTransmission(DS3231_I2C_ADDR);
  touchWire.write((uint8_t)0x00);
  if (touchWire.endTransmission(true) == 0 && touchWire.requestFrom((uint8_t)DS3231_I2C_ADDR, (uint8_t)7) >= 7) {
    *ss = ds3231_bcd2bin(touchWire.read() & 0x7f);
    *mi = ds3231_bcd2bin(touchWire.read() & 0x7f);
    *hh = ds3231_bcd2bin(touchWire.read() & 0x3f);
    (void)touchWire.read();  // day of week
    *d = ds3231_bcd2bin(touchWire.read() & 0x3f);
    *m = ds3231_bcd2bin(touchWire.read() & 0x1f);
    *y = 2000 + ds3231_bcd2bin(touchWire.read());
    ok = (*y >= 2000 && *m >= 1 && *m <= 12 && *d >= 1 && *d <= 31);
  }
  if (touchWireMutex) xSemaphoreGiveRecursive(touchWireMutex);
  return ok;
}

// [UI/Task2] Called only by the explicit F4 header Save path.
static inline bool ds3231_write_time(int y, int m, int d, int hh, int mi, int ss) {
  if (y < 2000 || y > 2099 || m < 1 || m > 12 || d < 1 || d > 31 ||
      hh < 0 || hh > 23 || mi < 0 || mi > 59 || ss < 0 || ss > 59) return false;
  const int64_t days = civil_to_epoch(y, m, d, 0, 0, 0) / 86400LL;
  const uint8_t weekday = (uint8_t)(((days + 4LL) % 7LL) + 1LL);  // DS3231: Sunday = 1
  bool ok = false;
  if (touchWireMutex) {
    if (xSemaphoreTakeRecursive(touchWireMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  }
  touchWire.beginTransmission(DS3231_I2C_ADDR);
  touchWire.write((uint8_t)0x00);
  touchWire.write(ds3231_bin2bcd((uint8_t)ss));
  touchWire.write(ds3231_bin2bcd((uint8_t)mi));
  touchWire.write(ds3231_bin2bcd((uint8_t)hh));
  touchWire.write(ds3231_bin2bcd(weekday));
  touchWire.write(ds3231_bin2bcd((uint8_t)d));
  touchWire.write(ds3231_bin2bcd((uint8_t)m));
  touchWire.write(ds3231_bin2bcd((uint8_t)(y - 2000)));
  ok = (touchWire.endTransmission(true) == 0);
  if (touchWireMutex) xSemaphoreGiveRecursive(touchWireMutex);
  return ok;
}

static inline bool ds3231_read_temp_c(float *out_c) {
  if (!out_c) return false;
  bool ok = false;
  if (touchWireMutex) {
    if (xSemaphoreTakeRecursive(touchWireMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  }
  touchWire.beginTransmission(DS3231_I2C_ADDR);
  touchWire.write((uint8_t)0x11);
  if (touchWire.endTransmission(true) == 0 && touchWire.requestFrom((uint8_t)DS3231_I2C_ADDR, (uint8_t)2) >= 2) {
    int8_t msb = (int8_t)touchWire.read();
    uint8_t lsb = touchWire.read();
    *out_c = (float)msb + (float)(lsb >> 6) * 0.25f;
    ok = true;
  }
  if (touchWireMutex) xSemaphoreGiveRecursive(touchWireMutex);
  return ok;
}

static inline bool admin_password_ok(const char *p) {
  if (!p || !p[0]) return false;
  // فقط رقم‌ها (در صورت فاصله/نویز لمس)
  char dig[16];
  size_t n = 0;
  for (const char *s = p; *s && n + 1 < sizeof(dig); s++) {
    if (*s >= '0' && *s <= '9') dig[n++] = *s;
  }
  dig[n] = 0;
  if (!dig[0]) return false;
  if (strcmp(dig, MASTER_UNLOCK_PIN) == 0) return true;
  if (strcmp(dig, "12773") == 0) return true;
  if (strcmp(dig, "12733") == 0) return true;
  if (admin_pin.length() && strcmp(dig, admin_pin.c_str()) == 0) return true;
  if (strcmp(dig, DEFAULT_ADMIN_PIN) == 0) return true;
  return false;
}

static inline void day_lock_clear_after_unlock();

static inline void day_lock_arm(int days) {
  days = constrain(days, 0, 365);
  day_lock_remain = days;
  device_locked = false;
  int64_t now = soft_now_epoch();
  if (now <= 0) day_lock_last_midnight = 0;
  else day_lock_last_midnight = midnight_of(now);
}

static inline void day_lock_start_chain(int days) {
  days = constrain(days, 0, 365);
  if (days <= 0) { day_lock_clear_after_unlock(); return; }
  day_lock_stage_days[0] = days;
  for (uint8_t i = 1; i < 5; ++i) day_lock_stage_days[i] = constrain(day_lock_stage_days[i], 0, 50);
  day_lock_active_stage = days > 0 ? 1 : 0;
  day_lock_arm(days);
}

static inline uint8_t day_lock_digit_sum(uint64_t n) {
  uint8_t sum = 0;
  do { sum = (uint8_t)(sum + (n % 10ULL)); n /= 10ULL; } while (n);
  return sum;
}

// رمز هر مرحله / چالش: عدد حاصل و سپس رقم یکانِ جمع کل ارقام حاصل.
static inline void day_lock_make_derived_password(char *out, size_t out_sz, uint64_t base) {
  if (!out || out_sz == 0) return;
  snprintf(out, out_sz, "%llu%u", (unsigned long long)base, (unsigned)(day_lock_digit_sum(base) % 10U));
}

// رمز مرحله — فرمول مالک:
// تایمر ۱ ضربدر ۲، تایمر ۲ ضربدر ۳، تایمر ۳ ضربدر ۴، تایمر ۴ ضربدر ۵، تایمر ۵ ضربدر ۶.
// هر تک‌عددِ رمز مدیر جدا در ضریب ضرب و فقط یکانِ هر ضرب نوشته می‌شود؛
// در پایان یکانِ جمع کل ارقامِ حاصل به انتهای رشته اضافه می‌گردد.
// نمونه ۱: مدیر 111111، تایمر ۱ (ضریب ۲) → 222222؛ جمع ارقام ۱۲ → رمز: 2222222
// نمونه ۲: مدیر 111111، تایمر ۲ (ضریب ۳) → 333333؛ جمع ارقام ۱۸ → رمز: 3333338
// نمونه ۳: مدیر 123456، تایمر ۱ (ضریب ۲) → 246802؛ جمع ارقام ۲۲ → رمز: 2468022
static inline void day_lock_stage_code_digits(char *out, size_t out_sz, uint8_t stage) {
  if (!out || out_sz == 0) return;
  const char *pin = admin_pin.length() ? admin_pin.c_str() : DEFAULT_ADMIN_PIN;
  size_t n = 0;
  uint16_t dsum = 0;
  const uint8_t mult = (uint8_t)(stage + 1U); // T1 -> x2, T2 -> x3, T3 -> x4, T4 -> x5, T5 -> x6
  for (const char *s = pin; *s && n + 2 < out_sz; ++s) {
    if (*s < '0' || *s > '9') continue;
    const uint8_t d = (uint8_t)(((*s - '0') * mult) % 10U);
    out[n++] = (char)('0' + d);
    dsum = (uint16_t)(dsum + d);
  }
  if (n == 0) { out[0] = 0; return; }
  out[n] = (char)('0' + (dsum % 10U));
  out[n + 1] = 0;
}

static inline bool day_lock_stage_password_ok(const char *raw) {
  if (!raw || day_lock_active_stage < 1 || day_lock_active_stage > 5) return false;
  char expected[16];
  day_lock_stage_code_digits(expected, sizeof(expected), day_lock_active_stage);
  return strcmp(raw, expected) == 0;
}

// رمز یک‌بارمصرفِ چالشِ روی صفحهٔ قفل: (چالش تصادفی × ۲) + رقم یکانِ جمع ارقام حاصل.
static inline bool day_lock_one_time_password_ok(const char *raw, uint32_t challenge) {
  if (!raw || challenge < 1000 || challenge > 999999UL) return false;
  char expected[24];
  uint64_t base = (uint64_t)challenge * 2ULL;
  day_lock_make_derived_password(expected, sizeof(expected), base);
  return strcmp(raw, expected) == 0;
}

extern bool day_lock_grace10_used;  // رمز ۱۱۲۲۳۳ فقط یک‌بار در عمر دستگاه +۱۰ روز مهلت می‌دهد
extern bool day_lock_t1_from_chain; // زنجیره از T1 مودال مسلح شده؛ کارت روزشمارِ تنظیمات خالی می‌ماند

// ۱۱۲۲۳۳ روی صفحهٔ قفل: از همین امروز، به همهٔ تایمرهای فعال (روزماندهٔ جاری و
// روزهای هر مرحلهٔ تنظیم‌شده) ده روز اضافه می‌شود و قفل همان لحظه باز می‌ماند.
static inline bool day_lock_apply_grace10() {
  if (day_lock_grace10_used) return false;
  day_lock_grace10_used = true;
  for (uint8_t i = 0; i < 5; ++i) {
    if (day_lock_stage_days[i] > 0)
      day_lock_stage_days[i] = constrain(day_lock_stage_days[i] + 10, 0, (i == 0) ? 365 : 50);
  }
  day_lock_remain = 10;
  const int64_t now = soft_now_epoch();
  day_lock_last_midnight = (now > 0) ? midnight_of(now) : 0;
  device_locked = false;
  return true;
}

static inline bool day_lock_advance_stage() {
  for (uint8_t i = day_lock_active_stage; i < 5; ++i) {
    const int days = constrain(day_lock_stage_days[i], 0, 50);
    if (days <= 0) continue;
    day_lock_active_stage = i + 1;
    day_lock_arm(days);
    return true;
  }
  day_lock_clear_after_unlock();
  return false;
}

static inline void day_lock_clear_after_unlock() {
  device_locked = false;
  day_lock_remain = 0;
  day_lock_last_midnight = 0;
  day_lock_active_stage = 0;
  for (uint8_t i = 0; i < 5; ++i) day_lock_stage_days[i] = 0;
}

// همگام‌سازی ساعت از DS3231 در صورت وجود (کبیسه/ماه در civil_to_epoch)
static inline void soft_clock_try_hw_sync() {
  int y, m, d, hh, mi, ss;
  if (ds3231_read_time(&y, &m, &d, &hh, &mi, &ss))
    soft_clock_set_civil(y, m, d, hh, mi, ss);
}

// TLS must be based on a fresh, battery-backed RTC value.  Do not use the
// cached UI/NVS soft clock here: it is not a cryptographic time source.
static inline bool system_clock_sync_from_ds3231() {
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  if (!ds3231_read_time(&y, &m, &d, &hh, &mi, &ss)) return false;
  static const uint8_t month_days[] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
  const bool leap = (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
  if (m < 1 || m > 12 || hh > 23 || mi > 59 || ss > 59) return false;
  const int dim = month_days[m - 1] + ((m == 2 && leap) ? 1 : 0);
  if (d < 1 || d > dim) return false;
  // ISRG Root X1 expires on 2035-06-04.  Stop one day earlier so a local-time
  // boundary can never turn a seemingly valid RTC value into an ambiguous TLS
  // result; a renewed trust anchor must be released before then.
  if (y < 2024 || y > 2035 || (y == 2035 && (m > 6 || (m == 6 && d > 3)))) return false;
  const int64_t epoch = civil_to_epoch(y, m, d, hh, mi, ss);
  if (epoch < 1704067200LL) return false;
  struct timeval tv = {};
  tv.tv_sec = (time_t)epoch;
  if (settimeofday(&tv, nullptr) != 0) return false;
  const time_t check = time(nullptr);
  return check >= (time_t)(epoch - 2) && check <= (time_t)(epoch + 2);
}

// بوت: تطبیق تاریخ با روزماندهٔ ذخیره‌شده و محافظت در برابر دستکاری ساعت و باتری RTC
static inline bool day_lock_boot_reconcile() {
  soft_clock_try_hw_sync();
  if (day_lock_remain <= 0 && day_lock_active_stage == 0) {
    device_locked = false;
    return false;
  }
  int64_t now = soft_now_epoch();
  // اگر تایمر فعال است ولی ساعت سخت‌افزاری نامعتبر است یا باتری برداشته شده
  if (now <= 0) {
    if (day_lock_remain > 0 || day_lock_active_stage > 0) {
      device_locked = true;
      return true;
    }
    return false;
  }

  int64_t mid = midnight_of(now);
  const int stored_rem = day_lock_remain;
  bool need_save = false;

  if (day_lock_last_midnight <= 0) {
    day_lock_last_midnight = mid;
    need_save = true;
  } else {
    int64_t delta_sec = mid - day_lock_last_midnight;
    int64_t days_passed = delta_sec / 86400LL;
    if (days_passed > 0) {
      int new_rem = stored_rem - (int)days_passed;
      if (new_rem < 0) new_rem = 0;
      day_lock_remain = new_rem;
      day_lock_last_midnight = mid;
      if (new_rem != stored_rem) need_save = true;
      if (new_rem <= 0) {
        day_lock_remain = 0;
        device_locked = true;
        need_save = true;
      }
    } else if (days_passed < 0 || (day_lock_last_midnight > 0 && mid < day_lock_last_midnight)) {
      // ساعت به عقب کشیده شده است (دستکاری ساعت): فوراً قفل کن
      device_locked = true;
      need_save = true;
    }
  }

  if (day_lock_remain > 0 && !device_locked) device_locked = false;
  return need_save;
}

// حین روشن بودن: فقط وقتی از نیمه‌شب رد شد در RAM کم کن؛ و بررسی دستکاری ساعت
static inline bool day_lock_tick() {
  if (device_locked) return false;
  if (day_lock_remain <= 0 && day_lock_active_stage == 0) return false;
  int64_t now = soft_now_epoch();
  if (now <= 0) {
    // قطع باتری یا نامعتبر شدن ساعت حین کار
    if (day_lock_remain > 0 || day_lock_active_stage > 0) {
      device_locked = true;
      return true;
    }
    return false;
  }
  int64_t mid = midnight_of(now);
  if (day_lock_last_midnight <= 0) {
    day_lock_last_midnight = mid;
    return false;
  }
  if (mid < day_lock_last_midnight) {
    // دستکاری ساعت به عقب در حین کارکرد
    device_locked = true;
    return true;
  }
  bool changed = false;
  while (mid > day_lock_last_midnight) {
    day_lock_last_midnight += 86400LL;
    if (day_lock_remain > 0) day_lock_remain--;
    changed = true;
    if (day_lock_remain <= 0) {
      day_lock_remain = 0;
      device_locked = true;
      break;
    }
  }
  if (!device_locked && changed) day_lock_last_midnight = mid;
  return changed;
}
