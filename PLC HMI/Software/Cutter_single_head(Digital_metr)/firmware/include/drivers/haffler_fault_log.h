#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

#define HAFFLER_FAULT_MAX_ENTRIES 10

typedef enum {
  FAULT_NONE = 0,
  FAULT_EMERGENCY_STOP = 1,       // شستی استپ اضطراری
  FAULT_BLADE_OVERTRAVEL = 2,     // خطای زمان حرکت تیغه برش
  FAULT_CLAMP_TIMEOUT = 3,        // خطای جک‌های پنوماتیک کلمپ
  FAULT_ENCODER_DISCONNECT = 4,   // قطعی یا پرش سیگنال خط‌کش دیجیتال
  FAULT_BROWNOUT = 5,             // افت ولتاژ برق ورودی (Brownout Reset)
  FAULT_WATCHDOG_RESET = 6,       // تریپ واچ‌داگ سخت‌افزاری/نرم‌افزاری
  FAULT_CORE_PANIC = 7,           // کرش هسته پردازنده (Panic Reset)
  FAULT_RTC_ERROR = 8,            // خطای ارتباط با ساعت سخت‌افزاری DS3231
  FAULT_TOUCH_ERROR = 9,          // خطای ارتباط با کنترلر تاچ GT911
  FAULT_LEASE_EXPIRED = 10        // پایان اعتبار زمانی قفل روزشمار
} haffler_fault_code_t;

typedef struct {
  uint32_t timestamp; // unix epoch
  uint8_t  code;      // haffler_fault_code_t
  int16_t  value;     // مقدار یا جزئیات پارامتر در لحظه خطا
} __attribute__((packed)) haffler_fault_entry_t;

typedef struct {
  uint8_t count;
  uint8_t head;
  haffler_fault_entry_t entries[HAFFLER_FAULT_MAX_ENTRIES];
} __attribute__((packed)) haffler_fault_storage_t;

static haffler_fault_storage_t s_fault_log;
static bool s_fault_log_inited = false;
static uint32_t s_fault_total_count = 0;

static inline void haffler_fault_log_init(void) {
  if (s_fault_log_inited) return;
  memset(&s_fault_log, 0, sizeof(s_fault_log));

  Preferences p;
  if (p.begin("flt_ns", true)) {
    size_t len = p.getBytes("flt_log", &s_fault_log, sizeof(s_fault_log));
    if (len != sizeof(s_fault_log)) {
      s_fault_log.count = 0;
      s_fault_log.head = 0;
      memset(s_fault_log.entries, 0, sizeof(s_fault_log.entries));
    }
    s_fault_total_count = p.getUInt("flt_tot", s_fault_log.count);
    p.end();
  }
  s_fault_log_inited = true;
  Serial.printf("[FaultLog] Inited. Stored count: %u, Total logged: %u\n",
                (unsigned)s_fault_log.count, (unsigned)s_fault_total_count);
}

static inline void haffler_fault_log_record(haffler_fault_code_t code, int16_t value) {
  if (!s_fault_log_inited) haffler_fault_log_init();
  if (code == FAULT_NONE) return;

  time_t now = time(nullptr);

  // نادیده گرفتن خطای تکراری در فاصله کمتر از ۵ ثانیه
  if (s_fault_log.count > 0) {
    int last_idx = (s_fault_log.head - 1 + HAFFLER_FAULT_MAX_ENTRIES) % HAFFLER_FAULT_MAX_ENTRIES;
    if (s_fault_log.entries[last_idx].code == (uint8_t)code &&
        (now - (time_t)s_fault_log.entries[last_idx].timestamp) < 5) {
      return;
    }
  }

  s_fault_log.entries[s_fault_log.head].timestamp = (uint32_t)now;
  s_fault_log.entries[s_fault_log.head].code = (uint8_t)code;
  s_fault_log.entries[s_fault_log.head].value = value;

  s_fault_log.head = (s_fault_log.head + 1) % HAFFLER_FAULT_MAX_ENTRIES;
  if (s_fault_log.count < HAFFLER_FAULT_MAX_ENTRIES) {
    s_fault_log.count++;
  }
  s_fault_total_count++;

  Preferences p;
  if (p.begin("flt_ns", false)) {
    p.putBytes("flt_log", &s_fault_log, sizeof(s_fault_log));
    p.putUInt("flt_tot", s_fault_total_count);
    p.end();
  }
  Serial.printf("[FaultLog] Recorded fault: code=%u, val=%d, time=%u\n",
                (unsigned)code, (int)value, (unsigned)now);
}

static inline int haffler_fault_log_get_count(void) {
  if (!s_fault_log_inited) haffler_fault_log_init();
  return s_fault_log.count;
}

static inline bool haffler_fault_log_get_entry(int index, haffler_fault_entry_t *out) {
  if (!s_fault_log_inited) haffler_fault_log_init();
  if (!out || index < 0 || index >= (int)s_fault_log.count) return false;
  // شاخص ۰ = جدیدترین خطا
  int actual_idx = (s_fault_log.head - 1 - index + HAFFLER_FAULT_MAX_ENTRIES * 2) % HAFFLER_FAULT_MAX_ENTRIES;
  *out = s_fault_log.entries[actual_idx];
  return true;
}

static inline void haffler_fault_log_clear(void) {
  if (!s_fault_log_inited) haffler_fault_log_init();
  s_fault_log.count = 0;
  s_fault_log.head = 0;
  memset(s_fault_log.entries, 0, sizeof(s_fault_log.entries));
  Preferences p;
  if (p.begin("flt_ns", false)) {
    p.putBytes("flt_log", &s_fault_log, sizeof(s_fault_log));
    p.end();
  }
  Serial.println("[FaultLog] Cleared all entries.");
}

static inline const char *haffler_fault_code_to_str(haffler_fault_code_t code) {
  switch (code) {
    case FAULT_EMERGENCY_STOP:      return "استپ اضطراری شستی پنل";
    case FAULT_BLADE_OVERTRAVEL:    return "خطای تایم اوت کورس تیغه برش";
    case FAULT_CLAMP_TIMEOUT:       return "خطای جک‌های پنوماتیک کلمپ";
    case FAULT_ENCODER_DISCONNECT:  return "خطای خط‌کش دیجیتال و اینکودر";
    case FAULT_BROWNOUT:            return "افت ولتاژ شدید برق ورودی";
    case FAULT_WATCHDOG_RESET:      return "ریست سیستم توسط واچ‌داگ";
    case FAULT_CORE_PANIC:          return "کرش هسته پردازنده میکروکنترلر";
    case FAULT_RTC_ERROR:           return "خطای ارتباط با ساعت سخت‌افزاری";
    case FAULT_TOUCH_ERROR:         return "خطای ارتباط با تاچ خازنی GT911";
    case FAULT_LEASE_EXPIRED:       return "پایان مهلت لایسنس و قفل روزشمار";
    default:                        return "بدون خطا / عملکرد عادی";
  }
}
